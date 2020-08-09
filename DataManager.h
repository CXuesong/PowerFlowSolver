//�������ݵ�����
#pragma once

namespace AppNamespace {
	//������
	class NetworkCase;
	namespace Solvers
	{
		class SSSolver;
	}
	using Solvers::SSSolver;
	//�������ݵ����롣
	class DataManager
	{
	private:
		NetworkCase *m_CaseInfo;
		SSSolver *m_Solver = NULL;			//ʹ�õ��������
	public:
		inline NetworkCase* CaseInfo()		//���簸����Ϣ�����ݽ����������˴���
		{
			return m_CaseInfo;
		}
		inline SSSolver* Solver()
		{
			return m_Solver;
		}
	public:
		void Load(const tstring &fileName);					//��ָ�����ļ��м������ݡ�
		void Load(tistream &source, bool interactive);		//��ָ�������м������ݡ�
		void WriteReport(const tstring &fileName, bool plainReport);
		void WriteReport(tostream &dest, bool plainReport);	//��ָ������д��������档
	public:
		~DataManager();
		DataManager();
	};
}