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
			_WRAP_BEGIN_OBJECT_MODEL(Bus, _NATIVE_OM Bus)
				// ��ȡ������ı�������ʽ��
				_WRAP_OBJECT_MODEL_TOSTRING(_NATIVE_OM Bus)
				;
			_WRAP_END_OBJECT_MODEL;

			/// <summary>
			/// ��ʾһ��ĸ�߶ԡ�
			/// </summary>
			public value struct BusPair : IEquatable<BusPair>
			{
			public:
				property Bus Bus1;
				property Bus Bus2;
			public:
				static bool operator== (BusPair x, BusPair y) { return x.Bus1 == y.Bus1 && x.Bus2 == y.Bus2; }
				static bool operator!= (BusPair x, BusPair y) { return x.Bus1 != y.Bus1 || x.Bus2 != y.Bus2; }
				// ��ĸ�߶Խ��в�����˳��ıȽϡ�
				virtual bool Equals(BusPair obj);
				virtual bool Equals(Object^ obj) override;
				int GetHashCode() override;
			public:
				operator _NATIVE_OM BusPair() { return _NATIVE_OM BusPair((_NATIVE_OM Bus*)Bus1, (_NATIVE_OM Bus*)Bus2); }
				BusPair(Bus bus1, Bus bus2);
				BusPair(_NATIVE_OM BusPair pair);
			};

			/// <summary>
			/// ���ڶ� <see cref="BusPair" /> ִ��һ��������˳��ıȽϡ�
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
			/// ��ʾһ��Ԫ����
			/// ��<see cref="PowerSolutions::ObjectModel::Component" />��
			/// </summary>
			_WRAP_BEGIN_OBJECT_MODEL(Component, _NATIVE_OM Component)
				// ��ȡ������ı�������ʽ��
				_WRAP_OBJECT_MODEL_TOSTRING(_NATIVE_OM Component)
				;
			_WRAP_END_OBJECT_MODEL;

			/// <summary>
			/// ��ʾһ���������ѹ����
			/// ��<see cref="PowerSolutions::ObjectModel::ThreeWindingTransformer" />��
			/// </summary>
			_WRAP_BEGIN_OBJECT_MODEL(ThreeWindingTransformer, _NATIVE_OM ThreeWindingTransformer)
				_WRAP_OBJECT_MODEL_BASE(ThreeWindingTransformer, _NATIVE_OM ThreeWindingTransformer, Component)
				/// <summary>
				/// �������ʱ���������ѹ����ʹ�õĹ���ĸ�ߡ�
				/// </summary>
				_WRAP_PROPERTY_READONLY(CommonBus, Bus, Bus)
				// ��ȡ������ı�������ʽ��
				_WRAP_OBJECT_MODEL_TOSTRING(_NATIVE_OM ThreeWindingTransformer)
				;
			_WRAP_END_OBJECT_MODEL;

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
				/// <param name="impedance">ĸ��1�മ���迹�ı���ֵ��</param>
				/// <param name="admittance">ĸ��1�ಢ���ӵص��ɵı���ֵ��ע�����ŵ���Ӧ��Ϊ��ֵ��</param>
				/// <param name="tapRatio">ĸ��1����ĸ��2��ķǱ�׼��ȣ��Լ���λ�ƶ���(��λ�ƶ�δʵ�֡�)</param>
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
