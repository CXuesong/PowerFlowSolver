
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
		class IterationEventArgs
		{
		private:
			int m_IterationCount;
			double m_MaxDeviation;
		public:
			// ��ȡ�Ѿ���ɵĵ���������0��ʾ������δ��ʼ��
			int IterationCount() const { return m_IterationCount; }
			// �˴ε�������ʱ��������ľ���ֵ��
			double MaxDeviation() const { return m_MaxDeviation; }
		public:
			// ��ʼ��һ����ʾ��ǰ���ڽ������� SolverStatus��
			IterationEventArgs(int iterationCount, double maxDeviation)
				: m_IterationCount(iterationCount), m_MaxDeviation(maxDeviation)
			{ }
		};

		// �����ڵ��������н���ÿһ���ĵ�����Ϣ��
		typedef void(__stdcall *IterationEventHandler)(class Solver* sender, IterationEventArgs* e);

		// �������������̬�����Ľ�����̡�
		class Solver
		{
		private:
			int m_MaxIterations;				//���ĵ���������
			double m_MaxDeviationTolerance;		//���������
			bool m_IntelliIterations;			//������������ٶ����ж��Ƿ��б�Ҫ����������
			IterationEventHandler m_IterationEvent;	//������ÿһ����������ʱ���յ�������Ϣ��
		public:
			int MaxIterations() const { return m_MaxIterations; }
			void MaxIterations(int val) { m_MaxIterations = val; }
			double MaxDeviationTolerance() const { return m_MaxDeviationTolerance; }
			void MaxDeviationTolerance(double val) { m_MaxDeviationTolerance = val; }
			bool IntelliIterations() const { return m_IntelliIterations; }
			void IntelliIterations(bool val) { m_IntelliIterations = val; }
			IterationEventHandler IterationEvent() const { return m_IterationEvent; }
			void IterationEvent(IterationEventHandler val) { m_IterationEvent = val; }
		public:
			// �������Ĺ��ʳ����ֲ���������һ�������������档
			virtual Solution* Solve(ObjectModel::PrimitiveNetwork& network) = 0;
			Solution* Solve(ObjectModel::NetworkCase& network);
			Solver();
			virtual ~Solver();
		public:
			static Solver* Create(SolverType type);
		};
	}
}

#endif	//__POWERSOLUTIONS_POWERFLOWSOLVERS_H
