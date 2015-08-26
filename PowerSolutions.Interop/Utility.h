#pragma once

#include <vector>

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

		inline cli::array<Complex>^ MarshalComplexArray(const std::vector<complexd>& value)
		{
			auto arr = gcnew cli::array<Complex>(value.size());
			int i = 0;
			for (auto item : value)
			{
				arr[i] = Complex(item.real(), item.imag());
				i++;
			}
			return arr;
		}

		System::Exception^ TranslateException(const std::exception& ex);

		/*template <class TFunc>
		inline void CRTExceptionBoundary(TFunc&& action)
		{
			try
			{
				action();
			}
			catch (std::exception& ex)
			{
				throw TranslateException(ex);
			}
		}*/
	}
}