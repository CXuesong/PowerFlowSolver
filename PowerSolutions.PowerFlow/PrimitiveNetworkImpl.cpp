#include "stdafx.h"
#include "PrimitiveNetworkImpl.h"
#include "Exceptions.h"
#include "NetworkCase.h"

using namespace std;

namespace PowerSolutions {
	namespace ObjectModel {
		

		inline void PrimitiveNetworkImpl::NodeInfo::AddPQ(complexd power)
		{
			ActivePowerInjection += power.real();
			ReactivePowerInjection += power.imag();
		}

		inline bool PrimitiveNetworkImpl::NodeInfo::AddPV(double activePower, double voltage)
		{
			//false ��ʾ��������ǲ����Եģ���Ҫ�����쳣��
			if (Type != NodeType::PQNode && std::abs(Voltage - voltage) > 1e-10)
				return false;
			if (Type != NodeType::SlackNode)
			{
				Voltage = voltage;
				ActivePowerInjection += activePower;
				Type = NodeType::PVNode;
			}
			return true;
		}

		inline bool PrimitiveNetworkImpl::NodeInfo::AddSlack(complexd voltagePhasor)
		{
			if (Type != NodeType::PQNode && std::abs(Voltage - std::abs(voltagePhasor)) > 1e-10)
				return false;
			Voltage = std::abs(voltagePhasor);
			Angle = std::arg(voltagePhasor);
			Type = NodeType::SlackNode;
			return true;
		}

		inline complexd PrimitiveNetworkImpl::NodeInfo::VoltagePhasor()
		{
			return polar(Voltage, Angle);
		}

		//////////////////////////////////////////

		void PrimitiveNetworkImpl::AddPi(Bus* pbus1, Bus* pbus2, PiEquivalencyParameters pieqv)
		{
			//���Ц��ε�ֵ��·
			int bus1 = BusMapping[pbus1]->Index;
			int bus2 = BusMapping[pbus2]->Index;
			//BUG CLOSED
			//��ϵ���������Ԫ����������ʱ��
			//���ܻᵼ��Ԥ�Ȼ�ȡ����Ԫ�ض�Ӧ��ַ�����ǣ�
			//�����ʱ����ʹ�� coeffRef ���ɵõ���ȷ�Ľ����
			complexd transAdmittance = 1.0 / pieqv.Impedance();
			Admittance.coeffRef(bus1, bus1) += transAdmittance + pieqv.Admittance1();
			Admittance.coeffRef(bus2, bus2) += transAdmittance + pieqv.Admittance2();
			if (bus1 > bus2) swap(bus1, bus2);
			Admittance.coeffRef(bus1, bus2) -= transAdmittance;
			//TraceFile << pieqv->PiAdmittance1() << " /- " << transAdmittance << " -\\ " << pieqv->PiAdmittance2() << endl;
			assert(!isnan(Admittance.coeffRef(bus1, bus1).imag()));
			assert(!isnan(Admittance.coeffRef(bus2, bus2).imag()));
			assert(!isnan(Admittance.coeffRef(bus1, bus2).imag()));
		}

		void PrimitiveNetworkImpl::AddShunt(Bus* bus, complexd shunt)
		{
			throw std::logic_error("The method or operation is not implemented.");
		}

		void PrimitiveNetworkImpl::SetNodeType(Bus* bus, NodeType type)
		{
			throw std::logic_error("The method or operation is not implemented.");
		}

		void PrimitiveNetworkImpl::FromNetworkCase(NetworkCase& network)
		{
			//TODO �滻 * / &
			m_SourceNetwork = &network;
			//���þֲ�����
			BusMapping.clear();
			Branches.clear();
			Nodes.clear();
			PQNodes.clear();
			PVNodes.clear();
			SlackNode = nullptr;
			//�ڵ�һ�� for ѭ������ǰ����ͳ��PQ/PV�ڵ���Ŀ��Ϊ�˺��� vector ��ǰԤ���ڴ�ʹ�á�
			NodeCount = PQNodeCount = (int)(m_Buses.size());
			PVNodeCount = 0;
			BusMapping.reserve(NodeCount);
			for (auto &obj : m_Buses)
			{
				//Ĭ��PQ�ڵ�
				shared_ptr<NodeInfo> info(new NodeInfo(obj));
				BusMapping.emplace(obj, info);
			}
			//������еĽڵ�������ӣ���֧·����Ϊ n(n-1)/2
			//�˴�����ÿ��ĸ���Ͼ���6�ؽ���
			Branches.reserve(m_Buses.size() * 3);
#if 0
			for (auto &obj : network.Components())
			{
				//ͳ��ÿ��ĸ�߱����ӵĴ�����
				//ע��˴�������˫��Ԫ��
				Component* cp = dynamic_cast<Component*>(obj);
				if (cp != nullptr)
				{
					if (cp->PortCount() == 2)
					{
						//����֧·-����б���
						if (Branches.insert(make_pair(cp->Buses(0), cp->Buses(1))).second)
						{
							//�ɹ���֧·�б��м��������˵����������֧·��
							BusMapping[cp->Buses(0)]->Degree++;
							BusMapping[cp->Buses(1)]->Degree++;
						}
					}
				}
				//����ȷ���ڵ����ͣ���ͳ�ƹ���ע�������
				//����PQ����
				auto *pqload = dynamic_cast<PQLoad*>(obj);
				if (pqload != nullptr)
				{
					BusMapping[pqload->Bus1()]->AddPQ(-pqload->Power());
				}
				//����PV�����
				auto *pvgen = dynamic_cast<PVGenerator*>(obj);
				if (pvgen != nullptr)
				{
					auto &node = BusMapping[pvgen->Bus1()];
					if (node->Type == NodeType::PQNode)
					{
						//�������һ̨PV�������������ĸ��ΪPV/ƽ��ڵ㡣
						//PQ -> PV
						PQNodeCount--;
						PVNodeCount++;
					}
					//����/У���ѹԼ��
					if (!node->AddPV(pvgen->ActivePower(), pvgen->Voltage()))
						throw Exception(ExceptionCode::VoltageMismatch);
				}
				//����ƽ�ⷢ���
				auto *slackgen = dynamic_cast<SlackGenerator*>(obj);
				if (slackgen != nullptr)
				{
					//�������һ̨ƽ�ⷢ�����������ĸ��Ϊƽ��ڵ㡣
					auto &node = BusMapping[slackgen->Bus1()];
					if (SlackNode == nullptr)
					{
						// PQ / PV -> Slack
						if (node->Type == NodeType::PQNode) PQNodeCount--;
						else if (node->Type == NodeType::PVNode) PVNodeCount--;
						SlackNode = node;
						node->AddSlack(slackgen->Voltage());
					} else if (SlackNode == node)
					{
						//��ͬһ��ĸ���Ϸ����˶�̨ƽ�ⷢ�����
						if (!node->AddSlack(slackgen->Voltage()))
							throw Exception(ExceptionCode::VoltageMismatch);
					} else {
						//���ڶ���һ̨ƽ�ⷢ���
						throw Exception(ExceptionCode::SlackBus);
					}
				}
			}
			//ʹ��ı��������ˡ���
			//ע�⣬��ʱ��ͳ�Ƶ�PQ�ڵ������л������˹����Ľڵ�
			//�� BusMapping ���Ƴ�δ�����õĽڵ㡣
			assert(BusMapping.size() == NodeCount);
			while (true)
			{
				auto i = find_if(BusMapping.begin(), BusMapping.end(),
					[](NodeDictionary::value_type &item){return item.second->Degree == 0; });
				if (i != BusMapping.end())
					BusMapping.erase(i);
				else
					break;
			}
			//����Ƿ����ƽ��ڵ㡣
			if (SlackNode == nullptr)
				throw Exception(ExceptionCode::SlackBus);
			//�۳� PQNodes �а�����δ�����õĽڵ�������
			PQNodeCount -= NodeCount - BusMapping.size();
			NodeCount = BusMapping.size();
			//���ƽڵ��б�
			Nodes.resize(NodeCount);
			transform(BusMapping.cbegin(), BusMapping.cend(), Nodes.begin(),
				[](const NodeDictionary::value_type &item){return item.second.get(); });
			_PS_TRACE("Bus\tNodeType");
			for (auto& node : Nodes)
			{
				_PS_TRACE((size_t)node << "\t" << (int)node->Type << endl);
			}
			//���þ�̬�ڵ��Ż����,�����ڵ�ĳ�������С������������
			//��Nodes�б��������
			if (NodeReorder())
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
				//�˴����ǵ����ܣ���������ƽ��ڵ�����һ���ڵ��λ�á�
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
			SlackNode->SubIndex = 0;			//���þֲ�����
			BusMapping.clear();
			Branches.clear();
			Nodes.clear();
			PQNodes.clear();
			PVNodes.clear();
			SlackNode = nullptr;
			//�ڵ�һ�� for ѭ������ǰ����ͳ��PQ/PV�ڵ���Ŀ��Ϊ�˺��� vector ��ǰԤ���ڴ�ʹ�á�
			NodeCount = PQNodeCount = (int)(network.Buses().size());
			PVNodeCount = 0;
			BusMapping.reserve(NodeCount);
			for (auto &obj : network.Buses())
			{
				//Ĭ��PQ�ڵ�
				shared_ptr<NodeInfo> info(new NodeInfo(obj));
				BusMapping.emplace(obj, info);
			}
			//������еĽڵ�������ӣ���֧·����Ϊ n(n-1)/2
			//�˴�����ÿ��ĸ���Ͼ���6�ؽ���
			Branches.reserve(network.Buses().size() * 3);
			for (auto &obj : network.Components())
			{
				//ͳ��ÿ��ĸ�߱����ӵĴ�����
				//ע��˴�������˫��Ԫ��
				Component* cp = dynamic_cast<Component*>(obj);
				if (cp != nullptr)
				{
					if (cp->PortCount() == 2)
					{
						//����֧·-����б���
						if (Branches.insert(make_pair(cp->Buses(0), cp->Buses(1))).second)
						{
							//�ɹ���֧·�б��м��������˵����������֧·��
							BusMapping[cp->Buses(0)]->Degree++;
							BusMapping[cp->Buses(1)]->Degree++;
						}
					}
				}
				//����ȷ���ڵ����ͣ���ͳ�ƹ���ע�������
				//����PQ����
				auto *pqload = dynamic_cast<PQLoad*>(obj);
				if (pqload != nullptr)
				{
					BusMapping[pqload->Bus1()]->AddPQ(-pqload->Power());
				}
				//����PV�����
				auto *pvgen = dynamic_cast<PVGenerator*>(obj);
				if (pvgen != nullptr)
				{
					auto &node = BusMapping[pvgen->Bus1()];
					if (node->Type == NodeType::PQNode)
					{
						//�������һ̨PV�������������ĸ��ΪPV/ƽ��ڵ㡣
						//PQ -> PV
						PQNodeCount--;
						PVNodeCount++;
					}
					//����/У���ѹԼ��
					if (!node->AddPV(pvgen->ActivePower(), pvgen->Voltage()))
						throw Exception(ExceptionCode::VoltageMismatch);
				}
				//����ƽ�ⷢ���
				auto *slackgen = dynamic_cast<SlackGenerator*>(obj);
				if (slackgen != nullptr)
				{
					//�������һ̨ƽ�ⷢ�����������ĸ��Ϊƽ��ڵ㡣
					auto &node = BusMapping[slackgen->Bus1()];
					if (SlackNode == nullptr)
					{
						// PQ / PV -> Slack
						if (node->Type == NodeType::PQNode) PQNodeCount--;
						else if (node->Type == NodeType::PVNode) PVNodeCount--;
						SlackNode = node;
						node->AddSlack(slackgen->Voltage());
					} else if (SlackNode == node)
					{
						//��ͬһ��ĸ���Ϸ����˶�̨ƽ�ⷢ�����
						if (!node->AddSlack(slackgen->Voltage()))
							throw Exception(ExceptionCode::VoltageMismatch);
					} else {
						//���ڶ���һ̨ƽ�ⷢ���
						throw Exception(ExceptionCode::SlackBus);
					}
				}
			}
			//ʹ��ı��������ˡ���
			//ע�⣬��ʱ��ͳ�Ƶ�PQ�ڵ������л������˹����Ľڵ�
			//�� BusMapping ���Ƴ�δ�����õĽڵ㡣
			assert(BusMapping.size() == NodeCount);
			while (true)
			{
				auto i = find_if(BusMapping.begin(), BusMapping.end(),
					[](NodeDictionary::value_type &item){return item.second->Degree == 0; });
				if (i != BusMapping.end())
					BusMapping.erase(i);
				else
					break;
			}
			//����Ƿ����ƽ��ڵ㡣
			if (SlackNode == nullptr)
				throw Exception(ExceptionCode::SlackBus);
			//�۳� PQNodes �а�����δ�����õĽڵ�������
			PQNodeCount -= NodeCount - BusMapping.size();
			NodeCount = BusMapping.size();
			//���ƽڵ��б�
			Nodes.resize(NodeCount);
			transform(BusMapping.cbegin(), BusMapping.cend(), Nodes.begin(),
				[](const NodeDictionary::value_type &item){return item.second.get(); });
			_PS_TRACE("Bus\tNodeType");
			for (auto& node : Nodes)
			{
				_PS_TRACE((size_t)node << "\t" << (int)node->Type << endl);
			}
			//���þ�̬�ڵ��Ż����,�����ڵ�ĳ�������С������������
			//��Nodes�б��������
			if (NodeReorder())
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
				//�˴����ǵ����ܣ���������ƽ��ڵ�����һ���ڵ��λ�á�
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
#endif
		}

		void PrimitiveNetworkImpl::FromNetworkCaseCore(PrimitiveNetworkImpl* pNetwork, NetworkCase* network)
		{

		}

		PrimitiveNetworkImpl::PrimitiveNetworkImpl(NetworkCase& network)
		{
			this->FromNetworkCase(network);
		}

#if 0
		void PrimitiveNetworkImpl::GenerateAdmittance()
		{
			// �����Ǿ���row < col
			//Ϊ����ϡ�����Ԥ���ռ�
			Admittance.resize(NodeCount, NodeCount);
			vector<int> ColSpace;
			ColSpace.resize(NodeCount);
			//���ڵ���ӳ��Ϊ��Ӧ�ڵ��֧·����
			transform(Nodes.begin(), Nodes.end(), ColSpace.begin(), [](NodeInfo *node){ return node->Degree + 1; });
			Admittance.reserve(ColSpace);
			//���ɵ��ɾ���
			for (auto& c : network.Components())
			{
				//�γɵ��ɾ���
				auto dpc = dynamic_cast<DoublePortComponent*>(c);
				if (dpc != NULL)
				{

				}
				auto sa = dynamic_cast<ShuntAdmittance*>(c);
				if (sa != NULL)
				{
					int bus = BusMapping[sa->Bus1()]->Index;
					Admittance.coeffRef(bus, bus) += sa->Admittance();
				}
			}
			_PS_TRACE("\n���ɾ��� ==========\n" << Admittance);
		}
#endif
	}
}
