@ECHO OFF
ECHO.
;odbc connect DSN=SFT_WDBUTIL_SQL;DBQ="C:\\DPT\\TESTDATA";
ODBC CONNECT
if %odbc_result%==ERROR GOTO THE_END
let x=odbc_result
IF NOT (ISYES(INPUT("Do you want to see a list of tables?"))) GOTO OPEN_TABLE
? odbcgettables(x)
:OPEN_TABLE
LET dataset=LTRIM(RTRIM(INPUT("Please enter data set name: ")))
IF "%dataset%"=="" GOTO EXIT_POINT
odbc open %x% %dataset%
if NOT %odbc_result%==ERROR GOTO OPEN_OK
echo.Unable to open data set %dataset%
ODBC ERROR %x%
ECHO.%ODBC_RESULT%
goto OPEN_TABLE
:OPEN_OK
IF NOT (ISYES(INPUT("Do you want to see column info?"))) GOTO BEGIN_READ
? odbcgetcolumninfo(x)
PAUSE
:BEGIN_READ
LET iRow=1
odbc first %x%
if NOT %ODBC_RESULT%==ERROR GOTO MAIN_LOOP
ECHO.?Unable to use 'ODBC FIRST' due to...
ODBC ERROR %x%
ECHO.%ODBC_RESULT%
PAUSE
ODBC NEXT %x%
:MAIN_LOOP
if NOT %ODBC_RESULT%==ERROR GOTO READ_OK
ECHO.?Read error resulting from cursor motion
ODBC ERROR %x%
ECHO.%ODBC_RESULT%
GOTO EXIT_POINT
:READ_OK
if %ODBC_RESULT%==EOF GOTO END_LOOP
ECHO.%(right("        "++iRow,6)++": "++odbcgetdata(x))%
LET iRow=iRow + 1
if NOT (CMD_ERROR) GOTO MAIN_LOOP_NEXT
ECHO.?Error executing 'odbcgetdata()'
ODBC ERROR %x%
ECHO.%ODBC_RESULT%
GOTO EXIT_POINT
:MAIN_LOOP_NEXT
odbc next %x%
GOTO MAIN_LOOP
:END_LOOP
ECHO.* DATA COMPLETE *
ECHO.
:EXIT_POINT
odbc disconnect %x%
:THE_END
