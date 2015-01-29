#include "stdafx.h"
#include "ObjectModel.h"
#include "NetworkCase.h"
#include "Exceptions.h"
#include <algorithm>

using namespace PowerSolutions::ObjectModel;
using namespace std;

namespace PowerSolutions {
	namespace ObjectModel {
		////////// 案例 //////////
		void NetworkCase::Validate() const
		{
			for_each(m_Components.begin(), m_Components.end(),
				[this](shared_ptr<Component> v){
				//确保所有元件的有效性。
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
			//移除所有成员
			m_Components.clear();
			m_Buses.clear();
		}

		shared_ptr<NetworkObject> NetworkCase::Attach(NetworkObject* obj)
		{
			assert(obj != nullptr);
			auto prev = obj->CaseInfo();
			//设置父级
			auto ptr = (prev != nullptr ? prev->Detach(obj) : shared_ptr<NetworkObject>(obj));
			AttachCore(ptr);
			return ptr;
		}

		void NetworkCase::Attach(std::shared_ptr<NetworkObject> obj)
		{
			assert(obj != nullptr);
			auto prev = obj->CaseInfo();
			//设置父级
			if (prev == this) return;
			if (prev != nullptr)
				prev->Detach(obj.get());
			AttachCore(obj);
		}

		void NetworkCase::AttachCore(std::shared_ptr<NetworkObject> obj)
		{
			obj->CaseInfo(this);
			//放入合适的列表
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
				//检查组件是否包含子级
				//将子级也移入此案例
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
			//用户操作时，不允许将依附于其它元件的子元件直接移除。
			assert(obj->Parent() == nullptr);
			shared_ptr<NetworkObject> ptr;
			auto b = dynamic_cast<Bus*>(obj);
			if (b != nullptr)
			{
				auto pos = find_if(m_Buses.begin(), m_Buses.end(), [obj](shared_ptr<Bus> item){return item.get() == obj; });
				//如果断言失败，表明存在着一个母线，其指向了此 NetworkCase，但 NetworkCase 并未收录。
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
			//复制自身
			auto nc = new NetworkCase();
			//复制母线
			for (auto& b : m_Buses)
			{
				//仅复制独立的母线。
				if (b->Parent() == nullptr)
				{
					nc->AttachCore(shared_ptr<Bus>(b->Clone(nc)));
				}
			}
			//复制元件
			for (auto& c : m_Components)
			{
				//仅复制独立的元件。例如三绕组变压器的内部元件将不会被复制。
				if (c->Parent() == nullptr)
				{
					auto newComponent = c->Clone(nc);
					//在Clone之后，由于元件绑定到了母线上，因此一般不需要手动绑定。
					//nc->AttachCore(shared_ptr<Component>());
					assert(newComponent->CaseInfo() == nc);
				}
			}
			return nc;
		}
	}
}