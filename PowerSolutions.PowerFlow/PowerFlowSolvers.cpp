#include "stdafx.h"
#include "PowerFlowSolvers.h"
#include "NRSolver.h"
#include "Exceptions.h"

using namespace std;

namespace PowerSolutions
{
	namespace PowerFlow
	{
		Solver::Solver()
			: m_MaxIterations(10), m_MaxDeviationTolerance(0.01), 
			m_NodeReorder(true), m_IntelliIterations(true), m_IterationEvent(nullptr)
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
	}
}