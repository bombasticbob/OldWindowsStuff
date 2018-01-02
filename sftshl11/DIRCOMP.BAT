@ECHO OFF
;
; see which files match between directories
;
;
; ** MAIN PROGRAM **
;
SET P1=%1
SET P2=%2
IF (LEN(P2)=0)
  LET P2=P1
  LET P1="."
ENDIF
LET pp1a=FILEINFO(P1)
LET pp1=P1
IF (LEN(P1)>18) LET pp1=LEFT(pp1a[1],3)++"...\\"++pp1a[0]
LET pp2a=FILEINFO(P2)
LET pp2=P2
IF (LEN(P2)>18) LET pp2=LEFT(pp2a[1],3)++"...\\"++pp2a[0]
SET pp1a=
SET pp2a=
SET FILE_MATCH=
SET FILE_DIFF=
SET FILE_DIFF2=
;
;* DISPLAY HEADINGS *
;
ECHO.FILE NAME                %(LEFT(pp1++"                  ",18))%  %(LEFT(pp2++"                  ",18))%  FLAG
ECHO.=======================  ==================  ==================  ====
;
;* OBTAIN FULLY QUALIFIED PATHS using long names *
;
LET P1=TRUENAME(P1)
LET P2=TRUENAME(P2)
IF (RIGHT(P1,1)!=="\\") LET P1=P1++"\\"
IF (RIGHT(P2,1)!=="\\") LET P2=P2++"\\"
LET x=DIRLIST(P1++"*.*","")
LET x2=NCOLS(x) - 1
FOR x1 FROM 0 TO x2
  ;
  LET Y=FILEINFO(P1++x[x1])
  ;
  ; ignore sub-directories
  IF (INSTR(1,Y[2],"D")<>0) CONTINUE
  ;
  ; get the long name (Y[6])
  ;
;  LET Y0=GetName(Y[6])
  LET Y0=Y[6]
  WHILE INSTR(1,Y0,"\\")<>0
    LET Y0=MID(Y0,INSTR(1,Y0,"\\")+1)
  WEND
  ; 
  ;
  ; see if the file exists in the 2nd path
  IF NOT EXIST %P2%%(Y0)% CONTINUE
  ;
  ; get file information on file in 2nd dir
  LET Z=FILEINFO(P2++Y0)
  ;
  ; verify file sizes/dates, and binary comparison
  ;
  IF (Y[3]!=Z[3] || Y[4]!=Z[4])
    ;
    ; FILES DO NOT MATCH
    IF (Y[3] == Z[3])
      IF (FILESMATCH(Y[1],Z[1])) 
         LET XX=" == "
         GOTO FILES_MATCH2
      ENDIF
    ENDIF

    IF (Y[4] > Z[4])
      LET XX=" >* "
    ELSE
      LET XX=" *< "
    ENDIF

    LET FILE_DIFF[NCOLS(FILE_DIFF)]=Y0
  ELSE IF (FILESMATCH(Y[1],Z[1]))
    ;
    ; * FILES MATCH! *
    LET XX="    "
    ;
:FILES_MATCH2
    ;
    LET FILE_MATCH[NCOLS(FILE_MATCH)]=Y0
  ELSE
    ;
    ; DATES AND SIZES MATCH, BUT CONTENTS DO NOT
    LET XX=" !! "
    LET FILE_DIFF2[NCOLS(FILE_DIFF2)]=Y0
  ENDIF
  ;
  ; ** DISPLAY THE RESULTS
  ;
  IF (LEN(Y0) > 24)
    ECHO.%(Y0)%
    ECHO.%("                        ")% %(RIGHT("        "++Y[3],7))% %(DATE(Y[4]))%  %(RIGHT("        "++Z[3],7))% %(DATE(Z[4]))% %XX%
  ELSE
    ECHO.%(LEFT(Y0++"                        ",24))% %(RIGHT("        "++Y[3],7))% %(DATE(Y[4]))%  %(RIGHT("        "++Z[3],7))% %(DATE(Z[4]))% %XX%
  ENDIF
;
; * end of main loop *
;
NEXT X1
;
ECHO.
REM * do cleanup *
SET X=
SET Y=
SET Z=
SET XX=
SET X1=
SET X2=
SET P1=
SET P2=
REM
REM * on exit, 'FILE_MATCH' contains matching files *
REM * and 'FILE_DIFF' contains files that changed   *
REM
RETURN
