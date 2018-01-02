/***************************************************************************/
/*                                                                         */
/*   WINCMD_B.C - Command line interpreter for Microsoft(r) Windows (tm)   */
/*                                                                         */
/*                      TRADE SECRET - CONFIDENTIAL                        */
/*                                                                         */
/*             The compiled libraries and API header files are             */
/*          (c) Copyright 1990-1995 by Stewart~Frazier Tools, Inc          */
/*                                                                         */
/*   This contains certain 'COMMAND' processing functions, specifically    */
/*   those which perform jumps, loops, conditional things, and 'wait's.    */
/*                                                                         */
/***************************************************************************/


#define THE_WORKS
#include "mywin.h"

#include "mth.h"
#include "wincmd.h"         /* 'dependency' related definitions */
#include "wincmd_f.h"       /* function prototypes and variables */


extern const char szRETVAL[];
extern const char szPATH[];


BOOL FAR PASCAL FindNextMatchingSection(CODE_BLOCK FAR *lpCB);



extern HMODULE hModule;



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



#define LockCode() { _asm push cs _asm call LockSegment }
#define UnlockCode() { _asm push cs _asm call UnlockSegment }


#define CHICAGO_MIN_VER 0x350 /* windows 3.8 */

// The following macro uses a method *DIFFERENT* than the 'IsChicago' var

#define IS_CHICAGO() (iswap(LOWORD(GetVersion())) >= CHICAGO_MIN_VER)




/***************************************************************************/
/*                                                                         */
/*             ** COMMAND PROCESSING - MAIN PARSER FUNCTION **             */
/*                                                                         */
/***************************************************************************/



int NEAR PASCAL GetCommandIndex(LPCSTR lpCmd);
BOOL FAR PASCAL SetCMDError(BOOL val);



#pragma code_seg("PROCESS_COMMAND_TEXT","CODE")


BOOL FAR PASCAL SetCMDError(BOOL val)
{
static char pCMD_ERROR[]="CMD_ERROR";
char valstr[2];



   valstr[0] = val?'1':0;
   valstr[1] = 0;


   SetEnvString(pCMD_ERROR, valstr); // later I might include more info...


   return(val);
}


int NEAR PASCAL GetCommandIndex(LPCSTR lpCmd)
{
int i, b, m, t;


   b = 0;
   t = wNcmd_array - 1;

   while(b<=t)
   {

      m = (b + t) / 2;

      i = _fstrcmp(lpCmd, cmd_array[m]);

      if(i==0)        return(m);
      else if(i<0)    t = m - 1;
      else            b = m + 1;

   }

   return(-1);

}


BOOL FAR PASCAL ProcessCommand(LPSTR lpCmd)
{
LPSTR lp1,lp2,lp3;
char old_lp2;              // special place to save data...
BOOL atsign;
int i;
static char pECHO1[]="ECHO.";
static char pECHO[]="ECHO";
static BOOL I_Have_Checked_Command_Order_Already=FALSE;



   atsign = FALSE; // flag which (when in batch mode) prevents echo

   if(!I_Have_Checked_Command_Order_Already)
   {
      for(i=1; i < (int)wNcmd_array; i++)
      {
         if(_fstricmp(cmd_array[i],cmd_array[i - 1])<0)
         {
            PrintErrorMessage(486);  // "?Internal error - command table corrupt!\r\n\n"

            wsprintf(work_buf, "Temporary:  i == %d\r\n", i);
            PrintString(work_buf);
            PrintString(cmd_array[i]);
            PrintString(" , ");
            PrintString(cmd_array[i - 1]);
            PrintString(pCRLFLF);

            PrintErrorMessage(260);
            PrintErrorMessage(325);

            PostMessage(hMainWnd, WM_QUIT, 0, 0);
            return(TRUE);
         }
      }

      // since the commands are 'in order', flag that I've checked them and
      // proceed to the interpreter!

      I_Have_Checked_Command_Order_Already = TRUE;

   }


   for(lp1=lpCmd; *lp1==' ' || *lp1=='\x9'; lp1++)
    ;                                             /* find first non-space */


   if(*lp1=='@' && *(lp1+1)!=0)
   {
      atsign = TRUE;             /* set flag indicating the '@' was there */
      lp1++;                       /* ignore the leading '@' when parsing */
   }
   else if(*lp1=='%')            /* self-interpreting command! */
   {
      lp2 = EvalString(lp1);     // get 'evaluated' version of string

      if(_fstrcmp(lp1, lp2))     // that is, the evaluating did something
      {
       BOOL rval;

         // for fun, put limited capability to handle multiple lines in here!

         rval = FALSE;
         lpCmd = lp2;            // save ptr so I can de-allocate it

         while(*lp2)
         {
            lp1 = _fstrchr(lp2, '\r');
            lp3 = _fstrchr(lp2, '\n');
            if(lp3 && (!lp1 || lp3<lp1)) lp1 = lp3;

            if(lp1)
            {
               *(lp1++) = 0;
               while(*lp1 && *lp1<=' ') lp1++;

            }
            else
            {
               lp1 = lstrlen(lp2) + lp2;
            }

            lp3 = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                        64 + lstrlen(lp2));  // room to expand

            if(!lp3)
            {
             HCURSOR hCursor;

               hCursor = MySetCursor(LoadCursor(NULL, IDC_WAIT));

               GlobalCompact((DWORD)-1L);

               lp3 = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                           64 + lstrlen(lp2));  // room to expand

               MySetCursor(hCursor);
            }


            if(!lp3)
            {
               PrintString(pMEMORYLOW);

               rval |= ProcessCommand(lp2);
            }
            else
            {
               lstrcpy(lp3, lp2);

               rval |= ProcessCommand(lp3);

               GlobalFreePtr(lp3);
            }

            lp2 = lp1;
         }

         GlobalFreePtr(lpCmd);

         return(rval);
      }
      else
      {
         GlobalFreePtr(lp2);
      }
   }


   // check for quoted commands (assume 'long name' for program)

   if(*lp1=='\"')  // name begins with quote mark!  *ALWAYS* a program!
   {
      lp1++;
      for(lp2=lp1; *lp2; lp2++)
      {
         if(*lp2 == '\"')  // found a quote mark!
         {

            lp3 = lp2 + 1;

            while(*lp3 && *lp3 <= ' ') lp3++;

            break;
         }
      }

      if(!*lp2) lp3 = lp2;
      else      *lp2 = 0;

      return(SetCMDError(CMDRunProgram(lp1,lp3,SW_SHOWNORMAL)));
   }


   // non-quoted command - procede normally

   for(lp2=lp1; *lp1!=0 && *lp2>' '; lp2++)
    ;  /* find next white space char */

   for(lp3=lp2; *lp1!=0 && *lp2!=0 && (*lp3==' ' || *lp3=='\x9'); lp3++)
    ;  /* find next non-space char (start of 1st command arg) */


   old_lp2 = *lp2;  // saves the value in here for later...
   *lp2 = 0;  /* terminates 1st arg pointed at by 'lp1' */

   if(*lp1==0)  return(FALSE);     /* empty string - return!! */



    /********************************************************************/
    /** First set of special cases:  look for 'CD\\{etc.}','CD.{etc.}' **/
    /********************************************************************/

   if(_fstricmp(lp1, "CD.")==0)   // 'CD.' - prints current dir (that's all!)
   {
      return(SetCMDError(CMDChdir(lp1,lp3)));
   }
   else if((_fstrnicmp(lp1, "CD.", 3)==0 || _fstrnicmp(lp1,"CD\\",3)==0)
            && *lp3==0)             /* only 1 command on line - no others! */
   {
      lstrcpy(work_buf, lp1+2);
      return(SetCMDError(CMDErrorCheck(MyChDir(work_buf))));
                                                         /* change directories  */
   }

               /**********************************************/
               /** Next set of special cases: '?' for CALC. **/
               /**********************************************/

   else if((lstrlen(lp1)==1 && *lp1=='?') ||
           (lstrlen(lp1)==2 && *lp1=='?' && lp1[1]=='?'))
   {
      return(SetCMDError(CMDCalc(lp1, lp3)));
   }


               /**********************************************/
               /** Next set of special cases: ';' for REM   **/
               /**********************************************/

   else if(*lp1==';')
   {
      *lp2 = old_lp2;  // restore the character which *WAS* there

      return(SetCMDError(CMDRem("REM", lp1 + 1)));
   }

               /**********************************************/
               /** Next set of special cases:  Drive Change **/
               /**********************************************/

   else if(lstrlen(lp1)==2 && lp1[1]==':' &&
           ((*lp1>='a' && *lp1<='z') || (*lp1>='A' && *lp1<='Z')))
   {
      return(SetCMDError(CMDErrorCheck(_chdrive(toupper(*lp1) - 'A' + 1))));
                                                         /* change drives! */
   }
               /**********************************************/
               /** Next set of special cases: Path=/Prompt= **/
               /**********************************************/

   else if(_fstrnicmp(lp1, "PATH=", 5)==0)
   {
      *lp2 = old_lp2;  // restore the character that *WAS* there

      return(SetCMDError(CMDPath((LPSTR)szPATH,lp1 + 4)));
   }
   else if(_fstrnicmp(lp1, "PROMPT=", 7)==0)
   {
      *lp2 = old_lp2;  // restore the character that *WAS* there

      return(SetCMDError(CMDPrompt("PROMPT",lp1 + 6)));
   }

                /*****************************************/
                /** Next special case: 'ECHO.', 'ECHO ' **/
                /*****************************************/

   else if(_fstrnicmp(lp1, pECHO1, 5)==0)
   {
        // assume that 'lp2' still points to the first white-space
        // following the 'ECHO' command, and 'old_lp2' is the value
        // that used to be there (and it's now a NULL byte)

      *lp2 = old_lp2;  // restore the character that *WAS* there

      return(SetCMDError(CMDEcho(pECHO1,lp1 + 5)));
                                              // the distinction is important
   }
   else if(_fstrnicmp(lp1, pECHO, 4)==0 && old_lp2 && old_lp2<=' ')
   {
      *lp2 = old_lp2;  // restore the character that *WAS* there

      return(SetCMDError(CMDEcho(pECHO,lp1 + 5)));
   }

                 /****************************************/
                 /** Next special case: 'IF(' vs 'IF (' **/
                 /****************************************/

   else if(!_fstrnicmp(lp1, "IF(", 3))
   {
      return(SetCMDError(CMDIf("IF", lp1 + 2)));
   }

                   /**************************************/
                   /*** Look through array of commands ***/
                   /**************************************/
   else
   {
    LPSTR lpSlash;
    /* static */ char cmd_temp[32];       // no command is larger than 32 bytes!

      lpSlash = _fstrchr(lp1, '/');  /* if there's a '/' */

      if(lpSlash)  *lpSlash = 0;   /* temporary */

      _fstrncpy(cmd_temp, lp1, sizeof(cmd_temp) - 1);
      cmd_temp[sizeof(cmd_temp) - 1] = 0;

      _fstrupr(cmd_temp);


//      for(i=0; i<(int)wNcmd_array; i++)
//      {
//         if(strcmp(cmd_array[i], cmd_temp)==0)  // use NEAR pointers now!
//         {
//            if(lpSlash)          // 'COMMAND'/ (with no space) was found
//            {
//               *lpSlash = '/';            /* restore contents for switch */
//               lstrcpy(work_buf, lpSlash);  /* make copy in 'work_buf' */
//               lstrcat(work_buf, " ");         /* rebuild arguments */
//               lstrcat(work_buf, lp3);
//
//               lstrcpy(lp3, work_buf);/* copy reformatted args back to line */
//                                        /* it MUST be able to handle it! */
//               *lpSlash = 0;      /* back to 0 again to mark end of command */
//            }
//
//            return(SetCMDError(fn_array[i](lp1, lp3)));
//         }
//      }

      if((i = GetCommandIndex(cmd_temp))>=0) // this does a binary search...
      {
         if(lpSlash)          // 'COMMAND'/ (with no space) was found
         {
            *lpSlash = '/';            /* restore contents for switch */
            lstrcpy(work_buf, lpSlash);  /* make copy in 'work_buf' */
            lstrcat(work_buf, " ");         /* rebuild arguments */
            lstrcat(work_buf, lp3);

            lstrcpy(lp3, work_buf);/* copy reformatted args back to line */
                                     /* it MUST be able to handle it! */
            *lpSlash = 0;      /* back to 0 again to mark end of command */
         }

         // special case:  if argument consists of '/?' and nothing else
         //                perform 'HELP command' on it.

         if(*lp3 == '/' && lp3[1]=='?' && lp3[2] <= ' ')
         {
            // command line begins with '/?'

            return(SetCMDError(CMDHelp("HELP",lp1)));
         }
         else
         {
            return(SetCMDError(fn_array[i](lp1, lp3)));
         }
      }

      return(SetCMDError(CMDRunProgram(lp1,lp3,SW_SHOWNORMAL)));
                                                  /* last but not least... */
   }

   return(FALSE);                   // it should *NEVER* get here, by the way
}



#pragma code_seg()




                      /**************************/
                      /*** COMMAND LINE PARSE ***/
                      /**************************/


#define SBPFROMLP(X) ((char _based(void) *)((WORD)((DWORD)X)))

LPPARSE_INFO FAR PASCAL CMDLineParse(LPSTR lpCmdLine)
{
LPPARSE_INFO lpRval;
LPSTR lp1, lp2, lp3, lpHeap;
WORD u,v,u1,v1;


   lpCmdLine = EvalString(lpCmdLine);  /* evaluate all variables */
   if(!lpCmdLine) return((LPPARSE_INFO)NULL);

   lpRval = (LPPARSE_INFO)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                         256L + sizeof(PARSE_INFO) +
                                         lstrlen(lpCmdLine) +
                                         2 * MAX_PATH);

   if(!lpRval)
   {
      GlobalFreePtr(lpCmdLine);
      return((LPPARSE_INFO)NULL);
   }

   // initialize a few important elements at this time

   lpRval->hOutFile = HFILE_ERROR;
   lpRval->hInFile = HFILE_ERROR;

   lpRval->sbpPipeNext = NULL;       // "ensurance"



   /** Step 1:  How many switches are there? **/

   for(u=0, lp1=lpCmdLine; lp1 && *lp1; lp1++)
   {
      if(*lp1=='\"')  // quote mark?
      {
         lp1++;

         while(*lp1 && *lp1 != '\"') lp1++;
//         while(*lp1)
//         {
//            if(*lp1 == '\\')
//            {
//               lp1++;
//               if(*lp1) lp1++;
//            }
//            else if(*lp1 == '\"') break;
//            else if(*lp1)         lp1++;
//         }
      }
      else if(*lp1 == '|') // PIPING - break now!
      {
         break;
      }
      else if(*lp1 == '/')
      {
         u++;
      }
   }

   lpRval->n_switches = u;        /* o.k. got # of switches now!! */



   /** Step 2:  Get total number of parameters (that aren't switches) **/

   for(v=0, lp1=lpCmdLine; lp1!=(LPSTR)NULL && *lp1!=0; lp1 = lp3)
   {
      for(lp2=lp1; *lp2<=' ' && *lp2!=0; lp2++)
        ;               /* First, get the next 'non-white-space' character */
      if(*lp2==0) break;                /* we're done now!! */


      if(*lp2 == '\"')      // parameter begins with quote mark
      {
         lp3 = lp2 + 1;

         while(*lp3 && *lp3 != '\"') lp3++;

//         while(*lp3)
//         {
//            if(*lp3 == '\\')
//            {
//               lp3++;
//               if(*lp3) lp3++;
//            }
//            else if(*lp3 == '\"') break;
//            else if(*lp3)         lp3++;
//         }

         if(*lp3) lp3++;
      }
      else if(*lp2=='|')  // piping!
      {
         break;  // done with string at this point
      }
      else
      {
         for(lp3=lp2+1; *lp3>' ' && *lp3!='/' && *lp3!='<' && *lp3!='>';
             lp3++);        /* next, find the next 'white-space' character */
                            /* or switch or file indirection character     */
                            /* that follows where 'lp2' currently points.  */
      }

      if(*lp2=='/' || *lp2=='<' || *lp2=='>')/* slash or file re-direction */
         continue;          /* keep going - it's nothing to consider (now) */
      else
         v++;               /* ah-hah!  This is a valid PARAMETER!!        */

   }

   lpRval->n_parameters = v;        /* o.k. got # of parameters now!! */



   /** Step 3:  Now that we know how large the 'PARSE_INFO' structure **/
   /**          needs to be, copy the string into the return struct.  **/

   lpHeap = (LPSTR)( (lpRval->sbpItemArray) + u + v + 1 );

   lstrcpy(lpHeap, lpCmdLine);               /* that was easy, wasn't it? */
   GlobalFreePtr(lpCmdLine);
   lpCmdLine = NULL;                            /* no longer needed! */


   /** Step 4:  OK now we go back one... more... time!  This time we look **/
   /**          for each and every parm,switch,etc. and place NULLS at    **/
   /**          the end of each value, storing a pointer                  **/

   for(u1=0,v1=0, lp1=lpHeap; lp1!=(LPSTR)NULL && *lp1!=0; lp1 = lp3)
   {
      for(lp2=lp1; *lp2<=' ' && *lp2!=0; lp2++)
        ;               /* First, get the next 'non-white-space' character */

      if(*lp2==0) break;              /* we're done now!! */


      if(*lp2 == '\"')      // parameter begins with quote mark
      {
         lp3 = lp2 + 1;

         while(*lp3 && *lp3 != '\"') lp3++;

//         while(*lp3)
//         {
//            if(*lp3 == '\\')
//            {
//               lp3++;
//               if(*lp3) lp3++;
//            }
//            else if(*lp3 == '\"') break;
//            else if(*lp3)         lp3++;
//         }

         if(*lp3) *(lp3++) = 0;
      }
      else if(*lp2=='|')  // piping!
      {
         // find the command following the '|' and point 'sbpPipeNext'
         // to this command.  'ReDirectOutput' will have a field day
         // with it when called with 'FALSE' for 'redir_flag'.

         *(lp2++) = 0;
         while(*lp2 && *lp2 <= ' ') lp2++;

         lpRval->sbpPipeNext = SBPFROMLP(lp2);

         break;  // done with string at this point
      }
      else
      {
                /* next, find the next 'white-space' character */
                /* or switch or file indirection character     */
                /* that follows where 'lp2' currently points.  */

         for(lp3=lp2+1; *lp3>' ' && *lp3!='/' && *lp3!='|'; lp3++)
         {
            if(*lp3=='<')
            {
               break;  // '<<' is not allowed...
            }
            else if(*lp3=='>')
            {
               if(*lp2!='>' || lp3!=(lp2 + 1))  // check for '>>'
               {
                  break;  // not a valid '>>' ('>' terminates this entry)
               }
            }
         }
      }

      switch(*lp2)        /* let's see what character this item starts with */
      {
         case '>':         /* indirection to output file... */

            lpRval->sbpOutFile = SBPFROMLP(lp2 + 1);
            *lp2 = 0;      /* if previous had no white space, marks end! */
            break;


         case '<':         /* indirection from input file... */

            lpRval->sbpInFile  = SBPFROMLP(lp2 + 1);
            *lp2 = 0;      /* if previous had no white space, marks end! */
            break;


         case '/':

            lpRval->sbpItemArray[u1++] = SBPFROMLP(lp2 + 1);
            *lp2 = 0;      /* if previous had no white space, marks end! */
            break;


         case '\"':

            lpRval->sbpItemArray[u + (v1++)] = SBPFROMLP(lp2 + 1);
            *lp2 = 0;      /* if previous had no white space, marks end! */
            break;


         default:

            lpRval->sbpItemArray[u + (v1++)] = SBPFROMLP(lp2);

      }

      if(*lp3>0 && *lp3<=' ')     /* if the 'end of string' is white space */
         *(lp3++) = 0;                /* mark 'end of string' for element! */

   }

   return(lpRval);

}


LPPARSE_INFO FAR PASCAL CMDRunProgramCommandLineParse(LPSTR lpCmdLine)
{
LPPARSE_INFO lpRval;
LPSTR lp1, lp2, lp3, lpHeap, lpHeapEnd;



   lpCmdLine = EvalString(lpCmdLine);  /* evaluate all variables */
   if(!lpCmdLine) return((LPPARSE_INFO)NULL);

   lpRval = (LPPARSE_INFO)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                         256L + sizeof(PARSE_INFO) +
                                         lstrlen(lpCmdLine) +
                                         2 * MAX_PATH);

   if(!lpRval)
   {
      GlobalFreePtr(lpCmdLine);
      return((LPPARSE_INFO)NULL);
   }

   // initialize a few important elements at this time

   lpRval->hOutFile = HFILE_ERROR;
   lpRval->hInFile = HFILE_ERROR;

   lpRval->sbpPipeNext = NULL;       // "ensurance"


   // NO SWITCHES OR PARAMETERS!  Just look for redirection and piping...
   // There will be exactly ONE parameter here, and that's the whole command
   // line without the piping or redirection.

   lpRval->n_switches = 0;    // no switches
   lpRval->n_parameters = 1;  // 1 parameter (entire command)

   lpHeap = (LPSTR)(lpRval->sbpItemArray + 2);
   lstrcpy(lpHeap, lpCmdLine);               /* that was easy, wasn't it? */
   GlobalFreePtr(lpCmdLine);
   lpCmdLine = NULL;                            /* no longer needed! */

   lpHeapEnd = lpHeap + lstrlen(lpHeap) + 1; // this is where I store redir's


   lpRval->sbpItemArray[0] = SBPFROMLP(lpHeap);  // the arguments!


   for(lp1=lpHeap; lp1!=(LPSTR)NULL && *lp1!=0; lp1 = lp3)
   {
      for(lp2=lp1; *lp2<=' ' && *lp2!=0; lp2++)
        ;               /* First, get the next 'non-white-space' character */

      if(*lp2==0) break;              /* we're done now!! */


      if(*lp2 == '\"')      // parameter begins with quote mark
      {
         lp3 = lp2 + 1;
         while(*lp3 && *lp3 != '\"') lp3++;
         if(*lp3) lp3++;

         continue;  // skip remainder of loop - I found the end of the term
      }
      else if(*lp2=='|')  // piping!
      {
         // find the command following the '|' and point 'sbpPipeNext'
         // to this command.  'ReDirectOutput' will have a field day
         // with it when called with 'FALSE' for 'redir_flag'.

         *(lp2++) = 0;
         while(*lp2 && *lp2 <= ' ') lp2++;

         lpRval->sbpPipeNext = SBPFROMLP(lp2);

         break;  // done with string at this point
      }
      else
      {
                /* next, find the next 'white-space' character */
                /* or switch or file indirection character     */
                /* that follows where 'lp2' currently points.  */

         for(lp3=lp2+1; *lp3>' ' && *lp3!='|'; lp3++)
         {
            if(*lp3=='<')
            {
               break;  // '<<' is not allowed...
            }
            else if(*lp3=='>')
            {
               if(*lp2!='>' || lp3!=(lp2 + 1))  // check for '>>'
               {
                  break;  // not a valid '>>' ('>' terminates this entry)
               }
            }
         }
      }

      switch(*lp2)        /* let's see what character this item starts with */
      {
         case '>':         /* indirection to output file... */

            if(lp3 > (lp2 + 1))
            {
               _hmemcpy(lpHeapEnd, lp2 + 1, lp3 - lp2 - 1); // copy file name
            }

            lpHeapEnd[lp3 - lp2 - 1] = 0;

            lpRval->sbpOutFile = SBPFROMLP(lpHeapEnd);
            lpHeapEnd += lp3 - lp2;  // same as 'lstrlen(lpHeapEnd) + 1'

            if(lp2 == lpHeap || *(lp2 - 1)<=' ')
            {
               while(*lp3 && *lp3 <= ' ') lp3++;  // trim excess white space
            }

            lstrcpy(lp2, lp3);  // WIPE OUT this parameter

            lp3 = lp2;          // make sure 'end of term' is corrected
            break;


         case '<':         /* indirection from input file... */

            if(lp3 > (lp2 + 1))
            {
               _hmemcpy(lpHeapEnd, lp2 + 1, lp3 - lp2 - 1); // copy file name
            }

            lpHeapEnd[lp3 - lp2 - 1] = 0;

            lpRval->sbpInFile = SBPFROMLP(lpHeapEnd);
            lpHeapEnd += lp3 - lp2;  // same as 'lstrlen(lpHeapEnd) + 1'

            if(lp2 == lpHeap || *(lp2 - 1)<=' ')
            {
               while(*lp3 && *lp3 <= ' ') lp3++;  // trim excess white space
            }

            lstrcpy(lp2, lp3);  // WIPE OUT this parameter

            lp3 = lp2;          // make sure 'end of term' is corrected
            break;

      }
   }


   if(lpRval->sbpPipeNext || PipingFlag)
   {
      // WE ARE PIPING!  Force an output file, whether or not one
      // was specified!!  All piped output goes to the screen unless
      // specifically directed elsewhere...

      if(!lpRval->sbpOutFile) // no '>' file has (yet) been assigned...
      {
         lpRval->sbpOutFile = SBPFROMLP(lpHeapEnd);

         // create temp file name using 'lpRval' selector

         GetTempFileName(0, "SFT", 0, lpHeapEnd);

         lpHeapEnd += lstrlen(lpHeapEnd) + 1;

         if(!lpRval->sbpPipeNext)
         {
            lpRval->sbpPipeNext = lpHeapEnd;  // flags temporary output file
                                              // and forces output to screen!
         }
      }

   }


   return(lpRval);

}



/***************************************************************************/
/*                                                                         */
/*              *** PROCESS SPAWNING/INITIATING FUNCTIONS ***              */
/*                                                                         */
/***************************************************************************/



BOOL FAR PASCAL CMDRunErrorMessage(WORD wRval)
{

   switch(wRval)
   {
      case 0:
         PrintString(pNOTENOUGHMEMORY);
         return(TRUE);
      case 10:
         PrintErrorMessage(528);
         return(TRUE);
      case 15:
         PrintErrorMessage(529);
         return(TRUE);

      case 5:
      case 6:
      case 12:
      case 13:
      case 14:
         PrintErrorMessage(530);
         return(TRUE);

      case 16:
         PrintErrorMessage(531);
         return(TRUE);
      case 17:
      case 18:
         PrintErrorMessage(532);
         return(TRUE);
   }

   return(FALSE);                   /* this means 'non-fatal error' */
                                    /* (it's safe to try DOS app!)  */
}


/***************************************************************************/
/*                                                                         */
/*   CMDRunProgram - function which loads and executes WINDOWS and DOS     */
/*                   programs.  Searches PATH and Windows dir's in the     */
/*                   same manner as 'OpenFile', 'WinExec', etc.  Passes    */
/*                   current DOS environment to new prog (RESERVED).       */
/*                                                                         */
/***************************************************************************/

static DWORD dwCMDRunProgramRetCode = (DWORD)-1L;

BOOL FAR PASCAL EXPORT CMDRunProgramNotifyCallback(WORD wID, DWORD dwData)
{
   if(wID == NFY_EXITTASK)  // task is terminating
   {
      dwCMDRunProgramRetCode = dwData;

//      if(!IS_CHICAGO())
//      {
//         dwCMDRunProgramRetCode &= 0xffff; // only low word is valid...
//      }

      return(1);  // handled by me
   }
   else if(wID == NFY_STARTTASK)
   {
      return(1);  // eat the message (just because)
   }

   return(0); // let others handle it also!
}


BOOL FAR PASCAL CMDRunProgram(LPSTR lpCmd, LPSTR lpArgs, int nCmdShow)
{
LPPARSE_INFO lpParseInfo;
LPSTR lp1, lpEnv;
WORD  wLen, wRval;
int i;
char temp_buf[MAX_PATH * 2];
HFILE hStdIn, hStdOut, hOldStdIn, hOldStdOut, hOldStdErr;
HCURSOR hOldCursor;
HTASK hNewTask;
HMODULE hNewModule;
static OFSTRUCT ofstr, ofstr2;
static TASKENTRY te;
static DOSRUNPARMBLOCK ParameterBlock;
static DOSRUNCMDSHOW CmdShow;



   dwLastProcessID = 0;

   wRval = 0;    /* initialize to zero so if file not found it's an error */
   DelEnvString(pTASK_ID);

   hOldCursor = MySetCursor(LoadCursor(NULL, IDC_WAIT));        /* hourglass */

   _fmemset((LPSTR)temp_buf, 0, sizeof(temp_buf)); /* just to make sure... */
   _fmemset((LPSTR)work_buf, 0, sizeof(work_buf));         /* same here... */



   lp1 = _fstrchr(lpCmd, '/');  /* looking for a slash! */

   if(lp1==(LPSTR)NULL)                      /* no slash!  handle normally */
   {
      lstrcpy(work_buf, lpCmd);
      lpParseInfo = CMDRunProgramCommandLineParse(lpArgs);
   }
   else
   {
      _hmemcpy(work_buf, lpCmd, lp1 - lpCmd);
      work_buf[lp1 - lpCmd] = 0;

      lstrcat((LPSTR)temp_buf, lp1);               /* the switch!! */
      lstrcat((LPSTR)temp_buf, " ");            /* a space to separate */
      lstrcat((LPSTR)temp_buf, lpArgs);

      lpParseInfo = CMDRunProgramCommandLineParse((LPSTR)temp_buf);
   }

   if(!lpParseInfo)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);  // error (can't run program)
   }


   // build a 'DOS COMPATIBLE' command line...

   lstrcpy((LPSTR)temp_buf, "  ");  // placeholder for length info + lead ' '
   lstrcat((LPSTR)temp_buf, (LPSTR)lpParseInfo->sbpItemArray[0]); // cmd line


   // at this point, 'lpParseInfo' contains redirection and piping
   // information.  At this point, open the new STDIN and STDOUT files
   // (as needed) and redirect existing STDIN and STDOUT to match

   hStdIn = hStdOut = hOldStdIn = hOldStdOut = hOldStdErr = HFILE_ERROR;

   if(lpParseInfo->sbpInFile && *lpParseInfo->sbpInFile)
   {
      hStdIn = _lopen(lpParseInfo->sbpInFile, READ | OF_SHARE_DENY_NONE);
      if(hStdIn != HFILE_ERROR)
      {
         hOldStdIn = MyDupFileHandle(0);  // duplicate stdin

         MyAssignFileHandle(0, hStdIn);
      }
      else
      {
         PrintString("?Warning - input 'pipe' file not opened\r\n");
      }
   }


   if(lpParseInfo->sbpOutFile && *lpParseInfo->sbpOutFile)
   {
    LPSTR lpOutFile = (LPSTR)lpParseInfo->sbpOutFile;
    BOOL bAppend = FALSE;

      if(*lpOutFile == '>')  // append???
      {
         bAppend = TRUE;  // flag 'append' and don't create unless I have to

         hStdOut = _lopen(lpOutFile, READ_WRITE | OF_SHARE_DENY_NONE);

         if(hStdOut == HFILE_ERROR)
         {
            hStdOut = _lcreat(lpOutFile, 0);
            _lclose(hStdOut);

            hStdOut = _lopen(lpOutFile, READ_WRITE | OF_SHARE_DENY_NONE);
         }
      }
      else
      {
         hStdOut = _lcreat(lpOutFile, 0);
         _lclose(hStdOut);

         hStdOut = _lopen(lpOutFile, READ_WRITE | OF_SHARE_DENY_NONE);
      }

      if(hStdOut != HFILE_ERROR)
      {
         // are we APPENDING?

         if(bAppend)
         {
            _llseek(hStdOut, 0, SEEK_END);  // point to end of file
         }

         hOldStdOut = MyDupFileHandle(1); // duplicate stdout
         hOldStdErr = MyDupFileHandle(2); // duplicate stderr

         MyAssignFileHandle(1, hStdOut);
         MyAssignFileHandle(2, hStdOut);
      }
      else
      {
         PrintString("?Warning - output 'pipe' file not opened\r\n");
      }
   }


   if(lpParseInfo->sbpPipeNext)  // we're PIPING
   {
      PipingFlag++;  // this will *ALWAYS* force executed programs to be
                     // "called" instead of run asynchronously
   }



   // set up command line as appropriate to the current command...

   *temp_buf = (char)lstrlen((LPSTR)temp_buf + 1);  /* length is 1st byte */
   lstrcat((LPSTR)temp_buf + 1, "\x0d");           /* terminate with <CR> */

   ParameterBlock.wEnvSeg = 0;                     // initial value

   ParameterBlock.lpCmdLine = (LPSTR)temp_buf;

   ParameterBlock.lpCmdShow = &CmdShow;
   ParameterBlock.lpReserved = (LPSTR)NULL;

   CmdShow.wTwo = 2;
   CmdShow.nCmdShow = nCmdShow;  /* the 'ShowWindow()' parm passed to prog */


   i = FindProgram(work_buf, (LPOFSTRUCT)&ofstr);

#if 1     /* do not compile this section of code until WINDOWS is fixed!! */
          /* a value of '1' does environment block stuff...               */

   if(i && iswap((WORD)GetVersion()) >= 0x332)
   {  // valid program and NT 3.5 or later - process environment block!!!

      // TO PREVENT PROBLEMS, I must add the cmd line args and program name
      // to the end of the environment area.  Howze about DEM apples, eh??

      lpEnv = GetMyEnvironment();
      wLen = (WORD)GlobalSizePtr(lpEnv);

//      lp1 = GlobalAllocPtr(GMEM_FIXED | GMEM_DDESHARE | GMEM_LOWER,
//                           wLen + 256);

      lp1 = (LPSTR)MAKELP(LOWORD(GlobalDosAlloc(wLen + 256)), 0);

      ParameterBlock.wEnvSeg = SELECTOROF(lp1);



      if(lp1)
      {
         _hmemcpy(lp1, lpEnv, wLen);

         while(*lp1) lp1 += lstrlen(lp1) + 1;  // move 'lp1' to end of environment
         lp1++;                                // point past '\0\0' sequence

         *((WORD FAR *)lp1) = 1;               // # of strings that follow

         lp1 += sizeof(WORD);         // this is where the program PATH goes!

         lstrcpy(lp1, ofstr.szPathName);      // program path name goes here!

         lp1 += lstrlen(lp1) + 1;             // point just past string

         _fmemcpy(lp1, temp_buf + 1, *temp_buf);  // 'prep'ped command line!
         lp1[*temp_buf] = 0;
         lp1 += lstrlen(lp1) + 1;
         *lp1 = 0;                            // 2 NULLS in a row mark the end
      }
   }

#endif

   if(i>0)  /* this is a WINDOWS EXE file! */
   {
      if(i & PROGRAM_ASSOCIATION)
      {
         _hmemset(&ofstr2, 0, sizeof(ofstr2));

         if(FileExists(lpCmd))
         {
            lstrcpy(ofstr2.szPathName, lpCmd);
            _fstrtrim(ofstr2.szPathName);
         }
         else if(MyOpenFile(lpCmd, &ofstr2, OF_EXIST | OF_READ | OF_SHARE_DENY_NONE)
                 == HFILE_ERROR &&
                 MyOpenFile(lpCmd, &ofstr2, OF_EXIST | OF_READ | OF_SHARE_COMPAT)
                 == HFILE_ERROR)
         {
            *ofstr2.szPathName = 0;
         }

         if(!*ofstr2.szPathName)
         {
            wRval = 2;  /* cannot find this file!! */
         }
         else if(!lpShellExecute)  // that is, the SHELL.DLL is not loaded
         {
            lstrcpy(temp_buf, "  ");
            DosQualifyPath(temp_buf + 2, ofstr2.szPathName);

            if(!temp_buf[2])  // if the resulting path is BLANK!
            {
               wRval = 2;   /* cannot find file! */
            }
            else
            {
               if(*lpArgs)
               {
                  lstrcat(temp_buf, " ");
                  lstrcat(temp_buf, lpArgs);
               }
               *temp_buf = (char)lstrlen((LPSTR)temp_buf + 1);
                                                        /* length in 1st byte */

//               ParameterBlock.wEnvSeg = 0;
               wRval = (WORD)LoadModule(ofstr.szPathName, (LPSTR)&ParameterBlock);
            }
         }
         else  // until I can render the DDE part correctly...
         {
            DosQualifyPath(temp_buf, ofstr2.szPathName);

            if(*temp_buf)
            {
               wRval = (WORD)lpShellExecute(hMainWnd, "open", temp_buf,
                                            lpArgs, ".", nCmdShow);
            }
            else
            {
               wRval = 2; /* 'cannot find file' error */
            }

         }
      }
      else if(i & REXX_COMMAND)
      {
         DosQualifyPath(temp_buf + 2, ofstr.szPathName);
         if(!*temp_buf)
         {
            wRval = 2;    /* 'cannot find file' error */
         }
         else
         {
            if(IsStart)  // 'START' command - must run in separate window!
            {
               lstrcpy(temp_buf, "  ");
               DosQualifyPath(temp_buf + 2, ofstr2.szPathName);

               if(!temp_buf[2])  // if the resulting path is BLANK!
               {
                  wRval = 2;   /* cannot find file! */
               }
               else
               {
                  if(*lpArgs)
                  {
                     lstrcat(temp_buf, " ");
                     lstrcat(temp_buf, lpArgs);
                  }

                  *temp_buf = (char)lstrlen((LPSTR)temp_buf + 1);
                  wRval = (WORD)LoadModule(pPROGRAMNAME,
                                           (LPSTR)&ParameterBlock);
               }
            }
            else
            {
               lstrcat(temp_buf, " ");
               lstrcat(temp_buf, lpArgs);

               MySetCursor(hOldCursor);  /* restore cursor before running */
               hOldCursor = NULL;

               wRval = RexxRun(temp_buf, FALSE)!=0 ? 2 : 0xffff;
                         /* On error: 2 ==> FILE NOT FOUND, or NOT EXECUTABLE */
            }
         }

         if(ParameterBlock.wEnvSeg)
         {
            GlobalDosFree(ParameterBlock.wEnvSeg);
            ParameterBlock.wEnvSeg = 0;
         }
      }
      else
      {
         if(i & PROGRAM_FOLDER)                  // whenever I open a folder
         {
            DosQualifyPath(temp_buf, ofstr.szPathName);

            if(!lpShellExecute)  // that is, the SHELL.DLL is not loaded
            {
               if(*lpArgs)
               {
                  lstrcat(temp_buf, " ");
                  lstrcat(temp_buf, lpArgs);
               }

               wRval = WinExec(temp_buf, nCmdShow);
            }
            else
            {
               wRval = (WORD)lpShellExecute(hMainWnd, "open", temp_buf,
                                            lpArgs, ".", nCmdShow);
            }

            if(ParameterBlock.wEnvSeg)
            {
               GlobalDosFree(ParameterBlock.wEnvSeg);
               ParameterBlock.wEnvSeg = 0;
            }
         }
         else if(i >= 3 && !(GetVersion() & 0x80000000L) && !IS_CHICAGO())
         {              // is this a 32-bit application on Win32s?  Hmmm????
                        // NOTE:  not the same as using 'IsChicago' variable

            DosQualifyPath(temp_buf, ofstr.szPathName);

            if(*lpArgs)
            {
               lstrcat(temp_buf, " ");
               lstrcat(temp_buf, lpArgs);
            }

            wRval = WinExec(temp_buf, nCmdShow);

            // Because of problems with Win32s I can NOT pass an
            // environment to a 32-bit application. Oh, well...

            if(ParameterBlock.wEnvSeg)
            {
               GlobalDosFree(ParameterBlock.wEnvSeg);
               ParameterBlock.wEnvSeg = 0;
            }
         }
         else
         {
            DosQualifyPath(ofstr.szPathName, ofstr.szPathName);

            // WINDOWS applications run from here...

            // IF I am re-directing STDIN/STDOUT, or the app is 32-bit,
            // use the 'CreateProcess' method of starting it, as long
            // as 32-bit thunks are available for this purpose

            if((i >= 3 || hStdIn != HFILE_ERROR || hStdOut != HFILE_ERROR)
               && hKernel32 && hWOW32) // I am supporting generic 32-bit thunks?
            {
             LPSTR lpInFileName = NULL, lpOutFileName = NULL;


               if(hStdIn != HFILE_ERROR)
               {
                  lpInFileName = (LPSTR)lpParseInfo->sbpInFile;

                  MyAssignFileHandle(0, hOldStdIn);

                  _lclose(hStdIn);
                  _lclose(hOldStdIn);

                  hStdIn = hOldStdIn = HFILE_ERROR;

               }

               if(hStdOut != HFILE_ERROR)
               {
                  lpOutFileName = (LPSTR)lpParseInfo->sbpOutFile;

                  MyAssignFileHandle(1, hOldStdOut);
                  MyAssignFileHandle(2, hOldStdErr);

                  _lclose(hStdOut);
                  _lclose(hOldStdOut);
                  _lclose(hOldStdErr);

                  hStdOut = hOldStdOut = hOldStdErr = HFILE_ERROR;
               }

               wRval = RunProgramUsingCreateProcess(ofstr.szPathName,
                                                    &ParameterBlock,
                                                    lpInFileName,
                                                    lpOutFileName);
            }
            else
            {
               wRval = (WORD)LoadModule(ofstr.szPathName,
                                        (LPSTR)&ParameterBlock);
            }
         }
      }
   }
   else if(i<0)  /* this is a NON-WINDOWS EXE file, or COM file! */
   {
      // check for file re-direction/piping, and if in use, handle this
      // sort of thing MANUALLY!  (for now)

      if(i != PROGRAM_BAT &&
         (hStdOut != HFILE_ERROR || hStdIn != HFILE_ERROR))
      {
         if(hKernel32 && hWOW32)  // I am supporting generic 32-bit thunks?
         {
          LPSTR lpInFileName = NULL, lpOutFileName = NULL;


            if(hStdIn != HFILE_ERROR)
            {
               lpInFileName = (LPSTR)lpParseInfo->sbpInFile;

               MyAssignFileHandle(0, hOldStdIn);

               _lclose(hStdIn);
               _lclose(hOldStdIn);

               hStdIn = hOldStdIn = HFILE_ERROR;

            }

            if(hStdOut != HFILE_ERROR)
            {
               lpOutFileName = (LPSTR)lpParseInfo->sbpOutFile;

               MyAssignFileHandle(1, hOldStdOut);
               MyAssignFileHandle(2, hOldStdErr);

               _lclose(hStdOut);
               _lclose(hOldStdOut);
               _lclose(hOldStdErr);

               hStdOut = hOldStdOut = hOldStdErr = HFILE_ERROR;
            }

            wRval = RunProgramUsingCreateProcess(ofstr.szPathName,
                                                 &ParameterBlock,
                                                 lpInFileName,
                                                 lpOutFileName);
         }
         else
         {
            //
            // WORKAROUND - for DOS programs (not PIF files) with re-direction
            //              in process, use 'COMMAND /C' to execute it.
            //              In either case, the command line must have the
            //              '>outfile' and '<infile' added to it.


            wRval = FALSE;                   // initial value (a flag)

            if(i != PROGRAM_PIF)             // i.e. *NOT* a 'PIF' file
            {
               lstrcpy(temp_buf, "  /C ");
               DosQualifyPath(temp_buf + 5, ofstr.szPathName);

               if(!temp_buf[5])
               {
                  wRval = 2;      /* cannot find file */
               }
            }
            else
            {
               lstrcpy(temp_buf, " ");  // re-initialize command line
            }

            if(!wRval)
            {
               // build command line using info in 'lpParseInfo'

               if(*(lpParseInfo->sbpItemArray[0]))
               {
                  lstrcat(temp_buf + 5, " ");
                  lstrcat(temp_buf + 5, lpParseInfo->sbpItemArray[0]);
               }

               if(hStdIn != HFILE_ERROR)
               {
                  MyAssignFileHandle(0, hOldStdIn);

                  _lclose(hStdIn);
                  _lclose(hOldStdIn);

                  hStdIn = hOldStdIn = HFILE_ERROR;

                  // build next part of command line for input file

                  lstrcat(temp_buf + 5, " <");
                  lstrcat(temp_buf + 5, lpParseInfo->sbpInFile);
               }

               if(hStdOut != HFILE_ERROR)
               {
                  MyAssignFileHandle(1, hOldStdOut);
                  MyAssignFileHandle(2, hOldStdErr);

                  _lclose(hStdOut);
                  _lclose(hOldStdOut);
                  _lclose(hOldStdErr);

                  hStdOut = hOldStdOut = hOldStdErr = HFILE_ERROR;

                  // build next part of command line for output file

                  lstrcat(temp_buf + 5, " >");
                  lstrcat(temp_buf + 5, lpParseInfo->sbpOutFile);
               }

               *temp_buf = (char)lstrlen((LPSTR)temp_buf + 1);
                                                        /* length in 1st byte */

               if(i != PROGRAM_PIF)             // i.e. *NOT* a 'PIF' file
               {
                  if(GetEnvString(pCOMSPEC))
                  {
                     lstrcpy(ofstr.szPathName, GetEnvString(pCOMSPEC));
                  }
                  else
                  {
                     lstrcpy(ofstr.szPathName, pCOMMANDCOM);
                  }
               }

               wRval = DosRunProgram(ofstr.szPathName, &ParameterBlock);
            }
         }
      }
      else if(i == PROGRAM_BAT && IsStart)
      {                     // 'START' command - must run in separate window!

         lstrcpy(temp_buf, "  /C ");
         DosQualifyPath(temp_buf + 5, ofstr.szPathName);

         if(!temp_buf[5])  // if the resulting path is BLANK!
         {
            wRval = 2;   /* cannot find file! */
         }
         else
         {
            if(*lpArgs)
            {
               lstrcat(temp_buf, " ");
               lstrcat(temp_buf, lpArgs);
            }

            *temp_buf = (char)lstrlen((LPSTR)temp_buf + 1);
            wRval = (WORD)LoadModule(pPROGRAMNAME,
                                     (LPSTR)&ParameterBlock);
         }
      }
      else if(i != PROGRAM_BAT && hKernel32 && hWOW32)
      {  // NOT a batch file, and no re-direction, but 32-bit stuff here

         wRval = RunProgramUsingCreateProcess(ofstr.szPathName,
                                              &ParameterBlock,
                                              NULL, NULL);
      }
      else
      {
         wRval = DosRunProgram(ofstr.szPathName, &ParameterBlock);
      }
   }
   else
   {
      wRval = 2;   /* FILE NOT FOUND, or NOT EXECUTABLE */
   }



   // if I'm re-directing output, I need to close the appropriate files
   // at this time, and restore everything back to "normal".

   if(hStdOut != HFILE_ERROR)
   {
      MyAssignFileHandle(1, hOldStdOut);
      MyAssignFileHandle(2, hOldStdErr);

      _lclose(hStdOut);
      _lclose(hOldStdOut);
      _lclose(hOldStdErr);

      hStdOut = hOldStdOut = hOldStdErr = HFILE_ERROR;
   }

   if(hStdIn != HFILE_ERROR)
   {
      MyAssignFileHandle(0, hOldStdIn);

      _lclose(hStdIn);
      _lclose(hOldStdIn);

      hStdIn = hOldStdIn = HFILE_ERROR;
   }



   MySetCursor((HCURSOR)GetClassWord(hMainWnd, GCW_HCURSOR));

   if(wRval>=32)    /* it ran ok!! */
   {
      if(wRval!=0xffff)  /* return value for batch file */
      {
         wsprintf(work_buf, "%04xH",
                  (WORD)(hNewTask = GetTaskFromInst((HINSTANCE)wRval)));

         SetEnvString(pTASK_ID,work_buf);


         // FOR NOW this section only applies to Chicago until
         // I have a valid work-around for the Win 3.1 environment
         // "ownership" bug.

         if(IS_CHICAGO())  // only Chicago
         {                 // NOTE: not the same as using 'IsChicago' var

            if(ParameterBlock.wEnvSeg)
            {
               GlobalDosFree(ParameterBlock.wEnvSeg);
               ParameterBlock.wEnvSeg = 0;
            }
         }
         else if(ParameterBlock.wEnvSeg)
         {
          WORD wNewDS, wOldDS, wNewSP, wNewSS, wPDB, wEnv, wOldPDB;
          BOOL bErrFlag;


                /***************************************/
                /*  ENVIRONMENT BLOCK 'HACK' SECTION!! */
                /***************************************/


            // get selector for PDB at offset 60H in TDB

            wEnv = 0;  // as a flag to prevent GP faults...

            wPDB = HIWORD(GlobalHandle((WORD)hNewTask));
            if(wPDB)
            {
               wPDB=*((WORD FAR *)MAKELP(wPDB, 0x60));
            }

            // use this selector to get the env selector, offset 2cH in PDB

            if(wPDB)
            {
               wEnv=*((WORD FAR *)MAKELP(wPDB, 0x2c));

               // for grins, get the SP and SS values too
               wNewSP =*((WORD FAR *)MAKELP(HIWORD(GlobalHandle((WORD)hNewTask)), 2));
               wNewSS =*((WORD FAR *)MAKELP(HIWORD(GlobalHandle((WORD)hNewTask)), 4));

            }

            if(wEnv != ParameterBlock.wEnvSeg)     // they're NOT the same
            {
               GlobalDosFree(ParameterBlock.wEnvSeg);
               ParameterBlock.wEnvSeg = 0;
            }


            wOldPDB = GetCurrentPDB();  // use Win function for this


            // Because of a bug, I must re-allocate the memory for the
            // PDB to be 'owned' by the newly-spawned process.

//            // fake out windows to think I'm a DLL
//
//            *((WORD FAR *)MAKELP(HIWORD(GlobalHandle(hModule)),0xc)) |= 0x8000;
//
            wNewDS = HIWORD(GlobalHandle(wRval));  // instance handle?


            wEnv = LOWORD(GlobalHandle(wEnv));

            _asm
            {

               mov wOldDS, ds
               mov ds, wNewDS

               mov ah, 0x51
               mov bx, wPDB     // switch PDB to that of other process
               call DOS3Call
            }


//            bErrFlag = !GlobalReAlloc((HGLOBAL)wEnv, 0,
//                                      GMEM_MODIFY | GMEM_DDESHARE);

            bErrFlag |= !GlobalReAlloc((HGLOBAL)wEnv, 0,
                                       GMEM_MODIFY | 0x8000);

            _asm
            {
               mov ah, 0x51
               mov bx, wOldPDB     // switch back to my PDB
               call DOS3Call

               mov ds, wOldDS
            }

            if(bErrFlag)
            {
               PrintString("\r\n?Warning - REALLOC failure on PSP environment\r\n");
            }

//            *((WORD FAR *)MAKELP(HIWORD(GlobalHandle(hModule)),0xc)) &= 0x7fff;

         }

      }
      else
      {
         lstrcpy(work_buf, "*BATCH FILE*");

         SetEnvString(pTASK_ID,work_buf);

         if(ParameterBlock.wEnvSeg)
            GlobalDosFree(ParameterBlock.wEnvSeg);
      }


        /**********************************************************/
        /* if I used 'CALL' to get here, wait for program to end! */
        /* this goes double if I'm 'piping'...                    */
        /**********************************************************/

      if((IsCall || PipingFlag) && wRval!=0xffff)
      {                              /* it's NOT a batch/REXX program file */
       LPFNNOTIFYCALLBACK lpProc = NULL;
       DWORD dwProcessID, dwThreadID, dwhProcess;


         dwCMDRunProgramRetCode = -1L;  // initial (error) value

         if(!dwLastProcessID || !hKernel32 || !hWOW32)  // I am supporting generic 32-bit thunks?
         {
            if(lpNotifyRegister)    // no - old fashioned way!
            {
               (FARPROC &)lpProc = MakeProcInstance((FARPROC)CMDRunProgramNotifyCallback,
                                                    hInst);
               if(lpProc)
               {
                  lpNotifyRegister(hNewTask, lpProc, NF_NORMAL);  // ignore errors
               }
            }
         }
         else
         {
//            dwProcessID = GetProcessIDFromHTask(hNewTask);

            dwProcessID = dwLastProcessID;
            dwThreadID  = GetThreadIDFromHTask(hNewTask);
            dwhProcess  = DoOpenProcess(dwProcessID);

         }

         te.dwSize = sizeof(te);
         if(lpTaskFindHandle(&te, hNewTask))
         {
            wRval = (WORD)te.hInst;     // just in case...
            hNewModule = te.hModule;

            MySetCursor(LoadCursor(NULL, IDC_WAIT));

            while(lpIsTask(hNewTask) && lpTaskFindHandle(&te, hNewTask) &&
                  hNewModule==te.hModule && (HINSTANCE)wRval==te.hInst)
            {
               LoopDispatch();  /* wait for program to finish, guys! */
               if(ctrl_break)
               {
                  if(MessageBox(hMainWnd, "Waiting for program to complete...",
                                "** 'CALL' command  **", MB_OKCANCEL | MB_ICONHAND)
                     ==IDCANCEL)
                  {
                     break;
                  }
                  else
                  {
                     ctrl_break = FALSE;  /* 'eat' the control break */
                  }
               }
            }

            MySetCursor((HCURSOR)GetClassWord(hMainWnd, GCW_HCURSOR));
         }

         MthreadSleep(500);  // for testing

         if(!dwLastProcessID || !hKernel32 || !hWOW32)  // I am supporting generic 32-bit thunks?
         {
            if(lpNotifyUnRegister)  // no - old fashioned way!
            {
               if(lpProc)
               {
                  lpNotifyUnRegister(hNewTask);
                  FreeProcInstance((FARPROC)lpProc);
               }
            }
         }
         else
         {
            dwCMDRunProgramRetCode = DoGetExitCodeProcess(dwhProcess);
            DoCloseHandle(dwhProcess);
         }

         wsprintf(work_buf, "%ld", dwCMDRunProgramRetCode);

         SetEnvString(szRETVAL, work_buf);
      }

      wRval = FALSE;
   }
   else                   /** In any case, if it gets here it's an error **/
   {
      // if 'wEnvSeg' was allocated, I must free it at once!

      if(ParameterBlock.wEnvSeg)
      {
         GlobalDosFree(ParameterBlock.wEnvSeg);
         ParameterBlock.wEnvSeg = 0;
      }

        /* 'CMDRunErrorMessage' returns TRUE if it prints a message */
      /* For 'REXX' commands error messages are printed automatically */

      if(CMDRunErrorMessage(wRval))      wRval = TRUE;
      else if(i>0 && (i & REXX_COMMAND)) wRval = TRUE;
      else                               wRval = CMDErrorCheck(TRUE);
   }



   if(lpParseInfo)
   {
      // did I do any PIPING?  If so, I need to dispatch it...

      if(lpParseInfo->sbpPipeNext)  // we're PIPING
      {
         ReDirectOutput(lpParseInfo, FALSE);  // this dispatches any piping

         if(PipingFlag) PipingFlag--;  // cleanup from before
      }

      GlobalFreePtr(lpParseInfo);
   }

   return(wRval);

}


         /******************************************************/
         /**                  FindProgram()                   **/
         /**                                                  **/
         /** Looks for program; returns >0 if Windows program,**/
         /** <0 if non-windows program, 0 if not found.       **/
         /**                                                  **/
         /** Searches in following order:                     **/
         /**                                                  **/
         /**   1)  WINDOWS EXE file (includes 32-bit 'PE')    **/
         /**   2)  .PIF FILE                                  **/
         /**   3)  .REX FILE                                  **/
         /**   4)  .BAT FILE                                  **/
         /**   5)  .COM FILE                                  **/
         /**   6)  NON-WINDOWS EXE FILE                       **/
         /**                                                  **/
         /******************************************************/


int FAR PASCAL FindProgram(LPSTR lpName, LPOFSTRUCT lpOfstr)
{
LPSTR lp1, lp2;
LPSTR lpPathVar=(LPSTR)NULL;
BOOL ExtFlag, DriveDirFlag;
DWORD dwLen, dw1;
HLOCAL h1=(HLOCAL)NULL, h2=(HLOCAL)NULL;
NPSTR path, drive, dir, name, ext, drive2, dir2;
volatile int rval;


   lp1 = GetEnvString(szPATH);

   if(!(h1=LocalAlloc(LMEM_MOVEABLE | LMEM_ZEROINIT, 2*QUAL_PATH_SIZE)) ||
      !(h2=LocalAlloc(LMEM_MOVEABLE | LMEM_ZEROINIT,
                      2 * QUAL_DRIVE_SIZE + QUAL_DIR_SIZE + QUAL_NAME_SIZE
                      + 2 * QUAL_EXT_SIZE )) ||
      !(path = (char NEAR *)LocalLock(h1)) || 
      !(drive = (char NEAR *)LocalLock(h2)) ||
      !(lpPathVar = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                          lstrlen(lp1) + 1)))
   {
      if(path)  LocalUnlock(h1);
      if(drive) LocalUnlock(h2);
      if(h1)    LocalFree(h1);
      if(h2)    LocalFree(h2);

      if(lpPathVar)  GlobalFreePtr(lpPathVar);

      PrintString(pMEMORYLOW);
      return(0);
   }

   lstrcpy(lpPathVar, lp1);

   dir2   = path  + QUAL_PATH_SIZE;

   drive2 = drive + QUAL_DRIVE_SIZE;
   dir    = drive2+ QUAL_DRIVE_SIZE;
   name   = dir   + QUAL_DIR_SIZE;
   ext    = name  + QUAL_NAME_SIZE;


   INIT_LEVEL


   lstrcpy(path, lpName);

   _splitpath(path,drive,dir,name,ext);

   /** check out the drive and path elements, and adjust drive if needed **/

   DriveDirFlag = (*drive || *dir);

   if(*drive==0 && *dir!='\\' && dir[1]!='\\')
   {
      *drive = (char)(_getdrive() + 'A' - 1);
   }

   _lgetdcwd(toupper(*drive) - 'A' + 1, dir2, QUAL_DIR_SIZE);
   if(dir2[1]==':') lstrcpy(dir2, dir2 + 2);


   if(!*dir) lstrcpy(dir, dir2);   // if NO directory specified, use the
                                   // CURRENT directory for the specified
                                   // drive; otherwise, use specified dir.


   /**********************************************************************/
   /** Check the CURRENT PATH - if path starts with '\\', exit on error **/
   /**********************************************************************/

   ExtFlag = FALSE;


   _makepath(path, drive, dir, name, ext);

   if(!IsProgramFile((LPSTR)path, lpOfstr, (int FAR *)&rval) && rval)
   {
      break;     // PROGRAM FILE was found (matches 'path' properly)
   }

   if(*ext)                          /* and, there's an extension ta-boot! */
   {
      ExtFlag = TRUE;

        /* if it qualifies, get the 'association' for the extension */

      if(_fstricmp(ext,pPIF)!=0 && _fstricmp(ext,pCOM)!=0 &&
         _fstricmp(ext,pBAT)!=0 && _fstricmp(ext,pEXE)!=0 &&
         _fstricmp(ext,pREX)!=0)
      {
         // FILE HAS A 'NON-PROGRAM' EXTENSION - SEE IF THERE IS AN ASSOCIATION!

         if(lpRegQueryValue)
         {
          HKEY hKey = HKEY_CLASSES_ROOT;

            if(iswap((WORD)GetVersion()) <= 0x350 &&
                !(GetVersion() & 0x80000000))
            {
               hKey = (HKEY)1;  // not Chicago, and not NT
            }

            dwLen = sizeof(lpOfstr->szPathName);
            if(lpRegQueryValue(hKey, ext, lpOfstr->szPathName,
                               (LONG FAR *)&dwLen)==ERROR_SUCCESS)
            {
               lstrcat(lpOfstr->szPathName, "\\shell\\open\\command");
               dwLen = sizeof(lpOfstr->szPathName);
               if(lpRegQueryValue(hKey, lpOfstr->szPathName,
                                  lpOfstr->szPathName, (LONG FAR *)&dwLen)
                  ==ERROR_SUCCESS)
               {
                  dwLen = lstrlen(lpOfstr->szPathName);

                      /* trim all trailing spaces */
                  while(dwLen && lpOfstr->szPathName[dwLen - 1]<=' ')
                  {
                     lpOfstr->szPathName[--dwLen]=0;
                  }


             /* see if command ends in '%1', and trim it if so */

                  if(lpOfstr->szPathName[dwLen-2]=='%' &&
                     lpOfstr->szPathName[dwLen-1]=='1')
                  {
                     dwLen -= 2;

                     lpOfstr->szPathName[dwLen]=0;

                      /* trim all trailing spaces */
                     while(dwLen && lpOfstr->szPathName[dwLen - 1]<=' ')
                     {
                        lpOfstr->szPathName[--dwLen]=0;
                     }

                     rval = PROGRAM_ASSOCIATION;   /* FOUND THE SUCKAH! */
                     break;

                  }


                  rval = PROGRAM_ASSOCIATION;   /* FOUND THE SUCKAH! */
                  break;

               }
            }
         }

              /* when not found in above, look in 'WIN.INI' */

         GetProfileString("EXTENSIONS",(*ext=='.'?ext+1:ext),"",
                          lpOfstr->szPathName, sizeof(lpOfstr->szPathName));

         if(*lpOfstr->szPathName)
         {
            lp1 = _fstrstr(lpOfstr->szPathName, " ^.");
            if(lp1) *lp1=0;

            rval = PROGRAM_ASSOCIATION;   /* for now... */
            break;

         }

      }

      _makepath(path,drive,dir,name,ext);

      if(IsProgramFile((LPSTR)path, lpOfstr, (int FAR *)&rval))
      {
         rval = 0;          /* bad result - not a 'program' file */
      }
      else if(rval==0)   /* not an error, but not an EXE file either */
      {
         if(_fstricmp(ext,pPIF)==0)
         {
            rval = PROGRAM_PIF;
         }
         else if(_fstricmp(ext,pBAT)==0)
         {
            rval = PROGRAM_BAT;
         }
         else if(_fstricmp(ext,pCOM)==0)
         {
            rval = PROGRAM_COM;
         }
         else if(_fstricmp(ext,pREX)==0)
         {
            rval = REXX_COMMAND;
         }
      }

      if(rval) break;              /* ends function at 'END_LEVEL' (below) */
   }
   else
   {             /** See if there's a WINDOWS 'EXE' file **/

      if(*name)
      {
         _makepath(path, drive, dir, name, pEXE);

         if(!IsProgramFile((LPSTR)path, lpOfstr, (int FAR *)&rval))
         {
            if(rval>0) break;  /* WINDOWS .EXE file - go to the 'END_LEVEL' */
         }

         _makepath(path, drive, dir, name, ext);

      }
                    /** Look for the other extensions **/

      rval = FindProgramExt(path, drive, dir, name, ext, lpOfstr);

      if(rval) break;
   }


   if(DriveDirFlag)     /* we're locked into a particular path.. oh, well! */
   {
      break;                  /* in this case the file wasn't found */
   }



         /*******************************************************/
         /*** Check the PROGRAM directory (where SFTShell is) ***/
         /*******************************************************/

   GetModuleFileName(hInst, dir2, QUAL_PATH_SIZE);

   lp1 = _fstrrchr(dir2, '\\');  // find the trailing backslash
   if(lp1)
   {
      lp1[1] = 0;  // terminate string after it - this is the 'exe path'
   }
   else
   {
      strcat(dir2, pBACKSLASH);  // in reality, this is a problem, but...
   }

   _makepath(path, "", dir2, name, ext);

   if(!IsProgramFile((LPSTR)path, lpOfstr, (int FAR *)&rval) && rval)
   {
      break;     // PROGRAM FILE was found (matches 'path' properly)
   }


   if(ExtFlag)         /* there is already an extension! */
   {
      _makepath(path,"",dir2,name,ext);

      if(IsProgramFile((LPSTR)path, lpOfstr, (int FAR *)&rval))
      {
         rval = 0;  /* bad result - error or not EXE file! */
      }
      else if(rval==0)   /* not an error, but not an EXE file either */
      {
         if(_fstricmp(ext,pPIF)==0)
         {
            rval = PROGRAM_PIF;
         }
         else if(_fstricmp(ext,pBAT)==0)
         {
            rval = PROGRAM_BAT;
         }
         else if(_fstricmp(ext,pCOM)==0)
         {
            rval = PROGRAM_COM;
         }
         else if(_fstricmp(ext,pREX)==0)
         {
            rval = REXX_COMMAND;
         }
      }

      if(rval) break;
   }
   else
   {             /** See if there's a WINDOWS 'EXE' file **/

      _makepath(path, "", dir2, name, pEXE);

      if(!IsProgramFile((LPSTR)path, lpOfstr, (int FAR *)&rval))
      {
         if(rval>0) break;  /* WINDOWS .EXE file - go to the 'END_LEVEL' */
      }
                    /** Look for the other extensions **/

      _makepath(path, "", dir2, name, ext);

      rval = FindProgramExt(path, "", dir2, name, ext, lpOfstr);

      if(rval) break;               /** FOUND! **/
   }



         /********************************************************/
         /*** Check the WINDOWS and WINDOWS SYSTEM directories ***/
         /********************************************************/

   GetWindowsDirectory(dir2, QUAL_PATH_SIZE);

   if(dir2[strlen(dir2) - 1]!='\\')  strcat(dir2, pBACKSLASH);


   _makepath(path, "", dir2, name, ext);

   if(!IsProgramFile((LPSTR)path, lpOfstr, (int FAR *)&rval) && rval)
   {
      break;     // PROGRAM FILE was found (matches 'path' properly)
   }


   if(ExtFlag)         /* there is already an extension! */
   {
      _makepath(path,"",dir2,name,ext);

      if(IsProgramFile((LPSTR)path, lpOfstr, (int FAR *)&rval))
      {
         rval = 0;  /* bad result - error or not EXE file! */
      }
      else if(rval==0)   /* not an error, but not an EXE file either */
      {
         if(_fstricmp(ext,pPIF)==0)
         {
            rval = PROGRAM_PIF;
         }
         else if(_fstricmp(ext,pBAT)==0)
         {
            rval = PROGRAM_BAT;
         }
         else if(_fstricmp(ext,pCOM)==0)
         {
            rval = PROGRAM_COM;
         }
         else if(_fstricmp(ext,pREX)==0)
         {
            rval = REXX_COMMAND;
         }
      }

      if(rval) break;
   }
   else
   {             /** See if there's a WINDOWS 'EXE' file **/

      _makepath(path, "", dir2, name, pEXE);

      if(!IsProgramFile((LPSTR)path, lpOfstr, (int FAR *)&rval))
      {
         if(rval>0) break;  /* WINDOWS .EXE file - go to the 'END_LEVEL' */
      }
                    /** Look for the other extensions **/

      _makepath(path, "", dir2, name, ext);

      rval = FindProgramExt(path, "", dir2, name, ext, lpOfstr);

      if(rval) break;               /** FOUND! **/
   }


   GetSystemDirectory(dir2, QUAL_PATH_SIZE);

   if(dir2[strlen(dir2) - 1]!='\\')  strcat(dir2, pBACKSLASH);


   _makepath(path, "", dir2, name, ext);

   if(!IsProgramFile((LPSTR)path, lpOfstr, (int FAR *)&rval) && rval)
   {
      break;     // PROGRAM FILE was found (matches 'path' properly)
   }


   if(ExtFlag)         /* there is already an extension! */
   {
      _makepath(path,"",dir2,name,ext);

      if(IsProgramFile((LPSTR)path, lpOfstr, (int FAR *)&rval))
      {
         rval = 0;  /* bad result - error or not EXE file! */
      }
      else if(rval==0)   /* not an error, but not an EXE file either */
      {
         if(_fstricmp(ext,pPIF)==0)
         {
            rval = PROGRAM_PIF;
         }
         else if(_fstricmp(ext,pBAT)==0)
         {
            rval = PROGRAM_BAT;
         }
         else if(_fstricmp(ext,pCOM)==0)
         {
            rval = PROGRAM_COM;
         }
         else if(_fstricmp(ext,pREX)==0)
         {
            rval = REXX_COMMAND;
         }
      }

      if(rval) break;
   }
   else
   {              /** See if there's a WINDOWS 'EXE' file **/

      _makepath(path, "", dir2, name, pEXE);

      if(!IsProgramFile((LPSTR)path, lpOfstr, (int FAR *)&rval))
      {
         if(rval>0) break;  /* WINDOWS .EXE file - go to the 'END_LEVEL' */
      }

                    /** Look for the other extensions **/

      _makepath(path, "", dir2, name, ext);

      rval = FindProgramExt(path, "", dir2, name, ext, lpOfstr);

      if(rval) break;               /** FOUND! **/
   }


          /**********************************************/
          /** SEARCH THE 'PATH' VARIABLE, IF IT EXISTS **/
          /**********************************************/

   if(*lpPathVar)
   {
      for(rval=0,lp1=lpPathVar; *lp1!=0; lp1 = lp2 + 1)
      {
         if(!(lp2 = _fstrchr(lp1, ';')))
         {
            lp2 = lp1 + lstrlen(lp1);
            lstrcpy(path, lp1);
         }
         else
         {
            *lp2 = 0;         /* temporary */
            lstrcpy(path, lp1);
            *lp2 = ';';  /* back to the way it was */
         }

         if(path[strlen(path) - 1]!='\\')  strcat(path,pBACKSLASH);
         strcat(path,pWILDCARD);  /* a 'dummy' name */

         _splitpath(path, drive2, dir2, path, path);

          /* add dir from original spec to 'PATH' extraction */

         if(dir2[strlen(dir2) - 1]!='\\')  strcat(dir2,pBACKSLASH);


         _makepath(path, drive2, dir2, name, ext);

         if(!IsProgramFile((LPSTR)path, lpOfstr, (int FAR *)&rval) && rval)
         {
            break;     // PROGRAM FILE was found (matches 'path' properly)
         }


         if(ExtFlag)         /* there is already an extension! */
         {
            _makepath(path,drive2,dir2,name,ext);

            if(IsProgramFile((LPSTR)path, lpOfstr, (int FAR *)&rval))
            {
               rval = 0;  /* bad result - error or not EXE file! */
            }
            else if(rval==0)   /* not an error, but not an EXE file either */
            {
               if(_fstricmp(ext,pPIF)==0)
               {
                  rval = PROGRAM_PIF;
               }
               else if(_fstricmp(ext,pBAT)==0)
               {
                  rval = PROGRAM_BAT;
               }
               else if(_fstricmp(ext,pCOM)==0)
               {
                  rval = PROGRAM_COM;
               }
               else if(_fstricmp(ext,pREX)==0)
               {
                  rval = REXX_COMMAND;
               }
            }

            if(rval) break;
         }
         else
         {              /** See if there's a WINDOWS 'EXE' file **/

            _makepath(path, drive2, dir2, name, pEXE);

            if(!IsProgramFile((LPSTR)path, lpOfstr, (int FAR *)&rval))
            {
               if(rval>0) break;  /* WINDOWS .EXE file - go to the 'END_LEVEL' */
            }

                          /** Look for the other extensions **/

            _makepath(path, drive2, dir2, name, ext);

            rval = FindProgramExt(path, drive2, dir2, name, ext, lpOfstr);

            if(rval) break;               /** FOUND! **/

         }

         if(*lp2==0)  break;       /* end of string (don't go over!) */

      }
   }


   if(rval) break;           /* rval is non-zero if program was found */



         /*******************************************************/
         /***     Check the REGISTRY for a matching key...    ***/
         /*******************************************************/


   if(lpRegOpenKey)
   {
      _makepath(path, "", "", name, ext);

      if(lpRegQueryValue && lpRegEnumKey)
      {
       HKEY hKey = HKEY_CLASSES_ROOT;

         if(iswap((WORD)GetVersion()) <= 0x350 &&
             !(GetVersion() & 0x80000000))
         {
            hKey = (HKEY)1;  // not Chicago, and not NT
         }

         // enumerate all keys that match 'path' followed by a '.' and
         // any additional text.  Use the first match's 'CLSID' entry,
         // then look up 'CLSID' and get the shell program, and run it.
         // (not too bad, really).

         if(path[lstrlen(path)-1] != '.')  lstrcat(path, ".");  // ends in '.'

         if(lpRegOpenKey(hKey, NULL, &hKey) == ERROR_SUCCESS)  // optimization
         {
            dw1 = 0;

            for(dw1 = 0; lpRegEnumKey(hKey, dw1, dir2, QUAL_PATH_SIZE)
                         == ERROR_SUCCESS; dw1++)
            {
               // see if 'dir2' matches the desired pattern...

               if(!_fstrnicmp(path, dir2, lstrlen(path)))
               {
                  // a MATCH!  Let's see what the doctor ordered...

                  lstrcat(dir2, "\\CLSID");  // get the OLE CLSID


                  dwLen = QUAL_PATH_SIZE;
                  if(lpRegQueryValue(hKey, dir2, dir2,(LONG FAR *)&dwLen)==ERROR_SUCCESS)
                  {
                     // we have the correct value now; prepare new key

                     _fmemmove(dir2 + 6, dir2, lstrlen(dir2) + 1);
                     _hmemcpy(dir2, "CLSID\\", 6);  // prepend 'CLSID\'

                     lstrcat(dir2, "\\ProgID");  // the next key I find...

                     dwLen = QUAL_PATH_SIZE;
                     if(lpRegQueryValue(hKey, dir2, dir2,(LONG FAR *)&dwLen)
                        ==ERROR_SUCCESS)
                     {
                        // OK!  We now have the key for the program!
                        // Get the 'Shell' 'Open' command for it...

                        lstrcat(dir2, "\\shell\\open\\command");

                        dwLen = QUAL_PATH_SIZE;
                        if(lpRegQueryValue(hKey, dir2, dir2, (LONG FAR *)&dwLen)
                           ==ERROR_SUCCESS)
                        {
                           // remove all of the 'fru fru' from the end of the command

                           if(*dir2 == '\"')  // starts with '"'?
                           {
                              for(lp1=dir2 + 1; *lp1 && *lp1 != '\"'; lp1++)
                                 ;  // find ending quote mark

                              if(lp1 > (dir2 + 1))
                              {
                                 _hmemcpy(path, dir2 + 1,
                                          lp1 - (LPSTR)dir2 - 1);
                              }

                              path[lp1 - (LPSTR)dir2 - 1] = 0;
                           }
                           else
                           {
                              for(lp1=dir2; *lp1 > ' '; lp1++)
                                 ; // find next white space character

                              if(lp1 > (dir2 + 1))
                              {
                                 _hmemcpy(path, dir2, lp1 - (LPSTR)dir2);
                              }

                              path[lp1 - (LPSTR)dir2] = 0;
                           }

                           if(!IsProgramFile((LPSTR)path, lpOfstr,
                                             (int FAR *)&rval))
                           {
                              break;  // FOUND!!!
                           }
                        }
                     }
                  }
               }
            }

            lpRegCloseKey(hKey);
         }

      }

   }


   if(rval) break;           /* rval is non-zero if program was found */




         /*******************************************************/
         /*** SET 'PATH' to BLANK, use OpenFile() to find it! ***/
         /*******************************************************/


   // THIS IS THE 'last desparate hope' TO MAP IN THE NETWORK PATHS!

   DelEnvString(szPATH);


   _makepath(path, "", "", name, ext);

   if(!IsProgramFile((LPSTR)path, lpOfstr, (int FAR *)&rval) && rval)
   {
      break;     // PROGRAM FILE was found (matches 'path' properly)
   }


   if(ExtFlag)         /* there is already an extension! */
   {
      _makepath(path,"","",name,ext);

      if(IsProgramFile((LPSTR)path, lpOfstr, (int FAR *)&rval))
      {
         rval = 0;  /* bad result - error or not EXE file! */
      }
      else if(rval==0)   /* not an error, but not an EXE file either */
      {
         if(_fstricmp(ext,pPIF)==0)
         {
            rval = PROGRAM_PIF;
         }
         else if(_fstricmp(ext,pBAT)==0)
         {
            rval = PROGRAM_BAT;
         }
         else if(_fstricmp(ext,pCOM)==0)
         {
            rval = PROGRAM_COM;
         }
         else if(_fstricmp(ext,pREX)==0)
         {
            rval = REXX_COMMAND;
         }
      }
   }
   else
   {              /** See if there's a WINDOWS 'EXE' file **/

      _makepath(path, "", "", name, pEXE);

      if(!ExtFlag && !IsProgramFile((LPSTR)path, lpOfstr, (int FAR *)&rval))
      {
         if(rval>0) break;  /* WINDOWS .EXE file - go to the 'END_LEVEL' */
      }

                       /** Look for the other extensions **/

      _makepath(path, "", "", name, ext);

      rval = FindProgramExt(path, NULL, NULL, name, ext, lpOfstr);
   }


   SetEnvString(szPATH, lpPathVar);  /* RESTORE ENVIRONMENT STRING 'PATH' */



   END_LEVEL                               /** ALL OF THE BREAKS GO HERE **/

   LocalUnlock(h1);
   LocalUnlock(h2);
   LocalFree(h1);
   LocalFree(h2);

   GlobalFreePtr(lpPathVar);

   return(rval);

}



int _near _fastcall FindProgramExt(NPSTR path, NPSTR drive, NPSTR dir,
                                   NPSTR name, NPSTR ext, LPOFSTRUCT lpOfstr)
{
int rval;
BOOL OpenOnly=FALSE;
static struct _find_t ff;
static char pBlank[1]={0};
#define EXIST_SHARE (OF_READ | OF_SHARE_DENY_NONE | OF_EXIST)
#define EXIST_COMPAT (OF_READ | OF_SHARE_COMPAT | OF_EXIST)



   if(!drive || !dir)
   {
      OpenOnly = TRUE; /* forces 'OpenFile' as only method */
   }

   if(!drive) drive = pBlank;  /* if either is NULL, it's blank */
   if(!dir)   dir   = pBlank;

   _makepath(path,drive,dir,name,pPIF);


   INIT_LEVEL

   // OBTW - 'MyFindFirst()' returns 0 when file exists!!


   if(OpenOnly || !MyFindFirst(path, (~_A_VOLID & ~_A_SUBDIR) & 0xff, &ff))
   {
      if(!OpenOnly) MyFindClose(&ff);

      if(MyOpenFile((LPSTR)path, lpOfstr, EXIST_SHARE)!=HFILE_ERROR ||
         MyOpenFile((LPSTR)path, lpOfstr, EXIST_COMPAT)!=HFILE_ERROR)
      {
         rval = PROGRAM_PIF;  // 'PIF' files are considered to be 'non-windows'
         break;
      }
   }

   _makepath(path,drive,dir,name,pREX);

   if(OpenOnly || !MyFindFirst(path, (~_A_VOLID & ~_A_SUBDIR) & 0xff, &ff))
   {
      if(!OpenOnly) MyFindClose(&ff);

      if(MyOpenFile((LPSTR)path, lpOfstr, EXIST_SHARE)!=HFILE_ERROR ||
         MyOpenFile((LPSTR)path, lpOfstr, EXIST_COMPAT)!=HFILE_ERROR)
      {
         rval = REXX_COMMAND;  /* '.REX' files are considered 'windows' */
         break;
      }
   }

   _makepath(path,drive,dir,name,pBAT);

   if(OpenOnly || !MyFindFirst(path, (~_A_VOLID & ~_A_SUBDIR) & 0xff, &ff))
   {
      if(!OpenOnly) MyFindClose(&ff);

      if(MyOpenFile((LPSTR)path, lpOfstr, EXIST_SHARE)!=HFILE_ERROR ||
         MyOpenFile((LPSTR)path, lpOfstr, EXIST_COMPAT)!=HFILE_ERROR)
      {
         rval = PROGRAM_BAT;   // 'BAT' files are considered to be 'non-windows'
         break;
      }
   }

   _makepath(path,drive,dir,name,pCOM);

   if(OpenOnly || !MyFindFirst(path, (~_A_VOLID & ~_A_SUBDIR) & 0xff, &ff))
   {
      if(!OpenOnly) MyFindClose(&ff);

      if(MyOpenFile((LPSTR)path, lpOfstr, EXIST_SHARE)!=HFILE_ERROR ||
         MyOpenFile((LPSTR)path, lpOfstr, EXIST_COMPAT)!=HFILE_ERROR)
      {
         rval = PROGRAM_COM; // 'COM' files are considered to be 'non-windows'
         break;
      }
   }

   _makepath(path,drive,dir,name,pEXE);

   rval = 0;               /* rval==0 when FILE NOT FOUND */

   if(OpenOnly || !MyFindFirst(path, (~_A_VOLID & ~_A_SUBDIR) & 0xff, &ff))
   {
      if(!OpenOnly) MyFindClose(&ff);

      if(!IsProgramFile((LPSTR)path, lpOfstr, (int FAR *)&rval))
      {
         break;
      }
   }

   END_LEVEL

   _makepath(path,drive,dir,name,ext);  /* restore the path as it was */

   return(rval);
}



BOOL FAR PASCAL IsProgramFile(LPSTR lpName, LPOFSTRUCT lpOfstr,
                              int FAR *lpType)
{
HFILE     hFile;
NEWEXEHDR ne;
EXEHDR    xe;



   if(iswap(LOWORD(GetVersion())) >= CHICAGO_MIN_VER
      && lstrlen(lpName)) // are we running Chicago??
   {
    struct _find_t ff;

      // Check for a sub-directory being opened, so I can attempt to
      // 'SHELL EXECUTE' the thing.
      //
      // Return the 'PROGRAM_FOLDER' flag if it's valid.


      if(lpName[lstrlen(lpName) - 1] == '\\')       // ends in a backslash??
      {
         if(FileExists(lpName)) goto it_is_a_subdirectory;
      }
      else if(!MyFindFirst(lpName, (~_A_VOLID) & 0xff, &ff))
      {
         if(ff.attrib & _A_SUBDIR)  // it's a sub-directory, eh??
         {
            MyFindClose(&ff);

it_is_a_subdirectory:

            DosQualifyPath(lpOfstr->szPathName, lpName);
            *lpType = PROGRAM_FOLDER;
                                   // yes, it's a "program file" (sort of...)

            return(FALSE);         // NOT an error!
         }
         else
         {
            MyFindClose(&ff);
         }
      }
   }


   if(IsChicago || IsNT)
   {
    UINT uiFlags;

      // MODE FLAGS:    OPEN_ACCESS_RO_NOMODLASTACCESS = 4
      // ACTION FLAGS:  FILE_OPEN = 1

      uiFlags = 4 | OF_SHARE_DENY_NONE;

      hFile = MyOpenFile(lpName, lpOfstr,
                         OF_READ | OF_SHARE_DENY_NONE | OF_EXIST);

      if(hFile==HFILE_ERROR) /* error 1st time - retry compatibility mode */
      {
         hFile = MyOpenFile(lpName, lpOfstr,
                            OF_READ | OF_SHARE_COMPAT | OF_EXIST);

         uiFlags = 4 | OF_SHARE_COMPAT;
      }

      if(hFile != HFILE_ERROR)
      {

         if(MyCreateFile(lpOfstr->szPathName, uiFlags, 0, 1,
                         (HFILE FAR *)&hFile))
         {
            hFile = HFILE_ERROR;  // error opening file
         }
      }
   }
   else
   {
      hFile = MyOpenFile(lpName, lpOfstr, OF_READ | OF_SHARE_DENY_NONE);

      if(hFile==HFILE_ERROR) /* error 1st time - retry compatibility mode */
      {
         hFile = MyOpenFile(lpName, lpOfstr, OF_READ | OF_SHARE_COMPAT);
      }
   }

   if(hFile!=HFILE_ERROR)
   {
      if(_lread(hFile, (LPSTR)(LPEXEHDR)&xe, sizeof(xe))!=sizeof(xe))
      {
         _lclose(hFile);
         *lpType = PROGRAM_FALSE;
         return(FALSE);        /* if I can't read it, no problem... */
      }

      if(xe.id_sig[0]!='M' || xe.id_sig[1]!='Z')  /* not an EXE file! */
      {
         _lclose(hFile);
         *lpType = PROGRAM_FALSE;
         return(FALSE);  /* not an error */
      }

      if(xe.first_reloc_item < 0x40)  /* not a 'WINDOWS' or 'OS2' EXE */
      {
         _lclose(hFile);
         *lpType = PROGRAM_NONWINDOWS;   // NON-WINDOWS EXE FILE
         return(FALSE);
      }

      _llseek(hFile, xe.new_exe_offset, 0);

      if(_lread(hFile, (LPSTR)(LPNEWEXEHDR)&ne, sizeof(ne))!=sizeof(ne))
      {
         _lclose(hFile);
         *lpType = PROGRAM_NONWINDOWS;   // NON-WINDOWS EXE FILE
         return(FALSE);          /** no error, really **/
      }

      if((ne.id_sig[0]!='N' && ne.id_sig[0]!='L' && ne.id_sig[0]!='P')
          || ne.id_sig[1]!='E')
      {
         _lclose(hFile);
         *lpType = PROGRAM_NONWINDOWS;   // NON-WINDOWS EXE FILE
         return(FALSE); /* no error */
      }

      if(ne.id_sig[0] == 'P' && ne.id_sig[1] == 'E')
      {
         *lpType = PROGRAM_PE;                // 32-bit 'PE' windows exe/dll
      }
      else if(ne.id_sig[0]=='L' && ne.id_sig[1] == 'E')
      {
         *lpType = PROGRAM_VXD;     // '32 bit' Windows driver...
      }
      else if((ne.wOSFlag & NEOS_OSFLAGS)==NEOS_WINDOWS) /* it's a WINDOWS EXE!! */
      {
         if(ne.id_sig[0]=='L')
         {
            *lpType = PROGRAM_L32;   // Unknown 32-bit 'LE' app
         }
         else
         {
            *lpType = PROGRAM_WIN16; // 16-bit windows exe/dll
         }
      }
      else if((ne.wOSFlag & NEOS_OSFLAGS)==NEOS_OS2)  /* it's an OS2 EXE */
      {
         if(ne.id_sig[0]=='L')
         {
            *lpType = PROGRAM_OS2_32;  // OS/2 32 bit app??
         }
         else
         {
            *lpType = PROGRAM_OS2;  // OS2 EXE file - well well well!
         }
      }
      else if(ne.id_sig[0]=='L')
      {
         *lpType = PROGRAM_NONWINDOWS;   // for now...
      }
      else
      {
         *lpType = PROGRAM_NONWINDOWS;   // NON-WINDOWS EXE FILE
      }

      _lclose(hFile);
      return(FALSE);    /* GOOD RESULT! */
   }

   return(TRUE);       /* BAD RESULT! */

}






/***************************************************************************/
/*                                                                         */
/*           BATCH FILE PROCESSING UTILITY AND 'CORE' FUNCTIONS            */
/*                                                                         */
/***************************************************************************/

/***************************************************************************/
/*                         GetAnotherBatchLine()                           */
/***************************************************************************/

extern LPSTR lpBatchLineBuf;   // allocated once, free'd on exit from batch
                               // processing thread.


LPSTR FAR PASCAL GetAnotherBatchLine(LPBATCH_INFO lpBatchInfo)
{
LPSTR lp1, lp2, lp3, lp4, lpBuf;
WORD wParmNum, wCount, buf_pos, buf_len, w1;


   if(!SELECTOROF(lpBatchInfo))
   {
      PrintErrorMessage(784);
      return(NULL);  // may eliminate some UAE's
   }

//   if(lpBatchInfo->hFile==HFILE_ERROR)  return(NULL);  /* bad file handle */
// removed 1/17/95 to allow memory batch files

   if(!lpBatchLineBuf)
   {
      lpBatchLineBuf = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE, BATCH_BUFSIZE);
   }

   if(!lpBatchLineBuf)
   {
      PrintErrorMessage(785);

      return(NULL);
   }

   lp1 = lpBatchLineBuf;
   *lp1 = 0;

// added 10/1/92 to speed things up a bit
   buf_pos = lpBatchInfo->buf_pos;
   buf_len = lpBatchInfo->buf_len;
   lpBuf   = (LPSTR)(lpBatchInfo->buf + buf_pos);


   do
   {
      if(buf_pos>=buf_len)
      {

// added 1/17/95 for memory batch files
         if(lpBatchInfo->hFile == HFILE_ERROR) goto end_of_file;


         lpBatchInfo->buf_start += lpBatchInfo->buf_len;
         lpBatchInfo->buf_len = _lread(lpBatchInfo->hFile, lpBatchInfo->buf,
                                       BATCH_BUFSIZE);
         lpBatchInfo->buf_pos = 0;

// added 10/1/92
         buf_pos = lpBatchInfo->buf_pos;
         buf_len = lpBatchInfo->buf_len;
         lpBuf   = (LPSTR)(lpBatchInfo->buf + buf_pos);

         if(lpBatchInfo->buf_len==(WORD)-1)
         {
            CMDErrorCheck(TRUE);
            return(NULL);
         }
         else if(lpBatchInfo->buf_len==0)
         {
end_of_file:

            if(lp1 == lpBatchLineBuf)  /* beginning of line - no command! */
            {
               return(NULL);   /* the end of the file!! */
            }
            else
            {
               *lp1 = 0;
               break;
            }
         }
      }

      *lp1 = *(lpBuf++);
      buf_pos++;

      if(*lp1=='\n' || *lp1=='\x1a')              /* line feed or ctrl z */
      {
         if(lp1>lpBatchLineBuf && *(lp1 - 1)=='\r')
         {
            *(--lp1) = 0;
         }
         else
         {
            *lp1 = 0;
         }

         break;      /* exit from loop.  End of the line */
      }

      if((WORD)(lp1 - lpBatchLineBuf)>=BATCH_BUFSIZE/2)
      {
         PrintErrorMessage(786);
         return(NULL);
      }

      *(++lp1) = 0;  /* always ensure end of string is marked */

   } while(TRUE);

// added 10/1/92

   lpBatchInfo->buf_pos = buf_pos;    // ensure 'lpBatchInfo' stays current!
   lpBatchInfo->buf_len = buf_len;




      /*** At this point we're at the end of the line.  Now, parse it ***/
      /*** for batch command line variables and substitution strings  ***/


   for(lp1=lpBatchLineBuf; *lp1!=0; lp1++)
   {
      lp2 = lp1;
      while(*lp2 && *lp2!='%') lp2++;

      if(!*lp2)               // I'm done
      {
         break;
      }
      else                    // I FOUND A '%'!  YAY!
      {
         lp1 = lp2++;
         if(*lp2<'0' || *lp2>'9')
         {
            while(*lp2 && *lp2!='%') lp2++; /* find next '%' */

            if(*lp2) lp1 = ++lp2;   /* point past the '%' in any case */
            else     lp1++;

            continue;  /* just keep going from this point... */
         }

                    /* which parameter number is it? */

         wParmNum = (WORD)(*lp2 - '0');  /* parameter number */

                      /* get ptr to parameter! */

         lp4 = "";      /* value if item not found!! */

         (volatile LPSTR &)lp3 = lpBatchInfo->lpArgList;  /* argument list strings */

         for(wCount=1; *lp3!=0; lp3 += lstrlen(lp3) + 1, wCount++)
         {
            if(wCount == (wParmNum + lpBatchInfo->nShift))
            {
               (volatile LPSTR &)lp4 = lp3;
               break;
            }
         }

                   /* insert this string on top of the old */

         lp3 = lp1 + (w1 = lstrlen(lp4));  /* new place for rest of string */
         lp2++;                     /* point 'lp2' just past '%#' */

         _fmemmove(lp3, lp2, lstrlen(lp2) + 1);/* move back half of string */


         if(w1)   // w1 is 'lstrlen(lp4)' (see above)
         {
            _fmemcpy(lp1, lp4, w1);  /* inject the parameter */
         }

         lp1 = lp3 - 1;      /* this prevents possible infinite loops */
                         /* if the user puts '%n' as n'th parm (whoops!) */

                   /** I'm done with this parameter! **/
      }

   }

   lp1 = EvalString(lpBatchLineBuf);  /* evaluate string!!! This gets env var's */

   return(lp1);

}


// this is similar to 'GetAnotherBatchLine()' but in reverse...

BOOL FAR PASCAL PointToPreviousBatchLine(LPBATCH_INFO lpBatchInfo)
{
LPSTR lpBuf;
WORD buf_pos, buf_len;
int iCRLFFlag = 0;


   if(!SELECTOROF(lpBatchInfo))
   {
      PrintErrorMessage(784);
      return(NULL);  // may eliminate some UAE's
   }

   buf_pos = lpBatchInfo->buf_pos;
   buf_len = lpBatchInfo->buf_len;
   lpBuf   = (LPSTR)(lpBatchInfo->buf + buf_pos);


   do
   {
      if(buf_pos == 0)
      {
         if(lpBatchInfo->hFile == HFILE_ERROR ||
            lpBatchInfo->buf_start == 0)
         {
            return(TRUE); // the end of the file! (or rather, the beginning)
         }

         if(lpBatchInfo->buf_start < BATCH_BUFSIZE)
         {
            lpBatchInfo->buf_start = 0;
         }
         else
         {
            lpBatchInfo->buf_start -= BATCH_BUFSIZE;
         }

         _llseek(lpBatchInfo->hFile, lpBatchInfo->buf_start, SEEK_SET);

         lpBatchInfo->buf_len = _lread(lpBatchInfo->hFile, lpBatchInfo->buf,
                                       BATCH_BUFSIZE);

         lpBatchInfo->buf_pos = lpBatchInfo->buf_len;

         buf_pos = lpBatchInfo->buf_pos;
         buf_len = lpBatchInfo->buf_len;
         lpBuf   = (LPSTR)(lpBatchInfo->buf + buf_pos);

         if(lpBatchInfo->buf_len==(WORD)-1)
         {
            return(TRUE); // this is a READ ERROR
         }
         else if(lpBatchInfo->buf_len==0)
         {
            return(TRUE); // the end of the file! (or, error on read)
         }
      }

      buf_pos--;                      // previous character
      lpBuf--;

      if(*lpBuf=='\n' || *lpBuf == '\r')             // CRLF (previous line?)
      {
         if(iCRLFFlag == 1 && *lpBuf == '\r')  // last char was a '\n'
         {
            iCRLFFlag = -1;                     // CRLF found (next is EOL)
            continue;
         }
         else if(iCRLFFlag == 2 && *lpBuf == '\n')
         {
            iCRLFFlag = -2;                     // LFCR found (next is EOL)
            continue;
         }
         else if(!iCRLFFlag)
         {
            iCRLFFlag = *lpBuf=='\n'?1:2;       // mark 'LF' or 'CR' found
            continue;
         }

         // at this point we stop our search at an 'EOL' character, then
         // move the pointer forward ONE character to point to the beginning
         // of the NEXT line after this EOL character.

         buf_pos++;  // now this points to the BEGINNING of a line

         // in order to NORMALIZE things properly, use the
         // 'SetBatchFilePosition()' command.

         return(SetBatchFilePosition(lpBatchInfo,
                                     buf_pos + lpBatchInfo->buf_start));
      }

   } while(TRUE);


   lpBatchInfo->buf_pos = buf_pos;    // ensure 'lpBatchInfo' stays current!
   lpBatchInfo->buf_len = buf_len;

}



void FAR PASCAL AddBatchLabel(LPBATCH_INFO lpBatchInfo, LPSTR lpLabel)
{
LPSTR lp1, lp2;
DWORD dw;


   while(*lpLabel && *lpLabel<=' ') lpLabel++;  // find first non-white-space

   if(*lpLabel!=':')
   {
      PrintErrorMessage(787);
   }

   lpLabel++;

   lp1 = lpLabel + lstrlen(lpLabel);

   while(lp1>lpLabel && *(lp1 - 1)<=' ') *(--lp1) = 0;  // trim string


   if(!lpBatchInfo->lpLabelList)
   {
      lpBatchInfo->lpLabelList = (LPSTR)
                      GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT, 4096);

      lp1 = lpBatchInfo->lpLabelList;
      *lp1 = 0;                          // the end, of course!
   }
   else
   {
      lp1 = lpBatchInfo->lpLabelList;

      while(*lp1)
      {
         if(!_fstricmp(lpLabel, lp1))
         {
            return;  // it's already in there - why continue?
         }
         else
         {
               // point 'lp1' to the next entry

            lp1 += lstrlen(lp1) + 1 + sizeof(long);


              // data stored as:  LABEL <0> <(long)position>
         }

      }

      // ok - lp1 points to the very end of the list now!

      if((dw = GlobalSizePtr(lpBatchInfo->lpLabelList)) <=
         (lp1 - lpBatchInfo->lpLabelList + lstrlen(lpLabel)
           + 16 + sizeof(long)))
      {
         lp2 = (LPSTR)GlobalReAllocPtr(lpBatchInfo->lpLabelList, dw + 4096,
                                       GMEM_MOVEABLE | GMEM_ZEROINIT);
         if(!lp2)
         {
          HCURSOR hOldCursor;

            hOldCursor = MySetCursor(LoadCursor(NULL, IDC_WAIT));

            GlobalCompact((DWORD)-1);

            MySetCursor(hOldCursor);

            if(!(lp2 = (LPSTR)GlobalReAllocPtr(lpBatchInfo->lpLabelList, dw + 4096,
                                               GMEM_MOVEABLE | GMEM_ZEROINIT)))
            {
               // if I cannot re-alloc the thing, free it!

               GlobalFreePtr(lpBatchInfo->lpLabelList);
               lpBatchInfo->lpLabelList = NULL;

               return;
            }
         }

         lp1 = lp2 + (lp1 - lpBatchInfo->lpLabelList);
             // assign lp1 to the 'lp1 offset from lpBatchInfo->lpLabelList'
             // offset from 'lp2' so that it's equivalent to what it was

         lpBatchInfo->lpLabelList = lp2;
      }
   }

   lstrcpy(lp1, lpLabel);
   lp1 += lstrlen(lp1) + 1;

   *((long FAR *)lp1) = lpBatchInfo->buf_start + lpBatchInfo->buf_pos;
      // this assigns the pointer to the statement BELOW the label!

   lp1 += sizeof(long);
   *(lp1) = 0;             // this terminates the list.  I'm done!
   lp1[1] = 0;             // just in case, next is also zero
}


BOOL FAR PASCAL SetBatchFilePosition(LPBATCH_INFO lpBatchInfo, long lFilePos)
{
long bufpos, filepos = lFilePos;



   // check for memory batch files and handle them differently

   if(lpBatchInfo->hFile == HFILE_ERROR) // memory batch files (1/17/95)
   {
      if(lFilePos > (long)lpBatchInfo->buf_len)
      {
         PrintErrorMessage(789);
         return(TRUE);
      }

      lpBatchInfo->buf_pos = (UINT)lFilePos;  // assign new position!

      return(FALSE);
   }


   // calculate 'seek' position in file, and position in buffer

   bufpos = filepos % BATCH_BUFSIZE;
   filepos -= bufpos;


   if(filepos != (long)lpBatchInfo->buf_start)
   {
      _llseek(lpBatchInfo->hFile, filepos, 0);

      if((lpBatchInfo->buf_len =
         _lread(lpBatchInfo->hFile, lpBatchInfo->buf,
                BATCH_BUFSIZE))==(WORD)-1)
      {
         PrintErrorMessage(788);
         return(TRUE);
      }

      lpBatchInfo->buf_start = filepos;
   }

   if(bufpos > (long)lpBatchInfo->buf_len)
   {
      PrintErrorMessage(789);
      return(TRUE);
   }

   lpBatchInfo->buf_pos = (WORD)bufpos;  // assign new position!

   return(FALSE);

}


BOOL FAR PASCAL GotoBatchLabel(LPBATCH_INFO lpBatchInfo, LPSTR lpLabel)
{
LPSTR lp1;


   if(lpBatchInfo->lpLabelList)           // there *is* a label listing!!
   {
      lp1 = lpBatchInfo->lpLabelList;

      while(*lp1)
      {
         if(_fstricmp(lpLabel, lp1)==0)
         {
            lp1 += lstrlen(lp1) + 1;

            return(SetBatchFilePosition(lpBatchInfo, *((long FAR *)lp1)));
         }
         else
         {
               // point 'lp1' to the next entry

            lp1 += lstrlen(lp1) + 1 + sizeof(long);


              // data stored as:  LABEL <0> <(long)position>
         }
      }
   }


   // no labels - gotta do it the *HARD* way!


   if(lpBatchInfo->hFile == HFILE_ERROR)
   {
      lpBatchInfo->buf_pos = 0;         // initial position within buffer
   }
   else
   {
      _llseek(lpBatchInfo->hFile, 0, 0);
      if((lpBatchInfo->buf_len =
          _lread(lpBatchInfo->hFile, lpBatchInfo->buf, BATCH_BUFSIZE))==(WORD)-1)
      {
         PrintErrorMessage(790);
         return(TRUE);
      }

      lpBatchInfo->buf_pos = 0;         // initial position within buffer
      lpBatchInfo->buf_start = 0L;      // initial position within file
   }

   while(lp1 = GetAnotherBatchLine(lpBatchInfo))
   {
      if(*lp1==':')
      {
         AddBatchLabel(lpBatchInfo, lp1);       // add any labels I find

         if(_fstricmp(lpLabel, lp1 + 1)==0)        /*** FOUND!!! ***/
         {
            GlobalFreePtr(lp1);

            return(FALSE);                  /* pointer is now at next line */
         }

      }

      GlobalFreePtr(lp1);
   }

   return(TRUE);

}


void FAR PASCAL TerminateCurrentBatchFile()
{
LPSTR lpTemp;
WORD w1;


   if(lpBatchInfo->hFile != HFILE_ERROR)
   {
      _lclose(lpBatchInfo->hFile);  /* close the file!! */

      lpBatchInfo->buf_pos = 0;     // added 1/17/95 for memory batch file
      lpBatchInfo->buf_start = 0L;  // compatibility...
      lpBatchInfo->buf[0] = 0;

      lpBatchInfo->hFile = HFILE_ERROR;  // CLOSED!
   }

   if(lpBatchInfo->lpLabelList)
   {
      GlobalFreePtr(lpBatchInfo->lpLabelList);
   }

   if(lpBatchInfo->lpBlockInfo)  // IF, CALL/RETURN, user functions, loops
   {
      // free resources for EACH entry in 'lpBlockInfo'

      for(w1=(WORD)lpBatchInfo->lpBlockInfo->dwEntryCount;
          w1 > 0; w1--)
      {
         RemoveCodeBlockEntry(lpBatchInfo->lpBlockInfo->lpCodeBlock + w1 - 1);
      }

      GlobalFreePtr(lpBatchInfo->lpBlockInfo);  // free memory block
   }

   lpTemp = (LPSTR)lpBatchInfo;

   if(SELECTOROF(lpBatchInfo->lpPrev))
   {
      lpBatchInfo = lpBatchInfo->lpPrev;
      GlobalFreePtr(lpTemp);
   }
   else if(OFFSETOF(lpBatchInfo->lpPrev))   // A FLAG!  Used by 'FOR', etc.
   {
      lpBatchInfo = NULL;
      GlobalFreePtr(lpTemp);

            /* Treat like a 'RETURN', except that 'lpBatchInfo' will */
            /* contain a NULL upon termination of the batch file,    */
            /* signaling the 'waiting' process that the batch file   */
            /* has ended!  This is used with the 'For' command to    */
            /* wait until the batch file completes before continuing.*/
   }
   else    // normal 'last' batch file in list!
   {
      lpBatchInfo = NULL;
      GlobalFreePtr(lpTemp);

      BatchMode = FALSE;
      BatchEcho = TRUE;


      if(hMainWnd && IsWindow(hMainWnd))
      {
         DisplayPrompt();   /* displays the prompt at least once */
                            /* if 'BatchMode' disappears!        */
      }

       /** If there is "Type Ahead", Fake typing in **/

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

   }

   UpdateStatusBar();
}






/***************************************************************************/
/*                                                                         */
/*      COMMANDS WHICH AFFECT PROGRAM CONTROL (IF,FOR,WAIT,GOTO,etc.)      */
/*       AND USER-DEFINED FUNCTIONS (including external DLL imports)       */
/*                                                                         */
/***************************************************************************/


// commonly used utilities

BOOL VerifyBlockInfo(LPBLOCK_INFO FAR *lplpBlockInfo)
{
LPBLOCK_INFO lpBI;


   if(!*lplpBlockInfo)
   {
      *lplpBlockInfo = (LPBLOCK_INFO)
               GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                              sizeof(BLOCK_INFO) + sizeof(CODE_BLOCK) * 64L);

      (*lplpBlockInfo)->dwEntryCount = 0;

      return(*lplpBlockInfo == NULL);  // return TRUE on error...
   }
   else if(GlobalSizePtr(*lplpBlockInfo)
           <= (sizeof(BLOCK_INFO) + sizeof(CODE_BLOCK)
               * (*lplpBlockInfo)->dwEntryCount))
   {
      lpBI = (LPBLOCK_INFO)
          GlobalReAllocPtr(*lplpBlockInfo,
                           sizeof(BLOCK_INFO) + sizeof(CODE_BLOCK)
                            * ((*lplpBlockInfo)->dwEntryCount + 64),
                           GMEM_MOVEABLE | GMEM_ZEROINIT);

      if(!lpBI)
      {
         return(TRUE);
      }

      *lplpBlockInfo = lpBI;
   }

   return(FALSE);  // everything's ok!
}

// NOTE:  The following function ensures that *ALL* resources are properly
//        freed when a 'CODE_BLOCK' entry is being removed...

void FAR PASCAL RemoveCodeBlockEntry(CODE_BLOCK FAR *lpCB)
{

   if(!lpCB || !SELECTOROF(lpCB)) return;

   switch(lpCB->dwType & CODEBLOCK_TYPEMASK)
   {
      case CODEBLOCK_DEFINE_EX:   // external function

         if((HMODULE)lpCB->dwStart) FreeLibrary((HMODULE)lpCB->dwStart);
         break;


      default:
         break;
   }

   _hmemset(lpCB, 0, sizeof(*lpCB));

}


// NOTE:  this common utility provides a standard means of determining
//        what a 'term' actually is.

LPSTR FAR PASCAL GetEndOfTerm(LPSTR lp1)
{
LPSTR lp2;
WORD wParen, wBracket;

   wParen = 0;
   wBracket = 0;


   for(lp2 = lp1; *lp2<=' ' && *lp2; lp2++)
      ;  // point 'lp2' to first non-white-space character!

   while(*lp2)    // NEXT, continue searching until 'end of term' found!
   {
      if(*lp2<=' ' && wBracket==0 && wParen==0)  // end of term!
      {
         break;
      }
      else if(*lp2=='(')
      {
         wParen++;
      }
      else if(*lp2=='[')
      {
         wBracket++;
      }
      else if(*lp2==')')
      {
         if(wParen)
         {
            wParen--;
         }
         else
         {
            PrintString(pSYNTAXERROR);
            PrintString(lp1);
            PrintString(pQCRLFLF);
            return(NULL);
         }
      }
      else if(*lp2==']')
      {
         if(wBracket)
         {
            wBracket--;
         }
         else
         {
            PrintString(pSYNTAXERROR);
            PrintString(lp1);
            PrintString(pQCRLFLF);
            return(NULL);
         }
      }

      lp2++;
   }

   return(lp2);
}






/***************************************************************************/
/*             USER-DEFINED FUNCTIONS AND EXTERNAL FUNCTIONS               */
/***************************************************************************/

//#define BSC __based(__segname("_CODE"))
#define BSC CODE_BASED
#define CCBSC const char BSC
#define PCCBSC CCBSC *

static CCBSC szVOID[]      = "VOID";
static CCBSC szBYTE[]      = "BYTE";
static CCBSC szSHORT[]     = "SHORT";
static CCBSC szLONG[]      = "LONG";
static CCBSC szUSHORT[]    = "USHORT";
static CCBSC szULONG[]     = "ULONG";
static CCBSC szDOUBLE[]    = "DOUBLE";
static CCBSC szLDOUBLE[]   = "LDOUBLE";
static CCBSC szLPSTR[]     = "LPSTR";
static CCBSC szGLOBALSTR[] = "GLOBALSTR";

static PCCBSC BSC szDefineDataTypes[]= {
   (PCCBSC)szVOID, (PCCBSC)szBYTE, (PCCBSC)szSHORT, (PCCBSC)szLONG,
   (PCCBSC)szUSHORT, (PCCBSC)szULONG, (PCCBSC)szDOUBLE, (PCCBSC)szLDOUBLE,
   (PCCBSC)szLPSTR, (PCCBSC)szGLOBALSTR};

#undef BSC
#undef CCBSC
#undef PCCBSC




#pragma code_seg("CMDDefine_TEXT","CODE")

BOOL FAR PASCAL CMDDefine  (LPSTR lpCmd, LPSTR lpArgs)
{
LPSTR lp1, lp2, lp3;
LPBLOCK_INFO lpBI;
char szFunctionName[64];
char szParmList[128];

#if 0     /* TEMPORARY TEMPORARY TEMPORARY TEMPORARY TEMPORARY */

WORD w1, w2, wOrdinal, wReturnType;
HMODULE hModule;
FARPROC lpProc;
char szLibName[MAX_PATH];

#endif // 0



   // COMMAND FORMATS:
   //
   // DEFINE name [(param list)] {definition}
   //
   // - or -
   //
   // DEFINE name [(param list)]
   //  ...
   // END DEFINE
   //
   // - or -
   //
   // DEFINE name LIB "dllname" [@ord] {return type} (type list)
   //


   lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   if(!lpBatchInfo)
   {
      PrintString("?'DEFINE' not valid in COMMAND mode!\r\n");
      return(TRUE);
   }


   if(VerifyBlockInfo(&(lpBatchInfo->lpBlockInfo)))
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   lpBI = lpBatchInfo->lpBlockInfo;  // for speed...


   INIT_LEVEL   // a 'break' bails out on error

   // the NEXT keyword will be the function name...

   lp1 = lpArgs;
   while(*lp1 && *lp1<=' ') lp1++;

   if(!*lp1)
   {
      PrintErrorMessage(875); // "?Missing function name in DEFINE\r\n\n"
      break;
   }


   // update 9/24/95 - if I find a '(', stop right there...!

   lp2 = lp1;
   while(*lp2 > ' ' && *lp2 != '(') lp2++;

   _hmemcpy(szFunctionName, lp1, min(sizeof(szFunctionName)-1,lp2 - lp1));
   szFunctionName[min(sizeof(szFunctionName)-1,lp2 - lp1)] = 0;


   while(*lp2 && *lp2<=' ') lp2++;

   if(*lp2)   // next is PARAMETERS or 'LIB xxx'
   {
      lp1 = lp2;
      while(*lp2 > ' ') lp2++;

      if((lp2 - lp1)==3 && !_fstrnicmp(lp1, "LIB", 3))  // LIBRARY??
      {
#if 1     /* TEMPORARY TEMPORARY TEMPORARY TEMPORARY TEMPORARY */

         PrintString("'DEFINE LIB' not supported in this release");
         PrintString(pCRLFLF);
         break;

#else // 1

         // get the library name (in quotes)

         while(*lp2 && *lp2 <= ' ') lp2++;

         if(*lp2 != '\"')  // missing quote mark
         {
            PrintErrorMessage(874); // "?Missing or invalid library name in DEFINE\r\n\n"
            break;
         }

         lp2++;
         lp1 = lp2;
         while(*lp2 && *lp2 != '\"') lp2++;  // ignore backslashes

         if(*lp2 != '\"')
         {
            PrintErrorMessage(874); // "?Missing or invalid library name in DEFINE\r\n\n"
            break;
         }

         _hmemcpy(szLibName, lp1, min(sizeof(szLibName)-1,lp2 - lp1));
         szLibName[min(sizeof(szLibName)-1,lp2 - lp1)] = 0;

         lp2++;
         while(*lp2 && *lp2 <=' ') lp2++;

         wOrdinal = 0;

         if(*lp2 == '@') // ORDINAL!!
         {
            lp1 = ++lp2;

            while(*lp2 >' ')
            {
               if(*lp2 < '0' || *lp2 > '9')
               {
                  break;
               }
            }

            if(*lp2 > ' ')
            {
               PrintErrorMessage(877); // "?Invalid '@ordinal' value in DEFINE command for external function\r\n\n"
               break;
            }

            while(*lp2 && *lp2<=' ') lp2++;
         }


         if(!*lp2)
         {
            PrintErrorMessage(876); // "?Missing return data type in DEFINE command for external function\r\n\n"

            break;
         }

         lp1 = lp2;

         while(*lp2 > ' ') lp2++;

         if(*lp2) *(lp2++) = 0; // I cheated - oh, well...

         for(wReturnType=0; wReturnType<N_DIMENSIONS(szDefineDataTypes);
             wReturnType++)
         {
            if(!_fstricmp(szDefineDataTypes[wReturnType], lp1)) break;
         }

         if(wReturnType >= N_DIMENSIONS(szDefineDataTypes))
         {
          static const char CODE_BASED szMsg1[]=
                                                   "?Invalid return type \"";
          static const char CODE_BASED szMsg2[]=
                         "\" in DEFINE command for external functions\r\n\n";

            PrintString(szMsg1);
            PrintString(lp1);
            PrintString(szMsg2);

            break;
         }

         while(*lp2 && *lp2 <=' ') lp2++;

         if(!*lp2)
         {
            PrintErrorMessage(878);  // "?Missing parameter list in DEFINE\r\n\n"
            break;
         }
         else if(*lp2 != '(')
         {
            PrintErrorMessage(879);  // "?Parameter list syntax error in DEFINE\r\n\n"

            break;
         }

         lp1 = ++lp2;

         while(*lp2 && *lp2 != ')') lp2++;
         if(*lp2 != ')')
         {
            PrintErrorMessage(879);  // "?Parameter list syntax error in DEFINE\r\n\n"

            break;
         }

         _hmemcpy(szParmList, lp1, min(sizeof(szParmList)-1,lp2 - lp1));
         szParmList[min(sizeof(szParmList)-1,lp2 - lp1)] = 0;

         lp2++;

         while(*lp2 && *lp2 <= ' ') lp2++;  // rest of line blank?
         if(*lp2)
         {
            PrintErrorMessage(879);  // "?Parameter list syntax error in DEFINE\r\n\n"

            break;
         }


         // all of the parameters have been parsed.  At this point, I need
         // to add the entry into the 'LOOP/DEFINE' table.  This also
         // means loading the library once per function.

         lpBI->lpCodeBlock[lpBI->dwEntryCount].dwStart = (DWORD)
                                                    LoadLibrary(szLibName);

         if(lpBI->lpCodeBlock[lpBI->dwEntryCount].dwStart <= 32)
         {
            PrintErrorMessage(896); // "DEFINE ERROR - LIB not found\r\n"

            break;
         }

         lpBI->lpCodeBlock[lpBI->dwEntryCount].dwBlockEnd = (DWORD)
           GetProcAddress((HMODULE)lpBI->lpCodeBlock[lpBI->dwEntryCount].dwStart,
                          wOrdinal?MAKEINTRESOURCE(wOrdinal):szFunctionName);

         if(!lpBI->lpCodeBlock[lpBI->dwEntryCount].dwBlockEnd)
         {
            FreeLibrary((HMODULE)lpBI->lpCodeBlock[lpBI->dwEntryCount].dwStart);

            PrintErrorMessage(897);

            break;
         }

         // assign correct type to the 'code block' entry

         lpBI->lpCodeBlock[lpBI->dwEntryCount].dwType = CODEBLOCK_DEFINE_EX;
         lp1 = lpBI->lpCodeBlock[lpBI->dwEntryCount].szInfo;

         // copy function name into 'szInfo' (first part)

         lstrcpy(lp1, szFunctionName);
         lp1 += lstrlen(lp1) + 1;      // point just past '\0'

         // next, add argument type list
         lstrcpy(lp1, szParmList);

         // I am done - increment counter!
         lpBI->dwEntryCount++;

         GlobalFreePtr(lpArgs);
         return(FALSE);  // everything's ok!

#endif // 0

      }

      // it's parameters - check syntax and store them

      lp2 = lp1;  // restore previous value for 'lp2' (points to '(')

      if(!*lp2)
      {
         PrintErrorMessage(878);  // "?Missing parameter list in DEFINE\r\n\n"
         break;
      }
      else if(*lp2 != '(')
      {
         PrintErrorMessage(879);  // "?Parameter list syntax error in DEFINE\r\n\n"

         break;
      }

      lp1 = ++lp2;

      while(*lp2 && *lp2 != ')') lp2++;
      if(*lp2 != ')')
      {
         PrintErrorMessage(879);  // "?Parameter list syntax error in DEFINE\r\n\n"

         break;
      }

      _hmemcpy(szParmList, lp1, min(sizeof(szParmList)-1,lp2 - lp1));
      szParmList[min(sizeof(szParmList)-1,lp2 - lp1)] = 0;

      lp2++;

   }

   lpBI->lpCodeBlock[lpBI->dwEntryCount].dwType = CODEBLOCK_DEFINE;

   lp1 = lpBI->lpCodeBlock[lpBI->dwEntryCount].szInfo;

   // copy functino name into 'szInfo' (first part)

   lstrcpy(lp1, szFunctionName);
   lp1 += lstrlen(lp1) + 1;      // point just past '\0'

   // next, add argument type list
   lstrcpy(lp1, szParmList);
   lp1 += lstrlen(lp1) + 1;      // point just past '\0' again


   while(*lp2 && *lp2 <= ' ') lp2++;  // rest of line blank?
   if(*lp2)
   {
      // this is a 'single line' DEFINE.  Store the remainder of the line
      // in 'szInfo', following the function name

      lpBI->lpCodeBlock[lpBI->dwEntryCount].dwStart = (DWORD)-1L;  // flag 'inline'

      if((UINT)lstrlen(lp2) >= (sizeof(lpBI->lpCodeBlock->szInfo) - 1 -
                      (lp1 - lpBI->lpCodeBlock[lpBI->dwEntryCount].szInfo)))
      {
         PrintErrorMessage(897);  // "DEFIfNE ERROR - inline function text is too long\r\n"

         break;
      }

      lstrcpy(lp1, lp2);

   }
   else
   {
      // point 'dwStart' to the beginning of the NEXT item to be read from
      // the batch file.  Then, read the batch file until I find an
      // 'END DEFINE' statement.

      lpBI->lpCodeBlock[lpBI->dwEntryCount].dwStart =
                               lpBatchInfo->buf_start + lpBatchInfo->buf_pos;

      *lp1 = 0;  // terminate with TWO nulls

      while(lp1 = GetAnotherBatchLine(lpBatchInfo))
      {
         for(lp2=lp1; *lp2 && *lp2 <= ' '; lp2++)
            ;   // find first non-white-space

         for(lp3=lp2; *lp3 > ' '; lp3++)
            ;   // find next white space

         if((lp3 - lp2) == 3 && !_fstrnicmp(lp2, "END", 3))  // found 'END'
         {
            for(lp2=lp3; *lp2 && *lp2 <= ' '; lp2++)
               ;  // find next non-white-space

            for(lp3=lp2; *lp3 > ' '; lp3++)
               ;  // find next white space

            if((lp3 - lp2) == 6 && !_fstrnicmp(lp2, "DEFINE", 6)) // found 'DEFINE'
            {
               while(*lp3 && *lp3 <= ' ') lp3++;

               if(*lp3)  // syntax error - warning only!
               {
                  PrintErrorMessage(898); // "END DEFINE - extraneous characters found on line, ignored.\r\n"
               }

               GlobalFreePtr(lp1);

               break;  // break out of main loop!
            }
         }

         GlobalFreePtr(lp1);  // always need to free this
      }

      if(!lp1)
      {
         PrintErrorMessage(899);  // "DEFINE ERROR - No 'END DEFINE' found\r\n"

         break;
      }
   }


   // point 'dwBlockEnd' to the beginning of the NEXT item to be read from
   // the batch file.  This applies for EITHER 'inline' or 'block' mode.
   // Also, increment 'dwEntryCount'.

   lpBI->lpCodeBlock[lpBI->dwEntryCount++].dwBlockEnd =
                               lpBatchInfo->buf_start + lpBatchInfo->buf_pos;

   GlobalFreePtr(lpArgs);
   return(FALSE);                       // everything's ok!


   END_LEVEL   // error bailouts go here

   GlobalFreePtr(lpArgs);

   return(TRUE);
}



/***************************************************************************/
/*                'END' command - used in various contexts                 */
/***************************************************************************/

// END DEFINE - forces a RETURN ""
// END IF - dispatches CMDEndif
// END WHILE - alternate for 'WEND'


BOOL FAR PASCAL CMDEnd     (LPSTR lpCmd, LPSTR lpArgs)
{
LPBLOCK_INFO lpBI;
LPSTR lp1, lp2;
WORD w1;
BOOL rval;


   lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   if(!lpBatchInfo)
   {
      PrintString("?'END' not valid in COMMAND mode!\r\n");
      return(TRUE);
   }

   // find out which suffix is on 'END'

   for(lp1=lpArgs; *lp1 && *lp1 <=' '; lp1++)
      ; // first non-white-space

   for(lp2=lp1; *lp2 > ' '; lp2++)
      ; // next white space

   w1 = (lp2 - lp1);

   if(w1==2 && !_fstrnicmp(lp1, "IF", w1))
   {
      while(*lp2 && *lp2 <= ' ') lp2++;  // point to next non-white-space

      rval = CMDEndif(lpCmd, lp2);       // pass remainder of line to CMDEndif()

      GlobalFreePtr(lpArgs);
      return(rval);
   }


   // {reserved}  place other 'END' options here  {reserved}

   else if(w1 != 6 || _fstrnicmp(lp1, "DEFINE", w1))
   {
      // it is *NOT* an 'END DEFINE'

      PrintAChar('\"');
      PrintString(lpArgs);
      PrintErrorMessage(900); // "\" - invalid option for 'END' command\r\n"

      GlobalFreePtr(lpArgs);
      return(TRUE);
   }



   while(*lp2 && *lp2 <= ' ') lp2++;  // point to next non-white-space
   if(*lp2)
   {
      PrintErrorMessage(898); // "END DEFINE - extraneous characters found on line, ignored.\r\n\n"
   }


   // process an 'END DEFINE' command - effectively, 'RETURN ""'

   rval = TRUE;

   if(!lpBatchInfo)
   {
      PrintErrorMessage(578);  //  "?Not valid in COMMAND mode\r\n\n"
   }
   else if(!lpBatchInfo->lpBlockInfo)
   {
      PrintErrorMessage(901);  // "'END DEFINE' not valid outside of DEFINE block\r\n"
   }
   else
   {
      lpBI = lpBatchInfo->lpBlockInfo;

      if(MthreadGetCurrentThread() == hBatchThread)  // we're NOT in 'function' mode
      {
         PrintErrorMessage(901);  // "'END DEFINE' not valid outside of DEFINE block\r\n"
      }
      else
      {
         rval = CMDReturn(lpCmd, "\"\"");   // force a 'RETURN ""'
      }
   }


   GlobalFreePtr(lpArgs);
   return(rval);
}




/***************************************************************************/
/*         LOOPS - 'FOR', 'FOR/NEXT', 'WHILE/WEND', 'REPEAT/UNTIL'         */
/***************************************************************************/



#pragma code_seg("CMDFor_TEXT","CODE")


BOOL FAR PASCAL CMDFor     (LPSTR lpCmd, LPSTR lpArgs)
{
LPSTR lp1, lp2, lp3, lp4, lp5, lp6, lp7, lpCommand, lpNewCmd, lpFileSpec,
      lpTemp;
WORD wNFileSpec=0, i, j, w, w2, attr, wNDirInfo;
BOOL NotFlag=FALSE, FoundFlag, ErrorFlag, PercentFlag;
char pVar[32]={0};
LPDIR_INFO plpDI[64]={NULL}, lpDI;   /* maximum of 64 file specs */
WORD pNDI[64];                       /* file count for 'plpDI[]' */
LPSTR plpPaths[64]={NULL};           /* pointer to paths */
#define lpTempBuf ((LPSTR)plpPaths)  /* shared usage for 'plpPaths[]' */



   /* FOR %v [NOT] IN (fileset) [DO {command line}] */
   /* FOR [%]v FROM min TO max [STEP step] [DO {command line}] */

   if(*lpArgs!='%')
   {
      PercentFlag = FALSE;
   }
   else
   {
      PercentFlag = TRUE;
   }

   /* lpArgs must point to the '%v' - {command line} gets */
   /* translated using '%v' first, then the 'normal' way. */


                     /** DETECT AND RECORD %VARIABLE **/

   for(lp1=lpArgs; *lp1>' '; lp1++)
      ; /* find the first 'white space' character */

   _hmemcpy((LPSTR)pVar, lpArgs, lp1 - lpArgs);

   pVar[lp1 - lpArgs] = 0;   /* 'pVar' contains the variable text */


   while(*lp1 && *lp1<=' ') lp1++;  /* find next non-white-space char */

   if(!*lp1)
   {
      PrintString(pSYNTAXERROR);
      PrintString(lpArgs);
      PrintString(pQCRLFLF);
      return(TRUE);
   }

   for(lp2=lp1; *lp2>' '; lp2++)
      ;                           /* find next white-space char */

                      /** DETECT 'NOT' (optional) **/

   if(*lp2 && (lp2 - lp1)==3 && !_fstrnicmp(lp1, "NOT", 3))
   {
      NotFlag = TRUE;

      while(*lp2 && *lp2<=' ') lp2++;  /* find next non-white-space char */

      for(lp1=lp2; *lp2>' '; lp2++)  /* assign 'lp1' to 'lp2', and */
         ;                           /* find next white-space char */

   }
   else
   {
      NotFlag = FALSE;
   }



                      /** FIND AND VERIFY 'FROM' **/

   if(*lp1 && (lp2 - lp1)==4 && !_fstrnicmp(lp1, "FROM", 4))
   {
    double dMin, dMax, dStep, dCtr;


      while(*lp2 && *lp2<=' ') lp2++;  /* find next non-white-space char */

      // FIND NEXT WHITE SPACE OUTSIDE OF '()' OR '[]'

      lp1 = lp2;

      lp2 = GetEndOfTerm(lp1);
      if(!lp2) return(TRUE);

//      for(lp1=lp2; *lp2>' '; lp2++)  /* assign 'lp1' to 'lp2', and */
//         ;                           /* find next white-space char */

      // GET 'FROM' VALUE

      _hmemcpy(lpTempBuf, lp1, lp2 - lp1);
      lpTempBuf[lp2 - lp1] = 0;      /* now contains 'start' value */

      lp3 = Calculate(lpTempBuf);
      if(!lp3)
      {
         PrintString(pSYNTAXERROR);
         PrintString(lpTempBuf);
         PrintString(pQCRLFLF);
         return(TRUE);
      }

      dMin = CalcEvalNumber(lp3);
      GlobalFreePtr(lp3);


      while(*lp2 && *lp2<=' ') lp2++;  /* find next non-white-space char */


      for(lp1=lp2; *lp2>' '; lp2++)  /* assign 'lp1' to 'lp2', and */
         ;                           /* find next white-space char */


      // FIND THE WORD 'TO'

      if(!*lp1 || (lp2 - lp1)!=2 || _fstrnicmp(lp1, "TO", 2))
      {
         PrintString(pSYNTAXERROR);
         PrintString(lpArgs);
         PrintString(pQCRLFLF);
         return(TRUE);
      }


      while(*lp2 && *lp2<=' ') lp2++;  /* find next non-white-space char */


      // FIND NEXT WHITE SPACE OUTSIDE OF '()' OR '[]'

      lp1 = lp2;

      lp2 = GetEndOfTerm(lp1);
      if(!lp2) return(TRUE);

//      for(lp1=lp2; *lp2>' '; lp2++)  /* assign 'lp1' to 'lp2', and */
//         ;                           /* find next white-space char */

      // GET 'TO' VALUE

      _hmemcpy(lpTempBuf, lp1, lp2 - lp1);
      lpTempBuf[lp2 - lp1] = 0;      /* now contains 'start' value */

      lp3 = Calculate(lpTempBuf);
      if(!lp3)
      {
         PrintString(pSYNTAXERROR);
         PrintString(lpTempBuf);
         PrintString(pQCRLFLF);
         return(TRUE);
      }

      dMax = CalcEvalNumber(lp3);
      GlobalFreePtr(lp3);


      while(*lp2 && *lp2<=' ') lp2++;  /* find next non-white-space char */

      for(lp1=lp2; *lp2>' '; lp2++)  /* assign 'lp1' to 'lp2', and */
         ;                           /* find next white-space char */


      // is this a 'STEP' or a 'DO'?

      if(*lp1 && (lp2 - lp1)==4 && !_fstrnicmp(lp1, "STEP", 4))
      {
         while(*lp2 && *lp2<=' ') lp2++;  /* find next non-white-space char */

         // FIND NEXT WHITE SPACE OUTSIDE OF '()' OR '[]'

         lp1 = lp2;

         lp2 = GetEndOfTerm(lp1);
         if(!lp2) return(TRUE);

//         for(lp1=lp2; *lp2>' '; lp2++)  /* assign 'lp1' to 'lp2', and */
//            ;                           /* find next white-space char */

         // GET 'STEP' VALUE

         _hmemcpy(lpTempBuf, lp1, lp2 - lp1);
         lpTempBuf[lp2 - lp1] = 0;      /* now contains 'start' value */

         lp3 = Calculate(lpTempBuf);
         if(!lp3)
         {
            PrintString(pSYNTAXERROR);
            PrintString(lpTempBuf);
            PrintString(pQCRLFLF);
            return(TRUE);
         }

         dStep = CalcEvalNumber(lp3);
         GlobalFreePtr(lp3);


         while(*lp2 && *lp2<=' ') lp2++;  /* find next non-white-space char */

         for(lp1=lp2; *lp2>' '; lp2++)  /* assign 'lp1' to 'lp2', and */
            ;                           /* find next white-space char */
      }
      else
      {
         if(dMax >= dMin) dStep = 1.0;
         else             dStep = -1.0;
      }

      while(*lp1 && *lp1<=' ') lp1++;  // just in case...

      if(!*lp1)               // FOR/NEXT version of command!!
      {
         if(PercentFlag)      // THIS IS *NOT* ALLOWED!!
         {
            PrintString("?Use of '%var' in 'FOR/NEXT' is not allowed\r\n\n");
            return(TRUE);
         }
         else if(!BatchMode || !lpBatchInfo)
         {
            PrintErrorMessage(578); // NOT VALID IN COMMAND MODE
            return(TRUE);
         }

         // place the location of the statement following the FOR, the
         // variable name, step value, and limit value into the LOOP STACK
         //
         // NOTE:  This is only valid for batch files
         //

         if(VerifyBlockInfo(&(lpBatchInfo->lpBlockInfo)))
         {
            PrintString(pNOTENOUGHMEMORY);
            ErrorFlag = TRUE;
         }
         else if(!(lp3 = StringFromFloat(dMin))) // start value for loop var!
         {
            PrintString(pNOTENOUGHMEMORY);
            ErrorFlag = TRUE;
         }
         else
         {
          LPBLOCK_INFO lpBI = lpBatchInfo->lpBlockInfo;

            SetEnvString(pVar, lp3);
            GlobalFreePtr(lp3);

            w2 = (WORD)lpBI->dwEntryCount++;

            // record next line's batch file position in 'dwStart'

            lpBI->lpCodeBlock[w2].dwStart = lpBatchInfo->buf_start
                                          + lpBatchInfo->buf_pos;

            lpBI->lpCodeBlock[w2].dwBlockEnd = (DWORD)-1L;  // a flag

            lp1 = lpBI->lpCodeBlock[w2].szInfo;

            // NOTE:  the following values are stored in 'szInfo':
            // bytes 0-n:         NULL terminated string (variable name)
            // byte n+1 to n+8:   DOUBLE value (start)
            // byte n+9 to n+16:  DOUBLE value (limit)
            // byte n+17 to n+24: DOUBLE value (step value)
            // NOTE:  a negative STEP value indicates limit is 'low' limit,
            //        and a positive STEP value indicates it is 'high' limit.

            lstrcpy(lp1, pVar);  // variable name
            lp1 += lstrlen(lp1) + 1;

            ((double FAR *)lp1)[0] = dMin;  // actually the COUNTER
            ((double FAR *)lp1)[1] = dMax;
            ((double FAR *)lp1)[2] = dStep;

            lpBI->lpCodeBlock[w2].dwType = CODEBLOCK_FOR;  // it's a FOR/NEXT

            ErrorFlag = FALSE;
         }
      }
      else                    // STANDARD version of command!!
      {

         if(toupper(*lp1)!='D' || toupper(lp1[1])!='O' || !lp1[2] || lp1[2]>' ')
         {
            PrintString(pSYNTAXERROR);
            PrintString(lpArgs);
            PrintString(pQCRLFLF);
            return(TRUE);
         }

         for(lp1 += 3; *lp1 && *lp1<=' '; lp1++)
            ;                             /* find next non-white-space char */

                           /** STORE 'command' STRING **/

         lpCommand = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                           2 * lstrlen(lp1) + 256);

         if(!lpCommand)
         {
            PrintString(pNOTENOUGHMEMORY);
            return(TRUE);
         }

         lpNewCmd = lpCommand + lstrlen(lp1) + 2;  /* where 'updated' cmd goes */

         lstrcpy(lpCommand, lp1); /* make a copy of the 'original' command line */



         BlockCopyResults = TRUE;


         for(dCtr = dMin; (dStep >= 0 && dCtr <= dMax) ||
                          (dStep < 0 && dCtr >= dMax); dCtr += dStep)
         {
            if(PercentFlag)
            {
               /** STEP 1:  find every occurrence of 'pVariable' and **/
               /**          substitute the full pathname for it.     **/

               lp1 = lpCommand;
               lp2 = lpNewCmd;

               while(*lp1)
               {
                  lp3 = _fstrchr(lp1, *pVar);
                  if(!lp3) break;     /* find next possible occurrence of 'pVar' */

                  j = lp3 - lp1;    /* length of string up to 1st char of 'pVar' */

                  if(_fstrnicmp(lp3 + 1, pVar + 1, lstrlen(pVar + 1))) /* match? */
                  {
                     j++;       /* add 1 to capture 1st char of 'pVar' also */

                     _hmemcpy(lp2, lp1, j);
                     *(lp2 += j) = 0;

                     lp1 += j;
                  }
                  else          /* found 'pVar' within this string!! */
                  {
                     _hmemcpy(lp2, lp1, j);
                     lp2 += j;

                     lp1 += j + lstrlen(pVar); /* point 'just past' the variable */

                     lp3 = StringFromFloat(dCtr);
                     lstrcpy(lp2, lp3);

                     GlobalFreePtr(lp3);

                     lp2 += lstrlen(lp2);
                  }
               }

               if(*lp1) lstrcpy(lp2, lp1);     /* if any left, add to 'lpNewCmd' */

            }
            else       // USING A VARIABLE NAME INSTEAD OF A '%v'
            {
               lp3 = StringFromFloat(dCtr);

               SetEnvString(pVar, lp3);

               GlobalFreePtr(lp3);

               lstrcpy(lpNewCmd, lpCommand); // necessary to use a COPY, not
                                             // the original.
            }

            // at this point 'pVar' has the correct value, either by
            // substitution or assignment.  Now, evaluate!

            lp1 = EvalString(lpNewCmd);

            if(!lp1)
            {
               PrintErrorMessage(582);
               ErrorFlag = TRUE;
               break;
            }

               /** now for some more fun - evaluate the puppy! **/


            IsCall = TRUE;    /* forces 'call' command to be in effect */
            IsFor  = TRUE;    /* forces new batch file to complete first */

            ErrorFlag = ProcessCommand(lp1);

            IsCall = FALSE;   /* 'call' command no longer in effect */
            IsFor  = FALSE;   /* reset the 'for' flag also */

            if(ErrorFlag)
            {
               PrintErrorMessage(583);
               GetUserInput(work_buf);

               if(ctrl_break || (*work_buf!='Y' && *work_buf!='y'))
               {
                  if(!BatchMode) ctrl_break = FALSE;
                  break;
               }
               else
               {
                  ErrorFlag = FALSE;  /* reset the error flag */
               }
            }


            GlobalFreePtr(lp1);  /* free ptr allocated by 'EvalString()' */

            /** and, for the final performance, do a LoopDispatch(), **/
            /** then check the 'ctrl_break' flag, bailing out if set **/

            UpdateWindow(hMainWnd);    /* ensure the screen is updated */
            LoopDispatch();           /* allow the user to intervene... */

            if(ctrl_break)
            {
               PrintErrorMessage(584);
               if(!BatchMode) ctrl_break = FALSE;
               ErrorFlag = TRUE;
               break;
            }

         }

         BlockCopyResults = FALSE;
         if(TotalCopiesSubmitted) ReportCopyResults();

         GlobalFreePtr(lpCommand);

      }

      return(ErrorFlag);
   }


                     /** NORMAL 'FOR %v IN' SYNTAX **/
                     /**   FIND AND VERIFY 'IN'    **/


   if(!PercentFlag)                // must have '%v' (for now...)
   {
      PrintString(pSYNTAXERROR);
      PrintString(lpArgs);
      PrintString(pQCRLFLF);
      return(TRUE);
   }

   if(!*lp1 || (lp2 - lp1)!=2 || _fstrnicmp(lp1, "IN", 2))
   {
      PrintString(pSYNTAXERROR);
      PrintString(lpArgs);
      PrintString(pQCRLFLF);
      return(TRUE);
   }


                   /** OBTAIN (file set) INFORMATION **/

   while(*lp2 && *lp2<=' ') lp2++;  /* find next non-white-space char */
   lp1 = lp2;                 /* assign lp1 to what should be the '(' */

   if(*lp1!='(')
   {
      PrintString(pSYNTAXERROR);
      PrintString(lpArgs);
      PrintString(pQCRLFLF);
      return(TRUE);
   }

   for(lp3=lp1+1; *lp3 && *lp3<=' '; lp3++)
      ;                             /* find next non-white-space char */

   lp2 = _fstrchr(lp1 + 1, ')');  /* find the ending parenthesis */

   if(!lp2 || lp3==lp2)         /* either no ')', or '()', or '(   )' */
   {
      PrintString(pSYNTAXERROR);
      PrintString(lpArgs);
      PrintString(pQCRLFLF);
      return(TRUE);
   }

                      /** FIND AND DETECT 'DO' **/

   for(lp3 = lp2 + 1; *lp3 && *lp3<=' '; lp3++)
      ;                             /* find next non-white-space char */

   if(toupper(*lp3)!='D' || toupper(lp3[1])!='O' || !lp3[2] || lp3[2]>' ')
   {
      PrintString(pSYNTAXERROR);
      PrintString(lpArgs);
      PrintString(pQCRLFLF);
      return(TRUE);
   }

   for(lp3 += 3; *lp3 && *lp3<=' '; lp3++)
      ;                             /* find next non-white-space char */

                     /** STORE 'command' STRING **/

   lpCommand = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                      2 * lstrlen(lp3) + 256);

   if(!lpCommand)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   lpNewCmd = lpCommand + lstrlen(lp3) + 2;  /* where 'updated' cmd goes */

   lstrcpy(lpCommand, lp3); /* make a copy of the 'original' command line */

                       /** PARSE OUT FILE SPEC **/


   attr = !_A_VOLID & !_A_HIDDEN & !_A_SUBDIR & 0xff;
                   /* hidden, volid, dir excluded - any combination matches */


         /* at this point 'lp1' points to '(' and 'lp2' points to ')' */
         /*  it needs to be converted if the string contains '%var%'  */

   _hmemcpy(lpNewCmd, lp1 + 1, lp2 - lp1 - 1);
   lpNewCmd[lp2 - lp1 - 1] = 0;

   lpFileSpec = EvalString(lpNewCmd);  /* evaluate and copy string! */
   _fstrupr(lpFileSpec);

   if(NotFlag)
   {
      if(!(w = GetDirList("*.*", attr, plpDI, NULL)) ||
         w==DIRINFO_ERROR)
      {
         if(!w)
         {
            PrintString(pNOMATCHINGFILES);
         }
         else
         {
            PrintErrorMessage(577);
         }
         GlobalFreePtr(lpCommand);
         GlobalFreePtr(lpFileSpec);
         return(w==DIRINFO_ERROR);
      }

      lpTemp = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT, MAX_PATH * 3);

      if(!lpTemp)
      {
         PrintString(pNOTENOUGHMEMORY);

         GlobalFreePtr(*plpDI);
         GlobalFreePtr(lpCommand);
         GlobalFreePtr(lpFileSpec);
         return(TRUE);
      }


      lp1= lpFileSpec;
      lp3 = lpTemp + MAX_PATH * 2;

      FoundFlag = FALSE;              /* 'TRUE' if anything removed */

      DosQualifyPath(lp3, ".");  /* get the 'TRUENAME' of current dir! */

      if(!*lp3 || lp3[lstrlen(lp3)-1]!='\\')
      {
         lstrcat(lp3, pBACKSLASH);
      }

      while(*lp1)
      {
         while(*lp1 && *lp1 <= ' ') lp1++;  // clean up any lead wht space
         if(!*lp1) break;

         if(*lp1 == '\"')  // file name in quotes...
         {
            lp2 = ++lp1;

            while(*lp2 && *lp2 != '\"') lp2++;

            _hmemcpy(lpTemp, lp1, lp2 - lp1);

            lpTemp[lp2 - lp1] = 0;

            if(*lp2) lp2++;
         }
         else
         {

            for(lp2=lp1; *lp2 && *lp2>' ' && *lp2!=','; lp2++)
               ;                                     /* find end of parm */

            _hmemcpy(lpTemp, lp1, lp2 - lp1);

            lpTemp[lp2 - lp1] = 0;
         }

               /* point 'lp1' at next parm or NULL byte */

         if(*lp2==',') lp2++;             /* point past ',' if it is one */
         while(*lp2 && *lp2<=' ') lp2++;  /* find next non-white-space */

         lp1 = lp2;                       /* ready for next iteration */

         DosQualifyPath(lpTemp + MAX_PATH, lpTemp);  /* generate 'qualified' path */
         ConvertAsterisks(lpTemp + MAX_PATH); // convert "*.*" to "????????.???"


              /* TRUENAME path must match current, with no '\' */
              /* following the current path.                   */

         if(!lpTemp[MAX_PATH] ||       // this happens on invalid drive error
            _fstrnicmp(lp3, lpTemp + MAX_PATH, lstrlen(lp3))!=0 ||
            _fstrchr(lp4 = lpTemp + MAX_PATH + lstrlen(lp3), '\\'))
         {
            PrintErrorMessage(579);
            GlobalFreePtr(*plpDI);
            GlobalFreePtr(lpTemp);
            GlobalFreePtr(lpCommand);
            GlobalFreePtr(lpFileSpec);
            return(TRUE);
         }
             /* obtain file spec, pad with spaces, and transfer it */

         for(lp5=lp4, lp6=lpTemp, i=0; *lp5 && *lp5!='.' && i<8; i++)
         {
            *lp6++ = *lp5++;
         }

         for(;i<8; i++)
         {
            *lp6++ = ' ';   /* finish padding with spaces if required */
         }

         while(*lp5 && *lp5!='.') lp5++;  /* trim 'extraneous' characters */

         if(*lp5) lp5++;    /* increment this one, too, if necessary */

         for(i=0; *lp5 && i<3; i++)
         {
            *lp6++ = *lp5++;
         }

         for(;i<3; i++)     /* pad extension with spaces */
         {
            *lp6++ = ' ';
         }

         *lp6 = 0;          /* done!  8 char name, dot, 3 char extension */
                            /* with space padding and '?' wildcards only */

              /* now that we have such a file spec, go through */
              /* the directory listing and remove any files    */
              /* that match our file specification here.       */

         for(lpDI = *plpDI, i=0; i<w;)
         {
                      /* see if file names match */

            for(lp5=lpTemp, lp6=lpDI->name, j=0; j<8; j++, lp5++, lp6++)
            {
               if(*lp5!=*lp6 && *lp5!='?') break;  /* no match */
            }

            if(j>=8)
            {
                    /* see if file extensions match */

               for(lp5=lpTemp + 8, lp6=lpDI->ext, j=0; j<3; j++, lp5++, lp6++)
               {
                  if(*lp5!=*lp6 && *lp5!='?') break;  /* no match */
               }

               if(j>=3)
               {

                  w--;            /* decrement total number of entries */

                  if(i<w)
                  {
                             /* 'pull back' remaining entries */
                     _hmemcpy((LPSTR)lpDI, (LPSTR)(lpDI + 1),
                               (w - i) * sizeof(*lpDI));
                  }

                     /* over-write previous final entry with NULLS */
                  _hmemset((LPSTR)((*plpDI) + w), 0, sizeof(*lpDI));

                  FoundFlag = TRUE;    /* marks that something was removed */
                  continue;        /* skip all of the increments and so on */
               }
            }

            i++;      /* increment counters only if nothing was deleted */
            lpDI++;   /* otherwise they point to the next one anyway... */
         }
      }

      if(!w || !FoundFlag)  /* either no files left, or no files removed */
      {
         if(!w)     /* no files left */
         {
            PrintString(pNOMATCHINGFILES);
         }
         else       /* no files removed - verify user wants '*.*' */
         {
            PrintErrorMessage(580);

            GetUserInput(work_buf);
         }

         if(!w || (*work_buf!='y' && *work_buf!='Y') || ctrl_break)
         {
            if(!BatchMode) ctrl_break = FALSE;

            GlobalFreePtr(*plpDI);
            GlobalFreePtr(lpTemp);
            GlobalFreePtr(lpCommand);
            GlobalFreePtr(lpFileSpec);
            return(FALSE);
         }
      }


      pNDI[0] = w;           /* the final count!! */
      lstrcpy(lpTemp, lp3);
      *plpPaths = lpTemp;        /* now it's the path spec */
      wNDirInfo = 1;         /* always only 1 'LPDIR_INFO' array */
   }
   else
   {
      lp1= lpFileSpec;
      FoundFlag = FALSE;              /* 'TRUE' if anything removed */
      wNDirInfo = 0;                  /* current count, eh? */

      while(*lp1)
      {
         if(*lp1 == '\"')  // file name in quotes...
         {
            lp2 = ++lp1;

            while(*lp2 && *lp2 != '\"') lp2++;

            _hmemcpy(work_buf, lp1, lp2 - lp1);

            work_buf[lp2 - lp1] = 0;

            if(*lp2) lp2++;
         }
         else
         {

            for(lp2=lp1; *lp2 && *lp2>' ' && *lp2!=','; lp2++)
               ;                                     /* find end of parm */

            _hmemcpy(work_buf, lp1, lp2 - lp1);

            work_buf[lp2 - lp1] = 0;
         }

               /* point 'lp1' at next parm or NULL byte */

         if(*lp2) lp2++;                  /* point past ',' if it is one */
         while(*lp2 && *lp2<=' ') lp2++;  /* find next non-white-space */

         lp1 = lp2;                       /* ready for next iteration */

          /** 'work_buf' contains the criteria.  Get dir info **/

         lp3 = work_buf + sizeof(work_buf)/2;  /* temporary */

      /* important thing to note:  if the file name contains wildcard */
      /* characters, then SOMETHING must exist - but, if it does not  */
      /* contain wild-card characters, then it doesn't need to exist. */
      /* Further, I don't fully qualify non-wildcard names. So there. */
      /* (file must also NOT be '.' or '..' at end of path...)        */

         if(_fstrchr(work_buf,'?') || _fstrchr(work_buf,'*') ||
            ((lp7 = _fstrrchr(work_buf,'\\')) &&
                        (!_fstrcmp(lp7+1,".") || !_fstrcmp(lp7+1,".."))) ||
            (!lp7 && (lp7 = _fstrchr(work_buf,':')) &&
                        (!_fstrcmp(lp7+1,".") || !_fstrcmp(lp7+1,".."))) ||
            (!lp7 && (!_fstrcmp(work_buf,".") || !_fstrcmp(work_buf,".."))))
         {
            w = GetDirList0(work_buf, attr, plpDI + wNDirInfo, lp3, TRUE, NULL);

            if(w && w!=DIRINFO_ERROR)
            {
               if(lp3[lstrlen(lp3) - 1]!='\\')
               {
                  lstrcat(lp3, pBACKSLASH);
               }
            }
         }
         else
         {
            w = 1;

            lp7 = _fstrrchr(work_buf,'\\');  /* find last backslash */
            if(!lp7) lp7 = _fstrrchr(work_buf,':');

            if(!lp7)
            {
               *lp3 = 0;
            }
            else
            {
               if(*lp7==':')
               {
                  DosQualifyPath(work_buf,work_buf);

                  if(!*work_buf || !(lp7 = _fstrrchr(work_buf,'\\')))
                  {
                     if(*work_buf)
                     {
                        PrintString(pSYNTAXERROR);
                        PrintString(lp1);
                        PrintString(pQCRLFLF);
                     }
                     else
                     {
                        PrintString(pINVALIDPATH);
                        PrintString(lp1);
                        PrintString(pQCRLFLF);
                     }

                     GlobalFreePtr(lpCommand);
                     GlobalFreePtr(lpFileSpec);
                     return(TRUE);
                  }
               }

               *lp7 = 0;                           /* temporarily */
               lstrcpy(lp3, work_buf);      /* jam the path in there! */
               lstrcat(lp3, pBACKSLASH);
               *(lp7++) = '\\';         /* restored to original value */

               lstrcpy(work_buf, lp7);
            }

            /* at this point 'work_buf' contains just a file name */
            /* now we get to turn it into a 'DIRENTRY' structure */

            plpDI[wNDirInfo] = (LPDIR_INFO)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                                     sizeof(DIR_INFO));

            if(!plpDI[wNDirInfo]) w = DIRINFO_ERROR;
            else
            {
               _hmemset(plpDI[wNDirInfo]->fullname, 0,
                        sizeof((*plpDI)->fullname));

               _hmemset(plpDI[wNDirInfo]->name, ' ', sizeof((*plpDI)->name));
               _hmemset(plpDI[wNDirInfo]->ext, ' ', sizeof((*plpDI)->ext));

               lp7 = _fstrchr(work_buf, '.');
               if(lp7) *(lp7) = 0;

               _hmemcpy(plpDI[wNDirInfo]->name, work_buf,
                        min(sizeof((*plpDI)->name), lstrlen(work_buf)));

               if(lp7)
               {
                  _hmemcpy(plpDI[wNDirInfo]->ext, lp7 + 1,
                        min(sizeof((*plpDI)->ext), lstrlen(lp7 + 1)));

                  *lp7 = '.';  /* restore the '.' */
               }

               _hmemcpy(plpDI[wNDirInfo]->fullname, work_buf,
                        min(sizeof((*plpDI)->fullname), lstrlen(work_buf)));
            }
         }

         if(w && w!=DIRINFO_ERROR)
         {
            plpPaths[wNDirInfo] = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                                        lstrlen(lp3) + 2);
            if(!plpPaths[wNDirInfo])
            {
               GlobalFreePtr(plpDI[wNDirInfo]);
            }
            else
            {
               lstrcpy(plpPaths[wNDirInfo], lp3);  /* store path info */
            }
         }

         if(w && (w==DIRINFO_ERROR || !plpPaths[wNDirInfo]))
         {
            if(w==DIRINFO_ERROR)
            {
               PrintErrorMessage(581);
               PrintString(work_buf);
               PrintString(pQCRLFLF);
            }
            else
            {
               PrintString(pNOTENOUGHMEMORY);
            }

            for(j=0; j<wNDirInfo; j++)    /* free allocated dir info */
            {
               GlobalFreePtr(plpDI[j]);
               GlobalFreePtr(plpPaths[j]);
            }

            GlobalFreePtr(lpCommand);
            GlobalFreePtr(lpFileSpec);
            return(TRUE);
         }
         else if(w)
         {
            pNDI[wNDirInfo++] = w;  /* number of items in this list */
         }
         else          /* no files .. i.e. w==0 */
         {
            GlobalFreePtr(plpDI[wNDirInfo]);
         }
      }

      if(!wNDirInfo)  /* no files found matching this criteria */
      {
         PrintString(pNOMATCHINGFILES);

         GlobalFreePtr(lpCommand);
         GlobalFreePtr(lpFileSpec);
         return(FALSE);
      }
   }

   GlobalFreePtr(lpFileSpec);  /* no longer needed! */

   /** GRAND FINAL - using the list of files within the array (above) **/
   /**                scan through each list, one file at a time, and  **/
   /**                substitute the name for the 'variable'; then,    **/
   /**                call 'EvalString()' to evaluate any %VAR% var's  **/
   /**                and (finally) interpret the resulting command.   **/


   BlockCopyResults = TRUE;

   for(ErrorFlag=FALSE, w=0; !ErrorFlag && w<wNDirInfo; w++)
   {
      for(lpDI=plpDI[w], i=0; i<pNDI[w]; i++, lpDI++)
      {
         /** STEP 1:  find every occurrence of 'pVariable' and **/
         /**          substitute the full pathname for it.     **/

         lp1 = lpCommand;
         lp2 = lpNewCmd;

         while(*lp1)
         {
            lp3 = _fstrchr(lp1, *pVar);
            if(!lp3) break;     /* find next possible occurrence of 'pVar' */

            j = lp3 - lp1;    /* length of string up to 1st char of 'pVar' */

            if(_fstrnicmp(lp3 + 1, pVar + 1, lstrlen(pVar + 1))) /* match? */
            {
               j++;       /* add 1 to capture 1st char of 'pVar' also */

               _hmemcpy(lp2, lp1, j);
               *(lp2 += j) = 0;

               lp1 += j;
            }
            else          /* found 'pVar' within this string!! */
            {
               _hmemcpy(lp2, lp1, j);
               lp2 += j;

               lp1 += j + lstrlen(pVar); /* point 'just past' the variable */

               lstrcpy(lp2, plpPaths[w]);  /* add the current path */
               lp2 += lstrlen(lp2);

               _fstrncpy(lp2, lpDI->fullname, sizeof(lpDI->fullname));
                                       /* add the name of the current file */

               lp2[sizeof(lpDI->fullname)] = 0;     /* in case it's needed */

               lp2 += lstrlen(lp2);            /* lp2 now at end of string */

            }
         }

         if(*lp1) lstrcpy(lp2, lp1);     /* if any left, add to 'lpNewCmd' */

         lp1 = EvalString(lpNewCmd);

         if(!lp1)
         {
            PrintErrorMessage(582);
            ErrorFlag = TRUE;
            break;
         }

            /** now for some more fun - evaluate the puppy! **/


         IsCall = TRUE;    /* forces 'call' command to be in effect */
         IsFor  = TRUE;    /* forces new batch file to complete first */

         ErrorFlag = ProcessCommand(lp1);

         IsCall = FALSE;   /* 'call' command no longer in effect */
         IsFor  = FALSE;   /* reset the 'for' flag also */

         if(ErrorFlag)
         {
            PrintErrorMessage(583);
            GetUserInput(work_buf);

            if(ctrl_break || (*work_buf!='Y' && *work_buf!='y'))
            {
               if(!BatchMode) ctrl_break = FALSE;
               break;
            }
            else
            {
               ErrorFlag = FALSE;  /* reset the error flag */
            }
         }


         GlobalFreePtr(lp1);  /* free ptr allocated by 'EvalString()' */

         /** and, for the final performance, do a LoopDispatch(), **/
         /** then check the 'ctrl_break' flag, bailing out if set **/

         UpdateWindow(hMainWnd);    /* ensure the screen is updated */
         LoopDispatch();           /* allow the user to intervene... */

         if(ctrl_break)
         {
            PrintErrorMessage(584);
            if(!BatchMode) ctrl_break = FALSE;
            ErrorFlag = TRUE;
            break;
         }
      }
   }


   BlockCopyResults = FALSE;
   if(TotalCopiesSubmitted) ReportCopyResults();


   for(j=0; j<wNDirInfo; j++)    /* free allocated dir info */
   {
      GlobalFreePtr(plpDI[j]);
      GlobalFreePtr(plpPaths[j]);
   }

   GlobalFreePtr(lpCommand);

   return(ErrorFlag);
}



BOOL FAR PASCAL CMDNext    (LPSTR lpCmd, LPSTR lpArgs)
{
LPSTR lp1, lp2;
UINT w1, w2;
LPBLOCK_INFO lpBI;
char szVarName[64];
double dCtr, dMax, dStep;



   if(!BatchMode || !lpBatchInfo)
   {
      PrintErrorMessage(578); // NOT VALID IN COMMAND MODE
      return(TRUE);
   }
   else if(!lpBatchInfo->lpBlockInfo)
   {
      PrintErrorMessage(908); // "?NEXT/UNTIL/WEND not valid outside of loop\r\n\n"
      return(TRUE);
   }

   if(lpArgs) lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   // if an argument exists, it will be a variable name.  That's all
   // that is allowed here.

   for(lp1=lpArgs; *lp1 && *lp1<=' '; lp1++)
      ; // first non-white-space char

   for(lp2=lp1; *lp2 > ' '; lp2++)
      ; // next white-space character

   if(*lp1) _hmemcpy(szVarName, lp1, lp2 - lp1);  // lp2 *will* be > lp1
   szVarName[lp2 - lp1] = 0;                 // this will *ALWAYS* be valid!

   for(lp1=lp2; *lp1 && *lp1<=' '; lp1++)
      ; // next non-white-space char

   if(*lp1) // syntax error kiddies!
   {
      PrintString(pSYNTAXERROR);
      PrintString(lpArgs);
      PrintString(pQCRLFLF);

      GlobalFreePtr(lpArgs);
      return(TRUE);
   }

   GlobalFreePtr(lpArgs);  // no longer needed now!
   lpArgs = NULL;  // just in case



   // NOTE:  the following values are stored in 'szInfo':
   // bytes 0-n:        NULL terminated string (variable name)
   // byte n+1 to n+8:   DOUBLE value (start)
   // byte n+9 to n+16:  DOUBLE value (limit)
   // byte n+17 to n+24: DOUBLE value (step value)
   // NOTE:  a negative STEP value indicates limit is 'low' limit,
   //        and a positive STEP value indicates it is 'high' limit.


   lpBI = lpBatchInfo->lpBlockInfo;

   for(w1 = (UINT)lpBI->dwEntryCount; w1 > 0;)
   {
      w1--;  // pre-decrement in this case

      // We are looking for a matching FOR command with the same variable
      // as the one indicated here (or any if NO variable name).

      if((lpBI->lpCodeBlock[w1].dwType & CODEBLOCK_TYPEMASK)==CODEBLOCK_FOR)
      {
         // do variable names match?  hmm????

         if(!*szVarName ||
            !_fstricmp(szVarName,lpBI->lpCodeBlock[w1].szInfo))
         {
            // YES!  This *IS* a matching entry.

            lp1 = lpBI->lpCodeBlock[w1].szInfo;

            lp1 += lstrlen(lp1) + 1;

            dCtr = ((double FAR *)lp1)[0];
            dMax = ((double FAR *)lp1)[1];
            dStep = ((double FAR *)lp1)[2];


            dCtr = dCtr + dStep;

            ((double FAR *)lp1)[0] = dCtr;  // re-store it here

            lp2 = StringFromFloat(dCtr);
            if(!lp2)
            {
               PrintString(pNOTENOUGHMEMORY);
               return(TRUE);
            }

            SetEnvString(szVarName, lp2);
            GlobalFreePtr(lp2);

            // remove ALL nested loops "below" this one

            if(lpBI->dwEntryCount)
            {
               for(w2 = (UINT)lpBI->dwEntryCount - 1; w2 > w1; w2--)
               {
                  RemoveCodeBlockEntry(lpBI->lpCodeBlock + w2);
               }

               lpBI->dwEntryCount = w1 + 1;  // reset new entry count
            }

            if((dStep >= 0 && dCtr > dMax) || (dStep < 0 && dCtr < dMax))
            {
               // the loop has ended. remove this entry, then return 'FALSE'.

               RemoveCodeBlockEntry(lpBI->lpCodeBlock + w1);

               lpBI->dwEntryCount = w1;  // reset new entry count

               return(FALSE);            // that's all, folks!
            }
            else
            {
               // go back to the beginning of the loop again...

               return(SetBatchFilePosition(lpBatchInfo,
                                           lpBI->lpCodeBlock[w1].dwStart));
            }
         }
      }
   }


   PrintErrorMessage(909); // "?Missing FOR command in NEXT\r\n\n"
   return(TRUE);
}





BOOL FAR PASCAL CMDRepeat  (LPSTR lpCmd, LPSTR lpArgs)
{
LPBLOCK_INFO lpBI;
WORD w1;
LPSTR lp1;


   if(!BatchMode || !lpBatchInfo)
   {
      PrintErrorMessage(578); // NOT VALID IN COMMAND MODE
      return(TRUE);
   }
   else if(VerifyBlockInfo(&(lpBatchInfo->lpBlockInfo)))
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   if(lpArgs) lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   for(lp1=lpArgs; *lp1 && *lp1<=' '; lp1++)
      ; // next non-white-space char

   if(*lp1) // syntax error kiddies!
   {
      PrintString(pSYNTAXERROR);
      PrintString(lpArgs);
      PrintString(pQCRLFLF);

      GlobalFreePtr(lpArgs);
      return(TRUE);
   }

   GlobalFreePtr(lpArgs);  // no longer needed now!
   lpArgs = NULL;  // just in case


   lpBI = lpBatchInfo->lpBlockInfo;

   w1 = (WORD)lpBI->dwEntryCount++;

   // record next line's batch file position in 'dwStart'

   lpBI->lpCodeBlock[w1].dwStart = lpBatchInfo->buf_start
                                 + lpBatchInfo->buf_pos;

   lpBI->lpCodeBlock[w1].dwBlockEnd = (DWORD)-1L;  // a flag

   lpBI->lpCodeBlock[w1].szInfo[0] = 0;  // not used for 'REPEAT'

   lpBI->lpCodeBlock[w1].dwType = CODEBLOCK_REPEAT;  // it's a REPEAT/UNTIL

   return(FALSE);

}

BOOL FAR PASCAL CMDUntil   (LPSTR lpCmd, LPSTR lpArgs)
{
LPSTR lp1;
UINT w1, w2;
BOOL bCondition;
LPBLOCK_INFO lpBI;



   if(!BatchMode || !lpBatchInfo)
   {
      PrintErrorMessage(578); // NOT VALID IN COMMAND MODE
      return(TRUE);
   }
   else if(!lpBatchInfo->lpBlockInfo)
   {
      PrintErrorMessage(908); // "?NEXT/UNTIL/WEND not valid outside of loop\r\n\n"
      return(TRUE);
   }

   if(lpArgs) lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   for(lp1=lpArgs; *lp1 && *lp1<=' '; lp1++)
      ; // next non-white-space char

   if(!*lp1) // syntax error kiddies!  no 'UNTIL' expression found!
   {
      PrintErrorMessage(893); // "?Missing expression in 'UNTIL'\r\n\n"

      GlobalFreePtr(lpArgs);
      return(TRUE);
   }



   // NOTE:  'lp1' contains the 'UNTIL' expression


   lpBI = lpBatchInfo->lpBlockInfo;

   for(w1 = (UINT)lpBI->dwEntryCount; w1 > 0;)
   {
      w1--;  // pre-decrement in this case

      // We are looking for the last REPEAT command that we find.

      if((lpBI->lpCodeBlock[w1].dwType & CODEBLOCK_TYPEMASK)==CODEBLOCK_REPEAT)
      {
         lp1 = Calculate(lp1);   // evaluate the expression following 'UNTIL'
         GlobalFreePtr(lpArgs);  // no longer needed now!
         lpArgs = NULL;  // just in case...

         if(!lp1)
         {
            PrintString(pNOTENOUGHMEMORY);
            return(TRUE);
         }

         bCondition = (BOOL)CalcEvalNumber(lp1);
         GlobalFreePtr(lp1);

         // remove ALL nested loops "below" this one

         if(lpBI->dwEntryCount)
         {
            for(w2 = (UINT)lpBI->dwEntryCount - 1; w2 > w1; w2--)
            {
               RemoveCodeBlockEntry(lpBI->lpCodeBlock + w2);
            }

            lpBI->dwEntryCount = w1 + 1;  // reset new entry count
         }

         if(bCondition)
         {
            // the loop has ended. Remove this entry, then return 'FALSE'.

            RemoveCodeBlockEntry(lpBI->lpCodeBlock + w1);

            lpBI->dwEntryCount = w1;  // reset new entry count

            return(FALSE);            // that's all, folks!
         }
         else
         {
            // go back to the beginning of the loop again...

            return(SetBatchFilePosition(lpBatchInfo,
                                        lpBI->lpCodeBlock[w1].dwStart));
         }
      }
   }


   if(lpArgs) GlobalFreePtr(lpArgs);  // no longer needed now!

   PrintErrorMessage(910); // "?Missing REPEAT command in UNTIL\r\n\n"
   return(TRUE);
}





BOOL FAR PASCAL CMDWhile   (LPSTR lpCmd, LPSTR lpArgs)
{
LPBLOCK_INFO lpBI;
BOOL bCondition;
WORD w1;
LPSTR lp1, lp2;


   if(!BatchMode || !lpBatchInfo)
   {
      PrintErrorMessage(578); // NOT VALID IN COMMAND MODE
      return(TRUE);
   }
   else if(VerifyBlockInfo(&(lpBatchInfo->lpBlockInfo)))
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   if(lpArgs) lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   for(lp1=lpArgs; *lp1 && *lp1<=' '; lp1++)
      ; // next non-white-space char

   if(!*lp1) // syntax error kiddies!
   {
      PrintErrorMessage(892); // "?Missing expression in 'WHILE'\r\n\n"

      GlobalFreePtr(lpArgs);
      return(TRUE);
   }

   lpBI = lpBatchInfo->lpBlockInfo;

   w1 = (WORD)lpBI->dwEntryCount++;

   // record next line's batch file position in 'dwStart'

   lpBI->lpCodeBlock[w1].dwStart = lpBatchInfo->buf_start
                                 + lpBatchInfo->buf_pos;

   lpBI->lpCodeBlock[w1].dwBlockEnd = (DWORD)-1L;  // a flag

   lstrcpy(lpBI->lpCodeBlock[w1].szInfo, lp1);  // the 'WHILE' condition

   lpBI->lpCodeBlock[w1].dwType = CODEBLOCK_WHILE;  // it's a WHILE/WEND




   // evaluate the 'WHILE' condition before I continue...

   lp2 = Calculate(lp1);
   GlobalFreePtr(lpArgs);  // no longer needed now!

   bCondition = (BOOL)CalcEvalNumber(lp2);
   GlobalFreePtr(lp2);

   if(!bCondition)         // condition is FALSE - find end of loop!
   {

      if(FindNextMatchingSection(lpBI->lpCodeBlock + w1))
      {
         // error - can't find the 'WEND' or 'END WHILE' that matches us

         PrintErrorMessage(894);  // "?Missing END WHILE/WEND in WHILE\r\n\n"

         TerminateCurrentBatchFile();
         return(TRUE);
      }

      // at this point, the batch file points to the 'WEND' or 'END WHILE'
   }

   return(FALSE);

}

BOOL FAR PASCAL CMDWend    (LPSTR lpCmd, LPSTR lpArgs)
{
LPSTR lp1;
UINT w1, w2;
BOOL bCondition;
LPBLOCK_INFO lpBI;



   if(!BatchMode || !lpBatchInfo)
   {
      PrintErrorMessage(578); // NOT VALID IN COMMAND MODE
      return(TRUE);
   }
   else if(!lpBatchInfo->lpBlockInfo)
   {
      PrintErrorMessage(908); // "?NEXT/UNTIL/WEND not valid outside of loop\r\n\n"
      return(TRUE);
   }

   if(lpArgs) lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   for(lp1=lpArgs; *lp1 && *lp1<=' '; lp1++)
      ; // next non-white-space char

   if(*lp1) // syntax error kiddies!
   {
      PrintString(pSYNTAXERROR);
      PrintString(lpArgs);
      PrintString(pQCRLFLF);

      GlobalFreePtr(lpArgs);
      return(TRUE);
   }

   GlobalFreePtr(lpArgs);  // no longer needed now!
   lpArgs = NULL;  // just in case



   // NOTE:  'szInfo' contains the 'WHILE' expression


   lpBI = lpBatchInfo->lpBlockInfo;

   for(w1 = (UINT)lpBI->dwEntryCount; w1 > 0;)
   {
      w1--;  // pre-decrement in this case

      // We are looking for the last WHILE command that we find.

      if((lpBI->lpCodeBlock[w1].dwType & CODEBLOCK_TYPEMASK)==CODEBLOCK_WHILE)
      {
         lp1 = Calculate(lpBI->lpCodeBlock[w1].szInfo);
         if(!lp1)
         {
            PrintString(pNOTENOUGHMEMORY);
            return(TRUE);
         }

         bCondition = (BOOL)CalcEvalNumber(lp1);
         GlobalFreePtr(lp1);

         // remove ALL nested loops "below" this one

         if(lpBI->dwEntryCount)
         {
            for(w2 = (UINT)lpBI->dwEntryCount - 1; w2 > w1; w2--)
            {
               RemoveCodeBlockEntry(lpBI->lpCodeBlock + w2);
            }

            lpBI->dwEntryCount = w1 + 1;  // reset new entry count
         }

         if(!bCondition)
         {
            // the loop has ended. Remove this entry, then return 'FALSE'.

            RemoveCodeBlockEntry(lpBI->lpCodeBlock + w1);

            lpBI->dwEntryCount = w1;  // reset new entry count

            return(FALSE);            // that's all, folks!
         }
         else
         {
            // go back to the beginning of the loop again...

            return(SetBatchFilePosition(lpBatchInfo,
                                        lpBI->lpCodeBlock[w1].dwStart));
         }
      }
   }


   PrintErrorMessage(911); // "?Missing WHILE command in WEND\r\n\n"
   return(TRUE);
}





/***************************************************************************/
/*             SQL PROCESSING!  USING 'WDBUTIL'!  YES YES YES!             */
/***************************************************************************/


#pragma code_seg("CMDSql_TEXT","CODE")

BOOL FAR PASCAL CMDSql     (LPSTR lpCmd, LPSTR lpArgs)
{
static const char CODE_BASED szWDBUTIL[]="WDBUTIL.DLL";
LPSTR lpCommand, lpLine, lp1, lp2;
WORD wSize, wLen, wFunction;
BOOL rval;


   if(lpArgs) lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   _fstrtrim(lpArgs);
   while(*lpArgs <= ' ' && *lpArgs) lstrcpy(lpArgs, lpArgs + 1);

   for(lp1=lpArgs; *lp1>' '; lp1++)
      ;  // finds next non-space

   wFunction = 0;  // DEFAULT - function 'SQL BEGIN'

   if(!*lpArgs)
   {
      PrintString("?Argument required for 'SQL'\r\n\n");

      GlobalFreePtr(lpArgs);
      return(TRUE);
   }
   else if(!_fstrnicmp(lpArgs, "BROWSE", 6) && (lp1 - lpArgs)==6)
   {
      wFunction = 1;

      while(*lp1 && *lp1<= ' ') lp1++;

      if(!*lp1)
      {
         PrintString("?SQL BROWSE - no file name specified\r\n\n");

         GlobalFreePtr(lpArgs);
         return(TRUE);
      }
   }
   else if(_fstricmp(lpArgs, "BEGIN"))
   {
      PrintString("?Illegal SQL option - \"");
      PrintString(lpArgs);
      PrintString(pQCRLFLF);

      GlobalFreePtr(lpArgs);
      return(TRUE);
   }




   if(!BatchMode && wFunction!=1)
   {
      PrintErrorMessage(596);

      GlobalFreePtr(lpArgs);
      return(TRUE);
   }


   if(!hWDBUTIL)
   {
      hWDBUTIL = GetModuleHandle(szWDBUTIL);

      if(!hWDBUTIL)
      {
       OFSTRUCT ofstr;

         if(MyOpenFile(szWDBUTIL, &ofstr, OF_READ | OF_EXIST | OF_SHARE_COMPAT)
            != HFILE_ERROR)
         {
            hWDBUTIL = (HMODULE)0xffff;
         }
      }

      if(hWDBUTIL)
      {
         hWDBUTIL = LoadLibrary(szWDBUTIL);

         if((WORD)hWDBUTIL < 32) hWDBUTIL = NULL;
      }
   }


   if(hWDBUTIL)
   {
      (FARPROC &)wdbQueryErrorMessage  = GetProcAddress(hWDBUTIL, (LPSTR)MAKELP(0,268));
      (FARPROC &)wdbSQLProgram         = GetProcAddress(hWDBUTIL, (LPSTR)MAKELP(0,269));

      (FARPROC &)wdbCreateBrowseWindow = GetProcAddress(hWDBUTIL, (LPSTR)MAKELP(0,234));
      (FARPROC &)wdbOpenDatabase       = GetProcAddress(hWDBUTIL, (LPSTR)MAKELP(0,203));
      (FARPROC &)wdbCloseDatabase      = GetProcAddress(hWDBUTIL, (LPSTR)MAKELP(0,204));

      if(!wdbQueryErrorMessage || !wdbSQLProgram ||
         !wdbCreateBrowseWindow || !wdbOpenDatabase || !wdbCloseDatabase)
      {
         FreeLibrary(hWDBUTIL);

         hWDBUTIL = NULL;
      }
   }

   if(!hWDBUTIL)
   {
      PrintString("?Unable to load 'WDBUTIL.DLL' - command rejected\r\n");

      TerminateCurrentBatchFile();

      GlobalFreePtr(lpArgs);
      return(TRUE);
   }


   if(wFunction == 1)       // 'SQL BROWSE' function - 'lp1' is file name!
   {
    HWDB hDb;
    HWND hBox;

      hDb = wdbOpenDatabase(lp1, OF_READWRITE | OF_SHARE_DENY_WRITE);

      GlobalFreePtr(lpArgs);  // not needed anymore

      if(!hDb)
      {
         wdbQueryErrorMessage(work_buf, sizeof(work_buf));

         if(*work_buf)  PrintString(work_buf);
         else           PrintString("?Unknown (FILE OPEN) error on database\r\n\n");

         return(TRUE);
      }

      hBox = wdbCreateBrowseWindow(hDb, hMainWnd, 1);  // 'AUTOCLOSE' mode!

      if(!hBox)
      {
         wdbCloseDatabase(hDb);

         wdbQueryErrorMessage(work_buf, sizeof(work_buf));

         if(*work_buf)  PrintString(work_buf);
         else           PrintString("?Unknown (BROWSE WINDOW) error on database\r\n\n");

         return(TRUE);
      }

      while(!LoopDispatch() && IsWindow(hBox))
         ;  // wait until window is destroyed before returning.
            // database will be closed automatically.

      return(FALSE);
   }


   GlobalFreePtr(lpArgs);  // no longer needed!!


   lpCommand = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE, 8192);

   if(!lpCommand)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   *lpCommand = 0;
   wSize = (WORD)GlobalSizePtr(lpCommand);

   // NOW I get to read through the batch file until I find an 'SQL END'


   do
   {
      lpLine = GetAnotherBatchLine(lpBatchInfo);
      if(!lpLine)
      {
         PrintString("?Error reading batch file (missing 'SQL END'?)\r\n\n");

         TerminateCurrentBatchFile();
         return(TRUE);
      }


      if(*lpLine!='@' && BatchEcho &&
         hMainWnd && IsWindow(hMainWnd))  /* echoing commands */
      {
         DisplayPrompt();
         PrintString(lpLine);
         PrintString(pCRLF);
      }

      lp1 = lpLine;

      if(*lp1=='@')
      {
         lp1++;
      }
      else if(*lp1==':')
      {
         PrintString("?Mis-placed label inside 'SQL' block - ignored\r\n\n");
         GlobalFreePtr(lpLine);
         continue;
      }

      while(*lp1 && *lp1<=' ') lp1++;

      if(*lp1)
      {
         // PARSE LINE - look for 'SQL END'

         for(lp2=lp1+1; *lp2>' '; lp2++)
            ;  // the first word!

         if((lp2 - lp1)==3 && !_fstrnicmp(lp1, "SQL", 3))
         {
            lp1 = lp2;
            while(*lp1 && *lp1<=' ') lp1++;

            if(*lp1)
            {
               for(lp2=lp1+1; *lp2>' '; lp2++)
                  ;  // the second word!

               if((lp2 - lp1)==3 && !_fstrnicmp(lp1, "END", 3))
               {
                  // who cares what else is on the line!

                  GlobalFreePtr(lpLine);

                  break;
               }
            }
         }
      }

      wLen = lstrlen(lpCommand) + lstrlen(lpLine) + 4;

      if(wLen >= wSize)
      {
         lp1 = (LPSTR)GlobalReAllocPtr(lpCommand, wLen + 1024, GMEM_MOVEABLE);

         if(!lp1)
         {
            PrintString(pNOTENOUGHMEMORY);

            GlobalFreePtr(lpCommand);
            GlobalFreePtr(lpLine);

            return(TRUE);
         }

         lpCommand = lp1;

         wSize = (WORD)GlobalSizePtr(lpCommand);
      }

      lp1 = lpLine;
      if(*lp1=='@') lp1++;  // skip '@' on beginning of line (if present)

      lstrcat(lpCommand, lp1);
      lstrcat(lpCommand, pCRLF);

      GlobalFreePtr(lpLine);     // must remember to free it!

   } while(TRUE);

   lp1 = lpCommand;
   while(*lp1 && *lp1<=' ') lp1++;

   rval = FALSE;

   if(!*lp1)
   {
      PrintString("?Warning - SQL Program is BLANK\r\n\n");
   }
   else
   {
      if(wdbSQLProgram(lpCommand)) // guess what?  ERROR!
      {
         if(wdbQueryErrorMessage(lpCommand, 256))
         {
            PrintString("SQL Error: ");
            PrintString(lpCommand);
            PrintString(pCRLFLF);

            rval = TRUE;
         }
         else
         {
            PrintString("?Unspecified SQL Error\r\n\n");
         }
      }
   }

   GlobalFreePtr(lpCommand);

   return(rval);
}





/***************************************************************************/
/*                                                                         */
/*                     ODBC commands (much like DDE)                       */
/*                                                                         */
/***************************************************************************/


#pragma code_seg("CMDOdbc_TEXT","CODE")

struct tagODBC_ARRAY
{
   HENV hEnv;
   HDBC hDbc;
   HSTMT hStmt;
   DWORD dwFlags;               // various flags (see below)

   DWORD dwCurPos, dwRowCount;

   char szTableName[260];       // valid ONLY when ODBC OPEN is used...
                                // (used for positioned update SQL code)

   char szLastError[260];       // last error message (when applicable)

   LPSTR lpParmInfo;            // contains info & buffer for SQL parameters

   LPSTR lpBindInfo;            // contains info & buffer for bind columns
                                // and *MUST* immediately precede the next element...
};

struct ODBC_ARRAY : public tagODBC_ARRAY
{
   char szConnect[1024 - sizeof(tagODBC_ARRAY)];
//   sizeof(((ODBC_ARRAY NEAR *)0)->lpBindInfo) -
//                  (WORD)&(((ODBC_ARRAY NEAR *)0)->lpBindInfo)];

};

typedef ODBC_ARRAY FAR *LPODBC_ARRAY;


// NOTE:  I'm allocating a 0x10000 byte array for this

static LPODBC_ARRAY lpODBCX=NULL;  // list of 'live' ODBC connections
static WORD wNODBCX = 0;           // # of 'live' ODBC connections

static char pSqlState[16]="000000";

static const char CODE_BASED szODBC_RESULT[]="ODBC_RESULT";
static const char CODE_BASED szSTATEMENT[]="STATEMENT";
static const char CODE_BASED szERROR[]="ERROR";
static const char CODE_BASED szODBC[]="ODBC";
static const char CODE_BASED szSQL[]="SQL";
static const char CODE_BASED szEND[]="END";
static const char CODE_BASED szEOF[]="EOF";
static const char CODE_BASED szALL[]="ALL";
static const char CODE_BASED szBEGIN[]="BEGIN";
static const char CODE_BASED szCANCEL[]="CANCEL";
static const char CODE_BASED szROLLBACK[]="ROLLBACK";
static const char CODE_BASED szOK[]="OK";


#define KEYWORD_MATCH(x,y,z) (z==(sizeof(y)-1) && !_fstrnicmp(x,y,z))

// bit values for 'dwFlags' member

#define ODBC_CURSOR_MASK      0x3fL     /* mask of cursor capabilities */
#define ODBC_CURSOR_NEXT      0x01L
#define ODBC_CURSOR_FIRST     0x02L
#define ODBC_CURSOR_LAST      0x04L
#define ODBC_CURSOR_PREVIOUS  0x08L
#define ODBC_CURSOR_ABSOLUTE  0x10L
#define ODBC_CURSOR_RELATIVE  0x20L

#define ODBC_RELEASE2         0x80000000L
#define ODBC_RESULT_SET       0x40000000L  /* set when a result set is available */



WORD NEAR PASCAL CMDOdbcParseID(LPCSTR FAR *lpID)
{
LPCSTR lpc1, lpc2;
WORD wID;
HENV hEnv;
HDBC hDbc;
HSTMT hStmt;


   lpc1 = *lpID;
   while(*lpc1 && *lpc1 < ' ') lpc1++;

   lpc2 = lpc1;

   // CONNECT ID is 3 hexadecimal LONG values, total 24 characters

   while(*lpc2 && *lpc2 > ' ') lpc2++;

   if((lpc2 - lpc1)!= 24)   return(0xffff);  // ERROR!

   hEnv = NULL;
   hDbc = NULL;
   hStmt = NULL;

   for(wID=0; wID<8; wID++, lpc1++)
   {
      if(lpc1[0] >='0' && lpc1[0] <='9')
      {
         hEnv = (HENV)(((DWORD)hEnv) * 16 + lpc1[0] - '0');
      }
      else if(toupper(lpc1[0]) >='A' && toupper(lpc1[0]) <='F')
      {
         hEnv = (HENV)(((DWORD)hEnv) * 16 + toupper(lpc1[0]) - 'A' + 10);
      }
      else
      {
         return(0xffff);  // error
      }

      if(lpc1[8] >='0' && lpc1[8] <='9')
      {
         hDbc = (HDBC)(((DWORD)hDbc) * 16 + lpc1[8] - '0');
      }
      else if(toupper(lpc1[8]) >='A' && toupper(lpc1[8]) <='F')
      {
         hDbc = (HDBC)(((DWORD)hDbc) * 16 + toupper(lpc1[8]) - 'A' + 10);
      }
      else
      {
         return(0xffff);  // error
      }

      if(lpc1[16] >='0' && lpc1[16] <='9')
      {
         hStmt = (HSTMT)(((DWORD)hStmt) * 16 + lpc1[16] - '0');
      }
      else if(toupper(lpc1[16]) >='A' && toupper(lpc1[16]) <='F')
      {
         hStmt = (HSTMT)(((DWORD)hStmt) * 16 + toupper(lpc1[16]) - 'A' + 10);
      }
      else
      {
         return(0xffff);  // error
      }
   }

   // successfully parsed.  Assign 'lpc2' to '*lpID'

   *lpID = lpc2;


   // look for a matching entry within lpODBCX

   for(wID=0; lpODBCX && wID < wNODBCX; wID++)
   {
      if(lpODBCX[wID].hEnv  == hEnv &&
         lpODBCX[wID].hDbc  == hDbc &&
         lpODBCX[wID].hStmt == hStmt)
      {
         return(wID);  //  done!  Found!
      }
   }

   return(0xffff);               // error (not found)
}



BOOL NEAR PASCAL CMDOdbcConnect(LPCSTR lpCmd, LPSTR lpBuf, WORD cbBuf)
{
RETCODE retcode;
HENV hEnv;
HDBC hDbc;
HSTMT hStmt;
WORD w1, wLen;
long l1;



   if(!lpODBCX)
   {
      lpODBCX = (LPODBC_ARRAY)GlobalAllocPtr(GHND,0x10000L);
      wNODBCX = 0;

      if(!lpODBCX)
      {
         PrintString(pNOTENOUGHMEMORY);

         return(TRUE);
      }
   }

   // IMPORTANT:  assume that 'SQLConnect' won't be used, and that
   //             'SQLDriverConnect' is available

   hEnv = NULL;
   hDbc = NULL;
   hStmt = NULL;

   if(lpSQLAllocEnv(&hEnv) != SQL_SUCCESS)
   {
      PrintErrorMessage(883); // "ODBC ERROR - unable to allocate environment\r\n\n"

      return(TRUE);
   }

   retcode = lpSQLAllocConnect(hEnv, &hDbc);
   if(retcode != SQL_SUCCESS)
   {
      if(retcode == SQL_SUCCESS_WITH_INFO)
      {
         // get the error message, and print it but don't return error

         lpSQLError(hEnv, SQL_NULL_HDBC, SQL_NULL_HSTMT, 
                    (UCHAR FAR *)pSqlState, (SDWORD FAR *)&l1,
                    (UCHAR FAR *)lpBuf, cbBuf, (SWORD FAR *)&wLen);

         PrintErrorMessage(882);  // "ODBC 'SUCCESS WITH INFO' ERROR Class=\042"
         PrintString(pSqlState);
         PrintString(pQCRLF);
         PrintString(lpBuf);
         PrintString(pCRLF);
      }
      else
      {
         lpSQLError(hEnv, SQL_NULL_HDBC, SQL_NULL_HSTMT,
                    (UCHAR FAR *)pSqlState, (SDWORD FAR *)&l1,
                    (UCHAR FAR *)lpBuf, cbBuf, (SWORD FAR *)&wLen);

         PrintErrorMessage(881);  // "ODBC ERROR Class=\042"
         PrintString(pSqlState);
         PrintString(pQCRLF);
         PrintString(lpBuf);
         PrintString(pCRLFLF);

         lpSQLFreeEnv(hEnv);

         return(TRUE);
      }
   }

   // at this point 'lpCmd' points to the remainder of the command line

   retcode = lpSQLDriverConnect(hDbc, hMainWnd, (UCHAR FAR *)lpCmd, lstrlen(lpCmd),
                                (UCHAR FAR *)lpBuf, cbBuf, (SWORD FAR *)&w1,
                                SQL_DRIVER_COMPLETE);

   if(retcode != SQL_SUCCESS)
   {
      if(retcode == SQL_SUCCESS_WITH_INFO)
      {
         // get the error message, and print it but don't return error

         lpSQLError(hEnv, hDbc, SQL_NULL_HSTMT,
                    (UCHAR FAR *)pSqlState, (SDWORD FAR *)&l1,
                    (UCHAR FAR *)lpBuf, cbBuf, (SWORD FAR *)&wLen);

         PrintErrorMessage(882);  // "ODBC 'SUCCESS WITH INFO' ERROR Class=\042"
         PrintString(pSqlState);
         PrintString(pQCRLF);
         PrintString(lpBuf);
         PrintString(pCRLF);

      }
      else
      {
         lpSQLError(hEnv, hDbc, SQL_NULL_HSTMT,
                    (UCHAR FAR *)pSqlState, (SDWORD FAR *)&l1,
                    (UCHAR FAR *)lpBuf, cbBuf, (SWORD FAR *)&wLen);

         PrintErrorMessage(881);  // "ODBC ERROR Class=\042"
         PrintString(pSqlState);
         PrintString(pQCRLF);
         PrintString(lpBuf);
         PrintString(pCRLFLF);

         lpSQLFreeConnect(hDbc);
         lpSQLFreeEnv(hEnv);

         return(TRUE);
      }
   }

   lpBuf[w1] = 0;


   // NEXT, allocate a statement that I can use for this connection

   retcode = lpSQLAllocStmt(hDbc, &hStmt);

   if(retcode != SQL_SUCCESS)
   {
      if(retcode == SQL_SUCCESS_WITH_INFO)
      {
         // get the error message, and print it but don't return error

         lpSQLError(hEnv, hDbc, SQL_NULL_HSTMT,
                    (UCHAR FAR *)pSqlState, (SDWORD FAR *)&l1,
                    (UCHAR FAR *)lpBuf, cbBuf, (SWORD FAR *)&wLen);

         PrintErrorMessage(882);  // "ODBC 'SUCCESS WITH INFO' ERROR Class=\042"
         PrintString(pSqlState);
         PrintString(pQCRLF);
         PrintString(lpBuf);
         PrintString(pCRLF);
      }
      else
      {
         lpSQLError(hEnv, hDbc, SQL_NULL_HSTMT,
                    (UCHAR FAR *)pSqlState, (SDWORD FAR *)&l1,
                    (UCHAR FAR *)lpBuf, cbBuf, (SWORD FAR *)&wLen);

         PrintErrorMessage(881);  // "ODBC ERROR Class=\042"
         PrintString(pSqlState);
         PrintString(pQCRLF);
         PrintString(lpBuf);
         PrintString(pCRLFLF);

         lpSQLFreeConnect(hDbc);
         lpSQLFreeEnv(hEnv);

         return(TRUE);
      }
   }


   lpODBCX[wNODBCX].hEnv = hEnv;
   lpODBCX[wNODBCX].hDbc = hDbc;
   lpODBCX[wNODBCX].hStmt = hStmt;

   lpODBCX[wNODBCX].dwFlags = 0;    // initial value for FLAGS
   lpODBCX[wNODBCX].szTableName[0] = 0;
   lpODBCX[wNODBCX].szLastError[0] = 0;

   lpODBCX[wNODBCX].dwCurPos = 0;   // nothing has been read
   lpODBCX[wNODBCX].dwRowCount = 0; // no records in result set (yet)
   lpODBCX[wNODBCX].lpParmInfo = NULL;  // no SQL parameters
   lpODBCX[wNODBCX].lpBindInfo = NULL;  // no bound columns



   _fstrncpy(lpODBCX[wNODBCX].szConnect, lpBuf,
             sizeof(lpODBCX->szConnect));

   wNODBCX++;       // connection has been established!


   // Now that I have a connection, let's get some information about
   // the driver itself (for later).


   // see which 'fetch directions' are supported

   retcode = lpSQLGetInfo(hDbc, SQL_FETCH_DIRECTION, &l1, sizeof(l1), (SWORD FAR *)&wLen);

   if(retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
   {
      if(l1&SQL_FD_FETCH_NEXT)     lpODBCX[wNODBCX].dwFlags |= ODBC_CURSOR_NEXT;
      if(l1&SQL_FD_FETCH_FIRST)    lpODBCX[wNODBCX].dwFlags |= ODBC_CURSOR_FIRST;
      if(l1&SQL_FD_FETCH_LAST)     lpODBCX[wNODBCX].dwFlags |= ODBC_CURSOR_LAST;
      if(l1&SQL_FD_FETCH_PRIOR)    lpODBCX[wNODBCX].dwFlags |= ODBC_CURSOR_PREVIOUS;
      if(l1&SQL_FD_FETCH_ABSOLUTE) lpODBCX[wNODBCX].dwFlags |= ODBC_CURSOR_ABSOLUTE;
      if(l1&SQL_FD_FETCH_RELATIVE) lpODBCX[wNODBCX].dwFlags |= ODBC_CURSOR_RELATIVE;
   }

   // see if ODBC 2.0 extensions are supported

   retcode = lpSQLGetInfo(hDbc, SQL_DRIVER_ODBC_VER, lpBuf, cbBuf, (SWORD FAR *)&wLen);

   if(retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
   {
      lpBuf[wLen] = 0;

      l1 = (long)(100.0 * CalcEvalNumber(lpBuf));

      if(l1 >= 200)  // release 2.0 or later
      {
         lpODBCX[wNODBCX].dwFlags |= ODBC_RELEASE2;
      }
   }


   // FINAL SECTION - 'ODBC_RESULT'

   // write to 'lpBuf' the correct 'Topic ID' value

   wsprintf(lpBuf, "%08lx%08lx%08lx",
            (DWORD)hEnv, (DWORD)hDbc, (DWORD)hStmt);

   // assign value to 'ODBC_RESULT'

   SetEnvString(szODBC_RESULT, lpBuf);

   return(FALSE);
}


WORD NEAR PASCAL CMDOdbcColumnNameIndex(LPODBC_ARRAY lpOA, LPCSTR lpName)
{
char tbuf[256];
RETCODE retcode;
WORD wLen, w2, w3, w4;
DWORD dw1;
LPSTR lp1;


   // search through the table for a matching column name.
   // return a ZERO if not found, or 0xffff on error...

   do
   {
      retcode = lpSQLNumResultCols(lpOA->hStmt, (SWORD FAR *)&w2);

      if(retcode != SQL_STILL_EXECUTING) break;

      if(LoopDispatch())
      {
         return(0xffff);
      }

   } while(TRUE);

   if(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
   {
      return(0xffff);
   }

   // is the value in 'lpName' a pure number value?

   lp1 = (LPSTR)lpName;

   while(*lp1 >= '0' && *lp1 <= '9') lp1++;

   if(!*lp1)          // a pure column number...
   {
      w3 = (WORD)CalcEvalNumber(tbuf);

      if(w3 < 1 || w3 > w2)
      {
         return(0);
      }
   }
   else
   {
      for(w3=1; w3<=w2; w3++)
      {
         do
         {
            wLen = sizeof(tbuf);

            retcode = lpSQLColAttributes(lpOA->hStmt, w3,
                                         SQL_COLUMN_NAME, tbuf,
                                         wLen, (SWORD FAR *)&wLen,
                                         (SDWORD FAR *)&dw1);

            if(retcode != SQL_STILL_EXECUTING) break;

            if(LoopDispatch())
            {
               return(0xffff);
            }

         } while(TRUE);


         if(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
         {
            return(0xffff);
         }

         if(!*tbuf) continue;  // blank field names don't count

         if(!_fstricmp(tbuf, lpName)) break;  // FOUND!



         // ODBC 2 extensions...

         if(!(lpOA->dwFlags & ODBC_RELEASE2)) continue;

         // Next, do something rather sneaky - grab the 'table name'
         // add a '.' and concatenate the field name.  THEN, compare
         // *THIS* to whatever is in 'tbuf'...

         // does the 'qualifier' belong at the beginning or end?

         retcode = lpSQLGetInfo(lpOA->hDbc, SQL_QUALIFIER_LOCATION,
                                &w4, sizeof(w4), (SWORD FAR *)&wLen);

         if(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
         {
            continue;  // if I can't determine it, don't bother
         }


         lp1 = tbuf + lstrlen(tbuf);

         if(w4 == SQL_QL_END)
         {
            wLen = sizeof(tbuf) - (lp1 - (LPSTR)tbuf);

            retcode = lpSQLGetInfo(lpOA->hDbc, SQL_QUALIFIER_NAME_SEPARATOR,
                                   lp1, wLen, (SWORD FAR *)&wLen);

            if(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
            {
               continue;
            }


            lp1 += lstrlen(lp1);

            do
            {
               wLen = sizeof(tbuf) - (lp1 - (LPSTR)tbuf);

               retcode = lpSQLColAttributes(lpOA->hStmt, w3,
                                            SQL_COLUMN_QUALIFIER_NAME,
                                            lp1, wLen, (SWORD FAR *)&wLen,
                                            (SDWORD FAR *)&dw1);

               if(retcode != SQL_STILL_EXECUTING) break;

               if(LoopDispatch())
               {
                  return(0xffff);
               }

            } while(TRUE);


            if(!*lp1 ||
               (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO))
            {
               continue;
            }

         }
         else // if(w4 == SQL_START)
         {
            do
            {
               wLen = sizeof(tbuf);

               retcode = lpSQLColAttributes(lpOA->hStmt, w3,
                                            SQL_COLUMN_QUALIFIER_NAME,
                                            tbuf, wLen, (SWORD FAR *)&wLen,
                                            (SDWORD FAR *)&dw1);

               if(retcode != SQL_STILL_EXECUTING) break;

               if(LoopDispatch())
               {
                  return(0xffff);
               }

            } while(TRUE);


            if(!*tbuf ||
               (retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO))
            {
               continue;
            }


            lp1 = tbuf + lstrlen(tbuf);

            wLen = sizeof(tbuf) - (lp1 - (LPSTR)tbuf);

            retcode = lpSQLGetInfo(lpOA->hDbc, SQL_QUALIFIER_NAME_SEPARATOR,
                                   lp1 + lstrlen(lp1), wLen, (SWORD FAR *)&wLen);


            if(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
            {
               continue;
            }


            lp1 += lstrlen(lp1);

            do
            {
               wLen = sizeof(tbuf) - (lp1 - (LPSTR)tbuf);

               retcode = lpSQLColAttributes(lpOA->hStmt, w3,
                                            SQL_COLUMN_NAME,
                                            lp1, wLen, (SWORD FAR *)&wLen,
                                            (SDWORD FAR *)&dw1);

               if(retcode != SQL_STILL_EXECUTING) break;

               if(LoopDispatch())
               {
                  return(0xffff);
               }

            } while(TRUE);


            if(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
            {
               continue;
            }
         }


         if(!_fstricmp(tbuf, lpName)) break;  // FOUND!
      }

   }


   if(!w3 || w3 > w2)  // ensure field index # is VALID
   {
      return(0);
   }
   else
   {
      return(w3);
   }
}



BOOL FAR PASCAL CMDOdbcDisconnectAll(void)
{
long l1, l2;
DWORD dw1;
RETCODE retcode;


   // for now, ignore errors.

   // TODO:  report errors (if any) but don't stop process



   if(!lpODBCX) return(FALSE);  // no problem

   for(l1=wNODBCX - 1; l1 >= 0; l1--)
   {
      // commit any pending transactions

      retcode = lpSQLGetConnectOption(lpODBCX[l1].hDbc, SQL_AUTOCOMMIT,
                                      &dw1);
//      if(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
//      {
//
//         break;
//      }

      if((lpODBCX[l1].dwFlags & ODBC_RESULT_SET)  // there IS a result set
         && dw1 != SQL_AUTOCOMMIT_ON)             // assume 'auto commit' is off
      {
         // COMMIT the current transaction!!  Will succeed if none

         retcode = lpSQLTransact(lpODBCX[l1].hEnv, lpODBCX[l1].hDbc,
                                 SQL_COMMIT);

//         if(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
//         {
//            rval = TRUE;
//            break;
//         }
      }


      lpSQLFreeStmt(lpODBCX[l1].hStmt, SQL_DROP);

      for(l2=0; l2<l1; l2++)
      {
         if(lpODBCX[l1].hEnv == lpODBCX[l2].hEnv &&
            lpODBCX[l1].hDbc == lpODBCX[l2].hDbc)
         {
            break;
         }
      }

      if(l2 >= l1)  // wasn't found - this is the last one
      {
         lpSQLDisconnect(lpODBCX[l1].hDbc);
         lpSQLFreeConnect(lpODBCX[l1].hDbc);
         lpSQLFreeEnv(lpODBCX[l1].hEnv);
      }

      if(lpODBCX[l1].lpParmInfo) GlobalFreePtr(lpODBCX[l1].lpParmInfo);
      if(lpODBCX[l1].lpBindInfo) GlobalFreePtr(lpODBCX[l1].lpBindInfo);

      _hmemset(lpODBCX + l1, 0, sizeof(*lpODBCX));  // ensure it's 'zeroed'
   }

   GlobalFreePtr(lpODBCX);

   lpODBCX = NULL;
   wNODBCX = 0;

   return(FALSE);
}



BOOL FAR PASCAL CMDOdbc    (LPSTR lpCmd, LPSTR lpArgs)
{
LPSTR lp1, lp2, lp3, lpCommand;
WORD wOption, wLen, w1, w2, w3, w4;
SWORD s1, s2, s3, s4;
DWORD dw1, dw2;
long l1;
//HCURSOR hCursor;
BOOL rval;
//HENV hEnv;
//HDBC hDbc;
//HSTMT hStmt;
RETCODE retcode;
char tbuf[512], tbuf2[64];


   // The following is a short list of commands (for my reference)
   //
   // ODBC BIND {CONV} {fieldname} {variable}      (fieldname may be in '[ ]')

   // ODBC CANCEL {CONV}

   // ODBC CONNECT [{DSN=dsn[;UID=user[;PWD=password]][...]}]

   // ODBC CREATE|DROP STATEMENT {CONV}

   // ODBC CURSOR {CONV} {cursor name}

   // ODBC DELETE {CONV}

   // ODBC DISCONNECT {CONV}|ALL

   // ODBC ERROR [{CONV}]

   // ODBC EXECUTE {CONV}

   // ODBC FILTER {CONV}

   // ODBC FIRST|LAST|NEXT|PREVIOUS {CONV}

   // ODBC INSERT {CONV}

   // ODBC LOCK|UNLOCK {CONV}

   // ODBC OPEN [READ|UPDATE] {CONV} {TABLE or VIEW}

   // ODBC PARAMETERS {CONV} {value list "a",1,...}|{array variable}

   // ODBC SQL [BEGIN|END] {CONV} [{SQL STRING}]

   // ODBC TRANSACT BEGIN|END|ROLLBACK|CANCEL {CONV}
   //      (ODBC TRANSACT CANCEL similar to ODBC CANCEL)

   // ODBC UPDATE {CONV}





   if(!hODBC)
   {
      PrintErrorMessage(880);  // "ODBC has not been installed properly\r\n"

      SetEnvString(szODBC_RESULT, szERROR);
      return(TRUE);
   }

   lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);

      SetEnvString(szODBC_RESULT, szERROR);
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


   for(wOption=0; wOption<wNOdbcOption; wOption++)
   {
      if(_fstrnicmp(lp1, pOdbcOption[wOption], lstrlen(lp1))==0)
      {
         break;
      }
   }

   if(wOption>=wNOdbcOption)
   {
      PrintString(pSYNTAXERROR);
      PrintString(lpArgs);
      PrintString(pQCRLFLF);

      SetEnvString(szODBC_RESULT, szERROR);

      GlobalFreePtr(lpArgs);
      return(TRUE);
   }


   /* at this point 'lp2' points to the remaining args on the string */


   switch(wOption)
   {
      case CMDODBC_ERROR:

         // returns an array:  element 0 = SQLSTATE, element 1 = error code,
         //                    element 2 = error message text

         if(*lp2)  // there is something else on the line...
         {
            w1 = CMDOdbcParseID((LPCSTR FAR *)&lp2);

            if(w1 < wNODBCX)
            {
               if(lpODBCX[w1].szLastError[0])
               {
                  SetEnvString(szODBC_RESULT, lpODBCX[w1].szLastError);
                  lpODBCX[w1].szLastError[0] = 0;

                  rval = FALSE;

                  GlobalFreePtr(lpArgs);
                  return(rval);    // bail out here - it's easier
               }


               l1 = 0;

               lpSQLError(lpODBCX[w1].hEnv, lpODBCX[w1].hDbc, lpODBCX[w1].hStmt,
                          (UCHAR FAR *)pSqlState, (SDWORD FAR *)&l1,
                          (UCHAR FAR *)tbuf, sizeof(tbuf), (SWORD FAR *)&wLen);

               wsprintf(work_buf, "%s\t%ld\t%s", (LPSTR)pSqlState, l1,
                        (LPSTR)tbuf);

               rval = FALSE;
            }
            else
            {
               PrintErrorMessage(886); // ODBC ERROR - invalid conversation value

               rval = TRUE; // error...
            }
         }
         else
         {
            l1 = 0;

            lpSQLError(SQL_NULL_HENV, SQL_NULL_HDBC, SQL_NULL_HSTMT,
                       (UCHAR FAR *)pSqlState, (SDWORD FAR *)&l1,
                       (UCHAR FAR *)tbuf, sizeof(tbuf), (SWORD FAR *)&wLen);

            wsprintf(work_buf, "%s\t%ld\t%s", (LPSTR)pSqlState, l1,
                     (LPSTR)tbuf);

            SetEnvString(szODBC_RESULT, work_buf);

            rval = FALSE;
         }

         if(rval)
         {
            SetEnvString(szODBC_RESULT, szERROR);
         }
         else
         {
            SetEnvString(szODBC_RESULT, work_buf);
         }

         GlobalFreePtr(lpArgs);
         return(rval);



      case CMDODBC_CONNECT:

         rval = CMDOdbcConnect(lp2, tbuf, sizeof(tbuf));

         if(rval) break;            // normal processing on error


         // at this point, 'ODBC_RESULT' must not be modified

         GlobalFreePtr(lpArgs);     // don't forget to free this...
         return(FALSE);             // OK!



      case CMDODBC_CREATE:  // CREATE STATEMENT
      case CMDODBC_DROP:    // DROP STATEMENT

         // 'lp2' should point to 'STATEMENT' (for now, only allowed keyword)

         for(lp3=lp2; *lp2>' '; lp2++)
            ;  // isolate the keyword in the string

         wLen = (WORD)(lp2 - lp3);

         while(*lp2 && *lp2 <=' ') lp2++;  // point to the 'conversation ID'

         w1 = CMDOdbcParseID((LPCSTR FAR *)&lp2);

         if(w1 >= wNODBCX)
         {
            PrintErrorMessage(886); // ODBC ERROR - invalid conversation value

            rval = TRUE;
            break;
         }

         lpODBCX[w1].szLastError[0] = 0;  // no error!

         if(!KEYWORD_MATCH(lp3,szSTATEMENT,wLen))
         {
            PrintErrorMessage(885); // "ODBC CREATE/DROP missing 'STATEMENT' keyword\r\n\n";

            rval = TRUE;
            break;
         }
         else
         {
            // First, find current entry and duplicate it

            if(wOption == CMDODBC_CREATE)
            {
               lpODBCX[wNODBCX] = lpODBCX[w1];

               // NOTE:  parameters and bound columns must NOT be transferred

               lpODBCX[wNODBCX].lpParmInfo = NULL;
               lpODBCX[wNODBCX].lpBindInfo = NULL;

               lpODBCX[wNODBCX].dwFlags &= ~ODBC_RESULT_SET;
               lpODBCX[wNODBCX].dwCurPos = 0;   // nothing has been read
               lpODBCX[wNODBCX].dwRowCount = 0; // no records in result set (yet)
               lpODBCX[wNODBCX].szTableName[0] = 0; // table name also initialized
               lpODBCX[wNODBCX].szLastError[0] = 0; // and 'last error message'


               w1 = wNODBCX;

               retcode = lpSQLAllocStmt(lpODBCX[w1].hDbc,
                                        &(lpODBCX[w1].hStmt));

               if(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
               {
                  _hmemset((LPSTR)(lpODBCX + w1), 0, sizeof(*lpODBCX));

                  rval = TRUE;
                  break;
               }

               wNODBCX++;  // it's added to the list now!

               // write 'conversation ID' for new conversation

               wsprintf(tbuf, "%08lx%08lx%08lx",
                        (DWORD)lpODBCX[w1].hEnv,
                        (DWORD)lpODBCX[w1].hDbc,
                        (DWORD)lpODBCX[w1].hStmt);

               // assign value to 'ODBC_RESULT'

               SetEnvString(szODBC_RESULT, tbuf);

               GlobalFreePtr(lpArgs);     // don't forget to free this...
               return(FALSE);             // OK!
            }
            else
            {
               goto do_the_disconnect; // for now, same as 'ODBC DISCONNECT'
            }

            rval = FALSE;
         }

         break;



      case CMDODBC_DISCONNECT:

         // has user specified "disconnect all"?

         for(lp3=lp2; *lp3>' '; lp3++)
            ;  // isolate the 'ALL' keyword in the string (if any)

         wLen = (WORD)(lp3 - lp2);

         if(KEYWORD_MATCH(lp2,szALL,wLen))
         {
            lp2 = lp3;

            while(*lp2 && *lp2 <= ' ') lp2++;  // trailing white space

            if(*lp2)  // SYNTAX ERROR
            {
               PrintString(pSYNTAXERROR);
               PrintString(lpArgs);
               PrintString(pQCRLFLF);

               rval = TRUE;
               break;
            }

            // DISCONNECT ALL!

            rval = CMDOdbcDisconnectAll();
            break;
         }

         w1 = CMDOdbcParseID((LPCSTR FAR *)&lp2);

         if(w1 >= wNODBCX)
         {
            PrintErrorMessage(886); // ODBC ERROR - invalid conversation value

            rval = TRUE;
         }
         else
         {
            lpODBCX[w1].szLastError[0] = 0;  // no error!

do_the_disconnect:

            // commit any pending transactions

            rval = FALSE;

            retcode = lpSQLGetConnectOption(lpODBCX[w1].hDbc, SQL_AUTOCOMMIT,
                                            &dw1);
            if(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
            {
               rval = TRUE;
            }
            else if((lpODBCX[w1].dwFlags & ODBC_RESULT_SET)  // there IS a result set
                    && dw1 != SQL_AUTOCOMMIT_ON)      // assume 'auto commit' is off
            {
               // COMMIT the current transaction!!  Will succeed if none

               retcode = lpSQLTransact(lpODBCX[w1].hEnv, lpODBCX[w1].hDbc,
                                       SQL_COMMIT);

               if(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
               {
                  rval = TRUE;
               }
            }


            // TODO:  error/warning message if 'rval' is TRUE at this point


            // FREE ALL RESOURCES for this entry in the CONNECTION TABLE

            lpSQLFreeStmt(lpODBCX[w1].hStmt, SQL_DROP);


            // see if there is a matching 'hStmt/hDbc' combination,
            // and if not, free this one (it's the last one...!)

            for(w2=0; w2<wNODBCX; w2++)
            {
               if(w1 != w2 &&
                  lpODBCX[w1].hEnv == lpODBCX[w2].hEnv &&
                  lpODBCX[w1].hDbc == lpODBCX[w2].hDbc)
               {
                  break;
               }
            }

            if(w2 >= wNODBCX)  // wasn't found - this is the last one
            {
               if(lpSQLDisconnect(lpODBCX[w1].hDbc) != SQL_SUCCESS)
               {
                  l1 = 0;

                  lpSQLError(lpODBCX[w1].hEnv, lpODBCX[w1].hDbc, SQL_NULL_HSTMT,
                             (UCHAR FAR *)pSqlState, (SDWORD FAR *)&l1,
                             (UCHAR FAR *)tbuf, sizeof(tbuf), (SWORD FAR *)&wLen);

                  wsprintf(work_buf, "?ODBC DISCONNECT (1) - %s\t%ld\t%s\r\n",
                           (LPSTR)pSqlState, l1, (LPSTR)tbuf);

                  PrintString(tbuf);
               }

               if(lpSQLFreeConnect(lpODBCX[w1].hDbc) != SQL_SUCCESS)
               {
                  l1 = 0;

                  lpSQLError(lpODBCX[w1].hEnv, lpODBCX[w1].hDbc, SQL_NULL_HSTMT,
                             (UCHAR FAR *)pSqlState, (SDWORD FAR *)&l1,
                             (UCHAR FAR *)tbuf, sizeof(tbuf), (SWORD FAR *)&wLen);

                  wsprintf(work_buf, "?ODBC DISCONNECT (2) - %s\t%ld\t%s\r\n",
                           (LPSTR)pSqlState, l1, (LPSTR)tbuf);

                  PrintString(tbuf);
               }

               if(lpSQLFreeEnv(lpODBCX[w1].hEnv) != SQL_SUCCESS)
               {
                  l1 = 0;

                  lpSQLError(lpODBCX[w1].hEnv, SQL_NULL_HDBC, SQL_NULL_HSTMT,
                             (UCHAR FAR *)pSqlState, (SDWORD FAR *)&l1,
                             (UCHAR FAR *)tbuf, sizeof(tbuf), (SWORD FAR *)&wLen);

                  wsprintf(work_buf, "?ODBC DISCONNECT (3) - %s\t%ld\t%s\r\n",
                           (LPSTR)pSqlState, l1, (LPSTR)tbuf);

                  PrintString(tbuf);
               }
            }

            if(lpODBCX[w1].lpParmInfo) GlobalFreePtr(lpODBCX[w1].lpParmInfo);
            lpODBCX[w1].lpParmInfo = NULL;

            if(lpODBCX[w1].lpBindInfo) GlobalFreePtr(lpODBCX[w1].lpBindInfo);
            lpODBCX[w1].lpBindInfo = NULL;


            // REMOVE THE AFFECTED ITEM FROM THE CONNECTION TABLE!

            wNODBCX--;  // reduce count first

            if(w1 < wNODBCX)
            {
               _hmemcpy((LPSTR)(lpODBCX + w1), (LPSTR)(lpODBCX + w1 + 1),
                        (wNODBCX - w1) * sizeof(*lpODBCX));
            }

            if(wNODBCX)  // do I free the array?  If empty, free it!
            {
               _hmemset((LPSTR)(lpODBCX + wNODBCX), 0, sizeof(*lpODBCX));
            }
            else
            {
               GlobalFreePtr(lpODBCX);

               lpODBCX = NULL;
            }

            rval = FALSE;

         }

         break;




      case CMDODBC_FIRST:
      case CMDODBC_LAST:
      case CMDODBC_NEXT:
      case CMDODBC_PREVIOUS:


         w1 = CMDOdbcParseID((LPCSTR FAR *)&lp2);

         if(w1 >= wNODBCX)
         {
            PrintErrorMessage(886); // ODBC ERROR - invalid conversation value

            rval = TRUE;
            break;
         }

         lpODBCX[w1].szLastError[0] = 0;  // no error!

         if(!(lpODBCX[w1].dwFlags & ODBC_RESULT_SET))
         {
            SetEnvString(szODBC_RESULT, szEOF);

            GlobalFreePtr(lpArgs);

            return(FALSE);
         }

         if(!lpODBCX[w1].dwRowCount)               // no records?
         {
            // see if there are records to be read (at all)
//
//            retcode = lpSQLMoreResults(lpODBCX[w1].hStmt);
//
//            if(retcode != SQL_SUCCESS)
//            {
//               // get the error code on this one... (am I still
//               // waiting for results to come back?)
//
//
//
//               else
//               {
//                  rval = TRUE;
//                  break;
//               }
//            }
         }

         // if we are going to be using 'SQLExtendedFetch' make sure
         // that the rowset size is 1

         if(lpODBCX[w1].dwFlags & ODBC_CURSOR_ABSOLUTE)
         {
            if((lpODBCX[w1].dwFlags & ODBC_RELEASE2) && lpSQLSetStmtOption)
            {
               lpSQLSetStmtOption(lpODBCX[w1].hStmt, SQL_ROWSET_SIZE, 1);
            }
            else
            {
               lpSQLSetScrollOptions(lpODBCX[w1].hStmt, SQL_CONCUR_VALUES,
                                     1, 1);
            }
         }

         rval = FALSE;  // this is a flag...

         switch(wOption)
         {
            case CMDODBC_FIRST:

               if(lpODBCX[w1].dwCurPos &&     // ONLY if it's NOT ZERO!
                  !(lpODBCX[w1].dwFlags & ODBC_CURSOR_ABSOLUTE))
               {
                  PrintErrorMessage(921); // ODBC ERROR - driver does not support 'SQLExtendedFetch()'

                  rval = TRUE;
                  break;
               }

               lpODBCX[w1].dwCurPos = 1;

               break;



            case CMDODBC_LAST:

               if(!(lpODBCX[w1].dwFlags & ODBC_CURSOR_ABSOLUTE))
               {
                  PrintErrorMessage(921); // ODBC ERROR - driver does not support 'SQLExtendedFetch()'

                  rval = TRUE;
                  break;
               }

               lpODBCX[w1].dwCurPos = 0xffffffffL;  // i.e. LAST row...

               break;



            case CMDODBC_NEXT:

               lpODBCX[w1].dwCurPos++;

               break;



            case CMDODBC_PREVIOUS:

               if(!(lpODBCX[w1].dwFlags & ODBC_CURSOR_ABSOLUTE))
               {
                  PrintErrorMessage(921); // ODBC ERROR - driver does not support 'SQLExtendedFetch()'

                  rval = TRUE;
                  break;
               }

               if(!lpODBCX[w1].dwCurPos)
               {
                  lpODBCX[w1].dwCurPos = 0xffffffff;
               }
               else
               {
                  lpODBCX[w1].dwCurPos--;
               }

               break;

         }

         if(rval) break;

         do
         {
            if(!lpODBCX[w1].dwCurPos && wOption==CMDODBC_PREVIOUS)
            {
               retcode = SQL_NO_DATA_FOUND;  // only when moving back...
            }
            else
            {
               dw1 = 1;

               if(!(lpODBCX[w1].dwFlags & ODBC_CURSOR_ABSOLUTE))
               {
                  retcode = lpSQLFetch(lpODBCX[w1].hStmt);
               }
               else
               {
                  retcode = lpSQLExtendedFetch(lpODBCX[w1].hStmt,
                                               SQL_FETCH_ABSOLUTE,
                                               lpODBCX[w1].dwCurPos,
                                               (UDWORD FAR *)&dw1,
                                               (UWORD FAR *)tbuf);
               }
            }

            if(retcode != SQL_STILL_EXECUTING) break;

            LoopDispatch();

         } while(TRUE);


         if(retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
         {
            if(lpODBCX[w1].dwFlags & ODBC_RELEASE2)
            {
               retcode = lpSQLGetStmtOption(lpODBCX[w1].hStmt,
                                            SQL_ROW_NUMBER, &dw1);

               if((retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
                  && dw1)
               {
                  lpODBCX[w1].dwCurPos = dw1;  // correct the actual position
               }
            }


            // check for BOUND columns at this point, and update their
            // associated variables...

            lp1 = lpODBCX[w1].lpBindInfo;

            if(lp1)
            {
               // format of BIND data is:  VARNAME\0{length}{col}{cbValue}DATA

               while(*lp1)
               {
                  lp2 = lp1;   // 'lp1' is the variable name

                  while(*lp2) lp2++;

                  lp2++;       // 'lp2' points to data size (WORD)
                               // followed by column number (WORD)
                               // followed by 'cbValue' (DWORD)

                  if(*((DWORD FAR *)(lp2 + 2 * sizeof(WORD)))==SQL_NULL_DATA)
                  {
                     // TODO:  handle NULL values

                     SetEnvString(lp1, "");
                  }
                  else
                  {
                     if(*((DWORD FAR *)(lp2 + 2 * sizeof(WORD)))==SQL_NTS)
                     {
                        *((DWORD FAR *)(lp2 + 2 * sizeof(WORD))) =
                           lstrlen(lp2 + 2 * sizeof(WORD) + sizeof(DWORD));
                     }
                     else
                     {
                        *(lp2 + 2 * sizeof(WORD) + sizeof(DWORD) +
                          *((DWORD FAR *)(lp2 + 2 * sizeof(WORD)))) = 0;
                     }

                     SetEnvString(lp1, lp2 + 2 * sizeof(WORD) + sizeof(DWORD));
                  }


                  // END OF LOOP!
                  // point 'lp1' at next entry (or zero byte to mark end)

                  lp1 = lp2 + *((WORD FAR *)lp2) + sizeof(WORD);
               }
            }

            rval = FALSE;
         }
         else if(retcode == SQL_NO_DATA_FOUND)
         {
            SetEnvString(szODBC_RESULT, szEOF);

            GlobalFreePtr(lpArgs);

            return(FALSE);
         }
         else
         {
            rval = TRUE;
         }

         break;



      case CMDODBC_CURSOR:   // assign the cursor name

         w1 = CMDOdbcParseID((LPCSTR FAR *)&lp2);

         if(w1 >= wNODBCX)
         {
            PrintErrorMessage(886); // ODBC ERROR - invalid conversation value

            rval = TRUE;
            break;
         }

         lpODBCX[w1].szLastError[0] = 0;  // no error!

         // lp2 should now point to a cursor name.  Assign the
         // current cursor name to this cursor name using the
         // ODBC driver.  If the driver does not support cursor names,
         // this will return an error state.  If the driver does not
         // support multiple cursors, and a different name is used, the
         // previous cursor name will be deleted.
         //
         // cursor names are only used for SQL commands that reference them
         // cursor names are defined before a SELECT query is performed


         while(*lp2 && *lp2 <= ' ') lp2++;  // next non-white-space

         if(*lp2 == '\'' || *lp2=='\"')
         {
          char quote = *(lp2++);


            for(lp3 = lp2; *lp3 && *lp3 != quote; lp3++)
              ;  // next non-quote or NULL

            if(*lp3) *(lp3++) = 0;  // ensure string is terminated
         }
         else
         {
            for(lp3=lp2; *lp3 > ' '; lp3++)
              ;  // find next white space

            if(*lp3) *(lp3++) = 0;  // ensure string is terminated
         }

         if(*lp3)
         {
            while(*lp3 && *lp3 <= ' ') lp3++;
         }

         if(*lp3)
         {
            PrintString(pSYNTAXERROR);
            PrintString(lp3);
            PrintString(pQCRLFLF);

            rval = TRUE;
            break;
         }

         // at THIS point, 'lp2' points to the cursor name.  NOW, set
         // the thing using 'SQLSetCursorName()'

         retcode = lpSQLSetCursorName(lpODBCX[w1].hStmt, (UCHAR FAR *)lp2, SQL_NTS);
         if(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
         {
            rval = TRUE;
            break;
         }

         if(retcode == SQL_SUCCESS_WITH_INFO)
         {
            // TODO:  execute SQLError() to get error/warning info
         }

         rval = FALSE;
         break;



      case CMDODBC_OPEN:

         // special case - open single table for read/write operations

         w2 = FALSE;  // use 'w2' as my 'READ ONLY' flag


         // see if I can support updates at all...

         retcode = lpSQLGetInfo(lpODBCX[w1].hDbc, SQL_DATA_SOURCE_READ_ONLY,
                                tbuf, sizeof(tbuf), &s1);

         if(retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
         {
            tbuf[s1] = 0;

            if(toupper(*tbuf)=='Y')  // read-only?
            {
               w2 = TRUE;            // default mode is now read-only
            }
         }


         w1 = CMDOdbcParseID((LPCSTR FAR *)&lp2);


         if(w1 >= wNODBCX)
         {
            PrintErrorMessage(886); // ODBC ERROR - invalid conversation value

            rval = TRUE;
            break;
         }

         lpODBCX[w1].szLastError[0] = 0;  // no error!

         // commit any pending transactions

         retcode = lpSQLGetConnectOption(lpODBCX[w1].hDbc, SQL_AUTOCOMMIT,
                                         &dw1);
         if(retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
         {
            if((lpODBCX[w1].dwFlags & ODBC_RESULT_SET)  // there IS a result set
               && dw1 != SQL_AUTOCOMMIT_ON) // assume 'auto commit' is off
            {
               // COMMIT the current transaction!!  Will succeed if none

               retcode = lpSQLTransact(lpODBCX[w1].hEnv, lpODBCX[w1].hDbc,
                                       SQL_COMMIT);

               if(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
               {
                  rval = TRUE;
                  break;
               }
            }
         }


         // get rid of any parameters

         if(lpODBCX[w1].lpParmInfo)
         {
            retcode = lpSQLFreeStmt(lpODBCX[w1].hStmt, SQL_RESET_PARAMS);
            if(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
            {
               rval = TRUE;
               break;
            }

            GlobalFreePtr(lpODBCX[w1].lpParmInfo);

            lpODBCX[w1].lpParmInfo = NULL;
         }

         // get rid of any bound columns

         if(lpODBCX[w1].lpBindInfo)
         {
            retcode = lpSQLFreeStmt(lpODBCX[w1].hStmt, SQL_UNBIND);
            if(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
            {
               rval = TRUE;
               break;
            }

            GlobalFreePtr(lpODBCX[w1].lpBindInfo);

            lpODBCX[w1].lpBindInfo = NULL;
         }


         // drop previous cursor (if any)

         retcode = lpSQLFreeStmt(lpODBCX[w1].hStmt, SQL_CLOSE);
         if(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
         {
            rval = TRUE;
            break;
         }


         // reset important parameters now

         lpODBCX[w1].dwFlags &= ~ODBC_RESULT_SET;
         lpODBCX[w1].dwCurPos = 0;   // nothing has been read
         lpODBCX[w1].dwRowCount = 0; // no records in result set (yet)
         lpODBCX[w1].szTableName[0] = 0;  // clear table name also
         lpODBCX[w1].szLastError[0] = 0;  // and 'last error'


         while(*lp2 && *lp2<=' ') lp2++;  // get table name or keyword
         for(lp3=lp2; *lp3 > ' '; lp3++)
            ;                             // next non-white-space

         if((lp3 - lp2)==4 && !_fstrnicmp(lp2, "READ", 4))
         {
            w2 = TRUE;

            lp2 = lp3;

            while(*lp2 && *lp2<=' ') lp2++;  // get table name
         }
         else if((lp3 - lp2)==9 && !_fstrnicmp(lp2, "READWRITE", 9))
         {
            w2 = FALSE;

            lp2 = lp3;

            while(*lp2 && *lp2<=' ') lp2++;  // get table name
         }

         if(!*lp2)
         {
            PrintErrorMessage(887); // "ODBC OPEN ERROR - missing table name in 'ODBC OPEN'\r\n\n";
            rval = TRUE;
            break;
         }

         // reset cursor options to support concurrency (ignore errors)

         lpSQLSetStmtOption(lpODBCX[w1].hStmt, SQL_CONCURRENCY,
                            SQL_CONCUR_LOCK);

         // prepare special SQL program

         wsprintf(tbuf, "SELECT * FROM %s", lp2);

         // see if I need to add "FOR UPDATE" to select statement for a
         // subsequent positioned update query...

         if(!w2)             // user is NOT in 'READ ONLY' mode...
         {
            lstrcat(tbuf, " FOR UPDATE");
         }

         do
         {
            retcode = lpSQLExecDirect(lpODBCX[w1].hStmt, (UCHAR FAR *)tbuf,
                                      lstrlen(tbuf));

            if(retcode == SQL_STILL_EXECUTING)
            {
               LoopDispatch();
            }

         } while(retcode == SQL_STILL_EXECUTING);

         if(retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
         {
            lpODBCX[w1].dwFlags |= ODBC_RESULT_SET;

            lstrcpy(lpODBCX[w1].szTableName, lp2);  // save the table name
            rval = FALSE;
         }
         else
         {
            rval = TRUE;
         }

         break;



      case CMDODBC_SQL:

         w1 = CMDOdbcParseID((LPCSTR FAR *)&lp2);

         if(w1 >= wNODBCX)
         {
            PrintErrorMessage(886); // ODBC ERROR - invalid conversation value

            rval = TRUE;
            break;
         }

         lpODBCX[w1].szLastError[0] = 0;  // no error!

         // commit any pending transactions

         retcode = lpSQLGetConnectOption(lpODBCX[w1].hDbc, SQL_AUTOCOMMIT,
                                         &dw1);
         if(retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
         {
            if((lpODBCX[w1].dwFlags & ODBC_RESULT_SET)  // there IS a result set
               && dw1 != SQL_AUTOCOMMIT_ON) // assume 'auto commit' is off
            {
               // COMMIT the current transaction!!  Will succeed if none

               retcode = lpSQLTransact(lpODBCX[w1].hEnv, lpODBCX[w1].hDbc,
                                       SQL_COMMIT);

               if(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
               {
                  rval = TRUE;
                  break;
               }
            }
         }


         // get rid of any parameters

         if(lpODBCX[w1].lpParmInfo)
         {
            retcode = lpSQLFreeStmt(lpODBCX[w1].hStmt, SQL_RESET_PARAMS);
            if(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
            {
               rval = TRUE;
               break;
            }

            GlobalFreePtr(lpODBCX[w1].lpParmInfo);

            lpODBCX[w1].lpParmInfo = NULL;
         }

         // get rid of any bound columns

         if(lpODBCX[w1].lpBindInfo)
         {
            retcode = lpSQLFreeStmt(lpODBCX[w1].hStmt, SQL_UNBIND);
            if(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
            {
               rval = TRUE;
               break;
            }

            GlobalFreePtr(lpODBCX[w1].lpBindInfo);

            lpODBCX[w1].lpBindInfo = NULL;
         }


         // drop previous cursor (if any)

         retcode = lpSQLFreeStmt(lpODBCX[w1].hStmt, SQL_CLOSE);
         if(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
         {
            rval = TRUE;
            break;
         }


         // reset important parameters now

         lpODBCX[w1].dwFlags &= ~ODBC_RESULT_SET;
         lpODBCX[w1].dwCurPos = 0;   // nothing has been read
         lpODBCX[w1].dwRowCount = 0; // no records in result set (yet)
         lpODBCX[w1].szTableName[0] = 0;  // clear table name also
         lpODBCX[w1].szLastError[0] = 0;  // and 'last error'


         // reset cursor options to support concurrency (ignore errors)

         lpSQLSetStmtOption(lpODBCX[w1].hStmt, SQL_CONCURRENCY,
                            SQL_CONCUR_LOCK);

         // see if program is on same line as 'ODBC SQL'

         while(*lp2 && *lp2<=' ') lp2++;  // get remainder of command

         if(!*lp2 && (!BatchMode || !lpBatchInfo)) // block syntax and NOT batch?
         {
            PrintErrorMessage(888); // ODBC SQL ERROR - specified syntax requires batch file

            rval = TRUE;
            break;
         }

         if(*lp2)     // SQL command is on remainder of command line...
         {
            do
            {
               retcode = lpSQLPrepare(lpODBCX[w1].hStmt, (UCHAR FAR *)lp2, lstrlen(lp2));

               if(retcode == SQL_STILL_EXECUTING)
               {
                  LoopDispatch();
               }

            } while(retcode == SQL_STILL_EXECUTING);

            rval = retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO;

            break;
         }


         // at this point, command identifies a block of SQL statements to
         // be included within the same SQL prepared statement


         // this specifies a block of commands within a batch file

         lpCommand = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE, 8192);

         if(!lpCommand)
         {
            PrintString(pNOTENOUGHMEMORY);
            return(TRUE);
         }

         *lpCommand = 0;
         wLen = (WORD)GlobalSizePtr(lpCommand);


         // NOW I get to read through the batch file until I find an 'ODBC SQL END'

         do
         {
            lp3 = GetAnotherBatchLine(lpBatchInfo);
            if(!lp3)
            {
               PrintString("?Error reading batch file (missing 'ODBC SQL END'?)\r\n\n");

               TerminateCurrentBatchFile();
               return(TRUE);
            }

            // if command is to be echoed, allow use of '@' at start of
            // line to suppress the echo.

            if(*lp3!='@' && BatchEcho &&
               hMainWnd && IsWindow(hMainWnd))  /* echoing commands */
            {
               DisplayPrompt();
               PrintString(lp3);
               PrintString(pCRLF);
            }

            lp1 = lp3;

            if(*lp1=='@')
            {
               lp1++;
            }
            else if(*lp1==':')
            {
               PrintString("?Mis-placed label inside 'SQL' block - ignored\r\n\n");
               GlobalFreePtr(lp3);
               continue;
            }

            // reserved for later - detect comments here and have a
            // commented-out line do a 'continue'


            while(*lp1 && *lp1<=' ') lp1++;

            if(*lp1)
            {
               // PARSE LINE - look for 'SQL END'

               for(lp2=lp1; *lp2>' '; lp2++)
                  ;  // the first word!


               if(KEYWORD_MATCH(lp1,szODBC,(lp2 - lp1)))
               {
                  lp1 = lp2;
                  while(*lp1 && *lp1<=' ') lp1++;

                  for(lp2=lp1; *lp2>' '; lp2++)
                     ;  // the next word!

                  if(KEYWORD_MATCH(lp1,szSQL,(lp2 - lp1)))
                  {
                     lp1 = lp2;
                     while(*lp1 && *lp1<=' ') lp1++;

                     for(lp2=lp1; *lp2>' '; lp2++)
                        ;  // the next word!

                     if(KEYWORD_MATCH(lp1,szEND,(lp2 - lp1)))
                     {
                        GlobalFreePtr(lp3);  // free the batch line

                        break;
                     }
                  }
               }
            }

            wLen = lstrlen(lpCommand) + lstrlen(lp3) + 4;

            if(wLen >= wLen)
            {
               lp1 = (LPSTR)GlobalReAllocPtr(lpCommand, wLen + 1024, GMEM_MOVEABLE);

               if(!lp1)
               {
                  PrintString(pNOTENOUGHMEMORY);

                  GlobalFreePtr(lpCommand);
                  GlobalFreePtr(lp3);

                  return(TRUE);
               }

               lpCommand = lp1;

               wLen = (WORD)GlobalSizePtr(lpCommand);
            }

            lp1 = lp3;
            if(*lp1=='@') lp1++;  // skip '@' on beginning of line (if present)

            lstrcat(lpCommand, lp1);
            lstrcat(lpCommand, pCRLF);

            GlobalFreePtr(lp3);     // must remember to free the batch line

         } while(TRUE);

         lp1 = lpCommand;
         while(*lp1 && *lp1<=' ') lp1++;

         if(!*lp1)
         {
            PrintErrorMessage(889); // "ODBC SQL WARNING - BLANK SQL Program ignored\r\n\n";
            rval = FALSE;
         }
         else
         {
            do
            {
               retcode = lpSQLPrepare(lpODBCX[w1].hStmt, (UCHAR FAR *)lp1, lstrlen(lp1));

               if(retcode == SQL_STILL_EXECUTING)
               {
                  LoopDispatch();
               }

            } while(retcode == SQL_STILL_EXECUTING);

            rval = retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO;

            if(!rval)
            {
               lpODBCX[w1].dwFlags |= ODBC_RESULT_SET;  // assume result set at THIS point
            }
         }

         GlobalFreePtr(lpCommand);

         break;



      case CMDODBC_EXECUTE:

         w1 = CMDOdbcParseID((LPCSTR FAR *)&lp2);

         if(w1 >= wNODBCX)
         {
            PrintErrorMessage(886); // ODBC ERROR - invalid conversation value

            rval = TRUE;
            break;
         }
         else
         {
            lpODBCX[w1].szLastError[0] = 0;  // no error!

            do
            {
               retcode = lpSQLExecute(lpODBCX[w1].hStmt);

               if(retcode == SQL_STILL_EXECUTING)
               {
                  LoopDispatch();
               }

            } while(retcode == SQL_STILL_EXECUTING);

            rval = retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO;
         }

         break;



      case CMDODBC_BIND:


         w1 = CMDOdbcParseID((LPCSTR FAR *)&lp2);

         if(w1 >= wNODBCX)
         {
            PrintErrorMessage(886); // ODBC ERROR - invalid conversation value

            rval = TRUE;
            break;
         }

         lpODBCX[w1].szLastError[0] = 0;  // no error!


         if(!lpODBCX[w1].lpBindInfo)
         {
            lpODBCX[w1].lpBindInfo = (LPSTR)
               GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT, 0x8000);

            if(!lpODBCX[w1].lpBindInfo)
            {
               PrintString(pNOTENOUGHMEMORY);

               rval = TRUE;
               break;
            }
         }

         lp1 = lpODBCX[w1].lpBindInfo;


         // format of BIND data is:  VARNAME\0{length}{col}{cbValue}DATA

         while(*lp1)
         {
            lp3 = lp1;   // 'lp1' is the variable name

            while(*lp3) lp3++;

            lp3++;       // 'lp3' points to data size (WORD)

            // END OF LOOP!
            // point 'lp1' at next entry (or zero byte to mark end)

            lp1 = lp3 + *((WORD FAR *)lp3) + sizeof(WORD);
         }

         // 'lp1' now points to the END of the bind information, and
         // 'lp2' points to the 'BIND' argument: {field name} {variable}
         // (fields with embedded spaces must be within '[]')


         while(*lp2 && *lp2<=' ') lp2++;
         if(!*lp2)
         {
            PrintErrorMessage(890); // "ODBC BIND - Command Syntax Error\r\n\n");

            rval = TRUE;
            break;
         }

         lp3 = lp2;
         if(*lp3=='[')
         {
            lp3 = ++lp2;

            while(*lp3 && *lp3!=']') lp3++;

            _hmemcpy(tbuf, lp2, lp3 - lp2);
            tbuf[lp3 - lp2] = 0;
            lp3++;
         }
         else
         {
            while(*lp3 > ' ') lp3++;

            _hmemcpy(tbuf, lp2, lp3 - lp2);
            tbuf[lp3 - lp2] = 0;
         }

         // 'tbuf' contains the table's field name or number

         lp2 = lp3;
         while(*lp2 && *lp2<=' ') lp2++;

         lp3 = lp2;
         if(!*lp3)
         {
            PrintErrorMessage(890); // "ODBC BIND - Command Syntax Error\r\n\n");

            rval = TRUE;
            break;
         }

         while(*lp3 > ' ') lp3++;

         if(*lp3)
         {
            *lp3 = 0;  // mark end of term - this is the environment var name
                       // pointed at by 'lp2'...
            lp3++;

            while(*lp3 && *lp3<=' ') lp3++;

            if(*lp3)   // too many arguments...
            {
               PrintErrorMessage(890); // "ODBC BIND - Command Syntax Error\r\n\n");

               rval = TRUE;
               break;
            }
         }

         w3 = CMDOdbcColumnNameIndex(lpODBCX + w1, tbuf);

         if(w3 == 0xffff)
         {
            rval = TRUE;
            break;
         }
         else if(!w3)
         {
            PrintErrorMessage(891); // "ODBC BIND - Invalid column name or number\r\n\n";
            rval = TRUE;
            break;
         }


         // OK, now I'm ready to actually bind the parameter.
         // 'w3' is the parameter number
         // 'lp2' points to the environment variable name
         // 'lp1' points to the end of the bind buffer


         // find out how much space I need first...

         retcode = lpSQLColAttributes(lpODBCX[w1].hStmt, w3,
                                      SQL_COLUMN_DISPLAY_SIZE, lp3,
                                      sizeof(DWORD), (SWORD FAR *)&wLen,
                                      (SDWORD FAR *)&dw1);

         if(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
         {
            dw1 = 0;
         }

         retcode = lpSQLColAttributes(lpODBCX[w1].hStmt, w3,
                                      SQL_COLUMN_LENGTH, lp3,
                                      sizeof(DWORD), (SWORD FAR *)&wLen,
                                      (SDWORD FAR *)&dw2);

         if(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
         {
            dw2 = 0;
         }

         dw1 = max(dw1, dw2) + 16;
                                       // ensure I use the maximum size


         // bind column 'w3' using 'SQL_C_CHAR' datatype to
         //  'lp1 + lstrlen(lp2) + 1 + sizeof(WORD)' for length 'dw1'

         retcode = lpSQLBindCol(lpODBCX[w1].hStmt, w3, SQL_C_CHAR,
                                lp1 + lstrlen(lp2) + 1
                                + 2 * sizeof(WORD) + sizeof(DWORD),
                                dw1,
                                (SDWORD FAR *)(lp1 + lstrlen(lp2) + 1
                                               + 2 * sizeof(WORD)));

         if(retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
         {
            lstrcpy(lp1, lp2);        // copy name of variable into storage
            lp1 += lstrlen(lp1) + 1;

            *((WORD FAR *)lp1) = (WORD)dw1 + sizeof(DWORD)
                               + sizeof(WORD);               // buf length
            lp1 += sizeof(WORD);

            *((WORD FAR *)lp1) = w3;                         // col #
            lp1 += sizeof(WORD);

            *((DWORD FAR *)lp1) = 0;                         // data length
            lp1 += dw1 + sizeof(DWORD);

            *lp1 = 0;                                        // data
            *(lp1 + 1) = 0;  // make SURE it terminates!

            rval = FALSE;
         }
         else
         {
            rval = TRUE;
         }

         break;




      case CMDODBC_PARAMETERS:

         // To make things simple, all parms should be "at run time" and
         // the data pointer is the position of the parameter's entry in
         // the parameter information block.
         //
         // Parms clear when the user specifies a new SQL query
         //


         PrintString("?ODBC option not (yet) supported\r\n\n");

         rval = TRUE;

         break;




      case CMDODBC_TRANSACT:

         w1 = CMDOdbcParseID((LPCSTR FAR *)&lp2);

         if(w1 >= wNODBCX)
         {
            PrintErrorMessage(886); // ODBC ERROR - invalid conversation value

            rval = TRUE;
            break;
         }
         else
         {
            lpODBCX[w1].szLastError[0] = 0;  // no error!

            // do I have transaction capability?

            retcode = lpSQLGetInfo(lpODBCX[w1].hDbc, SQL_TXN_CAPABLE,
                                   &l1, sizeof(l1), (SWORD FAR *)&wLen);

            if(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
            {
               rval = TRUE;
               break;
            }

            if(l1 == SQL_TC_NONE)
            {
               // transactions NOT supported...

               PrintErrorMessage(920);  // ODBC ERROR - Transactions NOT supported by this driver
               rval = TRUE;
               break;
            }

            // at this point 'l1' contains information about which transactions
            // are supported.
         }

         while(*lp2 && *lp2<=' ') lp2++;
         if(!*lp2)
         {
            PrintErrorMessage(924); // ODBC TRANSACT - Command Syntax Error

            rval = TRUE;
            break;
         }

         // the next keyword indicates the type of transaction action
         // expecting either BEGIN, END, CANCEL, or ROLLBACK

         for(lp3=lp2; *lp3>' '; lp3++)
            ;  // find the next keyword to determine how I handle this

         wLen = (WORD)(lp3 - lp2);

         while(*lp3 && *lp3 <= ' ') lp3++;  // point to remainder of string

         if(KEYWORD_MATCH(lp2,szBEGIN,wLen))
         {
            retcode = lpSQLGetConnectOption(lpODBCX[w1].hDbc, SQL_AUTOCOMMIT,
                                            &dw1);
            if(retcode == SQL_NO_DATA_FOUND)
            {

               // this is a valid condition - placeholder for code if needed

            }
            else if(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
            {
               retcode = lpSQLGetFunctions(lpODBCX[w1].hDbc,
                                           SQL_API_SQLGETCONNECTOPTION,
                                           &w3);
               if((retcode != SQL_SUCCESS &&
                   retcode != SQL_SUCCESS_WITH_INFO) || w3)
               {
                  rval = TRUE;
                  break;
               }
            }
            else if(dw1 != SQL_AUTOCOMMIT_ON)  // assume 'auto commit' is off
            {
               // COMMIT the current transaction!!  Will succeed if none

               retcode = lpSQLTransact(lpODBCX[w1].hEnv, lpODBCX[w1].hDbc,
                                       SQL_COMMIT);

               if(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
               {
                  rval = TRUE;
                  break;
               }
            }

            // this starts the transaction, actually, by assigning the
            // 'SQL_AUTOCOMMIT' parameter to 'SQL_AUTOCOMMIT_OFF'

            retcode = lpSQLSetConnectOption(lpODBCX[w1].hDbc, SQL_AUTOCOMMIT,
                                            SQL_AUTOCOMMIT_OFF);
            if(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
            {
               // initially, get the error message for the above failure

               l1 = 0;

               lpSQLError(lpODBCX[w1].hEnv, lpODBCX[w1].hDbc,
                          lpODBCX[w1].hStmt, (UCHAR FAR *)pSqlState,
                          (SDWORD FAR *)&l1, (UCHAR FAR *)tbuf,
                          sizeof(tbuf), (SWORD FAR *)&wLen);

               wsprintf(lpODBCX[w1].szLastError, "%s\t%ld\t%s",
                        (LPSTR)pSqlState, l1, (LPSTR)tbuf);



               retcode = lpSQLGetFunctions(lpODBCX[w1].hDbc,
                                           SQL_API_SQLSETCONNECTOPTION,
                                           &w3);
               if((retcode != SQL_SUCCESS &&
                   retcode != SQL_SUCCESS_WITH_INFO) || w3)
               {
                  rval = TRUE;
                  break;
               }

               // if it failed the above test (i.e. function not supported)
               // then clear the error message.  no problem!

               lpODBCX[w1].szLastError[0] = 0;
            }

            rval = FALSE;
         }
         else if(KEYWORD_MATCH(lp2,szEND,wLen))
         {
            // COMMIT the current transaction!!  Will succeed if none

            retcode = lpSQLTransact(lpODBCX[w1].hEnv, lpODBCX[w1].hDbc,
                                    SQL_COMMIT);

            if(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
            {
               rval = TRUE;
               break;
            }

            retcode = lpSQLSetConnectOption(lpODBCX[w1].hDbc, SQL_AUTOCOMMIT,
                                            SQL_AUTOCOMMIT_OFF);
            if(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
            {
               // initially, get the error message for the above failure

               l1 = 0;

               lpSQLError(lpODBCX[w1].hEnv, lpODBCX[w1].hDbc,
                          lpODBCX[w1].hStmt, (UCHAR FAR *)pSqlState,
                          (SDWORD FAR *)&l1, (UCHAR FAR *)tbuf,
                          sizeof(tbuf), (SWORD FAR *)&wLen);

               wsprintf(lpODBCX[w1].szLastError, "%s\t%ld\t%s",
                        (LPSTR)pSqlState, l1, (LPSTR)tbuf);



               retcode = lpSQLGetFunctions(lpODBCX[w1].hDbc,
                                           SQL_API_SQLSETCONNECTOPTION,
                                           &w3);
               if((retcode != SQL_SUCCESS &&
                   retcode != SQL_SUCCESS_WITH_INFO) || w3)
               {
                  rval = TRUE;
                  break;
               }

               // if it failed the above test (i.e. function not supported)
               // then clear the error message.  no problem!

               lpODBCX[w1].szLastError[0] = 0;
            }

            rval = FALSE;
         }
         else if(KEYWORD_MATCH(lp2,szROLLBACK,wLen))
         {
            retcode = lpSQLTransact(lpODBCX[w1].hEnv, lpODBCX[w1].hDbc,
                                    SQL_ROLLBACK);

            if(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
            {
               rval = TRUE;
               break;
            }

            rval = FALSE;
         }
         else if(KEYWORD_MATCH(lp2,szCANCEL,wLen))
         {
            // CANCEL is in lieu of 'END', but calling 'END' will literally
            // have no effect on the transaction.

            retcode = lpSQLTransact(lpODBCX[w1].hEnv, lpODBCX[w1].hDbc,
                                    SQL_ROLLBACK);

            if(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
            {
               rval = TRUE;
               break;
            }

            retcode = lpSQLSetConnectOption(lpODBCX[w1].hDbc, SQL_AUTOCOMMIT,
                                            SQL_AUTOCOMMIT_OFF);
            if(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
            {
               // initially, get the error message for the above failure

               l1 = 0;

               lpSQLError(lpODBCX[w1].hEnv, lpODBCX[w1].hDbc,
                          lpODBCX[w1].hStmt, (UCHAR FAR *)pSqlState,
                          (SDWORD FAR *)&l1, (UCHAR FAR *)tbuf,
                          sizeof(tbuf), (SWORD FAR *)&wLen);

               wsprintf(lpODBCX[w1].szLastError, "%s\t%ld\t%s",
                        (LPSTR)pSqlState, l1, (LPSTR)tbuf);



               retcode = lpSQLGetFunctions(lpODBCX[w1].hDbc,
                                           SQL_API_SQLSETCONNECTOPTION,
                                           &w3);
               if((retcode != SQL_SUCCESS &&
                   retcode != SQL_SUCCESS_WITH_INFO) || w3)
               {
                  rval = TRUE;
                  break;
               }

               // if it failed the above test (i.e. function not supported)
               // then clear the error message.  no problem!

               lpODBCX[w1].szLastError[0] = 0;
            }

            rval = FALSE;
         }
         else
         {
            PrintString(pSYNTAXERROR);
            PrintString(lp2);
            PrintString(pQCRLFLF);

            rval = TRUE;
            break;
         }


         break;



      case CMDODBC_CANCEL:

         w1 = CMDOdbcParseID((LPCSTR FAR *)&lp2);

         if(w1 >= wNODBCX)
         {
            PrintErrorMessage(886); // ODBC ERROR - invalid conversation value

            rval = TRUE;
            break;
         }
         else
         {
            lpODBCX[w1].szLastError[0] = 0;  // no error!

            retcode = lpSQLCancel(lpODBCX[w1].hDbc);

            rval = retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO;
         }

         break;



      case CMDODBC_LOCK:
      case CMDODBC_UNLOCK:

         PrintString("?ODBC option not (yet) supported\r\n\n");

         rval = TRUE;
         break;



      case CMDODBC_UPDATE:
      case CMDODBC_DELETE:
      case CMDODBC_INSERT:

         w1 = CMDOdbcParseID((LPCSTR FAR *)&lp2);

         if(w1 >= wNODBCX)
         {
            PrintErrorMessage(886); // ODBC ERROR - invalid conversation value

            rval = TRUE;
            break;
         }
         else
         {
            lpODBCX[w1].szLastError[0] = 0;  // no error!

            // do I have write access to the table?

            retcode = lpSQLGetInfo(lpODBCX[w1].hDbc,
                                   SQL_DATA_SOURCE_READ_ONLY,
                                   tbuf, sizeof(tbuf), (SWORD FAR *)&wLen);

            if(*tbuf=='Y')  // it's a READ-ONLY table
            {
               PrintErrorMessage(884); // "ODBC ERROR - driver only allows READ access on table\r\n\n"

               rval = TRUE;
               break;
            }


            rval = FALSE;  // initial value

            // FOR SIMPLICITY use 'SQLSetPos()' for all 3 of these; if the
            // driver won't support it, return the error message.
            //
            // IMPORTANT:  'row set' size must be 1; any other size will
            //             cause SERIOUS PROBLEMS!  This is taken care of
            //             automatically in the 'FIRST/LAST/NEXT/PREVIOUS'
            //             handler (above).


            // verify whether 'SQLExtendedFetch()' was used; if so, assume
            // that I can use 'SQLSetPos()' to perform the operations; else,
            // I must build an SQL string and use 'SQLExecDirect()' with a
            // positioned update using the current named cursor.

            if(!(lpODBCX[w1].dwFlags & ODBC_CURSOR_ABSOLUTE))
            {
             LPSTR lpSQL;

//               PrintErrorMessage(921); // ODBC ERROR - driver does not support 'SQLExtendedFetch()'
//
//               rval = TRUE;
//               break;

               // driver only supports sequential read through result set.
               // if it also supports POSITIONED UPDATE with CURSORS, I can
               // attempt to build and execute an SQL program to perform
               // the desired update operation.


               if(wOption != CMDODBC_INSERT)
               {
                  dw1 = 0;
                  retcode = lpSQLGetCursorName(lpODBCX[w1].hStmt,
                                               (UCHAR FAR *)tbuf, sizeof(tbuf),
                                               (SWORD FAR *)&dw1);

                  if(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
                  {
                     PrintErrorMessage(923); // ODBC ERROR - driver neither supports 'SQLExtendedFetch()' nor named cursors

                     rval = TRUE;
                     break;
                  }
               }


               lpSQL = (LPSTR)GlobalAllocPtr(GPTR, 0xffff);
               if(!lpSQL)
               {
                  PrintString(pNOTENOUGHMEMORY);

                  rval = TRUE;
                  break;
               }

               // build initial part of SQL string

               switch(wOption)
               {
                  case CMDODBC_UPDATE:  // NOTE:  bound fields only!

                     wsprintf(lpSQL, "UPDATE %s SET ",
                              (LPSTR)lpODBCX[w1].szTableName);

                     break;


                  case CMDODBC_DELETE:

                     wsprintf(lpSQL, "DELETE FROM %s WHERE CURRENT OF %s",
                              (LPSTR)lpODBCX[w1].szTableName,
                              (LPSTR)tbuf);

                     break;


                  case CMDODBC_INSERT:

                     wsprintf(lpSQL, "INSERT INTO %s (",
                              (LPSTR)lpODBCX[w1].szTableName);

                     break;

               }

               // check for BOUND columns at this point, and act
               // appropriately depending upon the desired operation.
               //
               // FOR UPDATE:  If data doesn't match, add a 'SET' clause for
               // that column to the SQL string.
               //
               // For all operations, ensure that bound data in the
               // 'bind info' array with the associated variables


               lp1 = lpODBCX[w1].lpBindInfo;
               w3 = 0;  // this acts as a flag as to how many fields changed

               if(lp1)
               {
                  // format of BIND data is:  VARNAME\0{length}{col}{cbValue}DATA

                  while(*lp1)
                  {
                   BOOL bChanged = FALSE;


                     lp2 = lp1;   // 'lp1' is the variable name

                     while(*lp2) lp2++;

                     lp2++;       // 'lp2' points to data size (WORD)
                                  // followed by column # (WORD)
                                  // followed by 'cbValue' (DWORD)

                     w2 = *((WORD FAR *)(lp2 + sizeof(WORD)));  // col #

                     *tbuf2 = 0;

                     lpSQLDescribeCol(lpODBCX[w1].hStmt, w2, 
                                      (UCHAR FAR *)tbuf2, sizeof(tbuf2),
                                      (SWORD FAR *)&s1, (SWORD FAR *)&s2,
                                      (UDWORD FAR *)&dw1, (SWORD FAR *)&s3,
                                      (SWORD FAR *)&s4);

                     tbuf2[s1] = 0;

                     // tbuf2 contains column name
                     // s2 contains the SQL type
                     // dw1 contains the column size (precision)
                     // s3 contains the scale (if any)
                     // s4 contains the 'nullable' flag


                     lp3 = GetEnvString(lp1);

                     if(!lp3)   // blank string value
                     {
                        if(*((DWORD FAR *)(lp2 + 2 * sizeof(WORD))) &&
                           *((DWORD FAR *)(lp2 + 2 * sizeof(WORD)))
                           != SQL_NULL_DATA)
                        {
                           bChanged = TRUE; // it changed!

                          *((DWORD FAR *)(lp2 + 2 * sizeof(WORD))) = 0;

                          *(lp2 + 2 * sizeof(WORD) + sizeof(DWORD)) = 0;
                        }
                     }
                     else
                     {
                        w2 = min((WORD)lstrlen(lp3),
                                 *((WORD FAR *)lp2) - sizeof(DWORD)
                                 - sizeof(WORD) - 1);

                        if(*((DWORD FAR *)(lp2 + 2 * sizeof(WORD))) != w2 ||
                           _hmemcmp(lp2 + 2 * sizeof(WORD) + sizeof(DWORD),
                                    lp3, w2))
                        {
                           bChanged = TRUE; // it changed!
                           _hmemcpy(lp2 + 2 * sizeof(WORD) + sizeof(DWORD),
                                    lp3, w2);

                           *((DWORD FAR *)(lp2 + 2 * sizeof(WORD))) = w2;

                           *(lp2 + 2 * sizeof(WORD) + sizeof(DWORD) + w2) = 0;
                        }
                     }

                     if(wOption == CMDODBC_INSERT)       // column names (ALL)
                     {
                        if(w3)  // not the FIRST one...
                        {
                           lstrcat(lpSQL, ",");
                        }

                        w3++;  // update count of fields

                        lstrcat(lpSQL, tbuf2);  // column name
                     }
                     else if(bChanged &&                 // only if it changed
                             wOption == CMDODBC_UPDATE)  // 'SET' expression
                     {
                        if(w3)  // not the FIRST one
                        {
                           lstrcat(lpSQL, ", ");
                        }

                        lstrcat(lpSQL, tbuf2);  // column name
                        lstrcat(lpSQL, "=");

                        // do I need quote marks?

                        if(s2 == SQL_BIT ||
                           s2 == SQL_DECIMAL ||
                           s2 == SQL_DOUBLE ||
                           s2 == SQL_FLOAT ||
                           s2 == SQL_INTEGER ||
                           s2 == SQL_NUMERIC ||
                           s2 == SQL_REAL ||
                           s2 == SQL_SMALLINT ||
                           s2 == SQL_TINYINT)
                        {
                           if(!*(lp2 + 2 * sizeof(WORD) + sizeof(DWORD)))
                           {
                              if(s4 == SQL_NULLABLE)
                              {
                                 lstrcat(lpSQL, "NULL");
                              }
                              else
                              {
                                 lstrcat(lpSQL, "0");  // zero if not NULLABLE
                              }
                           }
                           else
                           {
                              lstrcat(lpSQL, lp2 + 2 * sizeof(WORD) + sizeof(DWORD));
                           }
                        }
                        else
                        {
                           lstrcat(lpSQL, "'");

                           // TODO:  check for embedded quote marks

                           lstrcat(lpSQL, lp2 + 2 * sizeof(WORD) + sizeof(DWORD));

                           lstrcat(lpSQL, "'");
                        }

                        w3++;  // record the fact that it changed!
                     }



                     // END OF LOOP!
                     // point 'lp1' at next entry (or zero byte to mark end)

                     lp1 = lp2 + *((WORD FAR *)lp2) + sizeof(WORD);
                  }
               }

               rval = FALSE;  // a flag...

               switch(wOption)
               {
                  case CMDODBC_UPDATE:  // NOTE:  bound fields only!

                     if(!lpODBCX[w1].lpBindInfo)
                     {
                        PrintErrorMessage(924);  // ODBC ERROR - no bound fields for ADD/UPDATE
                        rval = TRUE;
                        break;
                     }

                     if(!w3)  // no fields have changed
                     {
                        *lpSQL = 0;  // flags it NOT to do anything
                        break;
                     }

                     lstrcat(lpSQL, " WHERE CURRENT OF ");
                     lstrcat(lpSQL, tbuf);  // the cursor name

                     break;


                  case CMDODBC_DELETE:

                     // nothing done here at this time

                     break;


                  case CMDODBC_INSERT:  // this is more difficult...

                     if(!lpODBCX[w1].lpBindInfo)
                     {
                        PrintErrorMessage(924);  // ODBC ERROR - no bound fields for ADD/UPDATE
                        rval = TRUE;
                        break;
                     }

                     lstrcat(lpSQL, ") VALUES (");

                     lp1 = lpODBCX[w1].lpBindInfo;
                     // format of BIND data is:  VARNAME\0{length}{col}{cbValue}DATA

                     w3 = 0;

                     while(*lp1)
                     {
                        lp2 = lp1;   // 'lp1' is the variable name

                        while(*lp2) lp2++;

                        lp2++;       // 'lp2' points to data size (WORD)
                                     // followed by 'cbValue' (DWORD)


                        w2 = *((WORD FAR *)(lp2 + sizeof(WORD)));  // col #

                        *tbuf2 = 0;

                        lpSQLDescribeCol(lpODBCX[w1].hStmt, w2,
                                         (UCHAR FAR *)tbuf2, sizeof(tbuf2),
                                         (SWORD FAR *)&s1, (SWORD FAR *)&s2,
                                         (UDWORD FAR *)&dw1, (SWORD FAR *)&s3,
                                         (SWORD FAR *)&s4);

                        tbuf2[s1] = 0;

                        // tbuf2 contains column name
                        // s2 contains the SQL type
                        // dw1 contains the column size (precision)
                        // s3 contains the scale (if any)
                        // s4 contains the 'nullable' flag


                        lp3 = GetEnvString(lp1);

                        // add appropriate value strings to VALUES list

                        if(w3)
                        {
                           lstrcat(lpSQL, ", ");
                        }

                        w3++;

                        // do I need quote marks?

                        if(s2 == SQL_BIT ||
                           s2 == SQL_DECIMAL ||
                           s2 == SQL_DOUBLE ||
                           s2 == SQL_FLOAT ||
                           s2 == SQL_INTEGER ||
                           s2 == SQL_NUMERIC ||
                           s2 == SQL_REAL ||
                           s2 == SQL_SMALLINT ||
                           s2 == SQL_TINYINT)
                        {
                           if(*((DWORD FAR *)(lp2 + 2 * sizeof(WORD)))
                              == SQL_NULL_DATA ||
                              !*(lp2 + 2 * sizeof(WORD) + sizeof(DWORD)))
                           {
                              if(s4 == SQL_NULLABLE)
                              {
                                 lstrcat(lpSQL, "NULL");
                              }
                              else
                              {
                                 lstrcat(lpSQL, "0");  // zero if not NULLABLE
                              }
                           }
                           else
                           {
                              lstrcat(lpSQL, lp2 + 2 * sizeof(WORD) + sizeof(DWORD));
                           }
                        }
                        else  // things which need quote marks...
                        {
                           if(*((DWORD FAR *)(lp2 + 2 * sizeof(WORD)))
                              == SQL_NULL_DATA
                              && s4 == SQL_NULLABLE)
                           {
                              lstrcat(lpSQL, "NULL");
                           }
                           else
                           {
                              lstrcat(lpSQL, "'");


                              // TODO:  check for embedded quote marks

                              lstrcat(lpSQL, lp2 + 2 * sizeof(WORD) + sizeof(DWORD));

                              lstrcat(lpSQL, "'");
                           }
                        }


                        // END OF LOOP!
                        // point 'lp1' at next entry (or zero byte to mark end)

                        lp1 = lp2 + *((WORD FAR *)lp2) + sizeof(WORD);
                     }

                     lstrcat(lpSQL, ")");

                     break;

               }

               if(!rval && *lpSQL)
               {
                HSTMT hNewStmt;

                  // FINAL STEP:  execute the SQL string in 'lpSQL'
                  // This MUST use a different statement handle


                  retcode = lpSQLAllocStmt(lpODBCX[w1].hDbc, &hNewStmt);

                  if(retcode == SQL_SUCCESS ||
                     retcode == SQL_SUCCESS_WITH_INFO)
                  {
                     do
                     {
                        retcode = lpSQLExecDirect(hNewStmt, (UCHAR FAR *)lpSQL, SQL_NTS);

                        if(retcode == SQL_STILL_EXECUTING)
                        {
                           LoopDispatch();
                        }

                     } while(retcode == SQL_STILL_EXECUTING);


                     // NOW, if there was an error, I won't have the error
                     // message available to me unless I save it here...

                     if(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
                     {
                        l1 = 0;

                        lpSQLError(lpODBCX[w1].hEnv, lpODBCX[w1].hDbc, hNewStmt,
                                   (UCHAR FAR *)pSqlState, (SDWORD FAR *)&l1,
                                   (UCHAR FAR *)tbuf, sizeof(tbuf), 
                                   (SWORD FAR *)&wLen);

                        wsprintf(lpODBCX[w1].szLastError,
                                "%s\t%ld\t%s",
                                (LPSTR)pSqlState, l1, (LPSTR)tbuf);
                     }
                     else
                     {
                        lpODBCX[w1].szLastError[0] = 0;
                     }

                     lpSQLFreeStmt(hNewStmt, SQL_DROP);
                  }

                  rval = retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO;
               }

               GlobalFreePtr(lpSQL);
            }
            else
            {
               dw1 = SQL_CONCUR_READ_ONLY;

               // find out what kind of locking we have available

               lpSQLGetStmtOption(lpODBCX[w1].hStmt, SQL_CONCURRENCY, &dw1);

               if(dw1 == SQL_CONCUR_READ_ONLY)
               {
                  PrintErrorMessage(922);  // ODBC ERROR - 'Read Only' access does not allow INSERT/UPDATE/DELETE

                  rval = TRUE;
                  break;
               }

               // check for BOUND columns at this point, and synchronize the
               // data in the 'bind info' array with the associated variables...

               lp1 = lpODBCX[w1].lpBindInfo;

               if(lp1)
               {
                  // format of BIND data is:  VARNAME\0{length}{col}{cbValue}DATA

                  while(*lp1)
                  {
                     lp2 = lp1;   // 'lp1' is the variable name

                     while(*lp2) lp2++;

                     lp2++;       // 'lp2' points to data size (WORD)
                                  // followed by 'cbValue' (DWORD)


                     lp3 = GetEnvString(lp1);
                     if(!lp3)   // blank string value
                     {
                        *((DWORD FAR *)(lp2 + 2 * sizeof(WORD))) = 0;

                        *(lp2 + 2 * sizeof(WORD) + sizeof(DWORD)) = 0;
                     }
                     else
                     {
                        w2 = min((WORD)lstrlen(lp3),
                                 *((WORD FAR *)lp2) - sizeof(DWORD)
                                 - sizeof(WORD));

                        _hmemcpy(lp2 + 2 * sizeof(WORD) + sizeof(DWORD), lp3, w2);

                        *((DWORD FAR *)(lp2 + 2 * sizeof(WORD))) = w2;
                     }


                     // END OF LOOP!
                     // point 'lp1' at next entry (or zero byte to mark end)

                     lp1 = lp2 + *((WORD FAR *)lp2) + sizeof(WORD);
                  }
               }

               switch(wOption)
               {
                  case CMDODBC_UPDATE:  // NOTE:  bound fields only!

                     retcode = lpSQLSetPos(lpODBCX[w1].hStmt,0,SQL_UPDATE,
                                           SQL_LOCK_NO_CHANGE);
                     break;


                  case CMDODBC_DELETE:

                     retcode = lpSQLSetPos(lpODBCX[w1].hStmt,0,SQL_DELETE,
                                           SQL_LOCK_NO_CHANGE);
                     break;


                  case CMDODBC_INSERT:

                     retcode = lpSQLSetPos(lpODBCX[w1].hStmt,0,SQL_ADD,
                                           SQL_LOCK_NO_CHANGE);
                     break;

               }

               if(retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
               {
                 retcode = lpSQLSetPos(lpODBCX[w1].hStmt,0,SQL_REFRESH,
                                       SQL_LOCK_NO_CHANGE);
               }

            }

            rval = retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO;

         }

         break;



      default:

         PrintString("?ODBC option not (yet) supported\r\n\n");

         rval = TRUE;
         break;
   }



   // if it gets here, use the default settings for 'ODBC_RESULT'

   if(rval)
   {
      SetEnvString(szODBC_RESULT, szERROR);
   }
   else
   {
      SetEnvString(szODBC_RESULT, szOK);
   }


   GlobalFreePtr(lpArgs);

   return(rval);
}


// functions referenced by 'WCALC' go here...

LPCSTR FAR PASCAL CMDOdbcGetConnectString(LPCSTR lpcConvID)
{
WORD w1 = CMDOdbcParseID(&lpcConvID);


   if(!lpODBCX) return(NULL);      // no data

   if(*lpcConvID) return(NULL);    // parameter error...

   return(lpODBCX[w1].szConnect);  // return actual connect string buffer

}



HDBC FAR PASCAL CMDOdbcGetDbc(LPCSTR lpcConvID)
{
WORD w1;


   if(!lpODBCX) return(NULL);      // no data
   if(*lpcConvID) return(NULL);    // parameter error...

   w1 = CMDOdbcParseID(&lpcConvID);

   if(w1 >= wNODBCX) return(NULL);

   return(lpODBCX[w1].hDbc);
}



HSTMT FAR PASCAL CMDOdbcGetStmt(LPCSTR lpcConvID)
{
WORD w1;


   if(!lpODBCX) return(NULL);      // no data
   if(*lpcConvID) return(NULL);    // parameter error...

   w1 = CMDOdbcParseID(&lpcConvID);

   if(w1 >= wNODBCX) return(NULL);

   return(lpODBCX[w1].hStmt);
}



LPSTR FAR PASCAL CMDOdbcGetData(LPCSTR lpcConvID, WORD wColumn)
{
WORD w1, w2, wNCol;
DWORD dw1;
LPSTR lp1, lpRval;
RETCODE retcode;



   if(!lpSQLGetData) return(NULL); // no PROC!
   if(!lpODBCX) return(NULL);      // no data
   if(!*lpcConvID) return(NULL);    // parameter error...

   w1 = CMDOdbcParseID(&lpcConvID);

   if(w1 >= wNODBCX) return(NULL);


   lpRval = (LPSTR)GlobalAllocPtr(GHND, 0x8000);
   if(!lpRval) return(NULL);       // no memory...


   if(!wColumn)        // retrieve ALL un-bound columns
   {
      retcode = lpSQLNumResultCols(lpODBCX[w1].hStmt, (SWORD FAR *)&wNCol);

      *lpRval = 0;
      lp1 = lpRval;

      for(w2=1; w2<=wNCol; w2++)
      {
         if(w2 > 1) *(lp1++) = '\t';

         do
         {
            dw1 = 0x7fff - (lp1 - lpRval);

            retcode = lpSQLGetData(lpODBCX[w1].hStmt, w2, SQL_C_CHAR,
                                   lp1, dw1, (SDWORD FAR *)&dw1);

//            if(dw1 == SQL_NULL_DATA) dw1 = 0;
//            if(dw1 == SQL_NO_TOTAL) dw1 = 0x7fff - OFFSET(lp1);

            if(retcode != SQL_STILL_EXECUTING) break;
            if(LoopDispatch())
            {
               GlobalFreePtr(lpRval);
               return(NULL);
            }

         } while(TRUE);

         if(retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
         {
//            lp1[dw1] = 0;

            lp1 += lstrlen(lp1);
         }
         else
         {
            *lp1 = 0;  // skip the column in error, but leave an empty
                       // array element to indicate a potential problem
         }
      }
   }
   else
   {
      do
      {
         retcode = lpSQLGetData(lpODBCX[w1].hStmt, wColumn, SQL_C_CHAR,
                                lpRval, 0x7fff, (SDWORD FAR *)&dw1);

         if(retcode != SQL_STILL_EXECUTING) break;
         if(LoopDispatch())
         {
            GlobalFreePtr(lpRval);
            return(NULL);
         }

      } while(TRUE);

      if(retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
      {
//         lpRval[dw1] = 0;
      }
      else
      {
         GlobalFreePtr(lpRval);

         lpRval = NULL;  // return zero to indicate error...
      }
   }


   if(lpRval)
   {
      lp1 = (LPSTR)GlobalReAllocPtr(lpRval, lstrlen(lpRval) + 1,
                                    GMEM_MOVEABLE);

      if(lp1) lpRval = lp1;
   }

   return(lpRval);

}



LPSTR FAR PASCAL CMDOdbcGetColumnIndex(LPCSTR lpcConvID, LPCSTR lpName)
{
WORD w1, w2;
LPSTR lpRval;


   // returns blank string if column name not found, or the index
   // of the column within the result set


   if(!lpSQLGetData) return(NULL); // no PROC!
   if(!lpODBCX) return(NULL);      // no data
   if(!*lpcConvID) return(NULL);    // parameter error...

   w1 = CMDOdbcParseID(&lpcConvID);

   if(w1 >= wNODBCX) return(NULL);

   lpRval = (LPSTR)GlobalAllocPtr(GHND, 16);

   w2 = CMDOdbcColumnNameIndex(lpODBCX + w1, lpName);

   if(w2 == 0xffff) return(NULL);
   else if(!w2)     *lpRval = 0;   // return blank string
   else             wsprintf(lpRval, "%u", w2);

   return(lpRval);

}



LPSTR FAR PASCAL CMDOdbcGetColumnInfo(LPCSTR lpcConvID, WORD wColumnIndex)
{
WORD w0, w1, w2, wNCol, wLen;
DWORD dw1;
LPSTR lp1, lpRval;
RETCODE retcode;
static const WORD pDT[]={
      SQL_COLUMN_NAME, SQL_COLUMN_LABEL, SQL_COLUMN_TYPE_NAME,
      SQL_COLUMN_LENGTH, SQL_COLUMN_PRECISION, SQL_COLUMN_SCALE,
      SQL_COLUMN_DISPLAY_SIZE, SQL_COLUMN_TYPE };        // enough for now...
static const BYTE pTP[]={  // 0 for CHAR, 1 for DWORD
      0,0,0,1,1,1,1,1 };




   if(!lpSQLGetData) return(NULL); // no PROC!
   if(!lpODBCX) return(NULL);      // no data
   if(!*lpcConvID) return(NULL);    // parameter error...

   w0 = CMDOdbcParseID(&lpcConvID);

   if(w0 >= wNODBCX) return(NULL);


   lpRval = (LPSTR)GlobalAllocPtr(GHND, 0x8000);
   if(!lpRval) return(NULL);       // no memory...


   retcode = lpSQLNumResultCols(lpODBCX[w0].hStmt, (SWORD FAR *)&wNCol);
   if(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
   {
      GlobalFreePtr(lpRval);
      return(NULL);
   }

   if(wColumnIndex > wNCol)
   {
      GlobalFreePtr(lpRval);

      lpRval = (LPSTR)GlobalAllocPtr(GHND, 16);
      if(lpRval) *lpRval = 0;

      return(lpRval);  // illegal column index (but return a blank string)
                       // NOTE:  this is *NOT* an error, but a warning!
   }


   *lpRval = 0;
   lp1 = lpRval;

   for(w1=1; w1<=wNCol; w1++)
   {
      if(wColumnIndex && wColumnIndex != w1) continue;

      for(w2=0; w2<N_DIMENSIONS(pDT); w2++)
      {
         if(w2 > 0) *(lp1++) = '\t';

         do
         {
            dw1 = 0;
            wLen = 0x7fff - (lp1 - lpRval);

            retcode = lpSQLColAttributes(lpODBCX[w0].hStmt, w1,
                                         pDT[w2], lp1, wLen,
                                         (SWORD FAR *)&wLen, (SDWORD FAR *)&dw1);

            if(retcode != SQL_STILL_EXECUTING) break;
            if(LoopDispatch())
            {
               GlobalFreePtr(lpRval);
               return(NULL);
            }

         } while(TRUE);


         if(retcode == SQL_SUCCESS || retcode == SQL_SUCCESS_WITH_INFO)
         {
            if(!pTP[w2])
            {
//               lp1[wLen] = 0;
            }
            else
            {
               wsprintf(lp1, "%ld", dw1);  // use THIS instead...
            }

            lp1 += lstrlen(lp1);
         }
         else
         {
            *lp1 = 0;  // skip the column in error, but leave an empty
                       // array element to indicate a potential problem
         }
      }

      *(lp1++) = '\r';
      *(lp1++) = '\n';

      *lp1 = 0;
   }



   lp1 = (LPSTR)GlobalReAllocPtr(lpRval, lstrlen(lpRval) + 1,
                                 GMEM_MOVEABLE);
   if(lp1) lpRval = lp1;

   return(lpRval);

}



LPSTR FAR PASCAL CMDOdbcGetCursorName(LPCSTR lpcConvID)
{
WORD w1;
LPSTR lpRval;
RETCODE retcode;
SWORD cbCursor;
char tbuf[256];


   if(!lpODBCX) return(NULL);      // no data
   if(*lpcConvID) return(NULL);    // parameter error...

   w1 = CMDOdbcParseID(&lpcConvID);

   if(w1 >= wNODBCX) return(NULL);

   retcode = lpSQLGetCursorName(lpODBCX[w1].hStmt,
                                (UCHAR FAR *)tbuf, sizeof(tbuf),
                                (SWORD FAR *)&cbCursor);

   if(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
   {
      return(NULL);
   }

   tbuf[cbCursor] = 0;  // ensure it's null terminated

   lpRval = (LPSTR)GlobalAllocPtr(GPTR, lstrlen(tbuf) + 1);

   if(lpRval)
   {
      lstrcpy(lpRval, tbuf);
   }

   return(lpRval);

}



LPSTR FAR PASCAL CMDOdbcGetInfo(LPCSTR lpcConvID, WORD wIndex)
{
char tbuf[256];
WORD w1, w2, wLen;
DWORD dw1;
RETCODE retcode;
LPSTR lp1, lpRval;



   if(!lpSQLGetInfo) return(NULL);
   if(!lpODBCX)   return(NULL);
   if(!*lpcConvID) return(NULL);

   w1 = CMDOdbcParseID(&lpcConvID);

   if(w1 >= wNODBCX) return(NULL);

   // values that return integer must be converted...

   lpRval = NULL;  // a flag...

   switch(wIndex)
   {
      // items that return a short or unsigned short...

      case SQL_ACTIVE_CONNECTIONS:
      case SQL_ACTIVE_STATEMENTS:
      case SQL_CONCAT_NULL_BEHAVIOR:
      case SQL_CORRELATION_NAME:
      case SQL_CURSOR_COMMIT_BEHAVIOR:
      case SQL_CURSOR_ROLLBACK_BEHAVIOR:
      case SQL_FILE_USAGE:
      case SQL_GROUP_BY:
      case SQL_IDENTIFIER_CASE:
      case SQL_MAX_COLUMN_NAME_LEN:
      case SQL_MAX_COLUMNS_IN_GROUP_BY:
      case SQL_MAX_COLUMNS_IN_INDEX:
      case SQL_MAX_COLUMNS_IN_ORDER_BY:
      case SQL_MAX_COLUMNS_IN_SELECT:
      case SQL_MAX_COLUMNS_IN_TABLE:
      case SQL_MAX_CURSOR_NAME_LEN:
      case SQL_MAX_OWNER_NAME_LEN:
      case SQL_MAX_PROCEDURE_NAME_LEN:
      case SQL_MAX_QUALIFIER_NAME_LEN:
      case SQL_MAX_TABLE_NAME_LEN:
      case SQL_MAX_TABLES_IN_SELECT:
      case SQL_MAX_USER_NAME_LEN:
      case SQL_NON_NULLABLE_COLUMNS:
      case SQL_NULL_COLLATION:
      case SQL_ODBC_API_CONFORMANCE:
      case SQL_ODBC_SAG_CLI_CONFORMANCE:
      case SQL_ODBC_SQL_CONFORMANCE:
      case SQL_QUALIFIER_LOCATION:
      case SQL_QUOTED_IDENTIFIER_CASE:
      case SQL_TXN_CAPABLE:

         retcode = lpSQLGetInfo(lpODBCX[w1].hDbc, wIndex, &w2,
                                sizeof(w2), (SWORD FAR *)&wLen);

         if(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
         {
            return(NULL);
         }

         wsprintf(tbuf, "%u", w2);
         break;



      // items that return a long or unsigned long...

      case SQL_ALTER_TABLE:
      case SQL_BOOKMARK_PERSISTENCE:
      case SQL_CONVERT_BIGINT:
      case SQL_CONVERT_BINARY:
      case SQL_CONVERT_BIT:
      case SQL_CONVERT_CHAR:
      case SQL_CONVERT_DATE:
      case SQL_CONVERT_DECIMAL:
      case SQL_CONVERT_DOUBLE:
      case SQL_CONVERT_FLOAT:
      case SQL_CONVERT_FUNCTIONS:
      case SQL_CONVERT_INTEGER:
      case SQL_CONVERT_LONGVARBINARY:
      case SQL_CONVERT_LONGVARCHAR:
      case SQL_CONVERT_NUMERIC:
      case SQL_CONVERT_REAL:
      case SQL_CONVERT_SMALLINT:
      case SQL_CONVERT_TIME:
      case SQL_CONVERT_TIMESTAMP:
      case SQL_CONVERT_TINYINT:
      case SQL_CONVERT_VARBINARY:
      case SQL_CONVERT_VARCHAR:
      case SQL_DEFAULT_TXN_ISOLATION:
      case SQL_DRIVER_HDBC:
      case SQL_DRIVER_HENV:
      case SQL_DRIVER_HLIB:
      case SQL_DRIVER_HSTMT:
      case SQL_FETCH_DIRECTION:
      case SQL_GETDATA_EXTENSIONS:
      case SQL_LOCK_TYPES:
      case SQL_MAX_BINARY_LITERAL_LEN:
      case SQL_MAX_CHAR_LITERAL_LEN:
      case SQL_MAX_INDEX_SIZE:
      case SQL_MAX_ROW_SIZE:
      case SQL_MAX_STATEMENT_LEN:
      case SQL_NUMERIC_FUNCTIONS:
      case SQL_OWNER_USAGE:
      case SQL_POS_OPERATIONS:
      case SQL_POSITIONED_STATEMENTS:
      case SQL_QUALIFIER_USAGE:
      case SQL_SCROLL_CONCURRENCY:
      case SQL_SCROLL_OPTIONS:
      case SQL_STATIC_SENSITIVITY:
      case SQL_STRING_FUNCTIONS:
      case SQL_SUBQUERIES:
      case SQL_SYSTEM_FUNCTIONS:
      case SQL_TIMEDATE_ADD_INTERVALS:
      case SQL_TIMEDATE_DIFF_INTERVALS:
      case SQL_TIMEDATE_FUNCTIONS:
      case SQL_TXN_ISOLATION_OPTION:
      case SQL_UNION:

         if(wIndex == SQL_DRIVER_HSTMT)
         {
            // special case - must pass current 'hStmt' on input

            dw1 = (DWORD)lpODBCX[w1].hStmt;
         }

         retcode = lpSQLGetInfo(lpODBCX[w1].hDbc, wIndex, &dw1,
                                sizeof(dw1), (SWORD FAR *)&wLen);

         if(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
         {
            return(NULL);
         }

         wsprintf(tbuf, "%lu", *((unsigned long FAR *)tbuf));
         break;



      // items that return a character buffer (everything else)

      case SQL_ACCESSIBLE_TABLES:
      case SQL_ACCESSIBLE_PROCEDURES:
      case SQL_COLUMN_ALIAS:
      case SQL_DATA_SOURCE_NAME:
      case SQL_DATA_SOURCE_READ_ONLY:
      case SQL_DBMS_NAME:
      case SQL_DBMS_VER:
      case SQL_DRIVER_NAME:
      case SQL_DRIVER_ODBC_VER:
      case SQL_DRIVER_VER:
      case SQL_EXPRESSIONS_IN_ORDERBY:
      case SQL_IDENTIFIER_QUOTE_CHAR:
      case SQL_KEYWORDS:
      case SQL_LIKE_ESCAPE_CLAUSE:
      case SQL_MAX_ROW_SIZE_INCLUDES_LONG:
      case SQL_MULT_RESULT_SETS:
      case SQL_MULTIPLE_ACTIVE_TXN:
      case SQL_NEED_LONG_DATA_LEN:
      case SQL_ODBC_SQL_OPT_IEF:
      case SQL_ODBC_VER:
      case SQL_ORDER_BY_COLUMNS_IN_SELECT:
      case SQL_OUTER_JOINS:
      case SQL_OWNER_TERM:
      case SQL_PROCEDURE_TERM:
      case SQL_PROCEDURES:
      case SQL_QUALIFIER_NAME_SEPARATOR:
      case SQL_QUALIFIER_TERM:
      case SQL_ROW_UPDATES:
      case SQL_SEARCH_PATTERN_ESCAPE:
      case SQL_SERVER_NAME:
      case SQL_SPECIAL_CHARACTERS:
      case SQL_TABLE_TERM:
      case SQL_USER_NAME:
      default:

         lpRval = (LPSTR)GlobalAllocPtr(GHND, 0x8000);

         if(!lpRval)
         {
            wLen = sizeof(tbuf)-1;

            retcode = lpSQLGetInfo(lpODBCX[w1].hDbc, wIndex, tbuf,
                                   sizeof(tbuf), (SWORD FAR *)&wLen);
         }
         else
         {
            wLen = 0x7fff;

            retcode = lpSQLGetInfo(lpODBCX[w1].hDbc, wIndex, lpRval,
                                   0x7fff, (SWORD FAR *)&wLen);
         }

         if(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
         {
            if(lpRval) GlobalFreePtr(lpRval);

            return(NULL);
         }

         if(lpRval)
         {
            lpRval[wLen] = 0;

            lp1 = (LPSTR)GlobalReAllocPtr(lpRval, lstrlen(lpRval) + 1,
                                          GMEM_MOVEABLE);

            if(lp1) lpRval = lp1;
         }
         else
         {
            tbuf[wLen] = 0;
         }

   }


   // make a copy of 'tbuf' into a globally alloc'ed buffer, and return it

   if(!lpRval)
   {
      lpRval = (LPSTR)GlobalAllocPtr(GHND, lstrlen(tbuf) + 1);
      if(lpRval) lstrcpy(lpRval, tbuf);
   }

   return(lpRval);

}



LPSTR FAR PASCAL CMDOdbcGetTables(LPCSTR lpcConvID)
{
WORD w1, w2;
DWORD dw1;
HSTMT hStmt;
LPSTR lpRval, lp1;
HCURSOR hOldCursor;
RETCODE retcode;



   if(!lpSQLTables) return(NULL);
   if(!lpODBCX)   return(NULL);
   if(!*lpcConvID) return(NULL);

   w1 = CMDOdbcParseID(&lpcConvID);

   if(w1 >= wNODBCX) return(NULL);


   lpRval = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT, 0x8000);
   if(!lpRval) return(NULL);


   if(lpSQLAllocStmt(lpODBCX[w1].hDbc, &hStmt) != SQL_SUCCESS)
   {
      goto bailout_point;
   }

   retcode = lpSQLTables(hStmt, (UCHAR FAR *)"", 0, (UCHAR FAR *)"", 0,
                         (UCHAR FAR *)"", 0, (UCHAR FAR *)"", 0);

   if(retcode != SQL_SUCCESS)
   {
      goto bailout_point;
   }




   // step through the table, one record at a time, and assign the results
   // to the output buffer as an array.  Assume it won't go over 32k...


   hOldCursor = MySetCursor(LoadCursor(NULL, IDC_WAIT));

   // get count of result columns first...

   while((retcode = lpSQLNumResultCols(hStmt, (SWORD FAR *)&w1)) == SQL_STILL_EXECUTING)
   {
      MthreadSleep(50);

      if(LoopDispatch())
      {
         MySetCursor(hOldCursor);
         goto bailout_point;
      }
   }

   if(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
   {
      MySetCursor(hOldCursor);
      goto bailout_point;
   }


   *lpRval = 0;
   lp1 = lpRval;

   while(TRUE)
   {
      while((retcode = lpSQLFetch(hStmt)) == SQL_STILL_EXECUTING)
      {
         MthreadSleep(50);

         if(LoopDispatch())
         {
            MySetCursor(hOldCursor);
            goto bailout_point;
         }
      }

      if(retcode == SQL_NO_DATA_FOUND)  break;  // end of loop


      if(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
      {

         // a *REAL* error!  bail out, now

         MySetCursor(hOldCursor);
         goto bailout_point;
      }

      // get each column in the result set, and place the text into
      // the array (tab delimited columns).

      for(w2=1; w2<w1; w2++)
      {
         if(w2 > 1) *(lp1++) = '\t';

         retcode = lpSQLGetData(hStmt, w2, SQL_C_CHAR, lp1,
                                0x7fff - OFFSETOF(lp1), (SDWORD FAR *)&dw1);

         if(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
         {
            break;  // just break out on this...
         }

         if(dw1==SQL_NULL_DATA)
         {
            *lp1 = 0;
         }
         else
         {
            lp1 += lstrlen(lp1);
         }
      }

      *(lp1++) = '\r';
      *(lp1++) = '\n';
      *lp1 = 0;

      LoopDispatch();
   }

   MySetCursor(hOldCursor);

   lpSQLFreeStmt(hStmt, SQL_DROP);
   hStmt = NULL;


   lp1 = (LPSTR)GlobalReAllocPtr(lpRval, lstrlen(lpRval) + 1, GMEM_MOVEABLE);

   if(lp1) lpRval = lp1;
   return(lpRval);



   // error bailout point...

bailout_point:

   if(hStmt)  lpSQLFreeStmt(hStmt, SQL_DROP);

   if(lpRval) GlobalFreePtr(lpRval);

   return(NULL);

}



LPSTR FAR PASCAL CMDOdbcGetDrivers(void)
{
char tbuf1[256], tbuf2[512];
HENV hEnv;
WORD wLen1, wLen2;
RETCODE retcode;
LPSTR lpRval, lp1, lp2;



   if(!lpSQLDrivers) return(NULL);

   if(lpSQLAllocEnv(&hEnv) != SQL_SUCCESS) return(NULL);

   lpRval = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT, 0x8000);

   if(!lpRval)
   {
      lpSQLFreeEnv(hEnv);
      return(NULL);
   }


   *lpRval = 0;
   lp1 = lpRval;


   while(TRUE)
   {
      retcode = lpSQLDrivers(hEnv,
                             (lp1 == lpRval)?SQL_FETCH_FIRST:SQL_FETCH_NEXT,
                             (UCHAR FAR *)tbuf1, sizeof(tbuf1), (SWORD FAR *)&wLen1,
                             (UCHAR FAR *)tbuf2, sizeof(tbuf2) - 1, (SWORD FAR *)&wLen2);

      if(retcode != SQL_SUCCESS && retcode != SQL_SUCCESS_WITH_INFO)
      {
         if(retcode == SQL_NO_DATA_FOUND) break;

         GlobalFreePtr(lpRval);
         lpSQLFreeEnv(hEnv);

         return(NULL);
      }

      tbuf1[wLen1] = 0;
      tbuf2[wLen2] = 0;

      // prepare 'tbuf2'

      lp2 = (LPSTR)tbuf2;

      while(*lp2)
      {
         lp2 += lstrlen(lp2);
         if(lp2[1]) *lp2 = '\t';

         lp2++;
      }

      lstrcpy(lp1, tbuf1);
      lp1 += lstrlen(lp1);

      *(lp1++) = '\t';
      lstrcpy(lp1, tbuf2);
      lp1 += lstrlen(lp1);

      *(lp1++) = '\r';
      *(lp1++) = '\n';
      *lp1 = 0;
   }


   lpSQLFreeEnv(hEnv);

   lp1 = (LPSTR)GlobalReAllocPtr(lpRval, lstrlen(lpRval) + 1,
                                 GMEM_MOVEABLE);

   if(lp1) lpRval = lp1;

   return(lpRval);

}



/***************************************************************************/
/*        'GOTO', 'RETURN', 'CONTINUE', and similar processes done here    */
/***************************************************************************/


#pragma code_seg("CMDGoto_TEXT","CODE")

BOOL FAR PASCAL CMDGoto    (LPSTR lpCmd, LPSTR lpArgs)
{
BOOL rval;
LPSTR lp1;


   if(!BatchMode)
   {
      PrintErrorMessage(596);
      return(TRUE);
   }


   if(!lpBatchInfo)
   {
      PrintErrorMessage(597);
      BatchMode=FALSE;

      return(TRUE);
   }


   lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   lp1 = lpArgs + lstrlen(lpArgs);

   while(lp1 > lpArgs && *(lp1 - 1)<=' ') *(--lp1) = 0;


   if(!*lpArgs || (rval = GotoBatchLabel(lpBatchInfo, lpArgs)))
   {
      PrintErrorMessage(598);
      PrintString(lpArgs);
      PrintErrorMessage(599);

      TerminateCurrentBatchFile();
   }

   GlobalFreePtr(lpArgs);
   return(rval);

}


BOOL FAR PASCAL CMDReturn  (LPSTR lpCmd, LPSTR lpArgs)
{
LPBLOCK_INFO lpBI;
LPSTR lp1;
WORD w1, w2;


   if(!BatchMode || !lpBatchInfo)
   {
      PrintString("?Must be in BATCH mode to use RETURN\r\n\n");
      return(TRUE);
   }

   lpBI = lpBatchInfo->lpBlockInfo;


   if(lpArgs)
   {
      lpArgs = EvalString(lpArgs); /* evaluate any substitutions */

      if(!lpArgs || !(lp1 = Calculate(lpArgs)))
      {
         PrintString("?Warning - not enough memory to RETURN value\r\n");
         if(lpArgs) GlobalFreePtr(lpArgs);
      }
      else
      {
         GlobalFreePtr(lpArgs);

         SetEnvString(szRETVAL, lp1);

         GlobalFreePtr(lp1);
      }
   }

   // at this point, see if we're in 'CALL' mode, and if so just go
   // back up the call stack.  Otherwise, we'll have to terminate the
   // current batch file.

   if(lpBI && lpBI->dwEntryCount)
   {
      for(w1=(WORD)(lpBI->dwEntryCount - 1); (int)w1 >= 0; w1--)
      {
         if((lpBI->lpCodeBlock[w1].dwType & CODEBLOCK_TYPEMASK)
             == CODEBLOCK_CALL &&
            *((HANDLE FAR *)lpBI->lpCodeBlock[w1].szInfo)
             == MthreadGetCurrentThread())
         {
            // clean out ALL entries below this one.  No longer needed

            for(w2 = (WORD)lpBI->dwEntryCount; w2 > w1; w2--)
            {
               RemoveCodeBlockEntry(lpBI->lpCodeBlock + w2);
            }

            // this re-assigns the correct batch file position...

            SetBatchFilePosition(lpBatchInfo,
                                 lpBI->lpCodeBlock[w1].dwStart);


            // remove entry from 'lpCodeBlock' array and update counter
            // (this will act as a flag to any process waiting for return)

            RemoveCodeBlockEntry(lpBI->lpCodeBlock + w1);
            lpBI->dwEntryCount = w1;  // remove entry and flag 'RETURN'

            return(FALSE);
         }
      }
   }

   // we're not in an 'internal CALL', so bail out!

   TerminateCurrentBatchFile();

   return(FALSE);

}



#pragma code_seg("CMDContinue_TEXT","CODE")

BOOL FAR PASCAL CMDContinue(LPSTR lpCmd, LPSTR lpArgs)
{
LPBLOCK_INFO lpBI;
WORD w1, w2;
LPSTR lp1, lp2, lp3;


   if(!BatchMode || !lpBatchInfo)
   {
      PrintErrorMessage(578); // NOT VALID IN COMMAND MODE

      return(TRUE);
   }

   if(!lpBatchInfo->lpBlockInfo)
   {
      PrintErrorMessage(895); // "?EXIT LOOP/CONTINUE not valid outside of loop\r\n\n"

      return(TRUE);
   }

   if(lpArgs)
   {
      lpArgs = EvalString(lpArgs); /* evaluate any substitutions */

      for(lp1=lpArgs; *lp1 && *lp1 <=' '; lp1++)
         ; // find argument (if any) past white space

      if(*lp1) // argument on line - whoops!
      {
         PrintString(pSYNTAXERROR);
         PrintString(lpArgs);
         PrintString(pQCRLFLF);

         GlobalFreePtr(lpArgs);
         return(TRUE);
      }

      GlobalFreePtr(lpArgs);
      lpArgs = NULL;
   }


   // find the END of the loop...

   lpBI = lpBatchInfo->lpBlockInfo;

   for(w1 = (WORD)lpBI->dwEntryCount; w1 > 0;)
   {
      w1--;  // pre-decrement in this case

      // We are looking for the last WHILE/REPEAT/FOR command that we find.

      if((lpBI->lpCodeBlock[w1].dwType & CODEBLOCK_TYPEMASK)
          == CODEBLOCK_WHILE ||
         (lpBI->lpCodeBlock[w1].dwType & CODEBLOCK_TYPEMASK)
          == CODEBLOCK_REPEAT ||
         (lpBI->lpCodeBlock[w1].dwType & CODEBLOCK_TYPEMASK)
          == CODEBLOCK_FOR)
      {
         // jump to the end of the loop

         if(FindNextMatchingSection(lpBI->lpCodeBlock + w1))
         {
            // print an appropriate error message

            if((lpBI->lpCodeBlock[w1].dwType & CODEBLOCK_TYPEMASK)
               == CODEBLOCK_WHILE)
            {
               PrintErrorMessage(894); // "?Missing END WHILE/WEND in WHILE loop\r\n\n"
            }
            else if((lpBI->lpCodeBlock[w1].dwType & CODEBLOCK_TYPEMASK)
                    == CODEBLOCK_REPEAT)
            {
               PrintErrorMessage(912); // "?Missing UNTIL in REPEAT/UNTIL loop\r\n\n"
            }
            else  // assume it's a 'FOR/NEXT' loop
            {
               PrintErrorMessage(913); // "?Missing NEXT in FOR/NEXT loop\r\n\n"
            }

            TerminateCurrentBatchFile();
            return(TRUE);
         }

         // the next command that will execute will be the 'NEXT/UNTIL/WEND'
         // which will automatically take care of nested loops, etc.

         return(FALSE);
      }
      else if((lpBI->lpCodeBlock[w1].dwType & CODEBLOCK_TYPEMASK)
               == CODEBLOCK_IF)
      {
         // when I find a nested 'IF' block I must jump to the 'ENDIF'
         // or else I'll have one TERRIFIC problem...!

         do
         {
            if(FindNextMatchingSection(lpBI->lpCodeBlock + w1))
            {
               PrintErrorMessage(915); // "?Missing ENDIF in IF/ELSE/ENDIF\r\n\n"

               TerminateCurrentBatchFile();

               return(TRUE);
            }

            lp1 = GetAnotherBatchLine(lpBatchInfo);  // get the line...

            if(!lp1)
            {
               PrintErrorMessage(915); // "?Missing ENDIF in IF/ELSE/ENDIF\r\n\n"

               TerminateCurrentBatchFile();

               return(TRUE);
            }

            for(lp2=lp1; *lp2 && *lp2<=' '; lp2++)
               ;  // trim lead white space

            for(lp3=lp2; *lp3 > ' '; lp3++)
               ;  // find next non-white-space

            w2 = lp3 - lp2;

            if(w2==3 && !_fstrnicmp(lp2, "END", w2))
            {
               for(lp2=lp3; *lp2 && *lp2<=' '; lp2++)
                  ;  // trim lead white space

               for(lp3=lp2; *lp3 > ' '; lp3++)
                  ;  // find next non-white-space

               w2 = lp3 - lp2;

               if(w2==2 && !_fstrnicmp(lp2, "IF", w2))
               {
                  GlobalFreePtr(lp1);
                  break;  // an ENDIF was found!!
               }
            }
            else if(w2==5 && !_fstrnicmp(lp2, "ENDIF", w2))
            {
               GlobalFreePtr(lp1);
               break;
            }

            GlobalFreePtr(lp1);

         } while(TRUE);

      }
   }

   PrintErrorMessage(895); // "?EXIT LOOP/CONTINUE not valid outside of loop\r\n\n"

   return(TRUE);
}


BOOL FAR PASCAL FindNextMatchingSection(CODE_BLOCK FAR *lpCB)
{
DWORD dwBatchPointer;
WORD wType, wMiniSP, w1;
LPSTR lp1, lp2, lp3;
DWORD lpdwMiniStack[128]; // this should *NEVER* happen...


   // Find next matching section for code block.  Depends on block type.
   // Assume 'lpBatchInfo' points to the correct batch information block.
   //
   // IF:     finds 'ELSE', 'ELSE IF', or 'ENDIF/END IF'
   // FOR:    finds corresponding 'NEXT'
   // REPEAT: finds 'UNTIL'
   // WHILE:  finds 'WEND' or 'END WHILE'
   //
   // This function properly handles *ALL* nesting conditions.  An error
   // will occur if the end of a loop is found within a nested conditional
   // block, since not all control paths will reach it.  For LOOP commands
   // the 'dwBlockEnd' field will be assigned a value equal to the pointer
   // to the 'NEXT/WEND/UNTIL' command (as appropriate), for use by the
   // 'EXIT LOOP' command.
   //
   // On exit, the return value is TRUE if the matching command was NOT
   // found, and FALSE if it was.  The batch file pointer will point to
   // the line which contains the matching command.
   //
   // NOTE:  Code blocks below the current one are NOT disposed of!



   wType = (WORD)(lpCB->dwType & CODEBLOCK_TYPEMASK);


   // see if I can use 'dwBlockEnd' for this entry type

   if(wType == CODEBLOCK_FOR         // a 'FOR/NEXT' loop
      || wType == CODEBLOCK_REPEAT   // a 'REPEAT/UNTIL' loop
      || wType == CODEBLOCK_WHILE)   // a 'WHILE/WEND' loop
   {
      if(lpCB->dwBlockEnd != (DWORD)-1L)  // a valid 'block end' value
      {
         if(SetBatchFilePosition(lpBatchInfo,lpCB->dwBlockEnd))
         {
            return(TRUE);  // error!
         }

         // one important detail:  I must re-read the PREVIOUS line, which
         // means I must back up the pointer by one line.  'dwBlockEnd'
         // points to the statement AFTER the block ends...

         return(PointToPreviousBatchLine(lpBatchInfo));
      }
   }



   // pre-assign 'dwBatchPointer' BEFORE reading another batch line, so
   // that I can go BACK to this position easily!
   // NOTE:  the 'messy' for command is done this way to ensure that
   // 'lp1' gets freed once per iteration, and that 'dwBatchPointer'
   // is also assigned.  It gives me the freedom to use 'continue'...

   for(wMiniSP = 0,
       dwBatchPointer = lpBatchInfo->buf_start + lpBatchInfo->buf_pos;
       (lp1 = GetAnotherBatchLine(lpBatchInfo)) != NULL;
       GlobalFreePtr(lp1),
       dwBatchPointer = lpBatchInfo->buf_start + lpBatchInfo->buf_pos)
   {
      if(wMiniSP >= N_DIMENSIONS(lpdwMiniStack))
      {
         PrintErrorMessage(907);  // "?Too many nested IF/FOR/UNTIL/WHILE commands\r\n\n"
         break;
      }

      for(lp2 = lp1; *lp2 && *lp2 <= ' '; lp2++)
         ;                                       // find non-white-space

      if(!*lp2) continue;                        // ignore blank line
      if(*lp2 == ';' || *lp2 == '?') continue;   // ignore comment and '?'

      for(lp3 = lp2; *lp3 > ' '; lp3++)
         ;                                       // find white space


      w1 = lp3 - lp2;

      if(w1==2 && !_fstrnicmp(lp2, "IF", w1))
      {
         // see what form this *IF* command is in... and should we concern
         // ourselves with another NESTING level?
         //
         // IF [NOT] [ERRORLEVEL n|string1==string2|(condition)] [{command}|THEN]
         // only the '[NOT] (condition) [DO {command}] version matters here

         for(lp2 = lp3; *lp2 && *lp2 <= ' '; lp2++)
            ;  // next non-white-space

         if(*lp2 != '(')  // not a parenthesis
         {
            for(lp3 = lp2; *lp3 > ' '; lp3++)
               ;  // next white-space

            w1 = lp3 - lp2;

            if(w1 != 3 || _fstrnicmp(lp2, "NOT", w1))
            {
               continue;  // no 'NOT' found, so ignore this one
            }

            for(lp2 = lp3; *lp2 && *lp2 <= ' '; lp2++)
               ;  // next non-white-space
         }

         if(*lp2 != '(') continue;  // ignore other types of 'IF' statements

         // find the matching parenthesis now...

         lp2 = GetEndOfTerm(lp2);

         if(!lp2)  // there was a syntax error in the IF statement
         {
            PrintErrorMessage(603); // "?Syntax error in 'IF' statement...\r\n"

            break;
         }

         while(*lp2 && *lp2 <= ' ') lp2++;

         if(*lp2)  // there is something else on the command line...
         {         // check for an optional 'THEN' command

            for(lp3 = lp2; *lp3 > ' '; lp3++)
               ;  // next white-space

            w1 = lp3 - lp2;

            if(w1 != 4 || _fstrnicmp(lp2, "THEN", w1))
            {
               continue;  // no 'THEN' found - it's a normal 'IF' command
            }
         }

         // at THIS point we're nesting an IF statement, so place an
         // appropriate entry on the 'mini stack' for it.

         lpdwMiniStack[wMiniSP++] = CODEBLOCK_IF;

      }
      else if(w1==4 && !_fstrnicmp(lp2, "ELSE", w1))
      {
         for(lp2 = lp3; *lp2 && *lp2 <= ' '; lp2++)
            ;  // next non-white-space
         for(lp3 = lp2; *lp3 > ' '; lp3++)
            ;  // next white-space

         w1 = lp3 - lp2;

         if(w1 == 2 && !_fstrnicmp(lp2, "IF", w1))
         {
            goto do_elseif;  // process the same as an 'ELSEIF'
         }

         if(*lp2)     // this is a SYNTAX ERROR, actually!
         {
            // for now, ignore it.
         }

         if(!wMiniSP)
         {
            if(wType == CODEBLOCK_IF)  // this *IS* an IF block!
            {
               GlobalFreePtr(lp1);

               return(SetBatchFilePosition(lpBatchInfo,dwBatchPointer));
            }

            PrintErrorMessage(902); // "?Improperly nested IF/ELSE/ENDIF block\r\n\n"

            break;       // otherwise, it's an ERROR!
         }
      }
      else if(w1==6 && !_fstrnicmp(lp2, "ELSEIF", w1))
      {

do_elseif:
         if(!wMiniSP)
         {
            if(wType == CODEBLOCK_IF)  // this *IS* an IF block!
            {
               GlobalFreePtr(lp1);

               return(SetBatchFilePosition(lpBatchInfo,dwBatchPointer));
            }

            PrintErrorMessage(902); // "?Improperly nested IF/ELSE/ENDIF block\r\n\n"

            break;       // otherwise, it's an ERROR!
         }
      }
      else if(w1==3 && !_fstrnicmp(lp2, "END", w1))
      {
        // this can either be 'END IF' or 'END WHILE' to be useful here

         for(lp2 = lp3; *lp2 && *lp2 <= ' '; lp2++)
            ;  // next non-white-space
         for(lp3 = lp2; *lp3 > ' '; lp3++)
            ;  // next white-space

         w1 = lp3 - lp2;

         if(w1 == 2 && !_fstrnicmp(lp2, "IF", w1))
         {
            goto do_endif;  // process the same as an 'ENDIF'
         }
         else if(w1 == 5 && !_fstrnicmp(lp2, "WHILE", w1))
         {
            goto do_wend;   // process the same as a 'WEND'
         }
         else if(w1 == 6 && !_fstrnicmp(lp2, "DEFINE", w1))
         {
            PrintErrorMessage(903); // "?Misplaced 'END DEFINE' command inside nested block\r\n\n"
            break;
         }

         // all other 'END' commands here are ignored
      }
      else if(w1==5 && !_fstrnicmp(lp2, "ENDIF", w1))
      {

do_endif:
         if(!wMiniSP)
         {
            if(wType == CODEBLOCK_IF)  // this *IS* an IF block!
            {
               GlobalFreePtr(lp1);

               return(SetBatchFilePosition(lpBatchInfo,dwBatchPointer));
            }

            PrintErrorMessage(902); // "?Improperly nested IF/ELSE/ENDIF block\r\n\n"

            break;       // otherwise, it's an ERROR!
         }

         if(lpdwMiniStack[wMiniSP - 1] != CODEBLOCK_IF)  // nesting error
         {
            PrintErrorMessage(902); // "?Improperly nested IF/ELSE/ENDIF block\r\n\n"

            break;       // otherwise, it's an ERROR!
         }

         lpdwMiniStack[--wMiniSP] = 0;
      }
      else if(w1==3 && !_fstrnicmp(lp2, "FOR", w1))
      {
         // verify the current format is as follows:
         //
         // FOR [%]var FROM 'x' TO 'y' [STEP 'z'] [DO {command}]
         //

         for(lp2 = lp3; *lp2 && *lp2 <= ' '; lp2++)
            ;  // next non-white-space
         for(lp3 = lp2; *lp3 > ' '; lp3++)
            ;  // next white-space

         // at this point lp2,lp3 surrounds the variable.  Look for 'IN'
         // or 'NOT IN' to exclude this 'FOR' command from scrutiny

         for(lp2 = lp3; *lp2 && *lp2 <= ' '; lp2++)
            ;  // next non-white-space
         for(lp3 = lp2; *lp3 > ' '; lp3++)
            ;  // next white-space

         w1 = lp3 - lp2;

         if((w1 == 2 && !_fstrnicmp(lp2, "IN", w1)) ||
            (w1 == 3 && !_fstrnicmp(lp2, "NOT", w1)))
         {
            continue;  // FOR %x [NOT] IN ({files}) DO {command}
         }

         if(w1 != 4 || _fstrnicmp(lp2, "FROM", w1))
         {
            continue;  // this is REALLY a syntax error, but for now ignore it
         }

         for(lp2 = lp3; *lp2 && *lp2 <= ' '; lp2++)
            ;  // next non-white-space

         lp3 = lp2;
         w1 = 0;

         lp3 = GetEndOfTerm(lp2);

         // at this point lp2,lp3 surrounds the 'FROM' limit.  Look for 'TO'

         for(lp2 = lp3; *lp2 && *lp2 <= ' '; lp2++)
            ;  // next non-white-space
         for(lp3 = lp2; *lp3 > ' '; lp3++)
            ;  // next white-space

         w1 = lp3 - lp2;

         if(w1 != 2 || _fstrnicmp(lp2, "TO", w1))
         {
            continue;  // this is REALLY a syntax error, but for now ignore it
         }

         for(lp2 = lp3; *lp2 && *lp2 <= ' '; lp2++)
            ;  // next non-white-space

         lp3 = lp2;
         w1 = 0;

         lp3 = GetEndOfTerm(lp2);

         // at this point lp2,lp3 surrounds the 'TO' limit.  Look for the
         // optional 'STEP' command.

         for(lp2 = lp3; *lp2 && *lp2 <= ' '; lp2++)
            ;  // next non-white-space
         for(lp3 = lp2; *lp3 > ' '; lp3++)
            ;  // next white-space

         w1 = lp3 - lp2;

         if(w1 == 4 && !_fstrnicmp(lp2, "STEP", w1))
         {
            for(lp2 = lp3; *lp2 && *lp2 <= ' '; lp2++)
               ;  // next non-white-space

            lp3 = lp2;
            w1 = 0;

            lp3 = GetEndOfTerm(lp2);

            // at this point lp2,lp3 surrounds the 'STEP' value.  Point
            // to the next word following this value

            for(lp2 = lp3; *lp2 && *lp2 <= ' '; lp2++)
               ;  // next non-white-space
            for(lp3 = lp2; *lp3 > ' '; lp3++)
               ;  // next white-space

            w1 = lp3 - lp2;
         }

         // if the next word is 'DO' then it's a single-line FOR command

         if(w1 == 2 && !_fstrnicmp(lp2, "DO", w1))
         {
            continue;  // 'DO' command found - single line FOR command
         }


         // FOR/NEXT command - update 'mini stack' accordingly for nesting

         lpdwMiniStack[wMiniSP++] = CODEBLOCK_FOR;
      }
      else if(w1==4 && !_fstrnicmp(lp2, "NEXT", w1))
      {
         if(!wMiniSP)
         {
            if(wType == CODEBLOCK_FOR)  // this *IS* a FOR block!
            {
               GlobalFreePtr(lp1);

               return(SetBatchFilePosition(lpBatchInfo,dwBatchPointer));
            }

            PrintErrorMessage(904); // "?Improperly nested FOR/NEXT block\r\n\n"

            break;       // otherwise, it's an ERROR!
         }

         if(lpdwMiniStack[wMiniSP - 1] != CODEBLOCK_FOR)  // nesting error
         {
            PrintErrorMessage(904); // "?Improperly nested FOR/NEXT block\r\n\n"

            break;       // otherwise, it's an ERROR!
         }

         lpdwMiniStack[--wMiniSP] = 0;
      }
      else if(w1==6 && !_fstrnicmp(lp2, "REPEAT", w1))
      {
         // just assume syntax is valid at this point in time

         lpdwMiniStack[wMiniSP++] = CODEBLOCK_REPEAT;
      }
      else if(w1==5 && !_fstrnicmp(lp2, "UNTIL", w1))
      {
         if(!wMiniSP)
         {
            if(wType == CODEBLOCK_REPEAT)  // this *IS* a REPEAT block!
            {
               GlobalFreePtr(lp1);

               return(SetBatchFilePosition(lpBatchInfo,dwBatchPointer));
            }

            PrintErrorMessage(905); // "?Improperly nested REPEAT/UNTIL block\r\n\n"

            break;       // otherwise, it's an ERROR!
         }

         if(lpdwMiniStack[wMiniSP - 1] != CODEBLOCK_REPEAT)  // nesting error
         {
            PrintErrorMessage(905); // "?Improperly nested REPEAT/UNTIL block\r\n\n"

            break;       // otherwise, it's an ERROR!
         }

         lpdwMiniStack[--wMiniSP] = 0;
      }
      else if(w1==5 && !_fstrnicmp(lp2, "WHILE", w1))
      {
         // just assume syntax is valid at this point in time

         lpdwMiniStack[wMiniSP++] = CODEBLOCK_WHILE;
      }
      else if(w1==4 && !_fstrnicmp(lp2, "WEND", w1))
      {

do_wend:
         if(!wMiniSP)
         {
            if(wType == CODEBLOCK_WHILE)  // this *IS* a WHILE block!
            {
               GlobalFreePtr(lp1);

               return(SetBatchFilePosition(lpBatchInfo,dwBatchPointer));
            }

            PrintErrorMessage(906); // "?Improperly nested WHILE/WEND block\r\n\n"

            break;       // otherwise, it's an ERROR!
         }

         if(lpdwMiniStack[wMiniSP - 1] != CODEBLOCK_WHILE)  // nesting error
         {
            PrintErrorMessage(906); // "?Improperly nested WHILE/WEND block\r\n\n"

            break;       // otherwise, it's an ERROR!
         }

         lpdwMiniStack[--wMiniSP] = 0;
      }

   }

   if(lp1) GlobalFreePtr(lp1);  // in case I used 'break'

   return(TRUE);  // error flag...
}



/***************************************************************************/
/*                   'IF', 'ELSE', 'ENDIF' processing                      */
/***************************************************************************/


#pragma code_seg("CMDIf_TEXT","CODE")


BOOL FAR PASCAL ProcessIf(LPSTR lpTemp, BOOL bNestFlag);


BOOL FAR PASCAL CMDIf      (LPSTR lpCmd, LPSTR lpArgs)
{
LPSTR lpTemp;
BOOL rval;


   if(*lpArgs==0)
   {
      PrintErrorMessage(601);
      return(TRUE);
   }

   lpTemp = EvalString(lpArgs);  /* evaluate any equations, etc.     */
                                 /* to some extent, it's reserved... */
   if(!lpTemp)  return(CMDErrorCheck(TRUE));

   rval = ProcessIf(lpTemp, FALSE);

   if(lpTemp)   GlobalFreePtr(lpTemp);
   return(rval);

}



BOOL FAR PASCAL ProcessIf(LPSTR lpTemp, BOOL bNestFlag)
{
LPSTR lp1, lp2;
LPBLOCK_INFO lpBI;
BOOL not_flag = FALSE;
BOOL rval = TRUE;
BOOL condition = FALSE;
WORD w1, w2;
char quote_flag = 0;



   if(!lpTemp) return(TRUE);

   INIT_LEVEL


   lp1 = lpTemp;
   while(*lp1<=' ' && *lp1) lp1++;

   if(_fstrnicmp(lp1, "NOT ", 4)==0)  /* the 'NOT' prefix!! */
   {
      not_flag = TRUE;
      lp1+=4;

      while(*lp1<=' ' && *lp1) lp1++;
   }

   if(*lp1==0)        /* is that the end of the string?? */
   {
      break;      /* a SYNTAX error! */
   }

      /* check for 1 of 4 conditions:  ERRORLEVEL, EXIST, ISTASK, '==' */

   if(_fstrnicmp(lp1, "ERRORLEVEL ", 11)==0)  /* an 'errorlevel' statement */
   {
      PrintErrorMessage(602);
      condition = FALSE;

      lp1 += 11;                /* get next arg */
      while(*lp1<=' ' && *lp1) lp1++;

      if(*lp1!=0)
      {
         while(*lp1>' ') lp1++;
      }

      while(*lp1<=' ' && *lp1) lp1++;   /* get next arg after that (command!) */

      if(*lp1!=0)
      {
         rval = FALSE;    /* only if there's something to do! */
      }

      break;
   }

   else if(_fstrnicmp(lp1, "EXIST ", 6)==0)  /* file existence checking */
   {
      lp1 += 6;                         /* get next arg */
      while(*lp1<=' ' && *lp1) lp1++;

      if(*lp1==0)
      {
         break;      /* an error... */
      }

      lp2 = work_buf;  /* destination buffer... */

      while(*lp1>' ')
      {
         *lp2++ = *lp1++;  /* copy buffer until white space found */
      }
      *lp2 = 0;

      while(*lp1<=' ' && *lp1)  lp1++;  /* find the command!! */

      if(*lp1==0)               /* no command present... */
      {
         rval = TRUE;
         break;
      }

                /** using '_dos_findfirst()' get a match... **/

      condition = !_dos_findfirst(work_buf, _A_ARCH | _A_HIDDEN | _A_SUBDIR |
                                            _A_RDONLY | _A_SYSTEM, &ff);

      rval = FALSE;                /* that's all! */
      break;
   }

   /*** 'ISTASK' - a Command Line Interpreter Extension (hooray!) ***/

   else if(_fstrnicmp(lp1, "ISTASK ", 7)==0)  /* YAY! */
   {
      lp1 += 7;

      while(*lp1<=' ' && *lp1) lp1++;

      lp2 = work_buf;  /* destination buffer... */

      while(*lp1>' ')
      {
         *lp2++ = *lp1++;  /* copy buffer until white space found */
      }
      *lp2 = 0;

      while(*lp1<=' ' && *lp1)  lp1++;  /* find the command!! */

      if(*lp1==0)
      {
         rval = TRUE;
         break;
      }

      if(lpIsTask((HTASK)MyValue(work_buf)))
      {
         condition = TRUE;
      }

      rval = FALSE;          /** GOOD RETURN!! **/

      break;
   }
   else if(_fstrnicmp(lp1, "ISYES ", 6)==0)  /* YAY YAY! */
   {
      lp1 += 6;  /* point to next character */
      while(*lp1 && *lp1<=' ') lp1++;  /* find next non-white-space */

      if(!*lp1) break;  /* an error! */

      if(*lp1=='"' || *lp1=='\'')  /* a quote! */
      {
         quote_flag = *lp1++;

         lp2 = work_buf;

         while(*lp1 && *lp1!=quote_flag)
         {
            *lp2++ = *lp1++;
         }
         *lp2=0;

         if(*lp1)   lp1++;  /* quote was found */
         else       break;  /* an error */

      }
      else
      {
         lp2 = work_buf;

         while(*lp1 && *lp1>' ')
         {
            *lp2++ = *lp1++;
         }
         *lp2=0;

      }

      while(*lp1 && *lp1<=' ')  lp1++;  /* find next non-white-space */
      if(!*lp1) break;                  /* no command - syntax error! */


      if(*work_buf)
      {
         lp2 = work_buf + strlen(work_buf) - 1;

         while(*lp2 && *lp2<=' ' && lp2>=work_buf)
         {
            *lp2-- = 0;          /* trim trailing spaces */
         }
      }

      if(*work_buf)
      {
         _strupr(work_buf);       /* convert to upper case */

         condition = strncmp(work_buf,"YES",strlen(work_buf))==0;
      }
      else
         condition = FALSE;  /* always FALSE if blank */


      rval = FALSE;                /* no errors now */

   }
   else if(_fstrnicmp(lp1, "ISNO ", 5)==0)   /* YAY YAY YAY! */
   {
      lp1 += 5;  /* point to next character */
      while(*lp1 && *lp1<=' ') lp1++;  /* find next non-white-space */

      if(!*lp1) break;  /* an error! */

      if(*lp1=='"' || *lp1=='\'')  /* a quote! */
      {
         quote_flag = *lp1++;

         lp2 = work_buf;

         while(*lp1 && *lp1!=quote_flag)
         {
            *lp2++ = *lp1++;
         }
         *lp2=0;

         if(*lp1)   lp1++;  /* quote was found */
         else       break;  /* an error */

      }
      else
      {
         lp2 = work_buf;

         while(*lp1 && *lp1>' ')
         {
            *lp2++ = *lp1++;
         }
         *lp2=0;

      }

      while(*lp1 && *lp1<=' ')  lp1++;  /* find next non-white-space */
      if(!*lp1) break;                  /* no command - syntax error! */


      if(*work_buf)
      {
         lp2 = work_buf + strlen(work_buf) - 1;

         while(*lp2 && *lp2<=' ' && lp2>=work_buf)
         {
            *lp2-- = 0;          /* trim trailing spaces */
         }
      }

      if(*work_buf)
      {
         _strupr(work_buf);       /* convert to upper case */

         condition = strncmp(work_buf,"NO",strlen(work_buf))==0;
      }
      else
         condition = FALSE;  /* always FALSE if blank */


      rval = FALSE;                /* no errors now */

   }
   else if(*lp1=='(')   // an equation!  This is *SPECIAL*
   {                    // 'TRUE' is a non-zero number or non-blank string
    WORD nP = 1;
    LPSTR lpEQ;

      lpEQ = lp2 = lp1 + 1;

      while(*lp2 && nP)
      {
         if(*lp2=='\'' || *lp2=='"')
         {
            quote_flag = *(lp2++);

            while(*lp2 && *lp2!=quote_flag)
            {
               if(*lp2=='\\')
               {
                  lp2++;

                  if(*lp2) lp2++;  // always skip 2nd character if not 0
               }
               else
               {
                  lp2++;           // normally just increment through!
               }
            }
         }
         else if(*lp2=='(')
         {
            nP++;
         }
         else if(*lp2==')')
         {
            nP--;

            if(!nP) break;  // final ')' is still pointed to by 'lp2'
         }

         lp2++;
      }

      if(!*lp2 || nP)
      {
         rval = TRUE;
      }
      else
      {
         *(lp2++) = 0;  // terminates end of equation

         lpEQ = Calculate(lpEQ);  // get the result of the equation
         if(!lpEQ)
         {
            rval = TRUE;
         }
         else
         {
            condition=TRUE;  // initial value - look for non-zero result!

            lp1 = lpEQ;
            if(*lp1=='-')            lp1++;  // ignore initial minus sign!

            while(*lp1 && *lp1<=' ') lp1++;  // go past any white space

            if(*lp1>='0' && *lp1<='9')
            {
               // assume this is a number...

               condition = (CalcEvalNumber(lp1)!=0.0);
            }
            else if(!*lp1)
            {
               if(*lpEQ<=' ')
                  condition = FALSE;  // blank or 'white-space' string
            }

            GlobalFreePtr(lpEQ);      // free up memory used here
         }

         lp1 = lp2;

         while(*lp1 && *lp1<=' ') lp1++; // get next 'non-white-space'

         rval = FALSE;                   // flag 'no error'
      }
   }
   else           // standard 'IF' - string compare
   {
    char quote_flag = 0;

      /* this will be a bit more difficult... */

      if(*lp1=='"' || *lp1=='\'')  /* a quote! */
      {
         quote_flag = *lp1++;

         lp2 = work_buf;

         while(*lp1 && *lp1!=quote_flag)
         {
            *lp2++ = *lp1++;
         }
         *lp2=0;

         if(*lp1)   lp1++;  /* quote was found */
         else       break;  /* an error */

      }
      else
      {
         if(lp2 = _fstrstr(lp1, "=="))
         {
            _fmemcpy(work_buf, lp1, (WORD)(lp2 - lp1));
            work_buf[(WORD)(lp2 - lp1)] = 0;
            lp1 = lp2;
         }
      }

      if(_fmemcmp(lp1, "==", 2)!=0)
      {
         break;             /** this is an error condition! **/
      }


      lp1 += 2;

      if(*lp1=='"' || *lp1=='\'')  /* a quote! */
      {
         quote_flag = *lp1++;
      }
      else
      {
         quote_flag = 0;
      }

      lp2 = work_buf;

      condition = TRUE;  /* initially say they're equal... */

      while(*lp1 && ((!quote_flag && *lp1>' ') ||
                     (quote_flag && *lp1!=quote_flag)))
      {
         if(!*lp2 || *(lp2++) != *(lp1++))  /* not a match */
         {
            condition = FALSE;  /* well, they don't match, now, do they? */
            break;                    /* strings must match, exactly! */
         }
      }

      if(condition==TRUE) /* we *think* they match, but '*lp2' should be 0 */
      {
         if(*lp2) condition=FALSE;  /* if lp2 not at end of string, NOPE! */
      }
      else
      {
         while(*lp1 && ((!quote_flag && *lp1>' ') ||
                        (quote_flag && *lp1!=quote_flag)))
         {
            lp1++;  /* find the end of the right-hand string */
         }
      }

//      while(*lp1 && *lp2 && ((!quote_flag && *lp1>' ') || *lp1!=quote_flag))
//      {
//         if(*lp2++ != *lp1++)
//         {
//            condition = FALSE;  /* well, they don't match, now, do they? */
//            break;                    /* strings must match, exactly! */
//         }
//      }

      if(*lp1==0)    /* error */
      {
         break;
      }

      if(quote_flag && *lp1!=quote_flag)
         lp1++;                               /* find end of quoted string */
                                 /* which may contain embedded white space */

      while(*lp1>' ')  lp1++;        /* find next (or current) white space */
                                          /* which must follow the strings */

      while(*lp1<=' ' && *lp1)  lp1++;        /* find next non-white space */
                                        /* which happens to be the command */

      if(*lp1==0)  break;  /* error - end of string (no command) */

      rval = FALSE;        /* result is GOOD, represents reality */

   }


   END_LEVEL


   if(!rval) while(*lp1 && *lp1<=' ') lp1++;   // just in case...

   if(rval)                                    /* improper 'IF' clause */
   {
      PrintErrorMessage(603); // "?Syntax error in 'IF' statement...\r\n"

      return(TRUE);
   }

   // OK!  Lets' see if this is an optional 'THEN' keyword...

   for(lp2=lp1; *lp2>' '; lp2++)
      ;  // finds end of keyword

   if(!*lp1 || (lp2 - lp1)==4 && !_fstrnicmp("THEN", lp1, 4))
   {
      // WE ARE USING THE 'IF-THEN/ELSE/ENDIF' construct!

      if(!BatchMode)
      {
         PrintErrorMessage(578); // NOT VALID IN COMMAND MODE
         return(TRUE);
      }
      else if((!bNestFlag || !lpBatchInfo->lpBlockInfo) &&
              VerifyBlockInfo(&(lpBatchInfo->lpBlockInfo)))
      {
         // NOTE:  only checked if not nested or NULL 'lpBlockInfo'

         PrintString(pNOTENOUGHMEMORY);
         return(TRUE);
      }

      // This section of code uses the 'lpBlockInfo' element of
      // the current 'BATCH_INFO' structure to store information
      // about the conditional block.

      lpBI = lpBatchInfo->lpBlockInfo;

      if(bNestFlag)  // this was 'passed to' by the 'CMDElse()' function
      {
         rval = TRUE;

         for(w1 = (WORD)lpBI->dwEntryCount; w1 > 0;)
         {
            w1--;  // pre-decrement in this case

            // We are looking for the last REPEAT command that we find.

            if((lpBI->lpCodeBlock[w1].dwType & CODEBLOCK_TYPEMASK)
                == CODEBLOCK_IF)
            {
               // remove ALL nested loops "below" this one

               if(lpBI->dwEntryCount)
               {
                  for(w2 = (WORD)(lpBI->dwEntryCount - 1); w2 > w1; w2--)
                  {
                     RemoveCodeBlockEntry(lpBI->lpCodeBlock + w2);
                  }

                  lpBI->dwEntryCount = w1 + 1;  // reset new entry count
               }

               rval = FALSE;
               break;
            }
         }

         if(rval)
         {
            // failed to find the matching 'IF' block...

            PrintErrorMessage(914);  // "?Missing IF in IF/ELSE/ENDIF\r\n\n"
         }
      }
      else
      {
         w1 = (WORD)lpBI->dwEntryCount++;

         // record next line's batch file position in 'dwStart'

         lpBI->lpCodeBlock[w1].dwStart = lpBatchInfo->buf_start
                                       + lpBatchInfo->buf_pos;

         lpBI->lpCodeBlock[w1].dwBlockEnd = (DWORD)-1L;  // a flag

         lpBI->lpCodeBlock[w1].szInfo[0] = 0;  // not used for IF

         lpBI->lpCodeBlock[w1].dwType = CODEBLOCK_IF;  // it's an 'IF' block
      }

      if(!rval)
      {
         if(lpBI->lpCodeBlock[w1].dwType & CODEBLOCK_IF_IGNOREALL)
         {
            lpBI->lpCodeBlock[w1].dwType &= ~CODEBLOCK_IF_TRUE;
            lpBI->lpCodeBlock[w1].dwType &= ~CODEBLOCK_IF_IGNORE;

            // skip to next section - assume condition to be FALSE (ignore it)

            if(FindNextMatchingSection(lpBI->lpCodeBlock + w1))
            {
               PrintErrorMessage(915); // "?Missing ENDIF in IF/ELSE/ENDIF\r\n\n"

               TerminateCurrentBatchFile();
               rval = TRUE;
            }
         }
         else if(condition)
         {
            lpBI->lpCodeBlock[w1].dwType &= ~CODEBLOCK_IF_IGNORE;
            lpBI->lpCodeBlock[w1].dwType |= CODEBLOCK_IF_TRUE;

            // continue at next line - condition is TRUE!
         }
         else
         {
            lpBI->lpCodeBlock[w1].dwType &= ~CODEBLOCK_IF_TRUE;
            lpBI->lpCodeBlock[w1].dwType |= CODEBLOCK_IF_IGNORE;

            if(FindNextMatchingSection(lpBI->lpCodeBlock + w1))
            {
               PrintErrorMessage(915); // "?Missing ENDIF in IF/ELSE/ENDIF\r\n\n"

               TerminateCurrentBatchFile();
               rval = TRUE;
            }
         }
      }
   }
//   else if(bNestFlag)                 // COMMAND FOLLOWS, but we're "ELSE IF"
//   {
//      PrintString("?'ELSE IF' does not allow command on same line\r\n");
//
//      rval = TRUE;
//   }
   else                               // COMMAND FOLLOWS 'IF' statement!
   {
      if((!not_flag && condition) || (not_flag && !condition))
      {
         rval = ProcessCommand(lp1); /* process the remainder of it!! */
      }
   }


   return(rval);


}

BOOL FAR PASCAL CMDElse    (LPSTR lpCmd, LPSTR lpArgs)
{
LPSTR lp1, lp2, lpTemp;
LPBLOCK_INFO lpBI;
WORD w1, w2;
BOOL bElseIf = FALSE;
BOOL rval;


   if(!BatchMode)
   {
      PrintErrorMessage(578); // NOT VALID IN COMMAND MODE
      return(TRUE);
   }

   // This section of code uses the 'lpBlockInfo' element of
   // the current 'BATCH_INFO' structure to store information
   // about the conditional block.


   lpTemp = EvalString(lpArgs);

   if(*lpTemp)
   {
      lp1 = lpTemp;
      while(*lp1 && *lp1<=' ') lp1++;

      lp2 = lp1;

      while(*lp2 > ' ') lp2++;

      if((lp2 - lp1)!=2 || _fstrnicmp(lp1, "IF", 2)) // syntax error
      {
         PrintErrorMessage(603); // "?Syntax error in 'IF' statement...\r\n"
         GlobalFreePtr(lpTemp);

         return(TRUE);
      }

      // NOTE:  'lp1' points to the IF keyword...

      lp1 += 2;

      if(*lp1 != '(')  // allow 'else if('
      {
        while(*lp1 && *lp1<=' ') lp1++;
      }

      if(!*lp1)  // no equation!
      {
         PrintErrorMessage(603); // "?Syntax error in 'IF' statement...\r\n"
         GlobalFreePtr(lpTemp);

         return(TRUE);
      }

      bElseIf = TRUE;
   }

   // SYNTAX:   ELSE
   //           ELSE IF {condition} THEN
   //

   rval = TRUE;
   lpBI = lpBatchInfo->lpBlockInfo;

   for(w1 = (WORD)lpBI->dwEntryCount; w1 > 0;)
   {
      w1--;  // pre-decrement in this case

      // We are looking for the last REPEAT command that we find.

      if((lpBI->lpCodeBlock[w1].dwType & CODEBLOCK_TYPEMASK)
          == CODEBLOCK_IF)
      {
         // remove ALL nested loops "below" this one

         if(lpBI->dwEntryCount)
         {
            for(w2 = (WORD)lpBI->dwEntryCount - 1; w2 > w1; w2--)
            {
               RemoveCodeBlockEntry(lpBI->lpCodeBlock + w2);
            }

            lpBI->dwEntryCount = w1 + 1;  // reset new entry count
         }

         rval = FALSE;
         break;
      }
   }

   if(rval)
   {
      // failed to find the matching 'IF' block...

      PrintErrorMessage(914);  // "?Missing IF in IF/ELSE/ENDIF\r\n\n"
   }
   else
   {
      if(lpBI->lpCodeBlock[w1].dwType & CODEBLOCK_IF_IGNOREALL)
      {
         lpBI->lpCodeBlock[w1].dwType &= ~CODEBLOCK_IF_TRUE;
         lpBI->lpCodeBlock[w1].dwType &= ~CODEBLOCK_IF_IGNORE;

         // skip to next section - assume condition to be FALSE (ignore it)

         if(FindNextMatchingSection(lpBI->lpCodeBlock + w1))
         {
            PrintErrorMessage(915); // "?Missing ENDIF in IF/ELSE/ENDIF\r\n\n"

            TerminateCurrentBatchFile();
            rval = TRUE;
         }
      }
      else if(lpBI->lpCodeBlock[w1].dwType & CODEBLOCK_IF_TRUE)
      {
         // last 'IF' or 'ELSE IF' was TRUE - ignore all other stuff in block

         lpBI->lpCodeBlock[w1].dwType &= ~CODEBLOCK_IF_TRUE;
         lpBI->lpCodeBlock[w1].dwType &= ~CODEBLOCK_IF_IGNORE;

         lpBI->lpCodeBlock[w1].dwType |= CODEBLOCK_IF_IGNOREALL;

         if(FindNextMatchingSection(lpBI->lpCodeBlock + w1))
         {
            PrintErrorMessage(915); // "?Missing ENDIF in IF/ELSE/ENDIF\r\n\n"

            TerminateCurrentBatchFile();
            rval = TRUE;
         }
      }
      else if(lpBI->lpCodeBlock[w1].dwType & CODEBLOCK_IF_IGNORE)
      {  // all other conditions were FALSE and this is a regular 'ELSE'
         // or an 'ELSE IF' which is in the correct position...

         if(bElseIf)
         {
            rval = ProcessIf(lp1, TRUE);
         }
         else
         {
            // by turning off both the 'TRUE' and the 'IGNORE' bits,
            // I am flagging that a "regular ELSE" was executed (see below)

            lpBI->lpCodeBlock[w1].dwType &= ~CODEBLOCK_IF_TRUE;
            lpBI->lpCodeBlock[w1].dwType &= ~CODEBLOCK_IF_IGNORE;
         }
      }
      else // misplaced 'ELSE' in this IF block!
      {
         if(bElseIf)
         {
            PrintErrorMessage(917); // "?Misplaced 'ELSE IF' in IF/ELSE/ENDIF block\r\n\n"
         }
         else
         {
            PrintErrorMessage(916); // "?Misplaced 'ELSE' in IF/ELSE/ENDIF block\r\n\n"
         }

         TerminateCurrentBatchFile();
         rval = TRUE;
      }
   }


   GlobalFreePtr(lpTemp);

   return(rval);

}

BOOL FAR PASCAL CMDEndif   (LPSTR lpCmd, LPSTR lpArgs)
{
LPSTR lp1;
WORD w1, w2;
LPBLOCK_INFO lpBI;


   if(!BatchMode || !lpBatchInfo)
   {
      PrintErrorMessage(578); // NOT VALID IN COMMAND MODE
      return(TRUE);
   }
   else if(!lpBatchInfo->lpBlockInfo)
   {
      PrintErrorMessage(918); // "?ENDIF not valid without corresponding 'IF'\r\n\n"
      return(TRUE);
   }

   if(lpArgs) lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   for(lp1=lpArgs; *lp1 && *lp1<=' '; lp1++)
      ; // next non-white-space char

   if(*lp1) // syntax error kiddies!
   {
      PrintString(pSYNTAXERROR);
      PrintString(lpArgs);
      PrintString(pQCRLFLF);

      GlobalFreePtr(lpArgs);
      return(TRUE);
   }

   GlobalFreePtr(lpArgs);  // no longer needed now!
   lpArgs = NULL;  // just in case


   lpBI = lpBatchInfo->lpBlockInfo;

   for(w1 = (WORD)lpBI->dwEntryCount; w1 > 0;)
   {
      w1--;  // pre-decrement in this case

      // We are looking for the last WHILE command that we find.

      if((lpBI->lpCodeBlock[w1].dwType & CODEBLOCK_TYPEMASK)==CODEBLOCK_IF)
      {
         // remove ALL nested loops "below" this one

         if(lpBI->dwEntryCount)
         {
            for(w2 = (WORD)(lpBI->dwEntryCount - 1); w2 >= w1; w2--)
            {
               RemoveCodeBlockEntry(lpBI->lpCodeBlock + w2);
            }

            lpBI->dwEntryCount = w1;  // reset new entry count
         }

         return(FALSE);  // complete!
      }
   }


   PrintErrorMessage(918); // "?ENDIF not valid without corresponding 'IF'\r\n\n"
   return(TRUE);
}




/***************************************************************************/
/*                   EXIT COMMANDS - all of them!!                         */
/***************************************************************************/

#pragma code_seg("CMDExit_TEXT","CODE")

BOOL FAR PASCAL CMDExit    (LPSTR lpCmd, LPSTR lpArgs)
{
LPSTR lp1, lp2, lp3;
LPBLOCK_INFO lpBI;
LPCSTR lpComSpec;
WORD w1, w2, ExitFlag=0;
int ierr;
static BOOL recurse=FALSE;

static const char CODE_BASED szRESTART[]="RESTART";
static const char CODE_BASED szLOOP[]   ="LOOP";
static const char CODE_BASED szREBOOT[] ="REBOOT";
static const char CODE_BASED szWINDOWS[]="WINDOWS";



   if(recurse) return(FALSE);

   recurse = TRUE;

   if(lpArgs)
   {
      lpArgs = EvalString(lpArgs);
      if(!lpArgs)
      {
         PrintString(pNOTENOUGHMEMORY);

         recurse = FALSE;
         return(TRUE);
      }

      for(lp1=lpArgs; *lp1 && *lp1<=' '; lp1++)
         ;  // find first non-white-space

      if(*lp1)
      {
         for(lp2=lp1; *lp2>' '; *lp2++)
            ;  // find next white space

         w1 = lp2 - lp1;

         if(w1==(sizeof(szWINDOWS) - 1) && !_fstrnicmp(lp1, szWINDOWS, w1))
         {
            ExitFlag = 1;
         }
         else if(w1==(sizeof(szRESTART) - 1) && !_fstrnicmp(lp1, szRESTART, w1))
         {
            ExitFlag = 2;
         }
         else if(w1==(sizeof(szREBOOT) - 1) && !_fstrnicmp(lp1, szREBOOT, w1))
         {
            ExitFlag = 3;
         }
         else if(w1==(sizeof(szLOOP) - 1) && !_fstrnicmp(lp1, szLOOP, w1))
         {
            ExitFlag = 4;
         }
         else
         {
            PrintErrorMessage(623);

            recurse = FALSE;
            return(TRUE);
         }
      }
   }


   INIT_LEVEL


   if(ExitFlag != 4)  // not 'EXIT LOOP'
   {
      if(copying)
      {
       static const char CODE_BASED pMsg[]=
                                                 "COPY in progress... Abort?";
       static const char CODE_BASED pMsg2[]="** CLOSE SFTSHELL **";

         LockCode();

         ierr = MessageBox(hMainWnd, pMsg, pMsg2,
                           MB_YESNO | MB_ICONHAND | MB_SYSTEMMODAL);

         UnlockCode();

         if(ierr!=IDYES) break;
      }
   }



   if(ExitFlag == 0 && !IsShell)
   {
      DestroyWindow(hMainWnd);

      PostQuitMessage(0);

      hMainWnd = (HWND)NULL;   /* well, it doesn't exist anymore!! */
   }
   else if(ExitFlag == 1 || ExitFlag == 0)  // WINDOWS
   {
    static const char CODE_BASED pMsg1[] =
                                        "This will end your WINDOWS session";
    static const char CODE_BASED pMsg2[] = "** EXIT WINDOWS **";
    static const char CODE_BASED pMsg3[] =
                                 "One or more applications refused to exit!";

      LockCode();

      ierr = MessageBox(hMainWnd, pMsg1, pMsg2,
                        MB_OKCANCEL | MB_ICONINFORMATION | MB_TASKMODAL);

      UnlockCode();

      if(ierr!=IDOK)  break;

      ExitWindows(0L,0);

      LockCode();

      MessageBox(hMainWnd, pMsg3, pMsg2,
                 MB_OK | MB_ICONHAND | MB_SYSTEMMODAL);

      UnlockCode();

      break;
   }
   else if(ExitFlag == 2)  // RESTART
   {
    static const char CODE_BASED pMsg2[] = "** EXIT RESTART **";
    static const char CODE_BASED pMsg3[] =
                                 "One or more applications refused to exit!";

      while(*lp2 && *lp2<=' ') lp2++;  // find remainder of command line
                                       // that might contain a 'RESTART' cmd

      if(*lp2)     // that is, 'lp2' points to a command to be executed...
      {
       BOOL (WINAPI *lpExitWindowsExec)(LPCSTR, LPCSTR);

         if(!(lpComSpec = GetEnvString(pCOMSPEC)))
            lpComSpec = pCOMMANDCOM;

         (FARPROC &)lpExitWindowsExec = GetProcAddress(hUser, "ExitWindowsExec");

         if(lpExitWindowsExec)
         {
            lp2 -= 4;        // add prefix of '/C'
            *lp2 = ' ';
            lp2[1] = '/';
            lp2[2] = 'C';
            lp2[3] = ' ';

            lpExitWindowsExec(lpComSpec, lp2);
         }
         else
         {
            ExitWindows(EW_RESTARTWINDOWS, 0);
         }
      }
      else
      {
         ExitWindows(EW_RESTARTWINDOWS, 0);
      }

      LockCode();

      MessageBox(hMainWnd, pMsg3, pMsg2,
                 MB_OK | MB_ICONHAND | MB_SYSTEMMODAL);

      UnlockCode();

      break;
   }
   else if(ExitFlag == 3) // REBOOT
   {
    static const char CODE_BASED pMsg2[] = "** EXIT REBOOT **";
    static const char CODE_BASED pMsg3[] =
                                 "One or more applications refused to exit!";

      ExitWindows(EW_REBOOTSYSTEM, 0);

      LockCode();

      MessageBox(hMainWnd, pMsg3, pMsg2,
                 MB_OK | MB_ICONHAND | MB_SYSTEMMODAL);

      UnlockCode();

      break;
   }
   else if(ExitFlag==4)            // EXIT LOOP!!
   {
      if(!BatchMode || !lpBatchInfo)
      {
         PrintErrorMessage(578); // NOT VALID IN COMMAND MODE

         recurse=FALSE;
         return(TRUE);
      }

      if(!lpBatchInfo->lpBlockInfo)
      {
         PrintErrorMessage(895); // "?EXIT LOOP/CONTINUE not valid outside of loop\r\n\n"

         recurse=FALSE;
         return(TRUE);
      }

      // find the END of the loop...

      lpBI = lpBatchInfo->lpBlockInfo;

      for(w1 = (WORD)lpBI->dwEntryCount; w1 > 0;)
      {
         w1--;  // pre-decrement in this case

         // We are looking for the last WHILE/REPEAT/FOR command that we find.

         if((lpBI->lpCodeBlock[w1].dwType & CODEBLOCK_TYPEMASK)
             == CODEBLOCK_WHILE ||
            (lpBI->lpCodeBlock[w1].dwType & CODEBLOCK_TYPEMASK)
             == CODEBLOCK_REPEAT ||
            (lpBI->lpCodeBlock[w1].dwType & CODEBLOCK_TYPEMASK)
             == CODEBLOCK_FOR)
         {
            // jump to the end of the loop

            if(FindNextMatchingSection(lpBI->lpCodeBlock + w1))
            {
               // print an appropriate error message

               if((lpBI->lpCodeBlock[w1].dwType & CODEBLOCK_TYPEMASK)
                  == CODEBLOCK_WHILE)
               {
                  PrintErrorMessage(894); // "?Missing END WHILE/WEND in WHILE loop\r\n\n"
               }
               else if((lpBI->lpCodeBlock[w1].dwType & CODEBLOCK_TYPEMASK)
                       == CODEBLOCK_REPEAT)
               {
                  PrintErrorMessage(912); // "?Missing UNTIL in REPEAT/UNTIL loop\r\n\n"
               }
               else  // assume it's a 'FOR/NEXT' loop
               {
                  PrintErrorMessage(913); // "?Missing NEXT in FOR/NEXT loop\r\n\n"
               }

               TerminateCurrentBatchFile();

               recurse=FALSE;
               return(TRUE);
            }

            // loop found.  clear all items from this point forward

            for(w2 = (WORD)lpBI->dwEntryCount; w2 > w1; )
            {
               w2--;  // pre-decrement
               RemoveCodeBlockEntry(lpBI->lpCodeBlock + w2);
            }

            lpBI->dwEntryCount = w1;  // reset new entry count


            lp1 = GetAnotherBatchLine(lpBatchInfo);  // this is the 'UNTIL/WEND/NEXT'
            if(lp1) GlobalFreePtr(lp1);   // just 'cause - ignore any error here

            recurse=FALSE;
            return(FALSE);  // must use 'return' or I can't detect success!
         }
         else if((lpBI->lpCodeBlock[w1].dwType & CODEBLOCK_TYPEMASK)
                  == CODEBLOCK_IF)
         {
            // when I find a nested 'IF' block I must jump to the 'ENDIF'
            // or else I'll have one TERRIFIC problem...!

            do
            {
               if(FindNextMatchingSection(lpBI->lpCodeBlock + w1))
               {
                  PrintErrorMessage(915); // "?Missing ENDIF in IF/ELSE/ENDIF\r\n\n"

                  TerminateCurrentBatchFile();

                  recurse=FALSE;
                  return(TRUE);
               }

               lp1 = GetAnotherBatchLine(lpBatchInfo);  // get the line...

               if(!lp1)
               {
                  PrintErrorMessage(915); // "?Missing ENDIF in IF/ELSE/ENDIF\r\n\n"

                  TerminateCurrentBatchFile();

                  recurse=FALSE;
                  return(TRUE);
               }

               for(lp2=lp1; *lp2 && *lp2<=' '; lp2++)
                  ;  // trim lead white space

               for(lp3=lp2; *lp3 > ' '; lp3++)
                  ;  // find next non-white-space

               w2 = lp3 - lp2;

               if(w2==3 && !_fstrnicmp(lp2, "END", w2))
               {
                  for(lp2=lp3; *lp2 && *lp2<=' '; lp2++)
                     ;  // trim lead white space

                  for(lp3=lp2; *lp3 > ' '; lp3++)
                     ;  // find next non-white-space

                  w2 = lp3 - lp2;

                  if(w2==2 && !_fstrnicmp(lp2, "IF", w2))
                  {
                     GlobalFreePtr(lp1);
                     break;  // an ENDIF was found!!
                  }
               }
               else if(w2==5 && !_fstrnicmp(lp2, "ENDIF", w2))
               {
                  GlobalFreePtr(lp1);
                  break;
               }

               GlobalFreePtr(lp1);

            } while(TRUE);

         }
      }

      PrintErrorMessage(895); // "?EXIT LOOP/CONTINUE not valid outside of loop\r\n\n"

      recurse=FALSE;
      return(TRUE);

   }


   END_LEVEL


   recurse=FALSE;

   return(FALSE);
}





/***************************************************************************/
/*                    The 'WAIT' command goes here!!                       */
/***************************************************************************/

#pragma code_seg("CMDWait_TEXT","CODE")

BOOL FAR PASCAL CMDWait    (LPSTR lpCmd, LPSTR lpArgs)
{
LPSTR lp1, lp2, lp3;
BOOL IsUntil, QuietFlag=FALSE;
static struct dostime_t dtt;
static struct dosdate_t ddt;
SFTTIME t0;
DWORD dwNSec;
static char pPM[]="PM", pAM[]="AM";


   lpArgs = EvalString(lpArgs);
   if(!lpArgs)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }

   _fmemset((LPSTR)&WaitForDay, 0, sizeof(WaitForDay));
   _fmemset((LPSTR)&WaitForTime, 0, sizeof(WaitForTime));

   // first, see if a '/Q' is present...

   lp1 = _fstrrchr(lpArgs, '/');
   if(lp1)
   {
      if(toupper(*(lp1 + 1))=='Q')
      {
         for(lp2 = lp1+2; *lp2 && *lp2<=' '; lp2++)
            ;  // go to end of string - look for more stuff...

         if(*lp2)  // whoops - other things follow!
         {
            PrintString(pSYNTAXERROR);
            PrintString(lp2);
            PrintString(pQCRLFLF);
            GlobalFreePtr(lpArgs);
            return(TRUE);
         }

         *lp1 = 0;          // that ends that!

         QuietFlag = TRUE;  // this says 'no messages'
      }
   }


   for(lp1=lpArgs; *lp1 && *lp1<=' '; lp1++)
      ;  /* find next non-white-space */


   if(lstrlen(lp1)>=6)   /* 'UNTIL ' */
   {
      if(_fstrnicmp(lp1, "UNTIL ", 6)==0)
      {
         IsUntil = TRUE;
         lp1 = lp1 + 6;
      }
      else if(_fstrnicmp(lp1, "FOR ", 4)!=0)
      {
         PrintErrorMessage(674);
         PrintString(lp1);
         PrintString(pQCRLFLF);

         GlobalFreePtr(lpArgs);
         return(TRUE);
      }
      else
      {
         IsUntil = FALSE;
         lp1 = lp1 + 4;
      }
   }
   else
   {
      PrintErrorMessage(675);
      PrintString(lp1);
      PrintString(pQCRLFLF);

      GlobalFreePtr(lpArgs);
      return(TRUE);
   }


   for(;*lp1 && *lp1<=' '; lp1++)
      ;  /* find next non-white-space */

   if(!*lp1)         /* no more arguments?  already? */
   {
      PrintErrorMessage(676);

      GlobalFreePtr(lpArgs);
      return(TRUE);
   }

   for(lp2 = work_buf; *lp1>' '; lp1++, lp2++)
   {
      *lp2 = *lp1;       /* copy 1st argument */
   }
   *lp2++ = 0;


   for(; *lp1 && *lp1<=' '; lp1++)
      ;  /* find next non-white-space */


   if(*lp1)       /* there's something, eh? */
   {
      if(!IsUntil)
      {
         PrintErrorMessage(677);

         GlobalFreePtr(lpArgs);
         return(TRUE);
      }

      for(lp3 = lp2; *lp1>' '; lp1++, lp3++)
      {
         *lp3 = *lp1;       /* copy 1st argument */
      }
      *lp3 = 0;
   }
   else
   {
      lp2 = NULL;       /* null pointer - one arg only! */
   }

   lp1 = (LPSTR)work_buf;  /* now: lp1==1st arg, lp2==2nd arg or NULL */

    /* ok - are we waiting 'for' or 'until'? */
   if(IsUntil)
   {
      if(lp2)          /* there are 2 parameters */
      {
         if(atodate(lp1, (LPSFTDATE)&WaitForDay) &&
            GetDayOfWeekDate(lp1, (LPSFTDATE)&WaitForDay))
         {


            PrintErrorMessage(678);
            PrintString(lp1);
            PrintString(pQCRLFLF);

            GlobalFreePtr(lpArgs);
            return(TRUE);
         }

         if(atotime(lp2, (LPSFTTIME)&WaitForTime))
         {
            PrintErrorMessage(679);
            PrintString(lp2);
            PrintString(pQCRLFLF);

            GlobalFreePtr(lpArgs);
            return(TRUE);
         }
      }
      else
      {
             /* user could specify either a date or a time */

         if(atodate(lp1, (LPSFTDATE)&WaitForDay) &&
            GetDayOfWeekDate(lp1, (LPSFTDATE)&WaitForDay))
         {
                /*** It's not a date - is it a time? ***/

            _fmemset((LPSTR)&WaitForDay, 0, sizeof(WaitForDay));

            if(atotime(lp1, (LPSFTTIME)&WaitForTime))
            {
               PrintErrorMessage(680);
               PrintString(lp1);
               PrintString(pQCRLFLF);

               GlobalFreePtr(lpArgs);
               return(TRUE);
            }
         }

               /* at this point the appropriate date/time */
               /* flags ('WaitForDay' or 'WaitForTime')   */
               /* have been set to the appropriate value. */
      }


      if(!QuietFlag)        // do we wish to inform the user of what is
      {                     // going on?  That is, waiting FOR/UNTIL ...

         if(lp2)       /* both a DATE and a TIME */
         {
          static const char CODE_BASED pFmt[]=
                     "\r\nWAITING UNTIL %02d/%02d/%04d, %02d:%02d:%02d %s";

            LockCode();
            wsprintf(work_buf, pFmt,
                     WaitForDay.day, WaitForDay.month, WaitForDay.year,
                     WaitForTime.hour>=12?WaitForTime.hour-12:WaitForTime.hour,
                     WaitForTime.minute, WaitForTime.second,
                     (LPSTR)(WaitForTime.hour>=12?(LPSTR)pPM:(LPSTR)pAM ));
            UnlockCode();
         }
         else  /* either one or the other; however, catch possible errors too */
         {
            if(WaitForDay.day || WaitForDay.month || WaitForDay.year)
            {
             static const char CODE_BASED pFmt[]=
                                         "\r\nWAITING UNTIL  %02d/%02d/%04d";
               LockCode();
               wsprintf(work_buf, pFmt,
                        WaitForDay.day, WaitForDay.month, WaitForDay.year);
               UnlockCode();
            }

            if(WaitForTime.hour || WaitForTime.minute || WaitForTime.second)
            {
             static const char CODE_BASED pFmt[]=
                                      "\r\nWAITING UNTIL  %02d:%02d:%02d %s";
               LockCode();
               wsprintf(work_buf, pFmt,
                     WaitForTime.hour>=12?WaitForTime.hour-12:WaitForTime.hour,
                     WaitForTime.minute, WaitForTime.second,
                     (LPSTR)(WaitForTime.hour>=12?(LPSTR)pPM:(LPSTR)pAM ));
               UnlockCode();
            }
         }

         PrintString(work_buf);

         PrintErrorMessage(845);  // " ..."

      }


      _dos_gettime(&dtt);

      t0.hour = dtt.hour;
      t0.minute = dtt.minute;
      t0.second = dtt.second;

      if(WaitForTime.hour==0 && dtt.hour>0)
      {
         WaitForTime.hour = 24;/* this places us *before* the time! */
      }

      WaitForTimeFlag = timecmp((LPSFTTIME)&WaitForTime, (LPSFTTIME)&t0);

      waiting_flag = TRUE;             // for STATUS dialog!

      do
      {
         if(LoopDispatch() || ctrl_break)
         {
            if(hMainWnd && IsWindow(hMainWnd))
            {
               PrintErrorMessage(513);

            }

            waiting_flag = FALSE;
            break;
         }

         if(WaitForDay.year!=0 && WaitForDay.month!=0
            && WaitForDay.day!=0)
         {
            _dos_getdate(&ddt);

            if(WaitForDay.year>ddt.year ||
               WaitForDay.month>ddt.month ||
               WaitForDay.day>ddt.day)
            {
               continue;             /* didn't pass test - keep waiting */
            }
            else if(WaitForDay.year<ddt.year ||
                    WaitForDay.month<ddt.month ||
                    WaitForDay.day<ddt.day)
            {
               waiting_flag = FALSE;     /* I'm done now! */

               WaitForDay.year = 0;
               WaitForDay.month = 0;
               WaitForDay.day = 0;
               WaitForTime.hour = 0;
               WaitForTime.minute = 0;
               WaitForTime.second = 0;

               if(hMainWnd && IsWindow(hMainWnd))
               {
                  if(!QuietFlag)
                  {
                     PrintErrorMessage(514);
                  }

               }

               break;
            }

         }

         if(WaitForTimeFlag==0)         /* initially compare times */
         {
            _dos_gettime(&dtt);

            t0.hour = dtt.hour;
            t0.minute = dtt.minute;
            t0.second = dtt.second;

            /** Special section to handle '0' hours!   **/
            /** if not executed at least once per hour **/
            /** this algorithm won't work.. not likely **/

            if(WaitForTime.hour==0 && dtt.hour>0)
            {
               WaitForTime.hour = 24;/* this places us *before* the time! */
            }

            WaitForTimeFlag = timecmp((LPSFTTIME)&WaitForTime, (LPSFTTIME)&t0);
         }
         else
         {
            _dos_gettime(&dtt);

            t0.hour = dtt.hour;
            t0.minute = dtt.minute;
            t0.second = dtt.second;

            /** Special section to handle '0' hours!   **/
            /** if not executed at least once per hour **/
            /** this algorithm won't work.. not likely **/

            if(WaitForTime.hour==24 && dtt.hour==0)
            {
               WaitForTime.hour = 0;  /* corrects for '0:0:0' problems */
            }

            if(WaitForTimeFlag!=timecmp((LPSFTTIME)&WaitForTime, (LPSFTTIME)&t0))
            {
               if(WaitForTimeFlag<0)
               {
                  WaitForTimeFlag = timecmp((LPSFTTIME)&WaitForTime,
                                            (LPSFTTIME)&t0);
                  if(WaitForTimeFlag)
                     continue;               /* low to high transition */
                                            /* i.e. crossing midnight */
               }

                  /** Since high to low transition (or equal) is valid **/
                  /** for stop times, I can now breathe easily.  I am  **/
                  /** now FINISHED WAITING!                            **/

               waiting_flag = FALSE;     /* I'm done now! */

               WaitForDay.year = 0;
               WaitForDay.month = 0;
               WaitForDay.day = 0;
               WaitForTime.hour = 0;
               WaitForTime.minute = 0;
               WaitForTime.second = 0;

               if(hMainWnd && IsWindow(hMainWnd))
               {
                  if(!QuietFlag)
                  {
                     PrintErrorMessage(514);
                  }

               }

               break;
            }
         }

      } while(waiting_flag);

      waiting_flag = FALSE;

      GlobalFreePtr(lpArgs);
      return(FALSE);             /* so far, so good... */

   }

               /** I am waiting 'FOR' **/

   _dos_gettime(&dtt);  /* get current time */

   if(atotime(lp1, (LPSFTTIME)&WaitForTime))
   {
      dwNSec = 0;

      for(lp2=lp1; *lp2; lp2++)
      {
         if(*lp2<'0' || *lp2>'9')
         {
            PrintErrorMessage(679);
            PrintString(lp1);
            PrintString(pQCRLFLF);

            GlobalFreePtr(lpArgs);
            return(TRUE);
         }

         dwNSec = dwNSec * 10L + *lp2 - '0';
      }
          /* user specified total number of seconds! */

      WaitForTime.second = (char)(dwNSec % 60);
      dwNSec /= 60;
      WaitForTime.minute = (char)(dwNSec % 60);
      dwNSec /= 60;
      WaitForTime.hour   = (char)dwNSec;

   }

   if(WaitForTime.second==0 && WaitForTime.minute==0
      && WaitForTime.hour==0)
   {
      PrintErrorMessage(681);

      GlobalFreePtr(lpArgs);
      return(TRUE);
   }


   if((WaitForTime.second += dtt.second)>=60)
   {
      WaitForTime.second -= 60;
      WaitForTime.minute ++;
   }

   if((WaitForTime.minute += dtt.minute)>=60)
   {
      WaitForTime.minute -= 60;
      WaitForTime.hour ++;
   }

   if((WaitForTime.hour += dtt.hour)>=24)
   {
      WaitForTime.hour -= 24;
   }

   if(WaitForTime.hour>=24)  /* STILL!!!!??? */
   {
      PrintErrorMessage(682);

      GlobalFreePtr(lpArgs);
      return(TRUE);
   }

   if(!QuietFlag)
   {
      PrintErrorMessage(846);  // "\r\nWAITING FOR "
      PrintString(lp1);
      PrintErrorMessage(845);  // " ..."
   }




          /* this section (below) stolen from message loop */

   _dos_gettime(&dtt);

   t0.hour = dtt.hour;
   t0.minute = dtt.minute;
   t0.second = dtt.second;

   if(WaitForTime.hour==0 && dtt.hour>0)
   {
      WaitForTime.hour = 24;/* this places us *before* the time! */
   }

   WaitForTimeFlag = timecmp((LPSFTTIME)&WaitForTime, (LPSFTTIME)&t0);

   waiting_flag = TRUE;             // for STATUS dialog!

   do
   {
      if(LoopDispatch() || ctrl_break)
      {
         if(hMainWnd && IsWindow(hMainWnd))
         {
            PrintErrorMessage(513);

         }

         waiting_flag = FALSE;
         break;
      }

      if(WaitForDay.year!=0 && WaitForDay.month!=0
         && WaitForDay.day!=0)
      {
         _dos_getdate(&ddt);

         if(WaitForDay.year>ddt.year ||
            WaitForDay.month>ddt.month ||
            WaitForDay.day>ddt.day)
         {
            continue;             /* didn't pass test - keep waiting */
         }
         else if(WaitForDay.year<ddt.year ||
                 WaitForDay.month<ddt.month ||
                 WaitForDay.day<ddt.day)
         {
            waiting_flag = FALSE;     /* I'm done now! */

            WaitForDay.year = 0;
            WaitForDay.month = 0;
            WaitForDay.day = 0;
            WaitForTime.hour = 0;
            WaitForTime.minute = 0;
            WaitForTime.second = 0;

            if(hMainWnd && IsWindow(hMainWnd))
            {
               if(!QuietFlag)
               {
                  PrintErrorMessage(514);
               }

            }

            break;
         }

      }

      if(WaitForTimeFlag==0)         /* initially compare times */
      {
         _dos_gettime(&dtt);

         t0.hour = dtt.hour;
         t0.minute = dtt.minute;
         t0.second = dtt.second;

         /** Special section to handle '0' hours!   **/
         /** if not executed at least once per hour **/
         /** this algorithm won't work.. not likely **/

         if(WaitForTime.hour==0 && dtt.hour>0)
         {
            WaitForTime.hour = 24;/* this places us *before* the time! */
         }

         WaitForTimeFlag = timecmp((LPSFTTIME)&WaitForTime, (LPSFTTIME)&t0);
      }
      else
      {
         _dos_gettime(&dtt);

         t0.hour = dtt.hour;
         t0.minute = dtt.minute;
         t0.second = dtt.second;

         /** Special section to handle '0' hours!   **/
         /** if not executed at least once per hour **/
         /** this algorithm won't work.. not likely **/

         if(WaitForTime.hour==24 && dtt.hour==0)
         {
            WaitForTime.hour = 0;  /* corrects for '0:0:0' problems */
         }

         if(WaitForTimeFlag!=timecmp((LPSFTTIME)&WaitForTime, (LPSFTTIME)&t0))
         {
            if(WaitForTimeFlag<0)
            {
               WaitForTimeFlag = timecmp((LPSFTTIME)&WaitForTime,
                                         (LPSFTTIME)&t0);
               if(WaitForTimeFlag)
                  continue;               /* low to high transition */
                                         /* i.e. crossing midnight */
            }

               /** Since high to low transition (or equal) is valid **/
               /** for stop times, I can now breathe easily.  I am  **/
               /** now FINISHED WAITING!                            **/

            waiting_flag = FALSE;     /* I'm done now! */

            WaitForDay.year = 0;
            WaitForDay.month = 0;
            WaitForDay.day = 0;
            WaitForTime.hour = 0;
            WaitForTime.minute = 0;
            WaitForTime.second = 0;

            if(hMainWnd && IsWindow(hMainWnd))
            {
               if(!QuietFlag)
               {
                  PrintErrorMessage(514);
               }

            }

            break;
         }
      }

   } while(waiting_flag);

   waiting_flag = FALSE;

   GlobalFreePtr(lpArgs);
   return(FALSE);             /* and now I'm done */

}
