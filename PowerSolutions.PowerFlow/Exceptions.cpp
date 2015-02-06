#include "stdafx.h"
#include "Exceptions.h"
#include <cstdio>

using namespace std;
using namespace PowerSolutions;

Exception::Exception()
	: m_Code(ExceptionCode::None)
{ }

Exception::Exception(ExceptionCode code)
	: m_Code(code)
{ 
	sprintf(m_What, "PowerSolutions.Exception, Code=0x%X.", code);
}
