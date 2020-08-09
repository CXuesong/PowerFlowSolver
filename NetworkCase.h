#ifndef __POWERSOLUTIONS_POWERFLOW_NETWORKCASE_H
#define __POWERSOLUTIONS_POWERFLOW_NETWORKCASE_H

#include <memory>
#include <unordered_set>
#include <unordered_map>

namespace PowerSolutions {
	namespace ObjectModel {
		class NetworkObject;
		// ��ʾһ�����ڷ��������簸����
		class NetworkCase
		{
			friend class NetworkObject;
		private:
			std::unordered_map<int, std::shared_ptr<Bus>> m_Buses;
			std::list<std::shared_ptr<Component>> m_Components;
			void* m_Tag;
		private:	//internal
			bool OnChildIndexChanged(NetworkObject* obj, int oldIndex, int newIndex);
		public:
			const std::unordered_map<int, std::shared_ptr<Bus>>& Buses()	//�����б�
			{ return m_Buses; }
			const std::list<std::shared_ptr<Component>>& Components()		//����Ԫ�����б�
			{ return m_Components; }
			void* Tag() const { return m_Tag; }		//Ӧ�ó����Զ���ĸ�����Ϣ��
			void Tag(void* val) { m_Tag = val; }
		public:
			void Attach(NetworkObject* obj);
			void Attach(std::shared_ptr<NetworkObject> obj);
			void Detach(NetworkObject* obj);
			void Clear();						//�������ʵ���е��������ݡ�
			void Validate() const;				//��֤��������ʵ������Ч�ԡ�
		public:
			NetworkCase();
			~NetworkCase();
		};
	}
}
#endif //__POWERSOLUTIONS_POWERFLOW_NETWORKCASE_H