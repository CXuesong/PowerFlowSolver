
#ifndef __POWERSOLUTIONS_POWERFLOWSOLVERS_H
#define __POWERSOLUTIONS_POWERFLOWSOLVERS_H

#include "PowerSolutions.h"
#include "NetworkCase.h"
#include "PowerFlowSolution.h"
#include <memory>

namespace PowerSolutions
{
	namespace PowerFlow
	{
		// 抽象用于完成稳态潮流的解决过程。
		class Solver
		{
		private:
			int m_MaxIterations;				//最大的迭代次数。
			double m_MaxDeviationTolerance;		//最大允许误差。
			bool m_NodeReorder;					//允许节点重新排序。
			bool m_IntelliIterations;			//允许根据收敛速度来判断是否有必要继续迭代。
		public:
			int MaxIterations() const { return m_MaxIterations; }
			void MaxIterations(int val) { m_MaxIterations = val; }
			double MaxDeviationTolerance() const { return m_MaxDeviationTolerance; }
			void MaxDeviationTolerance(double val) { m_MaxDeviationTolerance = val; }
			bool NodeReorder() const { return m_NodeReorder; }
			void NodeReorder(bool val) { m_NodeReorder = val; }
			bool IntelliIterations() const { return m_IntelliIterations; }
			void IntelliIterations(bool val) { m_IntelliIterations = val; }
		public:
			// 求解网络的功率潮流分布，并生成一个潮流分析报告。
			virtual std::shared_ptr<Solution*> Solve(ObjectModel::NetworkCase* CaseInfo) = 0;
			Solver();
			virtual ~Solver();
		};
	}
}

#endif	//__POWERSOLUTIONS_POWERFLOWSOLVERS_H