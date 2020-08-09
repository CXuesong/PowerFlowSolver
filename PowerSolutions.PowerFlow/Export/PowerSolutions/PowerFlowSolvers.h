
#ifndef __POWERSOLUTIONS_POWERFLOWSOLVERS_H
#define __POWERSOLUTIONS_POWERFLOWSOLVERS_H

#include "NetworkCase.h"
#include "PowerFlowSolution.h"
#include <memory>

namespace PowerSolutions
{
	namespace PowerFlow
	{
		enum class SolverType
		{
			NewtonRaphson = 0,
			FastDecoupled = 1
		};

		// �������������ʱ��״̬��Ϣ��
		struct SolverStatus
		{
		private:
			bool m_IsIterating;
			int m_LastIterationCount;
			IterationInfo m_LastIterationInfo;
		public:
			// ��ȡһ��ֵ��ָʾ�˵�ǰ�Ƿ����ڽ��е�����ע�⣺�����ʱ���ڽ��е���ǰ��������������Ҳ�᷵�� false��
			bool IsIterating() const { return m_IsIterating; }
			// ��ȡ�Ѿ���ɵĵ���������
			int LastIterationCount() const { return m_LastIterationCount; }
			// ��ȡ��һ�ε������������
			const IterationInfo& LastIterationInfo() const { return m_LastIterationInfo; }
		public:
			// ��ʼ��һ����ʾ��ǰδ�ڽ������� SolverStatus��
			SolverStatus()
				: m_IsIterating(false), m_LastIterationCount(0), m_LastIterationInfo(0)
			{ }
			// ��ʼ��һ����ʾ��ǰ���ڽ������� SolverStatus��
			SolverStatus(int lastIterationCount, const IterationInfo& lastIterationInfo)
				: m_IsIterating(true), m_LastIterationCount(lastIterationCount), m_LastIterationInfo(lastIterationInfo)
			{ }
		};

		// �������������̬�����Ľ�����̡�
		class Solver
		{
		private:
			int m_MaxIterations;				//���ĵ���������
			double m_MaxDeviationTolerance;		//���������
			bool m_NodeReorder;					//����ڵ���������
			bool m_IntelliIterations;			//������������ٶ����ж��Ƿ��б�Ҫ����������
		public:
			int MaxIterations() const { return m_MaxIterations; }
			void MaxIterations(int val) { m_MaxIterations = val; }
			double MaxDeviationTolerance() const { return m_MaxDeviationTolerance; }
			void MaxDeviationTolerance(double val) { m_MaxDeviationTolerance = val; }
			bool NodeReorder() const { return m_NodeReorder; }
			void NodeReorder(bool val) { m_NodeReorder = val; }
			bool IntelliIterations() const { return m_IntelliIterations; }
			void IntelliIterations(bool val) { m_IntelliIterations = val; }
		public:
			// �������Ĺ��ʳ����ֲ���������һ�������������档
			virtual Solution* Solve(ObjectModel::NetworkCase* CaseInfo) = 0;
			// ��ȡ��ǰ�����״̬��
			// ���������У�����ʹ����һ���̵߳��ô˺����Բ�ѯ״̬��
			virtual SolverStatus GetStatus() = 0;
			Solver();
			virtual ~Solver();
		public:
			static Solver* Create(SolverType type);
		};
	}
}

#endif	//__POWERSOLUTIONS_POWERFLOWSOLVERS_H
