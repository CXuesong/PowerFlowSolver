//网络模型
//约定：所有的元件只保存与元件本身有关的信息，而不反映父级。
//约定：能不使用多态性完成的功能，就不使用多态性完成。
//TODO 考虑是否移除 Tag。可以通过CLR集合来控制CLR封装
//TODO 考虑是否移除 Bus 的 Index 属性（不能移除，例如潮流分析报告中需要此属性，但约束可以弱一些）

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

		//编号约定
		// 非负数：元件索引
		// -1：空白
		// 其他负数：内部保留。例如自动生成的元件的索引。
		const int NullIndex = -1;	//表示“空白”的母线编号

		//用于表示一个π型等值参数。
		struct PiEquivalencyParameters
		{
		public:
			complexd m_Impedance;
			complexd m_Admittance1;
			complexd m_Admittance2;
		public:
			complexd Impedance() const { return m_Impedance; }	//串联阻抗。
			void Impedance(complexd val) { m_Impedance = val; }
			complexd Admittance1() const { return m_Admittance1; }	//节点1接地导纳。
			void Admittance1(complexd val) { m_Admittance1 = val; }
			complexd Admittance2() const { return m_Admittance2; }	//节点2接地导纳。
			void Admittance2(complexd val) { m_Admittance2 = val; }
		public:
			PiEquivalencyParameters(complexd z, complexd y1, complexd y2);
		};
		
		////提供一个基类，用于为 NetworkObject 提供附加数据。
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
			//TODO 不进行唯一性检查，或者提供显式进行检查功能
			int m_Index;
			//unique_ptr<Tag> m_Tag;
			NetworkCase* m_CaseInfo;
			NetworkObject* m_Parent;
		private:	//internal
			void CaseInfo(NetworkCase* val) { m_CaseInfo = val; }
			void Parent(NetworkObject* val) { m_Parent = val; }
		public:
			//用于在案例中唯一地标识一个对象的索引。注意：不进行唯一性检查/显式进行检查。
			int Index() const { return m_Index; }
			void Index(int val);
			//获取应用程序自定义的附加信息。
			//::Tag* Tag() const { return m_Tag.get(); }
			//设置应用程序自定义的附加信息。
			//注意，不要将同一 Tag 指针设置给两个 NetworkObject 使用。
			//void Tag(::Tag* val) { m_Tag.reset(val); }

			//获取此对象所属于的案例。
			NetworkCase* CaseInfo() const { return m_CaseInfo; }
			//获取此对象所依存于的父级。
			NetworkObject* Parent() const { return m_Parent; }
		public:
			virtual void Validate() const;
		protected:
			NetworkObject(int index);
			NetworkObject();
		private:
			//不允许拷贝构造函数。
			NetworkObject(NetworkObject&) = delete;
		};

		// 母线。
		class Bus : public NetworkObject
		{
		private:
			complexd m_InitialVoltage;
		public:
			complexd InitialVoltage() const { return m_InitialVoltage; }	//迭代时母线使用的初始电压值（幅值：标幺值，相角：弧度）。
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
			virtual ::Bus* BusAt(int index) const = 0;	//基础结构。获取或设置连接在指定端口处的母线索引。端口索引从0开始。
			virtual int PortCount() const = 0;		//获取此元件端口的数目。
		protected:
			//检查指定的母线索引是否正确。
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
			::Bus* Bus() const { return m_Bus; }	//此元件连接到的母线索引。
			void Bus(::Bus* val) { m_Bus = val; }
		private:	//基础结构
			virtual ::Bus* BusAt(int index) const override;	//获取或设置连接在指定端口处的母线索引。端口索引从0开始。
			virtual int PortCount() const override;		//获取此元件端口的数目。
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
			::Bus* Bus1() const { return m_Bus1; }		//此元件连接到的母线1索引。
			void Bus1(::Bus* val) { m_Bus1 = val; }
			::Bus* Bus2() const { return m_Bus2; }		//此元件连接到的母线2索引。
			void Bus2(::Bus* val) { m_Bus2 = val; }
		public:
			virtual PiEquivalencyParameters PiEquivalency() const = 0;//获取此元件π型等值电路参数。
		private:		//基础结构
			virtual ::Bus* BusAt(int index) const override;//获取或设置连接在指定端口处的母线索引。端口索引从0开始。
			virtual int PortCount() const override;//获取此元件端口的数目。
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
			::Bus* Bus1() const { return m_Bus1; }		//此元件连接到的母线1索引。
			void Bus1(::Bus* val) { m_Bus1 = val; }
			::Bus* Bus2() const { return m_Bus2; }		//此元件连接到的母线2索引。
			void Bus2(::Bus* val) { m_Bus2 = val; }
			::Bus* Bus3() const { return m_Bus3; }		//此元件连接到的母线3索引。
			void Bus3(::Bus* val) { m_Bus3 = val; }
		private:		//基础结构
			virtual ::Bus* BusAt(int index) const override;//获取或设置连接在指定端口处的母线索引。端口索引从0开始。
			virtual int PortCount() const override;//获取此元件端口的数目。
		protected:
			TriPortComponent(int index, ::Bus* bus1, ::Bus* bus2, ::Bus* bus3);
			TriPortComponent(int index);
			TriPortComponent();
		};

		//提供了包含其他网络对象的容器功能。
		class IContainer
		{
		public:
			//获取容器中子级的数量。
			virtual int ChildrenCount() const = 0;
			//按索引获取容器中的某个子级。索引从0开始。
			virtual NetworkObject* ChildAt(int index) const = 0;
		};

		// π形等效传输线。
		class Line : public DoublePortComponent
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
			virtual PiEquivalencyParameters PiEquivalency() const;//获取此元件π型等值电路参数。
		public:
			Line();
			Line(::Bus* bus1, ::Bus* bus2, complexd impedance, complexd admittance);
		};

		// PV 发电机。
		class PVGenerator : public SinglePortComponent
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
		public:
			PVGenerator();
			PVGenerator(::Bus* bus, double activePower, double voltage);
		};

		// 平衡节点发电机。
		class SlackGenerator : public SinglePortComponent
		{
		private:
			complexd m_Voltage;
		public:
			complexd Voltage() const { return m_Voltage; }		//发电机的机端电压（幅值：标幺值，相角：弧度）。
			void Voltage(complexd val) { m_Voltage = val; }
		public:
			virtual void Validate() const;
		public:
			SlackGenerator();
			SlackGenerator(int index, ::Bus* bus, complexd voltage);
		};

		// PQ 负载。
		class PQLoad : public SinglePortComponent
		{
		private:
			complexd m_Power;
		public:
			complexd Power() const { return m_Power; }		//注入负载吸收的功率大小（标幺值）。
			void Power(complexd val) { m_Power = val; }
		public:
			virtual void Validate() const;
		public:
			PQLoad();
			PQLoad(::Bus* bus, complexd power);
		};

		//并联接地负载。
		class ShuntAdmittance : public SinglePortComponent
		{
		private:
			complexd m_Admittance;
		public:
			complexd Admittance() const { return m_Admittance; }	//导纳的标幺值。
			void Admittance(complexd val) { m_Admittance = val; }
		public:
			virtual void Validate() const;
		public:
			ShuntAdmittance();
			ShuntAdmittance(int index, ::Bus* bus, complexd admittance);
		};

		// 含有非标准变比的、可计入铁损的变压器。
		class Transformer : public DoublePortComponent
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
		public:
			Transformer();
			Transformer(::Bus* bus1, ::Bus* bus2, complexd impedance, complexd admittance, complexd tapRatio);
		};

		//含有非标准变比的无铁损三绕组变压器。
		class ThreeWindingTransformer : public TriPortComponent, public IContainer
		{
		private:
			std::shared_ptr<Bus> m_CommonBus;
			std::shared_ptr<Transformer> m_Transformer1, m_Transformer2, m_Transformer3;
			complexd m_Impedance12, m_Impedance13, m_Impedance23, m_Admittance;
			complexd m_TapRatio1, m_TapRatio2, m_TapRatio3;
		public:
			//获取此变压器使用的一次侧内部公共母线。
			Bus* CommonBus() const { return m_CommonBus.get(); }
			//获取此变压器使用的一次侧内部变压器。
			Transformer* Transformer1() const { return m_Transformer1.get(); }
			//获取此变压器使用的二次侧内部变压器。
			Transformer* Transformer2() const { return m_Transformer2.get(); }
			//获取此变压器使用的三次侧内部变压器。
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
		private:	//基础结构。
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