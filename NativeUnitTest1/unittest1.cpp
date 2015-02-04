#include "stdafx.h"
#include "CppUnitTest.h"

#include "PowerSolutions/PowerFlowObjectModel.h"
#include "PowerSolutions/NetworkCase.h"
#include "PowerSolutions/PowerFlowSolvers.h"
#include <memory>
#include <crtdbg.h>

using namespace PowerSolutions;
using namespace PowerSolutions::ObjectModel;
using namespace PowerSolutions::PowerFlow;
using namespace std;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace PowerSolutions
{
	const char TraceFilePath[] = "TraceFile.txt";
}

namespace NativeUnitTest1
{
	TEST_CLASS(UnitTest1)
	{
	public:
		TEST_METHOD(PowerFlowTestMethod1)
		{
			// TODO:  在此输入测试代码
			_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
			NetworkCase network;
			auto b1 = network.CreateBus(),
				b2 = network.CreateBus(),
				b3 = network.CreateBus(),
				b4 = network.CreateBus();
			network.AddObject({
				new Transformer(b1, b2, complexd(0, 0.1666666666666666666666), 0.886363636363636),
				new PVGenerator(b3, 0.2, 1.05),
				new SlackGenerator(b4, 1.05),
				new PQLoad(b2, complexd(0.5, 0.3)),
				new PQLoad(b4, complexd(0.15, 0.1)),
				new ShuntAdmittance(b2, complexd(0, 0.05)),
				new Line(b4, b3, complexd(0.260331, 0.495868), complexd(0, 0.051728)),
				new Line(b1, b4, complexd(0.173554, 0.330579), complexd(0, 0.034486)),
				new Line(b1, b3, complexd(0.130165, 0.247934), complexd(0, 0.025864)),
				/*new ThreeWindingTransformer(b1, b2, b3, 1, 2, 3, 5, 1, 0.5, 0.3),*/
			});
			//for (int i = 0; i < 1000; i++)
			//	auto nc2 = shared_ptr<NetworkCase>(network.Clone());
			shared_ptr<Solver> solver(Solver::Create(SolverType::NewtonRaphson));
			solver->MaxDeviationTolerance(1e-12);
			solver->NodeReorder(false);
			auto s = solver->Solve(&network);
			delete s;
			s = solver->Solve(&network);
			delete s;
		}
	};
}