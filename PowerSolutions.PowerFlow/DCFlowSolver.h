/*
PowerSolutions
��̬�������ʵ�ֲ��֣�ֱ������������
by Chen [CXuesong.], 2015
*/

#pragma once
#include "PowerFlowSolvers.h"

namespace PowerSolutions
{
	namespace PowerFlow
	{
		// ��̬�������ʵ�ֲ��֣�ֱ��������
		class DCFlowSolver : public Solver
		{
		public:
			std::shared_ptr<Solution> Solve(ObjectModel::NetworkCase& network) override;
			std::shared_ptr<Solution> Solve(ObjectModel::PrimitiveNetwork& network) override;
			~DCFlowSolver() override;
		};
	}
}