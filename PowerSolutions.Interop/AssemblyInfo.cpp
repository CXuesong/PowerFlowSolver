#include "stdafx.h"

using namespace System;
using namespace System::Reflection;
using namespace System::Runtime::CompilerServices;
using namespace System::Runtime::InteropServices;
using namespace System::Security::Permissions;

//
// �йس��򼯵ĳ�����Ϣͨ���������Լ�
// ���ơ�������Щ����ֵ���޸�
// ����򼯹�������Ϣ��
//
[assembly:AssemblyTitleAttribute(L"PowerSolutions.Interop")];
[assembly:AssemblyDescriptionAttribute(L"")];
#if _DEBUG
[assembly:AssemblyConfigurationAttribute(L"Debug")];
#else
[assembly:AssemblyConfigurationAttribute(L"Release")];
#endif
[assembly:AssemblyCompanyAttribute(L"")];
[assembly:AssemblyProductAttribute(L"PowerSolutions.Interop")];
[assembly:AssemblyCopyrightAttribute(L"Copyright (c) Chen [CXuesong.], 2015")];
[assembly:AssemblyTrademarkAttribute(L"")];
[assembly:AssemblyCultureAttribute(L"")];

//
// ���򼯵İ汾��Ϣ�������ĸ�ֵ���: 
//
//      ���汾
//      �ΰ汾
//      ���ɺ�
//      �޶���
//
// ������ָ������ֵ��Ҳ����ʹ�á��޶��š��͡����ɺš���Ĭ��ֵ��
// �����ǰ�������ʾʹ�á�*��: 

[assembly:AssemblyVersionAttribute("1.0.*")];

[assembly:ComVisible(false)];

[assembly:CLSCompliantAttribute(true)];
