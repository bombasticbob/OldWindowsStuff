/***************************************************************************/
/*                                                                         */
/*   SHLTHDLL.C - Command line interpreter for Microsoft(r) Windows (tm)   */
/*                                                                         */
/*                      TRADE SECRET - CONFIDENTIAL                        */
/*                                                                         */
/*             The compiled libraries and API header files are             */
/*          (c) Copyright 1990-1995 by Stewart~Frazier Tools, Inc          */
/*                                                                         */
/*                SHLTHDLL.C - source file for SHLTHUNK.DLL                */
/*                                                                         */
/***************************************************************************/


#define THE_WORKS 1

#include "mywin.h"
#include "shlthunk.h"


// structures (internal use)


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
                        /* except in CHICAGO, where it starts at 210H */

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




// ** THE CODE **
 
int FAR PASCAL __export MyGetModuleUsage(HMODULE hModule)
{
  return(GetModuleUsage(hModule));  // just 'cause...
}

BOOL FAR PASCAL __export lpGlobalFirst(LPGLOBALENTRY lpGlobal, UINT wFlags)
{
  memset(lpGlobal, 0, sizeof(lpGlobal));

  lpGlobal->dwSize = sizeof(*lpGlobal);

  return(GlobalFirst(lpGlobal, wFlags));
}

BOOL FAR PASCAL __export lpGlobalNext(LPGLOBALENTRY lpGlobal, UINT wFlags)
{
  lpGlobal->dwSize = sizeof(*lpGlobal);

  return(GlobalNext(lpGlobal, wFlags));
}

BOOL FAR PASCAL __export lpMemManInfo(LPMEMMANINFO lpInfo)
{
  memset(lpInfo, 0, sizeof(lpInfo));

  lpInfo->dwSize = sizeof(*lpInfo);

  return(MemManInfo(lpInfo, wFlags));
}

BOOL FAR PASCAL __export lpModuleFirst(LPMODULEENTRY lpModule)
{
  memset(lpModule, 0, sizeof(lpModule));

  lpModule->dwSize = sizeof(*lpModule);

  return(ModuleFirst(lpModule, wFlags));
}

BOOL FAR PASCAL __export lpModuleNext(LPMODULEENTRY lpModule)
{
  lpModule->dwSize = sizeof(*lpModule);

  return(ModuleNext(lpModule, wFlags));
}

HTASK FAR PASCAL __export lpTaskFindHandle(LPTASKENTRY lpTask, HTASK hTask)
{
  memset(lpTask, 0, sizeof(lpTask));

  lpTask->dwSize = sizeof(*lpTask);

  return(TaskFindHandle(lpTask, hTask));
}

HMODULE FAR PASCAL __export lpModuleFindHandle(LPMODULEENTRY lpModule, HMODULE hModule)
{
  memset(lpModule, 0, sizeof(lpModule));

  lpModule->dwSize = sizeof(*lpModule);

  return(ModuleFindHandle(lpModule, hModule));
}


BOOL FAR PASCAL __export lpIsTask(HTASK hTask)
{
  return(IsTask(hTask));
}

BOOL FAR PASCAL __export lpTaskFirst(LPTASKENTRY lpTask)
{
  memset(lpTask, 0, sizeof(lpTask));

  lpTask->dwSize = sizeof(*lpTask);

  return(TaskFirst(lpTask, wFlags));
}

BOOL FAR PASCAL __export lpTaskNext(LPTASKENTRY lpTask)
{
  lpTask->dwSize = sizeof(*lpTask);

  return(TaskNext(lpTask, wFlags));
}




// internal procs

UINT FAR PASCAL __export GetDosMaxDrives(void)
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

#if 0  // for now...

BOOL FAR PASCAL __export IsWINOLDAP(HTASK hTask)
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


LPVOID FAR PASCAL __export GetDosCurDirEntry(WORD wDrive)
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


   return(lpRval);

}

#endif // 0


LPTDB __export GetTDBFromPSP(WORD wPSPSeg)
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


LPSTR FAR PASCAL __export GetTaskAndModuleFromPSP(WORD wMachID, WORD wPSPSeg,
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

                        if(lphTask) *lphTask =
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





// additional utilities (internal only)

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
