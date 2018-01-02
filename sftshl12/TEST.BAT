@ECHO OFF
DEFINE junk (X)
LET Y0=GetTextColor()
LET Y1=GetBkColor()
?? SetTextColor(15)
?? SetBkColor(0)
?? PrintString("This is " ++ X ++ "!!")
?? SetTextColor(Y0)
?? SetBkColor(Y1)
?? PrintString(char(13) ++ char(10))
RETURN ""
END DEFINE
DEFINE slime (X)
LET Y0=GetTextColor()
LET Y1=GetBkColor()
?? SetTextColor(0)
?? SetBkColor(7)
?? PrintString("This was " ++ X ++ "!!")
?? SetTextColor(Y0)
?? SetBkColor(Y1)
?? PrintString(char(13) ++ char(10))
RETURN ""
END DEFINE
;FOR I FROM 1 to 10 do ?? JUNK(i)
FOR I FROM 1 to 10
FOR J FROM 1 to 5
?? JUNK(j)
NEXT J
?? slime(i)
NEXT I
