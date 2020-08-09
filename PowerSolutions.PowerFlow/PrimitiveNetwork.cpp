#include "stdafx.h"
#include "PrimitiveNetwork.h"
#include "Exceptions.h"
#include "NetworkCase.h"
#include "Utility.h"
#include <queue>
#include "Export\PowerSolutions\PowerFlowObjectModel.h"

using namespace std;
using namespace PowerSolutions::Utility;

namespace PowerSolutions {
	namespace ObjectModel {

		PrimitiveNetwork::PrimitiveNetwork()
		{
			_PS_TRACE("PN Construct " << this);
		}

		PrimitiveNetwork::~PrimitiveNetwork()
		{
			_PS_TRACE("PN Dispose " << this);
			//BUG FIXED ʹ�� m_Nodes ��Ϊ����ɾ���Ķ���
			//�������γ� m_Nodes ǰ�ӳ��쳣ʱ�����ڴ�й©��
			for (auto& p : m_BusDict) delete p.second;
			for (auto& item : m_Branches) delete item;
		}

		template <class TNodeQueue, class TBranchQueue>
		void PrimitiveNetwork::LoadSubnetwork(PrimitiveNetwork* source, TNodeQueue& nodes, TBranchQueue& branches)
		{
			assert(source != nullptr);
			//��ʼ���������ԡ�
			m_Options = source->m_Options;
			m_SourceNetwork = source->m_SourceNetwork;
			m_SlackNodeAssignment = SlackNodeAssignmentType::SlackGenerator;
			//���ڽ� source �еĽڵ�ӳ�䵽 this �еĶ�Ӧ�ڵ㡣
			unordered_map<NodeInfo*, NodeInfo*> NewNodeDict(nodes.size());
			unordered_map<BranchInfo*, BranchInfo*> NewBranchDict(nodes.size());
			//���ƽڵ����ģ�͡�
			while (!nodes.empty())
			{
				auto oldInst = nodes.top();
				auto newInst = new NodeInfo(*oldInst);
				nodes.pop();
				//�ں���Ĵ��������½��б�š�
				m_Nodes.push_back(newInst);
				m_BusDict.emplace(newInst->Bus(), newInst);
				NewNodeDict.emplace(oldInst, newInst);
				if (newInst->Type() == NodeType::SlackNode)
				{
					assert(m_SlackNode == nullptr);
					assert(nodes.empty());
					m_SlackNode = newInst;
				}
			}
			//���Ʊ߶���ģ�͡�
			while (!branches.empty())
			{
				BranchInfo* oldInst = branches.top();
				auto newInst = new BranchInfo(*oldInst);
				newInst->Index(m_Branches.size());
				newInst->Nodes(make_pair(NewNodeDict.at(oldInst->Nodes().first),
					NewNodeDict.at(oldInst->Nodes().second)));
				m_Branches.push_back(newInst);
				NewBranchDict.emplace(oldInst, newInst);
				branches.pop();
			}
			if (m_SlackNode == nullptr)
			{
				//��Ҫ�ֶ�����һ��ƽ��ڵ㡣
				AssignSlackNode();
				swap(*find(m_Nodes.begin(), m_Nodes.end(), m_SlackNode), m_Nodes.back());
			}
			//Ϊ�ڵ����±�š�
			int NodeCounter1 = 0, NodeCounter2 = 0;
			vector<int> AdmittanceColSpace;
			AdmittanceColSpace.reserve(m_Nodes.size());
			for (auto& node : m_Nodes)
			{
				node->Index(NodeCounter1 + NodeCounter2);
				switch (node->Type())
				{
				case NodeType::PQNode:
					node->SubIndex(NodeCounter1);
					NodeCounter1++;
					m_PQNodes.push_back(node);
					break;
				case NodeType::PVNode:
					node->SubIndex(NodeCounter2);
					NodeCounter2++;
					m_PVNodes.push_back(node);
					break;
				}
				AdmittanceColSpace.push_back(node->AdjacentBranches().size() * 2 + 1);
			}
			m_SlackNode->SubIndex(m_Nodes.size() - 1);
			Admittance.resize(m_Nodes.size(), m_Nodes.size());
			Admittance.reserve(AdmittanceColSpace);
			//�����ɵĽڵ����á�
			for (auto& nodeP : NewNodeDict)
			{
				auto& oldNode = nodeP.first;
				auto& newNode = nodeP.second;
				auto m0 = oldNode->Index(), m = newNode->Index();
				Admittance.coeffRef(m, m) = source->Admittance.coeff(m0, m0);
				for (auto& abranch : newNode->AdjacentBranches())
				{
					//ע����������˳���ܷ���
					auto n0 = abranch->AnotherNode(oldNode)->Index();
					abranch = NewBranchDict.at(abranch);
					auto n = abranch->AnotherNode(newNode)->Index();
					Admittance.coeffRef(m, n) = source->Admittance.coeff(m0, n0);
					//Admittance.coeffRef(n, m) = source->Admittance.coeffRef(n0, m0);
				}
			}
		}

		void PrimitiveNetwork::LoadNetworkCase(ObjectModel::NetworkCase* network, PrimitiveNetworkOptions options)
		{
			//LoadNetworkCase ֻ�ܵ���һ�Ρ�
			assert(m_SourceNetwork == nullptr);
			_PS_TRACE("Load Network " << this << " From " << network);
			m_SourceNetwork = network;
			m_Options = options;
			m_SlackNodeAssignment = SlackNodeAssignmentType::SlackGenerator;
			//���þֲ�����
			//m_Buses.clear();
			//m_BusDict.clear();
			//m_Branches.clear();
			//m_Nodes.clear();
			//m_PQNodes.clear();
			//m_PVNodes.clear();
			//m_SlackNode = nullptr;
			assert(m_SlackNode == nullptr);
			//����ĸ��
			for (auto& obj : network->Objects())
			{
				auto b = dynamic_cast<Bus*>(obj);
				if (b != nullptr)
				{
					m_Buses.push_back(b);
					_PS_TRACE("Bus " << b->_ID);
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
					//����ڵ���Ϣ�Լ�֧·������Ϣ��
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
				{
					//BUG FIXED �˴�δ delete ����ڴ�й©��
					//BUG FIXED �Ƴ�������ƽ��ڵ㡣
					if (i->second->Type() == NodeType::SlackNode)
						m_SlackNode = nullptr;
					delete i->second;
					m_BusDict.erase(i);
				} else {
					break;
				}
			};
			//�ܲ��ɣ����еĽڵ㶼��ɾ���ˡ�
			if (m_BusDict.empty()) return;
			//����Ƿ����ƽ��ڵ㡣
			if (m_SlackNode == nullptr) AssignSlackNode();
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
			assert(m_Nodes.size() == m_BusDict.size());
			transform(m_BusDict.cbegin(), m_BusDict.cend(), m_Nodes.begin(),
				[](const NodeDictionary::value_type &item){return item.second; });
			if ((m_Options & PrimitiveNetworkOptions::NodeReorder) == PrimitiveNetworkOptions::NodeReorder)
			{
				//���þ�̬�ڵ��Ż����,�����ڵ�ĳ�������С������������
				//��Nodes�б��������
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
				_PS_TRACE(node->Bus()->_ID << "\t" << node->Index() << "\t" << (int)node->Type());
			}

			//���ɵ��ɾ���
			Admittance.resize(m_Nodes.size(), m_Nodes.size());
			if ((m_Options & PrimitiveNetworkOptions::NoAdmittanceMatrix) != PrimitiveNetworkOptions::NoAdmittanceMatrix)
			{
				//Ϊ����ϡ�����Ԥ���ռ�
				vector<int> ColSpace;
				ColSpace.resize(m_Nodes.size());
				//���ڵ���ӳ��Ϊ��Ӧ�ڵ��֧·����
				transform(m_Nodes.begin(), m_Nodes.end(), ColSpace.begin(),
					[](NodeInfo *node){ return node->Degree() * 2 + 1; });
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
		}

		void PrimitiveNetwork::AddPi(Bus* pbus1, Bus* pbus2, PiEquivalencyParameters pieqv)
		{
			//���Ц��ε�ֵ��·
			int index1 = Nodes(pbus1)->Index();
			int index2 = Nodes(pbus2)->Index();
			//BUG CLOSED
			//��ϵ���������Ԫ����������ʱ��
			//���ܻᵼ��Ԥ�Ȼ�ȡ����Ԫ�ض�Ӧ��ַ�����ǣ�
			//��˲�����ǰʹ�����ñ�������λ�á�
			//�����ʱ����ʹ�� coeffRef ���ɵõ���ȷ�Ľ����
			complexd transAdmittance = 1.0 / pieqv.Impedance();
			_PS_TRACE(index1 << " -- " << index2 << "\t" << transAdmittance << "\t" << pieqv.Admittance1() << "\t" << pieqv.Admittance2());
			//�Ե���������
			if ((m_Options & PrimitiveNetworkOptions::IgnoreShuntAdmittance) != PrimitiveNetworkOptions::IgnoreShuntAdmittance)
			{
				//����ӵص��ɡ�
				Admittance.coeffRef(index1, index1) += transAdmittance + pieqv.Admittance1();
				Admittance.coeffRef(index2, index2) += transAdmittance + pieqv.Admittance2();
			} else {
				//���ƽӵص��ɡ�
				Admittance.coeffRef(index1, index1) += transAdmittance;
				Admittance.coeffRef(index2, index2) += transAdmittance;
			}
			//�������Ǹ��ġ�
			Admittance.coeffRef(index1, index2) -= transAdmittance;
			Admittance.coeffRef(index2, index1) -= transAdmittance;
			//TraceFile << pieqv->PiAdmittance1() << " /- " << transAdmittance << " -\\ " << pieqv->PiAdmittance2() << endl;
			assert(!isnan(Admittance.coeffRef(index1, index1).imag()));
			assert(!isnan(Admittance.coeffRef(index2, index2).imag()));
			assert(!isnan(Admittance.coeffRef(index1, index2).imag()));
			assert(!isnan(Admittance.coeffRef(index2, index1).imag()));
		}

		void PrimitiveNetwork::AddShunt(Bus* bus, complexd admittance)
		{
			if (((m_Options & PrimitiveNetworkOptions::IgnoreShuntAdmittance) != PrimitiveNetworkOptions::IgnoreShuntAdmittance))
			{
				auto node = TryGetNode(bus);
				//��� node == nullptr��˵���ڵ��ǹ����ģ��Ѿ����Ż����ˡ�
				if (node != nullptr)
				{
					auto index = node->Index();
					Admittance.coeffRef(index, index) += admittance;
				}
			}
		}

		void PrimitiveNetwork::AddPQ(Bus* bus, complexd power)
		{
			auto node = TryGetNode(bus);
			if (node != nullptr)
			{
				node->AddActivePowerInjection(power.real());
				node->AddReactivePowerInjection(power.imag());
			}
		}

		void PrimitiveNetwork::AddPV(Bus* bus, double activePower, double voltage)
		{
			auto node = TryGetNode(bus);
			if (node != nullptr)
			{
				//false ��ʾ��������ǲ����Եģ���Ҫ�����쳣��
				if (node->Type() != NodeType::PQNode && abs(node->Voltage() - voltage) > 1e-10)
					throw Exception(ExceptionCode::VoltageMismatch);
				if (node->Type() != NodeType::SlackNode)
				{
					node->Voltage(voltage);
					node->AddActivePowerInjection(activePower);
					node->Type(NodeType::PVNode);
				}
			}
		}

		void PrimitiveNetwork::AddSlack(Bus* bus, complexd voltagePhasor)
		{
			auto node = TryGetNode(bus);
			if (node != nullptr)
			{
				if (m_SlackNode == nullptr)
				{
					//����ƽ���
					if (node->Type() != NodeType::PQNode && abs(node->Voltage() - abs(voltagePhasor)) > 1e-10)
						throw Exception(ExceptionCode::VoltageMismatch);
					node->Voltage(abs(voltagePhasor));
					node->Angle(arg(voltagePhasor));
					node->Type(NodeType::SlackNode);
					m_SlackNode = node;
				} else if (m_SlackNode == node)
				{
					//��ͬһ��ĸ���Ϸ����˶�̨ƽ�ⷢ���
					//���ƽ�����ѹ
					if (abs(node->Voltage() - abs(voltagePhasor)) > 1e-10)
						throw Exception(ExceptionCode::VoltageMismatch);
				} else {
					//���ڶ���һ̨ƽ�ⷢ���
					throw Exception(ExceptionCode::SlackBus);
				}
			}
		}

		void PrimitiveNetwork::ClaimBranch(Bus* bus1, Bus* bus2, Component* c)
		{
			assert(bus1 != bus2);	//�������Ի���
			//����֧·-����б���
			auto node1 = Nodes(bus1);
			auto node2 = Nodes(bus2);
			auto result = m_BranchDict.emplace(make_pair(node1, node2), nullptr);
			if (result.second)
			{
				//�ɹ���֧·�б��м��������˵����������֧·��
				auto newBranch = new BranchInfo(m_Branches.size(), node1, node2);
				m_Branches.push_back(newBranch);
				result.first->second = newBranch;
				node1->AdjacentBranches().push_back(newBranch);
				node2->AdjacentBranches().push_back(newBranch);
			}
			//Ϊ֧·����һ��Ԫ����
			result.first->second->Components().push_back(c);
		}
		
		void PrimitiveNetwork::ClaimParent(Bus* bus, Component* c)
		{
			//����ĸ�ߵ�Ԫ���б��С�
			auto node = TryGetNode(bus);
			//��� node == nullptr��˵���ڵ��ǹ����ģ��Ѿ����Ż����ˡ�
			if (node != nullptr)
				node->Components().push_back(c);
		}

		vector<shared_ptr<PrimitiveNetwork>> PrimitiveNetwork::ConnectedSubnetworks()
		{
			_PS_TRACE("=== BFS ConnectedSubnetworks ===");
			vector<shared_ptr<PrimitiveNetwork>> rv;
			// ���й������������BFS����
			vector<bool> NodeDiscovered(m_Nodes.size());	//��ɫ�ڵ�
			vector<bool> BranchVisited(m_Branches.size());
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
				auto i = find(NodeDiscovered.begin(), NodeDiscovered.end(), false);
				if (i == NodeDiscovered.end()) break;	//������ͨ��ͼ�Ѿ�������ϡ�
				assert(NodeQueue.empty() && SubNodes.empty() && SubBranches.empty());
				auto startingIndex = distance(NodeDiscovered.begin(), i);
				NodeQueue.push(m_Nodes[startingIndex]);
				NodeDiscovered[startingIndex] = true;
				while (!NodeQueue.empty())
				{
					auto node0 = NodeQueue.front();
					NodeQueue.pop();
					//��¼��ɫ�ڵ㡣
					_PS_TRACE("node " << node0->Bus()->_ID);
					//���ոշ��ֵĻ�ɫ�ڵ�����б�
					SubNodes.push(node0);
					for (auto& ab : node0->AdjacentBranches())
					{
						auto node = ab->AnotherNode(node0);
						if (node0 > node)
						{
							//��������� node0 > node ���ж�
							//��ÿ����ǡ�ñ��������Ρ�
							_PS_TRACE("branch " << node0->Bus()->_ID << "\t" << node->Bus()->_ID);
							SubBranches.push(m_BranchDict.at(NodePair(node0, node)));
						}
						if (!NodeDiscovered[node->Index()])
						{
							//���ڵ�Ⱦ�ҡ�
							NodeDiscovered[node->Index()] = true;
							//�����̽���б�
							NodeQueue.push(node);
						}
					}
					//����ǰ�ڵ���Ϊ�ѷ��ʡ���Ⱦ�ڣ�
					//NodeVisited[node0->Index()] = true;
				}
				shared_ptr<PrimitiveNetwork> newPN(new PrimitiveNetwork());
				newPN->LoadSubnetwork(this, SubNodes, SubBranches);
				rv.emplace_back(newPN);
				_PS_TRACE("Emplace to " << rv.back().get());
			}
			return rv;
		}

		void PrimitiveNetwork::AssignSlackNode()
		{
			//����ϵͳ������ʽָ��ƽ��ڵ�ʱ�ṩƽ��ڵ��ת���߼���
			if ((m_Options & PrimitiveNetworkOptions::AutoAssignSlackNode) != PrimitiveNetworkOptions::AutoAssignSlackNode)
				throw Exception(ExceptionCode::SlackBus);
				//��Ҫ�ֶ�����һ��ƽ��ڵ㡣
			const double NINF = -DBL_MAX * DBL_MAX;
			double MaxPVActivePower = 0, MaxPQActivePower = NINF;
			NodeInfo *MaxPV = nullptr, *MaxPQ = nullptr;
			for (auto& p : m_BusDict)
			{
				if (p.second->Type() == NodeType::PVNode)
				{
					if (p.second->ActivePowerInjection() > MaxPVActivePower)
					{
						MaxPVActivePower = p.second->ActivePowerInjection();
						MaxPV = p.second;
					}
				} else if (p.second->Type() == NodeType::PQNode)
				{
					if (p.second->ActivePowerInjection() > MaxPQActivePower)
					{
						MaxPQActivePower = p.second->ActivePowerInjection();
						MaxPQ = p.second;
					}
				}
			}
			m_SlackNode = MaxPV;
			m_SlackNodeAssignment = SlackNodeAssignmentType::PVNode;
			if (m_SlackNode == nullptr)
			{
				//û��ƽ��ڵ��PV�ڵ㡣
				if ((m_Options & PrimitiveNetworkOptions::ForceSetSlackNode) != PrimitiveNetworkOptions::ForceSetSlackNode)
					throw Exception(ExceptionCode::SlackBus);
				//ֻ��ʹ���й��������/�й�������С��PQ�ڵ㡣
				m_SlackNode = MaxPQ;
				m_SlackNodeAssignment = SlackNodeAssignmentType::PQNode;
				if (m_SlackNode == nullptr) throw Exception(ExceptionCode::SlackBus);
			}
			m_SlackNode->Type(NodeType::SlackNode);
		}

		void PrimitiveNetwork::DumpGraph()
		{
#if _DEBUG
			ofstream ofs("D:\\PNGraph.csv", ios::out | ios::ate);
			ofs << "Source,Target,Type" << endl;
			for (auto& b : m_Branches)
			{
				ofs << b->Nodes().first->Index() << "," <<
					b->Nodes().second->Index() << ",Undirected" << endl;
			}
			ofs.close();
#endif
		}

		PrimitiveNetwork::NodeInfo::NodeInfo(ObjectModel::Bus* bus) 
			: m_Bus(bus), m_Type(NodeType::PQNode),
			m_Voltage(0), m_Angle(0),
			m_ActivePowerInjection(0), m_ReactivePowerInjection(0)
		{ }
	}
}
