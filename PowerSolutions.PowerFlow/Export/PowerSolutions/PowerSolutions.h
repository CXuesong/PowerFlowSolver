#ifndef __POWERSOLUTIONS_H
#define __POWERSOLUTIONS_H

#ifdef _PS_LIB_EXPORT
#define _PS_INTERNAL public
#else
#define _PS_INTERNAL private
#endif

#include <complex>

namespace PowerSolutions {
	//�����ض���
	typedef std::complex<double> complexd;
	typedef unsigned char byte;
}

#if _DEBUG
//����֧��
#include <iostream>
namespace PowerSolutions {
	extern const char TraceFilePath[];
}
#endif

#endif //__POWERSOLUTIONS_H

