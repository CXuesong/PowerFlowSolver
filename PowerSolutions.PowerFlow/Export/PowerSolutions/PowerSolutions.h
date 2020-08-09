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
	//�����ض���
	typedef std::complex<double> complexd;
	typedef unsigned char byte;
	namespace PowerFlow
	{
		//����֧��
		void TraceFilePath(char path[]);
	}
}

#endif //__POWERSOLUTIONS_H

