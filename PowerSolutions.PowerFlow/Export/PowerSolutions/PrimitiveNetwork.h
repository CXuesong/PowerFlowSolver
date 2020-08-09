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
				//ĸ�ߵ������Ϣ��
				ObjectModel::Bus* Bus() const { return m_Bus; }
				void Bus(ObjectModel::Bus* val) { m_Bus = val; }
				//ĸ�������ӵ�Ԫ����������������Ԫ����
				ComponentCollection& Components() { return m_Components; }
				//���ĸ���ڽӵĽڵ㡣
				BranchInfoCollection& AdjacentBranches() { return m_AdjacentBranches; }
				//�ڵ�������Nodes����
				int Index() const { return m_Index; }
				void Index(int val) { m_Index = val; }
				//�ڵ�����Ӧ���͵Ľڵ��б�PQNodes/PVNodes���е�������
				int SubIndex() const { return m_SubIndex; }
				void SubIndex(int val) { m_SubIndex = val; }
				double Voltage() const { return m_Voltage; }
				void Voltage(double val) { m_Voltage = val; }
				double Angle() const { return m_Angle; }		//����
				void Angle(double val) { m_Angle = val; }
				int Degree() const { return m_AdjacentBranches.size(); }		//ĸ����������ķ�֧������
				NodeType Type() const { return m_Type; }		//ĸ�ߵ����͡�
				void Type(NodeType val) { m_Type = val; }
				//����PQ�ڵ㣬������֪��P��Q��
				//����PV�ڵ㣬������֪��P��V��
				double ActivePowerInjection() const { return m_ActivePowerInjection; }
				void AddActivePowerInjection(double val) { m_ActivePowerInjection += val; }
				double ReactivePowerInjection() const { return m_ReactivePowerInjection; }
				void AddReactivePowerInjection(double val) { m_ReactivePowerInjection += val; }
				//�����������޷�������cpp��,��Ϊ�䱻 Solver �������������ã�
				//������������ʶ����ⲿ�����Ĵ���
				void ClearPowerInjections()
				{
					m_ActivePowerInjection = m_ReactivePowerInjection = 0;
				}
				complexd VoltagePhasor()		//������ʽ�ĵ�ѹ����
				{
					return std::polar(m_Voltage, m_Angle);
				}
				bool HasPowerInjection()		//��ȡһ��ֵ��ָʾ�˽ڵ��Ƿ����ע�빦�ʡ�
				{
					//����Ժ��ִ˷����жϲ��ϸ񣬿����޸ġ�
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
			NodeCollection m_PQNodes;			//��������ĸ�ߣ�PQ�ڵ㣩��Ϣ�����վ�����������
			NodeCollection m_PVNodes;			//��������ĸ�ߣ�PV�ڵ㣩��Ϣ�����վ�����������
			//ע�⵽��NR���У�PQ �ڵ�� PV �ڵ��˳���ǿ��Խ���ġ�
			NodeCollection m_Nodes;
			NodeDictionary m_BusDict;			//Bus --> �ڵ���Ϣ
			NodeInfo* m_SlackNode;
			BranchCollection m_Branches;
			BranchDictionary m_BranchDict;
			//Eigen::SparseMatrix<bool> m_IncidenceMatrix;
		public:
			NetworkCase* SourceNetwork() const { return m_SourceNetwork; }
			PrimitiveNetworkOptions Options() const { return m_Options; }
			// ��ȡ��������ƽ��ڵ��ѡȡ��ʽ��
			SlackNodeAssignmentType SlackNodeAssignment() const { return m_SlackNodeAssignment; }
			bool IsEmpty() const { return m_Nodes.empty(); }
			const BusCollection& Buses() const { return m_Buses; }
			Eigen::SparseMatrix<complexd> Admittance;	//�����ĵ��ɾ���
			//const ComponentCollection& Components() const { return m_Components; }
			//const BusComponentCollection& BusComponents() const { return m_BusComponents; }
			const NodeCollection& PQNodes() const { return m_PQNodes; }
			const NodeCollection& PVNodes() const { return m_PVNodes; }
			const NodeCollection& Nodes() const { return m_Nodes; }	//�����������ֽڵ㣬���վ���������������ע��ƽ��ڵ�������
			NodeInfo* Nodes(size_t index) const { return m_Nodes.at(index); }
			NodeInfo* Nodes(Bus* busRef) const { return m_BusDict.at(busRef); }
			NodeInfo* TryGetNode(Bus* busRef) const
			{
				auto i = m_BusDict.find(busRef);
				if (i == m_BusDict.end()) return nullptr;
				return i->second;
			}
			NodeInfo* SlackNode() const { return m_SlackNode; }				//ƽ��ڵ����Ϣ��
			const BranchCollection& Branches() const { return m_Branches; }	//��¼�ڵ����ӣ�֧·��(m,n)
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
		public:	//ͼ��֧��
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
