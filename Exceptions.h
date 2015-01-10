#ifndef __POWERSOLUTIONS_POWERFLOW_EXCEPTIONS_H
#define __POWERSOLUTIONS_POWERFLOW_EXCEPTIONS_H

#include <stdexcept>

namespace PowerSolutions {
	enum class ExceptionCode : unsigned char
	{
		None = 0,
		InvalidArgument,
		InvalidOperation,
		ArgumentNull,
		ArgumentOutOfRange
	};
	class Exception : public std::exception
	{
	private:
		ExceptionCode m_Code;
	public:
		ExceptionCode Code() const { return m_Code; }
	public:
		Exception();
		Exception(ExceptionCode code);
	};
}

#endif //__POWERSOLUTIONS_POWERFLOW_EXCEPTIONS_H