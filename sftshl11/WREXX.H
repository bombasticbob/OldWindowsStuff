/* RXWIN.H Copyright (c)1991 Enterprise Alternatives, all rights reserved.
 |
 | C Header file for using of Personal Rexx for Windows.
 |
 | The content of this file is similar to the definitions for IBM OS2 REXX
 |     which IBM distributes as REXSAA.H.
 | For compatibility, so that user programs may easily be used with IBM REXX
 |	under OS2, many names and functions are identical to IBM's.
 |
 |
 |	       U P D A T E S   A N D   C H A N G E S
 |	       - - - - - - -   - - -   - - - - - - -
 |
 |  02Jun92 Add RxLibLink().
 |  23Nov91 Cosmetic review.
 |  26Jun91 Creation.
 |
 |
 |		T H I N G S   T O   D O  (to this header)
 |		- - - - - -   - -   - -
 |
 |
 */
 
#ifndef RXWIN_INCLUDED
#define RXWIN_INCLUDED
 
#ifdef INCL_REXXSAA
#define INCL_RXSUBCOM
#define INCL_RXSHV
#define INCL_RXFUNC
#define INCL_RXSYSEXIT
#define INCL_RXMACRO
#endif /* INCL_REXXSAA */

#ifndef APIENTRY
#define APIENTRY pascal far
#endif

/* System exits definitions require subcommand definitions
 */
#ifdef  INCL_RXSYSEXIT
#define INCL_RXSUBCOM
#endif
 
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*
 |  Structure for external interface string (RXSTRING).
 */
 
#define RXAUTOBUFLEN         256L
 
#define RXNULLSTRING(r)      (!(r).strptr)
#define RXZEROLENSTRING(r)   ((r).strptr && !(r).strlength)
#define RXVALIDSTRING(r)     ((r).strptr && (r).strlength)
#define RXSTRLEN(r)          (RXNULLSTRING(r)?0L:(r).strlength)
#define RXSTRPTR(r)          (r).strptr
#define MAKERXSTRING(r,p,l)  (r).strptr=(LPBYTE)p;(r).strlength=(DWORD)l

/* change made by me to reflect change I made before (10/21/92) */
#define PBYTE LPBYTE
/* end of change */

 
typedef struct
{   DWORD	strlength;	    // length of string
    LPBYTE	strptr; 	    // far pointer to string
} RXSTRING;

typedef RXSTRING FAR *PRXSTRING;	// pointer to a RXSTRING
 

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*
 | Structure for system exit block (RXSYSEXIT).
 */
 
typedef struct
{   LPBYTE	sysexit_name;	    // subcom environ for sysexit
    int 	sysexit_code;	    // sysexit function code
} RXSYSEXIT;

typedef RXSYSEXIT FAR *PRXSYSEXIT;	// pointer to a RXSYSEXIT
 

/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*
 |    Main Entry Point to the REXX Interpreter.
 */
#define RXCOMMAND	0	    // Program called as Command
#define RXSUBROUTINE	1	    // Program called as Subroutin
#define RXFUNCTION	2	    // Program called as Function
 
int FAR PASCAL REXXSAA (
	 int,			    // Num of args passed to rexx
	 PRXSTRING,		    // Array of args passed to rex
	 LPBYTE,		    // [d:][path] filename[.ext]
	 PRXSTRING,		    // Loc of rexx proc in memory
	 LPBYTE,		    // ASCIIZ initial environment.
	 int,			    // type (command,subrtn,funct)
	 PRXSYSEXIT,		    // SysExit env. names &  codes
	 LPINT,			    // Ret code if numeric (see next parm)
	 PRXSTRING );		    // Ret code value from the rexx proc


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*
 |  Subcommand Interface.
 */

#ifdef INCL_RXSUBCOM
 
/*  Drop Authority for RXSUBCOM interface
 */
#define RXSUBCOM_DROPPABLE  0x00    // handler to be dropped by all*/
#define RXSUBCOM_NONDROP    0x01    // process with same PID as the*/
				    // registrant may drop environ */
/*  Return Codes from RXSUBCOM interface
 */
#define RXSUBCOM_ISREG	    0x01    // Subcommand is registered.
#define RXSUBCOM_ERROR	    0x01    // Subcommand Ended in Error.
#define RXSUBCOM_FAILURE    0x02    // Subcommand Ended in Failure.
#define RXSUBCOM_BADENTRY   1001    // Invalid Entry Conditions.
#define RXSUBCOM_NOEMEM     1002    // Not enough memory to complete request.
#define RXSUBCOM_BADTYPE    1003    // Bad registration type.
#define RXSUBCOM_OK	       0    // Function Completed AOK.
#define RXSUBCOM_DUP	      10    // Duplicate Environment Name-
				    // but Registration Completed.
#define RXSUBCOM_MAXREG       20    // Cannot register more names.
#define RXSUBCOM_NOTREG       30    // Name Not Registered.
#define RXSUBCOM_NOCANDROP    40    // Name not droppable.
#define RXSUBCOM_LOADERR      50    // Could not load function.
#define RXSUBCOM_NOPROC      127    // RXSUBCOM routine - not found.
 
/*  Structure of Rexx Subcomand Block (SCBLOCK).
 */
typedef struct subcom_node
{   struct subcom_node far *next;   // pointer to the next block.
    LPBYTE scbname;		    // subcom environment name.
    LPBYTE scbdll_name;		    // subcom module name.
    LPBYTE scbdll_proc;		    // subcom procedure name.
    double scbuser;		    // user area.
    FARPROC scbaddr;		    // subcom environment address.
    WORD   scbmod_handle;	    // DLL module handle.
    WORD   scbdrop_auth;	    // Permission to drop.
    WORD   scbpid;		    // Pid of Registrant.
    WORD   scbsid;		    // Session ID.
} SCBLOCK;

typedef SCBLOCK FAR *PSCBLOCK;		// pointer to a SCBLOCK
 
/*  RxSubcomRegister - Register environment as Subcommand handler.
 */
WORD APIENTRY RxSubcomRegister(
	 PSCBLOCK );			// pointer to a SCBLOCK
 
/*  RxSubcomQuery - Query an environment for Existance.
 */
WORD APIENTRY RxSubcomQuery(
	 LPBYTE,			// Name of the Environment.
	 LPBYTE,			// DLL Module Name.
	 LPWORD,			// Stor for existance code.
	 double far *); 		// Stor for user word.
 
/*  RxSubcomLoad - Load a Subcommand environment.
 */
WORD APIENTRY RxSubcomLoad(
	 LPBYTE,			// Name of the Environment.
	 LPBYTE);			// DLL Module Name.
 
/*  RxSubcomDrop - Drop an Subcommand environment.
 */
WORD APIENTRY RxSubcomDrop(
	 LPBYTE,			// Name of the Environment.
	 LPBYTE);			// DLL Module Name.
 
/*  RxSubcomExecute - Execute a command in an environment.
 */
WORD APIENTRY RxSubcomExecute(
	 LPBYTE,			// Name of Subcommand Environment.
	 LPBYTE,			// Module name of its' DLL.
	 PRXSTRING,			// Command string to be passed.
	 LPWORD,			// Stor for error flag notice.
	 LPWORD,			// Stor for RC from handler.
	 PRXSTRING );			// Stor for returned string.
 
#endif /* INCL_RXSUBCOM */
 
 
/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*
 |  Shared Variable Pool Interface.
 */

#ifdef INCL_RXSHV
 
/*  Function Codes for Variable Pool Interface (shvcode).
 */
#define RXSHV_SET	   0x00     // Set var from given value.
#define RXSHV_FETCH	   0x01     // Copy value of var to buffer.
#define RXSHV_DROPV	   0x02     // Drop variable.
#define RXSHV_SYSET	   0x03     // Symbolic name Set variable.
#define RXSHV_SYFET	   0x04     // Symbolic name Fetch variable.
#define RXSHV_SYDRO	   0x05     // Symbolic name Drop variable.
#define RXSHV_NEXTV	   0x06     // Fetch "next" variable.
#define RXSHV_PRIV	   0x07     // Fetch private information.
#define RXSHV_EXIT	   0x08     // Set function exit value.
 
/*  Return Code Flags for Variable Pool Interface (shvret).
 */
#define RXSHV_OK	   0x00     // Execution was OK.
#define RXSHV_NEWV	   0x01     // Variable did not exist.
#define RXSHV_LVAR	   0x02     // Last var trans via SHVNEXTV.
#define RXSHV_TRUNC	   0x04     // Truncation occurred-Fetch.
#define RXSHV_BADN	   0x08     // Invalid variable name.
#define RXSHV_MEMFL	   0x10     // Out of memory failure.
#define RXSHV_BADF	   0x80     // Invalid funct code (shvcode).
#define RXSHV_NOAVL	   0x90     // Interface not available.
 
/*  Structure of Shared Variable Request Block (SHVBLOCK).
 */
typedef struct shvnode
{   struct shvnode FAR *shvnext;    // pointer to the next block.
    RXSTRING	       shvname;     // Pointer to the name buffer.
    RXSTRING	       shvvalue;    // Pointer to the value buffer.
    DWORD	       shvnamelen;  // Length of the name value.
    DWORD	       shvvaluelen; // Length of the fetch value.
    BYTE	       shvcode;	    // Function code for this block.
    BYTE	       shvret;	    // Individual Return Code Flags.
} SHVBLOCK;
typedef SHVBLOCK FAR *PSHVBLOCK;
 
/*  RxVar - Request Variable Pool Service.
 */
WORD APIENTRY RxVar(
	 PSHVBLOCK);		    // Pointer to list of SHVBLOCKs
 
#endif /* INCL_RXSHV */


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*
 |  External Function Interface.
 */

#ifdef INCL_RXFUNC
 
/*  Registration Type Identifiers for Functions.
 */
#define RXFUNC_DYNALINK       1     // Function Available in DLL.
#define RXFUNC_CALLENTRY      2     // Registered as mem entry point.
 
/*  Return Codes from RxFunction interface.
 */
#define RXFUNC_OK	      0     // REXX-API Call Successful.
#define RXFUNC_DEFINED	     10     // Function already Defined.
#define RXFUNC_NOMEM	     20     // Not Enough Mem to Add.
#define RXFUNC_NOTREG	     30     // Function Not Registered!
#define RXFUNC_MODNOTFND     40     // Function DLL Module Not Found.
#define RXFUNC_ENTNOTFND     50     // Function Entry Point Not Found.
 
/*  RxFunctionRegister - Register an external function.
 */
WORD APIENTRY RxFunctionRegister (
	LPBYTE, 		    // Name of function to add.
	LPBYTE, 		    // DLL file name (if in DLL).
	LPBYTE, 		    // Entry in dll OR mem address.
	WORD);			    // RX_DYNALINK || RX_CALLENTRY.
 
/*  RxFunctionDeregister - Delete an external function.
 */
WORD APIENTRY RxFunctionDeregister (
	LPBYTE );		    // Name of function to remove.
 
/*  RxFunctionQuery - Scan for an external function.
 */
WORD APIENTRY RxFunctionQuery (
	LPBYTE );		    // Name of function to find.
 
/*  RxFunctionCall - Call a regsitered external function.
 */
WORD APIENTRY RxFunctionCall (
	LPBYTE, 		    // Name of function to call.
	WORD,			    // Number of arguments.
	PRXSTRING,		    // Array of argument strings.
	LPWORD,			    // RC from function called.
	PRXSTRING,		    // Storage for returned data.
	LPBYTE );		    // Name of active data queue.
 
#endif /* INCL_RXFUNC */


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*
 |  System Exits.
 */

#ifdef INCL_RXSYSEXIT
 
/* System Exit function and sub-function definitions.
 */
#define RXENDLST    0		    // End of exit list.
#define RXFNC	 2		    // Process external functions.
#define    RXFNCCAL 1		       // subcode value.
#define RXCMD	 3		    // Process host commands.
#define    RXCMDHST 1		       // subcode value.
#define RXMSQ	 4		    // Manipulate queue.
#define    RXMSQPLL 1		       // Pull a line from queue.
#define    RXMSQPSH 2		       // Place a line on queue.
#define    RXMSQSIZ 3		       // Return num of lines on queue.
#define    RXMSQNAM 20		       // Set active queue name.
#define RXSIO	 5		    // Session I/O.
#define    RXSIOSAY 1		       // SAY a line to STDOUT.
#define    RXSIOTRC 2		       // Trace output.
#define    RXSIOTRD 3		       // Read from char stream.
#define    RXSIODTR 4		       // DEBUG read from char stream.
#define    RXSIOTLL 5		       // Return linelength(N/A OS/2).
#define RXHLT	 7		    // Halt processing.
#define    RXHLTCLR 1		       // Clear HALT indicator.
#define    RXHLTTST 2		       // Test HALT indicator.
#define RXTRC	 8		    // Test ext trace indicator.
#define    RXTRCTST 1		       // subcode value.
#define RXINI	 9		    // Initialization processing.
#define    RXINIEXT 1		       // subcode value.
#define RXTER	10		    // Termination processing.
#define    RXTEREXT 1		       // subcode value.
#define RXNOOFEXITS 11		    // 1 + largest exit number.
 
typedef PBYTE PEXIT;		/* ptr to exit parameter block */
 
/*  Subfunction RXFNCCAL - External Function Calls.
 */
typedef struct
{  struct
   {  unsigned rxfferr	: 1;	    // Invalid call to routine.
      unsigned rxffnfnd : 1;	    // Function not found.
      unsigned rxffsub	: 1;	    // Called as a subroutine if set.
				    // Return values are
				       //	    optional for subroutines,
				       //	    required for functions.
   }  rxfnc_flags ;
   PBYTE	    rxfnc_name;     // Pointer to function name.
   WORD 	   rxfnc_namel;     // Length of function name.
   PBYTE	    rxfnc_que;	    // Current queue name.
   WORD 	   rxfnc_quel;	    // Length of queue name.
   WORD 	   rxfnc_argc;	    // Number of args in list.
   PRXSTRING	     rxfnc_argv;    // Pointer to argument list.
				    // List mimics argv list in REXXSAA
				       //	    -- array of RXSTRINGs.
   RXSTRING	     rxfnc_retc;    // Return value.
} RXFNCCAL_PARM;
 
 
/*  Subfunction RXCMDHST -- Process Host Commands.
 */
typedef struct
{  struct			    // Condition flags
   {  unsigned rxfcfail : 1;	    // Command failed.	Trap with
				    // CALL or SIGNAL on FAILURE.
      unsigned rxfcerr	: 1;	    // Command ERROR occurred.	Trap with
				    // CALL or SIGNAL on ERROR.
   } rxcmd_flags;
   PBYTE	rxcmd_address;	    // Pointer to address name.
   WORD 	rxcmd_addressl;     // Length of address name.
   PBYTE	rxcmd_dll;	    // DLL name for command.
   WORD 	rxcmd_dll_len;	    // Length of dll name.  0 ==> .EXE file.
   RXSTRING	rxcmd_command;	    // The command string.
   RXSTRING	rxcmd_retc;	// Pointer to return code buffer.  User allocated.
} RXCMDHST_PARM;
 
 
/*  Subfunction RXMSQPLL -- Pull Entry from Queue.
 */
typedef struct
{  RXSTRING	rxmsq_retc;	// Pointer to dequeued entry buffer.  User allocated.
} RXMSQPLL_PARM;
 
 
/*  Subfunction RXMSQPSH -- Push Entry on Queue.
 */
typedef struct
{   struct			    // Operation flag.
    {	unsigned rxfmlifo : 1;	    // Stack entry LIFO if set, FIFO if reset.
    }	rxmsq_flags;
    RXSTRING	    rxmsq_value;    // The entry to be pushed.
} RXMSQPSH_PARM;
 
/*  Subfunction RXMSQSIZ -- Return the Current Queue Size.
 */
typedef struct
{   DWORD	    rxmsq_size;     // Number of Lines in Queue.
} RXMSQSIZ_PARM;
 
/*  Subfunction RXMXQNAM -- Set Current Queue Name.
 */
typedef struct
{   HANDLE	    rxmsq_name;     // Selector containing ASCIIZ queue name.
				    // Change length with GlobalRealloc() if required.
} RXMSQNAM_PARM;
 
/*  Subfunction RXSIOSAY -- Perform SAY Clause.
 */
typedef struct
{    RXSTRING	    rxsio_string;   // String to display.
} RXSIOSAY_PARM;
 
/*  Subfunction RXSIOTRC -- Write Trace Output.
 */
typedef struct
{   RXSTRING	    rxsio_string;   // Trace line to display.
} RXSIOTRC_PARM;
 
/*  Subfunction RXSIOTRD -- Read Input from the Terminal.
 */
typedef struct
{   RXSTRING	    rxsiotrd_retc;  // RXSTRING for output.
				    // NOTE: Allocate output buffer
				    //	     with GLobalAlloc().
} RXSIOTRD_PARM;
 
 
/*  Subfunction RXSIODTR -- Read Debug Input from the Terminal.
 */
typedef struct
{   RXSTRING	    rxsiodtr_retc;  // RXSTRING for output.
				    // NOTE: Allocate output buffer
				    //	     with GLobalAlloc().
} RXSIODTR_PARM;
 
 
/*  Subfunction RXHLTTST -- Test for HALT Condition.
 */
typedef struct
{   struct			    // Halt flag.
    {	unsigned rxfhhalt : 1;	    // Set if HALT occurred.
    } rxhlt_flags;
} RXHLTTST_PARM;
 
/*  Subfunction RXTRCTST -- Test for TRACE Condition.
 */
typedef struct
{   struct			    // Trace flags.
    {	unsigned rxftrace : 1;	    // Set to run external trace.
    } rxtrc_flags;
} RXTRCTST_PARM;
 
/*  RxExitRegister - Register a system exit.
 */
WORD APIENTRY RxExitRegister (
	 PSCBLOCK ) ;		    // Ptr to SCBLOCK to register.
 
/*  RxExitDrop - Drop a system exit.
 */
WORD APIENTRY RxExitDrop (
	 LPBYTE,		    // Exit name.
	 LPBYTE ) ;		    // DLL module name.
 
/*  RxExitQuery - Query an exit for existance.
 */
WORD APIENTRY RxExitQuery (
	 LPBYTE,		    // Exit name.
	 LPBYTE,		    // DLL Module name.
	 LPWORD,		    // Existance flag.
	 double far * ) ;	    // User data.
 
#endif /* INCL_RXSYSEXIT */


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*
 |  Macro Space Interface
 */

#ifdef INCL_RXMACRO
 
/*  Registration Search Order Flags
 */
#define RXMACRO_SEARCH_BEFORE	    1	// Beginning of search order.
#define RXMACRO_SEARCH_AFTER	    2	// End of search order.

/*  Return Codes from RxMacroSpace interface.
 */
#define RXMACRO_OK		    0	// Macro interface completed.
#define RXMACRO_NO_STORAGE	    1	// Not Enough Storage Available.
#define RXMACRO_NOT_FOUND	    2	// Requested function not found.
#define RXMACRO_EXTENSION_REQUIRED  3	// File ext required for save.
#define RXMACRO_ALREADY_EXISTS	    4	// Macro functions exist.
#define RXMACRO_FILE_ERROR	    5	// File I/O error in save/load.
#define RXMACRO_SIGNATURE_ERROR     6	// Incorrect format for load.
#define RXMACRO_SOURCE_NOT_FOUND    7	// Requested cannot be found.
#define RXMACRO_INVALID_POSITION    8	// Invalid search order pos.ition
 
 
/*  RxMacroChange - Register a function in the Macro Space.
 */
WORD APIENTRY RxMacroChange (
	 LPBYTE,			// Function to add/change.
	 LPBYTE,			// Name of file to get function.
	 WORD );			// Flag indicating search position.
 
/*  RxMacroDrop - Remove a function from the Macro Space.
 */
WORD APIENTRY RxMacroDrop (
	 LPBYTE );			// Name of function to remove.
 
/*  RxMacroSave - Save Macro Space functions to a file.
 */
WORD APIENTRY RxMacroSave (
	 WORD,				// Argument count (0==save all).
	 LPBYTE FAR *,			// List of funct names to save.
	 LPBYTE );			// File to save functions in.
 
/*  RxMacroLoad - Load Macro Space functions from a file.
 */
WORD APIENTRY RxMacroLoad (
	 WORD,				// Argument count (0==load all).
	 LPBYTE FAR *,			// List of funct names to load.
	 LPBYTE );			// File to load functions from.
 
/*  RxMacroQuery - Find a function's search-order position.
 */
WORD APIENTRY RxMacroQuery (
	 LPBYTE,			// Function to search for.
	 LPWORD );			// Ptr for position flag return.
 
/*  RxMacroReOrder - Change a function's search-order position.
 */
WORD APIENTRY RxMacroReOrder(
	 LPBYTE,			// Name of function change order.
	 WORD );			// New position for function.
 
/*  RxMacroErase - Remove all functions from the Macro Space.
 */
WORD APIENTRY RxMacroErase(
	 VOID );			// No Arguments.
 
#endif /* INCL_RXMACRO */
 
#endif /* REXXSAA_INCLUDED */


/*-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-*
 |  These below are WINDOWS specific additional type definitions
 */

typedef PRXSTRING LPRXSTRING;		// pointer to a RXSTRING.

#ifdef INCL_RXLIBLINK
/*  RxLibLink - Establish thread Links.
 */

/*  Call reason Codes for RxLibLink.
 */
#define THREAD_DETACH		    0	// Thread terminating - no more Rx call.
#define THREAD_ATTACH		    1	// NEW Thread - before any other Rx call.

/*  Return Codes from RxLibLink.
 */
#define THREAD_ATTACH_AOK	    0	// AOK attach - entry made in LinkTable.
#define THREAD_NOT_REGISTERED	   -1	// Registration failed.
#define THREAD_INIT_BEFORE_LIB	   -2	// LIB must be initialized before Thread.
#define THREAD_ALREADY_LINKED	   -3	// Thread already registered (Same hTask).


#define THREAD_DETACH_AOK	    0	// AOK detach - entry dropped from LinkTable..
#define DETACH_DERIGISTER_FAILED   -1	// Failed to drop task table entry.

#define LINK_TASK_NOT_CALLER	   -1	// passed hTask != current hTask.

int APIENTRY RxLibLink ( HTASK,	    // HTASK of caller.
			   int );   // Calling reason Code.

#endif /* INCL_RXLIBLINK */



/*  REXX file object code handling.
 */

/*  Get object code from an open REXX file.
 |  RETURNS: on failure returns 0.
 |	     on success returns the object code length (including EOF_CHAR).
 |	     (subtract this value from file size to get soruce code length).
 |
 */
LONG APIENTRY RxGetObjectFromFile( HFILE hRxFile,
			       PRXSTRING prxObject );

/*  Append object code from an open REXX file.
 |  Also appends EOF_CHAR if needed.
 |  RETURNS: on failure returns 0.
 |	     on success returns the object code length (!including EOF_CHAR).
 |
 */
LONG APIENTRY RxAppendObjectToFile( HFILE hRxFile,
				PRXSTRING prxObject );
