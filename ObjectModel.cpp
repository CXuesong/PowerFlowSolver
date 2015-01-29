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
			: m_Index(index), m_CaseInfo(nullptr), m_Parent(nullptr), m_BaseValue(1, 1)
		{ }

		void NetworkObject::Validate() const
		{

		}

		NetworkObject* NetworkObject::Clone(NetworkCase* caseInfo) const
		{
			auto newInstance = CloneInstance();
			//检查派生类是否正确完成了复制工作。
			assert(typeid(*newInstance) == typeid(*this));
			//设置基类属性。
			OnCloned(newInstance, caseInfo);
			return newInstance;
		}

		void NetworkObject::OnCloned(NetworkObject* newInstance, NetworkCase* caseInfo) const
		{
			//约定：Clone不会复制指向父级的指针，
			//且不在对应 caseInfo 中主动调用 Attach 以确定依存关系。
			//但可以复制母线的引用（此处需要用到新的 caseInfo 实例）
			newInstance->m_Index = m_Index;
			newInstance->m_CaseInfo = nullptr;
			newInstance->m_Parent = nullptr;
		}

		////////// 组件 //////////

		Component::Component(int portCount)
			: NetworkObject(NullIndex), m_Buses(portCount)
		{ }

		void Component::Buses(int index, Bus* value)
		{
			if (m_Buses[index] == value) return;
			if (value != nullptr)
			{
				if (value->CaseInfo() != CaseInfo())
				{
					//加入新的 NetworkCase
					//如果需要，则脱离原来的 NetworkCase
					value->CaseInfo()->Attach(this);
				}
				value->AttachComponent(this);
			}
			m_Buses[index] = value;
		}

		void Component::CaseInfo(NetworkCase* val)
		{
			if (val != NetworkObject::CaseInfo())
			{
				//需要将元件从原来的案例中的母线上脱离出来。
				//约定 NetworkCase 中的元件列表由 NetworkCase 自己来进行维护。
				//此处仅改变元件本身的状态。
				for (auto& b : m_Buses)
				{
					if (b != nullptr && b->CaseInfo() != val)
					{
						b->DetachComponent(this);
						b = nullptr;
					}
				}
			}
			NetworkObject::CaseInfo(val);
		}

		void Component::OnCloned(NetworkObject* newInstance, NetworkCase* caseInfo) const
		{
			NetworkObject::OnCloned(newInstance, caseInfo);
			auto inst = static_cast<Component*>(newInstance);
			//根据索引复制母线。
			if (caseInfo != nullptr)
			{
				for (size_t i = 0; i < m_Buses.size(); i++)
				{
					if (m_Buses[i] != nullptr)
						inst->Buses(i, caseInfo->Buses(m_Buses[i]->Index()).get());
				}
			}
		}

		SinglePortComponent::SinglePortComponent()
			: Component(1)
		{ }

		DoublePortComponent::DoublePortComponent()
			: Component(2)
		{ }

		TriPortComponent::TriPortComponent()
			: Component(3)
		{ }

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

		void Bus::AttachComponent(Component* c)
		{
			//注意：此处没有检查c是否在此操作之前已经加入列表。
			m_Components.push_back(c);
		}

		void Bus::DetachComponent(Component* c)
		{
			m_Components.erase(find(m_Components.begin(), m_Components.end(), c));
		}

		NetworkObject* Bus::CloneInstance() const
		{
			return new Bus(NullIndex, m_InitialVoltage);
		}
}
}