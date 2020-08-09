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
				//TODO ʵ�ֲ�����״̬�Ľ���
			public:
				typedef std::list<Component*> ComponentCollection;
				typedef std::list<NodeInfo*> NodeInfoCollection;
			public:
				ObjectModel::Bus* Bus;			//ĸ�ߵ������Ϣ��
				ComponentCollection Components;	//ĸ�������ӵ�Ԫ����
				NodeInfoCollection AdjacentNodes;	//���ĸ���ڽӵĽڵ㡣
				int Index = -1;					//�ڵ�������Nodes����
				int SubIndex = -1;				//�ڵ�����Ӧ���͵Ľڵ��б�PQNodes/PVNodes���е�������
				int Degree() const { return AdjacentNodes.size(); }					//ĸ����������ķ�֧������
				NodeType Type;					//ĸ�ߵ����͡�
				//�������Ļ���ֵ��
				//����Ŀ�������֮ǰ��
				//����PQ�ڵ㣬������֪��P��Q��
				//����PV�ڵ㣬������֪��P��V��
				//���������󣬱����˵�ǰ�Ľ�V/A/P/Q��
				double Voltage;
				double Angle;					//����
				double ActivePowerInjection = 0;
				double ReactivePowerInjection = 0;
				//�����������޷�������cpp��,��Ϊ�䱻 Solver �������������ã�
				//������������ʶ����ⲿ�����Ĵ���
				void ClearPowerInjections()
				{
					ActivePowerInjection = ReactivePowerInjection = 0;
				}
				complexd VoltagePhasor()		//������ʽ�ĵ�ѹ����
				{
					return std::polar(Voltage, Angle);
				}
				bool HasPowerInjection()		//��ȡһ��ֵ��ָʾ�˽ڵ��Ƿ����ע�빦�ʡ�
				{
					//����Ժ��ִ˷����жϲ��ϸ񣬿����޸ġ�
					return ActivePowerInjection != 0 || ReactivePowerInjection != 0;
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
			NodeInfo* Nodes(Bus* busRef) const 
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
			std::vector<std::shared_ptr<PrimitiveNetwork*>>&& ConnectedSubsets();
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
