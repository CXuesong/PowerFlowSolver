#include "stdafx.h"
#include "PrimitiveNetwork.h"
#include "Exceptions.h"
#include "NetworkCase.h"
#include "Utility.h"

using namespace std;
using namespace PowerSolutions::Utility;

namespace PowerSolutions {
	namespace ObjectModel {

		PrimitiveNetwork::PrimitiveNetwork(ObjectModel::NetworkCase& network, bool nodeReorder)
			: PrimitiveNetwork(network, nodeReorder, false)
		{
			
		}

		PrimitiveNetwork::PrimitiveNetwork(ObjectModel::NetworkCase& network, bool nodeReorder, bool ignoreShuntAdmittance)
			: m_SourceNetwork(nullptr), m_SlackNode(nullptr), m_NodeReorder(nodeReorder), m_IgnoreShuntAdmittance(ignoreShuntAdmittance)
		{
			this->LoadNetworkCase(&network);
		}

		PrimitiveNetwork::~PrimitiveNetwork()
		{
			for (auto& item : m_Nodes)
				delete item;
		}

		void PrimitiveNetwork::AddPi(Bus* pbus1, Bus* pbus2, PiEquivalencyParameters pieqv)
		{
			//具有π形等值电路
			int bus1 = Nodes(pbus1)->Index;
			int bus2 = Nodes(pbus2)->Index;
			_PS_TRACE(bus1 << " -- " << bus2 << "\t" << 1.0 / pieqv.Impedance() << "\t" << pieqv.Admittance1() << "\t" << pieqv.Admittance2());
			//BUG CLOSED
			//当系数矩阵非零元素数量增加时，
			//可能会导致预先获取的零元素对应地址被覆盖，
			//因此不能提前使用引用变量保存位置。
			//如果此时重新使用 coeffRef 即可得到正确的结果。
			complexd transAdmittance = 1.0 / pieqv.Impedance();
			if (m_IgnoreShuntAdmittance)
			{
				Admittance.coeffRef(bus1, bus1) += transAdmittance + pieqv.Admittance1();
				Admittance.coeffRef(bus2, bus2) += transAdmittance + pieqv.Admittance2();
			} else {
				Admittance.coeffRef(bus1, bus1) += transAdmittance;
				Admittance.coeffRef(bus2, bus2) += transAdmittance;
			}
			Admittance.coeffRef(bus1, bus2) -= transAdmittance;
			Admittance.coeffRef(bus2, bus1) -= transAdmittance;
			//TraceFile << pieqv->PiAdmittance1() << " /- " << transAdmittance << " -\\ " << pieqv->PiAdmittance2() << endl;
			assert(!isnan(Admittance.coeffRef(bus1, bus1).imag()));
			assert(!isnan(Admittance.coeffRef(bus2, bus2).imag()));
			assert(!isnan(Admittance.coeffRef(bus1, bus2).imag()));
			assert(!isnan(Admittance.coeffRef(bus2, bus1).imag()));
		}

		void PrimitiveNetwork::LoadNetworkCase(ObjectModel::NetworkCase* network)
		{
			//LoadNetworkCase 只能调用一次。
			assert(m_SourceNetwork == nullptr);
			m_SourceNetwork = network;
			//重置局部变量
			m_Buses.clear();
			m_BusMapping.clear();
			m_Branches.clear();
			m_Nodes.clear();
			m_PQNodes.clear();
			m_PVNodes.clear();
			m_SlackNode = nullptr;
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
			m_BusMapping.reserve(m_Buses.size());
			for (auto &obj : m_Buses)
			{
				//默认PQ节点
				m_BusMapping.emplace(obj, new NodeInfo(obj));
			}
			//如果所有的节点均有连接，则支路数量为 n(n-1)/2
			//此处假设每个母线上均有6回接线
			m_Branches.reserve(m_Buses.size() * 3);
			//载入元件信息
			for (auto& obj : network->Objects())
			{
				auto* c = dynamic_cast<Component*>(obj);
				if (c != nullptr)
				{
					//补充节点信息
					//包括节点的度
					c->BuildNodeInfo(this);
				}
			}
			//注意，此时的统计的PQ节点数量中还包含了孤立的节点
			//从 BusMapping 中移除未被引用的节点。
			assert(m_BusMapping.size() == m_Buses.size());
			while (true)
			{
				auto i = find_if(m_BusMapping.begin(), m_BusMapping.end(),
					[](NodeDictionary::value_type &item){return item.second->Degree() == 0; });
				if (i != m_BusMapping.end())
					m_BusMapping.erase(i);
				else
					break;
			};
			//检查是否存在平衡节点。
			if (m_SlackNode == nullptr) throw Exception(ExceptionCode::SlackBus);
			//统计PQ/PV节点数量，便于预留空间。
			size_t PQNodeCount = 0, PVNodeCount = 0;
			for (auto& p : m_BusMapping)
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
			m_Nodes.resize(PQNodeCount + PVNodeCount + 1);
			transform(m_BusMapping.cbegin(), m_BusMapping.cend(), m_Nodes.begin(),
				[](const NodeDictionary::value_type &item){return item.second; });
			//采用静态节点优化编号,即将节点的出线数从小到大依次排列
			//对Nodes列表进行排序。
			if (m_NodeReorder)
			{
				sort(m_Nodes.begin(), m_Nodes.end(),
					[](const NodeCollection::value_type &x, const NodeCollection::value_type &y)
				{
					//将平衡节点放到列表的末尾
					if (x->Type == NodeType::SlackNode) return false;
					if (y->Type == NodeType::SlackNode) return true;
					return x->Degree() < y->Degree();
				});
			} else {
				//不论如何，平衡节点应该在 Nodes 集合的最后面。
				//此处考虑到性能，仅仅交换平衡节点和除平衡节点以外最后一个节点的位置。
				swap(*find_if(m_Nodes.begin(), m_Nodes.end(), [](NodeInfo* node){return node->Type == NodeType::SlackNode; }),
					m_Nodes.back());
			}
			//按照新的顺序重新编号
			int IndexCounter1 = 0, IndexCounter2 = 0;
			m_PQNodes.reserve(PQNodeCount);
			m_PVNodes.reserve(PVNodeCount);
			//TODO 优化冗余的存储
			//CASE 如果没有PV节点，会导致异常
			for (auto node : m_Nodes)
			{
				assert(node->Degree() > 0);
				//为节点编号。
				node->Index = IndexCounter1 + IndexCounter2;
				if (node->Type == NodeType::PQNode)
				{
					node->SubIndex = IndexCounter1;
					IndexCounter1++;
					m_PQNodes.push_back(node);
				} else if (node->Type == NodeType::PVNode)
				{
					node->SubIndex = IndexCounter2;
					IndexCounter2++;
					m_PVNodes.push_back(node);
				}
			}
			//平衡节点编号放在最后面。
			m_SlackNode->Index = IndexCounter1 + IndexCounter2;
			m_SlackNode->SubIndex = 0;

			_PS_TRACE("Bus\tIndex\tNodeType");
			for (auto& node : m_Nodes)
			{
				_PS_TRACE((size_t)node << "\t" << node->Index << "\t" << (int)node->Type << endl);
			}

			//生成导纳矩阵。
			// 上三角矩阵，row < col
			//为导纳稀疏矩阵预留空间
			Admittance.resize(m_Nodes.size(), m_Nodes.size());
			vector<int> ColSpace;
			ColSpace.resize(m_Nodes.size());
			//将节点表格映射为对应节点的支路数量
			transform(m_Nodes.begin(), m_Nodes.end(), ColSpace.begin(), [](NodeInfo *node){ return node->Degree() * 2 + 1; });
			Admittance.reserve(ColSpace);
			//生成导纳矩阵
			for (auto& obj : network->Objects())
			{
				auto* c = dynamic_cast<Component*>(obj);
				if (c != nullptr)
					c->BuildAdmittanceInfo(this);
			}
			Admittance.makeCompressed();
			_PS_TRACE("\n导纳矩阵 ==========\n" << Admittance);
		}

		void PrimitiveNetwork::AddShunt(Bus* bus, complexd admittance)
		{
			if (!m_IgnoreShuntAdmittance)
			{
				auto index = Nodes(bus)->Index;
				Admittance.coeffRef(index, index) += admittance;
			}
		}

		void PrimitiveNetwork::AddPQ(Bus* bus, complexd power)
		{
			auto info = Nodes(bus);
			info->ActivePowerInjection += power.real();
			info->ReactivePowerInjection += power.imag();
		}

		void PrimitiveNetwork::AddPV(Bus* bus, double activePower, double voltage)
		{
			auto info = Nodes(bus);
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
			auto info = Nodes(bus);
			if (m_SlackNode == nullptr)
			{
				//设置平衡机
				if (info->Type != NodeType::PQNode && abs(info->Voltage - abs(voltagePhasor)) > 1e-10)
					throw Exception(ExceptionCode::VoltageMismatch);
				info->Voltage = abs(voltagePhasor);
				info->Angle = arg(voltagePhasor);
				info->Type = NodeType::SlackNode;
				m_SlackNode = info;
			} else if (m_SlackNode == info)
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
			assert(bus1 != bus2);	//不允许自环。
			//加入支路-组件列表中
			auto node1 = m_BusMapping.at(bus1);
			auto node2 = m_BusMapping.at(bus2);
			if (m_Branches.insert(make_pair(node1, node2)).second)
			{
				//成功向支路列表中加入了新项，说明出现了新支路。
				Nodes(bus1)->AdjacentNodes.push_back(node2);
				Nodes(bus2)->AdjacentNodes.push_back(node1);
			}
		}

		void PrimitiveNetwork::ClaimParent(Bus* bus, Component* c)
		{
			//加入母线的元件列表中。
			Nodes(bus)->Components.push_back(c);
		}

		PrimitiveNetwork::NodeInfo::NodeInfo(ObjectModel::Bus* bus) 
			: Bus(bus), Type(NodeType::PQNode),
			Voltage(0), Angle(0)
		{ }
	}
}
