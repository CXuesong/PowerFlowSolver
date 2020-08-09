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
				//Ҳ����˵��i ���ǡ��Ѿ���ɡ��ĵ�������
				//��ʵ��ѭ�������ȡ�������������1
				_PS_TRACE("����������" << i);
				double dev = abs(EvalDeviation());
				//�ص�����
				if (IterationEvent() != nullptr)
				{
					IterationEventArgs e(i, dev);
					IterationEvent()(this, &e);
				}
				if (dev < MaxDeviationTolerance())
					return GenerateSolution(SolutionStatus::Success, i, dev);
				//���һ�Σ��������һ�Σ�ѭ����������һ�¹���ƫ�
				if (i < MaxIterations())
				{
					if (!OnIteration())
						return GenerateSolution(SolutionStatus::IterationFailed, i, dev);
					if (IntelliIterations())
					{
						//TODO ���ݶ����������Խ����ж�
						//Ҳ����˵��Ҫ�����������ж�
						//�˴�ֻ������һ�����Ե��ж϶���
						if (i > 3 && dev > 1E10)
							return GenerateSolution(SolutionStatus::IntelliIterationAbort, i, dev);
					}
				}
			}
			//�ﵽ��������������δ������
			return GenerateSolution(SolutionStatus::MaxIteration, MaxIterations(), EvalDeviation());
		}

		Solution* SolverImpl::GenerateSolution(SolutionStatus status, int iterCount, double maxDev)
		{
			AfterIterations();	//��β����
			//ע�⣺����Ĺ������������쳣���������s�����ڴ�й©��
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
			//����ע�빦�ʺ͸����������ڵ������Ϣ��
			for (auto& node : PNetwork->Nodes)
			{
				complexd PowerGeneration(node->ActivePowerInjection, node->ReactivePowerInjection);
				complexd PowerConsumption;
				auto range = PNetwork->BusComponents().equal_range(node->Bus);
				for_each(range.first, range.second,
					[&](pair<const Bus*, Component*> p){
					//����PQ����
					PQLoad *pqload = dynamic_cast<PQLoad*>(p.second);
					if (pqload != nullptr)
					{
						PowerGeneration += pqload->Power();
						PowerConsumption += pqload->Power();
					}
					//���ڽӵص���
					ShuntAdmittance *sa = dynamic_cast<ShuntAdmittance*>(p.second);
					if (sa != nullptr)
					{
						//ע��˲��ֹ��ʲ�Ӧ�ۼ�����������
						// S = U^2 * conj(Y)
						//BUG FIXED:��ѹû��ȡƽ��
						PowerConsumption += node->Voltage * node->Voltage * conj(sa->Admittance());
					}
				});
				s->AddNodeFlow(node->Bus,
					NodeFlowSolution(node->VoltagePhasor(), PowerGeneration, PowerConsumption, node->Degree));
				totalPowerGeneration += PowerGeneration;
				totalPowerConsumption += PowerConsumption;
			}
			//���ݽڵ��ѹ����֧·���ʡ�
			for (auto& c : PNetwork->Components())
			{
				//��������˫��Ԫ����
				DoublePortComponent *dpc = dynamic_cast<DoublePortComponent*>(c);
				if (dpc != NULL)
				{
					auto node1 = PNetwork->BusMapping[dpc->Bus1()],
						node2 = PNetwork->BusMapping[dpc->Bus2()];
					complexd power1, power2, shunt1, shunt2;
					auto pieqv = dpc->PiEquivalency();
#define _EvalPower(v1, a1, v2, a2, y10, z12) \
			conj((v1)*(v1)*(y10) + (v1)*((v1) - polar((v2), (a2)-(a1)))/(z12))	//�� v1 ��Ϊ�ο�����
#define _EvalShuntPower(v, y10) (v) * (v) * conj(y10)
					power1 = _EvalPower(node1->Voltage, node1->Angle,
						node2->Voltage, node2->Angle, pieqv.Admittance1(), pieqv.Impedance());
					power2 = _EvalPower(node2->Voltage, node2->Angle,
						node1->Voltage, node1->Angle, pieqv.Admittance2(), pieqv.Impedance());
					//ע�⣬����֧·����ʱ
					//���ڱ�ѹ�������ܽ����͵�ֵ��·����Ľӵص��ɲ𿪼��㡣
					//ֻ�ܰ��զ��͵�ֵ��·���м��㡣
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
					//׷��Ԫ��������
					BranchFlowSolution branchFlow(power1, power2, shunt1, shunt2);
					s->AddComponentFlow(c, branchFlow);
					//����/׷��֧·����
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

