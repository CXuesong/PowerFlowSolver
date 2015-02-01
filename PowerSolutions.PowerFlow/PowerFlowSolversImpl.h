/*
PowerSolutions
稳态潮流求解实现（公共部分）。
by  Chen [CXuesong.], 2015
*/

#pragma once

#include "PowerSolutions.h"
#include "PowerFlowObjectModel.h"
#include "PowerFlowSolvers.h"
#include "Utility.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <Eigen/Sparse>

namespace PowerSolutions
{
	namespace PowerFlow
	{
		// 抽象用于完成稳态潮流的解决过程。
		class SolverImpl : public Solver
		{
		protected:	//内部数据
			// 节点的类型。
			enum class NodeType : byte
			{
				PQNode = 0,		//PQ节点。
				PVNode,			//PV节点。
				SlackNode,		//平衡节点。
			};
			class NodeInfo
			{
			public:
				ObjectModel::Bus* Bus;			//母线的组件信息。
				int Index = -1;					//节点索引（Nodes）。
				int SubIndex = -1;				//节点在相应类型的节点列表（PQNodes/PVNodes）中的索引。
				int Degree = 0;					//母线连结出来的分支数量。
				NodeType Type;					//母线的类型。
				//求解变量的缓存值。
				//生成目标解相量之前，
				//对于PQ节点，保存已知的P、Q；
				//对于PV节点，保存已知的P、V。
				//迭代结束后，保存了当前的解V/A/P/Q。
				double Voltage;
				double Angle;					//弧度
				double ActivePowerInjection;
				double ReactivePowerInjection;
				//此内联函数无法放置在cpp中,因为其被 Solver 的派生类所调用，
				//可能引发不可识别的外部函数的错误。
				void ClearPowerInjections()
				{
					ActivePowerInjection = ReactivePowerInjection = 0;
				}
				complexd VoltagePhasor();		//复数形式的电压相量
				void AddPQ(complexd power);
				bool AddPV(double activePower, double voltage);
				bool AddSlack(complexd voltagePhasor);
				NodeInfo(ObjectModel::Bus* bus, NodeType type)
					: Bus(bus), Type(type)
				{ }
				NodeInfo(ObjectModel::Bus* bus)
					: Bus(bus), Type(NodeType::PQNode)
				{ }
			};
			typedef std::vector<NodeInfo*> NodeCollection;
			typedef std::unordered_map<ObjectModel::Bus*, std::shared_ptr<NodeInfo>> NodeDictionary;
			typedef std::unordered_set < std::pair<ObjectModel::Bus*, ObjectModel::Bus*>,
				Utility::UnorderedPairHasher<ObjectModel::Bus*>, Utility::UnorderedPairEqualityComparer<ObjectModel::Bus* >>
				BranchCollection;

			ObjectModel::ExpandedNetworkCase CaseInfo;
			NodeCollection PQNodes;			//参与计算的母线（PQ节点）信息，按照矩阵索引排序。
			NodeCollection PVNodes;			//参与计算的母线（PV节点）信息，按照矩阵索引排序。
			//注意到在NR法中，PQ 节点和 PV 节点的顺序是可以交错的。
			NodeCollection Nodes;			//参与计算的三种节点，按照矩阵索引连续排序，注意平衡节点放在最后。
			std::shared_ptr<NodeInfo> SlackNode;					//平衡节点的信息。
			NodeDictionary BusMapping;				//Bus --> 节点信息
			BranchCollection Branches;				//记录节点连接（支路）(m,n)
			int NodeCount;							//实际参与计算的节点数量。
			int PQNodeCount;							//PQ节点数量。
			int PVNodeCount;							//PV节点数量。
			Eigen::SparseMatrix<complexd> Admittance;	//导纳矩阵的上三角部分（row <= col）。
		protected:
			void MapBuses();					//优化节点编号，决定母线在矩阵中的相对位置。
			void GenerateAdmittance();				//扫描组件列表，形成导纳矩阵，同时决定母线类型。
		protected:
			//约定：以下函数将会按照声明顺序被依次调用。
			virtual void BeforeIterations() = 0;
			virtual double EvalDeviation() = 0;		//计算当前结果的迭代误差。
			virtual bool OnIteration() = 0;
			virtual void AfterIterations() = 0;
			Solution* GenerateSolution(SolutionStatus status, int iterCount, double maxDev);
		public:
			Solution* Solve(ObjectModel::NetworkCase* caseInfo);									// 求解网络的功率潮流分布。返回值表示是否成功收敛。
			SolverImpl();
			virtual ~SolverImpl();
		};
	}
}