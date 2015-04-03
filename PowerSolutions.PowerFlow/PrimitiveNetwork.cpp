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
			//BUG FIXED 使用 m_Nodes 作为遍历删除的对象
			//导致在形成 m_Nodes 前扔出异常时发生内存泄漏。
			for (auto& p : m_BusDict) delete p.second;
			for (auto& item : m_Branches) delete item;
		}

		template <class TNodeQueue, class TBranchQueue>
		void PrimitiveNetwork::LoadSubnetwork(PrimitiveNetwork* source, TNodeQueue& nodes, TBranchQueue& branches)
		{
			assert(source != nullptr);
			//初始化公共属性。
			m_Options = source->m_Options;
			m_SourceNetwork = source->m_SourceNetwork;
			m_SlackNodeAssignment = SlackNodeAssignmentType::SlackGenerator;
			//用于将 source 中的节点映射到 this 中的对应节点。
			unordered_map<NodeInfo*, NodeInfo*> NewNodeDict(nodes.size());
			unordered_map<BranchInfo*, BranchInfo*> NewBranchDict(nodes.size());
			//复制节点对象模型。
			while (!nodes.empty())
			{
				auto oldInst = nodes.top();
				auto newInst = new NodeInfo(*oldInst);
				nodes.pop();
				//在后面的代码中重新进行编号。
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
			//复制边对象模型。
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
				//需要手动分配一个平衡节点。
				AssignSlackNode();
				swap(*find(m_Nodes.begin(), m_Nodes.end(), m_SlackNode), m_Nodes.back());
			}
			//为节点重新编号。
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
			//修正旧的节点引用。
			for (auto& nodeP : NewNodeDict)
			{
				auto& oldNode = nodeP.first;
				auto& newNode = nodeP.second;
				auto m0 = oldNode->Index(), m = newNode->Index();
				Admittance.coeffRef(m, m) = source->Admittance.coeff(m0, m0);
				for (auto& abranch : newNode->AdjacentBranches())
				{
					//注意下面两行顺序不能反。
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
			//LoadNetworkCase 只能调用一次。
			assert(m_SourceNetwork == nullptr);
			_PS_TRACE("Load Network " << this << " From " << network);
			m_SourceNetwork = network;
			m_Options = options;
			m_SlackNodeAssignment = SlackNodeAssignmentType::SlackGenerator;
			//重置局部变量
			//m_Buses.clear();
			//m_BusDict.clear();
			//m_Branches.clear();
			//m_Nodes.clear();
			//m_PQNodes.clear();
			//m_PVNodes.clear();
			//m_SlackNode = nullptr;
			assert(m_SlackNode == nullptr);
			//载入母线
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
				//默认PQ节点
				m_BusDict.emplace(obj, new NodeInfo(obj));
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
					//补充节点信息以及支路连接信息。
					c->BuildNodeInfo(this);
				}
			}
			//注意，此时的统计的PQ节点数量中还包含了孤立的节点
			//从 BusMapping 中移除未被引用的节点。
			assert(m_BusDict.size() == m_Buses.size());
			while (true)
			{
				auto i = find_if(m_BusDict.begin(), m_BusDict.end(),
					[](NodeDictionary::value_type &item){return item.second->Degree() == 0; });
				if (i != m_BusDict.end())
				{
					//BUG FIXED 此处未 delete 造成内存泄漏。
					//BUG FIXED 移除孤立的平衡节点。
					if (i->second->Type() == NodeType::SlackNode)
						m_SlackNode = nullptr;
					delete i->second;
					m_BusDict.erase(i);
				} else {
					break;
				}
			};
			//很不巧，所有的节点都被删完了。
			if (m_BusDict.empty()) return;
			//检查是否存在平衡节点。
			if (m_SlackNode == nullptr) AssignSlackNode();
			//统计PQ/PV节点数量，便于预留空间。
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
			//复制节点列表。
			m_Nodes.resize(PQNodeCount + PVNodeCount + 1);
			assert(m_Nodes.size() == m_BusDict.size());
			transform(m_BusDict.cbegin(), m_BusDict.cend(), m_Nodes.begin(),
				[](const NodeDictionary::value_type &item){return item.second; });
			if ((m_Options & PrimitiveNetworkOptions::NodeReorder) == PrimitiveNetworkOptions::NodeReorder)
			{
				//采用静态节点优化编号,即将节点的出线数从小到大依次排列
				//对Nodes列表进行排序。
				sort(m_Nodes.begin(), m_Nodes.end(),
					[](const NodeCollection::value_type &x, const NodeCollection::value_type &y)
				{
					//将平衡节点放到列表的末尾
					if (x->Type() == NodeType::SlackNode) return false;
					if (y->Type() == NodeType::SlackNode) return true;
					return x->Degree() < y->Degree();
				});
			} else {
				//不论如何，平衡节点应该在 Nodes 集合的最后面。
				//此处考虑到性能，仅仅交换平衡节点和除平衡节点以外最后一个节点的位置。
				swap(*find_if(m_Nodes.begin(), m_Nodes.end(), [](NodeInfo* node){return node->Type() == NodeType::SlackNode; }),
					m_Nodes.back());
			}
			//按照新的顺序重新编号
			int IndexCounter1 = 0, IndexCounter2 = 0;
			m_PQNodes.reserve(PQNodeCount);
			m_PVNodes.reserve(PVNodeCount);
			//CASE 如果没有PV节点，会导致异常
			for (auto node : m_Nodes)
			{
				assert(node->Degree() > 0);
				//为节点编号。
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
			//平衡节点编号放在最后面。
			m_SlackNode->Index(IndexCounter1 + IndexCounter2);
			m_SlackNode->SubIndex(0);

			_PS_TRACE("Bus\tIndex\tNodeType");
			for (auto& node : m_Nodes)
			{
				_PS_TRACE(node->Bus()->_ID << "\t" << node->Index() << "\t" << (int)node->Type());
			}

			//生成导纳矩阵。
			Admittance.resize(m_Nodes.size(), m_Nodes.size());
			if ((m_Options & PrimitiveNetworkOptions::NoAdmittanceMatrix) != PrimitiveNetworkOptions::NoAdmittanceMatrix)
			{
				//为导纳稀疏矩阵预留空间
				vector<int> ColSpace;
				ColSpace.resize(m_Nodes.size());
				//将节点表格映射为对应节点的支路数量
				transform(m_Nodes.begin(), m_Nodes.end(), ColSpace.begin(),
					[](NodeInfo *node){ return node->Degree() * 2 + 1; });
				Admittance.reserve(ColSpace);
				//生成导纳矩阵
				_PS_TRACE("Bus1 -- Bus2\tY12\tY1\tY2");
				for (auto& obj : network->Objects())
				{
					auto* c = dynamic_cast<Component*>(obj);
					if (c != nullptr)
						c->BuildAdmittanceInfo(this);
				}
				Admittance.makeCompressed();
				_PS_TRACE("\n导纳矩阵 ==========\n" << Admittance);
			}
		}

		void PrimitiveNetwork::AddPi(Bus* pbus1, Bus* pbus2, PiEquivalencyParameters pieqv)
		{
			//具有π形等值电路
			int index1 = Nodes(pbus1)->Index();
			int index2 = Nodes(pbus2)->Index();
			//BUG CLOSED
			//当系数矩阵非零元素数量增加时，
			//可能会导致预先获取的零元素对应地址被覆盖，
			//因此不能提前使用引用变量保存位置。
			//如果此时重新使用 coeffRef 即可得到正确的结果。
			complexd transAdmittance = 1.0 / pieqv.Impedance();
			_PS_TRACE(index1 << " -- " << index2 << "\t" << transAdmittance << "\t" << pieqv.Admittance1() << "\t" << pieqv.Admittance2());
			//自导纳是正的
			if ((m_Options & PrimitiveNetworkOptions::IgnoreShuntAdmittance) != PrimitiveNetworkOptions::IgnoreShuntAdmittance)
			{
				//计入接地导纳。
				Admittance.coeffRef(index1, index1) += transAdmittance + pieqv.Admittance1();
				Admittance.coeffRef(index2, index2) += transAdmittance + pieqv.Admittance2();
			} else {
				//不计接地导纳。
				Admittance.coeffRef(index1, index1) += transAdmittance;
				Admittance.coeffRef(index2, index2) += transAdmittance;
			}
			//互导纳是负的。
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
				//如果 node == nullptr，说明节点是孤立的，已经被优化掉了。
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
				//false 表示这种情况是不可以的，需要引发异常。
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
					//设置平衡机
					if (node->Type() != NodeType::PQNode && abs(node->Voltage() - abs(voltagePhasor)) > 1e-10)
						throw Exception(ExceptionCode::VoltageMismatch);
					node->Voltage(abs(voltagePhasor));
					node->Angle(arg(voltagePhasor));
					node->Type(NodeType::SlackNode);
					m_SlackNode = node;
				} else if (m_SlackNode == node)
				{
					//在同一个母线上放置了多台平衡发电机
					//检查平衡机电压
					if (abs(node->Voltage() - abs(voltagePhasor)) > 1e-10)
						throw Exception(ExceptionCode::VoltageMismatch);
				} else {
					//存在多于一台平衡发电机
					throw Exception(ExceptionCode::SlackBus);
				}
			}
		}

		void PrimitiveNetwork::ClaimBranch(Bus* bus1, Bus* bus2, Component* c)
		{
			assert(bus1 != bus2);	//不允许自环。
			//加入支路-组件列表中
			auto node1 = Nodes(bus1);
			auto node2 = Nodes(bus2);
			auto result = m_BranchDict.emplace(make_pair(node1, node2), nullptr);
			if (result.second)
			{
				//成功向支路列表中加入了新项，说明出现了新支路。
				auto newBranch = new BranchInfo(m_Branches.size(), node1, node2);
				m_Branches.push_back(newBranch);
				result.first->second = newBranch;
				node1->AdjacentBranches().push_back(newBranch);
				node2->AdjacentBranches().push_back(newBranch);
			}
			//为支路增加一个元件。
			result.first->second->Components().push_back(c);
		}
		
		void PrimitiveNetwork::ClaimParent(Bus* bus, Component* c)
		{
			//加入母线的元件列表中。
			auto node = TryGetNode(bus);
			//如果 node == nullptr，说明节点是孤立的，已经被优化掉了。
			if (node != nullptr)
				node->Components().push_back(c);
		}

		vector<shared_ptr<PrimitiveNetwork>> PrimitiveNetwork::ConnectedSubnetworks()
		{
			_PS_TRACE("=== BFS ConnectedSubnetworks ===");
			vector<shared_ptr<PrimitiveNetwork>> rv;
			// 进行广度优先搜索（BFS）。
			vector<bool> NodeDiscovered(m_Nodes.size());	//黑色节点
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
				if (i == NodeDiscovered.end()) break;	//所有连通子图已经遍历完毕。
				assert(NodeQueue.empty() && SubNodes.empty() && SubBranches.empty());
				auto startingIndex = distance(NodeDiscovered.begin(), i);
				NodeQueue.push(m_Nodes[startingIndex]);
				NodeDiscovered[startingIndex] = true;
				while (!NodeQueue.empty())
				{
					auto node0 = NodeQueue.front();
					NodeQueue.pop();
					//记录灰色节点。
					_PS_TRACE("node " << node0->Bus()->_ID);
					//将刚刚发现的灰色节点加入列表。
					SubNodes.push(node0);
					for (auto& ab : node0->AdjacentBranches())
					{
						auto node = ab->AnotherNode(node0);
						if (node0 > node)
						{
							//如果不存在 node0 > node 的判断
							//则每条边恰好被遍历两次。
							_PS_TRACE("branch " << node0->Bus()->_ID << "\t" << node->Bus()->_ID);
							SubBranches.push(m_BranchDict.at(NodePair(node0, node)));
						}
						if (!NodeDiscovered[node->Index()])
						{
							//将节点染灰。
							NodeDiscovered[node->Index()] = true;
							//接入待探索列表。
							NodeQueue.push(node);
						}
					}
					//将当前节点标记为已访问。（染黑）
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
			//仅在系统中无显式指定平衡节点时提供平衡节点的转换逻辑。
			if ((m_Options & PrimitiveNetworkOptions::AutoAssignSlackNode) != PrimitiveNetworkOptions::AutoAssignSlackNode)
				throw Exception(ExceptionCode::SlackBus);
				//需要手动分配一个平衡节点。
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
				//没有平衡节点和PV节点。
				if ((m_Options & PrimitiveNetworkOptions::ForceSetSlackNode) != PrimitiveNetworkOptions::ForceSetSlackNode)
					throw Exception(ExceptionCode::SlackBus);
				//只好使用有功出力最大/有功负荷最小的PQ节点。
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
