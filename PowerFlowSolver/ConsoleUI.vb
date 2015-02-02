''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''
' 通用 UI 模块。
' Ver 1.1.13.0629
' By  Chen [CXuesong.], 2013
''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''''

Imports System.IO
Imports System.Text

''' <summary>
''' 负责处理基本的用户界面操作。
''' </summary>
Public Class UI
    Private Class Prompts
        Public Const InvalidInput$ = "无效输入。"
        Public Const NumberOverflow$ = "数值过大。"
        Public Const Over$ = "结束"
        Public Const x64Process$ = "64位进程"
    End Class

    Private Structure Point
        Public X As Integer
        Public Y As Integer

        Public Sub New(x As Integer, y As Integer)
            Me.X = x
            Me.Y = y
        End Sub
    End Structure

    Private Shared m_CursorLocked As Boolean

    Public Shared Property CursorLocked As Boolean
        Get
            Return m_CursorLocked
        End Get
        Set(value As Boolean)
            m_CursorLocked = value
        End Set
    End Property

    ''' <summary>
    ''' 初始化。
    ''' </summary>
    Public Shared Sub Init()
        Console.CursorVisible = False
        Console.Title = My.Application.Info.ProductName
        Console.WriteLine(My.Application.Info.ProductName)
        Console.WriteLine(My.Application.Info.Description)
        Console.WriteLine(My.Application.Info.Title)
        Console.WriteLine(My.Application.Info.Version.ToString)
        Console.WriteLine(My.Application.Info.Copyright)
        If Environment.Is64BitProcess Then Console.WriteLine(Prompts.x64Process)
        Console.WriteLine()
    End Sub

    ''' <summary>
    ''' 接受是/否输入。
    ''' </summary>
    ''' <param name="prompt">接受输入的提示信息。</param>
    Public Shared Function Confirm(ByVal prompt As String, Optional ByVal defaultValue As Boolean = False) As Boolean
        Return (Input(prompt, If(defaultValue, "Y", "N"), _
                      "Y", "是", "N", "否") = "Y")
    End Function

    ''' <summary>
    ''' 接受数据输入。
    ''' </summary>
    ''' <param name="prompt">接受输入的提示信息。</param>
    Public Shared Function Input(ByVal prompt As String) As String
        Return Input(prompt, Nothing, New String() {})
    End Function

    ''' <summary>
    ''' 接受数据输入。
    ''' </summary>
    ''' <param name="prompt">接受输入的提示信息。</param>
    ''' <param name="defaultValue">如果输入为空，返回的默认值。</param>
    Public Shared Function Input(ByVal prompt As String, ByVal defaultValue As String) As String
        Return Input(prompt, defaultValue, New String() {})
    End Function

    ''' <summary>
    ''' 接受选项输入。
    ''' </summary>
    ''' <param name="prompt">接受输入的提示信息。</param>
    ''' <param name="defaultValue">如果输入为空，返回的默认值。</param>
    ''' <param name="selection">可用选项。格式为：option1, description1, option2, description2, ...</param>
    Public Shared Function Input(ByVal prompt As String, ByVal defaultValue As String, ByVal ParamArray selection() As String) As String
INPUT:
        Console.Write(prompt)
        If selection.Length > 0 Then
            Console.Write("["c)
            For I = 0 To selection.Length - 1 Step 2
                Console.Write(selection(I + 1))
                Console.Write("("c)
                Console.Write(selection(I).ToUpperInvariant)
                Console.Write(")"c)
                If I + 2 < selection.Length Then
                    Console.Write("/"c)
                End If
            Next
            Console.Write("]"c)
        End If
        If defaultValue <> Nothing Then
            Console.Write("<"c)
            Console.Write(defaultValue)
            Console.Write(">"c)
        End If
        Console.Write("："c)
        Console.CursorVisible = True
        Dim inp = Console.ReadLine
        Console.CursorVisible = False
        If inp = Nothing Then
            Return defaultValue
        ElseIf selection.Length > 0 Then
            '检查输入
            For I = 0 To selection.Length - 1 Step 2
                If String.Compare(inp, selection(I), StringComparison.OrdinalIgnoreCase) = 0 Then
                    Return selection(I)
                End If
            Next
            '输入无效
            Console.WriteLine(Prompts.InvalidInput)
            GoTo INPUT
        Else
            Return inp
        End If
    End Function

    ''' <summary>
    ''' 接受指定类型的输入。
    ''' </summary>
    ''' <param name="prompt">接受输入的提示信息。</param>
    ''' <param name="defaultValueHint">默认值的描述字符串。</param>
    Public Shared Function Input(Of T As Structure)(ByVal prompt As String, Optional ByVal defaultValueHint As String = Nothing) As T?
INPUT:
        Console.Write(prompt)
        If defaultValueHint IsNot Nothing Then
            Console.Write("<"c)
            Console.Write(defaultValueHint)
            Console.Write(">"c)
        End If
        Console.Write("："c)
        Dim inp = Console.ReadLine
        If inp = Nothing Then Return Nothing
        Try
            Return DirectCast(Convert.ChangeType(inp, GetType(T)), T)
        Catch ex As FormatException
            Console.WriteLine(Prompts.InvalidInput)
        Catch ex As OverflowException
            Console.Write(Prompts.NumberOverflow)
        End Try
        GoTo INPUT
    End Function

    ''' <summary>
    ''' 接受指定类型的输入。
    ''' </summary>
    ''' <param name="prompt">接受输入的提示信息。</param>
    ''' <param name="defaultValue">如果输入为空，返回的默认值。</param>
    Public Shared Function Input(Of T As Structure)(ByVal prompt As String, ByVal defaultValue As T?) As T
        Return Input(Of T)(prompt, defaultValue.ToString).GetValueOrDefault(defaultValue.GetValueOrDefault)
    End Function

    ''' <summary>
    ''' 接受列表输入。
    ''' </summary>
    Public Shared Sub Input(Of T As Structure)(ByVal prompt As String, ByVal list As IList(Of T))
        Do
            Dim NewItem = Input(Of T)(prompt, Prompts.Over)
            If NewItem Is Nothing Then
                Return
            Else
                list.Add(NewItem.Value)
            End If
        Loop
    End Sub

    ''' <summary>
    ''' 输出空行。
    ''' </summary>
    Public Shared Sub Print()
        If m_CursorLocked Then
            PushCursor()
            Console.Write(Space(Console.WindowWidth))
            PopCursor()
        Else
            Console.WriteLine()
        End If
    End Sub

    ''' <summary>
    ''' 输出字符串。
    ''' </summary>
    Public Shared Sub Print(ByVal v As Object)
        If m_CursorLocked Then PushCursor()
        Console.WriteLine(v)
        If m_CursorLocked Then PopCursor()
    End Sub
    ''' <summary>
    ''' 输出字符串。
    ''' </summary>
    Public Shared Sub Print(ByVal v As String)
        If m_CursorLocked Then PushCursor()
        Console.WriteLine(v)
        If m_CursorLocked Then PopCursor()
    End Sub

    ''' <summary>
    ''' 输出字符串。
    ''' </summary>
    Public Shared Sub Print(ByVal format As String, ByVal ParamArray args() As Object)
        If m_CursorLocked Then PushCursor()
        Console.WriteLine(format, args)
        If m_CursorLocked Then PopCursor()
    End Sub

    ''' <summary>
    ''' 输出列表。
    ''' </summary>
    Public Shared Sub Print(ByVal v As IEnumerable)
        Dim isFirst As Boolean = True
        For Each EachItem In v
            If isFirst Then
                isFirst = False
            Else
                Console.Write(", ")
            End If
            Console.Write(EachItem)
        Next
        Console.WriteLine()
    End Sub

    ''' <summary>
    ''' 输出错误信息字符串。
    ''' </summary>
    Public Shared Sub PrintError(ByVal v As Object)
        Console.Error.WriteLine(v)
    End Sub

    ''' <summary>
    ''' 输出错误信息字符串。
    ''' </summary>
    Public Shared Sub PrintError(ByVal v As Exception)
        PrintError(v, 0)
    End Sub

    ''' <summary>
    ''' 输出错误信息字符串。
    ''' </summary>
    Public Shared Sub PrintError(ByVal v As Exception, indent As Integer)
        Console.Error.WriteLine("{0}{1}:{2}",
                                    If(indent > 0, StrDup(indent, "-"c) & ">", Nothing),
                                    v.GetType, v.Message)
        If TypeOf v Is AggregateException Then
            For Each EachException In DirectCast(v, AggregateException).InnerExceptions
                PrintError(EachException, indent + 1)
            Next
        End If
    End Sub

    ''' <summary>
    ''' 输出错误信息字符串。
    ''' </summary>
    Public Shared Sub PrintError(ByVal format As String, ByVal ParamArray args() As Object)
        Console.Error.WriteLine(format, args)
    End Sub

    Private Shared PositionStack As New Stack(Of Point)
    Public Shared Sub PushCursor()
        PositionStack.Push(New Point(Console.CursorLeft, Console.CursorTop))
    End Sub
    Public Shared Sub PeekCursor()
        With PositionStack.Peek
            Console.SetCursorPosition(.X, .Y)
        End With
    End Sub
    Public Shared Sub PopCursor()
        PeekCursor()
        PositionStack.Pop()
    End Sub

    ''' <summary>
    ''' 输入文件路径。
    ''' </summary>
    Public Shared Function InputFile(Optional defaultValue As String = Nothing, Optional prompt As String = Nothing) As String
        Dim Path As Uri = Nothing
        Do
            Dim PathStr = UI.Input(If(prompt, "输入文件"), defaultValue).Replace("""", "")
            If Uri.TryCreate(PathStr, UriKind.RelativeOrAbsolute, Path) Then
                If Not Path.IsAbsoluteUri Then
                    Path = New Uri(New Uri(My.Application.Info.DirectoryPath & "\"), Path.ToString)
                    UI.Print(Path.LocalPath)
                End If
                Return Path.LocalPath
            Else
                PrintError("无效的路径。")
            End If
        Loop
    End Function

    ''' <summary>
    ''' 输入文件路径，用于打开。
    ''' </summary>
    Public Shared Function FileOpen(Optional defaultValue As String = Nothing, Optional prompt As String = Nothing) As String
        Dim Path As String
        Do
            Path = InputFile(defaultValue, "打开文件")
            If File.Exists(Path) Then
                Return Path
            Else
                PrintError("找不到文件：{0}。", Path)
            End If
        Loop
    End Function

    ''' <summary>
    ''' 输入文件路径，用于保存。
    ''' </summary>
    Public Shared Function FileSave(Optional defaultValue As String = Nothing, Optional prompt As String = Nothing) As String
        Return InputFile(defaultValue, "保存文件")
    End Function

    Public Shared Sub Pause()
        Console.Write("请按任意键继续。")
        Console.ReadKey()
    End Sub

    Private Sub New()

    End Sub
End Class
