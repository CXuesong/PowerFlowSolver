#include "stdafx.h"
#include "PowerFlow.h"
#include "ObjectModel.h"
#include "Prompts.h"

using namespace System::Runtime::InteropServices;
using namespace PowerSolutions::Interop::ObjectModel;
using namespace std;

namespace PowerSolutions
{
	namespace Interop
	{
		namespace PowerFlow
		{
			Solution::Solution(const _NATIVE_PF Solution& native)
				: m_NodeFlow(gcnew Dictionary<Bus, NodeFlowSolution>(native.NodeFlow().size())),
				m_ComponentFlow(gcnew Dictionary<Component, ComponentFlowSolution>(native.BranchFlow().size())),
				m_BranchFlow(gcnew Dictionary<BusPair, BranchFlowSolution>(native.ComponentFlow().size(), BusPairUnorderedComparer::Default)),
				m_s_NodeFlow(gcnew ReadOnlyDictionary<Bus, NodeFlowSolution>(m_NodeFlow)),
				m_s_ComponentFlow(gcnew ReadOnlyDictionary<Component, ComponentFlowSolution>(m_ComponentFlow)),
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
					m_ComponentFlow->Add(Component(item.first), ComponentFlowSolution(item.second));
				for (auto& item : native.BranchFlow())
					m_BranchFlow->Add(BusPair(item.first), BranchFlowSolution(item.second));
			}

			String^ Solution::ToString()
			{
				return Object::ToString();
			}

			Solver::Solver(SolverType type)
			{
				NodeReorder = true;
				switch (type)
				{
				case SolverType::NewtonRaphson:
				case SolverType::FastDecoupled:
					_WRAP_EXCEPTION_BOUNDARY(
						nativeObject = _NATIVE_PF Solver::Create((_NATIVE_PF SolverType)type);
					);
					m_Type = type;
					return;
				default:
					throw gcnew ArgumentException(nullptr, L"type");
				}
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

			String^ Solver::FriendlyName::get()
			{
				String^ name = Prompts::ResourceManager->GetString("SolverType." + m_Type.ToString());
				if (name == nullptr) return m_Type.ToString();
				return name;
			}

			Solution^ Solver::Solve(NetworkCase^ network)
			{
				shared_ptr<_NATIVE_PF Solution> nativeSolution;
				_WRAP_EXCEPTION_BOUNDARY(
					shared_ptr<_NATIVE_OM PrimitiveNetwork> nativePM =
						network->nativeObject->ToPrimitive(NodeReorder
							? _NATIVE_OM PrimitiveNetworkOptions::NodeReorder
							: _NATIVE_OM PrimitiveNetworkOptions::None);
					nativeSolution = nativeObject->Solve(nativePM);
				);
				auto solution = gcnew Solution(*nativeSolution);
				return solution;
			}

			void Solver::Iteration::add(IterationEventHandler^ name)
			{
				if (m_IterationEvent == nullptr)
				{
					if (NativeIterationEventProcAddress == IntPtr::Zero)
					{
						//将 delegate 声明在类中仅为了保持其生存期。
						NativeIterationEventDelegate = gcnew NativeIterationEventHandler(this, &Solver::NativeIterationEventProc);
						NativeIterationEventProcAddress = Marshal::GetFunctionPointerForDelegate(NativeIterationEventDelegate);
					}
					nativeObject->IterationEvent((_NATIVE_PF IterationEventHandler)(void*)NativeIterationEventProcAddress);
				}
				m_IterationEvent += name;
			}

			void Solver::Iteration::remove(IterationEventHandler^ name)
			{
				m_IterationEvent -= name;
				//在无事件监听时，撤销对底层对象的事件监听，以节省资源。
				if (m_IterationEvent == nullptr)
					nativeObject->IterationEvent(nullptr);
			}

			void Solver::NativeIterationEventProc(_NATIVE_PF Solver* sender, _NATIVE_PF IterationEventArgs* e)
			{
				Diagnostics::Debug::Assert(sender == nativeObject);
				Iteration(this, gcnew IterationEventArgs(*e));
			}

			NodeFlowSolution::NodeFlowSolution(const _NATIVE_PF NodeFlowSolution& native)
			{
				_INIT_PROPERTY_CACHE(Voltage, MarshalComplex);
				_INIT_PROPERTY_CACHE(PowerGeneration, MarshalComplex);
				_INIT_PROPERTY_CACHE(PowerConsumption, MarshalComplex);
				//TODO Set appropriate degree
				//_INIT_PROPERTY_CACHE(Degree, );
			}

			BranchFlowSolution::BranchFlowSolution(const _NATIVE_PF BranchFlowSolution& native)
			{
				_INIT_PROPERTY_CACHE(Power1, MarshalComplex);
				_INIT_PROPERTY_CACHE(Power2, MarshalComplex);
				_INIT_PROPERTY_CACHE(PowerShunt, MarshalComplex);
			}

			BranchFlowSolution::BranchFlowSolution(Complex power1, Complex power2, Complex powerShunt)
			{
				Power1 = power1;
				Power2 = power2;
				PowerShunt = powerShunt;
			}

			ComponentFlowSolution::ComponentFlowSolution(const _NATIVE_PF ComponentFlowSolution& native)
			{
				_INIT_PROPERTY_CACHE(PowerInjections, MarshalComplexArray);
				_INIT_PROPERTY_CACHE(PowerShunt, MarshalComplex);
				_INIT_PROPERTY_CACHE(IsUnconstrained, );
			}

			IterationEventArgs::IterationEventArgs(const _NATIVE_PF IterationEventArgs& native)
			{
				_INIT_PROPERTY_CACHE(IterationCount, );
				_INIT_PROPERTY_CACHE(MaxDeviation, );
			}

		}
	}
}

