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
			public value struct Bus : IEquatable<Bus>
			{
			private:
				_NATIVE_OM Bus* nativeObject;
			public:
				static bool operator== (const Bus x, const Bus y) { return x.nativeObject == y.nativeObject; }
				static bool operator!= (const Bus x, const Bus y) { return x.nativeObject != y.nativeObject; }
				virtual bool Equals(Bus obj);
				virtual bool Equals(Object^ obj) override;
				int GetHashCode() override;
			internal:
				operator _NATIVE_OM Bus*() { return nativeObject; }
				Bus(_NATIVE_OM Bus* native);
			};

			/// <summary>
			/// 表示一个母线对。默认情况下，母线对的顺序是无关紧要的。
			/// </summary>
			public value struct BusPair : IEquatable<BusPair>
			{
			public:
				property Bus Bus1;
				property Bus Bus2;
			public:
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
			/// 表示一个元件。
			/// （<see cref="PowerSolutions::ObjectModel::Component" />）
			/// </summary>
			public value struct Component : IEquatable<Component>
			{
			private:
				_NATIVE_OM Component* nativeObject;
			public:
				static bool operator== (const Component x, const Component y) { return x.nativeObject == y.nativeObject; }
				static bool operator!= (const Component x, const Component y) { return x.nativeObject != y.nativeObject; }
				virtual bool Equals(Component obj);
				virtual bool Equals(Object^ obj) override;
				int GetHashCode() override;
			internal:
				operator _NATIVE_OM Component*() { return nativeObject; }
				Component(_NATIVE_OM Component* native);
			};

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
				Component AddTransformer(Bus bus1, Bus bus2, Complex impedance, Complex admittance, Complex tapRatio);
				Component AddTransformer(Bus bus1, Bus bus2, Complex impedance, Complex tapRatio)
				{
					return AddTransformer(bus1, bus2, impedance, 0, tapRatio);
				}
				Component AddThreeWindingTransformer(Bus bus1, Bus bus2, Bus bus3,
					Complex impedance12, Complex impedance13, Complex impedance23,
					Complex admittance, Complex tapRatio1, Complex tapRatio2, Complex tapRatio3);
				Component AddThreeWindingTransformer(Bus bus1, Bus bus2, Bus bus3,
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
