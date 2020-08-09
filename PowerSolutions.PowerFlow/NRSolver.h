/*
PowerSolutions
��̬�������ʵ�ֲ��֣�NR������
by  Chen [CXuesong.], 2015
*/

#pragma once
#include "PowerFlowSolversImpl.h"
#include <Eigen/Sparse>

namespace PowerSolutions
{
	namespace PowerFlow
	{
		class NRSolver final : public SolverImpl
		{
		private:
			//��λ�㡣
			int Block1EquationCount() { return NodeCount - 1; }		//�����ӿ� H �Ľ�����
			int Block2EquationCount() { return PQNodeCount; }		//�����ӿ� L �Ľ�����
			int EquationCount()										//��Ҫ���ķ������ά����
			{
				return Block1EquationCount() + Block2EquationCount();
			}
			// ��y = -J ��x
			// ConstraintPower - CurrentPower = - Jocobian * CorrectionAnswer
			Eigen::SparseMatrix<double> Jocobian;		//�ſɱȾ���
			Eigen::VectorXd ConstraintPowerInjection;
			Eigen::VectorXd PowerInjectionDeviation;
			Eigen::VectorXd CorrectionAnswer;
			Eigen::VectorXd CurrentAnswer;
		private:
			void EvalPowerInjection();
			void GenerateJacobian();			//�γ��ſɱȾ���
			bool GenerateNextAnswer();			//��������ĵ�ǰ�����
			double NodeVoltage(int NodeIndex);
			double NodeAngle(int NodeIndex);
		protected:
			virtual void BeforeIterations() override;
			virtual double EvalDeviation() override;
			virtual bool OnIteration() override;
			virtual void AfterIterations() override;
		public:
			NRSolver();
			virtual ~NRSolver();
		};
	}
}