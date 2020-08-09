/*
PowerSolutions
��̬�������ʵ�֣��������֣���
by Chen [CXuesong.], 2015
*/

#pragma once

#include "PowerSolutions.h"
#include "PowerFlowObjectModel.h"
#include "PowerFlowSolvers.h"
#include "Utility.h"
#include "PrimitiveNetworkImpl.h"
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
			std::shared_ptr<ObjectModel::PrimitiveNetworkImpl> PNetwork;
			//�����ܼ������ݵľֲ����档
			int NodeCount;							//ʵ�ʲ������Ľڵ�������
			int PQNodeCount;							//PQ�ڵ�������
			int PVNodeCount;							//PV�ڵ�������
		protected:
			//Լ�������º������ᰴ������˳�����ε��á�
			virtual void BeforeIterations() = 0;
			virtual double EvalDeviation() = 0;		//���㵱ǰ����ĵ�����
			virtual bool OnIteration() = 0;
			virtual void AfterIterations() = 0;
			Solution* GenerateSolution(SolutionStatus status, int iterCount, double maxDev);
		public:
			virtual Solution* Solve(ObjectModel::NetworkCase* caseInfo) override;		// �������Ĺ��ʳ����ֲ�������ֵ��ʾ�Ƿ�ɹ�������
			SolverImpl();
			virtual ~SolverImpl();
		};
	}
}