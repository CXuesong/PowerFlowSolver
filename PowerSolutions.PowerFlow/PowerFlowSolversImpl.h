/*
PowerSolutions
��̬�������ʵ�֣��������֣���
by  Chen [CXuesong.], 2015
*/

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
			enum class NodeType : byte
			{
				PQNode = 0,		//PQ�ڵ㡣
				PVNode,			//PV�ڵ㡣
				SlackNode,		//ƽ��ڵ㡣
			};
			class NodeInfo
			{
			public:
				ObjectModel::Bus* Bus;			//ĸ�ߵ������Ϣ��
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
				double ActivePowerInjection;
				double ReactivePowerInjection;
				//�����������޷�������cpp��,��Ϊ�䱻 Solver �������������ã�
				//������������ʶ����ⲿ�����Ĵ���
				void ClearPowerInjections()
				{
					ActivePowerInjection = ReactivePowerInjection = 0;
				}
				complexd VoltagePhasor();		//������ʽ�ĵ�ѹ����
				void AddPQ(complexd power);
				bool AddPV(double activePower, double voltage);
				bool AddSlack(complexd voltagePhasor);
				NodeInfo(ObjectModel::Bus* bus, NodeType type)
					: Bus(bus), Type(type)
				{ }
				NodeInfo(ObjectModel::Bus* bus)
					: Bus(bus), Type(NodeType::PQNode)
				{ }
			};
			typedef std::vector<NodeInfo*> NodeCollection;
			typedef std::unordered_map<ObjectModel::Bus*, std::shared_ptr<NodeInfo>> NodeDictionary;
			typedef std::unordered_set < std::pair<ObjectModel::Bus*, ObjectModel::Bus*>,
				Utility::UnorderedPairHasher<ObjectModel::Bus*>, Utility::UnorderedPairEqualityComparer<ObjectModel::Bus* >>
				BranchCollection;

			ObjectModel::ExpandedNetworkCase CaseInfo;
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
			void GenerateAdmittance();				//ɨ������б��γɵ��ɾ���ͬʱ����ĸ�����͡�
		protected:
			//Լ�������º������ᰴ������˳�����ε��á�
			virtual void BeforeIterations() = 0;
			virtual double EvalDeviation() = 0;		//���㵱ǰ����ĵ�����
			virtual bool OnIteration() = 0;
			virtual void AfterIterations() = 0;
			Solution* GenerateSolution(SolutionStatus status, int iterCount, double maxDev);
		public:
			Solution* Solve(ObjectModel::NetworkCase* caseInfo);									// �������Ĺ��ʳ����ֲ�������ֵ��ʾ�Ƿ�ɹ�������
			SolverImpl();
			virtual ~SolverImpl();
		};
	}
}