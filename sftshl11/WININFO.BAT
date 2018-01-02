@ECHO OFF
;
; ** APPLICATION CHILD WINDOW INFO **
;
IF ("%1"=="")
  LET app=LTRIM(RTRIM(INPUT("ENTER TASK, INSTANCE, MODULE, or APPLICATION NAME: ")))
ELSE
  LET app="%1"
ENDIF
IF (LEN(app)=0) GOTO THE_END
REM
LET blanks=STRING(32,32)
;
;* MAIN LOOP *
;
FOR ctr FROM 0 to 65535
  LET a=GETWINDOW(app,ctr)
  IF (LEN(a)=0)
    IF (ctr=0) ECHO.??WARNING - NO MATCH FOUND FOR "%app%"!
    EXIT LOOP
  ENDIF
;
; ** DISPLAY THE WINDOW INFO HERE! **
;
  IF (ctr=0) 
    ECHO.HANDLE      TITLE                   OWNER   CLASS NAME
    ECHO.---------------------------------------------------------------------
  ENDIF
;
  LET line=LJ(LEFT(a[0]++blanks,8))++"    "++LJ(LEFT(a[1]++blanks,24))
  LET line=line++LJ(LEFT(a[2]++blanks,8))++"\""++a[7]++"\""
;
  ECHO.%line%
  FOR ctr2 FROM 0 TO 65535
    LET b=GETCHILDWINDOW(a[0],ctr2)
    IF (b[0]=a[0]) 
      CONTINUE
    ELSE IF (LEN(b)=0) 
      EXIT LOOP
    ENDIF
    LET line="  "++LJ(LEFT(b[0]++blanks,8))++"  "++LJ(LEFT(b[1]++blanks,24))
    IF (CMD_ERROR) ECHO %b%
    LET line=line++LJ(LEFT(b[2]++blanks,8))++"\""++b[7]++"\""
    IF (CMD_ERROR) ECHO %b%
    ECHO.%line%
    FOR ctr3 FROM 0 TO 65535
      LET c=GETCHILDWINDOW(b[0],ctr3)
      IF (LEN(c)=0) 
        EXIT LOOP
      ELSE IF (c[0]=b[0]) 
        CONTINUE
      ENDIF
      LET line="    "++LJ(LEFT(c[0]++blanks,8))++LJ(LEFT(c[1]++blanks,24))
      IF (CMD_ERROR) ECHO %c%
      LET line=line++LJ(LEFT(c[2]++blanks,8))++"\""++c[7]++"\""
      IF (CMD_ERROR) ECHO %c%
      ECHO.%line%
    NEXT ctr3
  NEXT ctr2
NEXT ctr
;
ECHO.
:THE_END
SET ctr=
SET a=
SET b=
SET c=
SET blanks=
SET app=
