#include "stdafx.h"
#include "PrimitiveNetwork.h"
#include "Exceptions.h"
#include "NetworkCase.h"
#include "Utility.h"

using namespace std;
using namespace PowerSolutions::Utility;

namespace PowerSolutions {
	namespace ObjectModel {

		void PrimitiveNetwork::AddPi(Bus* pbus1, Bus* pbus2, PiEquivalencyParameters pieqv)
		{
			//具有π形等值电路
			int bus1 = BusMapping[pbus1]->Index;
			int bus2 = BusMapping[pbus2]->Index;
			_PS_TRACE(bus1 << " -- " << bus2 << "\t" << 1.0 / pieqv.Impedance() << "\t" << pieqv.Admittance1() << "\t" << pieqv.Admittance2());
			//BUG CLOSED
			//当系数矩阵非零元素数量增加时，
			//可能会导致预先获取的零元素对应地址被覆盖，
			//如果此时重新使用 coeffRef 即可得到正确的结果。
			complexd transAdmittance = 1.0 / pieqv.Impedance();
			Admittance.coeffRef(bus1, bus1) += transAdmittance + pieqv.Admittance1();
			Admittance.coeffRef(bus2, bus2) += transAdmittance + pieqv.Admittance2();
			Admittance.coeffRef(bus1, bus2) -= transAdmittance;
			Admittance.coeffRef(bus2, bus1) -= transAdmittance;
			//TraceFile << pieqv->PiAdmittance1() << " /- " << transAdmittance << " -\\ " << pieqv->PiAdmittance2() << endl;
			assert(!isnan(Admittance.coeffRef(bus1, bus1).imag()));
			assert(!isnan(Admittance.coeffRef(bus2, bus2).imag()));
			assert(!isnan(Admittance.coeffRef(bus1, bus2).imag()));
			assert(!isnan(Admittance.coeffRef(bus2, bus1).imag()));
		}

		void PrimitiveNetwork::LoadNetworkCase(NetworkCase* network, bool nodeReorder)
		{
			m_SourceNetwork = network;
			//重置局部变量
			m_Buses.clear();
			//m_Components.clear();
			BusMapping.clear();
			Branches.clear();
			Nodes.clear();
			PQNodes.clear();
			PVNodes.clear();
			SlackNode = nullptr;
			//载入母线
			for (auto& obj : network->Objects())
			{
				auto b = dynamic_cast<Bus*>(obj);
				if (b != nullptr)
				{
					m_Buses.push_back(b);
					continue;
				}
				auto bc = dynamic_cast<IBusContainer*>(obj);
				if (bc != nullptr)
				{
					for (auto i = 0, j = bc->ChildBusCount(); i < j; i++)
					{
						auto b = bc->ChildBusAt(i);
						assert(b != nullptr);
						m_Buses.push_back(b);
					}
				}
			}
			//在第一个 for 循环中提前粗略统计PQ/PV节点数目是为了后面 vector 提前预留内存使用。
			BusMapping.reserve(m_Buses.size());
			for (auto &obj : m_Buses)
			{
				//默认PQ节点
				BusMapping.emplace(obj, make_shared<NodeInfo>(obj));
			}
			//如果所有的节点均有连接，则支路数量为 n(n-1)/2
			//此处假设每个母线上均有6回接线
			Branches.reserve(m_Buses.size() * 3);
			//载入元件信息
			for (auto& obj : network->Objects())
			{
				auto* c = dynamic_cast<Component*>(obj);
				if (c != nullptr) c->BuildNodeInfo(this);
			}
			//注意，此时的统计的PQ节点数量中还包含了孤立的节点
			//从 BusMapping 中移除未被引用的节点。
			assert(BusMapping.size() == m_Buses.size());
			while (true)
			{
				auto i = find_if(BusMapping.begin(), BusMapping.end(),
					[](NodeDictionary::value_type &item){return item.second->Degree == 0; });
				if (i != BusMapping.end())
					BusMapping.erase(i);
				else
					break;
			};
			//检查是否存在平衡节点。
			if (SlackNode == nullptr) throw Exception(ExceptionCode::SlackBus);
			//统计PQ/PV节点数量，便于预留空间。
			size_t PQNodeCount = 0, PVNodeCount = 0;
			for (auto& p : BusMapping)
			{
				switch (p.second->Type)
				{
				case NodeType::PQNode:
					PQNodeCount++;
					break;
				case NodeType::PVNode:
					PVNodeCount++;
					break;
				}
			}
			//复制节点列表。
			Nodes.resize(PQNodeCount + PVNodeCount + 1);
			transform(BusMapping.cbegin(), BusMapping.cend(), Nodes.begin(),
				[](const NodeDictionary::value_type &item){return item.second.get(); });
			//采用静态节点优化编号,即将节点的出线数从小到大依次排列
			//对Nodes列表进行排序。
			const bool NodeReorder = true;
			if (NodeReorder)
			{
				sort(Nodes.begin(), Nodes.end(),
					[](const NodeCollection::value_type &x, const NodeCollection::value_type &y)
				{
					//将平衡节点放到列表的末尾
					if (x->Type == NodeType::SlackNode) return false;
					if (y->Type == NodeType::SlackNode) return true;
					return x->Degree < y->Degree;
				});
			} else {
				//不论如何，平衡节点应该在 Nodes 集合的最后面。
				//此处考虑到性能，仅仅交换平衡节点和除平衡节点以外最后一个节点的位置。
				swap(*find_if(Nodes.begin(), Nodes.end(), [](NodeInfo* node){return node->Type == NodeType::SlackNode; }),
					Nodes.back());
			}
			//按照新的顺序重新编号
			int IndexCounter1 = 0, IndexCounter2 = 0;
			PQNodes.reserve(PQNodeCount);
			PVNodes.reserve(PVNodeCount);
			//TODO 优化冗余的存储
			//CASE 如果没有PV节点，会导致异常
			for (auto node : Nodes)
			{
				assert(node->Degree > 0);
				//为节点编号。
				node->Index = IndexCounter1 + IndexCounter2;
				if (node->Type == NodeType::PQNode)
				{
					node->SubIndex = IndexCounter1;
					IndexCounter1++;
					PQNodes.push_back(node);
				} else if (node->Type == NodeType::PVNode)
				{
					node->SubIndex = IndexCounter2;
					IndexCounter2++;
					PVNodes.push_back(node);
				}
			}
			//平衡节点编号放在最后面。
			SlackNode->Index = IndexCounter1 + IndexCounter2;
			SlackNode->SubIndex = 0;

			_PS_TRACE("Bus\tIndex\tNodeType");
			for (auto& node : Nodes)
			{
				_PS_TRACE((size_t)node << "\t" << node->Index << "\t" << (int)node->Type << endl);
			}

			//生成导纳矩阵。
			// 上三角矩阵，row < col
			//为导纳稀疏矩阵预留空间
			Admittance.resize(Nodes.size(), Nodes.size());
			vector<int> ColSpace;
			ColSpace.resize(Nodes.size());
			//将节点表格映射为对应节点的支路数量
			transform(Nodes.begin(), Nodes.end(), ColSpace.begin(), [](NodeInfo *node){ return node->Degree + 1; });
			//按照支路数量+1来预留矩阵列空间。
			Admittance.reserve(ColSpace);
			//生成导纳矩阵
			for (auto& obj : network->Objects())
			{
				auto* c = dynamic_cast<Component*>(obj);
				if (c != nullptr)
					c->BuildAdmittanceInfo(this);
			}
			_PS_TRACE("\n导纳矩阵 ==========\n" << Admittance);
		}

		PrimitiveNetwork::PrimitiveNetwork(ObjectModel::NetworkCase& network, bool nodeReorder)
		{
			this->LoadNetworkCase(&network, nodeReorder);
		}

		void PrimitiveNetwork::AddShunt(Bus* bus, complexd admittance)
		{
			auto index = BusMapping[bus]->Index;
			Admittance.coeffRef(index, index) += admittance;
		}

		void PrimitiveNetwork::AddPQ(Bus* bus, complexd power)
		{
			auto info = BusMapping[bus];
			info->ActivePowerInjection += power.real();
			info->ReactivePowerInjection += power.imag();
		}

		void PrimitiveNetwork::AddPV(Bus* bus, double activePower, double voltage)
		{
			auto info = BusMapping[bus];
			//false 表示这种情况是不可以的，需要引发异常。
			if (info->Type != NodeType::PQNode && abs(info->Voltage - voltage) > 1e-10)
				throw Exception(ExceptionCode::VoltageMismatch);
			if (info->Type != NodeType::SlackNode)
			{
				info->Voltage = voltage;
				info->ActivePowerInjection += activePower;
				info->Type = NodeType::PVNode;
			}
		}

		void PrimitiveNetwork::AddSlack(Bus* bus, complexd voltagePhasor)
		{
			auto info = BusMapping[bus];
			if (SlackNode == nullptr)
			{
				//设置平衡机
				if (info->Type != NodeType::PQNode && abs(info->Voltage - abs(voltagePhasor)) > 1e-10)
					throw Exception(ExceptionCode::VoltageMismatch);
				info->Voltage = abs(voltagePhasor);
				info->Angle = arg(voltagePhasor);
				info->Type = NodeType::SlackNode;
				SlackNode = info;
			} else if (SlackNode == info)
			{
				//在同一个母线上放置了多台平衡发电机
				//检查平衡机电压
				if (abs(info->Voltage - abs(voltagePhasor)) > 1e-10)
					throw Exception(ExceptionCode::VoltageMismatch);
			} else {
				//存在多于一台平衡发电机
				throw Exception(ExceptionCode::SlackBus);
			}

		}

		void PrimitiveNetwork::ClaimBranch(Bus* bus1, Bus* bus2)
		{
			//加入支路-组件列表中
			if (Branches.insert(make_pair(bus1, bus2)).second)
			{
				//成功向支路列表中加入了新项，说明出现了新支路。
				BusMapping[bus1]->Degree++;
				BusMapping[bus2]->Degree++;
			}
		}

		void PrimitiveNetwork::ClaimParent(Bus* bus, SinglePortComponent* c)
		{
			//加入母线的单端元件列表中。
			BusMapping[bus]->Components.push_back(c);
		}

		PrimitiveNetwork::NodeInfo::NodeInfo(ObjectModel::Bus* bus) 
			: Bus(bus), Type(NodeType::PQNode),
			Voltage(0), Angle(0)
		{ }
	}
}
