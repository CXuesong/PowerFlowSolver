#include "stdafx.h"
#include "PowerSolutions.h"

using namespace std;
#if _DEBUG
#include <iostream>
#include <fstream>
namespace PowerSolutions
{
	namespace PowerFlow
	{
		ofstream& TraceFile = ofstream("TraceFile.txt", ios::out | ios::trunc);

		void TraceFilePath(char path[])
		{
			TraceFile.open("TraceFile.txt", ios::out | ios::trunc);
		}
	}
}
#endif
