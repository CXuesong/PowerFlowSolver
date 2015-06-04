/*
PowerSolutions
公共对象模型
by Chen [CXuesong.], 2015
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
			PowerFlow::ComponentFlowSolution EvalComponentFlow(complexd voltage1, complexd voltage2) const;
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

		// 为网络案例中的对象（例如元件或母线）提供公共基类。
		// 网络案例中的所有对象通过内存地址（指针）来互相区分，因而无需额外增加编号这一特性。因为对象模型中的编号其实和实际计算时的编号没有什么关系。
		class NetworkObject
		{
#if _DEBUG
		public:
			static unsigned long _IDCounter;
			unsigned long _ID;	//一个标识符，用于在调试模式下区分不同的网络对象。
#endif
		private:
			void* _Tag;
		public:
			void* Tag() const { return _Tag; }
			void Tag(void* val) { _Tag = val; }
		public:
			virtual void Validate() const;
			//获取此对象的一个副本。
			NetworkObject* Clone(const NetworkCaseCorrespondenceInfo& context) const;
		protected:
			//在非抽象派生类中重写，用于返回一个包含派生类数据的实例。
			virtual NetworkObject* CloneInstance() const = 0;
			//在抽象派生类中重写，用于向指定的副本中填充此类的数据。
			virtual void OnCloned(NetworkObject* newInstance, const NetworkCaseCorrespondenceInfo& context) const;
			NetworkObject();
		public:
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
			IBusContainer* m_Parent;
		public:
			IBusContainer* Parent() const { return m_Parent; }
			void Parent(IBusContainer* val) { m_Parent = val; }
			complexd InitialVoltage() const { return m_InitialVoltage; }	//迭代时母线使用的初始电压值（幅值：标幺值，相角：弧度）。
			void InitialVoltage(complexd val) { m_InitialVoltage = val; }
		public:
			virtual void Validate() const;
			//获取此对象的一个副本。
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
			NetworkObject* Clone(const NetworkCaseCorrespondenceInfo& context) const
			{
				return static_cast<Component*>(NetworkObject::Clone(context));
			}
			virtual void BuildNodeInfo(PrimitiveNetwork* pNetwork);
			virtual void BuildAdmittanceInfo(PrimitiveNetwork* pNetwork) {}
			/*
			功率流向
			Component -----> [node]   PowerInjection
			            |
						|   PowerShunt [0]
			*/
			//根据节点电压获取此元件注入指定母线节点的功率。
			//index = 0 : 接地功率
			//index > 0 : 某端口的注入母线的功率
			virtual PowerFlow::ComponentFlowSolution EvalComponentFlow(const PowerFlow::PrimitiveSolution& solution) const = 0;
		protected:
			virtual void OnCloned(NetworkObject* newInstance, const NetworkCaseCorrespondenceInfo& context) const override;
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
			virtual void BuildNodeInfo(PrimitiveNetwork* pNetwork) override;
			virtual void BuildAdmittanceInfo(PrimitiveNetwork* pNetwork) override;
			virtual PowerFlow::ComponentFlowSolution EvalComponentFlow(const PowerFlow::PrimitiveSolution& solution) const override;
		public:	//基础结构
			int PortCount() const = delete;					//基础结构。
		protected:
			DoublePortComponent(Bus* bus1, Bus* bus2);
		};

		class TriPortComponent : public Component
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

		class IBusContainer abstract	//表示元件内部包含一个或多个母线。
		{
		public:
			virtual int ChildBusCount() const = 0;			//获取元件内部母线的数量。
			virtual Bus* ChildBusAt(int index) const  = 0;	//按照索引获取元件内部的母线。索引从0开始。
		};
	}
}

#endif	//__POWERSOLUTIONS_OBJECTMODEL_H
