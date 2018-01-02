/***************************************************************************/
/*                                                                         */
/*       Windows Multi-Thread Manager Utility Library - MTH32.LIB          */
/*                                                                         */
/*                      TRADE SECRET - CONFIDENTIAL                        */
/*                                                                         */
/*             The compiled libraries and API header files are             */
/*          (c) Copyright 1990-1995 by Stewart~Frazier Tools, Inc          */
/*                                                                         */
/*                                                                         */
/***************************************************************************/


/***************************************************************************/
/*                                                                         */
/*                       SPECIAL MACRO DEFINITIONS                         */
/*                                                                         */
/***************************************************************************/

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */


                       /*** HUGE MEMORY SUPPORT ***/

    /* use the macro below to ensure you have a proper pointer to an */
    /* element 'Y' within a structure pointed to by HUGE pointer 'X' */
    /* and the possibility of segment overlap exists.                */
    /* The resulting pointer can be passed to '_hmemXXX' functions.  */

#define HUGE_ELEMENT_PTR(X,Y) (((BYTE *)(X))+\
                               (((DWORD)(&((X)->Y))-((DWORD)(X))) )


#ifndef DECLARE_HANDLE32
#define DECLARE_HANDLE32 DECLARE_HANDLE
#endif



/***************************************************************************/
/*                                                                         */
/*          MESSAGES SENT BY THREADS TO CALLER & ROOT INSTANCE             */
/*                                                                         */
/***************************************************************************/


// TO AVOID PROBLEMS THE THREAD MESSAGES HAVE CHANGED TO CONFORM WITH THE
// MICROSOFT STANDARDS - BEGINNING WITH 'WM_USER+0x201' vs 'WM_USER+1'

#define WM_USR2 (WM_USER + 0x200)

#define THREAD_ACKNOWLEDGE (WM_USR2+1) /* response from thread to 'caller' */
				       /* wParam = hTask (of sender)       */
				       /* lParam = N/A                     */

#define THREAD_REGISTER    (WM_USR2+2) /* message sent to root instance    */
				       /* wParam = hTask (of sender)       */
				       /* lParam = N/A                     */

#define THREAD_COMPLETE    (WM_USR2+3) /* message sent when done           */
				       /* wParam = hTask (of sender)       */
				       /* lParam = Function return value   */
				       /*          (error == -1)           */

#define THREAD_STATUS      (WM_USR2+4) /* message sent during process to   */
				       /* caller from thread (optional)    */
				       /* wParam = hTask (of thread)       */
				       /* lParam = status information      */
				       /*   note:  sent to APPLICATION!    */

#define NO_MASTER_THREAD   (WM_USR2+5) /* message sent to thread applica-  */
				       /* tions when master exits - this   */
				       /* prevents it from sending any     */
				       /* messages to the 'master' task.   */

#define WM_SEMAPHORECALLBACK (WM_USR2+6) /* message sent to thread appli-  */
					 /* cations when a 'Semaphore'     */
					 /* callback function needs to be  */
					 /* called for this task.  The     */
					 /* function is automatically pro- */
					 /* cessed internally, but the     */
					 /* semaphore's handle can be used */
					 /* by a user proc (wParam).       */

#define THREAD_IDLE        (WM_USR2+7)   /* 'idle' message used when menu  */
					 /* or modal dialog box is active. */
					 /* this is *only* sent to a task! */

/***************************************************************************/
/*                                                                         */
/*         SpawnThread() 'FLAG' parameter style definitions                */
/*                                                                         */
/***************************************************************************/

#define THREADSTYLE_DEFAULT      0

#define THREADSTYLE_WAIT         1   /* thread style - wait until completed! */
				     /* not fully supported at this time.    */
#define THREADSTYLE_GOON         0   /* thread style - concurrent processing */

#define THREADSTYLE_SUSPEND      4   /* create thread SUSPENDED (Win32 only) */

#define THREADSTYLE_RETURNBOOL   0   /* default - returns FALSE if thread    */
				     /* spawned successfully, -1 otherwise.  */

#define THREADSTYLE_ICON      0x10   /* automatically creates 'thread window'*/


#define THREADSTYLE_RETURNHANDLE 0x2000  /* returns handle of thread, or -1  */

#define THREADSTYLE_LPTHREADPROC 0x40000000 /* use 'LPTHREADPROC' for proc  */
                                /* this is the default for 'SpawnThreadX()' */

#define THREADSTYLE_WIN32CALL    0x80000000 /* use Win32 calling convention */
                                /* default if using 'MthreadCreateThread()' */


// 'SEMAPHORESTYLE' FLAGS AND SEMAPHORE CONSTANTS

#define SEMAPHORE_NAMELENGTH      16     /* maximum length of semaphore name*/

#define SEMAPHORESTYLE_DEFAULT    0x0000
#define SEMAPHORESTYLE_GLOBAL     0x0000 /* enables system global semaphore */
					 /* (default style) */

#define SEMAPHORESTYLE_LOCAL      0x0001 /* enables 'local only' semaphores */
					 /* which are unique to a task.     */




/***************************************************************************/
/*                         MULTI-THREAD OVERVIEW                           */
/***************************************************************************/
/*                                                                         */
/*  A MULTI-THREAD APPLICATION must perform the following processes:       */
/*                                                                         */
/*  1)  Call MthreadInit(), passing the parms passed to WinMain()          */
/*      (this is automatic if linking to the 'xLIBCxWM' libraries)         */
/*                                                                         */
/*  2)  Call either MthreadGetMessage(), or MthreadMsgDispatch() to create */
/*      a proper message dispatch loop.                                    */
/*                                                                         */
/*  3)  Call MthreadExit() prior to exiting program.                       */
/*      (this is automatic if linking to the 'xLIBCxWM' libraries)         */
/*                                                                         */
/*  NOTE:  32-bit multi-thread programs do not need to perform 1 & 3.      */
/*                                                                         */
/*                                                                         */
/*  A THREAD PROCEDURE must periodically call 'LoopDispatch()' to ensure   */
/*  that other THREAD PROCEDURES, the MESSAGE LOOP, and other applications */
/*  can 'time-share' through the message dispatcher.                       */
/*                                                                         */
/*  The prototype for a 'THREAD PROCEDURE' must be as follows:             */
/*                                                                         */
/* LONG FAR PASCAL MyThread(HANDLE hCaller, HANDLE hParms, UINT flags);*/
/*                                                                         */
/*  where:   'hCaller' is thread handle of caller ('MthreadGetCurrentThread()')*/
/*       and 'hParms'  is a 'HANDLE' value (user specified)                */
/*       and 'flags'   is the 'flags' parameter passed to 'SpawnThread()'  */
/*                                                                         */
/*  (on entry, DS will automatically contain the AUTOMATIC data segment).  */
/*                                                                         */
/*  The function returns a LONG which is automatically sent to the caller  */
/*  upon completion by 'ThreadComplete()' as a 'THREAD_COMPLETE' message.  */
/*  This value should be '-1' on error, to conform with standards.         */
/*                                                                         */
/*                         16-bit and Win32s versions                      */
/*                         --------------------------                      */
/*  The 'hParms' parameter will always remain valid until 'LoopDispatch()' */
/*  is called.  The first 'LoopDispatch()' call normally returns to the    */
/*  calling thread (following 'SpawnThread()' or 'SpawnThreadX()').  Any   */
/*  parameters passed to the thread proc may be copied to LOCAL (AUTOMATIC)*/
/*  variables (if necessary) before calling 'LoopDispatch()'; then, the    */
/*  associated data areas may be 'free'd, modified, or whatever by the     */
/*  calling thread, following the 'SpawnThread()' or 'SpawnThreadX()'.     */
/*                                                                         */
/*  IMPORTANT!  All THREAD PROCS must assume SS!=DS (by definition)! Each  */
/*  thread gets a separate stack area, which is normally has the same      */
/*  selector as the thread handle.                                         */
/*                                                                         */
/*  NOTE: For a 16-bit thread, WORD PTR SS:[20H] normally contains DGROUP, */
/*        unless SS==DGROUP already (or a stack area was specified).       */
/*        If 'xLIBCxWM' is linked, WORD PTR DGROUP:[20H] also contains the */
/*        selector for DGROUP, so that this 'trick' can be used to load DS */
/*        for any exported CALLBACK proc that tries to assume SS==DS.      */
/*        See the 'MYLOADDS' macro (below) for a simple way to do this.    */
/*                                                                         */
/*                                                                         */
/*                          32-bit (Win32) versions                        */
/*                          -----------------------                        */
/*  Since Win32 has pre-emptive multithreading, you must pass a handle or  */
/*  pointer value for 'hParms' that will remain valid for as long as the   */
/*  thread procedure needs it.  If synchronization is needed on the object */
/*  you can use semaphores or mutices to control access to it.             */
/*                                                                         */
/*  To ensure that a 32-bit multi-thread program works in BOTH the Win32s  */
/*  and Win32 environments, use MthreadCreateMutex() to create a mutex     */
/*  that is compatible for both environments, and MthreadWaitObject() to   */
/*  wait for the object to change.                                         */
/*                                                                         */
/*  NOTE: SFT32.DLL implements its semaphores differently than does Win32. */
/*                                                                         */
/***************************************************************************/


#ifndef API             /* Compatibility only... */
#define API WINAPI
#endif


   /** Special 'shorthand' definitions for function call/return types **/

#ifndef FCPROC
#define FCPROC FAR _cdecl
#endif

#ifndef FPPROC
#define FPPROC FAR PASCAL
#endif

#ifndef VFPPROC
#define VFPPROC void FPPROC
#endif

#ifndef BFPPROC
#define BFPPROC BOOL FPPROC
#endif

#ifndef IFPPROC
#define IFPPROC int FPPROC
#endif

#ifndef WFPPROC
#define WFPPROC WORD FPPROC
#endif

#ifndef UFPPROC
#define UFPPROC UINT FPPROC
#endif

#ifndef HFPPROC
#define HFPPROC HANDLE FPPROC
#endif

#ifndef HFFPPROC
#define HFFPPROC HFILE FPPROC
#endif

#ifndef LFPPROC
#define LFPPROC LONG FPPROC
#endif

#ifndef LPCFPPROC
#define LPCFPPROC LPCSTR FPPROC
#endif

#ifndef DWFPPROC
#define DWFPPROC DWORD FPPROC
#endif

#ifndef LPFPPROC
#define LPFPPROC LPSTR FPPROC
#endif

#ifndef FFPPROC
#define FFPPROC float FPPROC
#endif

#ifndef DBLFPPROC
#define DBLFPPROC double FPPROC
#endif

#ifndef WP2LP
#define WP2LP(X) (MAKELPARAM((WORD)X,0))
#endif

#ifndef LP2WP
#define LP2WP(X) ((WPARAM)(LOWORD((DWORD)X)))
#endif



		    /*** Other (data) type definitions ***/

//typedef struct _date {       /* used by data conversion utilities   */
//     int year;               /* equivalent structure to 'dosdate_t' */
//     char month, day;
//
//     } SFTDATE, FAR *LPSFTDATE;
//
//
//
//typedef struct _time {      /* system time - similar to 'dostime_t' */
//     unsigned char hour, minute, second, hsecond;
//
//     } SFTTIME, FAR *LPSFTTIME;



#define HUGE

typedef char HUGE *HPSTR;
typedef const char HUGE *HPCSTR;


#ifndef _USHORT_
#define _USHORT_ 1
typedef unsigned short USHORT;
#endif /* _USHORT_ */


typedef LONG FAR PASCAL THREADPROC(HANDLE, HANDLE, UINT);
typedef LONG (FAR PASCAL *LPTHREADPROC)(HANDLE, HANDLE, UINT);

                      /** SEMAPHORE CALLBACK FUNCTION **/

typedef BOOL (FAR PASCAL *SEMAPHORECALLBACK)(HANDLE hSemaphore,
					     WORD wEventID, BOOL wState);


/***************************************************************************/
/*                       *** FUNCTION PROTOTYPES ***                       */
/***************************************************************************/

                  /***************************************/
                  /*** MESSAGE DISPATCH LOOP FUNCTIONS ***/
                  /***************************************/

BFPPROC MthreadGetMessage(LPMSG msg);  // get message; yields to threads
                                       // while no messages in the queue.

BFPPROC MthreadTranslateMessage(LPMSG msg);// performs message translation
                     // returns FALSE if 'DispatchMessage()' should be called
                     // (to be used in conjunction with 'MthreadGetMessage()')

BFPPROC MthreadMsgDispatch(LPMSG msg);  // 1 MESSAGE LOOP iteration per call
                                        // returns 'FALSE' on WM_QUIT

BFPPROC LoopDispatch(void);           /* dispatch from within a thread loop */
                                           /* returns 'TRUE' on WM_QUIT */



BFPPROC BroadcastMessageToChildren(HWND hWnd, UINT msg,
                                   WPARAM wParam, LPARAM lParam);
					  /* sends msg to all child windows */

VFPPROC RegisterCompactProc(FARPROC lpproc);
VFPPROC RegisterAppMessageProc(void (FAR PASCAL *AppMessageProc)(LPMSG));

	/*** functions to register/remove accelerator tables and ***/
	/*** do 'TranslateMessage()' function from dispatch loop ***/

BFPPROC RegisterAccelerators(HANDLE hAccel, HWND hwnd);
BFPPROC RemoveAccelerators(HANDLE hAccel, HWND hwnd);

BFPPROC RegisterDialogWindow(HWND hDlg);     /* calls 'IsDialogMessage()' */
					     /* within message loop.      */
VFPPROC UnregisterDialogWindow(HWND hDlg);   /* call on 'WM_DESTROY'.     */
					     /* this is necessary!        */

VFPPROC EnableMDIAccelerators(HWND hMDIClient);
		   /* enables 'TranslateMDISysAccel()' for one MDI Client  */
VFPPROC DisableMDIAccelerators(HWND hMDIClient);
		     /* remove MDI Accelerator processing for given window */

VFPPROC EnableTranslateMessage(void);     /* default condition */
VFPPROC DisableTranslateMessage(void);
BFPPROC MyTranslate(LPMSG msg);

VFPPROC EnableQuitMessage(void);     /* enables 'WM_QUIT' messages */
VFPPROC DisableQuitMessage(void);   /* disables 'WM_QUIT' messages */

VFPPROC EnableCloseMessage(void);   /* enables 'WM_CLOSE' messages */
VFPPROC DisableCloseMessage(void); /* disables 'WM_CLOSE' messages */

VFPPROC MthreadDisableMessages(void);
VFPPROC MthreadEnableMessages(void);


	      /************************************************/
	      /* THREAD INITIALIZATION / PROCESSING FUNCTIONS */
	      /************************************************/

BFPPROC MthreadInit(HANDLE hInst, HANDLE hPrev, LPSTR lpCmd, UINT uiCmdShow);
	    /* returns 1 if this process is a 'thread' to be dispatched. */
	    /* Returns zero for normal application, and -1 on error.     */


LFPPROC DefMthreadMasterProc(HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
				    /* alternate 'DefWindowProc()' for      */
				    /* 'MASTER' thread windows.             */
				    /* automatic processing of 'WM_CLOSE'   */

	    /***************************************************/
	    /*** THE FOLLOWING PROCEDURE IS REQUIRED ON EXIT ***/
	    /***************************************************/

VFPPROC MthreadExit(HANDLE hTask);  /* destroys all windows for this task,  */
				    /* and frees internal resources.        */
				    /* Passing NULL for hTask ends current  */
				    /* task. REQUIRED on exit from msg loop */



	       /********************************************/
	       /*** MESSAGE BOX AND DIALOG BOX FUNCTIONS ***/
	       /********************************************/


BFPPROC MthreadMessageBox(HWND hParent, LPCSTR lpcMessage,
			  LPCSTR lpcHeading, UINT fuStyle);
	// Identical to MessageBox(), but optimized for multi-thread


BFPPROC MthreadDialogBox(HINSTANCE hInst, LPCSTR lpszDlgTemp,
			 HWND hwndOwner, DLGPROC dlgprc);
	// Like 'DialogBox()', but optimized for multi-thread


BFPPROC MthreadDialogBoxParam(HINSTANCE hInst, LPCSTR lpszDlgTemp,
			      HWND hwndOwner, DLGPROC dlgprc,
			      LPARAM lParamInit);
	// Like 'DialogBoxParam()', but optimized for multi-thread


BFPPROC MthreadDialogBoxIndirect(HINSTANCE hInst, HANDLE hDlgTemp,
				 HWND hwndOwner, DLGPROC dlgprc);
	// Like 'DialogBoxIndirect()', but optimized for multi-thread


BFPPROC MthreadDialogBoxIndirectParam(HINSTANCE hInst, HANDLE hDlgTemp,
				      HWND hwndOwner, DLGPROC dlgprc,
				      LPARAM lParamInit);
	// Like 'DialogBoxIndirectParam()', but optimized for multi-thread



       /**************************************************************/
       /*** INFORMATION FUNCTIONS - REQUIRE 'MthreadInit()' FIRST! ***/
       /**************************************************************/

HFPPROC GetRootApplication(void);   /* returns handle of root application   */
HFPPROC GetRootInstance(void);      /* returns instance handle of root app. */

HINSTANCE FPPROC GetCurrentInstance(void);
                                    /* returns instance handle of cur. task */


HFPPROC MthreadGetCurrentThread(void); /* returns handle to Internal Thread    */
				    /* descriptor block, which describes    */
				    /* the thread process.  Used in certain */
                                    /* calls like 'MthreadSuspendThread()' and     */
                                    /* 'MthreadKillThread()', which require this   */
				    /* handle to perform their duties.      */
WFPPROC GetNumberOfThreads(void);   /* get the total number of threads this */
				    /* task has spawned that are still run- */
				    /* ning.  Returns 0 if none are present */
				    /* The message loop is not considered   */
				    /* to be a thread by this function.     */

HWND FPPROC RegisterMasterThreadWindow(HWND hWnd);
				    /* called by 'Master' application using */
				    /* the 'master' window - all others are */
				    /* parented by it so that WM_CLOSE will */
				    /* automatically ask to terminate any   */
				    /* outstanding threads.  Also receives  */
				    /* messages from threads.               */

HWND FPPROC GetMasterThreadWindow(void);
				    /* gets window registered above         */

HWND FPPROC GetMthreadWindow(void); /* gets MTHREAD (iconic) window handle  */

HWND FPPROC GetInstanceWindow(HANDLE hInst);
				    /* gets handle of FIRST PARENT WINDOW   */
				    /* found that belongs to 'hInst'!!      */

HTASK FPPROC GetInstanceTask(HANDLE hInst);
				    /* gets TASK HANDLE associated with a   */
				    /* given instance.. Isn't that nice!    */
HINSTANCE FPPROC GetTaskInstance(HANDLE hTask);
				    /* gets INSTANCE HANDLE associated with */
				    /* a given task.. Isn't that nice, too! */

BFPPROC IsHandleValid(HGLOBAL hGlobal); /* checks if global handle is valid */




UFPPROC SpawnThread(LPTHREADPROC lpProcName, HANDLE hArgs, UINT flags);
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


UFPPROC SpawnThreadX(LPTHREADPROC lpProcName, HANDLE hArgs, UINT flags,
                     LPSTR lpThreadStack, UINT wStackSize);
	 /* like it's predecessor, SpawnThread(), except that it  */
	 /* allows the caller to specify a stack area and/or size */
	 /* (stack size is required).  Specifying a NULL for the  */
	 /* 'lpThreadStack' parameter causes the API to create a  */
	 /* stack space internally (preferred method). If the     */
	 /* caller requires a specific stack area this parameter  */
	 /* must contain a valid selector:offset.  'wStackSize'   */
	 /* specifies the size (in bytes) of the total stack area */
	 /* (not including pointer OFFSET if 'lpThreadStack' is   */
	 /*  not NULL).                                           */
	 /* If the caller supplies a stack, the caller must free  */
	 /* this area (as required) when the thread completes.    */
	 /* TYPICALLY a caller supplied stack will have the same  */
	 /* selector as DGROUP, which may be required (due to a   */
	 /* language compatibility issue, COMMDLG.DLL, etc.).     */


void WINAPI MthreadExitThread(UINT uiRetVal);  // CALL THIS to terminate a thread
                                               // immediately with return code.


HANDLE WINAPI MthreadCreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes,
                                  DWORD dwStackSize,
                                  LPTHREAD_START_ROUTINE lpStartAddress,
                                  LPVOID lpParameter,
                                  DWORD dwCreateThreadFlags,
                                  LPDWORD lpThreadId,
                                  UINT uiMthreadFlags );

   // this proc mimics the behavior of 'CreateThread()' including under
   // 'Win32s' (except for security attribute information).
   //
   // IMPORTANT:  Note extra parm 'dwMthreadFlags' which corresponds to the
   //             'flags' parameter in 'SpawnThread()' and 'SpawnThreadX()'



HWND FPPROC CreateThreadWindow(HANDLE hThread);
         /* this function creates an (ICONIC) thread window that */
         /* can be used to represent that SOMETHING is happening,*/
         /* and allow the user to SUSPEND/RESUME operation.      */



BFPPROC IsThread(void);    /* returns TRUE if currently running within a   */
			   /* thread, or application supports multi-thread */


BFPPROC IsThreadHandle(HANDLE hThread); /* if 'hThread' is a thread handle */
					/* returns TRUE, otherwise FALSE.  */


	     /************************************************/
	     /*** FUNCTIONS SPECIFIC TO 'INTERNAL' THREADS ***/
	     /************************************************/

void FPPROC MthreadSleep(UINT uiTime);/* sleep for estimated 'uiTime' msecs */
                                      /* WIN32 uses 'Sleep()' API. Others   */
                                      /* not exact.  Forces 'WaitMessage()' */
				      /* when all threads waiting.          */

BFPPROC MthreadKillThread(HANDLE hThreadID); /* terminates thread associated with  */
				      /* 'hThreadID'.  Does not yield!      */
				      /* If thread must exit completely use */
				      /* 'WaitForThread()' following call   */
				      /* to 'KillThread()'.                 */

BFPPROC MthreadSuspendThread(HANDLE hThreadID);  /* temporarily stops a thread   */
					  /* from timesharing.            */

BFPPROC MthreadResumeThread(HANDLE hThreadID);   /* causes 'suspended' thread to */
					  /* resume timesharing.          */


BFPPROC MthreadThreadCritical(HANDLE hThreadID, BOOL bCriticalFlag);
   // This function can prevent the 'LoopDispatch()' function from returning
   // a non-zero value.  'bCriticalFlag' is TRUE to prevent LoopDispatch
   // from returning non-zero; FALSE for normal operation.  A thread can
   // still 'timeshare' when this API function has been called, but cannot
   // be terminated.  If the application is about to terminate, the thread
   // is forced to complete before the application can end.


BFPPROC MthreadThreadExclusive(HANDLE hThreadID, BOOL bExclusiveFlag);
   // This function enables/disables the LoopDispatch() function from
   // allowing other threads/applications to run.  Useful when a thread
   // might be calling a procedure that uses 'LoopDispatch()' and the
   // need to complete this thread ASAP exists.



BFPPROC MthreadWaitForThread(HANDLE hThreadID);  /* This function yields until    */
					  /* 'hThreadID' has ended.        */
					  /* returns FALSE when 'hThreadID'*/
					  /* ends, or TRUE if 'QUIT' rcv'd */

/***************************************************************************/
/*                                                                         */
/*                  General Purpose (useful) Utilities                     */
/*                                                                         */
/***************************************************************************/

	  /*------------------------------------------------------*/
	  /* These functions automatically allocate destination   */
	  /* memory buffer and copy contents of source, unlocking */
	  /* both source and destination when complete.           */
	  /* Return (HANDLE)NULL on error, else handle of dest.   */
	  /*------------------------------------------------------*/

HFPPROC GlobalHandleCopy(HANDLE hOldData);
HFPPROC GlobalHandleCopy2(HANDLE hOldData, WORD wFlags);

LPFPPROC GlobalPtrCopy(LPSTR lpSrc);
LPFPPROC GlobalPtrCopy2(LPSTR lpSrc, WORD wFlags);


	/**  These next 2 functions create/destroy 'near' buffers  **/
	/** operation is similar (in principle) to malloc()/free() **/

BFPPROC MakeLocalBuffer(void NEAR * FAR *lpnpBuf, WORD size);
BFPPROC FreeLocalBuffer(void NEAR *npBuf);



/***************************************************************************/
/*                                                                         */
/*      SEMAPHORE PROCEDURES - Create, Kill, Open, Close, Etc. (GLOBAL)    */
/*                                                                         */
/***************************************************************************/

BFPPROC MthreadCreateSemaphore(LPSTR lpSemName, WORD wFlags);
BFPPROC MthreadKillSemaphore(LPSTR lpSemName);
HFPPROC MthreadOpenSemaphore(LPSTR lpSemName);
HFPPROC MthreadCloseSemaphore(HANDLE hSemaphore);
BFPPROC MthreadSetSemaphore(HANDLE hSemaphore);
BFPPROC MthreadClearSemaphore(HANDLE hSemaphore);
BFPPROC MthreadWaitSemaphore(HANDLE hSemaphore, BOOL wState);
BFPPROC MthreadGetSemaphore(HANDLE hSemaphore);
BFPPROC RegisterSemaphoreProc(HANDLE hSemaphore,
			      SEMAPHORECALLBACK lpProc, WORD wEventID);
BFPPROC RemoveSemaphoreProc(HANDLE hSemaphore,
			    SEMAPHORECALLBACK lpProc, WORD wEventID);




/***************************************************************************/
/*                                                                         */
/*           THREAD INSTANCE DATA - Add, Remove, Get, Set, Etc.            */
/*                  (Similar to PROPERTIES for WINDOWS)                    */
/*                                                                         */
/***************************************************************************/


BFPPROC AddThreadInstanceData(HANDLE hThread, LPCSTR szDataName,
			      LPCSTR lpcData, DWORD cbBytes);
   // Adds thread data to 'hThread' (NULL for CURRENT THREAD).  Returns
   // TRUE on error, FALSE on success.  If 'szDataName' is already in
   // use, this function will return an ERROR.

DWFPPROC GetThreadInstanceData(HANDLE hThread, LPCSTR szDataName,
			       LPSTR lpDataBuf, DWORD cbBufSize);
   // Gets thread data, copying it into 'lpDataBuf'.  Returns # of bytes
   // copied, or 0 on error.  If 'lpDataBuf' is NULL, it returns the
   // actual size of the data stored under 'szDataName'.

LPCSTR FPPROC GetThreadInstanceDataPtr(HANDLE hThread, LPCSTR szDataName);
   // Returns pointer to data.  First 4 bytes is a DWORD indicating the
   // SIZE of the element.  DO NOT MODIFY THIS!  Remaining 'n' bytes are
   // the data items themselves.  NOTE:  data *may* cross segment boundary!
   // ALL THREAD DATA ITEMS ARE CONTIGUOUS!  Adding, setting, or removing
   // any thread data item MAY cause this pointer to become INVALID!

BFPPROC SetThreadInstanceData(HANDLE hThread, LPCSTR szDataName,
			      LPCSTR lpcData, DWORD cbBytes);
   // Assigns new data to existing 'szDataName' reference.  If it does not
   // exist already, the data item is created.  Otherwise, it is identical
   // to 'AddThreadInstanceData()'.


DWFPPROC RemoveThreadInstanceData(HANDLE hThread, LPCSTR szDataName,
				  LPSTR lpDataBuf, DWORD cbBufSize);
   // Removes thread data, optionally copying it into 'lpDataBuf'.
   // Returns # of bytes copied, or 0 on error.  If an error occurs,
   // the data item is *STILL* removed!  If 'lpDataBuf' is NULL,
   // it returns a '1' for success, or a '0' on error.

#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif  /* __cplusplus */
