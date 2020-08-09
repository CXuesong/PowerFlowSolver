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
			//���Ц��ε�ֵ��·
			int bus1 = BusMapping[pbus1]->Index;
			int bus2 = BusMapping[pbus2]->Index;
			_PS_TRACE(bus1 << " -- " << bus2 << "\t" << 1.0 / pieqv.Impedance() << "\t" << pieqv.Admittance1() << "\t" << pieqv.Admittance2());
			//BUG CLOSED
			//��ϵ���������Ԫ����������ʱ��
			//���ܻᵼ��Ԥ�Ȼ�ȡ����Ԫ�ض�Ӧ��ַ�����ǣ�
			//�����ʱ����ʹ�� coeffRef ���ɵõ���ȷ�Ľ����
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
			//���þֲ�����
			m_Buses.clear();
			//m_Components.clear();
			BusMapping.clear();
			Branches.clear();
			Nodes.clear();
			PQNodes.clear();
			PVNodes.clear();
			SlackNode = nullptr;
			//����ĸ��
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
			//�ڵ�һ�� for ѭ������ǰ����ͳ��PQ/PV�ڵ���Ŀ��Ϊ�˺��� vector ��ǰԤ���ڴ�ʹ�á�
			BusMapping.reserve(m_Buses.size());
			for (auto &obj : m_Buses)
			{
				//Ĭ��PQ�ڵ�
				BusMapping.emplace(obj, make_shared<NodeInfo>(obj));
			}
			//������еĽڵ�������ӣ���֧·����Ϊ n(n-1)/2
			//�˴�����ÿ��ĸ���Ͼ���6�ؽ���
			Branches.reserve(m_Buses.size() * 3);
			//����Ԫ����Ϣ
			for (auto& obj : network->Objects())
			{
				auto* c = dynamic_cast<Component*>(obj);
				if (c != nullptr) c->BuildNodeInfo(this);
			}
			//ע�⣬��ʱ��ͳ�Ƶ�PQ�ڵ������л������˹����Ľڵ�
			//�� BusMapping ���Ƴ�δ�����õĽڵ㡣
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
			//����Ƿ����ƽ��ڵ㡣
			if (SlackNode == nullptr) throw Exception(ExceptionCode::SlackBus);
			//ͳ��PQ/PV�ڵ�����������Ԥ���ռ䡣
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
			//���ƽڵ��б�
			Nodes.resize(PQNodeCount + PVNodeCount + 1);
			transform(BusMapping.cbegin(), BusMapping.cend(), Nodes.begin(),
				[](const NodeDictionary::value_type &item){return item.second.get(); });
			//���þ�̬�ڵ��Ż����,�����ڵ�ĳ�������С������������
			//��Nodes�б��������
			const bool NodeReorder = true;
			if (NodeReorder)
			{
				sort(Nodes.begin(), Nodes.end(),
					[](const NodeCollection::value_type &x, const NodeCollection::value_type &y)
				{
					//��ƽ��ڵ�ŵ��б��ĩβ
					if (x->Type == NodeType::SlackNode) return false;
					if (y->Type == NodeType::SlackNode) return true;
					return x->Degree < y->Degree;
				});
			} else {
				//������Σ�ƽ��ڵ�Ӧ���� Nodes ���ϵ�����档
				//�˴����ǵ����ܣ���������ƽ��ڵ�ͳ�ƽ��ڵ��������һ���ڵ��λ�á�
				swap(*find_if(Nodes.begin(), Nodes.end(), [](NodeInfo* node){return node->Type == NodeType::SlackNode; }),
					Nodes.back());
			}
			//�����µ�˳�����±��
			int IndexCounter1 = 0, IndexCounter2 = 0;
			PQNodes.reserve(PQNodeCount);
			PVNodes.reserve(PVNodeCount);
			//TODO �Ż�����Ĵ洢
			//CASE ���û��PV�ڵ㣬�ᵼ���쳣
			for (auto node : Nodes)
			{
				assert(node->Degree > 0);
				//Ϊ�ڵ��š�
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
			//ƽ��ڵ��ŷ�������档
			SlackNode->Index = IndexCounter1 + IndexCounter2;
			SlackNode->SubIndex = 0;

			_PS_TRACE("Bus\tIndex\tNodeType");
			for (auto& node : Nodes)
			{
				_PS_TRACE((size_t)node << "\t" << node->Index << "\t" << (int)node->Type << endl);
			}

			//���ɵ��ɾ���
			// �����Ǿ���row < col
			//Ϊ����ϡ�����Ԥ���ռ�
			Admittance.resize(Nodes.size(), Nodes.size());
			vector<int> ColSpace;
			ColSpace.resize(Nodes.size());
			//���ڵ���ӳ��Ϊ��Ӧ�ڵ��֧·����
			transform(Nodes.begin(), Nodes.end(), ColSpace.begin(), [](NodeInfo *node){ return node->Degree + 1; });
			//����֧·����+1��Ԥ�������пռ䡣
			Admittance.reserve(ColSpace);
			//���ɵ��ɾ���
			for (auto& obj : network->Objects())
			{
				auto* c = dynamic_cast<Component*>(obj);
				if (c != nullptr)
					c->BuildAdmittanceInfo(this);
			}
			_PS_TRACE("\n���ɾ��� ==========\n" << Admittance);
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
			//false ��ʾ��������ǲ����Եģ���Ҫ�����쳣��
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
				//����ƽ���
				if (info->Type != NodeType::PQNode && abs(info->Voltage - abs(voltagePhasor)) > 1e-10)
					throw Exception(ExceptionCode::VoltageMismatch);
				info->Voltage = abs(voltagePhasor);
				info->Angle = arg(voltagePhasor);
				info->Type = NodeType::SlackNode;
				SlackNode = info;
			} else if (SlackNode == info)
			{
				//��ͬһ��ĸ���Ϸ����˶�̨ƽ�ⷢ���
				//���ƽ�����ѹ
				if (abs(info->Voltage - abs(voltagePhasor)) > 1e-10)
					throw Exception(ExceptionCode::VoltageMismatch);
			} else {
				//���ڶ���һ̨ƽ�ⷢ���
				throw Exception(ExceptionCode::SlackBus);
			}

		}

		void PrimitiveNetwork::ClaimBranch(Bus* bus1, Bus* bus2)
		{
			//����֧·-����б���
			if (Branches.insert(make_pair(bus1, bus2)).second)
			{
				//�ɹ���֧·�б��м��������˵����������֧·��
				BusMapping[bus1]->Degree++;
				BusMapping[bus2]->Degree++;
			}
		}

		void PrimitiveNetwork::ClaimParent(Bus* bus, SinglePortComponent* c)
		{
			//����ĸ�ߵĵ���Ԫ���б��С�
			BusMapping[bus]->Components.push_back(c);
		}

		PrimitiveNetwork::NodeInfo::NodeInfo(ObjectModel::Bus* bus) 
			: Bus(bus), Type(NodeType::PQNode),
			Voltage(0), Angle(0)
		{ }
	}
}
