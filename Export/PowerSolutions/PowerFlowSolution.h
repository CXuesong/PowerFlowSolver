
#ifndef __POWERSOLUTIONS_POWERFLOWSOLUTION_H
#define __POWERSOLUTIONS_POWERFLOWSOLUTION_H

#include "PowerSolutions.h"
#include "Utility.h"
#include <unordered_map>

namespace PowerSolutions
{
	namespace PowerFlow
	{
		struct NodeFlowSolution		//�ڵ㳱�����
		{
		private:
			complexd m_PowerGeneration;
			complexd m_PowerConsumption;
		public:
			PowerSolutions::complexd PowerGeneration() const { return m_PowerGeneration; }
			void PowerGeneration(PowerSolutions::complexd val) { m_PowerGeneration = val; }
			PowerSolutions::complexd PowerConsumption() const { return m_PowerConsumption; }
			void PowerConsumption(PowerSolutions::complexd val) { m_PowerConsumption = val; }
		};
		struct BranchFlowSolution		//��ÿԪ����ڵ��Ŷԣ�֧·�������
		{
			//���飺֧·���	I��ڵ�����	J��ڵ�����		I��ע���й�	I��ע���޹�	J��ע���й�	J��ע���޹�	֧·�й����	֧·�޹����
			// Power1          Power2
			// --->--------------<----
			//       |       |
			// S.P.1 |       | S.P.2
			//       |       |
		private:
			complexd m_Power1;		//�ӽڵ�1ע��Ĺ���
			complexd m_Power2;		//�ӽڵ�2ע��Ĺ���
			complexd m_ShuntPower1;	//�ӽڵ�1ע��ӵ�֧·�Ĺ���
			complexd m_ShuntPower2;	//�ӽڵ�2ע��ӵ�֧·�Ĺ���
		public:
			PowerSolutions::complexd Power1() const { return m_Power1; }
			void Power1(PowerSolutions::complexd val) { m_Power1 = val; }
			PowerSolutions::complexd Power2() const { return m_Power2; }
			void Power2(PowerSolutions::complexd val) { m_Power2 = val; }
			PowerSolutions::complexd ShuntPower1() const { return m_ShuntPower1; }
			void ShuntPower1(PowerSolutions::complexd val) { m_ShuntPower1 = val; }
			PowerSolutions::complexd ShuntPower2() const { return m_ShuntPower2; }
			void ShuntPower2(PowerSolutions::complexd val) { m_ShuntPower2 = val; }
			bool ReversedDirection()	//ָʾ���ʵ�ʵ�ʴ��䷽���Ƿ���Լ���ķ���Bus1->Bus2���෴��
			{
				return m_Power1.real() < 0;
			}
			complexd PowerTransfer()	//��ȡʵ�ʴ���Ĺ��ʴ�С��
			{
				//ע�⵽�˴�Ϊע���������Ĺ���
				//"���书��"Ϊ������������Ĺ���
				//assert((power1.real() >= 0) ^ (power2.real() > 0));
				//��ĳЩ������������£����ܻ������������ʧ�ܵ������
				return m_Power1.real() > 0 ? -m_Power2 : -m_Power1;
			}
			complexd PowerLoss()		//�����ڵ�֮�䴫�����ʱ��ʧ���ܹ��ʡ�
			{
				return m_Power1 + m_Power2;
			}
			complexd PowerShunt()		//�����ڵ�Ľӵ�֧·����ʧ���ܹ��ʡ�
			{
				return m_ShuntPower1 + m_ShuntPower2;
			}
			BranchFlowSolution(complexd power1, complexd power2, complexd shuntPower1, complexd shuntPower2)
				: m_Power1(power1), m_Power2(power2), m_ShuntPower1(shuntPower1), m_ShuntPower2(shuntPower2)
			{ }
		};
		// ������յĽ���
		enum class SolutionStatus
		{
			Success = 0x00,					//���ɹ�������
			MaxIteration = 0x10,			//�Ѿ��ﵽ�������ĵ���������
			IterationFailed = 0x11,			//�������̳������⡣
			IntelliIterationAbort = 0x12,	//�������ܵ���������������̲����������жϡ�
		};

		//��������̬�����ķ������档
		class Solution
		{
			typedef std::unordered_map<int, NodeFlowSolution> NodeFlowCollection;
			typedef NodeFlowCollection::const_iterator NodeFlowIterator;
			typedef std::unordered_map<std::pair<int, int>, BranchFlowSolution,
				Utility::UnorderedPairHasher<int>, 
				Utility::UnorderedPairEqualityComparer<int>> BranchFlowCollection;
			typedef BranchFlowCollection::const_iterator BranchFlowIterator;
			typedef std::unordered_map<int, BranchFlowSolution> ComponentFlowCollection;
			typedef ComponentFlowCollection::const_iterator ComponentFlowIterator;
			NodeFlowCollection m_NodeFlow;				//�ڵ㳱����Ϣ��
			ComponentFlowCollection m_ComponentFlow;	//��ÿԪ����֧·������Ϣ��
			BranchFlowCollection m_BranchFlow;			//���ڵ��Ŷԣ�֧·������Ϣ��
			double m_PowerDeviation;
			double PowerDeviation() const { return m_PowerDeviation; }
			void PowerDeviation(double val) { m_PowerDeviation = val; }
			complexd m_TotalPowerGeneration;
			complexd m_TotalPowerConsumption;
			complexd m_TotalPowerLoss;
			complexd m_TotalPowerShunt;
			SolutionStatus m_Status;
		private:	//Internal
			friend class Solver;
			void TotalPowerGeneration(complexd val) { m_TotalPowerGeneration = val; }
			void TotalPowerConsumption(complexd val) { m_TotalPowerConsumption = val; }
			void TotalPowerLoss(complexd val) { m_TotalPowerLoss = val; }
			void TotalPowerShunt(complexd val) { m_TotalPowerShunt = val; }
			void Status(SolutionStatus val) { m_Status = val; }
		public:
			complexd TotalPowerGeneration() const { return m_TotalPowerGeneration; }
			complexd TotalPowerConsumption() const { return m_TotalPowerConsumption; }
			complexd TotalPowerLoss() const { return m_TotalPowerLoss; }
			complexd TotalPowerShunt() const { return m_TotalPowerShunt; }
			SolutionStatus Status() const { return m_Status; }
		public:
			NodeFlowIterator NodeFlowBegin() const { return m_NodeFlow.cbegin(); }
			NodeFlowIterator NodeFlowEnd() const { return m_NodeFlow.cend(); }
			size_t NodeFlowCount() const { return m_NodeFlow.size(); }
			ComponentFlowIterator ComponentFlowBegin() const { return m_ComponentFlow.cbegin(); }
			ComponentFlowIterator ComponentFlowEnd() const { return m_ComponentFlow.cend(); }
			size_t ComponentFlowCount() const { return m_ComponentFlow.size(); }
			BranchFlowIterator BranchFlowBegin() const { return m_BranchFlow.cbegin(); }
			BranchFlowIterator BranchFlowEnd() const { return m_BranchFlow.cend(); }
			size_t BranchFlowCount() const { return m_BranchFlow.size(); }
		};
	}
}

#endif	//__POWERSOLUTIONS_POWERFLOWSOLUTION_H