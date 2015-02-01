#ifndef __POWERSOLUTIONS_POWERFLOW_NETWORKCASE_H
#define __POWERSOLUTIONS_POWERFLOW_NETWORKCASE_H

#include "PowerSolutions.h"
#include "ObjectModel.h"
#include <list>
#include <unordered_map>
#include <memory>

namespace PowerSolutions {
	namespace ObjectModel {
		//保存已经展开、不包含 ComplexComponent 的网络元件信息。
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

		// 表示一个用于分析的网络案例。
		// 注意此处假定所有的元件均遵从向前引用的原则
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
			//网络对象列表只读枚举器的起始位置。
			NetworkObjectIterator ObjectsBegin() const { return m_Objects.cbegin(); }
			//网络对象列表只读枚举器的结束位置。
			NetworkObjectIterator ObjectsEnd() const { return m_Objects.cend(); }
			//网络对象列表的元素数量。
			size_t ObjectsCount() const { return m_Objects.size(); }
			//按照 Index 属性查找一个母线。
			Bus* Buses(int index) const;
			//向网络案例中加入一条母线。
			//注意：默认情况下，仅当没有从网络案例中删除母线或更改母线编号时，可以保证编号的正确性。
			//否则可能导致重复编号。
			Bus* CreateBus(complexd inititalVoltage);
			//向网络案例中加入一条母线。
			//注意：默认情况下，仅当没有从网络案例中删除母线或更改母线编号时，可以保证编号的正确性。
			//否则可能导致重复编号。
			Bus* CreateBus() { return CreateBus(1); };
			//按照 Index 属性查找一个元件。
			Component* Components(int index) const;
			//指示是否应在此实例析构时自动删除子级网络对象。
			bool AutoDeleteChildren() const { return m_AutoDeleteChildren; }
			void AutoDeleteChildren(bool val) { m_AutoDeleteChildren = val; }
		private:
			static void ExpandCore(ExpandedNetworkCase& enc, NetworkObject* obj);
		public:
			void AddObject(NetworkObject* obj);
			void AddObject(std::initializer_list<NetworkObject*> init);
			bool RemoveObject(NetworkObject* obj);
			void DeleteChildren();				//移除并删除此网络实例中的所有子级。
			void Validate() const;				//验证整个网络实例的有效性。
			ExpandedNetworkCase Expand() const;
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
