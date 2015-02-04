#pragma once

#include "PowerSolutions.Interop.h"

namespace PowerSolutions
{
	namespace Interop
	{
		private ref class Prompts
		{
		private:
			static ::System::Resources::ResourceManager^ m_ResourceManager = nullptr;
		public:
			static property ::System::Resources::ResourceManager^ ResourceManager
			{
				::System::Resources::ResourceManager^ get() { return m_ResourceManager; }
			}
			static Prompts();
		public:
			static property String^ PowerSolutionException
			{
				String^ get() { return ResourceManager->GetString("PowerSolutionException"); }
			}
			static property String^ PowerSolutionCRTException
			{
				String^ get() { return ResourceManager->GetString("PowerSolutionCRTException"); }
			}
			static property String^ NotIterating
			{
				String^ get() { return ResourceManager->GetString("NotIterating"); }
			}
		private:
			Prompts() {};
		};
	}
}

