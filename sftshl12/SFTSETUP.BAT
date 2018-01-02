@ECHO OFF
;
; ***************************************************************************
; *                                                                         *
; *    SETUP PROGRAM FOR "SFTShell" Command Line Interpreter for Windows    *
; *                                                                         *
; * Copyright 1994,95 by Stewart~Frazier Tools, Inc. - all rights reserved. *
; *                                                                         *
; ***************************************************************************
;
;
; SETUP.EXE runs SFTSHELL with this file on the command line, as
;    SFTSHELL SFTSETUP.BAT
; using the disk drive on which 'SETUP.EXE' resides.  SFTSHELL then
; procedes to install itself onto the user's machine.
;
;
;* FUNCTION DEFINITIONS *
;
DEFINE ClearMessageArea (x,y)
  ?? SetCursor(x,y)
  ?? PrintString(blanks)
  ?? SetCursor(x,y+1)
  ?? PrintString(blanks)
  ?? SetCursor(x,y+2)
  ?? PrintString(blanks)
  ?? SetCursor(x,y+3)
  ?? PrintString(blanks)
  ?? SetCursor(x,y+4)
  ?? PrintString(blanks)
  ?? SetCursor(x,y)
  RETURN ""
END DEFINE
;
DEFINE ClearMessageArea2 (x,y)
  ?? SetCursor(x,y)
  ?? PrintString(blanks)
  ?? SetCursor(x,y+1)
  ?? PrintString(blanks)
  ?? SetCursor(x,y)
  RETURN ""
END DEFINE
;
DEFINE DisplayHint (x,y,hIndex)
;
;* 'x' and 'y' refer to the 'message area'
;
  LET z2=GetCursor()
  ?? SetTextColor(14)
  ?? SetBkColor(6)
  ?? SetCursor(x,y+2)
  ?? PrintString(blanks)
  ?? SetCursor(x,y+3)
  ?? PrintString(blanks)
  ?? SetCursor(x,y+4)
  ?? PrintString(blanks)
  ?? SetCursor(x,y+2)
  ?? PrintString(hints[hIndex, 0])
  ?? SetCursor(x,y+3)
  ?? PrintString(hints[hIndex, 1])
  ?? SetCursor(z2[0],z2[1])
  ?? SetTextColor(15)
  ?? SetBkColor(4)
  RETURN ""
END DEFINE
;
;
;
;
IF "%WINDIR%"=="" GOTO NOT_SFTSHELL
;
; get window handle for this instance of SFTShell
LET x=GetWindow()
LET hWnd=x[0]
;
; force keystrokes to ensure that the DEFAULT font is in use
;
?? SendVirtualKey(hWnd,ASC("O"),4)
?? SendVirtualKey(hWnd,ASC("D"),0)
;
?? PAUSE(100)
;
; HIDE ME!
;
?? ShowWindow(0)
;
;
LET x=SetTextColor(15)
LET x=SetBkColor(4)
;
;
CLS
LET blanks=STRING(80,32)
;
; DISPLAY the 'cool' ANSI and OEM GRAPHIC amphitheater with ionic columns
;
ECHO.
ECHO.   [1;37;42m         ÉÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ»         [44m
ECHO.   [1;37;42m         º[1;33;40m * Stewart~Frazier Tools, Inc. SFTShell 1.01 Setup * [1;37;42mº         [44m
ECHO.   [1;37;42m         ÈÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍÍ¼         [44m
ECHO.     [1;37;41m [40mÛÞÞÞ[41mÝ[40m                                                        [1;37;41m Þ[40mÝÝÝÛ[41m [44m
ECHO.     [1;37;41m [40mÛÞÞÞ[41mÝ[40m  This program will install and configure your system   [1;37;41m Þ[40mÝÝÝÛ[41m [44m
ECHO.     [1;37;41m [40mÛÞÞÞ[41mÝ[40m  for the SFTShell Windows Command Line Interpreter.    [1;37;41m Þ[40mÝÝÝÛ[41m [44m
ECHO.     [1;37;41m [40mÛÞÞÞ[41mÝ[40m                                                        [1;37;41m Þ[40mÝÝÝÛ[41m [44m
ECHO.     [1;37;41m [40mÛÞÞÞ[41mÝ[40m  If you do not wish to install SFTShell at this time   [1;37;41m Þ[40mÝÝÝÛ[41m [44m
ECHO.     [1;37;41m [40mÛÞÞÞ[41mÝ[40m  you may exit this program at any time by pressing     [1;37;41m Þ[40mÝÝÝÛ[41m [44m
ECHO.     [1;37;41m [40mÛÞÞÞ[41mÝ[40m  the 'CTRL-BREAK' key combination and responding       [1;37;41m Þ[40mÝÝÝÛ[41m [44m
ECHO.     [1;37;41m [40mÛÞÞÞ[41mÝ[40m  'Yes' to terminate the batch file.                    [1;37;41m Þ[40mÝÝÝÛ[41m [44m
ECHO.     [1;37;41m [40mÛÞÞÞ[41mÝ[40m                                                        [1;37;41m Þ[40mÝÝÝÛ[41m [44m
ECHO.     [1;37;41m [40mÛÞÞÞ[41mÝ[40m  The setup program is an example of the type of        [1;37;41m Þ[40mÝÝÝÛ[41m [44m
ECHO.     [1;37;41m [40mÛÞÞÞ[41mÝ[40m  applications that you can write using SFTShell.       [1;37;41m Þ[40mÝÝÝÛ[41m [44m
ECHO.     [1;37;41m [40mÛÞÞÞ[41mÝ[40m                                                        [1;37;41m Þ[40mÝÝÝÛ[41m [44m
ECHO.   [1;30mÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛ[1;37;44m
ECHO.  [47m                                                                           [44m
ECHO. [1;37mÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛÛ[1;37;44m
ECHO.
ECHO.
ECHO.
;
; SHOW ME Maximized!
;
?? ShowWindow(3)
;
; Prepare for input from user (setting up a "cell")
;
LET InstallDir="C:\\SFTSHELL"
:GET_INSTALL_DIR
?? SetTextColor(15)
?? SetBkColor(4)
?? SetCursor(0,21)
?? PrintString("DESTINATION PATH:")
?? SetTextColor(11)
?? SetBkColor(8)
?? SetCursor(18,21)
?? PrintString(LEFT(InstallDir++blanks,56))
?? SetCursor(18,21)
LET InstallDir=LTrim(RTrim(INPUT("",InstallDir)))
;
IF (LEN(InstallDir)=0) GOTO ABNORMAL_TERMINATION
;
; check if the directory exists, or that it's a root directory
;
LET x=FileInfo(InstallDir)
IF (LEN(x)=0 && (LEN(InstallDir)>3 || Right(InstallDir,2)!==":\\")) GOTO NEW_INSTALL_DIR
IF (LEN(x)=0 || INSTR(UPPER(x[2]),"D")!=0) GOTO INSTALL_DIR_OK
;
; * path is NOT a directory - must choose another *
PS /W SystemAsterisk
?? SetTextColor(11)
?? SetBkColor(1)
?? SetCursor(18,22)
?? PrintString("Path name exists, and is not a sub-directory")
GOTO GET_INSTALL_DIR
;
;
:NEW_INSTALL_DIR
;
PS /W SystemQuestion
?? SetTextColor(15)
?? SetCursor(0,23)
;
LET Y=LTrim(RTrim(Input("Path does not exist - create it (Y/n)? ","Y")))
;
?? SetCursor(0,23)
?? SetTextColor(15)
?? SetBkColor(4)
?? PrintString(blanks)
;
IF (Left(UPPER(Y),1)!=="Y" && LEN(Y)!=0) GOTO GET_INSTALL_DIR
MKDIR %InstallDir%
IF (VAL(CMD_ERROR)!=0) GOTO ABNORMAL_TERMINATION
;
:INSTALL_DIR_OK
;
;* one last thing:  get the 'short name' for 'InstallDir' *
;*                  so that I don't confuse PROGMAN (DDE) *
;*                  This is a workaround for paths that   *
;*                  have embedded white space in them.    *
;
LET x=FileInfo(InstallDir)
IF (LEN(X)!=0) LET InstallDir=X[1]
;
IF (Right(InstallDir,1) !== "\\") LET InstallDir=InstallDir++"\\"
;
?? SetTextColor(15)
?? SetBkColor(4)
;
; what drive was I running SFTSHELL from originally?
;
LET x=MODULEINFO("SFTSHELL")
LET SFTShellPath=x[2]
; find the last backslash in the path
LET CTR=LEN(SFTShellPath)
:TEMP_LOOP
IF (CTR==0) GOTO ABNORMAL_TERMINATION
IF (Mid(SFTShellPath,CTR,1)=="\\") GOTO TEMP_FOUND
LET CTR=CTR - 1
GOTO TEMP_LOOP
;
:TEMP_FOUND
;
; 'SFTShellPath' is the exe path including terminating '\'
;
LET SFTShellPath=Left(SFTShellPath,CTR)
;
LET z=GetCursor()
;
LET y="SFTSHELL.EXE,SFTSH32X.EXE,SFTSHELL.HLP,SFTSHELL.WRI,ALARM.BAT,CALCULAT.BAT,"
LET y=y++"CONTROLR.BAT,DIRCOMP.BAT,MYDIR.BAT,ODBCDEMO.BAT,WININFO.BAT"
;
FOR %%x IN (%y%) DO COPY "%SFTShellPath%%%x" "%InstallDir%"
;
?? ClearMessageArea(z[0],z[1])
;?? SetCursor(z[0],z[1])
;?? PrintString(blanks)
;?? SetCursor(z[0],z[1]+1)
;?? PrintString(blanks)
;?? SetCursor(z[0],z[1]+2)
;?? PrintString(blanks)
;?? SetCursor(z[0],z[1]+3)
;?? PrintString(blanks)
;?? SetCursor(z[0],z[1]+4)
;?? PrintString(blanks)
;?? SetCursor(z[0],z[1])
;
;
; create a set of 'hints' for the user to look at while things are going on
;
LET hints[0,0]="SFTShell can run MS-DOS and Windows programs from the command line, in much"
LET hints[0,1]="the same manner as the MS-DOS prompt, but within the Windows environment."
LET hints[1,0]="The enhanced batch language offers many new features over MS-DOS batch files."
LET hints[1,1]="See the on-line HELP and accompanying Windows HELP file for details."
LET hints[2,0]="The sample batch files provide examples on how to use SFTShell's advanced"
LET hints[2,1]="features, such as process control and mathematical calculations."
LET hints[3,0]="Many new features can be directly accessed from the Tool Bar, including the"
LET hints[3,1]="'Folder' dialog box, the 'Queue' dialog box, and custom font selections."
LET hints[4,0]="Having trouble reading SFTShell's text display?  Try using the 'Font' Menu"
LET hints[4,1]="or the Font Tool Bar Button to customize your own font selection!"
LET hints[5,0]="Background copies and background diskette formats allow you to continue to"
LET hints[5,1]="work while SFTShell makes use of idle CPU time."
LET hints[6,0]="The 'Queue' menu allows you to suspend and resume the background operations"
LET hints[6,1]="as well as edit the 'queue' contents."
LET hints[7,0]="For information on a Tool Bar button, place the mouse cursor over it for at"
LET hints[7,1]="least 2 seconds, and a window will appear that describes the button."
;
;
?? PrintString("Copying files..")
LET hints_display=-1
WHILE COPYING=="TRUE"
  ?? PrintString(".")
  ?? Pause(1000)
  ;
  LET hints_display=hints_display + 1
  IF (hints_display >= 3 * NROWS(hints)) LET hints_display=0
  IF ((hints_display MOD 3) = 0)
    ?? DisplayHint(z[0],z[1],INT(hints_display / 3))
  ENDIF
  ;
WEND
;
; NEXT STEP - verify the files!
;
?? SetCursor(z[0],z[1])
?? PrintString(blanks)
?? SetCursor(z[0],z[1])
?? PrintString("COPY COMPLETE!!!")
?? SetCursor(z[0],z[1]+1)
?? PrintString(blanks)
?? SetCursor(z[0],z[1]+2)
?? PrintString(blanks)
?? SetCursor(z[0],z[1]+3)
?? PrintString(blanks)
?? SetCursor(z[0],z[1]+4)
?? PrintString(blanks)
?? PAUSE(1000)
?? SetCursor(z[0],z[1]+1)
?? PrintString("Verifying...")
;
FOR CTR FROM 0 TO NCOLS(y)-1
  ;
;  LET z2=GetCursor()
;  ?? SetTextColor(14)
;  ?? SetBkColor(6)
;  ?? SetCursor(z[0],z[1]+2)
;  ?? PrintString(blanks)
;  ?? SetCursor(z[0],z[1]+3)
;  ?? PrintString(blanks)
;  ?? SetCursor(z[0],z[1]+4)
;  ?? PrintString(blanks)
;  ?? SetCursor(z[0],z[1]+2)
;  ?? PrintString(hints[hints_display / 3, 0])
;  ?? SetCursor(z[0],z[1]+3)
;  ?? PrintString(hints[hints_display / 3, 1])
;  ?? SetCursor(z2[0],z2[1])
;  ?? SetTextColor(15)
;  ?? SetBkColor(4)
  ;
  ?? DisplayHint(z[0],z[1],INT(hints_display / 3))
  ;
  LET t0=NOW()
  IF (FilesMatch(SFTShellPath++y[CTR],InstallDir++y[CTR])=0)
    ;
    ?? ClearMessageArea(z[0],z[1])
    ;
    ?? PAUSE(500)
    ?? SetTextColor(11)
    ?? SetBkColor(1)
    ECHO.
    ECHO.* VERIFY ERROR * on %(SFTShellPath++y[CTR]++","++InstallDir++y[CTR])%
    ECHO.
    ?? SetTextColor(15)
    ?? SetBkColor(4)
    GOTO ABNORMAL_TERMINATION
  ENDIF
  ;
  ?? PrintString(".")
  LET hints_display=hints_display + (NOW() - t0) * 86400
  IF (hints_display >= 3 * NROWS(hints)) LET hints_display=0
  ;
NEXT CTR
;
;

;
:VERIFY_COMPLETE
;
?? ClearMessageArea(z[0],z[1])
;
?? PAUSE(500)
;
ECHO.
ECHO.The files have been copied and verified. Now adding a Program group 
ECHO.and items for SFTShell to the Program Manager.
ECHO.
;
LET z=GetCursor()
;
DDE INITIATE PROGMAN|PROGMAN
IF %DDE_RESULT%==ERROR GOTO NO_PROGMAN
LET DDEConv=DDE_RESULT
;
; set focus to PROGMAN first...
;
LET htaskProgman=GetTask("PROGMAN")
IF (htaskProgman!=0)
  LET CTR=0
  REPEAT
    LET x=GetWindow(htaskProgman,CTR)
    IF (LEN(x)=0) EXIT LOOP
    ; check that the window class does not start with '#', 
    ; that the owner is NULL, and that the window is visible.
    ; also check that the window is enabled.
    IF (Left(X[7],1)!=="#" && X[2]=0 && X[3]!=0 && X[5]!=0) EXIT LOOP
    LET CTR=CTR+1
  UNTIL 0
  IF (LEN(x)!=0)
    ; I found a 'PROGMAN' window that is appropriate.  Set focus
    IF (x[3]=2)  
      ; x[3] is 2 if the window is iconic - if so, restore it
      ?? ShowWindow(x[0],1)
    ELSE
      ; otherwise, merely activate it
      ?? ShowWindow(x[0],5)
    ENDIF
  ENDIF
ENDIF
;
?? ClearMessageArea2(z[0],z[1])
?? PAUSE(500)
;
;** CREATE PROGRAM GROUP **
;
DDE EXECUTE %DDEConv% [CreateGroup(SFTShell)]
;
?? ClearMessageArea2(z[0],z[1])
?? PAUSE(500)
;
DDE EXECUTE %DDEConv% [ShowGroup(SFTShell,1)]
IF (DDE_RESULT!=="OK")
  ECHO.?Unexpected DDE error while creating or displaying 'PROGMAN' group
  GOTO ABNORMAL_TERMINATION
ENDIF
;
?? ClearMessageArea2(z[0],z[1])
?? PAUSE(500)
;
;** CREATE 'SFTShell' GROUP ITEM **
;
; this next operation may fail, but it's not an error if it does
;
DDE EXECUTE %DDEConv% [ReplaceItem(SFTShell)]
IF (DDE_RESULT!=="OK" AND DDE_RESULT!=="NAK")
  ; A 'NAK' is acceptable - other errors are bad
  ECHO.?Unexpected DDE error while adding 'PROGMAN' group item
  GOTO ABNORMAL_TERMINATION
ENDIF
;
?? ClearMessageArea2(z[0],z[1])
?? PAUSE(500)
;
DDE EXECUTE %DDEConv% [AddItem(%InstallDir%SFTSHELL.EXE,SFTShell,,,,,%InstallDir%,,0)]
;
?? ClearMessageArea2(z[0],z[1])
?? PAUSE(500)
;
;
;** CREATE 'Help on SFTShell' GROUP ITEM **
;
; this next operation may fail, but it's not an error if it does
;
DDE EXECUTE %DDEConv% [ReplaceItem(Help on SFTShell)]
IF (DDE_RESULT!=="OK" AND DDE_RESULT!=="NAK")
  ; A 'NAK' is acceptable - other errors are bad
  ECHO.?Unexpected DDE error while adding 'PROGMAN' group item
  GOTO ABNORMAL_TERMINATION
ENDIF
;
?? ClearMessageArea2(z[0],z[1])
?? PAUSE(500)
;
DDE EXECUTE %DDEConv% [AddItem(%InstallDir%SFTSHELL.HLP,Help on SFTShell,,,,,%InstallDir%,,0)]
;
?? ClearMessageArea2(z[0],z[1])
?? PAUSE(500)
;
;
;** CREATE 'Read Me' GROUP ITEM **
;
; this next operation may fail, but it's not an error if it does
;
DDE EXECUTE %DDEConv% [ReplaceItem(Read Me)]
IF (DDE_RESULT!=="OK" AND DDE_RESULT!=="NAK")
  ; A 'NAK' is acceptable - other errors are bad
  ECHO.?Unexpected DDE error while adding 'PROGMAN' group item
  GOTO ABNORMAL_TERMINATION
ENDIF
;
?? ClearMessageArea2(z[0],z[1])
?? PAUSE(500)
;
DDE EXECUTE %DDEConv% [AddItem(%InstallDir%SFTSHELL.WRI,Read Me,,,,,%InstallDir%,,0)]
;
?? ClearMessageArea2(z[0],z[1])
?? PAUSE(500)
;
;
;* TERMINATE DDE CONVERSATION *
;
DDE TERMINATE %DDEConv%
;
?? ClearMessageArea2(z[0],z[1])
?? PAUSE(500)
;
LET DDEConv=""
;
GOTO COMPLETED_OK
;
:NO_PROGMAN
ECHO.
ECHO.WARNING - 'PROGMAN' not running - unable to add 'SFTShell' Program Group 
ECHO.
;
:COMPLETED_OK
ECHO.
ECHO.The SETUP process for SFTShell is now complete!  SFTShell will now open
ECHO.the 'Read Me' file (SFTSHELL.WRI), which contains some important
ECHO.information about SFTShell.
ECHO.
;
"%InstallDir%SFTSHELL.WRI"
?? PAUSE(2500)
;
GOTO NORMAL_TERMINATION
;
:ABNORMAL_TERMINATION
;
?? SetTextColor(15)
?? SetBkColor(4)
?? SetCursor(0,25)
;
ECHO.THE SFTShell SETUP PROCESS IS NOT COMPLETE!  You will need to complete
ECHO.the setup process in order to use SFTShell.
ECHO.
:NORMAL_TERMINATION
;
; bring the focus back HERE
;
?? PAUSE(1500)
?? ShowWindow(3)
?? ShowWindow(5)
;
PAUSE Press <ENTER> to terminate this program
EXIT
;
:NOT_SFTSHELL
ECHO You must *NOT* run this program from an MS-DOS prompt!
:END_OF_PROGRAM
