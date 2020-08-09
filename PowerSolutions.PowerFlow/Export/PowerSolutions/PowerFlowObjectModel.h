//����ģ��
//Լ�������е�Ԫ��ֻ������Ԫ�������йص���Ϣ��������ӳ������
//Լ�����ܲ�ʹ�ö�̬����ɵĹ��ܣ��Ͳ�ʹ�ö�̬����ɡ�
//TODO �����Ƿ��Ƴ� Tag������ͨ��CLR����������CLR��װ��ͬʱ��CLR��װ��ʹ�� shared_ptr��
//TODO �����Ƿ��Ƴ� Bus �� Index ���ԣ������Ƴ������糱��������������Ҫ�����ԣ���Լ��������һЩ��

#ifndef __POWERSOLUTIONS_POWERFLOW_OBJECTMODEL_H
#define __POWERSOLUTIONS_POWERFLOW_OBJECTMODEL_H

#include "PowerSolutions.h"
#include "ObjectModel.h"
#include <vector>
#include <list>
#include <functional>
#include <memory>

namespace PowerSolutions {
	namespace ObjectModel
	{
		// ���ε�Ч�����ߡ�
		class Line final : public DoublePortComponent
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
			virtual PiEquivalencyParameters PiEquivalency() const;	//��ȡ��Ԫ�����͵�ֵ��·������
		protected:
			virtual void BuildAdmittanceInfo(PrimitiveNetwork* pNetwork) const override;
			virtual NetworkObject* CloneInstance() const override;
		public:
			//����һ�������ߡ�
			static Line* Create(Bus *bus1, Bus *bus2, complexd impedance, complexd admittance);
		public:
			Line();
			Line(complexd impedance, complexd admittance);
			Line(Bus *bus1, Bus *bus2, complexd impedance, complexd admittance);
		};

		// PV �������
		class PVGenerator final : public SinglePortComponent
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
		protected:
			virtual NetworkObject* CloneInstance() const override;
		public:
			//����һ̨PV�������
			static PVGenerator* Create(Bus *bus1, double activePower, double voltage);
		public:
			PVGenerator();
			PVGenerator(double activePower, double voltage);
			PVGenerator(Bus *bus1, double activePower, double voltage);
		};

		// ƽ��ڵ㷢�����
		class SlackGenerator final : public SinglePortComponent
		{
		private:
			complexd m_Voltage;
		public:
			complexd Voltage() const { return m_Voltage; }		//������Ļ��˵�ѹ����ֵ������ֵ����ǣ����ȣ���
			void Voltage(complexd val) { m_Voltage = val; }
		public:
			virtual void Validate() const;
		protected:
			virtual NetworkObject* CloneInstance() const override;
		public:
			//����һ̨ƽ�ⷢ�����
			static SlackGenerator* Create(Bus *bus1, complexd voltage);
		public:
			SlackGenerator();
			SlackGenerator(complexd voltage);
			SlackGenerator(Bus *bus1, complexd voltage);
		};

		// PQ ���ء�
		class PQLoad final : public SinglePortComponent
		{
		private:
			complexd m_Power;
		public:
			complexd Power() const { return m_Power; }		//ע�븺�����յĹ��ʴ�С������ֵ����
			void Power(complexd val) { m_Power = val; }
		public:
			virtual void Validate() const;
		protected:
			virtual NetworkObject* CloneInstance() const override;
		public:
			//����һ��PQ���ء�
			static PQLoad* Create(Bus *bus1, complexd power);
		public:
			PQLoad();
			PQLoad(complexd power);
			PQLoad(Bus *bus1, complexd power);
		};

		//�����ӵص��ɡ�
		class ShuntAdmittance final : public SinglePortComponent
		{
		private:
			complexd m_Admittance;
		public:
			complexd Admittance() const { return m_Admittance; }	//���ɵı���ֵ��
			void Admittance(complexd val) { m_Admittance = val; }
		public:
			virtual void Validate() const;
		protected:
			virtual NetworkObject* CloneInstance() const override;
		public:
			//����һ�������ӵص��ɡ�
			static ShuntAdmittance* Create(Bus *bus1, complexd admittance);
		public:
			ShuntAdmittance();
			ShuntAdmittance(complexd admittance);
			ShuntAdmittance(Bus *bus1, complexd admittance);
		};

		// ���зǱ�׼��ȵġ��ɼ�������ı�ѹ����
		class Transformer final : public DoublePortComponent
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
		protected:
			virtual NetworkObject* CloneInstance() const override;
		public:
			//����һ����ѹ����
			static Transformer* Create(Bus *bus1, Bus *bus2, complexd impedance, complexd admittance, complexd tapRatio);
			//����һ����������ı�ѹ����
			static Transformer* Create(Bus *bus1, Bus *bus2, complexd impedance, complexd tapRatio)
			{
				return Create(bus1, bus2, impedance, 0, tapRatio);
			}
		public:
			Transformer();
			Transformer(complexd impedance, complexd admittance, complexd tapRatio);
			Transformer(Bus *bus1, Bus *bus2, complexd impedance, complexd tapRatio);
			Transformer(Bus *bus1, Bus *bus2, complexd impedance, complexd admittance, complexd tapRatio);
		};

		//���зǱ�׼��ȵ��������������ѹ����
		class ThreeWindingTransformer final : public TriPortComponent
		{
		private:
			std::unique_ptr<Bus> m_CommonBus;
			std::unique_ptr<Transformer> m_Transformer1, m_Transformer2, m_Transformer3;
			complexd m_Impedance12, m_Impedance13, m_Impedance23, m_Admittance;
			complexd m_TapRatio1, m_TapRatio2, m_TapRatio3;
		private:
			//�����Ӽ��Ĳ�����
			void UpdateChildren();
		public:
			//�˱�ѹ��ʹ�õ�һ�β��ڲ�����ĸ�ߡ�
			Bus* CommonBus() const { return m_CommonBus.get(); }
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
			complexd Impedance1() const { return (m_Impedance12 + m_Impedance13 - m_Impedance23) / 2.0; }
			complexd Impedance2() const { return (m_Impedance12 + m_Impedance23 - m_Impedance13) / 2.0; }
			complexd Impedance3() const { return (m_Impedance13 + m_Impedance23 - m_Impedance12) / 2.0; }
		public:
			virtual void Validate() const;
		private:	//�����ṹ��
			virtual int ChildrenCount() const override;
			virtual NetworkObject* ChildAt(int index) const override;
		protected:
			virtual NetworkObject* CloneInstance() const override;
			virtual void OnExpand();
		public:
			//����һ���������ѹ����
			static ThreeWindingTransformer* Create(Bus *bus1, Bus *bus2, Bus *bus3,
				complexd impedance12, complexd impedance13, complexd impedance23,
				complexd admittance, complexd tapRatio1, complexd tapRatio2, complexd tapRatio3);
		public:
			ThreeWindingTransformer();
			ThreeWindingTransformer(complexd impedance12, complexd impedance13, complexd impedance23,
				complexd admittance, complexd tapRatio1, complexd tapRatio2, complexd tapRatio3);
			ThreeWindingTransformer(Bus *bus1, Bus *bus2, Bus *bus3,
				complexd impedance12, complexd impedance13, complexd impedance23,
				complexd admittance, complexd tapRatio1, complexd tapRatio2, complexd tapRatio3);
		};
	}
}
#endif

