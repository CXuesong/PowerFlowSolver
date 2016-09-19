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

		std::shared_ptr<Solution> SolverImpl::Solve(ObjectModel::PrimitiveNetwork& network)
		{
			assert(MaxDeviationTolerance() >= 0);
			PNetwork = &network;
			PQNodeCount = PNetwork->PQNodes().size();
			PVNodeCount = PNetwork->PVNodes().size();
			NodeCount = PNetwork->Nodes().size();
			PSolution = make_shared<PrimitiveSolution>(network);
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

		shared_ptr<Solution> SolverImpl::GenerateSolution(SolutionStatus status, int iterCount, double maxDev)
		{
			AfterIterations();	//收尾工作
			//注意：下面的工作不能引发异常，否则会由s引发内存泄漏。
			//不一定吧？
			//注意由于 Solution 的构造函数是 protected ，因此无法使用 make_shared。
			shared_ptr<Solution> s(new Solution());
			s->Status(status);
			s->IterationCount(iterCount);
			s->MaxDeviation(maxDev);
			s->NodeCount(PNetwork->Nodes().size());
			s->PQNodeCount(PNetwork->PQNodes().size());
			s->PVNodeCount(PNetwork->PVNodes().size());
			s->SlackNode(PNetwork->SlackNode()->Bus());
			//根据注入功率和负载情况计算节点出力信息。
			for (auto& node : PNetwork->Nodes()) s->AddNodeFlow(node->Bus(), PSolution->NodeStatus(node->Index()));
			for (auto& obj : PNetwork->SourceNetwork()->Objects())
			{
				auto c = dynamic_cast<Component*>(obj);
				if (c != nullptr)
				{
					//计算元件的潮流。
					auto cflow = c->EvalComponentFlow(*PSolution);
					s->AddComponentFlow(c, move(cflow));
				}
			}
			complexd totalPowerGeneration, totalPowerConsumption,
				totalPowerLoss, totalPowerShunt;
			for (auto& node : s->NodeFlow())
			{
				totalPowerGeneration += node.second.PowerGeneration();
				totalPowerConsumption += node.second.PowerConsumption();
			}
			//根据节点电压计算支路功率。
			for (auto& c : s->ComponentFlow())
			{
				//仅适用于双端元件。
				auto dpc = dynamic_cast<const DoublePortComponent*>(c.first);
				if (dpc != nullptr)
				{
					BranchFlowSolution branchFlow(-c.second.PowerInjections(0),
						-c.second.PowerInjections(1), c.second.PowerShunt());
					//增加/追加支路潮流
					s->AddBranchFlow(dpc->Bus1(), dpc->Bus2(), branchFlow);
				}
				//统计。
				//统计过程可以包含三绕组变压器。
				if (c.first->PortCount() > 1)
				{
					totalPowerShunt += c.second.PowerShunt();
					//对于多端口元件，注入功率之和就是串联损耗。
					//注意功率的参考方向是流入元件的。
					for (auto& inj : c.second.PowerInjections())
						totalPowerLoss += inj;
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

