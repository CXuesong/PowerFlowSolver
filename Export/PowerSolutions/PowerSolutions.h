#ifndef __POWERSOLUTIONS_H
#define __POWERSOLUTIONS_H

#include <complex>

namespace PowerSolutions {
	//类型重定义
	typedef std::complex<double> complexd;
	typedef unsigned char byte;
	//基本接口

	//为类型实例提供复制功能。
	class ICloneable
	{
	public:
		//创建此实例的一个副本。
		virtual ICloneable* Clone() const = 0;
	};
}

#endif //__POWERSOLUTIONS_H