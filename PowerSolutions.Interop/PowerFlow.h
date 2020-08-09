#pragma once

#include "PowerSolutions.Interop.h"
#include "ObjectModel.h"

namespace PowerSolutions
{
	namespace Interop
	{
		namespace PowerFlow
		{
			public enum class SolverType : byte
			{
				NewtonRaphson = 0,
				FastDecoupled = 1
			};

			/// <summary>������յĽ��ۡ�</summary>
			public enum class SolutionStatus : byte
			{
				Success = 0x00,					//���ɹ�������
				MaxIteration = 0x10,			//�Ѿ��ﵽ�������ĵ���������
				IterationFailed = 0x11,			//�������̳������⡣
				IntelliIterationAbort = 0x12,	//�������ܵ���������������̲����������жϡ�
			};

			public value struct NodeFlowSolution
			{
			public:
				/// <summary>�ڵ�ĵ�ѹ������</summary>
				_WRAP_PROPERTY_CACHE(Voltage, Complex);
				/// <summary>�ڵ���ܳ�����</summary>
				_WRAP_PROPERTY_CACHE(PowerGeneration, Complex);
				// <summary>�ڵ���ܸ��ء�</summary>
				_WRAP_PROPERTY_CACHE(PowerConsumption, Complex);
				// <summary>�ڵ�Ķȡ�</summary>
				_WRAP_PROPERTY_CACHE(Degree, int);
			internal:
				NodeFlowSolution(const _NATIVE_PF NodeFlowSolution& native);
			};

			public value struct BranchFlowSolution
			{
			public:
				/// <summary>�ӽڵ�1ע��Ĺ��ʡ�</summary>
				_WRAP_PROPERTY_CACHE(Power1, Complex);
				/// <summary>�ӽڵ�2ע��Ĺ��ʡ�</summary>
				_WRAP_PROPERTY_CACHE(Power2, Complex);
				/// <summary>�ӽڵ�1ע��ӵ�֧·�Ĺ��ʡ�</summary>
				_WRAP_PROPERTY_CACHE(ShuntPower1, Complex);
				/// <summary>�ӽڵ�2ע��ӵ�֧·�Ĺ��ʡ�</summary>
				_WRAP_PROPERTY_CACHE(ShuntPower2, Complex);
			public:
				//ָʾ���ʵ�ʵ�ʴ��䷽���Ƿ���Լ���ķ���Bus1->Bus2���෴��
				bool ReversedDirection()
				{
					return Power1.Real < 0;
				}
				//��ȡʵ�ʴ���Ĺ��ʴ�С��
				Complex PowerTransfer()
				{
					return Power1.Real > 0 ? -Power2 : -Power1;
				}
				//�����ڵ�֮�䴫�����ʱ��ʧ���ܹ��ʣ������ӵ�֧·����
				Complex PowerLoss()
				{
					return Power1 + Power2;
				}
				//�����ڵ�Ľӵ�֧·����ʧ���ܹ��ʡ�
				Complex PowerShunt()
				{
					return ShuntPower1 + ShuntPower2;
				}
			internal:
				BranchFlowSolution(const _NATIVE_PF BranchFlowSolution& native);
			};

			public value struct IterationInfo
			{
			public:
				/// <summary>�˴ε���������������������ֵ��</summary>
				_WRAP_PROPERTY_CACHE(MaxDeviation, double);
				// ��ȡ�˵�����Ϣ���ַ���������ʽ��
				String^ ToString() override { return String::Format(L"��P={0}", MaxDeviation); }
			public:
				IterationInfo(const _NATIVE_PF IterationInfo& native);
			};

			public value struct SolverStatus
			{
			public:
				/// <summary>
				/// ��ȡһ��ֵ��ָʾ�˵�ǰ�Ƿ����ڽ��е�����
				/// ע�⣺�����ʱ���ڽ��е���ǰ��������������Ҳ�᷵�� false��
				/// </summary>
				_WRAP_PROPERTY_CACHE(IsIterating, bool);
				/// <summary>��ȡ�Ѿ���ɵĵ���������</summary>
				_WRAP_PROPERTY_CACHE(LastIterationCount, int);
				/// <summary>��ȡ��һ�ε������������</summary>
				_WRAP_PROPERTY_CACHE(LastIterationInfo, IterationInfo);
				// ��ȡ����Ϣ���ַ���������ʽ��
				String^ ToString() override;
			public:
				SolverStatus(const _NATIVE_PF SolverStatus& native);
			};

			/// <summary>
			/// ��̬�������������
			/// ��<see cref="PowerSolutions::PowerFlow::Solution" />�Ļ���������
			/// </summary>
			public ref class Solution
			{
			private:
				Dictionary<Bus, NodeFlowSolution>^ m_NodeFlow;
				ReadOnlyDictionary<Bus, NodeFlowSolution>^ m_s_NodeFlow;
				Dictionary<Component, BranchFlowSolution>^ m_ComponentFlow;
				ReadOnlyDictionary<Component, BranchFlowSolution>^ m_s_ComponentFlow;
				Dictionary<BusPair, BranchFlowSolution>^ m_BranchFlow;
				ReadOnlyDictionary<BusPair, BranchFlowSolution>^ m_s_BranchFlow;
				List<IterationInfo>^ m_IterationInfo;
				IList<IterationInfo>^ m_s_IterationInfo;
			public:
				_WRAP_PROPERTY_CACHE(TotalPowerGeneration, Complex);
				_WRAP_PROPERTY_CACHE(TotalPowerConsumption, Complex);
				_WRAP_PROPERTY_CACHE(TotalPowerLoss, Complex);
				_WRAP_PROPERTY_CACHE(TotalPowerShunt, Complex);
				_WRAP_PROPERTY_CACHE(IterationCount, int);
				_WRAP_PROPERTY_CACHE(MaxDeviation, double);
				_WRAP_PROPERTY_CACHE(Status, SolutionStatus);
				_WRAP_PROPERTY_CACHE(NodeCount, int);
				_WRAP_PROPERTY_CACHE(PQNodeCount, int);
				_WRAP_PROPERTY_CACHE(PVNodeCount, int);
				_WRAP_PROPERTY_CACHE(SlackNode, Bus);

				property IDictionary<Bus, NodeFlowSolution>^ NodeFlow
				{
					IDictionary<Bus, NodeFlowSolution>^ get(){ return m_s_NodeFlow; }
				}
				property IDictionary<Component, BranchFlowSolution>^ ComponentFlow
				{
					IDictionary<Component, BranchFlowSolution>^ get(){ return m_s_ComponentFlow; }
				}
				property IDictionary<BusPair, BranchFlowSolution>^ BranchFlow
				{
					IDictionary<BusPair, BranchFlowSolution>^ get(){ return m_s_BranchFlow; }
				}
				property IList<PowerFlow::IterationInfo>^ IterationInfo
				{
					IList<PowerFlow::IterationInfo>^ get() { return m_s_IterationInfo; }
				}
			public:
				String^ ToString() override;
			public:
				Solution(const _NATIVE_PF Solution& native);
			};

			/// <summary>
			/// ���������̬�����������̡�
			/// ��<see cref="PowerSolutions::PowerFlow::Solver" />��
			/// </summary>
			public ref class Solver
			{
			private:
				_NATIVE_PF Solver* nativeObject;
				SolverType m_Type;
			public:
				_WRAP_PROPERTY(NodeReorder, bool, );
				_WRAP_PROPERTY(MaxIterations, int, );
				_WRAP_PROPERTY(MaxDeviationTolerance, double, );
				_WRAP_PROPERTY(IntelliIterations, bool, );
				property SolverType Type
				{
					SolverType get() { return m_Type; }
				}
				property String^ FriendlyName
				{
					String^ get();
				}
			public:
				// �������Ĺ��ʳ����ֲ���������һ�������������档
				Solution^ Solve(ObjectModel::NetworkCase^ network);
				/// <summary>
				/// ��ȡ��ǰ�����״̬��
				/// ���������У�����ʹ����һ���̵߳��ô˺����Բ�ѯ״̬��
				/// </summary>
				SolverStatus GetStatus();
			public:
				Solver(SolverType type);
				!Solver();
				~Solver();
			};
		}
	}
}
