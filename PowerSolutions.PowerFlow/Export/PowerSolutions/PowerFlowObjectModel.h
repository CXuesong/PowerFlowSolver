//网络模型
//约定：所有的元件只保存与元件本身有关的信息，而不反映父级。
//约定：能不使用多态性完成的功能，就不使用多态性完成。
//TODO 考虑是否移除 Tag。可以通过CLR集合来控制CLR封装，同时在CLR封装中使用 shared_ptr。
//TODO 考虑是否移除 Bus 的 Index 属性（不能移除，例如潮流分析报告中需要此属性，但约束可以弱一些）

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
		// π形等效传输线。
		class Line final : public DoublePortComponent
		{
		private:
			complexd m_Impedance;
			complexd m_Admittance;
		public:
			complexd Impedance() const { return m_Impedance; }		//阻抗的标幺值。
			void Impedance(complexd val) { m_Impedance = val; }
			complexd Admittance() const { return m_Admittance; }	//导纳的标幺值。
			void Admittance(complexd val) { m_Admittance = val; }
		public:
			virtual void Validate() const;
			virtual PiEquivalencyParameters PiEquivalency() const;	//获取此元件π型等值电路参数。
		protected:
			virtual void BuildAdmittanceInfo(PrimitiveNetwork* pNetwork) const override;
			virtual NetworkObject* CloneInstance() const override;
		public:
			//创建一条传输线。
			static Line* Create(Bus *bus1, Bus *bus2, complexd impedance, complexd admittance);
		public:
			Line();
			Line(complexd impedance, complexd admittance);
			Line(Bus *bus1, Bus *bus2, complexd impedance, complexd admittance);
		};

		// PV 发电机。
		class PVGenerator final : public SinglePortComponent
		{
		private:
			double m_ActivePower;
			double m_Voltage;
		public:
			double ActivePower() const { return m_ActivePower; }	//发电机发出有功功率的大小（标幺值）。
			void ActivePower(double val) { m_ActivePower = val; }
			double Voltage() const { return m_Voltage; }			//发电机的机端电压（标幺值）。
			void Voltage(double val) { m_Voltage = val; }
		public:
			virtual void Validate() const;
		protected:
			virtual NetworkObject* CloneInstance() const override;
		public:
			//创建一台PV发电机。
			static PVGenerator* Create(Bus *bus1, double activePower, double voltage);
		public:
			PVGenerator();
			PVGenerator(double activePower, double voltage);
			PVGenerator(Bus *bus1, double activePower, double voltage);
		};

		// 平衡节点发电机。
		class SlackGenerator final : public SinglePortComponent
		{
		private:
			complexd m_Voltage;
		public:
			complexd Voltage() const { return m_Voltage; }		//发电机的机端电压（幅值：标幺值，相角：弧度）。
			void Voltage(complexd val) { m_Voltage = val; }
		public:
			virtual void Validate() const;
		protected:
			virtual NetworkObject* CloneInstance() const override;
		public:
			//创建一台平衡发电机。
			static SlackGenerator* Create(Bus *bus1, complexd voltage);
		public:
			SlackGenerator();
			SlackGenerator(complexd voltage);
			SlackGenerator(Bus *bus1, complexd voltage);
		};

		// PQ 负载。
		class PQLoad final : public SinglePortComponent
		{
		private:
			complexd m_Power;
		public:
			complexd Power() const { return m_Power; }		//注入负载吸收的功率大小（标幺值）。
			void Power(complexd val) { m_Power = val; }
		public:
			virtual void Validate() const;
		protected:
			virtual NetworkObject* CloneInstance() const override;
		public:
			//创建一个PQ负载。
			static PQLoad* Create(Bus *bus1, complexd power);
		public:
			PQLoad();
			PQLoad(complexd power);
			PQLoad(Bus *bus1, complexd power);
		};

		//并联接地导纳。
		class ShuntAdmittance final : public SinglePortComponent
		{
		private:
			complexd m_Admittance;
		public:
			complexd Admittance() const { return m_Admittance; }	//导纳的标幺值。
			void Admittance(complexd val) { m_Admittance = val; }
		public:
			virtual void Validate() const;
		protected:
			virtual NetworkObject* CloneInstance() const override;
		public:
			//创建一个并联接地导纳。
			static ShuntAdmittance* Create(Bus *bus1, complexd admittance);
		public:
			ShuntAdmittance();
			ShuntAdmittance(complexd admittance);
			ShuntAdmittance(Bus *bus1, complexd admittance);
		};

		// 含有非标准变比的、可计入铁损的变压器。
		class Transformer final : public DoublePortComponent
		{
		private:
			complexd m_Impedance;
			complexd m_Admittance;
			complexd m_TapRatio;
		public:
			complexd Impedance() const { return m_Impedance; }		//母线1侧串联阻抗的标幺值。
			void Impedance(complexd val) { m_Impedance = val; }
			complexd Admittance() const { return m_Admittance; }	//母线1侧并联接地导纳的标幺值。注意励磁电纳应当为负值。
			void Admittance(complexd val) { m_Admittance = val; }
			complexd TapRatio() const { return m_TapRatio; }		//母线1侧与母线2侧的非标准变比，以及相位移动。
			void TapRatio(complexd val) { m_TapRatio = val; }
		public:
			virtual void Validate() const;
			virtual PiEquivalencyParameters PiEquivalency() const override;//获取此元件π型等值电路参数。
		protected:
			virtual NetworkObject* CloneInstance() const override;
		public:
			//创建一个变压器。
			static Transformer* Create(Bus *bus1, Bus *bus2, complexd impedance, complexd admittance, complexd tapRatio);
			//创建一个不计铁损的变压器。
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

		//含有非标准变比的无铁损三绕组变压器。
		class ThreeWindingTransformer final : public TriPortComponent
		{
		private:
			std::unique_ptr<Bus> m_CommonBus;
			std::unique_ptr<Transformer> m_Transformer1, m_Transformer2, m_Transformer3;
			complexd m_Impedance12, m_Impedance13, m_Impedance23, m_Admittance;
			complexd m_TapRatio1, m_TapRatio2, m_TapRatio3;
		private:
			//更新子级的参数。
			void UpdateChildren();
		public:
			//此变压器使用的一次侧内部公共母线。
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
		private:	//基础结构。
			virtual int ChildrenCount() const override;
			virtual NetworkObject* ChildAt(int index) const override;
		protected:
			virtual NetworkObject* CloneInstance() const override;
			virtual void OnExpand();
		public:
			//创建一个三绕组变压器。
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

