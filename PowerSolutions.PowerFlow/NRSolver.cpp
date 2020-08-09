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
			//PQ�ڵ㣺[dP dQ] = J * [V theta]
			//PV�ڵ㣺[dP] = J * [theta]
			//       PQ/PV    |   PQ
			//��y = [dP ... dP | dQ ... dQ] = ConstraintPower - CurrentPower
			//��x = [theta ... theta | V ... V]
			// �ֽ���/��ʼ������
			// H[0,0], N[NodeCount - 1, 0]
			//��ʼ������ά��
			//ע��������Ҫ�ֶ�����
			ConstraintPowerInjection.resize(EquationCount());
			CurrentAnswer.resize(EquationCount());
			CorrectionAnswer.resize(EquationCount());
			PowerInjectionDeviation.resize(EquationCount());

			//ȷ���ſɱȾ���ÿһ����Ҫ�ķ���Ԫ�ռ�����
			//��������������������ſɱȾ���Ŀռ�
			//���������ռ����
			vector<int> JocobianColSpace;		//�ſɱȾ�����ÿһ�еķ���Ԫ������������Ϊ����Ԥ���ռ䡣
			JocobianColSpace.resize(EquationCount());
			for (int n = 0; n < Block1EquationCount(); n++)
			{
				auto *node = PNetwork->Nodes()[n];
				JocobianColSpace[n] = min(node->Degree() * 2, EquationCount());
				if (node->Type == NodeType::PQNode)
				{
					JocobianColSpace[Block1EquationCount() + node->SubIndex] = min(node->Degree() * 2, EquationCount());
				}
			}
			Jocobian.resize(EquationCount(), EquationCount());
			Jocobian.reserve(JocobianColSpace);

			//����Ŀ��ע�빦������ y���Լ�������ֵ������
			for (auto& node : PNetwork->Nodes())
			{
				if (node->Type != NodeType::SlackNode)
				{
					int subIndex = Block1EquationCount() + node->SubIndex;
					ConstraintPowerInjection(node->Index) = node->ActivePowerInjection;
					CurrentAnswer(node->Index) = arg(node->Bus->InitialVoltage());
					if (node->Type == NodeType::PQNode)
					{
						ConstraintPowerInjection(subIndex) = node->ReactivePowerInjection;
						CurrentAnswer(subIndex) = abs(node->Bus->InitialVoltage());
					}
				}
			}

			_PS_TRACE("Ŀ�꺯��ֵ Y ==========");
			_PS_TRACE(ConstraintPowerInjection);
		}

		double NRSolver::EvalDeviation()
		{
			EvalPowerInjection();
			//BUG CLOSED
			//��Сϵ����һ���Ǿ���ֵ��С��ϵ��
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
			{
				VoltageVector(i) = NodeVoltage(i);
				AngleVector(i) = NodeAngle(i);
			}
		}

		inline double NRSolver::NodeVoltage(int NodeIndex)
		{
			auto *node = PNetwork->Nodes(NodeIndex);
			if (node->Type == NodeType::PQNode)
				return CurrentAnswer(Block1EquationCount() + node->SubIndex);
			else
				return node->Voltage;
		}

		inline double NRSolver::NodeAngle(int NodeIndex)
		{
			assert(NodeIndex < NodeCount);
			//ע��˴�����ƽ��ڵ㡣
			return NodeIndex < NodeCount - 1 ? CurrentAnswer(NodeIndex) : 0;
		}

		void NRSolver::EvalPowerInjection()
		{
			//TODO ���뵼�ɾ�����ܵĲ��Գ���
			//������ڵ��ʵ��ע�빦�ʣ��Լ�����ƫ�� PowerInjectionDeviation
			PowerInjectionDeviation = ConstraintPowerInjection;
			for (auto& node : PNetwork->Nodes()) node->ClearPowerInjections();
			//�������нڵ㣬����ƽ��ڵ�
			//�˴�ʹ�� for ���� for-each ��Ϊ������ѧ��ﱣ��һ��
			auto &Admittance = PNetwork->Admittance;
			for (int m = 0; m < NodeCount; m++)
			{
				auto *nodeM = PNetwork->Nodes()[m];
				double Um = NodeVoltage(m);
				double thetaM = NodeAngle(m);
				int subM = Block1EquationCount() + nodeM->SubIndex;
				//����ǶԽ�Ԫ��
				for (int n = m + 1; n < NodeCount; n++)
				{
					// ���ɾ����������Ǿ���row < col
					complexd Y = Admittance.coeff(m, n);
					//if (abs(Y) < 1e-10) continue;
					double UmUn = Um * NodeVoltage(n);
					double thetaMn = thetaM - NodeAngle(n);
					double sinMn = sin(thetaMn);
					double cosMn = cos(thetaMn);
					auto *nodeN = PNetwork->Nodes()[n];
					//�ۼ���һ�ε��������Ӧ��ע�빦��
					nodeM->ActivePowerInjection += UmUn * (Y.real() * cosMn + Y.imag() * sinMn);
					nodeM->ReactivePowerInjection += UmUn * (Y.real() * sinMn - Y.imag() * cosMn);
					nodeN->ActivePowerInjection += UmUn * (Y.real() * cosMn - Y.imag() * sinMn);
					nodeN->ReactivePowerInjection -= UmUn * (Y.real() * sinMn + Y.imag() * cosMn);
				}
				//����Խ�Ԫ��
				double UmSqr = Um * Um;
				complexd Y = Admittance.coeff(m, m);
				nodeM->ActivePowerInjection += UmSqr * Y.real();
				nodeM->ReactivePowerInjection -= UmSqr * Y.imag();
				//���ɹ���ƫ������ ��y'
				if (nodeM->Type != NodeType::SlackNode)
				{
					PowerInjectionDeviation(m) -= nodeM->ActivePowerInjection;
					if (nodeM->Type == NodeType::PQNode)
						PowerInjectionDeviation(subM) -= nodeM->ReactivePowerInjection;
				}
			}
			_PS_TRACE("ƽ��ڵ� ======");
			_PS_TRACE("�й�ע�룺" << PNetwork->SlackNode()->ActivePowerInjection);
			_PS_TRACE("�޹�ע�룺" << PNetwork->SlackNode()->ReactivePowerInjection);
			_PS_TRACE("ƫ�� deltaY ==========");
			_PS_TRACE(PowerInjectionDeviation);
		}
		
		void NRSolver::GenerateJacobian()
		{
			Jocobian.setZero();
			//PQ�ڵ㣺[dP dQ] = J * [V theta]
			//PV�ڵ㣺[dP] = J * [theta]
			//       PQ/PV        PQ
			//��y = [dP ... dP dQ ... dQ]
			//��x = [theta ... theta V ... V]
			//    / H | N \
			//J = | --+-- |
			//    \ M | L /
			// ��ʼ��������ע���ų�ƽ��ڵ㣩
			// H[0,0], N[NodeCount - 1, 0]
			// M[NodeCount - 1, 0], L[..., ...]
			// �ԷǶԽ�Ԫ��
			// H =  L
			// N = -M

			//TODO ���ڽڵ������Ե�ϡ���ԣ����Բ���ѭ�����ꡣ
			for (int m = 0; m < NodeCount - 1; m++)
			{
				auto *nodeM = PNetwork->Nodes()[m];
				double Um = NodeVoltage(m);
				double thetaM = NodeAngle(m);
				int subM = Block1EquationCount() + nodeM->SubIndex;
				//����ǶԽ�Ԫ��
				auto& Admittance = PNetwork->Admittance;
				for (int n = m + 1; n < NodeCount - 1; n++)
				{
					// �����Ǿ���row < col
					complexd Y = Admittance.coeff(m, n);
					if (abs(Y) < 1e-10) continue;
					double UmUn = Um * NodeVoltage(n);
					double thetaMn = thetaM - NodeAngle(n);
					double sinMn = sin(thetaMn);
					double cosMn = cos(thetaMn);
					//H(m,n)
					double H = Jocobian.coeffRef(m, n) = -UmUn * (Y.real() * sinMn - Y.imag() * cosMn);
					//H(n,m)
					double Hp = Jocobian.coeffRef(n, m) = UmUn * (Y.real() * sinMn + Y.imag() * cosMn);
					//i.e. H(n,m) = -UmUn * (-Y.real() * sinMn - Y.imag() * cosMn)
					//N
					double N = -UmUn * (Y.real() * cosMn + Y.imag() * sinMn);
					double Np = -UmUn * (Y.real() * cosMn - Y.imag() * sinMn);
					//cout << "::" << m << "," << n << " = " << Jocobian.coeffRef(1, 0) << endl;
					auto *nodeN = PNetwork->Nodes()[n];

					int subN = Block1EquationCount() + nodeN->SubIndex;
					if (nodeM->Type == NodeType::PQNode)
					{
						if (nodeN->Type == NodeType::PQNode)
						{
							//PQ-PQ��������жԳ���
							// N
							Jocobian.coeffRef(m, subN) = N;
							Jocobian.coeffRef(n, subM) = Np;
							// M = -N
							Jocobian.coeffRef(subM, n) = -N;
							//BUG CLOSED
							//Jocobian.coeffRef(subM, n) = -Np;
							//����ȷ���ſɱȾ���Ԫ������
							//��ӡ�
							Jocobian.coeffRef(subN, m) = -Np;
							// L = H
							Jocobian.coeffRef(subM, subN) = H;
							Jocobian.coeffRef(subN, subM) = Hp;
						} else {
							//PQ-PV��m,n��
							// M = -N
							Jocobian.coeffRef(subM, n) = -N;
							//PV-PQ��n,m��
							// N
							Jocobian.coeffRef(n, subM) = Np;
						}
					} else {
						if (nodeN->Type == NodeType::PQNode)
						{
							//PV-PQ��m,n��
							// N
							Jocobian.coeffRef(m, subN) = N;
							//PQ-PV��n,m��
							// M
							Jocobian.coeffRef(subN, m) = -Np;
						}
					}
				}
				complexd Y = Admittance.coeffRef(m, m);
				double UmSqr = Um * Um;
				//����Խ�Ԫ��
				//H
				Jocobian.coeffRef(m, m) = UmSqr * Y.imag() + nodeM->ReactivePowerInjection;
				if (nodeM->Type == NodeType::PQNode)
				{
					//N
					Jocobian.coeffRef(m, subM) = -UmSqr * Y.real() - nodeM->ActivePowerInjection;
					//M
					Jocobian.coeffRef(subM, m) = UmSqr * Y.real() - nodeM->ActivePowerInjection;
					//L
					Jocobian.coeffRef(subM, subM) = UmSqr * Y.imag() - nodeM->ReactivePowerInjection;
				}
			}
			_PS_TRACE("�ſɱȾ��� ==========");
			_PS_TRACE(Jocobian);
		}

		bool NRSolver::GenerateNextAnswer()
		{
			SparseLU<SparseMatrix<double>> solver;
			//�����󷽳�
			Jocobian.makeCompressed();
			solver.compute(Jocobian);
			if (solver.info() != Eigen::Success) return false;
			CorrectionAnswer = solver.solve(PowerInjectionDeviation);
			if (solver.info() != Eigen::Success) return false;
			//�����µĽ��
			//ע�⵽ ��y = -J ��x
			//���˴�ʵ�ʽ�ķ�����Ϊ ��y' = J ��x
			//Ҳ����˵����y = -��y'
			//��x = [theta ... theta V ... V]
			CurrentAnswer.head(Block1EquationCount()) -= CorrectionAnswer.head(Block1EquationCount());
			CurrentAnswer.tail(Block2EquationCount()) -= CurrentAnswer.tail(Block2EquationCount()).cwiseProduct(CorrectionAnswer.tail(Block2EquationCount()));
			_PS_TRACE("������ ==========");
			_PS_TRACE(CurrentAnswer);
			return true;
		}
	}
}
