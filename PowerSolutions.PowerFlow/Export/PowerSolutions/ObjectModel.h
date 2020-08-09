/*
PowerSolutions
��������ģ��
by Chen [CXuesong.], 2015
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
	namespace PowerFlow
	{
		class PrimitiveSolution;
		struct ComponentFlowSolution;
	}
	namespace ObjectModel
	{
		class NetworkCase;
		class PrimitiveNetwork;
		class NetworkCaseCorrespondenceInfo;
		class Component;
		class IBusContainer;

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
			PowerFlow::ComponentFlowSolution EvalComponentFlow(complexd voltage1, complexd voltage2) const;
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

		// Ϊ���簸���еĶ�������Ԫ����ĸ�ߣ��ṩ�������ࡣ
		// ���簸���е����ж���ͨ���ڴ��ַ��ָ�룩���������֣��������������ӱ����һ���ԡ���Ϊ����ģ���еı����ʵ��ʵ�ʼ���ʱ�ı��û��ʲô��ϵ��
		class NetworkObject
		{
#if _DEBUG
		public:
			static unsigned long _IDCounter;
			unsigned long _ID;	//һ����ʶ���������ڵ���ģʽ�����ֲ�ͬ���������
#endif
		private:
			void* _Tag;
		public:
			void* Tag() const { return _Tag; }
			void Tag(void* val) { _Tag = val; }
		public:
			virtual void Validate() const;
			//��ȡ�˶����һ��������
			NetworkObject* Clone(const NetworkCaseCorrespondenceInfo& context) const;
		protected:
			//�ڷǳ�������������д�����ڷ���һ���������������ݵ�ʵ����
			virtual NetworkObject* CloneInstance() const = 0;
			//�ڳ�������������д��������ָ���ĸ���������������ݡ�
			virtual void OnCloned(NetworkObject* newInstance, const NetworkCaseCorrespondenceInfo& context) const;
			NetworkObject();
		public:
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
			IBusContainer* m_Parent;
		public:
			IBusContainer* Parent() const { return m_Parent; }
			void Parent(IBusContainer* val) { m_Parent = val; }
			complexd InitialVoltage() const { return m_InitialVoltage; }	//����ʱĸ��ʹ�õĳ�ʼ��ѹֵ����ֵ������ֵ����ǣ����ȣ���
			void InitialVoltage(complexd val) { m_InitialVoltage = val; }
		public:
			virtual void Validate() const;
			//��ȡ�˶����һ��������
			NetworkObject* Clone(const NetworkCaseCorrespondenceInfo& context) const
			{
				return static_cast<Bus*>(NetworkObject::Clone(context));
			}
		protected:
			virtual NetworkObject* CloneInstance() const override;
		public:
			Bus();
			Bus(complexd initialVoltage);
		_PS_INTERNAL:
			Bus(IBusContainer* parent, complexd initialVoltage);
		};

		typedef std::pair<ObjectModel::Bus*, ObjectModel::Bus*> BusPair;

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
			NetworkObject* Clone(const NetworkCaseCorrespondenceInfo& context) const
			{
				return static_cast<Component*>(NetworkObject::Clone(context));
			}
			virtual void BuildNodeInfo(PrimitiveNetwork* pNetwork);
			virtual void BuildAdmittanceInfo(PrimitiveNetwork* pNetwork) {}
			/*
			��������
			Component -----> [node]   PowerInjection
			            |
						|   PowerShunt [0]
			*/
			//���ݽڵ��ѹ��ȡ��Ԫ��ע��ָ��ĸ�߽ڵ�Ĺ��ʡ�
			//index = 0 : �ӵع���
			//index > 0 : ĳ�˿ڵ�ע��ĸ�ߵĹ���
			virtual PowerFlow::ComponentFlowSolution EvalComponentFlow(const PowerFlow::PrimitiveSolution& solution) const = 0;
		protected:
			virtual void OnCloned(NetworkObject* newInstance, const NetworkCaseCorrespondenceInfo& context) const override;
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
			virtual void BuildNodeInfo(PrimitiveNetwork* pNetwork) override;
			virtual void BuildAdmittanceInfo(PrimitiveNetwork* pNetwork) override;
			virtual PowerFlow::ComponentFlowSolution EvalComponentFlow(const PowerFlow::PrimitiveSolution& solution) const override;
		public:	//�����ṹ
			int PortCount() const = delete;					//�����ṹ��
		protected:
			DoublePortComponent(Bus* bus1, Bus* bus2);
		};

		class TriPortComponent : public Component
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

		class IBusContainer abstract	//��ʾԪ���ڲ�����һ������ĸ�ߡ�
		{
		public:
			virtual int ChildBusCount() const = 0;			//��ȡԪ���ڲ�ĸ�ߵ�������
			virtual Bus* ChildBusAt(int index) const  = 0;	//����������ȡԪ���ڲ���ĸ�ߡ�������0��ʼ��
		};
	}
}

#endif	//__POWERSOLUTIONS_OBJECTMODEL_H
