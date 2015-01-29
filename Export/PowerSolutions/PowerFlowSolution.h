
#ifndef __POWERSOLUTIONS_POWERFLOWSOLUTION_H
#define __POWERSOLUTIONS_POWERFLOWSOLUTION_H

#include "PowerSolutions.h"
#include "Utility.h"
#include <unordered_map>

namespace PowerSolutions
{
	namespace PowerFlow
	{
		struct NodeFlowSolution		//节点潮流结果
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
			bool ReversedDirection()	//指示功率的实际传输方向是否与约定的方向（Bus1->Bus2）相反。
			{
				return m_Power1.real() < 0;
			}
			complexd PowerTransfer()	//获取实际传输的功率大小。
			{
				//注意到此处为注入π形网络的功率
				//"传输功率"为流出π形网络的功率
				//assert((power1.real() >= 0) ^ (power2.real() > 0));
				//在某些不收敛的情况下，可能会出现上述断言失败的情况。
				return m_Power1.real() > 0 ? -m_Power2 : -m_Power1;
			}
			complexd PowerLoss()		//在两节点之间传输电能时损失的总功率。
			{
				return m_Power1 + m_Power2;
			}
			complexd PowerShunt()		//在两节点的接地支路中损失的总功率。
			{
				return m_ShuntPower1 + m_ShuntPower2;
			}
			BranchFlowSolution(complexd power1, complexd power2, complexd shuntPower1, complexd shuntPower2)
				: m_Power1(power1), m_Power2(power2), m_ShuntPower1(shuntPower1), m_ShuntPower2(shuntPower2)
			{ }
		};
		// 求解最终的结论
		enum class SolutionStatus
		{
			Success = 0x00,					//求解成功结束。
			MaxIteration = 0x10,			//已经达到最大允许的迭代次数。
			IterationFailed = 0x11,			//迭代过程出现问题。
			IntelliIterationAbort = 0x12,	//启用智能迭代后，由于运算过程不收敛而被中断。
		};

		//包含了稳态潮流的分析报告。
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
			NodeFlowCollection m_NodeFlow;				//节点潮流信息。
			ComponentFlowCollection m_ComponentFlow;	//（每元件）支路潮流信息。
			BranchFlowCollection m_BranchFlow;			//（节点编号对）支路潮流信息。
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