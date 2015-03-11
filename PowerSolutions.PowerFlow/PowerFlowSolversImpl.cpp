#include "stdafx.h"
#include "PowerFlowSolversImpl.h"
#include "PowerFlowSolution.h"
#include "Exceptions.h"
#include <algorithm>
#include <complex>
#include <cmath>

using namespace std;
using namespace PowerSolutions::ObjectModel;

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

		Solution* SolverImpl::Solve(NetworkCase* caseInfo)
		{
			assert(caseInfo);
			assert(MaxDeviationTolerance() >= 0);
			PNetwork = new PrimitiveNetworkImpl(*caseInfo);
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
			s->NodeCount(PNetwork->NodeCount);
			s->PQNodeCount(PNetwork->PQNodeCount);
			s->PVNodeCount(PNetwork->PVNodeCount);
			s->SlackNode(PNetwork->SlackNode->Bus);
			//根据注入功率和负载情况计算节点出力信息。
			for (auto& node : PNetwork->Nodes)
			{
				complexd PowerGeneration(node->ActivePowerInjection, node->ReactivePowerInjection);
				complexd PowerConsumption;
				auto range = PNetwork->BusComponents().equal_range(node->Bus);
				for_each(range.first, range.second,
					[&](pair<const Bus*, Component*> p){
					//对于PQ负载
					PQLoad *pqload = dynamic_cast<PQLoad*>(p.second);
					if (pqload != nullptr)
					{
						PowerGeneration += pqload->Power();
						PowerConsumption += pqload->Power();
					}
					//对于接地导纳
					ShuntAdmittance *sa = dynamic_cast<ShuntAdmittance*>(p.second);
					if (sa != nullptr)
					{
						//注意此部分功率不应累加至出力功率
						// S = U^2 * conj(Y)
						//BUG FIXED:电压没有取平方
						PowerConsumption += node->Voltage * node->Voltage * conj(sa->Admittance());
					}
				});
				s->AddNodeFlow(node->Bus,
					NodeFlowSolution(node->VoltagePhasor(), PowerGeneration, PowerConsumption, node->Degree));
				totalPowerGeneration += PowerGeneration;
				totalPowerConsumption += PowerConsumption;
			}
			//根据节点电压计算支路功率。
			for (auto& c : PNetwork->Components())
			{
				//仅适用于双端元件。
				DoublePortComponent *dpc = dynamic_cast<DoublePortComponent*>(c);
				if (dpc != NULL)
				{
					auto node1 = PNetwork->BusMapping[dpc->Bus1()],
						node2 = PNetwork->BusMapping[dpc->Bus2()];
					complexd power1, power2, shunt1, shunt2;
					auto pieqv = dpc->PiEquivalency();
#define _EvalPower(v1, a1, v2, a2, y10, z12) \
			conj((v1)*(v1)*(y10) + (v1)*((v1) - polar((v2), (a2)-(a1)))/(z12))	//以 v1 作为参考相量
#define _EvalShuntPower(v, y10) (v) * (v) * conj(y10)
					power1 = _EvalPower(node1->Voltage, node1->Angle,
						node2->Voltage, node2->Angle, pieqv.Admittance1(), pieqv.Impedance());
					power2 = _EvalPower(node2->Voltage, node2->Angle,
						node1->Voltage, node1->Angle, pieqv.Admittance2(), pieqv.Impedance());
					//注意，计算支路功率时
					//对于变压器，不能将π型等值电路两侧的接地导纳拆开计算。
					//只能按照Γ型等值电路进行计算。
					auto tf = dynamic_cast<Transformer*>(c);
					if (tf != nullptr)
					{
						shunt1 = _EvalShuntPower(node1->Voltage, tf->Admittance());
						shunt2 = 0;
					} else {
						shunt1 = _EvalShuntPower(node1->Voltage, pieqv.Admittance1());
						shunt2 = _EvalShuntPower(node2->Voltage, pieqv.Admittance2());
					}
#undef _EvalPower
#undef _EvalShuntPower
					//追加元件潮流项
					BranchFlowSolution branchFlow(power1, power2, shunt1, shunt2);
					s->AddComponentFlow(c, branchFlow);
					//增加/追加支路潮流
					s->AddBranchFlow(dpc->Bus1(), dpc->Bus2(), branchFlow);
					totalPowerLoss += branchFlow.PowerLoss();
					totalPowerShunt += branchFlow.PowerShunt();
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

