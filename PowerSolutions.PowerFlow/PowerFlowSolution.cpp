#include "stdafx.h"
#include "PowerFlowSolution.h"
#include "ObjectModel.h"
#include "PowerFlowSolvers.h"
#include <utility>

using namespace PowerSolutions::ObjectModel;
using namespace std;

namespace PowerSolutions
{
	namespace PowerFlow
	{
		Solution::Solution()
		{ }

		void Solution::AddNodeFlow(ObjectModel::Bus* node, const NodeEvaluationStatus& status)
		{
			auto result = m_NodeFlow.emplace(node, NodeFlowSolution(status.VoltagePhasor(), status.PowerInjection()));
			assert(result.second);
/*
			m_TotalPowerGeneration += result.first->second.PowerGeneration();
			m_TotalPowerConsumption += result.first->second.PowerConsumption();*/
		}

		void Solution::AddComponentFlow(ObjectModel::Component* c, const ComponentFlowSolution& solution)
		{
			auto result = m_ComponentFlow.emplace(c, solution);
			assert(result.second);
			if (solution.IsUnconstrained() == false && c->PortCount() == 1)
			{
				//����PV�����/ƽ�ⷢ���������Ҳ��Ҫ�޸� PowerGeneration �� PowerConsumption
				// ʦ�֣��ӵز�����Ӧ���븺�ɡ�
				//PQ�����൱��ע�루��������ʣ�powerInjection[0] = 0, powerInjection[1] = -SLoad
				//���ڽӵص��ɣ�powerInjection[0] = Ssa, powerInjection[1] = -Ssa
				m_NodeFlow.at(c->Buses(0)).AddPowerGeneration(solution.PowerShunt() + solution.PowerInjections(0));
				m_NodeFlow.at(c->Buses(0)).AddPowerConsumption(-solution.PowerInjections(0));
			}
		}

		void Solution::AddBranchFlow(Bus* node1, Bus* node2, const BranchFlowSolution& solution)
		{
			//�����ۼ�
			auto nodePair = make_pair(node1, node2);
			auto existing = m_BranchFlow.find(nodePair);
			if (existing != m_BranchFlow.end())
			{
				//��Ҫ�ۼ�
				existing->second += solution;
			} else {
				auto result = m_BranchFlow.emplace(nodePair, solution);
				assert(result.second);
			}
		}

		void Solution::IterationCount(int val)
		{
			m_IterationCount = val;
		}

	}
}