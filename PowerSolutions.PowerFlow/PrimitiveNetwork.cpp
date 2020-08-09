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
			//���Ц��ε�ֵ��·
			int bus1 = Nodes(pbus1)->Index;
			int bus2 = Nodes(pbus2)->Index;
			_PS_TRACE(bus1 << " -- " << bus2 << "\t" << 1.0 / pieqv.Impedance() << "\t" << pieqv.Admittance1() << "\t" << pieqv.Admittance2());
			//BUG CLOSED
			//��ϵ���������Ԫ����������ʱ��
			//���ܻᵼ��Ԥ�Ȼ�ȡ����Ԫ�ض�Ӧ��ַ�����ǣ�
			//��˲�����ǰʹ�����ñ�������λ�á�
			//�����ʱ����ʹ�� coeffRef ���ɵõ���ȷ�Ľ����
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
			//LoadNetworkCase ֻ�ܵ���һ�Ρ�
			assert(m_SourceNetwork == nullptr);
			m_SourceNetwork = network;
			//���þֲ�����
			m_Buses.clear();
			m_BusMapping.clear();
			m_Branches.clear();
			m_Nodes.clear();
			m_PQNodes.clear();
			m_PVNodes.clear();
			m_SlackNode = nullptr;
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
			m_BusMapping.reserve(m_Buses.size());
			for (auto &obj : m_Buses)
			{
				//Ĭ��PQ�ڵ�
				m_BusMapping.emplace(obj, new NodeInfo(obj));
			}
			//������еĽڵ�������ӣ���֧·����Ϊ n(n-1)/2
			//�˴�����ÿ��ĸ���Ͼ���6�ؽ���
			m_Branches.reserve(m_Buses.size() * 3);
			//����Ԫ����Ϣ
			for (auto& obj : network->Objects())
			{
				auto* c = dynamic_cast<Component*>(obj);
				if (c != nullptr)
				{
					//����ڵ���Ϣ
					//�����ڵ�Ķ�
					c->BuildNodeInfo(this);
				}
			}
			//ע�⣬��ʱ��ͳ�Ƶ�PQ�ڵ������л������˹����Ľڵ�
			//�� BusMapping ���Ƴ�δ�����õĽڵ㡣
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
			//����Ƿ����ƽ��ڵ㡣
			if (m_SlackNode == nullptr) throw Exception(ExceptionCode::SlackBus);
			//ͳ��PQ/PV�ڵ�����������Ԥ���ռ䡣
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
			//���ƽڵ��б�
			m_Nodes.resize(PQNodeCount + PVNodeCount + 1);
			transform(m_BusMapping.cbegin(), m_BusMapping.cend(), m_Nodes.begin(),
				[](const NodeDictionary::value_type &item){return item.second; });
			//���þ�̬�ڵ��Ż����,�����ڵ�ĳ�������С������������
			//��Nodes�б��������
			if (m_NodeReorder)
			{
				sort(m_Nodes.begin(), m_Nodes.end(),
					[](const NodeCollection::value_type &x, const NodeCollection::value_type &y)
				{
					//��ƽ��ڵ�ŵ��б��ĩβ
					if (x->Type == NodeType::SlackNode) return false;
					if (y->Type == NodeType::SlackNode) return true;
					return x->Degree() < y->Degree();
				});
			} else {
				//������Σ�ƽ��ڵ�Ӧ���� Nodes ���ϵ�����档
				//�˴����ǵ����ܣ���������ƽ��ڵ�ͳ�ƽ��ڵ��������һ���ڵ��λ�á�
				swap(*find_if(m_Nodes.begin(), m_Nodes.end(), [](NodeInfo* node){return node->Type == NodeType::SlackNode; }),
					m_Nodes.back());
			}
			//�����µ�˳�����±��
			int IndexCounter1 = 0, IndexCounter2 = 0;
			m_PQNodes.reserve(PQNodeCount);
			m_PVNodes.reserve(PVNodeCount);
			//TODO �Ż�����Ĵ洢
			//CASE ���û��PV�ڵ㣬�ᵼ���쳣
			for (auto node : m_Nodes)
			{
				assert(node->Degree() > 0);
				//Ϊ�ڵ��š�
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
			//ƽ��ڵ��ŷ�������档
			m_SlackNode->Index = IndexCounter1 + IndexCounter2;
			m_SlackNode->SubIndex = 0;

			_PS_TRACE("Bus\tIndex\tNodeType");
			for (auto& node : m_Nodes)
			{
				_PS_TRACE((size_t)node << "\t" << node->Index << "\t" << (int)node->Type << endl);
			}

			//���ɵ��ɾ���
			// �����Ǿ���row < col
			//Ϊ����ϡ�����Ԥ���ռ�
			Admittance.resize(m_Nodes.size(), m_Nodes.size());
			vector<int> ColSpace;
			ColSpace.resize(m_Nodes.size());
			//���ڵ���ӳ��Ϊ��Ӧ�ڵ��֧·����
			transform(m_Nodes.begin(), m_Nodes.end(), ColSpace.begin(), [](NodeInfo *node){ return node->Degree() * 2 + 1; });
			Admittance.reserve(ColSpace);
			//���ɵ��ɾ���
			for (auto& obj : network->Objects())
			{
				auto* c = dynamic_cast<Component*>(obj);
				if (c != nullptr)
					c->BuildAdmittanceInfo(this);
			}
			Admittance.makeCompressed();
			_PS_TRACE("\n���ɾ��� ==========\n" << Admittance);
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
			auto info = Nodes(bus);
			if (m_SlackNode == nullptr)
			{
				//����ƽ���
				if (info->Type != NodeType::PQNode && abs(info->Voltage - abs(voltagePhasor)) > 1e-10)
					throw Exception(ExceptionCode::VoltageMismatch);
				info->Voltage = abs(voltagePhasor);
				info->Angle = arg(voltagePhasor);
				info->Type = NodeType::SlackNode;
				m_SlackNode = info;
			} else if (m_SlackNode == info)
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
			assert(bus1 != bus2);	//�������Ի���
			//����֧·-����б���
			auto node1 = m_BusMapping.at(bus1);
			auto node2 = m_BusMapping.at(bus2);
			if (m_Branches.insert(make_pair(node1, node2)).second)
			{
				//�ɹ���֧·�б��м��������˵����������֧·��
				Nodes(bus1)->AdjacentNodes.push_back(node2);
				Nodes(bus2)->AdjacentNodes.push_back(node1);
			}
		}

		void PrimitiveNetwork::ClaimParent(Bus* bus, Component* c)
		{
			//����ĸ�ߵ�Ԫ���б��С�
			Nodes(bus)->Components.push_back(c);
		}

		PrimitiveNetwork::NodeInfo::NodeInfo(ObjectModel::Bus* bus) 
			: Bus(bus), Type(NodeType::PQNode),
			Voltage(0), Angle(0)
		{ }
	}
}
