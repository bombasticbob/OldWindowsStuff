/***************************************************************************/
/*                                                                         */
/*   WINCMD.H - Command line interpreter for Microsoft(r) Windows (tm)     */
/*                                                                         */
/*                      TRADE SECRET - CONFIDENTIAL                        */
/*                                                                         */
/*             The compiled libraries and API header files are             */
/*          (c) Copyright 1990-1995 by Stewart~Frazier Tools, Inc          */
/*                                                                         */
/*    This file contains ID and structure definitions common to all        */
/*    source files.  Each affected source file should include this         */
/*    as a build dependency within the make file.                          */
/*                                                                         */
/***************************************************************************/


#pragma warning(disable:4200 4704)

#include "resource.h"

// STRING TABLE ID's FOR ERROR MESSAGES

#define CMDERROR_CLASS  256    /* 0x100 */
#define CMDERROR_ACTION 320    /* 0x140 */
#define CMDERROR_LOCUS  384    /* 0x180 */



#define CMDDDE_ADVISE     0
#define CMDDDE_EXECUTE    1
#define CMDDDE_INITIATE   2
#define CMDDDE_POKE       3
#define CMDDDE_REGISTER   4
#define CMDDDE_REQUEST    5
#define CMDDDE_TERMINATE  6
#define CMDDDE_UNADVISE   7
#define CMDDDE_UNREGISTER 8


#define CMDODBC_BIND        0     /* bind field columns */
#define CMDODBC_CANCEL      1     /* cancel transaction or SQL */
#define CMDODBC_CONNECT     2     /* connect to DSN */
#define CMDODBC_CREATE      3     /* create STATEMENT */
#define CMDODBC_CURSOR      4     /* set current named cursor */
#define CMDODBC_DELETE      5     /* delete current record */
#define CMDODBC_DISCONNECT  6     /* disconnect [ALL] */
#define CMDODBC_DROP        7     /* drop STATEMENT */
#define CMDODBC_ERROR       8     /* get error message */
#define CMDODBC_EXECUTE     9     /* execute SQL block */
#define CMDODBC_FILTER      10    /* set 'filter' (WHERE clause) */
#define CMDODBC_FIRST       11    /* first record in dynaset/table */
#define CMDODBC_INSERT      12    /* insert record into table */
#define CMDODBC_LAST        13    /* last record in dynaset/table */
#define CMDODBC_LOCK        14    /* lock RECORD|TABLE */
#define CMDODBC_NEXT        15    /* next record in dynaset/table */
#define CMDODBC_OPEN        16    /* open a table for read/write access */
#define CMDODBC_PARAMETERS  17    /* assign parameters for SQL */
#define CMDODBC_PREVIOUS    18    /* previous record in dynaset/table */
#define CMDODBC_SQL         19    /* begins an 'SQL' block */
#define CMDODBC_TRANSACT    20    /* begin a transaction */
#define CMDODBC_UNLOCK      21    /* unlock RECORD|TABLE */
#define CMDODBC_UPDATE      22    /* update the current record */



#ifndef COLOR_3DLIGHT
#define COLOR_3DDKSHADOW 21
#define COLOR_3DLIGHT    22
#define COLOR_INFOTEXT   23
#define COLOR_INFOBK     24
#endif  // COLOR_3DLIGHT


#ifndef RC_INVOKED

                       /*************************/
                       /***    MACROS AND     ***/
                       /*** DEFINED CONSTANTS ***/
                       /*************************/

#define GlobalSizePtr(X) (GlobalSize(GlobalPtrHandle(X)))

#ifdef WIN32
#define HUGE
#else
#define HUGE __huge
#endif // WIN32

#define WORDPTR(X)  ((WORD FAR *)((void FAR *)&(X)))
#define BYTEPTR(X)  ((BYTE FAR *)((void FAR *)&(X)))
#define DWORDPTR(X) ((DWORD FAR *)((void FAR *)&(X)))


#define SCREEN_LINES iScreenLines     /* max # of lines in virtual screen */
#define SCREEN_COLS  80               /* max # of columns in virtual screen */


#define COPY_PRI 7         /* prime value for less thrashing */
#define FORMAT_PRI 11      /* somewhat worse than 'copy' */
#define BATCH_PRI 3        /* significantly better than any other */


#define N_DIMENSIONS(X) ((sizeof(X))/(sizeof(*(X))))


#define INIT_LEVEL    do {
#define END_LEVEL     } while(0);

#define MAX_PATH_SIZE 256

#define INITIAL_TREE_SIZE 256   /* for 'GetDirTree()' function */


#define ENV_MEM_FLAGS      GMEM_FIXED | GMEM_ZEROINIT | GMEM_DDESHARE
#define ENV_REALLOC_FLAGS  GMEM_MOVEABLE | GMEM_ZEROINIT


// DEVICE attribute (new for Win '95)
#define _A_DEV 0x40  /* taken from 'FILE_ATTRIBUTE_DIRECTORY' from IFS.H in DDK */


                /* definitions for 'QualifyPath()' */

//#define QUAL_PATH_SIZE  256    /* more than enough room, I am sure! */
//#define QUAL_DRIVE_SIZE 64     /* 64 bytes in case of network drive */
//#define QUAL_DIR_SIZE   256    /* 128 bytes just to make sure! */
//#define QUAL_NAME_SIZE  256    /* no more than 8 + 1, but 20 to make sure */
//#define QUAL_EXT_SIZE   12     /* no more than 4 + 1, but 20 to make sure */

#define QUAL_PATH_SIZE    MAX_PATH
#define QUAL_DRIVE_SIZE   MAX_PATH
#define QUAL_DIR_SIZE     MAX_PATH
#define QUAL_NAME_SIZE    MAX_PATH
#define QUAL_EXT_SIZE     MAX_PATH


              /* definitions for 'GetPathNameInfo()' */

#define PATH_HAS_DRIVE  0x1    /* drive present in path name */
#define PATH_HAS_DIR    0x2    /* directory present in path name */
#define PATH_HAS_ROOT   0x4    /* root dir present in path name */
#define PATH_HAS_NAME   0x8    /* file name present in path name */
#define PATH_HAS_EXT    0x10   /* extension present in path name */
#define PATH_IS_WILD    0x80   /* wildcards present in path name */


#define CMD_EXACT    0x8000
#define CMD_ATLEAST  0x4000
#define CMD_ATMOST   0x2000
#define CMD_DONTCARE 0x0000
#define CMD_N_MASK   0x00ff
#define CMD_T_MASK   (~(CMD_N_MASK))


                 /* definitions for FORMAT flags */

#define FORMAT_DEFAULT       0x0
#define FORMAT_UNCONDITIONAL 0x100   /* perform 'unconditional' format */
#define FORMAT_QUICKFORMAT   0x200   /* perform 'quick' format, no verify */
#define FORMAT_SYSTEMFILES   0x400   /* transfer system files to drive */
#define FORMAT_BOOTDRIVEMASK 0xff    /* masks out boot drive within flags */



#define DIRINFO_MIN_MEM (256 * sizeof(DIR_INFO))

#define DIRINFO_ERROR   0xffff        /* returned by GetDirList() on error */

#define DIRLIST_ONLY    0x8000        /* special bit mask for attributes.. */
                                    /* 'only' --> matches these attributes */
#define COPY_BUFSIZE    0x8000                 /* 32k buffers (for now...) */

#define OF_ERROR        ((HFILE)-1)

#define CMDHIST_SIZE    0x1000         /* size of command history buffer!! */

#define BATCH_BUFSIZE   2048           /* 2k buffer for batch files */

#define NE_SINGLEDATA    1
#define NE_MULTIPLEDATA  2
#define NE_NONWLOCOMPAT  0x0100    /* WINDOWS API for OS/2 not compatible */
#define NE_WLOCOMPATIBLE 0x0200    /* WINDOWS API for OS/2 compatible */
#define NE_PMAPPLICATION (0x0100+0x0200) /* both bits must be set */
#define NE_SELFLOADING   0x0800
#define NE_LINKERRORS    0x2000
#define NE_RESERVED      0x4000
#define NE_LIBRARY       0x8000  /* this is a DLL!!! */

#define NEOS_UNKNOWN     0x00
#define NEOS_OS2         0x01  /* OS/2 */
#define NEOS_WINDOWS     0x02  /* MS Windows */
#define NEOS_RESERVED3   0x03
#define NEOS_RESERVED4   0x04
#define NEOS_OSFLAGS     0x0f
    /* the following flags are combined with those above */
#define NEOS_MARKEDEXE   0x0100  /* a 'MARKED' 2.x Windows application */
#define NEOS_MARKEDFONT  0x0200  /* a 'MARKED' 2.x WinApp that supports the */
                                 /* 3.x proportional fonts */
#define NEOS_FASTLOAD    0x0400  /* exe file contains a 'fast load' area */


                      /* 'FindProgram' return flags */


#define PROGRAM_PE         3        /* 32-bit NT app */
#define PROGRAM_L32        2        /* 'Lx' app that's marked 'windows' */
#define PROGRAM_WIN16      1        /* 16-bit Win app */
#define PROGRAM_FALSE      0        /* NOT a program file */
#define PROGRAM_NONWINDOWS -1       /* non-windows app (generic) */
#define PROGRAM_OS2        -2       /* OS/2 16-bit application */
#define PROGRAM_OS2_32     -3       /* OS/2 32-bit application (?) */
#define PROGRAM_VXD        -4       /* Windows VXD */
#define PROGRAM_COM        -5       /* non-windows COM file */
#define PROGRAM_BAT        -16      /* BATCH file (.BAT extension) */
#define PROGRAM_PIF        -17      /* PIF file (.PIF extension) */

#define PROGRAM_FOLDER      0x1000  /* set for FOLDERS in Win 4+ */
#define REXX_COMMAND        0x2000  /* set for REXX command files */
#define PROGRAM_ASSOCIATION 0x4000  /* set when program is 'associated' */



                   /* buffer size and other definitions */

#define STD_BUFFER_SIZE   256   /* standard size for a buffer of some kind */
#define COMMAND_HIST_SIZE 32    /* 32 commands in command history list */


                       /*************************/
                       /*** CUSTOM DATA TYPES ***/
                       /*************************/


typedef BOOL (FAR PASCAL CMD_FN)(LPSTR lpCmd, LPSTR lpArgs);

#ifdef WIN32
typedef char *SBPCHAR;
#else  // WIN32
typedef char _based((_segment)_self) *SBPCHAR;
#endif // WIN32

typedef union tagWIN_VER {                      /* returned by GetVersion() */
   DWORD dw;
   struct {
      BYTE win_major,win_minor;
      BYTE dos_minor,dos_major;
      } ver;
   } WIN_VER;


typedef struct tagPARSE_INFO {
   SBPCHAR sbpProgram;                /* program name (NULL for 1st one) */
   SBPCHAR sbpCommand;                /* pointer to command line parms   */
   SBPCHAR sbpOutFile;                /* 'NULL' --> 'stdout' (the screen) */
   SBPCHAR sbpInFile;                 /* 'NULL' --> 'stdin' (the keyboard) */
   HFILE   hOutFile;                  /* 'NULL' --> 'stdout' closed */
   HFILE   hInFile;                   /* 'NULL' --> 'stdin' closed */
   WORD n_switches;
   WORD n_parameters;
   SBPCHAR sbpPipeNext; // points directly to the command that follows the '|'
                        // NULL if no '|' is present in the command line.
                        // 'ReDirectOutput(lpParseInfo,FALSE)' executes it

   SBPCHAR sbpItemArray[0];             /* switches followed by parameters */

   } PARSE_INFO;

typedef PARSE_INFO FAR *LPPARSE_INFO;


typedef struct tagDIR_INFO { /* size is 32 bytes */
   char name[8], ext[3];     /* padded with spaces, not null terminated */
   char attrib;              /* attribut byte, of course. */
   WORD date, time;          /* file date/time from _dos_findfirst(), etc. */
   DWORD size;               /* file size (in bytes) */
   char fullname[12];        /* full name with '.', padded with nulls */
   } DIR_INFO;

typedef DIR_INFO FAR *LPDIR_INFO;
typedef DIR_INFO HUGE *HPDIR_INFO;


typedef struct tagTREE {
   char szDir[16];
   WORD wSibling;        /* next 'sibling' element in owning branch */
   WORD wChild;          /* first 'child' element on this branch */
   } TREE;
typedef TREE FAR *LPTREE;


typedef int (FAR _cdecl *LPCOMPAREPROC)(LPSTR p1, LPSTR p2);

typedef struct tagQSORTVARS {
   int (FAR _cdecl *cmp)(LPSTR p1, LPSTR p2,
                         struct tagQSORTVARS STACK_BASED *bpqsv);
   char HUGE *_base;
   DWORD _num, _width;
   char HUGE *hp;
   } QSORTVARS;
typedef QSORTVARS FAR *LPQSORTVARS;
typedef QSORTVARS STACK_BASED *SBPQSORTVARS;


typedef struct tagCOPY_INFO {
   struct tagCOPY_INFO FAR *lpNext;      /*  next structure (NULL FOR END) */
   SBPCHAR source, dest, srcpath;    /* points to source, dest, & src path */
   SBPCHAR next_source;              /* pointer to next source if '+' used */
   SBPCHAR original_end; /* points to the 'original end' for the structure */
   SBPCHAR buf_start;                   /* points to 'COPY_BUFSIZE' buffer */
                          /* this is NULL if they haven't been initialized */
   WORD wNFiles, wCurFile;             /* Total # of files, current file # */
   LPDIR_INFO lpDirInfo;      /* pointer to DIR INFO for current file spec */
                                   /* this is always NULL if '+' is used!! */

   BOOL def_binary, fromfile_binary, tofile_binary, verify, modify;
                                     /* copy flags (obvious) */
   BOOL SrcIsCON, DestIsCON;         /* more 'CON' flags */

   HFILE hSrc, hDest;                /* file handles for source/dest files */
   OFSTRUCT ofsrc, ofdest;     /* corrent open/re-open buf for source/dest */
   BOOL file_is_open;           /* TRUE if file currently open; else FALSE */
   } COPY_INFO;

typedef COPY_INFO FAR *LPCOPY_INFO;




typedef struct tagSMARTFILEPARMENTRY {

   char szVarName[12];     // up to 12 chars per var name, excluding '.'
   char szValue[32];       // 'lpValue' may point here
   LPSTR lpValue;          // pointer to actual value

   } SMARTFILEPARMENTRY, FAR *LPSMARTFILEPARMENTRY;


typedef struct tagSMARTFILEPARM {

   WORD nEntryCount;
   SMARTFILEPARMENTRY pEntry[1];  // declare one, but expect many

   } SMARTFILEPARM, FAR *LPSMARTFILEPARM;



// 'LoadModule' and 'DosRunProgram' use these structures

typedef struct tagDOSRUNCMDSHOW {
  WORD wTwo;
  WORD nCmdShow;
  } DOSRUNCMDSHOW, FAR *LPDOSRUNCMDSHOW;

typedef struct tagDOSRUNPARMBLOCK {
  WORD wEnvSeg;
  LPSTR lpCmdLine;
  LPDOSRUNCMDSHOW lpCmdShow;
  LPSTR lpReserved;
  } DOSRUNPARMBLOCK, FAR *LPDOSRUNPARMBLOCK;







#define CODEBLOCK_TYPEMASK   0xf
#define CODEBLOCK_NONE       0
#define CODEBLOCK_FOR        1
#define CODEBLOCK_IF         2
#define CODEBLOCK_WHILE      3
#define CODEBLOCK_REPEAT     4
#define CODEBLOCK_DEFINE     5 /* defined function or proc */
#define CODEBLOCK_DEFINE_EX  6 /* EXTERNAL 'DLL' defined function */
#define CODEBLOCK_CALL       7 /* special use on 'GOSUB' for 'RETURN' */

#define CODEBLOCK_IF_TRUE      0x10 /* set when the 'IF' condition was TRUE */
                                    /* so that 'ELSE' is not processed.     */
#define CODEBLOCK_IF_IGNORE    0x20 /* set when current cond block is FALSE */
                                    /* and code should be ignored until the */
                                    /* next 'ELSE IF' or 'ELSE' or 'END IF' */
#define CODEBLOCK_IF_IGNOREALL 0x40 /* set when at least one condition has  */
                                    /* been TRUE in IF/ELSE IF/ENDIF        */


typedef struct {
   DWORD dwType;          // indicates type of code block and status

   DWORD dwStart;         // starting position within the batch file
                          // for EXTERNAL DEFINE, this is the DLL handle
   DWORD dwBlockEnd;      // ending position (-1L if not known or N/A)
                          // for EXTERNAL DEFINE, this is the PROC ADDRESS

   char szInfo[256 - 3 * sizeof(DWORD)];
                          // additional info (as a string) that's specific
                          // to each type of code block.
   } CODE_BLOCK;


typedef struct {
   DWORD dwEntryCount;        // total # of entries in the array
   CODE_BLOCK lpCodeBlock[1]; // array of 'CODE BLOCK' structures

   } BLOCK_INFO, FAR *LPBLOCK_INFO;


typedef struct tagBATCH_INFO {
   struct tagBATCH_INFO FAR *lpPrev;  /* where to return to when complete */
   LPSTR lpLabelList;        /* the LABELS (separated by nulls - sort of) */
                             /* initially NULL, assigned when label added */
   LPSTR lpArgList;          /* the command parameters (separated by nulls) */
                             /* initialized to point just past structure */

   LPBLOCK_INFO lpBlockInfo; /* pointer to info for 'FOR', 'IF', 'WHILE', */
                             /* and other 'code blocks'.                  */
                             /* NOTE:  This is a 'GlobalAlloc()'d buffer! */

   WORD  nArgs;              /* number of arguments in 'lpArgList' */
   WORD  nShift;             /* number of shifts we've done so far */
   HFILE hFile;              /* file handle for open file, or -1 if closed */
   OFSTRUCT ofstr;           /* open file structure */
   DWORD buf_start;          /* file position for buffer start */
   WORD  buf_pos, buf_len;   /* current position & length within buffer */
   BYTE  buf[BATCH_BUFSIZE]; /* the file i/o buffer */
   } BATCH_INFO;
typedef BATCH_INFO FAR *LPBATCH_INFO;


typedef struct tagFORMAT_REQUEST {
   WORD wDrive, wTracks, wHeads, wSectors;
   WORD wSwitchFlag;
   char label[12];
   struct tagFORMAT_REQUEST FAR *lpNext;  /* NULL if last in list */
   } FORMAT_REQUEST;

typedef FORMAT_REQUEST FAR *LPFORMAT_REQUEST;



typedef struct tagEXEHDR {
   BYTE id_sig[2];            /* 0x4d, 0x5a - 'M' 'Z' */
   WORD len_mod_512;
   WORD len_div_512;
   WORD nRelocItems;
   WORD header_size_div_16;
   WORD min_paragraphs_needed;
   WORD max_paragraphs_needed;
   WORD SS_displacement;
   WORD SP_on_entry;
   WORD checksum;
   WORD IP_on_entry;
   WORD CS_displacement;
   WORD first_reloc_item;     /* >= 0x40 for 'NEW' exe format */
   BYTE overlay_number;       /* this is zero for resident part of program */
   BYTE reserved[0x3c - 0x1b];/* reserved area prior to 'new exe' offset */
   WORD new_exe_offset;
   WORD reserved2;            /* this is always available, but for ??? */
} EXEHDR;
typedef EXEHDR FAR *LPEXEHDR;



typedef struct tagNEWEXEHDR {
   BYTE id_sig[2];       /* 0x4e, 0x45 - 'N' 'E' ( 'L' 'E' for .386 files) */
   BYTE link_version[2]; /* version, revision of linker */
   WORD entry_table;     /* offset to entry table from beg. of new header */
   WORD entry_table_size;/* size of entry table */
   WORD reserved1, reserved2;
   WORD wFlags;          /* see the 'NE' flags (above) */
   WORD wDS_segment;     /* segment # for automatic data */
   /* OFFSET 0x10 */
   WORD wLocalHeapSize;  /* initial size of local heap */
   WORD wStackSize;      /* initial size of stack */
   DWORD dwCSIP;         /* initial CS:IP */
   DWORD dwSSSP;         /* initial SS:SP */
   WORD wSegTableSize;   /* number of entries in seg table */
   WORD wModuleRefSize;  /* numbef of entries in module reference table */
   /* OFFSET 0x20 */
   WORD wNonResNameSize; /* number of bytes in nonresident-name table */
   WORD wOffsetSegTable; /* offset to seg table from beg. of new header */
   WORD wOffsetResTable; /* offset to resource table from beg. of new hdr */
   WORD wOffsetResName;  /* offset to resident name tbl from beg. of new hdr */
   WORD wOffsetModRef;   /* offset to module ref tbl from beg. of new hdr */
   WORD wOffsetImpName;  /* offset to import name tbl from beg. of new hdr */
   DWORD dwOffsetNonRes; /* offset to nonresident-name tbl from beg of file */
   /* OFFSET 0x30 */
   WORD wMovableEntryPt; /* number of movable entry points */
   WORD wShiftCount;     /* the log base 2 of the 'align' switch used in the */
                         /* linker command line; '/A:512' ==> '9'   */
   WORD wNumResourceSegs;/* number of resource segments */
   WORD wOSFlag;         /* target operating system - see 'NEOS' flags */
   WORD wFastLoadArea;   /* beginning of fast-load area (windows only) */
   WORD wLenFastLoad;    /* length of fast-load area (windows only) */
   WORD wReserved4;
   WORD wExpectedWinVer; /* expected windows version (windows only) */
   /* OFFSET 0x40 */

} NEWEXEHDR;
typedef NEWEXEHDR FAR *LPNEWEXEHDR;


typedef struct tagDATE {       /* used by data conversion utilities   */
     unsigned int year;        /* equivalent structure to 'dosdate_t' */
     unsigned char month, day;
     } SFTDATE;

typedef SFTDATE FAR *LPSFTDATE;


typedef struct tagTIME {
     unsigned char hour, minute, second, tick;
     } SFTTIME;

typedef SFTTIME FAR *LPSFTTIME;


typedef struct tagREALMODECALL {   /* used by DPMI to call real mode int */
 DWORD EDI,ESI,EBP,reserved,EBX,EDX,ECX,EAX;
 WORD  flags,ES,DS,FS,GS,IP,CS,SP,SS;
 } REALMODECALL;

typedef REALMODECALL FAR *LPREALMODECALL;


typedef GLOBALENTRY FAR *LPGLOBALENTRY;
typedef MEMMANINFO FAR *LPMEMMANINFO;
typedef MODULEENTRY FAR *LPMODULEENTRY;
typedef TASKENTRY FAR *LPTASKENTRY;






/***************************************************************************/


         /*****************************************************/
         /** The following type definitions are for ListOpen **/
         /** and anything else that might access the SFT.    **/
         /** NOTE:  THE PACKING FOR THESE STRUCTURES IS AT   **/
         /**        THE 'BYTE' LEVEL INTENTIONALLY!!!!!      **/
         /*****************************************************/


#pragma pack(1)         /* byte packing for these structures!! */


/*typedef*/ struct _sft2_ {      /* DOS 2.x SYSTEM FILE TABLE */
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

/*typedef*/ struct _sft30_ {     /* DOS 3.0 SYSTEM FILE TABLE */
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

/*typedef*/ struct _sft31_ {     /* DOS 3.x SYSTEM FILE TABLE */
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

/*typedef*/ struct _sft4_ {      /* DOS 4+ SYSTEM FILE TABLE */
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
   char lpData[0];
   } SFT;
typedef SFT FAR *LPSFT;


          /** Additional structures used by 'CMDListOpen' **/

typedef struct {
   char szName[16]; /* 15 chars plus NULL, blank padded */
   unsigned char bMode, bShare;
   WORD wVM, wPSP;
   DWORD dwFilePtr, dwFileSize;
   char ExtraLine[98];              // total size: 128 bytes
   } SFT_INFO;

typedef SFT_INFO FAR *LPSFT_INFO;


typedef struct {
   unsigned char type;  /* the letter 'M', or 'Z' */
   unsigned wOwnerPSP;  /* the DOS PSP of the owner */
   unsigned size;
   unsigned char unused[3];
   unsigned char dos4[8]; /* the application name, if dos 4+ */
   } MCB;
typedef MCB FAR *LPMCB;





#define DOSVER_2X    0
#define DOSVER_30    1
#define DOSVER_3X    2
#define DOSVER_4PLUS 3
#define DOSVER_7PLUS 4




                 /** OTHER 'DOS INTERNAL' STRUCTURES **/



           /** DPB LAYOUT - DOS 4.x and later **/

typedef struct {
   BYTE drive;
   BYTE unit;
   WORD sectorsize;
   BYTE clustersize;  /* actually clustersize - 1 == largest sector # */
   BYTE clustershift; /* the log2(clustersize + 1)                    */
   WORD reservedsectors; /* # of reserved sectors (boot area) */
   BYTE NFats;           /* # of FAT tables */
   WORD wRootEntries;    /* # of root entries (max) */
   WORD wFirstDataSector; /* location of 1st 'data' sector on medium */
   WORD wMaxCluster;      /* largest cluster # on medium */
   WORD wFatSize;         /* # of sectors in one FAT (prior to 4.0 == BYTE) */
   WORD wRootDirSector;   /* first sector of root dir */
   DWORD dwDriverAddress; /* address of device driver */
   BYTE MediaByte;        /* media descriptor byte */
   BYTE AccessFlag;    /* 'ffH' if block 'must be built'; '00' if accessed */
   DWORD dwNextDriver; /* 'FFFFH' for last driver, else address of next */
   WORD wNextFree;     /* next free cluster to search at for writing */
   WORD wNumFree;      /* # of free clusters, or FFFFH if unknown */
   } DPB;

typedef DPB FAR *LPDPB;



           /** DPB LAYOUT - DOS 3.x and earlier **/

typedef struct {
   BYTE drive;
   BYTE unit;
   WORD sectorsize;
   BYTE clustersize;  /* actually clustersize - 1 == largest sector # */
   BYTE clustershift; /* the log2(clustersize + 1)                    */
   WORD reservedsectors; /* # of reserved sectors (boot area) */
   BYTE NFats;           /* # of FAT tables */
   WORD wRootEntries;    /* # of root entries (max) */
   WORD wFirstDataSector; /* location of 1st 'data' sector on medium */
   WORD wMaxCluster;      /* largest cluster # on medium */
   BYTE wFatSize;         /* # of sectors in one FAT (prior to 4.0 == BYTE) */
   WORD wRootDirSector;   /* first sector of root dir */
   DWORD dwDriverAddress; /* address of device driver */
   BYTE MediaByte;        /* media descriptor byte */
   BYTE AccessFlag;    /* 'ffH' if block 'must be built'; '00' if accessed */
   DWORD dwNextDriver; /* 'FFFFH' for last driver, else address of next */
   WORD wNextFree;     /* next free cluster to search at for writing */
   WORD wNumFree;      /* # of free clusters, or FFFFH if unknown */
   } DPB3;

typedef DPB3 FAR *LPDPB3;



#define CURDIR_TYPE_MASK      0xC000  /* use on 'wFlags' element (below) */
#define CURDIR_TYPE_NETWORK   0x8000
#define CURDIR_TYPE_IFS       0xc000
#define CURDIR_TYPE_PHYSICAL  0x4000
#define CURDIR_TYPE_INVALID   0

#define CURDIR_JOIN           0x2000  /* set if 'JOIN'ed */
#define CURDIR_SUBST          0x1000  /* set if 'SUBST'ed */


      /** 'CURRENT DIRECTORY' STRUCTURE - DOS 3.x and earlier **/

typedef struct {
   char  pathname[67];  /* begins with 'X:\' or '\\machine\' */
   WORD  wFlags;        /* see 'CURDIR' flags (above) */
   DWORD dwDPB;         /* points to DPB for this drive */

   WORD  wDirStartClust;/* start cluster for DIR; 0==ROOT, FFFFH==unaccessed */
                        /* for NETWORK: lo-word of REDIRIFS pointer */
   WORD  wNetPtrHi;     /* for NETWORK: hi-word of REDIRIFS pointer */

   WORD  wInt21_5F03;   /* for NETWORK: stored WORD from Int 21H AX=5F03H */

   WORD  wPathRootOffs; /* offset of ROOT in path (2 if not SUBST/JOIN; else */
                        /* points to the end of the SUBST/JOIN path). This   */
                        /* always points to the 1st '\' in the apparent path */

   } CURDIR3;

typedef CURDIR3 FAR *LPCURDIR3;



       /** 'CURRENT DIRECTORY' STRUCTURE - DOS 4.x and later **/

typedef struct {
   char  pathname[67];  /* begins with 'X:\' or '\\machine\' */
   WORD  wFlags;        /* see 'CURDIR' flags (above) */
   DWORD dwDPB;         /* points to DPB for this drive */

   WORD  wDirStartClust;/* start cluster for DIR; 0==ROOT, FFFFH==unaccessed */
                        /* for NETWORK: lo-word of REDIRIFS pointer */
   WORD  wNetPtrHi;     /* for NETWORK: hi-word of REDIRIFS pointer */

   WORD  wInt21_5F03;   /* for NETWORK: stored WORD from Int 21H AX=5F03H */

   WORD  wPathRootOffs; /* offset of ROOT in path (2 if not SUBST/JOIN; else */
                        /* points to the end of the SUBST/JOIN path). This   */
                        /* always points to the 1st '\' in the apparent path */

   /** DOS 4 and later **/

   BYTE  wDOS4Byte;     /* UNKNOWN usage (network?) */
   DWORD dwIFSPtr;      /* points to IFS, or 0L for 'native DOS' */
   WORD  wDOS4Word2;    /* another UNKNOWN usage */

   } CURDIR;

typedef CURDIR FAR *LPCURDIR;


#pragma pack()                      /** RESTORE DEFAULT STRUCTURE PACKING **/


/***************************************************************************/





#endif /* RC_INVOKED */
