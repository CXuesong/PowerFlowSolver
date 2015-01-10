//网络模型
//假定：所有的元件只保存与元件本身有关的信息，而不反映父级。
#ifndef __POWERSOLUTIONS_POWERFLOW_OBJECTMODEL_H
#define __POWERSOLUTIONS_POWERFLOW_OBJECTMODEL_H

#include "PowerSolutions.PowerFlow.h"
#include <vector>
#include <functional>

namespace PowerSolutions {
	namespace ObjectModel
	{
		class NetworkCase;

		//编号约定
		// 非负数：元件索引
		// -1：空白
		// 其他负数：内部保留。例如自动生成的元件的索引。
		const int NullIndex = -1;//表示“空白”的母线编号

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

		class NetworkObject
		{
			friend class NetworkCase;
		private:
			//TODO 不进行唯一性检查/显式进行检查
			int m_Index;
			void *m_Tag;
			NetworkCase *m_CaseInfo;
		private:	//internal
			void CaseInfo(NetworkCase * val) { m_CaseInfo = val; }
		public:
			//用于在案例中唯一地标识一个对象的索引。注意：不进行唯一性检查/显式进行检查。
			int Index() const { return m_Index; }
			void Index(int val);
			void* Tag() const { return m_Tag; }	//应用程序自定义的附加信息。
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
			virtual int BusAt(int index) const = 0;	//基础结构。获取或设置连接在指定端口处的母线索引。端口索引从0开始。
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
			int m_Bus;	
		public:
			int Bus() const { return m_Bus; }	//此元件连接到的母线索引。
			void Bus(int val) { m_Bus = val; }
		private:	//基础结构
			virtual int BusAt(int index) const;//获取或设置连接在指定端口处的母线索引。端口索引从0开始。
			virtual int PortCount() const;//获取此元件端口的数目。
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
			int Bus1() const { return m_Bus1; }		//此元件连接到的母线1索引。
			void Bus1(int val) { m_Bus1 = val; }
			int Bus2() const { return m_Bus2; }		//此元件连接到的母线2索引。
			void Bus2(int val) { m_Bus2 = val; }
		public:
			virtual PiEquivalencyParameters PiEquivalency() = 0;//获取此元件π型等值电路参数。
		private:		//基础结构
			virtual int BusAt(int index) const;//获取或设置连接在指定端口处的母线索引。端口索引从0开始。
			virtual int PortCount() const;//获取此元件端口的数目。
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
			int Bus1() const { return m_Bus1; }		//此元件连接到的母线1索引。
			void Bus1(int val) { m_Bus1 = val; }
			int Bus2() const { return m_Bus2; }		//此元件连接到的母线2索引。
			void Bus2(int val) { m_Bus2 = val; }
			int Bus3() const { return m_Bus3; }		//此元件连接到的母线3索引。
			void Bus3(int val) { m_Bus3 = val; }
		private:		//基础结构
			virtual int BusAt(int index) const;//获取或设置连接在指定端口处的母线索引。端口索引从0开始。
			virtual int PortCount() const;//获取此元件端口的数目。
		protected:
			TriPortComponent(int index, int bus1, int bus2, int bus3);
			TriPortComponent(int index);
			TriPortComponent();
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
			Bus(int index, complexd initialVoltage);
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
			virtual PiEquivalencyParameters PiEquivalency();//获取此元件π型等值电路参数。
		public:
			Line();
			Line(int index, int bus1, int bus2, complexd impedance, complexd admittance);
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
			PVGenerator(int index, int bus, double activePower, double voltage);
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
			SlackGenerator(int index, int bus, complexd voltage);
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
			PQLoad(int index, int bus, complexd power);
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
			ShuntAdmittance(int index, int bus, complexd admittance);
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
			complexd TapRatio() const { return m_TapRatio; }		//母线1侧与母线2侧的非标准变比。
			void TapRatio(complexd val) { m_TapRatio = val; }
		public:
			virtual void Validate() const;
			virtual PiEquivalencyParameters PiEquivalency();//获取此元件π型等值电路参数。
		public:
			Transformer();
			Transformer(int index, int bus1, int bus2, complexd impedance, complexd admittance, complexd tapRatio);
		};

		//含有非标准变比的无铁损三绕组变压器。
		class ThreeWindingTransformer : public TriPortComponent
		{
		};
	}
}

#endif