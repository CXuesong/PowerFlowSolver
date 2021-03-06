﻿/*
PowerSolutions
稳态潮流求解实现部分（NR法）。
by Chen [CXuesong.], 2015
*/

#pragma once
#include "PowerFlowSolversImpl.h"

namespace PowerSolutions
{
	namespace PowerFlow
	{
		class NRSolver final : public SolverImpl
		{
		private:
			//定位点。
			int Block1EquationCount() const { return NodeCount - 1; }		//矩阵子块 H 的阶数。
			int Block2EquationCount() const { return PQNodeCount; }		//矩阵子块 L 的阶数。
			int EquationCount() const
			//需要求解的方程组的维数。
			{
				return Block1EquationCount() + Block2EquationCount();
			}
			// Δy = -J Δx
			// ConstraintPower - CurrentPower = - Jocobian * CorrectionAnswer
			Eigen::SparseMatrix<double> Jocobian;			//雅可比矩阵。
			int JocobianReservedValuesCount;				//雅可比矩阵非零元素数量的估计值。
			Eigen::VectorXd ConstraintPowerInjection;
			Eigen::VectorXd PowerInjectionDeviation;
			Eigen::VectorXd CorrectionAnswer;
			Eigen::VectorXd CurrentAnswer;
		private:
			void EvalPowerInjection();
			void GenerateJacobian();			//形成雅可比矩阵。
			bool GenerateNextAnswer();			//解出迭代的当前结果。
			double NodeVoltage(int NodeIndex);
			double NodeAngle(int NodeIndex);
			void UpdateNodeStatus(int nodeIndex)
			{
				PSolution->NodeStatus(nodeIndex).SetVoltage(NodeVoltage(nodeIndex), NodeAngle(nodeIndex));
			}
		protected:
			virtual void BeforeIterations() override;
			virtual std::pair<const ObjectModel::PrimitiveNetwork::NodeInfo*, double> EvalDeviation() override;
			virtual bool OnIteration() override;
			virtual void AfterIterations() override;
		public:
			NRSolver();
			virtual ~NRSolver();
		};
	}
}