Imports System.ComponentModel
Imports CXuesong.Shims.VisualBasic.ApplicationServices
Imports CXuesong.Shims.VisualBasic.Devices

Namespace My

    <HideModuleName>
    Friend Module MyProject

        Public ReadOnly Property Application As New MyApplication

        Public ReadOnly Property Computer As New MyComputer

    End Module

    <EditorBrowsable(EditorBrowsableState.Never)>
    Friend Class MyApplication
        Inherits ConsoleApplicationBase

        Public Sub New()
            MyBase.New(GetType(MyApplication).Assembly)
        End Sub
    End Class

    <EditorBrowsable(EditorBrowsableState.Never)>
    Friend Class MyComputer
        Inherits Computer

    End Class

End Namespace
