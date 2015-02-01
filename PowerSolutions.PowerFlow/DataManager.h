//管理数据的输入
#pragma once

namespace AppNamespace {
	//引用类
	class NetworkCase;
	namespace Solvers
	{
		class SSSolver;
	}
	using Solvers::SSSolver;
	//管理数据的输入。
	class DataManager
	{
	private:
		NetworkCase *m_CaseInfo;
		SSSolver *m_Solver = NULL;			//使用的求解器。
	public:
		inline NetworkCase* CaseInfo()		//网络案例信息，数据将被加载至此处。
		{
			return m_CaseInfo;
		}
		inline SSSolver* Solver()
		{
			return m_Solver;
		}
	public:
		void Load(const tstring &fileName);					//从指定的文件中加载数据。
		void Load(tistream &source, bool interactive);		//从指定的流中加载数据。
		void WriteReport(const tstring &fileName, bool plainReport);
		void WriteReport(tostream &dest, bool plainReport);	//向指定的流写入分析报告。
	public:
		~DataManager();
		DataManager();
	};
}