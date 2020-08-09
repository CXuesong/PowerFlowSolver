
#ifndef __POWERSOLUTIONS_POWERFLOWSOLVERS_H
#define __POWERSOLUTIONS_POWERFLOWSOLVERS_H

#include "PowerSolutions.h"
#include "NetworkCase.h"
#include "PowerFlowSolution.h"
#include <memory>

namespace PowerSolutions
{
	namespace PowerFlow
	{
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
			virtual std::shared_ptr<Solution*> Solve(ObjectModel::NetworkCase* CaseInfo) = 0;
			Solver();
			virtual ~Solver();
		};
	}
}

#endif	//__POWERSOLUTIONS_POWERFLOWSOLVERS_H