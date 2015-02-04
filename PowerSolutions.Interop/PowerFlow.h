#pragma once

#include "PowerSolutions.Interop.h"
#include "ObjectModel.h"

namespace PowerSolutions
{
	namespace Interop
	{
		namespace PowerFlow
		{
			public enum class SolverType : byte
			{
				NewtonRaphson = 0,
				FastDecoupled = 1
			};

			/// <summary>求解最终的结论。</summary>
			public enum class SolutionStatus : byte
			{
				Success = 0x00,					//求解成功结束。
				MaxIteration = 0x10,			//已经达到最大允许的迭代次数。
				IterationFailed = 0x11,			//迭代过程出现问题。
				IntelliIterationAbort = 0x12,	//启用智能迭代后，由于运算过程不收敛而被中断。
			};

			public value struct NodeFlowSolution
			{
			public:
				/// <summary>节点的电压相量。</summary>
				_WRAP_PROPERTY_CACHE(Voltage, Complex);
				/// <summary>节点的总出力。</summary>
				_WRAP_PROPERTY_CACHE(PowerGeneration, Complex);
				// <summary>节点的总负载。</summary>
				_WRAP_PROPERTY_CACHE(PowerConsumption, Complex);
			internal:
				NodeFlowSolution(const _NATIVE_PF NodeFlowSolution& native);
			};

			public value struct BranchFlowSolution
			{
			public:
				/// <summary>从节点1注入的功率。</summary>
				_WRAP_PROPERTY_CACHE(Power1, Complex);
				/// <summary>从节点2注入的功率。</summary>
				_WRAP_PROPERTY_CACHE(Power2, Complex);
				/// <summary>从节点1注入接地支路的功率。</summary>
				_WRAP_PROPERTY_CACHE(ShuntPower1, Complex);
				/// <summary>从节点2注入接地支路的功率。</summary>
				_WRAP_PROPERTY_CACHE(ShuntPower2, Complex);
			public:
				//指示功率的实际传输方向是否与约定的方向（Bus1->Bus2）相反。
				bool ReversedDirection()
				{
					return Power1.Real < 0;
				}
				//获取实际传输的功率大小。
				Complex PowerTransfer()
				{
					return Power1.Real > 0 ? -Power2 : -Power1;
				}
				//在两节点之间传输电能时损失的总功率（包括接地支路）。
				Complex PowerLoss()
				{
					return Power1 + Power2;
				}
				//在两节点的接地支路中损失的总功率。
				Complex PowerShunt()
				{
					return ShuntPower1 + ShuntPower2;
				}
			internal:
				BranchFlowSolution(const _NATIVE_PF BranchFlowSolution& native);
			};

			/// <summary>
			/// 稳态潮流的求解结果。
			/// （<see cref="PowerSolutions::PowerFlow::Solution" />的缓存结果。）
			/// </summary>
			public ref class Solution
			{
			private:
				Dictionary<Bus, NodeFlowSolution>^ m_NodeFlow;
				ReadOnlyDictionary<Bus, NodeFlowSolution>^ m_s_NodeFlow;
				Dictionary<Component, BranchFlowSolution>^ m_ComponentFlow;
				ReadOnlyDictionary<Component, BranchFlowSolution>^ m_s_ComponentFlow;
				Dictionary<BusPair, BranchFlowSolution>^ m_BranchFlow;
				ReadOnlyDictionary<BusPair, BranchFlowSolution>^ m_s_BranchFlow;
			public:
				_WRAP_PROPERTY_CACHE(TotalPowerGeneration, Complex);
				_WRAP_PROPERTY_CACHE(TotalPowerConsumption, Complex);
				_WRAP_PROPERTY_CACHE(TotalPowerLoss, Complex);
				_WRAP_PROPERTY_CACHE(TotalPowerShunt, Complex);
				_WRAP_PROPERTY_CACHE(IterationCount, int);
				_WRAP_PROPERTY_CACHE(MaxDeviation, double);
				_WRAP_PROPERTY_CACHE(Status, SolutionStatus);
				_WRAP_PROPERTY_CACHE(NodeCount, int);
				_WRAP_PROPERTY_CACHE(PQNodeCount, int);
				_WRAP_PROPERTY_CACHE(PVNodeCount, int);
				_WRAP_PROPERTY_CACHE(SlackNode, Bus);

				property IDictionary<Bus, NodeFlowSolution>^ NodeFlow
				{
					IDictionary<Bus, NodeFlowSolution>^ get(){ return m_s_NodeFlow; }
				}
				property IDictionary<Component, BranchFlowSolution>^ ComponentFlow
				{
					IDictionary<Component, BranchFlowSolution>^ get(){ return m_s_ComponentFlow; }
				}
				property IDictionary<BusPair, BranchFlowSolution>^ BranchFlow
				{
					IDictionary<BusPair, BranchFlowSolution>^ get(){ return m_s_BranchFlow; }
				}
			public:
				Solution(const _NATIVE_PF Solution& native);
			};

			/// <summary>
			/// 用于完成稳态潮流的求解过程。
			/// （<see cref="PowerSolutions::PowerFlow::Solver" />）
			/// </summary>
			public ref class Solver
			{
			private:
				_NATIVE_PF Solver* nativeObject;
			public:
				_WRAP_PROPERTY(MaxIterations, int, );
				_WRAP_PROPERTY(MaxDeviationTolerance, double, );
				_WRAP_PROPERTY(IntelliIterations, bool, );
			public:
				Solution^ Solve(ObjectModel::NetworkCase^ network);
			public:
				Solver(SolverType type);
				!Solver();
				~Solver();
			};
		}
	}
}
