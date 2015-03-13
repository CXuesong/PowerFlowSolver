#ifndef __POWERSOLUTIONS_H
#define __POWERSOLUTIONS_H

#ifdef _PS_LIB_EXPORT
#define _PS_INTERNAL public
#else
#define _PS_INTERNAL private
#endif

#include <complex>
#include <iostream>

namespace PowerSolutions {
	//类型重定义
	typedef std::complex<double> complexd;
	typedef unsigned char byte;
	namespace PowerFlow
	{
		//调试支持
		void TraceFilePath(char path[]);
	}
}

#endif //__POWERSOLUTIONS_H

