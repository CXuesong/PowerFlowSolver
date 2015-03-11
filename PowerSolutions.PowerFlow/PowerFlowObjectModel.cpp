#include "stdafx.h"
#include "PowerFlowObjectModel.h"
#include "NetworkCase.h"
#include "Exceptions.h"
#include <algorithm>

using namespace PowerSolutions;
using namespace std;

namespace PowerSolutions
{
	namespace ObjectModel
	{
		////////// 传输线 //////////
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

		void Line::BuildAdmittanceInfo(PrimitiveNetwork* pNetwork) const
		{
			pNetwork->AddPi(Bus1(), Bus2(), this->PiEquivalency());
		}

		////////// PV发电机 //////////
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

		////////// 平衡节点发电机 //////////
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

		////////// PQ负载 //////////
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

		////////// 接地导纳 //////////
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

		////////// 变压器 //////////
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

		int ThreeWindingTransformer::ChildrenCount() const
		{
			return 4;
		}

		NetworkObject* ThreeWindingTransformer::ChildAt(int index) const
		{
			switch (index)
			{
			case 0: return m_CommonBus.get();
			case 1: return m_Transformer1.get();
			case 2: return m_Transformer2.get();
			case 3: return m_Transformer3.get();
			default: assert(false); return nullptr;
			}
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
			//三绕组变压器模型
			//                  /---1:Tap2---Z2--- 2
			// 1 ---Z1---Tap1:1-
			//    |             \---1:Tap3---Z3--- 3
			//    |
			//    Y
			//    |
			//双绕组变压器模型
			// 1 ---Z---Tap:1--- 2
			//    |
			//    Y
			//    |
			assert(Bus1() != nullptr);
			//设置母线
			m_CommonBus->InitialVoltage(Bus1()->InitialVoltage());
			//设置变压器
			m_Transformer1->Bus1(Bus1());
			m_Transformer2->Bus1(Bus2());
			m_Transformer3->Bus1(Bus3());
			m_Transformer1->Bus2(m_CommonBus.get());
			m_Transformer2->Bus2(m_CommonBus.get());
			m_Transformer3->Bus2(m_CommonBus.get());
			//注意此处的阻值为折算至一次侧的值。
			auto z1 = Impedance1(),
				z2 = Impedance2() / pow((m_TapRatio1 * m_TapRatio2), 2),
				z3 = Impedance3() / pow((m_TapRatio1 * m_TapRatio3), 2),
				y1 = m_Admittance;
			//'需要将折算到一次侧的二次、三次侧绕组参数折回对应的电压等级
			//TODO 使用更为精确的比较策略（考虑标幺值引起的缩放）
			if (abs(z1.imag()) < 1e-10) z1.imag(1e-10);
			if (abs(z2.imag()) < 1e-10) z2.imag(1e-10);
			if (abs(z3.imag()) < 1e-10) z3.imag(1e-10);
			m_Transformer1->Impedance(z1);
			m_Transformer1->Admittance(y1);
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

		void ThreeWindingTransformer::OnExpand()
		{
			//在参与计算前，重新计算子级参数。
			UpdateChildren();
		}
	}
}