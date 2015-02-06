Module Utility
    Public Function PadRight(s As String, length As Integer) As String
        If length < 0 Then Throw New ArgumentOutOfRangeException("length")
        If length = 0 Then Return ""
        If s = Nothing Then Return Space(length)
        Dim lastspaceLength = 0
        Dim spaceLength = length
        For I = 0 To Len(s) - 1
            If AscW(s(I)) > &HFF Then
                spaceLength -= 2
            Else
                spaceLength -= 1
            End If
            If spaceLength < 0 Then
                'length 没地方了
                '当前字符 s(I) 就不显示了
                '显示到前一个字符为止。
                Return s.Substring(0, I) & Space(lastspaceLength)
            End If
            lastspaceLength = spaceLength
        Next
        Return s & Space(spaceLength)
    End Function
End Module
