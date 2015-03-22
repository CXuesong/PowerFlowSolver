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
		class PrimitiveNetwork
		{
		public:
			class NodeInfo
			{
			public:
				typedef std::list<Component*> ComponentCollection;
				typedef std::list<NodeInfo*> NodeInfoCollection;
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
				NodeInfoCollection m_AdjacentNodes;
			public:
				//母线的组件信息。
				ObjectModel::Bus* Bus() const { return m_Bus; }
				void Bus(ObjectModel::Bus* val) { m_Bus = val; }
				//母线所连接的元件。
				ComponentCollection& Components() { return m_Components; }
				//与此母线邻接的节点。
				NodeInfoCollection& AdjacentNodes() { return m_AdjacentNodes; }
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
				int Degree() const { return m_AdjacentNodes.size(); }		//母线连结出来的分支数量。
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
			typedef std::pair<NodeInfo*, NodeInfo*> NodePair;
			class BranchInfo
			{
			private:
				int m_Index;
				NodePair m_Nodes;
			public:
				int Index() const { return m_Index; }
				NodePair Nodes() const { return m_Nodes; }
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
			void ClaimBranch(Bus* bus1, Bus* bus2);
		private:
			NetworkCase* m_SourceNetwork;
			bool m_NodeReorder;
			bool m_IgnoreShuntAdmittance;
			BusCollection m_Buses;
			NodeCollection m_PQNodes;			//参与计算的母线（PQ节点）信息，按照矩阵索引排序。
			NodeCollection m_PVNodes;			//参与计算的母线（PV节点）信息，按照矩阵索引排序。
			//注意到在NR法中，PQ 节点和 PV 节点的顺序是可以交错的。
			NodeCollection m_Nodes;
			NodeDictionary m_BusDict;				//Bus --> 节点信息
			NodeInfo* m_SlackNode;
			BranchCollection m_Branches;
			BranchDictionary m_BranchDict;
		public:
			NetworkCase* SourceNetwork() const { return m_SourceNetwork; }
			bool NodeReorder() const { return m_NodeReorder; }
			bool IgnoreShuntAdmittance() const { return m_IgnoreShuntAdmittance; }
			const BusCollection& Buses() const { return m_Buses; }
			Eigen::SparseMatrix<complexd> Admittance;	//完整的导纳矩阵。
			//const ComponentCollection& Components() const { return m_Components; }
			//const BusComponentCollection& BusComponents() const { return m_BusComponents; }
			const NodeCollection& PQNodes() const { return m_PQNodes; }
			const NodeCollection& PVNodes() const { return m_PVNodes; }
			const NodeCollection& Nodes() const { return m_Nodes; }	//参与计算的三种节点，按照矩阵索引连续排序，注意平衡节点放在最后。
			NodeInfo* Nodes(size_t index) const { return m_Nodes[index]; }
			NodeInfo* Nodes(Bus* busRef) const { return m_BusDict.at(busRef); }
			NodeInfo* TryGetNode(Bus* busRef) const
			{
				auto i = m_BusDict.find(busRef);
				if (i == m_BusDict.end()) return nullptr;
				return i->second;
			}
			NodeInfo* SlackNode() const { return m_SlackNode; }				//平衡节点的信息。
			const BranchCollection& Branches() const { return m_Branches; }	//记录节点连接（支路）(m,n)
			BranchInfo* Branches(size_t index) const { return m_Branches[index]; }
			BranchInfo* Branches(NodePair branchRef) const
			{
				auto i = m_BranchDict.find(branchRef);
				if (i == m_BranchDict.end()) return nullptr;
				return i->second;
			}
		public:	//图论支持
			std::vector<std::shared_ptr<PrimitiveNetwork>> ConnectedSubsets();
		private:
			void LoadNetworkCase(ObjectModel::NetworkCase* network);
			template <class TNodeQueue, class TBranchQueue>
			PrimitiveNetwork(PrimitiveNetwork* source, TNodeQueue& nodes, TBranchQueue& branches);
		public:
			PrimitiveNetwork(ObjectModel::NetworkCase& network, bool nodeReorder);
			PrimitiveNetwork(ObjectModel::NetworkCase& network, bool nodeReorder, bool ignoreShuntAdmittance);
			~PrimitiveNetwork();
		};
	}
}

#endif	//__POWERSOLUTIONS_POWERFLOW_PRIMITIVENETWORK_H
