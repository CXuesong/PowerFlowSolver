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

		// ��ִ�� NetworkCase::Clone ʱ���ڽ�ԭ�������簸����ԭ�ͣ�Prototype���е�ĳЩ�������������ĸ�ߣ�ӳ�䵽�µ����簸����������Clone���С�
		// ��ʹ�� NetworkCase::TrackingClone ������ʵ��ԭ�����簸���͸������簸������֮��Ķ�Ӧ��
		class NetworkCaseTrackingInfo
		{
		private:
			struct MappingInfo
			{
			public:
				bool isPrototype;				//��Ԫ���Ƿ�������ԭ�����簸����
				NetworkObject *anotherObject;	//��� isPrototype = true���򱣴��˸������簸���ж�Ӧ�Ķ��󣬷�֮��Ȼ��
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
			// ��ȡԭ�����簸����ָ����������ڸ������簸���еĶ�Ӧ����
			NetworkObject* CloneOf(NetworkObject* prototypeObj) const;
			// ��ȡ�������簸����ָ�����������ԭ�����簸���еĶ�Ӧ����
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

		// �ڵ�����͡�
		enum class NodeType : byte
		{
			PQNode = 0,		//PQ�ڵ㡣
			PVNode,			//PV�ڵ㡣
			SlackNode,		//ƽ��ڵ㡣
		};

		class PrimitiveNetwork abstract
		{
		public:
			class NodeInfo
			{
				//TODO ʵ�ֲ�����״̬�Ľ���
			public:
				typedef std::list<SinglePortComponent*> SinglePortComponentCollection;
			public:
				ObjectModel::Bus* Bus;			//ĸ�ߵ������Ϣ��
				SinglePortComponentCollection Components;	//ĸ�������ӵĵ���Ԫ����
				int Index = -1;					//�ڵ�������Nodes����
				int SubIndex = -1;				//�ڵ�����Ӧ���͵Ľڵ��б�PQNodes/PVNodes���е�������
				int Degree = 0;					//ĸ����������ķ�֧������
				NodeType Type;					//ĸ�ߵ����͡�
				//�������Ļ���ֵ��
				//����Ŀ�������֮ǰ��
				//����PQ�ڵ㣬������֪��P��Q��
				//����PV�ڵ㣬������֪��P��V��
				//���������󣬱����˵�ǰ�Ľ�V/A/P/Q��
				double Voltage = 0;
				double Angle = 0;					//����
				double ActivePowerInjection = 0;
				double ReactivePowerInjection = 0;
				//�����������޷�������cpp��,��Ϊ�䱻 Solver �������������ã�
				//������������ʶ����ⲿ�����Ĵ���
				void ClearPowerInjections()
				{
					ActivePowerInjection = ReactivePowerInjection = 0;
				}
				complexd VoltagePhasor()		//������ʽ�ĵ�ѹ����
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
			NodeCollection PQNodes;			//��������ĸ�ߣ�PQ�ڵ㣩��Ϣ�����վ�����������
			NodeCollection PVNodes;			//��������ĸ�ߣ�PV�ڵ㣩��Ϣ�����վ�����������
			//ע�⵽��NR���У�PQ �ڵ�� PV �ڵ��˳���ǿ��Խ���ġ�
			NodeCollection Nodes;			//�����������ֽڵ㣬���վ���������������ע��ƽ��ڵ�������
			std::shared_ptr<NodeInfo> SlackNode;	//ƽ��ڵ����Ϣ��
			NodeDictionary BusMapping;				//Bus --> �ڵ���Ϣ
			BranchCollection Branches;				//��¼�ڵ����ӣ�֧·��(m,n)
		};

		// ��ʾһ�����ڷ��������簸����
		// ע��˴��ٶ����е�Ԫ���������ǰ���õ�ԭ��
		// �������ڲ��ļ�������У�����һ���м���̣����Խ����е�����Ԫ�����·�����չ�����ⲿ���߼����ܻ���δ�������Ż�����������ϣ�������ģ����Ϊһ�������ļ������ģ�顣
		// ����Ҫ�����ģ���Լ���صĶ���ģ�ͣ�����ĸ�ߡ���·�ȣ������Ӹ�Ϊ���ӵ��߼�������ʹ���ֵ�/ӳ�䣨dictionary/std::map�����������Լ��ĺ��������Ӹ����߼����б�Ҫ�Ļ�����ԭ�����簸���ĸ�������ͨ���޸Ķ���ģ�͵�������ʵ�֡�
		// ���ڶԶ�����ư���������⣺��ʱ���ϲ����Ƶļ�����̣�����ڵ����Ż��ȣ�����Ϊ�����������ģ��֮�����ϳ̶ȡ�
		// ���Կ���δ��������ʱ��̬�������Ƶؼ�����̣�����ĳЩ�м������л��档
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
			static void ExpandCore(PrimitiveNetwork* enc, NetworkObject* obj);
		public:
			void AddObject(NetworkObject* obj);
			void AddObject(std::initializer_list<NetworkObject*> init);
			bool RemoveObject(NetworkObject* obj);
			void DeleteChildren();				//�Ƴ���ɾ��������ʵ���е������Ӽ���
			void Validate() const;				//��֤��������ʵ������Ч�ԡ�
			PrimitiveNetwork* Expand() const;
			//��������簸����һ������������ȡ������ԭ�������ж���Ķ�Ӧ��ϵ��
			NetworkCase* Clone(NetworkCaseTrackingInfo*& trackingInfo);
			//��������簸����һ��������
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
						// ע��Ŀǰ��֧��һ�������ϵ��
						for (int i = 0, j = cc->ChildrenCount(); i < j; i++)
							func(cc->ChildAt(i));
					}
				}
			}
		public:
			NetworkCase();
			~NetworkCase();
			//������ʹ�ø��ƹ��캯����
			NetworkCase(const NetworkCase&) = delete;
		};
	}
}
#endif //__POWERSOLUTIONS_POWERFLOW_NETWORKCASE_H
