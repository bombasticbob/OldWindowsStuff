//
//      WinXRay - (c) 1992-94 by R. E. Frazier - all rights reserved
//
//            HEADER FILE CONTAINING MENU & CONTROL ID VALUES
//                  and COMMON DEFINITIONS for 'C' CODE
//


#define ID_LISTBOX 100




#define IDM_ABOUT   101



#define IDM_MINIMUM 102

#define IDM_FILES   102
#define IDM_MEMORY  103
#define IDM_TASKS   104
#define IDM_IDT     105
#define IDM_GDT     106
#define IDM_LDT     107
#define IDM_PAGEMAP 108
#define IDM_VM      109
#define IDM_x87     110
#define IDM_VXD     111

#define IDM_TEST    115

#define IDM_MAXIMUM 115



#define IDM_HELP_MINIMUM  116

#define IDM_HELP_FILES    116
#define IDM_HELP_MEMORY   117
#define IDM_HELP_TASKS    118
#define IDM_HELP_IDT      119
#define IDM_HELP_GDT      120
#define IDM_HELP_LDT      121
#define IDM_HELP_PAGEMAP  122
#define IDM_HELP_VM       123
#define IDM_HELP_VXD      124


#define IDM_HELP_CONTENTS 131

#define IDM_HELP_MAXIMUM  131


#define IDM_EXIT          132

#define IDM_EDIT_COPY     133


// CONTROL ID'S USED BY 'USER_INPUT' DIALOG BOX

#define ID_INPUT_LABEL    200
#define ID_INPUT_TEXT     201



#ifndef RC_INVOKED

		     /******************************/
		     /** COMMON MACRO DEFINITIONS **/
		     /******************************/


#define INIT_LEVEL do {           /* cheater 'death before goto' macros */
#define END_LEVEL } while(FALSE);

#define GlobalSizePtr(X) (GlobalSize(GlobalPtrHandle(X)))


			   /****************/
			   /** DATA TYPES **/
			   /****************/

#pragma pack(1)

	     /* these 4 should be in TOOLHELP.H, but aren't */

typedef MEMMANINFO FAR *LPMEMMANINFO;
typedef TASKENTRY FAR *LPTASKENTRY;
typedef GLOBALENTRY FAR *LPGLOBALENTRY;
typedef MODULEENTRY FAR *LPMODULEENTRY;



typedef struct {
   char module[10];
   HTASK hTask;
   HINSTANCE hInst;
   HMODULE hModule;
   } TASKLIST;
typedef TASKLIST FAR *LPTASKLIST;


typedef struct _sft2_ {      /* DOS 2.x SYSTEM FILE TABLE */
   char n_handles;
   char open_mode;
   char file_attr;
   char drive;
   char name[11];
   WORD reserv1,reserv2;
   DWORD file_size;
   WORD file_date;
   WORD file_time;
   char dev_attr;
   WORD start_clus;
   WORD rel_clus;
   WORD abs_clus;
   WORD reserv3;
   DWORD file_pos;
   };

typedef struct _sft30_ {     /* DOS 3.0 SYSTEM FILE TABLE */
   WORD n_handles;
   WORD open_mode;
   char file_attr;
   WORD dev_info_word;
   LPSTR lpDPB;
   WORD start_clus;
   WORD file_time;
   WORD file_date;
   DWORD file_size;
   DWORD file_pos;
   WORD rel_clus;
   WORD abs_clus;
   WORD dir_sector;
   WORD dir_entry_num;
   char name[11];
   DWORD lpPreviousSFT;  /* previous SFT entry sharing this file */
   WORD vm_id;
   WORD psp_seg;
   WORD share_cs_offset;
   };

typedef struct _sft31_ {     /* DOS 3.x SYSTEM FILE TABLE */
   WORD n_handles;
   WORD open_mode;
   char file_attr;
   WORD dev_info_word;
   LPSTR lpDPB;
   WORD start_clus;
   WORD file_time;
   WORD file_date;
   DWORD file_size;
   DWORD file_pos;
   WORD rel_clus;
   WORD abs_clus;
   WORD dir_sector;
   char dir_entry_num;
   char name[11];
   DWORD lpPreviousSFT;  /* previous SFT entry sharing this file */
   WORD vm_id;
   WORD psp_seg;
   WORD share_cs_offset;
   };

typedef struct _sft4_ {      /* DOS 4+ SYSTEM FILE TABLE */
   WORD n_handles;
   WORD open_mode;
   char file_attr;
   WORD dev_info_word;
   LPSTR lpDPB;
   WORD start_clus;
   WORD file_time;
   WORD file_date;
   DWORD file_size;
   DWORD file_pos;
   WORD rel_clus;
   DWORD dir_sector;
   char dir_entry_num;
   char name[11];
   DWORD lpPreviousSFT;  /* previous SFT entry sharing this file */
   WORD vm_id;
   WORD psp_seg;
   WORD share_cs_offset;
   WORD abs_clus;
   DWORD lpIFS;  /* NULL for DOS, pointer if Installed File System */
   };
   
typedef struct _sft_head_{   /* 'SFT HEAD' structure */
   struct _sft_head_ FAR *lpNext;
   WORD wCount;
   char lpData[];
   } SFT;
typedef SFT FAR *LPSFT;


typedef struct {
   WORD wSegLimLo;
   WORD wSegBaseLo;
   unsigned char wSegBaseMid;
   unsigned char wDPL_and_Flags;
   unsigned char wSegLimHi_and_Flags;
   unsigned char wSegBaseHi;
   } DESCRIPTOR;
typedef DESCRIPTOR FAR *LPDESCRIPTOR;


#define DOSVER_2X    0
#define DOSVER_30    1
#define DOSVER_3X    2
#define DOSVER_4PLUS 3
#define DOSVER_7PLUS 4



typedef struct tagREALMODECALL {   /* used by DPMI to call real mode int */
 DWORD EDI,ESI,EBP,reserved,EBX,EDX,ECX,EAX;
 WORD  flags,ES,DS,FS,GS,IP,CS,SP,SS;
 } REALMODECALL;

typedef REALMODECALL FAR *LPREALMODECALL;






	     /**   Special structure definition for TDB   **/
	     /**(Hacked-up and corrected version from DDK)**/
	     /**   (the DDK version had errors in it!!)   **/
	     /**    [3.1 build 61d DDK is now correct]    **/

typedef struct tagTDB {
   HTASK     hNext;       /* next task in list, or NULL */
   WORD      wSP, wSS;    /* stack pointer and segment */
   WORD      nEvents;     /* # of pending events */
   BYTE      bPriority,   /* priority (0 is highest) */
             bThread;     /* THREAD ORDINAL (?) */
   WORD      wThreadNext; /* 'Next' THREAD (?) */
   HTASK     hSelf;       /* SELF, or 'real' TDB (multi-thread?) */
   WORD      wThreadList, /* THREAD LIST (?) */
             wThreadFree, /* THREAD FREE (?) */
             wThreadCount;/* THREAD COUNT (?) */

   WORD      wFCW;        /* Floating Point Control Word! */

   WORD      wFlags;      /* the *REAL* task flags!! (Low byte only) */
   WORD      wErrMode;    /* error mode (0 for Windows, 1 for Task) */
   WORD      wExpWinVer;  /* expected windows version */
   HINSTANCE hInst;       /* instance handle for this task */
   HMODULE   hModule;     /* module handle for this task */

  /*** OFFSET 20H ***/

   HGLOBAL   hQueue;      /* handle to message queue (owned by 'USER'?) */
   HTASK     hParent;     /* TDB of 'parent' task (?) */
   WORD      wSigAction;  /* action for 'signal' proc */
   DWORD     dwSignal;    /* TASK signal proc address */
   DWORD     dwUSignal;   /* 'USER' signal proc address */
   DWORD     dwGNotify;   /* global notify proc (?) */

  /*** OFFSET 32H ***/

   DWORD     dwIntVect[7];/* 7 interrupt vectors (??) */

  /*** OFFSET 4EH ***/  /* LIM EMS AREA IS OBSOLETE (13 bytes) */

   WORD    wLIMSave;    /* LIM save area offset within TDB */
                        /* ALSO: COMPATIBILITY FLAGS */

   WORD    wEMSPID;     /* EMS PID for this task... */
   DWORD   dwEEMSSave;  /* far ptr to EEMS save area (in a TDB) ? */
   WORD    wEMSBCNT;    /* EMS bank count allocated so far */
   WORD    wEMSMAXBCNT; /* EMS Maximum bank count which this task wants */
   BYTE    bEMSRegSet,  /* EMS Register Set this TDB lives in */
	   bLibrary;    /* tracks adds/deletes of all libraries in system */

  /*** OFFSET 5CH ***/

   LPSTR   lpPHT;       /* far ptr to private handle table */
   WORD    wPDB;        /* the PDB (or PSP) - also at TDB:0x100 (aliased?) */
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

  } TDB;

typedef TDB FAR *LPTDB;


// TDB FLAGS ('wFlags' element, OFFSET 16H)

#define TDBF_WINOLDAP   1
#define TDBF_EMSSHARE   2
#define TDBF_CACHECHECK 4      /* internal - flags 'done already' */
#define TDBF_OS2APP     8
#define TDBF_WIN32S     16



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
typedef WOAENTRY4 FAR *LPWOAENTRY4;
typedef WOATABLE FAR *LPWOATABLE;
typedef WOATABLE4 FAR *LPWOATABLE4;
typedef WOATABLE30 FAR *LPWOATABLE30;






// CONTROL BLOCK - this is what a VM HANDLE points to (ABS address)!!

typedef struct {
   DWORD CB_VM_Status;        // for PM apps, it's 'PMCB_Flags'
   DWORD CB_High_Linear;      // for PM apps, it's 'PMCB_Parent'
   DWORD CB_Client_Pointer;   // see 'CB_CLIENT' (below)
   DWORD CB_VMID;
   } CONTROL_BLOCK;

typedef CONTROL_BLOCK FAR *LPCONTROL_BLOCK;



// 'CLIENT' structure (pointed to by 'CB_Client_Pointer' - VERIFIED!)
// this is pointed to by 'EBP' whenever a 'VxD' routine is entered.


typedef struct {
   DWORD Client_EDI, Client_ESI, Client_EBP, rsrv1;
   DWORD Client_EBX, Client_EDX, Client_ECX, Client_EAX;
   DWORD Client_Error;        // DWORD error code - see 'VMM.INC'
   DWORD Client_EIP;
   WORD  Client_CS, rsrv2;
   DWORD Client_EFlags, Client_ESP;
   WORD  Client_SS, rsrv3;
   WORD  Client_ES, rsrv4;
   WORD  Client_DS, rsrv5;
   WORD  Client_FS, rsrv6;
   WORD  Client_GS, rsrv7;
   DWORD Client_Alt_EIP;
   WORD  Client_Alt_CS, rsrv8;
   DWORD Client_Alt_EFlags, Client_Alt_ESP;
   WORD  Client_Alt_SS, rsrv9;
   WORD  Client_Alt_ES, rsrv10;
   WORD  Client_Alt_DS, rsrv11;
   WORD  Client_Alt_FS, rsrv12;
   WORD  Client_Alt_GS, rsrv13;

   } CB_CLIENT;

typedef CB_CLIENT FAR *LPCB_CLIENT;


// DDB structure for VxD's

typedef struct VxD_Desc_Block {
   DWORD DDB_Next;                // DD      ?
   WORD  DDB_SDK_Version;         // DW      DDK_VERSION
   WORD  DDB_Req_Device_Number;   // DW      UNDEFINED_DEVICE_ID
   BYTE  DDB_Dev_Major_Version;   // DB      0
   BYTE  DDB_Dev_Minor_Version;   // DB      0
   WORD  DDB_Flags;               // DW      0
   BYTE  DDB_Name[8];             // DB      "        "
   DWORD DDB_Init_Order;          // DD      UNDEFINED_INIT_ORDER
   DWORD DDB_Control_Proc;        // DD      ?
   DWORD DDB_V86_API_Proc;        // DD      0
   DWORD DDB_PM_API_Proc;         // DD      0
   DWORD DDB_V86_API_CSIP;        // DD      0
   DWORD DDB_PM_API_CSIP;         // DD      0
   DWORD DDB_Reference_Data;      // DD      ?
   DWORD DDB_Service_Table_Ptr;   // DD      0
   DWORD DDB_Service_Table_Size;  // DD      0
   DWORD DDB_Win32_Service_Table; // DD      0
   DWORD DDB_Prev;                // DD      'Prev'
   DWORD DDB_RegistryKey;         // DD      0
   DWORD DDB_Reserved1;           // DD      'Rsv1'
   DWORD DDB_Reserved2;           // DD      'Rsv2'
   DWORD DDB_Reserved3;           // DD      'Rsv3'
  } DDB, FAR *LPDDB;






// 'CB_VM_Status' flag values


#define VMStat_Exclusive            0x00000001L  /* VM is exclusive mode       */
#define VMStat_Exclusive_Bit        0
#define VMStat_Background           0x00000002L  /* VM runs in background      */
#define VMStat_Background_Bit       1
#define VMStat_Creating             0x00000004L  /* In process of creating     */
#define VMStat_Creating_Bit         2
#define VMStat_Suspended            0x00000008L  /* VM not scheduled           */
#define VMStat_Suspended_Bit        3
#define VMStat_Not_Executeable      0x00000010L  /* VM partially destroyed     */
#define VMStat_Not_Executeable_Bit  4
#define VMStat_PM_Exec              0x00000020L  /* Currently in PM app        */
#define VMStat_PM_Exec_Bit          5
#define VMStat_PM_App               0x00000040L  /* PM app present in VM       */
#define VMStat_PM_App_Bit           6
#define VMStat_PM_Use32             0x00000080L  /* PM app is 32-bit           */
#define VMStat_PM_Use32_Bit         7
#define VMStat_VxD_Exec             0x00000100L  /* Call from VxD              */
#define VMStat_VxD_Exec_Bit         8
#define VMStat_High_Pri_Back        0x00000200L  /* High pri background        */
#define VMStat_High_Pri_Back_Bit    9
#define VMStat_Blocked              0x00000400L  /* Blocked on semaphore       */
#define VMStat_Blocked_Bit          0Ah
#define VMStat_Awakening            0x00000800L  /* Woke up after blocked      */
#define VMStat_Awakening_Bit        0Bh
#define VMStat_PageableV86          0x00001000L  /* part of V86 is pageable (PM app) */
#define VMStat_PageableV86Bit       0Ch
#define VMStat_V86IntsLocked        0x00002000L  /* Rest of V86 is locked      */
#define VMStat_V86IntsLockedBit     0Dh          /* regardless of pager type   */
#define VMStat_TS_Sched             0x00004000L  /* Scheduled by time-slicer   */
#define VMStat_TS_Sched_Bit         0Eh
#define VMStat_Idle                 0x00008000L  /* VM has released time slice */
#define VMStat_Idle_Bit             0Fh
#define VMStat_Closing              0x00010000L  /* Close_VM called for VM     */
#define VMStat_Closing_Bit          10h

#define VMStat_Use32_Mask           VMStat_PM_Use32 OR VMStat_VxD_Exec









// PAGE FLAGS - 1 DWORD per page flag entry

#define P_PRES     0x01L             /* page present bit          */
#define P_PRESBit  0
#define P_WRITE    0x02L             /* write access bit          */
#define P_WRITEBit 1
#define P_USER     0x04L             /* access bit for User mode  */
#define P_USERBit  2
#define P_ACC      0x20L             /* page accessed bit         */
#define P_ACCBit   5
#define P_DIRTY    0x40L             /* page dirty bit            */
#define P_DIRTYBit 6



// 'OS RESERVED' bit definitions in PAGE FLAGS

#define PG_VM           0L
#define PG_SYS          1L
#define PG_RESERVED1    2L
#define PG_PRIVATE      3L
#define PG_RESERVED2    4L
#define PG_RELOCK       5L               /* PRIVATE to MMGR  */
#define PG_INSTANCE     6L
#define PG_HOOKED       7L
#define PG_IGNORE       0xFFFFFFFFL


#define PgT_VM        (PG_VM        << 9)
#define PgT_SYS       (PG_SYS       << 9)
#define PgT_RESERVED1 (PG_RESERVED1 << 9)
#define PgT_PRIVATE   (PG_PRIVATE   << 9)
#define PgT_RESERVED2 (PG_RESERVED2 << 9)
#define PgT_RELOCK    (PG_RELOCK    << 9)
#define PgT_INSTANCE  (PG_INSTANCE  << 9)
#define PgT_HOOKED    (PG_HOOKED    << 9)





// DEMANDINFOSTRUC structure (used by 'GetDemandPageInfo')

typedef struct DemandInfoStruc {

   DWORD DILin_Total_Count;       // # pages in linear address space
   DWORD DIPhys_Count;            // Count of phys pages
   DWORD DIFree_Count;            // Count of free phys pages
   DWORD DIUnlock_Count;          // Count of unlocked Phys Pages
   DWORD DILinear_Base_Addr;      // Base of pageable address space
   DWORD DILin_Total_Free;        // Total Count of free linear pages

//   DWORD DIReserved[10];          // Resvd for expansion

   // Win '95 stuff

   DWORD DIPage_Faults;           // total page faults
   DWORD DIPage_Ins;              // calls to pagers to page in
   DWORD DIPage_Outs;             // calls to pagers to page out
   DWORD DIPage_Discards;         // calls to pagers to discard
   DWORD DIInstance_Faults;       // instance page faults
   DWORD DIPagingFileMax;         // max size of paging file
   DWORD DIPagingFileInUse;       // # pages currently in use
   DWORD DICommit_Count;          // total committed pages
   DWORD DIReserved[2];           // reserved - don't use

   } DEMANDINFO;

typedef DEMANDINFO FAR *LPDEMANDINFO;



#pragma pack()




		   /**********************************/
		   /** EXTERNAL FUNCTION PROTOTYPES **/
		   /**********************************/



void FAR PASCAL DOS3Call();    // prototype not defined in WINDOWS.H




		   /**********************************/
		   /** INTERNAL FUNCTION PROTOTYPES **/
		   /**********************************/

BOOL FAR PASCAL GetUserInput(LPSTR lpTitle, LPSTR lpCaption,
                             LPSTR lpInputBuf, WORD cbBufSize);


void FAR PASCAL MySetCursor(WORD wFlag);
   // 'MySetCursor(0)' to assign DEFAULT cursor
   // 'MySetCursor(1)' to assign HOURGLASS cursor
   // 'MySetCursor(2)' to 'ROTATE' the hourglass!!


void FAR PASCAL CreateFileList(HWND hChild);
void FAR PASCAL CreateTaskList(HWND hChild);
void FAR PASCAL CreateMemList(HWND hChild);
void FAR PASCAL CreateVectorList(HWND hChild);
void FAR PASCAL CreateGDTList(HWND hChild);
void FAR PASCAL CreateLDTList(HWND hChild);
void FAR PASCAL CreateMemoryPageMap(HWND hChild);
void FAR PASCAL CreateVMList(HWND hChild);
void FAR PASCAL CreateVXDList(HWND hChild);

void FAR PASCAL TestFunction(HWND hChild);

int FAR _cdecl MyTaskListCompare(LPSTR lp1, LPSTR lp2);
int FAR _cdecl CMDMemSortProc(LPSTR lp1, LPSTR lp2);


void FAR PASCAL AddLine0(HWND hChild, LPSTR lpData);
void FAR PASCAL AddLine1(HWND hChild, LPSTR lpData);
void FAR PASCAL AddLine2(HWND hChild, LPSTR lpData);
void FAR PASCAL AddLine3(HWND hChild, LPSTR lpData);


		    /*** DPMI 'HELPER' FUNCTIONS ***/

WORD FAR PASCAL MyAllocSelector(void);
BOOL FAR PASCAL MyChangeSelector(WORD wSelector, DWORD dwAddr, WORD wLimit);
void FAR PASCAL MyFreeSelector(WORD wSelector);

DWORD FAR PASCAL MyGetSelectorBase(WORD wSelector);
DWORD FAR PASCAL MyGetSelectorLimit(WORD wSelector);
WORD FAR PASCAL MyGetAccessRights(WORD wSelector);

BOOL FAR PASCAL RealModeInt(WORD wIntNum, LPREALMODECALL lpIR);



		      /*** 'TOOLHELP' CLONES ***/

BOOL FAR PASCAL MyMemManInfo(LPMEMMANINFO lpInfo);

BOOL  FAR PASCAL MyTaskFirst(LPTASKENTRY lpTask);
BOOL  FAR PASCAL MyTaskNext(LPTASKENTRY lpTask);
HTASK FAR PASCAL MyTaskFindHandle(LPTASKENTRY lpTask, HTASK hTask);


		     /*** 'TOOLHELP' HELPERS ***/

BOOL FAR PASCAL MyIsTask(HTASK hTask);

BOOL FAR PASCAL IsWINOLDAP(HTASK hTask);
HTASK FAR PASCAL GetTaskFromInst(HINSTANCE hInst);

LPSTR FAR PASCAL GetTaskAndModuleFromPSP(WORD wMachID, WORD wPSPSeg,
                                         HTASK FAR *lphTask);

DWORD FAR PASCAL GetVMInfoFromTask(HTASK hTask, WORD FAR *lpwVMID,
                                   LPSTR lpAppName);
   // returns VM Handle (DWORD), plus VM ID and Active PSP's app name


LPWOATABLE FAR PASCAL GetWOATABLE();


                    /** SPECIAL WIN386 STUFF **/

DWORD FAR PASCAL MyGetSystemVM(void);   // RETURNS A 'VM HANDLE' (address32)
DWORD FAR PASCAL MyGetNextVM(DWORD dwPreviousVMHandle);

BOOL FAR PASCAL MyCopyPageTable(DWORD dwStartPage, DWORD dwPageCount,
                                DWORD FAR *lpdwPageInfo);
   // copies page table entry into 'lpdwPageInfo' buffer
   // 1 DWORD per page entry (same as hardware page table (?))


DWORD FAR PASCAL MyGetPgStatus(DWORD dwVMHandle, DWORD dwPage);
   // returns PAGE STATUS flags (see 'VMM.INC' and 'V86MMGR_GetPgStatus')

HWND FAR PASCAL MyGetVMInfo(DWORD dwVMHandle, DWORD FAR *lpdwRval);
   // returns VM info into 12 DWORD array 'lpdwRval' as follows:
   // [0]  == 'low' VM flags
   // [1]  == 'hi' VM flags (reserved - not yet in use)
   // [2]  == min EMS allocation
   // [3]  == max EMS allocation
   // [4]  == min XMS allocation
   // [5]  == max XMS allocation
   // [6]  == HMA Enabled/Disabled flag (1 == 'Enabled')
   // [7]  == Exclusive/Background flags (see 'Get_Time_Slice_Priority')
   // [8]  == FG/BG Priority; high word == Foreground, low word == Background
   // [9]  == CPU Time Slice 'Max % CPU usage' (calculated by VMM Scheduler)
   // [10] == total # of memory pages allocated to VM
   // [11] == # of pages allocated to VM which are not 'mapped in'
   // [12] == Total CPU time used by this VM since it was started
   //
   // For Chicago, function returns hWnd for WINOLDAP


void FAR PASCAL MyGetDemandPageInfo(LPDEMANDINFO lpDI);
   // gets 'demand page info' using VMM API



void FAR PASCAL MyGetVMPgCount(DWORD dwVMHandle, DWORD FAR *lpdwRval);
   // returns page count info like 'VMMcall _GetVMPgCount'.  The 1st
   // DWORD in 'lpdwRval' is the TOTAL page count; the 2nd DWORD is the
   // # of pages that are not 'mapped in'.


DWORD FAR PASCAL MyGetSysPageCount(void);
   // Returns page count info like 'VMMcall _GetSysPageCount'.  The
   // total # of 'PG_SYS' pages allocated in the system is returned,
   // which is (as it turns out) an estimate of the size of Win386!


DWORD FAR PASCAL MyGetPageDirectory(DWORD FAR *lpDir);
   // Obtains PAGE TABLE DIRECTORY (a 4k block of data)
   // return value is LINEAR address of page table directory

DWORD FAR PASCAL MyGetPageTable(DWORD dwPhysAddr, DWORD FAR *lpTable);
   // Obtains PAGE TABLE (a 4k block of data) using one of the DWORD
   // entries from the PAGE TABLE DIRECTORY.  (see above)
   // return value is LINEAR address of page table entry



void FAR PASCAL MyGetVXDList(DWORD FAR *lpdwRval);
   // returns list of VXD CB's, except for 1st entry [0] which is
   // the address of the 'DeviceInfo' structure used by VXDLDR.
   // Last entry in list is NULL.

DWORD FAR PASCAL MyGetVMMDDB(void);
   // returns linear address of DDB for VMM (should be first VxD).


void FAR PASCAL GetAppNameFromVMHandle(DWORD dwVMHandle, LPSTR lpAppName);
   // obtains "official" application name from control block address
   // by obtaining the CURRENT PSP from the DOS 'swappable' areas and
   // 'nuking out' the MCB address from it (and the app name - how nice!).
   // (this only works for NON-WINDOWS applications, by the way!)


   // This next function is an assembly language proc that allows
   // me to *REALLY* change a selector!!  The bits below apply to
   // 'normal' selector types.  Other types must be assigned manually,
   // where the low byte is the 'Access Rights' byte in the descriptor,
   // and the 'HIGH NYBBLE' is the 'extended attributes nybble' in
   // the descriptor (which only apply to 32-bit selectors).


BOOL FAR PASCAL MyChangeSelector2(WORD wSelector, DWORD dwAddr,
                                  DWORD dwLimit, WORD wFlags);

#define MCS2_CODE      0x0018
#define MCS2_DATA      0x0010
#define MCS2_ACCESS    0x0001
#define MCS2_EXECREAD  0x0002   /* APPLIES TO CODE SEGMENT */
#define MCS2_CONFORM   0x0004   /* APPLIES TO CODE SEGMENT */
#define MCS2_WRITEDATA 0x0002   /* APPLIES TO DATA SEGMENT */
#define MCS2_XPANDDOWN 0x0004   /* APPLIES TO DATA SEGMENT */

#define MCS2_DPL0      0x0000   /* for reference only - not required */
#define MCS2_DPL1      0x0020
#define MCS2_DPL2      0x0040
#define MCS2_DPL3      0x0060
#define MCS2_DPLMASK   0x0060
#define MCS2_PRESENT   0x0080   /* must be set, or a 'not present'    */
                                /* exception will occur when accessed */

//** 32-bit segment bits - only assign these for 32-bit segments **

#define MCS2_32BIT     0x4000   /* 32-bit when set; 16-bit when clear    */
                                /* REQUIRED if any of the next 3 are set */
#define MCS2_AVAIL     0x1000   /* 'AVAILABLE' bit                       */
#define MCS2_RESERVED  0x2000   /* 'RESERVED' bit - usable by app?       */
#define MCS2_PAGEGRAN  0x8000   /* PAGE granularity if set               */





                        /**********************/
			/** GLOBAL VARIABLES **/
			/**********************/


extern char pClassName[];
extern char pMenuName[];
extern char pHelpFileName[];

char pq[];
char *pOpenMode[8];
char *pShareMode[8];

extern WORD PASCAL __ahincr;

extern char buffer[256];

extern HINSTANCE hInst;
extern HWND hMainWnd;
extern WORD wIGROUP;




extern HMODULE hToolHelp;
extern HMODULE hKernel;


extern BOOL (FAR PASCAL *lpGlobalFirst)(LPGLOBALENTRY lpGlobal, WORD wFlags);
extern BOOL (FAR PASCAL *lpGlobalNext)(LPGLOBALENTRY lpGlobal, WORD wFlags);
extern BOOL (FAR PASCAL *lpMemManInfo)(LPMEMMANINFO lpInfo);
extern BOOL (FAR PASCAL *lpModuleFirst)(LPMODULEENTRY lpModule);
extern BOOL (FAR PASCAL *lpModuleNext)(LPMODULEENTRY lpModule);
extern HTASK (FAR PASCAL *lpTaskFindHandle)(LPTASKENTRY lpTask, HTASK hTask);
extern HMODULE (FAR PASCAL *lpModuleFindHandle)(LPMODULEENTRY lpModule,
						HMODULE hModule);

extern BOOL (FAR PASCAL *lpIsTask)(HTASK hTask);
extern BOOL (FAR PASCAL *lpTaskFirst)(LPTASKENTRY lpTask);
extern BOOL (FAR PASCAL *lpTaskNext)(LPTASKENTRY lpTask);

extern DWORD (FAR PASCAL *lpGlobalMasterHandle)(void);
extern DWORD (FAR PASCAL *lpGlobalHandleNoRIP)(WORD wSeg);






#endif /* RC_INVOKED */
