@ECHO OFF
LET old_bk=GETBKCOLOR()
LET old_text=GETTEXTCOLOR()
REM
REM new colors - white on blue
REM
LET void=SETBKCOLOR(4)++SETTEXTCOLOR(15)
CLS
LET blanks="                                                                                "
LET void=PRINTSTRING(CENTER(LEFT("** CALCULATOR PROGRAM **"++blanks,80)))
LET void=SETCURSOR(5,2)++SETTEXTCOLOR(11)++PRINTSTRING("RESULT:")
LET void=SETCURSOR(5,4)++SETTEXTCOLOR(14)++PRINTSTRING("EQUATION:")
REM
:MAINLOOP
REM
LET void=SETCURSOR(0,7)++SETTEXTCOLOR(15)++SETBKCOLOR(4)
LET eqn=LTRIM(RTRIM(INPUT("Enter equation (<ENTER> to exit): ")))
IF (LEN(eqn)=0) GOTO THE_END
REM
REM use 'printstring()' to clear out 2 rows using ANSI sequence
REM
LET void=SETCURSOR(0,7)++PRINTSTRING(CHAR(27)++"[K")
LET void=SETCURSOR(0,8)++PRINTSTRING(CHAR(27)++"[K")
REM
REM display the equation beneath the 'equation' label
REM color is BLACK on LIGHT GREY
REM
LET void=SETCURSOR(5,5)++SETTEXTCOLOR(0)++SETBKCOLOR(7)
LET void=PRINTSTRING(LJ(LEFT(eqn++blanks,70)))
REM
REM evaluate equation
REM
LET rval=%eqn%
REM
REM display the result to the right of the 'RESULT:' label
REM color is BRIGHT YELLOW on DARK RED
REM
LET void=SETCURSOR(12,2)++SETTEXTCOLOR(11)++SETBKCOLOR(1)
LET void=PRINTSTRING(LJ(LEFT(rval++blanks,60)))
REM
REM continue until user presses <ENTER> (blank line)
REM
GOTO MAINLOOP
REM
:THE_END
REM
LET void=SETCURSOR(0,10)++SETTEXTCOLOR(old_text)++SETBKCOLOR(old_bk)
ECHO.
ECHO. ** COMPLETE **
ECHO.
SET blanks=
SET void=
SET eqn=
SET rval=
