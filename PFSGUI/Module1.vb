Imports PowerFlowSolver.Interop
Imports PowerFlowSolver.Interop.ObjectModel
Imports System.Numerics

Module Module1

    Public Function Main() As Integer
        TestWorkflow()
        '初始化主窗口
        Dim app As Application = New Application()
        app.StartupUri = New Uri("MainWindow.xaml", System.UriKind.Relative)
        app.Run()
        Return 0
    End Function

    Sub TestWorkflow()
        'PASSED
        '与例题计算结果相同
        '首先建立一个网络案例，其中包含了待求解的网络信息
        Dim Network As New NetworkCase("测试案例 Eg. 4-3")
        '由于PFS是基于标幺值系统的，因此存在基值的转换问题
        '在PowerFlowSolver.Interop中约定，各元件可以有自己独立的基准值
        '在代入PFS求解时，将会把所有的参数转换到一个统一的基准值下
        '目前全系统取用的基准值为 1kV，1MVA
        '即与有名值在数值上相等
        Dim Base110 As New PerUnitBase(110, 100)
        Dim Base10 As New PerUnitBase(10, 100)
        Dim LineImpedance = New Complex(0.21, 0.4)
        Dim LineSusceptance = 0.00000285    '2.85E-6
        '向网络中加入母线
        Dim b1 = Network.Buses.Add("1-环网-终端", 1, Base110),
            b2 = Network.Buses.Add("2-终端", 1, Base10),
            b3 = Network.Buses.Add("3-环网-右侧", 1, Base110),
            b4 = Network.Buses.Add("4-环网-左侧", 1, Base110)
        '向网络中加入其他元件
        Transformer.Create(b1, b2, 0, 0.105, 110 / 11 * 0.975, New PerUnitBase(110, 63))
        PVGenerator.Create(b3, 0.2, 1.05, Base110)
        SlackGenerator.Create(b4, 1.05, Base110)
        PQLoad.Create(b2, 50, 30)
        PQLoad.Create(b4, 15, 10)
        ShuntAdmittance.Create(b2, 0, 0.05, Base10)
        Line.Create(b4, b3, LineImpedance * 150, LineSusceptance * 150)
        Line.Create(b1, b4, LineImpedance * 100, LineSusceptance * 100)
        Line.Create(b1, b3, LineImpedance * 75, LineSusceptance * 75)
        '由于PowerFlowSolver.Interop只是封装了PFS的功能，
        '因此要求解潮流，需要指定PFS实用工具的位置
        '在默认情况下，程序将在当前路径下查找 PowerFlowSolver.exe
        '如果您需要手动指定位置，请使用
        'Interop.Application.DefaultExecutivePath = "D:\MyPath\PowerFlowSolver.exe"

        '在此处可以使用 PowerFlowSolver.Interop.Application
        '也可以使用 PowerFlowSolver.Interop.ApplicationFactory
        '后者提供了对多个PFS应用程序的调度控制，便于多线程使用
        Dim W As New Stopwatch
        Const N = 100
        W.Start()
        Using f As New ApplicationFactory
            For I = 1 To N
                Dim report = f.Solve(Network)
            Next I
        End Using
        W.Stop()
        MsgBox(TimeSpan.FromMilliseconds(W.ElapsedMilliseconds / N).ToString)
    End Sub
End Module
