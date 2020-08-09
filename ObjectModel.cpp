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

		////////// ������� //////////
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
			//����������Ƿ���ȷ����˸��ƹ�����
			assert(typeid(*newInstance) == typeid(*this));
			//���û������ԡ�
			OnCloned(newInstance, caseInfo);
			return newInstance;
		}

		void NetworkObject::OnCloned(NetworkObject* newInstance, NetworkCase* caseInfo) const
		{
			//Լ����Clone���Ḵ��ָ�򸸼���ָ�룬
			//�Ҳ��ڶ�Ӧ caseInfo ���������� Attach ��ȷ�������ϵ��
			//�����Ը���ĸ�ߵ����ã��˴���Ҫ�õ��µ� caseInfo ʵ����
			newInstance->m_Index = m_Index;
			newInstance->m_CaseInfo = nullptr;
			newInstance->m_Parent = nullptr;
		}

		////////// ��� //////////

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
					//�����µ� NetworkCase
					//�����Ҫ��������ԭ���� NetworkCase
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
				//��Ҫ��Ԫ����ԭ���İ����е�ĸ�������������
				//Լ�� NetworkCase �е�Ԫ���б��� NetworkCase �Լ�������ά����
				//�˴����ı�Ԫ�������״̬��
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
			//������������ĸ�ߡ�
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

		////////// ĸ�� //////////
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
			//ע�⣺�˴�û�м��c�Ƿ��ڴ˲���֮ǰ�Ѿ������б�
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