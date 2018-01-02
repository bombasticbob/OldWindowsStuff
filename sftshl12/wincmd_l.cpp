/***************************************************************************/
/*                                                                         */
/*   WINCMD_L.C - Command line interpreter for Microsoft(r) Windows (tm)   */
/*                                                                         */
/*                      TRADE SECRET - CONFIDENTIAL                        */
/*                                                                         */
/*             The compiled libraries and API header files are             */
/*          (c) Copyright 1990-1995 by Stewart~Frazier Tools, Inc          */
/*                                                                         */
/*   This contains the 'long file name' stuff                              */
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




// ***************************************************************
// SPECIAL STUFF TO HANDLE LONG FILE NAMES AND WHATNOT STARTS HERE
// ***************************************************************



// CANONICALLY QUALFIED PATHS

void FAR PASCAL ConvertAsterisks(LPSTR lpBuffer)
{
LPSTR lp1, lp2;
char tbuf[2 * MAX_PATH];  // here it's better to use automatic array


   lp1 = _fstrrchr(lpBuffer, '\\');
   if(!lp1) lp1 = lpBuffer;
   else     lp1++;

   lp2 = _fstrchr(lp1, '.');  // find extension
   if(lp2)
   {
      lstrcpy(tbuf, lp2);

      *lp2 = 0;   // temporary
   }
   else
   {
      *tbuf = 0;
   }

   lp2 = _fstrchr(lp1, '*');  // find asterisk
   if(lp2)
   {
      lstrcpy(lp2, "????????");

      lp1[8] = 0;             // this function truncates name to 8.3
   }

   lp2 = _fstrchr(tbuf, '*'); // find asterisk
   if(lp2)
   {
      lstrcpy(lp2, "???");
      tbuf[4] = 0;
   }


   lstrcat(lp1, tbuf);  // restore extension and I'm done!

}



void FAR PASCAL DosQualifyPath(LPSTR lpDest, LPCSTR lpSrc)
{
LPSTR lp1, lp2, lp3, lp4, drive, dir, name, ext, lpSwitch;


   if(!lpDest)
   {
     OutputDebugString("NULL destination pointer in 'DosQualifyPath()'\r\n");
     return;
   }


   // important - must zero out buffer or we'll have problems...

   lp1 = (LPSTR)GlobalAllocPtr(GMEM_FIXED | GMEM_ZEROINIT,
                               10 * MAX_PATH);  // space for file name
   if(!lp1)
   {
      *lpDest = 0;
      return;
   }

   GlobalPageLock(GlobalPtrHandle(lp1));


            /* ensure that switches which follow the file name */
            /* are retained afterwards by temporarily removing */
            /* them before DOS3Call, then adding them back.    */

   lpSwitch = _fstrchr((LPSTR)lpSrc, '/');

   while(lpSwitch && lpSwitch > lpSrc && *(lpSwitch - 1) <= ' ')
   {
      lpSwitch--;  // back up to 1st non-white-space char
   }



   drive = lp1 + 2 * MAX_PATH;
   dir   = drive + 2 * MAX_PATH;
   name  = dir + 2 * MAX_PATH;
   ext   = name + 2 * MAX_PATH;


   // split name up into drive, dir, name, ext

   lp2 = (LPSTR)lpSrc;
   while(*lp2 && *lp2 <= ' ') lp2++;


   // get trimmed copy of 'lpSrc' into 'lp1'

   lstrcpy(lp1, lp2);
   if(lpSwitch) lp1[lpSwitch - lp2] = 0;
   else         _fstrtrim(lp1);


   lp2 = _fstrrchr(lp1, '\\');  // find last '\' in file name

   if(!lp2) lp2 = _fstrchr(lp1, ':');  // find first ':' in file name

   if(!lp2)
   {
      lstrcpy(name, lp1);        // name only; no path
      *drive = (char)(_getdrive() - 1 + 'A');
      drive[1] = ':';
      drive[2] = 0;

      _lgetdcwd((int)toupper(*drive) - ('A' - 1), dir, MAX_PATH);

      lstrcpy(dir, dir + 2);

      if(dir[lstrlen(dir) - 1] != '\\') lstrcat(dir, "\\");
   }
   else
   {
      lstrcpy(name, lp2 + 1);
      lp2[1] = 0;

      lp2 = _fstrchr(lp1, ':');

      if(!lp2)
      {
         if(*lp1=='\\' && lp1[1]=='\\')
         {
            *drive = 0;
         }
         else
         {
            *drive = (char)(_getdrive() - 1 + 'A');
            drive[1] = ':';
            drive[2] = 0;
         }

         lstrcpy(dir, lp1);
      }
      else
      {
         lstrcpy(dir, lp2 + 1);
         lp2[1] = 0;

         lstrcpy(drive, lp1);
      }
   }


   _fstrtrim(name);

   // special case:  '.' && '..'

   if(*name == '.')
   {
      lstrcat(dir, name);
      lstrcat(dir, "\\");

      *name = 0;
   }
   else
   {
      lp2 = _fstrchr(name, '.');

      if(lp2)
      {
         lstrcpy(ext, lp2);
         *lp2 = 0;
      }
      else
      {
         *ext = 0;
      }
   }

   if(*dir != '\\')  // does not begin with ROOT directory...
   {
      if(!*drive)
      {
         _lgetdcwd(0, lp1, MAX_PATH);
      }
      else
      {
         _lgetdcwd((int)toupper(*drive) - ('A' - 1), lp1, MAX_PATH);
      }

      if(lp1[lstrlen(lp1) - 1] != '\\') lstrcat(lp1, "\\");

      lstrcat(lp1, dir);
      lstrcpy(dir, lp1 + 2);  // ignore 1st 2 characters of path (drive)
   }


   // NEXT, go through the path and get rid of '\.\' and '\..\'

   lp2 = dir;

   while(*lp2)
   {
      lp3 = _fstrstr(lp2, "\\.\\");

      lp4 = _fstrstr(lp2, "\\..\\");

      if(!lp3 && !lp4) break;

      if(!lp3 || (lp4 && lp4 < lp3))
      {
         lp2 = lp4;

         while(lp2 > dir)
         {
            lp2--;

            if(*lp2 == '\\') break;
         }

         if(*lp2 != '\\')
         {
            *lpDest = 0;

            GlobalPageUnlock(GlobalPtrHandle(lp1));
            GlobalFreePtr(lp1);

            return;
         }

         lstrcpy(lp2, lp4 + 3);  // erase previous path and '\\..\\'
      }
      else
      {
         lstrcpy(lp3, lp3 + 2);

         lp2 = lp3;
      }

   }


   // UPDATE 12/15/94 (bug fix for character device names)
   //
   // SPECIAL CASE:  files with no extension may be devices; if this is
   //                possible check for device with same name and return
   //                only the name with no drive or directory if it is
   //                a character device name.

   if(!*ext || (ext[0]=='.' && ext[1]==0))
   {
    HFILE hFile;
    int i;

      // attempt to open file as device driver on boot drive

      if(HIWORD(GetVersion()) >= 0x400)  // dos 4.0 or greater
      {
         _asm mov ax, 0x3305   // get boot drive
         _asm int 0x21
         _asm mov dh, 0
         _asm mov i, dx

         ext[0] = 'A' + (char)i - 1;
         ext[1] = ':';
      }
      else
      {
         GetSystemDirectory(ext, MAX_PATH);
      }

      lstrcpy(ext + 2, name);

      hFile = _lopen(ext, OF_READ | OF_SHARE_DENY_NONE);  // does it exist?

      if(hFile != HFILE_ERROR)
      {
         // attempt to see if this is a device

         _asm mov ax, 0x4400   // get device info
         _asm mov bx, hFile
         _asm int 0x21
         _asm jnc no_problem
         _asm mov dx, 0

         no_problem:

         _asm mov i, dx


         if(i & 0x80)        // character device!!
         {
            _fstrupr(name);  // convert name to UPPER CASE

            *dir = 0;        // remove drive and directory
            *drive = 0;
         }

         _lclose(hFile);
      }

      *ext = 0;  // restore back to the way it was (either case)
   }


   // NOW, look for 'SUBST'ed and 'JOIN'ed drives here and canonize them


   if(*drive && *drive != '\\')         // ignore network drives...
   {
    LPCURDIR lpC = (LPCURDIR)GetDosCurDirEntry(toupper(*drive) - 'A' + 1);

      if(lpC)
      {
         // is this a valid 'SUBST'ed drive??  If so, substitute the
         // path within it for the 'drive' contents, and ensure it doesn't
         // end with a backslash (no matter what).  The backslash will be
         // added to it from 'dir', below.

         if((lpC->wFlags & CURDIR_TYPE_MASK)!=CURDIR_TYPE_INVALID &&
            (lpC->wFlags & CURDIR_SUBST) && lpC->wPathRootOffs > 1)
         {
            _hmemcpy(drive, lpC->pathname, lpC->wPathRootOffs);
            drive[lpC->wPathRootOffs] = 0;

            if(drive[lpC->wPathRootOffs - 1] == '\\')
            {
               drive[lpC->wPathRootOffs - 1] = 0;
            }
         }
         else if((lpC->wFlags & CURDIR_TYPE_MASK)!=CURDIR_TYPE_INVALID &&
                 (lpC->wFlags & CURDIR_JOIN) && lpC->wPathRootOffs > 1)
         {
            // LATER, add something to process 'JOIN'ed drives...

         }


#ifdef WIN32
         GlobalFreePtr(lpC);
#else // WIN32
         MyFreeSelector(SELECTOROF(lpC));
#endif // WIN32
      }
   }



   // NOW, put it *ALL* back together!!  YAY!

   lstrcpy(lpDest, drive);
   lstrcat(lpDest, dir);
   lstrcat(lpDest, name);
   lstrcat(lpDest, ext);

   _fstrtrim(lpDest);  // make sure there's no lead/trail spaces


   // As necessary trim trailing '\' (assuming it's not a root dir)

   if(lpDest[lstrlen(lpDest) - 1] == '\\')
   {
      lp2 = lpDest + lstrlen(lpDest) - 1;

      if(lp2 > lpDest && *(lp2 - 1)!=':') *lp2 = 0;
   }

   if(lpSwitch)
   {
      lstrcat(lpDest, lpSwitch);
   }


   GlobalPageUnlock(GlobalPtrHandle(lp1));
   GlobalFreePtr(lp1);

}




// DosQualifyPath0() - like 'DosQualifyPath()' but does *NOT*
// canonize a SUBST or JOIN drive, nor check for device names

void FAR PASCAL DosQualifyPath0(LPSTR lpDest, LPCSTR lpSrc)
{
LPSTR lp1, lp2, lp3, lp4, drive, dir, name, ext, lpSwitch;


   if(!lpDest)
   {
     OutputDebugString("NULL destination pointer in 'DosQualifyPath0()'\r\n");
     return;
   }


   // important - must zero out buffer or we'll have problems...

   lp1 = (LPSTR)GlobalAllocPtr(GMEM_FIXED | GMEM_ZEROINIT,
                               10 * MAX_PATH);  // space for file name
   if(!lp1)
   {
      *lpDest = 0;
      return;
   }

   GlobalPageLock(GlobalPtrHandle(lp1));


            /* ensure that switches which follow the file name */
            /* are retained afterwards by temporarily removing */
            /* them before DOS3Call, then adding them back.    */

   lpSwitch = _fstrchr(lpSrc, '/');

   while(lpSwitch && lpSwitch > lpSrc && *(lpSwitch - 1) <= ' ')
   {
      lpSwitch--;  // back up to 1st non-white-space char
   }



   drive = lp1 + 2 * MAX_PATH;
   dir   = drive + 2 * MAX_PATH;
   name  = dir + 2 * MAX_PATH;
   ext   = name + 2 * MAX_PATH;


   // split name up into drive, dir, name, ext

   lp2 = (LPSTR)lpSrc;
   while(*lp2 && *lp2 <= ' ') lp2++;


   // get trimmed copy of 'lpSrc' into 'lp1'

   lstrcpy(lp1, lp2);
   if(lpSwitch) lp1[lpSwitch - lp2] = 0;
   else         _fstrtrim(lp1);


   lp2 = _fstrrchr(lp1, '\\');  // find last '\' in file name

   if(!lp2) lp2 = _fstrchr(lp1, ':');  // find first ':' in file name

   if(!lp2)
   {
      lstrcpy(name, lp1);        // name only; no path
      *drive = (char)(_getdrive() - 1 + 'A');
      drive[1] = ':';
      drive[2] = 0;

      _lgetdcwd((int)toupper(*drive) - ('A' - 1), dir, 2 * MAX_PATH);

      lstrcpy(dir, dir + 2);

      if(dir[lstrlen(dir) - 1] != '\\') lstrcat(dir, "\\");
   }
   else
   {
      lstrcpy(name, lp2 + 1);
      lp2[1] = 0;

      lp2 = _fstrchr(lp1, ':');

      if(!lp2)
      {
         if(*lp1=='\\' && lp1[1]=='\\')
         {
            *drive = 0;
         }
         else
         {
            *drive = (char)(_getdrive() - 1 + 'A');
            drive[1] = ':';
            drive[2] = 0;
         }

         lstrcpy(dir, lp1);
      }
      else
      {
         lstrcpy(dir, lp2 + 1);
         lp2[1] = 0;

         lstrcpy(drive, lp1);
      }
   }


   _fstrtrim(name);

   // special case:  '.' && '..'

   if(*name == '.')
   {
      lstrcat(dir, name);
      lstrcat(dir, "\\");

      *name = 0;
   }
   else
   {
      lp2 = _fstrchr(name, '.');

      if(lp2)
      {
         lstrcpy(ext, lp2);
         *lp2 = 0;
      }
      else
      {
         *ext = 0;
      }
   }

   if(*dir != '\\')  // does not begin with ROOT directory...
   {
      if(!*drive)
      {
         _lgetdcwd(0, lp1, 2 * MAX_PATH);
      }
      else
      {
         _lgetdcwd((int)toupper(*drive) - ('A' - 1), lp1, 2 * MAX_PATH);
      }

      if(lp1[lstrlen(lp1) - 1] != '\\') lstrcat(lp1, "\\");

      lstrcat(lp1, dir);
      lstrcpy(dir, lp1 + 2);  // ignore 1st 2 characters of path (drive)
   }


   // NEXT, go through the path and get rid of '\.\' and '\..\'

   lp2 = dir;

   while(*lp2)
   {
      lp3 = _fstrstr(lp2, "\\.\\");

      lp4 = _fstrstr(lp2, "\\..\\");

      if(!lp3 && !lp4) break;

      if(!lp3 || (lp4 && lp4 < lp3))
      {
         lp2 = lp4;

         while(lp2 > dir)
         {
            lp2--;

            if(*lp2 == '\\') break;
         }

         if(*lp2 != '\\')
         {
            *lpDest = 0;

            GlobalPageUnlock(GlobalPtrHandle(lp1));
            GlobalFreePtr(lp1);

            return;
         }

         lstrcpy(lp2, lp4 + 3);  // erase previous path and '\\..\\'
      }
      else
      {
         lstrcpy(lp3, lp3 + 2);

         lp2 = lp3;
      }

   }


   // NOW, put it *ALL* back together!!  YAY!

   lstrcpy(lpDest, drive);
   lstrcat(lpDest, dir);
   lstrcat(lpDest, name);
   lstrcat(lpDest, ext);

   _fstrtrim(lpDest);  // make sure there's no lead/trail spaces


   // As necessary trim trailing '\' (assuming it's not a root dir)

   if(lpDest[lstrlen(lpDest) - 1] == '\\')
   {
      lp2 = lpDest + lstrlen(lpDest) - 1;

      if(lp2 > lpDest && *(lp2 - 1)!=':') *lp2 = 0;
   }

   if(lpSwitch)
   {
      lstrcat(lpDest, lpSwitch);
   }


   GlobalPageUnlock(GlobalPtrHandle(lp1));
   GlobalFreePtr(lp1);

}





/***************************************************************************/
/*                                                                         */
/*            LONG FILENAME SUPPORT (Windows 4.0 and later)                */
/*                                                                         */
/***************************************************************************/


BOOL WINAPI MyCreateFile(LPCSTR szPath, UINT uiModeFlags, UINT uiAttrib,
                         UINT uiActionFlags, HFILE FAR *lphFile)
{
BOOL rval;
HFILE hFile = HFILE_ERROR;                 // initial value


   // this function will return an ERROR if this function is not supported

   if(!szPath || !lphFile) return(0xffff); // error (invalid parms)

   _asm
   {
      push ds
      push si
      push di

      mov ax, 0x716c        // special LFN version of 'create file'
      mov bx, uiModeFlags   // access mode and special flags
      mov cx, uiAttrib      // attributes for file on create
      mov dx, uiActionFlags // whatcha wan' me ta do, boss?

      lds si, szPath        // da path, mahn

      mov di, 0             // 'hint' for short name (not used)

      int 0x21

      pop di
      pop si
      pop ds

      jc was_error

      mov hFile, ax         // file handle!  Yay!
      mov rval, 0           // ensure 'rval' is zero

      jmp not_error


   was_error:

      mov rval, ax          // error code (whatever)


   not_error:

   }


   *lphFile = hFile;

   return(rval);
}



HFILE WINAPI MyOpenFile(LPCSTR szPath, LPOFSTRUCT lpOfstr, UINT uiFlags)
{

   lpOfstr->cBytes = sizeof(*lpOfstr);  // safety precaution, OK?

   if(IsChicago)
   {
    OFSTRUCTEX ofstrx;
    static HFILE (WINAPI *lpOpenFileEx)(LPCSTR, OFSTRUCTEX FAR *, UINT)=NULL;
    HFILE rval = HFILE_ERROR;


      if(!lpOpenFileEx)  // get function pointer for it...
      {
       HMODULE hKernel=GetModuleHandle("KERNEL");

         (FARPROC &)lpOpenFileEx =
                       GetProcAddress(hKernel, (LPCSTR)MAKEINTRESOURCE(360));
                                // '360' is ordinal for OpenFileEx()
      }

      if(lpOpenFileEx)
      {
       int i1 = (int)((WORD)(&(((OFSTRUCT *)0)->szPathName)));

         ofstrx.nBytes     = sizeof(ofstrx);

         ofstrx.fFixedDisk = lpOfstr->fFixedDisk;
         ofstrx.nErrCode   = lpOfstr->nErrCode;

         _hmemcpy(ofstrx.reserved, lpOfstr->reserved,
                  sizeof(ofstrx.reserved));

         _hmemset(ofstrx.szPathName, 0, sizeof(ofstrx.szPathName));
         _hmemcpy(ofstrx.szPathName, lpOfstr->szPathName,
                  sizeof(lpOfstr->szPathName));


         rval = lpOpenFileEx(szPath, &ofstrx, uiFlags);


         lpOfstr->cBytes     = sizeof(*lpOfstr);
         lpOfstr->fFixedDisk = ofstrx.fFixedDisk;
         lpOfstr->nErrCode   = ofstrx.nErrCode;

         _hmemcpy(lpOfstr->reserved, ofstrx.reserved,
                  sizeof(ofstrx.reserved));

         _hmemcpy(lpOfstr->szPathName, ofstrx.szPathName,
                  sizeof(lpOfstr->szPathName));


         if(rval == HFILE_ERROR) return(rval);  // don't bother on error


         // I need to get the 'short name' for 'szPathName' at this time

         i1 = GetShortName(ofstrx.szPathName, lpOfstr->szPathName);


         // on error, close file and return 'HFILE_ERROR'

         if(i1)
         {
            _lclose(rval);

            lpOfstr->nErrCode = i1;  // last error was from 'get short path'

            return(HFILE_ERROR);
         }
         else
         {
            return(rval);
         }
      }
      else
      {
         return(OpenFile(szPath, lpOfstr, uiFlags));
      }
   }
   else if(IsNT)
   {
    char szNewPath[MAX_PATH];

      // in Windows NT, convert long name to short name before opening


      int i1 = GetShortName(szPath, szNewPath);

      if(i1) // on error, act as though I hadn't converted anything
      {
         return(OpenFile(szPath, lpOfstr, uiFlags));
      }
      else
      {
         return(OpenFile(szNewPath, lpOfstr, uiFlags));
      }
   }
   else
   {
      return(OpenFile(szPath, lpOfstr, uiFlags));
   }
}



// IMPORTANT:  'GetShortName()' returns a FULL path, but does not
//             canonize SUBST nor JOIN'ed drives (including networks)

BOOL WINAPI GetShortName(LPCSTR szPath, LPSTR lpDest)
{
BOOL rval;
LPSTR lp1, lp2;
char FAR *tbuf=NULL;


   if(!IsChicago && !IsNT)
      return(TRUE);

   tbuf = (LPSTR)GlobalAllocPtr(GPTR, 2 * MAX_PATH);
   if(!tbuf)
   {
     GlobalCompact(2 * MAX_PATH);
     
     tbuf = (LPSTR)GlobalAllocPtr(GPTR, 2 * MAX_PATH);
   }
   
   if(!tbuf)
   {
      OutputDebugString("Memory allocation failed - GetShortName()\r\n");
      return(TRUE);
   }


   DosQualifyPath0(tbuf, szPath);  // get FULL name, but don't convert
                                   // SUBST nor JOIN nor check for devices
                                   // (necessary in case current drive is
                                   //  not the same as the one in the path)
   lp1 = (LPSTR)tbuf;


   if(IsChicago)
   {

      _asm
      {
         push ds
         push es
         push si
         push di

         mov ax, 0x7160
         mov cx, 1        // get 'short' path

         lds si, lp1

         les di, lpDest

         int 0x21

         pop di
         pop si
         pop es
         pop ds

         jc was_error
         mov ax, 0
         jmp normal

      was_error:
         or ax, 0x8000    // ensure error code is non-zero

      normal:

         mov rval, ax
      }
   }
   else // if(IsNT)
   {
      DWORD dwProc, dwRval;

      dwProc = lpGetProcAddress32W(hKernel32, "GetShortPathNameA");

      // first, 'fix' selector addresses in linear memory

      GlobalPageLock(GlobalPtrHandle(szPath));
      GlobalPageLock(GlobalPtrHandle(lpDest));

      // Finally, call the 'GetShortPathNameA' Win32 API
      // (2nd parm to CallProcEx32W() converts both 16:16 pointers to flat)

      dwRval = lpCallProcEx32W(CPEX_DEST_STDCALL | 3, 1 | 2, dwProc,
                               (DWORD)szPath, (DWORD)lpDest, (DWORD)MAX_PATH);

      rval = !dwRval;

      GlobalPageUnlock(GlobalPtrHandle(szPath));
      GlobalPageUnlock(GlobalPtrHandle(lpDest));

   }
//   else
//   {
//      return(TRUE);  // error (not NT nor Chicago)
//   }

   if(!rval)
   {
      if(toupper(*tbuf) == toupper(*lpDest)) return(FALSE);  // drives match

      // at THIS point we have a mismatch between the drive letters in
      // the two strings.  SO, now I have to find out just how much of
      // the first name matches that of the 2nd name.  Typically I can
      // assume that the count of directories should match from the
      // first to the second...

      lp1 = tbuf + lstrlen(tbuf);
      lp2 = lpDest + lstrlen(lpDest);


      // if ONE ends in '\\' then both must!

      if(*tbuf && *(lp1 - 1) == '\\')
      {
         if(*lpDest && *(lp2 - 1) != '\\')
         {
            *(lp2++) = '\\';
            *lp2 = 0;
         }
      }
      else if(*lpDest && *(lp2 - 1) == '\\')
      {
         if(*tbuf && *(lp1 - 1) != '\\')
         {
            *(lp1++) = '\\';
            *lp1 = 0;
         }
      }


      while(lp1 > (LPSTR)tbuf && *(lp1 - 1) != ':')
      {
         lp1--;
         lp2--;

         while(lp1 > (LPSTR)tbuf && *lp1 != '\\') lp1--;

         while(lp2 > lpDest && *lp2 != '\\') lp2--;
      }

      if(lp1 > (LPSTR)tbuf && *(lp1 - 1) == ':')
      {
         // it worked & lp1 is 'in sync' with lp2, so overwrite lp1 with lp2

         lstrcpy(lp1, lp2);

         // OK!  'tbuf' contains the correct 'SHORT PATH' now!

         lstrcpy(lpDest, tbuf);
      }

      // at this point, always return FALSE indicating "no error"

      GlobalFreePtr(tbuf);

      return(FALSE);
   }
   else
   {
      lstrcpy(lpDest, szPath);  // on error, copy original name

      GlobalFreePtr(tbuf);

      return((rval & 0x7fff) ? (rval & 0x7fff) : rval);
   }
}




// IMPORTANT:  'GetLongName()' returns a FULL path, but does not
//             canonize SUBST nor JOIN'ed drives (including networks)

BOOL WINAPI GetLongName(LPCSTR szPath, LPSTR lpDest)
{
BOOL rval = TRUE;
BOOL bBSFlag = FALSE;
LPSTR lp1, lp2;
char FAR *pBuf=NULL, FAR *pBuf2=NULL;


   if(!IsChicago && !IsNT)
      return(TRUE);


   pBuf = (LPSTR)GlobalAllocPtr(GPTR, MAX_PATH * 4);
   if(!pBuf)
   {
     GlobalCompact(2 * MAX_PATH);
     
     pBuf = (LPSTR)GlobalAllocPtr(GPTR, 4 * MAX_PATH);
   }
   
   if(!pBuf)
   {
      OutputDebugString("Memory allocation failed - GetLongName()\r\n");
      return(TRUE);
   }


   pBuf2 = pBuf + 2 * MAX_PATH;

   DosQualifyPath0(pBuf, szPath);  // get FULL name, but don't convert
                                   // SUBST nor JOIN nor check for devices
                                   // (necessary in case current drive is
                                   //  not the same as the one in the path)

//   // Assume it's Chicago, and return appropriate error if not
//
//   if(IsChicago)
//   {
//      GetFullName(szPath, pBuf2);   // *ALWAYS* canonize the name first!
//                                    // THIS ensures I have no problems with
//                                    // SUBST'ed or JOIN'ed drives
//
//      lp1 = (LPSTR)pBuf2;
//
//
//      _asm
//      {
//         push ds
//         push es
//         push si
//         push di
//
//         mov ax, 0x7160
//         mov cx, 2        // get 'long' path
//
//         lds si, lp1
//
//         les di, lpDest
//
//         int 0x21
//
//         pop di
//         pop si
//         pop es
//         pop ds
//
//         jc was_error
//         mov ax, 0
//         jmp normal
//
//      was_error:
//         or ax, 0x8000    // ensure error code is non-zero
//
//      normal:
//
//         mov rval, ax
//      }
//   }
//   else // if(IsNT)
   {
      HANDLE hFF;
      struct _find_t fi;


//      if(GetFullName(szPath, lpDest))
//      {
//         GlobalFreePtr(pBuf);
//
//         return(TRUE);  // error (pass it along)
//      }

      rval = 0x8000;  // initial value

      // NOW that I have the 'full name', get the 'long name'
      // for the *LAST* element in the file name.  This duplicates
      // what the 'Chicago' version does.


#if 0  // this breaks long names in 'dir' and makes white text green... (?)

      // update 7/16/98 - get FULL LONG NAMES

      lstrcpy(pBuf2, szPath);  // keep it simple - no canonicalization

      if(pBuf2[0] == '\\' && pBuf2[1] == '\\')
      {
        lp1 = _fstrchr(pBuf2 + 2, '\\');

        if(lp1)
          lp1 = _fstrchr(lp1 + 1, '\\');  // find backslash after share name
      }
      else
      {
        lp1 = _fstrchr(pBuf2, '\\');  // find first backslash
      }

      if(!lp1)
      {
        lstrcpy(lpDest, szPath);  // just copy it (an error)
      }
      else
      {
        lp1++;

        memcpy(lpDest, pBuf2, (lp1 - pBuf2));

        lp2 = lpDest + (lp1 - pBuf2);
        *lp2 = 0;

        while(*lp1)
        {
          LPSTR lp3 = _fstrchr(lp1, '\\');  // next backslash

          if(!lp3)
          {
            lp3 = lp1 + lstrlen(lp1);
            bBSFlag = FALSE;
          }
          else
          {
            *lp3 = 0;
            bBSFlag = TRUE;
          }

          if( !(rval = MyFindFirst(pBuf2, ~_A_VOLID & 0xff, &fi)) )  // file was found
          {
            LPWIN32_FIND_DATA lpFI;

            lpFI = GetWin32FindData(&fi);

            if(lpFI && lpFI->cFileName[0])  // there *IS* a long name
              lstrcpy(lp2, lpFI->cFileName);
            else
              lstrcpy(lp2, lp1);

            MyFindClose(&fi);

            lp2 += lstrlen(lp2);  // point 'lp2' past this section of the name
          }
          else
          {
            // we are done now - rest of name remains "as-is"

            if(bBSFlag)
              *lp3 = '\\';

            lstrcpy(lp2, lp1);
            break;
          }


          if(bBSFlag)
          {
            *lp3 = '\\';      // put the '\' back
            lp1 = lp3 + 1;    // point 'lp1' just past it

            *(lp2++) = '\\';  // put it here also
            *lp2 = 0;  // always...
          }
          else
          {
            break;
          }
        }

      }

#else // 1

      bBSFlag = FALSE;

      lstrcpy(lpDest, szPath);

      lp1 = lpDest + lstrlen(lpDest);

      if(lp1 > lpDest && *(lp1 - 1) == '\\')  // is last char a backslash?
      {
         lp1--;
         *lp1 = 0;
         bBSFlag = TRUE;
      }
         while(lp1 > lpDest && *(lp1 - 1) != '\\')
           lp1--;  // find the last backslash, and point just past it

         if( !(rval = MyFindFirst(lpDest, ~_A_VOLID & 0xff, &fi)) )  // file was found
         {
            LPWIN32_FIND_DATA lpFI;

            lpFI = GetWin32FindData(&fi);

            if(lpFI && lpFI->cFileName[0])  // there *IS* a long name
               lstrcpy(lp1, lpFI->cFileName);

            MyFindClose(&fi);
         }

      if(bBSFlag)
      {
        lstrcat(lpDest, "\\");  // put backslash back
      }
#endif // 0
   }


   if(!rval)
   {
//      if(toupper(*pBuf) == toupper(*lpDest))
//        return(FALSE);  // drives match
//
//      // at THIS point we have a mismatch between the drive letters in
//      // the two strings.  SO, now I have to find out just how much of
//      // the first name matches that of the 2nd name.  Typically I can
//      // assume that the count of directories should match from the
//      // first to the second...
//
//      lp1 = pBuf + lstrlen(pBuf);
//      lp2 = lpDest + lstrlen(lpDest);
//
//
//      // if ONE ends in '\\' then both must!
//
//      if(*pBuf && *(lp1 - 1) == '\\')
//      {
//         if(*lpDest && *(lp2 - 1) != '\\')
//         {
//            *(lp2++) = '\\';
//            *lp2 = 0;
//         }
//      }
//      else if(*lpDest && *(lp2 - 1) == '\\')
//      {
//         if(*pBuf && *(lp1 - 1) != '\\')
//         {
//            *(lp1++) = '\\';
//            *lp1 = 0;
//         }
//      }
//
//
//      while(lp1 > (LPSTR)pBuf && *(lp1 - 1) != ':')
//      {
//         lp1--;
//         lp2--;
//
//         while(lp1 > (LPSTR)pBuf && *lp1 != '\\') lp1--;
//
//         while(lp2 > lpDest && *lp2 != '\\') lp2--;
//      }
//
//      if(lp1 > (LPSTR)pBuf && *(lp1 - 1) == ':')
//      {
//         // it worked & lp1 is 'in sync' with lp2, so overwrite lp1 with lp2
//
//         lstrcpy(lp1, lp2);
//
//         // OK!  'pBuf' contains the correct 'SHORT PATH' now!
//
//         lstrcpy(lpDest, pBuf);
//      }

      // at this point, always return FALSE indicating "no error"

      GlobalFreePtr(pBuf);

      return(FALSE);
   }
   else
   {
      lstrcpy(lpDest, szPath);  // on error, copy original name

      GlobalFreePtr(pBuf);

      return((rval & 0x7fff) ? (rval & 0x7fff) : rval);
   }
}



BOOL WINAPI GetFullName(LPCSTR szPath, LPSTR lpDest)
{
BOOL rval = TRUE;
LPSTR lp1;
char FAR *tbuf;


   if(!IsChicago && !IsNT)
      return(TRUE);


   tbuf = (LPSTR)GlobalAllocPtr(GPTR, 2 * MAX_PATH);

   if(!tbuf)
   {
     GlobalCompact(2 * MAX_PATH);
     
     tbuf = (LPSTR)GlobalAllocPtr(GPTR, 2 * MAX_PATH);
   }
   
   if(!tbuf)
   {
      OutputDebugString("Memory allocation failed - GetFullName()\n");
      return(TRUE);
   }
   

   // NOTE:  to ensure compatibility it is necessary to fully qualify
   //        this name before I attempt to canonize it... otherwise, if
   //        some "rogue" app changes the current dir for a drive that's
   //        not the current 'default' for me, I might end up with
   //        unexpected results...

   DosQualifyPath0(tbuf, szPath);  // get FULL name, but don't convert
                                   // SUBST nor JOIN nor check for devices

   lp1 = (LPSTR)tbuf;


   // Assume it's Chicago, and return appropriate error if not

   if(IsChicago)
   {
      _asm
      {
         push ds
         push es
         push si
         push di

         mov ax, 0x7160
         mov cx, 0        // get 'full' path name

         lds si, lp1

         les di, lpDest

         int 0x21

         pop di
         pop si
         pop es
         pop ds

         jc was_error
         mov ax, 0
         jmp normal

      was_error:
         or ax, 0x8000    // ensure error code is non-zero

      normal:

         mov rval, ax
      }
   }
   else // if(IsNT)
   {
      DWORD dwProc, dwRval;
      LPSTR lp1;

      dwProc = lpGetProcAddress32W(hKernel32, "GetFullPathNameA");

      // first, 'fix' selector addresses in linear memory

      GlobalPageLock(GlobalPtrHandle(szPath));
      GlobalPageLock(GlobalPtrHandle(lpDest));

      // Finally, call the 'GetFullPathNameA' Win32 API
      // (2nd parm to CallProcEx32W() converts both 16:16 pointers to flat)

      dwRval = lpCallProcEx32W(CPEX_DEST_STDCALL | 4, 1 | 4 | 8, dwProc,
                               (DWORD)szPath, (DWORD)MAX_PATH, (DWORD)lpDest,
                               (DWORD)(LPVOID)&lp1);

      rval = !dwRval;

      GlobalPageUnlock(GlobalPtrHandle(szPath));
      GlobalPageUnlock(GlobalPtrHandle(lpDest));

   }


   GlobalFreePtr(tbuf);
   tbuf = NULL;


   if(!rval)
     return(FALSE);


   lstrcpy(lpDest, szPath);  // on error, copy original name

   return((rval & 0x7fff) ? (rval & 0x7fff) : rval);
}



int FAR PASCAL MyRename(LPSTR lpOldName, LPSTR lpNewName)
{
int rval;
unsigned intval;


   if(!IsChicago && IsNT)
   {
      DWORD dwProc, dwRval;

      dwProc = lpGetProcAddress32W(hKernel32, "MoveFileA");

      // first, 'fix' selector addresses in linear memory

      GlobalPageLock(GlobalPtrHandle(lpOldName));
      GlobalPageLock(GlobalPtrHandle(lpNewName));

      // Finally, call the 'MoveFileA' Win32 API
      // (2nd parm to CallProcEx32W() converts both 16:16 pointers to flat)

      dwRval = lpCallProcEx32W(CPEX_DEST_STDCALL | 2, 1 | 2, dwProc,
                               (DWORD)lpOldName, (DWORD)lpNewName);

      GlobalPageUnlock(GlobalPtrHandle(lpOldName));
      GlobalPageUnlock(GlobalPtrHandle(lpNewName));

      if(dwRval)
      {
         return(0);
      }
      else
      {
         dwProc = lpGetProcAddress32W(hKernel32, "GetLastError");

         dwRval = lpCallProcEx32W(CPEX_DEST_STDCALL | 0, 0, dwProc);
         return((int)dwRval);
      }
   }


   if(IsChicago) intval = 0x7156;
   else          intval = 0x5600;

   _asm
   {
      push ds
      push es
      push di

      lds dx, lpOldName
      les di, lpNewName

      mov ax, intval
      int 0x21

      pop di
      pop es
      pop ds

      mov ax, 0xffff      /* -1 if error */

      jc did_not_work
      mov ax, 0           /* the error code */

did_not_work:

      mov rval, ax
   }


   return(rval);
}


int FAR PASCAL MyUnlink(LPSTR lpPath)
{
unsigned rval, intval;


   if(!IsChicago && IsNT)
   {
      DWORD dwProc, dwRval;

      dwProc = lpGetProcAddress32W(hKernel32, "DeleteFileA");

      // first, 'fix' selector addresses in linear memory

      GlobalPageLock(GlobalPtrHandle(lpPath));

      // Finally, call the 'DeleteFileA' Win32 API
      // (2nd parm to CallProcEx32W() converts both 16:16 pointers to flat)

      dwRval = lpCallProcEx32W(CPEX_DEST_STDCALL | 1, 1, dwProc,
                               (DWORD)lpPath);

      GlobalPageUnlock(GlobalPtrHandle(lpPath));

      if(dwRval)
      {
         return(0);
      }
      else
      {
         dwProc = lpGetProcAddress32W(hKernel32, "GetLastError");

         dwRval = lpCallProcEx32W(CPEX_DEST_STDCALL | 0, 0, dwProc);
         return((int)dwRval);
      }
   }


   if(IsChicago) intval = 0x7141;
   else          intval = 0x4100;

   _asm
   {
      push ds
      push si
      mov si, 0           // necessary for WIN 95
      mov ax, intval      /* get file attributes for NAME */
      lds dx, lpPath      /* the far pointer to the name */
      int 0x21

      mov ax, 0xffff      /* -1 if error */

      jc did_not_work
      mov ax, 0           /* the error code */

did_not_work:
      pop si
      pop ds
      mov rval, ax
   }

   return(rval);  // user must call 'GetExtErrMessage()' to get error info
}


unsigned FAR PASCAL MyGetFileAttr(LPSTR lpPath, unsigned FAR *lpwAttr)
{
unsigned file_attr, rval, intval;


   if(!IsChicago && IsNT)
   {
      DWORD dwProc, dwRval;

      dwProc = lpGetProcAddress32W(hKernel32, "GetFileAttributesA");

      // first, 'fix' selector addresses in linear memory

      GlobalPageLock(GlobalPtrHandle(lpPath));

      // Finally, call the 'GetFileAttributesA' Win32 API
      // (2nd parm to CallProcEx32W() converts both 16:16 pointers to flat)

      dwRval = lpCallProcEx32W(CPEX_DEST_STDCALL | 1, 1, dwProc,
                               (DWORD)lpPath);

      GlobalPageUnlock(GlobalPtrHandle(lpPath));

      if(dwRval != 0xffffffff)
      {
         return((unsigned)dwRval);
      }
      else
      {
         dwProc = lpGetProcAddress32W(hKernel32, "GetLastError");

         dwRval = lpCallProcEx32W(CPEX_DEST_STDCALL | 0, 0, dwProc);

         return(0xffff);
      }
   }

   if(IsChicago) intval = 0x7143;
   else          intval = 0x4300;

   _asm
   {
      push ds
      push bx
      mov ax, intval      /* get file attributes for NAME */
      lds dx, lpPath      /* the far pointer to the name */
      mov bl, 0           // this only applies for CHICAGO
      int 0x21

      mov ax, 0xffff      /* -1 if error */

      jc did_not_work
      mov file_attr, cx   /* the attribute */
      mov ax, 0           /* the error code */

did_not_work:
      pop bx
      pop ds
      mov rval, ax
   }

   *lpwAttr = file_attr;
   return(rval);
}


unsigned FAR PASCAL MySetFileAttr(LPSTR lpPath, unsigned wAttr)
{
unsigned rval, intval;



   if(!IsChicago && IsNT)
   {
      DWORD dwProc, dwRval;

      dwProc = lpGetProcAddress32W(hKernel32, "SetFileAttributesA");

      // first, 'fix' selector addresses in linear memory

      GlobalPageLock(GlobalPtrHandle(lpPath));

      // Finally, call the 'GetFileAttributesA' Win32 API
      // (2nd parm to CallProcEx32W() converts both 16:16 pointers to flat)

      dwRval = lpCallProcEx32W(CPEX_DEST_STDCALL | 2, 1, dwProc,
                               (DWORD)lpPath, (DWORD)wAttr);

      GlobalPageUnlock(GlobalPtrHandle(lpPath));

      if(dwRval) // an error
      {
         return(0);
      }
      else
      {
         dwProc = lpGetProcAddress32W(hKernel32, "GetLastError");

         dwRval = lpCallProcEx32W(CPEX_DEST_STDCALL | 0, 0, dwProc);

         return((unsigned)dwRval);  // the error code (why not!)
      }
   }

   if(IsChicago) intval = 0x7143;
   else          intval = 0x4301;

   _asm
   {
      push ds
      mov cx, wAttr       /* load attributes *NOW*! */
      mov ax, intval      /* set file attributes for NAME */
      lds dx, lpPath      /* the far pointer to the name */
      mov bl, 1           // this only applies for CHICAGO
      int 0x21

      mov ax, 0xffff      // -1 if it fails
      jc did_not_work
      mov ax, 0           // return code 0 if it works

did_not_work:
      mov rval, ax
      pop ds
   }


   return(rval);
}


LPSTR FAR PASCAL MyGetCurDir(WORD wDrive, LPSTR lpBuffer)
{
char FAR *rval=NULL;
int intval;
static char tbuf[MAX_PATH + 32];


   rval = lpBuffer;

   if(!lpBuffer)
     lpBuffer = tbuf;


   if(!wDrive) wDrive=_getdrive();

   *lpBuffer = (char)(wDrive + 'A' - 1);
   lpBuffer[1] = ':';
   lpBuffer[2] = '\\';
   lpBuffer[3] = 0;

   if(IsChicago) intval = 0x7147;
   else          intval = 0x4700;

   _asm
   {
      push ds       /* save default data segment on stack */

      mov ax, intval
      mov dl, BYTE PTR wDrive

      lds si, lpBuffer
      add si, 3
      call DOS3Call
      jnc i_am_done


      mov ax, 0
      mov WORD PTR rval, ax               /* returns a NULL */
      mov WORD PTR rval + 2, ax

i_am_done:

      pop ds                     /* restore default data segment */
   }

   if(lpBuffer[3]=='\\') lstrcpy(lpBuffer + 2, lpBuffer + 3);


   if(rval && IsNT && !IsChicago)
   {
      GetLongName(lpBuffer, lpBuffer);
   }


   return(rval);

}

int FAR PASCAL My_Chdir(LPSTR lpPath)
{
int rval, intval;
char FAR *tbuf = NULL;


   if(IsNT && !IsChicago)
   {
      tbuf = (LPSTR)GlobalAllocPtr(GHND, MAX_PATH);
      if(!tbuf)
         return(-1);

      if(!GetShortName(lpPath, tbuf))
      {
         lpPath = tbuf;  // on success, point to 'tbuf' (the SHORT name)
      }
   }

   if(IsChicago) intval = 0x713b;
   else          intval = 0x3b00;

   _asm
   {
      push ds
      mov ax, intval
      lds dx, lpPath      /* the far pointer to the name */
      int 0x21

      mov ax, 0xffff      /* -1 if error */

      jc did_not_work
      mov ax, 0           /* the error code */

did_not_work:
      pop ds
      mov rval, ax
   }

   if(tbuf)
      GlobalFreePtr(tbuf);

   return(rval);
}

int FAR PASCAL My_Rmdir(LPSTR lpPath)
{
int rval, intval;
char FAR *tbuf=NULL;


   if(IsNT && !IsChicago)
   {
      tbuf = (LPSTR)GlobalAllocPtr(GHND, MAX_PATH);
      if(!tbuf)
         return(-1);

      if(!GetShortName(lpPath, tbuf))
      {
         lpPath = tbuf;  // on success, point to 'tbuf' (the SHORT name)
      }
   }

   if(IsChicago) intval = 0x713a;
   else          intval = 0x3a00;

   _asm
   {
      push ds
      mov ax, intval
      lds dx, lpPath      /* the far pointer to the name */
      int 0x21

      mov ax, 0xffff      /* -1 if error */

      jc did_not_work
      mov ax, 0           /* the error code */

did_not_work:
      pop ds
      mov rval, ax
   }

   if(tbuf)
      GlobalFreePtr(tbuf);

   return(rval);
}

int FAR PASCAL My_Mkdir(LPSTR lpPath)
{
int rval, intval;


   if(!IsChicago && IsNT)
   {
      DWORD dwProc, dwRval;

      dwProc = lpGetProcAddress32W(hKernel32, "CreateDirectoryA");

      // first, 'fix' selector addresses in linear memory

      GlobalPageLock(GlobalPtrHandle(lpPath));

      // Finally, call the 'GetFileAttributesA' Win32 API
      // (2nd parm to CallProcEx32W() converts both 16:16 pointers to flat)

      dwRval = lpCallProcEx32W(CPEX_DEST_STDCALL | 2, 1 | 2, dwProc,
                               (DWORD)lpPath, (DWORD)0);

      GlobalPageUnlock(GlobalPtrHandle(lpPath));

      if(dwRval) // an error
      {
         return(0);
      }
      else
      {
         dwProc = lpGetProcAddress32W(hKernel32, "GetLastError");

         dwRval = lpCallProcEx32W(CPEX_DEST_STDCALL | 0, 0, dwProc);

         return((unsigned)dwRval);  // the error code (why not!)
      }
   }


   if(IsChicago) intval = 0x7139;
   else          intval = 0x3900;

   _asm
   {
      push ds
      mov ax, intval
      lds dx, lpPath      /* the far pointer to the name */
      int 0x21

      mov ax, 0xffff      /* -1 if error */

      jc did_not_work
      mov ax, 0           /* the error code */

did_not_work:
      pop ds
      mov rval, ax
   }

   return(rval);
}



           /************************************************/
           /** MyFindFirst(), MyFindNext(), MyFindClose() **/
           /************************************************/


// conversion:  'FILETIME' is 2 DWORDS containing 64-bit integer representing
//              the total # of nanoseconds since 1/1/1601 divided by 100.
//              DAYS("1/1/1601") == -109206L
//              (assume GREGORIAN CALENDAR and not 'Julian', else I must
//               add 11 days to the result to make up for calendar change)
//
// There are 8.64E11 '100-nanosecond' periods per day.  1 unit in the
// 'high word' of the 'FILETIME' structure is appx 4.971E-3 seconds.


//const long double ldDDFFT = 4294967296.0 / 8.64E11;



WORD FAR PASCAL DosDateFromFileTime(FILETIME FAR *lpFT)
{
//long double ld1;
//SFTDATE d;
//
//   ld1 = lpFT->dwHighDateTime + ((lpFT->dwLowDateTime & 0x80000000L)?1:0);
//
//   ld1 *= ldDDFFT;  // convert this to DAYS since 1/1/1601;
//   ld1 -= 109206.0; // convert this to DAYS since 12/31/1899;
//                    // (1/1/1900 is 'day 1' using 'Days()' function)
//
//   Date((long)ld1, &d);  // truncate the value
//
//   return((WORD)((d.year - 1980) * 512L) + 32 * d.month + d.day);


   if(!IsChicago && IsNT)
   {
      DWORD dwProc, dwRval;
      WORD w1=0, w2=0;

      dwProc = lpGetProcAddress32W(hKernel32, "FileTimeToDosDateTime");

      // first, 'fix' selector addresses in linear memory

      GlobalPageLock(GlobalPtrHandle(lpFT));
      GlobalPageLock(GlobalPtrHandle((LPSTR)&w1));

      // Finally, call the 'GetFileAttributesA' Win32 API
      // (2nd parm to CallProcEx32W() converts both 16:16 pointers to flat)

      dwRval = lpCallProcEx32W(CPEX_DEST_STDCALL | 3, 1 | 2 | 4, dwProc,
                               (DWORD)lpFT, (DWORD)(LPVOID)&w1,
                               (DWORD)(LPVOID)&w2);

      GlobalPageUnlock(GlobalPtrHandle((LPSTR)&w1));
      GlobalPageUnlock(GlobalPtrHandle(lpFT));

      if(dwRval) // an error
      {
         return(w1);
      }
      else
      {
         dwProc = lpGetProcAddress32W(hKernel32, "GetLastError");

         dwRval = lpCallProcEx32W(CPEX_DEST_STDCALL | 0, 0, dwProc);

         return(0);
      }
   }
   else
   {
    WORD rval;

      _asm
      {
         push ds
         mov ax, 0x71a7
         lds si, lpFT
         mov bl, 0

         int 0x21
         pop ds

         mov rval, dx
      }

      return(rval);
   }
}


WORD FAR PASCAL DosTimeFromFileTime(FILETIME FAR *lpFT)
{
//long double ld1;
//SFTTIME t;
//
//
//   ld1 = lpFT->dwHighDateTime + ((lpFT->dwLowDateTime & 0x80000000L)?1:0);
//
//   ld1 *= ldDDFFT;  // convert this to DAYS since 1/1/1601;
//
//   ld1 -= ((long)ld1);  // retain the FRACTION portion...
//
//   ld1 *= 24;           // the HOURS
//   t.hour = (BYTE)ld1;
//   ld1 -= t.hour;
//
//   ld1 *= 60;
//   t.minute = (BYTE)ld1;
//   ld1 -= t.minute;
//
//   ld1 *= 60;
//   t.second = (BYTE)ld1;
//
//
//   return((WORD)(2048L * t.hour + 32L * t.minute + (t.second >> 1)));

   if(!IsChicago && IsNT)
   {
      DWORD dwProc, dwRval;
      WORD w1=0, w2=0;

      dwProc = lpGetProcAddress32W(hKernel32, "FileTimeToDosDateTime");

      // first, 'fix' selector addresses in linear memory

      GlobalPageLock(GlobalPtrHandle(lpFT));
      GlobalPageLock(GlobalPtrHandle((LPSTR)&w1));

      // Finally, call the 'GetFileAttributesA' Win32 API
      // (2nd parm to CallProcEx32W() converts both 16:16 pointers to flat)

      dwRval = lpCallProcEx32W(CPEX_DEST_STDCALL | 3, 1 | 2 | 4, dwProc,
                               (DWORD)lpFT, (DWORD)(LPVOID)&w1,
                               (DWORD)(LPVOID)&w2);

      GlobalPageUnlock(GlobalPtrHandle((LPSTR)&w1));
      GlobalPageUnlock(GlobalPtrHandle(lpFT));

      if(dwRval) // an error
      {
         return(w2);
      }
      else
      {
         dwProc = lpGetProcAddress32W(hKernel32, "GetLastError");

         dwRval = lpCallProcEx32W(CPEX_DEST_STDCALL | 0, 0, dwProc);

         return(0);
      }
   }
   else
   {
    WORD rval;

      _asm
      {
         push ds
         mov ax, 0x71a7
         lds si, lpFT
         mov bl, 0

         int 0x21
         pop ds

         mov rval, cx
      }

      return(rval);
   }
}


int FAR PASCAL MyGetFTime(HFILE hFile, unsigned FAR *lpDate, unsigned FAR *lpTime)
{
int rval = 0;
unsigned date, time;


   _asm
   {
      mov ax, 0x5700      // GET file date & time
      mov bx, hFile
      int 0x21

      jnc it_worked
      mov cx, 0
      mov dx, 0
      mov rval, ax

it_worked:
      mov date, dx
      mov time, cx

   }

   *lpDate = date;
   *lpTime = time;

   return(rval);
}

int FAR PASCAL MySetFTime(HFILE hFile, unsigned date, unsigned time)
{
int rval = 0;


   _asm
   {
      mov ax, 0x5701      // SET file date & time
      mov bx, hFile
      mov dx, date
      mov cx, time
      int 0x21

      jnc it_worked
      mov rval, ax

it_worked:

   }

   return(rval);
}


int FAR PASCAL MyGetFAccessTime(HFILE hFile, unsigned FAR *lpDate, unsigned FAR *lpTime)
{
int rval = 0;
unsigned date, time;


   if(IsChicago) _asm
   {
      mov ax, 0x5704      // GET file ACCESS date & time
      mov bx, hFile
      int 0x21

      jnc it_worked
      mov cx, 0
      mov dx, 0
      mov rval, ax

it_worked:
      mov date, dx
      mov time, cx

   }
   else _asm
   {
      mov ax, 0x5700      // GET file date & time
      mov bx, hFile
      int 0x21

      jnc it_worked2
      mov cx, 0
      mov dx, 0
      mov rval, ax

it_worked2:
      mov date, dx
      mov time, cx

   }

   *lpDate = date;
   *lpTime = time;

   return(rval);
}


int FAR PASCAL MySetFAccessTime(HFILE hFile, unsigned date, unsigned time)
{
int rval = 0;


   if(IsChicago) _asm
   {
      mov ax, 0x5705      // SET file 'access' date & time
      mov bx, hFile
      mov dx, date
      mov cx, time
      int 0x21

      jnc it_worked
      mov rval, ax

it_worked:

   }
   else
   {
      rval = 1; // unsupported function (essentially)
   }

   return(rval);
}


int FAR PASCAL MyGetFCreateTime(HFILE hFile, unsigned FAR *lpDate, unsigned FAR *lpTime)
{
int rval = 0;
unsigned date, time;


   if(IsChicago) _asm
   {
      mov ax, 0x5706      // GET file CREATE date & time
      mov bx, hFile
      int 0x21

      jnc it_worked
      mov cx, 0
      mov dx, 0
      mov rval, ax

it_worked:
      mov date, dx
      mov time, cx

   }
   else _asm
   {
      mov ax, 0x5700      // GET file date & time
      mov bx, hFile
      int 0x21

      jnc it_worked2
      mov cx, 0
      mov dx, 0
      mov rval, ax

it_worked2:
      mov date, dx
      mov time, cx

   }

   *lpDate = date;
   *lpTime = time;

   return(rval);
}


int FAR PASCAL MySetFCreateTime(HFILE hFile, unsigned date, unsigned time)
{
int rval = 0;


   if(IsChicago) _asm
   {
      mov ax, 0x5707      // SET file 'create' date & time
      mov bx, hFile
      mov dx, date
      mov cx, time
      int 0x21

      jnc it_worked
      mov rval, ax

it_worked:

   }
   else
   {
      rval = 1; // unsupported function (essentially)
   }


   return(rval);
}






WORD FAR PASCAL MyFindFirst(LPCSTR lpName, WORD attrib, struct _find_t FAR *lpFI)
{
WORD rval, hFF;
LPWIN32_FIND_DATA lpFD;




   if(IsChicago)
   {
      // for Chicago:  'reserved[0..3]' is used to hold the 'lpFD' pointer;
      //               'reserved[4..7]' contains the returned HANDLE

      lpFD = (LPWIN32_FIND_DATA)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof(*lpFD));
      
      if(!lpFD)
        return(0xff);

      _asm
      {
         push ds             ; save registers that are getting 'blasted'
         push bx
         push si
         push di
         push es

         mov ax, 0x714e
         mov cl, BYTE PTR attrib
         mov ch, 0           ; attributes which MUST be matched! (none here)
                             ; this behavior differs from old "8.3" fn call
         lds dx, lpName

         les di, DWORD PTR lpFD ; es:di points to 'WIN32_FIND_DATA'

         mov bx, 0
         mov si, 0           ; remaining registers get '0' - just in case

         int 0x21            ; DOS 'findfirst' (using LFN)

         pop es
         pop di
         pop si
         pop bx
         pop ds

         mov hFF, ax         ; 'ax' is the returned handle.

         jc was_error0       ; test based on 'findfirst' call
         mov ax, 0           ; a ZERO value will be 'ok'
         jmp not_error0

was_error0:

         mov ax, 0x5900
         mov bx, 0
         int 0x21            ; get extended error info...
         or ax, 0x8000       ; ensure high bit of ax is set

not_error0:
         mov rval, ax        ; 'ax' is the error code
      }

      if(!rval)              // not error
      {

         // verify attributes are ALL contained in 'attrib'

         *((DWORD FAR *)(lpFI->reserved)) = (DWORD)lpFD;
         *(((DWORD FAR *)(lpFI->reserved)) + 1) = (DWORD)hFF;
         *(((DWORD FAR *)(lpFI->reserved)) + 2) = (DWORD)attrib;

         lpFI->wr_time = DosTimeFromFileTime(&lpFD->ftLastWriteTime);
         lpFI->wr_date = DosDateFromFileTime(&lpFD->ftLastWriteTime);
         lpFI->attrib  = (BYTE)lpFD->dwFileAttributes;

         lpFI->size    = lpFD->nFileSizeLow;  // only 2^32-1 in this case...

         if(*lpFD->cAlternateFileName)
         {
            _fmemcpy(lpFI->name, lpFD->cAlternateFileName, sizeof(lpFI->name));
         }
         else
         {
            _fmemcpy(lpFI->name, lpFD->cFileName, sizeof(lpFI->name));
         }
      }
      else
      {
         GlobalFreePtr(lpFD);
      }
   }
   else if(IsNT)
   {
      DWORD dwProc, dwRval;

      // for NT:  'reserved[0..3]' is used to hold the 'lpFD' pointer;
      //          'reserved[4..7]' contains the returned HANDLE

      lpFD = (LPWIN32_FIND_DATA)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof(*lpFD));
      
      if(!lpFD)
        return(0xff);


      dwProc = lpGetProcAddress32W(hKernel32, "FindFirstFileA");

      // first, 'fix' selector addresses in linear memory

      GlobalPageLock(GlobalPtrHandle(lpName));
      GlobalPageLock(GlobalPtrHandle((LPSTR)lpFD));

      // Finally, call the 'GetFileAttributesA' Win32 API
      // (2nd parm to CallProcEx32W() converts both 16:16 pointers to flat)

      dwRval = lpCallProcEx32W(CPEX_DEST_STDCALL | 2, 1 | 2, dwProc,
                               (DWORD)lpName, (DWORD)lpFD);

      GlobalPageUnlock(GlobalPtrHandle((LPSTR)lpFD));
      GlobalPageUnlock(GlobalPtrHandle(lpName));

      if(dwRval != 0xffffffff) // an error?
      {
         // verify attributes are ALL contained in 'attrib'

         *((DWORD FAR *)(lpFI->reserved)) = (DWORD)lpFD;
         *(((DWORD FAR *)(lpFI->reserved)) + 1) = dwRval;  // the 'hFF' handle
         *(((DWORD FAR *)(lpFI->reserved)) + 2) = (DWORD)attrib;

         lpFI->wr_time = DosTimeFromFileTime(&lpFD->ftLastWriteTime);
         lpFI->wr_date = DosDateFromFileTime(&lpFD->ftLastWriteTime);
         lpFI->attrib  = (BYTE)lpFD->dwFileAttributes;

         lpFI->size    = lpFD->nFileSizeLow;  // only 2^32-1 in this case...

         if(*lpFD->cAlternateFileName)
         {
            _fmemcpy(lpFI->name, lpFD->cAlternateFileName, sizeof(lpFI->name));
         }
         else
         {
            _fmemcpy(lpFI->name, lpFD->cFileName, sizeof(lpFI->name));
         }

         rval = 0;  // flags "ok" to next section (below)
      }
      else
      {
         dwProc = lpGetProcAddress32W(hKernel32, "GetLastError");

         dwRval = lpCallProcEx32W(CPEX_DEST_STDCALL | 0, 0, dwProc);

         rval = (WORD)dwRval;   // flag 'error' (return value)

         if(!rval)
            rval = 0x8000;

         GlobalFreePtr(lpFD);  // clean up resources
      }
   }
   else
   {
      _asm
      {
         push ds             ; save registers that are getting 'blasted'
         push bx
         push es


         mov ax, 0x2f00      ; get DTA address
         int 0x21

         push bx             ; save old DTA address on stack!
         push es


         mov ax, 0x1a00      ; set DTA address (to 'lpFI')
         lds dx, lpFI
         int 0x21

         mov ax, 0x4e00
         mov cx, attrib
         lds dx, lpName

         int 0x21            ; DOS 'findfirst'

         pop ds              ; load what was es:bx (old DTA) into ds:dx
         pop dx
         pushf               ; save flags (see below)

         mov ax, 0x1a00      ; set DTA address (to original value)
         int 0x21

         popf                ; restore flags
         pop es
         pop bx
         pop ds

         jc was_error        ; test based on 'findfirst' call
         mov ax, 0           ; a ZERO is not an error

   was_error:
         mov rval, ax        ; 'ax' is the error code
      }
   }


   if(!rval)
   {
      // NOTE:  If there are *ANY* attributes in the retrieved file that
      //        are *NOT* in 'attrib' then I must get the next entry...

      if((lpFI->attrib & attrib & 0xff) != (WORD)(lpFI->attrib & 0xff))
      {
         if(rval = MyFindNext(lpFI))
         {
            MyFindClose(lpFI);
         }
      }

      if(!rval)
      {
         return(0);  // *NO* error took place - otherwise, flow through...
      }
   }

   return((rval & 0xff)?(rval & 0xff):0x8000);

}

WORD FAR PASCAL MyFindNext(struct _find_t FAR *lpFI)
{
int attrib;
WORD rval, hFF;
LPWIN32_FIND_DATA lpFD;



   if(IsChicago)
   {
      attrib = (int)*(((DWORD FAR *)(lpFI->reserved)) + 2);

      do
      {
         lpFD = (LPWIN32_FIND_DATA)(*((DWORD FAR *)(lpFI->reserved)));
         hFF  = (WORD)(*(((DWORD FAR *)(lpFI->reserved)) + 1));

         if(!lpFD || !hFF) return(0xff);

         _asm
         {
            push bx             ; save registers that are getting 'blasted'
            push si
            push di
            push es

            mov ax, 0x714f
            mov bx, hFF

            les di, DWORD PTR lpFD ; es:di points to 'WIN32_FIND_DATA'

            mov si, 0           ; SI register gets '0' - just in case

            int 0x21            ; DOS 'findfirst' (using LFN)

            pop es
            pop di
            pop si
            pop bx

            jc was_error0       ; test based on 'findnext' call
            mov ax, 0           ; a ZERO value will be 'ok'
            jmp not_error0

was_error0:

            mov ax, 0x5900
            mov bx, 0
            int 0x21            ; get extended error info...
            or ax, 0x8000       ; ensure high bit of ax is set

not_error0:
            mov rval, ax        ; 'ax' is the error code
         }

         if(!rval)
         {
            lpFI->wr_time = DosTimeFromFileTime(&lpFD->ftLastWriteTime);
            lpFI->wr_date = DosDateFromFileTime(&lpFD->ftLastWriteTime);
            lpFI->attrib  = (BYTE)lpFD->dwFileAttributes;

            lpFI->size    = lpFD->nFileSizeLow;  // only 2^32-1 in this case...

            if(*lpFD->cAlternateFileName)
            {
               _fmemcpy(lpFI->name, lpFD->cAlternateFileName, sizeof(lpFI->name));
            }
            else
            {
               _fmemcpy(lpFI->name, lpFD->cFileName, sizeof(lpFI->name));
            }
         }

         // NOTE:  Continue searching if the current entry has *ANY*
         //        attributes that were not specified in the original spec...

      } while(!rval &&
              (lpFI->attrib & attrib & 0xff) != (lpFI->attrib & 0xff));

   }
   else if(IsNT)
   {
      DWORD dwProc, dwRval, dwHandle;

      attrib = (int)*(((DWORD FAR *)(lpFI->reserved)) + 2);

      lpFD = (LPWIN32_FIND_DATA)(*((DWORD FAR *)(lpFI->reserved)));
      dwHandle  = *(((DWORD FAR *)(lpFI->reserved)) + 1);

      if(!lpFD || !hFF) return(0xff);

      dwProc = lpGetProcAddress32W(hKernel32, "FindNextFileA");

      do
      {
         // for NT:  'reserved[0..3]' is used to hold the 'lpFD' pointer;
         //          'reserved[4..7]' contains the returned HANDLE

         // first, 'fix' selector addresses in linear memory

         GlobalPageLock(GlobalPtrHandle((LPSTR)lpFD));

         // Finally, call the 'GetFileAttributesA' Win32 API
         // (2nd parm to CallProcEx32W() converts both 16:16 pointers to flat)

         dwRval = lpCallProcEx32W(CPEX_DEST_STDCALL | 2, 2, dwProc,
                                  dwHandle, (DWORD)lpFD);

         GlobalPageUnlock(GlobalPtrHandle((LPSTR)lpFD));

         if(dwRval) // an error?
         {
            // verify attributes are ALL contained in 'attrib'

            lpFI->wr_time = DosTimeFromFileTime(&lpFD->ftLastWriteTime);
            lpFI->wr_date = DosDateFromFileTime(&lpFD->ftLastWriteTime);
            lpFI->attrib  = (BYTE)lpFD->dwFileAttributes;

            lpFI->size    = lpFD->nFileSizeLow;  // only 2^32-1 in this case...

            if(*lpFD->cAlternateFileName)
            {
               _fmemcpy(lpFI->name, lpFD->cAlternateFileName, sizeof(lpFI->name));
            }
            else
            {
               _fmemcpy(lpFI->name, lpFD->cFileName, sizeof(lpFI->name));
            }

            rval = 0;  // flags "ok" to next section (below)
         }
         else
         {
            dwProc = lpGetProcAddress32W(hKernel32, "GetLastError");

            dwRval = lpCallProcEx32W(CPEX_DEST_STDCALL | 0, 0, dwProc);

            rval = (WORD)dwRval;   // flag 'error' (return value)

            if(!rval)
               rval = 0x8000;
         }

         // NOTE:  Continue searching if the current entry has *ANY*
         //        attributes that were not specified in the original spec...

      } while(!rval &&
              (lpFI->attrib & attrib & 0xff) != (lpFI->attrib & 0xff));

   }
   else
   {
      _asm
      {
         push ds             ; save registers that are getting 'blasted'
         push bx
         push es


         mov ax, 0x2f00      ; get DTA address
         int 0x21

         push bx             ; save old DTA address on stack!
         push es


         mov ax, 0x1a00      ; set DTA address (to 'lpFI')
         lds dx, lpFI
         int 0x21

         mov ax, 0x4f00
         int 0x21            ; DOS 'findnext'

         pop ds              ; load what was es:bx (old DTA) into ds:dx
         pop dx
         pushf               ; save flags (see below)

         mov ax, 0x1a00      ; set DTA address (to original value)
         int 0x21

         popf                ; restore flags
         pop es
         pop bx
         pop ds

         jc was_error        ; test based on 'findnext' call
         mov ax, 0

was_error:
         mov rval, ax
      }
   }


   if(!rval) return(NULL);
   return((rval & 0xff)?(rval & 0xff):0x8000);

}


// This next function is in preparation for Win32s...

void FAR PASCAL MyFindClose(struct _find_t FAR *lpFI)
{
WORD hFF;
LPWIN32_FIND_DATA lpFD;


   if(IsChicago)
   {
      lpFD = (LPWIN32_FIND_DATA)(*((DWORD FAR *)(lpFI->reserved)));
      hFF  = (WORD)(*(((DWORD FAR *)(lpFI->reserved)) + 1));

      if(lpFD) GlobalFreePtr(lpFD);

      if(hFF) _asm
      {
         mov ax, 0x71a1
         mov bx, hFF
         int 0x21         ; carry set on error, but I'm gonna ignore it!
      }
   }
   else if(IsNT)
   {
      DWORD dwProc, dwRval, dwHandle;

      lpFD = (LPWIN32_FIND_DATA)(*((DWORD FAR *)(lpFI->reserved)));
      dwHandle  = *(((DWORD FAR *)(lpFI->reserved)) + 1);

      if(lpFD) GlobalFreePtr(lpFD);

      if(dwHandle && dwHandle != 0xffffffffL)
      {
         dwProc = lpGetProcAddress32W(hKernel32, "FindClose");

         dwRval = lpCallProcEx32W(CPEX_DEST_STDCALL | 1, 0, dwProc,
                                  dwHandle);

         if(!dwRval) // an error?
         {
            dwProc = lpGetProcAddress32W(hKernel32, "GetLastError");

            dwRval = lpCallProcEx32W(CPEX_DEST_STDCALL | 0, 0, dwProc);
         }
      }

   }


   _hmemset((void FAR *)lpFI, 0, sizeof(*lpFI));

}


LPWIN32_FIND_DATA FAR PASCAL GetWin32FindData(struct _find_t FAR *lpFI)
{

   if(IsChicago || IsNT)
   {
      return((LPWIN32_FIND_DATA)(*((DWORD FAR *)(lpFI->reserved))));
   }
   else
   {
      return(NULL);  // Not Applicable!!!
   }
}



BOOL FAR PASCAL FileExists(LPCSTR lpcFileName)
{
struct _find_t ff;
char tbuf[256];
BOOL MustBeSubDir = FALSE, rval = FALSE;
WORD w1, w2;



   while(*lpcFileName && *lpcFileName <=' ') lpcFileName++;
   if(!*lpcFileName) return(FALSE);

   lstrcpy(tbuf, lpcFileName);
   _fstrtrim(tbuf);

   w1 = lstrlen(tbuf);

   // Check to see if the user has a '.' or '..' at the end of the path

   if((w1 > 1 && tbuf[w1 - 1]=='.' &&
       (w1 == 1 || tbuf[w1 - 2]=='\\' || tbuf[w1 - 2]==':')) ||
      (w1 > 2 && tbuf[w1 - 1]=='.' && tbuf[w1 - 2]=='.' &&
       (w1 == 2 || tbuf[w1 - 3]=='\\' || tbuf[w1 - 3]==':')) )
   {
      DosQualifyPath(tbuf, tbuf);

      w1 = lstrlen(tbuf);
   }

   if(tbuf[w1 - 1] == '\\')                // file name ends in backslash!
   {
      if(lstrlen(tbuf) < 2 || tbuf[lstrlen(tbuf) - 2] == ':')
      {
//         lstrcat(tbuf, "DEV\\NUL");

         if(tbuf[lstrlen(tbuf) - 2]==':')   // we have a valid drive?
         {
            w2 = GetDriveType(toupper(*tbuf) - 'A');

            if(!w2 || w2 == 1)  // invalid drive or indeterminate type
            {
               return(FALSE);   // just say it's NOT FOUND
            }
            else
            {
               return(TRUE);    // root directory ALWAYS exists!
            }
         }
         else
         {
            w2 = GetDriveType(_getdrive() - 1);

            if(!w2 || w2 == 1)  // invalid drive or indeterminate type
            {
               return(FALSE);   // just say it's NOT FOUND
            }
            else
            {
               return(TRUE);    // root directory ALWAYS exists!
            }
         }
      }
      else
      {
         tbuf[lstrlen(tbuf) - 1] = 0;
         MustBeSubDir = TRUE;
      }
   }

   if(MyFindFirst(tbuf, (~_A_VOLID) & 0xff, &ff))
   {
      return(FALSE);
   }
   else
   {
      if(MustBeSubDir) rval = (ff.attrib & _A_SUBDIR)!=0;
      else             rval = TRUE;

      MyFindClose(&ff);

      return(rval);
   }

}





