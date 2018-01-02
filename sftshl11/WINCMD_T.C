/***************************************************************************/
/*                                                                         */
/*  WINCMD_T.C - Command line interpreter for Microsoft(r) Windows (tm)    */
/*                                                                         */
/*                      TRADE SECRET - CONFIDENTIAL                        */
/*                                                                         */
/*             The compiled libraries and API header files are             */
/*          (c) Copyright 1990-1995 by Stewart~Frazier Tools, Inc          */
/*                                                                         */
/*        Main functions, message loop, background stuff, etc. etc.        */
/*                                                                         */
/***************************************************************************/


#define THE_WORKS
#include "mywin.h"

#include "mth.h"            /* stand-alone multi-thread functions */
#include "wincmd.h"         /* 'dependency' related definitions */
#include "wincmd_f.h"       /* function prototypes and variables */




#ifdef WIN32
#define LockCode()
#define UnlockCode()
#else  // WIN32
#define LockCode() { _asm push cs _asm call LockSegment }
#define UnlockCode() { _asm push cs _asm call UnlockSegment }
#endif // WIN32



                      /***************************/
                      /*** FUNCTION PROTOTYPES ***/
                      /***************************/


void FAR PASCAL CopyIteration(void);

LONG FAR PASCAL CommandThread(HANDLE hCaller, HANDLE hParms, UINT wFlags);
LONG FAR PASCAL CopyThread(HANDLE hCaller, HANDLE hParms, UINT wFlags);
LONG FAR PASCAL BatchThread(HANDLE hCaller, HANDLE hParms, UINT wFlags);
LONG FAR PASCAL WaitThread(HANDLE hCaller, HANDLE hParms, UINT wFlags);
LONG FAR PASCAL FlashThread(HANDLE hCaller, HANDLE hParms, UINT wFlags);
LONG FAR PASCAL FormatThread(HANDLE hCaller, HANDLE hParms, UINT wFlags);
LONG FAR PASCAL MemoryBatchFileThreadProc(HANDLE hCaller, HANDLE hParms, UINT wFlags);


void FAR PASCAL InvalidateStatusBar(HWND hWnd);

void FAR PASCAL InvalidateToolBar(HWND hWnd);

void FAR PASCAL UpdateStatusBar(void);

void FAR PASCAL UpdateWindowStateFlag(void);


                        /**********************/
                        /** STATIC VARIABLES **/
                        /**********************/

extern LPSTR lpCmdLine;

extern LPFORMAT_REQUEST lpFormatRequest;
extern BOOL format_complete_flag;

extern BOOL FormatSuspended;
extern BOOL CopySuspended;

extern BOOL FormatCancelFlag;   // when TRUE, current item is "canceled"
extern BOOL CopyCancelFlag;     // when TRUE, current item is "canceled"


extern BOOL CommandLineExit;  // when TRUE, forces SFTShell to exit
                              // following execution of the command


extern const char szWINDOW_STATE[];
extern const char szMAXIMIZED[];
extern const char szMINIMIZED[];
extern const char szNORMAL[];

extern const char szFORMATTING[];
extern const char szCOPYING[];
extern const char szTRUE[];
extern const char szFALSE[];

extern const char szRETVAL[];



/*-------------------------------------------------------------------------*/
/*                   BACKGROUND AND 'THREAD' PROCEDURES                    */
/*-------------------------------------------------------------------------*/


#define MAX_MSEC 170     /* 170 msec is 3 '18.2/sec TICKS', which helps */
                         /* ensure that there is no "short changing"    */
                         /* going on.  It's pretty consistent that way. */

#define MAX_MSEC2 275    /* 275 msec is 5 '18.2/sec TICKS', which helps */
                         /* ensure that there is no "short changing"    */
                         /* going on.  It's pretty consistent that way. */
                         /* NOTE:  This is used for BATCH FILES!!       */


#pragma code_seg("WINMAIN_TEXT","CODE")


LONG FAR PASCAL CommandThread(HANDLE hCaller, HANDLE hParms, UINT wFlags)
{
LPSTR lpTemp;


   hCommandThread = MthreadGetCurrentThread();

   while(!LoopDispatch())
   {
      UpdateWindowStateFlag();
      UpdateStatusBar();

      if(lpCmdLine != lpCurCmd && CommandLineExit)
      {
         // At this point, I have already executed the 'command line' args
         // and I'm about to wait for another command.  Force 'EXIT' into
         // the command buffer.

         ProcessCommand("EXIT");

         return(0);  // that's all, folks!
      }

      if(lpCurCmd && !waiting_flag)
      {                      /* there's a command waiting to execute! */

         lpTemp = GlobalAllocPtr(GMEM_MOVEABLE, lstrlen(lpCurCmd)+1);

         if(!lpTemp)
            lpTemp = lpCurCmd;  /* just in case */
         else
            lstrcpy(lpTemp, lpCurCmd);  /* make a COPY of it... */

         ProcessCommand(lpTemp);

         if(lpTemp!=lpCurCmd)  GlobalFreePtr(lpTemp);

         if(lpLastCmd && lpCurCmd!=lpCmdLine)
         {
           lpLastCmd = lpCurCmd;   /* only if not command line arg */
                                   /* and 'keeping history' is al- */
                                   /* ready in progress...         */
         }

         lpCurCmd = (LPSTR)NULL;      /* I'm done with this one, now! */

         if(hMainWnd && IsWindow(hMainWnd))
         {
            if(!BatchMode && !waiting_flag)
               DisplayPrompt();

            UpdateWindow(hMainWnd);   /* and the prompt is displayed! */
         }

         UpdateStatusBar();


           /** IF we are waiting, do something about it NOW!! **/
           /** this way I can capture remaining type-ahead in **/
           /** the command buffer as soon as it's done!       **/
           /**       (same goes for batch mode also!!)        **/

         while(waiting_flag && !LoopDispatch())
            ;         /* wait until not waiting or 'quit' */

         UpdateStatusBar();


         while(BatchMode && !LoopDispatch())
            ;         /* wait until not batch mode or 'quit' */

              /** If there is "Type Ahead", Fake typing in **/

         UpdateStatusBar();


         if(TypeAheadHead!=TypeAheadTail)
         {
            while(TypeAheadHead!=TypeAheadTail)
            {
               if(TypeAheadHead>=sizeof(TypeAhead)) TypeAheadHead=0;

               SendMessage(hMainWnd, WM_CHAR,
                           (WPARAM)((WORD)TypeAhead[TypeAheadHead]&0xff),
                           (LPARAM)1);

               if(TypeAhead[TypeAheadHead++]=='\r')
               {
                  if(TypeAheadHead>=sizeof(TypeAhead)) TypeAheadHead=0;
                  break;
               }

               if(TypeAheadHead>=sizeof(TypeAhead)) TypeAheadHead=0;
            }
         }

         continue;                /* I've done enough this iteration! */
      }
      else
      {
         if(MthreadSleep) MthreadSleep(50);    // wait an estimated 50 msec
      }
   }

   return(0);
}




LONG FAR PASCAL CopyThread(HANDLE hCaller, HANDLE hParms, UINT wFlags)
{
static WORD copy_pri=0;
DWORD dwTime;

   hCopyThread = MthreadGetCurrentThread();


   CopySuspended = FALSE;
   CopyCancelFlag = FALSE;


   while(!LoopDispatch())
   {
//      copy_pri++;

      if(copying /* && (copy_pri % COPY_PRI)==0 */ )
      {
         dwTime = GetTickCount();

         while(copying && (GetTickCount() - dwTime) < MAX_MSEC)
         {
            CopyIteration();       /* do another iteration!! */
         }

         copy_pri = 0;
         if(copying) wCopyPercentComplete = 100;
         else        wCopyPercentComplete = 0;
      }
      else
      {
         if(MthreadSleep) MthreadSleep(50);    // wait an estimated 50 msec
      }
   }

   return(0);
}


LONG FAR PASCAL FormatThread(HANDLE hCaller, HANDLE hParms, UINT wFlags)
{
LPVOID lpv;
#ifndef WIN32
LPSTR lp1, lp2;
#endif // WIN32
WORD w;
DWORD dwTemp;
static char msg[]="Place disk in drive A and press OK to commence FORMAT";
static char pBackgroundFormat[]="** BACKGROUND FORMAT **";
static char temp_label[16];


   hFormatThread = MthreadGetCurrentThread();

   FormatSuspended = FALSE;
   FormatCancelFlag = FALSE;


   while(!LoopDispatch())
   {
      while(lpFormatRequest)
      {
         formatting = TRUE;
         SetEnvString(szFORMATTING,szTRUE);
         UpdateStatusBar();

         msg[20] = (char)('A' + lpFormatRequest->wDrive);
                               /* modify above message for current drive */
         // DisableMessages();


         if(FormatCancelFlag)
         {
            w = IDCANCEL;
         }
         else
         {
            w = MessageBox(hMainWnd, msg, pBackgroundFormat,
                           MB_OKCANCEL | MB_ICONINFORMATION);
         }

         // EnableMessages();

         if(w==IDOK)
         {
            if(lpFormatRequest->wSwitchFlag & FORMAT_QUICKFORMAT)
            {
               if(QuickFormatDrive(lpFormatRequest->wDrive))
               {
                static const char CODE_BASED pMsg[]=
                 "?Cannot QUICK-FORMAT drive - perform UNCONDITIONAL FORMAT?";

                  LockCode();

                  w = MessageBox(hMainWnd, pMsg, pBackgroundFormat,
                                 MB_OKCANCEL | MB_ICONQUESTION);

                  UnlockCode();
               }
               else
               {
                  w = FALSE;   /* causes flow through to 'volume label' */
                               /* and 'system transfer' sections (below) */
               }
            }

            if(w==IDOK)
            {
               w = FormatDrive(lpFormatRequest->wDrive,
                               lpFormatRequest->wTracks,
                               lpFormatRequest->wHeads,
                               lpFormatRequest->wSectors);
            }
            else if(w!=FALSE)    /* don't do a thing if w==FALSE! */
            {            /* the only way to get that is valid quick format */

               w = TRUE;  /* ensures it does not change volume label */
                               /* (for non-valid QUICK FORMAT) */
            }
         }
         else
         {
            w = TRUE;  /* ensures it does not change volume label */
         }

                /*********************************************/
                /** THIS IS WHERE I ASSIGN THE VOLUME LABEL **/
                /*********************************************/

         if(!FormatCancelFlag && !w && lpFormatRequest->label[0]>' ')
         {
            // at this point it is VERY important to reset the disk drive...

            w = lpFormatRequest->wDrive + 1;

#ifdef WIN32
            {
              HANDLE hVWIN32 = OpenVWIN32();
              if(hVWIN32 != INVALID_HANDLE_VALUE)
              {
                FlushDiskSystem(hVWIN32, w);
                CloseHandle(hVWIN32);
              }
            }
#else  // WIN32
            if(IsChicago) _asm
            {
               mov ax, 0x710d
               mov cx, 1           // flush flag:  '1' means flush ALL including cache
               mov dx, w

               int 0x21            // flushes all drive buffers NOW!
            }
            else _asm
            {
               mov ax, 0xd00
               int 0x21
            }
#endif // WIN32

            temp_label[0] = lpFormatRequest->wDrive + 'A';
            temp_label[1] = ':';
            temp_label[2] = '\\';
            temp_label[3] = 0;

#ifdef WIN32
            if(!SetVolumeLabel(temp_label, lpFormatRequest->label))
            {
               MessageBox(hMainWnd, "?Could not create volume label", 
                          "* FORMAT ERROR *", MB_OK | MB_ICONHAND);
            }
#else // WIN32
            for(lp1=temp_label+3, lp2=lpFormatRequest->label, w=0; w<11; w++)
            {
               if(*lp2>' ')
               {
                  *lp1++ = *lp2++;
               }
               else
               {
                  break;
               }
            }

            *lp1 = 0;

            if(lstrlen(temp_label)>8)
            {
               for(lp1=temp_label+15, w=0; w<4; w++, lp1--)
               {
                  *lp1 = *(lp1 - 1);
               }
               temp_label[11] = '.';
            }

            _asm
            {
               mov ax, 0x5b00         /* create a NEW file */
               mov cx, _A_VOLID
               mov dx, OFFSET temp_label  /* it's a STATIC var */

               call DOS3Call

               mov w, ax
               jnc label_created

               mov ax, 0xffff
               mov w, ax
label_created:
            }

            if(w==0xffff)
            {
             static const char CODE_BASED pMsg[]=
                                         "?Could not create volume label";
             static const char CODE_BASED pMsg2[]=
                                                       "* FORMAT ERROR *";
               LockCode();

               MessageBox(hMainWnd, pMsg, pMsg2, MB_OK | MB_ICONHAND);

               UnlockCode();
            }
            else if(w>=5)
            {
               _asm
               {
                  mov ax, 0x3e00      /* close file */
                  mov bx, w
                  call DOS3Call

               }
            }
#endif // WIN32
         }


         /** make sure I remember to reset the disk system! **/

         w = lpFormatRequest->wDrive + 1;

#ifdef WIN32
         {
           HANDLE hVWIN32 = OpenVWIN32();
           if(hVWIN32 != INVALID_HANDLE_VALUE)
           {
             FlushDiskSystem(hVWIN32, w);
             CloseHandle(hVWIN32);
           }
         }
#else  // WIN32
         if(IsChicago) _asm
         {
            mov ax, 0x710d
            mov cx, 1           // flush flag:  '1' means flush ALL including cache
            mov dx, w

            int 0x21            // flushes all drive buffers NOW!
         }
         else _asm
         {
            mov ax, 0x0d00
            int 0x21       /* reset disk system by flushing all buffers! */
         }
#endif // WIN32

         /** NOW - check if the system should be transferred as well **/

         if(!FormatCancelFlag && !w &&
            (lpFormatRequest->wSwitchFlag & FORMAT_SYSTEMFILES))
         {
            if(lpFormatRequest->wSwitchFlag & FORMAT_BOOTDRIVEMASK)
            {              /* use the specified BOOT drive */

               temp_label[0] = 'A' - 1 +
                       (lpFormatRequest->wSwitchFlag & FORMAT_BOOTDRIVEMASK);
               temp_label[1] = ':';
               temp_label[2] = ' ';

               temp_label[3] = lpFormatRequest->wDrive + 'A';
               temp_label[4] = ':';
               temp_label[5] = 0;

            }
            else           /* use 'default' (unspecified) BOOT drive */
            {

               temp_label[0] = lpFormatRequest->wDrive + 'A';
               temp_label[1] = ':';
               temp_label[2] = 0;

            }


            dwTemp = TotalCopiesSubmitted;
            BlockCopyResults ++;

            CMDSys(NULL, (LPSTR)temp_label);

            TotalCopiesSubmitted = dwTemp;
            if(BlockCopyResults) BlockCopyResults --;
         }



         lpv = lpFormatRequest;  /* for safe-keeping */
         lpFormatRequest = lpFormatRequest->lpNext;

         GlobalFreePtr(lpv);

         if(!lpFormatRequest)
         {
            format_complete_flag = TRUE;
            SetEnvString(szFORMATTING,szFALSE);
            formatting = FALSE;
         }

         UpdateStatusBar();
         FormatCancelFlag = FALSE; // update HERE to cancel a cancel request
      }


      if(MthreadSleep) MthreadSleep(50);    // wait an estimated 50 msec
   }

   return(0);

}


LPSTR lpBatchLineBuf=NULL;  // this is the GLOBAL batch 'line buffer'
                            // held here for convenience.

static BOOL terminate_batch=FALSE;  // this is assigned TRUE if I need
                                    // to terminate batch files but I am
                                    // not in the normal batch thread...

static BOOL NoBatchEchoFlag = 0;    // increment once for each entry into
                                    // a user-defined function to prevent
                                    // an extraneous echo.  Flag decrements
                                    // when user-defined function returns.

static BOOL NoPromptFlag = 0;       // increment once for each entry into
                                    // a memory batch file to prevent
                                    // an extraneous 'leading prompt'.

LONG FAR PASCAL BatchThread(HANDLE hCaller, HANDLE hParms, UINT wFlags)
{
DWORD dwTime;


   hBatchThread = MthreadGetCurrentThread();

   if(!lpBatchLineBuf)
   {
      lpBatchLineBuf = GlobalAllocPtr(GMEM_MOVEABLE, BATCH_BUFSIZE);
   }

   while(!LoopDispatch())
   {
      // if not batch mode, or >.275 sec has elapsed, do a LoopDispatch()

      dwTime = GetTickCount();

      if(lpBatchInfo)
      {
         while(lpBatchInfo && !waiting_flag && BatchMode
               && (GetTickCount() - dwTime) <= MAX_MSEC2)  // too much time?
         {
            BatchIteration();
         }

         if(!lpBatchInfo) terminate_batch = FALSE;  // safety valve
      }
      else
      {
         terminate_batch = FALSE;  // safety valve

         if(MthreadSleep) MthreadSleep(50);    // wait an estimated 50 msec
      }
   }


   if(lpBatchLineBuf) GlobalFreePtr(lpBatchLineBuf);

   lpBatchLineBuf = NULL;

   return(0);

}

                 /** THIS PROCEDURE IS FULLY RE-ENTRANT **/

void FAR PASCAL BatchIteration(void)
{
LPSTR lpTemp;


   // just in case, check if I should do an iteration and do a
   // 'LoopDispatch()' call if not.  This is a 'safety valve'

   if(!lpBatchInfo || waiting_flag || !BatchMode)
   {
      if(!lpBatchInfo || !BatchMode)
      {
         terminate_batch = FALSE;  // safety valve

         NoPromptFlag = 0;         // just 'cause
         NoBatchEchoFlag = 0;      // same here...
      }

      LoopDispatch();
      return;
   }


   UpdateWindowStateFlag();
   UpdateStatusBar();


   if(ctrl_break || terminate_batch)
   {
    static const char CODE_BASED pMsg[]="Terminate Batch Process?";
    static const char CODE_BASED pMsg2[]="** CONTROL-BREAK **";


      ctrl_break = FALSE;  // initially set to false, though it may be reset
                           // to TRUE again if I terminate the batch file...

      if(!terminate_batch)
      {
         LockCode();

         terminate_batch = MessageBox(hMainWnd, pMsg, pMsg2,
                                     MB_YESNO | MB_ICONHAND | MB_SYSTEMMODAL)
                           ==IDYES;
         UnlockCode();
      }


      if(terminate_batch)
      {
         if(MthreadGetCurrentThread() == hBatchThread)
         {
            while(lpBatchInfo)
            {
               TerminateCurrentBatchFile();
            }

            terminate_batch = FALSE;  // cancel - I'm now terminated
         }
         else
         {
            MthreadKillThread(MthreadGetCurrentThread());  // terminate thread to exit
         }

         ctrl_break = FALSE;       // ensure it's FALSE afterwards
         return;
      }

      if(!lpBatchInfo) terminate_batch = FALSE;  // safety valve
   }

   if(lpTemp = GetAnotherBatchLine(lpBatchInfo))
   {

      if(*lpTemp!='@' && BatchEcho && !NoBatchEchoFlag &&
         hMainWnd && IsWindow(hMainWnd))  /* echoing commands */
      {
         if(NoPromptFlag) NoPromptFlag--;
         else
         {
            DisplayPrompt();
         }

         PrintString(lpTemp);  // DO echo memory batch files (as needed)
         PrintString(pCRLF);

      }

      if(*lpTemp=='@')  lpTemp++;

      if(*lpTemp!=':')   /* it's NOT a label! */
      {

         ProcessCommand(lpTemp); /* re-entrancy may take place here! */

      }
      else
      {
         AddBatchLabel(lpBatchInfo, lpTemp);
      }

      GlobalFreePtr(lpTemp);

      UpdateWindowStateFlag();
      UpdateStatusBar();
   }
   else
   {
      if(MthreadGetCurrentThread() != hBatchThread)
      {
         // this is an error, really... but for now, just terminate thread

         SetEnvString(szRETVAL,"");  // blank value for 'RETVAL'

         MthreadKillThread(MthreadGetCurrentThread());
      }
      else
      {
         TerminateCurrentBatchFile();
      }
   }

}



LONG FAR PASCAL CalcUserFunctionThread(HANDLE hCaller, HANDLE hParms, UINT wFlags)
{
BOOL MyBatchEcho = BatchEcho;   // what it was at the start...
DWORD dw1, dwTickCount;


   // execute batch file until I get a RETURN (the thread will end there)

   if(MyBatchEcho)
   {
      NoBatchEchoFlag++;  // eliminate the echo in here
   }

   dw1 = lpBatchInfo->lpBlockInfo->dwEntryCount;
   if(!dw1) return(1);    // this is an ERROR condition (improper call)


   // user-defined functions are assumed to be there for SPEED as well as
   // for convenience.  Therefore, do not yield more than once every .2
   // seconds, except one time initially when the thread is created.

   while(!LoopDispatch() && lpBatchInfo &&
         lpBatchInfo->lpBlockInfo->dwEntryCount >= dw1)
   {
      dwTickCount = GetTickCount();

      do
      {
         BatchIteration();   // higher priority here...

      } while(GetTickCount() < (dwTickCount + 200) &&         // < .2 seconds
              lpBatchInfo &&
              lpBatchInfo->lpBlockInfo->dwEntryCount >= dw1); // not RETURN
   }

   if(MyBatchEcho)
   {
      NoBatchEchoFlag--;  // restore the echo
   }

   return(0);
}




LONG FAR PASCAL MemoryBatchFileThreadProc(HANDLE hCaller, HANDLE hParms, UINT wFlags)
{
LPSTR lp1, lpProg;
BOOL bFlag;


   // make a copy of 'hParms' and yield immediately

   lp1 = GlobalLock(hParms);
   if(!lp1)  return(1);

   lpProg = GlobalAllocPtr(GMEM_MOVEABLE, lstrlen(lp1) + 1);
   if(!lpProg)
   {
      GlobalUnlock(hParms);
      return(2);
   }

   lstrcpy(lpProg, lp1);
   GlobalUnlock(hParms);

   // it's now safe to yield!

   if(hMemoryBatchFileThread)  // is there another out there waiting?
   {
    HANDLE hOtherThread = hMemoryBatchFileThread;

      hMemoryBatchFileThread = MthreadGetCurrentThread();

      bFlag = /* LoopDispatch() || */ MthreadWaitForThread(hOtherThread);
   }
   else
   {
      hMemoryBatchFileThread = MthreadGetCurrentThread();

      bFlag = LoopDispatch();
   }


   while(!bFlag)     // it's safe to procede - wait for idle status
   {
      if(lpBatchInfo || waiting_flag || BatchMode || lpCurCmd)
      {
         if(MthreadSleep) MthreadSleep(50);  // sleep for 50 msecs to improve efficiency

         bFlag = LoopDispatch();
      }
      else
      {
         break; // it's safe to continue!
      }
   }


   if(!bFlag)
   {
      if(BatchEcho) NoPromptFlag++;       // eliminate the 'leading prompt'

      PrintString("* BEGIN MEMORY BATCH FILE *\r\n");  // initial message

      ExecuteBatchFile(NULL, lpProg);     // execute it, NOW!  Returns
                                          // immediately, and batch file
                                          // runs in 'hBatchThread'
   }


   GlobalFreePtr(lpProg);

   if(hMemoryBatchFileThread == MthreadGetCurrentThread())
   {
      hMemoryBatchFileThread = NULL;
   }

   return(0);
}



LONG FAR PASCAL FlashThread(HANDLE hCaller, HANDLE hParms, UINT wFlags)
{
static int iFlash=0, iFlash1=0, iFlash2=0, jFlash=0;
static char old_title[80];
DWORD dwTime;


   hFlashThread = MthreadGetCurrentThread();

   while(!LoopDispatch())
   {

      UpdateStatusBar();

                         /** TITLE BAR FLASHING **/

      if(hMainWnd && IsWindow(hMainWnd) &&
         (copy_complete_flag || format_complete_flag))
      {

         if(iFlash1==0 && iFlash2==0 && jFlash==0)
         {
            GetWindowText(hMainWnd, old_title, sizeof(old_title));
            iFlash = 0;
         }

            /* handle 'COPYING' environment variable */

         if(!lpCopyInfo) SetEnvString(szCOPYING,szFALSE);
         else            SetEnvString(szCOPYING,szTRUE);

         if(formatting)  SetEnvString(szFORMATTING,szTRUE);
         else            SetEnvString(szFORMATTING,szFALSE);

         if(lpCopyInfo || iFlash1>16)
         {
            copy_complete_flag = FALSE;
            iFlash1 = 0;

            if(!format_complete_flag)
            {
               iFlash = 0;
               jFlash = 0;
               FlashWindow(hMainWnd, FALSE);
               SetWindowText(hMainWnd, old_title);
            }
         }
         else if(copy_complete_flag && jFlash==0)
         {
            if(iFlash1>=16)
            {
               iFlash1++;
               continue;   /* see section above that handles this! */
            }

            dwTime = GetTickCount();

            if(iFlash & 2)
            {
               SetWindowText(hMainWnd, old_title);
            }
            else if(copy_complete_flag &&
                    (!format_complete_flag || (iFlash1 & 4)))
            {
               SetWindowText(hMainWnd, "** ALL COPIES COMPLETED! **");
            }


            if(!format_complete_flag)
            {
               FlashWindow(hMainWnd, TRUE);

               jFlash = 1;
               iFlash++;
            }

            iFlash1++;
         }


         if(formatting || iFlash2>16)
         {
            format_complete_flag = FALSE;
            iFlash2 = 0;

            if(!copy_complete_flag)
            {
               iFlash = 0;
               jFlash = 0;
               FlashWindow(hMainWnd, FALSE);
               SetWindowText(hMainWnd, old_title);
            }
         }
         else if(format_complete_flag && jFlash==0)
         {
            if(iFlash2>=16)
            {
               iFlash2++;
               continue;   /* see section above that handles this! */
            }

            dwTime = GetTickCount();

            if(iFlash & 2)
            {
               SetWindowText(hMainWnd, old_title);
            }
            else if(format_complete_flag &&
                    (!copy_complete_flag || !(iFlash & 4)))
            {
               SetWindowText(hMainWnd, "** ALL FORMATS COMPLETED! **");
            }

            FlashWindow(hMainWnd, TRUE);

            jFlash = 1;
            iFlash2++;
            iFlash++;
         }



         if(jFlash && (DWORD)(GetTickCount() - dwTime) > 200)
         {
            jFlash = 0;
         }

      }
      else
      {
         if(MthreadSleep) MthreadSleep(50);    // wait an estimated 50 msec
      }
   }

   return(0);
}





/***************************************************************************/
/*                                                                         */
/*           Background Operations - like Copy, and so forth               */
/*                                                                         */
/***************************************************************************/



#pragma code_seg("THREAD_ITERATION_TEXT","CODE")


BOOL FAR PASCAL SubmitFilesToCopy(LPSTR lpSrc, LPSTR lpDest, BOOL verify,
                                  BOOL def_binary, BOOL fromfile_binary,
                                  BOOL tofile_binary)
{
LPCOPY_INFO lpNewCopyInfo;
LPSTR lp1, lp2, lp3, lp4;
WORD wNFiles;
extern HWND hdlgQueue;  // declared in wincmd.c



   lpNewCopyInfo = (LPCOPY_INFO)
      GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                     sizeof(COPY_INFO) + 2 * lstrlen(lpSrc) + lstrlen(lpDest)
                     + MAX_PATH * 2 + 32);

                         /* source file name */

   lp1 = ((LPSTR)lpNewCopyInfo) + sizeof(COPY_INFO);

   lstrcpy(lp1, lpSrc);

#ifdef WIN32
   lpNewCopyInfo->source = lp1;
#else  // WIN32
   *(WORDPTR(lpNewCopyInfo->source)) = OFFSETOF(lp1);
#endif // WIN32

                       /* destination file name */


   lp2 = lp1 + lstrlen(lp1) + 1;
   lp3 = lp2 + MAX_PATH;

   if(*lpDest == '\"')
   {
      lstrcpy(lp3, lpDest + 1);

      if(*lp3 && lp3[lstrlen(lp3) - 1]=='\"') lp3[lstrlen(lp3) - 1] = 0;
   }
   else
   {
      lstrcpy(lp3, lpDest);
   }

   DosQualifyPath(lp2, lp3);  /* fully qualify the destination name */
   if(!*lp2)
   {
      PrintErrorMessage(515);
      GlobalFreePtr((LPSTR)lpNewCopyInfo);
      return(TRUE);
   }

   lp3 = lp2 + lstrlen(lp2) + 1; /* point lp3 just past 'lp2' string */

   QualifyPath(lp3, lp2, TRUE);  /* take care of '*.*' if needed */

   lstrcpy(lp2, lp3);            /* move result to where 'lp2' points */

#ifdef WIN32
   lpNewCopyInfo->dest = lp2;
#else  // WIN32
   *(WORDPTR(lpNewCopyInfo->dest)) = OFFSETOF(lp2);
#endif // WIN32

   lp2 += lstrlen(lp2) + 1;


                     /* if src file has no '+'s in it */
                     /* generate a 'dirlist' for it!  */

   for(lp4=lpSrc; *lp4 && *lp4 != '+'; lp4++)
   {
      if(*lp4=='\"')
      {
         lp4++;

         while(*lp4 && *lp4 != '\"') lp4++;
      }
   }

   if(*lp4 != '+')
   {
      if(*lp1=='\"')
      {
         lp1++;

         if(*lp1 && lp1[lstrlen(lp1) - 1]=='\"') lp1[lstrlen(lp1) - 1] = 0;
      }

      wNFiles = GetDirList(lp1, _A_HIDDEN|_A_SYSTEM|_A_ARCH|_A_RDONLY|_A_DEV,
                           &(lpNewCopyInfo->lpDirInfo), lp2);

#ifdef WIN32
      lpNewCopyInfo->srcpath = lp2;
#else  // WIN32
      *(WORDPTR(lpNewCopyInfo->srcpath)) = OFFSETOF(lp2);
#endif // WIN32

      lp2 += lstrlen(lp2) + 1;

      if(wNFiles == DIRINFO_ERROR)
      {
          return(CMDErrorCheck(TRUE));
      }
   }
   else
   {
      lpNewCopyInfo->lpDirInfo = (LPDIR_INFO)NULL;  /* make sure!!! */
      wNFiles = 1;
   }

   if(wNFiles==0)
   {
      GlobalFreePtr(lpNewCopyInfo->lpDirInfo);
//
//      PrintString("?No source files found matching criteria\r\n");
//
      GlobalFreePtr((LPSTR)lpNewCopyInfo);

      return(FALSE);
   }

                     /** assign the remaining items **/

   lpNewCopyInfo->wNFiles         = wNFiles;
   lpNewCopyInfo->wCurFile        = 0;
   lpNewCopyInfo->def_binary      = def_binary;
   lpNewCopyInfo->tofile_binary   = tofile_binary;
   lpNewCopyInfo->fromfile_binary = fromfile_binary;
   lpNewCopyInfo->verify          = verify;
   lpNewCopyInfo->modify          = FALSE;  /* the '/m' in XCOPY - N/A here */
   lpNewCopyInfo->file_is_open    = FALSE;
   lpNewCopyInfo->hSrc            = HFILE_ERROR;
   lpNewCopyInfo->hDest           = HFILE_ERROR;

   lpNewCopyInfo->buf_start       = NULL;
   lpNewCopyInfo->next_source     = NULL;
   lpNewCopyInfo->lpNext          = (LPCOPY_INFO)NULL;

#ifdef WIN32
   lpNewCopyInfo->original_end = lp2;
#else  // WIN32
   *(WORDPTR(lpNewCopyInfo->original_end)) = OFFSETOF(lp2);
#endif // WIN32

   if(lpCopyInfo)
   {
    LPCOPY_INFO lpCI = lpCopyInfo;

      while(lpCI->lpNext)   lpCI = lpCI->lpNext;

      lpCI->lpNext = lpNewCopyInfo;  /* append to queue!! */

   }
   else
   {
      lpCopyInfo = lpNewCopyInfo;
      copying = TRUE;
   }


          /** Inform user that he actually *DID* something! **/

   TotalCopiesSubmitted += wNFiles;  /* update total # of copies submitted */

   SetEnvString(szCOPYING,szTRUE);  /* assign env string for BATCH info */


   UpdateStatusBar();

   if(hdlgQueue)
   {
      SendMessage(hdlgQueue, WM_COMMAND,
                  (WPARAM)0x4321, (LPARAM)0x12345678);
   }

   return(FALSE);  /* that's it! */

}


    /* this next one submits a 'DIR_INFO' structure */

BOOL FAR PASCAL SubmitDirInfoToCopy(LPDIR_INFO lpDISrc, WORD wNFiles,
                                    LPSTR lpPath, LPSTR lpDest, BOOL verify,
                                    BOOL modify)
{
LPCOPY_INFO lpNewCopyInfo;
LPSTR lp1, lp2, lp3;
extern HWND hdlgQueue;  // declared in wincmd.c


   if(!lpDISrc || wNFiles==0 || !lpPath || !lpDest)
   {
      return(TRUE); /* bad argument - error! */
   }


   lpNewCopyInfo = (LPCOPY_INFO)
      GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                     sizeof(COPY_INFO) + lstrlen(lpPath) + lstrlen(lpDest)
                     + 2 * MAX_PATH + 32);


                       /* destination file name */


   lp2 = ((LPSTR)lpNewCopyInfo) + sizeof(COPY_INFO);
   lp3 = lp2 + MAX_PATH;

   if(*lpDest == '\"')
   {
      lstrcpy(lp3, lpDest + 1);

      if(*lp3 && lp3[lstrlen(lp3) - 1]=='\"') lp3[lstrlen(lp3) - 1] = 0;
   }
   else
   {
      lstrcpy(lp3, lpDest);
   }


   DosQualifyPath(lp2, lpDest);  /* fully qualify the destination name */
   if(!*lp2)
   {
      PrintErrorMessage(515);
      GlobalFreePtr((LPSTR)lpNewCopyInfo);
      return(TRUE);
   }


   lp3 = lp2 + lstrlen(lp2) + 1; /* point lp3 just past 'lp2' string */

   QualifyPath(lp3, lp2, TRUE);  /* take care of '*.*' if needed */

   lstrcpy(lp2, lp3);            /* move result to where 'lp2' points */

#ifdef WIN32
   lpNewCopyInfo->dest = lp2;
#else  // WIN32
   *(WORDPTR(lpNewCopyInfo->dest)) = OFFSETOF(lp2);
#endif // WIN32

   lp1 += lstrlen(lp2) + 1;


                            /* source path */

   lp1 = lp2 + lstrlen(lp2) + 1;

   if(*lpPath == '\"')
   {
      lstrcpy(lp1, lpDest + 1);

      if(*lp1 && lp1[lstrlen(lp1) - 1]=='\"') lp1[lstrlen(lp1) - 1] = 0;
   }
   else
   {
      lstrcpy(lp1, lpPath);
   }


#ifdef WIN32
   lpNewCopyInfo->source = NULL;
   lpNewCopyInfo->srcpath = lp1;
#else  // WIN32
   *(WORDPTR(lpNewCopyInfo->source)) = 0;
   *(WORDPTR(lpNewCopyInfo->srcpath)) = OFFSETOF(lp1);
#endif // WIN32

   lp1 += lstrlen(lp1) + 1;    /* the next 'available' item... */



                     /** assign the remaining items **/

   lpNewCopyInfo->lpDirInfo       = lpDISrc;

   lpNewCopyInfo->wNFiles         = wNFiles;
   lpNewCopyInfo->wCurFile        = 0;
   lpNewCopyInfo->def_binary      = TRUE;  /* always BINARY! */
   lpNewCopyInfo->tofile_binary   = TRUE;  /* always BINARY! */
   lpNewCopyInfo->fromfile_binary = TRUE;  /* always BINARY! */
   lpNewCopyInfo->verify          = verify;
   lpNewCopyInfo->modify          = modify;  /* the '/m' in XCOPY */
   lpNewCopyInfo->file_is_open    = FALSE;
   lpNewCopyInfo->hSrc            = HFILE_ERROR;
   lpNewCopyInfo->hDest           = HFILE_ERROR;

   lpNewCopyInfo->buf_start       = NULL;
   lpNewCopyInfo->next_source     = NULL;
   lpNewCopyInfo->lpNext          = (LPCOPY_INFO)NULL;

#ifdef WIN32
   lpNewCopyInfo->original_end = lp1;
#else  // WIN32
   *(WORDPTR(lpNewCopyInfo->original_end)) = OFFSETOF(lp1);
#endif // WIN32

   if(lpCopyInfo)
   {
    LPCOPY_INFO lpCI = lpCopyInfo;

      while(lpCI->lpNext)   lpCI = lpCI->lpNext;

      lpCI->lpNext = lpNewCopyInfo;  /* append to queue!! */

   }
   else
   {
      lpCopyInfo = lpNewCopyInfo;
      copying = TRUE;
   }


          /** Inform user that he actually *DID* something! **/

   TotalCopiesSubmitted += wNFiles;  /* update total # of copies submitted */

   SetEnvString(szCOPYING,szTRUE);  /* assign env string for BATCH info */


   UpdateStatusBar();

   if(hdlgQueue)
   {
      SendMessage(hdlgQueue, WM_COMMAND,
                  (WPARAM)0x4321, (LPARAM)0x12345678);
   }

   return(FALSE);  /* that's it! */

}


void FAR PASCAL ReportCopyResults(void)
{
extern HWND hdlgQueue;  // declared in wincmd.c
char tbuf[128];


   if(BlockCopyResults) return;

   if(TotalCopiesSubmitted)
   {
      if(TotalCopiesSubmitted>1)
      {
         wsprintf(tbuf, "%ld file sets", TotalCopiesSubmitted);
      }
      else
      {
         lstrcpy(tbuf, "1 file set");
      }

      lstrcat(tbuf," added to COPY QUEUE for background copy\r\n\n");
      PrintString(tbuf);
   }
   else
   {
      PrintErrorMessage(853); // "No files added to COPY QUEUE.\r\n"
                              // "Source file specification does not match any existing files\r\n\n"
   }


   TotalCopiesSubmitted = 0;

   UpdateStatusBar();

   if(hdlgQueue)
   {
      SendMessage(hdlgQueue, WM_COMMAND,
                  (WPARAM)0x4321, (LPARAM)0x12345678);
   }
}




void FAR PASCAL SubmitFormatRequest(UINT wDrive, UINT wTracks, UINT wHeads,
                                    UINT wSectors, UINT wSwitchFlag,
                                    LPSTR lpLabel)
{
LPFORMAT_REQUEST lpF, lpF2;
LPSTR lp1, lp2;
WORD w;
extern HWND hdlgQueue;  // declared in wincmd.c



   lpF2 = GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                         sizeof(FORMAT_REQUEST));

   if(!lpF2)
   {
      PrintErrorMessage(516);
      return;
   }


   lpF2->wDrive      = wDrive;
   lpF2->wTracks     = wTracks;
   lpF2->wHeads      = wHeads;
   lpF2->wSectors    = wSectors;
   lpF2->wSwitchFlag = wSwitchFlag;
   lpF2->lpNext      = NULL;

   lp1 = lpLabel;
   while(lp1 && *lp1 && *lp1 <= ' ') lp1++;  // trim lead white space

   for(w=0, lp2=lpF2->label; w<(sizeof(lpF2->label) - 1); w++)
   {
      if(lp1 && *lp1 > ' ')      // assume end of string at 1st white space
      {
         *lp2++ = *lp1++;
      }
      else
      {
         *lp2++ = ' ';
      }
   }

   *lp2 = 0;  /* terminate string */


   // check if name had embedded white space, and warn user if so

   while(lp1 && *lp1 && *lp1 <= ' ') lp1++;  // find next non-white-space

   if(lp1 && *lp1)
   {
      PrintString("?Warning - label truncated due to embedded white space\r\n\n");
   }




       /** add the information to the 'lpFormatRequest' chain **/


   if(lpFormatRequest)
   {
      for(lpF=lpFormatRequest; lpF->lpNext; lpF = lpF->lpNext)
         ;  /* find the LAST entry! */

      lpF->lpNext = lpF2;
   }
   else
   {
      lpFormatRequest = lpF2;
   }


   UpdateStatusBar();

   if(hdlgQueue)
   {
      SendMessage(hdlgQueue, WM_COMMAND,
                  (WPARAM)0x4321, (LPARAM)0x12345678);
   }

}




              /*** COPY ITERATION - MULTI-THREAD VERSION ***/

          /* this function processes a single 'CopyInfo' entry */


__inline BOOL PASCAL CopyIterationYield(void)
{
int i;
DWORD dw;
static DWORD dwStartTime=0;

       /* ensure I run for at least 100 msec before yielding! */

   dw = GetTickCount();
   if((dw - dwStartTime)<100) return(FALSE);  /* don't yield (yet) */

   if(LoopDispatch()) return(TRUE);

   UpdateStatusBar();  // force status bar to update, and paint if needed

   dwStartTime = dw;

   return(FALSE);
}


void FAR PASCAL CopyIteration(void)
{
WORD i, wBufSize, wDestDrive, wDestSectorSize, wDestClusterSize;
DWORD dwNBytes, dwPos, dwReadStart, dwInputSize;
HFILE hTemp;
int ierr;
BOOL error_occurred=FALSE, SrcIsCON, DestIsCON, DestIsPRN, CtrlZFlag=FALSE,
     SourceBinary, DestBinary, WriteError, VerifyError, BufSizeFlag,
     bSpecialWriteMode, bPlusFound;
LPSTR lp1, lp2, lp3;
LPDIR_INFO lpDirInfo;
LPCOPY_INFO lpCI;
volatile LPSTR lpV;
char tbuf[MAX_PATH * 2];

static unsigned file_date, file_time, file_attr;
static char pCOPYERROR[]="*** COPY ERROR ***";
static char pCTRLZ[]="\x1a";




   wBufSize = COPY_BUFSIZE;
   bPlusFound = FALSE;          // flags when '+' in source file spec


   if(lpCopyInfo==(LPCOPY_INFO)NULL)
   {
      copying = FALSE;

      UpdateStatusBar();

      CopyCancelFlag = FALSE;      // always, at this point

      return;
   }

   INIT_LEVEL


             /** Check if the buffer has been created (yet) **/

   if(!(lpCopyInfo->buf_start))  /* no buffer (yet)! */
   {
      dwNBytes = ((WORD)lpCopyInfo->original_end) + (DWORD)
           (lpCopyInfo->verify?2L * (DWORD)COPY_BUFSIZE:(DWORD)COPY_BUFSIZE);

      lpCI = (LPCOPY_INFO)
          GlobalReAllocPtr((LPSTR)lpCopyInfo, dwNBytes,
                           GMEM_MOVEABLE | GMEM_ZEROINIT);
      if(!lpCI)
      {
       static const char CODE_BASED pMsg[]=
                                                "?Out of memory for copy!!!";
         LockCode();

         MessageBox(hMainWnd, pMsg, pCOPYERROR,
                    MB_OK | MB_ICONHAND | MB_SYSTEMMODAL);

         UnlockCode();

         lpCopyInfo->wNFiles = 0;
         lpCopyInfo->wCurFile = 0x7fff;
         break;       /* forces structure to go away! */

      }
      lpCopyInfo = lpCI;  /* new structure */
      lpCopyInfo->buf_start = lpCopyInfo->original_end;
   }



         /** determine which file I am to copy next **/

   lpV = lpCopyInfo->buf_start;

   if(lpCopyInfo->wCurFile < lpCopyInfo->wNFiles
      || lpCopyInfo->next_source)
   {

      if(lpCopyInfo->next_source || !(lpDirInfo = lpCopyInfo->lpDirInfo))
      {
           /* if 'DIRINFO' is null then there's a single file spec */
           /* which probaly has '+'s in it.  Get next entry!       */


         if(lpCopyInfo->next_source)
         {
            lp1 = (LPSTR)(lpCopyInfo->source = lpCopyInfo->next_source);
         }
         else
         {
            lp1 = (LPSTR)(lpCopyInfo->source);
         }

         if(!*lp1)         /* lp1 points to end of string! */
         {
            if(lpCopyInfo->hDest!=HFILE_ERROR)
            {
               _lclose(lpCopyInfo->hDest);
               break;
            }
         }

         // if this file name is surrounded by quote marks, strip them off
         // and check for the presence of a '+' after it...

         if(*lp1 == '\"')
         {
            lp2 = ++lp1;

            while(*lp2 && *lp2 != '\"') lp2++;

            if(*lp2 == '/') lstrcpy(lp2, lp2 + 1);  // get rid of quote mark...
            else if(*lp2)   *(lp2++) = 0;

            while(*lp2 && *lp2 != '+') lp2++; /* find '+' or end of string */

            if(!*lp2) lp2 = NULL;
         }
         else
         {
            lp2 = _fstrchr(lp1, '+'); /* find '+' or end of string */
         }


         if(lp2)
         {
            bPlusFound = TRUE;

#ifdef WIN32
            lpCopyInfo->next_source = lp2 + 1;
#else  // WIN32
            *(WORDPTR(lpCopyInfo->next_source)) = OFFSETOF(lp2 + 1);
#endif // WIN32
            *lp2 = 0;

            SourceBinary = lpCopyInfo->def_binary;
         }
         else  /* on the 'last' file point to end of name list */
         {
#ifdef WIN32
            lpCopyInfo->next_source = lp1 + lstrlen(lp1);
#else  // WIN32
            *(WORDPTR(lpCopyInfo->next_source)) = OFFSETOF(lp1 + lstrlen(lp1));
#endif // WIN32

            SourceBinary = lpCopyInfo->fromfile_binary;
         }

         if(lp2 = _fstrrchr(lp1, '/'))  /* find a switch! */
         {
            if(*(lp2 - 1)!=':')  /* not a 'device' file spec */
            {
               if(toupper(lp2[1])=='A' || toupper(lp2[1])=='B')
               {
                  SourceBinary = (toupper(lp2[1])=='B');
                  *lp2 = 0;  /* prevent using '/' as part of file name */
               }
                 /** IGNORE ANY OTHER SWITCHES! **/
            }
         }
      }
      else
      {
             /** PROCESS NORMAL ENTRY (no '+' in it!!) **/

         lpDirInfo = lpCopyInfo->lpDirInfo + (lpCopyInfo->wCurFile++);

         _fmemset(lpV, 0, COPY_BUFSIZE);  /* intially zero it */

         lstrcpy(lpV, lpCopyInfo->srcpath);

         if(lpV[lstrlen(lpV)-1]!='\\')
            lstrcat(lpV, "\\");

         _fstrncat(lpV, lpDirInfo->fullname,
                   sizeof(lpDirInfo->fullname));

         SourceBinary = lpCopyInfo->fromfile_binary;
         lp1 = lpV;
      }

      DosQualifyPath(lpV, lp1); // this takes care of character devices also



      if(lpCopyInfo->hSrc!=HFILE_ERROR)
      {
         _lclose(lpCopyInfo->hSrc);

         lpCopyInfo->hSrc = HFILE_ERROR;
      }

          /**------------------------------------------**/
          /** RETRY LOOP FOR OPENING SOURCE/DEST FILES **/
          /**------------------------------------------**/

      do
      {
         if(CopyCancelFlag) break;  // must break out HERE....


         bSpecialWriteMode = FALSE;


                      /** INPUT FILE! **/

         if(lpCopyInfo->hSrc==HFILE_ERROR)
         {
            lpCopyInfo->hSrc = MyOpenFile(lp1, &(lpCopyInfo->ofsrc),
                                          OF_READ | OF_SHARE_DENY_WRITE);
         }

         if(CopyCancelFlag) break;

         if(lpCopyInfo->hSrc==HFILE_ERROR)
         {
            lpCopyInfo->hSrc = MyOpenFile(lp1, &(lpCopyInfo->ofsrc),
                                          OF_READ | OF_SHARE_COMPAT);
         }

         if(CopyCancelFlag) break;

         if(lpCopyInfo->hSrc==HFILE_ERROR)
         {
            // To overcome a bug, if the CON device is being read from
            // it's ok to be copying from the CON device, even when
            // another program is WRITING to it!

            lpCopyInfo->hSrc = MyOpenFile(lp1, &(lpCopyInfo->ofsrc),
                                          OF_READ | OF_SHARE_DENY_NONE);

            if(IsChicago && lpCopyInfo->hSrc==HFILE_ERROR)
            {
               // for Chicago only - attempt to open AGAIN in case it's
               // a device name...

               lpCopyInfo->hSrc = OpenFile(lp1, &(lpCopyInfo->ofsrc),
                                           OF_READ | OF_SHARE_DENY_NONE);
            }

            if(lpCopyInfo->hSrc != HFILE_ERROR &&
               !IsCONDevice(lpCopyInfo->hSrc))  // if not CON device, then
            {                                   // somebody's writing to it!
               _lclose(lpCopyInfo->hSrc);       // Therefore, can't I copy
                                                // this file.
               lpCopyInfo->hSrc = HFILE_ERROR;
            }
         }

         if(CopyCancelFlag) break;

         if(lpCopyInfo->hSrc==HFILE_ERROR)
         {
            lstrcpy(tbuf, "?Error opening file: \"");
            lstrcat(tbuf, lp1);
            lstrcat(tbuf, "\"!!");

            ierr = MyErrorMessageBox(hMainWnd, tbuf, pCOPYERROR);
            if(ierr!=IDRETRY)
            {
               if(lpCopyInfo->hDest!=HFILE_ERROR)
               {
                  _lclose(lpCopyInfo->hDest);

                  MyOpenFile(NULL, &(lpCopyInfo->ofdest),
                            OF_REOPEN | OF_DELETE);

                  lpCopyInfo->hDest = HFILE_ERROR;

               }

               error_occurred = TRUE;
               lpCopyInfo->file_is_open = FALSE;
               break;

            }
            else
               continue;
         }

         SrcIsCON = IsCONDevice(lpCopyInfo->hSrc);   /* uh, oh - CON! */
                                                     /* need to verify */
                  /** DEST FILE **/

         if(lpCopyInfo->hDest!=HFILE_ERROR)         /* already open!! */
         {
            lpCopyInfo->file_is_open = TRUE;

            DestIsCON = IsCONDevice(lpCopyInfo->hDest);

            break;
         }

         DestIsCON = FALSE;
         DestIsPRN = FALSE;


         memset(tbuf, 0, sizeof(tbuf)); /* initially zero buffer */

         if(lpDirInfo)         /* we're not appending files together */
         {                                    /* make copy of pattern */
            _fstrncpy(tbuf, lpDirInfo->fullname,
                      sizeof(lpDirInfo->fullname));

            ShortToLongName(tbuf, lpCopyInfo->srcpath);
         }
         else                /* using the '+' on multiple input files */
         {
            lstrcpy(tbuf, lpCopyInfo->dest);
         }

         if(!lpDirInfo ||
            !UpdateFileName(tbuf, lpCopyInfo->dest, tbuf))
         {
            if(!lpDirInfo) DosQualifyPath(tbuf, tbuf);
                                 // this does 'pre-crunching' on device names


            if((hTemp = MyOpenFile(tbuf, &(lpCopyInfo->ofdest),
                                OF_READ | OF_SHARE_DENY_NONE))!= HFILE_ERROR
            || (hTemp = OpenFile(tbuf, &(lpCopyInfo->ofdest),
                                OF_READ | OF_SHARE_DENY_NONE))!= HFILE_ERROR)
            {

         /* if the file exists, and is the same as input file! */

               DestIsCON = IsCONDevice(hTemp);
               _lclose(hTemp);        /* close file now! */

               if(_fstricmp(lpCopyInfo->ofsrc.szPathName,
                            lpCopyInfo->ofdest.szPathName)==0)
               {
                static const char CODE_BASED pMsg[]=
                                            "?Can't copy file onto itself!";
                  LockCode();

                  MessageBox(hMainWnd, pMsg, pCOPYERROR,
                             MB_ICONHAND | MB_OK | MB_SYSTEMMODAL);

                  UnlockCode();

                  if(lpCopyInfo->hSrc!=HFILE_ERROR)
                  {
                     _lclose(lpCopyInfo->hSrc);

                     lpCopyInfo->hSrc = HFILE_ERROR;

                  }

                  error_occurred = TRUE;
                  lpCopyInfo->file_is_open = FALSE;
                  break;

               }

            }


                 /* make sure I don't copy CON to CON */

            if(SrcIsCON && DestIsCON)      /* both are CON - bad news */
            {
             static const char CODE_BASED pMsg[]=
                                            "?Can't copy file onto itself!";

               LockCode();

               MessageBox(hMainWnd, pMsg, pCOPYERROR,
                          MB_ICONHAND | MB_OK | MB_SYSTEMMODAL);

               UnlockCode();

               if(lpCopyInfo->hSrc!=HFILE_ERROR)
               {
                  _lclose(lpCopyInfo->hSrc);

                  lpCopyInfo->hSrc = HFILE_ERROR;

               }

               error_occurred = TRUE;
               lpCopyInfo->file_is_open = FALSE;
               break;

            }


            // check for CON or PRN device, and appropriately update
            // the method I use to copy files

            if(DestIsCON)
            {
               lpCopyInfo->hDest = OpenFile(tbuf,
                                            &(lpCopyInfo->ofdest),
                                            OF_READWRITE);
            }
            else if(IsValidPortName(tbuf))
            {
               lpCopyInfo->hDest = (HFILE)CreatePrinterDCFromPort(tbuf);

               if(lpCopyInfo->hDest)
               {
                  DestIsPRN = TRUE;

                  lp1 = _fstrrchr(lpCopyInfo->ofsrc.szPathName, '\\');
                  if(!lp1) lp1 = lpCopyInfo->ofsrc.szPathName;
                  else     lp1++;

                  if(!*lp1) // just in case
                  {
                     lp1 = "COPY TO PRN";
                  }

                  Escape((HDC)lpCopyInfo->hDest, STARTDOC,
                         lstrlen(lp1), lp1, NULL);
               }
               else
               {
                  lpCopyInfo->hDest = HFILE_ERROR;
               }
            }
            else
            {
               wDestDrive = toupper(lpCopyInfo->ofdest.szPathName[0])
                            - 'A' + 1;
               if(wDestDrive < 1 || wDestDrive > 26) wDestDrive = 0xffff;

               // special:  for Chicago, use LFN version and commit
               // all write operations; else, use 'normal' call

               if(IsChicago && !bPlusFound) // not concatenating files either
               {
                  GetFullName(tbuf, tbuf);

                  // for DISKETTE DRIVES use the 'no buffering' option
                  // unless the file is too small to make it practical...

                  dwPos = _llseek(lpCopyInfo->hSrc, 0, SEEK_CUR);
                  dwInputSize = _llseek(lpCopyInfo->hSrc, 0, SEEK_END);
                  _llseek(lpCopyInfo->hSrc, dwPos, SEEK_SET);

#ifndef WIN32
                  // FOR NOW, don't do this for WIN32 - later, maybe I will

                  if(dwInputSize >= COPY_BUFSIZE &&
                     GetDriveType(toupper(*tbuf) - 'A') == DRIVE_REMOVABLE)
                  {
                     // NOTE:  if the drive capacity exceeds 3Mb, don't do this
                     //        (I have trouble with large removable drives here)

                     DWORD dwDriveCapacity = 0;

                     _asm
                     {
                        push ds
                
                        mov ax, 0x3200      // get DPB for drive (doc'd in DOS 5+)
                        mov dl, BYTE PTR wDestDrive
                
                        call DOS3Call
                
                        mov ax, [BX + 2]    // bytes per sector at offset 02H in DPB
                        mov cl, [BX + 4]    // shift count (sectors to clusters)
                        mov bx, [BX + 0xd]  // highest cluster #
                        shl ax, cl          // ax is now CLUSTER size (in bytes)

                        mov bx, cx

                        pop ds
                
                        mov wDestClusterSize, ax
                        mov WORD PTR dwDriveCapacity, bx
                     }

                     dwDriveCapacity *= wDestClusterSize;

                     if(dwDriveCapacity &&  // only if NO error doing this
                        dwDriveCapacity < (3 * 1024 * 1024))
                     {
                        OutputDebugString("Using 'special write' on copy\r\n");

                        bSpecialWriteMode = TRUE;
                     }
                  }
#endif // WIN32

                  if(bSpecialWriteMode)
                  {
                     lpCopyInfo->ofdest.nErrCode =
                        MyCreateFile(tbuf,
                                  OF_READWRITE | OF_SHARE_EXCLUSIVE
                                  | 0x0100,          // NO BUFFERING!
                                  _A_ARCH,           // file has arch attrib
                                  0x0012,            // CREATE | TRUNCATE
                                  &(lpCopyInfo->hDest));


                     if(!lpCopyInfo->ofdest.nErrCode)
                     {
                        lpCopyInfo->ofdest.nErrCode =
                           GetShortName(lpCopyInfo->ofdest.szPathName, tbuf);
                     }

                     // fill in remaining elements for 'OFSTRUCT'

                     lpCopyInfo->ofdest.cBytes = sizeof(lpCopyInfo->ofdest);

                     lpCopyInfo->ofdest.fFixedDisk = 0;
                  }
                  else
                  {
                     lpCopyInfo->hDest = MyOpenFile(tbuf,
                                            &(lpCopyInfo->ofdest),
                                             OF_CREATE | OF_READWRITE |
                                             OF_SHARE_EXCLUSIVE);
                  }
               }
               else
               {
                  lpCopyInfo->hDest = MyOpenFile(tbuf,
                                            &(lpCopyInfo->ofdest),
                                             OF_CREATE | OF_READWRITE |
                                             OF_SHARE_EXCLUSIVE);
               }

            }

         }

            /* if either of the above 2 function calls failed... */

         if(CopyCancelFlag) break;

         if(lpCopyInfo->hDest==HFILE_ERROR)
         {
            lp2 = lpV + COPY_BUFSIZE / 2;

            lstrcpy(lp2, "?Error creating file: \"");
            lstrcat(lp2, tbuf);
            lstrcat(lp2, "\"!!\r\nPress <RETRY> to retry,"
                         " <CANCEL> to cancel it.");

            ierr = MyErrorMessageBox(hMainWnd, lp2, pCOPYERROR);

            if(ierr!=IDRETRY)
            {
               if(lpCopyInfo->hSrc!=HFILE_ERROR)
               {
                  _lclose(lpCopyInfo->hSrc);

                  lpCopyInfo->hSrc = HFILE_ERROR;

               }

               error_occurred = TRUE;
               lpCopyInfo->file_is_open = FALSE;
               break;

            }
            else
               continue;
         }
         else
         {
            lpCopyInfo->file_is_open = TRUE;  /* files are open!!! */
            break;
         }

      } while(TRUE);

      lpCopyInfo->SrcIsCON = (SrcIsCON)?1:0;
      lpCopyInfo->DestIsCON = (DestIsCON)?1:0;

   }
   else
   {
           /* force a bailout!! */

      lpCopyInfo->file_is_open = FALSE;
      lpCopyInfo->wCurFile     = 0x7fff;
      lpCopyInfo->wNFiles      = 0;

      break;   /* goes to the end where resources are freed */
   }

   dwInputSize = 0;

   if(lpCopyInfo->hSrc!=HFILE_ERROR && !SrcIsCON)
   {
      dwPos = _llseek(lpCopyInfo->hSrc, 0, SEEK_CUR);

      dwInputSize = _llseek(lpCopyInfo->hSrc, 0, SEEK_END);

      _llseek(lpCopyInfo->hSrc, dwPos, SEEK_SET);
   }

   wCopyPercentComplete = 0;  // just beginning this section...

   UpdateStatusBar();


                         /**************************/
                         /** PERFORM A YIELD HERE **/
                         /**************************/

   if(!CopyCancelFlag && !DestIsPRN)
   {

      // do a 'disk reset' immediately before I copy anything...

#ifdef WIN32

      {
      HANDLE hVWIN32 = OpenVWIN32();

         if(hVWIN32 != INVALID_HANDLE_VALUE)
         {
         DIOC_REGISTERS regs;
         DWORD dwTemp;

            FlushDiskSystem(hVWIN32, wDestDrive);


            regs.reg_EAX = 0x3200; // get DPB for drive (doc'd in DOS 5+)
            regs.reg_EDX = wDestDrive & 0xff;

            ExecuteDOSInterrupt(hVWIN32, &regs);

            wDestSectorSize = *((WORD *)(regs.reg_EBX + 2));  // see 16-bit section
            wDestClusterSize = wDestSectorSize << *((BYTE *)(regs.reg_EBX + 4));

            CloseHandle(hVWIN32);
         }
      }

#else  // WIN32

      if(IsChicago && wDestDrive >=1 && wDestDrive <= 26) _asm
      {
         mov ax, 0x710d
         mov cx, 0           // flush flag:  '0' means don't invalidate cache
         mov dx, wDestDrive

         int 0x21            // flushes drive buffers NOW!
      }

      // find out the sector size for the destination drive, but do NOT
      // cause lots of CPU time to be used doing it!!!

      _asm
      {
         push ds

         mov ax, 0x3200      // get DPB for drive (doc'd in DOS 5+)
         mov dl, BYTE PTR wDestDrive

         call DOS3Call

         mov ax, [BX + 2]    // bytes per sector at offset 02H in DPB
         mov cl, [BX + 4]    // shift count (sectors to clusters)
         mov bx, ax
         shl ax, cl          // ax is now CLUSTER size (in bytes)

         pop ds

         mov wDestSectorSize, bx
         mov wDestClusterSize, ax
      }

#endif // WIN32

   }


   // do some 'pre-crunching' on copy buffer size, depending on source
   // and destination drives.  For a removable source or dest, cut the
   // initial buffer size in half.  If both removable, cut to one fourth.

#ifdef WIN32
   if(GetDriveType(lpCopyInfo->ofsrc.szPathName) == DRIVE_REMOVABLE)
#else  // WIN32
   if(GetDriveType(toupper(lpCopyInfo->ofsrc.szPathName[0]) - 'A')
       == DRIVE_REMOVABLE)
#endif // WIN32
   {
      wBufSize = wBufSize >> 1;
   }

#ifdef WIN32
   if(GetDriveType(lpCopyInfo->ofdest.szPathName) == DRIVE_REMOVABLE)
#else  // WIN32
   if(GetDriveType(toupper(lpCopyInfo->ofdest.szPathName[0]) - 'A')
       == DRIVE_REMOVABLE)
#endif // WIN32
   {
      wBufSize = wBufSize >> 1;
   }

   if(wBufSize < max(0x1000, wDestClusterSize))
   {                        // min buffer size is 4k for performance reasons

      wBufSize = max(0x1000, wDestClusterSize);

      BufSizeFlag = TRUE;   // don't try to shrink it any more
   }
   else
   {
      BufSizeFlag = FALSE;  // haven't determined buffer size yet...
   }




   // Now, perform a 'yield' to ensure I let other processes run

   if(CopyIterationYield()) break;

   if(error_occurred)       break;    // takes care of fatal errors (above)
   if(CopyCancelFlag)       break;    // user cancelled the copy



   DestBinary = lpCopyInfo->tofile_binary;

   lpV = lpCopyInfo->buf_start;

   if(SrcIsCON==1)
   {
               /* taken from <ESC> key processing */

      cmd_buf_len = 0;
      memset(cmd_buf, 0, sizeof(cmd_buf));

      PrintErrorMessage(517);

      SrcIsCON = 2;  /* this means 'printed it already' */

      start_col = curcol;   /* update start positions for cmd line edit */
      start_line = curline;

      UpdateWindow(hMainWnd);
   }
   else if(DestIsCON==1)
   {
      cmd_buf_len = 0;
      memset(cmd_buf, 0, sizeof(cmd_buf));

      PrintErrorMessage(518);

      UpdateWindow(hMainWnd);

      DestIsCON = 2;  /* so I don't get multiple messages for '+' input */
   }



   CtrlZFlag = FALSE;
   WriteError = FALSE;

   do      /* main copy loop! */
   {
      if(CopyCancelFlag)       break;    // user cancelled the copy


      if(!SrcIsCON)     /* source is NOT 'CON' device */
      {
         dwReadStart = GetTickCount();  // use this to adjust buffer size

         dwPos = _llseek(lpCopyInfo->hSrc, 0, SEEK_CUR);
         wCopyPercentComplete = (WORD)
                            (dwInputSize ? 100UL * dwPos / dwInputSize : 0);


         dwNBytes = MyRead(lpCopyInfo->hSrc, lpV, wBufSize);

         if(dwNBytes!=-1 && !SourceBinary) /** ASCII input - check ctrl-z **/
         {
            for(i=0, lp2=lpV; i<dwNBytes; i++, lp2++)
            {
               if(*lp2=='\x1a')  /* did we find a ctrl-z? */
               {
                  dwNBytes = i;  /* don't count the ctrl-z at all */
               }
            }
         }
         else if(dwNBytes == -1)      // READ ERROR!!
         {
            SaveDosErrorCodes();

            _llseek(lpCopyInfo->hSrc, dwPos, SEEK_SET);

            RestoreDosErrorCodes();
         }
      }
      else       /* source is 'CON' device - get user input */
      {

         ctrl_break = FALSE;         /* initially, just 'cause */

         dwNBytes = GetUserInput(lpV);

         if(ctrl_break)
         {
            ctrl_break = FALSE;      /* I know why it happened! */
            dwNBytes = 0;             /* flag end - ctrl break */
            CtrlZFlag = TRUE;
            PrintErrorMessage(519);
            DisplayPrompt();

         }

            /* only if there's a string there! */

         for(i=0; i<dwNBytes; i++)
         {
            if(lpV[i] == 26)  /* a CTRL-Z */
            {
               if(!DestBinary)
               {
                  dwNBytes = i;/* do not transfer control-z if dest is ASCII */
               }
               else
               {
                  dwNBytes = i + 1;  /* sends a ctrl-z anyway */
               }

               CtrlZFlag = TRUE;
               lpV[i+1] = 0;

               DisplayPrompt();

               break;
            }
         }
      }

            /* at this point 'dwNBytes==-1' on 'read error' */


      if(dwNBytes)                /* greater than zero bytes read */
      {
         WriteError = FALSE;
         VerifyError = FALSE;

         if(dwNBytes!=-1 && DestIsCON)
         {
            if(ctrl_break)
            {
               ctrl_break = FALSE;
               dwNBytes = 0;           /* this forces an EOF on input */
               PrintAChar('^');
               PrintAChar('C');
               PrintString(pCRLF);
            }
            else
            {
               PrintBuffer(lpV, (WORD)dwNBytes);
               UpdateWindow(hMainWnd);
            }
         }
         else if(dwNBytes!=-1 && DestIsPRN)
         {
            _fmemmove((LPSTR)lpV + sizeof(WORD), lpV, (UINT)dwNBytes);

            // assume less than 64kb of data

            *((WORD FAR *)lpV) = (WORD)dwNBytes;

            Escape((HDC)lpCopyInfo->hDest, PASSTHROUGH, 0, lpV, NULL);
         }
         else if(dwNBytes!=-1)
         {
            dwPos = _llseek(lpCopyInfo->hDest, 0L, SEEK_CUR);

            // if I'm using 'special' write mode, ensure I write a
            // full "buffer's worth' and then truncate the file size

            if((bSpecialWriteMode &&
                MyWrite2(lpCopyInfo->hDest, lpV, (WORD)dwNBytes,
                         wDestSectorSize) != (WORD)dwNBytes) ||
               (!bSpecialWriteMode &&
                MyWrite(lpCopyInfo->hDest, lpV, (WORD)dwNBytes)
                        != (WORD)dwNBytes))
            {
               WriteError = TRUE;
               SaveDosErrorCodes();

               _llseek(lpCopyInfo->hDest, dwPos, SEEK_SET);
               _llseek(lpCopyInfo->hSrc, dwPos, SEEK_SET);

               RestoreDosErrorCodes();
            }
            else if(lpCopyInfo->verify && !DestIsCON)
            {
               _llseek(lpCopyInfo->hDest, dwPos, SEEK_SET);

               _hmemcpy(lpV + COPY_BUFSIZE, lpV, COPY_BUFSIZE);
                               // must do this because of SEGMENT OVERLAP!!

               if(MyRead(lpCopyInfo->hDest, lpV, (WORD)dwNBytes)
                         !=(WORD)dwNBytes ||
                  _hmemcmp(lpV, lpV + COPY_BUFSIZE, dwNBytes))
               {
                  VerifyError = TRUE;

                  _llseek(lpCopyInfo->hDest, dwPos, SEEK_SET); /* for re-try */
                  _llseek(lpCopyInfo->hSrc, dwPos, SEEK_SET);
               }
            }

         }



         if(dwNBytes==-1 || WriteError || VerifyError)
         {
            if(!VerifyError)
            {
               GetExtErrMessage(lpCopyInfo->buf_start);
            }
            else
            {
               lstrcpy(lpCopyInfo->buf_start, "?Copy - Verify Error\r\n");
            }

            if(dwNBytes==-1)
            {
               lstrcat(lpV, "Error occurred in source file:  \"");
               lstrcat(lpV, lpCopyInfo->ofsrc.szPathName);
            }
            else
            {
               lstrcat(lpV, "Error occurred in destination file:  \"");
               lstrcat(lpV, lpCopyInfo->ofdest.szPathName);
            }
            lstrcat(lpV, "\"\r\n");

            ierr = MyErrorMessageBox(hMainWnd, lpV, pCOPYERROR);

            if(ierr!=IDRETRY)             /* user did not press 'retry' */
            {
               _lclose(lpCopyInfo->hSrc);   /* close the files, now! */

               if(DestIsPRN)
               {
                  Escape((HDC)lpCopyInfo->hDest, ABORTDOC, 0, NULL, NULL);
                  DeleteDC((HDC)lpCopyInfo->hDest);
               }
               else
               {
                  _lclose(lpCopyInfo->hDest);
               }

               lpCopyInfo->hSrc = HFILE_ERROR;   /* closes the entry */
               lpCopyInfo->hDest = HFILE_ERROR;
               lpCopyInfo->file_is_open = FALSE;

               if(!DestIsPRN)
               {
                  MyOpenFile("", &(lpCopyInfo->ofdest), OF_REOPEN | OF_DELETE);
                                       /* always purge destination file */
               }

               error_occurred = TRUE;
               break;
            }
         }
      }


      if(CopyCancelFlag) break;


      // see if I haven't determined the ideal buffer size yet.... and
      // if I haven't, verify it and adjust if needed!

      if(!BufSizeFlag && !SrcIsCON && !DestIsPRN && dwNBytes)
      {
         // get a good measurement on drive performance by flushing
         // the file the first couple of passes and measuring the
         // amount of time it takes to do so.

#ifdef WIN32
         if(wDestDrive >=1 && wDestDrive <= 26)
         {
           HANDLE hVWIN32 = OpenVWIN32();
           if(hVWIN32 != INVALID_HANDLE_VALUE)
           {
             FlushDiskSystem(hVWIN32, wDestDrive);
             CloseHandle(hVWIN32);
           }
         }
         else
         {
           FlushFileBuffers((HANDLE)lpCopyInfo->hDest);
         }
#else  // WIN32
         if(IsChicago && wDestDrive >=1 && wDestDrive <= 26) 
         {
           _asm
           {
              mov ax, 0x710d
              mov cx, 0           // flush flag:  '0' means don't invalidate cache
              mov dx, wDestDrive

              int 0x21            // flushes drive buffers NOW!
           }
         }
         else if(!IsChicago)
         {
            // commit the output file - this will force I/O to take place.
            // if I/O is too slow, I'll detect it for sure in the next
            // section.  Otherwise, if the dest is being buffered, I won't.

            if(HIWORD(GetVersion()) >= 0x330)  // MS-DOS 3.3 or later
            {
               hTemp = lpCopyInfo->hDest;

               _asm mov ax, 0x6800
               _asm mov bx, hTemp;
               _asm int 0x21        // force file to 'flush'
            }
            else
            {
               hTemp = lpCopyInfo->hDest;

               _asm mov ax, 0x4500
               _asm mov bx, hTemp
               _asm int 0x21        // duplicate handle

               _asm jnc dup_handle_not_error

               _asm mov ax, -1      // HFILE_ERROR

               dup_handle_not_error:

               _asm mov hTemp, ax

               if(hTemp != HFILE_ERROR) _lclose(hTemp);  // this forces 'flush'
            }

         }
#endif // WIN32


         dwReadStart = GetTickCount() - dwReadStart;
             // how long it takes per buffer - use this to adjust buffer size
             // NOTE:  Must calculate HERE after any possible disk resets!

         // check the amount of time taken to do last read / write operation

         if(dwReadStart > 200 && wBufSize > 0x1000)
         {   // more than 200 msec per iteration, more than 4k buf size...

            dwReadStart /= 200;

            if(dwReadStart == 1)
            {
               wBufSize = wBufSize >> 1;
            }
            else if(dwReadStart >= 2 && dwReadStart < 4)
            {
               wBufSize = wBufSize >> 2;
            }
            else if(dwReadStart >= 4 && dwReadStart < 8)
            {
               wBufSize = wBufSize >> 3;
            }
            else if(dwReadStart >= 8)
            {
               wBufSize = wBufSize >> 4;
            }

            if(wBufSize < max(0x1000, wDestClusterSize))
            {
               wBufSize = max(0x1000, wDestClusterSize);

               BufSizeFlag = TRUE;  // don't try to shrink it any more
            }
         }
         else
         {
            BufSizeFlag = TRUE;     // buffer size determined to be 'ok'
         }
      }
      else if(!SrcIsCON && !DestIsPRN && !bSpecialWriteMode)
      {
          // For Chicago, do another 'disk reset' on the dest drive
          // to force any cached 'write' data to be written to the
          // physical disk (and not just stay in the cache) unless
          // we're doing a binary write to the drive (which does it
          // for us already, so we don't need to do it twice!)

#ifdef WIN32
         if(wDestDrive >=1 && wDestDrive <= 26)
         {
           HANDLE hVWIN32 = OpenVWIN32();
           if(hVWIN32 != INVALID_HANDLE_VALUE)
           {
             FlushDiskSystem(hVWIN32, wDestDrive);
             CloseHandle(hVWIN32);
           }
         }
#else  // WIN32
         if(IsChicago && wDestDrive >=1 && wDestDrive <= 26) _asm
         {
            mov ax, 0x710d
            mov cx, 0           // flush flag:  '0' means don't invalidate cache
            mov dx, wDestDrive

            int 0x21            // flushes drive buffers NOW!
         }
#endif // WIN32
      }

      // YIELD here...

      if(!SrcIsCON && CopyIterationYield())
      {
         break;  /* returns TRUE if thread killed */
      }

   } while(!CopyCancelFlag && dwNBytes && !CtrlZFlag);



                   /***------------------------***/
                   /*** END OF FILE PROCESSING ***/
                   /***------------------------***/

   if(!CopyCancelFlag && !error_occurred && (!dwNBytes || CtrlZFlag))            /*** E O F ***/
   {
           /* if we didn't append files together, the */
             /* destination gets source's date/time */

      if(lpCopyInfo->lpDirInfo && !DestIsCON)
      {
         MyGetFTime((WORD)lpCopyInfo->hSrc, &file_date, &file_time);

         // File's date and time to be assigned LATER in case the file
         // has been written to using 'bSpecialWriteMode' to work around
         // a BUG in Chicago beta release 324 (so far).
      }

      if(IsChicago && bSpecialWriteMode)
      {
         // anticipate need to truncate destination file...

         dwNBytes = _llseek(lpCopyInfo->hSrc, 0, SEEK_END);
      }




      _lclose(lpCopyInfo->hSrc);   /* close the files, now! */


       /* either if I'm not using '+' or I'm done using it */
       /* close the output file; otherwise leave it open.  */

      if(lpCopyInfo->lpDirInfo || !*(lpCopyInfo->next_source))
      {
         if(bSpecialWriteMode)
         {
            // TRUNCATE AND RE-OPEN DEST FILE!!!

            _lclose(lpCopyInfo->hDest);
            lpCopyInfo->hDest = MyOpenFile("", &(lpCopyInfo->ofdest),
                              OF_REOPEN | OF_READWRITE | OF_SHARE_EXCLUSIVE);

            if(lpCopyInfo->hDest == HFILE_ERROR ||
               _llseek(lpCopyInfo->hDest, dwNBytes, SEEK_SET) != dwNBytes)
            {
               MessageBox(hMainWnd, "?CopyError (unbuffered I/O) - destination file truncation failed",
                          "* SFTShell COPY ERROR *",
                          MB_OK | MB_ICONHAND);
            }
            else
            {
               MyWrite(lpCopyInfo->hDest, "", 0);  // assume it worked
            }

            bSpecialWriteMode = 0;  // buffer remaining writes if appending
         }


      //***************************************************************//
      // assign destination file date and time *NOW* (when applicable) //
      //***************************************************************//

         if(lpCopyInfo->lpDirInfo && !DestIsCON && !DestIsPRN)
         {
            MySetFTime((WORD)lpCopyInfo->hDest, file_date, file_time);
         }


         /*  here's where I write the ctrl-z if ASCII   */
         /* make SURE that I check the 'DestIsCON' flag */

         if(!DestIsCON && !DestBinary && !DestIsPRN &&
            MyWrite(lpCopyInfo->hDest, pCTRLZ, 1) != 1)
         {
            do
            {
               GetExtErrMessage(lpCopyInfo->buf_start);

               lstrcat(lpV, "Error occurred in destination file:  \"");
               lstrcat(lpV, lpCopyInfo->ofdest.szPathName);
               lstrcat(lpV, "\"\r\n");

               ierr = MyErrorMessageBox(hMainWnd, lpV, pCOPYERROR);

               if(ierr!=IDRETRY)             /* user did not press 'retry' */
               {
                  _lclose(lpCopyInfo->hDest);  // never CON or PRN here!

                  lpCopyInfo->hSrc = HFILE_ERROR;   /* closes the entry */
                  lpCopyInfo->hDest = HFILE_ERROR;
                  lpCopyInfo->file_is_open = FALSE;

                  MyOpenFile("", &(lpCopyInfo->ofdest), OF_REOPEN | OF_DELETE);
                                          /* always purge destination file */

                  error_occurred = TRUE;
                  break;
               }
            } while(MyWrite(lpCopyInfo->hDest, pCTRLZ, 1) != 1);
         }

         if(!(lpCopyInfo->lpDirInfo))
         {
            lpCopyInfo->wCurFile = 0xffff; /* force a bailout! */
         }

         if(lpCopyInfo->hDest!=HFILE_ERROR)
         {
            if(DestIsPRN)
            {
               Escape((HDC)lpCopyInfo->hDest, NEWFRAME, 0, NULL, NULL);

               Escape((HDC)lpCopyInfo->hDest, ENDDOC, 0, NULL, NULL);

               DeleteDC((HDC)lpCopyInfo->hDest);
            }
            else
            {
               _lclose(lpCopyInfo->hDest);
            }
         }

         lpCopyInfo->hDest = HFILE_ERROR;
      }

           /* if we didn't append files together, the */
            /* destination gets source's attributes */
            /* (note files must be closed first!)   */

      if(lpCopyInfo->lpDirInfo!=(LPDIR_INFO)NULL && !DestIsCON
         && !DestIsPRN && !error_occurred)
      {
         if(!MyGetFileAttr(lpCopyInfo->ofsrc.szPathName, &file_attr))
         {
            if(lpCopyInfo->modify)  /* I must reset archive bit in source! */
            {
               file_attr &= ~_A_ARCH;  /* clear the ARCHIVE bit */

               MySetFileAttr(lpCopyInfo->ofsrc.szPathName, file_attr);
                                           // always assume that it worked.
            }

            file_attr |= _A_ARCH;  /* always set ARCHIVE bit in DEST */
                                   /* (i.e. needs to be backed up) */

            file_attr &= ~_A_RDONLY;  // ALWAYS CLEAR the 'READONLY' bit!

            MySetFileAttr(lpCopyInfo->ofdest.szPathName, file_attr);
                                     /* assume it worked! */
         }

          /* dest attributes == (source attributes | _A_ARCH), always */
          /* except that I *MUST* clear the _A_RDONLY bit if it's set */
      }
      else if(DestIsCON)/* if copy to CON make sure prompt is displayed */
      {
         DisplayPrompt();
      }


      lpCopyInfo->hSrc = HFILE_ERROR;   /* officially closes the entry */
      lpCopyInfo->file_is_open = FALSE;

      break;
   }


   END_LEVEL


   if(CopyCancelFlag || error_occurred)  // close files and purge (if open)
   {
      if(lpCopyInfo->hSrc != HFILE_ERROR)
      {
         _lclose(lpCopyInfo->hSrc);   /* close the files, now! */
      }

      if(lpCopyInfo->hDest != HFILE_ERROR)
      {
         if(DestIsPRN)
         {
            Escape((HDC)lpCopyInfo->hDest, ABORTDOC, 0, NULL, NULL);
            DeleteDC((HDC)lpCopyInfo->hDest);
         }
         else
         {
            _lclose(lpCopyInfo->hDest);

            MyOpenFile("", &(lpCopyInfo->ofdest), OF_REOPEN | OF_DELETE);
                                         /* always purge destination file */
         }
      }

      lpCopyInfo->hSrc = HFILE_ERROR;   /* closes the entry */
      lpCopyInfo->hDest = HFILE_ERROR;
      lpCopyInfo->file_is_open = FALSE;

      CopyCancelFlag = FALSE;          // ALWAYS reset it!
   }

   if(error_occurred)
   {
      if(lpCopyInfo->wNFiles>1 && lpCopyInfo->wCurFile<lpCopyInfo->wNFiles)
      {
       static const char CODE_BASED pMsg[]=
                                           "Continue with remaining copies?";

         LockCode();

         ierr = MessageBox(hMainWnd, pMsg, pCOPYERROR,
                           MB_YESNO | MB_ICONHAND | MB_SYSTEMMODAL);

         UnlockCode();
      }
      else
      {
         ierr = IDNO;
      }

      if(ierr!=IDYES)
      {
         lpCopyInfo->wCurFile = 0x7fff;
         lpCopyInfo->wNFiles = 0;
      }
   }


            /** See if this copy entry is complete yet **/

   if(!(lpCopyInfo->file_is_open) &&
        lpCopyInfo->wCurFile >= lpCopyInfo->wNFiles )
   {

      lp1 = (LPSTR)lpCopyInfo;  /* save for later... */

      if(lpCopyInfo->lpDirInfo)
         GlobalFreePtr(lpCopyInfo->lpDirInfo);

      lpCopyInfo = lpCopyInfo->lpNext;        /* the NEXT item in QUEUE!!! */

      GlobalFreePtr(lp1);                 /* free old pointer and I'm done */

      if(!lpCopyInfo)
      {
               /** FLASH THE TITLE BAR!! I'M DONE NOW!!! **/

         copy_complete_flag = TRUE;   /* does it within message loop! */

      }

   }

   UpdateStatusBar();

   return;                   /* that's all, folks! */
}
