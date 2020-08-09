#pragma once

#include "PowerSolutions.Interop.h"
#include "ObjectModel.h"
#include "CollectionWrappers.h"

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
				property Complex Voltage;
				/// <summary>�ڵ���ܳ�����</summary>
				property Complex PowerGeneration;
				// <summary>�ڵ���ܸ��ء�</summary>
				property Complex PowerConsumption;
			internal:
				NodeFlowSolution(const _NATIVE_PF NodeFlowSolution& native);
			};

			public value struct BranchFlowSolution
			{
			public:
				/// <summary>�ӽڵ�1ע��Ĺ��ʡ�</summary>
				property Complex Power1;
				/// <summary>�ӽڵ�2ע��Ĺ��ʡ�</summary>
				property Complex Power2;
				/// <summary>�ӽڵ�1ע��ӵ�֧·�Ĺ��ʡ�</summary>
				property Complex ShuntPower1;
				/// <summary>�ӽڵ�2ע��ӵ�֧·�Ĺ��ʡ�</summary>
				property Complex ShuntPower2;
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

			public ref class NodeFlowDictionary 
				: ReadOnlyDictionaryWrapper < _NATIVE_PF Solution::NodeFlowCollection, Bus, NodeFlowSolution >
			{
			internal:
				NodeFlowDictionary(const _NATIVE_PF Solution::NodeFlowCollection *native)
					: ReadOnlyDictionaryWrapper(native)
				{ }
			};

			public ref class ComponentFlowDictionary
				: ReadOnlyDictionaryWrapper < _NATIVE_PF Solution::ComponentFlowCollection, Component, BranchFlowSolution >
			{
			internal:
				ComponentFlowDictionary(const _NATIVE_PF Solution::ComponentFlowCollection *native)
					: ReadOnlyDictionaryWrapper(native)
				{ }
			};

			public ref class BranchFlowDictionary
				: ReadOnlyDictionaryWrapper < _NATIVE_PF Solution::BranchFlowCollection, BusPair, BranchFlowSolution >
			{
			internal:
				BranchFlowDictionary(const _NATIVE_PF Solution::BranchFlowCollection *native)
					: ReadOnlyDictionaryWrapper(native)
				{ }
			};

			public ref class Solution
			{
			internal:
				_NATIVE_PF Solution* nativeObject;
			private:
				NodeFlowDictionary^ m_NodeFlow;
				ComponentFlowDictionary^ m_ComponentFlow;
				BranchFlowDictionary^ m_BranchFlow;
			public:
				_WRAP_PROPERTY_READONLY(TotalPowerGeneration, Complex, MarshalComplex);
				_WRAP_PROPERTY_READONLY(TotalPowerConsumption, Complex, MarshalComplex);
				_WRAP_PROPERTY_READONLY(TotalPowerLoss, Complex, MarshalComplex);
				_WRAP_PROPERTY_READONLY(TotalPowerShunt, Complex, MarshalComplex);
				_WRAP_PROPERTY_READONLY(IterationCount, int, );
				_WRAP_PROPERTY_READONLY(MaxDeviation, double, );
				_WRAP_PROPERTY_READONLY(Status, SolutionStatus, (SolutionStatus));
				property NodeFlowDictionary^ NodeFlow
				{
					NodeFlowDictionary^ get(){ return m_NodeFlow; }
				}
				property ComponentFlowDictionary^ ComponentFlow
				{
					ComponentFlowDictionary^ get(){ return m_ComponentFlow; }
				}
				property BranchFlowDictionary^ BranchFlow
				{
					BranchFlowDictionary^ get(){ return m_BranchFlow; }
				}
			public:
				Solution(_NATIVE_PF Solution* native);
				!Solution();
				~Solution();
			};

			/// <summary>
			/// ���������̬�����������̡�
			/// ��<see cref="PowerSolutions::PowerFlow::Solver" />��
			/// </summary>
			public ref class Solver
			{
			private:
				_NATIVE_PF Solver* nativeObject;
			public:
				_WRAP_PROPERTY(MaxIterations, int, );
				_WRAP_PROPERTY(MaxDeviationTolerance, double, );
				_WRAP_PROPERTY(IntelliIterations, bool, );
			public:
				Solution^ Solve(ObjectModel::NetworkCase^ network);
			public:
				Solver(SolverType type);
				!Solver();
				~Solver();
			};
		}
	}
}
