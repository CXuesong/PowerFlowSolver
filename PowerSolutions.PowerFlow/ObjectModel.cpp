#include "stdafx.h"
#include "PowerFlowObjectModel.h"
#include "NetworkCase.h"
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

		PerUnitBase::PerUnitBase(double voltage, double power)
			: m_Voltage(voltage), m_Power(power)
		{ }

		////////// 网络对象 //////////
		NetworkObject::NetworkObject()
			: NetworkObject(NullIndex)
		{ }

		NetworkObject::NetworkObject(int index)
			: m_Index(index), m_BaseValue(1, 1)
		{ }

		NetworkObject::~NetworkObject()
		{
		}

		void NetworkObject::Validate() const
		{
		}

		NetworkObject* NetworkObject::Clone(const NetworkCaseCloneContext& context) const
		{
			//使用重写程度最高的函数构造新实例，同时设置派生类的属性。
			auto newInstance = CloneInstance();
			//检查派生类是否正确完成了复制工作。
			assert(typeid(*newInstance) == typeid(*this));
			//设置基类属性。
			OnCloned(newInstance, context);
			return newInstance;
		}

		void NetworkObject::OnCloned(NetworkObject* newInstance, const NetworkCaseCloneContext& context) const
		{
			//约定：Clone不会复制指向父级的指针，
			//且不在对应 caseInfo 中主动调用 Attach 以确定依存关系。
			//但可以复制母线的引用（此处需要用到 caseInfo 参数）
			newInstance->m_Index = m_Index;
		}

		////////// 组件 //////////

		Component::Component(int portCount)
			: NetworkObject(NullIndex), m_Buses(portCount)
		{ }

		void Component::OnCloned(NetworkObject* newInstance, const NetworkCaseCloneContext& context) const
		{
			NetworkObject::OnCloned(newInstance, context);
			auto inst = static_cast<Component*>(newInstance);
			//根据索引复制母线。
			for (size_t i = 0; i < m_Buses.size(); i++)
			{
				if (m_Buses[i] != nullptr)
					inst->Buses(i, static_cast<Bus*>(context.GetNewObject(m_Buses[i])));
			}
		}

		//complexd Component::EvalPowerInjection(int busIndex, std::vector<complexd>& busVoltage)
		//{
		//	throw Exception(ExceptionCode::NotSupported);
		//}

		//complexd Component::EvalPowerShunt(int busIndex, std::vector<complexd>& busVoltage)
		//{
		//	throw Exception(ExceptionCode::NotSupported);
		//}

		SinglePortComponent::SinglePortComponent(Bus* bus1) : Component(1)
		{
			Buses(0, bus1);
		}

		DoublePortComponent::DoublePortComponent(Bus* bus1, Bus* bus2) : Component(2)
		{
			Buses(0, bus1);
			Buses(1, bus2);
		}

		TriPortComponent::TriPortComponent(Bus* bus1, Bus* bus2, Bus* bus3) : ComplexComponent(3)
		{
			Buses(0, bus1);
			Buses(1, bus2);
			Buses(2, bus3);
		}

		////////// 母线 //////////
		Bus::Bus()
			: m_InitialVoltage(1, 0)
		{ }

		Bus::Bus(int index, complexd initialVoltage)
			: NetworkObject(index), m_InitialVoltage(initialVoltage)
		{ }

		Bus::Bus(int index)
			: Bus(index, 1)
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
			return new Bus(NullIndex, m_InitialVoltage);
		}

		ComplexComponent::ComplexComponent(int portCount)
			: Component(portCount)
		{ }

	}
}