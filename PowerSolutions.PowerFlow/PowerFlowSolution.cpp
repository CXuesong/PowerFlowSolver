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

		void Solution::AddNodeFlow(Bus* node, const NodeEvaluationStatus& status)
		{
			auto result = m_NodeFlow.emplace(node, NodeFlowSolution(status.VoltagePhasor(), status.PowerInjection()));
			assert(result.second);
/*
			m_TotalPowerGeneration += result.first->second.PowerGeneration();
			m_TotalPowerConsumption += result.first->second.PowerConsumption();*/
		}

		void Solution::AddComponentFlow(Component* c, ComponentFlowSolution&& solution)
		{
			auto result = m_ComponentFlow.emplace(c, move(solution));
			assert(result.second);
			auto& localSolution = result.first->second;
			if (!localSolution.IsUnconstrained() && c->PortCount() == 1)
			{
				//����PV�����/ƽ�ⷢ���������Ҳ��Ҫ�޸� PowerGeneration �� PowerConsumption
				// ʦ�֣��ӵز�����Ӧ���븺�ɡ�
				//PQ�����൱��ע��أ����ĸ�ߣ��Ĺ��ʣ�powerShunt = 0, powerInjection[0] = -SLoad
				//���ڽӵص��ɣ�powerShunt = Ssa, powerInjection[0] = -Ssa
				auto& nf = m_NodeFlow.at(c->Buses(0));
				auto powerInj = localSolution.PowerInjections(0);
				nf.AddPowerGeneration(powerInj + localSolution.PowerShunt());
				nf.AddPowerConsumption(powerInj);
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

	}
}