#include "stdafx.h"
#include "Prompts.h"

namespace PowerSolutions
{
	namespace Interop
	{
		static Prompts::Prompts()
		{
			m_ResourceManager = gcnew
				::System::Resources::ResourceManager("PowerSolutions.Interop.Prompts",
				Prompts::typeid->Assembly);
		}
	}
}