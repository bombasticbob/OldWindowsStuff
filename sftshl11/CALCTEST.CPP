/*

Description:  MATH CALCULATION PROGRAM


*/
#include "stdio.h"
#include "stdlib.h"
#include "math.h"
#include "string.h"
#include "memory.h"
#include "conio.h"
#include "graph.h"


#define SQL /* causes special 'SQL-specific' compiles */


typedef unsigned short WORD;
typedef short BOOL;

#define FALSE 0
#define TRUE (!FALSE)





/***************************************************************************/
/*                  STRUCTURES, 'TYPEDEF's AND CONSTANTS                   */
/***************************************************************************/




typedef struct tagEQUATION {
   char __based((__segment)__self) *pCurrent;
   char pData[];
   } EQUATION;

typedef EQUATION __far *LPEQUATION;


#define EQUATION_END           0     /* marks end of equation */
#define EQUATION_PUSHARG       1
#define EQUATION_PUSHOPERATOR  2
#define EQUATION_PUSHFUNCTION  3
#define EQUATION_PUSHARRAY     4
#define EQUATION_ERROR         0x7f  /* marks an error in pass 1 */
#define EQUATION_EVALUATE      0x80
#define EQUATION_EVALFUNCTION  0x81
#define EQUATION_EVALARRAY     0x82




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

#define OP_EQUAL        0x20     /* relational operators */
#define OP_LESSTHAN     0x21
#define OP_GREATERTHAN  0x22
#define OP_GREATEREQUAL 0x23
#define OP_LESSEQUAL    0x24
#define OP_NOTEQUAL     0x25

#define OP_AND          0x30     /* logical operators */
#define OP_OR           0x31
#define OP_NOT          0x32

#define OP_BITAND       0x40     /* bit-wise logical operators */
#define OP_BITOR        0x41
#define OP_BITNOT       0x42


#define OP_NEGATE       0x80     /* unary operators (immediate) */


                         /** GLOBAL VARIABLES **/


char *pOperators[]={"(", ")", ",",
		    "[", "]",
		    "+", "-", "*", "/", "^",
		    ">=", "<=", "<>", "!=",
		    "=", "<", ">", 
		    " AND ", " OR ", " NOT ", 
		    "&&", "||", "!",
		    "&", "|", "~" };

WORD wNOperators = sizeof(pOperators) / sizeof(*pOperators);


WORD wOperators[]={OP_LEFTPAREN,OP_RIGHTPAREN,OP_COMMA,
		   OP_LEFTBRACKET,OP_RIGHTBRACKET,
		   OP_ADD,OP_SUBTRACT,OP_MULTIPLY,OP_DIVIDE,OP_POWER,
		   OP_GREATEREQUAL,OP_LESSEQUAL,OP_NOTEQUAL,OP_NOTEQUAL,
		   OP_EQUAL,OP_LESSTHAN,OP_GREATERTHAN,
		   OP_AND, OP_OR, OP_NOT, 
		   OP_AND, OP_OR, OP_NOT,
		   OP_BITAND, OP_BITOR, OP_BITNOT };



                        /** INLINE FUNCTIONS **/

inline void ADD_EQUATION_PUSHARG(LPEQUATION Q, char __far *X)
{
   *(Q->pCurrent++)  = EQUATION_PUSHARG;

   _fstrcpy(Q->pCurrent, X);

   Q->pCurrent      += _fstrlen(X) + 1;
   *(Q->pCurrent)    = EQUATION_END;
}


inline void ADD_EQUATION_PUSHOPERATOR(LPEQUATION Q, char X)
{
   *(Q->pCurrent++) = EQUATION_PUSHOPERATOR;
   *(Q->pCurrent++) = X;
   *(Q->pCurrent)   = EQUATION_END;
}


inline void ADD_EQUATION_PUSHFUNCTION(LPEQUATION Q, char __far *X)
{
   *(Q->pCurrent++)  = EQUATION_PUSHFUNCTION;

   _fstrcpy(Q->pCurrent, X);

   Q->pCurrent      += _fstrlen(X) + 1;
   *(Q->pCurrent)    = EQUATION_END;
}


inline void ADD_EQUATION_PUSHARRAY(LPEQUATION Q, char __far *X)
{
   *(Q->pCurrent++)  = EQUATION_PUSHARRAY;

   _fstrcpy(Q->pCurrent, X);

   Q->pCurrent      += _fstrlen(X) + 1;
   *(Q->pCurrent)    = EQUATION_END;
}


inline void ADD_EQUATION_EVALUATE(LPEQUATION Q, char EvalType)
{
   *(Q->pCurrent++)  = EvalType;
   *(Q->pCurrent)    = EQUATION_END;
}


inline void ADD_EQUATION_ERROR(LPEQUATION Q, char __far *pErrorMessage,
                               int position)
{
   *(Q->pCurrent++)  = EQUATION_ERROR;
   *(Q->pCurrent++)  = (char)(position & 0xff);
   *(Q->pCurrent++)  = (char)((position >> 8) & 0xff);

   _fstrcpy(Q->pCurrent, pErrorMessage);

   Q->pCurrent      += _fstrlen(pErrorMessage) + 1;
   *(Q->pCurrent)    = EQUATION_END;
}



                        /** FUNCTION PROTOTYPES **/

BOOL seg_eval(char *line, LPEQUATION lpEQ);
BOOL seg_parse(char *line, WORD *ptr, WORD *op, LPEQUATION lpEQ);



/***************************************************************************
 *                                                                         *
 *                  M A I N   B O D Y  O F  P R O G R A M                  *
 *                                                                         *
 *                                                                         *
 ***************************************************************************/

#define MONO  0
#define COLOR 1

#define BLACK        0
#define BLUE         1
#define GREEN        2
#define CYAN         3
#define RED          4
#define MAGENTA      5
#define BROWN        6
#define LIGHTGRAY    7
#define DARKGRAY     8
#define LIGHTBLUE    9
#define LIGHTGREEN   10
#define LIGHTCYAN    11
#define LIGHTRED     12
#define LIGHTMAGENTA 13
#define YELLOW       14
#define WHITE        15


		 /*** A NEW VERSION OF 'cprintf()' ***/
		 /*** which uses the BIOS instead! ***/

	    /* it assumes far pointers and far functions */

int __cdecl my_cprintf(const char _far *format, ...)
{
static unsigned char attrib;
auto int rval;


   attrib = _gettextcolor() + 16 * (_getbkcolor() & 15);

   _asm
   {
      sub sp,256                     /* make room for 256 byte string */

	       /* set up arguments for call to 'vsprintf()' */

      lea ax,WORD PTR format + 4     /* address of variable arg list */
      push ss
      push ax                        /* push far pointer to (above) */

      push WORD PTR format + 2       /* segment first... */
      push WORD PTR format           /* then, offset! */

      push ss                        /* load 'last' arg .. address of strng */
      lea ax, WORD PTR [bp-256]      /* start of string...! */
      push ax

      call vsprintf                  /* call 'vsprintf'!! */
      add sp,12                      /* remove 3 far pointers from stack */

      mov cx,ax                      /* character count! */
      push ax                        /* and also here, for safekeeping! */
      or ax,ax
      jz bailout                     /* if zero just bail out!! */


      mov ax,0x40
      mov es,ax                      /* pointer to BIOS data area */

      mov al,es:[0x62]               /* video page */
      cbw
      mov bx,ax
      shl bx,1                       /* offset to cursor position */
      mov dx,es:[bx+0x50]            /* current cursor position */
      mov bh,al                      /* video page # (again) */
      mov bl,attrib

      lea ax,[bp - 256]              /* calculate start address of string */
      push bp
      mov bp,ax
      push ss
      pop es

      mov ax,0x1301                  /* use 'bl' as attrib & move cursor */
      int 0x10                       /* use BIOS to write to screen!     */

      pop bp                         /* restore the base pointer */
      pop ax                         /* the byte count! */

bailout:

      add sp,256                     /* restore stack pointer! */
				     /* at this point ax = char count */
      mov rval, ax
   }

   return(rval);
}



int main(int narg, char *arg[])
{

struct inbuf_struct
{
   char maxlen, nchars;
   char text[80];
} inbuf={80};

int i, driver_type, mode;
static char Equation[2048];



     if(narg>1)
     {        /*  command line argument option  */
         if(seg_eval(arg[1],(LPEQUATION)&Equation) )
         {
            printf("?ERROR - not evaluated\n\n");
         }

	 exit(0);
     }

     /*      Normal entry into program     */

     _asm
     {
	/* going to get display type */

	mov ax,0xf00
	int 0x10        /* get display mode */
	cbw
	mov mode,ax     /* save in 'mode' */
     }

     if(mode==7 || mode==15)   /* monochrome */
	driver_type = MONO;
     else
	driver_type = COLOR;  /* nothing fancy, here */

     if(driver_type==COLOR)
     {
	_setbkcolor(BLUE);
	_settextcolor(WHITE);
     }
     else
     {
	_setbkcolor(BLACK);
	_settextcolor(WHITE);
     }
     _clearscreen(_GCLEARSCREEN);


     my_cprintf("\xd\xa\xa                          Math calculation program!!"
	    "\xd\xa                    By Robert E. Frazier of San Diego, CA"
	    "\xd\xa\xa\xa");

     if(driver_type!=MONO) _settextcolor(LIGHTGREEN);

     my_cprintf("     "
	     "Compiled using Microsoft(R) 'C' 6.0 "
	     "{(c) 1990 Microsoft Corporation}\xd\xa\xa");

     if(driver_type!=MONO) _settextcolor(YELLOW);

     my_cprintf("Enter a blank line to exit!\xd\xa\xa");

     do
     {
	if(driver_type!=MONO) _settextcolor(LIGHTRED);
	my_cprintf("? ");   /* a '?' for a prompt */

	for(i=0;i<80;inbuf.text[i++]=0);    /* clear buffer */
	cgets((char *)&inbuf);
	printf("\r\n");              /* requires cr/lf printed manually */

	if(driver_type!=MONO) _settextcolor(WHITE);

	if(inbuf.nchars) /* the input string isn't blank if the # of chars */
			 /* in the array is non-zero                       */
        if(seg_eval(inbuf.text, (LPEQUATION)&Equation) )
            printf("?ERROR - not evaluated\n\n");
        else
            printf("Compile OK - continuing...\n\n");

     }

     while(inbuf.nchars);

     my_cprintf("\r\n\nEND OF PROGRAM\n");


   return(0);
}


/***************************************************************************
 *                         O P _ L E V E L ( )                             *
 ***************************************************************************/

/*  This function returns a positive integer value relating to the alge-   */
/*  braic order of operations of the character operator in the formal parm */

int op_level(int _op)
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
	return(10);       /* '&', like '+' but for logical operators */

      case OP_OR:
	return(20);       /* '|', like '*' but for logical operators */

      case OP_EQUAL:
      case OP_LESSTHAN:
      case OP_GREATERTHAN:
      case OP_GREATEREQUAL:
      case OP_LESSEQUAL:
      case OP_NOTEQUAL:
	return(30);       /* relational - like '^', same level as logical */


      case OP_ADD:
      case OP_SUBTRACT:
      case OP_BITAND:
      case OP_BITOR:
	return(100);      /* plus and minus have the same algeb. order   */
			  /* as well as bit-wise AND and OR operators.   */

      case OP_MULTIPLY:
      case OP_DIVIDE:
	return(200);      /* multiply & divide have same algeb. order */


      case OP_POWER:
	return(300);      /* exponentiation!  just for fun!              */


      case OP_NOT:        /* 'UNARY' operators - error if it gets here! */
      case OP_BITNOT:
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


int op_prec(int oper1, int oper2)
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

BOOL value(char *string, WORD P1, WORD P2, LPEQUATION lpEQ)
{
char copy_string[80];  /* a place to put junk in the meantime  */
double RESULT;
char *p;


    strncpy(copy_string, string + P1, (P2-P1) );
    copy_string[P2-P1]=0;
		  /* places the 'value' segment (up to the operator) */
		  /* into 'copy_string'.  Now, find its value!       */

    /** TRIM SPACES **/

    while(*copy_string && *copy_string<=' ')
       strcpy(copy_string,copy_string + 1);


    for(p = copy_string + strlen(copy_string); p>copy_string && *(p - 1)<=' ';
	*(--p) = 0)
	;      /* TRIM ALL TRAILING CONTROL CHARS */


    // printf("PUSH ARG \"%s\"\n", copy_string);

    ADD_EQUATION_PUSHARG(lpEQ, copy_string);

    return(0);

}


/***************************************************************************/
/*                      IsOperator(), NextOperator()                       */
/***************************************************************************/

BOOL IsOperator(char *line, WORD *ptr, WORD *op, WORD *op_start=NULL)
{  /* when I leave this proc '*ptr' points just past the op if it exists */
   /* the search for the operator must begin at (line + *ptr)            */
WORD i, p, l;



   for(p=*ptr; line[p] && line[p]<=' '; p++)
      ;  /* get the next 'non-white-space' character in line */



   for(i=0; i<wNOperators; i++)
   {
      l = strlen(pOperators[i]);

      if(!strnicmp(pOperators[i], line + p, l))
      {
		   /*** FOUND OPERATOR!! ***/

	 if(op_start) *op_start = p;     /* starting position of operator */

	 *ptr = p + l;     /* points *just* past the operator itself */
	 *op = wOperators[i];

	 return(TRUE);
      }
      else if((p==0 || line[p - 1]<=' ' || line[p - 1]=='"')
	      && *pOperators[i]<=' ' && l>2 
	      && !strnicmp(pOperators[i] + 1, line + p, l - 2)
	      && (line[p + l - 2]<=' ' || line[p + l - 2]=='"') )
      {
	 /* if 1st char in op is 'white space' I must check for */
	 /* other 'white space' possibilities also...           */

	 if(op_start) *op_start = p;     /* starting position of operator */

	 *ptr = p + l - 2;       /* points *just* past the operator itself */
	 *op = wOperators[i];

	 return(TRUE);
      }

   }


   return(FALSE);          // NO OPERATORS WERE FOUND!

}


BOOL NextOperator(char *line, WORD *ptr, WORD *op, WORD *op_start)
{
WORD i, p, l;


   for(p=*ptr; line[p] && line[p]<=' '; p++)
      ;  /* get the next 'non-white-space' character in line */

   for(; line[p]; p++)
   {
      if(line[p]=='"' && (!p || line[p - 1]!='\\'))
      {
	 p++;
	 while((line[p]!='"' || line[p - 1]=='\\') && line[p]) p++;
		/* find the end of the quoted string */

	 if(line[p]) p++;
	 else        break;
      }

      if(IsOperator(line, &p, op, op_start))
      {
	 *ptr = p;
	 return(TRUE);     /** FOUND!! **/
      }
   }

   *ptr += strlen(line + *ptr);     // points to end of string

   if(op_start) *op_start = *ptr;   // position of operator

   *op = OP_NULL;
   return(FALSE);   /** NO OPERATOR FOUND **/

}


/***************************************************************************
 *                           G E T _ T E R M ( )                           *
 ***************************************************************************/

 /*   This function returns the value of a term, and updates the next  */
 /*   operation value.  Negation and parentheses are handled here      */



BOOL get_term(char *line, WORD *ptr, WORD *op, LPEQUATION lpEQ)
{
BOOL r1;
int i, j, k;
WORD op_temp, P1, op_start;
char *p;
static char function_buf[64];


    while(line[*ptr] && line[*ptr]<=' ') (*ptr)++;  // trim lead spaces

    P1 = *ptr;                          /* save the beginning pointer value */

	     /** CHECK FOR AND PROCESS UNARY OPERATORS **/

    if(IsOperator(line, ptr, &op_temp))
    {
       if(op_temp==OP_SUBTRACT)         /* negation operator */
       {
//          printf("PUSH OPERATOR 'NEGATE'\n");
          ADD_EQUATION_PUSHOPERATOR(lpEQ, OP_NEGATE);

          get_term(line, ptr, op, lpEQ);


//          printf("EVALUATE\n");
          ADD_EQUATION_EVALUATE(lpEQ, EQUATION_EVALUATE);

          return(0);
       }
       else if(op_temp==OP_NOT)        /* LOGICAL 'NOT' */
       {
//          printf("PUSH OPERATOR 'NOT'\n");
          ADD_EQUATION_PUSHOPERATOR(lpEQ, OP_NOT);

          get_term(line, ptr, op, lpEQ);

//          printf("EVALUATE\n");
          ADD_EQUATION_EVALUATE(lpEQ, EQUATION_EVALUATE);

	  return(0);
       }
       else if(op_temp==OP_LEFTPAREN)   /* handle '( )' */
       {
	  *op=OP_LEFTPAREN;             /* flag operation as a left paren */

          r1 = seg_parse(line, ptr, op, lpEQ);// evaluate inside the parentheses

	      /* verify that the ')' was found, and get the next */
	      /* operator for evaluation!                        */

	  if(*op!=OP_RIGHTPAREN)
	  {                               /* ensure that there was a ')' */
					  /* and report the error if not */

//             printf("?Syntax error - parentheses mismatch!\n");

	     return(TRUE);
	  }

	  NextOperator(line, ptr, op, NULL);
	  return(FALSE);
       }
#ifdef SQL
       else if(op_temp==OP_MULTIPLY)
       {
               // This section is for SQL query processing only
               // it allows the '*' to be used as an argument

          NextOperator(line, ptr, op, &op_start);

             /* and now, flow through to section below */
             /* which will push the "*" as an argument */
       }
#endif /* SQL */
       else
       {
//          printf("?Syntax error - illegal 'unary' operator\n");
          ADD_EQUATION_ERROR(lpEQ, "?Syntax error - illegal 'unary' operator",
                          *ptr);
	  return(TRUE);
       }
    }
    else
    {
           /*  here we handle normal evaluation.  */
           /*  Step 1:  find the next operator    */

       NextOperator(line, ptr, op, &op_start);

    }


    if(*op==OP_LEFTPAREN)                     /* FUNCTION CALL! */
    {


      strncpy(function_buf, line + P1, (op_start-P1) );
      function_buf[op_start-P1]=0;
		    /* places the 'value' segment (up to the operator) */
		    /* into 'function_buf'.  Now, find its value!       */

      /** TRIM SPACES **/

      while(*function_buf && *function_buf<=' ')
	 strcpy(function_buf,function_buf + 1);


      for(p = function_buf + strlen(function_buf); p>function_buf && *(p - 1)<=' ';
	  *(--p) = 0)
	  ;      /* TRIM ALL TRAILING CONTROL CHARS */


//      printf("PUSH FUNCTION NAME \"%s\"\n", function_buf);
      ADD_EQUATION_PUSHFUNCTION(lpEQ, function_buf);


      do
      {
         if(seg_parse(line, ptr, op, lpEQ)) return(TRUE);
			 /* evaluate next parameter inside the parentheses */

	  /* verify that either ')' or ',' was found, and get the next */
	  /* parameter if ',';  bail out of loop if ')' (done!)        */

	 if(*op!=OP_COMMA && *op!=OP_RIGHTPAREN)
	 {                               /* ensure that there was a ')' */
					 /* and report the error if not */

//            printf("?Syntax error - parentheses mismatch in function call!\n");
            ADD_EQUATION_ERROR(lpEQ,
                   "?Syntax error - parentheses mismatch in function call!",
                   *ptr);

	    return(TRUE);
	 }

      } while(*op==OP_COMMA);        /* evaluate all args until NO COMMA! */

      P1 = *ptr;
      NextOperator(line, ptr, op, &op_start);

      if(op_start>P1)  // white space between ')' and next operator
      {
	 for(i=P1; i<op_start; i++)
	 {
	    if(line[i]>' ')
	    {
//               printf("?Syntax error - no operator following ')'!\n");
               ADD_EQUATION_ERROR(lpEQ,
                               "?Syntax error - no operator following ')'!",
                               *ptr);

               return(TRUE);
	    }
	 }
      }

//      printf("EVALUATE FUNCTION\n");
      ADD_EQUATION_EVALUATE(lpEQ, EQUATION_EVALFUNCTION);

      return(FALSE);

    }
    else if(*op==OP_LEFTBRACKET)                     /* ARRAY INDEX! */
    {

      strncpy(function_buf, line + P1, (op_start-P1) );
      function_buf[op_start-P1]=0;
		    /* places the 'value' segment (up to the operator) */
		    /* into 'function_buf'.  Now, find its value!       */

      /** TRIM SPACES **/

      while(*function_buf && *function_buf<=' ')
	 strcpy(function_buf,function_buf + 1);


      for(p = function_buf + strlen(function_buf); p>function_buf && *(p - 1)<=' ';
	  *(--p) = 0)
	  ;      /* TRIM ALL TRAILING CONTROL CHARS */


//      printf("PUSH ARRAY NAME \"%s\"\n", function_buf);
      ADD_EQUATION_PUSHARRAY(lpEQ, function_buf);


      do
      {
         if(seg_parse(line, ptr, op, lpEQ)) return(TRUE);
			 /* evaluate next parameter inside the parentheses */

	  /* verify that either ')' or ',' was found, and get the next */
	  /* parameter if ',';  bail out of loop if ')' (done!)        */

	 if(*op!=OP_COMMA && *op!=OP_RIGHTBRACKET)
	 {                               /* ensure that there was a ')' */
					 /* and report the error if not */

//            printf("?Syntax error - parentheses mismatch in function call!\n");
            ADD_EQUATION_ERROR(lpEQ,
                        "?Syntax error - mismatched '[]' in array subscript!",
                        *ptr);

	    return(TRUE);
	 }

      } while(*op==OP_COMMA);        /* evaluate all args until NO COMMA! */

      P1 = *ptr;
      NextOperator(line, ptr, op, &op_start);

      if(op_start>P1)  // white space between ']' and next operator
      {
	 for(i=P1; i<op_start; i++)
	 {
	    if(line[i]>' ')
	    {
//               printf("?Syntax error - no operator following ']'!\n");
               ADD_EQUATION_ERROR(lpEQ,
                               "?Syntax error - no operator following ']'!",
                               *ptr);

               return(TRUE);
	    }
	 }
      }

//      printf("EVALUATE ARRAY INDEX\n");
      ADD_EQUATION_EVALUATE(lpEQ, EQUATION_EVALARRAY);

      return(FALSE);

    }
    else
    {
       return(value(line,P1,op_start,lpEQ)); /* return the value of the term */
    }

}


/***************************************************************************
 *                          S E G _ P A R S E ( )                          *
 ***************************************************************************/

/* THIS IS A VERY POWERFUL RECURSIVE ROUTINE TO PARSE AN EQUATION USING
   ALGEBRAIC ORDER OF OPERATIONS                                         */

BOOL seg_parse( char *line, WORD *ptr, WORD *op, LPEQUATION lpEQ )
{
int op_cmp=0;
WORD op1, old_op;

  old_op = *op;

  if(get_term( line, ptr, op, lpEQ )) return(TRUE);
					  /* find next operation and get */
					  /* the term associated with it */
  do
  {
     op_cmp = op_prec((int) old_op ,(int) *op );

     if(op_cmp<0)                   /* if next operation has higher prec. */
     {                              /* evaluate by using recursion        */
       op1 = *op;

       if(op1==OP_RIGHTPAREN)       // this is always a SYNTAX ERROR
       {
//          printf("?Syntax error - parentheses mismatch (extra right ')')\n");
          ADD_EQUATION_ERROR(lpEQ,
                    "?Syntax error - parentheses mismatch - extra ')' found",
                    *ptr);

          return(TRUE);
       }
       else if(op1==OP_COMMA)
       {
	  /* at this point, *ptr will point to the next character in line */
	  /* after the ",", and *op is equal to the "," operator flag.    */

	  return(FALSE);
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

          ADD_EQUATION_PUSHOPERATOR(lpEQ, op1);
       }

       if(seg_parse( line, ptr, op, lpEQ )) return(TRUE);
		   /* recursive call to evaluate until an operation with  */
		   /* equal or lower precedence is found                  */

//       printf("EVALUATE\n");

       ADD_EQUATION_EVALUATE(lpEQ, EQUATION_EVALUATE);

     }
  }
  while(op_cmp<0);  /* repeat for all operations with this or greater     */
		    /* level of precedence, accumulating the result in    */
		    /* the variable 'r1'.                                 */

     /* at this point, *ptr will point to the next character in line after
	the operation, and *op is equal to the operation preceding *ptr   */

  return(FALSE);
}



/***************************************************************************
 *                         S E G _ E V A L ( )                             *
 ***************************************************************************/

 /*  This function calls the main segment parser to parse a string, and  */
 /*  checks for errors, etc.                                             */

BOOL seg_eval(char *line, LPEQUATION lpEQ)
{
BOOL rval;
WORD ww;
char __far *lp1;                      /* TEMPORARY! */
int i;                                /* TEMPORARY! */
unsigned short wLoc, w2, wNSymbols=0, wNFunctions=0, wNArrays=0;
                                      /* TEMPORARY! */
static char __far *plpSymbols[128];   /* TEMPORARY! */
static char __far *plpFunctions[128]; /* TEMPORARY! */
static char __far *plpArrays[128];    /* TEMPORARY! */

WORD PTR, OPER;        /*  here is the memory location of 'ptr'and 'op'! */
		       /*  the addresses are passed to seg_eval          */


    PTR=0;             /*  initialize a few things here   */
    OPER=OP_NULL;      /*  forces seg_eval to go to the end of the string */

    lpEQ->pCurrent = lpEQ->pData;   // assign value to 'current' pointer
    *(lpEQ->pCurrent) = EQUATION_END; // marks 'end of query'

    rval = seg_parse(line, &PTR, &OPER, lpEQ);




       /** TEMPORARY - leave this section for testing **/

    if(rval)
    {
       printf("?Call to 'seg_parse()' resulted in an error\n");
    }

    if(OPER!=OP_NULL)
    {
       for(ww=0; ww<wNOperators; ww++)
       {
	  if(wOperators[ww] == OPER)
	  {
	     break;
	  }
       }

       if(ww<wNOperators)
	  printf("?Evaluation ended on the \"%s\" operator!\n\n",
		  pOperators[ww]);
       else
	  printf("?Evaluation ended on operator #%x!\n\n", OPER);


       printf("The remainder of the line is: \"%s\"\n", line + PTR);
    }


    /** TEMPORARY - output the equation's results! **/


    lp1 = lpEQ->pData;

    while(*lp1!=EQUATION_END)
    {


       switch(*lp1)
       {
          case EQUATION_PUSHARG:

             lp1++;


             for(ww=0; ww<wNSymbols; ww++)
             {
                i=_fstricmp(plpSymbols[ww], lp1);
                if(i==0) break;
                else if(i>0)
                {
                   for(w2=wNSymbols; w2>ww; w2--)
                      plpSymbols[w2] = plpSymbols[w2-1];

                   plpSymbols[ww] = lp1;
                   wNSymbols++;
                   break;
                }
             }

             if(ww>=wNSymbols)
             {
                plpSymbols[wNSymbols++] = lp1;
             }

             printf("PUSH ARGUMENT: \"%s\"\n", lp1);
             lp1 += _fstrlen(lp1) + 1;

             break;

          case EQUATION_PUSHOPERATOR:
             lp1++;

             for(ww=0; ww<wNOperators; ww++)
             {
                if(wOperators[ww] == (unsigned short)*lp1)
                {
                   break;
                }
             }

             if(ww<wNOperators)
                printf("PUSH OPERATOR: \"%s\"\n", pOperators[ww]);
             else
                printf("PUSH OPERATOR #%d\n", (unsigned short)*lp1);

             lp1++;

             break;

          case EQUATION_PUSHFUNCTION:
             lp1++;
             printf("PUSH FUNCTION NAME: \"%s\"\n", lp1);
             lp1 += _fstrlen(lp1) + 1;

             break;

          case EQUATION_PUSHARRAY:
             lp1++;
             printf("PUSH ARRAY NAME: \"%s\"\n", lp1);
             lp1 += _fstrlen(lp1) + 1;

             break;

          case EQUATION_EVALUATE:
             printf("Evaluate OPERATION\n");
             lp1++;
             break;

          case EQUATION_EVALFUNCTION:
             printf("Evaluate FUNCTION\n");
             lp1++;
             break;

          case EQUATION_EVALARRAY:
             printf("Evaluate ARRAY\n");
             lp1++;
             break;

          case EQUATION_ERROR:

             lp1++;
             wLoc = (unsigned short)((unsigned char)*(lp1++));
             wLoc += 0x100 * ((unsigned char)*(lp1++));

             printf("%s\nError location: %d\n", lp1, wLoc);
             lp1 += _fstrlen(lp1) + 1;

             break;


          default:

             printf("?UNKNOWN operator - %d\n", (short)*lp1);

             lp1++;

             break;

       }
    }



    for(ww=0; ww<wNSymbols; ww++)
    {
       printf("SYMBOL %d: %s\n", ww, plpSymbols[ww]);
    }

    return(rval);

}
