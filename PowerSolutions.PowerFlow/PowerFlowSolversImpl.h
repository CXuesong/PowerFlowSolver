﻿/*
PowerSolutions
稳态潮流求解实现（公共部分）。
by Chen [CXuesong.], 2015
*/

#pragma once

#include "PowerSolutions.h"
#include "PowerFlowObjectModel.h"
#include "PowerFlowSolvers.h"
#include "Utility.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <Eigen/Dense>
#include <Eigen/Sparse>

namespace PowerSolutions
{
	namespace PowerFlow
	{
		// 抽象用于完成稳态潮流的解决过程。
		class SolverImpl : public Solver
		{
		protected:	//内部数据
			const ObjectModel::PrimitiveNetwork* PNetwork;
			//操作密集型数据的局部缓存。
			int NodeCount;								//实际参与计算的节点数量。
			int PQNodeCount;							//PQ节点数量。
			int PVNodeCount;							//PV节点数量。
			// 保存了当前的求解结果。
			std::shared_ptr<PrimitiveSolution> PSolution;
		protected:
			//约定：以下函数将会按照声明顺序被依次调用。
			virtual void BeforeIterations() = 0;
			virtual std::pair<int, double> EvalDeviation() = 0;		//计算当前结果中误差最大的节点编号以及迭代误差。
			virtual bool OnIteration() = 0;
			virtual void AfterIterations() = 0;
			std::shared_ptr<Solution> GenerateSolution(SolutionStatus status, int iterCount, double maxDev);
		public:
			virtual std::shared_ptr<Solution> Solve(const ObjectModel::PrimitiveNetwork& network) override;		// 求解网络的功率潮流分布。返回值表示是否成功收敛。
			SolverImpl();
			virtual ~SolverImpl();
		};
	}
}