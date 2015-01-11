#include "ObjectModel.h"
#include "NetworkCase.h"
#include "Exceptions.h"

using namespace PowerSolutions;
using namespace PowerSolutions::ObjectModel;
using namespace std;

PiEquivalencyParameters::PiEquivalencyParameters(complexd z, complexd y1, complexd y2)
	: m_Impedance(z), m_Admittance1(y1), m_Admittance2(y2)
{ }

////////// 网络对象 //////////
NetworkObject::NetworkObject()
	: NetworkObject(NullIndex)
{ }

NetworkObject::NetworkObject(int index)
	: m_Index(index), m_CaseInfo(nullptr), m_Parent(nullptr)
{ }

void NetworkObject::Validate() const
{

}

void NetworkObject::Index(int val)
{
	if (m_CaseInfo != nullptr)
	{
		if (!m_CaseInfo->OnChildIndexChanged(this, m_Index, val))
		{
			throw Exception(ExceptionCode::InvalidOperation);
		}
	}
	m_Index = val;
}

////////// 组件 //////////
Component::Component()
{ }

Component::Component(int index)
	: NetworkObject(index)
{ }

SinglePortComponent::SinglePortComponent()
	: m_Bus(nullptr)
{ }

SinglePortComponent::SinglePortComponent(int index)
	: Component(index), m_Bus(nullptr)
{ }

SinglePortComponent::SinglePortComponent(int index, ::Bus* bus)
	: Component(index), m_Bus(bus)
{ }

Bus* SinglePortComponent::BusAt(int index) const
{
	assert(index < 1);
	return m_Bus;
}

int SinglePortComponent::PortCount() const
{
	return 1;
}

DoublePortComponent::DoublePortComponent()
	: m_Bus1(nullptr), m_Bus2(nullptr)
{ }

DoublePortComponent::DoublePortComponent(int index)
	: Component(index), m_Bus1(nullptr), m_Bus2(nullptr)
{ }

DoublePortComponent::DoublePortComponent(int index, ::Bus* bus1, ::Bus* bus2)
	: Component(index), m_Bus1(bus1), m_Bus2(bus2)
{ }

Bus* DoublePortComponent::BusAt(int index) const
{
	assert(index < 2);
	return index == 0 ? m_Bus1 : m_Bus2;
}

int DoublePortComponent::PortCount() const
{
	return 2;
}

TriPortComponent::TriPortComponent()
	: m_Bus1(nullptr), m_Bus2(nullptr), m_Bus3(nullptr)
{ }

TriPortComponent::TriPortComponent(int index)
	: Component(index), m_Bus1(nullptr), m_Bus2(nullptr), m_Bus3(nullptr)
{ }

TriPortComponent::TriPortComponent(int index, ::Bus* bus1, ::Bus* bus2, ::Bus* bus3)
	: Component(index), m_Bus1(bus1), m_Bus2(bus2), m_Bus3(nullptr)
{ }

Bus* TriPortComponent::BusAt(int index) const
{
	assert(index < 3);
	switch (index)
	{
	case 0: return m_Bus1;
	case 1: return m_Bus2;
	case 2: return m_Bus3;
	default:
		assert(false);
		return nullptr;
	}
}

int TriPortComponent::PortCount() const
{
	return 3;
}

////////// 母线 //////////
Bus::Bus()
	: m_InitialVoltage(1, 0)
{ }

Bus::Bus(int index, complexd initialVoltage)
	: NetworkObject(index), m_InitialVoltage(initialVoltage)
{ }

Bus::Bus(int index)
	: Bus(index, 1)
{ }

void Bus::Validate() const
{
	/*if (Index < 0)
		throw InvalidArgumentException(_T("Index"));
		if (CaseInfo != nullptr && CaseInfo->Buses().find(Index) != CaseInfo->Buses().end())
		throw ValidationException(Format(ERROR_BUS_INDEX_DUPLICATED, Index));*/
}

////////// 传输线 //////////
Line::Line()
	: m_Impedance(0), m_Admittance(0)
{ }

Line::Line(::Bus* bus1, ::Bus* bus2, complexd impedance, complexd admittance)
	: DoublePortComponent(NullIndex, bus1, bus2),
	m_Impedance(impedance), m_Admittance(admittance)
{ }

void Line::Validate() const
{
	//_CheckBusIndex(Bus1);
	//_CheckBusIndex(Bus2);
}

PiEquivalencyParameters Line::PiEquivalency() const
{
	complexd y2 = m_Admittance / 2.0;
	return PiEquivalencyParameters(m_Impedance, y2, y2);
}

////////// PV发电机 //////////
PVGenerator::PVGenerator()
	: m_ActivePower(0), m_Voltage(1)
{ }

PVGenerator::PVGenerator(::Bus* bus, double activePower, double voltage)
	: SinglePortComponent(NullIndex, bus), m_ActivePower(activePower), m_Voltage(voltage)
{ }

void PVGenerator::Validate() const
{
	//_CheckBusIndex(Bus);
}

////////// 平衡节点发电机 //////////
SlackGenerator::SlackGenerator()
	: m_Voltage(1, 0)
{ }

SlackGenerator::SlackGenerator(int index, ::Bus* bus, complexd voltage)
	: SinglePortComponent(index, bus), m_Voltage(voltage)
{ }

void SlackGenerator::Validate() const
{
	//_CheckBusIndex(Bus);
}

////////// PQ负载 //////////
PQLoad::PQLoad()
	: m_Power(0, 0)
{ }

PQLoad::PQLoad(::Bus* bus, complexd power)
	: SinglePortComponent(NullIndex, bus), m_Power(power)
{ }

void PQLoad::Validate() const
{
	//_CheckBusIndex(Bus);
}

////////// 接地导纳 //////////
ShuntAdmittance::ShuntAdmittance()
{ }

ShuntAdmittance::ShuntAdmittance(int index, ::Bus* bus, complexd admittance)
	: SinglePortComponent(index, bus), m_Admittance(admittance)
{ }

void ShuntAdmittance::Validate() const
{
	//_CheckBusIndex(Bus);
}

////////// 变压器 //////////
Transformer::Transformer()
	: m_Impedance(0), m_Admittance(0), m_TapRatio(1)
{ }

Transformer::Transformer(::Bus* bus1, ::Bus* bus2, complexd impedance, complexd admittance, complexd tapRatio)
	: DoublePortComponent(NullIndex, bus1, bus2),
	m_Impedance(impedance), m_Admittance(admittance), m_TapRatio(tapRatio)
{ }

void Transformer::Validate() const
{
	//_CheckBusIndex(Bus1);
	//_CheckBusIndex(Bus2);
}

PiEquivalencyParameters Transformer::PiEquivalency() const
{
	// Z_pi = Z_T / k
	// Y_pi1 = (1 - k) / Z_T
	// Y_pi2 = k * (k - 1) / Z_T
	return PiEquivalencyParameters(
		m_Impedance / m_TapRatio,
		(1.0 - m_TapRatio) / m_Impedance + m_Admittance,
		m_TapRatio * (m_TapRatio - 1.0) / m_Impedance);
}

NetworkObject* ThreeWindingTransformer::ChildAt(int index) const
{
	switch (index)
	{
	case 0: return m_CommonBus.get();
	case 1: return m_Transformer1.get();
	case 2: return m_Transformer2.get();
	case 3: return m_Transformer3.get();
	default: throw Exception(ExceptionCode::ArgumentOutOfRange);
	}
}

ThreeWindingTransformer::ThreeWindingTransformer(::Bus* bus1, ::Bus* bus2, ::Bus* bus3,
	complexd impedance12, complexd impedance13, complexd impedance23,
	complexd admittance, complexd tapRatio1, complexd tapRatio2, complexd tapRatio3)
	: TriPortComponent(NullIndex, bus1, bus2, bus3),
	m_Impedance12(impedance12), m_Impedance13(impedance13), m_Impedance23(impedance23), 
	m_Admittance(admittance), m_TapRatio1(tapRatio1), m_TapRatio2(tapRatio2), m_TapRatio3(tapRatio3)
{ }

ThreeWindingTransformer::ThreeWindingTransformer()
	: ThreeWindingTransformer(nullptr, nullptr, nullptr, 0, 0, 0, 0, 1, 1, 1)
{ }

//Tag::Tag()
//{
//
//}
//
//Tag::~Tag()
//{
//
//}
