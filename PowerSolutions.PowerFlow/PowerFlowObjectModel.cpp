#include "stdafx.h"
#include "PowerFlowObjectModel.h"
#include "NetworkCase.h"
#include "PrimitiveNetwork.h"
#include "Exceptions.h"
#include <algorithm>

using namespace PowerSolutions;
using namespace std;

namespace PowerSolutions
{
	namespace ObjectModel
	{
		////////// ������ //////////
		Line::Line()
			: Line(nullptr, nullptr, 0, 0)
		{ }

		Line::Line(complexd impedance, complexd admittance)
			: Line(nullptr, nullptr, impedance, admittance)
		{ }

		Line::Line(Bus *bus1, Bus *bus2, complexd impedance, complexd admittance)
			: DoublePortComponent(bus1, bus2), m_Impedance(impedance), m_Admittance(admittance)
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

		Line* Line::Create(Bus *bus1, Bus *bus2, complexd impedance, complexd admittance)
		{
			auto newInst = new Line(impedance, admittance);
			newInst->Bus1(bus1);
			newInst->Bus2(bus2);
			return newInst;
		}

		NetworkObject* Line::CloneInstance() const
		{
			return new Line(m_Impedance, m_Admittance);
		}

		////////// PV����� //////////
		PVGenerator::PVGenerator()
			: PVGenerator(nullptr, 0, 1)
		{ }

		PVGenerator::PVGenerator(double activePower, double voltage)
			: PVGenerator(nullptr, activePower, voltage)
		{ }

		PVGenerator::PVGenerator(Bus *bus1, double activePower, double voltage)
			: SinglePortComponent(bus1), m_ActivePower(activePower), m_Voltage(voltage)
		{ }

		void PVGenerator::Validate() const
		{
			//_CheckBusIndex(Bus);
		}

		PVGenerator* PVGenerator::Create(Bus *bus1, double activePower, double voltage)
		{
			auto newInst = new PVGenerator(activePower, voltage);
			newInst->Bus1(bus1);
			return newInst;
		}

		NetworkObject* PVGenerator::CloneInstance() const
		{
			return new PVGenerator(m_ActivePower, m_Voltage);
		}

		void PVGenerator::BuildNodeInfo(PrimitiveNetwork* pNetwork)
		{
			SinglePortComponent::BuildNodeInfo(pNetwork);
			pNetwork->AddPV(Bus1(), m_ActivePower, m_Voltage);
		}

		vector<complexd> PVGenerator::EvalPowerInjection(PrimitiveNetwork* pNetwork) const
		{
			//PV������޷�ȷ�����������޹����ʵ����Ƕ��٣������ǵ�һ���ڵ��������˲�ֹһ̨�����ʱ��
			return{ };
		}

		////////// ƽ��ڵ㷢��� //////////
		SlackGenerator::SlackGenerator()
			: SlackGenerator(nullptr, 1)
		{ }

		SlackGenerator::SlackGenerator(complexd voltage)
			: SlackGenerator(nullptr, voltage)
		{ }

		SlackGenerator::SlackGenerator(Bus *bus1, complexd voltage)
			: SinglePortComponent(bus1), m_Voltage(voltage)
		{ }

		void SlackGenerator::Validate() const
		{
			//_CheckBusIndex(Bus);
		}

		SlackGenerator* SlackGenerator::Create(Bus *bus1, complexd voltage)
		{
			auto newInst = new SlackGenerator(voltage);
			newInst->Bus1(bus1);
			return newInst;
		}

		NetworkObject* SlackGenerator::CloneInstance() const
		{
			return new SlackGenerator(m_Voltage);
		}

		void SlackGenerator::BuildNodeInfo(PrimitiveNetwork* pNetwork)
		{
			SinglePortComponent::BuildNodeInfo(pNetwork);
			pNetwork->AddSlack(Bus1(), m_Voltage);
		}

		vector<complexd> SlackGenerator::EvalPowerInjection(PrimitiveNetwork* pNetwork) const
		{
			//��ƾһ̨ƽ��������޷�ȷ��������
			return{};
		}

		////////// PQ���� //////////
		PQLoad::PQLoad()
			: PQLoad(nullptr, 0)
		{ }

		PQLoad::PQLoad(complexd power)
			: PQLoad(nullptr, power)
		{ }

		PQLoad::PQLoad(Bus *bus1, complexd power)
			: SinglePortComponent(bus1), m_Power(power)
		{ }

		void PQLoad::Validate() const
		{
			//_CheckBusIndex(Bus);
		}

		PQLoad* PQLoad::Create(Bus *bus1, complexd power)
		{
			auto newInst = new PQLoad(power);
			newInst->Bus1(bus1);
			return newInst;
		}

		NetworkObject* PQLoad::CloneInstance() const
		{
			return new PQLoad(m_Power);
		}

		void PQLoad::BuildNodeInfo(PrimitiveNetwork* pNetwork)
		{
			SinglePortComponent::BuildNodeInfo(pNetwork);
			pNetwork->AddPQ(Bus1(), -m_Power);
		}

		vector<complexd> PQLoad::EvalPowerInjection(PrimitiveNetwork* pNetwork) const
		{
			return { 0, -m_Power };
		}

		////////// �ӵص��� //////////
		ShuntAdmittance::ShuntAdmittance()
			: ShuntAdmittance(0)
		{ }

		ShuntAdmittance::ShuntAdmittance(complexd admittance)
			: ShuntAdmittance(nullptr, admittance)
		{ }

		ShuntAdmittance::ShuntAdmittance(Bus *bus1, complexd admittance)
			: SinglePortComponent(bus1), m_Admittance(admittance)
		{ }

		void ShuntAdmittance::Validate() const
		{
			//_CheckBusIndex(Bus);
		}

		ShuntAdmittance* ShuntAdmittance::Create(Bus *bus1, complexd admittance)
		{
			auto newInst = new ShuntAdmittance(admittance);
			newInst->Bus1(bus1);
			return newInst;
		}

		NetworkObject* ShuntAdmittance::CloneInstance() const
		{
			return new ShuntAdmittance(m_Admittance);
		}

		vector<complexd> ShuntAdmittance::EvalPowerInjection(PrimitiveNetwork* pNetwork) const
		{
			//  |   �� Bus ��� ���� [1]
			//  |
			//  |
			//  |   ע�� GND [0]
			//ע��˲��ֹ��ʲ�Ӧ�ۼ�����������
			// S = U^2 * conj(Y)
			//BUG FIXED:��ѹû��ȡƽ��
			auto v = pNetwork->BusMapping[Bus1()]->Voltage;
			auto p = v  * v * conj(m_Admittance);
			return{ p, -p };
		}

		void ShuntAdmittance::BuildAdmittanceInfo(PrimitiveNetwork* pNetwork)
		{
			pNetwork->AddShunt(this->Bus1(), m_Admittance);
		}

		////////// ��ѹ�� //////////
		Transformer::Transformer()
			: Transformer(nullptr, nullptr, 0, 0, 1)
		{ }

		Transformer::Transformer(complexd impedance, complexd admittance, complexd tapRatio)
			: Transformer(nullptr, nullptr, impedance, admittance, tapRatio)
		{ }

		Transformer::Transformer(Bus *bus1, Bus *bus2, complexd impedance, complexd tapRatio)
			: Transformer(bus1, bus2, impedance, 0, tapRatio)
		{ }

		Transformer::Transformer(Bus *bus1, Bus *bus2, complexd impedance, complexd admittance, complexd tapRatio)
			: DoublePortComponent(bus1, bus2), m_Impedance(impedance), m_Admittance(admittance), m_TapRatio(tapRatio)
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

		std::vector<complexd> Transformer::EvalPowerInjection(PrimitiveNetwork* pNetwork) const
		{
			//ע�⣬����֧·����ʱ
			//���ڱ�ѹ�������ܽ����͵�ֵ��·����Ľӵص��ɲ𿪼��㡣
			//ֻ�ܰ��զ��͵�ֵ��·���м��㡣
			auto p = DoublePortComponent::EvalPowerInjection(pNetwork);
			auto v = pNetwork->BusMapping[Bus1()]->Voltage;
			p[0] = v * v * conj(m_Admittance);
			return p;
		}

		Transformer* Transformer::Create(Bus *bus1, Bus *bus2, complexd impedance, complexd admittance, complexd tapRatio)
		{
			auto newInst = new Transformer(impedance, admittance, tapRatio);
			newInst->Bus1(bus1);
			newInst->Bus2(bus2);
			return newInst;
		}

		NetworkObject* Transformer::CloneInstance() const
		{
			return new Transformer(m_Impedance, m_Admittance, m_TapRatio);
		}

		Bus* ThreeWindingTransformer::ChildBusAt(int index)  const
		{
			assert(index == 0);
			assert(m_CommonBus != nullptr);
			m_CommonBus->InitialVoltage(Bus1()->InitialVoltage());
			return m_CommonBus.get();
		}

		int ThreeWindingTransformer::ChildBusCount()  const
		{
			return 1;
		}

		ThreeWindingTransformer::ThreeWindingTransformer()
			: ThreeWindingTransformer(nullptr, nullptr, nullptr, 0, 0, 0, 0, 1, 1, 1)
		{ }

		ThreeWindingTransformer::ThreeWindingTransformer(
			complexd impedance12, complexd impedance13, complexd impedance23,
			complexd admittance, complexd tapRatio1, complexd tapRatio2, complexd tapRatio3)
			: ThreeWindingTransformer(nullptr, nullptr, nullptr,
			impedance12, impedance13, impedance23, admittance, tapRatio1, tapRatio2, tapRatio3)
		{ }

		ThreeWindingTransformer::ThreeWindingTransformer(Bus *bus1, Bus *bus2, Bus *bus3,
			complexd impedance12, complexd impedance13, complexd impedance23,
			complexd tapRatio1, complexd tapRatio2, complexd tapRatio3)
			: ThreeWindingTransformer(bus1, bus2, bus3,
			impedance12, impedance13, impedance23, 0, tapRatio1, tapRatio2, tapRatio3)
		{ }

		ThreeWindingTransformer::ThreeWindingTransformer(Bus *bus1, Bus *bus2, Bus *bus3,
			complexd impedance12, complexd impedance13, complexd impedance23,
			complexd admittance, complexd tapRatio1, complexd tapRatio2, complexd tapRatio3)
			: TriPortComponent(bus1, bus2, bus3),
			m_Impedance12(impedance12), m_Impedance13(impedance13), m_Impedance23(impedance23),
			m_Admittance(admittance), m_TapRatio1(tapRatio1), m_TapRatio2(tapRatio2), m_TapRatio3(tapRatio3),
			m_CommonBus(new Bus(-1)), m_Transformer1(new Transformer()), m_Transformer2(new Transformer()),
			m_Transformer3(new Transformer())
		{ }

		ThreeWindingTransformer* ThreeWindingTransformer::Create(Bus *bus1, Bus *bus2, Bus *bus3, complexd impedance12, complexd impedance13, complexd impedance23, complexd admittance, complexd tapRatio1, complexd tapRatio2, complexd tapRatio3)
		{
			auto newInst = new ThreeWindingTransformer(impedance12, impedance13, impedance23, admittance, tapRatio1, tapRatio2, tapRatio3);
			newInst->Bus1(bus1);
			newInst->Bus2(bus2);
			newInst->Bus3(bus3);
			return newInst;
		}

		NetworkObject* ThreeWindingTransformer::CloneInstance() const
		{
			return new ThreeWindingTransformer(m_Impedance12, m_Impedance13, m_Impedance23,
				m_Admittance, m_TapRatio1, m_TapRatio2, m_TapRatio3);
		}

		void ThreeWindingTransformer::UpdateChildren()
		{
			//�������ѹ��ģ��
			//                  /---1:Tap2---Z2--- 2
			// 1 ---Z1---Tap1:1-
			//    |             \---1:Tap3---Z3--- 3
			//    |
			//    Y
			//    |
			//˫�����ѹ��ģ��
			// 1 ---Z---Tap:1--- 2
			//    |
			//    Y
			//    |
			assert(Bus1() != nullptr);
			//���ñ�ѹ��
			m_Transformer1->Bus1(Bus1());
			m_Transformer2->Bus1(Bus2());
			m_Transformer3->Bus1(Bus3());
			m_Transformer1->Bus2(m_CommonBus.get());
			m_Transformer2->Bus2(m_CommonBus.get());
			m_Transformer3->Bus2(m_CommonBus.get());
			//ע��˴�����ֵΪ������һ�β��ֵ��
			auto a = (m_Impedance12 + m_Impedance23 - m_Impedance13) / 2.0;
			auto z1 = Impedance1(), z2 = Impedance2(), z3 = Impedance3();
			//'��Ҫ�����㵽һ�β�Ķ��Ρ����β���������ۻض�Ӧ�ĵ�ѹ�ȼ�
			//TODO ʹ�ø�Ϊ��ȷ�ıȽϲ��ԣ����Ǳ���ֵ��������ţ�
			const auto MinImpedance = 1e-8;
			if (abs(z1.imag()) < MinImpedance) z1.imag(MinImpedance * 10);
			if (abs(z2.imag()) < MinImpedance) z2.imag(MinImpedance * 10);
			if (abs(z3.imag()) < MinImpedance) z3.imag(MinImpedance * 10);
			z2 /= pow((m_TapRatio1 * m_TapRatio2), 2);
			z3 /= pow((m_TapRatio1 * m_TapRatio3), 2);
			m_Transformer1->Impedance(z1);
			m_Transformer1->Admittance(0);
			m_Transformer1->TapRatio(m_TapRatio1);
			m_Transformer2->Impedance(z2);
			m_Transformer2->Admittance(0);
			m_Transformer2->TapRatio(1.0 / m_TapRatio2);
			m_Transformer3->Impedance(z3);
			m_Transformer3->Admittance(0);
			m_Transformer3->TapRatio(1.0 / m_TapRatio3);
		}

		void ThreeWindingTransformer::Validate() const
		{
		}

		void ThreeWindingTransformer::BuildNodeInfo(PrimitiveNetwork* pNetwork)
		{
			pNetwork->ClaimBranch(Bus1(), m_CommonBus.get());
			pNetwork->ClaimBranch(Bus2(), m_CommonBus.get());
			pNetwork->ClaimBranch(Bus3(), m_CommonBus.get());
		}

		void ThreeWindingTransformer::BuildAdmittanceInfo(PrimitiveNetwork* pNetwork)
		{
			//�ڲ������ǰ�����¼����Ӽ�������
			//TODO �Ƴ��Ӷ���ֱ�ӱ�Ϊ�����㡣
			UpdateChildren();
			m_Transformer1->BuildAdmittanceInfo(pNetwork);
			m_Transformer2->BuildAdmittanceInfo(pNetwork);
			m_Transformer3->BuildAdmittanceInfo(pNetwork);
			pNetwork->AddShunt(m_CommonBus.get(), m_Admittance);
		}

		vector<complexd> ThreeWindingTransformer::EvalPowerInjection(PrimitiveNetwork* pNetwork) const
		{
			//���� ThreeWindingTransformer::UpdateChildren �е��������ѹ��ģ�͡�
			auto power1 = m_Transformer1->EvalPowerInjection(pNetwork);
			auto power2 = m_Transformer2->EvalPowerInjection(pNetwork);
			auto power3 = m_Transformer3->EvalPowerInjection(pNetwork);
			//�ӵع���Ӧ����Ҫ���ɱ�ѹ��1������
			//���������������ߵ��ɺ�С��ʱ�����¶��Ժܿ���ʧ�ܡ�
			//assert(abs(power2[0]) / abs(power1[0]) < 1e-5);
			//assert(abs(power3[0]) / abs(power1[0]) < 1e-5);
			return{ power1[0], power1[1], power2[1], power3[1] };
		}
	}
}