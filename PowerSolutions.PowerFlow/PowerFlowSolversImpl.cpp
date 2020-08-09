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

		shared_ptr<Solution> SolverImpl::GenerateSolution(SolutionStatus status, int iterCount, double maxDev)
		{
			AfterIterations();	//��β����
			//ע�⣺����Ĺ������������쳣���������s�����ڴ�й©��
			//��һ���ɣ�
			//ע������ Solution �Ĺ��캯���� protected ������޷�ʹ�� make_shared��
			shared_ptr<Solution> s(new Solution());
			s->Status(status);
			s->IterationCount(iterCount);
			s->MaxDeviation(maxDev);
			s->NodeCount(PNetwork->Nodes().size());
			s->PQNodeCount(PNetwork->PQNodes().size());
			s->PVNodeCount(PNetwork->PVNodes().size());
			s->SlackNode(PNetwork->SlackNode()->Bus());
			//����ע�빦�ʺ͸����������ڵ������Ϣ��
			for (auto& node : PNetwork->Nodes()) s->AddNodeFlow(node->Bus(), PSolution->NodeStatus(node->Index()));
			for (auto& obj : PNetwork->SourceNetwork()->Objects())
			{
				auto c = dynamic_cast<Component*>(obj);
				if (c != nullptr)
				{
					//����Ԫ���ĳ�����
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
			//���ݽڵ��ѹ����֧·���ʡ�
			for (auto& c : s->ComponentFlow())
			{
				//��������˫��Ԫ����
				auto dpc = dynamic_cast<const DoublePortComponent*>(c.first);
				if (dpc != nullptr)
				{
					BranchFlowSolution branchFlow(-c.second.PowerInjections(0),
						-c.second.PowerInjections(1), c.second.PowerShunt());
					//����/׷��֧·����
					s->AddBranchFlow(dpc->Bus1(), dpc->Bus2(), branchFlow);
				}
				//ͳ�ơ�
				//ͳ�ƹ��̿��԰����������ѹ����
				if (c.first->PortCount() > 1)
				{
					totalPowerShunt += c.second.PowerShunt();
					//���ڶ�˿�Ԫ����ע�빦��֮�;��Ǵ�����ġ�
					//ע�⹦�ʵĲο�����������Ԫ���ġ�
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

