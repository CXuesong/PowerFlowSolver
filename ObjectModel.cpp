#include "ObjectModel.h"
#include "NetworkCase.h"

using namespace PowerSolutions;
using namespace PowerSolutions::ObjectModel;

PiEquivalencyParameters::PiEquivalencyParameters(complexd z, complexd y1, complexd y2)
	: m_Impedance(z), m_Admittance1(y1), m_Admittance2(y2)
{ }

////////// 网络对象 //////////
NetworkObject::NetworkObject()
	: m_Index(NullIndex), m_Tag(0)
{ }

NetworkObject::NetworkObject(int index)
	: m_Index(index), m_Tag(0)
{ }

void NetworkObject::Validate() const
{

}

void NetworkObject::Index(int val)
{
	if (m_CaseInfo != NULL)
	{
		if (!m_CaseInfo->NotifyObjectIndexChanged(this, m_Index, val))
		{
			assert(false);
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
	: m_Bus(NullIndex)
{ }

SinglePortComponent::SinglePortComponent(int index)
	: Component(index), m_Bus(NullIndex)
{ }

SinglePortComponent::SinglePortComponent(int index, int bus)
	: Component(index), m_Bus(bus)
{ }

int SinglePortComponent::BusAt(int index) const
{
	assert(index < 1);
	return m_Bus;
}

int SinglePortComponent::PortCount() const
{
	return 1;
}

DoublePortComponent::DoublePortComponent()
	: m_Bus1(NullIndex), m_Bus2(NullIndex)
{ }

DoublePortComponent::DoublePortComponent(int index)
	: Component(index), m_Bus1(NullIndex), m_Bus2(NullIndex)
{ }

DoublePortComponent::DoublePortComponent(int index, int bus1, int bus2)
	: Component(index), m_Bus1(bus1), m_Bus2(bus2)
{ }

int DoublePortComponent::BusAt(int index) const
{
	assert(index < 2);
	return index == 0 ? m_Bus1 : m_Bus2;
}

int DoublePortComponent::PortCount() const
{
	return 2;
}

TriPortComponent::TriPortComponent()
	: m_Bus1(NullIndex), m_Bus2(NullIndex), m_Bus3(NullIndex)
{ }

TriPortComponent::TriPortComponent(int index)
	: Component(index), m_Bus1(NullIndex), m_Bus2(NullIndex), m_Bus3(NullIndex)
{ }

TriPortComponent::TriPortComponent(int index, int bus1, int bus2, int bus3)
	: Component(index), m_Bus1(bus1), m_Bus2(bus2), m_Bus3(NullIndex)
{ }

int TriPortComponent::BusAt(int index) const
{
	assert(index < 3);
	switch (index)
	{
	case 0: return m_Bus1;
	case 1: return m_Bus2;
	case 2: return m_Bus3;
	default:
		assert(false);
		return NullIndex;
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
void Bus::Validate() const
{
	/*if (Index < 0)
		throw InvalidArgumentException(_T("Index"));
		if (CaseInfo != NULL && CaseInfo->Buses().find(Index) != CaseInfo->Buses().end())
		throw ValidationException(Format(ERROR_BUS_INDEX_DUPLICATED, Index));*/
}

////////// 传输线 //////////
Line::Line()
	: m_Impedance(0), m_Admittance(0)
{ }

Line::Line(int index, int bus1, int bus2, complexd impedance, complexd admittance)
	: DoublePortComponent(index, bus1, bus2),
	m_Impedance(impedance), m_Admittance(admittance)
{ }

void Line::Validate() const
{
	//_CheckBusIndex(Bus1);
	//_CheckBusIndex(Bus2);
}

PiEquivalencyParameters Line::PiEquivalency()
{
	complexd y2 = m_Admittance / 2.0;
	return PiEquivalencyParameters(m_Impedance, y2, y2);
}

////////// PV发电机 //////////
PVGenerator::PVGenerator()
	: m_ActivePower(0), m_Voltage(1)
{ }

PVGenerator::PVGenerator(int index, int bus, double activePower, double voltage)
	: SinglePortComponent(index, bus), m_ActivePower(activePower), m_Voltage(voltage)
{ }

void PVGenerator::Validate() const
{
	//_CheckBusIndex(Bus);
}

////////// 平衡节点发电机 //////////
SlackGenerator::SlackGenerator()
	: m_Voltage(1, 0)
{ }

SlackGenerator::SlackGenerator(int index, int bus, complexd voltage)
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

PQLoad::PQLoad(int index, int bus, complexd power)
	: SinglePortComponent(index, bus), m_Power(power)
{ }

void PQLoad::Validate() const
{
	//_CheckBusIndex(Bus);
}

////////// 接地导纳 //////////
ShuntAdmittance::ShuntAdmittance()
{ }

ShuntAdmittance::ShuntAdmittance(int index, int bus, complexd admittance)
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

Transformer::Transformer(int index, int bus1, int bus2, complexd impedance, complexd admittance, complexd tapRatio)
	: DoublePortComponent(index, bus1, bus2),
	m_Impedance(impedance), m_Admittance(admittance), m_TapRatio(tapRatio)
{ }

void Transformer::Validate() const
{
	//_CheckBusIndex(Bus1);
	//_CheckBusIndex(Bus2);
}

PiEquivalencyParameters Transformer::PiEquivalency()
{
	// Z_pi = Z_T / k
	// Y_pi1 = (1 - k) / Z_T
	// Y_pi2 = k * (k - 1) / Z_T
	return PiEquivalencyParameters(
		m_Impedance / m_TapRatio,
		(1.0 - m_TapRatio) / m_Impedance + m_Admittance,
		m_TapRatio * (m_TapRatio - 1.0) / m_Impedance);
}