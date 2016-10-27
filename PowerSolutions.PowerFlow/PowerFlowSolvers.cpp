#include "stdafx.h"
#include "PowerFlowSolvers.h"
#include "NRSolver.h"
#include "Exceptions.h"
#include <PrimitiveNetwork.h>
#include "DCFlowSolver.h"

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
			case SolverType::DcPowerFlow:
				return new DCFlowSolver();
			}
			assert(false);
			return nullptr;
		}

		shared_ptr<Solution> Solver::Solve(const ObjectModel::NetworkCase& network)
		{
			auto pn = network.ToPrimitive(PrimitiveNetworkOptions::NodeReorder);
			return this->Solve(*pn);
		}

		PrimitiveSolution::PrimitiveSolution(const PrimitiveNetwork& network) 
			: m_Network(&network)
		{
			m_NodeStatus.reserve(network.Nodes().size());
			for (auto& node : network.Nodes())
				m_NodeStatus.push_back(NodeEvaluationStatus(*node));
		}
	}
}