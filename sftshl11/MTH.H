/***************************************************************************/
/*                                                                         */
/*        Windows Multi-Thread Manager Utility Library - MTH.LIB           */
/*                                                                         */
/*                      TRADE SECRET - CONFIDENTIAL                        */
/*                                                                         */
/*             The compiled libraries and API header files are             */
/*          (c) Copyright 1990-1995 by Stewart~Frazier Tools, Inc          */
/*                                                                         */
/*                                                                         */
/***************************************************************************/


#ifdef WIN32 // 32-bit definitions!!!

#include "mth32.h"  // all 32-bit compiles *MUST* use THIS header file!!

#else  // WIN32

/***************************************************************************/
/*                                                                         */
/*                       SPECIAL MACRO DEFINITIONS                         */
/*                                                                         */
/***************************************************************************/


                       /*** HUGE MEMORY SUPPORT ***/

    /* use the macro below to ensure you have a proper pointer to an */
    /* element 'Y' within a structure pointed to by HUGE pointer 'X' */
    /* and the possibility of segment overlap exists.                */
    /* The resulting pointer can be passed to '_hmemXXX' functions.  */

#define HUGE_ELEMENT_PTR(X,Y) (((BYTE _huge *)(X))+\
                              ((WORD)(OFFSETOF(&((X)->Y))-OFFSETOF((X)))) )



/***************************************************************************/
/*                                                                         */
/*          MESSAGES SENT BY THREADS TO CALLER & ROOT INSTANCE             */
/*                                                                         */
/***************************************************************************/


#define THREAD_ACKNOWLEDGE (WM_USER+1) /* response from thread to 'caller' */
                                       /* wParam = hTask (of sender)       */
                                       /* lParam = N/A                     */

#define THREAD_REGISTER    (WM_USER+2) /* message sent to root instance    */
                                       /* wParam = hTask (of sender)       */
                                       /* lParam = N/A                     */

#define THREAD_COMPLETE    (WM_USER+3) /* message sent when done           */
                                       /* wParam = hTask (of sender)       */
                                       /* lParam = Function return value   */
                                       /*          (error == -1)           */

#define THREAD_STATUS      (WM_USER+4) /* message sent during process to   */
                                       /* caller from thread (optional)    */
                                       /* wParam = hTask (of thread)       */
                                       /* lParam = status information      */
                                       /*   note:  sent to APPLICATION!    */

#define NO_MASTER_THREAD   (WM_USER+5) /* message sent to thread applica-  */
                                       /* tions when master exits - this   */
                                       /* prevents it from sending any     */
                                       /* messages to the 'master' task.   */

#define WM_SEMAPHORECALLBACK (WM_USER+6) /* message sent to thread appli-  */
                                         /* cations when a 'Semaphore'     */
                                         /* callback function needs to be  */
                                         /* called for this task.  The     */
                                         /* function is automatically pro- */
                                         /* cessed internally, but the     */
                                         /* semaphore's handle can be used */
                                         /* by a user proc (wParam).       */

#define THREAD_IDLE        (WM_USER+7)   /* 'idle' message used when menu  */
                                         /* or modal dialog box is active. */
                                         /* this is *only* sent to a task! */

/***************************************************************************/
/*                                                                         */
/*         SpawnThread() 'FLAG' parameter style definitions                */
/*                                                                         */
/***************************************************************************/

#define THREADSTYLE_DEFAULT      0

#define THREADSTYLE_WAIT         1  /* thread style - wait until completed! */
                                    /* not fully supported at this time.    */
#define THREADSTYLE_GOON         0  /* thread style - concurrent processing */
#define THREADSTYLE_INTERNAL     0x8000  /* internal (same task,ds)[default]*/
#define THREADSTYLE_EXTERNAL     0x4000  /* external (different task,ds)    */
#define THREADSTYLE_DEFPROCTYPE  0   /* default specification of proc name  */
                                     /* ASCII string 'exported' name if seg */
                                     /* type is DATA.                       */
                                     /* FARPROC address if seg type is CODE */
                                     /* Also assumes INTERNAL.              */
#define THREADSTYLE_FARPROC      2   /* FARPROC address - assumes INTERNAL  */
#define THREADSTYLE_EXPORTNAME   4   /* ASCII string 'exported' name.       */
                                     /* "PROCNAME" or "MODULE.PROCNAME"     */
                                     /* Assumes EXTERNAL unless designated  */
                                     /* as INTERNAL.                        */
#define THREADSTYLE_RETURNBOOL   0   /* default - returns FALSE if thread   */
                                     /* spawned successfully, -1 otherwise. */
#define THREADSTYLE_RETURNHANDLE 0x2000  /* returns handle of thread, or -1 */
                                         /* on error (errors are always -1) */


#define SEMAPHORE_NAMELENGTH      16     /* maximum length of semaphore name*/

#define SEMAPHORESTYLE_DEFAULT    0x0000
#define SEMAPHORESTYLE_GLOBAL     0x0000 /* enables system global semaphore */
                                         /* (default style) */

#define SEMAPHORESTYLE_LOCAL      0x0001 /* enables 'local only' semaphores */
                                         /* which are unique to a task.     */




/***************************************************************************/
/*                                                                         */
/*  By definition a THREAD must perform at least the following processes:  */
/*                                                                         */
/*  1)  Call MthreadInit(), passing the parms passed to WinMain()          */
/*    a)  Additionally 'ThreadDispatch()' allows executing a 'named' proc. */
/*                                                                         */
/*  2)  Call LoopDispatch() (no parms) periodically during processing      */
/*                                                                         */
/*  3)  Call ThreadComplete() prior to exiting program (parm = return val) */
/*                                                                         */
/*  These 3 functions perform all of the necessary message dispatches and  */
/*  so forth, when required, as well as allowing other processes to run.   */
/*                                                                         */
/*  The prototype for a 'THREAD PROCEDURE' must be as follows:             */
/*                                                                         */
/*    LONG FAR PASCAL MyThread(HANDLE hCaller, HANDLE hParms, WORD flags); */
/*                                                                         */
/*  where:   'hCaller' is task handle of caller (from GetCurrentTask())    */
/*       and 'hParms'  is a GMEM_DDESHARE memory block handle to parms     */
/*       and 'flags'   is the 'flags' parameter passed to 'SpawnThread()'  */
/*                                                                         */
/*  The function returns a LONG which is automatically sent to the caller  */
/*  upon completion by 'ThreadComplete()' as a 'THREAD_COMPLETE' message.  */
/*  This value should be '-1' on error, to conform with standards.         */
/*  Data may also be exchanged using 'WM_DDE' messages, since a handle to  */
/*  the caller has been included as a passed parameter to the function.    */
/*  The thread function should make a copy of the data passed to it using  */
/*  'HandleCopy()' and then free the memory associated with the 'passed'   */
/*  handle, unless it uses DDE to establish a warm or hot link with the    */
/*  calling application.  It must not assume that this memory block will   */
/*  remain once 'LoopDispatch()' has been called!!!!!                      */
/*                                                                         */
/***************************************************************************/


            /***************************************************/
            /******** STRUCTURE DEFINITIONS & DATA TYPES *******/
            /***************************************************/


   /** Special 'shorthand' definitions for function call/return types **/

#ifndef FPPROC
#define FPPROC FAR PASCAL
#endif

#ifndef VFPPROC
#define VFPPROC void FAR PASCAL
#endif

#ifndef BFPPROC
#define BFPPROC BOOL FAR PASCAL
#endif

#ifndef IFPPROC
#define IFPPROC int FAR PASCAL
#endif

#ifndef WFPPROC
#define WFPPROC WORD FAR PASCAL
#endif

#ifndef HFPPROC
#define HFPPROC HANDLE FAR PASCAL
#endif

#ifndef LFPPROC
#define LFPPROC LONG FAR PASCAL
#endif

#ifndef DWFPPROC
#define DWFPPROC DWORD FAR PASCAL
#endif

#ifndef LPFPPROC
#define LPFPPROC LPSTR FAR PASCAL
#endif

#ifndef FFPPROC
#define FFPPROC float FAR PASCAL
#endif

#ifndef DBLFPPROC
#define DBLFPPROC double FAR PASCAL
#endif

#ifndef WP2LP
#define WP2LP(X) (MAKELPARAM((WORD)X,0))
#endif

#ifndef LP2WP
#define LP2WP(X) ((WPARAM)(LOWORD((DWORD)X)))
#endif

#ifndef _THREADPROC_
#define _THREADPROC_
typedef LONG FAR PASCAL THREADPROC(HANDLE, HANDLE, WORD);
typedef LONG (FAR PASCAL *LPTHREADPROC)(HANDLE, HANDLE, WORD);
#endif

//                    /*** Other (data) type definitions ***/
//
//typedef struct _date {       /* used by data conversion utilities   */
//     int year;               /* equivalent structure to 'dosdate_t' */
//     char month, day;
//     } SFTDATE;

//typedef SFTDATE FAR *LPSFTDATE;


//typedef struct _time {      /* system time - similar to 'dostime_t' */
//     unsigned char hour, minute, second, hsecond;
//     } SFTTIME;

//typedef SFTTIME FAR *LPSFTTIME;



                      /** SEMAPHORE CALLBACK FUNCTION **/

typedef BOOL (FAR PASCAL *SEMAPHORECALLBACK)(HANDLE hSemaphore,
                                             WORD wEventID, BOOL wState);




/***************************************************************************/
/*                       *** FUNCTION PROTOTYPES ***                       */
/***************************************************************************/

                  /***************************************/
                  /*** MESSAGE DISPATCH LOOP FUNCTIONS ***/
                  /***************************************/

extern BOOL (CALLBACK * PASCAL MthreadMsgDispatch)(LPMSG msg);
                                        /* dispatch from main dispatch loop */
                                              /* returns 'FALSE' on WM_QUIT */

extern BOOL (FAR PASCAL * PASCAL LoopDispatch)(void);   /* dispatch from within a thread loop */
                                               /* returns 'TRUE' on WM_QUIT */

extern BOOL (FAR PASCAL * PASCAL BroadcastMessageToChildren)(HANDLE hWnd, WORD msg,
                                           WORD wParam, DWORD lParam);
                                           /* sends msg to children */

extern void (FAR PASCAL * PASCAL RegisterCompactProc)(FARPROC lpproc);
extern void (FAR PASCAL * PASCAL RegisterAppMessageProc)(
                   void (FAR PASCAL *AppMessageProc)(LPMSG lpmsg));

        /*** functions to register/remove accelerator tables and ***/
        /*** do 'TranslateMessage()' function from dispatch loop ***/

extern BOOL (FAR PASCAL * PASCAL RegisterAccelerators)(HANDLE hAccel, HWND hwnd);
extern BOOL (FAR PASCAL * PASCAL RemoveAccelerators)(HANDLE hAccel, HWND hwnd);

extern void (FAR PASCAL * PASCAL EnableMDIAccelerators)(HWND hMDIClient);
                   /* enables 'TranslateMDISysAccel()' for one MDI Client  */
extern void (FAR PASCAL * PASCAL DisableMDIAccelerators)(HWND hMDIClient);
                     /* remove MDI Accelerator processing for given window */

extern void (FAR PASCAL * PASCAL EnableTranslateMessage)(void);     /* default condition */
extern void (FAR PASCAL * PASCAL DisableTranslateMessage)(void);

extern void (FAR PASCAL * PASCAL EnableQuitMessage)(void);     /* enables 'WM_QUIT' messages */
extern void (FAR PASCAL * PASCAL DisableQuitMessage)(void);   /* disables 'WM_QUIT' messages */

extern void (FAR PASCAL * PASCAL EnableCloseMessage)(void);   /* enables 'WM_CLOSE' messages */
extern void (FAR PASCAL * PASCAL DisableCloseMessage)(void); /* disables 'WM_CLOSE' messages */

extern void (FAR PASCAL * PASCAL MthreadDisableMessages)(void);
extern void (FAR PASCAL * PASCAL MthreadEnableMessages)(void);


              /************************************************/
              /* THREAD INITIALIZATION / PROCESSING FUNCTIONS */
              /************************************************/

extern BOOL (FPPROC *lpMthreadInit)(HANDLE hInst, HANDLE hPrev, LPSTR lpCmd,
                                    WORD nCmdShow);

BFPPROC MthreadInit(HINSTANCE hInstance, HANDLE hPrev, LPSTR lpCmd,
                    WORD nCmdShow);

            /* returns 1 if this process is a 'thread' to be dispatched. */
            /* Returns zero for normal application, and -1 on error.     */
            /* FOR LIBRARY:  Loads 'WMTHREAD' (if present) and justifies */
            /* all of the PROC ADDRESSES in this header file!            */


             /*************************************************/
             /*** THE FOLLOWING TWO PROCEDURES ARE REQUIRED ***/
             /*** THEY ARE AUTOMATICALLY CALLED IF THE MAIN ***/
             /*** TASK WINDOW CALLS DefMthreadMasterProc()  ***/
             /*************************************************/

extern BOOL (FPPROC * PASCAL MthreadDestroy)(HWND hWnd);  /* removes properties, optionally kills */
                                    /* existing background threads. This is */
                                    /* normally called by default via       */
                                    /* 'DefMthreadMasterProc()' (above) for */
                                    /* a WM_CLOSE or WM_DESTROY message.    */

VFPPROC MthreadExit(HANDLE hTask);

extern void (FPPROC *lpMthreadExit)(HANDLE hTask);
                                    /* destroys all windows for this task   */
                                    /* passing NULL for hTask gets current  */
                                    /* task handle before destroying.       */
                                    /* Called by 'DefMthreadMasterProc()'.  */
                                    /* Should also call after exiting the   */
                                    /* message loop to ensure safe exit!!   */


       /**************************************************************/
       /*** INFORMATION FUNCTIONS - REQUIRE 'MthreadInit()' FIRST! ***/
       /**************************************************************/

extern HINSTANCE (FAR PASCAL * PASCAL GetCurrentInstance)(void);   /* returns instance handle of cur. task */


extern HANDLE (FPPROC * PASCAL MthreadGetCurrentThread)(void);     /* returns handle to Internal Thread    */
                                    /* descriptor block, which describes    */
                                    /* the thread process.  Used in certain */
                                    /* calls like 'MthreadSuspendThread()' and     */
                                    /* 'MthreadKillThread()', which require this   */
                                    /* handle to perform their duties.      */
extern WORD (FPPROC * PASCAL GetNumberOfThreads)(void);   /* get the total number of threads this */
                                    /* task has spawned that are still run- */
                                    /* ning.  Returns 0 if none are present */
                                    /* The message loop is not considered   */
                                    /* to be a thread by this function.     */


extern HANDLE (FAR PASCAL * PASCAL GetInstanceTask)(HANDLE hInst);
                                    /* gets TASK HANDLE associated with a   */
                                    /* given instance.. Isn't that nice!    */
extern HANDLE (FAR PASCAL * PASCAL GetTaskInstance)(HANDLE hTask);
                                    /* gets INSTANCE HANDLE associated with */
                                    /* a given task.. Isn't that nice, too! */


extern WORD (FPPROC * PASCAL GetSegmentLimit)(WORD wSeg); /* gets 'segment limit' (not via handle)*/
                                    /* returns 'GlobalSize()' in real mode. */

extern BOOL (FPPROC * PASCAL IsHandleValid)(HGLOBAL hGlobal); /* checks if global handle is valid */

extern BOOL (FPPROC * PASCAL IsSelectorValid)(WORD wSeg); /* returns TRUE if selector is valid.   */

extern BOOL (FPPROC * PASCAL IsSelectorReadable)(WORD wSeg);
                                    /* returns TRUE if selector is readable */

extern BOOL (FPPROC * PASCAL IsSelectorWritable)(WORD wSeg);
                                    /* returns TRUE if selector is writable */

extern BOOL (FPPROC * PASCAL IsCodeSelector)(WORD wSeg); /* returns TRUE if selector is CODE */



              /********************************************/
              /*** THREAD EXECUTION AND ACKNOWLEDGEMENT ***/
              /********************************************/

extern BOOL (FPPROC * PASCAL ThreadAcknowledge)(void);    /* sends THREAD_ACKNOWLEDGE messages    */
extern BOOL (FPPROC * PASCAL ThreadComplete)(LONG rval);  /* sends THREAD_COMPLETE messages       */

extern WORD (FPPROC * PASCAL SpawnThread)(LPTHREADPROC lpProcName, HANDLE hArgs, WORD flags);
         /* this creates a new thread with function 'lpProcName' */
         /* 'lpProcName' is a THREADPROC function which returns  */
         /* -1 in event of an error.  This proc must call the    */
         /* 'LoopDispatch()' function periodically to yield to   */
         /* other threads and tasks.  After the 1st call to the  */
         /* 'LoopDispatch()' function 'SpawnThread()' returns to */
         /* the caller.  If the 'lpProcName' function has an     */
         /* error during initialization, and it returns without  */
         /* calling 'LoopDispatch()', the return value from the  */
         /* function will be returned by 'SpawnThread()'. Other- */
         /* wise, the return value of the thread procedure is    */
         /* passed via a THREAD_COMPLETE message, sent both to   */
         /* the application *AND* the Master Thread Window.      */
         /* The value returned by 'SpawnThread()' normally will  */
         /* depend on the switches in 'flags'.                   */


extern WORD (FPPROC * PASCAL SpawnThreadX)(LPSTR lpProcName, HANDLE hArgs, WORD flags,
                     LPSTR lpThreadStack, WORD wStackSize);
         /* like it's predecessor, SpawnThread(), except that it  */
         /* allows the caller to specify a stack area and/or size */
         /* (stack size is required).  Specifying a NULL for the  */
         /* 'lpThreadStack' parameter causes the API to create a  */
         /* stack space internally.  This is preferred.  If the   */
         /* caller requires a specific stack area this parameter  */
         /* must have an offset of '0'.  The 'wStackSize' parm    */
         /* specifies the size (in bytes) of the total stack area */


//extern long (FPPROC * PASCAL MthreadDispatch)(void);
//    /* call this function to dispatch to thread proc  */
//    /* returns value of -1 in the event of an error */


extern BOOL (FPPROC * PASCAL IsThread)(void);    /* returns TRUE if task is an 'EXTERNAL' thread */
                           /* (i.e. threadproc being dispatched) or if the */
                           /* current task supports multi-threading.  Call */
                           /* this function to determine if 'LoopDispatch' */
                           /* needs to be used during long operations.     */


extern BOOL (FPPROC * PASCAL IsThreadHandle)(HANDLE hThread); /* if 'hThread' is a thread handle */
                                        /* returns TRUE, otherwise FALSE.  */


             /*************************************************/
             /*** FUNCTIONS SPECIFIC TO 'INTERNAL' THREADS ***/
             /*************************************************/

extern BOOL (FPPROC * PASCAL MthreadKillThread)(HANDLE hThreadID); /* terminates thread associated with  */
                                      /* 'hThreadID'.  Does not yield!      */
                                      /* If thread must exit completely use */
                                      /* 'MthreadWaitForThread()' following call   */
                                      /* to 'MthreadKillThread()'.                 */

extern void (FPPROC * PASCAL MthreadSleep)(UINT uiTime);

extern BOOL (FPPROC * PASCAL MthreadSuspendThread)(HANDLE hThreadID);  /* temporarily stops a thread   */
                                          /* from timesharing.            */

extern BOOL (FPPROC * PASCAL MthreadResumeThread)(HANDLE hThreadID);   /* causes 'suspended' thread to */
                                          /* resume timesharing.          */

extern BOOL (FPPROC * PASCAL MthreadWaitForThread)(HANDLE hThreadID);  /* This function yields until    */
                                          /* 'hThreadID' has ended.        */
                                          /* returns FALSE when 'hThreadID'*/
                                          /* ends, or TRUE if 'QUIT' rcv'd */

/***************************************************************************/
/*                                                                         */
/*                  General Purpose (useful) Utilities                     */
/*                                                                         */
/***************************************************************************/


#if 0   /* for now, do NOT allow these proc addresses! */

                   /*** MEMORY ALLOCATION UTILITIES ***/

 /**************************************************************************/
 /* THESE FUNCTIONS ALLOW ALLOC/LOCK/UNLOCK/FREE of LARGE #'s of Handles   */
 /* in all cases assume the following:  ALL parameters are passed as longs */
 /* no matter what!  This includes sizes, pointers, pointers to pointers,  */
 /* and so forth!  This way there is never any problems!!                  */
 /* To ensure parameters are passed properly cast all pointers as (LPSTR)  */
 /* and all sizes as (LONG) or (DWORD).  This has given highly successful  */
 /* results under a wide variety of datatypes (including automatic var's). */
 /*                                                                        */
 /* if _multi{Global/Local}Alloc() is successful, the handle will be non-0 */
 /* otherwise, all handles in list are guaranteed to be zero!! This way,   */
 /* the _multi{*}Alloc() and _multi{*}Lock() can be 'cascaded', and the    */
 /* variables freed if either returns true AND the handles aren't zero     */
 /* additionally, the _multi{*}Free() functions check for non-zero handles */
 /* and automatically unlock any 'locked' handles before freeing.          */
 /*                                                                        */
 /* These functions can be incredible time-savers in development.  They    */
 /* enable the programmer to do his allocation and error checking in one   */
 /* or two lines instead of 10 or 20 lines, with graceful 'bailout' built  */
 /* in automatically!  Either all allocations/locks/wires/frees are suc-   */
 /* cessful, or none of them are (with a non-zero return value, and NULLS  */
 /* placed in the 'destination' variables).                                */
 /*                                                                        */
 /* Sample usage:                                                          */
 /*              .  .  .                                                   */
 /*                                                                        */
 /*    if(_multiGlobalAlloc(2,(LPSTR)&hVar1,(LONG)wVar1Size,               */
 /*                           (LPSTR)&hVar2,(LONG)wVar2Size) ||            */
 /*       _multiGlobalLock(2,(LPSTR)&hVar1,(LPSTR)&lpVar1,                 */
 /*                          (LPSTR)&hVar2,(LPSTR)&lpVar2))                */
 /*    {                                                                   */
 /*                                                                        */
 /*       _multiGlobalFree(2,(LPSTR)&hVar1,(LPSTR)&hVar2);                 */
 /*       return(TRUE);              error = TRUE                          */
 /*    }                                                                   */
 /*                                                                        */
 /*              .  .  .                                                   */
 /*                                                                        */
 /**************************************************************************/

extern BOOL (FAR _cdecl *_multiGlobalAlloc)(int nHandles,...);
      /* parms follow in pairs:  lpHandle1,dwSize1,lpHandle2,dwSize2, etc. */

extern BOOL (FAR _cdecl *_multiGlobalAllocDDE)(int nHandles,...);
      /* parms follow in pairs:  lpHandle1,dwSize1,lpHandle2,dwSize2, etc. */

extern BOOL (FAR _cdecl *_multiGlobalFree)(int nHandles,...);
                  /* one parm for each handle as lpHandle1,lpHandle2, etc. */

extern BOOL (FAR _cdecl *_multiGlobalLock)(int nHandles,...);
  /* parms follow in pairs:  lpHandle1,lplpData1,lpHandle2,lplpData2, etc. */

extern BOOL (FAR _cdecl *_multiGlobalUnlock)(int nHandles,...);
                  /* one parm for each handle as lpHandle1,lpHandle2, etc. */

extern BOOL (FAR _cdecl *_multiGlobalWire)(int nHandles,...);
  /* parms follow in pairs:  lpHandle1,lplpData1,lpHandle2,lplpData2, etc. */

extern BOOL (FAR _cdecl *_multiGlobalUnWire)(int nHandles,...);
                  /* one parm for each handle as lpHandle1,lpHandle2, etc. */

extern BOOL (FAR _cdecl *_multiLocalAlloc)(int nHandles,...);
      /* parms follow in pairs:  lpHandle1,dwSize1,lpHandle2,dwSize2, etc. */

extern BOOL (FAR _cdecl *_multiLocalFree)(int nHandles,...);
                  /* one parm for each handle as lpHandle1,lpHandle2, etc. */

extern BOOL (FAR _cdecl *_multiLocalLock)(int nHandles,...);
  /* parms follow in pairs:  lpHandle1,lpnpData1,lpHandle2,lpnpData2, etc. */

extern BOOL (FAR _cdecl *_multiLocalUnlock)(int nHandles,...);
                  /* one parm for each handle as lpHandle1,lpHandle2, etc. */


                    /*** GLOBAL 'POINTER' VERSIONS ***/

extern BOOL (FAR _cdecl *_multiGlobalAllocPtr)(int nPtr,...);
      /* parms follow in pairs:  lpPtr1,dwSize1,lpPtr2,dwSize2, etc. */

extern BOOL (FAR _cdecl *_multiGlobalAllocPtrDDE)(int nHandles,...);
      /* parms follow in pairs:  lpPtr1,dwSize1,lpPtr2,dwSize2, etc. */

extern BOOL (FAR _cdecl *_multiGlobalAllocPtrZ)(int nPtr,...);
      /* parms follow in pairs:  lpPtr1,dwSize1,lpPtr2,dwSize2, etc. */

extern BOOL (FAR _cdecl *_multiGlobalAllocPtrDDEZ)(int nHandles,...);
      /* parms follow in pairs:  lpPtr1,dwSize1,lpPtr2,dwSize2, etc. */

extern BOOL (FAR _cdecl *_multiGlobalFreePtr)(int nPtr,...);;
                     /* one parm for each ptr as lpPtr1,lpPtr2, etc. */



#endif /* 0 */



          /*------------------------------------------------------*/
          /* These functions automatically allocate destination   */
          /* memory buffer and copy contents of source, unlocking */
          /* both source and destination when complete.           */
          /* Return (HANDLE)NULL on error, else handle of dest.   */
          /*------------------------------------------------------*/

extern HANDLE (FAR PASCAL * PASCAL GlobalHandleCopy)(HANDLE hOldData);
extern HANDLE (FAR PASCAL * PASCAL GlobalHandleCopy2)(HANDLE hOldData, WORD wFlags);

extern LPSTR (FAR PASCAL * PASCAL GlobalPtrCopy)(LPSTR lpSrc);
extern LPSTR (FAR PASCAL * PASCAL GlobalPtrCopy2)(LPSTR lpSrc, WORD wFlags);

        /**  These next 2 functions create/destroy 'near' buffers  **/
        /** operation is similar (in principle) to malloc()/free() **/

extern BOOL (FAR PASCAL * PASCAL MakeLocalBuffer)(void NEAR * FAR *lpnpBuf, WORD size);
extern BOOL (FAR PASCAL * PASCAL FreeLocalBuffer)(void NEAR *npBuf);


/***************************************************************************/
/*                                                                         */
/*      SEMAPHORE PROCEDURES - Create, Kill, Open, Close, Etc. (GLOBAL)    */
/*                                                                         */
/***************************************************************************/

extern BOOL (FPPROC * PASCAL MthreadCreateSemaphore)(LPSTR lpSemName, WORD wFlags);
extern BOOL (FPPROC * PASCAL MthreadKillSemaphore)(LPSTR lpSemName);
extern HANDLE (FAR PASCAL * PASCAL MthreadOpenSemaphore)(LPSTR lpSemName);
extern HANDLE (FAR PASCAL * PASCAL MthreadCloseSemaphore)(HANDLE hSemaphore);
extern BOOL (FPPROC * PASCAL MthreadSetSemaphore)(HANDLE hSemaphore);
extern BOOL (FPPROC * PASCAL MthreadClearSemaphore)(HANDLE hSemaphore);
extern BOOL (FPPROC * PASCAL MthreadWaitSemaphore)(HANDLE hSemaphore, BOOL wState);
extern BOOL (FPPROC * PASCAL MthreadGetSemaphore)(HANDLE hSemaphore);

extern BOOL (FPPROC * PASCAL RegisterSemaphoreProc)(HANDLE hSemaphore,
                                                    SEMAPHORECALLBACK lpProc,
                                                    WORD wEventID);
extern BOOL (FPPROC * PASCAL RemoveSemaphoreProc)(HANDLE hSemaphore,
                                                  SEMAPHORECALLBACK lpProc,
                                                  WORD wEventID);

#endif // WIN32
