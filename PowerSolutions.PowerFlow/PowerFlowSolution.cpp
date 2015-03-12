#include "stdafx.h"
#include "PowerFlowSolution.h"
#include "ObjectModel.h"
#include <utility>

using namespace PowerSolutions::ObjectModel;
using namespace std;

namespace PowerSolutions
{
	namespace PowerFlow
	{
		Solution::Solution()
		{ }

		void Solution::AddNodeFlow(Bus* node, const NodeFlowSolution& solution)
		{
			auto result = m_NodeFlow.emplace(node, solution);
			assert(result.second);
		}

		void Solution::AddComponentFlow(ObjectModel::Component* c, const ComponentFlowSolution& solution)
		{
			auto result = m_ComponentFlow.emplace(c, solution);
			assert(result.second);
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