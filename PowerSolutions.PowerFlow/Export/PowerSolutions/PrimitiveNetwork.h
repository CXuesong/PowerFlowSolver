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
				ObjectModel::Bus* _Bus;
				double _Voltage;
				double _Angle;
				int _Index;
				int _SubIndex;
				NodeType _Type;
				double _ActivePowerInjection;
				double _ReactivePowerInjection;
				ComponentCollection _Components;
				BranchInfoCollection _AdjacentBranches;
			public:
				//母线的组件信息。
				ObjectModel::Bus* Bus() const { return _Bus; }
				void Bus(ObjectModel::Bus* val) { _Bus = val; }
				//母线所连接的元件。包括单端与多端元件。
				ComponentCollection& Components() { return _Components; }
				//与此母线邻接的节点。
				BranchInfoCollection& AdjacentBranches() { return _AdjacentBranches; }
				//节点索引（Nodes）。
				int Index() const { return _Index; }
				void Index(int val) { _Index = val; }
				//节点在相应类型的节点列表（PQNodes/PVNodes）中的索引。
				int SubIndex() const { return _SubIndex; }
				void SubIndex(int val) { _SubIndex = val; }
				double Voltage() const { return _Voltage; }
				void Voltage(double val) { _Voltage = val; }
				double Angle() const { return _Angle; }		//弧度
				void Angle(double val) { _Angle = val; }
				int Degree() const { return _AdjacentBranches.size(); }		//母线连结出来的分支数量。
				NodeType Type() const { return _Type; }		//母线的类型。
				void Type(NodeType val) { _Type = val; }
				//对于PQ节点，保存已知的P、Q；
				//对于PV节点，保存已知的P、V。
				double ActivePowerInjection() const { return _ActivePowerInjection; }
				void AddActivePowerInjection(double val) { _ActivePowerInjection += val; }
				double ReactivePowerInjection() const { return _ReactivePowerInjection; }
				void AddReactivePowerInjection(double val) { _ReactivePowerInjection += val; }
				//此内联函数无法放置在cpp中,因为其被 Solver 的派生类所调用，
				//可能引发不可识别的外部函数的错误。
				void ClearPowerInjections()
				{
					_ActivePowerInjection = _ReactivePowerInjection = 0;
				}
				complexd VoltagePhasor()		//复数形式的电压相量
				{
					return std::polar(_Voltage, _Angle);
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
				int _Index;
				NodePair _Nodes;
				ComponentCollection _Components;
			public:
				int Multiplicity() const { return _Components.size(); }
				int Index() const { return _Index; }
				void Index(int val) { _Index = val; }
				NodePair Nodes() const { return _Nodes; }
				BusPair Buses() const { return std::make_pair(_Nodes.first->Bus(), _Nodes.second->Bus()); }
				void Nodes(NodePair val) { _Nodes = val; }
				ComponentCollection& Components() { return _Components; }
				NodeInfo* AnotherNode(NodeInfo* thisNode)
				{
					assert(_Nodes.first == thisNode || _Nodes.second == thisNode);
					if (_Nodes.first == thisNode) return _Nodes.second;
					return _Nodes.first;
				}
			public:
				BranchInfo(int index, NodeInfo* node1, NodeInfo* node2)
					: _Index(index), _Nodes(node1, node2)
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
			NetworkCase* _SourceNetwork;
			PrimitiveNetworkOptions _Options;
			SlackNodeAssignmentType _SlackNodeAssignment;
			BusCollection _Buses;
			NodeCollection _PQNodes;			//参与计算的母线（PQ节点）信息，按照矩阵索引排序。
			NodeCollection _PVNodes;			//参与计算的母线（PV节点）信息，按照矩阵索引排序。
			//注意到在NR法中，PQ 节点和 PV 节点的顺序是可以交错的。
			NodeCollection _Nodes;
			NodeDictionary _BusDict;			//Bus --> 节点信息
			NodeInfo* _SlackNode;
			BranchCollection _Branches;
			BranchDictionary _BranchDict;
			//Eigen::SparseMatrix<bool> _IncidenceMatrix;
		public:
			NetworkCase* SourceNetwork() const { return _SourceNetwork; }
			PrimitiveNetworkOptions Options() const { return _Options; }
			// 获取此网络中平衡节点的选取方式。
			SlackNodeAssignmentType SlackNodeAssignment() const { return _SlackNodeAssignment; }
			bool IsEmpty() const { return _Nodes.empty(); }
			const BusCollection& Buses() const { return _Buses; }
			Eigen::SparseMatrix<complexd> Admittance;	//完整的导纳矩阵。
			//const ComponentCollection& Components() const { return _Components; }
			//const BusComponentCollection& BusComponents() const { return _BusComponents; }
			const NodeCollection& PQNodes() const { return _PQNodes; }
			const NodeCollection& PVNodes() const { return _PVNodes; }
			const NodeCollection& Nodes() const { return _Nodes; }	//参与计算的三种节点，按照矩阵索引连续排序，注意平衡节点放在最后。
			NodeInfo& Nodes(size_t index) const { return *_Nodes.at(index); }
			NodeInfo& Nodes(Bus* busRef) const { return *_BusDict.at(busRef); }
			NodeInfo* TryGetNode(Bus* busRef) const
			{
				auto i = _BusDict.find(busRef);
				if (i == _BusDict.end()) return nullptr;
				return i->second;
			}
			NodeInfo* SlackNode() const { return _SlackNode; }				//平衡节点的信息。
			const BranchCollection& Branches() const { return _Branches; }	//记录节点连接（支路）(m,n)
			BranchInfo* Branches(size_t index) const { return _Branches.at(index); }
			BranchInfo* Branches(NodePair branchRef) const
			{
				return _BranchDict.at(branchRef);
			}
			BranchInfo* Branches(BusPair branchRef) const
			{
				return _BranchDict.at(std::make_pair(_BusDict.at(branchRef.first),
					_BusDict.at(branchRef.second)));
			}
			BranchInfo* Branches(std::pair<size_t, size_t> branchRefIndex) const
			{
				return _BranchDict.at(std::make_pair(_Nodes[branchRefIndex.first],
					_Nodes[branchRefIndex.second]));
			}
			BranchInfo* TryGetBranch(NodePair branchRef) const
			{
				auto i = _BranchDict.find(branchRef);
				if (i == _BranchDict.end()) return nullptr;
				return i->second;
			}
		public:
			//调整对象引用，使其按照指定的对应关系指向原来的网络，而非网络副本。
			void AdjustReferenceToPrototype(const NetworkCaseCorrespondenceInfo& info, bool strictMode = true);
		public:	//图论支持
			std::vector<std::shared_ptr<PrimitiveNetwork>> ConnectedSubnetworks() const;
			void DumpGraph() const;
		private:
			void LoadNetworkCase(ObjectModel::NetworkCase* network, PrimitiveNetworkOptions options);
			void AssignSlackNode();
			template <class TNodeQueue, class TBranchQueue>
			void LoadSubnetwork(const PrimitiveNetwork* source, TNodeQueue& nodes, TBranchQueue& branches);
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
