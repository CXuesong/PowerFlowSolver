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
#include <memory>

namespace PowerSolutions {
	namespace ObjectModel
	{
		class NetworkCase;
		class Component;
		typedef std::list<Component*> WeakComponentCollection;
		typedef WeakComponentCollection::const_iterator WeakComponentIterator;

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
			//TODO 不进行唯一性检查，或者提供显式进行检查功能
			int m_Index;
			NetworkCase* m_CaseInfo;
			NetworkObject* m_Parent;
			PerUnitBase m_BaseValue;
		protected:	//internal
			virtual void CaseInfo(NetworkCase* val) { m_CaseInfo = val; }
			void Parent(NetworkObject* val) { m_Parent = val; }
			//获取一个值，指示此元件或母线是否是由其他元件自动生成的。
			bool IsAutoGenerated() { return m_Parent == nullptr; }
			//当此对象的数据将要用于参与计算前，调用此方法。
			virtual void OnEvaluation() {}
		public:
			//用于在案例中唯一地标识一个对象的索引。注意：不进行唯一性检查/显式进行检查。
			int Index() const { return m_Index; }
			void Index(int val) { m_Index = val; }
			//获取此对象所属于的案例。
			NetworkCase* CaseInfo() const { return m_CaseInfo; }
			//获取此对象所依存于的父级。
			NetworkObject* Parent() const { return m_Parent; }
			PerUnitBase BaseValue() const { return m_BaseValue; }
			void BaseValue(PerUnitBase val) { m_BaseValue = val; }
		public:
			virtual void Validate() const;
			//获取此对象的一个副本。
			NetworkObject* Clone(NetworkCase* caseInfo) const;
		protected:
			//在非抽象派生类中重写，用于返回一个包含派生类数据的实例。
			virtual NetworkObject* CloneInstance() const = 0;
			//在抽象派生类中重写，用于向指定的副本中填充此类的数据。
			virtual void OnCloned(NetworkObject* newInstance, NetworkCase* caseInfo) const;
			NetworkObject(int index);
			NetworkObject();
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
			WeakComponentCollection m_Components;
		private:	//Internal
			void AttachComponent(Component* c);
			void DetachComponent(Component* c);
		public:
			complexd InitialVoltage() const { return m_InitialVoltage; }	//迭代时母线使用的初始电压值（幅值：标幺值，相角：弧度）。
			void InitialVoltage(complexd val) { m_InitialVoltage = val; }
			WeakComponentIterator ComponentsBegin() const { return m_Components.cbegin(); }
			WeakComponentIterator ComponentsEnd() const { return m_Components.cend(); }
			size_t ComponentsCount() const { return m_Components.size(); }
		public:
			virtual void Validate() const;
			//获取此对象的一个副本。
			Bus* Clone(NetworkCase* caseInfo) const
			{
				return static_cast<Bus*>(NetworkObject::Clone(caseInfo));
			}
		protected:
			virtual NetworkObject* CloneInstance() const override;
		public:
			Bus();
			Bus(int index);
			Bus(int index, complexd initialVoltage);
		};

		class Component : public NetworkObject
		{
			friend class NetworkCase;
		private:
			std::vector<Bus*> m_Buses;
		protected:
			virtual void CaseInfo(NetworkCase* val) override;
		public:
			using NetworkObject::CaseInfo;
			//获取连接在指定端口处的母线索引。端口索引从0开始。
			Bus* Buses(int index) const { return m_Buses[index]; }
			//设置连接在指定端口处的母线索引。端口索引从0开始。
			void Buses(int index, Bus* value);
			//获取此元件端口的数目。
			int PortCount() const { return m_Buses.size(); }
			//获取此对象的一个副本。
			Component* Clone(NetworkCase* caseInfo) const
			{
				return static_cast<Component*>(NetworkObject::Clone(caseInfo));
			}
		protected:
			virtual void OnCloned(NetworkObject* newInstance, NetworkCase* caseInfo) const override;
			//检查指定的母线索引是否正确。
			//void CheckBusIndex(int index, const TCHAR *argumentName, bool allowNull = false) const;
		protected:
			Component(int portCount);
		};

		class SinglePortComponent : public Component
		{
		public:
			Bus* Bus1() const { return Buses(0); }	//此元件连接到的母线索引。
			void Bus1(Bus* val) { Buses(0, val); }
		public:	//基础结构
			int PortCount() const = delete;					//基础结构。
		protected:
			SinglePortComponent();
		};

		class DoublePortComponent : public Component
		{
		public:
			Bus* Bus1() const { return Buses(0); }		//此元件连接到的母线1索引。
			void Bus1(Bus* val) { Buses(0, val); }
			Bus* Bus2() const { return Buses(1); }		//此元件连接到的母线2索引。
			void Bus2(Bus* val) { Buses(1, val); }
		public:
			virtual PiEquivalencyParameters PiEquivalency() const = 0;//获取此元件π型等值电路参数。
		public:	//基础结构
			int PortCount() const = delete;					//基础结构。
		protected:
			DoublePortComponent();
		};

		class TriPortComponent : public Component
		{
		public:
			Bus* Bus1() const { return Buses(0); }		//此元件连接到的母线1索引。
			void Bus1(Bus* val) { Buses(0, val); }
			Bus* Bus2() const { return Buses(1); }		//此元件连接到的母线2索引。
			void Bus2(Bus* val) { Buses(1, val); }
			Bus* Bus3() const { return Buses(2); }		//此元件连接到的母线3索引。
			void Bus3(Bus* val) { Buses(2, val); }
		public:	//基础结构
			int PortCount() const = delete;					//基础结构。
		protected:
			TriPortComponent();
		};

		//提供了包含其他网络对象的容器功能。
		class IContainer
		{
		public:
			//获取容器中子级的数量。
			virtual int ChildrenCount() const = 0;
			//按索引获取容器中的某个子级。索引从0开始。
			virtual std::shared_ptr<NetworkObject> ChildAt(int index) const = 0;
		};
	}
}

#endif	//__POWERSOLUTIONS_OBJECTMODEL_H
