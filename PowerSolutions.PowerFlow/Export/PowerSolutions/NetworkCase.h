#ifndef __POWERSOLUTIONS_POWERFLOW_NETWORKCASE_H
#define __POWERSOLUTIONS_POWERFLOW_NETWORKCASE_H

#include "PowerSolutions.h"
#include "ObjectModel.h"
#include "Utility.h"
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <memory>

namespace PowerSolutions {
	namespace ObjectModel {

		// 在执行 NetworkCase::Clone 时用于将原来的网络案例（原型，Prototype）中的某些网络对象（尤其是母线）映射到新的网络案例（副本，Clone）中。
		// 在使用 NetworkCase::TrackingClone 后，用于实现原型网络案例和副本网络案例对象之间的对应。
		class NetworkCaseTrackingInfo
		{
		private:
			struct MappingInfo
			{
			public:
				bool isPrototype;				//此元件是否来自于原型网络案例。
				NetworkObject *anotherObject;	//如果 isPrototype = true，则保存了副本网络案例中对应的对象，反之亦然。
			public:
				MappingInfo(bool _isPrototype, NetworkObject* _anotherObject)
					: isPrototype(_isPrototype), anotherObject(_anotherObject)
				{ }
			};
			std::unordered_map<NetworkObject*, MappingInfo> objectMapping;
		private:	//internal
			friend class NetworkCase;
			void MapObject(NetworkObject* oldObj, NetworkObject* newObj);
		public:
			// 获取原型网络案例中指定网络对象在副本网络案例中的对应对象。
			NetworkObject* CloneOf(NetworkObject* prototypeObj) const;
			// 获取副本网络案例中指定网络对象在原型网络案例中的对应对象。
			NetworkObject* PrototypeOf(NetworkObject* cloneObj) const;
			template <class TObj>
			TObj* CloneOf(TObj* prototypeObj) const { 
				return static_cast<TObj*>(CloneOf(static_cast<NetworkObject*>(prototypeObj))); 
			}
			template <class TObj>
			TObj* PrototypeOf(TObj* cloneObj) const {
				return static_cast<TObj*>(PrototypeOf(static_cast<NetworkObject*>(cloneObj)));
			}
		private:	//internal
			NetworkCaseTrackingInfo(std::size_t objectCount)
			{
				objectMapping.reserve(objectCount * 2);
			}
		public:
			NetworkCaseTrackingInfo()
			{ }
		};

		// 节点的类型。
		enum class NodeType : byte
		{
			PQNode = 0,		//PQ节点。
			PVNode,			//PV节点。
			SlackNode,		//平衡节点。
		};

		class PrimitiveNetwork abstract
		{
		public:
			class NodeInfo
			{
				//TODO 实现参数和状态的解耦
			public:
				typedef std::list<SinglePortComponent*> SinglePortComponentCollection;
			public:
				ObjectModel::Bus* Bus;			//母线的组件信息。
				SinglePortComponentCollection Components;	//母线所连接的单端元件。
				int Index = -1;					//节点索引（Nodes）。
				int SubIndex = -1;				//节点在相应类型的节点列表（PQNodes/PVNodes）中的索引。
				int Degree = 0;					//母线连结出来的分支数量。
				NodeType Type;					//母线的类型。
				//求解变量的缓存值。
				//生成目标解相量之前，
				//对于PQ节点，保存已知的P、Q；
				//对于PV节点，保存已知的P、V。
				//迭代结束后，保存了当前的解V/A/P/Q。
				double Voltage = 0;
				double Angle = 0;					//弧度
				double ActivePowerInjection = 0;
				double ReactivePowerInjection = 0;
				//此内联函数无法放置在cpp中,因为其被 Solver 的派生类所调用，
				//可能引发不可识别的外部函数的错误。
				void ClearPowerInjections()
				{
					ActivePowerInjection = ReactivePowerInjection = 0;
				}
				complexd VoltagePhasor()		//复数形式的电压相量
				{
					return std::polar(Voltage, Angle);
				}
				NodeInfo(ObjectModel::Bus* bus, NodeType type)
					: Bus(bus), Type(type)
				{ }
				NodeInfo(ObjectModel::Bus* bus)
					: Bus(bus), Type(NodeType::PQNode)
				{ }
			};
		public:
			typedef std::list<Bus*> BusCollection;
			typedef std::list<Component*> ComponentCollection;
			typedef std::vector<NodeInfo*> NodeCollection;
			typedef std::unordered_map<ObjectModel::Bus*, std::shared_ptr<NodeInfo>> NodeDictionary;
			typedef std::unordered_set < std::pair<ObjectModel::Bus*, ObjectModel::Bus*>,
				Utility::UnorderedPairHasher<ObjectModel::Bus*>, Utility::UnorderedPairEqualityComparer < ObjectModel::Bus* >>
				BranchCollection;
			//typedef std::unordered_multimap<Bus*, Component*> BusComponentCollection;
		public:
			virtual void AddPi(Bus* bus1, Bus* bus2, PiEquivalencyParameters pi) = 0;
			virtual void AddShunt(Bus* bus, complexd admittance) = 0;
			virtual void AddPQ(Bus* bus, complexd power) = 0;
			virtual void AddPV(Bus* bus, double activePower, double voltage) = 0;
			virtual void AddSlack(Bus* bus, complexd voltagePhasor) = 0;
			virtual void ClaimParent(Bus* bus, SinglePortComponent* c) = 0;
			virtual void ClaimBranch(Bus* bus1, Bus* bus2) = 0;
		protected:
			NetworkCase* m_SourceNetwork;
			BusCollection m_Buses;
			//ComponentCollection m_Components;
			//BusComponentCollection m_BusComponents;
		public:
			NetworkCase* SourceNetwork() const { return m_SourceNetwork; }
			const BusCollection& Buses() const { return m_Buses; }
			//const ComponentCollection& Components() const { return m_Components; }
			//const BusComponentCollection& BusComponents() const { return m_BusComponents; }
			NodeCollection PQNodes;			//参与计算的母线（PQ节点）信息，按照矩阵索引排序。
			NodeCollection PVNodes;			//参与计算的母线（PV节点）信息，按照矩阵索引排序。
			//注意到在NR法中，PQ 节点和 PV 节点的顺序是可以交错的。
			NodeCollection Nodes;			//参与计算的三种节点，按照矩阵索引连续排序，注意平衡节点放在最后。
			std::shared_ptr<NodeInfo> SlackNode;	//平衡节点的信息。
			NodeDictionary BusMapping;				//Bus --> 节点信息
			BranchCollection Branches;				//记录节点连接（支路）(m,n)
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
			const NetworkObjectCollection& Objects() { return m_Objects; }
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
			static void ExpandCore(PrimitiveNetwork* enc, NetworkObject* obj);
		public:
			void AddObject(NetworkObject* obj);
			void AddObject(std::initializer_list<NetworkObject*> init);
			bool RemoveObject(NetworkObject* obj);
			void DeleteChildren();				//移除并删除此网络实例中的所有子级。
			void Validate() const;				//验证整个网络实例的有效性。
			PrimitiveNetwork* Expand() const;
			//构造此网络案例的一个副本，并获取副本和原型中所有对象的对应关系。
			NetworkCase* Clone(NetworkCaseTrackingInfo*& trackingInfo);
			//构造此网络案例的一个副本。
			NetworkCase* Clone() 
			{
				NetworkCaseTrackingInfo* infoptr;
				auto nc = Clone(infoptr);
				delete infoptr;
				return nc;
			}
			template <class TFunc>
			void ForEachDescendant(TFunc func)
			{
				for (auto& obj : m_Objects)
				{
					func(obj);
					auto cc = dynamic_cast<ComplexComponent*>(obj);
					if (cc != nullptr)
					{
						// 注意目前仅支持一层包含关系。
						for (int i = 0, j = cc->ChildrenCount(); i < j; i++)
							func(cc->ChildAt(i));
					}
				}
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
