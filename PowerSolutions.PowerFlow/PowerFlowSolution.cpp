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
				//对于PV发电机/平衡发电机，无需也不要修改 PowerGeneration 和 PowerConsumption
				// 师兄：接地补偿不应计入负荷。
				//PQ负载相当于注入（抽出）功率，powerInjection[0] = 0, powerInjection[1] = -SLoad
				//对于接地导纳，powerInjection[0] = Ssa, powerInjection[1] = -Ssa
				m_NodeFlow.at(c->Buses(0)).AddPowerGeneration(solution.PowerShunt() + solution.PowerInjections(0));
				m_NodeFlow.at(c->Buses(0)).AddPowerConsumption(-solution.PowerInjections(0));
			}
		}

		void Solution::AddBranchFlow(Bus* node1, Bus* node2, const BranchFlowSolution& solution)
		{
			//允许累加
			auto nodePair = make_pair(node1, node2);
			auto existing = m_BranchFlow.find(nodePair);
			if (existing != m_BranchFlow.end())
			{
				//需要累加
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