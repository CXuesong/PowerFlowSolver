#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include <cassert>
#include <crtdbg.h>
#if _DEBUG
#include <iostream>
#include <fstream>
#define _PS_TRACE(iosExpression) \
	::PowerSolutions::PowerFlow::TraceFile << iosExpression << std::endl
#define _PS_TRACE_IF(condition, iosExpression) \
	if (condition)  ::PowerSolutions::PowerFlow::TraceFile << iosExpression << std::endl

namespace PowerSolutions
{
	namespace PowerFlow
	{
		extern std::ofstream& TraceFile;
	}
}
#else
#define _PS_TRACE(iosExpression)
#define _PS_TRACE_IF(condition, iosExpression)
#endif
