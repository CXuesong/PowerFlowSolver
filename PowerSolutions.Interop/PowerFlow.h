#pragma once

#include "PowerSolutions.Interop.h"
#include "ObjectModel.h"
#include "CollectionWrappers.h"

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
				property Complex Voltage;
				/// <summary>节点的总出力。</summary>
				property Complex PowerGeneration;
				// <summary>节点的总负载。</summary>
				property Complex PowerConsumption;
			internal:
				NodeFlowSolution(const _NATIVE_PF NodeFlowSolution& native);
			};

			public value struct BranchFlowSolution
			{
			public:
				/// <summary>从节点1注入的功率。</summary>
				property Complex Power1;
				/// <summary>从节点2注入的功率。</summary>
				property Complex Power2;
				/// <summary>从节点1注入接地支路的功率。</summary>
				property Complex ShuntPower1;
				/// <summary>从节点2注入接地支路的功率。</summary>
				property Complex ShuntPower2;
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

			public ref class NodeFlowDictionary 
				: ReadOnlyDictionaryWrapper < Bus, NodeFlowSolution, _NATIVE_PF Solution::NodeFlowIterator >
			{

			};

			public ref class Solution
			{
			internal:
				_NATIVE_PF Solution* nativeObject;
			public:
				_WRAP_PROPERTY_READONLY(TotalPowerGeneration, Complex, MarshalComplex);
				_WRAP_PROPERTY_READONLY(TotalPowerConsumption, Complex, MarshalComplex);
				_WRAP_PROPERTY_READONLY(TotalPowerLoss, Complex, MarshalComplex);
				_WRAP_PROPERTY_READONLY(TotalPowerShunt, Complex, MarshalComplex);
				_WRAP_PROPERTY_READONLY(IterationCount, int, );
				_WRAP_PROPERTY_READONLY(MaxDeviation, double, );
				_WRAP_PROPERTY_READONLY(Status, SolutionStatus, (SolutionStatus));
				//NodeFlowSolution NodeFlow(IntPtr )
			public:
				Solution(_NATIVE_PF Solution* native);
				!Solution();
				~Solution();
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
