/***************************************************************************/
/*                                                                         */
/*              This is a 'Scaled Down' version of MTHREAD.C               */
/*                                                                         */
/*                      TRADE SECRET - CONFIDENTIAL                        */
/*                                                                         */
/*             The compiled libraries and API header files are             */
/*          (c) Copyright 1990-1995 by Stewart~Frazier Tools, Inc          */
/*                                                                         */
/***************************************************************************/

#define STRICT
#define THE_WORKS
#include "mywin.h"
//#include "windows.h"
//#include "windowsx.h"
//#include "toolhelp.h"
//#include "stdio.h"
//#include "stdlib.h"
//#include "memory.h"
//#include "string.h"
#include "mth.h"


#pragma warning(disable:4010)


#ifndef API
#define API FAR PASCAL
#endif

#ifndef EXPORT
#define EXPORT __export
#endif /* EXPORT */

#ifndef HUGE
#define HUGE __huge
#endif // HUGE


#define _EXPORT_   /* the ones that don't *HAVE* to be exported use this */

#ifdef LOADDS
#undef LOADDS
#endif // LOADDS

#define LOADDS   /* not used here - export and use MakeProcInstance() */


//#define THREAD_STACK 12288 /* 8192 */
#define THREAD_STACK 16384


#define MTH_TIMER_ID     0     /* WM_TIMER ID for timer callback */

#define MAX_ACCEL        16
#define MAX_MDIACCEL     16

#define MAX_SEMAPHORES 32
#define MAX_SEMAPHORECALLBACKS 16


#define INIT_LEVEL do{            /* these are used to 'clean up' commonly */
#define END_LEVEL }while(FALSE);  /* used 'hierarchichal' error traps */


#define USE_32_BIT_OPERAND _asm _emit 0x66   /* prefix for 32 bit operand */
#define USE_32_BIT_ADDRESS _asm _emit 0x67   /* prefix for 32 bit address */

//#ifndef DEBUG
#define DISPATCH_WITHIN_HOOK 1
//#endif


#define DISABLE_WMQUIT  1
#define DISABLE_WMCLOSE 2

#define DISABLE_DISPATCH_NONE   0  /* 'disable_dispatch' values */
#define DISABLE_DISPATCH_MSG    0x4000
#define DISABLE_DISPATCH_THREAD 0x8000
#define DISABLE_DISPATCH_ALL    0xc000
#define DISABLE_DISPATCH_MASK   0x3fff   /* masks out 'disable' count */

#define MAX_THREADLIST 512       /* 512 threads per application (for now) */

#define MTHREAD_CMDSHOW SW_HIDE  /* 'wCmdShow' value on startup for thread */

#define BAD_ENTRY (0xffff)
#define SEM_HANDLE_OFFSET 0x1000   /* offset for 1st semaphore handle! */
                                   /* this ensures NULL handle is error */

#define THREADSTATUS_SUSPEND    1         /* thread is currently suspended */
#define THREADSTATUS_WAITSEM    2     /* thread is waiting for a semaphore */
#define THREADSTATUS_WAITTHREAD 4  /* thread is waiting for another thread */
#define THREADSTATUS_SLEEPWAIT  8   /* thread is waiting for timer message */

#define THREADSTATUS_WAITING (1 | 2 | 4 | 8) /* combination of the above 4 */

#define THREADSTATUS_SLEEP      0x1000             /* thread is 'sleeping' */

#define THREADSTATUS_CRITICAL   0x2000   /* 'LoopDispatch()' returns FALSE */
                                         /* whether thread is being killed */
                                         /* or not.                        */

#define THREADSTATUS_EXCLUSIVE  0x4000   /* thread runs exclusive.  The */
                                         /* 'LoopDispatch()' is disabled. */
                                         /* overriden by 'WAITING' flags. */

#define THREADSTATUS_KILL       0x8000       /* thread is to be terminated */
                                                    /* on next 'go around' */




                             /*** MACROS ***/

#define __STACK _segname("_STACK")


#define HEX_DIGIT(X) (((X)>='0'&&(X)<='9')?((X)-'0'):\
                      (((X)>='A'&&(X)<='F')?((X)-'A'+10):\
                      (((X)>='a'&&(X)<='f')?((X)-'a'+10):-1)))



                      /*** STRUCTURE DEFINITIONS ***/

typedef struct tagHookMsg {           /* structure is used by WINDOWS HOOK */
  WORD hlParam,llParam,wParam,wMsg;   /* function 'MthreadAppMsgHookProc()'*/
  HWND hWnd;                          /* pointed to by 'lParam' on entry.  */
  } HOOKMSG;
typedef HOOKMSG FAR *LPHOOKMSG;


typedef struct tagInternalThread {
  char zeros[0x10];       /* contains zeros - 1st 16 bytes in memory block */

                             /**********************************************/
  WORD DS;                   /** the DEFAULT DS (DGROUP) for this task    **/
                             /** THIS MUST BE AT OFFSET 10H IN THE STACK! **/
                             /** ?libcewm.lib contains startup code that  **/
                             /** ensures that DGROUP:10H ALSO contains    **/
                             /** the default DS (DGROUP) value!.          **/
                             /** This is needed for MFC to work!          **/
                             /**********************************************/

  char ID[14];                                /* contains "INTERNALTHREAD" */

  HGLOBAL hSelf;       /* identifies it as an 'internal thread' descriptor */
  HANDLE hWaitThread;        /* handle of a thread for which it is waiting */
  LPTHREADPROC lpThreadProc;       /* Non-NULL causes 'FreeProcInstance()' */
              /* later I can inject code here manually and re-reference it */
              /* as a code segment area (proc is exported - inst required) */

  DWORD ESP;                                      /* stack pointer storage */
  WORD  SS;                                       /* stack segment storage */

  DWORD ESPterminate;    // SS:ESP and CS:EIP values for termination due to
  WORD  SSterminate;     // GP fault, STACK fault, or invalid PAGE fault
  DWORD EIPterminate;    // (similar to throw/catch buffer).
  WORD  CSterminate;

  void HUGE *hpInstanceData;     /* huge pointer to THREAD INSTANCE DATA */

  WORD wStatus;        /* this determines if I get executed or not in loop */
                       /* (process could be waiting for something!!)       */

  HWND hThreadWindow;  /* if there is a 'THREAD WINDOW' handle goes here!  */

  WORD wPri;  /* this is the current thread-priority of the current thread */
  char curdir[128];    /* this is the current directory for this thread.   */

  } INTERNALTHREAD, FAR *LPINTERNALTHREAD;


typedef TASKENTRY FAR *LPTASKENTRY;
typedef MEMMANINFO FAR *LPMEMMANINFO;


typedef struct tagNEWTHREADINFO {
  LPTHREADPROC lpThreadProc;
  HANDLE hCaller, hArgs;
  WORD wFlags;
  WORD wNewSP, wNewSS;   /* points to top of new 'stack' area (16 bit) */
  HANDLE hThreadID;      /* handle to 'INTERNALTHREAD' structure */
                         /* created by caller to LoopDispatch2() */
  } NEWTHREADINFO;

typedef NEWTHREADINFO FAR *LPNEWTHREADINFO;



                         /*************************/
                         /** FUNCTION PROTOTYPES **/
                         /*************************/

void FAR PASCAL InitProcs(void); /* performs initialization normally */
                                 /* handled by 'LibMain()'.          */
static void _far _pascal SetCPUFlags(void);


BOOL API MyGetQueueStatus(void);

static BOOL LOADDS CALLBACK MthreadGetMessage(LPMSG msg);  /* for now... */

BOOL NEAR PASCAL UseExternalLibrary(HINSTANCE hInstance, HANDLE hPrev,
                              LPSTR lpCmd, WORD nCmdShow, BOOL FAR *lpRval);

BOOL NEAR PASCAL NearThreadDispatch(LPMSG msg, LPNEWTHREADINFO lpNewThread,
                                    BOOL wYieldFlag);
BFPPROC LOADDS MthreadNewThreadDispatch(LPMSG msg, LPNEWTHREADINFO lpNewThread);

BOOL NEAR PASCAL ThisTaskHasMDIAccelerators();
BOOL NEAR PASCAL TranslateMDITaskAccel(LPMSG msg);
BOOL NEAR PASCAL TranslateDialogTaskMessage(LPMSG msg);


static BOOL FAR PASCAL MyTranslate(LPMSG msg);

BOOL _EXPORT_ FAR PASCAL MthreadAppMessageProc(LPMSG msg/* , int current_task_index */);

static BFPPROC SemaphoreCallback(HANDLE hSemaphore, BOOL SetFlag);

BFPPROC _EXPORT_ LOADDS MthreadSemaphoreCallback(HANDLE hSemaphore,
                                               SEMAPHORECALLBACK lpProc);

void NEAR PASCAL InternalThreadExit(HANDLE hThreadID, LONG rval);

LONG NEAR PASCAL CallThreadProc(LPTHREADPROC lpThreadProc,HANDLE hCaller,
                                HANDLE hArgs, WORD wFlags, HINSTANCE hInst,
                                HANDLE hThreadID);



                   /** TOOLHELP 'CLONE' FUNCTIONS **/

extern "C"
{

BOOL FAR PASCAL MyMemManInfo(LPMEMMANINFO lpInfo);
BOOL FAR PASCAL MyTaskFirst(LPTASKENTRY lpTask);
BOOL FAR PASCAL MyTaskNext(LPTASKENTRY lpTask);
BOOL FAR PASCAL MyIsTask(HTASK hTask);
HTASK FAR PASCAL MyTaskFindHandle(LPTASKENTRY lpTask, HTASK hTask);
BOOL FAR PASCAL IsWINOLDAP(HTASK hTask);
BOOL FAR PASCAL IsInternalThread(HANDLE hThread);

}

//void NEAR LOADDS Int1C_Utility(void);

BOOL EXPORT API LOADDS DispatchWithinHookTimerProc(HWND hWnd, WORD wMsg,
                                                   WPARAM wParam, LPARAM lParam);

BOOL NEAR PASCAL ThisTaskHasMDIAccelerators(void);
BOOL NEAR PASCAL TranslateMDITaskAccel(LPMSG msg);



/***************************************************************************/
/*       OTHER EXPORT FUNCTIONS - SHOULD BE DECLARED IN '.DEF' FILE        */
/***************************************************************************/

LRESULT EXPORT LOADDS CALLBACK MthreadWndProc(HWND hWnd, unsigned msg,
                                                WPARAM wParam, LPARAM lParam);
                           /* Multi-Thread 'Main' window callback function */

BOOL EXPORT LOADDS CALLBACK MthreadEnumFunction(HWND hWnd, LPARAM lParam);
                                         /* generic 'enumeration' function */

BOOL EXPORT LOADDS CALLBACK MthreadIDDlgProc(HWND hDlg, unsigned msg,
                                               WPARAM wParam, LPARAM lParam);
                                  /* 'ID' Dialog proc for 'startup' banner */

LRESULT EXPORT LOADDS CALLBACK MthreadAppMsgHookProc(int nCode, WPARAM wParam,
                                                      LPARAM lParam);
                                   /* 'Hook' proc for Application Messages */


              /**********************************************/
              /** Undocumented WINDOWS function prototypes **/
              /**********************************************/

_segment FAR PASCAL GetTaskDS(WORD zero);   /* DOCUMENTED IN DDK */

VOID FAR PASCAL DOS3Call(void); /* documented, but not in header file */

DWORD FAR PASCAL GlobalHandleNoRIP(WORD wSeg);
              /* UNDOC'ed, confirmed by M/S to be what it says it is */
              /* not required in Windows 3.1 - use GlobalHandle()    */



                            /* GLOBAL VARIABLES */

         /* these variables are initialized such that if they are */
         /* used without being properly assigned there can be no  */
         /* harm done to the program or data files as a result.   */

extern "C"
{

BOOL _ok_to_use_386_instructions_ = FALSE;  /* set to TRUE if >286 */
BOOL _windows_in_real_mode_       = TRUE;   /* set to TRUE if in real mode */
                                            /* (most limiting is default) */
BOOL _windows_in_386_mode_        = FALSE;  /* set to TRUE if in '386 mode */
WORD _windows_version_            = 0;      /* which version of windows? */

}


                      /*****************************/
                      /*** MACRO 'REDEFINITIONS' ***/
                      /*****************************/

//#define CompactProc (pCompactProc[current_task_index])
//#define AppMessageProc (pAppMessageProc[current_task_index])

//#define Accelerators (pAccelerators[current_task_index])
//#define MDIAcceleratorsEnable (pMDIAcceleratorsEnable[current_task_index])
//#define TranslateEnable (pTranslateEnable[current_task_index])
//#define MDIAcceleratorsClient (pMDIAcceleratorsClient[current_task_index])

//#define ThreadParms (pThreadParms[current_task_index])


                    /*******************************/
                    /*** MACRO 'NEW DEFINITIONS' ***/
                    /*******************************/

//#define TASK_TABLE_ENTRY(h) ((((WORD)h)>>4)%MAX_MTHREAD_APPS) /* hash table entry point */

//#define NO_TASK_ENTRY ((WORD)current_task_index==(WORD)BAD_ENTRY)
#define NO_TASK_ENTRY FALSE  /* not required for 'stand-alone' version */


//#define NEXT_TASK_TABLE_ENTRY(X,Y) _asm {    /* finds next empty entry */  \
//      _asm mov bx, OFFSET TaskHashTable      /* in the 'TaskHashTable' */  \
//      _asm mov cx, MAX_MTHREAD_APPS               /* and places pointer into*/  \
//      _asm cmp WORD PTR [bx], 0              /* 'x', and index into 'y'*/  \
//      _asm jz $+8                                                          \
//      _asm add bx, sizeof(TASK_HASH_TABLE)                                 \
//      _asm loop $-10                                                       \
//      _asm mov bx, 0                                                       \
//      _asm mov WORD PTR (X), bx                                            \
//      _asm mov ax,bx                                                       \
//      _asm xor dx,dx                                                       \
//      _asm mov bx, sizeof(TASK_HASH_TABLE);                                \
//      _asm div bx                                                          \
//      _asm mov (Y),ax    }

//#define SET_CURRENT_TASK_INDEX \
//                 current_task_index = GetTaskTableEntry(GetCurrentTask())

#define SET_CURRENT_TASK_INDEX  /* not required for 'stand-alone' version */


                      /****************************/
                      /** DLL 'UNIQUE' VARIABLES **/
                      /****************************/



static void (FAR PASCAL *CompactProc)(void)           = NULL; // {(void (FAR PASCAL *)())NULL};
static void (FAR PASCAL *AppMessageProc)(LPMSG lpmsg) = NULL; // { (void (FAR PASCAL *)())NULL};

static DWORD FAR *Accelerators                           = NULL;
static WORD MDIAcceleratorsEnable                        =0;
static BOOL TranslateEnable                              =FALSE;
static HWND FAR *MDIAcceleratorsClient                   ={NULL};
static HWND FAR *DialogWindowList                        ={NULL};


static HANDLE hLibInst;
static WORD cbLibHeapSize, wLibDataSeg;

static UINT wTimer=NULL;

union {
    DWORD dw;
    struct {
        BYTE win_major, win_minor;
        BYTE dos_minor, dos_major;
        } s;
    } Version;

                        /***********************/
                        /** Function Pointers **/
                        /***********************/

static WORD (FAR PASCAL *lpGlobalMasterHandle)(void);
static DWORD (FAR PASCAL *lpGlobalHandleNoRIP)(WORD wSeg);

static HHOOK (FAR PASCAL *lpSetWindowsHookEx)(int, HOOKPROC, HINSTANCE, HTASK);
static LRESULT (FAR PASCAL *lpCallNextHookEx)(HHOOK, int, WPARAM, LPARAM);
static BOOL (FAR PASCAL *lpUnhookWindowsHookEx)(HHOOK);

static BOOL (FAR PASCAL *lpMemManInfo)(LPMEMMANINFO lpInfo);
static BOOL (FAR PASCAL *lpTaskFirst)(LPTASKENTRY lpTask);
static BOOL (FAR PASCAL *lpTaskNext)(LPTASKENTRY lpTask);
static BOOL (FAR PASCAL *lpIsTask)(HTASK hTask);
static HTASK (FAR PASCAL *lpTaskFindHandle)(LPTASKENTRY lpTask, HTASK hTask);

static HMODULE hToolHelp, hKernel, hUser, hWMTHREAD=NULL;
static WORD wIGROUP;

static void (_interrupt _far *Old_Int1C)();

static TIMERPROC lpDispatchWithinHookTimerProc = NULL;

    /** The items normally found in 'TaskHashTable' **/

    HTASK hTask;                  /* GetCurrentTask() is VERY VERY FAST! */
    HINSTANCE _hInst;             /* the instance handle for this task   */
    HANDLE hThreadList;           /* handle of 'threadlist' owned by it  */
    HANDLE hInternalThreadList;   /* handle of 'internal threadlist'     */
    WORD   wInternalThreadCount;  /* number of 'internal threads' in use */
    WORD   wInternalThreadIndex;  /* index of current 'internal thread'  */
    WORD   wNextInternalThread;   /* index of 'next internal thread'     */
    BOOL isthread_switch;         /* 'is a thread' boolean               */
    LONG mtEnumRval;              /* return value from enumeration fn.   */
    BOOL mtExitStatus;            /* exit status from enumeration fn.    */
    BOOL mtIdleMessage;           /* TRUE if 'Idle' message is active    */
    int  was_quit;                /* 'was_quit' flag from MessageLoop()  */
    WORD disable_messages;        /* 'disable_messages' flag - default 0 */
    BOOL disable_dispatch;        /* 'disable_dispatch' flag - default 0 */
    BOOL MarkTaskForDeletion;     /* 'MarkTaskForDeletion' - causes the  */
                                  /* task entry to be deleted when down  */
                                  /* to a single running thread.         */

    HHOOK    lpfnNextHook;        /* updated type 'HHOOK' vice 'HOOKPROC' */
    HOOKPROC lpfnHookProc;        /* proc instance of 'hook proc'.       */



BOOL MthreadExitStatus=FALSE; /* not part of 'TaskHashTable'... */









/***************************************************************************/
/*                    INITIALIZATION PROCS GO HERE!                        */
/***************************************************************************/


#pragma code_seg ("MTHREAD_INIT_TEXT","CODE")


BFPPROC MthreadInit(HINSTANCE hInstance, HANDLE hPrev, LPSTR lpCmd,
                    WORD nCmdShow)
{    /* first thing application does before anything else is call this! */
//LPSTR p1, p2;
//HANDLE hMem;
//NPSTR p3;
//HWND hWnd;
//int i, h;
//static char name[32];
LPCSTR lp1;
static const char CODE_BASED szWMTHREAD[]="WMTHREAD.DLL";
OFSTRUCT ofstr;



      // if command line begins with '/X', FORCE usage of 'external library'


      for(lp1=lpCmd; lp1 && *lp1 && *lp1<=' '; lp1++)
         ; // find first non-white-space in command line

      if((lp1 && *lp1=='/' && (lp1[1]=='x' || lp1[1]=='X') && lp1[2]<=' ')
         || GetProfileInt("SFTSHELL","UseExternalLibrary",0))
      {
         hWMTHREAD = GetModuleHandle(szWMTHREAD);

         if(!hWMTHREAD)
         {
            if(OpenFile(szWMTHREAD, &ofstr, OF_READ | OF_EXIST | OF_SHARE_COMPAT)
               != HFILE_ERROR)
            {
               hWMTHREAD = (HMODULE)0xffff;
            }
         }

         if(hWMTHREAD)
         {
            hWMTHREAD = LoadLibrary(szWMTHREAD);

            if((WORD)hWMTHREAD < 32)
            {
               hWMTHREAD = NULL;
            }
         }
      }
      else
      {
         hWMTHREAD = NULL;
      }


      if(hWMTHREAD)
      {
       BOOL rval;

         if(!UseExternalLibrary(hInstance, hPrev, lpCmd, nCmdShow, &rval))
         {
            return(rval);
         }
      }



      InitProcs();  /* does what 'LibMain()' normally does...  */

//    static BOOL not_yet_display_logo = TRUE;

//      if(AddTaskTableEntry(GetCurrentTask(), hInst, FALSE))
//      {                                            /* add to task table!!! */
//         return(-1);                /* error!  can't add for some reason.. */
//      }

      /* a special section to replace 'AddTaskTableEntry()' (above) */

      hTask                = GetCurrentTask();
      _hInst               = hInstance;
//      hThreadList          = 0;
      isthread_switch      = FALSE;
      mtEnumRval           = 0L;
      mtExitStatus         = FALSE;
      mtIdleMessage        = FALSE;

      hInternalThreadList  = NULL;
      wInternalThreadCount = 0;
      wInternalThreadIndex = 0;

      MarkTaskForDeletion  = FALSE;

      was_quit             = FALSE;
      disable_messages     = FALSE;
      disable_dispatch     = DISABLE_DISPATCH_NONE;

      TranslateEnable = TRUE;

//      if(not_yet_display_logo)     /* Only display this once! */
//      {
//         not_yet_display_logo = FALSE;
//
//         if(!display_wdbutil_logo)
//         {
//            DialogBox(hLibInst,"ABOUTMTHREAD",NULL,MthreadIDDlgProc);
//         }
//         else
//         {
//            DialogBox(hLibInst,"ABOUTDBUTIL",NULL,MthreadIDDlgProc);
//         }
//      }



//      RootApp.hInst       = hInst;
//      RootApp.hTask       = GetCurrentTask();
//      RootApp.hWnd        = (HWND)NULL;     /* initial value for 'hWnd' */
                                            /* used as parent for others! */
//      ThreadParms.hWnd    = (HWND)NULL;     /* this says 'I am a main task' */
//      ThreadParms.hCaller = (HANDLE)NULL;
//      ThreadParms.hArgs   = (HANDLE)NULL;
//      ThreadParms.hPrev   = (HANDLE)NULL;
//      ThreadParms.hInst   = hInst;
//      ThreadParms.flags   = 0;
//      *ThreadParms.fName  = '\0';

//      hThreadList         = NULL;  /* initial value */
//
//                  /** Allocate memory for thread table **/
//
//      if((hMem = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT | GMEM_DDESHARE,
//                              sizeof(HANDLE)*MAX_MTHREAD_APPS))==(HANDLE)NULL)
//      {
//          DelTaskTableEntry(GetCurrentTask());
//          return(-1);    /* error!  couldn't allocate mem for thread table */
//      }
//
//      hThreadList = hMem;

                    /*****************************************/
                    /*** INSTALLING WINDOWS HOOK PROCEDURE ***/
                    /*****************************************/

      (FARPROC &)lpfnHookProc = MakeProcInstance((FARPROC)MthreadAppMsgHookProc, _hInst);

      if(lpSetWindowsHookEx)
      {
         lpfnNextHook = lpSetWindowsHookEx(WH_MSGFILTER, lpfnHookProc,
                                           _hInst, GetCurrentTask());
      }
      else
      {
         lpfnNextHook = SetWindowsHook(WH_MSGFILTER,lpfnHookProc);
      }


          /* in the event of an error the hook is not installed. */
          /* the program will continue despite the above problem */




   return(FALSE);                          /* this says 'I am a main task' */

}


void FAR PASCAL InitProcs(void)
{
int i;
OFSTRUCT ofstr;   /* used for 'Toolhelp' load (bug fix for 3.0) */
#ifdef OLD_DISPATCH_WITHIN_HOOK
DWORD dwIntProc;
#endif



         /********************************************************/
         /** Get Windows Version and Library Function Addresses **/
         /********************************************************/

   Version.dw = GetVersion();

   hKernel = GetModuleHandle("KERNEL");
   hUser   = GetModuleHandle("USER");

   (FARPROC &)lpGlobalMasterHandle = GetProcAddress(hKernel,"GlobalMasterHandle");
   (FARPROC &)lpGlobalHandleNoRIP  = GetProcAddress(hKernel,"GlobalHandleNoRIP");
   wIGROUP = SELECTOROF(lpGlobalMasterHandle);

   if(GetModuleHandle("TOOLHELP") ||
      OpenFile("TOOLHELP.DLL",&ofstr,OF_EXIST|OF_SHARE_DENY_NONE)!=(HFILE)-1)
   {
      hToolHelp = LoadLibrary("TOOLHELP.DLL");
   }
   else
   {
      hToolHelp = NULL;
   }

   if((WORD)hToolHelp>=32)
   {
      (FARPROC &)lpTaskFindHandle = GetProcAddress(hToolHelp,"TaskFindHandle");
      (FARPROC &)lpTaskFirst      = GetProcAddress(hToolHelp,"TaskFirst");
      (FARPROC &)lpTaskNext       = GetProcAddress(hToolHelp,"TaskNext");
      (FARPROC &)lpMemManInfo     = GetProcAddress(hToolHelp,"MemManInfo");

      if(!lpTaskFindHandle || !lpTaskFirst || !lpTaskNext
         || !lpMemManInfo)
      {

         FreeLibrary(hToolHelp);

         hToolHelp = (HMODULE)NULL;

      }

   }

   if((WORD)hToolHelp<32)
   {
      lpTaskFindHandle = MyTaskFindHandle;
      lpTaskFirst      = MyTaskFirst;
      lpTaskNext       = MyTaskNext;
      lpMemManInfo     = MyMemManInfo;

      hToolHelp = (HMODULE)NULL;
   }

   if(Version.s.win_major>3 ||
      (Version.s.win_major==3 && Version.s.win_minor>=10))
   {
          /* windows 3.1 or greater */
       (FARPROC &)lpIsTask = GetProcAddress(hKernel,"IsTask");

       (FARPROC &)lpSetWindowsHookEx =
                           GetProcAddress(hUser,"SetWindowsHookEx");
       (FARPROC &)lpCallNextHookEx =
                           GetProcAddress(hUser,"CallNextHookEx");
       (FARPROC &)lpUnhookWindowsHookEx =
                           GetProcAddress(hUser,"UnhookWindowsHookEx");
   }
   else
   {
       lpIsTask              = NULL;
       lpSetWindowsHookEx    = NULL;
       lpCallNextHookEx      = NULL;
       lpUnhookWindowsHookEx = NULL;
   }


   if(!lpIsTask)
   {
       lpIsTask = MyIsTask;           /* windows 3.0 or less, or error */
   }

   if(!lpSetWindowsHookEx || !lpCallNextHookEx || !lpUnhookWindowsHookEx)
   {
       lpSetWindowsHookEx    = NULL;  /* windows 3.0 or less, or error */
       lpCallNextHookEx      = NULL;
       lpUnhookWindowsHookEx = NULL;
   }


   TranslateEnable = TRUE;

//   for(i=0; i<MAX_MTHREAD_APPS; i++)
//   {
//     pTranslateEnable[i] = TRUE;  /* requires run-time initialization */
//   }

   SetCPUFlags();             /* initialize flags for CPU type */

}


static void _far _pascal SetCPUFlags(void)
{
DWORD winflags;

            /*** Using 'GetWinFlags' obtain info about session   ***/
            /*** and set up appropriate 'flag words' accordingly ***/

   winflags = GetWinFlags();      /* use this to obtain info about session */

   if(winflags & WF_PMODE)
   {
      _windows_in_real_mode_ = FALSE;
      if(winflags & WF_ENHANCED)
      {
         _windows_in_386_mode_ = TRUE;
      }
      else
      {
         _windows_in_386_mode_ = FALSE;
      }
   }
   else
   {
      _windows_in_real_mode_ = TRUE;
      _windows_in_386_mode_  = FALSE;
   }

   if(_windows_in_386_mode_ ||
      (!(winflags & WF_CPU086) && !(winflags & WF_CPU186) &&
       !(winflags & WF_CPU286)))
   {
      _ok_to_use_386_instructions_ = TRUE;  /* it's a 386,486,... */
   }
   else
   {
      _ok_to_use_386_instructions_ = FALSE; /* it's an 8086,80186,80286 */
   }

   _windows_version_ = (WORD)GetVersion();  /* for reference purposes - faster! */
}




/***************************************************************************/
/*                    THE BEGINNING OF 'API' PROCS                         */
/***************************************************************************/


#pragma code_seg ("MTHREAD_REGISTER","CODE")

void FAR PASCAL _RegisterCompactProc(FARPROC lpproc)
{
   SET_CURRENT_TASK_INDEX;
   if(NO_TASK_ENTRY) return;

   (void (FAR PASCAL *)())CompactProc = (void (FAR PASCAL *)())lpproc;
}

void FAR PASCAL _RegisterAppMessageProc(
                   void (FAR PASCAL *_AppMessageProc)(LPMSG lpmsg))
{
   SET_CURRENT_TASK_INDEX;
   if(NO_TASK_ENTRY) return;

   (void (FAR PASCAL *&)())AppMessageProc = (void (FAR PASCAL *)())_AppMessageProc;
}

BOOL FAR PASCAL _RegisterAccelerators(HANDLE hAccel, HWND hwnd)
{   /* registers an accellerator table for a window (-1 == all windows) */
int i;
DWORD dwAccelEntry;


   SET_CURRENT_TASK_INDEX;
   if(NO_TASK_ENTRY) return(TRUE);

   dwAccelEntry = MAKELONG((WORD)hAccel,(WORD)hwnd);

   if(Accelerators==NULL)
   {
      Accelerators = (DWORD FAR *)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT, // | GMEM_DDESHARE,
                                                 sizeof(DWORD) * MAX_ACCEL);

      if(!Accelerators) return(TRUE);
   }

   for(i=0; i<MAX_ACCEL && Accelerators[i]!=dwAccelEntry; i++)
   {
      if(Accelerators[i]==(DWORD)NULL)
      {
         Accelerators[i] = dwAccelEntry;
         break;
      }
   }
   if(i>=MAX_ACCEL)
      return(TRUE);     /* error!  no room for accel table! */
   else
      return(FALSE);    /* Handle added (or else already in table!) */
}

BOOL FAR PASCAL _RemoveAccelerators(HANDLE hAccel, HWND hwnd)
{
int i, j;
DWORD dwAccelEntry;


   SET_CURRENT_TASK_INDEX;
   if(NO_TASK_ENTRY) return(TRUE);

   if(!Accelerators) return(TRUE);

   dwAccelEntry = MAKELONG((WORD)hAccel,(WORD)hwnd);

   for(i=0; i<MAX_ACCEL; i++)
   {
      if(Accelerators[i]==dwAccelEntry ||
         (!hwnd && LOWORD(Accelerators[i])==(WORD)hAccel))
      {
         for(j=i; j<(MAX_ACCEL-1); j++)
         {
            Accelerators[j] = Accelerators[j+1];  /* compact listing */
         }
         Accelerators[MAX_ACCEL-1] = (LONG)NULL;  /* flag it as unused */

         if(hwnd) break; /* repeat for NULL 'hwnd', but exit now otherwise */
      }
   }
   if(i>=MAX_ACCEL)
      return(TRUE);     /* error!  didn't find accel table! */
   else
      return(FALSE);    /* Handle removed */
}

void FAR PASCAL _EnableMDIAccelerators(HWND hMDIClient)
{
WORD w;

   SET_CURRENT_TASK_INDEX;
   if(NO_TASK_ENTRY) return;

   if(MDIAcceleratorsClient==NULL)
   {
      MDIAcceleratorsClient = (HWND FAR *)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                                         sizeof(HWND) * MAX_MDIACCEL);

      if(!MDIAcceleratorsClient) return;  /* an error, but... */
   }

   for(w=0; w<MDIAcceleratorsEnable && w<MAX_MDIACCEL; w++)
   {
      if(hMDIClient==MDIAcceleratorsClient[w])
         return;
   }

   if(w>=MAX_MDIACCEL) return;  /* no error code, but... can't do it! */

   MDIAcceleratorsClient[MDIAcceleratorsEnable++] = hMDIClient;
}

void FAR PASCAL _DisableMDIAccelerators(HWND hMDIClient)
{
WORD w1,w2;

   SET_CURRENT_TASK_INDEX;
   if(NO_TASK_ENTRY) return;

   if(!MDIAcceleratorsClient) return;

   for(w1=0; w1<MDIAcceleratorsEnable; w1++)
   {
      if(MDIAcceleratorsClient[w1]==hMDIClient)
         break;
   }
   for(w2=w1+1; w2<MDIAcceleratorsEnable; w2++)
   {
      MDIAcceleratorsClient[w2-1] = MDIAcceleratorsClient[w2];
   }

   if(w1<MDIAcceleratorsEnable)
   {
      MDIAcceleratorsClient[w2-1] = (HWND)NULL;
      MDIAcceleratorsEnable--;
   }

}


BFPPROC _RegisterDialogWindow(HWND hDlg)
{
WORD w;

   SET_CURRENT_TASK_INDEX;
   if(NO_TASK_ENTRY) return(TRUE);

   if(DialogWindowList==NULL)
   {
      DialogWindowList = (HWND FAR *)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                                    sizeof(HWND) * MAX_MDIACCEL);

      if(!DialogWindowList) return(TRUE);
   }

   for(w=0; DialogWindowList[w] && w<(MAX_MDIACCEL - 1); w++)
   {
      if(hDlg==DialogWindowList[w])
         return(FALSE);           // already there...
   }

   if(w>=(MAX_MDIACCEL - 1)) return(TRUE);  // return error

   DialogWindowList[w++] = hDlg;
   DialogWindowList[w] = NULL;   // ensure end is properly marked!


   return(FALSE);  // it worked!
}

VFPPROC _UnregisterDialogWindow(HWND hDlg)
{
WORD w1,w2;

   SET_CURRENT_TASK_INDEX;
   if(NO_TASK_ENTRY) return;

   if(!DialogWindowList) return;

   for(w1=0; DialogWindowList[w1]; w1++)
   {
      if(DialogWindowList[w1]==hDlg)
         break;
   }
   for(w2=w1+1; DialogWindowList[w2]; w2++)
   {
      DialogWindowList[w2-1] = DialogWindowList[w2];
   }

   DialogWindowList[w2-1] = (HWND)NULL;

}


void FAR PASCAL _EnableTranslateMessage(void)
{
   SET_CURRENT_TASK_INDEX;
   if(NO_TASK_ENTRY) return;

   TranslateEnable = TRUE;
}

void FAR PASCAL _DisableTranslateMessage(void)
{
   SET_CURRENT_TASK_INDEX;
   if(NO_TASK_ENTRY) return;

   TranslateEnable = FALSE;
}



/***************************************************************************/
/*                         TRANSLATE FUNCTIONS                             */
/***************************************************************************/


BOOL NEAR PASCAL ThisTaskHasMDIAccelerators(void)
{
   SET_CURRENT_TASK_INDEX;
   if(NO_TASK_ENTRY) return(FALSE);    /* no task entry, no accelerators! */

   if(MDIAcceleratorsClient && MDIAcceleratorsEnable>0)
      return(TRUE);
   else
      return(FALSE);
}


BOOL NEAR PASCAL TranslateMDITaskAccel(LPMSG msg)
{
WORD w;

   if(!MDIAcceleratorsClient || !MDIAcceleratorsEnable)
   {
      return(FALSE);  /* nothing there! */
   }

   for(w=0; w<MDIAcceleratorsEnable; w++)
   {
      if(TranslateMDISysAccel(MDIAcceleratorsClient[w],msg))
      {
         return(TRUE);
      }
   }
   return(FALSE);
}


BOOL NEAR PASCAL TranslateDialogTaskMessage(LPMSG msg)
{
WORD w;
HWND hWnd;

   /* when this function is called 'current_task_index' is correct */
   /* for the current task, so I don't need to 'set' it again!     */


   if(!TranslateEnable)
   {
      return(FALSE);        // do not translate messages!
   }

   // IF THIS WINDOW IS A DIALOG FRAME, OR ONE OF IT'S PROGENITORS IS
   // A DIALOG FRAME, THEN CALL 'IsDialogMessage()' TO TRANSLATE/DISPATCH!


   hWnd = msg->hwnd;

   while(hWnd && GetWindowLong(hWnd, GWL_STYLE) & WS_CHILD)
   {
      hWnd = GetParent(hWnd);   // get window's parent...
   }

   if(hWnd && HIWORD(GetWindowLong(hWnd, GWL_WNDPROC))==SELECTOROF(DefDlgProc)
       && GetClassWord(hWnd, GCW_HMODULE)==(WORD)hUser)
   {
      // this window is a valid 'dialog' window... I assume!  I included the
      // class's module handle equal to 'hUser' to ensure it's correct!!

      return(IsDialogMessage(hWnd, msg));
   }

   if(DialogWindowList)
   {
      for(w=0; DialogWindowList[w]; )
      {
         if(!IsWindow(DialogWindowList[w]))  // this is where I eliminate
         {                                   // windows that were destroyed
          WORD w2;

            for(w2=w + 1; DialogWindowList[w2]; w2++)
            {
               DialogWindowList[w2 - 1] = DialogWindowList[w2];
            }

            DialogWindowList[w2] = NULL;

            continue;     // go through the loop again (part of the tests)
         }

         if(IsDialogMessage(DialogWindowList[w], msg))
         {
            return(TRUE);
         }

         w++;    // increment is here so I can use 'continue' to bypass it
      }
   }

   return(FALSE);
}




static BOOL FAR PASCAL MyTranslate(LPMSG msg)
{
int i, iret;

   SET_CURRENT_TASK_INDEX;
   if(NO_TASK_ENTRY) return(FALSE);    /* no task entry, no translate! */


   if(TranslateDialogTaskMessage(msg))
   {
      return(TRUE);                   // this message was for a DIALOG BOX!
   }


   if(ThisTaskHasMDIAccelerators() &&    /* MDI Accel's enabled for task? */
      TranslateMDITaskAccel(msg))             /* was translated */
   {
      return(TRUE);
   }

   iret = FALSE;

   if(Accelerators)
   {
        /* go through 'Accelerators' list & translate as appropriate */

      for(i=0; !iret && i<MAX_ACCEL && Accelerators[i]; i++)
      {
                 /* first NULL entry in 'Accelerators' marks end of list */

                                 /* translate Accelerators */
         iret = TranslateAccelerator((HWND)HIWORD(Accelerators[i]),
                                     (HACCEL)LOWORD(Accelerators[i]),
                                      msg);

      }

   }

           /* if no translation (yet) & Translation is Enabled */

   if(!iret && TranslateEnable)
   {
      TranslateMessage(msg);  /* must return FALSE so I still dispatch it */
   }

   return(iret);                /* if no translation occurred - onward! */

}



#pragma code_seg ("MYDISPATCH_TEXT","CODE")

BFPPROC _ThreadComplete(LONG rval)
{
HTASK hTask;
HWND hMaster;
HANDLE hThread;


   SET_CURRENT_TASK_INDEX;

//   hMaster = GetMasterThreadWindow();
   hTask = GetCurrentTask();

   if(!hInternalThreadList || !wInternalThreadIndex)/* not internal thread */
   {
      hThread = NULL;
   }
   else
   {
      hThread = MthreadGetCurrentThread();
   }

   if(hThread)  /* I am an 'internal thread', o.k. dude? */
   {
      PostAppMessage(GetCurrentTask(), THREAD_COMPLETE, (WPARAM)hThread,
                     (LPARAM)rval);

      if(hMaster && IsWindow(hMaster))
      {
         PostMessage(hMaster, THREAD_COMPLETE, (WPARAM)hThread,
                     (LPARAM)rval);

      }

      return(FALSE);
   }


       /* This section below *ONLY* applies for EXTERNAL THREADS! */
       /* (that is, unless some sort of error happens... )        */


//   if(RootApp.hInst!=(HANDLE)NULL &&  /* the master thread is still there! */
//      (hMaster==(HWND)NULL || IsWindow(hMaster)))
//   {
//      if(lpIsTask(ThreadParms.hCaller))
//      {
//         PostAppMessage(ThreadParms.hCaller, THREAD_COMPLETE,
//                        (WPARAM)hTask, (LPARAM)rval);
//
//      }
//
//      SET_CURRENT_TASK_INDEX;
//      DeleteThreadProp(RootApp.hTask, hTask);
//                                      /* remove task from list for root app */
//
//      if(lpIsTask(RootApp.hTask))
//      {
//         PostAppMessage(RootApp.hTask, THREAD_COMPLETE, (WPARAM)hTask,
//                           (LPARAM)rval);
//      }
//   }


   MthreadExit(NULL);          /* destroy all windows for this task!!!!! */
                               /* and that includes the task entry and any */
                               /* internal threads associated with it.     */

   return(FALSE);
}




#pragma code_seg()


HINSTANCE FAR PASCAL _GetCurrentInstance()  /* special version for stand-alone */
{
   return(_hInst);   /* the global variable, of course */
}


HFPPROC _MthreadGetCurrentThread(void)
{
HANDLE FAR *lpHandle;
HANDLE hRval;

   SET_CURRENT_TASK_INDEX;
   if(NO_TASK_ENTRY) return(NULL);

     /** Thread #0 (the message loop) is not considered a thread **/

   if(!hInternalThreadList || !wInternalThreadIndex ||
      !(lpHandle = (HANDLE FAR *)GlobalLock(hInternalThreadList)))
   {
      return(NULL);       /* not a thread, so don't return anything */
   }

   hRval = lpHandle[wInternalThreadIndex];
   GlobalUnlock(hInternalThreadList);

   return(hRval);              /* the current thread ID handle */
}


WFPPROC _GetNumberOfThreads(void)
{
WORD wRval;

   SET_CURRENT_TASK_INDEX;
   if(NO_TASK_ENTRY) return(NULL);

   if(!hInternalThreadList || wInternalThreadCount<=1)
   {
      return(NULL);                  /* no threads, so return a NULL */
   }

   wRval = wInternalThreadCount - 1;

   return(wRval);            /* the current number of threads besides root */
}



BFPPROC _IsThread(void)       /* returns TRUE if task is an external thread */
{                            /* or called from within an 'internal' thread */
   SET_CURRENT_TASK_INDEX;
   if(NO_TASK_ENTRY) return(FALSE);

   if(hInternalThreadList)
   {
      return(wInternalThreadIndex != 0); /* TRUE means 'use LoopDispatch()'*/
   }

   return(isthread_switch);
}


BFPPROC _IsThreadHandle(HANDLE hThread)
{
   if(!hThread) return(FALSE);  /* always FALSE if handle is NULL */

              /* for now only check INTERNAL threads! */

   if(IsInternalThread(hThread)) return(TRUE);  /* YEP! It is!! */

       /* here is where I *would* check for EXTERNAL threads */

   return(FALSE);

}


BOOL FAR PASCAL _IsHandleValid(HANDLE hItem)
{

   if(Version.s.win_major>3 ||
      (Version.s.win_major==3 && Version.s.win_minor>=10))
   {
      return((DWORD)GlobalHandle((WORD)hItem)!=(DWORD)NULL);
   }
   else
   {
      return((DWORD)lpGlobalHandleNoRIP((WORD)hItem)!=(DWORD)NULL);
   }
}


BOOL FAR PASCAL IsInternalThread(HANDLE hThread)
{
BOOL rval = FALSE;
LPINTERNALTHREAD lpIT;


   SET_CURRENT_TASK_INDEX;
   if(NO_TASK_ENTRY) return(FALSE);      /* this shuts it down on error! */

   if(IsHandleValid(hThread))
   {
      if(GlobalSize(hThread)>=sizeof(INTERNALTHREAD))
      {
         lpIT = (LPINTERNALTHREAD)GlobalLock(hThread);
         if(lpIT)
         {
            rval = _fmemcmp("INTERNALTHREAD",lpIT->ID,sizeof(lpIT->ID))==0
                   && hThread == lpIT->hSelf;
            GlobalUnlock(hThread);
         }
      }
   }

   return(rval);
}





WFPPROC _SpawnThread(LPTHREADPROC lpProcName, HANDLE hArgs, WORD flags)
{

   /** For now, assume a stack size of 4k - later options will allow **/
   /** specifying different sizes or the actual stack space.         **/

#ifdef DISPATCH_WITHIN_HOOK

   if(!wTimer)
   {
      (FARPROC &)lpDispatchWithinHookTimerProc =
           MakeProcInstance((FARPROC)DispatchWithinHookTimerProc, _hInst);

      wTimer = SetTimer(NULL, MTH_TIMER_ID, 100, lpDispatchWithinHookTimerProc);
   }

#endif

   if(((flags & THREADSTYLE_EXTERNAL) && (flags & THREADSTYLE_INTERNAL)) ||
      ((flags & THREADSTYLE_EXPORTNAME) && (flags & THREADSTYLE_FARPROC)))
   {
      return((WORD)-1);  /* parm checking - conflicting flags! */
   }

   if((flags & THREADSTYLE_EXTERNAL)                     ||
      (!(flags & THREADSTYLE_INTERNAL)
       && ( !IsCodeSelector(SELECTOROF(lpProcName))
            || (flags & THREADSTYLE_EXPORTNAME) ) ) )
   {          /* disable 'EXTERNAL' spawning here */
      return((WORD)-1L/*SpawnExternal((LPSTR)lpProcName, hArgs, flags)*/);
   }

   return(SpawnThreadX(lpProcName, hArgs, flags, NULL, THREAD_STACK));
}


WFPPROC _SpawnThreadX(LPTHREADPROC lpProcName, HANDLE hArgs, WORD flags,
                      LPSTR lpThreadStack, WORD wStackSize)
{
MSG msg;
NEWTHREADINFO NewThread;
LPINTERNALTHREAD lpIT;
FARPROC threadproc, threadinst;
LPSTR lpDot;
HANDLE FAR *lpHandle;
WORD wDS;


   SET_CURRENT_TASK_INDEX;
   if(NO_TASK_ENTRY) return((WORD)-1);

   NewThread.hArgs = hArgs;
   NewThread.wFlags = flags;

   if(!hInternalThreadList || wInternalThreadIndex==0)
   {
      NewThread.hCaller = GetCurrentTask();  /** Task Handle of Caller **/
   }
   else
   {
      lpHandle = (HANDLE FAR *)GlobalLock(hInternalThreadList);

      NewThread.hCaller = lpHandle[wInternalThreadIndex];

      GlobalUnlock(hInternalThreadList);
   }


   if(!lpThreadStack)  /* need to create my own stack space */
   {
      NewThread.hThreadID = GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT,
                                        sizeof(INTERNALTHREAD) + wStackSize);

//// ** PAGE LOCK STACK ** 7/16/98
//      GlobalPageLock(NewThread.hThreadID);
//// ** PAGE LOCK STACK ** 7/16/98
   }
   else
   {
      NewThread.hThreadID = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                        sizeof(INTERNALTHREAD));
   }

   if(!NewThread.hThreadID ||              /* error creating thread? */
      !(lpIT = (LPINTERNALTHREAD)GlobalLock(NewThread.hThreadID)))
   {
      GlobalFree(NewThread.hThreadID);
      return((WORD)-1);
   }

   if(!lpThreadStack)  /* create my own stack space */
   {
      NewThread.wNewSS = SELECTOROF(lpIT);
                                       /* use internal thread id's segment */
      NewThread.wNewSP = sizeof(INTERNALTHREAD) + wStackSize - 4;
                                     /* point 2 words prior to end of stack */
   }
   else
   {
      NewThread.wNewSS = SELECTOROF(lpThreadStack);
      NewThread.wNewSP = wStackSize - 4;

      _fmemset(lpThreadStack, 0, wStackSize);  /* fill with zeros */
   }

         /** at this time it is assumed that a 'thread' procedure is **/
         /** exported from a task module (not a DLL) and thusly must **/
         /** be 'MakeProcInstance()'d before it can be called.       **/


   if(IsCodeSelector(SELECTOROF(lpProcName)))
   {
      (LPSTR &)threadproc = (LPSTR)lpProcName;
      lpIT->lpThreadProc = NULL;           /* no 'FreeProcInstance' call */
      (FARPROC &)NewThread.lpThreadProc = threadproc;
   }

     /*** This section of code taken from 'MthreadDispatch()' (above) ***/
     /*** and only applies when the proc name is passed (not address) ***/

   else if(!(lpDot = _fstrchr((LPSTR)lpProcName,'.')))
   {
            /* no '.' in proc name - use current module! */

      threadproc = GetProcAddress(NULL,(LPSTR)lpProcName);

   }
   else
   {

      *lpDot = 0;
      threadproc = GetProcAddress(GetModuleHandle((LPSTR)lpProcName),
                                  lpDot + 1);
      *lpDot = '.';      /* restore back to original status */

   }
     /* gets proc address for proc name passed on command line */

   if(threadproc && (LPSTR)threadproc!=(LPSTR)lpProcName) /* it's a string! */
   {
      if(!lpDot)
      {
       _segment old_seg, new_seg;

           /* MAKE A PROCEDURE INSTANCE OF THIS THING & CALL IT */

        LockData(0);               /* lock current data segment */
        _asm mov old_seg, ds       /* save current 'dll' data segment */
        new_seg = (_segment)HIWORD(GlobalLock(_hInst));
        _asm mov ds, new_seg       /* get data segment of application */


        threadinst=MakeProcInstance(threadproc,_hInst);

        _asm mov ds, old_seg       /* restore original 'dll' data segment */

        GlobalUnlock(_hInst);       /* unlock data segments */
        UnlockData(0);             /* unlock my data segment too */

        if(threadinst)
        {
           (FARPROC &)NewThread.lpThreadProc = threadinst;
           (FARPROC &)lpIT->lpThreadProc     = threadinst;
        }
        else
        {
           threadproc = NULL;     /* flags error below */
        }

      }
      else
      {
         lpIT->lpThreadProc = NULL;         /* no 'FreeProcInstance' call */
         (FARPROC &)NewThread.lpThreadProc = threadproc;
      }

   }

   if(!threadproc)
   {
      if(GlobalFlags(NewThread.hThreadID)&GMEM_LOCKCOUNT)
         GlobalUnlock(NewThread.hThreadID);

      GlobalFree(NewThread.hThreadID);
      return((WORD)-1);
   }


              /* assign values to identify internal thread */

   _fmemcpy(lpIT->ID, "INTERNALTHREAD", 14);     /* 14 character string */
   _fmemset(lpIT->zeros, 0, sizeof(lpIT->zeros));  /* 1st 16 bytes NULL */

   _asm mov wDS, ds  // added 9/1/97 to conform with WMTHREAD.DLL
   lpIT->DS = wDS;   // this segment is DGROUP, auto-loaded on errors, etc.
                     // by the C Startup library '_GetDGROUP()'

   lpIT->hSelf = NewThread.hThreadID;              /* handle to itself */

        /* in case thread exits right away, set up stack segment */

   lpIT->SS = NewThread.wNewSS;
   lpIT->ESP = NewThread.wNewSP;

   if(IsInternalThread(NewThread.hThreadID))
   {
      PostAppMessage(GetCurrentTask(), THREAD_REGISTER,
                     (WPARAM)(NewThread.hThreadID), 0L);
   }

          /* using message loop begin dispatching to new thread */
          /* notice that the memory remains locked, due to use  */
          /* of the same segment/selector for the stack.        */

   if(!MthreadNewThreadDispatch((LPMSG)&msg, (LPNEWTHREADINFO)&NewThread))
   {
      return((WORD)-1);   /* no error, but a QUIT message was received */
   }
   else
   {
          /* let's see if this thing has 'died' yet */

      if(IsInternalThread(NewThread.hThreadID))
      {
         /* the thread is alive!  Let application know about it */

         PostAppMessage(GetCurrentTask(), THREAD_ACKNOWLEDGE,
                        (WPARAM)(NewThread.hThreadID), 0L);

//         if(GetMasterThreadWindow())/* if there's a 'master thread window' */
//         {
//            PostMessage(GetMasterThreadWindow(), THREAD_ACKNOWLEDGE,
//                        (WPARAM)(NewThread.hThreadID), 0L);
//         }
      }

      if(flags & THREADSTYLE_RETURNHANDLE)
      {
         return((WORD)NewThread.hThreadID);   /* return handle of thread */
      }
      else
      {
         return(NULL);                   /* no error - okeedokee */
      }
   }

}




HTASK FAR PASCAL _GetInstanceTask(HINSTANCE hInst)
{
TASKENTRY te;

   _fmemset((LPSTR)&te, 0, sizeof(te));

   te.dwSize = sizeof(te); /* initializes structure */


   if(!lpTaskFirst((LPTASKENTRY)&te))
   {
      return(NULL);
   }

   do
   {
      if(te.hInst==hInst)
      {
         return(te.hTask);
      }
   }
   while(lpTaskNext((LPTASKENTRY)&te));

   return(NULL);  /* nope!  didn't find the thing!! */

}


static int IsProtectedMode = -1;  /* a '-1' indicates unchecked status */

WFPPROC _GetSegmentLimit(WORD wSeg) /* gets 'segment limit' (not via handle)*/
{                                  /* returns 'GlobalSize()' in real mode. */
   register WORD wRval;

   if(IsProtectedMode<0)
   {
      if(!_windows_in_real_mode_)
         IsProtectedMode = 1;                   /* protected mode for sure */
      else
      {
         if(GetWinFlags() & WF_PMODE)
         {
            IsProtectedMode = 1;
            _windows_in_real_mode_ = FALSE;
         }
         else
         {
            IsProtectedMode = 0;
         }
      }
   }

   if(!IsProtectedMode)
   {
      if(HIWORD(GlobalHandle(wSeg))!=0)
         return((WORD)GlobalSize((HANDLE)LOWORD(GlobalHandle(wSeg))));
      else
         return(NULL);
   }


   _asm _emit(0xf) _asm add ax,wSeg;   /* in reality it's 'LSL ax,[wSeg]' */
   _asm jz seg_ok;                     /* 'Z' is set if operation o.k. */
   _asm mov ax,0;                      /* '0' return value on error.   */

seg_ok:

   _asm mov wRval, ax

   return(wRval);
}


BFPPROC _IsSelectorValid(WORD wSeg) /* returns TRUE if selector is valid.   */
{
   register BOOL bRval;

   if(IsProtectedMode<0)
   {
      if(!_windows_in_real_mode_)
         IsProtectedMode = 1;                   /* protected mode for sure */
      else
      {
         if(GetWinFlags() & WF_PMODE)
         {
            IsProtectedMode = 1;
            _windows_in_real_mode_ = FALSE;
         }
         else
         {
            IsProtectedMode = 0;
            return(TRUE);
         }
      }
   }
   else if(IsProtectedMode==0)
      return(TRUE);

   _asm _emit(0xf) _asm add ax,wSeg;   /* in reality it's 'LSL ax,[wSeg]' */
   _asm mov ax,1;                      /* initial boolean 'TRUE' */
   _asm jz seg_ok;                     /* 'Z' is set if operation o.k. */
   _asm mov ax,0;                      /* '0' return value on error.   */

seg_ok:
   
   _asm mov bRval, ax
   
   return(bRval);
}


BFPPROC _IsSelectorReadable(WORD wSeg) /* returns TRUE if selector is readable */
{
   register BOOL bRval;


   if(IsProtectedMode<0)
   {
      if(!_windows_in_real_mode_)
         IsProtectedMode = 1;                   /* protected mode for sure */
      else
      {
         if(GetWinFlags() & WF_PMODE)
         {
            IsProtectedMode = 1;
            _windows_in_real_mode_ = FALSE;
         }
         else
         {
            IsProtectedMode = 0;
            return(TRUE);
         }
      }
   }
   else if(IsProtectedMode==0)
      return(TRUE);

   _asm mov ax,1;                      /* initial boolean 'TRUE' */
   _asm _emit(0xf) _asm add BYTE PTR wSeg,ah;
                                       /* in reality it's 'VERR [wSeg]'   */
   _asm jz seg_ok;                     /* 'Z' is set if operation o.k. */
   _asm mov ax,0;                      /* '0' return value on error.   */

seg_ok:
   
   _asm mov bRval, ax
   
   return(bRval);
}


BFPPROC _IsSelectorWritable(WORD wSeg) /* returns TRUE if selector is writable */
{
   register BOOL bRval;
   
   if(IsProtectedMode<0)
   {
      if(!_windows_in_real_mode_)
         IsProtectedMode = 1;                   /* protected mode for sure */
      else
      {
         if(GetWinFlags() & WF_PMODE)
         {
            IsProtectedMode = 1;
            _windows_in_real_mode_ = FALSE;
         }
         else
         {
            IsProtectedMode = 0;
            return(TRUE);
         }
      }
   }
   else if(IsProtectedMode==0)
      return(TRUE);

   _asm mov ax,1;                      /* initial boolean 'TRUE' */
   _asm _emit(0xf) _asm add BYTE PTR wSeg,ch;
                                       /* in reality it's 'VERW [wSeg]'   */
   _asm jz seg_ok;                     /* 'Z' is set if operation o.k. */
   _asm mov ax,0;                      /* '0' return value on error.   */

seg_ok:
   
   _asm mov bRval, ax
   
   return(bRval);
}


BFPPROC _IsCodeSelector(WORD wSeg) /* returns TRUE if selector is CODE */
{
register BOOL rval;

   _asm
   {
      mov bx,wSeg
      _asm _emit(0x0f) _asm _emit(0x02) _asm _emit(0xc3)  /* LAR AX,BX */

      push ax
      and ax,0x1000        /* is this a regular code/data segment? */
      jz not_code_or_data  /* if zero, requires special handling */
      pop ax
      and ax,0x800         /* if this bit is set it's a CODE segment */
      jz ready_to_return   /* bit clear - it's data! */
      mov ax,1
      jmp ready_to_return

not_code_or_data:
      push ax
      and ax,0x700         /* see if it's a 'call gate' */
      cmp ax,0x400         /* if it's a call gate, this is true */
      jnz not_call_gate    /* otherwise, it's something else... */
      mov ax,2             /* return '2' for call gate, just because */
      jmp ready_to_return

not_call_gate:
      pop ax
      push ax
      and ax,0x500         /* if 0x400 is clear and 0x100 set, it's a */
      cmp ax,0x100         /* 'task state segment' - i.e. not code seg */
      jnz not_task_state_seg
      mov ax,0
      jmp ready_to_return

not_task_state_seg:

      pop ax               /* clean up the stack */
      mov ax,0             /* just say "it isn't a code segment" for now */

ready_to_return:

      mov rval,ax
   }

   return(rval);

}


HINSTANCE FAR PASCAL _GetTaskInstance(HTASK hTask)
{
TASKENTRY te;

   _fmemset((LPSTR)&te, 0, sizeof(te));

   te.dwSize = sizeof(te); /* initializes structure */

   if(lpTaskFindHandle((LPTASKENTRY)&te, hTask))
   {
      return(te.hInst);
   }
   else
   {
      return(NULL);
   }
}



VFPPROC MthreadExit(HTASK hTask)      /* exits by killing all task windows */
{
HTASK hTask2;
//WORD current_task_index;    /* precludes 're-calling' 'GetTaskTableEntry()' */


   if(hWMTHREAD)
   {
      lpMthreadExit(hTask);

      FreeLibrary(hWMTHREAD);

      hWMTHREAD = NULL;

      return;
   }


   if(hTask == (HANDLE)NULL)
      hTask2 = GetCurrentTask();
   else
      hTask2 = hTask;

   if(hTask2==(HANDLE)NULL)                    /* error - bail out of here! */
      return;

//   if((current_task_index = GetTaskTableEntry(hTask2))==BAD_ENTRY)
//      return;                                      /* bail out if not there */

   SET_CURRENT_TASK_INDEX;
   if(NO_TASK_ENTRY) return;                       /*** ERROR ERROR ERROR ***/

   if(MthreadExitStatus) return;                    /* prevents recursion */

   MthreadExitStatus = TRUE;

          /** Here's some fun - force a QUIT status in message **/
          /** loop, and then call 'LoopDispatch()' - this will **/
          /** ensure that each thread is properly terminated,  **/
          /** and will return here when all threads (except    **/
          /** current thread) have properly ended.             **/

   was_quit = TRUE;                    /* force 'WM_QUIT message received' */
   disable_dispatch = DISABLE_DISPATCH_MSG;
                                    /* disable the dispatching of messages */

   LoopDispatch();        /* this causes all threads to die except current */
                          /* thread and thread #0 (if it's not thread 0)   */

/*   EnumTaskWindows(hTask2, MthreadEnumFunction, MAKELPARAM(WM_DESTROY,0)); */

   MthreadExitStatus = FALSE;

   if(lpfnNextHook!=(HHOOK)NULL)
   {
      if(lpUnhookWindowsHookEx)
      {
         lpUnhookWindowsHookEx(lpfnNextHook);
      }
      else
      {
         UnhookWindowsHook(WH_MSGFILTER, lpfnHookProc);
      }

      lpfnNextHook = (HHOOK)NULL;
      (FARPROC &)lpfnHookProc = (FARPROC)NULL;
   }

//   RootApp.hWnd = (HWND)NULL;          /* zero out 'RootApp' entries */
//   RootApp.hTask = (HANDLE)NULL;
//   RootApp.hInst = (HANDLE)NULL;

   if(MarkTaskForDeletion) return;        /* don't delete - already marked */

//   DelTaskTableEntry(hTask);                /* delete entry from table!!! */

}


/*************************************************************************/
/*                     MESSAGE DISPATCH FUNCTIONS                        */
/*************************************************************************/


#pragma code_seg("MYDISPATCH_TEXT","CODE")

BFPPROC _LoopDispatch(void)
{    /* this should be called once for each pass of a time-consuming    */
     /* operation during a thread!  If not in a thread, this function   */
     /* will only have the affect of enabling other applications.       */


   return(!NearThreadDispatch((LPMSG)NULL,NULL,0));

}




/*** MAIN MESSAGE DISPATCHER FOR MULTI-THREAD PROGRAMS.  RETURNS 'TRUE'  ***/
/*** IF 'WM_QUIT' NOT YET RECEIVED, OTHERWISE 'FALSE' FROM THAT POINT ON ***/


BOOL _EXPORT_ LOADDS CALLBACK _MthreadMsgDispatch(LPMSG msg)
{
BOOL GoAheadAndYield, not_quit, SendMessageActive, is_message;
WORD message;
HWND hwnd;

                    /* this function is thread #0!! */


   SET_CURRENT_TASK_INDEX;    /* assigns value to 'current_task_index' */
   if(NO_TASK_ENTRY) return(FALSE);     /*** ERROR ERROR ERROR (QUIT) ***/


   SendMessageActive = InSendMessage();
   not_quit = TRUE;                                      /* initial value */

   GoAheadAndYield = TRUE;     /* first time through only! */

   if(!SendMessageActive && !(disable_dispatch & DISABLE_DISPATCH_MSG))
   {                                  /* normal dispatch loop - disabled? */
//    int current_task_index;       /* copy for use within this scope only */

      INIT_LEVEL

      if(GoAheadAndYield)
      {
         SET_CURRENT_TASK_INDEX;
         if(NO_TASK_ENTRY) return(FALSE); /*** ERROR ERROR ERROR (QUIT) ***/

         GoAheadAndYield = FALSE;  /* only yield ONE TIME! */

      }

      if(PeekMessage(msg,0,NULL,NULL,PM_NOREMOVE | PM_NOYIELD))
      {
         if(((message = msg->message)==WM_QUIT &&
             (disable_messages & DISABLE_WMQUIT)) ||
            (message==WM_CLOSE && (disable_messages & DISABLE_WMCLOSE)))
         {
            PeekMessage(msg,0,NULL,NULL,PM_NOREMOVE);  /* cause 'yield' */
            NearThreadDispatch(msg, NULL, FALSE);
            break;
         }
      }

      not_quit = MthreadGetMessage(msg); /* yields to threads! */
      is_message = TRUE;


      if(is_message && not_quit)
      {
           /********************************************/
           /* TASK messages - send if hwnd==NULL *and* */
           /* 'CompactProc' is NULL (not yet assigned) */
           /********************************************/

         if((hwnd = msg->hwnd)==NULL &&
             (message!=WM_COMPACTING || !CompactProc))
         {
            if(!MthreadAppMessageProc(msg /*, current_task_index */))
            {
               if(!MyTranslate(msg))
               {
                  if(AppMessageProc)
                     AppMessageProc(msg);
                  else
                     DispatchMessage(msg);
               }
            }
         }
                       /**********************/
                       /* COMPACTING MESSAGE */
                       /**********************/

         else if(!hwnd && /* message==WM_COMPACTING &&  <<=== IMPLIED!! */
                 CompactProc)
         {

            CompactProc();  /* call function to reduce memory usage */
         }
          /************************************************/
          /*********** NORMAL (WINDOW) MESSAGE ************/
          /************************************************/

         else
         {
            if(!MyTranslate(msg))    /* did MDI or accel. get translated? */
                DispatchMessage(msg);          /* if not, don't dispatch! */
         }

         SET_CURRENT_TASK_INDEX;     /* required - I might have yielded */
         if(NO_TASK_ENTRY) return(FALSE);   /* ERROR ERROR ERROR (QUIT) */

      }

      if(!not_quit)                   /*   WM_QUIT MESSAGE!!!! */
      {
               /** TELL ALL WINDOWS IN THIS TASK TO QUIT **/

         was_quit = TRUE;  /* causes all subsequent calls to execute this */
                           /* section , but still dispatch messages       */
      }


      END_LEVEL

      SET_CURRENT_TASK_INDEX;                     /* one more time! */
      if(NO_TASK_ENTRY) return(FALSE);     /*** ERROR ERROR ERROR (QUIT) ***/

   }
   else
   {
      if(!SendMessageActive)
      {                             /* yield, but don't dispatch messages */
         PeekMessage(msg, NULL, NULL, NULL, PM_NOREMOVE);
      }

      LoopDispatch();                         /* do at least one thread!! */

      SET_CURRENT_TASK_INDEX;                     /* one more time! */
      if(NO_TASK_ENTRY) return(FALSE);     /*** ERROR ERROR ERROR (QUIT) ***/
   }


   return(!was_quit);

}



BFPPROC LOADDS MthreadNewThreadDispatch(LPMSG msg, LPNEWTHREADINFO lpNewThread)
{
   return(NearThreadDispatch(msg,lpNewThread,FALSE));
}


BOOL API MyGetQueueStatus(void)  // TRUE if hardware input in QUEUE
{
static DWORD (FAR PASCAL *lpGetQueueStatus)(WORD)=NULL;
static BOOL AlreadyChecked = FALSE;
static char WinVer[4]={0};
MSG msg;
HMODULE hUser;


#define dwWinVer (*((DWORD *)WinVer))


   if(!lpGetQueueStatus)
   {
      if(!AlreadyChecked)
      {
         if(!dwWinVer) dwWinVer = GetVersion();

         if(WinVer[0]>=3 && WinVer[1]>=0x0a)
         {
            if(hUser = GetModuleHandle("USER"))
            {
               (FARPROC &)lpGetQueueStatus =
                                  GetProcAddress(hUser, "GetQueueStatus");
            }
         }

         AlreadyChecked = TRUE;
      }

      if(!lpGetQueueStatus)
      {
         return(PeekMessage((LPMSG)&msg, NULL, NULL, NULL,
                            PM_NOYIELD | PM_NOREMOVE)!=0);
      }
      else
      {
         return(HIWORD(lpGetQueueStatus(QS_ALLINPUT))!=0);
      }

   }
   else
   {
      return(HIWORD(lpGetQueueStatus(QS_ALLINPUT))!=0);
   }


}



static BOOL _EXPORT_ LOADDS CALLBACK MthreadGetMessage(LPMSG msg)
{
BOOL wRval=TRUE;
HCURSOR hOldCursor;
MSG msg2;


   SET_CURRENT_TASK_INDEX;    /* assigns value to 'current_task_index' */
   if(NO_TASK_ENTRY) return(FALSE);     /*** ERROR ERROR ERROR (QUIT) ***/

   if(hInternalThreadList && wInternalThreadCount)
   {
      do
      {
         if(MyGetQueueStatus() &&
            PeekMessage(msg, NULL, NULL, NULL, PM_NOREMOVE | PM_NOYIELD))
         {
            wRval = GetMessage(msg, NULL, NULL, NULL);
            break;
         }
         else
         {
            if(!GetInputState()) /* no keyboard input to process anywhere */
            {
                // give a little 'boost' for other apps to run

               PeekMessage(msg, NULL, NULL, NULL, PM_NOREMOVE);

               NearThreadDispatch(msg, NULL, 1);
            }

            if(PeekMessage(msg, NULL, NULL, NULL, PM_NOREMOVE | PM_NOYIELD))
            {
               wRval = GetMessage(msg, NULL, NULL, NULL);
               break;
            }
         }

      } while(TRUE);
   }
   else
   {
      wRval = GetMessage(msg, NULL, NULL, NULL);
   }



   if(msg->message==WM_QUIT ||              /* QUIT or Windows is EXITING! */
       (msg->message==WM_ENDSESSION && msg->wParam))
   {
      SET_CURRENT_TASK_INDEX;     /* assigns value to 'current_task_index' */
      if(NO_TASK_ENTRY) return(FALSE);     /*** ERROR ERROR ERROR (QUIT) ***/

      wRval = was_quit;
      was_quit = TRUE;  /* forces threads to terminate; returns FALSE here */


      hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

      NearThreadDispatch(NULL, NULL, FALSE);
                                         /* wait for threads to TERMINATE! */

      SetCursor(hOldCursor);      /* threads have now been removed.  Good! */


      was_quit = wRval;              /* restore "OLD" value for 'was_quit' */

      wRval = FALSE;
   }
   else if(msg->message==WM_ENTERIDLE)  /* helps threads within menus! */
   {
      if(!PeekMessage((LPMSG)&msg2, NULL, THREAD_IDLE, THREAD_IDLE,
                      PM_NOYIELD | PM_NOREMOVE))
      {
         PostAppMessage(GetCurrentTask(), THREAD_IDLE, 0, 0);
      }
   }

   return(wRval);


}

    /*****************************************************************/
    /***                    NearThreadDispatch()                   ***/
    /*** This function returns FALSE on WM_QUIT - TRUE to continue ***/
    /***                                                           ***/
    /***   Because of the large amount of stack switching it has   ***/
    /***   written as one great big humungous (single) function.   ***/
    /*****************************************************************/


WORD NEAR PASCAL MySelectorVerify(WORD wSel)
{
WORD w1;


   _asm
   {
                       mov ax, wSel
      _asm _emit(0x0f) _asm add bl, al  /* LAR BX,AX */

                       jnz was_error

                       mov w1, bx
   }

   if(!(w1 & 0x8000))       // is the 'P' bit set? (i.e. NOT discarded!)
   {
      // attempt to force segment to load!

      if((w1 & 0x1800) != 0x1800 ||                 // not code segment or
         !GetCodeHandle((FARPROC)MAKELP(wSel, 0)))  // code seg not loaded
      {
was_error:

         return(0);
      }
   }


   // VERIFY READ ACCESS!

   _asm
   {
      _asm _emit(0x0f) _asm add BYTE PTR wSel, ah      /* VERR wSel */
      jnz was_error
   }

   return(wSel);    // at this point, everything's just fine!
}

BOOL NEAR PASCAL NearThreadDispatch(LPMSG lpMsg, LPNEWTHREADINFO lpNewThread,
                                    BOOL wYieldFlag)
{
HANDLE FAR *lpThreads;
BOOL MustYield, flag=FALSE, BadStack=FALSE;
LPINTERNALTHREAD lpIT;
DWORD dwSP;
WORD wSS;
MSG   Msg;
static char pThreadFatalError[]="** WMTHREAD - FATAL ERROR **";
static char buffer[64];      /* for message boxes only */
static int temp_quit;



   if(lpNewThread)
   {
      flag = TRUE;
   }
   else
   {
      flag = FALSE;
   }


   if(!lpMsg) lpMsg = &Msg;



   SET_CURRENT_TASK_INDEX;    /* assigns value to 'current_task_index' */
   if(NO_TASK_ENTRY) return(FALSE);     /*** ERROR ERROR ERROR (QUIT) ***/


   if((!hInternalThreadList || !wInternalThreadCount) && !flag)
   {
      if(!InSendMessage())
      {
         PeekMessage(lpMsg, NULL, NULL, NULL, PM_NOREMOVE);    /* yield */
      }

      return(!was_quit);           /* no threads are active - just return */
   }



/*   MustYield = GetInputState() || MyGetQueueStatus()
/*               || PeekMessage(lpMsg,NULL,NULL,NULL,PM_NOREMOVE|PM_NOYIELD);
/*
*/

             /**=============================================**/
             /** The following is a rather complicated means **/
             /** of determining if I should force thread #0  **/
             /** If 'wYieldFlag' is TRUE then this function  **/
             /** was called from within 'MthreadGetMessage()'**/
             /** (otherwise it wasn't).  If we're thread #0  **/
             /** and this flag is clear, we don't want to go **/
             /** back to thread #0 again (infinite loop?),   **/
             /** ESPECIALLY if there is a 'WaitThread()' or  **/
             /** similar function call going on.  Also, if   **/
             /** I am trying to kill a thread I can assign   **/
             /** the 'next' thread to be the one to kill,    **/
             /** and that thread will *always* go next.      **/
             /**=============================================**/


      /** 'wYieldFlag' values:                                      **/
      /**                                                           **/
      /**    0:   Called from LoopDispatch() or similar - thread #0 **/
      /**         must be switched to whenever a message is in the  **/
      /**         queue or hardware input is available, unless it   **/
      /**         was active when the call was made.                **/
      /**                                                           **/
      /**    1:   MthreadGetMessage() called this function; do not  **/
      /**         switch to any thread except thread #0 unless the  **/
      /**         QUEUE is empty and no hardware input is waiting.  **/
      /**                                                           **/
      /**    2:   This thing was called from within the 'Hook' proc **/
      /**         and it's not thread #0, which means that I must   **/
      /**         not go to thread #0 under *ANY* circumstances!    **/
      /**                                                           **/
      /**    3:   This thing was called from within the 'Hook' proc **/
      /**         and it's thread #0.                               **/
      /**                                                           **/


   if(flag)
//   if(flag || InSendMessage())  // safety valve
   {
      MustYield = FALSE;
   }
   else if(GetInputState())
   {

            /* this way it'll "alternate" unless called */
            /* from within MthreadGetMessage().         */

      if(wYieldFlag!=1)            /* not called from MthreadGetMessage() */
      {
         if(wInternalThreadIndex) /* if I'm being killed, don't yield! */
         {
            lpThreads = (HANDLE FAR *)GlobalLock(hInternalThreadList);
            lpIT = (LPINTERNALTHREAD)GlobalLock(lpThreads[wInternalThreadIndex]);

            MustYield = !(lpIT->wStatus & THREADSTATUS_KILL)
                        && wYieldFlag!=2 && wYieldFlag!=3;

            GlobalUnlock(lpThreads[wInternalThreadIndex]);
            GlobalUnlock(hInternalThreadList);
         }
         else   /* from within thread #0! */
         {
            MustYield = !InSendMessage() && (wYieldFlag!=2 && wYieldFlag!=3);
                                                /* force YIELD if possible */
         }
      }

        /** At this point I was called from within MthreadGetMessage() **/

      else if(!InSendMessage())
      {
         PeekMessage(lpMsg,NULL,NULL,NULL,PM_NOREMOVE);     /* yields now */
         MustYield = FALSE;              /* force 1 thread to run, anyway */
      }
      else
      {
         MustYield = FALSE; /* just for fun, force a single thread to run */
      }
   }
   else
   {
      if(wYieldFlag!=1)              /* not called from MthreadDispatch() */
      {
         if(wInternalThreadIndex) /* if I'm being killed, don't yield! */
         {
            lpThreads = (HANDLE FAR *)GlobalLock(hInternalThreadList);

            lpIT = (LPINTERNALTHREAD)GlobalLock(lpThreads[wInternalThreadIndex]);

          /* assign 'MustYield' to TRUE if there's a message waiting */
          /* and no conditions exists which might preclude it, like  */
          /* 'THREADSTATUS_KILL' or call from within 'Hook' proc.    */

            MustYield = (!(lpIT->wStatus & THREADSTATUS_KILL)
                         && wYieldFlag!=2 && wYieldFlag!=3);

//            MustYield = !(lpIT->wStatus & THREADSTATUS_KILL)
//                        && wYieldFlag!=2 && wYieldFlag!=3
//                        && (MyGetQueueStatus() ||
//                            PeekMessage(lpMsg, NULL, NULL, NULL,
//                                        PM_NOYIELD | PM_NOREMOVE));

            GlobalUnlock(lpThreads[wInternalThreadIndex]);
            GlobalUnlock(hInternalThreadList);
         }
         else   /* from within thread #0! */
         {
            MustYield = !InSendMessage() && (wYieldFlag!=2 && wYieldFlag!=3);
                         /* force YIELD if possible whenever in thread #0 */
         }
            /* assign to TRUE if there's a message waiting... */

         MustYield = MustYield && MyGetQueueStatus() &&
                     PeekMessage(lpMsg, NULL, NULL, NULL,
                                 PM_NOYIELD | PM_NOREMOVE);
      }

        /** At this point I was called from within MthreadGetMessage() **/

      else
      {
         MustYield = FALSE;    /* force a single thread to run, always! */
      }
   }


              /* to provide at least *SOME* optimization */

   if(!flag && MustYield && !wInternalThreadIndex && !was_quit)
   {
      if(wInternalThreadCount <= 1 && wYieldFlag == 1)
      {
         WaitMessage();
      }

      return(TRUE);
   }


                       /*****************************/
                       /*** CREATING 'THREADLIST' ***/
                       /*****************************/

   if(flag && !hInternalThreadList)
   {

      if(was_quit) return(FALSE);     /* just in case... */

      hInternalThreadList = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                        MAX_THREADLIST * sizeof(HANDLE));
      if(!hInternalThreadList)
      {
         return(-1);  /* this basically says that there was an error! */
      }

      lpThreads = (HANDLE FAR *)GlobalLock(hInternalThreadList);  /* yahoo! get the list */

      *lpThreads = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,
                               sizeof(INTERNALTHREAD));

      if(!(*lpThreads))
      {
         GlobalUnlock(hInternalThreadList);
         GlobalFree(hInternalThreadList);
         hInternalThreadList = NULL;

         return(-1);  /* basically, this says 'there was an error' */
      }

      lpIT = (LPINTERNALTHREAD)GlobalLock(*lpThreads);


                 /** Assign 'ID' elements of structure **/

      _fmemset(lpIT->zeros, 0, sizeof(lpIT->zeros));
      _fmemcpy(lpIT->ID, "INTERNALTHREAD", sizeof(lpIT->ID));
      lpIT->hSelf = *lpThreads;

      wInternalThreadIndex = 0;
      wInternalThreadCount = 1;      /* a single thread - me! */

   }
              /** NORMAL - GET THREAD LIST & CURRENT POINTER **/
   else
   {
      lpThreads = (HANDLE FAR *)GlobalLock(hInternalThreadList);  /* yahoo! get the list */
      lpIT = (LPINTERNALTHREAD)GlobalLock(lpThreads[wInternalThreadIndex]); /* CURRENT thread */
   }


   _asm mov wSS,SS


   dwSP = 0;

   if(flag)
   {               /* ahah!  creating new thread - push '0' for */
                   /* ES, FS, and GS registers (as applicable)  */

      if(was_quit) /* well, now, I have received a WM_QUIT - I suppose */
      {            /* that I should *NOT* add another thread now, eh?  */

         GlobalUnlock(lpThreads[wInternalThreadIndex]);
         GlobalUnlock(hInternalThreadList);
         return(FALSE);                        /* ee-gads - an error!! */
      }

      if(_ok_to_use_386_instructions_)

      {
         _asm
         {
            USE_32_BIT_OPERAND  _asm _emit(0x60);  /* PUSHA */
//            USE_32_BIT_OPERAND  _asm pushf
                                pushf
                                mov ax, 0
                                push ax    /* ES */
                                push ax    /* FS */
                                push ax    /* GS */

            USE_32_BIT_OPERAND  _asm mov WORD PTR dwSP,SP
         }
      }
      else
      {
         _asm
         {
            pushf
            _emit(0x60);  /* PUSHA */
            mov ax, 0
            push ax

            mov WORD PTR dwSP,SP
            nop             /* added to prevent compiler/assembly problems */
         }
      }
   }
   else
   {
      if(_ok_to_use_386_instructions_)

      {
         _asm
         {
            USE_32_BIT_OPERAND  _asm _emit(0x60);  /* PUSHA */
                                pushf
                                push ES

            _emit(0x0f)         _asm _emit(0xa0)        /* push FS */
            _emit(0x0f)         _asm _emit(0xa8)        /* push GS */

            USE_32_BIT_OPERAND  _asm mov WORD PTR dwSP,SP
         }
      }
      else
      {
         _asm
         {
            pushf
            _emit(0x60);  /* PUSHA */
            push ES
            mov WORD PTR dwSP,SP
            nop             /* added to prevent compiler/assembly problems */
         }
      }
   }

   lpIT->ESP = dwSP;    /* save current stack pointer and stack segment */
   lpIT->SS = wSS;

   GlobalUnlock(lpThreads[wInternalThreadIndex]);  /* unlock the current one now */

   lpIT = NULL;


                     /*******************************/
                     /*** NEW THREAD CREATED HERE ***/
                     /*******************************/


   if(flag && !was_quit)
   {           /* there's a new thread in town - fix it up! */

    static LPNEWTHREADINFO lpNT;    /* DS remains the same, eh? */
    static LONG rval;               /* return value when complete */
    static WORD wThreadID;
    static DWORD dwSP;
    static WORD wSS;
    static LPINTERNALTHREAD lpIT2;
    static HANDLE FAR *lpThreads2;

      lpNT = lpNewThread;           /* make a (temporary) copy */
      dwSP = lpNT->wNewSP;
      wSS  = lpNT->wNewSS;

      wThreadID = (WORD)lpNT->hThreadID;  /* temporary storage... */

      wInternalThreadIndex = wInternalThreadCount;
      wNextInternalThread  = 0;        /* the next thread to run after this */
                  /* new entry is next one to be accessed on LoopDispatch() */

      lpThreads[wInternalThreadCount++] = lpNT->hThreadID;
                              /* place handle to thread ID block into table */

      GlobalUnlock(hInternalThreadList);  /* prior to stack switch */
      lpThreads = NULL;                   /* clean this thing up! */

      _asm
      {
         mov AX,wSS
         mov BX,WORD PTR dwSP
         mov SS,AX
         mov SP,BX                   /* stack switch completed */
         mov AX,0
         push AX                     /* ensure top 2 words are zero */
         push AX
         mov AX, WORD PTR wThreadID  /* save the handle to this thread! */
         push AX                     /* place thread ID handle on stack */
         push DS             /* and make copy of data segment too; why not? */
      }

                /**********************************************/
                /**  initiate thread by calling thread proc  **/
                /**  use the 'CallThreadProc()' function to  **/
                /**  load the instance's DS automatically.   **/
                /**********************************************/

      CallThreadProc(lpNT->lpThreadProc,lpNT->hCaller,lpNT->hArgs,
                     lpNT->wFlags, GetCurrentInstance(), (HANDLE)wThreadID);


                /**********************************************/
                /** Thread proc has terminated at this point **/
                /**********************************************/

      /* ensure DS & ThreadID are correct, assign return value to 'rval' */
      _asm
      {
         pop DS            /* grab data segment off of stack (again) */
         pop BX            /* the task handle ID */
         mov wThreadID, BX /* and save the value in static var - ha! */

         mov WORD PTR rval, AX     /* save return value from function */
         mov WORD PTR rval+2,DX    /* in the static variable 'rval'.  */
      }

          /* NOW for the tricky part...                             */
          /* this next section must switch to a valid stack so that */
          /* it can remove the current one.  This means that I must */
          /* get the *original* stack pointer from thread #0 and    */
          /* switch to that, then re-initialize all automatic var's */
          /* (requires popping and re-pushing registers!!)          */

      SET_CURRENT_TASK_INDEX;                     /* one more time! */
      if(NO_TASK_ENTRY)
      {
         FatalAppExit(0,"?NO TASK ENTRY in 'MthreadMsgDispatch'");
                          /*** ERROR ERROR ERROR (UAE) ***/
      }

            /* since we have no valid auto variables yet, all */
            /* variables accessed here must be 'static'.      */

      lpThreads2 = (HANDLE FAR *)GlobalLock(hInternalThreadList);
      lpIT2 = (LPINTERNALTHREAD)GlobalLock(lpThreads2[0]);                /* 'ROOT' thread */


      dwSP = lpIT2->ESP;
      wSS  = lpIT2->SS;

      GlobalUnlock(lpThreads2[0]);              /* unlock global handles */
      GlobalUnlock(hInternalThreadList);       /* until stack is switched */

      if(_ok_to_use_386_instructions_)
      {
         _asm
         {
                               mov AX,wSS
            USE_32_BIT_OPERAND _asm mov BX,WORD PTR dwSP
                               mov SS,AX
            USE_32_BIT_OPERAND _asm mov SP,BX   /* stack switch completed */

                               call MySelectorVerify
                               push ax
            _asm _emit(0x0f)   _asm _emit(0xa9)        /* pop GS */

                               call MySelectorVerify
                               push ax
            _asm _emit(0x0f)   _asm _emit(0xa1)        /* pop FS */

                               call MySelectorVerify
                               push ax
                               pop ES

                               popf

            USE_32_BIT_OPERAND _asm _emit(0x61) /* POPA */
                                                   /* registers restored */

            USE_32_BIT_OPERAND  _asm _emit(0x60);  /* PUSHA */
                                pushf
                                push ES

            _emit(0x0f)         _asm _emit(0xa0)        /* push FS */
            _emit(0x0f)         _asm _emit(0xa8)        /* push GS */
                                                    /* registers saved */
         }
      }
      else
      {
         _asm
         {
            mov BX,wSS
            mov AX,WORD PTR dwSP
            mov SS,BX
            mov SP,AX         /* stack switch completed */
            pop ES
            _emit(0x61)  /* POPA */
            popf              /* registers restored */

            pushf
            _emit(0x60);  /* PUSHA */
            push ES             /* registers saved */
         }
      }
          /* note that stored registers were popped, but were */
          /* re-pushed onto the stack (as they were before)   */
          /* this way automatic variables are available again */

          /* now that I have a valid stack, terminate old thread */

      InternalThreadExit((HANDLE)wThreadID, rval);
        /** This function processes notifications and does cleanup **/
        /** including the re-assignment of 'wInternalThreadIndex'. **/


      /** At this point wInternalThreadIndex should point to the next  **/
      /** item to be executed, and the stack for thread #0 hasn't been **/
      /** changed.  Therefore I should be able to continue from this   **/
      /** point as though I had switched out of a thread and I'm going **/
      /** to switch into another thread.  YES!!!                       **/

      /** BUT:  Before I do, make sure that there's still a task table **/
      /**       entry (because 'InternalThreadExit()' might delete it) **/

      SET_CURRENT_TASK_INDEX;                     /* one more time! */
      if(NO_TASK_ENTRY)
      {                                  /** No Entry - bail out time **/

         if(_ok_to_use_386_instructions_)
         {
            _asm
            {
                                  call MySelectorVerify
                                  push ax
               _asm _emit(0x0f)   _asm _emit(0xa9)        /* pop GS */

                                  call MySelectorVerify
                                  push ax
               _asm _emit(0x0f)   _asm _emit(0xa1)        /* pop FS */

                                  call MySelectorVerify
                                  push ax
                                  pop ES

                                  popf

               USE_32_BIT_OPERAND _asm _emit(0x61) /* POPA */
                                                      /* registers restored */
            }
         }
         else
         {
            _asm
            {
               pop ES
               _emit(0x61)  /* POPA */
               popf              /* registers restored */
            }
         }

             /** Registers recovered from stack - now get out **/

         return(FALSE);                   /* return a 'QUIT' status  */
                                          /* using the current stack */
      }

      /* NEXT:  Re-assign values for the thread table & current thread */
      /*        These *ABSOLUTELY MUST* be in AUTOMATIC VARIABLES!     */

      lpThreads = (HANDLE FAR *)GlobalLock(hInternalThreadList);
      lpIT = NULL;             /* required later */

      wNextInternalThread = wInternalThreadIndex;  /* assign 'next' thread */



#ifdef DISPATCH_WITHIN_HOOK

      // see if there's only 1 thread remaining (now) and kill the timer
      // (and reset all of the static vars) if so.  This should eliminate
      // a random bug that shows up once in a while...

      if(wTimer && wInternalThreadCount<=1)  // assume no more threads running
      {
         wTimer = SetTimer(NULL, MTH_TIMER_ID, 100, lpDispatchWithinHookTimerProc);

         KillTimer(NULL, wTimer);
         FreeProcInstance((FARPROC)lpDispatchWithinHookTimerProc);

         lpDispatchWithinHookTimerProc = NULL;
         wTimer = NULL;
      }

#endif

   }


   if(was_quit)
   {
                /******************************************/
                /*       QUIT MESSAGE PROCESSING          */
                /******************************************/
                /* if QUIT message run 'last' thread next */
                /* passing the 'QUIT' status to it...     */
                /******************************************/

      if(wInternalThreadCount>1)
      {
         wInternalThreadIndex = wInternalThreadCount - 1;
         wNextInternalThread = wInternalThreadIndex;
      }
      else
      {
         wInternalThreadIndex = 0;
         wNextInternalThread = 1;  /* the standard - that's why... */
      }

      lpIT = (LPINTERNALTHREAD)GlobalLock(lpThreads[wInternalThreadIndex]);
      lpIT->wStatus = 0;               /* eliminate all waiting */

   }
   else if(MustYield)                /* proceed immediately to thread #0!! */
   {
      wInternalThreadIndex = 0;          /* thread 0 is used to 'yield' */

      lpIT = (LPINTERNALTHREAD)GlobalLock(lpThreads[0]);
   }
   else
   {
    WORD wOldIndex = wInternalThreadIndex;  // save for later...


      wInternalThreadIndex = wNextInternalThread;

                   /************************************/
                   /* GET THE NEXT THREAD IN THE QUEUE */
                   /************************************/

      while(wInternalThreadIndex < wInternalThreadCount)
      {
         lpIT = (LPINTERNALTHREAD)GlobalLock(lpThreads[wInternalThreadIndex]);

         if(lpIT->wStatus & THREADSTATUS_KILL)
         {
                 /* ensure 'suspend' or 'wait thread' flag is OFF */

            lpIT->wStatus &= ~THREADSTATUS_WAITING;

               /* make sure this thread runs until it dies! */
               /* notwithstanding 'thread #0' and messages. */

            wNextInternalThread = wInternalThreadIndex;

            break;                        /* bust out of loop - found one! */
         }
         else if(lpIT->wStatus & THREADSTATUS_WAITING)
         {          /* waiting for a task/thread to complete, eh? */

            if((lpIT->wStatus & THREADSTATUS_WAITTHREAD) &&
               !IsInternalThread(lpIT->hWaitThread))
            {
                  /*       simply stated, it finished already!       */
                  /* normally, however, this is taken care of in the */
                  /*     'InternalThreadExit()' function, below.     */

                  lpIT->wStatus &= ~THREADSTATUS_WAITTHREAD;
                  lpIT->hWaitThread = NULL;

                  wNextInternalThread = wInternalThreadIndex + 1;

                  break;
            }
         }
         else
         {
            wNextInternalThread = wInternalThreadIndex + 1;

            break;
         }

         GlobalUnlock(lpThreads[wInternalThreadIndex]);

         wInternalThreadIndex++;

         lpIT = NULL;                             /* ensure it's null now */

      }

                /* if 'wYieldFlag' is 2, avoid thread #0! */

      if(!lpIT && (wYieldFlag!=2 || wInternalThreadCount<=1))
      {
         wInternalThreadIndex = 0;  /* ensure it points to the correct entry */

         // SPECIAL SECTION!  *IF* I am waiting in all other threads then
         // I need to do a 'WaitMessage()' right here.

         if(!wOldIndex && !InSendMessage() && wYieldFlag == 1)
         {
          BOOL bOldIdleMessage = mtIdleMessage;

            mtIdleMessage = TRUE;    // force THREAD_IDLE message to be
                                     // sent when timer proc is activated

            WaitMessage();           // if it WAS 0, and it IS 0 now,
                                     // wait for a message first!!

            SET_CURRENT_TASK_INDEX;
            if(NO_TASK_ENTRY) return(FALSE);     /*** ERROR ERROR ERROR (QUIT) ***/

            mtIdleMessage = bOldIdleMessage;

            lpIT = (LPINTERNALTHREAD)GlobalLock(lpThreads[0]); /* the 'root' thread (msg loop) */

// BUG FIX/WORKAROUND - 2/7/95

            // There is a small possibility that if a yield took place here
            // that I might have destroyed the stack pointer for thread #0,
            // if I'm currently *IN* thread #0.

            if(_ok_to_use_386_instructions_)
            {
               _asm mov wSS, ss

               _asm _emit(0x66) _asm mov WORD PTR dwSP, sp /* mov dwSP, esp */
            }
            else
            {
               dwSP = 0;
               _asm mov WORD PTR dwSP, sp

               _asm mov wSS, ss
            }

            lpIT->ESP = dwSP; /* save current stack pointer and stack segment */
            lpIT->SS = wSS;

// BUG FIX/WORKAROUND - 2/7/95
         }
         else
         {
            lpIT = (LPINTERNALTHREAD)GlobalLock(lpThreads[0]); /* the 'root' thread (msg loop) */
         }


         wInternalThreadIndex = 0;  /* ensure it points to the correct entry */
         wNextInternalThread = 1; /* the "NEXT" thread to run after this one */
      }
      else if(!lpIT)
      {
         wInternalThreadIndex = 1;
         wNextInternalThread = 2;

         lpIT = (LPINTERNALTHREAD)GlobalLock(lpThreads[1]);
      }

   }

      /** The grand final‚ - SWITCH STACKS AND GET OUTA HERE!!!!! **/

   dwSP = lpIT->ESP;
   wSS  = lpIT->SS;


      /**************************************************************/
      /** THIS SPECIAL SECTION OF CODE VERIFIES THE STACK SELECTOR **/
      /**************************************************************/

   _asm
   {
      mov ax, wSS                         /* get access rights of SS */
      _emit (0xf) _asm add bl, al         /* LAR BX,AX */
      jz  selector_ok                     /* zero ==> it was successful! */
                                          /* non-zero ==> error!         */
      mov ax,1                            /* BadStack==1 invalid selector */
      jmp i_am_done

selector_ok:
      mov ax,bx
      and ax,0x1800                       /* make sure it's a data seg */
      cmp ax,0x1000
      jz is_data_seg                      /* if it's equal, it's a data seg */
      mov ax,2                            /* BadStack==2  not data segment */
      jmp i_am_done

is_data_seg:
      and bx, 0x0200                      /* see if it's "writeable" */
      jnz is_writeable
      mov ax,3                            /* BadStack==3  readonly segment */
      jmp i_am_done

is_writeable:
      mov ax,0                            /* GOOD result!  BadStack==0 */

i_am_done:
      mov WORD PTR BadStack,ax            /* error code value */

   }

   if(!BadStack)  // this next section is a "safety valve" to detect
   {              // a "bad stack" by comparing to the EBP it will have...

      // 386:  ESP at offset 0cH plus 4 WORD values (ES,FS,GS,flags)
      //       total bytes pushed:  28H
      // 286:  SP at offset 06H plus 1 WORD value (ES)
      //       total bytes pushed:  12H

      if((_ok_to_use_386_instructions_ &&
          (WORD)(dwSP + 0x28) != *((WORD FAR *)MAKELP(wSS, (WORD)dwSP + 0x14)))  ||
         (!_ok_to_use_386_instructions_ &&
          (WORD)(dwSP + 0x12) != *((WORD FAR *)MAKELP(wSS, (WORD)dwSP + 8 ))) )
      {
       static const char CODE_BASED szMsg[]=
            "?Warning - SP was inadvertently trashed in destination thread";

         FatalAppExit(0, szMsg);
      }
   }

   if(BadStack)
   {
      if(!wInternalThreadIndex)
      {
         switch(BadStack)
         {
            case 2:
                FatalAppExit(0,"Root Thread - SS not DATA");

            case 3:
                FatalAppExit(0,"Root Thread - SS not writeable");

            default:
                FatalAppExit(0,"Root Thread - BAD Stack Selector");
         }
      }

              /* unlock the thread's descriptor handle */

      GlobalUnlock(lpThreads[wInternalThreadIndex]);

              /* inform user of problem and kill thread */

      switch(BadStack)
      {
         case 2:            /* not data selector */

           wsprintf(buffer, "?Stack Selector for thread %4x is not DATA",
                    (WORD)lpThreads[wInternalThreadIndex]);

           MessageBox(NULL,buffer,pThreadFatalError,
                      MB_OK | MB_ICONHAND | MB_SYSTEMMODAL);

           break;


         case 3:            /* not writeable selector */

           wsprintf(buffer, "?Stack Selector for thread %4x is read-only",
                    (WORD)lpThreads[wInternalThreadIndex]);

           MessageBox(NULL,buffer,pThreadFatalError,
                      MB_OK | MB_ICONHAND | MB_SYSTEMMODAL);

           break;


         default:           /* invalid selector */

           wsprintf(buffer, "?Stack Selector for thread %4x is not valid",
                    (WORD)lpThreads[wInternalThreadIndex]);

           MessageBox(NULL,buffer,pThreadFatalError,
                      MB_OK | MB_ICONHAND | MB_SYSTEMMODAL);

           break;

      }

               /* this thread is no good - kill it now! */

      InternalThreadExit(lpThreads[wInternalThreadIndex], -1);

               /* restore thread #0 and continue onward */

      wInternalThreadIndex = 0;   /* ensure it points to the correct entry */

      lpIT = (LPINTERNALTHREAD)GlobalLock(lpThreads[0]);    /* the 'root' thread (msg loop) */
   }

                   /*********************************/
                   /** FINAL STACK SWITCH AND EXIT **/
                   /*********************************/

   temp_quit = was_quit;

   if(lpIT->wStatus & THREADSTATUS_KILL) temp_quit = TRUE;   /* force death */

   GlobalUnlock(lpThreads[wInternalThreadIndex]);
                                             /* unlock current thread info */
   if(_ok_to_use_386_instructions_)
   {
      _asm
      {
                            mov AX,wSS
         USE_32_BIT_OPERAND _asm mov BX,WORD PTR dwSP
                            mov SS,AX
         USE_32_BIT_OPERAND _asm mov SP,BX   /* stack switch completed */

                            call MySelectorVerify
                            push ax
         _asm _emit(0x0f)   _asm _emit(0xa9)        /* pop GS */

                            call MySelectorVerify
                            push ax
         _asm _emit(0x0f)   _asm _emit(0xa1)        /* pop FS */

                            call MySelectorVerify
                            push ax
                            pop ES

                            popf

         _asm{ USE_32_BIT_OPERAND _asm _emit(0x61)  /* POPA */ }
      }
   }
   else
   {
      _asm
      {
         mov AX,wSS
         mov BX,WORD PTR dwSP
         mov SS,AX
         mov SP,BX         /* stack switch completed */
         pop ES
         _emit(0x61)  /* POPA */
         popf
      }
   }


   GlobalUnlock(hInternalThreadList);           /* unlock the thread list */
                     /* and that's the end, guys! */

   return(!temp_quit);

}

 /**************************************************************************/
 /*                                                                        */
 /*                            CallThreadProc()                            */
 /*                                                                        */
 /* This next function does the great courtesy of transferring all of the  */
 /* static variables from above into automatic variables.  How convenient! */
 /* Now, it's really easy to load the 'Instance' data segment before the   */
 /* call.  Now that's the way to do it, eh?                                */
 /*                                                                        */
 /**************************************************************************/

LONG NEAR PASCAL CallThreadProc(LPTHREADPROC lpThreadProc, HANDLE hCaller,
                                HANDLE hArgs, WORD wFlags, HINSTANCE hInst,
                                HANDLE hThreadID)
{
WORD wDS;
LONG rval;
HWND hwndTemp;

   if(!(wDS = SELECTOROF(GlobalLock(hInst)) ) ) return(-1L);  /* error */

   GlobalUnlock(hInst);  /* now unlock it. */

   _asm mov DS,wDS       /* load new data segment.  No problem. It gets */
                         /* popped after I return anyway.               */

   rval = lpThreadProc(hCaller, hArgs, wFlags);

   /*** IF THERE ARE ANY NOTIFICATIONS TO BE DONE, THE CODE BELONGS HERE ***/
   /*** This way I can yield if I need to without blowing anything up!   ***/
   /*** (that is because the thread id and stack still live...)          ***/

   ThreadComplete(rval); /* this notifies owner that the task finished */
                         /* and supplies a return value.  Note that it */
                         /* get sent as a 'Task' message to the owning */
                         /* task (this one, right?) and may require a  */
                         /* special process to detect it.  That's not  */
                         /* my responsibility though.                  */

   PostAppMessage(GetCurrentTask(),THREAD_COMPLETE,(WPARAM)hThreadID,
                  (LPARAM) rval);
//   hwndTemp = GetMthreadWindow();  /* temporary 'main thread window' */
//
//   if(hwndTemp)
//   {
//      PostMessage(hwndTemp, THREAD_COMPLETE, (WPARAM)hThreadID,
//                   (LPARAM)rval);
//   }

   return(rval);   /* that's the end, eh?  Howse that for *smoooooth*! */
}


/***************************************************************************/
/*                                                                         */
/*  InternalThreadExit() - procedure which is called when a thread has     */
/*                         terminated.  Does the required housekeeping     */
/*                                                                         */
/***************************************************************************/


void NEAR PASCAL InternalThreadExit(HANDLE hThreadID, LONG rval)
{
HANDLE FAR *lpThreads;
LPINTERNALTHREAD lpIT;
int i;
HANDLE hTemp;

    /** This function MUST remove the internal thread and **/
    /** free up the stack/information area, and clean up  **/
    /** the internal thread information tables.           **/


   SET_CURRENT_TASK_INDEX;                     /* one more time! */
   if(NO_TASK_ENTRY)
   {
      FatalAppExit(0,"?NO TASK ENTRY in 'InternalThreadExit'");
                       /*** ERROR ERROR ERROR (UAE) ***/
   }

   lpThreads = (HANDLE FAR *)GlobalLock(hInternalThreadList);  /* yahoo! get the list */

   if(lpThreads[wInternalThreadIndex]==hThreadID)
   {
      lpIT = (LPINTERNALTHREAD)GlobalLock(lpThreads[wInternalThreadIndex]);/* CURRENT thread */
      i = wInternalThreadIndex;
   }
   else
   {
      for(i=1; i<(int)wInternalThreadCount; i++)
      {
         if(lpThreads[i]==hThreadID)
         {
            break;
         }
      }

      if(i>=(int)wInternalThreadCount)   /* that means I didn't find it */
      {
         GlobalUnlock(hInternalThreadList);

         while(GlobalFlags(hThreadID) & GMEM_LOCKCOUNT)
            GlobalUnlock(hThreadID);           /* *REALLY* unlocked! */
         GlobalFree(hThreadID);          /* at least I can free it here... */

         return;                          /* can't do anything else now... */
      }
   }

   lpIT = (LPINTERNALTHREAD)GlobalLock(hThreadID);  /* get pointer to thread information */
   if(lpIT->hSelf==hThreadID)       /* if corrupted skip next part */
   {
      if(lpIT->SS!=SELECTOROF(lpIT))        /* separate stack area!! */
      {
         hTemp = (HANDLE)LOWORD(GlobalHandle(lpIT->SS));
         GlobalUnlock(hTemp);
         GlobalFree(hTemp);
      }
   }
   if(lpIT->lpThreadProc)        /* free the 'proc instance' if applicable */
      FreeProcInstance((FARPROC)lpIT->lpThreadProc);


   while(GlobalFlags(hThreadID) & GMEM_LOCKCOUNT)
      GlobalUnlock(hThreadID);  /* free memory used by thread descriptor */

   GlobalFree(hThreadID);       /* (and, most likely, the stack as well) */

   wInternalThreadCount--;
   if(i<(int)wInternalThreadCount)    /* are other threads after it? */
   {
      _fmemcpy((LPVOID)(lpThreads + i), (LPVOID)(lpThreads + i + 1),
               (wInternalThreadCount - i) * sizeof(*lpThreads));
   }

   lpThreads[wInternalThreadCount] = NULL;  /* last is now NULL */


        /** OK - if there's a thread (or more than one) out there **/
        /**      waiting for this one to end, fix it!             **/


   for(i=0; i<(int)wInternalThreadCount; i++)
   {

      lpIT = (LPINTERNALTHREAD)GlobalLock(lpThreads[i]);

      if(lpIT->wStatus & THREADSTATUS_WAITTHREAD &&
         lpIT->hWaitThread == hThreadID)
      {
         lpIT->wStatus &= ~THREADSTATUS_WAITTHREAD;
         lpIT->hWaitThread = NULL;
      }

      GlobalUnlock(lpThreads[i]);

   }


   GlobalUnlock(hInternalThreadList);       /* that's the end */



   /** Final step:  If this is the 'last' thread to go, and the task is  **/
   /**             'marked for deletion', then now's the time to kill it **/

//   if(MarkTaskForDeletion && wInternalThreadCount<2)
//   {
//      DelTaskTableEntry(NULL);
//   }

}



/***************************************************************************/
/*                        MthreadAppMsgHookProc()                          */
/*                                                                         */
/* This proc is assigned as a 'Windows Hook' using 'WH_MSGFILTER' in the   */
/* call to 'SetWindowsHook()'.  It dispatches special application messages */
/* associated with the multi-thread operations defined by this program.    */
/*                                                                         */
/* In order to allow call to 'NearThreadDispatch()' it is already located  */
/* in the SAME code segment 'MYDISPATCH_TEXT'.                             */
/*                                                                         */
/***************************************************************************/


LRESULT EXPORT LOADDS CALLBACK MthreadAppMsgHookProc(int nCode, WPARAM wParam,
                                                      LPARAM lParam)
{
LPMSG lpMsg;
/* int current_task_index; */
LRESULT rval;
#ifdef DISPATCH_WITHIN_HOOK
MSG msg;
#endif


   /*****************************************************************/
   /* simultaneously check if 'nCode' is either 'MSGF_DIALOGBOX' or */
   /* 'MSGF_MENU', and if so assign 'lpHookMsg' to the pointer from */
   /* 'lParam' and check if 'hWnd' is NULL; if none of the above is */
   /* satisfied then just return a NULL to say "I didn't process it"*/
   /*****************************************************************/

   SET_CURRENT_TASK_INDEX;       /* assigns value to 'current_task_index' */

   if(nCode>=0)
   {
      lpMsg = (LPMSG)lParam;
   }

   if(nCode<0 || (nCode!=MSGF_DIALOGBOX && nCode!=MSGF_MENU) ||
           lpMsg->hwnd!=(HWND)NULL )
   {
      rval = FALSE;
   }
   else if(!NO_TASK_ENTRY)
   {

   /*******************************************************************/
   /** At this point I have a 'Task' message (hWnd == NULL), so make **/
   /** an attempt to process it via 'MthreadAppMessageProc()' and    **/
   /** return the result code to the caller.                         **/
   /*******************************************************************/


      rval = (LRESULT)(DWORD)MthreadAppMessageProc(lpMsg /*, current_task_index */);


   }

   if(lpCallNextHookEx)
   {
      rval |= lpCallNextHookEx(lpfnNextHook, nCode, wParam, lParam);
   }
   else
   {
      rval |= (LRESULT)DefHookProc(nCode, (WORD)wParam, (DWORD)lParam,
                                   (HHOOK FAR *)&(lpfnNextHook));
   }

#ifdef DISPATCH_WITHIN_HOOK

   if(InSendMessage()) return(rval);  // SAFETY VALVE!


            /* do a 'LoopDispatch()' to allow some threads   */
            /* to run.                                       */

   SET_CURRENT_TASK_INDEX;       /* assigns value to 'current_task_index' */

   if(!NO_TASK_ENTRY && wInternalThreadCount)
   {
      if(!wInternalThreadIndex)
      {
         if(nCode==MSGF_DIALOGBOX || nCode==MSGF_MENU)
         {
            NearThreadDispatch(NULL, NULL, 3);  /* like 'MthreadGetMessage()' */
            SET_CURRENT_TASK_INDEX;  /* re-assign value to 'current_task_index' */
         }

      }
      else
      {
         _asm nop                /* so I can set a breakpoint - for testing. */


         if((nCode==MSGF_DIALOGBOX || nCode==MSGF_MENU)
            && lpMsg && !lpMsg->hwnd)
         {
            NearThreadDispatch(NULL, NULL, 2);
                                             /* signal it's from HOOK PROC! */
         }

         SET_CURRENT_TASK_INDEX;  /* re-assign value to 'current_task_index' */
      }
   }

//   if((nCode==MSGF_DIALOGBOX || nCode==MSGF_MENU) && lpMsg && !lpMsg->hwnd)
//   {
//               /** ok - hook into thread dispatch function **/
//               /** but first, disable message dispatching  **/
//
//
//
//      LoopDispatch();   /* guaranteed *NOT* to call message loop, ever, */
//                        /* without switching stacks first!!             */
//                        /* And, since this *SHOULD* be thread #0, the   */
//                        /* message loop will return back to HERE!       */
//
//               /* I am done - re-enable dispatching messages! */
//   }
//

   if((nCode==MSGF_DIALOGBOX || nCode==MSGF_MENU) && lpMsg)
   {
      SET_CURRENT_TASK_INDEX;
      if(NO_TASK_ENTRY) return(FALSE);              /*** ERROR ERROR ERROR ***/

      if(!PeekMessage((LPMSG)&msg, (HWND)NULL, THREAD_IDLE, THREAD_IDLE,
                      PM_NOREMOVE | PM_NOYIELD))
      {
         mtIdleMessage = TRUE; /* allow another 'Idle' message to be posted */
      }

   }

#endif

   return(rval);
}


/***************************************************************************/
/*           This next section has been placed here for testing            */
/***************************************************************************/

WORD FAR PASCAL DefMthreadCommDlgHookProc(HWND hDlg, WORD wMsg, WORD wParam,
                                          DWORD lParam);
                   /* this proc should either be used as a 'Hook Proc' for  */
                   /* 'COMMDLG' dialog boxes, or else be returned by user   */
                   /* 'Hook Proc's if the message is not processed, so that */
                   /* message processing will work correctly.               */



WORD FAR PASCAL DefMthreadCommDlgHookProc(HWND hDlg, WORD wMsg, WORD wParam,
                                          DWORD lParam)
{
MSG msg;



   SET_CURRENT_TASK_INDEX;       /* assigns value to 'current_task_index' */
   if(NO_TASK_ENTRY) return(FALSE);              /*** ERROR ERROR ERROR ***/

   if(!NO_TASK_ENTRY && wInternalThreadCount)
   {
      if(!wInternalThreadIndex)
      {
         NearThreadDispatch(NULL, NULL, 3);  /* like 'MthreadGetMessage()' */
      }
      else
      {
         _asm nop                /* so I can set a breakpoint - for testing. */

         NearThreadDispatch(NULL, NULL, 2);  /* signal it's from HOOK PROC! */
      }
   }


   SET_CURRENT_TASK_INDEX;
   if(NO_TASK_ENTRY) return(FALSE);              /*** ERROR ERROR ERROR ***/

   if(!PeekMessage((LPMSG)&msg, (HWND)NULL, THREAD_IDLE, THREAD_IDLE,
                    PM_NOREMOVE | PM_NOYIELD))
   {
      mtIdleMessage = TRUE; /* allow another 'Idle' message to be posted */
   }
   else
   {
      mtIdleMessage = FALSE;
   }

   return(FALSE);

}



/***************************************************************************/
/*                        THREAD UTILITY FUNCTIONS                         */
/*                                                                         */
/* MthreadKillThread, MthreadSuspendThread, MthreadResumeThread, MthreadWaitForThread, etc. etc. etc.  */
/*                                                                         */
/***************************************************************************/


// 'MthreadSleep(n)' will wait for 'n' milliseconds, approximately.

void FAR PASCAL _MthreadSleep(UINT uiTime)
{
HANDLE FAR *lpThreads, hMyself;
LPINTERNALTHREAD lpIT;
DWORD dwStartTime;


   SET_CURRENT_TASK_INDEX;                     /* one more time! */
   if(NO_TASK_ENTRY)
   {
      return;  /*** ERROR! ***/
   }

   if(!wInternalThreadCount || !wInternalThreadIndex) return;

   dwStartTime = GetTickCount();


   do
   {
      lpThreads = (HANDLE FAR *)GlobalLock(hInternalThreadList);
      lpIT = (LPINTERNALTHREAD)GlobalLock(lpThreads[wInternalThreadIndex]);

      lpIT->wStatus |= (THREADSTATUS_SLEEP | THREADSTATUS_SLEEPWAIT);

      GlobalUnlock(lpThreads[wInternalThreadIndex]);
      GlobalUnlock(hInternalThreadList);

      if(LoopDispatch()) break;

   } while(uiTime > (DWORD)(GetTickCount() - dwStartTime));



   lpThreads = (HANDLE FAR *)GlobalLock(hInternalThreadList);
   lpIT = (LPINTERNALTHREAD)GlobalLock(lpThreads[wInternalThreadIndex]);

   lpIT->wStatus &= ~(THREADSTATUS_SLEEP | THREADSTATUS_SLEEPWAIT);

   GlobalUnlock(lpThreads[wInternalThreadIndex]);
   GlobalUnlock(hInternalThreadList);

}


BOOL FAR PASCAL _MthreadKillThread(HANDLE hThreadID)
{
HANDLE FAR *lpThreads, hMyself;
LPINTERNALTHREAD lpIT;
int i, j;



   SET_CURRENT_TASK_INDEX;                     /* one more time! */
   if(NO_TASK_ENTRY)
   {
      return(TRUE);  /*** ERROR! ***/
   }

   lpThreads = (HANDLE FAR *)GlobalLock(hInternalThreadList);  /* yahoo! get the list */


                        /* am I committing suicide?? */

   hMyself = lpThreads[wInternalThreadIndex];

   if(hMyself != hThreadID)
   {
                   /* no, I am *NOT* killing myself */

             /* move the thread to be killed so that it    */
             /* will be the *NEXT* to run, without causing */
             /* a disturbance in the order of the QUEUE.   */

      for(i=1; i<(int)wInternalThreadCount; i++)
      {
         if(lpThreads[i]==hThreadID)
         {
            break;   /* found! */
         }
      }

      if(i>=(int)wInternalThreadCount)   /* that means I didn't find it */
      {
         GlobalUnlock(hInternalThreadList);

         return(TRUE);                     /* unable to 'Kill' the thread */
      }

          /* using the index 'i' move the entry as needed to    */
          /* place it *before* the current thread, then cause   */
          /* it to execute next after the next 'LoopDispatch()' */


      if(!wInternalThreadIndex)      /* thread #0 - the one to die must */
      {                              /* be moved to the end!!           */

         for(j=i; j < ((int)wInternalThreadCount - 1); j++)
         {
            lpThreads[j] = lpThreads[j + 1];
         }

         wNextInternalThread = wInternalThreadCount - 1;
                                    /* that one will be the next to run. */

         lpThreads[wNextInternalThread] = hThreadID;
                                    /* The one I am gonna kill is *NEXT* */

      }
      else if(i>(int)wInternalThreadIndex)
      {                             /* thread to kill is 'after' current */

               /* need to make 'killed' thread run next,  */
               /* followed by this same thread afterwards */

         for(j=i; j>(int)wInternalThreadIndex; j--)
         {
            lpThreads[j] = lpThreads[j - 1];
         }

            /* the current thread is now one HIGHER in the array */

         lpThreads[wInternalThreadIndex] = hThreadID;

         wNextInternalThread = wInternalThreadIndex;

         wInternalThreadIndex++;  /* I moved it up by one - update value */

      }
      else                         /* thread to kill is 'before' current */
      {
         /* place thread to kill *immediately* before the current one */

         for(j=i; j < (int)(wInternalThreadIndex - 1); j++)
         {
            lpThreads[j] = lpThreads[j + 1];
         }

         wNextInternalThread = wInternalThreadIndex - 1;  /* next to run */

         lpThreads[wNextInternalThread] = hThreadID;   /* place in array */

      }
   }
   else
   {
      wNextInternalThread = wInternalThreadIndex;
   }

   lpIT = (LPINTERNALTHREAD)GlobalLock(hThreadID);  /* get pointer to thread information */

   lpIT->wStatus |= THREADSTATUS_KILL;          /* set the 'KILL' flag */

   GlobalUnlock(hThreadID);
   GlobalUnlock(hInternalThreadList);  /* that's all, folks! */

   return(FALSE);  /* it worked! */
}


BOOL FAR PASCAL _MthreadSuspendThread(HANDLE hThreadID)
{
HANDLE FAR *lpThreads;
LPINTERNALTHREAD lpIT;
int i;



   SET_CURRENT_TASK_INDEX;                     /* one more time! */
   if(NO_TASK_ENTRY)
   {
      return(TRUE);  /*** ERROR! ***/
   }

   lpThreads = (HANDLE FAR *)GlobalLock(hInternalThreadList);  /* yahoo! get the list */

   if(lpThreads[wInternalThreadIndex]==hThreadID)
   {
      lpIT = (LPINTERNALTHREAD)GlobalLock(lpThreads[wInternalThreadIndex]);/* CURRENT thread */
      i = wInternalThreadIndex;
   }
   else
   {
      for(i=1; i<(int)wInternalThreadCount; i++)
      {
         if(lpThreads[i]==hThreadID)
         {
            break;
         }
      }

      if(i>=(int)wInternalThreadCount)   /* that means I didn't find it */
      {
         GlobalUnlock(hInternalThreadList);

         return(TRUE);                   /* unable to 'suspend' the thread */
      }
   }

   lpIT = (LPINTERNALTHREAD)GlobalLock(hThreadID);  /* get pointer to thread information */

   lpIT->wStatus |= THREADSTATUS_SUSPEND;        /* set the 'SUSPEND' flag */

   GlobalUnlock(hThreadID);
   GlobalUnlock(hInternalThreadList);  /* that's all, folks! */

   return(FALSE);  /* it worked! */
}


BOOL FAR PASCAL _MthreadResumeThread(HANDLE hThreadID)
{
HANDLE FAR *lpThreads;
LPINTERNALTHREAD lpIT;
int i;



   SET_CURRENT_TASK_INDEX;                     /* one more time! */
   if(NO_TASK_ENTRY)
   {
      return(TRUE);  /*** ERROR! ***/
   }

   lpThreads = (HANDLE FAR *)GlobalLock(hInternalThreadList);  /* yahoo! get the list */

   if(lpThreads[wInternalThreadIndex]==hThreadID)
   {
      lpIT = (LPINTERNALTHREAD)GlobalLock(lpThreads[wInternalThreadIndex]);/* CURRENT thread */
      i = wInternalThreadIndex;
   }
   else
   {
      for(i=1; i<(int)wInternalThreadCount; i++)
      {
         if(lpThreads[i]==hThreadID)
         {
            break;
         }
      }

      if(i>=(int)wInternalThreadCount)   /* that means I didn't find it */
      {
         GlobalUnlock(hInternalThreadList);

         return(TRUE);                   /* unable to 'Resume' the thread */
      }
   }

   lpIT = (LPINTERNALTHREAD)GlobalLock(hThreadID);  /* get pointer to thread information */

   lpIT->wStatus &= ~THREADSTATUS_SUSPEND;    /* clear the 'suspend' flag */

   GlobalUnlock(hThreadID);
   GlobalUnlock(hInternalThreadList);  /* that's all, folks! */

   return(FALSE);  /* it worked! */
}


BOOL FAR PASCAL _MthreadWaitForThread(HANDLE hThreadID)
{
HANDLE FAR *lpThreads;
LPINTERNALTHREAD lpIT;



   SET_CURRENT_TASK_INDEX;                     /* one more time! */
   if(NO_TASK_ENTRY)
   {
      return(TRUE);  /*** ERROR! ***/
   }

   if(!hInternalThreadList) return(FALSE);  /* no threads! */


   if(!IsThreadHandle(hThreadID)) return(FALSE);/* it's not a thread */


    /* wait until thread terminates.  returns TRUE if 'QUIT' received */

   if(wInternalThreadIndex)
   {
      lpThreads = (HANDLE FAR *)GlobalLock(hInternalThreadList);

      lpIT = (LPINTERNALTHREAD)GlobalLock(lpThreads[wInternalThreadIndex]);   /* CURRENT thread */


      lpIT->wStatus |= THREADSTATUS_WAITTHREAD;

      lpIT->hWaitThread = hThreadID;

      GlobalUnlock(lpThreads[wInternalThreadIndex]);
      GlobalUnlock(hInternalThreadList);

      return(LoopDispatch());              /* isn't that simple?  Gawsh... */
   }
   else                    /* we are currently calling this from thread #0 */
   {
      while(IsThreadHandle(hThreadID))
      {
         if(LoopDispatch()) return(TRUE);
      }
   }

   return(FALSE);                      /* only gets here if it's thread #0 */
}



/***************************************************************************/
/*                         MthreadAppMessageProc()                         */
/*                                                                         */
/* This next proc is executed whenever an APPLICATION message needs to be  */
/* processed.  It is passed the correct value for 'current_task_index'.    */
/* If message is processed, it returns TRUE; otherwise it returns FALSE.   */
/*                                                                         */
/***************************************************************************/


#pragma code_seg()

BFPPROC _EXPORT_ MthreadAppMessageProc(LPMSG msg /* , int current_task_index */)
{

             /**********************************************/
             /*          APPLICATION MESSAGES              */
             /**********************************************/


    switch(msg->message)  /* see if it's a thread message! */
    {

                  /*** APPLICATION MESSAGES ***/

       case NO_MASTER_THREAD:
//          RootApp.hInst = (HANDLE)NULL;
//          RootApp.hWnd  = (HANDLE)NULL;
//          RootApp.hTask = (HANDLE)NULL;

          break;     /* this says 'there was a master thread, but */
                     /* now it ain't there no more!!!             */

       case THREAD_REGISTER:

       case THREAD_ACKNOWLEDGE:   /* always received by root app! */

       case THREAD_COMPLETE:

       case WM_SEMAPHORECALLBACK:

          MthreadSemaphoreCallback((HANDLE)msg->wParam,
                                   (SEMAPHORECALLBACK)msg->lParam);
          return(TRUE);                 /* 'I handled this message too' */


       default:
          return(FALSE);

    }
    return(TRUE);                    /* this says 'I handled this message' */
}




/***************************************************************************/
/*                                                                         */
/*            SEMAPHORE PROCEDURES and STATIC VARIABLES/TABLES             */
/*                                                                         */
/***************************************************************************/


                   /** STATIC VARIABLES FOR SEMAPHORES **/

static char semtable[MAX_SEMAPHORES][16]={0};
static HTASK semtask[MAX_SEMAPHORES] = {0};
static WORD semflags[MAX_SEMAPHORES] = {0};
static WORD semstatus[MAX_SEMAPHORES] = {0};

static SEMAPHORECALLBACK semproc[MAX_SEMAPHORECALLBACKS];
                                               /* proc-instance addresses! */
static HANDLE hsemproc_task[MAX_SEMAPHORECALLBACKS];
                                           /* task handles for 'semproc's! */
static HANDLE hsemproc_semaphore[MAX_SEMAPHORECALLBACKS];
                                        /* semaphores that cause callback! */
static WORD wsemproc_eventid[MAX_SEMAPHORECALLBACKS];
                                   /* 'id' values for semaphore callbacks! */




#pragma code_seg ("SEMAPHORE_TEXT","CODE")

BFPPROC _MthreadCreateSemaphore(LPSTR lpSemName, WORD wFlags)
{
int i, first_null = -1;
char buf[18];
HTASK hTask;


   _fmemset((LPSTR)buf, 0, sizeof(buf));
   _fstrncpy((LPSTR)buf, lpSemName, 16);
   _fstrupr((LPSTR)buf);

   hTask = GetCurrentTask();

   for(i=0; i<MAX_SEMAPHORES; i++)
   {
      if(semtable[i][0]==0)
      {
         if(first_null<0) first_null = i;
      }
      else if(_fstrncmp((LPSTR)buf, semtable[i], sizeof(semtable[i]))==0 &&
              (!semtask[i] || semtask[i]==hTask ))
      {
         return(1); /* can't create an already existing semaphore! */
      }
   }

   if(first_null<0)      /* no more room for semaphores! */
   {
      return(2);         /* appropriate error code for this */
   }

                    /* assign the value to the table!! */
   _fmemcpy((LPSTR)semtable[first_null], (LPSTR)buf, sizeof(*semtable));

   semflags[first_null] = wFlags;
   semstatus[first_null] = 0;     /* bit 15 is set when semaphore set */
                                  /* bits 0-14 are the reference count! */

   if(wFlags && SEMAPHORESTYLE_LOCAL)
   {
      semtask[first_null] = hTask;
   }
   else
   {
      semtask[first_null] = NULL;     /* a 'NULL' task handle ==> GLOBAL */
   }

   return(FALSE);        /* o.k. - good result! */

}

BFPPROC _MthreadKillSemaphore(LPSTR lpSemName)
{
int i, j, k;
char buf[18];
HTASK hTask;


   _fmemset((LPSTR)buf, 0, sizeof(buf));
   _fstrncpy((LPSTR)buf, lpSemName, 16);
   _fstrupr((LPSTR)buf);

   hTask = GetCurrentTask();

   for(i=0; i<MAX_SEMAPHORES; i++)
   {
      if(_fstrncmp((LPSTR)buf, semtable[i], sizeof(semtable[i]))==0 &&
         (!semtask[i] || semtask[i] == hTask))
      {
         if(semstatus[i] & 0x7fff)    /* there is a reference count! */
            return(1);      /* can't delete a 'referenced' semaphore! */

               /* delete semaphore and all references to it! */

         _fmemset(semtable[i], 0, sizeof(semtable[i]));

         semflags[i] = 0;
         semstatus[i] = 0;

         for(j=0, k=i+SEM_HANDLE_OFFSET; j<MAX_SEMAPHORECALLBACKS; j++)
         {
            if((int)hsemproc_semaphore[j]==k)     /* converted index to handle */
            {                       /* compare with callback table entries */

               semproc[j] = (SEMAPHORECALLBACK)NULL;
               hsemproc_semaphore[j] = (HANDLE)NULL;
               hsemproc_task[j] = (HANDLE)NULL;

               wsemproc_eventid[j] = (WORD)NULL;
            }
         }
         return(FALSE);
                    /* everything's o.k. now, so bail out with exit code 0 */
      }
   }

   return(2);      /* semaphore wasn't found, so return with error code #2 */
}



HANDLE FAR PASCAL _MthreadOpenSemaphore(LPSTR lpSemName)
{
int i;
char buf[18];
HTASK hTask;


   /* returned handle is offset by SEM_HANDLE_OFFSET to ensure NULL=error */

   _fmemset((LPSTR)buf, 0, sizeof(buf));
   _fstrncpy((LPSTR)buf, lpSemName, 16);
   _fstrupr((LPSTR)buf);

   hTask = GetCurrentTask();

   for(i=0; i<MAX_SEMAPHORES; i++)
   {
      if(_fstrncmp((LPSTR)buf, semtable[i], sizeof(semtable[i]))==0 &&
         (!semtask[i] || semtask[i] == hTask))
      {
         semstatus[i] = ((semstatus[i] & 0x7fff) + 1)
                        | (semstatus[i] & 0x8000);  /* increment ref count */

         return((HANDLE)(i + SEM_HANDLE_OFFSET));      /* the handle!! */
      }
   }

   return(NULL);  /* couldn't open the semaphore!! */
}


HANDLE FAR PASCAL _MthreadCloseSemaphore(HANDLE hSemaphore)
{
int i;

    if(hSemaphore==(HANDLE)NULL)  return((HANDLE)-1);  /* null handle! */

    i = (WORD)hSemaphore - SEM_HANDLE_OFFSET;

    if(i<0 || i>=MAX_SEMAPHORES)  /* out of range!!! */
       return((HANDLE)1);                 /* bad handle error */

    if(semstatus[i] & 0x7fff)     /* there is a reference count! */
    {
       semstatus[i] = ((semstatus[i] & 0x7fff) - 1)
                      | (semstatus[i] & 0x8000);    /* decrement ref count */

       return(0);
    }
    else
       return((HANDLE)2);  /* handle is valid, but reference count is bad! */

}





BFPPROC _EXPORT_ LOADDS MthreadSemaphoreCallback(HANDLE hSemaphore,
                                               SEMAPHORECALLBACK lpProc)
{
      /* this function must be called to dispatch to semaphore callback */
int i;
HTASK hTask;
HINSTANCE hInst;
BOOL Rval;
WORD wOldDS, wNewDS, wEventID, wSemValue;


   hTask = GetCurrentTask();
   hInst = GetCurrentInstance();

   for(i=0; i<MAX_SEMAPHORECALLBACKS; i++)
   {
      if(hsemproc_semaphore[i]==hSemaphore &&
         hsemproc_task[i]==hTask &&
         semproc[i]==lpProc)
      {

         wSemValue = (semstatus[i] & 0x8000)!=0;
         wEventID  = wsemproc_eventid[i];

         _asm mov wOldDS, ds

         wNewDS = HIWORD(GlobalLock(hInst));  /* get the program's DS */

         _asm mov ds, wNewDS           /* assign this DS to me! */

         Rval = lpProc(hSemaphore, wEventID, wSemValue);

         _asm mov ds, wOldDS           /* restore original DS */

         GlobalUnlock(hInst);          /* restore its lock count */

         return(Rval);
      }
   }

   return(TRUE);  /* error! */
}




static BFPPROC SemaphoreCallback(HANDLE hSemaphore, BOOL SetFlag)
{
int i;
HINSTANCE hInst;
LPSTR lpOwnerDS;
BOOL was_callback=FALSE;
MSG m;


   /** since the semaphore callback functions are possibly in another **/
   /** task it will be necessary to spawn them using a means other    **/
   /** than just CALLING them directly.  The 'WM_SEMAPHORECALLBACK    **/
   /** message is for this purpose.                                   **/

   for(i=0; i<MAX_SEMAPHORECALLBACKS; i++)
   {
      if(hsemproc_semaphore[i]==hSemaphore &&
         hsemproc_task[i]!=(HANDLE)NULL &&
         semproc[i]!=(SEMAPHORECALLBACK)NULL)
      {
         was_callback=TRUE;
         PostAppMessage((HTASK)hsemproc_task[i], WM_SEMAPHORECALLBACK,
                        (WPARAM)hSemaphore, (LPARAM)semproc[i]);
      }
   }

   if(was_callback)   /* if there was a callback message, YIELD!!! */
   {
      if(IsThread())
         LoopDispatch();
      else
         PeekMessage(&m, NULL, NULL, NULL, PM_NOREMOVE);
   }

   return(was_callback);             /* this says 'I processed something' */
}




BFPPROC _MthreadSetSemaphore(HANDLE hSemaphore)
{
int i;

    if(hSemaphore==(HANDLE)NULL)  return(-1);  /* null handle! */

    i = (WORD)hSemaphore - SEM_HANDLE_OFFSET;

    if(i<0 || i>=MAX_SEMAPHORES)    /* out of range!!! */
       return(1);                   /* bad handle error */

    if(semstatus[i] & 0x7fff)       /* there is a reference count! */
    {
       semstatus[i] |= 0x8000;      /* sets the semaphore! */

       SemaphoreCallback(hSemaphore, TRUE);
                                    /* call the callback functions! */

       return(0);
    }
    else
       return(2);  /* handle is valid, but reference count is bad! */

}


BFPPROC _MthreadClearSemaphore(HANDLE hSemaphore)
{
int i;

    if(hSemaphore==(HANDLE)NULL)  return(0xffff);  /* null handle! */

    i = (WORD)hSemaphore - SEM_HANDLE_OFFSET;

    if(i<0 || i>=MAX_SEMAPHORES)    /* out of range!!! */
       return(1);                   /* bad handle error */

    if(semstatus[i] & 0x7fff)      /* there is a reference count! */
    {
       semstatus[i] &= 0x7fff;      /* clears the semaphore! */

       SemaphoreCallback(hSemaphore, TRUE);
                                    /* call the callback functions! */

       return(0);
    }
    else
       return(2);  /* handle is valid, but reference count is bad! */

}


               /** THIS FUNCTION WILL WILL WILL YIELD!!! **/
           /** THEREFORE IT WILL WILL WILL LOCK OWNER's DS! **/


BFPPROC _WaitSemaphore(HANDLE hSemaphore, BOOL wState)
{
HINSTANCE hInst;
LPSTR lpOwnerDS;
BOOL rval = 0;


   if(MthreadGetSemaphore(hSemaphore)==0xffff)  /* this means 'bad semaphore'! */
   {
      return(1);
   }

   hInst = GetCurrentInstance();
   lpOwnerDS = (LPSTR)GlobalLock(hInst);       /* lock the owner's data segment! */

   while((MthreadGetSemaphore(hSemaphore)!=0)!=(wState!=0))
   {
      if(LoopDispatch())         /* calls this function to ensure 'yield' */
      {
         rval = 2;        /* getting a 'WM_QUIT' message constitues error */
         break;
      }

   }

   if(lpOwnerDS!=(LPSTR)NULL) GlobalUnlock(hInst);  /* unlock owner's DS */

   return(rval);        /* if everything's ok, just exit with a 0 return! */

}



BFPPROC _MthreadGetSemaphore(HANDLE hSemaphore)
{
int i;

    if(hSemaphore==(HANDLE)NULL)  return(0xffff);  /* null handle! */

    i = (WORD)hSemaphore - SEM_HANDLE_OFFSET;

    if(i<0 || i>=MAX_SEMAPHORES)    /* out of range!!! */
       return(0xffff);              /* bad handle error */

    if(semstatus[i] & 0x7fff)       /* there is a reference count! */
    {
       return((semstatus[i] & 0x8000)!=0);
    }
    else
       return(0xffff);              /* bad reference count! */

}



 /* register a semaphore proc to be called when semaphore changes states! */


BFPPROC _RegisterSemaphoreProc(HANDLE hSemaphore,
                              SEMAPHORECALLBACK lpProc, WORD wEventID)
{
int i;
HTASK hTask;


   hTask = GetCurrentTask();

       /** You cannot have multiple semaphore/callback entries **/
       /** check if the semaphore/callback combo is already    **/
       /** registered, and return an error if so!!             **/

   for(i=0; i<MAX_SEMAPHORECALLBACKS; i++)
   {
      if(hsemproc_semaphore[i]==hSemaphore &&
         hsemproc_task[i]==hTask &&
         semproc[i]==lpProc)
      {
         return(TRUE);  /* the semaphore proc is already registered!! */
      }
   }


   for(i=0; i<MAX_SEMAPHORECALLBACKS; i++)
   {
      if(hsemproc_semaphore[i]==(HANDLE)NULL &&
         hsemproc_task[i]==(HANDLE)NULL &&
         semproc[i]==(SEMAPHORECALLBACK)NULL)
      {
         semproc[i] = lpProc;
         hsemproc_semaphore[i] = hSemaphore;
         hsemproc_task[i] = hTask;

         wsemproc_eventid[i] = wEventID;

         return(FALSE);
      }
   }

   return(TRUE);

}


    /* when removing semaphores - a NULL in any arg assumes ALL, except */
    /* for 'wEventID' - a '0xffff' is the wildcard for this one!        */


BFPPROC _RemoveSemaphoreProc(HANDLE hSemaphore,
                            SEMAPHORECALLBACK lpProc, WORD wEventID)
{
int i;
HTASK hTask;
BOOL did_something = FALSE;


   hTask = GetCurrentTask();  /* always applies to current task only!! */

   for(i=0; i<MAX_SEMAPHORECALLBACKS; i++)
   {
      if((hsemproc_semaphore[i]==hSemaphore || hSemaphore==(HANDLE)NULL) &&
         hsemproc_task[i]==hTask &&
         (wsemproc_eventid[i]==wEventID || wEventID==0xffff) &&
         (semproc[i]==lpProc || lpProc==(SEMAPHORECALLBACK)NULL))
      {
         semproc[i] = (SEMAPHORECALLBACK)NULL;
         hsemproc_semaphore[i] = (HANDLE)NULL;
         hsemproc_task[i] = (HANDLE)NULL;

         wsemproc_eventid[i] = (WORD)NULL;

         did_something = TRUE;
      }
   }


   return(!did_something);   /* TRUE if none found; FALSE otherwise */

}


#ifdef DISPATCH_WITHIN_HOOK

#pragma code_seg ("WEP_TEXT","CODE")

BOOL EXPORT API LOADDS DispatchWithinHookTimerProc(HWND hWnd, WORD wMsg,
                                                   WPARAM wParam,
                                                   LPARAM lParam)
{
int i;
MSG msg;
LPINTERNALTHREAD lpIT;
HANDLE FAR *lpThreads;



   // added 10/1/92 - attempt to remove 'excess' WM_TIMER messages

   if(PeekMessage(&msg, NULL, WM_TIMER, WM_TIMER, PM_NOREMOVE | PM_NOYIELD))
   {
      i = 0;

      while(// msg.wParam==(WPARAM)MTH_TIMER_ID ||   // ID for the timer I set!!
            (DWORD)msg.lParam == (DWORD)lpDispatchWithinHookTimerProc)
      {
         PeekMessage(&msg, NULL, WM_TIMER, WM_TIMER, PM_REMOVE | PM_NOYIELD);
         if((++i > 32)) break;

         if(!PeekMessage(&msg, NULL, WM_TIMER, WM_TIMER,
                         PM_NOREMOVE | PM_NOYIELD)) break;
      }
   }



   lpThreads = (HANDLE FAR *)GlobalLock(hInternalThreadList);

   for(i=0; i < (int)wInternalThreadCount; i++)
   {
      lpIT = (LPINTERNALTHREAD)GlobalLock(lpThreads[i]);

      if(lpIT->wStatus & THREADSTATUS_SLEEP)
      {
         lpIT->wStatus &= ~THREADSTATUS_SLEEPWAIT;
      }

      GlobalUnlock(lpThreads[i]);
   }

   GlobalUnlock(hInternalThreadList);

//
// THIS SECTION REMOVED DUE TO CONFLICT WITH WIN '95
//
//   if(GetInputState()) return(FALSE);  /* ignore if system input present */

   if(hTask && mtIdleMessage)
   {
//      if(!PeekMessage(&msg, NULL, THREAD_IDLE, THREAD_IDLE,
//                       PM_NOREMOVE | PM_NOYIELD)
//         && msg.hwnd==NULL)
//      {
         PostAppMessage(hTask, THREAD_IDLE, NULL, NULL);

         mtIdleMessage = FALSE;
//      }
   }

   return(FALSE);

}

#pragma code_seg()


#else /* DISPATCH_WITHIN_HOOK */

BOOL EXPORT API LOADDS DispatchWithinHookTimerProc(HWND hWnd, WORD wMsg,
                                                   WPARAM wParam,
                                                   LPARAM lParam)
{
   return(FALSE);
}

#endif




/***************************************************************************/
/*                                                                         */
/*       'UseExternalLibrary()' - proc dedicated to allowing WMTHREAD      */
/*                                to be loaded and used!                   */
/*                                                                         */
/***************************************************************************/


#pragma code_seg ("MTHREAD_INIT_TEXT","CODE")

BOOL (CALLBACK * PASCAL MthreadMsgDispatch)(LPMSG)                               = _MthreadMsgDispatch;
BOOL (FAR PASCAL * PASCAL LoopDispatch)(void)                                    = _LoopDispatch;
BOOL (FAR PASCAL * PASCAL BroadcastMessageToChildren)(HANDLE, WORD, WORD, DWORD) = NULL; //_BroadcastMessageToChildren;
void (FAR PASCAL * PASCAL RegisterCompactProc)(FARPROC)                          = _RegisterCompactProc;
void (FAR PASCAL * PASCAL RegisterAppMessageProc)(void (FAR PASCAL *)(LPMSG))    = _RegisterAppMessageProc;
BOOL (FAR PASCAL * PASCAL RegisterAccelerators)(HANDLE, HWND)                    = _RegisterAccelerators;
BOOL (FAR PASCAL * PASCAL RemoveAccelerators)(HANDLE, HWND)                      = _RemoveAccelerators;
void (FAR PASCAL * PASCAL EnableMDIAccelerators)(HWND)                           = _EnableMDIAccelerators;
void (FAR PASCAL * PASCAL DisableMDIAccelerators)(HWND)                          = _DisableMDIAccelerators;
void (FAR PASCAL * PASCAL EnableTranslateMessage)(void)                          = _EnableTranslateMessage;
void (FAR PASCAL * PASCAL DisableTranslateMessage)(void)                         = _DisableTranslateMessage;
void (FAR PASCAL * PASCAL EnableQuitMessage)(void)                               = NULL; //_EnableQuitMessage;
void (FAR PASCAL * PASCAL DisableQuitMessage)(void)                              = NULL; //_DisableQuitMessage;
void (FAR PASCAL * PASCAL EnableCloseMessage)(void)                              = NULL; //_EnableCloseMessage;
void (FAR PASCAL * PASCAL DisableCloseMessage)(void)                             = NULL; //_DisableCloseMessage;
void (FAR PASCAL * PASCAL MthreadDisableMessages)(void)                          = NULL; //_MthreadDisableMessages;
void (FAR PASCAL * PASCAL MthreadEnableMessages)(void)                           = NULL; //_MthreadEnableMessages;
BOOL (FPPROC * PASCAL MthreadDestroy)(HWND)                                      = NULL; //_MthreadDestroy;
HINSTANCE (FAR PASCAL * PASCAL GetCurrentInstance)(void)                         = _GetCurrentInstance;
HANDLE (FPPROC * PASCAL MthreadGetCurrentThread)(void)                                  = _MthreadGetCurrentThread;
WORD (FPPROC * PASCAL GetNumberOfThreads)(void)                                  = _GetNumberOfThreads;
HTASK (FAR PASCAL * PASCAL GetInstanceTask)(HINSTANCE)                           = _GetInstanceTask;
HINSTANCE (FAR PASCAL * PASCAL GetTaskInstance)(HTASK)                           = _GetTaskInstance;
WORD (FPPROC * PASCAL GetSegmentLimit)(WORD)                                     = _GetSegmentLimit;
BOOL (FPPROC * PASCAL IsHandleValid)(HGLOBAL)                                    = _IsHandleValid;
BOOL (FPPROC * PASCAL IsSelectorValid)(WORD)                                     = _IsSelectorValid;
BOOL (FPPROC * PASCAL IsSelectorReadable)(WORD)                                  = _IsSelectorReadable;
BOOL (FPPROC * PASCAL IsSelectorWritable)(WORD)                                  = _IsSelectorWritable;
BOOL (FPPROC * PASCAL IsCodeSelector)(WORD)                                      = _IsCodeSelector;
BOOL (FPPROC * PASCAL ThreadAcknowledge)(void)                                   = NULL; //_ThreadAcknowledge;
BOOL (FPPROC * PASCAL ThreadComplete)(LONG)                                      = _ThreadComplete;
WORD (FPPROC * PASCAL SpawnThread)(LPTHREADPROC, HANDLE, WORD)                   = _SpawnThread;
WORD (FPPROC * PASCAL SpawnThreadX)(LPTHREADPROC, HANDLE, WORD, LPSTR, WORD)     = _SpawnThreadX;
BOOL (FPPROC * PASCAL IsThread)(void)                                            = NULL; //_IsThread;
BOOL (FPPROC * PASCAL IsThreadHandle)(HANDLE)                                    = _IsThreadHandle;
BOOL (FPPROC * PASCAL MthreadKillThread)(HANDLE)                                        = _MthreadKillThread;
BOOL (FPPROC * PASCAL MthreadSuspendThread)(HANDLE)                                     = _MthreadSuspendThread;
BOOL (FPPROC * PASCAL MthreadResumeThread)(HANDLE)                                      = _MthreadResumeThread;
BOOL (FPPROC * PASCAL MthreadWaitForThread)(HANDLE)                                     = _MthreadWaitForThread;
HANDLE (FAR PASCAL * PASCAL GlobalHandleCopy)(HANDLE)                            = NULL; //_GlobalHandleCopy;
HANDLE (FAR PASCAL * PASCAL GlobalHandleCopy2)(HANDLE, WORD)                     = NULL; //_GlobalHandleCopy2;
LPSTR (FAR PASCAL * PASCAL GlobalPtrCopy)(LPSTR)                                 = NULL; //_GlobalPtrCopy;
LPSTR (FAR PASCAL * PASCAL GlobalPtrCopy2)(LPSTR, WORD)                          = NULL; //_GlobalPtrCopy2;
BOOL (FAR PASCAL * PASCAL MakeLocalBuffer)(void NEAR * FAR *, WORD)              = NULL; //_MakeLocalBuffer;
BOOL (FAR PASCAL * PASCAL FreeLocalBuffer)(void NEAR *)                          = NULL; //_FreeLocalBuffer;
BOOL (FPPROC * PASCAL MthreadCreateSemaphore)(LPSTR, WORD)                              = _MthreadCreateSemaphore;
BOOL (FPPROC * PASCAL MthreadKillSemaphore)(LPSTR)                                      = _MthreadKillSemaphore;
HANDLE (FAR PASCAL * PASCAL MthreadOpenSemaphore)(LPSTR)                                = _MthreadOpenSemaphore;
HANDLE (FAR PASCAL * PASCAL MthreadCloseSemaphore)(HANDLE)                              = _MthreadCloseSemaphore;
BOOL (FPPROC * PASCAL MthreadSetSemaphore)(HANDLE)                                      = _MthreadSetSemaphore;
BOOL (FPPROC * PASCAL MthreadClearSemaphore)(HANDLE)                                    = _MthreadClearSemaphore;
BOOL (FPPROC * PASCAL WaitSemaphore)(HANDLE, BOOL)                               = _WaitSemaphore;
BOOL (FPPROC * PASCAL MthreadGetSemaphore)(HANDLE)                                      = _MthreadGetSemaphore;
BOOL (FPPROC * PASCAL RegisterSemaphoreProc)(HANDLE, SEMAPHORECALLBACK, WORD)    = _RegisterSemaphoreProc;
BOOL (FPPROC * PASCAL RemoveSemaphoreProc)(HANDLE, SEMAPHORECALLBACK, WORD)      = _RemoveSemaphoreProc;
void (FPPROC * PASCAL MthreadSleep)(HGLOBAL)                                            = _MthreadSleep;


#if 0     /* for reference - 'import by name' (not ordinal) */

#define CBCHAR const char CODE_BASED

static CBCHAR szMthreadMsgDispatch[]         = "MthreadMsgDispatch";
static CBCHAR szLoopDispatch[]               = "LoopDispatch";
static CBCHAR szBroadcastMessageToChildren[] = "BroadcastMessageToChildren";
static CBCHAR szRegisterCompactProc[]        = "RegisterCompactProc";
static CBCHAR szRegisterAppMessageProc[]     = "RegisterAppMessageProc";
static CBCHAR szRegisterAccelerators[]       = "RegisterAccelerators";
static CBCHAR szRemoveAccelerators[]         = "RemoveAccelerators";
static CBCHAR szEnableMDIAccelerators[]      = "EnableMDIAccelerators";
static CBCHAR szDisableMDIAccelerators[]     = "DisableMDIAccelerators";
static CBCHAR szEnableTranslateMessage[]     = "EnableTranslateMessage";
static CBCHAR szDisableTranslateMessage[]    = "DisableTranslateMessage";
static CBCHAR szEnableQuitMessage[]          = "EnableQuitMessage";
static CBCHAR szDisableQuitMessage[]         = "DisableQuitMessage";
static CBCHAR szEnableCloseMessage[]         = "EnableCloseMessage";
static CBCHAR szDisableCloseMessage[]        = "DisableCloseMessage";
static CBCHAR szMthreadDisableMessages[]     = "MthreadDisableMessages";
static CBCHAR szMthreadEnableMessages[]      = "MthreadEnableMessages";
static CBCHAR szlpMthreadInit[]              = "lpMthreadInit";
static CBCHAR szMthreadDestroy[]             = "MthreadDestroy";
static CBCHAR szGetCurrentInstance[]         = "GetCurrentInstance";
static CBCHAR szMthreadGetCurrentThread[]           = "MthreadGetCurrentThread";
static CBCHAR szGetNumberOfThreads[]         = "GetNumberOfThreads";
static CBCHAR szGetInstanceTask[]            = "GetInstanceTask";
static CBCHAR szGetTaskInstance[]            = "GetTaskInstance";
static CBCHAR szGetSegmentLimit[]            = "GetSegmentLimit";
static CBCHAR szIsHandleValid[]              = "IsHandleValid";
static CBCHAR szIsSelectorValid[]            = "IsSelectorValid";
static CBCHAR szIsSelectorReadable[]         = "IsSelectorReadable";
static CBCHAR szIsSelectorWritable[]         = "IsSelectorWritable";
static CBCHAR szIsCodeSelector[]             = "IsCodeSelector";
static CBCHAR szThreadAcknowledge[]          = "ThreadAcknowledge";
static CBCHAR szThreadComplete[]             = "ThreadComplete";
static CBCHAR szSpawnThread[]                = "SpawnThread";
static CBCHAR szSpawnThreadX[]               = "SpawnThreadX";
static CBCHAR szIsThread[]                   = "IsThread";
static CBCHAR szIsThreadHandle[]             = "IsThreadHandle";
static CBCHAR szMthreadKillThread[]                 = "MthreadKillThread";
static CBCHAR szMthreadSuspendThread[]              = "MthreadSuspendThread";
static CBCHAR szMthreadResumeThread[]               = "MthreadResumeThread";
static CBCHAR szMthreadWaitForThread[]              = "MthreadWaitForThread";
static CBCHAR szGlobalHandleCopy[]           = "GlobalHandleCopy";
static CBCHAR szGlobalHandleCopy2[]          = "GlobalHandleCopy2";
static CBCHAR szGlobalPtrCopy[]              = "GlobalPtrCopy";
static CBCHAR szGlobalPtrCopy2[]             = "GlobalPtrCopy2";
static CBCHAR szMakeLocalBuffer[]            = "MakeLocalBuffer";
static CBCHAR szFreeLocalBuffer[]            = "FreeLocalBuffer";
static CBCHAR szMthreadCreateSemaphore[]            = "MthreadCreateSemaphore";
static CBCHAR szMthreadKillSemaphore[]              = "MthreadKillSemaphore";
static CBCHAR szMthreadOpenSemaphore[]              = "MthreadOpenSemaphore";
static CBCHAR szMthreadCloseSemaphore[]             = "MthreadCloseSemaphore";
static CBCHAR szMthreadSetSemaphore[]               = "MthreadSetSemaphore";
static CBCHAR szMthreadClearSemaphore[]             = "MthreadClearSemaphore";
static CBCHAR szWaitSemaphore[]              = "WaitSemaphore";
static CBCHAR szMthreadGetSemaphore[]               = "MthreadGetSemaphore";
static CBCHAR szRegisterSemaphoreProc[]      = "RegisterSemaphoreProc";
static CBCHAR szRemoveSemaphoreProc[]        = "RemoveSemaphoreProc";
static CBCHAR szMthreadSleep[]                      = "MthreadSleep";



static CBCHAR * CODE_BASED lpszProcNames[] = {
   (CBCHAR *)szMthreadMsgDispatch,         (CBCHAR *)szLoopDispatch,
   (CBCHAR *)szBroadcastMessageToChildren, (CBCHAR *)szRegisterCompactProc,
   (CBCHAR *)szRegisterAppMessageProc,     (CBCHAR *)szRegisterAccelerators,
   (CBCHAR *)szRemoveAccelerators,         (CBCHAR *)szEnableMDIAccelerators,
   (CBCHAR *)szDisableMDIAccelerators,     (CBCHAR *)szEnableTranslateMessage,
   (CBCHAR *)szDisableTranslateMessage,    (CBCHAR *)szEnableQuitMessage,
   (CBCHAR *)szDisableQuitMessage,         (CBCHAR *)szEnableCloseMessage,
   (CBCHAR *)szDisableCloseMessage,        (CBCHAR *)szMthreadDisableMessages,
   (CBCHAR *)szMthreadEnableMessages,      (CBCHAR *)szlpMthreadInit,
   (CBCHAR *)szMthreadDestroy,
   (CBCHAR *)szGetCurrentInstance,         (CBCHAR *)szMthreadGetCurrentThread,
   (CBCHAR *)szGetNumberOfThreads,         (CBCHAR *)szGetInstanceTask,
   (CBCHAR *)szGetTaskInstance,            (CBCHAR *)szGetSegmentLimit,
   (CBCHAR *)szIsHandleValid,              (CBCHAR *)szIsSelectorValid,
   (CBCHAR *)szIsSelectorReadable,         (CBCHAR *)szIsSelectorWritable,
   (CBCHAR *)szIsCodeSelector,             (CBCHAR *)szThreadAcknowledge,
   (CBCHAR *)szThreadComplete,             (CBCHAR *)szSpawnThread,
   (CBCHAR *)szSpawnThreadX,
   (CBCHAR *)szIsThread,                   (CBCHAR *)szIsThreadHandle,
   (CBCHAR *)szMthreadKillThread,                 (CBCHAR *)szMthreadSuspendThread,
   (CBCHAR *)szMthreadResumeThread,               (CBCHAR *)szMthreadWaitForThread,
   (CBCHAR *)szGlobalHandleCopy,           (CBCHAR *)szGlobalHandleCopy2,
   (CBCHAR *)szGlobalPtrCopy,              (CBCHAR *)szGlobalPtrCopy2,
   (CBCHAR *)szMakeLocalBuffer,            (CBCHAR *)szFreeLocalBuffer,
   (CBCHAR *)szMthreadCreateSemaphore,            (CBCHAR *)szMthreadKillSemaphore,
   (CBCHAR *)szMthreadOpenSemaphore,              (CBCHAR *)szMthreadCloseSemaphore,
   (CBCHAR *)szMthreadSetSemaphore,               (CBCHAR *)szMthreadClearSemaphore,
   (CBCHAR *)szWaitSemaphore,              (CBCHAR *)szMthreadGetSemaphore,
   (CBCHAR *)szRegisterSemaphoreProc,      (CBCHAR *)szRemoveSemaphoreProc,
   (CBCHAR *)szMthreadSleep                    };


#else

#define ORD_MthreadMsgDispatch           16
#define ORD_LoopDispatch                 17
#define ORD_BroadcastMessageToChildren   18
#define ORD_RegisterCompactProc          19
#define ORD_RegisterAppMessageProc       20
#define ORD_RegisterAccelerators         21
#define ORD_RemoveAccelerators           22
#define ORD_EnableMDIAccelerators        23
#define ORD_DisableMDIAccelerators       24
#define ORD_EnableTranslateMessage       25
#define ORD_DisableTranslateMessage      26
#define ORD_EnableQuitMessage            27
#define ORD_DisableQuitMessage           28
#define ORD_EnableCloseMessage           29
#define ORD_DisableCloseMessage          30
#define ORD_MthreadInit                  31
#define ORD_DefMthreadMasterProc         32
#define ORD_MthreadDestroy               33
#define ORD_MthreadExit                  34
#define ORD_GetRootApplication           35
#define ORD_GetRootInstance              36
#define ORD_GetCurrentInstance           37
#define ORD_RegisterMasterThreadWindow   38
#define ORD_GetMasterThreadWindow        39
#define ORD_GetMthreadWindow             40
#define ORD_IsThread                     41
#define ORD_GetInstanceWindow            42
#define ORD_GetInstanceTask              43
#define ORD_GetTaskInstance              44
#define ORD_GetSegmentLimit              45
#define ORD_IsSelectorValid              46
#define ORD_IsSelectorReadable           47
#define ORD_IsSelectorWritable           48
#define ORD_ThreadAcknowledge            49
#define ORD_ThreadComplete               50
#define ORD_SpawnThread                  51
#define ORD_MthreadDispatch              52
#define ORD_spaces                       53
#define ORD_strtrim                      54
#define ORD_larger                       55
#define ORD_iswap                        56
#define ORD_lswap                        57
#define ORD_atodate                      58
#define ORD_days                         59
#define ORD_Date                         60
#define ORD__lftoa                       61
#define ORD___multiGlobalAlloc           62
#define ORD___multiGlobalFree            63
#define ORD___multiGlobalLock            64
#define ORD___multiGlobalUnlock          65
#define ORD___multiGlobalWire            66
#define ORD___multiGlobalUnWire          67
#define ORD___multiLocalAlloc            68
#define ORD___multiLocalFree             69
#define ORD___multiLocalLock             70
#define ORD___multiLocalUnlock           71
#define ORD_GlobalHandleCopy             72
#define ORD_GlobalHandleCopy2            73
#define ORD_MakeLocalBuffer              74
#define ORD_FreeLocalBuffer              75
#define ORD___hmemcpy                    77
#define ORD___hmemswap                   78
#define ORD___hmemcmp                    79
#define ORD___hmemset                    80
#define ORD___lqsort                     81
#define ORD_MthreadCreateSemaphore              82
#define ORD_MthreadKillSemaphore                83
#define ORD_MthreadOpenSemaphore                84
#define ORD_MthreadCloseSemaphore               85
#define ORD_MthreadSetSemaphore                 86
#define ORD_MthreadClearSemaphore               87
#define ORD_MthreadGetSemaphore                 88
#define ORD_RegisterSemaphoreProc        89
#define ORD_RemoveSemaphoreProc          90
#define ORD_DBFntof                      91
#define ORD_DBFfton                      92
#define ORD___multiGlobalAllocDDE        93
#define ORD_MthreadGetCurrentThread             94
#define ORD_MthreadKillThread                   95
#define ORD_MthreadSuspendThread                96
#define ORD_MthreadResumeThread                 97
#define ORD_GetNumberOfThreads           98
#define ORD_MthreadDisableMessages       99
#define ORD_MthreadEnableMessages       100
#define ORD_DayOfWeek                   101
#define ORD___lbsearch                  102
#define ORD_IsThreadHandle              103
#define ORD_IsInternalThread            104
#define ORD_IsHandleValid               105
#define ORD_MthreadWaitForThread               106
#define ORD_GlobalPtrCopy               107
#define ORD_GlobalPtrCopy2              108
#define ORD_IsCodeSelector              109
#define ORD___multiGlobalAllocPtr       110
#define ORD___multiGlobalAllocPtrDDE    111
#define ORD___multiGlobalFreePtr        112
#define ORD___multiGlobalAllocPtrZ      113
#define ORD___multiGlobalAllocPtrDDEZ   114
#define ORD_WaitSemaphore               115
#define ORD_MthreadGetMessage           116
#define ORD_MthreadSemaphoreCallback    117
#define ORD___lqsort2                   118
#define ORD___lbsearch2                 119
#define ORD_GetSystemDateTime           120
#define ORD_SpawnThreadX                121
#define ORD_lstrltrim                   122
#define ORD_lstrrtrim                   123
#define ORD_atotime                     124
#define ORD_DateStr                     125
#define ORD_TimeStr                     126
#define ORD_DaysStr                     127
#define ORD_MyTranslate                 128
#define ORD_cvtld2dl                    129
#define ORD_cvtdl2ld                    130
#define ORD_RegisterDialogWindow        131
#define ORD_UnregisterDialogWindow      132
#define ORD_MthreadThreadCritical              133
#define ORD_MthreadThreadExclusive             134
#define ORD__hstrcpy                    135
#define ORD__hstrcmp                    136
#define ORD__hstricmp                   137
#define ORD__hstrncpy                   138
#define ORD__hstrncmp                   139
#define ORD__hstrnicmp                  140
#define ORD__hstrlen                    141
#define ORD__hatol                      142
#define ORD__hatof                      143
#define ORD__hatold                     144
#define ORD__hsprintf                   145
#define ORD_VBCopyPointerToString       190
#define ORD_VBFreeGlobalPointer         191
#define ORD_VBMemCopy                   192
#define ORD_VBGetAddress                193

#define ORD_MthreadSleep                       103  /* same as IsThreadHandle() */

#define ORD__l32memcpy                  301
#define ORD__l32memcmp                  302
#define ORD__l32memswap                 303
#define ORD__l32memset                  304
#define ORD__l32memmove                 305
#define ORD__l32qsort                   306
#define ORD__l32bsearch                 307
#define ORD__l32qsort_terminate         308
#define ORD__l32hsort                   309
#define ORD_l32read                     311
#define ORD_l32write                    312
#define ORD_l32LocalInit                321
#define ORD_l32LocalAlloc               322
#define ORD_l32LocalLock                323
#define ORD_l32LocalUnlock              324
#define ORD_l32LocalFree                325
#define ORD_l32LocalReAlloc             326
#define ORD_l32LocalFlags               327
#define ORD_l32LocalSize                328
#define ORD_l32LocalCompact             329
#define ORD_Are32BitSegmentsAvailable   380
#define ORD_StubGlobal16PointerAlloc    381
#define ORD_StubGlobal16PointerFree     382
#define ORD_StubGlobal32Alloc           383
#define ORD_StubGlobal32CodeAlias       384
#define ORD_StubGlobal32CodeAliasFree   385
#define ORD_StubGlobal32Free            386
#define ORD_StubGlobal32Realloc         387
#define ORD_StubGetWinMem32Version      388
#define ORD_MyGlobal16PointerAlloc      391
#define ORD_MyGlobal16PointerFree       392
#define ORD_MyGlobal32Alloc             393
#define ORD_MyGlobal32CodeAlias         394
#define ORD_MyGlobal32CodeAliasFree     395
#define ORD_MyGlobal32Free              396
#define ORD_MyGlobal32Realloc           397
#define ORD_MyGetWinMem32Version        398
#define ORD_DefMthreadCommDlgHookProc   999





static WORD CODE_BASED lpwProcOrdinals[] = {
   ORD_MthreadMsgDispatch,         ORD_LoopDispatch,
   ORD_BroadcastMessageToChildren, ORD_RegisterCompactProc,
   ORD_RegisterAppMessageProc,     ORD_RegisterAccelerators,
   ORD_RemoveAccelerators,         ORD_EnableMDIAccelerators,
   ORD_DisableMDIAccelerators,     ORD_EnableTranslateMessage,
   ORD_DisableTranslateMessage,    ORD_EnableQuitMessage,
   ORD_DisableQuitMessage,         ORD_EnableCloseMessage,
   ORD_DisableCloseMessage,        ORD_MthreadDisableMessages,
   ORD_MthreadEnableMessages,
   ORD_MthreadDestroy,
   ORD_GetCurrentInstance,         ORD_MthreadGetCurrentThread,
   ORD_GetNumberOfThreads,         ORD_GetInstanceTask,
   ORD_GetTaskInstance,            ORD_GetSegmentLimit,
   ORD_IsHandleValid,              ORD_IsSelectorValid,
   ORD_IsSelectorReadable,         ORD_IsSelectorWritable,
   ORD_IsCodeSelector,             ORD_ThreadAcknowledge,
   ORD_ThreadComplete,             ORD_SpawnThread,
   ORD_SpawnThreadX,
   ORD_IsThread,                   ORD_IsThreadHandle,
   ORD_MthreadKillThread,                 ORD_MthreadSuspendThread,
   ORD_MthreadResumeThread,               ORD_MthreadWaitForThread,
   ORD_GlobalHandleCopy,           ORD_GlobalHandleCopy2,
   ORD_GlobalPtrCopy,              ORD_GlobalPtrCopy2,
   ORD_MakeLocalBuffer,            ORD_FreeLocalBuffer,
   ORD_MthreadCreateSemaphore,            ORD_MthreadKillSemaphore,
   ORD_MthreadOpenSemaphore,              ORD_MthreadCloseSemaphore,
   ORD_MthreadSetSemaphore,               ORD_MthreadClearSemaphore,
   ORD_WaitSemaphore,              ORD_MthreadGetSemaphore,
   ORD_RegisterSemaphoreProc,      ORD_RemoveSemaphoreProc,
   ORD_MthreadSleep                       };

#endif

#define FPNP FARPROC NEAR *

static FPNP CODE_BASED lplpProcs[] = {
   (FPNP)&MthreadMsgDispatch,         (FPNP)&LoopDispatch,
   (FPNP)&BroadcastMessageToChildren, (FPNP)&RegisterCompactProc,
   (FPNP)&RegisterAppMessageProc,     (FPNP)&RegisterAccelerators,
   (FPNP)&RemoveAccelerators,         (FPNP)&EnableMDIAccelerators,
   (FPNP)&DisableMDIAccelerators,     (FPNP)&EnableTranslateMessage,
   (FPNP)&DisableTranslateMessage,    (FPNP)&EnableQuitMessage,
   (FPNP)&DisableQuitMessage,         (FPNP)&EnableCloseMessage,
   (FPNP)&DisableCloseMessage,        (FPNP)&MthreadDisableMessages,
   (FPNP)&MthreadEnableMessages,
   (FPNP)&MthreadDestroy,
   (FPNP)&GetCurrentInstance,         (FPNP)&MthreadGetCurrentThread,
   (FPNP)&GetNumberOfThreads,         (FPNP)&GetInstanceTask,
   (FPNP)&GetTaskInstance,            (FPNP)&GetSegmentLimit,
   (FPNP)&IsHandleValid,              (FPNP)&IsSelectorValid,
   (FPNP)&IsSelectorReadable,         (FPNP)&IsSelectorWritable,
   (FPNP)&IsCodeSelector,             (FPNP)&ThreadAcknowledge,
   (FPNP)&ThreadComplete,             (FPNP)&SpawnThread,
   (FPNP)&SpawnThreadX,
   (FPNP)&IsThread,                   (FPNP)&IsThreadHandle,
   (FPNP)&MthreadKillThread,                 (FPNP)&MthreadSuspendThread,
   (FPNP)&MthreadResumeThread,               (FPNP)&MthreadWaitForThread,
   (FPNP)&GlobalHandleCopy,           (FPNP)&GlobalHandleCopy2,
   (FPNP)&GlobalPtrCopy,              (FPNP)&GlobalPtrCopy2,
   (FPNP)&MakeLocalBuffer,            (FPNP)&FreeLocalBuffer,
   (FPNP)&MthreadCreateSemaphore,            (FPNP)&MthreadKillSemaphore,
   (FPNP)&MthreadOpenSemaphore,              (FPNP)&MthreadCloseSemaphore,
   (FPNP)&MthreadSetSemaphore,               (FPNP)&MthreadClearSemaphore,
   (FPNP)&WaitSemaphore,              (FPNP)&MthreadGetSemaphore,
   (FPNP)&RegisterSemaphoreProc,      (FPNP)&RemoveSemaphoreProc,
   (FPNP)&MthreadSleep                     };




BOOL (FPPROC *lpMthreadInit)(HANDLE hInst, HANDLE hPrev, LPSTR lpCmd,
                             WORD nCmdShow) = NULL;

void (FPPROC *lpMthreadExit)(HANDLE hTask)  = NULL;


#define FPORD(X) ((LPCSTR)MAKELP(0,(X)))


BOOL NEAR PASCAL UseExternalLibrary(HINSTANCE hInstance, HANDLE hPrev,
                              LPSTR lpCmd, WORD nCmdShow, BOOL FAR *lpRval)
{
int i;

#if 0
   (FARPROC &)lpMthreadInit = GetProcAddress(hWMTHREAD, "MthreadInit");
   (FARPROC &)lpMthreadExit = GetProcAddress(hWMTHREAD, "MthreadExit");
#else
   (FARPROC &)lpMthreadInit = GetProcAddress(hWMTHREAD, FPORD(ORD_MthreadInit));
   (FARPROC &)lpMthreadExit = GetProcAddress(hWMTHREAD, FPORD(ORD_MthreadExit));
#endif

   if(!lpMthreadInit || !lpMthreadExit)
   {
      MessageBox(NULL, "?Failure loading 'WMTHREAD.DLL'",
              "** WMTHREAD ERROR **", MB_OK | MB_ICONHAND | MB_SYSTEMMODAL);

      FreeLibrary(hWMTHREAD);

      lpMthreadInit = NULL;
      lpMthreadExit = NULL;

      hWMTHREAD = NULL;

      return(TRUE);
   }


#if 0

   for(i=0; i < (sizeof(lpszProcNames) / sizeof(*lpszProcNames)); i++)
   {
      *(lplpProcs[i]) = GetProcAddress(hWMTHREAD, (LPCSTR)lpszProcNames[i]);
   }

#else

   for(i=0; i < (sizeof(lpwProcOrdinals) / sizeof(*lpwProcOrdinals)); i++)
   {
      *(lplpProcs[i]) = GetProcAddress(hWMTHREAD, FPORD(lpwProcOrdinals[i]));
   }

#endif


   *lpRval = lpMthreadInit(hInstance, hPrev, lpCmd, nCmdShow);

   return(FALSE);

}
