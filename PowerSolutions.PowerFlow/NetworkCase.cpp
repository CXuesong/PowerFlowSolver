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
		////////// ���� //////////
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
			//�Ƴ����г�Ա
			m_Objects.clear();
		}

		void NetworkCase::AddObject(NetworkObject* obj)
		{
			assert(obj);
			//�����б�
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
			if (m_AutoDeleteChildren) delete obj;
			m_Objects.erase(pos);
			return true;
		}

		Bus* NetworkCase::CreateBus(complexd inititalVoltage)
		{
			auto newInst = new Bus(inititalVoltage);
			AddObject(newInst);
			return newInst;
		}

		NetworkCase* NetworkCase::Clone(NetworkCaseTrackingInfo*& trackingInfo)
		{
			//ע���ڴ����⣺�˴��������ڴ棬��û���ͷ�
			// delete �����Ҫ���ڴ�ķ��䷽���ͷŷ�������ͬһ����
			// ��������ͬһ�� Dll �С�
			//��������
			auto nc = new NetworkCase();
			auto context = new NetworkCaseTrackingInfo(m_Objects.size());
			nc->m_AutoDeleteChildren = m_AutoDeleteChildren;
			//����ĸ�ߺ�Ԫ��
			//ע��˴��ٶ����е�Ԫ���������ǰ���õ�ԭ��
			for (auto& obj : m_Objects)
			{
				auto newObj = obj->Clone(*context);
				context->MapObject(obj, newObj);
				nc->AddObject(newObj);
			}
			trackingInfo = context;
			return nc;
		}

		NetworkCase* NetworkCase::ShallowClone()
		{
			auto nc = new NetworkCase();
			nc->m_AutoDeleteChildren = false;
			nc->m_Objects = m_Objects;
			return nc;
		}

		//PrimitiveNetwork* NetworkCase::Expand() const
		//{
		//	PrimitiveNetwork ec;
		//	for (auto& obj : m_Objects)
		//	{
		//		ExpandCore(ec, obj);
		//	}
		//	return ec;
		//}

		//void NetworkCase::ExpandCore(PrimitiveNetwork* enc, NetworkObject* obj)
		//{
		//	assert(obj);
		//	obj->OnExpand();
		//	auto cc = dynamic_cast<ComplexComponent*>(obj);
		//	if (cc == nullptr)
		//	{
		//		enc.AddObject(obj);
		//	} else {
		//		//��Ҫչ���Ӽ�
		//		int count = cc->ChildrenCount();
		//		for (int i = 0; i < count; i++)
		//			ExpandCore(enc, cc->ChildAt(i));
		//	}
		//	return;
		//}

		//void ExpandedNetworkCase::AddObject(NetworkObject* obj)
		//{
		//	//����������ʵ��б��м�����Ŀ�������� ComplexComponent ��չ����
		//	assert(obj);
		//	auto b = dynamic_cast<Bus*>(obj);
		//	if (b != nullptr)
		//	{
		//		m_Buses->push_back(b);
		//		return;
		//	}
		//	auto c = dynamic_cast<Component*>(obj);
		//	if (c != nullptr)
		//	{
		//		assert(IsKindOf<ComplexComponent>(obj) == false);
		//		m_Components->push_back(c);
		//		//TODO ���Ǽ�� Buses(i) �Ƿ�Ϊ null��
		//		for (int i = 0; i < c->PortCount(); i++)
		//			m_BusComponents->emplace(c->Buses(i), c);
		//	}
		//}

		//ExpandedNetworkCase::ExpandedNetworkCase()
		//	: m_Buses(new BusCollection),
		//	m_Components(new ComponentCollection),
		//	m_BusComponents(new BusComponentCollection)
		//{ }

		//ExpandedNetworkCase::~ExpandedNetworkCase()
		//{ }

		void NetworkCaseTrackingInfo::MapObject(NetworkObject* oldObj, NetworkObject* newObj)
		{
			assert(typeid(&oldObj) == typeid(&newObj));
			objectMapping.emplace(oldObj, MappingInfo(true, newObj));
			objectMapping.emplace(newObj, MappingInfo(true, oldObj));
		}

		NetworkObject* NetworkCaseTrackingInfo::CloneOf(NetworkObject* prototypeObj) const
		{
			auto i = objectMapping.find(prototypeObj);
			if (i == objectMapping.end()) throw Exception(ExceptionCode::InvalidArgument);
			//���������Ѿ��Ǹ����ˣ����Կ����Ƿ������쳣����ֱ�ӷ��ص�ǰ����ָ�����Ǹ�������
			if (!i->second.isPrototype) throw Exception(ExceptionCode::InvalidOperation);
			return i->second.anotherObject;
		}

		NetworkObject* NetworkCaseTrackingInfo::PrototypeOf(NetworkObject* cloneObj) const
		{
			auto i = objectMapping.find(cloneObj);
			if (i == objectMapping.end()) throw Exception(ExceptionCode::InvalidArgument);
			if (!i->second.isPrototype) throw Exception(ExceptionCode::InvalidOperation);
			return i->second.anotherObject;
		}
	}
}