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
		ofstream& TraceFile = ofstream(TraceFilePath, ios::out | ios::trunc);
	}
}
#endif
