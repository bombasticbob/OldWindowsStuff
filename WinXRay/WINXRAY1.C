/***************************************************************************/
/*                                                                         */
/*     WINXRAY1.C - (c) 1992-94 by R. E. Frazier - all rights reserved     */
/*                                                                         */
/* Application to generate system information lists into a listbox, with   */
/* optional copy of information to the clipboard in TEXT, CSV, BIF format. */
/*                                                                         */
/***************************************************************************/
/*                                                                         */
/*               Portions of the code were taken from WINCMD               */
/*         Copyright 1992,93 by R. E. Frazier - all rights reserved        */
/*                                                                         */
/***************************************************************************/
/*                                                                         */
/*  Portions of this code also taken from WDBUTIL and WMTHREAD 'DLL's      */
/* Copyright 1990-93 by Stewart~Frazier Tools, Inc. - all rights reserved  */
/*                                                                         */
/***************************************************************************/

#define STRICT
#define WIN31
#include "windows.h"
#include "windowsx.h"
#include "toolhelp.h"
#include "stdio.h"
#include "stdlib.h"
#include "memory.h"
#include "string.h"
#include "hugemem.h"
#include "winxray.h"




                /** PROTECTED MODE 'INTERNALS' STRUCTURES **/

typedef struct {
   WORD wOff16;
   WORD wSel;
   WORD wFlags;
   WORD wOff32;
   } CALL_GATE;

typedef CALL_GATE FAR *LPCALL_GATE;

typedef struct {
   WORD wLimit;
   DWORD dwAddr;
   } GDT_INFO;

typedef struct {
   WORD wLimit;
   DWORD dwAddr;
   } IDT_INFO;


typedef struct {

   CONTROL_BLOCK cb;       // 4 DWORD values
   HWND hwndWinOldAp;      // 1 word
   WORD wJunk;             // 1 word

   DWORD dwCB;             // 1 DWORD value

   DWORD dwInfo[10 + 16];  // total structure size is 32 'DWORD's

   } VMLIST;

typedef VMLIST FAR *LPVMLIST;


static WORD pBits[16]={
       0x0001,0x0002,0x0004,0x0008,0x0010,0x0020,0x0040,0x0080,
       0x0100,0x0200,0x0400,0x0800,0x1000,0x2000,0x4000,0x8000 };



/***************************************************************************/
/*                                                                         */
/*   The next section contains the functions & structures for listing      */
/*   open files.  The first part obtains the 'List Of Lists', and then     */
/*   dispatches to the appropriate function to add a line to the listbox   */
/*   depending upon which version of DOS we are currently running.         */
/*                                                                         */
/*   NOTE:  Use of DPMI necessary to translate linear (DOS) addresses to   */
/*          selector:offset address.  A selector is allocated via DPMI     */
/*          function 0000H, and its base:offset updated using.             */
/*                                                                         */
/*                                                                         */
/***************************************************************************/



void FAR PASCAL CreateFileList(HWND hChild)
{
LPSTR lpLOL, lpData;
LPSFT lpSFT;
WORD wSize, verflag, i, wMySel, wIter, wMode, wType;
DWORD dwVer, dwAddr;
static char tbuf[260];


		    /***------------------------***/
		    /*** INITIALIZATION SECTION ***/
		    /***------------------------***/

	  /** Obtain current DOS version using 'GetVersion()' **/

   dwVer = GetVersion();
   if(HIWORD(dwVer)<0x300)
   {
      verflag = DOSVER_2X;
   }
   else if(HIWORD(dwVer)>=0x300 && HIWORD(dwVer)<0x310)
   {
      verflag = DOSVER_30;
   }
   else if(HIWORD(dwVer)>=0x310 && HIWORD(dwVer)<0x400)
   {
      verflag = DOSVER_3X;
   }
   else if(HIWORD(dwVer)>=0x400 && HIWORD(dwVer)<0x700)
   {
      verflag = DOSVER_4PLUS;
   }
   else
   {
      verflag = DOSVER_7PLUS;
   }


	   /** Use INT 21H function 52H to get List of Lists **/

   _asm
   {
      mov ax,0x5200
      call DOS3Call
      mov WORD PTR lpLOL+2 ,es
      mov WORD PTR lpLOL, bx
   }



   lpSFT = *((LPSFT FAR *)(lpLOL + 4));  /** Address of System File Table **/


   wMySel = MyAllocSelector();

   if(!wMySel) return;                  /* '0' selector if error occurs */


			   /***-----------***/
			   /*** MAIN LOOP ***/
			   /***-----------***/

   wIter = 0;

   do
   {
      if(!((++wIter) & 0x1f)) MySetCursor(2);

            /* gotta translate that puppy to *protected* mode */

      dwAddr = SELECTOROF(lpSFT)*16L + OFFSETOF(lpSFT);

      if(MyChangeSelector(wMySel, dwAddr, 0xffff))
      {
	 lpSFT = NULL;
      }
      else
      {
	 lpSFT = (LPSFT)MAKELP(wMySel, 0);
      }

      if(!lpSFT) return;


      lpData = lpSFT->lpData;       /* pointer to array of 'SFT' entries */

	      /** read through 'SFT' array for the total # **/
	      /** of 'SFT' entries in this block.          **/

      for(i=0; i<lpSFT->wCount; i++)
      {
         if(!((++wIter) & 0x1f)) MySetCursor(2);

	 switch(verflag)    /* different structure sizes for */
	 {                  /* different DOS versions.       */

	    case DOSVER_2X:
	       AddLine0(hChild, lpData);
	       lpData += sizeof(struct _sft2_);
	       break;

	    case DOSVER_30:
	       AddLine1(hChild, lpData);
	       lpData += sizeof(struct _sft30_);
	       break;

	    case DOSVER_3X:
	       AddLine2(hChild, lpData);
	       lpData += sizeof(struct _sft31_);
	       break;

            case DOSVER_4PLUS:
            case DOSVER_7PLUS:
	       AddLine3(hChild, lpData);
	       lpData += sizeof(struct _sft4_);
	       break;
	 }
      }
   
      lpSFT = lpSFT->lpNext;       /* get (DOS) pointer to next SFT block */

   } while(SELECTOROF(lpSFT) && OFFSETOF(lpSFT)!=0xffff);

   MySetCursor(2);


			 /**-----------------**/
			 /** CLEANUP SECTION **/
			 /**-----------------**/

   MyFreeSelector(wMySel);

   if(verflag==DOSVER_2X)
   {
      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)0,(LPARAM)
		"Drive:Name.Ext Mode Share  File Pointer");
      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)1,(LPARAM)
		"-------------- ---- -----  ------------");
   }
   else
   {
      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)0,(LPARAM)
		  " File/Dev Name Open Share Machine Owner Cur File Cur File Owner Owner       ");
      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)1,(LPARAM)
		  "Drive:Name.Ext Mode Mode    ID     PSP  Pointer   Length  Task  Module      ");
      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)2,(LPARAM)
                  "-------------- ---- ----- ------- ----- -------- -------- ----- ------------");


      if(verflag==DOSVER_7PLUS)  // chicago only!
      {
         // insert 2 additional lines on top

         SendMessage(hChild, LB_INSERTSTRING, (WPARAM)0,(LPARAM)
                     "           LIST OF OPEN FILES USING MS-DOS's SYSTEM FILE TABLE");

         SendMessage(hChild, LB_INSERTSTRING, (WPARAM)1, (LPARAM)"");


         // insert separator and heading for next section

         SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1,(LPARAM)"");
         SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1,(LPARAM)"");
         SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1,(LPARAM)
                     "                  LIST OF OPEN FILES FOR 32-bit WINDOWS");
         SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1,(LPARAM)
                     "                  =====================================");
         SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1,(LPARAM)"");


         for(i=1; i<=26; i++)
         {
//            _asm
//            {
//               mov ax, 0x440d   // IOCTL
//               mov bh, 1        // lock level #1
//               mov bl, BYTE PTR i // drive number
//               mov cx, 0x084a   // device cat. 8, funct 4a (lock logical volume)
//               mov dx, 1        // permissions (allow read/write by others)
//
//               int 0x21
//
//               jnc lock_ok1      // if I can't lock it, skip it!
//               mov i, 0xffff
//
//               lock_ok1:
//            }
//
//            if(i == 0xffff) break;
//
//
//            _asm
//            {
//               mov ax, 0x440d   // IOCTL
//               mov bh, 2        // lock level #2
//               mov bl, BYTE PTR i // drive number
//               mov cx, 0x084a   // device cat. 8, funct 4a (lock logical volume)
//
//               int 0x21
//
//               jnc lock_ok2      // if I can't lock it, skip it!
//               mov i, 0xffff
//
//               lock_ok2:
//            }
//
//            if(i == 0xffff) break;
//
//
//            _asm
//            {
//               mov ax, 0x440d   // IOCTL
//               mov bh, 3        // lock level #3
//               mov bl, BYTE PTR i // drive number
//               mov cx, 0x084a   // device cat. 8, funct 4a (lock logical volume)
//
//               int 0x21
//
//               jnc lock_ok3      // if I can't lock it, skip it!
//               mov i, 0xffff
//
//               lock_ok3:
//            }
//
//            if(i == 0xffff) break;


            wIter = 0;
            do
            {

               _asm
               {
                  mov ax, 0x440d    // IOCTL
                  mov bx, i         // drive number
                  mov cx, 0x086d    // device cat. 8, func 6d (enum open files)
                  /* mov ds, DGROUP */
                  lea dx, WORD PTR tbuf
                  mov si, wIter     // iteration counter
                  mov di, 0         // enumerate ALL files

                  int 0x21

                  jnc was_ok
                  mov wIter, 0xffff

               was_ok:
                  mov wMode, ax
                  mov wType, cx
               }


               if(wIter != 0xffff)
               {
                  SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1,
                              (LPARAM)tbuf);

                  wsprintf(tbuf, "  Open Mode: %s   SHARE: %s   Type: %s",
                           (LPSTR)pOpenMode[wMode & 7],
                           (LPSTR)pShareMode[(wMode>>4) & 7],
                           (LPSTR)(wType==0?"Normal":
                                   (wType==1?"Memory Mapped":
                                    "Other Un-Moveable")));

                  SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1,
                              (LPARAM)tbuf);


                  SendMessage(hChild,LB_INSERTSTRING,(WPARAM)-1,(LPARAM)"");

                  wIter++;
               }


            } while(wIter != 0xffff);

//            for(wIter=0; wIter<3; wIter++)
//            {
//               _asm
//               {
//                  mov ax, 0x440d   // IOCTL
//                  mov bx, i        // drive
//                  mov cx, 0x086a   // device cat. 8, funct 6a (unlock logical volume)
//
//                  int 0x21
//               }
//            }
         }
      }
   }

}


			/** DOS VERSION 2.XX **/
void FAR PASCAL AddLine0(HWND hChild, LPSTR lpData)
{
struct _sft2_ FAR *lpS;

	     /** will this function *ever* get used? **/

   lpS = (struct _sft2_ FAR *)lpData;
   if(!lpS->n_handles) return;         /* entry not in use */

   wsprintf(buffer, "%c%c%-8.8s.%-3.3s %-4s %-4s  "
		    "   %08lx",
	    lpS->drive?'@'+lpS->drive:' ', lpS->drive?':':' ',
	    lpS->name, lpS->name + 8,
	    (LPSTR)pOpenMode[lpS->open_mode & 7],
	    (LPSTR)pShareMode[((lpS->open_mode)>>4) & 7],
	    lpS->file_pos);

   SendMessage(hChild, LB_ADDSTRING, NULL, (LPARAM)((LPSTR)(buffer)));

}

			/** DOS VERSION 3.0 **/
void FAR PASCAL AddLine1(HWND hChild, LPSTR lpData)
{
struct _sft30_ FAR *lpS;
WORD drive;
LPSTR lpModuleName;
HTASK hTask;


   lpS = (struct _sft30_ FAR *)lpData;
   if(!lpS->n_handles) return;         /* entry not in use */

   if(lpS->dev_info_word & 0x80)       /* bit set if device */
   { 
      drive = 0;
   }
   else
   {
      drive = (lpS->dev_info_word & 31) + 1;
   }

   if(drive)
   {
      wsprintf(buffer, "%c:%-8.8s.%-3.3s ",
		       '@' + drive,
		       lpS->name, lpS->name + 8);
   }
   else
   {
      wsprintf(buffer, "  %-8.8s.%-3.3s ", lpS->name, lpS->name + 8);
   }

   lpModuleName = GetTaskAndModuleFromPSP(lpS->vm_id, lpS->psp_seg, &hTask);

   if(hTask)
   {
      wsprintf(buffer + 15, "%-4.4s %-4.4s   %04x   %04x  "
			      "%08lx %08lx %04x  %s",
			      (LPSTR)pOpenMode[lpS->open_mode & 7],
			      (LPSTR)pShareMode[((lpS->open_mode)>>4) & 7],
			      lpS->vm_id,
			      lpS->psp_seg,
			      lpS->file_pos,
			      lpS->file_size,
			      hTask,
			      lpModuleName);
   }
   else
   {
      wsprintf(buffer + 15, "%-4.4s %-4.4s   %04x   %04x  "
			      "%08lx %08lx       %s",
			      (LPSTR)pOpenMode[lpS->open_mode & 7],
			      (LPSTR)pShareMode[((lpS->open_mode)>>4) & 7],
			      lpS->vm_id,
			      lpS->psp_seg,
			      lpS->file_pos,
			      lpS->file_size,
			      lpModuleName);
   }


   SendMessage(hChild, LB_ADDSTRING, NULL, (LPARAM)((LPSTR)(buffer)));

}

			/** DOS VERSION 3.X **/
void FAR PASCAL AddLine2(HWND hChild, LPSTR lpData)
{
struct _sft31_ FAR *lpS;
WORD drive;
LPSTR lpModuleName;
HTASK hTask;


   lpS = (struct _sft31_ FAR *)lpData;
   if(!lpS->n_handles) return;         /* entry not in use */

   if(lpS->dev_info_word & 0x80)       /* bit set if device */
   { 
      drive = 0;
   }
   else
   {
      drive = (lpS->dev_info_word & 31) + 1;
   }

   if(drive)
   {
      wsprintf(buffer, "%c:%-8.8s.%-3.3s ",
		       '@' + drive,
		       lpS->name, lpS->name + 8);
   }
   else
   {
      wsprintf(buffer, "  %-8.8s.%-3.3s ", lpS->name, lpS->name + 8);
   }

   lpModuleName = GetTaskAndModuleFromPSP(lpS->vm_id, lpS->psp_seg, &hTask);

   if(hTask)
   {
      wsprintf(buffer + 15, "%-4.4s %-4.4s   %04x   %04x  "
			      "%08lx %08lx %04x  %s",
			      (LPSTR)pOpenMode[lpS->open_mode & 7],
			      (LPSTR)pShareMode[((lpS->open_mode)>>4) & 7],
			      lpS->vm_id,
			      lpS->psp_seg,
			      lpS->file_pos,
			      lpS->file_size,
			      hTask,
			      lpModuleName);
   }
   else
   {
      wsprintf(buffer + 15, "%-4.4s %-4.4s   %04x   %04x  "
			      "%08lx %08lx       %s",
			      (LPSTR)pOpenMode[lpS->open_mode & 7],
			      (LPSTR)pShareMode[((lpS->open_mode)>>4) & 7],
			      lpS->vm_id,
			      lpS->psp_seg,
			      lpS->file_pos,
			      lpS->file_size,
			      lpModuleName);
   }



   SendMessage(hChild, LB_ADDSTRING, NULL, (LPARAM)((LPSTR)(buffer)));

}

			/** DOS VERSION 4+ **/
void FAR PASCAL AddLine3(HWND hChild, LPSTR lpData)
{
struct _sft4_ FAR *lpS;
WORD drive;
LPSTR lpModuleName;
HTASK hTask;


   lpS = (struct _sft4_ FAR *)lpData;
   if(!lpS->n_handles) return;         /* entry not in use */

   if(lpS->dev_info_word & 0x80)       /* bit set if device */
   { 
      drive = 0;
   }
   else
   {
      drive = (lpS->dev_info_word & 31) + 1;
   }

   if(drive)
   {
      wsprintf(buffer, "%c:%-8.8s.%-3.3s ",
		       '@' + drive,
		       lpS->name, lpS->name + 8);
   }
   else
   {
      wsprintf(buffer, "  %-8.8s.%-3.3s ", lpS->name, lpS->name + 8);
   }

   lpModuleName = GetTaskAndModuleFromPSP(lpS->vm_id, lpS->psp_seg, &hTask);

   if(hTask)
   {
      wsprintf(buffer + 15, "%-4.4s %-4.4s   %04x   %04x  "
			      "%08lx %08lx %04x  %s",
			      (LPSTR)pOpenMode[lpS->open_mode & 7],
			      (LPSTR)pShareMode[((lpS->open_mode)>>4) & 7],
			      lpS->vm_id,
			      lpS->psp_seg,
			      lpS->file_pos,
			      lpS->file_size,
			      hTask,
			      lpModuleName);
   }
   else
   {
      wsprintf(buffer + 15, "%-4.4s %-4.4s   %04x   %04x  "
			      "%08lx %08lx       %s",
			      (LPSTR)pOpenMode[lpS->open_mode & 7],
			      (LPSTR)pShareMode[((lpS->open_mode)>>4) & 7],
			      lpS->vm_id,
			      lpS->psp_seg,
			      lpS->file_pos,
			      lpS->file_size,
			      lpModuleName);
   }


   SendMessage(hChild, LB_ADDSTRING, NULL, (LPARAM)((LPSTR)(buffer)));


}





void FAR PASCAL CreateTaskList(HWND hChild)
{
TASKENTRY te;
MODULEENTRY me;
LPTASKLIST lpT, lpT2;
WORD wNumTasks, x, wVM, wSysVM;
char tbuf[32];



   // Get SYSTEM VM

   _asm
   {
      push bx
      mov ax, 0x1683
      int 0x2f

      mov wSysVM, bx
      pop bx
   }



   if(!lpTaskFirst || !lpTaskNext)
   {
      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1,(LPARAM)
	 "?Required DLL not present for TASK LIST display - Sorry!");

      return;
   }

   _hmemset((LPSTR)&te, 0, sizeof(te));
   _hmemset((LPSTR)&me, 0, sizeof(me));

   te.dwSize = sizeof(te);
   me.dwSize = sizeof(me);


   wNumTasks = GetNumTasks();

   /* TA-DAAAA!!!!  This one's gonna work now, in 3.0 and 3.1 mode!!! */

   lpT = (LPTASKLIST)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
				    (wNumTasks + 1)*sizeof(TASKLIST));

   if(!lpT || !lpTaskFirst((LPTASKENTRY)&te))
   {
      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1,(LPARAM)
		  "?Unable to obtain task list due to **ERROR**");

      if(lpT)  GlobalFreePtr(lpT);
      return;
   }


   for(lpT2=lpT, x=0; x<wNumTasks; x++, lpT2++)
   {
      MySetCursor(2);

      lpT2->hTask = te.hTask;
      lpT2->hModule = te.hModule;
      lpT2->hInst = te.hInst;

      if(lpModuleFindHandle(&me, te.hModule))
      {
         lstrcpy(lpT2->module, me.szModule);
      }
      else
      {
         lstrcpy(lpT2->module, te.szModule);
      }

      if(!lpTaskNext((LPTASKENTRY)&te)) break;
   }

   if(x<wNumTasks) wNumTasks = x;  /* just in case */

   MySetCursor(2);

   _lqsort((char _huge *)lpT, wNumTasks, sizeof(*lpT), MyTaskListCompare);


   MySetCursor(2);

   SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1,(LPARAM)
               "MODULE NAME  TASK ID  INSTANCE  MODULE ID  VM ID 'DOS' APP");
   SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1,(LPARAM)
               "-----------  -------  --------  ---------  ----- ---------");

   for(lpT2=lpT, x=0; x<wNumTasks; x++, lpT2++)
   {
      if(IsWINOLDAP(lpT2->hTask))
      {
         GetVMInfoFromTask(lpT2->hTask, &wVM, tbuf);

         if(!*tbuf) lstrcpy(tbuf, "NON-WINDOWS");
      }
      else
      {
         *tbuf = 0;

         wVM = wSysVM;
      }

      if((GetWinFlags() & WF_ENHANCED)!=0 || wVM == wSysVM)
      {
         wsprintf(buffer, "%-8.8s      %04xH    %04xH      %04xH     %2d   %s",
                  (LPSTR)lpT2->module, (WORD)lpT2->hTask,
                  (WORD)lpT2->hInst,   (WORD)lpT2->hModule,
                  (WORD)wVM,  (LPSTR)tbuf);
      }
      else
      {
         wsprintf(buffer, "%-8.8s      %04xH    %04xH      %04xH     ???  %s",
                  (LPSTR)lpT2->module, (WORD)lpT2->hTask,
                  (WORD)lpT2->hInst,   (WORD)lpT2->hModule,
                  (LPSTR)tbuf);
      }

      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)((LPSTR)(buffer)));

   }

   if(lpT)  GlobalFreePtr(lpT);
   return;

}

int FAR _cdecl MyTaskListCompare(LPSTR lp1, LPSTR lp2)
{
   return(_fstricmp(((LPTASKLIST)lp1)->module, ((LPTASKLIST)lp2)->module));
}



void FAR PASCAL CreateMemList(HWND hChild)
{
DWORD dwLargest, dwFree, dwFreeVirt,
      total_code, total_data, total_other, total_disc;
WORD nTasks, n1, n2, n3, n4, wIter;
LPSTR lpBuf1, lpBuf2;
static MODULEENTRY me;
static TASKENTRY te;
static GLOBALENTRY ge;
static MEMMANINFO mmi;

struct CMDMEM_MODULE {                       /* size must be power of 2!! */
   char module[MAX_MODULE_NAME + 1];
   HMODULE hModule;
   DWORD total_code;
   DWORD total_data;
   DWORD total_other;
   DWORD total_disc;
   char junk[32 - MAX_MODULE_NAME - 1 - sizeof(HMODULE) - 3 * sizeof(DWORD)];

   } _huge *lpMM1, _huge *lpMM2, _huge *lpMM3;

struct CMDMEM_ENTRY {
   HTASK hOwner;
   DWORD total_code;
   DWORD total_data;
   DWORD total_other;
   DWORD total_disc;
   char junk[16 - sizeof(HTASK) - 3 * sizeof(DWORD)];
   } _huge *lpME1, _huge *lpME2, _huge *lpME3;


	      /** Initialization of TOOLHELP structures **/

   memset(&mmi, 0, sizeof(mmi));
   memset(&ge, 0, sizeof(ge));
   memset(&te, 0, sizeof(te));
   memset(&me, 0, sizeof(me));

   mmi.dwSize = sizeof(mmi);
   ge.dwSize  = sizeof(ge);
   te.dwSize  = sizeof(te);
   me.dwSize  = sizeof(me);

   if(!lpGlobalFirst || !lpGlobalNext
      || !lpTaskFindHandle || !lpModuleFindHandle)
   {
      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1,(LPARAM)
	 "?Required DLL not present for MEMORY display - Sorry!");
      return;
   }



   if(!(lpBuf1 = GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
				sizeof(struct CMDMEM_ENTRY) * 128)))
   {
      return;
   }


   // PERFORM THE FOLLOWING INITIALIZATION HERE JUST IN CASE IT'S MISSED
   // ELSEWHERE - I THEREFORE ENSURE IT GETS DONE AT LEAST ONCE, THOUGH
   // THE VALUES OBTAINED HERE MAY NOT BE TOTALLY ACCURATE...

   dwFree    = GetFreeSpace(0);
   dwLargest = GlobalCompact(0); 
   nTasks    = GetNumTasks();

   if(!lpMemManInfo(&mmi))
   {
      return;
   }




   INIT_LEVEL

   n1 = 0;

   n2 = (WORD)(GlobalSizePtr(lpBuf1) / sizeof(struct CMDMEM_ENTRY));

   lpME1 = (struct CMDMEM_ENTRY _huge *)lpBuf1;
   lpME2 = lpME1;                            /* next available entry... */


   if(!lpGlobalFirst(&ge, GLOBAL_ALL))
   {
      break;
   }

   wIter = 0;

   do
   {
      if(!((++wIter) & 0x1f)) MySetCursor(2);

           /* bug in TOOLHELP - free blocks have NULL hBlock */
	   /*                   and NULL hOwner...           */

      if(ge.wFlags==GT_FREE || (!ge.hBlock && !ge.hOwner))  continue;

      if(n1>=n2)  /* time to re-allocate memory!! */
      {
	 lpBuf2 = GlobalReAllocPtr(lpBuf1,((n1 + 64L) / 64L) * 64L
					   * sizeof(struct CMDMEM_ENTRY),
				   GMEM_MOVEABLE | GMEM_ZEROINIT);
	 if(!lpBuf2)
	 {
	    GlobalFreePtr(lpBuf1);
	    lpBuf1 = (LPSTR)NULL;

	    break;
	 }
	 else
	 {
	    lpBuf1 = lpBuf2;

	    n1 = 0;

	    n2 = (WORD)(GlobalSizePtr(lpBuf1) /
			 sizeof(struct CMDMEM_ENTRY));

	    lpME1 = (struct CMDMEM_ENTRY _huge *)lpBuf1;
	    lpME2 = lpME1;

	    if(!lpGlobalFirst(&ge, GLOBAL_ALL))
	    {
	       break;
	    }

	    if(ge.wFlags==GT_FREE || (!ge.hBlock && !ge.hOwner))  continue;

	 }
      }

      for(n3=0, lpME3 = lpME1; n3<n1; n3++, lpME3++)
      {
         if(lpME3->hOwner == ge.hOwner)
	    break;
      }

      if(n3>=n1)
      {
	 lpME3 = lpME2++;                            /* insurance... */
	 n1++;                                            /* next... */

	 lpME3->hOwner = ge.hOwner;
	 lpME3->total_code  = 0L;
	 lpME3->total_data  = 0L;
	 lpME3->total_other = 0L;
	 lpME3->total_disc  = 0L;

      }


      if(!((++wIter) & 0x1f)) MySetCursor(2);


      if(ge.wType==GT_DATA || ge.wType==GT_DGROUP ||
	 ge.wType==GT_RESOURCE)
      {
	 lpME3->total_data += ge.dwBlockSize;
      }
      else if(ge.wType==GT_CODE)
      {
	 lpME3->total_code += ge.dwBlockSize;
      }
      else
      {
	 lpME3->total_other += ge.dwBlockSize;
      }

       /***** THIS WAS THE 'OLD' WAY OF DOING THINGS *****/
/*       if(ge.dwNextAlt)       /* block is discardable if not NULL!  */
				/* (already know block isn't GT_FREE) */

      if(ge.hBlock && (GlobalFlags(ge.hBlock) & GMEM_DISCARDABLE))
      {

	 lpME3->total_disc += ge.dwBlockSize;
      }


      if(n1 && ge.wFlags==GT_SENTINEL) break;  /* last entry! */


   } while(lpGlobalNext(&ge, GLOBAL_ALL));

   if(!lpBuf1) break;

   MySetCursor(2);


   // BEFORE I ALLOCATE ANY MORE MEMORY, GET IMPORTANT MEMORY INFO HERE!
   // AT THIS POINT IT IS 'IN SYNC' WITH THE STUFF IN THE ARRAY!

   dwFree    = GetFreeSpace(0);
   dwLargest = GlobalCompact(0); 
   nTasks    = GetNumTasks();

   if(!lpMemManInfo(&mmi))
   {
      return;
   }



   lpBuf2 = GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
			   (n1 + 1)* sizeof(struct CMDMEM_MODULE));
   if(!lpBuf2)
   {
      break;
   }

	 /* I've got all of the task handles!! Now, get the modules */

   lpMM1 = (struct CMDMEM_MODULE _huge *)lpBuf2;
   lpMM1->hModule = 0;
   _fstrcpy(lpMM1->module, "* N/A *");

   for(n2=0, n4=0, lpME2=lpME1, lpMM2=lpMM1 + 1; n2<n1; n2++, lpME2++)
   {
      if(!((++wIter) & 0x1f)) MySetCursor(2);

      if(!lpTaskFindHandle(&te, lpME2->hOwner))
      {
	 if(!lpModuleFindHandle(&me, (HMODULE)lpME2->hOwner))
	 {
	    lpMM1->total_code  += lpME2->total_code;
	    lpMM1->total_data  += lpME2->total_data;
	    lpMM1->total_other += lpME2->total_other;
	    lpMM1->total_disc  += lpME2->total_disc;

	    continue;
	 }
      }
      else
      {
	 if(!lpModuleFindHandle(&me, te.hModule))
	 {
	    lpMM1->total_code  += lpME2->total_code;
	    lpMM1->total_data  += lpME2->total_data;
	    lpMM1->total_other += lpME2->total_other;
	    lpMM1->total_disc  += lpME2->total_disc;

	    continue;
	 }
      }

      for(n3=0, lpMM3 = lpMM1 + 1; n3<n4; n3++, lpMM3++)
      {
	 if(lpMM3->hModule == me.hModule)
	 {
	    break;
	 }
      }

      if(!((++wIter) & 0x1f)) MySetCursor(2);

      if(n3>=n4)
      {
	 lpMM3 = lpMM2++;
	 n4++;

	 _fstrcpy(lpMM3->module, me.szModule);
	 lpMM3->hModule = me.hModule;

	 lpMM3->total_code  = 0L;
	 lpMM3->total_data  = 0L;
	 lpMM3->total_other = 0L;
	 lpMM3->total_disc  = 0L;
      }

      lpMM3->total_code  += lpME2->total_code;
      lpMM3->total_data  += lpME2->total_data;
      lpMM3->total_other += lpME2->total_other;
      lpMM3->total_disc  += lpME2->total_disc;

   }

   MySetCursor(2);

		    /* NOW, sort the thing!! */

   _lqsort((LPSTR)(lpMM1 + 1), n4, sizeof(struct CMDMEM_MODULE),
	    CMDMemSortProc);


   MySetCursor(2);


	       /*** Last but not least - output! ***/

   SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1,(LPARAM)
	"MODULE NAME  HANDLE    Code     Data &    Other     Total    Discard-");
   SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1,(LPARAM)
	"(Prog, Lib)  (Hex)   Segments  Resource   Memory    in Use     able  ");
   SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1,(LPARAM)
	"-----------  ------  --------  --------  --------  --------  --------");

   total_code = total_data = total_other = total_disc = 0L;

   for(n3=0, lpMM3=lpMM1; n3<=n4; n3++, lpMM3++)
   {
      if(!((++wIter) & 0x1f)) MySetCursor(2);

      if(!n3)             /* very first entry - 'unknown', essentially */
      {
	 wsprintf(buffer,
	       "* SYSTEM *     N/A   %8lu  %8lu  %8lu  %8lu  %8lu",
	       (DWORD)(lpMM3->total_code),
	       (DWORD)(lpMM3->total_data),
	       (DWORD)(lpMM3->total_other),
	       (DWORD)(lpMM3->total_code + lpMM3->total_data +
		       lpMM3->total_other),
	       (DWORD)(lpMM3->total_disc)   );
      }
      else
      {
	 wsprintf(buffer,
	       "%-8.8s      %04xH  %8lu  %8lu  %8lu  %8lu  %8lu",
	       (LPSTR)(lpMM3->module), (WORD)lpMM3->hModule,
	       (DWORD)(lpMM3->total_code),
	       (DWORD)(lpMM3->total_data),
	       (DWORD)(lpMM3->total_other),
	       (DWORD)(lpMM3->total_code + lpMM3->total_data +
		       lpMM3->total_other),
	       (DWORD)(lpMM3->total_disc)   );
      }

      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)buffer);

      total_code  += lpMM3->total_code;
      total_data  += lpMM3->total_data;
      total_other += lpMM3->total_other;
      total_disc  += lpMM3->total_disc;
   }

   MySetCursor(2);

   SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1,(LPARAM)
      "===================  ========  ========  ========  ========  ========");

   wsprintf(buffer,
      " ** GRAND TOTAL **   %8lu  %8lu  %8lu  %8lu  %8lu",
      (DWORD)total_code, (DWORD)total_data, (DWORD)total_other,
      (DWORD)(total_code + total_data + total_other),(DWORD)(total_disc));

   SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)buffer);


		 /** Obtain basic memory parameters **/

   GlobalFreePtr(lpBuf2);


   END_LEVEL

   if(lpBuf1) GlobalFreePtr(lpBuf1);




		    /* Now, print the final summary */

   if(GetWinFlags() & WF_ENHANCED)
   {

      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1,(LPARAM)
	       "             ** ENHANCED MODE WINDOWS **");

      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1,(LPARAM)
	       "อออออออออออออออออออออออออออออออออออออออออออออออออออออ");


      wsprintf(buffer,
	       "Total Available Memory:             %9lu bytes",
	       dwFree);
      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)buffer);


      wsprintf(buffer,
	       "Largest Contiguous Block:           %9lu bytes",
	       dwLargest);
      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)buffer);


      // ESTIMATE SIZE OF 'FREE' VIRTUAL MEMORY:
      // i.e. TOTAL FREE - DISCARDABLE - ESTIMATED FREE LINEAR MEMORY
      // (does not account for overlapping pages)


      dwFreeVirt = dwFree - total_disc - mmi.dwFreePages * mmi.wPageSize;


      // IF 'dwFreeVirt' is larger than the virtual file, correct the
      // value to equal the size of the virtual file.  Usually it's close!

      if(dwFreeVirt > mmi.dwSwapFilePages * mmi.wPageSize)
      {
	 dwFreeVirt = mmi.dwSwapFilePages * mmi.wPageSize;
      }

      wsprintf(buffer,
	       "Estimated Unused Linear Memory:     %9lu bytes",
	       dwFree - total_disc - dwFreeVirt);
      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)buffer);


      wsprintf(buffer,
	       "Total Linear Memory:                %9lu bytes",
	       mmi.dwTotalPages * mmi.wPageSize);
      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)buffer);


      wsprintf(buffer,
	       "Estimated Unused Virtual Memory:    %9lu bytes",
	       dwFreeVirt);
      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)buffer);


      wsprintf(buffer,
	       "Total Virtual Memory:               %9lu bytes",
	       mmi.dwSwapFilePages * mmi.wPageSize);
      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)buffer);



      wsprintf(buffer,
	       "Grand Total of System Memory:       %9lu bytes",
	       mmi.dwTotalPages * mmi.wPageSize +
	       mmi.dwSwapFilePages * mmi.wPageSize);
      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)buffer);



      wsprintf(buffer,
	       "Total Non-Discardable Memory In Use:%9lu bytes",
	       mmi.dwTotalPages * mmi.wPageSize +
		  mmi.dwSwapFilePages * mmi.wPageSize - dwFree);
      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)buffer);


      wsprintf(buffer,
	       "Windows Applications (non-discard): %9lu bytes",
	       total_code + total_data + total_other - total_disc);
      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)buffer);


      wsprintf(buffer,
	       "Windows Applications (discardable): %9lu bytes",
	       total_disc);
      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)buffer);


      wsprintf(buffer,
	       "Non-Windows/System/Miscellaneous:   %9lu bytes",
	       mmi.dwTotalPages * mmi.wPageSize +
		  mmi.dwSwapFilePages * mmi.wPageSize - dwFree
		   - total_code - total_data - total_other + total_disc);
      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)buffer);


      wsprintf(buffer,
	       "Total number of tasks:  %u",
	       nTasks );
      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)buffer);

   }
   else
   {
      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1,(LPARAM)
	       "        ** STANDARD MODE WINDOWS **");

      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1,(LPARAM)
	       "อออออออออออออออออออออออออออออออออออออออออออ");


      wsprintf(buffer,
	       "Total Available Memory:    %9lu bytes",
	       dwFree);
      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)buffer);


      wsprintf(buffer,
	       "Largest Contiguous Block:  %9lu bytes",
	       dwLargest);
      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)buffer);


      wsprintf(buffer,
	       "Total number of tasks:  %u",
	       nTasks );
      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)buffer);

   }

   return;

}



int FAR _cdecl CMDMemSortProc(LPSTR lp1, LPSTR lp2)
{
struct CMDMEM_MODULE {                       /* size must be power of 2!! */
   char module[MAX_MODULE_NAME + 1];
   HMODULE hModule;
   DWORD total_code;
   DWORD total_data;
   DWORD total_other;
   DWORD total_disc;
   char junk[32 - MAX_MODULE_NAME - 1 - sizeof(HMODULE) - 3 * sizeof(DWORD)];

   } FAR *lpMM1, FAR *lpMM2;



   lpMM1 = (struct CMDMEM_MODULE FAR *)lp1;
   lpMM2 = (struct CMDMEM_MODULE FAR *)lp2;

   return(_fstrnicmp(lpMM1->module, lpMM2->module, sizeof(lpMM1->module)));

}




void FAR PASCAL CreateVectorList(HWND hChild)
{
int i;
WORD wSel, wSeg, wOff, wSel2, wIDTSel;
BOOL Is386;
DWORD dwOff, dwOff2;
static IDT_INFO IDT;
static CALL_GATE Gate;




   SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1,(LPARAM)
               " INT    DPMI PROT MODE   DPMI REAL MODE   SYSTEM-WIDE     IDT VECTOR");
   SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1,(LPARAM)
               "NUMBER  SELECTOR:OFFSET  SEGMENT:OFFSET   IDT SEL:OFF        TYPE");
   SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1,(LPARAM)
               "------  ---------------  --------------  -------------  ---------------");

   Is386 = (GetWinFlags() & WF_ENHANCED)!=0;



   // ALLOCATE SELECTOR AND ASSIGN IT THE ADDRESS OF THE 'IDT'

   wIDTSel = MyAllocSelector();

   /* get the IDT address and place into 'IDT' for *true* vector */

   _asm
   {
      mov bx, OFFSET IDT
      _asm _emit(0xf) _asm _emit(0x1) _asm _emit (0xf); /* SIDT FWORD PTR [BX] */
   }

   if(MyChangeSelector(wIDTSel, IDT.dwAddr, 0xffff))
   {
      MyFreeSelector(wIDTSel);
      return;
   }


   // MAIN LOOP!

   for(i=0; i<=255; i++)
   {
      MySetCursor(2);

      _asm
      {
	 mov ax, 0x204        ; get prot mode vector
	 mov bl, BYTE PTR i
	 int 0x31

	 mov wSel, cx
	 mov ax, WORD PTR Is386
	 cmp ax,0
	 jz no_32_bit


	 // 32-bit version

	 _asm _emit(0x66) _asm mov WORD PTR dwOff, dx
	 jmp offset_found


	 // 16-bit version

no_32_bit:

	 mov WORD PTR dwOff, dx
	 mov ax, 0
	 mov WORD PTR dwOff + 2, ax


offset_found:

	 mov ax, 0x200       ; get real mode vector
	 mov bl, BYTE PTR i
	 int 0x31

	 mov wSeg, cx
	 mov wOff, dx


	 mov ax, 0             /* zero out the 'Gate' structure */
	 mov Gate.wOff16, ax
	 mov Gate.wSel, ax
	 mov Gate.wFlags, ax
	 mov Gate.wOff32, ax


	 /* using the IDT's selector read the IDT interrupt gate */

	 push es
	 mov es, wIDTSel
	 mov bx, i
	 mov cl,3
	 shl bx,cl

	 mov ax, es:[bx]
	 mov WORD PTR Gate, ax
	 mov ax, es:[bx+2]
	 mov WORD PTR Gate+2, ax
	 mov ax, es:[bx+4]
	 mov WORD PTR Gate+4, ax
	 mov ax, es:[bx+6]
	 mov WORD PTR Gate+6, ax

      }




      wsprintf(buffer, " %02xH     %04x:%08lx    %04x:%04x      %04x:%08lx  ",
	       i, wSel, dwOff, wSeg, wOff,
               Gate.wSel, MAKELONG(Gate.wOff16, Gate.wOff32));

      switch((Gate.wFlags / 0x100) & 0x1f)  // the type of gate...
      {
         case 4:    // CALL GATE
         case 12:
            wsprintf(buffer + lstrlen(buffer), "%s DPL=%1d %c",
                     Gate.wFlags & 0x800?(LPSTR)"CALL32":(LPSTR)"CALL16",
                     (Gate.wFlags & 0x6000) / 0x2000,
                     Gate.wFlags & 0x8000?'P':' ');
            break;

         case 5:
         case 13:   // TASK GATE
            wsprintf(buffer + lstrlen(buffer), "%s DPL=%1d %c",
                     Gate.wFlags & 0x800?(LPSTR)"TASK32":(LPSTR)"TASK16",
                     (Gate.wFlags & 0x6000) / 0x2000,
                     Gate.wFlags & 0x8000?'P':' ');
            break;

         case 6:
         case 14:   // INT GATE
            wsprintf(buffer + lstrlen(buffer), "%s DPL=%1d %c",
                     Gate.wFlags & 0x800?(LPSTR)"INT32 ":(LPSTR)"INT16 ",
                     (Gate.wFlags & 0x6000) / 0x2000,
                     Gate.wFlags & 0x8000?'P':' ');
            break;

         case 7:
         case 15:   // TRAP GATE
            wsprintf(buffer + lstrlen(buffer), "%s DPL=%1d %c",
                     Gate.wFlags & 0x800?(LPSTR)"TRAP32":(LPSTR)"TRAP16",
                     (Gate.wFlags & 0x6000) / 0x2000,
                     Gate.wFlags & 0x8000?'P':' ');
            break;

         default:
            wsprintf(buffer + lstrlen(buffer), "Flags = %04xH",
                     Gate.wFlags);
      }


      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1,(LPARAM)buffer);
   }


   MyFreeSelector(wIDTSel);

}





void PASCAL CreateDescriptorInfoText(WORD wSelMin, WORD wSelMax,
                                     LPDESCRIPTOR lpD, LPSTR lpBuf)
{
char c1, c2, c3, c4, c5, c6, c7;
DWORD dwAddr, dwLimit;
BOOL GateFlag = FALSE;
BOOL ParmFlag = FALSE;



   // STEP 1:  Calculate the correct SEGMENT TYPE (description)

   c7 = (lpD->wDPL_and_Flags & 0x80)?' ':'!';  // the 'not present' indicator

   if((lpD->wDPL_and_Flags & 0x18)==0x18) /** CODE SELECTOR **/
   {
      c1 = 'C';
      c2 = (lpD->wDPL_and_Flags & 0x4)?'C':' ';
      c3 = (lpD->wDPL_and_Flags & 0x2)?'R':'X';
      c4 = (lpD->wDPL_and_Flags & 0x1)?'A':' ';
      c5 = ' ';
      c6 = ' ';
   }
   else if((lpD->wDPL_and_Flags & 0x18)==0x10) /** DATA SELECTOR **/
   {
      c1 = 'D';
      c2 = (lpD->wDPL_and_Flags & 0x4)?'E':' ';
      c3 = (lpD->wDPL_and_Flags & 0x2)?'W':'R';
      c4 = (lpD->wDPL_and_Flags & 0x1)?'A':' ';
      c5 = ' ';
      c6 = ' ';
   }
   else if((lpD->wDPL_and_Flags & 0x7)==1)
   {
      c1 = 'T';
      c2 = 'S';
      c3 = 'S';
      c4 = (lpD->wDPL_and_Flags & 0x8)?'3':'1';
      c5 = (lpD->wDPL_and_Flags & 0x8)?'2':'6';
      c6 = ' ';
   }
   else if((lpD->wDPL_and_Flags & 0xf)==2)  // 2, but not 10
   {
      c1 = 'L';
      c2 = 'D';
      c3 = 'T';
      c4 = ' ';
      c5 = ' ';
      c6 = ' ';
   }
   else if((lpD->wDPL_and_Flags & 0x7)==3)
   {
      c1 = '*';
      c2 = 'T';
      c3 = 'S';
      c4 = 'S';
      c5 = (lpD->wDPL_and_Flags & 0x8)?'3':'1';
      c6 = (lpD->wDPL_and_Flags & 0x8)?'2':'6';
   }
   else if((lpD->wDPL_and_Flags & 0x7)==4)
   {
      c1 = 'C';
      c2 = 'A';
      c3 = 'L';
      c4 = 'L';
      c5 = (lpD->wDPL_and_Flags & 0x8)?'3':'1';
      c6 = (lpD->wDPL_and_Flags & 0x8)?'2':'6';

      GateFlag = TRUE;
      ParmFlag = TRUE;      // displays 'WORD COUNT' also!
   }
   else if((lpD->wDPL_and_Flags & 0xf)==5)  // 5, but not 13
   {
      c1 = 'T';
      c2 = 'A';
      c3 = 'S';
      c4 = 'K';
      c5 = ' ';
      c6 = ' ';

      GateFlag = TRUE;
   }
   else if((lpD->wDPL_and_Flags & 0x7)==6)
   {
      c1 = 'I';
      c2 = 'N';
      c3 = 'T';
      c4 = (lpD->wDPL_and_Flags & 0x8)?'3':'1';
      c5 = (lpD->wDPL_and_Flags & 0x8)?'2':'6';
      c6 = ' ';

      GateFlag = TRUE;
   }
   else if((lpD->wDPL_and_Flags & 0x7)==7)
   {
      c1 = 'T';
      c2 = 'R';
      c3 = 'A';
      c4 = 'P';
      c5 = (lpD->wDPL_and_Flags & 0x8)?'3':'1';
      c6 = (lpD->wDPL_and_Flags & 0x8)?'2':'6';

      GateFlag = TRUE;
   }
   else   // anything else not normally covered...
   {
      c1 = 'S';
      c2 = (lpD->wDPL_and_Flags & 0x8)?'1':'0';
      c3 = (lpD->wDPL_and_Flags & 0x4)?'1':'0';
      c4 = (lpD->wDPL_and_Flags & 0x2)?'1':'0';
      c5 = (lpD->wDPL_and_Flags & 0x1)?'1':'0';
      c6 = ' ';

      if((lpD->wDPL_and_Flags & 0xf)==0) c7 = ' ';
   }


   // STEP 2:  Calculate the correct ADDRESS and SEGMENT LIMIT

   if(!GateFlag)
   {
      // calculate the address and limit for a 'normal' selector

      dwAddr = (DWORD)(lpD->wSegBaseLo) + (65536L * lpD->wSegBaseMid)
               + (65536L * 256L * lpD->wSegBaseHi);
      dwLimit = (DWORD)(lpD->wSegLimLo)
               + (65536L * (lpD->wSegLimHi_and_Flags & 0x0f));

      if(lpD->wSegLimHi_and_Flags & 0x80)
      {
         dwLimit = dwLimit * 0x1000L + 0xfff; /* page gran. */
      }
   }
   else
   {
      // calculate the SELECTOR and OFFSET for a 'gate' selector

      dwAddr = ((LPCALL_GATE)lpD)->wSel;
      dwLimit = ((LPCALL_GATE)lpD)->wOff16 +
                 0x10000L * ((LPCALL_GATE)lpD)->wOff32;

   }


   // STEP 3:  Depending on type, generate 1 line of text describing
   //          the selector's contents, as best as we know how!

   if(!GateFlag)
   {
      wsprintf(lpBuf,
               "%4x to %4x  %8lx  %8lx  %1d/%c%c%c%c%c%c%c  %2d %s",
               wSelMin, wSelMax,
               dwAddr,
               dwLimit,
               (WORD)((lpD->wDPL_and_Flags >> 5) & 0x3),
               c1, c2, c3, c4, c5, c6, c7,
               (lpD->wSegLimHi_and_Flags & 0x40)?32:16,
               (lpD->wSegLimHi_and_Flags & 0x80)?(LPSTR)"Page"
                                                  :(LPSTR)"Byte");
   }
   else if(!ParmFlag)
   {
      wsprintf(lpBuf,
               "%4x to %4x  %8lx  %8lx  %1d/%c%c%c%c%c%c%c",
               wSelMin, wSelMax,
               dwAddr,
               dwLimit,
               (WORD)((lpD->wDPL_and_Flags >> 5) & 0x3),
               c1, c2, c3, c4, c5, c6, c7);
   }
   else
   {
      wsprintf(lpBuf,
               "%4x to %4x  %8lx  %8lx  %1d/%c%c%c%c%c%c%c  %d WORD%c",
               wSelMin, wSelMax,
               dwAddr,
               dwLimit,
               (WORD)((lpD->wDPL_and_Flags >> 5) & 0x3),
               c1, c2, c3, c4, c5, c6, c7,
               (((LPCALL_GATE)lpD)->wFlags & 0x1f),
               (((LPCALL_GATE)lpD)->wFlags & 0x1f)==1?' ':'S');
   }

}




void FAR PASCAL CreateGDTList(HWND hChild)
{
WORD wMySel, i, j;
LPSTR lp1;
DWORD dwAddr, dwLimit;
LPDESCRIPTOR lpD;
static GDT_INFO GDT;



   wMySel = MyAllocSelector();

   if(!wMySel) return;                  /* '0' selector if error occurs */


			   /***-----------***/
			   /*** MAIN LOOP ***/
			   /***-----------***/


   GDT.wLimit = 0;
   GDT.dwAddr = 0;

   _asm _emit(0xf) _asm add WORD PTR GDT, ax  /* SGDT GDT */

   dwAddr = GDT.dwAddr;



   if(MyChangeSelector(wMySel, dwAddr, 0xffff))
   {
      MyFreeSelector(wMySel);
      return;
   }

   lpD = (LPDESCRIPTOR)MAKELP(wMySel, 0);


   // NOTE:  If there are more than 256 entries in table, do the
   //        same thing here that we do for the LDT to eliminate
   //        'unused' GDT entries.



   SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)
        " Selectors     Linear   Segment   DPL/Flag   16/32 bit");
   SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)
        "(Min to Max)  Address    Limit    Sel Type   Granularity");
   SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)
        "------------  --------  --------  ---------  -----------");


   for(i=0, j=0; j<GDT.wLimit && (j!=0 || i==0); i++, j+=sizeof(*lpD))
   {
      MySetCursor(2);

      if(GDT.wLimit >= 0x800 &&               // Only when >=256 entries
         !lpD[i].wDPL_and_Flags)              // IGNORE '0' FLAGS
      {                                       // This represents the majority
         continue;                            // of the "unused" selectors.
      }


      CreateDescriptorInfoText(j, j + 3, lpD + i, buffer);

      lp1 = buffer + lstrlen(buffer);
      while(lp1 > (LPSTR)buffer && *(lp1 - 1)<= ' ') *(--lp1) = 0;

      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1,(LPARAM)buffer);
   }



   MyFreeSelector(wMySel);

}



void FAR PASCAL CreateLDTList(HWND hChild)
{
WORD wMySel, wSegLimit, i, j, c1, c2, c3, c4, c5;
LPSTR lp1;
DWORD dwAddr, dwLimit, dwLDTAddr, dwLDTLimit;
LPDESCRIPTOR lpD;
static GDT_INFO GDT;
   



   wMySel = MyAllocSelector();

   if(!wMySel) return;                  /* '0' selector if error occurs */


   // GET the GDT ADDRESS first, then look up LDT selector in it.

   GDT.wLimit = 0;
   GDT.dwAddr = 0;

   _asm
   {
      _asm nop
      _asm nop

      _asm _emit(0xf) _asm add WORD PTR GDT, ax  /* SGDT GDT */

      _asm nop
      _asm nop
   }

   dwAddr = GDT.dwAddr;



   if(MyChangeSelector(wMySel, dwAddr, 0xffff))
   {
      MyFreeSelector(wMySel);
      return;
   }




   // PLACE THE 'RPL=0' SELECTOR FOR THE 'LDT' INTO THE index for 'lpD'
   //  (which will then point to the 'GDT' selector entry for the 'LDT')

   _asm
   {
      nop
      nop

      _asm _emit(0xf) _asm _emit(0x0) _asm _emit(0xc3)     /* sldt bx */
				       /* get the REAL 'LDT' selector */

      nop
      nop

      mov i, bx                    // store selector here

      nop
      nop
   }

   i /= sizeof(*lpD);  // it's now an INDEX, not an OFFSET!

   if(!i)
   {
      MyFreeSelector(wMySel);

      return;
   }



	  /* next step:  build a NEW address/limit based on  */
	  /*             the values found in the GDT for LDT */
	  /*             selector entry.                     */

   lpD = (LPDESCRIPTOR)MAKELP(wMySel, 0); // at this point, the GDT

   dwLDTAddr = (DWORD)(lpD[i].wSegBaseLo) + (65536L * lpD[i].wSegBaseMid)
	       + (65536L * 256L * lpD[i].wSegBaseHi);
   dwLDTLimit = (DWORD)(lpD[i].wSegLimLo)
	       + (65536L * (lpD[i].wSegLimHi_and_Flags & 0x0f));


   if(lpD[i].wSegLimHi_and_Flags & 0x80)
   {
      dwLDTLimit = dwLDTLimit * 0x1000L + 0xfff; /* page gran. */
   }

   wSegLimit = (WORD)dwLDTLimit;

   if(!wSegLimit)  // ZERO segment size??
   {
      MyFreeSelector(wMySel);

      return;
   }

	  /* one more time - re-assign the offset for the   */
	  /* allocated LDT selector to point to the address */
	  /* pointed to by the *actual* LDT selector...     */
	  /* note that it MUST alias correctly to work.     */

   if(MyChangeSelector(wMySel, dwLDTAddr, 0xffff))
   {
      MyFreeSelector(wMySel);

      return;
   }


   lpD = (LPDESCRIPTOR)MAKELP(wMySel, 0);




			   /***-----------***/
			   /*** MAIN LOOP ***/
			   /***-----------***/


   SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)
        " Selectors     Linear   Segment   DPL/Flag   16/32 bit");
   SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)
        "(Min to Max)  Address    Limit    Sel Type   Granularity");
   SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)
        "------------  --------  --------  ---------  -----------");


   for(i=0, j=0; j<wSegLimit && (j!=0 || i==0); i++, j+=sizeof(*lpD))
   {
      MySetCursor(2);

      if(!lpD[i].wDPL_and_Flags) continue;  // FOR LDT, IGNORE '0' FLAGS
					    // This represents the majority
					    // of the "unused" selectors.


      CreateDescriptorInfoText(j + 4, j + 7, lpD + i, buffer);

      lp1 = buffer + lstrlen(buffer);
      while(lp1 > (LPSTR)buffer && *(lp1 - 1)<= ' ') *(--lp1) = 0;

      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1,(LPARAM)buffer);

   }

   MyFreeSelector(wMySel);

}




int __far __cdecl CreateMemoryPageMapSortProc(LPSTR hp1, LPSTR hp2)
{
LPVMLIST lp1, lp2;

   lp1 = (LPVMLIST)hp1;
   lp2 = (LPVMLIST)hp2;

   if(lp1->cb.CB_High_Linear > lp2->cb.CB_High_Linear)       return(1);
   else if(lp1->cb.CB_High_Linear == lp2->cb.CB_High_Linear) return(0);
   else                                                      return(-1);

}

int __far __cdecl CreateVMListSortProc(LPSTR hp1, LPSTR hp2)
{
LPVMLIST lp1, lp2;

   lp1 = (LPVMLIST)hp1;
   lp2 = (LPVMLIST)hp2;

   if(lp1->cb.CB_VMID > lp2->cb.CB_VMID)       return(1);
   else if(lp1->cb.CB_VMID == lp2->cb.CB_VMID) return(0);
   else                                        return(-1);

}



static WORD __huge * FAR PASCAL GetPageMap(DWORD FAR *lpdwTotalSysPage,
                                           DWORD dwMinPageAddr,
                                           DWORD FAR *lpdwSysPageCount)
{
WORD __huge *lpRval;
DWORD FAR *lpDir, FAR *lpEntry, dwPageInfo, dwPageDirLinAddr;
WORD w1, w2, w3, wIter;
//DWORD dwTemp[16]={0}; // TEMPORARY


#define PAGE_ENTRY_MASK (0xfffff000L | 0x198 | P_ACC | P_DIRTY)
            // This consists of all bits except user, write access,
            // and 'SYSTEM' bits (0xe00), as well as 'Present' bit.


   *lpdwTotalSysPage = 0L;     // initial value for SYSTEM pages
   *lpdwSysPageCount = 0L;     // initial value for SYSTEM pages


   lpDir = GlobalAllocPtr(GMEM_MOVEABLE, 0x800 * sizeof(DWORD));
   if(!lpDir) return(NULL);

   lpEntry = lpDir + 0x400;   // 1024 entries in DIR; 1024 entries in table

   lpRval = GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                           0x40000L);  // 256k for this!
   if(!lpRval)
   {
      GlobalFreePtr(lpDir);
      return(NULL);
   }


   wIter = 0;

   dwPageDirLinAddr = MyGetPageDirectory(lpDir);  // obtain page table directory!


   // TEMPORARY

   wsprintf(buffer, "Page Directory Linear Address:  %lxH\r\n",
            dwPageDirLinAddr);

   OutputDebugString(buffer);


   for(w1=0; w1<0x400; w1++)      // go through each page table section
   {
      MySetCursor(2);

      if(lpDir[w1] & 1)           // is the 'Present' bit set?
      {
//         // TEMPORARY
//
//         w3 = (WORD)(lpDir[w1] >> 9) & 3;  // PAGE DIRECTORY TYPE (?)
//
//         wsprintf(buffer, "PAGE DIR: %08lx - type %d\r\n",
//                  (DWORD)(w1 * 0x400000L), (WORD)w3);
//         OutputDebugString(buffer);
//
//

         MyGetPageTable(lpDir[w1], lpEntry);

         for(w2=0; w2<0x400; w2++)
         {
            if(!((++wIter) & 0x1f)) MySetCursor(2);

            w3 = (WORD)(lpEntry[w2] >> 9) & 3;   // PAGE TYPE!!

            if(w3 == PG_SYS &&
               ((lpEntry[w2] & 1) || (lpEntry[w2] & PAGE_ENTRY_MASK)))
            {
               // This only counts 'PRESENT' and 'SWAP FILE' pages!

               (*lpdwTotalSysPage)++;    // TOTAL # of pages marked 'PG_SYS'

               if(((w1 * 0x400L + w2) * 0x1000) >= dwMinPageAddr)
               {
                  (*lpdwSysPageCount)++; // 'Pageable' PG_SYS pages...
                                         // (based on 'dwMinPageAddr')
               }
            }


            if(lpEntry[w2] & P_PRES)   // is the page 'Present'??
            {
//               // TEMPORARY
//
//               // for now, keep track of how many of each PAGE TYPE
//               // there are, and place on DEBUG monitor...
//
//               dwTemp[w3 + 8]++;
//


               // '11' means PAGE IS VALID AND 'PRESENT'

               lpRval[w1 * 0x80L + (w2 >> 3)] |= pBits[2 * (w2 & 7)] |
                                                 pBits[2 * (w2 & 7) + 1];
            }
            else     // Page is *NOT* present according to PAGE TABLE!
            {
//               // TEMPORARY
//
//               // for now, keep track of how many of each PAGE TYPE
//               // there are, and place on DEBUG monitor...
//
//               dwTemp[w3]++;
//


               // First, ensure BOTH bits are clear...

               lpRval[w1 * 0x80L + (w2 >> 3)] &= ~(pBits[2 * (w2 & 7)] |
                                                  pBits[2 * (w2 & 7) + 1]);

//               // Next, see if this page is VALID by checking the 'type' bits
//
//               if(w3 == PG_VM || w3 == PG_HOOKED || w3 == PG_SYS ||
//                  w3 == PG_INSTANCE || w3 == PG_PRIVATE || w3 == PG_RELOCK)
//
               if((lpEntry[w2] & PAGE_ENTRY_MASK))   // page 'valid' (?)
               {

                  // '10' means PAGE VALID, BUT NOT PRESENT.  I assume that
                  // these pages are in the SWAP FILE.  There is considerable
                  // evidence to indicate that NON-ZERO addresses for pages
                  // which are 'NOT PRESENT' indicates SWAP FILE PAGES.

                  lpRval[w1 * 0x80L + (w2 >> 3)] |= pBits[2 * (w2 & 7) + 1];
               }
               else
               {
                  // '01' means PAGE 'NOT VALID', BUT DIRECTORY WAS PRESENT
                  // These pages have a physical address of ZERO

                  lpRval[w1 * 0x80L + (w2 >> 3)] |= pBits[2 * (w2 & 7)];
               }
            }
         }
      }
      else
      {
         for(w2=0; w2<0x80; w2++)
         {
            if(!((++wIter) & 0x1f)) MySetCursor(2);


            // '00' means PAGE DIRECTORY WAS NOT PRESENT!

            lpRval[w1 * 0x80 + w2] = 0;  // none of the pages are 'present'
         }
      }


   }

//   // TEMPORARY
//
//   for(w1=0; w1<8; w1++)
//   {
//      wsprintf(buffer, "  Total Page Type %d:  PRESENT=%-10ld  NOT PRESENT=%-10ld\r\n",
//               (WORD)w1, (DWORD)dwTemp[w1 + 8], (DWORD)dwTemp[w1]);
//      OutputDebugString(buffer);
//   }


   GlobalFreePtr(lpDir);

   MySetCursor(2);

   return(lpRval);

}


void FAR PASCAL CreateMemoryPageMap(HWND hChild)
{
DEMANDINFO __di;
LPVMLIST lpVMList, lpVML;
WORD wVMCount, wMySel, w1, w2, w3, wIter;
DWORD dwCB, dwCBSys, dwPageAddr, dwPageCount, dwAllocCount, dwSysPageCount,
      dwTotalSysPage, dwRawSwapCount, dwSwapCount, lpdwInfo[2], dw1, dw2;
LPCONTROL_BLOCK lpCB;
WORD __huge *lpPageMap;
static char NEAR *pPageType[4]={"ADDRESS NOT VALID",
                                "RESERVED {NOT PRESENT}",
                                "PAGE FILE",
                                "PHYSICAL RAM" };



   MyGetDemandPageInfo(&__di);   // find out how many pages there are!!

   if(!(lpPageMap = GetPageMap(&dwTotalSysPage, __di.DILinear_Base_Addr,
                               &dwSysPageCount)))
   {
      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)
                  "?Not enough memory to display PAGE info!");
      return;
   }

   // GET 'VM' list and sort in order of HIGH LINEAR ADDRESS!

   lpVMList = GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                             256L * sizeof(*lpVMList));

   if(!lpVMList)
   {
      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)
                  "?Not enough memory to display VM info!");

      if(lpPageMap) GlobalFreePtr(lpPageMap);
      return;
   }

   MySetCursor(2);  // rotate hourglass!


   lpVML = lpVMList;
   wVMCount = 0;
   dwSwapCount = 0;
   dwRawSwapCount = 0;

   dwCBSys = MyGetSystemVM();

   if(dwCBSys)
   {
      wMySel = MyAllocSelector();

      lpCB = (LPCONTROL_BLOCK)MAKELP(wMySel, 0);

      dwCB = dwCBSys;

      do
      {
	 MyChangeSelector(wMySel, dwCB, 0xffff);

         _hmemcpy(&(lpVML->cb), lpCB, sizeof(*lpCB));

         lpVML->dwCB = dwCB;

         lpVML->hwndWinOldAp = 
            MyGetVMInfo(dwCB, lpVML->dwInfo);  // get more VM info

         wVMCount++;
         lpVML++;        // update counter and pointer

         dwCB = MyGetNextVM(dwCB);

      } while(dwCB && dwCBSys!=dwCB);


      MyFreeSelector(wMySel);

      wMySel = NULL;

      if(!dwCB)
      {
	 SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)
                     "   ** ERROR - 'MyGetNextVM()' returned NULL **");

         GlobalFreePtr(lpVMList);
         lpVMList = NULL;
      }
      else
      {
         _lqsort(lpVMList, wVMCount, sizeof(*lpVMList),
                 CreateMemoryPageMapSortProc);


         // Here I display memory information using 'lpVMList' as
         // a guide to determine WHICH 'VM' owns a memory block...

         SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)
                     "         ** MEMORY 'PAGING' SYSTEM - ADDRESS MAP **");
         SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)
                     "   Linear Address Range  VM ID   Description (Page Type)");
         SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)
                     "   --------------------  ------  ------------------------");

         wIter = 0;

         for(w3=0, dw1=0, dw2=0; w3<wVMCount; w3++)
         {
            for(w1 = 0xffff;  // previous type - this forces output!!
                dw1<=0xfffffL && dw2 < lpVMList[w3].cb.CB_High_Linear;
                dw1++, dw2+=0x1000)
            {
               w2 = (lpPageMap[(dw1 >> 3)] >> (2 * (dw1 & 7))) & 3;

               if(w2==2)  // a 'swapped out' page in swap file
               {
                  dwRawSwapCount++;

                  if(dw2 >= __di.DILinear_Base_Addr) dwSwapCount++;
               }

               if(w1 != w2)
               {
                  if(w1 != 0xffff)
                  {
                     wsprintf(buffer, "   %08lx to %08lx  Global  %s",
                              (DWORD)dwPageAddr, (DWORD)(dw2 - 1),
                              (LPSTR)pPageType[w1]);
                     SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)
                                 (LPSTR)buffer);
                  }

                  dwPageAddr = dw2;
                  w1 = w2;
               }

               if(!((++wIter) & 0x1f)) MySetCursor(2);  // rotate hourglass!
            }

            wsprintf(buffer, "   %08lx to %08lx  Global  %s",
                     (DWORD)dwPageAddr, (DWORD)(dw2 - 1),
                     (LPSTR)pPageType[w1]);

            SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)
                        (LPSTR)buffer);


            for(w1 = 0xffff;
                dw1<=0xfffffL && dw2 < (lpVMList[w3].cb.CB_High_Linear +
                                        0x10f000L);
//                                        lpVMList[w3].dwInfo[10] * 0x1000);
                dw1++, dw2+=0x1000)
            {
               w2 = (lpPageMap[(dw1 >> 3)] >> (2 * (dw1 & 7))) & 3;

               if(w2==2)  // a 'swapped out' page in swap file
               {
                  dwRawSwapCount++;

                  if(dw2 >= __di.DILinear_Base_Addr) dwSwapCount++;
               }

               if(w1 != w2)
               {
                  if(w1 != 0xffff)
                  {
                     wsprintf(buffer, "   %08lx to %08lx   %2d     %s",
                              (DWORD)dwPageAddr, (DWORD)(dw2 - 1),
                              (WORD)lpVMList[w3].cb.CB_VMID,
                              (LPSTR)pPageType[w1]);
                     SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)
                                 (LPSTR)buffer);
                  }

                  dwPageAddr = dw2;
                  w1 = w2;
               }

               if(!((++wIter) & 0x1f)) MySetCursor(2);  // rotate hourglass!
            }

            wsprintf(buffer, "   %08lx to %08lx   %2d     %s",
                     (DWORD)dwPageAddr, (DWORD)(dw2 - 1),
                     (WORD)lpVMList[w3].cb.CB_VMID,
                     (LPSTR)pPageType[w1]);
            SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)
                        (LPSTR)buffer);
         }


         for(w1 = 0xffff; dw1<=0xfffffL; dw1++, dw2+=0x1000)
         {
            w2 = (lpPageMap[(dw1 >> 3)] >> (2 * (dw1 & 7))) & 3;

            if(w2==2)  // a 'swapped out' page in swap file
            {
               dwRawSwapCount++;

               if(dw2 >= __di.DILinear_Base_Addr) dwSwapCount++;
            }

            if(w1 != w2)
            {
               if(w1 != 0xffff)
               {
                  wsprintf(buffer, "   %08lx to %08lx  Global  %s",
                           (DWORD)dwPageAddr, (DWORD)(dw2 - 1),
                           (LPSTR)pPageType[w1]);
                  SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)
                              (LPSTR)buffer);
               }

               dwPageAddr = dw2;
               w1 = w2;
            }

            if(!((++wIter) & 0x1f)) MySetCursor(2);  // rotate hourglass!
         }

         wsprintf(buffer, "   %08lx to %08lx  Global  %s",
                  (DWORD)dwPageAddr, (DWORD)0xffffffffL,
                  (LPSTR)pPageType[w1]);

         SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)
                     (LPSTR)buffer);

      }
   }
   else
   {
      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)
                  "?Error - Unable to get SYSTEM VM info!");

      GlobalFreePtr(lpVMList);
      lpVMList = NULL;
   }


   MySetCursor(2);  // rotate hourglass!



   // SUMMARY SECTION (at the end!)

   SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1,(LPARAM)"");

   SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)
                    "        ** MEMORY PAGING SYSTEM INFO (SUMMARY) **");

   SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1,(LPARAM)
                    "อออออออออออออออออออออออออออออออออออออออออออออออออออออออออ");

   SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1,(LPARAM)"");


   wsprintf(buffer, "      Total # of Linear Pages:            %ld",
            __di.DILin_Total_Count);
   SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM) (LPSTR)buffer);


   wsprintf(buffer, "      Count of Physical Pages:            %ld",
            __di.DIPhys_Count);
   SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM) (LPSTR)buffer);


   wsprintf(buffer, "      Count of Free Physical Pages:       %ld",
            __di.DIFree_Count);
   SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM) (LPSTR)buffer);


   wsprintf(buffer, "      Count of Unlocked Physical Pages:   %ld",
            __di.DIFree_Count);
   SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM) (LPSTR)buffer);


   wsprintf(buffer, "      Total # of Free Linear Pages:       %ld",
            __di.DILin_Total_Free);
   SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM) (LPSTR)buffer);


   wsprintf(buffer, "      Total # of Linear Pages in Use:     %ld",
            __di.DILin_Total_Count - __di.DILin_Total_Free);
   SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM) (LPSTR)buffer);


   wsprintf(buffer, "      Total # of Physical Pages in Use:   %ld",
            __di.DIPhys_Count - __di.DIFree_Count);
   SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM) (LPSTR)buffer);


   wsprintf(buffer, "      Base Address for Pageable Memory:   %08lxH",
            __di.DILinear_Base_Addr);
   SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM) (LPSTR)buffer);


   SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1,(LPARAM)"");

   wsprintf(buffer, "      Raw Total of Pages Marked 'SYSTEM': %ld",
            dwTotalSysPage);   // # of 'PG_SYS' pages!
   SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM) (LPSTR)buffer);

   wsprintf(buffer, "      Total # 'Pageable' SYSTEM Pages:    %ld",
            dwSysPageCount);   // # of 'PG_SYS' pages above pageable base addr
   SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM) (LPSTR)buffer);

   wsprintf(buffer, "      Raw Total of SWAP FILE Pages:       %ld",
            dwRawSwapCount); // # of 'SWAP FILE' pages!
   SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM) (LPSTR)buffer);

   wsprintf(buffer, "      Estimated SWAP FILE Pages In Use:   %ld",
            dwSwapCount);      // # of 'SWAP FILE' pages!
   SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM) (LPSTR)buffer);



   MySetCursor(2);

   if(lpVMList)
   {
      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1,(LPARAM)"");
      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1,(LPARAM)
                    "               * Virtual Machine Summary *");
      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1,(LPARAM)
                    "---------------------------------------------------------");

      _lqsort(lpVMList, wVMCount, sizeof(*lpVMList),
              CreateVMListSortProc);

      dwPageCount = 0;
      dwAllocCount = 0;

      for(w1=0; w1<wVMCount; w1++)
      {
         if(!((++wIter) & 0x1f)) MySetCursor(2);

         wsprintf(buffer, "      VM #%02d - Total # Allocated Pages:   %ld",
                  (WORD)lpVMList[w1].cb.CB_VMID,
                  (DWORD)lpVMList[w1].dwInfo[10]);
         SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM) (LPSTR)buffer);

         dwAllocCount += lpVMList[w1].dwInfo[10];

         wsprintf(buffer, "               # of 'mapped in' pages:    %ld",
                  lpVMList[w1].dwInfo[10] - lpVMList[w1].dwInfo[11]);
         SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM) (LPSTR)buffer);

         dwPageCount += lpVMList[w1].dwInfo[11];
      }

      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1,(LPARAM)"");

      wsprintf(buffer, "      Overall VM Total # Allocated Pages: %ld",
               dwAllocCount);
      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM) (LPSTR)buffer);

      wsprintf(buffer, "      Overall VM Total Pages 'mapped in': %ld",
               dwAllocCount - dwPageCount);
      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM) (LPSTR)buffer);

      wsprintf(buffer, "      Estimated Total # of SYSTEM Pages:  %ld",
               (__di.DILin_Total_Count - __di.DILin_Total_Free)
               - (dwAllocCount - dwPageCount));
      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM) (LPSTR)buffer);

   }



   MySetCursor(2);


   // CLEANUP SECTION

   if(lpVMList) GlobalFreePtr(lpVMList);
   if(lpPageMap) GlobalFreePtr(lpPageMap);

}





void FAR PASCAL CreateVMList(HWND hChild)
{
LPVMLIST lpVMList, lpVML;
WORD wVMCount, wMySel, w1;
DWORD dwCB, dwCBSys;
LPCONTROL_BLOCK lpCB;
char tbuf[16];  // use this for FLAGS!


   lpVMList = GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                             256L * sizeof(*lpVMList));

   if(!lpVMList)
   {
      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)
                  "?Not enough memory to display VM info!");

      return;
   }

   lpVML = lpVMList;
   wVMCount = 0;

   dwCBSys = MyGetSystemVM();

   if(dwCBSys)
   {
      wMySel = MyAllocSelector();

      lpCB = (LPCONTROL_BLOCK)MAKELP(wMySel, 0);

      dwCB = dwCBSys;

      do
      {
         MySetCursor(2);

	 MyChangeSelector(wMySel, dwCB, 0xffff);

         _hmemcpy(&(lpVML->cb), lpCB, sizeof(*lpCB));

         lpVML->dwCB = dwCB;

         lpVML->hwndWinOldAp = 
            MyGetVMInfo(dwCB, lpVML->dwInfo);  // get more VM info

         wVMCount++;
         lpVML++;        // update counter and pointer

         dwCB = MyGetNextVM(dwCB);

      } while(dwCB && dwCBSys!=dwCB);


      if(!dwCB)
      {
	 SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)
		     "   ** ERROR - 'MyGetNextVM()' returned NULL **");
      }
      else
      {
         _lqsort(lpVMList, wVMCount, sizeof(*lpVMList),
                 CreateVMListSortProc);


         MySetCursor(2);

         SendMessage(hChild, LB_INSERTSTRING, (WPARAM)0, (LPARAM)(LPSTR)
            "VM # Ctrl Blk High Lin  Client  EMS Range XMS Range H  FG(BG)  CPU  Status &");

         SendMessage(hChild, LB_INSERTSTRING, (WPARAM)1, (LPARAM)(LPSTR)
            "(ID) Address  Address  Pointer  Min / Max Min / Max M Priority Pct   Flags");

         SendMessage(hChild, LB_INSERTSTRING, (WPARAM)2, (LPARAM)(LPSTR)
            "---- -------- -------- -------- --------- --------- - -------- --- ---------");


         for(w1=0, lpVML = lpVMList; w1<wVMCount; w1++, lpVML++)
         {
            MySetCursor(2);

            // Determine which FLAGS I use!

            *tbuf = 0;

            if(lpVML->cb.CB_VM_Status & VMStat_Exclusive)
               lstrcat(tbuf, "X");    // Exclusive Foreground!

            if(lpVML->cb.CB_VM_Status & VMStat_Background)
               lstrcat(tbuf, "B");    // Background operation

            if(lpVML->cb.CB_VM_Status & VMStat_High_Pri_Back)
               lstrcat(tbuf, "H");    // High priority background!

            if(lpVML->cb.CB_VM_Status & VMStat_Idle)
               lstrcat(tbuf, "I");    // 'Idle' flag set

            if(lpVML->cb.CB_VM_Status & (VMStat_PM_Exec | VMStat_PM_App))
               lstrcat(tbuf, "P");    // PROTECTED MODE APPLICATION!

//SGVMI_Windowed    equ 00000000000000000000000000000100B ; Is Windowed
//SGVMI_HasHotKey   equ 00000000000000000100000000000000b ; Has a shortcut key
//SGVMI_XMS_Lock    equ 00000000000000010000000000000000b ; XMS Hands Locked
//SGVMI_EMS_Lock    equ 00000000000000001000000000000000b ; EMS Hands Locked
//SGVMI_V86_Lock    equ 00000000000001000000000000000000b ; V86 Memory Locked
//SGVMI_ClsExit     equ 01000000000000000000000000000000b ; Close on Exit Enab

            if(lpVML->dwInfo[0] & 4)           lstrcat(tbuf, "W"); // WINDOWED
            else                               lstrcat(tbuf, "F"); // FULL-SCREEN

            if(lpVML->dwInfo[0] & 0x4000)      lstrcat(tbuf, "K"); // HOT KEY

            if(lpVML->dwInfo[0] & 0x58000L)    lstrcat(tbuf, "L"); // LOCKED

            if(lpVML->dwInfo[0] & 0x40000000L) lstrcat(tbuf, "C"); // close on exit
            else                               lstrcat(tbuf, "N"); // NO-CLOSE on exit


            wsprintf(buffer, "%04x %08lx %08lx %08lx %04x/%04x "
                             "%04x/%04x %c %3d(%3d)%3d%% %s",
                     (WORD)lpVML->cb.CB_VMID,
                     (DWORD)lpVML->dwCB,
                     (DWORD)lpVML->cb.CB_High_Linear,
                     (DWORD)lpVML->cb.CB_Client_Pointer,
                     (WORD)lpVML->dwInfo[2], (WORD)lpVML->dwInfo[3],
                     (WORD)lpVML->dwInfo[4], (WORD)lpVML->dwInfo[5],
                     (char)(lpVML->dwInfo[6]!=0?'Y':'N'),
                     HIWORD(lpVML->dwInfo[8]), LOWORD(lpVML->dwInfo[8]),
                     (WORD)lpVML->dwInfo[9],
                     (LPSTR)tbuf);

            SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)
                        (LPSTR)buffer);

            // 2nd line contains ADDITIONAL info....

            wsprintf(buffer, "    CPU TIME:%4d:%02d:%02d.%03d   HWND:%04XH   Allocated Pages: %lXH(%lXH)",
                     (WORD)(lpVML->dwInfo[12] / 3600000L),
                     (WORD)((lpVML->dwInfo[12] / 60000L) % 60),
                     (WORD)((lpVML->dwInfo[12] / 1000L) % 60),
                     (WORD)(lpVML->dwInfo[12] % 1000),
                     lpVML->hwndWinOldAp,
                     lpVML->dwInfo[10],
                     lpVML->dwInfo[11]);

            SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)
                        (LPSTR)buffer);


            // FINAL LINE:  application name (if possible to get it)

            lstrcpy(buffer, "    Application Name: \"");
            GetAppNameFromVMHandle(lpVML->dwCB, buffer + lstrlen(buffer));

            lstrcat(buffer, "\"");

            SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)
                        (LPSTR)buffer);
         }
      }

      MyFreeSelector(wMySel);

      wMySel = NULL;
   }
   else
   {
      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)
                  "?Error - Unable to get SYSTEM VM info!");
   }

   GlobalFreePtr(lpVMList);

}


void FAR PASCAL CreateVXDList(HWND hChild)
{
WORD w1, wMySel, wVersion;
DWORD dwVMM;
DWORD dwList[128];  // 512 bytes ought to be enough...
LPDDB lpDDB;
char tbuf[128];


   wVersion = LOWORD(GetVersion());
   wVersion = ((wVersion & 0xff)<<8) | ((wVersion >> 8) & 0xff);

   if(wVersion < 0x350) // Windows version 3.8
   {
      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)
                  "?Error - Not available on this version of Windows");
      return;
   }


   // TESTING - for NOW


   MyGetVXDList(dwList);

   wsprintf(tbuf, "ADDRESS FOR 'DeviceInfo' structure: %08lx",
            dwList[0]);
   SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)tbuf);


   dwVMM = MyGetVMMDDB();

   wsprintf(tbuf, "ADDRESS FOR 'VMM DDB' structure: %08lx", dwVMM);
   SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)tbuf);



   SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)"");
   SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)
               "DDB ADDRESS  DEVICE NAME  DEV ID:VER  ");
   SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)
               "___________  ___________  __________  ");

   wMySel = MyAllocSelector();

   if(!wMySel) return;  // error!

   lpDDB = (LPCONTROL_BLOCK)MAKELP(wMySel, 0);

   for(w1=1; dwList[w1]; w1++)
   {
      MyChangeSelector(wMySel, dwList[w1], 0xffff);

      wsprintf(tbuf, " %08lx    %-8.8s     %04x:%02d.%02d   ",
               (DWORD)dwList[w1], (LPSTR)lpDDB->DDB_Name,
               lpDDB->DDB_Req_Device_Number,
               lpDDB->DDB_Dev_Major_Version, lpDDB->DDB_Dev_Minor_Version);

      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)tbuf);
   }


   MyFreeSelector(wMySel);

}




/***************************************************************************/
/*                                                                         */
/*   'TEST' function - allows me to try something out in *this* program    */
/*                                                                         */
/***************************************************************************/


void FAR PASCAL TestFunction(HWND hChild)
{
char buf[64];
LPSTR lp1;
DWORD dwAddr, dw1;
WORD wSel, w1, wIter;




   *buf = 0;     // blank string!

   if(GetUserInput("* VIEW MEMORY *", "Enter 32-bit memory address",
                   buf, sizeof(buf)))
   {
      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)
                  "** USER CANCELED INPUT **");

      return;
   }

   lp1 = buf;

   while(*lp1 && *lp1<=' ') lp1++;

   if(lp1>buf) lstrcpy(buf, lp1);

   lp1 = buf + lstrlen(buf);

   while(lp1>buf && *(lp1 - 1)<=' ') lp1--;

   *lp1 = 0;

   if(!*buf)
   {
      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)
                  "** BLANK USER INPUT **");

      return;
   }

   for(dwAddr=0, lp1=buf; *lp1; lp1++)
   {
      if(*lp1>='0' && *lp1<='9')
      {
         dwAddr = dwAddr * 0x10L + (unsigned char)*lp1 - (unsigned char)'0';
      }
      else if(*lp1>='A' && *lp1<='F')
      {
         dwAddr = dwAddr * 0x10L + 10L +
                  (unsigned char)*lp1 - (unsigned char)'A';
      }
      else if(*lp1>='a' && *lp1<='f')
      {
         dwAddr = dwAddr * 0x10L + 10L +
                  (unsigned char)*lp1 - (unsigned char)'a';
      }
      else
      {
         SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)
                     "** INVALID USER INPUT **");

         return;
      }
   }

   wSel = MyAllocSelector();

   if(!wSel)
   {
      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)
                  "** NOT ENOUGH MEMORY/SELECTORS **");

      return;
   }

   MyChangeSelector(wSel, dwAddr, 0xffff);

   lp1 = MAKELP(wSel, 0);

   wIter = 0;

   for(dw1=dwAddr, w1=0; w1 < 0x1000; dw1 += 0x10, w1 += 0x10)
   {
      if(!((++wIter) & 0x1f)) MySetCursor(2);


      wsprintf(buffer, "%08lx:  %04x:%04x %04x:%04x %04x:%04x %04x:%04x "
                       ": %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c", dw1,
               *(WORD FAR *)(lp1 + w1 + 2), *(WORD FAR *)(lp1 + w1),
               *(WORD FAR *)(lp1 + w1 + 6), *(WORD FAR *)(lp1 + w1 + 4),
               *(WORD FAR *)(lp1 + w1 + 10), *(WORD FAR *)(lp1 + w1 + 8),
               *(WORD FAR *)(lp1 + w1 + 14), *(WORD FAR *)(lp1 + w1 + 12),
               (char)(lp1[w1 + 0]>=' '?lp1[w1 + 0]:'.'),
               (char)(lp1[w1 + 1]>=' '?lp1[w1 + 1]:'.'),
               (char)(lp1[w1 + 2]>=' '?lp1[w1 + 2]:'.'),
               (char)(lp1[w1 + 3]>=' '?lp1[w1 + 3]:'.'),
               (char)(lp1[w1 + 4]>=' '?lp1[w1 + 4]:'.'),
               (char)(lp1[w1 + 5]>=' '?lp1[w1 + 5]:'.'),
               (char)(lp1[w1 + 6]>=' '?lp1[w1 + 6]:'.'),
               (char)(lp1[w1 + 7]>=' '?lp1[w1 + 7]:'.'),
               (char)(lp1[w1 + 8]>=' '?lp1[w1 + 8]:'.'),
               (char)(lp1[w1 + 9]>=' '?lp1[w1 + 9]:'.'),
               (char)(lp1[w1 + 10]>=' '?lp1[w1 + 10]:'.'),
               (char)(lp1[w1 + 11]>=' '?lp1[w1 + 11]:'.'),
               (char)(lp1[w1 + 12]>=' '?lp1[w1 + 12]:'.'),
               (char)(lp1[w1 + 13]>=' '?lp1[w1 + 13]:'.'),
               (char)(lp1[w1 + 14]>=' '?lp1[w1 + 14]:'.'),
               (char)(lp1[w1 + 15]>=' '?lp1[w1 + 15]:'.') );

      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)buffer);

   }

   MyFreeSelector(wSel);

}


void FAR PASCAL NewTestFunction(HWND hChild)
{
int i;
LPSTR lpAPI;



   SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)
               "** WHICH WINDOWS VDD's HAVE PM API INTERFACES? **");


   for(i=1; i<256; i++)
   {

      lpAPI = NULL;

      _asm
      {
         push di
         push es

         mov ax, 0x1684
         mov bx, i

         les di, lpAPI

         int 0x2f

         mov WORD PTR lpAPI, di
         mov WORD PTR lpAPI + 2, es

         pop es                      ; restore seg register *NOW*
         pop di
      }

      if(lpAPI)
      {

         wsprintf(buffer, "VDD #%d PM entry point: %04x:%04x",
                  i, SELECTOROF(lpAPI), OFFSETOF(lpAPI));

         SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)
                     (LPSTR)buffer);
      }

   }


   SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)
               "              ** END OF LIST **");

}






extern WORD PASCAL wCode32Alias, PASCAL wData32Alias;

void FAR PASCAL OldOldTestFunction(HWND hChild)
{
WORD w, w2, w3, wMySel;
DWORD dw1, dw2, dwCB, dwCBSys;
char buf[64];
LPWOATABLE lpWT;
LPCONTROL_BLOCK lpCB;


   dwCBSys = MyGetSystemVM();

   SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)
	       "** INFORMATION OBTAINED VIA 32-bit CODE AND VMM API **");

   if(dwCBSys)
   {
      wMySel = MyAllocSelector();

      lpCB = (LPCONTROL_BLOCK)MAKELP(wMySel, 0);

      dwCB = dwCBSys;

      do
      {
	 MyChangeSelector(wMySel, dwCB, 0xffff);

	 wsprintf(buffer, "VMID=%02d  HANDLE=%04x:%04x  BASE=%04x:%04x  PTR=%04x:%04x",
		  (WORD)lpCB->CB_VMID,
		  HIWORD(dwCB),
		  LOWORD(dwCB),
		  HIWORD(lpCB->CB_High_Linear),
		  LOWORD(lpCB->CB_High_Linear),
		  HIWORD(lpCB->CB_Client_Pointer),
		  LOWORD(lpCB->CB_Client_Pointer));

	 SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)
		     (LPSTR)buffer);

	 dwCB = MyGetNextVM(dwCB);

      } while(dwCB && dwCBSys!=dwCB);

      if(!dwCB)
      {
	 SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)
		     "   ** ERROR - 'MyGetNextVM()' returned NULL **");

      }

      MyFreeSelector(wMySel);

      wMySel = NULL;
   }
   else
   {
      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)
		  "   ** ERROR - unable to access VMM API **");

   }


   SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)"");

   lpWT = GetWOATABLE();

   if(!lpWT)
   {
      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)
	   "** UNABLE TO GET 'WINOLDAP' TASK TABLE **");
   }
   else
   {
      SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)
	   "** INFORMATION OBTAINED FROM 'WINOLDAP' TASK TABLE **");

      w2 = GlobalSizePtr(lpWT);  // how big is it, anyway?

      w2 = (w2 - sizeof(*lpWT) + sizeof(lpWT->EntryTable[0]))
	   / sizeof(lpWT->EntryTable[0]);

      if(w2 > 64)   w2 = 64;     // I assume maximum of 64 DOS APP's!
				 // Based on experimentation with Win 3.1
				 // WINOLDAP data segment size & rough calcs

      wMySel = MyAllocSelector();
      if(!wMySel)
      {
	 SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)
	   "       ** UNABLE TO ALLOCATE SELECTOR! **");

	 return;
      }

      lpCB = (LPCONTROL_BLOCK)MAKELP(wMySel, 0);

      for(w=0; w<w2; w++)
      {

	 if(lpWT->EntryTable[w].hWnd       &&
	    lpWT->EntryTable[w].hInst      &&
	    lpWT->EntryTable[w].dwVMHandle &&
	    lpWT->EntryTable[w].hTask )
	 {
	    MyChangeSelector(wMySel, lpWT->EntryTable[w].dwVMHandle, 0xffff);

	    GetWindowText(lpWT->EntryTable[w].hWnd, buf, sizeof(buf));

	    wsprintf(buffer, "VM ID: %2d  TASK: %04xH  VM HANDLE: %04x:%04x   HWND: %04x   CAPTION: %s",
		     (WORD)lpCB->CB_VMID,
		     (WORD)lpWT->EntryTable[w].hTask,
		     HIWORD(lpWT->EntryTable[w].dwVMHandle),
		     LOWORD(lpWT->EntryTable[w].dwVMHandle),
		     (WORD)lpWT->EntryTable[w].hWnd,
		     (LPSTR)buf);

            SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)
                        (LPSTR)buffer);

               // NEXT, dump the 'desired' area to see if there's some
               //       consistent method that tells me what my WINOLDAP is!

            MyChangeSelector(wMySel,
                             *(DWORD FAR *)(((char FAR *)lpCB) + 0x88),
                             0xffff);

            for(w3 = 0; w3<0x140; w3 += 8)
            {
               wsprintf(buffer, "          %08lx:        %04x:%04x          %04x:%04x",
                        (DWORD)w3,
                        *(WORD FAR *)(((char FAR *)lpCB) + w3 + 2),
                        *(WORD FAR *)(((char FAR *)lpCB) + w3),
                        *(WORD FAR *)(((char FAR *)lpCB) + w3 + 6),
                        *(WORD FAR *)(((char FAR *)lpCB) + w3 + 4));

               SendMessage(hChild, LB_INSERTSTRING, (WPARAM)-1, (LPARAM)
                           (LPSTR)buffer);
            }


         }

      }


      MyFreeSelector(wMySel);

   }

}
