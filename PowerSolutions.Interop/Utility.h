#pragma once

#include <PowerSolutions/PowerSolutions.h>

using namespace System;
using namespace System::Numerics;

namespace PowerSolutions
{
	namespace Interop
	{
		Complex MarshalComplex(const complexd value)
		{
			return Complex(value.real(), value.imag());
		}

		complexd MarshalComplex(Complex value)
		{
			return complexd(value.Real, value.Imaginary);
		}

		template <class T>
		T* MarshalPointer(IntPtr ptr)
		{
			return (T*)(void*)(ptr);
		}

		template <class T>
		IntPtr MarshalPointer(T* ptr)
		{
			return IntPtr(ptr);
		}
	}
}