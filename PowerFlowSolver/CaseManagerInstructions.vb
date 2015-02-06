Imports PowerSolutions.Interop.ObjectModel
Imports PowerSolutions.Interop.PowerFlow
Imports System.Numerics

Partial Public Class CaseManager

    Private Shared InstructionDict As New Dictionary(Of String, Instruction)

    Public Delegate Sub InstructionAction(sender As CaseManager, args() As String)

    ''' <summary>
    ''' 表示一条已知的指令。
    ''' </summary>
    Public Structure Instruction
        Private m_Name As String
        Private m_ArgumentCount As Integer()
        Private m_Action As InstructionAction

        Public ReadOnly Property Name As String
            Get
                Return m_Name
            End Get
        End Property

        Public ReadOnly Property ArgumentCount As Integer()
            Get
                Return m_ArgumentCount
            End Get
        End Property

        Public ReadOnly Property Action As InstructionAction
            Get
                Return m_Action
            End Get
        End Property

        Public Sub Invoke(sender As CaseManager, args() As String)
            If sender Is Nothing Then Throw New ArgumentNullException("sender")
            If args Is Nothing Then Throw New ArgumentNullException("args")
            If m_ArgumentCount.Length > 0 AndAlso m_ArgumentCount.Contains(args.Length) = False Then
                Throw New InvalidOperationException(
                    String.Format(Prompts.ParameterCountMismatch3,
                                  m_Name, String.Join(My.Application.Culture.TextInfo.ListSeparator, m_ArgumentCount),
                                  args.Count))
            End If
            m_Action(sender, args)
        End Sub

        Public Sub New(name As String, argumentCount As Integer, action As InstructionAction)
            m_Name = name
            m_ArgumentCount = {argumentCount}
            m_Action = action
        End Sub

        Public Sub New(name As String, argumentCount() As Integer, action As InstructionAction)
            m_Name = name
            m_ArgumentCount = argumentCount
            m_Action = action
        End Sub
    End Structure

    Private Shared Sub RegisterInstruction(name As String, argumentCount As Integer, action As InstructionAction)
        InstructionDict.Add(UCase(name), New Instruction(name, argumentCount, action))
    End Sub

    Private Shared Sub RegisterInstruction(name As String, name2 As String, argumentCount As Integer, action As InstructionAction)
        RegisterInstruction(name, name2, {argumentCount}, action)
    End Sub

    Private Shared Sub RegisterInstruction(name As String, name2 As String, argumentCount() As Integer, action As InstructionAction)
        InstructionDict.Add(UCase(name), New Instruction(name, argumentCount, action))
        InstructionDict.Add(UCase(name2), New Instruction(name, argumentCount, action))
    End Sub

    Private Shared Sub RegisterInstruction(name As String, argumentCount() As Integer, action As InstructionAction)
        InstructionDict.Add(UCase(name), New Instruction(name, argumentCount, action))
    End Sub

    Private Sub InvokeInstruction(name As String, args() As String)
        Dim inst As Instruction
        Try
            inst = InstructionDict(UCase(name))
        Catch ex As KeyNotFoundException
            Throw New ArgumentException(String.Format(Prompts.InvalidInstruction1, name))
        End Try
        inst.Invoke(Me, args)
    End Sub

    Private Sub EnsureSolver()
        If m_Solver Is Nothing Then m_Solver = New Solver(SolverType.NewtonRaphson)
    End Sub

    Private Sub AddBus(key As String, Optional name As String = Nothing, Optional initialVoltage As Double = 1)
        Dim b = m_CaseInfo.AddBus(initialVoltage)
        Dim info As New BusInfo(b, key, name)
        m_BusKeyMapping.Add(UCase(key), info)
        m_BusMapping.Add(b, info)
    End Sub

    Private Function GetBus(key As String) As Bus
        Try
            Return m_BusKeyMapping(UCase(key)).Bus
        Catch ex As KeyNotFoundException
            Throw New ArgumentException(String.Format(Prompts.BusKeyNotFound, key), "key")
        End Try
    End Function

    Private Shared Function Eval(expression As String) As Double
        Return CDbl(expression)
    End Function

    Shared Sub New()
        Dim obsoleteAction As InstructionAction =
            Sub(s, p)
                s.ParserPrintWarning(Prompts.ObsoleteInstruction)
            End Sub
        RegisterInstruction("Attribute.Version", 1,
                            Sub(sender, p)
                                Dim v As New Version(p(0))
                            End Sub)
        RegisterInstruction("Attribute.Name", 1, Sub(sender, p) sender.m_CaseName = p(0))
        RegisterInstruction("Attribute.Locale", {}, obsoleteAction)
        RegisterInstruction("Attribute.Annotation", {}, obsoleteAction)
        RegisterInstruction("Attribute.Solver", 1,
                            Sub(s, p)
                                If s.m_Solver IsNot Nothing Then
                                    s.ParserPrintWarning(Prompts.DuplicateSolverInstruction)
                                    s.m_Solver.Dispose()
                                    s.m_Solver = Nothing
                                End If
                                Select Case UCase(p(0))
                                    Case "NR", "NEWTONRAPHSON"
                                        s.m_Solver = New Solver(SolverType.NewtonRaphson)
                                    Case "PQ", "FASTDECOUPLED"
                                        s.m_Solver = New Solver(SolverType.FastDecoupled)
                                    Case Else
                                        Throw New ArgumentException(String.Format(Prompts.InvalidSolver, p(0)))
                                End Select
                            End Sub)
        RegisterInstruction("Attribute.NodeReorder", 1,
                            Sub(sender, p)
                                sender.EnsureSolver()
                                sender.m_Solver.NodeReorder = CBool(p(0))
                            End Sub)
        RegisterInstruction("Attribute.MaxIterations", 1,
                            Sub(sender, p)
                                sender.EnsureSolver()
                                sender.m_Solver.MaxIterations = CInt(p(0))
                            End Sub)
        RegisterInstruction("Attribute.MaxDeviation", 1,
                            Sub(sender, p)
                                sender.EnsureSolver()
                                sender.m_Solver.MaxDeviationTolerance = CDbl(p(0))
                            End Sub)
        RegisterInstruction("Attribute.IntelliIterations", 1,
                            Sub(sender, p)
                                sender.EnsureSolver()
                                sender.m_Solver.IntelliIterations = CBool(p(0))
                            End Sub)
        RegisterInstruction("NextIndex", 1, obsoleteAction)
        RegisterInstruction("Bus", "B", {1, 2, 3},
                            Sub(sender, p)
                                'Bus Key
                                'Bus Key, Name
                                'Bus Key, Name, Voltage
                                Select Case p.Length
                                    Case 1
                                        sender.AddBus(p(0))
                                    Case 2
                                        sender.AddBus(p(0), p(1))
                                    Case 3
                                        sender.AddBus(p(0), p(1), Eval(p(2)))
                                End Select
                            End Sub)
        RegisterInstruction("Line", "L", 5,
                            Sub(s, p)
                                s.m_CaseInfo.AddLine(s.GetBus(p(0)), s.GetBus(p(1)),
                                                     New Complex(Eval(p(2)), Eval(p(3))),
                                                     New Complex(0, Eval(p(4))))
                            End Sub)
        RegisterInstruction("Transformer", "T", {5, 7},
                            Sub(s, p)
                                'Transformer Bus1, Bus2, R, X, k
                                'Transformer Bus1, Bus2, R, X, G, B, k
                                Select Case p.Length
                                    Case 5
                                        s.m_CaseInfo.AddTransformer(s.GetBus(p(0)), s.GetBus(p(1)),
                                                             New Complex(Eval(p(2)), Eval(p(3))),
                                                             New Complex(Eval(p(4)), 0))
                                    Case 7
                                        s.m_CaseInfo.AddTransformer(s.GetBus(p(0)), s.GetBus(p(1)),
                                                             New Complex(Eval(p(2)), Eval(p(3))),
                                                             New Complex(Eval(p(4)), Eval(p(5))),
                                                             New Complex(Eval(p(6)), 0))
                                End Select
                            End Sub)
        RegisterInstruction("SlackGenerator", "SG", 2,
                            Sub(s, p)
                                s.m_CaseInfo.AddSlackGenerator(s.GetBus(p(0)), Eval(p(1)))
                            End Sub)
        RegisterInstruction("PVGenerator", "PVG", 3,
                            Sub(s, p)
                                s.m_CaseInfo.AddPVGenerator(s.GetBus(p(0)), Eval(p(1)), Eval(p(2)))
                            End Sub)
        RegisterInstruction("PQLoad", "PQL", 3,
                            Sub(s, p)
                                s.m_CaseInfo.AddPQLoad(s.GetBus(p(0)), New Complex(Eval(p(1)), Eval(p(2))))
                            End Sub)
        RegisterInstruction("ShuntAdmittance", "SA", 3,
                            Sub(s, p)
                                s.m_CaseInfo.AddShuntAdmittance(s.GetBus(p(0)), New Complex(Eval(p(1)), Eval(p(2))))
                            End Sub)
    End Sub
End Class