
#ifndef __POWERSOLUTIONS_POWERFLOWSOLUTION_H
#define __POWERSOLUTIONS_POWERFLOWSOLUTION_H

#include "PowerSolutions.h"
#include "ObjectModel.h"
#include "Utility.h"
#include <unordered_map>
#include <vector>

namespace PowerSolutions
{
	namespace PowerFlow
	{
		struct NodeEvaluationStatus;

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
			void AddPowerGeneration(PowerSolutions::complexd val) { m_PowerGeneration += val; }
			PowerSolutions::complexd PowerConsumption() const { return m_PowerConsumption; }
			void AddPowerConsumption(PowerSolutions::complexd val) { m_PowerConsumption += val; }
		public:
			NodeFlowSolution(complexd voltage, complexd powerInjection)
				: m_Voltage(voltage), m_PowerGeneration(powerInjection), m_PowerConsumption(0)
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
			complexd m_PowerShunt;	//��ע��ӵ�֧·�Ĺ���֮��
		public:
			PowerSolutions::complexd Power1() const { return m_Power1; }
			void Power1(PowerSolutions::complexd val) { m_Power1 = val; }
			PowerSolutions::complexd Power2() const { return m_Power2; }
			void Power2(PowerSolutions::complexd val) { m_Power2 = val; }
			PowerSolutions::complexd PowerShunt() const { return m_PowerShunt; }
			void PowerShunt(PowerSolutions::complexd val) { m_PowerShunt = val; }
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
			BranchFlowSolution& operator+=(const BranchFlowSolution& y)
			{
				m_Power1 += y.m_Power1;
				m_Power2 += y.m_Power2;
				m_PowerShunt += y.m_PowerShunt;
				return *this;
			}
			BranchFlowSolution(complexd power1, complexd power2, complexd powerShunt)
				: m_Power1(power1), m_Power2(power2), m_PowerShunt(powerShunt)
			{ }
		};

		struct ComponentFlowSolution		//ÿԪ���������
		{
			// Power0          Power1
			// --->--------------<----
			//       |       |
			// S.P.0 |       | S.P.1
			//       |       |
		private:
			std::vector<complexd> m_PowerInjection;
			complexd m_PowerShunt;
			bool m_IsUnconstrained;
		public:
			//��ָ���ڵ�ע��ĸ�ߵĹ��ʡ�
			const std::vector<complexd>& PowerInjections() const { return m_PowerInjection; }
			//��ָ���ڵ�ע��ĸ�ߵĹ��ʡ�
			complexd PowerInjections(int portIndex) const { return m_PowerInjection[portIndex]; }
			void PowerInjections(int portIndex, complexd v) { m_PowerInjection[portIndex] = v; }
			PowerSolutions::complexd PowerShunt() const { return m_PowerShunt; }
			void PowerShunt(PowerSolutions::complexd val) { m_PowerShunt = val; }
			//ָʾ��Ԫ������ĳ����Ƿ�Ϊ�����ġ�
			bool IsUnconstrained() const { return m_IsUnconstrained; }
			void IsUnconstrained(bool val) { m_IsUnconstrained = val; }
		private:
			explicit ComponentFlowSolution(bool unconst)
				: m_PowerInjection(0), m_PowerShunt(0, 0), m_IsUnconstrained(unconst)
			{ }
		public:
			//��ȡһ����ʾ���ʲ�����Ԫ�����ʡ�һ������PVԪ���С�
			static ComponentFlowSolution Unconstrained()
			{
				return ComponentFlowSolution(true);
			}
			explicit ComponentFlowSolution(int portCount)
				: m_PowerInjection(portCount), m_PowerShunt(0, 0), m_IsUnconstrained(false)
			{ }
			ComponentFlowSolution(const std::vector<complexd>&& powerInjection, complexd powerShutnt)
				: m_PowerInjection(powerInjection), m_PowerShunt(powerShutnt), m_IsUnconstrained(false)
			{ }
			ComponentFlowSolution(ComponentFlowSolution&& rhs)
				: m_PowerInjection(std::move(rhs.m_PowerInjection)),
				m_PowerShunt(rhs.m_PowerShunt), m_IsUnconstrained(rhs.m_IsUnconstrained)
			{ }
			ComponentFlowSolution& operator=(ComponentFlowSolution&& other)
			{
				m_PowerInjection = std::move(other.m_PowerInjection);
				m_PowerShunt = other.m_PowerShunt;
				m_IsUnconstrained = other.m_IsUnconstrained;
				return *this;
			}
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
		public:
			typedef std::unordered_map<const ObjectModel::Bus*, NodeFlowSolution> NodeFlowCollection;
			typedef std::unordered_map<std::pair<const ObjectModel::Bus*, const ObjectModel::Bus*>, BranchFlowSolution,
				Utility::UnorderedPairHasher<const ObjectModel::Bus*>,
				Utility::UnorderedPairEqualityComparer<const ObjectModel::Bus*>> BranchFlowCollection;
			typedef std::unordered_map<const ObjectModel::Component*, ComponentFlowSolution> ComponentFlowCollection;
		private:
			NodeFlowCollection m_NodeFlow;				//�ڵ㳱����Ϣ��
			ComponentFlowCollection m_ComponentFlow;	//��ÿԪ����֧·������Ϣ��
			BranchFlowCollection m_BranchFlow;			//���ڵ��Ŷԣ�֧·������Ϣ��
			size_t m_NodeCount;
			size_t m_PQNodeCount;
			size_t m_PVNodeCount;
			const ObjectModel::Bus* m_SlackNode;
			complexd m_TotalPowerGeneration;
			complexd m_TotalPowerConsumption;
			complexd m_TotalPowerLoss;
			complexd m_TotalPowerShunt;
			SolutionStatus m_Status;
			int m_IterationCount;
			double m_MaxDeviation;
		_PS_INTERNAL:
			void NodeCount(size_t val) { m_NodeCount = val; }
			void PQNodeCount(size_t val) { m_PQNodeCount = val; }
			void PVNodeCount(size_t val) { m_PVNodeCount = val; }
			void SlackNode(const ObjectModel::Bus* val) { m_SlackNode = val; }
			void TotalPowerGeneration(complexd val) { m_TotalPowerGeneration = val; }
			void TotalPowerConsumption(complexd val) { m_TotalPowerConsumption = val; }
			void TotalPowerLoss(complexd val) { m_TotalPowerLoss = val; }
			void TotalPowerShunt(complexd val) { m_TotalPowerShunt = val; }
			void Status(SolutionStatus val) { m_Status = val; }
			void IterationCount(int val) { m_IterationCount = val; }
			void MaxDeviation(double val) { m_MaxDeviation = val; }
			void AddNodeFlow(const ObjectModel::Bus* node, const NodeEvaluationStatus& status);
			void AddComponentFlow(const ObjectModel::Component* c, ComponentFlowSolution&& solution);
			void AddBranchFlow(const ObjectModel::Bus* node1, const ObjectModel::Bus* node2, const BranchFlowSolution& solution);
		public:
			size_t NodeCount() const { return m_NodeCount; }
			size_t PQNodeCount() const { return m_PQNodeCount; }
			size_t PVNodeCount() const { return m_PVNodeCount; }
			const ObjectModel::Bus* SlackNode() const { return m_SlackNode; }
			complexd TotalPowerGeneration() const { return m_TotalPowerGeneration; }
			complexd TotalPowerConsumption() const { return m_TotalPowerConsumption; }
			complexd TotalPowerLoss() const { return m_TotalPowerLoss; }
			complexd TotalPowerShunt() const { return m_TotalPowerShunt; }
			SolutionStatus Status() const { return m_Status; }
			int IterationCount() const { return m_IterationCount; }
			double MaxDeviation() const { return m_MaxDeviation; }
		public:
			const NodeFlowCollection& NodeFlow() const { return m_NodeFlow; }
			const NodeFlowSolution& NodeFlow(ObjectModel::Bus* busRef) const {
				return m_NodeFlow.at(busRef);
			}
			const ComponentFlowCollection& ComponentFlow() const { return m_ComponentFlow; }
			const ComponentFlowSolution& ComponentFlow(ObjectModel::Component* c) const {
				return m_ComponentFlow.at(c);
			}
			const BranchFlowCollection& BranchFlow() const { return m_BranchFlow; }
		_PS_INTERNAL:
			Solution();
		};
	}
}

#endif	//__POWERSOLUTIONS_POWERFLOWSOLUTION_H
