
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
		struct SolverStatus
		{
		private:
			bool m_IsIterating;
			int m_LastIterationCount;
			IterationInfo m_LastIterationInfo;
		public:
			// 获取一个值，指示了当前是否正在进行迭代。注意：如果此时正在进行迭代前或迭代后操作，则也会返回 false。
			bool IsIterating() const { return m_IsIterating; }
			// 获取已经完成的迭代次数。
			int LastIterationCount() const { return m_LastIterationCount; }
			// 获取上一次迭代的最大功率误差。
			const IterationInfo& LastIterationInfo() const { return m_LastIterationInfo; }
		public:
			// 初始化一个表示当前未在进行求解的 SolverStatus。
			SolverStatus()
				: m_IsIterating(false), m_LastIterationCount(0), m_LastIterationInfo(0)
			{ }
			// 初始化一个表示当前正在进行求解的 SolverStatus。
			SolverStatus(int lastIterationCount, const IterationInfo& lastIterationInfo)
				: m_IsIterating(true), m_LastIterationCount(lastIterationCount), m_LastIterationInfo(lastIterationInfo)
			{ }
		};

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
			virtual Solution* Solve(ObjectModel::NetworkCase* CaseInfo) = 0;
			// 获取当前的求解状态。
			// 在求解过程中，可以使用另一个线程调用此函数以查询状态。
			virtual SolverStatus GetStatus() = 0;
			Solver();
			virtual ~Solver();
		public:
			static Solver* Create(SolverType type);
		};
	}
}

#endif	//__POWERSOLUTIONS_POWERFLOWSOLVERS_H
