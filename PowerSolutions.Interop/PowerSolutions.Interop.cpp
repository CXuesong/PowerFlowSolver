// 这是主 DLL 文件。
#include "stdafx.h"
#include "PowerSolutions.Interop.h"
#include "Utility.h"

#include <PowerSolutions/PowerFlowObjectModel.h>
#include <PowerSolutions/NetworkCase.h>
#include <PowerSolutions/PowerFlowSolvers.h>

using namespace PowerSolutions;
using namespace PowerSolutions::Interop::ObjectModel;
using namespace PowerSolutions::Interop::PowerFlow;

namespace PowerSolutions
{
	const char TraceFilePath[] = "TraceFile.txt";
}

namespace PowerSolutions
{
	namespace Interop
	{
		_NATIVE_OM Bus* MarshalBus(IntPtr ptr)
		{
			auto obj = MarshalPointer<_NATIVE_OM NetworkObject>(ptr);
			Diagnostics::Debug::Assert(dynamic_cast<_NATIVE_OM Bus*>(obj) != nullptr);
			return static_cast<_NATIVE_OM Bus*>(obj);
		}

		namespace ObjectModel
		{
			NetworkCase::NetworkCase()
				: nativeObject(new _NATIVE_OM NetworkCase)
			{
				nativeObject->AutoDeleteChildren(true);
			}

			NetworkCase::!NetworkCase()
			{
				delete nativeObject;
			}

			NetworkCase::~NetworkCase()
			{
				this->!NetworkCase();
			}

			void NetworkCase::AddObject(IntPtr obj)
			{
				nativeObject->AddObject(MarshalPointer<_NATIVE_OM NetworkObject>(obj));
			}

			IntPtr NetworkCase::AddBus(Complex initialVoltage)
			{
				return MarshalPointer(nativeObject->CreateBus(MarshalComplex(initialVoltage)));
			}

			IntPtr NetworkCase::AddLine(IntPtr bus1, IntPtr bus2, Complex impedance, Complex admittance)
			{
				auto newInst = new _NATIVE_OM Line(MarshalBus(bus1), MarshalBus(bus2), MarshalComplex(impedance), MarshalComplex(admittance));
				nativeObject->AddObject(newInst);
				return MarshalPointer(newInst);
			}

			IntPtr NetworkCase::AddPVGenerator(IntPtr bus1, double activePower, double voltage)
			{
				auto newInst = new _NATIVE_OM PVGenerator(MarshalBus(bus1), activePower, voltage);
				nativeObject->AddObject(newInst);
				return MarshalPointer(newInst);
			}

			IntPtr NetworkCase::AddSlackGenerator(IntPtr bus1, Complex voltage)
			{
				auto newInst = new _NATIVE_OM SlackGenerator(MarshalBus(bus1), MarshalComplex(voltage));
				nativeObject->AddObject(newInst);
				return MarshalPointer(newInst);
			}

			IntPtr NetworkCase::AddPQLoad(IntPtr bus1, Complex power)
			{
				auto newInst = new _NATIVE_OM PQLoad(MarshalBus(bus1), MarshalComplex(power));
				nativeObject->AddObject(newInst);
				return MarshalPointer(newInst);
			}

			IntPtr NetworkCase::AddShuntAdmittance(IntPtr bus1, Complex admittance)
			{
				auto newInst = new _NATIVE_OM ShuntAdmittance(MarshalBus(bus1), MarshalComplex(admittance));
				nativeObject->AddObject(newInst);
				return MarshalPointer(newInst);
			}

			IntPtr NetworkCase::AddTransformer(IntPtr bus1, IntPtr bus2, Complex impedance, Complex admittance, Complex tapRatio)
			{
				auto newInst = new _NATIVE_OM Transformer(
					MarshalBus(bus1), MarshalBus(bus2),
					MarshalComplex(impedance), MarshalComplex(admittance),
					MarshalComplex(tapRatio));
				nativeObject->AddObject(newInst);
				return MarshalPointer(newInst);
			}

			IntPtr NetworkCase::AddThreeWindingTransformer(IntPtr bus1, IntPtr bus2, IntPtr bus3, Complex impedance12, Complex impedance13, Complex impedance23, Complex admittance, Complex tapRatio1, Complex tapRatio2, Complex tapRatio3)
			{
				auto newInst = new _NATIVE_OM ThreeWindingTransformer(
					MarshalBus(bus1), MarshalBus(bus2), MarshalBus(bus3),
					MarshalComplex(impedance12), MarshalComplex(impedance13),
					MarshalComplex(impedance23), MarshalComplex(admittance),
					MarshalComplex(tapRatio1), MarshalComplex(tapRatio2),
					MarshalComplex(tapRatio3));
				nativeObject->AddObject(newInst);
				return MarshalPointer(newInst);
			}
		}

		namespace PowerFlow
		{
			Solution::Solution(_NATIVE_PF Solution* native)
				: nativeObject(native)
			{

			}

			Solution::!Solution()
			{
				delete nativeObject;
			}

			Solution::~Solution()
			{
				this->!Solution();
			}

			Solver::Solver(SolverType type)
				: nativeObject(_NATIVE_PF Solver::Create((_NATIVE_PF SolverType)type))
			{
				
			}

			Solver::!Solver()
			{
				delete nativeObject;
			}

			Solver::~Solver()
			{
				this->!Solver();
			}

			Solution^ Solver::Solve(NetworkCase^ network)
			{
				return gcnew Solution(nativeObject->Solve(network->nativeObject));
			}
		}
	}
}

#undef _NATIVE_OM