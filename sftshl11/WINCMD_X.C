/***************************************************************************/
/*                                                                         */
/*   WINCMD_X.C - Command line interpreter for Microsoft(r) Windows (tm)   */
/*                                                                         */
/*                      TRADE SECRET - CONFIDENTIAL                        */
/*                                                                         */
/*             The compiled libraries and API header files are             */
/*          (c) Copyright 1990-1995 by Stewart~Frazier Tools, Inc          */
/*                                                                         */
/*   This contains the 'REXX' processing stuff (via WINREXX API)           */
/*                                                                         */
/***************************************************************************/


#define THE_WORKS
#include "mywin.h"

#include "mth.h"
#include "wincmd.h"         /* 'dependency' related definitions */
#include "wincmd_f.h"       /* function prototypes and variables */

#define INCL_REXXSAA
#define INCL_RXLIBLINK

#include "wrexx.h"          /* the 'WINREXX' API header file */


#if defined(M_I86LM)

  #pragma message("** Large Model In Use - Single Instance Only!! **")

#elif !defined(M_I86MM)

  #error  "** Must use MEDIUM model to compile, silly!! **"

#else

  #pragma message("** Medium Model Compile - Multiple Instances OK **")

#endif



                 /* function prototypes used here */

short FAR PASCAL RxCallBack(WORD wExitFunc, WORD wExitSubFunc,
                            LPBYTE lpParm);

short FAR PASCAL RxHaltCallBack(WORD wExitFunc, WORD wExitSubFunc,
                                 LPBYTE lpParm);

short FAR PASCAL RxSioCallBack(WORD wExitFunc, WORD wExitSubFunc,
                               LPBYTE lpParm);

short FAR PASCAL RxExtCallBack(WORD wExitFunc, WORD wExitSubFunc,
                               LPBYTE lpParm);

short FAR PASCAL RxCmdCallBack(WORD wExitFunc, WORD wExitSubFunc,
                               LPBYTE lpParm);

BOOL FAR _cdecl _multiRxExitRegister(WORD wCount, FARPROC lpProc, ...);


short APIENTRY RxExternalCallback(LPSTR, short, PRXSTRING, LPBYTE, PRXSTRING);

WORD FAR PASCAL RxGetCharInput(LPSTR lpBuf);


BOOL FAR PASCAL RxSubComRegistrationOK(WORD wRegisterRC,LPSTR lpFuncName);



          // SEGMENT DEFINITIONS - WHERE THE STUFF GOES, EH?


#define REXXRUNBASED __based(__segname("REXXRUN_TEXT"))


#pragma alloc_text (REXXRUN_TEXT,RexxRun,RxCallBack,RxHaltCallBack,\
                                 RxSioCallBack,RxExtCallBack,RxCmdCallBack,\
                                 _multiRxExitRegister,RxExternalCallback,\
                                 RxGetCharInput,RxSubComRegistrationOK)



        /** a few statically defined PROC addresses below... **/


int (FAR PASCAL *lpREXXSAA)(int,PRXSTRING,LPBYTE,PRXSTRING,LPBYTE,
                            int,PRXSYSEXIT,LPINT,PRXSTRING)           = NULL;

       // FUNCTION 'RxLibLink' added 10/21/92 - Beta release 0.96
int (APIENTRY *lpRxLibLink)(HTASK,int)                                = NULL;

WORD (APIENTRY *lpRxSubcomRegister)(PSCBLOCK)                         = NULL;
 
WORD (APIENTRY *lpRxSubcomQuery)(LPBYTE,LPBYTE,LPWORD,double far *)   = NULL;
 
WORD (APIENTRY *lpRxSubcomLoad)(LPBYTE,LPBYTE)                        = NULL;
 
WORD (APIENTRY *lpRxSubcomDrop)(LPBYTE,LPBYTE)                        = NULL;
 
WORD (APIENTRY *lpRxSubcomExecute)(LPBYTE,LPBYTE,PRXSTRING,LPWORD,
                                   LPWORD,PRXSTRING)                  = NULL;
 
WORD (APIENTRY *lpRxVar)(PSHVBLOCK)                                   = NULL;

WORD (APIENTRY *lpRxFunctionRegister)(LPBYTE,LPBYTE,LPBYTE,WORD)      = NULL;
 
WORD (APIENTRY *lpRxFunctionDeregister)(LPBYTE)                       = NULL;
 
WORD (APIENTRY *lpRxFunctionQuery)(LPBYTE)                            = NULL;
 
WORD (APIENTRY *lpRxFunctionCall)(LPBYTE,WORD,PRXSTRING,LPWORD,
                                  PRXSTRING, LPBYTE)                  = NULL;
 
WORD (APIENTRY *lpRxExitRegister)(PSCBLOCK)                           = NULL;
 
WORD (APIENTRY *lpRxExitDrop)(LPBYTE,LPBYTE)                          = NULL;
 
WORD (APIENTRY *lpRxExitQuery)(LPBYTE,LPBYTE,LPWORD,double far *)     = NULL;
 
WORD (APIENTRY *lpRxMacroChange)(LPBYTE,LPBYTE,WORD)                  = NULL;
 
WORD (APIENTRY *lpRxMacroDrop)(LPBYTE)                                = NULL;
 
WORD (APIENTRY *lpRxMacroSave)(WORD,LPBYTE LPBYTE)                    = NULL;
 
WORD (APIENTRY *lpRxMacroLoad)(WORD,LPBYTE LPBYTE)                    = NULL;
 
WORD (APIENTRY *lpRxMacroQuery)(LPBYTE,LPWORD)                        = NULL;
 
WORD (APIENTRY *lpRxMacroReOrder)(LPBYTE,WORD)                        = NULL;
 
WORD (APIENTRY *lpRxMacroErase)(VOID)                                 = NULL;
 



        // STATIC ARRAYS LOCATED IN 'REXXRUN_TEXT' (CODE) SEGMENT


FARPROC NEAR * const REXXRUNBASED pRexxProcs[]={
  (FARPROC NEAR *)&lpREXXSAA,             (FARPROC NEAR *)&lpRxSubcomRegister,
  (FARPROC NEAR *)&lpRxSubcomQuery,       (FARPROC NEAR *)&lpRxSubcomLoad,
  (FARPROC NEAR *)&lpRxSubcomDrop,        (FARPROC NEAR *)&lpRxSubcomExecute,
  (FARPROC NEAR *)&lpRxVar,               (FARPROC NEAR *)&lpRxFunctionRegister,
  (FARPROC NEAR *)&lpRxFunctionDeregister,(FARPROC NEAR *)&lpRxFunctionQuery,
  (FARPROC NEAR *)&lpRxFunctionCall,      (FARPROC NEAR *)&lpRxExitRegister,
  (FARPROC NEAR *)&lpRxExitDrop,          (FARPROC NEAR *)&lpRxExitQuery,
  (FARPROC NEAR *)&lpRxMacroChange,       (FARPROC NEAR *)&lpRxMacroDrop,
  (FARPROC NEAR *)&lpRxMacroSave,         (FARPROC NEAR *)&lpRxMacroLoad,
  (FARPROC NEAR *)&lpRxMacroQuery,        (FARPROC NEAR *)&lpRxMacroReOrder,
  (FARPROC NEAR *)&lpRxMacroErase,        (FARPROC NEAR *)&lpRxLibLink };


#define CRB const char REXXRUNBASED
#define CRBP const char REXXRUNBASED *


static CRB szREXXSAA[]              = "REXXSAA";
static CRB szRxSubcomRegister[]     = "RxSubcomRegister";
static CRB szRxSubcomQuery[]        = "RxSubcomQuery";
static CRB szRxSubcomLoad[]         = "RxSubcomLoad";
static CRB szRxSubcomDrop[]         = "RxSubcomDrop";
static CRB szRxSubcomExecute[]      = "RxSubcomExecute";
static CRB szRxVar[]                = "RxVar";
static CRB szRxFunctionRegister[]   = "RxFunctionRegister";
static CRB szRxFunctionDeregister[] = "RxFunctionDeregister";
static CRB szRxFunctionQuery[]      = "RxFunctionQuery";
static CRB szRxFunctionCall[]       = "RxFunctionCall";
static CRB szRxExitRegister[]       = "RxExitRegister";
static CRB szRxExitDrop[]           = "RxExitDrop";
static CRB szRxExitQuery[]          = "RxExitQuery";
static CRB szRxMacroChange[]        = "RxMacroChange";
static CRB szRxMacroDrop[]          = "RxMacroDrop";
static CRB szRxMacroSave[]          = "RxMacroSave";
static CRB szRxMacroLoad[]          = "RxMacroLoad";
static CRB szRxMacroQuery[]         = "RxMacroQuery";
static CRB szRxMacroReOrder[]       = "RxMacroReOrder";
static CRB szRxMacroErase[]         = "RxMacroErase";
static CRB szRxLibLink[]            = "RxLibLink";


static CRBP const REXXRUNBASED pRexxProcNames[]={
  (CRBP)szREXXSAA,             (CRBP)szRxSubcomRegister,
  (CRBP)szRxSubcomQuery,       (CRBP)szRxSubcomLoad,
  (CRBP)szRxSubcomDrop,        (CRBP)szRxSubcomExecute,
  (CRBP)szRxVar,               (CRBP)szRxFunctionRegister,
  (CRBP)szRxFunctionDeregister,(CRBP)szRxFunctionQuery,
  (CRBP)szRxFunctionCall,      (CRBP)szRxExitRegister,
  (CRBP)szRxExitDrop,          (CRBP)szRxExitQuery,
  (CRBP)szRxMacroChange,       (CRBP)szRxMacroDrop,
  (CRBP)szRxMacroSave,         (CRBP)szRxMacroLoad,
  (CRBP)szRxMacroQuery,        (CRBP)szRxMacroReOrder,
  (CRBP)szRxMacroErase,        (CRBP)szRxLibLink };


static CRB szMessageBox[]  = "MessageBox";
static CRB szCancelBox[]   = "CancelBox";
static CRB szQuestionBox[] = "QuestionBox";
static CRB szPromptBox[]   = "PromptBox";
static CRB szChoiceBox[]   = "ChoiceBox";
static CRB szGetProfile[]  = "GetProfile";
static CRB szSetProfile[]  = "SetProfile";


static CRB pWREXXDLL_NAME[]="WREXX.DLL";
static CRB pWREXXDLL_NAME2[]=".\\WREXX.DLL";




            // OTHER ARRAYS LOCATED IN DEFAULT DATA SEGMENT


static const char *pExternalFunctions[]={
    "CharOut" };                             /* only 1 so far */


static BOOL IsHalt = NULL;  /* this causes the REXX program to HALT if TRUE */

static const char pRxHalt[]    ="RxHalt";
static const char pRxSio[]     ="RxSio";
static const char pRxCmd[]     ="RxCmd";
static const char pRxFnc[]     ="RxFnc";
static const char pWINCMD[]    ="RxShell";
static const char pWRXTRNL[]   ="WRXTRNL";
static const char pWRX[]       ="WRX";
static const char pWrXtrnlFn[] ="WrExternalFunctions";
static const char pRxWinUtil[] ="RxWinUtilities";



           // "SHELL.REX" embedded REXX code - SHELL program!!


static const char REXXRUNBASED shell_prog[]=
   "/* SHELL.REX */\r\n"
   "SAY D2C(27)\"[1;36;40m\";\r\n"  /* sets background at bright cyan on black */
   "SAY \"                          \"D2C(27)\"[1;33;41m** REXX COMMAND SHELL **\"D2C(27)\"[1;36;40m\";\r\n"
   "SAY \"                          \"D2C(27)\"[0;30;47m** Type 'EXIT' to end **\"D2C(27)\"[1;36;40m\";\r\n"
   "SAY \"\"\r\n"                  /* a blank line */
   "\r\n"
   "the_big_loop:\r\n"
   "\r\n"
   "SIGNAL ON ERROR NAME signalproc1\r\n"
   "SIGNAL ON FAILURE NAME signalproc2\r\n"
   "SIGNAL ON HALT NAME signalproc3\r\n"
   "SIGNAL ON NOTREADY NAME signalproc4\r\n"
   "/* SIGNAL ON NOVALUE NAME signalproc5 */\r\n"
   "SIGNAL ON SYNTAX NAME signalproc6\r\n"
   "\r\n"
   "Do Forever\r\n"
   "   PARSE PULL Command\r\n"
   "   INTERPRET Command\r\n"
   "End\r\n"
   "\r\n"
   "signalproc1:\r\n"
   "\r\n"
   "   SAY \"?Error in command line\"\r\n"
   "   SIGNAL the_big_loop\r\n"
   "\r\n"
   "signalproc2:\r\n"
   "   SAY \"?External command failed\"\r\n"
   "   SIGNAL the_big_loop\r\n"
   "\r\n"
   "signalproc3:\r\n"
   "   SAY \"Halt detected - terminate REXX session?\"\r\n"
   "   PARSE PULL yesorno\r\n"
   "   if left(yesorno,1)==\"Y\" then exit\r\n"
   "\r\n"
   "   SIGNAL the_big_loop\r\n"
   "\r\n"
   "signalproc4:\r\n"
   "   SAY \"?'NOT READY' error\"\r\n"
   "   SIGNAL the_big_loop\r\n"
   "\r\n"
   "signalproc5:\r\n"
   "   SAY \"?'NO VALUE' error\"\r\n"
   "   SIGNAL the_big_loop\r\n"
   "\r\n"
   "signalproc6:\r\n"
   "   SAY \"?Syntax Error\"\r\n"
   "   SIGNAL the_big_loop\r\n"
   "\r\n"
   "\r\n"
   "/* END Shell.REX */\r\n";


void FAR PASCAL RexxUnattach()
{
   if(hRexxAPI)
   {
      if(lpRxLibLink) lpRxLibLink(GetCurrentTask(), THREAD_DETACH);

      FreeLibrary(hRexxAPI);

      hRexxAPI = NULL;
   }
}


BOOL FAR PASCAL RexxRun(LPSTR lpArgs, BOOL IsCommand)
{
int rcode;
HCURSOR hOldCursor;
WORD i, /* wRC, */ j;
BOOL wRval;
LPSTR lp1, lp2;
FARPROC lpProc, lpProc2;
/* SCBLOCK scb; */
RXSTRING lprxsArg[10], lprxsInStore[2], rxsReturn;
RXSYSEXIT lprxSysExit[5];
OFSTRUCT ofstr;
HMODULE hWRXTRNL = NULL;
LPSTR lpShell;


   if(!hRexxAPI)    /* have not found the REXX API yet? */
   {
      hOldCursor = MySetCursor(LoadCursor(NULL, IDC_WAIT));

      if(hRexxAPI = GetModuleHandle("WREXX"))
      {
         GetModuleFileName(hRexxAPI, (LPSTR)ofstr.szPathName, 
                           sizeof(ofstr.szPathName));

         hRexxAPI = LoadLibrary((LPSTR)ofstr.szPathName);
      }   
      else if(MyOpenFile(pWREXXDLL_NAME2, (LPOFSTRUCT)&ofstr,
                         OF_EXIST | OF_SHARE_COMPAT)==HFILE_ERROR
           && MyOpenFile(pWREXXDLL_NAME2, (LPOFSTRUCT)&ofstr,
                         OF_EXIST | OF_SHARE_DENY_NONE)==HFILE_ERROR
           && MyOpenFile(pWREXXDLL_NAME, (LPOFSTRUCT)&ofstr,
                         OF_EXIST | OF_SHARE_COMPAT)==HFILE_ERROR
           && MyOpenFile(pWREXXDLL_NAME, (LPOFSTRUCT)&ofstr,
                         OF_EXIST | OF_SHARE_DENY_NONE)==HFILE_ERROR)
      {
         hRexxAPI = (HMODULE)NULL;
      }
      else
      {
         hRexxAPI = LoadLibrary((LPSTR)ofstr.szPathName);
      }

      if((WORD)hRexxAPI>=32)          /* this means it's ok! */
      {
         for(i=0; i<N_DIMENSIONS(pRexxProcs); i++)
         {
            if(!(*(pRexxProcs[i]) = GetProcAddress(hRexxAPI,
                                                   pRexxProcNames[i])))
            {
               PrintErrorMessage(769);
               FreeLibrary(hRexxAPI);
               hRexxAPI = NULL;

               MySetCursor(hOldCursor);
               return(TRUE);
            }
         }


         /* next thing I do is a call to 'RxLibLink()' to register my task */

         if(lpRxLibLink(GetCurrentTask(), THREAD_ATTACH)!=THREAD_ATTACH_AOK)
         {
            PrintErrorMessage(769);
            FreeLibrary(hRexxAPI);
            hRexxAPI = NULL;

            MySetCursor(hOldCursor);
            return(TRUE);
         }
      }
      else
      {
         PrintErrorMessage(768);

         MySetCursor(hOldCursor);
         return(TRUE);
      }




      /* register the external stuff for WRXTRNL.DLL */

      lpRxFunctionRegister(szMessageBox,pWRXTRNL, pWrXtrnlFn,
                            RXFUNC_DYNALINK);
      lpRxFunctionRegister(szCancelBox,pWRXTRNL, pWrXtrnlFn,
                            RXFUNC_DYNALINK);
      lpRxFunctionRegister(szQuestionBox,pWRXTRNL, pWrXtrnlFn,
                            RXFUNC_DYNALINK);
      lpRxFunctionRegister(szPromptBox,pWRXTRNL, pWrXtrnlFn,
                            RXFUNC_DYNALINK);
      lpRxFunctionRegister(szChoiceBox,pWRXTRNL, pWrXtrnlFn,
                            RXFUNC_DYNALINK);
    
      lpRxFunctionRegister(szGetProfile,pWRXTRNL, pRxWinUtil,
                            RXFUNC_DYNALINK);
      lpRxFunctionRegister(szSetProfile,pWRXTRNL, pRxWinUtil,
                            RXFUNC_DYNALINK);



   /** at this point we can assume that the proc addresses are all good! **/


   /** NEXT:  Register my 'RxHalt' callback for 'LoopDispatch()'ing **/
   /**        and my 'RxSio' callback for performing System I/O     **/

      lpProc = MakeProcInstance((FARPROC)RxCallBack, hInst);
      lpProc2 = MakeProcInstance((FARPROC)RxExternalCallback, hInst);
      if(!lpProc || !lpProc2)
      {
         PrintErrorMessage(770);

         if(lpProc) FreeProcInstance(lpProc);
         FreeLibrary(hRexxAPI);
         hRexxAPI = NULL;
         return(TRUE);
      }

      if(_multiRxExitRegister(4, lpProc, (LPSTR)pRxHalt, (LPSTR)pRxSio,
                                         (LPSTR)pRxFnc,  (LPSTR)pRxCmd))
      {
         FreeProcInstance(lpProc);
         FreeProcInstance(lpProc2);

         FreeLibrary(hRexxAPI);
         hRexxAPI = NULL;

         MySetCursor(hOldCursor);
         return(TRUE);
      }

//   for(i=0; i<N_DIMENSIONS(pExternalFunctions); i++)
//   {
//      wRC = lpRxFunctionRegister(pExternalFunctions[i], NULL, 
//                                 (LPSTR)lpProc2, RXFUNC_CALLENTRY);
//
//      if(wRC!=RXFUNC_OK)
//      {
//
//         if(wRC==RXFUNC_DEFINED)
//         {
//            PrintString("?Function \"");
//            PrintString(pExternalFunctions[i]);
//            PrintString("\" is already defined - attempting re-define - ");
//            lpRxFunctionDeregister(pExternalFunctions[i]);
//                   /* first, try to rip the old one out */
//
//            wRC = lpRxFunctionRegister(pExternalFunctions[i], NULL, 
//                                       (LPSTR)lpProc2, RXFUNC_CALLENTRY);
//            if(wRC!=RXFUNC_OK)
//            {
//               PrintString("FAILED!\r\n\n");
//            }
//            else
//            {
//               PrintString("SUCCESS!\r\n\n");
//               continue;  /* it worked!  keep on going */
//            }
//         }
//
//         for(j=i; j>0; j--)
//         {
//            lpRxFunctionDeregister(pExternalFunctions[j - 1]);
//         }
//
//         break;
//      }
//   }
//
//   if(i<N_DIMENSIONS(pExternalFunctions))  /* an error occurred above */
//   {
//
//      lpRxExitDrop(pRxHalt, NULL);
//      lpRxExitDrop(pRxSio, NULL);
//      lpRxExitDrop(pRxCmd, NULL);
//      lpRxExitDrop(pRxFnc, NULL);
//
//      FreeProcInstance(lpProc);
//      FreeProcInstance(lpProc2);
//
//      FreeLibrary(hRexxAPI);
//      hRexxAPI = NULL;
//
//      MySetCursor(hOldCursor);
//      return(TRUE);
//   }

      MySetCursor(hOldCursor);
   }




   /** STEP 1:  prepare everything to handle 'non-REXX' commands **/

   lprxSysExit[0].sysexit_name = pRxHalt;  /* perform 'yields' */
   lprxSysExit[0].sysexit_code = RXHLT;

   lprxSysExit[1].sysexit_name = pRxSio;   /* trap SYSTEM i/o */
   lprxSysExit[1].sysexit_code = RXSIO;

   lprxSysExit[2].sysexit_name = pRxFnc;   /* trap EXTERNAL FUNCTIONS */
   lprxSysExit[2].sysexit_code = RXFNC;

   lprxSysExit[3].sysexit_name = pRxCmd;   /* trap WINCMD commands */
   lprxSysExit[3].sysexit_code = RXCMD;


   lprxSysExit[4].sysexit_name = NULL;
   lprxSysExit[4].sysexit_code = RXENDLST;  /* marks 'end of list' */


   IsHalt = FALSE;  /* initially don't halt the thing, of course! */

   wRval = TRUE;  /* a default value in case something barphs */

   rxsReturn.strptr = NULL;
   rxsReturn.strlength = 0;

   _hmemset((LPSTR)lprxsArg, 0, sizeof(lprxsArg));

   if(IsCommand)  /* the 'lpArgs' string points directly to a command */
   {
      wRval = FALSE;

      if(!*lpArgs)
      {
         lpShell = GlobalAllocPtr(GMEM_MOVEABLE, sizeof(shell_prog)+1);
         if(!lpShell)
         {
            wRval = TRUE;
         }
         else
         {
            lstrcpy(lpShell, shell_prog);

            lprxsInStore[0].strptr = lpShell;
            lprxsInStore[0].strlength = lstrlen(lpShell);
         }
      }
      else
      {
         lprxsInStore[0].strptr = lpArgs;
         lprxsInStore[0].strlength = lstrlen(lpArgs);

         lpShell = NULL;
      }

      lprxsInStore[1].strptr = NULL;
      lprxsInStore[1].strlength = 0;

      if(wRval ||
         lpREXXSAA(0, lprxsArg, pWINCMD, lprxsInStore, pWINCMD, RXCOMMAND,
                   lprxSysExit, &rcode, &rxsReturn))
      {
         if(wRval || !IsHalt)
         {
            PrintErrorMessage(771);

            wRval = TRUE;
         }
      }
      else
      {
         wRval = FALSE;
      }

      if(lprxsInStore[1].strptr) GlobalFreePtr(lprxsInStore[1].strptr);

      if(lpShell) GlobalFreePtr(lpShell);

      IsHalt = FALSE;
   }
   else           /* the 'lpArgs' string points to a fully qualified path */
   {              /* and optionally arguments 'attached' to the thing.    */

                  /* parse the arguments (if any) */

      for(lp1=lpArgs; *lp1 && *lp1<=' '; lp1++)
         ;  /* find first non-space - following this is file name */

      while(*lp1 && *lp1>' ') lp1++;  /* find next ' ' */
      if(*lp1) *lp1++ = 0;  /* terminate file name - point to next char */

      j = 0;
      while(*lp1)
      {
         for(lp2=lp1; *lp2 && *lp2<=' '; lp2++)
            ;  /* find next non-space */
         if(!*lp2) break;           /* I am done - end of string! */

         lprxsArg[j].strptr = lp2;

         for(lp1=lp2, i=0; *lp1 && *lp1>' '; lp1++, i++)
            ; /* find next white-space character or end of string */

         if(*lp1)  *lp1++ = 0;  /* terminate previous arg. */
         lprxsArg[j++].strlength = i;  /* store string length here */
                                       /* and increment arg count */
      }
                      /* execute the program!! */

      if(lpREXXSAA(j, lprxsArg, lpArgs, NULL, pWINCMD, RXCOMMAND,
                   lprxSysExit, &rcode, &rxsReturn))
      {
         if(!IsHalt)
         {
            PrintErrorMessage(771);
         }
         else
         {
            PrintErrorMessage(772);
         }

         wRval = !IsHalt;

         IsHalt = FALSE;
      }
      else
      {
         wRval = FALSE;
      }
   }

          /* clean up any memory used by this here thang! */

   if(rxsReturn.strptr)       GlobalFreePtr(rxsReturn.strptr);

//
//   lpRxExitDrop(pRxHalt, NULL);
//   lpRxExitDrop(pRxSio, NULL);
//   lpRxExitDrop(pRxCmd, NULL);
//   lpRxExitDrop(pRxFnc, NULL);
//
//   for(i=0; i<N_DIMENSIONS(pExternalFunctions); i++)
//   {
//      lpRxFunctionDeregister(pExternalFunctions[i]);
//   }
//
//   FreeProcInstance(lpProc);
//   FreeProcInstance(lpProc2);

   return(wRval);
}



short FAR PASCAL RxCallBack(WORD wExitFunc, WORD wExitSubFunc,
                            LPBYTE lpParm)
{
   switch(wExitFunc)
   {
      case RXSIO:
         return(RxSioCallBack(wExitFunc, wExitSubFunc, lpParm));

      case RXHLT:
         return(RxHaltCallBack(wExitFunc, wExitSubFunc, lpParm));

      case RXFNC:
         return(RxExtCallBack(wExitFunc, wExitSubFunc, lpParm));

      case RXCMD:
         return(RxCmdCallBack(wExitFunc, wExitSubFunc, lpParm));


      default:
         PrintErrorMessage(773);
         return(-1);
   }
}


              /** SPECIAL CALLBACK FOR EXTERNAL FUNCTIONS **/

short APIENTRY RxExternalCallback(LPSTR szFunctionName, short ArgCount,
                                  PRXSTRING Args, LPBYTE szQueueName,
                                  PRXSTRING lprxsReturn)
{
WORD i, j, w;


   if(SELECTOROF(lprxsReturn->strptr))
   {
      GlobalFreePtr(lprxsReturn->strptr);
   }

   for(i=0; i<N_DIMENSIONS(pExternalFunctions); i++)
   {
      if(_fstricmp(szFunctionName, pExternalFunctions[i])==0)
      {
         switch(i)
         {
            case 0:    /* CharOut */

               for(w=0, j=0; j<(WORD)ArgCount; j++)
               {
                  if(Args[j].strlength)
                  {
                     PrintBuffer(Args[j].strptr, (WORD)Args[j].strlength);
                     w += (WORD)Args[j].strlength;
                  }
               }

               lprxsReturn->strptr = GlobalAllocPtr(GMEM_MOVEABLE, 16);
               if(lprxsReturn->strptr)
               {
                  lprxsReturn->strlength =
                         wsprintf(lprxsReturn->strptr, "%d", w);
               }

               break;


            default:
               PrintErrorMessage(854);  // "?\""
               PrintString(pExternalFunctions[i]);
               PrintErrorMessage(855);  // "\" not yet supported\r\n\n"


         }

         return(0);
      }
   }

   lprxsReturn->strptr = NULL;
   lprxsReturn->strlength = 0;

   return(1);       /* an error - couldn't run function */
}

// per E-Mail 7/23/92
//
//       RXSIOCIN        6       // CharIN
//       RXSIOCOU        7       // CharOUT
//       RXSIOLIN        8       // LineIN
//       RXSIOLOU        9       // LineOUT


#define RXSIOCIN 6
#define RXSIOCOU 7
#define RXSIOLIN 8
#define RXSIOLOU 9


short FAR PASCAL RxSioCallBack(WORD wExitFunc, WORD wExitSubFunc,
                               LPBYTE lpParm)
{
PRXSTRING lprxS;
char *p1;
WORD spare_attr;


   lprxS = (PRXSTRING)lpParm;  /* each parm structure contains only an     */
                               /* RXSTRING element for the following:      */
                               /* RXSIOSAY, RXSIOTRC, RXSIOTRD, RXSIODTR   */

   if(wExitFunc==RXSIO)  /* then we're in the RIGHT PLACE! */
   {
      switch(wExitSubFunc)
      {
         case RXSIOCOU:
         case RXSIOLOU:
         case RXSIOSAY:   /* output to terminal */

            PrintBuffer(lprxS->strptr, (WORD)lprxS->strlength);
            if(wExitSubFunc!=RXSIOCOU) PrintString(pCRLF);
            return(0);


         case RXSIODTR:   /* 'debug' terminal input */

            spare_attr = cur_attr;

            if((spare_attr & 0xf0)!=0x10) /* not a RED background */
               cur_attr   = 0x1b;     /* bright yellow on red */
            else
               cur_attr   = 0x0b;     /* bright yellow on black */

            PrintErrorMessage(856);  // "REXX DEBUG TERMINAL INPUT:\r\n"
            cur_attr = spare_attr;    /* restore original attribute */


                /* continue forward into next section */

         case RXSIOCIN:
         case RXSIOLIN:
         case RXSIOTRD:   /* get input from terminal */


            if(wExitSubFunc==RXSIOCIN)
            {
               RxGetCharInput(work_buf);
               work_buf[1]=0;
            }
            else
            {
               GetUserInput(work_buf);

               p1 = work_buf + lstrlen(work_buf) - 1;

               if(p1>=work_buf && *p1=='\r' || *p1=='\n' || *p1=='\x1b')
                  *p1-- = 0;
               if(p1>=work_buf && *p1=='\r' || *p1=='\n' || *p1=='\x1b')
                  *p1-- = 0;
            }

            if(!lprxS->strptr)
            {
               lprxS->strlength = lstrlen(work_buf);

               lprxS->strptr = GlobalAllocPtr(GMEM_MOVEABLE, lprxS->strlength);
               if(!lprxS->strptr)
               {
                  PrintErrorMessage(781);  // "?Input was lost - not enough memory!\r\n\n"
                  return(-1);
               }
            }
            else if((WORD)lprxS->strlength <= (WORD)lstrlen(work_buf))
            {
               PrintErrorMessage(774);     // "Input truncated\r\n"
               _hmemcpy(lprxS->strptr, work_buf, lprxS->strlength);
            }
            else
            {
               lstrcpy(lprxS->strptr, work_buf);
               lprxS->strlength = lstrlen(work_buf);
            }

            return(0);


         case RXSIOTRC: /* TRACE message - assume an error took place here */

            spare_attr = cur_attr;

            if((spare_attr & 0xf0)!=0x10) /* not a RED background */
               cur_attr   = 0x1b;     /* bright yellow on red */
            else
               cur_attr   = 0x0b;     /* bright yellow on black */

            PrintErrorMessage(775);

                   /* inverse the foreground/background attribs */

            if((spare_attr & 0xf0)!=0) /* not a BLACK background */
            {
                 /* are foreground/background colors the same? */

               if((spare_attr & 0x07)!=((spare_attr & 0x70)>>4))
               {
                  cur_attr = (spare_attr & 0x08) + ((spare_attr & 0x70)>>4)
                           + ((spare_attr & 0x07)<<4);
               }
               else
               {
                  cur_attr = (spare_attr & 0x08) + ((spare_attr & 0x70)>>4)
                           + (((spare_attr & 0x07)<<4)^0x07);
               }
            }
            else
            {
               if(spare_attr & 0x07)
                  cur_attr = ((spare_attr & 0x07)<<4);  // black on f/g color
               else
                  cur_attr = 0x70;                     // black on light grey
            }

            PrintBuffer(lprxS->strptr, (WORD)lprxS->strlength);
            PrintString(pCRLF);

            cur_attr = spare_attr;

            IsHalt = TRUE;  /* this ensures that I don't repeatedly produce */
                            /* error message after error message!           */
            return(0);


         default:
         {
          static const char REXXRUNBASED szFmt[]="REXX CALLBACK - RxSioCallBack() %d,%d,%x:%x\r\n";

            wsprintf(work_buf, szFmt, wExitFunc, wExitSubFunc,
                     HIWORD(lpParm), LOWORD(lpParm));

            PrintString(work_buf);
            return(0);
         }

      }
   }
   else
   {
      PrintErrorMessage(776);
      return(0);  /* bad */
   }
}



short FAR PASCAL RxExtCallBack(WORD wExitFunc, WORD wExitSubFunc,
                               LPBYTE lpParm)
{
RXFNCCAL_PARM FAR *lpP;


   lpP = (RXFNCCAL_PARM FAR *)lpParm;

   switch(wExitSubFunc)
   {



      default:
         return(1);  /* not processed flag */
   }

   return(0);        /* 'was processed' flag */

}



short FAR PASCAL RxCmdCallBack(WORD wExitFunc, WORD wExitSubFunc,
                               LPBYTE lpParm)
{
RXCMDHST_PARM FAR *lpP;
LPSTR lp1;
BOOL rval;

   lpP = (RXCMDHST_PARM FAR *)lpParm;

   if(wExitFunc==RXCMD && wExitSubFunc==RXCMDHST)
   {
      if((WORD)lpP->rxcmd_addressl!=(WORD)lstrlen(pWINCMD) ||
         _fstrnicmp(lpP->rxcmd_address, pWINCMD, lpP->rxcmd_addressl)!=0)
      {
         return(1);  /* this environment did NOT process the thing! */
      }
         
      lp1 = GlobalAllocPtr(GMEM_MOVEABLE, lpP->rxcmd_command.strlength + 1);
      if(!lp1)
      {
         PrintErrorMessage(778);
         return(1);
      }

      _hmemcpy(lp1, lpP->rxcmd_command.strptr, lpP->rxcmd_command.strlength);

      lp1[lpP->rxcmd_command.strlength] = 0;

      /* here's the dangerous part - if I am IN BATCH then I cannot */
      /* execute another BATCH program without hanging.  SO, I must */
      /* perform an operation similar to what 'FOR' does...         */

      IsRexx = TRUE;    /* forces new batch file to complete first */
                        /* note that I do *NOT* use 'call'!        */

      rval = ProcessCommand(lp1);

      IsRexx = FALSE;           /* I'm done now! */


      GlobalFreePtr(lp1);
      {
//         if(!lpP->rxcmd_retc.strptr)
//         {
            lpP->rxcmd_retc.strptr = GlobalAllocPtr(GMEM_MOVEABLE, 1);
//         }

         if(lpP->rxcmd_retc.strptr)
         {
            *(lpP->rxcmd_retc.strptr) = (BYTE)('0' + rval!=0?1:0);
            lpP->rxcmd_retc.strlength = 1;
         }
         else
         {
            PrintErrorMessage(779);
         }
      }

      return(0);
   }
   else
   {
    static const char REXXRUNBASED szFmt[]="REXX CALLBACK - RxCmdCallBack() %d,%d,%x:%x\r\n";

      wsprintf(work_buf, szFmt, wExitFunc, wExitSubFunc,
               HIWORD(lpParm), LOWORD(lpParm));

      PrintString(work_buf);
      return(0);
   }

}



short FAR PASCAL RxHaltCallBack(WORD wExitFunc, WORD wExitSubFunc,
                                LPBYTE lpParm)
{
RXHLTTST_PARM FAR *lpJunk;
static DWORD dwLastTime=NULL;  /* the 'last time' I yielded via this */


   lpJunk = (RXHLTTST_PARM FAR *)lpParm;

   if(wExitFunc==RXHLT)
   {
      switch (wExitSubFunc)
      {
         case RXHLTTST:

              /* go for at least .1 seconds before yielding */

            if((GetCurrentTime() - dwLastTime)>=100L)
            {             /* perform necessary yielding */

                  /* ctrl-C, ctrl-Break, or kill thread? */

               if(LoopDispatch())
               {
                  IsHalt = TRUE;
               }
               else if(ctrl_break)  /* user pressed ctrl-break, ok? */
               {
                  if(MessageBox(hMainWnd,
                                "?Ctrl-C/Ctrl-Break pressed - continue?",
                                "** RXSHELL/REXX COMMAND **",
                                MB_YESNO | MB_ICONHAND | MB_SYSTEMMODAL)
                     !=IDYES)
                  {
                     IsHalt = TRUE;
                  }

                  ctrl_break = FALSE; /* >I< eat the ctrl break this time */
               }

               dwLastTime = GetCurrentTime(); /* prepare for next yield */
            }

            lpJunk->rxhlt_flags.rxfhhalt = IsHalt;

            return(0);


         case RXHLTCLR:

            IsHalt = FALSE;
            return(0);


         default:
         {
          static const char REXXRUNBASED szFmt[]="RxCmdCallBack() %d,%d,%x:%x\r\n";

            wsprintf(work_buf, szFmt, wExitFunc, wExitSubFunc,
                     HIWORD(lpParm), LOWORD(lpParm));

            PrintString(work_buf);
            return(0);
         }

      }

   }
   else
   {
      PrintErrorMessage(780);
      return(0);
   }

}



BOOL FAR _cdecl _multiRxExitRegister(WORD wCount, FARPROC lpProc, ...)
{
struct _args_ {
   FARPROC lpProc;
   LPSTR lpNames[1];
   } FAR *lpArgs;
SCBLOCK scb;
WORD i, j, wRC;


   lpArgs = (struct _args_ FAR *)(&lpProc);


   for(i=0; i<wCount; i++)
   {
      scb.scbname      = lpArgs->lpNames[i];
      scb.scbdll_name  = NULL;
      scb.scbdll_proc  = NULL;
      scb.scbuser      = 0xffffeeee;  /* ???? */

      scb.scbaddr      = lpProc;
      scb.scbdrop_auth = RXSUBCOM_NONDROP;

      wRC = lpRxExitRegister((PSCBLOCK)&scb);

      if(!RxSubComRegistrationOK(wRC, scb.scbname))
      {
         for(j=i; j>0; j--)     /* free all of the other assigned exits */
         {
            lpRxExitDrop(lpArgs->lpNames[j - 1], NULL);
         }

         return(TRUE);  /* bad bad bad!! */
      }
   }

   return(FALSE);  /* everything's ok! */

}

BOOL FAR PASCAL RxSubComRegistrationOK(WORD wRegisterRC,LPSTR lpFuncName)
{
   switch (wRegisterRC)
   {
      case RXFUNC_OK :
         return(TRUE);

      case RXFUNC_DEFINED :
      {
       static const char REXXRUNBASED szFmt[]="!Warning - function %s already registered with WREXX.";

         wsprintf(work_buf, szFmt, lpFuncName);

         PrintString(work_buf);
         return(TRUE);
      }

      case RXFUNC_NOMEM :
      case RXFUNC_NOTREG :
      case RXFUNC_MODNOTFND :
      case RXFUNC_ENTNOTFND :
      default:
      {
       static const char REXXRUNBASED szFmt[]="?Unable to Register Function %s with WREXX.";

         wsprintf(work_buf, szFmt, lpFuncName);

         PrintString(work_buf);
         return(FALSE);
      }


   }

}


/***************************************************************************/
/*     The next function was stolen from 'GetUserInput' to get a char      */
/***************************************************************************/

WORD FAR PASCAL RxGetCharInput(LPSTR lpBuf)
{

     /** If there is "Type Ahead" force contents into 'lpBuf' first **/

   *lpBuf = 0;

   while(TypeAheadHead==TypeAheadTail)
   {
      if(LoopDispatch() || ctrl_break)  /* a QUIT message, by the way... */
      {
         if(ctrl_break) ctrl_break = FALSE;
         return(0);
      }
   }

   if(TypeAheadHead!=TypeAheadTail)
   {
      if(TypeAheadHead>=sizeof(TypeAhead)) TypeAheadHead=0;

      PrintAChar(*lpBuf = TypeAhead[TypeAheadHead++]);

      lpBuf[1] = 0;  /* ensure it's end of string at this point */

      if(TypeAheadHead>=sizeof(TypeAhead)) TypeAheadHead=0;

      UpdateWindow(hMainWnd);  /* make sure the input is updated! */
   }

   return(lstrlen(lpBuf));           /* total number of bytes - awright! */

}
