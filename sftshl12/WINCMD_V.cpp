/***************************************************************************/
/*                                                                         */
/*   WINCMD_V.C - Command line interpreter for Microsoft(r) Windows (tm)   */
/*                                                                         */
/*                      TRADE SECRET - CONFIDENTIAL                        */
/*                                                                         */
/*             The compiled libraries and API header files are             */
/*          (c) Copyright 1990-1997 by Stewart~Frazier Tools, Inc          */
/*                                                                         */
/*   This source file contains the STATIC variable definitions for GLOBAL  */
/*   variables that are common to all source files.                        */
/*                                                                         */
/***************************************************************************/



#define THE_WORKS
#include "mywin.h"
#include "dos.h"

//#define STATIC_VARS         /* prevents re-defining static variables */

#include "wincmd.h"         /* 'dependency' related definitions */
#include "wincmd_f.h"       /* function prototypes and variables */
#include "sftshell.ver"       /* version information */


                       /*************************/
                       /*** STATIC  VARIABLES ***/
                       /*************************/

HINSTANCE hInst=(HINSTANCE)NULL;                /* handle for this instance */
HWND      hMainWnd=(HWND)NULL;                    /* handle for main window */
HWND      hFrameWnd=(HWND)NULL;          // handle for FRAME around 'hMainWnd'
HACCEL    hAccel=(HACCEL)NULL;
BOOL      IsShell=FALSE;       /* TRUE if app is the SHELL; FALSE otherwise */

BOOL      IsChicago=FALSE;     /* TRUE if WIN 3.80 or greater; FALSE else   */
BOOL      IsNT=FALSE;

char FAR *lpScreen=NULL;

//char (__based(lpScreen) *screen)[SCREEN_COLS]=NULL; // array containing screen buffer
//char (__based(lpScreen) *attrib)[SCREEN_COLS]=NULL; // array containing attrib bytes
  
//UINT _screen = 0;
//UINT _attrib = 0;

SCREEN_BASED_PLINE pScreen = NULL;
SCREEN_BASED_PLINE pAttrib = NULL;


  
int iScreenLines=128;           // # of lines in screen buffer
int LineScrollCount=0;          // # of lines to scroll when painting screen
                                // and direction to scroll them (+up,-down)

LOCALHANDLE lhCmd[COMMAND_HIST_SIZE]={(LOCALHANDLE)0};
                                              /* record of last 32 commands */

volatile int maxlines=0, curline=0, curcol=0;
                                        /* max line and current line/column */
                                        /* on display (applies to KB input) */
volatile int cur_attr=0x0f;     /* current attribute value - WHITE on BLACK */
volatile int start_line=0, start_col=0;   /* starting line/column for input */
volatile int end_line=0, end_col=0;   /* ending line/col for input (so far) */

volatile BOOL ctrl_break = FALSE;       /* the 'ctrl-break is active' flag! */
volatile BOOL stall_output = FALSE;  /* the 'stall output' flag! (ctrl s,q) */

_DOSERROR last_error;         /* ext error information for last error */

volatile LPSTR lpMyEnv  = (LPSTR)NULL;      /* pointer to my (current) env. */
volatile WORD  orig_env = 0;
volatile WORD env_size = 0;

volatile LPSTR lpCmdHist = (LPSTR)NULL; /* points to command history buffer */
volatile LPSTR lpCmdHistIndex = NULL;   /* 'DOSKEY' pointer for arrows keys */
volatile LPSTR lpLastCmd = (LPSTR)NULL;       /* points to previous command */
volatile LPSTR lpCurCmd  = (LPSTR)NULL; /* points to current command!! This */
                                           /* is NULL if no command waiting */

volatile LPCOPY_INFO lpCopyInfo=(LPCOPY_INFO)NULL;/* points to 'head' struc */
volatile BOOL copying = FALSE;           /* 'TRUE' if copying in background */

volatile BOOL formatting = FALSE;     /* 'TRUE' if formatting in background */

volatile BOOL pausing_flag = FALSE;    /* 'TRUE' if PAUSE command in effect */

volatile BOOL waiting_flag = FALSE;     /* 'TRUE' if WAIT command in effect */
                                     /* this affects the main message loop! */
volatile BOOL waiting_quiet = FALSE;    // a 'quiet' flag for waiting
                                        // if TRUE, no message when done

volatile SFTDATE WaitForDay = {0,0,0};     /* flags which cause a wait until a */
volatile SFTTIME WaitForTime = {0,0,0};    /* particular date/time.            */
volatile int  WaitForTimeFlag = 0;


volatile BOOL copy_complete_flag = FALSE;/* 'TRUE' when last copy completes */

volatile BOOL GettingUserInput = FALSE;   /* 'TRUE' if getting 'user input' */

volatile BOOL BatchMode = FALSE;                     /* batch mode switches */
volatile BOOL BatchEcho = TRUE;
volatile BOOL IsCall = FALSE;  // TRUE when 'CALL' is used...
                               // remains TRUE if waiting for program to end
volatile BOOL IsStart = FALSE; // TRUE if 'START' invoked the program - this
                               // causes all BATCH and REXX files to run in
                               // a separate instance of SFTSHELL.

volatile BOOL IsFor  = FALSE;  /* similar to 'IsCall' but for 'FOR' command */
volatile BOOL IsRexx = FALSE;  /* about the same thing as 'IsFor' */

volatile DWORD dwLastProcessID = 0;        // assigned by SFTSH32X (when used)

volatile WORD PipingFlag = FALSE;          // increment for each 'pipe' level

volatile LPBATCH_INFO lpBatchInfo = NULL;         /* batch file information */
volatile LPSMARTFILEPARM lpSmartFileParm = NULL;

volatile BOOL BlockCopyResults = FALSE;     /* TRUE to prevent printing the */
                                          /* # of files added to copy queue */

volatile int WindowState=0x8000;     /* 0 for normal, 1 for max, -1 for min */
                                  /* this value forces an update right away */


volatile BOOL InvalidateFlag = TRUE;  /* TRUE when entire client is invalid */
                                      /* allows skipping 'InvalidateRect()' */
volatile TEXTMETRIC tmMain;              /* updated 1st time window paints! */
volatile BOOL TMFlag         = FALSE;    /* FALSE until tmMain is updated.  */

volatile HANDLE hCopyThread    = NULL;  /* handles for various threads */
volatile HANDLE hFormatThread  = NULL;
volatile HANDLE hBatchThread   = NULL;
volatile HANDLE hWaitThread    = NULL;
volatile HANDLE hCommandThread = NULL;
volatile HANDLE hFlashThread   = NULL;
volatile HANDLE hMemoryBatchFileThread = NULL;



WORD wIGROUP = 0;                            /* 'IGROUP' segment for KERNEL */
WIN_VER uVersion = {0};               /* the current Windows and DOS version!! */

char work_buf[STD_BUFFER_SIZE * 4], UserInputBuffer[STD_BUFFER_SIZE];

char TypeAhead[STD_BUFFER_SIZE];
WORD TypeAheadHead=0, TypeAheadTail=0;

struct find_t ff;

BOOL insert_mode = FALSE;
BOOL own_caret = FALSE;
int  caret_hidden = 0;
HBITMAP hCaret1=(HBITMAP)NULL, hCaret2=(HBITMAP)NULL;

char cmd_buf[STD_BUFFER_SIZE] = "";
int cmd_buf_len = 0;
BOOL ctrl_p=FALSE;

HFILE redirect_output=HFILE_ERROR, redirect_input=HFILE_ERROR;
HDC   redirect_output_DC=NULL;


DWORD idDDEInst = 0;    /* the ID for the DDEML instance */
LPSTR lpDDEInfo = NULL; /* the 'info' block for DDE conversations */

LPSTR lpHotLinkItems = NULL; // this contains a list of 'hot-linked' items
                             // which allows excellent optimization!

char pq[]="???";
char *pOpenMode[8]={"Rd","Wrt","R/W",pq,"RWN",pq,pq,pq};
char *pShareMode[8]={"Comp","Excl","NoWr","NoRd","Shr",pq,pq,pq};



const char szWINDOW_STATE[] = "WINDOW_STATE";
const char szMAXIMIZED[]    = "MAXIMIZED";
const char szMINIMIZED[]    = "MINIMIZED";
const char szNORMAL[]       = "NORMAL";

const char szFORMATTING[]   = "FORMATTING";
const char szCOPYING[]      = "COPYING";
const char szTRUE[]         = "TRUE";
const char szFALSE[]        = "FALSE";
const char szRETVAL[]       = "RETVAL";
const char szDEMO[]         = "DEMO";
const char szPATH[]         = "PATH";





#define PCMDB __based(__segname("PROCESS_COMMAND_TEXT"))

static char PCMDB _szAPPEND[]    = "APPEND";
static char PCMDB _szASSIGN[]    = "ASSIGN";
static char PCMDB _szATTRIB[]    = "ATTRIB";
static char PCMDB _szBREAK[]     = "BREAK";
static char PCMDB _szCALC[]      = "CALC";
static char PCMDB _szCALL[]      = "CALL";
static char PCMDB _szCD[]        = "CD";
static char PCMDB _szCHCP[]      = "CHCP";
static char PCMDB _szCHDIR[]     = "CHDIR";
static char PCMDB _szCLOSETASK[] = "CLOSETASK";
static char PCMDB _szCLS[]       = "CLS";
static char PCMDB _szCOMMAND[]   = "COMMAND";
static char PCMDB _szCOMP[]      = "COMP";
static char PCMDB _szCONTINUE[]  = "CONTINUE";
static char PCMDB _szCOPY[]      = "COPY";
static char PCMDB _szCT[]        = "CT";
static char PCMDB _szCTTY[]      = "CTTY";
static char PCMDB _szDATE[]      = "DATE";
static char PCMDB _szDBLSPACE[]  = "DBLSPACE";
static char PCMDB _szDDE[]       = "DDE";
static char PCMDB _szDEBUG[]     = "DEBUG";
static char PCMDB _szDEFINE[]    = "DEFINE";
static char PCMDB _szDEL[]       = "DEL";
static char PCMDB _szDELTREE[]   = "DELTREE";
static char PCMDB _szDIR[]       = "DIR";
static char PCMDB _szDOSKEY[]    = "DOSKEY";
static char PCMDB _szECHO[]      = "ECHO";
static char PCMDB _szEDIT[]      = "EDIT";
static char PCMDB _szELSE[]      = "ELSE";
static char PCMDB _szEND[]       = "END";
static char PCMDB _szENDIF[]     = "ENDIF";
static char PCMDB _szERASE[]     = "ERASE";
static char PCMDB _szEXIT[]      = "EXIT";
static char PCMDB _szEXPAND[]    = "EXPAND";
static char PCMDB _szFC[]        = "FC";
static char PCMDB _szFDISK[]     = "FDISK";
static char PCMDB _szFIND[]      = "FIND";
static char PCMDB _szFOR[]       = "FOR";
static char PCMDB _szFORMAT[]    = "FORMAT";
static char PCMDB _szGOTO[]      = "GOTO";
static char PCMDB _szGRAFTABL[]  = "GRAFTABL";
static char PCMDB _szGRAPHICS[]  = "GRAPHICS";
static char PCMDB _szHELP[]      = "HELP";
static char PCMDB _szIF[]        = "IF";
static char PCMDB _szINPUT[]     = "INPUT";
static char PCMDB _szJOIN[]      = "JOIN";
static char PCMDB _szKEYB[]      = "KEYB";
static char PCMDB _szKILLTASK[]  = "KILLTASK";
static char PCMDB _szKT[]        = "KT";
static char PCMDB _szLABEL[]     = "LABEL";
static char PCMDB _szLET[]       = "LET";
static char PCMDB _szLH[]        = "LH";
static char PCMDB _szLISTOPEN[]  = "LISTOPEN";
static char PCMDB _szLO[]        = "LO";
static char PCMDB _szLOAD[]      = "LOAD";
static char PCMDB _szLOADHIGH[]  = "LOADHIGH";
static char PCMDB _szMAX[]       = "MAX";
static char PCMDB _szMD[]        = "MD";
static char PCMDB _szMEM[]       = "MEM";
static char PCMDB _szMIN[]       = "MIN";
static char PCMDB _szMKDIR[]     = "MKDIR";
static char PCMDB _szMODE[]      = "MODE";
static char PCMDB _szMORE[]      = "MORE";
static char PCMDB _szMOVE[]      = "MOVE";
static char PCMDB _szNET[]       = "NET";
static char PCMDB _szNEXT[]      = "NEXT";
static char PCMDB _szNLSFUNC[]   = "NLSFUNC";
static char PCMDB _szNUKETASK[]  = "NUKETASK";
static char PCMDB _szODBC[]      = "ODBC";
static char PCMDB _szPATH[]      = "PATH";
static char PCMDB _szPAUSE[]     = "PAUSE";
static char PCMDB _szPLAYSOUND[] = "PLAYSOUND";
static char PCMDB _szPRINT[]     = "PRINT";
static char PCMDB _szPROMPT[]    = "PROMPT";
static char PCMDB _szPS[]        = "PS";
static char PCMDB _szRD[]        = "RD";
static char PCMDB _szREM[]       = "REM";
static char PCMDB _szREMOVE[]    = "REMOVE";
static char PCMDB _szREN[]       = "REN";
static char PCMDB _szRENAME[]    = "RENAME";
static char PCMDB _szREPEAT[]    = "REPEAT";
static char PCMDB _szREPLACE[]   = "REPLACE";
static char PCMDB _szRETURN[]    = "RETURN";
static char PCMDB _szREXX[]      = "REXX";
static char PCMDB _szRMDIR[]     = "RMDIR";
static char PCMDB _szSET[]       = "SET";
static char PCMDB _szSHARE[]     = "SHARE";
static char PCMDB _szSHIFT[]     = "SHIFT";
static char PCMDB _szSORT[]      = "SORT";
static char PCMDB _szSQL[]       = "SQL";
static char PCMDB _szSTART[]     = "START";
static char PCMDB _szSUBST[]     = "SUBST";
static char PCMDB _szSYS[]       = "SYS";
static char PCMDB _szTASKLIST[]  = "TASKLIST";
static char PCMDB _szTIME[]      = "TIME";
static char PCMDB _szTL[]        = "TL";
static char PCMDB _szTREE[]      = "TREE";
static char PCMDB _szTRUENAME[]  = "TRUENAME";
static char PCMDB _szTYPE[]      = "TYPE";
static char PCMDB _szUNDELETE[]  = "UNDELETE";
static char PCMDB _szUNTIL[]     = "UNTIL";
static char PCMDB _szVER[]       = "VER";
static char PCMDB _szVERIFY[]    = "VERIFY";
static char PCMDB _szVOL[]       = "VOL";
static char PCMDB _szWAIT[]      = "WAIT";
static char PCMDB _szWEND[]      = "WEND";
static char PCMDB _szWHILE[]     = "WHILE";
static char PCMDB _szXCOPY[]     = "XCOPY";


#define AA const char PCMDB *

const char PCMDB * const PCMDB cmd_array[] = {
 (AA)_szAPPEND,  (AA)_szASSIGN,  (AA)_szATTRIB,  (AA)_szBREAK,   (AA)_szCALC,
 (AA)_szCALL,    (AA)_szCD,      (AA)_szCHCP,    (AA)_szCHDIR,   (AA)_szCLOSETASK,
 (AA)_szCLS,     (AA)_szCOMMAND, (AA)_szCOMP,
 (AA)_szCONTINUE,(AA)_szCOPY,    (AA)_szCT,      (AA)_szCTTY,    (AA)_szDATE,    (AA)_szDBLSPACE,
 (AA)_szDDE,     (AA)_szDEBUG,   (AA)_szDEFINE,  (AA)_szDEL,     (AA)_szDELTREE, (AA)_szDIR,
 (AA)_szDOSKEY,  (AA)_szECHO,    (AA)_szEDIT,    (AA)_szELSE,    (AA)_szEND,     (AA)_szENDIF,
 (AA)_szERASE,   (AA)_szEXIT,    (AA)_szEXPAND,  (AA)_szFC,      (AA)_szFIND,    (AA)_szFOR,
 (AA)_szFORMAT,  (AA)_szGOTO,    (AA)_szGRAFTABL,(AA)_szGRAPHICS,(AA)_szHELP,    (AA)_szIF,
 (AA)_szINPUT,   (AA)_szJOIN,    (AA)_szKEYB,    (AA)_szKILLTASK,(AA)_szKT,      (AA)_szLABEL,
 (AA)_szLET,     (AA)_szLH,      (AA)_szLISTOPEN,(AA)_szLO,      (AA)_szLOAD,    (AA)_szLOADHIGH,
 (AA)_szMAX,     (AA)_szMD,      (AA)_szMEM,     (AA)_szMIN,
 (AA)_szMKDIR,   (AA)_szMODE,    (AA)_szMORE,    (AA)_szMOVE,    (AA)_szNET,     (AA)_szNEXT,    
 (AA)_szNLSFUNC ,(AA)_szNUKETASK,(AA)_szODBC,    (AA)_szPATH,    (AA)_szPAUSE,   (AA)_szPLAYSOUND,
 (AA)_szPRINT,   (AA)_szPROMPT,  (AA)_szPS,      (AA)_szRD,      (AA)_szREM,     (AA)_szREMOVE,
 (AA)_szREN,     (AA)_szRENAME,  (AA)_szREPEAT,  (AA)_szREPLACE, (AA)_szRETURN,  (AA)_szREXX,
 (AA)_szRMDIR,   (AA)_szSET,     (AA)_szSHARE,   (AA)_szSHIFT,   (AA)_szSORT,    (AA)_szSQL,
 (AA)_szSTART,   (AA)_szSUBST,   (AA)_szSYS,     (AA)_szTASKLIST,(AA)_szTIME,    (AA)_szTL,
 (AA)_szTREE,    (AA)_szTRUENAME,(AA)_szTYPE,    (AA)_szUNDELETE,(AA)_szUNTIL,   (AA)_szVER,
 (AA)_szVERIFY,  (AA)_szVOL,     (AA)_szWAIT,    (AA)_szWEND,    (AA)_szWHILE,   (AA)_szXCOPY
 };

#undef AA


WORD wNcmd_array = N_DIMENSIONS(cmd_array);

CMD_FN * const PCMDB fn_array[]={
   CMDNoRun,   CMDNoRun,   CMDAttrib,   CMDBreak,   CMDCalc,
   CMDCall,    CMDChdir,   CMDNoRun,    CMDChdir,   CMDCloseTask,
   CMDCls,     CMDCommand, CMDComp,
   CMDContinue,CMDCopy,    CMDCloseTask,CMDNoRun,   CMDDate,    CMDDblspace,
   CMDDde,     CMDDebug,   CMDDefine,   CMDDel,     CMDDeltree, CMDDir,
   CMDNoRun,   CMDEcho,    CMDEdit,     CMDElse,    CMDEnd,     CMDEndif,
   CMDDel,     CMDExit,    CMDExpand,  CMDFc,       CMDFind,    CMDFor,
   CMDFormat,  CMDGoto,    CMDNoRun,    CMDNoRun,   CMDHelp,    CMDIf,
   CMDInput,   CMDJoin,    CMDNoRun,    CMDKilltask,CMDKilltask,CMDLabel,
   CMDLet,     CMDLoadhigh,CMDListOpen, CMDListOpen,CMDLoadhigh,CMDLoadhigh,
   CMDMax,     CMDMkdir,   CMDMem,      CMDMin,
   CMDMkdir,   CMDMode,    CMDMore,     CMDMove,    CMDNet,     CMDNext,
   CMDNoRun,   CMDNuketask,CMDOdbc,    CMDPath,     CMDPause,   CMDPlaySound,
   CMDPrint,   CMDPrompt,  CMDPlaySound,CMDRmdir,   CMDRem,     CMDRemove,
   CMDRename,  CMDRename,  CMDRepeat,   CMDReplace, CMDReturn,  CMDRexx,
   CMDRmdir,   CMDSet,     CMDShare,    CMDShift,   CMDSort,    CMDSql,
   CMDStart,   CMDSubst,   CMDSys,      CMDTasklist,CMDTime,    CMDTasklist,
   CMDTree,    CMDTruename,CMDType,     CMDUndelete,CMDUntil,   CMDVer,
   CMDVerify,  CMDVol,     CMDWait,    CMDWend,     CMDWhile,   CMDXcopy
   };


char *pDdeOption[]={"ADVISE","EXECUTE","INITIATE","POKE","REGISTER",
                    "REQUEST","TERMINATE","UNADVISE","UNREGISTER"};
WORD wNDdeOption=N_DIMENSIONS(pDdeOption);

char *pOdbcOption[]={"BIND","CANCEL","CONNECT","CREATE","CURSOR","DELETE",
                     "DISCONNECT","DROP","ERROR","EXECUTE","FILTER","FIRST",
                     "INSERT","LAST","LOCK","NEXT","OPEN","PARAMETERS",
                     "PREVIOUS","SQL","TRANSACT","UNLOCK","UPDATE" };
WORD wNOdbcOption=N_DIMENSIONS(pOdbcOption);


char pDDESERVERLIST[]     = "DDE_SERVER_LIST";
char pDDESERVERTOPICLIST[]= "DDE_SERVERTOPIC_LIST";
#ifdef RXSHELL
char pDEFSERVERNAME[]     = "RXSHELL";          /* default server name */
#else
char pDEFSERVERNAME[]     = "SFTSHELL";          /* default server name */
#endif
char *pPROGRAMNAME        = pDEFSERVERNAME;  /* the program name also! */
char pDDERESULT[]         = "DDE_RESULT";    /* env var for dde result */

char pCOMMA[]      = ",";
char pOK[]         = "OK";
char pERROR[]      = "ERROR";

char pSYSTEM[]     = "SYSTEM";

char pTASK_ID[]    = "TASK_ID";            /* 'TASK_ID' variable name */


           /*** EXTERNALLY DEFINED MODULES & PROC ADDRESSES ***/

HMODULE hToolHelp=(HMODULE)NULL;
HMODULE hCommDlg =(HMODULE)NULL;
HMODULE hMMSystem=(HMODULE)NULL;
HMODULE hShell   =(HMODULE)NULL;
HMODULE hKernel  =(HMODULE)NULL;
HMODULE hUser    =(HMODULE)NULL;
HMODULE hDdeml   =(HMODULE)NULL;
HMODULE hODBC    =(HMODULE)NULL;
HMODULE hRexxAPI =(HMODULE)NULL; /* module handle for WREXX.DLL (REXX API) */
HMODULE hWDBUTIL =(HMODULE)NULL; /* module handle for WDBUTIL.DLL */


#ifndef WIN32
BOOL (WINAPI *lpGetOpenFileName)(OPENFILENAME FAR *lpofn)        =NULL;
BOOL (WINAPI *lpGetSaveFileName)(OPENFILENAME FAR *lpofn)        =NULL;
BOOL (WINAPI *lpChooseFont)(CHOOSEFONT FAR *lpcf)                =NULL;
DWORD (WINAPI *lpCommDlgExtendedError)(void)                     =NULL;

BOOL (WINAPI *lpGlobalFirst)(LPGLOBALENTRY lpGlobal, WORD wFlags)=NULL;
BOOL (WINAPI *lpGlobalNext)(LPGLOBALENTRY lpGlobal, WORD wFlags) =NULL;
BOOL (WINAPI *lpMemManInfo)(LPMEMMANINFO lpInfo)                 =NULL;
BOOL (WINAPI *lpModuleFirst)(LPMODULEENTRY lpModule)             =NULL;
BOOL (WINAPI *lpModuleNext)(LPMODULEENTRY lpModule)              =NULL;
HTASK (WINAPI *lpTaskFindHandle)(LPTASKENTRY lpTask, HTASK hTask)=NULL;
HMODULE (WINAPI *lpModuleFindHandle)(LPMODULEENTRY lpModule,
                                         HMODULE hModule)            =NULL;
BOOL (WINAPI *lpNotifyRegister)(HTASK, LPFNNOTIFYCALLBACK, WORD) =NULL;
BOOL (WINAPI *lpNotifyUnRegister)(HTASK)                         =NULL;

BOOL (WINAPI *lpIsTask)(HTASK hTask)                             =NULL;
BOOL (WINAPI *lpTaskFirst)(LPTASKENTRY lpTask)                   =NULL;
BOOL (WINAPI *lpTaskNext)(LPTASKENTRY lpTask)                    =NULL;

DWORD (WINAPI *lpGlobalMasterHandle)(void)                       =NULL;
DWORD (WINAPI *lpGlobalHandleNoRIP)(WORD wSeg)                   =NULL;

BOOL (WINAPI *lpsndPlaySound)(LPCSTR lpszSoundName, WORD wFlags)     =NULL;


// WOW Generic Thunk procs (WOWNT16.H)

DWORD hKernel32  = 0L;    // 32-bit module (Generic Thunk)
DWORD hWOW32     = 0L;    // 32-bit module (Generic Thunk)
DWORD hMPR       = 0L;    // 32-bit module (Generic Thunk)
DWORD hSFTSH32T  = 0L;    // 32-bit module (Generic Thunk)


DWORD (WINAPI *lpGetVDMPointer32W)(LPVOID vp, UINT fMode)        = NULL;
DWORD (WINAPI *lpLoadLibraryEx32W)(LPCSTR lpszLibFile,
                                       DWORD hFile, DWORD dwFlags)   = NULL;
DWORD (WINAPI *lpGetProcAddress32W)(DWORD hModule,
                                        LPCSTR lpszProc)             = NULL;
DWORD (WINAPI *lpFreeLibrary32W)(DWORD hLibModule)               = NULL;

DWORD (FAR __cdecl *lpCallProcEx32W)(DWORD dwCount, DWORD lpCvt,
                                     DWORD lpProc, ... )             = NULL;

#define CPEX_DEST_STDCALL   0x00000000L  /* 'or' with 'dwCount' parm */
#define CPEX_DEST_CDECL     0x80000000L  /* 'or' with 'dwCount' parm */



// 16-bit WNET stuff (from USER.EXE)

UINT (WINAPI *lpWNetAddConnection)(LPSTR, LPSTR, LPSTR)          = NULL;
UINT (WINAPI *lpWNetCancelConnection)(LPSTR, BOOL)               = NULL;
UINT (WINAPI *lpWNetGetConnection)(LPSTR, LPSTR, UINT FAR *)     = NULL;



// REGISTRY


LONG (WINAPI *lpRegCloseKey)(HKEY)                               =NULL;
LONG (WINAPI *lpRegCreateKey)(HKEY, LPCSTR, HKEY FAR *)          =NULL;
LONG (WINAPI *lpRegDeleteKey)(HKEY, LPCSTR)                      =NULL;
LONG (WINAPI *lpRegEnumKey)(HKEY, DWORD, LPSTR, DWORD)           =NULL;
LONG (WINAPI *lpRegOpenKey)(HKEY, LPCSTR, HKEY FAR *)            =NULL;
LONG (WINAPI *lpRegQueryValue)(HKEY, LPSTR, LPSTR, LONG FAR *)   =NULL;
LONG (WINAPI *lpRegSetValue)(HKEY, LPCSTR, DWORD, LPCSTR, DWORD) =NULL;


void (WINAPI *lpDragAcceptFiles)(HWND, BOOL)                     =NULL;
void (WINAPI *lpDragFinish)(HANDLE)                              =NULL;
WORD (WINAPI *lpDragQueryFile)(HANDLE, WORD, LPSTR, WORD)        =NULL;

HINSTANCE (WINAPI *lpShellExecute)(HWND, LPCSTR, LPCSTR, LPCSTR,
                                       LPCSTR, int)                  =NULL;


// Windows '95 SHELL functions (actually located in KERNEL)

LONG (WINAPI *lpRegDeleteValue)(HKEY, LPCSTR)                    =NULL;
LONG (WINAPI *lpRegQueryValueEx)(HKEY, LPCSTR, LONG FAR *,
                                     LONG FAR *, LPBYTE, LONG FAR *) =NULL;
LONG (WINAPI *lpRegEnumValue)(HKEY, DWORD, LPCSTR,
                                  LONG FAR *, DWORD, LONG FAR *,
                                  LPBYTE, LONG FAR *)                =NULL;
LONG (WINAPI *lpRegSetValueEx)(HKEY, LPCSTR, DWORD, DWORD,
                                   LPBYTE, DWORD)                    =NULL;



BOOL      (WINAPI *lpDdeAbandonTransaction)(DWORD, HCONV, DWORD)     =NULL;
BYTE      (WINAPI *lpDdeAccessData)(HDDEDATA, DWORD FAR *)           =NULL;
HDDEDATA  (WINAPI *lpDdeAddData)(HDDEDATA, void FAR *, DWORD, DWORD) =NULL;
HDDEDATA  (WINAPI *lpDdeClientTransaction)(void FAR *, DWORD, HCONV,
                                HSZ, UINT, UINT, DWORD, DWORD FAR *) =NULL;
int       (WINAPI *lpDdeCmpStringHandles)(HSZ, HSZ)                  =NULL;
HCONV     (WINAPI *lpDdeConnect)(DWORD, HSZ, HSZ, CONVCONTEXT FAR *) =NULL;
HCONVLIST (WINAPI *lpDdeConnectList)(DWORD, HSZ, HSZ, HCONVLIST,
                                     CONVCONTEXT)                    =NULL;
HDDEDATA  (WINAPI *lpDdeCreateDataHandle)(DWORD, void FAR *, DWORD,
                                          DWORD, HSZ, UINT, UINT)    =NULL;
HSZ       (WINAPI *lpDdeCreateStringHandle)(DWORD, LPCSTR, int)      =NULL;
BOOL      (WINAPI *lpDdeDisconnect)(HCONV)                           =NULL;
BOOL      (WINAPI *lpDdeDisconnectList)(HCONVLIST)                   =NULL;
BOOL      (WINAPI *lpDdeEnableCallback)(DWORD, HCONV, UINT)          =NULL;
BOOL      (WINAPI *lpDdeFreeDataHandle)(HDDEDATA)                    =NULL;
BOOL      (WINAPI *lpDdeFreeStringHandle)(DWORD, HSZ)                =NULL;
DWORD     (WINAPI *lpDdeGetData)(HDDEDATA, void FAR *, DWORD, DWORD) =NULL;
UINT      (WINAPI *lpDdeGetLastError)(DWORD)                         =NULL;
UINT      (WINAPI *lpDdeInitialize)(DWORD FAR *, PFNCALLBACK, DWORD,
                                    DWORD)                           =NULL;
BOOL      (WINAPI *lpDdeKeepStringHandle)(DWORD, HSZ)                =NULL;
HDDEDATA  (WINAPI *lpDdeNameService)(DWORD, HSZ, HSZ, UINT)          =NULL;
BOOL      (WINAPI *lpDdePostAdvise)(DWORD, HSZ, HSZ)                 =NULL;
UINT      (WINAPI *lpDdeQueryConvInfo)(HCONV, DWORD, CONVINFO FAR *) =NULL;
HCONV     (WINAPI *lpDdeQueryNextServer)(HCONVLIST, HCONV)           =NULL;
DWORD     (WINAPI *lpDdeQueryString)(DWORD, HSZ, LPSTR, DWORD, int)  =NULL;
HCONV     (WINAPI *lpDdeReconnect)(HCONV)                            =NULL;
BOOL      (WINAPI *lpDdeSetUserHandle)(HCONV, DWORD, DWORD)          =NULL;
BOOL      (WINAPI *lpDdeUnaccessData)(HDDEDATA)                      =NULL;
BOOL      (WINAPI *lpDdeUninitialize)(DWORD)                         =NULL;

HDDEDATA  (CALLBACK *lpDdeCallBack)(UINT,UINT,HCONV,HSZ,HSZ,
                                    HDDEDATA,DWORD,DWORD)            =NULL;

#endif // WIN32


RETCODE (SQL_API *lpSQLAllocConnect)(HENV,HDBC FAR *)                =NULL;
RETCODE (SQL_API *lpSQLAllocEnv)(HENV FAR *)                         =NULL;
RETCODE (SQL_API *lpSQLAllocStmt)(HDBC,HSTMT FAR *)                  =NULL;
RETCODE (SQL_API *lpSQLBindCol)(HSTMT,UWORD,SWORD,PTR,SDWORD,
                                SDWORD FAR *)                        =NULL;
RETCODE (SQL_API *lpSQLCancel)(HSTMT)                                =NULL;
RETCODE (SQL_API *lpSQLColAttributes)(HSTMT,UWORD,UWORD,PTR,SWORD,
                                      SWORD FAR *,SDWORD FAR *)      =NULL;
RETCODE (SQL_API *lpSQLConnect)(HDBC,UCHAR FAR *,SWORD,UCHAR FAR *,
                                SWORD, UCHAR FAR *,SWORD)            =NULL;
RETCODE (SQL_API *lpSQLDescribeCol)(HSTMT,UWORD,UCHAR FAR *,SWORD,
                                    SWORD FAR *,SWORD FAR *,
                                    UDWORD FAR *, SWORD FAR *,
                                    SWORD FAR *)                     =NULL;
RETCODE (SQL_API *lpSQLDisconnect)(HDBC)                             =NULL;
RETCODE (SQL_API *lpSQLError)(HENV,HDBC,HSTMT,UCHAR FAR *,
                              SDWORD FAR *, UCHAR FAR *,SWORD,
                              SWORD FAR *)                           =NULL;
RETCODE (SQL_API *lpSQLExecDirect)(HSTMT,UCHAR FAR *,SDWORD)         =NULL;
RETCODE (SQL_API *lpSQLExecute)(HSTMT)                               =NULL;
RETCODE (SQL_API *lpSQLFetch)(HSTMT)                                 =NULL;
RETCODE (SQL_API *lpSQLFreeConnect)(HDBC)                            =NULL;
RETCODE (SQL_API *lpSQLFreeEnv)(HENV)                                =NULL;
RETCODE (SQL_API *lpSQLFreeStmt)(HSTMT,UWORD)                        =NULL;
RETCODE (SQL_API *lpSQLGetCursorName)(HSTMT,UCHAR FAR *,SWORD,
                                      SWORD FAR *)                   =NULL;
RETCODE (SQL_API *lpSQLNumResultCols)(HSTMT,SWORD FAR *)             =NULL;
RETCODE (SQL_API *lpSQLPrepare)(HSTMT,UCHAR FAR *,SDWORD)            =NULL;
RETCODE (SQL_API *lpSQLRowCount)(HSTMT,SDWORD FAR *)                 =NULL;
RETCODE (SQL_API *lpSQLSetCursorName)(HSTMT,UCHAR FAR *,SWORD)       =NULL;
RETCODE (SQL_API *lpSQLTransact)(HENV,HDBC,UWORD)                    =NULL;


RETCODE (SQL_API *lpSQLColumns)(HSTMT,UCHAR FAR *,SWORD,UCHAR FAR *,
                                SWORD, UCHAR FAR *,SWORD,
                                UCHAR FAR *,SWORD)                   =NULL;
RETCODE (SQL_API *lpSQLDriverConnect)(HDBC,HWND,UCHAR FAR *,SWORD,
                                      UCHAR FAR *,SWORD,SWORD FAR *,
                                      UWORD)                         =NULL;
RETCODE (SQL_API *lpSQLGetConnectOption)(HDBC,UWORD,PTR)             =NULL;
RETCODE (SQL_API *lpSQLGetData)(HSTMT,UWORD,SWORD,PTR,SDWORD,
                                SDWORD FAR *)                        =NULL;
RETCODE (SQL_API *lpSQLGetFunctions)(HDBC,UWORD,UWORD FAR *)         =NULL;
RETCODE (SQL_API *lpSQLGetInfo)(HDBC,UWORD,PTR,SWORD,SWORD FAR *)    =NULL;
RETCODE (SQL_API *lpSQLGetStmtOption)(HSTMT,UWORD,PTR)               =NULL;
RETCODE (SQL_API *lpSQLGetTypeInfo)(HSTMT,SWORD)                     =NULL;
RETCODE (SQL_API *lpSQLParamData)(HSTMT,PTR FAR *)                   =NULL;
RETCODE (SQL_API *lpSQLPutData)(HSTMT,PTR,SDWORD)                    =NULL;
RETCODE (SQL_API *lpSQLSetConnectOption)(HDBC,UWORD,UDWORD)          =NULL;
RETCODE (SQL_API *lpSQLSetStmtOption)(HSTMT,UWORD,UDWORD)            =NULL;
RETCODE (SQL_API *lpSQLSpecialColumns)(HSTMT,UWORD,UCHAR FAR *,SWORD,
                                       UCHAR FAR *,SWORD,UCHAR FAR *,
                                       SWORD,UWORD,UWORD)            =NULL;
RETCODE (SQL_API *lpSQLStatistics)(HSTMT,UCHAR FAR *,SWORD,
                                   UCHAR FAR *,SWORD,UCHAR FAR *,
                                   SWORD,UWORD,UWORD)                =NULL;
RETCODE (SQL_API *lpSQLTables)(HSTMT,UCHAR FAR *,SWORD,UCHAR FAR *,
                               SWORD,UCHAR FAR *,SWORD,UCHAR FAR *,
                               SWORD)                                =NULL;
RETCODE (SQL_API *lpSQLBrowseConnect)(HDBC,UCHAR FAR *,SWORD,
                                      UCHAR FAR *,SWORD,SWORD FAR *) =NULL;
RETCODE (SQL_API *lpSQLColumnPrivileges)(HSTMT,UCHAR FAR *,SWORD,
                                         UCHAR FAR *,SWORD,
                                         UCHAR FAR *,SWORD,
                                         UCHAR FAR *,SWORD)          =NULL;
RETCODE (SQL_API *lpSQLDataSources)(HENV,UWORD,UCHAR FAR *,SWORD,
                                    SWORD FAR *,UCHAR FAR *,SWORD,
                                    SWORD FAR *)                     =NULL;
RETCODE (SQL_API *lpSQLDescribeParam)(HSTMT,UWORD,SWORD FAR *,
                                      UDWORD FAR *,SWORD FAR *,
                                      SWORD FAR *)                   =NULL;
RETCODE (SQL_API *lpSQLExtendedFetch)(HSTMT,UWORD,SDWORD,
                                      UDWORD FAR *,UWORD FAR *)      =NULL;
RETCODE (SQL_API *lpSQLForeignKeys)(HSTMT,UCHAR FAR *,SWORD,
                                    UCHAR FAR *,SWORD,UCHAR FAR *,
                                    SWORD,UCHAR FAR *,SWORD,
                                    UCHAR FAR *,SWORD,UCHAR FAR *,
                                    SWORD)                           =NULL;
RETCODE (SQL_API *lpSQLMoreResults)(HSTMT)                           =NULL;
RETCODE (SQL_API *lpSQLNativeSql)(HDBC,UCHAR FAR *,SDWORD,
                                  UCHAR FAR *,SDWORD,SDWORD FAR *)   =NULL;
RETCODE (SQL_API *lpSQLNumParams)(HSTMT,SWORD FAR *)                 =NULL;
RETCODE (SQL_API *lpSQLParamOptions)(HSTMT,UDWORD,UDWORD FAR *)      =NULL;
RETCODE (SQL_API *lpSQLPrimaryKeys)(HSTMT,UCHAR FAR *,SWORD,
                                    UCHAR FAR *,SWORD,UCHAR FAR *,
                                    SWORD)                           =NULL;
RETCODE (SQL_API *lpSQLProcedureColumns)(HSTMT,UCHAR FAR *,SWORD,
                                         UCHAR FAR *,SWORD,
                                         UCHAR FAR *,SWORD,
                                         UCHAR FAR *,SWORD)          =NULL;
RETCODE (SQL_API *lpSQLProcedures)(HSTMT,UCHAR FAR *,SWORD,
                                   UCHAR FAR *,SWORD,UCHAR FAR *,
                                   SWORD)                            =NULL;
RETCODE (SQL_API *lpSQLSetPos)(HSTMT,UWORD,UWORD,UWORD)              =NULL;
RETCODE (SQL_API *lpSQLTablePrivileges)(HSTMT,UCHAR FAR *,SWORD,
                                        UCHAR FAR *, SWORD,
                                        UCHAR FAR *,SWORD)           =NULL;


RETCODE (SQL_API *lpSQLDrivers)(HENV,UWORD,UCHAR FAR *,SWORD,
                                SWORD FAR *,UCHAR FAR *,SWORD,
                                SWORD FAR *)                         =NULL;

RETCODE (SQL_API *lpSQLBindParameter)(HSTMT,UWORD,SWORD,SWORD,SWORD,
                                      UDWORD,SWORD,PTR,SDWORD,
                                      SDWORD FAR *)                  =NULL;


RETCODE (SQL_API *lpSQLSetParam)(HSTMT,UWORD,SWORD,SWORD,UDWORD,
                                 SWORD,PTR,SDWORD FAR *)             =NULL;

RETCODE (SQL_API *lpSQLSetScrollOptions)(HSTMT,UWORD,SDWORD,UWORD)   =NULL;






BOOL (WINAPI *wdbSQLProgram)(LPCSTR)                             =NULL;
int (WINAPI *wdbQueryErrorMessage)(LPSTR, WORD)                  =NULL;
HWND (WINAPI *wdbCreateBrowseWindow)(HWDB, HWND, WORD)           =NULL;
HWDB (WINAPI *wdbOpenDatabase)(LPCSTR, DWORD)                    =NULL;
BOOL (WINAPI *wdbCloseDatabase)(HWDB hDb)                        =NULL;

char pVersionString[]=
                "\x1b[1;37;40m"
                "**  SFTShell - A Windows Command Line Interpreter  **\r\n"
                "\x1b[0;37m"
                "Copyright (c) 1991-97 by Stewart~Frazier Tools, Inc.\r\n"
                "                 all rights reserved\r\n"
                "\r\n\x1b[0;37;40m"
                "            \x1b[41m*****************************\x1b[40m\r\n"
                "            \x1b[41m**\x1b[40m                         \x1b[41m**\x1b[40m\r\n"

//// add this for DEMO versions that expire on a particular date
//                "            \x1b[41m**\x1b[1;33;40m      TRIAL VERSION      \x1b[0;37;41m**\x1b[40m\r\n"
//                "            \x1b[41m**\x1b[1;33;40m  Expires on 01/15/1996  \x1b[0;37;41m**\x1b[40m\r\n"
//                "            \x1b[41m**\x1b[40m                         \x1b[41m**\x1b[40m\r\n"

#ifdef RXSHELL
                "            \x1b[41m**\x1b[1;37;40m  RXShell - " VER_PRODUCTVER_STRING " \x1b[0;37;41m**\x1b[40m\r\n"
#else  // RXSHELL
                "            \x1b[41m**\x1b[1;37;40m SFTShell - " VER_PRODUCTVER_STRING " \x1b[0;37;41m**\x1b[40m\r\n"
#endif // RXSHELL
#ifdef DEMO
                "            \x1b[41m**\x1b[1;33;40m      TRIAL VERSION      \x1b[0;37;41m**\x1b[40m\r\n"
#endif // DEMO
//                "            \x1b[41m**\x1b[40m                         \x1b[41m**\x1b[40m\r\n"
//                "            \x1b[41m**\x1b[1;33;40m Release Date:  " VER_RELEASEDATE_STR " \x1b[0;37;41m**\x1b[40m\r\n"
//                "            \x1b[41m**\x1b[40m                         \x1b[41m**\x1b[40m\r\n"
//                "            \x1b[41m**\x1b[1;35;40m  - RELEASE CANDIDATE -  \x1b[0;37;41m**\x1b[40m\r\n"
//                "            \x1b[41m**\x1b[1;35;40m     - PRELIMINARY -     \x1b[0;37;41m**\x1b[40m\r\n"
//                "            \x1b[41m**\x1b[1;35;40m (not to be distributed) \x1b[0;37;41m**\x1b[40m\r\n"
                "            \x1b[41m**\x1b[40m                         \x1b[41m**\x1b[40m\r\n"
                "            \x1b[41m*****************************\x1b[40m\r\n"
                "\r\n\x1b[0;32m"
                "To obtain a list of valid commands, type "
                "\x1b[0;37mHELP\x1b[0;32m and\r\npress "
                "\x1b[0;37m<ENTER>.\r\n"
                "\r\n\x1b[1;37m";


                        /** EXTERNAL VARIABLES **/

extern WORD PASCAL __ahincr;

extern SBPCHAR FAR PASCAL _HELP_START_[];
extern char FAR PASCAL _HELP_COMMANDS_[];
extern char FAR PASCAL _HELP_KEYS_[];
extern char FAR PASCAL _HELP_VARIABLES_[];


/***************************************************************************/
/*                                                                         */
/*           GLOBAL STRING DEFINITIONS - Commonly used strings             */
/*                                                                         */
/***************************************************************************/


const char pCRLF[]="\r\n";
const char pCRLFLF[]="\r\n\n";
const char pQCRLF[]="\"\r\n";
const char pQCRLFLF[]="\"\r\n\n";
const char pBACKSLASH[]="\\";

const char pNOTENOUGHMEMORY[]=
            "?Not enough memory to complete operation\r\n\n";

const char pMEMORYLOW[]="?Warning - extremely low on memory...\r\n";

const char pNOMATCHINGFILES[]="?No matching files found\r\n\n";

const char pCOMSPEC[]    = "COMSPEC";
const char pCOMMANDCOM[] = "COMMAND.COM";
const char pIOSYS[]      = "IO.SYS";
const char pMSDOSSYS[]   = "MSDOS.SYS";

const char pHELPFILE[]   = "SFTSHELL.HLP";

const char pPIF[] = ".PIF";
const char pREX[] = ".REX";
const char pBAT[] = ".BAT";
const char pCOM[] = ".COM";
const char pEXE[] = ".EXE";


const char pWILDCARD[]   = "*.*";
const char pVOLUMEINDRIVE[]="Volume in drive '";
const char pHASNOLABEL[] = "' has no label - ";

const char pON[]="ON";
const char pOFF[]="OFF";
const char pILLEGALARGUMENT[]="?Illegal argument - \"";
const char pILLEGALSWITCH[]="?Illegal Switch - /";

const char pEXTRANEOUSARGSIGNORED[] = "?Extraneous arguments ignored - \"";

const char pSYNTAXERROR[]="?Syntax error - \"";

const char pERRORREADINGDRIVE[]="?Error reading specified drive\r\n\n";

const char pINVALIDPATH[]="?Invalid path - \"";
const char pINVALIDDRIVE[]="?Invalid drive specified\r\n\n";

const char pNoSysMessage[]=
            "?Unable to locate files to transfer system\r\n\n";

const char pCTRLBREAKMESSAGE[]=
"\r\nTerminated at user request by Ctrl-Break/Ctrl-C\r\n\n";

const char pCTRLC[]="^C";

const char pWINCMDERROR[] = "** SFTSHELL ERROR **";



WORD wFormatPercentComplete = 0;     // current 'percent complete' for
                                     // current disk format operation.

WORD wCopyPercentComplete = 0;       // like the above, but used for
                                     // tracking copy operations.
