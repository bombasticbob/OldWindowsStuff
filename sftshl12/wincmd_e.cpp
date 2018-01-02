/***************************************************************************/
/*                                                                         */
/*   WINCMD_E.C - Command line interpreter for Microsoft(r) Windows (tm)   */
/*                                                                         */
/*                      TRADE SECRET - CONFIDENTIAL                        */
/*                                                                         */
/*             The compiled libraries and API header files are             */
/*          (c) Copyright 1990-1995 by Stewart~Frazier Tools, Inc          */
/*                                                                         */
/*   This contains functions related to environment variables/substitution */
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



BOOL NEAR PASCAL NearDelEnvString(LPCSTR lpName);

LPSTR NEAR PASCAL FindEnvString(LPCSTR lpName);

LPSTR NEAR PASCAL NearGetMyEnv(void);

BOOL NEAR PASCAL SetMyEnvironment(LPSTR lpNewEnv);





/***************************************************************************/
/*                    ENVIRONMENT PROCESSING FUNCTIONS                     */
/***************************************************************************/




#pragma code_seg("ENVIRON_TEXT","CODE")

static const BYTE CODE_BASED UpperCase[] =
   {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
    0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,
    0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,
    0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,
    0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
    0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,
    0x60,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,
    0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x7b,0x7c,0x7d,0x7e,0x7f,
    0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
    0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
    0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
    0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
    0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
    0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,
    0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,
    0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff};


typedef struct {
   char FirstTwo[2];
   WORD wOffset;
   } ENV_VAR_ENTRY, FAR *LPENV_VAR_ENTRY;



extern "C" LPENV_VAR_ENTRY lpEnvVarEntry=NULL;

static LPSTR lpEnvEnd=NULL;


void FreeEnvVarEntry(void)
{
  if(lpEnvVarEntry)
    GlobalFreePtr(lpEnvVarEntry);
}

LPSTR FAR PASCAL GetEnvString(LPCSTR lpName)
{
LPSTR lpEnvStr;

   if(!(lpEnvStr = FindEnvString(lpName)))
      return(NULL);
   else
      return(lpEnvStr + _ihstrlen(lpName) + 1);
                               /* return a pointer *Just past* the '=' */

}

static LPSTR NEAR PASCAL GetEnvEnd(LPSTR lpEnv)
{
LPSTR lpEnd;
LPENV_VAR_ENTRY lpE, lpE2;
WORD wSize;
BOOL bBuildEnvVarEntry = FALSE;




   if(lpEnvEnd && lpEnvVarEntry && SELECTOROF(lpEnvEnd)==SELECTOROF(lpEnv))
   {
      return(lpEnvEnd);
   }

   if(!lpEnvVarEntry)
   {
      lpEnd = lpEnv;

      lpEnvVarEntry = (LPENV_VAR_ENTRY)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT, 0x4000);

      if(lpEnvVarEntry) bBuildEnvVarEntry = TRUE;
   }


   if(bBuildEnvVarEntry)
   {
      wSize = (WORD)GlobalSizePtr(lpEnvVarEntry);
      lpEnd = lpEnv;

      lpE = lpEnvVarEntry;

      while(*lpEnd)
      {
         lpE->FirstTwo[0] = UpperCase[lpEnd[0]];
         if(lpEnd[1]=='=') lpE->FirstTwo[1] = 0;
         else              lpE->FirstTwo[1] = UpperCase[lpEnd[1]];

         lpE->wOffset = lpEnd - lpEnv;
         lpE++;

         lpE->FirstTwo[0] = 0;
         lpE->FirstTwo[1] = 0;
         lpE->wOffset = 0;

         if((WORD)(((LPSTR)lpE) - ((LPSTR)lpEnvVarEntry) + 64) >= wSize)
         {
            wSize = (((LPSTR)lpE) - ((LPSTR)lpEnvVarEntry) + 0x1fff) & 0xf000;

            lpE2 = (LPENV_VAR_ENTRY)
                      GlobalReAllocPtr(lpEnvVarEntry, wSize, GMEM_MOVEABLE);

            if(!lpE2)
            {
               GlobalFreePtr(lpEnvVarEntry);
               lpEnvVarEntry = NULL;

               bBuildEnvVarEntry = FALSE;
               break;
            }

            lpE = (LPENV_VAR_ENTRY)
                 ( (((LPSTR)lpE) - ((LPSTR)lpEnvVarEntry)) + ((LPSTR)lpE2) );

            lpEnvVarEntry = lpE2;
         }


         _asm
         {
            push es
            push di

            les di, lpEnd
            cld
            mov al, 0
            mov cx, 0xffff

            repnz scasb     // find the terminating NULL character!

            mov WORD PTR lpEnd, di  // save pointer just past NULL byte here
            pop di
            pop es
         }
      }

      if(bBuildEnvVarEntry)   // no error, so return!
      {
         lpEnvEnd = lpEnd;

         return(lpEnd);
      }
   }


   _asm
   {
      push es
      push di
      les di, lpEnv

      mov cx, 0xffff
      cld
      mov al, 0

main_loop:

      repnz scasb      // scan string for zero byte - when found, points
                       // 'just past' the zero (to the one following it)
      mov ah, es:[di]  // is this pointing to another ZERO or not??

      or ah,ah
      jnz main_loop    // if not, loop until I find 2 zeros in a row...

      mov WORD PTR lpEnd, di
      mov WORD PTR lpEnd+2, es

      pop di
      pop es

   }

   lpEnvEnd = lpEnd;

   return(lpEnd);
}


BOOL FAR PASCAL SetEnvString(LPCSTR lpName, LPCSTR lpValue)
{
LPSTR lpEnd, lpEnv;
int l1,l2,l3,l4;
HSZ hsz;
LPENV_VAR_ENTRY lpE, lpE2;



   if(!(lpEnv = NearGetMyEnv()) || NearDelEnvString(lpName))
      return(TRUE);

   l1 = (int)_ihstrlen(lpName);
   l2 = (int)_ihstrlen(lpValue);

   if(l2!=0)          /* non-zero length - new string has data attached */
   {

      l3 = l1 + l2 + 2;    /* space for 2 strings, '=', and the terminating 0 */


      lpEnd = GetEnvEnd(lpEnv);


      l4 = (WORD)(lpEnd - lpEnv);     /* offset to the end of the environment */

      if((WORD)(l3 + l4 + 16)>=env_size)
      {
         if(!(lpEnv = (LPSTR)GlobalReAllocPtr(lpEnv, l3 + l4 + 16, ENV_REALLOC_FLAGS)))
         {
            return(TRUE);
         }

         SetMyEnvironment(lpEnv);   /* reset to new value!! */
         lpEnd = lpEnv + l4;        /* assign new value to 'lpEnd' also */
      }

      env_size = (WORD)GlobalSizePtr(lpEnv);

      _ihmemcpy(lpEnd, lpName, l1 + 1);         // 'l1' is length of 'lpName'
      _fstrupr(lpEnd);                          // convert name to upper case

      lpEnd[l1] = '=';
      _ihmemcpy(lpEnd + l1 + 1, lpValue, l2 + 1); // 'l2' is len of 'lpValue'

//      _ihmemset(lpEnd + l3, 0, env_size - l3 - l4);
//                                              /* fill remainder with nulls */

      // OPTIMIZATION STUFF...

      lpEnvEnd = lpEnd + l1 + 1 + l2 + 1;

      *lpEnvEnd = 0;                       // maximum of 4 NULL bytes only!
      if((lpEnv + env_size) > (lpEnvEnd + 1)) lpEnvEnd[1] = 0;
      if((lpEnv + env_size) > (lpEnvEnd + 2)) lpEnvEnd[2] = 0;
      if((lpEnv + env_size) > (lpEnvEnd + 3)) lpEnvEnd[3] = 0;


      if(lpEnvVarEntry)
      {
         lpE = lpEnvVarEntry;

         while(lpE->FirstTwo[0]) lpE++;

         if(GlobalSizePtr(lpEnvVarEntry) <
            (WORD)(((LPSTR)lpE) - ((LPSTR)lpEnvVarEntry)) + 64)
         {
            lpE2 = (LPENV_VAR_ENTRY)
               GlobalReAllocPtr(lpEnvVarEntry,
                                (((LPSTR)lpE - (LPSTR)lpEnvVarEntry) + 64
                                  + 0x1fff) & 0xf000L,
                                GMEM_MOVEABLE);

            if(!lpE2)
            {
               GlobalFreePtr(lpEnvVarEntry);
               lpEnvVarEntry = NULL;

               lpE = NULL;
            }
            else
            {
               lpE = (LPENV_VAR_ENTRY)
                   (((LPSTR)lpE2) + (((LPSTR)lpE) - ((LPSTR)lpEnvVarEntry)));

               lpEnvVarEntry = lpE2;
            }
         }

         if(lpE)
         {
            lpE->FirstTwo[0] = UpperCase[lpName[0]];
            lpE->FirstTwo[1] = UpperCase[lpName[1]];

            lpE->wOffset = lpEnd - lpEnv;

            lpE++;

            lpE->FirstTwo[0] = 0;
            lpE->FirstTwo[1] = 0;
            lpE->wOffset = 0;
         }

      }
   }

   if(idDDEInst && ItemHasHotLink(lpName))
   {
      hsz = lpDdeCreateStringHandle(idDDEInst, lpName, CP_WINANSI);

      if(hsz)
      {
         lpDdePostAdvise(idDDEInst, NULL, hsz);
      }

      lpDdeFreeStringHandle(idDDEInst, hsz);

   }

   return(FALSE);

}


BOOL FAR PASCAL DelEnvString(LPCSTR lpName)
{
BOOL rval;
HSZ hsz;


   rval = NearDelEnvString(lpName);

   if(idDDEInst && ItemHasHotLink(lpName))
   {
      hsz = lpDdeCreateStringHandle(idDDEInst, lpName, CP_WINANSI);

      if(hsz)
      {
         lpDdePostAdvise(idDDEInst, NULL, hsz);
      }

      lpDdeFreeStringHandle(idDDEInst, hsz);
   }

   return(rval);

}


BOOL NEAR PASCAL NearDelEnvString(LPCSTR lpName)
{
LPSTR lpEnv, lpEnvStr;
LPENV_VAR_ENTRY lpE, lpE2;
int l;


   if(!(lpEnv = NearGetMyEnv()))  return(TRUE);  /* error! */

   if(!(lpEnvStr = FindEnvString(lpName)))
   {
      return(FALSE);                           /* ok to not exist already! */
   }

   l = (int)_ihstrlen(lpEnvStr) + 1;   /* get length of this environment string */
                                       /* including the NULL byte at the end.   */

    /* shrink environment space by copying remainder on top of this string */

   if(!lpEnvEnd) env_size = GetEnvEnd(lpEnv) - lpEnv;
   else          env_size = lpEnvEnd - lpEnv; // just to the end of environment


   if((lpEnvStr + l) < (lpEnv + env_size))
   {
      _ihmemcpy(lpEnvStr,               /* beginning of string to delete */
                lpEnvStr + l,           /* beginning of next string      */
                (lpEnv + env_size) - (lpEnvStr + l));
   }


   if(lpEnvVarEntry)
   {
      lpE = lpEnvVarEntry;

      while(lpE->FirstTwo[0] && lpE->wOffset != (WORD)(lpEnvStr - lpEnv))
         lpE++;

      if(lpE->wOffset != (WORD)(lpEnvStr - lpEnv))  // not found (?)
      {
         GlobalFreePtr(lpEnvVarEntry);

         lpEnvVarEntry = NULL;                // safety valve...
      }
      else
      {
         lpE2 = lpE + 1;

         while(lpE2->FirstTwo[0])
         {
            (lpE2 - 1)->FirstTwo[0] = lpE2->FirstTwo[0];
            (lpE2 - 1)->FirstTwo[1] = lpE2->FirstTwo[1];

            (lpE2 - 1)->wOffset = lpE2->wOffset - l; // adjust for old var...

            lpE2++;
         }

         lpE2--;

         lpE2->FirstTwo[0] = 0;
         lpE2->FirstTwo[1] = 0;
         lpE2->wOffset = 0;
      }
   }

   if(!lpEnvEnd) GetEnvEnd(lpEnv);
   else          lpEnvEnd -= l;  // new position of end of environment...


   // zero out where the last environment string *WAS*

   _ihmemset(lpEnvEnd, 0, l);


   return(FALSE);  /* it worked o.k. */
}



LPSTR NEAR PASCAL FindEnvString(LPCSTR lpName)
{
auto LPSTR lpRval;
LPCSTR lp1, lp2;
LPENV_VAR_ENTRY lpE;


   if(!lpName || !*lpName) return(NULL);             // not found!

   if(!(lpRval = NearGetMyEnv())) return((LPSTR)NULL);  /* (not found) */

   if(lpEnvVarEntry)
   {
      for(lpE = lpEnvVarEntry; lpE->FirstTwo[0]; lpE++)
      {
         if(lpE->FirstTwo[0]==UpperCase[lpName[0]] &&
            lpE->FirstTwo[1]==UpperCase[lpName[1]])
         {
            lp1 = lpRval + lpE->wOffset;
            lp2 = lpName;

            while(*lp1 && *lp2)
            {
               if(UpperCase[*lp1]!=UpperCase[*lp2]) break;

               lp1++;
               lp2++;
            }

            if(!*lp2 && *lp1=='=') // at the end of the string, 'lp2' must point
            {                      // to '=', so string matches "NAME=" with 'NAME'

               return((LPSTR)lpRval + lpE->wOffset);
            }

         }
      }

      return(NULL);  // nothing found!
   }


   // no 'helper' array - do it the "old fashioned" (?) way

   _asm
   {
      push ds
      push es
      push si
      push di
      push bx

      lds si, lpName
      les di, lpRval
      jmp name_compare

loop_head:

      mov si, WORD PTR lpName

      // compare 'name' strings.  If equal, break out

name_compare:

      mov bh, 0
      mov bl, ds:[si]
      or bl,bl
      jz name_compare_end          // zero byte found

      mov al, cs:UpperCase[bx]     // convert name char to upper case

      mov bl, es:[di]
      or bl,bl
      jz name_compare_end          // zero byte found

      cmp al, cs:UpperCase[bx]     // compare to env string as upper case
      jnz name_compare_end

      inc si
      inc di
      jmp name_compare


name_compare_end:

      mov al, ds:[si]
      or al,al
      jnz next_env_string

      mov al, es:[di]
      cmp al, '='
      jz found_env_string


next_env_string:

      cld
      mov cx, 0xffff

      mov al, 0
      repnz scasb              // stops 1 char past the next NULL BYTE

      mov WORD PTR lpRval, di  // save ptr to NEXT environment string
                               // so that when I return, it will contain
                               // the correct pointer already
      mov al, es:[di]
      or al,al
      jnz loop_head            // continue with remaining env strings


//env_string_not_found:

      // not found - return NULL

      mov WORD PTR lpRval, 0
      mov WORD PTR lpRval + 2, 0


found_env_string:

      pop bx
      pop di
      pop si
      pop es
      pop ds
   }


   return(lpRval);
}



static __inline LPCSTR PASCAL NextEnvString(LPCSTR lpCur)
{
LPCSTR lpRval = lpCur;

//      while(*lpRval) lpRval++;  // increment until a NULL byte is found
//      lpRval++;                 // increment ONE MORE TIME!

   _asm
   {
      mov bx, es
      push di
      cld
      mov cx, 0xffff

      les di, lpRval
      mov al, 0

      repnz scasb        // stops 1 char past the next NULL BYTE

      mov WORD PTR lpRval, di

      pop di
      mov es, bx

   }

   return(lpRval);
}



static __inline int NEAR __fastcall my_toupper(int _c)
{
   return( (islower(_c)) ? _toupper(_c) : (_c) );
}



LPSTR NEAR PASCAL OldFindEnvString(LPCSTR lpName)
{
LPCSTR lp1, lp2, lp3;
LPSTR lpEnv;


   if(!(lpEnv = NearGetMyEnv())) return((LPSTR)NULL);  /* (not found) */

   lp2 = lpEnv;

   while(*lp2)
   {
      lp1 = lp2;
      lp3 = lpName;

      while(*lp2 && *lp3)
      {
         if(my_toupper(*lp2)!=my_toupper(*lp3)) break;

         lp2++;
         lp3++;
      }

      if(!*lp3 && *lp2=='=') // at the end of the string, 'lp2' must point
      {                      // to '=', so string matches "NAME=" with 'NAME'
         return((LPSTR)lp1);
      }

      lp2 = NextEnvString(lp2);

   }

   return((LPSTR)NULL);  /* not found - return NULL */

}





LPSTR FAR PASCAL GetMyEnvironment(void)
{
   return(NearGetMyEnv());
}


LPSTR NEAR PASCAL NearGetMyEnv(void)
{
LPSTR lp1, lpOldEnv, lpNewEnv;
WORD new_env_size;


   if(lpMyEnv==(LPSTR)NULL)  /* first time it's been called... */
   {

      if(!(lpOldEnv = GetDOSEnvironment()))  return((LPSTR)NULL);

      for(lp1=lpOldEnv; *lp1!=0; lp1+=_ihstrlen(lp1)+1)
         ;                               /* read through until NULL found! */


      env_size = (WORD)(lp1 - lpOldEnv);        /* get size of environment */
                                               /* and include the last 0's */

      if(!(lpNewEnv = (LPSTR)GlobalAllocPtr(ENV_MEM_FLAGS, env_size + 16)))
         return((LPSTR)NULL);            /* alloc a little extra for zeros */

      _ihmemcpy(lpNewEnv, lpOldEnv, (WORD)(env_size));


      SetMyEnvironment(lpNewEnv);
      new_env_size = (WORD)GlobalSizePtr(lpNewEnv);

      _ihmemset(lpNewEnv + env_size, 0, new_env_size - env_size);
                             /* ensure the 'trailing' section is all zeros */

      env_size = new_env_size;          /* and assign new environment size */

/*      GlobalPageUnlock((HGLOBAL)(_segment)lpOldEnv);
/*                                        /* this seems to be an issue */
/*      GlobalPageLock((HGLOBAL)(_segment)lpNewEnv);
/*                                         /* maybe this will help?? */
/*
/*      hOldEnv = (HGLOBAL)LOWORD(GlobalHandle((_segment)lpOldEnv));
/*
/*      if(hOldEnv)
/*      {
/*         while(GlobalFlags(hOldEnv) & GMEM_LOCKCOUNT)
/*            GlobalUnlock(hOldEnv);
/*
/*         GlobalFree(hOldEnv);        /* if I can, free it! */
/*      }
*/
   }


   return(lpMyEnv);

}


BOOL NEAR PASCAL SetMyEnvironment(LPSTR lpNewEnv)
{
WORD wPDB;


   if(lpNewEnv!=(LPSTR)NULL)
   {
      lpMyEnv = lpNewEnv;
      env_size = (WORD)GlobalSizePtr(lpMyEnv);

      if(!(wPDB = GetCurrentPDB())) return(TRUE);        /* Get PDB (psp) */

      *((WORD FAR *)MAKELP(wPDB, 0x2c)) = (WORD)((_segment)lpNewEnv);
         /* it is assumed that the new env begins at offset 0 */

      GetDOSEnvironment();       /* just to give O/S a 'kick in the pants' */

      return(FALSE);
   }
   else
      return(TRUE);
}


BOOL FAR PASCAL FreeMyEnvironment(void)
{
   if(lpMyEnv!=(LPSTR)NULL)
      return(GlobalFreePtr(lpMyEnv));
      
   return(FALSE);  // no error
}


static __inline LPSTR NEAR PASCAL EvalStringFindPercent(LPSTR lpSrc)
{
   _asm
   {
      push es
      push di
      les di, lpSrc

      jmp search_loop_next

search_loop:
      cmp al, '%'
      jz search_loop_end

      inc di

search_loop_next:
      mov al,es:[di]
      or al,al
      jnz search_loop


search_loop_end:

      mov WORD PTR lpSrc, di
      pop di
      pop es
   }

   return(lpSrc);
}



LPSTR FAR PASCAL EvalString(LPSTR lpStr)
{
LPSTR lpRval, lp1, lp2, lp3, lp4;
WORD wLen, wStrLen, w, quote_flag;
BOOL Freelp4Flag;
static char nomem_msg[] = "?Not enough memory to evaluate string\r\n";


   lpRval = NULL;  // Use this to flag that 'lpRval' hasn't been allocated...

   lp2 = lpStr;
   wStrLen = 0;

   while(*lp2)
   {
//      lp3 = lp2;
//      while(*lp3 && *lp3!='%') lp3++;

      lp3 = EvalStringFindPercent(lp2);

      if(!*lp3)
      {
         wStrLen = lp3 - lp2;  // this is *ONLY* used here!

         lp3 = NULL;
         break;                 // no more '%'!
      }

      if(!lpRval)  // NOW allocate return buffer!  I found a '%'!
      {
//         lpRval = GlobalAllocPtr(GMEM_MOVEABLE, lstrlen(lpStr) + 512);
         lpRval = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE, _ihstrlen(lpStr) + 512);

         if(!lpRval)
         {
            PrintString(nomem_msg);
            return(NULL);
         }

         wLen = (WORD)GlobalSizePtr(lpRval);

         lp1 = lpRval;
      }

      Freelp4Flag = FALSE;

      w = (WORD)(lp3 - lp2);

      if(w)   /* if 'lp2' wasn't already pointing at the '%' */
      {
         _ihmemcpy(lp1, lp2, w);
         lp1 += w;
      }

      *lp1 = 0;  /* to mark current end of string */

      lp2 = lp3 + 1;  /* just past the '%' */

      if(*lp2 == '%')  /* is it a 'doubled' percent? */
      {
         *lp1++ = *lp2++;  /* copy the '%' one time only */
         *lp1 = 0;         /* mark end of dest string */
         continue;         /* and continue on with searching... */
      }
      else if(*lp2 == '(') // is it an expression????? Hmmm????
      {
       WORD nP = 1;
       LPSTR lpEQ;
       char c1;

         lpEQ = lp3 = lp2 + 1; // ensure it points PAST the '(', not to it!

         quote_flag = 0;

         c1 = *lp3;

         while(c1 && nP)
         {
            if((c1=='"' || c1=='\'') && *(lp3 - 1)!='\\')
            {
               if(quote_flag==(WORD)c1) quote_flag = 0;
               else if(!quote_flag)     quote_flag = (WORD)c1;
            }
            else if(!quote_flag)
            {
               if(c1=='(')      nP++;
               else if(c1==')') nP--;
            }

            if(nP)
            {
               lp3++;  // don't increment on 'final ")"'
               c1 = *lp3;
            }
         }

         if(!c1)
         {
            *(lp1++) = '%';
            *(lp1++) = '(';
            *lp1 = 0;      // terminates string

            lp2 = lpEQ;    // point to remainder of line
            break;         // act like I didn't find a '%'
         }
         else
         {
            *(lp3++) = 0;  // terminates end of equation
            c1 = *lp3;
         }


         if(c1!='%')     // uh, oh - didn't find a ')%'!!
         {
            *(lp1++) = '%';
            *(lp1++) = '(';

            lstrcpy(lp1, lpEQ);
//            lp1 += lstrlen(lp1);
            lp1 += _ihstrlen(lp1);

            if(c1) *(lp1++) = ')';  // the ending ')' on the equation

            *lp1 = 0;

            lp2 = lp3;       // pick up where we 'left off'
            continue;
         }

         // at this point lp3 points to the ending '%' following expression

         lp2 = lp3 + 1;      // remainder of string points just past '%'

         lp3 = EvalString(lpEQ);  // get calculation result!
         if(lp3)
         {
            lp4 = Calculate(lpEQ);
            GlobalFreePtr(lp3);

            lp3 = NULL;
         }
         else
         {
            lp4 = NULL;
         }

         if(!lp4)                // calc error - insert 'error' message
         {
            lp4 = "**CALC ERROR**";
         }
         else
         {
            Freelp4Flag = TRUE;
         }

//         w = lstrlen(lpRval) + lstrlen(lp4) + lstrlen(lp2); /* needed length */
         w = (WORD)(_ihstrlen(lpRval) + _ihstrlen(lp4) + _ihstrlen(lp2));

      }
      else
      {
//         lp3 = lp2;                   /* do it again to get end of arg... */
//         while(*lp3 && *lp3!='%') lp3++;

         lp3 = EvalStringFindPercent(lp2);

         if(*lp3)
         {
            w = (WORD)(lp3 - lp2);
            if(w)
            {
               _ihmemcpy(lp1, lp2, w);    /* temporarily store variable name */
            }

            lp2 = lp3 + 1;                /* point 'lp2' just past the '%' */

            lp1[w] = 0;                   /* end variable name with a 0 byte */

            lp4 = GetEnvString(lp1);      /* gets the environment string */

            *lp1 = 0;                     /* re-null end of 'dest' string */

            if(!lp4)  continue;           /* variable not found - assume blank */

//            w = lstrlen(lpRval) + lstrlen(lp4) + lstrlen(lp2); /* needed length */
            w = (WORD)(_ihstrlen(lpRval) + _ihstrlen(lp4) + _ihstrlen(lp2));

         }
         else
         {
            *lp1++ = '%';     /* put the '%' back - no ending '%'!! */
            break;            /* and exit loop - byby! */
         }
      }

      if((w + 1)>=wLen)        /* I need more memory! */
      {
         lp3 = (LPSTR)GlobalReAllocPtr(lpRval, w + 512, GMEM_MOVEABLE);
         if(!lp3)
         {
            PrintString(nomem_msg);
            GlobalFreePtr(lpRval);

            if(Freelp4Flag) GlobalFreePtr(lp4);  // result from Calculate()

            return((LPSTR)NULL);        /* can't - a - do - it! */
         }

         lpRval = lp3;
//         lp1    = lpRval + lstrlen(lpRval);
         lp1    = lpRval + _ihstrlen(lpRval);
         wLen   = (WORD)GlobalSizePtr(lpRval);
      }

      lstrcpy(lp1, lp4);       /* copy the ENV variable results here */
//      lp1 += lstrlen(lp4);     /* update 'lp1' to point to the end */
      lp1 += _ihstrlen(lp4);     /* update 'lp1' to point to the end */

      *lp1 = 0;                /* just to make sure it happens this way... */

      if(Freelp4Flag) GlobalFreePtr(lp4);  // result from Calculate()
   }


   if(!lpRval)  // If needed, allocate return buffer!  More efficient??
   {
//      lpRval = GlobalAllocPtr(GMEM_MOVEABLE, lstrlen(lpStr) + 2);

      if(!wStrLen) wStrLen = (WORD)_ihstrlen(lp2);

      lpRval = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE, wStrLen + 2);

      if(!lpRval)
      {
         PrintString(nomem_msg);
         return(NULL);
      }

      if(wStrLen) _ihmemcpy(lpRval, lp2, wStrLen);
      lpRval[wStrLen] = 0;
   }
   else
   {
      if(!wStrLen) wStrLen = (WORD)_ihstrlen(lp2);

      if(wStrLen) _ihmemcpy(lp1, lp2, wStrLen);
                               /* copy remainder of string to destination */

      lp1[wStrLen] = 0;
   }

   return(lpRval);     /* I'm done now!  (caller must 'GlobalFreePtr()') */

}


#pragma code_seg()





                    /**********************************/
                    /**                              **/
                    /** ADDITIONAL SUPPORT FUNCTIONS **/
                    /**                              **/
                    /**********************************/


/***************************************************************************/
/*                                                                         */
/*                      EQUATION EVALUATION FUNCTIONS                      */
/*                                                                         */
/***************************************************************************/

long FAR PASCAL MyValue(LPSTR lpText)          /* value of a string (term) */
{
LPSTR lp1, lp2;
WORD l, radix, count;
BOOL sign;
long rval;
int x;
static char digits[]="0123456789ABCDEF";



   lp1 = EvalString(lpText);  /* that's the first step... */

   l = lstrlen(lp1);

   if(l && toupper(lp1[l - 1])=='H')  /* a hex number, we assume... */
   {
      if(GlobalSizePtr(lp1) < (l + 2))  /* make sure there's enough room */
      {
         lp2 = (LPSTR)GlobalReAllocPtr(lp1, l + 2, GMEM_MOVEABLE | GMEM_ZEROINIT);
         if(!lp2)
         {
            GlobalFreePtr(lp1);
            return(0L);                 /* ach, I canno' do it... */
         }
         lp1 = lp2;
      }

      lp1[l - 1] = 0;                      /* trim the 'H' */
      _fmemmove(lp1 + 2, lp1, l);  /* moves all, including NULL byte */

      lp1[0] = '0';
      lp1[1] = 'x';      /* insert a '0x' for 'atoi' */

      l += 2;            /* the new length! */

   }

   rval = 0;


   if(*lp1=='0')           /* octal or hex? */
   {
      if(toupper(lp1[1])=='X')
      {
         radix=16;
         lp2 = lp1 + 2;
      }
      else
      {
         radix=8;
         lp2 = lp1 + 1;
      }

   }
   else
   {
      radix=10;
      lp2 = lp1;
   }

   sign = FALSE;

   if(*lp2=='-')      /* a negative!! */
   {
      sign = TRUE;
      lp2++;
   }
   else if(*lp2=='+')
   {
      lp2++;
   }

   l = lstrlen(lp2);

   for(count=0; count<l; count++, lp2++)
   {
      x = (int)((WORD)strchr(digits, toupper(*lp2)) - (WORD)digits);

      if(x>=(int)radix || x<0) /* not found or outside of radix scope */
      {
         break;       /* found digit outside of scope - bail out! */
      }

      rval = rval * radix + x;
   }

   GlobalFreePtr(lp1);

   if(sign)
      return(-rval);
   else
      return(rval);

}




