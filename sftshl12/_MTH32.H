/***************************************************************************/
/*                                                                         */
/*         Win32 Multi-Thread Manager Utility Library - MTH32.LIB          */
/*                                                                         */
/*          '_MTH32.H' - Multi-Thread Utilities and API FUNCTIONS          */
/*                                                                         */
/*                      TRADE SECRET - CONFIDENTIAL                        */
/*                                                                         */
/*             The compiled libraries and API header files are             */
/*          (c) Copyright 1990-1994 by Stewart~Frazier Tools, Inc          */
/*                          all rights reserved                            */
/*                                                                         */
/*                                                                         */
/***************************************************************************/

#define STRICT 1       /* enable strict type checking!! */
#pragma warning(disable: 4001)

#include "windows.h"
#include "windowsx.h"
#include "stdio.h"
#include "stdlib.h"
#include "errno.h"
#include "string.h"
#include "memory.h"
#include "ctype.h"      // macro versions of 'toupper','tolower', etc.

#undef toupper       // later on I'll re-define these as INLINE functions!
#undef tolower


             /** BY DEFAULT, CERTAIN WARNINGS ARE ERRORS! **/

#pragma warning (error:4013)  // 'proc' undefined; assuming extern returning int
#pragma warning (error:4017)  // cast of int expression to far pointer
#pragma warning (error:4020)  // 'function': too many actual parameters
#pragma warning (error:4021)  // 'function': too few actual parameters
#pragma warning (error:4026)  // function declared with formal parameter list
#pragma warning (error:4027)  // function declared without formal parameter list
#pragma warning (error:4029)  // declared formal parameter list different from definition
#pragma warning (error:4034)  // sizeof returns 0
#pragma warning (error:4071)  // no function prototype given
#pragma warning (error:4072)  // no function prototype on __fastcall function
#pragma warning (error:4087)  // 'function': declared with void parameter list
#pragma warning (error:4137)  // 'function': no return value from floating-point function



      /** the following warnings are being promoted to 'Level 1' **/

#pragma warning (1:4100 4101) // unreferenced formal parameter/local variable


                /** SPECIAL DEFINITIONS FOR 'PROC' TYPES **/
                /** and DATA SEGMENTS for WIN32 & WIN16  **/

#define EXPORT
#define LOADDS
#define CODE_BASED

                        /** API HEADER FILES **/


#define NOSFT32LIB 1
#define NOWDB32LIB 1

#include "mth.h"


                      /****************************/
                      /*** CONSTANT DEFINITIONS ***/
		                /****************************/


#define MAX_ACCEL        64  /* maximum of 64 accelerators in message loop */
#define MAX_MDIACCEL     64  /* maximum of 64 MDI accelerators in msg loop */

#define MAX_SEMAPHORES 256         /* maximum of 256 semaphores! */
#define MAX_SEMAPHORECALLBACKS 256 /* maximum of 256 semaphore callbacks */


#define MIN_HASHTABLESIZE 128   /* minimum number of hashtable entries! */


#define DEL_FILE_RECSIZE  256           /* record size for 'DEL' file */
#define DEL_FILE_KEYNUM   ((UINT)-1)    /* 'key number' for 'DEL' file */

#define NO_PEEKMSG

#define STACK_BASED
#define ALT_BASED


#define INIT_LEVEL do{            /* these are used to 'clean up' commonly */
#define END_LEVEL }while(FALSE);  /* used 'hierarchichal' error traps */

#define HFILE_CLOSED ((HFILE)~0)

#ifndef GlobalSizePtr
#define GlobalSizePtr(X) (GlobalSize(GlobalPtrHandle((X))))
#endif /* GlobalSizePtr */


// from _dbutilf.h

#define lpGetRasterizerCaps(X,Y) GetRasterizerCaps(X,Y)

#define DEBUG_MESSAGE(X) { /* the 'X' parm *must* be a string literal! */   \
 static const char CODE_BASED __szDebugMsg[]= X ;          \
 static const char CODE_BASED __szDebugFmt[]=              \
                                                "!" __FILE__ ".%d %s\r\n";  \
 extern char debug_buf[];                                                   \
 wsprintf(debug_buf, __szDebugFmt, (UINT)__LINE__, (LPSTR)__szDebugMsg);\
 MyOutputDebugString(debug_buf); }


#define SwitchPSP()      ((WORD)0)
#define SwitchPSPBack(X)

#define SwitchPSPLoadDS()      ((WORD)0)
#define SwitchPSPBackLoadDS(X)


#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

static __inline int __cdecl toupper(int _c)
{
   if(islower(_c))
   {
      return(_toupper(_c));
   }
   else
   {
      return(_c);
   }
}

static __inline int __cdecl tolower(int _c)
{
   if(isupper(_c))
   {
      return(_tolower(_c));
   }
   else
   {
      return(_c);
   }
}

#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif  /* __cplusplus */



// MACROS to fix 'huge' stuff

#define _hmemcpy(X,Y,Z) memcpy(X,Y,Z)
#define _hmemcmp(X,Y,Z) memcmp(X,Y,Z)
#define _hmemset(X,Y,Z) memset(X,Y,Z)
#define _hatol(X) atol(X)


