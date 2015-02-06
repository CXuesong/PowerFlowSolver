
Imports System.Runtime.CompilerServices
Imports System.Numerics
Imports PowerSolutions.Interop.ObjectModel
Imports PowerSolutions.Interop.PowerFlow
Imports System.Text

<Discardable>
Friend Class ApplicationTests
    Public Shared Sub InteropBenchmark()
        Dim sw As New Stopwatch
        sw.Start()
        Using nc As New NetworkCase
            Dim b1 = nc.AddBus(),
                b2 = nc.AddBus(),
                b3 = nc.AddBus(),
                b4 = nc.AddBus()
            Dim tf = nc.AddTransformer(b1, b2, New Complex(0, 0.16666666666666666), 0.886363636363636)
            Dim gen = nc.AddPVGenerator(b3, 0.2, 1.05)
            nc.AddSlackGenerator(b4, 1.05)
            nc.AddPQLoad(b2, New Complex(0.5, 0.3))
            nc.AddPQLoad(b4, New Complex(0.15, 0.1))
            nc.AddShuntAdmittance(b2, New Complex(0, 0.05))
            nc.AddLine(b4, b3, New Complex(0.260331, 0.495868), New Complex(0, 0.051728))
            nc.AddLine(b1, b4, New Complex(0.173554, 0.330579), New Complex(0, 0.034486))
            nc.AddLine(b1, b3, New Complex(0.130165, 0.247934), New Complex(0, 0.025864))
            'For I = 1 To 100
            '    Dim twt = nc.AddThreeWindingTransformer(b1, b2, b3, 1, 2, 3, 5, 1, 0.5, 0.3)
            'Next I
            Console.WriteLine("Ellapsed : {0}", sw.Elapsed)
            Using solver As New Solver(SolverType.NewtonRaphson)
                solver.MaxDeviationTolerance = 0.0000000001
                Dim solution As Solution = Nothing
                sw.Restart()
                For I = 1 To 100
                    solution = solver.Solve(nc)
                Next
                sw.Stop()
                Console.WriteLine("Ellapsed : {0}", sw.Elapsed)
                Console.WriteLine("Iterations: {0}", solution.IterationCount)
                For Each nf In solution.NodeFlow
                    Console.WriteLine("Node:{0}" & vbTab & "Voltage:{1}∠{2}rad", nf.Key.GetHashCode, nf.Value.Voltage.Magnitude, nf.Value.Voltage.Phase)
                Next
                Console.WriteLine("PowerGeneration: {0}", solution.TotalPowerGeneration)
                Console.WriteLine("PowerConsumtion: {0} ", solution.TotalPowerConsumption)
                Console.WriteLine("PowerLoss: {0} ", solution.TotalPowerLoss)
                Console.WriteLine("PowerShunt: {0} ", solution.TotalPowerShunt)
            End Using
        End Using
    End Sub

    ''' <summary>
    ''' 构造一个文本文档，包含了n节点的性能测试样例。
    ''' </summary>
    ''' <param name="buses"></param>
    Shared Function BuildTestCase(buses As Integer) As String
        Dim r As New Random()
        Dim builder As New StringBuilder
        With builder
            .AppendLine("Attribute.Locale zh-CN")
            .AppendLine("Attribute.Version 1.1")
            .AppendLine("Attribute.Name 性能测试样例")
            .AppendLine("Attribute.Annotation Test Case")
            .AppendLine("Attribute.Solver NR")
            .AppendLine("Attribute.NodeReorder True")
            .AppendLine("Attribute.MaxDeviation 1E-6")
            Const LargeScaleBuses = 20
            For I = 1 To buses
                .AppendFormat("Bus {0}, Bus{0}, 1", I)
                .AppendLine()
            Next
            .AppendLine("SG 1, 1.05")
            '连接相邻的节点
            For I = 1 To buses - 1
                If I Mod LargeScaleBuses > 0 Then
                    .AppendFormat("LINE {0}, {1}, {2}, {3}, {4}", I, I + 1,
                               0.0026 + r.NextDouble * 0.001,
                               0.0139 + r.NextDouble * 0.001,
                               0.4611 + (r.NextDouble - 0.5) * 0.1)
                    .AppendLine()
                End If
            Next
            '--------+------+-----
            '        |      |
            '        \------/
            For I = 1 To buses - LargeScaleBuses - 1 Step LargeScaleBuses
                .AppendLine("#" & I)
                .AppendFormat("LINE {0}, {1}, {2}, {3}, {4}",
                           I + LargeScaleBuses \ 2, I + LargeScaleBuses + LargeScaleBuses \ 2, 0.0026, 0.0139, 0.4611)
                .AppendLine()
                .AppendFormat("LINE {0}, {1}, {2}, {3}, {4}",
                           I + LargeScaleBuses \ 4, I + LargeScaleBuses + LargeScaleBuses \ 3,
                           0.0026, 0.0139, 0.4611)
                .AppendLine()
            Next
            'PV发电机在最前面和最后面的节点上
            For I = 1 To buses - 1 Step LargeScaleBuses
                Dim Power1 = r.NextDouble
                .AppendFormat("PVGenerator {0}, {1}, 1.05", I, Power1)
                .AppendLine()
                .AppendFormat("PVGenerator {0}, {1}, 1.05", I + LargeScaleBuses - 1, 1 - Power1)
                .AppendLine()
            Next
            'PQ负载
            For I = 1 To buses
                .AppendFormat("PQLoad {0}, {1}, {2}", I, 1.0 / LargeScaleBuses, r.NextDouble)
                .AppendLine()
            Next
        End With
        Return builder.ToString
    End Function
End Class

