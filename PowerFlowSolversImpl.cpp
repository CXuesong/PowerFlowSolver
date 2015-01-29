#include "stdafx.h"
#include "PowerFlowSolversImpl.h"
#include "Exceptions.h"
#include <algorithm>

using namespace std;
using namespace PowerSolutions::ObjectModel;

namespace PowerSolutions
{
	namespace PowerFlow
	{
		void SolverImpl::MapBuses()
		{
			assert(CaseInfo);
			SlackNode = nullptr;
			//在第一个 for 循环中提前粗略统计PQ/PV节点数目是为了后面 vector 提前预留内存使用。
			NodeCount = PQNodeCount = (int)(CaseInfo->BusesCount());
			PVNodeCount = 0;
			BusMapping.clear();
			BusMapping.reserve(NodeCount);
			for(auto &i = CaseInfo->BusesBegin(), j = CaseInfo->BusesEnd(); i != j; i++)
			{
				//默认PQ节点
				shared_ptr<NodeInfo> info(new NodeInfo((*i).get(), NodeType::PQNode));
				BusMapping.emplace((*i).get(), info);
			}
			//如果所有的节点均有连接，则支路数量为 n(n-1)/2
			//此处假设每个母线上均有6回接线
			Branches.reserve(CaseInfo->BusesCount() * 3);
			for (auto &i = CaseInfo->ComponentsBegin(), j = CaseInfo->ComponentsEnd(); i != j; i++)
			{
				auto obj = (*i).get();
				//统计每条母线被连接的次数。
				//注意此处仅考虑双端元件
				Component* cp = dynamic_cast<Component*>(obj);
				if (cp != nullptr)
				{
					if (cp->PortCount() == 2)
					{
						//加入支路-组件列表中
						if (Branches.insert(make_pair(cp->Buses(0), cp->Buses(1))).second)
						{
							//成功向支路列表中加入了新项，说明出现了新支路。
							BusMapping[cp->Buses(0)]->Degree++;
							BusMapping[cp->Buses(1)]->Degree++;
						}
					}
				}
				//重新确定节点类型，并统计功率注入情况。
				//对于PQ负载
				auto *pqload = dynamic_cast<PQLoad*>(obj);
				if (pqload != nullptr)
				{
					BusMapping[pqload->Bus1()]->AddPQ(-pqload->Power());
				}
				//对于PV发电机
				auto *pvgen = dynamic_cast<PVGenerator*>(obj);
				if (pvgen != nullptr)
				{
					auto &node = BusMapping[pvgen->Bus1()];
					node->ActivePowerInjection += pvgen->ActivePower();
					if (node->Type == NodeType::PQNode)
					{
						//如果存在一台PV发电机，则整个母线为PV/平衡节点。
						//PQ -> PV
						PQNodeCount--;
						PVNodeCount++;
					}
					//设置/校验电压约束
					if (!node->AddPV(pvgen->ActivePower(), pvgen->Voltage()))
						throw Exception(ExceptionCode::VoltageMismatch);
				}
				//对于平衡发电机
				auto *slackgen = dynamic_cast<SlackGenerator*>(obj);
				if (slackgen != nullptr)
				{
					//如果存在一台平衡发电机，则整个母线为平衡节点。
					auto &node = BusMapping[slackgen->Bus1()];
					if (SlackNode == nullptr)
					{
						// PQ / PV -> Slack
						if (node->Type == NodeType::PQNode) PQNodeCount--;
						else if (node->Type == NodeType::PVNode) PVNodeCount--;
						SlackNode = node;
						node->AddSlack(slackgen->Voltage());
					} else if (SlackNode == node)
					{
						//在同一个母线上放置了多台平衡发电机。
						if (!node->AddSlack(slackgen->Voltage()))
							throw Exception(ExceptionCode::VoltageMismatch);
					} else {
						//存在多于一台平衡发电机
						throw Exception(ExceptionCode::SlackBus);
					}
				}
			}
			//痛苦的遍历结束了……
			//注意，此时的统计的PQ节点数量中还包含了孤立的节点
			//从 BusMapping 中移除未被引用的节点。
			assert(BusMapping.size() == NodeCount);
			BusMapping.erase(find_if(BusMapping.begin(), BusMapping.end(),
				[](NodeDictionary::value_type &item){return item.second->Degree == 0; }));
			//检查是否存在平衡节点。
			if (SlackNode == nullptr)
				throw Exception(ExceptionCode::SlackBus);
			//扣除 PQNodes 中包括的未被引用的节点数量。
			PQNodeCount -= NodeCount - BusMapping.size();
			NodeCount = BusMapping.size();
			//复制节点列表。
			Nodes.reserve(NodeCount);
			transform(BusMapping.cbegin(), BusMapping.cend(), Nodes.begin(),
				[](const NodeDictionary::value_type &item){return item.second; });
			//采用静态节点优化编号,即将节点的出线数从小到大依次排列
			//对Nodes列表进行排序。
			if (NodeReorder())
			{
				sort(Nodes.begin(), Nodes.end(),
					[](const NodeCollection::value_type &x, const NodeCollection::value_type &y)
				{
					//将平衡节点放到列表的末尾
					if (x->Type == NodeType::SlackNode) return false;
					if (y->Type == NodeType::SlackNode) return true;
					return x->Degree < y->Degree;
				});
			}
			//按照新的顺序重新编号
			int IndexCounter1 = 0, IndexCounter2 = 0;
			PQNodes.reserve(PQNodeCount);
			PVNodes.reserve(PVNodeCount);
			//TODO 优化冗余的存储
			//CASE 如果没有PV节点，会导致异常
			for (auto node : Nodes)
			{
				assert(node->Degree > 0);
				//为节点编号。
				node->Index = IndexCounter1 + IndexCounter2;
				if (node->Type == NodeType::PQNode)
				{
					node->SubIndex = IndexCounter1;
					IndexCounter1++;
					PQNodes.push_back(node);
				} else if (node->Type == NodeType::PVNode)
				{
					node->SubIndex = IndexCounter2;
					IndexCounter2++;
					PVNodes.push_back(node);
				}
			}
			//平衡节点编号放在最后面。
			SlackNode->Index = IndexCounter1 + IndexCounter2;
			SlackNode->SubIndex = 0;
		}
		void SolverImpl::ScanComponents()
		{

		}
	}
}

