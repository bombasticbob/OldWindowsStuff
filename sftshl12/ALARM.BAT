@ECHO OFF
LET esc=char(27)
LET blanks="                                                      "
REM
REM set up ANSI color strings
REM
LET wht_on_red=esc++"[1;37;41m"
LET yel_on_red=esc++"[1;33;41m"
LET cyn_on_blu=esc++"[1;36;44m"
REM
ECHO.
ECHO. %wht_on_red%%(CENTER(LEFT("** The ALARM Batch File **"++blanks,64)))%%cyn_on_blu%
ECHO.
REM
REM determine if the user specified a time on the command line,
REM and get input from user if not
REM
LET WAIT_UNTIL="%1"
IF NOT (LEN(WAIT_UNTIL)) LET WAIT_UNTIL=LTRIM(RTRIM(INPUT("WHEN SHOULD I WAIT UNTIL?")))
IF (LEN(WAIT_UNTIL)=0) RETURN
IF (TIMEVAL(WAIT_UNTIL)=0) GOTO INVALID_TIME
REM
REM if the current time is AFTER the time to wait for, we'll be doing
REM our wait until TOMORROW.  Warn the user in this case.
REM
IF (TIMEVAL(WAIT_UNTIL)<=NOW()) ECHO ?Warning - current time is %(TIME(NOW()))%
WAIT UNTIL %WAIT_UNTIL%
REM
REM ** at this point the WAIT is complete **
REM
REM ** make sure I MAXIMIZE this window! **
REM
LET a=SHOWWINDOW(3)
:INFINITE_LOOP
REM
REM USING THE 'FOR' COMMAND, PLAY EVERY '.WAV' FILE IN 'C:\WINDOWS'
REM
FOR %%X in (%WINDIR%\*.WAV) DO PS /W %%X
REM
REM BECAUSE I WANT TO 'WAKE UP' THE USER, KEEP GOING 'TILL THE USER
REM PRESSES CTRL-BREAK TO HALT THE BATCH PROCESS.
REM
GOTO INFINITE_LOOP
REM
REM
:INVALID_TIME
REM
LET msg1="** TIME VALUE \"%WAIT_UNTIL%\" IS NOT VALID **"
REM
ECHO %yel_on_red%%(CENTER(LEFT(msg1++blanks,64)))%%cyn_on_blu%
ECHO %(CENTER(LEFT("** OPERATION ABORTED **"++blanks,64)))%
REM
REM EXAMPLE OF A 'CLEANUP' SECTION - GET RID OF VARIABLES USED BY
REM THIS BATCH FILE SO THAT WE DON'T CLUTTER THE ENVIRONMENT
REM
SET msg1=
SET blanks=
SET yel_on_red=
SET cyn_on_blu=
SET wht_on_red=
SET WAIT_UNTIL=
