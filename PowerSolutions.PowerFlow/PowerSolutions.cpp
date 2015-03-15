#include "stdafx.h"
#include "PowerSolutions.h"
#include <iostream>
#include <fstream>

using namespace std;
namespace PowerSolutions
{
	namespace PowerFlow
	{
#if _DEBUG
		ofstream& TraceFile = ofstream("TraceFile.txt", ios::out | ios::trunc);

		void TraceFilePath(char path[])
		{
			TraceFile.open("TraceFile.txt", ios::out | ios::trunc);
		}
#else
		void TraceFilePath(char path[])
		{

		}
#endif
	}
}
