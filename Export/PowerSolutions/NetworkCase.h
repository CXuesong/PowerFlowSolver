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
		// 表示一个用于分析的网络案例。
		class NetworkCase
		{
			friend class NetworkObject;
		private:
			BusCollection m_Buses;
			ComponentCollection m_Components;
		private:	//internal
		public:
			//母线列表只读枚举器的起始位置。
			BusIterator BusesBegin() const { return m_Buses.cbegin(); }
			//母线列表只读枚举器的结束位置。
			BusIterator BusesEnd() const { return m_Buses.cend(); }
			//母线列表的元素数量。
			size_t BusesCount() const { return m_Buses.size(); }
			//按照 Index 属性查找一个母线。
			std::shared_ptr<Bus> Buses(int index) const;
			//向网络案例中加入一条母线。
			//注意：默认情况下，仅当没有从网络案例中删除母线或更改母线编号时，可以保证编号的正确性。
			//否则可能导致重复编号。
			std::shared_ptr<Bus> CreateBus(complexd inititalVoltage);
			//向网络案例中加入一条母线。
			//注意：默认情况下，仅当没有从网络案例中删除母线或更改母线编号时，可以保证编号的正确性。
			//否则可能导致重复编号。
			std::shared_ptr<Bus> CreateBus() { return CreateBus(1); };
			//元件列表只读枚举器的起始位置。
			CompoentIterator ComponentsBegin() const { return m_Components.cbegin(); }
			//元件列表只读枚举器的结束位置。
			CompoentIterator ComponentsEnd() const { return m_Components.cend(); }
			//元件列表的元素数量。
			size_t ComponentsCount() const { return m_Components.size(); }
			//按照 Index 属性查找一个元件。
			std::shared_ptr<Component> Components(int index) const;
		private:
			void AttachCore(std::shared_ptr<NetworkObject> obj);
		public:
			void Attach(std::shared_ptr<NetworkObject> obj);
			std::shared_ptr<NetworkObject> Attach(NetworkObject* obj);
			std::shared_ptr<NetworkObject> Detach(NetworkObject* obj);
			void Clear();						//清除网络实例中的所有数据。
			void Validate() const;				//验证整个网络实例的有效性。
			NetworkCase* Clone();				//构造此网络案例的一个副本。
		public:
			NetworkCase();
			~NetworkCase();
			//不允许使用复制构造函数。
			NetworkCase(const NetworkCase&) = delete;
		};
	}
}
#endif //__POWERSOLUTIONS_POWERFLOW_NETWORKCASE_H