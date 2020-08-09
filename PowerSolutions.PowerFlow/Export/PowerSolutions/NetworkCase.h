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

		// ��ִ�� NetworkCase::Clone ʱ���ڽ�ԭ�������簸����ԭ�ͣ�Prototype���е�ĳЩ�������������ĸ�ߣ�ӳ�䵽�µ����簸����������Clone���С�
		// ��ʹ�� NetworkCase::TrackingClone ������ʵ��ԭ�����簸���͸������簸������֮��Ķ�Ӧ��
		class NetworkCaseCorrespondenceInfo
		{
		private:
			struct MappingInfo
			{
			public:
				bool isPrototype;					//��Ԫ���Ƿ�������ԭ�����簸����
				const NetworkObject *anotherObject;	//��� isPrototype = true���򱣴��˸������簸���ж�Ӧ�Ķ��󣬷�֮��Ȼ��
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
			// ��ȡԭ�����簸����ָ����������ڸ������簸���еĶ�Ӧ����
			const NetworkObject* CloneOf(const NetworkObject* prototypeObj) const;
			// ��ȡ�������簸����ָ�����������ԭ�����簸���еĶ�Ӧ����
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
			const NetworkObjectCollection& Objects() const { return m_Objects; }
			//���� Index ���Բ���һ��ĸ�ߡ�
			Bus* Buses(int index) const;
			//�����簸���м���һ��ĸ�ߡ�
			Bus* CreateBus(complexd inititalVoltage);
			//�����簸���м���һ��ĸ�ߡ�
			Bus* CreateBus() { return CreateBus(1); };
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
			//���ݵ�ǰ���磬����һ���������������� PrimitiveNetwork��
			std::shared_ptr<PrimitiveNetwork> ToPrimitive(PrimitiveNetworkOptions options = PrimitiveNetworkOptions::NodeReorder) const;
			//��������簸����һ��ǳ�㸱������������˰�����ͬ�� NetworkObject ���á�
			std::shared_ptr<NetworkCase> ShallowClone() const;
			//��������簸����һ������������ȡ������ԭ�������ж���Ķ�Ӧ��ϵ��
			std::pair < std::shared_ptr<NetworkCase>, std::shared_ptr<NetworkCaseCorrespondenceInfo> >
				CorrespondenceClone();
			//��������簸����һ��������
			std::shared_ptr<NetworkCase> Clone()
			{
				auto info = CorrespondenceClone();
				return info.first;
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
