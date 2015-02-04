// PowerSolutions.Interop.h
#pragma once

#include "Utility.h"

using namespace System;
using namespace System::Numerics;
using namespace System::Collections::Generic;
using namespace System::Collections::ObjectModel;

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
//注意此处的宏不要使用过于复杂的代码，以便于 XML 注释。
#define _WRAP_PROPERTY_CACHE(name, type) \
		property type name;
#define _INIT_PROPERTY_CACHE(name, marshaller) \
		this->name = marshaller(native.name())

//用于使用托管结构体来封装非托管的对象模型指针
#define _WRAP_BEGIN_OBJECT_MODEL(_wrapperName, _nativeType) \
public value struct _wrapperName : System::IEquatable < _wrapperName > \
{ \
internal: \
	_nativeType* nativeObject; \
public: \
	static bool operator== (const _wrapperName x, const _wrapperName y) { return x.nativeObject == y.nativeObject; } \
	static bool operator!= (const _wrapperName x, const _wrapperName y) { return x.nativeObject != y.nativeObject; } \
	virtual bool Equals(_wrapperName obj) { return obj.nativeObject == this->nativeObject; } \
	virtual bool Equals(Object^ obj) override { \
		if (_wrapperName::typeid->IsInstanceOfType(obj)) \
																{ \
			return this->Equals((_wrapperName)obj); \
																} \
		return false; \
				} \
	int GetHashCode() override { return ((size_t)nativeObject).GetHashCode(); } \
internal: \
	static operator _nativeType*(_wrapperName obj) { return obj.nativeObject; } \
	_wrapperName(_nativeType* native) : nativeObject(native) {}

#define _WRAP_OBJECT_MODEL_TOSTRING(_nativeType) \
	public: System::String^ ToString() override \
	{ \
		return this->GetType()->Name + L" @ 0x" + ((size_t)nativeObject).ToString(L"x"); \
	}

#define _WRAP_OBJECT_MODEL_BASE(_wrapperName, _nativeType, _baseStruct) \
public: \
	static operator _baseStruct(_wrapperName obj) { return _baseStruct(obj.nativeObject); } \
	static explicit operator _wrapperName(_baseStruct base) \
{ \
if (base.nativeObject == nullptr) return _wrapperName(); \
auto native = dynamic_cast<_nativeType*>(base.nativeObject); \
if (native != nullptr) \
throw gcnew System::InvalidCastException(); \
return _wrapperName(native); \
} \
_wrapperName(_baseStruct base) \
: nativeObject(dynamic_cast<_nativeType*>(base.nativeObject)) \
{ \
if (nativeObject == nullptr && base.nativeObject != nullptr) \
throw gcnew System::InvalidCastException(); \
}

#define _WRAP_END_OBJECT_MODEL };

#define _WRAP_EXCEPTION_BOUNDARY(action) \
	try{action;} catch (std::exception& ex) {throw TranslateException(ex);}

#define _CHECK_NON_NULLABLE_PARAM(name) \
	if (name == nullptr) throw gcnew System::ArgumentNullException(L#name);

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
