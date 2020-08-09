#include "stdafx.h"
#include "PowerFlowObjectModel.h"
#include "NetworkCase.h"
#include "PrimitiveNetwork.h"
#include "Exceptions.h"
#include "PowerFlowSolvers.h"
#include "PowerFlowSolution.h"
#include <algorithm>

using namespace PowerSolutions;
using PowerSolutions::PowerFlow::PrimitiveSolution;
using PowerSolutions::PowerFlow::ComponentFlowSolution;
using namespace std;

namespace PowerSolutions
{
	namespace ObjectModel
	{
		PiEquivalencyParameters::PiEquivalencyParameters(complexd z, complexd y1, complexd y2)
			: m_Impedance(z), m_Admittance1(y1), m_Admittance2(y2)
		{ }

		ComponentFlowSolution PiEquivalencyParameters::EvalComponentFlow(complexd voltage1, complexd voltage2) const
		{
			ComponentFlowSolution s(2);
			//���º궨����ע����·�Ĺ��ʣ��Լ�����·�����صĹ��ʡ�
#define _EvalPower(v1, a1, v2, a2, y10, z12) \
			conj((v1)*(v1)*(y10) + (v1)*((v1) - polar((v2), (a2)-(a1)))/(z12))	//�� v1 ��Ϊ�ο�����
#define _EvalShuntPower(v, y10) (v) * (v) * conj(y10)
			s.PowerInjections(0, _EvalPower(abs(voltage1), arg(voltage1),
				abs(voltage2), arg(voltage2), m_Admittance1, m_Impedance));
			s.PowerInjections(1, _EvalPower(abs(voltage2), arg(voltage2),
				abs(voltage1), arg(voltage1), m_Admittance2, m_Impedance));
			s.PowerShunt(_EvalShuntPower(abs(voltage1), m_Admittance1) + _EvalShuntPower(abs(voltage2), m_Admittance2));
#undef _EvalPower
#undef _EvalShuntPower
			return s;
		}

		PerUnitBase::PerUnitBase(double voltage, double power)
			: m_Voltage(voltage), m_Power(power)
		{ }

		////////// ������� //////////
#if _DEBUG
		unsigned long NetworkObject::_IDCounter = 0;

		NetworkObject::NetworkObject()
			: _ID(_IDCounter)
		{
			_IDCounter++;
			//_PS_TRACE("NO Construct #" << _ID << " @ " << this);
		}
#else
		NetworkObject::NetworkObject()
		{ }
#endif

		NetworkObject::~NetworkObject()
		{
			//_PS_TRACE("NO Dispose #" << _ID << " @ " << this);
		}

		void NetworkObject::Validate() const
		{
		}

		NetworkObject* NetworkObject::Clone(const NetworkCaseCorrespondenceInfo& context) const
		{
			//ʹ����д�̶���ߵĺ���������ʵ����ͬʱ��������������ԡ�
			auto newInstance = CloneInstance();
			//����������Ƿ���ȷ����˸��ƹ�����
			assert(typeid(*newInstance) == typeid(*this));
			//���û������ԡ�
			OnCloned(newInstance, context);
			return newInstance;
		}

		void NetworkObject::OnCloned(NetworkObject* newInstance, const NetworkCaseCorrespondenceInfo& context) const
		{
			//Լ����Clone���Ḵ��ָ�򸸼���ָ�룬
			//�Ҳ��ڶ�Ӧ caseInfo ���������� Attach ��ȷ�������ϵ��
			//�����Ը���ĸ�ߵ����ã��˴���Ҫ�õ� context ������
			newInstance->_Tag = this->_Tag;
		}

		////////// ��� //////////

		Component::Component(int portCount)
			: m_Buses(portCount)
		{ }

		void Component::BuildNodeInfo(PrimitiveNetwork* pNetwork) const
		{
			for (auto& b : m_Buses)
			{
				assert(b != nullptr);
				pNetwork->ClaimParent(b, this);
			}
		}

		void Component::OnCloned(NetworkObject* newInstance, const NetworkCaseCorrespondenceInfo& context) const
		{
			NetworkObject::OnCloned(newInstance, context);
			auto inst = static_cast<Component*>(newInstance);
			//������������ĸ�ߡ�
			for (size_t i = 0; i < m_Buses.size(); i++)
			{
				if (m_Buses[i] != nullptr)
					inst->Buses(i, context.CloneOfStatic(m_Buses[i]));
			}
		}

		SinglePortComponent::SinglePortComponent(Bus* bus1) : Component(1)
		{
			Buses(0, bus1);
		}

		DoublePortComponent::DoublePortComponent(Bus* bus1, Bus* bus2) : Component(2)
		{
			Buses(0, bus1);
			Buses(1, bus2);
		}

		void DoublePortComponent::BuildNodeInfo(PrimitiveNetwork* pNetwork) const
		{
			Component::BuildNodeInfo(pNetwork);
			pNetwork->ClaimBranch(Bus1(), Bus2(), this);
		}

		PowerFlow::ComponentFlowSolution DoublePortComponent::EvalComponentFlow(const PowerFlow::PrimitiveSolution& solution) const
		{
			auto s = this->PiEquivalency().EvalComponentFlow(
				solution.NodeStatus(Bus1()).VoltagePhasor(),
				solution.NodeStatus(Bus2()).VoltagePhasor());
			return s;
		}

		void DoublePortComponent::BuildAdmittanceInfo(PrimitiveNetwork* pNetwork) const
		{
			pNetwork->AddPi(Bus1(), Bus2(), this->PiEquivalency());
		}

		TriPortComponent::TriPortComponent(Bus* bus1, Bus* bus2, Bus* bus3) : Component(3)
		{
			Buses(0, bus1);
			Buses(1, bus2);
			Buses(2, bus3);
		}

		////////// ĸ�� //////////
		Bus::Bus()
			: Bus(nullptr, 1.0)
		{ }

		Bus::Bus(complexd initialVoltage)
			: Bus(nullptr, initialVoltage)
		{ }

		Bus::Bus(IBusContainer* parent, complexd initialVoltage)
			: m_Parent(parent), m_InitialVoltage(initialVoltage)
		{ }

		void Bus::Validate() const
		{
			/*if (Index < 0)
			throw InvalidArgumentException(_T("Index"));
			if (CaseInfo != nullptr && CaseInfo->Buses().find(Index) != CaseInfo->Buses().end())
			throw ValidationException(Format(ERROR_BUS_INDEX_DUPLICATED, Index));*/
		}

		NetworkObject* Bus::CloneInstance() const
		{
			return new Bus(m_InitialVoltage);
		}
	}
}