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
		{
			_PS_TRACE("NC Construct " << this);
		}

		NetworkCase::~NetworkCase()
		{
			_PS_TRACE("NC Dispose " << this);
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
			auto pos = find(m_Objects.begin(), m_Objects.end(), obj);
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


		pair < shared_ptr<NetworkCase>, shared_ptr<NetworkCaseCorrespondenceInfo> > NetworkCase::CorrespondenceClone()
		{
			//ע���ڴ����⣺�˴��������ڴ棬��û���ͷ�
			// delete �����Ҫ���ڴ�ķ��䷽���ͷŷ�������ͬһ����
			// ��������ͬһ�� Dll �С�
			//��������
			auto nc = make_shared<NetworkCase>();
			shared_ptr<NetworkCaseCorrespondenceInfo> 
				context(new NetworkCaseCorrespondenceInfo(m_Objects.size()));
			nc->m_AutoDeleteChildren = m_AutoDeleteChildren;
			//����ĸ�ߺ�Ԫ��
			//ע��˴��ٶ����е�Ԫ���������ǰ���õ�ԭ��
			for (auto& obj : m_Objects)
			{
				auto newObj = obj->Clone(*context);
				context->MapObject(obj, newObj);
				nc->AddObject(newObj);
			}
			return make_pair(nc, context);
		}

		shared_ptr<NetworkCase> NetworkCase::ShallowClone() const
		{
			auto nc = make_shared<NetworkCase>();
			nc->m_AutoDeleteChildren = false;
			nc->m_Objects = m_Objects;
			return nc;
		}

		shared_ptr<PrimitiveNetwork> NetworkCase::ToPrimitive(PrimitiveNetworkOptions options)
		{
			shared_ptr<PrimitiveNetwork> newInst(new PrimitiveNetwork());
			newInst->LoadNetworkCase(this, options);
			return newInst;
		}
		void NetworkCaseCorrespondenceInfo::MapObject(NetworkObject* oldObj, NetworkObject* newObj)
		{
			assert(typeid(&oldObj) == typeid(&newObj));
			objectMapping.emplace(oldObj, MappingInfo(true, newObj));
			objectMapping.emplace(newObj, MappingInfo(true, oldObj));
		}

		NetworkObject* NetworkCaseCorrespondenceInfo::CloneOf(NetworkObject* prototypeObj) const
		{
			auto i = objectMapping.find(prototypeObj);
			if (i == objectMapping.end()) throw Exception(ExceptionCode::InvalidArgument);
			//���������Ѿ��Ǹ����ˣ����Կ����Ƿ������쳣����ֱ�ӷ��ص�ǰ����ָ�����Ǹ�������
			if (!i->second.isPrototype) throw Exception(ExceptionCode::InvalidOperation);
			return i->second.anotherObject;
		}

		NetworkObject* NetworkCaseCorrespondenceInfo::PrototypeOf(NetworkObject* cloneObj) const
		{
			auto i = objectMapping.find(cloneObj);
			if (i == objectMapping.end()) throw Exception(ExceptionCode::InvalidArgument);
			if (!i->second.isPrototype) throw Exception(ExceptionCode::InvalidOperation);
			return i->second.anotherObject;
		}
	}
}