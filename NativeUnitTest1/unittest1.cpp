#include "stdafx.h"
#include "CppUnitTest.h"

#include "PowerSolutions/PowerFlowObjectModel.h"
#include "PowerSolutions/NetworkCase.h"
#include "PowerSolutions/PowerFlowSolvers.h"
#include <iostream>
#include <memory>
#include <crtdbg.h>
#include "../PowerSolutions.PowerFlow/Export/PowerSolutions/PrimitiveNetwork.h"

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
		TEST_METHOD(NativeNetworkCaseCloneTest1)
		{
			//Clone Test
			_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
			NetworkCase network;
			stringstream ss;
			auto b1 = network.AddBus(),
				b2 = network.AddBus(),
				b3 = network.AddBus(),
				b4 = network.AddBus();
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
				new ThreeWindingTransformer(b1, b2, b3, 1, 2, 3, 5, 1, 0.5, 0.3)
			});
			//��¡����
			auto info = network.CorrespondenceClone();
			for (auto& c : network.Objects())
			{
				ss << "Proto : " << typeid(*c).name() << ", " << c << "-->" << info.second->CloneOf(c) << endl;
			}
			for (auto& c : info.first->Objects())
			{
				ss << "Clone : " << typeid(*c).name() << ", " << c << "-->" << info.second->PrototypeOf(c) << endl;
			}
			Logger::WriteMessage(ss.str().c_str());
		}

		TEST_METHOD(NativePowerFlowTest1)
		{
			// TODO:  �ڴ�������Դ���?
			_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
			NetworkCase network;
			auto b1 = network.AddBus(),
				b2 = network.AddBus(),
				b3 = network.AddBus(),
				b4 = network.AddBus();
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
				//new ThreeWindingTransformer(b1, b2, b3, 0.001, 0.002, 0.003, 0.005, 1, 1.1, 1.5)
			});
			//for (int i = 0; i < 1000; i++)
			//	auto nc2 = shared_ptr<NetworkCase>(network.Clone());
			shared_ptr<Solver> solver(Solver::Create(SolverType::NewtonRaphson));
			shared_ptr<Solver> dcsolver(Solver::Create(SolverType::DcPowerFlow));
			solver->MaxDeviationTolerance(1e-12);
			auto s = solver->Solve(network);
			stringstream ss;
			auto printSolution = [&]() {
				ss << s->IterationCount() << "times , maxDev = " << (int)s->MaxDeviation() << endl;
				ss << "Node" << endl;
				for (auto& item : s->NodeFlow())
				{
					ss << item.first << '\t'
						<< abs(item.second.Voltage()) << '\t'
						<< arg(item.second.Voltage()) << '\t'
						<< item.second.PowerGeneration().real() << '\t'
						<< item.second.PowerConsumption().real() << '\t'
						<< endl;
				}
				ss << "Component" << endl;
				for (auto& item : s->ComponentFlow())
				{
					ss << item.first << "\t->\t";
					for (auto& pi : item.second.PowerInjections())
						ss << pi.real() << '\t';
					ss << endl;
				}
				ss << endl;
			};
			ss << "Newton Raphson" << endl;
			printSolution();
			ss << "DC Power Flow" << endl;
			s = dcsolver->Solve(network);
			printSolution();
			Logger::WriteMessage(ss.str().c_str());
		}

		// �������ѹ�����ԡ�
		TEST_METHOD(NativeTwtPowerFlowTest1)
		{
			/*
					Dim Base110 As New PerUnitBase(110, 100)
					Dim Base35 As New PerUnitBase(35, 100)
					Dim Base10 As New PerUnitBase(10, 100)
					Network.MaxDeviation = 0.0001
					With Network.Buses
					Dim b1 = .Add("1-PV", 1, Base10),
					b2 = .Add("2-Pri", 1, Base110),
					b3 = .Add("3-Load", 1, Base110),
					b4 = .Add("4-Slack", 1, Base35),
					bT = .Add("5-TwT", 1, Base110)
					ThreeWindingTransformer.Create(b2, b4, b1,
					0.01, 0.105, 0.01, 0.17, 0.01, 0.065, 1,
					110 / 35, 110 / 10, New PerUnitBase(110, 100))
					PVGenerator.Create(b1, 0.8, 1, New PerUnitBase(10, 100))
					SlackGenerator.Create(b4, 35)
					PQLoad.Create(b3, 0.8, 0.6, New PerUnitBase(110, 100))
					Line.Create(b2, b3, 0.01, 0.1, 0.001, New PerUnitBase(110, 200))
					End With

					# ��������
					Attribute.Locale zh-CN
					Attribute.Name ���������?
					Attribute.Annotation Generated by PowerFlowSolver.Interop
					Attribute.Solver NR
					Attribute.NodeReorder True
					Attribute.MaxIterations 20
					Attribute.MaxDeviation 0.0001

					# ĸ���б�
					Bus 0,1-PV,10
					Bus 1,2-Pri,110
					Bus 2,3-Load,110
					Bus 3,4-Slack,35
					Bus 4,5-TwT,110
					# �Զ����ɵ�ĸ��
					Bus 5,TwT-2-Pri,110

					# Ԫ���б�
					# �������ѹ��������Ϊ�����������ѹ��
					# ��ȷֱ��? 1, 0.318181818181818, 0.0909090909090909
					PVGenerator 0,80,10
					SlackGenerator 3,35
					PQLoad 2,80,60
					Line 1,2,0.605,6.05,1.65289256198347E-05
					# �Զ����ɵ�Ԫ��
					Transformer 1,5,0.605,12.705,0,0,1
					Transformer 3,5,0.06125,1.01239669421488E-08,0,0,0.318181818181818
					Transformer 0,5,0.005,0.065,0,0,0.0909090909090909

					ĸ��            ��ѹ       ��λ     �й�����   �޹�����   �й�����   �޹�����  ֧·��
					[p.u.]     [ ��]      [p.u.]     [p.u.]     [p.u.]     [p.u.]
					[#0]1-PV        10         3.244665   80        -3.916739      -          -           1
					[#1]2-Pri        100.6982  -4.847346      -          -          -          -           2
					[#2]3-Load       96.32472  -7.4931     4.85e-009     -       80         60             1
					[#3]4-Slack       35            -       2.011347   88.1072       -          -           1
					[#4]5-TwT       ĸ��δ����ϵͳ��
					[#5]TwT-2-Pri      109.99     0.2524323     -          -          -          -           3
			*/
			_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
			NetworkCase network;
			auto b1 = network.AddBus(10),
				b2 = network.AddBus(110),
				b3 = network.AddBus(110),
				b4 = network.AddBus(35);
			network.AddObject({
				new PVGenerator(b1, 80, 10),
				new SlackGenerator(b4, 35),
				new PQLoad(b3, complexd(80, 60)),
				new Line(b2, b3, complexd(0.605, 6.05), complexd(0, 1.65289256198347E-05)),
				new ThreeWindingTransformer(b2, b4, b1, complexd(1.21, 12.705), complexd(1.21, 20.57),
				complexd(1.21, 7.865), 0, 1, 1 / 0.318181818181818, 1 / 0.0909090909090909)
			});
			shared_ptr<Solver> solver(Solver::Create(SolverType::NewtonRaphson));
			solver->MaxDeviationTolerance(1e-12);
			auto s = solver->Solve(network);
			stringstream ss;
			ss << s->IterationCount() << "times , maxDev = " << (int)s->MaxDeviation() << endl;
			for (auto& item : s->NodeFlow())
			{
				ss << item.first << '\t' << abs(item.second.Voltage()) << '\t' << arg(item.second.Voltage()) << endl;
			}
			Logger::WriteMessage(ss.str().c_str());
		}

		TEST_METHOD(NativePrimitiveNetworkTest)
		{
			// TODO:  �ڴ�������Դ���
			_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
			NetworkCase network;
			auto b1 = network.AddBus(),
				b2 = network.AddBus(),
				b3 = network.AddBus(),
				b4 = network.AddBus();
			network.AddObject({
				new Transformer(b1, b2, complexd(0, 0.1666666666666666666666), 0.886363636363636),
				new PVGenerator(b3, 0.2, 1.05),
				new SlackGenerator(b4, 1.05),
				new PQLoad(b2, complexd(0.5, 0.3)),
				new PQLoad(b4, complexd(0.15, 0.1)),
				new ShuntAdmittance(b2, complexd(0, 0.05)),
				new Line(b4, b3, complexd(0.260331, 0.495868), complexd(0, 0.051728)),
				new Line(b1, b4, complexd(0.173554, 0.330579), complexd(0, 0.034486)),
				new Line(b1, b3, complexd(0.130165, 0.247934), complexd(0, 0.025864))//,
				//new ThreeWindingTransformer(b1, b2, b3, 0.001, 0.002, 0.003, 0.005, 1, 1.1, 1.5)
			});
			//����һ����ͬ�����硣
			shared_ptr<NetworkCase> network2(network.Clone());
			NetworkObject* sg;
			for (auto& obj : network2->Objects())
			{
				//������·�еĸ�������Ż�ԭ�����С�
				if (dynamic_cast<SlackGenerator*>(obj) == nullptr)
					network.AddObject(obj);
				else
					sg = obj;
			}
			network2->RemoveObject(sg);
			network2->AutoDeleteChildren(false);
			auto pn = network.ToPrimitive(PrimitiveNetworkOptions::ForceSetSlackNode);
			stringstream ss;
			auto PrintNetwork = [&ss](const PrimitiveNetwork& pn)
			{
				for (auto& node : pn.Nodes())
					ss << "Node #" << node->Index() << "\t" << node->Bus() << endl;
				for (auto& b : pn.Branches())
					ss << "Branch #" << b->Index() << "\t" <<
					b->Nodes().first->Bus() << " -- " << b->Nodes().second->Bus() << endl;
				ss << pn.Admittance << endl << endl;
			};
			ss << "ԭʼ����" << endl;
			PrintNetwork(*pn);
			auto subPN = pn->ConnectedSubnetworks();
			for (auto& sn : subPN)
			{
				ss << "������" << endl;
				PrintNetwork(*sn);
			}
			Logger::WriteMessage(ss.str().c_str());
		}

		TEST_METHOD(NativeDcPowerFlowTest1)
		{
			// TODO:  �ڴ�������Դ���
			_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
			NetworkCase network;
			auto b1 = network.AddBus(),
				b2 = network.AddBus(),
				b3 = network.AddBus();
			network.AddObject({
				new PQLoad(b1, 2),
				new PQLoad(b2, -0.5),
				new SlackGenerator(b3, 1),
				new Line(b1, b2,complexd(0, 0.2),0),
				new Line(b1, b3, complexd(0, 0.1),0),
				new Line(b2, b3, complexd(0, 0.2),0),
				//new ThreeWindingTransformer(b1, b2, b3, 0.001, 0.002, 0.003, 0.005, 1, 1.1, 1.5)
			});
			shared_ptr<Solver> dcsolver(Solver::Create(SolverType::DcPowerFlow));
			stringstream ss;
			auto s = dcsolver->Solve(network);
			auto printSolution = [&]() {
				ss << s->IterationCount() << "times , maxDev = " << (int)s->MaxDeviation() << endl;
				ss << "Node" << endl;
				for (auto& item : s->NodeFlow())
				{
					ss << item.first << '\t'
						<< abs(item.second.Voltage()) << '\t'
						<< arg(item.second.Voltage()) << '\t'
						<< item.second.PowerGeneration().real() << '\t'
						<< item.second.PowerConsumption().real() << '\t'
						<< endl;
				}
				ss << "Component" << endl;
				for (auto& item : s->ComponentFlow())
				{
					ss << typeid(*item.first).name() << "\t"
						<< item.first << "\t->\t";
					for (auto& pi : item.second.PowerInjections())
						ss << pi.real() << '\t';
					ss << endl;
				}
				ss << endl;
			};
			ss << "DC Power Flow" << endl;
			printSolution();
			Logger::WriteMessage(ss.str().c_str());
		}
	};
}