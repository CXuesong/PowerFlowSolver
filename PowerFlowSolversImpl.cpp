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
			//�ڵ�һ�� for ѭ������ǰ����ͳ��PQ/PV�ڵ���Ŀ��Ϊ�˺��� vector ��ǰԤ���ڴ�ʹ�á�
			NodeCount = PQNodeCount = (int)(CaseInfo->BusesCount());
			PVNodeCount = 0;
			BusMapping.clear();
			BusMapping.reserve(NodeCount);
			for(auto &i = CaseInfo->BusesBegin(), j = CaseInfo->BusesEnd(); i != j; i++)
			{
				//Ĭ��PQ�ڵ�
				shared_ptr<NodeInfo> info(new NodeInfo((*i).get(), NodeType::PQNode));
				BusMapping.emplace((*i).get(), info);
			}
			//������еĽڵ�������ӣ���֧·����Ϊ n(n-1)/2
			//�˴�����ÿ��ĸ���Ͼ���6�ؽ���
			Branches.reserve(CaseInfo->BusesCount() * 3);
			for (auto &i = CaseInfo->ComponentsBegin(), j = CaseInfo->ComponentsEnd(); i != j; i++)
			{
				auto obj = (*i).get();
				//ͳ��ÿ��ĸ�߱����ӵĴ�����
				//ע��˴�������˫��Ԫ��
				Component* cp = dynamic_cast<Component*>(obj);
				if (cp != nullptr)
				{
					if (cp->PortCount() == 2)
					{
						//����֧·-����б���
						if (Branches.insert(make_pair(cp->Buses(0), cp->Buses(1))).second)
						{
							//�ɹ���֧·�б��м��������˵����������֧·��
							BusMapping[cp->Buses(0)]->Degree++;
							BusMapping[cp->Buses(1)]->Degree++;
						}
					}
				}
				//����ȷ���ڵ����ͣ���ͳ�ƹ���ע�������
				//����PQ����
				auto *pqload = dynamic_cast<PQLoad*>(obj);
				if (pqload != nullptr)
				{
					BusMapping[pqload->Bus1()]->AddPQ(-pqload->Power());
				}
				//����PV�����
				auto *pvgen = dynamic_cast<PVGenerator*>(obj);
				if (pvgen != nullptr)
				{
					auto &node = BusMapping[pvgen->Bus1()];
					node->ActivePowerInjection += pvgen->ActivePower();
					if (node->Type == NodeType::PQNode)
					{
						//�������һ̨PV�������������ĸ��ΪPV/ƽ��ڵ㡣
						//PQ -> PV
						PQNodeCount--;
						PVNodeCount++;
					}
					//����/У���ѹԼ��
					if (!node->AddPV(pvgen->ActivePower(), pvgen->Voltage()))
						throw Exception(ExceptionCode::VoltageMismatch);
				}
				//����ƽ�ⷢ���
				auto *slackgen = dynamic_cast<SlackGenerator*>(obj);
				if (slackgen != nullptr)
				{
					//�������һ̨ƽ�ⷢ�����������ĸ��Ϊƽ��ڵ㡣
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
						//��ͬһ��ĸ���Ϸ����˶�̨ƽ�ⷢ�����
						if (!node->AddSlack(slackgen->Voltage()))
							throw Exception(ExceptionCode::VoltageMismatch);
					} else {
						//���ڶ���һ̨ƽ�ⷢ���
						throw Exception(ExceptionCode::SlackBus);
					}
				}
			}
			//ʹ��ı��������ˡ���
			//ע�⣬��ʱ��ͳ�Ƶ�PQ�ڵ������л������˹����Ľڵ�
			//�� BusMapping ���Ƴ�δ�����õĽڵ㡣
			assert(BusMapping.size() == NodeCount);
			BusMapping.erase(find_if(BusMapping.begin(), BusMapping.end(),
				[](NodeDictionary::value_type &item){return item.second->Degree == 0; }));
			//����Ƿ����ƽ��ڵ㡣
			if (SlackNode == nullptr)
				throw Exception(ExceptionCode::SlackBus);
			//�۳� PQNodes �а�����δ�����õĽڵ�������
			PQNodeCount -= NodeCount - BusMapping.size();
			NodeCount = BusMapping.size();
			//���ƽڵ��б�
			Nodes.reserve(NodeCount);
			transform(BusMapping.cbegin(), BusMapping.cend(), Nodes.begin(),
				[](const NodeDictionary::value_type &item){return item.second; });
			//���þ�̬�ڵ��Ż����,�����ڵ�ĳ�������С������������
			//��Nodes�б��������
			if (NodeReorder())
			{
				sort(Nodes.begin(), Nodes.end(),
					[](const NodeCollection::value_type &x, const NodeCollection::value_type &y)
				{
					//��ƽ��ڵ�ŵ��б��ĩβ
					if (x->Type == NodeType::SlackNode) return false;
					if (y->Type == NodeType::SlackNode) return true;
					return x->Degree < y->Degree;
				});
			}
			//�����µ�˳�����±��
			int IndexCounter1 = 0, IndexCounter2 = 0;
			PQNodes.reserve(PQNodeCount);
			PVNodes.reserve(PVNodeCount);
			//TODO �Ż�����Ĵ洢
			//CASE ���û��PV�ڵ㣬�ᵼ���쳣
			for (auto node : Nodes)
			{
				assert(node->Degree > 0);
				//Ϊ�ڵ��š�
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
			//ƽ��ڵ��ŷ�������档
			SlackNode->Index = IndexCounter1 + IndexCounter2;
			SlackNode->SubIndex = 0;
		}
		void SolverImpl::ScanComponents()
		{

		}
	}
}

