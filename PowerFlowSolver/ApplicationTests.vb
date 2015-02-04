
Imports System.Runtime.CompilerServices
Imports System.Numerics
Imports PowerSolutions.Interop.ObjectModel
Imports PowerSolutions.Interop.PowerFlow

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
End Class

