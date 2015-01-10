#include "Exceptions.h"

using namespace std;
using namespace PowerSolutions;

Exception::Exception()
	: m_Code(ExceptionCode::None)
{ }

Exception::Exception(ExceptionCode code)
	: m_Code(code)
{ }
