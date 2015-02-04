#include "stdafx.h"
#include "PowerFlow.h"
#include "ObjectModel.h"

using namespace PowerSolutions::Interop::ObjectModel;

namespace PowerSolutions
{
	namespace Interop
	{
		namespace PowerFlow
		{
			Solution::Solution(const _NATIVE_PF Solution& native)
				: m_NodeFlow(gcnew Dictionary<Bus, NodeFlowSolution>(native.NodeFlow().size())),
				m_ComponentFlow(gcnew Dictionary<Component, BranchFlowSolution>(native.BranchFlow().size())),
				m_BranchFlow(gcnew Dictionary<BusPair, BranchFlowSolution>(native.ComponentFlow().size(), BusPairUnorderedComparer::Default)),
				m_s_NodeFlow(gcnew ReadOnlyDictionary<Bus, NodeFlowSolution>(m_NodeFlow)),
				m_s_ComponentFlow(gcnew ReadOnlyDictionary<Component, BranchFlowSolution>(m_ComponentFlow)),
				m_s_BranchFlow(gcnew ReadOnlyDictionary<BusPair, BranchFlowSolution>(m_BranchFlow))
			{
				//将解缓存至此实例中。
				_INIT_PROPERTY_CACHE(TotalPowerGeneration, MarshalComplex);
				_INIT_PROPERTY_CACHE(TotalPowerConsumption, MarshalComplex);
				_INIT_PROPERTY_CACHE(TotalPowerLoss, MarshalComplex);
				_INIT_PROPERTY_CACHE(TotalPowerShunt, MarshalComplex);
				_INIT_PROPERTY_CACHE(IterationCount, );
				_INIT_PROPERTY_CACHE(MaxDeviation, );
				_INIT_PROPERTY_CACHE(Status, SolutionStatus);
				_INIT_PROPERTY_CACHE(NodeCount, int);
				_INIT_PROPERTY_CACHE(PQNodeCount, int);
				_INIT_PROPERTY_CACHE(PVNodeCount, int);
				_INIT_PROPERTY_CACHE(SlackNode, Bus);
				for (auto& item : native.NodeFlow())
					m_NodeFlow->Add(Bus(item.first), NodeFlowSolution(item.second));
				for (auto& item : native.ComponentFlow())
					m_ComponentFlow->Add(Component(item.first), BranchFlowSolution(item.second));
				for (auto& item : native.BranchFlow())
					m_BranchFlow->Add(BusPair(item.first), BranchFlowSolution(item.second));
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
				_NATIVE_PF Solution* nativeSolution;
				_WRAP_EXCEPTION_BOUNDARY(
					nativeSolution = nativeObject->Solve(network->nativeObject);
				);
				auto solution = gcnew Solution(*nativeSolution);
				delete nativeSolution;
				return solution;
			}

			NodeFlowSolution::NodeFlowSolution(const _NATIVE_PF NodeFlowSolution& native)
			{
				_INIT_PROPERTY_CACHE(Voltage, MarshalComplex);
				_INIT_PROPERTY_CACHE(PowerGeneration, MarshalComplex);
				_INIT_PROPERTY_CACHE(PowerConsumption, MarshalComplex);
			}

			BranchFlowSolution::BranchFlowSolution(const _NATIVE_PF BranchFlowSolution& native)
			{
				_INIT_PROPERTY_CACHE(Power1, MarshalComplex);
				_INIT_PROPERTY_CACHE(Power2, MarshalComplex);
				_INIT_PROPERTY_CACHE(ShuntPower1, MarshalComplex);
				_INIT_PROPERTY_CACHE(ShuntPower2, MarshalComplex);
			}

		}
	}
}

