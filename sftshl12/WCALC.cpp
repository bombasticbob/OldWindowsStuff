/***************************************************************************/
/*                                                                         */
/*    WCALC.C - Command line interpreter for Microsoft(r) Windows (tm)     */
/*                                                                         */
/*                      TRADE SECRET - CONFIDENTIAL                        */
/*                                                                         */
/*             The compiled libraries and API header files are             */
/*          (c) Copyright 1990-1995 by Stewart~Frazier Tools, Inc          */
/*                                                                         */
/*        This contains the code required for the 'CALC' function          */
/*           and all other 'related' functions used elsewhere              */
/*                                                                         */
/***************************************************************************/


#define THE_WORKS
#include "mywin.h"

#include "share.h"
#include "mth.h"
#include "wincmd.h"         /* 'dependency' related definitions */
#include "wincmd_f.h"       /* function prototypes and variables */

#include "wcalc_f.h"        // static variables associated with functions
                            // stored here to prevent cluttering this file


extern HFONT hMainFont;  // defined in wincmd.c

static const char CODE_BASED szCALC_DIAG[]="CALC_DIAG";
extern const char szRETVAL[];


#pragma inline_recursion(off)


#if defined(M_I86LM)

  #pragma message("** Large Model In Use - Single Instance Only!! **")

#elif !defined(M_I86MM)

  #error  "** Must use MEDIUM model to compile, silly!! **"

#else

  #pragma message("** Medium Model Compile - Multiple Instances OK **")

#endif




              /** 'INLINE' STRING MANAGEMENT FUNCTIONS **/



__inline char FAR *NextNonBlank(char FAR *p1)
{
   if(!p1) return(NULL);  /* in case it's NULL */

   while(*p1 && *p1<=' ') p1++;

   if(*p1) return(p1);
   else    return(NULL);
}

__inline char FAR *NextBlank(char FAR *p1)
{
   if(!p1) return(NULL);

   while(*p1>' ') p1++;

   return(p1);  /* returns ptr to SPACE or end of string */
}

__inline char FAR *GetNextWord(char FAR *p1, char FAR *pWord)
{
char FAR *p2, FAR *p3, FAR *pDest;

   *pWord = 0;

   p1 = NextNonBlank(p1);
   if(!p1) return(NULL);

   p2 = NextBlank(p1);

   for(pDest = pWord, p3 = p1; p3<p2; p3++)
   {
      *(pDest++) = *p3;
   }

   *pDest = 0;        /* mark end of the word with NULL byte */

   return(p2);        /* returns pointer to next 'white-space' area */
}


__inline char FAR *strltrim(char FAR *p)
{
char FAR *p2;

   /** LTRIM **/

   p2 = p;

   while(*p2 && *p2<=' ') p2++;

   if(p2>p) _fstrcpy(p,p2);


   return(p);

}



__inline char FAR *strrtrim(char FAR *p)
{
char FAR *p2;

   /** RTRIM **/

   p2 = p + _fstrlen(p);

   while(p2>p && *(p2 - 1) && *(p2 - 1)<=' ')
   {
      *(--p2) = 0;
   }


   return(p);

}



__inline char FAR *strtrim2(char FAR *p)      // (strltrim(strrtrim(p)))
{
char FAR *p2;

   /** RTRIM **/

   p2 = p + _fstrlen(p);

   while(p2>p && *(p2 - 1) && *(p2 - 1)<=' ')
   {
      *(--p2) = 0;
   }

   /** LTRIM **/

   p2 = p;

   while(*p2 && *p2<=' ') p2++;

   if(p2>p) _fstrcpy(p,p2);


   return(p);

}



__inline char FAR * PASCAL strqchr(char FAR *p, char c)
{
char FAR *p2;
register char c0;
char qflag;


   p2 = p;

   while(c0 = *p2)
   {
      if(c!=c0 && (c0=='"' || c0=='\''))  // Quoted strings processed here
      {
         qflag = c0;
         p2++;

         while(c0 = *p2)
         {
            if(c0==qflag)
            {
               break;
            }
            else if(c0=='\\')
            {
               c0 = *(++p2);

               if(c0=='"' || c0=='\'' || c0=='\\')
               {
                  p2++;
               }
            }
            else
            {
               p2++;
            }
         }
      }
      else if(c0==c)
      {
         return(p2);
      }

      p2++;
   }

   return(NULL);  /* NOT FOUND! */
}


__inline char FAR * PASCAL strqpbrk(char FAR *p, const char FAR *pc)
{
char FAR *p2;
const char FAR *pc2;
register char c0;
char qflag, wflag;



   // assume that quote marks not preceded by white space, or quote marks
   // embedded within a string (with or without white space) should be
   // treated just like any other character


   p2 = p;
   wflag = 0;  // only white space so far

   while(c0 = *p2)
   {
      for(pc2=pc; *pc2; pc2++)
      {
         if(*pc2 == c0) return(p2);   // FOUND!
      }

      if(!wflag && (c0=='"' || c0=='\''))  // Quoted strings processed here
      {                                    // (must be first non-white-space)
         wflag = 1;

         qflag = c0;
         p2++;

         while(c0 = *p2)
         {
            if(c0==qflag)
            {
               break;
            }
            else if(c0=='\\')
            {
               c0 = *(++p2);

               if(c0=='"' || c0=='\'' || c0=='\\')
               {
                  p2++;
               }
            }
            else
            {
               p2++;
            }
         }
      }
      else if(!wflag && c0 > ' ')
      {
         wflag = 1;  // found a non-white-space character!!
      }

      p2++;
   }

   return(NULL);  /* NOT FOUND! */
}


__inline char FAR *strqistr(char FAR *p, char FAR *p0)
{
char FAR *p2;
char qflag;
int i, w, l;


   l = _fstrlen(p0);
   w = _fstrlen(p);

   for(p2=p, i=0; *p2 && i<=(w - l); p2++)
   {
      // for this one, I assume that 'p0' is NOT a quoted string!!

      if(*p2=='"' || *p2=='\'')           // Quoted strings processed here
      {
         qflag = *p2;
         p2++;

         while(*p2)
         {
            if(*p2==qflag)
            {
               break;
            }
            else if(*p2=='\\')
            {
               p2++;
               if(*p2=='"' || *p2=='\'' || *p2=='\\')
               {
                  p2++;
               }
            }
            else
            {
               p2++;
            }
         }
      }
      else if(!_fstrnicmp(p2, p0, l))
      {
         return(p2);
      }
   }

   return(NULL);  /* NOT FOUND! */
}






/***************************************************************************/
/*                  STRUCTURES, 'TYPEDEF's AND CONSTANTS                   */
/***************************************************************************/


typedef struct tagMYFILE {
   HFILE hFile;
   OFSTRUCT ofstr;
   char buffer[4096];
   DWORD dwFilePos;
   WORD wBufPos, wBufLen;
   WORD wLastErr;
   BOOL dirty;
   } MYFILE;




#define OP_NULL         0        /* 'NULL' operator */

#define OP_LEFTPAREN    1        /* argument separators */
#define OP_RIGHTPAREN   2
#define OP_COMMA        3
#define OP_LEFTBRACKET  4
#define OP_RIGHTBRACKET 5

#define OP_ADD          0x10     /* arithmetic operators */
#define OP_SUBTRACT     0x11
#define OP_MULTIPLY     0x12
#define OP_DIVIDE       0x13
#define OP_POWER        0x14
#define OP_MOD          0x15

#define OP_CONCAT       0x18     /* string operator (concatenate) */

#define OP_EQUAL        0x20     /* relational operators */
#define OP_LESSTHAN     0x21
#define OP_GREATERTHAN  0x22
#define OP_GREATEREQUAL 0x23
#define OP_LESSEQUAL    0x24
#define OP_NOTEQUAL     0x25

#define OP_STREQUAL     0x28
#define OP_SLESSTHAN    0x29
#define OP_SGREATERTHAN 0x2a
#define OP_SGREATEREQUAL 0x2b
#define OP_SLESSEQUAL   0x2c
#define OP_STRNOTEQUAL  0x2d

#define OP_AND          0x30     /* logical operators */
#define OP_OR           0x31
#define OP_NOT          0x32

#define OP_BITAND       0x40     /* bit-wise logical operators */
#define OP_BITOR        0x41
#define OP_BITNOT       0x42
#define OP_BITXOR       0x43


#define OP_NEGATE       0x80     /* unary operators (immediate) */

#define OP_ERROR        0xff     /* this operator flags an error! */



#define SBPWORD WORD STACK_BASED *




                         /** GLOBAL VARIABLES **/


#define MAX_OP_LEN 7  /* nothing over 7 chars (total, not incl. NULL byte) */

char *pOperators[]={"(", ")", ",",
                    "[", "]",
                    "++",                       // special - concatenation
                    "+", "-", "*", "/", "^",
                    " MOD ", " XOR ",
                    ">>==","<<==","<<>>","!==", // special - strings
                    "==","<<",">>",
                    ">=", "<=", "<>", "!=",     // regular numeric relationals
                    "=", "<", ">",
                    " AND ", " OR ", " NOT ",
                    "&&", "||", "!",
                    "&", "|", "~" };

int pOperatorLength[]={1,1,1,
                       1,1,
                       2,
                       1,1,1,1,1,
                       5,5,
                       4,4,4,3,
                       2,2,2,
                       2,2,2,2,
                       1,1,1,
                       5,4,5,
                       2,2,1,
                       1,1,1 };

WORD wNOperators = sizeof(pOperators) / sizeof(*pOperators);


WORD wOperators[]={OP_LEFTPAREN,OP_RIGHTPAREN,OP_COMMA,
                   OP_LEFTBRACKET,OP_RIGHTBRACKET,
                   OP_CONCAT,
                   OP_ADD,OP_SUBTRACT,OP_MULTIPLY,OP_DIVIDE,OP_POWER,
                   OP_MOD,OP_BITXOR,
                   OP_SGREATEREQUAL,OP_SLESSEQUAL,OP_STRNOTEQUAL,OP_STRNOTEQUAL,
                   OP_STREQUAL,OP_SLESSTHAN,OP_SGREATERTHAN,
                   OP_GREATEREQUAL,OP_LESSEQUAL,OP_NOTEQUAL,OP_NOTEQUAL,
                   OP_EQUAL,OP_LESSTHAN,OP_GREATERTHAN,
                   OP_AND, OP_OR, OP_NOT,
                   OP_AND, OP_OR, OP_NOT,
                   OP_BITAND, OP_BITOR, OP_BITNOT };





                        /** FUNCTION PROTOTYPES **/

LPSTR FAR PASCAL seg_parse(LPSTR line, SBPWORD ptr, SBPWORD op);


LPSTR FAR PASCAL CalcEvaluateFunction(LPSTR function, LPSTR FAR *lplpArgs);
LPSTR FAR PASCAL CalcEvaluateArray(LPSTR array, LPSTR FAR *lplpIndex);
LPSTR FAR PASCAL CalcEvaluateOperator(LPSTR arg1, LPSTR arg2, WORD op);

BOOL EXPORT CALLBACK CalcGetWindowEnum(HWND hWnd, LPARAM lParam);
BOOL EXPORT CALLBACK CalcFindWindowEnum(HWND hWnd, LPARAM lParam);

void FAR PASCAL CalcGetWindowInfo(HWND hWnd);

HMODULE FAR PASCAL CalcGetModuleHandle(LPCSTR szModule);


MYFILE FAR * FAR PASCAL my_fsopen(LPSTR, LPSTR, UINT);
LPSTR FAR PASCAL myfgets(LPSTR, UINT, MYFILE FAR *);
int FAR PASCAL myfputs(LPSTR, MYFILE FAR *);
void FAR PASCAL myfclose(MYFILE FAR *);
BOOL FAR PASCAL myferror(MYFILE FAR *);


// registry and network 'helpers'

DWORD MyRegOpenKey(LPCSTR szKeyName);
void MyRegCloseKey(DWORD dwKey);
DWORD MyRegCreateKey(LPCSTR szKeyName);
LPSTR MyRegQueryValue(HKEY hKey, LPCSTR szName);
LPSTR MyRegEnumValues(HKEY hKey);
LPSTR MyRegSetValue(HKEY hKey, LPCSTR szName, DWORD dwType, LPCSTR szValue);
LPSTR MyRegDeleteValue(HKEY hKey, LPCSTR szName);
LPSTR DoNetView(LPCSTR szArg);


// additional WIN32 'defines'

#ifndef HKEY_CURRENT_USER

    #undef HKEY_CLASSES_ROOT
    #define HKEY_CLASSES_ROOT               (( HKEY) 0x80000000)

    #define HKEY_CURRENT_USER               (( HKEY) 0x80000001)
    #define HKEY_LOCAL_MACHINE              (( HKEY) 0x80000002)
    #define HKEY_USERS                      (( HKEY) 0x80000003)
    #define HKEY_PERFORMANCE_DATA           (( HKEY) 0x80000004)
    #define HKEY_CURRENT_CONFIG             (( HKEY) 0x80000005)
    #define HKEY_DYN_DATA                   (( HKEY) 0x80000006)
    #define HKEY_PREDEF_KEYS                7

#endif // HKEY_CURRENT_USER

#define HKEY_CLASSES_ROOT_WIN31           (( HKEY) 0x1)

#ifndef REG_NONE
#ifdef REG_SZ
#undef REG_SZ
#endif // REG_SZ
#ifdef REG_BINARY
#undef REG_BINARY
#endif // REG_BINARY
#ifdef REG_DWORD
#undef REG_DWORD
#endif // REG_DWORD

#define REG_NONE                    ( 0 )   // No value type
#define REG_SZ                      ( 1 )   // Unicode nul terminated string
#define REG_EXPAND_SZ               ( 2 )   // Unicode nul terminated string
                                            // (with environment variable references)
#define REG_BINARY                  ( 3 )   // Free form binary
#define REG_DWORD                   ( 4 )   // 32-bit number
#define REG_DWORD_LITTLE_ENDIAN     ( 4 )   // 32-bit number (same as REG_DWORD)
#define REG_DWORD_BIG_ENDIAN        ( 5 )   // 32-bit number
#define REG_LINK                    ( 6 )   // Symbolic Link (unicode)
#define REG_MULTI_SZ                ( 7 )   // Multiple Unicode strings
#define REG_RESOURCE_LIST           ( 8 )   // Resource list in the resource map
#define REG_FULL_RESOURCE_DESCRIPTOR ( 9 )  // Resource list in the hardware description
#define REG_RESOURCE_REQUIREMENTS_LIST ( 10 )

#endif // REG_NONE






/***************************************************************************
 *                         O P _ L E V E L ( )                             *
 ***************************************************************************/

/*  This function returns a positive integer value relating to the alge-   */
/*  braic order of operations of the character operator in the formal parm */

int NEAR __fastcall op_level(int _op)
{

    switch(_op)
    {
      case OP_NULL:
        return(0);        /* the null operator; used as end of string */


      case OP_RIGHTPAREN:
      case OP_RIGHTBRACKET:
        return(1);        /* the end parenthesis is slightly higher than */
                          /* the null, but less than all others          */
      case OP_LEFTPAREN:
      case OP_LEFTBRACKET:
        return(2);        /* ensure that a '()' group is evaluated right */

      case OP_COMMA:
        return(3);        /* comma operator - perform last, but before '()' */


      case OP_AND:
        return(10);       /* '&&', like '+' but for logical operators */

      case OP_OR:
        return(20);       /* '||', like '*' but for logical operators */

      case OP_EQUAL:
      case OP_LESSTHAN:
      case OP_GREATERTHAN:
      case OP_GREATEREQUAL:
      case OP_LESSEQUAL:
      case OP_NOTEQUAL:
        return(30);       /* relational - like '^', same level as logical */

      case OP_STREQUAL:
      case OP_SLESSTHAN:
      case OP_SGREATERTHAN:
      case OP_SGREATEREQUAL:
      case OP_SLESSEQUAL:
      case OP_STRNOTEQUAL:
        return(30);       /* relational - like '^', same level as logical */

      case OP_ADD:
      case OP_SUBTRACT:
      case OP_CONCAT:     /* concatenation same as add/subtract in order */
      case OP_BITAND:
      case OP_BITOR:
      case OP_BITXOR:
        return(100);      /* plus and minus have the same algeb. order   */
                          /* as well as bit-wise AND and OR operators.   */

      case OP_MULTIPLY:
      case OP_DIVIDE:
      case OP_MOD:
        return(200);      /* multiply & divide have same algeb. order */


      case OP_POWER:
        return(300);      /* exponentiation!  just for fun!              */


      case OP_NOT:        /* 'UNARY' operators - error if it gets here! */
      case OP_BITNOT:
      case OP_ERROR:      // the 'error' operator
      default:
        return(-1);       /* a SYNTAX ERROR! This will force termination */

    }
}


/***************************************************************************
 *                          O P _ P R E C ( )                              *
 ***************************************************************************/

/*  This function returns a positive value if the 1st operation has a */
/*  higher algebraic order of operation than the second, a value of 0 */
/*  if their order of operations is equal, and a negative value if    */
/*  the second operation has a higher algebraic order of operation.   */


int NEAR __fastcall op_prec(int oper1, int oper2)
{
int ilevel1, ilevel2;   /* integer level of operands - relates to the  */
                        /* algebraic order of operations.  A higher #  */
                        /* indicates higher order of operation.        */


    ilevel1 = op_level(oper1);    /* get the level of 1st operand */
    ilevel2 = op_level(oper2);    /* get the level of 2nd operand */
    if(ilevel1 > ilevel2)
    {
       return(1);
    }
    else if(ilevel1 == ilevel2)
    {
        return(0);
    }
    return(-1);
}


/***************************************************************************
 *                             V A L U E ( )                               *
 ***************************************************************************/

/*  This returns the value of a string segment between two pointer values */

LPSTR NEAR PASCAL value(LPSTR string, WORD P1, WORD P2)
{
LPSTR p, lp1;
WORD w1;
char copy_string[256];  /* a place to put junk in the meantime  */



    _fstrncpy(copy_string, string + P1, (P2-P1) );
    copy_string[P2-P1]=0;
                  /* places the 'value' segment (up to the operator) */
                  /* into 'copy_string'.  Now, find its value!       */

    /** TRIM SPACES **/

    while(*copy_string && *copy_string<=' ')
       lstrcpy(copy_string,copy_string + 1);


    for(p = copy_string + lstrlen(copy_string); p>copy_string && *(p - 1)<=' ';
        *(--p) = 0)
        ;      /* TRIM ALL TRAILING CONTROL CHARS */


    // ok - let's see if it's a constant or variable!
    // first, check if it's a string constant...

    if(*copy_string=='\'' || *copy_string=='"')
    {
       for(lp1 = copy_string+1; *lp1; lp1++)
       {
          if(*lp1=='\\')
          {
             if(lp1[1]=='\\' || lp1[1]=='"' || lp1[1]=='\'')
             {
                lp1++;  // skip next char if it's one of the above 3
             }
          }
          else if(*lp1==*copy_string)
          {
             break;
          }
       }

       if(*lp1!=*copy_string)  // syntax error
       {
          PrintErrorMessage(800);
          return(NULL);
       }

       *(lp1++) = 0;

       lstrcpy(copy_string, copy_string+1); // trim the quotes off

       while(*lp1 && *lp1<=' ') lp1++;

       if(*lp1)
       {
          PrintErrorMessage(801);
          return(NULL);
       }

       // eliminate all of the '\"', "\'", and '\\' in the string

       for(lp1=copy_string; *lp1; lp1++)
       {
          if(*lp1=='\\')
          {
             if(lp1[1]=='"' || lp1[1]=='\'' || lp1[1]=='\\')
             {
                lstrcpy(lp1, lp1 + 1);
             }
             else
             {
                lp1++;  // next char to follow '\' must be ignored by this
             }          // loop and passed into the array unchanged, along
                        // with the '\' that preceded it.
          }
       }

       lp1 = copy_string;
    }
    else if((*copy_string>='0' && *copy_string<='9') || *copy_string=='.'
            || *copy_string=='+' || *copy_string=='-')
    {
       lp1 = copy_string;    // initially...

       // make sure it's a number by checking to see if it's one of the
       // "smart file set" per-file variables...

       if(*copy_string=='.' && copy_string[1]>'9')
       {
          // see if this entry corresponds to any of the variable names
          // stored in the structure pointed to by 'lpSmartFileSet'

          if(lpSmartFileParm)
          {
             for(w1=0; w1<lpSmartFileParm->nEntryCount; w1++)
             {
                // NOTE:  don't compare the '.' (it won't be in the table)

                if(!_fstricmp(lpSmartFileParm->pEntry[w1].szVarName,
                              copy_string + 1))
                {
                   // found!  assign 'lp1' to the value for this entry

                   lp1 = lpSmartFileParm->pEntry[w1].lpValue;
                }
             }
          }
       }

    }
    else
    {
       lp1 = GetEnvString(copy_string);  // see if there's a variable...

       if(!lp1) lp1="";
    }


    p = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                              lstrlen(lp1) + 1);

    if(!p)
    {
       PrintString(pNOTENOUGHMEMORY);
       return(NULL);
    }

    lstrcpy(p, lp1);

    return(p);

}


/***************************************************************************/
/*                      IsOperator(), NextOperator()                       */
/***************************************************************************/

#pragma intrinsic(strlen,strcmp,strcpy,memcmp,memcpy)

#ifdef WIN32
#define my_strncmp(X,Y,Z) strncmp(X,Y,Z)
#else // WIN32

static int __inline NEAR PASCAL my_strncmp(char __near *p1, char __near *p2, size_t n)
{
   register int iRval;

   if(n==0)
     return(0);   // for 0 bytes, always equal!

   _asm
   {
      push si
      push di

      cld
      mov cx, n

      mov si, p1
      mov di, p2

      jmp main_lup2

main_lup:

      inc di

main_lup2:

// OLD SECTION STARTS HERE

//      lodsb              // 5 cycles 386 or 486
//      jz done_lt         // assume something ALWAYS follows operator in p2
//      cmp al, [di]
//      jnz not_equal

// OLD SECTION ENDS HERE

// NEW SECTION STARTS HERE

      mov al, [si]         // 4 cycles 386, 1 cycle 486
      cmp al, [di]         // placed here for better reliability & speed (?)
      jnz not_equal        // as we are only failing 1 compare and not 2

      or al,al             // 2 cycles 386, 1 cycle 486
      jz done_lt
      inc si               // 2 cycles 386, 1 cycle 486

// NEW SECTION ENDS HERE

      loop main_lup
      mov ax, 0
      jmp bailout

not_equal:
      jb done_lt
      mov ax, 1
      jmp bailout

done_lt:
      mov ax, -1

bailout:
      pop di
      pop si
      
      mov iRval, ax
   }

   return(iRval);
}

#endif // WIN32



static const BYTE CODE_BASED MyUpperCase[] =
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


static __inline BOOL NEAR PASCAL IsOperator(LPSTR line, SBPWORD ptr,
                                            SBPWORD op, SBPWORD op_start)
{  /* when I leave this proc '*ptr' points just past the op if it exists */
   /* the search for the operator must begin at (line + *ptr)            */
auto WORD i, j, p, l, wOldES;
auto char c, c2;
auto BOOL IsQuoteOrBlank;
auto LPSTR lp1;
auto char *pOper;


   p = *ptr;

#ifdef WIN32
   for(p=*ptr; line[p] && line[p]<=' '; p++)
      ;  /* get the next 'non-white-space' character in line */
      
   if(!line[p]) return(FALSE);  // end of string - no operator found

   lp1 = line + p;
   c = MyUpperCase[*lp1];
   
   if(p)
     c2 = *(lp1 - 1);
   else
     c2 = 0;
   
#else // WIN32
   _asm
   {
      mov wOldES, es

      push si
      les si, line
      add si, p

   z_loop:
      mov al, es:[si]
      or al,al
      jz z_loop_end

      cmp al, ' '
      ja z_loop_end

      inc si
      inc p
      jmp z_loop

   z_loop_end:

      mov WORD PTR lp1, si // lp1 == 'line + p'
      mov bl, es:[si]
      mov bh, 0
      mov al, cs:MyUpperCase[bx]
      mov c, al            // c == 'MyUpperCase[*lp1]'

      mov ax, p
      or ax, ax
      jz p_is_zero       // 'al' will also be zero

      dec si
      mov al, es:[si]    // c2 == 'line[p - 1]' but not upper case'd

   p_is_zero:
      mov c2, al
      pop si
   }

#endif // WIN32

   if(!c)
   {
#ifndef WIN32   
      _asm mov es, wOldES
#endif // WIN32

      return(FALSE);
   }


      // this next switch is used to check for multi-char operators
      // that begin with a space.

//   IsQuoteOrBlank = (p==0) || (line[p-1]<=' ') || (line[p-1]=='"');

   IsQuoteOrBlank = c2<=' ' || c2=='"';  // if p==0, c2==0


//   lp1 = line + p;

//   c = MyUpperCase[*lp1];


   // FOR SIMPLICITY, ASSUME THAT 'es' ABSOLUTELY WILL NOT NOT CHANGE!

   for(i=0; i<wNOperators; i++)
   {
      l = pOperatorLength[i];

      if(l==1 && pOperators[i][0] == c)
      {
                   /*** FOUND OPERATOR!! ***/

         if(op_start)
           *op_start = p;     /* starting position of operator */

         *ptr = p + l;     /* points *just* past the operator itself */
         *op = wOperators[i];
         
#ifndef WIN32
         _asm mov es, wOldES
#endif // WIN32         

         return(TRUE);
      }


      if(l>2 && IsQuoteOrBlank && *pOperators[i]<=' ')
      {
         pOper = pOperators[i] + 1;
         l -= 2;
      }
      else
         pOper = pOperators[i];

      if(*pOper != c) continue;   // this should be MONDO FAST!


      j = 0;

#ifdef WIN32

      int i1;
      for(i1=0; i1 < l; i1++)
      {
        if(MyUpperCase[*lp1] != pOper[i1])
        {
          j = 1;
          break;
        }
      }
    
      if(!j)
        c2 = lp1[l];

#else // WIN32
      _asm
      {
//         push es
         push si
         push di

         mov cx, l
         mov si, pOper
//         les di, lp1
         mov di, WORD PTR lp1

         mov bx, 0

   cmp_loop:
         mov bl, es:[di]
         mov al, cs:MyUpperCase[bx]

         cmp al, [si]   // already upper case, by the way

         jz the_same

         mov j, 1       // under normal circumstances it goes here on 1st iter
         jmp the_end

   the_same:
         inc si
         inc di

         dec cx
         jnz cmp_loop

         mov al, es:[di]
         mov c2, al          // 'c2' is next char in 'line' after operator

   the_end:

         pop di
         pop si
//         pop es
      }
#endif // WIN32      

                        /*** FOUND OPERATOR!! ***/

         /* if 1st char in op is 'white space' I must check for */
         /* other 'white space' possibilities also...           */

      if(!j && (pOper == pOperators[i] || c2<=' ' || c2=='"'))
      {

         if(op_start) *op_start = p;     /* starting position of operator */

         *ptr = p + l;     /* points *just* past the operator itself */
         *op = wOperators[i];

#ifndef WIN32
         _asm mov es, wOldES
#endif // WIN32         


         return(TRUE);
      }

   }


#ifndef WIN32
   _asm mov es, wOldES
#endif // WIN32         


   return(FALSE);          // NO OPERATORS WERE FOUND!

}


BOOL NEAR PASCAL NextOperator(LPSTR line, SBPWORD ptr, SBPWORD op,
                              SBPWORD op_start)
{
WORD i, j, p;


   for(p=*ptr; line[p] && line[p]<=' '; p++)
      ;  /* get the next 'non-white-space' character in line */

   for(; line[p]; p++)
   {
      if(line[p]=='"')
      {
         p++;

         while(line[p])
         {
            if(line[p]=='"') break;  // I am done now!!

            if(line[p]=='\\')  // a backslash?  Do some special processing
            {
               p++;
               if(line[p]=='\\' || line[p]=='\'' || line[p]=='"')
               {
                  p++;  // skip next char also if one of these 3
               }
            }
            else
            {
               p++;
            }
         }

         if(line[p]) continue;  // 'p' gets incremented at end of loop
         else        break;     // 'p' points to a ZERO byte
      }

      if(IsOperator(line, (SBPWORD)&p, op, op_start))
      {
         if(op_start &&  // never when 'op_start' is NULL
            (*op==OP_SUBTRACT || *op==OP_ADD)) // special case - look for number constant
         {
            j = *op_start - 1;        // point to position before 'op_start'

            if(line[j]=='E' || line[j]=='e')    // is there an 'E+' or 'E-'?
            {
               for(i=*ptr; i<j; i++)            // yes - is it a valid number?
               {
                  if((line[i]<'0' || line[i]>'9') && line[i]!='.')
                  {
                     break;  // we found out that it is *NOT* a number!
                  }
               }

               if(i>=j) // that is, we did NOT find a non-numeric character
               {
                  continue; // assume it's a number constant with an EXPONENT
               }
            }
         }               // if this whole operation doesn't continue the loop
                         // we have a valid operator!!! Yay!!!
         *ptr = p;
         return(TRUE);     /** FOUND!! **/
      }

      // because of section above, must not have any code here...
   }

   *ptr += lstrlen(line + *ptr);     // points to end of string (NULL BYTE)

   if(op_start) *op_start = *ptr;    // position of operator

   *op = OP_NULL;
   return(FALSE);   /** NO OPERATOR FOUND **/

}


/***************************************************************************
 *                           G E T _ T E R M ( )                           *
 ***************************************************************************/

 /*   This function returns the value of a term, and updates the next  */
 /*   operation value.  Negation and parentheses are handled here      */



LPSTR NEAR PASCAL get_term(LPSTR line, SBPWORD ptr, SBPWORD op)
{
int i;
WORD op_temp, P1, op_start, nArg, w, w2, w3;
LPSTR p, arg1, fn, lpArg[16];
static char function_buf[64];


    while(line[*ptr] && line[*ptr]<=' ') (*ptr)++;  // trim lead spaces

    P1 = *ptr;                          /* save the beginning pointer value */

             /** CHECK FOR AND PROCESS UNARY OPERATORS **/

    if(IsOperator(line, ptr, (SBPWORD)&op_temp, NULL))
    {
       if(op_temp==OP_SUBTRACT)         /* negation operator */
       {

          arg1 = get_term(line, ptr, op);

          return(CalcEvaluateOperator(arg1, NULL, OP_NEGATE));

       }
       else if(op_temp==OP_NOT || op_temp==OP_BITNOT) /* LOGICAL/BIT 'NOT' */
       {
          arg1 = get_term(line, ptr, op);

          return(CalcEvaluateOperator(arg1, NULL, op_temp));

       }
       else if(op_temp==OP_LEFTPAREN)   /* handle '( )' */
       {
          *op=OP_LEFTPAREN;             /* flag operation as a left paren */

          arg1 = seg_parse(line, ptr, op);// evaluate inside the parentheses
          if(!arg1) return(NULL);

              /* verify that the ')' was found, and get the next */
              /* operator for evaluation!                        */

          if(*op!=OP_RIGHTPAREN)
          {                               /* ensure that there was a ')' */
                                          /* and report the error if not */

             PrintErrorMessage(802);

             GlobalFreePtr(arg1);

             return(NULL);

          }

          NextOperator(line, ptr, op, NULL);
          return(arg1);
       }
       else
       {
          PrintErrorMessage(803);
          return(NULL);
       }
    }
    else
    {
           /*  here we handle normal evaluation.  */
           /*  Step 1:  find the next operator    */

       NextOperator(line, ptr, op, (SBPWORD)&op_start);

    }


    if(*op==OP_LEFTPAREN)                     /* FUNCTION CALL! */
    {


      _fstrncpy(function_buf, line + P1, (op_start-P1) );
      function_buf[op_start-P1]=0;
                    /* places the 'value' segment (up to the operator) */
                    /* into 'function_buf'.  Now, find its value!       */

      /** TRIM SPACES **/

      while(*function_buf && *function_buf<=' ')
         strcpy(function_buf,function_buf + 1);


      for(p = function_buf + lstrlen(function_buf); p>function_buf && *(p - 1)<=' ';
          *(--p) = 0)
          ;      /* TRIM ALL TRAILING CONTROL CHARS */


      fn = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                 lstrlen(function_buf) + 2);
      if(!fn)
      {
         PrintString(pNOTENOUGHMEMORY);
         return(NULL);
      }

      lstrcpy(fn, function_buf);

      nArg = 0;  // argument counter for function call



      do
      {
         // search for blank arguments!

         w3 = *ptr;
         w2 = *op;

         NextOperator(line, (SBPWORD)&w3, (SBPWORD)&w2, (SBPWORD)&op_start);

         if(w2==OP_RIGHTPAREN || w2==OP_COMMA)  // might be blank...
         {
            for(w=*ptr; w<op_start; w++)
            {
               if(line[w]>' ') break;           // not blank
            }

            if(w>=op_start)         // it *is* only white-space (a blank arg)
            {
               *ptr = w3;                   // points pointer past op
               *op = w2;                    // assigns current op

               if(w2==OP_RIGHTPAREN && !nArg)  // was it ')' on the 1st arg?
               {
                  break;         // assume that this blank argument is 'none'
               }

               // here it was a ',' - load 'lpArg' with a blank string

               if(lpArg[nArg] = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE, 2))
               {
                  *(lpArg[nArg++]) = 0;
               }
               else
               {
                  for(w=0; w<nArg; w++)
                  {
                     if(lpArg[w]) GlobalFreePtr(lpArg[w]);
                  }

                  GlobalFreePtr(fn);

                  return(NULL);
               }

               continue;        // keep going through loop
            }

         }

         // at this point we have a 'non-white-space' argument. use the
         // 'seg_parse()' function to extract it and add to the list.

         if(!(lpArg[nArg++] = seg_parse(line, ptr, op)))
         {               /* evaluate next parameter inside the parentheses */
            for(w=0; nArg && w<(nArg - 1); w++)
            {
               if(lpArg[w]) GlobalFreePtr(lpArg[w]);
            }

            GlobalFreePtr(fn);

            return(NULL);

         }



          /* verify that either ')' or ',' was found, and get the next */
          /* parameter if ',';  bail out of loop if ')' (done!)        */

         if(*op!=OP_COMMA && *op!=OP_RIGHTPAREN)
         {                               /* ensure that there was a ')' */
                                         /* and report the error if not */

            for(w=0; w<nArg; w++)
            {
               if(lpArg[w]) GlobalFreePtr(lpArg[w]);
            }

            GlobalFreePtr(fn);

            PrintErrorMessage(804);

            return(NULL);
         }


         // must not have any 'end-of-loop' stuff here, or the 'blank arg'
         // section won't work properly anymore.


      } while(*op==OP_COMMA);        /* evaluate all args until NO COMMA! */


      P1 = *ptr;
      NextOperator(line, ptr, op, (SBPWORD)&op_start);

      if(op_start>P1)  // white space between ')' and next operator
      {
         for(i=P1; i<(int)op_start; i++)
         {
            if(line[i]>' ')
            {
               for(w=0; w<nArg; w++)
               {
                  if(lpArg[w]) GlobalFreePtr(lpArg[w]);
               }

               GlobalFreePtr(fn);

               PrintErrorMessage(805);

               return(NULL);

            }
         }
      }


      lpArg[nArg] = NULL;  // terminates list of arguments!
      return(CalcEvaluateFunction(fn, lpArg));

    }
    else if(*op==OP_LEFTBRACKET)                     /* ARRAY INDEX! */
    {

      _fstrncpy(function_buf, line + P1, (op_start-P1) );
      function_buf[op_start-P1]=0;
                    /* places the 'value' segment (up to the operator) */
                    /* into 'function_buf'.  Now, find its value!       */

      /** TRIM SPACES **/

      while(*function_buf && *function_buf<=' ')
         strcpy(function_buf,function_buf + 1);


      for(p = function_buf + lstrlen(function_buf); p>function_buf && *(p - 1)<=' ';
          *(--p) = 0)
          ;      /* TRIM ALL TRAILING CONTROL CHARS */


      fn = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                 lstrlen(function_buf) + 2);
      if(!fn)
      {
         PrintString(pNOTENOUGHMEMORY);
         return(NULL);
      }

      lstrcpy(fn, function_buf);

      nArg = 0;  // argument counter for function call

      do
      {
         if(!(lpArg[nArg++] = seg_parse(line, ptr, op)))
         {               /* evaluate next parameter inside the parentheses */
            for(w=0; nArg && w<(nArg - 1); w++)
            {
               if(lpArg[w]) GlobalFreePtr(lpArg[w]);
            }

            GlobalFreePtr(fn);

            return(NULL);

         }


          /* verify that either ')' or ',' was found, and get the next */
          /* parameter if ',';  bail out of loop if ')' (done!)        */

         if(*op!=OP_COMMA && *op!=OP_RIGHTBRACKET)
         {                               /* ensure that there was a ')' */
                                         /* and report the error if not */

            for(w=0; w<nArg; w++)
            {
               if(lpArg[w]) GlobalFreePtr(lpArg[w]);
            }

            GlobalFreePtr(fn);

            PrintErrorMessage(806);

            return(NULL);
         }

      } while(*op==OP_COMMA);        /* evaluate all args until NO COMMA! */

      P1 = *ptr;
      NextOperator(line, ptr, op, (SBPWORD)&op_start);

      if(op_start>P1)  // white space between ']' and next operator
      {
         for(i=P1; i<(int)op_start; i++)
         {
            if(line[i]>' ')
            {
               for(w=0; w<nArg; w++)
               {
                  if(lpArg[w]) GlobalFreePtr(lpArg[w]);
               }

               GlobalFreePtr(fn);

               PrintErrorMessage(807);

               return(NULL);
            }
         }
      }

      lpArg[nArg] = NULL;  // terminates list of arguments!
      return(CalcEvaluateArray(fn, lpArg));

    }
    else
    {
       return(value(line,P1,op_start)); /* return the value of the term */
    }

}


/***************************************************************************
 *                          S E G _ P A R S E ( )                          *
 ***************************************************************************/

/* THIS IS A VERY POWERFUL RECURSIVE ROUTINE TO PARSE AN EQUATION USING
   ALGEBRAIC ORDER OF OPERATIONS                                         */



LPSTR FAR PASCAL seg_parse( LPSTR line, SBPWORD ptr, SBPWORD op)
{
int op_cmp=0;
WORD op1, old_op, push_op;
LPSTR arg1, arg2;

  old_op = *op;

  push_op = OP_ERROR;  // used for error detection only

  arg1 = get_term( line, ptr, op);           /* find next operation and get */
  if(!arg1)                                  /* the term associated with it */
  {
     return(NULL);
  }

  do
  {
     op_cmp = op_prec((int) old_op ,(int) *op );

     if(op_cmp<0)                   /* if next operation has higher prec. */
     {                              /* evaluate by using recursion        */
       op1 = *op;

       if(op1==OP_RIGHTPAREN)       // this is always a SYNTAX ERROR
       {
//          printf("?Syntax error - parentheses mismatch (extra right ')')\n");
//          ADD_EQUATION_ERROR(lpEQ,
//                    "?Syntax error - parentheses mismatch - extra ')' found",
//                    *ptr);

//          return(TRUE);

          PrintErrorMessage(808);

          GlobalFreePtr(arg1);
          return(NULL);
       }
       else if(op1==OP_COMMA)
       {
          /* at this point, *ptr will point to the next character in line */
          /* after the ",", and *op is equal to the "," operator flag.    */

          return(arg1);
       }
       else
       {
//        WORD ww;
//          for(ww=0; ww<wNOperators; ww++)
//          {
//             if(wOperators[ww] == op1)
//             {
//                break;
//             }
//          }
//
//          if(ww<wNOperators)
//          {
//             printf("PUSH OPERATOR \"%s\"\n", pOperators[ww]);
//          }
//          else
//          {
//             printf("PUSH OPERATOR #%x\n", op1);
//          }

//          ADD_EQUATION_PUSHOPERATOR(lpEQ, op1);

          push_op = op1;
       }

       arg2 = seg_parse( line, ptr, op);
                   /* recursive call to evaluate until an operation with  */
                   /* equal or lower precedence is found                  */

       if(!arg2)
       {
          GlobalFreePtr(arg1);
          return(NULL);
       }

       arg1 = CalcEvaluateOperator(arg1, arg2, push_op);

       if(!arg1)
       {
          return(NULL);
       }

       push_op = OP_ERROR;

//       printf("EVALUATE\n");

//       ADD_EQUATION_EVALUATE(lpEQ, EQUATION_EVALUATE);

     }
  }
  while(op_cmp<0);  /* repeat for all operations with this or greater     */
                    /* level of precedence, accumulating the result in    */
                    /* the variable 'r1'.                                 */

     /* at this point, *ptr will point to the next character in line after
        the operation, and *op is equal to the operation preceding *ptr   */

//  return(FALSE);

  return(arg1);     // 'arg1' became my accumulator, I guess!!
}



/***************************************************************************
 *                         S E G _ E V A L ( )                             *
 ***************************************************************************/

 /*  This function calls the main segment parser to parse a string, and  */
 /*  checks for errors, etc.                                             */



LPSTR FAR PASCAL Calculate(LPSTR line)
{
LPSTR lpRval, lp1;
WORD w;
WORD PTR, OPER;        /*  here is the memory location of 'ptr'and 'op'! */
                       /*  the addresses are passed to seg_parse         */

    if(!line) return(NULL);  // in case it's a NULL, return NULL!


    PTR=0;             /*  initialize a few things here   */
    OPER=OP_NULL;      /*  forces seg_parse to go to the end of the string */

    lpRval = seg_parse(line, (SBPWORD)&PTR, (SBPWORD)&OPER);

    if(!lpRval)        // returned an error - am I in 'diag' mode??
    {
       if((lp1 = GetEnvString(szCALC_DIAG)) && *lp1)
       {
          while(*lp1 && *lp1<=' ') lp1++;

          if(*lp1>='0' && *lp1<='9')
          {
             if(!CalcEvalNumber(lp1)) return(NULL);
          }
          else if(!*lp1)
          {
             return(NULL);
          }


          PrintString(line);
          PrintString(pCRLF);
          for(w=0; w<PTR; w++)
          {
             PrintAChar(' ');
          }

          PrintAChar('^');
          PrintString(pCRLF);  // this shows where the PTR was (or might be?)

       }

    }
    else if(lpRval && OPER!=OP_NULL) // no error, but not at the end!
    {
       PrintErrorMessage(809);
       if(OPER==OP_COMMA)
       {
          PrintErrorMessage(810);
       }
       else
       {
          PrintAChar('\"');
          PrintString(line + PTR);
          PrintErrorMessage(811);
       }

       GlobalFreePtr(lpRval);
       return(NULL);
    }

    return(lpRval);

}




#pragma code_seg("CALC_FUNCTION_TEXT","CODE")


// note that 'GetWindow()','GetTask()','GetInstance()', 'GetModule()',
// and 'STR()' allow either 1 or 2 parameters; as a result, any
// parameters which are not specified are passed as BLANKS or ZEROS


    /* the next function calls a function using an array of 'LPSTR's */
    /* pointing to the various parameters.  A 'NULL' ends the list.  */
    /* The function returns NULL on error, or a text representation  */
    /* of the resulting equation value.                              */


static char tbuf[MAX_PATH * 2];


LPSTR FAR PASCAL StringFromFloat(double d)
{
LPSTR lpRval;
WORD w;

   _gcvt(d, 18, tbuf);

   w = strlen(tbuf);

   if(w && tbuf[w - 1]=='.') tbuf[w - 1] = 0;  // trim a trailing '.' if it exists


   lpRval = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE, strlen(tbuf) + 2);

   if(lpRval) lstrcpy(lpRval, tbuf);

   return(lpRval);

}


LPSTR FAR PASCAL StringFromInt(long l, int radix)
{
LPSTR lpRval;


   if(radix<2) radix=10;  // default setting

   _ltoa(l, tbuf, radix);


   lpRval = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE, strlen(tbuf) + 2);

   if(lpRval) lstrcpy(lpRval, tbuf);

   return(lpRval);

}


double FAR PASCAL CalcEvalNumber(LPCSTR lpS)
{
LPCSTR lp1, lp2;
WORD l, radix, count;
BOOL sign;
double rval;
int x;
static const char digits[]="0123456789ABCDEF";



   for(lp1=lpS; *lp1 && *lp1<=' '; lp1++)
      ;  // find first NON-WHITE-SPACE


   if(!*lp1) return(0.0);  // zero length string - done!


   if((*lp1<'0' || *lp1>'9') && *lp1!='-' && *lp1!='+' && *lp1!='.')
   {
        // variable expansion is now performed in the 'value()' function

//      lp1 = GetEnvString(lp1);
//
//      if(!lp1)
//      {
      PrintErrorMessage(812);

      return(0.0);  // done!
//      }
   }
   else
   {
      if(lstrlen(lp1)>=sizeof(tbuf))
      {
         PrintErrorMessage(813);
      }

   }

//   if(!*lp1) return(0.0);  // zero length string - done!

   lstrcpy(tbuf, lp1);


   l = lstrlen(lpS);

   while(l && tbuf[l - 1]<=' ')
   {
      tbuf[l - 1] = 0;     // trim off trailing white-space
      l --;
   }

   if(!l) return(0.0);     // zero length string - done!


   if(toupper(tbuf[l - 1])=='H')  /* a hex number, we assume... */
   {
      tbuf[l - 1] = 0;                      /* trim the 'H' */
      _fmemmove(tbuf + 2, tbuf, l);  /* moves all, including NULL byte */

      tbuf[0] = '0';
      tbuf[1] = 'x';      /* insert a '0x' for 'atoi' */

      l += 2;            /* the new length! */

   }

   rval = 0;


   if(*tbuf=='0')           /* octal or hex? */
   {



      if(toupper(tbuf[1])=='X')      // IS IT DEFINITELY HEXADECIMAL?
      {
         radix=16;
         lp2 = tbuf + 2;
      }
      else if(strchr(tbuf, '.') || strchr(tbuf, 'E') || strchr(tbuf, 'e'))
      {                      // is it a float?? does it contain one of these?
         return(atof(tbuf));
      }
      else                   // not FLOAT, so it's definitely OCTAL
      {
         radix=8;
         lp2 = tbuf + 1;
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
            PrintErrorMessage(814);
            break;
         }

         rval = rval * radix + x;
      }

      if(sign)
         return(-rval);
      else
         return(rval);

   }
   else
   {
      return(atof(tbuf));
   }

}


LPSTR FAR PASCAL CalcEvalString(LPSTR lpS)
{
//LPSTR lp1;



   return(lpS);  // string evaluation is now done in the 'value()' function
                 // as well as variable name expansion


//   while(*lpS && *lpS<=' ') lpS++;  // find first non-white-space
//
//   if(*lpS=='"' || *lpS=='\'')    // string literal
//   {
//      for(lp1 = lpS + 1; *lp1; lp1++)
//      {
//         if(*lp1 == *lpS && *(lp1 - 1)!='\\')
//         {
//            *(lp1++) = 0;
//
//            lstrcpy(lpS, lpS + 1);  // slide string back a bit...
//
//            while(*lp1)
//            {
//               if(*lp1>' ')
//               {
//                  PrintString("?Syntax error - quote mismatch in string\r\n");
//                  return(NULL);
//               }
//
//               lp1++;
//            }
//         }
//      }
//
//      // next - take out all of the '\"' and '\'' that I find
//
//      for(lp1=lpS; *lp1; lp1++)
//      {
//         if(*lp1=='\\' && (*(lp1 + 1)=='"' || *(lp1 + 1)=='\''))
//         {
//            lstrcpy(lp1, lp1 + 1);
//         }
//      }
//
//      return(lpS);
//
//   }
//   else               // string variable
//   {
//      lp1 = GetEnvString(lpS);
//      if(!lp1)
//      {
//         *lpS = 0;
//
//         return(lpS);
//      }
//
//      return(lp1);
//   }

}


LPSTR FAR PASCAL CalcCopyString(LPCSTR lpS, WORD wStart, WORD wLen)
{
LPSTR lpRval;
WORD w;



   w = lstrlen(lpS + wStart);

   if(wLen>w)
   {
      wLen = w;
   }

   lpRval = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                  wLen + 2);


   if(lpRval)
   {
      _fmemcpy(lpRval, lpS + wStart, wLen);

      lpRval[wLen] = 0;
   }

   return(lpRval);

}



HMODULE FAR PASCAL CalcGetModuleHandle(LPCSTR szModule)
{
MODULEENTRY me;
TASKENTRY te;


   if(!lpModuleFirst) return(GetModuleHandle(szModule));

   _hmemset((LPSTR)&me, 0, sizeof(me));

   me.dwSize = sizeof(me);

   if(lpModuleFirst(&me))
   {
      do
      {
         if(!_fstricmp(szModule, me.szModule)) return(me.hModule);
         if(!_fstricmp(szModule, me.szExePath)) return(me.hModule);

      } while(lpModuleNext(&me));
   }



   // this next section is here as a "hack"

   if(!lpTaskFirst) return(NULL);

   _hmemset((LPSTR)&te, 0, sizeof(te));

   te.dwSize = sizeof(te);

   if(lpTaskFirst(&te))
   {
      do
      {
         if(!_fstricmp(szModule, te.szModule)) return(te.hModule);

         _hmemset((LPSTR)&me, 0, sizeof(me));

         me.dwSize = sizeof(me);

         if(te.hModule == lpModuleFindHandle(&me, te.hModule))
         {
            if(!_fstricmp(szModule, me.szModule)) return(me.hModule);
            if(!_fstricmp(szModule, me.szExePath)) return(me.hModule);
         }

      } while(lpTaskNext(&te));
   }


   return(NULL);
}



WORD FAR PASCAL CalcGetWindowInstTaskModule(LPCSTR szValue, UINT FAR *lpwType)
{
LPCSTR lp1 = szValue;
UINT w1, w2;



   if(lp1)
   {
      while(*lp1 && *lp1<=' ') lp1++;  // kill off white-space

      if(!*lp1)
      {
         *lpwType = 0xffff;  // *ALL* windows!

         return(0);          // may not be an error in some cases...
      }
   }


   w2 = 0;  // 'undefined' flag...

   if(!lp1 || (*lp1>='0' && *lp1<='9')  ||     // it's a number, ok?
      (toupper(*lp1)>='A' && toupper(*lp1)<='F' &&
        toupper(lp1[lstrlen(lp1) - 1])=='H'))
   {
      if(lp1)
      {
         w1 = (WORD)CalcEvalNumber(lp1);     // get numerical value
      }
      else
      {
         w1 = 0;
      }

      if(!w1)
      {
         w1 = (UINT)GetCurrentTask();

         w2 = 3;
      }
      else if(IsWindow((HWND)w1))
      {
         w2 = 1;       // flag that it's a window
      }
      else if(lpIsTask((HTASK)w1))
      {
         w2 = 3;       // flag that it's a task
      }
      else if(GetTaskFromInst((HINSTANCE)w1))
      {
         if(GetTaskFromInst((HINSTANCE)w1) == (HTASK)w1)
         {
            w2 = 3;    // flag that it's a task
         }
         else
         {
            w2 = 2;    // flag that it's an instance
         }
      }
      else if(GetModuleFileName((HMODULE)w1, tbuf, sizeof(tbuf))!=NULL)
      {
         w1 = (WORD)CalcGetModuleHandle(tbuf);
         w2 = 4;    // likely as not, module handle (well, NOW, anyway)
      }
      else
      {
         w2 = 0;    // none of the above...
      }
   }

   if(!w2)
   {
      w1 = (WORD)CalcGetModuleHandle(lp1);

      if(w1)  // only if valid...
      {
         w2 = 4;       // it's a module handle
      }
   }


   *lpwType = w2;
   return(w1);

}


// EXTERNAL 'DEFINE'd FUNCTIONS!

LPSTR FAR PASCAL CalcExternalFunction(CODE_BLOCK FAR *lpCB, LPSTR FAR *lplpArgs)
{
LPSTR lp1, lp2, lpParmList, lpRval;
BOOL bByVal, bByRef;
WORD w1, w2, wCount;
union _temp_ {
  double d;
  float f;
  short s;
  WORD w;
  long l;
  DWORD dw;
  LPSTR lp;
  } uArray[32];  // 1 for each possible parameter...
WORD pStack[64]; // maximum of 64 WORDS for parameters on stack
WORD nStack;     // # of items to be placed onto stack


   // The format of the function arguments in 'lpCB' is as follows:
   // [BYVAL|BYREF] DATATYPE [, [BYVAL|BYREF] DATATYPE [,...]]
   // such that a 'BYVAL' parameter passes a value only, but a 'BYREF'
   // parameter expects a variable name in quotes to be passed for the
   // parameter, and the variable name must contain a value of the
   // appropriate type.  By default, the parameter is passed by reference,
   // but accepts a value (and not a variable name) for the parameter itself.
   //
   // NOTE:  maximum of 32 parameters only!


   lpRval = NULL;  // initial value...


   for(wCount=0; wCount < N_DIMENSIONS(uArray); wCount++)
   {
      if(!(lplpArgs[wCount])) break;
   }

   if(lplpArgs[wCount])
   {
      return(NULL);  // TOO MANY ARGUMENTS!!!
   }



   lpParmList = lpCB->szInfo + lstrlen(lpCB->szInfo) + 1;

   for(w1=0, lp1 = lpParmList; lplpArgs[w1]; w1++)
   {
      bByVal = bByRef = FALSE;

      while(*lp1 && *lp1<=' ') lp1++;  // next non-white-space

      for(lp2=lp1; *lp2 > ' ' && *lp2 != ','; lp2++)
         ;  // next white-space

      w2 = lp2 - lp1;

      if(w2 == 5 && !_fstrnicmp(lp1, "BYVAL", w2))
      {
         bByVal = TRUE;

         lp1 = lp2;
         while(*lp1 && *lp1<=' ') lp1++;  // next non-white-space

         for(lp2=lp1; *lp2 > ' ' && *lp2 != ','; lp2++)
            ;  // next white-space

         w2 = lp2 - lp1;
      }
      else if(w2 == 5 && !_fstrnicmp(lp1, "BYREF", w2))
      {
         bByRef = TRUE;

         lp1 = lp2;
         while(*lp1 && *lp1<=' ') lp1++;  // next non-white-space

         for(lp2=lp1; *lp2 > ' ' && *lp2 != ','; lp2++)
            ;  // next white-space

         w2 = lp2 - lp1;
      }

      // at this point 'lp1' should point to the data type, and 'lp2'
      // to either end of string, white space, or a comma.






      if(*lp2==',')
      {
         lp2++;
         while(*lp2 && *lp2 <=' ') lp2++;  // next non-white-space

         if(!*lp2)   // no argument!!!
         {
            lpRval = NULL;

            break;
         }
      }
   }




   return(lpRval);  // for NOW...
}



LPSTR FAR PASCAL CalcEvaluateFunction(LPSTR function, LPSTR FAR *lplpArgs)
{
UINT w, n, fn, w1, w2, w3, w4;
int i;
DWORD dwNow, dw1, dw2, dw3, dw4;
double v1, v2, v3, v4;
LPSTR lpRval = NULL, lp1, lp2, lp3, lp4;
LPWIN32_FIND_DATA lpFD;
FARPROC lpProc;
TASKENTRY te;
MODULEENTRY me;
OFSTRUCT ofstr;
SFTDATE dt;
SFTTIME tm;
MYFILE FAR *f1;
struct find_t ff0;
struct _enum_parms_ {
   WORD wItem;
   WORD wType;
   WORD wIter;
   WORD wCount;
 } EnumParms;
struct _enum_parms2_ {
   WORD wItem;
   WORD wType;
   WORD wCount;
   WORD wID, wIDFlag;
   LPSTR lpClass, lpCaption;
   LPSTR lpRval;
   FARPROC lpProc;
 } EnumParms2;


#define _PI_ 3.1415926535897932384626433832795



   for(n=0; lplpArgs[n]; n++)
      ;  // find out how many parameters are passed to the function


   INIT_LEVEL


   w1 = 0;
   w2 = N_DIMENSIONS(pFunctionList) - 1;

   do    // PERFORM BINARY SEARCH FOR FUNCTION NAME!
   {
      fn = (w1 + w2) / 2;

      lp1=function;
      lp2=(LPSTR)pFunctionList[fn].pName;

      while(*lp1 && *lp2 && toupper(*lp1)==toupper(*lp2))
      {
         lp1++;
         lp2++;
      }

      if(!*lp1 && !*lp2)  // matchee-poo!
      {
         break;  // FOUND!!!
      }
      else if(toupper(*lp1) < toupper(*lp2))
      {
         w2 = fn - 1;
      }
      else // toupper(*lp1) > toupper(*lp2)
      {
         w1 = fn + 1;
      }

      if(w1 > w2)
      {
         fn = 0xffff;   // this forces 'function name not found'
         break;
      }

   } while(TRUE);


   if(fn>=N_DIMENSIONS(pFunctionList))
   {
      // check for any DEFINED functions (batch file only)


      if(lpBatchInfo && lpBatchInfo->lpBlockInfo)
      {
       LPBLOCK_INFO lpBI = lpBatchInfo->lpBlockInfo;  // for speed...
       LPSTR lpSaveParms = NULL;

         // look for 'DEFINE' entries in the array of 'CODE_BLOCK' structs
         // and see if any of them match the current function...

         lp1 = NULL;  // this is a flag, by the way

         for(w1=0; w1<lpBI->dwEntryCount; w1++)
         {
            if((lpBI->lpCodeBlock[w1].dwType & CODEBLOCK_TYPEMASK)
                 != CODEBLOCK_DEFINE &&
               (lpBI->lpCodeBlock[w1].dwType & CODEBLOCK_TYPEMASK)
                 != CODEBLOCK_DEFINE_EX)
            {
               continue;
            }

            // a 'DEFINE' for function!  Now, ensure that it's
            // the one we're looking for!

            lp1 = lpBI->lpCodeBlock[w1].szInfo;

            if(!_fstricmp(function, lp1)) break;  // FOUND!

            lp1 = NULL;
         }

         if(lp1)  // I FOUND IT! The function names match!
         {
            // Now, if this is an EXTERNAL function, I need to dispatch
            // it properly.  Argument count will also be verified.

            if((lpBI->lpCodeBlock[w1].dwType & CODEBLOCK_TYPEMASK)
                 == CODEBLOCK_DEFINE_EX)
            {
               // EXTERNAL define.  Dispatch it properly...

               lpRval = CalcExternalFunction(lpBI->lpCodeBlock + w1, lplpArgs);

               break;
            }

            // THIS IS EITHER AN 'inline' OR A 'block' DEFINE!
            // Now see if the argument lists match...

            lp1 += lstrlen(lp1) + 1;

            if(!*lp1) w2 = 0;
            else for(w2=1, lp2 = lp1; *lp2; lp2++)
            {
               if(*lp2==',') w2++;
            }

            if(w2 != n)  // argument count mismatch
            {
               PrintErrorMessage(816);
               PrintString(function);
               PrintString(pQCRLF);

               break;
            }

            // Next job is to 1) save current values for parameter
            // variables, 2) re-assign them the parms to the function, and
            // 3) call the appropriate part of the batch file and execute
            // this function, then 4) restore the parms (all are 'by value')
            // to their original values and return the value 'RETVAL' to
            // whomever is the caller.

            // 1) save current values for parameter variables

            if(*lp1)  // we HAVE parameters...
            {
               w3 = 0;

               lp2 = lp1;

               // first step - get length needed to save values

               while(*lp2 && *lp2<=' ') lp2++;

               while(*lp2)
               {
                  lp3 = lp2;

                  while(*lp3 && *lp3 != ',') lp3++;  // end of next parm

                  _hmemcpy(tbuf, lp2, lp3 - lp2);    // copy to 'tbuf'
                  tbuf[lp3 - lp2] = 0;

                  lp2 = lp3;
                  if(*lp2) lp2++;
                  while(*lp2 && *lp2<=' ') lp2++;

                  _fstrtrim(tbuf);

                  lp3 = GetEnvString(tbuf);
                  if(lp3) w3 += lstrlen(lp3);
                  w3++;
               }

               lpSaveParms = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                                   w3 + 16);

               if(!lpSaveParms)
               {
                  PrintErrorMessage(792);  // not enough memory to perform operation

                  break;
               }


               // THIS time, save the parameters, then assign the value
               // for the various arguments to each.

               lp2 = lp1;
               lp4 = lpSaveParms;
               w3 = 0;

               while(*lp2 && *lp2<=' ') lp2++;

               while(*lp2)
               {
                  lp3 = lp2;

                  while(*lp3 && *lp3 != ',') lp3++;  // end of next parm

                  _hmemcpy(tbuf, lp2, lp3 - lp2);    // copy to 'tbuf'
                  tbuf[lp3 - lp2] = 0;

                  lp2 = lp3;
                  if(*lp2) lp2++;
                  while(*lp2 && *lp2<=' ') lp2++;

                  _fstrtrim(tbuf);

                  lp3 = GetEnvString(tbuf);
                  if(lp3)
                  {
                     lstrcpy(lp4, lp3);
                     lp4 += lstrlen(lp4) + 1;
                  }
                  else
                  {
                     *(lp4++) = 0;
                  }

                  *lp4 = 0;

                  // assign parameter value...

                  SetEnvString(tbuf, CalcEvalString(lplpArgs[w3++]));
               }


               INIT_LEVEL  // special level for user-defined functions

               if(lpBI->lpCodeBlock[w1].dwStart == (DWORD)-1L)
               {
                  // THIS IS AN 'INLINE' DEFINE FUNCTION

                  lp1 += lstrlen(lp1) + 1;  // point to function text

                  lpRval = Calculate(lp1);
               }
               else
               {
                  // THIS IS A 'BLOCK' DEFINE FUNCTION


                  // NEXT, perform a 'CALL' to the user defined function.

                  w3 = (UINT)(lpBI->dwEntryCount++);  // set up 'CALL' entry to mark
                                                      // batch file position on return

                  lpBI->lpCodeBlock[w3].dwType  = CODEBLOCK_CALL;
                  lpBI->lpCodeBlock[w3].dwStart = lpBatchInfo->buf_start
                                                + lpBatchInfo->buf_pos;

                  // this points to the next line in the batch file, which
                  // will be restored on RETURN.  'w1' is the index of the
                  // function I'm calling.  If 'lpBlockInfo' member becomes
                  // NULL I am 'nuking' the batch file.

                  if(SetBatchFilePosition(lpBatchInfo,
                                          lpBI->lpCodeBlock[w1].dwStart))
                  {
                     // error message goes here

                     break;
                  }

                  // NEXT, start a new thread for the proc I'm calling,
                  // and wait until it finishes.  This will ensure that
                  // cleanup can be properly performed, and that I can
                  // correctly track whether I'm in a function or not.


                  *((HANDLE FAR *)lpBI->lpCodeBlock[w3].szInfo) = (HANDLE)
                     SpawnThread((LPTHREADPROC)CalcUserFunctionThread, (HANDLE)w1,
                                 THREADSTYLE_GOON | THREADSTYLE_RETURNHANDLE);

                  if(!*((HANDLE FAR *)lpBI->lpCodeBlock[w3].szInfo))
                  {
                     // error message goes here

                     break;
                  }

                  if(MthreadWaitForThread(*((HANDLE FAR *)lpBI->lpCodeBlock[w3].szInfo)))
                  {
                     break;
                  }

                  lp3 = GetEnvString(szRETVAL);

                  if(!lp3) lp3 = "";
                  lpRval = CalcCopyString(lp3, 0, 0xffff);
               }


               END_LEVEL

               // restore variables!

               if(lpSaveParms)
               {
                  lp2 = lp1;
                  lp4 = lpSaveParms;

                  while(*lp2 && *lp2<=' ') lp2++;

                  while(*lp2)
                  {
                     lp3 = lp2;

                     while(*lp3 && *lp3 != ',') lp3++;  // end of next parm

                     _hmemcpy(tbuf, lp2, lp3 - lp2);    // copy to 'tbuf'
                     tbuf[lp3 - lp2] = 0;

                     lp2 = lp3;
                     if(*lp2) lp2++;
                     while(*lp2 && *lp2<=' ') lp2++;

                     _fstrtrim(tbuf);

                     SetEnvString(tbuf, lp4);

                     lp4 += lstrlen(lp4) + 1;
                  }

                  GlobalFreePtr(lpSaveParms);
               }
            }
         }

         break;  // success or fail, I'm now done!

      }

      PrintErrorMessage(815);
      PrintString(function);
      PrintString(pQCRLF);

      break;
   }

   if(n<pFunctionList[fn].ParmsMin || n>pFunctionList[fn].ParmsMax)
   {
      PrintErrorMessage(816);
      PrintString(function);
      PrintString(pQCRLF);

      break;
   }


   switch(fn)
   {
      case idLEN:       // LEN
         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         lpRval = StringFromInt(lstrlen(lp1),10);
         break;

      case idSTR:       // STR
         if(n==1)
         {
            lpRval = StringFromFloat(CalcEvalNumber(lplpArgs[0]));
         }
         else
         {
            w1 = (WORD)CalcEvalNumber(lplpArgs[1]);
            if(w1==10)
            {
               lpRval = StringFromFloat(CalcEvalNumber(lplpArgs[0]));
            }
            else
            {
               lpRval = StringFromInt((long)CalcEvalNumber(lplpArgs[0]),
                                      (int)CalcEvalNumber(lplpArgs[1]));
            }
         }

         break;

      case idVAL:       // VAL
         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         lpRval = StringFromFloat(CalcEvalNumber(lp1));
         break;


      case idLEFT:       // LEFT
         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         w1 = (WORD)CalcEvalNumber(lplpArgs[1]);

         lpRval = CalcCopyString(lp1, 0, w1);
         break;


      case idRIGHT:       // RIGHT
         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         w1 = (WORD)CalcEvalNumber(lplpArgs[1]);

         w2 = lstrlen(lp1);
         if(w1>w2) w1=w2;

         lpRval = CalcCopyString(lp1, w2 - w1, 0xffff);
         break;


      case idMID:       // MID
         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         w1 = (WORD)CalcEvalNumber(lplpArgs[1]);
         if(n==3) w2 = (WORD)CalcEvalNumber(lplpArgs[2]);
         else     w2 = 0xffff;

         lpRval = CalcCopyString(lp1, w1 - 1, w2);
         break;


      case idUPPER:       // UPPER
         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         lpRval = CalcCopyString(lp1, 0, 0xffff);
         _fstrupr(lpRval);
         break;


      case idLOWER:       // LOWER
         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         lpRval = CalcCopyString(lp1, 0, 0xffff);
         _fstrlwr(lpRval);
         break;



      case idISYES:       // ISYES
         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         while(*lp1 && *lp1<=' ') lp1++;
         w1 = lstrlen(lp1);

         for(lp2 = lp1 + w1; lp2>lp1; lp2--)
         {
            if(*(lp2 - 1)<=' ') *(lp2 - 1) = 0;
            else                break; // exit when non-white-space found
         }

         w1 = min(lstrlen(lp1), 3);

         lpRval = StringFromInt(w1!=0 && !_fstrnicmp(lp1, "YES", w1), 10);
         break;


      case idISNO:       // ISNO
         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         while(*lp1 && *lp1<=' ') lp1++;
         w1 = lstrlen(lp1);

         for(lp2 = lp1 + w1; lp2>lp1; lp2--)
         {
            if(*(lp2 - 1)<=' ') *(lp2 - 1) = 0;
            else                break; // exit when non-white-space found
         }

         w1 = min(lstrlen(lp1), 2);

         lpRval = StringFromInt(w1!=0 && !_fstrnicmp(lp1, "NO", w1), 10);
         break;


      case idISTASK:      // ISTASK
         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         w1 = (WORD)CalcEvalNumber(lp1);

         lpRval = StringFromInt(lpIsTask((HTASK)w1), 10);
         break;


      case idISWINDOW:      // ISWINDOW
         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         w1 = (WORD)CalcEvalNumber(lp1);

         lpRval = StringFromInt(IsWindow((HWND)w1), 10);
         break;


      case idISINSTANCE:      // ISINSTANCE
         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         w1 = (WORD)CalcEvalNumber(lp1);

         lpRval = StringFromInt(GetTaskFromInst((HINSTANCE)w1)!=NULL, 10);
         break;


      case idISMODULE:      // ISMODULE
         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         w1 = (WORD)CalcEvalNumber(lp1);

         lpRval = StringFromInt(GetModuleFileName((HMODULE)w1,tbuf,
                                                  sizeof(tbuf))!=NULL, 10);
         break;




      case idGETWINDOW:    // GETWINDOW  ([arg1 [,ITER]])  returns HWND
      case idGETTASK:      // GETTASK    ([arg1 [,ITER]])  returns HTASK
      case idGETINSTANCE:  // GETINSTANCE([arg1 [,ITER]])  returns HINSTANCE
      case idGETMODULE:    // GETMODULE  ([arg1 [,ITER]])  returns HMODULE


         // arg1 may be one of the following:
         //
         // HWND, HINSTANCE, HTASK, HMODULE, MODULE NAME
         //
         // if not present
         // if not a number, assume MODULE NAME
         // if number, try others (in above order), then MODULE NAME
         //

         if(n>0)
         {
            lp1 = CalcEvalString(lplpArgs[0]);
            if(!lp1) break;
         }
         else
         {
            lp1 = NULL;
         }

         if(n>1)
         {
            lp2 = CalcEvalString(lplpArgs[1]);
            if(!lp2) break;

            w2 = (WORD)CalcEvalNumber(lp2);
         }
         else
         {
            w2 = 0;
         }

         w1 = CalcGetWindowInstTaskModule(lp1, &w3);

         if(!w1 && !w3)   // non-valid module,handle,etc. - return ""
         {
            lpRval = CalcCopyString("",0,0xffff);
            break;
         }


         if(fn==idGETWINDOW)   // GETWINDOW  (arg1 [,ITER])  returns HWND
         {
            // guess what - I get to enumerate top windows!

            if(n==0)      // special case - THIS task!!
            {
               CalcGetWindowInfo(hMainWnd);

               lpRval = CalcCopyString(tbuf, 0, 0xffff);
               break;
            }
            else if(w3==1) // a window - no big deal here!
            {
               CalcGetWindowInfo((HWND)w1);

               lpRval = CalcCopyString(tbuf, 0, 0xffff);
               break;
            }

            lpProc = MakeProcInstance((FARPROC)CalcGetWindowEnum, hInst);

            if(lpProc)
            {
               *tbuf = 0;

               EnumParms.wItem  = w1;      // HINSTANCE, HTASK, or HMODULE
               EnumParms.wIter  = w2;      // the desired iteration count
               EnumParms.wType  = w3;      // which item I have (2, 3, or 4)
               EnumParms.wCount = 0;       // current item I'm on (instanced)

               EnumWindows((WNDENUMPROC)lpProc, (LPARAM)(LPSTR)&EnumParms);

               FreeProcInstance(lpProc);

               if(*tbuf) lpRval = CalcCopyString(tbuf, 0, 0xffff);
               else      lpRval = CalcCopyString("", 0, 0xffff);

               break;
            }


            break;

         }
         else if(fn==idGETTASK)   // GETTASK    (arg1 [,ITER])
         {
            if(w3==1 && w2==0)      // WINDOW
            {
               lpRval = StringFromInt((WORD)
                      GetTaskFromInst((HINSTANCE)
                                      GetWindowWord((HWND)w1, GWW_HINSTANCE))
                                      ,10);
            }
            else if(w3==2 && w2==0) // INSTANCE
            {
               lpRval = StringFromInt((WORD)GetTaskFromInst((HINSTANCE)w1),10);
            }
            else if(w3==3 && w2==0) // TASK
            {
               lpRval = StringFromInt(w1,10);
            }
            else if(w3==4) // MODULE
            {
               _fmemset((LPSTR)&te, 0, sizeof(te));
               te.dwSize = sizeof(te);

               if(!lpTaskFirst(&te)) break;

               do
               {
                  if(te.hModule == (HMODULE)w1)
                  {
                     if(w2==0)
                     {
                        lpRval = StringFromInt((WORD)te.hTask, 10);
                        break;
                     }
                     else
                        w2--;  // countdown to desired entry
                  }
               } while(lpTaskNext(&te));
            }
         }
         else if(fn==idGETINSTANCE)  // GETINSTANCE(arg1 [,ITER])
         {
            if(w3==1 && w2==0)      // WINDOW
            {
               lpRval = StringFromInt((WORD)
                               GetWindowWord((HWND)w1, GWW_HINSTANCE),10);
            }
            else if(w3==2 && w2==0) // INSTANCE
            {
               lpRval = StringFromInt(w1, 10);
            }
            else if(w3==3 && w2==0) // TASK
            {
               _fmemset((LPSTR)&te, 0, sizeof(te));
               te.dwSize = sizeof(te);

               if(lpTaskFindHandle(&te, (HTASK)w1))
               {
                  lpRval = StringFromInt((WORD)te.hInst, 10);
               }
            }
            else if(w3==4) // MODULE
            {
               _fmemset((LPSTR)&te, 0, sizeof(te));
               te.dwSize = sizeof(te);

               if(!lpTaskFirst(&te)) break;

               do
               {
                  if(te.hModule == (HMODULE)w1)
                  {
                     if(w2==0)
                     {
                        lpRval = StringFromInt((WORD)te.hInst, 10);
                        break;
                     }
                     else
                        w2--;  // countdown to desired entry
                  }
               } while(lpTaskNext(&te));
            }
         }
         else if(fn==idGETMODULE)  // GETMODULE  (arg1) returns HMODULE
         {
            if(w2) break;

            if(w3==1)      // HWND - get task handle
            {
               w1 = (WORD)GetTaskFromInst((HINSTANCE)
                                  GetWindowWord((HWND)w1, GWW_HINSTANCE));

               _fmemset((LPSTR)&te, 0, sizeof(te));
               te.dwSize = sizeof(te);

               if(w1 && lpTaskFindHandle(&te, (HTASK)w1))
               {
                  lpRval = StringFromInt((WORD)te.hModule, 10);
               }
            }
            else if(w3==2) // HINST - get task handle
            {
               w1 = (WORD)GetTaskFromInst((HINSTANCE)w1);

               _fmemset((LPSTR)&te, 0, sizeof(te));
               te.dwSize = sizeof(te);

               if(w1 && lpTaskFindHandle(&te, (HTASK)w1))
               {
                  lpRval = StringFromInt((WORD)te.hModule, 10);
               }
            }
            else if(w3==3) // HTASK
            {
               _fmemset((LPSTR)&te, 0, sizeof(te));
               te.dwSize = sizeof(te);

               if(w1 && lpTaskFindHandle(&te, (HTASK)w1))
               {
                  lpRval = StringFromInt((WORD)te.hModule, 10);
               }
            }
            else if(w3==4) // already HMODULE
            {
               lpRval = StringFromInt(w1, 10);
            }
         }

         // for these, never return a NULL value

         if(!lpRval) lpRval = CalcCopyString("",0,0xffff);

         break;



      case idSIN:      // SIN
         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         lpRval = StringFromFloat(sin(CalcEvalNumber(lp1)));
         break;


      case idCOS:      // COS
         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         lpRval = StringFromFloat(cos(CalcEvalNumber(lp1)));
         break;


      case idTAN:      // TAN
         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         lpRval = StringFromFloat(tan(CalcEvalNumber(lp1)));
         break;


      case idASIN:      // ASIN
         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         lpRval = StringFromFloat(asin(CalcEvalNumber(lp1)));
         break;


      case idACOS:      // ACOS
         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         lpRval = StringFromFloat(acos(CalcEvalNumber(lp1)));
         break;


      case idATAN:      // ATAN
         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         lpRval = StringFromFloat(atan(CalcEvalNumber(lp1)));
         break;




      case idLOG:      // LOG
         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         lpRval = StringFromFloat(log10(CalcEvalNumber(lp1)));
         break;


      case idLN:      // LN
         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         lpRval = StringFromFloat(log(CalcEvalNumber(lp1)));
         break;


      case idEXP10:      // EXP10
         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         lpRval = StringFromFloat(pow(10.0, CalcEvalNumber(lp1)));
         break;


      case idEXP:      // EXP
         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         lpRval = StringFromFloat(exp(CalcEvalNumber(lp1)));
         break;




      case idINT:      // INT
         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         v1 = CalcEvalNumber(lp1);

         if(v1>=0 || v1==(long)v1)
         {
            lpRval = StringFromInt((long)v1, 10);
         }
         else
         {
            lpRval = StringFromInt(-((long)(- v1) + 1), 10);
         }

         break;


      case idTRUNC:      // TRUNC
         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         v1 = CalcEvalNumber(lp1);

         if(v1>=0 || v1==(long)v1)
         {
            lpRval = StringFromInt((long)v1, 10);
         }
         else
         {
            lpRval = StringFromInt(-((long)(- v1)), 10);
         }

         break;


      case idROUND:      // ROUND

         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         v1 = CalcEvalNumber(lp1) + .5;

         if(v1>=0 || v1==(long)v1)
         {
            lpRval = StringFromInt((long)v1, 10);
         }
         else
         {
            lpRval = StringFromInt(-((long)(- v1) + 1), 10);
         }

         break;



      case idPAUSE:      // PAUSE

         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         v1 = CalcEvalNumber(lp1);

         dwNow = GetTickCount();

         v2 = 0;

         do
         {
            if((GetTickCount() - dwNow) > 50) MthreadSleep(50);

            if(LoopDispatch() || ctrl_break)
            {
               if(!BatchMode) ctrl_break = FALSE;
               v2 = 1;
               break;
            }

         } while((GetTickCount() - dwNow) < (DWORD)v1);

         lpRval = StringFromInt((long)v2, 10);

         break;     // for now...



      case idINPUT:      // INPUT
         if(n>0)
         {
            lp1 = CalcEvalString(lplpArgs[0]);
            if(!lp1) break;

            PrintString(lp1);
         }
         else
         {
            PrintAChar('?');
            PrintAChar(' ');
         }

         if(n>1)
         {
            lp2 = CalcEvalString(lplpArgs[1]);
            lstrcpy(work_buf, lp2);

            GetUserInput2(work_buf);
         }
         else
         {
            *work_buf = 0;
            GetUserInput(work_buf);
         }

         lpRval = CalcCopyString(work_buf, 0, 0xffff);
         break;


      case idPARSE:   // PARSE - turns a string into a tab-delimited 'array'

         lp1 = CalcEvalString(lplpArgs[0]);

         if(!lp1) break;

         lpRval = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                        lstrlen(lp1) + 2);

         lp3 = lp1;
         lp2 = lpRval;

         do
         {
            // trim leading white-space

            while(*lp3 && *lp3<=' ') lp3++;

            // is this argument 'quoted'?

            if(*lp3=='"')
            {
               lp3++;

               while(*lp3)
               {
                  if(*lp3=='"')
                  {
                     break;
                  }
                  else if(*lp3!='\\')
                  {
                     *(lp2++) = *(lp3++);
                  }
                  else
                  {
                     if(lp3[1]=='"' || lp3[1]=='\'' || lp3[1]=='\\')
                     {
                        lp3++;      // remove the leading '\'
                        *(lp2++) = *(lp3++);
                     }
                     else
                     {
                        *(lp2++) = *(lp3++);
                     }
                  }
               }

               lp3++;
            }
            else if(*lp3=='\'')
            {
               lp3++;

               while(*lp3)
               {
                  if(*lp3=='\'')
                  {
                     break;
                  }
                  else if(*lp3!='\\')
                  {
                     *(lp2++) = *(lp3++);
                  }
                  else
                  {
                     if(lp3[1]=='"' || lp3[1]=='\'' || lp3[1]=='\\')
                     {
                        lp3++;      // remove the leading '\'
                        *(lp2++) = *(lp3++);
                     }
                     else
                     {
                        *(lp2++) = *(lp3++);
                     }
                  }
               }

               lp3++;
            }
            else    // no quotes - copy 'till next white-space or end
            {       // and no special treatment for special characters

               while(*lp3 && *lp3>' ')
               {
                  *(lp2++) = *(lp3++);
               }
            }

            // find next non-white-space

            while(*lp3 && *lp3<=' ') lp3++;

            // if another argument left, append '\t' to destination

            if(*lp3) *(lp2++) = '\t';

            // terminate destination string (for now)

            *lp2 = 0;

         } while(*lp3);

         break;



      case idCENTER:      // CENTER

         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         w1 = lstrlen(lp1);
         if(!w1) return(CalcCopyString("", 0, 0xffff));

         for(lp2=lp1; *lp2 && *lp2<=' '; lp2++)
            ;  // get 1st non-white-space

         for(lp3 = lp2 + lstrlen(lp2); lp3>lp2 && *(lp3 - 1)<=' '; lp3--)
            ;  // get last non-white-space

         lpRval = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE, w1 + 2);
         if(!lpRval) break;


         _fmemset(lpRval, ' ', w1);  // fill it with spaces
         lpRval[w1] = 0;

         if(*(lp3 - 1)>' ')          // check 'lp3' points past end of string
         {
            w2 = (WORD)(lp3 - lp2);  // length of 'actual' string inside

            w3 = (w1 - w2) / 2;      // starting point for center

            _fmemcpy(lpRval + w3, lp2, w2);  // copy string to center
         }

         break;


      case idLJ:      // LJ

         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         w1 = lstrlen(lp1);
         if(!w1) return(CalcCopyString("", 0, 0xffff));

         for(lp2=lp1; *lp2 && *lp2<=' '; lp2++)
            ;  // get 1st non-white-space

         for(lp3 = lp2 + lstrlen(lp2) - 1; lp3>lp2 && *lp3<=' '; lp3--)
            ;  // get last non-white-space

         lpRval = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE, w1 + 2);
         if(!lpRval) break;


         _fmemset(lpRval, ' ', w1);  // fill it with spaces
         lpRval[w1] = 0;

         if(*lp3>' ')
         {
            lp3++;

            w2 = (WORD)(lp3 - lp2);  // length of 'actual' string inside

            w3 = 0;                  // starting point for left

            _fmemcpy(lpRval + w3, lp2, w2);  // copy string to left
         }

         break;


      case idRJ:      // RJ
         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         w1 = lstrlen(lp1);
         if(!w1) return(CalcCopyString("", 0, 0xffff));

         for(lp2=lp1; *lp2 && *lp2<=' '; lp2++)
            ;  // get 1st non-white-space

         for(lp3 = lp2 + lstrlen(lp2) - 1; lp3>lp2 && *lp3<=' '; lp3--)
            ;  // get last non-white-space

         lpRval = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE, w1 + 2);
         if(!lpRval) break;


         _fmemset(lpRval, ' ', w1);  // fill it with spaces
         lpRval[w1] = 0;

         if(*lp3>' ')
         {
            lp3++;

            w2 = (WORD)(lp3 - lp2);  // length of 'actual' string inside

            w3 = w1 - w2;            // starting point for right

            _fmemcpy(lpRval + w3, lp2, w2);  // copy string to right
         }

         break;


      case idLTRIM:      // LTRIM
         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         w1 = lstrlen(lp1);
         if(!w1) return(CalcCopyString("", 0, 0xffff));

         while(*lp1 && *lp1<=' ') lp1++;  // find first non-white-space

         lpRval = CalcCopyString(lp1, 0, 0xffff);

         break;


      case idRTRIM:      // RTRIM

         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         w1 = lstrlen(lp1);
         if(!w1) return(CalcCopyString("", 0, 0xffff));

         for(lp2 = lp1 + w1; lp2>lp1; lp2--)
         {
            if(*(lp2 - 1)<=' ') *(lp2 - 1) = 0;
            else                break; // exit when non-white-space found
         }

         lpRval = CalcCopyString(lp1, 0, 0xffff);

         break;



      case idCHAR:      // CHAR {zero returns '\0'}

         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         w1 = (WORD)CalcEvalNumber(lp1);

         if((char)w1!=0)
         {
            tbuf[0] = (char)w1;
            tbuf[1] = 0;
         }
         else
         {
            tbuf[0] = '\\';
            tbuf[1] = '0';
            tbuf[2] = 0;
         }

         lpRval = CalcCopyString(tbuf, 0, 0xffff);
         break;



      case idASC:      // ASC

         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         lpRval = StringFromInt(((unsigned char)*lp1), 10);
         break;


      case idNOW:      // NOW
         GetSystemDateTime(&dt, &tm);

         v1 = days(&dt) + tm.hour / 24.0
                        + tm.minute/(24.0 * 60.0)
                        + tm.second / (24.0 * 60.0 * 60.0)
                        + tm.tick / (24.0 * 60.0 * 60.0 * 100.0);

         lpRval = StringFromFloat(v1);
         break;


      case idDATE:      // DATE

         if(n==0)
         {
            GetSystemDateTime(&dt, NULL);
         }
         else
         {
            lp1 = CalcEvalString(lplpArgs[0]);
            if(!lp1) break;

            v1 = CalcEvalNumber(lp1);

            Date((long)v1, &dt);
         }

         lpRval = DateStr(&dt);
         break;


      case idDATEVAL:      // DATEVAL

         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         if(atodate(lp1, &dt))
         {
            lpRval = CalcCopyString("",0,0xffff);
         }
         else
         {
            v1 = days(&dt);
            lpRval = StringFromFloat(v1);
         }

         break;


      case idTIME:      // TIME
         if(n==0)
         {
            GetSystemDateTime(NULL, &tm);
         }
         else
         {
            lp1 = CalcEvalString(lplpArgs[0]);
            if(!lp1) break;

            v1 = CalcEvalNumber(lp1);

            v1 -= (long)v1;

            v1 *= 24;
            tm.hour = (char)v1;
            v1 -= (WORD)v1;

            v1 *= 60;
            tm.minute = (char)v1;
            v1 -= (WORD)v1;

            v1 *= 60;
            tm.second = (char)v1;
            v1 -= (WORD)v1;

            tm.tick = (char)(v1 * 100);

         }

         lpRval = TimeStr(&tm);
         break;



      case idTIMEVAL:      // TIMEVAL

         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         GetSystemDateTime(&dt, NULL);

         v1 = days(&dt);

         if(atotime(lp1, &tm))
         {
            lpRval = CalcCopyString("",0,0xffff);
         }
         else
         {
            v1 += tm.hour / 24.0
                  + tm.minute/(24.0 * 60.0)
                  + tm.second / (24.0 * 60.0 * 60.0)
                  + tm.tick / (24.0 * 60.0 * 60.0 * 100.0);

            lpRval = StringFromFloat(v1);
         }

         break;


      case idDOW:      // DOW

         if(n==0)
         {
            GetSystemDateTime(&dt, NULL);
            v1 = days(&dt);
         }
         else
         {
            lp1 = CalcEvalString(lplpArgs[0]);
            if(!lp1) break;

            v1 = CalcEvalNumber(lp1);
         }

         if(v1>=0)
         {
            w1 = (WORD)(((long)v1) % 7);
         }
         else
         {
            w1 = (WORD)(7 + (int)(-((long)v1) % 7));
         }


         lpRval = CalcCopyString(pDOW[w1], 0, 0xffff);
         break;



      case idYEAR:        // YEAR

         if(n==0)
         {
            GetSystemDateTime(&dt, NULL);
         }
         else
         {
            lp1 = CalcEvalString(lplpArgs[0]);
            if(!lp1) break;

            v1 = CalcEvalNumber(lp1);
            Date((long)v1, &dt);  // convert back to a date
         }

         lpRval = StringFromInt(dt.year, 10);
         break;



      case idMONTH:       // MONTH

         if(n==0)
         {
            GetSystemDateTime(&dt, NULL);
         }
         else
         {
            lp1 = CalcEvalString(lplpArgs[0]);
            if(!lp1) break;

            v1 = CalcEvalNumber(lp1);
            Date((long)v1, &dt);  // convert back to a date
         }

         lpRval = StringFromInt(dt.month, 10);
         break;



      case idDAY:         // DAY

         if(n==0)
         {
            GetSystemDateTime(&dt, NULL);
         }
         else
         {
            lp1 = CalcEvalString(lplpArgs[0]);
            if(!lp1) break;

            v1 = CalcEvalNumber(lp1);
            Date((long)v1, &dt);  // convert back to a date
         }

         lpRval = StringFromInt(dt.day, 10);
         break;



      case idHOUR:        // HOUR

         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         v1 = CalcEvalNumber(lp1);

         v1 -= (long)v1;  // gets the fraction portion only

         v1 = floor(24.0 * v1);  // convert to hours

         lpRval = StringFromFloat(v1);
         break;



      case idMINUTE:      // MINUTE

         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         v1 = CalcEvalNumber(lp1);

         v1 -= (long)v1;  // gets the fraction portion only

         v1 = floor(60.0 * 24.0 * v1);    // convert to minutes
         v1 -= 60.0 * (long)(v1 / 60.0);  // subtract hours

         lpRval = StringFromFloat(v1);
         break;



      case idSECOND:      // SECOND
         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         v1 = CalcEvalNumber(lp1);

         v1 -= (long)v1;  // gets the fraction portion only

         v1 *= 24.0 * 60.0 * 60.0;        // convert to seconds

         v1 -= 60.0 * (long)(v1 / 60.0);  // subtract hours & minutes

         // round off result to 2 decimal places, then return value

         lpRval = StringFromFloat(((long)(v1 * 100 + .5)) / 100.0);
         break;



      case idSECONDS:      // SECONDS
         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         v1 = CalcEvalNumber(lp1);

         v1 -= (long)v1;  // gets the fraction portion only

         v1 *= 24.0 * 60.0 * 60.0;  // convert to seconds


         // round off result to 2 decimal places, then return value

         lpRval = StringFromFloat(((long)(v1 * 100 + .5)) / 100.0);
         break;



      case idTASKLIST:  // TASKLIST    ([pattern])        - array of HTASK's
         if(n)
         {
            lp1 = CalcEvalString(lplpArgs[0]);
            if(!lp1) break;
         }
         else
         {
            lp1 = "*";
         }

         _fmemset((LPSTR)&te, 0, sizeof(te));
         te.dwSize = sizeof(te);

         if(!lpTaskFirst(&te)) break;

         w1 = 4096;   // initial size of this thing

         w2 = lstrlen(lp1);


         lp2 = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT, w1);
         if(!lp2) break;

         lp3 = lp2;
         *lp2 = 0;


         do
         {
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


            // see if my pattern matches or not...

            for(w3=0; w3<w2; w3++)
            {
               if(lp1[w3]=='*')
               {
                  w3 = w2;  // makes it look like I'm done, and it matches
                  break;
               }

               if(!te.szModule[w3]) break;      // end of 'compare' string

               if(lp1[w3]!='?' &&
                  toupper(lp1[w3])!=toupper(te.szModule[w3]))
               {
                  break;  // this is *not* a match!
               }
            }

            if(w3<w2) continue;                 // this name does not match

            if(lp3>lp2) *(lp3++) = '\t';

            wsprintf(lp3, "0x%04x", te.hTask);

            lp3 += 6;
            *lp3 = 0;

            if((WORD)(lp3 - lp2) >= (WORD)(w1 - 8))
            {
               w1 += 2048;
               lp3 = (LPSTR)GlobalReAllocPtr(lp2, w1, GMEM_MOVEABLE | GMEM_ZEROINIT);

               if(!lp3)
               {
                  PrintString(pNOTENOUGHMEMORY);
                  GlobalFreePtr(lp2);
                  break;
               }

               lp2 = lp3;
               lp3 += lstrlen(lp2);
            }

         } while(lpTaskNext(&te));

               // attempt to 'shrink down' the block

         lpRval = (LPSTR)GlobalReAllocPtr(lp2, lstrlen(lp2) + 2, GMEM_MOVEABLE);
         if(!lpRval) lpRval = lp2;

         break;


      case idMODULELIST: // MODULELIST  ([pattern])        - array of modules
         if(n)
         {
            lp1 = CalcEvalString(lplpArgs[0]);
            if(!lp1) break;
         }
         else
         {
            lp1 = "*";
         }

         _fmemset((LPSTR)&me, 0, sizeof(me));
         me.dwSize = sizeof(me);

         if(!lpModuleFirst || !lpModuleFirst(&me)) break;

         w1 = 4096;   // initial size of this thing

         w2 = lstrlen(lp1);


         lp2 = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT, w1);
         if(!lp2) break;

         lp3 = lp2;
         *lp2 = 0;


         do
         {
            // see if my pattern matches or not...

            for(w3=0; w3<w2; w3++)
            {
               if(lp1[w3]=='*')
               {
                  w3 = w2;  // makes it look like I'm done, and it matches
                  break;
               }

               if(!me.szModule[w3]) break;      // end of 'compare' string

               if(lp1[w3]!='?' &&
                  toupper(lp1[w3])!=toupper(me.szModule[w3]))
               {
                  break;  // this is *not* a match!
               }
            }

            if(w3<w2) continue;                 // this name does not match

            if(lp3>lp2) *(lp3++) = '\t';

            wsprintf(lp3, "0x%04x", me.hModule);

            lp3 += 6;
            *lp3 = 0;

            if((WORD)(lp3 - lp2) >= (WORD)(w1 - 8))
            {
               w1 += 2048;
               lp3 = (LPSTR)GlobalReAllocPtr(lp2, w1, GMEM_MOVEABLE | GMEM_ZEROINIT);

               if(!lp3)
               {
                  PrintString(pNOTENOUGHMEMORY);
                  GlobalFreePtr(lp2);
                  break;
               }

               lp2 = lp3;
               lp3 += lstrlen(lp2);
            }

         } while(lpModuleNext(&me));

               // attempt to 'shrink down' the block

         lpRval = (LPSTR)GlobalReAllocPtr(lp2, lstrlen(lp2) + 2, GMEM_MOVEABLE);
         if(!lpRval) lpRval = lp2;

         break;



      case idDIRLIST:   // DIRLIST     (pattern[,attrib]) - array of names

         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         if(QualifyPath(tbuf, lp1, TRUE))
         {
            PrintErrorMessage(817);
            PrintString(lp1);
            PrintString(pQCRLFLF);

            break;
         }

         DosQualifyPath(tbuf, tbuf);

         if(n==1)
         {
            w1 = ~_A_HIDDEN & ~_A_VOLID & 0xff;
         }
         else
         {
            lp2 = CalcEvalString(lplpArgs[1]);
            if(!lp2) break;


            w2 = (~_A_VOLID) & 0xff;

            w3 = 0;


            while(*lp2)
            {
               if(*lp2=='-')
               {
                  lp2++;  /* advance pointer if a '-' found... */

                  switch(toupper(*lp2))
                  {
                     case 'H':
                        w2 &= ~_A_HIDDEN;
                        break;

                     case 'S':
                        w2 &= ~_A_SYSTEM;
                        break;

                     case 'D':
                        w2 &= ~_A_SUBDIR;
                        break;

                     case 'A':
                        w2 &= ~_A_ARCH;
                        break;

                     case 'R':
                        w2 &= ~_A_RDONLY;
                        break;
                  }
               }
               else
               {
                  switch(toupper(*lp2))
                  {
                     case 'H':
                        w3 |= _A_HIDDEN;
                        break;

                     case 'S':
                        w3 |= _A_SYSTEM;
                        break;

                     case 'D':
                        w3 |= _A_SUBDIR;
                        break;

                     case 'A':
                        w3 |= _A_ARCH;
                        break;

                     case 'R':
                        w3 |= _A_RDONLY;
                        break;
                  }
               }

               if(*lp2)  lp2++;   /* in case of problem, don't make it worse! */
            }

            w1 = w2 | (w3 << 8); /* low byte is items to include */
                                                /* hi byte is items to match */
         }

         w2 = GetDirList(tbuf, w1, (LPDIR_INFO FAR *)&lp3, NULL);





         lpRval = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                 lstrlen(tbuf) + w2 * 13 + 2);
         if(!lpRval)
         {
            GlobalFreePtr(lp3);
            break;
         }

         // get the directory path only (with ending '\') for file spec

         lp2 = _fstrrchr(tbuf, '\\');
         if(lp2) *(lp2 + 1) = 0;

         lstrcpy(lpRval, tbuf);
         lp1 = lpRval + lstrlen(lpRval);

         for(w1=0; w1<w2; w1++)
         {
            *(lp1++) = '\t';

            _fmemcpy(lp1, ((LPDIR_INFO)lp3)->fullname,
                      sizeof(((LPDIR_INFO)lp3)->fullname));

            lp1[sizeof(((LPDIR_INFO)lp3)->fullname)] = 0;

            lp1 += lstrlen(lp1);  // now points to next element

            lp3 += sizeof(DIR_INFO);
         }

         break;


      case idFILEINFO:   // FILEINFO    (name)             - array of info

         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         w1 = lstrlen(lp1);

         if((w1>=2 && (_fstrcmp(lp1 + w1 - 2, "\\.")==0
                       || _fstrcmp(lp1 + w1 - 2, ":.")==0) ) ||
            (w1==1 && *lp1=='.'))
         {
            if(w1>=2)         // if it's '.', leave it alone
            {
               lp1[w1 - 1] = 0;
            }

            w2 = 1;
         }
         else if((w1>=3 && (_fstrcmp(lp1 + w1 - 3, "\\..")==0
                            || _fstrcmp(lp1 + w1 - 3, ":..")==0) ) ||
                 (w1==2 && *lp1=='.' && lp1[1]=='.'))
         {
            if(w1>=3)         // if it's '..', treat it differently
            {
               lp1[w1 - 2] = 0;
            }
            else
            {
               lp1[1] = 0;    // leaves path as '.'
            }

            w2 = 2;
            DosQualifyPath(tbuf, lp1);
         }
         else
         {
            w2 = 0;
         }

         DosQualifyPath(tbuf, lp1);

         if(!*tbuf)
         {
            lpRval = CalcCopyString("", 0, 0xffff);
            break;
         }

         if(w2==0)        // this is a regular file name
         {
            if(MyFindFirst(tbuf, ~_A_VOLID & 0xff, &ff0))  // file not found??
            {
               lpRval = CalcCopyString("", 0, 0xffff);
               break;
            }
         }
         else
         {
            w1 = lstrlen(tbuf);

            if(tbuf[w1 - 1]!='\\')
            {
               tbuf[w1++] = '\\';
            }

            lstrcpy(tbuf + w1, "*.*");

            if(MyFindFirst(tbuf, _A_SUBDIR, &ff0))  // file not found??
            {
               lpRval = CalcCopyString("", 0, 0xffff);
               break;
            }

            do
            {
               if((w2==1 && !_fstrcmp(ff0.name, ".")) ||
                  (w2==2 && !_fstrcmp(ff0.name, "..")))
               {
                  lstrcpy(tbuf + w1, ff0.name);

                  w2 = 0;
                  break;
               }

            } while(!MyFindNext(&ff0));

            if(w2)
            {
               MyFindClose(&ff0);

               lpRval = CalcCopyString("", 0, 0xffff);
               break;
            }
         }

         // NOTE:  must call 'MyFindClose' before returning to caller


         lpRval = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                 14 + lstrlen(tbuf) + 7 + 16 + 24 + 16
                                 + MAX_PATH + 16 + 16 + 24 + 24 + 7);
         if(!lpRval)
         {
            MyFindClose(&ff0);

            break;
         }

         lp4 = lpRval + 14 + lstrlen(tbuf) + 7 + 16 + 24 + 16;


         lstrcpy(lpRval, ff0.name);   // the FILE NAME field
         lp1 = lpRval + lstrlen(lpRval);

         *(lp1++) = '\t';            // the FULLY QUALIFIED PATH field

         if(IsChicago)
         {
            GetShortName(tbuf, tbuf); // for Chicago, it's the SHORT name
            GetLongName(tbuf, lp4);   // also, store LONG name at 'lp4'
         }
         else
         {
            _fstrupr(tbuf);           // otherwise, all UPPER CASE
            lstrcpy(lp4, tbuf);       // make a copy of the path here
         }

         lstrcpy(lp1, tbuf);
         lp1 += lstrlen(lp1);


         *(lp1++) = '\t';            // the ATTRIBUTE field
         if(ff0.attrib & _A_ARCH)    *(lp1++)='A';
         if(ff0.attrib & _A_RDONLY)  *(lp1++)='R';
         if(ff0.attrib & _A_HIDDEN)  *(lp1++)='H';
         if(ff0.attrib & _A_SYSTEM)  *(lp1++)='S';
         if(ff0.attrib & _A_SUBDIR)  *(lp1++)='D';
         if(ff0.attrib & _A_VOLID)   *(lp1++)='V';


         *(lp1++) = '\t';            // the SIZE field

         if(!(ff0.attrib & (_A_VOLID | _A_SUBDIR))) // none for VOLID/SUBDIR
         {
            wsprintf(lp1, "%ld", ff0.size);
            lp1 += lstrlen(lp1);

         }


         *(lp1++) = '\t';            // the DATE & TIME field

         if(IsChicago)
         {
            lpFD=GetWin32FindData(&ff0); // this is valid until 'MyFindClose()'

            lp3 = (LPSTR)&(lpFD->ftLastWriteTime);

            _asm
            {
               push ds
               mov ax, 0x71a7
               lds si, lp3
               mov bl, 0

               int 0x21          // date is in 'dx', time in 'cx',
               pop ds            // and milliseconds in 2 seconds in bh

               mov al, dl        // day
               and al, 0x1f
               mov dt.day, al

               shr dx, 5         // month
               mov al, dl
               and al, 0x0f
               mov dt.month, al

               shr dx, 4         // year
               add dx, 1980
               mov dt.year, dx

               mov al, cl        // second
               and al, 0x1f
               shl al, 1
               mov tm.second, al

               shr cx, 5         // minute
               mov al, cl
               and al, 0x3f
               mov tm.minute, al

               shr cx, 6         // hour
               mov tm.hour, cl

               mov tm.tick, bh   // # 10ms intervals in 2 seconds
            }

            tm.second += tm.tick / 100;

            tm.tick %= 100;      // # 10ms intervals
         }
         else
         {
            lpFD = NULL;   // this is a flag...

            tm.hour   = (char)((ff0.wr_time & 0xf800) >> 11);
            tm.minute = (char)((ff0.wr_time & 0x07e0) >> 5);
            tm.second = (char)((ff0.wr_time & 0x001f) << 1);
            tm.tick   = 0;

            dt.year   = 1980 + ((ff0.wr_date & 0xfe00) >> 9);
            dt.month  = (char)((ff0.wr_date & 0x01e0) >> 5);
            dt.day    = (char)(ff0.wr_date & 0x001f);
         }

         v1 = days(&dt) + tm.hour / 24.0
                        + tm.minute/(24.0 * 60.0)
                        + tm.second / (24.0 * 60.0 * 60.0)
                        + tm.tick / (24.0 * 60.0 * 60.0 * 100.0);


         // determine compressed file size and file type NOW
         // while 'tbuf' still has name in it.  Also, check to see
         // if the file has been opened by attempting to open it in
         // EXCLUSIVE ACCESS mode...

         // IMPORTANT!  Must open file in 'no modify access date' mode...

         if((ff0.attrib & (_A_VOLID | _A_SUBDIR))) // not for VOLID/SUBDIR
         {
            w3 = FALSE;  // never try to open a subdir or volume ID
         }
         else if(IsChicago)
         {
            // MODE FLAGS:    OPEN_ACCESS_RO_NOMODLASTACCESS = 4
            // ACTION FLAGS:  FILE_OPEN = 1

            if(MyCreateFile(tbuf, 4 | OF_SHARE_EXCLUSIVE, 0, 1,
                            (HFILE FAR *)&w3))
            {
               // file is OPEN!  (error on open command)

               w3 = TRUE;
            }
            else
            {
               // file wasn't open - close NOW!

               _lclose((HFILE)w3);

               w3 = FALSE;
            }
         }
         else
         {
            w3 = (UINT)_lopen(tbuf, READ | OF_SHARE_EXCLUSIVE);

            if(w3 != (UINT)HFILE_ERROR)
            {
               _lclose((HFILE)w3);

               w3 = FALSE;
            }
            else
            {
               w3 = TRUE;
            }
         }

         // NOTE:  'w3' now contains the 'file is open' flag - don't use it!


         if(!(ff0.attrib & (_A_VOLID | _A_SUBDIR))) // none for VOLID/SUBDIR
         {
            dw1 = DblspaceGetCompressedFileSize(tbuf);

            // determine cluster size for the drive the file is on

            w1 = toupper(*tbuf) - 'A' + 1;
            if(w1 >= 1 && w1 <= 26)
            {
               _asm  // get drive allocation information
               {
                  push ds       // this function call eats 'ds'
                  mov ax, 0x1c00
                  mov dx, w1
                  call DOS3Call
                  pop ds

                  mov w1, cx    // bytes per sector
                  mov ah, 0
                  mov w2, ax    // sectors per cluster
               }

               dw2 = (DWORD)w1 * w2;  // size of cluster, in bytes

               dw2 = ((ff0.size + dw2 - 1) / dw2) * dw2;  // 'actual' size
            }

         }
         else
         {
            dw1 = (DWORD)-1L;  // flags "not available" for compressed size
            dw2 = (DWORD)0;    // size not used for VOLID/SUBDIR
         }



         if(ff0.attrib & _A_SUBDIR)
         {
            lp3 = "<DIR>";
         }
         else if(ff0.attrib & _A_VOLID)
         {
            lp3 = "<VOL>";
         }
         else if(!IsProgramFile(tbuf, (LPOFSTRUCT)&ofstr, (int FAR *)&i))
         {
            switch(i)
            {
               case PROGRAM_WIN16:
                  lp3 = "<WIN>";
                  break;

               case PROGRAM_L32:
                  lp3 = ">L32<";     /* guessing */
                  break;

               case PROGRAM_PE:
                  lp3 = "<W32>";     // I'm positive!
                  break;

               case PROGRAM_NONWINDOWS:
                  lp3 = "<DOS>";
                  break;

               case PROGRAM_OS2:
                  lp3 = "<OS2>";
                  break;

               case PROGRAM_OS2_32:
                  lp3 = "OS/32";     /* guessing */
                  break;

               case PROGRAM_VXD:
                  lp3 = "<VXD>";
                  break;

               case PROGRAM_FALSE:
                  lp3 = _fstrrchr(tbuf, '\\');
                  if(!lp3) lp3 = tbuf;

                  lp3 = _fstrchr(lp3, '.');

                  if(lp3 && _fstrnicmp(lp3+1, "COM", 3)==0)
                  {
                     lp3 = "<COM>";
                  }
                  else if(lp3 && _fstrnicmp(lp3+1, "BAT", 3)==0)
                  {
                     lp3 = "<BAT>";
                  }
                  else if(lp3 && _fstrnicmp(lp3+1, "CMD", 3)==0)
                  {
                     lp3 = "<CMD>";
                  }
                  else if(lp3 && _fstrnicmp(lp3+1, "PIF", 3)==0)
                  {
                     lp3 = "<PIF>";
                  }
                  else
                  {
                     lp3 = "";
                  }
                  break;

               default:
                  if(i>0)  lp3 = "<WIN>";
                  else     lp3 = "";
            }
         }
         else
         {
            lp3 = "";
         }


         // add the date/time number to list first, then the file type

         lp2 = StringFromFloat(v1);
         if(!lp2)
         {
            MyFindClose(&ff0);  // this is absolutely necessary!

            GlobalFreePtr(lpRval);
            lpRval = NULL;
            break;
         }

         lstrcpy(lp1, lp2);
         lp1 += lstrlen(lp1);

         GlobalFreePtr(lp2);     // free up memory - not needed now


         *(lp1++) = '\t';            // the FILE (PROGRAM?) TYPE field

         lstrcpy(lp1, lp3);
         lp1 += lstrlen(lp1);


         // LONG FILE NAME goes here (Chicago Only) pointed to by 'lp4'

         *(lp1++) = '\t';            // the LONG FILE NAME (as applicable)
         lstrcpy(lp1, lp4);          // 'lp4' points to long name in buffer


         lp1 += lstrlen(lp1);


         *(lp1++) = '\t';            // FILE SIZE IN CLUSTERS

         if(dw2 != (DWORD)-1L &&
            !(ff0.attrib & (_A_VOLID | _A_SUBDIR))) // none for VOLID/SUBDIR
         {
            wsprintf(lp1, "%ld", dw2);
            lp1 += lstrlen(lp1);
         }


         *(lp1++) = '\t';            // COMPRESSED FILE SIZE (if applicable)

         if(dw1 != (DWORD)-1L &&
            !(ff0.attrib & (_A_VOLID | _A_SUBDIR))) // none for VOLID/SUBDIR
         {
            wsprintf(lp1, "%ld", dw1);
            lp1 += lstrlen(lp1);
         }


         *(lp1++) = '\t';            // LAST ACCESS DATE/TIME

         if(lpFD)
         {
            // convert this value to 'NOW' compatible value...

            lp3 = (LPSTR)&(lpFD->ftLastAccessTime);

            _asm
            {
               push ds
               mov ax, 0x71a7
               lds si, lp3
               mov bl, 0

               int 0x21          // date is in 'dx', time in 'cx',
               pop ds            // and milliseconds in 2 seconds in bh

               mov al, dl        // day
               and al, 0x1f
               mov dt.day, al

               shr dx, 5         // month
               mov al, dl
               and al, 0x0f
               mov dt.month, al

               shr dx, 4         // year
               add dx, 1980
               mov dt.year, dx

               mov al, cl        // second
               and al, 0x1f
               shl al, 1
               mov tm.second, al

               shr cx, 5         // minute
               mov al, cl
               and al, 0x3f
               mov tm.minute, al

               shr cx, 6         // hour
               mov tm.hour, cl

               mov tm.tick, bh   // # 10ms intervals in 2 seconds
            }

            tm.second += tm.tick / 100;

            tm.tick %= 100;      // # 10ms intervals

            v1 = days(&dt) + tm.hour / 24.0
                           + tm.minute/(24.0 * 60.0)
                           + tm.second / (24.0 * 60.0 * 60.0)
                           + tm.tick / (24.0 * 60.0 * 60.0 * 100.0);

            lp2 = StringFromFloat(v1);
            if(!lp2)
            {
               MyFindClose(&ff0);  // this is absolutely necessary!

               GlobalFreePtr(lpRval);
               lpRval = NULL;
               break;
            }

            lstrcpy(lp1, lp2);
            lp1 += lstrlen(lp1);

            GlobalFreePtr(lp2);     // free up memory - not needed now
         }



         *(lp1++) = '\t';            // CREATE DATE/TIME

         if(lpFD)
         {
            // convert this value to 'NOW' compatible value...

            lp3 = (LPSTR)&(lpFD->ftCreationTime);

            _asm
            {
               push ds
               mov ax, 0x71a7
               lds si, lp3
               mov bl, 0

               int 0x21          // date is in 'dx', time in 'cx',
               pop ds            // and milliseconds in 2 seconds in bh

               mov al, dl        // day
               and al, 0x1f
               mov dt.day, al

               shr dx, 5         // month
               mov al, dl
               and al, 0x0f
               mov dt.month, al

               shr dx, 4         // year
               add dx, 1980
               mov dt.year, dx

               mov al, cl        // second
               and al, 0x1f
               shl al, 1
               mov tm.second, al

               shr cx, 5         // minute
               mov al, cl
               and al, 0x3f
               mov tm.minute, al

               shr cx, 6         // hour
               mov tm.hour, cl

               mov tm.tick, bh   // # 10ms intervals in 2 seconds
            }

            tm.second += tm.tick / 100;

            tm.tick %= 100;      // # 10ms intervals

            v1 = days(&dt) + tm.hour / 24.0
                           + tm.minute/(24.0 * 60.0)
                           + tm.second / (24.0 * 60.0 * 60.0)
                           + tm.tick / (24.0 * 60.0 * 60.0 * 100.0);

            lp2 = StringFromFloat(v1);
            if(!lp2)
            {
               MyFindClose(&ff0);  // this is absolutely necessary!

               GlobalFreePtr(lpRval);
               lpRval = NULL;
               break;
            }

            lstrcpy(lp1, lp2);
            lp1 += lstrlen(lp1);

            GlobalFreePtr(lp2);     // free up memory - not needed now
         }


         *(lp1++) = '\t';            // 'FILE OPEN' flag (attempt to open in EXCLUSIVE mode)

         if(w3) *(lp1++) = '1';      // w3 contains 'file is open' status
         else   *(lp1++) = '0';

         *lp1 = 0;



                        // any more?  add them here!



         MyFindClose(&ff0);  // ABSOLUTELY NECESSARY!  (don't forget...)

         break;



      case idMODULEINFO:      // MODULEINFO  (handle)

         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         if((*lp1>='0' && *lp1<='9')  ||     // it's a number, ok?
            (toupper(*lp1)>='A' && toupper(*lp1)<='F' &&
              toupper(lp1[lstrlen(lp1) - 1])=='H'))
         {
            w1 = (WORD)CalcEvalNumber(lp1);        // get numerical value

            if(w1 && lpIsTask((HTASK)w1))
            {
               _fmemset((LPSTR)&te, 0, sizeof(TASKENTRY));
               te.dwSize = sizeof(te);

               if(lpTaskFindHandle(&te, (HTASK)w1))
               {
                  w1 = (WORD)te.hModule;
               }
               else
               {
                  PrintErrorMessage(818);
                  break;      // this is *ACTUALLY* a serious error!
               }
            }
            else if(w1 && GetModuleFileName((HMODULE)w1,tbuf,
                                            sizeof(tbuf))!=NULL)
            {
               w1 = (WORD)CalcGetModuleHandle(tbuf);
            }
            else
            {
               w1 = (WORD)CalcGetModuleHandle(lp1);
            }
         }
         else
         {
            w1 = (WORD)CalcGetModuleHandle(lp1);
         }

         if(!w1)    // non-valid module,handle,etc. - return ""
         {
            lpRval = CalcCopyString("",0,0xffff);
            break;
         }

         _fmemset((LPSTR)&me, 0, sizeof(me));
         me.dwSize = sizeof(me);

         if(!lpModuleFindHandle(&me, (HMODULE)w1))
         {
            PrintErrorMessage(819);
            break;
         }

         lpRval = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                           lstrlen(me.szModule) + lstrlen(me.szExePath) + 32);
         if(!lpRval)
         {
            PrintString(pNOTENOUGHMEMORY);
            break;
         }


            // write the module handle as a HEX number

         wsprintf(lpRval, "0x%x", w1);       // module handle
         lp2 = lpRval + lstrlen(lpRval);

         *(lp2++) = '\t';                    // a TAB to separate them
         lstrcpy(lp2, me.szModule);          // the name of the module
         lp2 += lstrlen(lp2);

         *(lp2++) = '\t';
         lstrcpy(lp2, me.szExePath);         // the actual module file name
         lp2 += lstrlen(lp2);

         wsprintf(lp2, "\t%d", me.wcUsage);  // usage count

         break;     // that's all she wrote, guys!


      case idTASKINFO:      // TASKINFO    (handle)

         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         if((*lp1>='0' && *lp1<='9')  ||     // it's a number, ok?
            (toupper(*lp1)>='A' && toupper(*lp1)<='F' &&
              toupper(lp1[lstrlen(lp1) - 1])=='H'))
         {
            w1 = (WORD)CalcEvalNumber(lp1);        // get numerical value

            if(!w1 || !lpIsTask((HTASK)w1))
            {
               lpRval = CalcCopyString("",0,0xffff);
               break;
            }
         }
         else
         {
            lpRval = CalcCopyString("",0,0xffff);
            break;
         }

         _fmemset((LPSTR)&te, 0, sizeof(te));
         te.dwSize = sizeof(te);

         if(!lpTaskFindHandle(&te, (HTASK)w1))
         {
            PrintErrorMessage(820);
            break;
         }

         lpRval = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                 48 + sizeof(me.szModule)
                                 + sizeof(me.szExePath));
         if(!lpRval)
         {
            PrintString(pNOTENOUGHMEMORY);
            break;
         }


            // write the module handle as a HEX number

         wsprintf(lpRval, "0x%x\t0x%x\t0x%x",
                  w1, (WORD)te.hInst, (WORD)te.hModule);

         lp2 = lpRval + lstrlen(lpRval);

            // get the module name and usage count

         _fmemset((LPSTR)&me, 0, sizeof(me));
         me.dwSize = sizeof(me);

         if(!lpModuleFindHandle(&me, te.hModule))  // error check
         {
            *(lp2++) = '\t';                    // a TAB to separate them
            lstrcpy(lp2, te.szModule);          // the name of the module
            lp2 += lstrlen(lp2);

            break;    // leave 'usage count' and 'file name' entries blank
         }

         *(lp2++) = '\t';                    // a TAB to separate them
         lstrcpy(lp2, me.szModule);          // the name of the module
         lp2 += lstrlen(lp2);


         wsprintf(lp2, "\t%d", me.wcUsage);  // usage count


         *(lp2++) = '\t';                    // a TAB to separate them
         lstrcpy(lp2, me.szExePath);         // the file name of the module
         lp2 += lstrlen(lp2);


         break;     // that's all she wrote, guys!





      case idINSTR:      // INSTR ([int,]string,string)

         if(n==3)
         {
            lp1 = CalcEvalString(lplpArgs[0]);
            if(!lp1) break;

            w1 = (WORD)CalcEvalNumber(lp1);
            if(!w1) w1 = 1; // the default is 1, really!

            lp2 = CalcEvalString(lplpArgs[1]);
            if(!lp2) break;

            lp3 = CalcEvalString(lplpArgs[2]);
            if(!lp3) break;
         }
         else
         {
            w1 = 1;

            lp2 = CalcEvalString(lplpArgs[0]);
            if(!lp2) break;

            lp3 = CalcEvalString(lplpArgs[1]);
            if(!lp3) break;
         }

         if(w1 > (WORD)lstrlen(lp2)) w2 = 0;
         else
         {
            if(lstrlen(lp3))
            {
               lp1 = _fstrstr(lp2 + w1 - 1, lp3);

               if(!lp1) w2 = 0;
               else
               {
                  w2 = (WORD)(lp1 - lp2) + 1;
               }
            }
            else
            {
               w2 = w1;  // zero length 'search' string - return start pos
            }
         }

         lpRval = StringFromInt(w2, 10);
         break;


      case idTRUENAME:      // TRUENAME

         if(n>0)
         {
            lp1 = CalcEvalString(lplpArgs[0]);
            if(!lp1) break;
         }
         else
            lp1 = ".";

         DosQualifyPath(tbuf, lp1);

         lpRval = CalcCopyString(tbuf, 0, 0xffff);
         break;


      case idCHDIR:      // CHDIR

         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         lstrcpy(tbuf, lp1);

         lpRval = StringFromInt(!MyChDir(tbuf), 10);
         break;


      case idCHDRIVE:      // CHDRIVE

         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         *lp1 = toupper(*lp1);

         if(*lp1<'A' || *lp1>'Z' || lp1[1]!=':' || lp1[2])
         {
            PrintErrorMessage(821);
            PrintString(lp1);
            PrintString(pQCRLF);
            break;
         }

         lpRval = StringFromInt(!_chdrive(*lp1 - 'A' + 1), 10);
         break;


      case idGETDIR:     // GETDIR

         if(n>0)
         {
            lp1 = CalcEvalString(lplpArgs[0]);
            if(!lp1) break;

            *lp1 = toupper(*lp1);

            if(*lp1<'A' || *lp1>'Z' || lp1[1]!=':' || lp1[2])
            {
               PrintErrorMessage(821);
               PrintString(lp1);
               PrintString(pQCRLF);
               break;
            }

            w1 = *lp1 - 'A' + 1;
         }
         else
         {
            w1 = 0;
         }

         if(!_lgetdcwd(w1, tbuf, sizeof(tbuf)))
         {
            PrintErrorMessage(822);
            break;
         }

         lpRval = CalcCopyString(tbuf, 0, 0xffff);

         break;


      case idGETDRIVE:     // GETDRIVE

         *tbuf = _getdrive() + 'A' - 1;
         tbuf[1] = ':';
         tbuf[2] = 0;

         lpRval = CalcCopyString(tbuf, 0, 0xffff);

         break;



      case idGETCURSOR:     // GETCURSOR  -  returns an ARRAY as "X,Y"

         wsprintf(tbuf, "%d,%d", curcol, curline);

         lpRval = CalcCopyString(tbuf, 0, 0xffff);

         break;


      case idSETCURSOR:     // SETCURSOR

         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         if(n==1)  // user passed an array - get 2nd half of it!
         {
            for(lp2=lp1; *lp2 && *lp2!=',' && *lp2!='\t'; lp2++)
               ;  // looks for a tab or a comma!

            if(*lp2==',' || *lp2=='\t')
            {
               *(lp2++) = 0;       // terminates first part of string
               while(*lp2 && *lp2<=' ') lp2++;  // trim white space
            }

            // at this point 'lp2' either points to the ending 'NULL' byte
            // (implying a 'y' value of zero) or to the start of the 'y'
            // parameter (if it was found).  'lp1' points to the 'x' parm.
         }
         else
         {
            lp2 = CalcEvalString(lplpArgs[1]);
            if(!lp2) break;
         }

         w1 = (WORD)CalcEvalNumber(lp1);
         w2 = (WORD)CalcEvalNumber(lp2);

         if(MoveCursor(w1, w2))
         {
            break;  // probably won't happen
         }

         lpRval = CalcCopyString("",0, 0xffff);
         break;


      case idGETBKCOLOR:     // GETBKCOLOR

         lpRval = StringFromInt((cur_attr & 0x70)/0x10, 10);
         break;


      case idSETBKCOLOR:     // SETBKCOLOR

         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         w1 = (WORD)CalcEvalNumber(lp1);

         w1 &= 7;  // we can only have value between 0 and 7, inclusive

         cur_attr &= 0x8f;      // trims the color
         cur_attr |= (w1 << 4); // adds in new background color

         lpRval = CalcCopyString("",0, 0xffff);
         break;


      case idGETTEXTCOLOR:     // GETTEXTCOLOR

         lpRval = StringFromInt(cur_attr & 0xf, 10);
         break;


      case idSETTEXTCOLOR:     // SETTEXTCOLOR

         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         w1 = (WORD)CalcEvalNumber(lp1);

         w1 &= 0xf;  // we can only have value between 0 and 0xf, inclusive

         cur_attr &= 0xf0;      // trims the color
         cur_attr |= w1;        // adds in new foreground (text) color

         lpRval = CalcCopyString("",0, 0xffff);
         break;


      case idGETSCREENFONT:

         w2 = (UINT)GetMenu(hMainWnd);

         for(w1=0; w1 < N_DIMENSIONS(uiFontMenuID); w1++)
         {
            if(GetMenuState((HMENU)w2, uiFontMenuID[w1], MF_BYCOMMAND)
               & MF_CHECKED)  // menu item is 'checked'
            {
               if(uiFontValue[w1] != (UINT)-1)
               {
                  lpRval = StringFromInt(uiFontValue[w1], 10);
               }
               else  // CUSTOM font setting
               {
                  if(!hMainFont)  // "default"
                  {
                     lpRval = StringFromInt(0, 10);
                  }
                  else  // for now, nothing...
                  {
                  }
               }

               break;
            }
         }

         if(!lpRval)
         {
            lpRval = StringFromInt(0, 10);
         }

         break;


      case idSETSCREENFONT:

         if(!n)  // no arguments - set DEFAULT font!
         {
            SendMessage(hMainWnd, WM_COMMAND, (WPARAM)IDM_FONT_NORMAL, NULL);

            lpRval = CalcCopyString("",0, 0xffff);
            break;
         }

         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

//         if(!_fstrnicmp(lp1, "CUSTOM", 6))  // custom font setting...
//         {
//          LOGFONT lf;
//
//            // custom font setting ! {reserved}
//
//            // FORMAT:  CUSTOM\tFONT FACE\tHeight\tWidth\tWeight\t
//            //          Italic\tUnderline\tStrikeout
//
//            lp4 = tbuf;
//
//            lp1 += 6;
//
//            if(*lp1 != '\t') break;
//
//            lp1++;
//
//            while(*lp1)
//            {
//               while(*lp1 && *lp1<=' ') lp1++;  // next parameter
//
//               lp2 = _fstrchr(lp1, '\t');       // find next tab (if any)
//
//               if(!lp2) lp2 = lp1 + lstrlen(lp1);
//
//               if(lp2 > lp1) _fmemcpy(lp4, lp1, lp2 - lp1);
//               lp4[lp2 - lp1] = 0;
//
//               lp4 += lstrlen(lp4) + 1;
//               *lp4 = 0;
//
//               if(*lp2) lp2++;
//               lp1 = lp2;
//            }
//
//            // at this point 'tbuf' contains the various parameters, as
//            // NULL-terminated strings.  Scan them.
//
//            lp1 = tbuf;
//
//
//
//
//         }
//         else
//         {
            w1 = (WORD)CalcEvalNumber(lp1);

            if(!w1)
            {
               SendMessage(hMainWnd, WM_COMMAND, (WPARAM)IDM_FONT_NORMAL, NULL);
               lpRval = CalcCopyString("",0, 0xffff);
            }
            else
            {
               for(w2=0; w2 < N_DIMENSIONS(uiFontMenuID); w2++)
               {
                  if(w1 == uiFontValue[w2])
                  {
                     SendMessage(hMainWnd, WM_COMMAND,
                                 (WPARAM)uiFontMenuID[w2], NULL);

                     lpRval = CalcCopyString("",0, 0xffff);
                     break;
                  }
               }
            }
//         }

         break;


      case idPRINTSTRING:     // PRINTSTRING(string)

         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         w1 = curline;

         PrintString(lp1);

         if(w1!=(WORD)curline)            // screen won't update by itself...
         {
            MoveCursor(curcol, curline);  // this forces screen update
         }

         lpRval = CalcCopyString("", 0, 0xffff);
         break;


      case idGETCLIPBOARDDATA:     // GETCLIPBOARDDATA()

         dwNow = GetTickCount();

         while(!OpenClipboard(hMainWnd))
         {
            if(LoopDispatch() ||       // wait for clipboard to be available
               (GetTickCount() - dwNow)>2000L) break;
         }

         if(w1 = (WORD)GetClipboardData(CF_TEXT))
         {
            lp1 = (LPSTR)GlobalLock((HGLOBAL)w1);
            if(lp1)
            {
               lpRval = CalcCopyString(lp1, 0, 0xffff);
               GlobalUnlock((HGLOBAL)w1);
            }
         }
         if(w1 = (WORD)GetClipboardData(CF_OEMTEXT))
         {
            lp1 = (LPSTR)GlobalLock((HGLOBAL)w1);
            if(lp1)
            {
               lpRval = CalcCopyString(lp1, 0, 0xffff);
               GlobalUnlock((HGLOBAL)w1);
            }
         }
         else
         {
            lpRval = CalcCopyString("", 0, 0xffff);
         }

         CloseClipboard();

         break;


      case idSETCLIPBOARDDATA:     // SETCLIPBOARDDATA(string)


         if(n)                     // was an argument specified?
         {
            lp1 = CalcEvalString(lplpArgs[0]);
            if(!lp1) break;

            lp2 = CalcCopyString(lp1, 0, 0xffff);
         }
         else
         {
            lp2 = NULL;
         }

         if(!n || lp2)
         {
            dwNow = GetTickCount();

            while(!OpenClipboard(hMainWnd))
            {
               if(LoopDispatch() ||       // wait for clipboard to be available
                  (GetTickCount() - dwNow)>2000L) break;
            }

            if(lp2 && *lp2)
            {
               w1 = (WORD)GlobalHandle(SELECTOROF(lp2));

               GlobalUnlock((HGLOBAL)w1);


               if(SetClipboardData(CF_TEXT, (HGLOBAL)w1) ||
                  SetClipboardData(CF_OEMTEXT, (HGLOBAL)w1))
               {
                  lpRval = CalcCopyString("", 0, 0xffff);
               }
            }
            else
            {
               EmptyClipboard();

               if(lp2) GlobalFreePtr(lp2);

               lpRval = CalcCopyString("", 0, 0xffff);
            }

            CloseClipboard();
         }

         break;



      case idSHOWWINDOW:     // SHOWWINDOW([hwnd,]mode)

         if(n>1)
         {
            lp1 = CalcEvalString(lplpArgs[1]);
            if(!lp1) break;

            lp2 = CalcEvalString(lplpArgs[0]);

            while(*lp2 && *lp2<=' ') lp2++;

            w1 = lstrlen(lp2);

            for(lp3 = lp2 + w1; w1 && lp3>lp2; lp3--)
            {
               if(*(lp3 - 1)<=' ') *(lp3 - 1) = 0;
               else                break; // exit when non-white-space found
            }

            w2 = (WORD)CalcEvalNumber(lp2);
         }
         else
         {
            w2 = (WORD)hMainWnd;

            lp1 = CalcEvalString(lplpArgs[0]);
            if(!lp1) break;
         }

         while(*lp1 && *lp1<=' ') lp1++;

         w1 = lstrlen(lp1);

         for(lp3 = lp1 + w1; w1 && lp3>lp1; lp3--)
         {
            if(*(lp3 - 1)<=' ') *(lp3 - 1) = 0;
            else                break; // exit when non-white-space found
         }

         if(*lp1 && ((*lp1>='0' && *lp1<='9') || *lp1=='-'))
         {
            w1 = (WORD)CalcEvalNumber(lp1);
         }
         else
         {
            w1 = SW_SHOWNORMAL;        // this is the default method
         }                             // for a 'blank' string

         if(IsWindow((HWND)w2))
         {
            ShowWindow((HWND)w2, w1);

            if(w1==SW_SHOWNORMAL || w1==SW_SHOW || w1==SW_RESTORE)
            {
               if(GetFocus()!=(HWND)w2)  // the focus isn't there??
               {
                  SetFocus((HWND)w2);    // set it there!
               }
            }

            lpRval = CalcCopyString("", 0, 0xffff);
         }

         break;


      case idSENDKEYS:     // SENDKEYS(window,keystring)

         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         lp2 = CalcEvalString(lplpArgs[1]);
         if(!lp2) break;

         while(*lp1 && *lp1<=' ') lp1++;

         w1 = lstrlen(lp1);

         for(lp3 = lp1 + w1; w1 && lp3>lp1; lp3--, w1--)
         {
            if(*(lp3 - 1)<=' ') *(lp3 - 1) = 0;
            else                break; // exit when non-white-space found
         }

         w1 = (WORD)CalcEvalNumber(lp1);

         if(IsWindow((HWND)w1))  // make sure it's a window, first!
         {
            w2 = lstrlen(lp2);
            for(w3=0; w3<w2; w3++)
            {
               PostMessage((HWND)w1, WM_CHAR,
                           (WPARAM)(WORD)((unsigned char)(lp2[w3])),
                           (LPARAM)0x00000001L);

               MthreadSleep(10);                 // ensures that the window can
               if(LoopDispatch()) break;  // try to process the key!!
                                          // (limits to 100cps - oh well)
            }

            MthreadSleep(50);                    // One more time, giving the
            if(LoopDispatch()) break;     // app a chance to use the keystroke
         }


         lpRval = CalcCopyString("", 0, 0xffff);

         break;


      case idSENDVIRTUALKEY: // SENDVIRTUALKEY(window, keycode[, ctrlaltflag])

         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         while(*lp1 && *lp1<=' ') lp1++;

         w1 = lstrlen(lp1);

         for(lp3 = lp1 + w1; w1 && lp3>lp1; lp3--)
         {
            if(*(lp3 - 1)<=' ') *(lp3 - 1) = 0;
            else                break; // exit when non-white-space found
         }

         w1 = (WORD)CalcEvalNumber(lp1);


         lp2 = CalcEvalString(lplpArgs[1]);
         if(!lp2) break;

         while(*lp2 && *lp2<=' ') lp2++;

         w2 = lstrlen(lp2);

         for(lp3 = lp2 + w2; w2 && lp3>lp2; lp3--)
         {
            if(*(lp3 - 1)<=' ') *(lp3 - 1) = 0;
            else                break; // exit when non-white-space found
         }

         w2 = (WORD)CalcEvalNumber(lp2);


         if(n>2)
         {
            lp3 = CalcEvalString(lplpArgs[2]);
            if(!lp3) break;

            while(*lp3 && *lp3<=' ') lp3++;

            w3 = lstrlen(lp3);

            for(lp4 = lp3 + w3; w3 && lp4>lp3; lp4--)
            {
               if(*(lp4 - 1)<=' ') *(lp4 - 1) = 0;
               else                break; // exit when non-white-space found
            }

            w3 = (WORD)CalcEvalNumber(lp3);
         }
         else
         {
            w3 = 0;
         }


         if(IsWindow((HWND)w1))  // make sure it's a window, first!
         {
            // TODO:  Win32 should go EXCLUSIVE at this point, or else
            //        force the target app/thread to be suspended
            //        until we finish this message sequence.

            if(w3==4)            // the 'ALT-ONLY' flag!
            {
               if(w2==0)
               {
                  PostMessage((HWND)w1, WM_SYSKEYDOWN, (WPARAM)VK_MENU,
                              (LPARAM)0x20380001L);
                  PostMessage((HWND)w1, WM_SYSKEYUP, (WPARAM)VK_MENU,
                              (LPARAM)0xc0380001L);
               }
               else
               {
                  PostMessage((HWND)w1, WM_SYSKEYDOWN, (WPARAM)VK_MENU,
                              (LPARAM)0x20380001L);
                  PostMessage((HWND)w1, WM_SYSKEYDOWN, (WPARAM)w2,
                              (LPARAM)0x20000001L);
                  PostMessage((HWND)w1, WM_SYSKEYUP, (WPARAM)w2,
                              (LPARAM)0xc0000001L);
                  PostMessage((HWND)w1, WM_KEYUP, (WPARAM)VK_MENU,
                              (LPARAM)0xc0380001L);
               }
            }
            else
            {
               // at this point, we're in NORMAL (non-syskey) sequence

               if(w3 & 2)       // CTRL!
               {
                  PostMessage((HWND)w1, WM_KEYDOWN, (WPARAM)VK_CONTROL,
                              (LPARAM)0x001d0001L);
               }

               if(w3 & 4)       // ALT!
               {
                  PostMessage((HWND)w1, WM_KEYDOWN, (WPARAM)VK_MENU,
                              (LPARAM)0x20380001L);
               }


               if(w3 & 1)       // SHIFT!
               {
                  PostMessage((HWND)w1, WM_KEYDOWN, (WPARAM)VK_SHIFT,
                              (w3 & 4)?(LPARAM)0x202a0001L:(LPARAM)0x002a0001L);
               }

               if(w2)
               {
                  PostMessage((HWND)w1, WM_KEYDOWN, (WPARAM)w2,
                              (w3 & 4)?(LPARAM)0x20000001L:(LPARAM)0x00000001L);

                  PostMessage((HWND)w1, WM_KEYUP, (WPARAM)w2,
                              (w3 & 4)?(LPARAM)0xe0000001L:(LPARAM)0xc0000001L);
               }


               if(w3 & 1)       // SHIFT!
               {
                  PostMessage((HWND)w1, WM_KEYUP, (WPARAM)VK_SHIFT,
                              (w3 & 4)?(LPARAM)0xe02a0001L:(LPARAM)0xc02a0001L);
               }


               if(w3 & 4)       // ALT!
               {
                  PostMessage((HWND)w1, WM_KEYUP, (WPARAM)VK_MENU,
                              (LPARAM)0xe0380001L);
               }

               if(w3 & 2)       // CTRL!
               {
                  PostMessage((HWND)w1, WM_KEYUP, (WPARAM)VK_CONTROL,
                              (LPARAM)0xc01d0001L);
               }
            }

            MthreadSleep(50);                 // ensures that the window can
            if(LoopDispatch()) break;  // try to process the key!!
         }

         lpRval = CalcCopyString("", 0, 0xffff);

         break;





      case idGETCHILDWINDOW:     // GETCHILDWINDOW(hwnd, iter)

         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         while(*lp1 && *lp1<=' ') lp1++;

         w1 = lstrlen(lp1);

         for(lp3 = lp1 + w1; w1 && lp3>lp1; lp3--)
         {
            if(*(lp3 - 1)<=' ') *(lp3 - 1) = 0;
            else                break; // exit when non-white-space found
         }

         w1 = (WORD)CalcEvalNumber(lp1);


         lp2 = CalcEvalString(lplpArgs[1]);
         if(!lp2) break;

         while(*lp2 && *lp2<=' ') lp2++;

         w2 = lstrlen(lp2);

         for(lp3 = lp2 + w2; w2 && lp3>lp2; lp3--)
         {
            if(*(lp3 - 1)<=' ') *(lp3 - 1) = 0;
            else                break; // exit when non-white-space found
         }

         w2 = (WORD)CalcEvalNumber(lp2);


         if(IsWindow((HWND)w1))
         {
          struct _enum_parms_ {
             WORD wItem;
             WORD wType;
             WORD wIter;
             WORD wCount;
             } EnumParms;

            // guess what - I get to enumerate top windows!

            lpProc = MakeProcInstance((FARPROC)CalcGetWindowEnum, hInst);

            if(lpProc)
            {
               *tbuf = 0;

               EnumParms.wItem  = w1;      // HWND
               EnumParms.wIter  = w2;      // the desired iteration count
               EnumParms.wType  = 0xffff;  // include ALL windows!
               EnumParms.wCount = 0;       // current item I'm on (instanced)

               EnumChildWindows((HWND)w1, (WNDENUMPROC)lpProc,
                                (LPARAM)(LPSTR)&EnumParms);

               FreeProcInstance(lpProc);

               if(*tbuf)
               {
                  lpRval = CalcCopyString(tbuf, 0, 0xffff);

                  break;
               }
            }
         }

         lpRval = CalcCopyString("", 0, 0xffff);

         break;


      case idFILEOPEN:     // FILEOPEN(name, mode) 0=read,1=write,2=append

         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         while(*lp1 && *lp1<=' ') lp1++;

         w1 = lstrlen(lp1);

         for(lp3 = lp1 + w1; w1 && lp3>lp1; lp3--)
         {
            if(*(lp3 - 1)<=' ') *(lp3 - 1) = 0;
            else                break; // exit when non-white-space found
         }

         lp2 = CalcEvalString(lplpArgs[1]);
         if(!lp2) break;

         while(*lp2 && *lp2<=' ') lp2++;

         w2 = lstrlen(lp2);

         for(lp3 = lp2 + w2; w2 && lp3>lp2; lp3--)
         {
            if(*(lp3 - 1)<=' ') *(lp3 - 1) = 0;
            else                break; // exit when non-white-space found
         }

         w2 = (WORD)CalcEvalNumber(lp2);

         lstrcpy(tbuf, lp1);

         if(w2==0)
         {
            f1 = my_fsopen(tbuf, "rt", _SH_DENYNO);
         }
         else if(w2==1)
         {
            f1 = my_fsopen(tbuf, "wt", _SH_DENYNO);
         }
         else if(w2==2)
         {
            f1 = my_fsopen(tbuf, "at", _SH_DENYNO);
         }
         else
         {
            PrintErrorMessage(834);
            break;
         }

         if(!f1)
         {
            PrintErrorMessage(835);
            break;
         }

         lpRval = StringFromInt((DWORD)f1, 10);

         break;



      case idREADLN:     // READLN(file)          {removes <CR><LF>}

         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         while(*lp1 && *lp1<=' ') lp1++;

         w1 = lstrlen(lp1);

         for(lp3 = lp1 + w1; w1 && lp3>lp1; lp3--)
         {
            if(*(lp3 - 1)<=' ') *(lp3 - 1) = 0;
            else                break; // exit when non-white-space found
         }

         f1 = (MYFILE FAR *)((DWORD)CalcEvalNumber(lp1));

         if(!myfgets(tbuf, sizeof(tbuf), f1))
         {
            if(myferror(f1))
            {
               PrintErrorMessage(836);
            }
            else  // on 'end of file' the string is returned as chr(26)
            {
               lpRval = CalcCopyString("\x1a", 0, 0xffff);
            }
         }
         else
         {
            if(tbuf[strlen(tbuf) - 1]=='\n')
            {
               tbuf[strlen(tbuf) - 1] = 0;
            }

            lpRval = CalcCopyString(tbuf, 0, 0xffff);
         }

         break;



      case idWRITELN:     // WRITELN(file,string)  {appends <CR><LF>}

         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         while(*lp1 && *lp1<=' ') lp1++;

         w1 = lstrlen(lp1);

         for(lp3 = lp1 + w1; w1 && lp3>lp1; lp3--)
         {
            if(*(lp3 - 1)<=' ') *(lp3 - 1) = 0;
            else                break; // exit when non-white-space found
         }

         f1 = (MYFILE FAR *)((DWORD)CalcEvalNumber(lp1));


         lp2 = CalcEvalString(lplpArgs[1]);
         if(!lp2) break;

         lstrcpy(tbuf, lp2);
         lstrcat(tbuf, "\n");

         if(myfputs(tbuf, f1)<0)
         {
            PrintErrorMessage(837);
         }
         else
         {
            lpRval = CalcCopyString("", 0, 0xffff);
         }

         break;



      case idFILECLOSE:     // FILECLOSE(file)

         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         while(*lp1 && *lp1<=' ') lp1++;

         w1 = lstrlen(lp1);

         for(lp3 = lp1 + w1; w1 && lp3>lp1; lp3--)
         {
            if(*(lp3 - 1)<=' ') *(lp3 - 1) = 0;
            else                break; // exit when non-white-space found
         }

         f1 = (MYFILE FAR *)((DWORD)CalcEvalNumber(lp1));

         myfclose(f1);

         lpRval = CalcCopyString("", 0, 0xffff);
         break;


      case idISEOF:     // ISEOF(string) - returns TRUE if string==CHR(26)

         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         if(*lp1=='\x1a' && lp1[1]==0) w1 = TRUE;
         else                          w1 = FALSE;

         lpRval = StringFromInt(w1, 10);
         break;



      case idGETPROFILESTRING: // GETPROFILESTRING([profile,] appname, valuename)

         if(n==3)
         {
            lp1 = CalcEvalString(lplpArgs[0]);
            if(!lp1) break;
         }
         else
         {
            lp1 = NULL;  // flags function to use WIN.INI
         }

         lp2 = CalcEvalString(lplpArgs[n - 2]);
         if(!lp2) break;

         lp3 = CalcEvalString(lplpArgs[n - 1]);
         if(!lp3) break;

         if(!*lp3)  // get a list of entries!
         {
            lp4 = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT, 4096);
            if(!lp4)
            {
               PrintErrorMessage(792);
               break;
            }

//            if(*lp2)
//            {
            if(!lp1) GetProfileString(lp2, NULL, "", lp4, 4096);
            else     GetPrivateProfileString(lp2, NULL, "", lp4, 4096, lp1);
//            }
//            else
//            {
//               if(!lp1) GetProfileString(NULL, NULL, "", lp4, 4096);
//               else     GetPrivateProfileString(NULL, NULL, "", lp4, 4096, lp1);
//            }


            lp1=lp4;

            do
            {
               w1 = lstrlen(lp1);
               if(lp1[w1 + 1])    // not the last string...
               {
                  lp1[w1] = '\t'; // change the NULL byte to a tab
               }

               lp1 += w1 + 1;     // point to beginning of 'next' string

            } while(*lp1);

            lpRval = CalcCopyString(lp4, 0, 0xffff);

            GlobalFreePtr(lp4);
            break;
         }
         else
         {
            if(!lp1) GetProfileString(lp2, lp3, "", tbuf, sizeof(tbuf));
            else     GetPrivateProfileString(lp2, lp3, "",
                                            tbuf, sizeof(tbuf), lp1);


            lpRval = CalcCopyString(tbuf, 0, 0xffff);
            break;
         }


      case idWRITEPROFILESTRING: // WRITEPROFILESTRING([profile,] appname, valuename, string)

         if(n==4)
         {
            lp1 = CalcEvalString(lplpArgs[0]);
            if(!lp1) break;
         }
         else
         {
            lp1 = NULL;  // flags function to use WIN.INI
         }

         lp2 = CalcEvalString(lplpArgs[n - 3]);
         if(!lp2) break;

         lp3 = CalcEvalString(lplpArgs[n - 2]);
         if(!lp3) break;

         lp4 = CalcEvalString(lplpArgs[n - 1]);
         if(!lp4) break;

         if(!*lp4)    // lp4 is a blank string
         {

            if(!*lp3) // lp3 is ALSO a blank string
            {
               if(!lp1) WriteProfileString(lp2, NULL, NULL);
               else     WritePrivateProfileString(lp2, NULL, NULL, lp1);
            }
            else
            {
               if(!lp1) WriteProfileString(lp2, lp3, NULL);
               else     WritePrivateProfileString(lp2, lp3, NULL, lp1);
            }
         }
         else
         {
            if(!lp1) WriteProfileString(lp2, lp3, lp4);
            else     WritePrivateProfileString(lp2, lp3, lp4, lp1);
         }

         lpRval = CalcCopyString("",0,0xffff);
         break;


      case idNROWS:     // NROWS(string) - returns # of rows in array

         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         for(w1=0; *lp1; w1++)
         {
            if(lp2 = strqpbrk(lp1, ";\r\n"))
            {
               if(*lp2==';')  lp2++;
               else
               {
                  if(*lp2=='\r') lp2++;
                  if(*lp2=='\n') lp2++;
               }

               lp1 = lp2;
            }
            else
            {
               lp1 += lstrlen(lp1);
            }
         }


         lpRval = StringFromInt(w1, 10);
         break;


      case idNCOLS:     // NCOLS(string) - returns max # of cols in array

         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;


         w2 = 0;

         while(*lp1)
         {
            if(lp2 = strqpbrk(lp1, ";\r\n"))
            {
               if(*lp2==';')  lp2++;
               else
               {
                  if(*lp2=='\r') lp2++;
                  if(*lp2=='\n') lp2++;
               }

               lp3 = lp2;
            }
            else
            {
               lp3 = lp1 + lstrlen(lp1);
            }


            for(w1=0; lp1<lp3; w1++)
            {
               if(lp2 = strqpbrk(lp1, ",\t"))
               {
                  if(lp2>=lp3)
                  {
                     w1++;

                     break;
                  }

                  lp2++;

                  lp1 = lp2;
               }
               else
               {
                  w1++;
                  break;    // end of array, dudes!
               }
            }

            if(w1>w2) w2=w1;

            lp1 = lp3;
         }


         lpRval = StringFromInt(w2, 10);
         break;



      case idFILESMATCH:    // FILESMATCH(string,string) - TRUE if binaries match

         if(n!=2) break;    // this should *NEVER* happen!

         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         lp2 = CalcEvalString(lplpArgs[1]);
         if(!lp2) break;

         w1 = (WORD)_lopen(lp1, READ | OF_SHARE_DENY_NONE);
         if(w1 == (WORD)HFILE_ERROR)
         {
            lpRval = StringFromInt(0, 10);
            break;
         }

         w2 = (WORD)_lopen(lp2, READ | OF_SHARE_DENY_NONE);
         if(w2 == (WORD)HFILE_ERROR)
         {
            _lclose((HFILE)w1);

            lpRval = StringFromInt(0, 10);
            break;
         }

         dw1 = _llseek((HFILE)w1, 0L, SEEK_END);     // sizes different??
         dw2 = _llseek((HFILE)w2, 0L, SEEK_END);

         if(dw1 != dw2)
         {
            _lclose((HFILE)w1);
            _lclose((HFILE)w2);

            lpRval = StringFromInt(0, 10);
            break;
         }

         _llseek((HFILE)w1, 0L, SEEK_SET);
         _llseek((HFILE)w2, 0L, SEEK_SET);

         lp3 = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE, 32768);
         if(!lp3)
         {
            _lclose((HFILE)w1);
            _lclose((HFILE)w2);

            PrintErrorMessage(792);  // not enough memory to perform operation
            break;                   // error!!
         }

         do
         {
            w3 = MyRead((HFILE)w1, lp3, 16384);

            LoopDispatch();

            if(w3 != MyRead((HFILE)w2, lp3 + 16384, 16384))
            {
               GlobalFreePtr(lp3);
               lp3 = NULL;                     // this is a flag, BTW...

               _lclose((HFILE)w1);
               _lclose((HFILE)w2);

               PrintString("?Read error in 'FILESMATCH()'\r\n");

               break;
            }

            if(_fmemcmp(lp3, lp3 + 16384, w3))
            {
               GlobalFreePtr(lp3);
               lp3 = NULL;                     // this is a flag, BTW...

               _lclose((HFILE)w1);
               _lclose((HFILE)w2);

               lpRval = StringFromInt(0, 10);  // file mismatch.....

               break;
            }

         } while(w3 == 16384);

         if(!lp3) break;                 // this is A FLAG!!

         GlobalFreePtr(lp3);

         _lclose((HFILE)w1);
         _lclose((HFILE)w2);

         lpRval = StringFromInt(1, 10);  // files match!  YAY!
         break;



      case idSTRING:  // STRING(count,ASCII code)

         if(n!=2) break;    // this should *NEVER* happen!

         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         lp2 = CalcEvalString(lplpArgs[1]);
         if(!lp2) break;

         w1 = CalcEvalNumber(lp1);
         w2 = CalcEvalNumber(lp2);

         if(!w1) lpRval = NULL;
         else
         {
            lpRval = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE, w1 + 1);

            if(lpRval)
            {
               _hmemset(lpRval, w2, w1);
               lpRval[w1] = 0;             // ensure string is terminated
            }
         }

         break;



      case idFINDWINDOW:      // FINDWINDOW([arg1][,arg2[,arg3]]])
                              // returns array of HWND
      case idFINDCHILDWINDOW: // FINDCHILDWINDOW(arg1,[,arg2[,arg3[,arg4]]])
                              // returns array of HWND

         if(n)
         {
            lp1 = CalcEvalString(lplpArgs[0]);
            if(!lp1) break;
         }
         else  lp1 = NULL;

         if(n>1)
         {
            lp2 = CalcEvalString(lplpArgs[1]);
            if(!lp2) break;
         }
         else    lp2 = NULL;

         if(n>2)
         {
            lp3 = CalcEvalString(lplpArgs[2]);
            if(!lp3) break;
         }
         else    lp3 = NULL;

         if(n>3)
         {
            lp4 = CalcEvalString(lplpArgs[3]);
            if(!lp4) break;
         }
         else    lp4 = NULL;

         // arg1 may be one of the following:
         //
         // HWND, HINSTANCE, HTASK, HMODULE, MODULE NAME
         //
         // if not present
         // if not a number, assume MODULE NAME
         // if number, try others (in above order), then MODULE NAME
         //


         w1 = CalcGetWindowInstTaskModule(lp1, &w2);
         if(!w1 && !w2)
         {
            // return BLANK array

            lpRval = CalcCopyString("",0,0xffff);

            break;
         }

         if(w2==0) break; // an ERROR


         // I need to enumerate windows belonging to a particular task,
         // instance, window handle, or module.


         lpProc = MakeProcInstance((FARPROC)CalcFindWindowEnum, hInst);

         if(lpProc)
         {
            *tbuf = 0;

            EnumParms2.wItem  = w1;      // HINSTANCE, HTASK, or HMODULE
            EnumParms2.wType  = w2;      // which item I have (1,2,3,or 4)
            EnumParms2.wCount = 0;       // current item I'm on
            EnumParms2.wID = w3;         // menu ID (for child windows)

            // 'wIDFlag' bit 0 = 'n>3'; bit 15 = child window

            EnumParms2.wIDFlag = (n > 3)?1:0;

            if(lp2 && *lp2) EnumParms2.lpClass = lp2;
            else            EnumParms2.lpClass = NULL;

            if(lp3 && *lp3) EnumParms2.lpCaption = lp3;
            else            EnumParms2.lpCaption = NULL;

            EnumParms2.lpRval = NULL;    // string into which I return
                                         // the array of window info


            if(fn==idFINDCHILDWINDOW)
            {
               if(w2 != 1)  // not a window handle - I need to get a list
               {            // of top level windows for this task (etc.)

                  EnumParms2.lpProc = lpProc;

                  EnumWindows((WNDENUMPROC)lpProc, (LPARAM)(LPSTR)&EnumParms2);
               }
               else
               {
                  EnumParms2.lpProc = NULL;

                  EnumChildWindows((HWND)w1, (WNDENUMPROC)lpProc,
                                   (LPARAM)(LPSTR)&EnumParms2);
               }
            }
            else
            {
               EnumParms2.lpProc = NULL;  // it's also a flag...

               EnumWindows((WNDENUMPROC)lpProc, (LPARAM)(LPSTR)&EnumParms2);
            }

            FreeProcInstance(lpProc);

            lpRval = EnumParms2.lpRval;

         }

         break;



      case idODBCCONNECTSTRING:

         if(n!=1) break;

         lp1 = CalcEvalString(lplpArgs[0]);

         lp2 = (LPSTR)CMDOdbcGetConnectString(lp1);
         if(lp2) lpRval = CalcCopyString(lp2,0,0xffff);


         break;



      case idODBCGETINFO:

         if(n!=2) break;

         lp1 = CalcEvalString(lplpArgs[0]);
         lp2 = CalcEvalString(lplpArgs[1]);

         if(!lp1 || !lp2) break;

         w2 = CalcEvalNumber(lp2);

         lpRval = CMDOdbcGetInfo(lp1, w2);  // returns 'GlobalAlloc'ed buffer
                                            // containing info, or NULL

         break;



      case idODBCGETTABLES:

         if(n!=1) break;

         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1 || !*lp1) break;

         lpRval = CMDOdbcGetTables(lp1);

         break;



      case idODBCGETDRIVERS:

         if(n!=0) break;

         lpRval = CMDOdbcGetDrivers();

         break;



      case idODBCGETDATA:

         if(n < 1 || n > 2) break;

         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1) break;

         if(n > 1)
         {
            lp2 = CalcEvalString(lplpArgs[1]);

            if(!lp2) break;

            w2 = CalcEvalNumber(lp2);
         }
         else
         {
            w2 = 0;
         }

         lpRval = CMDOdbcGetData(lp1, w2);

         break;



      case idODBCGETCOLUMNINFO:

         if(n < 1 || n > 2) break;

         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1 || !*lp1) break;

         if(n > 1)
         {
            lp2 = CalcEvalString(lplpArgs[1]);
            if(!lp2) break;

            w2 = CalcEvalNumber(lp2);
         }
         else
         {
            w2 = 0;
         }

         lpRval = CMDOdbcGetColumnInfo(lp1, w2);

         break;



      case idODBCGETCOLUMNINDEX:

         if(n != 2) break;

         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1 || !*lp1) break;

         lp2 = CalcEvalString(lplpArgs[1]);
         if(!lp2 || !*lp2) break;

         lpRval = CMDOdbcGetColumnIndex(lp1, lp2);

         break;



      case idODBCGETCURSORNAME:

         if(n != 1) break;

         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1 || !*lp1) break;

         lpRval = CMDOdbcGetCursorName(lp1);

         break;




      //
      // ** REGISTRY KEYS (Win 3.1 *AND* Win '95 compatibility) **
      //

      case idREGDELETEKEY:
         if(n != 1) break;

         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1 || !*lp1) break;


         // find the final '\' in the string to find the key to delete

         lp2 = _fstrrchr(lp1, '\\');
         if(!lp2) break;               // don't allow this situation!

         *(lp2++) = 0;
         if(!*lp2) break;              // subkey must NOT be blank

         // open the registry key

         dw1 = MyRegOpenKey(lp1);  // opens if already there; else create
         if(!dw1)
         {
            break;
         }

         dw2 = lpRegDeleteKey((HKEY)dw1, lp2);

         MyRegCloseKey(dw1);

         if(dw2 == ERROR_SUCCESS)
         {
            lpRval = CalcCopyString("",0,0xffff);
         }
         else  // an error condition - I want the error text!
         {
            lpRval = StringFromInt(dw2, 10);  // return actual error value
         }

         break;



      case idREGDELETEVALUE:
         if(!n || n > 2) break;

         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1 || !*lp1) break;


         if(n == 2)
         {
            lp2 = CalcEvalString(lplpArgs[1]);
            if(!lp2 || !*lp2) break;
         }
         else
         {
            lp2 = "";
         }

         // open the registry key

         dw1 = MyRegOpenKey(lp1);  // opens if already there; else create
         if(!dw1)
         {
            break;
         }


         lpRval = MyRegDeleteValue(dw1, lp2);


         MyRegCloseKey(dw1);


         PrintErrorMessage(823);
         break;


      case idREGENUMKEYS:

         if(n != 1) break;

         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1 || !*lp1) break;


         // open the registry key

         dw1 = MyRegOpenKey(lp1);  // opens if already there; else create
         if(!dw1)
         {
            break;
         }

         lpRval = (LPSTR)GlobalAllocPtr(GHND, 0x10000L);
         if(!lpRval)
            break;

         lp2 = lpRval;

         dw2 = 0;

         while(lpRegEnumKey((HKEY)dw1, dw2++, lp2,
                            0xffffL - (WORD)(lp2 - lpRval))
               == ERROR_SUCCESS)
         {
            lp2 += lstrlen(lp2);
            *(lp2++) = '\t';
         }

         if(lp2 > lpRval)
         {
            // trim the last tab

            *(--lp2) = 0;
         }

         MyRegCloseKey(dw1);

         lp2 = (LPSTR)GlobalReAllocPtr(lpRval, (lp2 - lpRval) + 1, GHND);

         if(lp2)
            lpRval = lp2;  // successful re-alloc


//         PrintErrorMessage(823);
         break;


      case idREGENUMVALUES:

         if(n != 1) break;

         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1 || !*lp1) break;


         // open the registry key

         dw1 = MyRegOpenKey(lp1);  // opens if already there; else create
         if(!dw1)
         {
            break;
         }

         lpRval = MyRegEnumValues((HKEY)dw1);

         MyRegCloseKey(dw1);

         break;


      case idREGQUERYVALUE:

         if(!n || n > 2) break;

         lp1 = CalcEvalString(lplpArgs[0]);
         if(!lp1 || !*lp1) break;

         if(n == 2)
         {
            lp2 = CalcEvalString(lplpArgs[1]);
            if(!lp2 || !*lp2) break;
         }
         else
         {
            lp2 = "";
         }

         // open the registry key

         dw1 = MyRegOpenKey(lp1);  // opens if already there; else create
         if(!dw1)
         {
            break;
         }

         lpRval = MyRegQueryValue((HKEY)dw1, lp2);


         MyRegCloseKey(dw1);


         PrintErrorMessage(823);
         break;


      case idREGSETVALUE:

         if(n < 3 || n > 4) break;

         lp1 = CalcEvalString(lplpArgs[0]);       // key
         if(!lp1 || !*lp1) break;

         lp2 = CalcEvalString(lplpArgs[1]);       // value name
         if(!lp2 || !*lp2) break;

         lp3 = CalcEvalString(lplpArgs[2]);       // data
         if(!lp3 || !*lp3) break;

         if(n == 4)
         {
            lp4 = CalcEvalString(lplpArgs[3]);    // data type
            if(!lp4 || !*lp4) break;

            _fstrtrim(lp4);

            if(!*lp4 || !_fstricmp(lp4, "ASCII"))
            {
               dw4 = REG_SZ;
            }
            else
            {
               if(!IsChicago && !IsNT)  // Windows '95 or NT ONLY!!!!
               {
                  break;  // an error condition
               }

               if(!_fstricmp(lp4, "DWORD"))
               {
                  dw4 = REG_DWORD;
               }
               else if(!_fstricmp(lp4, "EXPAND"))
               {
                  dw4 = REG_EXPAND_SZ;
               }

               // TODO:  add other things here as well

               else
               {
                break;  // ERROR (invalid type, eh?)
               }
            }
         }
         else
         {
            dw4 = REG_SZ;
         }


         // open/create the registry key

         dw1 = MyRegCreateKey(lp1);  // opens if already there; else create
         if(!dw1)
         {
            lpRval = StringFromInt(-1, 10);  // return an error value

         }


         lpRval = MyRegSetValue((HKEY)dw1, lp2, dw4, lp3);



         MyRegCloseKey(dw1);


         PrintErrorMessage(823);
         break;



      //
      // ** NETWORK FUNCTIONS (Win 3.1x *AND* Win '95 compatibility) **
      //

      case idNETGETUNC:    // get 'UNC' name for specified name/device

         if(n != 1) break;  // only 1 argument allowed, and it must be there

         lp1 = CalcEvalString(lplpArgs[0]);       // key
         if(!lp1 || !*lp1) break;

         lp3 = (LPSTR)GlobalAllocPtr(GHND, MAX_PATH);

         DosQualifyPath(lp3, lp1);  // must be fully qualified (when possible)
                                    // and 'canonized' (SUBST or JOIN drives)
                                    // (valid device names will remain 'as-is')
         lp2 = lp3;

         while(*lp2 && *lp2 != '\\')
         {
            if(*lp2 == ':')  // I found a colon!
            {
               lp2++;  // point just past it
               break;
            }

            lp2++;     // find the directory, if it's specified
         }

         if(!*lp2)  // this means the entire device name is probably a 'share' name
         {
            lpRval = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE, MAX_PATH);

            dw1 = MAX_PATH;

            if(lpWNetGetConnection(lp3, lpRval, (UINT FAR *)&dw1)
               != WN_SUCCESS)
            {
               GlobalFreePtr(lpRval);

               lpRval = NULL;  // an error
            }
         }
         else if(lp3[0] != '//' && lp3[1] != '//')
         {
            char c1 = *lp2;

            *lp2 = 0;  // initially, truncate 'lp3' to the device/drive only


            lpRval = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE, MAX_PATH * 2);

            dw1 = MAX_PATH * 2;

            if(lpWNetGetConnection(lp3, lpRval, (UINT FAR *)&dw1)
               != WN_SUCCESS)
            {
               GlobalFreePtr(lpRval);

               lpRval = NULL;  // an error
            }
            else
            {
               *lp2 = c1;  // restore its value

               if(*lpRval && lpRval[lstrlen(lpRval) - 1] == '\\')
                  lp2++;  // if backslash already present, don't add another

               lstrcat(lpRval, lp2);  // assume remaining path is legitimate
            }
         }
         else
         {
            // return the canonized file name

            lpRval = lp3;
            lp3 = NULL;  // so I don't free it (see below)
         }

         if(lp3)
         {
            GlobalFreePtr(lp3);
         }

         break;



      case idNETUSE:   // similar to 'Net Use'
                       // no args returns array of 'USE' info
                       // 1 arg deletes a share
                       // 2 args adds a share

         if(n > 2) break;  // only 2 args allowed

         if(n >= 1)
         {
            lp1 = CalcEvalString(lplpArgs[0]);       // key
            if(!lp1 || !*lp1) break;
         }
         else
         {
            lp1 = NULL;
         }

         if(n == 2)
         {
            lp2 = CalcEvalString(lplpArgs[1]);       // key
            if(!lp2 || !*lp2) break;
         }
         else
         {
            lp2 = NULL;
         }

         // 3 cases:
         //
         // NO argumens - enumerate shared resources
         // 1 argument  - delete shared resource
         // 2 arguments - add shared resource

         if(!lpWNetGetConnection ||
            !lpWNetAddConnection ||
            !lpWNetCancelConnection)
         {
            break;  // if networking not loaded, fail
         }

         if(n == 0)
         {
          static const char CODE_BASED szDevices[][5]={
             "A:","B:","C:","D:","E:","F:","G:","H:","I:","J:","K:","L:","M:",
             "N:","O:","P:","Q:","R:","S:","T:","U:","V:","W:","X:","Y:","Z:",
             "LPT1","LPT2","LPT3","LPT4","COM1","COM2","COM3","COM4"
             };


            lp3 = (LPSTR)GlobalAllocPtr(GHND, MAX_PATH);
            lpRval = (LPSTR)GlobalAllocPtr(GHND, 0xffff);

            lp4 = lpRval;

            for(w1 = 0; w1 < N_DIMENSIONS(szDevices); w1++)
            {
               UINT cbSize = MAX_PATH;
               UINT uiRval = lpWNetGetConnection((LPSTR)szDevices[w1], lp3, &cbSize);

               lp3[cbSize] = 0;

               if(uiRval == WN_SUCCESS)
               {
                  wsprintf(lp4, "%s\t%s\r\n",        // device<TAB>network path<CRLF>
                           (LPSTR)szDevices[w1],
                           (LPSTR)lp3);

                  lp4 += lstrlen(lp4);
               }
            }
         }
         else if(n == 1)  // delete the specified resource
         {
            w1 = lpWNetCancelConnection(lp1, FALSE);  // don't force closure

try_close_again:

            if(w1 != WN_SUCCESS)
            {
//               if(w1 == WN_OPEN_FILES)
//               {
//                  uiRval = lpWNetCancelConnection(lpItem2, TRUE);  // do force closure
//                  goto try_close_again;
//               }
//               else
//               {
                  lpRval = StringFromInt(w1, 10);  // return actual error value
//               }
            }
            else
            {
               lpRval = CalcCopyString("",0,0xffff);
            }
         }
         else if(n == 2)  // add the specified resource
         {
            w1 = lpWNetAddConnection(lp2, "", lp1);

            if(w1 != WN_SUCCESS)
            {
               lpRval = StringFromInt(w1, 10);  // return actual error value
            }
            else
            {
               lpRval = CalcCopyString("",0,0xffff);
            }
         }

         break;



      case idNETVIEW:  // similar to 'Net View', returns array of shares

         if(n > 1) break;  // only 1 argument allowed

         if(n == 1)
         {
            lp1 = CalcEvalString(lplpArgs[0]);       // key
            if(!lp1 || !*lp1) break;
         }
         else
         {
            lp1 = NULL;
         }


         lpRval = DoNetView(lp1);


         break;






      default:

         PrintErrorMessage(823);
         break;

       /* the rest of these are reserved for later */


   }



   END_LEVEL


   // CLEANUP SECTION

   GlobalFreePtr(function);

   for(w=0; w<n; w++)
   {
      GlobalFreePtr(lplpArgs[w]);
      lplpArgs[w] = NULL;
   }

   return(lpRval);

}



LPSTR FAR PASCAL CalcEvaluateArray(LPSTR array, LPSTR FAR *lplpIndex)
{
WORD n, r, c, w, w2;
LPSTR lp1, lp2, lp3, lp4, lpVar, lpRval=NULL;
static char pArrayMsg[]="Array \"";


   for(n=0; lplpIndex[n]; n++)
      ;  // find out dimension of array (only 1 or 2 allowed!)


   INIT_LEVEL

   if(n==0)
   {
      PrintString(pArrayMsg);
      PrintString(array);
      PrintErrorMessage(824);

      break;
   }

   if(n>2)
   {
      PrintString(pArrayMsg);
      PrintString(array);
      PrintErrorMessage(825);

      break;
   }


//   if(!IsNumber(lplpIndex[0]) || (n==2 && !IsNumber(lplpIndex[1])))
//   {
//      PrintString("?Illegal array index\r\n\n");
//
//      return(NULL);
//   }


   lp1 = CalcEvalString(lplpIndex[0]);
   if(!lp1) break;

   r = c = (WORD)CalcEvalNumber(lp1);

   // if it has one index, assume the other is zero (blank)

   if(n==1)
   {
      r=0;
   }
   else
   {

      lp1 = CalcEvalString(lplpIndex[1]);
      if(!lp1) break;

      c = (WORD)CalcEvalNumber(lp1);
   }



   lp1 = GetEnvString(array);
   if(!lp1)               // no environment variable, peoples!
   {
      lpRval = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT, 1);
      if(lpRval) *lpRval = 0;
      else
      {
         PrintString(pNOTENOUGHMEMORY);
      }

      break;
   }

   for(w=0; w<r && *lp1; w++)
   {
      if(lp2 = strqpbrk(lp1, ";\r\n"))
      {
         if(*lp2==';')  lp2++;
         else
         {
            if(*lp2=='\r') lp2++;
            if(*lp2=='\n') lp2++;
         }

         lp1 = lp2;
      }
      else
      {
         lp1 += lstrlen(lp1);
      }
   }

   if(w!=r)
   {
      lpRval = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT, 1);
      if(lpRval) *lpRval = 0;
      else
      {
         PrintString(pNOTENOUGHMEMORY);
      }

      break;
   }

   lp3 = strqpbrk(lp1, ";\r\n");
   if(!lp3) lp3 = lp1 + lstrlen(lp1);

   for(w=0; w<c && lp1<lp3; w++)
   {
      if(lp2 = strqpbrk(lp1, ",\t"))
      {
         if(lp2>=lp3) break;

         lp2++;

         lp1 = lp2;
      }
      else
      {
         break;    // end of array, dudes!
      }
   }

   if(w!=c)
   {
      lpRval = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT, 1);
      if(lpRval) *lpRval = 0;
      else
      {
         PrintString(pNOTENOUGHMEMORY);
      }

      break;
   }


   lp2 = strqpbrk(lp1, ",\t");
   if(!lp2 || lp2>lp3) lp2 = lp3; // stop at end of row, or next ',' or '\t'


   lpRval = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                           (WORD)(lp2 - lp1) + 2);

   for(lp3=lpRval; lp1<lp2; lp1++, lp3++)
   {
      *lp3 = *lp1;
   }

   *lp3 = 0;

   if(*lpRval=='\"' || *lpRval=='\'')  // it begins with a quote...
   {
      for(lp1=lpRval + 1; *lp1; lp1++)
      {
         if(*lp1==*lpRval) break;

         if(*lp1 == '\\')      // it's a backslash - special processing
         {
            if(lp1[1]=='"' || lp1[1]=='\'' || lp1[1]=='\\')
            {
               lp1++;  // skip the character if it's one of the above 3
            }
         }
      }

      *(lp1++) = 0;                     // terminate string
      lstrcpy(lpRval, lpRval + 1);  // slide string back a bit

      for(lp2=lp1; *lp2; lp2++)
      {
         if(*lp2>' ')   // something followed the '"'
         {
            PrintErrorMessage(826);
            PrintString(array);
            PrintErrorMessage(827);
         }
      }
   }


   END_LEVEL


   GlobalFreePtr(array);

   for(w=0; w<n; w++)
   {
      GlobalFreePtr(lplpIndex[w]);
      lplpIndex[w] = NULL;
   }

   return(lpRval);



}



LPSTR FAR PASCAL CalcEvaluateOperator(LPSTR arg1, LPSTR arg2, WORD op)
{
LPSTR lpRval = NULL;
LPSTR lp1, lp2;
double v1,v2,v3;


   INIT_LEVEL

   if(arg1==NULL ||
      (arg2==NULL &&
       (op!=OP_NEGATE && op!=OP_NOT && op!=OP_BITNOT)))
   {
      break;
   }


   switch(op)
   {
      case OP_ERROR:            /* this operator flags an error! */
      case OP_NULL:             /* 'NULL' operator */
         PrintErrorMessage(828);
         break;

      case OP_LEFTPAREN:        /* argument separators */
      case OP_RIGHTPAREN:
      case OP_COMMA:
      case OP_LEFTBRACKET:
      case OP_RIGHTBRACKET:
         PrintErrorMessage(829);
         break;


      case OP_ADD:              /* arithmetic operators */
         v1 = CalcEvalNumber(arg1);
         v2 = CalcEvalNumber(arg2);

         lpRval = StringFromFloat(v1 + v2);
         break;


      case OP_SUBTRACT:
         v1 = CalcEvalNumber(arg1);
         v2 = CalcEvalNumber(arg2);

         lpRval = StringFromFloat(v1 - v2);
         break;


      case OP_MULTIPLY:
         v1 = CalcEvalNumber(arg1);
         v2 = CalcEvalNumber(arg2);

         lpRval = StringFromFloat(v1 * v2);
         break;


      case OP_DIVIDE:
         v1 = CalcEvalNumber(arg1);
         v2 = CalcEvalNumber(arg2);

         if(v2)
            lpRval = StringFromFloat(v1 / v2);
         else
            lpRval = CalcCopyString("0", 0, 0xffff);

         break;

      case OP_MOD:

         v1 = CalcEvalNumber(arg1);
         v2 = CalcEvalNumber(arg2);

         lpRval = StringFromFloat(fmod(v1,v2));

         break;



      case OP_POWER:
         v1 = CalcEvalNumber(arg1);
         v2 = CalcEvalNumber(arg2);

         lpRval = StringFromFloat(pow(v1,v2));
         break;


      case OP_STREQUAL:         /* the '==' operator */

         lp1 = CalcEvalString(arg1);
         lp2 = CalcEvalString(arg2);

         lpRval = StringFromInt(_fstrcmp(lp1, lp2)==0, 10);
         break;

      case OP_SGREATERTHAN:      // '>>'
         lp1 = CalcEvalString(arg1);
         lp2 = CalcEvalString(arg2);

         lpRval = StringFromInt(_fstrcmp(lp1, lp2)>0, 10);
         break;

      case OP_SGREATEREQUAL:     // '>>=='
         lp1 = CalcEvalString(arg1);
         lp2 = CalcEvalString(arg2);

         lpRval = StringFromInt(_fstrcmp(lp1, lp2)>=0, 10);
         break;

      case OP_SLESSTHAN:         // '<<'
         lp1 = CalcEvalString(arg1);
         lp2 = CalcEvalString(arg2);

         lpRval = StringFromInt(_fstrcmp(lp1, lp2)<0, 10);
         break;

      case OP_SLESSEQUAL:        // '<<=='
         lp1 = CalcEvalString(arg1);
         lp2 = CalcEvalString(arg2);

         lpRval = StringFromInt(_fstrcmp(lp1, lp2)<=0, 10);
         break;

      case OP_STRNOTEQUAL:       // '<<>>'
         lp1 = CalcEvalString(arg1);
         lp2 = CalcEvalString(arg2);

         lpRval = StringFromInt(_fstrcmp(lp1, lp2)!=0, 10);
         break;




      case OP_EQUAL:            /* relational operators */

         v1 = CalcEvalNumber(arg1);
         v2 = CalcEvalNumber(arg2);

         lpRval = StringFromInt(v1 == v2, 10);
         break;

      case OP_LESSTHAN:
         v1 = CalcEvalNumber(arg1);
         v2 = CalcEvalNumber(arg2);

         lpRval = StringFromInt(v1 < v2, 10);
         break;

      case OP_GREATERTHAN:
         v1 = CalcEvalNumber(arg1);
         v2 = CalcEvalNumber(arg2);

         lpRval = StringFromInt(v1 > v2, 10);
         break;

      case OP_GREATEREQUAL:
         v1 = CalcEvalNumber(arg1);
         v2 = CalcEvalNumber(arg2);

         lpRval = StringFromInt(v1 >= v2, 10);
         break;

      case OP_LESSEQUAL:
         v1 = CalcEvalNumber(arg1);
         v2 = CalcEvalNumber(arg2);

         lpRval = StringFromInt(v1 <= v2, 10);
         break;

      case OP_NOTEQUAL:
         v1 = CalcEvalNumber(arg1);
         v2 = CalcEvalNumber(arg2);

         lpRval = StringFromInt(v1 != v2, 10);
         break;




      case OP_AND:              /* logical operators */
         v1 = CalcEvalNumber(arg1);
         v2 = CalcEvalNumber(arg2);

         lpRval = StringFromInt(v1 && v2, 10);
         break;


      case OP_OR:
         v1 = CalcEvalNumber(arg1);
         v2 = CalcEvalNumber(arg2);

         lpRval = StringFromInt(v1 || v2, 10);
         break;


      case OP_BITAND:           /* bit-wise logical operators */
         v1 = CalcEvalNumber(arg1);
         v2 = CalcEvalNumber(arg2);

         lpRval = StringFromInt((LONG)v1 & (LONG)v2, 10);
         break;


      case OP_BITOR:
         v1 = CalcEvalNumber(arg1);
         v2 = CalcEvalNumber(arg2);

         lpRval = StringFromInt((LONG)v1 | (LONG)v2, 10);
         break;



      case OP_BITXOR:
         v1 = CalcEvalNumber(arg1);
         v2 = CalcEvalNumber(arg2);

         lpRval = StringFromInt((LONG)v1 ^ (LONG)v2, 10);
         break;



      case OP_CONCAT:           /* concatenation operator */

         lpRval = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                 lstrlen(arg1) + lstrlen(arg2) + 2);

         if(!lpRval) break;

         lstrcpy(lpRval, arg1);
         lstrcat(lpRval, arg2);

         break;


      case OP_NOT:              /* unary operators (immediate) */
      case OP_BITNOT:
      case OP_NEGATE:

         if(arg2)
         {
            PrintErrorMessage(830);
            break;
         }

         lp1 = CalcEvalString(arg1);

         if(op==OP_NOT)
         {
            lpRval = StringFromInt(!((LONG)CalcEvalNumber(arg1)), 10);
         }
         else if(op==OP_BITNOT)
         {
            lpRval = StringFromInt(~((LONG)CalcEvalNumber(arg1)), 10);
         }
         else  // negate, of course
         {
            lpRval = StringFromFloat(-(CalcEvalNumber(arg1)));
         }


   }

   END_LEVEL

   if(arg1) GlobalFreePtr(arg1);
   if(arg2) GlobalFreePtr(arg2);

   return(lpRval);

}


#pragma code_seg()

//****************************************
// DEFAULT CODE SEGMENT FROM HERE FORWARD
//****************************************




BOOL FAR PASCAL CalcAssignVariable(LPSTR varexp, LPSTR lpValue)
{
LPSTR lp1, lp2, lp3, lp4, lpArray, lpVal2;
LPSTR lplpIndex[4];
BOOL commaflag, tabflag, semicolonflag, crlfflag, rval;
WORD w, w1, w2, w3, r, c, nArg=0;
char array_buf[128];  // name of array
static char pARRAYSYNTAXERROR[]="?Syntax error in assignment array specification\r\n";


   // this is a special function which assigns a regular or array
   // variable the value of 'lpValue'


   if(*varexp>='0' && *varexp<='9')
   {
      PrintErrorMessage(832);
      PrintString(varexp);
      PrintErrorMessage(833);
   }

   if(!(lp1 = _fstrchr(varexp, '[')))
   {
      SetEnvString(varexp, lpValue);
      return(FALSE);
   }


   // ok - here it's definitely an array, so I need to parse it correctly

   w1 = (WORD)(lp1 - varexp) + 1;
   w2 = OP_LEFTBRACKET;


   _fstrncpy(array_buf, varexp, (lp1 - varexp));

   array_buf[(WORD)(lp1 - varexp)]=0;
                 /* places the 'value' segment (up to the operator) */
                 /* into 'function_buf'.  Now, find its value!       */

   /** TRIM SPACES **/

   while(*array_buf && *array_buf<=' ')
      lstrcpy(array_buf,array_buf + 1);


   for(lp1 = array_buf + lstrlen(array_buf);
       lp1>array_buf && *(lp1 - 1)<=' '; *(--lp1) = 0)
       ;      /* TRIM ALL TRAILING CONTROL CHARS */


   if(!*array_buf)
   {
      PrintString(pARRAYSYNTAXERROR);

      return(TRUE);
   }

   nArg = 0;  // argument counter for function call

   do
   {
      if(!(lplpIndex[nArg++] = seg_parse(varexp, (SBPWORD)&w1, (SBPWORD)&w2)))
      {               /* evaluate next parameter inside the parentheses */
         for(w=0; nArg && w<(nArg - 1); w++)
         {
            if(lplpIndex[w]) GlobalFreePtr(lplpIndex[w]);
         }

         return(TRUE);
      }


       /* verify that either ')' or ',' was found, and get the next */
       /* parameter if ',';  bail out of loop if ')' (done!)        */

      if(w2!=OP_COMMA && w2!=OP_RIGHTBRACKET)
      {                               /* ensure that there was a ')' */
                                      /* and report the error if not */

         for(w=0; w<nArg; w++)
         {
            if(lplpIndex[w]) GlobalFreePtr(lplpIndex[w]);
         }

         PrintString(pARRAYSYNTAXERROR);

         return(NULL);
      }

   } while(w2==OP_COMMA);        /* evaluate all args until NO COMMA! */

   for(lp2=varexp + w1; *lp2 && *lp2<=' '; lp2++)
      ;  // find next non-white-space

   if(*lp2)  // there is something following, or some error!
   {
      PrintString(pARRAYSYNTAXERROR);

      for(w=0; nArg && w<nArg; w++)
      {
         if(lplpIndex[w]) GlobalFreePtr(lplpIndex[w]);
      }

      return(TRUE);
   }

   if(!nArg || nArg>2)
   {
      PrintErrorMessage(831);

      for(w=0; nArg && w<nArg; w++)
      {
         if(lplpIndex[w]) GlobalFreePtr(lplpIndex[w]);
      }

      return(TRUE);
   }



   INIT_LEVEL

   rval = TRUE;

   lpVal2 = NULL;


   // see if 'lpValue' contains anything that might require quote marks
   // around the array element, and convert it (as needed).

   // note:  because of the possibility of single quotes affecting
   //        'strqpbrk()' I must also put double quotes around any
   //        string containing a single-quote mark.

   if(_fstrpbrk(lpValue, ",'\t\r\n\"")) // need to update the value of 'lpValue'
   {
      lpVal2 = (LPSTR)GlobalAllocPtr(GHND, 2 * lstrlen(lpValue));

      if(!lpVal2)
      {
         PrintString(pNOTENOUGHMEMORY);
         break;
      }

      *lpVal2 = '\"';  // surround in quote marks!

      for(lp1=lpValue, lp2=lpVal2 + 1; *lp1; lp1++, lp2++)
      {
         if(*lp1=='\\')
         {
            *(lp2++) = '\\';
            *lp2 = '\\';
         }
         else if(*lp1=='\"')
         {
            *(lp2++) = '\\';
            *lp2 = '\"';
         }
         else if(*lp1=='\t')
         {
            *(lp2++) = '\\';
            *lp2 = 't';
         }
         else if(*lp1=='\r')
         {
            *(lp2++) = '\\';
            *lp2 = 'r';
         }
         else if(*lp1=='\n')
         {
            *(lp2++) = '\\';
            *lp2 = 'n';
         }
         else
         {
            *lp2 = *lp1;
         }
      }

      *(lp2++) = '\"';
      *lp2 = 0;

      lp1 = (LPSTR)GlobalReAllocPtr(lpVal2, (lp2 - lpVal2) + 1,
                                    GMEM_MOVEABLE);

      if(lp1) lpVal2 = lp1;

      lpValue = lpVal2;
   }



   // ok - next section stolen from 'CalcEvaluateArray()' above to get
   // the location and 'ending' of the array indexed element.


   lp1 = CalcEvalString(lplpIndex[0]);
   if(!lp1) break;

   r = c = (WORD)CalcEvalNumber(lp1);

   // if it has one index, assume the other is zero (blank)

   if(nArg==1)
   {
      r=0;
   }
   else
   {

      lp1 = CalcEvalString(lplpIndex[1]);
      if(!lp1) break;

      c = (WORD)CalcEvalNumber(lp1);
   }



   lp1 = lpArray = GetEnvString(array_buf);

   semicolonflag = FALSE;
   crlfflag = FALSE;

   for(w=0; lpArray && w<r && *lp1; w++)
   {
      if(lp2 = strqpbrk(lp1, ";\r\n"))
      {
         if(*lp2==';')
         {
            lp2++;

            semicolonflag = TRUE;
         }
         else
         {
            if(*lp2=='\r') lp2++;
            if(*lp2=='\n') lp2++;

            crlfflag = TRUE;
         }

         lp1 = lp2;
      }
      else
      {
         lp1 += lstrlen(lp1);
      }
   }

   if(!lpArray || w<r)         // NOT ENOUGH ROWS IN ARRAY - ADD SOME!
   {
      if(lpArray)
      {
         lp2 = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                      lstrlen(lpArray) + 2*(r - w) + c + lstrlen(lpValue) + 6);
      }
      else
      {
         lp2 = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                               2*(r - w) + c + lstrlen(lpValue) + 6);
      }

      if(!lp2)
      {
         PrintString(pNOTENOUGHMEMORY);

         break;
      }

      if(lpArray) lstrcpy(lp2, lpArray);
      else        *lp2 = 0;

      lp3 = lp2 + lstrlen(lp2);

      for(w1=w; w1<r; w1++)
      {
         if(crlfflag || !semicolonflag)
         {
            *(lp3++) = '\r';
            *(lp3++) = '\n';
         }
         else /* if(semicolonflag)  */
         {
            *(lp3++) = ';';
         }
      }

      for(w1=0; w1<c; w1++)
      {
         if(crlfflag || !semicolonflag)
         {
            *(lp3++) = '\t';
         }
         else /* if(semicolonflag) */
         {
            *(lp3++) = ',';
         }
      }


      lstrcpy(lp3, lpValue);

      if(crlfflag || !semicolonflag)
      {
         lstrcat(lp3, "\r\n");  // important!  terminate properly!
      }
      else
      {
         lstrcat(lp3, ";");
      }


      SetEnvString(array_buf, lp2);

      GlobalFreePtr(lp2);

      rval = FALSE;
      break;
   }


   lp3 = strqpbrk(lp1, ";\r\n");
   if(!lp3) lp3 = lp1 + lstrlen(lp1);


   commaflag = semicolonflag || *lp3==';';
   tabflag = crlfflag || *lp3=='\r' || *lp3=='\n';

   for(w=0; w<c && lp1<lp3; w++)
   {
      if(lp2 = strqpbrk(lp1, ",\t"))
      {
         if(*lp2==',') commaflag = TRUE;
         else          tabflag   = TRUE;

         if(lp2>=lp3) break;

         lp2++;

         lp1 = lp2;
      }
      else
      {
         break;    // end of array, dudes!
      }
   }

   if(w<c)        // NOT ENOUGH COLUMNS IN ARRAY - ADD SOME!
   {
      lp2 = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                      lstrlen(lpArray) + (c - w) + lstrlen(lpValue) + 6);
      if(!lp2)
      {
         PrintString(pNOTENOUGHMEMORY);

         break;
      }

      if(lp3>lpArray)
      {
         _hmemcpy(lp2, lpArray, (WORD)(lp3 - lpArray));
      }

      lp1 = lp2 + (WORD)(lp3 - lpArray);  // point at end of elements, right
                                          // on top of the ';' or '\r\n'

      for(w1=w; w1<c; w1++)
      {
         if(tabflag || !commaflag)     *(lp1++) = '\t';
         else /* if(commaflag) */      *(lp1++) = ',';
      }


      lstrcpy(lp1, lpValue);

      if(*lp3)                   // if array already terminates line, use it
      {
         lstrcat(lp2, lp3);      // copy in the remainder of the array!
      }
      else if(crlfflag || (!semicolonflag && !commaflag))
      {
         lstrcat(lp1, "\r\n");   // important!  terminate properly!
      }
      else
      {
         lstrcat(lp1, ";");      // important!  terminate properly!
      }

      SetEnvString(array_buf, lp2);

      GlobalFreePtr(lp2);

      rval = FALSE;
      break;
   }


   // element exists in array.  Find the end of this element

   lp2 = strqpbrk(lp1, ",\t");
   if(!lp2 || lp2>lp3) lp2 = lp3; // stop at end of row, or next ',' or '\t'

   lp3 = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                        lstrlen(lpArray) + lstrlen(lpValue)
                        - ((WORD)(lp2 - lp1)) + 4);
   if(!lp3)
   {
      PrintString(pNOTENOUGHMEMORY);
   }
   else
   {
      if(lp1>lpArray)
      {
         _hmemcpy(lp3, lpArray, (WORD)(lp1 - lpArray));
      }

      lp3[(WORD)(lp1 - lpArray)] = 0;


      lstrcat(lp3, lpValue);
      lstrcat(lp3, lp2);

      SetEnvString(array_buf, lp3);

      GlobalFreePtr(lp3);

      rval = FALSE;
   }






   END_LEVEL

   if(lpVal2) GlobalFreePtr(lpVal2);

   for(w=0; nArg && w<nArg; w++)
   {
      if(lplpIndex[w]) GlobalFreePtr(lplpIndex[w]);
   }

   return(rval);

}


BOOL EXPORT CALLBACK CalcGetWindowEnum(HWND hWnd, LPARAM lParam)
{
struct _enum_parms_ {
   WORD wItem;
   WORD wType;
   WORD wIter;
   WORD wCount;
   } FAR *lpParms;
TASKENTRY te;


   lpParms = (struct _enum_parms_ FAR *)lParam;

   *tbuf = 0;

   switch(lpParms->wType)
   {
      case 2:          // wItem is HINSTANCE

         if(GetWindowWord(hWnd, GWW_HINSTANCE)!=lpParms->wItem)
         {
            return(TRUE);  // NO MATCH!!
         }

         break;


      case 3:

         if(GetWindowTask(hWnd)!=(HTASK)(lpParms->wItem))
         {
            return(TRUE);  // NO MATCH!!
         }

         break;


      case 4:

         _hmemset((LPSTR)&te, 0, sizeof(te));
         te.dwSize = sizeof(te);

         if(!lpTaskFindHandle(&te, GetWindowTask(hWnd))) return(TRUE);

         if(te.hModule!=(HMODULE)(lpParms->wItem))
         {
            return(TRUE);  // NO MATCH!!
         }

         break;



      case 0xffff:

         break;          // for this value, ALL windows match!

      default:
         return(FALSE);  // invalid information (no match) - just quit now!

   }


   // at this point the window handle matches the selection criteria


   if(lpParms->wCount >= lpParms->wIter) // I have reached the desired item!
   {
      CalcGetWindowInfo(hWnd);

      return(FALSE);  // this stops the iterations
   }

   (lpParms->wCount)++;  // increment 'iteration' count

   return(TRUE);         // keep going until done
}



BOOL EXPORT CALLBACK CalcFindWindowEnum(HWND hWnd, LPARAM lParam)
{
LPSTR lp1;
FARPROC lpProc;
TASKENTRY te;
char tbuf2[256];
struct _enum_parms2_ {
   WORD wItem;
   WORD wType;
   WORD wCount;
   WORD wID, wIDFlag;
   LPSTR lpClass, lpCaption;
   LPSTR lpRval;
   FARPROC lpProc;
 } FAR *lpParms;

 // 'wIDFlag' bit 0 = 'n>3'; bit 15 = child window



   lpParms = (struct _enum_parms2_ FAR *)lParam;

   if(!lpParms->lpRval)
   {
      lpParms->lpRval = (LPSTR)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,0x8000);

      if(!lpParms->lpRval) return(FALSE);  // stop NOW!
   }

   if(lpParms->lpProc)  // need to recurse down and get child windows
   {
      lpProc = lpParms->lpProc;
      lpParms->lpProc = NULL;

      lp1 = lpParms->lpRval;
      lpParms->lpRval = NULL;

      EnumChildWindows(hWnd, (WNDENUMPROC)lpProc, lParam);

      if(lpParms->lpRval)
      {
         if(lpParms->lpRval[0])
         {
            if(*lp1) lstrcat(lp1, "\t");

            lstrcat(lp1, lpParms->lpRval);
         }

         GlobalFreePtr(lpParms->lpRval);
      }

      lpParms->lpRval = lp1;
      lpParms->lpProc = lpProc;

      return(TRUE);  // continue enumeration
   }



   lp1 = lpParms->lpRval + lstrlen(lpParms->lpRval);

   switch(lpParms->wType)
   {
      case 1:

         if(GetParent(hWnd) != (HWND)lpParms->wItem)
         {
            return(TRUE);  // NO MATCH!!
         }

         break;


      case 2:          // wItem is HINSTANCE

         if(GetWindowWord(hWnd, GWW_HINSTANCE)!=lpParms->wItem)
         {
            return(TRUE);  // NO MATCH!!
         }

         break;


      case 3:

         if(GetWindowTask(hWnd)!=(HTASK)(lpParms->wItem))
         {
            return(TRUE);  // NO MATCH!!
         }

         break;


      case 4:

         _hmemset((LPSTR)&te, 0, sizeof(te));
         te.dwSize = sizeof(te);

         if(!lpTaskFindHandle(&te, GetWindowTask(hWnd))) return(TRUE);

         if(te.hModule!=(HMODULE)(lpParms->wItem))
         {
            return(TRUE);  // NO MATCH!!
         }

         break;


      case 0xffff:        // ALL windows match!

         break;


      default:
         return(FALSE);  // invalid information (no match) - just quit now!

   }

   // as needed, verify that the window class, window caption, or window ID
   // matches the desired values (when applicable).

   if(lpParms->wIDFlag & 1)
   {
      if(GetWindowWord(hWnd, GWW_ID) != lpParms->wID)
      {
         return(TRUE);  // no match!
      }
   }

   if(lpParms->lpClass)
   {
      GetClassName(hWnd, tbuf2, sizeof(tbuf2));

      if(_fstricmp(tbuf2, lpParms->lpClass))
      {
         return(TRUE);  // no match!
      }
   }

   if(lpParms->lpCaption)
   {
      GetWindowText(hWnd, tbuf2, sizeof(tbuf2));

      if(_fstricmp(tbuf2, lpParms->lpCaption))
      {
         return(TRUE);  // no match!
      }
   }



   // at this point the window handle matches the selection criteria
   // so add the HEX value of the handle to the array, inserting a
   // 'tab' character before it if the array is not empty

   if(lp1 > lpParms->lpRval) *(lp1++) = '\t';
   wsprintf(lp1, "0x%04x", (UINT)hWnd);

   (lpParms->wCount)++;  // increment 'iteration' count

   return(TRUE);         // keep going until done

}


void FAR PASCAL CalcGetWindowInfo(HWND hWnd)
{
WORD wLen, wState;


   *tbuf = 0;
   if(!IsWindow(hWnd)) return;       // blank string if not window


   wsprintf(tbuf, "0x%04x\t\"", (WORD)hWnd);

   wLen = lstrlen(tbuf);

   GetWindowText(hWnd, tbuf + wLen, sizeof(tbuf) - wLen - 1);

   wLen = lstrlen(tbuf);


   if(GetParent(hWnd))                // window has a parent
   {
      wsprintf(tbuf + wLen, "\"\t0x%04x", (WORD)GetParent(hWnd));
      wLen = lstrlen(tbuf);
   }
   else                               // window has *NO* parent
   {
      tbuf[wLen++] = '"';
      tbuf[wLen++] = '\t';
      tbuf[wLen++] = '0';             // just put a zero there!
      tbuf[wLen] = 0;
   }


   if(IsWindowVisible(hWnd))          // calculate the window state
   {
      wState = IsZoomed(hWnd)?3:(IsIconic(hWnd)?2:1);

      // 1 for 'normal', 2 for 'iconic', 3 for 'maximized'
   }
   else
   {
      wState = 0;     // window is hidden (zero)
   }


   wsprintf(tbuf + wLen, "\t%d\t%d\t%d\t0x%04x\t\"",
            wState,                              // 'state' flag
            GetFocus()==hWnd,                    // 'active' flag
            IsWindowEnabled(hWnd),               // 'enabled' flag
            GetWindowWord(hWnd, GWW_HINSTANCE)); // 'instance' handle

   wLen = lstrlen(tbuf);

   GetClassName(hWnd, tbuf + wLen, sizeof(tbuf) - wLen - 1); // class name

   wLen = lstrlen(tbuf);
   tbuf[wLen++] = '\"';
   tbuf[wLen] = 0;



   // one more item (index #8) - child window control ID or menu handle

   wsprintf(tbuf + wLen, "\t0x%04x", (UINT)GetWindowWord(hWnd, GWW_ID));

   wLen = lstrlen(tbuf);



   // additional items go HERE...

}


/***************************************************************************/
/*              REGISTRY AND NETWORK 'HELPER' FUNCTIONS                    */
/***************************************************************************/


DWORD MyRegOpenKey(LPCSTR szKeyName)
{
LPCSTR lpc1;
int i1, i2;
HKEY hKeyRoot, hRval;

static const char CODE_BASED szKeys[][32]=
 {"HKEY_CLASSES_ROOT","HKEY_LOCAL_MACHINE","HKEY_USERS",
  "HKEY_PERFORMANCE_DATA","HKEY_CURRENT_CONFIG","HKEY_DYN_DATA"
 };
static const HKEY CODE_BASED hKeys[]=
 {HKEY_CLASSES_ROOT,HKEY_LOCAL_MACHINE,HKEY_USERS,
  HKEY_PERFORMANCE_DATA,HKEY_CURRENT_CONFIG,HKEY_DYN_DATA
 };


   if(!lpRegOpenKey)
      return(0);


   // FORMAT OF STRING:  HKEY_xxx\key
   //                    \key (assumes HKEY_CLASSES_ROOT)
   //                    key  (assumes HKEY_CLASSES_ROOT)


   hKeyRoot = HKEY_CLASSES_ROOT;  // initial value
   lpc1 = szKeyName;

   // first thing I look for is an 'HKEY' prefix....

   for(i1=0; i1 < N_DIMENSIONS(szKeys); i1++)
   {
      i2 = lstrlen(szKeys[i1]);

      if(!_fstrnicmp(szKeys[i1], szKeyName, i2))
      {
         if(szKeyName[i2] == 0 || szKeyName[i2] == '\\')  // valid!
         {
            hKeyRoot = hKeys[i1];

            lpc1 = szKeyName + i2;

            break;
         }
      }
   }

   if(*lpc1 == '\\')
      lpc1++;  // skip a leading '\'


   if(!IsChicago && !IsNT)
   {
      if(hKeyRoot == HKEY_CLASSES_ROOT)
      {
         hKeyRoot = HKEY_CLASSES_ROOT_WIN31;
      }
      else
      {
         return(0);  // an error
      }
   }

   if(!*lpc1)
   {
      return((DWORD)hKeyRoot);  // this is the key we want to open
   }


   if(lpRegOpenKey(hKeyRoot, lpc1, &hRval) != ERROR_SUCCESS)
   {
      // one last try - see if I can create it

      if(lpRegCreateKey(hKeyRoot, lpc1, &hRval) != ERROR_SUCCESS)
      {
         return(0);  // an error
      }
   }


   return((DWORD)hRval);
}

void MyRegCloseKey(DWORD dwKey)
{
int i1;
static const HKEY CODE_BASED hKeys[]=
 {HKEY_CLASSES_ROOT,HKEY_LOCAL_MACHINE,HKEY_USERS,
  HKEY_PERFORMANCE_DATA,HKEY_CURRENT_CONFIG,HKEY_DYN_DATA
 };


   if(!lpRegCloseKey)
      return;


   // I must not close the "standard" root keys (if they're used)

   if(dwKey == (DWORD)HKEY_CLASSES_ROOT_WIN31)
   {
      return;  // don't close *THIS* key!
   }

   for(i1=0; i1 < N_DIMENSIONS(hKeys); i1++)
   {
      if(dwKey == (DWORD)hKeys[i1])
      {
         return;  // don't close *THIS* key, either!
      }
   }

   lpRegCloseKey((HKEY)dwKey);  // ignore return value
}

DWORD MyRegCreateKey(LPCSTR szKeyName)
{
LPCSTR lpc1;
int i1, i2;
HKEY hKeyRoot, hRval;

static const char CODE_BASED szKeys[][32]=
 {"HKEY_CLASSES_ROOT","HKEY_LOCAL_MACHINE","HKEY_USERS",
  "HKEY_PERFORMANCE_DATA","HKEY_CURRENT_CONFIG","HKEY_DYN_DATA"
 };
static const HKEY CODE_BASED hKeys[]=
 {HKEY_CLASSES_ROOT,HKEY_LOCAL_MACHINE,HKEY_USERS,
  HKEY_PERFORMANCE_DATA,HKEY_CURRENT_CONFIG,HKEY_DYN_DATA
 };


   if(!lpRegCreateKey)
      return(0);


   // FORMAT OF STRING:  HKEY_xxx\key
   //                    \key (assumes HKEY_CLASSES_ROOT)
   //                    key  (assumes HKEY_CLASSES_ROOT)


   hKeyRoot = HKEY_CLASSES_ROOT;  // initial value
   lpc1 = szKeyName;

   // first thing I look for is an 'HKEY' prefix....

   for(i1=0; i1 < N_DIMENSIONS(szKeys); i1++)
   {
      i2 = lstrlen(szKeys[i1]);

      if(!_fstrnicmp(szKeys[i1], szKeyName, i2))
      {
         if(szKeyName[i2] == 0 || szKeyName[i2] == '\\')  // valid!
         {
            hKeyRoot = hKeys[i1];

            lpc1 = szKeyName + i2;

            break;
         }
      }
   }

   if(*lpc1 == '\\')
      lpc1++;  // skip a leading '\'


   if(!IsChicago && !IsNT)
   {
      if(hKeyRoot == HKEY_CLASSES_ROOT)
      {
         hKeyRoot = HKEY_CLASSES_ROOT_WIN31;
      }
      else
      {
         return(0);  // an error
      }
   }

   if(!*lpc1)
   {
      return((DWORD)hKeyRoot);  // this is the key we want to open
   }


   if(lpRegCreateKey(hKeyRoot, lpc1, &hRval) != ERROR_SUCCESS)
   {
      return(0);  // an error
   }


   return((DWORD)hRval);
}

LPSTR MyRegQueryValue(HKEY hKey, LPCSTR szName)
{
LPSTR lp1, lpRval;
DWORD cbSize;


   if(!lpRegQueryValue)
      return(NULL);


   if(IsChicago || IsNT)
   {
      // RegQueryValueEx
   }


   if(*szName)
   {
      // not allowed

      return(NULL);
   }

   lpRval = (LPSTR)GlobalAllocPtr(GHND, 0xffff);
   cbSize = 0xffff;


   if(lpRegQueryValue(hKey, NULL, lpRval, (long FAR *)&cbSize) != ERROR_SUCCESS)
   {
      GlobalFreePtr(lpRval);

      return(NULL);
   }

   // NOTE:  'cbSize' includes NULL terminating char

   lpRval[cbSize] = 0;  // just in case

   lp1 = (LPSTR)GlobalReAllocPtr(lpRval, cbSize + 1, GHND);

   if(lp1)  // it worked
      lpRval = lp1;


   return(lpRval);
}

LPSTR MyRegEnumValues(HKEY hKey)
{
LPSTR lp1, lpRval;
DWORD cbSize;


   if(IsChicago || IsNT)
   {
      // RegQueryValueEx
   }


   lpRval = (LPSTR)GlobalAllocPtr(GHND, 0xffff);
   cbSize = 0xffff;


   if(lpRegQueryValue(hKey, NULL, lpRval, (long FAR *)&cbSize) != ERROR_SUCCESS)
   {
      GlobalFreePtr(lpRval);

      return(NULL);
   }

   // NOTE:  'cbSize' includes NULL terminating char

   lpRval[cbSize] = 0;  // just in case

   lp1 = (LPSTR)GlobalReAllocPtr(lpRval, cbSize + 1, GHND);

   if(lp1)  // it worked
      lpRval = lp1;


   return(lpRval);
}

LPSTR MyRegSetValue(HKEY hKey, LPCSTR szName, DWORD dwType, LPCSTR szValue)
{
LPSTR lpRval;
DWORD cbSize;


   if(IsChicago || IsNT)
   {
      // RegSetValueEx
   }


   if(dwType != REG_SZ || *szName)
   {
      return(NULL);  // I won't do it... I can't!
   }


   cbSize = lstrlen(szValue) + 1;  // includes NULL terminating char

   if(lpRegSetValue(hKey, NULL, dwType, szValue, cbSize)
      == ERROR_SUCCESS)
   {
      lpRval = CalcCopyString("",0,0xffff);
   }
   else
   {
      lpRval = StringFromInt(-1, 10);  // an error
   }

   return(lpRval);
}
LPSTR MyRegDeleteValue(HKEY hKey, LPCSTR szName)
{
LPSTR lpRval;
DWORD cbSize;


   if(!IsChicago && !IsNT)
   {
      return(NULL);
   }


   return(NULL);  // for now...
}



static DWORD NEAR PASCAL DoNetViewProc(LPNETRESOURCE lpRSC, LPSTR FAR *lplpBufPos)
{
DWORD dwWNetOpenEnum, dwWNetEnumResource, dwWNetCloseEnum,
      dwWNetGetLastError;
DWORD dwRval, dwEnum, dwAddr, dw1, dw2;
LPNETRESOURCE lpBuf = NULL;
WORD wMySel = 0;
LPSTR lp1, lp2, lp3;



   // load proc addresses for 'WNetOpenEnum', 'WNetEnumResource', etc.

   dwWNetOpenEnum     = lpGetProcAddress32W(hMPR, "WNetOpenEnumA");
   dwWNetEnumResource = lpGetProcAddress32W(hMPR, "WNetEnumResourceA");
   dwWNetCloseEnum    = lpGetProcAddress32W(hMPR, "WNetCloseEnum");

   dwWNetGetLastError = lpGetProcAddress32W(hMPR, "WNetGetLastErrorA");

   if(!dwWNetOpenEnum || !dwWNetEnumResource ||
      !dwWNetCloseEnum || !dwWNetGetLastError)
   {
      return((DWORD)-1L);  // error
   }

   // allocate buffer for enumeration

   lpBuf = (LPNETRESOURCE)GlobalAllocPtr(GMEM_FIXED | GMEM_ZEROINIT, 0x10000L);
   if(!lpBuf)
   {
      return((DWORD)-1L);  // error
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
      if(dwRval == 1222 ||      // ERROR_NO_NETWORK
         dwRval == 1207 ||      // ERROR_NOT_CONTAINER
         dwRval == 87)          // ERROR_INVALID_PARAMETER
      {
         // dwRval is error code
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

            if(!dwErr)  // only when 'recursing'
            {
               GlobalFreePtr(lpBuf);

               return(0);  // assume "not an error" and continue
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

            dwRval = dwErr;
         }
      }

      GlobalFreePtr(lpBuf);

      return(dwRval);  // error code
   }


             /** Allocate an LDT selector via DPMI **/

   wMySel = MyAllocSelector();

   if(!wMySel)
   {
      PrintErrorMessage(618);

      lpCallProcEx32W(CPEX_DEST_STDCALL | 1, 0, dwWNetCloseEnum, dwEnum);

      return((DWORD)-1);
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
                  // ignore this

                  continue;
               }
               else if(!lpR->lpProvider)
               {
                  // when the provider name is NULL, I can't enumerate it

                  continue;  // just skip THIS entry...
               }
               else if(DoNetViewProc(lpR, lplpBufPos))
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
               lp2 = *lplpBufPos;

               dwAddr = (DWORD)lpR->lpProvider;
               if(dwAddr)
               {
                  MyChangeSelector(wMySel, dwAddr, 0xffff);  // assume it works
                  lstrcpy(lp2, lp1);
                  lp2 += lstrlen(lp2);
               }
               *(lp2++) = '\t';

               dwAddr = (DWORD)lpR->lpRemoteName;
               MyChangeSelector(wMySel, dwAddr, 0xffff);  // assume it works

               lstrcpy(lp2, lp1);
               lp2 += lstrlen(lp2);
               *(lp2++) = '\t';
               *(lp2++) = '\t';  // blank column for the server name

               // "comment" (in case it's NULL, I check the value)

               dwAddr = (DWORD)lpR->lpComment;
               if(dwAddr)
               {
                  MyChangeSelector(wMySel, dwAddr, 0xffff);
                  lstrcpy(lp2, lp1);
                  lp2 += lstrlen(lp2);
               }

               lstrcpy(lp2, pCRLF);
               lp2 += lstrlen(lp2);

               *lplpBufPos = lp2;      // new pointer to end of data

               // now that I have the workgroup entry,
               // add the things that are contained within

               if(dwRval = DoNetViewProc(lpR, lplpBufPos))
               {
                  GlobalPageUnlock(GlobalPtrHandle(lpBuf));

                  MyFreeSelector(wMySel); // free LDT selector

                  GlobalFreePtr(lpBuf);

                  lpCallProcEx32W(CPEX_DEST_STDCALL | 1, 0, dwWNetCloseEnum, dwEnum);

                  return(dwRval);  // what we do on error
               }
            }
            else
            {
               lp1 = (LPSTR)MAKELP(wMySel, 0);


               // if it's a server name, I display the whole thing.
               // If it's a share name, I do it a bit differently...

               if(lpR->dwUsage & RESOURCEUSAGE_CONTAINER)  // servers
               {
                  lp2 = *lplpBufPos;

                  dwAddr = (DWORD)lpR->lpProvider;
                  if(dwAddr)
                  {
                     MyChangeSelector(wMySel, dwAddr, 0xffff);  // assume it works
                     lstrcpy(lp2, lp1);
                     lp2 += lstrlen(lp2);
                  }

                  *(lp2++) = '\t';

                  // workgroup name goes here - get this from 'lpRSC'

                  dwAddr = (DWORD)lpRSC->lpRemoteName;
                  MyChangeSelector(wMySel, dwAddr, 0xffff);  // assume it works

                  lstrcpy(lp2, lp1);
                  lp2 += lstrlen(lp2);
                  *(lp2++) = '\t';

                  // now, server name

                  dwAddr = (DWORD)lpR->lpRemoteName;
                  MyChangeSelector(wMySel, dwAddr, 0xffff);  // assume it works

                  lstrcpy(lp2, lp1);
                  lp2 += lstrlen(lp2);
                  *(lp2++) = '\t';

                  // "comment" (in case it's NULL, I check the value)

                  dwAddr = (DWORD)lpR->lpComment;
                  if(dwAddr)
                  {
                     MyChangeSelector(wMySel, dwAddr, 0xffff);
                     lstrcpy(lp2, lp1);
                     lp2 += lstrlen(lp2);
                  }

                  lstrcpy(lp2, pCRLF);
                  lp2 += lstrlen(lp2);

                  *lplpBufPos = lp2;      // new pointer to end of data
               }
               else
               {
                  lp2 = *lplpBufPos;

                  // find the 1st backslash after the server name

                  dwAddr = (DWORD)lpR->lpRemoteName;
                  MyChangeSelector(wMySel, dwAddr, 0xffff);  // assume it works

                  lp3 = lp1;

                  if(*lp1 == '\\' && lp1[1] == '\\')
                  {
                     lp3 += 2;
                     while(*lp3 && *lp3 != '\\')
                        lp3++;

                     if(!*lp3)  // just in case
                        lp3 = lp1;              // restore it if none found
                     else // if(*lp3 == '\\')
                        lp3++;        // point past the backslash - resource name!
                  }

                  // at this point, 'lp3' points to the resource name
                  // and 'lp1' is the server + resource name (including '\')


                  if(lp3 != lp1)
                  {
                     _hmemcpy(lp2, lp1, (lp3 - lp1) - 1);
                     lp2 += (lp3 - lp1) - 1;
                  }

                  *(lp2++) = '\t';

                  lstrcpy(lp2, lp3);
                  lp2 += lstrlen(lp2);

                  *(lp2++) = '\t';

                  // next is the device type - either 'Disk' or 'Printer'

                  if(lpR->dwType & RESOURCETYPE_DISK)
                  {
                     lstrcpy(lp2, "Disk");
                  }
                  else if(lpR->dwType & RESOURCETYPE_PRINT)
                  {
                     lstrcpy(lp2, "Printer");
                  }
                  else
                  {
                     lstrcpy(lp2, "Unknown");
                  }

                  lp2 += lstrlen(lp2);
                  *(lp2++) = '\t';

                  // "comment" (in case it's NULL, I check the value)

                  dwAddr = (DWORD)lpR->lpComment;
                  if(dwAddr)
                  {
                     MyChangeSelector(wMySel, dwAddr, 0xffff);
                     lstrcpy(lp2, lp1);
                     lp2 += lstrlen(lp2);
                  }

                  lstrcpy(lp2, pCRLF);
                  lp2 += lstrlen(lp2);

                  *lplpBufPos = lp2;      // new pointer to end of data
               }
            }
         }
      }

   } while(dwRval == 0);


   if(dwRval != 259 /* ERROR_NO_MORE_ITEMS */)
   {

      if(dwRval == 1222 ||      // ERROR_NO_NETWORK
         dwRval == 1207 ||      // ERROR_NOT_CONTAINER
         dwRval == 87)          // ERROR_INVALID_PARAMETER
      {
         // dwRval is error code
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

            dwRval = dwErr;
         }
      }


      GlobalPageUnlock(GlobalPtrHandle(lpBuf));

      MyFreeSelector(wMySel); // free LDT selector

      GlobalFreePtr(lpBuf);

      lpCallProcEx32W(CPEX_DEST_STDCALL | 1, 0, dwWNetCloseEnum, dwEnum);

      return(dwRval);  // error code
   }



   // CLEANUP

   GlobalPageUnlock(GlobalPtrHandle(lpBuf));

   MyFreeSelector(wMySel); // free LDT selector

   GlobalFreePtr(lpBuf);

   lpCallProcEx32W(CPEX_DEST_STDCALL | 1, 0, dwWNetCloseEnum, dwEnum);


   return(0);  // no error
}

LPSTR DoNetView(LPCSTR szArg)
{
LPSTR lpRval, lp1;
DWORD dw1;
NETRESOURCE rsc;

  if(!IsChicago && !IsNT)
  {
     return(NULL);  // not supported on WfWg or Win 3.x
  }


  // if szArg is NULL, do a view of the entire network
  // if szArg is a workgroup name, do a view of that workgroup
  // if szArg is a server name, do a view of that server

  // The columns are as follows:
  //
  // For entire network enum:  (workgroup gets entry with blank server name)
  //
  // Service Provider, Workgroup, Server, Comment
  //
  // For workgroup enum: (no special entry for workgroup)
  //
  // Service Provider, Workgroup, Server, Comment
  //
  // For server enum:
  //
  // Server, Resource, Resource Type, Comment


  lpRval = (LPSTR)GlobalAllocPtr(GHND, 0xffff);

  if(!lpRval)
     return(NULL);  // an error

  lp1 = lpRval;

  if(!szArg || !*szArg)
  {

     dw1 = DoNetViewProc(NULL, &lp1);
  }
  else
  {
      rsc.dwScope = RESOURCE_GLOBALNET;
      rsc.dwType = RESOURCETYPE_ANY;
      rsc.dwDisplayType = RESOURCEDISPLAYTYPE_GENERIC;
      rsc.dwUsage = RESOURCEUSAGE_CONTAINER;
      rsc.lpLocalName = NULL;
      rsc.lpProvider = NULL;

      // 'remote name' matches 2nd parm, must be 32-bit flat pointer

      GlobalPageLock(GlobalPtrHandle(szArg));

      rsc.lpRemoteName = (LPSTR)lpGetVDMPointer32W((LPVOID)szArg, 1);
      rsc.lpComment    = rsc.lpRemoteName;


      dw1 = DoNetViewProc(&rsc, &lp1);

      GlobalPageUnlock(GlobalPtrHandle(szArg));
  }

  if(dw1)
  {
     GlobalFreePtr(lpRval);
     lpRval = NULL;
  }

  return(lpRval);
}






/***************************************************************************/
/*  SPECIAL SUBSTITUTE VERSIONS OF '_fsopen()','fgets()','fputs()', etc.   */
/***************************************************************************/


//typedef struct tagMYFILE {
//   HFILE hFile;
//   OFSTRUCT ofstr;
//   char buffer[4096];
//   DWORD dwFilePos;
//   WORD wBufPos, wBufLen;
//   WORD wLastErr;
//   BOOL dirty;
//   } MYFILE;


MYFILE FAR * FAR PASCAL my_fsopen(LPSTR lpName, LPSTR lpMode, UINT uiFlags)
{
MYFILE FAR *lpRval;
LPSTR lp1;
WORD wAccess = 0, wFileMode;


   for(lp1=lpMode; *lp1; lp1++)
   {
      if(*lp1=='r' || *lp1=='R')
      {
         wAccess &= 0xfff0;
         wAccess |= 0;
      }
      else if(*lp1=='w' || *lp1=='W')
      {
         wAccess &= 0xfff0;
         wAccess |= 1;
      }
      else if(*lp1=='a' || *lp1=='A')
      {
         wAccess &= 0xfff0;
         wAccess |= 2;
      }
      else if(*lp1=='+')
      {
         wAccess &= 0xff0f;
         wAccess |= 0x10;
      }
      else if(*lp1=='t' || *lp1=='T')
      {
         continue;  // ignore for now
      }
      else if(*lp1=='b' || *lp1=='B')
      {
         continue;  // ignore for now
      }
      else
      {
         return(NULL);  // invalid access mode!!
      }
   }

   switch(wAccess & 0xf)
   {
      case 0:
         if(wAccess & 0x10) wFileMode = OF_READWRITE;
         else               wFileMode = OF_READ;
         break;

      case 1:
         if(wAccess & 0x10) wFileMode = OF_CREATE | OF_READWRITE;
         else               wFileMode = OF_CREATE | OF_WRITE;
         break;

      case 2:
         //if(wAccess & 0x10) wFileMode = OF_READWRITE;
         //else               wFileMode = OF_WRITE;

         wFileMode = OF_READWRITE;
         break;
   }

   switch(uiFlags)
   {
      case _SH_DENYNO:
         wFileMode |= OF_SHARE_DENY_NONE;
         break;

      case _SH_DENYRD:
         wFileMode |= OF_SHARE_DENY_READ;
         break;

      case _SH_DENYWR:
         wFileMode |= OF_SHARE_DENY_WRITE;
         break;

      case _SH_DENYRW:
         wFileMode |= OF_SHARE_EXCLUSIVE;
         break;

      case _SH_COMPAT:
         wFileMode |= OF_SHARE_COMPAT;
         break;
   }


   lpRval = (MYFILE FAR *)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof(MYFILE));
   if(!lpRval) return(NULL);


   if((wAccess & 0xf)==2)
   {
      lpRval->hFile = MyOpenFile(lpName, &(lpRval->ofstr),
                                 OF_READ | OF_EXIST | OF_SHARE_DENY_NONE);

      if(lpRval->hFile==HFILE_ERROR) wFileMode |= OF_CREATE;

   }

   lpRval->hFile = MyOpenFile(lpName, &(lpRval->ofstr), wFileMode);

   if((wAccess & 0xf)==2)
   {
      _llseek(lpRval->hFile, 0L, SEEK_END);  // point to end of file!

      lpRval->dwFilePos = _llseek(lpRval->hFile, 0L, SEEK_CUR);  // get current file pos
      lpRval->wBufPos = (WORD)(lpRval->dwFilePos % sizeof(lpRval->buffer));

      lpRval->dwFilePos -= lpRval->wBufPos;

      if(lpRval->wBufPos)
      {
         _llseek(lpRval->hFile, lpRval->dwFilePos, SEEK_SET);

         lpRval->wBufLen = MyRead(lpRval->hFile, lpRval->buffer,
                                   sizeof(lpRval->buffer));

         if(lpRval->wBufLen == 0xffff)
         {
            _lclose(lpRval->hFile);
            GlobalFreePtr(lpRval);
            return(NULL);
         }
      }
   }
   else if((wAccess & 0xf)==0)
   {
      lpRval->dwFilePos = 0;
      _llseek(lpRval->hFile, 0L, SEEK_SET);  // point to beginning of file!

      lpRval->wBufPos = 0;

      lpRval->wBufLen = MyRead(lpRval->hFile, lpRval->buffer,
                                sizeof(lpRval->buffer));

      if(lpRval->wBufLen == 0xffff)
      {
         _lclose(lpRval->hFile);
         GlobalFreePtr(lpRval);
         return(NULL);
      }
   }
   else
   {
      lpRval->dwFilePos = 0;
      lpRval->wBufPos = 0;
      lpRval->wBufLen = 0;
   }



   lpRval->dirty = 0;
   lpRval->wLastErr = 0;

   return(lpRval);

}

__inline void PASCAL mycheckflush(MYFILE FAR *lpF)
{
   lpF->wLastErr = 0;

   if(lpF->dirty && lpF->wBufLen)  // dirty flag is set!
   {
      _llseek(lpF->hFile, lpF->dwFilePos, SEEK_SET);

      lpF->wLastErr = (MyWrite(lpF->hFile, lpF->buffer, lpF->wBufLen)
                       !=lpF->wBufLen);
   }

   lpF->dirty = 0;
}

__inline void PASCAL mygetbuffer(MYFILE FAR *lpF)
{
   if((long)lpF->dwFilePos < (long)_llseek(lpF->hFile, 0, SEEK_END))
   {
      _llseek(lpF->hFile, lpF->dwFilePos, SEEK_SET);

      lpF->wBufLen = MyRead(lpF->hFile, lpF->buffer, sizeof(lpF->buffer));

      if(lpF->wBufLen = 0xffff)
      {
         lpF->wBufLen = 0;
         lpF->wLastErr = 1;
      }
      else
      {
         lpF->wLastErr = 0;
      }
   }
   else
   {
      lpF->wBufLen = 0;
      lpF->wLastErr = 0;
   }

   lpF->dirty = 0;
}


__inline int PASCAL mygetchar(MYFILE FAR *lpF)
{
   lpF->wLastErr = 0;

   if(lpF->wBufPos >= lpF->wBufLen)
   {
      mycheckflush(lpF);

      lpF->dwFilePos += lpF->wBufPos;

      lpF->wBufPos = (WORD)(lpF->dwFilePos % sizeof(lpF->buffer));
      lpF->dwFilePos -= lpF->wBufPos;

      mygetbuffer(lpF);

      if(lpF->wBufPos >= lpF->wBufLen)  // it's an EOF!
      {
         return(0x100);  // value greater than 0xff
      }
   }

   return(((int)(lpF->buffer[(lpF->wBufPos)++])) & 0xff);

}

__inline void PASCAL myputchar(MYFILE FAR *lpF, char c)
{
   lpF->wLastErr = 0;

   if(lpF->wBufPos >= lpF->wBufLen && lpF->wBufLen >= sizeof(lpF->buffer))
   {
      mycheckflush(lpF);

      lpF->dwFilePos += lpF->wBufPos;

      lpF->wBufPos = (WORD)(lpF->dwFilePos % sizeof(lpF->buffer));
      lpF->dwFilePos -= lpF->wBufPos;

      mygetbuffer(lpF);

   }

   lpF->buffer[(lpF->wBufPos)++] = c;

   if(lpF->wBufLen < lpF->wBufPos)  lpF->wBufLen = lpF->wBufPos;

   lpF->dirty = 1;

}


#pragma warning(disable:4035)  // disables 'no return value' warning

__inline int PASCAL VerifySelector(WORD wSel)
{
  register int iRval;

   _asm
   {
      mov bx, wSel
      _emit 0xf _asm add ax,bx
      jz ok
      mov ax,0
ok:

     mov iRval, ax
   }
   
   return(iRval);
}

#pragma warning(default:4035)  // re-enables 'no return value' warning




void FAR PASCAL myfclose(MYFILE FAR *lpF)
{
   if(!VerifySelector(SELECTOROF(lpF))) return;

   mycheckflush(lpF);

   _lclose(lpF->hFile);
   GlobalFreePtr(lpF);
}

LPSTR FAR PASCAL myfgets(LPSTR lpBuf, UINT uiBufLen, MYFILE FAR *lpF)
{
WORD w;
LPSTR lp1;
int c;


   if(!VerifySelector(SELECTOROF(lpF)))
      return(NULL);

   for(lp1=lpBuf, w=1; w<uiBufLen; w++)
   {
      c = mygetchar(lpF);


      if(c == 0x100)  // an EOF
      {
         *lp1 = 0;
         if(w==1)  //  first time through?
         {
            lpF->wLastErr = 0xffff;  // an EOF

            return(NULL);
         }
         else
            break; // if chars were read, continue to translation section
      }

      *(lp1++) = (char)c;  // add to buffer

      if(c=='\n')
      {
         break;
      }
   }

   *lp1 = 0;



   if(lp1 > (lpBuf + 1) && *(lp1 - 1)=='\n' && *(lp1 - 2)=='\r')
   {
      *(lp1 - 2) = '\n';
      *(lp1 - 1) = 0;
   }


   return(lpBuf);

}

int FAR PASCAL myfputs(LPSTR lpBuf, MYFILE FAR *lpF)
{
LPSTR lp1;

   if(!VerifySelector(SELECTOROF(lpF)))
      return(NULL);

   for(lp1=lpBuf; *lp1; lp1++)
   {
      if(*lp1=='\n')
      {
         myputchar(lpF, '\r');

         if(lpF->wLastErr) return(-1);
      }

      myputchar(lpF, *lp1);
      if(lpF->wLastErr) return(-1);
   }

   return(0);
}

BOOL FAR PASCAL myferror(MYFILE FAR *lpF)
{
   if(!VerifySelector(SELECTOROF(lpF)))
      return(0xffff);

   return(lpF->wLastErr && lpF->wLastErr!=0xffff);
}
