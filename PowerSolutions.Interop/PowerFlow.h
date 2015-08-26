#pragma once
//TODO 包装 ComponentFlow
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
				/// <summary>从注入接地支路的功率。</summary>
				_WRAP_PROPERTY_CACHE(PowerShunt, Complex);
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
				BranchFlowSolution Reverse()
				{
					return BranchFlowSolution(Power2, Power1, PowerShunt);
				}
			internal:
				BranchFlowSolution(Complex power1, Complex power2, Complex powerShunt);
				BranchFlowSolution(const _NATIVE_PF BranchFlowSolution& native);
			};

			public value struct ComponentFlowSolution
			{
			public:
				/// <summary>从节点1注入的功率。</summary>
				_WRAP_PROPERTY_CACHE(PowerInjections, cli::array<Complex>^);
				/// <summary>从节点2注入的功率。</summary>
				_WRAP_PROPERTY_CACHE(PowerShunt, Complex);
				/// <summary>指示此元件自身的潮流是否为不定的。</summary>
				_WRAP_PROPERTY_CACHE(IsUnconstrained, bool);
			internal:
				ComponentFlowSolution(const _NATIVE_PF ComponentFlowSolution& native);
			};

			public ref class IterationEventArgs : public System::EventArgs
			{
			public:
				/// <summary>已经完成的迭代次数。</summary>
				_WRAP_PROPERTY_CACHE(IterationCount, double);
				/// <summary>此次迭代结束后的最大功率误差绝对值。</summary>
				_WRAP_PROPERTY_CACHE(MaxDeviation, double);
				// 获取此迭代信息的字符串表现形式。
				String^ ToString() override { return String::Format(L"ΔP={0}", MaxDeviation); }
			public:
				IterationEventArgs(const _NATIVE_PF IterationEventArgs& native);
			};

			public delegate void IterationEventHandler(System::Object^ sender, IterationEventArgs^ e);

			/// <summary>
			/// 稳态潮流的求解结果。
			/// （<see cref="PowerSolutions::PowerFlow::Solution" />的缓存结果。）
			/// </summary>
			public ref class Solution
			{
			private:
				Dictionary<Bus, NodeFlowSolution>^ m_NodeFlow;
				ReadOnlyDictionary<Bus, NodeFlowSolution>^ m_s_NodeFlow;
				Dictionary<Component, ComponentFlowSolution>^ m_ComponentFlow;
				ReadOnlyDictionary<Component, ComponentFlowSolution>^ m_s_ComponentFlow;
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
				property IDictionary<Component, ComponentFlowSolution>^ ComponentFlow
				{
					IDictionary<Component, ComponentFlowSolution>^ get(){ return m_s_ComponentFlow; }
				}
				property IDictionary<BusPair, BranchFlowSolution>^ BranchFlow
				{
					IDictionary<BusPair, BranchFlowSolution>^ get(){ return m_s_BranchFlow; }
				}
			public:
				String^ ToString() override;
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
				delegate void NativeIterationEventHandler(_NATIVE_PF Solver* sender, _NATIVE_PF IterationEventArgs* e);
			private:
				_NATIVE_PF Solver* nativeObject;
				SolverType m_Type;
				IterationEventHandler^ m_IterationEvent;
				void NativeIterationEventProc(_NATIVE_PF Solver* sender, _NATIVE_PF IterationEventArgs* e);
				NativeIterationEventHandler^ NativeIterationEventDelegate;
				IntPtr NativeIterationEventProcAddress;
			public:
				/// <summary>
				/// 在第一次迭代开始前以及每一次迭代操作完成后引发此事件。
				/// </summary>
				event IterationEventHandler^ Iteration
				{
					void add(IterationEventHandler^ name);
					void remove(IterationEventHandler^ name);
				protected:
					void raise(System::Object^ sender, IterationEventArgs^ e)
					{
						if (m_IterationEvent)
							m_IterationEvent->Invoke(sender, e);
					}
				}
			public:
				//_WRAP_PROPERTY(NodeReorder, bool, );
				/// <summary>
				/// 指示在分析网络时，是否应当启用节点重排序功能。
				/// </summary>
				property bool NodeReorder;
				_WRAP_PROPERTY(MaxIterations, int, );
				_WRAP_PROPERTY(MaxDeviationTolerance, double, );
				_WRAP_PROPERTY(IntelliIterations, bool, );
				property SolverType Type
				{
					SolverType get() { return m_Type; }
				}
				property String^ FriendlyName
				{
					String^ get();
				}
			public:
				// 求解网络的功率潮流分布，并生成一个潮流分析报告。
				Solution^ Solve(ObjectModel::NetworkCase^ network);
			public:
				Solver(SolverType type);
				!Solver();
				~Solver();
			};
		}
	}
}
