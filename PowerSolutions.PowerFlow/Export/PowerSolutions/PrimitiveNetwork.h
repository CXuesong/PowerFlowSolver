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
				typedef std::list<SinglePortComponent*> SinglePortComponentCollection;
			public:
				ObjectModel::Bus* Bus;			//ĸ�ߵ������Ϣ��
				SinglePortComponentCollection Components;	//ĸ�������ӵĵ���Ԫ����
				int Index = -1;					//�ڵ�������Nodes����
				int SubIndex = -1;				//�ڵ�����Ӧ���͵Ľڵ��б�PQNodes/PVNodes���е�������
				int Degree = 0;					//ĸ����������ķ�֧������
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
			public:
				NodeInfo(ObjectModel::Bus* bus);
			};
		public:
			typedef std::list<Bus*> BusCollection;
			typedef std::list<Component*> ComponentCollection;
			typedef std::vector<NodeInfo*> NodeCollection;
			typedef std::unordered_map<ObjectModel::Bus*, std::shared_ptr<NodeInfo>> NodeDictionary;
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
			void ClaimParent(Bus* bus, SinglePortComponent* c);
			void ClaimBranch(Bus* bus1, Bus* bus2);
		private:
			NetworkCase* m_SourceNetwork;
			BusCollection m_Buses;
			//ComponentCollection m_Components;
			//BusComponentCollection m_BusComponents;
		public:
			NetworkCase* SourceNetwork() const { return m_SourceNetwork; }
			const BusCollection& Buses() const { return m_Buses; }
			Eigen::SparseMatrix<complexd> Admittance;	//���ɾ���������ǲ��֣�row <= col����
			//const ComponentCollection& Components() const { return m_Components; }
			//const BusComponentCollection& BusComponents() const { return m_BusComponents; }
			NodeCollection PQNodes;			//��������ĸ�ߣ�PQ�ڵ㣩��Ϣ�����վ�����������
			NodeCollection PVNodes;			//��������ĸ�ߣ�PV�ڵ㣩��Ϣ�����վ�����������
			//ע�⵽��NR���У�PQ �ڵ�� PV �ڵ��˳���ǿ��Խ���ġ�
			NodeCollection Nodes;			//�����������ֽڵ㣬���վ���������������ע��ƽ��ڵ�������
			std::shared_ptr<NodeInfo> SlackNode;	//ƽ��ڵ����Ϣ��
			NodeDictionary BusMapping;				//Bus --> �ڵ���Ϣ
			BranchCollection Branches;				//��¼�ڵ����ӣ�֧·��(m,n)
		private:
			void LoadNetworkCase(ObjectModel::NetworkCase* network, bool nodeReorder);
		public:
			PrimitiveNetwork(ObjectModel::NetworkCase& network, bool nodeReorder);
		};
	}
}

#endif	//__POWERSOLUTIONS_POWERFLOW_PRIMITIVENETWORK_H
