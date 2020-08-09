#pragma once

#include "PowerSolutions.h"
#include "PowerFlowObjectModel.h"
#include "PowerFlowSolvers.h"
#include "Utility.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <Eigen/Sparse>

namespace PowerSolutions
{
	namespace PowerFlow
	{
		// �������������̬�����Ľ�����̡�
		class SolverImpl : public Solver
		{
		protected:	//�ڲ�����
			// �ڵ�����͡�
			enum class NodeType
			{
				PQNode = 0,		//PQ�ڵ㡣
				PVNode,			//PV�ڵ㡣
				SlackNode,		//ƽ��ڵ㡣
			};
			class NodeInfo
			{
			public:
				ObjectModel::Bus* Bus;				//ĸ�ߵ������Ϣ��
				int Index = -1;						//�����Ż��Ľڵ�������
				int SubIndex = -1;					//�ڵ�����Ӧ���͵Ľڵ��б�PQNodes/PVNodes���е�������
				int Degree = 0;				//ĸ����������ķ�֧������
				NodeType Type;						//ĸ�ߵ����͡�
				//�������Ļ���ֵ
				double Voltage;
				double Angle;						//����
				double ActivePowerInjection;
				double ReactivePowerInjection;
				void ClearPowerInjections()
				{
					ActivePowerInjection = ReactivePowerInjection = 0;
				}
				void AddPQ(complexd power)
				{
					ActivePowerInjection += power.real();
					ReactivePowerInjection += power.imag();
				}
				bool AddPV(double activePower, double voltage)
				{
					if (Type != NodeType::PQNode && std::abs(Voltage - voltage) > 1e-10)
						return false;
					Voltage = voltage;
					ActivePowerInjection += activePower;
					Type = NodeType::PVNode;
					return true;
				}
				bool AddSlack(complexd voltagePhasor)
				{
					if (Type != NodeType::PQNode && std::abs(Voltage - std::abs(voltagePhasor)) > 1e-10)
						return false;
					Voltage = std::abs(voltagePhasor);
					Angle = std::arg(voltagePhasor);
					Type = NodeType::SlackNode;
					return true;
				}
				NodeInfo(ObjectModel::Bus* bus, NodeType type)
					: Bus(bus), Type(type)
				{
				}
			};
			typedef std::vector<std::shared_ptr<NodeInfo>> NodeCollection;
			typedef std::unordered_map<ObjectModel::Bus*, std::shared_ptr<NodeInfo>> NodeDictionary;
			typedef std::unordered_set < std::pair<ObjectModel::Bus*, ObjectModel::Bus*>,
				Utility::UnorderedPairHasher<ObjectModel::Bus*>, Utility::UnorderedPairEqualityComparer<ObjectModel::Bus* >>
				BranchCollection;

			ObjectModel::NetworkCase *CaseInfo;
			NodeCollection PQNodes;			//��������ĸ�ߣ�PQ�ڵ㣩��Ϣ�����վ�����������
			NodeCollection PVNodes;			//��������ĸ�ߣ�PV�ڵ㣩��Ϣ�����վ�����������
			//ע�⵽��NR���У�PQ �ڵ�� PV �ڵ��˳���ǿ��Խ���ġ�
			NodeCollection Nodes;			//�����������ֽڵ㣬���վ���������������ע��ƽ��ڵ�������
			std::shared_ptr<NodeInfo> SlackNode;					//ƽ��ڵ����Ϣ��
			NodeDictionary BusMapping;				//Bus --> �ڵ���Ϣ
			BranchCollection Branches;				//��¼�ڵ����ӣ�֧·��(m,n)
			int NodeCount;							//ʵ�ʲ������Ľڵ�������
			int PQNodeCount;							//PQ�ڵ�������
			int PVNodeCount;							//PV�ڵ�������
			Eigen::SparseMatrix<complexd> Admittance;	//���ɾ���������ǲ��֣�row <= col����
		protected:
			void MapBuses();					//�Ż��ڵ��ţ�����ĸ���ھ����е����λ�á�
			void ScanComponents();				//ɨ������б��γɵ��ɾ���ͬʱ����ĸ�����͡�
			//inline int MapBus(int BusIndex)		//��ĸ�߱��ת��Ϊ��������
			//{
			//	assert(BusIndex == NullIndex || BusMapping.find(BusIndex) != BusMapping.end());
			//	return BusIndex == NullIndex ? NullIndex : BusMapping[BusIndex]->Index;
			//}
		protected:
			//Լ�������º������ᰴ������˳�����ε��á�
			virtual void BeforeIterations() = 0;
			virtual double EvalDeviation() = 0;		//���㵱ǰ����ĵ�����
			virtual bool OnIteration() = 0;
			virtual void AfterIterations();
		public:
			bool Solve();									// �������Ĺ��ʳ����ֲ�������ֵ��ʾ�Ƿ�ɹ�������
			SolverImpl();
			virtual ~SolverImpl();
		};
	}
}