#ifndef __POWERSOLUTIONS_POWERFLOW_NETWORKCASE_H
#define __POWERSOLUTIONS_POWERFLOW_NETWORKCASE_H

#include "PowerSolutions.h"
#include "ObjectModel.h"
#include <list>
#include <unordered_map>
#include <memory>

namespace PowerSolutions {
	namespace ObjectModel {
		//�����Ѿ�չ���������� ComplexComponent ������Ԫ����Ϣ��
		struct ExpandedNetworkCase
		{
		public:
			typedef std::list<Bus*> BusCollection;
			typedef std::list<Component*> ComponentCollection;
			typedef std::unordered_multimap<Bus*, Component*> BusComponentCollection;
		private:
			std::shared_ptr<BusCollection> m_Buses;
			std::shared_ptr<ComponentCollection> m_Components;
			std::shared_ptr<BusComponentCollection> m_BusComponents;
		private:	//internal
			friend class NetworkCase;
			void AddObject(NetworkObject* obj);
		public:
			const BusCollection& Buses() const { return *m_Buses; }
			const ComponentCollection& Components() const { return *m_Components; }
			const BusComponentCollection& BusComponents() const { return *m_BusComponents; }
		public:
			ExpandedNetworkCase();
			~ExpandedNetworkCase();
		};

		class NetworkCaseCloneContext
		{
		private:
			std::unordered_map<NetworkObject*, NetworkObject*> objectMapping;
		public:
			void MapObject(NetworkObject* oldObj, NetworkObject* newObj);
			NetworkObject* GetNewObject(NetworkObject* oldObj) const;
		public:
			NetworkCaseCloneContext(std::size_t objectCount)
				: objectMapping()
			{ }
		};

		// ��ʾһ�����ڷ��������簸����
		// ע��˴��ٶ����е�Ԫ���������ǰ���õ�ԭ��
		class NetworkCase
		{
			friend class NetworkObject;
		public:
			typedef std::list<NetworkObject*> NetworkObjectCollection;
			typedef NetworkObjectCollection::const_iterator NetworkObjectIterator;
		private:
			NetworkObjectCollection m_Objects;
			bool m_AutoDeleteChildren;
		private:	//internal
		public:
			//��������б�ֻ��ö��������ʼλ�á�
			NetworkObjectIterator ObjectsBegin() const { return m_Objects.cbegin(); }
			//��������б�ֻ��ö�����Ľ���λ�á�
			NetworkObjectIterator ObjectsEnd() const { return m_Objects.cend(); }
			//��������б��Ԫ��������
			size_t ObjectsCount() const { return m_Objects.size(); }
			//���� Index ���Բ���һ��ĸ�ߡ�
			Bus* Buses(int index) const;
			//�����簸���м���һ��ĸ�ߡ�
			//ע�⣺Ĭ������£�����û�д����簸����ɾ��ĸ�߻����ĸ�߱��ʱ�����Ա�֤��ŵ���ȷ�ԡ�
			//������ܵ����ظ���š�
			Bus* CreateBus(complexd inititalVoltage);
			//�����簸���м���һ��ĸ�ߡ�
			//ע�⣺Ĭ������£�����û�д����簸����ɾ��ĸ�߻����ĸ�߱��ʱ�����Ա�֤��ŵ���ȷ�ԡ�
			//������ܵ����ظ���š�
			Bus* CreateBus() { return CreateBus(1); };
			//���� Index ���Բ���һ��Ԫ����
			Component* Components(int index) const;
			//ָʾ�Ƿ�Ӧ�ڴ�ʵ������ʱ�Զ�ɾ���Ӽ��������
			bool AutoDeleteChildren() const { return m_AutoDeleteChildren; }
			void AutoDeleteChildren(bool val) { m_AutoDeleteChildren = val; }
		private:
			static void ExpandCore(ExpandedNetworkCase& enc, NetworkObject* obj);
		public:
			void AddObject(NetworkObject* obj);
			void AddObject(std::initializer_list<NetworkObject*> init);
			bool RemoveObject(NetworkObject* obj);
			void DeleteChildren();				//�Ƴ���ɾ��������ʵ���е������Ӽ���
			void Validate() const;				//��֤��������ʵ������Ч�ԡ�
			ExpandedNetworkCase Expand() const;
			NetworkCase* Clone();				//��������簸����һ��������
		public:
			NetworkCase();
			~NetworkCase();
			//������ʹ�ø��ƹ��캯����
			NetworkCase(const NetworkCase&) = delete;
		};
	}
}
#endif //__POWERSOLUTIONS_POWERFLOW_NETWORKCASE_H
