#ifndef __POWERSOLUTIONS_POWERFLOW_EXCEPTIONS_H
#define __POWERSOLUTIONS_POWERFLOW_EXCEPTIONS_H

#include <stdexcept>

namespace PowerSolutions {
	enum class ExceptionCode : unsigned int
	{
		None = 0x00000000,
		ExceptionClassMask = 0xFFFF0000,
		InvalidArgument = 0x00010000,
		ArgumentNull = InvalidArgument + 1,
		ArgumentOutOfRange = InvalidArgument + 2,
		InvalidOperation = 0x00020000,
		NotSupported = InvalidOperation + 1,
		Validation = 0x00030000,
		VoltageMismatch = Validation + 1,
		SlackBus = Validation + 2,
	};
	class Exception : public std::exception
	{
	private:
		ExceptionCode m_Code;
		char m_What[64];
	public:
		ExceptionCode Code() const { return m_Code; }
		virtual const char* what() const override { return m_What; }
	public:
		Exception();
		Exception(ExceptionCode code);
	};
}

#endif //__POWERSOLUTIONS_POWERFLOW_EXCEPTIONS_H
