@ECHO OFF
;
;USER-DEFINABLE FUNCTIONS
;
DEFINE DirLine (item)
;
;    USING THE 'FILEINFO()' FUNCTION OBTAIN INFO ABOUT FILE
;    NOTE THAT WE CONCATENATE THE NAME IN 'b[ctr]' ONTO THE
;    FULLY QUALIFIED PATHNAME 'b[0]' USING THE '++' OPERATOR
;
LET c=FILEINFO(item)
;
;    EXTRACT EACH FIELD FROM THE ARRAY 'c'
;
LET name=c[0]
LET attr=c[2]
LET size=c[3]
LET date_time=c[4]
LET type=c[5]
LET dt=DATE(date_time)
LET tm=TIME(date_time)
;
;    ** FORMAT ITEMS FOR DISPLAY **
;    ** DEMO OF FORMAT UTILITIES **
;
LET name=LJ(LEFT(name++"             ",13))
LET attr=LJ(LEFT(attr++"        ",8))
LET size=RJ(LEFT(size++"          ",10))
LET type=LJ(LEFT(type++"        ",8))
;
RETURN name++" "++attr++" "++size++" "++dt++" "++tm++" "++type
END DEFINE
;
;
;
;  BEGINNING OF PROGRAM
;
:MAINLOOP
LET in=INPUT("Enter File specification (Blank line to end): ")
;
;  TRIM OFF UNNECESSARY JUNK (WHITE-SPACE)
;
LET in=LTRIM(RTRIM(in))
;
;  BLANK STRING?  END PROGRAM
;
IF (LEN(in)=0) GOTO THE_END
;
;  GET FILE LISTING INTO ARRAY 'b'
;
LET b=DIRLIST(in)
;
;  IF THERE ARE NO FILES, b[1] WILL BE 'blank'
;
IF (LEN(b[1])) GOTO THERE_ARE_FILES
;
ECHO ?No files matching critieria
GOTO MAINLOOP
;
;
:THERE_ARE_FILES
;
LET c=b[0]
;    ** note - arrays do not expand when placed in '%%' **
;
ECHO Directory for %c%
;
;    INITIALIZE COUNTER FOR LOOP 'ctr'
;
FOR ctr FROM 1 TO NCOLS(b)-1
LET c=DirLine(b[0]++b[ctr])
ECHO %c%
NEXT ctr
;
LET b=""
LET c=""
;
;
ECHO.
ECHO.
GOTO MAINLOOP
;
;  *** WHEN THE USER TYPES A BLANK LINE, THE PROGRAM GOES HERE ***
;
:THE_END
ECHO.
ECHO.
ECHO ** FINISHED **
;
;** end of program **
;
