#pragma once

#include "PowerSolutions.Interop.h"

namespace PowerSolutions
{
	namespace Interop
	{
		namespace ObjectModel
		{
			/// <summary>
			/// 表示一个母线。
			/// （<see cref="PowerSolutions::ObjectModel::Bus" />）
			/// </summary>
			_WRAP_BEGIN_OBJECT_MODEL(Bus, _NATIVE_OM Bus)
				// 获取句柄的文本表现形式。
				_WRAP_OBJECT_MODEL_TOSTRING(_NATIVE_OM Bus)
				;
			_WRAP_END_OBJECT_MODEL;

			/// <summary>
			/// 表示一个母线对。
			/// </summary>
			public value struct BusPair : IEquatable<BusPair>
			{
			public:
				property Bus Bus1;
				property Bus Bus2;
			public:
				static bool operator== (BusPair x, BusPair y) { return x.Bus1 == y.Bus1 && x.Bus2 == y.Bus2; }
				static bool operator!= (BusPair x, BusPair y) { return x.Bus1 != y.Bus1 || x.Bus2 != y.Bus2; }
				// 对母线对进行不区分顺序的比较。
				virtual bool Equals(BusPair obj);
				virtual bool Equals(Object^ obj) override;
				int GetHashCode() override;
			public:
				operator _NATIVE_OM BusPair() { return _NATIVE_OM BusPair((_NATIVE_OM Bus*)Bus1, (_NATIVE_OM Bus*)Bus2); }
				BusPair(Bus bus1, Bus bus2);
				BusPair(_NATIVE_OM BusPair pair);
			};

			/// <summary>
			/// 用于对 <see cref="BusPair" /> 执行一个不区分顺序的比较。
			/// </summary>
			public ref class BusPairUnorderedComparer : EqualityComparer < BusPair >
			{
			public:
				static BusPairUnorderedComparer^ m_Default = gcnew BusPairUnorderedComparer();
			public:
				static property BusPairUnorderedComparer^ Default
				{
					BusPairUnorderedComparer^ get() 
					{
						return m_Default;
					}
				}
				virtual bool Equals(BusPair x, BusPair y) override
				{
					return x.Bus1 == y.Bus1 && x.Bus2 == y.Bus2 ||
						x.Bus1 == y.Bus2 && x.Bus2 == y.Bus1;
				}
				virtual int GetHashCode(BusPair obj) override
				{
					return obj.GetHashCode();
				}
			};

			/// <summary>
			/// 表示一个元件。
			/// （<see cref="PowerSolutions::ObjectModel::Component" />）
			/// </summary>
			_WRAP_BEGIN_OBJECT_MODEL(Component, _NATIVE_OM Component)
				// 获取句柄的文本表现形式。
				_WRAP_OBJECT_MODEL_TOSTRING(_NATIVE_OM Component)
				;
			_WRAP_END_OBJECT_MODEL;

			/// <summary>
			/// 表示一个三绕组变压器。
			/// （<see cref="PowerSolutions::ObjectModel::ThreeWindingTransformer" />）
			/// </summary>
			_WRAP_BEGIN_OBJECT_MODEL(ThreeWindingTransformer, _NATIVE_OM ThreeWindingTransformer)
				_WRAP_OBJECT_MODEL_BASE(ThreeWindingTransformer, _NATIVE_OM ThreeWindingTransformer, Component)
				/// <summary>
				/// 参与计算时，三绕组变压器所使用的公共母线。
				/// </summary>
				_WRAP_PROPERTY_READONLY(CommonBus, Bus, Bus)
				// 获取句柄的文本表现形式。
				_WRAP_OBJECT_MODEL_TOSTRING(_NATIVE_OM ThreeWindingTransformer)
				;
			_WRAP_END_OBJECT_MODEL;

			/// <summary>
			/// 表示一个网络案例。
			///（<see cref="PowerSolutions::ObjectModel::NetworkCase" />）
			/// </summary>
			public ref class NetworkCase
			{
			internal:
				_NATIVE_OM NetworkCase* nativeObject;
			private:
				void AddObject(IntPtr obj);
			public:
				Bus AddBus(Complex initialVoltage);
				Bus AddBus() { return AddBus(1); }
				Component AddLine(Bus bus1, Bus bus2, Complex impedance, Complex admittance);
				Component AddPVGenerator(Bus bus1, double activePower, double voltage);
				Component AddSlackGenerator(Bus bus1, Complex voltage);
				Component AddPQLoad(Bus bus1, Complex power);
				Component AddShuntAdmittance(Bus bus1, Complex admittance);
				/// <param name="impedance">母线1侧串联阻抗的标幺值。</param>
				/// <param name="admittance">母线1侧并联接地导纳的标幺值。注意励磁电纳应当为负值。</param>
				/// <param name="tapRatio">母线1侧与母线2侧的非标准变比，以及相位移动。(相位移动未实现。)</param>
				Component AddTransformer(Bus bus1, Bus bus2, Complex impedance, Complex admittance, Complex tapRatio);
				Component AddTransformer(Bus bus1, Bus bus2, Complex impedance, Complex tapRatio)
				{
					return AddTransformer(bus1, bus2, impedance, 0, tapRatio);
				}
				ThreeWindingTransformer AddThreeWindingTransformer(Bus bus1, Bus bus2, Bus bus3,
					Complex impedance12, Complex impedance13, Complex impedance23,
					Complex admittance, Complex tapRatio1, Complex tapRatio2, Complex tapRatio3);
				ThreeWindingTransformer AddThreeWindingTransformer(Bus bus1, Bus bus2, Bus bus3,
					Complex impedance12, Complex impedance13, Complex impedance23,
					Complex tapRatio1, Complex tapRatio2, Complex tapRatio3)
				{
					return AddThreeWindingTransformer(bus1, bus2, bus3, impedance12, impedance13, impedance23, 0, tapRatio1, tapRatio2, tapRatio3);
				}
			public:
				NetworkCase();
				!NetworkCase();
				~NetworkCase();
			};
		}
	}
}
