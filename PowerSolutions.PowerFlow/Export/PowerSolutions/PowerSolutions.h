#ifndef __POWERSOLUTIONS_H
#define __POWERSOLUTIONS_H

#ifdef _PS_LIB_EXPORT
#define _PS_INTERNAL public
#else
#define _PS_INTERNAL private
#endif

#include <complex>
#include <iostream>
#include <memory>

namespace PowerSolutions {
	//类型重定义
	typedef std::complex<double> complexd;
	typedef unsigned char byte;
	namespace PowerFlow
	{
		//调试支持
		void TraceFilePath(char path[]);
	}

	//强类型枚举的按位运算支持。
	template<typename E>
	struct enable_bitmask_operators{
		static const bool enable = false;
	};

	template<typename E>
	typename std::enable_if<enable_bitmask_operators<E>::enable, E>::type
		operator|(E lhs, E rhs)
	{
		typedef typename std::underlying_type<E>::type underlying;
		return static_cast<E>(static_cast<underlying>(lhs) |
			static_cast<underlying>(rhs));
	}

	template<typename E>
	typename std::enable_if<enable_bitmask_operators<E>::enable, E>::type
		operator&(E lhs, E rhs)
	{
		typedef typename std::underlying_type<E>::type underlying;
		return static_cast<E>(static_cast<underlying>(lhs) &
			static_cast<underlying>(rhs));
	}

}

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

#endif //__POWERSOLUTIONS_H

