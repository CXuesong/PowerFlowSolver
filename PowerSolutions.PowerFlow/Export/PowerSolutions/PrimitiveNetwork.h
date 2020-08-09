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
		// �ڵ�����͡�
		enum class NodeType : byte
		{
			PQNode = 0,		//PQ�ڵ㡣
			PVNode,			//PV�ڵ㡣
			SlackNode,		//ƽ��ڵ㡣
		};
		enum class PrimitiveNetworkOptions : byte
		{
			None = 0,
			NodeReorder = 1,			//���нڵ����Ż���
			IgnoreShuntAdmittance = 2,	//���Խӵص��ɣ�����ֱ���������㡣ע���ѡ���ı� NodeInfo::Components ����Ϊ��
			NoAdmittanceMatrix = 4,		//�����ɵ��ɾ���һ�����ڴ�ͼ�۷�����
			//��������в�����ƽ��ڵ㣬�򽫷�����������PV�ڵ���Ϊƽ��ڵ㡣
			AutoAssignSlackNode = 8,
			//��֤�����д���ƽ��ڵ㡣
			//��������мȲ�����ƽ��ڵ㣬Ҳ������PV�ڵ㣬��ѡ��һPQ�ڵ���Ϊƽ��ڵ㡣
			ForceSetSlackNode = AutoAssignSlackNode | 16,
			//�����������ڴ�ͼ�۷������������������ڵ㼰���ڽӱ���
			PureGraphicalAnalysis = NoAdmittanceMatrix | ForceSetSlackNode,
		};
		//������ PrimitiveNetwork ��ƽ��ڵ��ѡȡ������
		enum class SlackNodeAssignmentType : byte
		{
			SlackGenerator = 0,		//ͨ��ƽ�ⷢ���ѡȡ��
			PVNode = 1,				//ͨ��ת���й���������PV�ڵ�ѡȡ��
			PQNode = 2,				//ͨ��ת��PQ�ڵ�ѡȡ��
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
				// ��Ȼ�����ǲ�Ӧ���ڼ��㳱��ʱ�޸�Ԫ����Ϣ��
				typedef std::list<const Component*> ComponentCollection;
				typedef std::list<BranchInfo*> BranchInfoCollection;
			private:
				const ObjectModel::Bus* _Bus;
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
				//ĸ�ߵ������Ϣ��
				const ObjectModel::Bus* Bus() const { return _Bus; }
				void Bus(const ObjectModel::Bus* val) { _Bus = val; }
				//ĸ�������ӵ�Ԫ����������������Ԫ����
				ComponentCollection& Components() { return _Components; }
				//���ĸ���ڽӵĽڵ㡣
				BranchInfoCollection& AdjacentBranches() { return _AdjacentBranches; }
				//�ڵ�������Nodes����
				int Index() const { return _Index; }
				void Index(int val) { _Index = val; }
				//�ڵ�����Ӧ���͵Ľڵ��б�PQNodes/PVNodes���е�������
				int SubIndex() const { return _SubIndex; }
				void SubIndex(int val) { _SubIndex = val; }
				double Voltage() const { return _Voltage; }
				void Voltage(double val) { _Voltage = val; }
				double Angle() const { return _Angle; }		//����
				void Angle(double val) { _Angle = val; }
				int Degree() const { return _AdjacentBranches.size(); }		//ĸ����������ķ�֧������
				NodeType Type() const { return _Type; }		//ĸ�ߵ����͡�
				void Type(NodeType val) { _Type = val; }
				//����PQ�ڵ㣬������֪��P��Q��
				//����PV�ڵ㣬������֪��P��V��
				double ActivePowerInjection() const { return _ActivePowerInjection; }
				void AddActivePowerInjection(double val) { _ActivePowerInjection += val; }
				double ReactivePowerInjection() const { return _ReactivePowerInjection; }
				void AddReactivePowerInjection(double val) { _ReactivePowerInjection += val; }
				//�����������޷�������cpp��,��Ϊ�䱻 Solver �������������ã�
				//������������ʶ����ⲿ�����Ĵ���
				void ClearPowerInjections()
				{
					_ActivePowerInjection = _ReactivePowerInjection = 0;
				}
				complexd VoltagePhasor()		//������ʽ�ĵ�ѹ����
				{
					return std::polar(_Voltage, _Angle);
				}
				bool HasPowerInjection()		//��ȡһ��ֵ��ָʾ�˽ڵ��Ƿ����ע�빦�ʡ�
				{
					//����Ժ��ִ˷����жϲ��ϸ񣬿����޸ġ�
					return ActivePowerInjection() != 0 || ReactivePowerInjection() != 0;
				}
			public:
				NodeInfo(const ObjectModel::Bus* bus);
			};
			class BranchInfo
			{
			public:
				// ��Ȼ�����ǲ�Ӧ���ڼ��㳱��ʱ�޸�Ԫ����Ϣ��
				typedef std::list<const Component*> ComponentCollection;
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
			typedef std::list<const Bus*> BusCollection;
			typedef std::list<const Component*> ComponentCollection;
			typedef std::vector<NodeInfo*> NodeCollection;
			// ����Ӧ�ò����޸���Ϊ�������ڵ�Bus*�����ݡ�����Ҫ�ģ�Ҳ������ NodeInfo::Bus
			typedef std::unordered_map<const ObjectModel::Bus*, NodeInfo*> NodeDictionary;
			typedef std::vector<BranchInfo*> BranchCollection;
			typedef std::unordered_map < NodePair, BranchInfo*,
				Utility::UnorderedPairHasher<NodeInfo*>, Utility::UnorderedPairEqualityComparer < NodeInfo* >>
				BranchDictionary;
		_PS_INTERNAL:
			void AddPi(const Bus* bus1, const Bus* bus2, PiEquivalencyParameters pi);
			void AddShunt(const Bus* bus, complexd admittance);
			void AddPQ(const Bus* bus, complexd power);
			void AddPV(const Bus* bus, double activePower, double voltage);
			void AddSlack(const Bus* bus, complexd voltagePhasor);
			void ClaimParent(const Bus* bus, const Component* c);
			void ClaimBranch(const Bus* bus1, const Bus* bus2, const Component* c);
		private:
			const NetworkCase* _SourceNetwork;
			PrimitiveNetworkOptions _Options;
			SlackNodeAssignmentType _SlackNodeAssignment;
			BusCollection _Buses;
			NodeCollection _PQNodes;			//��������ĸ�ߣ�PQ�ڵ㣩��Ϣ�����վ�����������
			NodeCollection _PVNodes;			//��������ĸ�ߣ�PV�ڵ㣩��Ϣ�����վ�����������
			//ע�⵽��NR���У�PQ �ڵ�� PV �ڵ��˳���ǿ��Խ���ġ�
			NodeCollection _Nodes;
			NodeDictionary _BusDict;			//Bus --> �ڵ���Ϣ
			NodeInfo* _SlackNode;
			BranchCollection _Branches;
			BranchDictionary _BranchDict;
			//Eigen::SparseMatrix<bool> _IncidenceMatrix;
		public:
			const NetworkCase* SourceNetwork() const { return _SourceNetwork; }
			PrimitiveNetworkOptions Options() const { return _Options; }
			// ��ȡ��������ƽ��ڵ��ѡȡ��ʽ��
			SlackNodeAssignmentType SlackNodeAssignment() const { return _SlackNodeAssignment; }
			bool IsEmpty() const { return _Nodes.empty(); }
			const BusCollection& Buses() const { return _Buses; }
			Eigen::SparseMatrix<complexd> Admittance;	//�����ĵ��ɾ���
			//const ComponentCollection& Components() const { return _Components; }
			//const BusComponentCollection& BusComponents() const { return _BusComponents; }
			const NodeCollection& PQNodes() const { return _PQNodes; }
			const NodeCollection& PVNodes() const { return _PVNodes; }
			const NodeCollection& Nodes() const { return _Nodes; }	//�����������ֽڵ㣬���վ���������������ע��ƽ��ڵ�������
			NodeInfo& Nodes(size_t index) const { return *_Nodes.at(index); }
			NodeInfo& Nodes(const Bus* busRef) const { return *_BusDict.at(busRef); }
			NodeInfo* TryGetNode(const Bus* busRef) const
			{
				auto i = _BusDict.find(busRef);
				if (i == _BusDict.end()) return nullptr;
				return i->second;
			}
			NodeInfo* SlackNode() const { return _SlackNode; }				//ƽ��ڵ����Ϣ��
			const BranchCollection& Branches() const { return _Branches; }	//��¼�ڵ����ӣ�֧·��(m,n)
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
			//�����������ã�ʹ��ǰPrimitiveNetwork��ĸ�ߺ���·���ð���ָ���Ķ�Ӧ��ϵָ��ԭ�������磬�������縱����
			//	allowsUnmatch: ���Ϊ false�����ڸ���info���ṩ����Ϣ�޷��ҵ��ҵ�PrimitiveNetwork�ж�Ӧ��ĸ�߻�Ԫ��ʱ��
			//					�����쳣������ᱣ����ԭ��ĸ��/Ԫ�������á�
			void AdjustReferenceToPrototype(const NetworkCaseCorrespondenceInfo& info, bool allowsUnmatch);
			//�����������ã�ʹ��ǰPrimitiveNetwork��ĸ�ߺ���·���ð���ָ���Ķ�Ӧ��ϵָ��ԭ�������磬�������縱����
			void AdjustReferenceToPrototype(const NetworkCaseCorrespondenceInfo& info);
		public:	//ͼ��֧��
			std::vector<std::shared_ptr<PrimitiveNetwork>> ConnectedSubnetworks() const;
			void DumpGraph() const;
		private:
			// �˺����� NetworkCase::ToPrimitive ���á����ڼ������簸����
			void LoadNetworkCase(const ObjectModel::NetworkCase* network, PrimitiveNetworkOptions options);
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
