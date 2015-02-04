#include "stdafx.h"
#include "ObjectModel.h"

namespace PowerSolutions
{
	namespace Interop
	{
		_NATIVE_OM Bus* MarshalBus(IntPtr ptr)
		{
			auto obj = MarshalPointer<_NATIVE_OM NetworkObject>(ptr);
			Diagnostics::Debug::Assert(dynamic_cast<_NATIVE_OM Bus*>(obj) != nullptr);
			return static_cast<_NATIVE_OM Bus*>(obj);
		}

		namespace ObjectModel
		{
			NetworkCase::NetworkCase()
				: nativeObject(new _NATIVE_OM NetworkCase)
			{
				nativeObject->AutoDeleteChildren(true);
			}

			NetworkCase::!NetworkCase()
			{
				delete nativeObject;
				nativeObject = nullptr;
			}

			NetworkCase::~NetworkCase()
			{
				this->!NetworkCase();
			}

			void NetworkCase::AddObject(IntPtr obj)
			{
				nativeObject->AddObject(MarshalPointer<_NATIVE_OM NetworkObject>(obj));
			}

			Bus NetworkCase::AddBus(Complex initialVoltage)
			{
				return Bus(nativeObject->CreateBus(MarshalComplex(initialVoltage)));
			}

			Component NetworkCase::AddLine(Bus bus1, Bus bus2, Complex impedance, Complex admittance)
			{
				_CHECK_NON_NULLABLE_PARAM(bus1);
				_CHECK_NON_NULLABLE_PARAM(bus2);
				auto newInst = new _NATIVE_OM Line((bus1), (bus2), MarshalComplex(impedance), MarshalComplex(admittance));
				nativeObject->AddObject(newInst);
				return Component(newInst);
			}

			Component NetworkCase::AddPVGenerator(Bus bus1, double activePower, double voltage)
			{
				_CHECK_NON_NULLABLE_PARAM(bus1);
				auto newInst = new _NATIVE_OM PVGenerator((bus1), activePower, voltage);
				nativeObject->AddObject(newInst);
				return Component(newInst);
			}

			Component NetworkCase::AddSlackGenerator(Bus bus1, Complex voltage)
			{
				_CHECK_NON_NULLABLE_PARAM(bus1);
				auto newInst = new _NATIVE_OM SlackGenerator((bus1), MarshalComplex(voltage));
				nativeObject->AddObject(newInst);
				return Component(newInst);
			}

			Component NetworkCase::AddPQLoad(Bus bus1, Complex power)
			{
				_CHECK_NON_NULLABLE_PARAM(bus1);
				auto newInst = new _NATIVE_OM PQLoad((bus1), MarshalComplex(power));
				nativeObject->AddObject(newInst);
				return Component(newInst);
			}

			Component NetworkCase::AddShuntAdmittance(Bus bus1, Complex admittance)
			{
				_CHECK_NON_NULLABLE_PARAM(bus1);
				auto newInst = new _NATIVE_OM ShuntAdmittance((bus1), MarshalComplex(admittance));
				nativeObject->AddObject(newInst);
				return Component(newInst);
			}

			Component NetworkCase::AddTransformer(Bus bus1, Bus bus2, Complex impedance, Complex admittance, Complex tapRatio)
			{
				_CHECK_NON_NULLABLE_PARAM(bus1);
				_CHECK_NON_NULLABLE_PARAM(bus2);
				auto newInst = new _NATIVE_OM Transformer(
					(bus1), (bus2),
					MarshalComplex(impedance), MarshalComplex(admittance),
					MarshalComplex(tapRatio));
				nativeObject->AddObject(newInst);
				return Component(newInst);
			}

			ThreeWindingTransformer NetworkCase::AddThreeWindingTransformer(Bus bus1, Bus bus2, Bus bus3, Complex impedance12, Complex impedance13, Complex impedance23, Complex admittance, Complex tapRatio1, Complex tapRatio2, Complex tapRatio3)
			{
				_CHECK_NON_NULLABLE_PARAM(bus1);
				_CHECK_NON_NULLABLE_PARAM(bus2);
				_CHECK_NON_NULLABLE_PARAM(bus3);
				auto newInst = new _NATIVE_OM ThreeWindingTransformer(
					(bus1), (bus2), (bus3),
					MarshalComplex(impedance12), MarshalComplex(impedance13),
					MarshalComplex(impedance23), MarshalComplex(admittance),
					MarshalComplex(tapRatio1), MarshalComplex(tapRatio2),
					MarshalComplex(tapRatio3));
				nativeObject->AddObject(newInst);
				return ThreeWindingTransformer(newInst);
			}

			BusPair::BusPair(Bus bus1, Bus bus2)
			{
				Bus1 = bus1;
				Bus2 = bus2;
			}

			BusPair::BusPair(_NATIVE_OM BusPair pair)
			{
				Bus1 = Bus(pair.first);
				Bus2 = Bus(pair.second);
			}

			bool BusPair::Equals(BusPair obj)
			{
				return obj.Bus1 == Bus1 && obj.Bus2 == Bus2;
			}

			bool BusPair::Equals(Object^ obj)
			{
				if (Component::typeid->IsInstanceOfType(obj))
				{
					return this->Equals((BusPair)obj);
				}
				return false;
			}

			int BusPair::GetHashCode()
			{
				return Bus1.GetHashCode() ^ Bus2.GetHashCode();
			}

		}
	}
}

