//����ģ��
//Լ�������е�Ԫ��ֻ������Ԫ�������йص���Ϣ��������ӳ������
//Լ�����ܲ�ʹ�ö�̬����ɵĹ��ܣ��Ͳ�ʹ�ö�̬����ɡ�
//TODO �����Ƿ��Ƴ� Tag������ͨ��CLR����������CLR��װ
//TODO �����Ƿ��Ƴ� Bus �� Index ���ԣ������Ƴ������糱��������������Ҫ�����ԣ���Լ��������һЩ��

#ifndef __POWERSOLUTIONS_POWERFLOW_OBJECTMODEL_H
#define __POWERSOLUTIONS_POWERFLOW_OBJECTMODEL_H

#include "PowerSolutions.PowerFlow.h"
#include <vector>
#include <functional>
#include <memory>

namespace PowerSolutions {
	namespace ObjectModel
	{
		class NetworkCase;

		//���Լ��
		// �Ǹ�����Ԫ������
		// -1���հ�
		// �����������ڲ������������Զ����ɵ�Ԫ����������
		const int NullIndex = -1;	//��ʾ���հס���ĸ�߱��

		//���ڱ�ʾһ�����͵�ֵ������
		struct PiEquivalencyParameters
		{
		public:
			complexd m_Impedance;
			complexd m_Admittance1;
			complexd m_Admittance2;
		public:
			complexd Impedance() const { return m_Impedance; }	//�����迹��
			void Impedance(complexd val) { m_Impedance = val; }
			complexd Admittance1() const { return m_Admittance1; }	//�ڵ�1�ӵص��ɡ�
			void Admittance1(complexd val) { m_Admittance1 = val; }
			complexd Admittance2() const { return m_Admittance2; }	//�ڵ�2�ӵص��ɡ�
			void Admittance2(complexd val) { m_Admittance2 = val; }
		public:
			PiEquivalencyParameters(complexd z, complexd y1, complexd y2);
		};
		
		////�ṩһ�����࣬����Ϊ NetworkObject �ṩ�������ݡ�
		//class Tag
		//{
		//protected:
		//	Tag();
		//public:
		//	virtual ~Tag();
		//};

		class NetworkObject
		{
			friend class NetworkCase;
			friend class ThreeWindingTransformer;
		private:
			//TODO ������Ψһ�Լ�飬�����ṩ��ʽ���м�鹦��
			int m_Index;
			//unique_ptr<Tag> m_Tag;
			NetworkCase* m_CaseInfo;
			NetworkObject* m_Parent;
		private:	//internal
			void CaseInfo(NetworkCase* val) { m_CaseInfo = val; }
			void Parent(NetworkObject* val) { m_Parent = val; }
		public:
			//�����ڰ�����Ψһ�ر�ʶһ�������������ע�⣺������Ψһ�Լ��/��ʽ���м�顣
			int Index() const { return m_Index; }
			void Index(int val);
			//��ȡӦ�ó����Զ���ĸ�����Ϣ��
			//::Tag* Tag() const { return m_Tag.get(); }
			//����Ӧ�ó����Զ���ĸ�����Ϣ��
			//ע�⣬��Ҫ��ͬһ Tag ָ�����ø����� NetworkObject ʹ�á�
			//void Tag(::Tag* val) { m_Tag.reset(val); }

			//��ȡ�˶��������ڵİ�����
			NetworkCase* CaseInfo() const { return m_CaseInfo; }
			//��ȡ�˶����������ڵĸ�����
			NetworkObject* Parent() const { return m_Parent; }
		public:
			virtual void Validate() const;
		protected:
			NetworkObject(int index);
			NetworkObject();
		private:
			//�����������캯����
			NetworkObject(NetworkObject&) = delete;
		};

		// ĸ�ߡ�
		class Bus : public NetworkObject
		{
		private:
			complexd m_InitialVoltage;
		public:
			complexd InitialVoltage() const { return m_InitialVoltage; }	//����ʱĸ��ʹ�õĳ�ʼ��ѹֵ����ֵ������ֵ����ǣ����ȣ���
			void InitialVoltage(complexd val) { m_InitialVoltage = val; }
		public:
			virtual void Validate() const;
		public:
			Bus();
			Bus(int index);
			Bus(int index, complexd initialVoltage);
		};

		class Component : public NetworkObject
		{
		public:
			virtual ::Bus* BusAt(int index) const = 0;	//�����ṹ����ȡ������������ָ���˿ڴ���ĸ���������˿�������0��ʼ��
			virtual int PortCount() const = 0;		//��ȡ��Ԫ���˿ڵ���Ŀ��
		protected:
			//���ָ����ĸ�������Ƿ���ȷ��
			//void CheckBusIndex(int index, const TCHAR *argumentName, bool allowNull = false) const;
		protected:
			Component(int index);
			Component();
		};

		class SinglePortComponent : public Component
		{
		private:
			::Bus* m_Bus;	
		public:
			::Bus* Bus() const { return m_Bus; }	//��Ԫ�����ӵ���ĸ��������
			void Bus(::Bus* val) { m_Bus = val; }
		private:	//�����ṹ
			virtual ::Bus* BusAt(int index) const override;	//��ȡ������������ָ���˿ڴ���ĸ���������˿�������0��ʼ��
			virtual int PortCount() const override;		//��ȡ��Ԫ���˿ڵ���Ŀ��
		protected:
			SinglePortComponent(int index, ::Bus* bus);
			SinglePortComponent(int index);
			SinglePortComponent();
		};

		class DoublePortComponent : public Component
		{
		private:
			::Bus* m_Bus1;	
			::Bus* m_Bus2;	
		public:
			::Bus* Bus1() const { return m_Bus1; }		//��Ԫ�����ӵ���ĸ��1������
			void Bus1(::Bus* val) { m_Bus1 = val; }
			::Bus* Bus2() const { return m_Bus2; }		//��Ԫ�����ӵ���ĸ��2������
			void Bus2(::Bus* val) { m_Bus2 = val; }
		public:
			virtual PiEquivalencyParameters PiEquivalency() const = 0;//��ȡ��Ԫ�����͵�ֵ��·������
		private:		//�����ṹ
			virtual ::Bus* BusAt(int index) const override;//��ȡ������������ָ���˿ڴ���ĸ���������˿�������0��ʼ��
			virtual int PortCount() const override;//��ȡ��Ԫ���˿ڵ���Ŀ��
		protected:
			DoublePortComponent(int index, ::Bus* bus1, ::Bus* bus2);
			DoublePortComponent(int index);
			DoublePortComponent();
		};

		class TriPortComponent : public Component
		{
		private:
			::Bus* m_Bus1;
			::Bus* m_Bus2;
			::Bus* m_Bus3;
		public:
			::Bus* Bus1() const { return m_Bus1; }		//��Ԫ�����ӵ���ĸ��1������
			void Bus1(::Bus* val) { m_Bus1 = val; }
			::Bus* Bus2() const { return m_Bus2; }		//��Ԫ�����ӵ���ĸ��2������
			void Bus2(::Bus* val) { m_Bus2 = val; }
			::Bus* Bus3() const { return m_Bus3; }		//��Ԫ�����ӵ���ĸ��3������
			void Bus3(::Bus* val) { m_Bus3 = val; }
		private:		//�����ṹ
			virtual ::Bus* BusAt(int index) const override;//��ȡ������������ָ���˿ڴ���ĸ���������˿�������0��ʼ��
			virtual int PortCount() const override;//��ȡ��Ԫ���˿ڵ���Ŀ��
		protected:
			TriPortComponent(int index, ::Bus* bus1, ::Bus* bus2, ::Bus* bus3);
			TriPortComponent(int index);
			TriPortComponent();
		};

		//�ṩ�˰����������������������ܡ�
		class IContainer
		{
		public:
			//��ȡ�������Ӽ���������
			virtual int ChildrenCount() const = 0;
			//��������ȡ�����е�ĳ���Ӽ���������0��ʼ��
			virtual NetworkObject* ChildAt(int index) const = 0;
		};

		// ���ε�Ч�����ߡ�
		class Line : public DoublePortComponent
		{
		private:
			complexd m_Impedance;
			complexd m_Admittance;
		public:
			complexd Impedance() const { return m_Impedance; }		//�迹�ı���ֵ��
			void Impedance(complexd val) { m_Impedance = val; }
			complexd Admittance() const { return m_Admittance; }	//���ɵı���ֵ��
			void Admittance(complexd val) { m_Admittance = val; }
		public:
			virtual void Validate() const;
			virtual PiEquivalencyParameters PiEquivalency() const;//��ȡ��Ԫ�����͵�ֵ��·������
		public:
			Line();
			Line(::Bus* bus1, ::Bus* bus2, complexd impedance, complexd admittance);
		};

		// PV �������
		class PVGenerator : public SinglePortComponent
		{
		private:
			double m_ActivePower;
			double m_Voltage;
		public:
			double ActivePower() const { return m_ActivePower; }	//����������й����ʵĴ�С������ֵ����
			void ActivePower(double val) { m_ActivePower = val; }
			double Voltage() const { return m_Voltage; }			//������Ļ��˵�ѹ������ֵ����
			void Voltage(double val) { m_Voltage = val; }
		public:
			virtual void Validate() const;
		public:
			PVGenerator();
			PVGenerator(::Bus* bus, double activePower, double voltage);
		};

		// ƽ��ڵ㷢�����
		class SlackGenerator : public SinglePortComponent
		{
		private:
			complexd m_Voltage;
		public:
			complexd Voltage() const { return m_Voltage; }		//������Ļ��˵�ѹ����ֵ������ֵ����ǣ����ȣ���
			void Voltage(complexd val) { m_Voltage = val; }
		public:
			virtual void Validate() const;
		public:
			SlackGenerator();
			SlackGenerator(int index, ::Bus* bus, complexd voltage);
		};

		// PQ ���ء�
		class PQLoad : public SinglePortComponent
		{
		private:
			complexd m_Power;
		public:
			complexd Power() const { return m_Power; }		//ע�븺�����յĹ��ʴ�С������ֵ����
			void Power(complexd val) { m_Power = val; }
		public:
			virtual void Validate() const;
		public:
			PQLoad();
			PQLoad(::Bus* bus, complexd power);
		};

		//�����ӵظ��ء�
		class ShuntAdmittance : public SinglePortComponent
		{
		private:
			complexd m_Admittance;
		public:
			complexd Admittance() const { return m_Admittance; }	//���ɵı���ֵ��
			void Admittance(complexd val) { m_Admittance = val; }
		public:
			virtual void Validate() const;
		public:
			ShuntAdmittance();
			ShuntAdmittance(int index, ::Bus* bus, complexd admittance);
		};

		// ���зǱ�׼��ȵġ��ɼ�������ı�ѹ����
		class Transformer : public DoublePortComponent
		{
		private:
			complexd m_Impedance;
			complexd m_Admittance;
			complexd m_TapRatio;
		public:
			complexd Impedance() const { return m_Impedance; }		//ĸ��1�മ���迹�ı���ֵ��
			void Impedance(complexd val) { m_Impedance = val; }
			complexd Admittance() const { return m_Admittance; }	//ĸ��1�ಢ���ӵص��ɵı���ֵ��ע�����ŵ���Ӧ��Ϊ��ֵ��
			void Admittance(complexd val) { m_Admittance = val; }
			complexd TapRatio() const { return m_TapRatio; }		//ĸ��1����ĸ��2��ķǱ�׼��ȣ��Լ���λ�ƶ���
			void TapRatio(complexd val) { m_TapRatio = val; }
		public:
			virtual void Validate() const;
			virtual PiEquivalencyParameters PiEquivalency() const override;//��ȡ��Ԫ�����͵�ֵ��·������
		public:
			Transformer();
			Transformer(::Bus* bus1, ::Bus* bus2, complexd impedance, complexd admittance, complexd tapRatio);
		};

		//���зǱ�׼��ȵ��������������ѹ����
		class ThreeWindingTransformer : public TriPortComponent, public IContainer
		{
		private:
			std::shared_ptr<Bus> m_CommonBus;
			std::shared_ptr<Transformer> m_Transformer1, m_Transformer2, m_Transformer3;
			complexd m_Impedance12, m_Impedance13, m_Impedance23, m_Admittance;
			complexd m_TapRatio1, m_TapRatio2, m_TapRatio3;
		public:
			//��ȡ�˱�ѹ��ʹ�õ�һ�β��ڲ�����ĸ�ߡ�
			Bus* CommonBus() const { return m_CommonBus.get(); }
			//��ȡ�˱�ѹ��ʹ�õ�һ�β��ڲ���ѹ����
			Transformer* Transformer1() const { return m_Transformer1.get(); }
			//��ȡ�˱�ѹ��ʹ�õĶ��β��ڲ���ѹ����
			Transformer* Transformer2() const { return m_Transformer2.get(); }
			//��ȡ�˱�ѹ��ʹ�õ����β��ڲ���ѹ����
			Transformer* Transformer3() const { return m_Transformer3.get(); }
			complexd Impedance12() const { return m_Impedance12; }
			void Impedance12(complexd val) { m_Impedance12 = val; }
			complexd Impedance13() const { return m_Impedance13; }
			void Impedance13(complexd val) { m_Impedance13 = val; }
			complexd Impedance23() const { return m_Impedance23; }
			void Impedance23(complexd val) { m_Impedance23 = val; }
			complexd Admittance() const { return m_Admittance; }
			void Admittance(complexd val) { m_Admittance = val; }
			complexd TapRatio1() const { return m_TapRatio1; }
			void TapRatio1(complexd val) { m_TapRatio1 = val; }
			complexd TapRatio2() const { return m_TapRatio2; }
			void TapRatio2(complexd val) { m_TapRatio2 = val; }
			complexd TapRatio3() const { return m_TapRatio3; }
			void TapRatio3(complexd val) { m_TapRatio3 = val; }
		private:	//�����ṹ��
			virtual int ChildrenCount() const override { return 4; }
			virtual NetworkObject* ChildAt(int index) const override;
		public:
			ThreeWindingTransformer();
			ThreeWindingTransformer(::Bus* bus1, ::Bus* bus2, ::Bus* bus3,
				complexd impedance12, complexd impedance13, complexd impedance23,
				complexd admittance, complexd tapRatio1, complexd tapRatio2, complexd tapRatio3);
		};
	}
}

#endif