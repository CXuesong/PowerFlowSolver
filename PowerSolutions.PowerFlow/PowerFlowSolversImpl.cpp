#include "stdafx.h"
#include "PowerFlowSolversImpl.h"
#include "PowerFlowSolution.h"
#include "PrimitiveNetwork.h"
#include "Exceptions.h"
#include "Utility.h"
#include <algorithm>
#include <complex>
#include <cmath>

using namespace std;
using namespace PowerSolutions::ObjectModel;
using namespace PowerSolutions::Utility;

namespace PowerSolutions
{
	namespace PowerFlow
	{
		SolverImpl::SolverImpl()
		{
		}

		SolverImpl::~SolverImpl()
		{
		}

		Solution* SolverImpl::Solve(PrimitiveNetwork& network)
		{
			assert(MaxDeviationTolerance() >= 0);
			PNetwork = &network;
			PQNodeCount = PNetwork->PQNodes.size();
			PVNodeCount = PNetwork->PVNodes.size();
			NodeCount = PNetwork->Nodes.size();
			BeforeIterations();
			for (int i = 0; i <= MaxIterations(); i++)
			{
				//也就是说，i 就是“已经完成”的迭代次数
				//即实际循环次数比“迭代次数”多1
				_PS_TRACE("迭代次数：" << i);
				double dev = abs(EvalDeviation());
				//回调函数
				if (IterationEvent() != nullptr)
				{
					IterationEventArgs e(i, dev);
					IterationEvent()(this, &e);
				}
				if (dev < MaxDeviationTolerance())
					return GenerateSolution(SolutionStatus::Success, i, dev);
				//最后一次（多出来的一次）循环仅仅计算一下功率偏差。
				if (i < MaxIterations())
				{
					if (!OnIteration())
						return GenerateSolution(SolutionStatus::IterationFailed, i, dev);
					if (IntelliIterations())
					{
						//TODO 根据二次收敛特性进行判断
						//也就是说需要在派生类中判断
						//此处只是作出一个粗略的判断而已
						if (i > 3 && dev > 1E10)
							return GenerateSolution(SolutionStatus::IntelliIterationAbort, i, dev);
					}
				}
			}
			//达到最大迭代次数，且未收敛。
			return GenerateSolution(SolutionStatus::MaxIteration, MaxIterations(), EvalDeviation());
		}

		Solution* SolverImpl::GenerateSolution(SolutionStatus status, int iterCount, double maxDev)
		{
			AfterIterations();	//收尾工作
			//注意：下面的工作不能引发异常，否则会由s引发内存泄漏。
			auto s = new Solution();
			complexd totalPowerGeneration, totalPowerConsumption,
				totalPowerLoss, totalPowerShunt;
			s->Status(status);
			s->IterationCount(iterCount);
			s->MaxDeviation(maxDev);
			s->NodeCount(PNetwork->Nodes.size());
			s->PQNodeCount(PNetwork->PQNodes.size());
			s->PVNodeCount(PNetwork->PVNodes.size());
			s->SlackNode(PNetwork->SlackNode->Bus);
			//根据注入功率和负载情况计算节点出力信息。
			for (auto& node : PNetwork->Nodes)
			{
				complexd PowerGeneration(node->ActivePowerInjection, node->ReactivePowerInjection);
				complexd PowerConsumption;
				for (auto& c : PNetwork->BusMapping[node->Bus]->Components)
				{
					auto powerInjection = c->EvalPowerInjection(PNetwork);
					//对于PV发电机/平衡发电机，无需也不要修改 PowerGeneration 和 PowerConsumption
					if (powerInjection.size() > 0)
					{
						assert(powerInjection.size() == 2);
						// 师兄：接地补偿不应计入负荷。
						//PQ负载相当于注入（抽出）功率，powerInjection[0] = 0, powerInjection[1] = -SLoad
						//对于接地导纳，powerInjection[0] = Ssa, powerInjection[1] = -Ssa
						PowerGeneration -= powerInjection[0] + powerInjection[1];
						PowerConsumption -= powerInjection[1];
					}
				}
				s->AddNodeFlow(node->Bus,
					NodeFlowSolution(node->VoltagePhasor(), PowerGeneration, PowerConsumption, node->Degree));
				totalPowerGeneration += PowerGeneration;
				totalPowerConsumption += PowerConsumption;
			}
			//根据节点电压计算支路功率。
			for (auto& obj : PNetwork->SourceNetwork()->Objects())
			{
				auto c = dynamic_cast<Component*>(obj);
				if (c != nullptr)
				{
					vector<complexd> powerInjection = c->EvalPowerInjection(PNetwork);
					if (powerInjection.size() > 0)
					{
						s->AddComponentFlow(c, ComponentFlowSolution(powerInjection));
						//仅适用于双端元件。
						DoublePortComponent *dpc = dynamic_cast<DoublePortComponent*>(c);
						if (dpc != nullptr)
						{
							//追加元件潮流项
							BranchFlowSolution branchFlow(-powerInjection[1], -powerInjection[2], powerInjection[0]);
							//增加/追加支路潮流
							s->AddBranchFlow(dpc->Bus1(), dpc->Bus2(), branchFlow);
						}
						if (c->PortCount() > 1)
						{
							assert(powerInjection.size() > 2);
							totalPowerShunt += powerInjection[0];
							//对于多端口元件，注入功率之和就是串联损耗。
							//注意功率的参考方向是流入母线的。
							for (auto i = powerInjection.size() - 1; i > 0; i--)
								totalPowerLoss -= powerInjection[i];
						}
					}
				}
			}
			s->TotalPowerGeneration(totalPowerGeneration);
			s->TotalPowerConsumption(totalPowerConsumption);
			s->TotalPowerLoss(totalPowerLoss);
			s->TotalPowerShunt(totalPowerShunt);
			return s;
		}
	}
}

