/***************************************************************************/
/*                                                                         */
/*   WINCMD_S.C - Command line interpreter for Microsoft(r) Windows (tm)   */
/*                                                                         */
/*                      TRADE SECRET - CONFIDENTIAL                        */
/*                                                                         */
/*             The compiled libraries and API header files are             */
/*          (c) Copyright 1990-1995 by Stewart~Frazier Tools, Inc          */
/*                                                                         */
/*   This contains functions related to screen I/O                         */
/*                                                                         */
/***************************************************************************/


#define THE_WORKS
#include "mywin.h"

#include "mth.h"
#include "wincmd.h"         /* 'dependency' related definitions */
#include "wincmd_f.h"       /* function prototypes and variables */



#ifdef WIN32

#define _fmemmove(X,Y,Z) memmove(X,Y,Z)
#define _ihstrlen(X) lstrlen(X)

#else  // WIN32


// The following WIN32 definitions are needed by some WIN16 code


#define STARTF_USESHOWWINDOW    0x00000001
#define STARTF_USESIZE          0x00000002
#define STARTF_USEPOSITION      0x00000004
#define STARTF_USECOUNTCHARS    0x00000008
#define STARTF_USEFILLATTRIBUTE 0x00000010
#define STARTF_RUNFULLSCREEN    0x00000020  // ignored for non-x86 platforms
#define STARTF_FORCEONFEEDBACK  0x00000040
#define STARTF_FORCEOFFFEEDBACK 0x00000080
#define STARTF_USESTDHANDLES    0x00000100

#define DEBUG_PROCESS               0x00000001
#define DEBUG_ONLY_THIS_PROCESS     0x00000002

#define CREATE_SUSPENDED            0x00000004

#define DETACHED_PROCESS            0x00000008

#define CREATE_NEW_CONSOLE          0x00000010

#define NORMAL_PRIORITY_CLASS       0x00000020
#define IDLE_PRIORITY_CLASS         0x00000040
#define HIGH_PRIORITY_CLASS         0x00000080
#define REALTIME_PRIORITY_CLASS     0x00000100

#define STATUS_PENDING              ((DWORD   )0x00000103L)
#define STILL_ACTIVE                STATUS_PENDING


#define CREATE_NEW_PROCESS_GROUP    0x00000200
#define CREATE_UNICODE_ENVIRONMENT  0x00000400

#define CREATE_SEPARATE_WOW_VDM     0x00000800
#define CREATE_SHARED_WOW_VDM       0x00001000

#define CREATE_DEFAULT_ERROR_MODE   0x04000000
#define CREATE_NO_WINDOW            0x08000000

#define PROFILE_USER                0x10000000
#define PROFILE_KERNEL              0x20000000
#define PROFILE_SERVER              0x40000000

#define PROCESS_QUERY_INFORMATION 0x400 /* from WINNT.H */

#define THREAD_PRIORITY_LOWEST          THREAD_BASE_PRIORITY_MIN
#define THREAD_PRIORITY_BELOW_NORMAL    (THREAD_PRIORITY_LOWEST+1)
#define THREAD_PRIORITY_NORMAL          0
#define THREAD_PRIORITY_HIGHEST         THREAD_BASE_PRIORITY_MAX
#define THREAD_PRIORITY_ABOVE_NORMAL    (THREAD_PRIORITY_HIGHEST-1)
#define THREAD_PRIORITY_ERROR_RETURN    (MAXLONG)

#define THREAD_PRIORITY_TIME_CRITICAL   THREAD_BASE_PRIORITY_LOWRT
#define THREAD_PRIORITY_IDLE            THREAD_BASE_PRIORITY_IDLE


#define STD_INPUT_HANDLE    (DWORD)-10    /* for GetStdHandle() */
#define STD_OUTPUT_HANDLE   (DWORD)-11    /* for GetStdHandle() */
#define STD_ERROR_HANDLE      (DWORD)-12  /* for GetStdHandle() */

typedef enum _WOW_HANDLE_TYPE { /* WOW */
   WOW_TYPE_HWND,
   WOW_TYPE_HMENU,
   WOW_TYPE_HDWP,
   WOW_TYPE_HDROP,
   WOW_TYPE_HDC,
   WOW_TYPE_HFONT,
   WOW_TYPE_HMETAFILE,
   WOW_TYPE_HRGN,
   WOW_TYPE_HBITMAP,
   WOW_TYPE_HBRUSH,
   WOW_TYPE_HPALETTE,
   WOW_TYPE_HPEN,
   WOW_TYPE_HACCEL,
   WOW_TYPE_HTASK,
   WOW_TYPE_FULLHWND
} WOW_HANDLE_TYPE;



// Additional 'WIN16-only' stuff

#if defined(M_I86LM)

  #pragma message("** Large Model In Use - Single Instance Only!! **")

#elif !defined(M_I86MM)

  #error  "** Must use MEDIUM model to compile, silly!! **"

#else

  #pragma message("** Medium Model Compile - Multiple Instances OK **")

#endif


#pragma intrinsic(_fstrcpy,_fstrcmp,_fstrcat,_fstrlen)

#pragma optimize("g",off)    // disables global sub-expression optimization


static __inline void __far * _fmemmove(void __far *dest, void __far *src,
                                       size_t count )
{
   // ASSUME NO SELECTOR 'OVERLAP' EXISTS - ever, ever, ever!

   if(count==0) return(dest);

   if(SELECTOROF(dest)!=SELECTOROF(src) || OFFSETOF(dest)<=OFFSETOF(src))
   {
      return(_fmemcpy(dest, src, count));
   }

   _asm
   {
      push ds
      push es
      push si
      push di

      std            // direction flag 'reverse' order!
      mov cx, count

      lds si, src
      add si, cx
      dec si         // src + count - 1

      les di, src
      add di, cx
      dec di         // dest + count - 1

      rep movsb      // moves one byte at a time, starting at 'src+count-1'
                     // to 'dest+count-1' for 'count' repeats

      cld            // as a convention, clear direction flag on exit

      pop di
      pop si
      pop es
      pop ds
   }

   return(dest);
}



static __inline DWORD PASCAL _ihstrlen(const char __huge *hp1)
{
auto DWORD dw1;
extern BOOL _ok_to_use_386_instructions_;


   if(_ok_to_use_386_instructions_)
   {
      _asm
      {
         _emit(0x66) _asm push di      /* push EDI */
         push es

         _emit(0x66) _asm xor di, di   /* mov edi, 0 */

         les di, hp1

         cld
         _emit(0x66) _asm xor cx,cx    /* mov ecx, 0 */
         _emit(0x66) _asm dec cx       /* dec ecx */

         mov al, 0
         _emit(0xf2) _asm _emit(0x67) _asm _emit(0xae)
                                       /* repnz scas BYTE PTR es:[edi] */

         _emit(0x66) _asm dec di       // always increments edi once too many
         _emit(0x66) _asm mov WORD PTR dw1, di
                                       /* mov dw1, EDI */

         pop es
         _emit(0x66) _asm pop di       /* pop EDI */
      }
   }
   else
   {
      dw1 = 0;

      _asm
      {
         push es
         push di

         les di, hp1
         cld
         mov cx, 0
         mov al,0

         repnz scasb
         dec di
         mov WORD PTR dw1, di

         nop

         pop di
         pop es
      }
   }

   return(dw1 - OFFSETOF(hp1));
}


#pragma optimize("",on)  /* restore default optimizations */

#endif // WIN32




/***************************************************************************/
/*               PRINTING TO THE SCREEN and PROMPT support                 */
/***************************************************************************/

//#pragma alloc_text (PRINT_TEXT,PrintAChar,NearPrintAChar,AnsiCommand,
//                               CheckScreenScroll,PrintString,
//                               ProcessAttribCode,PrintBuffer,
//                               PrintErrorMessage)

#pragma code_seg("PRINT_TEXT","CODE")


__inline BOOL NEAR _fastcall NearPrintChars(LPCSTR lpBuf, int count);
__inline BOOL NEAR _fastcall AnsiCommand(int chr, int *is_ansi);

BOOL NEAR _fastcall CheckScreenScroll(BOOL PaintFlag);



void FAR PASCAL ReDirectOutput(PARSE_INFO FAR *lpParseInfo, BOOL redir_flag)
{
   if(!lpParseInfo)
   {
     redirect_output = HFILE_ERROR;
     redirect_output_DC = NULL;
   }
   else
   {
      if(!redir_flag)
      {
         if(lpParseInfo->hOutFile != HFILE_ERROR)
         {
            if(lpParseInfo->hOutFile == HFILE_USE_DC && redirect_output_DC)
            {
               Escape(redirect_output_DC, NEWFRAME, 0, NULL, NULL);
               Escape(redirect_output_DC, ENDDOC, 0, NULL, NULL);

               DeleteDC(redirect_output_DC);
            }
            else
            {
               _lclose(lpParseInfo->hOutFile);
            }

            lpParseInfo->hOutFile = HFILE_ERROR;
         }
         else if(lpParseInfo->sbpPipeNext &&  // error in piping?
                 (!lpParseInfo->sbpOutFile || !*lpParseInfo->sbpOutFile))
         {
            lpParseInfo->sbpPipeNext = NULL;
            PipingFlag = 0;

            MessageBox(hMainWnd, "Unable to complete 'piping' - missing input file!",
                       "* SFTShell COMMAND ERROR *", MB_OK | MB_ICONHAND);
         }

         redirect_output = HFILE_ERROR;
         redirect_output_DC = NULL;

         // at this point it's safe to read from the previous output file
         // if I'm doing piping at all.  So, "force" the issue by tacking
         // the old file name onto the command line with a '<' preceding it

         if(lpParseInfo->sbpPipeNext)
         {
          LPSTR lp1, lp2, lp3;

            if(*lpParseInfo->sbpPipeNext)
            {
               lp1 = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE,
                                           lstrlen(lpParseInfo->sbpPipeNext) +
                                           lstrlen(lpParseInfo->sbpOutFile) + 4);

               if(!lp1)
               {
                  MessageBox(hMainWnd, "Unable to complete 'piping' - not enough memory!",
                             "* SFTShell COMMAND ERROR *", MB_OK | MB_ICONHAND);
               }


               // isolate the COMMAND portion of the pipe, and copy it to the
               // destination buffer 'lp1'.

               for(lp2=(LPSTR)lpParseInfo->sbpPipeNext, lp3=lp2;
                   *lp3 > ' ' && *lp3 != '/'; lp3++)
                  ; // find end of command

               _hmemcpy(lp1, lp2, lp3 - lp2);
               lp1[lp3 - lp2] = 0;


               // concatenate the input file onto the command line, but
               // right at the beginning of it.  This ensures that correct
               // re-direction will occur unless there's something wrong
               // with the command line itself.

               lstrcat(lp1, " <");
               lstrcat(lp1, (LPSTR)lpParseInfo->sbpOutFile);


               // find next non-white-space in command line and append
               // the remainder of the string onto the new command line.

               while(*lp3 && *lp3 <= ' ') lp3++;
               if(*lp3)
               {
                  lstrcat(lp1, " ");
                  lstrcat(lp1, lp3);
               }

               // next, execute the program using the newly build command line

               PipingFlag++;

               ProcessCommand(lp1);  // execute newly build command!!!

               if(PipingFlag) PipingFlag--;

               GlobalFreePtr(lp1);
            }
            else  // just a '|' without a command - just type the output!
            {
               CMDType(NULL, (LPSTR)lpParseInfo->sbpOutFile);
                                      // type the output to the screen *NOW*!
            }

            MyUnlink((LPSTR)lpParseInfo->sbpOutFile);
                                 // delete the 'pipe' file (assume temporary)
         }
      }
      else
      {
         if(lpParseInfo->hOutFile == HFILE_ERROR)  // not already open...
         {
            if(lpParseInfo->sbpPipeNext)  // implicitly create output file!
            {
               if(!lpParseInfo->sbpOutFile) // *I* shall allow BOTH '>' and '|'
               {
                  lpParseInfo->sbpOutFile = lpParseInfo->sbpPipeNext
                                          + lstrlen(lpParseInfo->sbpPipeNext)
                                          + 4;  // account for '<' (above)

                  // create temp file name using 'lpParseInfo' selector

                  GetTempFileName(0, "SFT", 0, (LPSTR)lpParseInfo->sbpOutFile);
               }
               else if(IsValidPortName((LPSTR)lpParseInfo->sbpOutFile))
               {
                  // OOPS!  User is trying to pipe through PRN, and that's a NO-NO!

                  // TODO:  add error message here.  port name is ignored

                  lpParseInfo->sbpOutFile = lpParseInfo->sbpPipeNext
                                          + lstrlen(lpParseInfo->sbpPipeNext)
                                          + 4;  // account for '<' (above)

                  // create temp file name using 'lpParseInfo' selector

                  GetTempFileName(0, "SFT", 0, (LPSTR)lpParseInfo->sbpOutFile);
               }


               if((lpParseInfo->hOutFile=_lcreat(lpParseInfo->sbpOutFile, 0))
                  != HFILE_ERROR)
               {
                  _lclose(lpParseInfo->hOutFile);
               }

               if((lpParseInfo->hOutFile =
                   _lopen(lpParseInfo->sbpOutFile,
                          OF_WRITE | OF_SHARE_EXCLUSIVE))
                  != HFILE_ERROR)
               {
                  if(IsCONDevice(lpParseInfo->hOutFile))
                  {
                     _lclose(lpParseInfo->hOutFile);
                     lpParseInfo->hOutFile = HFILE_ERROR;
                     redirect_output = HFILE_ERROR;
                  }
                  else
                  {
                     redirect_output = lpParseInfo->hOutFile;
                  }

               }
            }
            else if(lpParseInfo->sbpOutFile
                    && *(lpParseInfo->sbpOutFile)
                    && ((*(lpParseInfo->sbpOutFile) != '>' &&
                         IsValidPortName((LPSTR)lpParseInfo->sbpOutFile)) ||
                        (*(lpParseInfo->sbpOutFile) == '>'&&
                         IsValidPortName((LPSTR)(++lpParseInfo->sbpOutFile)))))
            {
               // OUTPUT TO 'PRN' DEVICE....  oooooohhhhhh!


               redirect_output_DC =
                     CreatePrinterDCFromPort((LPSTR)lpParseInfo->sbpOutFile);

               if(redirect_output_DC)
               {
                static const char CODE_BASED szRedirPRN[]="SFTShell$REDIR:PRN";

                  Escape(redirect_output_DC, STARTDOC,
                         lstrlen(szRedirPRN), szRedirPRN, NULL);

                  redirect_output = HFILE_USE_DC;

                  lpParseInfo->hOutFile = HFILE_USE_DC;
               }
            }
            else if(lpParseInfo->sbpOutFile
                    && *(lpParseInfo->sbpOutFile))
            {
             BOOL bAppendFlag = 0;


               // create the file (if needed), close it, and open exclusive

               if(*lpParseInfo->sbpOutFile == '>')
               {
                  lpParseInfo->sbpOutFile++;

                  if(FileExists(lpParseInfo->sbpOutFile))
                  {
                     bAppendFlag = 1;
                  }
               }

               if(!bAppendFlag &&
                  (lpParseInfo->hOutFile=_lcreat(lpParseInfo->sbpOutFile, 0))
                  != HFILE_ERROR)
               {
                  _lclose(lpParseInfo->hOutFile);
               }


               if((lpParseInfo->hOutFile =
                    _lopen(lpParseInfo->sbpOutFile,
                            OF_WRITE | OF_SHARE_EXCLUSIVE)) != HFILE_ERROR)
               {
                  if(IsCONDevice(lpParseInfo->hOutFile))
                  {
                     _lclose(lpParseInfo->hOutFile);
                     lpParseInfo->hOutFile = HFILE_ERROR;
                     redirect_output = HFILE_ERROR;
                  }
                  else
                  {
                     redirect_output = lpParseInfo->hOutFile;

                     if(bAppendFlag)
                     {
                        _llseek(lpParseInfo->hOutFile, 0, SEEK_END);
                     }
                  }
               }
               else
               {
                  lpParseInfo->hOutFile = HFILE_ERROR;  /* essentially marks stdout */
                  redirect_output = HFILE_ERROR;        /* and carry over error here too */
               }
            }
         }
         else
         {
            redirect_output = lpParseInfo->hOutFile;
                                           /* file already open - why not? */
         }
      }
   }
}


WORD FAR PASCAL GetUserInput(LPSTR lpBuf)
{
   *lpBuf = 0;
   return(GetUserInput2(lpBuf));
}

WORD FAR PASCAL GetUserInput2(LPSTR lpBuf)
{
LPSTR lpBuf2;
WORD lin,col;


   if(redirect_input != HFILE_ERROR)  // re-directing input!!!
   {
      lpBuf2 = lpBuf;

      while(_lread(redirect_input, lpBuf2, 1) == 1)
      {
         if((*lpBuf2 == '\n' &&
             (lpBuf2 == lpBuf || *(lpBuf2-1)=='\r')) ||
            (*lpBuf2 == '\r' &&
             (lpBuf2 == lpBuf || *(lpBuf2-1)=='\n')) ||
            *lpBuf2 == 26)
         {
            lpBuf2++;
            break;
         }


         lpBuf2++;  // read 1 char at a time until <CR><LF> found!
                    // let MS-DOS manage all of the buffering in this case
      }

      *lpBuf2 = 0;

      return(lstrlen(lpBuf));  // a blank line will equate to EOF
   }


   GettingUserInput = TRUE;  /* this lets everyone know I'm doing this */

   lpBuf2 = lpBuf;


     /** If there is "Type Ahead" force contents into 'cmd_buf' first **/
     /** or just copy into 'lpBuf' and exit if a '\r' is found...     **/

   if(TypeAheadHead!=TypeAheadTail)
   {
      lin = curline;
      col = curcol;

      while(TypeAheadHead!=TypeAheadTail)
      {
         if(TypeAheadHead>=sizeof(TypeAhead)) TypeAheadHead=0;

         PrintAChar(*(lpBuf2++) = TypeAhead[TypeAheadHead]);

         *lpBuf2 = 0;  /* ensure it's end of string at this point */

         if(TypeAhead[TypeAheadHead++]=='\r')
         {
            *(lpBuf2++) = '\n';  /* input line has <CR><LF> attached */
            PrintAChar('\n');
            *lpBuf2 = 0;             /* string is now terminated */

            if(TypeAheadHead>=sizeof(TypeAhead)) TypeAheadHead=0;

            *UserInputBuffer = 0;   /* just in case, zero out the buffer!! */
            GettingUserInput = FALSE;    /* turn off input - I'm done now! */
            return(lstrlen(lpBuf));    /* total number of bytes - awright! */
         }

         if(TypeAheadHead>=sizeof(TypeAhead)) TypeAheadHead=0;
      }

      UpdateWindow(hMainWnd);  /* make sure the input is updated! */

              /* at this point only part of a line is there */
          /* now, copy over to the 'cmd_buf' where I can edit it */

      lstrcpy(cmd_buf, lpBuf);  /* copy into the command buffer */
      cmd_buf_len = lstrlen(cmd_buf);

      start_line = lin;
      start_col  = col;
   }
   else
   {
      start_line = curline;
      start_col  = curcol;

//      *cmd_buf = 0;
//      cmd_buf_len = 0;

      lstrcpy(cmd_buf, lpBuf);         // 'lpBuf' contains default input
      cmd_buf_len = lstrlen(cmd_buf);

      if(*cmd_buf)
      {
         ReDisplayUserInput0(cmd_buf);
      }
   }


   while(GettingUserInput      /* this is a 'volatile' global variable */
         && !ctrl_break)
   {
      if(LoopDispatch())  /* a QUIT message, by the way... */
      {
         *lpBuf = 26;
         *(lpBuf + 1) = 0;
         return(1);       /* forces a CTRL-Z */
      }
   }

   GettingUserInput = FALSE;       /* in case I got here from ctrl_break */
   if(ctrl_break)
   {
      *lpBuf = 0;                  /* blast the input out of the water */
      *cmd_buf = 0;                // *AND* clear this out, too!
      cmd_buf_len = 0;
   }

   lstrcpy(lpBuf, UserInputBuffer);   /* copy from buffer to buffer... */

   *UserInputBuffer = 0;           /* just in case, zero out the buffer!! */

   return(lstrlen(lpBuf));           /* total number of bytes - awright! */

}




#define NearPrintAChar(chr) NearPrintChars(NULL, chr)

//typedef char __based(lpScreen) *PSCREEN_BASED_CHAR;
typedef char FAR *PSCREEN_BASED_CHAR;

__inline BOOL NEAR _fastcall NearPrintChars(LPCSTR lpBuf, int count)
{
extern BOOL _ok_to_use_386_instructions_;
static BOOL is_ansi=FALSE;
LPCSTR lpc1;
LPSTR lp1;
int i, chr, ctr;
BOOL bCheckScreenScroll = FALSE;
PSCREEN_BASED_CHAR pScreen0, pAttrib0;




   if(!lpBuf)
   {
      chr = count;
      if(redirect_output != HFILE_ERROR)
      {
         if(redirect_output == HFILE_USE_DC)
         {
            char t[3];
            t[2] = chr;
            *((WORD FAR *)t) = 1;

            return(!Escape(redirect_output_DC, PASSTHROUGH, 0, t, NULL));
         }
         else
         {
            return(MyWrite(redirect_output, (LPSTR)&chr, 1)!=1);
         }
      }
   }
   else
   {
      chr = 0;

      if(redirect_output == HFILE_USE_DC)
      {
         BOOL result;

         if(count)  ctr = count;
         else       ctr = lstrlen(lpBuf);

         lp1 = (LPSTR)GlobalAllocPtr(GPTR, ctr + sizeof(WORD) + 1);
         if(!lp1)
         {
            return(TRUE); // error
         }

         _hmemcpy(lp1 + sizeof(WORD), lpBuf, ctr);
         lp1[ctr + sizeof(WORD)] = 0;

         *((WORD FAR *)lp1) = ctr;

         result = !Escape(redirect_output_DC, PASSTHROUGH, 0, lp1, NULL);

         GlobalFreePtr(lp1);

         return(result);
      }
      else if(redirect_output != HFILE_ERROR)
      {
         if(count)  ctr = count;
         else       ctr = lstrlen(lpBuf);

         return((int)MyWrite(redirect_output, lpBuf, ctr) != ctr);
      }
   }

   if(CheckScreenScroll(FALSE))
     return(TRUE);

   if(lpBuf)
   {
      if(_ok_to_use_386_instructions_)
      {
         GlobalLock(GetCodeHandle((FARPROC)_hmemcpy));
         GlobalLock(GetCodeHandle((FARPROC)_hmemset));

         _asm mov ax, WORD PTR lpBuf + 2
         _asm _emit(0x8e) _asm _emit(0xe0)         // mov FS, ax
      }

      ctr = 0;

      lpc1 = lpBuf;
   }


   pScreen0 = (PSCREEN_BASED_CHAR)&(pScreen[curline][curcol]);
   pAttrib0 = (PSCREEN_BASED_CHAR)&(pAttrib[curline][curcol]);

   do
   {
      if(lpBuf)
      {
         if(count)
         {
            if(ctr >= count) break;
            ctr++;
         }

         if(_ok_to_use_386_instructions_)
         {
//            // temporary
//
//            _asm _emit(0x8c)    _asm _emit(0xe0)         // mov ax, FS
//                                _asm cmp ax, WORD PTR lpBuf + 2
//                                _asm jz seg_ok
//
//                                _asm mov ax, WORD PTR lpBuf + 2
//            _asm _emit(0x8e)    _asm _emit(0xe0)         // mov FS, ax
//
//         seg_ok:
//
//            // end temporary section

                                _asm mov bx, WORD PTR lpc1
            _asm _emit(0x64)    _asm mov al, [bx]          // mov al, fs:[bx]
                                _asm mov BYTE PTR chr, al

            lpc1++;
         }
         else
         {
            chr = *(lpc1++);
         }

         if(!count && !chr) break;
      }


      if(is_ansi)
      {
         if(AnsiCommand(chr, &is_ansi)) return(TRUE);

         if(_ok_to_use_386_instructions_)
         {
            _asm mov ax, WORD PTR lpBuf + 2
            _asm _emit(0x8e) _asm _emit(0xe0)         // mov FS, ax
         }

         pScreen0 = (PSCREEN_BASED_CHAR)&(pScreen[curline][curcol]);
         pAttrib0 = (PSCREEN_BASED_CHAR)&(pAttrib[curline][curcol]);

      }
      else if(chr >= ' ')
      {
         // an attempt at optimization

         *(pScreen0++) = (char)chr;
         *(pAttrib0++) = (char)cur_attr;

         curcol++;
      }
      else switch(chr)
      {
         case 0x0d:   /* CR */

            for(i=curcol; i<SCREEN_COLS; i++)
            {
               *(pAttrib0++) = (char)cur_attr;  /* extend current attribute! */
            }

            curcol = 0;

            pScreen0 = (PSCREEN_BASED_CHAR)&(pScreen[curline][curcol]);
            pAttrib0 = (PSCREEN_BASED_CHAR)&(pAttrib[curline][curcol]);

            break;


         case 0x0a:   /* LF */

            curline++;

            pScreen0 = (PSCREEN_BASED_CHAR)&(pScreen[curline][curcol]);
            pAttrib0 = (PSCREEN_BASED_CHAR)&(pAttrib[curline][curcol]);

            bCheckScreenScroll = TRUE;      /* always invalidate on <LF> */
            break;


         case 0x1b:   /* ESC */

            if(AnsiCommand(chr, &is_ansi)) return(TRUE);

            if(_ok_to_use_386_instructions_)
            {
               _asm mov ax, WORD PTR lpBuf + 2
               _asm _emit(0x8e) _asm _emit(0xe0)         // mov FS, ax
            }

            pScreen0 = (PSCREEN_BASED_CHAR)&(pScreen[curline][curcol]);
            pAttrib0 = (PSCREEN_BASED_CHAR)&(pAttrib[curline][curcol]);

            break;


         case 0x08:   /* BackSpace */

            curcol--;

               /* later, make it behave like DELETE except for previous char */
               /* for now, however, it just deletes the previous char and    */
               /* does not 'pull back' anything to the right of it.          */

            *(--pScreen0) = 0;
            *(--pAttrib0) = (char)cur_attr;
            break;


         case 0x09:   /* TAB */

            i = curcol;

            curcol += 8;
            curcol &= 0xfff8;     /* move up to next column of '8' */

            while(i < curcol)     // fill in characters I skipped over...
            {
               if(i >= SCREEN_COLS)
               {
                  if(CheckScreenScroll(TRUE)) return(FALSE);

                  bCheckScreenScroll = FALSE;

                  i -= SCREEN_COLS;

                  pScreen0 = (PSCREEN_BASED_CHAR)&(pScreen[curline][i]);
                  pAttrib0 = (PSCREEN_BASED_CHAR)&(pAttrib[curline][i]);
               }

               *(pScreen0++) = 0;
               *(pAttrib0++) = (char)cur_attr;

               i++;
            }

            break;


         case 0xff:   /* DEL */

            if(end_line<curline || (end_line==curline && end_col<curcol))
               break;                         /* in this case, just ignore it */

            if(end_line>curline || end_col>curcol)
            {
               _hmemcpy((void FAR *)&(pScreen[curline][curcol]),
                        (void FAR *)&(pScreen[curline][curcol + 1]),
                        ((end_line - curline) * SCREEN_COLS
                         + end_col - curcol - 1) * sizeof(pScreen[0][0]));

               _hmemcpy((void FAR *)&(pAttrib[curline][curcol]),
                        (void FAR *)&(pAttrib[curline][curcol + 1]),
                        ((end_line - curline) * SCREEN_COLS
                         + end_col - curcol - 1) * sizeof(pAttrib[0][0]));
            }

            *pScreen0 = 0;
            *pAttrib0 = (char)cur_attr;

            bCheckScreenScroll = TRUE;  /* invalidate whole screen */
            break;


         default:
            *(pScreen0++) = (char)chr;
            *(pAttrib0++) = (char)cur_attr;

            curcol++;

      }

      if(curline >= SCREEN_LINES || curcol >= SCREEN_COLS)
      {
         if(CheckScreenScroll(TRUE)) return(TRUE);

         bCheckScreenScroll = FALSE;

         pScreen0 = (PSCREEN_BASED_CHAR)&(pScreen[curline][curcol]);
         pAttrib0 = (PSCREEN_BASED_CHAR)&(pAttrib[curline][curcol]);
      }


   } while(lpBuf);


   if(lpBuf && _ok_to_use_386_instructions_)
   {
      _asm mov ax, 0
      _asm _emit(0x8e) _asm _emit(0xe0)         // mov FS, ax

      // on exit, FS will have a zero in it (safer)

      GlobalUnlock(GetCodeHandle((FARPROC)_hmemcpy));
      GlobalUnlock(GetCodeHandle((FARPROC)_hmemset));
   }


   return(CheckScreenScroll(bCheckScreenScroll));

}


BOOL NEAR _fastcall AnsiCommand(int chr, int *is_ansi)
{
static char ansi_buf[64]={0};
static int cursorx=0, cursory=0;  /* for saving/retrieving cursor position */
int i,j,l;
char _near *np1, *np2;



   if(!(*is_ansi))  /* we haven't done anything before!  1st entry */
   {
      memset(ansi_buf, 0, sizeof(ansi_buf));

      if(chr!=0x1b) return(TRUE);  /* this means it was called in error */

      *ansi_buf = (char)chr;
      *is_ansi  = TRUE;
      return(FALSE);       /* no error! */
   }

   l = strlen(ansi_buf);

   if(l==1 && chr!='[')  /* only '<ESC>[' sequences are supported... */
   {
      *is_ansi = FALSE;
      return(TRUE);      /* ANSI decoding error!! */
   }

   if(l>1 && (chr<'0' || chr>'9') && chr!=';')  /* end of sequence!! */
   {
      np1 = ansi_buf + 2;     /* points past '<ESC>[' */
      *is_ansi = FALSE;       /* turn off the 'ansi' switch */

      switch(chr)
      {
         case 'H':  /* move cursor absolute */
         case 'f':

            if(!(np2 = strchr(np1, ';')))
               return(TRUE);

            *np2++ = 0;
            return(MoveCursor(atoi(np1),atoi(np2)));


         case 'A':  /* relative move up */

            return(MoveCursor(curcol, curline - atoi(np1)));

         case 'B':  /* relative move down */

            return(MoveCursor(curcol, curline + atoi(np1)));

         case 'C':  /* relative move right */

            return(MoveCursor(curcol + atoi(np1), curline));

         case 'D':  /* relative move left */

            return(MoveCursor(curcol - atoi(np1), curline));

         case 's':  /* save cursor position */

            cursorx = curcol;
            cursory = curline;
            return(FALSE);        /* good result! */

         case 'u':  /* restore cursor position */

            return(MoveCursor(cursorx, cursory));

         case 'J':  /* clear the screen? */

            if(l!=3 || *np1!='2')   return(TRUE);
            else
            {
               ClearScreen();
               return(CheckScreenScroll(TRUE));
            }

         case 'K':  /* erase to EOL */

            if(l!=2) return(TRUE);
            else
            {
               for(i=curcol; i<SCREEN_COLS; i++)
               {
                  pScreen[curline][i] = 0;
                  pAttrib[curline][i] = (char)cur_attr;
               }

               return(CheckScreenScroll(TRUE));  /* ANSI always re-paints */
            }


         case 'm':      /* SCREEN COLORS AND OTHER ATTRIBUTES!! */

            for(np2=np1; *np2!=0; np2++)
            {
               if(*np2==';')
               {
                  *np2 = 0;

                  if(np1==np2)  j=0;
                  else          j = atoi(np1);

                  np1 = np2 + 1;

                  if(ProcessAttribCode(j)) return(TRUE);  /* error!! */

               }
            }

            if(np1 < np2)
            {
               return(ProcessAttribCode(atoi(np1)));
            }
            else
            {
               return(FALSE);        /* result is GOOD */
            }

         default:
            return(TRUE);       /* ANSI error - unsupported function! */

      }
   }
   else
   {
      ansi_buf[l] = (char)chr;
   }

   return(FALSE);               /* GOOD result!! */

}


BOOL NEAR _fastcall ProcessAttribCode(int code)
{
   if(code==0 || code==1)
   {
      if(code==0) cur_attr &= 0xf7;  /* dull (boring?) text */
      else        cur_attr |= 0x08;  /* bright (smart!) text */
   }
   else if(code>=30 && code<=37)
   {
      cur_attr &= 0xf8;
      cur_attr |= (code - 30);
   }
   else if(code>=40 && code<=47)
   {
      cur_attr &= 0x8f;
      cur_attr |= (code - 40)<<4;
   }
   else if(code!=4 && code!=5 && code!=7 && code!=8)
      return(TRUE);      /* non-valid ANSI attrib code */

   return(FALSE);
}


BOOL FAR PASCAL FarCheckScreenScroll(BOOL PaintFlag)
{
   return(CheckScreenScroll(PaintFlag));
}


BOOL NEAR _fastcall CheckScreenScroll(BOOL PaintFlag)
{
RECT r;
BOOL LineChange=FALSE;
int actual_text_height, actual_text_width, vpos, hpos,
    lines_per_screen, columns_per_screen;


   LineChange = FALSE;          /* indicates the line # has *NOT* changed! */

   if(curcol>=SCREEN_COLS)
   {
      curline += curcol / SCREEN_COLS;
      curcol %= SCREEN_COLS;

      LineChange = TRUE;
   }
   else if(curcol<0)
   {
      if(curline>0)
      {
         curline += curcol / SCREEN_COLS;
         curcol %= SCREEN_COLS;

         curcol += SCREEN_COLS;
         curline --;

         LineChange=TRUE;
      }
      else
         curcol = 0;
   }


   if(curline>=SCREEN_LINES)           /*** SCROLL THE SCREEN ***/
   {
      _hmemcpy((void FAR *)pScreen,
               (void FAR *)(((char FAR *)pScreen) + SCREEN_COLS),
               (SCREEN_LINES - 1) * sizeof(pScreen[0][0]) * SCREEN_COLS);

      _hmemcpy((void FAR *)pAttrib,
               (void FAR *)(((char FAR *)pAttrib) + SCREEN_COLS),
               (SCREEN_LINES - 1) * sizeof(pAttrib[0][0]) * SCREEN_COLS);

      _hmemset((void FAR *)(pScreen[SCREEN_LINES - 1]), 0,
               sizeof(pScreen[0][0]) * SCREEN_COLS);

      _hmemset((void FAR *)(pAttrib[SCREEN_LINES - 1]), cur_attr,
               sizeof(pAttrib[0][0]) * SCREEN_COLS);

      start_line--;
      end_line--;

      if(LineScrollCount != 0x8000)
      {
         LineScrollCount += curline - (SCREEN_LINES - 1);  // for PAINTING
      }

      curline = SCREEN_LINES - 1;
      maxlines = curline;

      LineChange = TRUE;
   }
   else if(curline<0)
   {
      curline = 0;

      _hmemset((void FAR *)pScreen, 0,
               sizeof(pScreen[0][0]) * SCREEN_COLS);
      _hmemset((void FAR *)pAttrib, cur_attr,
               sizeof(pAttrib[0][0]) * SCREEN_COLS);

      LineScrollCount = 0x8000;  // guarantees complete screen re-paint

      LineChange = TRUE;
   }

   if(maxlines<curline)
   {
      // 'LineScrollCount' will be taken care of elsewhere

      maxlines = curline;
   }


   if(InvalidateFlag) return(FALSE);           /* window already 'invalid' */

           /* since we've made changes, invalidate client rect. */

   if(PaintFlag || LineChange || !curcol || !TMFlag)
   {                                     /* forces update to ENTIRE window */

      InvalidateRect(hMainWnd, NULL, FALSE);

      InvalidateFlag = TRUE;

   }
   else  /* if line not change & not force paint, invalidate cur char only */
   {
      vpos = GetScrollPos(hMainWnd, SB_VERT);
      hpos = GetScrollPos(hMainWnd, SB_HORZ);

      GetClientRect(hMainWnd, (LPRECT)&r);

      if(!TMFlag)
        UpdateTextMetrics(NULL);
        
      actual_text_height = tmMain.tmHeight + tmMain.tmExternalLeading;
      lines_per_screen = (r.bottom - r.top - 1) / actual_text_height;

      actual_text_width = tmMain.tmAveCharWidth;
      columns_per_screen  = (r.right - r.left) / actual_text_width;

         /** NOTE: using 'curcol-1' because I *JUST* updated it! **/
         /**       for reverse motion I invalidate entire window **/

      if((curcol > 0 &&
          (vpos + lines_per_screen)>curline && vpos<=curline &&
          (hpos + columns_per_screen)>(curcol - 1) && hpos<(curcol - 1)) ||
         (curcol == 0 &&
          (vpos + lines_per_screen)>curline && vpos<=curline &&
          (hpos == 0 || (hpos + columns_per_screen) > (SCREEN_COLS - 1))) )
      {
         r.top += (curline - vpos) * actual_text_height;
         r.bottom = r.top + actual_text_height;

         r.left += (curcol - hpos - 1) * actual_text_width;
         r.right = r.left + actual_text_width;

         InvalidateRect(hMainWnd, (LPRECT)&r, FALSE);
      }
   }

              /* return FALSE indicating 'no problems here' */

   return(FALSE);
}



BOOL FAR PASCAL PrintAChar(int chr)
{
   return(NearPrintAChar(chr));
}


BOOL FAR PASCAL PrintBuffer(LPCSTR lpBuf, WORD wCount)
{
//WORD cnt;
//LPCSTR lp1;


   if(!wCount) return(FALSE);

   if(NearPrintChars(lpBuf, wCount)) return(TRUE);

//   for(lp1 = lpBuf, cnt=0; cnt<wCount; cnt++, lp1++)
//   {
//      if(NearPrintAChar(*lp1)) return(TRUE);
//   }

   return(CheckScreenScroll(TRUE));

}


void FAR PASCAL PrintErrorMessage(WORD wMessageID)
{
LPSTR lpString;
static char pFatal[]="?Unable to load MESSAGE string - forcing CTRL-BREAK!\r\n\n";


   lpString = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE, 256); // 256 bytes for error buf
            // 256 bytes is the maximum string length allowed by the
            // RC complier for a 'STRINGTABLE' string entry.

   if(!lpString)
   {
      GlobalCompact(512);

      lpString = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE, 256); // 256 bytes for error buf
      if(!lpString)
      {
         PrintString(pFatal);
         ctrl_break = TRUE;  // this forces operations to abort!
         return;
      }
   }

   if(!LoadString(hInst, wMessageID, lpString, 256))
   {
      GlobalFreePtr(lpString);

      PrintString(pFatal);
      ctrl_break = TRUE;  // this forces operations to abort!
      return;
   }


   PrintString(lpString);

   GlobalFreePtr(lpString);

}


BOOL FAR PASCAL PrintString(LPCSTR lpString)
{
//LPCSTR lp1;
BOOL rval;


   rval = NearPrintChars(lpString, 0);

//   for(lp1=lpString; *lp1!=0; lp1++)
//   {
//      if((rval = NearPrintAChar((int)*lp1))!=0)  break;
//   }

   return(rval || CheckScreenScroll(TRUE));
}


BOOL FAR PASCAL PrintStringX(LPCSTR lpString) // incorporate CTRL-S and MORE
{
LPCSTR lp1;
BOOL rval;
WORD wLineCount, actual_text_height, lines_per_screen;
int oldcurline;
RECT rct;
DWORD dwTickCount;


   if(!TMFlag)
      UpdateTextMetrics(NULL);

        
   actual_text_height = tmMain.tmHeight + tmMain.tmExternalLeading;

   GetClientRect(hMainWnd, &rct);
   lines_per_screen = (rct.bottom - rct.top - 1) / actual_text_height;

   wLineCount = 0;
   dwTickCount = GetTickCount();

   for(lp1=lpString; !ctrl_break && *lp1!=0; lp1++)
   {
      if((rval = NearPrintAChar((int)*lp1))!=0)  break;

      if(*lp1 == '\n')  // newline character - check line count!
      {
         // this section stolen from parts of 'CMDDirProcessPause()'
         // and 'CMDDirCheckBreakAndStall()'

         if(ctrl_break) break;

         if(redirect_output == HFILE_ERROR)
         {
            while(stall_output && !ctrl_break)
            {
               MthreadSleep(100);  // sleep .1 sec until output no longer stalled!

               if(!ctrl_break) ctrl_break = LoopDispatch();

               dwTickCount = GetTickCount();
            }
         }

         if(ctrl_break) stall_output = FALSE;

         if((++wLineCount) >= (lines_per_screen - 1))
         {
            InvalidateRect(hMainWnd, NULL, TRUE);

            oldcurline = curline;

            PrintErrorMessage(796); // "Press <ENTER> to continue:  "
            pausing_flag = TRUE;    // assign global flag TRUE to cause pause

            while(pausing_flag)
            {
               if(LoopDispatch())   // returns TRUE on WM_QUIT
               {
                  PrintErrorMessage(635);

                  ctrl_break=TRUE;  // fakes things out for abnormal termination
                  break;
               }
            }

            curline = oldcurline;
            PrintErrorMessage(751); // "\r\033[K"
                                    // deletes to end of line from left column
            wLineCount = 0;

            dwTickCount = GetTickCount();
         }
         else if((dwTickCount + 50) < GetTickCount())
         {
            if(LoopDispatch())
            {
               ctrl_break = TRUE;    // abnormal terminate
            }
            else
            {
               dwTickCount = GetTickCount();
            }
         }
      }

      if(ctrl_break)
      {
         rval = TRUE;
         break;
      }
   }

   return(rval || CheckScreenScroll(wLineCount != 0));
}


void FAR PASCAL ReDisplayUserInput0(LPCSTR lpcBuffer)
{
int i, j, oldcurline, oldcurcol, oldstartline;
LPCSTR lpc1;


   for(i=start_line; i<=end_line; i++)
   {
      if(i==start_line && i==end_line)
      {
         for(j=start_col; j<=end_col; j++)
         {
            pScreen[i][j] = 0;
         }
      }
      else if(i==start_line)
      {
         for(j=start_col; j<SCREEN_COLS; j++)
         {
            pScreen[i][j] = 0;
         }
      }
      else if(i==end_line)
      {
         for(j=0; j<end_col; j++)
         {
            pScreen[i][j] = 0;
         }
      }
      else
      {
         for(j=0; j<SCREEN_COLS; j++)
         {
            pScreen[i][j] = 0;
         }
      }
   }

   oldcurline = curline;
   oldcurcol = curcol;

   oldstartline = start_line;

   curline = start_line;
   curcol = start_col;


//   PrintString(lpcBuffer);  // the code (below) was stolen from 'PrintString'

   for(lpc1=lpcBuffer; *lpc1!=0; lpc1++)
   {
      // 'ABSOLUTE' display mode (in case of ctrl-p!)

      if(NearPrintAChar((int)*lpc1 | 0x8000))  break;
   }

// end of 'PrintString()' extract


   end_line = curline;    /* YES!!! print the new command! */
   end_col  = curcol;

   if(oldstartline != start_line)  // did I scroll the screen?
   {
      oldcurline -= oldstartline - start_line;
   }

   if(oldcurline > end_line ||
      (oldcurline == end_line && oldcurcol > end_col))
   {
      curline = end_line;
      curcol = end_col;
   }
   else if(oldcurline < start_line ||
           (oldcurline == start_line && oldcurcol < start_col))
   {
      curline = start_line;
      curcol = start_col;
   }
   else
   {
      curline = oldcurline;
      curcol = oldcurcol;
   }


   CheckScreenScroll(FALSE);
}


void FAR PASCAL ReDisplayUserInput(LPCSTR lpcBuffer)
{
int i, j;


   for(i=start_line; i<=end_line; i++)
   {
      if(i==start_line && i==end_line)
      {
         for(j=start_col; j<=end_col; j++)
         {
            pScreen[i][j] = 0;
         }
      }
      else if(i==start_line)
      {
         for(j=start_col; j<SCREEN_COLS; j++)
         {
            pScreen[i][j] = 0;
         }
      }
      else if(i==end_line)
      {
         for(j=0; j<end_col; j++)
         {
            pScreen[i][j] = 0;
         }
      }
      else
      {
         for(j=0; j<SCREEN_COLS; j++)
         {
            pScreen[i][j] = 0;
         }
      }
   }

   curline = start_line;
   curcol = start_col;

   PrintString(lpcBuffer);

   end_line = curline;    /* YES!!! print the new command! */
   end_col  = curcol;
}


BOOL FAR PASCAL DisplayPrompt(void)
{                                  /* $e,$$,$g,$t,$d,$_ are supported here */
static struct dosdate_t ddt;
static struct dostime_t dtt;
static char _near *days[]={"Sunday","Monday","Tuesday","Wednesday",
                           "Thursday","Friday","Saturday"};
static char _near *months[]={"January","February","March","April","May",
                             "June","July","August","September","October",
                             "November","December"};
LPSTR lpTempBuf;
LPSTR lpPrompt, lp1;
int i;
BOOL rval;
WIN_VER uVer;

#define TEMP_BUF_SIZE 4096  /* size big enough for just about any prompt */



   lpPrompt = GetEnvString("PROMPT");       /* get 'prompt' env variable */

   if(lpPrompt==(LPSTR)NULL)  lpPrompt="$p$g";   /* default prompt!! */


   lpTempBuf = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT, TEMP_BUF_SIZE);
   if(!lpTempBuf)
   {
      return(PrintString("?Not enough memory to display 'PROMPT'\r\n>"));
   }



   for(i=0, lp1=lpPrompt; *lp1!=0; lp1++)
   {
      if(*lp1!='$')  lpTempBuf[i++] = *lp1;
      else
      {
         lp1++;

         switch(*lp1)
         {
            case '$':
               lpTempBuf[i++] = '$';
               break;

            case '_':
               lpTempBuf[i++] = '\r';
               lpTempBuf[i++] = '\n';
               break;

            case 'e':
            case 'E':
               lpTempBuf[i++] = 0x1b;   /* escape */
               break;

            case 'g':
            case 'G':
               lpTempBuf[i++] = '>';
               break;

            case 'l':
            case 'L':
               lpTempBuf[i++] = '<';
               break;


            case 'q':
            case 'Q':
               lpTempBuf[i++] = '=';
               break;


            case 'b':
            case 'B':
               lpTempBuf[i++] = '|';
               break;


            case 'h':          /** BACKSPACE! **/
            case 'H':
               lpTempBuf[i++] = '\x8';
               break;


            case 'n':         /* current drive (letter only) */
            case 'N':
               lpTempBuf[i++] = (char)('A' - 1 + _getdrive());
               break;


            case 'p':   /* path!! */
            case 'P':

               if(!_lgetdcwd(0, lpTempBuf + i, TEMP_BUF_SIZE - i - 1))
               {
                  lstrcpy(lpTempBuf + i, "* ERR *");
                  i+= 7;
               }
               else
               {
                  if(IsChicago || IsNT)  // bug fix for SUBST drives
                  {
                     GetLongName(lpTempBuf + i, lpTempBuf + i);
                  }

                  i = lstrlen(lpTempBuf);  /* re-sync string length */
               }

               break;

            case 'd':
            case 'D':

               _dos_getdate(&ddt);

               i += wsprintf(((LPSTR)lpTempBuf)+i,"%s, %s %d, %d",
                             (LPSTR)days[ddt.dayofweek],
                             (LPSTR)months[ddt.month - 1],
                             ddt.day, ddt.year);
               break;

            case 't':
            case 'T':

               _dos_gettime(&dtt);

               i += wsprintf(((LPSTR)lpTempBuf)+i,"%02d:%02d:%02d.%02d",
                             dtt.hour, dtt.minute, dtt.second, dtt.hsecond);
               break;


            case 'v':    /** MS-DOS Version **/
            case 'V':
               uVer.dw = GetVersion();

               i += wsprintf(((LPSTR)lpTempBuf)+i,
                             "MS-DOS Version %d.%02d, "
                             "Windows Version %d.%02d",
                             uVer.ver.dos_major, uVer.ver.dos_minor,
                             uVer.ver.win_major, uVer.ver.win_minor);


               break;

            default:

               lpTempBuf[i++] = '$';
               lpTempBuf[i++] = *lp1;
         }
      }

      lpTempBuf[i] = 0;  // ensure string remains properly terminated
   }

   rval = PrintString(lpTempBuf);

   GlobalFreePtr(lpTempBuf);


   end_line = start_line = curline;  /* set up current pos for input!! */
   end_col  = start_col  = curcol;

   return(rval);
}



BOOL FAR PASCAL MoveCursor(int x, int y)
{
RECT r;
int actual_text_height, actual_text_width, vpos, hpos,
    lines_per_screen, columns_per_screen;


   if(x>=SCREEN_COLS)  x = SCREEN_COLS - 1;
   if(x<0)             x = 0;

   if(y>=SCREEN_LINES) y = SCREEN_LINES - 1;
   if(y<0)             y = 0;


   // if the old 'curline' and 'curcol' were within the boundary
   // of the screen, *AND* the new 'curline' and 'curcol' are
   // also within the boundary of the screen, just invalidate their
   // respective character positions.  That's all...


   vpos = GetScrollPos(hMainWnd, SB_VERT);
   hpos = GetScrollPos(hMainWnd, SB_HORZ);

   GetClientRect(hMainWnd, (LPRECT)&r);

   if(!TMFlag)
     UpdateTextMetrics(NULL);
        
   actual_text_height = tmMain.tmHeight + tmMain.tmExternalLeading;
   lines_per_screen = (r.bottom - r.top - 1) / actual_text_height;

   actual_text_width = tmMain.tmAveCharWidth;
   columns_per_screen  = (r.right - r.left) / actual_text_width;

      /** NOTE: using 'curcol-1' because I *JUST* updated it! **/
      /**       for reverse motion I invalidate entire window **/

   if((vpos + lines_per_screen)   > curline  && vpos <= curline &&
      (hpos + columns_per_screen) > curcol   && hpos <= curcol  &&
      (vpos + lines_per_screen)   > y        && vpos <= y       &&
      (hpos + columns_per_screen) > x        && hpos <= x )
   {
      r.top += (curline - vpos) * actual_text_height;
      r.bottom = r.top + actual_text_height;

      r.left += (curcol - hpos) * actual_text_width;
      r.right = r.left + actual_text_width;

      InvalidateRect(hMainWnd, (LPRECT)&r, FALSE);


      r.top += (y - vpos) * actual_text_height;
      r.bottom = r.top + actual_text_height;

      r.left += (x - hpos) * actual_text_width;
      r.right = r.left + actual_text_width;

      InvalidateRect(hMainWnd, (LPRECT)&r, FALSE);
   }
   else
   {
      InvalidateRect(hMainWnd, NULL, FALSE);  // invalidates whole thing!
   }


   curline = y;
   curcol  = x;

   if(maxlines<curline)  maxlines = curline;

//           /* since we've made changes, invalidate client rect. */
//
//   GetClientRect(hMainWnd, (LPRECT)&r);
//   InvalidateRect(hMainWnd, (LPRECT)&r, FALSE);

              /* return FALSE indicating 'no problems here' */

   return(FALSE);

}


BOOL FAR PASCAL ClearScreen(void)
{
   _hmemset((void FAR *)pScreen, 0,
            sizeof(pScreen[0][0]) * SCREEN_LINES * SCREEN_COLS);
                                                  /* zero xlates as space */

   _hmemset((void FAR *)pAttrib, cur_attr,
            sizeof(pAttrib[0][0]) * SCREEN_LINES * SCREEN_COLS);

   maxlines = 0;
   return(MoveCursor(0,0));
}

#pragma code_seg()






