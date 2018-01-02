@ECHO OFF
;
;* FUNCTION DEFINITIONS *
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
LET opn=c[11]
IF (c[8])
LET cmpr=c[7]/c[8]
ELSE
LET cmpr=0
ENDIF
;
;    ** FORMAT ITEMS FOR DISPLAY **
;    ** DEMO OF FORMAT UTILITIES **
;
LET name=LJ(LEFT(name++STRING(13,32),13))
LET attr=LJ(LEFT(attr++STRING(8,32),8))
LET size=RJ(LEFT(size++STRING(10,32),10))
LET type=LJ(LEFT(type++STRING(8,32),8))
LET dttm=LJ(LEFT(dt++" "++tm++STRING(22,32),22))
IF (opn!=0)
  LET opn=" *  "
ELSE
  LET opn="    "
ENDIF
IF (cmpr)
  LET cmpr=ROUND(cmpr * 10)/10 + .01
  LET cmpr=RJ(LEFT(LEFT(cmpr,LEN(cmpr)-1)++STRING(5,32),5))
ELSE
  LET cmpr=STRING(6,32)
ENDIF
;
RETURN name++" "++attr++" "++size++" "++dttm++" "++type++" "++opn++" "++cmpr
END DEFINE
;
;* MAIN PROGRAM *
;
REPEAT
;
LET in=INPUT("Enter File specification (Blank line to end): ")
;
;  TRIM OFF UNNECESSARY JUNK (WHITE-SPACE)
;
LET in=LTRIM(RTRIM(in))
;
;  BLANK STRING?  END PROGRAM
;
IF (LEN(in)=0) EXIT LOOP
;
;  GET FILE LISTING INTO ARRAY 'b'
;
LET b=DIRLIST(in)
;
;  IF THERE ARE NO FILES, b[1] WILL BE 'blank'
;
IF (LEN(b[1])==0)
;
; no matching files - continue prompting for file specs
;
ECHO ?No files matching critieria
CONTINUE
ENDIF
;
; note: must place array in () to use %% substitutions
;
ECHO.Directory for %(b[0])%
ECHO.
ECHO.FileName.Ext Attrib   File Size   Last Write Date/Time     Type   OPEN COMPR
ECHO.------------ ------- ------------ ---------------------- -------- ---- ------
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
; loop will repeat indefinitely until 'EXIT LOOP' is executed
UNTIL 0
;
;  *** WHEN THE USER TYPES A BLANK LINE, THE PROGRAM GOES HERE ***
;
ECHO.
ECHO.
ECHO ** FINISHED **
;
;** end of program **
;
