/***************************************************************************/
/*                                                                         */
/*   WINCMD.C - Command line interpreter for Microsoft(r) Windows (tm)     */
/*                                                                         */
/*                      TRADE SECRET - CONFIDENTIAL                        */
/*                                                                         */
/*             The compiled libraries and API header files are             */
/*          (c) Copyright 1990-1995 by Stewart~Frazier Tools, Inc          */
/*                                                                         */
/*        Main functions, message loop, background stuff, etc. etc.        */
/*                                                                         */
/***************************************************************************/

// DEFINE 'DROP_DEAD_DATE' for evaluation versions!!!!!

//#define DROP_DEAD_DATE 35078
#define DROP_DEAD_WARNING_PERIOD 15


#define THE_WORKS
#include "mywin.h"

#include "mth.h"            /* stand-alone multi-thread functions */
#include "wincmd.h"         /* 'dependency' related definitions */
#include "wincmd_f.h"       /* function prototypes and variables */

#ifndef WIN32
#if defined(M_I86LM)

  #pragma message("** Large Model In Use - Single Instance Only!! **")

#elif !defined(M_I86MM)

  #error  "** Must use MEDIUM model to compile, silly!! **"

#else

  #pragma message("** Medium Model Compile - Multiple Instances OK **")

#endif
#endif // WIN32



#define COMMAND_STACK_SIZE 16384     /* this BETTER be big enough! */


#define DEFAULT_FONT (GetStockObject(OEM_FIXED_FONT))


#define StatusBarFontHeight() ( min(                                        \
       min((UINT)((tmMain.tmHeight - tmMain.tmInternalLeading) * 1.05 + 1), \
           (UINT)tmMain.tmHeight), (UINT)HIWORD(dwDialogUnits) ) )






                      /***************************/
                      /*** FUNCTION PROTOTYPES ***/
                      /***************************/

                    /** Main Loop and Startup Code **/

int PASCAL WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,
                   int nCmdShow);


BOOL InitApplication(HINSTANCE hInstance);

BOOL InitInstance(HANDLE hInstance, int nCmdShow, BOOL ForceExternalLibrary);

void FAR PASCAL UpdateWindowStateFlag(void);

#ifndef WIN32
void _interrupt FAR MyCtrlBreak(WORD _es, WORD _ds, WORD _di,
                                WORD _si, WORD _bp, WORD _sp,
                                WORD _bx, WORD _dx, WORD _cx,
                                WORD _ax, WORD _ip, WORD _cs,
                                WORD _flags );
#endif // WIN32


                   /** WINDOW PROCESSING FUNCTIONS **/

LRESULT CALLBACK EXPORT MainWndProc(HWND hWnd, UINT message, WPARAM wParam,
                                    LPARAM lParam);

LRESULT CALLBACK EXPORT ToolBarHintProc(HWND hWnd,UINT message, WPARAM wParam,
                                        LPARAM lParam);

void FAR PASCAL SubclassStaticControls(HWND hDlg);
void FAR PASCAL SubclassEditControls(HWND hDlg);

HBRUSH FAR PASCAL DlgOn_WMCtlColor(HWND hWnd, WPARAM wParam, LPARAM lParam);

BOOL CALLBACK EXPORT About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

BOOL CALLBACK EXPORT AssociateDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

BOOL CALLBACK EXPORT FolderDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

BOOL CALLBACK EXPORT ScreenLinesDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
                                        LPARAM lParam);

BOOL CALLBACK EXPORT QueueDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
                                  LPARAM lParam);

BOOL CALLBACK EXPORT InstantBatchDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
                                         LPARAM lParam);

BOOL CALLBACK EXPORT ExtErrorDlg(HWND hDlg, UINT message, WPARAM wParam,
                          LPARAM lParam);

BOOL CALLBACK EXPORT EditCommandDlg(HWND hDlg, UINT message, WPARAM wParam,
                             LPARAM lParam);

void FAR PASCAL EnableToolBarButton(WORD wID, BOOL bEnable);

LRESULT NEAR PASCAL CheckToolBarCursor(HWND hWnd, UINT msg, WPARAM wParam,
                                       LPARAM lParam);

void FAR PASCAL PaintToolBarButton(HWND hWnd, UINT uiButtonIndex);
void FAR PASCAL PaintStatusBarPane(HWND hWnd, UINT uiPaneIndex);

HFONT NEAR PASCAL CreateStatusBarFont(void);

LRESULT NEAR _fastcall MainOnNCPaint(HWND hWnd, WPARAM wParam, LPARAM lParam);

VOID NEAR _fastcall MainOnCommand(HWND hWnd, int id, HWND hwndCtl,
                                  UINT wCodeNotify);

VOID NEAR _fastcall MainOnSysCommand(HWND hWnd, int cmd, int x, int y);

VOID FAR PASCAL     MainOnChar(HWND hWnd, UINT ch, int cRepeat);

VOID NEAR _fastcall MainOnPaint(HWND hWnd);

BOOL NEAR _fastcall MainCheckScrollBars(HWND hWnd);

VOID NEAR _fastcall MainOnVScroll(HWND hWnd, HWND hwndCtl, UINT wCode,
                                  int pos);

VOID NEAR _fastcall MainOnHScroll(HWND hWnd, HWND hwndCtl, UINT wCode,
                                  int pos);

void FAR PASCAL InvalidateStatusBar(HWND hWnd);

void FAR PASCAL InvalidateToolBar(HWND hWnd);

void FAR PASCAL UpdateStatusBar(void);

LONG FAR PASCAL CommandThread(HANDLE hCaller, HANDLE hParms, UINT wFlags);
LONG FAR PASCAL CopyThread(HANDLE hCaller, HANDLE hParms, UINT wFlags);
LONG FAR PASCAL BatchThread(HANDLE hCaller, HANDLE hParms, UINT wFlags);
LONG FAR PASCAL WaitThread(HANDLE hCaller, HANDLE hParms, UINT wFlags);
LONG FAR PASCAL FlashThread(HANDLE hCaller, HANDLE hParms, UINT wFlags);
LONG FAR PASCAL FormatThread(HANDLE hCaller, HANDLE hParms, UINT wFlags);
LONG FAR PASCAL MemoryBatchFileThreadProc(HANDLE hCaller, HANDLE hParms, UINT wFlags);



                        /**********************/
                        /** STATIC VARIABLES **/
                        /**********************/

LPSTR lpCmdLine;

LPFORMAT_REQUEST lpFormatRequest=NULL;
BOOL format_complete_flag = FALSE;

BOOL FormatSuspended=FALSE;
BOOL CopySuspended=FALSE;

BOOL FormatCancelFlag=FALSE;   // when TRUE, current item is "canceled"
BOOL CopyCancelFlag=FALSE;     // when TRUE, current item is "canceled"


BOOL CommandLineExit = FALSE;  // when TRUE, forces SFTShell to exit
                               // following execution of the command


static BOOL ShowStatusBar=TRUE;
static BOOL ShowToolBar=TRUE;

static HBITMAP hbmpToolBar=NULL;

volatile DWORD TotalCopiesSubmitted = 0L;


// static string constants containing environment variable names & values

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


// other stuff (local only)

HFONT hMainFont=NULL;

static char szFontName[MAX_PATH]={0};

static char pFONTHEIGHT[]="FontHeight";
static char pFONTWIDTH[]="FontWidth";
static char pFONTNAME[]="FontName";
static char pFONTWEIGHT[]="FontWeight";
static char pFONTITALIC[]="FontItalic";
static char pFONTUNDERLINE[]="FontUnderline";
static char pFONTSTRIKEOUT[]="FontStrikeout";

static char pSHOWSTATUSBAR[]="ShowStatusBar";
static char pSHOWTOOLBAR[]="ShowToolBar";

static char pSCREENLINES[]="ScreenLines";

static BOOL BlockFontChangeMsg = FALSE;

static OFSTRUCT TempOfstr;

static HCURSOR hMainCursor = NULL;

static HBRUSH hbrDlgBackground = NULL;



// TOOLBAR BUTTONS (status, layout, etc.)

#define TOOLBAR_TIMER 0x00c0        // for now, the timer ID for toolbar hint

static int pButtonBits[]={1,2,4,8,
                          0x10,0x20,0x40,0x80,
                          0x100,0x200,0x400,0x800,
                          0x1000,0x2000,0x4000,0x8000};

static int pButtons[]={0, 16, 32, 52, 68, 84, 100, 116, 136, 152, 168};
static int pButtonID[]={
           IDM_OPEN_MENU,    IDM_FOLDER_MENU,  IDM_PRINT_MENU,
           IDM_COPY_ALL,     IDM_COPY_CMDLINE, IDM_PASTE_MENU,
           IDM_ERASEHISTORY, IDM_INSTANT_BATCH,
           IDM_FONT_MENU,    IDM_QUEUE_MENU,   IDM_HELP_CALCULATION};

static const char * const szButtonHint[]={
           "Open File",            "Change Path/Drive",    "Print File",
           "Copy Window Text",     "Copy Command History", "Paste Commands",
           "Erase Command History","Instant Batch Program",
           "Assign Custom Font",   "Manage Queues",        "Help Contents" };

static int iButtonEnabled=0xffff; // each bit set for appropriate button
static int iButtonPressed=0;      // each bit set for appropriate button

static int iToolBarTimer = 0;
static const char szToolBarHint[]="TOOLBARHINTCLASS";
static HWND hwndToolBarHint = NULL;


static WNDPROC lpStaticWndProc=NULL;
static WNDPROC lpEditWndProc=NULL;

HWND hdlgQueue=NULL;  // QUEUE dialog (this one is MODELESS on purpose)


HMODULE hModule=NULL; // this is actually GLOBAL in scope, though I declare it
                      // here.  Elsewhere I must use 'extern' in source file.



DWORD dwDialogUnits;



/***************************************************************************/
/*                                                                         */
/*           Windows Initialization and Message Loop Functions             */
/*                                                                         */
/***************************************************************************/

#ifdef WIN32
#define LockCode()
#define UnlockCode()
#else  // WIN32
#define LockCode() { _asm push cs _asm call LockSegment }
#define UnlockCode() { _asm push cs _asm call UnlockSegment }
#endif // WIN32

#pragma code_seg("WINMAIN_TEXT","CODE")

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR lpCmdLn,
                   int nCmdShow)
{
MSG msg;
BOOL ForceExternalLibrary;
BOOL bDisableNT = FALSE, bDisable95 = FALSE;
extern void FAR *lpEnvVarEntry;

//typedef void (_interrupt _far *LPCBRK)();
/*  LPCBRK old_cbrk, new_cbrk;  */


   if(MthreadInit(hInstance, hPrev, lpCmdLn, nCmdShow))
   {
     return(-1);
   }

   lpCmdLine = (LPSTR)LocalLock(LocalAlloc(LMEM_FIXED, lstrlen(lpCmdLn)+1));    /* the static var gets the data */
   if(!lpCmdLine)
   {
     MessageBox(NULL, "Not enough local memory to comlete operation",
                "SFTShell Initialization", MB_OK | MB_ICONHAND);

     return(-1);
   }

   lstrcpy(lpCmdLine, lpCmdLn);

   if(!SetMessageQueue(128) && !SetMessageQueue(64) &&
      !SetMessageQueue(32) && !SetMessageQueue(16))
   {
    static const char CODE_BASED pMsg[]=
                                    "?Not enough space for message queue";

      LockCode();

      MessageBox(NULL, pMsg, pWINCMDERROR,
                 MB_OK | MB_ICONHAND | MB_SYSTEMMODAL);

      UnlockCode();

      return(1);
   }

// MODIFIED 9/1/97 - now using MLIBCEWM.DLL like MTHREAD.DLL stuff...

//   if(MthreadInit(hInstance, hPrev, lpCmdLine, nCmdShow)) return(1);

   cmd_buf_len = 0;
   memset(cmd_buf, 0, sizeof(cmd_buf));
   ForceExternalLibrary = FALSE;

   if(*lpCmdLine)
   {
    LPSTR lp1;

      lp1 = lpCmdLine;
      while(*lp1 && *lp1<=' ') lp1++;

      while(*lp1 == '/')
      {
         if(!lp1[1] || (lp1[2] > ' ' && lp1[2] != '/' && lp1[2] != ':'))
         {
            break;  // valid switch is 1 char followed by '/' or ':' or wht space
         }

         lp1++;

         if(toupper(*lp1)=='X')      // '/X' for external DLL's
         {
            ForceExternalLibrary = TRUE;
         }
         else if(toupper(*lp1)=='H') // '/H' for hidden window
         {
            nCmdShow = SW_HIDE;
         }
         else if(toupper(*lp1)=='C') // '/C' for EXIT after command executes
         {
            CommandLineExit = TRUE;  // forces exit, like 'COMMAND /C'
         }
         else if(toupper(*lp1)=='D') // '/D' for 'Disable'
         {
            if(lp1[1] == ':' && lp1[2] > ' ')
            {
               LPSTR lp1a = (lp1 += 2);

               while(*lp1a > ' ')
                  lp1a++;

               if(!_fstrnicmp("NT", lp1, lp1a - lp1))
               {
                  bDisableNT = TRUE;
               }
               else if(!_fstrnicmp("95", lp1, lp1a - lp1))
               {
                  bDisable95 = TRUE;
               }
               else
               {
                  // unknown option - just ignore it
               }

               lp1 = lp1a - 1;  // the loop will increment it again
            }
            else
            {
               lp1--;
               break;  // invalid switch - pass as command line
            }
         }
         else
         {
            lp1--;  // invalid switch - pass it as a command line!!

            break;
         }

         lp1++;
         while(*lp1 && *lp1<=' ') lp1++;
      }

      if(lp1 > lpCmdLine) lstrcpy(lpCmdLine, lp1);
                                  // clean up leading spaces, switches, etc.

      if(*lpCmdLine)
      {
         lpCurCmd = lpCmdLine;
      }
      else
      {
         lpCurCmd = NULL;
      }
   }

   if(bDisableNT || bDisable95)
   {
      MessageBox(NULL, "Disabling either Win 95 or Win NT", "** SFTSHELL DEBUG **",
                 MB_OK | MB_ICONHAND);
   }


   SetErrorMode(1);       /* ensure we don't get an 'insert diskette' box */

   if(!hPrev && !InitApplication(hInstance))
   {
      return(FALSE);
   }

   if(!InitInstance(hInstance, nCmdShow, ForceExternalLibrary))
   {
      return(FALSE);
   }




   if(bDisableNT)
   {
      IsNT = FALSE;
   }
   if(bDisable95)
   {
      IsChicago = FALSE;
   }




          /*** INSTALL A CONTROL-BREAK INTERRUPT 23H HANDLER ***/

   ctrl_break = FALSE;

//    old_cbrk = _dos_getvect(0x23);       // ctrl-break interrupt
//    new_cbrk = (LPCBRK)MakeProcInstance((FARPROC)MyCtrlBreak, hInstance);
//
//    _dos_setvect(0x23, new_cbrk);
//

   if(SpawnThreadX(CommandThread, NULL, THREADSTYLE_DEFAULT,
                    NULL,COMMAND_STACK_SIZE)==-1 ||
      SpawnThread(CopyThread, NULL, THREADSTYLE_DEFAULT)==-1 ||
      SpawnThread(FormatThread, NULL, THREADSTYLE_DEFAULT)==-1 ||
      SpawnThreadX(BatchThread, NULL, THREADSTYLE_DEFAULT,
                    NULL,COMMAND_STACK_SIZE)==-1 ||
      SpawnThread(FlashThread, NULL, THREADSTYLE_DEFAULT)==-1)
   {
      MthreadKillThread(hCommandThread);
      MthreadKillThread(hCopyThread);
      MthreadKillThread(hFormatThread);
      MthreadKillThread(hBatchThread);
      MthreadKillThread(hFlashThread);
      return(2);                /* could not spawn anything */
   }


   if((bDisable95 || bDisableNT) && (IsNT || IsChicago))
   {
      MessageBox(NULL, "Failure to disable either Win 95 or Win NT", "** SFTSHELL DEBUG **",
                 MB_OK | MB_ICONHAND);
   }


   if(bDisableNT)
      PrintString("\r\nWindows NT Features are disabled\r\n\r\n");

   if(bDisable95)
      PrintString("\r\nWindows '95 Features are disabled\r\n\r\n");

   if(bDisableNT || bDisable95)
   {
      DisplayPrompt();
   }


   while(MthreadMsgDispatch(&msg))
     ;  /* the message loop! */

/*    _dos_setvect(0x23, old_cbrk);   /* restore old INT 23H handler */
/*    FreeProcInstance((FARPROC)new_cbrk); */


   if(hMainWnd && IsWindow(hMainWnd)) DestroyWindow(hMainWnd);

   if(hMainFont)
   {
      DeleteObject(hMainFont);
   }
//   if(hFontLib)
//   {
//      if(RemoveFontResource(MAKELP(0,hFontLib)))
//      {
//         SendMessage((HWND)-1, WM_FONTCHANGE, 0, 0);
//      }
//
//      FreeLibrary(hFontLib);
//   }


   // See if there are ANY other instances of 'SFTSHELL' running...

   if(GetModuleUsage(GetModuleHandle("SFTSHELL")) <= 1 &&
      GetModuleHandle("SFTSHFON"))
   {
      GetModuleFileName(GetModuleHandle("SFTSHFON"), szFontName,
                        sizeof(szFontName));

      if(*szFontName)
      {
         // remove the READ ONLY attribute on font file before deleting it

         MySetFileAttr(szFontName, 0);

         RemoveFontResource(szFontName);

         SendMessage((HWND)-1, WM_FONTCHANGE, 0, 0);

         if(!GetModuleHandle("SFTSHFON"))
         {
            MyUnlink(szFontName);
         }
      }
   }



   if(lpMyEnv)     /* I assigned an environment string!! */
   {
//
// ** I really *CANNOT* remember just *WHY* I did this.... **
//
//       GlobalPageUnlock((HGLOBAL)((WORD)((_segment)lpMyEnv)));  /* page unlock it */
//       GlobalReAllocPtr(lpMyEnv, 0, GMEM_MODIFY | GMEM_MOVEABLE);
//
      GlobalFreePtr(lpMyEnv);  // what I have here is the *normal* method!
   }

   if(lpEnvVarEntry) GlobalFreePtr(lpEnvVarEntry);


   if(idDDEInst)
   {
      if(lpDdeUninitialize) lpDdeUninitialize(idDDEInst);

      if(lpDdeCallBack)     FreeProcInstance((FARPROC)lpDdeCallBack);

      if(lpHotLinkItems)    GlobalFreePtr(lpHotLinkItems);
   }


   RexxUnattach();  // unhooks things if I have WREXX.DLL loaded...

   CMDOdbcDisconnectAll();  // if I haven't disconnected ODBC stuff yet,
                            // THIS will!

#ifndef WIN32

   if(hToolHelp)   FreeLibrary(hToolHelp);
   if(hCommDlg)    FreeLibrary(hCommDlg);
   if(hDdeml)      FreeLibrary(hDdeml);
   if(hMMSystem)   FreeLibrary(hMMSystem);
   if(hShell)      FreeLibrary(hShell);

   if(lpFreeLibrary32W && hKernel32)
   {
      lpFreeLibrary32W(hKernel32);
   }
   if(lpFreeLibrary32W && hWOW32)
   {
      lpFreeLibrary32W(hWOW32);
   }
   if(lpFreeLibrary32W && hMPR)
   {
      lpFreeLibrary32W(hMPR);
   }
   if(lpFreeLibrary32W && hSFTSH32T)
   {
      lpFreeLibrary32W(hSFTSH32T);
   }



#endif // WIN32

   if(hODBC)       FreeLibrary(hODBC);

#ifndef WIN32

   if(hWDBUTIL)    FreeLibrary(hWDBUTIL);

#endif // WIN32

   if(hbmpToolBar)      DeleteObject(hbmpToolBar);
   if(hbrDlgBackground) DeleteObject(hbrDlgBackground);


// MODIFIED 9/1/97 - now using MLIBCEWM.DLL like MTHREAD.DLL stuff...

//   MthreadExit(GetCurrentTask());


   return(0);                   // byby - buy bonds!
}





/***************************************************************************/
/*                                                                         */
/*            INITIALIZATION CODE - GOES IN ITS OWN SEGMENT!               */
/*                                                                         */
/***************************************************************************/


#pragma code_seg("INIT_STUFF_TEXT","CODE")

#ifdef WIN32
#pragma data_seg("INIT_STUFF_DATA","RDATA")
#define INIT_BASED
#else  // WIN32
#define INIT_BASED __based(__segname("INIT_STUFF_TEXT"))
#endif // WIN32

static const char INIT_BASED szGetOpenFileName[]       = "GetOpenFileName";
static const char INIT_BASED szGetSaveFileName[]       = "GetSaveFileName";
static const char INIT_BASED szChooseFont[]            = "ChooseFont";
static const char INIT_BASED szCommDlgExtendedError[]  = "CommDlgExtendedError";

static const char INIT_BASED szIsTask[]                = "IsTask";

static const char INIT_BASED szGlobalFirst[]           = "GlobalFirst";
static const char INIT_BASED szGlobalNext[]            = "GlobalNext";
static const char INIT_BASED szMemManInfo[]            = "MemManInfo";
static const char INIT_BASED szModuleFirst[]           = "ModuleFirst";
static const char INIT_BASED szModuleNext[]            = "ModuleNext";
static const char INIT_BASED szModuleFindHandle[]      = "ModuleFindHandle";
static const char INIT_BASED szNotifyRegister[]        = "NotifyRegister";
static const char INIT_BASED szNotifyUnRegister[]      = "NotifyUnRegister";
static const char INIT_BASED szTaskFirst[]             = "TaskFirst";
static const char INIT_BASED szTaskNext[]              = "TaskNext";
static const char INIT_BASED szTaskFindHandle[]        = "TaskFindHandle";

static const char INIT_BASED szGlobalMasterHandle[]    = "GlobalMasterHandle";
static const char INIT_BASED szGlobalHandleNoRIP[]     = "GlobalHandleNoRIP";

static const char INIT_BASED szsndPlaySound[]          = "sndPlaySound";


static const char INIT_BASED szGetVDMPointer32W[]      = "GetVDMPointer32W";
static const char INIT_BASED szLoadLibraryEx32W[]      = "LoadLibraryEx32W";
static const char INIT_BASED szGetProcAddress32W[]     = "GetProcAddress32W";
static const char INIT_BASED szFreeLibrary32W[]        = "FreeLibrary32W";
static const char INIT_BASED szCallProcEx32W[]         = "_CallProcEx32W";


static const char INIT_BASED szWNetAddConnection[]     = "WNetAddConnection";
static const char INIT_BASED szWNetCancelConnection[]  = "WNetCancelConnection";
static const char INIT_BASED szWNetGetConnection[]     = "WNetGetConnection";



static const char INIT_BASED szRegCloseKey[]           = "RegCloseKey";
static const char INIT_BASED szRegCreateKey[]          = "RegCreateKey";
static const char INIT_BASED szRegDeleteKey[]          = "RegDeleteKey";
static const char INIT_BASED szRegEnumKey[]            = "RegEnumKey";
static const char INIT_BASED szRegOpenKey[]            = "RegOpenKey";
static const char INIT_BASED szRegQueryValue[]         = "RegQueryValue";
static const char INIT_BASED szRegSetValue[]           = "RegSetValue";

static const char INIT_BASED szDragAcceptFiles[]       = "DragAcceptFiles";
static const char INIT_BASED szDragFinish[]            = "DragFinish";
static const char INIT_BASED szDragQueryFile[]         = "DragQueryFile";
static const char INIT_BASED szShellExecute[]          = "ShellExecute";

static const char INIT_BASED szDdeAbandonTransaction[] = "DdeAbandonTransaction";
static const char INIT_BASED szDdeAccessData[]         = "DdeAccessData";
static const char INIT_BASED szDdeAddData[]            = "DdeAddData";
static const char INIT_BASED szDdeClientTransaction[]  = "DdeClientTransaction";
static const char INIT_BASED szDdeCmpStringHandles[]   = "DdeCmpStringHandles";
static const char INIT_BASED szDdeConnect[]            = "DdeConnect";
static const char INIT_BASED szDdeConnectList[]        = "DdeConnectList";
static const char INIT_BASED szDdeCreateDataHandle[]   = "DdeCreateDataHandle";
static const char INIT_BASED szDdeCreateStringHandle[] = "DdeCreateStringHandle";
static const char INIT_BASED szDdeDisconnect[]         = "DdeDisconnect";
static const char INIT_BASED szDdeDisconnectList[]     = "DdeDisconnectList";
static const char INIT_BASED szDdeEnableCallback[]     = "DdeEnableCallback";
static const char INIT_BASED szDdeFreeDataHandle[]     = "DdeFreeDataHandle";
static const char INIT_BASED szDdeFreeStringHandle[]   = "DdeFreeStringHandle";
static const char INIT_BASED szDdeGetData[]            = "DdeGetData";
static const char INIT_BASED szDdeGetLastError[]       = "DdeGetLastError";
static const char INIT_BASED szDdeInitialize[]         = "DdeInitialize";
static const char INIT_BASED szDdeKeepStringHandle[]   = "DdeKeepStringHandle";
static const char INIT_BASED szDdeNameService[]        = "DdeNameService";
static const char INIT_BASED szDdePostAdvise[]         = "DdePostAdvise";
static const char INIT_BASED szDdeQueryConvInfo[]      = "DdeQueryConvInfo";
static const char INIT_BASED szDdeQueryNextServer[]    = "DdeQueryNextServer";
static const char INIT_BASED szDdeQueryString[]        = "DdeQueryString";
static const char INIT_BASED szDdeReconnect[]          = "DdeReconnect";
static const char INIT_BASED szDdeSetUserHandle[]      = "DdeSetUserHandle";
static const char INIT_BASED szDdeUnaccessData[]       = "DdeUnaccessData";
static const char INIT_BASED szDdeUninitialize[]       = "DdeUninitialize";

//static const char INIT_BASED szSQLAllocConnect[]       = "SQLAllocConnect";
//static const char INIT_BASED szSQLAllocEnv[]           = "SQLAllocEnv";
//static const char INIT_BASED szSQLAllocStmt[]          = "SQLAllocStmt";
//static const char INIT_BASED szSQLBindCol[]            = "SQLBindCol";
//static const char INIT_BASED szSQLCancel[]             = "SQLCancel";
//static const char INIT_BASED szSQLColAttributes[]      = "SQLColAttributes";
//static const char INIT_BASED szSQLConnect[]            = "SQLConnect";
//static const char INIT_BASED szSQLDescribeCol[]        = "SQLDescribeCol";
//static const char INIT_BASED szSQLDisconnect[]         = "SQLDisconnect";
//static const char INIT_BASED szSQLError[]              = "SQLError";
//static const char INIT_BASED szSQLExecDirect[]         = "SQLExecDirect";
//static const char INIT_BASED szSQLExecute[]            = "SQLExecute";
//static const char INIT_BASED szSQLFetch[]              = "SQLFetch";
//static const char INIT_BASED szSQLFreeConnect[]        = "SQLFreeConnect";
//static const char INIT_BASED szSQLFreeEnv[]            = "SQLFreeEnv";
//static const char INIT_BASED szSQLFreeStmt[]           = "SQLFreeStmt";
//static const char INIT_BASED szSQLGetCursorName[]      = "SQLGetCursorName";
//static const char INIT_BASED szSQLNumResultCols[]      = "SQLNumResultCols";
//static const char INIT_BASED szSQLPrepare[]            = "SQLPrepare";
//static const char INIT_BASED szSQLRowCount[]           = "SQLRowCount";
//static const char INIT_BASED szSQLSetCursorName[]      = "SQLSetCursorName";
//static const char INIT_BASED szSQLTransact[]           = "SQLTransact";
//
//static const char INIT_BASED szSQLColumns[]            = "SQLColumns";
//static const char INIT_BASED szSQLDriverConnect[]      = "SQLDriverConnect";
//static const char INIT_BASED szSQLGetConnectOption[]   = "SQLGetConnectOption";
//static const char INIT_BASED szSQLGetData[]            = "SQLGetData";
//static const char INIT_BASED szSQLGetFunctions[]       = "SQLGetFunctions";
//static const char INIT_BASED szSQLGetInfo[]            = "SQLGetInfo";
//static const char INIT_BASED szSQLGetStmtOption[]      = "SQLGetStmtOption";
//static const char INIT_BASED szSQLGetTypeInfo[]        = "SQLGetTypeInfo";
//static const char INIT_BASED szSQLParamData[]          = "SQLParamData";
//static const char INIT_BASED szSQLPutData[]            = "SQLPutData";
//static const char INIT_BASED szSQLSetConnectOption[]   = "SQLSetConnectOption";
//static const char INIT_BASED szSQLSetStmtOption[]      = "SQLSetStmtOption";
//static const char INIT_BASED szSQLSpecialColumns[]     = "SQLSpecialColumns";
//static const char INIT_BASED szSQLStatistics[]         = "SQLStatistics";
//static const char INIT_BASED szSQLTables[]             = "SQLTables";
//
//static const char INIT_BASED szSQLBrowseConnect[]      = "SQLBrowseConnect";
//static const char INIT_BASED szSQLColumnPrivileges[]   = "SQLColumnPrivileges";
//static const char INIT_BASED szSQLDataSources[]        = "SQLDataSources";
//static const char INIT_BASED szSQLDescribeParam[]      = "SQLDescribeParam";
//static const char INIT_BASED szSQLExtendedFetch[]      = "SQLExtendedFetch";
//static const char INIT_BASED szSQLForeignKeys[]        = "SQLForeignKeys";
//static const char INIT_BASED szSQLMoreResults[]        = "SQLMoreResults";
//static const char INIT_BASED szSQLNativeSql[]          = "SQLNativeSql";
//static const char INIT_BASED szSQLNumParams[]          = "SQLNumParams";
//static const char INIT_BASED szSQLParamOptions[]       = "SQLParamOptions";
//static const char INIT_BASED szSQLPrimaryKeys[]        = "SQLPrimaryKeys";
//static const char INIT_BASED szSQLProcedureColumns[]   = "SQLProcedureColumns";
//static const char INIT_BASED szSQLProcedures[]         = "SQLProcedures";
//static const char INIT_BASED szSQLSetPos[]             = "SQLSetPos";
//static const char INIT_BASED szSQLTablePrivileges[]    = "SQLTablePrivileges";

static const char INIT_BASED szSQLDrivers[]            = "SQLDrivers";
static const char INIT_BASED szSQLBindParameter[]      = "SQLBindParameter";
static const char INIT_BASED szSQLSetParam[]           = "SQLSetParam";
static const char INIT_BASED szSQLSetScrollOptions[]   = "SQLSetScrollOptions";

// for ODBC, use the ORDINAL values instead... (when practical)

#define szSQLAllocConnect      ORD_SQLALLOCCONNECT
#define szSQLAllocEnv          ORD_SQLALLOCENV
#define szSQLAllocStmt         ORD_SQLALLOCSTMT
#define szSQLBindCol           ORD_SQLBINDCOL
#define szSQLCancel            ORD_SQLCANCEL
#define szSQLColAttributes     ORD_SQLCOLATTRIBUTES
#define szSQLConnect           ORD_SQLCONNECT
#define szSQLDescribeCol       ORD_SQLDESCRIBECOL
#define szSQLDisconnect        ORD_SQLDISCONNECT
#define szSQLError             ORD_SQLERROR
#define szSQLExecDirect        ORD_SQLEXECDIRECT
#define szSQLExecute           ORD_SQLEXECUTE
#define szSQLFetch             ORD_SQLFETCH
#define szSQLFreeConnect       ORD_SQLFREECONNECT
#define szSQLFreeEnv           ORD_SQLFREEENV
#define szSQLFreeStmt          ORD_SQLFREESTMT
#define szSQLGetCursorName     ORD_SQLGETCURSORNAME
#define szSQLNumResultCols     ORD_SQLNUMRESULTCOLS
#define szSQLPrepare           ORD_SQLPREPARE
#define szSQLRowCount          ORD_SQLROWCOUNT
#define szSQLSetCursorName     ORD_SQLSETCURSORNAME
#define szSQLTransact          ORD_SQLTRANSACT

#define szSQLColumns           ORD_SQLCOLUMNS
#define szSQLDriverConnect     ORD_SQLDRIVERCONNECT
#define szSQLGetConnectOption  ORD_SQLGETCONNECTOPTION
#define szSQLGetData           ORD_SQLGETDATA
#define szSQLGetFunctions      ORD_SQLGETFUNCTIONS
#define szSQLGetInfo           ORD_SQLGETINFO
#define szSQLGetStmtOption     ORD_SQLGETSTMTOPTION
#define szSQLGetTypeInfo       ORD_SQLGETTYPEINFO
#define szSQLParamData         ORD_SQLPARAMDATA
#define szSQLPutData           ORD_SQLPUTDATA
#define szSQLSetConnectOption  ORD_SQLSETCONNECTOPTION
#define szSQLSetStmtOption     ORD_SQLSETSTMTOPTION
#define szSQLSpecialColumns    ORD_SQLSPECIALCOLUMNS
#define szSQLStatistics        ORD_SQLSTATISTICS
#define szSQLTables            ORD_SQLTABLES

#define szSQLBrowseConnect     ORD_SQLBROWSECONNECT
#define szSQLColumnPrivileges  ORD_SQLCOLUMNPRIVILEGES
#define szSQLDataSources       ORD_SQLDATASOURCES
#define szSQLDescribeParam     ORD_SQLDESCRIBEPARAM
#define szSQLExtendedFetch     ORD_SQLEXTENDEDFETCH
#define szSQLForeignKeys       ORD_SQLFOREIGNKEYS
#define szSQLMoreResults       ORD_SQLMORERESULTS
#define szSQLNativeSql         ORD_SQLNATIVESQL
#define szSQLNumParams         ORD_SQLNUMPARAMS
#define szSQLParamOptions      ORD_SQLPARAMOPTIONS
#define szSQLPrimaryKeys       ORD_SQLPRIMARYKEYS
#define szSQLProcedureColumns  ORD_SQLPROCEDURECOLUMNS
#define szSQLProcedures        ORD_SQLPROCEDURES
#define szSQLSetPos            ORD_SQLSETPOS
#define szSQLTablePrivileges   ORD_SQLTABLEPRIVILEGES

// #define szSQLDrivers           ORD_SQLDRIVERS
// #define szSQLBindParameter     ORD_SQLBINDPARAMETER
// #define szSQLSetParam          ORD_SQLSETPARAM
// #define szSQLSetScrollOptions  ORD_SQLSETSCROLLOPTIONS



static const char INIT_BASED szTOOLHELP_DLL[]          = "TOOLHELP.DLL";
static const char INIT_BASED szTOOLHELP[]              = "TOOLHELP";
static const char INIT_BASED szCOMMDLG_DLL[]           = "COMMDLG.DLL";
static const char INIT_BASED szCOMMDLG[]               = "COMMDLG";
static const char INIT_BASED szMMSYSTEM_DLL[]          = "MMSYSTEM.DLL";
static const char INIT_BASED szMMSYSTEM[]              = "MMSYSTEM";
static const char INIT_BASED szSHELL_DLL[]             = "SHELL.DLL";
static const char INIT_BASED szSHELL[]                 = "SHELL";
static const char INIT_BASED szDDEML_DLL[]             = "DDEML.DLL";
static const char INIT_BASED szDDEML[]                 = "DDEML";
static const char INIT_BASED szODBC_DLL[]              = "ODBC.DLL";
static const char INIT_BASED szODBC[]                  = "ODBC";
static const char INIT_BASED szODBC16GT_DLL[]          = "ODBC16GT.DLL";
static const char INIT_BASED szODBC16GT[]              = "ODBC16GT";
static const char INIT_BASED szKERNEL[]                = "KERNEL";



static const char INIT_BASED szMainWClass[]            = "MainWClass";
static const char INIT_BASED szMainMenu[]              = "MainMenu";
static const char INIT_BASED szWINCMDICON[]            = "SFTSHELLICON";
static const char INIT_BASED szMAINACCEL[]             = "MAINACCEL";
static const char INIT_BASED szSYSTEM_INI[]            = "SYSTEM.INI";



static const char INIT_BASED szBOOT[]                  = "BOOT";

#ifdef WIN32
#pragma data_seg()
#endif // WIN32



BOOL InitApplication(HINSTANCE hInstance)
{
WNDCLASS  wc;
LPSTR lp1, lp2;


   lp1 = work_buf;
   lp2 = work_buf + sizeof(work_buf)/2;

       /* generate path to 'SYSTEM.INI' in 'lp1' */

   GetWindowsDirectory(lp1, sizeof(work_buf)/2);
   if(lp1[lstrlen(lp1) - 1]!='\\') lstrcat(lp1, "\\");
   lstrcat(lp1,szSYSTEM_INI);

#ifndef WIN32
   if(GetNumTasks()<2)                          /* hey, I'm the SHELL!!! */
   {
      IsShell = TRUE;
   }
   else
#endif // WIN32
   {
    OFSTRUCT ofn;

      GetPrivateProfileString(szBOOT,szSHELL,"",lp2,sizeof(work_buf)/2,lp1);

      GetModuleFileName(hInstance, lp1, sizeof(work_buf)/2);

      if(MyOpenFile(lp2, &ofn, OF_READ | OF_SHARE_COMPAT | OF_EXIST))
      {
                   /* this expands out the PATH if required */

         OemToAnsi(ofn.szPathName, lp2);
      }

      DosQualifyPath(lp1, lp1);
      DosQualifyPath(lp2, lp2);

      if(_fstricmp(lp1,lp2)==0)         /* they are the same!! */
      {
         IsShell = TRUE;
      }
      else
      {
         IsShell = FALSE;
      }
   }

   hMainCursor = LoadCursor(NULL, IDC_ARROW);



   // register TOOLBAR HINT window class

   wc.style         = 0;
   wc.lpfnWndProc   = (WNDPROC)ToolBarHintProc;

   wc.cbClsExtra    = 0;
   wc.cbWndExtra    = 0;
   wc.hInstance     = hInstance;
   wc.hIcon         = NULL;
   wc.hCursor       = hMainCursor;
   wc.hbrBackground = NULL;
   wc.lpszMenuName  = NULL;
   wc.lpszClassName = szToolBarHint;

   if(!RegisterClass(&wc)) return(0);  // error!



   // register MAIN WINDOW class

   wc.style         = CS_HREDRAW | CS_VREDRAW;
   wc.lpfnWndProc   = (WNDPROC)MainWndProc;

   wc.cbClsExtra    = 0;
   wc.cbWndExtra    = 0;
   wc.hInstance     = hInstance;
   wc.hIcon         = LoadIcon(GetInstanceModule(hInstance), szWINCMDICON);
   wc.hCursor       = hMainCursor;
   wc.hbrBackground = GetStockObject(BLACK_BRUSH);
   wc.lpszMenuName  = szMainMenu;
   wc.lpszClassName = szMainWClass;

   return (RegisterClass(&wc));

}

BOOL FAR PASCAL MyAddFontResource(LPCSTR lpcFontFile)
{
char tbuf[MAX_PATH];

//   if(SELECTOROF(lpcFontFile))
   if(HIWORD(((DWORD)lpcFontFile)))
   {
      return(AddFontResource(lpcFontFile));
   }
   else
   {
      GetModuleFileName((HINSTANCE)LOWORD(((DWORD)lpcFontFile)),
                        tbuf, sizeof(tbuf));
      return(AddFontResource(tbuf));
   }
}


BOOL FAR PASCAL __export AuthCodeDialog(HWND hDlg, UINT uiMsg,
                                        WPARAM wParam, LPARAM lParam)
{
static LPSTR lpDest=NULL;

   switch(uiMsg)
   {
      case WM_INITDIALOG:
        lpDest = lParam;
        return(TRUE);


      case WM_COMMAND:
        switch((WORD)wParam)
        {
           case IDOK:
              if(lpDest) GetDlgItemText(hDlg, 101, lpDest, 32);
              EndDialog(hDlg, IDOK);
              return(TRUE);

           case IDCANCEL:
              if(lpDest) *lpDest = 0;
              EndDialog(hDlg, IDCANCEL);
              return(TRUE);
        }
        break;
   }

   return(FALSE);
}

static int NEAR PASCAL Rad34DigitValue(char c)
{

   c = toupper(c);

   if(c >= '0' && c <= '9')
   {
      return(c - '0');
   }
   else if(c >= 'A' && c <= 'N')
   {
      return(c - 'A' + 10);
   }
   else if(c >= 'P' && c <= 'Z')
   {
      return(c - 'A' + 10 - 1);  // I skipped the 'O'
   }
   else
   {
      return(-1);
   }
}

BOOL InitInstance(HANDLE hInstance, int nCmdShow, BOOL ForceExternalLibrary)
{
OFSTRUCT ofstr;
LPSTR lp1, lp2, lp3, lp4;
WNDCLASS wc2;
HFILE hFile;
int i, __winflags__;
char tbuf[256];



   hInst = hInstance;
   dwDialogUnits = GetDialogBaseUnits();  // do this ONCE, here...

   *tbuf = 0;
   GetModuleFileName((HMODULE)hInstance, tbuf, sizeof(tbuf));
   hModule = GetModuleHandle(tbuf);  // store module handle (for use later)
   *tbuf = 0;


   // get info about 'STATIC' and 'EDIT' window classes so I can subclass them

   _hmemset((LPSTR)&wc2, 0, sizeof(wc2));

   if(GetClassInfo(NULL, "STATIC", &wc2))
   {
      lpStaticWndProc = (WNDPROC)wc2.lpfnWndProc;
   }

   _hmemset((LPSTR)&wc2, 0, sizeof(wc2));

   if(GetClassInfo(NULL, "EDIT", &wc2))
   {
      lpEditWndProc = (WNDPROC)wc2.lpfnWndProc;
   }


   // get handles for DLL's that are ALWAYS loaded, and version info

#ifndef WIN32

   hKernel = GetModuleHandle(szKERNEL);      /* it's ALWAYS loaded!! */
   hUser   = GetModuleHandle("USER");        /* and so is this! */


   uVersion.dw = GetVersion();         /* the current windows/dos version */

   if(uVersion.ver.win_major<3 ||
      (uVersion.ver.win_major==3 && uVersion.ver.win_minor<0x0a))
   {
                /** Earlier than 3.1 **/
      lpIsTask = MyIsTask;

      IsChicago = FALSE;
   }
   else
   {
#ifndef WF_WINNT
      if(GetWinFlags() & 0x4000)  // Windows NT
#else  // WF_WINNT
      if(GetWinFlags() & WF_WINNT)
#endif // WF_WINNT
      {
         // WINDOWS NT!!!  Load the 'Generic Thunk' stuff...

         (FARPROC)lpGetVDMPointer32W    = GetProcAddress(hKernel, szGetVDMPointer32W);
         (FARPROC)lpLoadLibraryEx32W    = GetProcAddress(hKernel, szLoadLibraryEx32W);
         (FARPROC)lpGetProcAddress32W   = GetProcAddress(hKernel, szGetProcAddress32W);
         (FARPROC)lpFreeLibrary32W      = GetProcAddress(hKernel, szFreeLibrary32W);
         (FARPROC)lpCallProcEx32W       = GetProcAddress(hKernel, szCallProcEx32W);

         IsNT = TRUE;  // this really means that 'WOW32' stuff is available
      }
      else if(uVersion.ver.win_major >3 ||
              (uVersion.ver.win_major == 3 && uVersion.ver.win_minor>=0x50))
      {
         // This release follows WIN 3.5 and is therefore `Chicago'

         IsChicago = TRUE;
         IsNT = TRUE;  // this really means that 'WOW32' stuff is available

         OutputDebugString("SFTSHELL - 'Chicago' Extensions are active\r\n");

         (FARPROC)lpGetVDMPointer32W    = GetProcAddress(hKernel, szGetVDMPointer32W);
         (FARPROC)lpLoadLibraryEx32W    = GetProcAddress(hKernel, szLoadLibraryEx32W);
         (FARPROC)lpGetProcAddress32W   = GetProcAddress(hKernel, szGetProcAddress32W);
         (FARPROC)lpFreeLibrary32W      = GetProcAddress(hKernel, szFreeLibrary32W);
         (FARPROC)lpCallProcEx32W       = GetProcAddress(hKernel, szCallProcEx32W);
      }
      else
      {
         // Win 3.x

         (FARPROC)lpGetVDMPointer32W    = NULL;  // act as flags - see below
         (FARPROC)lpLoadLibraryEx32W    = NULL;
         (FARPROC)lpGetProcAddress32W   = NULL;
         (FARPROC)lpFreeLibrary32W      = NULL;
         (FARPROC)lpCallProcEx32W       = NULL;

         IsNT = FALSE;  // to make sure there are no problems later
      }


      (FARPROC)lpIsTask = GetProcAddress(hKernel, szIsTask);
      if(!lpIsTask) lpIsTask = MyIsTask;


      // next, check for the presence of 'Generic Thunk' procs (NT, Win '95)

      if(!lpGetVDMPointer32W || !lpLoadLibraryEx32W ||
         !lpGetProcAddress32W || !lpFreeLibrary32W || !lpCallProcEx32W)
      {
         lpGetVDMPointer32W    = NULL;  // one fails, they all fail
         lpLoadLibraryEx32W    = NULL;
         lpGetProcAddress32W   = NULL;
         lpFreeLibrary32W      = NULL;
         lpCallProcEx32W       = NULL;

         hKernel32 = NULL;
         hSFTSH32T = NULL;

         IsNT = FALSE;  // this overcomes errors also
      }
      else
      {
         hKernel32 = lpLoadLibraryEx32W("KERNEL32.DLL",0,0);
         hWOW32    = lpLoadLibraryEx32W("WOW32.DLL",0,0);
         hMPR      = lpLoadLibraryEx32W("MPR.DLL",0,0);

         hSFTSH32T = lpLoadLibraryEx32W("SFTSH32T.DLL",0,0);


         if(!IsChicago &&    // didn't detect 'chicago' (win '95) earlier...
            hKernel32)       // see what version Win32 returns...
         {
            IsNT = TRUE; // it's Windows 'NT (or Win '95)
         }
      }
   }

   // NET stuff - located in 'user.exe'

   (FARPROC)lpWNetAddConnection    = GetProcAddress(hUser, szWNetAddConnection);
   (FARPROC)lpWNetCancelConnection = GetProcAddress(hUser, szWNetCancelConnection);
   (FARPROC)lpWNetGetConnection    = GetProcAddress(hUser, szWNetGetConnection);

   if(!lpWNetAddConnection || !lpWNetCancelConnection | !lpWNetGetConnection)
   {
      lpWNetAddConnection    = NULL;
      lpWNetCancelConnection = NULL;
      lpWNetGetConnection    = NULL;
   }


#endif // WIN32


   if(!(lpCmdHist = GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                   CMDHIST_SIZE)))
   {
      return(FALSE);
   }


#ifndef WIN32

          /******************************************************/
          /* load 'TOOLHELP.DLL' library and get proc addresses */
          /******************************************************/

   if(!(hToolHelp = GetModuleHandle(szTOOLHELP)) &&
      MyOpenFile(szTOOLHELP_DLL, (LPOFSTRUCT)&ofstr, OF_EXIST)==HFILE_ERROR)
   {
      hToolHelp = (HMODULE)NULL;
   }
   else
   {
      if(hToolHelp)
      {
         GetModuleFileName(hToolHelp, ofstr.szPathName,
                           sizeof(ofstr.szPathName));
      }

      hToolHelp = LoadLibrary((LPSTR)ofstr.szPathName);
   }


   if((WORD)hToolHelp>=32)          /* this means it's ok! */
   {
      (FARPROC)lpGlobalFirst      = GetProcAddress(hToolHelp, szGlobalFirst);
      (FARPROC)lpGlobalNext       = GetProcAddress(hToolHelp, szGlobalNext);

      (FARPROC)lpMemManInfo       = GetProcAddress(hToolHelp, szMemManInfo);

      (FARPROC)lpModuleFirst      = GetProcAddress(hToolHelp, szModuleFirst);
      (FARPROC)lpModuleNext       = GetProcAddress(hToolHelp, szModuleNext);
      (FARPROC)lpModuleFindHandle = GetProcAddress(hToolHelp, szModuleFindHandle);

      (FARPROC)lpNotifyRegister   = GetProcAddress(hToolHelp, szNotifyRegister);
      (FARPROC)lpNotifyUnRegister = GetProcAddress(hToolHelp, szNotifyUnRegister);

      (FARPROC)lpTaskFirst        = GetProcAddress(hToolHelp, szTaskFirst);
      (FARPROC)lpTaskNext         = GetProcAddress(hToolHelp, szTaskNext);
      (FARPROC)lpTaskFindHandle   = GetProcAddress(hToolHelp, szTaskFindHandle);


      if(!lpGlobalFirst      || !lpGlobalNext || !lpMemManInfo     ||
         !lpModuleFirst      || !lpModuleNext || !lpTaskFindHandle ||
         !lpModuleFindHandle || !lpTaskFirst  || !lpTaskNext       ||
         !lpNotifyRegister   || !lpNotifyUnRegister )
      {
         FreeLibrary(hToolHelp);

         hToolHelp = (HMODULE)NULL;  /* follows through to next section */
      }
   }

             /* this section loads my 'toolhelp' procs */

   if((WORD)hToolHelp<32) /* an error loading procs, or library */
   {
      hToolHelp = (HMODULE)0;

      lpGlobalFirst      = NULL;     /* until I get my version written */
      lpGlobalNext       = NULL;

      lpMemManInfo       = MyMemManInfo;

      lpModuleFirst      = NULL;
      lpModuleNext       = NULL;
      lpModuleFindHandle = NULL;

      lpTaskFirst        = MyTaskFirst;
      lpTaskNext         = MyTaskNext;
      lpTaskFindHandle   = MyTaskFindHandle;

      (FARPROC)lpGlobalMasterHandle =
                          GetProcAddress(hKernel, szGlobalMasterHandle);
      (FARPROC)lpGlobalHandleNoRIP  =
                          GetProcAddress(hKernel, szGlobalHandleNoRIP);
      wIGROUP = (_segment)lpGlobalMasterHandle;   /* assume this is seg 1 */
   }


          /*****************************************************/
          /* load 'COMMDLG.DLL' library and get proc addresses */
          /*****************************************************/

   if(!(hCommDlg = GetModuleHandle(szCOMMDLG)) &&
      MyOpenFile(szCOMMDLG_DLL, (LPOFSTRUCT)&ofstr, OF_EXIST)==HFILE_ERROR)
   {
      hCommDlg = (HMODULE)NULL;
   }
   else
   {
      if(hCommDlg)
      {
         GetModuleFileName(hCommDlg, ofstr.szPathName,
                           sizeof(ofstr.szPathName));
      }

      hCommDlg = LoadLibrary((LPSTR)ofstr.szPathName);
   }


   if((WORD)hCommDlg>=32)          /* this means it's ok! */
   {
      (FARPROC)lpGetOpenFileName      = GetProcAddress(hCommDlg, szGetOpenFileName);
      (FARPROC)lpGetSaveFileName      = GetProcAddress(hCommDlg, szGetSaveFileName);
      (FARPROC)lpChooseFont           = GetProcAddress(hCommDlg, szChooseFont);
      (FARPROC)lpCommDlgExtendedError = GetProcAddress(hCommDlg, szCommDlgExtendedError);

      if(!lpGetOpenFileName || !lpGetSaveFileName || !lpCommDlgExtendedError)
      {
         FreeLibrary(hCommDlg);

         hCommDlg = (HMODULE)NULL;  /* follows through to next section */
      }
   }

             /* this section loads my 'toolhelp' procs */

   if((WORD)hCommDlg<32) /* an error loading procs, or library */
   {
      lpGetOpenFileName      = NULL;
      lpGetSaveFileName      = NULL;
      lpChooseFont           = NULL;
      lpCommDlgExtendedError = NULL;

      hCommDlg = NULL;
   }



     /**-----------------------------------------------------------**/
     /** LOAD MULTI-MEDIA SYSTEM 'MMSYSTEM' and get Proc Addresses **/
     /**-----------------------------------------------------------**/

   if(!(hMMSystem = GetModuleHandle(szMMSYSTEM)) &&
      MyOpenFile(szMMSYSTEM_DLL, (LPOFSTRUCT)&ofstr, OF_EXIST)==HFILE_ERROR)
   {
      hMMSystem = (HMODULE)NULL;
   }
   else
   {
      if(hMMSystem)
      {
         GetModuleFileName(hMMSystem, ofstr.szPathName,
                           sizeof(ofstr.szPathName));
      }

      hMMSystem = LoadLibrary((LPSTR)ofstr.szPathName);
   }

   if((WORD)hMMSystem>=32)
   {
      (FARPROC)lpsndPlaySound = GetProcAddress(hMMSystem, szsndPlaySound);
   }
   else
   {
      hMMSystem = NULL;
   }


     /**-----------------------------------------------------------**/
     /** LOAD SHELL/REG SUPPORT 'SHELL.DLL' and get Proc Addresses **/
     /**-----------------------------------------------------------**/

   if(!(hShell = GetModuleHandle(szSHELL)) &&
      MyOpenFile(szSHELL_DLL, (LPOFSTRUCT)&ofstr, OF_EXIST)==HFILE_ERROR)
   {
      hShell = (HMODULE)NULL;
   }
   else
   {
      if(hShell)
      {
         GetModuleFileName(hShell, ofstr.szPathName,
                           sizeof(ofstr.szPathName));
      }

      hShell = LoadLibrary((LPSTR)ofstr.szPathName);
   }

   if((WORD)hShell>=32)
   {
      (FARPROC)lpRegCloseKey     = GetProcAddress(hShell,szRegCloseKey);
      (FARPROC)lpRegCreateKey    = GetProcAddress(hShell,szRegCreateKey);
      (FARPROC)lpRegDeleteKey    = GetProcAddress(hShell,szRegDeleteKey);
      (FARPROC)lpRegEnumKey      = GetProcAddress(hShell,szRegEnumKey);
      (FARPROC)lpRegOpenKey      = GetProcAddress(hShell,szRegOpenKey);
      (FARPROC)lpRegQueryValue   = GetProcAddress(hShell,szRegQueryValue);
      (FARPROC)lpRegSetValue     = GetProcAddress(hShell,szRegSetValue);

      (FARPROC)lpDragAcceptFiles = GetProcAddress(hShell,szDragAcceptFiles);
      (FARPROC)lpDragFinish      = GetProcAddress(hShell,szDragFinish);
      (FARPROC)lpDragQueryFile   = GetProcAddress(hShell,szDragQueryFile);

      (FARPROC)lpShellExecute    = GetProcAddress(hShell,szShellExecute);

      if(!lpDragAcceptFiles || !lpDragFinish  || !lpDragQueryFile ||
         !lpShellExecute    || !lpRegCloseKey || !lpRegCreateKey ||
         !lpRegDeleteKey    || !lpRegEnumKey  || !lpRegOpenKey   ||
         !lpRegQueryValue   || !lpRegSetValue )
      {
         FreeLibrary(hShell);

         hShell = NULL;  // flows through to section below
      }

   }

   if((WORD)hShell < 32)
   {
      hShell = NULL;

      lpRegCloseKey     = NULL;
      lpRegCreateKey    = NULL;
      lpRegDeleteKey    = NULL;
      lpRegEnumKey      = NULL;
      lpRegOpenKey      = NULL;
      lpRegQueryValue   = NULL;
      lpRegSetValue     = NULL;

      lpDragAcceptFiles = NULL;
      lpDragFinish      = NULL;
      lpDragQueryFile   = NULL;

      lpShellExecute    = NULL;
   }




     /**-----------------------------------------------------------**/
     /** LOAD DDEML API LIBRARY 'DDEML.DLL' and get Proc Addresses **/
     /**-----------------------------------------------------------**/

   if(!(hDdeml = GetModuleHandle(szDDEML)) &&
      MyOpenFile(szDDEML_DLL, (LPOFSTRUCT)&ofstr, OF_EXIST)==HFILE_ERROR)
   {
      if(IsNT && !IsChicago)
      {
        // this file may be located in the 'SYSTEM32' directory...

         GetSystemDirectory(tbuf, sizeof(tbuf));

         if(lstrlen(tbuf) && tbuf[lstrlen(tbuf) - 1] == '\\')
            tbuf[lstrlen(tbuf) - 1] = 0;

         if(lstrlen(tbuf) > 2 &&
            stricmp(tbuf + lstrlen(tbuf) - 2, "32"))  // doesn't end in '32'
         {
            lstrcat(tbuf, "32");
         }

         lstrcat(tbuf, "\\");
         lstrcat(tbuf, szDDEML_DLL);

         hDdeml = LoadLibrary(tbuf);

         *tbuf = 0;

// TEMPORARY TEMPORARY TEMPORARY

         if(!hDdeml)
         {
            MessageBox(NULL,
                       "Unable to load DDEML.DLL",
                       "SFTSHELL 'NT' ERROR",
                       MB_OK | MB_ICONHAND);
         }
      }
      else
      {
         hDdeml = (HMODULE)NULL;
      }
   }
   else
   {
      if(hDdeml)
      {
         GetModuleFileName(hDdeml, ofstr.szPathName,
                           sizeof(ofstr.szPathName));
      }

      hDdeml = LoadLibrary((LPSTR)ofstr.szPathName);
   }

   if((WORD)hDdeml>=32)
   {
      (FARPROC)lpDdeAbandonTransaction = GetProcAddress(hDdeml,
                                                        szDdeAbandonTransaction);
      (FARPROC)lpDdeAccessData         = GetProcAddress(hDdeml,
                                                        szDdeAccessData);
      (FARPROC)lpDdeAddData            = GetProcAddress(hDdeml,
                                                        szDdeAddData);
      (FARPROC)lpDdeClientTransaction  = GetProcAddress(hDdeml,
                                                        szDdeClientTransaction);
      (FARPROC)lpDdeCmpStringHandles   = GetProcAddress(hDdeml,
                                                        szDdeCmpStringHandles);
      (FARPROC)lpDdeConnect            = GetProcAddress(hDdeml,
                                                        szDdeConnect);
      (FARPROC)lpDdeConnectList        = GetProcAddress(hDdeml,
                                                        szDdeConnectList);
      (FARPROC)lpDdeCreateDataHandle   = GetProcAddress(hDdeml,
                                                        szDdeCreateDataHandle);
      (FARPROC)lpDdeCreateStringHandle = GetProcAddress(hDdeml,
                                                        szDdeCreateStringHandle);
      (FARPROC)lpDdeDisconnect         = GetProcAddress(hDdeml,
                                                        szDdeDisconnect);
      (FARPROC)lpDdeDisconnectList     = GetProcAddress(hDdeml,
                                                        szDdeDisconnectList);
      (FARPROC)lpDdeEnableCallback     = GetProcAddress(hDdeml,
                                                        szDdeEnableCallback);
      (FARPROC)lpDdeFreeDataHandle     = GetProcAddress(hDdeml,
                                                        szDdeFreeDataHandle);
      (FARPROC)lpDdeFreeStringHandle   = GetProcAddress(hDdeml,
                                                        szDdeFreeStringHandle);
      (FARPROC)lpDdeGetData            = GetProcAddress(hDdeml,
                                                        szDdeGetData);
      (FARPROC)lpDdeGetLastError       = GetProcAddress(hDdeml,
                                                        szDdeGetLastError);
      (FARPROC)lpDdeInitialize         = GetProcAddress(hDdeml,
                                                        szDdeInitialize);
      (FARPROC)lpDdeKeepStringHandle   = GetProcAddress(hDdeml,
                                                        szDdeKeepStringHandle);
      (FARPROC)lpDdeNameService        = GetProcAddress(hDdeml,
                                                        szDdeNameService);
      (FARPROC)lpDdePostAdvise         = GetProcAddress(hDdeml,
                                                        szDdePostAdvise);
      (FARPROC)lpDdeQueryConvInfo      = GetProcAddress(hDdeml,
                                                        szDdeQueryConvInfo);
      (FARPROC)lpDdeQueryNextServer    = GetProcAddress(hDdeml,
                                                        szDdeQueryNextServer);
      (FARPROC)lpDdeQueryString        = GetProcAddress(hDdeml,
                                                        szDdeQueryString);
      (FARPROC)lpDdeReconnect          = GetProcAddress(hDdeml,
                                                        szDdeReconnect);
      (FARPROC)lpDdeSetUserHandle      = GetProcAddress(hDdeml,
                                                        szDdeSetUserHandle);
      (FARPROC)lpDdeUnaccessData       = GetProcAddress(hDdeml,
                                                        szDdeUnaccessData);
      (FARPROC)lpDdeUninitialize       = GetProcAddress(hDdeml,
                                                        szDdeUninitialize);

      if(!lpDdeAbandonTransaction || !lpDdeAccessData ||
         !lpDdeAddData            || !lpDdeClientTransaction ||
         !lpDdeCmpStringHandles   || !lpDdeConnect ||
         !lpDdeConnectList        || !lpDdeCreateDataHandle ||
         !lpDdeCreateStringHandle || !lpDdeDisconnect ||
         !lpDdeDisconnectList     || !lpDdeEnableCallback ||
         !lpDdeFreeDataHandle     || !lpDdeFreeStringHandle ||
         !lpDdeGetData            || !lpDdeGetLastError ||
         !lpDdeInitialize         || !lpDdeKeepStringHandle ||
         !lpDdeNameService        || !lpDdePostAdvise ||
         !lpDdeQueryConvInfo      || !lpDdeQueryNextServer ||
         !lpDdeQueryString        || !lpDdeReconnect ||
         !lpDdeSetUserHandle      || !lpDdeUnaccessData ||
         !lpDdeUninitialize)
      {
         (FARPROC)lpDdeAbandonTransaction =
         (FARPROC)lpDdeAccessData =
         (FARPROC)lpDdeAddData =
         (FARPROC)lpDdeClientTransaction =
         (FARPROC)lpDdeCmpStringHandles =
         (FARPROC)lpDdeConnect =
         (FARPROC)lpDdeConnectList =
         (FARPROC)lpDdeCreateDataHandle =
         (FARPROC)lpDdeCreateStringHandle =
         (FARPROC)lpDdeDisconnect =
         (FARPROC)lpDdeDisconnectList =
         (FARPROC)lpDdeEnableCallback =
         (FARPROC)lpDdeFreeDataHandle =
         (FARPROC)lpDdeFreeStringHandle =
         (FARPROC)lpDdeGetData =
         (FARPROC)lpDdeGetLastError =
         (FARPROC)lpDdeInitialize =
         (FARPROC)lpDdeKeepStringHandle =
         (FARPROC)lpDdeNameService =
         (FARPROC)lpDdePostAdvise =
         (FARPROC)lpDdeQueryConvInfo =
         (FARPROC)lpDdeQueryNextServer =
         (FARPROC)lpDdeQueryString =
         (FARPROC)lpDdeReconnect =
         (FARPROC)lpDdeSetUserHandle =
         (FARPROC)lpDdeUnaccessData =
         (FARPROC)lpDdeUninitialize = NULL;
      }
   }
   else
   {
      hDdeml = NULL;
   }

#endif // WIN32


   // ODBC! (both 16-bit and 32-bit)

//   if(!(hODBC = GetModuleHandle(szODBC16GT)) &&
//      MyOpenFile(szODBC16GT_DLL, (LPOFSTRUCT)&ofstr, OF_EXIST)==HFILE_ERROR)
//   {
//      hODBC = (HMODULE)NULL;
//   }
//   else
//   {
//      if(hODBC)
//      {
//         GetModuleFileName(hODBC, ofstr.szPathName,
//                           sizeof(ofstr.szPathName));
//      }
//
//      hODBC = LoadLibrary((LPSTR)ofstr.szPathName);
//   }



   if(!hODBC)
   {
      if(!(hODBC = GetModuleHandle(szODBC)) &&
         MyOpenFile(szODBC_DLL, (LPOFSTRUCT)&ofstr, OF_EXIST)==HFILE_ERROR)
      {
         hODBC = (HMODULE)NULL;
      }
      else
      {
         if(hODBC)
         {
            GetModuleFileName(hODBC, ofstr.szPathName,
                              sizeof(ofstr.szPathName));
         }

         hODBC = LoadLibrary((LPSTR)ofstr.szPathName);
      }
   }


   if((WORD)hODBC>=32)
   {
      (FARPROC)lpSQLAllocConnect     = GetProcAddress(hODBC,szSQLAllocConnect);
      (FARPROC)lpSQLAllocEnv         = GetProcAddress(hODBC,szSQLAllocEnv);
      (FARPROC)lpSQLAllocStmt        = GetProcAddress(hODBC,szSQLAllocStmt);
      (FARPROC)lpSQLBindCol          = GetProcAddress(hODBC,szSQLBindCol);
      (FARPROC)lpSQLCancel           = GetProcAddress(hODBC,szSQLCancel);
      (FARPROC)lpSQLColAttributes    = GetProcAddress(hODBC,szSQLColAttributes);
      (FARPROC)lpSQLConnect          = GetProcAddress(hODBC,szSQLConnect);
      (FARPROC)lpSQLDescribeCol      = GetProcAddress(hODBC,szSQLDescribeCol);
      (FARPROC)lpSQLDisconnect       = GetProcAddress(hODBC,szSQLDisconnect);
      (FARPROC)lpSQLError            = GetProcAddress(hODBC,szSQLError);
      (FARPROC)lpSQLExecDirect       = GetProcAddress(hODBC,szSQLExecDirect);
      (FARPROC)lpSQLExecute          = GetProcAddress(hODBC,szSQLExecute);
      (FARPROC)lpSQLFetch            = GetProcAddress(hODBC,szSQLFetch);
      (FARPROC)lpSQLFreeConnect      = GetProcAddress(hODBC,szSQLFreeConnect);
      (FARPROC)lpSQLFreeEnv          = GetProcAddress(hODBC,szSQLFreeEnv);
      (FARPROC)lpSQLFreeStmt         = GetProcAddress(hODBC,szSQLFreeStmt);
      (FARPROC)lpSQLGetCursorName    = GetProcAddress(hODBC,szSQLGetCursorName);
      (FARPROC)lpSQLNumResultCols    = GetProcAddress(hODBC,szSQLNumResultCols);
      (FARPROC)lpSQLPrepare          = GetProcAddress(hODBC,szSQLPrepare);
      (FARPROC)lpSQLRowCount         = GetProcAddress(hODBC,szSQLRowCount);
      (FARPROC)lpSQLSetCursorName    = GetProcAddress(hODBC,szSQLSetCursorName);
      (FARPROC)lpSQLTransact         = GetProcAddress(hODBC,szSQLTransact);
      (FARPROC)lpSQLColumns          = GetProcAddress(hODBC,szSQLColumns);
      (FARPROC)lpSQLDriverConnect    = GetProcAddress(hODBC,szSQLDriverConnect);
      (FARPROC)lpSQLGetConnectOption = GetProcAddress(hODBC,szSQLGetConnectOption);
      (FARPROC)lpSQLGetData          = GetProcAddress(hODBC,szSQLGetData);
      (FARPROC)lpSQLGetFunctions     = GetProcAddress(hODBC,szSQLGetFunctions);
      (FARPROC)lpSQLGetInfo          = GetProcAddress(hODBC,szSQLGetInfo);
      (FARPROC)lpSQLGetStmtOption    = GetProcAddress(hODBC,szSQLGetStmtOption);
      (FARPROC)lpSQLGetTypeInfo      = GetProcAddress(hODBC,szSQLGetTypeInfo);
      (FARPROC)lpSQLParamData        = GetProcAddress(hODBC,szSQLParamData);
      (FARPROC)lpSQLPutData          = GetProcAddress(hODBC,szSQLPutData);
      (FARPROC)lpSQLSetConnectOption = GetProcAddress(hODBC,szSQLSetConnectOption);
      (FARPROC)lpSQLSetStmtOption    = GetProcAddress(hODBC,szSQLSetStmtOption);
      (FARPROC)lpSQLSpecialColumns   = GetProcAddress(hODBC,szSQLSpecialColumns);
      (FARPROC)lpSQLStatistics       = GetProcAddress(hODBC,szSQLStatistics);
      (FARPROC)lpSQLTables           = GetProcAddress(hODBC,szSQLTables);
      (FARPROC)lpSQLBrowseConnect    = GetProcAddress(hODBC,szSQLBrowseConnect);
      (FARPROC)lpSQLColumnPrivileges = GetProcAddress(hODBC,szSQLColumnPrivileges);
      (FARPROC)lpSQLDataSources      = GetProcAddress(hODBC,szSQLDataSources);
      (FARPROC)lpSQLDescribeParam    = GetProcAddress(hODBC,szSQLDescribeParam);
      (FARPROC)lpSQLExtendedFetch    = GetProcAddress(hODBC,szSQLExtendedFetch);
      (FARPROC)lpSQLForeignKeys      = GetProcAddress(hODBC,szSQLForeignKeys);
      (FARPROC)lpSQLMoreResults      = GetProcAddress(hODBC,szSQLMoreResults);
      (FARPROC)lpSQLNativeSql        = GetProcAddress(hODBC,szSQLNativeSql);
      (FARPROC)lpSQLNumParams        = GetProcAddress(hODBC,szSQLNumParams);
      (FARPROC)lpSQLParamOptions     = GetProcAddress(hODBC,szSQLParamOptions);
      (FARPROC)lpSQLPrimaryKeys      = GetProcAddress(hODBC,szSQLPrimaryKeys);
      (FARPROC)lpSQLProcedureColumns = GetProcAddress(hODBC,szSQLProcedureColumns);
      (FARPROC)lpSQLProcedures       = GetProcAddress(hODBC,szSQLProcedures);
      (FARPROC)lpSQLSetPos           = GetProcAddress(hODBC,szSQLSetPos);
      (FARPROC)lpSQLTablePrivileges  = GetProcAddress(hODBC,szSQLTablePrivileges);
      (FARPROC)lpSQLDrivers          = GetProcAddress(hODBC,szSQLDrivers);
      (FARPROC)lpSQLBindParameter    = GetProcAddress(hODBC,szSQLBindParameter);
      (FARPROC)lpSQLSetParam         = GetProcAddress(hODBC,szSQLSetParam);
      (FARPROC)lpSQLSetScrollOptions = GetProcAddress(hODBC,szSQLSetScrollOptions);


      if(!lpSQLAllocConnect     || !lpSQLAllocEnv         ||
         !lpSQLAllocStmt        || !lpSQLBindCol          ||
         !lpSQLCancel           || !lpSQLColAttributes    ||
         !lpSQLConnect          || !lpSQLDescribeCol      ||
         !lpSQLDisconnect       || !lpSQLError            ||
         !lpSQLExecDirect       || !lpSQLExecute          ||
         !lpSQLFetch            || !lpSQLFreeConnect      ||
         !lpSQLFreeEnv          || !lpSQLFreeStmt         ||
         !lpSQLGetCursorName    || !lpSQLNumResultCols    ||
         !lpSQLPrepare          || !lpSQLRowCount         ||
         !lpSQLSetCursorName    || !lpSQLTransact         ||
         !lpSQLColumns          || !lpSQLDriverConnect    ||
         !lpSQLGetConnectOption || !lpSQLGetData          ||
         !lpSQLGetFunctions     || !lpSQLGetInfo          ||
         !lpSQLGetStmtOption    || !lpSQLGetTypeInfo      ||
         !lpSQLParamData        || !lpSQLPutData          ||
         !lpSQLSetConnectOption || !lpSQLSetStmtOption    ||
         !lpSQLSpecialColumns   || !lpSQLStatistics       ||
         !lpSQLTables           || !lpSQLBrowseConnect    ||
         !lpSQLColumnPrivileges || !lpSQLDataSources      ||
         !lpSQLDescribeParam    || !lpSQLExtendedFetch    ||
         !lpSQLForeignKeys      || !lpSQLMoreResults      ||
         !lpSQLNativeSql        || !lpSQLNumParams        ||
         !lpSQLParamOptions     || !lpSQLPrimaryKeys      ||
         !lpSQLProcedureColumns || !lpSQLProcedures       ||
         !lpSQLSetPos           || !lpSQLTablePrivileges )
//         !lpSQLDrivers          || !lpSQLBindParameter    ||
//         !lpSQLSetParam         || !lpSQLSetScrollOptions )
      {
         FreeLibrary(hODBC);

         hODBC = NULL;  // flows through to section below
      }

      // IMPORTANT:  ODBC 1.0 drivers will have NULL 'lpSQLDrivers' and
      //             'lpSQLBindParameter'.  >2.0 drivers may have NULL
      //             'lpSQLSetParam' or 'lpSQLSetScrollOptions'.  I Must
      //             check these pointers before using them...
   }

   if((WORD)hODBC < 32)
   {
      lpSQLAllocConnect     = NULL;
      lpSQLAllocEnv         = NULL;
      lpSQLAllocStmt        = NULL;
      lpSQLBindCol          = NULL;
      lpSQLCancel           = NULL;
      lpSQLColAttributes    = NULL;
      lpSQLConnect          = NULL;
      lpSQLDescribeCol      = NULL;
      lpSQLDisconnect       = NULL;
      lpSQLError            = NULL;
      lpSQLExecDirect       = NULL;
      lpSQLExecute          = NULL;
      lpSQLFetch            = NULL;
      lpSQLFreeConnect      = NULL;
      lpSQLFreeEnv          = NULL;
      lpSQLFreeStmt         = NULL;
      lpSQLGetCursorName    = NULL;
      lpSQLNumResultCols    = NULL;
      lpSQLPrepare          = NULL;
      lpSQLRowCount         = NULL;
      lpSQLSetCursorName    = NULL;
      lpSQLTransact         = NULL;
      lpSQLColumns          = NULL;
      lpSQLDriverConnect    = NULL;
      lpSQLGetConnectOption = NULL;
      lpSQLGetData          = NULL;
      lpSQLGetFunctions     = NULL;
      lpSQLGetInfo          = NULL;
      lpSQLGetStmtOption    = NULL;
      lpSQLGetTypeInfo      = NULL;
      lpSQLParamData        = NULL;
      lpSQLPutData          = NULL;
      lpSQLSetConnectOption = NULL;
      lpSQLSetStmtOption    = NULL;
      lpSQLSpecialColumns   = NULL;
      lpSQLStatistics       = NULL;
      lpSQLTables           = NULL;
      lpSQLBrowseConnect    = NULL;
      lpSQLColumnPrivileges = NULL;
      lpSQLDataSources      = NULL;
      lpSQLDescribeParam    = NULL;
      lpSQLExtendedFetch    = NULL;
      lpSQLForeignKeys      = NULL;
      lpSQLMoreResults      = NULL;
      lpSQLNativeSql        = NULL;
      lpSQLNumParams        = NULL;
      lpSQLParamOptions     = NULL;
      lpSQLPrimaryKeys      = NULL;
      lpSQLProcedureColumns = NULL;
      lpSQLProcedures       = NULL;
      lpSQLSetPos           = NULL;
      lpSQLTablePrivileges  = NULL;
      lpSQLDrivers          = NULL;
      lpSQLBindParameter    = NULL;
      lpSQLSetParam         = NULL;
      lpSQLSetScrollOptions = NULL;
   }




               /** Ensure I load the font I'm gonna use! **/

//   if(GetModuleHandle(pDOSAPPFON) ||
//      MyOpenFile(pDOSAPPFON, (LPOFSTRUCT)&ofstr,
//                OF_READ | OF_EXIST)!=HFILE_ERROR)
//   {
//
//      if((hFontLib = LoadLibrary(pDOSAPPFON))>=HINSTANCE_ERROR)
//      {
//
//         MyAddFontResource(MAKELP(0,hFontLib));/* font file that has the 3.1 */
//                                     /* MS-DOS fonts in it (which I want!) */
//
//         BlockFontChangeMsg = TRUE;
//
//
//         SendMessage((HWND)0xffff, WM_FONTCHANGE, 0, 0);
//
//         BlockFontChangeMsg = FALSE;
//      }
//      else
//      {
//         hFontLib = NULL;
//      }
//   }




   // load font resource, and transfer contents to an appropriately
   // named font file, if it does not already exist...

   if(!GetModuleHandle("SFTSHFON"))
   {
    HRSRC hRsrc = FindResource(hInst, MAKEINTRESOURCE(SFTSHELL_FONT),
                               MAKEINTRESOURCE(FONTFILE));

      if(hRsrc)
      {
       HANDLE h1 = LoadResource(hInst, hRsrc);

         if(h1)
         {
            // create a temporary file with my fonts in them...

//            GetModuleFileName(hInst, szFontName, sizeof(szFontName));
//            lp1 = _fstrrchr(szFontName, '\\');
//            if(!lp1) lp1 = szFontName;
//            else     lp1++;

            GetSystemDirectory(szFontName, sizeof(szFontName));

            lp1 = szFontName + lstrlen(szFontName);
            if(!*szFontName || *(lp1 - 1) != '\\') *(lp1++) = '\\';

            lstrcpy(lp1, "SFTSHFON.FNR"); // font resource (essentially)

            if(MyOpenFile(szFontName, (LPOFSTRUCT)&ofstr,
                          OF_READWRITE | OF_EXIST | OF_SHARE_DENY_NONE)
               != HFILE_ERROR)
            {
               MySetFileAttr(szFontName, 0); // ensure I can write to it
            }


            lp1 = LockResource(h1);

            hFile = MyOpenFile(szFontName, (LPOFSTRUCT)&ofstr,
                               OF_READWRITE | OF_CREATE | OF_SHARE_EXCLUSIVE);

            if(hFile != HFILE_ERROR)
            {
               lstrcpy(szFontName, ofstr.szPathName);

               _lwrite(hFile, lp1, (UINT)SizeofResource(hInst, hRsrc));

               _lclose(hFile);


//               // mark font file as READ ONLY until further notice!
//
//               MySetFileAttr(szFontName, _A_RDONLY);

               if(MyAddFontResource(szFontName))
               {
                  BlockFontChangeMsg = TRUE;
                  SendMessage((HWND)0xffff, WM_FONTCHANGE, 0, 0);
                  BlockFontChangeMsg = FALSE;
               }
            }
            else
            {
               *szFontName = 0;
            }

            UnlockResource(h1);

            FreeResource(h1);

         }
      }
   }



   /*******************************************************************/
   /*         PROCESSING 'SFTSHELL' AS THE 'SYSTEM SHELL'             */
   /*******************************************************************/

   if(IsShell)
   {
    static const char CODE_BASED szSystemStartup[]="SystemStartup";

      if(lpsndPlaySound)
      {
         lpsndPlaySound(szSystemStartup, SND_ASYNC | SND_NOSTOP | SND_NODEFAULT);
      }
   }

   hbmpToolBar = LoadBitmap(hInstance, "TOOLBAR");

   if(IsChicago || IsNT)  // we're running chicago - get the dialog box background
   {
      hbrDlgBackground = CreateSolidBrush(GetSysColor(COLOR_3DLIGHT));
   }
   else           // not chicago - use 'button face' color
   {
      hbrDlgBackground = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
   }




                      /***------------------------***/
                      /*** CREATE THE MAIN WINDOW ***/
                      /***------------------------***/

   if(i = GetProfileInt(pPROGRAMNAME,pSCREENLINES,0))
   {
      SCREEN_LINES = i;
   }

   if(SCREEN_LINES > 400)  // max # of lines is actually 409, but...
   {
      SCREEN_LINES = 400;
   }


   lpScreen = GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                             (sizeof(screen[0][0]) + sizeof(attrib[0][0]))
                             * SCREEN_COLS * SCREEN_LINES);

   if(!lpScreen) return(FALSE);

   screen = (char __based(lpScreen) *)lpScreen;
   attrib = (char __based(lpScreen) *)
             (lpScreen + sizeof(screen[0][0]) * SCREEN_COLS * SCREEN_LINES);


   hMainWnd = CreateWindow(szMainWClass,"* Command Line Interpreter *",
                           WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL,
                           CW_USEDEFAULT,CW_USEDEFAULT,
                           CW_USEDEFAULT,CW_USEDEFAULT,NULL,NULL,
                           hInstance,NULL);

   if(!hMainWnd)
   {
#ifndef WIN32

      if(hToolHelp)  FreeLibrary(hToolHelp);
      if(hMMSystem)  FreeLibrary(hMMSystem);
      if(hShell)     FreeLibrary(hShell);
      if(hDdeml)     FreeLibrary(hDdeml);

      hToolHelp = hMMSystem = hShell = hDdeml = NULL;

#endif // WIN32

      GlobalFreePtr(lpScreen);
      lpScreen = NULL;

      return (FALSE);
   }


   hAccel  = LoadAccelerators(hInstance, szMAINACCEL);
   RegisterAccelerators(hAccel, hMainWnd);


   ClearScreen();  /* ensures the slate is 'wiped clean' prior to showing */

   PrintString(pVersionString);


   DisplayPrompt();


   ShowWindow(hMainWnd, nCmdShow);
   UpdateWindow(hMainWnd);

#if defined(DROP_DEAD_DATE) || defined(DEMO)

   {
    SFTDATE dt;
    long dz;
    DWORD dwDropDeadDate, dwDropDeadDateCRC;
    char DropDeadDateBuf[32];
    char DropDeadMsgBuf[256];
    BOOL bEntryFlag = FALSE;

#ifdef DEMO

     GetProfileString(pPROGRAMNAME, szDEMO, "", DropDeadDateBuf, sizeof(DropDeadDateBuf));

     if(!*DropDeadDateBuf) // user needs to enter an authorization code
     {
      DLGPROC pDlg;

enter_auth_code:  // a label - I cheated, oh, well


        pDlg = (DLGPROC)MakeProcInstance(AuthCodeDialog, hInst);
        if(!pDlg)
        {
           MessageBox(hMainWnd, "Internal error - terminating", "SFTShell DEMO",
                      MB_OK | MB_ICONHAND);

           goto bailout_point;
        }

        *DropDeadDateBuf = 0;
        DialogBoxParam(hInst, "AUTHCODE_DIALOG", hMainWnd, pDlg,
                       (LPARAM)(LPSTR)DropDeadDateBuf);

        FreeProcInstance(pDlg);

        _fstrtrim(DropDeadDateBuf);
        if(!*DropDeadDateBuf)
        {
           goto bailout_point;
        }

        bEntryFlag = TRUE;  // this says "I entered an authorization code"
     }

     dwDropDeadDate = 0;
     dwDropDeadDateCRC = 0;

     // code must be 8 digits in length, and have valid XOR mask.
     // XOR mask is 0x96a53c5a
     // 1st 2 digits is 2 MSB of date
     // next 4 digits is mask
     // last 2 digits is 2 LSB of date
     // method uses 'RAD34' method (excluding the 'O')

     _fstrtrim(DropDeadDateBuf);
     if(lstrlen(DropDeadDateBuf)!=14)
     {
auth_code_invalid:

        if(MessageBox(hMainWnd, "Authorization code is invalid!",
                      "* SFTSHELL ERROR *", MB_OKCANCEL | MB_ICONHAND)
           == IDCANCEL)
        {
           goto bailout_point;
        }
        else
        {
           goto enter_auth_code;
        }
     }


     for(i=0; i < 8; i++)
     {
        if(Rad34DigitValue(DropDeadDateBuf[i]) < 0)
        {
           goto auth_code_invalid;
        }
     }

     dwDropDeadDate = Rad34DigitValue(DropDeadDateBuf[0]) * 1544804416L
                    + Rad34DigitValue(DropDeadDateBuf[1]) * 45435424L
                    + Rad34DigitValue(DropDeadDateBuf[2]) * 1336336L
                    + Rad34DigitValue(DropDeadDateBuf[10]) * 39304L
                    + Rad34DigitValue(DropDeadDateBuf[11]) * 1156L
                    + Rad34DigitValue(DropDeadDateBuf[12]) * 34L
                    + Rad34DigitValue(DropDeadDateBuf[13]);

     dwDropDeadDateCRC = Rad34DigitValue(DropDeadDateBuf[3]) * 1544804416L
                       + Rad34DigitValue(DropDeadDateBuf[4]) * 45435424L
                       + Rad34DigitValue(DropDeadDateBuf[5]) * 1336336L
                       + Rad34DigitValue(DropDeadDateBuf[6]) * 39304L
                       + Rad34DigitValue(DropDeadDateBuf[7]) * 1156L
                       + Rad34DigitValue(DropDeadDateBuf[8]) * 34L
                       + Rad34DigitValue(DropDeadDateBuf[9]);

     // monkey with the bits and bytes for 'dwDropDeadDate'
     _asm
     {
        mov al, BYTE PTR dwDropDeadDate
        mov ah, BYTE PTR dwDropDeadDate + 1
        mov dl, BYTE PTR dwDropDeadDate + 2
        mov dh, BYTE PTR dwDropDeadDate + 3

        rol al, 3
        ror ah, 2
        rol dl, 4
        ror dh, 1

        mov BYTE PTR dwDropDeadDate, dh
        mov BYTE PTR dwDropDeadDate + 1, dl
        mov BYTE PTR dwDropDeadDate + 2, al
        mov BYTE PTR dwDropDeadDate + 3, ah
     }

     if((dwDropDeadDate & 0xfffffL) !=
        ((dwDropDeadDateCRC ^ 0x96a53c5aL) & 0xfffffL))
     {
        goto auth_code_invalid;
     }



#else // DEMO

      dwDropDeadDate = DROP_DEAD_DATE;

#endif // DROP_DEAD_DATE

      GetSystemDateTime(&dt, NULL);  // today's date...

      dz = days(&dt);  // 'days' for today's date


      if(dz > (dwDropDeadDate - DROP_DEAD_WARNING_PERIOD) &&
         dz <= dwDropDeadDate)
      {

       static const char CODE_BASED szTitle[]=
                       "* SFTSHELL EVALUATION PERIOD EXPIRES SOON *";
       static const char CODE_BASED szMsg1[]=
         "You have only %ld day(s) left for the SFTShell evaluation period!\n"
         "Please obtain a release version of SFTShell as soon as possible.";


         wsprintf(DropDeadMsgBuf, szMsg1, (dwDropDeadDate - dz + 1));

         MessageBox(hMainWnd, DropDeadMsgBuf, szTitle,
                    MB_OK | MB_ICONEXCLAMATION);
      }
      else if(dz > dwDropDeadDate)
      {
       static const char CODE_BASED szTitle[]=
                       "* SFTSHELL EVALUATION PERIOD HAS EXPIRED *";
#ifdef DEMO
       static const char CODE_BASED szMsg2[]=
         "The evaluation period has expired; this program will no\n"
         "longer function.  You must re-enter a valid authorization\n"
         "code, or else obtain the release version of SFTShell if\n"
         "you wish to continue using this application.";

         if(MessageBox(hMainWnd, szMsg2, szTitle, MB_OKCANCEL | MB_ICONHAND)
            == IDOK)
         {
            goto enter_auth_code;  // only for DEMO version!
         }

#else  // DEMO
       static const char CODE_BASED szMsg2[]=
         "The evaluation period has expired; this program will no\n"
         "longer function.  You must obtain the release version of\n"
         "SFTShell if you wish to continue using this application.";

         MessageBox(hMainWnd, szMsg2, szTitle, MB_OK | MB_ICONHAND);

#endif // DEMO


bailout_point: // a label - ~sigh~

         DestroyWindow(hMainWnd);
         hMainWnd = NULL;

#ifndef WIN32

         if(hToolHelp)  FreeLibrary(hToolHelp);
         if(hMMSystem)  FreeLibrary(hMMSystem);
         if(hShell)     FreeLibrary(hShell);
         if(hDdeml)     FreeLibrary(hDdeml);

         hToolHelp = hMMSystem = hShell = hDdeml = NULL;

#endif // WIN32

         GlobalFreePtr(lpScreen);
         lpScreen = NULL;

         return (FALSE);
      }


#ifdef DEMO
      if(bEntryFlag)  // did use enter a new code??
      {
         WriteProfileString(pPROGRAMNAME, szDEMO, DropDeadDateBuf);
            // save the authorization code in WIN.INI
      }

#endif // DEMO

   }
#endif // defined(DROP_DEAD_DATE) || defined(DEMO)


   SetEnvString(szCOPYING,szFALSE);  /* for batch information */

   if(lpDdeInitialize) // I have successfully loaded the DDEML library
   {
      SetEnvString(pDDESERVERLIST, "");
      SetEnvString(pDDESERVERTOPICLIST, "");  // ensure these two strings
                                              // are REMOVED if DDE is valid


      (FARPROC)lpDdeCallBack = MakeProcInstance((FARPROC)MyDdeCallBack,
                                                   hInst);
      if(!lpDdeCallBack ||
         lpDdeInitialize(&idDDEInst, lpDdeCallBack,
                         CBF_FAIL_SELFCONNECTIONS | APPCLASS_STANDARD, 0L)
         !=DMLERR_NO_ERROR)
      {
         idDDEInst = 0;      /* flags that DDE is not available */
      }
      else
      {             /** REGISTER THE 'DEFAULT' SERVER NOW! **/
       HSZ hsz1;

         hsz1 = lpDdeCreateStringHandle(idDDEInst, pDEFSERVERNAME,
                                        CP_WINANSI);


         lpDdeNameService(idDDEInst, hsz1, 0, DNS_REGISTER);
      }
   }



   /*******************************************************************/
   /*         PROCESSING 'SFTSHELL' AS THE 'SYSTEM SHELL'             */
   /*******************************************************************/

   if(IsShell)
   {
    static const char CODE_BASED szAppName[]= "windows";
    static const char CODE_BASED szRun[]    = "run";
    static const char CODE_BASED szLoad[]   = "load";
    static const char CODE_BASED szBlank[]  = "";


       /** Here is where I process the 'run=' & 'load=' in WIN.INI **/

      GetProfileString(szAppName, szRun, szBlank, tbuf, sizeof(tbuf));
      lp4 = tbuf + lstrlen(tbuf) + 1;

      lp1=tbuf;
      while(*lp1)
      {
         lp2 = _fstrchr(lp1, ',');
         if(!lp2) lp2 = lp1 + lstrlen(lp1);
         else     *(lp2++) = 0;

         // FIND COMMAND ARGS, AND COPY TO 'lp4' (at end of 'tbuf')

         lp3 = lp1;
         while(*lp3>' ' && *lp3!='/') lp3++;
         if(*lp3)
         {
            lstrcpy(lp4, lp3);

            *lp3 = 0;

            lp3 = lp4;
         }


         CMDRunProgram(lp1,lp3,SW_SHOWNORMAL);

         lp1 = lp2;
      }

      GetProfileString(szAppName, szLoad, szBlank, tbuf, sizeof(tbuf));

      lp1=tbuf;
      while(*lp1)
      {
         lp2 = _fstrchr(lp1, ',');
         if(!lp2) lp2 = lp1 + lstrlen(lp1);
         else     *(lp2++) = 0;

         // FIND COMMAND ARGS, AND COPY TO 'lp4' (at end of 'tbuf')

         lp3 = lp1;
         while(*lp3>' ' && *lp3!='/') lp3++;
         if(*lp3)
         {
            lstrcpy(lp4, lp3);

            *lp3 = 0;

            lp3 = lp4;
         }

         CMDRunProgram(lp1,lp3,SW_SHOWMINIMIZED);

         lp1 = lp2;
      }

   }



   return (TRUE);
}





/***************************************************************************/
/*                                                                         */
/*               SUBCLASS PROCS FOR STANDARD WINDOW CLASSES                */
/*                                                                         */
/***************************************************************************/



#pragma code_seg("STDWNDSUBCLASS_TEXT","CODE")

LRESULT CALLBACK EXPORT SubclassStaticWindowProc(HWND hWnd, UINT msg,
                                                 WPARAM wParam, LPARAM lParam)
{
RECT rct;
//PAINTSTRUCT ps;
//long ws;
//WORD wMode;
//HDC hDC;
//UINT uiFormat;
//char tbuf[512];


   switch(msg)
   {

      case WM_ERASEBKGND:

         {
          HDC hdcBackGround = (HDC)wParam;

            GetClientRect(hWnd, &rct);

            FillRect(hdcBackGround, &rct, hbrDlgBackground);
         }

         return(TRUE);
   }

   return(CallWindowProc(lpStaticWndProc, hWnd, msg, wParam, lParam));
}


BOOL CALLBACK EXPORT SubclassStaticEnumCallback(HWND hWnd, LPARAM lParam)
{
char tbuf[64];

   GetClassName(hWnd, tbuf, sizeof(tbuf));

   if(!_fstricmp(tbuf, "STATIC"))
   {
      SetWindowLong(hWnd, GWL_WNDPROC, (LPARAM)SubclassStaticWindowProc);
   }

   return(TRUE);  // continue enumeration
}


// this next proc is called ONCE for each dialog on 'WM_CREATE'

void FAR PASCAL SubclassStaticControls(HWND hDlg)
{
WNDENUMPROC lpProc;


   if(!lpStaticWndProc) return;  // don't do it if no window proc!

   lpProc = (WNDENUMPROC)
               MakeProcInstance((FARPROC)SubclassStaticEnumCallback, hInst);

   EnumChildWindows(hDlg, lpProc, 0);

   FreeProcInstance((FARPROC)lpProc);
}




LRESULT CALLBACK EXPORT SubclassEditWindowProc(HWND hWnd, UINT msg,
                                               WPARAM wParam, LPARAM lParam)
{
LRESULT rval;
long lSelect;


   switch(msg)
   {
      case WM_SETFOCUS:

         lSelect = CallWindowProc(lpEditWndProc, hWnd, EM_GETSEL, 0, 0);

         rval = CallWindowProc(lpEditWndProc, hWnd, msg, wParam, lParam);

         CallWindowProc(lpEditWndProc, hWnd, EM_SETSEL, 0, (LPARAM)lSelect);

         return(rval);


//      case WM_GETDLGCODE:
//
//         return(CallWindowProc(lpEditWndProc, hWnd, msg, wParam, lParam)
//                | DLGC_WANTALLKEYS);
//
   }

   return(CallWindowProc(lpEditWndProc, hWnd, msg, wParam, lParam));
}



BOOL CALLBACK EXPORT SubclassEditEnumCallback(HWND hWnd, LPARAM lParam)
{
char tbuf[64];

   GetClassName(hWnd, tbuf, sizeof(tbuf));

   if(!_fstricmp(tbuf, "EDIT"))
   {
      SetWindowLong(hWnd, GWL_WNDPROC, (LPARAM)SubclassEditWindowProc);
   }

   return(TRUE);  // continue enumeration
}


// this next proc is called ONCE for each dialog on 'WM_CREATE'

void FAR PASCAL SubclassEditControls(HWND hDlg)
{
WNDENUMPROC lpProc;


   if(!lpEditWndProc) return;  // don't do it if no window proc!

   lpProc = (WNDENUMPROC)
               MakeProcInstance((FARPROC)SubclassEditEnumCallback, hInst);

   EnumChildWindows(hDlg, lpProc, 0);

   FreeProcInstance((FARPROC)lpProc);
}







HBRUSH FAR PASCAL DlgOn_WMCtlColor(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
HDC hdcChild = (HDC)wParam;
HWND hwndChild = (HWND)LOWORD(lParam);
int nCtlType = (int)HIWORD(lParam);



   if(nCtlType == CTLCOLOR_STATIC)
   {
      SetTextColor(hdcChild, RGB(0,0,0));  // always black text

      // get the correct shade of grey for the background text...

      if(IsChicago || IsNT)
         SetBkColor(hdcChild, GetSysColor(COLOR_3DLIGHT));
      else
         SetBkColor(hdcChild, GetSysColor(COLOR_BTNFACE));

      return(hbrDlgBackground);
   }
   else if(nCtlType == CTLCOLOR_BTN)
   {
      // NOTE:  PUSHBUTTON controls aren't affected

      SetTextColor(hdcChild, RGB(0,0,0));  // always black text

      // get the correct shade of grey for the background text...

      if(IsChicago || IsNT)
         SetBkColor(hdcChild, GetSysColor(COLOR_3DLIGHT));
      else
         SetBkColor(hdcChild, GetSysColor(COLOR_BTNFACE));

      return(hbrDlgBackground);
   }

   return(NULL);  // by default, do nothing
}



#pragma code_seg("WINMAIN_TEXT","CODE")


/***************************************************************************/
/*                                                                         */
/*       OPERATIONAL CODE - THE REMAINING STUFF EXECUTES AT RUN TIME       */
/*                                                                         */
/***************************************************************************/



void FAR PASCAL UpdateWindowStateFlag(void)
{
   if(!hMainWnd || !IsWindow(hMainWnd)) return;

   if(IsZoomed(hMainWnd))
   {
      if(WindowState!=1)
      {
         SetEnvString(szWINDOW_STATE,szMAXIMIZED);
         WindowState=1;
      }
   }
   else if(IsIconic(hMainWnd))
   {
      if(WindowState!=-1)
      {
         SetEnvString(szWINDOW_STATE,szMINIMIZED);
         WindowState=-1;
      }
   }
   else
   {
      if(WindowState)
      {
         SetEnvString(szWINDOW_STATE,szNORMAL);
         WindowState=0;
      }
   }
}


void FAR PASCAL UpdateStatusBar(void)
{
static BOOL old_copying=0, old_formatting=0, old_BatchMode=0,
            old_waiting_flag=0, old_IsCall=0, old_GettingUserInput=0,
            old_wFormatPercentComplete=0, old_wCopyPercentComplete=0,
            old_FormatSuspended=0, old_CopySuspended=0,
            WasCommand=0;
static BOOL wNCopyFiles=0, wCurCopyFile=0, wFormatDrive=0xffff;
static BOOL bStatusBarVisible = 0;
static LPFORMAT_REQUEST old_lpFormatRequest = NULL;
static LPCOPY_INFO old_lpCopyInfo = NULL;
BOOL bUpdate, bUpdateCopy, bUpdateFormat;
LPCOPY_INFO lpCI;
WORD wNFiles, wCurFile;




   if(!hdlgQueue && !ShowStatusBar)     // not showing - assign DEFAULTS!
   {
      old_copying=0;
      old_formatting=0;
      old_BatchMode=0;
      old_waiting_flag=0;
      old_IsCall=0;
      old_GettingUserInput=0;
      old_wFormatPercentComplete=0;
      old_wCopyPercentComplete=0;
      WasCommand = 0;

      old_lpFormatRequest = NULL;
      old_lpCopyInfo      = NULL;

      wNCopyFiles=0;
      wCurCopyFile=0;
      wFormatDrive=0xffff;

      bStatusBarVisible = 0;

      return;
   }



   bUpdate       = FALSE;
   bUpdateCopy   = FALSE;
   bUpdateFormat = FALSE;


   // See if any of the status flags have changed, and if so update
   // the appropriate pane within the statups bar

   if(copying != old_copying || old_CopySuspended != CopySuspended
      || old_lpCopyInfo != lpCopyInfo
      || bStatusBarVisible != ShowStatusBar)
   {
      old_copying = copying;
      old_CopySuspended = CopySuspended;

      old_lpCopyInfo = lpCopyInfo;

      bUpdateCopy = TRUE;
   }

   if(formatting != old_formatting || old_FormatSuspended != FormatSuspended
      || old_lpFormatRequest != lpFormatRequest
      || bStatusBarVisible != ShowStatusBar)
   {
      old_formatting = formatting;
      old_FormatSuspended = FormatSuspended;

      old_lpFormatRequest = lpFormatRequest;

      bUpdateFormat = TRUE;
   }


   if(BatchMode != old_BatchMode || waiting_flag != old_waiting_flag ||
      IsCall    != old_IsCall    ||
      (lpCurCmd && !WasCommand)  || (!lpCurCmd && WasCommand)        ||
      old_GettingUserInput != GettingUserInput                       ||
      bStatusBarVisible != ShowStatusBar)
   {
      old_BatchMode    = BatchMode;
      old_waiting_flag = waiting_flag;
      old_IsCall       = IsCall;

      WasCommand       = lpCurCmd != NULL;

      old_GettingUserInput = GettingUserInput;

      bUpdate = TRUE;
   }

   if(copying)
   {
      wNFiles = 0;
      wCurFile = 0;

      if(lpCI = lpCopyInfo)
      {
         wNFiles = lpCI->wNFiles;
         wCurFile = lpCI->wCurFile;

         lpCI = lpCI->lpNext;
      }
      while(lpCI)
      {
         wNFiles += lpCI->wNFiles;

         lpCI = lpCI->lpNext;
      }

      if(wNFiles != wNCopyFiles || wCurFile != wCurCopyFile ||
         old_wCopyPercentComplete != wCopyPercentComplete)
      {
         wNCopyFiles = wNFiles;
         wCurCopyFile = wCurFile;

         old_wCopyPercentComplete = wCopyPercentComplete;

         bUpdateCopy = TRUE;
      }
   }
   else
   {
      wNCopyFiles = 0;
      wCurCopyFile = 0;

      old_wCopyPercentComplete = 0;
   }


   if(formatting)
   {
      if(lpFormatRequest)
      {
         if(wFormatDrive != lpFormatRequest->wDrive ||
            old_wFormatPercentComplete != wFormatPercentComplete)
         {
            bUpdateFormat = TRUE;

            wFormatDrive = lpFormatRequest->wDrive;
            old_wFormatPercentComplete = wFormatPercentComplete;
         }
      }
      else
      {
         wFormatDrive = 0xffff;
      }
   }
   else
   {
      wFormatDrive = 0xffff;
      old_wFormatPercentComplete = 0;
   }


   // indicate 'change' status to QUEUE dialog (if applicable)

   if(hdlgQueue)
   {
      if(bUpdateCopy || bUpdateFormat)
      {
         SendMessage(hdlgQueue, WM_COMMAND,
                     (WPARAM)0x4321, (LPARAM)0x12345678);
      }
   }


   // each of those status bar panes which changed must be updated here...

   if(ShowStatusBar)
   {
      if(bStatusBarVisible)   // I've painted it at least once already...
      {
         if(bUpdateCopy)   PaintStatusBarPane(hMainWnd, 1);
         if(bUpdateFormat) PaintStatusBarPane(hMainWnd, 2);
         if(bUpdate)       PaintStatusBarPane(hMainWnd, 3);
      }
      else
      {
         SendMessage(hMainWnd, WM_NCPAINT, 0, 0);  // paint it all!

         bStatusBarVisible = 1;  // next time I only paint the changes
      }
   }
   else
   {
      bStatusBarVisible = 0;  // next time, paint the whole thing
   }


}



/***************************************************************************/
/*                                                                         */
/*               UTILITIES AND OTHER 'Main' SPECIFIC STUFF                 */
/*                                                                         */
/***************************************************************************/


                       /*** CONTROL-BREAK HANDLER ***/

//#pragma alloc_text (MyCtrlBreak_TEXT,MyCtrlBreak)
//
//void _interrupt _far MyCtrlBreak(WORD _es, WORD _ds, WORD _di,
//                                 WORD _si, WORD _bp, WORD _sp,
//                                 WORD _bx, WORD _dx, WORD _cx,
//                                 WORD _ax, WORD _ip, WORD _cs,
//                                 WORD _flags )
//{
//
//    ctrl_break = TRUE;
//
//}




/***************************************************************************/
/*                                                                         */
/*  Main Window Processing - Callback plus message 'Cracker' functions.    */
/*                                                                         */
/***************************************************************************/

#pragma code_seg("MAIN_WINDOW_TEXT","CODE")


HCURSOR FAR PASCAL MySetCursor(HCURSOR hCursor)
{
HCURSOR hRval;
POINT pt;


   if(!hMainCursor)
   {
#ifdef WIN32
      hMainCursor = (HCURSOR)GetClassLong(hMainWnd,GCL_HCURSOR);
#else  // WIN32
      hMainCursor = (HCURSOR)GetClassWord(hMainWnd,GCW_HCURSOR);
#endif // WIN32
   }

   hRval = hMainCursor;

   if(hCursor) hMainCursor = hCursor;
#ifdef WIN32
   else        hMainCursor = (HCURSOR)GetClassLong(hMainWnd,GCL_HCURSOR);
#else  // WIN32
   else        hMainCursor = (HCURSOR)GetClassWord(hMainWnd,GCW_HCURSOR);
#endif // WIN32


   // check mouse position - see if it's within my window's client area
   // and if it is, change the cursor!

   GetCursorPos(&pt);

   if(SendMessage(hMainWnd, WM_NCHITTEST, 0, MAKELPARAM(pt.x,pt.y))
      == HTCLIENT)
   {
      SetCursor(hMainCursor);
   }

   return(hRval);
}


void FAR PASCAL UpdateTextMetrics(HWND hWnd)
{
HDC hDC;
HFONT hOldFont;



   if(!TMFlag)   /* if we haven't gotten the TEXT METRICS yet... */
   {
      hDC = GetDC(hWnd);

      if(hMainFont)
      {
         hOldFont = SelectObject(hDC, hMainFont);
      }
      else
      {
         hOldFont = SelectObject(hDC, DEFAULT_FONT);
      }

      if(GetTextMetrics(hDC, (LPTEXTMETRIC)&tmMain))
      {
         TMFlag = TRUE;
      }

      SelectObject(hDC, hOldFont);

      ReleaseDC(hWnd, hDC);
   }

}


LRESULT CALLBACK EXPORT MainWndProc(HWND hWnd, UINT message, WPARAM wParam,
                                    LPARAM lParam)
{
LRESULT rval;
RECT r;
WORD wNItems, w;
int hF, wF;
char tbuf[80];
static HWND hwndNextClipboardViewer = NULL;



   // special 'pre-crunching' for tool bar hint window

   if(message == WM_KEYDOWN    || message == WM_KEYUP ||
      message == WM_SYSKEYDOWN || message == WM_SYSKEYUP ||
      message == WM_MOUSEMOVE  || message == WM_CANCELMODE ||
      message == WM_SETFOCUS   || message == WM_KILLFOCUS ||
      message == WM_ACTIVATE   || message == WM_NCACTIVATE ||
      message == WM_NCHITTEST)
   {
#ifdef WIN32
      POINT pt0;
      pt0.x=LOWORD(lParam);
      pt0.y=HIWORD(lParam);
#endif // WIN32

      if(message == WM_NCHITTEST) // in this case, *ONLY* if point is outside
      {                           // of the window rectangle

         GetWindowRect(hWnd, &r);
      }


      // WM_NCHITTEST must be outside of window to continue this process

#ifdef WIN32
      if(message != WM_NCHITTEST || !PtInRect(&r, pt0))
#else  // WIN32
      if(message != WM_NCHITTEST || !PtInRect(&r, MAKEPOINT(lParam)))
#endif // WIN32
      {
         // get rid of any 'tool bar hint' stuff

         if(iToolBarTimer)
         {
            if((iToolBarTimer & 0xffc0)==TOOLBAR_TIMER)
               KillTimer(hWnd, iToolBarTimer);

            iToolBarTimer = 0;
         }

         if(hwndToolBarHint)
         {
            DestroyWindow(hwndToolBarHint);
            hwndToolBarHint = NULL;
         }
      }
   }


   switch (message)
   {
      case WM_NCCREATE:

         // Place items here that affect the NON-CLIENT area, how it is
         // displayed, etc. etc.

         ShowStatusBar = GetProfileInt(pPROGRAMNAME,pSHOWSTATUSBAR,1);
         ShowToolBar   = GetProfileInt(pPROGRAMNAME,pSHOWTOOLBAR,1);

         break;


      case WM_CREATE:

         if(lpDragAcceptFiles)
         {
            lpDragAcceptFiles(hWnd, TRUE);
         }

         CheckMenuItem(GetMenu(hWnd), IDM_STATUS_BAR,
               (ShowStatusBar ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);

         CheckMenuItem(GetMenu(hWnd), IDM_TOOL_BAR,
               (ShowToolBar ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);


         hF = GetProfileInt(pPROGRAMNAME,pFONTHEIGHT,0x7fff);
         wF = GetProfileInt(pPROGRAMNAME,pFONTWIDTH,0x7fff);
         GetProfileString(pPROGRAMNAME,pFONTNAME,"",tbuf,sizeof(tbuf));

         if(!*tbuf)           // internal fonts
         {
            if(wF==5 && hF==7)
            {
               MainOnCommand(hWnd, IDM_FONT7x5, 0, 0);
            }
            else if(wF==6 && hF==8)
            {
               MainOnCommand(hWnd, IDM_FONT8x6, 0, 0);
            }
            else if(wF==7 && hF==9)
            {
               MainOnCommand(hWnd, IDM_FONT9x7, 0, 0);
            }
            else if(wF==8 && hF==12)
            {
               MainOnCommand(hWnd, IDM_FONT12x8, 0, 0);
            }
            else if(wF==12 && hF==16)
            {
               MainOnCommand(hWnd, IDM_FONT16x12, 0, 0);
            }
            else if(wF==18 && hF==24)
            {
               MainOnCommand(hWnd, IDM_FONT24x18, 0, 0);
            }
            else if(wF==24 && hF==32)
            {
               MainOnCommand(hWnd, IDM_FONT32x24, 0, 0);
            }
            else if(wF==36 && hF==48)
            {
               MainOnCommand(hWnd, IDM_FONT48x36, 0, 0);
            }
            else
            {
               MainOnCommand(hWnd, IDM_FONT_NORMAL, 0, 0);
            }
         }
         else   // CUSTOM FONT WAS LAST USED!
         {
          int iWeight     = GetProfileInt(pPROGRAMNAME, pFONTWEIGHT, FW_NORMAL);
          BYTE bItalic    = (BYTE)GetProfileInt(pPROGRAMNAME, pFONTITALIC, 0);
          BYTE bUnderline = (BYTE)GetProfileInt(pPROGRAMNAME, pFONTUNDERLINE, 0);
          BYTE bStrikeout = (BYTE)GetProfileInt(pPROGRAMNAME, pFONTSTRIKEOUT, 0);



            hMainFont = CreateFont(hF, wF, 0, 0,
                                   iWeight, bItalic, bUnderline, bStrikeout,
                                   DEFAULT_CHARSET,
                                   OUT_DEFAULT_PRECIS,
                                   CLIP_DEFAULT_PRECIS,
                                   DEFAULT_QUALITY,
                                   FIXED_PITCH | FF_DONTCARE,
                                   tbuf);

            CheckMenuItem(GetMenu(hWnd), IDM_FONT_MENU,
                          MF_CHECKED | MF_BYCOMMAND);

            CheckMenuItem(GetMenu(hWnd), IDM_FONT7x5,
                          MF_UNCHECKED | MF_BYCOMMAND);
            CheckMenuItem(GetMenu(hWnd), IDM_FONT8x6,
                          MF_UNCHECKED | MF_BYCOMMAND);
            CheckMenuItem(GetMenu(hWnd), IDM_FONT9x7,
                          MF_UNCHECKED | MF_BYCOMMAND);
            CheckMenuItem(GetMenu(hWnd), IDM_FONT12x8,
                          MF_UNCHECKED | MF_BYCOMMAND);
            CheckMenuItem(GetMenu(hWnd), IDM_FONT16x12,
                          MF_UNCHECKED | MF_BYCOMMAND);
            CheckMenuItem(GetMenu(hWnd), IDM_FONT24x18,
                          MF_UNCHECKED | MF_BYCOMMAND);
            CheckMenuItem(GetMenu(hWnd), IDM_FONT32x24,
                          MF_UNCHECKED | MF_BYCOMMAND);
            CheckMenuItem(GetMenu(hWnd), IDM_FONT48x36,
                          MF_UNCHECKED | MF_BYCOMMAND);
            CheckMenuItem(GetMenu(hWnd), IDM_FONT_NORMAL,
                          MF_UNCHECKED | MF_BYCOMMAND);

            TMFlag = FALSE;
            UpdateTextMetrics(hWnd);  // do this NOW (necessary)

            // to ensure the window will be properly painted, 'move' it

            GetWindowRect(hWnd, &r);

            MoveWindow(hWnd, r.left, r.top, r.right - r.left,
                       r.bottom - r.top, FALSE);
         }


         // disable the 'copy command history' menu under 'EDIT'
         // and the 'erase command history' as well...

         EnableMenuItem(GetMenu(hWnd), IDM_COPY_CMDLINE,
                        MF_GRAYED | MF_BYCOMMAND);

         EnableToolBarButton(IDM_COPY_CMDLINE, 0);


         EnableMenuItem(GetMenu(hWnd), IDM_ERASEHISTORY,
                        MF_GRAYED | MF_BYCOMMAND);

         EnableToolBarButton(IDM_ERASEHISTORY, 0);


         // check the clipboard status and enable/disable pasting

         if(IsClipboardFormatAvailable(CF_TEXT) ||
            IsClipboardFormatAvailable(CF_OEMTEXT))
         {
            EnableMenuItem(GetMenu(hWnd), IDM_PASTE_MENU,
                           MF_ENABLED | MF_BYCOMMAND);

            EnableToolBarButton(IDM_PASTE_MENU, 1);
         }
         else
         {
            EnableMenuItem(GetMenu(hWnd), IDM_PASTE_MENU,
                           MF_GRAYED | MF_BYCOMMAND);

            EnableToolBarButton(IDM_PASTE_MENU, 0);
         }


         // see if 'hFontLib' is available, and if not disable the
         // appropriate font choices

//         if(!hFontLib)  // no extra fonts?  Disable certain menu choices
//         {
//            EnableMenuItem(GetMenu(hWnd), IDM_FONT8x6,
//                           MF_GRAYED | MF_BYCOMMAND);
//
//            EnableMenuItem(GetMenu(hWnd), IDM_FONT7x5,
//                           MF_GRAYED | MF_BYCOMMAND);
//
//            EnableMenuItem(GetMenu(hWnd), IDM_FONT16x12,
//                           MF_GRAYED | MF_BYCOMMAND);
//         }


         // FINALLY, set me up as a clipboard viewer in the chain

         hwndNextClipboardViewer = SetClipboardViewer(hWnd);

         break;




      case WM_DROPFILES:

         if(!lpDragQueryFile || !lpDragFinish) break;

         wNItems = lpDragQueryFile((HANDLE)wParam, 0xffff, work_buf,
                                   sizeof(work_buf));
         if(wNItems>1)
         {
          static const char CODE_BASED pMsg[]=
                             "?Cannot (yet) drop multiple files on SFTSHELL";

             LockCode();

             MessageBox(hWnd, pMsg, pWINCMDERROR, MB_OK | MB_ICONHAND);

             UnlockCode();

             return(0);
         }

         lpDragQueryFile((HANDLE)wParam, 0, work_buf, sizeof(work_buf));


         if(!BatchMode || GettingUserInput)
         {
                    /* place file name into command buffer */

            DosQualifyPath(cmd_buf, work_buf);
            cmd_buf_len = lstrlen(cmd_buf);

            PrintString(cmd_buf);  /* make sure the thing ECHO's */

                        /* simulate pressing <ENTER> */
            MainOnChar(hWnd, '\r', 1);
         }
         else      /* otherwise, place into 'TypeAhead' buffer */
         {
            DosQualifyPath(work_buf, work_buf);
            lstrcat(work_buf, "\r");

            wNItems = lstrlen(work_buf);

            for(w=0; w<wNItems; w++)
            {
               if(TypeAheadTail>=sizeof(TypeAhead)) TypeAheadTail = 0;

               if((TypeAheadTail + 1)<TypeAheadHead ||
                  TypeAheadTail < sizeof(TypeAhead))
               {
                  TypeAhead[TypeAheadTail++] = (char)work_buf[w];

                  if(TypeAheadTail>=sizeof(TypeAhead)) TypeAheadTail = 0;
               }
               else
               {
                static const char CODE_BASED pMsg[]=
                        "?File name corrupted - Type Ahead Buffer is FULL";

                  LockCode();

                  MessageBox(hWnd, pMsg, pWINCMDERROR, MB_OK | MB_ICONHAND);

                  UnlockCode();

                  break;
               }
            }
         }

         lpDragFinish((HANDLE)wParam);

         return(0);


      case WM_FONTCHANGE:
//         if(BlockFontChangeMsg || !hFontLib) break;
//
//         MyAddFontResource(MAKELP(0,hFontLib));

         break;


      case WM_WININICHANGE:
         WinIniChange();    // inform 'international' (date/time/etc.)
                            // conversion functions of the change
         break;


      case WM_GETMINMAXINFO:


         if(!lParam) break;

         if(!TMFlag) UpdateTextMetrics(hWnd);

         ((MINMAXINFO FAR *)lParam)->ptMaxSize.x = (int)
                            min(GetSystemMetrics(SM_CXSCREEN) +
                                2L * GetSystemMetrics(SM_CXFRAME),
                                tmMain.tmAveCharWidth * 80L +
                                2L * GetSystemMetrics(SM_CXFRAME) +
//                                2 * GetSystemMetrics(SM_CXBORDER) +
                                3 * GetSystemMetrics(SM_CXHTHUMB) / 2);

         ((MINMAXINFO FAR *)lParam)->ptMaxPosition.x =
                        ((int)GetSystemMetrics(SM_CXSCREEN) -
                         (int)((MINMAXINFO FAR *)lParam)->ptMaxSize.x) / 2;

         ((MINMAXINFO FAR *)lParam)->ptMaxTrackSize.x = (int)
                              min(((MINMAXINFO FAR *)lParam)->ptMaxSize.x,
                                  tmMain.tmAveCharWidth * 80L +
                                  2 * GetSystemMetrics(SM_CXFRAME) +
//                                  2 * GetSystemMetrics(SM_CXBORDER) +
                                  3 * GetSystemMetrics(SM_CXHTHUMB) / 2);

         // 3/4 of the screen width and 1/2 the screen height is the
         // MINIMUM tracking size...
         // (if you want it smaller, use a smaller font!!)

         ((MINMAXINFO FAR *)lParam)->ptMinTrackSize.x =
                         min(((MINMAXINFO FAR *)lParam)->ptMaxTrackSize.x,
                             tmMain.tmAveCharWidth * 53 +
                             2 * GetSystemMetrics(SM_CXFRAME));

         ((MINMAXINFO FAR *)lParam)->ptMinTrackSize.y = (int)
                            (GetSystemMetrics(SM_CYSCREEN) / 2);

         return(0);


      case WM_NCCALCSIZE:

         if(!TMFlag) UpdateTextMetrics(hWnd);

         rval = DefWindowProc(hWnd, message, wParam, lParam);

         if(!IsIconic(hWnd) && (ShowStatusBar || ShowToolBar))
         {
            if(ShowToolBar)
            {
               ((LPRECT)lParam)->top += 2 * HIWORD(dwDialogUnits);
               // 16 'dialog unit' pixels (always)
            }

            if(ShowStatusBar)
            {
               ((LPRECT)lParam)->top += HIWORD(dwDialogUnits) +
                                        StatusBarFontHeight();

//               ((LPRECT)lParam)->bottom -= HIWORD(dwDialogUnits) +
//                                           StatusBarFontHeight();

               // 16 'dialog unit' pixels adjusted for actual font height
            }
         }

         return(rval);


      case WM_NCPAINT:

         if(!IsIconic(hWnd) && (ShowStatusBar || ShowToolBar))
         {
            return(MainOnNCPaint(hWnd, wParam, lParam));
         }

         break;




      // HANDLING THE 'TOOLBAR' BUTTONS HERE

      case WM_NCMOUSEMOVE:
      case WM_NCLBUTTONDOWN:
      case WM_NCLBUTTONUP:

         // if cursor is over tool bar, handle it...

         if(CheckToolBarCursor(hWnd, message, wParam, lParam))
         {
            return(TRUE); // I handled this...
         }

         break;           // default processing




      case WM_TIMER:      // special timer handler for tool bar


         if(((UINT)wParam & 0xffc0) == TOOLBAR_TIMER)
         {
            CheckToolBarCursor(hWnd, message, wParam, lParam);

//            KillTimer(hWnd, (UINT)wParam);
            return(0);
         }

         break;



      case WM_SETCURSOR:

         if(LOWORD(lParam)==HTCLIENT && (HWND)wParam == hMainWnd)
         {
            if(!hMainCursor) hMainCursor = LoadCursor(NULL, IDC_ARROW);

            SetCursor(hMainCursor);
            return((LRESULT)TRUE);
         }

         break;



      ON_WM_COMMAND(MainOnCommand, hWnd, wParam, lParam);

      ON_WM_CHAR(MainOnChar, hWnd, wParam, lParam);

      ON_WM_PAINT(MainOnPaint, hWnd, wParam, lParam);

      ON_WM_VSCROLL(MainOnVScroll, hWnd, wParam, lParam);

      ON_WM_HSCROLL(MainOnHScroll, hWnd, wParam, lParam);

      ON_WM_SYSCOMMAND(MainOnSysCommand, hWnd, wParam, lParam);

      case WM_SIZE:
         while(MainCheckScrollBars(hWnd))
            ;   /* continues to check until no changes made */

         break;


      case WM_KILLFOCUS:
         DestroyCaret();          /* see to it that I lose the caret */
         own_caret = FALSE;       /* I no longer own the caret */
         caret_hidden = 0;        /* also the 'hide' count goes to 0 */

         if(hCaret1)  DeleteObject(hCaret1);
         if(hCaret2)  DeleteObject(hCaret2);

         hCaret1 = (HBITMAP)NULL;
         hCaret2 = (HBITMAP)NULL;

         break;

      case WM_SETFOCUS:

         while(MainCheckScrollBars(hWnd))
            ;   /* continues to check until no changes made */

         if(!hCaret1) hCaret1 = LoadBitmap(hInst, "CARET1");
         if(!hCaret2) hCaret2 = LoadBitmap(hInst, "CARET2");

         CreateCaret(hWnd, (insert_mode?hCaret2:hCaret1), 8, 8);
         own_caret = TRUE;
         caret_hidden = 1;                       /* hide count on caret */

         GetClientRect(hWnd, (LPRECT)&r);
         InvalidateRect(hWnd, (LPRECT)&r, FALSE);  /* repaint */

         break;


      case WM_CLOSE:
         CMDExit(NULL,NULL);       /* normal operations handled here */
                                   /* it checks for several things!  */
         return(FALSE);




      // CLIPBOARD MESSAGES (CLIPBOARD VIEWER CHAIN)

      case WM_CHANGECBCHAIN:

         if((HWND)wParam == hwndNextClipboardViewer)
         {
            hwndNextClipboardViewer = (HWND)(UINT)lParam;
         }
         else
         {
            SendMessage(hwndNextClipboardViewer, WM_CHANGECBCHAIN,
                        wParam, lParam);
         }

         return(0);  // I processed this message!


      case WM_DRAWCLIPBOARD:

         if(hwndNextClipboardViewer)
         {
            SendMessage(hwndNextClipboardViewer, WM_DRAWCLIPBOARD,
                        wParam, lParam);
         }

         // Is clipboard data in the format 'CF_TEXT' or
         // 'CF_OEMTEXT' still available?  As such, modify
         // the 'IDM_PASTE_MENU' menu item and button
         // 'enabled' state appropriately

         if(IsClipboardFormatAvailable(CF_TEXT) ||
            IsClipboardFormatAvailable(CF_OEMTEXT))
         {
            EnableMenuItem(GetMenu(hWnd), IDM_PASTE_MENU,
                           MF_ENABLED | MF_BYCOMMAND);

            EnableToolBarButton(IDM_PASTE_MENU, 1);
         }
         else
         {
            EnableMenuItem(GetMenu(hWnd), IDM_PASTE_MENU,
                           MF_GRAYED | MF_BYCOMMAND);

            EnableToolBarButton(IDM_PASTE_MENU, 0);
         }

         return(0);




      case WM_DESTROY:

         if(lpDragAcceptFiles)
         {
            lpDragAcceptFiles(hWnd, FALSE);
         }

         DestroyCaret();          /* see to it that I lose the caret */
         own_caret = FALSE;       /* I no longer own the caret */
         caret_hidden = 0;        /* also the 'hide' count goes to 0 */

         if(hCaret1)  DeleteObject(hCaret1);
         if(hCaret2)  DeleteObject(hCaret2);

         hCaret1 = (HBITMAP)NULL;
         hCaret2 = (HBITMAP)NULL;

         ChangeClipboardChain(hWnd, hwndNextClipboardViewer);

         PostQuitMessage(0);
         return(FALSE);


      case WM_USER:

         if(wParam == (WPARAM)12345)  // message from SFTSH32X
         {
            dwLastProcessID = (DWORD)lParam;
            return(TRUE);
         }

         break;  // continue with default processing
   }

   return (DefWindowProc(hWnd, message, wParam, lParam));
}



__inline void NEAR PASCAL GetToolBarRect(HWND hWnd, LPRECT lpR)
{
RECT r, r2;
//DWORD dwDialogUnits = GetDialogBaseUnits();

       // x pixels == x * LOWORD(dwDialogUnits) / 4
       // y pixels == y * HIWORD(dwDialogUnits) / 8


   if(!ShowToolBar || IsIconic(hWnd))
   {
      lpR->left = lpR->right = lpR->top = lpR->bottom = 0;
      return;
   }

   GetWindowRect(hWnd, &r);
   r2 = r;

   DefWindowProc(hWnd, WM_NCCALCSIZE, 0, (LPARAM)(LPRECT)&r);

   // update 'r' to reflect toolbar rectangle with respect to window
   // client area (client DC has origin of 0,0 at upper left corner)

   r.left   -= r2.left;
   r.right   = r2.right - r2.left - r.left;  // ignore scroll bar width

   r.top    -= r2.top;

   r.bottom  = r.top + 2 * HIWORD(dwDialogUnits);

             // add 16 'dialog unit' pixels for tool bar

   *lpR = r;
}

__inline void NEAR PASCAL GetStatusBarRect(HWND hWnd, LPRECT lpR)
{
RECT r, r2;
//DWORD dwDialogUnits = GetDialogBaseUnits();

       // x pixels == x * LOWORD(dwDialogUnits) / 4
       // y pixels == y * HIWORD(dwDialogUnits) / 8


   // NOTE:  This function places the status bar just below the tool bar

   if(!ShowStatusBar || IsIconic(hWnd))
   {
      lpR->left = lpR->right = lpR->top = lpR->bottom = 0;
      return;
   }

   if(!TMFlag) UpdateTextMetrics(hWnd);

   GetWindowRect(hWnd, &r);
   r2 = r;

   DefWindowProc(hWnd, WM_NCCALCSIZE, 0, (LPARAM)(LPRECT)&r);

   // update 'r' to reflect toolbar rectangle with respect to window
   // client area (client DC has origin of 0,0 at upper left corner)

   r.left   -= r2.left;
   r.right   = r2.right - r2.left - r.left;  // ignore scroll bar width

   r.top    -= r2.top;

   if(ShowToolBar)
   {
      r.top += 2 * HIWORD(dwDialogUnits);
   }

   r.bottom  = r.top + ( HIWORD(dwDialogUnits) + StatusBarFontHeight() );

             // add 16 'dialog unit' pixels for status bar (max), adjusted
             // for the actual height of the font being used.

   *lpR = r;
}

__inline void NEAR PASCAL GetStatusBarRectBottom(HWND hWnd, LPRECT lpR)
{
RECT r, r2;
//DWORD dwDialogUnits = GetDialogBaseUnits();

       // x pixels == x * LOWORD(dwDialogUnits) / 4
       // y pixels == y * HIWORD(dwDialogUnits) / 8


   if(!ShowStatusBar || IsIconic(hWnd))
   {
      lpR->left = lpR->right = lpR->top = lpR->bottom = 0;
      return;
   }

   if(!TMFlag) UpdateTextMetrics(hWnd);

   GetWindowRect(hWnd, &r);
   r2 = r;


   DefWindowProc(hWnd, WM_NCCALCSIZE, 0, (LPARAM)(LPRECT)&r);


   // update 'r' to reflect toolbar rectangle with respect to window
   // non-client area (non-client DC has origin of 0,0 at upper left corner)

   r.left   -= r2.left;
   r.right   = r2.right - r2.left - r.left;     // ignore scroll bar width

   r.bottom  = r2.bottom - r2.top;              // ignore scroll bar height


   if(!IsMaximized(hWnd))
   {
      r.bottom -= GetSystemMetrics(SM_CYFRAME); // border height (if applicable)
   }

   r.top     = r.bottom - ( HIWORD(dwDialogUnits) + StatusBarFontHeight() );

             // add 16 'dialog unit' pixels for status bar (max), adjusted
             // for the actual height of the font being used.

   *lpR = r;
}


void FAR PASCAL InvalidateToolBar(HWND hWnd)
{
RECT r0, r1;

   if(!ShowToolBar) return;

   GetWindowRect(hWnd, &r0);
   GetToolBarRect(hWnd, &r1);  // rectangle for tool bar relative to border

   r0.right = r1.right + r0.left;
   r0.left += r1.left;

   r0.bottom = r1.bottom + r0.top;
   r0.top += r1.top;

   ScreenToClient(hWnd, (LPPOINT)&r0);
   ScreenToClient(hWnd, ((LPPOINT)&r0) + 1);

   InvalidateRect(hWnd, &r0, TRUE);  // invalidate entire toolbar
}


void FAR PASCAL InvalidateStatusBar(HWND hWnd)
{
RECT r0, r1;

   if(!ShowStatusBar) return;

   GetWindowRect(hWnd, &r0);
   GetStatusBarRect(hWnd, &r1);  // rectangle for tool bar relative to border

   r0.right = r1.right + r0.left;
   r0.left += r1.left;

   r0.bottom = r1.bottom + r0.top;
   r0.top += r1.top;

   ScreenToClient(hWnd, (LPPOINT)&r0);
   ScreenToClient(hWnd, ((LPPOINT)&r0) + 1);

   InvalidateRect(hWnd, &r0, TRUE);  // invalidate entire toolbar
}


void FAR PASCAL EnableToolBarButton(WORD wID, BOOL bEnable)
{
WORD w1;


   for(w1=0; w1 < N_DIMENSIONS(pButtonID); w1++)
   {
      if(pButtonID[w1] == wID)
      {
         if(bEnable)
         {
            if(!(iButtonEnabled & pButtonBits[w1]))
            {
               iButtonEnabled |= pButtonBits[w1];
               iButtonPressed &= ~pButtonBits[w1];

               PaintToolBarButton(hMainWnd, w1);
            }
         }
         else
         {
            if(iButtonEnabled & pButtonBits[w1])
            {
               iButtonEnabled &= ~pButtonBits[w1];
               iButtonPressed &= ~pButtonBits[w1];

               PaintToolBarButton(hMainWnd, w1);
            }
         }

         break;
      }
   }

}


LRESULT NEAR PASCAL CheckToolBarCursor(HWND hWnd, UINT msg, WPARAM wParam,
                                       LPARAM lParam)
{
RECT r0, r1;
POINT pt, pt2;
WORD i;
//DWORD dwDialogUnits;




   if(!ShowToolBar)
   {
      if(iToolBarTimer)
      {
         if((iToolBarTimer & 0xffc0)==TOOLBAR_TIMER)
            KillTimer(hWnd, iToolBarTimer);

         iToolBarTimer = 0;
      }

      if(hwndToolBarHint)
      {
         DestroyWindow(hwndToolBarHint);
         hwndToolBarHint = NULL;
      }

      return(FALSE);     // not displaying tool bar
   }


   if(msg == WM_TIMER)
   {
      GetCursorPos(&pt);       // get cursor for TIMER messages
   }
   else
   {
      pt.x = LOWORD(lParam);   // use 'lParam' for all other messages
      pt.y = HIWORD(lParam);
   }

   GetWindowRect(hWnd, &r0);
   GetToolBarRect(hWnd, &r1);  // rectangle for tool bar relative to border


   r0.right = r1.right + r0.left;
   r0.left += r1.left;

   r0.bottom = r1.bottom + r0.top;
   r0.top += r1.top;


   // if not over toolbar area, or 'mousemove' message and button not down,
   // cancel any currently 'pressed' buttons.

   if(msg == WM_CANCELMODE ||
      !PtInRect(&r0, pt)   ||
      (msg == WM_NCMOUSEMOVE && !(GetKeyState(VK_LBUTTON) & 0x8000)))
   {
      if(iButtonPressed)          // cancel 'pressed' button
      {
         iButtonPressed = 0;      // all buttons are now "up"

         InvalidateToolBar(hWnd); // invalidate entire bar

//         ScreenToClient(hWnd, (LPPOINT)&r0);
//         ScreenToClient(hWnd, ((LPPOINT)&r0) + 1);
//         InvalidateRect(hWnd, &r0, TRUE);
//
//         UpdateWindow(hWnd);               // and re-paint it NOW

         SendMessage(hWnd, WM_NCPAINT, 0, 0);
      }


      // if not 'mouse move' or not within the toolbar rect, cancel any
      // timers or toolbar 'hint' windows and return immediately

      if(msg != WM_NCMOUSEMOVE || !PtInRect(&r0, pt))
      {
         if(hwndToolBarHint)
         {
            DestroyWindow(hwndToolBarHint);
            hwndToolBarHint = NULL;
         }

         if(iToolBarTimer)
         {
            if((iToolBarTimer & 0xffc0)==TOOLBAR_TIMER)
               KillTimer(hWnd, iToolBarTimer);

            iToolBarTimer = 0;
         }

         return(FALSE);  // only return if outside of toolbar or not
                         // a 'mousemove' message (mousemove handled below)
      }
   }


   // see if the mouse is inside one of the buttons!

//   dwDialogUnits = GetDialogBaseUnits();

   r1 = r0;

   r1.top += HIWORD(dwDialogUnits) / 4;  // 2 dialog units
   r1.bottom = r1.top + (3 * HIWORD(dwDialogUnits)) / 2;

   for(i=0; i<N_DIMENSIONS(pButtons); i++)
   {
      if(pButtons[i] < 0) continue;

      r1.left = r0.left + LOWORD(dwDialogUnits) * 2   // 8 units to the left
              + (pButtons[i] * LOWORD(dwDialogUnits)) / 4;

      r1.right = r1.left + (7 * LOWORD(dwDialogUnits)) / 2;// 14 units wide


      if(PtInRect(&r1, pt)) // ah, HAH!  current point within the rect!
      {
         if(msg == WM_TIMER && i == ((UINT)wParam & 0x3f))
         {
            if(GetFocus() != hWnd)  // I don't have the focus - oh, well
            {
               if(hwndToolBarHint) DestroyWindow(hwndToolBarHint);
               hwndToolBarHint = NULL;

               if((iToolBarTimer & 0xffc0)==TOOLBAR_TIMER &&
                  (int)wParam != iToolBarTimer)
               {
                  KillTimer(hWnd, iToolBarTimer);
               }

               KillTimer(hWnd, (WORD)wParam);
               iToolBarTimer = 0;                // cancels timer entirely
            }
            else if(hwndToolBarHint)
            {
               // rather than DESTROYING 'hwndToolBarHint' just hide it
               // so that it turns OFF but doesn't "flash"

               ShowWindow(hwndToolBarHint, SW_HIDE);

               if((iToolBarTimer & 0xffc0)==TOOLBAR_TIMER &&
                  (int)wParam != iToolBarTimer)
               {
                  KillTimer(hWnd, iToolBarTimer);
               }

               KillTimer(hWnd, (WORD)wParam);
               iToolBarTimer = i;     // this marks the timer
            }
            else
            {
               if((iToolBarTimer & 0xffc0)==TOOLBAR_TIMER &&
                  (int)wParam != iToolBarTimer)
               {
                  KillTimer(hWnd, iToolBarTimer);
               }

               KillTimer(hWnd, (WORD)wParam);

               iToolBarTimer = 0;  // in case hWnd gets 'WM_SETFOCUS'


               pt2.x = r1.right + LOWORD(dwDialogUnits) / 4;  // 1 unit right
               pt2.y = r1.bottom + HIWORD(dwDialogUnits) / 8; // 1 unit down

//               ScreenToClient(hWnd, &pt2);  // convert to CLIENT coordinates

               hwndToolBarHint =
                  CreateWindow(szToolBarHint, szButtonHint[i],
                               WS_BORDER | WS_DISABLED | WS_POPUP,
                               pt2.x, pt2.y, 1, 1, hWnd, NULL, hInst, NULL);

               ShowWindow(hwndToolBarHint, SW_SHOWNOACTIVATE);

               iToolBarTimer = TOOLBAR_TIMER | i;

               SetTimer(hWnd, iToolBarTimer, 4000, NULL);   // visible 4 sec
            }

            return(TRUE);
         }


         if((iToolBarTimer || hwndToolBarHint) &&
            (msg != WM_NCMOUSEMOVE || i != (iToolBarTimer & 0x3f)))
         {
            if((iToolBarTimer & 0xffc0)==TOOLBAR_TIMER)
               KillTimer(hWnd, iToolBarTimer);
            iToolBarTimer = 0;

            if(hwndToolBarHint) DestroyWindow(hwndToolBarHint);
            hwndToolBarHint = NULL;
         }

         if(msg == WM_NCLBUTTONDOWN)  // pressing a button
         {
            if(!(iButtonEnabled & pButtonBits[i])) // button disabled?
            {
               iButtonPressed &= ~pButtonBits[i];

               MessageBeep(0);
            }
            else if(!(iButtonPressed & pButtonBits[i]))
            {                               // button not already 'pressed'

               iButtonPressed |= pButtonBits[i];

               PaintToolBarButton(hWnd, i);
            }
         }
         else if(msg == WM_NCLBUTTONUP)          // going UP!
         {
            if(!(iButtonEnabled & pButtonBits[i])) // button disabled?
            {
               iButtonPressed &= ~pButtonBits[i];
            }
            else if(iButtonPressed & pButtonBits[i])
            {                                    // button is 'pressed'
               iButtonPressed &= ~pButtonBits[i];

               PaintToolBarButton(hWnd, i);

               PostMessage(hWnd, WM_COMMAND, (WPARAM)pButtonID[i], 0);
            }
         }
         else if(msg = WM_NCMOUSEMOVE)
         {
            if(iToolBarTimer && (iToolBarTimer & 0x3f) != i)
            {
               if((iToolBarTimer & 0xffc0)==TOOLBAR_TIMER)
                  KillTimer(hWnd, iToolBarTimer);

               iToolBarTimer = 0;

               if(hwndToolBarHint) DestroyWindow(hwndToolBarHint);
               hwndToolBarHint = NULL;
            }

            if(!iToolBarTimer && !hwndToolBarHint)
            {
               iToolBarTimer = TOOLBAR_TIMER | i;

               SetTimer(hWnd, iToolBarTimer, 1000, NULL);   // 1 sec delay
            }
         }

         return(TRUE);
      }
   }


   // mouse not currently on a button in the toolbar, so we need to
   // cancel any currently 'pressed' buttons or 'tool bar' timer

   if(iToolBarTimer)
   {
      if((iToolBarTimer & 0xffc0)==TOOLBAR_TIMER)
         KillTimer(hWnd, iToolBarTimer);

      iToolBarTimer = 0;
   }
   if(hwndToolBarHint)
   {
      DestroyWindow(hwndToolBarHint);
      hwndToolBarHint = NULL;
   }


   if(iButtonPressed)                   // cancel any 'pressed' button
   {
      iButtonPressed = 0;               // all buttons are now "up"

      InvalidateToolBar(hWnd);

//      ScreenToClient(hWnd, (LPPOINT)&r0);
//      ScreenToClient(hWnd, ((LPPOINT)&r0) + 1);
//      InvalidateRect(hWnd, &r0, TRUE);
//
//      UpdateWindow(hWnd);               // and re-paint it NOW

      SendMessage(hWnd, WM_NCPAINT, 0, 0);

   }

   return(FALSE);
}


void NEAR PASCAL PaintToolBarButtonBorder(HDC hdcBar, LPRECT lpRctBar,
                                          UINT uiButtonIndex,
                                          HPEN hpenDarkGrey, HPEN hpenWhite,
                                          DWORD dwDialogUnits)
{
RECT r1 = *lpRctBar;
HPEN hOldPen;



   if(pButtons[uiButtonIndex] < 0) return;  // don't paint it


   // add 8 units to the left, then make it 12 units high and 14 units wide
   // vertically centered, beginning at 'pButtons[]' for horizontal position


   r1.left = lpRctBar->left + LOWORD(dwDialogUnits) * 2
           + (pButtons[uiButtonIndex] * LOWORD(dwDialogUnits)) / 4;

   r1.right = r1.left + (7 * LOWORD(dwDialogUnits)) / 2;

   r1.top += HIWORD(dwDialogUnits) / 4;                 // 2 dialog units
   r1.bottom = r1.top + (3 * HIWORD(dwDialogUnits)) / 2;


   r1.left++;                                // make up for border
   r1.right--;                               // make up for border
   r1.top++;
   r1.bottom--;

   hOldPen = SelectObject(hdcBar, hpenDarkGrey); // select dark grey pen

   if(iButtonPressed & pButtonBits[uiButtonIndex])
   {
      MoveTo(hdcBar, r1.left, r1.bottom - 1);
      LineTo(hdcBar, r1.left, r1.top);
      LineTo(hdcBar, r1.right, r1.top);

      MoveTo(hdcBar, r1.left + 1, r1.bottom - 2);
      LineTo(hdcBar, r1.left + 1, r1.top + 1);
      LineTo(hdcBar, r1.right - 1, r1.top + 1);
   }
   else
   {
      MoveTo(hdcBar, r1.left + 1, r1.bottom - 1);
      LineTo(hdcBar, r1.right - 1, r1.bottom - 1);
      LineTo(hdcBar, r1.right - 1, r1.top);

      MoveTo(hdcBar, r1.left + 2, r1.bottom - 2);
      LineTo(hdcBar, r1.right - 2, r1.bottom - 2);
      LineTo(hdcBar, r1.right - 2, r1.top + 1);
   }

   SelectObject(hdcBar, hpenWhite);              // select white pen

   if(iButtonPressed & pButtonBits[uiButtonIndex])
   {
      MoveTo(hdcBar, r1.left + 1, r1.bottom - 1);
      LineTo(hdcBar, r1.right - 1, r1.bottom - 1);
      LineTo(hdcBar, r1.right - 1, r1.top);

      MoveTo(hdcBar, r1.left + 2, r1.bottom - 2);
      LineTo(hdcBar, r1.right - 2, r1.bottom - 2);
      LineTo(hdcBar, r1.right - 2, r1.top + 1);
   }
   else
   {
      MoveTo(hdcBar, r1.left, r1.bottom - 1);
      LineTo(hdcBar, r1.left, r1.top);
      LineTo(hdcBar, r1.right, r1.top);

      MoveTo(hdcBar, r1.left + 1, r1.bottom - 2);
      LineTo(hdcBar, r1.left + 1, r1.top + 1);
      LineTo(hdcBar, r1.right - 1, r1.top + 1);
   }


   SelectObject(hdcBar, hOldPen);

}


void NEAR PASCAL PaintToolBarButtonFace(HDC hdcBar, LPRECT lpRctBar,
                                        HDC hdcBMP, UINT uiButtonIndex,
                                        HPEN hpenBlack, HBRUSH hBrush,
                                        DWORD dwDialogUnits)
{
RECT r1 = *lpRctBar;
HPEN hOldPen;
int iOldStretchMode;


   if(pButtons[uiButtonIndex] < 0) return;  // don't paint it


   // add 8 units to the left, then make it 12 units high and 14 units wide
   // vertically centered, beginning at 'pButtons[]' for horizontal position


   r1.left = lpRctBar->left + LOWORD(dwDialogUnits) * 2
           + (pButtons[uiButtonIndex] * LOWORD(dwDialogUnits)) / 4;

   r1.right = r1.left + (7 * LOWORD(dwDialogUnits)) / 2;

   r1.top += HIWORD(dwDialogUnits) / 4;                 // 2 dialog units
   r1.bottom = r1.top + (3 * HIWORD(dwDialogUnits)) / 2;



   // draw border rectangle in black

   hOldPen = SelectObject(hdcBar, hpenBlack);        // select black pen

   MoveTo(hdcBar, r1.left, r1.top);
   LineTo(hdcBar, r1.right - 1, r1.top);
   LineTo(hdcBar, r1.right - 1,r1.bottom - 1);
   LineTo(hdcBar, r1.left, r1.bottom - 1);
   LineTo(hdcBar, r1.left, r1.top);


   // fill in button face with gray brush

   r1.left   += 3;
   r1.right  -= 3;
   r1.top    += 3;
   r1.bottom -= 3;

   FillRect(hdcBar, &r1, hBrush);


   // bitmap height and width are 15.  Adjust 'r1' to reflect this

   r1.left++;
   r1.right--;
   r1.top++;
   r1.bottom--;

   if((r1.right - r1.left) > 17)
   {
      if((r1.right - r1.left) > 25 && (r1.bottom - r1.top) > 23)
      {
         r1.left++;        // give it a little extra space around
         r1.right--;       // but stretch to fit when it paints!
      }
      else
      {
         r1.left  = (r1.left + r1.right - 17) / 2;
         r1.right = r1.left + 17;
      }
   }
   if((r1.bottom - r1.top) > 16)
   {
      if((r1.right - r1.left) > 25 && (r1.bottom - r1.top) > 23)
      {                              // I need consistency with above

         r1.top++;         // give it a little extra space around
         r1.bottom--;      // but stretch to fit when it paints!
      }
      else
      {
         r1.top    = (r1.top + r1.bottom - 16) / 2;
         r1.bottom = r1.top + 16;
      }
   }

   if(iButtonPressed & pButtonBits[uiButtonIndex])
   {
      // if button is pressed, shift bitmap 1 right and 1 down

      r1.left++;
      r1.top++;
      r1.right++;
      r1.bottom++;
   }


   if(iButtonEnabled & pButtonBits[uiButtonIndex])
   {
//      iOldStretchMode = SetStretchBltMode(hdcBar, BLACKONWHITE);
//                  // preserve BLACK pixels (was STRETCH_ANDSCANS)

      iOldStretchMode = SetStretchBltMode(hdcBar, COLORONCOLOR);
                  // delete 'extra' lines during stretch (was STRETCH_DELETESCANS)

//      if(15 * (r1.right - r1.left)) > (16 * r1.bottom - r1.top))
//      {
//         r1.left += ((r1.right - r1.left)
//                  - (r1.bottom - r1.top) * 16 / 15) / 2;
//         r1.right = r1.left + (r1.bottom - r1.top) * 16 / 15;
//      }
//      else
//      {
//        r1.top += ((r1.bottom - r1.top)
//                  - (r1.right - r1.left) * 15 / 16) / 2;
//         r1.bottom = r1.top + (r1.right - r1.left) * 15 / 16;
//      }

      StretchBlt(hdcBar, r1.left, r1.top,
                 r1.right - r1.left, r1.bottom - r1.top,
                 hdcBMP, uiButtonIndex * 16, 0, 16, 15,
                 SRCCOPY);

      SetStretchBltMode(hdcBar, iOldStretchMode);
   }
   else
   {
    HDC hDC2;
    HBITMAP hbmpPattern, hbmpCompat, hbmpOld;
    HBRUSH hbrOld, hbrPattern;
    static const short CODE_BASED pDither[]=
                            {0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55};



      hDC2 = CreateCompatibleDC(hdcBar);

      hbmpPattern = CreateBitmap(8, 8, 1, 1, pDither);

      // create color bitmap that matches the bitmap used for the buttons

      hbmpCompat = CreateCompatibleBitmap(hdcBMP,
                                          r1.right - r1.left,
                                          r1.bottom - r1.top);

      hbrPattern = CreatePatternBrush(hbmpPattern);
      hbmpOld = SelectObject(hDC2, hbmpCompat);


      // copy button bitmap (stretch it) into compatible DC

      iOldStretchMode = SetStretchBltMode(hDC2, BLACKONWHITE);
                          // preserve BLACK pixels in favor of any other type

      StretchBlt(hDC2, 0, 0,
                 r1.right - r1.left, r1.bottom - r1.top,
                 hdcBMP, uiButtonIndex * 16, 0, 16, 15,
                 SRCCOPY);

      SetStretchBltMode(hDC2, iOldStretchMode);



      // THIS is where we get 'cute' because the bitmap ought to have a
      // 'grayed' appearance.  The way we manage that is to do some
      // pattern stuff while painting the bitmap onto the button.


      UnrealizeObject(hbrPattern);
      hbrOld = SelectObject(hdcBar, hbrPattern);

      // using the current pattern, 'and' the face of the button with
      // the inverted pattern to leave some 'holes'.

      PatBlt(hdcBar, r1.left, r1.top,
             r1.right - r1.left, r1.bottom - r1.top,
             0x000a0329);                // ROP 0A - DPna


      // now 'AND' the pattern and the source, and 'OR' into the dest

      BitBlt(hdcBar, r1.left, r1.top,
             r1.right - r1.left, r1.bottom - r1.top,
             hDC2, 0, 0, 0x00ea02e9);    // ROP ea - DPSao


      // object cleanup

      SelectObject(hdcBar, hbrOld);
      SelectObject(hDC2, hbmpOld);
      DeleteDC(hDC2);

      DeleteObject(hbmpCompat);
      DeleteObject(hbrPattern);
      DeleteObject(hbmpPattern);
   }

   SelectObject(hdcBar, hOldPen);
}


// THIS PROC PAINTS A SINGLE BUTTON ON THE TOOLBAR!!

void FAR PASCAL PaintToolBarButton(HWND hWnd, UINT uiButtonIndex)
{
HDC hdcBar, hdcBMP;
HPEN hpenDarkGrey;
HBRUSH hBrush;
HBITMAP hOldBitmap;
RECT rctBar;
//DWORD dwDialogUnits = GetDialogBaseUnits();
       // x pixels == x * LOWORD(dwDialogUnits) / 4
       // y pixels == y * HIWORD(dwDialogUnits) / 8


   if(!ShowToolBar) return;
   if(!IsWindow(hWnd) || IsIconic(hWnd) || !IsWindowVisible(hWnd)) return;

   hdcBar = GetWindowDC(hWnd);
   GetToolBarRect(hWnd, &rctBar);

   hpenDarkGrey = CreatePen(PS_SOLID, 1, RGB(128,128,128)); // dark grey pen
   hBrush = CreateSolidBrush(RGB(192,192,192));             // grey brush!!

   PaintToolBarButtonBorder(hdcBar, &rctBar, uiButtonIndex,
                            hpenDarkGrey, (HPEN)GetStockObject(WHITE_PEN),
                            dwDialogUnits);

   hdcBMP = CreateCompatibleDC(hdcBar);
   hOldBitmap = SelectObject(hdcBMP, hbmpToolBar);

   PaintToolBarButtonFace(hdcBar, &rctBar, hdcBMP, uiButtonIndex,
                          (HPEN)GetStockObject(BLACK_PEN), hBrush,
                          dwDialogUnits);

   SelectObject(hdcBMP, hOldBitmap);
   DeleteDC(hdcBMP);

   DeleteObject(hpenDarkGrey);
   DeleteObject(hBrush);

   ReleaseDC(hWnd, hdcBar);
}


void NEAR PASCAL PaintToolBar(HWND hWnd, HDC hDC)
{
RECT rctBar, r, r1;
HDC hDC2, hdcBar;
HBRUSH hBrush;
HBITMAP hOldBitmap, hBarBitmap, hOldBarBitmap;
HPEN hPen1, hPen2, hOldPen;
WORD i;
//DWORD dwDialogUnits;



   if(!IsWindow(hWnd) || IsIconic(hWnd) || !IsWindowVisible(hWnd)) return;

//   dwDialogUnits = GetDialogBaseUnits();
       // x pixels == x * LOWORD(dwDialogUnits) / 4
       // y pixels == y * HIWORD(dwDialogUnits) / 8


   // PAINTING THE TOOL BAR

   if(ShowToolBar)
   {
      GetToolBarRect(hWnd, &rctBar);
      hdcBar = CreateCompatibleDC(hDC);
      hBrush = CreateSolidBrush(RGB(192,192,192));  // grey brush!!


      r.left = 0;
      r.top = 0;
      r.right = rctBar.right - rctBar.left;
      r.bottom = rctBar.bottom - rctBar.top;

      hBarBitmap = CreateCompatibleBitmap(hDC, r.right, r.bottom);
      hOldBarBitmap = SelectObject(hdcBar, hBarBitmap);

      FillRect(hdcBar, &r, hBrush);  // fill in background at this time


      // CREATE A '3D' BORDER FOR THE TOOL BAR AND FOR EACH BUTTON

      hPen1 = CreatePen(PS_SOLID, 1, RGB(128,128,128)); // dark grey
      hPen2 = (HPEN)GetStockObject(WHITE_PEN);

      hOldPen = (HPEN)SelectObject(hdcBar, hPen2); // select white pen

      MoveTo(hdcBar, r.left, r.bottom - 1);
      LineTo(hdcBar, r.left, r.top);
      LineTo(hdcBar, r.right, r.top);

      MoveTo(hdcBar, r.left + 1, r.bottom - 2);
      LineTo(hdcBar, r.left + 1, r.top + 1);
      LineTo(hdcBar, r.right - 1, r.top + 1);


      SelectObject(hdcBar, hPen1);                 // select dark grey pen

      MoveTo(hdcBar, r.left + 1, r.bottom - 1);
      LineTo(hdcBar, r.right - 1, r.bottom - 1);
      LineTo(hdcBar, r.right - 1, r.top);

      MoveTo(hdcBar, r.left + 2, r.bottom - 2);
      LineTo(hdcBar, r.right - 2, r.bottom - 2);
      LineTo(hdcBar, r.right - 2, r.top + 1);


      // height of buttons is 12 dialog units, roughly centered

      r1 = r;


      for(i=0; i<N_DIMENSIONS(pButtons); i++)
      {
         PaintToolBarButtonBorder(hdcBar, &r1, i, hPen1, hPen2,
                                  dwDialogUnits);
      }

      SelectObject(hdcBar, hOldPen);
      DeleteObject(hPen1);

      hDC2 = CreateCompatibleDC(hDC);
      hOldBitmap = SelectObject(hDC2, hbmpToolBar);


      hPen1 = (HPEN)GetStockObject(BLACK_PEN);
      r1 = r;

      for(i=0; i<N_DIMENSIONS(pButtons); i++)
      {
         PaintToolBarButtonFace(hdcBar, &r1, hDC2, i, hPen1, hBrush,
                                dwDialogUnits);
      }

      SelectObject(hDC2, hOldBitmap);
      DeleteDC(hDC2);




      // NOW, 'BitBlt' the bitmap onto the screen, in what appears to be
      // a VERY VERY smooth, seamless painting operation.  Neat, huh?

      BitBlt(hDC, rctBar.left, rctBar.top,
             rctBar.right - rctBar.left, rctBar.bottom - rctBar.top,
             hdcBar, 0, 0, SRCCOPY);


      // additional object cleanup goes here

      SelectObject(hdcBar, hOldBarBitmap);
      DeleteDC(hdcBar);

      DeleteObject(hBarBitmap);
      DeleteObject(hBrush);
   }


}



void NEAR PASCAL AssignStatusBarText(LPSTR lpBuf1, LPSTR lpBuf2,
                                     LPSTR lpBuf3)
{


   if(lpBuf1)
   {
      if(copying)
      {
       LPCOPY_INFO lpCI = lpCopyInfo;
       WORD wNFiles = 0, wCurFile = 0;

         if(lpCI)
         {
            wNFiles = lpCI->wNFiles;
            wCurFile = lpCI->wCurFile;

            lpCI = lpCI->lpNext;
         }
         while(lpCI)
         {
            wNFiles += lpCI->wNFiles;

            lpCI = lpCI->lpNext;
         }

         wsprintf(lpBuf1, "Copy Queue:  %d of %d (%d%%)", wCurFile, wNFiles,
                  wCopyPercentComplete);
      }
      else
      {
         lstrcpy(lpBuf1, "Copy Queue: {Idle}");
      }
   }

   if(lpBuf2)
   {
      if(formatting)
      {
         if(wFormatPercentComplete > 0 && wFormatPercentComplete <= 100)
         {
            wsprintf(lpBuf2, "Formatting %c: (%d%%)",
                     'A' + lpFormatRequest->wDrive, wFormatPercentComplete);
         }
         else
         {
            wsprintf(lpBuf2, "Formatting Drive %c",
                            'A' + lpFormatRequest->wDrive);
         }
      }
      else
      {
         lstrcpy(lpBuf2, "Format Queue: {Idle}");
      }
   }


   if(lpBuf3)
   {
      *lpBuf3 = 0;

      if(waiting_flag)
      {
         lstrcpy(lpBuf3, "Waiting");
      }
      if(IsCall)
      {
         if(*lpBuf3) lstrcat(lpBuf3, ", ");
         lstrcat(lpBuf3, "CALL");
      }
      if(BatchMode)
      {
         if(*lpBuf3) lstrcat(lpBuf3, ", ");
         lstrcat(lpBuf3, "Batch Mode");
      }
      if(GettingUserInput)
      {
         if(*lpBuf3) lstrcat(lpBuf3, ", ");
         lstrcat(lpBuf3, "User Input");
      }

      if(!*lpBuf3)
      {
         if(lpCurCmd)
         {
            lstrcpy(lpBuf3, "Command");
         }
         else
         {
            lstrcpy(lpBuf3, "{Idle}");
         }
      }
   }

}


HFONT NEAR PASCAL CreateStatusBarFont(void)
{


   if(!TMFlag) UpdateTextMetrics(hMainWnd);


   return(CreateFont(-(int)StatusBarFontHeight(), 0,
                     0, 0, 700, 0, 0, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS,
                     CLIP_DEFAULT_PRECIS, PROOF_QUALITY,
                     VARIABLE_PITCH | FF_DONTCARE, "Arial"));
}


void NEAR PASCAL PaintStatusBarPaneBorder(HDC hdcBar, LPRECT lpRctPane,
                                          HPEN hpenDarkGrey, HPEN hpenWhite,
                                          DWORD dwDialogUnits)
{
HPEN hOldPen;


   hOldPen = SelectObject(hdcBar, hpenDarkGrey);    // select dark grey pen

   MoveTo(hdcBar, lpRctPane->left, lpRctPane->bottom - 1);
   LineTo(hdcBar, lpRctPane->left, lpRctPane->top);
   LineTo(hdcBar, lpRctPane->right, lpRctPane->top);

   MoveTo(hdcBar, lpRctPane->left + 1, lpRctPane->bottom - 2);
   LineTo(hdcBar, lpRctPane->left + 1, lpRctPane->top + 1);
   LineTo(hdcBar, lpRctPane->right - 1, lpRctPane->top + 1);


   SelectObject(hdcBar, hpenWhite);                 // select white pen

   MoveTo(hdcBar, lpRctPane->left + 1, lpRctPane->bottom - 1);
   LineTo(hdcBar, lpRctPane->right - 1, lpRctPane->bottom - 1);
   LineTo(hdcBar, lpRctPane->right - 1, lpRctPane->top);

   MoveTo(hdcBar, lpRctPane->left + 2, lpRctPane->bottom - 2);
   LineTo(hdcBar, lpRctPane->right - 2, lpRctPane->bottom - 2);
   LineTo(hdcBar, lpRctPane->right - 2, lpRctPane->top + 1);


   SelectObject(hdcBar, hOldPen);
}


void NEAR PASCAL PaintStatusBarPaneText(HDC hdcBar, LPRECT lpRctPane,
                                        LPCSTR szText, HBRUSH hBrush,
                                        BOOL bEnabled)
{
COLORREF fgcolor;
WORD oldbkmode;
RECT r0;


   if(bEnabled)
   {
      fgcolor = SetTextColor(hdcBar, RGB(255,255,255));
   }
   else
   {
      fgcolor = SetTextColor(hdcBar, RGB(192,192,192));
   }

   oldbkmode = SetBkMode(hdcBar, OPAQUE);

   r0 = *lpRctPane;

   r0.left += 2;
   r0.right -= 2;
   r0.top += 2;
   r0.bottom -= 2;

   FillRect(hdcBar, &r0, hBrush);   // fill background for this box

   SetBkMode(hdcBar, TRANSPARENT);

   r0 = *lpRctPane;

   r0.right --;    // this is 3D text!
   r0.bottom --;

   DrawText(hdcBar, szText, lstrlen(szText), &r0,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE);


   if(bEnabled)
   {
      SetTextColor(hdcBar, RGB(0,0,0));
   }
   else
   {
      SetTextColor(hdcBar, RGB(128,128,128));
   }

   r0.left   ++;
   r0.top    ++;
   r0.right  ++;
   r0.bottom ++;

   DrawText(hdcBar, szText, lstrlen(szText), &r0,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE);


   // restore device context as it was

   SetBkMode(hdcBar, oldbkmode);
   SetTextColor(hdcBar, fgcolor);

}



// THIS PROC PAINTS A SINGLE PANE ON THE STATUS BAR!

void FAR PASCAL PaintStatusBarPane(HWND hWnd, UINT uiPaneIndex)
{
DWORD dw1; /* , dwDialogUnits; */
RECT rctBar, r1, r2, r3;
HDC hdcBar;
HBRUSH hBrush;
HPEN hpenDarkGrey, hpenWhite;
HFONT hBarFont, hOldFont;
char tbuf[40];


   if(!ShowStatusBar) return;
   if(!IsWindow(hWnd) || IsIconic(hWnd) || !IsWindowVisible(hWnd)) return;

//   dwDialogUnits = GetDialogBaseUnits();
       // x pixels == x * LOWORD(dwDialogUnits) / 4
       // y pixels == y * HIWORD(dwDialogUnits) / 8


   if(!TMFlag) UpdateTextMetrics(hWnd);

   switch(uiPaneIndex)
   {
      case 1:
         AssignStatusBarText(tbuf, NULL, NULL);
         break;

      case 2:
         AssignStatusBarText(NULL, tbuf, NULL);
         break;

      case 3:
         AssignStatusBarText(NULL, NULL, tbuf);
         break;

      default:
         return;
   }


   GetStatusBarRect(hWnd, &rctBar);

   hdcBar = GetWindowDC(hWnd);
   hBrush = CreateSolidBrush(RGB(192,192,192));  // grey brush!!


   // Calculate height of text and rectangle sizes

   hBarFont = CreateStatusBarFont();

   if(hBarFont)
   {
      hOldFont = (HFONT)SelectObject(hdcBar, hBarFont);
   }
   else
   {
      hOldFont = (HFONT)SelectObject(hdcBar,
                                     (HFONT)GetStockObject(SYSTEM_FONT));
   }

#ifdef WIN32
   {
     SIZE sz;
     GetTextExtentPoint32(hdcBar, tbuf, lstrlen(tbuf), &sz); // width/height of text
     dw1 = MAKELONG((WORD)sz.cx, (WORD)sz.cy);
   }
#else  // WIN32
   dw1 = GetTextExtent(hdcBar, tbuf, lstrlen(tbuf));  // width/height of text
#endif // WIN32

   r1 = rctBar;
   r1.bottom = HIWORD(dw1) + 7;  // 4 for lines, 3 for 'white space'
   r1.top = 0;

   r1.top     = ((rctBar.top + rctBar.bottom - r1.bottom - 1) >> 1) + 2;
   r1.bottom += r1.top - 2;

   r1.left  = rctBar.left + LOWORD(dwDialogUnits) / 2;
   r1.right = rctBar.left + (rctBar.right - rctBar.left) / 3
              - LOWORD(dwDialogUnits) / 2;

   r2 = r1;
   r2.left  = rctBar.left + (rctBar.right - rctBar.left) / 3
              + LOWORD(dwDialogUnits) / 2;
   r2.right = rctBar.left + 2 * (rctBar.right - rctBar.left) / 3
              - LOWORD(dwDialogUnits) / 2;

   r3 = r1;
   r3.left  = rctBar.left + 2 * (rctBar.right - rctBar.left) / 3
              + LOWORD(dwDialogUnits) / 2;
   r3.right = rctBar.right - LOWORD(dwDialogUnits) / 2;



   hpenDarkGrey = CreatePen(PS_SOLID, 1, RGB(128,128,128)); // dark grey
   hpenWhite = (HPEN)GetStockObject(WHITE_PEN);

   switch(uiPaneIndex)
   {
      case 1:
         PaintStatusBarPaneBorder(hdcBar, &r1, hpenDarkGrey, hpenWhite,
                                  dwDialogUnits);
         PaintStatusBarPaneText(hdcBar, &r1, tbuf, hBrush, !CopySuspended);

         break;

      case 2:
         PaintStatusBarPaneBorder(hdcBar, &r2, hpenDarkGrey, hpenWhite,
                                  dwDialogUnits);
         PaintStatusBarPaneText(hdcBar, &r2, tbuf, hBrush, !FormatSuspended);

         break;

      case 3:
         PaintStatusBarPaneBorder(hdcBar, &r3, hpenDarkGrey, hpenWhite,
                                  dwDialogUnits);
         PaintStatusBarPaneText(hdcBar, &r3, tbuf, hBrush, TRUE);

         break;
   }



   /* DELETE FONT AND BRUSH RESOURCES */

   SelectObject(hdcBar, hOldFont);
   if(hBarFont) DeleteObject(hBarFont);

   DeleteObject(hBrush);
   DeleteObject(hpenDarkGrey);

   ReleaseDC(hWnd, hdcBar);
}



void NEAR PASCAL PaintStatusBar(HWND hWnd, HDC hDC)
{
RECT rctBar, r, r1, r2, r3;
HDC hdcBar;
HBRUSH hBrush;
HBITMAP hBarBitmap, hOldBarBitmap;
HFONT hOldFont, hBarFont;
char tbuf1[40], tbuf2[40], tbuf3[40];
DWORD dw1; /* , dwDialogUnits; */
HPEN hPen1, hPen2, hOldPen;



   if(!IsWindow(hWnd) || IsIconic(hWnd) || !IsWindowVisible(hWnd)) return;

//   dwDialogUnits = GetDialogBaseUnits();
       // x pixels == x * LOWORD(dwDialogUnits) / 4
       // y pixels == y * HIWORD(dwDialogUnits) / 8


   // PAINTING THE STATUS BAR

   if(ShowStatusBar)
   {
      GetStatusBarRect(hWnd, &rctBar);

      hdcBar = CreateCompatibleDC(hDC);
      hBrush = CreateSolidBrush(RGB(192,192,192));  // grey brush!!


      r.left = 0;
      r.top = 0;
      r.right = rctBar.right - rctBar.left;
      r.bottom = rctBar.bottom - rctBar.top;

      hBarBitmap = CreateCompatibleBitmap(hDC, r.right, r.bottom);
      hOldBarBitmap = SelectObject(hdcBar, hBarBitmap);

      FillRect(hdcBar, &r, hBrush);  // fill in background at this time


      // Generate text to be written (centered in 3 sections of title bar)

      AssignStatusBarText(tbuf1, tbuf2, tbuf3);



      // Calculate height of text and rectangle sizes

      hBarFont = CreateStatusBarFont();

      if(hBarFont)
      {
         hOldFont = (HFONT)SelectObject(hdcBar, hBarFont);
      }
      else
      {
         hOldFont = (HFONT)SelectObject(hdcBar,
                                        (HFONT)GetStockObject(SYSTEM_FONT));
      }

      dw1 = GetTextExtent(hdcBar, tbuf1, lstrlen(tbuf1));  // width/height of text

      r1 = r;
      r1.bottom = HIWORD(dw1) + 7;  // 4 for lines, 3 for 'white space'
      r1.top = 0;

      r1.top     = ((r.top + r.bottom - r1.bottom - 1) >> 1) + 2;
      r1.bottom += r1.top - 2;

      r1.left  = r.left + LOWORD(dwDialogUnits) / 2;
      r1.right = r.left + (r.right - r.left) / 3
                 - LOWORD(dwDialogUnits) / 2;

      r2 = r1;
      r2.left  = r.left + (r.right - r.left) / 3 + LOWORD(dwDialogUnits) / 2;
      r2.right = r.left + 2 * (r.right - r.left) / 3
                 - LOWORD(dwDialogUnits) / 2;

      r3 = r1;
      r3.left  = r.left + 2 * (r.right - r.left) / 3
                 + LOWORD(dwDialogUnits) / 2;
      r3.right = r.right - LOWORD(dwDialogUnits) / 2;



      // CREATE A '3D' BORDER FOR THE STATUS BAR AND INFO BOXES

      hPen1 = CreatePen(PS_SOLID, 1, RGB(128,128,128)); // dark grey
      hPen2 = (HPEN)GetStockObject(WHITE_PEN);

      hOldPen = (HPEN)SelectObject(hdcBar, hPen2); // select white pen

      MoveTo(hdcBar, r.left, r.bottom - 1);
      LineTo(hdcBar, r.left, r.top);
      LineTo(hdcBar, r.right, r.top);

      MoveTo(hdcBar, r.left + 1, r.bottom - 2);
      LineTo(hdcBar, r.left + 1, r.top + 1);
      LineTo(hdcBar, r.right - 1, r.top + 1);


      SelectObject(hdcBar, hPen1);                 // select dark grey pen

      MoveTo(hdcBar, r.left + 1, r.bottom - 1);
      LineTo(hdcBar, r.right - 1, r.bottom - 1);
      LineTo(hdcBar, r.right - 1, r.top);

      MoveTo(hdcBar, r.left + 2, r.bottom - 2);
      LineTo(hdcBar, r.right - 2, r.bottom - 2);
      LineTo(hdcBar, r.right - 2, r.top + 1);

      SelectObject(hdcBar, hOldPen);


      PaintStatusBarPaneBorder(hdcBar, &r1, hPen1, hPen2, dwDialogUnits);
      PaintStatusBarPaneBorder(hdcBar, &r2, hPen1, hPen2, dwDialogUnits);
      PaintStatusBarPaneBorder(hdcBar, &r3, hPen1, hPen2, dwDialogUnits);


      DeleteObject(hPen1);


      /* DRAW TEXT WITHIN STATUS BOXES INDICATING CURRENT STATUS */

      PaintStatusBarPaneText(hdcBar, &r1, tbuf1, hBrush, !CopySuspended);
      PaintStatusBarPaneText(hdcBar, &r2, tbuf2, hBrush, !FormatSuspended);
      PaintStatusBarPaneText(hdcBar, &r3, tbuf3, hBrush, TRUE);


      /* DELETE FONT AND BRUSH RESOURCES */

      SelectObject(hdcBar, hOldFont);
      if(hBarFont) DeleteObject(hBarFont);

      DeleteObject(hBrush);


      // NOW, 'BitBlt' the bitmap onto the screen, in what appears to be
      // a VERY VERY smooth, seamless painting operation.  Neat, huh?

      BitBlt(hDC, rctBar.left, rctBar.top,
             rctBar.right - rctBar.left, rctBar.bottom - rctBar.top,
             hdcBar, 0, 0, SRCCOPY);


      // additional object cleanup goes here

      SelectObject(hdcBar, hOldBarBitmap);
      DeleteDC(hdcBar);

      DeleteObject(hBarBitmap);
   }


}


LRESULT NEAR _fastcall MainOnNCPaint(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
HDC hDC;



   DefWindowProc(hWnd, WM_NCPAINT, wParam, lParam);

   hDC = GetWindowDC(hWnd);


   if(ShowToolBar)
   {
      PaintToolBar(hWnd, hDC);
   }

   if(ShowStatusBar)
   {
      PaintStatusBar(hWnd, hDC);
   }



   /* I AM DONE - RELEASE THE DC AND FREE COMMON OBJECTS */

   ReleaseDC(hWnd, hDC);

   return(0);  // I processed this message

}


void NEAR PASCAL AdjustWindowSizeForFont(HWND hWnd)
{
HDC hDC;
RECT r;
MINMAXINFO mmi;
HFONT hOldFont;



   hDC = GetDC(hWnd);

   if(hMainFont)
   {
      hOldFont = SelectObject(hDC, hMainFont);
   }
   else
   {
      hOldFont = SelectObject(hDC, DEFAULT_FONT);
   }


   if(GetTextMetrics(hDC, (LPTEXTMETRIC)&tmMain))
   {
      TMFlag = TRUE;
   }
   else
   {
      TMFlag = FALSE;
   }

   SelectObject(hDC, hOldFont);


   GetClientRect(hWnd, &r);
   InvalidateRect(hWnd, &r, TRUE);

   GetWindowRect(hWnd, &r);

   CallWindowProc(MainWndProc, hWnd, WM_GETMINMAXINFO,
                  0, (LPARAM)&mmi);

   if(IsMaximized(hWnd))
   {
      if((r.right - r.left) != mmi.ptMaxSize.x)
      {
         SetWindowPos(hWnd, NULL,
                      mmi.ptMaxPosition.x, r.top,
                      mmi.ptMaxSize.x, r.bottom - r.top,
                      SWP_NOACTIVATE | SWP_NOZORDER | SWP_DRAWFRAME);
      }
      else
      {
         SetWindowPos(hWnd, NULL, 0, 0,
                      r.right - r.left, r.bottom - r.top,
                      SWP_NOACTIVATE | SWP_NOMOVE |
                      SWP_NOZORDER | SWP_DRAWFRAME);
      }
   }
   else if(!IsIconic(hWnd))
   {
      if((r.right - r.left) > mmi.ptMaxTrackSize.x)
      {
         SetWindowPos(hWnd, NULL, 0, 0,
                      mmi.ptMaxTrackSize.x,
                      r.bottom - r.top,
                      SWP_NOACTIVATE | SWP_NOMOVE |
                      SWP_NOZORDER | SWP_DRAWFRAME);
      }
      else if((r.right - r.left) < mmi.ptMinTrackSize.x)
      {
         SetWindowPos(hWnd, NULL, 0, 0,
                      mmi.ptMinTrackSize.x,
                      r.bottom - r.top,
                      SWP_NOACTIVATE | SWP_NOMOVE |
                      SWP_NOZORDER | SWP_DRAWFRAME);
      }
      else
      {
         SetWindowPos(hWnd, NULL, 0, 0,
                      r.right - r.left, r.bottom - r.top,
                      SWP_NOACTIVATE | SWP_NOMOVE |
                      SWP_NOZORDER | SWP_DRAWFRAME);
      }
   }


   // ensure status bar completely repaints at least once

   if(ShowStatusBar)
   {
      PaintStatusBar(hWnd, hDC);
   }

   ReleaseDC(hWnd, hDC);
}



VOID NEAR _fastcall MainOnCommand(HWND hWnd, int id, HWND hwndCtl,
                                  UINT wCodeNotify)
{
LPSTR lp1, lp2;
LPCSTR szFaceName;
int i, j, k, hF, wF;
HANDLE h;
HMENU hMenu;
HDC hDC;
HFONT hOldFont;
RECT r;
char tbuf[256];



   switch(id)
   {
                           /*********************/
                           /** FILE MENU STUFF **/
                           /*********************/


      case IDM_OPEN_MENU:
      case IDM_PRINT_MENU:
      case IDM_FOLDER_MENU:

         if(id==IDM_OPEN_MENU)
         {
            if(MyGetOpenFileName("Open File", tbuf, 1)) return;
         }
         else if(id==IDM_PRINT_MENU)
         {
            if(MyGetOpenFileName("Print File", tbuf, 0)) return;
         }
         else if(id==IDM_FOLDER_MENU)
         {
          FARPROC lpProc = MakeProcInstance((FARPROC)FolderDlgProc, hInst);

            if(DialogBoxParam(hInst, "FOLDER", hWnd, (DLGPROC)lpProc,
                              (LPARAM)(LPSTR)tbuf))
            {
               *tbuf = 0;     // user pressed 'Cancel'
            }


            FreeProcInstance(lpProc);
         }
         else
         {
            return;
         }

         if(!*tbuf) return;  // empty string - do nothing!

         if(*cmd_buf)
         {
            MainOnChar(hWnd, '\x1b', 1);  /* forces entry of <ESC> character */
         }

         if(id == IDM_PRINT_MENU)
         {
            lstrcpy(cmd_buf, "PRINT ");
         }
         else if(id == IDM_FOLDER_MENU)
         {
            lp1 = _fstrchr(tbuf, ':'); // find that colon!!

            // if I'm changing drives, ensure I plunk the 'D:' (or whatever)
            // into the command line ahead of the 'CHDIR' command

            if(lp1 &&
               ((_getdrive() + 'A' - 1) != toupper(*tbuf) || tbuf[1] != ':'))
            {
//               _hmemcpy(cmd_buf, tbuf, lp1 - (LPSTR)tbuf + 1);
//               cmd_buf[lp1 - (LPSTR)tbuf + 1];
//
//               cmd_buf_len = lstrlen(cmd_buf);
//
//               MainOnChar(hWnd, '\r', 1);      // load up the drive command
//                                               // and execute it first

               // assign current drive 'manually' at this time
               // assume the character preceding the ':' is a drive letter

               _chdrive(toupper(lp1[-1]) - 'A' + 1);
            }

            lstrcpy(cmd_buf, "CHDIR ");
         }

         lstrcat(cmd_buf, tbuf);
         PrintString(cmd_buf);

         cmd_buf_len = lstrlen(cmd_buf);

         MainOnChar(hWnd, '\r', 1);    /* forces entry of '\r' character */

         return;


      case IDM_ASSOCIATE_MENU:
         {
          DLGPROC lpProc;

            lpProc = (DLGPROC)MakeProcInstance((FARPROC)AssociateDlgProc, hInst);

            DialogBox(hInst, "ASSOCIATE", hWnd, lpProc);

            FreeProcInstance((FARPROC)lpProc);
            break;
         }



      case IDM_EXIT:

         CMDExit(NULL,NULL);  /* indicates that it came from a menu!! */
         return;



                           /*********************/
                           /** EDIT MENU STUFF **/
                           /*********************/


      case IDM_ERASEHISTORY:

         _hmemset(lpCmdHist, 0, CMDHIST_SIZE);
                                // erase history by resetting buffer contents
         lpCurCmd = NULL;
         lpLastCmd = NULL;

         EnableMenuItem(GetMenu(hWnd), IDM_COPY_CMDLINE,
                        MF_GRAYED | MF_BYCOMMAND);

         EnableToolBarButton(IDM_COPY_CMDLINE, 0);

         EnableMenuItem(GetMenu(hWnd), IDM_ERASEHISTORY,
                        MF_GRAYED | MF_BYCOMMAND);

         EnableToolBarButton(IDM_ERASEHISTORY, 0);

         return;



      case IDM_COPY_CMDLINE:  /* copy command line history to clipboard */

         if(!lpCmdHist || !*lpCmdHist)
         {
            MessageBeep(0);
            break;
         }

               /* let's find out how big it is... */

         for(i=0, lp1=lpCmdHist; lpCmdHist && *lp1!=0;)
         {
            j = lstrlen(lp1) + 1;
            i += j + 2;  /* add 2 for <CR><LF> */
            lp1 += j;
         }

                      /* now, allocate the RAM */

         h = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT | GMEM_DDESHARE, i);

         if(!h || !(lp2 = GlobalLock(h)))
         {
          static const char CODE_BASED pMsg[]=
                                      "?Unable to allocate memory for text";
          static const char CODE_BASED pMsg2[]=
                                      "** SFTShell Clipboard Error **";

            if(h) GlobalFree(h);

            LockCode();

            MessageBox(hWnd, pMsg, pMsg2,
                       MB_OK | MB_ICONHAND | MB_TASKMODAL);

            UnlockCode();

            return;
         }


            /* copy the command line history to DDESHARE memory */

         for(lp1=lpCmdHist; lpCmdHist && *lp1!=0;)
         {
            j = lstrlen(lp1);

            _hmemcpy(lp2, lp1, j);
            lp2[j++] = '\r';
            lp2[j]   = '\n';


            lp1 += j;
            lp2 += j + 1;

            *lp2 = 0;
         }

               /* transfer ownership of memory to clipboard */

         GlobalUnlock(h);
         OpenClipboard(hWnd);
         EmptyClipboard();
         SetClipboardData(CF_OEMTEXT, h);
         CloseClipboard();

         return;



      case IDM_EDIT_CMDLINE:  /* command line edit dialog box */

         if(lpLastCmd)    /* there's command history out there! */
         {
          DLGPROC lpEditProc;

            lpEditProc = (DLGPROC)MakeProcInstance((FARPROC)EditCommandDlg,
                                                   hInst);

            DialogBox(hInst,"CMDHistory",hWnd,lpEditProc);

            ctrl_p = FALSE;  /* always clear this after 'ALT F3' */

            FreeProcInstance((FARPROC)lpEditProc);

            ReDisplayUserInput(cmd_buf);
         }
         else
         {
          static const char CODE_BASED pMsg[]=
                                        "?No Command History Buffer present";
          static const char CODE_BASED pMsg2[]=
                                               "** COMMAND HISTORY ERROR **";
            LockCode();

            MessageBox(hMainWnd, pMsg, pMsg2,
                       MB_OK | MB_ICONHAND | MB_TASKMODAL);

            UnlockCode();
         }

         break;

                   /** COPY CONTENTS TO CLIPBOARD **/

      case IDM_COPY_ALL:

         h = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE,
                   (SCREEN_COLS + 2) *
                       (maxlines<SCREEN_LINES?maxlines:SCREEN_LINES) + 1);

         if(!h || !(lp1 = GlobalLock(h)))
         {
          static const char CODE_BASED pMsg[]=
                                       "?Unable to allocate memory for text";
          static const char CODE_BASED pMsg2[]=
                                              "** SFTShell Clipboard Error **";

            if(h) GlobalFree(h);

            LockCode();

            MessageBox(hWnd, pMsg, pMsg2, MB_OK | MB_ICONHAND | MB_TASKMODAL);

            UnlockCode();

            return;
         }

         for(lp2 = lp1, i=0; i<=maxlines && i<SCREEN_LINES; i++)
         {
            for(k=SCREEN_COLS - 1; k>=0; k--)
            {
               if(screen[i][k]) break;  /* finds last non-null character */
            }

            for(j=0; j<=k; j++)
            {
               if(*lp2 = screen[i][j]) lp2++;
               else  *(lp2++) = ' ';
            }
            *(lp2++) = '\r';
            *(lp2++) = '\n';
         }

         *lp2 = 0;

         GlobalUnlock(h);
         OpenClipboard(hWnd);
         EmptyClipboard();
         SetClipboardData(CF_OEMTEXT, h);
         CloseClipboard();
         return;



      case IDM_PASTE_MENU:
         // this executes a 'memory batch file'

         if(hMemoryBatchFileThread)   // too much recursion...
         {
            MessageBeep(0);
            return;
         }

         OpenClipboard(hWnd);

         if((h = GetClipboardData(CF_TEXT)) ||
            (h = GetClipboardData(CF_OEMTEXT)))
         {
            if(SpawnThread(MemoryBatchFileThreadProc, h, THREADSTYLE_DEFAULT))
            {
               CloseClipboard();

               MessageBox(hWnd, "?Unable to spawn thread for memory batch file",
                          "** SFTShell CLIPBOARD PASTE Error **",
                          MB_OK | MB_ICONHAND);
            }
            else
            {
               CloseClipboard();
            }
         }
         else
         {
            CloseClipboard();

            MessageBox(hWnd, "?Unable to obtain compatible clipboard format",
                       "** SFTShell CLIPBOARD PASTE Error **",
                       MB_OK | MB_ICONHAND);
         }



         return;


      case IDM_INSTANT_BATCH:

         {
          DLGPROC lpProc;

            lpProc = (DLGPROC)MakeProcInstance((FARPROC)InstantBatchDlgProc, hInst);

            DialogBox(hInst, "INSTANT_BATCH", hWnd, lpProc);

            FreeProcInstance((FARPROC)lpProc);
         }

         return;



                          /***********************/
                          /** STATUS MENU STUFF **/
                          /***********************/
      case IDM_STATUS:

         if(!copying && !formatting && !BatchMode && !waiting_flag && !IsCall)
         {
            lstrcpy(work_buf, "No background operations in progress");
         }
         else
         {
            *work_buf = 0;

            if(copying)  lstrcpy(work_buf, "Copy in progress");

            if(formatting)
            {
               if(*work_buf)
               {
                  if(BatchMode || waiting_flag || IsCall)
                  {
                     lstrcat(work_buf, ", ");
                  }
                  else
                  {
                     lstrcat(work_buf, " and ");
                  }
               }

               lstrcat(work_buf, "Format in progress");
            }

            if(BatchMode)
            {
               if(*work_buf)
               {
                  if(waiting_flag || IsCall)
                  {
                     lstrcat(work_buf, ", ");
                  }
                  else
                  {
                     lstrcat(work_buf, " and ");
                  }
               }

               lstrcat(work_buf, "Batch File executing");
            }

            if(waiting_flag)
            {
               if(*work_buf)
               {
                  if(IsCall)
                  {
                     lstrcat(work_buf, ", ");
                  }
                  else
                  {
                     lstrcat(work_buf, " and ");
                  }
               }

               lstrcat(work_buf, "'WAIT' in progress");
            }

            if(IsCall)
            {
               if(*work_buf) lstrcat(work_buf, " and ");

               lstrcat(work_buf, "'CALL' in progress");
            }

            lstrcat(work_buf, "!");
         }

         MessageBeep(MB_ICONASTERISK);

         MessageBox(hWnd, work_buf, "** SFTShell Status **",
                    MB_OK | MB_ICONASTERISK);

         return;


      case IDM_TOOL_BAR:

         ShowToolBar = !ShowToolBar;

         CheckMenuItem(GetMenu(hWnd), IDM_TOOL_BAR,
                 (ShowToolBar ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);

         wsprintf(tbuf, "%d", (WORD)(ShowToolBar != 0));
         WriteProfileString(pPROGRAMNAME,pSHOWTOOLBAR,tbuf);

         GetWindowRect(hWnd, &r);
         MoveWindow(hWnd, r.left, r.top,
                          r.right - r.left, r.bottom - r.top, TRUE);

         InvalidateRect(hWnd, NULL, TRUE);
         UpdateWindow(hWnd);

         break;


      case IDM_STATUS_BAR:

         ShowStatusBar = !ShowStatusBar;

         CheckMenuItem(GetMenu(hWnd), IDM_STATUS_BAR,
                 (ShowStatusBar ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);

         wsprintf(tbuf, "%d", (WORD)(ShowStatusBar != 0));
         WriteProfileString(pPROGRAMNAME,pSHOWSTATUSBAR,tbuf);

         GetWindowRect(hWnd, &r);
         MoveWindow(hWnd, r.left, r.top,
                          r.right - r.left, r.bottom - r.top, TRUE);

         InvalidateRect(hWnd, NULL, TRUE);
         UpdateWindow(hWnd);

         break;



      case IDM_SCREEN_LINES:

         {
          DLGPROC lpProc = NULL;

            lpProc = (DLGPROC)MakeProcInstance((FARPROC)ScreenLinesDlgProc, hInst);

            if(DialogBox(hInst, "SCREEN_LINES", hWnd, lpProc))
            {
               AdjustWindowSizeForFont(hWnd);
               UpdateWindow(hWnd);
            }
         }

         break;






                           /*********************/
                           /** FONT MENU STUFF **/
                           /*********************/

      case IDM_FONT_MENU:      /* 'ChooseFont' dialog box */

      case IDM_FONT7x5:        /* select which font to use */
      case IDM_FONT8x6:
      case IDM_FONT9x7:
      case IDM_FONT12x8:
      case IDM_FONT16x12:
      case IDM_FONT24x18:
      case IDM_FONT32x24:
      case IDM_FONT48x36:
      case IDM_FONT_NORMAL:


         // custom fonts chosen here - use 'Font' common dialog

         if(id == IDM_FONT_MENU)
         {
          CHOOSEFONT cf;
          LOGFONT lf;

            if(!lpChooseFont)
            {
               MessageBeep(0);
               break;
            }

            // get existing font information...

            if(hMainFont)
            {
               GetObject(hMainFont, sizeof(lf), (LPVOID)&lf);
//               if(!lf.lfFaceName[0])
//               {
//                  // empty face name - get actual font face


                  // GET THE ACTUAL FONT FACE THAT I AM USING!!!

                  hDC = GetDC(hWnd);

                  hOldFont = SelectObject(hDC, hMainFont);

                  GetTextFace(hDC, sizeof(lf.lfFaceName), lf.lfFaceName);

                  SelectObject(hDC, hOldFont);

                  ReleaseDC(hWnd, hDC);

                  hDC = NULL;
                  hOldFont = NULL;
//               }
            }
            else
               GetObject(DEFAULT_FONT, sizeof(lf), (LPVOID)&lf);


            _hmemset((LPSTR)&cf, 0, sizeof(cf));

            cf.lStructSize = sizeof(cf);

            cf.hwndOwner   = hWnd;
            cf.hDC         = NULL;
            cf.lpLogFont   = &lf;
            cf.iPointSize  = 0;
            cf.Flags       = CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS |
                             CF_FIXEDPITCHONLY | CF_FORCEFONTEXIST;


            if(lpChooseFont(&cf))
            {
               if(hMainFont) DeleteObject(hMainFont);

               hMainFont = CreateFontIndirect(&lf);

               wsprintf(tbuf, "%d", lf.lfHeight);
               WriteProfileString(pPROGRAMNAME, pFONTHEIGHT,tbuf);

               wsprintf(tbuf, "%d", lf.lfWidth);
               WriteProfileString(pPROGRAMNAME, pFONTWIDTH,tbuf);

               WriteProfileString(pPROGRAMNAME, pFONTNAME, lf.lfFaceName);

               wsprintf(tbuf, "%d", lf.lfWeight);
               WriteProfileString(pPROGRAMNAME, pFONTWEIGHT, tbuf);

               wsprintf(tbuf, "%d", lf.lfItalic);
               WriteProfileString(pPROGRAMNAME, pFONTITALIC, tbuf);

               wsprintf(tbuf, "%d", lf.lfUnderline);
               WriteProfileString(pPROGRAMNAME, pFONTUNDERLINE, tbuf);

               wsprintf(tbuf, "%d", lf.lfStrikeOut);
               WriteProfileString(pPROGRAMNAME, pFONTSTRIKEOUT, tbuf);
            }
            else
            {
               break;
            }
         }
         else
         {
            if(id==IDM_FONT7x5)        // teeny
            {
               hF=7;
               wF=5;

               szFaceName="SFTSHELL";  // SFTSHEL5.FNT
            }
            else if(id==IDM_FONT8x6)   // tiny
            {
               hF=8;
               wF=6;

               szFaceName="SFTSHELL";  // SFTSHEL6.FNT
            }
            else if(id==IDM_FONT9x7)   // small
            {
               hF=9;
               wF=7;

               szFaceName="SFTSHELL";  // SFTSHEL7.FNT
            }
            else if(id==IDM_FONT12x8)  // medium
            {
               hF=12;
               wF=8;

               szFaceName="SFTSHELL";  // SFTSHEL8.FNT
            }
            else if(id==IDM_FONT16x12) // large
            {
               hF=16;
               wF=12;

               szFaceName="SFTSHELL";  // SFTSHL12.FNT
            }
            else if(id==IDM_FONT24x18) // huge
            {
               hF=24;
               wF=18;

               szFaceName=NULL;  // any font will do
            }
            else if(id==IDM_FONT32x24) // gigantic
            {
               hF=32;
               wF=24;

               szFaceName=NULL;  // any font will do
            }
            else if(id==IDM_FONT48x36) // monolithic
            {
               hF=48;
               wF=36;

               szFaceName=NULL;  // any font will do
            }
            else // if(id==IDM_FONT_NORMAL)
            {
               hF=0x7fff;
               wF=0x7fff;

               szFaceName=NULL;
            }

            wsprintf(tbuf, "%d", hF);

            WriteProfileString(pPROGRAMNAME, pFONTHEIGHT, tbuf);

            wsprintf(tbuf, "%d", wF);

            WriteProfileString(pPROGRAMNAME, pFONTWIDTH, tbuf);


            WriteProfileString(pPROGRAMNAME, pFONTNAME,NULL);
            WriteProfileString(pPROGRAMNAME, pFONTWEIGHT, NULL);
            WriteProfileString(pPROGRAMNAME, pFONTITALIC, NULL);
            WriteProfileString(pPROGRAMNAME, pFONTUNDERLINE, NULL);
            WriteProfileString(pPROGRAMNAME, pFONTSTRIKEOUT, NULL);


            // create 'hMainFont' (as necessary)

            if(hMainFont) DeleteObject(hMainFont);
            hMainFont = NULL;

            if(hF!=0x7fff && wF!=0x7fff)
            {
               hMainFont = CreateFont(hF, wF, 0, 0, 400, 0, 0, 0,
                                      OEM_CHARSET,
                                      OUT_CHARACTER_PRECIS,
                                      CLIP_DEFAULT_PRECIS,
                                      DRAFT_QUALITY /* DEFAULT_QUALITY */,
                                      FIXED_PITCH | FF_DONTCARE,
                                      szFaceName);
            }
         }


         // update 'checked' menu items

         hMenu = GetMenu(hWnd);


         if(id!=IDM_FONT_NORMAL)
         {
            CheckMenuItem(hMenu, IDM_FONT_NORMAL, MF_UNCHECKED | MF_BYCOMMAND);
         }
         if(id!=IDM_FONT7x5)
         {
            CheckMenuItem(hMenu, IDM_FONT7x5, MF_UNCHECKED | MF_BYCOMMAND);
         }
         if(id!=IDM_FONT8x6)
         {
            CheckMenuItem(hMenu, IDM_FONT8x6, MF_UNCHECKED | MF_BYCOMMAND);
         }
         if(id!=IDM_FONT9x7)
         {
            CheckMenuItem(hMenu, IDM_FONT9x7, MF_UNCHECKED | MF_BYCOMMAND);
         }
         if(id!=IDM_FONT12x8)
         {
            CheckMenuItem(hMenu, IDM_FONT12x8, MF_UNCHECKED | MF_BYCOMMAND);
         }
         if(id!=IDM_FONT16x12)
         {
            CheckMenuItem(hMenu, IDM_FONT16x12, MF_UNCHECKED | MF_BYCOMMAND);
         }
         if(id!=IDM_FONT24x18)
         {
            CheckMenuItem(hMenu, IDM_FONT24x18, MF_UNCHECKED | MF_BYCOMMAND);
         }
         if(id!=IDM_FONT32x24)
         {
            CheckMenuItem(hMenu, IDM_FONT32x24, MF_UNCHECKED | MF_BYCOMMAND);
         }
         if(id!=IDM_FONT48x36)
         {
            CheckMenuItem(hMenu, IDM_FONT48x36, MF_UNCHECKED | MF_BYCOMMAND);
         }
         if(id!=IDM_FONT_MENU)
         {
            CheckMenuItem(hMenu, IDM_FONT_MENU, MF_UNCHECKED | MF_BYCOMMAND);
         }


         CheckMenuItem(hMenu, id, MF_CHECKED | MF_BYCOMMAND);

         AdjustWindowSizeForFont(hWnd);


         break;







                           /*********************/
                           /** QUEUE DLG STUFF **/
                           /*********************/

      case IDM_QUEUE_MENU:

         {
          static DLGPROC lpProc = NULL;

            if(hdlgQueue)
            {
               MessageBeep(0);
               break;
            }

            if(!lpProc)
            {
               // do this ONCE, and leave it 'made'

               lpProc = (DLGPROC)MakeProcInstance((FARPROC)QueueDlgProc, hInst);
            }

            hdlgQueue = CreateDialog(hInst, "QUEUE", hWnd, lpProc);

            if(hdlgQueue)
            {
               // disable menu and toolbar button

               EnableMenuItem(GetMenu(hWnd), IDM_QUEUE_MENU,
                              MF_GRAYED | MF_BYCOMMAND);

               DrawMenuBar(hWnd);  // necessary when menu item is top-level

               EnableToolBarButton(IDM_QUEUE_MENU, 0);

               ShowWindow(hdlgQueue, SW_SHOWNORMAL);
            }
         }

         break;



                           /*********************/
                           /** HELP MENU STUFF **/
                           /*********************/

      case IDM_ABOUT:
         {
          DLGPROC lpProcAbout;

            lpProcAbout = (DLGPROC)MakeProcInstance((FARPROC)About, hInst);

            DialogBox(hInst,"AboutBox",hWnd,lpProcAbout);

            FreeProcInstance((FARPROC)lpProcAbout);
         }

         break;



      case IDM_REGISTRATION:
         {
          DLGPROC lpProcAbout;

            lpProcAbout = (DLGPROC)MakeProcInstance((FARPROC)About, hInst);

            DialogBox(hInst,"REGISTRATION",hWnd,lpProcAbout);

            FreeProcInstance((FARPROC)lpProcAbout);
         }
         break;



      case IDM_HELP_COMMAND:  /* the F1 key... help on a command */

         if(pausing_flag || waiting_flag || BatchMode || GettingUserInput)
         {
            MessageBeep(0);
            return;
         }

         i = cur_attr;    /** WHITE ON RED **/
         PrintErrorMessage(848);  // " \033[1;37;41m** CONTEXT HELP **"
         cur_attr = i;
         PrintString(pCRLF);

         _fmemmove(cmd_buf + 5, cmd_buf, cmd_buf_len + 1);
         cmd_buf_len += 5;
         _fmemcpy(cmd_buf, "HELP ", 5);

         MainOnChar(hWnd, '\r', 1);    /* forces entry of '\r' character */

         return;


      case IDM_HELP_KEYS:

         if(pausing_flag || waiting_flag || BatchMode || GettingUserInput)
         {
            MessageBeep(0);
            return;
         }

         if(*cmd_buf)
         {
            MainOnChar(hWnd, '\x1b', 1);  /* forces entry of <ESC> character */
         }


         i = cur_attr;    /** WHITE ON RED **/
         PrintErrorMessage(849);  // " \033[1;37;41m** HELP 'KEYS' **"
         cur_attr = i;
         PrintString(pCRLF);

         lstrcpy(cmd_buf, "HELP KEYS");
         cmd_buf_len = lstrlen(cmd_buf);

         MainOnChar(hWnd, '\r', 1);    /* forces entry of '\r' character */

         return;


      case IDM_HELP_VARIABLES:

         if(pausing_flag || waiting_flag || BatchMode || GettingUserInput)
         {
            MessageBeep(0);
            return;
         }

         if(*cmd_buf)
         {
            MainOnChar(hWnd, '\x1b', 1);  /* forces entry of <ESC> character */
         }

         i = cur_attr;    /** WHITE ON RED **/
         PrintErrorMessage(850);  // " \033[1;37;41m** HELP 'VARIABLES' **"
         cur_attr = i;
         PrintString(pCRLF);

         lstrcpy(cmd_buf, "HELP VARIABLES");
         cmd_buf_len = lstrlen(cmd_buf);

         MainOnChar(hWnd, '\r', 1);    /* forces entry of '\r' character */

         return;


      case IDM_HELP_ANSI:

         if(pausing_flag || waiting_flag || BatchMode || GettingUserInput)
         {
            MessageBeep(0);
            return;
         }

         if(*cmd_buf)
         {
            MainOnChar(hWnd, '\x1b', 1);  /* forces entry of <ESC> character */
         }

         i = cur_attr;    /** WHITE ON RED **/
         PrintErrorMessage(851);  // " \033[1;37;41m** HELP 'ANSI' **"
         cur_attr = i;
         PrintString(pCRLF);

         lstrcpy(cmd_buf, "HELP ANSI");
         cmd_buf_len = lstrlen(cmd_buf);

         MainOnChar(hWnd, '\r', 1);    /* forces entry of '\r' character */

         return;



      case IDM_HELP_CALCULATION:

         GetModuleFileName(hInst, tbuf, sizeof(tbuf));

         lp1 = _fstrrchr(tbuf, '\\');
         if(lp1) lp1++;

         lstrcpy(lp1, pHELPFILE);


         if(MyOpenFile(tbuf, &TempOfstr, OF_EXIST | OF_READ | OF_SHARE_DENY_NONE)
            == HFILE_ERROR)
         {
            lstrcpy(tbuf, pHELPFILE);
         }

         if(!WinHelp(hMainWnd, tbuf, HELP_CONTENTS, 0L))
         {
            MessageBeep(MB_ICONHAND);

            wsprintf(tbuf, "?Unable to show help file '%s'",
                     (LPSTR)pHELPFILE);

            MessageBox(hMainWnd, tbuf, "** SFTSHELL HELP ERROR **",
                       MB_OK | MB_ICONHAND);
         }

         return;



                      /*******************************/
                      /** HOT-KEY TRANSLATION STUFF **/
                      /*******************************/

      case IDM_CBRK:

         if(!ctrl_p)
            ctrl_break = TRUE;                    /* flag that puppy! */
         else
            MainOnChar(hWnd, 'C' - 'A' + 1, 1); /* input ^C to cmd line */
         return;


      case IDM_CTRL_S:

         if(!ctrl_p)
            stall_output = TRUE;                  /* stall the output */
         else
            MainOnChar(hWnd, 'S' - 'A' + 1, 1);
         return;


      case IDM_CTRL_Q:

         if(!ctrl_p)
            stall_output = FALSE;          /* un-stall output (if stalled) */
         else
            MainOnChar(hWnd, 'Q' - 'A' + 1, 1);
         return;


               /********************************************/
               /** PAGE UP/PAGE DOWN/CTRL RIGHT/CTRL LEFT **/
               /********************************************/

      case IDM_PAGE_UP:
         MainOnVScroll(hWnd, NULL, SB_PAGEUP, 0);
         return;

      case IDM_PAGE_DOWN:
         MainOnVScroll(hWnd, NULL, SB_PAGEDOWN, 0);
         return;

      case IDM_CTRL_LEFT:
         MainOnHScroll(hWnd, NULL, SB_PAGEUP, 0);
         return;

      case IDM_CTRL_RIGHT:
         MainOnHScroll(hWnd, NULL, SB_PAGEDOWN, 0);
         return;

      case IDM_CTRL_UP:
         MainOnVScroll(hWnd, NULL, SB_LINEUP, 0);
         return;

      case IDM_CTRL_DOWN:
         MainOnVScroll(hWnd, NULL, SB_LINEDOWN, 0);
         return;

      case IDM_CTRL_HOME:
         MainOnVScroll(hWnd, NULL, SB_TOP, 0);
         return;

      case IDM_CTRL_END:
         MainOnVScroll(hWnd, NULL, SB_BOTTOM, 0);
         return;



             /**************************************************/
             /* CURSOR CONTROLS (etc.) FOR 'DOSKEY' SIMULATION */
             /**************************************************/


      case IDM_HOME:
      case IDM_END:
      case IDM_LEFT:
      case IDM_RIGHT:

         // If we're not in a 'compatible' situation, bleep an error

         if(!pausing_flag && !waiting_flag && !GettingUserInput &&
            (BatchMode || lpCurCmd))
         {
            MessageBeep(0);
            break;
         }

         if(id == IDM_HOME)
         {
            MoveCursor(start_col, start_line);
         }
         else if(id == IDM_END)
         {
            MoveCursor(end_col, end_line);
         }
         else if(id == IDM_LEFT)
         {
            if(curcol > start_col)
            {
               MoveCursor(curcol - 1, curline);
            }
            else if(curline > start_line)
            {
               if(curcol)
               {
                  MoveCursor(curcol - 1, curline);
               }
               else
               {
                  MoveCursor(SCREEN_COLS - 1, curline - 1);
               }

               FarCheckScreenScroll(FALSE);
            }
            else
            {
               MessageBeep(0);
            }
         }
         else if(id == IDM_RIGHT)
         {
            if(curcol < end_col)
            {
               MoveCursor(curcol + 1, curline);
            }
            else if(curline < end_line)
            {
               if(curcol < (SCREEN_COLS - 1))
               {
                  MoveCursor(curcol + 1, curline);
               }
               else
               {
                  MoveCursor(0, curline + 1);
               }

               FarCheckScreenScroll(FALSE);
            }
            else
            {
               MessageBeep(0);
            }
         }

         break;



      case IDM_CMDLINE:   // previous command (always)

         // If we're not in a 'compatible' situation, bleep an error

         if(!pausing_flag && !waiting_flag && !GettingUserInput &&
            (BatchMode || lpCurCmd))
         {
            MessageBeep(0);
            break;
         }

         if(lpLastCmd)    /* there's command history out there! */
         {
            lpCmdHistIndex = lpCmdHist;

            // we are at the beginning, so go to the end of history

            while(*lpCmdHistIndex)
               lpCmdHistIndex += lstrlen(lpCmdHistIndex) + 1;


            // now "back up" by 1 command and put THAT into the buffer

            if(lpCmdHistIndex > lpCmdHist)  // go to PREVIOUS command!
            {
               lpCmdHistIndex --;

               while(lpCmdHistIndex > lpCmdHist)
               {
                  if(*(lpCmdHistIndex - 1)) lpCmdHistIndex--;
                  else                      break;
               }
            }

            if(*lpCmdHistIndex)
            {
               // adjust this for current input (if smaller than history)

               if(cmd_buf_len < lstrlen(lpCmdHistIndex))
               {
                  lstrcpy(cmd_buf + cmd_buf_len,
                          lpCmdHistIndex + cmd_buf_len);

                  cmd_buf_len = lstrlen(cmd_buf);

                  ReDisplayUserInput(cmd_buf);
               }
               else
               {
                  MessageBeep(0);
               }
            }
            else
            {
               MessageBeep(0);
            }
         }
         else
         {
            MessageBeep(0);
         }

         break;


      case IDM_UP:      // use COMMAND HISTORY and scroll forward...

         // If we're not in a 'compatible' situation, bleep an error

         if(!pausing_flag && !waiting_flag && !GettingUserInput &&
            (BatchMode || lpCurCmd))
         {
            MessageBeep(0);
            break;
         }

         if(lpLastCmd)    /* there's command history out there! */
         {
            if(!lpCmdHistIndex || lpCmdHistIndex <= lpCmdHist)
            {
               lpCmdHistIndex = lpCmdHist;

               // if we are at the beginning, "rotate" to the end of history

               while(*lpCmdHistIndex)
                  lpCmdHistIndex += lstrlen(lpCmdHistIndex) + 1;
            }

            if(lpCmdHistIndex > lpCmdHist)  // go to PREVIOUS command!
            {
               lpCmdHistIndex --;

               while(lpCmdHistIndex > lpCmdHist)
               {
                  if(*(lpCmdHistIndex - 1)) lpCmdHistIndex--;
                  else                      break;
               }
            }

            if(*lpCmdHistIndex)
            {
               lstrcpy(cmd_buf, lpCmdHistIndex);
               cmd_buf_len = lstrlen(cmd_buf);

               ReDisplayUserInput(cmd_buf);
            }
            else
            {
               MessageBeep(0);
            }
         }
         else
         {
            MessageBeep(0);
         }

         break;


      case IDM_DOWN:    // use COMMAND HISTORY and scroll backward...

         // If we're not in a 'compatible' situation, bleep an error

         if(!pausing_flag && !waiting_flag && !GettingUserInput &&
            (BatchMode || lpCurCmd))
         {
            MessageBeep(0);
            break;
         }

         if(lpLastCmd)    /* there's command history out there! */
         {
            if(!lpCmdHistIndex || !*lpCmdHistIndex)
            {
               lpCmdHistIndex = lpCmdHist;
            }
            else
            {
               lpCmdHistIndex += lstrlen(lpCmdHistIndex) + 1;

               if(!*lpCmdHistIndex) lpCmdHistIndex = lpCmdHist;
            }

            if(*lpCmdHistIndex)
            {
               lstrcpy(cmd_buf, lpCmdHistIndex);
               cmd_buf_len = lstrlen(cmd_buf);

               ReDisplayUserInput(cmd_buf);
            }
            else
            {
               MessageBeep(0);
            }
         }
         else
         {
            MessageBeep(0);
         }

         break;



      case IDM_DELETE:

         // If we're not in a 'compatible' situation, bleep an error

         if(!pausing_flag && !waiting_flag && !GettingUserInput &&
            (BatchMode || lpCurCmd))
         {
            MessageBeep(0);
            break;
         }

         MainOnChar(hWnd, 0x7f, 1);  // send a DELETE character

         break;


      case IDM_INSERT:

         // If we're not in a 'compatible' situation, bleep an error

         if(!pausing_flag && !waiting_flag && !GettingUserInput &&
            (BatchMode || lpCurCmd))
         {
            MessageBeep(0);
            break;
         }

         insert_mode = !insert_mode;       // toggle this thing

         if(own_caret)           // do I own the caret right now??
         {
            if(!caret_hidden) HideCaret(hWnd);

            DestroyCaret();

            CreateCaret(hWnd, (insert_mode?hCaret2:hCaret1), 8, 8);
               // note:  caret created with 'hide count' of 1 (initially)

            if(!caret_hidden)
            {
               ShowCaret(hWnd);
            }
            else if(caret_hidden > 1)
            {
               for(i=1; i < caret_hidden; i++)
               {
                  HideCaret(hWnd);              // same hide count!!
               }
            }
         }


         MoveCursor(curcol, curline);    // this updates the screen...
                                         // and forces a 'caret' update


         break;




      default:
         FORWARD_WM_COMMAND(hWnd, id, hwndCtl, wCodeNotify, DefWindowProc);
   }

}


VOID NEAR _fastcall MainOnSysCommand(HWND hWnd, int cmd, int x, int y)
{



   switch(cmd & 0xfff0)
   {
      case SC_CLOSE:
         CMDExit(NULL,NULL);  /* indicates that it came from a menu! */
         return;

   }

   FORWARD_WM_SYSCOMMAND(hWnd, cmd, x, y, DefWindowProc);

}


VOID FAR PASCAL MainOnChar(HWND hWnd, UINT ch, int cRepeat)
{
int i, j, repeat, iCMDBUF, iTabLength;
LPSTR lp1;



   UpdateStatusBar();       // always udpate status bar on input


   if((ch=='\r' || (ch==26 && GettingUserInput)) && !waiting_flag)
   {
      ctrl_break   = FALSE;         /* in case they were on previously */
      stall_output = FALSE;        /* and something doesn't reset them */

      if(insert_mode) MainOnCommand(hWnd, IDM_INSERT, 0, 0);


      if(GettingUserInput)
      {
          if(ch==26)
          {
             PrintAChar('^');
             PrintAChar('Z');
             PrintString(pCRLF);
             cmd_buf[cmd_buf_len++] = 26;  /* add the <CTRL-Z> */
          }
          else
          {
             PrintString(pCRLF);  /* print the <CR><LF> and update window */
          }

          UpdateWindow(hWnd);

          cmd_buf[cmd_buf_len] = 0;
          lstrcpy(UserInputBuffer, cmd_buf);
          lstrcat(UserInputBuffer, pCRLF);  /* add <CR> <LF> to it! */

          cmd_buf[0] = 0;
          cmd_buf_len = 0;           /* zero command buf */
          GettingUserInput = FALSE;

          return;  /* return now if getting user input */
      }


      if(pausing_flag)          /* we're 'PAUSE'ing - so clear flag */
      {                               /* and return immediately! */
         pausing_flag = FALSE;
         return;
      }

      if(waiting_flag)
      {
         MessageBeep(0);  /* an <ENTER> does not cancel a WAIT! */
         return;
      }

      if(BatchMode || lpCurCmd)  /* if in BATCH mode or processing command */
      {                          /* at this point, put in TypeAhead */

         if(TypeAheadTail>=sizeof(TypeAhead)) TypeAheadTail = 0;

         if((TypeAheadTail + 1)<TypeAheadHead ||
            TypeAheadTail < sizeof(TypeAhead))
         {
            TypeAhead[TypeAheadTail++] = (char)ch;

            if(TypeAheadTail>=sizeof(TypeAhead)) TypeAheadTail = 0;
         }
         else
         {
            MessageBeep(0);
         }

         return;
      }


      PrintString(pCRLF);     /* print the <CR><LF> and update window */
      UpdateWindow(hWnd);

      cmd_buf[cmd_buf_len++] = 0;        /* ensure string is terminated */

      lpCmdHistIndex = NULL;     // any time user presses <ENTER> the
                                 // 'DOSKEY' history index MUST become NULL!

      if(*cmd_buf && !lpLastCmd)
      {
        /* the absolute first time through, when something besides a */
        /* <RETURN> was pressed at the prompt.                       */

         lpLastCmd = lpCmdHist;
         lpCurCmd  = lpCmdHist;


         // while I'm at it, enable the 'copy command history' menu item
         // and the 'erase command history' menu item (and buttons)

         EnableMenuItem(GetMenu(hWnd), IDM_COPY_CMDLINE,
                        MF_ENABLED | MF_BYCOMMAND);

         EnableToolBarButton(IDM_COPY_CMDLINE, 1);

         EnableMenuItem(GetMenu(hWnd), IDM_ERASEHISTORY,
                        MF_ENABLED | MF_BYCOMMAND);

         EnableToolBarButton(IDM_ERASEHISTORY, 1);

      }
      else if(*cmd_buf)  /* assume that cmd_buf has something in it */
      {

               /***********************************************/
               /* find last command in command history buffer */
               /***********************************************/

         for(lpLastCmd = (lp1 = lpCmdHist); *lp1; lp1 += lstrlen(lp1) + 1)
         {
             lpLastCmd = lp1;    /* points to 'last' valid command line */
         }

               /**********************************************/
               /** Ensure there's room for the next command **/
               /**********************************************/

         while(((DWORD)(lpLastCmd + lstrlen(lpLastCmd) + cmd_buf_len + 1)
                - (DWORD)lpCmdHist)>=CMDHIST_SIZE)
         {
            lp1 = lpCmdHist + (i = lstrlen(lpCmdHist) + 1);
            _fmemcpy(lpCmdHist, lp1, CMDHIST_SIZE - i);

            lpLastCmd -= i;

         }


          /** Point to next available command line storage spot **/

         lpCurCmd = lpLastCmd + lstrlen(lpLastCmd) + 1;

      }
      else
      {
         lpCurCmd = cmd_buf;        /* points to zero length string */
      }


      if(*cmd_buf)        /* only if not zero length string, to save time */
      {
         _fmemcpy(lpCurCmd, cmd_buf, cmd_buf_len);
         lpCurCmd[cmd_buf_len] = 0;        /* ensure it's terminated well */
      }

          /*** Since 'lpCurCmd' is NON-NULL, it'll execute! ***/

      cmd_buf_len = 0;
      memset(cmd_buf, 0, sizeof(cmd_buf));

      UpdateWindow(hWnd);

      return;
   }



                 /**************************************/
                 /*** Process Ctrl-C, Ctrl-S, Ctrl-Q ***/
                 /**************************************/

   if(ch=='C'-'A'+1)
   {
      ctrl_break = TRUE;      /* alternate methods! */
      pausing_flag = FALSE;   /* in case we're pausing */

      if(insert_mode) MainOnCommand(hWnd, IDM_INSERT, 0, 0);
   }
   else if(ch=='S'-'A'+1 && !pausing_flag)
   {
      stall_output = TRUE;
   }
   else
   {
      stall_output = FALSE;  /* turn off 'stall' for any other char. */
                             /* or if 'pause'ing.  Just because.     */
   }

              /*********************************************/
              /*** IF CURRENTLY PROCESSING A COMMAND OR  ***/
              /*** IF IN 'BATCH' MODE PROCESS TYPE AHEAD ***/
              /***    (cannot be getting user input!)    ***/
              /*********************************************/


   if(!pausing_flag && !waiting_flag && !GettingUserInput &&
      (BatchMode || lpCurCmd))   /* if in BATCH mode or processing command */
   {                             /* at this point, put in TypeAhead */

      for(repeat=1; repeat<=cRepeat; repeat++)
      {
         if(TypeAheadTail>=sizeof(TypeAhead)) TypeAheadTail = 0;

         if((TypeAheadTail + 1)<TypeAheadHead ||
            TypeAheadTail < sizeof(TypeAhead))
         {
            TypeAhead[TypeAheadTail++] = (char)ch;

            if(TypeAheadTail>=sizeof(TypeAhead)) TypeAheadTail = 0;
         }
         else
         {
            MessageBeep(0);
         }
      }
      return;
   }

                     /***************************/
                     /*** PROCESS CONTROL 'P' ***/
                     /***************************/

        /* note that control-p only works if *NOT* in type-ahead! */

   if(!pausing_flag && !waiting_flag)       /* only if NOT pausing!! */
   {
      if(!ctrl_p)
      {
         if(ch==('P'-'A'+1))  /* ctrl - P */
         {
            ctrl_p = repeat & 1; // odd/even repeats (?)

            ctrl_break = FALSE;  /* just in case... */
            return;
         }
      }
      else           /* CTRL-P allows imbedded special characters */
      {
         ctrl_p = FALSE;


//         if(ch!='\r')  /* not a carriage return - that's the only exception */
//         {
//            cmd_buf[cmd_buf_len++] = (char)ch;
//
//            ctrl_break = FALSE;  /* just in case... */
//
//            PrintAChar(ch | 0x8000); /* forces 'absolute' display mode */
//            return;
//         }

         if(ch != '\r')
         {
            ch |= 0x8000;
         }
      }
   }
               /* process character repeats here */

   for(repeat=1; repeat<=cRepeat; repeat++)
   {
      if(repeat > 1) ch &= 0x7fff;  // if ctrl-p WAS active, clear it!

             /***************************************************/
             /** If 'PAUSING' reject all remaining keystrokes. **/
             /** otherwise, place printable char's into buffer **/
             /***************************************************/

      if(pausing_flag || waiting_flag)   /* 'PAUSE'ing and NOT <ENTER> */
      {
         MessageBeep(0);

           /* if it's a SYSTEM character, forward it by exiting loop */
           /* which continues to the 'FORWARD_WM_CHAR' cracker.      */

         break;
      }


                               /*** ESCAPE ***/

      if(ch=='\x1b')
      {
         cmd_buf_len = 0;
         memset(cmd_buf, 0, sizeof(cmd_buf));

         ReDisplayUserInput(cmd_buf);  // new method - blank line!

         curcol=start_col;
         curline=start_line;

//         PrintErrorMessage(852);  // "!!!\r\n"
//
//         curcol = start_col;
//         if(curcol>0) curcol--;
//         if(curcol>0) curcol--;
//
//         PrintAChar('\\');
//         PrintAChar(' ');
//
//         start_col = curcol;     /* update start positions for cmd line edit */
//         start_line = curline;

         UpdateWindow(hWnd);

         if(repeat>=cRepeat) return;

         continue;  // skip remainder of loop (not needed)
      }

      iCMDBUF = (curline - start_line) * SCREEN_COLS + curcol - start_col;


      if(ch=='\x9')  /* TAB! */
      {
         iTabLength = 8 - (curcol & 7);

         if((cmd_buf_len + iTabLength) >= sizeof(cmd_buf) ||
            ((insert_mode || iCMDBUF==cmd_buf_len) &&
             (cmd_buf_len + iTabLength) >= (sizeof(cmd_buf) - 1)))
         {
            MessageBeep(0);
            return;
         }

         if(!insert_mode)
         {
            MainOnChar(hWnd, ' ', 1);  // the first one 'overwrites'

            if(iTabLength > 1)
            {
               insert_mode = TRUE;

               MainOnChar(hWnd, ' ', iTabLength - 1);  // insert that many spaces!

               insert_mode = FALSE;
            }
         }
         else
         {
            MainOnChar(hWnd, ' ', iTabLength);  // insert that many spaces!
         }

         if(repeat>=cRepeat) return;
      }
      else if(ch=='\x7f') /* DELETE!! like bksp except cursor doesn't move */
      {
         if(!cmd_buf_len || iCMDBUF >= cmd_buf_len)
         {
            // here we're trying to 'delete' at the end of the line.  NO!

            MessageBeep(0);
            return;            // ignore further repeats
         }

         if(cmd_buf_len > (iCMDBUF + 1))
         {
            _hmemmove(cmd_buf + iCMDBUF, cmd_buf + iCMDBUF + 1,
                      cmd_buf_len - iCMDBUF - 1);

            _hmemmove(screen[curline] + curcol,
                      screen[curline] + curcol + 1,
                      cmd_buf_len - iCMDBUF - 1);

            _hmemmove(attrib[curline] + curcol,
                      attrib[curline] + curcol + 1,
                      cmd_buf_len - iCMDBUF - 1);
         }

         cmd_buf[--cmd_buf_len] = 0;

         screen[end_line][end_col] = 0;

         if(end_col) end_col--;
         else if(end_line > start_line)
         {
            end_line --;
            end_col = SCREEN_COLS - 1;
         }

         screen[end_line][end_col] = 0;  // the *NEW* end must be cleared

         MoveCursor(curcol, curline);    // this updates the screen...


         FarCheckScreenScroll(TRUE);

         if(repeat>=cRepeat) return;
      }
      else if(ch=='\x8')  /* backspace!! */
      {
         if(iCMDBUF > 0)  // can't backspace at beginning of line
         {
            if(cmd_buf_len > iCMDBUF)
            {
               _hmemmove(cmd_buf + iCMDBUF - 1, cmd_buf + iCMDBUF,
                         cmd_buf_len - iCMDBUF);

               _hmemmove(screen[curline] + curcol - 1,
                         screen[curline] + curcol,
                         cmd_buf_len - iCMDBUF);

               _hmemmove(attrib[curline] + curcol - 1,
                         attrib[curline] + curcol,
                         cmd_buf_len - iCMDBUF);
            }

            cmd_buf[--cmd_buf_len] = 0;

            if(end_col) end_col--;
            else if(end_line > start_line)
            {
               end_line --;
               end_col = SCREEN_COLS - 1;
            }

            screen[end_line][end_col] = 0;  // the *NEW* end must be cleared

            // back up to previous character...

            if(curcol)
            {
               MoveCursor(curcol - 1, curline);
            }
            else if(curline)
            {
               MoveCursor(SCREEN_COLS - 1, curline - 1);
            }
         }
         else
         {
            MessageBeep(0);
            return;
         }

         FarCheckScreenScroll(TRUE);

         if(repeat>=cRepeat) return;
      }
      else if((ch>=' ' && ch<=0x7e) || (ch>=0xa0 && ch<=0xfe) ||
              (ch & 0x8000))
      {
         if((iCMDBUF == cmd_buf_len || insert_mode) &&
            cmd_buf_len >= (sizeof(cmd_buf) - 1))
         {
            MessageBeep(0);
            return;
         }


         if(insert_mode)
         {
            if(cmd_buf_len > iCMDBUF)
            {
               memmove(cmd_buf + iCMDBUF + 1,
                       cmd_buf + iCMDBUF,
                       cmd_buf_len - iCMDBUF);
            }

            cmd_buf[iCMDBUF] = (char)ch;
            cmd_buf_len ++;

            cmd_buf[cmd_buf_len] = 0;


            // MOVE CURSOR FORWARD!

            if(curcol >= (SCREEN_COLS - 1))
            {
               MoveCursor(0, curline + 1);
            }
            else
            {
               MoveCursor(curcol + 1, curline);
            }

            ReDisplayUserInput0(cmd_buf);

            FarCheckScreenScroll(TRUE);        // for now...
         }
         else  // 'overwrite' or 'append' mode
         {
            PrintAChar(ch);

            cmd_buf[iCMDBUF] = (char)ch;

            if(iCMDBUF >= cmd_buf_len)  // '>=' is a 'caveat'
            {
               cmd_buf_len++;
               cmd_buf[cmd_buf_len] = 0;

               end_line = curline;
               end_col  = curcol;
            }
         }

         if(repeat>=cRepeat) return;
      }
      else
      {
         // other characters are ignored...

         break;
      }

   }

   FORWARD_WM_CHAR(hWnd, ch, cRepeat, DefWindowProc);
}


BOOL NEAR _fastcall MainCheckScrollBars(HWND hWnd)
{
RECT r;
int vmin, vmax, hmin, hmax;
int lines_per_screen, columns_per_screen;
int actual_text_height, actual_text_width;
BOOL WasChange;
static int BandAidRecurse = 0;  // a patch - hence the name!



   if(!TMFlag) UpdateTextMetrics(hWnd);

   GetClientRect(hWnd, (LPRECT)&r);

   WasChange = FALSE;

   actual_text_height = tmMain.tmHeight + tmMain.tmExternalLeading;
   lines_per_screen = (r.bottom - r.top - 1) / actual_text_height;

   actual_text_width = tmMain.tmAveCharWidth;
   columns_per_screen  = (r.right - r.left) / actual_text_width;


   GetScrollRange(hWnd, SB_HORZ, (LPINT)&hmin, (LPINT)&hmax);
   GetScrollRange(hWnd, SB_VERT, (LPINT)&vmin, (LPINT)&vmax);

   if(columns_per_screen < SCREEN_COLS)
   {
      if(hmin!=0 || hmax!=(SCREEN_COLS - columns_per_screen))
      {
         hmin = 0;
         hmax = SCREEN_COLS - columns_per_screen;

         SetScrollPos(hWnd, SB_HORZ, 0, FALSE);
         SetScrollRange(hWnd, SB_HORZ, 0, hmax, TRUE);
         SetScrollPos(hWnd, SB_HORZ, 0, TRUE);

         WasChange = TRUE;

         LineScrollCount = 0x8000;  // prevents scrolling lines on screen
      }
   }
   else
   {
      if(hmin!=0 || hmax!=0)
      {
         SetScrollPos(hWnd, SB_HORZ, 0, FALSE);
         SetScrollRange(hWnd, SB_HORZ, 0, 0, TRUE);
         GetScrollRange(hWnd, SB_HORZ, (LPINT)&hmin, (LPINT)&hmax);

         if(hmin==0 && hmax==0)
         {
            WasChange = TRUE;
         }
         else if(BandAidRecurse<3 && !WasChange)
         {
            WasChange = TRUE;
            BandAidRecurse++;
         }

         hmin = 0;
         hmax = 0;

         LineScrollCount = 0x8000;  // prevents scrolling lines on screen
      }
   }

   if(WasChange) return(TRUE);  // causes lines per screen to be updated
                                // before any scrollbars are drawn!


   if(lines_per_screen <= maxlines)
   {
      if(vmin!=0 || vmax!=(maxlines - lines_per_screen + 1))
      {
         if(LineScrollCount != 0x8000)
         {
            LineScrollCount += (maxlines - lines_per_screen + 1) - vmax;
         }

         vmin = 0;
         vmax = maxlines - lines_per_screen + 1;

         SetScrollPos(hWnd, SB_VERT, 0, FALSE);
         SetScrollRange(hWnd, SB_VERT, 0, vmax, TRUE);

            /* since setting scroll range might send 'WM_SIZE' */
            /* make sure my range didn't change in between!!   */

         GetScrollRange(hWnd, SB_VERT, (LPINT)&vmin, (LPINT)&vmax);

         SetScrollPos(hWnd, SB_VERT, vmax, TRUE);

         WasChange = TRUE;
      }
   }
   else
   {
      if(vmin!=0 || vmax!=0)
      {
         SetScrollPos(hWnd, SB_VERT, 0, FALSE);
         SetScrollRange(hWnd, SB_VERT, 0, 0, TRUE);
         GetScrollRange(hWnd, SB_VERT, (LPINT)&vmin, (LPINT)&vmax);

         if(vmin==0 && vmax==0)
         {
            WasChange = TRUE;
         }
         else if(BandAidRecurse<3 && !WasChange)
         {
            WasChange = TRUE;
            BandAidRecurse++;
         }

         vmin = 0;
         vmax = 0;

         LineScrollCount = 0x8000;  // prevents scrolling lines on screen
      }
   }


   if(!WasChange) BandAidRecurse = 0;  // setup for NEXT TIME!

   return(WasChange);

}



VOID NEAR _fastcall MainOnPaint(HWND hWnd)
{
HFONT hOldFont;
PAINTSTRUCT ps;
HDC hDC;
RECT r, r2;
int lines_per_screen, columns_per_screen;
int actual_text_height, actual_text_width;
int vpos, hpos;
int last_attr = -1;
int i, i0, j, j0, k, k0, x, y, caretx, carety;
static char temp_buf[SCREEN_COLS + 1];
BOOL caret_ok=FALSE; /* assigned 'TRUE' if the current row,col is displayed */

static COLORREF text_colors[16]={
     RGB(0,0,0),       RGB(128,0,0),  RGB(0,128,0),  RGB(128,96,0),
     RGB(0,0,128),     RGB(128,0,128),RGB(0,128,128),RGB(192,192,192),
     RGB(128,128,128), RGB(255,0,0),  RGB(0,255,0),  RGB(255,255,0),
     RGB(0,0,255),     RGB(255,0,255),RGB(0,255,255),RGB(255,255,255)};
static COLORREF back_colors[8]={
     RGB(0,0,0),   RGB(128,0,0),  RGB(0,128,0),  RGB(128,128,0),
     RGB(0,0,128), RGB(128,0,128),RGB(0,128,128),RGB(192,192,192)};




   /** Step 1:  Initiate paint process by checking flags & hiding caret **/
   /**          and obtaining TEXT METRICS (if it hasn't been done yet) **/


   if(own_caret && !caret_hidden)
   {
      HideCaret(hWnd);  /* I force this initially... */
      caret_hidden++;
   }


   while(MainCheckScrollBars(hWnd))
       ;   /* ensure scroll bars are up-to-date! */
           /* also forces 'tmMain' to update if needed */



       /** Step 2:   Determine screen size & position **/

   GetClientRect(hWnd, (LPRECT)&r);

   actual_text_height = tmMain.tmHeight + tmMain.tmExternalLeading;
   lines_per_screen = (r.bottom - r.top - 1) / actual_text_height;

   actual_text_width = tmMain.tmAveCharWidth;
   columns_per_screen  = (r.right - r.left) / actual_text_width;



   vpos = GetScrollPos(hWnd, SB_VERT);
   hpos = GetScrollPos(hWnd, SB_HORZ);




       /** STEP 3:  Write text for the current visible window! **/

   UpdateStatusBar();                    // re-paints status bar, if needed!


   // see if the clip region is outside of the client area.  If it is,
   // call the 'NCPaint' handler to paint the non-client areas.

   GetUpdateRect(hWnd, &r2, FALSE);
   if(r2.left < r.left || r2.right > r.right ||
      r2.top < r.top || r2.bottom > r.bottom)
   {
      MainOnNCPaint(hWnd, 0, 0);
   }


   if(!(hDC = BeginPaint(hWnd, (LPPAINTSTRUCT)&ps)))  return;

   if(!hMainFont)
   {
      hOldFont = SelectObject(hDC, DEFAULT_FONT);
   }
   else
   {
      hOldFont = SelectObject(hDC, hMainFont);
   }


   SetTextAlign(hDC, TA_TOP | TA_LEFT);  /* left, top alignment */
   SetBkMode(hDC, OPAQUE);  /* destroy old background before printing */

   for(i=vpos, i0=0; i0<=lines_per_screen; i++,i0++)
   {
      memset(temp_buf, 0, sizeof(temp_buf));  /* zero string buffer! */
      y = i0 * actual_text_height;
      x = 0;
      k = 0;

      if(i==curline && hpos<=curcol &&
         (hpos + columns_per_screen)>=curcol)
      {
         caret_ok = TRUE;   /* it's ok now to display the caret!! */
         caretx   = (curcol - hpos) * actual_text_width;
         carety   = y - 8 + 1 + tmMain.tmHeight;  /* one dot below font */
      }

      for(j=hpos, j0=0; j0<=columns_per_screen; j++,j0++)
      {
         if(   (i0==0 && j0==0)
            || (i<SCREEN_LINES && j<SCREEN_COLS
                 && last_attr!=attrib[i][j])
            || ((i>=SCREEN_LINES || j>=SCREEN_COLS)
                && last_attr!=cur_attr))
         {
            if((i0 || j0) &&(*temp_buf != 0))/* there's text; print it!! */
            {

               TextOut(hDC, x, y, temp_buf, k);

               x = j0 * actual_text_width;
               memset(temp_buf, 0, sizeof(temp_buf));
               k = 0;
            }

            if(i<SCREEN_LINES && j<SCREEN_COLS)
            {
               last_attr = attrib[i][j];
            }

            SetTextColor(hDC, text_colors[last_attr & 0xf]);
            SetBkColor(hDC, back_colors[(last_attr & 0x70)>>4]);
         }

         if(i>=SCREEN_LINES || j>=SCREEN_COLS || (k0 = screen[i][j])==0)
         {
            temp_buf[k++] = ' ';
         }
         else
         {
            temp_buf[k++] = (char)k0;
         }

      }

      TextOut(hDC, x, y, temp_buf, k);
   }


                            /*** DONE! ***/

   SelectObject(hDC, hOldFont);  /* restore old font to DC */

   EndPaint(hWnd, (LPPAINTSTRUCT)&ps);


   InvalidateFlag = FALSE;      /* resets 'entire client invalid' flag */

   if(own_caret && caret_ok)    /* if it's safe to display the caret! */
   {
      SetCaretPos(caretx,carety);
      while(((int)caret_hidden)>0)
      {
         ShowCaret(hWnd);         /* this is a cumulative operation! */
         caret_hidden--;
      }
      caret_hidden = 0;
   }

}


VOID NEAR _fastcall MainOnVScroll(HWND hWnd, HWND hwndCtl, UINT wCode,
                                  int pos)
{
RECT r;
int sb_pos, min, max;
int lines_per_screen, actual_text_height;



   switch(wCode)
   {
      case SB_THUMBTRACK:
      case SB_THUMBPOSITION:

         SetScrollPos(hWnd, SB_VERT, pos, TRUE);
         break;


      case SB_LINEDOWN:
      case SB_LINEUP:

         sb_pos = GetScrollPos(hWnd, SB_VERT);
         GetScrollRange(hWnd, SB_VERT, (LPINT)&min, (LPINT)&max);

         sb_pos += wCode==SB_LINEDOWN?1:-1;

         if(sb_pos<min) sb_pos=min;
         if(sb_pos>max) sb_pos=max;

         SetScrollPos(hWnd, SB_VERT, sb_pos, TRUE);
         break;


      case SB_PAGEDOWN:
      case SB_PAGEUP:

         GetClientRect(hWnd, (LPRECT)&r);

         actual_text_height = tmMain.tmHeight + tmMain.tmExternalLeading;
         lines_per_screen = (r.bottom - r.top - 1) / actual_text_height;


         sb_pos = GetScrollPos(hWnd, SB_VERT);
         GetScrollRange(hWnd, SB_VERT, (LPINT)&min, (LPINT)&max);


         if(wCode==SB_PAGEDOWN)
            sb_pos += lines_per_screen - 1;
         else
            sb_pos -= lines_per_screen - 1;

         if(sb_pos<min) sb_pos=min;
         if(sb_pos>max) sb_pos=max;

         SetScrollPos(hWnd, SB_VERT, sb_pos, TRUE);
         break;


      case SB_BOTTOM:
      case SB_TOP:

         sb_pos = GetScrollPos(hWnd, SB_VERT);
         GetScrollRange(hWnd, SB_VERT, (LPINT)&min, (LPINT)&max);

         sb_pos = wCode==SB_BOTTOM?max:min;

         SetScrollPos(hWnd, SB_VERT, sb_pos, TRUE);
         break;


      default:
         FORWARD_WM_VSCROLL(hWnd, hwndCtl, wCode, sb_pos, DefWindowProc);
         return;
   }


   if(!InvalidateFlag)
   {
      GetClientRect(hWnd, (LPRECT)&r);
      InvalidateRect(hWnd, (LPRECT)&r, FALSE);
      InvalidateFlag = TRUE;
   }

}


VOID NEAR _fastcall MainOnHScroll(HWND hWnd, HWND hwndCtl, UINT wCode,
                                  int pos)
{
RECT r;
int sb_pos, min, max;

   switch(wCode)
   {
      case SB_THUMBTRACK:
      case SB_THUMBPOSITION:

         SetScrollPos(hWnd, SB_HORZ, pos, TRUE);
         break;


      case SB_LINEDOWN:
      case SB_LINEUP:

         sb_pos = GetScrollPos(hWnd, SB_HORZ);
         GetScrollRange(hWnd, SB_HORZ, (LPINT)&min, (LPINT)&max);

         sb_pos += wCode==SB_LINEDOWN?1:-1;

         if(sb_pos<min) sb_pos=min;
         if(sb_pos>max) sb_pos=max;

         SetScrollPos(hWnd, SB_HORZ, sb_pos, TRUE);
         break;


      case SB_PAGEDOWN:
      case SB_PAGEUP:

         sb_pos = GetScrollPos(hWnd, SB_HORZ);
         GetScrollRange(hWnd, SB_HORZ, (LPINT)&min, (LPINT)&max);

         sb_pos += wCode==SB_PAGEDOWN?8:-8;

         if(sb_pos<min) sb_pos=min;
         if(sb_pos>max) sb_pos=max;

         SetScrollPos(hWnd, SB_HORZ, sb_pos, TRUE);
         break;


      case SB_BOTTOM:
      case SB_TOP:

         sb_pos = GetScrollPos(hWnd, SB_HORZ);
         GetScrollRange(hWnd, SB_HORZ, (LPINT)&min, (LPINT)&max);

         sb_pos = wCode==SB_BOTTOM?max:min;

         SetScrollPos(hWnd, SB_HORZ, sb_pos, TRUE);
         break;


      default:
         FORWARD_WM_HSCROLL(hWnd, hwndCtl, wCode, sb_pos, DefWindowProc);
         return;
   }


   if(!InvalidateFlag)
   {
      GetClientRect(hWnd, (LPRECT)&r);
      InvalidateRect(hWnd, (LPRECT)&r, FALSE);
      InvalidateFlag = TRUE;
   }

}


// TOOLBAR 'HINT' WINDOW PROC

LRESULT CALLBACK EXPORT ToolBarHintProc(HWND hWnd, UINT message, WPARAM wParam,
                                        LPARAM lParam)
{
HDC hDC;
HFONT hOldFont;
DWORD dw1; /* , dwDialogUnits; */
RECT rct;
HBRUSH hBrush;
PAINTSTRUCT ps;
COLORREF clrfg;
int bkmode;
char tbuf[128];
static HFONT hHintFont = NULL;



   switch(message)
   {
      case WM_CREATE:

         dw1 = GetVersion();
         dw1 = ((dw1 & 0xff) << 8) + ((dw1 & 0xff00) >> 8);

         if(!hHintFont)
         {
            hHintFont = CreateFont(-9, 0, 0, 0, FW_NORMAL, 0, 0, 0,
                                   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                   CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                   DEFAULT_PITCH | FF_DONTCARE,
                                   dw1<=0x300 ? "Helv" : "MS Sans Serif");
         }

//         dwDialogUnits = GetDialogBaseUnits();

         GetWindowText(hWnd, tbuf, sizeof(tbuf));
         hDC = GetDC(hWnd);

         hOldFont = SelectObject(hDC, hHintFont);

         dw1 = GetTextExtent(hDC, tbuf, lstrlen(tbuf));

         if(hOldFont) SelectObject(hDC, hOldFont);
         ReleaseDC(hWnd, hDC);

         SetWindowPos(hWnd, HWND_TOP, 0, 0,
                      LOWORD(dw1) + LOWORD(dwDialogUnits) / 2,
                      HIWORD(dw1) + HIWORD(dwDialogUnits) / 4,
                      SWP_NOMOVE | SWP_NOACTIVATE);

         if(GetFocus() != GetParent(hWnd))
            SetFocus(GetParent(hWnd));      // ensure parent keeps focus

         InvalidateRect(hWnd, NULL, TRUE);  // ensure it paints

         return(FALSE);



      case WM_PAINT:

         if(!GetUpdateRect(hWnd, NULL, TRUE)) return(0);  // no need to paint

         hDC = BeginPaint(hWnd, &ps);

         if(IsChicago || IsNT)
         {
            clrfg = SetTextColor(hDC, GetSysColor(COLOR_INFOTEXT));
         }
         else
         {
            clrfg = SetTextColor(hDC, RGB(0,0,0));  // black
         }

         GetClientRect(hWnd, &rct);

         GetWindowText(hWnd, tbuf, sizeof(tbuf));

         bkmode = SetBkMode(hDC, TRANSPARENT);

         hOldFont = SelectObject(hDC, hHintFont);

         DrawText(hDC, tbuf, lstrlen(tbuf), &rct,
                  DT_CENTER | DT_SINGLELINE | DT_VCENTER | DT_NOCLIP);


         if(hOldFont) SelectObject(hDC, hOldFont);

         SetBkMode(hDC, bkmode);
         SetTextColor(hDC, clrfg);

         EndPaint(hWnd, &ps);
         return(0);



      case WM_ERASEBKGND:

         if(IsChicago || IsNT)
         {
            hBrush = CreateSolidBrush(GetSysColor(COLOR_INFOBK));
         }
         else
         {
            hBrush = CreateSolidBrush(RGB(255,255,127));
         }

         GetClientRect(hWnd, &rct);

         FillRect((HDC)wParam, &rct, hBrush);

         DeleteObject(hBrush);
         return(TRUE);   // flag 'I erased background'



      case WM_DESTROY:

         DeleteObject(hHintFont);
         hHintFont = NULL;

         hwndToolBarHint = NULL;
         break;
   }

   return(DefWindowProc(hWnd, message, wParam, lParam));

}


/***************************************************************************/
/*                                                                         */
/*            'ABOUT' Dialog Processing (functions, etc.)                  */
/*                                                                         */
/***************************************************************************/


#pragma code_seg("DIALOG_TEXT","CODE")


BOOL CALLBACK EXPORT About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{

   switch (message)
   {
      case WM_INITDIALOG:

         SubclassStaticControls(hDlg);

         return(TRUE);



      case WM_CTLCOLOR:
         return((BOOL)DlgOn_WMCtlColor(hDlg, wParam, lParam));



      case WM_ERASEBKGND:

         {
          HDC hdcBackGround = (HDC)wParam;
          RECT rctClient;

            GetClientRect(hDlg, &rctClient);

            FillRect(hdcBackGround, &rctClient, hbrDlgBackground);
         }

         return(TRUE);



      case WM_COMMAND:
         if ((WORD)wParam == IDOK || (WORD)wParam == IDCANCEL)
         {
            EndDialog(hDlg, TRUE);
            return (TRUE);
         }
         break;
   }

   return (FALSE);
}




/***************************************************************************/
/*                                                                         */
/*          'ASSOCIATE' Dialog Processing (functions, etc.)                */
/*                                                                         */
/***************************************************************************/


BOOL CALLBACK EXPORT AssociateDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
char tbuf[512], tbuf2[512], tbuf3[128];
HKEY hKeyRoot;
WORD w1, w2;
DWORD dwIndex;
LPSTR lp1, lp2;
long l, l2;
static BOOL BlockChangeFlag = FALSE;


   switch (message)
   {
      case WM_INITDIALOG:

         SubclassStaticControls(hDlg);

         if(!lpRegOpenKey)
         {
            EnableWindow(GetDlgItem(hDlg, IDC_APPLIST), FALSE);
            return(TRUE);
         }

         BlockChangeFlag = TRUE;


         // SETTING TABS FOR LISTBOX CONTROL

         ((int FAR *)tbuf)[0] = 120;  // approximately 1/2 way across
         ((int FAR *)tbuf)[1] = 240;  // everything after 2nd tab is off screen

         SendDlgItemMessage(hDlg, IDC_APPLIST, LB_SETTABSTOPS,
                            (WPARAM)2, (LPARAM)(LPVOID)tbuf);



         // ENUMERATING THE REGISTRY
         //
         // THINGS TO LOOK FOR:
         //   1) key does not start with '.'
         //   2) key's 'shell\open\command' or 'shell\print\command' path
         //      for '.EXE' file name.
         //   3) if (2) not present, key's 'protocol\StdExecute\server' or
         //      'protocol\StdFileEditing\server' path for '.EXE' file name

         w1 = (WORD)GetVersion();
         w1 = ((w1 & 0xff) << 8) | ((w1 >> 8) & 0xff);

         if(w1 <= 0x350 && !(GetVersion() & 0x80000000))
         {  // windows 3.1, 3.11, etc. (not NT!)
            // use the "old" value for HKEY_CLASSES_ROOT as defined in the
            // Win 3.1 SDK...

            w1 = lpRegOpenKey((HKEY)1, NULL, &hKeyRoot)==ERROR_SUCCESS;
         }
         else
         {
            w1 = lpRegOpenKey(HKEY_CLASSES_ROOT, NULL, &hKeyRoot)==ERROR_SUCCESS;

            if(!w1)
            {
               // use the "old" value for HKEY_CLASSES_ROOT as defined in
               // the Win 3.1 SDK...

               w1 = lpRegOpenKey((HKEY)1, NULL, &hKeyRoot)==ERROR_SUCCESS;
            }
         }


         if(w1)
         {
            dwIndex = 0;
            while(lpRegEnumKey(hKeyRoot, dwIndex++, tbuf, sizeof(tbuf))
                  == ERROR_SUCCESS)
            {
               if(*tbuf=='.') continue;

               l = sizeof(tbuf2) - 1;
               if(lpRegQueryValue(hKeyRoot, tbuf, tbuf2, &l)==ERROR_SUCCESS)
               {
                  tbuf2[l] = 0;

                  _fstrtrim(tbuf2);
                  l = lstrlen(tbuf2);  // just for fun...
                  if(!l) continue;     // skip blank values

                  tbuf2[l++] = '\t';
                  tbuf2[l] = 0;

                  // At this point I may decide to store 'tbuf's value, but
                  // for now, not yet!!

                  w1 = lstrlen(tbuf);

                  lstrcpy(tbuf + w1, "\\shell\\open\\command");
                  l2 = sizeof(tbuf2) - l - 1;

                  if(lpRegQueryValue(hKeyRoot, tbuf, tbuf2 + l, &l2)
                     ==ERROR_SUCCESS)
                  {
                     tbuf2[l + l2] = 0;
                     l2 = lstrlen(tbuf2 + l);  // just for fun...

                     tbuf2[l + l2] = 0;
                  }
                  else
                  {
                     lstrcpy(tbuf + w1, "\\shell\\print\\command");
                     l2 = sizeof(tbuf2) - l - 1;

                     if(lpRegQueryValue(hKeyRoot, tbuf, tbuf2 + l, &l2)
                        ==ERROR_SUCCESS)
                     {
                        tbuf2[l + l2] = 0;
                        l2 = lstrlen(tbuf2 + l);  // just for fun...

                        tbuf2[l + l2] = 0;
                     }
                     else
                     {
                        lstrcpy(tbuf + w1, "\\protocol\\StdExecute\\server");
                        l2 = sizeof(tbuf2) - l - 1;

                        if(lpRegQueryValue(hKeyRoot, tbuf, tbuf2 + l, &l2)
                           ==ERROR_SUCCESS)
                        {
                           tbuf2[l + l2] = 0;
                           l2 = lstrlen(tbuf2 + l);  // just for fun...

                           tbuf2[l + l2] = 0;
                        }
                        else
                        {
                           lstrcpy(tbuf + w1, "\\protocol\\StdFileEditing\\server");
                           l2 = sizeof(tbuf2) - l - 1;

                           if(lpRegQueryValue(hKeyRoot, tbuf, tbuf2 + l, &l2)
                              ==ERROR_SUCCESS)
                           {
                              tbuf2[l + l2] = 0;
                              l2 = lstrlen(tbuf2 + l);  // just for fun...

                              tbuf2[l + l2] = 0;
                           }
                           else
                           {
                              continue;  // ignore this element (no method)
                           }
                        }
                     }
                  }

                  tbuf[w1] = 0;     // once again, get original key back!

                  lstrcat(tbuf2, "\t");
                  lstrcat(tbuf2, tbuf);

                  SendDlgItemMessage(hDlg, IDC_APPLIST, LB_ADDSTRING, 0,
                                     (DWORD)(LPSTR)tbuf2);
               }
            }

            lpRegCloseKey(hKeyRoot);

         }

         // Now, search through the 'WIN.INI' file for additional file types
         // assigned through the '[extensions]' key



         // {reserved}



         // FINAL STEP:  place '(NONE)' at the top of the list of
         //              applications and select it.

         SendDlgItemMessage(hDlg, IDC_APPLIST, LB_INSERTSTRING, 0,
                            (DWORD)(LPSTR)"(NONE)" );


         // select nothing in the list box at this time

         SendDlgItemMessage(hDlg, IDC_APPLIST, LB_SETCURSEL, (WPARAM)-1, 0);

         BlockChangeFlag = FALSE;
         return (TRUE);


      case WM_CTLCOLOR:
         return((BOOL)DlgOn_WMCtlColor(hDlg, wParam, lParam));



      case WM_ERASEBKGND:

         {
          HDC hdcBackGround = (HDC)wParam;
          RECT rctClient;

            GetClientRect(hDlg, &rctClient);

            FillRect(hdcBackGround, &rctClient, hbrDlgBackground);
         }

         return(TRUE);


      case WM_COMMAND:

         if((WORD)wParam == IDOK)
         {
            // look in 'IDC_ASSOCIATE' to see what I have there...
            // I should either have a document type, or a program name
            // For now, only do DOCUMENT TYPES.  Handle program names
            // later by forcing the user to add registry keys for it.

            GetDlgItemText(hDlg, IDC_ASSOCIATE, tbuf2, sizeof(tbuf2));
            _fstrtrim(tbuf2);

            GetDlgItemText(hDlg, IDC_EXTENSION, tbuf3, sizeof(tbuf3));
            _fstrtrim(tbuf3);

            if(!*tbuf3)
            {
               MessageBox(hDlg, "?No extension selected",
                          "* SFTShell ASSOCIATE ERROR *",
                          MB_OK | MB_ICONHAND);

               return(TRUE);
            }

            if(*tbuf3 != '.')
            {
               MessageBox(hDlg, "?Extension must begin with a '.'",
                          "* SFTShell ASSOCIATE ERROR *",
                          MB_OK | MB_ICONHAND);

               return(TRUE);
            }

            if(!*tbuf2)
            {
               // associate with *NONE* - this means remove any existing
               // keys that might match this file...


               if(lpRegDeleteKey)
               {
                  w1 = lpRegDeleteKey(HKEY_CLASSES_ROOT, tbuf3)==ERROR_SUCCESS;

                  if(!w1)
                  {
                     w1 = lpRegDeleteKey((HKEY)1, tbuf3)==ERROR_SUCCESS;
                  }
               }
               else
               {
                  w1 = FALSE;
               }

               if(!w1)  // key not deleted (must not have existed in registry)
               {
                  // {reserved} - remove from WIN.INI
               }
            }
            else  // new value to be assigned for extension
            {

               // find out which registry key applies for the document type

               w2 = (WORD)SendDlgItemMessage(hDlg, IDC_APPLIST,
                                             LB_GETCOUNT, 0, 0);

               lp2 = NULL;  // this remains NULL if no key found...

               if(lpRegSetValue)
               {
                  // see if one of the list box entries matches...

                  for(w1=1; w1<w2; w1++)
                  {
                     SendDlgItemMessage(hDlg, IDC_APPLIST, LB_GETTEXT,
                                        (WORD)w1, (LPARAM)(LPSTR)tbuf);

                     lp1 = _fstrchr(tbuf, '\t');
                     if(lp1)
                     {
                        *(lp1++) = 0;

                        lp2 = lp1;

                        lp1 = _fstrchr(lp2, '\t');

                        if(lp1)
                        {
                           lp2 = lp1 + 1;
                        }
                        else
                        {
                           *lp2 = 0;
                        }
                     }
                     else
                     {
                        lp2 = lp1 + lstrlen(lp1);
                     }

                     // at this point 'tbuf' is description and
                     // 'lp2' points to the registry key for it

                     lp1 = _fstrchr(lp2, '\\');  // find first backslash in key
                     if(lp1) *lp1 = 0;           // terminate at that point


                     if(*tbuf && !_fstricmp(tbuf, tbuf2))  // A MATCH!
                     {
                        break; // at this point, lp2 points to the key
                     }

                     lp2 = NULL;  // this acts like a 'FOUND flag'
                  }

                  if(lp2)         // found!  change 'tbuf3's entry to 'lp2'
                  {
                     if(lpRegSetValue(HKEY_CLASSES_ROOT, tbuf3, REG_SZ,
                                      lp2, lstrlen(lp2) + 1) != ERROR_SUCCESS)
                     {
                        // try again, using the "old" HKEY_CLASS_ROOT value
                        if(lpRegSetValue((HKEY)1, tbuf3, REG_SZ,
                                         lp2, lstrlen(lp2) + 1)
                           != ERROR_SUCCESS)
                        {
                           MessageBox(hDlg, "?Unable to create/modify registry",
                                      "* SFTShell ASSOCIATE ERROR *",
                                      MB_OK | MB_ICONHAND);

                           return(TRUE);
                        }
                     }
                  }
               }


               // document type not recognized - assume WIN.INI

               if(!lp2)           // no matching entry found in list box
               {

                  // {reserved}



                  // TEMPORARY - later, do something with WIN.INI

                  wsprintf(tbuf, "?Unrecognized document type - \"%s\"",
                           (LPSTR)tbuf2);

                  MessageBox(hDlg, tbuf, "* SFTShell ASSOCIATE ERROR *",
                             MB_OK | MB_ICONHAND);

                  return(TRUE);
               }
            }


            EndDialog(hDlg, TRUE);         // FOR NOW...
            return (TRUE);
         }
         else if((WORD)wParam == IDCANCEL)
         {
            EndDialog(hDlg, TRUE);
            return (TRUE);
         }
         else if((WORD)wParam == IDC_BROWSE)
         {
            if(MyGetOpenFileName("Browse File", tbuf, 1)) break;
         }
         else if((WORD)wParam == IDC_EXTENSION)
         {
            if(HIWORD(lParam)==EN_CHANGE)
            {
               BlockChangeFlag = TRUE;

               // initially, select '(None)' in the list box

               SendDlgItemMessage(hDlg, IDC_APPLIST, LB_SETCURSEL, 0, 0);

               BlockChangeFlag = FALSE;


               if(lpRegQueryValue)
               {
                  GetDlgItemText(hDlg, IDC_EXTENSION, tbuf, sizeof(tbuf));

                  l = sizeof(tbuf3) - 1;
                  w1 = lpRegQueryValue(HKEY_CLASSES_ROOT, tbuf,
                                       tbuf3, &l)==ERROR_SUCCESS;
                  if(!w1)
                  {
                     // use the "old" value for HKEY_CLASSES_ROOT as defined in the
                     // Win 3.1 SDK...

                     l = sizeof(tbuf3) - 1;
                     w1 = lpRegQueryValue((HKEY)1, tbuf,
                                          tbuf3, &l)==ERROR_SUCCESS;

                     if(w1)  // extension found!  get application description
                     {
                        tbuf3[l] = 0;
                        l = sizeof(tbuf2) - 1;

                        w1 = lpRegQueryValue((HKEY)1, tbuf3,
                                             tbuf2, &l)==ERROR_SUCCESS;
                     }
                  }
                  else
                  {
                     tbuf3[l] = 0;
                     l = sizeof(tbuf2) - 1;

                     w1 = lpRegQueryValue(HKEY_CLASSES_ROOT, tbuf3,
                                          tbuf2, &l)==ERROR_SUCCESS;
                  }

                  if(w1)  // search for matching key in registry
                  {
                     tbuf2[l] = 0;


                     SetDlgItemText(hDlg, IDC_ASSOCIATE, tbuf2);

                     // look through each item in the list box and see if
                     // any of them match.  if THIS is the case, assign
                     // the 'current selection' to the matching entry.


                     w2 = (WORD)SendDlgItemMessage(hDlg, IDC_APPLIST,
                                                   LB_GETCOUNT, 0, 0);

                     for(w1=1; w1<w2; w1++)
                     {
                        SendDlgItemMessage(hDlg, IDC_APPLIST, LB_GETTEXT,
                                           (WORD)w1, (LPARAM)(LPSTR)tbuf);

                        lp1 = _fstrchr(tbuf, '\t');
                        if(lp1)
                        {
                           lp1++;
                           lstrcpy(tbuf, lp1);

                           lp1 = _fstrchr(tbuf, '\t');

                           if(lp1)
                           {
                              lstrcpy(tbuf, lp1 + 1);
                           }
                           else
                           {
                              *tbuf = 0;
                           }
                        }
                        else
                        {
                           *tbuf = 0;
                        }

                        lp1 = _fstrchr(tbuf, '\\'); // find first backslash
                        if(lp1) *lp1 = 0;           // terminate at that point

                        _fstrtrim(tbuf);
                        _fstrtrim(tbuf3);

                        if(*tbuf && !_fstricmp(tbuf, tbuf3))  // A MATCH!
                        {
                           BlockChangeFlag = TRUE;

                           SendDlgItemMessage(hDlg, IDC_APPLIST,
                                              LB_SETCURSEL, (WPARAM)w1, 0);

                           BlockChangeFlag = FALSE;

                           break;
                        }
                     }

                     return(TRUE);  // I processed this! Yay!
                  }
               }

               // no matching key found in registry - look in 'WIN.INI'

               // {reserved}


               // if we get here, it wasn't found...

               SetDlgItemText(hDlg, IDC_ASSOCIATE, "");  // blank string

               return(TRUE);
            }
         }
         else if((WORD)wParam == IDC_APPLIST)
         {
            if(HIWORD(lParam)==LBN_SELCHANGE &&
               !BlockChangeFlag)
            {
               w1 = (WORD)SendDlgItemMessage(hDlg, IDC_APPLIST, LB_GETCURSEL,
                                             0, 0);
               if(w1==0 || w1==LB_ERR)
               {
                  *tbuf = 0;
               }
               else
               {
                  SendDlgItemMessage(hDlg, IDC_APPLIST, LB_GETTEXT,
                                     (WORD)w1, (LPARAM)(LPSTR)tbuf);

                  lp1 = _fstrchr(tbuf, '\t');
                  if(lp1) *lp1 = 0;
               }

               _fstrtrim(tbuf);  // ensure string is 'trimmed'

               // assign the 'application type' to the 'associate' edit box

               SetDlgItemText(hDlg, IDC_ASSOCIATE, tbuf);
            }
         }

         break;

   }

   return (FALSE);
}



/***************************************************************************/
/*                                                                         */
/*            'EXTERROR' Dialog Processing (functions, etc.)               */
/*                                                                         */
/***************************************************************************/

WORD FAR PASCAL MyErrorMessageBox(HWND hWnd, LPSTR lpText, LPSTR lpTitle)
{
DLGPROC lpProc;
WORD rval;
struct {
  LPSTR lpTitle;
  LPSTR lpText;
  } lpParms;


   lpParms.lpTitle = lpTitle;
   lpParms.lpText  = lpText;

   if(!(lpProc = (DLGPROC)MakeProcInstance((FARPROC)ExtErrorDlg, hInst)))
   {
    static const char CODE_BASED pMsg[]=
                                  "?Fatal error - running out of resources!";
    static const char CODE_BASED pMsg2[]=
                                                   "** ERROR MESSAGE BOX **";

      LockCode();

      MessageBox(hWnd, pMsg, pMsg2,
                 MB_OK | MB_ICONHAND | MB_SYSTEMMODAL);

      UnlockCode();

      return(IDCANCEL);  /* an error-----arrrgghhh!! */
   }


   rval = DialogBoxParam(hInst, "EXTERROR", hWnd, lpProc,
                         (LPARAM)((LPSTR)&lpParms));

   FreeProcInstance((FARPROC)lpProc);

   return(rval);

}



BOOL CALLBACK EXPORT ExtErrorDlg(HWND hDlg, UINT message, WPARAM wParam,
                                 LPARAM lParam)
{

   switch (message)
   {
      case WM_INITDIALOG:

         SubclassStaticControls(hDlg);

         SetWindowText(hDlg, *((LPSTR FAR *)lParam));
         SetDlgItemText(hDlg, IDM_EXTERR, *(((LPSTR FAR *)lParam)+1));
         return (TRUE);


      case WM_CTLCOLOR:
         return((BOOL)DlgOn_WMCtlColor(hDlg, wParam, lParam));



      case WM_ERASEBKGND:

         {
          HDC hdcBackGround = (HDC)wParam;
          RECT rctClient;

            GetClientRect(hDlg, &rctClient);

            FillRect(hdcBackGround, &rctClient, hbrDlgBackground);
         }

         return(TRUE);


      case WM_COMMAND:

         if ((WORD)wParam == IDRETRY || (WORD)wParam == IDCANCEL)
         {
             EndDialog(hDlg, (WORD)wParam);
             return (TRUE);
         }
         break;
   }
   return (FALSE);
}




/***************************************************************************/
/*                                                                         */
/*            EditCommandDlg() - 'Edit Commands' Dialog Box                */
/*                                                                         */
/***************************************************************************/



BOOL CALLBACK EXPORT EditCommandDlg(HWND hDlg, UINT message, WPARAM wParam,
                                    LPARAM lParam)
{
LPSTR lp1;
int i;
HWND hCtrl;


   switch(message)
   {

      case WM_INITDIALOG:

         SubclassStaticControls(hDlg);

         hCtrl = GetDlgItem(hDlg, IDM_CMD_LIST);

         for(i=0, lp1=lpCmdHist; lpCmdHist && *lp1!=0;
             lp1+= lstrlen(lp1) + 1, i++)
         {
            ListBox_InsertString(hCtrl, -1, lp1);
         }

         ListBox_SetCurSel(hCtrl, i - 1);

         cmd_buf[cmd_buf_len] = 0;  /* ensure it's null terminated first.. */

         if(*cmd_buf || !lpLastCmd)
         {
            SetDlgItemText(hDlg, IDM_CMD_EDIT, cmd_buf);
         }
         else
         {
            SetDlgItemText(hDlg, IDM_CMD_EDIT, lpLastCmd);
         }

/*         SendMessage(hDlg, DM_SETDEFID, IDM_CMD_SELECTED, 0); */

/*         SetFocus(GetDlgItem(hDlg, IDM_CMD_EDIT)); */

         return(TRUE);



      case WM_CTLCOLOR:
         return((BOOL)DlgOn_WMCtlColor(hDlg, wParam, lParam));



      case WM_ERASEBKGND:

         {
          HDC hdcBackGround = (HDC)wParam;
          RECT rctClient;

            GetClientRect(hDlg, &rctClient);

            FillRect(hdcBackGround, &rctClient, hbrDlgBackground);
         }

         return(TRUE);



      case WM_COMMAND:

         switch((WORD)wParam)
         {
            case IDM_CMD_EDIT:
               switch(HIWORD((DWORD)lParam))
               {
                  case EN_SETFOCUS:

                     PostMessage(hDlg, DM_SETDEFID, IDOK, 0);
                     return(FALSE);


                  default:
                     return(FALSE);
               }


            case IDM_CMD_LIST:
               switch(HIWORD((DWORD)lParam))
               {
                  case LBN_SETFOCUS:

                     PostMessage(hDlg, DM_SETDEFID, IDM_CMD_SELECTED, 0);
                     return(FALSE);


                  case LBN_DBLCLK:
                     hCtrl = GetDlgItem(hDlg, IDM_CMD_LIST);

                     ListBox_GetText(hCtrl,ListBox_GetCurSel(hCtrl),
                                     work_buf);

                     SetDlgItemText(hDlg, IDM_CMD_EDIT, work_buf);

                     SetFocus(GetDlgItem(hDlg, IDM_CMD_EDIT));
/*                     SendMessage(hDlg, DM_SETDEFID, IDOK, 0); */

                     return(TRUE);


                  default:
                     return(FALSE);
               }

            case IDOK:          /* user pressed 'OK' button */
               GetDlgItemText(hDlg, IDM_CMD_EDIT, cmd_buf,
                              sizeof(cmd_buf));

               cmd_buf_len = lstrlen(cmd_buf);

               EndDialog(hDlg, 0);  /* this says 'it changed!' */
               return(TRUE);

            case IDCANCEL:
               EndDialog(hDlg, 1);  /* this says 'no change' */
               *work_buf = 0;
               return(TRUE);


            case IDM_CMD_ORIGINAL:
               if(*cmd_buf || !lpLastCmd)
               {
                  SetDlgItemText(hDlg, IDM_CMD_EDIT, cmd_buf);
               }
               else
               {
                  SetDlgItemText(hDlg, IDM_CMD_EDIT, lpLastCmd);
               }

               SetFocus(GetDlgItem(hDlg, IDM_CMD_EDIT));
/*               SendMessage(hDlg, DM_SETDEFID, IDOK, 0);  */

               return(TRUE);        /* like on startup, eh? */


            case IDM_CMD_SELECTED:

               hCtrl = GetDlgItem(hDlg, IDM_CMD_LIST);

               ListBox_GetText(hCtrl,ListBox_GetCurSel(hCtrl),
                               work_buf);

               SetDlgItemText(hDlg, IDM_CMD_EDIT, work_buf);

               SetFocus(GetDlgItem(hDlg, IDM_CMD_EDIT));
/*               SendMessage(hDlg, DM_SETDEFID, IDOK, 0); */

               return(TRUE);


         }
   }

   return(FALSE);       /* I didn't process a darned thing! */

}




/***************************************************************************/
/*                                                                         */
/*           SCREEN LINES DIALOG - ADJUST # OF LINES ON SCREEN             */
/*                                                                         */
/***************************************************************************/


BOOL CALLBACK EXPORT ScreenLinesDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
                                        LPARAM lParam)
{
LPDRAWITEMSTRUCT lpDI;
HDC hDC, hDC2;
RECT rct, rc2;
HPEN hPen, hOldPen;
HBITMAP hbmp, hbmpBtn, hbmpOld, hbmpOld2;
BITMAP bmp;
char tbuf[64];
COLORREF clrfg, clrbk;
long l;


   switch(msg)
   {
      case WM_INITDIALOG:

         wsprintf(tbuf, "%ld", (long)SCREEN_LINES);

         SetDlgItemText(hDlg, IDC_LINES, tbuf);
         return(1);



      case WM_CTLCOLOR:
         return((BOOL)DlgOn_WMCtlColor(hDlg, wParam, lParam));



      case WM_ERASEBKGND:

         {
          HDC hdcBackGround = (HDC)wParam;
          RECT rctClient;

            GetClientRect(hDlg, &rctClient);

            FillRect(hdcBackGround, &rctClient, hbrDlgBackground);
         }

         return(TRUE);



      case WM_COMMAND:

         if((WORD)wParam == IDOK)
         {
          LPSTR lp1;


            // change screen line count here

            GetDlgItemText(hDlg, IDC_LINES, tbuf, sizeof(tbuf));

            l = (long)CalcEvalNumber(tbuf);

            if(l < 10 || l > 400)
            {
               MessageBox(hDlg,
                          "You must specify a line count between 10 and 400",
                          "* SFTShell Line Count Error *",
                          MB_OK | MB_ICONHAND);

               if(l < 10) l = 10;
               if(l > 400) l = 400;

               wsprintf(tbuf, "%ld", l);

               SetDlgItemText(hDlg, IDC_LINES, tbuf);
               return(1);
            }

            // attempt to assign the number of screen lines indicated by 'l'

            lp1 = GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                 (sizeof(screen[0][0]) + sizeof(attrib[0][0]))
                                  * SCREEN_COLS * l);

            if(!lp1)
            {
               MessageBox(hDlg,
                          "Not enough memory - screen buffer size not changed",
                          "* SFTShell Line Count Error *",
                          MB_OK | MB_ICONHAND);

               EndDialog(hDlg, 0);
            }
            else
            {
               SCREEN_LINES = l;
               GlobalFreePtr(lpScreen);
               lpScreen = lp1;

               screen = (char (__based(lpScreen) *)[80])lpScreen;
               attrib = (char (__based(lpScreen) *)[80])
                          (lpScreen + sizeof(screen[0][0])
                          * SCREEN_COLS * SCREEN_LINES);

               ClearScreen();  // this will take care of all of the
                               // potential problems all at once

               if(!lpBatchInfo && !BatchMode && !lpCurCmd
                  && !GettingUserInput)
               {
                  cmd_buf[0] = 0;
                  cmd_buf_len = 0;           /* zero command buf */

                  DisplayPrompt();  // ensure the prompt is displayed
               }

               wsprintf(tbuf, "%ld", l);
               WriteProfileString(pPROGRAMNAME,pSCREENLINES,tbuf);

               EndDialog(hDlg, 1);
            }

            return(1);
         }
         else if((WORD)wParam == IDCANCEL)
         {
            EndDialog(hDlg, 0);
            return(1);
         }
         else if((WORD)wParam == IDC_SPIN_UP)
         {
            GetDlgItemText(hDlg, IDC_LINES, tbuf, sizeof(tbuf));

            l = (long)CalcEvalNumber(tbuf) + 1;

            if(l > 400)
            {
               MessageBeep(0);

               l = 400;
            }

            wsprintf(tbuf, "%ld", l);

            SetDlgItemText(hDlg, IDC_LINES, tbuf);
         }
         else if((WORD)wParam == IDC_SPIN_DOWN)
         {
            GetDlgItemText(hDlg, IDC_LINES, tbuf, sizeof(tbuf));

            l = (long)CalcEvalNumber(tbuf) - 1;

            if(l < 10)
            {
               MessageBeep(0);

               l = 10;
            }

            wsprintf(tbuf, "%ld", l);

            SetDlgItemText(hDlg, IDC_LINES, tbuf);
         }

         break;



      case WM_DRAWITEM:

         lpDI = (LPDRAWITEMSTRUCT)lParam;
         GetDlgItemText(hDlg, lpDI->CtlID, tbuf, sizeof(tbuf));

         hbmpBtn = LoadBitmap(hInst, tbuf);
         if(!hbmpBtn) break;  // don't do anything else if no bitmap!

         GetObject(hbmpBtn, sizeof(BITMAP), (LPSTR)&bmp);


         // set up compatible DC with appropriately sized bitmap

         hDC = CreateCompatibleDC(lpDI->hDC);
         hDC2 = CreateCompatibleDC(lpDI->hDC);

         GetClientRect(GetDlgItem(hDlg, lpDI->CtlID), &rct);
         rc2 = rct;

         rc2.right  -= rc2.left;
         rc2.bottom -= rc2.top;
         rc2.left   = 0;
         rc2.top    = 0;

         hbmp = CreateCompatibleBitmap(lpDI->hDC, rc2.right, rc2.bottom);
         hbmpOld = SelectObject(hDC, hbmp);
         hbmpOld2 = SelectObject(hDC2, hbmpBtn);


         // draw black border around button

         hPen = (HPEN)GetStockObject(BLACK_PEN);
         hOldPen = SelectObject(hDC, hOldPen);
         Rectangle(hDC, rc2.left, rc2.top, rc2.right, rc2.bottom);

         rc2.left++;
         rc2.right--;
         rc2.top++;
         rc2.bottom--;

         // if button is pressed IN, paint dark shadow up left and shift
         // bitmap 2 right and 2 down; else, paint light shadow up left
         // and dark shadow bottom right and shift bitmap 1 pixel 'in' on
         // all sides.

         if(lpDI->itemState & ODS_SELECTED)  // button "pressed"
         {
            hPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNSHADOW));

            SelectObject(hDC, hPen);

            MoveTo(hDC, rc2.left, rc2.bottom - 1);
            LineTo(hDC, rc2.left, rc2.top);
            LineTo(hDC, rc2.right, rc2.top);
            MoveTo(hDC, rc2.left + 1, rc2.bottom - 1);
            LineTo(hDC, rc2.left + 1, rc2.top + 1);
            LineTo(hDC, rc2.right, rc2.top + 1);

            SelectObject(hDC, hOldPen);
            DeleteObject(hPen);
            hPen = NULL;

            rc2.left += 2;
            rc2.top  += 2;
         }
         else
         {
            hPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNHIGHLIGHT));

            SelectObject(hDC, hPen);

            MoveTo(hDC, rc2.left, rc2.bottom - 1);
            LineTo(hDC, rc2.left, rc2.top);
            LineTo(hDC, rc2.right, rc2.top);

            SelectObject(hDC, hOldPen);
            DeleteObject(hPen);

            hPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNSHADOW));

            SelectObject(hDC, hPen);

            MoveTo(hDC, rc2.left + 1, rc2.bottom - 1);
            LineTo(hDC, rc2.right - 1, rc2.bottom - 1);
            LineTo(hDC, rc2.right - 1, rc2.top);


            SelectObject(hDC, hOldPen);
            DeleteObject(hPen);

            hPen = NULL;

            rc2.left++;
            rc2.right--;
            rc2.top++;
            rc2.bottom--;
         }

         // last step: draw bitmap

         clrfg=SetTextColor(hDC, RGB(0,0,0));
         clrbk=SetBkColor(hDC, GetSysColor(COLOR_BTNFACE));

         StretchBlt(hDC, rc2.left, rc2.top,
                    rc2.right - rc2.left, rc2.bottom - rc2.top,
                    hDC2, 0, 0, bmp.bmWidth, bmp.bmHeight,
                    SRCCOPY);

         SetTextColor(hDC, clrfg);
         SetBkColor(hDC, clrbk);


         // final step:  make bitmap in 'hDC' go onto the button face,
         //              then clean up resources.

         BitBlt(lpDI->hDC, rct.left, rct.top,
                rct.right - rct.left, rct.bottom - rct.top,
                hDC, 0, 0, SRCCOPY);


         SelectObject(hDC2, hbmpOld2);
         DeleteObject(hbmpBtn);
         DeleteDC(hDC2);

         SelectObject(hDC, hbmpOld);
         DeleteObject(hbmp);
         DeleteDC(hDC);

         return(1);

   }

   return(0);

}


/***************************************************************************/
/*                                                                         */
/*        'MyGetOpenFileName()' dialog box - uses 'COMMDLG.DLL'!           */
/*                                                                         */
/***************************************************************************/

BOOL FAR PASCAL MyGetOpenFileName(LPCSTR lpTitle, LPSTR lpBuf, BOOL bExec)
{
BOOL rval;
OPENFILENAME ofn;
static const char CODE_BASED pFilter[]=
   "Executable Files\0*.pif;*.bat;*.exe;*.com;*.rex\0"
   "All Files\0*.*\0";


   if(!lpGetOpenFileName)
   {
      MessageBeep(MB_ICONHAND);

      MessageBox(hMainWnd, "?Unable to create 'OPEN FILE' dialog",
                 "** COMMDLG ERROR **", MB_OK | MB_ICONHAND);

      return(TRUE);     // return TRUE on error!
   }


   *lpBuf = 0;

   _fmemset((LPSTR)&ofn, 0, sizeof(OPENFILENAME));
   ofn.lStructSize = sizeof(OPENFILENAME);

   ofn.hInstance    = hInst;

   ofn.lpstrFilter  = pFilter;
   ofn.nFilterIndex = bExec!=0 ? 1 : 2;
   ofn.Flags        = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST |
                      OFN_HIDEREADONLY | OFN_SHAREAWARE;

   ofn.lpstrFile       = lpBuf;
   ofn.lpstrInitialDir = "";
   ofn.lpstrDefExt     = "";

   ofn.nMaxFile     = 66;
   ofn.lpstrTitle   = lpTitle;

   rval = !lpGetOpenFileName(&ofn);

   if(rval) *lpBuf = 0;

   return(rval);
}


#pragma code_seg()



/***************************************************************************/
/*                                                                         */
/*             'FOLDER' Dialog Processing (functions, etc.)                */
/*                                                                         */
/***************************************************************************/


#pragma code_seg("FOLDERDLG_TEXT","CODE")

void FAR PASCAL FolderDlgSetDirList(HWND hDlg, LPCSTR lpPath);
void FAR PASCAL FolderDlgSetDriveList(HWND hDlg, LPCSTR lpPath);
void FAR PASCAL FolderDlgDirDrawItem(HWND hDlg, LPDRAWITEMSTRUCT lpDrawItemStruct);
void FAR PASCAL FolderDlgDriveDrawItem(HWND hDlg, LPDRAWITEMSTRUCT lpDrawItemStruct);
HBITMAP FAR PASCAL GetDriveBitmap(int iDrive);


static BOOL FolderDisableNotify = FALSE;
static HBITMAP hbmpDriveCD=NULL, hbmpDriveFL3=NULL, hbmpDriveFL5=NULL,
               hbmpDriveHD=NULL, hbmpDriveNET=NULL, hbmpDriveRAM=NULL,
               hbmpDriveSUB=NULL, hbmpDriveNONE=NULL;
static HBITMAP hbmpOpenFolder=NULL, hbmpGrayFolder=NULL, hbmpShutFolder=NULL;
static TEXTMETRIC FolderTextMetric;

static WORD wFolderDlgDirListIndex=0xffff;


BOOL CALLBACK EXPORT FolderDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
                                   LPARAM lParam)
{
UINT u1;
char tbuf[MAX_PATH * 2];
LPSTR lp1;
RECT rct;
static LPSTR lpDest = NULL;
static BOOL bEditChangeFlag=FALSE;



   switch(msg)
   {
      case WM_INITDIALOG:

         SubclassStaticControls(hDlg);

         lpDest = (LPSTR)lParam;             // parm passed on create

         _lgetdcwd(0, lpDest, MAX_PATH);     // get current working dir

         // let's get some bitmaps, shall we??

         hbmpDriveCD    = LoadBitmap(hInst, "BitmapDriveCD");
         hbmpDriveFL3   = LoadBitmap(hInst, "BitmapDriveFL3");
         hbmpDriveFL5   = LoadBitmap(hInst, "BitmapDriveFL5");
         hbmpDriveHD    = LoadBitmap(hInst, "BitmapDriveHD");
         hbmpDriveNET   = LoadBitmap(hInst, "BitmapDriveNET");
         hbmpDriveRAM   = LoadBitmap(hInst, "BitmapDriveRAM");
         hbmpDriveSUB   = LoadBitmap(hInst, "BitmapDriveSUB");
         hbmpDriveNONE  = LoadBitmap(hInst, "BitmapDriveNONE");

         hbmpOpenFolder = LoadBitmap(hInst, "BitmapOpenFolder");
         hbmpGrayFolder = LoadBitmap(hInst, "BitmapGrayFolder");
         hbmpShutFolder = LoadBitmap(hInst, "BitmapShutFolder");


         FolderDisableNotify = TRUE;
         SetDlgItemText(hDlg, IDC_PATH, lpDest);
         FolderDisableNotify = FALSE;

         FolderDlgSetDirList(hDlg, lpDest);
         FolderDlgSetDriveList(hDlg, lpDest);

         return(TRUE);




      case WM_DESTROY:

         // free up static resources

         DeleteObject(hbmpDriveCD);
         DeleteObject(hbmpDriveFL3);
         DeleteObject(hbmpDriveFL5);
         DeleteObject(hbmpDriveHD);
         DeleteObject(hbmpDriveNET);
         DeleteObject(hbmpDriveRAM);
         DeleteObject(hbmpDriveNONE);

         hbmpDriveCD=NULL;
         hbmpDriveFL3=NULL;
         hbmpDriveFL5=NULL;
         hbmpDriveHD=NULL;
         hbmpDriveNET=NULL;
         hbmpDriveRAM=NULL;
         hbmpDriveNONE=NULL;

         DeleteObject(hbmpOpenFolder);
         DeleteObject(hbmpGrayFolder);
         DeleteObject(hbmpShutFolder);

         hbmpOpenFolder=NULL;
         hbmpGrayFolder=NULL;
         hbmpShutFolder=NULL;

         break;




      case WM_CTLCOLOR:
         return((BOOL)DlgOn_WMCtlColor(hDlg, wParam, lParam));



      case WM_ERASEBKGND:

         {
          HDC hdcBackGround = (HDC)wParam;
          RECT rctClient;

            GetClientRect(hDlg, &rctClient);

            FillRect(hdcBackGround, &rctClient, hbrDlgBackground);
         }

         return(TRUE);




      case WM_COMMAND:

         if(LOWORD(wParam)==IDOK)
         {
            if(bEditChangeFlag)
            {
               bEditChangeFlag = FALSE;

               GetDlgItemText(hDlg, IDC_PATH, tbuf, sizeof(tbuf));
               if(!*tbuf)
               {
                  MessageBeep(0);
                  SetDlgItemText(hDlg, IDC_PATH, lpDest);

                  SetFocus(GetDlgItem(hDlg, IDC_PATH));

                  return(TRUE);
               }

               // see if path in box is valid

               DosQualifyPath(tbuf, tbuf);

               if(tbuf[lstrlen(tbuf)-1]!='\\') lstrcat(tbuf, "\\");

               if(!FileExists(tbuf))
               {
                  // path does not exist, huh?  Well, let's make sure
                  // the user knows about this one

                  tbuf[lstrlen(tbuf)-1] = 0;
                  if(!*tbuf || tbuf[lstrlen(tbuf)-1]==':') lstrcat(tbuf, "\\");


                  MessageBox(hDlg, "?Specified Drive or Folder does not exist",
                             "* SFTShell Change Drive/Folder",
                             MB_OK | MB_ICONHAND);

                  SetFocus(GetDlgItem(hDlg, IDC_PATH));

                  return(TRUE);
               }

               // path is valid - assign new path to 'lpDest'

               tbuf[lstrlen(tbuf)-1] = 0;
               if(!*tbuf || tbuf[lstrlen(tbuf)-1]==':') lstrcat(tbuf, "\\");

               lstrcpy(lpDest, tbuf);


               // re-assign the text box, just because...

               FolderDisableNotify = TRUE;
               SetDlgItemText(hDlg, IDC_PATH, lpDest);
               FolderDisableNotify = FALSE;

               FolderDlgSetDirList(hDlg, lpDest);
               FolderDlgSetDriveList(hDlg, lpDest);

               SetFocus(GetDlgItem(hDlg, IDC_PATH));

               return(TRUE);
            }

            EndDialog(hDlg, 0);

            return(TRUE);
         }
         else if(LOWORD(wParam)==IDCANCEL)
         {
            *lpDest = 0;

            EndDialog(hDlg, 1);

            return(TRUE);
         }
         else if(LOWORD(wParam)==IDC_PATH)
         {
            if(FolderDisableNotify) break;

            if(HIWORD(lParam)==EN_CHANGE)
            {
               bEditChangeFlag = TRUE;
            }
         }
         else if(LOWORD(wParam)==IDC_DIRLIST)
         {
            if(FolderDisableNotify) break;

            if(HIWORD(lParam)==LBN_DBLCLK ||
               HIWORD(lParam)==LBN_SELCHANGE)
            {
               // which drive/path am I now?

               u1 = (UINT)SendDlgItemMessage(hDlg, IDC_DIRLIST,
                                             LB_GETCURSEL, 0, 0);

               if(u1 == LB_ERR)
               {
                  MessageBeep(0);
                  FolderDlgSetDirList(hDlg, lpDest);
               }
               else
               {
                  *tbuf = 0;

                  SendDlgItemMessage(hDlg, IDC_DIRLIST, LB_GETTEXT,
                                     (WPARAM)u1, (LPARAM)lpDest);

                  FolderDisableNotify = TRUE;
                  SetDlgItemText(hDlg, IDC_PATH, lpDest);
                  FolderDisableNotify = FALSE;

                  FolderDlgSetDriveList(hDlg, lpDest);

                  if(HIWORD(lParam)==LBN_DBLCLK)
                  {
                     PostMessage(hDlg, WM_COMMAND, (WPARAM)IDOK,
                                 MAKELPARAM(GetDlgItem(hDlg, IDOK),
                                            BN_CLICKED));
                  }
                  else
                  {
                     FolderDlgSetDirList(hDlg, lpDest);  // must set self
                  }
               }

               return(TRUE);
            }
         }
         else if(LOWORD(wParam)==IDC_DRIVELIST)
         {
            if(FolderDisableNotify) break;

            if(HIWORD(lParam)==CBN_SELCHANGE ||
               HIWORD(lParam)==CBN_CLOSEUP)
            {
               // if list is NOT dropped, count the change as 'good'

               if(!SendDlgItemMessage(hDlg, IDC_DRIVELIST,
                                      CB_GETDROPPEDSTATE, 0, 0))
               {
                  // which drive/path am I now?

                  u1 = (UINT)SendDlgItemMessage(hDlg, IDC_DRIVELIST,
                                                CB_GETCURSEL, 0, 0);

                  if(u1 == LB_ERR)
                  {
                     MessageBeep(0);
                     FolderDlgSetDirList(hDlg, lpDest);
                  }
                  else
                  {
                   BOOL bChanged = TRUE;  // by default, it changed

                     *tbuf = 0;

                     SendDlgItemMessage(hDlg, IDC_DRIVELIST, CB_GETLBTEXT,
                                        (WPARAM)u1, (LPARAM)(LPSTR)tbuf);

                     lp1 = _fstrchr(tbuf, '\t');
                     if(lp1)
                     {
                        bChanged = toupper(lp1[1]) != toupper(*lpDest);

                        lstrcpy(lpDest, lp1 + 1);
                     }
                     else
                     {
                        lstrcpy(lpDest, tbuf);  // just in case...
                     }

                     if(bChanged) // a REAL change!
                     {
                        FolderDisableNotify = TRUE;
                        SetDlgItemText(hDlg, IDC_PATH, lpDest);
                        FolderDisableNotify = FALSE;

                        FolderDlgSetDirList(hDlg, lpDest);
                     }
                  }
               }

               return(TRUE);
            }
         }

         break;



      case WM_MEASUREITEM:

         // which control?

//         switch(LOWORD(wParam))
         switch(((LPMEASUREITEMSTRUCT)lParam)->CtlID)
         {
            case IDC_DIRLIST:

               {
                HFONT hOldFont, hCurFont = (HFONT)
                     SendDlgItemMessage(hDlg, IDC_DIRLIST, WM_GETFONT, 0, 0);
                HDC hDC = GetDC(GetDlgItem(hDlg, IDC_DIRLIST));


                  if(hCurFont) hOldFont = SelectObject(hDC, hCurFont);
                  else         hOldFont = NULL;

                  GetTextMetrics(hDC, &FolderTextMetric);

                  if(hOldFont) SelectObject(hDC, hOldFont);

                  ReleaseDC(GetDlgItem(hDlg, IDC_DIRLIST), hDC);
               }


               GetWindowRect(GetDlgItem(hDlg, IDC_DIRLIST), &rct);

               ((LPMEASUREITEMSTRUCT)lParam)->itemWidth = rct.right - rct.left;
               ((LPMEASUREITEMSTRUCT)lParam)->itemHeight = FolderTextMetric.tmHeight;

               return(TRUE);



            case IDC_DRIVELIST:

               {
                HFONT hOldFont, hCurFont = (HFONT)
                     SendDlgItemMessage(hDlg, IDC_DRIVELIST, WM_GETFONT, 0, 0);
                HDC hDC = GetDC(GetDlgItem(hDlg, IDC_DRIVELIST));


                  if(hCurFont) hOldFont = SelectObject(hDC, hCurFont);
                  else         hOldFont = NULL;

                  GetTextMetrics(hDC, &FolderTextMetric);

                  if(hOldFont) SelectObject(hDC, hOldFont);

                  ReleaseDC(GetDlgItem(hDlg, IDC_DRIVELIST), hDC);
               }

               GetWindowRect(GetDlgItem(hDlg, IDC_DRIVELIST), &rct);

               ((LPMEASUREITEMSTRUCT)lParam)->itemWidth = rct.right - rct.left;
               ((LPMEASUREITEMSTRUCT)lParam)->itemHeight = FolderTextMetric.tmHeight;

               return(TRUE);

         }

         break;


      case WM_COMPAREITEM:

         // which control?

//         switch(LOWORD(wParam))
         switch(((LPCOMPAREITEMSTRUCT)lParam)->CtlID)
         {
            case IDC_DIRLIST:
               return(
                _fstricmp((LPSTR)(((LPCOMPAREITEMSTRUCT)lParam)->itemData1),
                          (LPSTR)(((LPCOMPAREITEMSTRUCT)lParam)->itemData2)));


            case IDC_DRIVELIST:
               return(
                _fstricmp((LPSTR)(((LPCOMPAREITEMSTRUCT)lParam)->itemData1),
                          (LPSTR)(((LPCOMPAREITEMSTRUCT)lParam)->itemData2)));

         }

         break;


      case WM_DRAWITEM:

         // which control?

         switch(((LPDRAWITEMSTRUCT)lParam)->CtlID)
         {
            case IDC_DIRLIST:
               FolderDlgDirDrawItem(hDlg, (LPDRAWITEMSTRUCT)lParam);
               return(TRUE);


            case IDC_DRIVELIST:
               FolderDlgDriveDrawItem(hDlg, (LPDRAWITEMSTRUCT)lParam);
               return(TRUE);

         }

         break;
   }


   return(FALSE);
}


void FAR PASCAL FolderDlgSetDriveList(HWND hDlg, LPCSTR lpPath)
{
WORD w1, w2, wSel, wItem;
char path[MAX_PATH * 2], drive[MAX_PATH];
//LPSTR lp1;
LPCSTR lpc1;
HCURSOR hOldCursor;



   FolderDisableNotify = TRUE;

   hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

   // find out which drive 'lpPath' is using... (if any)

   for(lpc1=lpPath; *lpc1 && *lpc1 != ':'; lpc1++)
      ;

   if(*lpc1)
   {
      lpc1++;

      _hmemcpy(drive, lpPath, lpc1 - lpPath);
      drive[lpc1 - lpPath] = 0;

      _fstrupr(drive);  // ensure it's upper case
   }
   else
   {
      drive[0] = 0;
   }


   w2 = (WORD)SendDlgItemMessage(hDlg, IDC_DRIVELIST, CB_GETCOUNT, 0, 0);

   if(!w2)
   {
      SendDlgItemMessage(hDlg, IDC_DRIVELIST, WM_SETREDRAW, (WPARAM)0, 0);

      // Generate a list of *ALL* valid drives on the system!!!  YAY!
      // FORMAT OF TEXT:  d: drive_label  (lower case to be consistent!)

      for(wSel = 0xffff, wItem = 0, w1=0; w1<26; w1++)
      {
#ifdef WIN32
         {
            char drive_tbuf[4];
            drive_tbuf[0] = 'A' + (char)w1;
            drive_tbuf[1] = ':';
            drive_tbuf[2] = '\\';
            drive_tbuf[3] = 0;
            w2 = GetDriveType(drive_tbuf);
         }
#else  // WIN32
            w2 = GetDriveType(w1);
#endif // WIN32

         if(w2 == 0)  // 'UNKNOWN' type...  is this a SUBST'ed drive?
         {
          LPCURDIR lpC = (LPCURDIR)GetDosCurDirEntry(w1 + 1);  // check for SUBST drive

            if(lpC)
            {
               // if SUBST'ed drive, get info on the drive it points to

               if((lpC->wFlags & CURDIR_TYPE_MASK)!=CURDIR_TYPE_INVALID &&
                  (lpC->wFlags & CURDIR_SUBST))
               {
                  if(toupper(lpC->pathname[0]) <'A' ||
                     toupper(lpC->pathname[0]) >'Z')
                  {
                     w2 = DRIVE_REMOTE;  // assume network drive
                  }
                  else
                  {
#ifdef WIN32
                     w2 = GetDriveType(lpC->pathname);
#else  // WIN32
                     w2 = GetDriveType(toupper(lpC->pathname[0]) - 'A');
#endif // WIN32
                  }
               }
#ifdef WIN32
               GlobalFreePtr(lpC);
#else  // WIN32
               MyFreeSelector(SELECTOROF(lpC));
#endif // WIN32
            }
         }

         if(w2 > 1)
         {
            _hmemset(path, 0, sizeof(path));

            *path = w1 + 'A';
            path[1] = ':';
            path[2] = ' ';
            path[3] = 0;

            if(w2 != DRIVE_REMOVABLE)
            {
               GetDriveLabelOnly(path + 3, path);
            }

            lstrcat(path, "\t");       // a TAB character separates args

            if(*path == *drive)
            {
               lstrcat(path, lpPath);     // and is followed by the drive letter

               wSel = wItem;
            }
            else
            {
               _lgetdcwd(w1 + 1, path + lstrlen(path), MAX_PATH);
            }

            SendDlgItemMessage(hDlg, IDC_DRIVELIST, CB_INSERTSTRING,
                               (WPARAM)wItem, (LPARAM)(LPSTR)path);

            SendDlgItemMessage(hDlg, IDC_DRIVELIST, CB_SETITEMDATA,
                               (WPARAM)wItem,
                               (LPARAM)(WORD)GetDriveBitmap(w1 + 1));

            wItem++;
         }
      }

      if(wSel != 0xffff)
      {
         SendDlgItemMessage(hDlg, IDC_DRIVELIST, CB_SETCURSEL,
                            (WPARAM)wSel, 0);
      }

      SendDlgItemMessage(hDlg, IDC_DRIVELIST, WM_SETREDRAW, (WPARAM)1, 0);
      InvalidateRect(GetDlgItem(hDlg, IDC_DRIVELIST), NULL, TRUE);
   }
   else
   {
      // see which drive the current path applies to (if any)

      for(w1=0; w1<w2; w1++)
      {
         if(SendDlgItemMessage(hDlg, IDC_DRIVELIST, CB_GETLBTEXT, (WPARAM)w1,
                               (LPARAM)(LPSTR)path) == LB_ERR) continue;

         if(toupper(*path) == *drive)
         {
            // drive matches - change designated path string

            SendDlgItemMessage(hDlg, IDC_DRIVELIST, WM_SETREDRAW,
                               (WPARAM)0, 0);

            SendDlgItemMessage(hDlg, IDC_DRIVELIST, CB_DELETESTRING,
                               (WPARAM)w1, 0);

//            lp1 = _fstrchr(path, '\t');  // find the tab...
//            if(lp1)
//            {
//               lp1++;
//               lstrcpy(lp1, lpPath);
//            }
//            else
            {
               w2 = toupper(*path) - 'A';

               _hmemset(path + 1, 0, sizeof(path) - 1);

               path[1] = ':';
               path[3] = 0;
#ifdef WIN32
               path[2] = '\\';
               if(GetDriveType(path) != DRIVE_REMOVABLE)
#else  // WIN32
               if(GetDriveType(w2) != DRIVE_REMOVABLE)
#endif // WIN32
               {
                  GetDriveLabelOnly(path + 3, path);
               }

               path[2] = ' ';

               lstrcat(path, "\t");       // a TAB character separates args

               lstrcat(path, lpPath);     // and is followed by the drive letter
            }

            SendDlgItemMessage(hDlg, IDC_DRIVELIST, CB_INSERTSTRING,
                               (WPARAM)w1, (LPARAM)(LPSTR)path);

            SendDlgItemMessage(hDlg, IDC_DRIVELIST, CB_SETITEMDATA,
                               (WPARAM)w1,
                               (LPARAM)(WORD)GetDriveBitmap(w2 + 1));

            SendDlgItemMessage(hDlg, IDC_DRIVELIST, CB_SETCURSEL,
                               (WPARAM)w1, 0);

            SendDlgItemMessage(hDlg, IDC_DRIVELIST, WM_SETREDRAW,
                               (WPARAM)1, 0);


            InvalidateRect(GetDlgItem(hDlg, IDC_DRIVELIST), NULL, TRUE);
         }
      }
   }


   if(hOldCursor) SetCursor(hOldCursor);

   FolderDisableNotify = FALSE;
}



void FAR PASCAL FolderDlgSetDirList(HWND hDlg, LPCSTR lpPath)
{
WORD w1;
char path[MAX_PATH], dir[MAX_PATH], drive[MAX_PATH];
LPSTR lp1, lp2;
LPCSTR lpc1;
HCURSOR hOldCursor;
struct _find_t ff;



   FolderDisableNotify = TRUE;

   hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

   // find out which drive 'lpPath' is using... (if any)

   for(lpc1=lpPath; *lpc1 && *lpc1 != ':'; lpc1++)
      ;

   if(*lpc1)
   {
      lpc1++;

      _hmemcpy(drive, lpPath, lpc1 - lpPath);
      drive[lpc1 - lpPath] = 0;

      _fstrupr(drive);
   }
   else
   {
      drive[0] = 0;
   }

   lstrcpy(path, lpPath);

   if(!*path || path[lstrlen(path) - 1]!='\\') lstrcat(path, "\\");


   // for the sake of appearance, don't redraw the 'resetcontent' yet

   SendDlgItemMessage(hDlg, IDC_DIRLIST, WM_SETREDRAW, (WPARAM)0, 0);

   SendDlgItemMessage(hDlg, IDC_DIRLIST, LB_RESETCONTENT, 0, 0);



   // 1st step:  Add 1 item for EACH sub-directory below the current one!

   lp1 = path;

   w1 = 0;       // current item in directory...

   while(lp2 = _fstrchr(lp1, '\\'))
   {
      if(lp1 == path)   // this is the ROOT directory, I HOPE!
      {
         _hmemcpy(dir, path, lp2 - (LPSTR)path + 1);
         dir[lp2 - (LPSTR)path + 1] = 0;
      }
      else
      {
         _hmemcpy(dir, path, lp2 - (LPSTR)path);
         dir[lp2 - (LPSTR)path] = 0;
      }

      lp1 = lp2 + 1;

      SendDlgItemMessage(hDlg, IDC_DIRLIST, LB_INSERTSTRING,
                         (WPARAM)w1, (LPARAM)(LPSTR)dir);

      w1++;
   }

   if(w1 > 0)
   {
      wFolderDlgDirListIndex = w1 - 1;

      SendDlgItemMessage(hDlg, IDC_DIRLIST, LB_SETCURSEL,
                         (WPARAM)(w1 - 1), 0);
                      // this sets the current path to what it ought to be
   }
   else
   {
      wFolderDlgDirListIndex = 0xffff;  // no current selection...
   }

   // NOW, fill up the list with sub-directories belonging to THIS path

   lstrcpy(dir, path);                  // make copy of PATH ONLY right here!
   lp1 = dir + lstrlen(dir);            // point 'lp1' to the end
   lstrcat(path, "*.*");                // create search path by adding '*.*'


   // Using the pattern in 'path' generate a list of files (not 'DIR's!)
   // from the current directory, and place them in the list box!  NOTE
   // especially that if I use 'AddString' that the entries will remain
   // in the correct order, since ALL of them begin with the SAME
   // string up to the CURRENT PATH, so they'll alphabetise CORRECTLY!


   if(!MyFindFirst(path, _A_ARCH | _A_RDONLY | _A_SYSTEM | _A_SUBDIR, &ff))
   {
      do
      {
         if(ff.attrib & _A_SUBDIR) // it's a SUB-DIRECTORY!!
         {
            if(ff.name[0]!='.' ||
               (ff.name[1]!=0 && (ff.name[1]!='.' || ff.name[2]!=0)))
            {
               // DO NOT include '.' or '..' in this list!!

               _hmemcpy(lp1, ff.name, sizeof(ff.name));
               lp1[sizeof(ff.name)] = 0;

               SendDlgItemMessage(hDlg, IDC_DIRLIST, LB_ADDSTRING, 0,
                                  (LPARAM)(LPSTR)dir);  // inserts complete path
            }
         }

      } while(!MyFindNext(&ff));

      MyFindClose(&ff);
   }


   SendDlgItemMessage(hDlg, IDC_DIRLIST, WM_SETREDRAW, (WPARAM)1, 0);
   InvalidateRect(GetDlgItem(hDlg, IDC_DIRLIST), NULL, TRUE);

   if(hOldCursor) SetCursor(hOldCursor);

   FolderDisableNotify = FALSE;

}



HBITMAP FAR PASCAL GetDriveBitmap(int iDrive)
{
WORD bIsCD, wDriveType, wFlags;
LPSTR lp1;
LPCURDIR lpC;
static WORD bHasCD = 0, bHasCDChecked = 0;



   if(!iDrive) return(NULL); // zero is 'Default', drive 'A' is 1, etc.


   iDrive--;              // convert so that drive 'A' is zero

#ifdef WIN32
   {
      char drive_tbuf[4];
      drive_tbuf[0] = 'A' + (char)iDrive;
      drive_tbuf[1] = ':';
      drive_tbuf[2] = '\\';
      drive_tbuf[3] = 0;
      wDriveType = GetDriveType(drive_tbuf);
   }

   bIsCD = (wDriveType == DRIVE_CDROM);

#else  // WIN32

   if(!bHasCDChecked)     // CHECK if CD ROM EXTENSIONS are installed!
   {
      _asm mov ax, 0x150c
      _asm mov bx, 0      // this is necessary to verify check below...
      _asm int 0x2f

      _asm mov bHasCD, ax
      _asm mov bHasCDChecked, bx

      if(bHasCD == 0x150c && bHasCDChecked < 0x200)
      {
         bHasCDChecked = TRUE;
         bHasCD = FALSE;
      }
      else
      {
         bHasCD = TRUE;
      }
   }

   bIsCD = FALSE;
   wDriveType = GetDriveType(iDrive);   // drive 'A' is 0, 'B' is 1, etc.

   if(bHasCD)
   {
      _asm mov cx, iDrive;

      _asm mov ax, 0x150b
      _asm int 0x2f

      _asm mov bIsCD, ax    // 'ax' is non-zero if drive is a CD ROM

      if(bIsCD == 0x150b) bIsCD = 0;   // error trapper!
   }

#endif // WIN32

   lpC = (LPCURDIR)GetDosCurDirEntry(iDrive + 1);

   if(lpC)
   {
      wFlags = lpC->wFlags;
#ifdef WIN32
      GlobalFreePtr(lpC);
#else // WIN32
      MyFreeSelector(SELECTOROF(lpC));
#endif // WIN32
      lpC = NULL;


      if((wFlags & CURDIR_TYPE_MASK)==CURDIR_TYPE_INVALID)
      {
         // in case I want to do something here...
      }
      else if(wFlags & CURDIR_SUBST)
      {
         return(hbmpDriveSUB);
      }
   }

   if(bIsCD)
   {
      return(hbmpDriveCD);
   }
   else if(wDriveType == DRIVE_REMOVABLE)
   {
      // is this a 3.5" or 5.25" diskette drive??

      GetDriveParms(GetPhysDriveFromLogical(iDrive + 1), &wDriveType,
                    NULL, NULL, NULL, NULL);

      if(wDriveType == 1 || wDriveType == 2)
      {
         return(hbmpDriveFL5);   // 5.25" drive
      }
      else
      {
         return(hbmpDriveFL3);   // 3.5" drive
      }
   }
   #ifdef WIN32
   else if(wDriveType == DRIVE_RAMDISK)
   {
      return(hbmpDriveRAM);
   }
   #endif // WIN32
   else if(wDriveType == DRIVE_FIXED)
   {
   #ifndef WIN32
    struct _diskfree_t dft;
    WORD wParmBlock[5];
    BOOL bErrFlag = 0;

      // RAM drives show up as 'FIXED' drives, so read the boot sector and
      // look for 'RDV x.xx' for the 'OEM DOS' name

      if(MyGetDiskFreeSpace(iDrive + 1, &dft)) return(hbmpDriveHD);

      lp1 = GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                           max(512, dft.bytes_per_sector));

      if(!lp1) return(hbmpDriveHD);

      // read the boot sector and look for OEM name 'RDV x.xx'

      wParmBlock[0] = 0;    // low word of 32-bit sector
      wParmBlock[1] = 0;    // high word of 32-bit sector
      wParmBlock[2] = 1;    // # of sectors to read
      wParmBlock[3] = OFFSETOF(lp1);
      wParmBlock[4] = SELECTOROF(lp1);

      if(HIWORD(GetVersion) >= 0x400) _asm
      {
         push es
         push ds
         pusha
         push bp

         mov ax, iDrive
         mov cx, -1

         push ss
         pop ds
         lea bx, wParmBlock

         int 0x25     // read boot sector
         add sp,2     // clear the flags off of the stack

         pop bp

         mov bErrFlag, ax

         popa
         pop ds
         pop es

         jc was_error

         mov bErrFlag, 0
was_error:
      }
      else _asm
      {
         push es
         push ds
         pusha
         push bp

         mov ax, iDrive
         mov cx, 1
         mov dx, 0

         lds bx, lp1

         int 0x25     // read boot sector
         add sp,2     // clear the flags off of the stack

         pop bp

         mov bErrFlag, ax

         popa
         pop ds
         pop es

         jc was_error0

         mov bErrFlag, 0
was_error0:
      }


      if(!bErrFlag)
      {
         if(lp1[3]=='R' && lp1[4]=='D' && lp1[5] == 'V' &&
            lp1[6]==' ' && lp1[7]>='0' && lp1[7]<='9')
         {
            // close enough - it's a RAM DRIVE!

            return(hbmpDriveRAM);
         }
      }

   #endif // WIN32

      return(hbmpDriveHD);
   }
   else if(wDriveType == DRIVE_REMOTE)
   {
      return(hbmpDriveNET);
   }
   else // BY DEFAULT, PICK SOMETHING!
   {
      return(hbmpDriveNONE);
   }


   return(NULL); // why not!

}



void FAR PASCAL FolderDlgDriveDrawItem(HWND hDlg, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
LPSTR lp1;
HFONT hCurFont, hOldFont;
HBITMAP hBitmap, hBitmap2, hOldBitmap, hOldBitmap2;
HBRUSH hBrush;
HDC hCompatDC, hCompatDC2;
RECT rct;
char tbuf[MAX_PATH * 2];
BITMAP bmp;
TEXTMETRIC tm;




   *tbuf = 0;
   SendDlgItemMessage(hDlg, IDC_DRIVELIST, CB_GETLBTEXT,
                      (WPARAM)lpDrawItemStruct->itemID, (LPARAM)(LPSTR)tbuf);

   lp1 = _fstrchr(tbuf, '\t');
   if(lp1) *lp1 = 0;               // terminate string at TAB character



   hCurFont = (HFONT)SendDlgItemMessage(hDlg, IDC_DRIVELIST, WM_GETFONT, 0, 0);
   if(!hCurFont) hCurFont = (HFONT)GetStockObject(SYSTEM_FONT);

   hOldFont = SelectObject(lpDrawItemStruct->hDC, hCurFont);

   GetTextMetrics(lpDrawItemStruct->hDC, &tm);



   // FILL ENTIRE RECTANGLE WITH THE BACKGROUND COLOR!

   rct = lpDrawItemStruct->rcItem;


   // ASSIGN CORRECT BACKGROUND COLOR DEPENDING ON WHETHER THE CURRENT
   // ITEM IS HIGHLIGHTED OR NOT.

   if(lpDrawItemStruct->itemState & ODS_SELECTED)
   {
      SetBkColor(lpDrawItemStruct->hDC, GetSysColor(COLOR_HIGHLIGHT));
   }
   else
   {
      SetBkColor(lpDrawItemStruct->hDC, GetSysColor(COLOR_WINDOW));
   }

   hBrush = CreateSolidBrush(GetBkColor(lpDrawItemStruct->hDC));
   FillRect(lpDrawItemStruct->hDC, &rct, hBrush);
   DeleteObject(hBrush);
   hBrush = NULL;


   if(*tbuf)
   {
      hCompatDC = CreateCompatibleDC(lpDrawItemStruct->hDC);
      hCompatDC2 = CreateCompatibleDC(lpDrawItemStruct->hDC);


      hBitmap = (HBITMAP)
                SendDlgItemMessage(hDlg, IDC_DRIVELIST, CB_GETITEMDATA,
                                  (WPARAM)lpDrawItemStruct->itemID, 0);

//      hBitmap = GetDriveBitmap(toupper(*tbuf) - 'A' + 1);

      if(hBitmap)
      {
         GetObject(hBitmap, sizeof(bmp), (LPSTR)&bmp);
         hOldBitmap = (HBITMAP)SelectObject(hCompatDC, hBitmap);
      }
      else         // just in case...
      {
         _hmemset((LPSTR)&bmp, 0, sizeof(bmp));

         hOldBitmap = NULL;
      }


      // FOR NOW, DRAW THE ENTIRE CONTROL!  Later, I'll get 'cute'...



      // ASSIGN DEFAULT TEXT COLOR OF 'BLACK' (needed for ROP's later...)

      SetTextColor(lpDrawItemStruct->hDC, RGB(0,0,0));



      // CALCULATE THE CORRECT POSITION OF THE BITMAP

      rct.top += ((rct.bottom - rct.top) - bmp.bmHeight + 1) >> 1;



      // CREATE MONOCHROME BITMAP THAT I CAN USE AS A BIT MASK, AND A
      // COLOR BITMAP THAT I CAN USE TO PERFORM 'monkey motions' IN.

      hBitmap = CreateBitmap(bmp.bmWidth, bmp.bmHeight, 1, 1, NULL);
      hBitmap2 = CreateCompatibleBitmap(lpDrawItemStruct->hDC,
                                         bmp.bmWidth, bmp.bmHeight);

      hOldBitmap2 = SelectObject(hCompatDC2, hBitmap);

      SetBkColor(hCompatDC, GetPixel(hCompatDC, 0, 0));  // the BACKGROUND!
      SetBkColor(hCompatDC2, GetPixel(hCompatDC, 0, 0));  // the BACKGROUND!

      SetTextColor(hCompatDC, RGB(255,255,255));  // WHITE foreground!
      SetTextColor(hCompatDC2, RGB(255,255,255));  // WHITE foreground!

      BitBlt(hCompatDC2, 0, 0, bmp.bmWidth, bmp.bmHeight,
             hCompatDC, 0, 0, SRCCOPY);    // copy color to mono - anything that
                                           // isn't BKCOLOR in SOURCE becomes
                                           // BLACK; remainder is WHITE.

      // hCompatDC2 essentially has a bitmap with the BACKGROUND COLOR
      // surrounding a BLACK IMAGE.  COPY this image onto the PAINT DC using
      // the assigned FOREGROUND (BLACK) and BACKGROUND (WHITE/BLUE) colors.

      BitBlt(lpDrawItemStruct->hDC, rct.left, rct.top,
             bmp.bmWidth, bmp.bmHeight, hCompatDC2, 0, 0, SRCCOPY);

      // bitmap 'internals' are now BLACK; EXTERNALS match background!


      SelectObject(hCompatDC2, hBitmap2);  // select COLOR bitmap!
      BitBlt(hCompatDC2, 0, 0, bmp.bmWidth, bmp.bmHeight,
             hCompatDC, 0, 0, SRCCOPY);


      // NOW for some fun!  Assign the MONO bitmap to 'hCompatDC' with a
      // BLACK background and WHITE foreground, then 'AND' the mono bitmap
      // with the 'work copy' in 'hCompatDC2'.

      SelectObject(hCompatDC, hBitmap);    // select MONO (mask) bitmap!

      SetBkColor(hCompatDC, RGB(0,0,0));          // BLACK background
      SetTextColor(hCompatDC, RGB(255,255,255));  // WHITE foreground!

      SetBkColor(hCompatDC2, RGB(0,0,0));          // BLACK background
      SetTextColor(hCompatDC2, RGB(255,255,255));  // WHITE foreground!

      BitBlt(hCompatDC2, 0, 0, bmp.bmWidth, bmp.bmHeight,
             hCompatDC, 0, 0, SRCAND);

      // RESULT:  'BORDER' is BLACK, internals remaind UN-CHANGED!



      // FINAL STEP:  'OR' the COLOR BITMAP with the PAINT DC and we're done!

      BitBlt(lpDrawItemStruct->hDC, rct.left, rct.top,
             bmp.bmWidth, bmp.bmHeight, hCompatDC2, 0, 0,
             SRCPAINT);


      // cleanup objects used only for the bitmap

      if(hOldBitmap)  SelectObject(hCompatDC, hOldBitmap);
      if(hOldBitmap2) SelectObject(hCompatDC2, hOldBitmap2);

      DeleteDC(hCompatDC);
      DeleteDC(hCompatDC2);



      // BITMAP HAS BEEN PAINTED!  *NOW* paint the text!!


      rct.left += bmp.bmWidth + tm.tmAveCharWidth;
      rct.top = lpDrawItemStruct->rcItem.top;


      if(lpDrawItemStruct->itemState & ODS_SELECTED)
      {
         SetTextColor(lpDrawItemStruct->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
      }
      else
      {
         SetTextColor(lpDrawItemStruct->hDC, GetSysColor(COLOR_MENUTEXT));
      }

      TextOut(lpDrawItemStruct->hDC, rct.left, rct.top, tbuf, lstrlen(tbuf));
   }


   if(lpDrawItemStruct->itemState & ODS_FOCUS)
   {
      rct = lpDrawItemStruct->rcItem;

      rct.left ++;
      rct.top ++;
      rct.right --;
      rct.bottom --;

      DrawFocusRect(lpDrawItemStruct->hDC, &rct);
   }


   // CLEANUP SECTION - DELETE OBJECTS AND SO ON...

   DeleteObject(hBitmap);
   DeleteObject(hBitmap2);

   if(hOldFont) SelectObject(lpDrawItemStruct->hDC, hOldFont);

}


void FAR PASCAL FolderDlgDirDrawItem(HWND hDlg, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
HFONT hCurFont, hOldFont;
HBITMAP hBitmap, hBitmap2, hOldBitmap, hOldBitmap2;
HBRUSH hBrush;
HDC hCompatDC, hCompatDC2;
RECT rct;
LPSTR lp1;
char tbuf[MAX_PATH];
BITMAP bmp;
WORD CurDirIndex;
TEXTMETRIC tm;



   *tbuf = 0;
   SendDlgItemMessage(hDlg, IDC_DIRLIST, LB_GETTEXT,
                      (WPARAM)lpDrawItemStruct->itemID, (LPARAM)(LPSTR)tbuf);

   lp1 = _fstrchr(tbuf, '\t');
   if(lp1) *lp1 = 0;               // terminate string at TAB character



   hCurFont = (HFONT)SendDlgItemMessage(hDlg, IDC_DIRLIST, WM_GETFONT, 0, 0);
   if(!hCurFont) hCurFont = (HFONT)GetStockObject(SYSTEM_FONT);

   hOldFont = SelectObject(lpDrawItemStruct->hDC, hCurFont);

   GetTextMetrics(lpDrawItemStruct->hDC, &tm);



   // FILL ENTIRE RECTANGLE WITH THE BACKGROUND COLOR!

   rct = lpDrawItemStruct->rcItem;


   // ASSIGN CORRECT BACKGROUND COLOR DEPENDING ON WHETHER THE CURRENT
   // ITEM IS HIGHLIGHTED OR NOT.

   if(lpDrawItemStruct->itemState & ODS_SELECTED)
   {
      SetBkColor(lpDrawItemStruct->hDC, GetSysColor(COLOR_HIGHLIGHT));
   }
   else
   {
      SetBkColor(lpDrawItemStruct->hDC, GetSysColor(COLOR_WINDOW));
   }

   hBrush = CreateSolidBrush(GetBkColor(lpDrawItemStruct->hDC));
   FillRect(lpDrawItemStruct->hDC, &rct, hBrush);
   DeleteObject(hBrush);
   hBrush = NULL;


   if(*tbuf)
   {
      hCompatDC = CreateCompatibleDC(lpDrawItemStruct->hDC);
      hCompatDC2 = CreateCompatibleDC(lpDrawItemStruct->hDC);


//      CurDirIndex = SendDlgItemMessage(hDlg, IDC_DIRLIST, LB_GETCURSEL, 0, 0);
      CurDirIndex = wFolderDlgDirListIndex;    // it's simpler this way


      if(lpDrawItemStruct->itemID < CurDirIndex)
      {
         GetObject(hbmpOpenFolder, sizeof(bmp), (LPSTR)&bmp);
         hOldBitmap = SelectObject(hCompatDC, hbmpOpenFolder);
      }
      else if(lpDrawItemStruct->itemID == CurDirIndex)
      {
         GetObject(hbmpGrayFolder, sizeof(bmp), (LPSTR)&bmp);
         hOldBitmap = SelectObject(hCompatDC, hbmpGrayFolder);
      }
      else
      {
         GetObject(hbmpShutFolder, sizeof(bmp), (LPSTR)&bmp);
         hOldBitmap = SelectObject(hCompatDC, hbmpShutFolder);
      }



      // FOR NOW, DRAW THE ENTIRE CONTROL!  Later, I'll get 'cute'...



      // ASSIGN DEFAULT TEXT COLOR OF 'BLACK' (needed for ROP's later...)

      SetTextColor(lpDrawItemStruct->hDC, RGB(0,0,0));



      // CALCULATE THE CORRECT 'indenture' OF THE BITMAP

      if(lpDrawItemStruct->itemID <= CurDirIndex)
      {
         rct.left += tm.tmAveCharWidth * lpDrawItemStruct->itemID;
      }
      else
      {
         rct.left += tm.tmAveCharWidth * (CurDirIndex + 1);
      }

      rct.top += ((rct.bottom - rct.top) - bmp.bmHeight + 1) >> 1;



      // CREATE MONOCHROME BITMAP THAT I CAN USE AS A BIT MASK, AND A
      // COLOR BITMAP THAT I CAN USE TO PERFORM 'monkey motions' IN.

      hBitmap = CreateBitmap(bmp.bmWidth, bmp.bmHeight, 1, 1, NULL);
      hBitmap2 = CreateCompatibleBitmap(lpDrawItemStruct->hDC,
                                         bmp.bmWidth, bmp.bmHeight);

      hOldBitmap2 = SelectObject(hCompatDC2, hBitmap);

      SetBkColor(hCompatDC, GetPixel(hCompatDC, 0, 0));  // the BACKGROUND!
      SetBkColor(hCompatDC2, GetPixel(hCompatDC, 0, 0));  // the BACKGROUND!

      SetTextColor(hCompatDC, RGB(255,255,255));  // WHITE foreground!
      SetTextColor(hCompatDC2, RGB(255,255,255));  // WHITE foreground!

      BitBlt(hCompatDC2, 0, 0, bmp.bmWidth, bmp.bmHeight,
             hCompatDC, 0, 0, SRCCOPY);    // copy color to mono - anything that
                                           // isn't BKCOLOR in SOURCE becomes
                                           // BLACK; remainder is WHITE.

      // hCompatDC2 essentially has a bitmap with the BACKGROUND COLOR
      // surrounding a BLACK IMAGE.  COPY this image onto the PAINT DC using
      // the assigned FOREGROUND (BLACK) and BACKGROUND (WHITE/BLUE) colors.

      BitBlt(lpDrawItemStruct->hDC, rct.left, rct.top,
             bmp.bmWidth, bmp.bmHeight, hCompatDC2, 0, 0, SRCCOPY);

      // bitmap 'internals' are now BLACK; EXTERNALS match background!


      SelectObject(hCompatDC2, hBitmap2);  // select COLOR bitmap!
      BitBlt(hCompatDC2, 0, 0, bmp.bmWidth, bmp.bmHeight,
               hCompatDC, 0, 0, SRCCOPY);


      // NOW for some fun!  Assign the MONO bitmap to 'hCompatDC' with a
      // BLACK background and WHITE foreground, then 'AND' the mono bitmap
      // with the 'work copy' in 'hCompatDC2'.

      SelectObject(hCompatDC, hBitmap);    // select MONO (mask) bitmap!

      SetBkColor(hCompatDC, RGB(0,0,0));          // BLACK background
      SetTextColor(hCompatDC, RGB(255,255,255));  // WHITE foreground!

      SetBkColor(hCompatDC2, RGB(0,0,0));          // BLACK background
      SetTextColor(hCompatDC2, RGB(255,255,255));  // WHITE foreground!

      BitBlt(hCompatDC2, 0, 0, bmp.bmWidth, bmp.bmHeight,
               hCompatDC, 0, 0, SRCAND);

      // RESULT:  'BORDER' is BLACK, internals remaind UN-CHANGED!



      // FINAL STEP:  'OR' the COLOR BITMAP with the PAINT DC and we're done!

      BitBlt(lpDrawItemStruct->hDC, rct.left, rct.top,
             bmp.bmWidth, bmp.bmHeight, hCompatDC2, 0, 0,
             SRCPAINT);




      // cleanup objects used for the bitmap only

      if(hOldBitmap)  SelectObject(hCompatDC, hOldBitmap);
      if(hOldBitmap2) SelectObject(hCompatDC2, hOldBitmap2);

      DeleteObject(hBitmap);
      DeleteObject(hBitmap2);


      // BITMAP HAS BEEN PAINTED!  *NOW* paint the text!!


      rct.left += bmp.bmWidth + tm.tmAveCharWidth;
      rct.top = lpDrawItemStruct->rcItem.top;


      if(lpDrawItemStruct->itemState & ODS_SELECTED)
      {
         SetTextColor(lpDrawItemStruct->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
      }
      else
      {
         SetTextColor(lpDrawItemStruct->hDC, GetSysColor(COLOR_MENUTEXT));
      }

      if(!lpDrawItemStruct->itemID)
      {
         TextOut(lpDrawItemStruct->hDC, rct.left, rct.top, tbuf,
                   lstrlen(tbuf));
      }
      else // find the LAST '\\' in the name and go from there
      {
         lp1 = _fstrrchr(tbuf, '\\');   // it *BETTER* work!
         if(!lp1) lp1 = tbuf - 1;     // but, just in case...

         TextOut(lpDrawItemStruct->hDC, rct.left, rct.top, lp1 + 1,
                   lstrlen(lp1 + 1));
      }
   }


   if(lpDrawItemStruct->itemState & ODS_FOCUS)
   {
      rct = lpDrawItemStruct->rcItem;

      rct.left ++;
      rct.top ++;
      rct.right --;
      rct.bottom --;

      DrawFocusRect(lpDrawItemStruct->hDC, &rct);
   }



   // CLEANUP SECTION - DELETE OBJECTS AND SO ON...

   DeleteDC(hCompatDC);
   DeleteDC(hCompatDC2);


   if(hOldFont) SelectObject(lpDrawItemStruct->hDC, hOldFont);


}

#pragma code_seg()


/***************************************************************************/
/*                                                                         */
/*             'QUEUE' Dialog Processing (functions, etc.)                 */
/*                                                                         */
/***************************************************************************/


#pragma code_seg("QUEUEDLG_TEXT","CODE")


void NEAR PASCAL QueueDialogUpdate(HWND hDlg)
{
LPCOPY_INFO lpCI;
LPFORMAT_REQUEST lpFR;
LPDIR_INFO lpDI;
WORD wNFiles, wCurFile;
char tbuf[MAX_PATH * 2 + 32], fullname[16];



   if(FormatSuspended)
   {
      EnableWindow(GetDlgItem(hDlg, IDC_SUSPEND_FORMAT), FALSE);
      EnableWindow(GetDlgItem(hDlg, IDC_RESUME_FORMAT), TRUE);
   }
   else
   {
      EnableWindow(GetDlgItem(hDlg, IDC_SUSPEND_FORMAT), TRUE);
      EnableWindow(GetDlgItem(hDlg, IDC_RESUME_FORMAT), FALSE);
   }

   if(!lpFormatRequest /* !formatting */)
   {
      EnableWindow(GetDlgItem(hDlg, IDC_FORMAT_QUEUE), FALSE);
      EnableWindow(GetDlgItem(hDlg, IDC_REMOVE_FORMAT), FALSE);
      SendDlgItemMessage(hDlg, IDC_FORMAT_QUEUE, LB_RESETCONTENT, 0, 0);
   }
   else
   {
      EnableWindow(GetDlgItem(hDlg, IDC_FORMAT_QUEUE), TRUE);

      SendDlgItemMessage(hDlg, IDC_FORMAT_QUEUE, WM_SETREDRAW, 0, 0);
      SendDlgItemMessage(hDlg, IDC_FORMAT_QUEUE, LB_RESETCONTENT, 0, 0);

      if(lpFR = lpFormatRequest)
      {
         EnableWindow(GetDlgItem(hDlg, IDC_REMOVE_FORMAT), TRUE);

         while(lpFR)
         {
            wsprintf(tbuf, "%c: LABEL=\"%s\" /T:%d /N:%d Heads:%d",
                     (WORD)('A' + lpFR->wDrive),
                     (LPSTR)lpFR->label,
                     (WORD)lpFR->wTracks,
                     (WORD)lpFR->wSectors,
                     (WORD)lpFR->wHeads);

            if(lstrlen(tbuf) < MAX_PATH * 2)
            {
               _hmemset(tbuf + lstrlen(tbuf), ' ',
                        MAX_PATH * 2 - lstrlen(tbuf));
            }

            // at the end of the buffer, add a section to describe
            // this entry so that I can leave *NO* doubt about it

            wsprintf(tbuf + MAX_PATH * 2, "%08lxFFFFFFFF", (DWORD)lpFR);

            SendDlgItemMessage(hDlg, IDC_FORMAT_QUEUE, LB_INSERTSTRING,
                               (WPARAM)-1, (LPARAM)(LPSTR)tbuf);

            lpFR = lpFR->lpNext;
         }
      }
      else
      {
         EnableWindow(GetDlgItem(hDlg, IDC_REMOVE_FORMAT), FALSE);
      }

      SendDlgItemMessage(hDlg, IDC_FORMAT_QUEUE, WM_SETREDRAW, (WPARAM)1, 0);
      InvalidateRect(GetDlgItem(hDlg, IDC_FORMAT_QUEUE), NULL, TRUE);
      UpdateWindow(GetDlgItem(hDlg, IDC_COPY_QUEUE));
   }



   if(CopySuspended)
   {
      EnableWindow(GetDlgItem(hDlg, IDC_SUSPEND_COPY), FALSE);
      EnableWindow(GetDlgItem(hDlg, IDC_RESUME_COPY), TRUE);
   }
   else
   {
      EnableWindow(GetDlgItem(hDlg, IDC_SUSPEND_COPY), TRUE);
      EnableWindow(GetDlgItem(hDlg, IDC_RESUME_COPY), FALSE);
   }

   if(!lpCopyInfo /* copying */)
   {
      EnableWindow(GetDlgItem(hDlg, IDC_COPY_QUEUE), FALSE);
      EnableWindow(GetDlgItem(hDlg, IDC_REMOVE_COPY), FALSE);
      SendDlgItemMessage(hDlg, IDC_COPY_QUEUE, LB_RESETCONTENT, 0, 0);
   }
   else
   {
      EnableWindow(GetDlgItem(hDlg, IDC_COPY_QUEUE), TRUE);

      SendDlgItemMessage(hDlg, IDC_COPY_QUEUE, WM_SETREDRAW, 0, 0);
      SendDlgItemMessage(hDlg, IDC_COPY_QUEUE, LB_RESETCONTENT, 0, 0);

      lpCI = lpCopyInfo;
      wNFiles = 0;

      while(lpCI)
      {
         if(lpCI->next_source)
         {
            // add information to the list box at this time

            _hmemset(tbuf, 0, sizeof(tbuf));

            _fstrncpy(tbuf, lpCI->next_source,
                      2 * MAX_PATH);      // make a copy of source spec only!

            if(lstrlen(tbuf) < MAX_PATH * 2)
            {
               _hmemset(tbuf + lstrlen(tbuf), ' ',
                        MAX_PATH * 2 - lstrlen(tbuf));
            }

            // at the end of the buffer, add a section to describe
            // this entry so that I can leave *NO* doubt about it

            wsprintf(tbuf + MAX_PATH * 2, "%08lxFFFFFFFF", (DWORD)lpCI);

            SendDlgItemMessage(hDlg, IDC_COPY_QUEUE, LB_INSERTSTRING,
                               (WPARAM)-1, (LPARAM)(LPSTR)tbuf);
         }
         else
         {

            if(lpCI->wCurFile) wCurFile = lpCI->wCurFile - 1;
            else               wCurFile = 0;


            for(; wCurFile<lpCI->wNFiles; wCurFile++)
            {
               // add information to the list box at this time

               _hmemset(tbuf, 0, sizeof(tbuf));  /* intially zero it */

               lpDI = lpCI->lpDirInfo + wCurFile;


               // add source file spec to tbuf

               _hmemset(fullname, 0, sizeof(fullname));
               _hmemcpy(fullname, lpDI->fullname, sizeof(lpDI->fullname));

               lstrcpy(tbuf, lpCI->srcpath);

               if(tbuf[lstrlen(tbuf)-1]!='\\')
                  lstrcat(tbuf, "\\");

               lstrcat(tbuf, fullname);

               DosQualifyPath(tbuf, tbuf);


               // add destination filespec/path to tbuf

               lstrcat(tbuf, " -> ");

               UpdateFileName(tbuf + lstrlen(tbuf), lpCI->dest, fullname);

               if(lstrlen(tbuf) < MAX_PATH * 2)
               {
                  _hmemset(tbuf + lstrlen(tbuf), ' ',
                           MAX_PATH * 2 - lstrlen(tbuf));
               }


               // at the end of the buffer, add a section to describe
               // this entry so that I can leave *NO* doubt about it

               wsprintf(tbuf + MAX_PATH * 2, "%08lx%08lx",
                        (DWORD)lpCI,(DWORD)wCurFile);

               SendDlgItemMessage(hDlg, IDC_COPY_QUEUE, LB_INSERTSTRING,
                                  (WPARAM)-1, (LPARAM)(LPSTR)tbuf);
            }
         }

         wNFiles += lpCI->wNFiles - lpCI->wCurFile;

         lpCI = lpCI->lpNext;
      }

      if(wNFiles)
      {
         EnableWindow(GetDlgItem(hDlg, IDC_REMOVE_COPY), TRUE);
      }
      else
      {
         EnableWindow(GetDlgItem(hDlg, IDC_REMOVE_COPY), FALSE);
      }

      SendDlgItemMessage(hDlg, IDC_COPY_QUEUE, WM_SETREDRAW, (WPARAM)1, 0);
      InvalidateRect(GetDlgItem(hDlg, IDC_COPY_QUEUE), NULL, TRUE);
      UpdateWindow(GetDlgItem(hDlg, IDC_COPY_QUEUE));
   }

}


BOOL CALLBACK EXPORT QueueDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
                                  LPARAM lParam)
{
int i1, i2, i3;
LPFORMAT_REQUEST lpFQ, lpFQ0;
LPCOPY_INFO lpCQ, lpCQ0;
LPVOID lpV;



   switch(msg)
   {
      case WM_INITDIALOG:

         SubclassStaticControls(hDlg);

         QueueDialogUpdate(hDlg);

         return(TRUE);



      case WM_CTLCOLOR:
         return((BOOL)DlgOn_WMCtlColor(hDlg, wParam, lParam));



      case WM_ERASEBKGND:

         {
          HDC hdcBackGround = (HDC)wParam;
          RECT rctClient;

            GetClientRect(hDlg, &rctClient);

            FillRect(hdcBackGround, &rctClient, hbrDlgBackground);
         }

         return(TRUE);



      case WM_COMMAND:

         switch(LOWORD(wParam))
         {
            case 0x4321:

               if(lParam == (LPARAM)0x12345678)
               {
                  QueueDialogUpdate(hDlg);
               }

               break;


            case IDCANCEL:

               {
                static BOOL bDestroyRecurse = FALSE;

                  if(!bDestroyRecurse)
                  {
                     bDestroyRecurse = TRUE;
                     DestroyWindow(hDlg);  // destroy it (hope it don't blow up!)
                     bDestroyRecurse = FALSE;
                  }

                  return(TRUE);
               }



            case IDC_SUSPEND_FORMAT:

               MthreadSuspendThread(hFormatThread);  // suspend format

               FormatSuspended = TRUE;
               EnableWindow(GetDlgItem(hDlg, IDC_RESUME_FORMAT), TRUE);
               SetFocus(GetDlgItem(hDlg, IDC_RESUME_FORMAT));
               EnableWindow(GetDlgItem(hDlg, IDC_SUSPEND_FORMAT), FALSE);

               UpdateStatusBar();

               break;


            case IDC_RESUME_FORMAT:

               FormatSuspended = FALSE;
               EnableWindow(GetDlgItem(hDlg, IDC_SUSPEND_FORMAT), TRUE);
               SetFocus(GetDlgItem(hDlg, IDC_SUSPEND_FORMAT));
               EnableWindow(GetDlgItem(hDlg, IDC_RESUME_FORMAT), FALSE);

               MthreadResumeThread(hFormatThread);

               UpdateStatusBar();

               break;


            case IDC_REMOVE_FORMAT:  // only if suspended

               if(!FormatSuspended)
               {
                  MessageBox(hDlg, "You cannot remove this item unless FORMAT is suspended",
                             "* SFTShell FORMAT QUEUE *", MB_OK | MB_ICONHAND);
                  break;
               }

               i1 = SendDlgItemMessage(hDlg, IDC_FORMAT_QUEUE, LB_GETCURSEL, 0, 0);
               if(i1 == LB_ERR)
               {
                  MessageBox(hDlg, "No item selected",
                             "* SFTShell FORMAT QUEUE *", MB_OK | MB_ICONHAND);
                  break;
               }

               if(i1 == 0)  // the CURRENT item...
               {
                  if(FormatCancelFlag)
                  {
                     MessageBox(hDlg, "Current item is already marked for cancellation - RESUME FORMAT to remove it",
                                "* SFTShell FORMAT QUEUE *", MB_OK | MB_ICONINFORMATION);
                  }
                  else if(MessageBox(hDlg, "Cancel current FORMAT in progress?",
                                     "* SFTShell FORMAT QUEUE *",
                                     MB_OKCANCEL | MB_ICONHAND)
                          == IDOK)
                  {
                     FormatCancelFlag = TRUE;

                     MessageBox(hDlg, "Current item will be removed from the QUEUE when FORMAT is RESUMEd",
                                "* SFTShell FORMAT QUEUE *", MB_OK | MB_ICONINFORMATION);
                  }
               }
               else  // NOTE:  this is NEVER the FIRST item!
               {
                  // physically remove item from queue!
                  // need to traverse the queue for the 'i1'th item...

                  lpFQ0 = NULL;

                  for(i2=0, lpFQ = lpFormatRequest; i2 < i1 && lpFQ; i2++)
                  {
                     lpFQ0 = lpFQ;         // previous item

                     lpFQ = lpFQ->lpNext;  // traverse the list...
                  }

                  if(lpFQ && lpFQ0 && i2 == i1)  // item found!
                  {
                     lpFQ0->lpNext = lpFQ->lpNext;  // skip current one

                     GlobalFreePtr((LPVOID)lpFQ);
                  }
                  else
                  {
                     MessageBox(hDlg, "Unexpected error while attempting to remove current item from FORMAT queue",
                                "* SFTShell FORMAT QUEUE *", MB_OK | MB_ICONHAND);
                  }
               }

               QueueDialogUpdate(hDlg);
               break;


            case IDC_SUSPEND_COPY:

               MthreadSuspendThread(hCopyThread);  // suspend format

               CopySuspended = TRUE;
               EnableWindow(GetDlgItem(hDlg, IDC_RESUME_COPY), TRUE);
               SetFocus(GetDlgItem(hDlg, IDC_RESUME_COPY));
               EnableWindow(GetDlgItem(hDlg, IDC_SUSPEND_COPY), FALSE);

               UpdateStatusBar();

               break;


            case IDC_RESUME_COPY:

               CopySuspended = FALSE;
               EnableWindow(GetDlgItem(hDlg, IDC_SUSPEND_COPY), TRUE);
               SetFocus(GetDlgItem(hDlg, IDC_SUSPEND_COPY));
               EnableWindow(GetDlgItem(hDlg, IDC_RESUME_COPY), FALSE);

               MthreadResumeThread(hCopyThread);

               UpdateStatusBar();

               break;


            case IDC_REMOVE_COPY:

               if(!CopySuspended)
               {
                  MessageBox(hDlg, "You cannot remove this item unless COPY is suspended",
                             "* SFTShell COPY QUEUE *", MB_OK | MB_ICONHAND);
                  break;
               }

               i1 = SendDlgItemMessage(hDlg, IDC_COPY_QUEUE, LB_GETCURSEL, 0, 0);
               if(i1 == LB_ERR)
               {
                  MessageBox(hDlg, "No item selected",
                             "* SFTShell COPY QUEUE *", MB_OK | MB_ICONHAND);
                  break;
               }

               if(i1 == 0)  // the CURRENT (first) item...
               {
                  if(CopyCancelFlag)
                  {
                     MessageBox(hDlg, "Current item is already marked for cancellation - RESUME COPY to remove it",
                                "* SFTShell COPY QUEUE *", MB_OK | MB_ICONINFORMATION);
                  }
                  else if(MessageBox(hDlg, "Cancel current COPY operation in progress?",
                                     "* SFTShell COPY QUEUE *",
                                     MB_OKCANCEL | MB_ICONHAND)
                          == IDOK)
                  {
                     CopyCancelFlag = TRUE;

                     MessageBox(hDlg, "Current item will be removed from the QUEUE when COPY is RESUMEd",
                                "* SFTShell COPY QUEUE *", MB_OK | MB_ICONINFORMATION);
                  }
               }
               else  // NOTE:  this is NEVER the FIRST item!
               {
                  // physically remove item from copy queue!
                  // need to traverse the queue for the 'i1'th item...
                  // (this may be more difficult due to multi-file copies)


                  lpCQ0 = NULL;

                  for(i2=0, lpCQ = lpCopyInfo; i2 < i1 && lpCQ;)
                  {

                     // get items within the copy structure itself...

                     if(lpCQ->next_source)
                     {
                        i2++;   // count 1 for 1 entry
                     }
                     else
                     {
                        // multiple entries... add one for each not yet done

                        if(lpCQ->wCurFile)  // subtract one to get actual index
                        {
                           i3 = (int)(lpCQ->wNFiles - lpCQ->wCurFile + 1);
                        }
                        else
                        {
                           i3 = (int)lpCQ->wNFiles;
                        }

                        if(!lpCQ0) lpCQ0 = lpCQ;   // in case it's the first one

                        if((i2 + i3) > i1) break;  // I FOUND IT!

                        i2 += i3;                  // increment 'i2' by # of files
                     }

                     lpCQ0 = lpCQ;
                     lpCQ = lpCQ->lpNext;
                  }

                  if(lpCQ && lpCQ0 &&           // I actually found the item
                     ((lpCQ->next_source && i1 == i2) ||
                      (!lpCQ->next_source && i2 <= i1)))
                  {
                     // TODO:  verify that I have the correct item by checking
                     //        the right 16 characters of the listbox entry


                     if(lpCQ->next_source || lpCQ->wNFiles < 2)
                     {                          // remove this item OUTRIGHT
                        lpCQ0->lpNext = lpCQ->lpNext;

                        GlobalFreePtr(lpCQ->lpDirInfo);
                        GlobalFreePtr(lpCQ);


                        // NOTE:  I shall NEVER be removing the CURRENT item

                     }
                     else
                     {
                        i3 = i1 - i2;       // 'wCurFile' index for item to remove


                        if((i3 + 1) < (int)lpCQ->wNFiles)
                        {
                           _hmemcpy(lpCQ->lpDirInfo + i3,
                                    lpCQ->lpDirInfo + i3 + 1,
                                    sizeof(*(lpCQ->lpDirInfo))
                                    * (lpCQ->wNFiles - i3 - 1));
                        }

                        _hmemset(lpCQ->lpDirInfo + lpCQ->wNFiles - 1, 0,
                                 sizeof(*(lpCQ->lpDirInfo)));

                        lpCQ->wNFiles--;
                     }

                  }
                  else
                  {
                     MessageBox(hDlg, "Unexpected error while attempting to remove current item from COPY queue",
                                "* SFTShell COPY QUEUE *", MB_OK | MB_ICONHAND);
                  }

               }

               QueueDialogUpdate(hDlg);
               break;

         }

         return(TRUE);


      case WM_DESTROY:
         hdlgQueue = NULL;

               // disable menu and toolbar button

         EnableMenuItem(GetMenu(hMainWnd), IDM_QUEUE_MENU,
                        MF_ENABLED | MF_BYCOMMAND);
         DrawMenuBar(hMainWnd);  // necessary when menu item is top-level

         EnableToolBarButton(IDM_QUEUE_MENU, 1);
         break;

   }


   return(FALSE);
}




/***************************************************************************/
/*                                                                         */
/*              'INSTANT BATCH' edit/spawn dialog support                  */
/*                                                                         */
/***************************************************************************/


BOOL CALLBACK EXPORT InstantBatchDlgProc(HWND hDlg, UINT msg, WPARAM wParam,
                                         LPARAM lParam)
{
HANDLE h1;
LPSTR lp1;
WORD w1;
static const char CODE_BASED szMsgHdr[]=
                                        "** SFTShell INSTANT BATCH Error **";
static const char CODE_BASED szInitialBatch[]=
   "@ECHO OFF\r\n"
   "; Enter your program text here and press 'OK' to\r\n"
   "; execute these commands as a 'memory batch file', or\r\n"
   "; 'Cancel' to exit without executing these commands\r\n"
   ";****************************************************\r\n";


   switch(msg)
   {
      case WM_INITDIALOG:

//         SubclassEditControls(hDlg);

         SetDlgItemText(hDlg, IDC_BATCH_TEXT, szInitialBatch);

         SetFocus(GetDlgItem(hDlg, IDC_BATCH_TEXT));

         w1 = (WORD)SendDlgItemMessage(hDlg, IDC_BATCH_TEXT,
                                       WM_GETTEXTLENGTH, 0, 0);

         SendDlgItemMessage(hDlg, IDC_BATCH_TEXT, EM_SETSEL, 0,
                            MAKELPARAM(w1,w1));

         return(0);



      case WM_CTLCOLOR:
         return((BOOL)DlgOn_WMCtlColor(hDlg, wParam, lParam));



      case WM_ERASEBKGND:

         {
          HDC hdcBackGround = (HDC)wParam;
          RECT rctClient;

            GetClientRect(hDlg, &rctClient);

            FillRect(hdcBackGround, &rctClient, hbrDlgBackground);
         }

         return(TRUE);



      case WM_COMMAND:

         switch(LOWORD(wParam))
         {
//            case IDC_BATCH_TEXT:
//
//               if(HIWORD(lParam)==EN_SETFOCUS)
//               {
//                  SendMessage(hDlg, DM_SETDEFID, 0, 0);  // remove default
//               }
//               else if(HIWORD(lParam)==EN_KILLFOCUS)
//               {
//                  SendMessage(hDlg, DM_SETDEFID, (WPARAM)IDYES, 0);
//                                                   // remove default setting
//               }
//
//               break;


            case IDYES:

               w1 = 2 + (WORD)SendDlgItemMessage(hDlg, IDC_BATCH_TEXT,
                                                 WM_GETTEXTLENGTH, 0, 0);

               h1 = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT, w1);
               if(!h1 || !(lp1 = GlobalLock(h1)))
               {
                  MessageBox(hDlg, "?Not enough memory to execute memory batch file",
                             szMsgHdr, MB_OK | MB_ICONHAND);

                  if(h1) GlobalFree(h1);

                  return(TRUE);  // I executed SOMETHING but don't exit dialog
               }

               GetDlgItemText(hDlg, IDC_BATCH_TEXT, lp1, w1);
               GlobalUnlock(h1);

               if(SpawnThread(MemoryBatchFileThreadProc, h1,
                              THREADSTYLE_DEFAULT))
               {
                  MessageBox(hDlg, "?Unable to spawn thread for memory batch file",
                             szMsgHdr, MB_OK | MB_ICONHAND);

                  GlobalFree(h1);
               }
               else
               {
                  GlobalFree(h1);

                  EndDialog(hDlg, 1);
               }

               return(TRUE);



            case IDCANCEL:

               EndDialog(hDlg, 0);
               return(TRUE);

         }

         return(FALSE);

   }


   return(FALSE);
}






/***************************************************************************/
/*                            DDEML CALLBACK                               */
/*                                                                         */
/***************************************************************************/


#pragma code_seg("DDEML_TEXT","CODE")


LPSTR FAR PASCAL FindDdeEnvStr(LPSTR lpList, LPSTR lpStr)
{
LPSTR lp1, lp2, lp3;
int i;



   lp1 = lpList;

   while(lp1 && *lp1)
   {
      lp2 = _fstrchr(lp1, ',');
      if(lp2) *lp2 = 0;    /* temporary */

      if(*lpStr!='|')       /* is the server name 'blank'? */
      {
         i = _fstricmp(lp1, lpStr);  // no - include it in the comparison
      }
      else
      {
         lp3 = _fstrchr(lp1, '|');
         if(lp3)
         {
            i = _fstricmp(lp3, lpStr);
         }
         else
            i = 1;  /* no match */
      }

      if(lp2) *lp2 = ',';  /* restore it */

      if(!i) return(lp1);

      if(lp2) lp1 = lp2+1;
      else    break;
   }

   return(NULL);  // not found!
}


BOOL FAR PASCAL ItemHasHotLink(LPCSTR szItem)
{
LPSTR lp1;


   if(lpHotLinkItems)
   {
      lp1 = lpHotLinkItems;

      while(*lp1)
      {
         if(!_fstricmp(lp1, szItem))  // found?
         {
            return(TRUE);
         }

         lp1 += lstrlen(lp1) + 1 + sizeof(WORD);
      }
   }

   return(FALSE);
}


HDDEDATA EXPENTRY MyDdeCallBack(WORD wType, WORD wFmt, HCONV hConv,
                                HSZ hsz1, HSZ hsz2, HDDEDATA hData,
                                DWORD dwData1, DWORD dwData2)
{
LPSTR lp1, lp2, lp3;
WORD w, w2, w3;
BOOL IsSystemTopic;
HDDEDATA hRval;
HSZPAIR lphp1[64];  /* maximum of 64 server/topic pairs */
char temp_buf[128];


   switch(wType)
   {
      case XTYP_REQUEST:
      case XTYP_ADVREQ:
         if(wFmt!=CF_TEXT && wFmt!=CF_OEMTEXT) return(0);

         lpDdeQueryString(idDDEInst,hsz1,temp_buf,sizeof(temp_buf),CP_WINANSI);
         IsSystemTopic = !_fstricmp(temp_buf, pSYSTEM);

         lpDdeQueryString(idDDEInst,hsz2,temp_buf,sizeof(temp_buf),CP_WINANSI);

         if(!*temp_buf)
         {
            return(0);
         }

         if(IsSystemTopic)
         {
            if(!_fstricmp(temp_buf, SZDDESYS_ITEM_FORMATS))
            {
               wsprintf(temp_buf,"%d\tTEXT\r\n%d\tOEM TEXT\r\n",
                        (WORD)CF_TEXT,(WORD)CF_OEMTEXT);
               lp1 = temp_buf;
            }
            else if(!_fstricmp(temp_buf, SZDDESYS_ITEM_STATUS))
            {
               if(GettingUserInput || BatchMode)
                 lp1 = "BUSY";
               else
                 lp1 = "READY";

            }
            else if(!_fstricmp(temp_buf, SZDDESYS_ITEM_SYSITEMS))
            {
               lp1 = SZDDESYS_ITEM_FORMATS "\r\n"
                     SZDDESYS_ITEM_SYSITEMS "\r\n"
                     SZDDESYS_ITEM_STATUS "\r\n";
            }
            else
            {
               lp1 = GetEnvString(temp_buf);
               if(!lp1) lp1 = "";
            }
         }
         else
         {
            lp1 = GetEnvString(temp_buf);
            if(!lp1) lp1 = "";
         }

         if(wFmt==CF_OEMTEXT)
         {
            lp2 = GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                 lstrlen(lp1) + 1);
            if(!lp2)
            {
               wFmt = CF_TEXT;
            }
            else
            {
               AnsiToOem(lp1, lp2);
               lp1 = lp2;
            }
         }

         hRval = lpDdeCreateDataHandle(idDDEInst, lp1, lstrlen(lp1)+1, 0,
                                       hsz2, wFmt, 0);

         if(wFmt==CF_OEMTEXT && lp2) GlobalFreePtr(lp2);

         return(hRval);



      case XTYP_ADVSTOP:

         if(lpHotLinkItems)
         {
            // find this item in 'lpHotLinkItems' and remove it

            lpDdeQueryString(idDDEInst, hsz2, temp_buf, sizeof(temp_buf),
                             CP_WINANSI);

            lp1 = lpHotLinkItems;

            while(*lp1)
            {
               if(!_fstricmp(lp1, temp_buf))  // found?
               {
                  lp2 = lp1 + lstrlen(lp1) + 1;

                  if(*((WORD FAR *)lp2)) *((WORD FAR *)lp2)--;

                  if(!*((WORD FAR *)lp2))  // remove item from list...
                  {
                     // find the end...

                     lp2 += sizeof(WORD);  // point to next item
                     lp3 = lp2;

                     while(*lp3)
                     {
                        lp3 += lstrlen(lp3) + 1 + sizeof(WORD);
                     }

                     if(lp3 > lp2)         // something to copy...
                     {
                        _hmemcpy(lp1, lp2, (lp3 - lp2 + 1));

                        _hmemset(lp3, 0, (lp2 - lp1));
                     }
                     else
                     {
                        _hmemset(lp1, 0, (lp2 - lp1));
                     }
                  }

                  break;
               }

               lp1 += lstrlen(lp1) + 1 + sizeof(WORD);
            }

            if(!*lpHotLinkItems)
            {
               GlobalFreePtr(lpHotLinkItems);

               lpHotLinkItems = NULL;
            }
         }

         return(0);



      case XTYP_ADVSTART:

         // get item to begin hot link on...

         lpDdeQueryString(idDDEInst, hsz2, temp_buf, sizeof(temp_buf),
                          CP_WINANSI);

         // search for item in 'lpHotLinkItems' and add if not there...

         if(!lpHotLinkItems)  // buffer doesn't exist...
         {
            lpHotLinkItems = GlobalAllocPtr(GHND, 0x1000);

            if(!lpHotLinkItems)
            {
               return((HDDEDATA)0);  // an error, really...
            }

            lstrcpy(lpHotLinkItems, temp_buf);  // add entry
            lp1 = lpHotLinkItems + lstrlen(lpHotLinkItems) + 1;
            *((WORD FAR *)lp1) = 1;

            lp1 += sizeof(WORD);

            *lp1 = 0;   // this marks the end of the entry
         }
         else
         {
            lp1 = lpHotLinkItems;

            while(*lp1)
            {
               if(!_fstricmp(lp1, temp_buf))  // found?
               {
                  lp2 = lp1 + lstrlen(lp1) + 1;

                  *((WORD FAR *)lp2)++;
               }

               lp1 += lstrlen(lp1) + 1 + sizeof(WORD);
            }

            if(!*lp1)  // item not found - add to the list!
            {
               w = (lp1 - lpHotLinkItems) + lstrlen(temp_buf) + 1
                     + sizeof(WORD) + 64;

               if(w >= GlobalSizePtr(lpHotLinkItems))
               {
                  // need to re-allocate the buffer!

                  lp2 = GlobalReAllocPtr(lpHotLinkItems,
                                         (w + 0x1fff) & 0xf000L,
                                         GMEM_MOVEABLE);

                  if(!lp2) return((HDDEDATA)0);  // not enough memory!

                  lp1 = lp2 + (lp1 - lpHotLinkItems);

               }

               // add this item to end of buffer!

               lstrcpy(lp1, temp_buf);

               lp1 += lstrlen(lp1) + 1;
               *((WORD FAR *)lp1) = 1;

               lp1 += sizeof(WORD);
               *lp1 = 0;
            }
         }

         lpDdePostAdvise(idDDEInst, hsz1, hsz2);  /* cause advisory post */
         return((HDDEDATA)TRUE);      /* causes item to be 'advised' on! */



      case XTYP_CONNECT:
      case XTYP_WILDCONNECT:

         lp1 = temp_buf;

         if(hsz2)  // is there a server ID listed? If so, it must match one
         {         // of the names in 'DDESERVERLIST', or 'pDEFSERVERNAME'.

            lpDdeQueryString(idDDEInst, hsz2, temp_buf, sizeof(temp_buf),
                             CP_WINANSI);

            if(!FindDdeEnvStr(GetEnvString(pDDESERVERLIST),temp_buf))
            {
               if(_fstricmp(lp1, pDEFSERVERNAME)) /* 'default' server? */
               {
                  return(FALSE);  /* no matching server name - no connect! */
               }
            }

            lp1 += lstrlen(lp1);
         }

         *(lp1++) = '|';  // insert the '|' into the server|topic string


         if(hsz1)  /* is there a server ID listed? */
         {
            lpDdeQueryString(idDDEInst, hsz1, lp1, sizeof(temp_buf),
                             CP_WINANSI);

            if(!FindDdeEnvStr(GetEnvString(pDDESERVERTOPICLIST),temp_buf))
            {
               if(_fstricmp(lp1, pSYSTEM)) /* the 'SYSTEM' topic - always! */
               {
                  return(FALSE);  /* no matching server|topic - no connect! */
               }
            }

         }

         if(wType==XTYP_CONNECT)
         {
            hRval = (HDDEDATA)TRUE;
         }
         else
         {
            lp1 = GetEnvString(pDDESERVERTOPICLIST);

            if(!lp1) hRval = 0;
            else
            {
               w = 0;
               do
               {
                  lp2 = _fstrchr(lp1, ',');
                  if(!lp2)
                     lstrcpy(temp_buf, lp1);
                  else
                  {
                     _hmemcpy(temp_buf, lp1, (lp2 - lp1));
                     temp_buf[lp2 - lp1] = 0;
                     lp2++;
                  }

                  lp3 = _fstrchr(temp_buf, '|');
                  if(*lp3) *(lp3++) = 0;

                  lphp1[w].hszSvc = lpDdeCreateStringHandle(idDDEInst, lp1,
                                                            CP_WINANSI);
                  if(lp3)
                  {
                     lphp1[w].hszTopic = lpDdeCreateStringHandle(idDDEInst,
                                                                 lp3,
                                                                 CP_WINANSI);
                  }
                  else
                  {
                     lphp1[w].hszTopic = 0;
                  }

                  w++;
                  lp1 = lp2;

               } while(lp1 && *lp1);


              /* see if the 'default server'|SYSTEM is registered */
              /* and if not, include it in the list anyway!       */

               lstrcpy(temp_buf, pDEFSERVERNAME);
               lstrcat(temp_buf, "|");
               lstrcat(temp_buf, pSYSTEM);


               if(!FindDdeEnvStr(GetEnvString(pDDESERVERTOPICLIST),temp_buf))
               {
                  lphp1[w].hszSvc = lpDdeCreateStringHandle(idDDEInst,
                                                            pDEFSERVERNAME,
                                                            CP_WINANSI);

                  lphp1[w].hszTopic = lpDdeCreateStringHandle(idDDEInst,
                                                              pSYSTEM,
                                                              CP_WINANSI);
                  w++;
               }

               lphp1[w].hszTopic = 0;
               lphp1[w].hszSvc = 0;

               hRval = lpDdeCreateDataHandle(idDDEInst, lphp1,
                                             (w + 1)*sizeof(HSZPAIR), 0,
                                             0, CF_TEXT, 0);
            }
         }

         return(hRval);


      case XTYP_CONNECT_CONFIRM:
         return(0);

      case XTYP_DISCONNECT:
         return(0);

      case XTYP_ERROR:
         return(0);

      case XTYP_EXECUTE:

//         if(wFmt!=CF_TEXT && wFmt!=CF_OEMTEXT) return((HDDEDATA)DDE_FNOTPROCESSED);

         if(GettingUserInput)
         {
            return((HDDEDATA)DDE_FBUSY);
         }

         lpDdeGetData(hData, temp_buf, sizeof(temp_buf), 0);

         w = lstrlen(temp_buf);
         if(temp_buf[0]=='[' && temp_buf[w - 1]==']')
         {
            temp_buf[w - 1] = 0;
            lstrcpy(temp_buf, temp_buf + 1);
         }

//         if(wFmt==CF_OEMTEXT) OemToAnsi(temp_buf, temp_buf);

         if(!BatchMode)
         {
                    /* place file name into command buffer */

            lstrcpy(cmd_buf, temp_buf);
            cmd_buf_len = lstrlen(cmd_buf);

            PrintString(cmd_buf);  /* make sure the thing ECHO's */

                        /* simulate pressing <ENTER> */
            MainOnChar(hMainWnd, '\r', 1);
         }
         else      /* otherwise, place into 'TypeAhead' buffer */
         {
            lstrcat(temp_buf, "\r");

            w2 = lstrlen(temp_buf);

            w3 = TypeAheadTail;


            for(w=0; w<w2; w++)
            {
               if(TypeAheadTail>=sizeof(TypeAhead)) TypeAheadTail = 0;

               if((TypeAheadTail + 1)<TypeAheadHead ||
                  TypeAheadTail < sizeof(TypeAhead))
               {
                  TypeAhead[TypeAheadTail++] = (char)temp_buf[w];

                  if(TypeAheadTail>=sizeof(TypeAhead)) TypeAheadTail = 0;
               }
               else
               {
                  TypeAheadTail = w3;  /* restore original value */
                  return((HDDEDATA)DDE_FBUSY);
               }
            }
         }

         return((HDDEDATA)DDE_FACK);

      case XTYP_XACT_COMPLETE:
         if(!hData || hData==(HDDEDATA)TRUE) return(0);  /* ignore */
         return(0);  /* for now... */

      case XTYP_ADVDATA:
      case XTYP_POKE:

         if(wFmt!=CF_TEXT && wFmt!=CF_OEMTEXT) return((HDDEDATA)DDE_FNOTPROCESSED);

         lpDdeQueryString(idDDEInst,hsz2,temp_buf,sizeof(temp_buf),CP_WINANSI);

         if(!*temp_buf) return((HDDEDATA)DDE_FNOTPROCESSED);
         else
         {
            w = (WORD)lpDdeGetData(hData, NULL, 0, 0);
            if(!w)
            {
               DelEnvString(temp_buf);
            }
            else
            {
               lp1 = GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT, w+1);

               if(!lp1) return((HDDEDATA)DDE_FBUSY);

               lpDdeGetData(hData, lp1, w+1, 0);

               if(wFmt==CF_OEMTEXT) OemToAnsi(lp1, lp1);

               SetEnvString(temp_buf, lp1);
               GlobalFreePtr(lp1);
            }
         }

         return((HDDEDATA)DDE_FACK);

      case XTYP_REGISTER:
         return(0);

      case XTYP_UNREGISTER:
         return(0);


      default:
         return((HDDEDATA)DDE_FNOTPROCESSED);
   }

}
