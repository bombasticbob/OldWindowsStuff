;***************************************************************************
;*                                                                         *
;*   WINCMDH.ASM - Command line interpreter for Microsoft(r) Windows (tm)  *
;*                                                                         *
;*         (c) 1991-95 by R. E. Frazier  -  all rights reserved            *
;*                                                                         *
;*                 Help file stuff for the WINCMD system!!!                *
;*                                                                         *
;***************************************************************************


TITLE Help file stuff for the SFTSHELL system!!!


         ;***************** MACRO DEFINITIONS *******************

CR   EQU 0dH
LF   EQU 0aH
CRLF EQU <CR,LF>


NOT_SUPPORT MACRO FN_NAME
&FN_NAME LABEL BYTE
   DB "The function '&FN_NAME' has been trapped by the Windows(tm) Command Line", CRLF
   DB "Interpreter.  If you wish to run &FN_NAME, please do so prior to loading", CRLF
   DB "Windows(tm).  The use of this program (as it would be under DOS) has been",CRLF
   DB "trapped to prevent potential corruption of the system while running",CRLF
   DB "Windows(tm).  See the MS-DOS(r) technical reference for more information.", CRLF
   DB 0
   ENDM

NOT_INTERNAL MACRO CMD_STRING
&CMD_STRING LABEL BYTE
   DB "&CMD_STRING [args]",CRLF,LF
   DB "This command is not currently performed internally by the Windows(tm)",CRLF
   DB "Command Line Interpreter. Its entries in the command table and HELP text",CRLF
   DB "are included for future expansion.  The equivalent MS-DOS(r) program is",CRLF
   DB "executed in its place.",CRLF
   DB "For information regarding parameters and switches, see the MS-DOS(r)",CRLF
   DB "User's Guide and Reference regarding this program.",CRLF
   DB 0
   ENDM


         ;***************** REGULAR CODE/DATA *******************

PUBLIC _HELP_START_,_HELP_START2_
PUBLIC _HELP_COMMANDS_,_HELP_KEYS_,_HELP_VARIABLES_
PUBLIC _HELP_ANSI_



_HELP_TEXT_ SEGMENT PARA PUBLIC 'CODE'   ; **CODE** same as fn that reads it


_HELP_START_ LABEL WORD

   DW    0                             ; the other one has the value below
   DW    (_HELP_END_ - _HELP_START_) / 2 - 2

   DW    APPEND,   ASSIGN,   ATTRIB,   BREAK,    CALC
   DW    _CALL,    CD,       CHCP,     CHDIR,    CLOSETASK
   DW    CLS,      COMMAND,  COMP
   DW    CONTINUE, COPY,     CLOSETASK,CTTY,     DATE,     DBLSPACE
   DW    DDE,      DEBUG,    DEFINE,   DEL,      DELTREE,  DIR
   DW    DOSKEY,   _ECHO,    EDIT,     _ELSE,    _END,     _ENDIF
   DW    ERASE,    EXIT,     EXPAND,   FC,       FIND,     _FOR
   DW    FORMAT,   _GOTO,    GRAFTABL, GRAPHICS, HELP,     _IF
   DW    _INPUT,   JOIN,     KEYB,     KILLTASK, KILLTASK, _LABEL
   DW    LET,      LH,       LISTOPEN, LISTOPEN, LOAD,     LOADHIGH
;   DW    _LOOP

_HELP_END_ DW 0     ; marks end of the list!!



NOT_SUPPORT APPEND

NOT_SUPPORT ASSIGN

ATTRIB   LABEL BYTE
   DB "ATTRIB [+r|-r] [+a|-a] [+s|-s] [+h|-h] [pathname] [/s]",CRLF,LF
   DB "ATTRIB [+r|-r] [+a|-a] [+s|-s] [+h|-h] [pathname]",CRLF,LF
   DB "The 'ATTRIB' command allows you to either view or change the attributes",CRLF
   DB "of a DOS file.",CRLF,LF
   DB "PARAMETERS:    pathname  - valid DOS path (default = *.*)",CRLF,LF
   DB "SWITCHES:      +r        - set 'read only' attribute",CRLF
   DB "               -r        - clear 'read only' attribute",CRLF
   DB "               +a        - set 'archive' attribute",CRLF
   DB "               -a        - clear 'archive' attribute",CRLF
   DB "               +s        - set 'system file' attribute",CRLF
   DB "               -s        - clear 'system file' attribute",CRLF
   DB "               +h        - set 'hidden' attribute",CRLF
   DB "               -h        - clear 'hidden' attribute",CRLF
   DB "               /S        - recurse sub-directories",CRLF
   DB CRLF
   DB "On output, the left 8 characters of each line will contain various letters",CRLF
   DB 'indicating which attribute bits are "on" for the particular entry.',CRLF
   DB "The following is a list and explanation of each:",CRLF
   DB "               A  - 'archive' bit (file has been modified since backup).",CRLF
   DB "               R  - 'read-only' bit.  This file may not be modified.",CRLF
   DB "               H  - 'hidden' bit.  File does not show up in normal 'DIR'",CRLF
   DB "                    listings.",CRLF
   DB "               S  - 'system' bit.  File is used by the operating system.",CRLF
   DB "               V  - 'volume ID' bit.  This is only set for volume labels.",CRLF
   DB "               D  - 'directory' bit.  This is a sub-directory entry.",CRLF,CRLF
   DB " NOTE:  to view files with specific attributes only, use the '/A'",CRLF
   DB "        switch on the 'DIR' command (type 'HELP DIR' for more info).",CRLF,CRLF
   DB 0

BREAK    LABEL BYTE
   DB "BREAK [ON|OFF]",CRLF,LF
   DB "The 'BREAK' command allows you to enable/disable/view the ability to",CRLF
   DB "stop an operation via the 'Ctrl-Break' key combination.",CRLF,LF
   DB "PARAMETERS:     ON       - enables Ctrl-Break checking (default)",CRLF
   DB "                OFF      - disables Ctrl-Break checking",CRLF,LF
   DB "Specifying the 'BREAK' command with no parameter displays the current",CRLF
   DB "setting.  Note that this only applies for functions that are internal",CRLF
   DB "to the Windows(tm) Command Line Interpreter, and does not include any",CRLF
   DB "background operations (such as 'COPY').  For further information see", CRLF
   DB "the respective HELP display for the command in question.",CRLF
   DB 0


CALC      LABEL BYTE
   DB "CALC [/Q] expression",CRLF
   DB "? expression",CRLF
   DB "?? expression",CRLF,LF
   DB "Calculates and displays the result of an expression.  For more information,",CRLF
   DB "see the 'HIGH LEVEL LANGUAGE' overview in 'SFTSHELL.HLP'.",CRLF,LF
   DB "This command is useful for displaying the results of a particular expression,",CRLF
   DB "as opposed to assigning the value to a variable (as with the 'LET' command).",CRLF
   DB "When called from within a BATCH program a single <CR><LF> sequence is added",CRLF
   DB "following the return value from 'expression', which may also contain such",CRLF
   DB "sequences, cursor control functions, I/O functions, and so on.  If it is",CRLF
   DB "desired to perform a particular 'expression' function, and output of the",CRLF
   DB "result would degrade from the performance of the command (as it would be for",CRLF
   DB "cursor control functions or output functions) the '/Q' (i.e. 'Quiet') switch",CRLF
   DB "preceding the expression inhibits output.  The result may also be assigned to",CRLF
   DB "a 'dummy' variable using the 'LET' command; i.e. 'LET dummy=expression'.",CRLF,LF
   DB "The '?' command is a synonym for 'CALC', and the '??' command a synonym",CRLF
   DB "for 'CALC /Q'.  Use of '?' and '??' is both for convenience and readability",CRLF
   DB "of batch programs using 'CALC'.",CRLF,LF
   DB "See also the 'LET' and 'IF' commands, and the 'VARIABLES' discussion.",CRLF
   DB 0

_CALL     LABEL BYTE
   DB "CALL pathname",CRLF,LF
   DB "This command may be used either within a batch file, or from the command",CRLF
   DB "line to run programs 'synchronously'.  It will begin executing a program",CRLF
   DB "and WAIT until it completes before continuing.  If this command is invoked",CRLF
   DB "from within a batch file, control returns to the statement that follows",CRLF
   DB "the CALL command.  The 'CALL' commands from within batch files may also be",CRLF
   DB "used in 'CALL'ed batch files if desired, limited only by the available stack",CRLF
   DB "space.  For an additional method of waiting for programs to complete, type",CRLF
   DB "'HELP IF' at the command prompt, specifically regarding 'ISTASK' operator.",CRLF
   DB "NOTE:  The 'CALL' command is the only method by which a batch file can run",CRLF
   DB "       a subsequent batch file and have it return to the caller.  If the",CRLF
   DB "       'CALL' command is not used, the current batch file will terminate",CRLF
   DB "       at the point the new batch file begins, and be substituted by the",CRLF
   DB "       new batch file (similar to 'CHAIN' in certain BASIC languages).",CRLF
   DB CRLF
   DB "When an application is executed via 'CALL', the RETVAL environment variable",CRLF
   DB "contains the return code from the application, similar to ERRORLEVEL in",CRLF
   DB "COMMAND.COM, except that the value persists until another command causes",CRLF
   DB "it to be re-assigned.",CRLF
   DB CRLF
   DB "See also:  LOAD, START, RETURN",CRLF
   DB LF,0


CD       LABEL BYTE
CHDIR    LABEL BYTE
   DB "CD pathname",CRLF
   DB "CHDIR pathname",CRLF,LF
   DB "This function changes directories to the specified pathname for the",CRLF
   DB "specified drive (default is current drive).  If no 'pathname' parameter",CRLF
   DB "is specified, the current drive:pathname is displayed.",CRLF
   DB 0


NOT_SUPPORT CHCP


CLOSETASK LABEL BYTE
   DB "CLOSETASK task|instance|window|module name",CRLF,CRLF
   DB "This function allows you to close a task using either the Task 'ID', the",CRLF
   DB "Main Window 'ID', or the MODULE NAME (enhancement from prior release).",CRLF
   DB "To obtain a list of running tasks and their associated MODULE NAMEs,",CRLF
   DB "Task 'ID's see the 'TASKLIST' function.  The preferred method uses the",CRLF
   DB "MODULE NAME (normally the same as the Program Name, without the extension)",CRLF
   DB "to identify the task to be closed.  If more than one 'instance' of the same",CRLF
   DB "program is running a list of these tasks, their Task 'ID's, and their Main",CRLF
   DB "Window 'Captions' (titles) will appear, with sequence numbers to the left.",CRLF
   DB "The program then prompts which one to delete.  You may respond by entering",CRLF
   DB "the appropriate sequence number (1,2,3,etc.) or by pressing <ENTER> to",CRLF
   DB "cancel the operation.  For Task 'ID's and Main Window 'ID's there will",CRLF
   DB "always be a unique task associated with it.",CRLF
   DB "The Task 'ID' is normally obtained as a result of running a program, in",CRLF
   DB "which the Task 'ID' value was returned in the 'TASK_ID' environment",CRLF,LF
   DB "If no valid Main Window exists for a given task handle, but the task is",CRLF
   DB "valid, then you must use either 'KILLTASK' or 'NUKETASK' to end it.",CRLF
   DB "This application closes tasks in the same manner as using the TASK MANAGER",CRLF
   DB "application (runs via Ctrl-Esc, 'Switch To' system menu, or double clicking",CRLF
   DB "the desktop).",CRLF,CRLF
   DB "See also:  KILLTASK, NUKETASK",CRLF,CRLF
   DB 0


CLS      LABEL BYTE
   DB "CLS",CRLF,LF
   DB "This command causes the screen to be cleared.  The attribute byte for the",CRLF
   DB "last displayed character is used to determine the screen background and",CRLF
   DB "text colors.",CRLF,LF
   DB "  NOTE:  The Windows(tm) Command Line Interpreter allows imbedded ANSI",CRLF
   DB "         escape sequences in the PROMPT and ECHO commands to force the",CRLF
   DB "         screen background and text colors to a specific value.  Some",CRLF
   DB "         ANSI sequences (such as BLINKING TEXT) are not supported.",CRLF
   DB "         See the MS-DOS(r) User's Guide and Reference for information",CRLF
   DB "         regarding ANSI escape sequences under the section for the",CRLF
   DB "         ANSI.SYS device driver.",CRLF
   DB 0


COMMAND  LABEL BYTE
   DB "COMMAND [command string]",CRLF,LF
   DB "Runs the program specified in the COMSPEC environment variable; if this",CRLF
   DB "environment variable is not present, it attempts to run 'COMMAND.COM'.",CRLF
   DB "The argument specified in 'command string' is passed to the program as",CRLF
   DB "it's command line argument.",CRLF,LF
   DB "NOTE:  SFTShell fully supports piping and redirection, even to MS-DOS",CRLF
   DB "       programs.  SFTShell automatically invokes 'COMMAND /C' for each",CRLF
   DB "       MS-DOS application that's included in the command.  All output to",CRLF
   DB "       the screen (i.e. 'standard output', or the 'CON' device) from a",CRLF
   DB "       'piped' MS-DOS program will be displayed on SFTShell's screen.",CRLF
   DB 0


NOT_INTERNAL COMP


CONTINUE LABEL BYTE
   DB "CONTINUE",CRLF,LF
   DB "Skips to the end of the current FOR/NEXT loop, WHILE/WEND loop, or",CRLF
   DB "REPEAT/UNTIL loop.",CRLF,LF
   DB 0


COPY     LABEL BYTE
   DB "COPY [/a|/b] source[/a|/b][+source2[/a|/b][+...]] [dest[/a|/b]][/v][/w]",CRLF,LF
   DB "This command will copy a file from one location/device to another.  As an",CRLF
   DB "enhancement to the normal DOS 'COPY' command, this operation is done in",CRLF
   DB "BACKGROUND to avoid tying up the user interface for such operations.  As",CRLF
   DB "more file sets are selected (through additional 'COPY' commands) they are",CRLF
   DB "added to the internal QUEUE.  When copy operations are complete the user",CRLF
   DB "will be notified by the appearance of a message box.",CRLF
   DB "Additionally, for BATCH operations, the 'COPYING' environment variable",CRLF
   DB "contains one of two values:  'TRUE' while background copy is in progress,",CRLF
   DB "and 'FALSE' otherwise (this is assigned whenever copy status changes).",CRLF
   DB CRLF
   DB "SWITCHES:  /A  - ASCII COPY (as default or for single file).  A ctrl-Z (1aH)",CRLF
   DB "                 generates an END OF FILE condition.",CRLF
   DB "           /B  - BINARY COPY (as default or for single file).  No character",CRLF
   DB "                 checking is performed.  ASCII files appended together using",CRLF
   DB "                 BINARY mode may have multiple CTRL-Z's inserted into the",CRLF
   DB "                 destination file, depending upon the contents of the source.",CRLF
   DB "           /V  - VERIFY FLAG - causes files to be re-read after copying to",CRLF
   DB "                 ensure that the copy was completed properly.",CRLF
   DB "           /W  - ENHANCEMENT - forces COPY to wait until the copy queue is",CRLF
   DB "                 empty before continuing to the next command.  Use this in",CRLF
   DB "                 batch files that must wait for copies to complete.",CRLF
   DB CRLF
   DB 0


NOT_SUPPORT CTTY

DATE     LABEL BYTE
   DB "DATE [mm/dd/yy]",CRLF,LF
   DB "Displays or changes system date from the command line.  Specifying a new",CRLF
   DB "date causes the system date to change to the new value.",CRLF
   DB 0


DBLSPACE LABEL BYTE
   DB "DBLSPACE [/switch [Drive][...]]",CRLF,CRLF
   DB "Invokes the equivalent of the DBLSPACE program within Windows.  DBLSPACE.EXE",CRLF
   DB "cannot run while in Windows, so SFTSHELL emulates some of its COMMAND LINE",CRLF
   DB "capabilities within the Windows environment.",CRLF,CRLF
;   DB "capabilities within the Windows environment.  Operations which are legal",CRLF
;   DB "from within SFTSHELL include 'information' functions, and MOUNT/DISMOUNT of",CRLF
;   DB "compressed volume files on removable media (diskettes).",CRLF
   DB "NOTE:  This command is not valid for 'non-DBLSPACE' compressed drives.",CRLF,CRLF
   DB " The following DBLSPACE command switches are supported by SFTSHELL:",CRLF
   DB "   /HELP                            - prints a short HELP screen",CRLF
   DB "   [/INFO] [d:]                     - compressed volume file info",CRLF
   DB "   /LIST                            - list valid drives, and CVF status",CRLF
;   DB "(The following options have not yet been implemented - PRELIMINARY)",CRLF
;   DB "   /MOUNT[=sss] [d:] [/NEWDRIVE=e:] - MOUNT a new CVF from drive 'd' as 'e'",CRLF
;   DB "   /UNMOUNT [d:]                    - UNMOUNT drive 'd' (only if not in use!)",CRLF,CRLF
   DB "For more information, see the MS-DOS(r) 6.0 User's Guide on DBLSPACE",CRLF,CRLF
   DB 0


DDE      LABEL BYTE
   DB "DDE [INITIATE|REGISTER] [[server]|topic]",CRLF
   DB "DDE [function] [TOPIC_ID] [item] [text]",CRLF,LF
   DB "This is an advanced function which allows a user to make an 'ad-hoc'",CRLF
   DB "'Dynamic Data Exchange' (DDE) command from the command line, or from within",CRLF
   DB "a BATCH file.  This command causes the environment string 'DDE_RESULT' to",CRLF
   DB "be assigned a value representing the result from this command.",CRLF,LF
   DB "PARAMETERS:    function  - this represents a valid DDE function, which",CRLF
   DB "                           may be one of the following:",CRLF
   DB "                           ADVISE     - Tell server to update (hot link)",CRLF
   DB "                                        whenever data changes.",CRLF
   DB "                           EXECUTE    - Sends command string to server",CRLF
   DB "                           INITIATE   - Begins a DDE conversation.",CRLF
   DB "                                        The 'TOPIC_ID' is returned in the",CRLF
   DB "                                        DDE_RESULT environment variable.",CRLF
   DB "                                        Note the different command format!",CRLF
   DB "                           POKE       - Send un-solicited data to server",CRLF
   DB "                           REGISTER   - Registers this application as a",CRLF
   DB "                                        DDE server for a particular topic.",CRLF
   DB "                                        Note the different command format!",CRLF
   DB "                           REQUEST    - Request data item from server",CRLF
   DB "                           TERMINATE  - Ends a Client DDE conversation.",CRLF
   DB "                           UNADVISE   - Terminate warm/hot link (see",CRLF
   DB "                                        'ADVISE' above).",CRLF,LF
   DB "                           UNREGISTER - Removes a server and topic that are",CRLF
   DB "                                        associated with the 'TOPIC_ID'.",CRLF
   DB "                                        Any active conversations are ended.",CRLF
   DB "               TOPIC_ID  - This value is returned in the 'DDE_RESULT'",CRLF
   DB "                           environment variable after executing the",CRLF
   DB "                           INITIATE or REGISTER function.  Note that",CRLF
   DB "                           the TOPIC_ID usage is different for client and",CRLF
   DB "                           server uses, and cannot be interchanged.",CRLF
   DB "               server    - The name of the server to REGISTER/INITIATE a",CRLF
   DB "                           DDE conversation for/with.  The default server",CRLF
   DB "                           name is 'SFTSHELL' for REGISTER, and NONE (blank)",CRLF
   DB "                           for INITIATE.",CRLF
   DB "               topic     - The name of the DDE topic.  The default is the",CRLF
   DB "                           'SYSTEM' topic.",CRLF
   DB "               item      - An individual 'storage location' associated with",CRLF
   DB "                           a DDE conversation.  The items correspond to the",CRLF
   DB "                           environment variables of the same name, and are",CRLF 
   DB "                           GLOBAL for all conversations.",CRLF
   DB "               text      - The DDE 'EXECUTE' command sent by a client to a",CRLF
   DB "                           server, using the 'EXECUTE' function, or the",CRLF
   DB "                           value to 'POKE' using a 'POKE' transaction.",CRLF
   DB "                           (If not specified on 'POKE' it uses the current",CRLF
   DB "                            value of the environment variable having the",CRLF
   DB "                            same name as the item to 'POKE').",CRLF
   DB CRLF
   DB "NOTE:  When the INITIATE or REGISTER functions are used, the 'DDE_RESULT'",CRLF
   DB "       environment variable will contain the conversation handle, or 'ERROR'",CRLF
   DB "       if unable to perform the desired operation.",CRLF
   DB CRLF
   DB "All DDE operations use the environment variables for the 'ITEM' associated",CRLF
   DB "with a topic.  All topics share the same value for a particular 'ITEM',",CRLF
   DB "including server topics.  If it is necessary to instance the data for",CRLF
   DB "different topics you must spawn multiple copies of SFTSHELL.",CRLF
   DB 0

DEFINE   LABEL BYTE
   DB "DEFINE name [(param list)] {definition}",CRLF
   DB "DEFINE name [(param list)]",CRLF
   DB " ...",CRLF
   DB "END DEFINE",CRLF
;   DB "DEFINE name LIB ",34,"dllname",34," [@ord] {return type} (type list)",CRLF
   DB CRLF
   DB "This command defines a user-definable function, or an external function or",CRLF
   DB "subroutine that is called from within SFTShell.  This command is only valid",CRLF
   DB "within the context of a batch file.",CRLF,LF
   DB "PARAMETERS:  name          - This is the name by which the function or pro-",CRLF
   DB "                             cedure will be referenced.",CRLF
   DB "             (param list)  - A list of environment variables which will be",CRLF
   DB "                             assigned the values passed to the function in",CRLF
   DB "                             the original call.",CRLF
   DB "             {definition}  - Optional one-line definition for the function.",CRLF
   DB "                             Placing the definition on the 'DEFINE' command",CRLF
   DB "                             eliminates the need for 'END DEFINE', but limits",CRLF
   DB "                             the defined function to a single line of code.",CRLF
   DB "             ",34,"dllname",34,"     - For externally defined functions or subroutines,",CRLF
   DB "                             this is the module name or file name for the DLL",CRLF
   DB "                             that contains the function name.",CRLF
   DB "             @ord          - Optional ordinal number for the function within",CRLF
   DB "                             the DLL.  If '@ord' is not specified, the name",CRLF
   DB "                             will be used as specified on the command line.",CRLF
   DB "             {return type} - This is the data type returned by the external",CRLF
   DB "                             function or subroutine.  Valid return types are:",CRLF
   DB "                               VOID, BYTE, SHORT, LONG, USHORT, ULONG, FLOAT,",CRLF
   DB "                               DOUBLE, LDOUBLE, LPSTR, GLOBALSTR",CRLF
   DB "                             NOTE:  type 'GLOBALSTR' is like 'LPSTR' except",CRLF
   DB "                                    that it will be 'GlobalFree'd after the",CRLF
   DB "                                    data has been copied internally.",CRLF
   DB "             (type list)   - This is a list of data types to be passed to the",CRLF
   DB "                             external function as parameters, translated from",CRLF
   DB "                             the values passed in the SFTShell function call.",CRLF
   DB "                             The valid parameters include all of the types",CRLF
   DB "                             listed above (except for VOID), plus a modifier",CRLF
   DB "                             'BYREF', which causes a parameter to be passed",CRLF
   DB "                             by 'REFERENCE' instead of by value.  All",CRLF
   DB "                             reference parameters are passed as FAR pointers",CRLF
   DB "                             to the actual data.  'LPSTR' by definition is",CRLF
   DB "                             a 'BYREF' parameter, and points to the actual",CRLF
   DB "                             data passed to the function in STRING format.",CRLF
   DB CRLF
   DB "For an example of the use of the 'DEFINE' command, see the sample program",CRLF
   DB "'FUNCTION.BAT' included with SFTShell.",CRLF
   DB CRLF
   DB 0


DEL      LABEL BYTE
ERASE    LABEL BYTE
   DB "DEL [/Y][/H][/Q][/S] [filename]",CRLF
   DB "ERASE [/Y][/H][/Q][/S] [filename]",CRLF,LF
   DB "This function deletes files.  As with the MS-DOS(r) version, you may specify",CRLF
   DB "a wildcard pattern for 'filename'.  If the resulting 'filename' would cause",CRLF
   DB "all files in a directory to be deleted, the user will be asked to confirm",CRLF
   DB "the file set with an 'are you sure' message box.",CRLF,LF
   DB "PARAMETERS:    filename  - the file name specification to delete.  This",CRLF
   DB "                           may contain wildcard characters, or may be a",CRLF
   DB "                           directory name.",CRLF,LF
   DB "SWITCHES:   /Y  -  Invokes an automatic 'YES' response to the prompt which",CRLF
   DB "                   asks the user to confirm deletion of '*.*'.",CRLF
   DB "            /H  -  EXCLUDES hidden files from being deleted.  Normally, 'DEL'",CRLF
   DB "                   will delete hidden files also (unlike COMMAND.COM's 'DEL').",CRLF
   DB "                   Use this switch to duplicate the COMMAND.COM behavior.",CRLF
   DB "            /Q  -  Prompts user for each file prior to attempting to delete",CRLF
   DB "                   it.  User presses a single key to delete or keep the file.",CRLF
   DB "                   A 'Y' deletes the file; any other key keeps the file.",CRLF
   DB "            /S  -  Recurse sub-directories when deleting files.  This does",CRLF
   DB "                   *NOT* remove empty sub-directories!.",CRLF,CRLF
   DB "NOTE:  See also 'DELTREE'",CRLF,CRLF
   DB 0


DELTREE  LABEL BYTE
   DB "DELTREE [/Y][/Q] [path]",CRLF,LF
   DB "This function deletes a directory and all of its sub-directories and files",CRLF
   DB "contained within the sub-directories, effectively 'PRUNING' the tree branch",CRLF
   DB "off of the remainder of the directory tree.",CRLF,CRLF
   DB "SWITCHES:   /Y  -  Invokes an automatic 'YES' response to the prompt which",CRLF
   DB "                   asks the user to confirm deletion of the tree.",CRLF
   DB "            /Q  -  Prompts user for each file and sub-directory prior to",CRLF
   DB "                   attempting to delete it.  User presses a single key to",CRLF
   DB "                   delete or keep the file.  A 'Y' deletes the file; any",CRLF
   DB "                   other key keeps the file.",CRLF,CRLF
   DB 0


NOT_INTERNAL DEBUG

DIR      LABEL BYTE
   DB "DIR [filename] [/p][/w][/a[[:]attributes]][/o[[:]sortorder]][/s][/b][/l]",CRLF,LF
   DB "This function lists the specified directory and files according to the",CRLF
   DB "drive, directory, and filename pattern specified on the command line.",CRLF
   DB "It is functionally equivalent to the 'DIR' command found in MS-DOS(r), though",CRLF
   DB "the appearance of the output may vary.  The 'DIRCMD' environment variable",CRLF
   DB "contains the default switches to be used whenever the 'DIR' command is",CRLF
   DB "invoked; to disable an option contained in the 'DIRCMD' environment",CRLF
   DB "variable you can precede the normal switch value with a '-', as with",CRLF
   DB "'/-w' or '/-o' (etc.).  The sort, attribute, and recursion switches",CRLF
   DB "follow the conventions set forth in MS-DOS(r) version 5.0 .",CRLF,LF
   DB "PARAMETERS:    filename  - The file name specification for which to list",CRLF
   DB "                           the directory.  This may or may not contain",CRLF
   DB "                           wildcard '*' or '?' characters.  If a",CRLF
   DB "                           sub-directory name is specified, '*.*' is",CRLF
   DB "                           assumed for the file name specification.",CRLF,LF
   DB "SWITCHES:      /p        - displays one screen at a time, and prompts",CRLF
   DB "                           for a key to be pressed prior to the next",CRLF
   DB "                           screen.",CRLF,LF
   DB "               /w        - displays the listing in 'wide' format, with",CRLF
   DB "                           sub-directories surrounded by '[ ]'.",CRLF,LF
   DB "               /a:attr   - attributes to be included/excluded in the",CRLF
   DB "                           directory listing.  Items to be included",CRLF
   DB "                           must be present in all files listed.  Items",CRLF
   DB "                           to be excluded (preceded by '-') may not",CRLF
   DB "                           appear in any item listed.  The ':' is not",CRLF
   DB "                           required in the switch syntax.  Below is a",CRLF
   DB "                           list of allowed attribute flags:",CRLF
   DB "                           a - archive bit      h - hidden files",CRLF
   DB "                           d - directories      r - read-only files",CRLF
   DB "                           s - system files     p - programs (NEW!)",CRLF
   DB "                           (NOTE:  no spaces may appear within switch)",CRLF,LF
   DB "               /o:sort   - sort order to be used.  Default is 'gne' (dir,",CRLF
   DB "                           name, ext in alphabetical order).  To sort on",CRLF
   DB "                           an item in descending order, prefix it with a",CRLF
   DB "                           '-' (no spaces allowed in switch).  Below is",CRLF
   DB "                           a list of allowed sort flags:",CRLF
   DB "                           d - date/time        e - extension",CRLF
   DB "                           g - directories 1st  n - filename",CRLF
   DB "                           s - size",CRLF,LF
   DB "               /s          Recurse through sub-directories",CRLF
   DB "               /b          'Brief' listing - like '/w', except only",CRLF
   DB "                           one filename per line.",CRLF
   DB "               /l          Displays un-sorted directories & filenames",CRLF
   DB "                           in lower case.",CRLF
;   DB "               /c          COMPRESSION RATIO - valid only for DBLSPACE drives.",CRLF
   DB "               /x          ** ENHANCEMENT **",CRLF
   DB "                           Opens each file and reads the header to determine",CRLF
   DB "                           if it is an EXECUTABLE file;  if so, it attempts",CRLF
   DB "                           to identify it (<WIN>,<DOS>,<OS2>,etc.) and puts",CRLF
   DB "                           an appropriate identifier string in the 'Type'",CRLF
   DB "                           column of the directory listing (/W overrides it)",CRLF,LF
   DB "  NOTE:  For additional information on 'DIR', see the MS-DOS(r) User's Guide",CRLF
   DB "         and reference concerning use of the various switches, and specifics",CRLF
   DB "         concerning file name specifications and so forth.",CRLF
   DB 0


NOT_SUPPORT DOSKEY

_ECHO    LABEL BYTE
   DB "ECHO [ON|OFF]",CRLF
   DB "ECHO message",CRLF
   DB "ECHO.[message]",CRLF,LF
   DB "The above three allowable syntaxes represent two different uses of the 'ECHO'",CRLF
   DB "command - normally they will only be used in BATCH processing.  The first",CRLF
   DB "syntax either causes command echoing to turn ON or OFF for batch processing",CRLF
   DB "ONLY (this differs somewhat from MS-DOS(r) in this respect).  Echo is always",CRLF
   DB "ON when commands are being entered at the prompt.  The 2nd and 3rd syntaxes",CRLF
   DB "represent methods used to display lines on the screen from within a batch",CRLF
   DB "file.  The 2nd one has a message on the line following the word 'ECHO'' the",CRLF
   DB "3rd optionally has a message (and is always distinguished from the 1st syntax",CRLF
   DB "in case your message may conflict with the 1st syntax).  The 2nd syntax does",CRLF
   DB "require a message to be present on the line, so if you want a blank line you",CRLF
   DB "MUST use the 3rd syntax, with no 'message' parameter following.  If 'ECHO'",CRLF
   DB "appears on a line by itself, it is the same as 'ECHO ON' (compatibility).",CRLF,LF
   DB "  NOTE:  The Windows(tm) Command Line Interpreter allows imbedded ANSI",CRLF
   DB "         escape sequences in the PROMPT and ECHO commands to force the",CRLF
   DB "         screen background and text colors to a specific value.  Some",CRLF
   DB "         ANSI sequences (such as BLINKING TEXT) are not supported.",CRLF
   DB "         See the MS-DOS(r) User's Guide and Reference for information",CRLF
   DB "         regarding ANSI escape sequences under the section for the",CRLF
   DB "         ANSI.SYS device driver.",CRLF
   DB "         You can embed '<Esc>' and other special characters into the",CRLF
   DB "         command string by use of the Ctrl-P character, followed by",CRLF
   DB "         the character you wish to insert (<Enter> is an exception).",CRLF
   DB 0


NOT_INTERNAL EDIT

_ELSE    LABEL BYTE
   DB "ELSE",CRLF
   DB "(PRELIMINARY)",CRLF
   DB "This command is part of the 'IF' block processing, and is only valid for",CRLF
   DB "batch files.  This command causes 'IF' processing in the preceding block",CRLF
   DB "to terminate, and the batch processor immediatly searches for the 'ENDIF'",CRLF
   DB "command that ends the block.  No other text may appear on the command line.",CRLF
   DB "'IF/ELSE/ENDIF' blocks may be nested within one another, however.",CRLF,CRLF
   DB "SEE ALSO:  IF, ENDIF",CRLF
   DB 0

_END      LABEL BYTE
_ENDIF    LABEL BYTE
   DB "END IF",CRLF
   DB "ENDIF",CRLF
   DB "END DEFINE",CRLF
   DB "The first two forms of this command mark the end of an 'IF' block.  The",CRLF
   DB "third form marks the end of a 'DEFINE' block for a user-defined funciton.",CRLF
   DB "SEE ALSO:  IF, ELSE, DEFINE",CRLF
   DB 0

EXIT     LABEL BYTE
   DB "EXIT",CRLF
   DB "EXIT WINDOWS",CRLF
   DB "EXIT REBOOT",CRLF
   DB "EXIT RESTART [command]",CRLF
;   DB "(The following has not yet been implemented - PRELIMINARY)",CRLF
;   DB "EXIT LOOP",CRLF,CRLF
;   DB "The first 4 forms of the command end the current instance of 'SFTSHELL'.",CRLF
   DB "All of the 'EXIT' commands shown end the current instance of 'SFTSHELL'.",CRLF
   DB "If a background operation, such as 'COPY' is in progress the user will be",CRLF
   DB "prompted to abort or continue that operation prior to ending the program.",CRLF,CRLF
;   DB "The 5th form, 'EXIT LOOP', is valid only for BATCH FILES, and causes program",CRLF
;   DB "execution to continue at the statement following the 'terminating' command",CRLF
;   DB "for the current 'FOR', 'REPEAT', or 'WHILE' loop.  It is essentially a 'GOTO'",CRLF
;   DB "command with an 'implied' label just past the 'NEXT', 'UNTIL', or 'WEND'",CRLF
;   DB "that marks the end of the current loop.",CRLF,CRLF
   DB "For 'EXIT WINDOWS', 'EXIT REBOOT', or 'EXIT RESTART', each running task",CRLF
   DB "will be queried whether it's safe to end; if they all respond in a",CRLF
   DB "favorable manner, then the Windows(tm) session will be ended.  For",CRLF
   DB "'EXIT RESTART [command]', if 'command' is present it is executed in DOS",CRLF
   DB "from the command line interpreter prior to re-starting Windows (3.1+ only).",CRLF,CRLF
;   DB "SEE ALSO:  FOR, LOOP, NEXT, REPEAT, RETURN, UNTIL, WEND, WHILE",CRLF,CRLF
   DB "SEE ALSO:  FOR",CRLF,CRLF
   DB 0


NOT_INTERNAL EXPAND

NOT_INTERNAL FC

FIND     LABEL BYTE
   DB "FIND [/V] [/C] [/N] [/I] ""string"" [filename]",CRLF,LF
;   DB "FIND [/V] [/C] [/N] [/I] [/S] ""string"" [filespec]",CRLF,LF
   DB "The 'FIND' command searches the input file for a particular string, and",CRLF
   DB "generates output for each matching line found.  The type of output and",CRLF
   DB "search method may be modified by use of one of the switches.  Following",CRLF
   DB "is a list of the parameters, and a brief description of each:",CRLF
   DB " ""string""      This represents the string to be searched for.",CRLF
   DB "  /V           Reverses the search criteria to include all lines NOT",CRLF
   DB "               containing the specified text.",CRLF
   DB "  /C           Display only the total count of matching lines.",CRLF
   DB "  /N           Display line numbers along with matching lines.",CRLF
   DB "  /I           Ignore case (case insensitive comparison).",CRLF
;   DB "  /S           Span sub-directories (valid only with a wildard file spec)",CRLF
   DB LF
;   DB "ENHANCEMENT",CRLF
;   DB "SFTShell's implementation of 'FIND' allows you to search for a string in a",CRLF
;   DB "group of files (similar to the UNIX 'grep' utility) by specifying a file set",CRLF
;   DB "containing wildcard characters.  Additionally, the '/S' switch can be used",CRLF
;   DB "to span sub-directories in the search.",CRLF,LF
   DB "This command may also be used as a 'filter' for use with any command or",CRLF
   DB "program that generates text output to the screen (i.e. Standard Output, or",CRLF
   DB "'CON' device).  SFTShell allows you to specify MS-DOS, Windows, and",CRLF
   DB "'internal' commands as input to a 'piped' command.",CRLF
   DB LF, 0


FORMAT   LABEL BYTE
   DB "FORMAT drive: [/v[:label]] [/q][/u] [size modifiers] [/b|/s]",CRLF,LF
   DB "This command initiates a BACKGROUND FORMAT of the appropriate device",CRLF
   DB "by QUEUING the specified information to the background formatter.",CRLF
   DB "The background formatter will not format more than one disk at a time",CRLF
   DB "since doing so would greatly reduce the efficiency of the process",CRLF
   DB "(the floppy controller can only access one drive at a time anyway).",CRLF
   DB "Attempting to format a Hard Drive, non-removable drive, network drive,",CRLF
   DB "or 'SUBST/JOIN'ed drives will result in an error.  If, however, a",CRLF
   DB "drive has active 'SUBST/JOIN'ed drives pointing to it it will still",CRLF
   DB "format correctly.  It is recommended that you verify no 'SUBST/JOIN'ed",CRLF
   DB "drives are pointing to the drive to be formatted prior to formatting.",CRLF
   DB "See the on-line help regarding SUBST and JOIN for further information.",CRLF,LF
   DB "SWITCHES:",CRLF
   DB "The FORMAT command switches correspond to the switches used in MS-DOS(r)",CRLF
   DB "version 5.0, including the /q switch.  However, this FORMAT command does",CRLF
   DB "NOT write 'UNFORMAT' information in the absence of '/u'!  It is important",CRLF
   DB "to consider this whenever you format a diskette without the '/u' switch.",CRLF,LF
   DB "  /v[:label]   Assigns a VOLUME LABEL at format time.  If it is omitted",CRLF
   DB "               the volume label will be prompted for prior to queuing",CRLF
   DB "               the disk to be formatted.  Specifying '/v:' yields the",CRLF
   DB "               same result as pressing <ENTER> for the volume label.",CRLF
   DB "  /q           'Quick' format - writes FAT and root directory, but does",CRLF
   DB "               not verify any tracks/sectors on the diskette.",CRLF
   DB "  /u           'Unconditional' - by default SFTSHELL will format a diskette",CRLF
   DB "               in this manner.  It is not compatible with UNFORMAT.",CRLF
   DB "  /f:size      Specify the size of the media to format, if it is other",CRLF
   DB "               than the default size for the drive.  Only the following",CRLF
   DB "               values are accepted:",CRLF
   DB "                 360  or 360k  or 360kb     - 360k double sided 5-1/4",22h,CRLF
   DB "                 720  or 720k  or 720kb     - 720k double sided 3-1/2",22h,CRLF
   DB "                 1200 or 1200k or 1200kb or",CRLF
   DB "                 1.2  or 1.2m  or 1.2mb     - 1.2M double sided 5-1/4",22h,CRLF
   DB "                 1440 or 1440k or 1440kb or",CRLF
   DB "                 1.44 or 1.44m or 1.44mb    - 1.44M double sided 3-1/2",22h,CRLF
;   DB "                 2880 or 2880k or 2880kb or",CRLF
;   DB "                 2.88 or 2.88m or 2.88mb    - 2.88M double sided 3-1/2",22h,CRLF
;   DB "                 (note - at this time the 2.88m format is unsupported.  If",CRLF
;   DB "                  you format a 2.88m diskette you should not specify the",CRLF
;   DB "                  diskette media size - the BIOS default should work ok)",CRLF
   DB "  /b           Reserves space for IO.SYS and MSDOS.SYS .  This switch is",CRLF
   DB "               ignored since MS-DOS(r) 5.0 and later does not require it.",CRLF
   DB "  /s           Copies system files IO.SYS, MSDOS.SYS, and COMMAND.COM",CRLF
   DB "               to the formatted diskette.",CRLF
   DB "  /t:tracks    Specifies number of tracks to use.  Not compatible with '/f'",CRLF
   DB "  /n:sectors   Specifies number of sectors/track.  Not compatible with '/f'",CRLF
   DB "  /4           Specifies '40 track' for 1.2Mb drives only.  Obsolete - use",CRLF
   DB "               the '/f' switch instead - it's better.",CRLF,LF
   DB "Switches not listed above are NOT supported - this includes disk sizes for",CRLF
   DB "MS-DOS(r) version 1.x, single sided diskettes, and so on.  If you wish to",CRLF
   DB "format a Hard Disk, use one of the options above, or format media that is",CRLF
   DB "not supported by the SFTSHELL FORMAT command, type FORMAT.EXE in place of",CRLF
   DB "'FORMAT' (or FORMAT.COM for older DOS versions) to force running the program",CRLF
   DB "in place of the internal 'FORMAT' command.",CRLF
   DB 0

_FOR     LABEL BYTE
   DB "FOR %variable [NOT] IN (file set) DO command",CRLF
   DB "FOR %%variable [NOT] IN (file set) DO command",CRLF
   DB "FOR variable FROM start TO end [STEP stepval] [DO command]",CRLF,CRLF
   DB "This command allows repeated processing of a given command using each of the",CRLF
   DB "file names referenced within '(file set)'.  An ENHANCEMENT to the normal",CRLF
   DB "MS-DOS(r) version of this function is the 'NOT' keyword, which allows you to",CRLF
   DB "specify an EXCLUSION file set (which *MUST* pertain to the current",CRLF
   DB "directory!).  An EXCLUSION set will apply to all files in the CURRENT",CRLF
   DB "directory that do not match the file specs provided in '(file set)'.  The",CRLF
   DB "'command' argument can be any valid command.  Note that the nesting of",CRLF
   DB "'FOR' commands may be performed here, but can lead to insufficient memory",CRLF
   DB "or stack overflow conditions if done to excess.  This could also be",CRLF
   DB "considered an enhancement over MS-DOS(r), which does not allow nesting of",CRLF
   DB "'FOR' commands.",CRLF
   DB "The MS-DOS(r) version of 'FOR' requires batch files to be specified using",CRLF
   DB "the 'CALL' command preceding them, or else it does not function as expected.",CRLF
   DB "Here, you may leave off the 'CALL' command for batch files, with no adverse",CRLF
   DB "side effects.  The use of a double '%' on the variable name is still required",CRLF
   DB "for batch files, but is optional when used from the command line",CRLF,CRLF
   DB "PARAMETERS:  %variable  - The text to be substituted with a file name from",CRLF
   DB "                          '(file set)' (or NOT from file set, depending on",CRLF
   DB "                          the presence/absence of the 'NOT' option) for each",CRLF
   DB "                          iteration of 'command'.",CRLF
   DB "             (file set) - A list of files specifications used for determining",CRLF
   DB "                          which file names to substitute for '%variable'.",CRLF
   DB "                          This may consist of multiple file names, separated",CRLF
   DB "                          either by spaces or commas.  Each file name may",CRLF
   DB "                          contain wildcard ('*' or '?') characters, which",CRLF
   DB "                          operate in the same manner as directory searches.",CRLF
   DB "             command    - A valid command, program, or batch file.  As an",CRLF
   DB "                          extra added bonus, the 'FOR' command will pause",CRLF
   DB "                          until the batch file or program file has completed.",CRLF
   DB "                          Note that there is no way to determine if a program",CRLF
   DB "                          that was run from within a BATCH file has finished.",CRLF
   DB "                          In such a case, the batch file should use the",CRLF
   DB "                          'IF ISTASK' function to wait for completion.",CRLF,CRLF
;   DB "The ENHANCED mode of the 'FOR' command, in which a variable is 'stepped' from",CRLF
;   DB "the starting value to the ending value (inclusive) by the 'step' value works",CRLF
;   DB "the same way as the 'FOR/NEXT' loop in the BASIC programming language.  A",CRLF
;   DB "variable is first assigned the 'start' value, then performs the indicated",CRLF
;   DB "operation at least one time.  At the end of the operation, the variable is",CRLF
;   DB "incremented by the 'step' value (default: 1) and compared with the 'end'",CRLF
;   DB "value.  If it has 'passed' the 'end' value (from being incremented) the",CRLF
;   DB "loop ends and execution begins at the statement following the end of the",CRLF
;   DB "loop.  The following format are acceptable:",CRLF
;   DB "loop.  The following formats are acceptable:",CRLF
;   DB "   FOR var FROM start TO end [STEP stepval] DO command",CRLF
;   DB "(PRELIMINARY - the following form has not yet been implemented)",CRLF
;   DB "                       - OR -",CRLF
;   DB "   FOR var FROM start TO end [STEP stepval]",CRLF
;   DB "                  ... commands ...",CRLF
;   DB "   NEXT [var]",CRLF,CRLF
;   DB "SEE ALSO:  EXIT LOOP, LOOP, NEXT, REPEAT, UNTIL, WHILE, WEND",CRLF,CRLF
   DB 0

_GOTO    LABEL BYTE
   DB "GOTO label",CRLF,LF
   DB "This command resumes execution from a specific point in a batch file.  It is",CRLF
   DB "not valid for on-line command entry (and will result in an error).  Labels",CRLF
   DB "are declared in batch files by prefixing them with a ':', as follows:",CRLF
   DB ":MY_LABEL",CRLF
   DB "In a 'GOTO' statement you would refer to this label as follows:",CRLF
   DB "GOTO MY_LABEL",CRLF,LF
   DB "For more information on this topic, refer to the MS-DOS(r) User's Guide and",CRLF
   DB "Reference.",CRLF
   DB 0


NOT_SUPPORT GRAFTABL

NOT_SUPPORT GRAPHICS

HELP     LABEL BYTE
   DB "HELP [command]",CRLF,LF
   DB "This is the HELP command, which allows you to view a short description of",CRLF
   DB "a standard MS-DOS(r) command supported by this program, or an ENHANCEMENT",CRLF
   DB "which has been added to the Windows(tm) Command Line Interpreter.  To",CRLF
   DB "see a brief list of commands recognized by this program, enter 'HELP' with",CRLF
   DB "no arguments on the command line.  To see help on a particular command,",CRLF
   DB "enter 'HELP' followed by that command.  For this message, enter 'HELP HELP'.",CRLF
   DB 0

_IF       LABEL BYTE
   DB "IF [not] ERRORLEVEL number command  (not supported under Windows(tm))",CRLF
   DB "IF [not] string1==string2 command",CRLF
   DB "IF [not] EXIST filename command",CRLF
   DB "IF [not] ISTASK handle command      ** ENHANCEMENT **",CRLF
   DB "IF [not] ISYES string command       ** ENHANCEMENT **",CRLF
   DB "IF [not] ISNO string command        ** ENHANCEMENT **",CRLF
   DB "IF [not] (equation) command         ** ENHANCEMENT **",CRLF
;   DB "IF {conditional syntax} THEN",CRLF
;   DB "      ... commands ...",CRLF
;   DB "[ELSE]",CRLF
;   DB "     [... commands ...]",CRLF
;   DB "ENDIF",CRLF
;   DB "(IF/THEN/ELSE format is not yet implemented - PRELIMINARY)",CRLF,CRLF
   DB "This command allows conditional processing.  The first uses the exit code",CRLF
   DB "from a spawned process, by MS-DOS(r) definitions;  however, Windows(tm) is",CRLF
   DB "a multi-tasking system, and processes spawned from within the Command Line",CRLF
   DB "Interpreter normally run concurrently, and therefore do not generate exit",CRLF
   DB "codes.  If it is necessary to wait for a process to finish before continuing",CRLF
   DB "then the program should wait until a task finishes by use of the 'WAIT'",CRLF
   DB "command within a loop, and using 'IF NOT ISTASK handle GOTO xxxx' to form a",CRLF
   DB "loop that waits for a given task to complete.",CRLF
   DB "The second compares two strings.  These strings can either be literals",CRLF
   DB "(if embedded spaces exist they should be surrounded by quotes) or variables",CRLF
   DB "so long as variables are either BATCH variables (%1, %2, etc.) or MS-DOS(r)",CRLF
   DB "ENVIRONMENT variables (%path%, %prompt%, %temp%, etc.).",CRLF
   DB "The third allows the user to check for existence of a file before trying",CRLF
   DB "an operation in which the file must either exist or not exist (that is",CRLF
   DB "the question...), and an error or error message would be unacceptable.",CRLF
   DB "The 4th through 6th are all ENHANCEMENTS to normal MS-DOS(r) COMMAND.COM",CRLF
   DB "processing.  'ISTASK' allows a process to take place based on the existence",CRLF
   DB "of a task handle.  This handle is usually obtained either by the TASKLIST",CRLF
   DB "or via the 'TASK_ID' environment variable (assigned whenever a task is",CRLF
   DB "successfully spawned).  It may be necessary to check if this variable exists",CRLF
   DB "by using the '==' form of the command against a blank string, as follows:",CRLF
   DB "    IF ",34,"%TASK_ID%",34,"==",34,34," GOTO BAD_TASK",CRLF
   DB "Typically, this variable will be assigned to another following a successful",CRLF
   DB "run of a program, as follows:",CRLF
   DB "    MYPROG.EXE",CRLF
   DB "    IF ",34,"%TASK_ID%",34,"==",34,34," GOTO MYPROG_DID_NOT_RUN",CRLF
   DB "    SET MYPROG_ID=%TASK_ID%",CRLF
   DB "       ...",CRLF
   DB "       ...",CRLF
   DB "    IF ISTASK %MYPROG_ID% CLOSETASK %MYPROG_ID%",CRLF
   DB "(this is one possible use of the CLOSETASK function as well).",CRLF
   DB "For 'background' processing it is possible to have the application minimize",CRLF
   DB "itself via the 'MIN' function - see 'MIN' for more information",CRLF
   DB "(The 'CALL' command provides another means of waiting for a program to end",CRLF
   DB " prior to continuing a batch file or returning control to the command line",CRLF
   DB " interpreter.  Type 'HELP CALL' for more information).",CRLF
   DB "The next two options, 'ISYES' and 'ISNO' check the string which follows to",CRLF
   DB "see if it 'qualifies' as a valid 'YES' or 'NO' response, and processes",CRLF
   DB "the remaining command accordingly.  Note that if imbedded spaces may exist",CRLF
   DB "in the string (as would be possible if obtained via the 'INPUT' function)",CRLF
   DB "it would be wise to use a format similar to the example below:",CRLF
   DB "    INPUT USER_YESNO Enter a 'Yes' or 'No' response:",CRLF
   DB "    IF ISYES ",34,"%USER_YESNO%",34," ECHO You entered a 'Yes' response",CRLF
   DB "    IF ISNO ",34,"%USER_YESNO%",34," ECHO You entered a 'No' response",CRLF
   DB "If a default of 'No' is expected, use 'ISYES' only.  If a default of 'Yes'",CRLF
   DB "is expected, use 'ISNO' only.  If you wish to force the user to enter a",CRLF
   DB "valid 'Yes' or valid 'No' response only, use both forms in such a manner",CRLF
   DB "that neither a valid yes or valid no would print a warning/error and trap",CRLF
   DB "the error in whatever manner is desired.",CRLF,LF
   DB "The final (and most powerful) option is an enhancement which covers the",CRLF
   DB "functionality of the other options, and also exceeds their capabilities,",CRLF
   DB "and is a part of SFTShell's high level language extensions.  It first",CRLF
   DB "evaluates 'equation', then uses the results to determine if it is TRUE",CRLF
   DB "(a non-zero number or a non-'blank' string) or FALSE.  This result is",CRLF
   DB "also modified by the 'NOT' operator as with the other options.  If the",CRLF
   DB "total result is TRUE, the command following the equation is evaluated and",CRLF
   DB "performed.  Otherwise, it is not, and control passes to the next line.",CRLF,LF
   DB "See also the 'LET' and 'CALC' commands, and the 'VARIABLES' discussion.",CRLF,LF
   DB 0


_INPUT   LABEL BYTE
   DB "INPUT [variable [string]]",CRLF,LF
   DB "This is a *very* useful command for creating interactive batch files.  You",CRLF
   DB "could use this command to prompt for file names, options, etc.  Use this in",CRLF
   DB "conjunction with the 'LET' command to provide string parsing capabilities.",CRLF
   DB "Use the 'ISYES' and 'ISNO' options for the 'IF' command to automatically",CRLF
   DB "parse out 'yes' and 'no' responses in a standard manner.",CRLF
   DB "The command operates by displaying the optional 'string' (which begins at",CRLF
   DB "the first non-white-space character following 'variable', and extends to",CRLF
   DB "the end of the command line), following this with a single space.  If no",CRLF
   DB "prompt appears in the command, a default prompt consisting of the variable",CRLF
   DB "name followed by a '?' will be displayed.  The user will then be able to",CRLF
   DB "type in a command from the keyboard, which will then be assigned to the",CRLF
   DB "environment variable (minus the <CR> <LF>).  Pressing <Ctrl-C> during the",CRLF
   DB "process terminates the input and returns a NULL string.",CRLF,LF
   DB "PARAMETERS:   variable  -  The name of the DOS environment variable.  This",CRLF
   DB "                           variable will be created if it does not already",CRLF
   DB "                           exist, and will be assigned the value of the",CRLF
   DB "                           string entered by the user.",CRLF
   DB "              string    -  The prompt printed prior to input.  This parm",CRLF
   DB "                           is optional.  The default prompt consists of the",CRLF
   DB "                           variable name followed by '?'.",CRLF,LF
   DB 0

JOIN LABEL BYTE
;   DB "JOIN [drive:] [/D|[drive:][path]]",CRLF,LF
;   DB "The JOIN command accesses the DOS internal data structures that maintain",CRLF
;   DB "information about logical drives, and either lists all 'JOIN'ed drives,",CRLF
;   DB "or optionally add or delete a 'JOIN'ed drive to the system.",CRLF
;   DB "A drive may be 'JOIN'ed to another drive on any empty path.  This causes",CRLF
;   DB "MS-DOS(r) to treat the 'JOIN' path and all sub-directories from the path",CRLF
;   DB "as being part of the 'JOIN'ed drive internally, while externally showing",CRLF
;   DB "them to be part of the path on the drive it was 'JOIN'ed to.  For example",CRLF
;   DB "if I wish to have drive 'E' appear as part of drive 'D' on the 'D:\E_DRIVE'",CRLF
;   DB "path (an empty sub-directory) I would issue the following command:",CRLF
;   DB "   JOIN E: D:\E_DRIVE",CRLF
;   DB "If both drives and the path are valid (and the path is empty) the 'E' drive",CRLF
;   DB "entry will be updated such that it is 'JOIN'ed to the 'D' drive.  When the",CRLF
;   DB "command 'DIR D:\E_DRIVE\*.*' is entered, a directory of 'E:\' will be shown,",CRLF
;   DB "except that it will appear to be part of the 'D:\E_DRIVE' directory.",CRLF,LF
;   DB "To 'JOIN' two drives, the command format is:",CRLF
;   DB "   JOIN D: C:\path",CRLF
;   DB "   where   'D:' is the device to be 'JOIN'ed",CRLF
;   DB "    and    'C:\path' is the EMPTY sub-directory to which 'D:' will",CRLF
;   DB "           be joined (must be on a different drive!).",CRLF,LF
;   DB "To un-'JOIN' a drive, use the normal logical designator and the '/D' switch,",CRLF
;   DB "as follows:",CRLF
;   DB "   JOIN D: /D",CRLF
;   DB "   where   'D:' is the device to be un-'JOIN'ed",CRLF,CRLF
   DB "JOIN",CRLF,LF
   DB "The 'JOIN' command is an 'information-only' function which provides a list",CRLF
   DB "of 'JOIN'ed drives currently in the system.",CRLF,LF
   DB 0


NOT_SUPPORT KEYB


KILLTASK LABEL BYTE
   DB "KILLTASK task|instance|window|module name",CRLF,LF
   DB "This command is one which allows you to 'kill' a task by specifying its",CRLF
   DB "MODULE NAME (enhancement from prior release) or the 'Task Handle' (the",CRLF
   DB "'ID' number Windows(tm) uses for the task).  No other function anywhere",CRLF
   DB "(to my knowledge) does this so easily.  Developers, take note!  If a task",CRLF
   DB "is running WITHOUT a corresponding window (normally an ERROR condition)",CRLF
   DB "it is possible to see this task via the TASKLIST function.  You can then",CRLF
   DB "terminate such tasks via KILLTASK by specifying the task handle or MODULE",CRLF
   DB "NAME (from the TASKLIST display); the Task 'ID' may be in hexadecimal,",CRLF
   DB "decimal, or octal (for mainframe people who refuse to downsize).",CRLF
   DB "(ENHANCEMENT) If the MODULE NAME is specified, and more than one 'instance'",CRLF
   DB "of this program is running, then the program will produce a list of all of",CRLF
   DB "the running programs, their Task 'ID's, and Window Captions.  It will then",CRLF
   DB "prompt for the task entry to be KILL'ed using sequence numbers (1,2,3,etc.)",CRLF
   DB "printed to the left of each entry.  You may press <ENTER> to cancel, or",CRLF
   DB "enter an appropriate sequence number to delete that task.",CRLF,LF
   DB "As with the 'C' language, OCTAL numbers are preceded by a leading zero,",CRLF
   DB "and HEXADECIMAL numbers by '0x'.  You can also use an 'H' suffix on HEX",CRLF
   DB "numbers (as in Assembler), and as displayed by TASKLIST.  Normally, HEX",CRLF
   DB "notation (with the 'H') is the preferred method, but the others are once",CRLF
   DB "again provided for flexibility (and it uses same function as the equation",CRLF
   DB "evaluator anyway - that's why they're supported).",CRLF,CRLF
   DB "CAUTION:  Certain tasks may not take too kindly at being 'killed' with this",CRLF
   DB "          function.  Exercise discretion in its use!  If possible, use the",CRLF
   DB "          'CLOSETASK' function in its place, which attempts to close the",CRLF
   DB "          main window of a task in order to terminate it.  Once it has",CRLF
   DB "          been determined that 'KILLTASK' can be used without causing any",CRLF
   DB "          loss of system integrity, go for it!  There are applications that",CRLF
   DB "          do not exit properly when you use 'KILLTASK' on them, and they may",CRLF
   DB "          leave *time bombs* behind that result in GP faults.  If in doubt,",CRLF
   DB "          you should try CLOSETASK.  If this won't work, then you should try",CRLF
   DB "          using KILLTASK.  In the worst cases, the 'NUKETASK' command may be",CRLF
   DB "          employed, which provides the LEAST FRIENDLY method of ending an",CRLF
   DB "          application.",CRLF,CRLF
   DB "See also:  CLOSETASK, NUKETASK",CRLF,CRLF
   DB 0


_LABEL    LABEL BYTE
   DB "LABEL [drive:][label]",CRLF,LF
   DB "This command allows you to display or change the volume label of the current",CRLF
   DB "or a specified disk drive.  If 'label' is omitted, the current label is only",CRLF
   DB "displayed; if 'drive:' is omitted, the current (default) drive is assumed.",CRLF
   DB 0



LET       LABEL BYTE
   DB "LET variable=[equation]",CRLF,LF
   DB "Assigns a value to 'variable' based on the evaluation of 'equation' using",CRLF
   DB "a plethora of mathematical operators and functions, and algebraic order of",CRLF
   DB "of operations (similar to an assignment statement in BASIC, PASCAL, or C/C++).",CRLF
   DB "Numeric and String data types are somewhat interchangable, and the type of",CRLF
   DB "function called affects how the values are treated.  Results are stored as",CRLF
   DB "string values within environment variables, and can also be imbedded within",CRLF
   DB "string constants using the '%' substitution operators.",CRLF,CRLF
   DB "Information on the various operators and functions is included in the",CRLF
   DB "'High Level Language' overview in the 'SFTSHELL.HLP' help file, that",CRLF
   DB "has been included with this program.",CRLF,LF
   DB "SAMPLE EXPRESSION:",CRLF
   DB "   LET a=1+2      this assigns the environment variable 'a' with the",CRLF
   DB "                  result of the equation '1+2'",CRLF
   DB "   LET a= a + 1   this assigns the environment variable 'a' with the",CRLF
   DB "                  result of the equation 'a + 1'.  It is important to",CRLF
   DB "                  note that 'a' must not contain any white-space in",CRLF
   DB "                  its name, and especially must not have white-space",CRLF
   DB "                  between the variable name and the '=' when used as",CRLF
   DB "                  the 'target' variable for assignment.  In this regard,",CRLF
   DB "                  it behaves like the 'SET' command, with the exception",CRLF
   DB "                  that the right-hand side of the '=' is treated as an",CRLF
   DB "                  expression instead of a string-literal",CRLF,LF
   DB "String operators are normally 'doubled' to denote them from numeric operators.",CRLF
   DB "This is due to the treatment of *ALL* data as text (assigned to an environment",CRLF
   DB "variable, for example) and such distinction is necessary in order to properly",CRLF
   DB "render the data.  Therefore, the expression 'a + b' treats 'a' and 'b' as",CRLF
   DB "environment variables containing 'numeric' data, while 'a ++ b' treats",CRLF
   DB "'a' and 'b' as environment variables containing 'text' data.  As well, the",CRLF
   DB "use of '%' substitution is always evaluated *BEFORE* the equation is parsed.",CRLF
   DB "This is useful information for generating 'self-writing' code, which is a",CRLF
   DB "unique phenomena of interpretive languages.",CRLF,LF
   DB "It is also important to note that functions which provide control to the",CRLF
   DB "environment (i.e. setting the cursor position, text output, and so forth",CRLF
   DB "might be adversely affected if they are used via the 'CALC' command.  The",CRLF
   DB "alternative would be to assign the results of one of these functions (which",CRLF
   DB "is normally a blank string) to a 'dummy' variable via the LET command.",CRLF,LF
   DB "See also the 'CALC' and 'IF' commands, and the 'VARIABLES' discussion.",CRLF
   DB 0


LH       LABEL BYTE
LOAD     LABEL BYTE
LOADHIGH LABEL BYTE
   DB "LH [program] [commands]",CRLF
   DB "LOAD [program] [commands]",CRLF
   DB "LOADHIGH [program] [commands]",CRLF,LF
   DB "For command compatibility with MS-DOS(r) 5.x and above, the LH and LOADHIGH",CRLF
   DB "commands, plus the Windows(tm) Command Line Interpreter *ENHANCEMENT* 'LOAD'",CRLF
   DB "all execute programs (similar to typing the program name at the command",CRLF
   DB "prompt).  However, use of these commands causes the 'nCmdShow' parameter",CRLF
   DB "(passed to the program on startup) to MINIMIZE the window (icon), similar",CRLF
   DB "to specifying 'LOAD=' from within WIN.INI (on startup only), or using the",CRLF
   DB "'Run Minimized' option from File Manager's 'FILE' 'RUN' menu.  Because of",CRLF
   DB "the similarity in command appearance, 'LH' and 'LOADHIGH' have been",CRLF
   DB "included with 'LOAD' in providing this functionality.",CRLF
   DB CRLF
   DB "See also: CALL, START",CRLF
   DB LF,0


LISTOPEN LABEL BYTE
   DB "LISTOPEN",CRLF
   DB "LO",CRLF,CRLF
   DB "The LISTOPEN function gets you a 'sneak peek' at the SYSTEM FILE TABLE",CRLF
   DB "(SFT) which contains an entry for each open file in the system.  This",CRLF
   DB "function may be useful in tracking the progress of extensive operations",CRLF
   DB "in which file I/O is being performed, or for obtaining general information",CRLF
   DB "about MS-DOS and Windows.",CRLF,CRLF
   DB "PARAMETERS:  NONE",CRLF,CRLF
   DB "The columns in the 'Open File' list are defined as follows:",CRLF,CRLF
   DB "  File/Dev Name:    The drive letter and file name, or device name, of the",CRLF
   DB "                    open file.  Since MS-DOS does *not* keep track of the",CRLF
   DB "                    file's directory path, only the name is available.",CRLF
   DB "  Open Mode:        The access mode for the open file.  This can be 'Rd',",CRLF
   DB "                    'Wrt', or 'R/W' for Read, Write, and Read/Write mode.",CRLF
   DB "  Share Mode:       The 'file sharing' mode used for this open file.  This",CRLF
   DB "                    only has an effect if SHARE or Network software has",CRLF
   DB "                    been loaded.  The 5 share modes are 'Comp', 'Shr',",CRLF
   DB "                    'NoRd', 'NoWr', and 'Excl' for COMPATIBILITY, Shared,",CRLF
   DB "                    'No Read' (by other applications), 'No Write' (by other",CRLF
   DB "                    applications), and 'Exclusive' access.",CRLF
   DB "  Machine ID:       This is the 'Virtual Machine' (VM) within which the",CRLF
   DB "                    file's owning application resides.  VM '0' is the",CRLF
   DB "                    'MS-DOS' VM.  VM '1' is normally the 'Windows' VM, in",CRLF
   DB "                    which all Windows applications run.  The remaining VM's",CRLF
   DB "                    normally belong to 'non-Windows' applications.",CRLF
   DB "  Owner PSP:        The 'DOS' (real mode) segment value for the 'PSP' (PDB)",CRLF
   DB "                    that identifies the program which owns the open file.  A",CRLF
   DB "                    'PSP' (or PDB) is a Program Segment Prefix (MS-DOS; in",CRLF
   DB "                    Windows it's known as a Program DataBase) that MS-DOS",CRLF
   DB "                    uses to identify memory and file 'ownership', and it",CRLF
   DB "                    contains a 'file handle translation table' which is used",CRLF
   DB "                    to translate a program's file handle to an SFT entry.",CRLF
   DB "  Cur File Pointer: This is the position within the file at which the next",CRLF
   DB "                    READ or WRITE operation will occur.  This value may",CRLF
   DB "                    exceed the file size (see below).",CRLF
   DB "  Cur File Length:  This is the *ACTUAL* file length, independent of the",CRLF
   DB "                    file length indicated in a 'DIR' listing.  A 'DIR' list",CRLF
   DB "                    will only reflect what the file's directory entry has",CRLF
   DB "                    stored in it.  MS-DOS must be explicitly invoked to",CRLF
   DB "                    update the directory entry.  Otherwise, this function",CRLF
   DB "                    (or a similar one, using the SFT) is the only method by",CRLF
   DB "                    which the actual file length may be determined",CRLF
   DB "  Owner Task:       This is the TASK HANDLE for the open file's owner.  If",CRLF
   DB "                    no task is associated with the PSP (or VM), this entry",CRLF
   DB "                    is blank.",CRLF
   DB "  Owner Module:     This is the MODULE NAME for the open file's owner.  For",CRLF
   DB "                    Windows applications, it is the module name associated",CRLF
   DB "                    with the task (or PDB).  For NON-WINDOWS applications,",CRLF
   DB "                    it is the PROGRAM NAME contained in the PSP's Memory",CRLF
   DB "                    Control Block (DOS 4 or later).  For MS-DOS (VM #0) and",CRLF
   DB "                    'Windows owned' files, or if the file's owner cannot be",CRLF
   DB "                    determined, an appropriate label appears in this column.",CRLF,CRLF
   DB 0



_LOOP     LABEL BYTE

   DB "LOOP",CRLF
   DB "(PRELIMINARY)",CRLF,CRLF
;   DB "Causes execution to proceed to the END of the current loop (either a 'FOR',",CRLF
;   DB "'WHILE', or 'REPEAT' loop) and be evaluated for the next pass.  This command",CRLF
;   DB "is valid only for BATCH files, which may have 'code blocks' that contain",CRLF
;   DB "several commands to be executed for each loop iteration.",CRLF,CRLF
;   DB "SEE ALSO:  EXIT LOOP, FOR, NEXT, REPEAT, UNTIL, WEND, WHILE",CRLF,CRLF
   DB 0


_HELP_TEXT_ ENDS



;
;** broke it into 2 segments - 7/5/92
;
;


_HELP_TEXT2_ SEGMENT PARA PUBLIC 'CODE' ;** other half of help text stuff

_HELP_START2_ LABEL WORD

   DW    (_HELP_END_ - _HELP_START_) / 2 - 2
   DW    (_HELP_END2_ - _HELP_START2_) / 2 - 2

   DW    MAX,      MD,       MEM,      MIN
   DW    MKDIR,    MODE,     MORE,     MOVE,     NET,      _NEXT
   DW    NLSFUNC,  NUKETASK, ODBC,     PATH,     PAUSE,    PLAYSOUND
   DW    PRINT,    PROMPT,   PLAYSOUND,RD,       REM,      REMOVE
   DW    REN,      RENAME,   _REPEAT,  REPLACE,  _RETURN,  REXX
   DW    RMDIR,    SET,      SHARE,    SHIFT,    SORT,     SQL
   DW    START,    SUBST,    SYS,      TASKLIST, TIME,     TL
   DW    TREE,     TRUENAME, _TYPE,    UNDELETE, _UNTIL,   VER
   DW    VERIFY,   VOL,      _WAIT,    _WEND,    _WHILE,   XCOPY

_HELP_END2_ DW 0     ; marks end of the list!!




MAX      LABEL BYTE
   DB "MAX [window]",CRLF,LF
   DB "Toggles a window between its MAXIMIZED state (full-screen view) and its",CRLF
   DB "RESTORED state (normal view).  If 'window' is a valid window handle that",CRLF
   DB "is capable of being maximized, it will be maximized. Otherwise, if there",CRLF
   DB "is no argument, the Windows(tm) Command Line Interpreter window is maximized",CRLF
   DB "or restored to 'NORMAL'.  The 'WINDOW_STATE' variable can be used to verify",CRLF
   DB "if the window has already been maximized.  This variable will have one of",CRLF
   DB "the following values, depending on the current window state:",CRLF
   DB "       ",34,"MINIMIZED",34,"  Window is minimized",CRLF
   DB "       ",34,"NORMAL",34,"     Window is 'normal' (neither minimized nor maximized)",CRLF
   DB "       ",34,"MAXIMIZED",34,"  Window is maximized",CRLF,LF
   DB "To restore the main window to its NORMAL state, the following batch commands",CRLF
   DB "work rather nicely:",CRLF
   DB "    IF %WINDOW_STATE%==MINIMIZED MIN",CRLF
   DB "    IF %WINDOW_STATE%==MAXIMIZED MAX",CRLF,LF
   DB 0

MD       LABEL BYTE
MKDIR    LABEL BYTE
   DB "MD dirname",CRLF
   DB "MKDIR dirname",CRLF,LF
   DB "Makes a new sub-directory with the name 'dirname' on the specified drive",CRLF
   DB "(defaults to current drive/directory as 'source node').  The 'dirname'",CRLF
   DB "parameter may either consist of a full drive:\path specification, or a ",CRLF
   DB "directory specification without the root '\' (in which case the current",CRLF
   DB "default directory on the specified drive is used as the point from which",CRLF
   DB "the directory is created.",CRLF
   DB "For more information, see the MS-DOS(r) User's Guide and Reference.",CRLF
   DB 0

MEM      LABEL BYTE
;   DB "MEM [/program|/debug|/classify]",CRLF,LF
   DB "MEM [/C]",CRLF,LF
   DB "This function is not the same as you find under MS-DOS(r)! Since Windows(tm)",CRLF
   DB "uses a much different memory management method than does DOS, you will not",CRLF
   DB "get the same results from this program that you would if you used a similar",CRLF
   DB "program running under MS-DOS(r).",CRLF,LF
;   DB "SWITCHES:   /P[rogram]   - provides a list of programs which are currently",CRLF
;   DB "                           loaded, including a total amount of memory for",CRLF
;   DB "                           each module, and a list of TASK and INSTANCE id's",CRLF
;   DB "                           for each task instance.  This does not include",CRLF
;   DB "                           any library or system code/data.",CRLF
;   DB "             /D[ebug]    - provides a list, similar to above, except that",CRLF
;   DB "                           it includes library and system code/data.",CRLF
   DB "            /C[lassify]  - lists each loaded module once, and the total",CRLF
   DB "                           amount of memory allocated to it for all",CRLF
   DB "                           instances combined.  This allows you to spot",CRLF
   DB "                           any 'pig applications' which might be running!",CRLF
   DB "               {none}      When no switch is supplied, this function prints",CRLF
   DB "                           a summary report of memory use in the system.",CRLF
   DB "                           This report differs between 386 Enhanced Mode",CRLF
   DB "                           and Standard Mode.  The 386 Enhanced Mode sum-",CRLF
   DB "                           mary includes information about Virtual and",CRLF
   DB "                           linear (RAM) memory, including an estimate of",CRLF
   DB "                           the total 'unused' amount of each type.",CRLF,CRLF
   DB 0

MIN      LABEL BYTE
   DB "MIN [window]",CRLF,LF
   DB "Toggles a window between its MINIMIZED state (Icon, if you will) and its",CRLF
   DB "RESTORED state (normal view).  If 'window' is a valid window handle that",CRLF
   DB "is capable of being minimized, it will be minimized.  Otherwise, if there",CRLF
   DB "is no argument, the Windows(tm) Command Line Interpreter window is minimized",CRLF
   DB "or restored to 'NORMAL'.  The 'WINDOW_STATE' variable can be used to verify",CRLF
   DB "if the window has already been minimized.  This variable will have one of",CRLF
   DB "the following values, depending on the current window state:",CRLF
   DB "       ",34,"MINIMIZED",34,"  Window is minimized",CRLF
   DB "       ",34,"NORMAL",34,"     Window is 'normal' (neither minimized nor maximized)",CRLF
   DB "       ",34,"MAXIMIZED",34,"  Window is maximized",CRLF,LF
   DB "To restore the main window to its NORMAL state, the following batch commands",CRLF
   DB "work rather nicely:",CRLF
   DB "    IF %WINDOW_STATE%==MINIMIZED MIN",CRLF
   DB "    IF %WINDOW_STATE%==MAXIMIZED MAX",CRLF,LF
   DB 0


NOT_INTERNAL MODE

MORE     LABEL BYTE

   DB "MORE [< inpath]",CRLF
   DB "{command} | MORE",CRLF,LF
   DB "This command is intended as a 'filter' for use with any command or program",CRLF
   DB "that generates text output to the screen (i.e. Standard Output, or 'CON'",CRLF
   DB "device).  SFTShell allows you to specify MS-DOS, Windows, and 'internal'",CRLF
   DB "commands as input to a 'piped' command.",CRLF
   DB "The 'MORE' command will display one screen at a time, and prompt you to",CRLF
   DB "press 'ENTER' to continue output.  Each prompt will be erased as it is",CRLF
   DB "displayed, so that you may subsequently copy the screen contents to the",CRLF
   DB "clipboard, or scroll back through the screen, without having the output",CRLF
   DB "intermixed with prompts to press 'ENTER' to continue.",CRLF
   DB LF, 0


MOVE     LABEL BYTE
   DB "MOVE source [dest]",CRLF,LF
   DB "This is much like the MS-DOS 'MOVE' command, which allows you to 'move'",CRLF
   DB "entire file sets from one directory to another.  This version is restricted",CRLF
   DB "to files on the same drive only, though they may be in different sub-",CRLF
   DB "directories on the same drive.   Also, the source and destination file name",CRLF
   DB "specifications are identical to those used by the 'COPY' command, with",CRLF
   DB "automatic conversion of sub-directory names to 'path\*.*', and so forth.",CRLF
   DB "Destination file names can contain either patterns (involving '?' or '*'",CRLF
   DB "wildcards), or a file / dir name.  If a directory name is specified for",CRLF
   DB "the destination, it is automatically converted to a wildcard 'path\*.*'",CRLF
   DB "destination pattern.",CRLF,LF
   DB "PARAMETERS:    source    - source file name (or pattern)",CRLF
   DB "               dest      - destination file name (assumes current dir).",CRLF
   DB "                           Characters from the source file are auto-",CRLF
   DB "                           matically substituted for any '?' or '*'",CRLF
   DB "                           characters in the destination.",CRLF,LF
;   DB "SWITCHES:      /s        - recurse sub-directories",CRLF,LF
;   DB "  NOTE:  When recursing sub-directories the 'MOVE' function will",CRLF
;   DB "         create new paths in the destination which match the paths",CRLF
;   DB "         in the source, and delete any 'empty' directories from",CRLF
;   DB "         the source path spec.  This literally 'PRUNES' from one",CRLF
;   DB "         node and 'GRAFTS' onto another.",CRLF
   DB 0


NET      LABEL BYTE
   DB "NET VIEW              (Win '95/NT only)",CRLF
   DB "NET VIEW workgroup    (Win '95/NT only)",CRLF
   DB "NET VIEW \\server     (Win '95/NT only)",CRLF
   DB "NET USE",CRLF
   DB "NET USE device sharename",CRLF
   DB "NET USE device /DELETE",CRLF,LF
   DB "This command allows you to invoke a limited number of 'NET' comands from",CRLF
   DB "within SFTShell.  The following is a short description of each:",CRLF,LF
   DB "  NET VIEW                 - list all servers on the network.",CRLF
   DB "  NET VIEW workgroup       - list all servers within a workgroup.",CRLF
   DB "  NET VIEW \\server        - list all share names for a server.",CRLF
   DB "  NET USE                  - list all connected network resources.",CRLF
   DB "  NET USE device sharename - connect the device 'device' to 'sharename'.",CRLF
   DB "                             (example:  NET USE LPT1: \\server\printer)",CRLF
   DB "  NET USE device /DELETE   - disconnect the specified device.",CRLF
   DB LF
   DB "NOTE:  'NET VIEW' is not supported under Win 3.1x.",CRLF
   DB 0


_NEXT    LABEL BYTE
   DB "NEXT [var]",CRLF
   DB "(PRELIMINARY)",CRLF,CRLF
;   DB "This command is used in conjuction with a 'FOR' command using the ENHANCED",CRLF
;   DB "form ('FOR var FROM start TO end [STEP stepval]'), and is only valid for",CRLF
;   DB "BATCH FILES.  This command increments the 'for' variable, and continues loop",CRLF
;   DB "execution until the 'for' variable exceeds the 'end' value.",CRLF,CRLF
;   DB "SEE ALSO:  EXIT LOOP, FOR, LOOP, REPEAT, UNTIL, WEND, WHILE",CRLF,CRLF
   DB 0


NOT_SUPPORT NLSFUNC


NUKETASK LABEL BYTE
   DB "NUKETASK task|instance|window|module name",CRLF,CRLF
   DB "This function allows you to 'NUKE' a task using either the Task 'ID', the",CRLF
   DB "Main Window 'ID', or the MODULE NAME.  To obtain a list of running tasks and",CRLF
   DB "their associated MODULE NAMES, Task 'ID's see the 'TASKLIST' function.",CRLF
   DB "The preferred method uses the MODULE NAME (normally the same as the Program",CRLF
   DB "Name, without the extension) to identify the task to be closed.  If more than",CRLF
   DB "one 'instance' of the same program is running a list of these tasks, their",CRLF
   DB "Task 'ID's, and their Main Window 'Captions' (titles) will appear, with",CRLF
   DB "sequence numbers to the left.  The program then prompts which one to delete.",CRLF
   DB "You may respond by entering the appropriate sequence number (1,2,3,etc.) or",CRLF
   DB "by pressing <ENTER> to cancel the operation.  For Task 'ID's and Main Window",CRLF
   DB "'ID's there will always be a unique task associated with it.",CRLF,CRLF
   DB "WARNING:  THIS IS THE LEAST PREFERRED METHOD OF ENDING A TASK!  Using this",CRLF
   DB "          method can leave limited resources (such as FONTS, BITMAPS, etc.)",CRLF
   DB "          allocated, even though the task has been ended.  Excessive use of",CRLF
   DB "          this function can lead to system failures, requiring Windows to",CRLF
   DB "          be restarted, or even a REBOOT, to correct the problems.",CRLF,CRLF
   DB "This function 'NUKE's the associated task by forcing Windows to end the task",CRLF
   DB "immediately, without any prompts or messages.",CRLF,CRLF
   DB "See also:  CLOSETASK, KILLTASK",CRLF,CRLF
   DB 0


ODBC LABEL BYTE
   DB "ODBC CONNECT [connect string]",CRLF
   DB "ODBC DISCONNECT [CONV_ID | ALL]",CRLF
   DB "ODBC SQL [CONV_ID] {program text}",CRLF
   DB "ODBC SQL BEGIN [CONV_ID]",CRLF
   DB "ODBC SQL END",CRLF
   DB "ODBC SQL TRANSACT BEGIN|END|ROLLBACK|CANCEL",CRLF
   DB "ODBC [CREATE|DROP] STATEMENT [CONV_ID]",CRLF
   DB "ODBC [function] [CONV_ID] [arguments]",CRLF,LF
   DB "The ODBC commands allow SFTShell to use the Microsoft Open Database Con-",CRLF
   DB "nectivity protocol to access any valid 16-bit ODBC driver that has been",CRLF
   DB "installed on your system, and perform standard SQL operations.",CRLF
   DB "The 'ODBC CONNECT' command establishes a connection with the an ODBC driver,",CRLF
   DB "and creates a 'statement' that defines a unique environment for accessing one",CRLF
   DB "or more tables through SQL 'SELECT' statements, or for performing standard",CRLF
   DB "operations within the specified database via SQL.  If an additional statement",CRLF
   DB "is needed, you can create one using the ODBC CREATE STATEMENT command.  Any",CRLF
   DB "one statement can be deleted using the ODBC DROP STATEMENT command.  The",CRLF
   DB "ODBC DISCONNECT command will automatically drop all open statements for the",CRLF
   DB "specified conversation, and disconnect the driver.",CRLF,LF
   DB "Following each command, the 'ODBC_RESULT' environment variable will be",CRLF
   DB "assigned a value appropriate to the action.  When performing a CONNECT or",CRLF
   DB "CREATE STATEMENT operation, it will be assigned the new 'CONV_ID' value",CRLF
   DB "that must be passed to the other commands.  A value of 'ERROR' is assigned",CRLF
   DB "whenever an error occurs.  The actual error information can then be obtained",CRLF
   DB "using the ODBC ERROR command.",CRLF,LF
   DB "PARAMETERS: function - this represents a valid ODBC command function, which",CRLF
   DB "                       may be one of the following:",CRLF
   DB "                       OPEN [tbl] - Opens table '[tbl]' for read/write access",CRLF
   DB "                                    so that you can perform UPDATE, INSERT,",CRLF
   DB "                                    and/or DELETE operations.",CRLF
   DB "                       EXECUTE    - Executes the current SQL program defined",CRLF
   DB "                                    by ODBC SQL BEGIN and ODBC SQL END.  The",CRLF
   DB "                                    results are saved as a 'Dynaset'.  This",CRLF
   DB "                                    statement does not wait for completion.",CRLF
   DB "                       UPDATE     - Performs an update operation on the",CRLF
   DB "                                    current record.",CRLF
   DB "                       DELETE     - Deletes the current record.",CRLF
   DB "                       INSERT     - Inserts a new record.",CRLF
   DB "                       FIRST      - Sets pointer to first record in table or",CRLF
   DB "                                    result 'Dynaset' and retrieves the record.",CRLF
   DB "                                    If the result set or table is empty, the",CRLF
   DB "                                    'ODBC_RESULT' variable will contain 'EOF'",CRLF
   DB "                                    and the retrieved record will be blank.",CRLF
   DB "                       NEXT       - Increments the record pointer in the",CRLF
   DB "                                    table or dynaset and retrieves the record.",CRLF
   DB "                                    Following 'OPEN' or 'EXECUTE', it behaves",CRLF
   DB "                                    the same as 'FIRST'.",CRLF
   DB "                                    When the record pointer passes the last",CRLF
   DB "                                    record in the table or Dynaset, it causes",CRLF
   DB "                                    an 'ODBC_RESULT' of 'EOF', and the record",CRLF
   DB "                                    it retrieves is blank.",CRLF
   DB "                       PREVIOUS   - Decrements the record pointer in the",CRLF
   DB "                                    table or dynaset and retrieves the record.",CRLF
   DB "                                    Following 'OPEN' or 'EXECUTE', it causes",CRLF
   DB "                                    an 'ODBC_RESULT' of 'EOF', and the record",CRLF
   DB "                                    it retrieves is blank.",CRLF
   DB "                       LAST       - Completes any deferred I/O for a Dynaset,",CRLF
   DB "                                    and retrieves the last record.",CRLF
   DB "                                    If the result set or table is empty, the",CRLF
   DB "                                    'ODBC_RESULT' variable will contain 'EOF'",CRLF
   DB "                                    and the retrieved record will be blank.",CRLF
   DB "                       CANCEL     - Cancels current SQL operation.",CRLF
   DB "                       BIND [v f] - Binds a variable 'v' to a field 'f' in the",CRLF
   DB "                                    Dynaset or table.  Any changes to the var-",CRLF
   DB "                                    iable will be reflected in the table for",CRLF
   DB "                                    UPDATE and INSERT operations.  As each",CRLF
   DB "                                    record is retrieved, the variable 'v'",CRLF
   DB "                                    will be assigned the corresponding value",CRLF
   DB "                                    from the retrieved record.",CRLF
   DB "                       TRANSACT   - Based on the command argument (BEGIN, END,",CRLF
   DB "                                    ROLLBACK, or CANCEL) the TRANSACT option",CRLF
   DB "                                    manages transactions for DBMS's that",CRLF
   DB "                                    support them.  See the SFTSHELL.HLP entry",CRLF
   DB "                                    on TRANSACTIONS for more information.",CRLF
;   DB "                       PARAMETERS - Following the 'PARAMETERS' keyword is an",CRLF
;   DB "                                    ASCII delimited list (commas and quote marks)",CRLF
;   DB "                                    or an ARRAY variable name containing the",CRLF
;   DB "                                    parameters to be passed to the ODBC driver",CRLF
;   DB "                                    on execution of an SQL statement containing",CRLF
;   DB "                                    '?' for parameter substitution.",CRLF
;   DB "                                    Where a string is required, quote marks will",CRLF
;   DB "                                    be added to the parameter by SFTShell.",CRLF
   DB LF
   DB "            CONV_ID  - the 'ODBC_RESULT' value returned from 'ODBC CONNECT'",CRLF
   DB "                       or 'ODBC CREATE STATEMENT'.",CRLF,LF
   DB "Possible values for ODBC_RESULT are as follows:",CRLF
   DB " [CONV_ID] - from ODBC CONNECT or ODBC CREATE STATEMENT.",CRLF
   DB " ERROR     - any ODBC function.",CRLF
   DB " OK        - any ODBC function except ODBC CONNECT or ODBC CREATE STATEMENT.",CRLF
   DB " EOF       - ODBC FIRST, ODBC NEXT, ODBC PREVIOUS, or ODBC LAST.",CRLF
   DB LF
   DB "For more information, see 'Using ODBC' in the 'Contents' section of the",CRLF
   DB "SFTSHELL.HLP Windows help file.",CRLF,LF
   DB 0

PATH     LABEL BYTE
   DB "PATH[=| ][newpath]",CRLF,LF
   DB "This is functionally equivalent to 'SET PATH=newpath' when 'newpath' is not",CRLF
   DB "blank.  If 'PATH' is the only item on the command line, then the current",CRLF
   DB "value for 'PATH' is displayed.  The value of the 'PATH' environment variable",CRLF
   DB "determines which drives/directories are to be searched whenever a program",CRLF
   DB "name is entered on the command line.  The search order always procedes as",CRLF
   DB "follows:",CRLF
   DB "  1:  Current drive/directory",CRLF
   DB "  2:  Windows(tm) directory",CRLF
   DB "  3:  Windows(tm) SYSTEM directory",CRLF
   DB "  4:  All directories in the 'PATH' environment variable, in order",CRLF
   DB "  5:  'Mapped' network drives",CRLF,LF
   DB "Each entry in the 'PATH' environment variable should be separated with a ';'",CRLF
   DB "and a ';' at the end of the PATH is optional.",CRLF
   DB 0


PAUSE    LABEL BYTE
   DB "PAUSE [message]",CRLF,LF
   DB "Pauses the execution of a batch job, and optionally prints a user-defined",CRLF
   DB "message in place of the (default) 'Press any key to continue . . .'.",CRLF
   DB "The 'message' parameter is an ENHANCEMENT to the MS-DOS(r) function.",CRLF
   DB 0


PLAYSOUND LABEL BYTE
   DB "PLAYSOUND [/W | /L] [soundfile]",CRLF
   DB "PS [/W | /L] [soundfile]",CRLF,LF
   DB "Plays the sound file 'soundfile' (a .WAV file) using the Multi-Media API.",CRLF
;   DB "(if present).  This function will work on Microsoft(r) Windows(tm) versions",CRLF
;   DB "3.1 and later, which include the Multi-Media extentions in the package.",CRLF
;   DB "Users of Windows(tm) 3.0 must have the Multi-Media API installed to use this",CRLF
;   DB "function.  If the library is not installed you will obtain an error message",CRLF
;   DB "indicating this if you try to use this function.",CRLF,LF
   DB "SWITCHES:        /W     - forces sound file to play 'SYNCHRONOUSLY' (waits",CRLF
   DB "                          for the sound file to finish before continuing)",CRLF,LF
   DB "                 /L     - forces sound file to play 'ASYNCHRONOUSLY' (does not",CRLF
   DB "                          wait for the sound file to finish before continuing)",CRLF
   DB "                          and 'loop' until a blank name is specified.",CRLF,LF
   DB "PARAMETERS:   soundfile - the name of a '.WAV' file containing the sound",CRLF
   DB "                          to be played on the Multi-Media WAVE device.",CRLF
   DB "                          If no extension is on the file, '.WAV' is",CRLF
   DB "                          assumed.  If this parameter is BLANK, all",CRLF
   DB "                          WAVE output will cease.  The sound file is",CRLF
   DB "                          played 'ASYNCHRONOUSLY' (i.e. at the same time",CRLF
   DB "                          SFTSHELL is processing additional commands) when",CRLF
   DB "                          possible.  (NOTE:  the PC SPEAKER driver cannot",CRLF
   DB "                          play sound files ASYNCHRONOUSLY).",CRLF,LF
   DB "Specifying a SYSTEM SOUND in lieu of 'soundfile' will play the appropriate",CRLF
   DB "SYSTEM sound (as defined in the control panel 'Sound' dialog).",CRLF,LF
   DB "The DEFAULT SYSTEM SOUNDS (and associated Message Box icons) are:",CRLF
   DB "   SystemAsterisk    - ""Asterisk"" (information) - uses 'Circle ""i""' icon.",CRLF
   DB "   SystemHand        - ""Critical Stop"" - uses 'Stop Sign' icon.",CRLF
   DB "   SystemDefault     - ""Default"" - no icon (default sound).",CRLF
   DB "   SystemExclamation - ""Exclamation"" - uses 'Exclamation Point' icon.",CRLF
   DB "   SystemQuestion    - ""Question"" - uses 'Question Mark' icon.",CRLF
   DB "   SystemExit        - ""Exit"" - no icon (plays on exit from windows).",CRLF
   DB "   SystemStart       - ""Start"" - no icon (plays on Windows startup).",CRLF,LF
   DB 0


PRINT    LABEL BYTE
   DB "PRINT filename",CRLF,LF
   DB "This function uses the registration database information to determine",CRLF
   DB "which application should be run to print the file specified, based on its",CRLF
   DB "extension.  The appropriate application is then executed using the SHELL",CRLF
   DB "interface, and commanded to print the specified file.  The application will",CRLF
   DB "normally terminate itself, depending on the commands stored within the",CRLF
   DB "registration database.",CRLF
   DB "PARAMETERS:  filename  - A file whose extension is included in the",CRLF
   DB "                         REGISTRATION database.  See the 'REGEDIT'",CRLF
   DB "                         application and the 'File Associate' menu",CRLF
   DB "                         within File Manager.",CRLF,LF
   DB 0


PROMPT   LABEL BYTE
   DB "PROMPT[=][value]",CRLF,LF
   DB "This is functionally equivalent to 'SET PROMPT=value' when 'value' is not",CRLF
   DB "blank.  If 'PROMPT' is the only item on the command line, then the current",CRLF
   DB "value for 'PROMPT' is displayed.  The value of the 'PROMPT' environment",CRLF
   DB "variable determines what is displayed when the system is waiting for the",CRLF
   DB "user to type a command, and what appears when ECHO is ON during batch file",CRLF
   DB "execution.  Certain characters (prefixed by '$') are special and generate",CRLF
   DB "unique functionality when included in the 'PROMPT' environment variable.",CRLF
   DB "Below is a list of these and a brief explanation of each:",CRLF
   DB "   $q    '=' (equals sign)",CRLF
   DB "   $$    '$' (dollars sign)",CRLF
   DB "   $t    Current Time",CRLF
   DB "   $d    Current Date",CRLF
   DB "   $p    Current drive:path",CRLF
   DB "   $v    MS-DOS(r) Version (and Windows Version - ENHANCEMENT)",CRLF
   DB "   $n    Current Drive Only",CRLF
   DB "   $g    '>' (greater than sign)",CRLF
   DB "   $l    '<' (less than sign)",CRLF
   DB "   $b    '|' (pipe symbol)",CRLF
   DB "   $_    Carriage Return/Line Feed Combination",CRLF
   DB "   $e    'Escape' character (ASCII 27., 1bH)",CRLF
   DB "   $h    Backspace Character",CRLF,LF
   DB "  NOTE:  The Windows(tm) Command Line Interpreter allows imbedded ANSI",CRLF
   DB "         escape sequences in the PROMPT and ECHO commands to force the",CRLF
   DB "         screen background and text colors to a specific value.  Some",CRLF
   DB "         ANSI sequences (such as BLINKING TEXT) are not supported.",CRLF
   DB "         See the MS-DOS(r) User's Guide and Reference for information",CRLF
   DB "         regarding ANSI escape sequences under the section for the",CRLF
   DB "         ANSI.SYS device driver.",CRLF
   DB 0



RD       LABEL BYTE
RMDIR    LABEL BYTE
   DB "RD dirname",CRLF
   DB "RMDIR dirname",CRLF,LF
   DB "This command removes a sub-directory.  If the sub-directory is in-use, or",CRLF
   DB "any files reside in it, it will not be deleted, and an error message will",CRLF
   DB "result.  This is essentially the opposite of the 'MD' or 'MKDIR' command.",CRLF
   DB 0


REM      LABEL BYTE
   DB "REM {comments}",CRLF,LF
   DB "Normally, this is used for comments in batch files.  From the command line",CRLF
   DB "it does absolutely nothing.",CRLF
   DB 0


REMOVE   LABEL BYTE
   DB "REMOVE [module handle|module name]",CRLF,LF
   DB "This command removes the specified module either by name or by handle",CRLF
   DB "from memory.  On occasions, especially if an application has a fatal error,",CRLF
   DB "certain modules may be 'left behind'.  These may be unloaded manually by",CRLF
   DB "use of the 'REMOVE' function.  Additionally, Windows(tm) may load fonts or",CRLF
   DB "other 'memory hog' modules that you do not *REALLY* want.  These may also",CRLF
   DB "be removed by this function.",CRLF,LF
   DB "WARNING:  This function tampers with normal Windows' memory management.",CRLF
   DB "          If you unload a module that is *IN USE* the results are un-",CRLF
   DB "          predictable and might possibly require a re-boot or other drastic",CRLF
   DB "          action on the part of the user.  It should be noted that unless",CRLF
   DB "          you are ABSOLUTELY SURE that you wish to unload a module you",CRLF
   DB "          should not do so...",CRLF
   DB 0


REN      LABEL BYTE
RENAME   LABEL BYTE
   DB "REN oldname newname",CRLF
   DB "RENAME oldname newname",CRLF,LF
   DB "This function, like its DOS counterpart, renames a file from one name to",CRLF
   DB "another, without changing its path name.  If a drive or path is specified in",CRLF
   DB "the 'newname' parameter, an ERROR will result.  Wildcards may be used in both",CRLF
   DB "the 'oldname' and 'newname' parameters.  As an extra added bonus, you may also",CRLF
   DB "rename a SUB-DIRECTORY using this function!!  In order to do this, you must be",CRLF
   DB "in the 'PARENT' directory at the time you invoke the command.",CRLF,LF
   DB "PARAMETERS:    oldname   - original file name (or pattern).",CRLF
   DB "               newname   - new file name (or pattern).  Characters from",CRLF
   DB "                           the original name are automatically substituted",CRLF
   DB "                           for any '?' or '*' characters.",CRLF
   DB 0


_REPEAT  LABEL BYTE
_UNTIL   LABEL BYTE
   DB "REPEAT",CRLF
   DB "  .. commands ..",CRLF
   DB "UNTIL [NOT] [condition]",CRLF,CRLF
   DB "(PRELIMINARY)",CRLF,CRLF
;   DB "This command causes a set of commands to be 'repeated' until the condition",CRLF
;   DB "specified by 'CONDITION' is FALSE.  Execution through the loop takes place",CRLF
;   DB "BEFORE the condition is checked (as indicated by the command syntax).",CRLF,CRLF
;   DB "SEE ALSO:  EXIT LOOP, FOR, LOOP, NEXT, WEND, WHILE",CRLF,CRLF
   DB 0


REPLACE  LABEL BYTE
   DB "REPLACE [source] [[dest]] [/A][/P][/R][/S][/U][/W]",CRLF,LF
   DB "Similar to copy, except that you can compare existing files in the",CRLF
   DB "destination directory to those in the source, and copy only those files",CRLF
   DB "which match the desired criteria.",CRLF,CRLF
   DB "SWITCHES:  /A  - Adds new files to destination directory instead of",CRLF
   DB "                 replacing existing files.",CRLF
   DB "                 This switch cannot be used with either '/S/ or '/U'",CRLF
   DB "           /P  - Prompts you for confirmation before replacing a",CRLF
   DB "                 destination file or adding a source file.",CRLF
   DB "           /R  - Replaces read-only files as well as normal files.",CRLF
   DB "           /S  - Recurse sub-directories in the destination path only.",CRLF
   DB "                 This switch cannot be used with '/A'.",CRLF
   DB "           /U  - Updates ONLY those files in the destination which are older",CRLF
   DB "                 than the files in the source.",CRLF
   DB "                 This switch cannot be used with '/A'.",CRLF
   DB "           /W  - Prompts you to insert the correct disk before searching",CRLF
   DB "                 for files to be copied.",CRLF,CRLF
   DB 0


_RETURN  LABEL BYTE
   DB "RETURN [value]",CRLF,CRLF
   DB "Valid only for BATCH FILES.  Terminates the current batch file, optionally",CRLF
   DB "assigning 'value' to the variable 'RETVAL' (blank if not specified).  This",CRLF
   DB "command may also be invoked using an 'IF' statement.",CRLF,CRLF
   DB 0


REXX     LABEL BYTE
   DB "REXX [rexx command]",CRLF,LF
   DB "Executes a REXX batch-language command on-line using QUERCUS SYSTEMS",CRLF
   DB "WREXX API library (if present).  If this library is not present, the",CRLF
   DB "command will fail.  Information regarding the REXX batch-language and",CRLF
   DB "the WREXX API and WINREXX application can be obtained from:",CRLF
   DB "    QUERCUS SYSTEMS",CRLF
   DB "    P.O. Box 2157",CRLF
   DB "    Saratoga, CA  95070",CRLF,LF
   DB 0


SET      LABEL BYTE
   DB "SET [varname=[string][%[varname2]%[string][%[varname3]%]...]]",CRLF,CRLF
   DB "This command either displays all environment variables, or assigns a new",CRLF
   DB "string value to an environment variable (creating it if it does not already",CRLF
   DB "exist).  Note that any spaces between the 'varname' and the '=' are included",CRLF
   DB "as part of the environment variables name (allowing imbedded spaces, so be",CRLF
   DB "careful when specifying variable names!).  Environment variables are used",CRLF
   DB "a number of items, plus many enhanced functions unique to the Windows(tm)",CRLF
   DB "Command Line Interpreter.",CRLF,CRLF
   DB "One enhancement over the MS-DOS(r) 'SET' command is the ability to include a",CRLF
   DB "variable name within '%' as part of the SET command from the command prompt.",CRLF
   DB "In the MS-DOS(r) version only BATCH files could reference another environment",CRLF
   DB "variable using the '%variable%' designation.  By doing this the user can",CRLF
   DB "make use of environment variables for temporary storage of the PATH or",CRLF
   DB "PROMPT variables, as follows:",CRLF
   DB "   SET TEMP_PATH=%PATH%        (this would put the path into 'TEMP_PATH')",CRLF,LF
   DB "Alternately, you could add something to an existing variable as follows:",CRLF
   DB "   SET PROMPT=Type 'EXIT' and press <ENTER> to return$_$_%PROMPT%",CRLF,LF
   DB "Note that there is no limit to environment variables within the Command",CRLF
   DB "Line Interpreter, but DOS programs expect environment variables to have",CRLF
   DB "a length less than 128 characters, including the name and equals sign.",CRLF
   DB 0


NOT_SUPPORT SHARE


SHIFT    LABEL BYTE
   DB "SHIFT",CRLF,LF
   DB "This command is used in batch files only, and causes the values of the",CRLF
   DB "parameters %0, %1, etc. to be 'shift'ed, such that %0 now represents",CRLF
   DB "what '%1' used to be, and so on.  This is one way to access more than 10",CRLF
   DB "parameters from the command line within a batch file.  Once the 'SHIFT'",CRLF
   DB "command has been used, it cannot be reversed.",CRLF
   DB 0


SORT     LABEL BYTE
   DB "SORT [/R] [/+n] [< inpath | inpath] [> outpath]",CRLF
   DB "{command} | SORT [/R] [/+n] [> outpath]",CRLF,LF
   DB "This command is intended as a 'filter' for use with any command or program",CRLF
   DB "that generates text output to the screen (i.e. Standard Output, or 'CON'",CRLF
   DB "device).  SFTShell allows you to specify MS-DOS, Windows, and 'internal'",CRLF
   DB "commands as input to a 'piped' command.",CRLF
   DB "The SORT command will sort the input file (or piped output from the prior",CRLF
   DB "command '{command}') in ASCII order, beginning with column 'n' (if the /+n",CRLF
   DB "switch was used), or in the first column (by default).",CRLF
   DB "The '/R' switch reverses the sort order (i.e. last to first).  Normally,",CRLF
   DB "the sort order is in ASCII order; i.e. the ASCII code of the characters",CRLF
   DB "are compared, and sorted from least to greatest, such that a blank is less",CRLF
   DB "than a '0', which is less than a '9', which is less than an 'A', which is",CRLF
   DB "less than a 'Z', and so forth.",CRLF
   DB LF, 0


SQL      LABEL BYTE
   DB "SQL BEGIN",CRLF
   DB "  ... commands ...",CRLF
   DB "SQL END",CRLF
   DB "SQL BROWSE",CRLF
   DB "(PRELIMINARY)",CRLF,CRLF
;   DB "EXECUTES an SQL Query using the Stewart~Frazier Tools, Inc. MULTI-THREAD",CRLF
;   DB "DATABASE API library 'WDBUTIL.DLL' (if present).",CRLF
;   DB "NOTE:  This also requires 'WMTHREAD.DLL', 'TOOLHELP.DLL', and 'COMMDLG.DLL'",CRLF
;   DB "       to load and execute.",CRLF,CRLF
   DB "For ODBC database access, use the ODBC command.",CRLF
   DB LF, 0


START    LABEL BYTE
   DB "START [/Minimized|/MAXimized|/Restored|/Hidden][/Wait] program [args]",CRLF,LF
   DB "This command starts a program as a separate process, optionally specifying",CRLF
   DB "the size of the application window, and optionally waiting for the process",CRLF
   DB "to complete before returning control to the program or command line.  This",CRLF
   DB "command is compatible with the Windows NT and Win '95 'START' command, with",CRLF
   DB "some additional enhancements.",CRLF,LF
   DB "PARAMETERS: program    - this is the name of the program that you wish to run",CRLF
   DB "                         using the 'START' command.  This program will always",CRLF
   DB "                         run as a separate process, similar to typing the name",CRLF
   DB "                         of the program at the command prompt, or as a command",CRLF
   DB "                         line argument to either the 'CALL' or 'LOAD' commands.",CRLF
   DB "            args       - this represents the optional command line arguments",CRLF
   DB "                         for the specified program.",CRLF
   DB "            /MAXimized - This switch starts the application with a MAXIMIZED",CRLF
   DB "                         main window.  It cannot be specified with either the",CRLF
   DB "                         /H, /M, or /R switches.",CRLF
   DB "            /Minimized - This switch starts the application with a MINIMIZED",CRLF
   DB "                         (Iconic) window.  It cannot be specified with either",CRLF
   DB "                         the /H, /MAX, or /R switches.",CRLF
   DB "            /Restored  - This switch starts the application with a RESTORED",CRLF
   DB "                         (normal) window.  It cannot be specified with either",CRLF
   DB "                         the /H, /MAX, or /M switches.",CRLF
   DB "            /Hidden    - This switch specifies that the main window should be",CRLF
   DB "                         initially HIDDEN.  It cannot be specified with either",CRLF
   DB "                         the /MAX, /MIN, or /R switches.  A hidden window must",CRLF
   DB "                         be explicitly shown, either by using the MIN or MAX",CRLF
   DB "                         commands, or by using the 'ShowWindow()' function.",CRLF
   DB "            /Wait      - This switch causes SFTShell to wait until the program",CRLF
   DB "                         terminates before it contines with the next command.",CRLF
   DB "                         This is functionally similar to the 'CALL' command.",CRLF
   DB CRLF
   DB "NOTE:  lower case letters in switch values denote optional characters.",CRLF
   DB CRLF
   DB "Specifying the '/Wait' option causes the 'RETVAL' environment variable to be",CRLF
   DB "assigned the 'return code' value from the application, in the same manner as",CRLF
   DB "the 'CALL' command.",CRLF
   DB CRLF
   DB "See also:  CALL, LOAD, RETURN",CRLF
   DB LF, 0


SUBST    LABEL BYTE
   DB "SUBST [drive:] [/D|[drive:][path]]",CRLF,LF
   DB "The SUBST command accesses the DOS internal data structures that maintain",CRLF
   DB "information about logical drives, and either lists all 'SUBST'ed drives,",CRLF
   DB "or optionally add or delete a 'SUBST'ed drive to the system.",CRLF
   DB "A new logical drive may be 'SUBST'ituted for another drive.  This causes",CRLF
   DB "MS-DOS(r) to treat the root directory of the 'SUBST'ed drive as though",CRLF
   DB "it was the same as the 'SUBST'ed drive and directory.  For example if I",CRLF
   DB "wish to have drive 'E' point to the 'D:\E_DRIVE' sub-directory I would issue",CRLF
   DB "the following command:",CRLF
   DB "   SUBST E: D:\E_DRIVE",CRLF
   DB "If both drives and the path are valid the 'E' drive will be created and",CRLF
   DB "point to the same files as the 'D:\E_DRIVE' directory.  When the command",CRLF
   DB "'DIR E:\*.*' is entered, a directory of 'D:\E_DRIVE' will be shown,",CRLF
   DB "except that it will appear to be part of the 'E:\' directory.",CRLF,LF
   DB "To 'SUBST'itute a drive for a path on another drive, the command format is:",CRLF
   DB "   SUBST D: C:\path",CRLF
   DB "   where   'D:' is the new logical drive to create (must not exist)",CRLF
   DB "    and    'C:\path' is the sub-directory to which 'D:' will refer",CRLF,LF
   DB "To delete a 'SUBST'ed drive, use the logical drive designator and the '/D'",CRLF
   DB "switch as follows:",CRLF
   DB "   SUBST D: /D",CRLF
   DB "   where   'D:' is the logical 'SUBST'ed drive to be deleted",CRLF,CRLF
   DB 0


SYS      LABEL BYTE
   DB "SYS [boot drive:[path]] dest drive:",CRLF,LF
   DB "Copies the MS-DOS system files (IO.SYS and MSDOS.SYS) from the boot",CRLF
   DB "drive (either specified by 'boot drive:[path]' or the 'default') to",CRLF
   DB "the destination drive.  If 'boot drive:[path]' is not specified, the",CRLF
   DB "current drive is assumed; as an enhancement, if no system files are",CRLF
   DB "found on the current drive (current path or root directory) then the",CRLF
   DB "system will attempt to determine which drive the 'boot' drive was when",CRLF
   DB "the system was started.  If it is not possible to make this determination",CRLF
   DB "the function will fail.  Otherwise, the system files will be copied from",CRLF
   DB "the appropriate drive, as well as 'COMMAND.COM'.",CRLF,LF
   DB 0


TL       LABEL BYTE
TASKLIST LABEL BYTE
   DB "TL",CRLF
   DB "TASKLIST",CRLF,LF
   DB "This provides the user with a list of tasks that are currently running from",CRLF
   DB "within the Windows(tm) environment.  See also 'MEM' for additional info",CRLF
   DB "that can be obtained about other functions running from within Windows(tm).",CRLF
   DB 0

TIME     LABEL BYTE
   DB "TIME [newtime]",CRLF,LF
   DB "This command allows the user to view the current system time, or else change",CRLF
   DB "it to a new value.  If 'TIME' with no arguments is specified, the current",CRLF
   DB "system time is displayed; otherwise, the 'newtime' argument is assigned to",CRLF
   DB "the system clock.",CRLF
   DB 0

TREE     LABEL BYTE
   DB "TREE [pathname]",CRLF,LF
   DB "This produces a rather nice display of the directory structure for either",CRLF
   DB "the current drive:directory combination, or the drive:directory specified",CRLF
   DB "by 'pathname'.",CRLF
   DB 0


TRUENAME LABEL BYTE
   DB "TRUENAME [filename]",CRLF,LF
   DB "This (undocumented?) function began with DOS 4.0 - if a SUBST or JOIN or",CRLF
   DB "'.' or '..' is present for the 'filename' parameter specified by the user",CRLF
   DB "this function will determine the 'TRUE' pathname component for such a drive",CRLF
   DB "or directory name, and the fully qualified (result) path is displayed. This",CRLF
   DB "corresponds to INT 21H function 60H (an undocumented(?) DOS function).",CRLF,LF
   DB 0


_TYPE     LABEL BYTE
   DB "TYPE [filename]",CRLF,LF
   DB "This function types the contents of 'filename' into the SFTSHELL display.",CRLF
   DB "It is recommended that (especially for large files) a program such as",CRLF
   DB "'NOTEPAD.EXE' be used to view the file, rather than using 'TYPE'.  This",CRLF
   DB "command has been included mostly for compatibility.",CRLF
   DB 0

NOT_INTERNAL UNDELETE


VER      LABEL BYTE
   DB "VER",CRLF,LF
   DB "This command displays the current MS-DOS(r) version, AND Windows(tm) version",CRLF
   DB "(an enhancement over the MS-DOS(r) functionality).",CRLF
   DB 0

VERIFY   LABEL BYTE
   DB "VERIFY [ON|OFF]",CRLF,LF
   DB "Specifies whethter MS-DOS(r) is to verify that write operations are done",CRLF
   DB "correctly.  The 'on' value enables this verification process, and hence",CRLF
   DB "causes disk operations to be slowed down somewhat.  The 'off' value",CRLF
   DB "disables it, resulting in slightly faster (but possibly unreliable) disk",CRLF
   DB "write operations.  Specifying the 'VERIFY' command only (with no argument)",CRLF
   DB "displays the current VERIFY status.  NOTE:  This is a GLOBAL status!",CRLF
   DB 0

VOL      LABEL BYTE
   DB "VOL [drive:]",CRLF,LF
   DB "Displays the volume label for drive 'drive' (or default if none specified)",CRLF
   DB "and serial number, when applicable.",CRLF
   DB 0


_WAIT     LABEL BYTE
   DB "WAIT FOR time [/Q]",CRLF
   DB "WAIT UNTIL [date|time|date time] [/Q]",CRLF,LF
   DB "This is a powerful command that tells the BATCH processor to WAIT until",CRLF
   DB "a particular date and time, or for a specified time interval.  If the date",CRLF
   DB "is not specified, the program waits for a particular time only.  As an",CRLF
   DB "added bonus, you may specify a day of the week in place of the date.",CRLF,LF
   DB "PARAMETERS:   date  -  this may be specified in one of the acceptable",CRLF
   DB "                       date formats.  Additionally, you can specify",CRLF
   DB "                       a day of the week (i.e. Sunday, Monday, etc.)",CRLF
   DB "                       which is case insensitive.  Dates may be specified",CRLF
   DB "                       as MM/DD/YY, MM/DD/YYYY, MM-DD-YY, DD-Mmm-YY, and",CRLF
   DB "                       related format variations (specifying 2 digit or",CRLF
   DB "                       4 digit years, leaving the year off, etc.",CRLF
   DB "                       NOTE:  This parameter may only be specified if",CRLF
   DB "                              the 'WAIT UNTIL' option is used.",CRLF
   DB "              time  -  optional if 'date' specified.  Required if",CRLF
   DB "                       the 'WAIT FOR' option is in use.  With the",CRLF
   DB "                       'WAIT UNTIL' option, specifies the time at which",CRLF
   DB "                       control returns to the batch file.  For the",CRLF
   DB "                       'WAIT FOR' option, specifies the time duration",CRLF
   DB "                       (Hours:Minutes:Seconds) to suspend execution.",CRLF
   DB "                       Time is specified in HH:MM:SS format; leaving off",CRLF
   DB "                       the seconds assumes 'HH:MM'.  Specifying a single",CRLF
   DB "                       integer value with 'WAIT FOR' assumes that number",CRLF
   DB "                       of seconds.  It is recommended that you minimize",CRLF
   DB "                       the window using the 'MIN' command before 'WAIT'.",CRLF
   DB "              /Q       This switch inhibits the printing of any status",CRLF
   DB "                       messages - desirable for use in batch files in",CRLF
   DB "                       which the 'WAIT' command is used for periodic",CRLF
   DB "                       polling, or other (similar) operations.  In",CRLF
   DB "                       such cases the status messages would be of",CRLF
   DB "                       little use and may clutter the screen.",CRLF,LF
   DB 0


_WEND    LABEL BYTE
_WHILE   LABEL BYTE
   DB "WHILE [NOT] [condition]",CRLF
   DB "  .. commands ..",CRLF
   DB "WEND",CRLF
   DB "(PRELIMINARY)",CRLF,CRLF
;   DB "This command causes a set of commands to be 'repeated' as long as the",CRLF
;   DB "condition specified by 'CONDITION' is TRUE.  Execution through the loop",CRLF
;   DB "takes place AFTER the condition is checked (as indicated by the command",CRLF
;   DB "syntax).",CRLF,CRLF
;   DB "SEE ALSO:  EXIT LOOP, FOR, LOOP, NEXT, REPEAT, UNTIL",CRLF,CRLF
   DB 0


XCOPY    LABEL BYTE
   DB "XCOPY [source] [dest] [switches]",CRLF,LF
   DB "Implements an equivalent of MS-DOS(r)'s 'XCOPY' command, but in background",CRLF
   DB "(as with other copy operations).  Creates any necessary directories before",CRLF
   DB "copying any files, to ensure that the paths are available.",CRLF
;   DB "Optional '/X' switch removes certain file specifications from those copied.",CRLF
;   DB "This may include multiple file specs separated by commas.",CRLF
   DB CRLF
   DB "PARAMETERS:  [source]   - this is a valid path specification which may",CRLF
   DB "                          contain wildcard characters.",CRLF
   DB "             [dest]     - this is a valid drive, dir, or drive:dir combi-",CRLF
   DB "                          nation - if left off assumes current directory.",CRLF
   DB "             [switches] - optional switches (see below).",CRLF,LF
   DB "SWITCHES:    /a         - copies only source files that have the 'ARCHIVE'",CRLF
   DB "                          bit set.",CRLF
   DB "             /d:date    - copy only files modified on or after a certain date.",CRLF
;   DB "             /-d:date   - ENHANCEMENT - copy files modified on or BEFORE a",CRLF
;   DB "                          certain date!",CRLF
   DB "             /e         - copies any subdirectories, even if empty.  Use",CRLF
   DB "                          in combination with '/S' (below).",CRLF
   DB "             /m         - like '/a', above, except that archive bits in the",CRLF
   DB "                          source files are turned off after copying.",CRLF
   DB "             /p         - prompts for each file name whether or not to copy.",CRLF
   DB "                          NOTE:  this prompting is done prior to placing the",CRLF
   DB "                          files into the COPY QUEUE, and not at the time they",CRLF
   DB "                          are actually copied!!!",CRLF
   DB "             /s         - recursively copies files in sub-directories,",CRLF
   DB "                          creating new directories as required.",CRLF
   DB "             /v         - verifies each file after copying.",CRLF
   DB "             /w         - Displays a 'warning' message prompting you to press",CRLF
   DB "                          <ENTER> to commence copying files.",CRLF
;   DB "             /x:f1[,f2] - ENHANCEMENT - 'Exclude' option to exclude these",CRLF
;   DB "                          file specifications. Multiple file specifications",CRLF
;   DB "                          may be excluded by separating them with commas (,)",CRLF
;   DB "                          and they may contain wildcard characters.  One use",CRLF
;   DB "                          for this would be to exclude '.BAK' files, for",CRLF
;   DB "                          example.",CRLF,LF
;   DB 0
   DB LF,0



_HELP_COMMANDS_ LABEL BYTE

   DB "The following is a brief list of commands that are recognized internally",CRLF
   DB "by the Windows(tm) Command Line Interpreter:",CRLF
   DB CRLF
   DB "     ATTRIB    BREAK     CALC      CALL      CD        CHDIR     CLOSETASK ",CRLF
   DB "     CLS       COMMAND   COMP      COPY      CT        DATE      DBLSPACE  ",CRLF
   DB "     DDE       DEFINE    DEL       DELTREE   DIR       ECHO      ELSE      ",CRLF
   DB "     ENDIF     ERASE     EXIT      FC        FIND      FOR       FORMAT    ",CRLF
   DB "     GOTO      HELP      IF        INPUT     JOIN      KILLTASK  KT        ",CRLF
   DB "     LABEL     LET       LH        LISTOPEN  LO        LOAD      LOADHIGH  ",CRLF
   DB "     MAX       MD        MEM       MIN       MKDIR     MORE      MOVE      ",CRLF
   DB "     NEXT      NUKETASK  ODBC      PATH      PAUSE     PLAYSOUND PRINT     ",CRLF
   DB "     PROMPT    PS        RD        REM       REMOVE    REN       RENAME    ",CRLF
   DB "     REPEAT    REPLACE   RETURN    RMDIR     SET       SHIFT     SORT      ",CRLF
   DB "     SUBST     SYS       TASKLIST  TIME      TL        TREE      TRUENAME  ",CRLF
   DB "     TYPE      UNDELETE  VER       VERIFY    VOL       WAIT      WEND      ",CRLF
   DB "     WHILE     XCOPY     ",CRLF
   DB CRLF
   DB "Other commands are interpreted as PROGRAM NAMES and are run 'asynchronously'",CRLF
   DB "unless invoked through the 'CALL' command, or within a 'piped' command.",CRLF
   DB "For information on waiting for a program to complete prior to continuing,",CRLF
   DB "type 'HELP CALL' at the command prompt.",CRLF
   DB CRLF
   DB "NOTE:  The majority of the commands listed above are processed internally",CRLF
   DB "       by SFTSHELL.  A few of them, however, were included for the purpose",CRLF
   DB "       of future expansion, and currently run the MS-DOS(r) program which",CRLF
   DB "       program which supplies the required function.",CRLF
   DB "       For any internal command in which there is a corresponding MS-DOS(r)",CRLF
   DB "       program, the MS-DOS(r) program may be run instead by specifying the",CRLF
   DB "       extension, such as 'SUBST.EXE' vice 'SUBST', or by prefixing the",CRLF
   DB "       command with 'COMMAND /C'.",CRLF
   DB CRLF
   DB "PIPING and I/O REDIRECTION through the use of '|', '<' or '>'",CRLF
   DB "SFTShell fully supports all I/O redirection and piping.  Some filters (such",CRLF
   DB "as 'MORE') have been implemented internally.  Others still run the MS-DOS",CRLF
   DB "program, but send all output to SFTShell's display.  Normally, an MS-DOS",CRLF
   DB "program will send all output to the screen (i.e. 'standard output', or the",CRLF
   DB "'CON' device), and runs asynchronously.  If, however, a '|' (pipe) command",CRLF
   DB "is used, ANY such application runs synchronously with respect to SFTShell,",CRLF
   DB "as though the 'CALL' command were used to invoke it.  Each 'piped' section",CRLF
   DB "of the command line is executed independently, in the proper order, with",CRLF
   DB "re-direction to temporary files provided by SFTShell, including MS-DOS",CRLF
   DB "applications.  Output resulting from all piped expressions is sent to the",CRLF
   DB "SFTShell display screen, unless a final output redirection is used.",CRLF
   DB "NOTE:  MS-DOS allows you to specify '>' and '<' anywhere within a piped",CRLF
   DB "       expression.  With SFTShell, however, you must specify the input",CRLF
   DB "       re-direction BEFORE the first '|', and the final output re-direction",CRLF
   DB "       (if any) after the last command in the expression.",CRLF
   DB "HINT:  To force output from an MS-DOS application to the SFTShell screen,",CRLF
   DB "       place a '|' at the end of the command, followed by nothing.",CRLF
   DB "       Example:  'EXEHDR myfile.exe |' sends output to SFTShell's screen",CRLF
   DB CRLF
   DB "For help on special keys used by the Command Line Interpreter, enter",CRLF
   DB "'HELP KEYS' at the prompt and press <ENTER>.",CRLF
   DB "For help on special environment variables, enter 'HELP VARIABLES' at the",CRLF
   DB "prompt and press <ENTER>.",CRLF
   DB "For help on ANSI sequences, type 'HELP ANSI' at the prompt and press <ENTER>.",CRLF
   DB CRLF
   DB "Information on various high-level language features and extensions, such as",CRLF
   DB "the 'LET' command, 'IF' and 'FOR' extensions, and algebraic functions, can",CRLF
   DB "be found in the accompanying WINHELP file 'SFTSHELL.HLP', under 'High Level",CRLF
   DB "Language Features'.",CRLF,LF
   DB CRLF
   DB "To access the 'SFTSHELL.HLP' Windows(tm) Help File, containing additional",CRLF
   DB "detailed information about SFTShell, select 'SFTShell Help Contents' from",CRLF
   DB "the Help menu, or type the name of the file at the command prompt.",CRLF
   DB CRLF, 0



_HELP_KEYS_ LABEL BYTE

   DB "The following keys have the associated functions listed below:",CRLF,LF
   DB "   KEY          FUNCTION",CRLF
   DB " -------------- -------------------------------------------------",CRLF
   DB " F1             Context Sensitive Help (current command line)",CRLF
   DB " F3             Re-types previous command line (like COMMAND.COM)",CRLF
   DB " Up, Down       Command history - rotates through the list",CRLF
   DB " Right, Left,   Command line editing - similar to 'DOSKEY' in its",CRLF
   DB "  Home, End     implementation.  Use the DELETE, BACKSPACE, and",CRLF
   DB "                INSERT keys to modify the current command text.",CRLF
   DB " Alt-F3         Command Line History (dialog box)",CRLF
   DB " Ctrl-F3        Copy the COMMAND HISTORY buffer to the Clipboard",CRLF
   DB " PgUp           Scrolls Screen Up by a specific number of lines",CRLF
   DB " PgDn           Scrolls Screen Down by a specific number of lines",CRLF
   DB " Ctrl-Up        Scrolls Screen Up by a single line",CRLF
   DB " Ctrl-Down      Scrolls Screen Down by a single line",CRLF
   DB " Ctrl-Rt        Scrolls Screen to the right by 8 characters",CRLF
   DB " Ctrl-Left      Scrolls Screen to the left by 8 characters",CRLF
   DB " Ctrl-PgUp      Scrolls Screen to the left by 8 characters",CRLF
   DB " Ctrl-PgDn      Scrolls Screen to the right by 8 characters",CRLF
   DB " Ctrl-Shft-Ins  Copies *entire* screen buffer to the clipboard!",CRLF
   DB " Ctrl-Break     Stops processing on certain operations",CRLF
   DB " Ctrl-C         Same as Ctrl-Break",CRLF
   DB " Ctrl-S,Ctrl-Q  With certain functions stalls and resumes output",CRLF
   DB " Alt-F4         Exits program (exits Windows(tm) if SHELL)",CRLF,CRLF
   DB 0


_HELP_VARIABLES_ LABEL BYTE

   DB "The following Environment Variables are pre-defined for use by SFTSHELL and",CRLF
   DB "yield special results/values when used in 'smart' batch programs.  Below",CRLF
   DB "is a list of these variables and a brief description of each:",CRLF,CRLF
   DB " VARIABLE NAME        Description of Contents and Usage",CRLF
   DB "--------------------- ---------------------------------------------------------",CRLF
   DB " TASK_ID              Generated whenever a program is run from within SFTSHELL.",CRLF
   DB "                      It contains a hexadecimal representation of its Task ID.",CRLF,LF
   DB " RETVAL               This is used by 'RETURN', 'CALL', and 'START' to store",CRLF
   DB "                      a returned value.  For 'RETURN', the value is the result",CRLF
   DB "                      from an equation that appears on the command line.  For",CRLF
   DB "                      'CALL' and 'START', it is the return code from the app-",CRLF
   DB "                      lication on exit.  (This is only valid for 'START' when",CRLF
   DB "                      the '/Wait' option is used).",CRLF,LF
   DB " WINDOW_STATE         Contains either the string 'MINIMIZED', 'MAXIMIZED', or",CRLF
   DB "                      'NORMAL'.  It is updated whenever the window state has",CRLF
   DB "                      been changed, either by the 'MIN' and/or 'MAX' commands,",CRLF
   DB "                      or by the System Menu or Minimize/Maximize buttons.",CRLF,LF
   DB " COPYING              Set to either 'TRUE' (if copying in background) or",CRLF
   DB "                      'FALSE' (if no copies in progress).",CRLF,LF
   DB " FORMATTING           Set to either 'TRUE' (if formatting in background) or",CRLF
   DB "                      'FALSE' if not.",CRLF,LF
   DB " DDE_RESULT           Indicates Return (error) state of the last DDE Operation",CRLF
   DB "                      Type 'HELP DDE' for information on the DDE functions.",CRLF
   DB "                      Possible values are: 'OK','ERROR','TIMEOUT','BUSY','NAK'",CRLF
   DB "                      (On return from 'DDE INITIATE' it contains the conver-",CRLF
   DB "                       sation information, to be passed to subsequent 'DDE'",CRLF
   DB "                       commands [unaltered]).",CRLF,LF
   DB " DDE_SERVER_LIST      Contains a list of registered servers, maintained via the",CRLF
   DB "                      'DDE REGISTER' and 'DDE UNREGISTER' commands.",CRLF
   DB "                      (This value should never be modified directly).",CRLF,LF
   DB "                      The arguments are separated by ',' characters, which",CRLF
   DB "                      enables use of the ARRAY subscript operator to access",CRLF
   DB "                      the values in this variable.",CRLF,LF
   DB " DDE_SERVERTOPIC_LIST A list of 'DDE 'SERVER|TOPIC' combinations, maintained",CRLF
   DB "                      via the 'DDE REGISTER' and 'DDE UNREGISTER' commands.",CRLF
   DB "                      (This value should never be modified directly).",CRLF
   DB "                      The arguments are separated by ',' characters, which",CRLF
   DB "                      enables use of the ARRAY subscript operator to access",CRLF
   DB "                      the values in this variable.",CRLF,LF
   DB " ODBC_RESULT          Indicates Return (error) state of the last ODBC Opera-",CRLF
   DB "                      tion.  Type 'HELP ODBC' for more information on the",CRLF
   DB "                      ODBC functions and possible values.",CRLF,LF
   DB " CMD_ERROR            This variable contains a ZERO or NON-ZERO value",CRLF
   DB "                      depending on the result from the last command.  If",CRLF
   DB "                      the command was successful, the value is ",22H,"0",22H,".  If",CRLF
   DB "                      the command failed (due to an error), the variable",CRLF
   DB "                      contains a non-zero numeric value.",CRLF,LF
   DB " CALC_DIAG            Setting this variable to a non-blank / non-zero",CRLF
   DB "                      value causes additional diagnostic information to",CRLF
   DB "                      be displayed whenever the expression evaluator",CRLF
   DB "                      encounters an error.  Normally this variable is",CRLF
   DB "                      *NOT* assigned a value, which evaluates to FALSE.",CRLF
   DB CRLF
   DB "Environment variables are used not only for storing system configuration",CRLF
   DB "data (like the PATH, PROMPT, and DIRCMD variables), or for system status",CRLF 
   DB "(like the variables listed above), but also as variables in algebraic",CRLF
   DB "EXPRESSIONS, which are used in conjunction with the 'LET', 'CALC', and",CRLF 
   DB "'IF (expression)' commands, and a special form of text substition using '%'.",CRLF
   DB "As is the standard in MS-DOS(r), an environment variable may be used for",CRLF
   DB "text substitution in a command line by surrounding it with '%' characters.",CRLF
   DB "Although COMMAND.COM only allows text substitution in BATCH files, the",CRLF
   DB "Windows(tm) Command Line Interpreter allows you to specify environment",CRLF
   DB "variables surrounded by '%' characters on commands entered from the keyboard",CRLF
   DB "as well as BATCH file statements.  In addition, the expression evaluator",CRLF
   DB "provides the ability to place a valid algebraic EXPRESSION (surrounded by",CRLF
   DB "parentheses) within the '%' characters; the format is '%(expression)%',",CRLF 
   DB "where 'expression' is a valid algebraic EXPRESSION, as defined by the",CRLF
   DB "'High Level Language Calculation Overview' in 'SFTSHELL.HLP, included with",CRLF
   DB "this program. The 'High Level Language Calculation Overview' documents each",CRLF
   DB "operator and function in detail, and provides code samples to assist in",CRLF
   DB "developing 'BATCH' applications designed to run within the SFTShell Command",CRLF
   DB "Line Interpreter for Windows(tm).",CRLF,CRLF
   DB "See also:  'LET', 'CALC', 'IF', 'DDE', 'MAX', 'MIN'",CRLF
   DB CRLF
   DB 0



_HELP_ANSI_ LABEL BYTE
   DB "ANSI SEQUENCES SUPPORTED BY SFTSHELL",CRLF,CRLF
   DB "One unique feature of SFTSHELL is the ability to support most of the standard",CRLF
   DB "ANSI sequences that are supported by the ANSI.SYS keyboard driver.  The ANSI",CRLF
   DB "command sequences are a series of standard ASCII printable characters, pre-",CRLF
   DB "ceded by an ESCAPE '^[' character (ASCII 27 {1BH}).  Numeric values are",CRLF
   DB "specified as DECIMAL ASCII strings embedded into the sequence.",CRLF,CRLF
   DB "The following standard ANSI sequences are supported by SFTSHELL:",CRLF,CRLF
   DB "    SEQUENCE      Description",CRLF
   DB " ---------------  -------------------------------------------------------",CRLF
   DB "  <ESC>[y;xH      Move cursor to row 'y', column 'x', where '0;0' would",CRLF
   DB "                  represent the upper left corner of the screen",CRLF,CRLF
   DB "  <ESC>[y;xf      Same as above.",CRLF,CRLF
   DB "  <ESC>[ÎyA       Cursor up by 'Îy' lines; column does not change.",CRLF,CRLF
   DB "  <ESC>[ÎyB       Cursor down by 'Îy' lines; column does not change.",CRLF,CRLF
   DB "  <ESC>[ÎxC       Cursor right by 'Îx' columns; row does not change.",CRLF,CRLF
   DB "  <ESC>[ÎxD       Cursor left by 'Îx' columns; row does not change.",CRLF,CRLF
   DB "  <ESC>[s         Save current cursor position - see <ESC>[u (below).",CRLF,CRLF
   DB "  <ESC>[u         Restore cursor position saved by <ESC>[s (above).",CRLF,CRLF
   DB "  <ESC>[2J        Erase display, move cursor to row 0, column 0.",CRLF,CRLF
   DB "  <ESC>[K         Erase to end of line from current column.",CRLF,CRLF
   DB "  <ESC>[‡0;...‡Xm Using numeric values '‡0' through '‡X' (as appropriate)",CRLF
   DB "                  it assigns colors and other text attributes to all of",CRLF
   DB "                  the displayable characters following the sequence.",CRLF
   DB "          0       All attributes off.",CRLF
   DB "          1       Bold Text on.",CRLF
   DB "          4       Underscore On (NOT SUPPORTED).",CRLF
   DB "          5       Blinking Text on (NOT SUPPORTED).",CRLF
   DB "          7       Reverse Video on (NOT SUPPORTED).",CRLF
   DB "          8       Concealed on (NOT SUPPORTED).",CRLF
   DB "    30 through 39 FOREGROUND (TEXT) COLOR - Black, red, green, yellow,",CRLF
   DB "                                            blue, magenta, cyan, white.",CRLF
   DB "    40 through 49 BACKGROUND (SCREEN) COLOR - Black, red, green, yellow,",CRLF
   DB "                                              blue, magenta, cyan, white.",CRLF,CRLF
   DB "  <ESC>[=zH       Set video mode (NOT SUPPORTED).",CRLF,CRLF
   DB "  <ESC>[=zl       Reset video mode (NOT SUPPORTED).",CRLF,CRLF
   DB "  <ESC>[c;s;...p  Remaps keyboard string for 'c' to 's' (NOT SUPPORTED).",CRLF,CRLF
   DB 0


_HELP_TEXT2_ ENDS


END
