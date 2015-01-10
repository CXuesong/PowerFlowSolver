#include "ObjectModel.h"
#include "NetworkCase.h"
#include "Exceptions.h"
#include <algorithm>

using namespace PowerSolutions::ObjectModel;
using namespace std;

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
	for_each(m_Components.begin(), m_Components.end(),
		[this](shared_ptr<NetworkObject> v){
		v->CaseInfo(NULL);
	});
	for_each(m_Buses.begin(), m_Buses.end(),
		[this](pair<int, shared_ptr<Bus>> v){
		v.second->CaseInfo(NULL);
	});
	//移除所有成员
	m_Components.clear();
	m_Buses.clear();
}

void NetworkCase::Attach(shared_ptr<NetworkObject> obj)
{
	//设置父级
	auto prev = obj->CaseInfo();
	if (prev != NULL) prev->Detach(obj);
	obj->CaseInfo(this);
	//放入合适的列表
	auto b = dynamic_pointer_cast<Bus>(obj);
	if (b != NULL)
	{
		m_Buses.insert(make_pair(obj->Index(), b));
	} else {
#if _DEBUG
		auto c = dynamic_pointer_cast<Component>(obj);
		assert(c != NULL);
#else
		auto c = static_pointer_cast<Component>(obj);
#endif
		m_Components.insert(c);
	}
}

void NetworkCase::Detach(shared_ptr<NetworkObject> obj)
{
	if (obj->CaseInfo() != this)
		throw Exception(ExceptionCode::InvalidOperation);
	auto b = dynamic_pointer_cast<Bus>(obj);
	if (b != NULL)
	{
		assert(m_Buses[b->Index()] == b);
		m_Buses.erase(b->Index());
	} else {
#if _DEBUG
		auto c = dynamic_pointer_cast<Component>(obj);
		assert(c != NULL);
#else
		auto c = static_pointer_cast<Component>(obj);
#endif
		m_Components.erase(c);
	}
	obj->CaseInfo(NULL);
}


bool NetworkCase::NotifyObjectIndexChanged(NetworkObject* obj, int oldIndex, int newIndex)
{
	auto b = dynamic_cast<Bus*>(obj);
	if (b != NULL)
	{
		//检查 newIndex 是否已经存在
		if (m_Buses.find(newIndex) != m_Buses.end()) return false;
		auto bp = m_Buses[oldIndex];
		assert(bp.get() == obj);
		m_Buses.erase(oldIndex);
		m_Buses.insert(make_pair(newIndex, bp));
	}
	//如果是元件，则无需处理
	return true;
}


