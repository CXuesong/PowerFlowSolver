#include "stdafx.h"
#include "ObjectModel.h"
#include "NetworkCase.h"
#include "Exceptions.h"
#include <algorithm>

using namespace PowerSolutions::ObjectModel;
using namespace std;

namespace PowerSolutions {
	namespace ObjectModel {
		////////// ���� //////////
		void NetworkCase::Validate() const
		{
			for_each(m_Components.begin(), m_Components.end(),
				[this](shared_ptr<Component> v){
				//ȷ������Ԫ������Ч�ԡ�
				assert(v->CaseInfo() == this);
			});
		}

		NetworkCase::NetworkCase()
		{ }

		NetworkCase::~NetworkCase()
		{
			this->Clear();
		}

		void NetworkCase::Clear()
		{
			for (auto &c : m_Components)
			{
				c->CaseInfo(nullptr);
			}
			for (auto &b : m_Buses)
			{
				b->CaseInfo(nullptr);
			}
			//�Ƴ����г�Ա
			m_Components.clear();
			m_Buses.clear();
		}

		shared_ptr<NetworkObject> NetworkCase::Attach(NetworkObject* obj)
		{
			assert(obj != nullptr);
			auto prev = obj->CaseInfo();
			//���ø���
			auto ptr = (prev != nullptr ? prev->Detach(obj) : shared_ptr<NetworkObject>(obj));
			AttachCore(ptr);
			return ptr;
		}

		void NetworkCase::Attach(std::shared_ptr<NetworkObject> obj)
		{
			assert(obj != nullptr);
			auto prev = obj->CaseInfo();
			//���ø���
			if (prev == this) return;
			if (prev != nullptr)
				prev->Detach(obj.get());
			AttachCore(obj);
		}

		void NetworkCase::AttachCore(std::shared_ptr<NetworkObject> obj)
		{
			obj->CaseInfo(this);
			//������ʵ��б�
			auto b = dynamic_pointer_cast<Bus>(obj);
			if (b)
			{
				m_Buses.push_back(b);
			} else {
#if _DEBUG
				auto cp = dynamic_pointer_cast<Component>(obj);
				assert(cp);
#else
				auto cp = static_pointer_cast<Component>(obj);
#endif
				//�������Ƿ�����Ӽ�
				//���Ӽ�Ҳ����˰���
				auto container = dynamic_pointer_cast<IContainer>(cp);
				if (container != nullptr)
				{
					for (int i = 0; i < container->ChildrenCount(); i++)
					{
						Attach(container->ChildAt(i));
					}
				}
				m_Components.push_back(cp);
			}
		}

		shared_ptr<NetworkObject> NetworkCase::Detach(NetworkObject* obj)
		{
			assert(obj != nullptr);
			assert(obj->CaseInfo() == this);
			//�û�����ʱ������������������Ԫ������Ԫ��ֱ���Ƴ���
			assert(obj->Parent() == nullptr);
			shared_ptr<NetworkObject> ptr;
			auto b = dynamic_cast<Bus*>(obj);
			if (b != nullptr)
			{
				auto pos = find_if(m_Buses.begin(), m_Buses.end(), [obj](shared_ptr<Bus> item){return item.get() == obj; });
				//�������ʧ�ܣ�����������һ��ĸ�ߣ���ָ���˴� NetworkCase���� NetworkCase ��δ��¼��
				assert(pos != m_Buses.end());
				ptr = *pos;
				m_Buses.erase(pos);
			} else {
#if _DEBUG
				auto c = dynamic_cast<Component*>(obj);
				assert(c != nullptr);
#else
				auto c = static_cast<Component*>(obj);
#endif
				auto pos = find_if(m_Components.begin(), m_Components.end(), [obj](shared_ptr<Component> item){return item.get() == obj; });
				assert(pos != m_Components.end());
				ptr = *pos;
				m_Components.erase(pos);
			}
			obj->CaseInfo(nullptr);
			return ptr;
		}

		shared_ptr<Bus> NetworkCase::Buses(int index) const
		{
			auto i = find_if(m_Buses.cbegin(), m_Buses.cend(),
				[index](const shared_ptr<Bus>& item){return item.get()->Index() == index; });
			if (i != m_Buses.cend())
				return *i;
			else
				return nullptr;
		}

		std::shared_ptr<Component> NetworkCase::Components(int index) const
		{
			auto i = find_if(m_Components.cbegin(), m_Components.cend(),
				[index](const shared_ptr<Component> &item){return item.get()->Index() == index; });
			if (i != m_Components.cend())
				return *i;
			else
				return nullptr;
		}

		shared_ptr<Bus> NetworkCase::CreateBus(complexd inititalVoltage)
		{
			int newIndex = m_Buses.size();
			auto newInst = new Bus(newIndex, inititalVoltage);
			return static_pointer_cast<Bus>(Attach(newInst));
		}

		NetworkCase* NetworkCase::Clone()
		{
			//��������
			auto nc = new NetworkCase();
			//����ĸ��
			for (auto& b : m_Buses)
			{
				//�����ƶ�����ĸ�ߡ�
				if (b->Parent() == nullptr)
				{
					nc->AttachCore(shared_ptr<Bus>(b->Clone(nc)));
				}
			}
			//����Ԫ��
			for (auto& c : m_Components)
			{
				//�����ƶ�����Ԫ���������������ѹ�����ڲ�Ԫ�������ᱻ���ơ�
				if (c->Parent() == nullptr)
				{
					auto newComponent = c->Clone(nc);
					//��Clone֮������Ԫ���󶨵���ĸ���ϣ����һ�㲻��Ҫ�ֶ��󶨡�
					//nc->AttachCore(shared_ptr<Component>());
					assert(newComponent->CaseInfo() == nc);
				}
			}
			return nc;
		}
	}
}