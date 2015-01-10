#include "stdafx.h"
#include "Application.h"
#include "ObjectModel.h"
#include "NetworkCase.h"
#include "Solvers.h"
#include "DataManager.h"
#include "resource.h"
#include "Console.h"
#include "ReportGenerator.h"

using namespace AppNamespace;
using namespace AppNamespace::ObjectModel;
using namespace AppNamespace::Solvers;

DataManager::DataManager()
{
	m_CaseInfo = new NetworkCase;
}

DataManager::~DataManager()
{
	if (m_Solver != NULL) delete m_Solver;
	delete m_CaseInfo;
}

void DataManager::Load(const tstring &fileName)
{
	tifstream dataFile;
	Console::WritePrompt(Format(PROCEDURE_LOAD_FILE, fileName.c_str()));
	dataFile.open(fileName);
	if (dataFile.fail())
		throw IOException(Format(ERROR_FILE_OPEN, fileName.c_str()));
	try
	{
		//设置代码页为系统默认值
		dataFile.imbue(locale(""));
		Load(dataFile, false);
	}
	catch (exception&)
	{
		dataFile.close();
		throw;
	}
	dataFile.close();
	Console::WritePrompt(Format(PROMPT_FILE_LOADED, fileName.c_str()));
}

void DataManager::Load(tistream &source, bool interactive)
{
	//状态变量
	tstring buffer;
	size_t pos;
	int LineCounter = 0;
	int NextIndex = NullIndex;
	//初始化
	assert(m_CaseInfo);
	m_CaseInfo->Clear();
	if (m_Solver != NULL)
	{
		delete m_Solver;
		m_Solver = NULL;
	}
	//载入数据
	while (interactive || !source.eof())
	{
		if (interactive) Console::WriteKeyword(Console::Keyword::InputCase);
		getline(source, buffer, _T('\n'));
		if (interactive && buffer == _T("##")) break;	//交互模式下##表示结束
		LineCounter++;
		assert(LineCounter > 0);
		Trim(buffer);
		if (buffer.empty() || buffer[0] == '#')
		{
			//空行或行注释
		} else {
			//解析行
			//示例：
			//Attribute: Name, Test1
			//Bus: 1, Test Bus, 1.05
			pos = find_if(buffer.begin(), buffer.end(), ptr_fun(_istspace)) - buffer.begin();
			if (pos == tstring::npos)
			{
				Console::WritePrompt(Format(WARNING_INVALID_INSTRUCTION, LineCounter, buffer.c_str()));
			}
			else
			{
				//提取指令
				tstring InstructionName = Trim(buffer.substr(0, pos));
				//将剩下的部分作为参数
				buffer.erase(0, pos + 1);
				Trim(buffer);
				//解析指令
#define _CheckKeyword(expression, keyword) else if (Compare(expression, _T(#keyword)) == 0)
				if (InstructionName.empty())
					throw ValidationException(EXCEPTION_MISSING_INSTRUCTION);
				try
				{
					if (false) {}
					_CheckKeyword(InstructionName, Attribute.Locale)
					{
						//设置文件编码
						source.imbue(locale(_t2s(buffer).c_str()));
					}
					_CheckKeyword(InstructionName, Attribute.Version)
					{
						vector <tstring> versionParts;
						//设置案例文件版本
						m_CaseInfo->Version = Version(buffer);
						if (m_CaseInfo->Version.Major > Application::DataFileVersion.Major)
							throw ValidationException(EXCEPTION_DATA_VERSION);
					}
					_CheckKeyword(InstructionName, Attribute.Name)
					{
						//设置案例名称
						m_CaseInfo->Name = Trim(buffer);
					}
					_CheckKeyword(InstructionName, Attribute.Annotation)
					{
						//增加一行注释
						if (!m_CaseInfo->Annotation.empty()) m_CaseInfo->Annotation += '\n';
						m_CaseInfo->Annotation += buffer;
						Console::WritePrompt(Format(PROMPT_ANNOTATION, buffer.c_str()));
					}
					_CheckKeyword(InstructionName, Attribute.BusListCapacity)
					{
						//设置总线列表容量，方便预留内存空间
						//if (!buffer.empty())
							//_CaseInfo->Buses.reserve(stoi(buffer));
					} 
					_CheckKeyword(InstructionName, Attribute.Solver)
					{
						if ((m_Solver = SSSolver::Create(buffer)) == NULL)
							throw ValidationException(Format(EXCEPTION_SOLVER_UNSUPPORTED, buffer.c_str()));
						m_Solver->CaseInfo = this->m_CaseInfo;
					}
					_CheckKeyword(InstructionName, Attribute.MaxIterations)
					{
						if (m_Solver == NULL)
							throw ValidationException(EXCEPTION_SOLVER_UNDEFINED);
						m_Solver->Settings.MaxIterations = stoi(buffer);
					}
					_CheckKeyword(InstructionName, Attribute.MaxDeviation)
					{
						if (m_Solver == NULL)
							throw ValidationException(EXCEPTION_SOLVER_UNDEFINED);
						m_Solver->Settings.MaxDeviationTolerance = stod(buffer);
					}
					_CheckKeyword(InstructionName, Attribute.NodeReorder)
					{
						if (m_Solver == NULL)
							throw ValidationException(EXCEPTION_SOLVER_UNDEFINED);
						m_Solver->Settings.NodeReorder = CBool(buffer);
					}
					_CheckKeyword(InstructionName, NextIndex)
					{
						//试图为下一个元件指定索引
						NextIndex = stoi(buffer);
					}
					else 
					{
						vector<tstring> params(6);
						NetworkObject *c;
						Split(buffer, _T(","), params);
						for_each(params.begin(), params.end(), [](tstring &p){Trim(p); });
						c = NetworkObject::Create(InstructionName);
						if (c == NULL)
						{
							//未知的指令
							Console::WritePrompt(Format(WARNING_INVALID_INSTRUCTION, LineCounter, InstructionName.c_str()));
						} else {
							c->LoadData(params);
							m_CaseInfo->Attach(c);
							if (NextIndex != NullIndex)
							{
								if (IsInstanceOf<Bus>(c))
									throw ValidationException(EXCEPTION_BUS_NEXTINDEX);
								c->Index = NextIndex;
								NextIndex = NullIndex;
							}
						}
					}
				} catch(exception& e) {
					throw FileParseException(InstructionName + _T(":") + ToString(e), LineCounter);
				}
#undef _CheckKeyword
			}
		}
	}
	m_CaseInfo->Validate();
	if (m_Solver == NULL)
	{
		m_Solver = new NRSolver;
		m_Solver->CaseInfo = this->m_CaseInfo;
	}
	Console::WritePrompt(Format(INFO_DATA_LOADED,
		m_CaseInfo->Name.c_str(),
		m_CaseInfo->Buses().size(), m_CaseInfo->Components().size()));
}

void DataManager::WriteReport(const tstring &fileName, bool plainReport)
{
	tofstream reportFile;
	Console::WritePrompt(Format(PROCEDURE_SAVE_FILE, fileName.c_str()));
	reportFile.open(fileName, ios::out | ios::trunc);
	if (reportFile.fail())
		throw IOException(Format(ERROR_FILE_OPEN, fileName.c_str()));
	//reportFile.imbue();
	WriteReport(reportFile, plainReport);
	reportFile.close();
	Console::WritePrompt(Format(PROMPT_FILE_SAVED, fileName.c_str()));
}

inline void DataManager::WriteReport(tostream &dest, bool plainReport)
{
	assert(m_Solver);
	ReportGenerator rep;
	rep.Dest = &dest;
	rep.PlainReport = plainReport;
	rep.ZeroThreshold = m_Solver->SolutionInfo.MaxDeviation;
	rep.GenerateReport(*m_Solver);
}
