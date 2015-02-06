
Module Module1

    Public GlobalParameters As New ApplicationParameters

    Function Main() As Integer
        Const DefaultArgument = "/?"
        Console.CursorVisible = False
        Console.Title = My.Application.Info.ProductName
        Console.WriteLine(My.Application.Info.ProductName)
        Console.WriteLine(My.Application.Info.Description)
        Console.WriteLine(My.Application.Info.Title)
        Console.WriteLine(My.Application.Info.Version.ToString)
        Console.WriteLine(My.Application.Info.Copyright)
        If Environment.Is64BitProcess Then Console.WriteLine(Prompts.x64Process)
        Console.WriteLine()
        '处理命令行
        Dim Args As IEnumerable(Of String) = {DefaultArgument}
        If My.Application.CommandLineArgs.Count > 0 Then Args = My.Application.CommandLineArgs
        For Each EachArg In Args
            Dim Cmd As String, Arg As String, Locator As Integer
            Locator = EachArg.IndexOf(":"c)     '-Cmd:Arg
            If Locator > 0 Then
                Cmd = EachArg.Substring(0, Locator).Trim
                Arg = EachArg.Substring(Locator + 1).Trim(" "c, ControlChars.Tab, """"c)
            Else
                Cmd = EachArg
                Arg = ""
            End If
            Select Case UCase(Cmd)
                Case "-?", "--?", "/?", "-HELP", "--HELP", "/HELP"
                    '显示帮助
                    With My.Application.Info
                        Console.WriteLine(Prompts.CommandLineUsage)
                        Return 0
                    End With
                Case "/CASE", "-CASE"
                    GlobalParameters.CaseFile = Arg
                Case "/REPORT", "-REPORT"
                    GlobalParameters.ReportFile = Arg
                Case "/ANSI", "-ANSI"
                    GlobalParameters.ANSI = True
                Case "/BUILDTESTCASE"
                    My.Computer.FileSystem.WriteAllText("PerfTestCase_" & CInt(Arg) & ".txt", ApplicationTests.BuildTestCase(CInt(Arg)), False)
                    Return 0
                Case Else
                    '无效的参数
                    Console.Error.WriteLine(Prompts.InvalidArgument1, Command)
                    Return 1
            End Select
        Next
        '校验参数
        If GlobalParameters.CaseFile = Nothing Then
            Console.Error.WriteLine(Prompts.CaseFileMissing)
            Return 2
        End If
        Workflow()
#If DEBUG Then
        Console.WriteLine("--- END ---")
        Console.ReadKey()
#End If
        Return 0
    End Function

    Private Sub Workflow()
        'ApplicationTests.InteropBenchmark()
        Using manager As New CaseManager
            manager.Load(GlobalParameters.CaseFile)
            Console.WriteLine(Prompts.SolutionInProgress)
            If manager.Solve Then
                If GlobalParameters.ReportFile = Nothing Then
                    manager.SaveReport()
                Else
                    manager.SaveReport(GlobalParameters.ReportFile)
                End If
            End If
        End Using
    End Sub
End Module

''' <summary>
''' 保存了应用程序所使用到的参数。
''' </summary>
Public Class ApplicationParameters
    Public Property ANSI As Boolean = False
    Public Property CaseFile As String = Nothing
    Public Property ReportFile As String = Nothing
End Class
