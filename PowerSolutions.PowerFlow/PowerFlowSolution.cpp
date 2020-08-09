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
			if (node->Index() != NullIndex)
			{
				auto result = m_NodeFlow.emplace(node->Index(), solution);
				assert(result.second);
			}
		}

		void Solution::AddComponentFlow(Component* c, const BranchFlowSolution& solution)
		{
			if (c->Index() != NullIndex)
			{
				auto result = m_ComponentFlow.emplace(c->Index(), solution);
				assert(result.second);
			}
		}

		void Solution::AddBranchFlow(Bus* node1, Bus* node2, const BranchFlowSolution& solution)
		{
			//�����ۼ�
			if (node1->Index() != NullIndex && node1->Index() != NullIndex)
			{
				auto nodePair = make_pair(node1->Index(), node2->Index());
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
}