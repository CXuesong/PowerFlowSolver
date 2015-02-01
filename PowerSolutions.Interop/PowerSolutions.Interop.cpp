// 这是主 DLL 文件。
#include "stdafx.h"
#include "PowerSolutions.Interop.h"
#include "PowerSolutions/PowerFlowObjectModel.h"
#include "PowerSolutions/NetworkCase.h"
#include "PowerSolutions/PowerFlowSolvers.h"

using namespace PowerSolutions;
using namespace PowerSolutions::ObjectModel;
using namespace PowerSolutions::PowerFlow;

void PowerSolutions::Interop::Class1::TestWorkflow()
{
	/*
	Bus  1, 环网-终端, 1.00
	Bus  2, 终端, 1.00
	Bus  3, 环网-右侧, 1.05
	Bus  4, 环网-左侧, 1.05
	#Bus  5, Test1, 1
	#Bus  6, Test2, 1
	#L 5,6,	0.260331, 0.495868, 0.051728
	T  1,2,	0, 0.1666666666666666666666, 0.886363636363636
	PVG  3,	0.2, 1.05
	SG  4,	1.05
	PQL  2,	0.5, 0.3
	PQL  4,	0.15, 0.1
	SA  2,	0, 0.05
	L  4,3,	0.260331, 0.495868, 0.051728
	L  1,4,	0.173554, 0.330579, 0.034486
	L  1,3,	0.130165, 0.247934, 0.025864
	*/
	NetworkCase network;
	auto b1 = network.CreateBus(),
		b2 = network.CreateBus(),
		b3 = network.CreateBus(),
		b4 = network.CreateBus(),
		b5 = network.CreateBus();
	Transformer::Create(b1, b2, complexd(0, 0.1666666666666666666666), 0.886363636363636);
	PVGenerator::Create(b3, 0.2, 1.05);
	SlackGenerator::Create(b4, 1.05);
	PQLoad::Create(b2, complexd(0.5, 0.3));
	PQLoad::Create(b4, complexd(0.15, 0.1));
	ShuntAdmittance::Create(b2, complexd(0, 0.05));
	Line::Create(b4, b3, complexd(0.260331, 0.495868), complexd(0, 0.051728));
	Line::Create(b1, b4, complexd(0.173554, 0.330579), complexd(0, 0.034486));
	Line::Create(b1, b3, complexd(0.130165, 0.247934), complexd(0, 0.025864));

	auto solver = Solver::Create(SolverType::NewtonRaphson);
	solver->Solve(&network);
	delete solver;
}
