#ifndef __POWERSOLUTIONS_POWERFLOW_NETWORKCASE_H
#define __POWERSOLUTIONS_POWERFLOW_NETWORKCASE_H

#include <memory>
#include <unordered_set>
#include <unordered_map>

namespace PowerSolutions {
	namespace ObjectModel {
		class NetworkObject;
		// 表示一个用于分析的网络案例。
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
			const std::unordered_map<int, std::shared_ptr<Bus>>& Buses()	//总线列表。
			{ return m_Buses; }
			const std::list<std::shared_ptr<Component>>& Components()		//网络元件的列表。
			{ return m_Components; }
			void* Tag() const { return m_Tag; }		//应用程序自定义的附加信息。
			void Tag(void* val) { m_Tag = val; }
		public:
			void Attach(NetworkObject* obj);
			void Attach(std::shared_ptr<NetworkObject> obj);
			void Detach(NetworkObject* obj);
			void Clear();						//清除网络实例中的所有数据。
			void Validate() const;				//验证整个网络实例的有效性。
		public:
			NetworkCase();
			~NetworkCase();
		};
	}
}
#endif //__POWERSOLUTIONS_POWERFLOW_NETWORKCASE_H