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
				//ĸ�ߵ������Ϣ��
				ObjectModel::Bus* Bus() const { return m_Bus; }
				void Bus(ObjectModel::Bus* val) { m_Bus = val; }
				//ĸ�������ӵ�Ԫ����
				ComponentCollection& Components() { return m_Components; }
				//���ĸ���ڽӵĽڵ㡣
				NodeInfoCollection& AdjacentNodes() { return m_AdjacentNodes; }
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
				int Degree() const { return m_AdjacentNodes.size(); }		//ĸ����������ķ�֧������
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
			NodeCollection m_PQNodes;			//��������ĸ�ߣ�PQ�ڵ㣩��Ϣ�����վ�����������
			NodeCollection m_PVNodes;			//��������ĸ�ߣ�PV�ڵ㣩��Ϣ�����վ�����������
			//ע�⵽��NR���У�PQ �ڵ�� PV �ڵ��˳���ǿ��Խ���ġ�
			NodeCollection m_Nodes;
			NodeDictionary m_BusDict;				//Bus --> �ڵ���Ϣ
			NodeInfo* m_SlackNode;
			BranchCollection m_Branches;
			BranchDictionary m_BranchDict;
		public:
			NetworkCase* SourceNetwork() const { return m_SourceNetwork; }
			bool NodeReorder() const { return m_NodeReorder; }
			bool IgnoreShuntAdmittance() const { return m_IgnoreShuntAdmittance; }
			const BusCollection& Buses() const { return m_Buses; }
			Eigen::SparseMatrix<complexd> Admittance;	//�����ĵ��ɾ���
			//const ComponentCollection& Components() const { return m_Components; }
			//const BusComponentCollection& BusComponents() const { return m_BusComponents; }
			const NodeCollection& PQNodes() const { return m_PQNodes; }
			const NodeCollection& PVNodes() const { return m_PVNodes; }
			const NodeCollection& Nodes() const { return m_Nodes; }	//�����������ֽڵ㣬���վ���������������ע��ƽ��ڵ�������
			NodeInfo* Nodes(size_t index) const { return m_Nodes[index]; }
			NodeInfo* Nodes(Bus* busRef) const { return m_BusDict.at(busRef); }
			NodeInfo* TryGetNode(Bus* busRef) const
			{
				auto i = m_BusDict.find(busRef);
				if (i == m_BusDict.end()) return nullptr;
				return i->second;
			}
			NodeInfo* SlackNode() const { return m_SlackNode; }				//ƽ��ڵ����Ϣ��
			const BranchCollection& Branches() const { return m_Branches; }	//��¼�ڵ����ӣ�֧·��(m,n)
			BranchInfo* Branches(size_t index) const { return m_Branches[index]; }
			BranchInfo* Branches(NodePair branchRef) const
			{
				auto i = m_BranchDict.find(branchRef);
				if (i == m_BranchDict.end()) return nullptr;
				return i->second;
			}
		public:	//ͼ��֧��
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
