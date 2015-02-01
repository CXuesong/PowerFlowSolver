#include "stdafx.h"
#include "ObjectModel.h"
#include "NetworkCase.h"
#include "Exceptions.h"
#include "Utility.h"
#include <algorithm>

using namespace PowerSolutions::ObjectModel;
using namespace PowerSolutions::Utility;
using namespace std;

namespace PowerSolutions {
	namespace ObjectModel {
		////////// 案例 //////////
		NetworkCase::NetworkCase()
			: m_AutoDeleteChildren(true)
		{ }

		NetworkCase::~NetworkCase()
		{
			if (m_AutoDeleteChildren) DeleteChildren();
		}

		void NetworkCase::DeleteChildren()
		{
			for (auto &obj : m_Objects)
				delete obj;
			//移除所有成员
			m_Objects.clear();
		}

		void NetworkCase::AddObject(NetworkObject* obj)
		{
			assert(obj);
			//放入列表
			m_Objects.push_back(obj);
		}

		void NetworkCase::AddObject(std::initializer_list<NetworkObject*> init)
		{
			assert(!any_of(init.begin(), init.end(), [](NetworkObject* obj){return obj == nullptr; }));
			m_Objects.insert(m_Objects.end(), init.begin(), init.end());
		}

		bool NetworkCase::RemoveObject(NetworkObject* obj)
		{
			assert(obj);
			auto pos = find_if(m_Objects.cbegin(), m_Objects.cend(),
				[&obj](const NetworkObject* item){return item == obj; });
			if (pos == m_Objects.end()) return false;
			m_Objects.erase(pos);
			return true;
		}

		Bus* NetworkCase::CreateBus(complexd inititalVoltage)
		{
			auto newInst = new Bus(inititalVoltage);
			AddObject(newInst);
			return newInst;
		}

		NetworkCase* NetworkCase::Clone()
		{
			//复制自身
			auto nc = new NetworkCase();
			NetworkCaseCloneContext context(m_Objects.size());
			//复制母线和元件
			//注意此处假定所有的元件均遵从向前引用的原则
			for (auto& obj : m_Objects)
			{
				auto newObj = obj->Clone(context);
				context.MapObject(obj, newObj);
				nc->AddObject(newObj);
			}
			return nc;
		}

		ExpandedNetworkCase NetworkCase::Expand() const
		{
			ExpandedNetworkCase ec;
			for (auto& obj : m_Objects)
			{
				ExpandCore(ec, obj);
			}
			return ec;
		}

		void NetworkCase::ExpandCore(ExpandedNetworkCase& enc, NetworkObject* obj)
		{
			assert(obj);
			obj->OnExpand();
			auto cc = dynamic_cast<ComplexComponent*>(obj);
			if (cc == nullptr)
			{
				enc.AddObject(obj);
			} else {
				//需要展开子级
				int count = cc->ChildrenCount();
				for (int i = 0; i < count; i++)
					ExpandCore(enc, cc->ChildAt(i));
			}
			return;
		}

		void ExpandedNetworkCase::AddObject(NetworkObject* obj)
		{
			//仅负责向合适的列表中加入项目，不进行 ComplexComponent 的展开。
			assert(obj);
			auto b = dynamic_cast<Bus*>(obj);
			if (b != nullptr)
			{
				m_Buses->push_back(b);
				return;
			}
			auto c = dynamic_cast<Component*>(obj);
			if (c != nullptr)
			{
				assert(IsKindOf<ComplexComponent>(obj) == false);
				m_Components->push_back(c);
				//TODO 考虑检查 Buses(i) 是否为 null。
				for (int i = 0; i < c->PortCount(); i++)
					m_BusComponents->emplace(c->Buses(i), c);
			}
		}

		ExpandedNetworkCase::ExpandedNetworkCase()
			: m_Buses(new BusCollection),
			m_Components(new ComponentCollection),
			m_BusComponents(new BusComponentCollection)
		{ }

		ExpandedNetworkCase::~ExpandedNetworkCase()
		{ }

		void NetworkCaseCloneContext::MapObject(NetworkObject* oldObj, NetworkObject* newObj)
		{
			assert(typeid(&oldObj) == typeid(&newObj));
			objectMapping.emplace(oldObj, newObj);
		}

		NetworkObject* NetworkCaseCloneContext::GetNewObject(NetworkObject* oldObj) const
		{
			auto i = objectMapping.find(oldObj);
			assert(i != objectMapping.end());
			return i == objectMapping.end() ? nullptr : i->second;
		}
	}
}