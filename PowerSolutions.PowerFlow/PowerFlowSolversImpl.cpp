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
				_PS_TRACE("迭代次数：" << i);
				double dev = abs(EvalDeviation());
				if (dev < MaxDeviationTolerance())
					return GenerateSolution(SolutionStatus::Success, i, dev);
				if (!OnIteration())
					return GenerateSolution(SolutionStatus::IterationFailed, i, dev);
				if (IntelliIterations())
				{
					//TODO 根据二次收敛特性进行判断
					//也就是说需要在派生类中判断
					if (i > 3 && dev > 1E10)
						return GenerateSolution(SolutionStatus::IntelliIterationAbort, i, dev);
				}
			}
			//达到最大迭代次数，不论是否收敛，均可停止。
			return GenerateSolution(SolutionStatus::MaxIteration, MaxIterations(), EvalDeviation());
		}

		void SolverImpl::MapBuses()
		{
			SlackNode = nullptr;
			//在第一个 for 循环中提前粗略统计PQ/PV节点数目是为了后面 vector 提前预留内存使用。
			NodeCount = PQNodeCount = (int)(CaseInfo.Buses().size());
			PVNodeCount = 0;
			BusMapping.clear();
			BusMapping.reserve(NodeCount);
			for(auto &obj : CaseInfo.Buses())
			{
				//默认PQ节点
				shared_ptr<NodeInfo> info(new NodeInfo(obj));
				BusMapping.emplace(obj, info);
			}
			//如果所有的节点均有连接，则支路数量为 n(n-1)/2
			//此处假设每个母线上均有6回接线
			Branches.reserve(CaseInfo.Buses().size() * 3);
			for (auto &obj : CaseInfo.Components())
			{
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
			while (true)
			{
				auto i = find_if(BusMapping.begin(), BusMapping.end(),
					[](NodeDictionary::value_type &item){return item.second->Degree == 0; });
				if (i != BusMapping.end())
					BusMapping.erase(i);
				else
					break;
			}
			//检查是否存在平衡节点。
			if (SlackNode == nullptr)
				throw Exception(ExceptionCode::SlackBus);
			//扣除 PQNodes 中包括的未被引用的节点数量。
			PQNodeCount -= NodeCount - BusMapping.size();
			NodeCount = BusMapping.size();
			//复制节点列表。
			Nodes.resize(NodeCount);
			transform(BusMapping.cbegin(), BusMapping.cend(), Nodes.begin(),
				[](const NodeDictionary::value_type &item){return item.second.get(); });
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
			} else {
				//不论如何，平衡节点应该在 Nodes 集合的最后面。
				//此处考虑到性能，仅仅交换平衡节点和最后一个节点的位置。
				swap(*find_if(Nodes.begin(), Nodes.end(), [](NodeInfo* node){return node->Type == NodeType::SlackNode; }),
					Nodes.back());
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

		void SolverImpl::GenerateAdmittance()
		{
			// 上三角矩阵，row < col
			//为导纳稀疏矩阵预留空间
			Admittance.resize(NodeCount, NodeCount);
			vector<int> ColSpace;
			ColSpace.resize(NodeCount);
			//将节点表格映射为对应节点的支路数量
			transform(Nodes.begin(), Nodes.end(), ColSpace.begin(), [](NodeInfo *node){ return node->Degree + 1; });
			Admittance.reserve(ColSpace);
			//生成导纳矩阵
			for (auto& c : CaseInfo.Components())
			{
				//形成导纳矩阵
				auto dpc = dynamic_cast<DoublePortComponent*>(c);
				if (dpc != NULL)
				{
					auto pieqv = dpc->PiEquivalency();
					//具有π形等值电路
					int bus1 = BusMapping[dpc->Bus1()]->Index;
					int bus2 = BusMapping[dpc->Bus2()]->Index;
					//BUG CLOSED
					//当系数矩阵非零元素数量增加时，
					//可能会导致预先获取的零元素对应地址被覆盖，
					//如果此时重新使用 coeffRef 即可得到正确的结果。
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
			_PS_TRACE("\n导纳矩阵 ==========\n" << Admittance);
		}

		Solution* SolverImpl::GenerateSolution(SolutionStatus status, int iterCount, double maxDev)
		{
			AfterIterations();	//收尾工作
			auto s = new Solution();
			complexd totalPowerGeneration, totalPowerConsumption,
				totalPowerLoss, totalPowerShunt;
			s->Status(status);
			s->IterationCount(iterCount);
			s->MaxDeviation(maxDev);
			//根据注入功率和负载情况计算节点出力信息。
			for(auto& node : Nodes)
			{
				complexd PowerGeneration(node->ActivePowerInjection, node->ReactivePowerInjection);
				complexd PowerConsumption;
				auto range = CaseInfo.BusComponents().equal_range(node->Bus);
				for_each(range.first, range.second,
					[&](pair<const Bus*, Component*> p){
					//对于PQ负载
					PQLoad *pqload = dynamic_cast<PQLoad*>(p.second);
					if (pqload != nullptr)
					{
						PowerGeneration += pqload->Power();
						PowerConsumption += pqload->Power();
					}
					//对于接地导纳
					ShuntAdmittance *sa = dynamic_cast<ShuntAdmittance*>(p.second);
					if (sa != nullptr)
					{
						//注意此部分功率不应累加至出力功率
						// S = U^2 * conj(Y)
						//BUG FIXED:电压没有取平方
						PowerConsumption += node->Voltage * node->Voltage * conj(sa->Admittance());
					}
				});
				s->AddNodeFlow(node->Bus,
					NodeFlowSolution(node->VoltagePhasor(), PowerGeneration, PowerConsumption));
				totalPowerGeneration += PowerGeneration;
				totalPowerConsumption += PowerConsumption;
			}
			//根据节点电压计算支路功率。			
			for (auto& c : CaseInfo.Components())
			{
				//仅适用于双端元件。
				DoublePortComponent *dpc = dynamic_cast<DoublePortComponent*>(c);
				if (dpc != NULL)
				{
					auto node1 = BusMapping[dpc->Bus1()],
						node2 = BusMapping[dpc->Bus2()];
					complexd power1, power2, shunt1, shunt2;
					auto pieqv = dpc->PiEquivalency();
#define _EvalPower(v1, a1, v2, a2, y10, z12) \
			conj((v1)*(v1)*(y10) + (v1)*((v1) - polar((v2), (a2)-(a1)))/(z12))	//以 v1 作为参考相量
#define _EvalShuntPower(v, y10) (v) * (v) * conj(y10)
					power1 = _EvalPower(node1->Voltage, node1->Angle,
						node2->Voltage, node2->Angle, pieqv.Admittance1(), pieqv.Impedance());
					power2 = _EvalPower(node2->Voltage, node2->Angle,
						node1->Voltage, node1->Angle, pieqv.Admittance2(), pieqv.Impedance());
					//注意，计算支路功率时
					//对于变压器，不能将π型等值电路两侧的接地导纳拆开计算。
					//只能按照Γ型等值电路进行计算。
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
					//追加元件潮流项
					BranchFlowSolution branchFlow(power1, power2, shunt1, shunt2);
					s->AddComponentFlow(c, branchFlow);
					//增加/追加支路潮流
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

