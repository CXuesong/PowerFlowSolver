
#ifndef __POWERSOLUTIONS_POWERFLOWSOLVERS_H
#define __POWERSOLUTIONS_POWERFLOWSOLVERS_H

#include "NetworkCase.h"
#include "PrimitiveNetwork.h"
#include "PowerFlowSolution.h"
#include <vector>

namespace PowerSolutions
{
	namespace PowerFlow
	{
		enum class SolverType
		{
			NewtonRaphson = 0,
			FastDecoupled = 1,
			DcPowerFlow = 10,
		};

		// 包含了求解器此时的状态信息。
		class IterationEventArgs
		{
		private:
			int m_IterationCount;
			double m_MaxDeviation;
		public:
			// 获取已经完成的迭代次数。0表示迭代尚未开始。
			int IterationCount() const { return m_IterationCount; }
			// 此次迭代结束时最大功率误差的绝对值。
			double MaxDeviation() const { return m_MaxDeviation; }
		public:
			// 初始化一个表示当前正在进行求解的 SolverStatus。
			IterationEventArgs(int iterationCount, double maxDeviation)
				: m_IterationCount(iterationCount), m_MaxDeviation(maxDeviation)
			{ }
		};

		// 用于在迭代过程中接收每一步的迭代信息。
		typedef void(__stdcall *IterationEventHandler)(class Solver* sender, IterationEventArgs* e);

		// 用于在运算中保存节点的运算状态。
		struct NodeEvaluationStatus
		{
		private:
			double m_Voltage;
			double m_Angle;
			double m_ActivePowerInjection;
			double m_ReactivePowerInjection;
			ObjectModel::NodeType m_Type;
			int m_Index;
			int m_SubIndex;
		public:
			double Voltage() const { return m_Voltage; }
			double Angle() const { return m_Angle; }
			complexd VoltagePhasor() const { return std::polar(m_Voltage, m_Angle); }
			double ActivePowerInjection() const { return m_ActivePowerInjection; }
			double ReactivePowerInjection() const { return m_ReactivePowerInjection; }
			complexd PowerInjection() const { return complexd(m_ActivePowerInjection, m_ReactivePowerInjection); }
			ObjectModel::NodeType Type() const { return m_Type; }
			void Type(ObjectModel::NodeType val) { m_Type = val; }
			int Index() const { return m_Index; }
			void Index(int val) { m_Index = val; }
			int SubIndex() const { return m_SubIndex; }
			void SubIndex(int val) { m_SubIndex = val; }
			void SetVoltage(double voltage, double angle)
			{
				m_Voltage = voltage;
				m_Angle = angle;
			}
			void ClearPowerInjection()
			{
				m_ActivePowerInjection = m_ReactivePowerInjection = 0;
			}
			void AddPowerInjections(double active, double reactive)
			{
				m_ActivePowerInjection += active;
				m_ReactivePowerInjection += reactive;
			}
		public:
			NodeEvaluationStatus(ObjectModel::PrimitiveNetwork::NodeInfo& info)
				: m_Voltage(info.Voltage()), m_Angle(info.Angle()),
				m_ActivePowerInjection(info.ActivePowerInjection()), m_ReactivePowerInjection(info.ReactivePowerInjection()),
				m_Type(info.Type()), m_Index(info.Index()), m_SubIndex(info.SubIndex())
			{ }
		};

		// 用于表示原始的潮流求解结果。
		class PrimitiveSolution
		{
		public:
			typedef std::vector<NodeEvaluationStatus> NodeStatusCollection;
		private:
			NodeStatusCollection m_NodeStatus;
			ObjectModel::PrimitiveNetwork* m_Network;
		_PS_INTERNAL:
			NodeStatusCollection& NodeStatus() { return m_NodeStatus; }
			NodeEvaluationStatus& NodeStatus(int nodeIndex) { return m_NodeStatus.at(nodeIndex); }
		public:
			const NodeStatusCollection& NodeStatus() const { return m_NodeStatus; }
			const NodeEvaluationStatus& NodeStatus(int nodeIndex) const { return m_NodeStatus.at(nodeIndex); }
			const NodeEvaluationStatus& NodeStatus(const ObjectModel::Bus* bus) const { return m_NodeStatus.at(m_Network->Nodes(bus).Index()); }
			ObjectModel::PrimitiveNetwork* Network() const { return m_Network; }
		public:
			PrimitiveSolution(ObjectModel::PrimitiveNetwork& network);
		};

		// 抽象用于完成稳态潮流的解决过程。
		class Solver
		{
		private:
			int m_MaxIterations;				//最大的迭代次数。
			double m_MaxDeviationTolerance;		//最大允许误差。
			bool m_IntelliIterations;			//允许根据收敛速度来判断是否有必要继续迭代。
			IterationEventHandler m_IterationEvent;	//允许在每一步迭代结束时接收迭代的信息。
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
			// 求解网络的功率潮流分布，并生成一个潮流分析报告。
			virtual std::shared_ptr<Solution> Solve(ObjectModel::PrimitiveNetwork& network) = 0;
			std::shared_ptr<Solution> Solve(std::shared_ptr<ObjectModel::PrimitiveNetwork> network);
			virtual std::shared_ptr<Solution> Solve(ObjectModel::NetworkCase& network);
			Solver();
			virtual ~Solver();
		public:
			static Solver* Create(SolverType type);
		};
	}
}

#endif	//__POWERSOLUTIONS_POWERFLOWSOLVERS_H
