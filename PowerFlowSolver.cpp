#include "PowerFlowSolvers.h"
#include "PowerFlowSolversImpl.h"

namespace PowerSolutions
{
	namespace PowerFlow
	{
		Solver::Solver()
			: m_MaxIterations(10), m_MaxDeviationTolerance(0.01), m_NodeReorder(true), m_IntelliIterations(true)
		{ }

		Solver::~Solver()
		{ }


	}
}