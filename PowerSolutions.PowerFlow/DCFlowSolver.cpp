#include "stdafx.h"
#include "ObjectModel.h"
#include "NetworkCase.h"
#include "Exceptions.h"
#include "Utility.h"
#include "DCFlowSolver.h"
#include <algorithm>
#include <Eigen/Sparse>
#include <Eigen/SparseLU>

using namespace std;
using namespace PowerSolutions::ObjectModel;
using namespace Eigen;

namespace PowerSolutions
{
	namespace PowerFlow
	{
		DCFlowSolver::~DCFlowSolver()
		{
		}

		shared_ptr<Solution> DCFlowSolver::Solve(NetworkCase& network)
		{
			auto pn = network.ToPrimitive(PrimitiveNetworkOptions::NodeReorder | PrimitiveNetworkOptions::IgnoreShuntAdmittance);
			return this->Solve(*pn);
		}

		shared_ptr<Solution> DCFlowSolver::Solve(PrimitiveNetwork& network)
		{
			auto s = make_shared<Solution>();
			s->Status(SolutionStatus::Success);
			s->IterationCount(0);
			s->MaxDeviation(0);
			s->NodeCount(network.Nodes().size());
			s->PQNodeCount(network.PQNodes().size());
			s->PVNodeCount(network.PVNodes().size());
			s->SlackNode(network.SlackNode()->Bus());
			// 如果在结果生成完毕之前，直接使用 return s;
			// 就会返回一个表示求解失败的结果。
			s->Status(SolutionStatus::IterationFailed);
			// 计算直流潮流
			// P = Bθ
			auto& YMatrix = network.Admittance;
			// TODO: 考虑此处为何取负。
			auto BMatrix = -YMatrix.imag();
			//生成注入功率向量
			auto Nodes = network.Nodes().size();
			assert(Nodes > 1);
			auto IndependentNodes = Nodes - 1;
			SparseLU<SparseMatrix<double>> solver;
			//求解直流潮流。
			VectorXd PVector;
			PVector.resize(IndependentNodes);
			for (auto& node : network.Nodes())
			{
				if (node->Type() != NodeType::SlackNode) PVector[node->Index()] = node->ActivePowerInjection();
			}
			solver.compute(BMatrix.topLeftCorner(IndependentNodes, IndependentNodes));
			if (solver.info() != Eigen::Success) return s;
			//tcout << _T("  B矩阵秩：") << Solver.rank() << endl;
			// 除平衡节点外其它节点的 theta 。
			VectorXd theta = solver.solve(PVector);
			auto ps = make_shared<PrimitiveSolution>(network);
			//cout << ThetaVector << endl;
			unordered_map<Component*, ComponentFlowSolution> componentPowerInjections{};
			//初始化节点功率
			for (auto& ns : ps->NodeStatus())
			{
				//直流潮流，电压幅值为1。
				if (ns.Type() == NodeType::SlackNode)
					ns.SetVoltage(1, 0);
				else
					ns.SetVoltage(1, theta[ns.Index()]);
			}
			//计算并写入支路功率。
			for (auto& obj : network.SourceNetwork()->Objects())
			{
				auto c = dynamic_cast<Component*>(obj);
				if (c != nullptr)
				{
					//计算元件的潮流。
					//注意，此处计算元件潮流时，还是按照交流情况进行计算的。只不过仅保留计算结果的实数部分。
					//可能会造成结果偏差。
					auto cflow = c->EvalComponentFlow(*ps);
					auto injections = cflow.PowerInjections();
					for (size_t i = 0; i < cflow.PowerInjections().size(); i++)
					{
						complexd inj(cflow.PowerInjections(i).real(), 0);
						//修正潮流结果。
						cflow.PowerInjections(i, inj);
						//记录潮流结果。
						//并累加节点注入功率。
						ps->NodeStatus(network.Nodes(c->Buses(i)).Index()).AddPowerInjections(inj.real(), inj.imag());
					}
					cflow.PowerShunt(0);
					//注意，此处不能直接 AddComponentFlow
					//因为 AddComponentFlow 会尝试修改 Solution.NodeFlow 的内容。
					//而此时 NodeFlow 为空。
					componentPowerInjections.emplace(c, move(cflow));
				}
			}
			//写入节点功率。
			for (auto& node : network.Nodes()) s->AddNodeFlow(node->Bus(), ps->NodeStatus(node->Index()));
			//写入元件注入功率。
			for (auto& p : componentPowerInjections) s->AddComponentFlow(p.first, move(p.second));
			s->Status(SolutionStatus::Success);
			return s;
		}
	}
}