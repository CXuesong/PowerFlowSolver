
#ifndef __POWERSOLUTIONS_POWERFLOWSOLUTION_H
#define __POWERSOLUTIONS_POWERFLOWSOLUTION_H

#include "PowerSolutions.h"
#include "ObjectModel.h"
#include "Utility.h"
#include <unordered_map>

namespace PowerSolutions
{
	namespace PowerFlow
	{
		struct NodeFlowSolution		//�ڵ㳱�����
		{
		private:
			complexd m_Voltage;
			complexd m_PowerGeneration;
			complexd m_PowerConsumption;
		public:
			PowerSolutions::complexd Voltage() const { return m_Voltage; }
			void Voltage(PowerSolutions::complexd val) { m_Voltage = val; }
			PowerSolutions::complexd PowerGeneration() const { return m_PowerGeneration; }
			void PowerGeneration(PowerSolutions::complexd val) { m_PowerGeneration = val; }
			PowerSolutions::complexd PowerConsumption() const { return m_PowerConsumption; }
			void PowerConsumption(PowerSolutions::complexd val) { m_PowerConsumption = val; }
		public:
			NodeFlowSolution(complexd voltage, complexd powerGeneration, complexd powerConsumption)
				: m_Voltage(voltage), m_PowerGeneration(powerGeneration), m_PowerConsumption(powerConsumption)
			{ }
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
			bool ReversedDirection() const	//ָʾ���ʵ�ʵ�ʴ��䷽���Ƿ���Լ���ķ���Bus1->Bus2���෴��
			{
				return m_Power1.real() < 0;
			}
			complexd PowerTransfer() const	//��ȡʵ�ʴ���Ĺ��ʴ�С��
			{
				//ע�⵽�˴�Ϊע���������Ĺ���
				//"���书��"Ϊ������������Ĺ���
				//assert((power1.real() >= 0) ^ (power2.real() > 0));
				//��ĳЩ������������£����ܻ������������ʧ�ܵ������
				return m_Power1.real() > 0 ? -m_Power2 : -m_Power1;
			}
			complexd PowerLoss() const		//�����ڵ�֮�䴫�����ʱ��ʧ���ܹ��ʣ������ӵ�֧·����
			{
				return m_Power1 + m_Power2;
			}
			complexd PowerShunt() const		//�����ڵ�Ľӵ�֧·����ʧ���ܹ��ʡ�
			{
				return m_ShuntPower1 + m_ShuntPower2;
			}
			BranchFlowSolution& operator+=(const BranchFlowSolution& y)
			{
				m_Power1 += y.m_Power1;
				m_Power2 += y.m_Power2;
				m_ShuntPower1 += y.m_ShuntPower1;
				m_ShuntPower2 += y.m_ShuntPower2;
				return *this;
			}
			BranchFlowSolution(complexd power1, complexd power2, complexd shuntPower1, complexd shuntPower2)
				: m_Power1(power1), m_Power2(power2), m_ShuntPower1(shuntPower1), m_ShuntPower2(shuntPower2)
			{ }
		};
		// ������յĽ���
		enum class SolutionStatus : byte
		{
			Success = 0x00,					//���ɹ�������
			MaxIteration = 0x10,			//�Ѿ��ﵽ�������ĵ���������
			IterationFailed = 0x11,			//�������̳������⡣
			IntelliIterationAbort = 0x12,	//�������ܵ���������������̲����������жϡ�
		};

		//��������̬�����ķ������档
		class Solution
		{
		private:
			typedef std::unordered_map<ObjectModel::Bus*, NodeFlowSolution> m_NodeFlowCollection;
			typedef m_NodeFlowCollection::const_iterator NodeFlowIterator;
			typedef std::unordered_map<std::pair<ObjectModel::Bus*, ObjectModel::Bus*>, BranchFlowSolution,
				Utility::UnorderedPairHasher<ObjectModel::Bus*>,
				Utility::UnorderedPairEqualityComparer<ObjectModel::Bus*>> m_BranchFlowCollection;
			typedef m_BranchFlowCollection::const_iterator BranchFlowIterator;
			typedef std::unordered_map<ObjectModel::Component*, BranchFlowSolution> m_ComponentFlowCollection;
			typedef m_ComponentFlowCollection::const_iterator ComponentFlowIterator;
		public:
			typedef readonly_map<m_NodeFlowCollection> NodeFlowCollection;
			typedef readonly_map<m_BranchFlowCollection> BranchFlowCollection;
			typedef readonly_map<m_ComponentFlowCollection> ComponentFlowCollection;
		private:
			m_NodeFlowCollection m_NodeFlow;				//�ڵ㳱����Ϣ��
			m_ComponentFlowCollection m_ComponentFlow;	//��ÿԪ����֧·������Ϣ��
			m_BranchFlowCollection m_BranchFlow;			//���ڵ��Ŷԣ�֧·������Ϣ��
			complexd m_TotalPowerGeneration;
			complexd m_TotalPowerConsumption;
			complexd m_TotalPowerLoss;
			complexd m_TotalPowerShunt;
			SolutionStatus m_Status;
			int m_IterationCount;
			double m_MaxDeviation;
		private:	//Internal
			friend class SolverImpl;
			void TotalPowerGeneration(complexd val) { m_TotalPowerGeneration = val; }
			void TotalPowerConsumption(complexd val) { m_TotalPowerConsumption = val; }
			void TotalPowerLoss(complexd val) { m_TotalPowerLoss = val; }
			void TotalPowerShunt(complexd val) { m_TotalPowerShunt = val; }
			void Status(SolutionStatus val) { m_Status = val; }
			void IterationCount(int val) { m_IterationCount = val; }
			void MaxDeviation(double val) { m_MaxDeviation = val; }
			void AddNodeFlow(ObjectModel::Bus* node, const NodeFlowSolution& solution);
			void AddComponentFlow(ObjectModel::Component* c, const BranchFlowSolution& solution);
			void AddBranchFlow(ObjectModel::Bus* node1, ObjectModel::Bus* node2, const BranchFlowSolution& solution);
		public:
			complexd TotalPowerGeneration() const { return m_TotalPowerGeneration; }
			complexd TotalPowerConsumption() const { return m_TotalPowerConsumption; }
			complexd TotalPowerLoss() const { return m_TotalPowerLoss; }
			complexd TotalPowerShunt() const { return m_TotalPowerShunt; }
			SolutionStatus Status() const { return m_Status; }
			int IterationCount() const { return m_IterationCount; }
			double MaxDeviation() const { return m_MaxDeviation; }
		public:
			NodeFlowIterator NodeFlowBegin() const { return m_NodeFlow.cbegin(); }
			NodeFlowIterator NodeFlowEnd() const { return m_NodeFlow.cend(); }
			std::size_t NodeFlowCount() const { return m_NodeFlow.size(); }
			const NodeFlowSolution* NodeFlow(ObjectModel::Bus* node) const
			{
				auto i = m_NodeFlow.find(node);
				if (i != m_NodeFlow.end()) return &(i->second);
				return nullptr;
			}
			ComponentFlowIterator ComponentFlowBegin() const { return m_ComponentFlow.cbegin(); }
			ComponentFlowIterator ComponentFlowEnd() const { return m_ComponentFlow.cend(); }
			std::size_t ComponentFlowCount() const { return m_ComponentFlow.size(); }
			const BranchFlowSolution* ComponentFlow(ObjectModel::Component* component) const
			{
				auto i = m_ComponentFlow.find(component);
				if (i != m_ComponentFlow.end()) return &(i->second);
				return nullptr;
			}
			BranchFlowIterator BranchFlowBegin() const { return m_BranchFlow.cbegin(); }
			BranchFlowIterator BranchFlowEnd() const { return m_BranchFlow.cend(); }
			std::size_t BranchFlowCount() const { return m_BranchFlow.size(); }
			const BranchFlowSolution* BranchFlow(ObjectModel::BusPair branch) const
			{
				auto i = m_BranchFlow.find(branch);
				if (i != m_BranchFlow.end()) return &(i->second);
				return nullptr;
			}
		protected:	//internal
			Solution();
		};
	}
}

#endif	//__POWERSOLUTIONS_POWERFLOWSOLUTION_H
