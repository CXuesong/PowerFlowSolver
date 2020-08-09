/*
PowerSolutions
��������ģ��
by  Chen [CXuesong.], 2015
*/

//Լ�������ʹ�� Attach ��Ԫ���󶨵� NetworkCase���� NetworkCase ��ӵ�жԴ�Ԫ������Դռ��Ȩ��
//ע�⣬����ζ�������Ԫ������� NetworkCase������ת�������� NetworkCase����������ᱻ�Զ� delete��

#ifndef __POWERSOLUTIONS_OBJECTMODEL_H
#define __POWERSOLUTIONS_OBJECTMODEL_H

#include "PowerSolutions.h"
#include <vector>
#include <list>
#include <functional>

namespace PowerSolutions {
	namespace ObjectModel
	{
		class NetworkCase;
		class NetworkCaseCloneContext;
		class Component;

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

		//���ڱ�ʾ����ֵ�Ļ�ֵ��
		struct PerUnitBase
		{
		private:
			double m_Voltage;
			double m_Power;
		public:
			double Voltage() const { return m_Voltage; }
			void Voltage(double val) { m_Voltage = val; }
			double Power() const { return m_Power; }
			void Power(double val) { m_Power = val; }
			double Current() const { return m_Power / 1.7320508075688772 / m_Voltage; }
			double Impedance() const { return m_Voltage * m_Voltage / m_Power; }
			double Admittance() const { return m_Power / m_Voltage / m_Voltage; }
		public:
			PerUnitBase(double voltage, double power);
		};

		//Ϊ���簸���еĶ�������Ԫ����ĸ�ߣ��ṩ�������ࡣ
		class NetworkObject
		{
			friend class NetworkCase;
			friend class ThreeWindingTransformer;
		private:
		protected:	//internal
			//���˶�������ݽ�Ҫ���ڲ������ǰ�����ô˷�����
			virtual void OnExpand() {}
		public:
			virtual void Validate() const;
			//��ȡ�˶����һ��������
			NetworkObject* Clone(const NetworkCaseCloneContext& context) const;
		protected:
			//�ڷǳ�������������д�����ڷ���һ���������������ݵ�ʵ����
			virtual NetworkObject* CloneInstance() const = 0;
			//�ڳ�������������д��������ָ���ĸ���������������ݡ�
			virtual void OnCloned(NetworkObject* newInstance, const NetworkCaseCloneContext& context) const;
			NetworkObject();
			virtual ~NetworkObject();
		private:
			//�����������캯����
			NetworkObject(NetworkObject&) = delete;
			NetworkObject& operator=(NetworkObject&) = delete;
		};

		// ĸ�ߡ�
		class Bus final : public NetworkObject
		{
			friend class Component;
		private:
			complexd m_InitialVoltage;
		private:	//Internal
			void AttachComponent(Component* c);
			void DetachComponent(Component* c);
		public:
			complexd InitialVoltage() const { return m_InitialVoltage; }	//����ʱĸ��ʹ�õĳ�ʼ��ѹֵ����ֵ������ֵ����ǣ����ȣ���
			void InitialVoltage(complexd val) { m_InitialVoltage = val; }
		public:
			virtual void Validate() const;
			//��ȡ�˶����һ��������
			NetworkObject* Clone(const NetworkCaseCloneContext& context) const
			{
				return static_cast<Bus*>(NetworkObject::Clone(context));
			}
		protected:
			virtual NetworkObject* CloneInstance() const override;
		public:
			Bus();
			Bus(complexd initialVoltage);
		};

		//��ʾ��һ������ĸ�������ӵ�Ԫ����
		class Component : public NetworkObject
		{
			friend class NetworkCase;
		private:
			std::vector<Bus*> m_Buses;
		public:
			//��ȡ������ָ���˿ڴ���ĸ���������˿�������0��ʼ��
			Bus* Buses(int index) const { return m_Buses[index]; }
			//����������ָ���˿ڴ���ĸ���������˿�������0��ʼ��
			void Buses(int index, Bus* value) { m_Buses[index] = value; }
			//��ȡ��Ԫ���˿ڵ���Ŀ��
			int PortCount() const { return m_Buses.size(); }
			//��ȡ�˶����һ��������
			NetworkObject* Clone(const NetworkCaseCloneContext& context) const
			{
				return static_cast<Component*>(NetworkObject::Clone(context));
			}
			/*
			��������
			Component -----> [node]   PowerInjection
			            |
						|   PowerShunt
			*/
			//���ݽڵ��ѹ��ȡ��Ԫ��ע��ָ��ĸ�߽ڵ�Ĺ��ʡ�
			//virtual complexd EvalPowerInjection(int busIndex, std::vector<complexd>& busVoltage);
			//���ݽڵ��ѹ��ȡ��Ԫ����ָ���˿ڴ�ע��صĹ��ʡ�
			//virtual complexd EvalPowerShunt(int busIndex, std::vector<complexd>& busVoltage);
		protected:
			virtual void OnCloned(NetworkObject* newInstance, const NetworkCaseCloneContext& context) const override;
		protected:
			Component(int portCount);
		};

		class SinglePortComponent : public Component
		{
		public:
			Bus* Bus1() const { return Buses(0); }	//��Ԫ�����ӵ���ĸ�ߡ�
			void Bus1(Bus* val) { Buses(0, val); }
		public:	//�����ṹ
			int PortCount() const = delete;					//�����ṹ��
		protected:
			SinglePortComponent(Bus* bus1);
		};

		class DoublePortComponent : public Component
		{
		public:
			Bus* Bus1() const { return Buses(0); }		//��Ԫ�����ӵ���ĸ��1��
			void Bus1(Bus* val) { Buses(0, val); }
			Bus* Bus2() const { return Buses(1); }		//��Ԫ�����ӵ���ĸ��2��
			void Bus2(Bus* val) { Buses(1, val); }
		public:
			virtual PiEquivalencyParameters PiEquivalency() const = 0;//��ȡ��Ԫ�����͵�ֵ��·������
		public:	//�����ṹ
			int PortCount() const = delete;					//�����ṹ��
		protected:
			DoublePortComponent(Bus* bus1, Bus* bus2);
		};

		//��ʾһ����ĸ�ߺ�Ԫ����ɵĸ���Ԫ����
		class ComplexComponent : public Component
		{
		public:
			//��ȡ�������Ӽ���������
			virtual int ChildrenCount() const = 0;
			//��������ȡ�����е�ĳ���Ӽ���������0��ʼ��
			virtual NetworkObject* ChildAt(int index) const = 0;
		protected:
			ComplexComponent(int portCount);
		};


		class TriPortComponent : public ComplexComponent
		{
		public:
			Bus* Bus1() const { return Buses(0); }		//��Ԫ�����ӵ���ĸ��1��
			void Bus1(Bus* val) { Buses(0, val); }
			Bus* Bus2() const { return Buses(1); }		//��Ԫ�����ӵ���ĸ��2��
			void Bus2(Bus* val) { Buses(1, val); }
			Bus* Bus3() const { return Buses(2); }		//��Ԫ�����ӵ���ĸ��3��
			void Bus3(Bus* val) { Buses(2, val); }
		public:	//�����ṹ
			int PortCount() const = delete;					//�����ṹ��
		protected:
			TriPortComponent(Bus* bus1, Bus* bus2, Bus* bus3);
		};
	}
}

#endif	//__POWERSOLUTIONS_OBJECTMODEL_H
