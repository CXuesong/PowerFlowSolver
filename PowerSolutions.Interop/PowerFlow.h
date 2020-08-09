#pragma once
//TODO ��װ ComponentFlow
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
				/// <summary>��ע��ӵ�֧·�Ĺ��ʡ�</summary>
				_WRAP_PROPERTY_CACHE(PowerShunt, Complex);
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
				BranchFlowSolution Reverse()
				{
					return BranchFlowSolution(Power2, Power1, PowerShunt);
				}
			internal:
				BranchFlowSolution(Complex power1, Complex power2, Complex powerShunt);
				BranchFlowSolution(const _NATIVE_PF BranchFlowSolution& native);
			};

			public value struct ComponentFlowSolution
			{
			public:
				/// <summary>�ӽڵ�1ע��Ĺ��ʡ�</summary>
				_WRAP_PROPERTY_CACHE(PowerInjections, cli::array<Complex>^);
				/// <summary>�ӽڵ�2ע��Ĺ��ʡ�</summary>
				_WRAP_PROPERTY_CACHE(PowerShunt, Complex);
				/// <summary>ָʾ��Ԫ������ĳ����Ƿ�Ϊ�����ġ�</summary>
				_WRAP_PROPERTY_CACHE(IsUnconstrained, bool);
			internal:
				ComponentFlowSolution(const _NATIVE_PF ComponentFlowSolution& native);
			};

			public ref class IterationEventArgs : public System::EventArgs
			{
			public:
				/// <summary>�Ѿ���ɵĵ���������</summary>
				_WRAP_PROPERTY_CACHE(IterationCount, double);
				/// <summary>�˴ε���������������������ֵ��</summary>
				_WRAP_PROPERTY_CACHE(MaxDeviation, double);
				// ��ȡ�˵�����Ϣ���ַ���������ʽ��
				String^ ToString() override { return String::Format(L"��P={0}", MaxDeviation); }
			public:
				IterationEventArgs(const _NATIVE_PF IterationEventArgs& native);
			};

			public delegate void IterationEventHandler(System::Object^ sender, IterationEventArgs^ e);

			/// <summary>
			/// ��̬�������������
			/// ��<see cref="PowerSolutions::PowerFlow::Solution" />�Ļ���������
			/// </summary>
			public ref class Solution
			{
			private:
				Dictionary<Bus, NodeFlowSolution>^ m_NodeFlow;
				ReadOnlyDictionary<Bus, NodeFlowSolution>^ m_s_NodeFlow;
				Dictionary<Component, ComponentFlowSolution>^ m_ComponentFlow;
				ReadOnlyDictionary<Component, ComponentFlowSolution>^ m_s_ComponentFlow;
				Dictionary<BusPair, BranchFlowSolution>^ m_BranchFlow;
				ReadOnlyDictionary<BusPair, BranchFlowSolution>^ m_s_BranchFlow;
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
				property IDictionary<Component, ComponentFlowSolution>^ ComponentFlow
				{
					IDictionary<Component, ComponentFlowSolution>^ get(){ return m_s_ComponentFlow; }
				}
				property IDictionary<BusPair, BranchFlowSolution>^ BranchFlow
				{
					IDictionary<BusPair, BranchFlowSolution>^ get(){ return m_s_BranchFlow; }
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
				delegate void NativeIterationEventHandler(_NATIVE_PF Solver* sender, _NATIVE_PF IterationEventArgs* e);
			private:
				_NATIVE_PF Solver* nativeObject;
				SolverType m_Type;
				IterationEventHandler^ m_IterationEvent;
				void NativeIterationEventProc(_NATIVE_PF Solver* sender, _NATIVE_PF IterationEventArgs* e);
				NativeIterationEventHandler^ NativeIterationEventDelegate;
				IntPtr NativeIterationEventProcAddress;
			public:
				/// <summary>
				/// �ڵ�һ�ε�����ʼǰ�Լ�ÿһ�ε���������ɺ��������¼���
				/// </summary>
				event IterationEventHandler^ Iteration
				{
					void add(IterationEventHandler^ name);
					void remove(IterationEventHandler^ name);
				protected:
					void raise(System::Object^ sender, IterationEventArgs^ e)
					{
						if (m_IterationEvent)
							m_IterationEvent->Invoke(sender, e);
					}
				}
			public:
				//_WRAP_PROPERTY(NodeReorder, bool, );
				/// <summary>
				/// ָʾ�ڷ�������ʱ���Ƿ�Ӧ�����ýڵ��������ܡ�
				/// </summary>
				property bool NodeReorder;
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
			public:
				Solver(SolverType type);
				!Solver();
				~Solver();
			};
		}
	}
}
