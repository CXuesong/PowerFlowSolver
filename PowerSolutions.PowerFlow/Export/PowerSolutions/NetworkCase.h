#ifndef __POWERSOLUTIONS_POWERFLOW_NETWORKCASE_H
#define __POWERSOLUTIONS_POWERFLOW_NETWORKCASE_H

#include "PowerSolutions.h"
#include "ObjectModel.h"
#include "Utility.h"
#include "PrimitiveNetwork.h"
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <memory>

namespace PowerSolutions {
	namespace ObjectModel {

		// 在执行 NetworkCase::Clone 时用于将原来的网络案例（原型，Prototype）中的某些网络对象（尤其是母线）映射到新的网络案例（副本，Clone）中。
		// 在使用 NetworkCase::TrackingClone 后，用于实现原型网络案例和副本网络案例对象之间的对应。
		class NetworkCaseCorrespondenceInfo
		{
		private:
			struct MappingInfo
			{
			public:
				bool isPrototype;					//此元件是否来自于原型网络案例。
				const NetworkObject *anotherObject;	//如果 isPrototype = true，则保存了副本网络案例中对应的对象，反之亦然。
			public:
				MappingInfo(bool _isPrototype, const NetworkObject* _anotherObject)
					: isPrototype(_isPrototype), anotherObject(_anotherObject)
				{ }
			};
			std::unordered_map<const NetworkObject*, MappingInfo> objectMapping;
		private:	//internal
			friend class NetworkCase;
			void MapObject(const NetworkObject* oldObj, const NetworkObject* newObj);
		public:
			// 获取原型网络案例中指定网络对象在副本网络案例中的对应对象。
			const NetworkObject* CloneOf(const NetworkObject* prototypeObj) const;
			// 获取副本网络案例中指定网络对象在原型网络案例中的对应对象。
			const NetworkObject* PrototypeOf(const NetworkObject* cloneObj) const;
			template <class TObj>
			const TObj* CloneOfStatic(const TObj* prototypeObj) const {
				return static_cast<const TObj*>(CloneOf(static_cast<const NetworkObject*>(prototypeObj)));
			}
			template <class TObj>
			const TObj* PrototypeOfStatic(const TObj* cloneObj) const {
				return static_cast<const TObj*>(PrototypeOf(static_cast<const NetworkObject*>(cloneObj)));
			}
		private:	//internal
			NetworkCaseCorrespondenceInfo(std::size_t objectCount)
			{
				objectMapping.reserve(objectCount * 2);
			}
		public:
			NetworkCaseCorrespondenceInfo()
			{ }
		};

		// 表示一个用于分析的网络案例。
		// 注意此处假定所有的元件均遵从向前引用的原则。
		// 尽管在内部的计算过程中，存在一个中间过程，用以将所有的网络元件重新分析并展开（这部分逻辑可能会在未来进行优化），但对外希望将这个模块作为一个基本的计算计算模块。
		// 即不要在这个模块以及相关的对象模型（例如母线、线路等）中增加更为复杂的逻辑，而是使用字典/映射（dictionary/std::map），并构造自己的函数，增加附加逻辑（有必要的话构造原型网络案例的副本），通过修改对象模型的内容来实现。
		// 关于对多个相似案例进行求解：暂时不合并相似的计算过程（例如节点编号优化等），因为这样会大大提高模块之间的耦合程度。
		// 可以考虑未来在运行时动态发现类似地计算过程，并对某些中间结果进行缓存。
		class NetworkCase
		{
			friend class NetworkObject;
		public:
			typedef std::list<NetworkObject*> NetworkObjectCollection;
		private:
			NetworkObjectCollection m_Objects;
			bool m_AutoDeleteChildren;
		private:	//internal
		public:
			const NetworkObjectCollection& Objects() const { return m_Objects; }
			//按照 Index 属性查找一个母线。
			Bus* Buses(int index) const;
			//向网络案例中加入一条母线。
			Bus* CreateBus(complexd inititalVoltage);
			//向网络案例中加入一条母线。
			Bus* CreateBus() { return CreateBus(1); };
			//指示是否应在此实例析构时自动删除子级网络对象。
			bool AutoDeleteChildren() const { return m_AutoDeleteChildren; }
			void AutoDeleteChildren(bool val) { m_AutoDeleteChildren = val; }
		private:
			static void ExpandCore(PrimitiveNetwork* enc, NetworkObject* obj);
		public:
			void AddObject(NetworkObject* obj);
			void AddObject(std::initializer_list<NetworkObject*> init);
			bool RemoveObject(NetworkObject* obj);
			void DeleteChildren();				//移除并删除此网络实例中的所有子级。
			void Validate() const;				//验证整个网络实例的有效性。
			//根据当前网络，生成一个经过初步分析的 PrimitiveNetwork。
			std::shared_ptr<PrimitiveNetwork> ToPrimitive(PrimitiveNetworkOptions options = PrimitiveNetworkOptions::NodeReorder) const;
			//构造此网络案例的一个浅层副本，包含了与此案例相同的 NetworkObject 引用。
			std::shared_ptr<NetworkCase> ShallowClone() const;
			//构造此网络案例的一个副本，并获取副本和原型中所有对象的对应关系。
			std::pair < std::shared_ptr<NetworkCase>, std::shared_ptr<NetworkCaseCorrespondenceInfo> >
				CorrespondenceClone();
			//构造此网络案例的一个副本。
			std::shared_ptr<NetworkCase> Clone()
			{
				auto info = CorrespondenceClone();
				return info.first;
			}
		public:
			NetworkCase();
			~NetworkCase();
			//不允许使用复制构造函数。
			NetworkCase(const NetworkCase&) = delete;
		};
	}
}
#endif //__POWERSOLUTIONS_POWERFLOW_NETWORKCASE_H
