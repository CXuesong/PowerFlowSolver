Imports System.Text
Imports Microsoft.VisualStudio.TestTools.UnitTesting

Imports PowerSolutions.Interop.ObjectModel
Imports PowerSolutions.Interop.PowerFlow
Imports System.Numerics

<TestClass()> Public Class UnitTest1

    Private Function GenerateDemoNetwork() As NetworkCase
        Dim nc As New NetworkCase
        Dim b1 = nc.AddBus(),
            b2 = nc.AddBus(),
            b3 = nc.AddBus(),
            b4 = nc.AddBus()
        '检查 = 运算符重载是否正确
        Debug.Assert(b1 = b1)
        Debug.Assert(b1 <> b2)
        Console.WriteLine("b1 @ {0}", b1.GetHashCode)
        Console.WriteLine("b2 @ {0}", b2.GetHashCode)
        Console.WriteLine("b3 @ {0}", b3.GetHashCode)
        Dim tf = nc.AddTransformer(b1, b2, New Complex(0, 0.16666666666666666), 0.886363636363636)
        Dim gen = nc.AddPVGenerator(b3, 0.2, 1.05)
        nc.AddSlackGenerator(b4, 1.05)
        nc.AddPQLoad(b2, New Complex(0.5, 0.3))
        nc.AddPQLoad(b4, New Complex(0.15, 0.1))
        nc.AddShuntAdmittance(b2, New Complex(0, 0.05))
        nc.AddLine(b4, b3, New Complex(0.260331, 0.495868), New Complex(0, 0.051728))
        nc.AddLine(b1, b4, New Complex(0.173554, 0.330579), New Complex(0, 0.034486))
        nc.AddLine(b1, b3, New Complex(0.130165, 0.247934), New Complex(0, 0.025864))
        'Dim twt = nc.AddThreeWindingTransformer(b1, b2, b3, 1, 2, 3, 5, 1, 0.5, 0.3)
        'Console.WriteLine(twt.CommonBus.GetHashCode.ToString)
        Return nc
    End Function

    <TestMethod()>
    Public Sub PFInteropTest()
        Using nc = GenerateDemoNetwork()
            Using solver = New Solver(SolverType.NewtonRaphson)
                solver.MaxDeviationTolerance = 0.0000000001
                Console.WriteLine(solver.IntelliIterations)
                Dim solution = solver.Solve(nc)
                For Each nf In solution.NodeFlow
                    Console.WriteLine("Node:{0}" & vbTab & "Voltage:{1}∠{2}rad",
                                      nf.Key.GetHashCode, nf.Value.Voltage.Magnitude, nf.Value.Voltage.Phase)
                Next
                For Each cf In solution.ComponentFlow
                    Console.WriteLine("Component:{0}" & vbTab & "Inj:{1}" & vbTab & "Sht:{2}",
                                      cf.Key.GetHashCode, String.Join(";", cf.Value.PowerInjections), cf.Value.PowerShunt)
                Next
            End Using
        End Using
    End Sub

    <TestMethod()>
    Public Sub PFInteropProfiling()
        Const Repetitions = 100
        Dim sw As New Stopwatch
        Using nc = GenerateDemoNetwork()
            Using solver = New Solver(SolverType.NewtonRaphson)
                solver.Solve(nc)
                solver.NodeReorder = True
                sw.Start()
                For I = 1 To Repetitions
                    solver.MaxDeviationTolerance = 0.0000000001
                    Dim solution = solver.Solve(nc)
                    Assert.IsTrue(solution.Status = SolutionStatus.Success)
                Next
                sw.Stop()
                Trace.WriteLine(String.Format("NodeReorder = True, {0}ms", sw.ElapsedMilliseconds / Repetitions))
                solver.NodeReorder = False
                sw.Start()
                For I = 1 To Repetitions
                    solver.MaxDeviationTolerance = 0.0000000001
                    Dim solution = solver.Solve(nc)
                    Assert.IsTrue(solution.Status = SolutionStatus.Success)
                Next
                sw.Stop()
                Trace.WriteLine(String.Format("NodeReorder = False, {0}ms", sw.ElapsedMilliseconds / Repetitions))
            End Using
        End Using
    End Sub

End Class