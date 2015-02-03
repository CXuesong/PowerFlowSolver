// PowerSolutions.Interop.h
#pragma once

#include "Utility.h"

using namespace System;
using namespace System::Numerics;

#define _NATIVE ::PowerSolutions::
#define _NATIVE_OM ::PowerSolutions::ObjectModel::
#define _NATIVE_PF ::PowerSolutions::PowerFlow::

#define _WRAP_PROPERTY(name, type, marshaller) \
		property type name { \
			type get() { return marshaller(nativeObject->name()); } \
			void set(type value) { nativeObject->name(marshaller(value)); } \
		}
#define _WRAP_PROPERTY_READONLY(name, type, marshaller) \
		property type name { \
			type get() { return marshaller(nativeObject->name()); } \
		}
#define _WRAP_STRUCT_INIT(name, marshaller) \
		this->name = marshaller(native.name())

namespace PowerSolutions
{
	namespace Interop
	{
		namespace ObjectModel
		{
		}

		namespace PowerFlow
		{
		}
	}
}

using namespace PowerSolutions;
using namespace PowerSolutions::Interop::ObjectModel;
using namespace PowerSolutions::Interop::PowerFlow;
