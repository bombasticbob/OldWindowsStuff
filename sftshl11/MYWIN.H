/***************************************************************************/
/*                                                                         */
/* MYWIN.H - Special WINDOWS.H file that includes the correct header files */
/*           based on DEFINE statements found at the beginning of programs */
/*           which include this file in place of 'WINDOWS.H'.              */
/*                                                                         */
/*                      TRADE SECRET - CONFIDENTIAL                        */
/*                                                                         */
/*             The compiled libraries and API header files are             */
/*          (c) Copyright 1990-1995 by Stewart~Frazier Tools, Inc          */
/*                                                                         */
/***************************************************************************/

#pragma message("** MYWIN.H ** (c) 1991 R.E. Frazier - all rights reserved")

#define WINVER 0x400    /* windows version 4.0 and later!! */
#define WIN40 1

#ifdef WIN32
 #define EXPORT
 #define FASTCALL
 #define LOADDS
#else  // WIN32
 #define EXPORT __export
 #define FASTCALL __fastcall
 #define LOADDS __loadds
#endif // WIN32



           /* if WIN31 or STRICT is defined, use 'winstric.h' */

#ifdef THE_WORKS

 #pragma message("           ** THE WORKS ** has been defined")

 #define WIN31               /* 3.1 API functions */
 #define CTLAPIS             /* API macros for CONTROL windows */
 #define STRICT              /* STRICT type checking on handles, etc. */
 #define OEMRESOURCE         /* OEM Resource definitions */
 #define USECOMM             /* COMM driver definitions */

 #define COMMDLG             /* COMMON DIALOG API */
 #define TOOLHELP            /* TOOLHELP API */
 #define DDE                 /* Standard DDE messages */
 #define DDEML               /* DDEML API */
 #define OLE                 /* OLE API */
 #define SHELL               /* SHELL API */
 #define VER                 /* VERSION CHECKING API */

 #define MMSYSTEM            /* MULTI-MEDIA DEFINITIONS */

 #ifdef NO_CLIBS       /* if this is defined, undefine it for 'THE_WORKS' */
   #undef NO_CLIBS
 #endif

#endif


#if defined(WIN31) || defined(STRICT) || defined(CTLAPIS)

 #include "windows.h"
 #include "windowsx.h"
 #include "winerror.h"
 #include "on_wm.h"

/* #ifndef ON_WM_DROPFILES
/* #define ON_WM_DROPFILES(fn, hwnd, wParam, lParam) \
/*       case WM_DROPFILES: (fn)((hwnd), (HDROP)(wParam)); return 0L
/* #endif
/* #ifndef FORWARD_WM_DROPFILES
/*   #define FORWARD_WM_DROPFILES(fn, hwnd, hDrop) \
/*       (VOID)(fn)((hwnd), WM_DROPFILES, (WPARAM)(WORD)(hDrop), 0L)
/*
/* #endif
*/

#else

 #include "windows.h"
 #include "windowsx.h"      /* I include this anyway */

#endif

#ifdef WIN32
#include "shlthunk.h"
#endif // WIN32

#ifdef COMMDLG

 #include "commdlg.h"
 #include "cderr.h"

#endif


#ifdef TOOLHELP
#ifdef WIN32
 #include "tlhelp32.h"
#else
 #include "toolhelp.h"
#endif // WIN32
#endif // TOOLHELP


#ifdef DDE

 #include "DDE.H"

#endif


#ifdef DDEML

 #ifndef DDE
  #include "DDE.H"
 #endif

#include "DDEML.H"

#endif


#ifndef WIN32
#ifdef OLE

 #include "ole.h"

#endif
#endif // WIN32


#ifdef SHELL

 #include "shellapi.h"

#endif


#ifdef VER
 #ifdef WIN32
  #include "winver.h"
 #else  // WIN32
  #include "ver.h"
 #endif // WIN32
#endif // VER

#ifdef MMSYSTEM

 #include "mmsystem.h"

#endif


#ifndef NO_CLIBS

#include "stdio.h"
#include "stdlib.h"

#include "ctype.h"
#include "dos.h"
#include "direct.h"
#include "errno.h"
#include "fcntl.h"
#include "math.h"
#include "memory.h"
#include "signal.h"
#include "string.h"
#include "sys\types.h"
#include "sys\stat.h"


#undef toupper
#undef tolower


static int __inline __cdecl toupper(int _c)
{
   return( (islower(_c)) ? _toupper(_c) : (_c) );
}
static int __inline __cdecl tolower(int _c)
{
   return( (isupper(_c)) ? _tolower(_c) : (_c) );
}


#endif


// OTHER common definitions

#ifdef WIN32
#define STACK_BASED
#define CODE_BASED
#else  // WIN32
#define STACK_BASED __based(__segname("_STACK"))
#define CODE_BASED __based(__segname("_CODE"))
#endif // WIN32

#ifdef WIN32
#define MoveTo(H,X,Y) MoveToEx(H,X,Y,NULL)
#endif // WIN32
