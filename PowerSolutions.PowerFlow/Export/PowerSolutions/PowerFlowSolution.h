
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
		struct NodeFlowSolution		//节点潮流结果
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
		struct BranchFlowSolution		//（每元件或节点编号对）支路潮流结果
		{
			//建议：支路编号	I侧节点名称	J侧节点名称		I侧注入有功	I侧注入无功	J侧注入有功	J侧注入无功	支路有功损耗	支路无功损耗
			// Power1          Power2
			// --->--------------<----
			//       |       |
			// S.P.1 |       | S.P.2
			//       |       |
		private:
			complexd m_Power1;		//从节点1注入的功率
			complexd m_Power2;		//从节点2注入的功率
			complexd m_ShuntPower1;	//从节点1注入接地支路的功率
			complexd m_ShuntPower2;	//从节点2注入接地支路的功率
		public:
			PowerSolutions::complexd Power1() const { return m_Power1; }
			void Power1(PowerSolutions::complexd val) { m_Power1 = val; }
			PowerSolutions::complexd Power2() const { return m_Power2; }
			void Power2(PowerSolutions::complexd val) { m_Power2 = val; }
			PowerSolutions::complexd ShuntPower1() const { return m_ShuntPower1; }
			void ShuntPower1(PowerSolutions::complexd val) { m_ShuntPower1 = val; }
			PowerSolutions::complexd ShuntPower2() const { return m_ShuntPower2; }
			void ShuntPower2(PowerSolutions::complexd val) { m_ShuntPower2 = val; }
			bool ReversedDirection() const	//指示功率的实际传输方向是否与约定的方向（Bus1->Bus2）相反。
			{
				return m_Power1.real() < 0;
			}
			complexd PowerTransfer() const	//获取实际传输的功率大小。
			{
				//注意到此处为注入π形网络的功率
				//"传输功率"为流出π形网络的功率
				//assert((power1.real() >= 0) ^ (power2.real() > 0));
				//在某些不收敛的情况下，可能会出现上述断言失败的情况。
				return m_Power1.real() > 0 ? -m_Power2 : -m_Power1;
			}
			complexd PowerLoss() const		//在两节点之间传输电能时损失的总功率（包括接地支路）。
			{
				return m_Power1 + m_Power2;
			}
			complexd PowerShunt() const		//在两节点的接地支路中损失的总功率。
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
		// 求解最终的结论
		enum class SolutionStatus : byte
		{
			Success = 0x00,					//求解成功结束。
			MaxIteration = 0x10,			//已经达到最大允许的迭代次数。
			IterationFailed = 0x11,			//迭代过程出现问题。
			IntelliIterationAbort = 0x12,	//启用智能迭代后，由于运算过程不收敛而被中断。
		};

		//包含了稳态潮流的分析报告。
		class Solution
		{
		public:
			typedef std::unordered_map<ObjectModel::Bus*, NodeFlowSolution> NodeFlowCollection;
			typedef std::unordered_map<std::pair<ObjectModel::Bus*, ObjectModel::Bus*>, BranchFlowSolution,
				Utility::UnorderedPairHasher<ObjectModel::Bus*>,
				Utility::UnorderedPairEqualityComparer<ObjectModel::Bus*>> BranchFlowCollection;
			typedef std::unordered_map<ObjectModel::Component*, BranchFlowSolution> ComponentFlowCollection;
		private:
			NodeFlowCollection m_NodeFlow;				//节点潮流信息。
			ComponentFlowCollection m_ComponentFlow;	//（每元件）支路潮流信息。
			BranchFlowCollection m_BranchFlow;			//（节点编号对）支路潮流信息。
			size_t m_NodeCount;
			size_t m_PQNodeCount;
			size_t m_PVNodeCount;
			ObjectModel::Bus* m_SlackNode;
			complexd m_TotalPowerGeneration;
			complexd m_TotalPowerConsumption;
			complexd m_TotalPowerLoss;
			complexd m_TotalPowerShunt;
			SolutionStatus m_Status;
			int m_IterationCount;
			double m_MaxDeviation;
		private:	//Internal
			friend class SolverImpl;
			void NodeCount(size_t val) { m_NodeCount = val; }
			void PQNodeCount(size_t val) { m_PQNodeCount = val; }
			void PVNodeCount(size_t val) { m_PVNodeCount = val; }
			void SlackNode(ObjectModel::Bus* val) { m_SlackNode = val; }
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
			size_t NodeCount() const { return m_NodeCount; }
			size_t PQNodeCount() const { return m_PQNodeCount; }
			size_t PVNodeCount() const { return m_PVNodeCount; }
			ObjectModel::Bus* SlackNode() const { return m_SlackNode; }
			complexd TotalPowerGeneration() const { return m_TotalPowerGeneration; }
			complexd TotalPowerConsumption() const { return m_TotalPowerConsumption; }
			complexd TotalPowerLoss() const { return m_TotalPowerLoss; }
			complexd TotalPowerShunt() const { return m_TotalPowerShunt; }
			SolutionStatus Status() const { return m_Status; }
			int IterationCount() const { return m_IterationCount; }
			double MaxDeviation() const { return m_MaxDeviation; }
		public:
			const NodeFlowCollection& NodeFlow() const { return m_NodeFlow; }
			const ComponentFlowCollection& ComponentFlow() const { return m_ComponentFlow; }
			const BranchFlowCollection& BranchFlow() const { return m_BranchFlow; }
		protected:	//internal
			Solution();
		};
	}
}

#endif	//__POWERSOLUTIONS_POWERFLOWSOLUTION_H
