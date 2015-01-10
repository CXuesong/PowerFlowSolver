#include "stdafx.h"
#include "Console.h"
#include <Eigen/SparseLU>
#include "Solvers.h"
#include "resource.h"

using namespace AppNamespace::Solvers;
using namespace AppNamespace::Resource::Solvers;
using Eigen::VectorXd;
using Eigen::SparseLU;
using Eigen::SparseQR;

tstring NRSolver::Name() const
{
	return NRSolverName;
}

tstring NRSolver::FriendlyName() const
{
	return NRSolverFriendlyName;
}

void NRSolver::BeforeIterations()
{
	Block1EquationCount = PQNodeCount + PVNodeCount;
	EquationCount = PQNodeCount * 2 + PVNodeCount;
	//PQ节点：[dP dQ] = J * [V theta]
	//PV节点：[dP] = J * [theta]
	//       PQ/PV    |   PQ
	//Δy = [dP ... dP | dQ ... dQ] = ConstraintPower - CurrentPower
	//Δx = [theta ... theta | V ... V]
	// 分界线/起始索引：
	// H[0,0], N[NodeCount - 1, 0]
	//初始化矩阵维数
	//注意向量需要手动清零
	ConstraintPowerInjection.resize(EquationCount);
	CurrentAnswer.resize(EquationCount);
	CorrectionAnswer.resize(EquationCount);
	PowerInjectionDeviation.resize(EquationCount);

	//确定雅可比矩阵每一列需要的非零元空间数量
	//按照最不理想的情况来申请雅可比矩阵的空间
	//具体的申请空间操作
	vector<int> JocobianColValues;		//雅可比矩阵中每一列的非零元素数量，用于为矩阵预留空间。
	JocobianColValues.resize(EquationCount);
	for (int n = 0; n < Block1EquationCount; n++)
	{
		NodeInfo *node = Nodes[n];
		JocobianColValues[n] = min(node->BranchCount * 2, EquationCount);
		if (node->NodeType == NodeType::PQNode)
		{
			JocobianColValues[Block1EquationCount + node->SubIndex] =
				min(node->BranchCount * 2, EquationCount);
		}
	}
	Jocobian.resize(EquationCount, EquationCount);
	Jocobian.reserve(JocobianColValues);

	//生成目标注入功率向量 y，以及迭代初值向量。
	for_each(Nodes.begin(), --Nodes.end(), [this](NodeInfo* node)
	{
		int subIndex = Block1EquationCount + node->SubIndex;
		ConstraintPowerInjection(node->Index) = node->ActivePowerInjection;
		CurrentAnswer(node->Index) = 0;			//初始相位角 = 0
		if (node->NodeType == NodeType::PQNode)
		{
			ConstraintPowerInjection(subIndex) = node->ReactivePowerInjection;
			CurrentAnswer(subIndex) = node->Reference->InitialVoltage;
		}
	});
}

double NRSolver::EvalDeviation()
{
	EvalCurrentPowerInjection();
	//BUG CLOSED
	//最小系数不一定是绝对值最小的系数
	return PowerInjectionDeviation.cwiseAbs().maxCoeff();
}

bool PowerFlowSolver::Solvers::NRSolver::OnIteration()
{
	GenerateJacobian();
	if (!GenerateNextAnswer()) return false;
	return true;
}

void NRSolver::AfterIterations()
{
	for_each(Nodes.begin(), Nodes.end(), [this](NodeInfo *node)
	{
		node->Voltage = NodeVoltage(node->Index);
		node->Angle = NodeAngle(node->Index);
	});
	SSSolver::AfterIterations();
}

void NRSolver::Clear()
{
	SSSolver::Clear();
}

void NRSolver::EvalCurrentPowerInjection()
{
	PowerInjectionDeviation = ConstraintPowerInjection;
	for_each(Nodes.begin(), Nodes.end(), [](NodeInfo *node){node->ClearPowerInjections(); });
	//遍历所有节点，包括平衡节点
	for (int m = 0; m < NodeCount; m++)
	{
		NodeInfo *nodeM = Nodes[m];
		double Um = NodeVoltage(m);
		double thetaM = NodeAngle(m);
		int subM = Block1EquationCount + nodeM->SubIndex;
		//计算非对角元素
		for (int n = m + 1; n < NodeCount; n++)
		{
			// 上三角矩阵，row < col
			complexd Y = Admittance.coeff(m, n);
			if (abs(Y) < 1e-10) continue;
			double UmUn = Um * NodeVoltage(n);
			double thetaMn = thetaM - NodeAngle(n);
			double sinMn = sin(thetaMn);
			double cosMn = cos(thetaMn);
			NodeInfo *nodeN = Nodes[n];
			//累计上一次迭代结果对应的注入功率
			nodeM->ActivePowerInjection += UmUn * (Y.real() * cosMn + Y.imag() * sinMn);
			nodeM->ReactivePowerInjection += UmUn * (Y.real() * sinMn - Y.imag() * cosMn);
			nodeN->ActivePowerInjection += UmUn * (Y.real() * cosMn - Y.imag() * sinMn);
			nodeN->ReactivePowerInjection -= UmUn * (Y.real() * sinMn + Y.imag() * cosMn);
		}
		//计算对角元素
		double UmSqr = Um * Um;
		complexd Y = Admittance.coeff(m, m);
		nodeM->ActivePowerInjection += UmSqr * Y.real();
		nodeM->ReactivePowerInjection -= UmSqr * Y.imag();
		//生成功率偏差向量 Δy'
		if (nodeM->NodeType != NodeType::SlackNode)
		{
			PowerInjectionDeviation(m) -= nodeM->ActivePowerInjection;
			if (nodeM->NodeType == NodeType::PQNode)
				PowerInjectionDeviation(subM) -= nodeM->ReactivePowerInjection;
		}
	}
#if _DEBUG
	TraceFile << "Slack Node ======\n" << 
		"Active Injection:" << SlackNode->ActivePowerInjection << endl <<
		"Reactive Injection:" << SlackNode->ReactivePowerInjection << endl;
	TraceFile << "constraintY ==========\n" << ConstraintPowerInjection << endl;
	TraceFile << "deltaY ==========\n" << PowerInjectionDeviation << endl;
#endif
}

void NRSolver::GenerateJacobian()
{
	//WriteOutput(OutputType::None, PROMPT_GENERATE_JOCOBIAN);
	Jocobian.setZero();
	//PQ节点：[dP dQ] = J * [V theta]
	//PV节点：[dP] = J * [theta]
	//       PQ/PV        PQ
	//Δy = [dP ... dP dQ ... dQ]
	//Δx = [theta ... theta V ... V]
	//    / H | N \
	//J = | --+-- |
	//    \ M | L /
	// 起始索引：（注意排除平衡节点）
	// H[0,0], N[NodeCount - 1, 0]
	// M[NodeCount - 1, 0], L[..., ...]
	// 对非对角元有
	// H =  L
	// N = -M
	for (int m = 0; m < NodeCount - 1; m++)
	{
		NodeInfo *nodeM = Nodes[m];
		double Um = NodeVoltage(m);
		double thetaM = NodeAngle(m);
		int subM = Block1EquationCount + nodeM->SubIndex;
		//计算非对角元素
		for (int n = m + 1; n < NodeCount - 1; n++)
		{
			// 上三角矩阵，row < col
			complexd Y = Admittance.coeff(m, n);
			if (abs(Y) < 1e-10) continue;
			double UmUn = Um * NodeVoltage(n);
			double thetaMn = thetaM - NodeAngle(n);
			double sinMn = sin(thetaMn);
			double cosMn = cos(thetaMn);
			//H(m,n)
			double H = Jocobian.coeffRef(m, n) = -UmUn * (Y.real() * sinMn - Y.imag() * cosMn);
			//H(n,m)
			double Hp = Jocobian.coeffRef(n, m) = UmUn * (Y.real() * sinMn + Y.imag() * cosMn) /*-UmUn * (-Y.real() * sinMn - Y.imag() * cosMn)*/;
			//N
			double N = -UmUn * (Y.real() * cosMn + Y.imag() * sinMn);
			double Np = -UmUn * (Y.real() * cosMn - Y.imag() * sinMn);
			//cout << "::" << m << "," << n << " = " << Jocobian.coeffRef(1, 0) << endl;
			NodeInfo *nodeN = Nodes[n];

			int subN = Block1EquationCount + nodeN->SubIndex;
			if (nodeM->NodeType == NodeType::PQNode)
			{
				if (nodeN->NodeType == NodeType::PQNode)
				{
					//PQ-PQ，子阵具有对称性
					// N
					Jocobian.coeffRef(m, subN) = N;
					Jocobian.coeffRef(n, subM) = Np;
					// M = -N
					Jocobian.coeffRef(subM, n) = -N;
					//BUG CLOSED
					//Jocobian.coeffRef(subM, n) = -Np;
					//不正确的雅可比矩阵元素生成
					//天坑。
					Jocobian.coeffRef(subN, m) = -Np;
					// L = H
					Jocobian.coeffRef(subM, subN) = H;
					Jocobian.coeffRef(subN, subM) = Hp;
				} else {
					//PQ-PV（m,n）
					// M = -N
					Jocobian.coeffRef(subM, n) = -N;
					//PV-PQ（n,m）
					// N
					Jocobian.coeffRef(n, subM) = Np;
				}
			} else {
				if (nodeN->NodeType == NodeType::PQNode)
				{
					//PV-PQ（m,n）
					// N
					Jocobian.coeffRef(m, subN) = N;
					//PQ-PV（n,m）
					// M
					Jocobian.coeffRef(subN, m) = -Np;
				}
			}
		}
		complexd Y = Admittance.coeffRef(m, m);
		double UmSqr = Um * Um;
		//计算对角元素
		//H
		Jocobian.coeffRef(m, m) = UmSqr * Y.imag() + nodeM->ReactivePowerInjection;
		if (nodeM->NodeType == NodeType::PQNode)
		{
			//N
			Jocobian.coeffRef(m, subM) = -UmSqr * Y.real() - nodeM->ActivePowerInjection;
			//M
			Jocobian.coeffRef(subM, m) = UmSqr * Y.real() - nodeM->ActivePowerInjection;
			//L
			Jocobian.coeffRef(subM, subM) = UmSqr * Y.imag() - nodeM->ReactivePowerInjection;
		}
	}
#if _DEBUG
	TraceFile << "\nJocobian ==========\n" << Jocobian << endl;
#endif
}

bool NRSolver::GenerateNextAnswer()
{
	//WriteOutput(OutputType::None, PROMPT_SOLVE_EQUATIONS);
	SparseLU<SparseMatrix<double>> solver;
	//求解矩阵方程
	Jocobian.makeCompressed();
	solver.compute(Jocobian);
	if (solver.info() != Eigen::Success)
	{
		Console::WritePrompt(ERROR_MATRIX_DECOMPOSITION);
		return false;
	}
	CorrectionAnswer = solver.solve(PowerInjectionDeviation);
	if (solver.info() != Eigen::Success)
	{
		Console::WritePrompt(ERROR_MATRIX_DECOMPOSITION);
		return false;
	}
	//计算新的结果
	//注意到 Δy = -J Δx
	//而此处实际解的方程组为 Δy' = J Δx
	//也就是说，Δy = -Δy'
	//Δx = [theta ... theta V ... V]
	CurrentAnswer.head(Block1EquationCount) -= CorrectionAnswer.head(Block1EquationCount);
	CurrentAnswer.tail(Block2EquationCount) -= CurrentAnswer.tail(Block2EquationCount).cwiseProduct(CorrectionAnswer.tail(Block2EquationCount));
	//cout << "X = \n" << CurrentAnswer << endl;
	return true;
}

