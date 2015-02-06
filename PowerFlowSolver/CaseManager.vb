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
    Private m_IterationHistory As New List(Of Double)
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
            Console.WriteLine(Prompts.FileParseComplete)
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
        '准备工作
        EnsureSolver()
        m_IterationHistory.Clear()
        '每隔1s向控制台输出目前的进度
        Dim maxDev = m_Solver.MaxDeviationTolerance
        Dim maxIter = m_Solver.MaxIterations
        'Using MonitorTimer As New Timers.Timer(1000)
        Dim watch As New Stopwatch
        Dim OnIteration As IterationEventHandler =
        Sub(sender, e)
                Debug.Assert(e.IterationCount = m_IterationHistory.Count)
                m_IterationHistory.Add(e.MaxDeviation)
                Console.WriteLine(Prompts.IterationProgressA, watch.Elapsed, e.IterationCount, e.MaxDeviation)
            End Sub
        Try
            'AddHandler MonitorTimer.Elapsed,
            '    Sub()
            '        'Dim status = m_Solver.GetStatus
            '        'If status.IsIterating Then
            '        '    Console.Write(Prompts.SolutionProgress, watch.Elapsed,
            '        '                      status.LastIterationCount, status.LastIterationInfo.MaxDeviation,
            '        '                      maxDev)
            '        '    '初期可能估计不准确
            '        '    If status.LastIterationCount > 1 Then
            '        '        Console.Write(Prompts.TimeRemainingEstimation,
            '        '                      TimeSpan.FromTicks(CLng(watch.ElapsedTicks * (maxIter / status.LastIterationCount - 1))))
            '        '    End If
            '        '    Console.CursorLeft = 0
            '        'End If
            '    End Sub
            AddHandler m_Solver.Iteration, OnIteration
            Console.WriteLine()
            Console.WriteLine(Prompts.IterationProgress)
            'MonitorTimer.Enabled = True
            watch.Start()
            '开始求解
            m_Solution = m_Solver.Solve(m_CaseInfo)
            watch.Stop()
            Console.WriteLine()
            Console.WriteLine(Prompts.SolutionComplete, m_Solution.IterationCount, watch.Elapsed,
                              Prompts.ResourceManager.GetString("SolutionStatus_" & m_Solution.Status.ToString))
            Return True
        Catch ex As Exception
            Console.Error.WriteLine(ex.Message)
            Return False
        Finally
            RemoveHandler m_Solver.Iteration, OnIteration
        End Try
        'End Using
    End Function

    ''' <summary>
    ''' 向控制台输出报告。
    ''' </summary>
    Public Sub SaveReport()
        Console.WriteLine(Prompts.PFReport)
        SaveReport(Console.Out)
    End Sub

    ''' <summary>
    ''' 向文件输出报告。
    ''' </summary>
    Public Sub SaveReport(path As String)
        Try
            Using writer As New StreamWriter(path, False, If(GlobalParameters.ANSI, Encoding.Default, Encoding.UTF8))
                SaveReport(writer)
            End Using
            Console.WriteLine(Prompts.FileSaveComplete, IO.Path.GetFullPath(path))
        Catch ex As Exception
            Console.Error.WriteLine(ex.Message)
        End Try
    End Sub

    Private Sub SaveReport(dest As TextWriter)
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
            '求解信息
            dest.WriteLine(Prompts.PFReportMessage3,
                           Prompts.ResourceManager.GetString("SolutionStatus_" & .Status.ToString),
                           m_Solver.FriendlyName, .IterationCount, .MaxDeviation)
            For I = 0 To m_IterationHistory.Count - 1
                dest.WriteLine(Prompts.PFReportMessage3A, I, m_IterationHistory(I))
            Next
            '节点潮流
            dest.WriteLine(Prompts.PFReportMessage4)
            Dim Rad2Deg = Function(rad As Double) rad / Math.PI * 180
            For Each node In (From n In m_BusMapping.Values Order By n.Name Ascending)
                Dim nodeFlow As NodeFlowSolution
                If .NodeFlow.TryGetValue(node.Bus, nodeFlow) Then
                    With nodeFlow
                        '        母线            电压       相位     有功出力   无功出力   有功负载   无功负载  支路数
                        dest.WriteLine(Prompts.PFReportMessage4A, PadRight(node.Name, 20),
                                       .Voltage.Magnitude, Rad2Deg(.Voltage.Phase),
                                       .PowerGeneration.Real, .PowerGeneration.Imaginary,
                                       .PowerConsumption.Real, .PowerConsumption.Imaginary,
                                       .Degree)
                    End With
                Else
                    dest.WriteLine(Prompts.PFReportMessage4B, PadRight(node.Name, 20))
                End If
            Next
            '支路潮流
            dest.WriteLine(Prompts.PFReportMessage5)
            Dim BranchEx =
                Iterator Function()
                    For Each b In .BranchFlow
                        Dim bus1 As BusInfo = Nothing, bus2 As BusInfo = Nothing
                        Dim flow = b.Value
                        If m_BusMapping.TryGetValue(b.Key.Bus1, bus1) AndAlso
                            m_BusMapping.TryGetValue(b.Key.Bus2, bus2) Then
                            If b.Value.ReversedDirection Then
                                Dim temp = bus1
                                bus1 = bus2
                                bus2 = temp
                                flow = flow.Reverse
                            End If
                            Yield New With {.Bus1 = m_BusMapping(b.Key.Bus1),
                                            .Bus2 = m_BusMapping(b.Key.Bus2),
                                            .Flow = b.Value}
                        End If
                    Next
                End Function
            For Each branch In (From b In BranchEx()
                                Order By b.Bus1.Name, b.Bus2.Name Ascending)
                With branch
                    Dim loss = .Flow.PowerLoss, shunt = .Flow.PowerShunt
                    dest.WriteLine(Prompts.PFReportMessage5A, PadRight(.Bus1.Name, 14),
                                   PadRight(.Bus2.Name, 14),
                                   .Flow.Power1.Real, .Flow.Power1.Imaginary,
                                   .Flow.Power2.Real, .Flow.Power2.Imaginary,
                                   loss.Real, loss.Imaginary,
                                   shunt.Real, shunt.Imaginary)
                End With
            Next
            '总计
            dest.WriteLine(Prompts.PFReportMessage6,
                           .TotalPowerGeneration.Real, .TotalPowerGeneration.Imaginary,
                           .TotalPowerConsumption.Real, .TotalPowerConsumption.Imaginary,
                           .TotalPowerLoss.Real, .TotalPowerLoss.Imaginary,
                           .TotalPowerShunt.Real, .TotalPowerShunt.Imaginary)
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

