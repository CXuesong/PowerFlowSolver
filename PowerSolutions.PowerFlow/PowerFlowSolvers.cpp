#include "stdafx.h"
#include "PowerFlowSolvers.h"
#include "NRSolver.h"
#include "Exceptions.h"
#include <PrimitiveNetwork.h>

using namespace std;
using namespace PowerSolutions::ObjectModel;

namespace PowerSolutions
{
	namespace PowerFlow
	{
		Solver::Solver()
			: m_MaxIterations(10), m_MaxDeviationTolerance(0.01),
			m_IntelliIterations(true), m_IterationEvent(nullptr)
		{ }

		Solver::~Solver()
		{ }

		Solver* Solver::Create(SolverType type)
		{
			switch (type)
			{
			case SolverType::NewtonRaphson:
				return new NRSolver();
			case SolverType::FastDecoupled:
				throw Exception(ExceptionCode::NotSupported);
			}
			assert(false);
			return nullptr;
		}

		Solution* Solver::Solve(ObjectModel::NetworkCase& network)
		{
			PrimitiveNetwork pn(network, true);
			return this->Solve(pn);
		}

		PrimitiveSolution::PrimitiveSolution(PrimitiveNetwork& network) 
			: m_Network(&network)
		{
			m_NodeStatus.reserve(network.Nodes().size());
			for (auto& node : network.Nodes())
				m_NodeStatus.push_back(NodeEvaluationStatus(*node));
		}
	}
}