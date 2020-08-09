#include "stdafx.h"
#include "PrimitiveNetwork.h"
#include "Exceptions.h"
#include "NetworkCase.h"
#include "Utility.h"
#include <queue>

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
			for (auto& item : m_Nodes) delete item;
			for (auto& item : m_Branches) delete item;
		}

		template <class TNodeQueue, class TBranchQueue>
		PrimitiveNetwork::PrimitiveNetwork(PrimitiveNetwork* source, TNodeQueue& nodes, TBranchQueue& branches)
			: m_IgnoreShuntAdmittance(source->m_IgnoreShuntAdmittance),
			m_NodeReorder(source->m_NodeReorder), m_SourceNetwork(source->m_SourceNetwork)
		{
			assert(source != nullptr);
			//��ʼ���������ԡ�
			int NodeCounter1 = 0, NodeCounter2 = 0;
			//���ڽ� source �еĽڵ�ӳ�䵽 this �еĶ�Ӧ�ڵ㡣
			unordered_map<NodeInfo*, NodeInfo*> NewNodeDict(nodes.size());
			vector<int> AdmittanceColSpace;
			AdmittanceColSpace.reserve(nodes.size());
			while (!nodes.empty())
			{
				auto oldInst = nodes.top();
				auto newInst = new NodeInfo(*oldInst);
				nodes.pop();
				m_Nodes.push_back(newInst);
				m_BusDict.emplace(newInst->Bus(), newInst);
				NewNodeDict.emplace(oldInst, newInst);
				AdmittanceColSpace.push_back(newInst->AdjacentNodes().size() * 2 + 1);
				newInst->Index(NodeCounter1 + NodeCounter2);
				switch (newInst->Type())
				{
				case NodeType::PQNode:
					newInst->SubIndex(NodeCounter1);
					NodeCounter1++;
					m_PQNodes.push_back(newInst);
					break;
				case NodeType::PVNode:
					newInst->SubIndex(NodeCounter2);
					NodeCounter2++;
					m_PVNodes.push_back(newInst);
					break;
				case NodeType::SlackNode:
					assert(m_SlackNode == nullptr);
					assert(nodes.empty());
					newInst->SubIndex(m_Nodes.size() - 1);
					m_SlackNode = newInst;
				}
			}
			Admittance.resize(m_Nodes.size(), m_Nodes.size());
			Admittance.reserve(AdmittanceColSpace);
			//�����ɵĽڵ����á�
			for (auto& nodeP : NewNodeDict)
			{
				auto& oldNode = nodeP.first;
				auto& newNode = nodeP.second;
				auto m0 = oldNode->Index(), m = newNode->Index();
				Admittance.coeffRef(m, m) = source->Admittance.coeffRef(m0, m0);
				for (auto& anode : newNode->AdjacentNodes())
				{
					//ע����������˳���ܷ���
					auto n0 = anode->Index();
					anode = NewNodeDict.at(anode);
					auto n = anode->Index();
					Admittance.coeffRef(m, n) = source->Admittance.coeffRef(m0, n0);
					//Admittance.coeffRef(n, m) = source->Admittance.coeffRef(n0, m0);
				}
			}
			if (m_SlackNode == nullptr)
			{
				//��Ҫ�ֶ�����һ��ƽ��ڵ㡣
			}
			while (!branches.empty())
			{
				BranchInfo* oldInst = branches.top();
				auto newInst = new BranchInfo(m_Branches.size(), 
					NewNodeDict.at(oldInst->Nodes().first), 
					NewNodeDict.at(oldInst->Nodes().second));
				branches.pop();
				m_Branches.push_back(newInst);
			}
		}

		void PrimitiveNetwork::LoadNetworkCase(ObjectModel::NetworkCase* network)
		{
			//LoadNetworkCase ֻ�ܵ���һ�Ρ�
			assert(m_SourceNetwork == nullptr);
			m_SourceNetwork = network;
			//���þֲ�����
			//m_Buses.clear();
			//m_BusDict.clear();
			//m_Branches.clear();
			//m_Nodes.clear();
			//m_PQNodes.clear();
			//m_PVNodes.clear();
			//m_SlackNode = nullptr;
			//����ĸ��
			for (auto& obj : network->Objects())
			{
				auto b = dynamic_cast<Bus*>(obj);
				if (b != nullptr)
				{
					m_Buses.push_back(b);
					_PS_TRACE("BUS " << b << endl);
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
			m_BusDict.reserve(m_Buses.size());
			for (auto &obj : m_Buses)
			{
				//Ĭ��PQ�ڵ�
				m_BusDict.emplace(obj, new NodeInfo(obj));
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
			assert(m_BusDict.size() == m_Buses.size());
			while (true)
			{
				auto i = find_if(m_BusDict.begin(), m_BusDict.end(),
					[](NodeDictionary::value_type &item){return item.second->Degree() == 0; });
				if (i != m_BusDict.end())
					m_BusDict.erase(i);
				else
					break;
			};
			//����Ƿ����ƽ��ڵ㡣
			if (m_SlackNode == nullptr) throw Exception(ExceptionCode::SlackBus);
			//ͳ��PQ/PV�ڵ�����������Ԥ���ռ䡣
			size_t PQNodeCount = 0, PVNodeCount = 0;
			for (auto& p : m_BusDict)
			{
				switch (p.second->Type())
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
			transform(m_BusDict.cbegin(), m_BusDict.cend(), m_Nodes.begin(),
				[](const NodeDictionary::value_type &item){return item.second; });
			//���þ�̬�ڵ��Ż����,�����ڵ�ĳ�������С������������
			//��Nodes�б��������
			if (m_NodeReorder)
			{
				sort(m_Nodes.begin(), m_Nodes.end(),
					[](const NodeCollection::value_type &x, const NodeCollection::value_type &y)
				{
					//��ƽ��ڵ�ŵ��б��ĩβ
					if (x->Type() == NodeType::SlackNode) return false;
					if (y->Type() == NodeType::SlackNode) return true;
					return x->Degree() < y->Degree();
				});
			} else {
				//������Σ�ƽ��ڵ�Ӧ���� Nodes ���ϵ�����档
				//�˴����ǵ����ܣ���������ƽ��ڵ�ͳ�ƽ��ڵ��������һ���ڵ��λ�á�
				swap(*find_if(m_Nodes.begin(), m_Nodes.end(), [](NodeInfo* node){return node->Type() == NodeType::SlackNode; }),
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
				node->Index(IndexCounter1 + IndexCounter2);
				if (node->Type() == NodeType::PQNode)
				{
					node->SubIndex(IndexCounter1);
					IndexCounter1++;
					m_PQNodes.push_back(node);
				} else if (node->Type() == NodeType::PVNode)
				{
					node->SubIndex(IndexCounter2);
					IndexCounter2++;
					m_PVNodes.push_back(node);
				}
			}
			//ƽ��ڵ��ŷ�������档
			m_SlackNode->Index(IndexCounter1 + IndexCounter2);
			m_SlackNode->SubIndex(0);

			_PS_TRACE("Bus\tIndex\tNodeType");
			for (auto& node : m_Nodes)
			{
				_PS_TRACE(node->Bus() << "\t" << node->Index() << "\t" << (int)node->Type() << endl);
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
			_PS_TRACE("Bus1 -- Bus2\tY12\tY1\tY2");
			for (auto& obj : network->Objects())
			{
				auto* c = dynamic_cast<Component*>(obj);
				if (c != nullptr)
					c->BuildAdmittanceInfo(this);
			}
			Admittance.makeCompressed();
			_PS_TRACE("\n���ɾ��� ==========\n" << Admittance);
		}

		void PrimitiveNetwork::AddPi(Bus* pbus1, Bus* pbus2, PiEquivalencyParameters pieqv)
		{
			//���Ц��ε�ֵ��·
			int bus1 = Nodes(pbus1)->Index();
			int bus2 = Nodes(pbus2)->Index();
			//BUG CLOSED
			//��ϵ���������Ԫ����������ʱ��
			//���ܻᵼ��Ԥ�Ȼ�ȡ����Ԫ�ض�Ӧ��ַ�����ǣ�
			//��˲�����ǰʹ�����ñ�������λ�á�
			//�����ʱ����ʹ�� coeffRef ���ɵõ���ȷ�Ľ����
			complexd transAdmittance = 1.0 / pieqv.Impedance();
			_PS_TRACE(bus1 << " -- " << bus2 << "\t" << transAdmittance << "\t" << pieqv.Admittance1() << "\t" << pieqv.Admittance2());
			//�Ե���������
			if (!m_IgnoreShuntAdmittance)
			{
				//����ӵص��ɡ�
				Admittance.coeffRef(bus1, bus1) += transAdmittance + pieqv.Admittance1();
				Admittance.coeffRef(bus2, bus2) += transAdmittance + pieqv.Admittance2();
			} else {
				//���ƽӵص��ɡ�
				Admittance.coeffRef(bus1, bus1) += transAdmittance;
				Admittance.coeffRef(bus2, bus2) += transAdmittance;
			}
			//�������Ǹ��ġ�
			Admittance.coeffRef(bus1, bus2) -= transAdmittance;
			Admittance.coeffRef(bus2, bus1) -= transAdmittance;
			//TraceFile << pieqv->PiAdmittance1() << " /- " << transAdmittance << " -\\ " << pieqv->PiAdmittance2() << endl;
			assert(!isnan(Admittance.coeffRef(bus1, bus1).imag()));
			assert(!isnan(Admittance.coeffRef(bus2, bus2).imag()));
			assert(!isnan(Admittance.coeffRef(bus1, bus2).imag()));
			assert(!isnan(Admittance.coeffRef(bus2, bus1).imag()));
		}

		void PrimitiveNetwork::AddShunt(Bus* bus, complexd admittance)
		{
			if (!m_IgnoreShuntAdmittance)
			{
				auto index = Nodes(bus)->Index();
				Admittance.coeffRef(index, index) += admittance;
			}
		}

		void PrimitiveNetwork::AddPQ(Bus* bus, complexd power)
		{
			auto info = Nodes(bus);
			info->AddActivePowerInjection(power.real());
			info->AddReactivePowerInjection(power.imag());
		}

		void PrimitiveNetwork::AddPV(Bus* bus, double activePower, double voltage)
		{
			auto info = Nodes(bus);
			//false ��ʾ��������ǲ����Եģ���Ҫ�����쳣��
			if (info->Type() != NodeType::PQNode && abs(info->Voltage() - voltage) > 1e-10)
				throw Exception(ExceptionCode::VoltageMismatch);
			if (info->Type() != NodeType::SlackNode)
			{
				info->Voltage(voltage);
				info->AddActivePowerInjection(activePower);
				info->Type(NodeType::PVNode);
			}
		}

		void PrimitiveNetwork::AddSlack(Bus* bus, complexd voltagePhasor)
		{
			auto info = Nodes(bus);
			if (m_SlackNode == nullptr)
			{
				//����ƽ���
				if (info->Type() != NodeType::PQNode && abs(info->Voltage() - abs(voltagePhasor)) > 1e-10)
					throw Exception(ExceptionCode::VoltageMismatch);
				info->Voltage(abs(voltagePhasor));
				info->Angle(arg(voltagePhasor));
				info->Type(NodeType::SlackNode);
				m_SlackNode = info;
			} else if (m_SlackNode == info)
			{
				//��ͬһ��ĸ���Ϸ����˶�̨ƽ�ⷢ���
				//���ƽ�����ѹ
				if (abs(info->Voltage() - abs(voltagePhasor)) > 1e-10)
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
			auto node1 = m_BusDict.at(bus1);
			auto node2 = m_BusDict.at(bus2);
			auto result = m_BranchDict.emplace(make_pair(node1, node2), nullptr);
			if (result.second)
			{
				//�ɹ���֧·�б��м��������˵����������֧·��
				auto newBranch = new BranchInfo(m_Branches.size(), node1, node2);
				m_Branches.push_back(newBranch);
				result.first->second = newBranch;
				Nodes(bus1)->AdjacentNodes().push_back(node2);
				Nodes(bus2)->AdjacentNodes().push_back(node1);
			}
		}

		void PrimitiveNetwork::ClaimParent(Bus* bus, Component* c)
		{
			//����ĸ�ߵ�Ԫ���б��С�
			Nodes(bus)->Components().push_back(c);
		}

		vector<shared_ptr<PrimitiveNetwork>> PrimitiveNetwork::ConnectedSubsets()
{
			vector<shared_ptr<PrimitiveNetwork>> rv;
			// ���й������������BFS����
			vector<bool> NodeVisited(m_Nodes.size());	//��ɫ�ڵ�
			queue<NodeInfo*> NodeQueue;
			struct NodeInfoComparer {
				bool operator() (NodeInfo* x, NodeInfo* y) const
				{
					return x->Index() > y->Index();
				}
			};
			struct BranchInfoComparer {
				bool operator() (BranchInfo* x, BranchInfo* y) const
				{
					return x->Index() > y->Index();
				}
			};
			priority_queue<NodeInfo*, vector<NodeInfo*>, NodeInfoComparer> SubNodes;
			priority_queue<BranchInfo*, vector<BranchInfo*>, BranchInfoComparer> SubBranches;
			while (true)
			{
				auto i = find(NodeVisited.begin(), NodeVisited.end(), false);
				if (i == NodeVisited.end()) break;	//������ͨ��ͼ�Ѿ�������ϡ�
				assert(NodeQueue.empty() && SubNodes.empty() && SubBranches.empty());
				auto startingIndex = distance(NodeVisited.begin(), i);
				NodeQueue.push(m_Nodes[startingIndex]);
				SubNodes.push(NodeQueue.back());
				while (!NodeQueue.empty())
				{
					auto node0 = NodeQueue.front();
					NodeQueue.pop();
					//ע�⵽�˴�û�м�¼��ɫ�ڵ㣬
					//��˶�����еġ������Ѿ�ȾΪ��ɫ�Ľڵ���Ҫ�����жϡ�
					_PS_TRACE("node " << node0->Bus());
					if (NodeVisited[node0->Index()]) continue;
					//���ոշ��ֵĻ�ɫ�ڵ�����б�
					SubNodes.push(node0);
					for (auto& node : node0->AdjacentNodes())
					{
						if (!NodeVisited[node->Index()])
						{
							_PS_TRACE("branch " << node0->Bus() << "\t" << node->Bus());
							SubBranches.emplace(m_BranchDict.at(NodePair(node0, node)));
							NodeQueue.push(node);
							//��¼��ǰ�ڵ���ڽӽڵ㡣
							//Ⱦ�ң��˴�δ����
						}
					}
					//����ǰ�ڵ���Ϊ�ѷ��ʡ�
					NodeVisited[node0->Index()] = true;
				}
				_PS_TRACE("emplace");
				rv.emplace_back(new PrimitiveNetwork(this, SubNodes, SubBranches));
			}
			return rv;
		}

		PrimitiveNetwork::NodeInfo::NodeInfo(ObjectModel::Bus* bus) 
			: m_Bus(bus), m_Type(NodeType::PQNode),
			m_Voltage(0), m_Angle(0),
			m_ActivePowerInjection(0), m_ReactivePowerInjection(0)
		{ }
	}
}
