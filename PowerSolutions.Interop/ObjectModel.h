#pragma once

#include "PowerSolutions.Interop.h"

namespace PowerSolutions
{
	namespace Interop
	{
		namespace ObjectModel
		{
			/// <summary>
			/// ��ʾһ��ĸ�ߡ�
			/// ��<see cref="PowerSolutions::ObjectModel::Bus" />��
			/// </summary>
			public value struct Bus
			{
			private:
				_NATIVE_OM Bus* nativeObject;
			public:
				static bool operator== (const Bus x, const Bus y) { return x.nativeObject == y.nativeObject; }
				static bool operator!= (const Bus x, const Bus y) { return x.nativeObject != y.nativeObject; }
				bool Equals(Object^ obj) override;
				int GetHashCode() override;
			internal:
				operator _NATIVE_OM Bus*() { return nativeObject; }
				Bus(_NATIVE_OM Bus* native);
			};

			/// <summary>
			/// ��ʾһ��Ԫ����
			/// ��<see cref="PowerSolutions::ObjectModel::Component" />��
			/// </summary>
			public value struct Component
			{
			private:
				_NATIVE_OM Component* nativeObject;
			public:
				static bool operator== (const Component x, const Component y) { return x.nativeObject == y.nativeObject; }
				static bool operator!= (const Component x, const Component y) { return x.nativeObject != y.nativeObject; }
				bool Equals(Object^ obj) override;
				int GetHashCode() override;
			internal:
				operator _NATIVE_OM Component*() { return nativeObject; }
				Component(_NATIVE_OM Component* native);
			};

			/// <summary>
			/// ��ʾһ�����簸����
			///��<see cref="PowerSolutions::ObjectModel::NetworkCase" />��
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
