Imports PowerSolutions.Interop.ObjectModel
Imports PowerSolutions.Interop.PowerFlow
Imports System.Numerics

Module Module1

    Sub Main()
        UI.Init()
        Dim nc As New NetworkCase
        Dim b1 = nc.AddBus(),
        b2 = nc.AddBus(),
        b3 = nc.AddBus(),
        b4 = nc.AddBus()
        nc.AddTransformer(b1, b2, New Complex(0, 0.16666666666666666), 0.886363636363636)
        nc.AddPVGenerator(b3, 0.2, 1.05)
        nc.AddSlackGenerator(b4, 1.05)
        nc.AddPQLoad(b2, New Complex(0.5, 0.3))
        nc.AddPQLoad(b4, New Complex(0.15, 0.1))
        nc.AddShuntAdmittance(b2, New Complex(0, 0.05))
        nc.AddLine(b4, b3, New Complex(0.260331, 0.495868), New Complex(0, 0.051728))
        nc.AddLine(b1, b4, New Complex(0.173554, 0.330579), New Complex(0, 0.034486))
        nc.AddLine(b1, b3, New Complex(0.130165, 0.247934), New Complex(0, 0.025864))
        Dim solver As New Solver(SolverType.NewtonRaphson)
        solver.MaxDeviationTolerance = 0.0000000001
        MsgBox(solver.IntelliIterations)
        Dim solution = solver.Solve(nc)
        solution.Dispose()
    End Sub

End Module
