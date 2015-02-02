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
				auto newInst = new _NATIVE_OM Line((bus1), (bus2), MarshalComplex(impedance), MarshalComplex(admittance));
				nativeObject->AddObject(newInst);
				return Component(newInst);
			}

			Component NetworkCase::AddPVGenerator(Bus bus1, double activePower, double voltage)
			{
				auto newInst = new _NATIVE_OM PVGenerator((bus1), activePower, voltage);
				nativeObject->AddObject(newInst);
				return Component(newInst);
			}

			Component NetworkCase::AddSlackGenerator(Bus bus1, Complex voltage)
			{
				auto newInst = new _NATIVE_OM SlackGenerator((bus1), MarshalComplex(voltage));
				nativeObject->AddObject(newInst);
				return Component(newInst);
			}

			Component NetworkCase::AddPQLoad(Bus bus1, Complex power)
			{
				auto newInst = new _NATIVE_OM PQLoad((bus1), MarshalComplex(power));
				nativeObject->AddObject(newInst);
				return Component(newInst);
			}

			Component NetworkCase::AddShuntAdmittance(Bus bus1, Complex admittance)
			{
				auto newInst = new _NATIVE_OM ShuntAdmittance((bus1), MarshalComplex(admittance));
				nativeObject->AddObject(newInst);
				return Component(newInst);
			}

			Component NetworkCase::AddTransformer(Bus bus1, Bus bus2, Complex impedance, Complex admittance, Complex tapRatio)
			{
				auto newInst = new _NATIVE_OM Transformer(
					(bus1), (bus2),
					MarshalComplex(impedance), MarshalComplex(admittance),
					MarshalComplex(tapRatio));
				nativeObject->AddObject(newInst);
				return Component(newInst);
			}

			Component NetworkCase::AddThreeWindingTransformer(Bus bus1, Bus bus2, Bus bus3, Complex impedance12, Complex impedance13, Complex impedance23, Complex admittance, Complex tapRatio1, Complex tapRatio2, Complex tapRatio3)
			{
				auto newInst = new _NATIVE_OM ThreeWindingTransformer(
					(bus1), (bus2), (bus3),
					MarshalComplex(impedance12), MarshalComplex(impedance13),
					MarshalComplex(impedance23), MarshalComplex(admittance),
					MarshalComplex(tapRatio1), MarshalComplex(tapRatio2),
					MarshalComplex(tapRatio3));
				nativeObject->AddObject(newInst);
				return Component(newInst);
			}

			Bus::Bus(_NATIVE_OM Bus* native)
				: nativeObject(native)
			{ }

			bool Bus::Equals(Object^ obj)
			{
				auto other = dynamic_cast<Bus^>(obj);
				if (other != nullptr) return this->nativeObject == other->nativeObject;
				return false;
			}

			int Bus::GetHashCode()
			{
				return ((size_t)nativeObject).GetHashCode();
			}

			Component::Component(_NATIVE_OM Component* native)
				: nativeObject(native)
			{ }

			bool Component::Equals(Object^ obj)
			{
				auto other = dynamic_cast<Component^>(obj);
				if (other != nullptr) return this->nativeObject == other->nativeObject;
				return false;
			}

			int Component::GetHashCode()
			{
				return ((size_t)nativeObject).GetHashCode();
			}
		}
	}
}

