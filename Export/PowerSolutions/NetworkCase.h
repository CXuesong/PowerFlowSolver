#ifndef __POWERSOLUTIONS_POWERFLOW_NETWORKCASE_H
#define __POWERSOLUTIONS_POWERFLOW_NETWORKCASE_H

#include "PowerSolutions.h"
#include "ObjectModel.h"
#include <memory>
#include <unordered_set>
#include <unordered_map>

namespace PowerSolutions {
	namespace ObjectModel {
		class NetworkObject;
		typedef std::list<std::shared_ptr<Bus>> BusCollection;
		typedef BusCollection::const_iterator BusIterator;
		typedef std::list<std::shared_ptr<Component>> ComponentCollection;
		typedef ComponentCollection::const_iterator CompoentIterator;
		// ��ʾһ�����ڷ��������簸����
		class NetworkCase
		{
			friend class NetworkObject;
		private:
			BusCollection m_Buses;
			ComponentCollection m_Components;
		private:	//internal
		public:
			//ĸ���б�ֻ��ö��������ʼλ�á�
			BusIterator BusesBegin() const { return m_Buses.cbegin(); }
			//ĸ���б�ֻ��ö�����Ľ���λ�á�
			BusIterator BusesEnd() const { return m_Buses.cend(); }
			//ĸ���б��Ԫ��������
			size_t BusesCount() const { return m_Buses.size(); }
			//���� Index ���Բ���һ��ĸ�ߡ�
			std::shared_ptr<Bus> Buses(int index) const;
			//�����簸���м���һ��ĸ�ߡ�
			//ע�⣺Ĭ������£�����û�д����簸����ɾ��ĸ�߻����ĸ�߱��ʱ�����Ա�֤��ŵ���ȷ�ԡ�
			//������ܵ����ظ���š�
			std::shared_ptr<Bus> CreateBus(complexd inititalVoltage);
			//�����簸���м���һ��ĸ�ߡ�
			//ע�⣺Ĭ������£�����û�д����簸����ɾ��ĸ�߻����ĸ�߱��ʱ�����Ա�֤��ŵ���ȷ�ԡ�
			//������ܵ����ظ���š�
			std::shared_ptr<Bus> CreateBus() { return CreateBus(1); };
			//Ԫ���б�ֻ��ö��������ʼλ�á�
			CompoentIterator ComponentsBegin() const { return m_Components.cbegin(); }
			//Ԫ���б�ֻ��ö�����Ľ���λ�á�
			CompoentIterator ComponentsEnd() const { return m_Components.cend(); }
			//Ԫ���б��Ԫ��������
			size_t ComponentsCount() const { return m_Components.size(); }
			//���� Index ���Բ���һ��Ԫ����
			std::shared_ptr<Component> Components(int index) const;
		private:
			void AttachCore(std::shared_ptr<NetworkObject> obj);
		public:
			void Attach(std::shared_ptr<NetworkObject> obj);
			std::shared_ptr<NetworkObject> Attach(NetworkObject* obj);
			std::shared_ptr<NetworkObject> Detach(NetworkObject* obj);
			void Clear();						//�������ʵ���е��������ݡ�
			void Validate() const;				//��֤��������ʵ������Ч�ԡ�
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