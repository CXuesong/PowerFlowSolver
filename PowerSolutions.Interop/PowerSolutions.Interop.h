// PowerSolutions.Interop.h
#pragma once
#include <PowerSolutions/NetworkCase.h>
#include <PowerSolutions/PowerFlowSolvers.h>
#include <PowerSolutions/PowerFlowSolution.h>
#include "Utility.h"

using namespace System;
using namespace System::Numerics;

#define _NATIVE_OM ::PowerSolutions::ObjectModel::
#define _NATIVE_PF ::PowerSolutions::PowerFlow::
#define _WRAP_PROPERTY(name, type, mashaller) \
		property type name { \
			type get() { return mashaller(nativeObject->name()); } \
			void set(type value) { nativeObject->name(mashaller(value)); } \
		}
#define _WRAP_PROPERTY_READONLY(name, type, mashaller) \
		property type name { \
			type get() { return mashaller(nativeObject->name()); } \
		}

namespace PowerSolutions
{
	namespace Interop
	{
		namespace ObjectModel
		{
			/// <summary>
			/// 表示一个网络案例。
			///（<see cref="PowerSolutions.ObjectModel.NetworkCase" />）
			/// </summary>
			public ref class NetworkCase
			{
			internal:
				_NATIVE_OM NetworkCase* nativeObject;
			private:
				void AddObject(IntPtr obj);
			public:
				IntPtr AddBus(Complex initialVoltage);
				IntPtr AddBus() { return AddBus(1); }
				IntPtr AddLine(IntPtr bus1, IntPtr bus2, Complex impedance, Complex admittance);
				IntPtr AddPVGenerator(IntPtr bus1, double activePower, double voltage);
				IntPtr AddSlackGenerator(IntPtr bus1, Complex voltage);
				IntPtr AddPQLoad(IntPtr bus1, Complex power);
				IntPtr AddShuntAdmittance(IntPtr bus1, Complex admittance);
				IntPtr AddTransformer(IntPtr bus1, IntPtr bus2, Complex impedance, Complex admittance, Complex tapRatio);
				IntPtr AddTransformer(IntPtr bus1, IntPtr bus2, Complex impedance, Complex tapRatio)
				{
					return AddTransformer(bus1, bus2, impedance, 0, tapRatio);
				}
				IntPtr AddThreeWindingTransformer(IntPtr bus1, IntPtr bus2, IntPtr bus3,
					Complex impedance12, Complex impedance13, Complex impedance23,
					Complex admittance, Complex tapRatio1, Complex tapRatio2, Complex tapRatio3);
				IntPtr AddThreeWindingTransformer(IntPtr bus1, IntPtr bus2, IntPtr bus3,
					Complex impedance12, Complex impedance13, Complex impedance23,
					Complex tapRatio1, Complex tapRatio2, Complex tapRatio3)
				{
					return AddThreeWindingTransformer(bus1, bus2, bus3, impedance12, impedance13, impedance23, 0, tapRatio1, tapRatio2, tapRatio3);
				}
			public:
				NetworkCase();
				!NetworkCase();
				~NetworkCase();
			};
		}

		namespace PowerFlow
		{
			public enum class SolverType : byte
			{
				NewtonRaphson = 0,
				FastDecoupled = 1
			};

			// 求解最终的结论
			public enum class SolutionStatus : byte
			{
				Success = 0x00,					//求解成功结束。
				MaxIteration = 0x10,			//已经达到最大允许的迭代次数。
				IterationFailed = 0x11,			//迭代过程出现问题。
				IntelliIterationAbort = 0x12,	//启用智能迭代后，由于运算过程不收敛而被中断。
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
			public:
				Solution(_NATIVE_PF Solution* native);
				!Solution();
				~Solution();
			};

			/// <summary>
			/// 用于完成稳态潮流的求解过程。
			/// （<see cref="PowerSolutions.ObjectModel.NetworkCase" />）
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
