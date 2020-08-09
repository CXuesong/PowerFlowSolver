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
			s->NodeCount(PNetwork->Nodes.size());
			s->PQNodeCount(PNetwork->PQNodes.size());
			s->PVNodeCount(PNetwork->PVNodes.size());
			s->SlackNode(PNetwork->SlackNode->Bus);
			//����ע�빦�ʺ͸����������ڵ������Ϣ��
			for (auto& node : PNetwork->Nodes)
			{
				complexd PowerGeneration(node->ActivePowerInjection, node->ReactivePowerInjection);
				complexd PowerConsumption;
				for (auto& c : PNetwork->BusMapping[node->Bus]->Components)
				{
					auto powerInjection = c->EvalPowerInjection(PNetwork);
					//����PV�����/ƽ�ⷢ���������Ҳ��Ҫ�޸� PowerGeneration �� PowerConsumption
					if (powerInjection.size() > 0)
					{
						assert(powerInjection.size() == 2);
						// ʦ�֣��ӵز�����Ӧ���븺�ɡ�
						//PQ�����൱��ע�루��������ʣ�powerInjection[0] = 0, powerInjection[1] = -SLoad
						//���ڽӵص��ɣ�powerInjection[0] = Ssa, powerInjection[1] = -Ssa
						PowerGeneration -= powerInjection[0] + powerInjection[1];
						PowerConsumption -= powerInjection[1];
					}
				}
				s->AddNodeFlow(node->Bus,
					NodeFlowSolution(node->VoltagePhasor(), PowerGeneration, PowerConsumption, node->Degree));
				totalPowerGeneration += PowerGeneration;
				totalPowerConsumption += PowerConsumption;
			}
			//���ݽڵ��ѹ����֧·���ʡ�
			for (auto& obj : PNetwork->SourceNetwork()->Objects())
			{
				auto c = dynamic_cast<Component*>(obj);
				if (c != nullptr)
				{
					vector<complexd> powerInjection = c->EvalPowerInjection(PNetwork);
					if (powerInjection.size() > 0)
					{
						s->AddComponentFlow(c, ComponentFlowSolution(powerInjection));
						//��������˫��Ԫ����
						DoublePortComponent *dpc = dynamic_cast<DoublePortComponent*>(c);
						if (dpc != nullptr)
						{
							//׷��Ԫ��������
							BranchFlowSolution branchFlow(-powerInjection[1], -powerInjection[2], powerInjection[0]);
							//����/׷��֧·����
							s->AddBranchFlow(dpc->Bus1(), dpc->Bus2(), branchFlow);
						}
						if (c->PortCount() > 1)
						{
							assert(powerInjection.size() > 2);
							totalPowerShunt += powerInjection[0];
							//���ڶ�˿�Ԫ����ע�빦��֮�;��Ǵ�����ġ�
							//ע�⹦�ʵĲο�����������ĸ�ߵġ�
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

