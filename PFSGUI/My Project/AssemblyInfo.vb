﻿Imports System
Imports System.Reflection
Imports System.Runtime.InteropServices
Imports System.Globalization
Imports System.Resources
Imports System.Windows

' 有关程序集的常规信息通过下列特性集
' 控制。更改这些特性值可修改
' 与程序集关联的信息。

' 查看程序集特性的值

<Assembly: AssemblyTitle("PFSGUI")> 
<Assembly: AssemblyDescription("")> 
<Assembly: AssemblyCompany("")> 
<Assembly: AssemblyProduct("PFSGUI")> 
<Assembly: AssemblyCopyright("Copyright ©  2015")> 
<Assembly: AssemblyTrademark("")> 
<Assembly: ComVisible(false)>

'若要开始生成可本地化的应用程序，请
'在您的 .vbproj 文件中的 <PropertyGroup> 内设置 <UICulture>CultureYouAreCodingWith</UICulture>。
'例如，如果您在源文件中使用的是美国英语，
'请将 <UICulture> 设置为“en-US”。  然后取消下面对
'NeutralResourceLanguage 特性的注释。  更新下面行中的“en-US”
'以与项目文件中的 UICulture 设置匹配。

'<Assembly: NeutralResourcesLanguage("en-US", UltimateResourceFallbackLocation.Satellite)> 


'ThemeInfo 特性说明在何处可以找到任何特定于主题的和一般性的资源词典。
'第一个参数:  特定于主题的资源词典的位置
'(在页面或应用程序资源词典中 
' 未找到某个资源的情况下使用)

'第二个参数:  一般性资源词典的位置
'(未在页、应用程序和任何特定于主题的
'资源词典中找到资源时使用)
<Assembly: ThemeInfo(ResourceDictionaryLocation.None, ResourceDictionaryLocation.SourceAssembly)>



'如果此项目向 COM 公开，则下列 GUID 用于类型库的 ID
<Assembly: Guid("48f60ceb-322f-4553-8497-37121f961fd4")> 

' 程序集的版本信息由下面四个值组成: 
'
'      主版本
'      次版本
'      生成号
'      修订号
'
' 可以指定所有这些值，也可以使用“生成号”和“修订号”的默认值，
' 方法是按如下所示使用“*”: 
' <Assembly: AssemblyVersion("1.0.*")> 

<Assembly: AssemblyVersion("1.0.0.0")> 
<Assembly: AssemblyFileVersion("1.0.0.0")> 
