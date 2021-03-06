﻿#include "stdafx.h"
#include "PowerFlowObjectModel.h"
#include "NetworkCase.h"
#include "PrimitiveNetwork.h"
#include "Exceptions.h"
#include "PowerFlowSolvers.h"
#include "PowerFlowSolution.h"
#include <algorithm>

using namespace PowerSolutions;
using PowerSolutions::PowerFlow::PrimitiveSolution;
using PowerSolutions::PowerFlow::ComponentFlowSolution;
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

		void PVGenerator::BuildNodeInfo(PrimitiveNetwork* pNetwork) const
		{
			SinglePortComponent::BuildNodeInfo(pNetwork);
			pNetwork->AddPV(Bus1(), m_ActivePower, m_Voltage);
		}

		ComponentFlowSolution PVGenerator::EvalComponentFlow(const PrimitiveSolution& solution) const
		{
			//PV发电机无法确定自身发出的无功功率到底是多少，尤其是当一个节点上链接了不止一台发电机时。
			return ComponentFlowSolution::Unconstrained();
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

		void SlackGenerator::BuildNodeInfo(PrimitiveNetwork* pNetwork) const
		{
			SinglePortComponent::BuildNodeInfo(pNetwork);
			pNetwork->AddSlack(Bus1(), m_Voltage);
		}

		ComponentFlowSolution SlackGenerator::EvalComponentFlow(const PrimitiveSolution& solution) const
		{
			//仅凭一台平衡机自身无法确定出力。
			return ComponentFlowSolution::Unconstrained();
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

		void PQLoad::BuildNodeInfo(PrimitiveNetwork* pNetwork) const
		{
			SinglePortComponent::BuildNodeInfo(pNetwork);
			pNetwork->AddPQ(Bus1(), -m_Power);
		}

		ComponentFlowSolution PQLoad::EvalComponentFlow(const PrimitiveSolution& solution) const
		{
			//流入负载端口的功率，即端口的注入功率。
			return ComponentFlowSolution({ m_Power }, 0);
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

		ComponentFlowSolution ShuntAdmittance::EvalComponentFlow(const PrimitiveSolution& solution) const
		{
			//  |   从 Bus 抽出 功率 [1]
			//  |
			//  |
			//  |   注入 GND [0]
			//注意此部分功率不应累加至出力功率
			// S = U^2 * conj(Y)
			//BUG FIXED:电压没有取平方
			auto v = solution.NodeStatus(Bus1()).Voltage();
			auto p = v  * v * conj(m_Admittance);
			return ComponentFlowSolution({ -p }, p);
		}

		void ShuntAdmittance::BuildAdmittanceInfo(PrimitiveNetwork* pNetwork) const
		{
			pNetwork->AddShunt(this->Bus1(), m_Admittance);
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

		PowerFlow::ComponentFlowSolution Transformer::EvalComponentFlow(const PowerFlow::PrimitiveSolution& solution) const
		{
			//注意，计算支路功率时
			//对于变压器，不能将π型等值电路两侧的接地导纳拆开计算。
			//只能按照Γ型等值电路进行计算。
			auto p = DoublePortComponent::EvalComponentFlow(solution);
			auto v = solution.NodeStatus(Bus1()).Voltage();
			//修正导纳功率。
			p.PowerShunt(v * v * conj(m_Admittance));
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
			m_CommonBus(new Bus(this, 1)), m_Transformer1(new Transformer()), m_Transformer2(new Transformer()),
			m_Transformer3(new Transformer())
		{
			UpdateChildren();
		}

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

		// TODO 在某一分支参数变化时，仅更新此分支的数据。
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
			// 考虑此函数可能会被 CloneInstance() 间接调用。
			// 此时 m_Bus1 == nullptr
			//assert(Bus1() != nullptr);
			//设置变压器
			m_Transformer1->Bus1(Bus1());
			m_Transformer2->Bus1(Bus2());
			m_Transformer3->Bus1(Bus3());
			m_Transformer1->Bus2(m_CommonBus.get());
			m_Transformer2->Bus2(m_CommonBus.get());
			m_Transformer3->Bus2(m_CommonBus.get());
			//注意此处的阻值为折算至一次侧的值。
			auto z1 = Impedance1(), z2 = Impedance2(), z3 = Impedance3();
			//需要将折算到一次侧的二次、三次侧绕组参数折回对应的电压等级
			//TODO 使用更为精确的比较策略（考虑标幺值引起的缩放）
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

		void ThreeWindingTransformer::BuildNodeInfo(PrimitiveNetwork* pNetwork) const
		{
			Component::BuildNodeInfo(pNetwork);
			pNetwork->ClaimBranch(Bus1(), m_CommonBus.get(), this);
			pNetwork->ClaimBranch(Bus2(), m_CommonBus.get(), this);
			pNetwork->ClaimBranch(Bus3(), m_CommonBus.get(), this);
		}

		void ThreeWindingTransformer::BuildAdmittanceInfo(PrimitiveNetwork* pNetwork) const
		{
			//在参与计算前，重新计算子级参数。
			m_Transformer1->BuildAdmittanceInfo(pNetwork);
			m_Transformer2->BuildAdmittanceInfo(pNetwork);
			m_Transformer3->BuildAdmittanceInfo(pNetwork);
			pNetwork->AddShunt(m_CommonBus.get(), m_Admittance);
		}

		ComponentFlowSolution ThreeWindingTransformer::EvalComponentFlow(const PrimitiveSolution& solution) const
		{
			//参阅 ThreeWindingTransformer::UpdateChildren 中的三绕组变压器模型。
			auto power1 = m_Transformer1->EvalComponentFlow(solution);
			auto power2 = m_Transformer2->EvalComponentFlow(solution);
			auto power3 = m_Transformer3->EvalComponentFlow(solution);
			//接地功率应该主要是由变压器1产生。
			//潮流不收敛，或者导纳很小的时候，以下断言很可能失败。
			//assert(abs(power2[0]) / abs(power1[0]) < 1e-5);
			//assert(abs(power3[0]) / abs(power1[0]) < 1e-5);
			return ComponentFlowSolution({ power1.PowerInjections(0), 
				power2.PowerInjections(0), power3.PowerInjections(0) }, 
				power1.PowerShunt());
		}
	}
}