Imports PowerSolutions.Interop.ObjectModel
Imports PowerSolutions.Interop.PowerFlow
Imports System.IO
Imports System.Text

''' <summary>
''' 用于从文本流中载入一个 <see cref="NetworkCase" />。
''' </summary>
Partial Public Class CaseManager
    Implements IDisposable

    Private m_CaseInfo As NetworkCase
    Private m_Solver As Solver
    Private m_BusKeyMapping As New Dictionary(Of String, BusInfo)
    Private m_BusMapping As New Dictionary(Of Bus, BusInfo)
    Private m_CaseName As String
    Private m_Solution As Solution
    '解析状态
    Private LineNumber As Integer

    Private Sub ParseLine(content As String)
        Debug.Assert(content <> Nothing)
        '注释 [ # .... ]
        If content(0) = "#"c Then Return
        '提取指令
        Dim Args = content.Split(DirectCast(Nothing, Char()), 2, StringSplitOptions.RemoveEmptyEntries)
        Dim Instruction = Args(0)
        If Args.Length > 1 Then
            Args = Args(1).Split({","c})
            For I = 0 To Args.Length - 1
                Args(I) = Args(I).Trim
            Next
        Else
            Args = {}
        End If
        InvokeInstruction(Instruction, Args)
    End Sub

    Private Sub Clear()
        If m_CaseInfo IsNot Nothing Then m_CaseInfo.Dispose()
        If m_Solver IsNot Nothing Then m_Solver.Dispose()
        m_CaseInfo = New NetworkCase
        m_Solver = Nothing
        m_BusKeyMapping.Clear()
        m_BusMapping.Clear()
        m_CaseName = Nothing
    End Sub

    Private Sub ParserPrintWarning(format As String, ParamArray args() As String)
        Console.WriteLine(Prompts.FileParseWarning, LineNumber, String.Format(format, args))
    End Sub

    ''' <summary>
    ''' 从文件中载入网络案例。
    ''' </summary>
    Public Sub Load(path As String)
        Try
            Using reader As New StreamReader(path, Encoding.Default, True)
                Load(reader)
            End Using
        Catch ex As Exception
            Console.Error.WriteLine(ex.Message)
        End Try
    End Sub

    ''' <summary>
    ''' 载入网络案例。
    ''' </summary>
    Private Sub Load(source As TextReader)
        Clear()
        Dim CurrentLine As String = source.ReadLine
        LineNumber = 1
        While CurrentLine IsNot Nothing
            CurrentLine = CurrentLine.Trim
            If CurrentLine <> Nothing Then
                Try
                    ParseLine(CurrentLine)
                Catch ex As Exception
                    Console.Error.WriteLine(Prompts.FileParseError, LineNumber, ex.Message)
                End Try
            End If
            CurrentLine = source.ReadLine
            LineNumber += 1
        End While
    End Sub

    Public Function Solve() As Boolean
        Using MonitorTimer As New Timers.Timer(1000)
            Try
                AddHandler MonitorTimer.Elapsed,
                    Sub()
                        Dim status = m_Solver.GetStatus
                        If status.IsIterating Then
                            Console.WriteLine(status.ToString)
                        End If
                    End Sub
                MonitorTimer.Enabled = True
                m_Solution = m_Solver.Solve(m_CaseInfo)
                Return True
            Catch ex As Exception
                Console.Error.WriteLine(ex.Message)
                Return False
            End Try
        End Using
    End Function

    Public Sub SaveReport(path As String)
        Try
            Using writer As New StreamWriter(path, False, If(GlobalParameters.ANSI, Encoding.Default, Encoding.UTF8))
                SaveReport(writer)
            End Using
        Catch ex As Exception
            Console.Error.WriteLine(ex.Message)
        End Try
    End Sub

    Public Sub SaveReport(dest As TextWriter)
        dest.WriteLine(My.Application.Info.ProductName)
        dest.WriteLine(My.Application.Info.Description)
        dest.WriteLine(My.Application.Info.Title)
        dest.WriteLine(My.Application.Info.Version.ToString)
        dest.WriteLine(My.Application.Info.Copyright)
        dest.WriteLine()
        dest.WriteLine(Prompts.PFReportMessage1, m_CaseName, Now.ToString)
        With m_Solution
            dest.WriteLine(Prompts.PFReportMessage2, m_BusMapping.Count, .NodeCount, .PQNodeCount, .PVNodeCount,
                           m_BusMapping(.SlackNode).Name, .BranchFlow.Count)
            dest.WriteLine(Prompts.PFReportMessage3, m_Solver.FriendlyName, .IterationCount, .MaxDeviation)
            For I = 0 To .IterationInfo.Count - 1
                dest.WriteLine(Prompts.PFReportMessage3A, I, .IterationInfo(I).MaxDeviation)
            Next
            dest.WriteLine(Prompts.PFReportMessage4)
            Dim Rad2Deg = Function(rad As Double) rad / Math.PI * 180
            Dim DblProc = Function(value As Double) Math.Round(value, 4)
            For Each node In .NodeFlow
                '        母线            电压       相位     有功出力   无功出力   有功负载   无功负载  支路数
                dest.WriteLine(Prompts.PFReportMessage4A, m_BusMapping(node.Key).Name,
                               DblProc(node.Value.Voltage.Magnitude), DblProc(Rad2Deg(node.Value.Voltage.Phase)),
                               DblProc(node.Value.PowerGeneration.Real), DblProc(node.Value.PowerGeneration.Imaginary),
                               DblProc(node.Value.PowerConsumption.Real), DblProc(node.Value.PowerConsumption.Imaginary),
                               node.Value.Degree)
            Next
        End With
    End Sub

#Region "IDisposable Support"
    Private disposedValue As Boolean ' 检测冗余的调用

    ' IDisposable
    Protected Overridable Sub Dispose(disposing As Boolean)
        If Not Me.disposedValue Then
            If disposing Then
                ' 释放托管状态(托管对象)。
                If m_CaseInfo IsNot Nothing Then m_CaseInfo.Dispose()
                If m_Solver IsNot Nothing Then m_Solver.Dispose()
            End If

            ' 释放非托管资源(非托管对象)并重写下面的 Finalize()。
            ' 将大型字段设置为 null。
        End If
        Me.disposedValue = True
    End Sub

    ' TODO:  仅当上面的 Dispose(ByVal disposing As Boolean)具有释放非托管资源的代码时重写 Finalize()。
    'Protected Overrides Sub Finalize()
    '    ' 不要更改此代码。    请将清理代码放入上面的 Dispose(ByVal disposing As Boolean)中。
    '    Dispose(False)
    '    MyBase.Finalize()
    'End Sub

    ' Visual Basic 添加此代码是为了正确实现可处置模式。
    Public Sub Dispose() Implements IDisposable.Dispose
        ' 不要更改此代码。    请将清理代码放入上面的 Dispose (disposing As Boolean)中。
        Dispose(True)
        GC.SuppressFinalize(Me)
    End Sub
#End Region

End Class

Public Class BusInfo
    Private m_Bus As Bus
    Private m_Key As String
    Private m_Name As String

    Public ReadOnly Property Key As String
        Get
            Return m_Key
        End Get
    End Property

    Public ReadOnly Property Name As String
        Get
            Return m_Name
        End Get
    End Property

    Public ReadOnly Property Bus As Bus
        Get
            Return m_Bus
        End Get
    End Property

    Public Sub New(bus As Bus, key As String, name As String)
        m_Bus = bus
        m_Key = key
        m_Name = If(name, key)
    End Sub
End Class

