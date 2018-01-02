/***************************************************************************/
/*                                                                         */
/*   WINCMD_U.C - Command line interpreter for Microsoft(r) Windows (tm)   */
/*                                                                         */
/*                      TRADE SECRET - CONFIDENTIAL                        */
/*                                                                         */
/*             The compiled libraries and API header files are             */
/*          (c) Copyright 1990-1995 by Stewart~Frazier Tools, Inc          */
/*                                                                         */
/*   This contains what might be referred to as 'utility' functions that   */
/*   are used mostly within the commands, but also (often) universally.    */
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



//BOOL NEAR PASCAL NearDelEnvString(LPCSTR lpName);
//
//LPSTR NEAR PASCAL FindEnvString(LPCSTR lpName);
//
//LPSTR NEAR PASCAL NearGetMyEnv(void);
//
//BOOL NEAR PASCAL SetMyEnvironment(LPSTR lpNewEnv);



static LPSTR lpPathArray=NULL;
static WORD wNPathArray=0;

#define PATH_ARRAY_WIDTH MAX_PATH      /* width of 'lpPathArray' entry */

// 'lpPathArray' (the above array) includes current directory of all
// drives on the system at the time I created it, and is maintained
// by 'MyChDir()' and 'forced' by the other functions.

BOOL FAR PASCAL AssignPathArray(void)
{
LPCURDIR lpC;
LPSTR lp1;
WORD w;


   wNPathArray = GetDosMaxDrives();

   lpPathArray = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                       PATH_ARRAY_WIDTH * wNPathArray);

   if(!lpPathArray) return(TRUE);


   for(lp1=lpPathArray, w=1; w<wNPathArray; w++, lp1+=PATH_ARRAY_WIDTH)
   {
      lpC = (LPCURDIR)GetDosCurDirEntry(w);

      *lp1 = (char)('@' + w);
      lp1[1] = ':';
      lp1[2] = '\\';
      lp1[3] = 0;

      if(lpC)
      {
         lstrcpy(lp1 + 2, lpC->pathname + lpC->wPathRootOffs);

         // for Chicago, convert to *LONG* name if the drive isn't a SUBST
         // or JOIN drive, and if it's not the ROOT directory...

         if(lp1[3] && (IsChicago || IsNT) &&
            !(lpC->wFlags & CURDIR_SUBST) && !(lpC->wFlags & CURDIR_JOIN) )
         {
            GetLongName(lp1, lp1);
         }

#ifdef WIN32
         GlobalFreePtr(lpC);
#else // WIN32
         MyFreeSelector(SELECTOROF(lpC));
#endif // WIN32
      }
//      else
//      {
//       _asm
//       {
//         push ds       /* save default data segment on stack */
//
//         mov ax, 0x4700
//         mov dl, BYTE PTR w
//         lds si, lp1           /* address of 'my' buffer to store path into */
//
//         call DOS3Call
//         jnc no_error
//
//         lds si, lp1           /* address of 'my' buffer to store path into */
//
//         mov al, BYTE PTR w        // on error store 'X:\' (i.e. drive 'X')
//         add al, '@'
//         mov BYTE PTR [si], al
//         mov BYTE PTR [si + 1], ':'
//         mov BYTE PTR [si + 2], '\\'
//         mov BYTE PTR [si + 3], 0
//
//no_error:
//         pop ds
//       }
//    }

   }


   return(FALSE);

}


 /** ALTERNATE 'DOS' UTILITIES (because the normal ones won't work...) **/

char *getcwd(char *buffer, int maxlen)
{
   return(_getdcwd(0, buffer, maxlen));
}

char *_getcwd(char *buffer, int maxlen)
{
   return(_getdcwd(0, buffer, maxlen));
}

char *_getdcwd(int drive, char *buffer, int maxlen)
{

#if defined( M_I86TM ) || defined( M_I86SM ) || defined( M_I86MM )

   return((char *)OFFSETOF(_lgetdcwd(drive, buffer, maxlen)));

#else

   return((char *)_lgetdcwd(drive, buffer, maxlen));

#endif
}

char FAR *_lgetdcwd(int drive, char FAR *buffer, int maxlen)
{
BOOL bFlag;
LPSTR rval;
char mybuf[2 * MAX_PATH];



//   if(maxlen < 4 || !buffer) return(NULL);  // parameter validation

   rval = buffer;

   if(!drive)
     drive=_getdrive();

   if(!lpPathArray)
   {
      AssignPathArray();
   }

   bFlag = FALSE;

   if(lpPathArray && drive <= (int)wNPathArray)
   {
      lstrcpy(mybuf, lpPathArray + (drive - 1) * PATH_ARRAY_WIDTH);

      // NOW, attempt to SET the current directory...

      bFlag = My_Chdir(mybuf)!=0;

      if(!bFlag)
      {
         if(maxlen > lstrlen(mybuf)) // this means the above section worked properly
         {
            lstrcpy(buffer, mybuf);  // it worked!
            return(buffer);
         }
         else
         {
            return(NULL);            // error (buffer too short)
         }
      }
   }



   // if the above section does not pass, use the section below!

   MyGetCurDir(drive, mybuf);


   if(buffer)
   {
      if(lstrlen(mybuf) >= maxlen)
      {
         _fmemcpy(buffer, mybuf, maxlen - 1);
         buffer[maxlen - 1] = 0;
      }
      else
      {
         lstrcpy(buffer, mybuf);
      }
   }


   if(drive <= (int)wNPathArray)
   {
      lstrcpy(lpPathArray + (drive - 1) * PATH_ARRAY_WIDTH, mybuf);
   }

   return(rval);
}

int _getdrive(void)
{
#ifdef WIN32
   char tbuf[2 * MAX_PATH];

   GetCurrentDirectory(tbuf, sizeof(tbuf));

   if(tbuf[0] != '\\' && tbuf[1] != '\\')
     return(toupper(tbuf[0]) - 'A' + 1);
   else
     return(0);  // not a valid drive - use 'default' (if any)

#else  // WIN32

register int iRval;

   _asm
   {
      mov ax, 0x1900
      call DOS3Call
      inc al
      mov ah, 0
      mov iRval, ax
   }

  return(iRval);
#endif // WIN32
}

int _chdrive(int drive)     /* must do this one also because in 'mlibcew' */
{
#ifdef WIN32

   char tbuf[4];

   if(!drive)
     return(0);

   tbuf[0] = drive + 'A' - 1;
   tbuf[1] = ':';
   tbuf[2] = 0;

   return((int)!SetCurrentDirectory(tbuf));

#else  // WIN32

register int iRval;

NPSTR np1;
HLOCAL h1;

                            /* it's in the same seg as '_getdrive()'. */
   if(drive==0) return(0);  /* already on default drive, eh?? */

   h1 = LocalAlloc(LMEM_MOVEABLE, PATH_ARRAY_WIDTH);
   if(h1 && (np1 = (char NEAR *)LocalLock(h1)))
   {
      _lgetdcwd(drive, np1, PATH_ARRAY_WIDTH);  // forces dir to be current

      LocalUnlock(h1);
   }

   if(h1) LocalFree(h1);


   drive--;

   if(FALSE) return(0);     /* will never execute */

   _asm
   {
      mov ax, 0xe00
      mov dl, BYTE PTR drive
      call DOS3Call

      mov ax, 0x1900
      call DOS3Call
      mov ah, 0
      cmp al, BYTE PTR drive
      jz it_works
      mov ah, 0xff
it_works:
      mov al, ah
            /* at this point ax==0 if GOOD, or 0xffff if BAD */
            
      mov iRval, ax      
   }
   
   return(iRval);
#endif // WIN32
}


int FAR PASCAL MyChDir(LPSTR lpDir)
{
BOOL bFlag;
LPSTR lp1;
WORD wDrive;
char FAR *tbuf=NULL;


   if(!lpPathArray)
   {
      if(AssignPathArray())
        return(1);
   }

   tbuf = (LPSTR)GlobalAllocPtr(GPTR, 2 * MAX_PATH);
   if(!tbuf)
      return(-1);  // failure code


   if(IsChicago || IsNT) // for Chicago and NT, convert to *SHORT* name, do the 'CHDIR',
   {                     // and then convert the original path to a LONG name
                         // before I assign it to the 'path' array...

      GetShortName(lpDir, tbuf);

      bFlag = (My_Chdir(tbuf) != 0);

      lpDir = tbuf;
   }
   else
   {
      bFlag = (My_Chdir(lpDir) != 0);
   }

   if(bFlag)
   {
     GlobalFreePtr(tbuf);
     return(1);  // error, so return now!
   }

   if(lpDir[1]==':')     // there is a drive spec here!
   {
      wDrive = toupper(*lpDir) - '@';
   }
   else
   {
      wDrive = _getdrive();
   }

   if(wDrive > wNPathArray)     // invalid drive specified somehow
   {
      // because there is a problem determining the drive info, re-create
      // the 'lpPathArray' matrix.

      GlobalFreePtr(lpPathArray);
      lpPathArray = NULL;
      wNPathArray = 0;

      GlobalFreePtr(tbuf);

      return(AssignPathArray());  // this updates the array, but it's slower
   }


   // Final step:  Update 'lpPathArray' entry

   lp1 = lpPathArray + (wDrive - 1) * PATH_ARRAY_WIDTH;

   bFlag = (MyGetCurDir(wDrive, lp1) == NULL);

   if((IsChicago || IsNT) && bFlag)
   {
      GetLongName(lp1, lp1);  // for Chicago, bug fix (SUBST drives)
                              // force LONG name after retrieving path!
   }

   GlobalFreePtr(tbuf);

   return(bFlag);        // done!

}


UINT FAR PASCAL MyRead(HFILE hFile, void _huge *hpBuf, UINT cbBuf)
{
UINT rval;


#ifdef WIN32

   if(ReadFile((HANDLE)hFile, hpBuf, cbBuf, &rval, NULL))
     return(rval);
   else
     return((UINT)-1);

#else // WIN32

   _asm
   {
      push ds

      mov ax, 0x3f00
      mov bx, hFile
      mov cx, cbBuf

      lds dx, hpBuf

//      call DOS3Call
      int 0x21

      pop ds

      jnc no_error

      mov ax, 0xffff     ; return ffffH on error

no_error:
      mov rval, ax
   }

   return(rval);
#endif // WIN32
}


UINT FAR PASCAL MyWrite(HFILE hFile, const void _huge *hpBuf, UINT cbBuf)
{
UINT rval;

#ifdef WIN32

   if(WriteFile((HANDLE)hFile, hpBuf, cbBuf, &rval, NULL))
     return(rval);
   else
     return((UINT)-1);

#else // WIN32

   _asm
   {
      push ds

      mov ax, 0x4000
      mov bx, hFile
      mov cx, cbBuf

      lds dx, hpBuf

//      call DOS3Call
      int 0x21

      pop ds

      jnc no_error

      mov ax, 0xffff     ; return ffffH on error

no_error:
      mov rval, ax

//      // COMMIT the output now!  ignore any errors
//
//      mov ah, 0x68
//      mov bx, hFile
//
//      call DOS3Call
//
////      mov ah, 0x0d
////      call DOS3Call      ; DISK RESET (flush all buffers)

   }

   return(rval);
#endif // WIN32
}


UINT FAR PASCAL MyWrite2(HFILE hFile, void _huge *hpBuf, UINT cbBuf,
                         UINT wDestSectorSize)
{
DWORD dwPos, dwSize;
UINT rval;


   // NOTE:  Assume we're extending the file size, not overwriting any
   //        existing data on the disk.

   if(!wDestSectorSize || !(cbBuf % wDestSectorSize))
   {
      return(MyWrite(hFile, hpBuf, cbBuf));
   }

#ifdef WIN32
   dwPos = SetFilePosition((HANDLE)hFile, 0, NULL, FILE_BEGIN);
#else  // WIN32
   dwPos = _llseek(hFile, 0, SEEK_CUR);  // get current file pointer
#endif // WIN32

   dwSize = dwPos + cbBuf;               // final size of file

   cbBuf = (cbBuf + (wDestSectorSize - 1));
   cbBuf -= cbBuf % wDestSectorSize;

   rval = MyWrite(hFile, hpBuf, cbBuf);

   if(rval != cbBuf) return(rval);

//   _llseek(hFile, dwSize, SEEK_SET);     // position at "THE END"
//   MyWrite(hFile, hpBuf, 0);             // write zero bytes (truncate)!

   return((UINT)(dwSize - dwPos));       // return correct number of bytes...
}



HFILE FAR PASCAL MyDupFileHandle(HFILE hFile)     // create dup handle
{
HFILE hRval;

#ifdef WIN32

   if(!DuplicateHandle(GetCurrentProcess(), (HANDLE)hFile,
                       GetCurrentProcess(), (HANDLE *)&hRval,
                       0, TRUE, DUPLICATE_SAME_ACCESS))
     return((HFILE)INVALID_HANDLE_VALUE)
   else
     return(hRval);

#else  // WIN32

   _asm
   {
      mov ax, 0x4500      // dup file handle
      mov bx, hFile
      call DOS3Call

      jnc no_error
      mov ax, 0xffff

no_error:
      mov hRval, ax
   }

   return(hRval);

#endif // WIN32
}

#ifndef WIN32

BOOL FAR PASCAL MyAssignFileHandle(HFILE hFile, HFILE hDupFile)
{                        // force 'hFile' to refer to same file as 'hDupFile'
BOOL rval;


   if(hFile==hDupFile) return(TRUE);  // error!

   _asm
   {
      mov ax, 0x4600      // dup file handle
      mov bx, hDupFile    // this handle gets duplicated
      mov cx, hFile       // this handle gets assigned the file
      call DOS3Call

      jc was_error

      mov ax, 0

was_error:
      mov rval, ax
   }

   return(rval);
}

#endif // WIN32



/***************************************************************************/
/*                                                                         */
/*  Specialized functions which perform the 'meat' of the functionality    */
/*                                                                         */
/***************************************************************************/





/***************************************************************************/
/*                                                                         */
/*   'UTILITY' Functions that do *Something* with File Names/Paths/Etc.    */
/*                                                                         */
/***************************************************************************/



BOOL FAR PASCAL QualifyPath(LPSTR dest, LPSTR src, BOOL bWildFlag)
{
HLOCAL h1=(HLOCAL)NULL,h2=(HLOCAL)NULL;
NPSTR path, cwd, drive, dir, name, ext, p1;
struct find_t NEAR *npff;
BOOL rval;


   if(!(h1=LocalAlloc(LMEM_MOVEABLE | LMEM_ZEROINIT, 2 * QUAL_PATH_SIZE)) ||
      !(h2=LocalAlloc(LMEM_MOVEABLE | LMEM_ZEROINIT,
                      QUAL_DRIVE_SIZE + QUAL_DIR_SIZE + QUAL_NAME_SIZE
                      + QUAL_EXT_SIZE)) ||
      !(path = (char NEAR *)LocalLock(h1)) || !(drive = (char NEAR *)LocalLock(h2)))
   {
      if(path)  LocalUnlock(h1);
      if(drive) LocalUnlock(h2);
      if(h1)    LocalFree(h1);
      if(h2)    LocalFree(h2);
      *dest = 0;
      return(TRUE);
   }

   cwd  = path  + QUAL_PATH_SIZE;

   dir  = drive + QUAL_DRIVE_SIZE;
   name = dir   + QUAL_DIR_SIZE;
   ext  = name  + QUAL_NAME_SIZE;

   npff = (struct find_t NEAR *)dir;

   lstrcpy(path, src);          /* make a copy of the source in local mem */

   if(*path && path[1]==':' && path[2]=='/')
   {                                 /* special condition - device driver */
      path[2] = '\\';                /* convert 'd:/' to 'd:\'  */
   }
//   else if(*path && *path=='\\' && path[1]=='\\')  // NETWORK DRIVE
//   {
//      p1 = strchr(path, '/');
//
//      if(p1)
//      {
//         *p1='\\';  // that's all, folks!
//      }
//   }

   _splitpath(path,drive,dir,name,ext);

   if(*drive==0 && (*dir!='\\' || dir[1]!='\\'))
   {
      drive[0] = (char)(_getdrive() - 1 + 'A');
      drive[1] = ':';
      drive[2] = 0;
   }


   if(*dir!='\\')
   {
      _lgetdcwd(toupper(*drive) - 'A' + 1, cwd, QUAL_PATH_SIZE);

      if(cwd[strlen(cwd) - 1]!='\\')
         strcat(cwd, "\\");

      strcat(cwd, dir);

      strcpy(dir, cwd + 2);  /* copy back to 'dir' */
   }

   if(bWildFlag && *name==0 && *ext==0 /* && strcmp(dir,"\\")==0 */)
   {
      strcpy(name,"*.*");    /* wildcard file names for root directories! */
   }

   _makepath(path,drive,dir,name,ext);                 /* re-assemble path */


   if(bWildFlag && (*name!=0 || *ext!=0)
       && strpbrk(name, "?*")==(char *)NULL
       && strpbrk(ext,  "?*")==(char *)NULL)
   {
      BOOL bDoneFlag = FALSE;

         /** There's a file name/ext, but no wildcards so check to  **/
         /** see if it represents a path, and if so add "*.*" to it **/

      if(path[0] == '\\' && path[1] == '\\')  // \\server\share
      {
         // if only 1 backslash following server name, it's a root dir
         // and must have "\*.*" added to it

         p1 = strchr(path + 2, '\\');
         if(p1)
         {
           p1 = strchr(p1 + 1, '\\');
         }

         if(!p1)  // yes, I do add wildcard to end of name
         {
            strcat(path, "\\*.*");
            bDoneFlag = TRUE;
         }
         else if(p1[1] == 0)  // backslash at end (not likely, but...)
         {
            strcat(path, "*.*");
            bDoneFlag = TRUE;
         }
      }

      if(!bDoneFlag && !MyFindFirst(path, ~_A_VOLID & 0xff, npff))
      {
         if(npff->attrib & _A_SUBDIR)      /* it's a sub-directory!! */
         {
            strcat(path, "\\*.*");
         }

         MyFindClose(npff);

         rval = FALSE;
      }

      rval = FALSE;         /* if not found assume it's a 'new' file */
   }
   else
      rval = FALSE;

   lstrcpy(dest, path);         /* copy path back to destination! */

   LocalUnlock(h1);
   LocalUnlock(h2);
   LocalFree(h1);
   LocalFree(h2);

   return(rval);

}



int FAR PASCAL GetPathNameInfo(LPSTR lpPath)
{
HLOCAL h1=(HLOCAL)NULL, h2=(HLOCAL)NULL;
NPSTR path, drive, dir, name, ext, p1;
int rval = 0;


   if(!(h1=LocalAlloc(LMEM_MOVEABLE | LMEM_ZEROINIT, QUAL_PATH_SIZE)) ||
      !(h2=LocalAlloc(LMEM_MOVEABLE | LMEM_ZEROINIT,
                      QUAL_DRIVE_SIZE + QUAL_DIR_SIZE + QUAL_NAME_SIZE
                      + QUAL_EXT_SIZE)) ||
      !(path = (char NEAR *)LocalLock(h1)) || !(drive = (char NEAR *)LocalLock(h2)))
   {
      if(path)  LocalUnlock(h1);
      if(drive) LocalUnlock(h2);
      if(h1)    LocalFree(h1);
      if(h2)    LocalFree(h2);
      return(TRUE);
   }


   dir  = drive + QUAL_DRIVE_SIZE;
   name = dir   + QUAL_DIR_SIZE;
   ext  = name  + QUAL_NAME_SIZE;


   lstrcpy(path, lpPath);

   if(*path && path[1]==':' && path[2]=='/')
   {                                 /* special condition - device driver */
      path[2] = '\\';                /* convert 'd:/' to 'd:\'  */
   }
//   else if(*path && *path=='\\' && path[1]=='\\')  // NETWORK DRIVE
//   {
//      p1 = strchr(path, '/');
//
//      if(p1)
//      {
//         *p1='\\';  // that's all, folks!
//      }
//   }


   _splitpath(path,drive,dir,name,ext);

   if(*drive)     rval |= PATH_HAS_DRIVE;
   if(*dir)       rval |= PATH_HAS_DIR;
   if(*dir=='\\') rval |= PATH_HAS_ROOT;
   if(*name)      rval |= PATH_HAS_NAME;
   if(*ext)       rval |= PATH_HAS_EXT;

   if(strchr(name, '?') || strchr(name, '*') ||
      strchr(ext,  '?') || strchr(ext, '*'))  rval |= PATH_IS_WILD;

   LocalUnlock(h1);
   LocalUnlock(h2);
   LocalFree(h1);
   LocalFree(h2);

   return(rval);

}


  /** This next function returns pointer to extension in volatile area **/

volatile LPSTR FAR PASCAL GetExtension(const LPSTR lpPath)
{
HLOCAL h1=(HLOCAL)NULL, h2=(HLOCAL)NULL;
NPSTR path, drive, dir, name, p1;
static char ext[QUAL_EXT_SIZE];
int rval = 0;


   if(!(h1=LocalAlloc(LMEM_MOVEABLE | LMEM_ZEROINIT, QUAL_PATH_SIZE)) ||
      !(h2=LocalAlloc(LMEM_MOVEABLE | LMEM_ZEROINIT,
                      QUAL_DRIVE_SIZE + QUAL_DIR_SIZE + QUAL_NAME_SIZE
                      + QUAL_EXT_SIZE)) ||
      !(path = (char NEAR *)LocalLock(h1)) || !(drive = (char NEAR *)LocalLock(h2)))
   {
      if(path)  LocalUnlock(h1);
      if(drive) LocalUnlock(h2);
      if(h1)    LocalFree(h1);
      if(h2)    LocalFree(h2);
      return(NULL);
   }


   dir  = drive + QUAL_DRIVE_SIZE;
   name = dir   + QUAL_DIR_SIZE;


   lstrcpy(path, lpPath);

   if(*path && path[1]==':' && path[2]=='/')
   {                                 /* special condition - device driver */
      path[2] = '\\';                /* convert 'd:/' to 'd:\'  */
   }
   else if(*path && *path=='\\' && path[1]=='\\')  // NETWORK DRIVE
   {
      p1 = strchr(path, '/');

      if(p1)
      {
         *p1='\\';  // that's all, folks!
      }
   }


   _splitpath(path,drive,dir,name,ext);

   LocalUnlock(h1);
   LocalUnlock(h2);
   LocalFree(h1);
   LocalFree(h2);

   return((LPSTR)ext);
}


   /** This next function is like the one above, but substitutes a new **/
   /** extension in place of the old, writing on top of original path. **/

BOOL FAR PASCAL NewExtension(volatile LPSTR lpPath, const LPSTR lpNewExt)
{
HLOCAL h1=(HLOCAL)NULL, h2=(HLOCAL)NULL;
NPSTR path, drive, dir, name, ext, p1;
int rval = 0;


   if(!(h1=LocalAlloc(LMEM_MOVEABLE | LMEM_ZEROINIT, QUAL_PATH_SIZE)) ||
      !(h2=LocalAlloc(LMEM_MOVEABLE | LMEM_ZEROINIT,
                      QUAL_DRIVE_SIZE + QUAL_DIR_SIZE + QUAL_NAME_SIZE
                      + QUAL_EXT_SIZE)) ||
      !(path = (char NEAR *)LocalLock(h1)) || !(drive = (char NEAR *)LocalLock(h2)))
   {
      if(path)  LocalUnlock(h1);
      if(drive) LocalUnlock(h2);
      if(h1)    LocalFree(h1);
      if(h2)    LocalFree(h2);
      return(TRUE);
   }

   dir  = drive + QUAL_DRIVE_SIZE;
   name = dir   + QUAL_DIR_SIZE;
   ext  = name  + QUAL_NAME_SIZE;

   lstrcpy(path, lpPath);

   if(*path && path[1]==':' && path[2]=='/')
   {                                 /* special condition - device driver */
      path[2] = '\\';                /* convert 'd:/' to 'd:\'  */
   }
   else if(*path && *path=='\\' && path[1]=='\\')  // NETWORK DRIVE
   {
      p1 = strchr(path, '/');

      if(p1)
      {
         *p1='\\';  // that's all, folks!
      }
   }


   _splitpath(path,drive,dir,name,ext);

   lstrcpy(ext, lpNewExt);

   _makepath(path,drive,dir,name,ext);

   lstrcpy(lpPath, path);      /* puts file name back on top of original! */

   LocalUnlock(h1);
   LocalUnlock(h2);
   LocalFree(h1);
   LocalFree(h2);

   return(FALSE);
}



//   _dos_getdiskfree(drDrive + 1, &dft);

BOOL FAR PASCAL MyGetDiskFreeSpace(WORD wDrive, struct _diskfree_t FAR *lpDFT)
{
#ifdef WIN32

#error "need to write this"

#else  // WIN32

WORD wClusterSize, wFreeClusters, wSectorSize, wTotalClusters;


   // I'm going to try until I'm blue in the face to get this to work...

   if(wDrive == 0) wDrive = _getdrive();


   _asm
   {
      mov ax, 0x3600
      mov dl, BYTE PTR wDrive

      call DOS3Call

      mov wClusterSize, ax
      mov wFreeClusters, bx
      mov wSectorSize, cx
      mov wTotalClusters, dx
   }

   if(wClusterSize == 0xffff)  // this is a sign of an error...
   {
      // because of bugs in Win '95 (which may not be fixed) I'll try
      // ONE MORE TIME to get this thing to work...

      if(GetDriveType(wDrive - 1) != 1)  // not 'ERROR' (unknown or 'ok')
      {
       LPCURDIR lpC = (LPCURDIR)GetDosCurDirEntry(wDrive);  // check for SUBST drive

         if(lpC)
         {
            // if SUBST'ed drive, get info on the drive it points to

            if((lpC->wFlags & CURDIR_TYPE_MASK)!=CURDIR_TYPE_INVALID &&
               (lpC->wFlags & CURDIR_SUBST))
            {
               if(toupper(lpC->pathname[0]) <'A' ||
                  toupper(lpC->pathname[0]) >'Z')
               {
                  // I'm not sure how to handle *THIS* one...


                  MyFreeSelector(SELECTOROF(lpC));
                  return(TRUE);
               }
               else
               {
                  wDrive = toupper(lpC->pathname[0]) - 'A' + 1;

                  MyFreeSelector(SELECTOROF(lpC));

                  _asm
                  {
                     mov ax, 0x3600
                     mov dl, BYTE PTR wDrive

                     call DOS3Call

                     mov wClusterSize, ax
                     mov wFreeClusters, bx
                     mov wSectorSize, cx
                     mov wTotalClusters, dx
                  }

                  if(wClusterSize == 0xffff)  // this is a sign of an error...
                  {
                     return(TRUE);   // error
                  }
               }
            }
            else
            {
               MyFreeSelector(SELECTOROF(lpC));
               return(TRUE);
            }
         }
         else
         {
            return(TRUE); // error
         }
      }
      else
      {
         return(TRUE); // error
      }
   }


   if(lpDFT)
   {
      lpDFT->total_clusters = wTotalClusters;
      lpDFT->avail_clusters = wFreeClusters;
      lpDFT->sectors_per_cluster = wClusterSize;
      lpDFT->bytes_per_sector = wSectorSize;
   }

   return(FALSE);

#endif // WIN32
}




BOOL FAR PASCAL GetDriveLabel(LPSTR lpDest, LPSTR lpSrc)
{
NPSTR npTemp, npSrc, npPath;
struct find_t *npF;
BOOL rval = TRUE;
int i;
DWORD dwDosMem;
WORD wDosSeg;
LPSTR lpDosMem;
REALMODECALL rmc;
LPCURDIR lpC;
static struct {
   WORD info_level;
   DWORD serial_number;
   char  volume[11];
   char  filesys[8];
   char  junk[32];  /* just in case... */
   } sernum;



   *lpDest = 0;                   /* initially assign empty string to dest */

   if(!(npTemp = LocalAllocPtr(LMEM_MOVEABLE, MAX_PATH_SIZE)) ||
      !(npSrc  = LocalAllocPtr(LMEM_MOVEABLE, lstrlen(lpSrc) + 1)) ||
      !(npF    = (struct find_t *)
                 LocalAllocPtr(LMEM_MOVEABLE, sizeof(struct find_t)) ) )
   {
      if(npTemp)  LocalFreePtr(npTemp);
      if(npSrc)   LocalFreePtr(npSrc);

      return(TRUE);                       /* it failed!! (assume no label) */
   }

   lstrcpy(npSrc, lpSrc);                 /* make copy of source string... */

   INIT_LEVEL

   if(!(npPath = _fullpath(npTemp, npSrc, MAX_PATH_SIZE)))
   {
      break;
   }

   if(npPath[0] == '\\' && npPath[1] == '\\' && npPath[2])  // a network drive?
   {
      // for now, assume the path exists....

      // find 1st backslash after '\\' at beginning

      for(i=2; npPath[i] && npPath[i] != '\\'; i++)
        ;

      if(npPath[i])
        i++;  // skip '\' (assume it was found)

      // find next backslash or end of string - this will be the '\\server\share\'

      while(npPath[i] && npPath[i] != '\\')
        i++;

      if(!npPath[i])  // no backslash found
      {
        npPath[i++] = '\\';  // add one (must have terminating '\\')
        npPath[i] = 0;
      }

      strcat(npPath, "*.*");

      goto the_drive_is_valid;  // at this point, 'npPath' is "\\server\sharename\*.*"
   }



   strcpy(npPath + 3, "*.*");                     /* makes it "n:\\*.*" */
   i = (toupper(*npPath) - 'A') + 1;           /* get drive number */
                                            /* drive A==1, B==2, etc. */



   if(GetDriveType(i - 1)<=1)   // drive does not exist
   {
      if(GetDriveType(i - 1) == 0)  // drive type is 'UNKNOWN', actually
      {
         lpC = (LPCURDIR)GetDosCurDirEntry(i);  // check for SUBST drive

         if(lpC)
         {
            // if SUBST'ed drive, get info on the drive it points to

            if((lpC->wFlags & CURDIR_TYPE_MASK)!=CURDIR_TYPE_INVALID &&
               (lpC->wFlags & CURDIR_SUBST))
            {
               if(toupper(lpC->pathname[0]) >='A' &&
                  toupper(lpC->pathname[0]) <='Z')
               {
                  // re-assign drive letter to 'SUBST' drive's letter

                  i = toupper(lpC->pathname[0]) - 'A' + 1;
               }

#ifdef WIN32
               GlobalFreePtr(lpC);
#else // WIN32
               MyFreeSelector(SELECTOROF(lpC));
#endif // WIN32
            }
            else
            {
#ifdef WIN32
               GlobalFreePtr(lpC);
#else // WIN32
               MyFreeSelector(SELECTOROF(lpC));
#endif // WIN32

               rval = TRUE;
               break;
            }
         }
         else
         {
            rval = TRUE;
            break;
         }
      }
      else
      {
         rval = TRUE;
         break;
      }
   }


   /** use the drive # above to get 'drive data' using DOS to verify **/
   /** that the drive is *VALID* before continuing.                  **/

#ifdef WIN32

  rval = (GetLogicalDrives() & (1 << (i - 1))) != 0;  // NOTE:  'i' must NOT be zero!!

#else // WIN32

   _asm
   {
      push ds
      push es
      mov ax, 0x1c00          /* get drive data function */
      mov dl, BYTE PTR i      /* A==1, B==2, etc. */
      call DOS3Call           /* perform the dos call */
      pop es
      pop ds
      mov ax, 0
      jnc drive_is_valid
      mov ax, 1
drive_is_valid:
      mov WORD PTR rval, ax
   }

   if(rval)
   {
      break;         /* it's an error - bail out */
   }

#endif // WIN32



   /** DRIVE IS VALID - OBTAIN LABEL (if it exists) **/

the_drive_is_valid:  // a label (it was easier)



#ifdef WIN32
   {
      DWORD dw1, dw2;

      if(npTemp[0] != '\\' || npTemp[1] != '\\')  // i.e. not a network drive
      {
         npTemp[3] = 0;  // restore 'npTemp' to just the volume letter and root dir
      }
      else
      {
         // terminate path after share name

         char *p1;
         for(p1=npTemp + 2; *p1 && *p1 != '\\'; p1++)
           ;  // find next backslash

         if(*p1)
           p1++;  // go past it

         for(p1=npTemp + 2; *p1 && *p1 != '\\'; p1++)
           ;  // find next backslash

         if(*p1)
           p1++;  // go past it

         *p1 = 0;  // terminate (trailing backslash is required)
      }

      rval = !GetVolumeInformation(npTemp, lpDest, MAX_PATH, &(sernum.serial_number),
                                   &dw1, &dw2, NULL, 0);

      if(!rval)
      {
         if(!*lpDest)
         {
            // FOR NOW, assume NO VOLUME NAME, but print a warning message

            // TODO:  get the correct error # and put into 'rval'

            PrintString("Warning - ");
            PrintErrorMessage(rval);
            PrintString(pCRLFLF);

            rval = 0;
         }

         wsprintf(lpDest + lstrlen(lpDest), " SERIAL NUMBER %04x-%04x",
                  HIWORD(sernum.serial_number),
                  LOWORD(sernum.serial_number));
      }

   }
#else // WIN32

   if(IsChicago || IsNT)
   {
      DWORD dwProc, dwRval, dwHandle, dw1, dw2;

      // use Win32 'GetVolumeInformation'


      if(npTemp[0] != '\\' || npTemp[1] != '\\')  // i.e. not a network drive
      {
         npTemp[3] = 0;  // restore 'npTemp' to just the volume letter and root dir
      }
      else
      {
         // terminate path after share name

         char *p1;
         for(p1=npTemp + 2; *p1 && *p1 != '\\'; p1++)
           ;  // find next backslash

         if(*p1)
           p1++;  // go past it

         for(p1=npTemp + 2; *p1 && *p1 != '\\'; p1++)
           ;  // find next backslash

         if(*p1)
           p1++;  // go past it

         *p1 = 0;  // terminate (trailing backslash is required)
      }


      dwProc = lpGetProcAddress32W(hKernel32, "GetVolumeInformationA");

      dwRval = lpCallProcEx32W(CPEX_DEST_STDCALL | 8, 1|2|8|16|32, dwProc,
                               (DWORD)(LPSTR)npTemp,
                               (DWORD)(LPSTR)lpDest,
                               (DWORD)MAX_PATH,
                               (DWORD)(LPSTR)&(sernum.serial_number),
                               (DWORD)(LPSTR)&dw1,
                               (DWORD)(LPSTR)&dw2,
                               (DWORD)0,
                               (DWORD)0);

      if(!dwRval) // an error?
      {
         dwProc = lpGetProcAddress32W(hKernel32, "GetLastError");

         dwRval = lpCallProcEx32W(CPEX_DEST_STDCALL | 0, 0, dwProc);

         rval = TRUE;
      }
      else
      {
         rval = FALSE;  // no error (OK!)
      }


      if(!rval)
      {
         if(!*lpDest)
         {
            // FOR NOW, assume NO VOLUME NAME, but print a warning message

            // TODO:  get the correct error # and put into 'rval'

            PrintString("Warning - ");
//            PrintErrorMessage(rval);
            PrintString("Volume in drive ");
            PrintString(npTemp);
            PrintString(" has no label");
            PrintString(pCRLFLF);

            rval = 0;
         }

         wsprintf(lpDest + lstrlen(lpDest), " SERIAL NUMBER %04x-%04x",
                  HIWORD(sernum.serial_number),
                  LOWORD(sernum.serial_number));
      }
   }
   else
   {
     rval = MyFindFirst(npPath, _A_VOLID | _A_ARCH, npF);
     *lpDest = 0;
        
     if(!rval)
     {
       lstrcpy(lpDest, npF->name);            /* that was easy, wasn't it? */
        
       MyFindClose(npF);
     }

     rval &= 0xff;

     if(rval && rval!=2 && rval!=3 && rval!=0x12)
     {                                     /* valid 'file not found' errors */
        // FOR NOW, assume NO VOLUME NAME, but print a warning message
      
        PrintString("Warning - ");
        PrintErrorMessage(rval);
        PrintString(pCRLFLF);

        rval = 0;
     }
     else if((i =lstrlen(lpDest))>8)  /* if >8 then the volume ID has a dot in it */
     {
        lstrcpy(lpDest + 8, lpDest + 9);
     }
     else if(lpDest[i - 1]=='.')   /* otherwise, there's a dot at the end(?) */
     {
        lpDest[i - 1] = 0;
     }

     if(uVersion.ver.dos_major>=4)          /* DOS version 4.0 or above... */
     {
        i = (toupper(*npTemp) - 'A') + 1;       /* get drive number */
                                            /* drive A==1, B==2, etc. */
        dwDosMem = GlobalDosAlloc(256 + sizeof(sernum));
        if(!dwDosMem)
        {
           rval = TRUE;
           break;
        }

        wDosSeg = HIWORD(dwDosMem);

        rmc.DS  = wDosSeg;
        rmc.ES  = wDosSeg;
        rmc.SS  = wDosSeg;
        rmc.SP  = (sizeof(sernum)+254) & 0xfffe;   /* new stack pointer - even! */
        rmc.ESI = 0;
        rmc.EDI = 0;     /* why not... */
        rmc.EAX = 0x6900;                               /* int 21H function 69H */
        rmc.EBX = i & 0xff;       /* 'get drive serial number' for drive in 'i' */
        rmc.EDX = 0;                                  /* beginning of DOS block */

        RealModeInt(0x21, &rmc);   /* perform DOS call - get drive serial # */

        lpDosMem = (LPSTR)MAKELP(LOWORD(dwDosMem), 0);
        _hmemcpy((LPSTR)&sernum, lpDosMem, sizeof(sernum));
                                              /* copy info to destination */

        GlobalDosFree(LOWORD(dwDosMem));          /* free DOS memory */

        if(!(rmc.flags & 1))                  /* carry is clear */
        {
           wsprintf(lpDest + lstrlen(lpDest), " SERIAL NUMBER %04x-%04x",
                    HIWORD(sernum.serial_number),
                    LOWORD(sernum.serial_number));
        }
     }
   }

#endif // WIN32

   rval = FALSE;  /* this means it's OK (no error) */

   END_LEVEL

   LocalFreePtr((NPSTR)npF);
   LocalFreePtr(npSrc);
   LocalFreePtr(npTemp);

   return(rval);

}

BOOL FAR PASCAL GetDriveLabelOnly(LPSTR lpDest, LPSTR lpSrc)
{
NPSTR npTemp, npSrc, npPath;
struct find_t *npF;
BOOL rval = TRUE;
int i;
DWORD dwDosMem;
WORD wDosSeg;
LPSTR lpDosMem;
REALMODECALL rmc;
LPCURDIR lpC;
static struct {
   WORD info_level;
   DWORD serial_number;
   char  volume[11];
   char  filesys[8];
   char  junk[32];  /* just in case... */
   } sernum;



   *lpDest = 0;                   /* initially assign empty string to dest */

   if(!(npTemp = LocalAllocPtr(LMEM_MOVEABLE, MAX_PATH_SIZE)) ||
      !(npSrc  = LocalAllocPtr(LMEM_MOVEABLE, lstrlen(lpSrc) + 1)) ||
      !(npF    = (struct find_t *)
                 LocalAllocPtr(LMEM_MOVEABLE, sizeof(struct find_t)) ) )
   {
      if(npTemp)  LocalFreePtr(npTemp);
      if(npSrc)   LocalFreePtr(npSrc);

      return(TRUE);                       /* it failed!! (assume no label) */
   }

   lstrcpy(npSrc, lpSrc);                 /* make copy of source string... */

   INIT_LEVEL

   if(!(npPath = _fullpath(npTemp, npSrc, MAX_PATH_SIZE)))
   {
      break;
   }

   strcpy(npPath + 3, "*.*");                     /* makes it "n:\\*.*" */
   i = (toupper(*npPath) - 'A') + 1;           /* get drive number */
                                            /* drive A==1, B==2, etc. */



   if(GetDriveType(i - 1)<=1)   // drive does not exist
   {
      if(GetDriveType(i - 1) == 0)  // drive type is 'UNKNOWN', actually
      {
         lpC = (LPCURDIR)GetDosCurDirEntry(i);  // check for SUBST drive

         if(lpC)
         {
            // if SUBST'ed drive, get info on the drive it points to

            if((lpC->wFlags & CURDIR_TYPE_MASK)!=CURDIR_TYPE_INVALID &&
               (lpC->wFlags & CURDIR_SUBST))
            {
               if(toupper(lpC->pathname[0]) >='A' &&
                  toupper(lpC->pathname[0]) <='Z')
               {
                  // re-assign drive letter to 'SUBST' drive's letter

                  i = toupper(lpC->pathname[0]) - 'A' + 1;
               }

#ifdef WIN32
               GlobalFreePtr(lpC);
#else // WIN32
               MyFreeSelector(SELECTOROF(lpC));
#endif // WIN32
            }
            else
            {
#ifdef WIN32
               GlobalFreePtr(lpC);
#else // WIN32
               MyFreeSelector(SELECTOROF(lpC));
#endif // WIN32

               rval = TRUE;
               break;
            }
         }
         else
         {
            rval = TRUE;
            break;
         }
      }
      else
      {
         rval = TRUE;
         break;
      }
   }


   /** use the drive # above to get 'drive data' using DOS to verify **/
   /** that the drive is *VALID* before continuing.                  **/

#ifdef WIN32

  rval = (GetLogicalDrives() & (1 << (i - 1))) != 0;  // NOTE:  'i' must NOT be zero!!

#else // WIN32

   _asm
   {
      push ds
      push es
      mov ax, 0x1c00          /* get drive data function */
      mov dl, BYTE PTR i      /* A==1, B==2, etc. */
      call DOS3Call           /* perform the dos call */
      pop es
      pop ds
      mov ax, 0
      jnc drive_is_valid
      mov ax, 1
drive_is_valid:
      mov WORD PTR rval, ax
   }

   if(rval)
   {
      break;         /* it's an error - bail out */
   }

#endif // WIN32

   /** DRIVE IS VALID - OBTAIN LABEL (if it exists) **/

#ifdef WIN32
   {
      DWORD dw1, dw2;

      if(npTemp[0] != '\\' || npTemp[1] != '\\')  // i.e. not a network drive
      {
         npTemp[3] = 0;  // restore 'npTemp' to just the volume letter and root dir
      }
      else
      {
         // terminate path after share name

         char *p1;
         for(p1=npTemp + 2; *p1 && *p1 != '\\'; p1++)
           ;  // find next backslash

         if(*p1)
           p1++;  // go past it

         for(p1=npTemp + 2; *p1 && *p1 != '\\'; p1++)
           ;  // find next backslash

         if(*p1)
           p1++;  // go past it

         *p1 = 0;  // terminate (trailing backslash is required)
      }

      rval = !GetVolumeInformation(npTemp, lpDest, MAX_PATH, &(sernum.serial_number),
                                   &dw1, &dw2, NULL, 0);

      if(!rval)
      {
         if(!*lpDest)
         {
            lstrcpy(lpDest, "{NO LABEL!}");
         }

         wsprintf(lpDest + lstrlen(lpDest), " SERIAL NUMBER %04x-%04x",
                  HIWORD(sernum.serial_number),
                  LOWORD(sernum.serial_number));
      }
   
   }
#else // WIN32

   if(IsChicago || IsNT)
   {
      DWORD dwProc, dwRval, dwHandle, dw1, dw2;

      // use Win32 'GetVolumeInformation'


      if(npTemp[0] != '\\' || npTemp[1] != '\\')  // i.e. not a network drive
      {
         npTemp[3] = 0;  // restore 'npTemp' to just the volume letter and root dir
      }
      else
      {
         // terminate path after share name

         char *p1;
         for(p1=npTemp + 2; *p1 && *p1 != '\\'; p1++)
           ;  // find next backslash

         if(*p1)
           p1++;  // go past it

         for(p1=npTemp + 2; *p1 && *p1 != '\\'; p1++)
           ;  // find next backslash

         if(*p1)
           p1++;  // go past it

         *p1 = 0;  // terminate (trailing backslash is required)
      }


      dwProc = lpGetProcAddress32W(hKernel32, "GetVolumeInformationA");

      dwRval = lpCallProcEx32W(CPEX_DEST_STDCALL | 8, 1|2|8|16|32, dwProc,
                               (DWORD)(LPSTR)npTemp,
                               (DWORD)(LPSTR)lpDest,
                               (DWORD)MAX_PATH,
                               (DWORD)(LPSTR)&(sernum.serial_number),
                               (DWORD)(LPSTR)&dw1,
                               (DWORD)(LPSTR)&dw2,
                               (DWORD)0,
                               (DWORD)0);

      if(!dwRval) // an error?
      {
         dwProc = lpGetProcAddress32W(hKernel32, "GetLastError");

         dwRval = lpCallProcEx32W(CPEX_DEST_STDCALL | 0, 0, dwProc);

         rval = TRUE;
      }
      else
      {
         rval = FALSE;  // no error (OK!)
      }


      if(!rval)
      {
         if(!*lpDest)
         {
            lstrcpy(lpDest, "{NO LABEL!}");
         }
         else
         {
            wsprintf(lpDest + lstrlen(lpDest), " SERIAL NUMBER %04x-%04x",
                     HIWORD(sernum.serial_number),
                     LOWORD(sernum.serial_number));
         }
      }
   }
   else
   {
      rval = MyFindFirst(npPath, _A_VOLID | _A_ARCH, npF);
      *lpDest = 0;

      if(!rval)
      {
         lstrcpy(lpDest, npF->name);            /* that was easy, wasn't it? */

         MyFindClose(npF);
      }

      rval &= 0xff;

      if(rval && rval!=2 && rval!=3 && rval!=0x12)
      {                                     /* valid 'file not found' errors */

         lstrcpy(lpDest, "{NO LABEL!}");
         rval = 0;
      }
      else if(rval)
      {
         lstrcpy(lpDest, "{* ERROR *}");
         rval = 0;
      }
      else if((i =lstrlen(lpDest))>8)  /* if >8 then the volume ID has a dot in it */
      {
         lstrcpy(lpDest + 8, lpDest + 9);
      }
      else if(lpDest[i - 1]=='.')   /* otherwise, there's a dot at the end(?) */
      {
         lpDest[i - 1] = 0;
      }
    
      if(uVersion.ver.dos_major>=4)          /* DOS version 4.0 or above... */
      {
         i = (toupper(*npTemp) - 'A') + 1;       /* get drive number */
                                               /* drive A==1, B==2, etc. */
         dwDosMem = GlobalDosAlloc(256 + sizeof(sernum));
         if(!dwDosMem)
         {
            rval = TRUE;
            break;
         }

         wDosSeg = HIWORD(dwDosMem);

         rmc.DS  = wDosSeg;
         rmc.ES  = wDosSeg;
         rmc.SS  = wDosSeg;
         rmc.SP  = (sizeof(sernum)+254) & 0xfffe;   /* new stack pointer - even! */
         rmc.ESI = 0;
         rmc.EDI = 0;     /* why not... */
         rmc.EAX = 0x6900;                               /* int 21H function 69H */
         rmc.EBX = i & 0xff;       /* 'get drive serial number' for drive in 'i' */
         rmc.EDX = 0;                                  /* beginning of DOS block */

         RealModeInt(0x21, &rmc);   /* perform DOS call - get drive serial # */

         lpDosMem = (LPSTR)MAKELP(LOWORD(dwDosMem), 0);
         _hmemcpy((LPSTR)&sernum, lpDosMem, sizeof(sernum));
                                                 /* copy info to destination */

         GlobalDosFree(LOWORD(dwDosMem));          /* free DOS memory */
            
         if(!(rmc.flags & 1))                  /* carry is clear */
         {
            wsprintf(lpDest + lstrlen(lpDest), " SERIAL NUMBER %04x-%04x",
                     HIWORD(sernum.serial_number),
                     LOWORD(sernum.serial_number));
         }
      }
   }

#endif // WIN32

   rval = FALSE;  /* this means it's OK (no error) */

   END_LEVEL

   LocalFreePtr((NPSTR)npF);
   LocalFreePtr(npSrc);
   LocalFreePtr(npTemp);

   return(rval);

}


LPTREE FAR PASCAL GetDirTree(LPSTR lpPath)
{
NPSTR pTemp;
LPTREE lpTree, lpT2;
WORD wTreeSize, wNext, wErr;
DWORD dwTickCount;
HCURSOR hCursor;


   dwTickCount = GetTickCount();


   pTemp = LocalAllocPtr(LMEM_MOVEABLE | LMEM_ZEROINIT,
                         MAX_PATH_SIZE);                    /* temp buf */

   DosQualifyPath(pTemp, lpPath);  /* get 'qualified' path */

   if(!*pTemp)
   {
      LocalFreePtr(pTemp);          /* free remaining resources */

      return(NULL);
   }

   hCursor = MySetCursor(LoadCursor(NULL, IDC_WAIT));

   lpTree = (LPTREE)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                   INITIAL_TREE_SIZE * sizeof(TREE));

   wNext = 0;
   wTreeSize = INITIAL_TREE_SIZE;

   wErr = GetDirTreeLevel((LPTREE FAR *)&lpTree, pTemp,
                          (WORD FAR *)&wTreeSize, (WORD FAR *)&wNext,
                          (DWORD FAR *)&dwTickCount);

   LocalFreePtr(pTemp);          /* free remaining resources */

   MySetCursor(hCursor);

   if(!wErr)                 /* result was good */
   {
      lpT2 = (LPTREE)GlobalReAllocPtr(lpTree, (wNext + 1) * sizeof(TREE),
                                      GMEM_MOVEABLE | GMEM_ZEROINIT);

      if(lpT2) return(lpT2);
      else     return(lpTree);
   }
   else                      /* result was IN ERROR */
   {
      GlobalFreePtr(lpTree);
      return(NULL);
   }

}


BOOL FAR PASCAL GetDirTreeLevel(LPTREE FAR *lplpTree, NPSTR pPath,
                                WORD FAR *lpwTreeSize, WORD FAR *lpwNext,
                                DWORD FAR *lpdwTickCount)
{
BOOL rval;
struct find_t NEAR *pFF;
LPTREE lpTree;
NPSTR p0, p1;
WORD wCurrent, wPrevious = 0xffff;   /* a 'marker' */


   pFF = (struct find_t NEAR *)LocalAllocPtr(LMEM_MOVEABLE | LMEM_ZEROINIT,
                                             sizeof(struct find_t));
   if(!pFF) return(TRUE);

   p0 = p1 = pPath + strlen(pPath);
   if(*(p1 - 1)!='\\')
   {
      *p1++ = '\\';
      *p1 = 0;
   }

   strcpy(p1, "*.*");   /* pick up ALL files!! */

   rval = MyFindFirst(pPath, (~_A_VOLID) & 0xff, pFF);

   // do a possible yield RIGHT AFTER the 'MyFindFirst()' in case it failed...

   if((GetTickCount() - *lpdwTickCount) > 50)
   {
      if(LoopDispatch() || ctrl_break)
      {
         LocalFreePtr((NPSTR)pFF);                /* free resources */
         return(TRUE);              // error (break)
      }

      *lpdwTickCount = GetTickCount();
   }


   if(!rval)
   {
      while(!rval)
      {
         if((GetTickCount() - *lpdwTickCount) > 50)
         {
            if(LoopDispatch() || ctrl_break)
            {
               LocalFreePtr((NPSTR)pFF);                /* free resources */
               return(TRUE);              // error (break)
            }

            *lpdwTickCount = GetTickCount();
         }


         if((pFF->attrib & _A_SUBDIR) &&
            (pFF->name[0]!='.' || (pFF->name[1]!=0 && pFF->name[1]!='.')))
//            strcmp(pFF->name,".")!=0 && strcmp(pFF->name,"..")!=0)
         {

             /* a valid directory was found!  Add to the list and recurse */

            if((*lpwNext + 1)>=*lpwTreeSize)
            {


               lpTree = (LPTREE)GlobalReAllocPtr(*lplpTree,
                                                 (*lpwNext + 256) * sizeof(TREE),
                                                 GMEM_MOVEABLE | GMEM_ZEROINIT);

               if(!lpTree)
               {
                  LocalFreePtr((NPSTR)pFF);
                  return(TRUE);           /* error - could not get enough RAM */
               }
               else
                  *lplpTree = lpTree;  /* new pointer! */

            }
            else
               lpTree = *lplpTree;     /* for convenience and speed, hopefully */

            wCurrent = (*lpwNext)++;   /* index for new (current) entry */

            if(wPrevious!=0xffff)      /* not first time through */
            {
               lpTree[wPrevious].wSibling = wCurrent;
            }

            wPrevious = wCurrent;              /* to help build the chains */

            lstrcpy(lpTree[wCurrent].szDir, pFF->name);  /* copy the name */

            lpTree[wCurrent].wSibling = NULL;  /* 'end of chain' marker */
            lpTree[wCurrent].wChild   = NULL;  /* and, initially, here also */

                        /** Recurse into this sub-directory **/

            strcpy(p1, pFF->name);     /* copy name onto end of previous path */

            if(GetDirTreeLevel(lplpTree, pPath, lpwTreeSize, lpwNext, lpdwTickCount))
            {
               LocalFreePtr((NPSTR)pFF);
               return(TRUE);
            }

            if((wCurrent + 1)<*lpwNext)     /* entries were added!! */
            {
               lpTree = *lplpTree;          /* in case it changed... */

               lpTree[wCurrent].wChild = wCurrent + 1;  /* 1st child entry */
            }

         }

         rval = MyFindNext(pFF);                          /* get next item */
      }

      MyFindClose(pFF);
   }

   LocalFreePtr((NPSTR)pFF);                /* free resources */
   *p0 = 0;                /* restore path string to the way it was before */

   return(FALSE);                      /* no error - good result! */

}



WORD FAR PASCAL GetDirList(LPSTR lpFileSpec, int attrib,
                           LPDIR_INFO FAR *lplpDirInfo, LPSTR lpPathBuffer)
{
   return(GetDirList0(lpFileSpec, attrib, lplpDirInfo, lpPathBuffer,
                      TRUE, NULL));
}



WORD FAR PASCAL GetDirList0(LPSTR lpFileSpec, int attrib,
                            LPDIR_INFO FAR *lplpDirInfo, LPSTR lpPathBuffer,
                            BOOL wQualifyFlag, LPSTR FAR *lpLFNInfo)
{
LPDIR_INFO lpDirInfo;
LPSTR lpDI0;
DIR_INFO _huge *lpDI2;
NPSTR np1, np2, np3;
BOOL rval;
WORD count=0;
DWORD buf_size, cur_size, dwTickCount;
HCURSOR hOldCursor;
int i, attrib0, attrib1;



   dwTickCount = GetTickCount();

   hOldCursor = MySetCursor(LoadCursor(NULL, IDC_WAIT));

   *lplpDirInfo = NULL;                                  /* initial value */

   if(!(lpDirInfo = (LPDIR_INFO)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                               DIRINFO_MIN_MEM)) ||
      !(np1 = LocalAllocPtr(LMEM_MOVEABLE, 256)))
   {
      PrintErrorMessage(793);
      UpdateWindow(hMainWnd);

      if(lpDirInfo)  GlobalFreePtr((LPSTR)lpDirInfo);

      MySetCursor(hOldCursor);

      return(DIRINFO_ERROR);
   }

   INIT_LEVEL

   if(QualifyPath(np1, lpFileSpec, wQualifyFlag))
      break;

   buf_size = GlobalSizePtr(((LPSTR)lpDirInfo));
   cur_size = 0;

   attrib0 = attrib & 0xff;
   if(!attrib0)
   {
      attrib0 = _A_ARCH | _A_SYSTEM | _A_RDONLY;  // these attribs are ok
   }

   attrib1 = (((UINT)attrib) >> 8) & 0xff;      // high byte - 'must' attribs

// TEMPORARY - DEBUG

   if(attrib0 & _A_VOLID)
   {
      OutputDebugString("Warning - 'VOLID' bit set in 'GetDirList0'\r\n");
   }


   rval = MyFindFirst(np1, attrib0, &ff);
   lpDI2 = (DIR_INFO _huge *)lpDirInfo;

   if(!rval)
   {
      while(!rval)
      {
         if((GetTickCount() - dwTickCount) > 100)
         {
            if(LoopDispatch() || ctrl_break)
            {
//               ctrl_break = FALSE;    // eat the control break

               rval = FALSE;          // return with error
               break;
            }

            dwTickCount = GetTickCount();
         }

         if((attrib1 & ff.attrib) == attrib1)
         {
            cur_size += sizeof(DIR_INFO);
            if(cur_size>=buf_size)  /* time to 're-alloc' */
            {
               lpDI0 = (LPSTR)GlobalReAllocPtr((LPSTR)lpDirInfo, 
                                               cur_size + 64L * sizeof(DIR_INFO),
                                               GMEM_MOVEABLE | GMEM_ZEROINIT);
               if(!lpDI0)
               {
                  GlobalCompact((DWORD)-1);
                  lpDI0 = (LPSTR)GlobalReAllocPtr((LPSTR)lpDirInfo,
                                                  cur_size + 64L * sizeof(DIR_INFO),
                                                  GMEM_MOVEABLE | GMEM_ZEROINIT);
                  if(!lpDI0)
                  {
                     rval = FALSE;       /* indicates bailout of loop on error */
                     PrintErrorMessage(794);
                     UpdateWindow(hMainWnd);
                     GlobalFreePtr(lpDirInfo);
                     break;
                  }

               }

               lpDirInfo = (LPDIR_INFO)lpDI0;                /* new pointer */
               buf_size = GlobalSizePtr(lpDI0);

               lpDI2 = ((HPDIR_INFO)lpDirInfo) + count;
            }

            count++;   /* at this time, update the entry counter */

                                  /* find the '.' */

            np3 = "   ";   /* initial value - points to 3 blanks */

            if(*ff.name!='.')
            {
               for(np2=ff.name, i=0; i<=8 && *np2; i++, np2++)
               {
                  if(*np2=='.')
                  {
                     np3 = np2 + 1;   /* 'i' contains file name width */
                     break;
                  }
               }
            }
            else
            {
               i = lstrlen(ff.name);                  /* width for file name */
            }

                                   /** Name **/

            _hmemset(lpDI2->name, ' ', sizeof(lpDI2->name));

            i = min(i, sizeof(lpDI2->name));          /* prevent over-runs */

            _hmemcpy(lpDI2->name, ff.name, i); /* copy name */

                                 /** Extension **/

            _hmemset(lpDI2->ext, ' ', sizeof(lpDI2->ext));

            i = min(strlen(np3), sizeof(lpDI2->ext));

            if(i) _hmemcpy(lpDI2->ext, np3, i);             /* copy extension */


                                 /** Full Name **/

            _hmemset(lpDI2->fullname, 0,  sizeof(lpDI2->fullname));

            i = min(strlen(ff.name), sizeof(lpDI2->fullname));

            _hmemcpy(lpDI2->fullname, ff.name, i);


            lpDI2->attrib = ff.attrib;
            lpDI2->date   = ff.wr_date;
            lpDI2->time   = ff.wr_time;
            lpDI2->size   = ff.size;

            lpDI2++;

         }

         rval = MyFindNext(&ff); /* it helps to have this here... */
      }

      MyFindClose(&ff);
   }

   if(!rval)  break;           /* this means I bailed out on error (above) */

   MySetCursor(hOldCursor);

   if(lpPathBuffer)                       /* if user wants path (not NULL) */
   {
    LPSTR lp1;

      lstrcpy(lpPathBuffer, np1);     /* make a copy of the qualified path */

      lp1 = _fstrrchr(lpPathBuffer, '\\');    /* find the 'last' backslash */

      if(lp1)  *lp1 = 0;  /* this puts the 'path only' into 'lpPathBuffer' */
   }

   LocalFreePtr(np1);        /* free local buffer */

   *lplpDirInfo = lpDirInfo; /* return the 'DIR_INFO' pointer */
   return(count);            /* returns the number of files found */

   END_LEVEL                              /* only errors get to this point */

   if(lpPathBuffer)  GlobalFreePtr((LPSTR)lpPathBuffer);
   if(np1)           LocalFreePtr(np1);

   MySetCursor(hOldCursor);
   return(DIRINFO_ERROR);

}




void FAR PASCAL ShortToLongName(LPSTR lpName, LPSTR lpPath)
{
LPSTR lp1;
char FAR *tbuf=NULL, FAR *tbuf2=NULL;



   if(!IsChicago && !IsNT) return;  // ignore if not chicago

   tbuf = (LPSTR)GlobalAllocPtr(GPTR, MAX_PATH * 4);
   if(!tbuf)
     return;

   tbuf2 = tbuf + 2 * MAX_PATH;

   if(!lpPath)
   {
      lstrcpy(tbuf, lpName);
   }
   else if(!*lpPath)
   {
      lp1 = _fstrrchr(lpName, '\\');
      if(!lp1) lp1 = lpName;
      else     lp1++;

      lstrcpy(tbuf, lp1);
   }
   else
   {
      lp1 = _fstrrchr(lpName, '\\');
      if(!lp1) lp1 = lpName;
      else     lp1++;

      lstrcpy(tbuf, lpPath);
      if(tbuf[lstrlen(tbuf)-1]!='\\') lstrcat(tbuf, "\\");

      lstrcat(tbuf, lp1);
   }

   if(!GetLongName(tbuf,tbuf2) && *tbuf2)
   {
      lstrcpy(lpName, tbuf2);  // only if NOT a blank name!
   }

   GlobalFreePtr(tbuf);
}



  /***********************************************************************/
  /*                         UpdateFileName()                            */
  /*  function to create drive:\path\name from 'pattern' and 'filename'  */
  /***********************************************************************/

BOOL FAR PASCAL UpdateFileName(LPSTR lpDest, LPSTR lpPattern,LPSTR lpName)
{
   // assign 'bDirCheckFlag' to TRUE to add "*.*" to dir names in 'lpPattern'
   // that don't already have wildcards in them.

   return(UpdateFileName2(lpDest, lpPattern, lpName, TRUE));
}


BOOL FAR PASCAL UpdateFileName2(LPSTR lpDest, LPSTR lpPattern,LPSTR lpName,
                                BOOL bDirCheckFlag)
{
HLOCAL h1=(HLOCAL)NULL,h2=(HLOCAL)NULL;
NPSTR path, drive, dir, name, ext, np1;
NPSTR path2, drive2, dir2, name2, ext2;
struct find_t NEAR *npff;
BOOL rval, NameHasAsterisk=FALSE, ExtHasAsterisk=FALSE;
int i;


   if(!(h1=LocalAlloc(LMEM_MOVEABLE | LMEM_ZEROINIT, 2*QUAL_PATH_SIZE)) ||
      !(h2=LocalAlloc(LMEM_MOVEABLE | LMEM_ZEROINIT,
                      2*(QUAL_DRIVE_SIZE + QUAL_DIR_SIZE + QUAL_NAME_SIZE
                      + QUAL_EXT_SIZE))) ||
      !(path = (char NEAR *)LocalLock(h1)) || !(drive = (char NEAR *)LocalLock(h2)))
   {
      if(path)  LocalUnlock(h1);
      if(drive) LocalUnlock(h2);
      if(h1)    LocalFree(h1);
      if(h2)    LocalFree(h2);
      *lpDest = 0;
      return(TRUE);
   }


   dir  = drive + QUAL_DRIVE_SIZE;
   name = dir   + QUAL_DIR_SIZE;
   ext  = name  + QUAL_NAME_SIZE;

   path2  = path   + QUAL_PATH_SIZE;     /* a second set of buffers */
   drive2 = ext    + QUAL_EXT_SIZE;
   dir2   = drive2 + QUAL_DRIVE_SIZE;
   name2  = dir2   + QUAL_DIR_SIZE;
   ext2   = name2  + QUAL_NAME_SIZE;


   npff = (struct find_t NEAR *)dir;

   lstrcpy(path, lpPattern);    /* make a copy of the pattern in local mem */

   _splitpath(path,drive,dir,name,ext);


   if(*drive==0 && (*dir!='\\' || dir[1]!='\\'))
   {
      drive[0] = (char)(_getdrive() - 1 + 'A');
      drive[1] = ':';
      drive[2] = 0;
   }


   if(*dir!='\\')
   {
      _lgetdcwd(toupper(*drive) - 'A' + 1, path2, QUAL_PATH_SIZE);

      if(path2[strlen(path2) - 1]!='\\')
         strcat(path2, "\\");

      strcat(path2, dir);

      strcpy(dir, path2 + 2);  /* copy back to 'dir' */
   }


   if(*name==0 && *ext==0)
   {
      strcpy(name,"*.*");    /* wildcard file names for root directories! */
   }

   _makepath(path,drive,dir,name,ext);                 /* re-assemble path */


   if(bDirCheckFlag)
   {
      if((*name!=0 || *ext!=0)
          && strpbrk(name, "?*")==(char *)NULL
          && strpbrk(ext,  "?*")==(char *)NULL)
      {
            /** There's a file name/ext, but no wildcards so check to  **/
            /** see if it represents a path, and if so add "*.*" to it **/

         if(!MyFindFirst(path, ~_A_VOLID & 0xff, npff))
         {
            if(npff->attrib & _A_SUBDIR)      /* it's a sub-directory!! */
            {
               strcat(path, "\\*.*");
            }

            MyFindClose(npff);
         }

      }
   }

   rval = FALSE;


   /*** NOW -- One... More... TIME!!!!  Split the path and substitute  ***/
   /*** for any wildcard characters with characters from the file name ***/

   _splitpath(path,drive,dir,name,ext);

   /* Next, convert any '*' to '????????' and then trim them appropriately */

   if(np1 = strchr(name, '*'))
   {
      NameHasAsterisk = TRUE;

//      strcpy(np1, "????????");

      if((np1 - name) < (QUAL_NAME_SIZE - 1))
      {
         _hmemset(np1, '?', QUAL_NAME_SIZE - (np1 - name) - 1);
      }

      name[QUAL_NAME_SIZE - 1] = 0;
   }
   if(np1 = strchr(ext, '*'))
   {
      ExtHasAsterisk = TRUE;

//      strcpy(np1, "???");

      if((np1 - ext) < (QUAL_EXT_SIZE - 1))
      {
         _hmemset(np1, '?', QUAL_EXT_SIZE - (np1 - ext) - 1);
      }

      ext[QUAL_EXT_SIZE - 1] = 0;      /* 'ext' begins with a dot */
   }

//   // zero out remainder of buffers (for later)
//
//   _hmemset(name + strlen(name) + 1, 0, QUAL_NAME_SIZE - strlen(name) - 1);
//   _hmemset(ext + strlen(ext) + 1, 0, QUAL_EXT_SIZE - strlen(ext) - 1);



 /* Third, split the path used for patterns and make necessary adjustments */

   lstrcpy(path2, lpName);

   _splitpath(path2, drive2, dir2, name2, ext2);

   if((!IsChicago && !IsNT) || !NameHasAsterisk)
   {
      name2[8] = 0;
      strcat(name2, "       ");
      name2[8] = 0;  /* make sure it's padded with spaces */
   }

   if(*ext2==0)
   {
      strcpy(ext2, ".   ");
   }
   else if((!IsChicago && !IsNT) || !ExtHasAsterisk)
   {
      ext2[4] = 0;
      strcat(ext2, "   ");
      ext2[4] = 0;   /* make sure it's padded with spaces also */
   }

   // zero out remainder of 'name2' and 'ext2' buffers

   _hmemset(name2 + strlen(name2), 0, QUAL_NAME_SIZE - strlen(name2));
   _hmemset(ext2 + strlen(ext2), 0, QUAL_EXT_SIZE - strlen(ext2));


   // NOW, substitute '?' in name and ext for original name's characters
   // taking into account the existence or absence of a '*' in name or ext

   for(i=0; i < (QUAL_NAME_SIZE - 1); i++)
   {
      if(name[i]=='?') // ||
//         ((IsChicago || IsNT) && name[i]==0 && NameHasAsterisk))
      {
//         if(name[i]==0)   name[i + 1] = 0;

         if(name2[i])     name[i] = name2[i];
         else if(i < 8)   name[i] = ' ';
         else             name[i] = 0;

//         else if(name[i]) name[i] = ' ';
      }

      if(name[i]==0) break;

   }

   for(i=0; i < (QUAL_EXT_SIZE - 1); i++)
   {
      if(i==0 || ext[i]=='?') // ||
//         ((IsChicago || IsNT) && ext[i]==0 && ExtHasAsterisk))
      {
//         if(ext[i]==0)    ext[i + 1] = 0;

         if(ext2[i])      ext[i] = ext2[i];
         else if(i < 4)   ext[i] = ' ';
         else             ext[i] = 0;

//         else if(ext[i])  ext[i] = ' ';
      }

      if(ext[i]==0) break;
   }


         /* if the pattern has a directory, concatinate it onto the */
         /* current path (unless it begins with a '\\')             */

   if(*dir2!=0 && *dir2!='\\')
   {
      if(dir[strlen(dir) - 1]!='\\')
      {
         strcat(dir, "\\");
         strcat(dir, dir2);
      }
      else
      {
         strcat(dir, dir2);
         strcat(dir, "\\");
      }
   }

   for(np1=name+strlen(name)-1; *np1<=' ' && np1>=name; np1--)
   {
      *np1 = 0;              /* trim trailing spaces off of name */
   }

   for(np1=ext+strlen(ext)-1; *np1<=' ' && np1>=ext; np1--)
   {
      *np1 = 0;              /* trim trailing spaces off of ext */
   }

   // UPDATE 12/15/94 (bug fix for character device names)
   //
   // FINAL CHECK!  If the resulting file name is a CHARACTER DEVICE,
   //               modify the 'drive' and 'dir' elements to point to
   //               the root directory (see also 'DosQualifyPath()')

   if(!*ext || (ext[0]=='.' && ext[1]==0))
   {
    HFILE hFile;

      //  files with no extension may be devices; if this is
      //  possible check for device with same name and return
      //  only the name with no drive or directory if it is
      //  a character device name.

#ifdef WIN32

      hFile = (HFILE)CreateFile(name, 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                NULL);

      if(hFile != (HFILE)INVALID_HANDLE_VALUE)
      {
        if(GetFileType((HANDLE)hFile) == FILE_TYPE_CHAR)
        {
          // character device

          _fstrupr(name);  // convert name to UPPER CASE

          *dir = 0;        // remove drive and directory
          *drive = 0;
        }

        CloseHandle(hFile);
      }

#else  // WIN32

      // attempt to open file as device driver on boot drive

      if(HIWORD(GetVersion()) >= 0x400)  // dos 4.0 or greater
      {
         _asm mov ax, 0x3305   // get boot drive
         _asm int 0x21
         _asm mov dh, 0
         _asm mov i, dx

         path2[0] = 'A' + (char)i - 1;
         path2[1] = ':';
      }
      else
      {
         GetSystemDirectory(path2, QUAL_PATH_SIZE);
      }

      lstrcpy(path2 + 2, name);

      hFile = _lopen(path2, OF_READ | OF_SHARE_DENY_NONE);  // does it exist?

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
#endif // WIN32
   }



   _makepath(path, drive, dir, name, ext);

   lstrcpy(lpDest, path);         /* copy path back to destination! */


   LocalUnlock(h1);
   LocalUnlock(h2);
   LocalFree(h1);
   LocalFree(h2);

   return(rval);

}





/***************************************************************************/
/*                                                                         */
/*    GetExtErrMessage() - function which returns resource strings that    */
/*                         correspond to an extended error description.    */
/*                                                                         */
/***************************************************************************/

void FAR PASCAL GetExtErrMessage(LPSTR lpMsgBuf)
{
int ierr, iclass, iaction, ilocus;

#ifdef WIN32

   DWORD dwErr = GetLastError();

   ierr = (int)(dwErr & 0xffff);           // bottom 16 bits only

   iclass = (int)(dwErr >> 30);            // top 2 bits only (severity)
   ilocus = (int)((dwErr >> 16) & 0xfff);  // 12 bits in upper word

   switch(iclass)  // convert to DOS value
   {
     case 0:
       iclass = 0;
       iaction = 0;
       break;

     case 1:
       iclass = 14;  // WIN32 informational only
       iaction = 6;  // message may be ignored
       braek;

     case 2:
       iclass = 15;  // WIN32 warning
       iaction = 6;  // message may be ignored
       break;

     case 3:
       iclass = 7;   // Application Program Error!
       iaction = 4;  // *** SERIOUS ERROR CONDITION ***  Application should be terminated!
       break;

     default:
       iclass = 13;
       iaction = 8;
   }

   switch(ilocus)
   {
     case FACILITY_WINDOWS:
       ilocus = 7;
       break;

     case FACILITY_STORAGE:
       ilocus = 2;
       break;

     case FACILITY_RPC:
       ilocus = 9;
       break;

     case FACILITY_WIN32:
       ilocsu = 8;
       break;

     case FACILITY_CONTROL:
       ilocus = 10;
       break;

     case FACILITY_NULL:
       ilocus = 0;
       break;

     case FACILITY_ITF:
       ilocus = 11;
       break;

     case FACILITY_DISPATCH:
       ilocus = 12;
       break;

     default:
       ilocus = 0;
   }


#else // WIN32

   /* ierr = dosexterr(&last_error); */

   _asm
   {
      push ds        /* these registers are destroyed */
      push es
      push bp
      push si
      push di

      mov ax, 0x5900
      mov bx, 0

      call DOS3Call

      pop di         /* restore the registers 'push'ed above */
      pop si
      pop bp
      pop es
      pop ds

      mov ah, 0      // ensure error code is BELOW 256!!! (MS-DOS)

      mov WORD PTR ierr, ax     /* store ext err code in 'ierr' */
   
      mov WORD PTR last_error.exterror, ax
      mov BYTE PTR last_error.errclass, bh
      mov BYTE PTR last_error.action, bl
      mov BYTE PTR last_error.locus, ch
   }

   iclass = last_error.errclass;
   iaction = last_error.action;
   ilocus = last_error.locus;

#endif // WIN32

   if(!ierr || ierr > 127 ||
      !LoadString(hInst, ierr, work_buf, sizeof(work_buf)))
   {
      lstrcpy(lpMsgBuf, "?Unknown or unspecified error (bad command, file name, or path)\r\n");
      return;
   }

   lstrcpy(lpMsgBuf, work_buf);
   lstrcat(lpMsgBuf, " - ");

   if(!LoadString(hInst, iclass + CMDERROR_CLASS,
                  work_buf, sizeof(work_buf)))
   {
      lstrcat(lpMsgBuf, "** Error class cannot be determined! **");
   }
   else
      lstrcat(lpMsgBuf, work_buf);

   lstrcat(lpMsgBuf, "\r\n");

   if(!LoadString(hInst, iaction + CMDERROR_ACTION,
                  work_buf, sizeof(work_buf)))
   {
      lstrcat(lpMsgBuf, "Unable to determine corrective action!");
   }
   else
      lstrcat(lpMsgBuf, work_buf);

   lstrcat(lpMsgBuf, "\r\n");


   if(!LoadString(hInst, ilocus + CMDERROR_LOCUS,
                  work_buf, sizeof(work_buf)))
   {
      lstrcat(lpMsgBuf, "Unable to determine source of error!");
   }
   else
      lstrcat(lpMsgBuf, work_buf);

   lstrcat(lpMsgBuf, "\r\n");

   return;

}


static WORD wErrIndex = 0;

static struct _DOSERROR save_error, pErr[32];

typedef struct tagDOSDPL {
   WORD _ax, _bx, _cx, _dx, _si, _di, _ds, _es;
   WORD reserved;
   WORD wMachineID;
   WORD wPSP;
   } DOSDPL, FAR *LPDOSDPL;

void SaveDosErrorCodes(void)
{

#ifdef WIN32

   ((DWORD *)&save_error) = GetLastError();

#else  // WIN32

   _asm
   {
      push ds        /* these registers are destroyed */
      push es
      push bp
      push si
      push di

      mov ax, 0x5900
      mov bx, 0

      call DOS3Call

      pop di         /* restore the registers 'push'ed above */
      pop si
      pop bp
      pop es
      pop ds

      mov ah, 0      // ensure error code is BELOW 256!!! (MS-DOS)

      mov WORD PTR save_error.exterror, ax
      mov BYTE PTR save_error.errclass, bh
      mov BYTE PTR save_error.action, bl
      mov BYTE PTR save_error.locus, ch
   }

#endif // WIN32

   if(wErrIndex < sizeof(pErr)/sizeof(*pErr))
   {
      pErr[wErrIndex++] = save_error;
   }

}

void RestoreDosErrorCodes(void)
{
#ifdef WIN32

   if(wErrIndex)
   {
      save_error = pErr[--wErrIndex];
   }

   SetLastError(*((DWORD *)&save_error));

#else // WIN32

static DOSDPL dpl;


   if(wErrIndex)
   {
      save_error = pErr[--wErrIndex];
   }

   dpl._ax = save_error.exterror;
   dpl._bx = (save_error.errclass << 8) | (save_error.action & 0xff);
   dpl._cx = (save_error.locus << 8);

   dpl._dx = 0;
   dpl._ds = 0;
   dpl._si = 0;
   dpl._es = 0;
   dpl._di = 0;

   dpl.reserved = 0;
   dpl.wMachineID = 0;
   dpl.wPSP = GetCurrentPDB();


   // SERVER CALL (AH=5DH) function 0AH - set error codes

   _asm
   {
      mov ax, 0x5d0a
      mov dx, OFFSET dpl
      call DOS3Call
   }
#endif // WIN32

}





/***************************************************************************/
/*                                                                         */
/*     Low-Level functions (Memory Allocation, String manipulation)        */
/*                                                                         */
/***************************************************************************/



NPSTR FAR PASCAL LocalAllocPtr(WORD wFlags, WORD wNbytes)
{
NPSTR npRval;
HLOCAL hRval;

    if(!(hRval = LocalAlloc(wFlags, wNbytes)))
    {
       return(NULL);
    }

    if(!(npRval = (char NEAR *)LocalLock(hRval)))
    {
       LocalFree(hRval);
       return(NULL);
    }

    return(npRval);

}

BOOL FAR PASCAL LocalFreePtr(NPSTR npArg)
{
HLOCAL hRval;

    if(!(hRval = LocalHandle((NPSTR)npArg)))
       return(TRUE);

    LocalUnlock(hRval);

    return((LocalFree(hRval)!=(HANDLE)NULL));
}


LPSTR FAR _cdecl _fstrtrim(LPSTR lpSrc)
{
LPSTR lp1;
WORD wLen;

   if(!(wLen = lstrlen(lpSrc))) return(NULL); /* empty string, null return */

   lp1 = lpSrc + wLen - 1;

   while(lp1>=lpSrc && *lp1<' ')  // should this be " <=' ' " ???
   {
      *lp1-- = 0;
   }

   return(lpSrc);
}


LPSTR FAR _cdecl _fstrichr(LPSTR lpSrc, int c)
{
LPSTR lp1;
register char cc;

   cc = (char)toupper(c);

   if(lpSrc==(LPSTR)NULL)  return(NULL);

   for(lp1=lpSrc; *lp1!=0; lp1++)
   {
      if(cc == (char)toupper(*lp1))
      {
         return(lp1);
      }
   }

   return(NULL);
}




/***************************************************************************/
/*                         DATE AND TIME FUNCTIONS                         */
/***************************************************************************/

static BYTE month_days[13]={0,31,28,31,30,31,30,31,31,30,31,30,31};
static int  total_days[13]={0,31,59,90,120,151,181,212,243,273,304,334,365};
static BYTE leap_days[13] ={0,31,29,31,30, 31, 30, 31, 31, 30, 31, 30, 31};
static int  total_leap[13]={0,31,60,91,121,152,182,213,244,274,305,335,366};
static char month_names[] ="JANFEBMARAPRMAYJUNJULAUGSEPOCTNOVDEC";
static char pNumerals[]   ="0123456789";

static char *pDOW[]={"SUNDAY","MONDAY","TUESDAY","WEDNESDAY","THURSDAY",
                     "FRIDAY","SATURDAY"};

#define FOUR_HUNDRED_YEARS (400L * 365L + 24L * 4L + 1L)
                  /* number of days in 400 years.  (24 * 4 + 1 leap years) */


// LEAP YEAR RULES:  every 4th century, every 4th 'non-century' year.
//                   1900 is NOT leap; 2000 is leap; 1600 is leap


void FAR PASCAL GetSystemDateTime(LPSFTDATE lpDate, LPSFTTIME lpTime)
{
#ifdef WIN32
SYSTEMTIME tm;
   GetLocalTime(&tm);
#endif // WIN32


   if(lpDate)
   {
#ifdef WIN32

      lpDate->year = tm.wYear;
      lpDate->month = tm.wMonth;
      lpDate->day = tm.wDay;

#else  // WIN32
      _asm
      {
         mov ax,0x2a00
         call DOS3Call

         push es
         les bx,lpDate

         mov WORD PTR es:[bx], cx   /* lpDate->year */
         mov BYTE PTR es:[bx+2], dh /* lpDate->month */
         mov BYTE PTR es:[bx+3], dl /* lpDate->day */

         pop es
      }
#endif // WIN32
   }

   if(lpTime)
   {
#ifdef WIN32

      lpDate->hour = tm.wHour;
      lpDate->minute = tm.wMinute;
      lpDate->second = tm.wSecond;
      lpDate->hsecond = tm.wMilliseconds < 995
                      ? (tm.wMilliseconds + 5) / 10
                      : 99;

#else  // WIN32
      _asm
      {
         mov ax, 0x2c00
         call DOS3Call

         push es
         les bx, lpTime

         mov BYTE PTR es:[bx],ch    /* hour */
         mov BYTE PTR es:[bx+1],cl  /* minute */
         mov BYTE PTR es:[bx+2],dh  /* second */
         mov BYTE PTR es:[bx+3],dl  /* hsecond */

         pop es
      }
#endif // WIN32
   }
}


      /* converts 'day of week' (Monday, etc.) to corresponding date */
      /* assumes the 'next' day of week that matches including today */

BOOL FAR PASCAL GetDayOfWeekDate(LPSTR lpDOW, LPSFTDATE lpDate)
{
static struct dosdate_t ddt;
DWORD dwDays;
WORD day;
WORD wLen;


   _dos_getdate(&ddt);
   lpDate->month = ddt.month;
   lpDate->day   = ddt.day;
   lpDate->year  = ddt.year;

   dwDays = days(lpDate);     /* calculate days since 1/1/1900 inclusive */

   wLen = lstrlen(lpDOW);           /* allow truncated abbreviations */

   for(day=0; day<N_DIMENSIONS(pDOW); day++)
   {
      if(_fstrnicmp(pDOW[day], lpDOW, wLen)==0)
         break;
   }

   if(day>=N_DIMENSIONS(pDOW))
   {
      _hmemset((LPSTR)lpDate, 0, sizeof(SFTDATE));

      return(TRUE);  /* bad bad bad! */
   }

   if(day<ddt.dayofweek) day += 7;  /* day is 'earlier' in week than today */
   day -= ddt.dayofweek;           /* how many days to add to current date */

   Date(dwDays + day, lpDate); /* convert 'days' plus offset to DATE */

   return(FALSE);                    /* I am done!  HOORAY! */
}



 /* this procedure requires a CPU with LSB,MSB order on int/long */

int  FAR PASCAL datecmp(LPSFTDATE d1, LPSFTDATE d2)
{
union {
   struct {
     char day, month;
     int year;
     } d;

   DWORD dw;
   } u1, u2;


   u1.d.day   = d1->day;
   u1.d.month = d1->month;
   u1.d.year  = d1->year;

   u2.d.day   = d2->day;
   u2.d.month = d2->month;
   u2.d.year  = d2->year;

   if(u1.dw>u2.dw)       return(1);
   else if(u1.dw==u2.dw) return(0);
   else                  return(-1);

}


 /* this procedure requires a CPU with LSB,MSB order on int/long */

int  FAR PASCAL timecmp(LPSFTTIME t1, LPSFTTIME t2)
{
union {
   struct {
     char second, minute;
     int hour;             /* 'int' keeps the sizes matched */
     } d;

   DWORD dw;
   } u1, u2;


   u1.d.second = t1->second;
   u1.d.minute = t1->minute;
   u1.d.hour   = t1->hour;


   u2.d.second = t2->second;
   u2.d.minute = t2->minute;
   u2.d.hour   = t2->hour;

   if(u1.dw>u2.dw)       return(1);
   else if(u1.dw==u2.dw) return(0);
   else                  return(-1);

}



long FAR PASCAL days(LPSFTDATE d)  /* convert date to # days since 1/1/1900 */
{                                  /* beginning with 1 for 1/1/1900         */
                                   /* because 1/1/1900 was a MONDAY!        */
long l;
int n_years;
long adjustment;
SFTDATE d0;


   if(d==(LPSFTDATE)NULL)
      return(0x80000000L);      /* this indicates an error situation */

   _hmemcpy((char _huge *)&d0, (char _huge *)d, sizeof(SFTDATE));

   adjustment = 0;              /* initially don't adjust for century */

   n_years = d0.year - 1900;    /* # of years since 1900 */

   while(n_years<0)
   {
      n_years += 400;                       /* add 400 years */

      adjustment -= FOUR_HUNDRED_YEARS;    /* number of days in 400 years */
   }

   while(n_years >=400)
   {
      n_years -= 400;

      adjustment += FOUR_HUNDRED_YEARS;
   }

      /*    CALCULATE THE TOTAL NUMBER OF DAYS UP TO 1/1 THIS YEAR    */
      /* terms:  # of days + # of "Feb/29"s - # of non-leap centuries */

   l = 365L * n_years + ((n_years>4)?((n_years - 1) >> 2):0)
       + ((n_years>100)?(1 - (n_years - 1)/ 100):0);


   if(((d0.year % 400)==0 || (d0.year % 100)!=0) && (d0.year % 4)==0)
   {
                           /** LEAP YEAR **/

      l += total_leap[d0.month - 1];    /* month-to-date totals (leap) */
   }
   else
   {
                          /** NORMAL YEAR **/

      l += total_days[d0.month - 1];    /* a slightly faster method! */
   }


   l += d0.day;        /* l is now updated for exact # of days! */

   return(l + adjustment);         /* thdya thdya thdya that's all folks! */
}


void FAR PASCAL Date(long n, LPSFTDATE d)     /* convert 'n' days to a date! */
{
long l;
int count, n_years, j, century;
SFTDATE d0;


   if(!d) return;        /* just in case it's NULL */

   century = 1900;       /* current 'zero' setting 1/1/1900 */

   while(n<=0)
   {
      century -= 400;   /* 400 years */

      n += FOUR_HUNDRED_YEARS;        /* number of days in 400 years */
   }

   while(n>FOUR_HUNDRED_YEARS)
   {
      century += 400;
      n -= FOUR_HUNDRED_YEARS;
   }

   n_years = (int)(n / 365);   /* 'n_years' is an approximated # of years */

                  /* estimate days using 'n_years' */

   l = 365L * n_years + ((n_years>4)?((n_years - 1) >> 2):0)
       + ((n_years>100)?(1 - (n_years - 1)/ 100):0);

   while(l >= n) /* if the approximate result is too large (due to leap years) */
   {
      n_years --;         /* calculate value based on previous year! */

                 /** Speed improvement - 2/18/92 **/

      l = 365L * n_years + ((n_years>4)?((n_years - 1) >> 2):0)
          + ((n_years>100)?(1 - (n_years - 1)/ 100):0);

   }

   d0.year = century + n_years;                 /* calculate year value */


   j = (int)(n - l);       /* day # within current year */

   if(((d0.year % 400)==0 || (d0.year % 100)!=0) && (d0.year % 4)==0)
   {
                           /** LEAP YEAR **/

      for(count=1; j>total_leap[count] && count<N_DIMENSIONS(total_leap);
          count++)  ;  /* find out which month we are *currently* in! */

      j -= total_leap[count - 1];
   }
   else
   {
                          /** NORMAL YEAR **/

      for(count=1; j>total_days[count] && count<N_DIMENSIONS(total_days);
          count++)  ;  /* find out which month we are *currently* in! */

      j -= total_days[count - 1];
   }

   d0.month = (char)count;       /* the current month! */
   d0.day = (char)j;             /* the current day! */

   _hmemcpy((char _huge *)d, (char _huge *)&d0, sizeof(SFTDATE));

}                                /* end of function */





/***************************************************************************/
/*                INTERNATIONAL DATE/TIME/CURRENCY FORMATS                 */
/*                                                                         */
/*  These functions return an 'LPSTR' type which must be GlobalFreePtr'd   */
/*  by the caller to prevent eating up system resources.                   */
/*                                                                         */
/***************************************************************************/

#define TIMEFLAG_VALID    0x8000

#define TIMEFLAG_TYPE     1
#define TIMEFLAG_12       0
#define TIMEFLAG_24       1

#define TIMEFLAG_LEAD0    2


#define DATEFLAG_VALID      0x8000
#define DATEFLAG_ORDER      0x3
#define DATEFLAG_MDY        0
#define DATEFLAG_DMY        1
#define DATEFLAG_YMD        2
#define DATEFLAG_MONTHLEAD0 4
#define DATEFLAG_DAYLEAD0   8
#define DATEFLAG_CENTURY    0x10

static BOOL TimeFlag=0, DateFlag=0;
static char sDate='/', sTime=':', s1159[8]="AM", s2359[8]="PM";

static char pINTL[]       = "intl";
static char pIDATE[]      = "iDate";
static char pSDATE[]      = "sDate";
static char pSSHORTDATE[] = "sShortDate";
static char pSLONGDATE[]  = "sLongDate";  // reserved for later
static char pITIME[]      = "iTime";
static char pITLZERO[]    = "iTLZero";
static char pSTIME[]      = "sTime";
static char pS1159[]      = "s1159";
static char pS2359[]      = "s2359";

static char szDateTimeBuf[64];  // helps minimize stack usage


void FAR PASCAL WinIniChange() // call this when 'WM_WININICHANGE' processed
{
   TimeFlag = 0;  // this initializes the settings above to force the
   DateFlag = 0;  // date/time/etc. conversions to re-evaluate WIN.INI

}


static void FAR PASCAL GetDateProfileInfo(void)
{
   DateFlag = GetProfileInt(pINTL, pIDATE, DATEFLAG_MDY)
              | DATEFLAG_VALID;

   *szDateTimeBuf = sDate;
   szDateTimeBuf[1] = 0;
   GetProfileString(pINTL, pSDATE, szDateTimeBuf, szDateTimeBuf, sizeof(szDateTimeBuf));
   sDate = *szDateTimeBuf;

   GetProfileString(pINTL, pSSHORTDATE, "", szDateTimeBuf, sizeof(szDateTimeBuf));

   if(_fstrstr(szDateTimeBuf,"MM"))   DateFlag |= DATEFLAG_MONTHLEAD0;
   if(_fstrstr(szDateTimeBuf,"dd"))   DateFlag |= DATEFLAG_DAYLEAD0;
   if(_fstrstr(szDateTimeBuf,"yyyy")) DateFlag |= DATEFLAG_CENTURY;
}

static void FAR PASCAL GetTimeProfileInfo(void)
{
   TimeFlag = GetProfileInt(pINTL, pITIME, TIMEFLAG_12)
              | TIMEFLAG_VALID;

   if(GetProfileInt(pINTL, pITLZERO, 0)!=0) TimeFlag |= TIMEFLAG_LEAD0;

   *szDateTimeBuf = sTime;
   szDateTimeBuf[1] = 0;
   GetProfileString(pINTL, pSTIME, szDateTimeBuf, szDateTimeBuf, sizeof(szDateTimeBuf));
   sTime = *szDateTimeBuf;

   if((TimeFlag & TIMEFLAG_TYPE)==TIMEFLAG_12)
   {
      GetProfileString(pINTL, pS1159, s1159, s1159, sizeof(s1159));
      GetProfileString(pINTL, pS2359, s2359, s2359, sizeof(s2359));
   }
}


LPSTR FAR PASCAL DaysStr(long dwDays)
{
SFTDATE d;

   Date(dwDays, &d);
   return(DateStr(&d));
}


LPSTR FAR PASCAL DateStr(LPSFTDATE lpDate)
{
int yy;
LPSTR lp1, lp2;


   if(!(DateFlag & DATEFLAG_VALID))
   {
      GetDateProfileInfo();
   }

   lp1 = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT, 16);

   if(!lp1) return(NULL);  // error - not enough memory, eh?


   lp2 = szDateTimeBuf;

   yy = lpDate->year;


   if((DateFlag & DATEFLAG_ORDER)==DATEFLAG_MDY)
   {
      *(lp2++) = '%';

      if(DateFlag & DATEFLAG_MONTHLEAD0)
      {
         *(lp2++) = '0';
         *(lp2++) = '2';
      }

      *(lp2++) = 'd';
      *(lp2++) = sDate;


      *(lp2++) = '%';

      if(DateFlag & DATEFLAG_DAYLEAD0)
      {
         *(lp2++) = '0';
         *(lp2++) = '2';
      }

      *(lp2++) = 'd';
      *(lp2++) = sDate;


      *(lp2++) = '%';

      if(DateFlag & DATEFLAG_CENTURY)
      {
         *(lp2++) = '0';
         *(lp2++) = '4';
      }
      else
      {
         *(lp2++) = '0';
         *(lp2++) = '2';

         yy %= 100;  // trim off the century, leaving only 0-99
      }

      *(lp2++) = 'd';

      *lp2 = 0;

      wsprintf(lp1, szDateTimeBuf, lpDate->month, lpDate->day, yy);
   }
   else if((DateFlag & DATEFLAG_ORDER)==DATEFLAG_DMY)
   {
      *(lp2++) = '%';

      if(DateFlag & DATEFLAG_DAYLEAD0)
      {
         *(lp2++) = '0';
         *(lp2++) = '2';
      }

      *(lp2++) = 'd';
      *(lp2++) = sDate;


      *(lp2++) = '%';

      if(DateFlag & DATEFLAG_MONTHLEAD0)
      {
         *(lp2++) = '0';
         *(lp2++) = '2';
      }

      *(lp2++) = 'd';
      *(lp2++) = sDate;


      *(lp2++) = '%';

      if(DateFlag & DATEFLAG_CENTURY)
      {
         *(lp2++) = '0';
         *(lp2++) = '4';
      }
      else
      {
         *(lp2++) = '0';
         *(lp2++) = '2';

         yy %= 100;  // trim off the century, leaving only 0-99
      }

      *(lp2++) = 'd';

      *lp2 = 0;

      wsprintf(lp1, szDateTimeBuf, lpDate->day, lpDate->month, yy);
   }
   else if((DateFlag & DATEFLAG_ORDER)==DATEFLAG_YMD)
   {
      *(lp2++) = '%';

      if(DateFlag & DATEFLAG_CENTURY)
      {
         *(lp2++) = '0';
         *(lp2++) = '4';
      }
      else
      {
         *(lp2++) = '0';
         *(lp2++) = '2';

         yy %= 100;  // trim off the century, leaving only 0-99
      }

      *(lp2++) = 'd';
      *(lp2++) = sDate;


      *(lp2++) = '%';

      if(DateFlag & DATEFLAG_MONTHLEAD0)
      {
         *(lp2++) = '0';
         *(lp2++) = '2';
      }

      *(lp2++) = 'd';
      *(lp2++) = sDate;


      *(lp2++) = '%';

      if(DateFlag & DATEFLAG_DAYLEAD0)
      {
         *(lp2++) = '0';
         *(lp2++) = '2';
      }

      *(lp2++) = 'd';


      *lp2 = 0;

      wsprintf(lp1, szDateTimeBuf, yy, lpDate->month, lpDate->day);
   }
   else
   {
      GlobalFreePtr(lp1);
      return(NULL);
   }

   return(lp1);
}


LPSTR FAR PASCAL TimeStr(LPSFTTIME lpTime)
{
LPSTR lp1, lp2;
int hour;
BOOL ampm;



   if(!(TimeFlag & TIMEFLAG_VALID))
   {
      GetTimeProfileInfo();
   }

   lp1 = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT, 16);
   if(!lp1) return(NULL);  // error - out of memory

   lp2 = szDateTimeBuf;

   if((TimeFlag & TIMEFLAG_TYPE)==TIMEFLAG_12)
   {
      ampm = lpTime->hour>=12 && lpTime->hour!=24;

      hour = lpTime->hour % 12;
      if(!hour) hour+=12;  // noon == 12:00 PM; midnight 12:00 AM

      *(lp2++) = '%';

      if(TimeFlag & TIMEFLAG_LEAD0)
      {
         *(lp2++) = '0';
         *(lp2++) = '2';
      }

      *(lp2++) = 'd';
      *(lp2++) = sTime;

      *(lp2++) = '%';

      if(TimeFlag & TIMEFLAG_LEAD0)
      {
         *(lp2++) = '0';
         *(lp2++) = '2';
      }

      *(lp2++) = 'd';
      *(lp2++) = sTime;

      *(lp2++) = '%';

      if(TimeFlag & TIMEFLAG_LEAD0)
      {
         *(lp2++) = '0';
         *(lp2++) = '2';
      }

      *(lp2++) = 'd';

      lstrcpy(lp2, ampm?(LPSTR)s2359:(LPSTR)s1159);

   }
   else if((TimeFlag & TIMEFLAG_TYPE)==TIMEFLAG_24)
   {
      hour = lpTime->hour;

      *(lp2++) = '%';

      if(TimeFlag & TIMEFLAG_LEAD0)
      {
         *(lp2++) = '0';
         *(lp2++) = '2';
      }

      *(lp2++) = 'd';
      *(lp2++) = sTime;

      *(lp2++) = '%';

      if(TimeFlag & TIMEFLAG_LEAD0)
      {
         *(lp2++) = '0';
         *(lp2++) = '2';
      }

      *(lp2++) = 'd';
      *(lp2++) = sTime;

      *(lp2++) = '%';

      if(TimeFlag & TIMEFLAG_LEAD0)
      {
         *(lp2++) = '0';
         *(lp2++) = '2';
      }

      *(lp2++) = 'd';

      *lp2 = 0;
   }
   else
   {
      GlobalFreePtr(lp1);
      return(NULL);
   }

   wsprintf(lp1, szDateTimeBuf, hour, lpTime->minute, lpTime->second);

   return(lp1);
}



/***************************************************************************/
/*         CONVERSION FROM INT'L FORMAT DATE/TIME STRING TO STRUCT         */
/***************************************************************************/

static int NEAR PASCAL AddCentury(int iYear, int iThisYear)
{
int i1 = iThisYear % 100;
int i2 = iYear % 100;


   if((iYear / 100) != 0) return(iYear);  // already has a century

   if(i1 >= 50)  // between 1950 and 1999, let's say
   {
      // in this case, if I enter a value >= (i1 - 50),
      // it's my century; otherwise, add 100 to the year for next century

      if(i2 >= (i1 - 50))  // same century
      {
         return(i2 - i1 + iThisYear);
      }
      else
      {
         return(i2 - i1 + iThisYear + 100);
      }
   }
   else
   {
      // in this case, if I enter a value <= (i1 + 50), it's
      // the same century; else, subtract 100 from the year for last
      // century.

      if(i2 <= (i1 + 50))
      {
         return(i2 - i1 + iThisYear);
      }
      else
      {
         return(i2 - i1 + iThisYear - 100);
      }
   }

}

BOOL FAR PASCAL atodate(LPSTR string, LPSFTDATE d)
{
/*   this function will parse dates in the following formats:

     MM/DD/YY                      12/03/89     1/4/90
     MM/DD/YYYY                    12/03/1989   1/4/2002
     MM/DD                         12/03        1/4  (assumes current year)
     MMDDYY                        120389       10490 (no lead 0 needed)
     MMDDYYYY                      12031989     1042002
     MDD (implied leading zero)    104
     MM-DD-YY                      12-03-89     1-4-90
     MM-DD-YYYY                    12-03-1989   1-4-1990
     DD-Mmm-YY                     03-Dec-89    4-Jan-90   (not case sensitive)
     DD-Mmm-YYYY                   03-Dec-1989  4-Jan-2000

     invalid date returns 0/0/0 (except 99/99/99 which returns 99/99/9999)
*/
static struct dosdate_t d2;
static char parm1[16], parm2[16], parm3[16];
LPSTR c1, c2;
BOOL rval = FALSE;
unsigned long l;
SFTDATE d0;

//#define ATODATE_GET_YEAR(X)
//         if(strspn((X),pNumerals)<strlen(X)) rval=TRUE; 
//         else { if(strlen(X)<=3) d0.year = atoi(X) + (d2.year/100)*100; 
//                else             d0.year = atoi(X); }

#define ATODATE_GET_YEAR(X) \
         if(strspn((X),pNumerals)<strlen(X)) rval=TRUE; \
         else { if(strlen(X)<=3) d0.year = AddCentury(atoi(X), d2.year); \
                else             d0.year = atoi(X); }



   _dos_getdate(&d2);                               /* get current DOS date */

   if(!(DateFlag & DATEFLAG_VALID))
   {
      GetDateProfileInfo();
   }

   c1 = c2 = _fstrchr(string, sDate);
   if(!c1)
   {
      c1 = c2 = _fstrchr(string, '/');                  /* look for slashes! */
      if(c1 == (LPSTR)NULL)
         c1 = c2 = _fstrchr(string, '-');               /* look for dashes! */
   }


                      /**    NO SLASHES OR DASHES!    **/
                      /** Assume one of the following **/
                      /**   (dependent on WIN.INI)    **/
                      /**                             **/
                      /**   MMDDYY, MMDDYYYY, MMDD    **/
                      /**   DDMMYY, DDMMYYYY, DDMM    **/
                      /**   YYMMDD, YYYYMMDD, MMDD    **/

   if(!c1)                           /* if no '-' either, then all numbers! */
   {
      _fstrncpy(parm1,string,sizeof(parm1));

      l = atol(parm1);                                /* convert to a long! */

      // assume YMD format if 8-digit value, no '/' or '-'
      // and year forms an invalid month or day

//      if((l >= 13000000 && (DateFlag & DATEFLAG_ORDER)==DATEFLAG_MDY) ||
//         (l >= 10000000 && (l % 1000000) > 12 &&
//          (DateFlag & DATEFLAG_ORDER)==DATEFLAG_DMY))
      if(l >= 13000000)
      {
         d0.year  = (int)(l / 10000L); // get the year (left 4 digits)

         l %= 10000L;

         d0.day   = (char)(l % 100);        // obtain day (right 2 digits)

         d0.month = (char)(l / 100);        // obtain month (left 2 digits)
      }
      else
      {
         if(l<10000)
         {
                         /* no year - add current year! */

            if((DateFlag & DATEFLAG_ORDER)==DATEFLAG_YMD)
            {
               l += 10000 * d2.year;
            }
            else
            {
               l = l * 10000 + d2.year;
            }
         }
         else if(l<1000000L)
         {
                      /* no century - add current century */

            if((DateFlag & DATEFLAG_ORDER)==DATEFLAG_YMD)
            {
               l = (l % 10000) + 10000 * AddCentury(l / 10000, d2.year);
//               l += 1000000L * (d2.year / 100);
            }
            else
            {
               l = (l / 100) * 10000L + AddCentury(l % 100, d2.year);
//               l = (l / 100) * 10000L + (l % 100) + (d2.year / 100) * 100L;
            }
         }


         if((DateFlag & DATEFLAG_ORDER)==DATEFLAG_YMD)
         {
            d0.year  = (int)(l / 10000L); // get the year (left 4 digits)

            l %= 10000L;

            d0.day   = (char)(l % 100);        // obtain day (right 2 digits)

            d0.month = (char)(l / 100);        // obtain month (left 2 digits)
         }
         else if((DateFlag & DATEFLAG_ORDER)==DATEFLAG_MDY)
         {
            d0.year = (int)(l % 10000); // place year (4 digits) into structure

            l /= 10000L;                // divide l by 10000 to simplify next

            d0.day   = (char)(l % 100);        // obtain day (right 2 digits)

            d0.month = (char)(l / 100);        // obtain month (left 2 digits)
         }
         else if((DateFlag & DATEFLAG_ORDER)==DATEFLAG_DMY)
         {
            d0.year = (int)(l % 10000); // place year (4 digits) into structure

            l /= 10000L;                // divide l by 10000 to simplify next

            d0.month = (char)(l % 100);        // obtain month (right 2 digits)

            d0.day   = (char)(l / 100);        // obtain day (left 2 digits)
         }
      }
   }

                       /**  SLASHES or DASHES FOUND!  **/
                       /** USE INTERNATIONAL SETTINGS **/
   else
   {

      _fstrncpy((LPSTR)parm1, string, (WORD)((long)c1-(long)string));
                                                               /* copy month */
      parm1[((long)c1 - (long)string)] = 0;              /* terminate string */

      c2 = _fstrchr(c1+1, sDate);
      if(!c2)
      {
         c2 = _fstrchr(c1+1, '/');                /* look for another slash! */
         if(c2 == (LPSTR)NULL)
            c2 = _fstrchr(c1+1, '-');              /* look for another dash! */
      }

      if(c2 == (LPSTR)NULL)         /* if still not found, use current year! */
         lstrcpy(parm2, c1+1);         /* copy remainder of string into 'day' */

      else
      {
         _fstrncpy((LPSTR)parm2, c1+1, (WORD)((long)c2 - (long)c1) - 1);
         parm2[((long)c2 - (long)c1) - 1] = 0;

         lstrcpy(parm3, c2+1);            /* copy remaining into year buffer */
      }

      _fstrupr(parm1);
      _fstrupr(parm2);                       /* ensure it's all upper case! */
      _fstrupr(parm3);


               /*               SPECIAL CASE!                */
               /*   DD-Mmm-YY or DD-Mmm-YYYY or DD-Mmm or    */
               /*      YY-Mmm-DD or YYYY-Mmm-DD Format!      */
               /*     (check this before checking type)      */

      if(lstrlen(parm2)==3 && (c1=_fstrstr(month_names, parm2)))
      {
         d0.month = (char)(1 + ((long)c1 - (long)((LPSTR)month_names))/3);
                                                              /* get month # */

             /* month name in middle!  Now, which is the year?? */

         if(!c2)     // only 2 parameters were specified
         {
            d0.day = (char)(atoi(parm1));
            d0.year = d2.year;
         }
         else if((DateFlag & DATEFLAG_ORDER)==DATEFLAG_YMD)
         {
            d0.day = (char)(atoi(parm3));
            ATODATE_GET_YEAR(parm1);
         }
         else
         {
            d0.day = (char)(atoi(parm1));
            ATODATE_GET_YEAR(parm3);
         }


      }
               /*               SPECIAL CASE!                */
               /* Mmm-DD-YY or Mmm-DD-YYYY or Mmm-DD Format! */
               /*     (check this before checking type)      */

      else if(lstrlen(parm1)==3 && (c1=_fstrstr(month_names, parm1)))
      {
         d0.month = (char)(1 + ((long)c1 - (long)((LPSTR)month_names))/3);
                                                              /* get month # */

         d0.day = (char)(atoi(parm2));

         if(!c2)  // there is no year parameter...
         {
            d0.year = d2.year;
         }
         else
         {
            ATODATE_GET_YEAR(parm3);
         }
      }

            /** ALL DIGIT ENTRIES!  Now, check to see which order... **/
      else
      {
         if(strspn(parm1,pNumerals)<strlen(parm1) || /* non-numeral digits? */
            strspn(parm2,pNumerals)<strlen(parm2) ||
            (c2 && strspn(parm3,pNumerals)<strlen(parm3)))
         {
            rval = TRUE;
         }


         if(!c2)           // there is no year on it
         {
            if((DateFlag & DATEFLAG_ORDER)==DATEFLAG_DMY)
            {
               d0.day   = (char)atoi(parm1);
               d0.month = (char)atoi(parm2);
            }
            else           // it's either MM/DD/YY or YY/MM/DD
            {
               d0.month = (char)atoi(parm1);
               d0.day   = (char)atoi(parm2);
            }

            d0.year  = d2.year;             // current year
         }
         else if(atoi(parm1)>=100)          // assume YMD format
         {
            ATODATE_GET_YEAR(parm1);

            d0.month = (char)atoi(parm2);
            d0.day   = (char)atoi(parm3);
         }
         else
         {
            if((DateFlag & DATEFLAG_ORDER)==DATEFLAG_MDY)
            {
               d0.month = (char)atoi(parm1);
               d0.day   = (char)atoi(parm2);

               ATODATE_GET_YEAR(parm3);
            }
            else if((DateFlag & DATEFLAG_ORDER)==DATEFLAG_DMY)
            {
               d0.day   = (char)atoi(parm1);
               d0.month = (char)atoi(parm2);

               ATODATE_GET_YEAR(parm3);
            }
            else if((DateFlag & DATEFLAG_ORDER)==DATEFLAG_YMD)
            {
               ATODATE_GET_YEAR(parm1);

               d0.month = (char)atoi(parm2);
               d0.day   = (char)atoi(parm3);
            }
         }
      }


//      if(!c2)
//      {
//         d0.year = d2.year;            /* use current year if none present */
//      }
//      else
//      {
//         if(strspn(parm3,pNumerals)<strlen(parm3))    /* non-numeral digits? */
//         {
//            rval = TRUE;
//         }
//
//         if(lstrlen(c2)<=3)                        /* use current century */
//         {
//            d0.year = atoi(parm3) + (d2.year/100)*100;
//         }
//         else
//         {
//            d0.year = atoi(parm3);
//         }
//      }
   }

                    /* next, perform a sanity check */

   if(d0.month==99 && d0.day==99)    /* "99/99" or "99/99/99" */
   {
      d0.month = 99;
      d0.day = 99;
      d0.year = 9999;
   }
   else if(d0.month==0 && d0.day==0)   /* "0/0" or "0/0/0" */
   {
      d0.month = 0;
      d0.day = 0;
      d0.year = 0;
   }
   else if(((d0.year % 400)==0 || (d0.year % 100)!=0)
           && (d0.year % 4)==0)         /* it's a leap year!! */
   {
      if((d0.month<1 || d0.month>12 || d0.day<1
          || d0.day>leap_days[d0.month] || d0.year<0))
      {

         d0.month = 0;
         d0.day = 0;
         d0.year = 0;
         rval = TRUE;
      }
   }
   else
   {
      if((d0.month<1 || d0.month>12 || d0.day<1
          || d0.day>month_days[d0.month] || d0.year<0))
      {

         d0.month = 0;
         d0.day = 0;
         d0.year = 0;
         rval = TRUE;
      }
   }


   _hmemcpy((char _huge *)d, (char _huge *)&d0, sizeof(SFTDATE));

   return(rval);

#undef ATODATE_GET_YEAR
}


BOOL FAR PASCAL atotime(LPSTR string, LPSFTTIME t)
{
LPSTR lp1;
int hour, minute, second;
static const char pAM[]="AM", pPM[]="PM";


   if(!(TimeFlag & TIMEFLAG_VALID))
   {
      GetTimeProfileInfo();
   }

   _hmemset((LPSTR)t, 0, sizeof(SFTTIME));   /* initialize structure */

   for(lp1=string; *lp1 && *lp1<=' '; lp1++)
     ;                                    /* find first non-space */


   for(hour=0; *lp1>' ' && *lp1!=sTime; lp1++)
   {
      if(*lp1>='0' && *lp1<='9')
      {
         hour *=10;
         hour += (int)*lp1 - (int)'0';
      }
      else
      {
         return(TRUE);
      }
   }

   if(hour>24) return(TRUE);  /* error!! */

   t->hour = (char) hour;

   if(*lp1>' ') lp1++;
   else         return(FALSE);   /* done (end of string) */

   for(minute=0; *lp1>' ' && *lp1!=sTime; lp1++)
   {
      if(*lp1>='0' && *lp1<='9')
      {
         minute *=10;
         minute += (int)*lp1 - (int)'0';
      }
      else if(!_fstrnicmp(lp1, s1159, strlen(s1159)) ||
              !_fstrnicmp(lp1, pAM, sizeof(pAM)))
      {
         t->minute = (char) minute;         /*   i.e.   ##:##AM */

         return(FALSE);
      }
      else if(!_fstrnicmp(lp1, s2359, strlen(s2359)) ||
              !_fstrnicmp(lp1, pPM, sizeof(pPM)))
      {
         t->hour += (char) 12;              /*   i.e.   ##:##PM */
         t->minute = (char) minute;

         return(hour>24 || minute>59);  /* an error flag */
      }
      else
      {
         return(TRUE);
      }
   }

   if(minute>59) return(TRUE);  /* error!! */

   t->minute = (char) minute;

   if(*lp1>' ') lp1++;
   else         return(FALSE);   /* done (end of string) */

   for(second=0; *lp1>' '; lp1++)
   {
      if(*lp1>='0' && *lp1<='9')
      {
         second *=10;
         second += (int)*lp1 - (int)'0';
      }
      else if(!_fstrnicmp(lp1, s1159, strlen(s1159)) ||
              !_fstrnicmp(lp1, pAM, sizeof(pAM)))
      {
         t->second = (char) second;         /*   i.e.   ##:##:##AM */

         return(FALSE);
      }
      else if(!_fstrnicmp(lp1, s2359, strlen(s2359)) ||
              !_fstrnicmp(lp1, pPM, sizeof(pPM)))
      {
         t->hour += (char) 12;              /*   i.e.   ##:##:##PM */
         t->second = (char) second;

         return(hour>24 || second>59);  /* an error flag */
      }
      else
      {
         return(TRUE);
      }
   }

   if(second>59) return(TRUE);  /* error!! */

   t->second = (char) second;

   if(*lp1<=' ') return(FALSE);
   else          return(TRUE);   /* uh, oh! */


}



/***************************************************************************/
/*   _lqsort() - function to do a 'qsort' on far/huge data buffer          */
/*                                                                         */
/***************************************************************************/


//#pragma alloc_text (LQSORT_TEXT, _lqsort, _lqsort_, _lqsort2_,
//                    _lqsort_middle_, _lqsort_defcmp)


#pragma code_seg("LQSORT_TEXT","CODE")

void FAR _cdecl _lqsort(char _huge *base, DWORD num, DWORD width,
                        LPCOMPAREPROC compare)
//                       int (FAR _cdecl *compare)(LPSTR p1, LPSTR p2))
{
QSORTVARS qsv;


      /* step 1:  get memory for temporary buffer */

   if(num<=1L)  return;     /* if 1 or 0 elements, just return - no sort! */


   qsv.hp = (char _huge *)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                         width);

   if(!qsv.hp)
   {
      return;
   }

   if((LPVOID)compare != NULL)
   {
//      (int (FAR _cdecl *&)())qsv.cmp = (int (FAR _cdecl *)())compare;
      (LPCOMPAREPROC &)qsv.cmp = compare;
   }
   else
   {
      qsv.cmp = _lqsort_defcmp;/* if 'NULL' specified for compare function */
   }

   qsv._num = num;
   qsv._width = width;
   qsv._base = base;

   _lqsort_(0,(num - 1),(SBPQSORTVARS)LOWORD((LPSTR)&qsv));/* perform sort */

   GlobalFreePtr((LPSTR)(qsv.hp));

}



static int FAR PASCAL _lqsort_(LONG left, LONG right, SBPQSORTVARS bpqsv)
{
LONG i, j;

      /***************************************************************/
      /*** Initially do 'middle of 3' optimization for pivot point ***/
      /***************************************************************/

   _lqsort_middle_(left, right, bpqsv);  /* return in 'bpqsv' */

   i = left;
   j = right;

   _lqsort2_(left, right, (LONG FAR *)&i, (LONG FAR *)&j, bpqsv);

             /** The next section will perform recursion **/

   if(left<j)  _lqsort_(left, j, bpqsv);
   if(i<right) _lqsort_(i, right, bpqsv);

   return(FALSE);

}



           /*******************************************************/
           /* this next section has its own automatic variables   */
           /* in order to save on stack space prior to recursion. */
           /*******************************************************/

void NEAR _fastcall _lqsort2_(LONG left, LONG right, LONG FAR *lpI,
                              LONG FAR *lpJ, SBPQSORTVARS bpqsv)
{

int (FAR _cdecl *_cmp)(LPSTR p1, LPSTR p2,
                      struct tagQSORTVARS STACK_BASED *bpqsv);
char _huge *__base;
DWORD __width;
char _huge *_hp;
LONG i, j, i2, j2;


   i = *lpI;
   j = *lpJ;

   _cmp    = bpqsv->cmp;        /* local equivalents - faster! */
   __base  = bpqsv->_base;
   __width = bpqsv->_width;
   _hp     = bpqsv->hp;
   i2      = i * __width;
   j2      = j * __width;

   do
   {

      while(_cmp((__base + i2), _hp, bpqsv)<0 && i<right)
      {
         i ++;
         i2 += __width;
      }
      while(_cmp((__base + j2), _hp, bpqsv)>0 && j>left)
      {
         j --;
         j2 -= __width;
      }

      if(i<=j)
      {
         _hmemswap(__base + i2, __base + j2, __width);

         i ++;
         i2 += __width;

         j --;
         j2 -= __width;
      }

   } while(i<=j);


   *lpI = i;
   *lpJ = j;

}

                  /****************************************/
                  /** 'Middle of 3' Optimization routine **/
                  /****************************************/

void NEAR _fastcall _lqsort_middle_(LONG left, LONG right,SBPQSORTVARS bpqsv)
{
int (FAR _cdecl *_cmp)(LPSTR p1, LPSTR p2,
                      struct tagQSORTVARS STACK_BASED *bpqsv);
char _huge *__base;
DWORD __width;
LONG i, j, k;


   if((right - left)<8) /* with 7 or less shouldn't really do 'middle of 3' */
   {
      _hmemcpy(bpqsv->hp,
               bpqsv->_base + (((left + right) / 2) * bpqsv->_width),
               bpqsv->_width);
   }
   else                   /* 'middle of 3' optimization!!! */
   {

      _cmp    = bpqsv->cmp;        /* local equivalents - faster! */
      __base  = bpqsv->_base;
      __width = bpqsv->_width;

      if(_cmp(__base + (i=((((left + right) / 2) + left) / 2) * __width),
                    __base + (j=((left + right) / 2) * __width), bpqsv) < 0)
      {
         if(_cmp(__base + (k=((((left + right) / 2) + right) / 2) * __width),
                       __base + i, bpqsv) > 0)
         {
            if(_cmp(__base + k, __base + j, bpqsv) < 0)
               _hmemcpy(bpqsv->hp, __base + k, __width);
            else
               _hmemcpy(bpqsv->hp, __base + j, __width);
         }
         else
            _hmemcpy(bpqsv->hp, __base + i, __width);

      }
      else
      {
         if(_cmp(__base + (k=((((left + right) / 2) + right) / 2) * __width),
                       __base + j, bpqsv) > 0)
         {
            if(_cmp(__base + k, __base + i, bpqsv) < 0)
               _hmemcpy(bpqsv->hp, __base + k, __width);
            else
               _hmemcpy(bpqsv->hp, __base + i, __width);
         }
         else
            _hmemcpy(bpqsv->hp, __base + j, __width);

      }
   }

}



/***************************************************************************/
/* _lqsort_defcmp() - default compare function if NULL passed to '_lqsort' */
/***************************************************************************/

static int FAR _cdecl _lqsort_defcmp(LPSTR p1, LPSTR p2, SBPQSORTVARS bpqsv)
{
   return(_hmemcmp(p1, p2, bpqsv->_width));
}



#pragma code_seg()




/***************************************************************************/
/*                                                                         */
/*   Programs which include in-line ASSEMBLY code (and must have certain   */
/*   optimizations disabled to prevent compiler warnings).                 */
/*                                                                         */
/***************************************************************************/


#pragma optimize ("gle",off)

            /*** GLOBAL OPTIMIZATIONS HAVE BEEN DISABLED ***/


WORD FAR PASCAL ExecuteBatchFile(LPSTR lpProgName, LPDOSRUNPARMBLOCK lpParmBlock)
{
//register WORD rval;
LPSTR lpCmdLine, lp1, lp2;
LPBATCH_INFO lpNewBatch;
BOOL last_was_space=FALSE, OldBatchMode;



   // load and execute a BATCH file


   if(lpProgName)             // normal: a program file is being executed
   {
      lpCmdLine = lpParmBlock->lpCmdLine;

      lpNewBatch = (LPBATCH_INFO)GlobalAllocPtr(GMEM_MOVEABLE|GMEM_ZEROINIT,
                                       sizeof(BATCH_INFO) + *lpCmdLine + 1);
      if(!lpNewBatch)
      {
         MessageBox(hMainWnd, "?Not enough memory to open batch file!",
                    "** BATCH FILE ERROR **",
                    MB_OK | MB_ICONHAND | MB_SYSTEMMODAL);
         return(0);
      }

      lp2 = lpNewBatch->lpArgList = (LPSTR)(lpNewBatch + 1);
                                        /* just after structure is arg list */

                /*** PARSE THE COMMAND LINE FOR PARAMETERS ***/

      last_was_space = TRUE;                  /* assume a space (initially) */
      lpNewBatch->nArgs = 0;                    /* initial count is zero!!! */

      for(*lp2=0, lp1=lpCmdLine + 1; lp1<=(lpCmdLine + *lpCmdLine); lp1++)
      {
         if(*lp1>' ')                                  /* not white space */
         {
            if(last_was_space)
            {
               lpNewBatch->nArgs ++;
            }

            *lp2++ = *lp1;                /* copy char over to new buffer */
            *lp2 = 0;               /* ensure next char is a null for now */
            last_was_space = FALSE;
         }
         else if(!last_was_space)
         {
            *(++lp2) = 0;           /* end of word - puts a NULL between! */
            last_was_space = TRUE;
         }
      }


      lpNewBatch->hFile = MyOpenFile(lpProgName, &(lpNewBatch->ofstr),
                                    OF_READ | OF_SHARE_DENY_WRITE);

      if(lpNewBatch->hFile == HFILE_ERROR ||
         (lpNewBatch->buf_len = MyRead(lpNewBatch->hFile, lpNewBatch->buf,
                                       BATCH_BUFSIZE))==-1)
      {
         if(lpNewBatch->hFile!=HFILE_ERROR)
            _lclose(lpNewBatch->hFile);

         GlobalFreePtr((LPSTR)lpNewBatch);  /* gonzo! */

         CMDErrorCheck(TRUE); /* print an appropriate error message */
         return(2);               /* essentially 'file not found' */
      }

   }
   else      // MEMORY BATCH FILE:  'lpParmBlock' points to the actual code
   {
    LPSTR lpCode = (LPSTR)lpParmBlock;

      lpNewBatch = (LPBATCH_INFO)
                   GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                  sizeof(BATCH_INFO) + lstrlen(lpCode)
                                  - BATCH_BUFSIZE + 4);

      if(!lpNewBatch)
      {
         MessageBox(hMainWnd,
                    "?Not enough memory to create memory batch file!",
                    "** BATCH FILE ERROR **",
                    MB_OK | MB_ICONHAND | MB_SYSTEMMODAL);
         return(0);
      }

      lpNewBatch->lpArgList = (LPSTR)lpNewBatch->buf;
      lpNewBatch->nArgs = 0;        // zero arguments

      lstrcpy((LPSTR)lpNewBatch->buf, lpCode);
                                   // this stores the actual program text

      lpNewBatch->hFile = HFILE_ERROR; // this says 'I am a memory batch file'

      // assign various buffer pointers

      lpNewBatch->buf_len = lstrlen(lpCode);
   }


   if(lpNewBatch->hFile != HFILE_ERROR &&    // don't do this for mem batch
      lpNewBatch->buf_len > 5)
   {
      if(*(lpNewBatch->buf)=='/' && (lpNewBatch->buf)[1]=='*')
      {
       LPSTR lpRxCmdLine;

     /*****************************************************************/
     /* 1st 2 characters are '/' and '*' - try to run as REXX program */
     /*****************************************************************/

         lpRxCmdLine = (LPSTR)lpNewBatch->buf;
         _hmemset(lpRxCmdLine, 0, BATCH_BUFSIZE);

              /* build the 'REXX' program command line */

         if(lpNewBatch->hFile != HFILE_ERROR)
            _lclose(lpNewBatch->hFile); /* file must be closed!! */

         lpNewBatch->hFile = HFILE_ERROR; // CLOSED!

         lstrcpy(lpRxCmdLine, lpNewBatch->ofstr.szPathName);
         lstrcat(lpRxCmdLine, " ");
         _hmemcpy(lpRxCmdLine + lstrlen(lpRxCmdLine), lpCmdLine + 1,
                  (DWORD)(*lpCmdLine));

         if(!RexxRun(lpRxCmdLine, FALSE))  /* it worked, it worked! */
         {
            GlobalFreePtr(lpNewBatch);
            return(0xffff);
         }
         else if(hRexxAPI)  /* the API handle - it was loaded? */
         {
            GlobalFreePtr(lpNewBatch);
            return(2);      /* error running program... */
         }
         else                /* REXX not present - run as before! */
         {                      /* (duplicate code from above) */

            lpNewBatch->hFile = MyOpenFile(lpProgName, &(lpNewBatch->ofstr),
                                          OF_READ | OF_SHARE_DENY_WRITE);

            if(lpNewBatch->hFile==OF_ERROR ||
               (lpNewBatch->buf_len = MyRead(lpNewBatch->hFile, lpNewBatch->buf,
                                             BATCH_BUFSIZE))==-1)
            {
               if(lpNewBatch->hFile!=OF_ERROR)
                  _lclose(lpNewBatch->hFile);

               GlobalFreePtr((LPSTR)lpNewBatch);  /* gonzo! */

               CMDErrorCheck(TRUE); /* print an appropriate error message */
               return(2);               /* essentially 'file not found' */
            }
         }

      }
   }

   lpNewBatch->buf_start = 0L;
   lpNewBatch->buf_pos   = 0;

   OldBatchMode = BatchMode;       /* save for later, in case we must */
                                   /* wait for a batch file to end first */
   BatchMode = TRUE;               /* assigns batch mode!! */

   if(IsFor || IsRexx)    /* special handling for 'FOR' command & 'REXX' */
   {
    LPBATCH_INFO lpOldBatchInfo;

      IsRexx = FALSE;/* prevents 'recursive' processing when not wanted */
      IsFor = FALSE; /* prevents 'recursive' processing when not wanted */
      IsCall = FALSE;/* prevents 'recursive' processing when not wanted */


      lpOldBatchInfo = lpBatchInfo;  /* store the previous 'batch info' */
                                   /* pointer - I will restore it later */

      lpNewBatch->lpPrev = (LPBATCH_INFO)MAKELP(0,1);
                     /* indicate 'for' command in progress, which tells */
                  /* the BATCH handler not to reset 'BatchMode' on exit */

      lpBatchInfo = lpNewBatch;

      do
      {
         if(LoopDispatch()) return(0);    /* allows thread to terminate */


         if(MthreadGetCurrentThread()==hBatchThread)
         {
            BatchIteration();  /* the only way I can execute batch file! */
         }
      }
      while(lpBatchInfo);  /* wait for it to finish! */

      lpBatchInfo = lpOldBatchInfo;  /* restore 'BatchMode' */
      BatchMode = OldBatchMode;       /* and 'lpBatchInfo' */

      return(0xffff);  /* this says 'it worked ok' to 'CMDRunProgram()' */
   }
   else if(IsCall || !lpBatchInfo)
   {
      lpNewBatch->lpPrev = lpBatchInfo;
      lpBatchInfo = lpNewBatch;       /* it's now official!! */

      IsCall = FALSE;/* prevents 'recursive' processing when not wanted */
   }
   else                                 /* normal batch file 'chaining' */
   {
      if(lpNewBatch->hFile != HFILE_ERROR)
         _lclose(lpBatchInfo->hFile);

      GlobalFreePtr(lpBatchInfo);

      lpNewBatch->lpPrev = NULL;
      lpBatchInfo = lpNewBatch;

   }

   return(0xffff);                 /* signals that result was good */

}



static int FAR __cdecl EnvPointerCompare(char __huge *p1, char __huge *p2)
{
   p1 = *((char __huge * __huge *)p1);
   p2 = *((char __huge * __huge *)p2);

   while(*p1 && *p2 && *p1 != '=' && *p2 != '=' && *p1 == *p2)
   {
      p1++;
      p2++;
   }

   if(*p2 != '=' && (*p1 == '=' || *p1 < *p2))
   {
      return(-1);  // p1 is less than p2
   }
   else if(*p1 != '=' && (*p2 == '=' || *p1 > *p2))
   {
      return(1);   // p1 is greater than p2
   }
   else
   {
      return(0);   // p1 is equal to p2 (shouldn't happen in this case)
   }

}

// this next version invokes 'CreateProcess' from Win32 API

WORD FAR PASCAL RunProgramUsingCreateProcess(LPSTR lpProgName,
                                             LPDOSRUNPARMBLOCK lpParmBlock,
                                             LPSTR lpStdIn, LPSTR lpStdOut)
{
DWORD dw1, dw2, dw3;
WORD wRval;
LPSTR lpCmdLine, lpEnv0, lp1, lp2, lpNewEnv = NULL, lpBuf = NULL;
static const char szHelperProg[]="SFTSH32X.EXE";
char tbuf[64];




   // 1st step:  create SORTED environment block by enumerating the
   // keys, sorting them, and re-adding them to a NEW environment block.


   if(lpParmBlock->wEnvSeg)
   {
      lpNewEnv = (LPSTR)GlobalAllocPtr(GPTR, GlobalSize((HGLOBAL)GlobalHandle(lpParmBlock->wEnvSeg)) + 4);
      if(lpNewEnv)
      {
       LPSTR FAR *lplpEnv;

         // first, count the strings

         for(dw2=0, lp1 = (LPSTR)MAKELP(lpParmBlock->wEnvSeg, 0); *lp1;
             dw2++, lp1 += lstrlen(lp1) + 1)
            ;  // find out just how many strings there are

         // next, get a set of pointers to these strings, and place them
         // at the END of the new environment block

         lplpEnv = (LPSTR FAR *)(lpNewEnv + GlobalSizePtr(lpNewEnv) -
                                 (dw2 + 1) * sizeof(*lplpEnv));

         for(dw1=0, lp1 = (LPSTR)MAKELP(lpParmBlock->wEnvSeg, 0); *lp1;
             dw1++, lp1 += lstrlen(lp1) + 1)
         {
            lplpEnv[dw1] = lp1;
         }

         lp1++;  // point 'lp1' to the command line/program name

         // NOW, sort the strings

         _lqsort((char HUGE *)lplpEnv, dw2, sizeof(*lplpEnv),
                 (LPCOMPAREPROC)EnvPointerCompare);

         // last but not least, add strings to new env block

         for(dw1=0, lp2 = lpNewEnv; dw1 < dw2; dw1++)
         {
            lstrcpy(lp2, lplpEnv[dw1]);
            lp2 += lstrlen(lp2) + 1;
            *lp2 = 0;
         }
         // add one more null byte at the end

         *(lp2++) = 0;

         // NOW, copy the command line and program name info

         if(*lp1)
         {
            lstrcpy(lp2, lp1);

            lp1 += lstrlen(lp1) + 1;
            lp2 += lstrlen(lp2) + 1;
         }

         if(*lp1)
         {
            lstrcpy(lp2, lp1);

            lp1 += lstrlen(lp1) + 1;
            lp2 += lstrlen(lp2) + 1;
         }

         *(lp1++) = 0;  // 4 extra NULL bytes
         *(lp1++) = 0;
         *(lp1++) = 0;
         *lp1     = 0;
      }
   }


   // Use 'CreateProcess' to run this program - this allows me
   // to correctly specify the environment, STDIN, and STDOUT
   // If 'lpStdIn' is not NULL, it will be opened for input for STDIN
   // If 'lpStdOut' is not NULL, it will be opened for output for STDERR
   // (if the 1st character of the name is '>', it will open in append mode)



   dw1 = lpGetProcAddress32W(hKernel32, "CreateProcessA");

   if(dw1)
   {
    struct {
       DWORD     cb;
       LPSTR     lpReserved;
       LPSTR     lpDesktop;
       LPSTR     lpTitle;
       DWORD     dwX;
       DWORD     dwY;
       DWORD     dwXSize;
       DWORD     dwYSize;
       DWORD     dwXCountChars;
       DWORD     dwYCountChars;
       DWORD     dwFillAttribute;
       DWORD     dwFlags;
       WORD      wShowWindow;
       WORD      cbReserved2;
       LPBYTE    lpReserved2;
       DWORD     hStdInput;
       DWORD     hStdOutput;
       DWORD     hStdError;
      } StartupInfo;

    struct {
       DWORD     hProcess;
       DWORD     hThread;
       DWORD     dwProcessId;
       DWORD     dwThreadId;
      } ProcessInfo;


      _hmemset(&StartupInfo, 0, sizeof(StartupInfo));

      StartupInfo.cb = sizeof(StartupInfo);

      StartupInfo.wShowWindow = lpParmBlock->lpCmdShow->nCmdShow;
      StartupInfo.dwFlags = STARTF_USESHOWWINDOW;

      if(lpStdOut || lpStdIn)
      {
         dw2 = lpGetProcAddress32W(hKernel32, "CreateFileA");
         dw3 = lpGetProcAddress32W(hKernel32, "GetStdHandle");

         StartupInfo.dwFlags |= STARTF_USESTDHANDLES;

         // convert current stdin, stdout, and stderr to 32-bit handles

         if(lpStdIn)
         {
            // use 32-bit 'CreateFile()' to create output files
            // for these...

            StartupInfo.hStdInput =
              lpCallProcEx32W(CPEX_DEST_STDCALL | 7, 0x1, dw2, lpStdIn,
                              0x80000000L, // GENERIC_READ
                              3L, // FILE_SHARE_READ | FILE_SHARE_WRITE
                              0L, // security (NULL)
                              3L, // open existing
                              0L, 0L);
         }
         else
         {
            StartupInfo.hStdInput =
              lpCallProcEx32W(CPEX_DEST_STDCALL | 1, 0x1, dw3,
                              (DWORD)STD_INPUT_HANDLE);
         }

         if(lpStdOut)
         {
          BOOL bAppend = FALSE;

            if(*lpStdOut == '>') // append
            {
               lpStdOut++;
               bAppend = TRUE;
            }

            // use 32-bit 'CreateFile()' to create output files
            // for these...

            StartupInfo.hStdOutput =
              lpCallProcEx32W(CPEX_DEST_STDCALL | 7, 0x1, dw2, lpStdOut,
                              0xc0000000L, // GENERIC_READ | GENERIC_WRITE
                              3L, // FILE_SHARE_READ | FILE_SHARE_WRITE
                              0L, // security (NULL)
                              3L, // open existing
                              0L, 0L);


//            StartupInfo.hStdError  = StartupInfo.hStdOutput;

            if(bAppend)
            {
               dw2 = lpGetProcAddress32W(hKernel32, "SetFilePointer");

               lpCallProcEx32W(CPEX_DEST_STDCALL | 4, 0, dw2,
                               StartupInfo.hStdOutput,
                               0L, 0L, SEEK_END);  //
            }
         }
         else
         {
            StartupInfo.hStdOutput =
              lpCallProcEx32W(CPEX_DEST_STDCALL | 1, 0x1, dw3,
                              (DWORD)STD_OUTPUT_HANDLE);

//            StartupInfo.hStdError  =
//              lpCallProcEx32W(CPEX_DEST_STDCALL | 1, 0x1, dw3,
//                              (DWORD)STD_ERROR_HANDLE);
         }


         // keep existing 'STDERR' for now - later, I may put this into
         // a special file and display it locally without piping...

         StartupInfo.hStdError  =
                lpCallProcEx32W(CPEX_DEST_STDCALL | 1, 0x1, dw3,
                                (DWORD)STD_ERROR_HANDLE);
      }


      // build a program arg list for 'SFTSH32X'

      wsprintf(tbuf, "%s $%04x ", (LPSTR)szHelperProg, (WORD)hMainWnd);

      // for the purpose of ROBUSTNESS, I shall allocate a buffer for the
      // program name and command line, then assemble it and pass NULL for
      // the program name to the 'CreateProcess' function...

      if(lpProgName)  // not NULL...
      {
         lpBuf = (LPSTR)GlobalAllocPtr(GPTR, 
                                       lstrlen(lpProgName) + lstrlen(tbuf)
                                       + lpParmBlock->lpCmdLine[0] + 2);

      }
      else
      {
         lpBuf = (LPSTR)GlobalAllocPtr(GPTR,
                                       lstrlen(tbuf)
                                       + lpParmBlock->lpCmdLine[0] + 2);
      }

      if(lpBuf)
      {
         lstrcpy(lpBuf, tbuf); // 'helper' app for 32bit and WINOLDAP
                               // with '$xxxx ' window handle info
                               // (has extra space at the end already)

         if(lpProgName)
         {
            lstrcat(lpBuf, lpProgName);
         }

         if(lpParmBlock->lpCmdLine[0])
         {
            if(lpProgName)
            {
               lstrcat(lpBuf, " ");
            }

            lpCmdLine = lpBuf + lstrlen(lpBuf);

            _hmemcpy(lpCmdLine, lpParmBlock->lpCmdLine + 1,
                     lpParmBlock->lpCmdLine[0]);

            lpCmdLine[lpParmBlock->lpCmdLine[0]] = 0;
         }

         lpCmdLine = lpBuf;
         lpProgName = NULL;
      }
      else
      {
         lpParmBlock->lpCmdLine[1 + lpParmBlock->lpCmdLine[0]] = 0;

         lpCmdLine = lpParmBlock->lpCmdLine + 1;
         lpProgName = (LPSTR)szHelperProg;    // 'helper' app for 32bit and WINOLDAP
      }


      MthreadSleep(10);      // give everything a chance to run, immediately
                             // before I do anything with handles, etc.

      if(!lpNewEnv)
      {
         lpEnv0 = (LPSTR)MAKELP(lpParmBlock->wEnvSeg,0);
      }
      else
      {
         lpEnv0 = lpNewEnv;
      }


      if(!lpCallProcEx32W(CPEX_DEST_STDCALL | 10,
                          0x000003cfL, dw1,
                          (DWORD)lpProgName,
                          (DWORD)lpCmdLine,
                          (DWORD)0,
                          (DWORD)0,
                          (DWORD)0,
                          (DWORD)(NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE
                                  | CREATE_SUSPENDED),
                          (DWORD)lpEnv0,
                          (DWORD)0,
                          (DWORD)(LPVOID)(&StartupInfo),
                          (DWORD)(LPVOID)(&ProcessInfo)
                          ))
      {
         dw2 = lpGetProcAddress32W(hKernel32, "GetLastError");

         if(dw2)
         {
            // let's find out what happened (exactly)

            dw3 = lpCallProcEx32W(CPEX_DEST_STDCALL | 0, 0, dw2);

            // examine error code and generate appropriate 16-bit equivalent
            //

            if(dw3 == 7 || dw3 == 9 || dw3 == 14)
            {
               wRval = 0;  // out of memory or corrupted EXE
            }
            else if(dw3 < 32)
            {
               wRval = (WORD)dw3;   // this is approximately correct
            }
            else
            {
               wRval = 31; // unknown error code or general failure
            }
         }
         else
         {
            wRval = 31;  // unknown error code or general failure
         }
      }
      else
      {
         MthreadSleep(50);   // give everything a chance to run, immediately
                             // before I do anything with handles, etc.

//         dw2 = lpGetProcAddress32W(hWOW32, "WOWHandle16");
//         if(dw2)
//         {
//            wRval = (WORD)lpCallProcEx32W(CPEX_DEST_STDCALL | 2, 0,
//                                          dw2, ProcessInfo.dwThreadId,
//                                          (DWORD)WOW_TYPE_HTASK);
//         }
//         else
//         {
//            wRval = 32;  // just in case...
//         }
//
//         MthreadSleep(100);  // sleep for 100 msecs

         dw2 = lpGetProcAddress32W(hKernel32, "ResumeThread");

         if(dw2)
         {
            lpCallProcEx32W(CPEX_DEST_STDCALL | 1, 0, dw2,  // RESUME thread
                            ProcessInfo.hThread);

            wRval = 0xffff;  // flag for later...
         }
         else
         {
//            PrintString("?Error - could not 'RESUME' thread of new process\r\n\n");
            wRval = 31;  // unknown error code or general failure
         }


         dw2 = lpGetProcAddress32W(hKernel32, "GetExitCodeProcess");

         while(wRval >= 32)
         {
            MthreadSleep(50);

            if(dw2)
            {
               if(lpCallProcEx32W(CPEX_DEST_STDCALL | 2, 0x2,
                                  dw2, ProcessInfo.hProcess,
                                  (DWORD)(LPSTR)&dw3))
               {
                  if(dw3 != STILL_ACTIVE )
                  {
                     wRval = (WORD)dw3;  // program returns 'HTASK' value

                     break;
                  }
               }
               else
               {
//                  PrintString("?Error - could not get HTASK from 'SFTSH32X'\r\n\n");
                  wRval = 31;
                  break;
               }
            }
            else
            {
//               PrintString("?Error - could not 'RESUME' thread of new process\r\n\n");
               wRval = 31;
               break;
            }
         }

      }

      dw2 = lpGetProcAddress32W(hKernel32, "CloseHandle");

      lpCallProcEx32W(CPEX_DEST_STDCALL | 1, 0, dw2,
                      ProcessInfo.hProcess);
      lpCallProcEx32W(CPEX_DEST_STDCALL | 1, 0, dw2,
                      ProcessInfo.hThread);

      // 'CloseHandle' calls for re-directed files (as needed)

      if(lpStdIn)
      {
         lpCallProcEx32W(CPEX_DEST_STDCALL | 1, 0, dw2,
                         StartupInfo.hStdInput);
      }

      if(lpStdOut)
      {
         lpCallProcEx32W(CPEX_DEST_STDCALL | 1, 0, dw2,
                         StartupInfo.hStdOutput);
      }
   }
   else
   {
      PrintString("?Unable to invoke 'CreateProcess' via W.O.W. API\r\n");

      wRval = (WORD)LoadModule(lpProgName, lpParmBlock);
   }


   if(lpNewEnv)
   {
      GlobalFreePtr(lpNewEnv);
   }

   if(lpBuf)
   {
      GlobalFreePtr(lpBuf);
   }

   return(wRval);
}



DWORD FAR PASCAL GetThreadIDFromHTask(HTASK hTask)
{
DWORD dw1, dwRval;


   if(!hKernel32 || !hWOW32)
   {
      return((DWORD)-1L);
   }


   dw1 = lpGetProcAddress32W(hWOW32, "WOWHandle32");

   if(!dw1)  return((DWORD)-1L);


   dwRval = lpCallProcEx32W(CPEX_DEST_STDCALL | 2, 0, dw1,
                            (DWORD)(WORD)hTask, (DWORD)WOW_TYPE_HTASK);


   return(dwRval);
}


DWORD FAR PASCAL GetProcessIDFromHTask(HTASK hTask)
{
DWORD dw1, dw2, dwRval;
TASKENTRY te;


   // see description of TDB (below)

   return(0);  // for now...


//   _fmemset((LPSTR)&te, 0, sizeof(te));
//
//   te.dwSize = sizeof(te); /* initializes structure */
//
//   if(!lpTaskFindHandle || !lpTaskFindHandle((LPTASKENTRY)&te, hTask))
//   {
//      return((DWORD)-1L);
//   }
//
//   if(te.hInst == (HINSTANCE)te.hTask)
//   {
//      dw2 = (WORD)te.hModule;
//   }
//   else
//   {
//      dw2 = (WORD)te.hInst;
//   }
//
//
//   if(!hKernel32 || !hWOW32)
//   {
//      return((DWORD)-1L);
//   }
//
//   dw1 = lpGetProcAddress32W(hWOW32, "WOWHandle32");
//
//   if(!dw1)  return((DWORD)-1L);
//
//
//   dwRval = lpCallProcEx32W(CPEX_DEST_STDCALL | 2, 0, dw1,
//                            (DWORD)dw2, (DWORD)WOW_TYPE_HTASK);
//
//   return(dwRval);
}


DWORD FAR PASCAL DoOpenProcess(DWORD dwProcessID)
{
DWORD dw1;


   if(!hKernel32 || !hWOW32)
   {
      return((DWORD)-1L);
   }


   dw1 = lpGetProcAddress32W(hKernel32, "OpenProcess");

   return(lpCallProcEx32W(CPEX_DEST_STDCALL | 3, 0, dw1,
                          (DWORD)PROCESS_QUERY_INFORMATION,
                          (DWORD)0, dwProcessID));
}

DWORD FAR PASCAL DoGetExitCodeProcess(DWORD dwhProcess)
{
DWORD dw1, dwRval = (DWORD)-1L;


   if(!hKernel32 || !hWOW32)
   {
      return((DWORD)-1L);
   }


   dw1 = lpGetProcAddress32W(hKernel32, "GetExitCodeProcess");

   if(!lpCallProcEx32W(CPEX_DEST_STDCALL | 2, 0x2, dw1, dwhProcess,
                       (DWORD)(LPSTR)&dwRval))
   {
      dwRval = (DWORD)-1L;
   }

   return(dwRval);
}

DWORD FAR PASCAL DoCloseHandle(DWORD dwHandle)
{
DWORD dw1;


   if(!hKernel32 || !hWOW32)
   {
      return((DWORD)-1L);
   }


   dw1 = lpGetProcAddress32W(hKernel32, "CloseHandle");

   return(lpCallProcEx32W(CPEX_DEST_STDCALL | 1, 0, dw1, dwHandle));

}


WORD FAR PASCAL DosRunProgram(LPSTR lpProgName, LPDOSRUNPARMBLOCK lpParmBlock)
{
register WORD rval;
LPSTR lpDot;



   for(lpDot = lpProgName + lstrlen(lpProgName) - 1;
       lpDot>lpProgName && *lpDot!='.'; lpDot--)
      ;  /* find the last '.' in the file name */

                   /***********************************/
                   /* DETECT AND PROCESS BATCH FILES! */
                   /***********************************/

   if(lpDot>lpProgName && toupper(lpDot[1])=='B'
       && toupper(lpDot[2])=='A' && toupper(lpDot[3])=='T')
   {
      return(ExecuteBatchFile(lpProgName, lpParmBlock));
   }


                 /***************************************/
                 /** NORMAL 'RUN DOS PROGRAM' FUNCTION **/
                 /***************************************/

   _asm
   {
      push bx
      push dx
      push ds
      push es

      lds dx, DWORD PTR lpProgName
      les bx, DWORD PTR lpParmBlock

      mov ax, 0x4b00                 /* load and execute a program!! */

      call DOS3Call                  /* run tha suckah */

      pop es
      pop ds
      pop dx
      pop bx

      mov rval, ax
   }

   return(rval);
}


void  _dos_setvect( unsigned intnum, void (_interrupt _far *handler)() )
{
   _asm
   {
      mov ax, 0x2523
      push dx
      push ds
      mov ds,WORD PTR handler+2
      mov dx,WORD PTR handler
      call DOS3Call
      pop ds
      pop dx
   }
}


BOOL FAR PASCAL IsCONDevice(HFILE hFile)
{
WORD rval;


   _asm

   {
      mov ax,0x4400          /* Function 44H subfunction 0 */
      mov bx,WORD PTR hFile  /* file handle in 'bh' */

      int 0x21               /* IOCTL device information */

      jnc ok_news            /* carry set on error */

      mov dx, 0              /* forces return of FALSE */

ok_news:
      mov rval, dx           /* device flags!! */
   }

   if(rval & 0x0080)         /* is it a device? */
   {
      if(rval & 3)           /* is it STDIN or STDOUT? */
      {
         return(TRUE);       /* YES!  Therefore, assume CON */
      }
   }

   return(FALSE);


}



// THE FOLLOWING FUNCTION RETURNS 'TRUE' IF THE PORT NAME IS A VALID PRINTER
// PORT; OTHERWISE, IT RETURNS 'FALSE'.

BOOL FAR PASCAL IsValidPortName(LPCSTR szPortName)
{
HFILE hFile;
WORD wDevInfo;

#ifdef WIN32
#error need to re-write this for WIN32
#endif // WIN32


  // First, attempt to open the name as a device...

   hFile = _lopen(szPortName, READ | OF_SHARE_DENY_NONE);
   if(hFile == HFILE_ERROR)  // can't open port
   {
      return(FALSE);
   }

   // use IOCTL to see if this is a CHARACTER device or not...

   _asm
   {
      mov ax, 0x4400
      mov bx, hFile

      call DOS3Call
      jnc no_error

      mov dx, 0

no_error:
      mov wDevInfo, dx
   }

   _lclose(hFile);

   // note:  'wDevInfo' bit 0=STDIN, bit 1=STDOUT, bit 2=NUL

   if(!(wDevInfo & 0x80) || (wDevInfo & 7))  // NOT character device or CON or NUL??
   {
      return(FALSE);  // either not char device or it's CON - bail out
   }


   // at this point we have a valid port, recognized by a device name of
   // some type, and it's a character device.  theoretically it ought to
   // be possible to PRINT to it (one way or another), so I shall return
   // TRUE to indicate that it's a valid 'port name'.

   return(TRUE);


//            else if(!_fstricmp(work_buf, "PRN") ||              // PRN
//                    !_fstricmp(work_buf, "PRN.") ||             // PRN.
//                    !_fstricmp(work_buf, "FAX") ||              // FAX
//                    !_fstricmp(work_buf, "FAX.") ||             // FAX.
//                    !_fstricmp(work_buf, "PUB") ||              // PUB
//                    !_fstricmp(work_buf, "PUB.") ||             // PUB.
//                    !_fstricmp(work_buf, "LPT") ||              // LPT
//                    !_fstricmp(work_buf, "LPT.") ||             // LPT.
//                    (!_fstrnicmp(work_buf, "LPT", 3) &&         // LPT#
//                     work_buf[3]>='1' && work_buf[3] <= '9' &&  // LPT0 - LPT9
//                     (work_buf[4]==0 ||                         // LPT#
//                      (work_buf[4]=='.' && work_buf[5]==0))))   // LPT#.
//
//            else if(!_fstricmp(work_buf, "AUX") ||              // AUX
//                    !_fstricmp(work_buf, "AUX.") ||             // AUX.
//                    !_fstricmp(work_buf, "COM") ||              // COM
//                    !_fstricmp(work_buf, "COM.") ||             // COM.
//                    (!_fstrnicmp(work_buf, "COM", 3) &&         // COM#
//                     work_buf[3]>='1' && work_buf[3] <= '9' &&  // COM0 - COM9
//                     (work_buf[4]==0 ||                         // COM#
//                      (work_buf[4]=='.' && work_buf[5]==0))))   // COM#.

}

// THE FOLLOWING FUNCTION RETURNS A PRINTER DC OR NULL ON ERROR BASED
// UPON THE PORT NAME.  An entry MUST exist in WIN.INI for the port!!

HDC FAR PASCAL CreatePrinterDCFromPort(LPCSTR szPortName)
{
char tbuf[256], tbuf2[256], tbuf3[256], szPort[64];
LPSTR lp1, lp2, lp3;


   if(!szPortName || !*szPortName ||
      !IsValidPortName(szPortName)) return(NULL);

   _fstrncpy(szPort, szPortName, sizeof(szPort));
   if(szPort[lstrlen(szPort) - 1] == '.')
   {
      szPort[lstrlen(szPort) - 1] = 0;
   }

   if(!*szPort)
   {
      return(NULL);  // just in case...
   }


   // look through INI file to find out which printer uses this port.
   // if NONE, try using 'generic' (or DEFAULT) printer driver...

   GetProfileString("windows", "device", "", tbuf, sizeof(tbuf));

   if(!*tbuf) return(NULL);

   for(lp1=tbuf; *lp1; lp1++)
   {
      if(*lp1 == ',') *lp1 = 0;
   }

   *(++lp1) = 0;  // terminate with "double NULL"


   if(!_fstricmp(szPort, "LPT") || !_fstricmp(szPort, "PRN"))
   {
      // default printer - use specs from 'device' setting (above)

      for(lp1=tbuf; *lp1; lp1++)
         ;  // find 1st end of string

      lp1++;

      for(lp2=lp1; *lp2; lp2++)
         ;  // find 2nd end of string

      if(lp2 > lp1) lp2++;

      return(CreateDC(lp1, tbuf, lp2, NULL));

   }


   // get a list of items under 'Printer Ports' and find one that matches...

   GetProfileString("Printer Ports", NULL, "", tbuf2, sizeof(tbuf2));

   if(!*tbuf2) // none in use...
   {
      return(NULL);
   }

   for(lp3=tbuf2; *lp3; lp3 += lstrlen(lp3) + 1)
   {

      GetProfileString("Printer Ports", lp3, "", tbuf3, sizeof(tbuf3));

      if(!*tbuf3) continue;


      for(lp1=tbuf3; *lp1; lp1++)
      {
         if(*lp1 == ',') *lp1 = 0;
      }

      *(++lp1) = 0;  // terminate with "double NULL"


      for(lp1=tbuf3; *lp1; lp1++)
         ;  // find 1st end of string

      lp1++;

      for(lp2=lp1; *lp2; lp2++)
         ;  // find 2nd end of string

      if(lp2 > lp1) lp2++;


      // see if device names match

      if(!_fstrnicmp(szPort, lp2, lstrlen(szPort)) &&
         lp2[lstrlen(szPort)] == ':')
      {
         // A MATCH!!!  This is the device I've been WAITING for!

         return(CreateDC(lp1, tbuf3, lp2, NULL));
      }

   }


   return(NULL);  // no match found

}



/***************************************************************************/
/*                  REALLY NUTSO DOS INTERNAL STUFF                        */
/***************************************************************************/


//#pragma alloc_text(NUTSO_DOS_TEXT,GetPhysDriveFromLogical,GetDPB,
//                   GetDosCurDirEntry,MyAllocSelector,MyChangeSelector,
//                   MyFreeSelector,RealModeInt)


#pragma code_seg("NUTSO_DOS_TEXT","CODE")

 /** This next function obtains the PHYSICAL drive # from the LOGICAL **/
 /** by using the 'current directory' map.  Returns 0xFFFF on error.  **/

 // NOTE:  'wDrive' is 1-based (1 == 'A', 2 == 'B', etc.)

WORD FAR PASCAL GetPhysDriveFromLogical(WORD wDrive)
{
LPCURDIR lpC;
LPDPB lpDPB;
WORD wRval;


   lpC = (LPCURDIR) GetDosCurDirEntry(wDrive);
   if(!lpC)
   {
      if(GetDriveType(wDrive - 1)==DRIVE_REMOTE)
      {
         return(wDrive - 1);  // for NETWORK drive, don't return error
                              // just return the drive letter (0-based)
      }
      else
      {
         return(0xffff);  // error
      }
   }
   else
   {
#ifdef WIN32

      lpDPB = HIWORD(lpC->dwDPB)*16L + LOWORD(lpC->dwDPB);

      // read absolute memory address if pointer is "invalid"

      if(IsBadReadPtr(&(lpDPB->drive), sizeof(lpDPB->drive))
      {
       DWORD dw1;

         wRval = 0;

         if(!ReadProcessMemory(GetCurrentProcess(), 
                               &(lpDPB->drive), &wRval,
                               sizeof(lpDPB->drive), &dw1))
         {
            MyOutputDebugString("?ReadProcessMemory() fails to get DPB info\r\n");

            wRval = wDrive - 1;  // operation fails - assume logical==physical
         }
      }
      else
      {
        wRval = lpDPB->drive;
      }

#else  // WIN32
      WORD wSel = MyAllocSelector();

      MyChangeSelector(wSel, HIWORD(lpC->dwDPB)*16L + LOWORD(lpC->dwDPB),
                       0xffff);

      lpDPB = (LPDPB)MAKELP(wSel, 0);

      wRval = lpDPB->drive;      /* return the 'drive' entry in DPB */

      MyFreeSelector(wSel);
#endif // WIN32
   }

#ifdef WIN32
   GlobalFreePtr(lpC);
#else // WIN32
   MyFreeSelector(SELECTOROF(lpC));
#endif // WIN32

   return(wRval);

}


 // NOTE:  'wDrive' is 1-based (1 == 'A', 2 == 'B', etc.)

BOOL FAR PASCAL GetDPB(WORD wDrive, LPDPB lpDest)
{
LPCURDIR lpC;
LPDPB lpDPB;
#ifndef WIN32
LPDPB3 lpDPB3;
#endif // WIN32
WORD wSel;
DWORD dwVer;
BOOL bRval = FALSE;


   dwVer = GetVersion();
#ifndef WIN32
   if(HIWORD(dwVer)<0x300) return(NULL);  /* can't do for 2.x */
#endif // WIN32

   lpC = (LPCURDIR)GetDosCurDirEntry(wDrive);
   if(!lpC)
   {
      return(TRUE);
   }
#ifdef WIN32
   else
   {
      wSel = MyAllocSelector();

      MyChangeSelector(wSel, HIWORD(lpC->dwDPB)*16L + LOWORD(lpC->dwDPB),
                       0xffff);

      lpDPB = (LPDPB)MAKELP(wSel, 0);

      _fmemcpy((LPSTR)lpDest, (LPSTR)lpDPB, sizeof(*lpDPB));

      lpDPB = HIWORD(lpC->dwDPB)*16L + LOWORD(lpC->dwDPB);

      // read absolute memory address if pointer is "invalid"

      if(IsBadReadPtr(lpDPB, sizeof(*lpDPB))
      {
       DWORD dw1;

         if(!ReadProcessMemory(GetCurrentProcess(), 
                               &lpDPB, (LPVOID)lpDest,
                               sizeof(*lpDPB), &dw1))
         {
            MyOutputDebugString("?ReadProcessMemory() fails to get DPB info\r\n");
            bRval = TRUE;
         }
      }
      else
      {
         memcpy((LPSTR)lpDest, (LPSTR)lpDPB, sizeof(*lpDPB));
      }
   }
#else  // WIN32
   else if(dwVer<0x400)
   {
      wSel = MyAllocSelector();

      MyChangeSelector(wSel, HIWORD(lpC->dwDPB)*16L + LOWORD(lpC->dwDPB),
                       0xffff);

      lpDPB3 = (LPDPB3)MAKELP(wSel, 0);

      _fmemset((LPSTR)lpDest, 0, sizeof(*lpDPB));
      _fmemcpy((LPSTR)lpDest, (LPSTR)lpDPB3, sizeof(*lpDPB3));
   }
   else
   {
      wSel = MyAllocSelector();

      MyChangeSelector(wSel, HIWORD(lpC->dwDPB)*16L + LOWORD(lpC->dwDPB),
                       0xffff);

      lpDPB = (LPDPB)MAKELP(wSel, 0);

      _fmemcpy((LPSTR)lpDest, (LPSTR)lpDPB, sizeof(*lpDPB));
   }

   MyFreeSelector(wSel);
#endif // WIN32

#ifdef WIN32
   GlobalFreePtr(lpC);
#else // WIN32
   MyFreeSelector(SELECTOROF(lpC));
#endif // WIN32

   return(bRval);

}



#ifndef WIN32

WORD FAR PASCAL GetDosMaxDrives(void)
{
DWORD dwVer;
LPSTR lpLOL;


   dwVer = GetVersion();

   if(HIWORD(dwVer)<0x300) return(NULL);  /* can't do for 2.x */


   _asm
   {
      mov ax,0x5200
      int 0x21
      mov WORD PTR lpLOL+2 ,es
      mov WORD PTR lpLOL, bx
   }


   if(HIWORD(dwVer)==0x300)
   {
      return((WORD) *((BYTE FAR *)(lpLOL + 0x1b)));
   }
   else
   {
      return((WORD) *((BYTE FAR *)(lpLOL + 0x21)));
   }
}


 /** This function gets the 'CURRENT DIRECTORY' structure entry for **/
 /** the logical drive specified by 'wDrive' and returns a pointer  **/
 /** to it.                                                         **/
 /**                                                                **/
 /** IMPORTANT:  THE CALLER MUST FREE THE POINTER'S SELECTOR WHEN   **/
 /**             ITS USE IS COMPLETE BY CALLING 'MyFreeSelector()'! **/


 // NOTE:  To get pointer to entire list, pass '1' as 'wDrive' value!
 //        Then, use 'GetDosMaxDrives()' to determine "max_index - 1".
 //
 //        'wDrive' is 1-based (1 == 'A', 2 == 'B', etc.)

LPVOID FAR PASCAL GetDosCurDirEntry(WORD wDrive)
{
DWORD dwVer;
LPSTR lpLOL;
WORD wSel, wMaxDrives;
LPVOID lpRval;
LPCURDIR3 lpC3;
LPCURDIR lpC;


   if(!wDrive)  /* the CURRENT drive */
      wDrive = _getdrive();

           /** Use INT 21H function 52H to get List of Lists **/
           /** It very kindly returns a SELECTOR in ES!      **/

   _asm
   {
      mov ax,0x5200
      int 0x21
      mov WORD PTR lpLOL+2 ,es
      mov WORD PTR lpLOL, bx
   }

          /** Obtain current DOS version using 'GetVersion()' **/
          /** then, dispatch accordingly!                     **/

   dwVer = GetVersion();

   if(HIWORD(dwVer)<0x300) return(NULL);  /* can't do for 2.x */


   wMaxDrives = GetDosMaxDrives();


            /** Ensure that drive letter is 'in range' **/

   if(wMaxDrives < wDrive) /* drive ID too large! */
   {
      return(NULL);
   }


   wSel = MyAllocSelector();
   if(!wSel) return(NULL);     // error - can't alloc selector

   if(HIWORD(dwVer)<0x400)
   {
      if(HIWORD(dwVer)==0x300)
      {
         lpC3 = (LPCURDIR3) *((DWORD FAR *)(lpLOL + 0x17));
      }
      else
      {
         lpC3 = (LPCURDIR3) *((DWORD FAR *)(lpLOL + 0x16));
      }

      MyChangeSelector(wSel, HIWORD(lpC3) * 16L + LOWORD(lpC3), 0xffff);

      lpC3 = (LPCURDIR3)MAKELP(wSel, 0);


      lpC3 += (wDrive - 1);          /* add the offset of the drive table */

      lpRval = (LPVOID)lpC3;

   }
   else
   {
      lpC = (LPCURDIR) *((DWORD FAR *)(lpLOL + 0x16));


      MyChangeSelector(wSel, HIWORD(lpC) * 16L + LOWORD(lpC), 0xffff);

      lpC = (LPCURDIR)MAKELP(wSel, 0);


      lpC += (wDrive - 1);           /* add the offset of the drive table */

      lpRval = (LPVOID)lpC;

   }


   if(!lpC->pathname[0])  // not "filled in" yet... (this happens on NT)
   {
     // get the current directory for this drive, and do a 'chdir' to
     // force it correct...
     
     HANDLE h1;
     char NEAR *p1;

     h1 = LocalAlloc(LMEM_MOVEABLE, MAX_PATH);
     if(!h1)
     {
       MyFreeSelector(wSel);
       return(NULL);
     }
     
     p1 = (char NEAR *)LocalLock(h1);
     if(!p1)
     {
       LocalFree(h1);
       
       MyFreeSelector(wSel);
       return(NULL);
     }


     if(!_getdcwd(wDrive, p1, MAX_PATH))
     {
       LocalUnlock(h1);
       LocalFree(h1);
       
       MyFreeSelector(wSel);
       return(NULL);
     }

     // OK, at this point 'p1' should be the same as lpC->pathname[]...
     
     if(lpC->pathname[0])  // is it?
     {
       // TODO:  'ASSERT' that they match...
     
       LocalUnlock(h1);
       LocalFree(h1);
       
       return(lpRval);
     }

     // let's see if we can SET the directory
     
     lstrcpy(lpC->pathname, p1);
     
     LocalUnlock(h1);
     LocalFree(h1);
   }

   return(lpRval);

}


      /** This function allocates an LDT selector using DPMI **/

WORD FAR PASCAL MyAllocSelector(void)
{
WORD wMySel;

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


   return(wMySel);  /* returns 0 on error, or selector if OK */

}


      /** This function alters a selector's ADDRESS and LIMIT **/

BOOL FAR PASCAL MyChangeSelector(WORD wSelector, DWORD dwAddr, WORD wLimit)
{
register WORD wRval;

   if(!wSelector) return(TRUE);

   _asm
   {
         mov ax, 0x0007               ; DPMI 'set selector base' function
         mov bx, WORD PTR wSelector
         mov cx, WORD PTR dwAddr + 2
         mov dx, WORD PTR dwAddr      ; address of selector in linear memory

         int 0x31
         jc  was_error

         mov ax, 0x0008               ; DPMI 'set selector limit' function
         mov bx, WORD PTR wSelector
         mov cx, 0
         mov dx, wLimit               ; segment limit

         int 0x31
         jnc no_error

was_error:
         mov ax, 1
         jmp i_am_done

no_error:
         mov ax, 0;

i_am_done:

         mov wRval, ax           /* return 0 if OK, 1 if ERROR */
   }

   return(wRval);
}



 /** This function frees a selector allocated using 'MyAllocSelector()' **/

void FAR PASCAL MyFreeSelector(WORD wSelector)
{
   if(!wSelector) return;

   _asm
   {
      mov ax, 1
      mov bx, WORD PTR wSelector
      int 0x31                ; DPMI call to Free LDT Selector
   }
}



DWORD FAR PASCAL MyGetSelectorBase(WORD wSelector)
{
DWORD dwBase;


   if(!wSelector) return(TRUE);

   _asm
   {
         mov ax, 0x0006               ; DPMI 'set selector base' function
         mov bx, WORD PTR wSelector

         int 0x31
         jnc not_error

         mov cx, 0xffff
         mov dx, 0xffff

not_error:
         mov WORD PTR dwBase, dx
         mov WORD PTR dwBase + 2, cx
   }

   return(dwBase);

}

DWORD FAR PASCAL MyGetSelectorLimit(WORD wSelector)
{
DWORD dwResult = 0;
static BOOL Is386 = 0, CheckedIs386 = 0;


   if(!CheckedIs386)
   {
    DWORD wf = GetWinFlags();

      if((wf & WF_PMODE) && ((wf & WF_ENHANCED) || !(wf & WF_CPU286)))
      {
         Is386 = 1;
      }

      CheckedIs386 = 1;
   }


   if(!wSelector) return(0);

   _asm
   {
      mov ax, wSelector
      test BYTE PTR Is386, 0xff
      jz not_386

      _asm _emit(0x66) _asm _emit(0xf) _asm _emit(0x3) _asm _emit(0xc0);  // lsl ax, ax

      _asm _emit(0x66) _asm mov WORD PTR dwResult, ax

      jmp was_386

not_386:

      _asm _emit(0xf) _asm _emit(0x3) _asm _emit(0xc0);  // lsl ax, ax

      _asm _asm mov WORD PTR dwResult, ax

was_386:

   }

   return(dwResult);

}



BOOL FAR PASCAL RealModeInt(WORD wIntNum, LPREALMODECALL lpIR)
{
register BOOL rval;


   _asm
   {
      push es
      push ds

      mov ax, 0x300
      mov bl, BYTE PTR wIntNum
      mov bh, 0               /* do not reset INT ctrlr & A20 line */
      mov cx, 0               /* do not copy parms onto real mode stack */
      les di, lpIR            /* the 'INTREGS' structure  */
      int 0x31
      xor ax, ax
      adc ax, 0               /* AX=1 if carry set, 0 if clear */

      mov rval, ax

      pop ds
      pop es
   }

   return(rval);              /* value in AX returns 0 on GOOD, 1 on ERROR */



}

#endif // WIN32



#if 0
     /** the old 'GetDPB()' function - I'll improve it later **/


char GetDPBRealMode[]={(BYTE)0x89, (BYTE)0xde, (BYTE)0xbf,
                       (BYTE)0,    (BYTE)0,    (BYTE)0xb9,
                       LOBYTE(sizeof(DPB)), HIBYTE(sizeof(DPB)),
                       (BYTE)0xfc, (BYTE)0xf3, (BYTE)0xa4,
                       (BYTE)0xcb, (BYTE)0};


BOOL FAR PASCAL OldGetDPB(WORD wDrive, LPDPB lpDPB)
{
REALMODECALL rmc;
DWORD dwDosBlock;



   dwDosBlock = GlobalDosAlloc(sizeof(DPB)+ 512);

   if(!dwDosBlock)
   {
      PrintErrorMessage(792);

      return(TRUE);
   }

   rmc.EAX = 0x3200;
   rmc.EDX = wDrive;
   rmc.SS  = HIWORD(dwDosBlock);
   rmc.DS  = HIWORD(dwDosBlock);
   rmc.ES  = HIWORD(dwDosBlock);

   rmc.SP  = (sizeof(DPB)+510) & 0xfffe;  /* end of the stack area, minus 2 */


            /* int 21H function 32 - returns AL==FF on error */

   if(RealModeInt(0x21, &rmc) || (rmc.EAX & 0xff)==0xff)
   {
      GetExtErrMessage(work_buf);
      PrintString(work_buf);       /* for now... */

      GlobalDosFree(LOWORD(dwDosBlock));
      return(TRUE);
   }


    /* next step:  copy section of code into real mode block from above */
    /*             that will make a copy of the DPB into the real mode  */
    /*             block so I can then copy it to the destination.      */

   rmc.CS  = HIWORD(dwDosBlock);
   rmc.ES  = HIWORD(dwDosBlock); /* ES may have changed */
                                 /* DS:BX already contains the source addr. */
   rmc.SS  = HIWORD(dwDosBlock);
   rmc.SP  = (sizeof(DPB)+510) & 0xfffe;  /* end of the stack area, minus 2 */
   rmc.IP  = sizeof(DPB);        /* offset from beginning of segment */

   _asm
   {
      push es
      push ds

      mov es, WORD PTR dwDosBlock

      mov si, OFFSET GetDPBRealMode
      mov di, size DPB                 /* offset into 'real mode' area */

      mov cx, size GetDPBRealMode      /* copy 'real mode' code into block */

      cld
      rep movsb   /* move the code section into the 'real mode' area */

      pop ds
      pop es


      mov ax, 0x301           /* DPMI - real mode call with RETF frame */
      mov bl, 0
      mov bh, 0               /* do not reset INT ctrlr & A20 line */
      mov cx, 0               /* do not copy parms onto real mode stack */
      push ss
      pop es                  /* the segment for the 'INTREGS' structure */
      lea di, WORD PTR rmc    /* the 'INTREGS' structure  */
      int 0x31

      /* for errors see if carry is set - I won't check though */



      push ds
      push es

      les di, DWORD PTR lpDPB;

      mov ds, WORD PTR dwDosBlock
      mov si, 0               /* offset to DPB info */

      mov cx, size DPB
      cld
      rep movsb              /* copy DPB stuff to dest (finally!) */

      pop es
      pop ds

   }

   GlobalDosFree(LOWORD(dwDosBlock));  /* free the DOS memory now */

   return(FALSE);

}

#endif // 0


#pragma code_seg()



#ifndef WIN32

/***************************************************************************/
/*                                                                         */
/*  TOOLHELP CLONES - Functions that mimic equivalent ToolHelp functions   */
/*                    for Windows 3.0 when TOOLHELP is not available!      */
/*                                                                         */
/***************************************************************************/


#pragma code_seg("TOOLHELP_CLONE_TEXT","CODE")

BOOL FAR PASCAL MyMemManInfo(LPMEMMANINFO lpInfo)
{
#define mmi (__mmi._mmi_)
struct {
  MEMMANINFO _mmi_;
  char junk[32];  /* 32 bytes of junk, just to make sure I don't overflow */
  } __mmi;


   if(lpInfo==(LPMEMMANINFO)NULL || lpInfo->dwSize!=sizeof(MEMMANINFO))
      return(FALSE);

   mmi.dwSize = sizeof(MEMMANINFO);

   _asm
   {
      push es
      push di

//      lea di, WORD PTR mmi.dwLargestFreeBlock
      lea di, WORD PTR __mmi._mmi_.dwLargestFreeBlock
                                       /* the first element after 'dwSize' */
      push ss
      pop es

      mov ax, 0x0500    /* get memory manager info - documented in DDK */

      int 0x31                 /* Windows internal interrupt!! */

      pop di                       /* restore stack stuff */
      pop es
   }

   _fmemcpy((LPSTR)lpInfo, (LPSTR)&mmi, sizeof(MEMMANINFO));

   lpInfo->wPageSize = 0x1000;            /* that's the way it's done, ok? */

   return(TRUE);          /* IT WORKED!! */

}



BOOL FAR PASCAL MyIsTask(HTASK hTask)
{
TASKENTRY te;

   _fmemset((LPSTR)&te, 0, sizeof(TASKENTRY));

   te.dwSize = sizeof(TASKENTRY);


   if(!lpTaskFirst((LPTASKENTRY)&te))
   {
     return(FALSE);  /* didn't find it, so return FALSE (not a task) */
   }

   if(te.hTask == hTask) return(TRUE);  /* found - I'm done! */

   while(lpTaskNext((LPTASKENTRY)&te))
   {
      if(te.hTask == hTask) return(TRUE);  /* found - I'm done! */
   }

   return(FALSE);      /* not found, and I've searched the *WORLD* for it! */
}

BOOL FAR PASCAL MyTaskFirst(LPTASKENTRY lpTask)
{
HTASK FAR *lpFirst;

   lpFirst = ((HTASK FAR *)MAKELP(wIGROUP,0x1e));   /* first task in list! */
                    /* the address was hacked out of "other guy's" code... */
                        /* and what a pain in the neck that was, too!! */

   return((WORD)MyTaskFindHandle(lpTask, *lpFirst)!=0);  /* return info!! */
                /* why bother checking for a UAE - if it works, it works! */
                /* and, if it don't --- ka-boom!  The thing should work.  */
}


BOOL FAR PASCAL MyTaskNext(LPTASKENTRY lpTask)
{

   if(lpTask->hNext)
      return((WORD)MyTaskFindHandle(lpTask, (HTASK)lpTask->hNext)!=0);
   else
      return(FALSE);
}


             /**   Special structure definition for TDB   **/
             /**(Hacked-up and corrected version from DDK)**/
             /**   (the DDK version had errors in it!!)   **/

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

   // NOTE:  in 32-bit TDB, offset 52H is selector for the 'Thread' TCB, and
   //        offset 54H is 32-bit linear address for a header that appears
   //        to allow walking of the thread list that precedes the TCB
   //        selector's offset by 10H bytes.  Hacking reveals that it
   //        successfully matches the expected TCB structure, with a few
   //        minor inconsistencies.


  /*** OFFSET 5CH ***/

   LPSTR   lpPHT;       /* far ptr to private handle table */
   WORD    wPDB;        /* the PDB (or PSP) - in Win 3.x at TDB:0x100 */
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
                        /* except in CHICAGO, where it might start at 210H */

  } TDB, FAR *LPTDB;



// NOTE:  TCB is Preceded by a 10H byte block as follows:
//        DWORD SomeKindOfID
//        DWORD FlagsOfSomeKind (?) [usually zero, but process is '3']
//        DWORD PointerToProcessInfo  [this describes the process (?)]
//        DWORD PointerToNextTCB

typedef struct tagTCB {

   DWORD TCB_Flags;
   DWORD TCB_Reserved1;
   DWORD TCB_Reserved2;
   DWORD TCB_Signature;     // 1a671abeH (?)
   DWORD TCB_ClientPtr;
   DWORD TCB_VMHandle;

   // offset 18H

   DWORD TCB_ThreadId;      // I have observed this to be a DWORD...
                            // in Win 95 build 314 it points to the struct
                            // VMM declares this as a WORD member

   WORD TCB_PMLockOrigSS;       // I believe this value to be irrelevent

   DWORD TCB_PMLockOrigESP;     // I believe this value to be irrelevent
   DWORD TCB_PMLockOrigEIP;     // I believe this value to be irrelevent
   DWORD TCB_PMLockStackCount;  // this is a pointer of some type... (?)
   WORD TCB_PMLockOrigCS;       // this is a zero.. (?)

   // OFFSET 28H

   DWORD TCB_PMPSPSelector;     // by observation at offset 28H [dword]
                                // VMM declares this as a WORD member

   DWORD TCB_ThreadType;        // appears to point to end of structure...(?)

   WORD TCB_pad1;               // these 3 members form ptr to process CB(?)
   BYTE TCB_pad2;
   BYTE TCB_extErrLocus;


   WORD TCB_extErr;             // appear to be useless
   BYTE TCB_extErrAction;
   BYTE TCB_extErrClass;
   DWORD TCB_extErrPtr;


   } TCB, FAR *LPTCB;




HTASK FAR PASCAL MyTaskFindHandle(LPTASKENTRY lpTask, HTASK hTask)
{
LPTDB lpTDB;
DWORD dwTDBSeg;

   if(!(dwTDBSeg = lpGlobalHandleNoRIP((WORD)hTask)))
   {
      return(FALSE);  /* bad handle - bail out with error code */
   }

   lpTDB = (LPTDB)MAKELP(HIWORD(dwTDBSeg), 0);


   /* now the above action gets me the address without locking it!!  */
   /* this will only work in protected mode, but I'm not running any */
   /* real mode code with this program anyway, so who cares??        */
   /* by doing this I hope to avoid some problems I've noted with    */
   /* the accessing of task database segments by other tasks...      */


   /* first step - 'TDB or not TDB - that is the question' (boo, hiss) */

   if(lpTDB->bSig[0]=='T' && lpTDB->bSig[1]=='D' && /* yep, it's a TDB */
      lpTDB->hSelf==hTask )                       /* just to make sure */
   {
      lpTask->hTask       = hTask;
      lpTask->hTaskParent = NULL;  /* I don't support this one.  So what. */
      lpTask->hInst       = lpTDB->hInst;
      lpTask->hModule     = lpTDB->hModule;
      lpTask->wSS         = lpTDB->wSS;
      lpTask->wSP         = lpTDB->wSP;
      lpTask->wStackTop   = NULL;  /* if I really want this I can get it.. */
      lpTask->wStackMinimum = NULL; /* but it's too much hassle for this */
      lpTask->wStackBottom= NULL;  /* program, so I won't bother... */
      lpTask->wcEvents    = lpTDB->nEvents;

      lpTask->hNext       = lpTDB->hNext;

      _fmemset(lpTask->szModule, 0, sizeof(lpTask->szModule));

      _fmemcpy(lpTask->szModule, lpTDB->sModule, sizeof(lpTDB->sModule));

      lpTask->wPSPOffset  = 0;  /* I don't know WHAT the blazes THIS is! */

      return(hTask);                 /* it worked! it worked! it worked! */
   }


   return(FALSE);                    /* for now, this is how I handle it */

}

        /** Uses documented info from DDK (above) to determine **/
        /** if a valid task handle belongs to a WINOLDAP task! **/

BOOL FAR PASCAL IsWINOLDAP(HTASK hTask)
{
TASKENTRY te;
LPTDB lpTDB;
DWORD dwTDBSeg;


   _fmemset((LPSTR)&te, 0, sizeof(te));

   te.dwSize = sizeof(te);

   if(lpIsTask(hTask) && lpTaskFindHandle((LPTASKENTRY)&te, hTask))
   {
      dwTDBSeg = GlobalHandle((WORD)hTask);
      lpTDB = (LPTDB)MAKELP(HIWORD(dwTDBSeg), 0);

      if(lpTDB->wFlags & 1)  /* ah, hah!! */
      {
         return(TRUE);
      }
   }

       /** Either not a task, or task isn't a WINOLDAP task! **/

   return(FALSE);

}

#endif // WIN32


HTASK FAR PASCAL GetTaskFromInst(HINSTANCE hInst)
{
TASKENTRY te;

   _fmemset((LPSTR)&te, 0, sizeof(te));

   te.dwSize = sizeof(te); /* initializes structure */


   if(!lpTaskFirst((LPTASKENTRY)&te))
   {
      return(NULL);
   }

   do
   {
      if(te.hInst==hInst || te.hTask==(HTASK)hInst ||
         te.hModule==(HMODULE)hInst)
      {
         return(te.hTask);
      }

   } while(lpTaskNext((LPTASKENTRY)&te));

   return(NULL);  /* nope!  didn't find the thing!! */

}



/*
/*            reserved for later
/*
/*
/*HMODULE FAR PASCAL MyModuleFindName(LPMODULEENTRY lpModule, LPSTR lpName)
/*{
/*HMODULE hModule;
/*
/*   hModule = GetModuleHandle(lpName);  /* pretty simple, wasn't it? */
/*
/*}
*/


#pragma code_seg()





/***************************************************************************/
/*                                                                         */
/*      THE FOLLOWING IS A SPECIAL FUNCTION FOR 'LISTOPEN' and OTHERS      */
/*  It obtains the TASK and MODULE handle from a 'DOS' PSP Segment value   */
/*                                                                         */
/***************************************************************************/

#ifndef WIN32

#pragma pack(1)

#pragma code_seg("HACKOMATIC_TEXT","CODE")


// ** WINOLDAP INFORMATION - 1st NON-DEFAULT DATA SEGMENT **

typedef struct tagWOAENTRY {
   HWND hWnd;            // Window Handle for WINOLDAP
   HINSTANCE hInst;      // Instance Handle (DGROUP selector)
   WORD wFlags1;         // some kind of flag - usually '80H'
   WORD wFlags2;         // some kind of flag - usually '2'
   DWORD dwVMHandle;     // ABSOLUTE ADDRESS of Control Block for VM
   HTASK hTask;          // Task Handle for WINOLDAP Instance
   } WOAENTRY;


typedef struct tagWOAENTRY4 {
   HWND hWnd;
   HINSTANCE hInst;
   DWORD dwVMHandle;
   WORD wFlags;          // I *THINK* that this is a flag word!
   } WOAENTRY4;


typedef struct tagWOATABLE {
   WORD wReserved1;
   WORD wCode3;      // selector of code segment #3
   WORD wCount;      // reference count for WOA tasks
   HWND hwndFirst;   // WINDOW HANDLE for 'first' item in list (?)

   WOAENTRY EntryTable[1];     // table of entries (to end of segment)
                               // unused elements are ALL ZEROS
   } WOATABLE;


typedef struct tagWOATABLE4 {
   BYTE wReserved[0x184];      // as of Chicago 'M7' by hacking...
   WORD wCount;
   HWND hwndInitial;
   WOAENTRY4 EntryTable[1];

   } WOATABLE4;


typedef struct tagWOATABLE30 {
   WORD wReserved1;
   WORD wCode3;      // selector of code segment #3
   WORD wCount;      // reference count for WOA tasks

   WOAENTRY EntryTable[1];     // table of entries (to end of segment)
                               // unused elements are ALL ZEROS
   } WOATABLE30;





typedef WOAENTRY FAR *LPWOAENTRY;
typedef WOATABLE FAR *LPWOATABLE;
typedef WOATABLE4 FAR *LPWOATABLE4;
typedef WOATABLE30 FAR *LPWOATABLE30;



// CONTROL BLOCK - this is what a VM HANDLE points to (ABS address)!!

typedef struct {
   DWORD CB_VM_Status;
   DWORD CB_High_Linear;
   DWORD CB_Client_Pointer;
   DWORD CB_VMID;
   } CONTROL_BLOCK;

typedef CONTROL_BLOCK FAR *LPCONTROL_BLOCK;


LPWOATABLE FAR PASCAL GetWOATABLE();    // FUNCTION PROTOTYPE


LPTDB GetTDBFromPSP(WORD wPSPSeg)
{
TASKENTRY te;
LPTDB lpTDB;


   _hmemset((LPSTR)&te, 0, sizeof(te));

   te.dwSize = sizeof(te);

   if(!lpTaskFirst(&te))
   {
      return(NULL);
   }

   do
   {
      lpTDB = (LPTDB)MAKELP(HIWORD(GlobalHandle((WORD)te.hTask)),0);

      if(MyGetSelectorBase(lpTDB->wPDB) == (wPSPSeg * 0x10UL))
      {
         return(lpTDB);
      }

   } while(lpTaskNext(&te));


   return(NULL);

}


LPSTR FAR PASCAL GetTaskAndModuleFromPSP(WORD wMachID, WORD wPSPSeg,
                                         HTASK FAR *lphTask)
{
WORD wMySel, /* wMCB, wENV, wPSP, */ w, w2;
DWORD dwAddr, dwVer /*, dwWhen */;
LPSTR lpRval /* , lpLOL, lp1, lp2, lp3, lpOldEnvData */;
LPTDB lpTDB;
LPMCB lpMCB;
/* REALMODECALL rmc; */
LPWOATABLE lpWT;
LPWOATABLE4 lpWT4;
LPWOATABLE30 lpWT30;
LPCONTROL_BLOCK lpCB;
static char buf[32], pFmt[]="%-8.8s";
static WORD CurrentVM = 0, CurrentVMFlag=0;


   if(!CurrentVMFlag)
   {
      _asm mov ax, 0x1683    /* Windows(tm) INT 2FH interface to get VM ID */
      _asm int 0x2f
      _asm mov CurrentVM, bx

      CurrentVMFlag = TRUE;
   }

   if(lphTask) *lphTask = 0;

   wMySel = MyAllocSelector();

   if(!wMySel) return("* DPMI ERROR *");



   if(wMachID==CurrentVM)            // It's the WINDOWS VM!  Windows App.
   {
      /** Assign address to it via DPMI, only subtract 256 bytes to get the **/
      /** TASK ID information - YES!  Then, read the task structure and get **/
      /** the other pertinent info out of it.                               **/


      dwAddr = wPSPSeg * 0x10L - 0x100; // the address of the TASK DESCRIPTOR
                                        // in LINEAR memory - normally <640k


      // First step:  To retain COMPATIBILITY walk the task list and find
      // out which one of the 'TDB's point to this PSP segment!  It's a 2
      // step process, but I can handle it...

      if(lpTDB = GetTDBFromPSP(wPSPSeg))
      {
         // for now, nothing else is done here.
      }
      else if(MyChangeSelector(wMySel, dwAddr, 0xffff))
      {
         lpTDB = NULL;
      }
      else
      {
         lpTDB = (LPTDB)MAKELP(wMySel, 0);
      }


      /* now, obtain the task and MODULE NAME from the task descriptor */

      if(lpTDB)
      {
         if(lpTDB->bSig[0]!='T' || lpTDB->bSig[1]!='D')
         {
            lpMCB = (LPMCB)MAKELP(wMySel, 256 - sizeof(MCB));

            if(lpMCB->type=='M' || lpMCB->type=='Z')
            {
               if(lpMCB->wOwnerPSP==wPSPSeg)  /* I  *FOUND*  It! */
               {
                  lpRval = "* WINDOWS *";
               }
               else if(HIWORD(GetVersion())>=0x400)
               {
                  wsprintf(buf, pFmt, (LPSTR)lpMCB->dos4);
                  lpRval = buf;
               }
               else
               {
                  lpRval = "* UNKNOWN *";
               }

            }
            else       // not owned by SYSTEM, so...
            {
             GLOBALENTRY ge;
             TASKENTRY te;
             MODULEENTRY me;


               // Walk through ALL of memory, trying to find the section that
               // contains *THIS* PSP.  When I find it, determine which
               // APPLICATION / MODULE (hopefully the *FIRST* one) owns this
               // block, and return the appropriate task handle/module name

               lpRval = "* NOT VALID *"; // initial assigned value (if not found)


               _fmemset((LPSTR)&ge, 0, sizeof(ge));
               _fmemset((LPSTR)&te, 0, sizeof(te));
               _fmemset((LPSTR)&me, 0, sizeof(me));

               ge.dwSize = sizeof(ge);
               te.dwSize = sizeof(te);
               me.dwSize = sizeof(me);

               if(lpGlobalFirst(&ge, GLOBAL_ALL))
               {
                  do
                  {  /* bug in TOOLHELP - free blocks have NULL hBlock */
                     /*                   and NULL hOwner...           */

                     if(ge.wFlags==GT_FREE || (!ge.hBlock && !ge.hOwner))
                     {
                        continue;
                     }


                     // NOTE:  'dwAddr + 0x100' is address of PSP...

                     if(ge.dwAddress <= (dwAddr + 0x100) &&
                        (ge.dwAddress + ge.dwSize) >= (dwAddr + 0x200))
                     {
                        // at this point 'ge' describes the first memory block
                        // I've found that fully contains this PSP.


                        // NOW, let's get info on the task & module that owns it...

                        if(lpTaskFindHandle(&te, (HTASK)ge.hOwner))
                        {
                           *lphTask = te.hTask;

                           if(lpModuleFindHandle &&
                              lpModuleFindHandle(&me, te.hModule))
                           {
                              wsprintf(buf, pFmt, (LPSTR)me.szModule);
                           }
                           else
                           {
                              wsprintf(buf, pFmt, (LPSTR)te.szModule);
                           }

                           lpRval = (LPSTR)buf;
                        }
                        else if(lpModuleFindHandle &&
                                lpModuleFindHandle(&me, (HMODULE)ge.hOwner))
                        {
                           *lphTask = NULL;  // there is *NO* task handle!

                           wsprintf(buf, pFmt, (LPSTR)me.szModule);

                           lpRval = (LPSTR)buf;
                        }


                        break;       // see to it that I break out of the loop!
                     }

                  } while(lpGlobalNext(&ge, GLOBAL_ALL));
               }
            }
         }
         else
         {
            if(lphTask) *lphTask = lpTDB->hSelf;

            wsprintf(buf, pFmt, (LPSTR)lpTDB->sModule);

            lpRval = (LPSTR)buf;
         }
      }
      else
      {
         lpRval = "* DPMI ERR#2 *";
      }

   }
   else if(!wMachID)                       // MS-DOS 'SYSTEM' VM
   {
      lpRval = "* MS-DOS *";               // Assign as such!
   }
   else                                    // NON-WINDOWS APPLICATION
   {
      lpRval = "*NON-WINDOWS*";  // THE 'OLD METHOD' - INITIAL VALUE HERE!

      dwVer = GetVersion();


      if((GetWinFlags() & WF_ENHANCED) &&   // only valid in ENCHANTED mode
          HIWORD(dwVer)>=0x400)      // DOS 4 or greater - MCB has app name!!
      {
         lpWT = GetWOATABLE();

         if(lpWT)
         {
            // OK - now that I have the WOATABLE I need to find the entry
            // which corresponds with the correct VM ID.  This is a somewhat
            // difficult process, but I think I can get it

            w = LOWORD(GetVersion());  // what Windows version?
            _asm mov ax, w
            _asm xchg ah,al
            _asm mov w, ax

//            if(w>=0x350)   // checking for Windows 3.8 or later...
            if(IsChicago)
            {
               lpWT4 = (WOATABLE4 FAR *)lpWT;

               w2 = (WORD)GlobalSizePtr(lpWT4);  // how big is it, anyway?

               w2 = (w2 - sizeof(*lpWT4) + sizeof(lpWT4->EntryTable[0]))
                     / sizeof(lpWT4->EntryTable[0]);

               if(w2 > 64) w2 = 64; // I assume maximum of 64 DOS APP's!
                                    // Based on experimentation with Win 3.1
                                    // WINOLDAP data segment size & rough calcs

               lpCB = (LPCONTROL_BLOCK)MAKELP(wMySel, 0);

               for(dwAddr=0, w=0; w<w2; w++)
               {
                  if(lpWT4->EntryTable[w].hWnd       &&
                     lpWT4->EntryTable[w].hInst      &&
                     lpWT4->EntryTable[w].dwVMHandle &&
                     lpWT4->EntryTable[w].wFlags )
                  {
                     MyChangeSelector(wMySel, lpWT4->EntryTable[w].dwVMHandle,
                                      0xffff);

                     if(LOWORD(lpCB->CB_VMID)==wMachID)
                     {
                        dwAddr = lpCB->CB_High_Linear + (wPSPSeg - 1) * 0x10L;
                                                     // actual address to view!!

                        if(lphTask) *lphTask = (HTASK)
                                 GetInstanceTask(lpWT4->EntryTable[w].hInst);

                        break;
                     }
                  }
               }
            }
            else if(w>=0x30a)   // checking for Windows 3.1 or later...
            {
               w2 = (WORD)GlobalSizePtr(lpWT);  // how big is it, anyway?

               w2 = (w2 - sizeof(*lpWT) + sizeof(lpWT->EntryTable[0]))
                     / sizeof(lpWT->EntryTable[0]);

               if(w2 > 64) w2 = 64; // I assume maximum of 64 DOS APP's!
                                    // Based on experimentation with Win 3.1
                                    // WINOLDAP data segment size & rough calcs

               lpCB = (LPCONTROL_BLOCK)MAKELP(wMySel, 0);

               for(dwAddr=0, w=0; w<w2; w++)
               {
                  if(lpWT->EntryTable[w].hWnd       &&
                     lpWT->EntryTable[w].hInst      &&
                     lpWT->EntryTable[w].dwVMHandle &&
                     lpWT->EntryTable[w].hTask )
                  {
                     MyChangeSelector(wMySel, lpWT->EntryTable[w].dwVMHandle,
                                      0xffff);

                     if(LOWORD(lpCB->CB_VMID)==wMachID)
                     {
                        dwAddr = lpCB->CB_High_Linear + (wPSPSeg - 1) * 0x10L;
                                                     // actual address to view!!

                        if(lphTask) *lphTask = lpWT->EntryTable[w].hTask;

                        break;
                     }
                  }
               }
            }
            else
            {
               lpWT30 = (LPWOATABLE30)lpWT;

               w2 = (WORD)GlobalSizePtr(lpWT30);  // how big is it, anyway?

               w2 = (w2 - sizeof(*lpWT30) + sizeof(lpWT30->EntryTable[0]))
                     / sizeof(lpWT30->EntryTable[0]);

               if(w2 > 64) w2 = 64; // I assume maximum of 64 DOS APP's!
                                    // Based on experimentation with Win 3.1
                                    // WINOLDAP data segment size & rough calcs

               lpCB = (LPCONTROL_BLOCK)MAKELP(wMySel, 0);

               for(dwAddr=0, w=0; w<w2; w++)
               {
                  if(lpWT30->EntryTable[w].hWnd       &&
                     lpWT30->EntryTable[w].hInst      &&
                     lpWT30->EntryTable[w].dwVMHandle &&
                     lpWT30->EntryTable[w].hTask )
                  {
                     MyChangeSelector(wMySel, lpWT30->EntryTable[w].dwVMHandle,
                                      0xffff);

                     if(LOWORD(lpCB->CB_VMID)==wMachID)
                     {
                        dwAddr = lpCB->CB_High_Linear + (wPSPSeg - 1) * 0x10L;
                                                     // actual address to view!!

                        if(lphTask) *lphTask = lpWT30->EntryTable[w].hTask;

                        break;
                     }
                  }
               }
            }

            if(dwAddr && !MyChangeSelector(wMySel, dwAddr, 0xffff))
            {
               lpMCB = (LPMCB)MAKELP(wMySel, 0); // address of the MCB!!

               if(lpMCB->wOwnerPSP != wPSPSeg)
               {
                  wsprintf(buf, "* NOT PSP *");
               }
               else
               {
                  _fmemset(buf, 0, sizeof(buf));
                  _fmemcpy(buf, lpMCB->dos4, sizeof(lpMCB->dos4));
               }

               lpRval = buf;                     // that's all, folks!
            }

         }

      }
   }


   MyFreeSelector(wMySel);

   return(lpRval);
}





//   ** MODULE DATABASE STRUCTURE (and 'NEW EXE HEADER' structure) **


// TAKEN FROM NEWEXE.INC in DDK

typedef struct tagNEW_EXE {

   WORD  ne_magic;         // Magic value 'NE'
   BYTE  ne_ver;           // version number
   BYTE  ne_rev;           // revision number
   WORD  ne_enttab;        // offset to entry table
   WORD  ne_cbenttab;      // number of bytes in entry table

   DWORD ne_crc;           // CRC of file

   WORD  ne_flags;         // flag word
   WORD  ne_autodata;      // segment number of auto data segment
   WORD  ne_heap;          // initial size of local heap
   WORD  ne_stack;         // initial size of stack

   DWORD ne_csip;          // CS:IP start address
   DWORD ne_sssp;          // SS:SP initial stack pointer.  0 if
                           // stack size word non-zero

   WORD  ne_cseg;          // number of segment in segment table
   WORD  ne_cmod;          // number of entries in module reference table
   WORD  ne_cbnrestab;     // number of bytes in non-resident name table

   WORD  ne_segtab;        // NE relative offset to segment table
   WORD  ne_rsrctab;       // NE relative offset to resource table
   WORD  ne_restab;        // NE relative offset to resident name table
   WORD  ne_modtab;        // NE relative offset to module reference table
   WORD  ne_imptab;        // NE relative offset to imported name table
   DWORD ne_nrestab;       // file offset to non-resident name table
   WORD  ne_cmovent;       // Count of movable entries
   WORD  ne_align;         // Alignment shift count for segment data
   WORD  ne_cres;          // Count of resource segments
   BYTE  ne_exetyp;        // Target operating system
   BYTE  ne_flagsothers;   // Other .EXE flags
   WORD  ne_pretthunks;    // offset to return thunks
   WORD  ne_psegrefbytes;  // offset to segment ref. bytes
   WORD  ne_swaparea;      // Minimum code swap area size
   WORD  ne_expver;        // Expected Windows version number
   } NEW_EXE;

typedef NEW_EXE FAR *LPNEW_EXE;


// 'GANG LOAD' AREA ('NE' header section in module database)

typedef struct tagNEW_EXE1 {

   WORD  ne_magic;         // Magic value 'NE'
   WORD  ne_usage;         // usage (reference) count (?)
   WORD  ne_enttab;        // offset to entry table
   WORD  ne_pnextexe;      // selector of NEXT MODULE in chain (?)
   WORD  ne_pautodata;     // automatic data segment (?) (not selector)
   WORD  ne_pfileinfo;     // file info (?) (not selector)

   // REMAINING FIELDS IDENTICAL TO 'NEW_EXE' (above); TAKEN FROM EXE FILE

   WORD  ne_flags;         // flag word
   WORD  ne_autodata;      // segment number of auto data segment

   // 10H

   WORD  ne_heap;          // initial size of local heap
   WORD  ne_stack;         // initial size of stack

   DWORD ne_csip;          // CS:IP start address
   DWORD ne_sssp;          // SS:SP initial stack pointer.  0 if
                           // stack size word non-zero

   WORD  ne_cseg;          // number of segment in segment table
   WORD  ne_cmod;          // number of entries in module reference table

   // 20H

   WORD  ne_cbnrestab;     // number of bytes in non-resident name table

   WORD  ne_segtab;        // NE relative offset to segment table
   WORD  ne_rsrctab;       // NE relative offset to resource table
   WORD  ne_restab;        // NE relative offset to resident name table
   WORD  ne_modtab;        // NE relative offset to module reference table
   WORD  ne_imptab;        // NE relative offset to imported name table
   DWORD ne_nrestab;       // file offset to non-resident name table

   // 30H

   WORD  ne_cmovent;       // Count of movable entries
   WORD  ne_align;         // Alignment shift count for segment data
   WORD  ne_cres;          // Count of resource segments
   BYTE  ne_exetyp;        // Target operating system
   BYTE  ne_flagsothers;   // Other .EXE flags
   WORD  ne_pretthunks;    // offset to return thunks
   WORD  ne_psegrefbytes;  // offset to segment ref. bytes
   WORD  ne_swaparea;      // Minimum code swap area size
   WORD  ne_expver;        // Expected Windows version number

   // 40H

   } MDB;


typedef MDB FAR *LPMDB;    // MODULE DATABASE!!



// FOLLOWING THE 'MDB' STRUCTURE IN THE MODULE DATABASE ARE THE
// VARIOUS TABLES THAT DESCRIBE WHERE SEGMENTS & RESOURCES ARE...

typedef struct tagNEW_SEG {
   WORD ns_sector;      // logical sector number in file of start of segment
   WORD ns_cbseg;       // number bytes in file
   WORD ns_flags;       // segment flags
   WORD ns_minalloc;    // minimum number bytes to allocate for segment

   } NEW_SEG;



// The following structure shows the way the segment information is
// stored within the MDB structure, which differs slightly from the file!!

typedef struct tagNEW_SEG1 {
   NEW_SEG newseg;
   WORD ns_handle;         // HANDLE TO SEGMENT (0 if not loaded)
   } NEW_SEG1;


typedef NEW_SEG1 FAR *LPNEW_SEG1;



// SEGMENT FLAGS - 'ns_flags' element in 'NEW_SEG' (above)

#define NSTYPE     0x0007      /* Segment type mask */
#define NSCODE     0x0000      /* Code segment */
#define NSDATA     0x0001      /* Data segment */
#define NSITER     0x0008      /* Iterated segment data */
#define NSMOVE     0x0010      /* Moveable segment */
#define NSSHARE    0x0020      /* Shareable segment */
#define NSPRELOAD  0x0040      /* Preload this segment */
#define NSERONLY   0x0080      /* EXECUTE ONLY code/READ ONLY data segment */
#define NSRELOC    0x0100      /* Relocation information following segment data */
#define NSDPL      0x0C00      /* 286 DPL bits */
#define NSDISCARD  0x1000      /* Discard priority bits */
#define NS286DOS   0xEE06      /* These bits only used by 286DOS */

#define NSALIGN    0x9         /* Default alignment shift count for seg. data */

#define NSALLOCED  0x0002      /* set if ns_handle points to uninitialized mem. */
#define NSLOADED   0x0004      /* set if ns_handle points to initialized mem. */
#define NSUSESDATA 0x0400      /* set if an entry point in this segment uses */
                               /* the automatic data segment of a SOLO library */

#define NSGETHIGH  0x0200
#define NSINDIRECT 0x2000
#define NSWINCODE  0x4000      /* flag for code */

#define NSKCACHED  0x0800      /* cached by kernel */
#define NSPRIVLIB  NSITER
#define NSNOTP     0x8000

#define NSINROM    NSINDIRECT  /* segment is loaded in ROM */
#define NSCOMPR    NSGETHIGH   /* segment is compressed in ROM */



LPWOATABLE FAR PASCAL GetWOATABLE()
{
HMODULE hModule;
LPMDB lpMDB;
LPNEW_SEG1 lpS;
LPWOATABLE lpRval;
WORD w, w2;


   hModule = GetModuleHandle("WINOLDAP");

   if(!hModule) return(NULL);    // WINOLDAP IS NOT LOADED - NO SUCH LUCK!


   lpMDB = (LPMDB)MAKELP(HIWORD(GlobalHandle((WORD)hModule)),0);

   lpS = (LPNEW_SEG1)((LPSTR)(lpMDB) + lpMDB->ne_segtab);

   w2 = (lpMDB->ne_restab - lpMDB->ne_segtab) / sizeof(*lpS);


   for(w=1; w<=w2; w++)
   {
      if(w==lpMDB->ne_autodata)  continue;  // DON'T USE THIS ONE!!

      if((lpS[w - 1].newseg.ns_flags & NSDATA) == NSDATA)
      {
         lpRval = (LPWOATABLE)
                  MAKELP(HIWORD(GlobalHandle(lpS[w - 1].ns_handle)),0);

         return(lpRval);
      }
   }

   return(NULL);       // NOT FOUND!!

}


#endif // WIN32





#if 0

LPSTR FAR PASCAL OldGetTaskAndModuleFromPSP(WORD wMachID, WORD wPSPSeg,
                                            HTASK FAR *lphTask)
{
WORD wMySel;
DWORD dwAddr;
LPSTR lpRval;
LPTDB lpTDB;
LPMCB lpMCB;
static char buf[32], pFmt[]="%-8.8s";
static WORD CurrentVM = 0, CurrentVMFlag=0;


   if(!CurrentVMFlag)
   {
      _asm mov ax, 0x1683    /* Windows(tm) INT 2FH interface to get VM ID */
      _asm int 0x2f
      _asm mov CurrentVM, bx

      CurrentVMFlag = TRUE;
   }

   *lphTask = 0;

   if(wMachID!=CurrentVM)
   {
      // FOR NOW, LEAVE A PLACE WHERE I CAN GO AFTER THE APPLICATION NAME!!



      return("*NON-WINDOWS*");
   }


   wMySel = MyAllocSelector();

   if(!wMySel) return("* DPMI ERROR *");


   /** Assign address to it via DPMI, only subtract 256 bytes to get the **/
   /** TASK ID information - YES!  Then, read the task structure and get **/
   /** the other pertinent info out of it.                               **/


   dwAddr = wPSPSeg * 16L - 0x100;  /* the address of the TASK DESCRIPTOR */



   /* now, obtain the task and MODULE NAME from the task descriptor */

   if(!MyChangeSelector(wMySel, dwAddr, 0xffff))
   {
      lpTDB = (LPTDB)MAKELP(wMySel, 0);

      if(lpTDB->bSig[0]!='T' || lpTDB->bSig[1]!='D')
      {
         lpMCB = (LPMCB)MAKELP(wMySel, 0x100 - sizeof(MCB));

         if(lpMCB->type=='M' || lpMCB->type=='Z')
         {
            if(lpMCB->wOwnerPSP==wPSPSeg)  /* I  *FOUND*  It! */
            {
               lpRval = "* SYSTEM *";
            }
            else if(HIWORD(GetVersion())>=0x400)
            {
               wsprintf(buf, "* %-8.8s *", (LPSTR)lpMCB->dos4);
               lpRval = buf;
            }
            else
            {
               lpRval = "* UNKNOWN *";
            }

         }
         else       // not owned by SYSTEM, so...
         {
          GLOBALENTRY ge;
          TASKENTRY te;
          MODULEENTRY me;


            // Walk through ALL of memory, trying to find the section that
            // contains *THIS* PSP.  When I find it, determine which
            // APPLICATION / MODULE (hopefully the *FIRST* one) owns this
            // block, and return the appropriate task handle/module name

            lpRval = "* NOT VALID *"; // initial assigned value (if not found)


            _hmemset((LPSTR)&ge, 0, sizeof(ge));
            _hmemset((LPSTR)&te, 0, sizeof(te));
            _hmemset((LPSTR)&me, 0, sizeof(me));

            ge.dwSize = sizeof(ge);
            te.dwSize = sizeof(te);
            me.dwSize = sizeof(me);


            if(lpGlobalFirst && lpGlobalFirst(&ge, GLOBAL_ALL))
            {
               do
               {  /* bug in TOOLHELP - free blocks have NULL hBlock */
                  /*                   and NULL hOwner...           */

                  if(ge.wFlags==GT_FREE || (!ge.hBlock && !ge.hOwner))
                  {
                     continue;
                  }


                  // NOTE:  'dwAddr + 0x100' is address of PSP...

                  if(ge.dwAddress <= (dwAddr + 0x100) &&
                     (ge.dwAddress + ge.dwSize) >= (dwAddr + 0x200))
                  {
                     // at this point 'ge' describes the first memory block
                     // I've found that fully contains this PSP.


                     // NOW, let's get info on the task & module that owns it...

                     if(lpTaskFindHandle &&
                        lpTaskFindHandle(&te, (HTASK)ge.hOwner))
                     {
                        *lphTask = te.hTask;

                        if(lpModuleFindHandle &&
                           lpModuleFindHandle(&me, te.hModule))
                        {
                           wsprintf(buf, pFmt, (LPSTR)me.szModule);
                        }
                        else
                        {
                           wsprintf(buf, pFmt, (LPSTR)te.szModule);
                        }

                        lpRval = (LPSTR)buf;
                     }
                     else if(lpModuleFindHandle &&
                             lpModuleFindHandle(&me, (HMODULE)ge.hOwner))
                     {
                        *lphTask = NULL;  // there is *NO* task handle!

                        wsprintf(buf, pFmt, (LPSTR)me.szModule);

                        lpRval = (LPSTR)buf;
                     }


                     break;       // see to it that I break out of the loop!
                  }

               } while(lpGlobalNext && lpGlobalNext(&ge, GLOBAL_ALL));
            }
         }
      }
      else
      {
         *lphTask = lpTDB->hSelf;

         wsprintf(buf, pFmt, (LPSTR)lpTDB->sModule);

         lpRval = (LPSTR)buf;
      }
   }
   else
   {
      lpRval = "* DPMI ERR#2 *";
   }


   MyFreeSelector(wMySel);


   return(lpRval);
}

#endif // 0


#pragma pack()

#pragma code_seg()
