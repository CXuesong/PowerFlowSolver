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
			//BUG FIXED ʹ�� _Nodes ��Ϊ����ɾ���Ķ���
			//�������γ� _Nodes ǰ�ӳ��쳣ʱ�����ڴ�й©��
			for (auto& p : _BusDict) delete p.second;
			for (auto& item : _Branches) delete item;
		}

		template <class TNodeQueue, class TBranchQueue>
		void PrimitiveNetwork::LoadSubnetwork(const PrimitiveNetwork* source, TNodeQueue& nodes, TBranchQueue& branches)
		{
			assert(source != nullptr);
			//��ʼ���������ԡ�
			_Options = source->_Options;
			_SourceNetwork = source->_SourceNetwork;
			_SlackNodeAssignment = SlackNodeAssignmentType::SlackGenerator;
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
				_Nodes.push_back(newInst);
				_BusDict.emplace(newInst->Bus(), newInst);
				NewNodeDict.emplace(oldInst, newInst);
				if (newInst->Type() == NodeType::SlackNode)
				{
					assert(_SlackNode == nullptr);
					assert(nodes.empty());
					_SlackNode = newInst;
				}
			}
			//���Ʊ߶���ģ�͡�
			while (!branches.empty())
			{
				BranchInfo* oldInst = branches.top();
				auto newInst = new BranchInfo(*oldInst);
				newInst->Index(_Branches.size());
				newInst->Nodes(make_pair(NewNodeDict.at(oldInst->Nodes().first),
					NewNodeDict.at(oldInst->Nodes().second)));
				_Branches.push_back(newInst);
				NewBranchDict.emplace(oldInst, newInst);
				branches.pop();
			}
			if (_SlackNode == nullptr)
			{
				//��Ҫ�ֶ�����һ��ƽ��ڵ㡣
				AssignSlackNode();
				swap(*find(_Nodes.begin(), _Nodes.end(), _SlackNode), _Nodes.back());
			}
			//Ϊ�ڵ����±�š�
			int NodeCounter1 = 0, NodeCounter2 = 0;
			vector<int> AdmittanceColSpace;
			AdmittanceColSpace.reserve(_Nodes.size());
			for (auto& node : _Nodes)
			{
				node->Index(NodeCounter1 + NodeCounter2);
				switch (node->Type())
				{
				case NodeType::PQNode:
					node->SubIndex(NodeCounter1);
					NodeCounter1++;
					_PQNodes.push_back(node);
					break;
				case NodeType::PVNode:
					node->SubIndex(NodeCounter2);
					NodeCounter2++;
					_PVNodes.push_back(node);
					break;
				}
				AdmittanceColSpace.push_back(node->AdjacentBranches().size() * 2 + 1);
			}
			_SlackNode->SubIndex(_Nodes.size() - 1);
			Admittance.resize(_Nodes.size(), _Nodes.size());
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
			assert(_SourceNetwork == nullptr);
			_PS_TRACE("Load Network " << this << " From " << network);
			_SourceNetwork = network;
			_Options = options;
			_SlackNodeAssignment = SlackNodeAssignmentType::SlackGenerator;
			//���þֲ�����
			//_Buses.clear();
			//_BusDict.clear();
			//_Branches.clear();
			//_Nodes.clear();
			//_PQNodes.clear();
			//_PVNodes.clear();
			//_SlackNode = nullptr;
			assert(_SlackNode == nullptr);
			//����ĸ��
			for (auto& obj : network->Objects())
			{
				auto b = dynamic_cast<Bus*>(obj);
				if (b != nullptr)
				{
					_Buses.push_back(b);
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
						_Buses.push_back(b);
					}
				}
			}
			_BusDict.reserve(_Buses.size());
			for (auto &obj : _Buses)
			{
				//Ĭ��PQ�ڵ�
				_BusDict.emplace(obj, new NodeInfo(obj));
			}
			//������еĽڵ�������ӣ���֧·����Ϊ n(n-1)/2
			//�˴�����ÿ��ĸ���Ͼ���6�ؽ���
			_Branches.reserve(_Buses.size() * 3);
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
			assert(_BusDict.size() == _Buses.size());
			while (true)
			{
				auto i = find_if(_BusDict.begin(), _BusDict.end(),
					[](NodeDictionary::value_type &item){return item.second->Degree() == 0; });
				if (i != _BusDict.end())
				{
					//BUG FIXED �˴�δ delete ����ڴ�й©��
					//BUG FIXED �Ƴ�������ƽ��ڵ㡣
					if (i->second->Type() == NodeType::SlackNode)
						_SlackNode = nullptr;
					delete i->second;
					_BusDict.erase(i);
				} else {
					break;
				}
			};
			//�ܲ��ɣ����еĽڵ㶼��ɾ���ˡ�
			if (_BusDict.empty()) return;
			//����Ƿ����ƽ��ڵ㡣
			if (_SlackNode == nullptr) AssignSlackNode();
			//ͳ��PQ/PV�ڵ�����������Ԥ���ռ䡣
			size_t PQNodeCount = 0, PVNodeCount = 0;
			for (auto& p : _BusDict)
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
			_Nodes.resize(PQNodeCount + PVNodeCount + 1);
			assert(_Nodes.size() == _BusDict.size());
			transform(_BusDict.cbegin(), _BusDict.cend(), _Nodes.begin(),
				[](const NodeDictionary::value_type &item){return item.second; });
			if ((_Options & PrimitiveNetworkOptions::NodeReorder) == PrimitiveNetworkOptions::NodeReorder)
			{
				_PS_TRACE("Node Reorder = True");
				//���þ�̬�ڵ��Ż����,�����ڵ�ĳ�������С������������
				//��Nodes�б��������
				sort(_Nodes.begin(), _Nodes.end(),
					[](const NodeCollection::value_type &x, const NodeCollection::value_type &y)
				{
					//��ƽ��ڵ�ŵ��б��ĩβ
					if (x->Type() == NodeType::SlackNode) return false;
					if (y->Type() == NodeType::SlackNode) return true;
					return x->Degree() < y->Degree();
				});
			} else {
#if _DEBUG
				//�ڵ���ģʽ�£����԰��սڵ�� Id Ϊ�ڵ�����
				sort(_Nodes.begin(), _Nodes.end(),
					[](const NodeCollection::value_type &x, const NodeCollection::value_type &y)
				{
					//��ƽ��ڵ�ŵ��б��ĩβ
					if (x->Type() == NodeType::SlackNode) return false;
					if (y->Type() == NodeType::SlackNode) return true;
					return x->Bus()->_ID < y->Bus()->_ID;
				});
#else
				//������Σ�ƽ��ڵ�Ӧ���� Nodes ���ϵ�����档
				//�˴����ǵ����ܣ���������ƽ��ڵ�ͳ�ƽ��ڵ��������һ���ڵ��λ�á�
				swap(*find_if(_Nodes.begin(), _Nodes.end(), [](NodeInfo* node){return node->Type() == NodeType::SlackNode; }),
					_Nodes.back());
#endif
			}
			//�����µ�˳�����±��
			int IndexCounter1 = 0, IndexCounter2 = 0;
			_PQNodes.reserve(PQNodeCount);
			_PVNodes.reserve(PVNodeCount);
			//CASE ���û��PV�ڵ㣬�ᵼ���쳣
			for (auto node : _Nodes)
			{
				assert(node->Degree() > 0);
				//Ϊ�ڵ��š�
				node->Index(IndexCounter1 + IndexCounter2);
				if (node->Type() == NodeType::PQNode)
				{
					node->SubIndex(IndexCounter1);
					IndexCounter1++;
					_PQNodes.push_back(node);
				} else if (node->Type() == NodeType::PVNode)
				{
					node->SubIndex(IndexCounter2);
					IndexCounter2++;
					_PVNodes.push_back(node);
				}
			}
			//ƽ��ڵ��ŷ�������档
			_SlackNode->Index(IndexCounter1 + IndexCounter2);
			_SlackNode->SubIndex(0);

			_PS_TRACE("Bus\tIndex\tNodeType");
			for (auto& node : _Nodes)
			{
				_PS_TRACE(node->Bus()->_ID << "\t" << node->Index() << "\t" << (int)node->Type());
			}

			//���ɵ��ɾ���
			Admittance.resize(_Nodes.size(), _Nodes.size());
			if ((_Options & PrimitiveNetworkOptions::NoAdmittanceMatrix) != PrimitiveNetworkOptions::NoAdmittanceMatrix)
			{
				//Ϊ����ϡ�����Ԥ���ռ�
				vector<int> ColSpace;
				ColSpace.resize(_Nodes.size());
				//���ڵ���ӳ��Ϊ��Ӧ�ڵ��֧·����
				transform(_Nodes.begin(), _Nodes.end(), ColSpace.begin(),
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
			int index1 = Nodes(pbus1).Index();
			int index2 = Nodes(pbus2).Index();
			//BUG CLOSED
			//��ϵ���������Ԫ����������ʱ��
			//���ܻᵼ��Ԥ�Ȼ�ȡ����Ԫ�ض�Ӧ��ַ�����ǣ�
			//��˲�����ǰʹ�����ñ�������λ�á�
			//�����ʱ����ʹ�� coeffRef ���ɵõ���ȷ�Ľ����
			complexd transAdmittance = 1.0 / pieqv.Impedance();
			_PS_TRACE(index1 << " -- " << index2 << "\t" << transAdmittance << "\t" << pieqv.Admittance1() << "\t" << pieqv.Admittance2());
			//�Ե���������
			if ((_Options & PrimitiveNetworkOptions::IgnoreShuntAdmittance) != PrimitiveNetworkOptions::IgnoreShuntAdmittance)
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
			if (((_Options & PrimitiveNetworkOptions::IgnoreShuntAdmittance) != PrimitiveNetworkOptions::IgnoreShuntAdmittance))
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
				if (_SlackNode == nullptr)
				{
					//����ƽ���
					if (node->Type() != NodeType::PQNode && abs(node->Voltage() - abs(voltagePhasor)) > 1e-10)
						throw Exception(ExceptionCode::VoltageMismatch);
					node->Voltage(abs(voltagePhasor));
					node->Angle(arg(voltagePhasor));
					node->Type(NodeType::SlackNode);
					_SlackNode = node;
				} else if (_SlackNode == node)
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
			auto& node1 = Nodes(bus1);
			auto& node2 = Nodes(bus2);
			auto result = _BranchDict.emplace(make_pair(&node1, &node2), nullptr);
			if (result.second)
			{
				//�ɹ���֧·�б��м��������˵����������֧·��
				auto newBranch = new BranchInfo(_Branches.size(), &node1, &node2);
				_Branches.push_back(newBranch);
				result.first->second = newBranch;
				node1.AdjacentBranches().push_back(newBranch);
				node2.AdjacentBranches().push_back(newBranch);
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

		vector<shared_ptr<PrimitiveNetwork>> PrimitiveNetwork::ConnectedSubnetworks() const
		{
			_PS_TRACE("=== BFS ConnectedSubnetworks ===");
			vector<shared_ptr<PrimitiveNetwork>> rv;
			// ���й������������BFS����
			vector<bool> NodeDiscovered(_Nodes.size());	//��ɫ�ڵ�
			vector<bool> BranchVisited(_Branches.size());
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
				NodeQueue.push(_Nodes[startingIndex]);
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
							SubBranches.push(_BranchDict.at(NodePair(node0, node)));
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
			if ((_Options & PrimitiveNetworkOptions::AutoAssignSlackNode) != PrimitiveNetworkOptions::AutoAssignSlackNode)
				throw Exception(ExceptionCode::SlackBus);
				//��Ҫ�ֶ�����һ��ƽ��ڵ㡣
			const double NINF = -DBL_MAX * DBL_MAX;
			double MaxPVActivePower = 0, MaxPQActivePower = NINF;
			NodeInfo *MaxPV = nullptr, *MaxPQ = nullptr;
			for (auto& p : _BusDict)
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
			_SlackNode = MaxPV;
			_SlackNodeAssignment = SlackNodeAssignmentType::PVNode;
			if (_SlackNode == nullptr)
			{
				//û��ƽ��ڵ��PV�ڵ㡣
				if ((_Options & PrimitiveNetworkOptions::ForceSetSlackNode) != PrimitiveNetworkOptions::ForceSetSlackNode)
					throw Exception(ExceptionCode::SlackBus);
				//ֻ��ʹ���й��������/�й�������С��PQ�ڵ㡣
				_SlackNode = MaxPQ;
				_SlackNodeAssignment = SlackNodeAssignmentType::PQNode;
				if (_SlackNode == nullptr) throw Exception(ExceptionCode::SlackBus);
			}
			_SlackNode->Type(NodeType::SlackNode);
		}

		void PrimitiveNetwork::DumpGraph() const
		{
#if _DEBUG
			ofstream ofs("D:\\PNGraph.csv", ios::out | ios::ate);
			ofs << "Source,Target,Type" << endl;
			for (auto& b : _Branches)
			{
				ofs << b->Nodes().first->Index() << "," <<
					b->Nodes().second->Index() << ",Undirected" << endl;
			}
			ofs.close();
#endif
		}

		void PrimitiveNetwork::AdjustReferenceToPrototype(const NetworkCaseCorrespondenceInfo& info, bool strictMode /*= true*/)
		{
			auto PBus = [&](Bus* thisBus) {
				auto nb = dynamic_cast<Bus*>(info.PrototypeOf(thisBus));
				if (nb == nullptr)
					if (strictMode)
						throw Exception(ExceptionCode::Validation);
				return nb;
			};
			auto PComponent = [&](Component* thisComponent) {
				auto nb = dynamic_cast<Component*>(info.PrototypeOf(thisComponent));
				if (nb == nullptr)
					if (strictMode)
						throw Exception(ExceptionCode::Validation);
				return nb;
			};
			for (auto& b : _Buses)
				b = PBus(b);
			_BusDict.clear();
			for (auto& n : _Nodes)
			{
				n->Bus(PBus(n->Bus()));
				for (auto& c : n->Components()) c = PComponent(c);
				_BusDict.emplace(n->Bus(), n);
			}
			for (auto& b : _Branches)
			{
				for (auto& c : b->Components())
					c = PComponent(c);
			}
		}

		PrimitiveNetwork::NodeInfo::NodeInfo(ObjectModel::Bus* bus) 
			: _Bus(bus), _Type(NodeType::PQNode),
			_Voltage(0), _Angle(0),
			_ActivePowerInjection(0), _ReactivePowerInjection(0)
		{ }
	}
}
