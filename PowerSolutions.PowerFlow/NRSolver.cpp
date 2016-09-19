#include "stdafx.h"
#include "NRSolver.h"
#include "PrimitiveNetwork.h"
#include "Exceptions.h"
#include <Eigen/Sparse>
#include <Eigen/SparseLU>

using Eigen::SparseLU;
using Eigen::SparseQR;
using Eigen::SparseMatrix;

using namespace std;
using namespace PowerSolutions::ObjectModel;

namespace PowerSolutions
{
	namespace PowerFlow
	{

		NRSolver::NRSolver()
		{ }

		NRSolver::~NRSolver()
		{ }

		void NRSolver::BeforeIterations()
		{
			assert(Block1EquationCount() == PQNodeCount + PVNodeCount);
			assert(EquationCount() == PQNodeCount * 2 + PVNodeCount);
			//PQ节点：[dP dQ] = J * [V theta]
			//PV节点：[dP] = J * [theta]
			//       PQ/PV    |   PQ
			//Δy = [dP ... dP | dQ ... dQ] = ConstraintPower - CurrentPower
			//Δx = [theta ... theta | V ... V]
			// 分界线/起始索引：
			// H[0,0], N[NodeCount - 1, 0]
			//初始化矩阵维数
			//注意向量需要手动清零
			ConstraintPowerInjection.resize(EquationCount());
			CurrentAnswer.resize(EquationCount());
			CorrectionAnswer.resize(EquationCount());
			PowerInjectionDeviation.resize(EquationCount());

			//确定雅可比矩阵每一列需要的非零元空间数量
			//按照最不理想的情况来申请雅可比矩阵的空间
			//具体的申请空间操作
			vector<int> JocobianColSpace(EquationCount());		//雅可比矩阵中每一列的非零元素数量，用于为矩阵预留空间。
			for (int n = 0; n < Block1EquationCount(); n++)
			{
				auto& node = PNetwork->Nodes(n);
				JocobianColSpace[n] = min(node.Degree() * 2, EquationCount());
				if (node.Type() == NodeType::PQNode)
				{
					JocobianColSpace[Block1EquationCount() + node.SubIndex()] = min(node.Degree() * 2, EquationCount());
				}
			}
			Jocobian.resize(EquationCount(), EquationCount());
			Jocobian.reserve(JocobianColSpace);

			//生成目标注入功率向量 y，以及迭代初值向量。
			for (auto& node : PNetwork->Nodes())
			{
				if (node->Type() != NodeType::SlackNode)
				{
					int subIndex = Block1EquationCount() + node->SubIndex();
					ConstraintPowerInjection(node->Index()) = node->ActivePowerInjection();
					CurrentAnswer(node->Index()) = arg(node->Bus()->InitialVoltage());
					if (node->Type() == NodeType::PQNode)
					{
						ConstraintPowerInjection(subIndex) = node->ReactivePowerInjection();
						CurrentAnswer(subIndex) = abs(node->Bus()->InitialVoltage());
					}
				}
			}
			_PS_TRACE("目标函数值 Y ==========");
			_PS_TRACE(ConstraintPowerInjection);
		}

		double NRSolver::EvalDeviation()
		{
			EvalPowerInjection();
			//BUG CLOSED
			//最小系数不一定是绝对值最小的系数
			return PowerInjectionDeviation.cwiseAbs().maxCoeff();
		}

		bool NRSolver::OnIteration()
		{
			GenerateJacobian();
			if (!GenerateNextAnswer()) return false;
			return true;
		}

		void NRSolver::AfterIterations()
		{
			for (int i = 0; i < NodeCount; i++)
				UpdateNodeStatus(i);
		}

		inline double NRSolver::NodeVoltage(int NodeIndex)
		{
			auto& node = PNetwork->Nodes(NodeIndex);
			if (node.Type() == NodeType::PQNode)
				return CurrentAnswer(Block1EquationCount() + node.SubIndex());
			else
				return node.Voltage();
		}

		inline double NRSolver::NodeAngle(int NodeIndex)
		{
			assert(NodeIndex < NodeCount);
			//注意此处区分平衡节点。
			return NodeIndex < NodeCount - 1 ? CurrentAnswer(NodeIndex) : 0;
		}

		void NRSolver::EvalPowerInjection()
		{
			//TODO 计入导纳矩阵可能的不对称性
			//计算各节点的实际注入功率，以及功率偏差 PowerInjectionDeviation
			PowerInjectionDeviation = ConstraintPowerInjection;
			for (auto& node : PSolution->NodeStatus()) node.ClearPowerInjection();
			//遍历所有节点，包括平衡节点
			//此处使用 for 而非 for-each 是为了与数学表达保持一致
			auto &Admittance = PNetwork->Admittance;
			for (int m = 0; m < NodeCount; m++)
			{
				auto &statusM = PSolution->NodeStatus(m);
				auto Um = NodeVoltage(m);
				auto thetaM = NodeAngle(m);
				int subM = Block1EquationCount() + statusM.SubIndex();
				//计算导纳矩阵中非对角元素对应的功率。
				for (int n = m + 1; n < NodeCount; n++)
				{
					// 导纳矩阵是上三角矩阵，row < col
					// TODO 取消对导纳矩阵对称的假定（移相变压器）。
					complexd Y = Admittance.coeff(m, n);
					complexd Y1 = Admittance.coeff(n, m);
					//if (abs(Y) < 1e-10) continue;
					auto UmUn = Um * NodeVoltage(n);
					auto thetaMn = thetaM - NodeAngle(n);
					auto sinMn = sin(thetaMn);
					auto cosMn = cos(thetaMn);
					auto &statusN = PSolution->NodeStatus(n);
					//累计上一次迭代结果对应的注入功率
					statusM.AddPowerInjections(UmUn * (Y.real() * cosMn + Y.imag() * sinMn),
						UmUn * (Y.real() * sinMn - Y.imag() * cosMn));
					statusN.AddPowerInjections(UmUn * (Y1.real() * cosMn - Y1.imag() * sinMn),
						UmUn * (-Y1.real() * sinMn - Y1.imag() * cosMn));
				}
				//计算导纳矩阵中对角元素对应的功率。
				auto UmSqr = Um * Um;
				auto Y = Admittance.coeff(m, m);
				statusM.AddPowerInjections(UmSqr * Y.real(), -UmSqr * Y.imag());
				//生成功率偏差向量 Δy'
				if (statusM.Type() != NodeType::SlackNode)
				{
					PowerInjectionDeviation(m) -= statusM.ActivePowerInjection();
					if (statusM.Type() == NodeType::PQNode)
						PowerInjectionDeviation(subM) -= statusM.ReactivePowerInjection();
				}
			}
			_PS_TRACE("平衡节点 ======");
			_PS_TRACE("有功注入：" << PSolution->NodeStatus(NodeCount - 1).ActivePowerInjection());
			_PS_TRACE("无功注入：" << PSolution->NodeStatus(NodeCount - 1).ReactivePowerInjection());
			_PS_TRACE("偏差 deltaY ==========");
			_PS_TRACE(PowerInjectionDeviation);
		}
		
		void NRSolver::GenerateJacobian()
		{
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
			auto& Admittance = PNetwork->Admittance;
			for (int m = 0; m < NodeCount - 1; m++)
			{
				auto& nodeM = PSolution->NodeStatus(m);
				double Um = NodeVoltage(m);
				double thetaM = NodeAngle(m);
				int subM = Block1EquationCount() + nodeM.SubIndex();
				//计算非对角元素
				for (int n = m + 1; n < NodeCount - 1; n++)
				{
					complexd Ymn = Admittance.coeff(m, n);
					complexd Ynm = Admittance.coeff(m, n);
					//TODO 改为根据矩阵内部保存的索引来进行选择性循环。
					if (abs(Ymn) < 1e-20 && abs(Ynm) < 1e-20) continue;
					double UmUn = Um * NodeVoltage(n);
					double thetaMn = thetaM - NodeAngle(n);
					double sinMn = sin(thetaMn);
					double cosMn = cos(thetaMn);
					//H(m,n)
					double H = Jocobian.coeffRef(m, n) = -UmUn * (Ymn.real() * sinMn - Ymn.imag() * cosMn);
					//H(n,m)
					double Hp = Jocobian.coeffRef(n, m) = UmUn * (Ynm.real() * sinMn + Ynm.imag() * cosMn);
					//i.e. H(n,m) = -UmUn * (-Y.real() * sinMn - Y.imag() * cosMn)
					//N
					double N = -UmUn * (Ymn.real() * cosMn + Ymn.imag() * sinMn);
					double Np = -UmUn * (Ynm.real() * cosMn - Ynm.imag() * sinMn);
					//cout << "::" << m << "," << n << " = " << Jocobian.coeffRef(1, 0) << endl;
					auto& nodeN = PSolution->NodeStatus(n);
					int subN = Block1EquationCount() + nodeN.SubIndex();
					if (nodeM.Type() == NodeType::PQNode)
					{
						if (nodeN.Type() == NodeType::PQNode)
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
						if (nodeN.Type() == NodeType::PQNode)
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
				Jocobian.coeffRef(m, m) = UmSqr * Y.imag() + nodeM.ReactivePowerInjection();
				if (nodeM.Type() == NodeType::PQNode)
				{
					//N
					Jocobian.coeffRef(m, subM) = -UmSqr * Y.real() - nodeM.ActivePowerInjection();
					//M
					Jocobian.coeffRef(subM, m) = UmSqr * Y.real() - nodeM.ActivePowerInjection();
					//L
					Jocobian.coeffRef(subM, subM) = UmSqr * Y.imag() - nodeM.ReactivePowerInjection();
				}
			}
			_PS_TRACE("雅可比矩阵 ==========");
			_PS_TRACE(Jocobian);
		}

		bool NRSolver::GenerateNextAnswer()
		{
			SparseLU<SparseMatrix<double>> solver;
			//求解矩阵方程
			Jocobian.makeCompressed();
			solver.compute(Jocobian);
			if (solver.info() != Eigen::Success) return false;
			CorrectionAnswer = solver.solve(PowerInjectionDeviation);
			if (solver.info() != Eigen::Success) return false;
			//计算新的结果
			//注意到 Δy = -J Δx
			//而此处实际解的方程组为 Δy' = J Δx
			//也就是说，Δy = -Δy'
			//Δx = [theta ... theta V ... V]
			CurrentAnswer.head(Block1EquationCount()) -= CorrectionAnswer.head(Block1EquationCount());
			CurrentAnswer.tail(Block2EquationCount()) -= CurrentAnswer.tail(Block2EquationCount()).cwiseProduct(CorrectionAnswer.tail(Block2EquationCount()));
			_PS_TRACE("当前解向量 ==========");
			_PS_TRACE(CurrentAnswer);
			return true;
		}
	}
}
