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
IF (X=1)
?? PrintString("This was the 1st item!!")
ELSE IF (X=2)
?? PrintString("This was the 2nd item!!")
ELSE IF (X=3)
?? PrintString("This was the 3rd item!!")
ELSE
?? PrintString("This was the " ++ X ++ "th item!!")
ENDIF
?? SetTextColor(Y0)
?? SetBkColor(Y1)
?? PrintString(char(13) ++ char(10))
RETURN ""
END DEFINE
WHILE 0
ECHO this is a test
WEND
REPEAT
LET I=1
REPEAT
LET J=1
WHILE J<=5
?? JUNK(j)
LET J=J+1
WEND
?? slime(i)
LET I=I+1
UNTIL I>10
exit loop
ECHO this is another test
UNTIL 0
echo 'exit loop' worked ok
