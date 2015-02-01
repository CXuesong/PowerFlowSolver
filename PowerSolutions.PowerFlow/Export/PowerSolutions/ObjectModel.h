/*
PowerSolutions
公共对象模型
by  Chen [CXuesong.], 2015
*/

//约定：如果使用 Attach 将元件绑定到 NetworkCase，则 NetworkCase 将拥有对此元件的资源占有权。
//注意，这意味着如果将元件脱离出 NetworkCase（而非转移至其它 NetworkCase），其自身会被自动 delete。

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

		//用于表示标幺值的基值。
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

		//为网络案例中的对象（例如元件或母线）提供公共基类。
		class NetworkObject
		{
			friend class NetworkCase;
			friend class ThreeWindingTransformer;
		private:
		protected:	//internal
			//当此对象的数据将要用于参与计算前，调用此方法。
			virtual void OnExpand() {}
		public:
			virtual void Validate() const;
			//获取此对象的一个副本。
			NetworkObject* Clone(const NetworkCaseCloneContext& context) const;
		protected:
			//在非抽象派生类中重写，用于返回一个包含派生类数据的实例。
			virtual NetworkObject* CloneInstance() const = 0;
			//在抽象派生类中重写，用于向指定的副本中填充此类的数据。
			virtual void OnCloned(NetworkObject* newInstance, const NetworkCaseCloneContext& context) const;
			NetworkObject();
			virtual ~NetworkObject();
		private:
			//不允许拷贝构造函数。
			NetworkObject(NetworkObject&) = delete;
			NetworkObject& operator=(NetworkObject&) = delete;
		};

		// 母线。
		class Bus final : public NetworkObject
		{
			friend class Component;
		private:
			complexd m_InitialVoltage;
		private:	//Internal
			void AttachComponent(Component* c);
			void DetachComponent(Component* c);
		public:
			complexd InitialVoltage() const { return m_InitialVoltage; }	//迭代时母线使用的初始电压值（幅值：标幺值，相角：弧度）。
			void InitialVoltage(complexd val) { m_InitialVoltage = val; }
		public:
			virtual void Validate() const;
			//获取此对象的一个副本。
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

		//表示与一个或多个母线相连接的元件。
		class Component : public NetworkObject
		{
			friend class NetworkCase;
		private:
			std::vector<Bus*> m_Buses;
		public:
			//获取连接在指定端口处的母线索引。端口索引从0开始。
			Bus* Buses(int index) const { return m_Buses[index]; }
			//设置连接在指定端口处的母线索引。端口索引从0开始。
			void Buses(int index, Bus* value) { m_Buses[index] = value; }
			//获取此元件端口的数目。
			int PortCount() const { return m_Buses.size(); }
			//获取此对象的一个副本。
			NetworkObject* Clone(const NetworkCaseCloneContext& context) const
			{
				return static_cast<Component*>(NetworkObject::Clone(context));
			}
			/*
			功率流向
			Component -----> [node]   PowerInjection
			            |
						|   PowerShunt
			*/
			//根据节点电压获取此元件注入指定母线节点的功率。
			//virtual complexd EvalPowerInjection(int busIndex, std::vector<complexd>& busVoltage);
			//根据节点电压获取此元件在指定端口处注入地的功率。
			//virtual complexd EvalPowerShunt(int busIndex, std::vector<complexd>& busVoltage);
		protected:
			virtual void OnCloned(NetworkObject* newInstance, const NetworkCaseCloneContext& context) const override;
		protected:
			Component(int portCount);
		};

		class SinglePortComponent : public Component
		{
		public:
			Bus* Bus1() const { return Buses(0); }	//此元件连接到的母线。
			void Bus1(Bus* val) { Buses(0, val); }
		public:	//基础结构
			int PortCount() const = delete;					//基础结构。
		protected:
			SinglePortComponent(Bus* bus1);
		};

		class DoublePortComponent : public Component
		{
		public:
			Bus* Bus1() const { return Buses(0); }		//此元件连接到的母线1。
			void Bus1(Bus* val) { Buses(0, val); }
			Bus* Bus2() const { return Buses(1); }		//此元件连接到的母线2。
			void Bus2(Bus* val) { Buses(1, val); }
		public:
			virtual PiEquivalencyParameters PiEquivalency() const = 0;//获取此元件π型等值电路参数。
		public:	//基础结构
			int PortCount() const = delete;					//基础结构。
		protected:
			DoublePortComponent(Bus* bus1, Bus* bus2);
		};

		//表示一个由母线和元件组成的复合元件。
		class ComplexComponent : public Component
		{
		public:
			//获取容器中子级的数量。
			virtual int ChildrenCount() const = 0;
			//按索引获取容器中的某个子级。索引从0开始。
			virtual NetworkObject* ChildAt(int index) const = 0;
		protected:
			ComplexComponent(int portCount);
		};


		class TriPortComponent : public ComplexComponent
		{
		public:
			Bus* Bus1() const { return Buses(0); }		//此元件连接到的母线1。
			void Bus1(Bus* val) { Buses(0, val); }
			Bus* Bus2() const { return Buses(1); }		//此元件连接到的母线2。
			void Bus2(Bus* val) { Buses(1, val); }
			Bus* Bus3() const { return Buses(2); }		//此元件连接到的母线3。
			void Bus3(Bus* val) { Buses(2, val); }
		public:	//基础结构
			int PortCount() const = delete;					//基础结构。
		protected:
			TriPortComponent(Bus* bus1, Bus* bus2, Bus* bus3);
		};
	}
}

#endif	//__POWERSOLUTIONS_OBJECTMODEL_H
