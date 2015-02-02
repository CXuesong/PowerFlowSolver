#include "stdafx.h"
#include "PowerFlow.h"

using namespace PowerSolutions::Interop::ObjectModel;

namespace PowerSolutions
{
	namespace Interop
	{
		namespace PowerFlow
		{
			Solution::Solution(_NATIVE_PF Solution* native)
				: nativeObject(native)
			{ }

			Solution::!Solution()
			{
				delete nativeObject;
				nativeObject = nullptr;
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
				nativeObject = nullptr;
			}

			Solver::~Solver()
			{
				this->!Solver();
			}

			Solution^ Solver::Solve(NetworkCase^ network)
			{
				return gcnew Solution(nativeObject->Solve(network->nativeObject));
			}

			NodeFlowSolution::NodeFlowSolution(const _NATIVE_PF NodeFlowSolution& native)
			{
				_WRAP_STRUCT_INIT(Voltage, MarshalComplex);
				_WRAP_STRUCT_INIT(PowerGeneration, MarshalComplex);
				_WRAP_STRUCT_INIT(PowerConsumption, MarshalComplex);
			}

			BranchFlowSolution::BranchFlowSolution(const _NATIVE_PF BranchFlowSolution& native)
			{
				_WRAP_STRUCT_INIT(Power1, MarshalComplex);
				_WRAP_STRUCT_INIT(Power2, MarshalComplex);
				_WRAP_STRUCT_INIT(ShuntPower1, MarshalComplex);
				_WRAP_STRUCT_INIT(ShuntPower2, MarshalComplex);
			}

		}
	}
}

