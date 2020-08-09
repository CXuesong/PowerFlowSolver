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
			CaseInfo = caseInfo->Expand();
			MapBuses();
			GenerateAdmittance();
			BeforeIterations();
			for (int i = 0; i <= MaxIterations(); i++)
			{
				_PS_TRACE("����������" << i);
				double dev = abs(EvalDeviation());
				if (dev < MaxDeviationTolerance())
					return GenerateSolution(SolutionStatus::Success, i, dev);
				if (!OnIteration())
					return GenerateSolution(SolutionStatus::IterationFailed, i, dev);
				if (IntelliIterations())
				{
					//TODO ���ݶ����������Խ����ж�
					//Ҳ����˵��Ҫ�����������ж�
					if (i > 3 && dev > 1E10)
						return GenerateSolution(SolutionStatus::IntelliIterationAbort, i, dev);
				}
			}
			//�ﵽ�����������������Ƿ�����������ֹͣ��
			return GenerateSolution(SolutionStatus::MaxIteration, MaxIterations(), EvalDeviation());
		}

		void SolverImpl::MapBuses()
		{
			SlackNode = nullptr;
			//�ڵ�һ�� for ѭ������ǰ����ͳ��PQ/PV�ڵ���Ŀ��Ϊ�˺��� vector ��ǰԤ���ڴ�ʹ�á�
			NodeCount = PQNodeCount = (int)(CaseInfo.Buses().size());
			PVNodeCount = 0;
			BusMapping.clear();
			BusMapping.reserve(NodeCount);
			for(auto &obj : CaseInfo.Buses())
			{
				//Ĭ��PQ�ڵ�
				shared_ptr<NodeInfo> info(new NodeInfo(obj));
				BusMapping.emplace(obj, info);
			}
			//������еĽڵ�������ӣ���֧·����Ϊ n(n-1)/2
			//�˴�����ÿ��ĸ���Ͼ���6�ؽ���
			Branches.reserve(CaseInfo.Buses().size() * 3);
			for (auto &obj : CaseInfo.Components())
			{
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
			while (true)
			{
				auto i = find_if(BusMapping.begin(), BusMapping.end(),
					[](NodeDictionary::value_type &item){return item.second->Degree == 0; });
				if (i != BusMapping.end())
					BusMapping.erase(i);
				else
					break;
			}
			//����Ƿ����ƽ��ڵ㡣
			if (SlackNode == nullptr)
				throw Exception(ExceptionCode::SlackBus);
			//�۳� PQNodes �а�����δ�����õĽڵ�������
			PQNodeCount -= NodeCount - BusMapping.size();
			NodeCount = BusMapping.size();
			//���ƽڵ��б�
			Nodes.resize(NodeCount);
			transform(BusMapping.cbegin(), BusMapping.cend(), Nodes.begin(),
				[](const NodeDictionary::value_type &item){return item.second.get(); });
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
			} else {
				//������Σ�ƽ��ڵ�Ӧ���� Nodes ���ϵ�����档
				//�˴����ǵ����ܣ���������ƽ��ڵ�����һ���ڵ��λ�á�
				swap(*find_if(Nodes.begin(), Nodes.end(), [](NodeInfo* node){return node->Type == NodeType::SlackNode; }),
					Nodes.back());
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

		void SolverImpl::GenerateAdmittance()
		{
			// �����Ǿ���row < col
			//Ϊ����ϡ�����Ԥ���ռ�
			Admittance.resize(NodeCount, NodeCount);
			vector<int> ColSpace;
			ColSpace.resize(NodeCount);
			//���ڵ���ӳ��Ϊ��Ӧ�ڵ��֧·����
			transform(Nodes.begin(), Nodes.end(), ColSpace.begin(), [](NodeInfo *node){ return node->Degree + 1; });
			Admittance.reserve(ColSpace);
			//���ɵ��ɾ���
			for (auto& c : CaseInfo.Components())
			{
				//�γɵ��ɾ���
				auto dpc = dynamic_cast<DoublePortComponent*>(c);
				if (dpc != NULL)
				{
					auto pieqv = dpc->PiEquivalency();
					//���Ц��ε�ֵ��·
					int bus1 = BusMapping[dpc->Bus1()]->Index;
					int bus2 = BusMapping[dpc->Bus2()]->Index;
					//BUG CLOSED
					//��ϵ���������Ԫ����������ʱ��
					//���ܻᵼ��Ԥ�Ȼ�ȡ����Ԫ�ض�Ӧ��ַ�����ǣ�
					//�����ʱ����ʹ�� coeffRef ���ɵõ���ȷ�Ľ����
					complexd transAdmittance = 1.0 / pieqv.Impedance();
					Admittance.coeffRef(bus1, bus1) += transAdmittance + pieqv.Admittance1();
					Admittance.coeffRef(bus2, bus2) += transAdmittance + pieqv.Admittance2();
					if (bus1 > bus2) swap(bus1, bus2);
					Admittance.coeffRef(bus1, bus2) -= transAdmittance;
					//TraceFile << pieqv->PiAdmittance1() << " /- " << transAdmittance << " -\\ " << pieqv->PiAdmittance2() << endl;
					assert(!isnan(Admittance.coeffRef(bus1, bus1).imag()));
					assert(!isnan(Admittance.coeffRef(bus2, bus2).imag()));
					assert(!isnan(Admittance.coeffRef(bus1, bus2).imag()));
				}
				auto sa = dynamic_cast<ShuntAdmittance*>(c);
				if (sa != NULL)
				{
					int bus = BusMapping[sa->Bus1()]->Index;
					Admittance.coeffRef(bus, bus) += sa->Admittance();
				}
			}
			_PS_TRACE("\n���ɾ��� ==========\n" << Admittance);
		}

		Solution* SolverImpl::GenerateSolution(SolutionStatus status, int iterCount, double maxDev)
		{
			AfterIterations();	//��β����
			auto s = new Solution();
			complexd totalPowerGeneration, totalPowerConsumption,
				totalPowerLoss, totalPowerShunt;
			s->Status(status);
			s->IterationCount(iterCount);
			s->MaxDeviation(maxDev);
			//����ע�빦�ʺ͸����������ڵ������Ϣ��
			for(auto& node : Nodes)
			{
				complexd PowerGeneration(node->ActivePowerInjection, node->ReactivePowerInjection);
				complexd PowerConsumption;
				auto range = CaseInfo.BusComponents().equal_range(node->Bus);
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
					NodeFlowSolution(node->VoltagePhasor(), PowerGeneration, PowerConsumption));
				totalPowerGeneration += PowerGeneration;
				totalPowerConsumption += PowerConsumption;
			}
			//���ݽڵ��ѹ����֧·���ʡ�			
			for (auto& c : CaseInfo.Components())
			{
				//��������˫��Ԫ����
				DoublePortComponent *dpc = dynamic_cast<DoublePortComponent*>(c);
				if (dpc != NULL)
				{
					auto node1 = BusMapping[dpc->Bus1()],
						node2 = BusMapping[dpc->Bus2()];
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

		inline void SolverImpl::NodeInfo::AddPQ(complexd power)
		{
			ActivePowerInjection += power.real();
			ReactivePowerInjection += power.imag();
		}

		inline bool SolverImpl::NodeInfo::AddPV(double activePower, double voltage)
		{
			if (Type != NodeType::PQNode && std::abs(Voltage - voltage) > 1e-10)
				return false;
			Voltage = voltage;
			ActivePowerInjection += activePower;
			Type = NodeType::PVNode;
			return true;
		}

		inline bool SolverImpl::NodeInfo::AddSlack(complexd voltagePhasor)
		{
			if (Type != NodeType::PQNode && std::abs(Voltage - std::abs(voltagePhasor)) > 1e-10)
				return false;
			Voltage = std::abs(voltagePhasor);
			Angle = std::arg(voltagePhasor);
			Type = NodeType::SlackNode;
			return true;
		}

		inline complexd SolverImpl::NodeInfo::VoltagePhasor()
		{
			return polar(Voltage, Angle);
		}

	}
}

