#include "ObjectModel.h"
#include "NetworkCase.h"
#include "Exceptions.h"
#include <algorithm>

using namespace PowerSolutions::ObjectModel;
using namespace std;

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

	for_each(m_Components.begin(), m_Components.end(),
		[this](shared_ptr<NetworkObject> v){
		v->CaseInfo(nullptr);
	});
	for_each(m_Buses.begin(), m_Buses.end(),
		[this](pair<int, shared_ptr<Bus>> v){
		v.second->CaseInfo(nullptr);
	});
	//�Ƴ����г�Ա
	m_Components.clear();
	m_Buses.clear();
}

void NetworkCase::Attach(shared_ptr<NetworkObject> obj)
{
	//���ø���
	auto prev = obj->CaseInfo();
	if (prev != nullptr) prev->Detach(obj.get());
	obj->CaseInfo(this);
	//������ʵ��б�
	auto b = dynamic_pointer_cast<Bus>(obj);
	if (b != nullptr)
	{
		m_Buses.insert(make_pair(obj->Index(), b));
	} else {
#if _DEBUG
		auto c = dynamic_pointer_cast<Component>(obj);
		assert(c != nullptr);
#else
		auto c = static_pointer_cast<Component>(obj);
#endif
		m_Components.push_back(c);
	}
}

void NetworkCase::Attach(NetworkObject* obj)
{
	Attach(shared_ptr<NetworkObject>(obj));
}

void NetworkCase::Detach(NetworkObject* obj)
{
	if (obj->CaseInfo() != this || obj->Parent() != nullptr)
		throw Exception(ExceptionCode::InvalidOperation);
	auto b = dynamic_cast<Bus*>(obj);
	if (b != nullptr)
	{
		assert(m_Buses[b->Index()].get() == b);
		m_Buses.erase(b->Index());
	} else {
#if _DEBUG
		auto c = dynamic_cast<Component*>(obj);
		assert(c != nullptr);
#else
		auto c = static_cast<Component*>(obj);
#endif
		auto eresult = m_Components.erase(find_if(m_Components.begin(), m_Components.end(),
			[c](shared_ptr<Component> item){return item.get() == c; }));
		assert(eresult != m_Components.end());
	}
	obj->CaseInfo(nullptr);
}


bool NetworkCase::OnChildIndexChanged(NetworkObject* obj, int oldIndex, int newIndex)
{
	auto b = dynamic_cast<Bus*>(obj);
	if (b != nullptr)
	{
		//��� newIndex �Ƿ��Ѿ�����
		if (m_Buses.find(newIndex) != m_Buses.end()) return false;
		auto bp = m_Buses[oldIndex];
		assert(bp.get() == obj);
		m_Buses.erase(oldIndex);
		m_Buses.insert(make_pair(newIndex, bp));
	}
	//�����Ԫ���������账��
	return true;
}


