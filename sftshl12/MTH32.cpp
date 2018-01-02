/***************************************************************************/
/*                                                                         */
/*         Win32 Multi-Thread Manager Utility Library - MTH32.LIB          */
/*                                                                         */
/*          'MTH32.C' - Multi-Thread Utilities and API FUNCTIONS           */
/*                                                                         */
/*     This is the WIN32 equivalent of the static MT lib for SFTSHELL      */
/*                                                                         */
/*                                                                         */
/*                      TRADE SECRET - CONFIDENTIAL                        */
/*                                                                         */
/*             The compiled libraries and API header files are             */
/*          (c) Copyright 1990-1994 by Stewart~Frazier Tools, Inc          */
/*                          all rights reserved                            */
/*                                                                         */
/***************************************************************************/

#ifndef WIN32
#error ?You must define WIN32, bozo!
#endif


#pragma warning(disable: 4001)

//#define DISPLAY_LOGO /* define this to display the 'nag' logo screen */


#define INCLUDE_EXTERN_VARS


#define WIN31                // STOLEN from '_mthread.h'
#define NO_EXTERNAL_VARS     /* this prevents defining 'extern' vars */
#include "_mth32.h"
#include "excpt.h"           // structured exception handling
#include "tlhelp32.h"
#include "mmsystem.h"
#include "winerror.h"        // error codes from 'GetLastError()'


#define INLINE __inline



// MACROS NORMALLY DEFINED IN '_mthread.h'

#ifdef __cplusplus
#define LVFARPROC FARPROC FAR &
#else
#define LVFARPROC FARPROC
#endif

#define MAKELPC(X,Y) ((LPCSTR)MAKELONG(Y,X))


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


#define IDM_SUSPEND  0x1000   /* for thread windows - '_MTHREAD_' class */
#define IDM_RESUME   0x2000



                             /*** MACROS ***/

#define __STACK _segname("_STACK")
#define INIT_LEVEL do{            /* these are used to 'clean up' commonly */
#define END_LEVEL }while(FALSE);       /* used 'hierarchichal' error traps */

#define HEX_DIGIT(X) (((X)>='0'&&(X)<='9')?((X)-'0'):\
                      (((X)>='A'&&(X)<='F')?((X)-'A'+10):\
                      (((X)>='a'&&(X)<='f')?((X)-'a'+10):-1)))




// SPECIAL STRUCTURE & CONSTANT FOR THREAD INFO (stored as T.L.S.)

typedef struct {

   FARPROC  lpProc;         // thread proc
   HANDLE   hArgs;          // thread arguments
   HANDLE   hCaller;        // caller's handle (must delete it)
   HANDLE   hSelf;          // handle to self (duplicated from CreateThread)
   HANDLE   hThread;        // handle to self (returned by CreateThread)
                            // NOTE:  *THIS* is the *OFFICIAL* handle!
   DWORD    idCaller;       // caller's ID
   DWORD    idSelf;         // thread's ID
   UINT     flags;          // flags passed to 'SpawnThread()'
   DWORD    dwStatus;       // status flags (suspended, etc.)
   HWND     hwndThread;     // 'thread window' handle (if one exists)
   void *   lpInstanceData; // pointer to 'Thread Instance Data'
   DWORD    retcode;        // return code if call 'MthreadExitThread()'

   } THREADPARM32, *LPTHREADPARM32;

typedef const THREADPARM32 *LPCTHREADPARM32;

static const char szTHREADPARM32[]="_THREADPARM32_";


#define SFT_THREAD_TERMINATE 0x20534654   /* Bit 30 set in high byte   */
                                          /* plus the characters 'SFT' */



// OTHER SPECIAL STRUCTURES ADDED FOR WIN32

typedef struct {
   HWND hWnd;
   HACCEL hAccel;
   } SFTACCELERATORS, *LPSFTACCELERATORS;




HINSTANCE hLibInst = NULL;           // instance for THIS library

static HMODULE hWMth32 = NULL;       // 32-bit Win32s multi-thread 'helper'
static HMODULE hKERNEL = NULL;       // KERNEL32 (should be loaded already)


static HWND hMthreadIDFocus = NULL;  // helps prevent 'focus' problems on
                                     // the 'brag' dialog screen
static HTASK htaskMthreadIDFocus = NULL;  // task for above window (or NULL)

static BOOL bInterruptRegisterFlag = FALSE;  // 'InterruptRegister()' retval

static BOOL bWin32sFlag = FALSE;          // 'Win32s' flag - TRUE for WIN32s

static DWORD dwTLSIndex = 0xffffffffL;    // 'Thread Local Storage' index


static HWND hMasterThreadWindow    = NULL;

static HWND *MDIAcceleratorsClient = NULL;
static BOOL MDIAcceleratorsEnable  = FALSE;

static HWND *DialogWindowList      = NULL;

static BOOL display_wdbutil_logo   = FALSE;

static BOOL TranslateEnable        = TRUE;

static DWORD disable_dispatch      = 0;

static DWORD disable_messages      = 0;

static BOOL was_quit               = FALSE;

static void (FAR PASCAL *CompactProc)(void)     = NULL;

static void (FAR PASCAL *AppMessageProc)(LPMSG) = NULL;


SYSTEM_INFO SysInfo;           // from 'GetSystemInfo()'
MEMORYSTATUS MemStat;          // from 'GetMemoryStatus()'

static DWORD dwVMem = 0;       // from 'VirtualAlloc()' (reserved space)
static DWORD dwVMemSize = 0;   // size of virtual memory


DWORD dwVersion = 0;

static LPSFTACCELERATORS Accelerators = NULL;





// THE FOLLOWING VARIABLES DEFINE THE CURRENT APPLICATION

static HINSTANCE hRootInst = NULL;    // instance (task) handle of app
static HANDLE hRootApp = NULL;        // 'Root' application thread
static DWORD idRootApp = 0;           // Thread ID for 'root app'
                                      // (use this for task messages)


// SPECIAL 'INTERNAL THREAD LIST' FOR THIS DLL

#define SFTMAXTHREADS 512

static UINT   uiThreadCount = 0;              // # of threads in table
static HANDLE lpThreadHandle[SFTMAXTHREADS];  // handles for these threads
static DWORD  lpThreadID[SFTMAXTHREADS];      // ID's for these threads
static LPVOID lpThreadTLS[SFTMAXTHREADS];     // TLS pointer for these threads

static const char szMutexName[]="SFT32_Mutex"; // mutex for any serialized
                                               // processes in this DLL
static HANDLE hMutex           = NULL;


// EXTERN variables defined in 'sft32asm.asm'

extern volatile DWORD __cdecl dwSharedInstanceCount;
extern volatile DWORD __cdecl dwSharedThreadTableCount;
extern volatile DWORD __cdecl pSharedThreadTable[0x400][4];




         /*******************************************************/
         /* EXPORTED PROC ADDRESSES FROM WMTH32.DLL (if needed) */
         /*******************************************************/

BOOL (FPPROC *lpMthreadMsgDispatch)(LPMSG)                            = NULL;
BOOL (FPPROC *lpLoopDispatch)(void)                                   = NULL;
BOOL (FPPROC *lpBroadcastMessageToChildren)(HWND,UINT,WPARAM,LPARAM)  = NULL;
void (FPPROC *lpRegisterCompactProc)(FARPROC)                         = NULL;
void (FPPROC *lpRegisterAppMessageProc)(void (FAR PASCAL *)(LPMSG))   = NULL;
BOOL (FPPROC *lpRegisterAccelerators)(HANDLE, HWND)                   = NULL;
BOOL (FPPROC *lpRemoveAccelerators)(HANDLE, HWND)                     = NULL;
void (FPPROC *lpEnableMDIAccelerators)(HWND)                          = NULL;
void (FPPROC *lpDisableMDIAccelerators)(HWND)                         = NULL;
void (FPPROC *lpEnableTranslateMessage)(void)                         = NULL;
void (FPPROC *lpDisableTranslateMessage)(void)                        = NULL;
void (FPPROC *lpEnableQuitMessage)(void)                              = NULL;
void (FPPROC *lpDisableQuitMessage)(void)                             = NULL;
void (FPPROC *lpEnableCloseMessage)(void)                             = NULL;
void (FPPROC *lpDisableCloseMessage)(void)                            = NULL;
BOOL (FPPROC *lpMthreadInit)(HANDLE,HANDLE,LPSTR,UINT)                = NULL;
long (FPPROC *lpDefMthreadMasterProc)(HWND,UINT,WPARAM,LPARAM)        = NULL;
void (FPPROC *lpMthreadExit)(HANDLE)                                  = NULL;
HANDLE (FPPROC *lpGetRootApplication)(void)                           = NULL;
HANDLE (FPPROC *lpGetRootInstance)(void)                              = NULL;
HINSTANCE (FPPROC *lpGetCurrentInstance)(void)                        = NULL;
HWND (FPPROC *lpRegisterMasterThreadWindow)(HWND)                     = NULL;
HWND (FPPROC *lpGetMasterThreadWindow)(void)                          = NULL;
HWND (FPPROC *lpGetMthreadWindow)(void)                               = NULL;
BOOL (FPPROC *lpIsThread)(void)                                       = NULL;
HWND (FPPROC *lpGetInstanceWindow)(HANDLE)                            = NULL;
HTASK (FPPROC *lpGetInstanceTask)(HANDLE)                             = NULL;
HINSTANCE (FPPROC *lpGetTaskInstance)(HANDLE)                         = NULL;
BOOL (FPPROC *lpThreadAcknowledge)(void)                              = NULL;
BOOL (FPPROC *lpSpawnThread)(LPTHREADPROC,HANDLE,UINT)                = NULL;
BOOL (FPPROC *lpMthreadCreateSemaphore)(LPSTR,WORD)                   = NULL;
BOOL (FPPROC *lpMthreadKillSemaphore)(LPSTR)                          = NULL;
BOOL (FPPROC *lpMthreadOpenSemaphore)(LPSTR)                          = NULL;
BOOL (FPPROC *lpMthreadCloseSemaphore)(HANDLE)                        = NULL;
BOOL (FPPROC *lpMthreadSetSemaphore)(HANDLE)                          = NULL;
BOOL (FPPROC *lpMthreadClearSemaphore)(HANDLE)                        = NULL;
BOOL (FPPROC *lpMthreadGetSemaphore)(HANDLE)                          = NULL;
BOOL (FPPROC *lpRegisterSemaphoreProc)(HANDLE,SEMAPHORECALLBACK,WORD) = NULL;
BOOL (FPPROC *lpRemoveSemaphoreProc)(HANDLE,SEMAPHORECALLBACK,WORD)   = NULL;
HANDLE (FPPROC *lpMthreadGetCurrentThread)(void)                      = NULL;
BOOL (FPPROC *lpMthreadKillThread)(HANDLE)                            = NULL;
BOOL (FPPROC *lpMthreadSuspendThread)(HANDLE)                         = NULL;
BOOL (FPPROC *lpMthreadResumeThread)(HANDLE)                          = NULL;
WORD (FPPROC *lpGetNumberOfThreads)(void)                             = NULL;
void (FPPROC *lpMthreadDisableMessages)(void)                         = NULL;
void (FPPROC *lpMthreadEnableMessages)(void)                          = NULL;
BOOL (FPPROC *lpIsThreadHandle)(HANDLE)                               = NULL;
BOOL (FPPROC *lpIsInternalThread)(HANDLE)                             = NULL;
BOOL (FPPROC *lpIsHandleValid)(HANDLE)                                = NULL;
BOOL (FPPROC *lpMthreadWaitForThread)(HANDLE)                         = NULL;
BOOL (FPPROC *lpMthreadWaitSemaphore)(HANDLE,BOOL)                    = NULL;
BOOL (FPPROC *lpMthreadGetMessage)(LPMSG)                             = NULL;
BOOL (FPPROC *lpMthreadSemaphoreCallback)(HANDLE,SEMAPHORECALLBACK)   = NULL;
BOOL (FPPROC *lpSpawnThreadX)(LPTHREADPROC,HANDLE,UINT,LPSTR,UINT)    = NULL;
BOOL (FPPROC *lpRegisterDialogWindow)(HWND)                           = NULL;
void (FPPROC *lpUnregisterDialogWindow)(HWND)                         = NULL;
BOOL (FPPROC *lpMthreadThreadCritical)(HANDLE,BOOL)                   = NULL;
BOOL (FPPROC *lpMthreadThreadExclusive)(HANDLE,BOOL)                  = NULL;
BOOL (FPPROC *lpAddThreadInstanceData)(HANDLE,LPCSTR,LPCSTR,DWORD)    = NULL;
DWORD (FPPROC *lpGetThreadInstanceData)(HANDLE,LPCSTR,LPSTR,DWORD)    = NULL;
LPCSTR (FPPROC *lpGetThreadInstanceDataPtr)(HANDLE,LPCSTR)            = NULL;
DWORD (FPPROC *lpRemoveThreadInstanceData)(HANDLE,LPCSTR,LPSTR,DWORD) = NULL;
BOOL (FPPROC *lpSetThreadInstanceData)(HANDLE,LPCSTR,LPCSTR,DWORD)    = NULL;
void (FPPROC *lpMthreadSleep)(UINT)                                   = NULL;
HWND (FPPROC *lpCreateThreadWindow)(HANDLE)                           = NULL;
BOOL (FPPROC *lpMthreadTranslateMessage)(LPMSG)                       = NULL;
LONG (FPPROC *lpCPlApplet)(HWND, UINT, LPARAM, LPARAM)                = NULL;
void (FPPROC *lpDisplayWDBUtilLogo)(void)                             = NULL;

BOOL (FPPROC *lpMthreadDialogBoxParam)(HINSTANCE, LPCSTR, HWND,
                                       DLGPROC, LPARAM)               = NULL;
void (FPPROC *lpSetHelperTaskPDB)(UINT)                               = NULL;
UINT (FPPROC *lpGetHelperTaskPDB)(void)                               = NULL;
BOOL (FPPROC *lpMthreadDialogBoxIndirectParam)(HINSTANCE, HANDLE, HWND,
                                               DLGPROC, LPARAM)       = NULL;
void (FPPROC *lpUpdateThreadPath)(HANDLE)                             = NULL;


// "TOOLHELP 32" FUNCTIONS (Chicago only??)

HANDLE (WINAPI *lpCreateToolhelp32Snapshot)(DWORD, DWORD)             = NULL;
BOOL   (WINAPI *lpHeap32ListFirst)(HANDLE, LPHEAPLIST32)              = NULL;
BOOL   (WINAPI *lpHeap32ListNext)(HANDLE, LPHEAPLIST32)               = NULL;
BOOL   (WINAPI *lpHeap32First)(LPHEAPENTRY32, DWORD, DWORD)           = NULL;
BOOL   (WINAPI *lpHeap32Next)(LPHEAPENTRY32)                          = NULL;
BOOL   (WINAPI *lpToolhelp32ReadProcessMemory)(DWORD, LPCVOID, LPVOID,
                                               DWORD, LPDWORD)        = NULL;
BOOL   (WINAPI *lpProcess32First)(HANDLE, LPPROCESSENTRY32)           = NULL;
BOOL   (WINAPI *lpProcess32Next)(HANDLE, LPPROCESSENTRY32)            = NULL;
BOOL   (WINAPI *lpThread32First)(HANDLE, LPTHREADENTRY32)             = NULL;
BOOL   (WINAPI *lpThread32Next)(HANDLE, LPTHREADENTRY32)              = NULL;
BOOL   (WINAPI *lpModule32First)(HANDLE, LPMODULEENTRY32)             = NULL;
BOOL   (WINAPI *lpModule32Next)(HANDLE, LPMODULEENTRY32)              = NULL;


void FAR PASCAL MyFatalAppExit(int iCode, LPCSTR lpcMsg);

INLINE BOOL NEAR PASCAL TranslateDialogTaskMessage(LPMSG msg);

LRESULT LOADDS CALLBACK MthreadWndProc(HWND hWnd, UINT msg,
                                       WPARAM wParam, LPARAM lParam);

BOOL FAR PASCAL MthreadAppMessageProc(LPMSG msg);

BOOL FAR PASCAL IsInternalThread(HANDLE hThread);

BOOL FPPROC MthreadMsgBoxProc(HWND hDlg, UINT message, WPARAM wParam,
                              LPARAM lParam);

long FAR PASCAL SpawnThreadProc(LPTHREADPARM32 lpParm);

long PASCAL MyThreadExceptionHandler(DWORD dwExceptionCode,
                                     PEXCEPTION_POINTERS lpXP,
                                     LPTHREADPARM32 lpParm);

BOOL FAR PASCAL LOADDS MyExceptionDlgProc(HWND hDlg, UINT msg,
                                                                   WPARAM wParam, LPARAM lParam);

BFPPROC EXPORT LOADDS MthreadSemaphoreCallback(HANDLE hSemaphore,
                                               SEMAPHORECALLBACK lpProc);

                         /* INLINE FUNCTIONS */

static __inline void MyOutputDebugString(LPCSTR szMsg)
{
#ifdef DEBUG
  OutputDebugString(szMsg);  // for now, this is all I'll do... 
                             // later, I can thunk to the 16-bit world!
#endif // DEBUG
}

__inline HANDLE MyDuplicateThreadHandle(HANDLE hItem)
{
HANDLE hRval;

   if(DuplicateHandle(GetCurrentProcess(), hItem,
                      GetCurrentProcess(), &hRval,
                      THREAD_ALL_ACCESS, FALSE, 0))
   {
      return(hRval);
   }

   if(DuplicateHandle(GetCurrentProcess(), hItem,
                      GetCurrentProcess(), &hRval,
                      0, FALSE, DUPLICATE_SAME_ACCESS))
   {
      return(hRval);
   }

   return(NULL);
}









static __inline void AddThreadEntry0(LPTHREADPARM32 lpParm)
{
UINT w1;

   if(uiThreadCount >= SFTMAXTHREADS) return;  // ignore it (for now)

   for(w1=0; w1<uiThreadCount; w1++)
   {
      if((lpThreadHandle[w1] == lpParm->hThread) ||
         (lpThreadID[w1] == lpParm->idSelf))
      {
         return;  // already there
      }
   }

   w1 = uiThreadCount++;

//   if(!dwThreadID) dwThreadID = GetCurrentThreadId();
//

   lpThreadHandle[w1] = lpParm->hThread;
   lpThreadID[w1]     = lpParm->idSelf;
   lpThreadTLS[w1]    = (LPVOID)lpParm;

   TlsSetValue(dwTLSIndex, (LPVOID)lpParm);

}


static __inline void AddThreadEntry(LPTHREADPARM32 lpParm)
{
   if(uiThreadCount >= SFTMAXTHREADS) return;  // ignore it (for now)

   if(hMutex) WaitForSingleObject(hMutex, INFINITE);

   AddThreadEntry0(lpParm);

   if(hMutex) ReleaseMutex(hMutex);
}




static __inline void RemoveThreadEntry(HANDLE hThread, DWORD dwThreadID)
{
UINT w1, w2;
LPCTHREADPARM32 lpTP;



   if(hMutex) WaitForSingleObject(hMutex, INFINITE);

   for(w1=0; w1<uiThreadCount; w1++)
   {
      if((hThread && lpThreadHandle[w1] == hThread) ||
         (dwThreadID && lpThreadID[w1] == dwThreadID))
      {

         lpTP = (LPCTHREADPARM32)lpThreadTLS[w1];

         if(lpTP->lpInstanceData) GlobalFree((HGLOBAL)lpTP->lpInstanceData);


         uiThreadCount--;

         for(w2 = w1; w2 < uiThreadCount; w2++)
         {
            lpThreadHandle[w2] = lpThreadHandle[w2 + 1];
            lpThreadID[w2]     = lpThreadID[w2 + 1];
            lpThreadTLS[w2]    = lpThreadTLS[w2 + 1];
         }

         lpThreadHandle[uiThreadCount] = 0;
         lpThreadID[uiThreadCount] = 0;
         lpThreadTLS[uiThreadCount] = 0;
      }
   }


   if(hMutex) ReleaseMutex(hMutex);

}





// This next function QUICKLY returns the 'THREADPARM32' pointer for
// the specified thread; else, returns NULL on error...

static __inline LPCTHREADPARM32 GetThreadParmPtr(HANDLE hThread)
{
LPCTHREADPARM32 lpTP;
DWORD dw1, dw2;


   lpTP = (LPTHREADPARM32)TlsGetValue(dwTLSIndex);

   if(lpTP && (!hThread || lpTP->hThread == hThread ||
               hThread == GetCurrentThread()))
   {
      return(lpTP);
   }

   if(hMutex) WaitForSingleObject(hMutex, INFINITE);

   dw2 = GetCurrentThreadId();

   for(dw1=0; dw1<uiThreadCount; dw1++)
   {
      if((hThread && lpThreadHandle[dw1]==hThread) ||
         (!hThread && lpThreadID[dw1]==dw2))
      {
         if(lpThreadTLS[dw1])
         {
            lpTP = (LPTHREADPARM32)lpThreadTLS[dw1];


            // in case it's "out of sync" with the TLS, fix it now!

            if(!hThread || lpTP->idSelf == GetCurrentThreadId() ||
               hThread == GetCurrentThread())
            {
               TlsSetValue(dwTLSIndex, lpThreadTLS[dw1]);
            }


            if(hMutex) ReleaseMutex(hMutex);

            return(lpTP);
         }

         break;   // entry exists, but there's no PARM pointer!
      }
   }


   if(hMutex) ReleaseMutex(hMutex);

   return(NULL);  // no PARM pointer exists!!

}


__inline DWORD MyGetThreadStatus(LPCTHREADPARM32 lpTP)
{
DWORD rval;


   if(!lpTP) return(0);

   // wait for ownership of the MTH32Mutex

   if(hMutex) WaitForSingleObject(hMutex, INFINITE);

   rval = lpTP->dwStatus;

   if(hMutex) ReleaseMutex(hMutex);

   return(rval);
}




extern void NEAR PASCAL RegisterCustomControlClasses(void);


int WINAPI MyLibMain(HINSTANCE hInstance);

BOOL WINAPI InitLibrary(HINSTANCE hInstance)
{
DWORD dw1;

   dw1 = (DWORD)MyLibMain(hInstance);

   hRootApp = MyDuplicateThreadHandle(GetCurrentThread());
   idRootApp = GetCurrentThreadId();

   if(dw1 && !bWin32sFlag)
   {
    static THREADPARM32 Parm;  // one per instance of DLL!!!

      TlsSetValue(dwTLSIndex, 0);  // zero entry

      Parm.lpProc   = NULL;        // these verify the presence of a
      Parm.hArgs    = NULL;        // 'root process' thread entry.
      Parm.hCaller  = NULL;
      Parm.hSelf    = hRootApp;
      Parm.hThread  = hRootApp;
      Parm.idCaller = 0;
      Parm.idSelf   = idRootApp;
      Parm.flags    = 0;
      Parm.dwStatus = 0;

      AddThreadEntry(&Parm);

      hMutex = CreateMutex(NULL, FALSE, szMutexName);

      // boost priority of the main thread at this time....

      // SetThreadPriority(hRootApp, THREAD_PRIORITY_HIGHEST);
   }


   return(dw1);
}

void WINAPI OnThreadAttach()
{
   if(!bWin32sFlag) TlsSetValue(dwTLSIndex, 0);  // zero entry
}

void WINAPI OnTerminate()
{
   RemoveThreadEntry(NULL, GetCurrentThreadId());

   if(bWin32sFlag)
   {
//      MyOutputDebugString("SFT32 - unloading \"WMTH32.DLL\"\r\n");
//      if(hWMth32)  FreeLibrary(hWMth32);

      if(dwTLSIndex != 0xffffffffL) TlsFree(dwTLSIndex);

      MyOutputDebugString("SFT32 - unloading \"WMTH32.DLL\"\r\n");
      if(hWMth32)  FreeLibrary(hWMth32);
      hWMth32 = NULL;
   }
}

void WINAPI OnThreadDetach()
{
   RemoveThreadEntry(NULL, GetCurrentThreadId());
}


int WINAPI MyLibMain(HINSTANCE hInstance)
{
WNDCLASS wc;      /* window class structure for "_MTH32_" */
OFSTRUCT ofstr;   /* used for 'Toolhelp' load (bug fix for 3.0) */
char tbuf[64];

   MyOutputDebugString("MTH32.LIB - initializing...\r\n");

   hLibInst = hInstance;

         /********************************************************/
         /** Get Windows Version and Library Function Addresses **/
         /********************************************************/

   dwVersion = GetVersion();

   dwVersion = (dwVersion & 0xffff0000L) |
               ((dwVersion & 0x0000ff00L) >> 8) |
               ((dwVersion & 0x000000ffL) << 8);

   if(dwVersion & 0x80000000L)  // we are Win32s?!
   {
      dwVersion &= 0xffffL;

      if(dwVersion < 0x400)
      {
         bWin32sFlag = TRUE;
      }

      MyOutputDebugString(bWin32sFlag?"SFT32.DLL - WIN32s detected!  "
                                     :"SFT32.DLL - Chicago detected!  ");

      wsprintf(tbuf, "Version: %8x\r\n", (DWORD)dwVersion);
      MyOutputDebugString(tbuf);
   }
   else
   {
      MyOutputDebugString("SFT32.DLL - WIN NT detected!\r\n");
   }



   if(bWin32sFlag)
   {
      dwSharedInstanceCount++;

      MyOutputDebugString("SFT32.DLL - Loading WMTH32.DLL...\r\n");

      hWMth32 = LoadLibrary("WMTH32.DLL");

      if(hWMth32 < (HINSTANCE)32)
      {
         MyOutputDebugString("SFT32.DLL - Loading failure on WMTH32.DLL!\r\n");
      }
   }
   else
   {
      // NOTE:  'DllMain()' functions are serialized!

      dwSharedInstanceCount++;

//      InterlockedIncrement(&dwSharedInstanceCount);


      hWMth32 = NULL;

      dwTLSIndex = TlsAlloc();  // get a 'Thread Local Storage' index

      if(dwTLSIndex == 0xffffffffL) // uh, oh - ERROR!
      {
         return(FALSE);  // fail the loading of the library
      }
   }


   if(hWMth32 >= (HINSTANCE)32)
   {
      (LVFARPROC)lpMthreadMsgDispatch         = GetProcAddress(hWMth32,MAKELPC(0,16));
      (LVFARPROC)lpLoopDispatch               = GetProcAddress(hWMth32,MAKELPC(0,17));
      (LVFARPROC)lpBroadcastMessageToChildren = GetProcAddress(hWMth32,MAKELPC(0,18));
      (LVFARPROC)lpRegisterCompactProc        = GetProcAddress(hWMth32,MAKELPC(0,19));
      (LVFARPROC)lpRegisterAppMessageProc     = GetProcAddress(hWMth32,MAKELPC(0,20));
      (LVFARPROC)lpRegisterAccelerators       = GetProcAddress(hWMth32,MAKELPC(0,21));
      (LVFARPROC)lpRemoveAccelerators         = GetProcAddress(hWMth32,MAKELPC(0,22));
      (LVFARPROC)lpEnableMDIAccelerators      = GetProcAddress(hWMth32,MAKELPC(0,23));
      (LVFARPROC)lpDisableMDIAccelerators     = GetProcAddress(hWMth32,MAKELPC(0,24));
      (LVFARPROC)lpEnableTranslateMessage     = GetProcAddress(hWMth32,MAKELPC(0,25));
      (LVFARPROC)lpDisableTranslateMessage    = GetProcAddress(hWMth32,MAKELPC(0,26));
      (LVFARPROC)lpEnableQuitMessage          = GetProcAddress(hWMth32,MAKELPC(0,27));
      (LVFARPROC)lpDisableQuitMessage         = GetProcAddress(hWMth32,MAKELPC(0,28));
      (LVFARPROC)lpEnableCloseMessage         = GetProcAddress(hWMth32,MAKELPC(0,29));
      (LVFARPROC)lpDisableCloseMessage        = GetProcAddress(hWMth32,MAKELPC(0,30));
      (LVFARPROC)lpMthreadInit                = GetProcAddress(hWMth32,MAKELPC(0,31));
      (LVFARPROC)lpDefMthreadMasterProc       = GetProcAddress(hWMth32,MAKELPC(0,32));
      (LVFARPROC)lpMthreadExit                = GetProcAddress(hWMth32,MAKELPC(0,34));
      (LVFARPROC)lpGetRootApplication         = GetProcAddress(hWMth32,MAKELPC(0,35));
      (LVFARPROC)lpGetRootInstance            = GetProcAddress(hWMth32,MAKELPC(0,36));
      (LVFARPROC)lpGetCurrentInstance         = GetProcAddress(hWMth32,MAKELPC(0,37));
      (LVFARPROC)lpRegisterMasterThreadWindow = GetProcAddress(hWMth32,MAKELPC(0,38));
      (LVFARPROC)lpGetMasterThreadWindow      = GetProcAddress(hWMth32,MAKELPC(0,39));
      (LVFARPROC)lpGetMthreadWindow           = GetProcAddress(hWMth32,MAKELPC(0,40));
      (LVFARPROC)lpIsThread                   = GetProcAddress(hWMth32,MAKELPC(0,41));
      (LVFARPROC)lpGetInstanceWindow          = GetProcAddress(hWMth32,MAKELPC(0,42));
      (LVFARPROC)lpGetInstanceTask            = GetProcAddress(hWMth32,MAKELPC(0,43));
      (LVFARPROC)lpGetTaskInstance            = GetProcAddress(hWMth32,MAKELPC(0,44));
      (LVFARPROC)lpSpawnThread                = GetProcAddress(hWMth32,MAKELPC(0,51));
      (LVFARPROC)lpMthreadCreateSemaphore     = GetProcAddress(hWMth32,MAKELPC(0,82));
      (LVFARPROC)lpMthreadKillSemaphore       = GetProcAddress(hWMth32,MAKELPC(0,83));
      (LVFARPROC)lpMthreadOpenSemaphore       = GetProcAddress(hWMth32,MAKELPC(0,84));
      (LVFARPROC)lpMthreadCloseSemaphore      = GetProcAddress(hWMth32,MAKELPC(0,85));
      (LVFARPROC)lpMthreadSetSemaphore        = GetProcAddress(hWMth32,MAKELPC(0,86));
      (LVFARPROC)lpMthreadClearSemaphore      = GetProcAddress(hWMth32,MAKELPC(0,87));
      (LVFARPROC)lpMthreadGetSemaphore        = GetProcAddress(hWMth32,MAKELPC(0,88));
      (LVFARPROC)lpRegisterSemaphoreProc      = GetProcAddress(hWMth32,MAKELPC(0,89));
      (LVFARPROC)lpRemoveSemaphoreProc        = GetProcAddress(hWMth32,MAKELPC(0,90));
      (LVFARPROC)lpMthreadGetCurrentThread    = GetProcAddress(hWMth32,MAKELPC(0,94));
      (LVFARPROC)lpMthreadKillThread          = GetProcAddress(hWMth32,MAKELPC(0,95));
      (LVFARPROC)lpMthreadSuspendThread       = GetProcAddress(hWMth32,MAKELPC(0,96));
      (LVFARPROC)lpMthreadResumeThread        = GetProcAddress(hWMth32,MAKELPC(0,97));
      (LVFARPROC)lpGetNumberOfThreads         = GetProcAddress(hWMth32,MAKELPC(0,98));
      (LVFARPROC)lpMthreadDisableMessages     = GetProcAddress(hWMth32,MAKELPC(0,99));
      (LVFARPROC)lpMthreadEnableMessages      = GetProcAddress(hWMth32,MAKELPC(0,100));
      (LVFARPROC)lpIsThreadHandle             = GetProcAddress(hWMth32,MAKELPC(0,103));
      (LVFARPROC)lpIsInternalThread           = GetProcAddress(hWMth32,MAKELPC(0,104));
      (LVFARPROC)lpIsHandleValid              = GetProcAddress(hWMth32,MAKELPC(0,105));
      (LVFARPROC)lpMthreadWaitForThread       = GetProcAddress(hWMth32,MAKELPC(0,106));
      (LVFARPROC)lpMthreadWaitSemaphore       = GetProcAddress(hWMth32,MAKELPC(0,115));
      (LVFARPROC)lpMthreadGetMessage          = GetProcAddress(hWMth32,MAKELPC(0,116));
      (LVFARPROC)lpMthreadSemaphoreCallback   = GetProcAddress(hWMth32,MAKELPC(0,117));
      (LVFARPROC)lpSpawnThreadX               = GetProcAddress(hWMth32,MAKELPC(0,121));
      (LVFARPROC)lpRegisterDialogWindow       = GetProcAddress(hWMth32,MAKELPC(0,131));
      (LVFARPROC)lpUnregisterDialogWindow     = GetProcAddress(hWMth32,MAKELPC(0,132));
      (LVFARPROC)lpMthreadThreadCritical      = GetProcAddress(hWMth32,MAKELPC(0,133));
      (LVFARPROC)lpMthreadThreadExclusive     = GetProcAddress(hWMth32,MAKELPC(0,134));
      (LVFARPROC)lpAddThreadInstanceData      = GetProcAddress(hWMth32,MAKELPC(0,147));
      (LVFARPROC)lpGetThreadInstanceData      = GetProcAddress(hWMth32,MAKELPC(0,148));
      (LVFARPROC)lpGetThreadInstanceDataPtr   = GetProcAddress(hWMth32,MAKELPC(0,149));
      (LVFARPROC)lpRemoveThreadInstanceData   = GetProcAddress(hWMth32,MAKELPC(0,150));
      (LVFARPROC)lpSetThreadInstanceData      = GetProcAddress(hWMth32,MAKELPC(0,151));
      (LVFARPROC)lpMthreadSleep               = GetProcAddress(hWMth32,MAKELPC(0,161));
      (LVFARPROC)lpCreateThreadWindow         = GetProcAddress(hWMth32,MAKELPC(0,162));
      (LVFARPROC)lpMthreadTranslateMessage    = GetProcAddress(hWMth32,MAKELPC(0,163));

      (LVFARPROC)lpMthreadDialogBoxParam         = GetProcAddress(hWMth32,MAKELPC(0,813));
      (LVFARPROC)lpSetHelperTaskPDB              = GetProcAddress(hWMth32,MAKELPC(0,815));
      (LVFARPROC)lpGetHelperTaskPDB              = GetProcAddress(hWMth32,MAKELPC(0,816));
      (LVFARPROC)lpMthreadDialogBoxIndirectParam = GetProcAddress(hWMth32,MAKELPC(0,819));
      (LVFARPROC)lpUpdateThreadPath              = GetProcAddress(hWMth32,MAKELPC(0,821));

      // special 'internal' stuff

      (LVFARPROC)lpCPlApplet                  = GetProcAddress(hWMth32,"CPlApplet");
      (LVFARPROC)lpDisplayWDBUtilLogo         = GetProcAddress(hWMth32,MAKELPC(0,806));

      // for now, assume they worked...


      if(!lpMthreadMsgDispatch            ||
         !lpLoopDispatch                  ||
         !lpBroadcastMessageToChildren    ||
         !lpRegisterCompactProc           ||
         !lpRegisterAppMessageProc        ||
         !lpRegisterAccelerators          ||
         !lpRemoveAccelerators            ||
         !lpEnableMDIAccelerators         ||
         !lpDisableMDIAccelerators        ||
         !lpEnableTranslateMessage        ||
         !lpDisableTranslateMessage       ||
         !lpEnableQuitMessage             ||
         !lpDisableQuitMessage            ||
         !lpEnableCloseMessage            ||
         !lpDisableCloseMessage           ||
         !lpMthreadInit                   ||
         !lpDefMthreadMasterProc          ||
         !lpMthreadExit                   ||
         !lpGetRootApplication            ||
         !lpGetRootInstance               ||
         !lpGetCurrentInstance            ||
         !lpRegisterMasterThreadWindow    ||
         !lpGetMasterThreadWindow         ||
         !lpGetMthreadWindow              ||
         !lpIsThread                      ||
         !lpGetInstanceWindow             ||
         !lpGetInstanceTask               ||
         !lpGetTaskInstance               ||
         !lpSpawnThread                   ||
         !lpMthreadCreateSemaphore        ||
         !lpMthreadKillSemaphore          ||
         !lpMthreadOpenSemaphore          ||
         !lpMthreadCloseSemaphore         ||
         !lpMthreadSetSemaphore           ||
         !lpMthreadClearSemaphore         ||
         !lpMthreadGetSemaphore           ||
         !lpRegisterSemaphoreProc         ||
         !lpRemoveSemaphoreProc           ||
         !lpMthreadGetCurrentThread       ||
         !lpMthreadKillThread             ||
         !lpMthreadSuspendThread          ||
         !lpMthreadResumeThread           ||
         !lpGetNumberOfThreads            ||
         !lpMthreadDisableMessages        ||
         !lpMthreadEnableMessages         ||
         !lpIsThreadHandle                ||
         !lpIsInternalThread              ||
         !lpIsHandleValid                 ||
         !lpMthreadWaitForThread          ||
         !lpMthreadWaitSemaphore          ||
         !lpMthreadGetMessage             ||
         !lpMthreadSemaphoreCallback      ||
         !lpSpawnThreadX                  ||
         !lpRegisterDialogWindow          ||
         !lpUnregisterDialogWindow        ||
         !lpMthreadThreadCritical         ||
         !lpMthreadThreadExclusive        ||
         !lpAddThreadInstanceData         ||
         !lpGetThreadInstanceData         ||
         !lpGetThreadInstanceDataPtr      ||
         !lpRemoveThreadInstanceData      ||
         !lpSetThreadInstanceData         ||
         !lpMthreadSleep                  ||
         !lpCreateThreadWindow            ||
         !lpMthreadTranslateMessage       ||
         !lpMthreadDialogBoxParam         ||
         !lpSetHelperTaskPDB              ||
         !lpGetHelperTaskPDB              ||
         !lpMthreadDialogBoxIndirectParam ||
         !lpUpdateThreadPath              ||
         !lpCPlApplet                     ||
         !lpDisplayWDBUtilLogo)
      {
         MyOutputDebugString("SFT32.DLL - 'GetProcAddress()' failure on WMTH32.DLL!\r\n");

         FreeLibrary(hWMth32);
         hWMth32 = NULL;
      }
      else
      {
         MyOutputDebugString("SFT32.DLL - 'GetProcAddress()' success on WMTH32.DLL!\r\n");
      }
   }


   if(hWMth32 < (HINSTANCE)32)
   {
      if(bWin32sFlag)
      {
         MyOutputDebugString("?SFT32 - WMTH32.DLL Library load has failed\r\n");
         return(FALSE);  // FAIL THE LIBRAY LOAD OPERATION!
      }
   }

   // See if KERNEL32 has the toolhelp functions in it...

   hKERNEL = GetModuleHandle("KERNEL32.DLL");

   if(hKERNEL && !bWin32sFlag)
   {
      (LVFARPROC)lpCreateToolhelp32Snapshot    = GetProcAddress(hKERNEL,"CreateToolhelp32Snapshot");
      (LVFARPROC)lpHeap32ListFirst             = GetProcAddress(hKERNEL,"Heap32ListFirst");
      (LVFARPROC)lpHeap32ListNext              = GetProcAddress(hKERNEL,"Heap32ListNext");
      (LVFARPROC)lpHeap32First                 = GetProcAddress(hKERNEL,"Heap32First");
      (LVFARPROC)lpHeap32Next                  = GetProcAddress(hKERNEL,"Heap32Next");
      (LVFARPROC)lpToolhelp32ReadProcessMemory = GetProcAddress(hKERNEL,"Toolhelp32ReadProcessMemory");
      (LVFARPROC)lpProcess32First              = GetProcAddress(hKERNEL,"Process32First");
      (LVFARPROC)lpProcess32Next               = GetProcAddress(hKERNEL,"Process32Next");
      (LVFARPROC)lpThread32First               = GetProcAddress(hKERNEL,"Thread32First");
      (LVFARPROC)lpThread32Next                = GetProcAddress(hKERNEL,"Thread32Next");
      (LVFARPROC)lpModule32First               = GetProcAddress(hKERNEL,"Module32First");
      (LVFARPROC)lpModule32Next                = GetProcAddress(hKERNEL,"Module32Next");
   }
   else
   {
      (LVFARPROC)lpCreateToolhelp32Snapshot    = NULL;
      (LVFARPROC)lpHeap32ListFirst             = NULL;
      (LVFARPROC)lpHeap32ListNext              = NULL;
      (LVFARPROC)lpHeap32First                 = NULL;
      (LVFARPROC)lpHeap32Next                  = NULL;
      (LVFARPROC)lpToolhelp32ReadProcessMemory = NULL;
      (LVFARPROC)lpProcess32First              = NULL;
      (LVFARPROC)lpProcess32Next               = NULL;
      (LVFARPROC)lpThread32First               = NULL;
      (LVFARPROC)lpThread32Next                = NULL;
      (LVFARPROC)lpModule32First               = NULL;
      (LVFARPROC)lpModule32Next                = NULL;
   }


   RegisterCustomControlClasses();     // register custom window classes


               /*********************************************/
               /*  Create a special window class "_MTH32_"  */
               /* that is GLOBAL for Win32s, and local else */
               /*********************************************/


   // NOTE:  'DllMain()' functions are serialized!

   if(!GetClassInfo(hInstance, "_MTH32_", &wc))
   {
      wc.style = CS_CLASSDC | CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS;

      wc.lpfnWndProc = MthreadWndProc;
      wc.cbClsExtra = 0;
      wc.cbWndExtra = 12;  /* OFFSET 0 (WORD):  Current Task Handle */
                           /* OFFSET 4 (WORD):  Current Thread ID   */
                           /* OFFSET 8 (WORD):  System Menu Handle  */
      wc.hInstance = hInstance;
      wc.hIcon = LoadIcon(hInstance,(LPSTR)"THREADICON");
      wc.hCursor = LoadCursor(NULL,IDC_ARROW);  /* default cursor */
      wc.hbrBackground = (HBRUSH)(COLOR_BACKGROUND + 1);
      wc.lpszMenuName = NULL;
      wc.lpszClassName = "_MTH32_";

      if(!RegisterClass((LPWNDCLASS)&wc))    /* 'TRUE' if o.k. - FALSE if not */
      {
         MyOutputDebugString("?SFT32 - Unable to register window class!  Failing load...\r\n");

         return(FALSE);                     // ERROR ERROR ERROR (for Win32)
      }
   }

   // FINAL STEP:  Set up a memory area for buffering for WDB32

   memset(&SysInfo, 0, sizeof(SysInfo));
   memset(&MemStat, 0, sizeof(MemStat));

   MemStat.dwLength = sizeof(MemStat);

   GetSystemInfo(&SysInfo);
   GlobalMemoryStatus(&MemStat);


   // reserve area of memory equal to twice the amount of physical RAM!
   // (this leaves plenty of room for 'holes' when needed...)

   dwVMemSize = MemStat.dwTotalPhys * 2;

   dwVMem = (DWORD)VirtualAlloc(0, dwVMemSize, MEM_RESERVE, PAGE_NOACCESS);

   if(!dwVMem)
   {
      if(bWin32sFlag)
      {
         if(hWMth32)  FreeLibrary(hWMth32);

         hWMth32 = NULL;


         MyOutputDebugString("?SFT32 (Win32s) - Unable to allocate virtual memory buffer!  Failing load...\r\n");
      }
      else
      {
         TlsFree(dwTLSIndex);

         dwTLSIndex = 0;


         MyOutputDebugString("?SFT32 - Unable to allocate virtual memory buffer!  Failing load...\r\n");
      }

      return(0);  // FAIL the load of this DLL!
   }



   return(TRUE);              // NO error - process is ok

}


int WINAPI GetUsageCount(void)
{
   return(dwSharedInstanceCount);  // just return it 'as-is' right now
}


#pragma code_seg()



/***************************************************************************/
/*    Additional DLL Support functions for multi-thread initialization     */
/***************************************************************************/

#pragma code_seg ("DLL_CONTROL_TEXT","CODE")

VFPPROC DisplayWDBUtilLogo(void)
{
   if(bWin32sFlag)
   {
      lpDisplayWDBUtilLogo();
      return;
   }

   display_wdbutil_logo = TRUE; /* causes logo for WDBUTIL.DLL to display */
                                 /* following logo for WMTHREAD.DLL!!      */
}


void FAR PASCAL SetCPUFlags(void)
{

   // for now, nothing.

}


#pragma code_seg()



/***************************************************************************/
/*                         TRANSLATE FUNCTIONS                             */
/***************************************************************************/

#pragma code_seg("MYTRANSLATE_TEXT","CODE")

INLINE BOOL NEAR PASCAL ThisTaskHasMDIAccelerators(void)
{
   if(MDIAcceleratorsClient && MDIAcceleratorsEnable>0)
      return(TRUE);
   else
      return(FALSE);
}



INLINE BOOL NEAR PASCAL TranslateMDITaskAccel(LPMSG msg)
{
WORD w;


   /* when this function is called 'current_task_index' is correct */
   /* for the current task, so I don't need to 'set' it again!     */


   if(!MDIAcceleratorsClient || !MDIAcceleratorsEnable)
   {
      return(FALSE);  /* nothing there! */
   }

   for(w=0; w<MDIAcceleratorsEnable; w++)
   {
      if(IsWindow(MDIAcceleratorsClient[w]) &&
         TranslateMDISysAccel(MDIAcceleratorsClient[w],msg))
      {
         return(TRUE);
      }
   }

   return(FALSE);
}


INLINE BOOL NEAR PASCAL TranslateDialogTaskMessage(LPMSG msg)
{
UINT w1, w2;
HWND hWnd;
char tbuf[32];


   if(!TranslateEnable)
   {
      return(FALSE);        // do not translate messages!
   }

   hWnd = msg->hwnd;

   if(!IsWindow(hWnd)) return(FALSE);  // window doesn't exist

   if(DialogWindowList)
   {
      for(w1=0; DialogWindowList[w1]; )
      {
         if(!IsWindow(DialogWindowList[w1])) // this is where I eliminate
         {                                   // windows that were destroyed

            for(w2=w1 + 1; DialogWindowList[w2]; w2++)
            {
               DialogWindowList[w2 - 1] = DialogWindowList[w2];
            }

            DialogWindowList[w2 - 1] = NULL;

            continue;     // go through the loop again (part of the tests)
         }

         if(IsDialogMessage(DialogWindowList[w1], msg))
         {
            return(TRUE);
         }

         w1++;    // increment is here so I can use 'continue' to bypass it
      }
   }


   // IF THIS WINDOW IS A DIALOG FRAME, OR ONE OF IT'S PROGENITORS IS
   // A DIALOG FRAME, THEN CALL 'IsDialogMessage()' TO TRANSLATE/DISPATCH!

   while(hWnd && IsWindow(hWnd) /* &&
         (GetWindowLong(hWnd, GWL_STYLE) & WS_CHILD) */)
   {
      if(GetClassName(hWnd, tbuf, sizeof(tbuf)) &&
         tbuf[0]=='#' && _hatol(tbuf + 1) == (DWORD)WC_DIALOG)
      {
         // this window is a valid 'dialog' window... I assume!  I included the
         // class's module handle equal to 'hUser' to ensure it's correct!!

         if(IsDialogMessage(hWnd, msg)) return(TRUE);
      }

      hWnd = GetParent(hWnd);   // get window's parent...
   }

   return(FALSE);   // this allows 'DispatchMessage()' to be used
}





BOOL FAR PASCAL MyTranslate(LPMSG msg)
{
int i, iret;

   // If this is a dialog, or has been registered as a dialog, use the
   // 'IsDialogMessage()' function to translate/dispatch message as req'd

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

      for(i=0; !iret && i<MAX_ACCEL &&
               Accelerators[i].hWnd && Accelerators[i].hAccel; i++)
      {
                 /* first NULL entry in 'Accelerators' marks end of list */

         if(IsWindow(Accelerators[i].hWnd))
         {
                                 /* translate Accelerators */
            iret = TranslateAccelerator(Accelerators[i].hWnd,
                                        Accelerators[i].hAccel,
                                        msg);
         }
      }

   }

           /* if no translation (yet) & Translation is Enabled */

   if(!iret && TranslateEnable)
   {
      TranslateMessage(msg);  /* must return FALSE so I still dispatch it */
   }

   return(iret);              /* if no translation occurred - onward! */

}

#pragma code_seg()



/***************************************************************************/
/*    THREAD WINDOW (ICONIC) CALLBACK FUNCTION - tie an icon to thread!    */
/*              (This window is used when printing graphs!)                */
/***************************************************************************/


#pragma code_seg ("MYTRANSLATE_TEXT","CODE")


HWND FPPROC CreateThreadWindow(HANDLE hThread)
{
LPTHREADPARM32 lpTP;
HWND hWnd;

   if(bWin32sFlag)
   {
      return(lpCreateThreadWindow(hThread));
   }

//   if(!hThread) hThread = MthreadGetCurrentThread();

//   if(!hThread || !IsThreadHandle(hThread)) return(NULL);

   lpTP = (LPTHREADPARM32)GetThreadParmPtr(hThread);
   if(!lpTP) return(NULL);

   hWnd = CreateWindow((LPSTR)"_MTH32_",
                       (LPSTR)"Exec Thread",
                       WS_ICONIC | WS_OVERLAPPED | WS_SYSMENU,
                       CW_USEDEFAULT,CW_USEDEFAULT,
                       200, 64, NULL, NULL,
                       (HINSTANCE)GetCurrentInstance(), NULL);

   if(!hWnd) return(NULL);


   lpTP->hwndThread = hWnd;  // save handle of attached window here!!!

   SetWindowLong(hWnd, 0, (long)GetCurrentProcessId()); /* assign task handle */
   SetWindowLong(hWnd, 4, (long)hThread);               /* assign Thread ID */


   ShowWindow(hWnd, SW_SHOWMINNOACTIVE);
                /* show window as an icon and don't activate it! */

   UpdateWindow(hWnd);   // make sure it paints NOW!


   if(hMutex) WaitForSingleObject(hMutex, INFINITE);

   if(lpTP->dwStatus & THREADSTATUS_SUSPEND)  // thread is SUSPENDED
   {
    HMENU hMenu = (HMENU)GetWindowWord(hWnd, 4);

      if(hMenu)
      {
         EnableMenuItem(hMenu, IDM_SUSPEND, MF_BYCOMMAND | MF_DISABLED);
         EnableMenuItem(hMenu, IDM_RESUME, MF_BYCOMMAND | MF_ENABLED);
      }
   }

   if(hMutex) ReleaseMutex(hMutex);

   return(hWnd);         // return the window handle!

}


LRESULT LOADDS CALLBACK MthreadWndProc(HWND hWnd, UINT msg,
                                       WPARAM wParam, LPARAM lParam)
{
HMENU hMenu;




                   /*****************************************/
                   /*            WINDOW WORDS               */
                   /* OFFSET 0 (long):  Current Task Handle */
                   /* OFFSET 4 (long):  Current Thread ID   */
                   /* OFFSET 8 (long):  System Menu Handle  */
                   /*****************************************/

   switch(msg)
   {
      case WM_NCCREATE:

         SetWindowLong(hWnd, 0, 0);
         SetWindowLong(hWnd, 4, 0);
         SetWindowLong(hWnd, 8, 0);

         break;


      case WM_CREATE:
         hMenu = GetSystemMenu(hWnd, FALSE);


         // for now, I must assume the thread is RUNNING!

         AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
         AppendMenu(hMenu, MF_STRING | MF_ENABLED, IDM_SUSPEND, "&Suspend");
         AppendMenu(hMenu, MF_STRING | MF_GRAYED | MF_DISABLED, IDM_RESUME, "&Resume");

         SetWindowLong(hWnd, 8, (WORD)hMenu);


         return(0);



      case WM_COMMAND:
         break;

//    case WM_WININICHANGE:
//       WinIniChangeInternational();
//       break;
//
      case WM_DESTROY:         /* Now we'll know for sure! */
         {
          HTASK hTask=NULL;
          HANDLE hThread=NULL;
          LPTHREADPARM32 lpTP;


            if(!(hTask = (HTASK)GetWindowLong(hWnd, 0)))
               break;                   /* task handle ID */

            if(!(hThread = (HANDLE)GetWindowLong(hWnd, 4)))
               break;      /* Thread ID - not a thread - just QUIT */

            MthreadKillThread(hThread);           /* kill the bloomin' thread! */
                                        /* the one attached to the window */

            lpTP = (LPTHREADPARM32)GetThreadParmPtr(hThread);

            if(lpTP->hwndThread == hWnd)   // make sure attached 'hWnd'
            {                                 // is assigned a NULL here...
               lpTP->hwndThread = NULL;
            }
         }

         GetSystemMenu(hWnd, TRUE);
         break;


      case WM_SYSCOMMAND:

         switch(((WORD)wParam) & 0xfff0)
         {
            case SC_MAXIMIZE:
            case SC_MINIMIZE:
            case SC_RESTORE:
            case SC_SIZE:
            case SC_HSCROLL:
            case SC_VSCROLL:

               return(TRUE);     // eat the message



            case SC_CLOSE:

               if(MthreadMessageBox(hWnd,
                             "Do you wish to terminate the running thread?",
                             "** MTHREAD WARNING **", MB_YESNO | MB_ICONHAND)
                  != IDYES)
               {
                  return(TRUE);  // eat the message
               }

               break;            // default processing



            case IDM_SUSPEND:         // 'suspend'

               hMenu = (HMENU)GetWindowWord(hWnd, 4);

               EnableMenuItem(hMenu, IDM_SUSPEND, MF_BYCOMMAND | MF_GRAYED | MF_DISABLED);
               EnableMenuItem(hMenu, IDM_RESUME, MF_BYCOMMAND | MF_ENABLED);

               MthreadSuspendThread((HANDLE)GetWindowWord(hWnd, 2));
               return(TRUE);     // message handled


            case IDM_RESUME:         // 'resume'

               hMenu = (HMENU)GetWindowWord(hWnd, 4);

               EnableMenuItem(hMenu, IDM_SUSPEND, MF_BYCOMMAND | MF_ENABLED);
               EnableMenuItem(hMenu, IDM_RESUME, MF_BYCOMMAND | MF_GRAYED | MF_DISABLED);

               MthreadResumeThread((HANDLE)GetWindowWord(hWnd, 2));
               return(TRUE);     // message handled


         }

         break;



      case WM_QUERYOPEN:

         return((LRESULT)NULL);   /* prevents application from opening icon */

   }


   return(DefWindowProc(hWnd, msg, wParam, lParam));
}


#pragma code_seg()



/***************************************************************************/
/*     This proc executes once for each load of 'MTHREAD' - Displays (c)   */
/***************************************************************************/

#pragma code_seg ("MTHREADIDDLG_TEXT","CODE")


static HWND hdlgLogo = NULL;
static DWORD dwLogoTime = 0;
static HTASK hLogoOwnerTask = NULL;

void FAR PASCAL LOADDS CheckOrDestroyLogo(void)
{
   if(!hdlgLogo) return;

   if((GetTickCount() - dwLogoTime) >= 3000
      && GetCurrentProcessId() == (DWORD)hLogoOwnerTask)
   {
    HWND hwnd = hdlgLogo;

     hdlgLogo = NULL;

      if(IsWindow(hwnd) && IsWindowVisible(hwnd))
      {
         ShowWindow(hwnd, SW_HIDE);

         SendMessage(hwnd, WM_USER, 0, 0);
      }
   }
   else if((GetTickCount() - dwLogoTime) >= 10000)
   {
    HWND hwnd = hdlgLogo;

     hdlgLogo = NULL;

      PostMessage(hwnd, WM_COMMAND, (WPARAM)IDCANCEL, 0);
   }
}


BOOL LOADDS CALLBACK MthreadIDDlgProc(HWND hDlg, UINT msg,
                                      WPARAM wParam, LPARAM lParam)
{
static HWND hFocus0, hFocus1, hwndFocus;
DWORD dwVer;


   lParam = lParam;
   wParam = wParam;  // prevents warnings...

   switch(msg)
   {
      case WM_INITDIALOG:

         hFocus0 = hMthreadIDFocus; // who had focus before 'CreateDialog()'

         ShowWindow(hDlg, SW_SHOWNORMAL);

         hFocus1 = GetFocus(); // if the focus changes when I show the dialog
                               // make sure I get the 'new' value now.

         dwVer = GetVersion();
         if(((LOWORD(dwVer) & 0xff00)>=0xa00 && (LOWORD(dwVer) & 0xff)==0x3)
            || (LOWORD(dwVer) & 0xff)>0x3)
         {
            SetWindowPos(hDlg, HWND_TOPMOST, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE);
         }

         return(TRUE);


      case WM_DESTROY:

         hdlgLogo = NULL;

         KillTimer(hDlg, (WORD)-1);
         break;


      case WM_COMMAND:
         if((WORD)wParam != IDCANCEL) break;

         // flow through to NEXT section...

      case WM_USER:      /* I sent this, eh?? */
      case WM_TIMER:     /* alternate method */

         KillTimer(hDlg, (WORD)-1);

         hwndFocus = GetFocus();

         if(!hwndFocus || hwndFocus==hDlg || hwndFocus==hFocus1)
         {
            if(IsWindowVisible(hDlg)) ShowWindow(hDlg, SW_HIDE);

            if(hFocus0!=hDlg
//               && (!htaskMthreadIDFocus || IsTask(htaskMthreadIDFocus))
               && IsWindow(hFocus0))// is 'hFocus0' valid?
            {                       // if so, try to set focus to the
                                    // window that had focus when I
                                    // created this thing
               SetFocus(hFocus0);

               DestroyWindow(hDlg);
            }
            else
            {

            // I have a choice what to do here, which includes finding
            // a window for the current task, etc. etc.

               if((hFocus0 = GetMasterThreadWindow())
                  && IsWindow(hFocus0) && IsWindowVisible(hFocus0))
               {
                  SetFocus(hFocus0);
               }
               else if((hFocus0 = GetMthreadWindow())
                       && IsWindow(hFocus0) && IsWindowVisible(hFocus0))
               {
                  SetFocus(hFocus0);
               }
               else if((hFocus0 = GetInstanceWindow(GetCurrentInstance()))
                       && IsWindow(hFocus0) && IsWindowVisible(hFocus0))
               {
                  SetFocus(hFocus0);
               }
               else
               {
//                  hFocus0 = FindWindow(NULL, NULL);
//
//                  if(IsWindow(hFocus0) && IsWindowVisible(hFocus0))
//                  {
//                     while(GetParent(hFocus0))
//                        hFocus0 = GetParent(hFocus0);
//
//
//                     SetFocus(hFocus0);
//                  }

                  SetFocus(GetDesktopWindow());
               }
            }

            DestroyWindow(hDlg);
         }
         else
         {
            SetFocus(hFocus0 = GetFocus());  // a 'kick-in-the-pants'?

            ShowWindow(hDlg, SW_HIDE);

            SetFocus(hFocus0);

            DestroyWindow(hDlg);
         }

         return(TRUE);


   }

   return(FALSE);
}



/***************************************************************************/
/*                       THREAD CONTROL PROCEDURES                         */
/***************************************************************************/

#pragma code_seg ("DLL_CONTROL_TEXT", "CODE")

BFPPROC MthreadInit(HINSTANCE hInst, HANDLE hPrev, LPSTR lpCmd, UINT uiCmdShow)
{    /* first thing application does before anything else is call this! */
LPSTR p1=NULL;
static char name[32];


   if(!InitLibrary(hInst)) return(TRUE);  // ERROR

   if(bWin32sFlag) return(lpMthreadInit(hInst, hPrev, lpCmd, uiCmdShow));


   hRootInst = hInst;                 // make a record of the task's instance

   return(FALSE);                          /* this says 'I am a main task' */

}



#pragma code_seg ("SPAWNTHREAD_TEXT","CODE")


UFPPROC SpawnThread(LPTHREADPROC lpProcName, HANDLE hArgs, UINT flags)
{

   /** For now, assume a stack size of 16k - later options will allow **/
   /** specifying different sizes or the actual stack space.          **/

   return(SpawnThreadX((LPTHREADPROC)lpProcName, hArgs, flags, NULL, 0));
}



UFPPROC SpawnThreadX(LPTHREADPROC lpProcName, HANDLE hArgs, UINT flags,
                     LPSTR lpThreadStack, UINT dwStackSize)
{
HANDLE hThread;


   if(bWin32sFlag)
   {
      if(!lpSpawnThreadX) return((flags & THREADSTYLE_RETURNHANDLE)?0:1);

      return(lpSpawnThreadX(lpProcName, hArgs, flags,
                            lpThreadStack, dwStackSize));
   }

   if(flags & THREADSTYLE_LPTHREADPROC)
   {
      flags &= ~THREADSTYLE_WIN32CALL;
   }
   else if(!(flags & THREADSTYLE_WIN32CALL))
   {
      flags |= THREADSTYLE_LPTHREADPROC;
   }

   hThread = MthreadCreateThread(NULL, dwStackSize,
                                 (LPTHREAD_START_ROUTINE)lpProcName,
                                 (LPVOID)hArgs,
                                 (flags & THREADSTYLE_SUSPEND)?
                                    CREATE_SUSPENDED:0,
                                 NULL,
                                 flags);


   if(flags & THREADSTYLE_RETURNHANDLE)
   {
      return((UINT)hThread);   /* return handle of thread */
   }
   else
   {
      return(!hThread);
   }

}


static BOOL bSpawnThreadSyncFlag = 0;


HANDLE WINAPI MthreadCreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes,
                                  DWORD dwStackSize,
                                  LPTHREAD_START_ROUTINE lpStartAddress,
                                  LPVOID lpParameter,
                                  DWORD dwCreationFlags,
                                  LPDWORD lpThreadIdRval,
                                  UINT uiMthreadFlags )
{
HANDLE hThread;
DWORD dwThreadID, dw1;
volatile LPTHREADPARM32 lpParm;



   if(lpThreadIdRval) *lpThreadIdRval = 0;   // do THIS first...

   if(bWin32sFlag)
   {
      if(!lpSpawnThreadX) return(NULL);

      uiMthreadFlags |= THREADSTYLE_RETURNHANDLE;

      if(!(uiMthreadFlags & THREADSTYLE_LPTHREADPROC))
      {
         uiMthreadFlags |= THREADSTYLE_WIN32CALL;
      }

      if(dwCreationFlags & CREATE_SUSPENDED)
      {
         uiMthreadFlags |= THREADSTYLE_SUSPEND;
      }

      hThread = lpSpawnThreadX((LPTHREADPROC)lpStartAddress,
                               (HANDLE)lpParameter,
                               uiMthreadFlags, NULL, dwStackSize);

      if(lpThreadIdRval) *lpThreadIdRval = (DWORD)hThread;

      return(hThread);
   }


   if(dwCreationFlags & CREATE_SUSPENDED)
   {
      uiMthreadFlags |= THREADSTYLE_SUSPEND;
   }

   if(!(uiMthreadFlags & THREADSTYLE_LPTHREADPROC))
   {
      uiMthreadFlags |= THREADSTYLE_WIN32CALL;
   }


   // NOTE:  'lpThreadStack' is NOT used in this version!!!


   lpParm = (LPTHREADPARM32)GlobalAlloc(GPTR, sizeof(*lpParm));
   if(!lpParm) return(NULL);  // ERROR!


   lpParm->lpProc   = (FARPROC)lpStartAddress;
   lpParm->hArgs    = (HANDLE)lpParameter;
   lpParm->hCaller  = MyDuplicateThreadHandle(GetCurrentThread());
   lpParm->idCaller = GetCurrentThreadId();
   lpParm->hSelf    = 0;  // for now
   lpParm->hThread  = 0;  // for now
   lpParm->idSelf   = 0;  // for now
   lpParm->flags    = uiMthreadFlags;
   lpParm->retcode  = 0;
   lpParm->dwStatus = 0;

//   if(dwCreationFlags & CREATE_SUSPENDED)
//   {
//      lpParm->dwStatus = THREADSTATUS_SUSPEND;
//   }



   hThread = CreateThread(lpThreadAttributes, dwStackSize,
                          SpawnThreadProc, (LPVOID)lpParm,
                          dwCreationFlags | CREATE_SUSPENDED,
                          &dwThreadID);


   if(!hThread)
   {
      // failure to create thread - return appropriate value

      CloseHandle(lpParm->hCaller);

      GlobalFree((HGLOBAL)lpParm);

      return(NULL);                // on error, return NULL
   }


   // before I post any messages or start the thread, ensure I have
   // valid fields for 'lpParm'...

   lpParm->hSelf   = MyDuplicateThreadHandle(hThread); // dup handle FIRST...
   lpParm->hThread = hThread;                          // this identifies it
   lpParm->idSelf  = dwThreadID;                       // then store the ID!


   // at this point, 'hSelf' keeps the thread from dying if the user closes
   // the identifying handle 'hThread'.  'hThread' will continue to be the
   // 'token' handle for this thread, however, under all circumstances
   // until it is terminated via 'KillThread' or it terminates itself.


   dw1 = GetThreadPriority(hRootApp);  // priority of root application

   if(dw1 > THREAD_PRIORITY_BELOW_NORMAL) dw1 = THREAD_PRIORITY_BELOW_NORMAL;
   SetThreadPriority(hThread, dw1);

   // thread priority is either 'below normal' or else equal to that of the
   // 'root thread' if the 'root thread' is at an even lower priority.


   if(hMutex) WaitForSingleObject(hMutex, INFINITE);

   bSpawnThreadSyncFlag = 0;

   ResumeThread(hThread);              // thread execution begins NOW
                                       // suspend will happen later, if
                                       // it needs to !

   if(dwCreationFlags & CREATE_SUSPENDED)
   {
      while(!bSpawnThreadSyncFlag)
      {
         Sleep(0);                   // gives up remainder of time slice
      }

      if(hMutex) ReleaseMutex(hMutex); // this may not release the thread first

      while(!lpParm->retcode)        // wait until 'lpParm' return code
      {                              // restored for the thread.
         Sleep(0);                   // give up my time slice
      }

      lpParm->retcode = 0;           // reset to zero (it means 'go ahead')

      if(hMutex) WaitForSingleObject(hMutex,INFINITE);

      Sleep(1);                      // wait for just a bit...
   }

   ReleaseMutex(hMutex);             // release the mutex now...
                                     // (*THAT* was *WEIRD*!)


   // NEXT, if I am to wait for this thread to continue, use a 'Wait'
   // command right now.

   if(uiMthreadFlags & THREADSTYLE_WAIT)
   {
      do
      {
         dw1 = WaitForSingleObject(hThread, 50);  // wait 50 msec

         // here, add check for 'soft terminate'.  For now, nothing

         if(dw1 = WAIT_FAILED)
         {
            CloseHandle(hThread);

            return(0);
         }

      } while(dw1 != WAIT_OBJECT_0);   // wait for thread to end
   }

   if(lpThreadIdRval) *lpThreadIdRval = dwThreadID;
   return(hThread);
}



void WINAPI MthreadExitThread(UINT uiRetVal)   // CALL THIS to terminate a thread
{                                              // immediately with return code.
LPTHREADPARM32 lpParm;

   lpParm = (LPTHREADPARM32)GetThreadParmPtr(NULL);

   if(lpParm) lpParm->retcode = uiRetVal;

   RaiseException(SFT_THREAD_TERMINATE, EXCEPTION_NONCONTINUABLE, 0, NULL);
}



long FAR PASCAL SpawnThreadProc(LPTHREADPARM32 lpParm)
{
THREADPARM32 Parm;
HWND hMaster;
long rval;
DWORD dw1;

   OnThreadAttach();

//   TlsSetValue(dwTLSIndex, 0);              // First thing's first

   Parm = *lpParm;                          // make a copy of everything


   lpParm->retcode = 0;
   bSpawnThreadSyncFlag = 1;                // this is where I am...
                                            // caller will release mutex


   // ALL threads created here generate 'per thread' data

   if(hMutex) WaitForSingleObject(hMutex, INFINITE);  // wait here to synchronize

   AddThreadEntry0(&Parm);    // add thread entry without the mutex...



   if(Parm.flags & THREADSTYLE_SUSPEND)
   {
      // I need to SYNCHRONIZE with 'MthreadCreateThread()' to ensure
      // that 'MthreadCreateThread()' returns AFTER I suspend this
      // thread internally.


      lpParm->retcode = 1;                  // this is where I am...
                                            // caller will synchronize on it

      while(lpParm->retcode) Sleep(0);      // wait until it's zero again
                                            // caller is not referencing it
   }

   GlobalFree((HGLOBAL)lpParm);             // no longer needed...
                                            // caller now waits for mutex



   /* the thread is alive!  Let application know about it */

   while(!PostThreadMessage(idRootApp, THREAD_REGISTER,
                            (WPARAM)Parm.idSelf, (LPARAM)(Parm.hThread)))
   {
      dw1 = GetLastError();

      if(dw1==ERROR_INVALID_THREAD_ID || dw1==ERROR_INVALID_MESSAGE)
      {
         break;  // for now, just bust out of loop...
      }

      Sleep(0); // wait and try again ( ERROR_CAN_NOT_COMPLETE ? )
   }

   while(!PostThreadMessage(idRootApp, THREAD_ACKNOWLEDGE,
                            (WPARAM)Parm.idSelf, (LPARAM)(Parm.hThread)))
   {
      dw1 = GetLastError();

      if(dw1==ERROR_INVALID_THREAD_ID || dw1==ERROR_INVALID_MESSAGE)
      {
         break;  // for now, just bust out of loop...
      }

      Sleep(0); // wait and try again ( ERROR_CAN_NOT_COMPLETE ? )
   }


   hMaster = GetMasterThreadWindow();

   if(hMaster && IsWindow(hMaster)) /* if there's a 'master thread window' */
   {
      while(!PostMessage(hMaster, THREAD_ACKNOWLEDGE,
                         (WPARAM)Parm.idSelf, (LPARAM)(Parm.hThread)))
      {
         dw1 = GetLastError();

         if(dw1!=ERROR_CAN_NOT_COMPLETE)
         {
            break;  // for now, just bust out of loop...
         }

         Sleep(0); // wait and try again
      }
   }



   // is the thread being created "SUSPENDED"?  If so, I must suspend
   // myself PRONTO!

   if(Parm.flags & THREADSTYLE_SUSPEND)
   {
//      // If I have already 'un-suspended' the thread, the dwFlags will have
//      // the 'SUSPEND' flag cleared already.  Check it...
//
//      if(Parm.dwStatus & THREADSTATUS_SUSPEND)
//      {

         // this code 'stolen' from 'MthreadSuspendThread()'

         Parm.dwStatus |= THREADSTATUS_SUSPEND;      // assign correct flags


         if(hMutex)
         {
            Sleep(0);              // release time slice NOW
            ReleaseMutex(hMutex);  // release mutex IMMEDIATELY!
         }

         SuspendThread(Parm.hThread); // and IMMEDIATELY suspend the thread. Whew!


         // on return, the mutex will already be freed
         // as the thread suspends, the caller will continue and return
         // to the proc that called MthreadCreateThread or SpawnThreadX
         // with THIS thread actually suspended.  How about that!

//      }
//      else
//      {
//         if(hMutex) ReleaseMutex(hMutex);  // at *THIS* point CreateThread completes
//      }
   }
   else
   {
      if(hMutex) ReleaseMutex(hMutex);  // at *THIS* point CreateThread completes
   }



   // If there's a "thread icon" for this thread, create it!

   if(Parm.flags & THREADSTYLE_ICON)     // should I have a thread window?
   {
      if(!CreateThreadWindow(Parm.hThread))
      {
//         CloseHandle(Parm.hCaller);
//         CloseHandle(Parm.hThread);
//         return(-1L);  // ERROR!  (whoops!)

         rval = -1;  // an error code on return

         goto thread_bailout;
      }
   }


   // implement structured exception handling here... unhandled exceptions
   // should pop up a dialog box allowing the user to terminate the thread
   // or re-try the action.  This will eliminate the need for TOOLHELP
   // intervention!  YAY!


   __try
   {
    LPTHREADPROC lpThreadProc;
    LPTHREAD_START_ROUTINE lpStartAddress;


      if(Parm.flags & THREADSTYLE_LPTHREADPROC)
      {
         // call the thread proc with the same parameters as a 16-bit thread

         lpThreadProc = (LPTHREADPROC)Parm.lpProc;
         rval = lpThreadProc(Parm.hCaller, Parm.hArgs, Parm.flags);
      }
      else
      {
         // call the thread proc as a Win32 thread proc

         lpStartAddress = (LPTHREAD_START_ROUTINE)Parm.lpProc;
         rval = lpStartAddress((LPVOID)Parm.hArgs);
      }

   }
   __except(MyThreadExceptionHandler(dw1 = GetExceptionCode(),
                                     GetExceptionInformation(),
                                     &Parm))
   {
      if(dw1 == SFT_THREAD_TERMINATE)
      {
         rval = Parm.retcode;
      }
      else
      {
         rval = -1;  // this is the 'generic' error return value
      }
   }


thread_bailout:

   // thread proc should RETURN, and not call 'ThreadExit()'

   RemoveThreadEntry(Parm.hThread, Parm.idSelf);  // remove thread prop, etc.
                                                  // and the 'internal' entry
                                                  // in the 'TLS' table.

   /* the thread is dead!  Let application know about it */

   while(!PostThreadMessage(idRootApp, THREAD_COMPLETE,
                            (WPARAM)rval, (LPARAM)(Parm.hThread)))
   {
      dw1 = GetLastError();

      if(dw1==ERROR_INVALID_THREAD_ID || dw1==ERROR_INVALID_MESSAGE)
      {
         break;  // for now, just bust out of loop...
      }

      Sleep(50); // wait 50 msec's and try again ( ERROR_CAN_NOT_COMPLETE ? )
   }


   hMaster = GetMasterThreadWindow();

   if(hMaster && IsWindow(hMaster)) /* if there's a 'master thread window' */
   {
      while(!PostMessage(hMaster, THREAD_COMPLETE,
                         (WPARAM)rval, (LPARAM)(Parm.hThread)))
      {
         dw1 = GetLastError();

         if(dw1!=ERROR_CAN_NOT_COMPLETE)
         {
            break;  // for now, just bust out of loop...
         }

         Sleep(50); // wait 50 msec's and try again
      }
   }



   CloseHandle(Parm.hCaller);  // no longer needed

   if(Parm.hThread != Parm.hSelf) CloseHandle(Parm.hThread);

   CloseHandle(Parm.hSelf);    // if this doesn't kill the thread, then
                               // someone might want the return value...

   OnThreadDetach();

   return(rval);               // exit from thread.  I am done.

}



long PASCAL MyThreadExceptionHandler(DWORD dwExceptionCode,
                                     PEXCEPTION_POINTERS lpXP,
                                     LPTHREADPARM32 lpParm)
{
LPSTR lp1, lp2;
DWORD dwCS, dwEIP, dwException, idThread, dw1;
HANDLE hThread;
char faultbuf[512];
static const char szCRLF[]="\r\n";
static const char CODE_BASED szFmt1[]=
                 " caused a%s\nat CS:EIP %04X:%08X in Thread %08X(%08X)";




   // return value:  EXCEPTION_CONTINUE_SEARCH, EXCEPTION_EXECUTE_HANDLER,
   //                or EXCEPTION_CONTINUE_EXECUTION
   //
   // It is assumed that 'EXCEPTION_CONTINUE_SEARCH' will kill the app.
   //


   // THIS PROVIDES A CENTRAL LOCATION FROM WHICH I CAN
   // SET BREAKPOINTS OR WHATEVER (as needed)





   // Provide a "swan song" for the dying application.  Later, the
   // 'TerminateApp()' function can do the drop-down register dump;
   // for now, I'll just look 'up the chain' for exception handlers.

   dwEIP = lpXP->ContextRecord->Eip;
   dwCS  = lpXP->ContextRecord->SegCs;

   hThread  = lpParm->hThread;
   idThread = lpParm->idSelf;




   switch(dwExceptionCode)
   {
      case SFT_THREAD_TERMINATE:         // user called 'MthreadExitThread()'
         return(EXCEPTION_EXECUTE_HANDLER);


      case EXCEPTION_ACCESS_VIOLATION:
         lp1 = "n Access Violation Exception";
         break;

      case EXCEPTION_NONCONTINUABLE_EXCEPTION:
         lp1 = " Non-Continuable Exception";
         break;

      case EXCEPTION_DATATYPE_MISALIGNMENT:
         lp1 = " Data Type Alignment Error";
         break;

      case EXCEPTION_INVALID_DISPOSITION:
         lp1 = "n Invalid Disposition Exception";
         break;

      case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
         lp1 = "n Array Bounds Exceeded Exception";
         break;

      case EXCEPTION_FLT_DENORMAL_OPERAND:
         lp1 = " Floating Point Denormal Operand Error";
         break;

      case EXCEPTION_FLT_DIVIDE_BY_ZERO:
         lp1 = " Floating Point Divide by Zero Error";
         break;

      case EXCEPTION_FLT_INEXACT_RESULT:
         lp1 = " Floating Point In-Exact Result Error";
         break;

      case EXCEPTION_FLT_INVALID_OPERATION:
         lp1 = " Floating Point Invalid Operation Error";
         break;

      case EXCEPTION_FLT_OVERFLOW:
         lp1 = " Floating Point Overflow Error";
         break;

      case EXCEPTION_FLT_STACK_CHECK:
         lp1 = " Floating Point Stack Check Error";
         break;

      case EXCEPTION_FLT_UNDERFLOW:
         lp1 = " Floating Point Underflow Error";
         break;

      case EXCEPTION_INT_DIVIDE_BY_ZERO:
         lp1 = "n Integer Divide by Zero Error";
         break;

      case EXCEPTION_INT_OVERFLOW:
         lp1 = "n Integer Overflow Error";
         break;

      case EXCEPTION_PRIV_INSTRUCTION:
         lp1 = " Priveleged Instruction Exception";
         break;

      case EXCEPTION_IN_PAGE_ERROR:
         lp1 = "n Invalid Page Fault Exception";
         break;

      case EXCEPTION_ILLEGAL_INSTRUCTION:
         lp1 = "n Illegal Instruction Exception";
         break;

      case EXCEPTION_STACK_OVERFLOW:
         lp1 = " Stack Overflow Exception";
         break;

      case EXCEPTION_GUARD_PAGE:
         lp1 = " 'Guard Page' Access Violation";
         break;


      case EXCEPTION_BREAKPOINT:
      case EXCEPTION_SINGLE_STEP:
         return(EXCEPTION_CONTINUE_SEARCH);


      default:    // UNKNOWN (to me) fault
         lp1 = "n Exception of Unknown Type";
         break;
   }

   *faultbuf = 0;


   // for now, to get the module name, use 'GetModuleFileName()' on
   // the 'root instance' of the application.

   if(GetRootInstance())
   {
      GetModuleFileName(GetRootInstance(), faultbuf, sizeof(faultbuf));
   }

   if(!*faultbuf)            // try *VERY* hard to get this one right!
   {
       GetModuleFileName(GetCurrentProcessId(), faultbuf, sizeof(faultbuf));
       if(!*faultbuf)
       {
          GetModuleFileName(GetCurrentThreadId(), faultbuf, sizeof(faultbuf));
       }
   }


   lp2 = strrchr(faultbuf, '\\');          // find last backslash
   if(lp2) lstrcpy(faultbuf, lp2 + 1);     // trim the qualifying path name
   lp2 = strrchr(faultbuf, '.');           // find the '.' for the extension
   if(lp2) *lp2 = 0;                       // trim off the extension.
   else    lp2 = faultbuf + lstrlen(faultbuf);


   wsprintf(lp2, szFmt1, lp1, dwCS, dwEIP, idThread, (DWORD)hThread);


   MyOutputDebugString(faultbuf);
   MyOutputDebugString(szCRLF);




   // for *NOW* until I have this thing managed properly...

   dw1 = DialogBoxParam(hLibInst, "GP_FAULT0", NULL,
                        MyExceptionDlgProc, faultbuf);


   // depending on return from message box, process the following:

   if(dw1 == IDABORT)       return(EXCEPTION_CONTINUE_SEARCH);
   else if(dw1 == IDCANCEL) return(EXCEPTION_CONTINUE_EXECUTION);
   else if(dw1 == IDOK)     return(EXCEPTION_EXECUTE_HANDLER);
   else
   {
      MessageBox(NULL, "?Error in Thread Exception Processing - terminating thread",
                 "SFT32 Thread Exception", MB_OK | MB_ICONHAND);

   }

   return(EXCEPTION_EXECUTE_HANDLER);  // by default, terminate thread
}


BOOL FAR PASCAL LOADDS MyExceptionDlgProc(HWND hDlg, UINT msg,
                                          WPARAM wParam, LPARAM lParam)
{

   switch(msg)
   {
      case WM_INITDIALOG:

         if(lParam)
         {
            SetDlgItemText(hDlg, 100, (LPSTR)lParam);
         }

         return(TRUE);


      case WM_COMMAND:

         if(wParam == IDOK || wParam == IDABORT || wParam == IDCANCEL)
         {
            EndDialog(hDlg, (WORD)wParam);
            return(TRUE);
         }

         break;
   }

   return(FALSE);

}

#pragma code_seg()



/***************************************************************************/
/*                        ENUMERATION FUNCTIONS                            */
/***************************************************************************/

#pragma code_seg ("MTHREAD_ENUM_TEXT","CODE")

static long MthreadEnumRval = 0;

BOOL LOADDS CALLBACK MthreadEnumFunction(HWND hWnd, LPARAM lParam)
{


   MthreadEnumRval = 0L;               /* initial value for 'return value' */

   switch(LOWORD(lParam))
   {
      case WM_DESTROY:   /* using this function to destroy windows, I see! */
         if(IsWindow(hWnd) &&    /* only if window handle is valid */
            hWnd != GetMasterThreadWindow() &&
            GetParent(hWnd)==(HWND)NULL)
         {

            ShowWindow(hWnd, SW_SHOWNORMAL);  /* show it first! */
            if(IsWindowVisible(hWnd))         /* double check!! */
               SendMessage(hWnd, WM_CLOSE, 0, 0L);
                    /* use 'SendMessage()' to prevent problems by */
                 /* destroying it while the application still lives */
         }
         return(TRUE);   /* this means 'keep on a-going! */

      case WM_COMPAREITEM:      /* I use this to find an 'Instance Window' */

         if(GetWindowLong(hWnd,GWL_HINSTANCE)==(long)HIWORD(lParam))
         {
            MthreadEnumRval = MAKELONG(hWnd,GetWindowTask(hWnd));
                                  /* obtains both window and task handle!! */
            return(FALSE);
         }
         else
         {
            return(TRUE);
         }
      case WM_GETTEXT:    /* I can use this to find a 'Task Window' */
                          /* grab first window in enumeration list  */
                          /* and return its instance handle         */

         MthreadEnumRval = MAKELONG(hWnd,GetWindowLong(hWnd, GWL_HINSTANCE));
                          /* obtains both window and task handle!! */

         return(FALSE);
   }

   return(FALSE);        /* this means 'quit at this point'. */

}

#pragma code_seg()


/***************************************************************************/
/*    MULTI-THREAD 'MAIN TASK WINDOW' EXIT PROCESSING & DEFAULT PROCS      */
/***************************************************************************/


#pragma code_seg ("DLL_CONTROL_TEXT","CODE")

extern "C" VFPPROC MthreadExit(HTASK hTask)      /* exits by killing all task windows */
{
//HTASK hTask2;
//WORD current_task_index;    /* precludes 're-calling' 'GetTaskTableEntry()' */


   if(bWin32sFlag) lpMthreadExit(hTask);
}


#pragma code_seg ("MYTRANSLATE_TEXT","CODE")

LFPPROC DefMthreadMasterProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{

   if(bWin32sFlag)
   {
      return(lpDefMthreadMasterProc(hWnd, wMsg, wParam, lParam));
   }



   if(wMsg==WM_CLOSE || wMsg==WM_DESTROY)     /* window being closed!! */
   {                                          /* (two chances)         */

      MthreadExit((HTASK)NULL);

      RegisterMasterThreadWindow(NULL);  // no more 'master thread window'
   }

   return((LONG)DefWindowProc(hWnd, wMsg, (WPARAM)wParam, (LPARAM)lParam));

}

#pragma code_seg()



/*************************************************************************/
/*                     MESSAGE DISPATCH FUNCTIONS                        */
/*************************************************************************/

#pragma code_seg ("MYTRANSLATE_TEXT","CODE")

void FAR PASCAL EXPORT UpdateThreadPath(HANDLE hThread)
{
   if(bWin32sFlag) lpUpdateThreadPath(hThread);  // the only time it's needed

   // for non-Win32s applications this thing does NOTHING!
}


#pragma auto_inline(off)  // no 'inline' for LoopDispatch(),
                          // MthreadMsgDispatch(), MthreadNewThreadDispatch()

BFPPROC LoopDispatch(void)
{
LPTHREADPARM32 lpTP;
MSG msg;


   if(bWin32sFlag) return(lpLoopDispatch());
   else
   {
      // check for 'soft terminate' flag

      lpTP = (LPTHREADPARM32)GetThreadParmPtr(NULL);

      if(hMutex) WaitForSingleObject(hMutex, INFINITE);

      if(lpTP)
      {
         if(lpTP->dwStatus & THREADSTATUS_CRITICAL)
         {
            if(hMutex) ReleaseMutex(hMutex);
            return(FALSE);  // the 'critical' flag is set
         }
         else if(lpTP->dwStatus & THREADSTATUS_KILL)
         {
            if(hMutex) ReleaseMutex(hMutex);
            return(TRUE);   // the 'QUIT' flag is set
         }
      }

//      FOR NOW, THIS SECTION IS DISABLED
//
//      if(was_quit)                // have I gotten a 'WM_QUIT' message??
//      {
//         if(hMutex) ReleaseMutex(hMutex);
//         return(TRUE);            // the 'QUIT' flag is set
//      }

      // Check to see if there's a 'WM_QUIT' message in a message queue...

      if(PeekMessage(&msg, NULL, WM_QUIT, WM_QUIT, PM_REMOVE | PM_NOYIELD))
      {
         lpTP->dwStatus |= THREADSTATUS_KILL;

         if(hMutex) ReleaseMutex(hMutex);

         return(TRUE);
      }

      if(hMutex) ReleaseMutex(hMutex);

      // If there's some background process I want to place here, I'll
      // leave an open space for it.  Until then, nothing.

   }

   return(FALSE);
}


// TRANSLATION PROC for message loops that call 'MthreadGetMessage()'

BOOL FPPROC MthreadTranslateMessage(LPMSG msg)
{
   if(bWin32sFlag)
   {
      return(lpMthreadTranslateMessage(msg));
   }

   return(MyTranslate(msg));
}



/*** MAIN MESSAGE DISPATCHER FOR MULTI-THREAD PROGRAMS.  RETURNS 'TRUE'  ***/
/*** IF 'WM_QUIT' NOT YET RECEIVED, OTHERWISE 'FALSE' FROM THAT POINT ON ***/


BFPPROC MthreadMsgDispatch(LPMSG msg)
{
BOOL not_quit, SendMessageActive, is_message /*, GoAheadAndYield */;
WORD message = WM_NULL;
HWND hwnd;


   if(bWin32sFlag) return(lpMthreadMsgDispatch(msg));


                    /* this function is thread #0!! */

   if(hdlgLogo)
   {
      CheckOrDestroyLogo();
   }


   SendMessageActive = InSendMessage();
   not_quit = TRUE;                                      /* initial value */

//   GoAheadAndYield = TRUE;     /* first time through only! */

   if(!SendMessageActive && !(disable_dispatch & DISABLE_DISPATCH_MSG))
   {                                  /* normal dispatch loop - disabled? */
//    int current_task_index;       /* copy for use within this scope only */

      INIT_LEVEL

//      if(GoAheadAndYield)
//      {


//         GoAheadAndYield = FALSE;  /* only yield ONE TIME! */
//
//      }
//
//      if(PeekMessage(msg,0,NULL,NULL,PM_NOREMOVE | PM_NOYIELD))
//      {
//         if(((message = msg->message)==WM_QUIT &&
//             (disable_messages & DISABLE_WMQUIT)) ||
//            (message==WM_CLOSE && (disable_messages & DISABLE_WMCLOSE)))
//         {
//            PeekMessage(msg,0,NULL,NULL,PM_NOREMOVE);  /* cause 'yield' */
//            NearThreadDispatch(msg, NULL, FALSE);
//            break;
//         }
//      }

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
            if(!MthreadAppMessageProc(msg))
            {
               if(!MyTranslate(msg))
               {
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

      }

      if(!not_quit)                   /*   WM_QUIT MESSAGE!!!! */
      {
               /** TELL ALL WINDOWS IN THIS TASK TO QUIT **/


         was_quit = TRUE;  /* causes all subsequent calls to execute this */
                           /* section , but still dispatch messages       */
      }


      END_LEVEL

   }
   else
   {
      if(!SendMessageActive)
      {                             /* yield, but don't dispatch messages */
         PeekMessage(msg, NULL, 0, 0, PM_NOREMOVE);
      }

      LoopDispatch();                         /* do at least one thread!! */

   }


   return(!was_quit);

}




#pragma auto_inline(on)


BOOL FAR PASCAL LOADDS MyGetQueueStatus(void)  // TRUE if hardware input in QUEUE
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
               (LVFARPROC)lpGetQueueStatus =
//                                  GetProcAddress(hUser, "GetQueueStatus");
                                  GetProcAddress(hUser, MAKELPC(0, 334));
            }
         }

         AlreadyChecked = TRUE;
      }

      if(!lpGetQueueStatus)
      {
         return(PeekMessage((LPMSG)&msg, NULL, 0, 0,
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


#pragma auto_inline(off) // no 'inline' for MthreadGetMessage()


BFPPROC MthreadGetMessage(LPMSG msg)
{
#ifdef DISPLAY_LOGO
static BOOL not_yet_display_logo = TRUE;
#else
static BOOL not_yet_display_logo = FALSE;
#endif
BOOL wRval=TRUE;
HCURSOR hOldCursor=NULL;
MSG msg2;
UINT w1;



   if(bWin32sFlag) return(lpMthreadGetMessage(msg));


   if(not_yet_display_logo)     /* Only display this once! */
   {
      not_yet_display_logo = FALSE;
      hLogoOwnerTask = (HTASK)GetCurrentProcessId();

      hMthreadIDFocus = GetFocus();

      if(hMthreadIDFocus &&
         GetWindowTask(hMthreadIDFocus) != GetCurrentProcessId())
      {
         htaskMthreadIDFocus = GetWindowTask(hMthreadIDFocus);
      }
      else
      {
         htaskMthreadIDFocus = NULL;
      }

      if(!display_wdbutil_logo)
      {
         hdlgLogo = CreateDialog(hLibInst,"ABOUTMTHREAD",NULL,MthreadIDDlgProc);
      }
      else
      {
         hdlgLogo = CreateDialog(hLibInst,"ABOUTDBUTIL",NULL,MthreadIDDlgProc);
      }

      dwLogoTime = GetTickCount();

      if(!SetTimer(hdlgLogo, (WORD)-1, 2000, NULL))
      {
         PostMessage(hdlgLogo, WM_TIMER, 0, 0);  // send timer message to QUIT it!
      }
   }
   else if(hdlgLogo)
   {
      CheckOrDestroyLogo();
   }


//   // Here's the logic of this system:
//   //  1:  if no threads are running, work just like GetMessage()
//   //  2:  if threads are running and messages are in the queue, call
//   //      GetMessage() and return
//   //  3:  if threads are running and messages are NOT in the queue,
//   //      make a call to 'NearThreadDispatch()' to allow at least
//   //      one thread to run right now (with appropriate yields if there
//   //      are keyboard or mouse messages for other applications)
//
//   if(hInternalThreadList && wInternalThreadCount)
//   {
//      do
//      {
//
//         if(!InsideHook && MyGetQueueStatus() &&
//            PeekMessage(msg, NULL, NULL, NULL, PM_NOREMOVE | PM_NOYIELD))
//         {
//            PeekMessage(msg, NULL, NULL, NULL, PM_NOREMOVE); // YIELD once
//
////            if(!InsideHook)
////            {
//               if(PeekMessage(msg, NULL, NULL, NULL, PM_REMOVE | PM_NOYIELD))
//               {
//                  wRval = msg->message!=WM_QUIT;
//                  break;
//               }
////            }
//
////            wRval = GetMessage(msg, NULL, NULL, NULL);
////            break;
//         }
//         else
//         {
//             // commented out 'GetInputState()' since the same checks
//             // are already provided (better?) by 'NearThreadDispatch()'
//
////            if(!GetInputState())  /* no keyboard input to process anywhere */
////            {
//               NearThreadDispatch(msg, NULL, 1); // threads will alternate
//                                                 // with thread #0!
////            }
//
//
////               /** BUG SWATTER - will this help?? **/
////
////            while(InsideHook && NearThreadDispatch(msg, NULL, 1))
////               ;
//
//
//            if(PeekMessage(msg, NULL, NULL, NULL, PM_NOREMOVE)) /* yield */
//            {
//
////               /** BUG SWATTER - will this help?? **/
////
////               while(InsideHook && NearThreadDispatch(msg, NULL, 1))
////                  ;
//
//               if(PeekMessage(msg, NULL, NULL, NULL,
//                              PM_NOREMOVE | PM_NOYIELD))
//               {
//                  PeekMessage(msg, NULL, NULL, NULL, PM_NOREMOVE);
//
////                  if(!InsideHook)
////                  {
////                     if(PeekMessage(msg, NULL, NULL, NULL, PM_REMOVE | PM_NOYIELD))
////                     {
////                        wRval = msg->message!=WM_QUIT;
////                        break;
////                     }
//
//                     wRval = GetMessage(msg, NULL, NULL, NULL);
//                     break;
////                  }
//               }
//            }
//         }
//
//         // As part of loop, check on status of 'logo'
//
//         if(hdlgLogo)
//         {
//            CheckOrDestroyLogo();
//         }
//
//      } while(TRUE);
//   }
//   else if(hdlgLogo)

   if(hdlgLogo)
   {
      while(hdlgLogo && !PeekMessage(msg, NULL, 0, 0, PM_NOREMOVE))
      {
         CheckOrDestroyLogo();
      }

      wRval = GetMessage(msg, NULL, 0, 0);
   }
   else
   {
      wRval = GetMessage(msg, NULL, 0, 0);
   }



   if(msg->message==WM_QUIT ||              /* QUIT or Windows is EXITING! */
       (msg->message==WM_ENDSESSION && msg->wParam))
   {

      if(msg->message == WM_QUIT &&   // somehow a WM_QUIT message was posted
         (disable_messages & DISABLE_WMQUIT))  // no WM_QUIT messages allowed
      {
       static const char CODE_BASED szMsg[]=
                   "*W* WM_QUIT message disabled in MthreadGetMessage()\r\n";

         MyOutputDebugString(szMsg);

         msg->hwnd    = NULL;                   // application message (now)
         msg->message = THREAD_STATUS;
         msg->wParam  = (WPARAM)0;              // root task information
         msg->lParam  = MAKELPARAM(WM_QUIT, 0); // the mesage that was
                                                // disabled goes here

         return(TRUE);                          // no longer a 'WM_QUIT'

      }


//      wRval = was_quit;
//      was_quit = TRUE;  /* forces threads to terminate; returns FALSE here */


      hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));


      // The only way to tell ALL of the threads to terminate would
      // be to individually call 'KillThread' for each, then wait
      // until the thread count is down to 1 (this one) plus any
      // other 'system' threads that are running.


      for(w1=0; w1<uiThreadCount; w1++)
      {
         MthreadKillThread(lpThreadHandle[w1]);
      }

      while(uiThreadCount > 1)
      {
         Sleep(50);  // allow the threads to shut down normally
      }


      SetCursor(hOldCursor);      /* threads have now been removed.  Good! */


//      was_quit = wRval;              /* restore "OLD" value for 'was_quit' */

      wRval = FALSE;
   }
   else if(msg->message == WM_CLOSE)     // see if we're disabling WM_CLOSE
   {

      if(disable_messages & DISABLE_WMCLOSE)
      {
       static const char CODE_BASED szMsg[]=
                     "*W* WM_CLOSE message disabled in MthreadGetMessage()\r\n";

         MyOutputDebugString(szMsg);

         msg->message = THREAD_STATUS;
         msg->wParam  = (WPARAM)0;              // root task information
         msg->lParam  = MAKELPARAM(WM_CLOSE,    // the mesage that was
                               (WORD)msg->hwnd);// disabled goes here
         msg->hwnd    = NULL;                   // application message (now)

         return(TRUE);                          // no longer a 'WM_CLOSE'
      }
   }
   else if(msg->message==WM_ENTERIDLE)  /* helps threads within menus! */
   {
      if(!PeekMessage((LPMSG)&msg2, NULL, THREAD_IDLE, THREAD_IDLE,
                      PM_NOYIELD | PM_NOREMOVE))
      {
         PostAppMessage(GetCurrentProcessId(), THREAD_IDLE, 0, 0);
      }
   }
//   else if(msg->message==WM_WININICHANGE)
//   {
//      WinIniChangeInternational();  // update 'international' settings for
//                                    // date, time, etc.
//   }
   else if(msg->message==WM_COMPACTING && msg->hwnd)
   {
    static const char CODE_BASED szMsg[]=
             "*W* WM_COMPACTING message detected in MthreadGetMessage()\r\n";

      // for COMPACTING messages, don't "eat" the message; just post it
      // to the application queue for later digesting...

      PostAppMessage(GetCurrentProcessId(), WM_COMPACTING,
                     msg->wParam, msg->lParam);

      MyOutputDebugString(szMsg);
   }
//   else if(msg->message==WM_WININICHANGE)   // change to 'WIN.INI'!
//   {
//    static const char CODE_BASED szMsg[]=
//           "*W* WM_WININICHANGE message detected in MthreadGetMessage()\r\n";
//
//      WinIniChangeInternational();
//
//      MyOutputDebugString(szMsg);
//   }

   return(wRval);

}

#pragma auto_inline(on)

#pragma code_seg()



/***************************************************************************/
/*                         MthreadAppMessageProc()                         */
/*                                                                         */
/* This next proc is executed whenever an APPLICATION message needs to be  */
/* processed.                                                              */
/* If message is processed, it returns TRUE; otherwise it returns FALSE.   */
/*                                                                         */
/***************************************************************************/


#pragma code_seg ("MYTRANSLATE_TEXT","CODE")

BOOL FAR PASCAL MthreadAppMessageProc(LPMSG msg)
{

             /**********************************************/
             /*          APPLICATION MESSAGES              */
             /**********************************************/




    switch(msg->message)  /* see if it's a thread message! */
    {

                  /*** APPLICATION MESSAGES ***/

       case NO_MASTER_THREAD:

          break;     /* this says 'there was a master thread, but */
                     /* now it ain't there no more!!!             */

       case WM_SEMAPHORECALLBACK:

          MthreadSemaphoreCallback((HANDLE)msg->wParam,
                                   (SEMAPHORECALLBACK)msg->lParam);
          return(TRUE);                 /* 'I handled this message too' */


         // THREAD REGISTRATION MESSAGES - In case I decide I need to process
         //                                them in any other fashion...

       case THREAD_REGISTER:      // thread spawned

       case THREAD_ACKNOWLEDGE:   // thread running

       case THREAD_COMPLETE:      // thread finished

       default:                   // ALL OTHER TASK MESSAGES!

          if(AppMessageProc)
          {
             AppMessageProc(msg);
             return(TRUE);
          }

          return(FALSE);

    }

    return(TRUE);                    /* this says 'I handled this message' */
}




/***************************************************************************/
/*                        THREAD UTILITY FUNCTIONS                         */
/*                                                                         */
/* MthreadKillThread, MthreadSuspendThread, MthreadResumeThread, MthreadWaitForThread, etc. etc. etc.  */
/*                                                                         */
/***************************************************************************/


// 'MthreadSleep(n)' will wait for 'n' milliseconds, approximately.

void FPPROC MthreadSleep(UINT uiTime)
{
DWORD dwTickCount = GetTickCount();
long l1;
LPTHREADPARM32 lpTP;


   if(bWin32sFlag)
   {
      lpMthreadSleep(uiTime);
   }
   else
   {
      lpTP = (LPTHREADPARM32)GetThreadParmPtr(NULL);

      if(lpTP)
      {
         if(hMutex) WaitForSingleObject(hMutex, INFINITE);
         lpTP->dwStatus |= THREADSTATUS_SLEEP;       // assign correct flags
         if(hMutex) ReleaseMutex(hMutex);

         while((l1 = uiTime - (long)(GetTickCount() - dwTickCount)) > 0)
         {
            if(lpTP->dwStatus & THREADSTATUS_KILL) break;

            if(l1 > 0 && l1 < 50) Sleep(l1);
            else                  Sleep(50);  // 50 msec increments (just because)
         }

         if(hMutex) WaitForSingleObject(hMutex, INFINITE);
         lpTP->dwStatus &= ~THREADSTATUS_SLEEP;      // clear correct flags
         if(hMutex) ReleaseMutex(hMutex);
      }
      else
      {
         Sleep(uiTime);
      }
   }
}


BFPPROC MthreadKillThread(HANDLE hThreadID)
{
LPTHREADPARM32 lpTP;


   // NOTE:  This differs from normal 'TerminateThread()' method in that
   //        the thread must call 'LoopDispatch()' to determine if
   //        it's being killed or not...
   //        The user should call THIS before calling 'TerminateThread()'
   //        (calling 'CloseHandle()' does *NOT* terminate the thread!!)

   if(bWin32sFlag) return(lpMthreadKillThread(hThreadID));


   __try
   {

      lpTP = (LPTHREADPARM32)GetThreadParmPtr(hThreadID);

      if(lpTP)
      {
         if(hMutex) WaitForSingleObject(hMutex, INFINITE);
         lpTP->dwStatus |= THREADSTATUS_KILL;         // assign correct flags
         if(hMutex) ReleaseMutex(hMutex);

         return(!PostThreadMessage(lpTP->idSelf, WM_QUIT, 0, 0));
                                                   // post QUIT message too!

      }
      else
      {
         return(!PostThreadMessage((DWORD)hThreadID, WM_QUIT, 0, 0));
      }
   }
   __except(EXCEPTION_EXECUTE_HANDLER)
   {
      return(TRUE);  // error code
   }
}


BFPPROC MthreadSuspendThread(HANDLE hThreadID)
{
LPTHREADPARM32 lpTP;
DWORD dw1;


   if(bWin32sFlag) return(lpMthreadSuspendThread(hThreadID));


   __try
   {
      lpTP = (LPTHREADPARM32)GetThreadParmPtr(hThreadID);


      // *IF* I am suspending myself, then I must do so *THIS* way...

      if(hThreadID==NULL || hThreadID == GetCurrentThread() ||
         (lpTP && lpTP->hThread == hThreadID))
      {
         if(hMutex) WaitForSingleObject(hMutex, INFINITE);

//         dw1 = GetThreadPriority(hThreadID);
//         SetThreadPriority(hThreadID, THREAD_PRIORITY_TIME_CRITICAL);


         lpTP->dwStatus |= THREADSTATUS_SUSPEND;      // assign correct flags

         if(!hThreadID)
         {
            if(lpTP) hThreadID = lpTP->hThread;
            else     hThreadID = GetCurrentThread();
         }

         if(hMutex)
         {
            Sleep(0);                         // release time slice NOW
            ReleaseMutex(hMutex);  // critical priority - release mutex
         }

         SuspendThread(hThreadID); // and IMMEDIATELY suspend the thread. WHew!

//         if(GetThreadPriority(hThreadID) ==  THREAD_PRIORITY_TIME_CRITICAL)
//         {
//            SetThreadPriority(hThreadID, dw1); // restore old priority if it's
//                                               // THREAD_PRIORITY_TIME_CRITICAL
//         }

         return(FALSE);                               // assume it worked
      }


      if(hMutex) WaitForSingleObject(hMutex, INFINITE);


      if((dw1 = SuspendThread(hThreadID)) != 0xffffffff)
      {
         // FYI 'dw1' contains the current 'suspend' count minus 1

         lpTP = (LPTHREADPARM32)GetThreadParmPtr(hThreadID);

         if(lpTP)
         {
            lpTP->dwStatus |= THREADSTATUS_SUSPEND;      // assign correct flags
         }

         if(hMutex) ReleaseMutex(hMutex);
         return(FALSE);
      }
      else
      {
         if(hMutex) ReleaseMutex(hMutex);
         return(TRUE);
      }
   }
   __except(EXCEPTION_EXECUTE_HANDLER)
   {
      return(TRUE);  // error code
   }
}



BFPPROC MthreadResumeThread(HANDLE hThreadID)
{
LPTHREADPARM32 lpTP;
DWORD dw1;


   if(bWin32sFlag) return(lpMthreadResumeThread(hThreadID));


   __try
   {
      if(hMutex) WaitForSingleObject(hMutex, INFINITE);


      if((dw1 = ResumeThread(hThreadID)) != 0xffffffff)
      {
         lpTP = (LPTHREADPARM32)GetThreadParmPtr(hThreadID);

         if(lpTP && dw1 <= 1)
         {
            lpTP->dwStatus &= ~THREADSTATUS_SUSPEND;     // clear correct flags
         }

         if(hMutex) ReleaseMutex(hMutex);
         return(FALSE);
      }
      else
      {
         if(hMutex) ReleaseMutex(hMutex);
         return(TRUE);
      }
   }
   __except(EXCEPTION_EXECUTE_HANDLER)
   {
      return(TRUE);  // error code
   }
}


BFPPROC MthreadWaitForThread(HANDLE hThreadID)
{
DWORD dw1;
LPTHREADPARM32 lpTP;


   if(bWin32sFlag) return(lpMthreadWaitForThread(hThreadID));


   __try
   {
      lpTP = (LPTHREADPARM32)GetThreadParmPtr(NULL);

      if(hMutex) WaitForSingleObject(hMutex, INFINITE);
      lpTP->dwStatus |= THREADSTATUS_WAITTHREAD;
      if(hMutex) ReleaseMutex(hMutex);

      do
      {
         dw1 = WaitForSingleObject(hThreadID, 50);  // wait 50 msec

         if(dw1 == WAIT_FAILED) break;


         // here, check for 'soft terminate'.

         if(lpTP->dwStatus & THREADSTATUS_KILL)
         {
            dw1 = WAIT_FAILED;
            break;
         }

      } while(dw1 != WAIT_OBJECT_0);   // wait for thread to end

      if(hMutex) WaitForSingleObject(hMutex, INFINITE);
      lpTP->dwStatus &= ~THREADSTATUS_WAITTHREAD;
      if(hMutex) ReleaseMutex(hMutex);

      return(dw1 != WAIT_OBJECT_0);                    // good result == 0
   }
   __except(EXCEPTION_EXECUTE_HANDLER)
   {
      return(TRUE);  // error code
   }
}




BFPPROC MthreadThreadCritical(HANDLE hThreadID, BOOL bCriticalFlag)
{
LPTHREADPARM32 lpTP;


   if(bWin32sFlag) return(lpMthreadResumeThread(hThreadID));


   __try
   {
      lpTP = (LPTHREADPARM32)GetThreadParmPtr(hThreadID);

      if(lpTP)
      {
         if(hMutex) WaitForSingleObject(hMutex, INFINITE);

         if(bCriticalFlag)
         {
            lpTP->dwStatus |= THREADSTATUS_CRITICAL;     // assign correct flags
         }
         else
         {
            lpTP->dwStatus &= ~THREADSTATUS_CRITICAL;    // clear correct flags
         }

         if(hMutex) ReleaseMutex(hMutex);
      }

      return(!lpTP);
   }
   __except(EXCEPTION_EXECUTE_HANDLER)
   {
      return(TRUE);  // error code
   }
}


BFPPROC MthreadThreadExclusive(HANDLE hThreadID, BOOL bExclusiveFlag)
{
LPTHREADPARM32 lpTP;


   if(bWin32sFlag) return(lpMthreadResumeThread(hThreadID));

   __try
   {
      // I can't do "EXCLUSIVE" here, but I can assign a VERY high priority...

      lpTP = (LPTHREADPARM32)GetThreadParmPtr(hThreadID);

      if(lpTP)
      {
         if(hMutex) WaitForSingleObject(hMutex, INFINITE);

         if(bExclusiveFlag)
         {
            lpTP->dwStatus |= THREADSTATUS_EXCLUSIVE;    // assign correct flags
         }
         else
         {
            lpTP->dwStatus &= ~THREADSTATUS_EXCLUSIVE;   // clear correct flags
         }

         if(hMutex) ReleaseMutex(hMutex);
      }

      if(bExclusiveFlag)
      {
         return(!SetThreadPriority(hThreadID,THREAD_PRIORITY_TIME_CRITICAL));
      }
      else
      {
         return(!SetThreadPriority(hThreadID,THREAD_PRIORITY_NORMAL));
      }
   }
   __except(EXCEPTION_EXECUTE_HANDLER)
   {
      return(TRUE);  // error code
   }
}


#pragma code_seg()



/**************************************************************************/
/*                 REGISTRATION/ENABLE/DISABLE FUNCTIONS                  */
/**************************************************************************/

#pragma code_seg("MTHREAD_UTILITY","CODE")


VFPPROC MthreadDisableMessages(void)
{
   if(bWin32sFlag)
   {
      lpMthreadDisableMessages();
      return;
   }


   if(hMutex) WaitForSingleObject(hMutex, INFINITE);

   disable_dispatch |= DISABLE_DISPATCH_MSG;  /* no messages will dispatch */

   disable_dispatch = (disable_dispatch & ~DISABLE_DISPATCH_MASK)
                    | ((disable_dispatch & DISABLE_DISPATCH_MASK) + 1);

   if(hMutex) ReleaseMutex(hMutex);
}


VFPPROC MthreadEnableMessages(void)
{
   if(bWin32sFlag)
   {
      lpMthreadEnableMessages();
      return;
   }


   if(hMutex) WaitForSingleObject(hMutex, INFINITE);

   if(disable_dispatch & DISABLE_DISPATCH_MASK)
   {

      disable_dispatch = (disable_dispatch & ~DISABLE_DISPATCH_MASK)
                       | ((disable_dispatch & DISABLE_DISPATCH_MASK) - 1);
   }

   if(!(disable_dispatch & DISABLE_DISPATCH_MASK))  /* count is now zero */
   {
      disable_dispatch &= ~DISABLE_DISPATCH_MSG;  /* turn bit OFF */
   }

   if(hMutex) ReleaseMutex(hMutex);
}


VFPPROC RegisterCompactProc(FARPROC lpproc)
{
   if(bWin32sFlag)
   {
      lpRegisterCompactProc(lpproc);
      return;
   }


   if(hMutex) WaitForSingleObject(hMutex, INFINITE);

   CompactProc = (void (FAR PASCAL *)(void))lpproc;

   if(hMutex) ReleaseMutex(hMutex);
}

VFPPROC RegisterAppMessageProc(
                   void (FAR PASCAL *_AppMessageProc)(LPMSG lpmsg))
{
   if(bWin32sFlag)
   {
      lpRegisterAppMessageProc(_AppMessageProc);
      return;
   }


   if(hMutex) WaitForSingleObject(hMutex, INFINITE);

   AppMessageProc = (void (FAR PASCAL *)(LPMSG))_AppMessageProc;

   if(hMutex) ReleaseMutex(hMutex);
}

BFPPROC RegisterAccelerators(HANDLE hAccel, HWND hwnd)
{   /* registers an accellerator table for a window (-1 == all windows) */
int i;
SFTACCELERATORS AccelEntry;


   if(bWin32sFlag)
   {
      return(lpRegisterAccelerators(hAccel, hwnd));
   }


   if(hMutex) WaitForSingleObject(hMutex, INFINITE);

   AccelEntry.hWnd = hwnd;
   AccelEntry.hAccel = hAccel;

   if(Accelerators==NULL)
   {
      Accelerators = (LPSFTACCELERATORS)
             GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT | GMEM_DDESHARE,
                            sizeof(SFTACCELERATORS) * MAX_ACCEL);

      if(!Accelerators)
      {
         if(hMutex) ReleaseMutex(hMutex);

         return(TRUE);
      }
   }

   for(i=0; i<MAX_ACCEL &&
            memcmp(Accelerators + i,&AccelEntry,sizeof(AccelEntry)); i++)
   {
      if(!Accelerators[i].hWnd && !Accelerators[i].hAccel)
      {
         Accelerators[i] = AccelEntry;
         break;
      }
   }

   if(hMutex) ReleaseMutex(hMutex);

   if(i>=MAX_ACCEL)
      return(TRUE);     /* error!  no room for accel table! */
   else
      return(FALSE);    /* Handle added (or else already in table!) */
}

BFPPROC RemoveAccelerators(HANDLE hAccel, HWND hwnd)
{
int i, j;
SFTACCELERATORS AccelEntry;


   if(bWin32sFlag)
   {
      return(lpRemoveAccelerators(hAccel, hwnd));
   }


   if(!Accelerators) return(TRUE);


   if(hMutex) WaitForSingleObject(hMutex, INFINITE);

   AccelEntry.hWnd = hwnd;
   AccelEntry.hAccel = hAccel;

   for(i=0; i<MAX_ACCEL; i++)
   {
      if(!memcmp(Accelerators + i,&AccelEntry,sizeof(AccelEntry)) ||
         (!hwnd && Accelerators[i].hAccel==hAccel))
      {
         for(j=i; j<(MAX_ACCEL-1); j++)
         {
            Accelerators[j] = Accelerators[j+1];  /* compact listing */
         }

         Accelerators[MAX_ACCEL-1].hWnd = NULL;  /* flag it as unused */
         Accelerators[MAX_ACCEL-1].hAccel = NULL;

         if(hwnd) break; /* repeat for NULL 'hwnd', but exit now otherwise */
      }
   }

   if(hMutex) ReleaseMutex(hMutex);

   if(i>=MAX_ACCEL)
      return(TRUE);     /* error!  didn't find accel table! */
   else
      return(FALSE);    /* Handle removed */
}



BFPPROC RegisterDialogWindow(HWND hDlg)
{
WORD w;


   if(bWin32sFlag)
   {
      return(lpRegisterDialogWindow(hDlg));
   }


   if(hMutex) WaitForSingleObject(hMutex, INFINITE);

   if(DialogWindowList==NULL)
   {
      DialogWindowList = (HWND FAR *)
               GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT | GMEM_DDESHARE,
                              sizeof(HWND) * MAX_MDIACCEL);

      if(!DialogWindowList)
      {
         if(hMutex) ReleaseMutex(hMutex);

         return(TRUE);
      }
   }

   for(w=0; DialogWindowList[w] && w<(MAX_MDIACCEL - 1); w++)
   {
      if(hDlg==DialogWindowList[w])
      {
         if(hMutex) ReleaseMutex(hMutex);

         return(FALSE);           // already there...
      }
   }

   if(w>=(MAX_MDIACCEL - 1)) return(TRUE);  // return error

   DialogWindowList[w++] = hDlg;
   DialogWindowList[w] = NULL;   // ensure end is properly marked!


   if(hMutex) ReleaseMutex(hMutex);

   return(FALSE);  // it worked!
}

VFPPROC UnregisterDialogWindow(HWND hDlg)
{
WORD w1,w2;


   if(bWin32sFlag)
   {
      lpUnregisterDialogWindow(hDlg);
      return;
   }

   if(!DialogWindowList) return;


   if(hMutex) WaitForSingleObject(hMutex, INFINITE);

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

   if(hMutex) ReleaseMutex(hMutex);
}



VFPPROC EnableMDIAccelerators(HWND hMDIClient)
{
WORD w;


   if(bWin32sFlag)
   {
      lpEnableMDIAccelerators(hMDIClient);
      return;
   }


   if(hMutex) WaitForSingleObject(hMutex, INFINITE);

   if(MDIAcceleratorsClient==NULL)
   {
      MDIAcceleratorsClient = (HWND FAR *)
              GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT | GMEM_DDESHARE,
                             sizeof(HWND) * MAX_MDIACCEL);

      if(!MDIAcceleratorsClient)
      {
         if(hMutex) ReleaseMutex(hMutex);

         return;  /* an error, but... */
      }
   }

   for(w=0; w<MDIAcceleratorsEnable && w<MAX_MDIACCEL; w++)
   {
      if(hMDIClient==MDIAcceleratorsClient[w])
      {
         if(hMutex) ReleaseMutex(hMutex);

         return;
      }
   }

   if(w>=MAX_MDIACCEL)
   {
      if(hMutex) ReleaseMutex(hMutex);

      return;  /* no error code, but... can't do it! */
   }

   MDIAcceleratorsClient[MDIAcceleratorsEnable++] = hMDIClient;

   if(hMutex) ReleaseMutex(hMutex);
}


VFPPROC DisableMDIAccelerators(HWND hMDIClient)
{
WORD w1,w2;


   if(bWin32sFlag)
   {
      lpDisableMDIAccelerators(hMDIClient);
      return;
   }

   if(!MDIAcceleratorsClient)
   {
      if(hMutex) ReleaseMutex(hMutex);

      return;
   }

   if(hMutex) WaitForSingleObject(hMutex, INFINITE);

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

   if(hMutex) ReleaseMutex(hMutex);
}


VFPPROC EnableTranslateMessage(void)
{
   if(bWin32sFlag)
   {
      lpEnableTranslateMessage();
      return;
   }


   if(hMutex) WaitForSingleObject(hMutex, INFINITE);

   TranslateEnable = TRUE;

   if(hMutex) ReleaseMutex(hMutex);
}

VFPPROC DisableTranslateMessage(void)
{
   if(bWin32sFlag)
   {
      lpDisableTranslateMessage();
      return;
   }


   if(hMutex) WaitForSingleObject(hMutex, INFINITE);

   TranslateEnable = FALSE;

   if(hMutex) ReleaseMutex(hMutex);
}


VFPPROC EnableQuitMessage(void)
{
   if(bWin32sFlag)
   {
      lpEnableQuitMessage();
      return;
   }


   if(hMutex) WaitForSingleObject(hMutex, INFINITE);

   disable_messages &= ~DISABLE_WMQUIT;

   if(hMutex) ReleaseMutex(hMutex);
}


VFPPROC DisableQuitMessage(void)
{
   if(bWin32sFlag)
   {
      lpDisableQuitMessage();
      return;
   }


   if(hMutex) WaitForSingleObject(hMutex, INFINITE);

   disable_messages |= DISABLE_WMQUIT;

   if(hMutex) ReleaseMutex(hMutex);
}


VFPPROC EnableCloseMessage(void)
{
   if(bWin32sFlag)
   {
      lpEnableCloseMessage();
      return;
   }


   if(hMutex) WaitForSingleObject(hMutex, INFINITE);

   disable_messages &= ~DISABLE_WMCLOSE;

   if(hMutex) ReleaseMutex(hMutex);
}


VFPPROC DisableCloseMessage(void)
{
   if(bWin32sFlag)
   {
      lpDisableCloseMessage();
      return;
   }


   if(hMutex) WaitForSingleObject(hMutex, INFINITE);

   disable_messages |= DISABLE_WMCLOSE;

   if(hMutex) ReleaseMutex(hMutex);
}

#pragma code_seg()


/***************************************************************************/
/*                                                                         */
/*       MULTI-THREAD APPLICATION PROCESSING & INFORMATION FUNCTIONS       */
/*                                                                         */
/***************************************************************************/



#pragma code_seg ("MTHREAD_UTILITY","CODE")

HFPPROC GetRootApplication(void)
{
   if(bWin32sFlag)
   {
      return(lpGetRootApplication());
   }

   return(hRootApp);
}


HFPPROC GetRootInstance(void)
{
   if(bWin32sFlag)
   {
      return(lpGetRootInstance());
   }

   if(hRootInst) return(hRootInst);

   return(GetTaskInstance(NULL));
}


HINSTANCE FPPROC GetCurrentInstance(void)
{
   if(bWin32sFlag)
   {
      return(lpGetCurrentInstance());
   }

   return(GetTaskInstance(NULL));
}


HFPPROC MthreadGetCurrentThread(void)
{
LPCTHREADPARM32 lpTP;

   if(bWin32sFlag)
   {
      return(lpMthreadGetCurrentThread());
   }

   lpTP = GetThreadParmPtr(NULL);
   if(lpTP) return(lpTP->hThread);

   return(GetCurrentThread());
}


WFPPROC GetNumberOfThreads(void)
{
WORD wRval = 0;
HANDLE hSnap;
THREADENTRY32 te;


   if(bWin32sFlag)
   {
      return(lpGetNumberOfThreads());
   }

   if(!lpCreateToolhelp32Snapshot) return(uiThreadCount);  // simpler method

                  // the method below is MOST reliable!

   hSnap = lpCreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);

   if(!hSnap) return(0);

   // walk the thread list and count them!

   memset(&te, 0, sizeof(te));
   te.dwSize = sizeof(te);

   if(lpThread32First(hSnap, &te))
   {
      wRval = 1;

      while(lpThread32Next(hSnap, &te)) wRval++;
   }

   CloseHandle(hSnap);

   return(wRval);            /* the current number of threads besides root */
}


HWND FPPROC RegisterMasterThreadWindow(HWND hWnd)
{
   if(bWin32sFlag)
   {
      return(lpRegisterMasterThreadWindow(hWnd));
   }

   return(hMasterThreadWindow = hWnd);
}


HWND FPPROC GetMasterThreadWindow(void)
{
   if(bWin32sFlag)
   {
      return(lpGetMasterThreadWindow());
   }

   return(hMasterThreadWindow);
}


HWND FPPROC GetMthreadWindow(void) /* gets MTHREAD (iconic) window handle  */
{
LPCTHREADPARM32 lpTP;

   if(bWin32sFlag)
   {
      return(lpMthreadGetCurrentThread());
   }

   lpTP = GetThreadParmPtr(NULL);
   if(lpTP) return(lpTP->hwndThread);

   return(NULL);
}


BFPPROC IsThread(void)       /* returns TRUE if task is an external thread */
{                            /* or called from within an 'internal' thread */
   if(bWin32sFlag)
   {
      return(lpIsThread());
   }

   return(TRUE);             // this library ALWAYS returns TRUE
}


BFPPROC IsThreadHandle(HANDLE hThread)
{
   if(bWin32sFlag)
   {
      return(lpIsThreadHandle(hThread));
   }

   if(IsInternalThread(hThread)) return(TRUE);  /* YEP! It is!! */

       /* here is where I *would* check for EXTERNAL threads */

   return(FALSE);

}


BOOL FAR PASCAL IsHandleValid(HANDLE hItem)
{
   return(hItem != NULL &&
          hItem != INVALID_HANDLE_VALUE); // that's all I CAN do!  For now...
}


BOOL FAR PASCAL IsInternalThread(HANDLE hThread)
{
UINT w1;

   if(bWin32sFlag)
   {
      return(lpIsInternalThread(hThread));
   }


   // check through my list of HANDLES to see if this one is in the
   // list someplace...

   for(w1=0; w1<uiThreadCount; w1++)
   {
      if(lpThreadHandle[w1] == hThread) return(TRUE);
   }

   return(FALSE);
}



/***************************************************************************/
/*     TASK/INSTANCE INFORMATION FUNCTIONS - ENHANCEMENTS TO WINDOWS!!     */
/***************************************************************************/

/* GetInstanceWindow - Returns the first PARENT window found which belongs */
/*                     to the desired instance.                            */

HWND FPPROC GetInstanceWindow(HINSTANCE hInst)
{

   if(bWin32sFlag)
   {
      return(lpGetInstanceWindow(hInst));
   }

   return(NULL);  // for now

//   EnumWindows(MthreadEnumFunction, MAKELPARAM(WM_COMPAREITEM,hInst));
//
//   return((HWND)LOWORD(MthreadEnumRval));
}


HTASK FPPROC GetInstanceTask(HINSTANCE hInst)
{
HANDLE hSnap;
PROCESSENTRY32 pe;


   if(bWin32sFlag)
   {
      return(lpGetInstanceTask(hInst));
   }


   if(!lpCreateToolhelp32Snapshot)
   {
      if(!hInst) return((HTASK)hRootInst);

      return((HTASK)hInst);  // task == instance handle (?)
   }



   hSnap = lpCreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

   if(!hSnap) return(0);

   // walk the task list and count them!

   memset(&pe, 0, sizeof(pe));
   pe.dwSize = sizeof(pe);

   if(lpProcess32First(hSnap, &pe))
   {
      do
      {
         if(pe.th32ModuleID == (DWORD)hInst ||
            pe.th32ProcessID == (DWORD)hInst)
         {
            return((HTASK)pe.th32ProcessID);
         }

      } while(lpProcess32Next(hSnap, &pe));
   }

   CloseHandle(hSnap);

   return(NULL);
}



HINSTANCE FPPROC GetTaskInstance(HTASK hTask)
{
HANDLE hSnap;
PROCESSENTRY32 pe;


   if(bWin32sFlag)
   {
      return(lpGetTaskInstance(hTask));
   }

//   if(!lpCreateToolhelp32Snapshot) return(NULL);  // not possible (yet)

   if(!lpCreateToolhelp32Snapshot)
   {
      if(!hTask) return(hRootInst);
      return((HINSTANCE)hTask);  // task == instance handle
   }


   hSnap = lpCreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

   if(!hSnap) return(0);

   // walk the task list and count them!

   memset(&pe, 0, sizeof(pe));
   pe.dwSize = sizeof(pe);

   if(lpProcess32First(hSnap, &pe))
   {
      do
      {
//         if(pe.th32ProcessID == (DWORD)hTask) return((HINSTANCE)pe.th32ModuleID);
         if(pe.th32ProcessID == (DWORD)hTask) return((HINSTANCE)pe.th32ProcessID);

      } while(lpProcess32Next(hSnap, &pe));
   }

   CloseHandle(hSnap);

   return(NULL);
}





/***************************************************************************/
/*  MESSAGE 'BROADCAST' FUNCTION - Broadcast message to all child windows  */
/***************************************************************************/


BOOL FAR PASCAL BroadcastMessageToChildren(HWND hWnd, UINT msg, WPARAM wParam,
                                           LPARAM lParam)
{
BOOL rval = 0;
HWND hChild=NULL;

   hChild = (HWND)GetWindow(hWnd, GW_CHILD);  /* get 1st child window */

   while(hChild!=(HANDLE)NULL)
   {
      if(PostMessage((HWND)hChild, msg, (WPARAM)wParam, (LPARAM)lParam))  /* message sent? */
         rval++;

      hChild = (HWND)GetWindow(hChild, GW_HWNDNEXT);/* get next window in list */
   }

   return(rval);                  /* returns # of messages actually posted */

}

#pragma code_seg()




// SPECIAL PROCS WHICH ARE USED BY 'WDBUTIL.DLL' WHEN A HELPER TASK
// HAS BEEN INVOKED!!!

static UINT wHelperPDB = 0;

void FAR PASCAL EXPORT SetHelperTaskPDB(UINT wPDB)
{
   if(bWin32sFlag) lpSetHelperTaskPDB(wPDB);
}


WORD FAR PASCAL EXPORT GetHelperTaskPDB(void)
{
   if(bWin32sFlag) return(lpGetHelperTaskPDB());

   return(0);
}







/***************************************************************************/
/*                                                                         */
/*           The following procs were stolen from 'mthread3.c'             */
/*                                                                         */
/***************************************************************************/



#pragma auto_inline( off )

void FAR PASCAL MyFatalAppExit(int iCode, LPCSTR lpcMsg)
{
DWORD wEIP;
char tbuf[80];
static const char CODE_BASED szCRLF[]="\r\n";
static const char CODE_BASED szFmt[]=
                            "?Call to 'WMTHREAD.MyFatalAppExit()' from %08lx\r\n";


   // THIS PROVIDES A CENTRAL LOCATION FROM WHICH I CAN
   // SET BREAKPOINTS OR WHATEVER (as needed)

   MyOutputDebugString(lpcMsg);
   MyOutputDebugString(szCRLF);

   // Provide a "swan song" for the dying application

   wEIP = *(((DWORD *)&(lpcMsg)) - 1);

   wsprintf(tbuf, szFmt, wEIP);
   MyOutputDebugString(tbuf);


   _asm nop
   _asm nop   // I can set breakpoints here very easily
   _asm nop

   FatalAppExit(iCode, lpcMsg);

}

#pragma auto_inline( on )





                  /*************************************/
                  /** STATIC VARIABLES FOR SEMAPHORES **/
                  /*************************************/

#define SEM_SIZE 32

static char semtable[MAX_SEMAPHORES][SEM_SIZE]={0};
static WORD semflags[MAX_SEMAPHORES] = {0};
static WORD semstatus[MAX_SEMAPHORES] = {0};

static SEMAPHORECALLBACK semproc[MAX_SEMAPHORECALLBACKS];
                                               /* proc-instance addresses! */
static HANDLE hsemproc_semaphore[MAX_SEMAPHORECALLBACKS];
                                        /* semaphores that cause callback! */
static WORD wsemproc_eventid[MAX_SEMAPHORECALLBACKS];
                                   /* 'id' values for semaphore callbacks! */


                    /*********************************/
                    /** SEMAPHORE UTILITY FUNCTIONS **/
                    /** (obsolete - compat only!)   **/
                    /*********************************/


#pragma code_seg("SEMAPHORE_TEXT","CODE")

BFPPROC MthreadCreateSemaphore(LPSTR lpSemName, WORD wFlags)
{
int i, first_null = -1;
char buf[18];


   if(bWin32sFlag)
   {
      return(lpMthreadCreateSemaphore(lpSemName, wFlags));
   }

   _hmemset((LPSTR)buf, 0, sizeof(buf));
   _fstrncpy((LPSTR)buf, lpSemName, 16);
   _fstrupr((LPSTR)buf);

   for(i=0; i<MAX_SEMAPHORES; i++)
   {
      if(semtable[i][0]==0)
      {
         if(first_null<0) first_null = i;
      }
      else if(_fstrncmp((LPSTR)buf, semtable[i], sizeof(semtable[i]))==0)
      {
         return(1); /* can't create an already existing semaphore! */
      }
   }

   if(first_null<0)      /* no more room for semaphores! */
   {
      return(2);         /* appropriate error code for this */
   }

                    /* assign the value to the table!! */
   _hmemcpy((LPSTR)semtable[first_null], (LPSTR)buf, sizeof(*semtable));

   semflags[first_null] = wFlags;
   semstatus[first_null] = 0;     /* bit 15 is set when semaphore set */
                                  /* bits 0-14 are the reference count! */

   return(FALSE);        /* o.k. - good result! */

}

BFPPROC MthreadKillSemaphore(LPSTR lpSemName)
{
int i, j, k;
char buf[18];
HTASK hTask=NULL;


   if(bWin32sFlag)
   {
      return(lpMthreadKillSemaphore(lpSemName));
   }

   _hmemset((LPSTR)buf, 0, sizeof(buf));
   _fstrncpy((LPSTR)buf, lpSemName, 16);
   _fstrupr((LPSTR)buf);

   for(i=0; i<MAX_SEMAPHORES; i++)
   {
      if(_fstrncmp((LPSTR)buf, semtable[i], sizeof(semtable[i]))==0)
      {
         if(semstatus[i] & 0x7fff)    /* there is a reference count! */
            return(1);      /* can't delete a 'referenced' semaphore! */

               /* delete semaphore and all references to it! */

         _hmemset(semtable[i], 0, sizeof(semtable[i]));

         semflags[i] = 0;
         semstatus[i] = 0;

         for(j=0, k=i+SEM_HANDLE_OFFSET; j<MAX_SEMAPHORECALLBACKS; j++)
         {
            if((int)hsemproc_semaphore[j]==k)     /* converted index to handle */
            {                       /* compare with callback table entries */

               semproc[j] = (SEMAPHORECALLBACK)NULL;
               hsemproc_semaphore[j] = (HANDLE)NULL;

               wsemproc_eventid[j] = (WORD)NULL;
            }
         }
         return(FALSE);
                    /* everything's o.k. now, so bail out with exit code 0 */
      }
   }

   return(2);      /* semaphore wasn't found, so return with error code #2 */
}



HANDLE FAR PASCAL MthreadOpenSemaphore(LPSTR lpSemName)
{
int i;
char buf[18];
HTASK hTask=NULL;


   /* returned handle is offset by SEM_HANDLE_OFFSET to ensure NULL=error */

   _hmemset((LPSTR)buf, 0, sizeof(buf));
   _fstrncpy((LPSTR)buf, lpSemName, 16);
   _fstrupr((LPSTR)buf);

   for(i=0; i<MAX_SEMAPHORES; i++)
   {
      if(_fstrncmp((LPSTR)buf, semtable[i], sizeof(semtable[i]))==0)
      {
         semstatus[i] = ((semstatus[i] & 0x7fff) + 1)
                        | (semstatus[i] & 0x8000);  /* increment ref count */

         return((HANDLE)(i + SEM_HANDLE_OFFSET));      /* the handle!! */
      }
   }

   return(NULL);  /* couldn't open the semaphore!! */
}


HANDLE FAR PASCAL MthreadCloseSemaphore(HANDLE hSemaphore)
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





BFPPROC EXPORT LOADDS MthreadSemaphoreCallback(HANDLE hSemaphore,
                                               SEMAPHORECALLBACK lpProc)
{
      /* this function must be called to dispatch to semaphore callback */
int i;
HTASK hTask=NULL;
HINSTANCE hInst=NULL;
BOOL Rval;
WORD wEventID, wSemValue;


   if(bWin32sFlag)
   {
      return(lpMthreadSemaphoreCallback(hSemaphore, lpProc));
   }

   for(i=0; i<MAX_SEMAPHORECALLBACKS; i++)
   {
      if(hsemproc_semaphore[i]==hSemaphore &&
         semproc[i]==lpProc)
      {

         wSemValue = (semstatus[i] & 0x8000)!=0;
         wEventID  = wsemproc_eventid[i];

         Rval = lpProc(hSemaphore, wEventID, wSemValue);

         return(Rval);
      }
   }

   return(TRUE);  /* error! */
}




BOOL FAR PASCAL InternalSemaphoreCallback(HANDLE hSemaphore, BOOL SetFlag)
{
int i;
BOOL was_callback=FALSE;



   /** since the semaphore callback functions are possibly in another **/
   /** task it will be necessary to spawn them using a means other    **/
   /** than just CALLING them directly.  The 'WM_SEMAPHORECALLBACK    **/
   /** message is for this purpose.                                   **/

   for(i=0; i<MAX_SEMAPHORECALLBACKS; i++)
   {
      if(hsemproc_semaphore[i]==hSemaphore &&
         semproc[i]!=(SEMAPHORECALLBACK)NULL)
      {
         was_callback=TRUE;

         // 32-bit version - only THIS task is involved

         MthreadSemaphoreCallback(hSemaphore, semproc[i]);
      }
   }

   return(was_callback);             /* this says 'I processed something' */
}




BFPPROC MthreadSetSemaphore(HANDLE hSemaphore)
{
int i;

   if(bWin32sFlag)
   {
      return(lpMthreadSetSemaphore(hSemaphore));
   }

    if(hSemaphore==(HANDLE)NULL)  return(-1);  /* null handle! */

    i = (WORD)hSemaphore - SEM_HANDLE_OFFSET;

    if(i<0 || i>=MAX_SEMAPHORES)    /* out of range!!! */
       return(1);                   /* bad handle error */

    if(semstatus[i] & 0x7fff)       /* there is a reference count! */
    {
       semstatus[i] |= 0x8000;      /* sets the semaphore! */

       InternalSemaphoreCallback(hSemaphore, TRUE);
                                    /* call the callback functions! */

       return(0);
    }
    else
       return(2);  /* handle is valid, but reference count is bad! */

}


BFPPROC MthreadClearSemaphore(HANDLE hSemaphore)
{
int i;

   if(bWin32sFlag)
   {
      return(lpMthreadClearSemaphore(hSemaphore));
   }

    if(hSemaphore==(HANDLE)NULL)  return(0xffff);  /* null handle! */

    i = (WORD)hSemaphore - SEM_HANDLE_OFFSET;

    if(i<0 || i>=MAX_SEMAPHORES)    /* out of range!!! */
       return(1);                   /* bad handle error */

    if(semstatus[i] & 0x7fff)      /* there is a reference count! */
    {
       semstatus[i] &= 0x7fff;      /* clears the semaphore! */

       InternalSemaphoreCallback(hSemaphore, TRUE);
                                    /* call the callback functions! */

       return(0);
    }
    else
       return(2);  /* handle is valid, but reference count is bad! */

}




BFPPROC MthreadWaitSemaphore(HANDLE hSemaphore, BOOL wState)
{

   if(bWin32sFlag)
   {
      return(lpMthreadWaitSemaphore(hSemaphore, wState));
   }

   if(MthreadGetSemaphore(hSemaphore)==0xffff)  /* this means 'bad semaphore'! */
   {
      return(1);
   }


   while((MthreadGetSemaphore(hSemaphore)!=0)!=(wState!=0))
   {
      Sleep(0);  // give up remainder of time slice (not efficient, but...)
   }

   return(0);        /* if everything's ok, just exit with a 0 return! */

}



BFPPROC MthreadGetSemaphore(HANDLE hSemaphore)
{
int i;

   if(bWin32sFlag)
   {
      return(lpMthreadGetSemaphore(hSemaphore));
   }

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


BFPPROC RegisterSemaphoreProc(HANDLE hSemaphore,
                              SEMAPHORECALLBACK lpProc, WORD wEventID)
{
int i;


   if(bWin32sFlag)
   {
      return(lpRegisterSemaphoreProc(hSemaphore, lpProc, wEventID));
   }

       /** You cannot have multiple semaphore/callback entries **/
       /** check if the semaphore/callback combo is already    **/
       /** registered, and return an error if so!!             **/

   for(i=0; i<MAX_SEMAPHORECALLBACKS; i++)
   {
      if(hsemproc_semaphore[i]==hSemaphore &&
         semproc[i]==lpProc)
      {
         return(TRUE);  /* the semaphore proc is already registered!! */
      }
   }


   for(i=0; i<MAX_SEMAPHORECALLBACKS; i++)
   {
      if(hsemproc_semaphore[i]==(HANDLE)NULL &&
         semproc[i]==(SEMAPHORECALLBACK)NULL)
      {
         semproc[i] = lpProc;
         hsemproc_semaphore[i] = hSemaphore;

         wsemproc_eventid[i] = wEventID;

         return(FALSE);
      }
   }

   return(TRUE);

}


    /* when removing semaphores - a NULL in any arg assumes ALL, except */
    /* for 'wEventID' - a '0xffff' is the wildcard for this one!        */


BFPPROC RemoveSemaphoreProc(HANDLE hSemaphore,
                            SEMAPHORECALLBACK lpProc, WORD wEventID)
{
int i;
BOOL did_something = FALSE;


   if(bWin32sFlag)
   {
      return(lpRemoveSemaphoreProc(hSemaphore, lpProc, wEventID));
   }

   for(i=0; i<MAX_SEMAPHORECALLBACKS; i++)
   {
      if((hsemproc_semaphore[i]==hSemaphore || hSemaphore==(HANDLE)NULL) &&
         (wsemproc_eventid[i]==wEventID || wEventID==0xffff) &&
         (semproc[i]==lpProc || lpProc==(SEMAPHORECALLBACK)NULL))
      {
         semproc[i] = (SEMAPHORECALLBACK)NULL;
         hsemproc_semaphore[i] = (HANDLE)NULL;

         wsemproc_eventid[i] = (WORD)NULL;

         did_something = TRUE;
      }
   }


   return(!did_something);   /* TRUE if none found; FALSE otherwise */

}

#pragma code_seg()




/***************************************************************************/
/*                                                                         */
/*           THREAD INSTANCE DATA - Add, Remove, Get, Set, Etc.            */
/*                  (Similar to PROPERTIES for WINDOWS)                    */
/*                                                                         */
/***************************************************************************/

#pragma code_seg("MEMORY_UTIL_TEXT","CODE")

#ifndef ELEMENT_OFFSET_X
#define ELEMENT_OFFSET_X(X,Y) ((UINT)((DWORD)(&(((X FAR *)0)->Y))))
#endif  /* ELEMENT_OFFSET_X */

#ifndef ELEMENT_SIZE_X
#define ELEMENT_SIZE_X(X,Y) (sizeof(((X FAR *)0)->Y))
#endif  /* ELEMENT_SIZE_X */

#ifndef ELEMENT_SIZE
#define ELEMENT_SIZE(X,Y) (sizeof( ((struct X *)0)->Y ))
#endif  /* ELEMENT_SIZE */

#ifndef ELEMENT_OFFSET
#define ELEMENT_OFFSET(X,Y) ((UINT)((DWORD)(&( ((struct X *)0)->Y ))))
#endif  /* ELEMENT_OFFSET */


             // Function prototypes for *THIS* section

BOOL NEAR PASCAL InternalAddThreadInstanceData(char HUGE * FAR *lphpInstanceData,
                          LPCSTR szDataName, LPCSTR lpcData, DWORD cbBytes);

BOOL NEAR PASCAL InternalRemoveThreadInstanceData(char HUGE * FAR *lphpInstanceData,
                                                  LPCSTR szDataName);

LPCSTR NEAR PASCAL FindThreadInstanceData(void HUGE * FAR *lphpInstanceData,
                                          LPCSTR szDataName);


typedef struct {
   DWORD dwNextEntry;
   char szDataName[40];
   DWORD cbBlockSize;
   } TIDHEADER;

typedef TIDHEADER HUGE *LPTIDHEADER;


// BUFFER FORMAT:  consists of serially linked list of DATA BLOCKS, with
//                 each entry in the following format:
//
// Bytes 0 - 3     DWORD OFFSET (in buffer) of NEXT ENTRY or '0' for LAST
// Bytes 4 - 43    NULL-BYTE terminated label (non-case sensitive).
// Bytes 44 - 47   DWORD indicating size of DATA BLOCK that follows!
// Bytes 48 - ???  Data block (tries to END on an EVEN PARAGRAPH!)


   // Adds thread data to 'hThread' (NULL for CURRENT THREAD).  Returns
   // TRUE on error, FALSE on success.  If 'szDataName' is already in
   // use, this function will return an ERROR.


static void HUGE * FAR * NEAR __fastcall InternalGetTIDPtr(HANDLE hThread)
{
LPTHREADPARM32 lpTP = (LPTHREADPARM32)GetThreadParmPtr(hThread);

   if(lpTP)  return((void HUGE * FAR *)&(lpTP->lpInstanceData));
   else      return(NULL);   // no PARM pointer exists!! (whoops)

}


BFPPROC AddThreadInstanceData(HANDLE hThread, LPCSTR szDataName,
                              LPCSTR lpcData, DWORD cbBytes)
{
void HUGE * FAR *lphpInstanceData=NULL;


   if(bWin32sFlag) return(lpAddThreadInstanceData(hThread, szDataName,
                                                  lpcData, cbBytes));


   if(!(lphpInstanceData = InternalGetTIDPtr(hThread))) return(TRUE);


   if(FindThreadInstanceData(lphpInstanceData, szDataName))
   {
      return(TRUE);                        // error! (name already exists!)
   }

   return(InternalAddThreadInstanceData((char HUGE * FAR *)lphpInstanceData,
                                        szDataName, lpcData, cbBytes));

}



   // Gets thread data, copying it into 'lpDataBuf'.  Returns # of bytes
   // copied, or 0 on error.  If 'lpDataBuf' is NULL, it returns the
   // actual size of the data stored under 'szDataName'.

DWFPPROC GetThreadInstanceData(HANDLE hThread, LPCSTR szDataName,
                               LPSTR lpDataBuf, DWORD cbBufSize)
{
LPCSTR lpcData=NULL;
DWORD cbDataSize;
void HUGE * FAR *lphpInstanceData=NULL;


   if(bWin32sFlag) return(lpGetThreadInstanceData(hThread, szDataName,
                                                  lpDataBuf, cbBufSize));


   if(!(lphpInstanceData = InternalGetTIDPtr(hThread))) return(0);


   if(!(lpcData = FindThreadInstanceData(lphpInstanceData, szDataName)))
   {
      return(0);                       // data wasn't found, so return a '0'
   }

   _hmemcpy((LPSTR)&cbDataSize, lpcData, sizeof(DWORD));

   cbDataSize = min(cbBufSize, cbDataSize);

   if(lpDataBuf)
   {
      _hmemcpy(lpDataBuf, lpcData + sizeof(DWORD), cbDataSize);
   }


   return(lpDataBuf?cbDataSize:1);

}




   // Returns pointer to data.  First 4 bytes is a DWORD indicating the
   // SIZE of the element.  DO NOT MODIFY THIS!  Remaining 'n' bytes are
   // the data items themselves.  NOTE:  data *may* cross segment boundary!
   // ALL THREAD DATA ITEMS ARE CONTIGUOUS!  Adding, setting, or removing
   // any thread data item MAY cause this pointer to become INVALID!

LPCSTR FPPROC GetThreadInstanceDataPtr(HANDLE hThread, LPCSTR szDataName)
{
void HUGE * FAR *lphpInstanceData=NULL;


   if(bWin32sFlag) return(lpGetThreadInstanceDataPtr(hThread, szDataName));


   if(!(lphpInstanceData = InternalGetTIDPtr(hThread))) return(NULL);


   return(FindThreadInstanceData(lphpInstanceData, szDataName));

}



   // Assigns new data to existing 'szDataName' reference.  If it does not
   // exist already, the data item is created.  Otherwise, it is identical
   // to 'AddThreadInstanceData()'.

BFPPROC SetThreadInstanceData(HANDLE hThread, LPCSTR szDataName,
                              LPCSTR lpcDataBuf, DWORD cbBytes)
{
LPCSTR lpcData=NULL;
void HUGE * FAR *lphpInstanceData=NULL;


   if(bWin32sFlag) return(lpSetThreadInstanceData(hThread, szDataName,
                                                  lpcDataBuf, cbBytes));


   if(!(lphpInstanceData = InternalGetTIDPtr(hThread))) return(TRUE);


   if(lpcData = FindThreadInstanceData(lphpInstanceData, szDataName))
   {
      InternalRemoveThreadInstanceData((char HUGE * FAR *)lphpInstanceData,
                                       lpcData);
   }

   return(InternalAddThreadInstanceData((char HUGE * FAR *)lphpInstanceData,
                                        szDataName, lpcDataBuf, cbBytes));
}



   // Removes thread data, optionally copying it into 'lpDataBuf'.
   // Returns # of bytes copied, or 0 on error.  If an error occurs,
   // the data item is *STILL* removed!  If 'lpDataBuf' is NULL,
   // it returns a '1' for success, or a '0' on error.

DWFPPROC RemoveThreadInstanceData(HANDLE hThread, LPCSTR szDataName,
                                  LPSTR lpDataBuf, DWORD cbBufSize)
{
LPCSTR lpcData=NULL;
DWORD cbDataSize;
void HUGE * FAR *lphpInstanceData=NULL;


   if(bWin32sFlag) return(lpRemoveThreadInstanceData(hThread,szDataName,
                                                     lpDataBuf, cbBufSize));


   if(!(lphpInstanceData = InternalGetTIDPtr(hThread))) return(0);


   if(!(lpcData = FindThreadInstanceData(lphpInstanceData, szDataName)))
   {
      return(0);                       // data wasn't found, so return a '0'
   }

   _hmemcpy((LPSTR)&cbDataSize, lpcData, sizeof(DWORD));

   cbDataSize = min(cbBufSize, cbDataSize);

   if(lpDataBuf)
   {
      _hmemcpy(lpDataBuf, lpcData + sizeof(DWORD), cbDataSize);
   }

   InternalRemoveThreadInstanceData((char HUGE * FAR *)lphpInstanceData,
                                    lpcData);
   return(lpDataBuf?cbDataSize:1);

}





BOOL NEAR PASCAL InternalAddThreadInstanceData(char HUGE * FAR *lphpInstanceData,
                          LPCSTR szDataName, LPCSTR lpcData, DWORD cbBytes)
{
char HUGE *lpTable=NULL;
LPTIDHEADER lpHeader=NULL;
TIDHEADER hdr;



   lpTable = (char HUGE *)*lphpInstanceData;

   if(lpTable)
   {
      lpHeader = (LPTIDHEADER)lpTable;

      do
      {
         _hmemcpy((char HUGE *)&hdr, (char HUGE *)lpHeader, sizeof(hdr));

         if(hdr.dwNextEntry)
         {
            lpHeader = (LPTIDHEADER)(lpTable + hdr.dwNextEntry);
         }

      } while(hdr.dwNextEntry);

      // ENSURE THE NEXT ENTRY STARTS ON AN 'EVEN PARAGRAPH'!

      hdr.dwNextEntry = (((char HUGE *)lpHeader) - lpTable
                        + sizeof(hdr) + hdr.cbBlockSize + 15) & 0xfffffff0L;

      if(GlobalSizePtr(lpTable) <=
         ((hdr.cbBlockSize + 2 * sizeof(hdr) + cbBytes + 31) & 0xfffffff0L))
      {
       char HUGE *hp1;

         hp1 = (char HUGE *)
                GlobalReAllocPtr(lpTable, ((hdr.cbBlockSize + 2 * sizeof(hdr)
                                          + cbBytes + 0x8000) & 0xffff8000L),
                                 GMEM_MOVEABLE);

         if(!hp1)
         {
            GlobalCompact(((hdr.cbBlockSize + 2 * sizeof(hdr)
                          + cbBytes + 0x18000L) & 0xffff8000L));

            hp1 = (char HUGE *)
                GlobalReAllocPtr(lpTable, ((hdr.cbBlockSize + 2 * sizeof(hdr)
                                          + cbBytes + 0x8000) & 0xffff8000L),
                                 GMEM_MOVEABLE);
         }

         if(!hp1) return(TRUE);  // error (not enough memory)

         *lphpInstanceData = hp1;

         lpHeader = (LPTIDHEADER)((((char HUGE *)lpHeader)-lpTable) + hp1);
         lpTable = hp1;
      }

      // update 'last item's header!

      _hmemcpy((char HUGE *)lpHeader, (char HUGE *)&hdr, sizeof(hdr));

      lpHeader = (LPTIDHEADER)(lpTable + hdr.dwNextEntry);
   }
   else
   {
      lpTable = (char HUGE *)
                GlobalAllocPtr(GMEM_MOVEABLE, (2 * sizeof(hdr) + cbBytes +
                                               0x8000) & 0xffff8000L);

      if(!lpTable)
      {
         GlobalCompact((2 * sizeof(hdr) + cbBytes + 0x18000) & 0xffff8000L);

         lpTable = (char HUGE *)
                 GlobalAllocPtr(GMEM_MOVEABLE, (2 * sizeof(hdr) + cbBytes +
                                                0x8000) & 0xffff8000L);
      }

      if(!lpTable)
      {
         return(TRUE);
      }


      *lphpInstanceData = lpTable;

      lpHeader = (LPTIDHEADER)lpTable;

   }

   _hmemset((char HUGE *)&hdr, 0, sizeof(hdr));

   hdr.dwNextEntry = 0;     // marks 'end of table'
   _hmemcpy(hdr.szDataName, szDataName,
            min(sizeof(hdr.szDataName) - 1, lstrlen(szDataName) + 1));

   hdr.cbBlockSize = cbBytes;

   _hmemcpy((char HUGE *)lpHeader, (char HUGE *)&hdr, sizeof(hdr));
   _hmemcpy(((char HUGE *)lpHeader) + sizeof(hdr), lpcData, cbBytes);

   return(FALSE);    // DONE!  COMPLETE!  SAT!  OK!  YAHOO!!

}


BOOL NEAR PASCAL InternalRemoveThreadInstanceData(char HUGE * FAR *lphpInstanceData,
                                                  LPCSTR lpcEntry)
{
LPTIDHEADER lpHeader=NULL, lpHdr2=NULL, lpHdr3=NULL;
TIDHEADER hdr1, hdr2;



   // NOTE:  If this is the *LAST* data item, free the memory buffer!

   if(((const char HUGE *)lpcEntry - *lphpInstanceData)
      < ELEMENT_OFFSET_X(TIDHEADER, cbBlockSize))
   {
      return(TRUE);  // this is an *INTERNAL* error, by the way!!!
   }

   lpHeader = (LPTIDHEADER)((char HUGE *)lpcEntry -
                            ELEMENT_OFFSET_X(TIDHEADER, cbBlockSize));



   // make a copy of *THIS* header into 'hdr1'

   _hmemcpy((char HUGE *)&hdr1, (char HUGE *)lpHeader, sizeof(hdr1));


   if(hdr1.dwNextEntry == 0)     // LAST ENTRY IN THE FILE!
   {

      if((char HUGE *)lpHeader == *lphpInstanceData)
      {
         GlobalFreePtr(lpHeader);

         *lphpInstanceData = NULL;
         return(FALSE);
      }


      // FIND THE ITEM THAT POINTS TO *THIS* ONE!!

      lpHdr2 = (LPTIDHEADER)*lphpInstanceData;

      do
      {
         _hmemcpy((char HUGE *)&hdr2, (char HUGE *)lpHdr2, sizeof(hdr2));

         if(!hdr2.dwNextEntry)    // this indicates a *BAD* error!! (recurse)
         {
            return(TRUE);
         }

         lpHdr3 = lpHdr2;

         lpHdr2 = (LPTIDHEADER)((char HUGE *)*lphpInstanceData
                                + hdr2.dwNextEntry);

      } while(lpHdr2 != lpHeader);


      // OK!  'lpHdr3' points to the entry I'm going to DELETE!  Now,
      // change it's 'dwNextEntry' value to ZERO and I'm done.

      _hmemset((LPSTR)&(lpHdr3->dwNextEntry), 0, sizeof(lpHdr3->dwNextEntry));

   }
   else
   {
    DWORD dwDelta;


      // starting with the *NEXT* item, move everything BACK to fill in
      // the hole where *THIS* item was!!

      lpHdr2 = lpHeader;
      lpHdr3 = (LPTIDHEADER)((char HUGE *)*lphpInstanceData
                             + hdr1.dwNextEntry);

      dwDelta = ((char HUGE *)lpHdr3) - ((char HUGE *)lpHdr2);

      do
      {
         _hmemcpy((char HUGE *)&hdr2, (char HUGE *)lpHdr3, sizeof(hdr2));

         _hmemcpy((char HUGE *)lpHdr2, (char HUGE *)lpHdr3,
                  sizeof(hdr2) + hdr2.cbBlockSize);

         if(hdr2.dwNextEntry)
         {
            hdr2.dwNextEntry -= dwDelta;

            _hmemcpy((char HUGE *)lpHdr2, (char HUGE *)&hdr2,
                     sizeof(hdr2));

            lpHdr3 = (LPTIDHEADER)((char HUGE *)*lphpInstanceData
                                   + hdr2.dwNextEntry);

            lpHdr3 = (LPTIDHEADER)((char HUGE *)*lphpInstanceData
                                   + hdr2.dwNextEntry + dwDelta);

         }

      } while(hdr2.dwNextEntry);

   }

   return(FALSE);

}


LPCSTR NEAR PASCAL FindThreadInstanceData(void HUGE * FAR *lphpInstanceData,
                                          LPCSTR szDataName)
{
const char HUGE *lpTable=NULL;
LPTIDHEADER lpHeader=NULL;
TIDHEADER hdr;
char tbuf[40];                          // MAXIMUM (legal) 'szDataName' size!


   if(!(lpTable = (char HUGE *)*lphpInstanceData)) return(NULL);


   lpHeader = (LPTIDHEADER)lpTable;

   _hmemcpy(tbuf, szDataName, min(lstrlen(szDataName) + 1, sizeof(tbuf) - 1));

   tbuf[sizeof(tbuf) - 1] = 0;

   do
   {
      // does the label match?

      _hmemcpy((char HUGE *)&hdr, (char HUGE *)lpHeader, sizeof(hdr));

      if(!_fstricmp(hdr.szDataName, tbuf))
      {
         return((LPCSTR)((char HUGE *)lpHeader
                         + ELEMENT_OFFSET_X(TIDHEADER,cbBlockSize)));
      }

      lpHeader = (LPTIDHEADER)(lpTable + hdr.dwNextEntry);

   } while(hdr.dwNextEntry);

   return(NULL);

}


#pragma code_seg()
