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
				//TODO 实现参数和状态的解耦
			public:
				typedef std::list<Component*> ComponentCollection;
				typedef std::list<Bus*> BusCollection;
			public:
				ObjectModel::Bus* Bus;			//母线的组件信息。
				ComponentCollection Components;	//母线所连接的元件。
				BusCollection AdjacentBuses;	//与此母线邻接的母线。
				int Index = -1;					//节点索引（Nodes）。
				int SubIndex = -1;				//节点在相应类型的节点列表（PQNodes/PVNodes）中的索引。
				int Degree() const { return AdjacentBuses.size(); }					//母线连结出来的分支数量。
				NodeType Type;					//母线的类型。
				//求解变量的缓存值。
				//生成目标解相量之前，
				//对于PQ节点，保存已知的P、Q；
				//对于PV节点，保存已知的P、V。
				//迭代结束后，保存了当前的解V/A/P/Q。
				double Voltage;
				double Angle;					//弧度
				double ActivePowerInjection = 0;
				double ReactivePowerInjection = 0;
				//此内联函数无法放置在cpp中,因为其被 Solver 的派生类所调用，
				//可能引发不可识别的外部函数的错误。
				void ClearPowerInjections()
				{
					ActivePowerInjection = ReactivePowerInjection = 0;
				}
				complexd VoltagePhasor()		//复数形式的电压相量
				{
					return std::polar(Voltage, Angle);
				}
			public:
				NodeInfo(ObjectModel::Bus* bus);
			};
		public:
			typedef std::list<Bus*> BusCollection;
			typedef std::list<Component*> ComponentCollection;
			typedef std::vector<NodeInfo*> NodeCollection;
			typedef std::unordered_map<ObjectModel::Bus*, NodeInfo*> NodeDictionary;
			typedef std::unordered_set < std::pair<ObjectModel::Bus*, ObjectModel::Bus*>,
				Utility::UnorderedPairHasher<ObjectModel::Bus*>, Utility::UnorderedPairEqualityComparer < ObjectModel::Bus* >>
				BranchCollection;
			//typedef std::unordered_multimap<Bus*, Component*> BusComponentCollection;
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
			NodeDictionary m_BusMapping;				//Bus --> 节点信息
			NodeInfo* m_SlackNode;
			BranchCollection m_Branches;
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
			NodeInfo* Nodes(Bus* busRef) const 
			{
				auto i = m_BusMapping.find(busRef);
				if (i == m_BusMapping.end()) return nullptr;
				return i->second;
			}
			NodeInfo* SlackNode() const { return m_SlackNode; }			//平衡节点的信息。
			const BranchCollection& Branches() const { return m_Branches; }	//记录节点连接（支路）(m,n)
		private:
			void LoadNetworkCase(ObjectModel::NetworkCase* network);
		public:
			PrimitiveNetwork(ObjectModel::NetworkCase& network, bool nodeReorder);
			PrimitiveNetwork(ObjectModel::NetworkCase& network, bool nodeReorder, bool ignoreShuntAdmittance);
			~PrimitiveNetwork();
		};
	}
}

#endif	//__POWERSOLUTIONS_POWERFLOW_PRIMITIVENETWORK_H
