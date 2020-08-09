#ifndef __POWERSOLUTIONS_H
#define __POWERSOLUTIONS_H

#include <complex>

namespace PowerSolutions {
	//�����ض���
	typedef std::complex<double> complexd;
	typedef unsigned char byte;
	//�����ӿ�

	//Ϊ����ʵ���ṩ���ƹ��ܡ�
	class ICloneable
	{
	public:
		//������ʵ����һ��������
		virtual ICloneable* Clone() const = 0;
	};
}

#endif //__POWERSOLUTIONS_H