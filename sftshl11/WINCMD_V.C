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

#define STATIC_VARS         /* prevents re-defining static variables */

#include "wincmd.h"         /* 'dependency' related definitions */
#include "wincmd_f.h"       /* function prototypes and variables */
#include "wincmd.ver"       /* version information */


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

char (__based(lpScreen) *screen)[SCREEN_COLS]=NULL; // array containing screen buffer
char (__based(lpScreen) *attrib)[SCREEN_COLS]=NULL; // array containing attrib bytes

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

struct DOSERROR last_error;         /* ext error information for last error */

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



#define PCMDB __based(__segname("PROCESS_COMMAND_TEXT"))

static char PCMDB szAPPEND[]    = "APPEND";
static char PCMDB szASSIGN[]    = "ASSIGN";
static char PCMDB szATTRIB[]    = "ATTRIB";
static char PCMDB szBREAK[]     = "BREAK";
static char PCMDB szCALC[]      = "CALC";
static char PCMDB szCALL[]      = "CALL";
static char PCMDB szCD[]        = "CD";
static char PCMDB szCHCP[]      = "CHCP";
static char PCMDB szCHDIR[]     = "CHDIR";
static char PCMDB szCLOSETASK[] = "CLOSETASK";
static char PCMDB szCLS[]       = "CLS";
static char PCMDB szCOMMAND[]   = "COMMAND";
static char PCMDB szCOMP[]      = "COMP";
static char PCMDB szCONTINUE[]  = "CONTINUE";
static char PCMDB szCOPY[]      = "COPY";
static char PCMDB szCT[]        = "CT";
static char PCMDB szCTTY[]      = "CTTY";
static char PCMDB szDATE[]      = "DATE";
static char PCMDB szDBLSPACE[]  = "DBLSPACE";
static char PCMDB szDDE[]       = "DDE";
static char PCMDB szDEBUG[]     = "DEBUG";
static char PCMDB szDEFINE[]    = "DEFINE";
static char PCMDB szDEL[]       = "DEL";
static char PCMDB szDELTREE[]   = "DELTREE";
static char PCMDB szDIR[]       = "DIR";
static char PCMDB szDOSKEY[]    = "DOSKEY";
static char PCMDB szECHO[]      = "ECHO";
static char PCMDB szEDIT[]      = "EDIT";
static char PCMDB szELSE[]      = "ELSE";
static char PCMDB szEND[]       = "END";
static char PCMDB szENDIF[]     = "ENDIF";
static char PCMDB szERASE[]     = "ERASE";
static char PCMDB szEXIT[]      = "EXIT";
static char PCMDB szEXPAND[]    = "EXPAND";
static char PCMDB szFC[]        = "FC";
static char PCMDB szFDISK[]     = "FDISK";
static char PCMDB szFIND[]      = "FIND";
static char PCMDB szFOR[]       = "FOR";
static char PCMDB szFORMAT[]    = "FORMAT";
static char PCMDB szGOTO[]      = "GOTO";
static char PCMDB szGRAFTABL[]  = "GRAFTABL";
static char PCMDB szGRAPHICS[]  = "GRAPHICS";
static char PCMDB szHELP[]      = "HELP";
static char PCMDB szIF[]        = "IF";
static char PCMDB szINPUT[]     = "INPUT";
static char PCMDB szJOIN[]      = "JOIN";
static char PCMDB szKEYB[]      = "KEYB";
static char PCMDB szKILLTASK[]  = "KILLTASK";
static char PCMDB szKT[]        = "KT";
static char PCMDB szLABEL[]     = "LABEL";
static char PCMDB szLET[]       = "LET";
static char PCMDB szLH[]        = "LH";
static char PCMDB szLISTOPEN[]  = "LISTOPEN";
static char PCMDB szLO[]        = "LO";
static char PCMDB szLOAD[]      = "LOAD";
static char PCMDB szLOADHIGH[]  = "LOADHIGH";
static char PCMDB szMAX[]       = "MAX";
static char PCMDB szMD[]        = "MD";
static char PCMDB szMEM[]       = "MEM";
static char PCMDB szMIN[]       = "MIN";
static char PCMDB szMKDIR[]     = "MKDIR";
static char PCMDB szMODE[]      = "MODE";
static char PCMDB szMORE[]      = "MORE";
static char PCMDB szMOVE[]      = "MOVE";
static char PCMDB szNET[]       = "NET";
static char PCMDB szNEXT[]      = "NEXT";
static char PCMDB szNLSFUNC[]   = "NLSFUNC";
static char PCMDB szNUKETASK[]  = "NUKETASK";
static char PCMDB szODBC[]      = "ODBC";
static char PCMDB szPATH[]      = "PATH";
static char PCMDB szPAUSE[]     = "PAUSE";
static char PCMDB szPLAYSOUND[] = "PLAYSOUND";
static char PCMDB szPRINT[]     = "PRINT";
static char PCMDB szPROMPT[]    = "PROMPT";
static char PCMDB szPS[]        = "PS";
static char PCMDB szRD[]        = "RD";
static char PCMDB szREM[]       = "REM";
static char PCMDB szREMOVE[]    = "REMOVE";
static char PCMDB szREN[]       = "REN";
static char PCMDB szRENAME[]    = "RENAME";
static char PCMDB szREPEAT[]    = "REPEAT";
static char PCMDB szREPLACE[]   = "REPLACE";
static char PCMDB szRETURN[]    = "RETURN";
static char PCMDB szREXX[]      = "REXX";
static char PCMDB szRMDIR[]     = "RMDIR";
static char PCMDB szSET[]       = "SET";
static char PCMDB szSHARE[]     = "SHARE";
static char PCMDB szSHIFT[]     = "SHIFT";
static char PCMDB szSORT[]      = "SORT";
static char PCMDB szSQL[]       = "SQL";
static char PCMDB szSTART[]     = "START";
static char PCMDB szSUBST[]     = "SUBST";
static char PCMDB szSYS[]       = "SYS";
static char PCMDB szTASKLIST[]  = "TASKLIST";
static char PCMDB szTIME[]      = "TIME";
static char PCMDB szTL[]        = "TL";
static char PCMDB szTREE[]      = "TREE";
static char PCMDB szTRUENAME[]  = "TRUENAME";
static char PCMDB szTYPE[]      = "TYPE";
static char PCMDB szUNDELETE[]  = "UNDELETE";
static char PCMDB szUNTIL[]     = "UNTIL";
static char PCMDB szVER[]       = "VER";
static char PCMDB szVERIFY[]    = "VERIFY";
static char PCMDB szVOL[]       = "VOL";
static char PCMDB szWAIT[]      = "WAIT";
static char PCMDB szWEND[]      = "WEND";
static char PCMDB szWHILE[]     = "WHILE";
static char PCMDB szXCOPY[]     = "XCOPY";


#define AA const char PCMDB *

const char PCMDB * const PCMDB cmd_array[] = {
 (AA)szAPPEND,  (AA)szASSIGN,  (AA)szATTRIB,  (AA)szBREAK,   (AA)szCALC,
 (AA)szCALL,    (AA)szCD,      (AA)szCHCP,    (AA)szCHDIR,   (AA)szCLOSETASK,
 (AA)szCLS,     (AA)szCOMMAND, (AA)szCOMP,
 (AA)szCONTINUE,(AA)szCOPY,    (AA)szCT,      (AA)szCTTY,    (AA)szDATE,    (AA)szDBLSPACE,
 (AA)szDDE,     (AA)szDEBUG,   (AA)szDEFINE,  (AA)szDEL,     (AA)szDELTREE, (AA)szDIR,
 (AA)szDOSKEY,  (AA)szECHO,    (AA)szEDIT,    (AA)szELSE,    (AA)szEND,     (AA)szENDIF,
 (AA)szERASE,   (AA)szEXIT,    (AA)szEXPAND,  (AA)szFC,      (AA)szFIND,    (AA)szFOR,
 (AA)szFORMAT,  (AA)szGOTO,    (AA)szGRAFTABL,(AA)szGRAPHICS,(AA)szHELP,    (AA)szIF,
 (AA)szINPUT,   (AA)szJOIN,    (AA)szKEYB,    (AA)szKILLTASK,(AA)szKT,      (AA)szLABEL,
 (AA)szLET,     (AA)szLH,      (AA)szLISTOPEN,(AA)szLO,      (AA)szLOAD,    (AA)szLOADHIGH,
 (AA)szMAX,     (AA)szMD,      (AA)szMEM,     (AA)szMIN,
 (AA)szMKDIR,   (AA)szMODE,    (AA)szMORE,    (AA)szMOVE,    (AA)szNET,     (AA)szNEXT,    
 (AA)szNLSFUNC ,(AA)szNUKETASK,(AA)szODBC,    (AA)szPATH,    (AA)szPAUSE,   (AA)szPLAYSOUND,
 (AA)szPRINT,   (AA)szPROMPT,  (AA)szPS,      (AA)szRD,      (AA)szREM,     (AA)szREMOVE,
 (AA)szREN,     (AA)szRENAME,  (AA)szREPEAT,  (AA)szREPLACE, (AA)szRETURN,  (AA)szREXX,
 (AA)szRMDIR,   (AA)szSET,     (AA)szSHARE,   (AA)szSHIFT,   (AA)szSORT,    (AA)szSQL,
 (AA)szSTART,   (AA)szSUBST,   (AA)szSYS,     (AA)szTASKLIST,(AA)szTIME,    (AA)szTL,
 (AA)szTREE,    (AA)szTRUENAME,(AA)szTYPE,    (AA)szUNDELETE,(AA)szUNTIL,   (AA)szVER,
 (AA)szVERIFY,  (AA)szVOL,     (AA)szWAIT,    (AA)szWEND,    (AA)szWHILE,   (AA)szXCOPY
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

BOOL (FAR PASCAL *lpGetOpenFileName)(OPENFILENAME FAR *lpofn)        =NULL;
BOOL (FAR PASCAL *lpGetSaveFileName)(OPENFILENAME FAR *lpofn)        =NULL;
BOOL (FAR PASCAL *lpChooseFont)(CHOOSEFONT FAR *lpcf)                =NULL;
DWORD (FAR PASCAL *lpCommDlgExtendedError)(void)                     =NULL;

BOOL (FAR PASCAL *lpGlobalFirst)(LPGLOBALENTRY lpGlobal, WORD wFlags)=NULL;
BOOL (FAR PASCAL *lpGlobalNext)(LPGLOBALENTRY lpGlobal, WORD wFlags) =NULL;
BOOL (FAR PASCAL *lpMemManInfo)(LPMEMMANINFO lpInfo)                 =NULL;
BOOL (FAR PASCAL *lpModuleFirst)(LPMODULEENTRY lpModule)             =NULL;
BOOL (FAR PASCAL *lpModuleNext)(LPMODULEENTRY lpModule)              =NULL;
HTASK (FAR PASCAL *lpTaskFindHandle)(LPTASKENTRY lpTask, HTASK hTask)=NULL;
HMODULE (FAR PASCAL *lpModuleFindHandle)(LPMODULEENTRY lpModule,
                                         HMODULE hModule)            =NULL;
BOOL (FAR PASCAL *lpNotifyRegister)(HTASK, LPFNNOTIFYCALLBACK, WORD) =NULL;
BOOL (FAR PASCAL *lpNotifyUnRegister)(HTASK)                         =NULL;

BOOL (FAR PASCAL *lpIsTask)(HTASK hTask)                             =NULL;
BOOL (FAR PASCAL *lpTaskFirst)(LPTASKENTRY lpTask)                   =NULL;
BOOL (FAR PASCAL *lpTaskNext)(LPTASKENTRY lpTask)                    =NULL;

DWORD (FAR PASCAL *lpGlobalMasterHandle)(void)                       =NULL;
DWORD (FAR PASCAL *lpGlobalHandleNoRIP)(WORD wSeg)                   =NULL;

BOOL (WINAPI *lpsndPlaySound)(LPCSTR lpszSoundName, WORD wFlags)     =NULL;



// WOW Generic Thunk procs (WOWNT16.H)

DWORD hKernel32  = 0L;    // 32-bit module (Generic Thunk)
DWORD hWOW32     = 0L;    // 32-bit module (Generic Thunk)
DWORD hMPR       = 0L;    // 32-bit module (Generic Thunk)
DWORD hSFTSH32T  = 0L;    // 32-bit module (Generic Thunk)


DWORD (FAR PASCAL *lpGetVDMPointer32W)(LPVOID vp, UINT fMode)        = NULL;
DWORD (FAR PASCAL *lpLoadLibraryEx32W)(LPCSTR lpszLibFile,
                                       DWORD hFile, DWORD dwFlags)   = NULL;
DWORD (FAR PASCAL *lpGetProcAddress32W)(DWORD hModule,
                                        LPCSTR lpszProc)             = NULL;
DWORD (FAR PASCAL *lpFreeLibrary32W)(DWORD hLibModule)               = NULL;

DWORD (FAR __cdecl *lpCallProcEx32W)(DWORD dwCount, DWORD lpCvt,
                                     DWORD lpProc, ... )             = NULL;

#define CPEX_DEST_STDCALL   0x00000000L  /* 'or' with 'dwCount' parm */
#define CPEX_DEST_CDECL     0x80000000L  /* 'or' with 'dwCount' parm */



// 16-bit WNET stuff (from USER.EXE)

UINT (FAR PASCAL *lpWNetAddConnection)(LPSTR, LPSTR, LPSTR)          = NULL;
UINT (FAR PASCAL *lpWNetCancelConnection)(LPSTR, BOOL)               = NULL;
UINT (FAR PASCAL *lpWNetGetConnection)(LPSTR, LPSTR, UINT FAR *)     = NULL;



// REGISTRY


LONG (FAR PASCAL *lpRegCloseKey)(HKEY)                               =NULL;
LONG (FAR PASCAL *lpRegCreateKey)(HKEY, LPCSTR, HKEY FAR *)          =NULL;
LONG (FAR PASCAL *lpRegDeleteKey)(HKEY, LPCSTR)                      =NULL;
LONG (FAR PASCAL *lpRegEnumKey)(HKEY, DWORD, LPSTR, DWORD)           =NULL;
LONG (FAR PASCAL *lpRegOpenKey)(HKEY, LPCSTR, HKEY FAR *)            =NULL;
LONG (FAR PASCAL *lpRegQueryValue)(HKEY, LPSTR, LPSTR, LONG FAR *)   =NULL;
LONG (FAR PASCAL *lpRegSetValue)(HKEY, LPCSTR, DWORD, LPCSTR, DWORD) =NULL;


void (FAR PASCAL *lpDragAcceptFiles)(HWND, BOOL)                     =NULL;
void (FAR PASCAL *lpDragFinish)(HANDLE)                              =NULL;
WORD (FAR PASCAL *lpDragQueryFile)(HANDLE, WORD, LPSTR, WORD)        =NULL;

HINSTANCE (FAR PASCAL *lpShellExecute)(HWND, LPCSTR, LPCSTR, LPCSTR,
                                       LPCSTR, int)                  =NULL;


// Windows '95 SHELL functions (actually located in KERNEL)

LONG (FAR PASCAL *lpRegDeleteValue)(HKEY, LPCSTR)                    =NULL;
LONG (FAR PASCAL *lpRegQueryValueEx)(HKEY, LPCSTR, LONG FAR *,
                                     LONG FAR *, LPBYTE, LONG FAR *) =NULL;
LONG (FAR PASCAL *lpRegEnumValue)(HKEY, DWORD, LPCSTR,
                                  LONG FAR *, DWORD, LONG FAR *,
                                  LPBYTE, LONG FAR *)                =NULL;
LONG (FAR PASCAL *lpRegSetValueEx)(HKEY, LPCSTR, DWORD, DWORD,
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






BOOL (FAR PASCAL *wdbSQLProgram)(LPCSTR)                             =NULL;
int (FAR PASCAL *wdbQueryErrorMessage)(LPSTR, WORD)                  =NULL;
HWND (FAR PASCAL *wdbCreateBrowseWindow)(HWDB, HWND, WORD)           =NULL;
HWDB (FAR PASCAL *wdbOpenDatabase)(LPCSTR, DWORD)                    =NULL;
BOOL (FAR PASCAL *wdbCloseDatabase)(HWDB hDb)                        =NULL;

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
