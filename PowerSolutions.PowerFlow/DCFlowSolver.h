/*
PowerSolutions
稳态潮流求解实现部分（直流潮流法）。
by Chen [CXuesong.], 2015
*/

#pragma once
#include "PowerFlowSolvers.h"

namespace PowerSolutions
{
	namespace PowerFlow
	{
		// 稳态潮流求解实现部分（直流法）。
		class DCFlowSolver : public Solver
		{
		public:
			std::shared_ptr<Solution> Solve(ObjectModel::NetworkCase& network) override;
			std::shared_ptr<Solution> Solve(ObjectModel::PrimitiveNetwork& network) override;
			~DCFlowSolver() override;
		};
	}
}