/***************************************************************************/
/*                                                                         */
/*   WINCMD_C.C - Command line interpreter for Microsoft(r) Windows (tm)   */
/*                                                                         */
/*                      TRADE SECRET - CONFIDENTIAL                        */
/*                                                                         */
/*             The compiled libraries and API header files are             */
/*          (c) Copyright 1990-1995 by Stewart~Frazier Tools, Inc          */
/*                                                                         */
/*   This contains the 'COMMAND' processing functions, including the main  */
/*   parser and some minor 'support' stuff unique to command processing.   */
/*                                                                         */
/***************************************************************************/


#define THE_WORKS
#include "mywin.h"

#ifndef WIN32
typedef unsigned short USHORT;  // necessary for 'dbt.h' (see below)
#endif /* WIN32 */

#include "dbt.h"            // definitions for 'WM_DEVICECHANGE'

#include "mth.h"
#include "wincmd.h"         /* 'dependency' related definitions */
#include "wincmd_f.h"       /* function prototypes and variables */


#if defined(M_I86LM)

  #pragma message("** Large Model In Use - Single Instance Only!! **")

#elif !defined(M_I86MM)

  #error  "** Must use MEDIUM model to compile, silly!! **"

#else

  #pragma message("** Medium Model Compile - Multiple Instances OK **")

#endif


#ifndef WM_FILESYSCHANGE

#define WM_FILESYSCHANGE    0x0034              /* ;Internal */

#endif  /* WM_FILESYSCHANGE */



            /**ออออออออออออออออออออออออออออออออออออออออออออ**/
            /**    Special structure definition for TDB    **/
            /** (Hacked-up and corrected version from DDK) **/
            /**    (the DDK version had errors in it!!)    **/
            /**ออออออออออออออออออออออออออออออออออออออออออออ**/
            /** (this structure also located in WINCMD_U!) **/
            /**ออออออออออออออออออออออออออออออออออออออออออออ**/

typedef struct tagTDB {
   HTASK     hNext;       /* next task in list, or NULL */
   WORD      wSP, wSS;    /* stack pointer and segment */
   WORD      nEvents;     /* # of pending events */
   BYTE      bPriority,   /* priority (0 is highest) */
             bJunk0;      /* was known as 'task flags' but isn't... */
   WORD      wJunk1;      /* who knows? */
   HTASK     hSelf;       /* equal to itself if valid!!! */
   WORD      wJunk2[4];   /* 4 words of garbage... */
   WORD      wFlags;      /* the *REAL* task flags!! */
   WORD      wErrMode;    /* error mode (0 for Windows, 1 for Task) */
   WORD      wExpWinVer;  /* expected windows version */
   HINSTANCE hInst;       /* instance handle for this task */
   HMODULE   hModule;     /* module handle for this task */

  /*** OFFSET 20H ***/

   HGLOBAL   hQueue;      /* handle to message queue (owned by 'USER'?) */
   WORD      junk3[2];    /* two more words of junk ?? */
   DWORD     dwSignal;    /* signal proc (?) */
   DWORD     dwGNotify;   /* global notify proc (?) */
   WORD      junk4[2];    /* two more words of junk ?? */

  /*** OFFSET 32H ***/

   DWORD     dwIntVect[7];/* 7 interrupt vectors (??) */

  /*** OFFSET 4EH ***/

   WORD    wLIMSave;    /* LIM save area offset within TDB */
   WORD    wEMSPID;     /* EMS PID for this task... */
   DWORD   dwEEMSSave;  /* far ptr to EEMS save area (in a TDB) ? */
   WORD    wEMSBCNT;    /* EMS bank count allocated so far */
   WORD    wEMSMAXBCNT; /* EMS Maximum bank count which this task wants */
   BYTE    bEMSRegSet,  /* EMS Register Set this TDB lives in */
           bLibrary;    /* tracks adds/deletes of all libraries in system */

  /*** OFFSET 5CH ***/

   LPSTR   lpPHT;       /* far ptr to private handle table */
   WORD    wPDB;        /* the PDB (or PSP) - also at TDB:0x100 (aliased?) */
   LPSTR   lpDTA;       /* the DTA, which proves PDB is at above address */
   BYTE    bDrive,      /* drive # (may have bit 8 set... Hard drive flag?)*/
           szPath[65];  /* total of 66 chars for drive + path */

  /*** OFFSET A8H ***/

   WORD    wValidity;   /* this is the 'AX' to be passed to a task (?) */

   HTASK   hYieldTo;    /* to whom do I 'directed yield'? (Arg goes here) */
   WORD    wLibInitSeg; /* segment of lib's to init (?) */

   WORD    wThunks[34]; /* no kidding, I counted 34 of them!! */

  /*** OFFSET F2H ***/

   BYTE    sModule[8];  /* module name - may not have a NULL byte! */
   BYTE    bSig[2];     /* signature 'word' - actually 2 bytes - "TD" */
                        /* use this feature to validate a task database */
   WORD    junk5[2];    /* 2 words of garbage at the end */

  /*** OFFSET 100H ***/ /* this is where the PDB begins, by the way! */

  } TDB;

typedef TDB FAR *LPTDB;



         /** back to the STONE AGE - an FCB structure definition **/

#pragma pack(1)

typedef struct {
   BYTE extFlag;
   BYTE junk[5];
   BYTE attrib;      /* assign to 8 for deleting volume labels */
   BYTE drivenum;    /* 0==current, 1==A, etc. */
   BYTE name[11];    /* file name without the '.' and spaces padding it */
   WORD curblock;
   WORD recsize;
   DWORD filesize;
   WORD  date;
   WORD  time;
   BYTE  reserved[8];
   BYTE  position;
   DWORD recnum;
   } FCB;

typedef FCB FAR *LPFCB;

#pragma pack()



               /* general use (local) function prototypes */



             /**-------------------------------------------**/
             /** Support functions for individual commands **/
             /**-------------------------------------------**/

BOOL EXPORT CALLBACK CMDCloseTaskEnum(HWND hWnd, LPARAM lParam);
BOOL EXPORT CALLBACK CMDCloseTaskEnum2(HWND hWnd, LPARAM lParam);

LPSTR FAR PASCAL CMDDdeFindStr(LPSTR lpList, LPSTR lpStr);
BOOL FAR PASCAL CMDDdeSub1(LPSTR lp2, WORD wOption);
BOOL FAR PASCAL CMDDdeErrorCheck(BOOL wEnvVarFlag);
DWORD FAR PASCAL CMDDdeGetConvValue(LPSTR lp1);

BOOL FAR PASCAL CMDDirSort(LPDIR_INFO lpDirInfo, LPSTR lpSwitchArgs,
                           WORD wNFiles);

int FAR _cdecl CMDDirSortProc(LPSTR lp1, LPSTR lp2);

int FAR _cdecl CMDMemSortProc(LPSTR lp1, LPSTR lp2);


void FAR PASCAL CMDHelpPrintCommands(void);
void FAR PASCAL CMDHelpPrintKeys(void);
void FAR PASCAL CMDHelpPrintVariables(void);
void FAR PASCAL CMDHelpPrintAnsi(void);
void FAR PASCAL CMDHelpPrintString(WORD i);
void FAR PASCAL CMDHelpPrintString2(WORD i);

void FAR PASCAL CMDListOpenAddLine0(LPSTR lpData, LPSFT_INFO FAR *lplpSFT,
                                    WORD FAR *lpwCount);
void FAR PASCAL CMDListOpenAddLine1(LPSTR lpData, LPSFT_INFO FAR *lplpSFT,
                                    WORD FAR *lpwCount);
void FAR PASCAL CMDListOpenAddLine2(LPSTR lpData, LPSFT_INFO FAR *lplpSFT,
                                    WORD FAR *lpwCount);
void FAR PASCAL CMDListOpenAddLine3(LPSTR lpData, LPSFT_INFO FAR *lplpSFT,
                                    WORD FAR *lpwCount);

int FAR _cdecl CMDListOpenCompare(LPSTR lp1, LPSTR lp2);

int FAR _cdecl CMDListOpenCompare95(LPSTR lp1, LPSTR lp2);

BOOL FAR PASCAL CMDReplaceConfirm(LPSTR lpPathSpec);

BOOL EXPORT CALLBACK CMDSubstEnum(HWND hWnd, LPARAM lParam);

BOOL FAR PASCAL CMDSysTransferSub(LPSTR lp1, LPSTR lp2, BOOL ErrMsg);

BOOL EXPORT CALLBACK CMDTaskListEnum(HWND hWnd, LPARAM lParam);

BOOL FAR PASCAL CMDXcopyConfirm(LPSTR lpPathSpec, LPDIR_INFO lpDI);



#define LockCode() { _asm push cs _asm call LockSegment }
#define UnlockCode() { _asm push cs _asm call UnlockSegment }



     /****************************************************************/
     /**                                                            **/
     /** THE 'ProcessCommand()' FUNCTION WAS MOVED TO 'WINCMD_B.C'. **/
     /** Others (parsing, 'IF', 'GOTO', 'FOR', 'WAIT', etc.) were   **/
     /** also moved to 'WINCMD_B.C'.                                **/
     /**                                                            **/
     /****************************************************************/




/***************************************************************************/
/*                                                                         */
/*           *** THE ACTUAL COMMANDS ARE PERFORMED HERE!! ***              */
/*                                                                         */
/***************************************************************************/


BOOL FAR PASCAL CMDNoRun   (LPSTR lpCmd, LPSTR lpArgs)
{
   PrintErrorMessage(520);       // "You cannot run \""
   PrintString(lpCmd);
   PrintErrorMessage(521);       // "\" while under Windows(tm)!\r\n"
                                 //  "Doing so could potentially corrupt the system.\r\n\n"

   return(TRUE);
}


#pragma code_seg("CMD_Bad_TEXT", "CODE")

BOOL FAR PASCAL CMDBad     (LPSTR lpCmd, LPSTR lpArgs)
{

   /** NULL's for 'lpCmd' and 'lpArgs' MUST be supported!! **/

   PrintErrorMessage(522);       // "?Bad command, file name, or path\r\n\n"

   return(TRUE);
}



BOOL FAR PASCAL CMDNotYet  (LPSTR lpCmd, LPSTR lpArgs)
{
   PrintErrorMessage(523);       // "I'm sorry, but the function \""
   PrintString(lpCmd);
   PrintErrorMessage(524);       // "\" is not (yet) supported...\r\n\n"

   return(TRUE);
}

    /*** ERROR CHECKER - Prints DOS error message if available! ***/

#pragma code_seg ("CMDErrorCheck_TEXT","CODE")

BOOL FAR PASCAL CMDErrorCheck(BOOL rval)
{
LPSTR lpTempBuf;

   if(!rval) return(FALSE);

   lpTempBuf = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT, 512);
   if(!lpTempBuf)
   {
      PrintString("?Extremely low on memory - can't even process "
                  "error messages!\r\n\n");
      return(TRUE);
   }

   GetExtErrMessage(lpTempBuf);

   PrintString(lpTempBuf);
   PrintString(pCRLF);      /* one extra line feed! */

   GlobalFreePtr(lpTempBuf);     /* it *always* helps to free memory! */

   return(TRUE);
}


BOOL FAR PASCAL CMDCheckParseInfo(LPPARSE_INFO lpParseInfo, LPSTR lpSwChars,
                                   WORD wNParms)
{
int i;
volatile LPSTR lp1;



   if(lpParseInfo==(LPPARSE_INFO)NULL)
   {
      PrintErrorMessage(533);
      return(TRUE);
   }

   if(lpSwChars==(LPSTR)NULL || lstrlen(lpSwChars)==0)
   {
      if(lpParseInfo->n_switches!=0)
      {
         PrintErrorMessage(534);
         PrintString((LPSTR)(lpParseInfo->sbpItemArray[0]));
         PrintErrorMessage(535);
         return(TRUE);
      }
   }
   else
   {
      for(i=0; i<(int)(lpParseInfo->n_switches); i++)
      {
         lp1 = ((LPSTR)(lpParseInfo->sbpItemArray[i]));

         if(!_fstrichr(lpSwChars, *lp1) )
         {
            PrintErrorMessage(534);
            PrintString((LPSTR)(lpParseInfo->sbpItemArray[i]));
            PrintErrorMessage(535);
            return(TRUE);
         }
      }
   }


   switch(wNParms & CMD_T_MASK)
   {
      case CMD_DONTCARE:
        return(FALSE);                           /* don't check at all */

      case CMD_EXACT:

        if(lpParseInfo->n_parameters == (wNParms & CMD_N_MASK))
           return(FALSE);

        break;

      case CMD_ATLEAST:

        if(lpParseInfo->n_parameters >= (wNParms & CMD_N_MASK))
           return(FALSE);

        break;

      case CMD_ATMOST:

        if(lpParseInfo->n_parameters <= (wNParms & CMD_N_MASK))
           return(FALSE);

        break;


   }

   PrintErrorMessage(536);

   return(TRUE);

}



#pragma code_seg("CMDAttrib_TEXT","CODE")

BOOL FAR PASCAL CMDAttrib  (LPSTR lpCmd, LPSTR lpArgs)
{
LPPARSE_INFO lpParseInfo;
volatile LPSTR lpItem, lpPath, lpTemp, lpDirPath, lp1, lpDirSpec;
int i, j, k, n, ctr, n_switches;
int readonly=0, hidden=0, system=0, archive=0;
BOOL rval, AbnormalTermination, recurse = FALSE;
WORD nFiles, new_attr, wRecurse, wNRecurse, wTreeLevel, wTreeIndex;
LPTREE lpT, lpTree;
WORD wLevelArray[64];
DWORD dwTickCount;
LPDIR_INFO lpDirInfo = NULL, lpDI;




   dwTickCount = GetTickCount();


      /* this command is a *little bit* different */

   if(!(lpParseInfo = CMDLineParse(lpArgs)) ||
      !(lpTemp = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                       MAX_PATH * 2)))
   {
      if(lpParseInfo) GlobalFreePtr(lpParseInfo);

      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   lpDirPath = lpTemp + MAX_PATH;

   if(CMDCheckParseInfo(lpParseInfo, "sS", CMD_DONTCARE))
   {
      GlobalFreePtr(lpParseInfo);
      GlobalFreePtr(lpTemp);
      return(TRUE);
   }

   ReDirectOutput(lpParseInfo,TRUE);


   INIT_LEVEL

   n_switches = lpParseInfo->n_switches;

   if(n_switches)
   {
      for(i=0; i < (int)lpParseInfo->n_switches; i++)
      {
         if(toupper(*((LPSTR)(lpParseInfo->sbpItemArray[i])))=='S')
         {
            recurse = TRUE;
         }
      }
   }

   for(i=0; i < (int)lpParseInfo->n_parameters; i++)
   {
      lpItem = ((LPSTR)(lpParseInfo->sbpItemArray[i+n_switches]));

      if(*lpItem!='+' && *lpItem!='-')
      {
         if(i == (int)(lpParseInfo->n_parameters - 1)) break;

         PrintErrorMessage(545);
         PrintString(lpItem);
         PrintString(pQCRLFLF);

         i = -1;

         break;
      }

      switch(toupper(lpItem[1]))
      {
         case 'R':
            readonly = *lpItem=='+'?1:-1;
            break;

         case 'H':
            hidden   = *lpItem=='+'?1:-1;
            break;

         case 'S':
            system   = *lpItem=='+'?1:-1;
            break;

         case 'A':
            archive  = *lpItem=='+'?1:-1;
            break;

         default:
            PrintErrorMessage(546);
            PrintAChar(lpItem[1]);
            PrintErrorMessage(547);
      }
   }

   if(i < 0)
   {
      break;  /* syntax error was found in above command line */
   }


   if(i < (int)lpParseInfo->n_parameters)
   {
      lpPath = (LPSTR)(lpParseInfo->sbpItemArray[n_switches + i]);
   }
   else
   {
      lpPath = "*.*";  // blank path defaults to '*.*'
   }

   // it is important to distinguish a DIRECTORY from a file by adding
   // the wildcard or '\\' specifier at the end of the directory name


   DosQualifyPath(lpTemp, lpPath);

   if(!*lpTemp || QualifyPath(lpTemp, lpTemp, FALSE))  break;

   if(lpTemp[lstrlen(lpTemp) - 1]=='\\')  // last char is a '\\'
   {
      lstrcat(lpTemp, "*.*");
   }

        // use GetDirList0() in place of GetDirList() so it won't
        // 'qualify' the path if it's a directory!
        // also, if no wildcards include '_A_SUBDIR' in the criteria


   lstrcpy(lpDirPath, lpTemp);

   lp1 = _fstrrchr(lpDirPath, '\\'); /* get the 'final \' in the string */
   if(lp1)
   {
      _fmemmove(lp1 + 2, lp1 + 1, lstrlen(lp1));

      *(lp1 + 1) = 0;            /* terminate string after last backslash */

      lpDirSpec = lp1 + 2;
   }
   else
   {
      PrintErrorMessage(575);
      break;
   }


   if(recurse)
   {
      lstrcpy(lpTemp, lpDirPath);

      if(!(lpTree = GetDirTree(lpTemp)))
      {
         if(ctrl_break)
         {
            PrintString(pCTRLC);
            PrintString(pCRLFLF);
         }
         else
         {
            PrintErrorMessage(576);
         }

         break;
      }

      lpT = lpTree;

      if(!(*lpTree->szDir))  /* empty tree list? */
      {
         wNRecurse = 1;
         recurse = FALSE;
         GlobalFreePtr(lpTree);
         lpTree = NULL;
      }
      else
      {
         wNRecurse = 0xffff;  /* for now, it's a bit easier... */
         wTreeLevel = 0;
         wTreeIndex = 0;

         wLevelArray[wTreeLevel] = wTreeIndex;
      }

   }
   else
   {
      wNRecurse = 1;   /* always go at least 1 time; helps detect errors */
      lpTree = NULL;
   }


   AbnormalTermination = FALSE;

   for(n=0, wRecurse=0; wRecurse<wNRecurse; wRecurse++)
   {
                        /** Get directory list buffer **/

      if(wRecurse && recurse)
      {
         lstrcpy(lpTemp, lpDirPath);

         for(ctr=0; ctr <= (int)wTreeLevel; ctr++)  /* build current path */
         {
            lstrcat(lpTemp, lpTree[wLevelArray[ctr]].szDir);
            lstrcat(lpTemp, pBACKSLASH);
         }

         lstrcat(lpTemp, lpDirSpec);

         if(lpTree[wTreeIndex].wChild)  /* a child element exists? */
         {
            wLevelArray[wTreeLevel++] = wTreeIndex;
            wTreeIndex = lpTree[wTreeIndex].wChild;

            wLevelArray[wTreeLevel] = wTreeIndex;
         }
         else if(lpTree[wTreeIndex].wSibling)
         {
            wTreeIndex = lpTree[wTreeIndex].wSibling;
            wLevelArray[wTreeLevel] = wTreeIndex;
         }
         else
         {
            while(wTreeLevel>0 && !lpTree[wTreeIndex].wSibling)
            {
               wTreeIndex = wLevelArray[--wTreeLevel];
            }

            if(!wTreeLevel && !lpTree[wTreeIndex].wSibling)
            {
               wNRecurse = wRecurse; /* end after this one! */
            }
            else
            {
               wTreeIndex = lpTree[wTreeIndex].wSibling;
               wLevelArray[wTreeLevel] = wTreeIndex;
            }
         }
      }
      else
      {        /* first time through - current dir (always) */

         lstrcpy(lpTemp, lpDirPath);
         lstrcat(lpTemp, lpDirSpec);

      }


      // Note:  'ATTRIB' doesn't qualify DIRECTORY path names, unlike 'DIR',
      //        which converts a directory name to "directory\*.*".

      if(recurse)
      {
         nFiles = GetDirList0(lpTemp, (~_A_VOLID)&0xff,
                              (LPDIR_INFO FAR *)&lpDirInfo, work_buf, FALSE, NULL);
      }
      else
      {
         if(!(nFiles = GetDirList0(lpTemp, (~_A_SUBDIR&~_A_VOLID)&0xff,
                                  (LPDIR_INFO FAR *)&lpDirInfo, work_buf, FALSE, NULL)))
         {
            if(_fstrchr(lpTemp, '*') || _fstrchr(lpTemp, '?') ||
               !(nFiles = GetDirList0(lpTemp, (~_A_VOLID)&0xff,
                                  (LPDIR_INFO FAR *)&lpDirInfo, work_buf, FALSE, NULL)))
            {

               PrintErrorMessage(548);
               rval = FALSE;
               break;
            }
         }
      }

      if(!ctrl_break && (GetTickCount() - dwTickCount) >= 50)  // 20 times per second
      {
         if(LoopDispatch())
         {
            AbnormalTermination = TRUE;

            if(lpDirInfo) GlobalFreePtr((LPSTR)lpDirInfo);
            lpDirInfo = NULL;
            break;
         }

         dwTickCount = GetTickCount();
      }

      if(ctrl_break)
      {
         AbnormalTermination = TRUE;

         if(lpDirInfo) GlobalFreePtr((LPSTR)lpDirInfo);
         lpDirInfo = NULL;

         break;
      }


      if(!nFiles || nFiles==DIRINFO_ERROR)
      {
         if(lpDirInfo) GlobalFreePtr((LPSTR)lpDirInfo);
         lpDirInfo = NULL;

         if(!nFiles)   continue;    /* if it's empty, just continue on */
         else          break;       /* otherwise, bail out! */
      }



      // PERFORM THE REQUIRED 'ATTRIB' PROCESS here...


      lstrcpy(lpTemp, work_buf);

      lp1 = lpTemp + lstrlen(lpTemp);

      if(lp1!=lpTemp && *(lp1 - 1)!='\\')
      {
         *(lp1++) = '\\';        /* ensure it ends in a backslash */
      }

      for(rval=FALSE, lpDI=lpDirInfo, j=0; j<(int)nFiles; j++, lpDI++)
      {
         if((GetTickCount() - dwTickCount) >= 50)  // 20 times per second
         {
            if(LoopDispatch())
            {
               AbnormalTermination = TRUE;

               break;
            }

            dwTickCount = GetTickCount();
         }

         if(redirect_output==HFILE_ERROR)
         {
            while(!ctrl_break && stall_output)
            {
               MthreadSleep(50);
               if(!ctrl_break) ctrl_break = LoopDispatch();
            }
         }

         if(ctrl_break)
         {
            stall_output = FALSE;
            AbnormalTermination = TRUE;

            break;
         }

         if(lpDI->fullname[0]=='.' &&
            (!lpDI->fullname[1] ||
             (lpDI->fullname[1]=='.' && !lpDI->fullname[2])))
         {
            continue;  // ignore THESE files ('.' and '..')
         }


         _fmemset(lp1, 0, sizeof(lpDI->fullname) + 2);
         _fstrncpy(lp1, lpDI->fullname, sizeof(lpDI->fullname));

         if(!readonly && !hidden && !system && !archive)
         {
            n++;  // count the files THIS way!

            i = 0;

            if(lpDI->attrib & _A_RDONLY) {PrintAChar('R'); i++;}
            if(lpDI->attrib & _A_HIDDEN) {PrintAChar('H'); i++;}
            if(lpDI->attrib & _A_SYSTEM) {PrintAChar('S'); i++;}
            if(lpDI->attrib & _A_ARCH)   {PrintAChar('A'); i++;}
            if(lpDI->attrib & _A_VOLID)  {PrintAChar('V'); i++;}
            if(lpDI->attrib & _A_SUBDIR) {PrintAChar('D'); i++;}

            for(k=8; k>i; k--)
            {
               PrintAChar(' ');
            }

            PrintString(lpTemp);

            PrintString(pCRLF);
         }
         else
         {
            new_attr = lpDI->attrib;

            if(readonly<0)        new_attr &= ~_A_RDONLY;
            else if(readonly>0)   new_attr |= _A_RDONLY;

            if(hidden<0)          new_attr &= ~_A_HIDDEN;
            else if(hidden>0)     new_attr |= _A_HIDDEN;

            if(system<0)          new_attr &= ~_A_SYSTEM;
            else if(system>0)     new_attr |= _A_SYSTEM;

            if(archive<0)         new_attr &= ~_A_ARCH;
            else if(archive>0)    new_attr |= _A_ARCH;

            if(new_attr != (WORD)(lpDI->attrib))  /* it changed! */
            {
               new_attr &= ~_A_SUBDIR;  // clear the 'dir' bit (if there)

               PrintErrorMessage(525); // "Updating "
               PrintString(lpTemp);
               PrintErrorMessage(526); // " -- "

               if(MySetFileAttr(lpTemp, new_attr))  /* change it! */
               {
                  PrintErrorMessage(537);  // "  ** ERROR **\r\n"
               }
               else
               {
                  n = n + 1;
                  PrintErrorMessage(527);  // "  ** OK **\r\n"
               }
            }
         }

         *lp1 = 0;
      }

      // end of the 'BIG LOOP'

      if(lpDirInfo) GlobalFreePtr(((LPSTR)lpDirInfo)); /* I'm done with this... */
      lpDirInfo = NULL;

      if(AbnormalTermination) break;
   }


   if(!AbnormalTermination && !rval)
   {
      if((readonly || hidden || system || archive) && n!=1)
      {
       static const char CODE_BASED pFmt0[]=
                                        "No files changed\r\n\n";
       static const char CODE_BASED pFmt[]=
                                        "Total of %u files changed\r\n\n";

         LockCode();

         if(!n)  lstrcpy(work_buf, pFmt0);
         else    wsprintf(work_buf, pFmt, n);

         UnlockCode();
      }
      else if((readonly || hidden || system || archive) && n==1)
      {
       static const char CODE_BASED pFmt[]=
                                                   "1 file changed\r\n\n";
         LockCode();
         lstrcpy(work_buf, pFmt);
         UnlockCode();
      }
      else if(nFiles!=1)
      {
       static const char CODE_BASED pFmt0[]=
                                               "No files found.\r\n\n";
       static const char CODE_BASED pFmt[]=
                                               "Total of %u files.\r\n\n";
         LockCode();

         if(!n)  lstrcpy(work_buf, pFmt0);
         else    wsprintf(work_buf, pFmt, n);

         UnlockCode();
      }
      else
      {
       static const char CODE_BASED pFmt[]=
                                                 "Total of 1 file.\r\n\n";
         LockCode();
         lstrcpy(work_buf, pFmt);
         UnlockCode();
      }

      PrintString(work_buf);
   }


   END_LEVEL

   if(ctrl_break)
   {
      PrintString(pCTRLC);
      PrintString(pCRLFLF);

      if(!BatchMode) ctrl_break = FALSE;  // eat ctrl break if not BATCH
   }

   if(lpDirInfo)    GlobalFreePtr(lpDirInfo);
   if(lpTree)       GlobalFreePtr(lpTree);

   if(lpParseInfo)
   {
      ReDirectOutput(lpParseInfo,FALSE);
      GlobalFreePtr(lpParseInfo);
   }

   if(lpTemp) GlobalFreePtr(lpTemp);

   return(rval);
}


#pragma code_seg("CMDBreak_TEXT","CODE")

BOOL FAR PASCAL CMDBreak  (LPSTR lpCmd, LPSTR lpArgs)
{
WORD flag;
static char pBREAKIS[]="BREAK is ";


   lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   if(!*lpArgs)
   {
      _asm
      {
         mov ax, 0x3300
         int 0x21
         mov dh, 0
         mov flag, dx
      }

      PrintString(pBREAKIS);
      PrintString(flag?pON:pOFF);
      PrintString(pCRLFLF);
   }
   else if(!_fstricmp(lpArgs, pON))
   {
      _asm
      {
         mov ax, 0x3301
         mov dl, 1
         int 0x21

      }

      PrintString(pBREAKIS);
      PrintString(pON);
      PrintString(pCRLFLF);
   }
   else if(!_fstricmp(lpArgs, pOFF))
   {
      _asm
      {
         mov ax, 0x3301
         mov dl, 0
         int 0x21

      }

      PrintString(pBREAKIS);
      PrintString(pOFF);
      PrintString(pCRLFLF);
   }
   else
   {
      PrintString(pILLEGALARGUMENT);
      PrintString(lpArgs);
      PrintString(pQCRLFLF);

      GlobalFreePtr(lpArgs);
      return(TRUE);
   }

   GlobalFreePtr(lpArgs);
   return(FALSE);
}


#pragma code_seg("CMDCalc_TEXT","CODE")

BOOL FAR PASCAL CMDCalc    (LPSTR lpCmd, LPSTR lpArgs)
{
LPSTR lp1;

   lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   if(!lstrcmp(lpCmd, "??") ||  // special case - '??' used to invoke CALC
      (*lpArgs=='/' && toupper(lpArgs[1])=='Q' && lpArgs[2] <= ' '))
   {
      if(!lstrcmp(lpCmd, "??"))     // 2 question marks ==> no '/Q' in lpArgs
      {
         lp1 = Calculate(lpArgs);
      }
      else
      {
         lp1 = Calculate(lpArgs + 2);  // use '2' in case of "/Q" and nothing else
      }

      GlobalFreePtr(lpArgs);

      if(!lp1)
      {
         PrintErrorMessage(549);

         return(TRUE);
      }
      else
      {
         GlobalFreePtr(lp1);  // "lose" the output

         return(FALSE);
      }
   }


   lp1 = Calculate(lpArgs);
   GlobalFreePtr(lpArgs);


   if(!lp1)
   {
      PrintErrorMessage(549);

      return(TRUE);
   }
   else
   {
      while(!ctrl_break && stall_output)
      {
         MthreadSleep(50);
         if(!ctrl_break) ctrl_break = LoopDispatch();
      }

      if(ctrl_break)
      {
         PrintString(pCTRLC);

         stall_output = FALSE;
      }
      else
      {
         PrintString(lp1);
      }

      if(!BatchMode || !lpBatchInfo || BatchEcho)
      {
         PrintString(pCRLFLF);
      }
      else
      {
         PrintString(pCRLF);
      }

      GlobalFreePtr(lp1);

      if(ctrl_break)
      {
         if(!BatchMode) ctrl_break = FALSE;  // if not BATCH eat it!

         return(TRUE);
      }

      return(FALSE);
   }
}



#pragma code_seg("CMDCall_TEXT","CODE")

BOOL FAR PASCAL CMDCall    (LPSTR lpCmd, LPSTR lpArgs)
{
BOOL rval;
LPSTR lp1;
char c, temp_buf[MAX_PATH];


   lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   if(*lpArgs)
   {
      IsCall = TRUE;    /* this causes batch files to be able to return  */
                        /* by telling 'CMDRunProgram' to set up a ptr to */
                        /* the previous batch information.               */

      for(lp1=lpArgs; *lp1 && *lp1 <= ' '; lp1++)
         ;  /* find first non-white-space */

      if(lp1 > lpArgs) lstrcpy(lpArgs, lp1);

      lp1 = lpArgs;

      if(*lp1 == '\"')
      {
         lstrcpy(lpArgs, lpArgs + 1);
         while(*lp1 && *lp1 != '\"') lp1++;

         if(*lp1) *(lp1++) = 0;
         lstrcpy(temp_buf, lpArgs);
      }
      else
      {
         while(*lp1 && *lp1>' ' && *lp1!='/') lp1++;
                          /* find next white space, '/', or end of string */
         c = *lp1;
         *lp1 = 0;  /* temporary */
         lstrcpy(temp_buf, lpArgs);
         *lp1 = c;
      }


      while(*lp1 && *lp1<=' ') lp1++; /* find next non-space or end of string */

      rval = CMDRunProgram(temp_buf, lp1, SW_SHOWNORMAL);

      IsCall = FALSE;     /* turn flag off... I'm done with it now! */

      GlobalFreePtr(lpArgs);
      return(rval);
   }
   else
   {
      PrintErrorMessage(536);

      GlobalFreePtr(lpArgs);
      return(FALSE);
   }

}


#pragma code_seg("CMDChdir_TEXT","CODE")

BOOL FAR PASCAL CMDChdir   (LPSTR lpCmd, LPSTR lpArgs)
{

   lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   if(*lpArgs==0)
   {
      getcwd(work_buf, sizeof(work_buf));
      strcat(work_buf, pCRLFLF);
      PrintString(work_buf);             /* display current working dir */
   }
   else
   {
      lstrcpy(work_buf, lpArgs);
      GlobalFreePtr(lpArgs);

      if(*work_buf == '\"')  // quoted path?
      {
       LPSTR lp1 = work_buf;

         lstrcpy(lp1, lp1 + 1);
         while(*lp1 && *lp1 != '\"') lp1++;

         if(*lp1=='\"')
         {
            if(*lp1) *(lp1++) = 0;

            while(*lp1 && *lp1 <= ' ') lp1++;

            if(*lp1) // non-white-space is a syntax error...
            {
               PrintErrorMessage(868);  // "?Invalid 'long' file name specified\r\n"
               return(TRUE);
            }
         }
      }

      return(CMDErrorCheck(MyChDir(work_buf)));
                                   /* change directories according to lp3! */
   }

   GlobalFreePtr(lpArgs);
   return(FALSE);
}


#pragma code_seg("CMDCloseTask_TEXT","CODE")

HTASK CloseTaskEnumTask  = NULL;
WORD  CloseTaskEnumCount = 0;

BOOL FAR PASCAL CMDCloseTask(LPSTR lpCmd, LPSTR lpArgs)
{
HTASK hTask, FAR *lphTask;
TASKENTRY te;
LPPARSE_INFO lpParseInfo;
WNDENUMPROC lpProc;
WORD w, wCount;



   lpParseInfo = CMDLineParse(lpArgs);
   if(CMDCheckParseInfo(lpParseInfo, NULL, 1 | CMD_EXACT))
   {
      GlobalFreePtr(lpParseInfo);
      return(TRUE);
   }

   GlobalFreePtr((LPSTR)lpParseInfo);  /* who needs it now? */
   lpParseInfo = NULL;

   if(!(hTask = (HTASK)MyValue(lpArgs)))
   {

     /** It's not a valid handle, so look for a module name that matches **/

      _fmemset((LPSTR)&te, 0, sizeof(te));

      te.dwSize = sizeof(te);

      if(!lpTaskFirst || !lpTaskNext || !lpTaskFirst((LPTASKENTRY)&te))
      {
         PrintErrorMessage(550);
         return(TRUE);
      }

      wCount = 0;
      lphTask = (HTASK FAR *)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                            sizeof(HTASK) * GetNumTasks());
      if(!lphTask)
      {
         PrintString(pNOTENOUGHMEMORY);
         return(TRUE);
      }

      do
      {
         if(lpModuleFindHandle)
         {
          MODULEENTRY me;

            _fmemset((LPVOID)&me, 0, sizeof(me));

            me.dwSize = sizeof(me);

            if(!lpModuleFindHandle(&me, te.hModule)) continue; // error

            if(!_fstricmp(me.szModule, lpArgs))
            {
               lphTask[wCount++] = te.hTask;
            }
         }
         else if(!_fstricmp(te.szModule, lpArgs))
         {
            lphTask[wCount++] = te.hTask;
         }

      } while(lpTaskNext((LPTASKENTRY)&te));

      if(wCount>1)
      {
         lpProc = (WNDENUMPROC)MakeProcInstance((FARPROC)CMDCloseTaskEnum2,
                                                hInst);

         PrintErrorMessage(538); // "** MULTIPLE Tasks have this module name **\r\n"
                                 // "Entry  Task Handle  Window Caption\r\n"
                                 // "-----  -----------  --------------\r\n"

         for(w=0; w<wCount; w++)
         {
            if(lpTaskFindHandle((LPTASKENTRY)&te, lphTask[w]))
            {
             static const char CODE_BASED pFmt[]=
                                                    " %2d)     %04x       ";
               LockCode();
               wsprintf(work_buf, pFmt, w+1, (WORD)te.hTask);
               UnlockCode();

               PrintString(work_buf);

               CloseTaskEnumTask = lphTask[w];
               CloseTaskEnumCount = 0;

               EnumWindows(lpProc, (LPARAM)NULL);

               if(!CloseTaskEnumCount)
                  PrintErrorMessage(539);  // " ** N/A **\r\n"

               PrintString(pCRLF);
            }
         }

         FreeProcInstance((FARPROC)lpProc);

         PrintErrorMessage(540);     // "Which Task to Close (1,2,3,etc.) or <Return> to cancel? "
         GetUserInput(work_buf);
         if(*work_buf<' ')
         {
            PrintErrorMessage(541);  // "** CANCELLED **\r\n\n"

            GlobalFreePtr(lphTask);
            return(TRUE);
         }
         else if((w = (WORD)MyValue(work_buf))>wCount || w<1)
         {
            PrintErrorMessage(542);  // "?Invalid Sequence # - \""
            PrintString(work_buf);
            PrintErrorMessage(543);  // "\".\r\n\n"

            GlobalFreePtr(lphTask);
            return(TRUE);
         }
         else
         {
            hTask = lphTask[w - 1];
         }

      }
      else if(wCount==0)
      {
         PrintErrorMessage(552);
         PrintString(lpArgs);
         PrintString(pQCRLFLF);

         GlobalFreePtr(lphTask);
         return(TRUE);
      }
      else
      {
         hTask = lphTask[0];
      }

      GlobalFreePtr(lphTask);
   }
   else if(!lpIsTask(hTask))
   {
      if(IsWindow((HWND)hTask))   /* is it a window handle?? */
      {
         PostMessage((HWND)hTask, WM_SYSCOMMAND, (WPARAM)SC_CLOSE, (LPARAM)0);
         PrintErrorMessage(554);
         return(FALSE);
      }

      {
       static const char CODE_BASED pFmt[]=
                              "?Task/Window ID %04xH(%d) not found...\r\n\n";
         LockCode();
         wsprintf(work_buf, pFmt, (WORD)hTask, (WORD)hTask);
         UnlockCode();
      }

      PrintString(work_buf);
      return(TRUE);
   }


   lpProc = (WNDENUMPROC)MakeProcInstance((FARPROC)CMDCloseTaskEnum, hInst);

   CloseTaskEnumTask = hTask;
   CloseTaskEnumCount = 0;

   EnumWindows(lpProc, (LPARAM)NULL);

   FreeProcInstance((FARPROC)lpProc);

   if(CloseTaskEnumCount==0)
   {
      PrintErrorMessage(552);
   }
   else
   {
      PrintErrorMessage(553);
   }


   CloseTaskEnumTask  = NULL;
   CloseTaskEnumCount = 0;

   return(FALSE);

}


BOOL EXPORT CALLBACK CMDCloseTaskEnum(HWND hWnd, LPARAM lParam)
{

   if(GetWindowTask(hWnd)==CloseTaskEnumTask)
   {
      PostMessage(hWnd, WM_CLOSE, 0, 0);  /* that's how TASKMAN works! */

      CloseTaskEnumCount++;
   }

   return(TRUE);        /* keep going... keep going... */

}


BOOL EXPORT CALLBACK CMDCloseTaskEnum2(HWND hWnd, LPARAM lParam)
{
   if(GetWindowTask(hWnd)==CloseTaskEnumTask && IsWindowVisible(hWnd))
   {
      GetWindowText(hWnd, work_buf, sizeof(work_buf));
      if(*work_buf)
      {
         PrintErrorMessage(843);      // "\r\033[20C"
         PrintString(work_buf);
         PrintString(pCRLF);
         CloseTaskEnumCount++;
      }
   }
   return(TRUE);
}


#pragma code_seg("CMDCls_TEXT","CODE")

BOOL FAR PASCAL CMDCls     (LPSTR lpCmd, LPSTR lpArgs)
{

   return(ClearScreen());
}

#pragma code_seg("CMDCommand_TEXT","CODE")

BOOL FAR PASCAL CMDCommand (LPSTR lpCmd, LPSTR lpArgs)
{
LPCSTR lpComSpec;
BOOL rval;


   if(!(lpComSpec = GetEnvString(pCOMSPEC)))
      lpComSpec = pCOMMANDCOM;


   lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   rval = CMDRunProgram((LPSTR)lpComSpec, lpArgs, SW_SHOWNORMAL);
   GlobalFreePtr(lpArgs);

   return(rval);
}

#pragma code_seg("CMDComp_TEXT","CODE")

BOOL FAR PASCAL CMDComp    (LPSTR lpCmd, LPSTR lpArgs)
{
BOOL rval;


   lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   rval = CMDRunProgram(lpCmd, lpArgs, SW_SHOWNORMAL);
   GlobalFreePtr(lpArgs);

   return(rval);
}


#pragma code_seg("CMDCopy_TEXT","CODE")

BOOL FAR PASCAL CMDCopy    (LPSTR lpCmd, LPSTR lpArgs)
{
LPPARSE_INFO lpParseInfo;
LPSTR lpTempBuf, lpSrc, lpDest, lp1, lp2, lp3;
BOOL  def_binary=TRUE, tofile_binary=TRUE, fromfile_binary=TRUE,
      FromFileDefault=FALSE, verify=FALSE, waitflag=FALSE, rval=TRUE;


   lpParseInfo = CMDLineParse(lpArgs);

   if(CMDCheckParseInfo(lpParseInfo, "ABVW", 1 | CMD_ATLEAST))
   {
      GlobalFreePtr(lpParseInfo);
      return(TRUE);
   }

   GlobalFreePtr(lpParseInfo);  /* now that I'm done, free it!!! */
   lpParseInfo = NULL;


         /** Because of the nature of 'COPY' I must parse **/
         /** the thing manually... oh, well...            **/

   if(!(lpTempBuf = EvalString(lpArgs)))
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   INIT_LEVEL


   /** FORMAT OF COMMAND (very very strict):                              **/
   /**   COPY [/A|/B] src[/A|/B][+src2[/A|/B][...]] [dest[/A|/B]][/V][/W] **/


   for(lp1 = lpTempBuf; *lp1<=' ' && *lp1; lp1++)
      ; /* find the first 'non-space' at the beginning of the cmd line */

   if(*lp1=='/')           /* a switch!!! */
   {
      lp1++;
      if(toupper(*lp1)=='A')  /* ascii!! */
      {
         def_binary = FALSE;  /* don't default to binary */
         fromfile_binary = FALSE;
         tofile_binary = FALSE;

         FromFileDefault = TRUE;/* I specified a 'From File' default switch */
      }
      else if(toupper(*lp1)=='B') /* binary!! */
      {
         def_binary = TRUE;   /* default to binary!! (default setting) */
         fromfile_binary = TRUE;
         tofile_binary = TRUE;

         FromFileDefault = TRUE;/* I specified a 'From File' default switch */
      }
      else
      {
         *(lp1 + 1) = 0;
         PrintString(pSYNTAXERROR);
         PrintString(lp1);
         PrintString(pQCRLFLF);
         break;
      }

      for(; *lp1>' '; lp1++)
         ;                       /* find the next white space character... */

      for(; *lp1<=' ' && *lp1; lp1++)
         ;                       /* find the next 'non-space' character... */
   }

              /** At this time 'lp1' points to the first arg **/


   lpSrc = lp1;

   while(*lp1 > ' ' && *lp1 != '/')
   {                 /* find the next white space character (or end of line) */
                     /* or a switch character */

      if(*lp1 == '\"')
      {
         lp1++;

         while(*lp1 && *lp1 != '\"') lp1++;
      }

      if(*lp1) lp1++;
   }

   if(*lp1=='/')         /* special case - deal with 'file1/a+file2/a...' */
   {
      lp2 = lp1;

      for(; *lp1>' '; lp1++)
      {            /* find the next white-space character or end of string */

         if(*lp1=='\"')  // quote mark?
         {
            lp1++;

            while(*lp1 && *lp1 != '\"') lp1++;
         }
      }

      if(lp1>(lp2 + 2))
      {
         lp3 = _fstrchr(lp2, '+');
         if(lp3 && lp3<lp1)
         {
//
//            lp3 = _fstrrchr(lp1, '+');  /* the very last '+' */
//
            for(lp3 = lp2 + lstrlen(lp2) - 1; lp3 >= lp2; lp3--)
            {
               if(*lp3=='\"' && lp3 > lp1)
               {
                  lp3--;

                  while(lp3 > lp1 && *lp3 != '\"') lp3--;
               }
               else if(*lp3 == '+') break;  // found

               if(lp3 == lp2)
               {
                  lp3 = NULL;
                  break;
               }
            }


            while(*lp3 && *lp3 != '/') /* the first '/' after the last '+' */
            {
               if(*lp3=='\"')
               {
                  lp3++;
                  while(*lp3 && *lp3 != '\"') lp3++;
               }

               if(*lp3) lp3++;
            }

            if(!lp3 || lp3>lp2)
               lp1 = lp2;  /* use the next white-space after the string */
            else
               lp1 = lp3;  /* use the 'first "/" after the last "+"' */
         }
      }
   }

   if(*lp1 && *lp1!='/')
   {
      *(lp1++) = 0;       /* terminates 1st argument!! */

      for(; *lp1<=' ' && *lp1; lp1++)
         ;        /* find the next 'non-space' character (or end of line) */
   }


   if(*lp1=='/')     /* if it's a switch... */
   {
      *(lp1++)=0;   /** 'fromfile_binary' is determined by this switch **/
                   /** assigning zero ensures previous arg is terminated **/

      if((toupper(*lp1)=='A' || toupper(*lp1)=='B') &&
         lp1[1]<=' ')  /* must be followed by white space */
      {
         fromfile_binary = toupper(*lp1)=='B';

         lp1++;

         for(; *lp1<=' ' && *lp1; lp1++)
            ;     /* find the next 'non-space' character (or end of line) */
      }
      else
      {
         PrintString(pILLEGALSWITCH);
         PrintAChar(*lp1);
         PrintString(pCRLFLF);
         break;
      }
   }


   lpDest = lp1;  /* it's the 2nd argument!!!!!!!!! */

   while(*lp1 > ' ' && *lp1 != '/')
   {                 /* find the next white space character (or end of line) */
                     /* or a switch character */

      if(*lp1 == '\"')
      {
         lp1++;

         while(*lp1 && *lp1 != '\"') lp1++;
      }

      if(*lp1) lp1++;
   }

   if(*lp1 && *lp1!='/')
   {
      *lp1++ = 0;     /* again, terminate argument */

      for(; *lp1<=' ' && *lp1; lp1++)
         ;        /* find the next 'non-space' character (or end of line) */
   }


   if(*lp1=='/')           /* a switch!!! */
   {
      *(lp1++)=0;     /** 'tofile_binary' is determined by this switch **/
                   /** assigning zero ensures previous arg is terminated **/

      if((toupper(*lp1)=='A' || toupper(*lp1)=='B') &&
         (lp1[1]<=' ' || lp1[1]=='/'))  /* follow with white space or '/' */
      {
         tofile_binary = toupper(*lp1)=='B';

         lp1++;

         for(; *lp1<=' ' && *lp1; lp1++)
            ;                                /* find next non-white space */

         if(*lp1 && *lp1!='/')
         {
            PrintString(pILLEGALARGUMENT);
            PrintString(lp1);
            PrintString(pQCRLFLF);
            break;
         }
         else if(*lp1=='/')  /* a switch!!! */
         {
            lp1++;          /* flows through to the section below... */
         }

      }
//      else
//      {
//         PrintString(pILLEGALSWITCH);
//         PrintAChar(*lp1);
//         PrintString(pCRLFLF);
//         break;
//      }


      if(*lp1)          /* not the end of the string, mind you! */
      {
         if(toupper(*lp1)=='V')
         {
            verify = TRUE;
         }
         else if(toupper(*lp1)=='W')
         {
            waitflag = TRUE;
         }
         else // illegal switch
         {
            *(lp1 + 1) = 0;
            PrintString(pILLEGALSWITCH);
            PrintString(lp1);
            PrintString(pQCRLFLF);
            break;
         }

         lp1++;

         if(*lp1 >' ' && *lp1!='/') /* illegal character following 'V' or 'W' */
         {
            PrintString(pSYNTAXERROR);
            PrintString(lp1 - 2);
            PrintString(pQCRLFLF);
            break;
         }

         while(*lp1 && *lp1<=' ') lp1++;
                  /* find next 'non-white-space' character or end of line */

         if(*lp1 && *lp1!='/')
         {
            PrintErrorMessage(555);
            break;
         }
         else if(*lp1)
         {
            lp1++;

            if(toupper(*lp1)=='V' && !verify)
            {
               verify = TRUE;
            }
            else if(toupper(*lp1)=='W' && !waitflag)
            {
               waitflag = TRUE;
            }
            else // illegal switch
            {
               *(lp1 + 1) = 0;
               PrintString(pILLEGALSWITCH);
               PrintString(lp1);
               PrintString(pQCRLFLF);
               break;
            }

            lp1++;

            if(*lp1 >' ')  /* non-white space following 'V' or 'W' */
            {
               PrintString(pSYNTAXERROR);
               PrintString(lp1 - 2);
               PrintString(pQCRLFLF);
               break;
            }

            while(*lp1 && *lp1<=' ') lp1++;
                     /* find next 'non-white-space' character or end of line */

            if(*lp1)
            {
               PrintErrorMessage(555);
               break;
            }
         }
      }
   }

   if(*lp1)
   {
      PrintString(pILLEGALARGUMENT);
      PrintString(lp1);
      PrintString(pQCRLFLF);
      break;
   }

          /** O.K. - at this time 'lpSrc' and 'lpDest' both       **/
          /** point to the 2 'main' arguments, and any other      **/
          /** switches outside of them have been accounted for... **/


   /** Next, check for a few illegal combinations: **/
   /**  1:  both Wildcards and '+' in source       **/
   /**  2:  '+' in source with no dest file (?)    **/
   /**                                             **/
   /**  for anything else, just try it anyway!     **/

   if(_fstrchr(lpSrc, '+') &&
       (_fstrchr(lpSrc, '?') || _fstrchr(lpSrc, '*')))
   {
      PrintErrorMessage(556);
      break;
   }

   rval = SubmitFilesToCopy(lpSrc, lpDest, verify, def_binary,
                            fromfile_binary, tofile_binary);

   ReportCopyResults();

   END_LEVEL

   if(lpTempBuf)  GlobalFreePtr(lpTempBuf);

   if(!rval && waitflag)  // WE ARE GOING TO WAIT UNTIL DONE COPYING!!
   {
      while(copying && !LoopDispatch()) ; // wait until not copying or
                                          // thread is being terminated!
   }

   return(rval);
}


#pragma code_seg("CMDDate_TEXT","CODE")

BOOL FAR PASCAL CMDDate    (LPSTR lpCmd, LPSTR lpArgs)
{
LPSTR lpD, lp1;
SFTDATE d;
int rval /* , year */;
DWORD dwDosMem;
WORD  wDosSeg;
REALMODECALL rmc;

   lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   if(*lpArgs)
   {
      lpD = lpArgs;
      while(*lpD==' ') lpD++;

      lstrcpy(work_buf, lpD);
      _fstrtrim(work_buf);

      if(atodate(work_buf, (LPSFTDATE)&d))
      {
         PrintErrorMessage(557);
         PrintString(lpD);
         PrintString(pQCRLFLF);

         GlobalFreePtr(lpArgs);
         return(FALSE);
      }

      _fmemset((LPSTR)&rmc, 0, sizeof(rmc));  /* zero everything first. */

      dwDosMem = GlobalDosAlloc(512);        /* length of DOS stack area */
      wDosSeg = HIWORD(dwDosMem);


      rmc.DS  = wDosSeg;
      rmc.ES  = wDosSeg;
      rmc.SS  = wDosSeg;
      rmc.EAX = 0x2b00;                          /* int 21H function 2BH */
      rmc.SP  = 510;                             /* new stack pointer */

      rmc.ECX = d.year;
      rmc.EDX = (((DWORD)d.month)<<8) + d.day;


               /** USE DPMI TO MAKE 'REAL MODE' DOS CALL **/

      _asm
      {
         push es
         mov ax, 0x0300  /* DPMI function - SIMULATE REAL MODE INTERRUPT */
         mov bl, 0x21    /* interrupt number */
         mov bh, 0       /* interrupt ctrlr/A20 line reset */
         mov cx, 0       /* copy 0 words to real mode stack */
         push ss
         pop es
         lea di, WORD PTR rmc /* address of real mode call structure */
                              /* must use LEA because of [BP-xx] address */

         int 0x31        /* DPMI CALL! */

         pop es
      }

      rval = (unsigned char)rmc.EAX;  /* get value of error code */

      if(rval)
      {
         PrintErrorMessage(558);
         PrintString(lpD);
         PrintString(pQCRLFLF);
      }
      else
      {
         SendMessage((HWND)0xffff, WM_TIMECHANGE, NULL, NULL);
      }

      GlobalDosFree(LOWORD(dwDosMem));

      GlobalFreePtr(lpArgs);
      return(FALSE);

   }

   GlobalFreePtr(lpArgs);

   GetSystemDateTime(&d, NULL);

   PrintErrorMessage(741);        // "CURRENT DATE:  "

   PrintString((lp1 = DateStr(&d))?lp1:(LPSTR)pNOTENOUGHMEMORY);

   if(lp1)
   {
      GlobalFreePtr(lp1);

      PrintString(pCRLFLF);
   }
   else
   {
      return(TRUE);
   }

                 /** next section asks for new date **/

   PrintErrorMessage(559);

   GetUserInput(work_buf);

   lp1 = work_buf;

   while(*lp1 && *lp1<=' ') lp1++;
   _fstrtrim(lp1);

   if(*lp1) return(CMDDate(lpCmd, lp1));
   else     return(FALSE);  /* done */
}


#pragma code_seg("CMDDblspace_TEXT","CODE")

BOOL FAR PASCAL CMDDblspace(LPSTR lpCmd, LPSTR lpArgs)
{
union {
   DWORD dw;
   struct {
      BYTE win_major,win_minor;
      BYTE dos_minor,dos_major;
      } ver;
   } uRval;

WORD wRval, wSerial;
BYTE bFirst, bDrivesUsed;




   uRval.dw = GetVersion();
   if(uRval.ver.dos_major < 6)
   {
      PrintErrorMessage(738);    // "?Incorrect DOS Version\r\n\n"
      return(TRUE);
   }

   // determine if DBLSPACE is even present, and get/display version if so


   _asm
   {
      mov ax, 0x4a11    // DBLSPACE API interrupt - AX==0x4a11
      mov bx, 0         // FUNCTION 0 - get version
      int 0x2f

      cmp bx, 0x444d    // 'M' 'D'
      jz dblspace_sig_ok
      mov ax, 1

dblspace_sig_ok:
      mov wRval, ax
      mov wSerial, dx
      mov bFirst, cl
      mov bDrivesUsed, ch
   }


   if(wRval)
   {
      PrintErrorMessage(739); // "?Function not available - DBLSPACE not loaded\r\n\n"
      return(TRUE);
   }

   PrintErrorMessage(740);    // "SFTSHELL - Command Line Pathway to Microsoft(r) DBLSPACE, Release "

   {
    static const char CODE_BASED pFmt[]=
           "%d.%02d\r\n         (using MS-DOS %d.%02d, Windows %d.%02d)\r\n";

      LockCode();
      wsprintf(work_buf, pFmt,
               LOBYTE(wSerial), HIBYTE(wSerial),
               uRval.ver.dos_major, uRval.ver.dos_minor,
               uRval.ver.win_major, uRval.ver.win_minor);
      UnlockCode();
   }

   PrintString(work_buf);


   return(DblspaceCommand(lpArgs));

}




#pragma code_seg("CMDDde_TEXT","CODE")


BOOL FAR PASCAL CMDDde     (LPSTR lpCmd, LPSTR lpArgs)
{
LPSTR lp1, lp2, lp3;
WORD wOption, wLen, w;
DWORD dw;
HSZ hsz1;
HDDEDATA hData;
HCONV hConv;
HCURSOR hCursor;
BOOL rval;


   if(!idDDEInst)
   {
      PrintErrorMessage(560);
      SetEnvString(pDDERESULT,pERROR);
      return(TRUE);
   }

   lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   /* STEP 1:  get next 'option' in command line */

   for(lp1=lpArgs; *lp1 && *lp1<=' '; lp1++)
      ;  /* find first non-space */

   for(lp2=lp1; *lp2>' '; lp2++)
      ;  /* find next space */

   if(*lp2)
   {
      *(lp2++)=0;
   }

   while(*lp2 && *lp2<=' ') lp2++;  /* find next 'non-space' */


   for(wOption=0; wOption<wNDdeOption; wOption++)
   {
      if(_fstrnicmp(lp1, pDdeOption[wOption], lstrlen(lp1))==0)
      {
         break;
      }
   }

   if(wOption>=wNDdeOption)
   {
      PrintString(pSYNTAXERROR);
      PrintString(lpArgs);
      PrintString(pQCRLFLF);
      SetEnvString(pDDERESULT,pERROR);

      GlobalFreePtr(lpArgs);
      return(TRUE);
   }


   /* at this point 'lp2' points to the remaining args on the string */


   switch(wOption)
   {
      case CMDDDE_REGISTER:
      case CMDDDE_UNREGISTER:
      case CMDDDE_INITIATE:
         rval = CMDDdeSub1(lp2, wOption);
         break;


      case CMDDDE_TERMINATE:
         /* in this case lp2 should point to a valid conversation handle */
         /* 8 hex digits followed by :SERVER|TOPIC (for info's sake)     */

         lstrcpy(work_buf, lp2);

         if(!(dw = CMDDdeGetConvValue(work_buf)))
         {
            PrintString(pILLEGALARGUMENT);
            PrintString(lp2);
            PrintString(pQCRLFLF);
            SetEnvString(pDDERESULT,pERROR);
            rval = TRUE;
            break;
         }

         hConv = (HCONV)dw;

         lp3 = work_buf + 9;
         while(*lp3>' ') lp3++;

//         *(lp3++) = 0;

         while(*lp3 && *lp3<=' ') lp3++;  /* find next non-space or end */

         if(*lp3)
         {
            PrintString(pEXTRANEOUSARGSIGNORED);
            PrintString(lp3);
            PrintString(pQCRLFLF);
         }

         if(!lpDdeDisconnect(hConv))
         {
            SetEnvString(pDDERESULT,pERROR);
         }
         else
         {
            SetEnvString(pDDERESULT,pOK);
         }

         rval = FALSE;
         break;


      case CMDDDE_EXECUTE:
         lstrcpy(work_buf, lp2);
         if(lp2[8]!=':')
         {
            PrintString(pILLEGALARGUMENT);
            PrintString(lp2);
            PrintString(pQCRLFLF);
            SetEnvString(pDDERESULT,pERROR);
            rval = TRUE;
            break;
         }

         lp3 = work_buf;
         while(*lp3>' ') lp3++;

         while(*lp3 && *lp3<=' ') lp3++;  /* find next non-space or end */

         if(!*lp3)
         {
            PrintErrorMessage(561);
            SetEnvString(pDDERESULT,pERROR);
            rval = TRUE;
            break;
         }

         if(!(dw = CMDDdeGetConvValue(work_buf)))
         {
            PrintString(pILLEGALARGUMENT);
            PrintString(lp2);
            PrintString(pQCRLFLF);
            SetEnvString(pDDERESULT,pERROR);
            rval = TRUE;
            break;
         }

         hConv = (HCONV)dw;

         do
         {
            hCursor = MySetCursor(LoadCursor(NULL, IDC_WAIT));

            hData = lpDdeClientTransaction(lp3, lstrlen(lp3)+1, hConv, NULL,
                                           CF_TEXT, XTYP_EXECUTE,5000L,NULL);

            MySetCursor(hCursor);

         } while(CMDDdeErrorCheck(TRUE));  /* verifies results */
                                          /* sets DDE_RESULT env variable */
         rval = !hData;
         break;




      case CMDDDE_ADVISE:
      case CMDDDE_UNADVISE:
      case CMDDDE_REQUEST:
      case CMDDDE_POKE:

         lstrcpy(work_buf, lp2);
         if(lp2[8]!=':')
         {
            PrintString(pILLEGALARGUMENT);
            PrintString(lp2);
            PrintString(pQCRLFLF);
            SetEnvString(pDDERESULT,pERROR);
            rval = TRUE;
            break;
         }

         lp3 = work_buf;
         while(*lp3>' ') lp3++;
         if(*lp3) *(lp3++) = 0;

         if(!(dw = CMDDdeGetConvValue(work_buf)))
         {
            PrintString(pILLEGALARGUMENT);
            PrintString(lp2);
            PrintString(pQCRLFLF);
            SetEnvString(pDDERESULT,pERROR);
            rval = TRUE;
            break;
         }

         hConv = (HCONV)dw;

         while(*lp3 && *lp3<=' ') lp3++;  /* find next non-space or end */

         if(!*lp3)         /* oh no! there's no item here */
         {
            PrintErrorMessage(562);
            SetEnvString(pDDERESULT,pERROR);
            rval = TRUE;
            break;
         }

         lp1 = lp3;  /* 'lp1' now points to the item */

         while(*lp3>' ') lp3++;  /* find end of 'item' */
         if(*lp3) *(lp3++) = 0;  /* terminate string */

         while(*lp3 && *lp3<=' ') lp3++;  /* find next non-space or end */

         if(*lp3 && wOption!=CMDDDE_POKE)  /* too many arguments */
         {
            PrintErrorMessage(563);
            PrintString(lp3);
            PrintString(pQCRLFLF);
         }

         do
         {
            hCursor = MySetCursor(LoadCursor(NULL, IDC_WAIT));

            hsz1 = lpDdeCreateStringHandle(idDDEInst, lp1, CP_WINANSI);

            if(wOption==CMDDDE_POKE)
            {
               if(!*lp3)
                  lp3 = GetEnvString(lp1);   /* get value of 'lp1's string */

               if(!lp3) lp3="";

               wLen = lstrlen(lp3) + 1;
               w = XTYP_POKE;
            }
            else
            {
               lp3 = NULL;
               wLen = 0;

               if(wOption==CMDDDE_ADVISE)        w = XTYP_ADVSTART;
               else if(wOption==CMDDDE_UNADVISE) w = XTYP_ADVSTOP;
               else if(wOption==CMDDDE_REQUEST)  w = XTYP_REQUEST;
               else
               {
                  PrintErrorMessage(564);
                  SetEnvString(pDDERESULT,pERROR);

                  GlobalFreePtr(lpArgs);
                  return(TRUE);
               }
            }

            hData = lpDdeClientTransaction(lp3, wLen, hConv, hsz1,
                                           CF_TEXT, w, 5000L, NULL);

            MySetCursor(hCursor);

            if(wOption==CMDDDE_POKE)
            {
               if(hData==(HDDEDATA)DDE_FACK)  /* acknowledged by server */
               {
                  SetEnvString(pDDERESULT,pOK);
                  break;  /* no error! */
               }
            }

         } while(CMDDdeErrorCheck(TRUE));  /* verifies results */
                                       /* sets DDE_RESULT env variable */

         if(wOption==CMDDDE_REQUEST)  /* 'hData' contains result! */
         {
            if(hData && !_fstricmp(GetEnvString(pDDERESULT),pOK))
            {
               w = (WORD)lpDdeGetData(hData, NULL, 0, 0);  /* get size */
               if(w)
               {
                  lp3 = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT, w+1);
                  if(!lp3)
                  {
                     PrintString(pNOTENOUGHMEMORY);
                     SetEnvString(pDDERESULT,pERROR);
                     rval = TRUE;
                     break;
                  }

                  lpDdeGetData(hData, lp3, w + 1, 0);

                  SetEnvString(lp1, lp3);
                  GlobalFreePtr(lp3);
               }
               else
                  DelEnvString(lp1);  /* no value on 'zero length' */
            }
            else
               DelEnvString(lp1);  /* no value on error */
         }

         rval = _fstricmp(GetEnvString(pDDERESULT),pOK) != 0;

         break;

   }


   GlobalFreePtr(lpArgs);
   return(rval);

}


BOOL FAR PASCAL CMDDdeErrorCheck(BOOL wEnvVarFlag)
{
WORD wError;
static char pDDEWARNING[]="** DDE WARNING **";


   wError = lpDdeGetLastError(idDDEInst);

   switch(wError)
   {
      case DMLERR_ADVACKTIMEOUT:
      case DMLERR_DATAACKTIMEOUT:
      case DMLERR_EXECACKTIMEOUT:
      case DMLERR_POKEACKTIMEOUT:
      case DMLERR_UNADVACKTIMEOUT:

         if(MessageBox(hMainWnd,"?DDE Timeout Error during transaction",
                       pDDEWARNING, MB_RETRYCANCEL | MB_ICONHAND)
            ==IDRETRY)
         {
            return(TRUE);
         }

         if(wEnvVarFlag)
         {
            SetEnvString(pDDERESULT,"TIMEOUT");
         }

         break;


      case DMLERR_BUSY:

         if(MessageBox(hMainWnd,"?DDE Warning - Other application is BUSY",
                       pDDEWARNING, MB_RETRYCANCEL | MB_ICONHAND)
            ==IDRETRY)
         {
            return(TRUE);
         }

         if(wEnvVarFlag)
         {
            SetEnvString(pDDERESULT,"BUSY");
         }
         break;


      case DMLERR_DLL_NOT_INITIALIZED:
      case DMLERR_POSTMSG_FAILED:
      case DMLERR_REENTRANCY:

         PrintErrorMessage(565);
         if(wEnvVarFlag)
         {
            SetEnvString(pDDERESULT,pERROR);
         }

         break;


      case DMLERR_MEMORY_ERROR:
         PrintString(pNOTENOUGHMEMORY);
         if(wEnvVarFlag)
         {
            SetEnvString(pDDERESULT,pERROR);
         }

         break;


      case DMLERR_SERVER_DIED:
         PrintErrorMessage(566);
         if(wEnvVarFlag)
         {
            SetEnvString(pDDERESULT,pERROR);
         }

         break;


      case DMLERR_INVALIDPARAMETER:
         PrintErrorMessage(567);
         if(wEnvVarFlag)
         {
            SetEnvString(pDDERESULT,pERROR);
         }

         break;


      case DMLERR_NO_ERROR:

         if(wEnvVarFlag)
         {
            SetEnvString(pDDERESULT,pOK);
         }

         break;


      case DMLERR_NOTPROCESSED:
         PrintErrorMessage(568);
         if(wEnvVarFlag)
         {
            SetEnvString(pDDERESULT,"NAK");
         }

         break;

      default:
         PrintErrorMessage(569);
         if(wEnvVarFlag)
         {
            SetEnvString(pDDERESULT,pERROR);
         }

         break;


   }

   return(FALSE);

}


LPSTR FAR PASCAL CMDDdeFindStr(LPSTR lpList, LPSTR lpStr)
{
LPSTR lp1, lp2;
int i;



   lp1 = lpList;

   while(lp1 && *lp1)
   {
      lp2 = _fstrchr(lp1, ',');
      if(lp2) *lp2 = 0;    /* temporary */

      i = _fstricmp(lp1, lpStr);

      if(lp2) *lp2 = ',';  /* restore it */

      if(!i) return(lp1);

      if(lp2) lp1 = lp2+1;
      else    break;
   }

   return(NULL);
}


BOOL FAR PASCAL CMDDdeSub1(LPSTR lpArgs, WORD wOption)
{
LPSTR lp1, lp2, lp3, lp4;
//WORD wLen;
HSZ hsz1,hsz2;
HCONV hConv;
char tbuf[256];
/* HDDEDATA hData; */



   lp1 = lpArgs;          // lpArgs points to a 'GMEM_MOVEABLE' memory block

   while(*lp1 && *lp1<=' ') lp1++;
   if(!*lp1)
   {
      SetEnvString(pDDERESULT, pERROR);
      return(TRUE);
   }

   lstrcpy(tbuf, lp1);


   _fstrupr(tbuf); // convert to UPPER CASE first...
   lp2 = _fstrchr(tbuf, '|');

   if(lp2)
   {
      lp3 = lp2 + 1;
   }
   else
   {
      lp3 = tbuf;
   }

   while(*lp3>' ') lp3++;
   if(*lp3) *(lp3++) = 0;

   while(*lp3 && *lp3<=' ') lp3++;  /* find next non-space or end */

   if(*lp3)
   {
      PrintString(pEXTRANEOUSARGSIGNORED);
      PrintString(lp3);
      PrintString(pQCRLFLF);
   }


   // at this point 'tbuf' contains the DDE "SERVER|TOPIC" string, and
   // 'lp2' points to the '|' within the string, or NULL if there isn't one



   if(wOption==CMDDDE_REGISTER)
   {
      if(CMDDdeFindStr(lp3 = GetEnvString(pDDESERVERTOPICLIST), tbuf))
      {
         PrintErrorMessage(570);
         SetEnvString(pDDERESULT, pERROR);
         return(TRUE);
      }

      if(lp3)
      {
         lstrcpy(work_buf, lp3);
         lstrcat(work_buf, pCOMMA);
      }
      else
      {
         *work_buf = 0;
      }

      lstrcat(work_buf, tbuf);

      SetEnvString(pDDESERVERTOPICLIST, work_buf);


      if(!lp2)
      {
         lp2 = tbuf;
         lp1 = pDEFSERVERNAME;
      }
      else
      {
         lp1 = tbuf;
         *(lp2++) = 0;  /* separates the 2 'halves' */
      }

      // at this point, 'lp1' points to the SERVER name, and 'lp2' to the
      // TOPIC name.

      if(!CMDDdeFindStr(lp3 = GetEnvString(pDDESERVERLIST),lp1))
      {
         hsz1 = lpDdeCreateStringHandle(idDDEInst, lp1, CP_WINANSI);

         if(!lpDdeNameService(idDDEInst, hsz1, NULL, DNS_REGISTER))
         {
            SetEnvString(pDDERESULT,pERROR);  // unable to register!
            return(TRUE);
         }

         if(lp3)
         {
            lstrcpy(work_buf, lp3);
            lstrcat(work_buf, pCOMMA);
         }
         else
         {
            *work_buf = 0;
         }

         lstrcat(work_buf, lp1);

         SetEnvString(pDDESERVERLIST, work_buf);
      }

      SetEnvString(pDDERESULT,pOK);

      return(FALSE);

   }
   else if(wOption==CMDDDE_UNREGISTER)
   {
      /** the argument must be 'server|topic' or it fails! **/

      if(lstrlen(tbuf)<3 ||  /* the theoretical minimum length */
         !(lp2 = _fstrchr(tbuf, '|')) )
      {
         PrintString(pILLEGALARGUMENT);
         PrintString(lpArgs);
         PrintString(pQCRLFLF);

         SetEnvString(pDDERESULT, pERROR);
         return(TRUE);  /* akk! syntax error, dude! */
      }


      lp1 = tbuf;
      lp2 = work_buf;

      lp3 = GetEnvString(pDDESERVERTOPICLIST);

      lstrcpy(lp2, lp3);  /* make a copy of the current value */

      lp3 = CMDDdeFindStr(lp2, lp1);  /* find the SERVER|TOPIC in string */

      if(lp3)
      {
         lp1 = _fstrchr(lp3, ',');

         if(!lp1) *lp3 = 0;  /* terminate string right here */
         else     lstrcpy(lp3, lp1 + 1);  /* remove this entry only */

         SetEnvString(pDDESERVERTOPICLIST, lp2);
      }

            /* if no topics left, remove entry from 'SERVER' list */



      lp1 = _fstrchr(tbuf, '|');  /* find the 'topic' */
      if(lp1)
      {
         lp1[1] = 0;   // terminate with '0' for now, including '|' at end

         _fstrupr(lp2);  /* ensure it is UPPER CASE */
         _fstrupr(tbuf);

         while(lp4 = _fstrstr(lp2, tbuf))
         {
           if(lp4!=lp2 && *(lp4 - 1)!=',') lp2 = lp4 + 1;  /* keep looking */
           else                            break;  /* found! I am done now */
         }

         if(!lp4)       /* no 'proper' occurrence of this service exists */
         {
            lp1 = _fstrchr(tbuf, '|');
            if(lp1)  *lp1 = 0;              // get rid of the "|" at the end

            lp2 = work_buf;

            lp3 = GetEnvString(pDDESERVERLIST);

            lstrcpy(lp2, lp3);  /* make a copy of the current value */
            _fstrupr(lp2);

            lp3 = CMDDdeFindStr(lp2, tbuf);  /* find the SERVER in string */

            if(lp3)
            {
               lp1 = _fstrchr(lp3, ',');

               if(!lp1) *lp3 = 0;  /* terminate string right here */
               else     lstrcpy(lp3, lp1 + 1);  /* remove this entry only */

               SetEnvString(pDDESERVERLIST, lp2);
            }

            if(_fstricmp(tbuf, pDEFSERVERNAME)) /* never un-register this! */
            {
               hsz1 = lpDdeCreateStringHandle(idDDEInst, tbuf, CP_WINANSI);

               lpDdeNameService(idDDEInst, hsz1, NULL, DNS_UNREGISTER);
            }
         }
      }

      SetEnvString(pDDERESULT,pOK);

      return(FALSE);
   }
   else if(wOption==CMDDDE_INITIATE)
   {
      lp1 = tbuf;   // 'lp1' is server, 'lp2' is topic

      if(!lp2)
      {
         lp2 = "";
      }
      else
      {
         *(lp2++) = 0;  /* separates the 2 'halves' */
      }


      if(*lp1)  hsz1 = lpDdeCreateStringHandle(idDDEInst, lp1, CP_WINANSI);
      else      hsz1 = NULL;

      if(*lp2)  hsz2 = lpDdeCreateStringHandle(idDDEInst, lp2, CP_WINANSI);
      else      hsz2 = NULL;

      hConv = lpDdeConnect(idDDEInst, hsz1, hsz2, NULL);

      if(!hConv)
      {
         PrintErrorMessage(571);
         PrintString(lp1);
         PrintAChar('|');
         PrintString(lp2);
         PrintString(pQCRLFLF);
      }
      else
      {
         wsprintf(work_buf,"%08lx",(DWORD)hConv);
         lstrcat(work_buf,":");
         lstrcat(work_buf,lp1);
         lstrcat(work_buf,"|");
         lstrcat(work_buf,lp2);

         SetEnvString(pDDERESULT,work_buf);  /* establish result string! */

         return(FALSE);
      }

          /* the conversation handle is now in 'DDE_RESULT' as */
       /* 8 hex digits followed by :SERVER|TOPIC (for info's sake) */
   }



   SetEnvString(pDDERESULT,pERROR);

   return(TRUE);  /* if it gets here, problem */

}

DWORD FAR PASCAL CMDDdeGetConvValue(LPSTR lp1)
{
DWORD dw;
WORD w;
LPSTR lp2;

             /* lp1 points to '########:server|topic' */

   if(lstrlen(lp1)<11 ||  /* the theoretical minimum length */
      lp1[8]!=':' ||
      !(lp2 = _fstrchr(lp1 + 9, '|')) )
   {
      return(NULL);                         /* akk! syntax error, dude! */
   }

   for(w=0, dw=0; w<8; w++)
   {
      if(lp1[w]>='A' && lp1[w]<='F')
      {
         dw = dw * 16 + lp1[w] - 'A' + 10;
      }
      else if(lp1[w]>='a' && lp1[w]<='f')
      {
         dw = dw * 16 + lp1[w] - 'a' + 10;
      }
      else if(lp1[w]>='0' && lp1[w]<='9')
      {
         dw = dw * 16 + lp1[w] - '0';
      }
      else
      {
         return(NULL);  /* akk! syntax error, dude! */
      }
   }

   return(dw);

}


//#pragma alloc_text (CMDDebug_TEXT,CMDDebug)
#pragma code_seg("CMDDebug_TEXT","CODE")


BOOL FAR PASCAL CMDDebug    (LPSTR lpCmd, LPSTR lpArgs)
{
BOOL rval;


   lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   rval = CMDRunProgram(lpCmd, lpArgs, SW_SHOWNORMAL);
   GlobalFreePtr(lpArgs);

   return(rval);
}


#pragma code_seg("CMDDel_TEXT","CODE")


BOOL FAR PASCAL CMDDel     (LPSTR lpCmd, LPSTR lpArgs)
{
LPPARSE_INFO lpParseInfo=(LPPARSE_INFO)NULL;
LPTREE lpTree = NULL;
LPDIR_INFO lpDirInfo=(LPDIR_INFO)NULL, lpDI;
BOOL rval = TRUE, AskFlag = FALSE, YesFlag = FALSE, SubDirFlag = FALSE,
     HiddenFlag = FALSE;
LPSTR lpPath, lpCurPath, lpDirPath, lpDirSpec, lp1, lp2;
WORD i, ctr, nFiles, wRecurse, wNRecurse, wTreeLevel, wTreeIndex;
DWORD dwTotalFiles, dwTotalFound;
WORD wLevelArray[64];
char tbuf[16];


   if(!(lpDirPath = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                          sizeof(work_buf))))
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   lpDirSpec = lpDirPath + (sizeof(work_buf) / 4);
   lpCurPath = lpDirPath + (sizeof(work_buf) / 2);


   INIT_LEVEL

   lpParseInfo = CMDLineParse(lpArgs);

   if(CMDCheckParseInfo(lpParseInfo, "HSQY", 1 | CMD_EXACT))
   {
      GlobalFreePtr(lpParseInfo);
      GlobalFreePtr(lpDirPath);
      return(TRUE);
   }

   if((int)(lpParseInfo->n_switches)>0)
   {
      for(i=0; i<lpParseInfo->n_switches; i++)
      {
         if(toupper(*((LPSTR)(lpParseInfo->sbpItemArray[i])))=='H')
         {
            HiddenFlag = TRUE;
         }
         else if(toupper(*((LPSTR)(lpParseInfo->sbpItemArray[i])))=='Y')
         {
            YesFlag = TRUE;
         }
         else if(toupper(*((LPSTR)(lpParseInfo->sbpItemArray[i])))=='S')
         {
            SubDirFlag = TRUE;
         }
         else if(toupper(*((LPSTR)(lpParseInfo->sbpItemArray[i])))=='Q')
         {
            AskFlag = TRUE;
         }
      }
   }


   ReDirectOutput(lpParseInfo,TRUE);  /* redirect output if needed */

   lpPath = (LPSTR)(lpParseInfo->sbpItemArray[lpParseInfo->n_switches]);

   if(QualifyPath(lpDirPath, lpPath, TRUE))  break;

         /* determine if user specified '*.*' or any derivative */

   lp1 = _fstrrchr(lpDirPath, '\\'); /* get the 'final \' in the string */
   if(lp1)
   {
      lstrcpy(lpDirSpec, lp1 + 1); /* copy remaining string to 'lpDirSpec' */
      *(lp1 + 1) = 0;             /* terminate string after last backslash */
   }
   else
   {
      lstrcpy(lpDirSpec, lpDirPath);
      *lpDirPath = 0;
   }

   lp1 = lpDirSpec;

   if(lp2 = _fstrrchr(lp1, '.'))     /* find extension */
   {
    BOOL wild=TRUE;

      lp2++;                         /* points to actual extension */

      for(i=0; lp2[i] && i<3; i++)
      {
         if(lp2[i]=='*')
         {
            break;
         }
         if(lp2[i]!='?')
         {
            wild = FALSE;
            break;
         }
      }

      if(wild)
      {
         for(i=0; lp1[i] && lp1[i]!='.' && i<8; i++)
         {
            if(lp1[i]=='*')
            {
               break;
            }
            if(lp1[i]!='?')
            {
               wild = FALSE;
               break;
            }
         }
         if(wild && !YesFlag)
         {
            MessageBeep(MB_ICONQUESTION);  /* why not? */

            PrintErrorMessage(742);  // "Delete \""
            PrintString(lpDirPath);
            PrintString("*.*");
            PrintErrorMessage(743);  // "\"... are you sure?"

            _fmemset(work_buf, 0, sizeof(work_buf));
            GetUserInput(work_buf);

            _fstrtrim(work_buf);

            if(*work_buf)
            {
               _fstrupr(work_buf);
            }

            if(!*work_buf || ctrl_break ||
               _fstrncmp(work_buf,"YES",lstrlen(work_buf))!=0)
            {
               PrintErrorMessage(572);
               rval = FALSE;

               if(ctrl_break && !BatchMode) ctrl_break = FALSE;

               break;
            }

         }
      }
   }


   if(SubDirFlag)
   {
      lstrcpy(work_buf, lpDirPath);

      if(!(lpTree = GetDirTree(work_buf)))
      {
         if(ctrl_break)
         {
            PrintString(pCTRLC);
            PrintString(pCRLFLF);
         }
         else
         {
            PrintErrorMessage(576);
         }

         break;
      }

      if(!(*lpTree->szDir))  /* empty tree list? */
      {
         wNRecurse = 1;
         SubDirFlag = FALSE;
         GlobalFreePtr(lpTree);
         lpTree = NULL;
      }
      else
      {
         wNRecurse = 0xffff;  /* for now, it's a bit easier... */
         wTreeLevel = 0;
         wTreeIndex = 0;

         wLevelArray[wTreeLevel] = wTreeIndex;
      }
   }
   else
   {
      wNRecurse = 1;   /* always go at least 1 time; helps detect errors */
      lpTree = NULL;
   }

   dwTotalFiles = dwTotalFound = 0;

   for(rval=FALSE, wRecurse=0; !ctrl_break && wRecurse<wNRecurse; wRecurse++)
   {
                     /** Get directory list buffer **/

      if(wRecurse && SubDirFlag)
      {
         lstrcpy(lpCurPath, lpDirPath);

         for(ctr=0; ctr<=wTreeLevel; ctr++)  /* build current path */
         {
            lstrcat(lpCurPath, lpTree[wLevelArray[ctr]].szDir);
            lstrcat(lpCurPath, pBACKSLASH);
         }

         lstrcat(lpCurPath, lpDirSpec);

         if(lpTree[wTreeIndex].wChild)  /* a child element exists? */
         {
            wLevelArray[wTreeLevel++] = wTreeIndex;
            wTreeIndex = lpTree[wTreeIndex].wChild;

            wLevelArray[wTreeLevel] = wTreeIndex;
         }
         else if(lpTree[wTreeIndex].wSibling)
         {
            wTreeIndex = lpTree[wTreeIndex].wSibling;
            wLevelArray[wTreeLevel] = wTreeIndex;
         }
         else
         {
            while(wTreeLevel>0 && !lpTree[wTreeIndex].wSibling)
            {
               wTreeIndex = wLevelArray[--wTreeLevel];
            }

            if(!wTreeLevel && !lpTree[wTreeIndex].wSibling)
            {
               wNRecurse = wRecurse; /* end after this one! */
            }
            else
            {
               wTreeIndex = lpTree[wTreeIndex].wSibling;
               wLevelArray[wTreeLevel] = wTreeIndex;
            }
         }

      }
      else
      {        /* first time through - current dir (always) */

         lstrcpy(lpCurPath, lpDirPath);
         lstrcat(lpCurPath, lpDirSpec);

      }


      nFiles = GetDirList(lpCurPath, (~_A_RDONLY&~_A_SUBDIR&~_A_VOLID)&0xff,
                          (LPDIR_INFO FAR *)&lpDirInfo, work_buf);

      if(nFiles==DIRINFO_ERROR)
      {
         rval = TRUE;
         break;
      }
      else
      {
         lp1 = work_buf + strlen(work_buf);

         if(lp1!=work_buf && *(lp1 - 1)!='\\')
         {
            *(lp1++) = '\\';        /* ensure it ends in a backslash */
         }

         dwTotalFound += nFiles;

         for(lpDI = lpDirInfo, i=0; i<nFiles; i++, lpDI++)
         {
            _fmemset(lp1, 0, sizeof(lpDI->fullname) + 2);

            _fstrncpy(lp1, lpDI->fullname, sizeof(lpDI->fullname));

            if(lpDI->attrib & _A_RDONLY)
            {
               PrintErrorMessage(872);  // "?Read-Only file \""
               PrintString(work_buf);
               PrintAChar('\"');
               PrintString(pCRLF);
            }
            else if(HiddenFlag && lpDI->attrib & _A_HIDDEN)
            {
               PrintErrorMessage(873);  // "?Hidden file \""
               PrintString(work_buf);
               PrintAChar('\"');
               PrintString(pCRLF);
            }
            else
            {
               PrintErrorMessage(744);   // "Deleting "
               PrintString(work_buf);

               if(AskFlag)
               {
                  PrintString("(N/Y)? ");

                  _fmemset(tbuf, 0, sizeof(tbuf));
                  GetUserInput(tbuf);

                  if(ctrl_break) break;

                  if(toupper(*tbuf)!='Y')
                  {
                     *lp1 = 0;
                     continue;
                  }

               }
               else
               {
                  PrintString(pCRLF);
               }

               rval |= (MyUnlink(work_buf)!=0);

               dwTotalFiles++;
            }

            *lp1 = 0;
         }
      }

      if(lpDirInfo)    GlobalFreePtr(lpDirInfo);

      lpDirInfo = NULL;
   }


   if(ctrl_break && !BatchMode) ctrl_break = FALSE;

   if(!dwTotalFound)
   {
      PrintErrorMessage(548);
   }
   else
   {
      if(!dwTotalFiles)
      {
       static const char CODE_BASED pFmt[]=
                                                  "No files deleted\r\n\n";

         LockCode();
         lstrcpy(work_buf, pFmt);
         UnlockCode();
      }
      else if(dwTotalFiles!=1)
      {
       static const char CODE_BASED pFmt[]=
                                       "Total of %lu files deleted\r\n\n";
         LockCode();
         wsprintf(work_buf, pFmt, dwTotalFiles);
         UnlockCode();
      }
      else
      {
       static const char CODE_BASED pFmt[]=
                                                  "1 file deleted\r\n\n";

         LockCode();
         lstrcpy(work_buf, pFmt);
         UnlockCode();
      }

      PrintString(work_buf);
   }



   END_LEVEL

   if(lpDirInfo)    GlobalFreePtr(lpDirInfo);

   if(lpTree)       GlobalFreePtr(lpTree);

   if(lpParseInfo)
   {
      ReDirectOutput(lpParseInfo,FALSE);
      GlobalFreePtr(lpParseInfo);
   }

   if(lpDirPath)       GlobalFreePtr(lpDirPath);

   return(CMDErrorCheck(rval));
}


//BOOL FAR PASCAL MyRmDir(LPCSTR lpName)
//{
//BOOL rval = 0;
//
//   _asm
//   {
//      push ds
//      mov ax, 0x3a00
//
//      lds dx, lpName
//
//      call DOS3Call
//
//      jnc no_error
//
//      mov rval, ax
//
//no_error:
//
//   }
//
//   return(rval);
//}


BOOL NEAR PASCAL AskAndRemoveDirectory(LPCSTR lpCurPath, BOOL AskFlag)
{
BOOL rval=FALSE;
char tbuf[16];


   PrintString("Removing ");
   PrintString(lpCurPath);

   if(AskFlag)
   {
      PrintString("(N/Y)? ");

      _fmemset(tbuf, 0, sizeof(tbuf));
      GetUserInput(tbuf);

      if(ctrl_break) return(TRUE);
   }
   else
   {
      PrintString(pCRLF);
   }

   if(!AskFlag || toupper(*tbuf)!='Y')
   {
      if(My_Rmdir((LPSTR)lpCurPath))
      {
         PrintString("?Warning - \"");
         PrintString(lpCurPath);
         PrintString("\" not removed\r\n");

         rval = TRUE;
      }
   }

   return(rval);
}


BOOL FAR PASCAL CMDDeltree (LPSTR lpCmd, LPSTR lpArgs)
{
LPPARSE_INFO lpParseInfo=(LPPARSE_INFO)NULL;
LPTREE lpTree = NULL;
LPDIR_INFO lpDirInfo=NULL, lpDirInfo0=NULL, lpDI, lpDI0;
BOOL rval = TRUE, AskFlag = FALSE, YesFlag = FALSE;
LPSTR lpPath, lpCurPath, lpDirPath, lpDirPath0, lp1 /*, lp2 */;
WORD i, i0, ctr, nFiles, nFiles0, wRecurse, wNRecurse, wTreeLevel, wTreeIndex;
DWORD dwTotalFiles, dwTotalFound;
WORD wLevelArray[64];
char tbuf[16];


   if(!(lpDirPath = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                          sizeof(work_buf))))
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   lpDirPath0 = lpDirPath + (sizeof(work_buf) / 4);
   lpCurPath = lpDirPath + (sizeof(work_buf) / 2);


   INIT_LEVEL

   lpParseInfo = CMDLineParse(lpArgs);

   if(CMDCheckParseInfo(lpParseInfo, "QY", 1 | CMD_EXACT))
   {
      GlobalFreePtr(lpParseInfo);
      GlobalFreePtr(lpDirPath);
      return(TRUE);
   }

   if((int)(lpParseInfo->n_switches)>0)
   {
      for(i=0; i<lpParseInfo->n_switches; i++)
      {
         if(toupper(*((LPSTR)(lpParseInfo->sbpItemArray[i])))=='Y')
         {
            YesFlag = TRUE;
         }
         else if(toupper(*((LPSTR)(lpParseInfo->sbpItemArray[i])))=='Q')
         {
            AskFlag = TRUE;
         }
      }
   }


   ReDirectOutput(lpParseInfo,TRUE);  /* redirect output if needed */

   lpPath = (LPSTR)(lpParseInfo->sbpItemArray[lpParseInfo->n_switches]);

   if(QualifyPath(lpDirPath0, lpPath, FALSE))  break;


   // To conform with MS-DOS version, we must do a directory using the
   // path spec in 'lpDirPath' (without '*.*' added when not there)

   lstrcpy(lpCurPath, lpDirPath0);

        // use 'GetDirList0()' which lets me NOT 'qualify' sub-dirs!

   nFiles0 = GetDirList0(lpCurPath, (~_A_RDONLY&~_A_VOLID)&0xff,
                         (LPDIR_INFO FAR *)&lpDirInfo0, lpDirPath0, FALSE, NULL);

   lp1 = lpDirPath0 + lstrlen(lpDirPath0);
   if(*(lp1 - 1)!='\\')
   {
      *(lp1++) = '\\';
      *lp1 = 0;
   }

   if(nFiles0==DIRINFO_ERROR)
   {
      rval = TRUE;
      break;
   }


   dwTotalFiles = 0;
   dwTotalFound = nFiles0;


   for(lpDI0 = lpDirInfo0, i0=0; i0<nFiles0; i0++, lpDI0++)
   {
      lstrcpy(lpDirPath, lpDirPath0);

      lp1 = lpDirPath + lstrlen(lpDirPath);
      if(*(lp1 - 1)!='\\')
      {
         *(lp1++) = '\\';
      }

      _fstrncpy(lp1, lpDI0->fullname, sizeof(lpDI0->fullname));
      lp1[sizeof(lpDI0->fullname)] = 0;

      if(!_fstricmp(lp1, ".") || !_fstricmp(lp1, ".."))
      {
         continue;
      }

      if(!YesFlag)
      {
       static char CODE_BASED szMsg1[]= "Delete \"";
       static char CODE_BASED szMsg2[]= "\" and all sub-directories? ";

         MessageBeep(MB_ICONQUESTION);  /* why not? */

         PrintString(szMsg1);
         PrintString(lpDirPath);

         if(!(lpDI0->attrib & _A_SUBDIR))  // not a sub-directory!
         {
            PrintString("? ");
         }
         else
         {
            PrintString(szMsg2);
         }

         _fmemset(work_buf, 0, sizeof(work_buf));
         GetUserInput(work_buf);

         _fstrtrim(work_buf);

         if(*work_buf)
         {
            _fstrupr(work_buf);
         }

         if(!*work_buf || ctrl_break ||
            _fstrncmp(work_buf,"YES",lstrlen(work_buf))!=0)
         {
            if(ctrl_break && !BatchMode)
            {
               PrintErrorMessage(572);
               ctrl_break = FALSE;
               break;
            }

            continue;
         }
      }


      lstrcpy(work_buf, lpDirPath);

      if(!(lpDI0->attrib & _A_SUBDIR))  // not a sub-directory!
      {
         lpTree = NULL;
         lpDirInfo = NULL;

         PrintErrorMessage(744);   // "Deleting "
         PrintString(work_buf);

         if(AskFlag && YesFlag)    // 1st part did *NOT* ask yet!
         {
            PrintString("(N/Y)? ");

            _fmemset(tbuf, 0, sizeof(tbuf));
            GetUserInput(tbuf);

            if(ctrl_break)
            {
               break;
            }

            if(toupper(*tbuf)!='Y')
            {
               continue;
            }
         }
         else
         {
            PrintString(pCRLF);
         }

         rval |= (MyUnlink(work_buf)!=0);

         *lp1 = 0;

         dwTotalFiles++;

         continue;
      }


      // HERE the file is a sub-directory!!  Therefore, I explode the thing
      // down and delete all files 'below' it.

      if(!(lpTree = GetDirTree(work_buf)))
      {
         if(ctrl_break)
         {
            PrintString(pCTRLC);
            PrintString(pCRLFLF);
         }
         else
         {
            PrintErrorMessage(576);
         }

         break;
      }

      wTreeLevel = 0;
      wTreeIndex = 0;

      if(!(*lpTree->szDir))  /* empty tree list? */
      {
         wNRecurse = 1;
         GlobalFreePtr(lpTree);
         lpTree = NULL;
      }
      else
      {
         wNRecurse = 0xffff;  /* for now, it's a bit easier... */

         wLevelArray[wTreeLevel] = wTreeIndex;
      }



      for(rval=FALSE, wRecurse=0; !ctrl_break && wRecurse<wNRecurse; wRecurse++)
      {
                        /** Get directory list buffer **/

         lpDirInfo = NULL;  // initial value

         lstrcpy(lpCurPath, lpDirPath);
         lp1 = lpCurPath + lstrlen(lpCurPath);

         if(*(lp1 - 1)!='\\')
         {
            *(lp1++) = '\\';
            *lp1 = 0;
         }

         if(wRecurse)
         {
            for(ctr=0; ctr<=wTreeLevel; ctr++)  /* build current path */
            {
               lstrcat(lpCurPath, lpTree[wLevelArray[ctr]].szDir);
               lstrcat(lpCurPath, pBACKSLASH);
            }

            lstrcat(lpCurPath, "*.*");

            if(lpTree[wTreeIndex].wChild)  /* a child element exists? */
            {
               wLevelArray[wTreeLevel++] = wTreeIndex;
               wTreeIndex = lpTree[wTreeIndex].wChild;

               wLevelArray[wTreeLevel] = wTreeIndex;
            }
            else if(lpTree[wTreeIndex].wSibling)
            {
               wTreeIndex = lpTree[wTreeIndex].wSibling;
               wLevelArray[wTreeLevel] = wTreeIndex;
            }
            else
            {
               while(wTreeLevel>0 && !lpTree[wTreeIndex].wSibling)
               {
                  wTreeIndex = wLevelArray[--wTreeLevel];
               }

               if(!wTreeLevel && !lpTree[wTreeIndex].wSibling)
               {
                  wNRecurse = wRecurse; /* end after this one! */
               }
               else
               {
                  wTreeIndex = lpTree[wTreeIndex].wSibling;
                  wLevelArray[wTreeLevel] = wTreeIndex;
               }
            }

         }
         else
         {        /* first time through - current dir (always) */

            lstrcat(lpCurPath, "*.*");

         }


         nFiles = GetDirList(lpCurPath, (~_A_RDONLY&~_A_SUBDIR&~_A_VOLID)&0xff,
                             (LPDIR_INFO FAR *)&lpDirInfo, work_buf);

         if(nFiles==DIRINFO_ERROR)
         {
            rval = TRUE;
            break;
         }
         else
         {
            dwTotalFound += nFiles;

            lp1 = work_buf + strlen(work_buf);

            if(lp1!=work_buf && *(lp1 - 1)!='\\')
            {
               *(lp1++) = '\\';        /* ensure it ends in a backslash */
            }

            dwTotalFound += nFiles;

            for(lpDI = lpDirInfo, i=0; i<nFiles; i++, lpDI++)
            {
               _fmemset(lp1, 0, sizeof(lpDI->fullname) + 2);

               _fstrncpy(lp1, lpDI->fullname, sizeof(lpDI->fullname));

               if(lpDI->attrib & _A_RDONLY)
               {
                  PrintErrorMessage(872);  // "?Read-Only file \""
                  PrintString(work_buf);
                  PrintAChar('\"');
                  PrintString(pCRLF);
               }
               else
               {
                  PrintErrorMessage(744);   // "Deleting "
                  PrintString(work_buf);

                  if(AskFlag)
                  {
                     PrintString("(N/Y)? ");

                     _fmemset(tbuf, 0, sizeof(tbuf));
                     GetUserInput(tbuf);

                     if(ctrl_break) break;

                     if(toupper(*tbuf)!='Y')
                     {
                        *lp1 = 0;
                        continue;
                     }

                  }
                  else
                  {
                     PrintString(pCRLF);
                  }

                  rval |= (MyUnlink(work_buf)!=0);

                  dwTotalFiles++;
               }

               *lp1 = 0;
            }
         }

         if(lpDirInfo)    GlobalFreePtr(lpDirInfo);

         lpDirInfo = NULL;
      }


      //  I *STILL* need to get rid of the directories!!  So, jam on
      //  through the tree until I remove them all!!  Or, at least TRY TO!


      wTreeLevel = 0;
      wTreeIndex = 0;

      wLevelArray[0] = 0;

      while(lpTree)
      {
         // FIRST STEP:  get 'lowest child'

         lstrcpy(lpCurPath, lpDirPath);

         lp1 = lpCurPath + lstrlen(lpCurPath);

         if(*(lp1 - 1)!='\\')
         {
            *(lp1++) = '\\';
            *lp1 = 0;
         }

         while(lpTree[wTreeIndex].wChild)
         {
            wLevelArray[wTreeLevel++] = wTreeIndex;
            wTreeIndex = lpTree[wTreeIndex].wChild;

            wLevelArray[wTreeLevel] = wTreeIndex;
         }

         for(ctr=0; ctr<=wTreeLevel; ctr++)  /* build current path */
         {
            lstrcat(lpCurPath, lpTree[wLevelArray[ctr]].szDir);
            lstrcat(lpCurPath, pBACKSLASH);
         }

         lp1 = lpCurPath + lstrlen(lpCurPath) - 1;
         if(*lp1 == '\\') *lp1 = 0;

         rval |= AskAndRemoveDirectory(lpCurPath, AskFlag);
         if(ctrl_break) break;

         if(lpTree[wTreeIndex].wSibling)
         {
            wTreeIndex = lpTree[wTreeIndex].wSibling;
            wLevelArray[wTreeLevel] = wTreeIndex;
         }
         else
         {
            // NO MORE SIBLINGS!  Recursively remove 'parent' directories
            // until I find one that has siblings.

            while(wTreeLevel>0 && !lpTree[wTreeIndex].wSibling)
            {
               wTreeIndex = wLevelArray[--wTreeLevel];

               lstrcpy(lpCurPath, lpDirPath);

               lp1 = lpCurPath + lstrlen(lpCurPath);

               if(*(lp1 - 1)!='\\')
               {
                  *(lp1++) = '\\';
                  *lp1 = 0;
               }

               for(ctr=0; ctr<=wTreeLevel; ctr++)  /* build current path */
               {
                  lstrcat(lpCurPath, lpTree[wLevelArray[ctr]].szDir);
                  lstrcat(lpCurPath, pBACKSLASH);
               }

               lp1 = lpCurPath + lstrlen(lpCurPath) - 1;
               if(*lp1 == '\\') *lp1 = 0;

               rval |= AskAndRemoveDirectory(lpCurPath, AskFlag);
               if(ctrl_break) break;
            }

            if(!wTreeLevel && !lpTree[wTreeIndex].wSibling)
            {
               break;       // I AM DONE!!
            }
            else
            {
               wTreeIndex = lpTree[wTreeIndex].wSibling;
               wLevelArray[wTreeLevel] = wTreeIndex;
            }
         }
      }

      if(!ctrl_break)
      {
         lp1 = lpDirPath + lstrlen(lpDirPath) - 1;
         if(*lp1 == '\\') *lp1 = 0;

         rval |= AskAndRemoveDirectory(lpDirPath, AskFlag);
      }


      if(lpTree)     GlobalFreePtr(lpTree);
      lpTree = NULL;

      if(lpDirInfo)    GlobalFreePtr(lpDirInfo);
      lpDirInfo = NULL;

      if(ctrl_break) break;

   }

   if(ctrl_break && !BatchMode) ctrl_break = FALSE;

   if(!dwTotalFound)
   {
      PrintErrorMessage(548);
   }
//   else
//   {
//      if(!dwTotalFiles)
//      {
//       static const char CODE_BASED pFmt[]=
//                                                  "No files deleted\r\n\n";
//
//         LockCode();
//         lstrcpy(work_buf, pFmt);
//         UnlockCode();
//      }
//      else if(dwTotalFiles!=1)
//      {
//       static const char CODE_BASED pFmt[]=
//                                       "Total of %lu files deleted\r\n\n";
//         LockCode();
//         wsprintf(work_buf, pFmt, dwTotalFiles);
//         UnlockCode();
//      }
//      else
//      {
//       static const char CODE_BASED pFmt[]=
//                                                  "1 file deleted\r\n\n";
//
//         LockCode();
//         lstrcpy(work_buf, pFmt);
//         UnlockCode();
//      }
//
//      PrintString(work_buf);
//   }



   END_LEVEL

   if(lpDirInfo0)   GlobalFreePtr(lpDirInfo0);
   if(lpDirInfo)    GlobalFreePtr(lpDirInfo);

   if(lpTree)       GlobalFreePtr(lpTree);

   if(lpParseInfo)
   {
      ReDirectOutput(lpParseInfo,FALSE);
      GlobalFreePtr(lpParseInfo);
   }

   if(lpDirPath)       GlobalFreePtr(lpDirPath);

   return(CMDErrorCheck(rval));
}





#pragma code_seg("CMDDir_TEXT","CODE")

void NEAR PASCAL CMDDirProcessPause(int FAR *lpPauseCtr);

BOOL NEAR PASCAL CMDDirCheckBreakAndStall(BOOL redirect_output,
                                       DWORD FAR *lpdwTickCount);



BOOL FAR PASCAL CMDDir(LPSTR lpCmd, LPSTR lpArgs)
{
long ttl_size, ttl_size2, cvf_size, total_usage, free_space;
WORD file_cnt, dir_cnt, item, ctr, cluster_size, wRecurse,
     wNRecurse, wTreeLevel, wTreeIndex, file_ctr;
BOOL rval, is_wide=FALSE, recurse=FALSE, pause=FALSE, extended=FALSE,
     lower_case=FALSE, compress=FALSE, fSwapped=FALSE, barebones=FALSE,
     AbnormalTermination, FirstTime;
DWORD grand_file_cnt, grand_dir_cnt, grand_ttl_size, grand_ttl_size2,
      grand_total_usage;
int i, j, attr, pause_ctr=0, program_only=0, longname_only=0, iTemp,
    is_sorted=-1, attribs=-1, dblspace_seq;
BYTE drDrive, drHost;
volatile LPSTR lpPath, lpSwitch;
LPSTR lpDirPath, lpDirSpec, lpCurPath, lpTempBuf, lp1, lp2;
NPSTR np1;
HCURSOR hOldCursor;
LPDIR_INFO lpDirInfo;
HPDIR_INFO lpDI;
LPTREE lpTree, lpT;
LPPARSE_INFO lpParseInfo;
WORD wLevelArray[64];
char label_buf[64];
OFSTRUCT ofstr;
DWORD dwTickCount;
LPSTR szLFNBuf, szLFNPath;

static struct diskfree_t dft;

#define DIRPATH_SIZE 512      /* size of 'lpDirPath' */



   dwTickCount = GetTickCount();


          /** Initially, check if 'DIRCMD' environment variable  **/
          /** is present, and if so insert it in front of the    **/
          /** argument string.  Any duplicate switches/args will **/
          /** automatically take precedence over those in DIRCMD **/

   if(!(lp1 = GetEnvString("DIRCMD")))
   {
      lpTempBuf = lpArgs;
   }
   else
   {
      lpTempBuf = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                        lstrlen(lp1) + lstrlen(lpArgs) + 2);

      if(!lpTempBuf)
      {
         PrintString(pNOTENOUGHMEMORY);
         return(TRUE);
      }

      lstrcpy(lpTempBuf, lp1);                /* build new command line!!! */
      lstrcat(lpTempBuf, " ");                /* with switches in 'DIRCMD' */
      lstrcat(lpTempBuf, lpArgs);           /* which will be superceded if */
                                         /* the user re-specifies a switch */
   }


   lpParseInfo = CMDLineParse(lpTempBuf);

   if(lpTempBuf!=lpArgs)      /* that is, if I built a temp buffer (above) */
      GlobalFreePtr(lpTempBuf);

             /** Allocate memory space for 'DirPath' storage **/

   lpDirPath = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE|GMEM_ZEROINIT,
                                      DIRPATH_SIZE + 2 * MAX_PATH);
   if(!lpDirPath)
   {
      PrintString(pNOTENOUGHMEMORY);
      GlobalFreePtr(lpParseInfo);
      return(TRUE);
   }

   lpDirSpec = lpDirPath + DIRPATH_SIZE/2 - 16;  /* the 'dir spec' only */
   lpCurPath = lpDirPath + DIRPATH_SIZE/2;       /* the 'current path' */


   // 2 additional buffer areas for LFN stuff...

   szLFNBuf  = lpDirPath + DIRPATH_SIZE;
   szLFNPath = lpDirPath + DIRPATH_SIZE + MAX_PATH;



          /** Verify correctness of command line input and parse **/

   if(CMDCheckParseInfo(lpParseInfo, "-WSPABCOXL4", 1 | CMD_ATMOST))
   {
      GlobalFreePtr(lpParseInfo);
      GlobalFreePtr(lpDirPath);
      return(TRUE);
   }

   for(i=0; i<(int)(lpParseInfo->n_switches); i++)
   {
      lpSwitch = (LPSTR)(lpParseInfo->sbpItemArray[i]);

      j = toupper(*lpSwitch);

      if(j=='W')       is_wide   = TRUE;
      else if(j=='S')  recurse   = TRUE;
      else if(j=='O')  is_sorted = i;   /* remember index of sort! */
      else if(j=='A')  attribs   = i;
      else if(j=='P')  pause     = TRUE;
      else if(j=='X')  extended  = TRUE;
      else if(j=='L')  lower_case= TRUE;
      else if(j=='C')  compress  = TRUE; /* show compression ratios */
      else if(j=='B')  barebones = TRUE;
      else if(j=='4')
      {
         PrintErrorMessage(573);
         CMDPause("PAUSE","");
      }
      else if(j=='-')
      {
         j = toupper(*(lpSwitch + 1));        /* turn it off (not on) */

         if(j=='W')       is_wide   = FALSE;
         else if(j=='S')  recurse   = FALSE;
         else if(j=='O')  is_sorted = -1;
         else if(j=='A')  attribs   = -1;
         else if(j=='X')  extended  = FALSE;
         else if(j=='P')  pause     = FALSE;
         else if(j=='L')  lower_case= FALSE;
         else if(j=='C')  compress  = FALSE;
         else if(j=='B')  barebones = FALSE;
         else if(j=='4')
         {
            PrintErrorMessage(574);
            CMDPause("PAUSE","");
         }
      }

   }


   attr = ~_A_HIDDEN & ~_A_VOLID & 0xff;   /* initial value (no hid/volid) */

   if(attribs!=-1)
   {
    int and_mask=(~_A_VOLID & 0xff), or_mask=0;

      attr = 0;

      lp1 = (LPSTR)(lpParseInfo->sbpItemArray[attribs]) + 1;
                                    /* 'lp1' now points right past the 'A' */
      if(*lp1 == ':') lp1++;        /* point past the ':' if there is one */

      if(*lp1==0)
      {
         attr=~_A_VOLID & 0xff;   /* nothing else in switch - all files!!! */
      }
      else
      {
         while(*lp1)
         {
            if(*lp1=='-')
            {
               lp1++;  /* advance pointer if a '-' found... */

               switch(toupper(*lp1))
               {
                  case 'H':
                     and_mask &= ~_A_HIDDEN;
                     break;

                  case 'S':
                     and_mask &= ~_A_SYSTEM;
                     break;

                  case 'D':
                     and_mask &= ~_A_SUBDIR;
                     break;

                  case 'A':
                     and_mask &= ~_A_ARCH;
                     break;

                  case 'R':
                     and_mask &= ~_A_RDONLY;
                     break;

                  case 'P':  /** SPECIAL!  Program Files Only **/
                     program_only = -1;  /* except now it's NOT programs */
                     break;

                  case 'L':  /** SPECIAL!  'Long File Names' Only **/
                     longname_only = -1;
                     break;

                  default:
                     PrintErrorMessage(534);
                     PrintString("/A-");
                     PrintAChar(*lp1);
                     PrintErrorMessage(543);

                     GlobalFreePtr(lpParseInfo);
                     GlobalFreePtr(lpDirPath);
                     return(TRUE);

               }
            }
            else
            {
               switch(toupper(*lp1))
               {
                  case 'H':
                     or_mask |= _A_HIDDEN;
                     break;

                  case 'S':
                     or_mask |= _A_SYSTEM;
                     break;

                  case 'D':
                     or_mask |= _A_SUBDIR;
                     break;

                  case 'A':
                     or_mask |= _A_ARCH;
                     break;

                  case 'R':
                     or_mask |= _A_RDONLY;
                     break;

                  case 'P':  /** SPECIAL!  Program Files Only **/
                     program_only = 1;
                     break;

                  case 'L':  /** SPECIAL!  'Long File Names' Only **/
                     longname_only = 1;
                     break;

                  default:
                     PrintErrorMessage(534);
                     PrintString("/A");
                     PrintAChar(*lp1);
                     PrintErrorMessage(543);

                     GlobalFreePtr(lpParseInfo);
                     GlobalFreePtr(lpDirPath);
                     return(TRUE);

               }
            }

            if(*lp1)  lp1++;   /* in case of problem, don't make it worse! */
         }

         attr = and_mask | (or_mask << 8); /* low byte is items to include */
                                             /* hi byte is items to match */
      }

   }



   hOldCursor = MySetCursor(LoadCursor(NULL, IDC_WAIT));

                  /** get path argument (if present) **/

   if(lpParseInfo->n_parameters==1)
   {
    volatile int itemp;

      itemp = lpParseInfo->n_switches;
      lpPath = (LPSTR)((lpParseInfo->sbpItemArray[itemp]));
   }
   else
   {
      lpPath = (LPSTR)pWILDCARD;  /* the default */
   }

   ReDirectOutput(lpParseInfo,TRUE);

   INIT_LEVEL

   if((rval = QualifyPath(lpDirPath, lpPath, TRUE))!=FALSE)
      break;

   lstrcpy(work_buf, lpDirPath);  /* also copy into 'work_buf' */



                /** obtain drive label if there is one **/

   if(GetDriveLabel((LPSTR)label_buf, (LPSTR)work_buf))
   {
      PrintString(pERRORREADINGDRIVE);

      rval = TRUE;
      break;
   }
   else if(!barebones)
   {
      if(*label_buf<=' ')  /* blank volume label */
      {
         PrintString(pVOLUMEINDRIVE);
         PrintAChar(toupper(*lpDirPath));
         PrintString(pHASNOLABEL);
         if(*label_buf) PrintString(label_buf);
         PrintString(pCRLF);
         pause_ctr++;
      }
      else
      {
         PrintErrorMessage(745);   // " Volume in drive '"
         PrintAChar(toupper(*lpDirPath));
         PrintErrorMessage(746);   // "' is "
         PrintString((LPSTR)label_buf);
         PrintString(pCRLF);
         pause_ctr++;
      }
   }

         /* get 'free space', cluster size, and other info */

   if(work_buf[0] == '\\' && work_buf[1] == '\\')  // netwrk drive
   {
      drHost = drDrive;
      fSwapped = FALSE;
      dblspace_seq = -1;     // flags 'not a DBLSPACE drive'
   }
   else
   {
      drDrive = toupper(*work_buf) - 'A';   // 'A'==0, 'B'==1, etc.


      if(!IsDoubleSpaceDrive(drDrive, &fSwapped, &drHost, &dblspace_seq))
      {
         drHost = drDrive;
         fSwapped = FALSE;
         dblspace_seq = -1;     // flags 'not a DBLSPACE drive'
      }
      else if(!barebones)
      {
       static const char CODE_BASED pFmt[]=
                                                     "%c:\\DBLSPACE.%03d  ";

         PrintErrorMessage(747);   // " 'DBLSPACE' Compressed drive    HOST: "

         LockCode();
         wsprintf(work_buf, pFmt, (int)drHost + 'A', dblspace_seq);
         UnlockCode();

         PrintString(work_buf);
         if(fSwapped) PrintErrorMessage(705);  // "  (SWAPPED)"

         PrintString(pCRLF);

         pause_ctr++;
      }
   }



   if(work_buf[0] == '\\' && work_buf[1] == '\\')  // netwrk drive
   {
      cluster_size = (WORD)-1;
      free_space = -1;
   }
   else
   {
      if(MyGetDiskFreeSpace(drDrive + 1, &dft))
      {
         PrintString("\r\n?Unable to get drive allocation information\r\n\n");

         rval = TRUE;

         break;
      }

//   _dos_getdiskfree(drDrive + 1, &dft);

      cluster_size = dft.bytes_per_sector * dft.sectors_per_cluster;
      free_space   = (long)cluster_size * dft.avail_clusters;
   }


   lp1 = _fstrrchr(lpDirPath, '\\'); /* get the 'final \' in the string */
   if(lp1)
   {
      lstrcpy(lpDirSpec, lp1 + 1); /* copy remaining string to 'lpDirSpec' */
      *(lp1 + 1) = 0;             /* terminate string after last backslash */
   }
   else
   {
      PrintErrorMessage(575);
      break;
   }


   grand_file_cnt    = 0;
   grand_dir_cnt     = 0;
   grand_ttl_size    = 0;
   grand_ttl_size2   = 0;
   grand_total_usage = 0;

   file_cnt = 0;
   dir_cnt = 0;

   if(recurse)
   {

      lstrcpy(work_buf, lpDirPath);

      if(!(lpTree = GetDirTree(work_buf)))
      {
         if(ctrl_break)
         {
            PrintString(pCTRLC);
            PrintString(pCRLFLF);
         }
         else
         {
            PrintErrorMessage(576);
         }

         break;
      }

      lpT = lpTree;

      if(!(*lpTree->szDir))  /* empty tree list? */
      {
         wNRecurse = 1;
         recurse = FALSE;
         GlobalFreePtr(lpTree);
         lpTree = NULL;
      }
      else
      {
         wNRecurse = 0xffff;  /* for now, it's a bit easier... */
         wTreeLevel = 0;
         wTreeIndex = 0;

         wLevelArray[wTreeLevel] = wTreeIndex;
      }

   }
   else
   {
      wNRecurse = 1;   /* always go at least 1 time; helps detect errors */
      lpTree = NULL;
   }

   AbnormalTermination = FALSE;

   for(wRecurse=0; wRecurse<wNRecurse; wRecurse++)
   {
                        /** Get directory list buffer **/

      ttl_size    = 0;     /* initialize sub-total buckets */
      ttl_size2   = 0;
      total_usage = 0;


      if(wRecurse && recurse)
      {
         lstrcpy(lpCurPath, lpDirPath);

         for(ctr=0; ctr<=wTreeLevel; ctr++)  /* build current path */
         {
            lstrcat(lpCurPath, lpTree[wLevelArray[ctr]].szDir);
            lstrcat(lpCurPath, pBACKSLASH);
         }


         *szLFNPath = 0;

         if(IsChicago || IsNT)
         {
            if(*lpCurPath && lstrlen(lpCurPath) > 2
               && lpCurPath[lstrlen(lpCurPath) - 1]=='\\'
               && lpCurPath[lstrlen(lpCurPath) - 2]!=':')
            {
               lpCurPath[lstrlen(lpCurPath) - 1] = 0;

               GetShortName(lpCurPath, lpCurPath);
               GetLongName(lpCurPath, szLFNPath);

               if(!lstrcmp(lpCurPath, szLFNPath)) *szLFNPath = 0;

               lstrcat(lpCurPath, pBACKSLASH);
            }
         }

         // use 'lower_case' flag to either convert path to ALL lower case,
         // or to convert path to ALL upper case...

         if(lower_case)
         {
            _fstrlwr(lpCurPath);  // convert to ALL lower case
         }
         else
         {
            _fstrupr(lpCurPath);  // convert to ALL upper case
         }


         lstrcat(lpCurPath, lpDirSpec);

         if(lpTree[wTreeIndex].wChild)  /* a child element exists? */
         {
            wLevelArray[wTreeLevel++] = wTreeIndex;
            wTreeIndex = lpTree[wTreeIndex].wChild;

            wLevelArray[wTreeLevel] = wTreeIndex;
         }
         else if(lpTree[wTreeIndex].wSibling)
         {
            wTreeIndex = lpTree[wTreeIndex].wSibling;
            wLevelArray[wTreeLevel] = wTreeIndex;
         }
         else
         {
            while(wTreeLevel>0 && !lpTree[wTreeIndex].wSibling)
            {
               wTreeIndex = wLevelArray[--wTreeLevel];
            }

            if(!wTreeLevel && !lpTree[wTreeIndex].wSibling)
            {
               wNRecurse = wRecurse; /* end after this one! */
            }
            else
            {
               wTreeIndex = lpTree[wTreeIndex].wSibling;
               wLevelArray[wTreeLevel] = wTreeIndex;
            }
         }

      }
      else
      {     /* first (or only) time through - current dir (always) */

         lstrcpy(lpCurPath, lpDirPath);


         *szLFNPath = 0;

         if(IsChicago || IsNT)
         {
            if(*lpCurPath && lstrlen(lpCurPath) > 2
               && lpCurPath[lstrlen(lpCurPath) - 1]=='\\'
               && lpCurPath[lstrlen(lpCurPath) - 2]!=':')
            {
               lpCurPath[lstrlen(lpCurPath) - 1] = 0;

               GetShortName(lpCurPath, lpCurPath);
               GetLongName(lpCurPath, szLFNPath);

               if(!lstrcmp(lpCurPath, szLFNPath)) *szLFNPath = 0;

               // network path may have had backslash added to it...
               // therefore I must check BEFORE I add one onto the end...

               if(!*lpCurPath || lpCurPath[lstrlen(lpCurPath) - 1] != '\\')
                  lstrcat(lpCurPath, pBACKSLASH);
            }
         }

         // use 'lower_case' flag to either convert path to ALL lower case,
         // or to convert path to ALL upper case...
         // (note that DIRECTORY SPEC is not case sensitive; if it ever
         //  becomes case sensitive I must modify this procedure accordingly)

         if(lower_case)
         {
            _fstrlwr(lpCurPath);  // convert to ALL lower case
         }
         else
         {
            _fstrupr(lpCurPath);  // convert to ALL upper case
         }

         lstrcat(lpCurPath, lpDirSpec);
      }

      file_ctr = GetDirList(lpCurPath, attr, (LPDIR_INFO FAR *)&lpDirInfo, NULL);

      if(!ctrl_break && (GetTickCount() - dwTickCount) >= 50)  // 20 times per second
      {
         if(LoopDispatch())
         {
            AbnormalTermination = TRUE;

            if(lpDirInfo) GlobalFreePtr((LPSTR)lpDirInfo);
            lpDirInfo = NULL;
            break;
         }

         dwTickCount = GetTickCount();
      }

      if(ctrl_break)
      {
         AbnormalTermination = TRUE;

         if(lpDirInfo) GlobalFreePtr((LPSTR)lpDirInfo);
         lpDirInfo = NULL;

         break;
      }

      if(!file_ctr || file_ctr==DIRINFO_ERROR)
      {
         if(lpDirInfo) GlobalFreePtr((LPSTR)lpDirInfo);
         lpDirInfo = NULL;

         if(!file_ctr) continue;    /* if it's empty, just continue on */
         else          break;       /* otherwise, bail out! */
      }




                          /*** DIRECTORY SORTING ***/

      if(is_sorted!=-1)
      {
         lp1 = (LPSTR)(lpParseInfo->sbpItemArray[is_sorted]) + 1;
                                       /* 'lp1' now points right past the 'O' */
         if(*lp1==':') lp1++;                         /* points it past colon */

         CMDDirSort(lpDirInfo, lp1, file_ctr);
      }


      FirstTime = TRUE;

      file_cnt = 0;
      dir_cnt = 0;

      for(ctr=0,lpDI=(HPDIR_INFO)lpDirInfo,item=0;item<file_ctr;item++,lpDI++)
      {
                 /* here I verify valid attribute flags first */
               /* don't forget to do a 'rval = _dos_findnext()' */

         if(barebones)
         {
            if(lpDI->fullname[0]=='.' &&
               (lpDI->fullname[1]==0 ||
                (lpDI->fullname[1]=='.' && lpDI->fullname[2]==0)))
            {
               continue; // go to next item.  Period.  NO matter what!
            }
         }


               /** Determine file type string (when applicable) **/

         if(lpDI->attrib & _A_SUBDIR)
         {
            np1 = "<DIR>";    /* 'program only' still includes directories */
         }
         else if(program_only || (extended && !is_wide))/*** the /X switch! ***/
         {                                              /*** or '/A:P'!     ***/
            lstrcpy(work_buf, lpCurPath);

            lp1 = _fstrrchr(work_buf, '\\');  /* find the 'final \' */

            if(lp1)
            {
               lp1++;   /* point just past the '\\' */
               _hmemset(lp1, 0, sizeof(lpDI->fullname) + 2);
               _fstrncpy(lp1, (LPSTR)lpDI->fullname, sizeof(lpDI->fullname));
            }

            if(lp1 && !IsProgramFile(work_buf, (LPOFSTRUCT)&ofstr,
                                     (int FAR *)&iTemp))
            {
               switch(iTemp)
               {
                  case PROGRAM_WIN16:
                     np1 = "<WIN>";
                     break;

                  case PROGRAM_L32:
                     np1 = ">L32<";     /* guessing */
                     break;

                  case PROGRAM_PE:
                     np1 = "<W32>";     // I'm positive!
                     break;

                  case PROGRAM_NONWINDOWS:
                     np1 = "<DOS>";
                     break;

                  case PROGRAM_OS2:
                     np1 = "<OS2>";
                     break;

                  case PROGRAM_OS2_32:
                     np1 = "OS/32";     /* guessing */
                     break;

                  case PROGRAM_VXD:
                     np1 = ">VXD<";
                     break;

                  case PROGRAM_FALSE:
                     if(_fstrnicmp((LPSTR)lpDI->ext, "COM", 3)==0)
                     {
                        np1 = "<COM>";
                     }
                     else if(_fstrnicmp((LPSTR)lpDI->ext, "BAT", 3)==0)
                     {
                        np1 = "<BAT>";
                     }
                     else if(_fstrnicmp((LPSTR)lpDI->ext, "CMD", 3)==0)
                     {
                        np1 = "<CMD>";
                     }
                     else if(_fstrnicmp((LPSTR)lpDI->ext, "PIF", 3)==0)
                     {
                        np1 = "<PIF>";
                     }
                     else
                     {
                        np1 = "";
                     }
                     break;

                  default:
                     if(iTemp>0)  np1 = "<WIN>";
                     else         np1 = "";
               }

               if((program_only>0 && *np1==0) ||
                  (program_only<0 && *np1))  continue;

              /* use string (above) to determine if it's a program or not */
                    /* a non-program file has a 'blank' identifier */
            }
            else
            {
               np1 = "<ERR>";
            }

         }
         else
         {
            np1 = "";
         }


         *szLFNBuf = 0;

         // Do I need to get LFN information for each file?  hmmm??

         if((IsChicago || IsNT) && ((!is_wide && !barebones) || longname_only))
         {
            if((lpDI->fullname[0] != '.' ||
                (lpDI->fullname[1] != 0 && lpDI->fullname[1] != '.') ||
                (lpDI->fullname[1] == '.' && lpDI->fullname[2] != 0)))
            {
               // get the LFN here

               lp1 = work_buf + 256;

               _fstrcpy(lp1, lpCurPath);
               lp2 = _fstrrchr(lp1, '\\');  /* find the 'final \' */

               if(lp2)
               {
                  lp2++;   /* point just past the '\\' */
                  _hmemset(lp2, 0, sizeof(lpDI->fullname) + 2);
                  _fstrncpy(lp2, (LPSTR)lpDI->fullname, sizeof(lpDI->fullname));

                  if(lower_case) _fstrlwr(lp2);

                  GetLongName(lp1, szLFNBuf);

                  lp1 = _fstrrchr(szLFNBuf, '\\');

                  if(lp1 && !_fstrcmp(lp1 + 1, lp2))
                  {
                     *szLFNBuf = 0;
                  }
                  else
                  {
                     lstrcpy(szLFNBuf, lp1 + 1);
                  }

               }
            }

            if((longname_only>0 && !*szLFNBuf) ||
               (longname_only<0 && *szLFNBuf))  continue;
         }


         file_cnt++;   // count ACTUAL number of files displayed



                 /**********************************************/
                 /** AT THIS POINT THE ENTRY WILL BE PRINTED! **/
                 /** Now it is safe to print header/add sizes **/
                 /**********************************************/


         if(FirstTime)                  /* the first time through !!! */
         {
            FirstTime = FALSE;

            if(!barebones || (recurse && is_wide))
            {
               pause_ctr++;

               if(pause && redirect_output==HFILE_ERROR)
               {
                  CMDDirProcessPause(&pause_ctr);
               }

               if(CMDDirCheckBreakAndStall(redirect_output, &dwTickCount))
               {
                  AbnormalTermination = TRUE;
                  break;
               }


               if(!barebones)
               {
                  PrintErrorMessage(748);   // "Directory of "
               }

               PrintString(lpCurPath);

               if(!barebones && (IsChicago || IsNT) && *szLFNPath)
               {                             // directory has LONG name also

                  PrintString(pCRLF);

                  pause_ctr++;

                  if(pause && redirect_output==HFILE_ERROR)
                  {
                     CMDDirProcessPause(&pause_ctr);
                  }

                  if(CMDDirCheckBreakAndStall(redirect_output, &dwTickCount))
                  {
                     AbnormalTermination = TRUE;
                     break;
                  }

                  PrintAChar('\"');

                  lp1 = szLFNPath;

                  do
                  {
                     if(lstrlen(lp1) <= 77)
                     {
                        PrintString(lp1);
                        lp1 += lstrlen(lp1);
                     }
                     else
                     {
                        _hmemcpy(work_buf, lp1, 77);
                        work_buf[77] = 0;
                        lp1 += 77;

                        PrintString(work_buf);

                        pause_ctr++;

                        if(pause && redirect_output==HFILE_ERROR)
                        {
                           CMDDirProcessPause(&pause_ctr);
                        }

                        if(CMDDirCheckBreakAndStall(redirect_output, &dwTickCount))
                        {
                           AbnormalTermination = TRUE;
                           break;
                        }


                        PrintAChar('\"');
                        PrintString(pCRLF);
                        PrintAChar('\"');
                     }

                  } while(*lp1);

                  PrintAChar('\"');
               }

               PrintString(pCRLF);   // one line break for all

               if(!barebones)
               {
                  pause_ctr++;

                  if(pause && redirect_output==HFILE_ERROR)
                  {
                     CMDDirProcessPause(&pause_ctr);
                  }

                  if(CMDDirCheckBreakAndStall(redirect_output, &dwTickCount))
                  {
                     AbnormalTermination = TRUE;
                     break;
                  }

                  PrintString(pCRLF);
               }

            }

            if(!barebones && !is_wide)
            {
               FirstTime = FALSE;

               pause_ctr += 2;

               if(pause && redirect_output==HFILE_ERROR)
               {
                  CMDDirProcessPause(&pause_ctr);
               }

               if(CMDDirCheckBreakAndStall(redirect_output, &dwTickCount))
               {
                  AbnormalTermination = TRUE;
                  break;
               }


               if(IsChicago || IsNT)
               {
                  if(compress && dblspace_seq!=-1)
                  {
                     PrintErrorMessage(866); // "NAME    .EXT  Type   File Size    Date     Time    Ratio  Long Name\r\n"
                                             // "-------- ---  -----  ---------  -------- --------  -----  --------------------\r\n"
                  }
                  else
                  {
                     PrintErrorMessage(867); // "NAME    .EXT  Type   File Size    Date     Time    Long Name\r\n"
                                             // "-------- ---  -----  ---------  -------- --------  --------------------\r\n"
                  }
               }
               else
               {
                  if(compress && dblspace_seq!=-1)
                  {
                     PrintErrorMessage(749); // "NAME    .EXT  Type   File Size    Date     Time    Comp Ratio\r\n"
                                             // "-------- ---  -----  ---------  -------- --------  ----------\r\n"
                  }
                  else
                  {
                     PrintErrorMessage(750); // "NAME    .EXT  Type   File Size    Date     Time\r\n"
                                             // "-------- ---  -----  ---------  -------- --------\r\n"
                  }
               }
            }

         }


         if(lpDI->attrib & _A_SUBDIR)
         {
            dir_cnt++;
         }
         else if(cluster_size != -1)
         {
            ttl_size += lpDI->size;
            total_usage += (lpDI->size + cluster_size - 1) -
                           ((lpDI->size + cluster_size - 1) % cluster_size);
         }


                     /** Write file information to screen **/

         if(is_wide)
         {
            if(lpDI->attrib & _A_SUBDIR)
            {
             static const char CODE_BASED pFmt[]=      "[%.12s]            ";

               LockCode();
               wsprintf(work_buf, pFmt, (LPSTR)lpDI->fullname);
               UnlockCode();

               work_buf[14] = 0;          /* end string at 14 chars (exactly) */
            }
            else
            {
             static const char CODE_BASED pFmt[]=          "%-8.8s.%-3.3s  ";

               LockCode();
               wsprintf(work_buf, pFmt, (LPSTR)lpDI->name, (LPSTR)lpDI->ext);
               UnlockCode();
            }

            if((ctr % 5)==4)              /* for each 5 items print <CR><LF> */
            {
               strcat(work_buf, pCRLF);
               pause_ctr++;
               ctr = 0;
            }
            else
            {
               ctr++;
               strcat(work_buf, "  ");              /* otherwise, next column */
            }

         }
         else if(barebones)
         {
            if(recurse)
            {
               lstrcpy(work_buf, lpCurPath);

               lp1 = _fstrrchr(work_buf, '\\');  /* find the 'final \' */

               if(lp1)
               {
                  lp1++;   /* point just past the '\\' */
                  _hmemset(lp1, 0, sizeof(lpDI->fullname) + 2);
                  _fstrncpy(lp1, (LPSTR)lpDI->fullname, sizeof(lpDI->fullname));
               }
            }
            else
            {
               _hmemset(work_buf, 0, sizeof(lpDI->fullname) + 2);
               _fstrncpy(work_buf, (LPSTR)lpDI->fullname, sizeof(lpDI->fullname));
            }

            lstrcat(work_buf, pCRLF);
         }
         else
         {

            if(compress && dblspace_seq!=-1)
            {
               if(lpDI->attrib & _A_SUBDIR)
               {
                  cvf_size = 0;  // also prints "N/A" for compression ratio...
               }
               else
               {
                  lstrcpy(work_buf, lpCurPath);

                  lp1 = _fstrrchr(work_buf, '\\');  /* find the 'final \' */

                  if(lp1)
                  {
                     lp1++;   /* point just past the '\\' */
                     _hmemset(lp1, 0, sizeof(lpDI->fullname) + 2);
                     _fstrncpy(lp1, (LPSTR)lpDI->fullname, sizeof(lpDI->fullname));
                  }

                  cvf_size = DblspaceGetCompressedFileSize(work_buf);

                  if(cvf_size >= 0) ttl_size2 += cvf_size;
               }
            }
            else
            {
               cvf_size = -1;        // indicates error or N/A
            }



            // finally I get to print something!

            if(lpDI->attrib & _A_SUBDIR)
            {
             static const char CODE_BASED pFmt[]=
                  "%-8.8s      %-5s             %02d/%02d/%02d %02d:%02d:%02d";
             static const char CODE_BASED pFmt2[]=
                  "%-8.8s.%-3.3s  %-5s             %02d/%02d/%02d %02d:%02d:%02d";

               LockCode();

               if((lpDI->ext[0] && lpDI->ext[0] != ' ') ||
                  (lpDI->ext[1] && lpDI->ext[1] != ' ') ||
                  (lpDI->ext[2] && lpDI->ext[2] != ' '))
               {
                 wsprintf(work_buf, pFmt2,
                  (LPSTR)lpDI->name, (LPSTR)lpDI->ext,   /* file name.ext */
                  (LPSTR)np1,                            /* <DIR> or other */
                  (int)((lpDI->date >> 5)&0xf),          /* month */
                  (int)(lpDI->date & 0x1f),              /* day */
                  (int)(((lpDI->date >> 9)+80)%100),     /* year (last 2 dig) */
                  (int)(lpDI->time >> 11),               /* hour */
                  (int)((lpDI->time >> 5) & 0x3f),       /* minute */
                  (int)((lpDI->time & 0x1f)*2)           /* seconds! */
                  );
               }
               else
               {
                 wsprintf(work_buf, pFmt,
                  (LPSTR)lpDI->name,                     /* file name     */
                  (LPSTR)np1,                            /* <DIR> or other */
                  (int)((lpDI->date >> 5)&0xf),          /* month */
                  (int)(lpDI->date & 0x1f),              /* day */
                  (int)(((lpDI->date >> 9)+80)%100),     /* year (last 2 dig) */
                  (int)(lpDI->time >> 11),               /* hour */
                  (int)((lpDI->time >> 5) & 0x3f),       /* minute */
                  (int)((lpDI->time & 0x1f)*2)           /* seconds! */
                  );
               }
               UnlockCode();
            }
            else
            {
             static const char CODE_BASED pFmt[]=
                  "%-8.8s      %-5s %10ld  %02d/%02d/%02d %02d:%02d:%02d";
             static const char CODE_BASED pFmt2[]=
                  "%-8.8s.%-3.3s  %-5s %10ld  %02d/%02d/%02d %02d:%02d:%02d";

               LockCode();

               if((lpDI->ext[0] && lpDI->ext[0] != ' ') ||
                  (lpDI->ext[1] && lpDI->ext[1] != ' ') ||
                  (lpDI->ext[2] && lpDI->ext[2] != ' '))
               {
                 wsprintf(work_buf, pFmt2,
                  (LPSTR)lpDI->name, (LPSTR)lpDI->ext,   /* file name.ext */
                  (LPSTR)np1,                            /* <DIR> or other */
                  (long)lpDI->size,                      /* file size */
                  (int)((lpDI->date >> 5)&0xf),          /* month */
                  (int)(lpDI->date & 0x1f),              /* day */
                  (int)(((lpDI->date >> 9)+80)%100),     /* year (last 2 dig) */
                  (int)(lpDI->time >> 11),               /* hour */
                  (int)((lpDI->time >> 5) & 0x3f),       /* minute */
                  (int)((lpDI->time & 0x1f)*2)           /* seconds! */
                  );
               }
               else
               {
                 wsprintf(work_buf, pFmt,
                  (LPSTR)lpDI->name,                     /* file name     */
                  (LPSTR)np1,                            /* <DIR> or other */
                  (long)lpDI->size,                      /* file size */
                  (int)((lpDI->date >> 5)&0xf),          /* month */
                  (int)(lpDI->date & 0x1f),              /* day */
                  (int)(((lpDI->date >> 9)+80)%100),     /* year (last 2 dig) */
                  (int)(lpDI->time >> 11),               /* hour */
                  (int)((lpDI->time >> 5) & 0x3f),       /* minute */
                  (int)((lpDI->time & 0x1f)*2)           /* seconds! */
                  );
               }

               UnlockCode();
            }

            if(lower_case) _fstrlwr(work_buf);

            _fstrcat(work_buf, "  ");    // 2 separating white spaces

            if(compress)
            {
               lp1 = work_buf + _fstrlen(work_buf);

               if(cvf_size>=0 && cluster_size != -1)
               {
                  lstrcpy(lp1,
                   DblspaceCompressionRatio((lpDI->size + cluster_size - 1)
                                            -((lpDI->size + cluster_size - 1)
                                              % cluster_size),
                                            cvf_size));
               }

               _fstrcat(lp1, "       ");
               lp1[7] = 0;               // make it exactly 7 characters long
                                         // 5 characters for ratio, plus 2
                                         // more white space for separation.
            }


            if((IsChicago || IsNT) && *szLFNBuf)   // LFN found on file name!
            {
               lp1 = szLFNBuf;

               if(redirect_output==HFILE_ERROR)
               {
                  lp2 = work_buf + lstrlen(work_buf);

                  do
                  {
                     if(lstrlen(lp1) < (79 - (int)(lp2 - (LPSTR)work_buf)))
                     {
                        lstrcat(work_buf, lp1);
                        break;
                     }

                     work_buf[lstrlen(work_buf) +
                              (79 - (int)(lp2 - (LPSTR)work_buf))] = 0;

                     _hmemcpy(work_buf + lstrlen(work_buf),
                              lp1, (79 - (int)(lp2 - (LPSTR)work_buf)));

                     lp1 += (79 - (int)(lp2 - (LPSTR)work_buf));

                     if(*lp1)
                     {
                        lstrcat(work_buf, pCRLF);
                        pause_ctr++;

                        // insert ANSI command to skip to correct column

                        wsprintf((LPSTR)work_buf + lstrlen(work_buf),
                                 "\x1b[K\x1b[%dC", (int)(lp2 - (LPSTR)work_buf));
                     }

                  } while(*lp1);

               }
               else
               {
                  lstrcat(work_buf, lp1);  // for now it's easier
               }
            }


            lstrcat(work_buf, pCRLF);

            pause_ctr++;

         }

                           /* Process '/P' PAUSE command */

         if(pause && redirect_output==HFILE_ERROR)
         {
            CMDDirProcessPause(&pause_ctr);
         }

         if(CMDDirCheckBreakAndStall(redirect_output, &dwTickCount))
         {
            AbnormalTermination = TRUE;
            break;
         }

         PrintString(work_buf);

      }

      if(lpDirInfo) GlobalFreePtr(((LPSTR)lpDirInfo)); /* I'm done with this... */
      lpDirInfo = NULL;

      if(AbnormalTermination) break;


                               /**  END OF LOOP  **/
                               /** CLEANUP, ETC. **/

      if(is_wide && (ctr % 5)!=0)
         PrintString(pCRLF);          /* the final <CR><LF> on wide format!! */

      if(!barebones && file_cnt)      // don't print if no files matched
      {                               // or we are using 'bare bones' format

         // Print the 'Total of x bytes in x files' message

         pause_ctr++;

         if(pause && redirect_output==HFILE_ERROR)
         {
            CMDDirProcessPause(&pause_ctr);
         }

         if(CMDDirCheckBreakAndStall(redirect_output, &dwTickCount))
         {
            AbnormalTermination = TRUE;
            break;
         }


         if((file_cnt - dir_cnt)==1)
         {
            if(ttl_size!=1)
            {
             static const char CODE_BASED pFmt[]=
                                          "\nTotal of %ld bytes in 1 file";

               LockCode();
               wsprintf(work_buf, pFmt, (DWORD)ttl_size);
               UnlockCode();
            }
            else
            {
             static const char CODE_BASED pFmt[]=
                                             "\nTotal of 1 byte in 1 file";

               LockCode();
               lstrcpy(work_buf, pFmt);
               UnlockCode();
            }
         }
         else if(ttl_size || file_cnt != dir_cnt)
         {
            if(ttl_size!=1)
            {
             static const char CODE_BASED pFmt[]=
                                       "\nTotal of %ld bytes in %ld files";

               LockCode();
               wsprintf(work_buf, pFmt, (DWORD)ttl_size,
                        (DWORD)file_cnt - dir_cnt);
               UnlockCode();
            }
            else
            {
             static const char CODE_BASED pFmt[]=
                                          "\nTotal of 1 byte in %ld files";

               LockCode();
               wsprintf(work_buf, pFmt, (DWORD)file_cnt - dir_cnt);
               UnlockCode();
            }
         }

         if(dir_cnt)
         {
            if(file_cnt == dir_cnt && !ttl_size)
            {
             static const char CODE_BASED pFmt[]=
                                          "\nTotal of ";

               lstrcpy(work_buf, pFmt);
            }
            else
            {
             static const char CODE_BASED pFmt[]=
                                          " and ";

               lstrcat(work_buf, pFmt);
            }

            if(dir_cnt == 1)
            {
             static const char CODE_BASED pFmt[]=
                                         "1 directory";

               lstrcat(work_buf, pFmt);
            }
            else
            {
             static const char CODE_BASED pFmt[]=
                                         "%ld directories";

               wsprintf(work_buf + lstrlen(work_buf), pFmt, (DWORD)dir_cnt);
            }
         }

         lstrcat(work_buf, ".\r\n");

         PrintString(work_buf);            /* print the # of bytes/files message */


         if(compress && dblspace_seq!=-1)
         {
            pause_ctr++;

            if(pause && redirect_output==HFILE_ERROR)
            {
               CMDDirProcessPause(&pause_ctr);
            }

            if(CMDDirCheckBreakAndStall(redirect_output, &dwTickCount))
            {
               AbnormalTermination = TRUE;
               break;
            }

            PrintErrorMessage(752);  // "Average Compression Ratio:  "

            PrintString(DblspaceCompressionRatio(total_usage, ttl_size2));

            PrintString(pCRLF);
         }



         // print the 'Disk Usage / free space / efficiency' message

         pause_ctr+=2;

         if(pause && redirect_output==HFILE_ERROR)
         {
            CMDDirProcessPause(&pause_ctr);
         }

         if(CMDDirCheckBreakAndStall(redirect_output, &dwTickCount))
         {
            AbnormalTermination = TRUE;
            break;
         }


         *work_buf = 0;


         if(!recurse)
         {
            if(total_usage && cluster_size != -1)
            {
             static const char CODE_BASED pFmt[]=
                   "Disk Usage: %ld bytes   Free: %ld bytes   Efficiency:  %2d%%\r\n\n";

               LockCode();
               wsprintf(work_buf, pFmt,
                        (DWORD)total_usage, (DWORD)free_space,
                        (int)(100.0 * (double)ttl_size/(double)total_usage + .5));
               UnlockCode();
            }
            else if(cluster_size != -1)
            {
             static const char CODE_BASED pFmt[]=
                                 "Disk Usage:  0 bytes   Free: %ld bytes\r\n\n";

               LockCode();
               wsprintf(work_buf, pFmt, (DWORD)free_space);
               UnlockCode();
            }

         }
         else
         {
            if(total_usage && cluster_size != -1)
            {
             static const char CODE_BASED pFmt[]=
                              "Disk Usage: %ld bytes  Efficiency:  %2d%%\r\n\n";

               LockCode();
               wsprintf(work_buf, pFmt,
                        (DWORD)total_usage,
                        (int)(100.0 * (double)ttl_size/(double)total_usage + .5));
               UnlockCode();
            }
            else if(cluster_size != -1)
            {
             static const char CODE_BASED pFmt[]=
                                                  "Disk Usage:  0 bytes\r\n\n";

               LockCode();
               lstrcpy(work_buf, pFmt);
               UnlockCode();
            }

         }

         if(*work_buf)
            PrintString(work_buf);    /* print the disk usage/free space message */
      }


      if(cluster_size != -1)
      {
         grand_file_cnt    += file_cnt; /* must total it - used as flag also! */
         grand_dir_cnt     += dir_cnt;
         grand_ttl_size    += ttl_size;
         grand_ttl_size2   += ttl_size2;
         grand_total_usage += total_usage;
      }

   }


   if(AbnormalTermination || grand_file_cnt==0 || wRecurse<wNRecurse)
   {
      if(grand_file_cnt==0)
      {
         PrintErrorMessage(548);
         rval = FALSE;
      }
      else if(!AbnormalTermination)
      {
         PrintErrorMessage(577);
         rval = TRUE;
      }
      else if(ctrl_break)
      {
         if(!BatchMode) ctrl_break = FALSE;  // eat ctrl break if not BATCH

         PrintString(pCTRLC);
         PrintString(pCRLFLF);
         rval = TRUE;
      }
      else
      {
         PrintString(pCRLF);
         rval = TRUE;
      }

      break;
   }
   else if(!barebones && cluster_size != -1)
   {
      if(recurse)
      {
         pause_ctr++;

         if(pause && redirect_output==HFILE_ERROR)
         {
            CMDDirProcessPause(&pause_ctr);
         }

         if(CMDDirCheckBreakAndStall(redirect_output, &dwTickCount))
         {
            AbnormalTermination = TRUE;
            break;
         }


         if((grand_file_cnt - grand_dir_cnt)==1)
         {
            if(grand_ttl_size!=1)
            {
             static const char CODE_BASED pFmt[]=
                                  "\nGrand Total of %ld bytes in 1 file";

               LockCode();
               wsprintf(work_buf, pFmt, (DWORD)grand_ttl_size);
               UnlockCode();
            }
            else
            {
             static const char CODE_BASED pFmt[]=
                                           "\nGrand Total of 1 byte in 1 file";

               LockCode();
               lstrcpy(work_buf, pFmt);
               UnlockCode();
            }
         }
         else if(grand_ttl_size || grand_file_cnt != grand_dir_cnt)
         {
            if(grand_ttl_size!=1)
            {
             static const char CODE_BASED pFmt[]=
                                      "\nGrand Total of %ld bytes in %ld files";

               LockCode();
               wsprintf(work_buf, pFmt,
                        (DWORD)grand_ttl_size,
                        (DWORD)grand_file_cnt - grand_dir_cnt);
               UnlockCode();
            }
            else
            {
             static const char CODE_BASED pFmt[]=
                                         "\nGrand Total of 1 byte in %ld files";

               LockCode();
               wsprintf(work_buf, pFmt, (DWORD)grand_file_cnt - grand_dir_cnt);
               UnlockCode();
            }
         }

         if(grand_dir_cnt)
         {
            if(grand_file_cnt == grand_dir_cnt && !grand_ttl_size)
            {
             static const char CODE_BASED pFmt[]=
                                          "\nGrand Total of ";

               lstrcpy(work_buf, pFmt);
            }
            else
            {
             static const char CODE_BASED pFmt[]=
                                          " and ";

               lstrcat(work_buf, pFmt);
            }

            if(grand_dir_cnt == 1)
            {
             static const char CODE_BASED pFmt[]=
                                         "1 directory";

               lstrcat(work_buf, pFmt);
            }
            else
            {
             static const char CODE_BASED pFmt[]=
                                         "%ld directories";

               wsprintf(work_buf + lstrlen(work_buf), pFmt,
                        (DWORD)grand_dir_cnt);
            }
         }

         lstrcat(work_buf, ".\r\n");

         PrintString(work_buf);            /* print the # of bytes/files message */



         // Print the 'Grand total disk usage/free space/effiency' message

         pause_ctr+=2;

         if(pause && redirect_output==HFILE_ERROR)
         {
            CMDDirProcessPause(&pause_ctr);
         }

         if(CMDDirCheckBreakAndStall(redirect_output, &dwTickCount))
         {
            AbnormalTermination = TRUE;
            break;
         }


         if(grand_total_usage)                    /* if there is any space being used */
         {
          static const char CODE_BASED pFmt[]=
                 "Total Disk Usage: %ld bytes   Free: %ld bytes   Efficiency:  %2d%%\r\n\n";

            LockCode();
            wsprintf(work_buf, pFmt,
                     (DWORD)grand_total_usage, (DWORD)free_space,
                     (int)(100.0 * (double)grand_ttl_size/(double)grand_total_usage + .5));
            UnlockCode();
         }
         else
         {
          static const char CODE_BASED pFmt[]=
                               "Total Disk Usage:  0 bytes   Free: %ld bytes\r\n\n";

            LockCode();
            wsprintf(work_buf, pFmt, (DWORD)free_space);
            UnlockCode();
         }

         PrintString(work_buf);       /* print the disk usage/free space message */


         if(compress && dblspace_seq!=-1)
         {
            pause_ctr++;

            if(pause && redirect_output==HFILE_ERROR)
            {
               CMDDirProcessPause(&pause_ctr);
            }

            if(CMDDirCheckBreakAndStall(redirect_output, &dwTickCount))
            {
               AbnormalTermination = TRUE;
               break;
            }


            PrintErrorMessage(753);    // "Overall Average Compression Ratio:  "

            PrintString(DblspaceCompressionRatio(grand_ttl_size, grand_ttl_size2));

            PrintString(pCRLF);
         }
      }
   }


   rval = FALSE;                           /* GOOD return!! */

   END_LEVEL

   ReDirectOutput(lpParseInfo,FALSE);

   if(lpTree)      GlobalFreePtr((LPSTR)lpTree);

   if(lpDirPath)   GlobalFreePtr((LPSTR)lpDirPath);
   if(lpParseInfo) GlobalFreePtr((LPSTR)lpParseInfo);

   MySetCursor(hOldCursor);                  /* restore original cursor!! */

   if(AbnormalTermination) return(TRUE);

   return(rval==DIRINFO_ERROR?TRUE:CMDErrorCheck(rval));

}


void NEAR PASCAL CMDDirProcessPause(int FAR *lpPauseCtr)
{
int oldcurline = curline;  // save current line
WORD actual_text_height, lines_per_screen;
RECT rct;



   if(ctrl_break) return;

   if(!TMFlag)
     UpdateTextMetrics(NULL);
        
   actual_text_height = tmMain.tmHeight + tmMain.tmExternalLeading;

   GetClientRect(hMainWnd, &rct);
   lines_per_screen = (rct.bottom - rct.top - 1) / actual_text_height;


   if(*lpPauseCtr > (lines_per_screen - 1))
   {
      // this code stolen from 'CMDPause()' (below)

      PrintErrorMessage(796);  // "Press <ENTER> to continue:  "

      pausing_flag = TRUE;      /* assign global flag TRUE to cause pause */

      while(pausing_flag)
      {
         if(LoopDispatch())    /* returns TRUE on WM_QUIT */
         {
            PrintErrorMessage(635);

            ctrl_break = TRUE; // fakes things out for abnormal termination
            return;
         }
      }

      curline = oldcurline;
      *lpPauseCtr = *lpPauseCtr % (lines_per_screen - 1);
            // this ensures that a one-time output of more than the
            // # of lines on the screen won't screw it up beyond
            // all possibility of sanity or recognition.


      PrintErrorMessage(751);  // "\r\033[K"
                        // deletes to end of line from left column
   }
}


BOOL NEAR PASCAL CMDDirCheckBreakAndStall(BOOL redirect_output,
                                          DWORD FAR *lpdwTickCount)
{
   if(ctrl_break) return(TRUE);

   if(redirect_output==HFILE_ERROR)
   {
      while(stall_output && !ctrl_break)
      {
         MthreadSleep(100);  // sleep .1 sec until output no longer stalled!

         if(!ctrl_break) ctrl_break = LoopDispatch();

         *lpdwTickCount = GetTickCount();
      }
   }

   if(ctrl_break) stall_output = FALSE;

   if(!ctrl_break && (GetTickCount() - *lpdwTickCount) >= 50)
   {
                               // force a yield at least 20 times per second
      if(LoopDispatch())
      {
         ctrl_break = TRUE;    // abnormal terminate
      }
      else
      {
         *lpdwTickCount = GetTickCount();
      }
   }

   return(ctrl_break);
}




LPSTR lpCMDDirSortSwitchArgs;

BOOL FAR PASCAL CMDDirSort(LPDIR_INFO lpDirInfo, LPSTR lpSwitchArgs,
                           WORD wNFiles)
{
   lpCMDDirSortSwitchArgs = lpSwitchArgs;

   _lqsort((LPSTR)lpDirInfo, wNFiles, sizeof(DIR_INFO), CMDDirSortProc);

   return(FALSE);

}


int FAR _cdecl CMDDirSortProc(LPSTR l1, LPSTR l2)
{
LPSTR lp1;
BOOL minus;
int rval;
LPDIR_INFO lpDI1, lpDI2;


   lp1 = lpCMDDirSortSwitchArgs;
   lpDI1 = (LPDIR_INFO)l1;
   lpDI2 = (LPDIR_INFO)l2;


   if(*lp1==0)  lp1 = "GNE";           /* default sort if none specified!! */

   while(*lp1!=0)
   {
      minus = (*lp1=='-');
      if(minus) lp1++;

      switch(toupper(*lp1))
      {
         case 'G':           /* sub-directory */

            if(lpDI1->attrib & _A_SUBDIR)
            {
               if(!(lpDI2->attrib & _A_SUBDIR))
                  rval = -1;
               else
                  rval = 0;
            }
            else
            {
               if(lpDI2->attrib & _A_SUBDIR)
                  rval = 1;
               else
                  rval = 0;
            }
            break;

         case 'N':

            rval = _fstrnicmp(lpDI1->name, lpDI2->name, sizeof(lpDI1->name));
            break;

         case 'E':

            rval = _fstrnicmp(lpDI1->ext, lpDI2->ext, sizeof(lpDI1->ext));
            break;

         case 'S':

            rval = (lpDI1->size==lpDI2->size)? 0 :
                      (lpDI1->size>lpDI2->size)? 1 : -1;
            break;

         case 'D':

            if(lpDI1->date==lpDI2->date)
            {
               rval = (lpDI1->time==lpDI2->time)? 0 :
                        (lpDI1->time>lpDI2->time)? 1 : -1;
            }
            else
            {
               rval = (lpDI1->date>lpDI2->date)? 1 : -1;
            }
            break;

         case 'P':      /* 'program' file sort */

            if(_fstrnicmp(lpDI1->ext, "EXE", sizeof(lpDI1->ext))==0 ||
               _fstrnicmp(lpDI1->ext, "COM", sizeof(lpDI1->ext))==0 ||
               _fstrnicmp(lpDI1->ext, "BAT", sizeof(lpDI1->ext))==0 ||
               _fstrnicmp(lpDI1->ext, "PIF", sizeof(lpDI1->ext))==0)
            {
               if(_fstrnicmp(lpDI2->ext, "EXE", sizeof(lpDI2->ext))==0 ||
                  _fstrnicmp(lpDI2->ext, "COM", sizeof(lpDI2->ext))==0 ||
                  _fstrnicmp(lpDI1->ext, "BAT", sizeof(lpDI2->ext))==0 ||
                  _fstrnicmp(lpDI1->ext, "PIF", sizeof(lpDI2->ext))==0)
               {
                  rval = 0;  /* 'equal' (both programs) */
               }
               else
               {
                  rval = -1; /* first is '<' (before) second */
               }
            }
            else if(_fstrnicmp(lpDI2->ext, "EXE", sizeof(lpDI2->ext))==0 ||
                    _fstrnicmp(lpDI2->ext, "COM", sizeof(lpDI2->ext))==0 ||
                    _fstrnicmp(lpDI1->ext, "BAT", sizeof(lpDI2->ext))==0 ||
                    _fstrnicmp(lpDI1->ext, "PIF", sizeof(lpDI2->ext))==0)
            {
               rval = 1;      /* first is '>' (after) second */
            }
            else
            {
               rval = 0;      /* 'equal' (neither are programs) */
            }

            break;


         default:
            rval = 0;         /* an invalid switch means 'equal' - oh well */

      }

      if(minus) rval = -rval;    /* 'opposite' order specified on this one */

      if(rval)   return(rval);  /* already different - no need to continue */

      lp1++;                    /* get next item in sort specification */
   }

   return(0);                           /* if it gets here, they'e equal!! */

}



#pragma code_seg("CMDEcho_TEXT","CODE")


BOOL FAR PASCAL CMDEcho    (LPSTR lpCmd, LPSTR lpArgs)
{
LPSTR lp1;


   if(_fstricmp(lpCmd,"ECHO.")!=0)
   {
      if(BatchMode)
      {
         if(_fstricmp(lpArgs, "OFF")==0)
         {
            BatchEcho = FALSE;
            return(FALSE);
         }
         else if(lpArgs==(LPSTR)NULL || *lpArgs==0 ||
                 _fstricmp(lpArgs, "ON")==0)
         {
            BatchEcho = TRUE;
            return(FALSE);
         }
      }
      else
      {
         if(lpArgs==(LPSTR)NULL || *lpArgs==0 ||
            _fstricmp(lpArgs, "OFF")==0 || _fstricmp(lpArgs,"ON")==0)
         {
            PrintErrorMessage(578);
            return(FALSE);
         }
      }
   }

   if(lp1 = EvalString(lpArgs))
   {
      while(!ctrl_break && stall_output)
      {
         MthreadSleep(50);
         if(!ctrl_break) ctrl_break = LoopDispatch();
      }

      if(ctrl_break)
      {
         PrintString(pCTRLC);
         PrintString(pCRLF);

         stall_output = FALSE;
      }
      else
      {
         PrintString(lp1);
      }

      GlobalFreePtr(lp1);
   }

   PrintString(pCRLF);

   if(!BatchMode || BatchEcho)
   {
      PrintString(pCRLF);        /* extra <LF> if not batch mode or echoing */
   }

   if(ctrl_break)
   {
      if(!BatchMode) ctrl_break = FALSE;  // if not BATCH eat ctrl break

      return(TRUE);  // return ERROR if control break!
   }

   return(FALSE);

}


#pragma code_seg("CMDEdit_TEXT","CODE")


BOOL FAR PASCAL CMDEdit    (LPSTR lpCmd, LPSTR lpArgs)
{
BOOL rval;


   lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   rval = CMDRunProgram(lpCmd, lpArgs, SW_SHOWNORMAL);
   GlobalFreePtr(lpArgs);

   return(rval);
}



#pragma code_seg("CMDExpand_TEXT","CODE")


BOOL FAR PASCAL CMDExpand  (LPSTR lpCmd, LPSTR lpArgs)
{
BOOL rval;


   lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   rval = CMDRunProgram(lpCmd, lpArgs, SW_SHOWNORMAL);
   GlobalFreePtr(lpArgs);

   return(rval);
}



#pragma code_seg("CMDFc_TEXT","CODE")


BOOL FAR PASCAL CMDFc      (LPSTR lpCmd, LPSTR lpArgs)
{
BOOL rval;


   lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   rval = CMDRunProgram(lpCmd, lpArgs, SW_SHOWNORMAL);
   GlobalFreePtr(lpArgs);

   return(rval);
}


#pragma code_seg("CMDFind_TEXT","CODE")


BOOL FAR PASCAL CMDFind    (LPSTR lpCmd, LPSTR lpArgs)
{
BOOL rval;


   lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   rval = CMDRunProgram(lpCmd, lpArgs, SW_SHOWNORMAL);
   GlobalFreePtr(lpArgs);

   return(rval);
}




#pragma code_seg("CMDFormat_TEXT","CODE")


char NEAR *pValidFormats[]={//"160","160K","160KB",
                            //"180","180K","180KB",
                            //"320","320K","320KB",
                            "360","360K","360KB",
                            "720","720K","720KB",
                            "1200","1200K","1200KB","1.2","1.2M","1.2MB",
                            "1440","1440K","1440KB","1.44","1.44M","1.44MB",
                            "2880","2880K","2880KB","2.88","2.88M","2.88MB"
                            };

WORD pFormatSize[]={//160,160,160,180,180,180,320,320,320,
                    360,360,360,720,720,720,
                    1200,1200,1200,1200,1200,1200,
                    1440,1440,1440,1440,1440,1440,
                    2880,2880,2880,2880,2880,2880
                    };




BOOL FAR PASCAL CMDFormat  (LPSTR lpCmd, LPSTR lpArgs)
{
LPPARSE_INFO lpParseInfo;
UINT i, j, k;
UINT wDrive, wFormatSize, wTracks, wSectors, wHeads, wSwitchFlag, wBootDrive, wDriveType;
LPSTR lpSwitch, lp1;
BOOL is_unconditional=FALSE, wTSFlag=FALSE, wLabelFlag=FALSE,
     wQuickFormat=FALSE;
/* DPB dpb; */
char Label[64];  /* a few extra chars, just in case! */


   lpParseInfo = CMDLineParse(lpArgs);

   if(CMDCheckParseInfo(lpParseInfo,
                        "vVqQuUfFbBsStTnN148", 1 | CMD_ATMOST))
   {
      GlobalFreePtr(lpParseInfo);
      return(TRUE);
   }


   wBootDrive = 0xffff;  /* flags that the /S switch has not been used */

   if(lpParseInfo->n_parameters==0)
   {
      PrintErrorMessage(754);    // "ENTER DRIVE TO FORMAT: "
      GetUserInput(work_buf);

      if(ctrl_break)
      {
         GlobalFreePtr(lpParseInfo);

         if(!BatchMode) ctrl_break = FALSE;  // eat it if not BATCH

         return(TRUE);  /* user pressed <Ctrl-C> */
      }

      wDrive = toupper(*work_buf);
      if((work_buf[1] && work_buf[1]!=':') || work_buf[2]
         || wDrive<'A' || wDrive>'Z')
      {
         PrintErrorMessage(585);

         GlobalFreePtr(lpParseInfo);
         return(TRUE);
      }
   }
   else if(lpParseInfo->n_parameters==1)
   {
    volatile int itemp;

      itemp = lpParseInfo->n_switches;
      lp1 = (LPSTR)((lpParseInfo->sbpItemArray[itemp]));

      wDrive = toupper(*lp1);
      if((lp1[1] && lp1[1]!=':') || lp1[2]
         || wDrive<'A' || wDrive>'Z')
      {
         PrintErrorMessage(585);

         GlobalFreePtr(lpParseInfo);
         return(TRUE);
      }
   }

    /** Verify that the drive is a REMOVEABLE drive (and get DPB)! **/

   wDrive -= 'A' - 1;  /* drive 'A'==1 ,'B'==2, etc. */

   _asm
   {
      mov ax, 0x4409      /* IOCTL "check drive remote" */
                          /* in reality gets device info word! */
      mov bl, BYTE PTR wDrive
      call DOS3Call

      jc ioctl_was_error

      and dx, 0x1000       /* see if it's a NETWORK device */

      jnz drive_is_network
      jmp drive_is_local

   }
drive_is_network:
      {
         PrintErrorMessage(586);

         GlobalFreePtr(lpParseInfo);
         return(TRUE);
      }

ioctl_was_error:

      {
         PrintString(pERRORREADINGDRIVE);
         PrintAChar(wDrive + 'A' - 1);
         PrintString(pQCRLFLF);

         GlobalFreePtr(lpParseInfo);
         return(TRUE);
      }


   _asm
   {
drive_is_local:

      mov ax, 0x4408      /* IOCTL "check device removable" */

      mov bl, BYTE PTR wDrive
      call DOS3Call

      jc ioctl_was_error

      or al,al
      jz no_problem       /* 0 == removable drive */
   }

      {
         PrintErrorMessage(587);

         GlobalFreePtr(lpParseInfo);
         return(TRUE);
      }


no_problem:



   wDrive = GetPhysDriveFromLogical(wDrive);

   if(wDrive==0xffff)       /* not a valid drive or SUBST/JOIN used */
   {
      PrintErrorMessage(588);

      GlobalFreePtr(lpParseInfo);
      return(TRUE);
   }


   if(GetDriveParms(wDrive, &wDriveType, &wTracks, &wSectors, &wHeads, NULL))
   {
      PrintErrorMessage(589);

      GlobalFreePtr(lpParseInfo);
      return(TRUE);
   }

   wTracks++;   /* these two parameters are ZERO-BASED! */
   wHeads++;
   wFormatSize = 0;
   wSwitchFlag = 0;
   *Label = 0;

   for(i=0; i<(lpParseInfo->n_switches); i++)
   {
      lpSwitch = (LPSTR)(lpParseInfo->sbpItemArray[i]);

      j = toupper(*lpSwitch);

      if(j=='U')
      {
         is_unconditional = TRUE;
      }
      else if(j=='F')
      {
         if(wTSFlag)
         {
            PrintErrorMessage(590);

            GlobalFreePtr(lpParseInfo);
            return(TRUE);
         }

         if(lpSwitch[1]!=':')
         {
            PrintString(pSYNTAXERROR);
            PrintString(lpSwitch);
            PrintString(pQCRLFLF);

            GlobalFreePtr(lpParseInfo);
            return(TRUE);
         }

         for(k=0, wFormatSize=0; k<N_DIMENSIONS(pValidFormats); k++)
         {
            if(!_fstricmp(lpSwitch + 2, pValidFormats[k]))
            {
               wFormatSize = pFormatSize[k];
               break;
            }
         }

         if(!wFormatSize)
         {
            PrintErrorMessage(591);
            PrintString(lpSwitch);
            PrintString(pQCRLFLF);

            GlobalFreePtr(lpParseInfo);
            return(TRUE);
         }
      }
      else if(j=='N')
      {
         if(wFormatSize)
         {
            PrintErrorMessage(590);

            GlobalFreePtr(lpParseInfo);
            return(TRUE);
         }

         if(lpSwitch[1]!=':')
         {
            PrintString(pSYNTAXERROR);
            PrintString(lpSwitch);
            PrintString(pQCRLFLF);

            GlobalFreePtr(lpParseInfo);
            return(TRUE);
         }

         lstrcpy(work_buf, lpSwitch + 2);
         wSectors = atoi(work_buf);

         wTSFlag = TRUE;

      }
      else if(j=='T')
      {
         if(wFormatSize)
         {
            PrintErrorMessage(590);

            GlobalFreePtr(lpParseInfo);
            return(TRUE);
         }

         if(lpSwitch[1]!=':')
         {
            PrintString(pSYNTAXERROR);
            PrintString(lpSwitch);
            PrintString(pQCRLFLF);

            GlobalFreePtr(lpParseInfo);
            return(TRUE);
         }

         lstrcpy(work_buf, lpSwitch + 2);
         wTracks = atoi(work_buf);

         wTSFlag = TRUE;

      }
      else if(j=='V')
      {
         if(lpSwitch[1]!=':')
         {
            PrintString(pSYNTAXERROR);
            PrintString(lpSwitch);
            PrintString(pQCRLFLF);

            GlobalFreePtr(lpParseInfo);
            return(TRUE);
         }

         lstrcpy(Label, lpSwitch + 2);

         wLabelFlag = TRUE;

      }
      else if(j=='Q')
      {
         wQuickFormat = TRUE;
      }
      else if(j=='S')
      {
         if(lpSwitch[1] && (lpSwitch[1]!=':' ||
                            toupper(lpSwitch[2])<'A' ||
                            toupper(lpSwitch[2])>'Z' ||
                            lpSwitch[3]!=0 ))
         {
            PrintString(pSYNTAXERROR);
            PrintString(lpSwitch);
            PrintString(pQCRLFLF);

            GlobalFreePtr(lpParseInfo);
            return(TRUE);
         }

         if(lpSwitch[1])
         {
            wBootDrive = toupper(lpSwitch[2]) - 'A' + 1;
         }
         else
         {
            wBootDrive = 0;
         }

      }
      else if(j=='8' || j=='1')
      {
         PrintErrorMessage(592);
         PrintAChar(j);
         PrintErrorMessage(593);

         GlobalFreePtr(lpParseInfo);
         return(TRUE);
      }
   }

   switch(wFormatSize)        /* if we selected a format size... */
   {                           /* assume 2.88 drive is type '5' */
      case 360:
         if(wDriveType==1 || wDriveType==2)
         {
            wTracks = 40;
            wSectors = 9;
            wHeads = 2;
            break;
         }
         else
         {
            PrintErrorMessage(594);
            GlobalFreePtr(lpParseInfo);
            return(TRUE);
         }


      case 720:
         if(wDriveType==3 || wDriveType==4 || wDriveType==5)
         {
            wTracks = 80;
            wSectors = 9;
            wHeads = 2;
            break;
         }
         else
         {
            PrintErrorMessage(594);
            GlobalFreePtr(lpParseInfo);
            return(TRUE);
         }


      case 1200:
         if(wDriveType==2)
         {
            wTracks = 80;
            wSectors = 15;
            wHeads = 2;
            break;
         }
         else
         {
            PrintErrorMessage(594);
            GlobalFreePtr(lpParseInfo);
            return(TRUE);
         }


      case 1440:
         if(wDriveType==4 || wDriveType==5)
         {
            wTracks = 80;
            wSectors = 18;
            wHeads = 2;
            break;
         }
         else
         {
            PrintErrorMessage(594);
            GlobalFreePtr(lpParseInfo);
            return(TRUE);
         }


      case 2880:  /* can't support at this time, so 'ignore' it... */
         break;

   }


   if(!wLabelFlag)
   {
      PrintErrorMessage(755);   // "VOLUME LABEL (<ENTER> for none)? "

      GetUserInput(Label);

      if(ctrl_break)
      {
         PrintErrorMessage(595);
         GlobalFreePtr(lpParseInfo);

         if(!BatchMode) ctrl_break = FALSE; // eat it if not BATCH mode

         return(TRUE);
      }
   }

   if(wBootDrive!=0xffff) /* a /S switch was found */
   {
      wSwitchFlag |= FORMAT_SYSTEMFILES;
      wSwitchFlag |= (wBootDrive & FORMAT_BOOTDRIVEMASK);
   }

   if(wQuickFormat)      wSwitchFlag |= FORMAT_QUICKFORMAT;
   if(is_unconditional)  wSwitchFlag |= FORMAT_UNCONDITIONAL;

   SubmitFormatRequest(wDrive, wTracks, wHeads, wSectors, wSwitchFlag,
                       (LPSTR)Label);

//   FormatDrive(wDrive, 0, 0, 0); /* wTracks, wHeads, wSectors); */

   GlobalFreePtr(lpParseInfo);
   return(FALSE);

}



// NOTE:  This function, unlike others, shares a segment with something else...
//        in this case it's with the 'ProcessCommand()' functions, which
//        also share the same segment as the command array.

#pragma code_seg("PROCESS_COMMAND_TEXT")

BOOL FAR PASCAL CMDHelp    (LPSTR lpCmd, LPSTR lpArgs)
{
int i;
LPSTR lp1 /*, lp2 */;
BOOL rval;
LPPARSE_INFO lpParseInfo;
static const char CODE_BASED szCOMMANDS[]  = "COMMANDS";
static const char CODE_BASED szKEYS[]      = "KEYS";
static const char CODE_BASED szVARIABLES[] = "VARIABLES";
static const char CODE_BASED szANSI[]      = "ANSI";


   lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   lpParseInfo = CMDLineParse(lpArgs);

   if(!lpParseInfo)
   {
      PrintString(pNOTENOUGHMEMORY);

      GlobalFreePtr(lpArgs);
      return(TRUE);
   }

   if(lpParseInfo->n_switches || lpParseInfo->n_parameters > 1)
   {
      PrintString(pSYNTAXERROR);
      PrintString(lpArgs);
      PrintString(pQCRLFLF);

      GlobalFreePtr(lpParseInfo);
      return(TRUE);
   }

   GlobalFreePtr(lpArgs);


   if(lpParseInfo->n_parameters==1)
   {
      lp1 = lpParseInfo->sbpItemArray[lpParseInfo->n_switches];
   }
   else
   {
      lp1 = NULL;
   }

   ReDirectOutput(lpParseInfo,TRUE);


   rval = TRUE;  // error flag...

   if(!lp1 || !*lp1 || _fstricmp(lp1, szCOMMANDS)==0)
   {
      CMDHelpPrintCommands();
      rval = FALSE;
   }
   else if(_fstricmp(lp1, szKEYS)==0)
   {
      CMDHelpPrintKeys();
      rval = FALSE;
   }
   else if(_fstricmp(lp1, szVARIABLES)==0)
   {
      CMDHelpPrintVariables();
      rval = FALSE;
   }
   else if(_fstricmp(lp1, szANSI)==0)
   {
      CMDHelpPrintAnsi();

      rval = FALSE;
   }
   else for(i=0; i<(int)wNcmd_array; i++)
   {
      if(_fstricmp(lp1, cmd_array[i])==0)
      {
         CMDHelpPrintString(i);
         PrintString(pCRLF);

         rval = FALSE;
         break;
      }
   }


   if(rval)
   {
      PrintErrorMessage(600);
      PrintString(lp1);
      PrintString(pQCRLFLF);
   }

   ReDirectOutput(lpParseInfo,FALSE);
   GlobalFreePtr(lpParseInfo);

   return(rval);

}


void FAR PASCAL CMDHelpPrintString(WORD i)
{

   _asm push cs
   _asm call LockSegment

   if(i>=(WORD)_HELP_START_[0] &&
      i < ((WORD)_HELP_START_[1] + (WORD)_HELP_START_[0]) )
   {
      PrintStringX(_HELP_START_[i + 2 - (WORD)_HELP_START_[0]]);

      _asm push cs
      _asm call UnlockSegment
   }
   else
   {
      _asm push cs
      _asm call UnlockSegment

      CMDHelpPrintString2(i);
   }

}








#pragma code_seg("_HELP_TEXT2_","CODE")

void FAR PASCAL CMDHelpPrintCommands(void)
{

   LockCode();

   PrintStringX(_HELP_COMMANDS_);

   UnlockCode();

}

void FAR PASCAL CMDHelpPrintKeys(void)
{

   LockCode();

   PrintStringX(_HELP_KEYS_);

   UnlockCode();

}

void FAR PASCAL CMDHelpPrintVariables(void)
{

   LockCode();

   PrintStringX(_HELP_VARIABLES_);

   UnlockCode();

}

void FAR PASCAL CMDHelpPrintAnsi(void)
{

   LockCode();

   PrintStringX(_HELP_ANSI_);

   UnlockCode();

}



void FAR PASCAL CMDHelpPrintString2(WORD i)
{

   LockCode();

   if(i>=(WORD)_HELP_START2_[0] &&
      i < ((WORD)_HELP_START2_[1] + (WORD)_HELP_START2_[0]) )
   {
      PrintStringX(_HELP_START2_[i + 2 - (WORD)_HELP_START2_[0]]);

      UnlockCode();
   }
   else
   {
      UnlockCode();

//      CMDHelpPrintString3(i);
   }

}



#pragma code_seg("CMDInput_TEXT","CODE")


BOOL FAR PASCAL CMDInput   (LPSTR lpCmd, LPSTR lpArgs)
{
LPSTR lp1, lp2;
BOOL rval;


   lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   if(!*lpArgs)
   {
      PrintErrorMessage(604);
      return(TRUE);
   }

   for(lp1 = lpArgs, lp2 = (LPSTR)work_buf; *lp1>' '; lp1++, lp2++)
   {
      *lp2 = *lp1;
   }

   *lp2++ = 0;   /* 'lp2' points to next available area of buffer - nice! */

   lp1++;        /* the first (separating) space is a separator, so do  */
                 /* not print it (increment 'lp1' 1 past it).  However, */
                 /* if there are any more spaces, include those.        */

   _strupr(work_buf);

   if(*lp1)
   {
      PrintString(lp1);

      if(lp1[lstrlen(lp1)-1]>' ')  /* 'lp1' doesn't end with a space */
      {
         PrintAChar(' ');          /* so add one just for kicks! */
      }
   }
   else
   {
      PrintString(work_buf);
      PrintAChar('?');
      PrintAChar(' ');
   }

   GetUserInput(lp2);           /* now - what does the user say about it? */

   if(ctrl_break)
   {
      GlobalFreePtr(lpArgs);

      // in THIS case, do *NOT* eat the ctrl break no matter what!

      return(TRUE);  /* he pressed ^C, that's what! */
   }


   lp1 = lp2 + lstrlen(lp2) - 2;

             /* trim any trailing '\r', '\n', or '\r\n' */

   if(*lp1=='\r' && lp1[1]=='\n')        *lp1=0;
   else if(lp1[1]=='\r' || lp1[1]=='\n') lp1[1]=0;

   rval = SetEnvString(work_buf, lp2); /* and finally - make it happen! */

   GlobalFreePtr(lpArgs);
   return(rval);

}



#pragma code_seg("CMDJoin_TEXT","CODE")


BOOL FAR PASCAL CMDJoin    (LPSTR lpCmd, LPSTR lpArgs)
{
LPSTR lpLOL;
DWORD dwDosVer;
LPCURDIR lpC;
WORD w, wNDrives, count;


   if(*lpArgs)
   {
      return(CMDNotYet(lpCmd, lpArgs));
   }


   dwDosVer = GetVersion();  /* HIWORD == Dos Version */

   _asm
   {
      push es
      mov ax,0x5200
      int 0x21

      mov ax, 0
      mov dx, ax
      jc bad_news

      mov dx, es
      mov ax, bx

bad_news:
      mov WORD PTR lpLOL, ax
      mov WORD PTR lpLOL+2, dx

      pop es
   }


   if(!lpLOL)
   {
      PrintErrorMessage(605);
      return(TRUE);
   }

   if(HIWORD(dwDosVer)>=0x310)  /* DOS 3.1 or later */
   {
      wNDrives = *((BYTE FAR *)(lpLOL + 0x21));
   }
   else
   {
      wNDrives = *((BYTE FAR *)(lpLOL + 0x1b));
   }

   for(count=0, w=1; w<=wNDrives; w++)
   {
      lpC = (LPCURDIR)GetDosCurDirEntry(w);

      if(!lpC) continue;  /* ignore NULL returns */

      if((lpC->wFlags & CURDIR_TYPE_MASK)==CURDIR_TYPE_INVALID) continue;

      if(lpC->wFlags & CURDIR_JOIN)
      {
         /* this is a JOIN'ed drive! */

         PrintAChar(w + 'A' - 1);
         PrintErrorMessage(756);   // ": => "

         _fmemcpy(work_buf, lpC->pathname, lpC->wPathRootOffs);
         if(lpC->wPathRootOffs<=2)
         {
            work_buf[lpC->wPathRootOffs] = '\\';
            work_buf[lpC->wPathRootOffs+1] = 0;
         }
         else
         {
            work_buf[lpC->wPathRootOffs] = 0;
         }

         PrintString(work_buf);
         PrintString(pCRLF);

         count++;
      }

      MyFreeSelector(SELECTOROF(lpC));  /* don't forget to do this! */
   }

   if(!count)
   {
      PrintErrorMessage(606);
   }

   return(FALSE);
}



#pragma code_seg("CMDKilltask_TEXT","CODE")


BOOL FAR PASCAL CMDKilltask(LPSTR lpCmd, LPSTR lpArgs)
{
HTASK hTask, FAR *lphTask;
LPPARSE_INFO lpParseInfo;
TASKENTRY te;
WNDENUMPROC lpProc;
WORD w, wCount;


   lpParseInfo = CMDLineParse(lpArgs);
   if(CMDCheckParseInfo(lpParseInfo, NULL, 1 | CMD_EXACT))
   {
      GlobalFreePtr(lpParseInfo);
      return(TRUE);
   }

   GlobalFreePtr((LPSTR)lpParseInfo);  /* who needs it now? */

   if(!(hTask = (HTASK)MyValue(lpArgs)))
   {

     /** It's not a valid handle, so look for a module name that matches **/

      _fmemset((LPSTR)&te, 0, sizeof(te));

      te.dwSize = sizeof(te);

      if(!lpTaskFirst || !lpTaskNext || !lpTaskFirst((LPTASKENTRY)&te))
      {
         PrintErrorMessage(607);
         return(TRUE);
      }

      wCount = 0;
      lphTask = (HTASK FAR *)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                            sizeof(HTASK) * GetNumTasks());
      if(!lphTask)
      {
         PrintString(pNOTENOUGHMEMORY);
         return(TRUE);
      }

      do
      {
         if(lpModuleFindHandle)
         {
          MODULEENTRY me;

            _fmemset((LPVOID)&me, 0, sizeof(me));

            me.dwSize = sizeof(me);

            if(!lpModuleFindHandle(&me, te.hModule)) continue; // error

            if(!_fstricmp(me.szModule, lpArgs))
            {
               lphTask[wCount++] = te.hTask;
            }
         }
         else if(!_fstricmp(te.szModule, lpArgs))
         {
            lphTask[wCount++] = te.hTask;
         }

      } while(lpTaskNext((LPTASKENTRY)&te));

      if(wCount>1)
      {
         lpProc = (WNDENUMPROC)MakeProcInstance((FARPROC)CMDCloseTaskEnum2,
                                                hInst);

         PrintErrorMessage(538);  // "** MULTIPLE Tasks have this module name **\r\n"
                                  // "Entry  Task Handle  Window Caption\r\n"
                                  // "-----  -----------  --------------\r\n"

         for(w=0; w<wCount; w++)
         {
            if(lpTaskFindHandle((LPTASKENTRY)&te, lphTask[w]))
            {
             static const char CODE_BASED pFmt[]=
                                                   " %2d)     %04x       ";

               LockCode();
               wsprintf(work_buf, pFmt, w+1, (WORD)te.hTask);
               UnlockCode();

               PrintString(work_buf);

               CloseTaskEnumTask = lphTask[w];
               CloseTaskEnumCount = 0;

               EnumWindows(lpProc, (LPARAM)NULL);

               if(!CloseTaskEnumCount) PrintErrorMessage(539); // " ** N/A **\r\n"

               PrintString(pCRLF);
            }
         }

         FreeProcInstance((FARPROC)lpProc);

         PrintErrorMessage(608);     // "Which Task to KILL (1,2,3,etc.) or <Return> to cancel? "
         GetUserInput(work_buf);

         if(*work_buf<' ')
         {
            PrintErrorMessage(541);  // "** CANCELLED **\r\n\n"

            GlobalFreePtr(lphTask);
            return(TRUE);
         }
         else if((w = (WORD)MyValue(work_buf))>wCount || w<1)
         {
            PrintErrorMessage(609);
            PrintString(work_buf);
            PrintString(pQCRLFLF);

            GlobalFreePtr(lphTask);
            return(TRUE);
         }
         else
         {
            hTask = lphTask[w - 1];
         }

      }
      else if(wCount==0)
      {
         PrintErrorMessage(610);
         PrintString(lpArgs);
         PrintString(pQCRLFLF);

         GlobalFreePtr(lphTask);
         return(TRUE);
      }
      else
      {
         hTask = lphTask[0];
      }

      GlobalFreePtr(lphTask);
   }

   if(!lpIsTask(hTask))
   {
    static const char CODE_BASED pFmt[]=
                                     "?Task ID %04xH(%d) not found...\r\n\n";

      LockCode();
      wsprintf(work_buf, pFmt, (WORD)hTask, (WORD)hTask);
      UnlockCode();

      PrintString(work_buf);
      return(TRUE);
   }


   PostAppMessage(hTask, WM_QUIT, (WPARAM)0, (LPARAM)0L);

   return(FALSE);
}


#pragma code_seg("CMDLabel_TEXT","CODE")


BOOL FAR PASCAL CMDLabel   (LPSTR lpCmd, LPSTR lpArgs)
{
char label_buf[64];
static char old_label[16];
LPSTR lp1, lp2;
static WORD w;
FCB fcb;
DWORD dwDosSeg;
WORD wDrive;
REALMODECALL rmc;



   lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }


   for(lp1=lpArgs; *lp1 && *lp1<=' '; lp1++)
      ;  // find first non-white-space


   if(!*lp1)
   {
      work_buf[0] = _getdrive() + 'A' - 1;
      work_buf[1] = ':';
      work_buf[2] = 0;

      lp2 = lp1;
   }
   else if(!(lp2 = _fstrchr(lp1, ':')))
   {
      PrintString(pSYNTAXERROR);
      PrintString(lpArgs);
      PrintString(pCRLFLF);

      GlobalFreePtr(lpArgs);
      return(TRUE);
   }
   else
   {
      _fstrupr(lp1);

      _fstrncpy(work_buf, lp1, (lp2 - lp1) + 1);

      work_buf[lp2 - lp1 + 1] = 0;

      lp2++;
   }


         /** Find out the CANONICAL drive designation **/


   strcpy(work_buf + sizeof(work_buf)/2, work_buf);  // temporary

   strcat(work_buf, "\\");

   DosQualifyPath(work_buf, work_buf);
                                           /* get the DOS qualified path */

   if(!*work_buf)
   {
      PrintString(pERRORREADINGDRIVE);
      PrintString(work_buf + sizeof(work_buf)/2);
      PrintString(pQCRLFLF);

      GlobalFreePtr(lpArgs);
      return(TRUE);
   }


   if(*work_buf!='\\')                // NOT a NOVELL NETWORK DRIVE
   {
      _fmemcpy(old_label, work_buf, 3);  /* get 1st 3 bytes of work_buf */
   }
   else
      *old_label = 0;                 // marker for later...


   // at this point it is VERY important to reset the disk drive...

   wDrive = toupper(*work_buf) - 'A' + 1;

   if(IsChicago) _asm
   {
      mov ax, 0x710d
      mov cx, 1           // flush flag:  '1' means flush ALL including cache
      mov dx, wDrive

      int 0x21            // flushes all drive buffers NOW!
   }
   else _asm
   {
      mov ax, 0x0d00
      int 0x21
   }



   if(GetDriveLabel((LPSTR)label_buf, (LPSTR)work_buf))
   {
      PrintString(pERRORREADINGDRIVE);
      PrintString(work_buf + sizeof(work_buf)/2);
      PrintString(pQCRLFLF);

      GlobalFreePtr(lpArgs);
      return(TRUE);
   }


       /* parse args to get 2nd arg (new label), if there is one */

   lp1 = lp2;

   if(*lp1)
   {
      while(*lp1 && *lp1<=' ') lp1++; /* find next NON-BLANK or end of string */
   }



   if(!*lp1 || !*old_label)  // no label on cmd line, or NOVELL NETWORK drive
   {
      lp2 = _fstrchr(work_buf, ':');
      if(lp2) *lp2 = 0;

      if(*label_buf<=' ')  /* blank volume label */
      {
         PrintString(pVOLUMEINDRIVE);

         PrintString(work_buf);

         PrintString(pHASNOLABEL);
         if(*label_buf) PrintString(label_buf);
         PrintString(pCRLF);
      }
      else
      {
         PrintString(pVOLUMEINDRIVE);

         PrintString(work_buf);

         PrintErrorMessage(757);   // "' is "
         PrintString((LPSTR)label_buf);
         PrintString(pCRLF);
      }

      if(!*old_label)      // NOVELL NETWORK DRIVE - can't assign label!
      {
         if(*lp1)
         {
            PrintErrorMessage(611);

            GlobalFreePtr(lpArgs);
            return(TRUE);
         }
         else
         {
            PrintAChar('\n');

            GlobalFreePtr(lpArgs);
            return(FALSE);
         }
      }


      PrintErrorMessage(612);

      GetUserInput(work_buf);

      if(ctrl_break)
      {
         ctrl_break = FALSE;  /* so I don't cancel BATCH files! */

         PrintString(pCRLFLF);

         GlobalFreePtr(lpArgs);
         return(FALSE);
      }

      _fstrtrim(work_buf);  /* trim off garbage and trailing spaces */

   }
   else
   {
      lstrcpy(work_buf, lp1);
   }

   _fstrupr(work_buf);


   /** First step - if the label exists, we need to delete it! **/

   if(*label_buf>' ')           /* there is currently a label! */
   {
      _fmemset((LPSTR)&fcb, 0, sizeof(fcb));

      for(lp1=(LPSTR)fcb.name, lp2=label_buf, w=0; w<11; w++)
      {
         if(*lp2>' ')
         {
            *lp1++ = *lp2++;
         }
         else
         {
            *lp1++ = ' ';
         }
      }

      fcb.drivenum = (BYTE)(*old_label - 'A' + 1); /* drive designation */

      fcb.extFlag = 0xff;

      fcb.attrib = _A_VOLID;  /* volume ID label attribute */

      _fmemset((LPSTR)&rmc, 0, sizeof(rmc));

      dwDosSeg = GlobalDosAlloc(sizeof(fcb) + 512);
      if(!dwDosSeg)
      {
         PrintString(pNOTENOUGHMEMORY);

         GlobalFreePtr(lpArgs);
         return(TRUE);
      }

      _lgetdcwd(fcb.drivenum, work_buf + sizeof(work_buf)/2,
               sizeof(work_buf)/2);

      old_label[4] = 0;
      MyChDir(old_label);         /* change to ROOT DIR on affected drive */

      rmc.SS = rmc.DS = rmc.ES = HIWORD(dwDosSeg);      /* segment value */
      rmc.EDX = 0;                                /* offset for FCB data */

      rmc.EAX = 0x1300;                         /* delete file using FCB */

      rmc.SP = (sizeof(fcb) + 510) & 0xfffe;   /* even value - stack top */

      _fmemcpy(MAKELP(LOWORD(dwDosSeg),0),(LPSTR)&fcb, sizeof(fcb));

      RealModeInt(0x21, &rmc);  /* do a REAL MODE interrupt - FCB delete */

      GlobalDosFree(LOWORD(dwDosSeg));

      MyChDir(work_buf + sizeof(work_buf)/2);        /* restore directory */

      if((rmc.EAX & 0xff)==0xff)  /* error */
      {
         PrintErrorMessage(613);

         GlobalFreePtr(lpArgs);
         return(TRUE);
      }
   }


         /* NEXT: if a label was specified, we need to add it */

   while(*work_buf && *work_buf<=' ') lstrcpy(work_buf, work_buf+1);

   if(*work_buf > ' ')
   {
      for(lp1=label_buf, lp2=work_buf, w=0; w<11; w++)
      {
         if(*lp2>=' ')    // mod 1/27/93 - allow ' ' to be specified in label
         {
            *lp1++ = *lp2++;
         }
         else
         {
            break;
         }
      }

      *lp1 = 0;

      if(lstrlen(label_buf)>8)
      {
         for(lp1=label_buf+12, w=0; w<4; w++, lp1--)
         {
            *lp1 = *(lp1 - 1);
         }
         label_buf[8] = '.';
      }

      lstrcpy(old_label + 3, label_buf);      /* copy to 'old_label' */

      _asm
      {
         mov ax, 0x5b00         /* create a NEW file */
         mov cx, _A_VOLID
         mov dx, OFFSET old_label  /* it's a STATIC var */

         call DOS3Call

         mov w, ax
         jnc label_created

         mov ax, 0xffff
         mov w, ax
label_created:
      }

//      if(_dos_creatnew(old_label, _A_VOLID, &w))
//      {

      if(w==0xffff)
      {
         PrintErrorMessage(614);
         PrintString(old_label);
         PrintString(pQCRLFLF);

         GlobalFreePtr(lpArgs);
         return(TRUE);
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
   }

   GlobalFreePtr(lpArgs);
   return(FALSE);

}



#pragma code_seg("CMDLet_TEXT","CODE")


BOOL FAR PASCAL CMDLet     (LPSTR lpCmd, LPSTR lpArgs)
{
LPSTR lp1, lp2;
WORD w;


   lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   for(lp1=lpArgs; *lp1 && *lp1!='=' && *lp1!='['; lp1++)
      ;  // find the '=', or a '['

   if(*lp1=='[')      // it's an array, guys!
   {

      for(w=1, lp1++; *lp1; lp1++)
      {                       // find the last ']' or end of string
         if(*lp1=='[')      w++;
         else if(*lp1==']') w--;
         else if(*lp1=='"')
         {
            while(*lp1 && (*lp1!='"' || *(lp1 - 1)=='\\'))
               lp1++;  // skip quoted strings
         }
         else if(*lp1=='\'')
         {
            while(*lp1 && (*lp1!='\'' || *(lp1 - 1)=='\\'))
               lp1++;  // skip quoted strings
         }

         if(!w) break;

      }

      if(*lp1!=']')
      {
         PrintErrorMessage(615);
         GlobalFreePtr(lpArgs);
         return(TRUE);
      }

      for(lp1++; *lp1 && *lp1!='='; lp1++)
         ;  // find the '=' or end of string
   }

   if(!*lp1)
   {
      PrintErrorMessage(616);
      GlobalFreePtr(lpArgs);
      return(TRUE);
   }

   *(lp1++) = 0;  // ends the target variable name, points to equation

   lp2 = Calculate(lp1);

   if(!lp2)
   {
      PrintErrorMessage(617);
      GlobalFreePtr(lpArgs);
      return(TRUE);
   }

   CalcAssignVariable(lpArgs, lp2);

   GlobalFreePtr(lp2);
   GlobalFreePtr(lpArgs);

   return(FALSE);

}







/***************************************************************************/

#pragma pack(1)     /* for all of this section, and below */





#pragma code_seg("CMDListOpen_TEXT","CODE")


BOOL FAR PASCAL CMDListOpen(LPSTR lpCmd, LPSTR lpArgs)
{
LPPARSE_INFO lpParseInfo;
LPSTR lpLOL, lpData, lp1, lp2;
LPSFT_INFO lpS, lpS2;
LPSFT lpSFT;
WORD verflag, i, wMySel, wNItems;
DWORD dwVer, dwAddr, dwTickCount;
HTASK hTask;
LPSTR lpModuleName;
HCURSOR hOldCursor;
BOOL AbnormalTermination = FALSE;



   dwTickCount = GetTickCount();


                    /***------------------------***/
                    /*** INITIALIZATION SECTION ***/
                    /***------------------------***/

           /** Allocate space for 256 entries, just because **/
           /** the maximum SFT size is currently 256.       **/

   if(!(lpS = (LPSFT_INFO)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                         max(sizeof(*lpS) * 256, 0x10000L))))
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }


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
      int 0x21
      mov WORD PTR lpLOL+2 ,es
      mov WORD PTR lpLOL, bx
   }



   lpSFT = *((LPSFT FAR *)(lpLOL + 4));  /** Address of System File Table **/


                /** Allocate an LDT selector via DPMI **/

   _asm
   {
      mov ax, 0
      mov cx, 1
      int 0x31     ; allocate 1 selector from LDT
      jnc sel_no_error
      mov ax, 0

sel_no_error:
      mov WORD PTR wMySel, ax

   }

   if(!wMySel)
   {
      PrintErrorMessage(618);
      GlobalFreePtr((LPSTR)lpS);    /* free resources allocated earlier */

      return(TRUE);
   }


                /** Re-direct output and print headings **/

   lpParseInfo = CMDLineParse(lpArgs);
   if(lpParseInfo->n_switches || lpParseInfo->n_parameters)
   {
      PrintErrorMessage(619);
   }

   ReDirectOutput(lpParseInfo,TRUE);

   if(verflag==DOSVER_2X)
   {
      PrintErrorMessage(758);  // "       ** MS-DOS(r) Version 2.x Open Files **\r\n"
                               // " File/Dev Name Open Share Cur File Cur File\r\n"
                               // "Drive:Name.Ext Mode Mode  Pointer   Length \r\n"
                               // "-------------- ---- ----- -------- --------\r\n"
   }
   else
   {
      switch(verflag)
      {
         case DOSVER_30:
            PrintErrorMessage(759); // "                 ** MS-DOS(r) Version 3.0 Open Files **\r\n"
            break;

         case DOSVER_3X:
            PrintErrorMessage(760); // "                 ** MS-DOS(r) Version 3.X Open Files **\r\n"
            break;

         case DOSVER_4PLUS:
            PrintErrorMessage(761); // "               ** MS-DOS(r) Version 4.x-6.x Open Files **\r\n"
            break;

         case DOSVER_7PLUS:
            PrintErrorMessage(864); // "    ** MS-DOS(r) Version 7+ Open Files Using System File Table **\r\n"
            break;

      }

      PrintErrorMessage(762);  // " File/Dev Name Open Share Machine Owner Cur File Cur File Owner Owner     \r\n"
      PrintErrorMessage(763);  // "Drive:Name.Ext Mode Mode    ID     PSP  Pointer   Length  Task  Module    \r\n"
      PrintErrorMessage(764);  // "-------------- ---- ----- ------- ----- -------- -------- ----- ----------\r\n"
   }


                           /***-----------***/
                           /*** MAIN LOOP ***/
                           /***-----------***/


   lpS2 = lpS;
   wNItems = 0;


   do
   {
            /* gotta translate that puppy to *protected* mode */

      dwAddr = SELECTOROF(lpSFT)*16L + OFFSETOF(lpSFT);

      _asm
      {
         mov ax, 0x0007               ; DPMI 'set selector base' function
         mov bx, WORD PTR wMySel
         mov cx, WORD PTR dwAddr + 2
         mov dx, WORD PTR dwAddr      ; address of selector in linear memory

         int 0x31
         jc  was_error

         mov ax, 0x0008               ; DPMI 'set selector limit' function
         mov bx, WORD PTR wMySel
         mov cx, 0
         mov dx, 0xffff               ; segment limit 0xffff

         int 0x31
         jnc no_error

was_error:
         mov bx, 0
         jmp set_lpsft

no_error:
         mov bx, wMySel
set_lpsft:
         mov WORD PTR lpSFT + 2, bx
         mov bx, 0
         mov WORD PTR lpSFT, bx
      }

      if(!lpSFT)
      {
         if(lpParseInfo)
         {
            ReDirectOutput(lpParseInfo,FALSE);
            GlobalFreePtr(lpParseInfo);
         }

         PrintErrorMessage(620);
         GlobalFreePtr((LPSTR)lpS); /* free resources allocated earlier */

         return(TRUE);
      }

      lpData = lpSFT->lpData;       /* pointer to array of 'SFT' entries */

              /** read through 'SFT' array for the total # **/
              /** of 'SFT' entries in this block.          **/

      for(i=0; i<lpSFT->wCount; i++)
      {
         switch(verflag)    /* different structure sizes for */
         {                  /* different DOS versions.       */

            case DOSVER_2X:
               CMDListOpenAddLine0(lpData, &lpS2, &wNItems);
               lpData += sizeof(struct _sft2_);
               break;

            case DOSVER_30:
               CMDListOpenAddLine1(lpData, &lpS2, &wNItems);
               lpData += sizeof(struct _sft30_);
               break;

            case DOSVER_3X:
               CMDListOpenAddLine2(lpData, &lpS2, &wNItems);
               lpData += sizeof(struct _sft31_);
               break;

            case DOSVER_4PLUS:
            case DOSVER_7PLUS:
               CMDListOpenAddLine3(lpData, &lpS2, &wNItems);
               lpData += sizeof(struct _sft4_);
               break;
         }
      }

      lpSFT = lpSFT->lpNext;       /* get (DOS) pointer to next SFT block */

   } while(SELECTOROF(lpSFT) && OFFSETOF(lpSFT)!=0xffff);


   _lqsort((LPSTR)lpS, wNItems, sizeof(*lpS), CMDListOpenCompare);


   for(i=0, lpS2=lpS; i<wNItems; i++, lpS2++)
   {
      if(!ctrl_break &&
         ((i & 5)==0 ||
          (GetTickCount() - dwTickCount) >= 50))  // 20 times per second
      {
         if(LoopDispatch())
         {
            AbnormalTermination = TRUE;
            break;
         }

         dwTickCount = GetTickCount();
      }


      if(redirect_output==HFILE_ERROR)
      {
         while(!ctrl_break && stall_output)
         {
            MthreadSleep(50);
            if(!ctrl_break) ctrl_break = LoopDispatch();
         }
      }

      if(ctrl_break)
      {
         stall_output = FALSE;
         AbnormalTermination = TRUE;

         break;
      }


      if(verflag==DOSVER_2X)
      {
       static const char CODE_BASED pFmt[]=
                                "%-15.15s%-4.4s %-4.4s  %8lx %8lx\r\n";

         LockCode();
         wsprintf(work_buf, pFmt,
                     lpS2->szName,
                     (LPSTR)pOpenMode[lpS2->bMode],
                     (LPSTR)pShareMode[lpS2->bShare],
                     lpS2->dwFilePtr,
                     lpS2->dwFileSize);
         LockCode();
      }
      else
      {
         lpModuleName = GetTaskAndModuleFromPSP(lpS2->wVM, lpS2->wPSP,
                                                &hTask);

         if(hTask)
         {
          static const char CODE_BASED pFmt[]=
             "%-15.15s%-4.4s %-4.4s   %04x   %04x  %08lx %08lx %04x  %s\r\n";

            LockCode();
            wsprintf(work_buf, pFmt,
                     lpS2->szName,
                     (LPSTR)pOpenMode[lpS2->bMode],
                     (LPSTR)pShareMode[lpS2->bShare],
                     lpS2->wVM,
                     lpS2->wPSP,
                     lpS2->dwFilePtr,
                     lpS2->dwFileSize,
                     hTask,
                     lpModuleName);
            UnlockCode();
         }
         else
         {
          static const char CODE_BASED pFmt[]=
             "%-15.15s%-4.4s %-4.4s   %04x   %04x  %08lx %08lx       %s\r\n";

            LockCode();
            wsprintf(work_buf, pFmt,
                     lpS2->szName,
                     (LPSTR)pOpenMode[lpS2->bMode],
                     (LPSTR)pShareMode[lpS2->bShare],
                     lpS2->wVM,
                     lpS2->wPSP,
                     lpS2->dwFilePtr,
                     lpS2->dwFileSize,
                     lpModuleName);
            UnlockCode();
         }
      }

      PrintString(work_buf);

      if(lpS2->ExtraLine[0])
      {
         PrintString(lpS2->ExtraLine);
         PrintString(pCRLF);
      }

   }

                         /**-----------------**/
                         /** CLEANUP SECTION **/
                         /**-----------------**/

   _asm
   {
      mov ax, 1
      mov bx, WORD PTR wMySel
      int 0x31                ; free the selector I allocated earlier
   }


   AbnormalTermination = LoopDispatch();  // always at least once here...


   // NEXT, if I'm running Chicago, attempt to get PM open file info

   if(!AbnormalTermination && (IsChicago || IsNT) && verflag==DOSVER_7PLUS)
   {
    WORD wMode, wType;
    char tbuf[MAX_PATH + 64];
    typedef struct {
       char sz83Name[16];
       WORD wMode, wType;
       char szFullPath[MAX_PATH];
       } WIN95_FILE_INFO, FAR *LPWIN95_FILE_INFO;

    LPWIN95_FILE_INFO lpWFI;

#define MAXWFI (0x10000L / sizeof(WIN95_FILE_INFO))


      lpWFI = (LPWIN95_FILE_INFO)lpS;

      PrintString(pCRLF);
      PrintString(pCRLF);
      PrintErrorMessage(865);  // "         ** MS-DOS(r) Version 7+ Protected Mode Open Files **\r\n"
      PrintErrorMessage(869);  // "  File Name    Open Share Access                                              \r\n"
      PrintErrorMessage(870);  // "Drive:Name.Ext Mode Mode   Type   Directory Name                              \r\n"
      PrintErrorMessage(871);  // "-------------- ---- ----- ------- --------------------------------------------\r\n"

      if(redirect_output==HFILE_ERROR)
      {
         LoopDispatch();  // ignore return value (for now)
      }


      // get list of open files, then sort it

      hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

      for(i=1; i<=26; i++)
      {
         wNItems = 0;

         do
         {
            _asm
            {
               push ds
               mov ax, 0x440d    // IOCTL
               mov bx, i         // drive number
               mov cx, 0x086d    // device cat. 8, func 6d (enum open files)

               push ss
               pop ds
               lea dx, WORD PTR tbuf

               mov si, wNItems   // iteration counter
               mov di, 0         // enumerate ALL files

               int 0x21
               pop ds

               jnc was_ok
               mov wNItems, 0xffff

            was_ok:
               mov wMode, ax
               mov wType, cx
            }


            if(wNItems != 0xffff)
            {
               lstrcpy(lpWFI->szFullPath, tbuf);


               // files will be in the format DRIVE:\PATH..\name.ext

               lp1 = _fstrrchr(tbuf, '\\');

               if(tbuf[0] != '\\' && tbuf[1] == ':')
               {
                  if(!lp1) lp1 = (LPSTR)tbuf + 2;

                  lp2 = _fstrchr(tbuf, '.');
                  if(lp2) *(lp2++) = 0;

                  wsprintf(lpWFI->sz83Name, "%c:%-8.8s.%-3.3s",
                           *tbuf, lp1 + 1, lp2);

                  lpWFI->wMode = wMode;
                  lpWFI->wType = wType;

//                  wsprintf(work_buf, "%c:%-8.8s.%-3.3s %-4.4s %-4.4s  %-7.7s ",
//                           *tbuf, lp1 + 1, lp2,
//                           (LPSTR)pOpenMode[wMode & 7],
//                           (LPSTR)pShareMode[(wMode>>4) & 7],
//                           (LPSTR)(wType==0?"Normal":
//                                  (wType==1?"Mem Map": "UnMovbl")));
//
//                  *(lp1--) = 0;
//                  if(*lp1 == ':') lstrcat(lp1, "\\");  // for root dirs
//
//                  lstrcat(work_buf, tbuf + 2);
               }
               else
               {
                  if(!lp1) lp1 = tbuf;

                  lp2 = _fstrchr(tbuf, '.');
                  if(lp2) *(lp2++) = 0;


                  wsprintf(lpWFI->sz83Name, "  %-8.8s.%-3.3s",
                           *tbuf, lp1 + 1, lp2);

                  lpWFI->wMode = wMode;
                  lpWFI->wType = wType;

//                  wsprintf(work_buf, "  %-8.8s.%-3.3s %-4.4s %-4.4s  %-7.7s ",
//                           lp1 + 1, lp2,
//                           (LPSTR)pOpenMode[wMode & 7],
//                           (LPSTR)pShareMode[(wMode>>4) & 7],
//                           (LPSTR)(wType==0?"Normal":
//                                  (wType==1?"Mem Map": "UnMovbl")));
//
//                  if(lp1 > (LPSTR)tbuf)
//                  {
//                     *(lp1--) = 0;
//                  }
//                  else
//                  {
//                     lp1[0] = 0;
//                  }
//
//                  lstrcat(work_buf, tbuf);
               }

//               PrintString(work_buf);
//               PrintString(pCRLF);

               lpWFI++;

               wNItems++;
            }


         } while(wNItems != 0xffff);
      }


      // sort list, then print it!

      wNItems = ((LPSTR)lpWFI - (LPSTR)lpS) / sizeof(*lpWFI);

      _lqsort((LPSTR)lpS, wNItems, sizeof(*lpWFI), CMDListOpenCompare95);

      lpWFI = (LPWIN95_FILE_INFO)lpS;

      if(hOldCursor) SetCursor(hOldCursor);
      hOldCursor = NULL;


      for(i=0; !AbnormalTermination && i<wNItems; i++)
      {
         if(!ctrl_break &&
            ((i & 5)==0 ||
             (GetTickCount() - dwTickCount) >= 50))  // 20 times per second
         {
            if(LoopDispatch())
            {
               AbnormalTermination = TRUE;
               break;
            }

            dwTickCount = GetTickCount();
         }

         if(redirect_output==HFILE_ERROR)
         {
            while(!ctrl_break && stall_output)
            {
               MthreadSleep(50);
               if(!ctrl_break) ctrl_break = LoopDispatch();
            }
         }

         if(ctrl_break)
         {
            stall_output = FALSE;
            AbnormalTermination = TRUE;

            break;
         }


         wMode = lpWFI[i].wMode;
         wType = lpWFI[i].wType;
         lp2 = lpWFI[i].sz83Name;

         lstrcpy(tbuf, lpWFI[i].szFullPath);

         lp1 = _fstrrchr(tbuf, '\\');

         if(tbuf[0] != '\\' && tbuf[1] == ':')
         {
            if(!lp1) lp1 = (LPSTR)tbuf + 2;

            wsprintf(work_buf, "%-14.14s %-4.4s %-4.4s  %-7.7s ",
                     lp2,
                     (LPSTR)pOpenMode[wMode & 7],
                     (LPSTR)pShareMode[(wMode>>4) & 7],
                     (LPSTR)(wType==0?"Normal":
                            (wType==1?"Mem Map": "UnMovbl")));

            *(lp1--) = 0;
            if(*lp1 == ':') lstrcat(lp1, "\\");  // for root dirs

            lstrcat(work_buf, tbuf + 2);
         }
         else
         {
            if(!lp1) lp1 = tbuf;

            wsprintf(work_buf, "%-14.14s %-4.4s %-4.4s  %-7.7s ",
                     lp2,
                     (LPSTR)pOpenMode[wMode & 7],
                     (LPSTR)pShareMode[(wMode>>4) & 7],
                     (LPSTR)(wType==0?"Normal":
                            (wType==1?"Mem Map": "UnMovbl")));

            if(lp1 > (LPSTR)tbuf)
            {
               *(lp1--) = 0;
            }
            else
            {
               lp1[0] = 0;
            }

            lstrcat(work_buf, tbuf);
         }

         PrintString(work_buf);
         PrintString(pCRLF);
      }
   }


   PrintString(pCRLF);              // one final blank line at the very end

   if(lpParseInfo)
   {
      ReDirectOutput(lpParseInfo,FALSE);
      GlobalFreePtr(lpParseInfo);
   }

   GlobalFreePtr((LPSTR)lpS);        /* free resources allocated earlier */

   return(FALSE);  // no error
}




int FAR _cdecl CMDListOpenCompare(LPSTR lp1, LPSTR lp2)
{
LPSFT_INFO lpS1, lpS2;
int i;

   lpS1 = (LPSFT_INFO)lp1;
   lpS2 = (LPSFT_INFO)lp2;

   if(i = lstrcmp(lpS1->szName, lpS2->szName)) return(i);

   if(lpS1->wPSP==lpS2->wPSP)       return(0);
   else if(lpS1->wPSP > lpS2->wPSP) return(1);
   else                             return(-1);

}



int FAR _cdecl CMDListOpenCompare95(LPSTR lp1, LPSTR lp2)
{
   // Only the 1st 16 characters are significant, and it's a NULL-terminated
   // string, so all I need is a string compare to do this...

   return(lstrcmp(lp1, lp2));
}



                        /** DOS VERSION 2.XX **/
void FAR PASCAL CMDListOpenAddLine0(LPSTR lpData, LPSFT_INFO FAR *lplpSFT,
                                    WORD FAR *lpwCount)
{
struct _sft2_ FAR *lpS;

             /** will this function *ever* get used? **/

   lpS = (struct _sft2_ FAR *)lpData;
   if(!lpS->n_handles) return;         /* entry not in use */

   wsprintf((*lplpSFT)->szName, "%c%c%-8.8s.%-3.3s ",
            lpS->drive?'@'+lpS->drive:' ', lpS->drive?':':' ',
            lpS->name, lpS->name + 8);


   (*lplpSFT)->bMode      = (BYTE)(lpS->open_mode & 7);
   (*lplpSFT)->bShare     = (BYTE)(((lpS->open_mode) >> 4) & 7);
   (*lplpSFT)->wVM        = 0;
   (*lplpSFT)->wPSP       = 0;
   (*lplpSFT)->dwFilePtr  = lpS->file_pos;
   (*lplpSFT)->dwFileSize = lpS->file_size;

   (*lplpSFT)->ExtraLine[0] = 0;

   (*lplpSFT)++;
   (*lpwCount)++;
}

                        /** DOS VERSION 3.0 **/
void FAR PASCAL CMDListOpenAddLine1(LPSTR lpData, LPSFT_INFO FAR *lplpSFT,
                                    WORD FAR *lpwCount)
{
struct _sft30_ FAR *lpS;
WORD drive;

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
      wsprintf((*lplpSFT)->szName, "%c:%-8.8s.%-3.3s ",
                       '@' + drive,
                       lpS->name, lpS->name + 8);
   }
   else
   {
      wsprintf((*lplpSFT)->szName, "  %-8.8s.%-3.3s ",
                       lpS->name, lpS->name + 8);
   }


   (*lplpSFT)->bMode      = (BYTE)(lpS->open_mode & 7);
   (*lplpSFT)->bShare     = (BYTE)(((lpS->open_mode) >> 4) & 7);
   (*lplpSFT)->wVM        = lpS->vm_id;
   (*lplpSFT)->wPSP       = lpS->psp_seg;
   (*lplpSFT)->dwFilePtr  = lpS->file_pos;
   (*lplpSFT)->dwFileSize = lpS->file_size;

   (*lplpSFT)->ExtraLine[0] = 0;

   (*lplpSFT)++;
   (*lpwCount)++;
}

                        /** DOS VERSION 3.X **/
void FAR PASCAL CMDListOpenAddLine2(LPSTR lpData, LPSFT_INFO FAR *lplpSFT,
                                    WORD FAR *lpwCount)
{
struct _sft31_ FAR *lpS;
WORD drive;

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
      wsprintf((*lplpSFT)->szName, "%c:%-8.8s.%-3.3s ",
                       '@' + drive,
                       lpS->name, lpS->name + 8);
   }
   else
   {
      wsprintf((*lplpSFT)->szName, "  %-8.8s.%-3.3s ",
                       lpS->name, lpS->name + 8);
   }


   (*lplpSFT)->bMode      = (BYTE)(lpS->open_mode & 7);
   (*lplpSFT)->bShare     = (BYTE)(((lpS->open_mode) >> 4) & 7);
   (*lplpSFT)->wVM        = lpS->vm_id;
   (*lplpSFT)->wPSP       = lpS->psp_seg;
   (*lplpSFT)->dwFilePtr  = lpS->file_pos;
   (*lplpSFT)->dwFileSize = lpS->file_size;

   (*lplpSFT)->ExtraLine[0] = 0;

   (*lplpSFT)++;
   (*lpwCount)++;
}

                        /** DOS VERSION 4+ **/
void FAR PASCAL CMDListOpenAddLine3(LPSTR lpData, LPSFT_INFO FAR *lplpSFT,
                                    WORD FAR *lpwCount)
{
struct _sft4_ FAR *lpS;
WORD drive;
char tbuf[16], tbuf2[98];  // if needed, temp buffer for device/file name



   lpS = (struct _sft4_ FAR *)lpData;
   if(!lpS->n_handles) return;         /* entry not in use */

   _hmemcpy(tbuf, lpS->name, sizeof(lpS->name));
   tbuf[sizeof(lpS->name)] = 0;

   *tbuf2 = 0;  // initialize to 'empty string'


   if(lpS->dev_info_word & 0x80)       /* bit set if device */
   {
      drive = 0;
   }
   else
   {
      // for now, just print info!

//      wsprintf(tbuf2,
//               "IFS->%04x:%04x REDIR->%04x:%04x  RelCls:%04x DIRSec:%08lx ent#:%d  %s",
//               (WORD)HIWORD(lpS->lpIFS),
//               (WORD)LOWORD(lpS->lpIFS),
//               (WORD)LOWORD(lpS->dir_sector),
//               (WORD)lpS->rel_clus,
//               (WORD)lpS->rel_clus,
//               (DWORD)lpS->dir_sector,
//               (WORD)lpS->dir_entry_num,
//               (LPSTR)((lpS->dev_info_word & 0x8000)?"Remote":"Local"));

      if(lpS->dev_info_word & 0x8000) /* bit set if REMOTE (?) */
      {
         // cannot determine file name here if 'blank'

      }
      else
      {
         // get the 'full path' for the file in 'tbuf2'

         if(!*tbuf || !_fmemcmp(tbuf, "           ", 11))
         {
            // name is "all blanks" - determine what the actual file name
            // is and generate an 8.3 name for it in 'tbuf'

         }
      }

      drive = (lpS->dev_info_word & 31) + 1;
   }

   if(drive)
   {
      wsprintf((*lplpSFT)->szName, "%c:%-8.8s.%-3.3s ",
                                   '@' + drive,
                                   (LPSTR)tbuf, (LPSTR)tbuf + 8);
   }
   else
   {
      wsprintf((*lplpSFT)->szName, "  %-8.8s.%-3.3s ",
                                   (LPSTR)tbuf, (LPSTR)tbuf + 8);
   }


   (*lplpSFT)->bMode      = (BYTE)(lpS->open_mode & 7);
   (*lplpSFT)->bShare     = (BYTE)(((lpS->open_mode) >> 4) & 7);
   (*lplpSFT)->wVM        = lpS->vm_id;
   (*lplpSFT)->wPSP       = lpS->psp_seg;
   (*lplpSFT)->dwFilePtr  = lpS->file_pos;
   (*lplpSFT)->dwFileSize = lpS->file_size;

   lstrcpy((*lplpSFT)->ExtraLine, tbuf2);

   (*lplpSFT)++;
   (*lpwCount)++;
}


#pragma pack()      /* revert structure packing to that used originally */

/***************************************************************************/





#pragma code_seg("CMDLoadhigh_TEXT","CODE")


BOOL FAR PASCAL CMDLoadhigh(LPSTR lpCmd, LPSTR lpArgs)
{
LPSTR lp1, lp2, lp3;
BOOL rval;


   lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   for(lp1=lpArgs; *lp1 && *lp1<=' '; lp1++)
      ;  /* find first non-white-space */

   if(*lp1)
   {
      lp3 = NULL;  // this avoids problems later on...

      if(*lp1 == '\"')
      {
         lp2 = ++lp1;

         while(*lp2 && *lp2 != '\"') lp2++;

         if(*lp2) *(lp2++) = 0;
      }
      else
      {
         for(lp2=lp1+1; *lp2 && *lp2>' ' && *lp2!='/'; lp2++)
            ;                          /* find NEXT white-space or '/' */
      }

      if(*lp2)
      {
         if(*lp2=='/') /* I was afraid of that... */
         {
            lp3 = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE, lstrlen(lp2) + 2);
            if(!lp3)
            {
               PrintString(pNOTENOUGHMEMORY);
               GlobalFreePtr(lpArgs);
               return(TRUE);
            }

            lstrcpy(lp3, lp2);
         }
         else
         {
            lp3 = NULL;
         }

         *lp2++ = 0;  /* terminate 'program name' portion */

         if(lp3) lp2 = lp3;
      }

      rval = CMDRunProgram(lp1, lp2, SW_SHOWMINNOACTIVE);

      if(lp3) GlobalFreePtr(lp3);

      GlobalFreePtr(lpArgs);
      return(rval);
   }
   else
   {
      PrintErrorMessage(621);

      GlobalFreePtr(lpArgs);
      return(FALSE);
   }
}


#pragma code_seg("CMDMkdir_TEXT","CODE")


BOOL FAR PASCAL CMDMkdir   (LPSTR lpCmd, LPSTR lpArgs)
{
BOOL rval;


   lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   if(*lpArgs==0)
   {

      rval = CMDBad(lpCmd, lpArgs);

   }
   else
   {
      lstrcpy(work_buf, lpArgs);

      if(*work_buf == '\"')  // quoted path?
      {
       LPSTR lp1 = work_buf;

         lstrcpy(lp1, lp1 + 1);
         while(*lp1 && *lp1 != '\"') lp1++;

         if(*lp1=='\"')
         {
            if(*lp1) *(lp1++) = 0;

            while(*lp1 && *lp1 <= ' ') lp1++;

            if(*lp1) // non-white-space is a syntax error...
            {
               PrintErrorMessage(868);  // "?Invalid 'long' file name specified\r\n"

               GlobalFreePtr(lpArgs);
               return(TRUE);
            }
         }
      }

      rval = CMDErrorCheck(My_Mkdir(work_buf));
   }

   GlobalFreePtr(lpArgs);
   return(rval);

}

//#pragma alloc_text (CMDMax_TEXT,CMDMax)
#pragma code_seg("CMDMax_TEXT","CODE")


BOOL FAR PASCAL CMDMax     (LPSTR lpCmd, LPSTR lpArgs)
{

   if(*lpArgs==0)  /* no arguments today */
   {
      if(IsZoomed(hMainWnd))
      {
         PostMessage(hMainWnd, WM_SYSCOMMAND, (WPARAM)SC_RESTORE, 0);
      }
      else
      {
         PostMessage(hMainWnd, WM_SYSCOMMAND, (WPARAM)SC_MAXIMIZE, 0);
      }
      return(FALSE);
   }
   else
   {
    HWND hWnd;

      lpArgs = EvalString(lpArgs);

      if(!lpArgs)
      {
         PrintString(pNOTENOUGHMEMORY);

         return(TRUE);
      }

      hWnd = (HWND)MyValue(lpArgs);

      if(IsWindow(hWnd))
      {
         if(IsZoomed(hWnd))
         {
            ShowWindow(hWnd, SW_SHOWNORMAL);
         }
         else
         {
            ShowWindow(hWnd, SW_SHOWMAXIMIZED);
         }
      }
      else
      {
       static const char CODE_BASED szMsg[]="?Invalid window handle\r\n";

         PrintString(szMsg);
      }

      GlobalFreePtr(lpArgs);
   }

   return(FALSE);
}


#pragma code_seg("CMDMem_TEXT","CODE")


BOOL FAR PASCAL CMDMem     (LPSTR lpCmd, LPSTR lpArgs)
{
DWORD dwLargest, dwFree, total_code, total_data, total_other, total_disc,
      dwFreeVirt;
WORD nTasks, n1, n2, n3, n4;
LPSTR lpBuf1, lpBuf2;
volatile LPSTR lpSwitch;
HCURSOR hOldCursor;
LPPARSE_INFO lpParseInfo;
static MODULEENTRY me;
static TASKENTRY te;
static GLOBALENTRY ge;
static MEMMANINFO mmi;
//static char nomemerr[]="?Not enough memory to complete operation!\r\n\n";

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





   hOldCursor = MySetCursor(LoadCursor(NULL, IDC_WAIT));  /* hourglass! */


              /** Initialization of TOOLHELP structures **/

   memset(&mmi, 0, sizeof(mmi));
   memset(&ge, 0, sizeof(ge));
   memset(&te, 0, sizeof(te));
   memset(&me, 0, sizeof(me));

   mmi.dwSize = sizeof(mmi);
   ge.dwSize  = sizeof(ge);
   te.dwSize  = sizeof(te);
   me.dwSize  = sizeof(me);



     /** SET THINGS UP, GET LIST OF MEMORY CONTENTS, AND MOVE FORWARD **/

   if(!(lpParseInfo = CMDLineParse(lpArgs)))
   {
      PrintString(pNOTENOUGHMEMORY);

      MySetCursor(hOldCursor);
      return(TRUE);
   }

   if(CMDCheckParseInfo(lpParseInfo, "cCdDpP", CMD_ATMOST | 0) ||
      lpParseInfo->n_switches > 1)
   {
      PrintErrorMessage(623);

      GlobalFreePtr(lpParseInfo);

      MySetCursor(hOldCursor);
      return(TRUE);
   }

   if(lpParseInfo->n_switches && !hToolHelp)
   {
      PrintErrorMessage(624);
      PrintString(lpArgs);
      PrintErrorMessage(625);

      GlobalFreePtr(lpParseInfo);

      MySetCursor(hOldCursor);
      return(FALSE);
   }

   ReDirectOutput(lpParseInfo,TRUE);



   lpSwitch = (LPSTR)(lpParseInfo->sbpItemArray[0]);

   total_code = total_data = total_other = total_disc = 0L;


   if(!(lpBuf1 = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                       sizeof(struct CMDMEM_ENTRY) * 128)))
   {
      PrintString(pNOTENOUGHMEMORY);

      ReDirectOutput(lpParseInfo,FALSE);
      GlobalFreePtr(lpParseInfo);

      MySetCursor(hOldCursor);
      return(TRUE);
   }

   n2 = (WORD)(GlobalSizePtr(lpBuf1) / sizeof(struct CMDMEM_ENTRY));
   lpME1 = (struct CMDMEM_ENTRY _huge *)lpBuf1;
   lpME2 = lpME1;                            /* next available entry... */


   n1 = 0;

   if(!lpGlobalFirst(&ge, GLOBAL_ALL))
   {
      PrintErrorMessage(626);

      GlobalFreePtr(lpBuf1);
      ReDirectOutput(lpParseInfo,FALSE);
      GlobalFreePtr(lpParseInfo);

      MySetCursor(hOldCursor);
      return(TRUE);
   }

   do
   {       /* bug in TOOLHELP - free blocks have NULL hBlock */
           /*                   and NULL hOwner...           */

      if(ge.wFlags==GT_FREE || (!ge.hBlock && !ge.hOwner))  continue;

      if(n1>=n2)  /* time to re-allocate memory!! */
      {
         lpBuf2 = (LPSTR)GlobalReAllocPtr(lpBuf1,((n1 + 64L) / 64L) * 64L
                                           * sizeof(struct CMDMEM_ENTRY),
                                          GMEM_MOVEABLE | GMEM_ZEROINIT);
         if(!lpBuf2)
         {
            GlobalFreePtr(lpBuf1);
            lpBuf1 = (LPSTR)NULL;

            PrintString(pNOTENOUGHMEMORY);
            GlobalFreePtr(lpBuf1);
            ReDirectOutput(lpParseInfo,FALSE);
            GlobalFreePtr(lpParseInfo);

            MySetCursor(hOldCursor);
            return(TRUE);
         }
         else
         {
            lpBuf1 = lpBuf2;
            lpME1 = (struct CMDMEM_ENTRY _huge *)lpBuf1;
            lpME2 = n1 + lpME1;
            n2 = (WORD)(GlobalSizePtr(lpBuf1) /
                         sizeof(struct CMDMEM_ENTRY));

            // OK - take a bit of a chance here, and start at the
            // beginning all over again to ensure my numbers add up!


            n1 = 0;       // start over!!

            if(!lpGlobalFirst(&ge, GLOBAL_ALL))
            {
               PrintErrorMessage(626);

               GlobalFreePtr(lpBuf1);
               ReDirectOutput(lpParseInfo,FALSE);
               GlobalFreePtr(lpParseInfo);

               MySetCursor(hOldCursor);
               return(TRUE);
            }
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

         lpME3->hOwner = (HTASK)ge.hOwner;
         lpME3->total_code  = 0L;
         lpME3->total_data  = 0L;
         lpME3->total_other = 0L;
         lpME3->total_disc  = 0L;

      }

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

//      /***** THIS WAS THE 'OLD' WAY OF DOING THINGS *****/
//      
//      if(ge.dwNextAlt)       // block is discardable if not NULL!
//                             // (already know block isn't GT_FREE)

      if(ge.hBlock && (GlobalFlags(ge.hBlock) & GMEM_DISCARDABLE))
      {

         lpME3->total_disc += ge.dwBlockSize;
      }


      if(n1 && ge.wFlags==GT_SENTINEL) break;  // last entry!


   } while(lpGlobalNext(&ge, GLOBAL_ALL));



   lpBuf2 = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                  (n1 + 1)* sizeof(struct CMDMEM_MODULE));
   if(!lpBuf2)
   {
      PrintString(pNOTENOUGHMEMORY);

      GlobalFreePtr(lpBuf1);
      ReDirectOutput(lpParseInfo,FALSE);
      GlobalFreePtr(lpParseInfo);

      MySetCursor(hOldCursor);
      return(TRUE);
   }

         /* I've got all of the task handles!! Now, get the modules */

   lpMM1 = (struct CMDMEM_MODULE _huge *)lpBuf2;
   lpMM1->hModule = 0;
   _fstrcpy(lpMM1->module, "* N/A *");

   for(n2=0, n4=0, lpME2=lpME1, lpMM2=lpMM1 + 1; n2<n1; n2++, lpME2++)
   {
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


   if(*lpSwitch=='C' || *lpSwitch=='c')                       /* 'MEM /C' */
   {

                       /* NOW, sort the thing!! */

      _lqsort((LPSTR)(lpMM1 + 1), n4, sizeof(struct CMDMEM_MODULE),
               CMDMemSortProc);


                  /*** Last but not least - output! ***/

      PrintString(pCRLF);
      PrintErrorMessage(765); // "MODULE NAME  HANDLE    Code     Data &    Other     Total    Discard-\r\n"
      PrintErrorMessage(766); // "(Prog, Lib)  (Hex)   Segments  Resource   Memory    in Use     able  \r\n"
      PrintErrorMessage(767); // "-----------  ------  --------  --------  --------  --------  --------\r\n"

   }



   for(n3=0, lpMM3=lpMM1; n3<=n4; n3++, lpMM3++)
   {
      if(*lpSwitch=='C' || *lpSwitch=='c')                    /* 'MEM /C' */
      {

         if(!n3)             /* very first entry - 'unknown', essentially */
         {
          static const char CODE_BASED pFmt[]=
                  "* SYSTEM *     N/A   %8lu  %8lu  %8lu  %8lu  %8lu\r\n";

            LockCode();
            wsprintf(work_buf, pFmt,
                  (DWORD)(lpMM3->total_code),
                  (DWORD)(lpMM3->total_data),
                  (DWORD)(lpMM3->total_other),
                  (DWORD)(lpMM3->total_code + lpMM3->total_data +
                          lpMM3->total_other),
                  (DWORD)(lpMM3->total_disc)   );
            UnlockCode();
         }
         else
         {
          static const char CODE_BASED pFmt[]=
                  "%-8.8s      %04xH  %8lu  %8lu  %8lu  %8lu  %8lu\r\n";

            LockCode();
            wsprintf(work_buf, pFmt,
                  (LPSTR)(lpMM3->module), (WORD)lpMM3->hModule,
                  (DWORD)(lpMM3->total_code),
                  (DWORD)(lpMM3->total_data),
                  (DWORD)(lpMM3->total_other),
                  (DWORD)(lpMM3->total_code + lpMM3->total_data +
                          lpMM3->total_other),
                  (DWORD)(lpMM3->total_disc)   );
            UnlockCode();
         }

         PrintString(work_buf);

      }

      total_code  += lpMM3->total_code;
      total_data  += lpMM3->total_data;
      total_other += lpMM3->total_other;
      total_disc  += lpMM3->total_disc;
   }


   if(*lpSwitch=='C' || *lpSwitch=='c')                       /* 'MEM /C' */
   {
    static const char CODE_BASED pFmt[]=
          " ** GRAND TOTAL **   %8lu  %8lu  %8lu  %8lu  %8lu\r\n\n";

      PrintErrorMessage(857); // "===================  ========  ========  ========  ========  ========\r\n"

      LockCode();
      wsprintf(work_buf, pFmt,
         (DWORD)total_code, (DWORD)total_data, (DWORD)total_other,
         (DWORD)(total_code + total_data + total_other),(DWORD)(total_disc));
      UnlockCode();

      PrintString(work_buf);

   }
   else if(*lpSwitch>' ')                /* Un-Supported Switch was used! */
   {
      PrintErrorMessage(627);
      PrintAChar(*lpSwitch);
      PrintErrorMessage(628);

   }


   // OK!  Before I get "total memory" info, ensure I free 'lpBuf2' first

   GlobalFreePtr(lpBuf2);


   // Now, I can get the TRUE memory in use by SFTSHELL reflected by the
   // 'mmi' structure so that my numbers add up properly.  There.

   if(!lpMemManInfo(&mmi))
   {
      PrintErrorMessage(622);

      GlobalFreePtr(lpBuf1);
      ReDirectOutput(lpParseInfo,FALSE);
      GlobalFreePtr(lpParseInfo);

      MySetCursor(hOldCursor);
      return(TRUE);
   }

   // In addition to the above, get more info from Windows concerning
   // free space, etc.

   dwFree    = GetFreeSpace(0);
   dwLargest = GlobalCompact(0);
   nTasks    = GetNumTasks();


   GlobalFreePtr(lpBuf1);
   MySetCursor(hOldCursor);            // restore cursor (from above)



                    /* Now, print the final summary */

   if(GetWinFlags() & WF_ENHANCED)
   {
    static const char CODE_BASED pTitle[]=
               "             ** ENHANCED MODE WINDOWS **\r\n"
 /* ASC 205 */ "อออออออออออออออออออออออออออออออออออออออออออออออออออออ\r\n";

    static const char CODE_BASED pFmt1[]=
               "Total Available Memory:             %9lu bytes\r\n"
               "Largest Contiguous Block:           %9lu bytes\r\n"
               "Estimated Unused Linear Memory:     %9lu bytes\r\n"
               "Total Linear Memory:                %9lu bytes\r\n"
               "Estimated Unused Virtual Memory:    %9lu bytes\r\n"
               "Total Virtual (Swap File) Memory:   %9lu bytes\r\n\r\n";

    static const char CODE_BASED pFmt2[]=
               "Total Non-Discardable Memory In Use:%9lu bytes\r\n"
               "Windows Applications (non-discard): %9lu bytes\r\n"
               "Windows Applications (discardable): %9lu bytes\r\n"
               "Non-Windows/System/Miscellaneous:   %9lu bytes\r\n\r\n"
               "Total number of tasks:  %u\r\n\r\n";


      dwFreeVirt = dwFree - total_disc - mmi.dwFreePages * mmi.wPageSize;

      // IF 'dwFreeVirt' is larger than the virtual file, correct the
      // value to equal the size of the virtual file.  Usually it's close!

      if(dwFreeVirt > mmi.dwSwapFilePages * mmi.wPageSize)
      {
         dwFreeVirt = mmi.dwSwapFilePages * mmi.wPageSize;
      }


      LockCode();

      PrintString(pTitle);

      wsprintf(work_buf, pFmt1,
               dwFree,
               dwLargest,
               dwFree - total_disc - dwFreeVirt,
               mmi.dwTotalPages * mmi.wPageSize,
               dwFreeVirt,
               mmi.dwSwapFilePages * mmi.wPageSize);

      PrintString(work_buf);

      wsprintf(work_buf, pFmt2,
               mmi.dwTotalPages * mmi.wPageSize +
                  mmi.dwSwapFilePages * mmi.wPageSize - dwFree,
               total_code + total_data + total_other - total_disc,
               total_disc,
               mmi.dwTotalPages * mmi.wPageSize +
                  mmi.dwSwapFilePages * mmi.wPageSize - dwFree
                   - total_code - total_data - total_other + total_disc,
               nTasks );

      UnlockCode();
   }
   else
   {
    static const char CODE_BASED pFmt[]=
               "             ** STANDARD MODE WINDOWS **\r\n"
 /* ASC 205 */ "อออออออออออออออออออออออออออออออออออออออออออออออออออออ\r\n"
               "Total Available Memory:             %9lu bytes\r\n"
               "Largest Contiguous Block:           %9lu bytes\r\n\r\n"
               "Total Memory In Use  (non-discard): %9lu bytes\r\n"
               "                     (discardable): %9lu bytes\r\n\r\n"
               "Total number of tasks:  %u\r\n\r\n";

      LockCode();

      wsprintf(work_buf, pFmt,
               dwFree,
               dwLargest,
               total_code + total_data + total_other - total_disc,
               total_disc,
               nTasks );

      UnlockCode();
   }

   PrintString(work_buf);


   if(lpParseInfo)
   {
      ReDirectOutput(lpParseInfo,FALSE);
      GlobalFreePtr(lpParseInfo);
   }

   MySetCursor(hOldCursor);
   return(FALSE);

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




#pragma code_seg("CMDMin_TEXT","CODE")


BOOL FAR PASCAL CMDMin     (LPSTR lpCmd, LPSTR lpArgs)
{

   if(*lpArgs==0)  /* no arguments today */
   {
      if(IsIconic(hMainWnd))
      {
         PostMessage(hMainWnd, WM_SYSCOMMAND, (WPARAM)SC_RESTORE, 0);
      }
      else
      {
         PostMessage(hMainWnd, WM_SYSCOMMAND, (WPARAM)SC_MINIMIZE, 0);
      }
      return(FALSE);
   }
   else
   {
    HWND hWnd;

      lpArgs = EvalString(lpArgs);

      if(!lpArgs)
      {
         PrintString(pNOTENOUGHMEMORY);

         return(TRUE);
      }

      hWnd = (HWND)MyValue(lpArgs);

      if(IsWindow(hWnd))
      {
         if(IsIconic(hWnd))
         {
            ShowWindow(hWnd, SW_SHOWNORMAL);
         }
         else
         {
            ShowWindow(hWnd, SW_SHOWMINNOACTIVE);
         }
      }
      else
      {
       static const char CODE_BASED szMsg[]="?Invalid window handle\r\n";

         PrintString(szMsg);
      }

      GlobalFreePtr(lpArgs);
   }

   return(FALSE);
}


#pragma code_seg("CMDMode_TEXT","CODE")


BOOL FAR PASCAL CMDMode    (LPSTR lpCmd, LPSTR lpArgs)
{
BOOL rval;


   lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   rval = CMDRunProgram(lpCmd, lpArgs, SW_SHOWNORMAL);
   GlobalFreePtr(lpArgs);

   return(rval);
}


#pragma code_seg("CMDMore_TEXT","CODE")

BOOL NEAR PASCAL CMDMoreCheckBreakAndStall(DWORD FAR *lpdwTickCount);
void NEAR PASCAL CMDMoreProcessPause(int FAR *lpPauseCtr);


BOOL FAR PASCAL CMDMore    (LPSTR lpCmd, LPSTR lpArgs)
{
LPPARSE_INFO lpParseInfo;
int iPauseCtr;
DWORD dwTickCount;
char lpBuf[2048];


   lpParseInfo = CMDLineParse(lpArgs);

   if(CMDCheckParseInfo(lpParseInfo, "", 1 | CMD_ATMOST))
   {
      GlobalFreePtr(lpParseInfo);
      return(TRUE);
   }


   // if I get ambitious,perhaps I'll write a 'ReDirectInput()' for this


   if(lpParseInfo->sbpInFile)  // re-directing input!!
   {
      if(lpParseInfo->n_parameters)
      {
         PrintString("?Warning - file name in 'MORE' command line ignored\r\n\n");
      }


      redirect_input = _lopen(lpParseInfo->sbpInFile,
                              READ | OF_SHARE_DENY_WRITE);

      if(redirect_input == HFILE_ERROR)
      {
         GlobalFreePtr(lpParseInfo);
         return(CMDErrorCheck(TRUE));
      }
      else if(IsCONDevice(redirect_input))  // CON device??
      {
         _lclose(redirect_input);

         redirect_input = HFILE_ERROR;  // no redirection
      }
   }
   else if(lpParseInfo->n_parameters)   // only 1 parm, EVER!
   {
      redirect_input = _lopen(lpParseInfo->sbpItemArray[0],
                              READ | OF_SHARE_DENY_WRITE);

      if(redirect_input == HFILE_ERROR)
      {
         GlobalFreePtr(lpParseInfo);
         return(CMDErrorCheck(TRUE));
      }
      else if(IsCONDevice(redirect_input))  // CON device??
      {
         _lclose(redirect_input);

         redirect_input = HFILE_ERROR;  // no redirection
      }
   }



   // in THIS case, re-directed OUTPUT is invalid, so don't bother with it

   if(lpParseInfo->sbpOutFile || lpParseInfo->sbpPipeNext)
   {
      PrintString("?Warning - piping and output redirection invalid for 'MORE'\r\n");
   }



   // FILE I/O IS PERFORMED HERE!!!

   iPauseCtr = 0;
   dwTickCount = GetTickCount();

   while(GetUserInput(lpBuf))
   {
      if(ctrl_break ||
         (redirect_input != HFILE_ERROR
          && CMDMoreCheckBreakAndStall(&dwTickCount)))
      {
         break;
      }

      // check # of screen lines we've printed so far

      if(iPauseCtr)
      {
         if(*lpBuf=='\n')
         {
            iPauseCtr ++;  // must add 1 line, always (bug fix)
         }
         else
         {
            iPauseCtr += (lstrlen(lpBuf) + SCREEN_COLS - 3) / SCREEN_COLS;
         }

         CMDMoreProcessPause(&iPauseCtr);

         if(ctrl_break ||
            (redirect_input != HFILE_ERROR
             && CMDMoreCheckBreakAndStall(&dwTickCount)))
         {
            break;
         }
      }
      else
      {
         if(*lpBuf=='\n')
         {
            iPauseCtr ++;  // must add 1 line, always (bug fix)
         }
         else
         {
            iPauseCtr += (lstrlen(lpBuf) + SCREEN_COLS - 3) / SCREEN_COLS;
         }
      }

      PrintString(lpBuf);
   }


   if(redirect_input != HFILE_ERROR)
   {
      _lclose(redirect_input);
      redirect_input = HFILE_ERROR;
   }

   iPauseCtr +=2;                    // add one extra line, plus 1 for prompt
   CMDMoreProcessPause(&iPauseCtr);
   PrintString(pCRLF);               // print an extra <CR><LF> at the end...

   GlobalFreePtr(lpParseInfo);
   return(FALSE);
}

void NEAR PASCAL CMDMoreProcessPause(int FAR *lpPauseCtr)
{
int oldcurline = curline;  // save current line
WORD actual_text_height, lines_per_screen;
RECT rct;



   if(ctrl_break) return;

   if(!TMFlag)
     UpdateTextMetrics(NULL);
        
   actual_text_height = tmMain.tmHeight + tmMain.tmExternalLeading;

   GetClientRect(hMainWnd, &rct);
   lines_per_screen = (rct.bottom - rct.top - 1) / actual_text_height;


   if(*lpPauseCtr > (lines_per_screen - 1))
   {
      // this code stolen from 'CMDPause()' (below)

      PrintErrorMessage(796);  // "Press <ENTER> to continue:  "

      pausing_flag = TRUE;      /* assign global flag TRUE to cause pause */

      while(pausing_flag)
      {
         if(LoopDispatch())    /* returns TRUE on WM_QUIT */
         {
            PrintErrorMessage(635);

            ctrl_break = TRUE; // fakes things out for abnormal termination
            return;
         }
      }

      curline = oldcurline;
      *lpPauseCtr = *lpPauseCtr % (lines_per_screen - 1);
            // this ensures that a one-time output of more than the
            // # of lines on the screen won't screw it up beyond
            // all possibility of sanity or recognition.


      PrintErrorMessage(751);  // "\r\033[K"
                        // deletes to end of line from left column
   }
}


BOOL NEAR PASCAL CMDMoreCheckBreakAndStall(DWORD FAR *lpdwTickCount)
{
   if(ctrl_break) return(TRUE);

   while(stall_output && !ctrl_break)
   {
      MthreadSleep(100);  // sleep .1 sec until output no longer stalled!

      if(!ctrl_break) ctrl_break = LoopDispatch();

      *lpdwTickCount = GetTickCount();
   }

   if(ctrl_break) stall_output = FALSE;

   if(!ctrl_break && (GetTickCount() - *lpdwTickCount) >= 50)
   {
                               // force a yield at least 20 times per second
      if(LoopDispatch())
      {
         ctrl_break = TRUE;    // abnormal terminate
      }
      else
      {
         *lpdwTickCount = GetTickCount();
      }
   }

   return(ctrl_break);
}







#pragma code_seg("CMDMove_TEXT","CODE")


BOOL FAR PASCAL CMDMove    (LPSTR lpCmd, LPSTR lpArgs)
{
LPPARSE_INFO lpParseInfo=(LPPARSE_INFO)NULL;
NPSTR np1, np2;
LPSTR lp1, lp2, lp3;
WORD file_cnt, c;
LPDIR_INFO lpDirInfo=(LPDIR_INFO)NULL, lpDI;
char path_buf[512];
BOOL rval;



   /* this is not the same as RENAME, so don't allow DIR's to be specified */


   lpParseInfo = CMDLineParse(lpArgs);

   if(CMDCheckParseInfo(lpParseInfo, "S", 2 | CMD_ATMOST)
      || CMDCheckParseInfo(lpParseInfo, "S", 1 | CMD_ATLEAST))
   {                            /* 1 switch, 1 to 2 commands */
      GlobalFreePtr(lpParseInfo);
      return(TRUE);
   }


//
//   moved the check for 'drive in dest path' to section below to correct bugs
//
//   if(lpParseInfo->n_parameters==2 &&
//      (GetPathNameInfo(lpParseInfo->sbpItemArray[lpParseInfo->n_switches + 1])
//       & (PATH_HAS_DRIVE)))
//   {
//      GlobalFreePtr((LPSTR)lpParseInfo);
//      PrintString("?New name may not have drive specified\r\n\n");
//      return(FALSE);
//   }


   np1 = work_buf + (sizeof(work_buf) / 2);
   lp1 = (LPSTR)path_buf + (sizeof(path_buf) / 2);

                /* source path */

   DosQualifyPath(work_buf,          /* make "FULL" DOS path! */
                  lpParseInfo->sbpItemArray[lpParseInfo->n_switches]);

   if(!*work_buf)
   {
      PrintErrorMessage(629);
      GlobalFreePtr(lpParseInfo);
      return(TRUE);
   }


                        /* add '*.*' if required */
   QualifyPath(work_buf, work_buf, TRUE);


              /* destination path */

   if(lpParseInfo->n_parameters==1)
   {
      getcwd(np1, sizeof(work_buf)/2);
   }
   else
   {
      lstrcpy(np1,lpParseInfo->sbpItemArray[lpParseInfo->n_switches + 1]);
   }

   // only allow same drive on both source & dest if drive was specified
   // on destination path - otherwise, ensure the dest drive == source drive

   if(/* lpParseInfo->n_parameters==2 && */
      (GetPathNameInfo(lpParseInfo->sbpItemArray[lpParseInfo->n_switches + 1])
       & (PATH_HAS_DRIVE)))
   {
      // drive specified in dest - ensure it is the same as the source

      if(*work_buf=='\\' && work_buf[1]=='\\')  // source is a NETWORK drive!
      {
         DosQualifyPath(np1, np1);  // generate canonical name

         if(!*np1)
         {
            PrintErrorMessage(630);
            GlobalFreePtr(lpParseInfo);
            return(TRUE);
         }

         if(*np1=='\\' && np1[1]=='\\')         // dest is also a NETWORK drive
         {
            // verify that both have the same 'network machine' name
            // assume it's ok if they both are on the same server, for now...

            lp2 = _fstrchr(work_buf + 2, '\\');
            lp3 = _fstrchr(np1 + 2, '\\');

            if((lp2 - (LPSTR)work_buf)!=(lp3 - (LPSTR)np1) ||
               _fstrnicmp(work_buf + 2, np1 + 2, (lp3 - (LPSTR)np1)))
            {
               GlobalFreePtr((LPSTR)lpParseInfo);
               PrintErrorMessage(631);
               return(FALSE);
            }
         }
      }
      else if(work_buf[1]==':' && np1[1]==':')
      {
         DosQualifyPath(np1, np1);  // generate canonical name

         if(!*np1)
         {
            PrintErrorMessage(630);
            GlobalFreePtr(lpParseInfo);
            return(TRUE);
         }

         if(toupper(*np1)!=toupper(*work_buf))
         {
            GlobalFreePtr((LPSTR)lpParseInfo);
            PrintErrorMessage(632);
            return(FALSE);
         }
      }
      else
      {
         GlobalFreePtr((LPSTR)lpParseInfo);
         PrintErrorMessage(633);
         return(FALSE);
      }
   }
   else              // no drive specified in dest - make it same as source!
   {
      if(*work_buf=='\\')  // drive specified by source is a network drive
      {
         // use the drive specifier from the source path, if any

         if(GetPathNameInfo(lpParseInfo->sbpItemArray[lpParseInfo->n_switches])
             & (PATH_HAS_DRIVE))
         {
            *lp1 = *(lpParseInfo->sbpItemArray[lpParseInfo->n_switches]);
         }
         else
         {
            *lp1 = _getdrive() + 'A' - 1;
         }

      }
      else
      {
         *lp1 = *work_buf;
      }

      lp1[1] = ':';

      lstrcpy(lp1 + 2, np1);
      DosQualifyPath(np1, lp1);  // generate canonical name

      if(!*np1)
      {
         PrintErrorMessage(630);
         GlobalFreePtr(lpParseInfo);
         return(TRUE);
      }
   }


   QualifyPath(lp1, np1, TRUE);         /* takes care of dest path '*.*' */
                                        /* dest name pattern now in 'lp1' */
//
//   lp2 = _fstrchr(work_buf, ':');  // find the ':'
//   lp3 = _fstrchr(lp1, ':');       // and find the ':' here too
//
//   if(lp2 && lp3 && (lp2 - (LPSTR)work_buf)!=(lp3 - lp1))
//   {
//       // make 'lp1' point to 'same drive':'original path' as 'work_buf'
//
//      _fmemmove(lp1 + (lp2 - (LPSTR)work_buf), lp3, lstrlen(lp3) + 1);
//      _fmemcpy(lp1, work_buf, (lp2 - (LPSTR)work_buf));
//   }
//

   file_cnt = GetDirList0(work_buf, ~_A_VOLID & ~_A_SUBDIR & 0xff,
                          (LPDIR_INFO FAR *)&lpDirInfo,
                          (LPSTR)path_buf, FALSE, NULL);

   if(file_cnt==0 || file_cnt==DIRINFO_ERROR)
   {
      if(lpParseInfo) GlobalFreePtr((LPSTR)lpParseInfo);
      if(lpDirInfo)   GlobalFreePtr(lpDirInfo);

      if(file_cnt==0)
      {
         PrintErrorMessage(548);
         return(FALSE);
      }
      else
      {
         return(CMDErrorCheck(TRUE));
      }
   }

   for(rval = FALSE, c=0, lpDI = lpDirInfo; c<file_cnt; c++, lpDI++)
   {
      memset(work_buf, 0, sizeof(work_buf));  /* clear entire buffer */

            /* use pattern in 'path_buf' to make new name */

      if(lpDI->fullname[0] == '.' &&
         (!lpDI->fullname[1] ||
          (lpDI->fullname[1]=='.' && !lpDI->fullname[2])))
      {
         continue;                /* for '.' and '..' just go on... */
      }

      lstrcpy(work_buf, (LPSTR)path_buf);
      if(work_buf[strlen(work_buf) - 1]!='\\')
         strcat(work_buf, pBACKSLASH);

      np2 = work_buf + strlen(work_buf);  /* points to where name will be */
      _fstrncpy(np2, lpDI->fullname, sizeof(lpDI->fullname));

      if(IsChicago || IsNT)
      {
         ShortToLongName(work_buf, NULL);      // convert to LONG file name
         np2 = strrchr(work_buf, '\\') + 1;  // it *BETTER* work!!
      }

      if(UpdateFileName(np1, lp1, np2))
      {
         PrintString(work_buf);
         PrintErrorMessage(634);
         rval = TRUE;
         break;
      }

            /* 'np1' now points to updated 'pattern' for new name */
            /* 'np2' points to the "name only" of the source file */
            /* 'lp1' is the pattern to use for 'renaming'.        */

      if(!IsChicago && !IsNT)              // only if LFN's not supported
      {
         _strupr(work_buf);
         _strupr(np1);            /* just to make the output look nice, ok? */
      }

      PrintString(work_buf);
      if(lstrlen(work_buf) > 30) PrintString(pCRLF);

      PrintErrorMessage(795);     // " --> "
      PrintString(np1);
      if(lstrlen(np1) > 60) PrintString(pCRLF);

      if(MyRename(work_buf, np1)) PrintErrorMessage(537); // "  ** ERROR **\r\n"
      else                        PrintErrorMessage(527); // "  ** OK **\r\n"
   }

   GlobalFreePtr((LPSTR)lpDirInfo);
   GlobalFreePtr((LPSTR)lpParseInfo);

   return(CMDErrorCheck(rval));

}


#pragma code_seg("CMDNet_TEXT","CODE")


static BOOL NEAR PASCAL CMDNetEnum(LPNETRESOURCE lpRSC, LPPARSE_INFO lpParseInfo,
                                   BOOL bRecurseFlag)
{
DWORD dwWNetOpenEnum, dwWNetEnumResource, dwWNetCloseEnum,
      dwWNetGetLastError;
DWORD dwRval, dwEnum, dwAddr, dw1, dw2;
LPNETRESOURCE lpBuf = NULL;
WORD wMySel = 0;
LPSTR lp1;


   // load proc addresses for 'WNetOpenEnum', 'WNetEnumResource', etc.

   dwWNetOpenEnum     = lpGetProcAddress32W(hMPR, "WNetOpenEnumA");
   dwWNetEnumResource = lpGetProcAddress32W(hMPR, "WNetEnumResourceA");
   dwWNetCloseEnum    = lpGetProcAddress32W(hMPR, "WNetCloseEnum");

   dwWNetGetLastError = lpGetProcAddress32W(hMPR, "WNetGetLastErrorA");

   if(!dwWNetOpenEnum || !dwWNetEnumResource ||
      !dwWNetCloseEnum || !dwWNetGetLastError)
   {
      if(bRecurseFlag)
         ReDirectOutput(lpParseInfo, FALSE);

      PrintErrorMessage(927);  // NET VIEW - Function not available

      return(TRUE);  // error
   }

   // allocate buffer for enumeration

   lpBuf = (LPNETRESOURCE)GlobalAllocPtr(GMEM_FIXED | GMEM_ZEROINIT, 0x10000L);
   if(!lpBuf)
   {
      if(bRecurseFlag)
         ReDirectOutput(lpParseInfo, FALSE);

      PrintString(pNOTENOUGHMEMORY);

      return(TRUE);
   }



   if(lpRSC)  // viewing a particular server
   {
      // 'remote name' matches 2nd parm, must be 32-bit flat pointer

      GlobalPageLock(GlobalPtrHandle(lpRSC));
      GlobalPageLock(GlobalPtrHandle((LPSTR)&dwEnum));

      dwEnum = 0;
      dwRval = lpCallProcEx32W(CPEX_DEST_STDCALL | 5, 8 | 16,
                               dwWNetOpenEnum,
                               (DWORD)RESOURCE_GLOBALNET,
                               (DWORD)RESOURCETYPE_ANY,
                               (DWORD)RESOURCEUSAGE_CONTAINER,
                               (DWORD)lpRSC,
                               (DWORD)(LPVOID)&dwEnum);

      GlobalPageUnlock(GlobalPtrHandle((LPSTR)&dwEnum));
      GlobalPageUnlock(GlobalPtrHandle(lpRSC));
   }
   else
   {
      GlobalPageLock(GlobalPtrHandle((LPSTR)&dwEnum));

      dwEnum = 0;
      dwRval = lpCallProcEx32W(CPEX_DEST_STDCALL | 5, 8 | 16,
                               dwWNetOpenEnum,
                               (DWORD)RESOURCE_GLOBALNET,
                               (DWORD)RESOURCETYPE_ANY,
                               (DWORD)RESOURCEUSAGE_CONTAINER,
                               (DWORD)0,  // a NULL parameter here
                               (DWORD)(LPVOID)&dwEnum);

      GlobalPageUnlock(GlobalPtrHandle((LPSTR)&dwEnum));
   }


   if(dwRval)  // this function returns zero on success
   {
      if(bRecurseFlag)
         ReDirectOutput(lpParseInfo, FALSE);


      if(dwRval == 1222)      // ERROR_NO_NETWORK
      {
         PrintString("ERROR - No Network!\r\n");
      }
      else if(dwRval == 1207) // ERROR_NOT_CONTAINER
      {
         PrintString("ERROR - Specified item is not a 'container' on the network\r\n");
      }
      else if(dwRval == 87)   // ERROR_INVALID_PARAMETER
      {
         PrintString("ERROR - Invalid parameter in 'NET VIEW'\r\n");
      }
      else
      {
         char tbuf1[128], tbuf2[128];
         DWORD dwErr;

         GlobalPageLock(GlobalPtrHandle((LPSTR)tbuf1));  // the stack, essentially

         // get error code
         dwRval = lpCallProcEx32W(CPEX_DEST_STDCALL | 5, 1 | 2 | 8,
                                  dwWNetGetLastError,
                                  (DWORD)(LPVOID)&dwErr,
                                  (DWORD)(LPSTR)tbuf1,
                                  (DWORD)sizeof(tbuf1),
                                  (DWORD)(LPSTR)tbuf2,
                                  (DWORD)sizeof(tbuf2));

         GlobalPageUnlock(GlobalPtrHandle((LPSTR)tbuf1));
         if(dwRval)
         {
            // assume it's an internal error

            PrintString("Internal (WIN32) error in NET VIEW\r\n");
         }
         else
         {
            // error code zero is "error information not available"
            // and is most likely due to enumerating the entire network
            // and as such, it ought to be ignored...

            if(!dwErr && bRecurseFlag)  // only when 'recursing'
            {
#ifdef _DEBUG
               _asm int 3
#endif // _DEBUG


               GlobalFreePtr(lpBuf);

               return(FALSE);  // assume "not an error" and continue
            }
            else
            {
               if(*tbuf1)
               {
                  PrintString(tbuf1);
                  PrintString(pCRLF);
               }

               if(*tbuf2)
               {
                  PrintString(tbuf2);
                  PrintString(pCRLF);
               }
            }
         }
      }

      GlobalFreePtr(lpBuf);

      return(TRUE);  // error
   }


             /** Allocate an LDT selector via DPMI **/

   wMySel = MyAllocSelector();

   if(!wMySel)
   {
      if(bRecurseFlag)
         ReDirectOutput(lpParseInfo, FALSE);

      PrintErrorMessage(618);

      GlobalFreePtr(lpBuf);

      lpCallProcEx32W(CPEX_DEST_STDCALL | 1, 0, dwWNetCloseEnum, dwEnum);

      return(TRUE);
   }

   if(!bRecurseFlag)
   {
      ReDirectOutput(lpParseInfo, TRUE);

      if(lpRSC)
      {
         lp1 = (LPSTR)MAKELP(wMySel, 0);

         dwAddr = (DWORD)lpRSC->lpRemoteName;
         MyChangeSelector(wMySel, dwAddr, 0xffff);  // assume it works

         if(lp1[0] == '\\' && lp1[1] == '\\')  // it's a network server name
         {
            PrintString("Network resources for ");
            PrintString(lp1);
            PrintString("\r\n"
                        "Resource Name                   Type    Comment\r\n"
                        "------------------------------- ------- --------------------------------\r\n");
         }
         else  // a workgroup
         {
            PrintString("Servers for workgroup \"");
            PrintString(lp1);
            PrintString("\"\r\n"
                        "Server Name                     Description\r\n"
                        "------------------------------- -----------------------------------------\r\n");
         }
      }
      else
      {
         PrintString("Available Network Resources\r\n\r\n");
      }
   }

   // MAIN LOOP

   GlobalPageLock(GlobalPtrHandle(lpBuf));

   do
   {
      dw1 = 0x10000L / (sizeof(NETRESOURCE) + 2 * MAX_PATH);
      dw2 = 0x10000L;  // size of buffer

      dwRval = lpCallProcEx32W(CPEX_DEST_STDCALL | 4, 2 | 4 | 8,
                               dwWNetEnumResource,
                               dwEnum,
                               (DWORD)(LPVOID)&dw1,
                               (DWORD)lpBuf,
                               (DWORD)(LPVOID)&dw2);

      if(dwRval == 0 /* NO_ERROR */ )
      {
       DWORD dwAddr;

         // 'dw1' is the count of resource items pointed to by 'lpBuf'

         for(dw2 = 0; dw2 < dw1; dw2++)
         {
            LPNETRESOURCE lpR = lpBuf + dw2;

            if(lpR->lpRemoteName == NULL)  // NULL pointer - net provider
            {
               // recurse the sonofabitch!  It's most likely a service
               // provider name

               if(!(lpR->dwUsage & RESOURCEUSAGE_CONTAINER))  // not container?
               {
                  char tbuf[33];

                  _hmemset(tbuf, ' ', 32);
                  tbuf[32] = 0;

                  PrintString(tbuf);

                  dwAddr = (DWORD)lpR->lpComment;
                  if(dwAddr)
                  {
                     MyChangeSelector(wMySel, dwAddr, 0xffff);

                     PrintString(lp1);
                  }
                  else
                  {
                     PrintString("{Unknown resource}");
                  }

                  PrintString(pCRLF);
               }
               else if(!lpR->lpProvider)
               {
                  // when the provider name is NULL, I can't enumerate it

                  continue;  // just skip THIS entry...
               }
               else if(CMDNetEnum(lpR, lpParseInfo, TRUE))
               {
                  GlobalPageUnlock(GlobalPtrHandle(lpBuf));

                  MyFreeSelector(wMySel); // free LDT selector

                  GlobalFreePtr(lpBuf);

                  lpCallProcEx32W(CPEX_DEST_STDCALL | 1, 0, dwWNetCloseEnum, dwEnum);

                  return(TRUE);  // what we do on error
               }
            }
            else if(lpR->dwDisplayType == RESOURCEDISPLAYTYPE_DOMAIN)
            {
               // this is a network domain

               lp1 = (LPSTR)MAKELP(wMySel, 0);

               dwAddr = (DWORD)lpR->lpRemoteName;
               MyChangeSelector(wMySel, dwAddr, 0xffff);  // assume it works

               if(dw2)
                 PrintString(pCRLF); // line break between workgroups

               PrintString("Servers for workgroup \"");
               PrintString(lp1);
               PrintString("\"");

               // "provider" (in case it's NULL, I check the value)

               dwAddr = (DWORD)lpR->lpProvider;
               if(dwAddr)
               {
                  PrintString(" (");  // display the provider within '()'

                  MyChangeSelector(wMySel, dwAddr, 0xffff);

                  PrintString(lp1);

                  PrintString(")");
               }

               // "comment" (in case it's NULL, I check the value)

               dwAddr = (DWORD)lpR->lpComment;
               if(dwAddr)
               {
                  PrintString(" - ");  // display the comment after ' - '

                  MyChangeSelector(wMySel, dwAddr, 0xffff);

                  PrintString(lp1);
               }

               PrintString("\r\n"
                           "Server Name                     Description\r\n"
                           "------------------------------- -----------------------------------------\r\n");


               // now that I have the workgroup name displayed,
               // show the things that reside within it

               if(CMDNetEnum(lpR, lpParseInfo, TRUE))
               {
                  GlobalPageUnlock(GlobalPtrHandle(lpBuf));

                  MyFreeSelector(wMySel); // free LDT selector

                  GlobalFreePtr(lpBuf);

                  lpCallProcEx32W(CPEX_DEST_STDCALL | 1, 0, dwWNetCloseEnum, dwEnum);

                  return(TRUE);  // what we do on error
               }
            }
            else
            {
               lp1 = (LPSTR)MAKELP(wMySel, 0);

               dwAddr = (DWORD)lpR->lpRemoteName;
               MyChangeSelector(wMySel, dwAddr, 0xffff);  // assume it works


               // if it's a server name, I display the whole thing.
               // If it's a share name, I do it a bit differently...

               if(lpR->dwUsage & RESOURCEUSAGE_CONTAINER)  // servers
               {
                  PrintString("  ");

                  PrintString(lp1);

                  // "tab over" to position 32

                  if(lstrlen(lp1) < 30)
                  {
                     char tbuf[32];

                     _hmemset(tbuf, ' ', 31);

                     tbuf[30 - lstrlen(lp1)] = 0;

                     PrintString(tbuf);
                  }
               }
               else
               {
                  // find the 1st backslash after the server name

                  if(*lp1 == '\\' && lp1[1] == '\\')
                  {
                     lp1 += 2;
                     while(*lp1 && *lp1 != '\\')
                        lp1++;

                     if(!*lp1)  // just in case
                        lp1 = (LPSTR)MAKELP(wMySel, 0);  // restore it if none found
                     else // if(*lp1 == '\\')
                        lp1++;  // point past the backslash - resource name!
                  }

                  PrintString(lp1);  // resource name

                  // "tab over" to position 32

                  if(lstrlen(lp1) < 32)
                  {
                     char tbuf[33];

                     _hmemset(tbuf, ' ', 32);

                     tbuf[32 - lstrlen(lp1)] = 0;

                     PrintString(tbuf);
                  }

                  // next 8 columns is the device type - either 'Disk' or 'Printer'

                  if(lpR->dwType & RESOURCETYPE_DISK)
                  {
                     PrintString("Disk    ");
                  }
                  else if(lpR->dwType & RESOURCETYPE_PRINT)
                  {
                     PrintString("Printer ");
                  }
                  else
                  {
                     PrintString("Unknown ");
                  }
               }

               lp1 = (LPSTR)MAKELP(wMySel, 0);  // restore the string pointer

               // "comment" (in case it's NULL, I check the value)

               dwAddr = (DWORD)lpR->lpComment;
               if(dwAddr)
               {
                  MyChangeSelector(wMySel, dwAddr, 0xffff);

                  PrintString(lp1);
               }

               PrintString(pCRLF);
            }
         }

      }

   } while(dwRval == 0);


   if(dwRval != 259 /* ERROR_NO_MORE_ITEMS */)
   {
      ReDirectOutput(lpParseInfo,FALSE);

      if(dwRval == 1222)      // ERROR_NO_NETWORK
      {
         PrintString("ERROR - No Network!\r\n");
      }
      else if(dwRval == 1207) // ERROR_NOT_CONTAINER
      {
         PrintString("ERROR - Specified item is not a 'container' on the network\r\n");
      }
      else if(dwRval == 87)   // ERROR_INVALID_PARAMETER
      {
         PrintString("ERROR - Invalid parameter in 'NET VIEW'\r\n");
      }
      else
      {
         char tbuf1[128], tbuf2[128];
         DWORD dwErr;

         GlobalPageLock(GlobalPtrHandle((LPSTR)tbuf1));  // the stack, essentially

         // get error code
         dwRval = lpCallProcEx32W(CPEX_DEST_STDCALL | 5, 1 | 2 | 8,
                                  dwWNetGetLastError,
                                  (DWORD)(LPVOID)&dwErr,
                                  (DWORD)(LPSTR)tbuf1,
                                  (DWORD)sizeof(tbuf1),
                                  (DWORD)(LPSTR)tbuf2,
                                  (DWORD)sizeof(tbuf2));

         GlobalPageUnlock(GlobalPtrHandle((LPSTR)tbuf1));
         if(dwRval)
         {
            // assume it's an internal error

            PrintString("Internal (WIN32) error in NET VIEW\r\n");
         }
         else
         {
            if(*tbuf1)
            {
               PrintString(tbuf1);
               PrintString(pCRLF);
            }

            if(*tbuf2)
            {
               PrintString(tbuf2);
               PrintString(pCRLF);
            }

//            wsprintf(tbuf1, "WNetGetLastError() returns error code %ld (2)\r\n", dwErr);
//            PrintString(tbuf1);
         }
      }


      GlobalPageUnlock(GlobalPtrHandle(lpBuf));

      MyFreeSelector(wMySel); // free LDT selector

      GlobalFreePtr(lpBuf);

      lpCallProcEx32W(CPEX_DEST_STDCALL | 1, 0, dwWNetCloseEnum, dwEnum);

      return(TRUE);
   }



   // CLEANUP

   GlobalPageUnlock(GlobalPtrHandle(lpBuf));

   MyFreeSelector(wMySel); // free LDT selector

   GlobalFreePtr(lpBuf);

   lpCallProcEx32W(CPEX_DEST_STDCALL | 1, 0, dwWNetCloseEnum, dwEnum);


   return(FALSE);  // no error (and output STILL re-directed)
}



BOOL FAR PASCAL CMDNet     (LPSTR lpCmd, LPSTR lpArgs)
{
LPPARSE_INFO lpParseInfo=(LPPARSE_INFO)NULL;
BOOL bDelete = FALSE;
BOOL bDomain = FALSE;
BOOL bHelp   = FALSE;
LPSTR lpItem1, lpItem2, lpItem3, lp1;
WORD wMySel;
HCURSOR hOldCursor;
int i1;
char tbuf[MAX_PATH];

static const char CODE_BASED szViewHelp[]=
   "  NET VIEW                 - list all servers on the network.\r\n"
   "  NET VIEW workgroup       - list all servers within a workgroup.\r\n"
   "  NET VIEW \\\\server        - list all share names for a server.\r\n\r\n"
   "NOTE:  'NET VIEW' is not supported under Win 3.1x.\r\n";

static const char CODE_BASED szUseHelp[]=
   "  NET USE                  - list all connected network resources.\r\n"
   "  NET USE device sharename - connect the device 'device' to 'sharename'.\r\n"
   "                             (example:  NET USE LPT1: \\server\\printer)\r\n"
   "  NET USE device /DELETE   - disconnect the specified device.\r\n";




   // supported commands:
   //
   // Win '95 and NT only:
   // NET VIEW
   // NET VIEW server
   // NET VIEW /DOMAIN <domain name>
   //
   // NET USE
   // NET USE item /DELETE
   // NET USE item sharename
   //

   if(!lpWNetGetConnection)  // not loaded
   {
     PrintErrorMessage(926); // NET - Function not available
     return(TRUE);
   }


   hOldCursor = MySetCursor(LoadCursor(NULL, IDC_WAIT));

   // parse command line

   if(!(lpParseInfo = CMDLineParse(lpArgs)))
   {
      PrintString(pNOTENOUGHMEMORY);

      MySetCursor(hOldCursor);
      return(TRUE);
   }


   // check for presence of '/DELETE' or '/DOMAIN' or '/?'

   if(lpParseInfo->n_switches)
   {
      if(!_fstricmp(lpParseInfo->sbpItemArray[0],"?") ||
         !_fstricmp(lpParseInfo->sbpItemArray[0],"HELP"))
      {
         bHelp = TRUE;
      }
      else if(!_fstricmp(lpParseInfo->sbpItemArray[0],"DELETE"))
      {
         bDelete = TRUE;
      }
      else if(!_fstricmp(lpParseInfo->sbpItemArray[0],"DOMAIN"))
      {
         bDomain = TRUE;
      }
      else
      {
         PrintErrorMessage(534); // illegal switch
         PrintString((LPSTR)(lpParseInfo->sbpItemArray[0]));
         PrintString(pQCRLFLF);

         GlobalFreePtr(lpParseInfo);

         MySetCursor(hOldCursor);
         return(TRUE);
      }
   }

   if(lpParseInfo->n_switches > 1 && !bHelp)
   {
      PrintErrorMessage(536);  // invalid number of parameters

      GlobalFreePtr(lpParseInfo);

      MySetCursor(hOldCursor);
      return(TRUE);
   }


   // verify arguments

   if(lpParseInfo->n_parameters <= 0 || lpParseInfo->n_parameters > 3)
   {
      PrintErrorMessage(536);  // invalid number of parameters

      GlobalFreePtr(lpParseInfo);

      MySetCursor(hOldCursor);
      return(TRUE);
   }


   lpItem1 = ((LPSTR)(lpParseInfo->sbpItemArray[lpParseInfo->n_switches]));

   if(lpParseInfo->n_parameters > 1)
      lpItem2 = ((LPSTR)(lpParseInfo->sbpItemArray[1 + lpParseInfo->n_switches]));
   else
      lpItem2 = NULL;

   if(lpParseInfo->n_parameters > 2)
      lpItem3 = ((LPSTR)(lpParseInfo->sbpItemArray[2 + lpParseInfo->n_switches]));
   else
      lpItem3 = NULL;


   // check 1st parameter for 'VIEW' or 'USE'

   if(!_fstricmp(lpItem1, "VIEW"))
   {
      // NET VIEW

      if(bHelp)
      {
         PrintString(szViewHelp);

         GlobalFreePtr(lpParseInfo);

         MySetCursor(hOldCursor);
         return(FALSE);            // this is *NOT* an error
      }

      if(bDelete)
      {
         PrintErrorMessage(534); // illegal switch
         PrintString("/DELETE");
         PrintString(pQCRLFLF);

         GlobalFreePtr(lpParseInfo);

         MySetCursor(hOldCursor);
         return(TRUE);
      }


      if(lpItem3)
      {
         PrintErrorMessage(563);
         PrintString(lpItem3);
         PrintString(pQCRLFLF);
      }

      if(!hMPR)
      {
         PrintErrorMessage(927);  // NET VIEW - Function not available
         GlobalFreePtr(lpParseInfo);

         MySetCursor(hOldCursor);
         return(TRUE);  // error
      }


      if(lpItem2)  // viewing a particular server
      {
         NETRESOURCE rsc;

         rsc.dwScope = RESOURCE_GLOBALNET;
         rsc.dwType = RESOURCETYPE_ANY;
         rsc.dwDisplayType = RESOURCEDISPLAYTYPE_GENERIC;
         rsc.dwUsage = RESOURCEUSAGE_CONTAINER;
         rsc.lpLocalName = NULL;
         rsc.lpProvider = NULL;

         // 'remote name' matches 2nd parm, must be 32-bit flat pointer

         GlobalPageLock(GlobalPtrHandle(lpItem2));

         rsc.lpRemoteName = (LPSTR)lpGetVDMPointer32W(lpItem2, 1);
         rsc.lpComment = rsc.lpRemoteName;


         if(CMDNetEnum(&rsc, lpParseInfo, FALSE))
         {
            GlobalPageUnlock(GlobalPtrHandle(lpItem2));

            GlobalFreePtr(lpParseInfo);

            MySetCursor(hOldCursor);
            return(TRUE);
         }

         GlobalPageUnlock(GlobalPtrHandle(lpItem2));
      }
      else
      {
         if(CMDNetEnum(NULL, lpParseInfo, FALSE))
         {
            GlobalFreePtr(lpParseInfo);

            MySetCursor(hOldCursor);
            return(TRUE);
         }
      }
   }
   else if(!_fstricmp(lpItem1, "USE"))
   {
    static const char CODE_BASED szDevices[][5]={
       "A:","B:","C:","D:","E:","F:","G:","H:","I:","J:","K:","L:","M:",
       "N:","O:","P:","Q:","R:","S:","T:","U:","V:","W:","X:","Y:","Z:",
       "LPT1","LPT2","LPT3","LPT4","COM1","COM2","COM3","COM4"
       };


      // NET USE

      if(bHelp)
      {
         PrintString(szUseHelp);

         GlobalFreePtr(lpParseInfo);

         MySetCursor(hOldCursor);
         return(FALSE);            // this is *NOT* an error
      }

      if(bDomain)
      {
         PrintErrorMessage(534); // illegal switch
         PrintString("/DOMAIN");
         PrintString(pQCRLFLF);

         GlobalFreePtr(lpParseInfo);

         MySetCursor(hOldCursor);
         return(TRUE);
      }

      if(!lpItem2)  // no argument after "USE"
      {
         ReDirectOutput(lpParseInfo,TRUE);
         PrintString("List of currently connected devices\r\n"
                     "Local   Remote Share Name\r\n"
                     "------- -----------------------------------------------\r\n");

         // using WNetGetName enumerate every possible drive letter,
         // every printer port, and every COM port.

         for(i1=0; i1 < N_DIMENSIONS(szDevices); i1++)
         {
            UINT cbSize = sizeof(tbuf);
            UINT uiRval = lpWNetGetConnection((LPSTR)szDevices[i1], tbuf, &cbSize);

            tbuf[cbSize] = 0;

            if(uiRval == WN_SUCCESS)
            {
               char szBuf2[10];
               wsprintf(szBuf2, "%-8.8s", (LPCSTR)szDevices[i1]);

               PrintString(szBuf2);
               PrintString(tbuf);
               PrintString(pCRLF);
            }
         }

      }
      else if(bDelete)  // we're deleting a shared resource
      {
         UINT uiRval;

         if(lpItem3)
         {
            PrintErrorMessage(563);  // extraneous argument ignored
            PrintString(lpItem3);
            PrintString(pQCRLFLF);
         }

         uiRval = lpWNetCancelConnection(lpItem2, FALSE);  // don't force closure

try_close_again:

         if(uiRval != WN_SUCCESS)
         {
            if(uiRval == WN_NOT_CONNECTED)
            {
               PrintString("The resource \"");
               PrintString(lpItem3);
               PrintString("\" was not connected\r\n");
            }
            else if(uiRval == WN_OPEN_FILES)
            {
               PrintString("The specified resource has open files on it... close anyway?");
               GetUserInput(tbuf);

               while(*tbuf && tbuf[lstrlen(tbuf)-1] <= ' ')
                  tbuf[lstrlen(tbuf) - 1] = 0;

               while(*tbuf && *tbuf <= ' ')
                  lstrcpy(tbuf, tbuf + 1);

               if(toupper(*tbuf) == 'Y')
               {
                  uiRval = lpWNetCancelConnection(lpItem2, TRUE);  // do force closure
                  goto try_close_again;
               }
            }
            else
            {
               PrintString("Unknown error while trying to delete connection\r\n");
            }
         }
      }
      else // we're adding a shared resource
      {
         UINT uiRval;

         if(!lpItem3)
         {
            PrintString("Missing share name in 'NET USE'\r\n");
         }
         else
         {
            uiRval = lpWNetAddConnection(lpItem3, "", lpItem2);

            if(uiRval != WN_SUCCESS)
            {
               if(uiRval == WN_ALREADY_CONNECTED)
               {
                  PrintString("Already connected to this resource\r\n");
               }
               else if(uiRval == WN_ACCESS_DENIED)
               {
                  PrintString("Access to resource is denied\r\n");
               }
               else if(uiRval == WN_BAD_NETNAME)
               {
                  PrintString("Bad network resource name\r\n");
               }
               else if(uiRval == WN_BAD_LOCALNAME)
               {
                  PrintString("Bad local device name\r\n");
               }
               else if(uiRval == WN_BAD_PASSWORD)
               {
                  PrintString("Internal error (bad password)\r\n");
               }
               else
               {
                  PrintString("Unknown error while trying to connect resource\r\n");
               }
            }
         }
      }

   }
   else
   {
      PrintString("Specified 'NET' option not supported\r\n");
   }

   PrintString(pCRLFLF);

   ReDirectOutput(lpParseInfo,FALSE);
   GlobalFreePtr(lpParseInfo);

   MySetCursor(hOldCursor);
   return(FALSE);  // no error
}



#pragma code_seg("CMDNuketask_TEXT","CODE")

static void FAR PASCAL CMDNuketask_FakeOut(void)
{
   _asm
   {
      mov ax, 0x4c00
      int 0x21
      mov WORD PTR cs:[0], 0      // This causes a GP fault.
      int 0x20                    // so does this.
      push cs
      pop ss                      // and this!
      int 3                       // and this!

   }
}

BOOL FAR PASCAL CMDNuketask(LPSTR lpCmd, LPSTR lpArgs)
{
HTASK hTask, FAR *lphTask;
LPPARSE_INFO lpParseInfo;
TASKENTRY te;
WNDENUMPROC lpProc;
WORD w, wCount;
void (FAR PASCAL *pTerminateApp)(HTASK hTask, WORD wFlags);



   lpParseInfo = CMDLineParse(lpArgs);
   if(CMDCheckParseInfo(lpParseInfo, NULL, 1 | CMD_EXACT))
   {
      GlobalFreePtr(lpParseInfo);
      return(TRUE);
   }

   GlobalFreePtr((LPSTR)lpParseInfo);  /* who needs it now? */

   if(!(hTask = (HTASK)MyValue(lpArgs)))
   {

     /** It's not a valid handle, so look for a module name that matches **/

      _fmemset((LPSTR)&te, 0, sizeof(te));

      te.dwSize = sizeof(te);

      if(!lpTaskFirst || !lpTaskNext || !lpTaskFirst((LPTASKENTRY)&te))
      {
         PrintErrorMessage(607);
         return(TRUE);
      }

      wCount = 0;
      lphTask = (HTASK FAR *)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                            sizeof(HTASK) * GetNumTasks());
      if(!lphTask)
      {
         PrintString(pNOTENOUGHMEMORY);
         return(TRUE);
      }

      do
      {
         if(lpModuleFindHandle)
         {
          MODULEENTRY me;

            _fmemset((LPVOID)&me, 0, sizeof(me));

            me.dwSize = sizeof(me);

            if(!lpModuleFindHandle(&me, te.hModule)) continue; // error

            if(!_fstricmp(me.szModule, lpArgs))
            {
               lphTask[wCount++] = te.hTask;
            }
         }
         else if(!_fstricmp(te.szModule, lpArgs))
         {
            lphTask[wCount++] = te.hTask;
         }

      } while(lpTaskNext((LPTASKENTRY)&te));

      if(wCount>1)
      {
         lpProc = (WNDENUMPROC)MakeProcInstance((FARPROC)CMDCloseTaskEnum2,
                                                hInst);

         PrintErrorMessage(538);  // "** MULTIPLE Tasks have this module name **\r\n"
                                  // "Entry  Task Handle  Window Caption\r\n"
                                  // "-----  -----------  --------------\r\n"

         for(w=0; w<wCount; w++)
         {
            if(lpTaskFindHandle((LPTASKENTRY)&te, lphTask[w]))
            {
             static const char CODE_BASED pFmt[]=
                                                   " %2d)     %04x       ";

               LockCode();
               wsprintf(work_buf, pFmt, w+1, (WORD)te.hTask);
               UnlockCode();

               PrintString(work_buf);

               CloseTaskEnumTask = lphTask[w];
               CloseTaskEnumCount = 0;

               EnumWindows(lpProc, (LPARAM)NULL);

               if(!CloseTaskEnumCount) PrintErrorMessage(539); // " ** N/A **\r\n"

               PrintString(pCRLF);
            }
         }

         FreeProcInstance((FARPROC)lpProc);

         PrintErrorMessage(608);     // "Which Task to KILL (1,2,3,etc.) or <Return> to cancel? "
         GetUserInput(work_buf);

         if(*work_buf<' ')
         {
            PrintErrorMessage(541);  // "** CANCELLED **\r\n\n"

            GlobalFreePtr(lphTask);
            return(TRUE);
         }
         else if((w = (WORD)MyValue(work_buf))>wCount || w<1)
         {
            PrintErrorMessage(609);
            PrintString(work_buf);
            PrintString(pQCRLFLF);

            GlobalFreePtr(lphTask);
            return(TRUE);
         }
         else
         {
            hTask = lphTask[w - 1];
         }

      }
      else if(wCount==0)
      {
         PrintErrorMessage(610);
         PrintString(lpArgs);
         PrintString(pQCRLFLF);

         GlobalFreePtr(lphTask);
         return(TRUE);
      }
      else
      {
         hTask = lphTask[0];
      }

      GlobalFreePtr(lphTask);
   }

   if(!lpIsTask(hTask))
   {
    static const char CODE_BASED pFmt[]=
                                     "?Task ID %04xH(%d) not found...\r\n\n";

      LockCode();
      wsprintf(work_buf, pFmt, (WORD)hTask, (WORD)hTask);
      UnlockCode();

      PrintString(work_buf);
      return(TRUE);
   }


   // NUKING 'hTask'!


   if(hToolHelp)
   {
    static const char CODE_BASED szTerminateApp[]="TerminateApp";

      (FARPROC &)pTerminateApp = GetProcAddress(hToolHelp, szTerminateApp);
   }
   else
   {
      pTerminateApp = NULL;
   }


   if(pTerminateApp)
   {
      pTerminateApp(hTask, NO_UAE_BOX);
   }
   else
   {
    DWORD dwProcAddress = (DWORD)CMDNuketask_FakeOut;

      // Here is where I must get CREATIVE and nuke the task somehow...
      // What I'll do is assign a CS:IP to the task which ends it!!


      _asm
      {
         push es
         push si
         mov es, hTask
         les si, DWORD PTR es:[2]    // offset to SS:SP within TDB

         mov ax, WORD PTR dwProcAddress
         mov WORD PTR es:[si + 12H], ax
         mov WORD PTR es:[si + 14H], cs

         pop si
         pop es

      }

      PostAppMessage(hTask, WM_NULL, 0, 0);
      DirectedYield(hTask);          // this will switch to *that* task!

   }


   return(FALSE);

}




#pragma code_seg("CMDPath_TEXT","CODE")


BOOL FAR PASCAL CMDPath    (LPSTR lpCmd, LPSTR lpArgs)
{
LPSTR lp1;
static char pPATH[]="PATH";


   if(*lpArgs==0)
   {
      PrintString(pPATH);
      PrintAChar('=');
      PrintString(GetEnvString(pPATH));
      PrintString(pCRLFLF);
      return(FALSE);
   }

   if(*lpArgs=='=')
      lp1 = lpArgs + 1;
   else
      lp1 = lpArgs;

   return(SetEnvString(pPATH, lp1));
}


#pragma code_seg("CMDPause_TEXT","CODE")


BOOL FAR PASCAL CMDPause   (LPSTR lpCmd, LPSTR lpArgs)
{

   lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   if(*lpArgs)
   {
      PrintString(lpArgs);
   }
   else
   {
      PrintErrorMessage(796);  // "Press <ENTER> to continue:  "
   }

   pausing_flag = TRUE;         /* assign global flag TRUE to cause pause */

   while(pausing_flag)
   {
      if(LoopDispatch())    /* returns TRUE on WM_QUIT */
      {
         PrintErrorMessage(635);

         GlobalFreePtr(lpArgs);
         return(FALSE);
      }
   }

   PrintString(pCRLFLF);

   GlobalFreePtr(lpArgs);
   return(FALSE);            /* that's all, folks! */

}



#pragma code_seg("CMDPlaySound_TEXT","CODE")


BOOL FAR PASCAL CMDPlaySound(LPSTR lpCmd, LPSTR lpArgs)
{
LPSTR lp1, lp2;
BOOL rval;
OFSTRUCT ofstr;


   if(!lpsndPlaySound)
   {
      PrintErrorMessage(636);
      return(TRUE);
   }

   lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }


   INIT_LEVEL


   if(!*lpArgs)
   {
      lpsndPlaySound(NULL, SND_SYNC);
      rval = FALSE;
      break;
   }
   else if(*lpArgs=='/')
   {
      if((lpArgs[1]=='L' || lpArgs[1]=='l' ||
          lpArgs[1]=='w' || lpArgs[1]=='W') && lpArgs[2]==' ')
      {
         lp1 = lpArgs+3;
         while(*lp1 && *lp1<=' ') lp1++;

         if(!*lp1)
         {
            PrintErrorMessage(637);
            PrintString(lpArgs);
            PrintString(pQCRLFLF);
            rval = TRUE;
            break;
         }


         // quote marks?  strip them!

         if(*lp1 == '\"')
         {
            lp2 = ++lp1;

            while(*lp2 && *lp2 != '\"') lp2++;

            if(*lp2 == '\"') *(lp2++) = 0;

            // does anything follow the name?

            while(*lp2 && *lp2 <= ' ') lp2++;

            if(*lp2)
            {
               PrintString(pSYNTAXERROR);
               PrintString(lp2);
               PrintString(pQCRLFLF);

               rval = TRUE;
               break;
            }

         }

         lp2 = lp1;

         if(!(GetPathNameInfo(lp1) & PATH_HAS_EXT))
         {
            lstrcpy(work_buf, lp1);
            lstrcat(work_buf, ".WAV");
            lp2 = work_buf;
         }

         if(MyOpenFile(lp2, &ofstr, OF_EXIST)==OF_ERROR)
         {
            // if it isn't a file name, attempt to find it from 'WIN.INI'
            // exactly as typed in by the user!

            if(lpArgs[1]=='w' || lpArgs[1]=='W')
            {
               if(!lpsndPlaySound(lp1, SND_SYNC | SND_NOSTOP | SND_NODEFAULT))
               {
                  PrintErrorMessage(638);
                  PrintString(lp1);
                  PrintString(pQCRLFLF);
                  rval = TRUE;
               }
               else
               {
                  rval = FALSE;
               }
            }
            else
            {
               if(!lpsndPlaySound(lp1, SND_ASYNC | SND_LOOP | SND_NOSTOP | SND_NODEFAULT))
               {
                  PrintErrorMessage(638);
                  PrintString(lp1);
                  PrintString(pQCRLFLF);
                  rval = TRUE;
               }
               else
               {
                  rval = FALSE;
               }
            }
         }
         else
         {
           /* function 'sndPlaySound' returns TRUE if sound is played */
            rval = !lpsndPlaySound(ofstr.szPathName,
                                   SND_SYNC | SND_NOSTOP | SND_NODEFAULT);

         }

      }
      else
      {
         PrintErrorMessage(534);
         PrintString(lpArgs);
         PrintString(pQCRLFLF);
         rval = TRUE;
      }
   }
   else
   {
      lp1 = lpArgs;

      // quote marks?  strip them!

      if(*lp1 == '\"')
      {
         lp2 = ++lp1;

         while(*lp2 && *lp2 != '\"') lp2++;

         if(*lp2 == '\"') *(lp2++) = 0;

         // does anything follow the name?

         while(*lp2 && *lp2 <= ' ') lp2++;

         if(*lp2)
         {
            PrintString(pSYNTAXERROR);
            PrintString(lp2);
            PrintString(pQCRLFLF);

            rval = TRUE;
            break;
         }

      }

      lp2 = lp1;


      if(!(GetPathNameInfo(lp1) & PATH_HAS_EXT))
      {
         lstrcpy(work_buf, lp1);
         lstrcat(work_buf, ".WAV");
         lp2 = work_buf;
      }

      if(MyOpenFile(lp2, &ofstr, OF_EXIST)==OF_ERROR)
      {
         // if it isn't a file name, attempt to find it from 'WIN.INI'
         // exactly as typed in by the user!

         if(!lpsndPlaySound(lp1, SND_ASYNC | SND_NOSTOP | SND_NODEFAULT))
         {
            PrintErrorMessage(638);
            PrintString(lp1);
            PrintString(pQCRLFLF);
            rval = TRUE;
         }
         else
         {
            rval = FALSE;
         }
      }
      else
      {
         rval = !lpsndPlaySound(ofstr.szPathName,
                                SND_ASYNC | SND_NOSTOP | SND_NODEFAULT);
      }
   }


   END_LEVEL


   GlobalFreePtr(lpArgs);
   return(rval);
}



#pragma code_seg("CMDPrint_TEXT","CODE")


BOOL FAR PASCAL CMDPrint   (LPSTR lpCmd, LPSTR lpArgs)
{
HINSTANCE hRval;
LPSTR lp1;

   if(!lpShellExecute)
   {
      PrintErrorMessage(639);
      return(TRUE);
   }

   lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   lp1 = work_buf + sizeof(work_buf) / 2;

   DosQualifyPath(lp1, lpArgs);  /* get 'qualified' path name for file */

   if(!*lp1)
   {
      PrintString(pINVALIDPATH);
      PrintString(lpArgs);
      PrintString(pQCRLFLF);

      GlobalFreePtr(lpArgs);
      return(TRUE);
   }

   GlobalFreePtr(lpArgs);

   DosQualifyPath(work_buf, "."); /* a quickie way to get the current dir */

   hRval = lpShellExecute(hMainWnd, "print", lp1, NULL, work_buf,
                           SW_SHOWMINNOACTIVE);

   if(hRval>=(HINSTANCE)32) return(FALSE);  /* worked */

   if(hRval==(HINSTANCE)31)
   {
      PrintErrorMessage(640);
      return(TRUE);
   }

   if(!CMDRunErrorMessage((WORD)hRval))
   {
      PrintErrorMessage(641);
   }
   return(TRUE);

}


#pragma code_seg("CMDPrompt_TEXT","CODE")


BOOL FAR PASCAL CMDPrompt  (LPSTR lpCmd, LPSTR lpArgs)
{
LPSTR lp1;
BOOL rval;
static char pPROMPT[]="PROMPT";

   lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   if(*lpArgs==0)
   {
      PrintString(pPROMPT);
      PrintAChar('=');
      PrintString(GetEnvString(pPROMPT));
      PrintString(pCRLFLF);
      return(FALSE);
   }

   if(*lpArgs=='=')
      lp1 = lpArgs + 1;
   else
      lp1 = lpArgs;

   rval = SetEnvString(pPROMPT, lp1);

   GlobalFreePtr(lpArgs);
   return(rval);

}




#pragma code_seg("CMDRem_TEXT","CODE")


BOOL FAR PASCAL CMDRem     (LPSTR lpCmd, LPSTR lpArgs)
{
   return(FALSE);       /* ignore it, just like it says to! */
}



#pragma code_seg("CMDRemove_TEXT","CODE")


BOOL FAR PASCAL CMDRemove  (LPSTR lpCmd, LPSTR lpArgs)
{
HMODULE hModule;
LPPARSE_INFO lpParseInfo;
TASKENTRY te;


   lpParseInfo = CMDLineParse(lpArgs);
   if(CMDCheckParseInfo(lpParseInfo, NULL, 1 | CMD_EXACT))
   {
      GlobalFreePtr(lpParseInfo);
      return(TRUE);
   }

   GlobalFreePtr((LPSTR)lpParseInfo);  /* who needs it now? */

   if(!(hModule = (HMODULE)MyValue(lpArgs)))
   {

     /** It's not a valid handle, so look for a module name that matches **/

      hModule = GetModuleHandle(lpArgs);  /* obtain module handle */

   }

   if(!hModule || !GetModuleFileName(hModule, work_buf, sizeof(work_buf)))
   {
      PrintErrorMessage(642);
      PrintString(lpArgs);
      PrintErrorMessage(643);
      return(TRUE);
   }
         /** Verify that it is *NOT* a TASK module! **/


   _fmemset((LPSTR)&te, 0, sizeof(te));

   te.dwSize = sizeof(te);

   if(!lpTaskFirst || !lpTaskNext || !lpTaskFirst((LPTASKENTRY)&te))
   {
      PrintErrorMessage(644);
      return(TRUE);
   }


   do
   {
      if(hModule==te.hModule)  /* the task has the same module handle */
      {
         PrintErrorMessage(645);
         return(TRUE);
      }

   } while(lpTaskNext((LPTASKENTRY)&te));


   while(hModule)
   {
      FreeLibrary(hModule);
      hModule = GetModuleHandle(work_buf);
   }

   PrintErrorMessage(797);  // "Module \""
   PrintString(work_buf);
   PrintErrorMessage(798);  // "\" successfully unloaded!\r\n\n"

   return(FALSE);

}



#pragma code_seg("CMDRename_TEXT","CODE")


BOOL FAR PASCAL CMDRename  (LPSTR lpCmd, LPSTR lpArgs)
{
LPPARSE_INFO lpParseInfo=(LPPARSE_INFO)NULL;
NPSTR np1, np2;
WORD file_cnt, c;
LPDIR_INFO lpDirInfo=(LPDIR_INFO)NULL, lpDI;
char path_buf[256];
BOOL rval;


    /* this is not the same as move, so allow DIR's to be specified */


   lpParseInfo = CMDLineParse(lpArgs);

   if(CMDCheckParseInfo(lpParseInfo, NULL, 2 | CMD_EXACT))
   {                            /* no switches, exactly 2 commands */
      GlobalFreePtr(lpParseInfo);
      return(TRUE);
   }


   if(GetPathNameInfo(lpParseInfo->sbpItemArray[lpParseInfo->n_switches + 1])
      & (PATH_HAS_DRIVE | PATH_HAS_DIR))
   {
      GlobalFreePtr((LPSTR)lpParseInfo);
      PrintErrorMessage(646);
      return(FALSE);
   }

   np1 = work_buf + (sizeof(work_buf) / 2);

   lstrcpy(work_buf, lpParseInfo->sbpItemArray[lpParseInfo->n_switches]);

             /** At first, only rename files (not sub-dir's) **/

   file_cnt = GetDirList0(work_buf, ~_A_VOLID & 0xff,
                          (LPDIR_INFO FAR *)&lpDirInfo,
                          (LPSTR)path_buf, FALSE, NULL);

   if(file_cnt==0 || file_cnt==DIRINFO_ERROR)
   {
      if(lpParseInfo) GlobalFreePtr((LPSTR)lpParseInfo);
      if(lpDirInfo)   GlobalFreePtr(lpDirInfo);

      if(file_cnt==0)
      {
         PrintErrorMessage(548);
         return(FALSE);
      }
      else
         return(CMDErrorCheck(TRUE));
   }

   for(rval = FALSE, c=0, lpDI = lpDirInfo; c<file_cnt; c++, lpDI++)
   {
      memset(work_buf, 0, sizeof(work_buf));  /* clear entire buffer */

      if(*(lpDI->fullname)=='.')  /* for '.' and '..' just go on... */
         continue;

      lstrcpy(work_buf, (LPSTR)path_buf);
      if(work_buf[strlen(work_buf) - 1]!='\\')
         strcat(work_buf, pBACKSLASH);

      np2 = work_buf + strlen(work_buf);  /* points to where name will be */
      _fstrncpy(np2, lpDI->fullname, sizeof(lpDI->fullname));

      if(IsChicago || IsNT)
      {
         ShortToLongName(work_buf, NULL);      // convert to LONG file name
         np2 = strrchr(work_buf, '\\') + 1;  // it *BETTER* work!!
      }

      lstrcpy(np1, (LPSTR)path_buf);
      if(np1[strlen(np1) - 1]!='\\')
         strcat(np1, pBACKSLASH);

      lstrcat(np1, lpParseInfo->sbpItemArray[lpParseInfo->n_switches + 1]);

      if(UpdateFileName2(np1, np1, np2, FALSE))
      {                // subst '?' and '*' in 'np1' with characters in 'np2'
                       // except that directory names in 'np1' stay 'as is'
         rval = TRUE;
         break;
      }

      if(!IsChicago && !IsNT)            // only if LFN's not supported
      {
         _strupr(work_buf);
         _strupr(np1);            /* just to make the output look nice, ok? */
      }

      PrintString(work_buf);
      if(lstrlen(work_buf) > 30) PrintString(pCRLF);

      PrintErrorMessage(795);   // " --> "
      PrintString(np1);
      if(lstrlen(np1) > 60) PrintString(pCRLF);

      if(MyRename(work_buf, np1)) PrintErrorMessage(527); // " ** ERROR **\r\n"
      else                        PrintErrorMessage(527); // "  ** OK **\r\n"

   }

   GlobalFreePtr((LPSTR)lpDirInfo);
   GlobalFreePtr((LPSTR)lpParseInfo);

   return(CMDErrorCheck(rval));


}


#pragma code_seg("CMDReplace_TEXT","CODE")


BOOL FAR PASCAL CMDReplace (LPSTR lpCmd, LPSTR lpArgs)
{
LPPARSE_INFO lpParseInfo;
LPTREE lpTree, lpT;
LPDIR_INFO lpDirInfo=NULL, lpDI;
HCURSOR hOldCursor;
LPSTR lp1, lp2, lp3, lpPath, lpDest, lpCurPath, lpSrcPath, lpDirSpec,
      lpCurDest, lpDestPath, lpCurPath0;
BOOL AFlag,PFlag,RFlag,SFlag,UFlag,VFlag,WFlag,rval,CopyFlag;
BOOL PromptedFlag, CopiedFlag;
int RdOnlyFlag;  // THIS MUST BE AN 'int' (attrib)
char c;
WORD w, p, attr, wNFiles, wRecurse, wNRecurse, wTreeLevel, wTreeIndex, ctr;
WORD wLevelArray[64];
struct _find_t ff;



   /* REPLACE source [destination] switches */

   /* SWITCHES:  /a      -  add files from source to dest                  */
   /*            /p      -  'pause' (confirm on each file)                 */
   /*            /r      -  replace 'read-only' files also.                */
   /*            /s      -  recurse sub-directories on DEST path only!     */
   /*            /u      -  update (replace ONLY "older" files)            */
   /*            /v      -  verify (enhancement!)                          */
   /*            /w      -  prompts for correct disk first                 */
   /*                       (only for diskettes - ignored otherwise)       */


   hOldCursor = MySetCursor(LoadCursor(NULL, IDC_WAIT));

   lpParseInfo = CMDLineParse(lpArgs);

   if(CMDCheckParseInfo(lpParseInfo, "APRSUVW", 1 | CMD_ATLEAST))
   {
      GlobalFreePtr(lpParseInfo);
      MySetCursor(hOldCursor);
      return(TRUE);
   }

   /* well, here we go! */

   AFlag=PFlag=RFlag=SFlag=UFlag=VFlag=WFlag=FALSE;

   CopiedFlag = FALSE;
   PromptedFlag = FALSE;


   p = lpParseInfo->n_switches;  /* # of switches, 1st argument */

   for(w=0; w<p; w++)
   {
      c = toupper(*(lpParseInfo->sbpItemArray[w]));

      switch(c)
      {
         case 'A':
            AFlag = TRUE;
            break;

         case 'P':
            PFlag = TRUE;
            break;

         case 'R':
            RFlag = TRUE;
            break;

         case 'S':
            SFlag = TRUE;
            break;

         case 'U':
            UFlag = TRUE;
            break;

         case 'V':
            VFlag = TRUE;
            break;

         case 'W':
            WFlag = TRUE;
            break;

         default:

            PrintString(pSYNTAXERROR);
            PrintAChar('/');
            PrintString(lpParseInfo->sbpItemArray[w]);
            PrintString(pQCRLFLF);

            GlobalFreePtr(lpParseInfo);
            MySetCursor(hOldCursor);
            return(TRUE);
      }
   }

   // check for non-valid combinations

   if(AFlag && (UFlag || SFlag))
   {
    static const char CODE_BASED szMsg[]=
        "?Cannot specify '/A' with either '/S' or '/U' in REPLACE\r\n\n";

      PrintString(szMsg);

      GlobalFreePtr(lpParseInfo);
      MySetCursor(hOldCursor);
      return(TRUE);
   }


   lpPath = lpParseInfo->sbpItemArray[p];    /* the FIRST one! */

   if(lpParseInfo->n_parameters==2)
   {
      lpDest = lpParseInfo->sbpItemArray[p+1]; /* the SECOND one! */
   }
   else if(lpParseInfo->n_parameters==1)
   {
      lpDest = ".";
   }
   else
   {
      PrintErrorMessage(536);
      GlobalFreePtr(lpParseInfo);
      MySetCursor(hOldCursor);
      return(TRUE);
   }


   lp3 = work_buf + sizeof(work_buf)/2;

   if(QualifyPath(work_buf, lpPath, TRUE))
   {
      PrintString(pILLEGALARGUMENT);
      PrintString(lpPath);
      PrintString(pQCRLFLF);

      GlobalFreePtr(lpParseInfo);
      MySetCursor(hOldCursor);
      return(TRUE);
   }
   else
   {
      lp1 = _fstrrchr(work_buf, '\\');
      if(!lp1)
      {
         PrintString(pILLEGALARGUMENT);
         PrintString(lpPath);
         PrintString(pQCRLFLF);

         GlobalFreePtr(lpParseInfo);
         MySetCursor(hOldCursor);
         return(TRUE);
      }
   }

   lpPath = work_buf;


   if(QualifyPath(lp3, lpDest, TRUE))
   {
      PrintString(pILLEGALARGUMENT);
      PrintString(lpDest);
      PrintString(pQCRLFLF);

      GlobalFreePtr(lpParseInfo);
      MySetCursor(hOldCursor);
      return(TRUE);
   }
   else
   {
      lp2 = _fstrrchr(lp3, '\\');
      if(!lp2 || (lstrcmp(lp2, "\\*.*") && lstrcmp(lp2, "\\????????.???")))
      {
         if(!lp2)
         {
            PrintString(pILLEGALARGUMENT);
            PrintString(lpDest);
            PrintString(pQCRLFLF);
         }
         else
         {
            PrintErrorMessage(515);  // invalid destination path
         }

         GlobalFreePtr(lpParseInfo);
         MySetCursor(hOldCursor);
         return(TRUE);
      }


      lp2[1] = 0;    // terminate path here!!  ends in '\' as it ought to...
   }


   MySetCursor(hOldCursor);


   lpDest = lp3;




   MySetCursor(LoadCursor(NULL, IDC_WAIT));  /* hourglass is back! */

   lpSrcPath = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE, sizeof(work_buf) +
                              lstrlen(lpPath) + lstrlen(lpDest));
   if(!lpSrcPath)
   {
      PrintString(pNOTENOUGHMEMORY);
      GlobalFreePtr(lpParseInfo);
      MySetCursor(hOldCursor);
      return(TRUE);
   }


   *lp1 = 0;
   lstrcpy(lpSrcPath, lpPath);      /* make a copy in global memory block */
   lstrcat(lpSrcPath, pBACKSLASH);

   lpDirSpec = lpSrcPath + lstrlen(lpSrcPath) + 1;
   lstrcpy(lpDirSpec, lp1 + 1);       /* make copy of the 'dir spec' used */


   *lp2 = 0;
   lpDestPath = lpDirSpec + lstrlen(lpDirSpec) + 1;
   lstrcpy(lpDestPath, lpDest);     /* make a copy in global memory block */
   lstrcat(lpDestPath, pBACKSLASH);

   // these next 2 are 'work areas'

   lpCurPath = lpDestPath + lstrlen(lpDestPath) + 1;

// update 7/15/98 - use long name, not short name!
   lpCurPath0= lpCurPath + sizeof(work_buf)/4;

   lpCurDest = lpCurPath0 + sizeof(work_buf)/4;
// end of update 7/15/98





            /** This begins the 'recurse specific' code **/

   if(SFlag)
   {
      lstrcpy(work_buf, lpDestPath);

      if(!(lpTree = GetDirTree(work_buf)))
      {
         if(ctrl_break)
         {
            PrintString(pCTRLC);
            PrintString(pCRLFLF);

            if(!BatchMode) ctrl_break = FALSE;  // eat ctrl break if not BATCH
         }
         else
         {
            PrintErrorMessage(576);
         }

         GlobalFreePtr(lpSrcPath);
         GlobalFreePtr(lpParseInfo);

         MySetCursor(hOldCursor);
         return(TRUE);
      }

      lpT = lpTree;

      if(!(*lpTree->szDir))  /* empty tree list? */
      {
         wNRecurse = 1;
         SFlag = FALSE;
         GlobalFreePtr(lpTree);
         lpTree = NULL;
      }
      else
      {
         wNRecurse = 0xffff;  /* for now, it's a bit easier... */
         wTreeLevel = 0;
         wTreeIndex = 0;

         wLevelArray[wTreeLevel] = wTreeIndex;
      }
   }
   else
   {
      wNRecurse = 1;   /* always go at least 1 time; helps detect errors */
      lpTree = NULL;
   }


   attr = _A_HIDDEN|_A_SYSTEM|_A_ARCH|_A_RDONLY; /* enhancement */


   if(WFlag)
   {
      MySetCursor(hOldCursor);

      PrintErrorMessage(686);
      GetUserInput(work_buf);

      if(ctrl_break)
      {
         PrintString(pCTRLBREAKMESSAGE);

         GlobalFreePtr(lpSrcPath);
         GlobalFreePtr(lpParseInfo);

         if(lpTree)  GlobalFreePtr(lpTree);

         if(!BatchMode) ctrl_break = FALSE;  // if not BATCH mode, eat it!

         return(TRUE);
      }

      PrintString(pCRLF);

      MySetCursor(LoadCursor(NULL, IDC_WAIT));
   }


   for(wRecurse=0; wRecurse<wNRecurse; wRecurse++)
   {
                        /** Get directory list buffer **/

      lstrcpy(lpCurPath, lpSrcPath);
      lstrcat(lpCurPath, lpDirSpec);

      if(wRecurse && SFlag)
      {
         lstrcpy(lpCurDest, lpDestPath);

         for(ctr=0; ctr<=wTreeLevel; ctr++)  /* build current path */
         {
            lstrcat(lpCurDest, lpTree[wLevelArray[ctr]].szDir);
            lstrcat(lpCurDest, pBACKSLASH);
         }

         if(lpTree[wTreeIndex].wChild)  /* a child element exists? */
         {
            wLevelArray[wTreeLevel++] = wTreeIndex;
            wTreeIndex = lpTree[wTreeIndex].wChild;

            wLevelArray[wTreeLevel] = wTreeIndex;
         }
         else if(lpTree[wTreeIndex].wSibling)
         {
            wTreeIndex = lpTree[wTreeIndex].wSibling;
            wLevelArray[wTreeLevel] = wTreeIndex;
         }
         else
         {
            while(wTreeLevel>0 && !lpTree[wTreeIndex].wSibling)
            {
               wTreeIndex = wLevelArray[--wTreeLevel];
            }

            if(!wTreeLevel && !lpTree[wTreeIndex].wSibling)
            {
               wNRecurse = wRecurse; /* end after this one! */
            }
            else
            {
               wTreeIndex = lpTree[wTreeIndex].wSibling;
               wLevelArray[wTreeLevel] = wTreeIndex;
            }
         }
      }
      else
      {        /* first time through - current dir (always) */

         lstrcpy(lpCurDest, lpDestPath);
      }

      wNFiles = GetDirList(lpCurPath, attr, (LPDIR_INFO FAR *)&lpDirInfo, NULL);

      if(PFlag) MySetCursor(hOldCursor);  /* no hourglass if prompting */

// update 7/15/98 - use long name, not short name!
      lp2 = _fstrrchr(lpCurPath, '\\');

      if(lp2)
        lp2++;
      else
        lp2 = lpCurPath;

      *lp2 = 0;
// end of update 7/15/98


      lp1 = lpCurDest + lstrlen(lpCurDest);


      for(w=0, lpDI=lpDirInfo; w<wNFiles; )
      {
         CopyFlag = FALSE;    // initial value
         RdOnlyFlag = FALSE;  // initial value

         // build appropriate dest file name...

//         _fmemset(lp1, 0, sizeof(lpDI->fullname) + 2);
//         _fstrncpy(lp1, lpDI->fullname, sizeof(lpDI->fullname));
// update 7/15/98 - use long name, not short name!

         _fmemset(lp2, 0, sizeof(lpDI->fullname) + 2);
         _fmemcpy(lp2, lpDI->fullname, sizeof(lpDI->fullname));

         if(GetLongName(lpCurPath, lpCurPath0))
         {
           lstrcpy(lpCurPath0, lpCurPath);  // make a copy
         }

         lp3 = _fstrrchr(lpCurPath0, '\\');
         if(!lp3)
           lp3 = lpCurPath0;
         else
           lp3++;

         lstrcpy(lp1, lp3);  // copy LONG name into this path (not short name)

// end of update 7/15/98


         if(AFlag)  // file should *NOT* exist!
         {
            if(!MyFindFirst(lpCurDest, ~_A_VOLID, &ff))
            {
               MyFindClose(&ff);
            }
            else
            {
               CopyFlag = TRUE;
            }
         }
         else
         {
            if(!MyFindFirst(lpCurDest, ~_A_VOLID, &ff))
            {
               if(RFlag)
               {
                  if(ff.attrib & _A_RDONLY)  // dest is read-only file!!
                  {
                     RdOnlyFlag = ff.attrib;
                  }

                  CopyFlag = TRUE;
               }
               else if(!(ff.attrib & _A_RDONLY))  // no READ-ONLY files!
               {
                  CopyFlag = TRUE;
               }


               if(CopyFlag)          // the 'readonly' test passed...
               {
                  CopyFlag = FALSE;  // reset it

                  if(UFlag)  // UPDATE - source must be *NEWER*
                  {
                     if(lpDI->date > ff.wr_date ||
                        (lpDI->date == ff.wr_date &&
                         lpDI->time > ff.wr_time))
                     {
                        CopyFlag = TRUE;
                     }
                  }
                  else
                  {
                     CopyFlag = TRUE;
                  }
               }

               MyFindClose(&ff);
            }
         }

         if(CopyFlag && PFlag)
         {
            PromptedFlag = TRUE;            // at least ONE file matched...

            if(!CMDReplaceConfirm(lpCurDest))
            {
               if(ctrl_break)      /* only when using 'confirm'! */
               {
                  PrintString(pCTRLBREAKMESSAGE);

                  GlobalFreePtr(lpSrcPath);
                  GlobalFreePtr(lpParseInfo);

                  if(lpTree)    GlobalFreePtr(lpTree);
                  if(lpDirInfo) GlobalFreePtr(lpDirInfo);

                  if(!BatchMode) ctrl_break = FALSE;  // if not BATCH, eat it!

                  return(TRUE);
               }

               CopyFlag = FALSE;  // user did *NOT* confirm this file!
            }
         }


         if(CopyFlag)
         {
            if(RFlag && RdOnlyFlag)
            {
               if(MySetFileAttr(lpCurDest, RdOnlyFlag & (~_A_RDONLY)))
               {
                static const char CODE_BASED szMsg[]=
                    "?Unable to clear read-only bit on destination file.\r\n";

                  PrintString(szMsg);

                  if(lpTree)    GlobalFreePtr(lpTree);
                  if(lpDirInfo) GlobalFreePtr(lpDirInfo);

                  GlobalFreePtr(lpSrcPath);
                  GlobalFreePtr(lpParseInfo);

                  return(TRUE);
               }
            }

            lpDI++;
            w++;
         }
         else
         {
            if((--wNFiles))
            {
               _hmemcpy((LPSTR)lpDI, (LPSTR)(lpDI + 1),
                        (wNFiles - w) * sizeof(*lpDI));
            }

            _hmemset((LPSTR)(lpDI + wNFiles), 0, sizeof(*lpDI));
         }
      }

      if(PFlag) MySetCursor(LoadCursor(NULL, IDC_WAIT));


      if(wNFiles)  /* if we have files to copy, that is... */
      {
         CopiedFlag = TRUE;  // at least ONE file was sent to the queue

         *lp1 = 0; // terminate path for 'lpCurDest' to only contain dir

         lp1 = _fstrrchr(lpCurPath, '\\'); /* find last '\' in source path */
         if(lp1)
         {
            *(lp1 + 1) = 0;  /* terminate string just after '\' */
         }

         rval = SubmitDirInfoToCopy(lpDirInfo, wNFiles, lpCurPath,
                                    lpCurDest, VFlag, FALSE);

         if(rval)
         {
            PrintErrorMessage(687);
            GlobalFreePtr(lpDirInfo);
            lpDirInfo = NULL;

            break;
         }

         lpDirInfo = NULL;
      }
      else
      {
         if(lpDirInfo)  GlobalFreePtr(lpDirInfo);
         lpDirInfo = NULL;
      }
   }


   // if I didn't have '/P', or I didn't find ANY matching files, or
   // I copied at least ONE file, report the copy results.

   if(!PFlag || !PromptedFlag || CopiedFlag) ReportCopyResults();
//   else
//   {
//      TotalCopiesSubmitted = 0;
//
//      UpdateStatusBar();
//   }

   if(lpTree)     GlobalFreePtr(lpTree);
   if(lpDirInfo)  GlobalFreePtr(lpDirInfo);

   GlobalFreePtr(lpSrcPath);
   GlobalFreePtr(lpParseInfo);

   MySetCursor(hOldCursor);
   return(rval);


}

BOOL FAR PASCAL CMDReplaceConfirm(LPSTR lpPathSpec)
{
LPSTR lp1;
char temp_buf[64];


   do
   {
      PrintString("REPLACE ");

      PrintString(lpPathSpec);
      PrintErrorMessage(847);   // " (Y/n)?  "

      GetUserInput(temp_buf);

      if(ctrl_break) return(FALSE);  // a "NO"

      if(toupper(*temp_buf)!='Y' && toupper(*temp_buf)!='N'
           && *temp_buf > ' ')
      {
         MessageBeep(0);
      }

   } while(toupper(*temp_buf)!='Y' && toupper(*temp_buf)!='N'
           && *temp_buf > ' ');


   return(toupper(*temp_buf)!='N');

}



#pragma code_seg("CMDRexx_TEXT","CODE")


BOOL FAR PASCAL CMDRexx    (LPSTR lpCmd, LPSTR lpArgs)
{
BOOL rval;
LPSTR lp1;

   /** using the 'REXX' API run a REXX command! **/

   for(lp1=lpArgs; *lp1 && *lp1<=' '; lp1++)
      ; /* point to first non-space / control char */

   if(*lp1)
   {
      if(!(lp1 = EvalString(lp1)))
      {
         PrintString(pNOTENOUGHMEMORY);
         return(TRUE);
      }

      rval = RexxRun(lp1, TRUE);

      GlobalFreePtr(lp1);
   }
   else
   {
      rval = RexxRun("",TRUE);
   }

   return(rval);

   /* return(CMDNotYet(lpCmd, lpArgs)); */

}


#pragma code_seg("CMDRmdir_TEXT","CODE")


BOOL FAR PASCAL CMDRmdir   (LPSTR lpCmd, LPSTR lpArgs)
{
   lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   if(*lpArgs==0)  /* nothing, so just display all of the env vars! */
   {
      GlobalFreePtr(lpArgs);
      return(CMDBad(lpCmd, lpArgs));
   }

   lstrcpy(work_buf, lpArgs);
   GlobalFreePtr(lpArgs);

   if(*work_buf == '\"')  // quoted path?
   {
    LPSTR lp1 = work_buf;

      lstrcpy(lp1, lp1 + 1);
      while(*lp1 && *lp1 != '\"') lp1++;

      if(*lp1=='\"')
      {
         if(*lp1) *(lp1++) = 0;

         while(*lp1 && *lp1 <= ' ') lp1++;

         if(*lp1) // non-white-space is a syntax error...
         {
            PrintErrorMessage(868);  // "?Invalid 'long' file name specified\r\n"

            return(TRUE);
         }
      }
   }

   return(CMDErrorCheck(My_Rmdir(work_buf)));
}


#pragma code_seg("CMDSet_TEXT","CODE")


BOOL FAR PASCAL CMDSet     (LPSTR lpCmd, LPSTR lpArgs)
{
LPSTR lp1, lpEnv;
BOOL rval;


   lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   if(*lpArgs==0)  /* nothing, so just display all of the env vars! */
   {
      if(!(lpEnv = GetMyEnvironment())) return(TRUE);  /* (not found) */

      for(lp1=lpEnv; *lp1!=0; lp1+=lstrlen(lp1) + 1)
      {
         PrintString(lp1);           /* write env string to screen!! */
         PrintString(pCRLF);        /* <CR><LF> */
      }

      rval = FALSE;
   }
   else
   {
      for(lp1=lpArgs; *lp1!=0; lp1++)
      {
         if(*lp1=='=')
         {
            *(lp1++) = 0;

            rval = SetEnvString(lpArgs,lp1);

            GlobalFreePtr(lpArgs);
            return(rval);
         }
      }

      PrintErrorMessage(647);
      rval = TRUE;
   }

   GlobalFreePtr(lpArgs);
   return(rval);           /* if it gets here, there's an error! */

}


#pragma code_seg("CMDShare_TEXT","CODE")


BOOL FAR PASCAL CMDShare   (LPSTR lpCmd, LPSTR lpArgs)
{
   return(CMDNotYet(lpCmd, lpArgs));
}


#pragma code_seg("CMDShift_TEXT","CODE")


BOOL FAR PASCAL CMDShift   (LPSTR lpCmd, LPSTR lpArgs)
{

   if(BatchMode)
   {
      if(lpBatchInfo)
      {
         (lpBatchInfo->nShift)++;
         return(FALSE);
      }
      else
      {
         PrintErrorMessage(648);
         BatchMode = FALSE;
         DisplayPrompt();
         return(TRUE);
      }
   }
   else
   {
      PrintErrorMessage(596);
      return(TRUE);
   }

}


#pragma code_seg("CMDSort_TEXT","CODE")


BOOL FAR PASCAL CMDSort    (LPSTR lpCmd, LPSTR lpArgs)
{
BOOL rval;


   lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   rval = CMDRunProgram(lpCmd, lpArgs, SW_SHOWNORMAL);
   GlobalFreePtr(lpArgs);

   return(rval);
}


#pragma code_seg("CMDStart_TEXT","CODE")


BOOL FAR PASCAL CMDStart   (LPSTR lpCmd, LPSTR lpArgs)
{
BOOL rval;
LPSTR lp1, lp2;
WORD w1;
char c, temp_buf[MAX_PATH];
BOOL bMin=0, bMax=0, bNormal=0, bHidden=0, bWait=0;
int nCmdShow = SW_SHOWNORMAL;



   lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }



   for(lp1=lpArgs; *lp1 && *lp1 <= ' '; lp1++)
      ; // find first non-white-space

   while(*lp1 == '/')
   {
      lp1++;
      for(lp2=lp1; *lp2 > ' ' && *lp2 != '/'; lp2++)
        ; // find end of switch or '/'


      w1 = lp2 - lp1;

      if(!_fstrnicmp(lp1, "MINIMIZED", w1))
      {
         if(bMin) goto syntax_error;
         bMin = TRUE;
         nCmdShow = SW_SHOWMINNOACTIVE;
      }
      else if(!_fstrnicmp(lp1, "MAXIMIZED", w1))
      {
         if(bMax) goto syntax_error;
         bMax = TRUE;
         nCmdShow = SW_SHOWMAXIMIZED;
      }
      else if(!_fstrnicmp(lp1, "RESTORED", w1))
      {
         if(bNormal) goto syntax_error;
         bNormal = TRUE;
         nCmdShow = SW_SHOWNORMAL;
      }
      else if(!_fstrnicmp(lp1, "HIDDEN", w1))
      {
         if(bHidden) goto syntax_error;
         bHidden = TRUE;
         nCmdShow = SW_HIDE;
      }
      else if(!_fstrnicmp(lp1, "WAIT", w1))
      {
         if(bWait) goto syntax_error;
         bWait = TRUE;
      }
      else
      {
syntax_error:

         PrintString(pSYNTAXERROR);
         PrintString(lp1);
         PrintString(pQCRLFLF);

         GlobalFreePtr(lpArgs);
         return(TRUE);
      }


      lp1 = lp2;
      while(*lp1 && *lp1 <= ' ') lp1++;
   }

   if(*lp1)
   {
      IsCall = bWait;   // invokes 'WAIT' as required
      IsStart = TRUE;   // forces BATCH and REXX files to run in a separate
                        // 'SFTSHELL' window.

      // 'lp1' currently points to the program name.  Parse it for '"'

      if(*lp1 == '\"')
      {
         lp1++;
         lp2 = lp1;

         while(*lp2 && *lp2 != '\"') lp2++;

         if(*lp2) *(lp2++) = 0;
         lstrcpy(temp_buf, lp1);
      }
      else
      {
         lp2 = lp1;
         while(*lp2 && *lp2>' ' && *lp2!='/') lp2++;
                          /* find next white space, '/', or end of string */
         c = *lp2;
         *lp2 = 0;  /* temporary */
         lstrcpy(temp_buf, lp1);
         *lp2 = c;
      }


      while(*lp2 && *lp2<=' ') lp2++; /* find next non-space or end of string */

      rval = CMDRunProgram(temp_buf, lp2, nCmdShow);

      IsCall = FALSE;     // turn flag off... I'm done with it now!
      IsStart = FALSE;    // and the same goes with THIS flag

      GlobalFreePtr(lpArgs);
      return(rval);
   }
   else
   {
      PrintErrorMessage(919);  // "?Missing PROGRAM NAME in 'START'\r\n\n"

      GlobalFreePtr(lpArgs);
      return(FALSE);
   }

}


#pragma code_seg("CMDSubst_TEXT","CODE")


BOOL FAR PASCAL CMDSubst   (LPSTR lpCmd, LPSTR lpArgs)
{
LPSTR lpLOL, lp1;
PSTR np2;
DWORD dwDosVer;
HMODULE hWinFile;
TASKENTRY te;
LPCURDIR lpC, lpC2;
WORD w, wNDrives, count, wDrive;
//LPTDB lpTDB;
WNDENUMPROC lpProc;


   dwDosVer = GetVersion();  /* HIWORD == Dos Version */

   lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   _hmemset((LPSTR)&te, 0, sizeof(te));
   te.dwSize = sizeof(te);

   _asm
   {
      push es
      mov ax,0x5200
      int 0x21

      mov ax, 0
      mov dx, ax
      jc bad_news

      mov dx, es
      mov ax, bx

bad_news:
      mov WORD PTR lpLOL, ax
      mov WORD PTR lpLOL+2, dx

      pop es
   }


   if(!lpLOL)
   {
      PrintErrorMessage(605);

      GlobalFreePtr(lpArgs);
      return(TRUE);
   }

   if(HIWORD(dwDosVer)>=0x310)  /* DOS 3.1 or later */
   {
      wNDrives = *((BYTE FAR *)(lpLOL + 0x21));
   }
   else
   {
      wNDrives = *((BYTE FAR *)(lpLOL + 0x1b));
   }


   for(lp1=lpArgs; *lp1 && *lp1<=' '; lp1++)
      ;   /* find first non-space in command line */

   if(*lp1)  /* there is a command line! */
   {
      /* must be 2 args - 2nd one might be '/D' */
      /* first one must be 2 bytes long and represent a drive */

      _fstrupr(lp1);

      if(*lp1<'A' || *lp1>'Z' || lp1[1]!=':' || lp1[2]!=' ')
      {
         PrintErrorMessage(585);
         return(TRUE);
      }

      wDrive = *lp1 - 'A' + 1;

      lp1 += 3;

      while(*lp1 && *lp1<=' ')
         lp1++;                    /* find next non-space in command line */

      _fstrtrim(lp1);  /* get rid of trailing white space */

      if(!*lp1)  /* oh, no - only 1 argument!! */
      {
         PrintErrorMessage(649);

         GlobalFreePtr(lpArgs);
         return(TRUE);
      }
      else if(*lp1=='/' && lp1[1]=='D' && lp1[2]==0)   /* SUBST drive: /D */
      {
         /* here is where we process the '/D' command!! */

         lpC = (LPCURDIR)GetDosCurDirEntry(wDrive);

         if(!lpC || (lpC->wFlags & CURDIR_TYPE_MASK)==CURDIR_TYPE_INVALID)
         {
            PrintErrorMessage(650);
            PrintAChar(wDrive + 'A' - 1);
            PrintErrorMessage(651);

            GlobalFreePtr(lpArgs);
            return(TRUE);
         }

         if(!(lpC->wFlags & CURDIR_SUBST))
         {
            PrintErrorMessage(650);
            PrintAChar(wDrive + 'A' - 1);
            PrintErrorMessage(652);

            MyFreeSelector(SELECTOROF(lpC));

            GlobalFreePtr(lpArgs);
            return(TRUE);
         }

            /* here I check if drive is active using IsDriveInUse() */

         if(IsDriveInUse(wDrive))
         {
            PrintErrorMessage(654);
            PrintAChar(wDrive + 'A' - 1);
            PrintErrorMessage(655);

            MyFreeSelector(SELECTOROF(lpC));


            GlobalFreePtr(lpArgs);
            return(TRUE);
         }

         // FOR CHICAGO, tell the world that I'm getting rid of the drive

         if(IsChicago || IsNT)
         {
          DEV_BROADCAST_VOLUME dbv;

            dbv.dbcv_size       = sizeof(dbv);
            dbv.dbcv_devicetype = DBT_DEVTYP_VOLUME;
            dbv.dbcv_reserved   = 0;
//            dbv.dbcv_unitmask   = wDrive - 1;          // guess... (?)
            dbv.dbcv_unitmask   = 1L << (wDrive - 1);
            dbv.dbcv_flags      = DBTF_NET; // DBTF_MEDIA;

            PostMessage(HWND_BROADCAST, WM_DEVICECHANGE,
                        DBT_DEVICEREMOVEPENDING,
//                        (LPARAM)(LPSTR)&dbv);
                        (LPARAM)(MyGetSelectorBase(SELECTOROF((LPSTR)&dbv))
                                 + OFFSETOF((LPSTR)&dbv)));

            MthreadSleep(50);  // wait for a short time for apps to respond...
         }


         PrintErrorMessage(654);
         PrintAChar(wDrive + 'A' - 1);
         PrintErrorMessage(656);


         lpC->wFlags = 0;  /* it's no longer valid now */
         lpC->pathname[0] = (char)(wDrive + 'A' - 1);  /* the drive letter desig. */
         lpC->pathname[1] = ':';
         lpC->pathname[2] = '\\';
         lpC->pathname[3] = 0;

         lpC->wPathRootOffs = 2;  /* normal for root dir! */

         lpC->dwDPB          = 0;
         lpC->wDirStartClust = 0xffff;
         lpC->wNetPtrHi      = 0xffff;
         lpC->wInt21_5F03    = 0;

         if(HIWORD(dwDosVer)>=0x400)
         {
            lpC->wDOS4Byte   = 0;
            lpC->dwIFSPtr    = 0;
            lpC->wDOS4Word2  = 0;
         }

         MyFreeSelector(SELECTOROF(lpC));

         /** Drive has been deleted.  Tell File Manager, P&P stuff ! **/

         if(IsChicago || IsNT)
         {
          DEV_BROADCAST_VOLUME dbv;

            dbv.dbcv_size       = sizeof(dbv);
            dbv.dbcv_devicetype = DBT_DEVTYP_VOLUME;
            dbv.dbcv_reserved   = 0;
//            dbv.dbcv_unitmask   = wDrive - 1;          // guess... (?)
            dbv.dbcv_unitmask   = 1L << (wDrive - 1);
            dbv.dbcv_flags      = DBTF_NET; // DBTF_MEDIA;

            PostMessage(HWND_BROADCAST, WM_DEVICECHANGE,
                        DBT_DEVICEREMOVECOMPLETE,
//                        (LPARAM)(LPSTR)&dbv);
                        (LPARAM)(MyGetSelectorBase(SELECTOROF((LPSTR)&dbv))
                                 + OFFSETOF((LPSTR)&dbv)));
         }

         hWinFile = GetModuleHandle("WINFILE");
         if(hWinFile)
         {
            (FARPROC &)lpProc = MakeProcInstance((FARPROC)CMDSubstEnum,hInst);
            if(!lpTaskFirst(&te))
            {
               PrintErrorMessage(657);

               GlobalFreePtr(lpArgs);
               return(TRUE);
            }

            do
            {
               if(te.hModule == hWinFile)
               {
                  if(lpProc) EnumWindows(lpProc, (LPARAM)te.hTask);
               }
            } while(lpTaskNext(&te) && te.hTask);

            FreeProcInstance((FARPROC)lpProc);
         }

/*         SendMessage(HWND_BROADCAST, WM_FILESYSCHANGE, 0, 0); */


         GlobalFreePtr(lpArgs);
         return(FALSE);
      }
      else   /* 2nd arg must be a valid path */
      {

          /* first step:  make sure 'new' drive is *NOT* in use! */

         lpC = (LPCURDIR)GetDosCurDirEntry(wDrive);

         if((lpC->wFlags & CURDIR_TYPE_MASK)!=CURDIR_TYPE_INVALID)
         {
            PrintErrorMessage(650);
            PrintAChar(wDrive + 'A' - 1);
            PrintErrorMessage(658);

            MyFreeSelector(SELECTOROF(lpC));


            GlobalFreePtr(lpArgs);
            return(TRUE);
         }

          /* next step:  verify new path is 'VALID' */

         DosQualifyPath(work_buf, lp1);  /* get 'qualified' path */

         if(IsChicago || IsNT)
         {
            GetShortName(work_buf, work_buf);  // convert to 'short' name
         }

         np2 = work_buf + sizeof(work_buf) / 2;

         if(!*work_buf ||
            !_lgetdcwd(*work_buf - 'A' + 1, np2, sizeof(work_buf) / 2)
            || MyChDir(work_buf))  /* get cur dir of this drive & test path */
         {
            PrintString(pINVALIDPATH);
            PrintString(lp1);
            PrintString(pQCRLFLF);

            GlobalFreePtr(lpArgs);
            return(TRUE);
         }

         MyChDir(np2);       /* restore old directory */
                       /* I *MUST* assume that this worked! */

          /* next step:  create the drive entry!! */

         if(!(lpC2 = (LPCURDIR)GetDosCurDirEntry(*work_buf - 'A' + 1)))
         {
            MyFreeSelector(SELECTOROF(lpC));
            PrintErrorMessage(659);

            GlobalFreePtr(lpArgs);
            return(TRUE);
         }

         lpC->dwDPB = lpC2->dwDPB;
         if((lpC2->wFlags & CURDIR_TYPE_MASK)==CURDIR_TYPE_NETWORK)
         {
            lpC->wDirStartClust = lpC2->wDirStartClust;
            lpC->wNetPtrHi      = lpC2->wNetPtrHi;
            lpC->wInt21_5F03    = lpC2->wInt21_5F03;
         }
         else
         {
            lpC->wDirStartClust = 0xffff;  /* indicates 'not obtained yet' */
            lpC->wNetPtrHi      = 0xffff;  /* indicates 'unused' really */
            lpC->wInt21_5F03    = 0;
         }


         lstrcpy(lpC->pathname, work_buf);  /* build the correct path name */

         w = lstrlen(work_buf) - 1;
         if(work_buf[w]!='\\') w++;  /* if not '\' then point just past end */

         lpC->wPathRootOffs = w;       /* either points to '\' or NULL byte */

         if(HIWORD(dwDosVer)>=0x400)
         {
            lpC->wDOS4Byte  = lpC2->wDOS4Byte;
            lpC->dwIFSPtr   = lpC2->dwIFSPtr;
            lpC->wDOS4Word2 = lpC2->wDOS4Word2;
         }

         lpC->wFlags = lpC2->wFlags | CURDIR_SUBST; /* add 'subst' flag */

             /* this section copied from below */

         PrintAChar(wDrive + 'A' - 1);
         PrintErrorMessage(756);            // ": => "

         _fmemcpy(work_buf, lpC->pathname, lpC->wPathRootOffs);
         if(lpC->wPathRootOffs<=2)
         {
            work_buf[lpC->wPathRootOffs] = '\\';
            work_buf[lpC->wPathRootOffs+1] = 0;
         }
         else
         {
            work_buf[lpC->wPathRootOffs] = 0;
         }

         PrintString(work_buf);
         PrintString(pCRLF);

         MyFreeSelector(SELECTOROF(lpC));
         MyFreeSelector(SELECTOROF(lpC2));


         /** Drive has been added.  Tell File Manager / P&P stuff! **/

         if(IsChicago || IsNT)
         {
          DEV_BROADCAST_VOLUME dbv;

            dbv.dbcv_size       = sizeof(dbv);
            dbv.dbcv_devicetype = DBT_DEVTYP_VOLUME;
            dbv.dbcv_reserved   = 0;
//            dbv.dbcv_unitmask   = wDrive - 1;          // guess... (?)
            dbv.dbcv_unitmask   = 1L << (wDrive - 1);
            dbv.dbcv_flags      = DBTF_NET; // DBTF_MEDIA;

            PostMessage(HWND_BROADCAST, WM_DEVICECHANGE,
                        DBT_DEVICEARRIVAL,
//                        (LPARAM)(LPSTR)&dbv);
                        (LPARAM)(MyGetSelectorBase(SELECTOROF((LPSTR)&dbv))
                                 + OFFSETOF((LPSTR)&dbv)));
         }

         hWinFile = GetModuleHandle("WINFILE");
         if(hWinFile)
         {
            (FARPROC &)lpProc = MakeProcInstance((FARPROC)CMDSubstEnum,hInst);
            if(!lpTaskFirst(&te))
            {
               PrintErrorMessage(657);

               GlobalFreePtr(lpArgs);
               return(TRUE);
            }

            do
            {
               if(te.hModule == hWinFile)
               {
                  if(lpProc) EnumWindows(lpProc, (LPARAM)te.hTask);
               }
            } while(lpTaskNext(&te) && te.hTask);

            FreeProcInstance((FARPROC)lpProc);
         }

/*         SendMessage(HWND_BROADCAST, WM_FILESYSCHANGE, 0, 0); */


         GlobalFreePtr(lpArgs);
         return(FALSE);
      }
   }

   for(count=0, w=1; w<=wNDrives; w++)
   {
      lpC = (LPCURDIR)GetDosCurDirEntry(w);

      if(!lpC) continue;  /* ignore NULL returns */

      if((lpC->wFlags & CURDIR_TYPE_MASK)==CURDIR_TYPE_INVALID) continue;

      if(lpC->wFlags & CURDIR_SUBST)
      {
         /* this is a SUBST'ed drive! */

         PrintAChar(w + 'A' - 1);
         PrintErrorMessage(756);            // ": => "

         _fmemcpy(work_buf, lpC->pathname, lpC->wPathRootOffs);
         if(lpC->wPathRootOffs<=2)
         {
            work_buf[lpC->wPathRootOffs] = '\\';
            work_buf[lpC->wPathRootOffs+1] = 0;
         }
         else
         {
            work_buf[lpC->wPathRootOffs] = 0;
         }

         PrintString(work_buf);
         PrintString(pCRLF);
         count++;
      }

      MyFreeSelector(SELECTOROF(lpC));  /* don't forget to do this! */
   }

   if(!count)
   {
      PrintErrorMessage(660);
   }


   GlobalFreePtr(lpArgs);
   return(FALSE);
}


   /* this proc (below) attempts to force an 'F5' (refresh) on WINFILE */

BOOL EXPORT CALLBACK CMDSubstEnum(HWND hWnd, LPARAM lParam)
{
   if(GetWindowTask(hWnd)==(HTASK)lParam /* && IsWindowVisible(hWnd) */)
   {
     /*  GetWindowText(hWnd, work_buf, sizeof(work_buf)); */
      if(/* *work_buf && */ !GetWindowWord(hWnd, GWW_HWNDPARENT))
      {
         PostMessage(hWnd, WM_KEYDOWN, (WPARAM)0x74, (LPARAM)0x3f0001L);
         PostMessage(hWnd, WM_KEYUP, (WPARAM)0x74, (LPARAM)0xc03f0001L);
      }
   }
   return(TRUE);

}




#pragma code_seg("CMDSys_TEXT","CODE")


BOOL FAR PASCAL CMDSys     (LPSTR lpCmd, LPSTR lpArgs)
{
LPSTR lp1, lp2, lp3;
WORD i;
char temp_buf[80];
char pDestSyntaxError[]="?Syntax error in destination drive specification\r\n\n";



   lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   lstrcpy(temp_buf, lpArgs);  /* first, make a copy of it! */
   _fstrupr(temp_buf);         /* convert to ALL upper case! */

   GlobalFreePtr(lpArgs);

   for(lp1=temp_buf; *lp1 && *lp1<=' '; lp1++)
     ; /* find first non-space */


   for(lp2=lp1; *lp2>' '; lp2++)
      ;  /* find next space */

   if(*lp2) *(lp2++)=0;

   while(*lp2 && *lp2<=' ') lp2++;  /* find next 'non-space' */

   for(lp3 = lp2; *lp3>' '; lp3++)
      ;  /* find next space */

   if(*lp3) *(lp3++)=0;

   /* see if the command line is BOGUS (too many args) */

   while(*lp3 && *lp3<=' ') lp3++;  /* find next 'non-space' */

   if(*lp3)
   {
      PrintErrorMessage(661);
      return(TRUE);
   }


   /* here lp1 points to the 1st arg, and lp2 points to the 2nd. */



   if(*lp2)   /* path for 'SYS' drive was found - use it! */
   {
      if(*lp2<'A' || *lp2>'Z' || lp2[1]!=':' || lp2[2]!=0)
      {
         PrintString(pDestSyntaxError);
         return(TRUE);
      }

      return(CMDSysTransferSub(lp1, lp2, TRUE));
   }
   else
   {
      if(*lp1<'A' || *lp1>'Z' || lp1[1]!=':' || lp1[2]!=0)
      {
         PrintString(pDestSyntaxError);
         return(TRUE);
      }

      _getcwd(work_buf, sizeof(work_buf));


      if(!CMDSysTransferSub(work_buf, lp1, FALSE)) return(FALSE);

      work_buf[3] = 0;  /* guaranteed "D:\" in 1st 3 bytes */


      if(!CMDSysTransferSub(work_buf, lp1, FALSE)) return(FALSE);

      work_buf[3] = 0;  /* guaranteed "D:\" in 1st 3 bytes */


      /** next, if I'm DOS 4.0+ find out 'boot drive' **/

      if(HIWORD(GetVersion())>=0x400)
      {

         _asm
         {
            mov ax, 0x3305      /* get BOOT DRIVE */
            int 0x21

            mov dh, 0
            mov i, dx

         }

         work_buf[0] = (char)('A' + i - 1);

         return(CMDSysTransferSub(work_buf, lp1, TRUE));
      }
      else
      {
         lp3 = GetEnvString(pCOMSPEC);
         if(lp3)
         {
            *work_buf = *lp3;
            return(CMDSysTransferSub(work_buf, lp1, TRUE));
         }
      }

      PrintString(pNoSysMessage);
      return(TRUE);
   }

}


BOOL FAR PASCAL CMDSysTransferSub(LPSTR lp1, LPSTR lp2, BOOL ErrMsg)
{
LPSTR lp3;
WORD i;
BOOL rval;
OFSTRUCT ofstr;
static char pDest[]="A:\\????????.???";

   i = lstrlen(lp1);   /* length of 'source' path */

   lstrcpy(work_buf, lp1);
   if(work_buf[i - 1]!='\\')
   {
      work_buf[i++] = '\\';
      work_buf[i] = 0;
   }

   /* 'i' is the index within work_buf of the 'end of path' */

   lstrcpy(work_buf + i, pIOSYS);

   if(MyOpenFile(work_buf, &ofstr,
               OF_READ | OF_SHARE_DENY_NONE | OF_EXIST)!=HFILE_ERROR)
   {
      lstrcpy(work_buf + i, pMSDOSSYS);

      if(MyOpenFile(work_buf, &ofstr,
               OF_READ | OF_SHARE_DENY_NONE | OF_EXIST)!=HFILE_ERROR)
      {
         pDest[0] = toupper(*lp2);

         if(MyOpenFile(work_buf, &ofstr,
                OF_READ | OF_SHARE_DENY_NONE | OF_EXIST)!=HFILE_ERROR)
         {
            lp3 = NULL; /* this says 'use COMMAND.COM from specified dir' */
         }
         else if(ErrMsg)
         {
            lp3 = GetEnvString(pCOMSPEC);

            if(lp3)
            {
               if(_fstrchr(lp3, '?') || _fstrchr(lp3, '*'))
               {
                  PrintErrorMessage(662);
                  return(TRUE);
               }
            }
            else
            {
               PrintErrorMessage(663);
               return(TRUE);
            }
         }
         else
            return(TRUE);


         lstrcpy(work_buf + i, pIOSYS);
         rval = SubmitFilesToCopy(work_buf, pDest, FALSE, TRUE, TRUE, TRUE);

         lstrcpy(work_buf + i, pMSDOSSYS);
         rval |= SubmitFilesToCopy(work_buf, pDest, FALSE, TRUE, TRUE, TRUE);

         if(!lp3)
         {
            lstrcpy(work_buf + i, pCOMMANDCOM);
            rval |= SubmitFilesToCopy(work_buf, pDest, FALSE, TRUE, TRUE, TRUE);
         }
         else
         {
            rval |= SubmitFilesToCopy(lp3, pDest, FALSE, TRUE, TRUE, TRUE);
         }

         if(!rval)
         {
            ReportCopyResults();
         }
         else
         {
            if(TotalCopiesSubmitted) ReportCopyResults();
            PrintErrorMessage(664);
         }

         return(FALSE);

      }
   }


   if(ErrMsg)
   {
      PrintString(pNoSysMessage);
   }

   return(TRUE);  /* this says 'I have not found the system files' */

}





#pragma code_seg("CMDTasklist_TEXT","CODE")


volatile HTASK TaskListEnumTask = NULL;
volatile WORD TaskListEnumCount = 0;

BOOL FAR PASCAL CMDTasklist(LPSTR lpCmd, LPSTR lpArgs)
{
LPPARSE_INFO lpParseInfo;
WNDENUMPROC lpProc;
TASKENTRY te;


   /* TA-DAAAA!!!!  This one's gonna work now, in 3.0 and 3.1 mode!!! */

   _fmemset((LPSTR)&te, 0, sizeof(te));

   te.dwSize = sizeof(te);

   if(!lpTaskFirst || !lpTaskNext)
   {
      PrintErrorMessage(665);
      return(TRUE);
   }
   else if(!lpTaskFirst((LPTASKENTRY)&te))
   {
      PrintErrorMessage(667);
      return(TRUE);
   }


   lpParseInfo = CMDLineParse(lpArgs);
   if(lpParseInfo->n_switches || lpParseInfo->n_parameters)
   {
      PrintErrorMessage(619);
   }

   ReDirectOutput(lpParseInfo,TRUE);


   PrintErrorMessage(838);  // "   TASK      MODULE    INSTANCE    MODULE    MAIN WINDOW\r\n"
   PrintErrorMessage(839);  // "  HANDLE      NAME      HANDLE     HANDLE      CAPTION  \r\n"
   PrintErrorMessage(840);  // " --------   --------   --------   --------   -----------\r\n"

   (FARPROC &)lpProc = MakeProcInstance((FARPROC)CMDTaskListEnum,hInst);

   do
   {
    static const char CODE_BASED pFmt[]=
                               "   %04xH    %-8.8s     %04xH      %04xH    ";

      if(lpModuleFindHandle)
      {
       MODULEENTRY me;

         _fmemset((LPVOID)&me, 0, sizeof(me));

         me.dwSize = sizeof(me);

         if(lpModuleFindHandle(&me, te.hModule))
         {
            _fmemcpy(te.szModule, me.szModule, sizeof(te.szModule));
         }
      }

      LockCode();
      wsprintf(work_buf, pFmt,
               (WORD)te.hTask, (LPSTR)te.szModule,
               (WORD)te.hInst, (WORD)te.hModule);
      UnlockCode();

      PrintString(work_buf);

      TaskListEnumCount = 0;
      TaskListEnumTask  = te.hTask;

      if(lpProc) EnumWindows(lpProc, (LPARAM)NULL);

      if(!TaskListEnumCount)
      {
         if(!lpProc)  PrintErrorMessage(841); // "** INTERNAL ERROR **\r\n"
         else         PrintErrorMessage(842); // "** N/A **\r\n"
      }

   } while(lpTaskNext((LPTASKENTRY)&te));

   if(lpProc) FreeProcInstance((FARPROC)lpProc);

   PrintString(pCRLF);

   if(lpParseInfo)
   {
      ReDirectOutput(lpParseInfo,FALSE);
      GlobalFreePtr(lpParseInfo);
   }

   return(FALSE);

}


BOOL EXPORT CALLBACK CMDTaskListEnum(HWND hWnd, LPARAM lParam)
{
   if(GetWindowTask(hWnd)==TaskListEnumTask && IsWindowVisible(hWnd))
   {
      GetWindowText(hWnd, work_buf, sizeof(work_buf));
      if(*work_buf)
      {
         PrintErrorMessage(799);      // "\r\033[45C"
         PrintString(work_buf);
         PrintString(pCRLF);
         TaskListEnumCount++;
      }
   }
   return(TRUE);

}




#pragma code_seg("CMDTime_TEXT","CODE")


BOOL FAR PASCAL CMDTime    (LPSTR lpCmd, LPSTR lpArgs)
{
LPSTR lpT, lp1;
SFTTIME t;
int rval;
//unsigned char hour, minute, second, tick;
DWORD dwDosMem;
WORD  wDosSeg;
REALMODECALL rmc;


   lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);  // error - not enough memory
   }

   if(*lpArgs)
   {
      lpT = lpArgs;
      while(*lpT==' ') lpT++;

      lstrcpy(work_buf, lpT);
      _fstrtrim(work_buf);

      if(atotime(work_buf, (LPSFTTIME)&t))
      {
         PrintErrorMessage(668);
         PrintString(lpT);
         PrintString(pQCRLFLF);

         GlobalFreePtr(lpArgs);
         return(FALSE);
      }

      _fmemset((LPSTR)&rmc, 0, sizeof(rmc));  /* zero everything first. */

      dwDosMem = GlobalDosAlloc(512);        /* length of DOS stack area */
      wDosSeg = HIWORD(dwDosMem);


      rmc.DS  = wDosSeg;
      rmc.ES  = wDosSeg;
      rmc.SS  = wDosSeg;
      rmc.EAX = 0x2d00;                          /* int 21H function 2dH */
      rmc.SP  = 510;                             /* new stack pointer */

      rmc.ECX = (((DWORD)t.hour)<<8)   + t.minute;
      rmc.EDX = (((DWORD)t.second)<<8) + t.tick;


               /** USE DPMI TO MAKE 'REAL MODE' DOS CALL **/

      _asm
      {
         push es
         mov ax, 0x0300  /* DPMI function - SIMULATE REAL MODE INTERRUPT */
         mov bl, 0x21    /* interrupt number */
         mov bh, 0       /* interrupt ctrlr/A20 line reset */
         mov cx, 0       /* copy 0 words to real mode stack */
         push ss
         pop es
         lea di, WORD PTR rmc /* address of real mode call structure */
                              /* must use LEA because of [BP-xx] address */

         int 0x31        /* DPMI CALL! */

         pop es
      }

      rval = (unsigned char)rmc.EAX;  /* get value of error code */

      if(rval)
      {
         PrintErrorMessage(669);
         PrintString(lpT);
         PrintString(pQCRLFLF);
      }
      else
      {
         SendMessage((HWND)0xffff, WM_TIMECHANGE, NULL, NULL);
      }

      GlobalDosFree(LOWORD(dwDosMem));


      GlobalFreePtr(lpArgs);
      return(FALSE);

   }

   GlobalFreePtr(lpArgs);

//   _asm
//   {
//      mov ax,0x2c00             /* get current time */
//      int 0x21
//
//      mov BYTE PTR hour, ch
//      mov BYTE PTR minute, cl
//      mov BYTE PTR second, dh
//      mov BYTE PTR tick, dl
//   }
//
//
//   wsprintf(work_buf, "CURRENT TIME:  %02d:%02d:%02d.%02d\r\n\n",
//            hour, minute, second, tick);
//
//   PrintString(work_buf);
//

   GetSystemDateTime(NULL, &t);

   PrintErrorMessage(844);  // "CURRENT TIME: "

   PrintString((lp1 = TimeStr(&t))?lp1:(LPSTR)pNOTENOUGHMEMORY);

   if(lp1)
   {
      GlobalFreePtr(lp1);

      PrintString(pCRLFLF);
   }
   else
   {
      return(TRUE);
   }

                 /** next section asks for new time **/

   PrintErrorMessage(670);

   GetUserInput(work_buf);
   _fstrtrim(work_buf);

   lp1 = work_buf;

   while(*lp1 && *lp1<=' ') lp1++;

   if(*lp1) return(CMDTime(lpCmd, lp1));
   else     return(FALSE);  /* done */
}


#pragma code_seg("CMDTree_TEXT","CODE")


BOOL FAR PASCAL CMDTree    (LPSTR lpCmd, LPSTR lpArgs)
{
LPTREE lpTree, lpT;
WORD i, j, wStart, wNCol=0, wRow;
volatile WORD n_switches;
BOOL first_time=TRUE, newline=TRUE;
WORD wTreeLevel[128];
DWORD dwTickCount;
HCURSOR hOldCursor;
LPPARSE_INFO lpParseInfo;


    /* 179 ณ  180 ด  191 ฟ  192 ภ  193 ม  194 ย  195 ร  196 ฤ */

                  /* 197 ล  217 ู  218 ฺ  */


   _fmemset((LPSTR)wTreeLevel, 0, sizeof(wTreeLevel));  /* init array */

   hOldCursor = MySetCursor(LoadCursor(NULL, IDC_WAIT));  /* hourglass! */

   lpParseInfo = CMDLineParse(lpArgs);

   if(n_switches = lpParseInfo->n_switches)
   {
      PrintErrorMessage(671);
   }

   if(lpParseInfo->n_parameters)
   {
      lstrcpy(work_buf, (LPSTR)(lpParseInfo->sbpItemArray[n_switches]));
   }
   else
   {
      getcwd(work_buf, sizeof(work_buf));
   }

   if(!(lpTree = GetDirTree(work_buf)))
   {
      if(ctrl_break)
      {
         PrintString(pCTRLC);
         PrintString(pCRLFLF);

         if(!BatchMode) ctrl_break = FALSE;  // eat ctrl break if not BATCH
      }
      else
      {
         PrintErrorMessage(576);
      }

      MySetCursor(hOldCursor);
      GlobalFreePtr(lpParseInfo);
      return(TRUE);                               /* error - error - error */
   }

               /** HERE WE GO!! **/

   ReDirectOutput(lpParseInfo,TRUE);

   if(!(*lpTree->szDir))  /* empty tree list? */
   {
      PrintString(work_buf);
      PrintString("ฤฤ * EMPTY TREE *\r\n\n");
   }

   wStart = lstrlen(work_buf) + 2;

   dwTickCount = GetTickCount();


   for(lpT=lpTree, wRow = 0; *lpT->szDir; wRow++)
   {
      if(first_time)
      {
         first_time = FALSE;
         newline = FALSE;         /* next one will be on same line... */

         PrintString(work_buf);   /* first time through print path name */

         if(lpTree->wSibling)
         {
            PrintString("ฤฤยฤ");
         }
         else
         {
            PrintString("ฤฤฤฤ");
         }
      }
      else if(newline)
      {
         newline = FALSE;

         for(i=0; i<wStart; i++)
         {
            work_buf[i] = ' ';
         }
         work_buf[i] = 0;

         PrintString(work_buf);

         for(i=0; i<=wNCol; i++)
         {
            if(lpTree[wTreeLevel[i]].wSibling)  /* 1st column, sibling present */
            {
               if(i==wNCol)
                  PrintString("รฤ");
               else
                  PrintString("ณ ");
            }
            else if(i==wNCol)
            {
               PrintString("ภฤ");
            }
            else
            {
               PrintString("  ");
            }

            if(i<wNCol)
            {
               PrintString("            ");  /* 12 spaces */
            }
         }
      }
      else             /* normal entry on same line as previous */
      {
         if(lpT->wSibling)  /* there are siblings */
         {
            PrintString("ยฤ");
         }
         else
         {
            PrintString("ฤฤ");  /* no siblings - just lead in with line */
         }
      }


      j = lstrlen(lpT->szDir);  /* length of directory */

      PrintString(lpT->szDir);  /* display the directory */

      if(lpT->wChild)            /* there is a child entry! */
      {
         wTreeLevel[++wNCol] = lpT->wChild;

         lpT = lpTree + lpT->wChild;

         for(i=j; i<12; i++)  /* fill remainder of 12 spaces in column */
         {
            PrintAChar('ฤ');
         }

      }
      else if(lpT->wSibling)
      {
         wTreeLevel[wNCol] = lpT->wSibling;
         lpT = lpTree + lpT->wSibling;

         PrintString(pCRLF);     /* a 'newline' */
         newline = TRUE;
      }
      else
      {

         PrintString(pCRLF);     /* a 'newline' again */
         newline = TRUE;

          /* go back to the previous level(s) until valid entry found */

         while(!lpT->wSibling && wNCol)
         {
              /* if 'wNCol' is non-zero, we're "out on a limb" (gag!) */
              /* I hope Shirley MacLane doesn't see this code.  But,  */
              /* if Gary Larson does he'll write a nice comic for it. */

            wTreeLevel[wNCol--] = 0;
            lpT = lpTree + wTreeLevel[wNCol];
         }

         if(!lpT->wSibling) break;  /* bail out now */

         wTreeLevel[wNCol] = lpT->wSibling;  /* the sibling is the next one */
         lpT = lpTree + lpT->wSibling;
      }


      if(newline && (GetTickCount() - dwTickCount) > 50)  // only if new line...
      {
         // PER-LOOP YIELD (outer loop)

         if(LoopDispatch()) break;

         while(stall_output && !ctrl_break)
         {
            MthreadSleep(50);
            if(!ctrl_break) ctrl_break = LoopDispatch();
         }

         if(ctrl_break)
         {
            stall_output = FALSE;
            break;
         }

         dwTickCount = GetTickCount();
      }
   }

   if(ctrl_break)
   {
      PrintString(pCTRLC);
      if(!BatchMode) ctrl_break = FALSE;  // eat ctrl break if not BATCH
   }

   PrintString(pCRLFLF);

   if(lpTree) GlobalFreePtr(lpTree);

   if(lpParseInfo)
   {
      ReDirectOutput(lpParseInfo,FALSE);
      GlobalFreePtr(lpParseInfo);
   }

   MySetCursor(hOldCursor);

   return(FALSE);

}



#pragma code_seg("CMDTruename_TEXT","CODE")


BOOL FAR PASCAL CMDTruename(LPSTR lpCmd, LPSTR lpArgs)
{
char buf[256];

   lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   if(*lpArgs)
   {
      if(*lpArgs == '\"')  // quoted path?
      {
       LPSTR lp1 = lpArgs;

         lstrcpy(lp1, lp1 + 1);
         while(*lp1 && *lp1 != '\"') lp1++;

         if(*lp1=='\"')
         {
            if(*lp1) *(lp1++) = 0;

            while(*lp1 && *lp1 <= ' ') lp1++;

            if(*lp1) // non-white-space is a syntax error...
            {
               PrintErrorMessage(868);  // "?Invalid 'long' file name specified\r\n"

               return(TRUE);
            }
         }
      }

      DosQualifyPath((LPSTR)buf, lpArgs);
   }
   else
   {
      DosQualifyPath((LPSTR)buf, ".");
   }

   if(!*buf)
   {
      PrintString(pINVALIDPATH);
      PrintString(lpArgs);
      PrintString(pQCRLFLF);
   }
   else
   {
      PrintString((LPSTR)buf);
      PrintString(pCRLFLF);
   }

   GlobalFreePtr(lpArgs);
   return(FALSE);

}



#pragma code_seg("CMDType_TEXT","CODE")


BOOL FAR PASCAL CMDType    (LPSTR lpCmd, LPSTR lpArgs)
{
LPPARSE_INFO lpParseInfo;
HFILE hFile;
OFSTRUCT ofstr;
WORD uret;
LPSTR lp1;
char c;


   lpParseInfo = CMDLineParse(lpArgs);

   if(!lpParseInfo || CMDCheckParseInfo(lpParseInfo, NULL, 1 | CMD_ATMOST))
   {
      if(lpParseInfo) GlobalFreePtr(lpParseInfo);
      return(TRUE);
   }

   if(!lpParseInfo->sbpInFile && !lpParseInfo->n_parameters)
   {
      PrintString("?Missing input file in 'TYPE'\r\n\n");
      GlobalFreePtr(lpParseInfo);
      return(TRUE);
   }
   else if(lpParseInfo->sbpInFile)
   {
      if(lpParseInfo->n_parameters)
      {
         PrintString("?Warning - file name \"");
         PrintString(lpParseInfo->sbpItemArray[lpParseInfo->n_switches]);
         PrintString("\" ignored\r\n\n");
      }

      lp1 = lpParseInfo->sbpInFile;
   }
   else
   {
      lp1 = lpParseInfo->sbpItemArray[lpParseInfo->n_switches];
   }



   hFile = MyOpenFile(lp1, (LPOFSTRUCT)&ofstr, OF_READ | OF_SHARE_DENY_NONE);


   if(hFile==(HFILE)-1)       /* an error! */
   {
      GlobalFreePtr(lpParseInfo);

      return(CMDErrorCheck(TRUE));
   }

   if(IsCONDevice(hFile))      /* are we typing the 'CON' device? */
   {
      PrintErrorMessage(672); // "\r\nYou CANNOT 'TYPE' the 'CON' device!!\r\n\n"

      GlobalFreePtr(lpParseInfo);
      return(TRUE);
   }


   // output re-direction from TYPE (allows piping)

   ReDirectOutput(lpParseInfo,TRUE);


   while((uret = _lread(hFile, work_buf, sizeof(work_buf)))>0
         && uret!=(WORD)-1)
   {
      PrintBuffer(work_buf, uret);

      UpdateWindow(hMainWnd);       /* every buffer update the window */

      if(LoopDispatch()) ctrl_break = TRUE;

      while(!ctrl_break && stall_output)
      {
         MthreadSleep(50);

         if(!ctrl_break) ctrl_break = LoopDispatch();
      }

      if(ctrl_break)
      {
         PrintAChar('^');
         PrintAChar('C');

         stall_output = FALSE;

         if(!BatchMode) ctrl_break = FALSE;

         uret = 0;     /* forces no error message on exit */
         break;
      }

   }

   if(redirect_output == HFILE_ERROR) // output to the screen?
   {
      PrintString(pCRLFLF);  // extra <CR><LF><LF> at end of file
   }

   _lclose(hFile);


   ReDirectOutput(lpParseInfo,FALSE);

   GlobalFreePtr(lpParseInfo);
   return(CMDErrorCheck(uret));

}


#pragma code_seg("CMDUndelete_TEXT","CODE")


BOOL FAR PASCAL CMDUndelete(LPSTR lpCmd, LPSTR lpArgs)
{
BOOL rval;


   lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   rval = CMDRunProgram(lpCmd, lpArgs, SW_SHOWNORMAL);
   GlobalFreePtr(lpArgs);

   return(rval);
}



#pragma code_seg("CMDVer_TEXT","CODE")


BOOL FAR PASCAL CMDVer     (LPSTR lpCmd, LPSTR lpArgs)
{
union {
   DWORD dw;
   struct {
      BYTE win_major,win_minor;
      BYTE dos_minor,dos_major;
      } ver;
   } uRval;


   if(*lpArgs!=0)  return(CMDBad(lpCmd, lpArgs));

   PrintString(pVersionString);

   uRval.dw = GetVersion();

   {
    static const char CODE_BASED pFmt[]=
       "MS-DOS Version %d.%02d, Microsoft(r) Windows (tm) Version %d.%02d\r\n\n";

      LockCode();
      wsprintf(work_buf, pFmt,
               uRval.ver.dos_major, uRval.ver.dos_minor,
               uRval.ver.win_major, uRval.ver.win_minor);
      UnlockCode();
   }

   PrintString(work_buf);

   return(FALSE);

}


#pragma code_seg("CMDVerify_TEXT","CODE")

BOOL FAR PASCAL CMDVerify  (LPSTR lpCmd, LPSTR lpArgs)
{
WORD flag;
static char pVERIFYIS[]="VERIFY is ";


   lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   if(!*lpArgs)
   {
      _asm
      {
         mov ax, 0x5400
         int 0x21
         mov ah, 0
         mov flag, ax
      }

      PrintString(pVERIFYIS);
      PrintString(flag?pON:pOFF);
      PrintString(pCRLFLF);
   }
   else if(!_fstricmp(lpArgs, pON))
   {
      _asm
      {
         mov ax, 0x2e01
         mov dl, 0
         int 0x21

      }

      PrintString(pVERIFYIS);
      PrintString(pON);
      PrintString(pCRLFLF);
   }
   else if(!_fstricmp(lpArgs, pOFF))
   {
      _asm
      {
         mov ax, 0x2e00
         mov dl, 0
         int 0x21

      }

      PrintString(pVERIFYIS);
      PrintString(pOFF);
      PrintString(pCRLFLF);
   }
   else
   {
      PrintString(pILLEGALARGUMENT);
      PrintString(lpArgs);
      PrintString(pQCRLFLF);

      GlobalFreePtr(lpArgs);
      return(TRUE);
   }

   GlobalFreePtr(lpArgs);
   return(FALSE);
}


#pragma code_seg("CMDVol_TEXT","CODE")


BOOL FAR PASCAL CMDVol     (LPSTR lpCmd, LPSTR lpArgs)
{
char label_buf[64];
LPSTR lp1, lp2;



   lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   for(lp1=lpArgs; *lp1 && *lp1<=' '; lp1++)
      ;  // find first non-white-space

   if(!*lp1)
   {
      work_buf[0] = _getdrive() + 'A' - 1;
      work_buf[1] = ':';
      work_buf[2] = 0;

      lp2 = lp1;
   }
   else if(!(lp2 = _fstrchr(lp1, ':')))
   {
      PrintString(pSYNTAXERROR);
      PrintString(lpArgs);
      PrintString(pCRLFLF);

      GlobalFreePtr(lpArgs);
      return(TRUE);
   }
   else
   {
      _fstrupr(lp1);

      _fstrncpy(work_buf, lp1, (lp2 - lp1) + 1);

      work_buf[lp2 - lp1 + 1] = 0;

      lp2++;
   }


         /** Find out the CANONICAL drive designation **/


   strcpy(work_buf + sizeof(work_buf)/2, work_buf);  // temporary

   strcat(work_buf, "\\");

   DosQualifyPath(work_buf, work_buf);
                                           /* get the DOS qualified path */

   if(!*work_buf)
   {
      PrintString(pERRORREADINGDRIVE);
      PrintString(work_buf + sizeof(work_buf)/2);
      PrintString(pQCRLFLF);

      GlobalFreePtr(lpArgs);
      return(TRUE);
   }

            /* parse args to ensure no syntax error */

   if(*lp1)
   {
      while(*lp2 && *lp2<=' ') lp2++;
                                  /* find next NON-BLANK or end of string */

      if(*lp2)
      {
         PrintErrorMessage(673);

         GlobalFreePtr(lpArgs);
         return(TRUE);
      }
   }


   if(GetDriveLabel((LPSTR)label_buf, (LPSTR)work_buf))
   {
      PrintString(pERRORREADINGDRIVE);
      PrintString(work_buf + sizeof(work_buf)/2);
      PrintString(pQCRLFLF);

      GlobalFreePtr(lpArgs);
      return(TRUE);
   }
   else
   {
      lp1 = _fstrchr(work_buf, ':');
      if(lp1) *lp1 = 0;

      if(*label_buf<=' ')  /* blank volume label */
      {
         PrintString(pVOLUMEINDRIVE);

         PrintString(work_buf);

         PrintString(pHASNOLABEL);
         if(*label_buf) PrintString(label_buf);
         PrintString(pCRLF);
      }
      else
      {
         PrintString(pVOLUMEINDRIVE);

         PrintString(work_buf);

         PrintErrorMessage(746);     // "' is "
         PrintString((LPSTR)label_buf);
         PrintString(pCRLF);
      }
   }

   GlobalFreePtr(lpArgs);
   return(FALSE);

}



#pragma code_seg("CMDXcopy_TEXT","CODE")


BOOL FAR PASCAL CMDXcopy   (LPSTR lpCmd, LPSTR lpArgs)
{
LPPARSE_INFO lpParseInfo;
LPTREE lpTree, lpT;
LPDIR_INFO lpDirInfo=NULL, lpDI;
HCURSOR hOldCursor;
LPSTR lp1, lp2, lp3, lpPath, lpDest, lpCurPath, lpDirPath, lpDirSpec,
      lpCurDest, lpDestPath, lpDestSpec;
BOOL AFlag,MFlag,PFlag,SFlag,EFlag,VFlag,WFlag,SwitchValue,rval;
int DFlag;
char c;
WORD w, p, attr, wNFiles, wRecurse, wNRecurse, wTreeLevel, wTreeIndex, ctr;
SFTDATE d;
WORD wLevelArray[64];



   /* if 'destination' doesn't contain an existing   */
   /* directory or end with a '\' then the following */
   /* message appears to clarify it:                 */

     /** Does 'destination' specify a file name  **/
     /** or a directory name on the target       **/
     /** (F = file, D = directory)?              **/


   /* XCOPY source [destination] switches */

   /* SWITCHES:  /a      -  only files with ARCHIVE bit set                */
   /*            /m      -  like '/a', but turns ARCHIVE bit off in source */
   /*            /d:date -  only copy files on or after 'date'             */
   /*            /p      -  prompts for confirmation on each file          */
   /*            /s      -  recurse sub-directories                        */
   /*            /e      -  copy even EMPTY sub-directories                */
   /*            /v      -  verifies file as it is written to destination  */
   /*            /w      -  uses a 'PAUSE' before copying                  */


   hOldCursor = MySetCursor(LoadCursor(NULL, IDC_WAIT));

   lpParseInfo = CMDLineParse(lpArgs);

   if(CMDCheckParseInfo(lpParseInfo, "-ADEMPSVW", 1 | CMD_ATLEAST))
   {
      GlobalFreePtr(lpParseInfo);
      MySetCursor(hOldCursor);
      return(TRUE);
   }

   /* well, here we go! */

   AFlag=MFlag=PFlag=SFlag=EFlag=VFlag=WFlag=FALSE;
   DFlag=0;

   p = lpParseInfo->n_switches;  /* # of switches, 1st argument */

   for(w=0; w<p; w++)
   {
      c = toupper(*(lpParseInfo->sbpItemArray[w]));

      if(c=='-')
      {
         SwitchValue = FALSE;
         c = toupper((lpParseInfo->sbpItemArray[w])[1]);
      }
      else
      {
         SwitchValue = TRUE;
      }

      switch(c)
      {
         case 'A':
            AFlag = SwitchValue;
            break;

         case 'D':
            if(SwitchValue) DFlag = 1;   // copy ON or AFTER a specific date
            else            DFlag = -1;  // copy ON or BEFORE a specific date

            lp1 = (LPSTR)(lpParseInfo->sbpItemArray[w])+1;

            if(*(lp1++)!=':' || !*lp1 || atodate(lp1, (LPSFTDATE)&d))
            {
               PrintString(pSYNTAXERROR);
               PrintAChar('/');
               PrintString(lpParseInfo->sbpItemArray[w]);
               PrintString(pQCRLFLF);

               GlobalFreePtr(lpParseInfo);
               MySetCursor(hOldCursor);
               return(TRUE);
            }

         case 'E':
            EFlag = SwitchValue;
            break;

         case 'M':
            MFlag = SwitchValue;
            break;

         case 'P':
            PFlag = SwitchValue;
            break;

         case 'S':
            SFlag = SwitchValue;
            break;

         case 'V':
            VFlag = SwitchValue;
            break;

         case 'W':
            WFlag = SwitchValue;
            break;

         default:

            PrintString(pSYNTAXERROR);
            PrintAChar('/');
            PrintString(lpParseInfo->sbpItemArray[w]);
            PrintString(pQCRLFLF);

            GlobalFreePtr(lpParseInfo);
            MySetCursor(hOldCursor);
            return(TRUE);

      }
   }


   lpPath = lpParseInfo->sbpItemArray[p];    /* the FIRST one! */

   if(lpParseInfo->n_parameters==2)
   {
      lpDest = lpParseInfo->sbpItemArray[p+1]; /* the SECOND one! */
   }
   else if(lpParseInfo->n_parameters==1)
   {
      lpDest = (LPSTR)pWILDCARD;
   }
   else
   {
      PrintErrorMessage(536);
      GlobalFreePtr(lpParseInfo);
      MySetCursor(hOldCursor);
      return(TRUE);
   }


   lp3 = work_buf + sizeof(work_buf)/2;

   if(QualifyPath(work_buf, lpPath, TRUE))
   {
      PrintString(pILLEGALARGUMENT);
      PrintString(lpPath);
      PrintString(pQCRLFLF);

      GlobalFreePtr(lpParseInfo);
      MySetCursor(hOldCursor);
      return(TRUE);
   }
   else
   {
      lp1 = _fstrrchr(work_buf, '\\');
      if(!lp1)
      {
         PrintString(pILLEGALARGUMENT);
         PrintString(lpPath);
         PrintString(pQCRLFLF);

         GlobalFreePtr(lpParseInfo);
         MySetCursor(hOldCursor);
         return(TRUE);
      }
   }

   lpPath = work_buf;


   if(QualifyPath(lp3, lpDest, TRUE))
   {
      PrintString(pILLEGALARGUMENT);
      PrintString(lpDest);
      PrintString(pQCRLFLF);

      GlobalFreePtr(lpParseInfo);
      MySetCursor(hOldCursor);
      return(TRUE);
   }
   else
   {
      lp2 = _fstrrchr(lp3, '\\');
      if(!lp2)
      {
         PrintString(pILLEGALARGUMENT);
         PrintString(lpDest);
         PrintString(pQCRLFLF);

         GlobalFreePtr(lpParseInfo);
         MySetCursor(hOldCursor);
         return(TRUE);
      }
   }


   MySetCursor(hOldCursor);

   lpDest = lp3;

   if(!_fstrchr(lp2, '?') && !_fstrchr(lp2, '*') &&
      (_fstrchr(lp1, '?') || _fstrchr(lp1, '*')) )
   {
        /* no output wildcards - is this spec for a file or directory? */


      do
      {
         PrintErrorMessage(683);
         PrintString(lpDest);
         PrintErrorMessage(684);

         lp3 = lpDest + lstrlen(lpDest) + 1;

         GetUserInput(lp3);

         if(ctrl_break)
         {
            PrintString(pCTRLBREAKMESSAGE);
            GlobalFreePtr(lpParseInfo);

            if(!BatchMode) ctrl_break = FALSE; // eat it if not in BATCH mode

            return(TRUE);
         }

         if(toupper(*lp3)=='F')
         {
            break;  /* no further processing */
         }
         else if(toupper(*lp3)=='D')
         {
            lp2 = lpDest + lstrlen(lpDest);

            lstrcpy(lp2, "\\????????.???");
            break;
         }
         else
         {
            PrintErrorMessage(685);
            PrintString(lp3);
            PrintString(pQCRLFLF);
         }

      } while(TRUE);
   }

   MySetCursor(LoadCursor(NULL, IDC_WAIT));  /* hourglass is back! */

   lpDirPath = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE, sizeof(work_buf) +
                                     lstrlen(lpPath) + lstrlen(lpDest));
   if(!lpDirPath)
   {
      PrintString(pNOTENOUGHMEMORY);
      GlobalFreePtr(lpParseInfo);
      MySetCursor(hOldCursor);
      return(TRUE);
   }


   *lp1 = 0;
   lstrcpy(lpDirPath, lpPath);      /* make a copy in global memory block */
   lstrcat(lpDirPath, pBACKSLASH);

   lpDirSpec = lpDirPath + lstrlen(lpDirPath) + 1;
   lstrcpy(lpDirSpec, lp1 + 1);       /* make copy of the 'dir spec' used */


   *lp2 = 0;
   lpDestPath = lpDirSpec + lstrlen(lpDirSpec) + 1;
   lstrcpy(lpDestPath, lpDest);     /* make a copy in global memory block */
   lstrcat(lpDestPath, pBACKSLASH);

   lpDestSpec = lpDestPath + lstrlen(lpDestPath) + 1;
   lstrcpy(lpDestSpec, lp2 + 1);     /* make copy of the 'dest spec' used */


   lpCurPath = lpDestSpec + lstrlen(lpDestSpec) + 1;
   lpCurDest = lpCurPath + sizeof(work_buf)/2;





            /** This begins the 'recurse specific' code **/

   if(SFlag)
   {

      lstrcpy(work_buf, lpDirPath);

      if(!(lpTree = GetDirTree(work_buf)))
      {
         if(ctrl_break)
         {
            PrintString(pCTRLC);
            PrintString(pCRLFLF);

            if(!BatchMode) ctrl_break = FALSE;  // eat ctrl break if not BATCH
         }
         else
         {
            PrintErrorMessage(576);
         }

         GlobalFreePtr(lpDirPath);
         GlobalFreePtr(lpParseInfo);

         MySetCursor(hOldCursor);
         return(TRUE);
      }

      lpT = lpTree;

      if(!(*lpTree->szDir))  /* empty tree list? */
      {
         wNRecurse = 1;
         SFlag = FALSE;
         GlobalFreePtr(lpTree);
         lpTree = NULL;
      }
      else
      {
         wNRecurse = 0xffff;  /* for now, it's a bit easier... */
         wTreeLevel = 0;
         wTreeIndex = 0;

         wLevelArray[wTreeLevel] = wTreeIndex;
      }

   }
   else
   {
      wNRecurse = 1;   /* always go at least 1 time; helps detect errors */
      lpTree = NULL;
   }

   attr = _A_HIDDEN|_A_SYSTEM|_A_ARCH|_A_RDONLY; /* enhancement */


   if(WFlag)
   {
      MySetCursor(hOldCursor);

      PrintErrorMessage(686);
      GetUserInput(work_buf);

      if(ctrl_break)
      {
         PrintString(pCTRLBREAKMESSAGE);

         GlobalFreePtr(lpDirPath);
         GlobalFreePtr(lpParseInfo);

         if(lpTree)  GlobalFreePtr(lpTree);

         if(!BatchMode) ctrl_break = FALSE;  // eat ctrl break if not BATCH

         return(TRUE);
      }

      PrintString(pCRLF);

      MySetCursor(LoadCursor(NULL, IDC_WAIT));
   }

   /* first thing's first - make sure that the destination path exists! */

   lp1 = _fstrchr(lpDestPath, '\\');  /* find the FIRST backslash */
                                      /* this should be the ROOT DIR */

   while(lp1 && (lp1 = _fstrchr(lp1 + 1, '\\')))  /* find next directory */
   {

      *lp1 = 0;     /* temporary */
      lstrcpy(work_buf, lpDestPath);
      *lp1 = '\\';  /* restored */

      My_Mkdir(work_buf);        /* make the directory (ignore errors) */

   }

   for(wRecurse=0; wRecurse<wNRecurse; wRecurse++)
   {
                        /** Get directory list buffer **/

      if(wRecurse && SFlag)
      {
         lstrcpy(lpCurPath, lpDirPath);
         lstrcpy(lpCurDest, lpDestPath);

         for(ctr=0; ctr<=wTreeLevel; ctr++)  /* build current path */
         {
            lstrcat(lpCurPath, lpTree[wLevelArray[ctr]].szDir);
            lstrcat(lpCurPath, pBACKSLASH);

            lstrcat(lpCurDest, lpTree[wLevelArray[ctr]].szDir);
            lstrcat(lpCurDest, pBACKSLASH);
         }

         lstrcat(lpCurPath, lpDirSpec);
         lstrcat(lpCurDest, lpDestSpec);

         if(lpTree[wTreeIndex].wChild)  /* a child element exists? */
         {
            wLevelArray[wTreeLevel++] = wTreeIndex;
            wTreeIndex = lpTree[wTreeIndex].wChild;

            wLevelArray[wTreeLevel] = wTreeIndex;
         }
         else if(lpTree[wTreeIndex].wSibling)
         {
            wTreeIndex = lpTree[wTreeIndex].wSibling;
            wLevelArray[wTreeLevel] = wTreeIndex;
         }
         else
         {
            while(wTreeLevel>0 && !lpTree[wTreeIndex].wSibling)
            {
               wTreeIndex = wLevelArray[--wTreeLevel];
            }

            if(!wTreeLevel && !lpTree[wTreeIndex].wSibling)
            {
               wNRecurse = wRecurse; /* end after this one! */
            }
            else
            {
               wTreeIndex = lpTree[wTreeIndex].wSibling;
               wLevelArray[wTreeLevel] = wTreeIndex;
            }
         }
      }
      else
      {        /* first time through - current dir (always) */

         lstrcpy(lpCurPath, lpDirPath);
         lstrcat(lpCurPath, lpDirSpec);

         lstrcpy(lpCurDest, lpDestPath);
         lstrcat(lpCurDest, lpDestSpec);

      }

      wNFiles = GetDirList(lpCurPath, attr, (LPDIR_INFO FAR *)&lpDirInfo, NULL);

      if(PFlag || DFlag || AFlag || MFlag)         /* file checking stuff */
      {

         if(PFlag) MySetCursor(hOldCursor);  /* no hourglass if prompting */

         for(w=0, lpDI=lpDirInfo; w<wNFiles;)
         {
            /* check for DATE, 'archive bit' and USER RESPONSE */

            if((DFlag && ((DFlag>0 && datecmp((LPSFTDATE)&(lpDI->date),&d)<0) ||
                          (DFlag<0 && datecmp((LPSFTDATE)&(lpDI->date),&d)>0)) ) ||
               ((AFlag || MFlag) && !(lpDI->attrib & _A_ARCH) ) ||
               (PFlag && !CMDXcopyConfirm(lpCurPath, lpDI)) )
            {
               if(PFlag && ctrl_break)      /* only when using 'confirm'! */
               {
                  PrintString(pCTRLBREAKMESSAGE);

                  GlobalFreePtr(lpDirPath);
                  GlobalFreePtr(lpParseInfo);

                  if(lpTree)    GlobalFreePtr(lpTree);
                  if(lpDirInfo) GlobalFreePtr(lpDirInfo);

                  if(!BatchMode) ctrl_break = FALSE; // eat it if not BATCH

                  return(TRUE);
               }

               if((--wNFiles))
               {
                  _hmemcpy((LPSTR)lpDI, (LPSTR)(lpDI + 1),
                           (wNFiles - w) * sizeof(*lpDI));
               }

               _hmemset((LPSTR)(lpDI + wNFiles), 0, sizeof(*lpDI));

            }
            else
            {
               lpDI++;
               w++;
            }
         }

         if(PFlag) MySetCursor(LoadCursor(NULL, IDC_WAIT));
      }


      if((wNFiles && SFlag) || EFlag)
      {                   /* user specified 'create empty sub-directories' */
         /* need to build all sub-directories right now!! */

         lstrcpy(work_buf, lpCurDest);  /* copy current 'dest path' here */
         lp1 = _fstrrchr(work_buf, '\\');  /* find the 'last' backslash */

         if(lp1)  /* only if I find it - safety precaution */
         {
            if(*(lp1 - 1)!=':' && *(lp1 - 1)!='\\')
            {    /* also - it CANNOT be a root directory or server name */

               *lp1 = 0;           /* terminate the string at the '\' */

               My_Mkdir(work_buf);  /* and attempt to make the directory */
                   /* (don't bother checking for errors at this point) */

            }
         }

      }

      if(wNFiles)  /* if we have files to copy, that is... */
      {
         lp1 = _fstrrchr(lpCurPath, '\\'); /* find last '\' in source path */
         if(lp1)
         {
            *(lp1 + 1) = 0;  /* terminate string just after '\' */
         }

         rval = SubmitDirInfoToCopy(lpDirInfo, wNFiles, lpCurPath,
                                    lpCurDest, VFlag, MFlag);

         if(rval)
         {
            PrintErrorMessage(687);
            GlobalFreePtr(lpDirInfo);
            lpDirInfo = NULL;

            break;
         }

         lpDirInfo = NULL;
      }
      else
      {
         if(lpDirInfo)  GlobalFreePtr(lpDirInfo);
         lpDirInfo = NULL;
      }
   }


   ReportCopyResults();

   if(lpTree)    GlobalFreePtr(lpTree);
   if(lpDirInfo) GlobalFreePtr(lpDirInfo);

   GlobalFreePtr(lpDirPath);
   GlobalFreePtr(lpParseInfo);

   MySetCursor(hOldCursor);
   return(rval);


}


BOOL FAR PASCAL CMDXcopyConfirm(LPSTR lpPathSpec, LPDIR_INFO lpDI)
{
LPSTR lp1;
char temp_buf[64], tbuf[80];

   lp1 = _fstrrchr(lpPathSpec, '\\');  /* find the 'last' backslash */

   if(lp1) *lp1 = 0;  /* temporarily */

   PrintString(lpPathSpec);
   PrintString(pBACKSLASH);
   if(lp1) *lp1 = '\\';

   _hmemset(tbuf, 0, sizeof(tbuf));
   _fstrncpy(tbuf, lpDI->fullname, sizeof(lpDI->fullname));

   do
   {
      PrintString(tbuf);
      PrintErrorMessage(847);   // " (Y/n)?  "

      GetUserInput(temp_buf);

      if(ctrl_break) return(FALSE);  // a "NO"

      if(toupper(*temp_buf)!='Y' && toupper(*temp_buf)!='N'
           && *temp_buf > ' ')
      {
         MessageBeep(0);
      }

   } while(toupper(*temp_buf)!='Y' && toupper(*temp_buf)!='N'
           && *temp_buf > ' ');

   return(toupper(*temp_buf)!='N');

}
