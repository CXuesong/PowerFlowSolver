#pragma once

using namespace System;
using namespace System::Numerics;

namespace PowerSolutions
{
	namespace Interop
	{
		inline Complex MarshalComplex(const complexd value)
		{
			return Complex(value.real(), value.imag());
		}

		inline complexd MarshalComplex(Complex value)
		{
			return complexd(value.Real, value.Imaginary);
		}

		template <class T>
		inline T* MarshalPointer(IntPtr ptr)
		{
			return (T*)(void*)(ptr);
		}

		template <class T>
		inline IntPtr MarshalPointer(T* ptr)
		{
			return IntPtr(ptr);
		}
	}
}