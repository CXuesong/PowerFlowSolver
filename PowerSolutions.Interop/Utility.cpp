#include "stdafx.h"
#include "Utility.h"
#include "Prompts.h"
#include <PowerSolutions/Exceptions.h>

using namespace System;
using namespace std;

namespace PowerSolutions
{
	namespace Interop
	{
		public enum class ExceptionCode : unsigned int
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

		public ref class PSException
			: System::Exception
		{
		private:
			ExceptionCode m_Code;
			String^ m_Message;
		public:
			// 获取异常的数值表示形式。
			property ExceptionCode Code
			{
				ExceptionCode get() { return m_Code; }
			}
			property String^ Message
			{
				String^ get() override { return m_Message; }
			}
		protected:
		internal:
			PSException(const PowerSolutions::Exception& native)
				: m_Code(ExceptionCode(native.Code()))
			{
				m_Message = Prompts::ResourceManager->GetString(L"ExceptionCode." + m_Code.ToString());
				if (m_Message == nullptr) m_Message = String::Format(Prompts::PowerSolutionException, m_Code);
			}
			PSException(const std::exception& native)
			{
				m_Message = String::Format(
					Prompts::PowerSolutionCRTException,
					gcnew String(typeid(native).name()),
					gcnew String(native.what()));
			}
		};

		System::Exception^ TranslateException(const exception& nativeEx)
		{
			auto psex = dynamic_cast<const PowerSolutions::Exception*>(&nativeEx);
			if (psex == nullptr)
			{
				return gcnew PSException(nativeEx);
			} else {
				return gcnew PSException(*psex);
				/*switch (psex->Code())
				{
				case ExceptionCode::ArgumentNull:
					return gcnew ArgumentNullException(nullptr, inner);
					break;
				case ExceptionCode::ArgumentOutOfRange:
					return gcnew ArgumentOutOfRangeException(nullptr, inner);
					break;
				case ExceptionCode::NotSupported:
					return gcnew NotSupportedException(nullptr, inner);
					break;
				}
				switch (ExceptionCode((int)psex->Code() & (int)(ExceptionCode::ExceptionClassMask)))
				{
				case ExceptionCode::InvalidArgument:
					return gcnew ArgumentException(nullptr, inner);
					break;
				case ExceptionCode::InvalidOperation:
					return gcnew InvalidOperationException(nullptr, inner);
					break;
				case ExceptionCode::Validation:
					return gcnew InvalidOperationException(nullptr, inner);
					break;
				}*/
			}
		}
	}
}
