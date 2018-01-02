@ECHO OFF
;
ECHO.******************************************************
ECHO.*           ODBC DEMO PROGRAM for SFTShell           *
ECHO.******************************************************
ECHO.
ECHO.******************************************************
ECHO.* Performing a 'generic' connect to the ODBC drivers *
ECHO.******************************************************
ECHO.
;
ODBC CONNECT
if %odbc_result%==ERROR GOTO THE_END
let x=odbc_result
? "CONNECT STRING: "++odbcconnectstring(x)
ECHO.
REPEAT
IF (ISYES(INPUT("Do you want to see a list of tables?")))
  LET y=odbcgettables(x)
  IF (CMD_ERROR)
    ODBC ERROR %x%
    ? ODBC_RESULT
    PAUSE
  ELSE
    LET n_col=NCOLS(y) - 1
    FOR row FROM 0 TO NROWS(y)-1
      LET z=getcursor()
      FOR col FROM 0 to n_col
        ?? setcursor(col * 20,z[1])
        ?? PrintString(y[row,col])
      NEXT col
      ECHO.
    NEXT row
  ENDIF
ENDIF
;
;* open table and allow user to optionally view fields, then list *
;* each row's contents as it is fetched, using 'ODBCGetData()'    *
;
LET dataset=LTRIM(RTRIM(INPUT("Please enter data set name: ")))
IF "%dataset%"=="" CONTINUE
odbc open %x% %dataset%
IF NOT %odbc_result%==ERROR GOTO OPEN_OK
echo.Unable to open data set %dataset%
ODBC ERROR %x%
ECHO.%ODBC_RESULT%
goto OPEN_TABLE
;
;
;
:OPEN_OK
IF NOT (ISYES(INPUT("Do you want to see column info?"))) GOTO BEGIN_READ
? odbcgetcolumninfo(x)
PAUSE
;
;
;
:BEGIN_READ
LET iRow=1
odbc first %x%
IF (ODBC_RESULT=="ERROR")
  ECHO.?Unable to use 'ODBC FIRST' due to...
  ODBC ERROR %x%
  ECHO.%ODBC_RESULT%
  PAUSE
  ODBC NEXT %x%
ENDIF
;
;
;
WHILE ODBC_RESULT!=="EOF"
  IF (ODBC_RESULT=="ERROR")
    ECHO.?Read error resulting from cursor motion
    ODBC ERROR %x%
    ECHO.%ODBC_RESULT%
    EXIT LOOP
  ENDIF
  ;
  ECHO.%(right("        "++iRow,6)++": "++odbcgetdata(x))%
  IF (CMD_ERROR)
    ECHO.?Error executing 'odbcgetdata()'
    ODBC ERROR %x%
    ECHO.%ODBC_RESULT%
    EXIT LOOP
  ENDIF
  ;
  LET iRow=iRow + 1
  ODBC NEXT %x%
WEND
;
ECHO.* DATA COMPLETE *
ECHO.
;
UNTIL ISNO(INPUT("Do you wish to view another table (Y/n)?"))
odbc disconnect %x%
:THE_END
