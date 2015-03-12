
#ifndef __POWERSOLUTIONS_POWERFLOWSOLVERS_H
#define __POWERSOLUTIONS_POWERFLOWSOLVERS_H

#include "NetworkCase.h"
#include "PowerFlowSolution.h"
#include <memory>

namespace PowerSolutions
{
	namespace PowerFlow
	{
		enum class SolverType
		{
			NewtonRaphson = 0,
			FastDecoupled = 1
		};

		// 包含了求解器此时的状态信息。
		class IterationEventArgs
		{
		private:
			int m_IterationCount;
			double m_MaxDeviation;
		public:
			// 获取已经完成的迭代次数。0表示迭代尚未开始。
			int IterationCount() const { return m_IterationCount; }
			// 此次迭代结束时最大功率误差的绝对值。
			double MaxDeviation() const { return m_MaxDeviation; }
		public:
			// 初始化一个表示当前正在进行求解的 SolverStatus。
			IterationEventArgs(int iterationCount, double maxDeviation)
				: m_IterationCount(iterationCount), m_MaxDeviation(maxDeviation)
			{ }
		};

		// 用于在迭代过程中接收每一步的迭代信息。
		typedef void(__stdcall *IterationEventHandler)(class Solver* sender, IterationEventArgs* e);

		// 抽象用于完成稳态潮流的解决过程。
		class Solver
		{
		private:
			int m_MaxIterations;				//最大的迭代次数。
			double m_MaxDeviationTolerance;		//最大允许误差。
			bool m_IntelliIterations;			//允许根据收敛速度来判断是否有必要继续迭代。
			IterationEventHandler m_IterationEvent;	//允许在每一步迭代结束时接收迭代的信息。
		public:
			int MaxIterations() const { return m_MaxIterations; }
			void MaxIterations(int val) { m_MaxIterations = val; }
			double MaxDeviationTolerance() const { return m_MaxDeviationTolerance; }
			void MaxDeviationTolerance(double val) { m_MaxDeviationTolerance = val; }
			bool IntelliIterations() const { return m_IntelliIterations; }
			void IntelliIterations(bool val) { m_IntelliIterations = val; }
			IterationEventHandler IterationEvent() const { return m_IterationEvent; }
			void IterationEvent(IterationEventHandler val) { m_IterationEvent = val; }
		public:
			// 求解网络的功率潮流分布，并生成一个潮流分析报告。
			virtual Solution* Solve(ObjectModel::PrimitiveNetwork& network) = 0;
			Solution* Solve(ObjectModel::NetworkCase& network);
			Solver();
			virtual ~Solver();
		public:
			static Solver* Create(SolverType type);
		};
	}
}

#endif	//__POWERSOLUTIONS_POWERFLOWSOLVERS_H
