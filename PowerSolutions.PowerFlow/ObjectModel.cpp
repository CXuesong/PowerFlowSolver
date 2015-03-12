#include "stdafx.h"
#include "PowerFlowObjectModel.h"
#include "NetworkCase.h"
#include "PrimitiveNetwork.h"
#include "Exceptions.h"
#include <algorithm>

using namespace PowerSolutions;
using namespace std;

namespace PowerSolutions
{
	namespace ObjectModel
	{
		PiEquivalencyParameters::PiEquivalencyParameters(complexd z, complexd y1, complexd y2)
			: m_Impedance(z), m_Admittance1(y1), m_Admittance2(y2)
		{ }

		vector<complexd> PiEquivalencyParameters::EvalPowerInjection(complexd voltage1, complexd voltage2) const
		{
			vector<complexd> power(3);
			complexd power1, power2, shunt1, shunt2;
			//以下宏定义了注入线路的功率，以及从线路流到地的功率。
#define _EvalPower(v1, a1, v2, a2, y10, z12) \
			conj((v1)*(v1)*(y10) + (v1)*((v1) - polar((v2), (a2)-(a1)))/(z12))	//以 v1 作为参考相量
#define _EvalShuntPower(v, y10) (v) * (v) * conj(y10)
			power[1] = -_EvalPower(abs(voltage1), arg(voltage1),
				abs(voltage2), arg(voltage2), m_Admittance1, m_Impedance);
			power[2] = -_EvalPower(abs(voltage2), arg(voltage2),
				abs(voltage1), arg(voltage1), m_Admittance2, m_Impedance);
			power[0] = _EvalShuntPower(abs(voltage1), m_Admittance1) + _EvalShuntPower(abs(voltage2), m_Admittance2);
#undef _EvalPower
#undef _EvalShuntPower
			return power;
		}

		PerUnitBase::PerUnitBase(double voltage, double power)
			: m_Voltage(voltage), m_Power(power)
		{ }

		////////// 网络对象 //////////
		NetworkObject::NetworkObject()
		{ }

		NetworkObject::~NetworkObject()
		{
		}

		void NetworkObject::Validate() const
		{
		}

		NetworkObject* NetworkObject::Clone(const NetworkCaseTrackingInfo& context) const
		{
			//使用重写程度最高的函数构造新实例，同时设置派生类的属性。
			auto newInstance = CloneInstance();
			//检查派生类是否正确完成了复制工作。
			assert(typeid(*newInstance) == typeid(*this));
			//设置基类属性。
			OnCloned(newInstance, context);
			return newInstance;
		}

		void NetworkObject::OnCloned(NetworkObject* newInstance, const NetworkCaseTrackingInfo& context) const
		{
			//约定：Clone不会复制指向父级的指针，
			//且不在对应 caseInfo 中主动调用 Attach 以确定依存关系。
			//但可以复制母线的引用（此处需要用到 caseInfo 参数）
			//newInstance->m_Index = m_Index;
		}

		////////// 组件 //////////

		Component::Component(int portCount)
			: m_Buses(portCount)
		{ }

		void Component::OnCloned(NetworkObject* newInstance, const NetworkCaseTrackingInfo& context) const
		{
			NetworkObject::OnCloned(newInstance, context);
			auto inst = static_cast<Component*>(newInstance);
			//根据索引复制母线。
			for (size_t i = 0; i < m_Buses.size(); i++)
			{
				if (m_Buses[i] != nullptr)
					inst->Buses(i, context.CloneOf(m_Buses[i]));
			}
		}

		SinglePortComponent::SinglePortComponent(Bus* bus1) : Component(1)
		{
			Buses(0, bus1);
		}

		void SinglePortComponent::BuildNodeInfo(PrimitiveNetwork* pNetwork)
		{
			pNetwork->ClaimParent(this->Bus1(), this);
		}

		DoublePortComponent::DoublePortComponent(Bus* bus1, Bus* bus2) : Component(2)
		{
			Buses(0, bus1);
			Buses(1, bus2);
		}

		void DoublePortComponent::BuildNodeInfo(PrimitiveNetwork* pNetwork)
		{
			pNetwork->ClaimBranch(Bus1(), Bus2());
		}

		vector<complexd> DoublePortComponent::EvalPowerInjection(PrimitiveNetwork* pNetwork) const
		{
			return this->PiEquivalency().EvalPowerInjection(
				pNetwork->BusMapping[Bus1()]->VoltagePhasor(),
				pNetwork->BusMapping[Bus2()]->VoltagePhasor());
		}

		void DoublePortComponent::BuildAdmittanceInfo(PrimitiveNetwork* pNetwork)
		{
			pNetwork->AddPi(Bus1(), Bus2(), this->PiEquivalency());
		}

		TriPortComponent::TriPortComponent(Bus* bus1, Bus* bus2, Bus* bus3) : Component(3)
		{
			Buses(0, bus1);
			Buses(1, bus2);
			Buses(2, bus3);
		}

		////////// 母线 //////////
		Bus::Bus()
			: m_InitialVoltage(1, 0)
		{ }

		Bus::Bus(complexd initialVoltage)
			:  m_InitialVoltage(initialVoltage)
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