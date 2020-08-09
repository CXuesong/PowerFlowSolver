//����ģ��
//�ٶ������е�Ԫ��ֻ������Ԫ�������йص���Ϣ��������ӳ������
#ifndef __POWERSOLUTIONS_POWERFLOW_OBJECTMODEL_H
#define __POWERSOLUTIONS_POWERFLOW_OBJECTMODEL_H

#include "PowerSolutions.PowerFlow.h"
#include <vector>
#include <functional>

namespace PowerSolutions {
	namespace ObjectModel
	{
		class NetworkCase;

		//���Լ��
		// �Ǹ�����Ԫ������
		// -1���հ�
		// �����������ڲ������������Զ����ɵ�Ԫ����������
		const int NullIndex = -1;//��ʾ���հס���ĸ�߱��

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

		class NetworkObject
		{
			friend class NetworkCase;
		private:
			//TODO ������Ψһ�Լ��/��ʽ���м��
			int m_Index;
			void *m_Tag;
			NetworkCase *m_CaseInfo;
		private:	//internal
			void CaseInfo(NetworkCase * val) { m_CaseInfo = val; }
		public:
			//�����ڰ�����Ψһ�ر�ʶһ�������������ע�⣺������Ψһ�Լ��/��ʽ���м�顣
			int Index() const { return m_Index; }
			void Index(int val);
			void* Tag() const { return m_Tag; }	//Ӧ�ó����Զ���ĸ�����Ϣ��
			void Tag(void * val) { m_Tag = val; }
			NetworkCase * CaseInfo() const { return m_CaseInfo; }
		public:
			virtual void Validate() const;
		protected:
			NetworkObject(int index);
			NetworkObject();
		private:
			NetworkObject(NetworkObject&) {}
		};

		class Component : public NetworkObject
		{
		public:
			virtual int BusAt(int index) const = 0;	//�����ṹ����ȡ������������ָ���˿ڴ���ĸ���������˿�������0��ʼ��
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
			int m_Bus;	
		public:
			int Bus() const { return m_Bus; }	//��Ԫ�����ӵ���ĸ��������
			void Bus(int val) { m_Bus = val; }
		private:	//�����ṹ
			virtual int BusAt(int index) const;//��ȡ������������ָ���˿ڴ���ĸ���������˿�������0��ʼ��
			virtual int PortCount() const;//��ȡ��Ԫ���˿ڵ���Ŀ��
		protected:
			SinglePortComponent(int index, int bus);
			SinglePortComponent(int index);
			SinglePortComponent();
		};

		class DoublePortComponent : public Component
		{
		private:
			int m_Bus1;	
			int m_Bus2;	
		public:
			int Bus1() const { return m_Bus1; }		//��Ԫ�����ӵ���ĸ��1������
			void Bus1(int val) { m_Bus1 = val; }
			int Bus2() const { return m_Bus2; }		//��Ԫ�����ӵ���ĸ��2������
			void Bus2(int val) { m_Bus2 = val; }
		public:
			virtual PiEquivalencyParameters PiEquivalency() = 0;//��ȡ��Ԫ�����͵�ֵ��·������
		private:		//�����ṹ
			virtual int BusAt(int index) const;//��ȡ������������ָ���˿ڴ���ĸ���������˿�������0��ʼ��
			virtual int PortCount() const;//��ȡ��Ԫ���˿ڵ���Ŀ��
		protected:
			DoublePortComponent(int index, int bus1, int bus2);
			DoublePortComponent(int index);
			DoublePortComponent();
		};

		class TriPortComponent : public Component
		{
		private:
			int m_Bus1;
			int m_Bus2;
			int m_Bus3;
		public:
			int Bus1() const { return m_Bus1; }		//��Ԫ�����ӵ���ĸ��1������
			void Bus1(int val) { m_Bus1 = val; }
			int Bus2() const { return m_Bus2; }		//��Ԫ�����ӵ���ĸ��2������
			void Bus2(int val) { m_Bus2 = val; }
			int Bus3() const { return m_Bus3; }		//��Ԫ�����ӵ���ĸ��3������
			void Bus3(int val) { m_Bus3 = val; }
		private:		//�����ṹ
			virtual int BusAt(int index) const;//��ȡ������������ָ���˿ڴ���ĸ���������˿�������0��ʼ��
			virtual int PortCount() const;//��ȡ��Ԫ���˿ڵ���Ŀ��
		protected:
			TriPortComponent(int index, int bus1, int bus2, int bus3);
			TriPortComponent(int index);
			TriPortComponent();
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
			Bus(int index, complexd initialVoltage);
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
			virtual PiEquivalencyParameters PiEquivalency();//��ȡ��Ԫ�����͵�ֵ��·������
		public:
			Line();
			Line(int index, int bus1, int bus2, complexd impedance, complexd admittance);
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
			PVGenerator(int index, int bus, double activePower, double voltage);
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
			SlackGenerator(int index, int bus, complexd voltage);
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
			PQLoad(int index, int bus, complexd power);
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
			ShuntAdmittance(int index, int bus, complexd admittance);
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
			complexd TapRatio() const { return m_TapRatio; }		//ĸ��1����ĸ��2��ķǱ�׼��ȡ�
			void TapRatio(complexd val) { m_TapRatio = val; }
		public:
			virtual void Validate() const;
			virtual PiEquivalencyParameters PiEquivalency();//��ȡ��Ԫ�����͵�ֵ��·������
		public:
			Transformer();
			Transformer(int index, int bus1, int bus2, complexd impedance, complexd admittance, complexd tapRatio);
		};

		//���зǱ�׼��ȵ��������������ѹ����
		class ThreeWindingTransformer : public TriPortComponent
		{
		};
	}
}

#endif