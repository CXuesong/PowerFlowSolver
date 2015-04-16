#ifndef __POWERSOLUTIONS_POWERFLOW_PRIMITIVENETWORK_H
#define __POWERSOLUTIONS_POWERFLOW_PRIMITIVENETWORK_H

#include "PowerSolutions.h"
#include "ObjectModel.h"
#include "Utility.h"
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <Eigen/Sparse>

namespace PowerSolutions {
	namespace ObjectModel {
		// 节点的类型。
		enum class NodeType : byte
		{
			PQNode = 0,		//PQ节点。
			PVNode,			//PV节点。
			SlackNode,		//平衡节点。
		};
		enum class PrimitiveNetworkOptions : byte
		{
			None = 0,
			NodeReorder = 1,			//进行节点编号优化。
			IgnoreShuntAdmittance = 2,	//忽略接地导纳，用于直流潮流计算。注意此选项不会改变 NodeInfo::Components 的行为。
			NoAdmittanceMatrix = 4,		//不生成导纳矩阵，一般用于纯图论分析。
			//如果网络中不存在平衡节点，则将发电容量最大的PV节点作为平衡节点。
			AutoAssignSlackNode = 8,
			//保证网络中存在平衡节点。
			//如果网络中既不存在平衡节点，也不存在PV节点，则选择一PQ节点作为平衡节点。
			ForceSetSlackNode = AutoAssignSlackNode | 16,
			//仅保留适用于纯图论分析的网络参数（例如节点及其邻接表）。
			PureGraphicalAnalysis = NoAdmittanceMatrix | ForceSetSlackNode,
		};
		//描述了 PrimitiveNetwork 中平衡节点的选取方法。
		enum class SlackNodeAssignmentType : byte
		{
			SlackGenerator = 0,		//通过平衡发电机选取。
			PVNode = 1,				//通过转换有功出力最大的PV节点选取。
			PQNode = 2,				//通过转换PQ节点选取。
		 };
		class PrimitiveNetwork
		{
			friend class NetworkCase;
		public:
			class NodeInfo;
			class BranchInfo;
			typedef std::pair<NodeInfo*, NodeInfo*> NodePair;
			class NodeInfo
			{
			public:
				typedef std::list<Component*> ComponentCollection;
				typedef std::list<BranchInfo*> BranchInfoCollection;
			private:
				ObjectModel::Bus* m_Bus;
				double m_Voltage;
				double m_Angle;
				int m_Index;
				int m_SubIndex;
				NodeType m_Type;
				double m_ActivePowerInjection;
				double m_ReactivePowerInjection;
				ComponentCollection m_Components;
				BranchInfoCollection m_AdjacentBranches;
			public:
				//母线的组件信息。
				ObjectModel::Bus* Bus() const { return m_Bus; }
				void Bus(ObjectModel::Bus* val) { m_Bus = val; }
				//母线所连接的元件。包括单端与多端元件。
				ComponentCollection& Components() { return m_Components; }
				//与此母线邻接的节点。
				BranchInfoCollection& AdjacentBranches() { return m_AdjacentBranches; }
				//节点索引（Nodes）。
				int Index() const { return m_Index; }
				void Index(int val) { m_Index = val; }
				//节点在相应类型的节点列表（PQNodes/PVNodes）中的索引。
				int SubIndex() const { return m_SubIndex; }
				void SubIndex(int val) { m_SubIndex = val; }
				double Voltage() const { return m_Voltage; }
				void Voltage(double val) { m_Voltage = val; }
				double Angle() const { return m_Angle; }		//弧度
				void Angle(double val) { m_Angle = val; }
				int Degree() const { return m_AdjacentBranches.size(); }		//母线连结出来的分支数量。
				NodeType Type() const { return m_Type; }		//母线的类型。
				void Type(NodeType val) { m_Type = val; }
				//对于PQ节点，保存已知的P、Q；
				//对于PV节点，保存已知的P、V。
				double ActivePowerInjection() const { return m_ActivePowerInjection; }
				void AddActivePowerInjection(double val) { m_ActivePowerInjection += val; }
				double ReactivePowerInjection() const { return m_ReactivePowerInjection; }
				void AddReactivePowerInjection(double val) { m_ReactivePowerInjection += val; }
				//此内联函数无法放置在cpp中,因为其被 Solver 的派生类所调用，
				//可能引发不可识别的外部函数的错误。
				void ClearPowerInjections()
				{
					m_ActivePowerInjection = m_ReactivePowerInjection = 0;
				}
				complexd VoltagePhasor()		//复数形式的电压相量
				{
					return std::polar(m_Voltage, m_Angle);
				}
				bool HasPowerInjection()		//获取一个值，指示此节点是否存在注入功率。
				{
					//如果以后发现此方法判断不严格，可以修改。
					return ActivePowerInjection() != 0 || ReactivePowerInjection() != 0;
				}
			public:
				NodeInfo(ObjectModel::Bus* bus);
			};
			class BranchInfo
			{
			public:
				typedef std::list<Component*> ComponentCollection;
			private:
				int m_Index;
				NodePair m_Nodes;
				ComponentCollection m_Components;
			public:
				int Multiplicity() const { return m_Components.size(); }
				int Index() const { return m_Index; }
				void Index(int val) { m_Index = val; }
				NodePair Nodes() const { return m_Nodes; }
				BusPair Buses() const { return std::make_pair(m_Nodes.first->Bus(), m_Nodes.second->Bus()); }
				void Nodes(NodePair val) { m_Nodes = val; }
				ComponentCollection& Components() { return m_Components; }
				NodeInfo* AnotherNode(NodeInfo* thisNode)
				{
					assert(m_Nodes.first == thisNode || m_Nodes.second == thisNode);
					if (m_Nodes.first == thisNode) return m_Nodes.second;
					return m_Nodes.first;
				}
			public:
				BranchInfo(int index, NodeInfo* node1, NodeInfo* node2)
					: m_Index(index), m_Nodes(node1, node2)
				{ }
			};
		public:
			typedef std::list<Bus*> BusCollection;
			typedef std::list<Component*> ComponentCollection;
			typedef std::vector<NodeInfo*> NodeCollection;
			typedef std::unordered_map<ObjectModel::Bus*, NodeInfo*> NodeDictionary;
			typedef std::vector<BranchInfo*> BranchCollection;
			typedef std::unordered_map < NodePair, BranchInfo*,
				Utility::UnorderedPairHasher<NodeInfo*>, Utility::UnorderedPairEqualityComparer < NodeInfo* >>
				BranchDictionary;
		_PS_INTERNAL:
			void AddPi(Bus* bus1, Bus* bus2, PiEquivalencyParameters pi);
			void AddShunt(Bus* bus, complexd admittance);
			void AddPQ(Bus* bus, complexd power);
			void AddPV(Bus* bus, double activePower, double voltage);
			void AddSlack(Bus* bus, complexd voltagePhasor);
			void ClaimParent(Bus* bus, Component* c);
			void ClaimBranch(Bus* bus1, Bus* bus2, Component* c);
		private:
			NetworkCase* m_SourceNetwork;
			PrimitiveNetworkOptions m_Options;
			SlackNodeAssignmentType m_SlackNodeAssignment;
			BusCollection m_Buses;
			NodeCollection m_PQNodes;			//参与计算的母线（PQ节点）信息，按照矩阵索引排序。
			NodeCollection m_PVNodes;			//参与计算的母线（PV节点）信息，按照矩阵索引排序。
			//注意到在NR法中，PQ 节点和 PV 节点的顺序是可以交错的。
			NodeCollection m_Nodes;
			NodeDictionary m_BusDict;			//Bus --> 节点信息
			NodeInfo* m_SlackNode;
			BranchCollection m_Branches;
			BranchDictionary m_BranchDict;
			//Eigen::SparseMatrix<bool> m_IncidenceMatrix;
		public:
			NetworkCase* SourceNetwork() const { return m_SourceNetwork; }
			PrimitiveNetworkOptions Options() const { return m_Options; }
			// 获取此网络中平衡节点的选取方式。
			SlackNodeAssignmentType SlackNodeAssignment() const { return m_SlackNodeAssignment; }
			bool IsEmpty() const { return m_Nodes.empty(); }
			const BusCollection& Buses() const { return m_Buses; }
			Eigen::SparseMatrix<complexd> Admittance;	//完整的导纳矩阵。
			//const ComponentCollection& Components() const { return m_Components; }
			//const BusComponentCollection& BusComponents() const { return m_BusComponents; }
			const NodeCollection& PQNodes() const { return m_PQNodes; }
			const NodeCollection& PVNodes() const { return m_PVNodes; }
			const NodeCollection& Nodes() const { return m_Nodes; }	//参与计算的三种节点，按照矩阵索引连续排序，注意平衡节点放在最后。
			NodeInfo* Nodes(size_t index) const { return m_Nodes.at(index); }
			NodeInfo* Nodes(Bus* busRef) const { return m_BusDict.at(busRef); }
			NodeInfo* TryGetNode(Bus* busRef) const
			{
				auto i = m_BusDict.find(busRef);
				if (i == m_BusDict.end()) return nullptr;
				return i->second;
			}
			NodeInfo* SlackNode() const { return m_SlackNode; }				//平衡节点的信息。
			const BranchCollection& Branches() const { return m_Branches; }	//记录节点连接（支路）(m,n)
			BranchInfo* Branches(size_t index) const { return m_Branches.at(index); }
			BranchInfo* Branches(NodePair branchRef) const
			{
				return m_BranchDict.at(branchRef);
			}
			BranchInfo* Branches(BusPair branchRef) const
			{
				return m_BranchDict.at(std::make_pair(m_BusDict.at(branchRef.first),
					m_BusDict.at(branchRef.second)));
			}
			BranchInfo* Branches(std::pair<size_t, size_t> branchRefIndex) const
			{
				return m_BranchDict.at(std::make_pair(m_Nodes[branchRefIndex.first],
					m_Nodes[branchRefIndex.second]));
			}
			BranchInfo* TryGetBranch(NodePair branchRef) const
			{
				auto i = m_BranchDict.find(branchRef);
				if (i == m_BranchDict.end()) return nullptr;
				return i->second;
			}
		public:	//图论支持
			std::vector<std::shared_ptr<PrimitiveNetwork>> ConnectedSubnetworks();
			void DumpGraph();
		private:
			void LoadNetworkCase(ObjectModel::NetworkCase* network, PrimitiveNetworkOptions options);
			void AssignSlackNode();
			template <class TNodeQueue, class TBranchQueue>
			void LoadSubnetwork(PrimitiveNetwork* source, TNodeQueue& nodes, TBranchQueue& branches);
		private:	// internal
			PrimitiveNetwork();
		public:
			~PrimitiveNetwork();
		};
	}
	template<>
	struct enable_bitmask_operators < ObjectModel::PrimitiveNetworkOptions >
	{
		static const bool enable = true;
	};
}

#endif	//__POWERSOLUTIONS_POWERFLOW_PRIMITIVENETWORK_H
