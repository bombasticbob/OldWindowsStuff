/***************************************************************************/
/*                                                                         */
/*   WINCMD_D.C - Command line interpreter for Microsoft(r) Windows (tm)   */
/*                                                                         */
/*                      TRADE SECRET - CONFIDENTIAL                        */
/*                                                                         */
/*             The compiled libraries and API header files are             */
/*          (c) Copyright 1990-1995 by Stewart~Frazier Tools, Inc          */
/*                                                                         */
/*   This contains the 'DBLSPACE' functions specific to DBLSPACE drives.   */
/*                                                                         */
/***************************************************************************/


#define THE_WORKS
#include "mywin.h"

#include "mth.h"
#include "wincmd.h"         /* 'dependency' related definitions */
#include "wincmd_f.h"       /* function prototypes and variables */


#if defined(M_I86LM)

  #pragma message("** Large Model In Use - Single Instance Only!! **")

#elif !defined(M_I86MM)

  #error  "** Must use MEDIUM model to compile, silly!! **"

#else

  #pragma message("** Medium Model Compile - Multiple Instances OK **")

#endif


#ifndef WM_FILESYSCHANGE

#define WM_FILESYSCHANGE    0x0034              /* ;Internal */

#endif  /* WM_FILESYSCHANGE */



                      /** DBLSPACE ERROR CODES **/

#define I2F_ERR_BAD_FN           0x100
#define I2F_ERR_BAD_DRV          0x101
#define I2F_ERR_NOT_COMPR        0x102
#define I2F_ERR_ALREADY_SWAPPED  0x103
#define I2F_ERR_NOT_SWAPPED      0x104




            /**様様様様様様様様様様様様様様様様様様様様様様**/
	    /**    Special structure definition for TDB    **/
	    /** (Hacked-up and corrected version from DDK) **/
	    /**    (the DDK version had errors in it!!)    **/
	    /**様様様様様様様様様様様様様様様様様様様様様様**/
	    /** (this structure also located in WINCMD_U!) **/
	    /**様様様様様様様様様様様様様様様様様様様様様様**/

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



typedef struct tagDOSDIRENTRY {
   char name[8];            // 00H-07H
   char ext[3];             // 08H-0aH
   char attrib;             // 0bH
   char reserved1[4];       // 0cH to 0fH
   char reserved2[6];       // 10H to 15H
   unsigned short time;     // 16H to 17H
   unsigned short date;     // 18H to 19H
   unsigned short cluster;  // 1aH to 1bH      STARTING CLUSTER!
   unsigned long  size;     // 1cH to 1fH      TOTAL SIZE (bytes)
   } DOSDIRENTRY;

typedef DOSDIRENTRY FAR *LPDOSDIRENTRY;





          // THE FOLLOWING STRUCTURES WERE STOLEN FROM 'CVF.H'


#pragma pack(1)

typedef struct {

    BYTE    jmpBOOT[3];     // Jump to bootstrap routine
    char    achOEMName[8];  // OEM Name ("MSDBL6.0")

    /*
     *  The following fields are a clone of an MS-DOS BPB
     */
    WORD    cbPerSec;       // Count of bytes per sector (always 512)
    BYTE    csecPerClu;     // Count of sectors per cluster (always 16)
    WORD    csecReserved;   // Count of reserved sectors.
    BYTE    cFATs;          // Count of FATs (always 1)
                            // NOTE: Even though we store 1 on the disk,
                            //       when the drive is mounted, DBLSPACE.BIN
                            //       simulates 2 FATs.
    WORD    cRootDirEntries;// Count of root directory entries (always 512)
    WORD    csecTotalWORD;  // Count of total sectors (see csecTotalDWORD if 0)
    BYTE    bMedia;         // Media byte (always 0xF8 == hard disk)
    WORD    csecFAT;        // Count of sectors occupied by the FAT
    WORD    csecPerTrack;   // Count of sectors per track (random, ignored)
    WORD    cHeads;         // Count of heads (random, ignored)
    DWORD   csecHidden;     // Count of hidden sectors
    DWORD   csecTotalDWORD; // Count of total sectors

    /*
     *  The following fields are DoubleSpace extensions
     */

    WORD    secMDFATStart;  // Logical sector of start of MDFAT
    BYTE    nLog2cbPerSec;  // Log base 2 of cbPerSec
    WORD    csecMDReserved; // Number of sectors before DOS BOOT sector
    WORD    secRootDirStart;// Logical sector of start of root directory
    WORD    secHeapStart;   // Logical sector of start of sector heap
    WORD    cluFirstData;   // Number of MDFAT entries (clusters) which are
                            // occupied by the DOS boot sector, reserved area,
                            // and root directory.
    BYTE    cpageBitFAT;    // Count of 2K pages in the BitFAT
    WORD    RESERVED1;
    BYTE    nLog2csecPerClu;// Log base 2 of csecPerClu
    WORD    RESERVED2;
    DWORD   RESERVED3;
    DWORD   RESERVED4;
    BYTE    f12BitFAT;      // 1 => 12-bit FAT, 0 => 16-bit FAT
    WORD    cmbCVFMax;      // Maximum CVF size, in megabytes (1024*1024)

} MDBPB;

typedef MDBPB FAR *LPMDBPB;


/***    MDFATENTRY - Entry in the MDFAT
 *
 *      The MDFAT is a table that is used to keep track of the data for
 *      each cluster in the DoubleSpace drive.
 *
 *      The MDFAT parallels the DOS FAT in the CVF.
 *
 *      In a normal FAT partition, the starting sector for a cluster is found
 *      by this formula:
 *
 *          sector  = FAT overhead + cluster number * sectors per cluster
 *
 *      This formula is simple, and allows the FAT to be used both as a
 *      free space map and also as a means of linking the clusters of a
 *      file together.  However, it also means that the physical space
 *      occupied by a file is a multiple of the cluster size.
 *
 *      In a CVF, on the other hand, this mapping does not apply.  The DOS
 *      FAT is still used by MS-DOS for "virtual" space allocation and linking
 *      file clusters, but no physical space allocation in the CVF occurs
 *      until disk I/O occurs on the data of a cluster.
 *
 *      To find the data for a cluster in a CVF, the cluster number is used
 *      as an index into the MDFAT, and the MDFAT Entry describes the state,
 *      location, and size of the cluster, as follows:
 *
 *      secStart
 *          This is the logical sector in the CVF Sector Heap where the
 *          data for the cluster resides.  All the sectors of this cluster
 *          are stored contiguously, starting at this location.
 *          Add 1 to this value to get the logical sector number (from the
 *          beginning of the CVF), i.e., if secStart is 134, then the sector
 *          is located at sector 135 in the CVF.
 *
 *      csecCoded
 *          This is the length of the cluster data, in sectors.
 *          Values 0..15 are interpreted as 1..16.
 *          A maximally compressed cluster will have length 0.  An
 *          uncompressed cluster will have the length MDBPB.csecPerClu-1 (15).
 *
 *      csecPlain
 *          This is the length of the uncompressed cluster data, in sectors.
 *          Values are 0..15, as with csecCoded.  This value is usually
 *          MDBPB.csecPerClu-1 (15), except for the last cluster of a file,
 *          which may be shorter, since the cluster has not been completely
 *          written.
 *
 *      fUncoded
 *          Indicates whether the cluster data is compressed.
 *          1 => uncompressed; 0 => compressed.  If DoubleSpace is unable
 *          to achieve at least a 1 sector saving, data is stored uncompressed.
 *
 *      fUsed
 *          Indicates whether this entry is in use.  This is used to allow
 *          DoubleSpace to support FAT "undelete" programs.  DBLSPACE.BIN scans
 *          a FAT sector when it is written to disk to infer three types
 *          of operations:
 *
 *          1) Cluster allocation by DOS
 *          2) Cluster free (by DOS or a utility program, like a defragger)
 *          3) Cluster "resurrection" by an undelete program.
 */

typedef struct {
    unsigned long secStart  : 22;   // Starting sector in sector heap
    unsigned int  csecCoded : 4;    // Length of coded data (in sectors)
    unsigned int  csecPlain : 4;    // Length of original data (in sectors)
    unsigned int  fUncoded  : 1;    // TRUE => coded data is NOT CODED
    unsigned int  fUsed     : 1;    // TRUE => MDFAT entry is allocate

} MDFATENTRY;

typedef MDFATENTRY FAR *LPMDFATENTRY;

// NOTE: C6 does not want to treat MDFATENTRY as a 4 byte structre, so
//       we hard-code the length here, and use explicit masks and shifts
//       in code that manipulates MDFATENTRYs.

#define cbMDFATENTRY   4

#pragma pack()


           // 'HARD CODED' DEFINITIONS FOR CVF FILES
           //        MOST STOLEN FROM 'CVF.H'!


#define cbSECTORSIZE  (512L)
#define cbCLUSTERSIZE (16L * cbSECTORSIZE)


#define szDS_STAMP1  "\xf8" "DR" // First CVF stamp
#define szDS_STAMP2  "MDR"       // Second CVF stamp
#define cbDS_STAMP   4           // Length of stamp (includes NULL)


/***    csecRESERVED1 - count of sectors in CVF region RESERVED1
 *
 *      This is the region between the BitFAT and the MDFAT.
 */
#define csecRESERVED1  1        // Hard-coded size of reserved region 1


/***    csecRESERVED2 - count of sectors in CVF region RESERVED2
 *
 *      This is the region between the MDFAT and the DOS BOOT region.
 */
#define csecRESERVED2 31        // Hard-coded size of reserved region 2


/***    csecRESERVED4 - count of sectors in CVF region RESERVED4
 *
 *      This is the region between the ROOTDIR and the SECTORHEAP.
 */
#define csecRESERVED4  2        // Hard-coded size of reserved region 4


/***    csecRETRACT_STAMP - count of sectors at end of Sector Heap
 *
 *      The tail CVF stamp is stored in the last *complete* sector
 *      of the CVF.  That is, if the CVF is exactly a multiple of 512
 *      bytes in length, then the last sector contains the stamp.
 *      If the CVF is *not* a multiple of 512 bytes, then the stamp
 *      is stored in the next to last sector in the file, which is
 *      512 bytes long (the last sector is less than 512 bytes).
 */
#define csecRETRACT_STAMP   1


/***    cbPER_BITFAT_PAGE - size of a BitFAT page in DBLSPACE.BIN
 *
 *      DBLSPACE.BIN "pages" the BitFAT into a buffer of this size,
 *      so the BitFAT total size must be a multiple of this page size.
 */
#define cbPER_BITFAT_PAGE   2048




     // THIS NEXT SECTION STOLEN FROM THE SOURCE FOR 'DSSNAP'
     // and RE-ADAPTED for my purposes (not theirs...)

/***    MDRGN - file region characterization
 *
 *      Used to record data on the BitFAT, MDFAT, boot sector, DOS FAT,
 *      DOS root directory, and the sector heap.
 *
 *      The cbTotal is the reserved amount of space in the file for the
 *      region.  This is usually a limiting factor on growing the CVF.
 *
 *      The cbActive is the length of the region (starting from the
 *      front) that is valid for the CVF at its current size.
 */
typedef struct {    /* reg */
    long    ibStart;        // Byte offset in file of region start
    long    cbTotal;        // Total length of region in bytes
    long    cbActive;       // Active area of region, in bytes
} MDRGN;

typedef MDRGN FAR *LPMDRGN; /* preg */

typedef enum { /* ireg */
    iregMDBPB,
    iregBITFAT,
    iregRESERVED1,
    iregMDFAT,
    iregRESERVED2,
    iregDOSBOOT,
    iregRESERVED3,
    iregDOSFAT,
    iregDOSROOTDIR,
    iregRESERVED4,
    iregSECTORHEAP,
    cregMAX,                        // Count of ireg's
} INDEXMDRGN;


#define cbDIR_ENT       32      // Number bytes per DOS directory entry



/***************************************************************************/
/*                                                                         */
/*    'DISK_UNIT' structure - derived from documentation and hacking       */
/*                                                                         */
/***************************************************************************/

#pragma pack(1)

typedef struct tagDISK_UNIT {

    WORD  wReserved0; // this is usually 0 for drive A, 6 for B (on my machine)
    DWORD dwSize;     // total # of 512-byte sectors on drive
    DWORD dwFree;     // total # of free 512-byte sectors on drive
    DWORD lpStrategy; // FAR real-mode ptr to device driver STRATEGY routine
    DWORD lpInterrupt;// FAR real-mode ptr to device driver INTERRUPT routine
    WORD  wSectorSize;// sector size in bytes - ALWAYS 512
    WORD  wClustSize; // cluster size - always 10H!
    WORD  wMDFat;     // MDFAT Sector Start (BPB offset 24H)
    WORD  wFour1;     // I'm not sure what it is, but it contains '4'
    WORD  wFour2;     // I'm not sure what it is, but it contains '4'
    WORD  wRsrvSect;  // # of reserved sectors; BPB offset 0EH
    WORD  wBPB;       // Logical Sector start of DOS BPB (BPB offs 27H)
    WORD  wNotSure1;  // I'm not sure what this is yet...
    WORD  wNotSure2;  // I'm not sure what this is yet...
    WORD  wFirstData; // Number of MDFAT entries (clusters) which are
                      // occupied by the DOS boot sector, reserved area,
                      // and root directory.  (BPB offset 2DH)
    struct {
       BYTE    jmpBOOT[3];     // Jump to bootstrap routine
       char    achOEMName[8];  // OEM Name ("MSDBL6.0")
       WORD    cbPerSec;       // Count of bytes per sector (always 512)
       BYTE    csecPerClu;     // Count of sectors per cluster (always 16)
       WORD    csecReserved;   // Count of reserved sectors.
       BYTE    cFATs;          // Count of FATs (always 1)
                               // NOTE: Even though we store 1 on the disk,
                               //       when the drive is mounted, DBLSPACE.BIN
                               //       simulates 2 FATs.
       WORD    cRootDirEntries;// Count of root directory entries (always 512)
       WORD    csecTotalWORD;  // Count of total sectors (see csecTotalDWORD if 0)
       BYTE    bMedia;         // Media byte (0xF8 == hard disk)
       WORD    csecFAT;        // Count of sectors occupied by the FAT
       WORD    csecPerTrack;   // Count of sectors per track (random, ignored)
       WORD    cHeads;         // Count of heads (random, ignored)
       DWORD   csecHidden;     // Count of hidden sectors
       DWORD   csecTotalDWORD; // Count of total sectors

       } DosBPB;         // the DOS BPB (well, the first 24H bytes of it)...


    WORD wNotSure3;   // I'm not sure what this is yet...
    WORD wNotSure4;   // I'm not sure what this is yet (always 0x0409)
    WORD wHost0;      // drive unit number for host drive?? ('D' was 4, not 3!)

    BYTE bMediaByte;  // media byte, from FAT - hard disk is 'F8'
    BYTE bOne;        // this is always a 1 - not sure why...

    WORD wHost;       // low byte is HOST drive; high byte 1 if NOT swapped,
                      // zero if it IS swapped...

    WORD wFlags;      // some type of flags - contains 100H if diskette,
                      // and zero for hard drive.
    WORD wCVHid;      // Low byte is CVH entry #; high byte matches that of
                      // the 'wFlags' field (above).

    DWORD dwParms1;   // appears to represent drive parm information
    DWORD dwParms2;   // appears to represent drive parm information (in DOS)


    } DISK_UNIT;

#pragma pack()




                   // INTERNAL FUNCTION PROTOTYPES


BOOL NEAR PASCAL DblspaceCommandHelp(LPSTR lpCommand);
BOOL NEAR PASCAL DblspaceCommandInfo(LPSTR lpCommand);
BOOL NEAR PASCAL DblspaceCommandList(LPSTR lpCommand);
BOOL NEAR PASCAL DblspaceCommandMount(LPSTR lpCommand);
BOOL NEAR PASCAL DblspaceCommandUnMount(LPSTR lpCommand);





// VALID COMMAND LINES ARE AS FOLLOWS:
//
//   [/INFO] drive:
//   /LIST
//   /MOUNT drive: [/NEWDRIVE=drive2:]
//   /UNMOUNT [drive:]
//
//
// VALID (BUT UNSUPPORTED) COMMAND LINES ARE AS FOLLOWS:
//
//   /CHKDSK [/F] [drive:]
//   /COMPRESS drive: [/NEWDRIVE=drive2:] [/RESERVE=size]
//   /CREATE drive: [/NEWDRIVE=drive2:] [/SIZE=size | /RESERVED=size]
//   /DEFRAGMENT [drive:]
//   /DELETE drive:
//   /FORMAT drive:
//   /MAXCOMPRESS [drive:]
//   /RATIO[=r.r] [drive: | /ALL]
//   /SIZE[=size | /RESERVE=size] [drive:]
//


#define DTB __based(__segname("DBLSPACE_TEXT"))

#define SWITCHSEGP const char DTB *
#define SWITCHSEG const char DTB


SWITCHSEG pCHKDSK[]      = "CHKDSK";
SWITCHSEG pCOMPRESS[]    = "COMPRESS";
SWITCHSEG pCREATE[]      = "CREATE";
SWITCHSEG pDEFRAGMENT[]  = "DEFRAGMENT";
SWITCHSEG pDELETE[]      = "DELETE";
SWITCHSEG pFORMAT[]      = "FORMAT";
SWITCHSEG pHELP[]        = "HELP";
SWITCHSEG pINFO[]        = "INFO";
SWITCHSEG pLIST[]        = "LIST";
SWITCHSEG pMAXCOMPRESS[] = "MAXCOMPRESS";
SWITCHSEG pMOUNT[]       = "MOUNT";
SWITCHSEG pRATIO[]       = "RATIO";
SWITCHSEG pSIZE[]        = "SIZE";
SWITCHSEG pUNMOUNT[]     = "UNMOUNT";

SWITCHSEGP DTB switches[]={
   (SWITCHSEGP)pCHKDSK,      (SWITCHSEGP)pCOMPRESS,    (SWITCHSEGP)pCREATE,
   (SWITCHSEGP)pDEFRAGMENT,  (SWITCHSEGP)pDELETE,      (SWITCHSEGP)pFORMAT,
   (SWITCHSEGP)pHELP,        (SWITCHSEGP)pINFO,        (SWITCHSEGP)pLIST,
   (SWITCHSEGP)pMAXCOMPRESS, (SWITCHSEGP)pMOUNT,       (SWITCHSEGP)pRATIO,
   (SWITCHSEGP)pSIZE,        (SWITCHSEGP)pUNMOUNT
   };




#define DBLSPACE_HELP     6
#define DBLSPACE_INFO     7     /* indices within above array */
#define DBLSPACE_LIST     8
#define DBLSPACE_MOUNT    10
#define DBLSPACE_UNMOUNT  13


#pragma code_seg("DBLSPACE_TEXT","CODE")


static __inline LPSTR FAR PASCAL lstrltrim(LPSTR lpS)
{
LPSTR lp1 = lpS;

   while(*lp1 && *lp1<=' ') lp1++;

   if(lp1>lpS) lstrcpy(lpS, lp1);

   return(lpS);
}



#pragma auto_inline(off)

BOOL FAR PASCAL DblspaceCommand(LPSTR lpCommand)
{
LPSTR lp1, lp2;
char tbuf[32];
WORD w, wSW;
BOOL rval;
HCURSOR hOldCursor;


   hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

   lpCommand = EvalString(lpCommand);
   if(!lpCommand)
   {
      PrintString(pNOTENOUGHMEMORY);

      SetCursor(hOldCursor);
      return(TRUE);
   }

   lp1 = lpCommand;

   while(*lp1 && *lp1<=' ') lp1++;  // find first non-space

   if(*lp1=='/')        // a SWITCH!!
   {

      for(lp2=lp1 + 1; *lp2 && *lp2>' '; lp2++)
        ;  // find end of word

      w=lp2 - lp1 - 1;

      if(w>0)
      {
         _fmemcpy(tbuf, lp1 + 1, w);

         tbuf[w] = 0;
      }
      else
      {
         PrintErrorMessage(534);    // ILLEGAL SWITCH
         PrintString(lp1);
         PrintString(pQCRLFLF);

         GlobalFreePtr(lpCommand);

         SetCursor(hOldCursor);
         return(TRUE);
      }


      // Next, is the switch a valid one??

      for(wSW=0; wSW<N_DIMENSIONS(switches); wSW++)
      {
         if(!_fstrnicmp(switches[wSW], tbuf, w))
         {
            break;
         }
      }

      if(wSW >= N_DIMENSIONS(switches))
      {
         PrintErrorMessage(534);    // ILLEGAL SWITCH
         PrintString(lp1);
         PrintString(pQCRLFLF);

         GlobalFreePtr(lpCommand);

         SetCursor(hOldCursor);
         return(TRUE);
      }

      lp1 = lp2;            // point 'lp1' to next available non-space

      while(*lp1 && *lp1<=' ') lp1++;
   }
   else
   {
      // at this point, assume it's '/INFO' (default)

      wSW = DBLSPACE_INFO;
   }

   switch(wSW)
   {

      case DBLSPACE_HELP:
         rval = DblspaceCommandHelp(lp1);
         break;

      case DBLSPACE_INFO:
         rval = DblspaceCommandInfo(lp1);
         break;

      case DBLSPACE_LIST:
         rval = DblspaceCommandList(lp1);
         break;

      case DBLSPACE_MOUNT:
         rval = DblspaceCommandMount(lp1);
         break;

      case DBLSPACE_UNMOUNT:
         rval = DblspaceCommandUnMount(lp1);
         break;

      default:

         PrintErrorMessage(627);
         PrintAChar('/');
         PrintString(tbuf);
         PrintErrorMessage(628);

         rval = TRUE;
         break;
   }


   GlobalFreePtr(lpCommand);

   SetCursor(hOldCursor);
   return(rval);

}



static LPSTR FAR PASCAL MegabyteString(DWORD dwBytes, WORD wLen)
{
static char buf[16];

   wsprintf(buf, "%14lu",
            (DWORD)(((double)dwBytes / 10485.76) + .5));  // 1Mb --> "100"

   buf[15] = 0;
   buf[14] = buf[13];

   if(buf[12]!=' ')   buf[13] = buf[12];
   else               buf[13] = '0';          // fixes 'lead zero' bug

   buf[12] = '.';

   if(buf[11]==' ')   buf[11] = '0';          // fixes 'lead zero' bug

   // at this point, output is:  "           0.00" for dwBytes==0
   // and lead zeros are correctly placed when input is below .1 Mb

   if(wLen>=15) return((LPSTR)buf);
   else         return((LPSTR)buf + 15 - wLen);

}




SWITCHSEG dblspace_help[]=
     "\r\n\nMicrosoft(r) DBLSPACE Compressed Volume File Driver\r\n\r\n"
     "DBLSPACE Compressed Volume File Utility (emulated by SFTSHELL)\r\n"
     " The following DBLSPACE command switches are supported by SFTSHELL:\r\n"
     "   /HELP                             - prints this HELP screen\r\n"
     "   [/INFO] [d:]                      - compressed volume file info\r\n"
     "   /LIST                             - list valid drives, and CVF status\r\n"
     "   /MOUNT[=sss] [d:] [/NEWDRIVE=e:]  - MOUNT a new CVF from drive 'd' as 'e'\r\n"
     "   /UNMOUNT [d:]                     - UNMOUNT drive 'd' (only if not in use!)\r\n"
     "\r\nFor more information, see the MS-DOS(r) User's Guide\r\n\n";


BOOL NEAR PASCAL DblspaceCommandHelp(LPSTR lpCommand)
{
   PrintString(dblspace_help);
   return(FALSE);
}



BOOL NEAR PASCAL DblspaceCommandInfo(LPSTR lpCommand)
{
WORD wDrive, wSel, w, w2, w3, w4;
DWORD dwDosBlock, dwTotalSectors, dwFreeSectors, dwTotalHost, dwFreeHost,
      dwTotalComp, dwFreeComp;
DWORD FAR *lpdw;
int seq;
BYTE fHost;
BOOL fSwapped;
LPSTR lp1;
LPCURDIR lpC;
SFTDATE d;
SFTTIME t;
REALMODECALL rmc;
char tbuf[64];


   // 'lpCommand' must consist of 'drive:' or nothing - that's it!




   if(*lpCommand)
   {
      if(lpCommand[1]!=':')
      {
         PrintErrorMessage(650);    // ?Drive "
         PrintString(lp1);
         PrintErrorMessage(643);    // " is not valid.
         PrintString(pQCRLFLF);
         return(TRUE);

      }

      wDrive = toupper(*lpCommand) - 'A';

      lp1 = lpCommand + 2;

      while(*lp1 && *lp1<=' ') lp1++;

      if(*lp1)
      {
         PrintErrorMessage(563);    // extraneous parameters ignored
         PrintString(lp1);
         PrintString(pQCRLFLF);
      }
   }
   else
   {
      wDrive = _getdrive() - 1;     // returns 'A'==1, so subtract 1
   }

   // for drive 'A', wDrive==0 - now, see if it's a DBLSPACE drive...

   if(!IsDoubleSpaceDrive((BYTE)wDrive, &fSwapped, &fHost, &seq))
   {
      lpC = GetDosCurDirEntry(wDrive + 1);   // 'wDrive' is 1-based for this

      if((lpC->wFlags & CURDIR_TYPE_MASK)==CURDIR_TYPE_INVALID)
      {
         PrintErrorMessage(650);
         PrintAChar(wDrive + 'A');
         PrintErrorMessage(651);
         return(TRUE);
      }

      // Is this drive a SUBST'ed drive, pointing to a DBLSPACE drive??

      while(lpC && lpC->wFlags & CURDIR_SUBST)  // this is a SUBST'ed drive
      {
         PrintErrorMessage(692);   // Drive "
         PrintAChar(wDrive + 'A');
         PrintErrorMessage(693);   // " is a VIRTUAL (SUBST) drive for "
         PrintString(lpC->pathname);
         PrintString(pQCRLF);

         wDrive = toupper(*(lpC->pathname)) - 'A';
         if(wDrive>26)
         {
            PrintErrorMessage(650);
            PrintAChar('A'+wDrive);
            PrintErrorMessage(651);
            return(TRUE);
         }

         MyFreeSelector(SELECTOROF(lpC));  // NEVER FORGET TO DO THIS!!

         lpC = GetDosCurDirEntry(wDrive + 1);
      }

      if(SELECTOROF(lpC)) MyFreeSelector(SELECTOROF(lpC));
                                           // NEVER FORGET TO DO THIS!!


      // The above loop 'expands' the SUBST drives until no more left!

      if(!IsDoubleSpaceDrive((BYTE)wDrive, &fSwapped, &fHost, &seq))
      {
         PrintErrorMessage(689);
         PrintAChar('A'+wDrive);
         PrintErrorMessage(690);
         return(TRUE);
      }
   }

   *tbuf = (char)('A' + wDrive);
   tbuf[1] = ':';
   tbuf[2] = 0;

   if(GetDriveLabel(tbuf, tbuf))
   {
      PrintErrorMessage(694);
      return(TRUE);
   }

   PrintErrorMessage(704);       // Compressed Drive
   PrintAChar('A' + wDrive);
   PrintAChar(' ');
   PrintString(tbuf);          // the volume label!!

   if(fSwapped)
   {
      PrintErrorMessage(705);    // (SWAPPED)
   }

   PrintErrorMessage(706);       // Created On

   wsprintf(work_buf, "%c:\\DBLSPACE.%03d", 'A' + fHost, seq);

   lstrcpy(tbuf, work_buf);      // save for later, in case it's wiped out

   _dos_findfirst(work_buf, _A_ARCH | _A_HIDDEN | _A_RDONLY | _A_SYSTEM,
                  &ff);

   d.year   =((ff.wr_date & 0xfe00) >> 9) + 1980;
   d.month  = (ff.wr_date & 0x01e0) >> 5;
   d.day    = (ff.wr_date & 0x001f);

   t.hour   = (ff.wr_time & 0xf800) >> 11;
   t.minute = (ff.wr_time & 0x07e0) >> 5;
   t.second = (ff.wr_time & 0x001f) << 1;


   lp1 = DateStr(&d);
   PrintString(lp1);
   GlobalFreePtr(lp1);

   PrintErrorMessage(707);          // " at "

   lp1 = TimeStr(&t);
   PrintString(lp1);
   GlobalFreePtr(lp1);

   PrintErrorMessage(708);          // "\r\nDrive "
   PrintAChar('A' + wDrive);
   PrintErrorMessage(709);          // " is stored on uncompressed drive "
   PrintAChar(*tbuf);
   PrintErrorMessage(710);          // " in the file "
   PrintString(tbuf + 3);           // the file name, without "D:\\"

   PrintErrorMessage(711);          // ".\r\n\n"

   // at this point ff.size is the total size of the compressed volume file
   // in case I decide to add some info about this file to the report...

   // Next, I need to get some information from DBLSPACE itself.  Because
   // of some potential protected mode problems, do this as a REAL MODE call!


   if(IsChicago)  // for Chicago, use PM version (?)
   {
      _asm
      {
         push ds
         mov ax, 0x4a11; // the function for the REAL MODE INT 2fH
         mov bx, 7
         mov dl, BYTE PTR wDrive  // DL = drive
         mov dh, 0
         int 0x2f

         mov WORD PTR lpdw, si
         mov WORD PTR lpdw + 2, ds
         pop ds

         mov w, ax  // this is now a flag
      }

      dwTotalSectors = 0xffffffff;
      dwFreeSectors = 0xffffffff;

      if(w == 0)  // i.e. not an error
      {
         dwTotalSectors = lpdw[0];
         dwFreeSectors = lpdw[1];
      }
   }
   else
   {
      dwDosBlock = GlobalDosAlloc(512);
      if(!dwDosBlock)
      {
         PrintString(pNOTENOUGHMEMORY);
         return(TRUE);  /* error - no memory */
      }

      _fmemset((LPSTR)&rmc, 0, sizeof(rmc));


      rmc.SS = HIWORD(dwDosBlock);
      rmc.SP = 510;    /* top of block - 2 */

      rmc.DS = rmc.SS;
      rmc.ES = rmc.SS;

      rmc.EAX = 0x4a11; // the function for the REAL MODE INT 2fH
      rmc.EBX = 7;
      rmc.EDX = ((DWORD)wDrive) & 0xffL;  // DL = drive

      if(RealModeInt(0x2f, &rmc))
      {
         PrintErrorMessage(696);

         GlobalDosFree(LOWORD(dwDosBlock));
         return(TRUE);
      }

      GlobalDosFree(LOWORD(dwDosBlock));

      // at this point, DS:SI points to 2 DWORD's (in REAL MODE DOS-land)
      // so I must de-reference this address as a selector:offset and get
      // the ACTUAL value. No problem, eh?

      dwTotalSectors = 0xffffffff;
      dwFreeSectors = 0xffffffff;

      if(!rmc.EAX)  // only if the above call was SUCCESSFUL!
      {
         wSel = MyAllocSelector();       // gets a selector!

         if(!wSel)
         {
            PrintString(pNOTENOUGHMEMORY);
            return(TRUE);
         }

         dwDosBlock = (rmc.ESI & 0xffff) + (((DWORD)(rmc.DS))<<4);

         MyChangeSelector(wSel, dwDosBlock, sizeof(DWORD) * 2);

         lpdw = (DWORD FAR *)MAKELP(wSel, 0);

         dwTotalSectors = lpdw[0];
         dwFreeSectors = lpdw[1];

         MyFreeSelector(wSel);
      }
   }


   // Now, let's see what MS-DOS says is my total/free sectors...

   _asm
   {
      mov ah, 0x36
      mov dl, BYTE PTR wDrive
      inc dl

      int 0x21         // get drive info

      mov w, ax        // sectors per cluster
      mov w2, bx       // # of available clusters
      mov w3, cx       // bytes per sector
      mov w4, dx       // # clusters per drive
   }


   dwTotalComp = (DWORD)w * (DWORD)w4 * (DWORD)w3;
   dwFreeComp  = (DWORD)w * (DWORD)w2 * (DWORD)w3;



   // Next step, gotta find out similar info for HOST drive...

   _asm
   {
      mov ah, 0x36
      mov dl, BYTE PTR fHost
      inc dl

      int 0x21         // get drive info

      mov w, ax        // sectors per cluster
      mov w2, bx       // # of available clusters
      mov w3, cx       // bytes per sector
      mov w4, dx       // # clusters per drive
   }


   dwTotalHost = (DWORD)w * (DWORD)w4 * (DWORD)w3;
   dwFreeHost  = (DWORD)w * (DWORD)w2 * (DWORD)w3;



   // OK! Got all of the info, let's print the report!!

   PrintErrorMessage(712);    // "                    Compressed      Uncompressed\r\n"
                              // "                      Drive "
   PrintAChar('A' + wDrive);

   PrintErrorMessage(713);           // "          Drive "
   PrintAChar(*tbuf);
   PrintString(pCRLFLF);

   PrintErrorMessage(714);           // "   Total space:    "
   PrintString(MegabyteString(dwTotalComp, 8));
   PrintErrorMessage(715);           // " MB      "
   PrintString(MegabyteString(dwTotalHost, 8));
   PrintErrorMessage(716);           // " MB\r\n"

   PrintErrorMessage(717);           // "   Space Used:     "
   PrintString(MegabyteString(dwTotalComp - dwFreeComp, 8));
   PrintErrorMessage(715);           // " MB      "
   PrintString(MegabyteString(dwTotalHost - dwFreeHost, 8));
   PrintErrorMessage(716);           // " MB\r\n"

   PrintErrorMessage(718);           // "   Space Free:     "
   PrintString(MegabyteString(dwFreeComp, 8));
   PrintErrorMessage(719);           // " MB**    "
   PrintString(MegabyteString(dwFreeHost, 8));
   PrintErrorMessage(716);           // " MB\r\n"


   // calculate compression ratios based on actual vs 'in use' bytes for
   // compressed drive, and actual free vs 'estimated free' bytes.

   PrintErrorMessage(702);           // "\r\n   The actual compression ratio is "

   wsprintf(work_buf, "%8ld",
            (DWORD)(10.0 * (dwTotalComp - dwFreeComp) /
                    (512.0 * (dwTotalSectors - dwFreeSectors)) + .5) );

   work_buf[9] = 0;
   work_buf[8] = work_buf[7];
   work_buf[7] = '.';

   lstrltrim(work_buf);

   PrintString(work_buf);
   PrintErrorMessage(703);           // " to 1.\r\n\r\n** based on estimated compression ratio of "


   wsprintf(work_buf, "%8ld",
            (DWORD)(10.0 * dwFreeComp / (512.0 * dwFreeSectors) + .5) );

   work_buf[9] = 0;
   work_buf[8] = work_buf[7];
   work_buf[7] = '.';

   lstrltrim(work_buf);

   PrintString(work_buf);

   PrintErrorMessage(720);           // " to 1.\r\n\n"

   return(FALSE);

}


BOOL NEAR PASCAL DblspaceCommandList(LPSTR lpCommand)
{
LPCURDIR lpC;
LPCURDIR3 lpC3;
LPSTR lpLOL; //, lpData;
DWORD dwVer;
WORD w, w2, wMySel, wNDrives, wFirst, wLast;
WORD wClustSize, wFreeClust, wSectSize, wDriveSize;
int seq;
BYTE fHost;
BOOL fSwapped;



   dwVer = GetVersion();


           /** Use INT 21H function 52H to get List of Lists **/

   _asm
   {
      mov ax,0x5200
      int 0x21
      mov WORD PTR lpLOL+2 ,es
      mov WORD PTR lpLOL, bx
   }

   wMySel = MyAllocSelector();

   if(!wMySel)
   {
      PrintString(pNOTENOUGHMEMORY);
      return(TRUE);
   }



    /* OBTAIN ADDRESS FOR CURDIR & # OF VALID DRIVES FROM List of Lists */


   if(HIWORD(dwVer)>=0x310)  /* DOS 3.1 or later */
   {
      wNDrives = *((BYTE FAR *)(lpLOL + 0x21));
   }
   else
   {
      wNDrives = *((BYTE FAR *)(lpLOL + 0x1b));
   }

   if(HIWORD(dwVer)<0x400)          // DOS VERSIONS BEFORE 4.0
   {
      if(HIWORD(dwVer)==0x300)      // RELEASE 3.0 only
      {
         lpC3 = (LPCURDIR3) *((DWORD FAR *)(lpLOL + 0x17));
      }
      else
      {
	 lpC3 = (LPCURDIR3) *((DWORD FAR *)(lpLOL + 0x16));
      }

      MyChangeSelector(wMySel, HIWORD(lpC3) * 16L + LOWORD(lpC3), 0xffff);

      lpC3 = (LPCURDIR3)MAKELP(wMySel, 0);
      lpC = NULL;
   }
   else
   {
      lpC = (LPCURDIR) *((DWORD FAR *)(lpLOL + 0x16));


      MyChangeSelector(wMySel, HIWORD(lpC) * 16L + LOWORD(lpC), 0xffff);

      lpC  = (LPCURDIR)MAKELP(wMySel, 0);
      lpC3 = NULL;
   }


   // determine the MAXIMUM DRIVE NUMBER that DBLSPACE can use...


   _asm
   {
      mov ax, 0x4a11
      mov bx, 0
      int 0x2f

      mov dl, cl
      mov dh, 0

      mov cl, ch
      mov ch, 0

      mov wFirst, dx         // first drive used by DBLSPACE
      mov wLast, cx          // # of drives used by DBLSPACE
   }



   // PERFORM A 'DISK RESET' TO KICK SYSTEM IN THE TROUSERS

   _asm mov ax, 0x0d00
   _asm int 0x21              // performs a DISK RESET!



   // DISPLAY HEADER!

   PrintString(pCRLF);

   PrintErrorMessage(721);    // "Drive  Type                        Total Free  Total Size  CVF Filename   \r\n"
   PrintErrorMessage(722);    // "-----  --------------------------  ----------  ----------  ---------------\r\n"

            // COL WIDTH:SPACING - 5:2, 26:2, 10:2, 10:2, 15




   // (for now, 'wFirst' and 'wLast' not used until I'm sure how to use them)

   for(w=0; w<wNDrives; w++)
   {
      if(lpC3)      // DOS version 3.0 only
      {
         w2 = lpC3[w].wFlags;
      }
      else if(lpC)
      {
         w2 = lpC[w].wFlags;
      }
      else
      {
         break;     // unexpected condition - just bail out (for now)
      }


      if((w2 & CURDIR_TYPE_MASK)==CURDIR_TYPE_INVALID)
      {
         continue;         // ignore this drive - it's invalid!
      }

      PrintErrorMessage(723);          // "  "
      PrintAChar('A' + w);
      PrintErrorMessage(724);          // "    "
                                       // now at start of 'Type' column

      if(IsDoubleSpaceDrive((BYTE)w, &fSwapped, &fHost, &seq))
      {
         if(GetDriveType(w)==DRIVE_REMOVABLE)
         {
            PrintErrorMessage(725);          // "Compressed Removeable     "
         }
         else if(GetDriveType(w)==DRIVE_FIXED)
         {
            PrintErrorMessage(726);          // "Compressed Fixed Drive    "
         }
         else if(GetDriveType(w)==DRIVE_REMOTE)
         {
            PrintErrorMessage(727);          // "Compressed Remote Drive   "
         }
         else
         {
            PrintErrorMessage(728);          // "Compressed Volume (?)     "
         }
      }
      else if((w2 & CURDIR_SUBST) || (w2 & CURDIR_JOIN))
      {
         if(w2 & CURDIR_SUBST)
         {
            PrintErrorMessage(729);          // "Virtual ('SUBST'ed) Drive\r\n"
         }
         else
         {
            PrintErrorMessage(730);          // "Virtual ('JOIN'ed) Drive\r\n"
         }

         continue;
      }
      else if((w2 & CURDIR_TYPE_MASK)==CURDIR_TYPE_NETWORK)
      {
         seq = -1;

         PrintErrorMessage(731);          // "Network Drive             "
      }
      else if((w2 & CURDIR_TYPE_MASK)==CURDIR_TYPE_IFS)
      {
         seq = -1;

         PrintErrorMessage(732);          // "Installable File System   "
      }
      else         // PHYSICAL DRIVE - the only possibility left!
      {            // Need to determine if this thing is removable...

         seq = -1;

         if(GetDriveType(w)==DRIVE_REMOVABLE)
         {
            PrintErrorMessage(733);          // "Removable-media drive     "
         }
         else if(GetDriveType(w)==DRIVE_FIXED)
         {
            PrintErrorMessage(734);          // "Local Fixed Drive         "
         }
         else
         {
            PrintErrorMessage(735);          // "?Unknown Drive Type       "
         }

//         if(lpC3)
//         {
//            lpDPB3 = (LPDPB3)lpC3[w].dwDPB;
//            lpDPB = NULL;
//         }
//         else
//         {
//            lpDPB = (LPDPB)lpC[w].dwDPB;
//            lpDPB3 = NULL;
//         }


      }

      // OH BOY!  Now that I have *THIS* information, get the sizes...

      _asm
      {
         mov ah, 0x36
         mov dl, BYTE PTR w
         inc dl
         int 0x21

         mov wClustSize, ax
         mov wFreeClust, bx
         mov wSectSize, cx
         mov wDriveSize, dx
      }

      if(wClustSize==0xffff)
      {
         PrintErrorMessage(736);          // "No Disk in Drive\r\n"
         continue;
      }


      // fun part #1 - write the free space

      PrintErrorMessage(723);          // "  "

      PrintString(MegabyteString((DWORD)wClustSize * (DWORD)wFreeClust
                                 * (DWORD)wSectSize, 7));
      PrintErrorMessage(737);          // " MB  "


      // fun part #2 - write the total size

      PrintString(MegabyteString((DWORD)wClustSize * (DWORD)wDriveSize
                                 * (DWORD)wSectSize, 7));
      PrintErrorMessage(737);          // " MB  "


      // if this is a DBLSPACE drive, I need the HOST DRIVE and path...

      if(seq>=0)
      {
         wsprintf(work_buf, "%c:\\DBLSPACE.%03d", fHost + 'A', seq);

         PrintString(work_buf);
      }

      PrintString(pCRLF);
   }



}



BOOL NEAR PASCAL DblspaceCommandMount(LPSTR lpCommand)
{

   PrintErrorMessage(695);
   return(TRUE);
}


BOOL NEAR PASCAL DblspaceCommandUnMount(LPSTR lpCommand)
{
WORD wDrive, wDrive0, w;
DWORD dwVer;
int seq;
BYTE fHost;
BOOL fSwapped;
LPSTR lp1;
LPCURDIR lpC;



   dwVer = GetVersion();           // for later...



   for(lp1=lpCommand; *lp1 && *lp1<=' '; lp1++)
      ;  // find first non-white-space or end of string

   if(!*lp1)
   {
      wDrive = _getdrive();
   }
   else
   {
      wDrive = toupper(*lp1) - 'A' + 1;
   }


   wDrive0 = wDrive;      // keep for posterity!


   // CHECK TO SEE THAT THIS DBLSPACE DRIVE ACTUALLY EXISTS - NO 'SUBST'
   // DRIVES MAY BE SPECIFIED!!  PERIOD!!


   if(!IsDoubleSpaceDrive((BYTE)(wDrive - 1), &fSwapped, &fHost, &seq))
   {
      PrintErrorMessage(689);
      PrintAChar('A' + wDrive - 1);
      PrintErrorMessage(690);
      return(TRUE);
   }

   // Next, check 'wDrive' to see if it's IN USE by anything important

   if(IsDriveInUse(wDrive))
   {
      PrintErrorMessage(689);
      PrintAChar(wDrive + 'A' - 1);
      PrintErrorMessage(691);
      return(TRUE);
   }

   // Again, check 'fHost' to see if it's IN USE by anything important
   // if the drive is SWAPPED.  If not swapped, who cares...!

   if(fSwapped)
   {
      if(IsDriveInUse(fHost + 1))
      {
         PrintErrorMessage(689);
         PrintAChar(fHost + 'A');
         PrintErrorMessage(691);
         return(TRUE);
      }
   }


   // It is *now* safe to UNMOUNT this drive!!  Do the dirty deed...

   wDrive--;       // convert to '0-based' for next sections


   // if drive is SWAPPED, unswap the thing (it's required!!!)

   if(fSwapped)
   {
      _asm
      {
         mov ax, 0x4a11
         mov bx, 2
         mov dl, BYTE PTR wDrive
         int 0x2f

         mov w, ax                  // error return code!  0 if ok...
      }

//      if(w && w!=I2F_ERR_ALREADY_SWAPPED) // ignore errors.... (BUG?)
//      {
//         PrintErrorMessage(697);
//
//         return(TRUE);
//      }

      wDrive = fHost;               // convert this one to host drive now!
   }

   _asm
   {
      mov ax, 0x4a11
      mov bx, 6
      mov dl, BYTE PTR wDrive
      int 0x2f              // unmount drive 'wDrive'

      mov ah, 0
      mov w, ax             // w==0 if OK, or !=0 on error
   }

   if(w)
   {
      PrintErrorMessage(698);

      if(fSwapped)          // drive is swapped - finish error message with
      {                     // the appropriate warning
         PrintErrorMessage(699);
      }
      else
      {
         PrintString(pCRLFLF);
      }

      return(TRUE);
   }


   // NEXT STEP - RESET DISK SYSTEM

   _asm mov ax, 0x0d00
   _asm int 0x21              // performs a DISK RESET!






   // OK!  Success in the UNMOUNT process - now I must mark the 'CURDIR'
   //      entry (drive table) as FREE so that I can use it again.


   lpC = GetDosCurDirEntry(wDrive + 1);  // 'GetDosCurDirEntry()' is 1-based

   lpC->wFlags = 0;  /* it's no longer valid now */

   lpC->pathname[0] = (char)(wDrive + 'A');  /* the drive letter desig. */
   lpC->pathname[1] = ':';
   lpC->pathname[2] = '\\';
   lpC->pathname[3] = 0;

   lpC->wPathRootOffs = 2;  /* normal for root dir! */

   lpC->dwDPB          = 0;
   lpC->wDirStartClust = 0xffff;
   lpC->wNetPtrHi      = 0xffff;
   lpC->wInt21_5F03    = 0;

   if(HIWORD(dwVer)>=0x400)
   {
      lpC->wDOS4Byte   = 0;
      lpC->dwIFSPtr    = 0;
      lpC->wDOS4Word2  = 0;
   }

   MyFreeSelector(SELECTOROF(lpC));



   // LAST STEP - RESET DISK SYSTEM

   _asm mov ax, 0x0d00
   _asm int 0x21              // performs a DISK RESET!




   //
   // LATER I NEED TO ADD CODE TO INFORM 'FILE MANAGER' OF CHANGES
   //




   PrintErrorMessage(692);    // Drive "
   PrintAChar(wDrive0 - 1 + 'A');
   PrintErrorMessage(700);    // " has been successfully 'UNMOUNT'ed!!


   return(FALSE);             // the operation has succeeded!!

}





   /** DOWNLOADED FROM 'DOSBETA' FORUM ON CIS - 1/21/93                **/

/***    IsDoubleSpaceDrive - Get information on a DoubleSpace drive
 *
 *      Entry:
 *          drive     - Drive to test (0=A, 1=B, etc.)
 *                      NOTE: No parameter checking is done on drive.
 *          pfSwapped - Receives TRUE/FALSE indicating if drive is swapped.
 *          pdrHost   - Receives drive number of host drive
 *          pseq      - Receives CVFs sequence number if DoubleSpace drive
 *
 *      Exit:
 *          returns TRUE, if DoubleSpace drive:
 *              *pfSwapped = TRUE, if drive is swapped with host,
 *                           FALSE, if drive is not swapped with host
 *              *pdrHost   = current drive number of host drive (0=A,...)
 *              *pseq      = CVF sequence number (always zero if swapped
 *                             with host drive)
 *
 *                           NOTE: The full file name of the CVF is:
 *                                   *pdrHost:\DBLSPACE.*pseq
 *
 *                               pdrHost  pseq  Full Path
 *                               -------  ----  -----------
 *                                  0       1   a:\dblspace.001
 *                                  3       0   d:\dblspace.000
 *
 *          returns FALSE, if *not* DoubleSpace drive:
 *              *pdrHost   = drive number of host drive at boot time
 *              *pfSwapped = TRUE, if swapped with a DoubleSpace drive
 *                           FALSE, if not swapped with a DoubleSpace drive
 */

BOOL FAR PASCAL IsDoubleSpaceDrive(BYTE drive, BOOL FAR *pfSwapped,
                                   BYTE FAR *pdrHost, int FAR *pseq)
{
BYTE        seq;
BYTE        drHost;
BOOL        fSwapped;
BOOL        fDoubleSpace;


    // Assume drive is a normal, non-host drive
    drHost = drive;
    fSwapped = FALSE;
    fDoubleSpace = FALSE;
    seq = 0;

    _asm
    {
        mov     ax,4A11h        ; DBLSPACE.BIN INT 2F number
        mov     bx,1            ; bx = GetDriveMap function
        mov     dl,drive        ;
        int     2Fh             ; (bl AND 80h) == DS drive flag
                                ; (bl AND 7Fh) == host drive

        or      ax,ax           ; Success?
        jnz     gdiExit         ;    NO, DoubleSpace not installed

        test    bl,80h          ; Is the drive compressed?
        jz      gdiHost         ;    NO, could be host drive

        ; We have a DoubleSpace Drive, need to figure out host drive.
        ;
        ; This is tricky because of the manner in which DBLSPACE.BIN
        ; keeps track of drives.
        ;
        ; For a swapped CVF, the current drive number of the host
        ; drive is returned by the first GetDriveMap call.  But for
        ; an unswapped CVF, we must make a second GetDriveMap call
        ; on the "host" drive returned by the first call.  But, to
        ; distinguish between swapped and unswapped CVFs, we must
        ; make both of these calls.  So, we make them, and then check
        ; the results.

        mov     fDoubleSpace,TRUE ; Drive is DS drive
        mov     seq,bh          ; Save sequence number

        and     bl,7Fh          ; bl = "host" drive number
        mov     drHost,bl       ; Save 1st host drive
        mov     dl,bl           ; Set up for query of "host" drive

        mov     ax,4A11h        ; DBLSPACE.BIN INT 2F number
        mov     bx,1            ; bx = GetDriveMap function
        int     2Fh             ; (bl AND 7Fh) == 2nd host drive

        and     bl,7Fh          ; bl = 2nd host drive
        cmp     bl,drive        ; Is host of host of drive itself?
        mov     fSwapped,TRUE   ; Assume CVF is swapped
        je      gdiExit         ;   YES, CVF is swapped

        mov     fSwapped,FALSE  ;   NO, CVF is not swapped
        mov     drHost,bl       ; True host is 2nd host drive
        jmp     short gdiExit

    gdiHost:
        and     bl,7Fh          ; bl = host drive number
        cmp     bl,dl           ; Is drive swapped?
        je      gdiExit         ;    NO

        mov     fSwapped,TRUE   ;    YES
        mov     drHost,bl       ; Set boot drive number

    gdiExit:
    }

    if(pdrHost)   *pdrHost   = drHost;
    if(pfSwapped) *pfSwapped = fSwapped;
    if(pseq)      *pseq      = seq;

    return(fDoubleSpace);
}









/** IsDriveInUse(wDrive) - function called by several places, but located **/
/** here because it was convenient at the time (during development).      **/
/** Drive 'A'==1, 'B'==2, etc. (0 implies current (active) drive).        **/

/* This function checks for:  1) Current directory assigned to drive       */
/*                            2) Open files on this drive                  */
/*                            3) Drive is target for SUBST/JOIN            */


BOOL FAR PASCAL IsDriveInUse(WORD wDrive)
{
TASKENTRY te;
HTASK hCurrentTask;
LPCURDIR lpC;
LPSTR lpLOL, lpData;
LPSFT lpSFT;
LPTDB lpTDB;
WORD verflag, i, wMySel, wNItems, wNDrives, w;
DWORD dwVer, dwAddr;





		    /***------------------------***/
		    /*** INITIALIZATION SECTION ***/
		    /***------------------------***/

   if(!wDrive) wDrive = _getdrive();    // assigns correct drive # to wDrive


   _hmemset((LPSTR)&te, 0, sizeof(te));
   te.dwSize = sizeof(te);


   hCurrentTask = GetCurrentTask();


          /** Obtain current DOS version using 'GetVersion()' **/

   dwVer = GetVersion();
   if(HIWORD(dwVer)<0x300)
   {
      verflag = DOSVER_2X;
   }
   else if(HIWORD(dwVer)>=0x300 && HIWORD(dwVer)<0x310)
   {
      verflag = DOSVER_30;
   }
   else if(HIWORD(dwVer)>=0x310 && HIWORD(dwVer)<0x400)
   {
      verflag = DOSVER_3X;
   }
   else
   {
      verflag = DOSVER_4PLUS;
   }

           /** Use INT 21H function 52H to get List of Lists **/

   _asm
   {
      mov ax,0x5200
      int 0x21
      mov WORD PTR lpLOL+2 ,es
      mov WORD PTR lpLOL, bx
   }


    /* OBTAIN ADDRESS FOR SFT, TOTAL # OF DRIVES, ETC. FROM List of Lists */


   lpSFT = *((LPSFT FAR *)(lpLOL + 4));  /** Address of System File Table **/

   if(HIWORD(dwVer)>=0x310)  /* DOS 3.1 or later */
   {
      wNDrives = *((BYTE FAR *)(lpLOL + 0x21));
   }
   else
   {
      wNDrives = *((BYTE FAR *)(lpLOL + 0x1b));
   }





   // STEP 1:  Use System File Table to determine if drive is 'in use'



		/** Allocate an LDT selector via DPMI **/


   wMySel = MyAllocSelector();

   if(!wMySel)
   {
      PrintErrorMessage(618);

      return(TRUE);
   }


                           /***-----------***/
			   /*** MAIN LOOP ***/
			   /***-----------***/


   wNItems = 0;

   do
   {
	    /* gotta translate that puppy to *protected* mode */

      dwAddr = SELECTOROF(lpSFT)*16L + OFFSETOF(lpSFT);

      if(MyChangeSelector(wMySel, dwAddr, 0xffff))
      {
         PrintErrorMessage(620);         // this says 'DPMI FAILURE'
         return(TRUE);                   // on error assume 'drive in use'
      }
      else
      {
         lpSFT = (LPSFT)MAKELP(wMySel, 0);
      }


      lpData = lpSFT->lpData;       /* pointer to array of 'SFT' entries */

	      /** read through 'SFT' array for the total # **/
	      /** of 'SFT' entries in this block.          **/

      for(i=0; i<lpSFT->wCount; i++)
      {
	 switch(verflag)    /* different structure sizes for */
	 {                  /* different DOS versions.       */

	    case DOSVER_2X:
               if(((struct _sft2_ FAR *)lpData)->n_handles && // entry in use
                  (WORD)((struct _sft2_ FAR *)lpData)->drive == wDrive)
               {
                  MyFreeSelector(wMySel);
                  return(TRUE);            // DRIVE IS IN USE!!!!
               }
	       lpData += sizeof(struct _sft2_);
	       break;

	    case DOSVER_30:
               if(((struct _sft30_ FAR *)lpData)->n_handles && // entry in use
                  !(((struct _sft30_ FAR *)lpData)->dev_info_word & 0x80))
               {                                    /* bit set if device */
                  if((((struct _sft30_ FAR *)lpData)->dev_info_word & 31)
                     == (wDrive - 1))  // check drive # equal to 'test' drive
                  {
                     MyFreeSelector(wMySel);
                     return(TRUE);            // DRIVE IS IN USE!!!!
                  }
               }

               lpData += sizeof(struct _sft30_);
	       break;

	    case DOSVER_3X:
               if(((struct _sft31_ FAR *)lpData)->n_handles && // entry in use
                  !(((struct _sft31_ FAR *)lpData)->dev_info_word & 0x80))
               {                                    /* bit set if device */
                  if((((struct _sft31_ FAR *)lpData)->dev_info_word & 31)
                     == (wDrive - 1))  // check drive # equal to 'test' drive
                  {
                     MyFreeSelector(wMySel);
                     return(TRUE);            // DRIVE IS IN USE!!!!
                  }
               }

               lpData += sizeof(struct _sft31_);
	       break;

	    case DOSVER_4PLUS:
               if(((struct _sft4_ FAR *)lpData)->n_handles && // entry in use
                  !(((struct _sft4_ FAR *)lpData)->dev_info_word & 0x80))
               {                                    /* bit set if device */
                  if((((struct _sft4_ FAR *)lpData)->dev_info_word & 31)
                     == (wDrive - 1))  // check drive # equal to 'test' drive
                  {
                     MyFreeSelector(wMySel);
                     return(TRUE);            // DRIVE IS IN USE!!!!
                  }
               }

               lpData += sizeof(struct _sft4_);
	       break;
	 }
      }
   
      lpSFT = lpSFT->lpNext;       /* get (DOS) pointer to next SFT block */

   } while(SELECTOROF(lpSFT) && OFFSETOF(lpSFT)!=0xffff);

   MyFreeSelector(wMySel);       // free selector - no longer needed!!



   // STEP 1a: In addition to the System File Table, check the Current Dir
   //          (drive list) table to see if any drives are 'SUBST'ed or
   //          'JOIN'ed to this drive!  If there are any, it's IN USE!


   for(w=1; w<=wNDrives; w++)
   {
      if(w==wDrive) continue;          // ignore if 'same drive'

      lpC = GetDosCurDirEntry(w);

      if(!lpC) continue;  /* ignore NULL returns */

      if((lpC->wFlags & CURDIR_TYPE_MASK)==CURDIR_TYPE_INVALID) continue;

      if((lpC->wFlags & CURDIR_SUBST) || (lpC->wFlags & CURDIR_JOIN))
      {
         /* this is a 'SUBST'ed or 'JOIN'ed drive! */

         if((WORD)(toupper(*(lpC->pathname)) - 'A') == (wDrive - 1))
         {
            MyFreeSelector(SELECTOROF(lpC));  /* don't forget to do this! */
            return(TRUE);                 // guess what - it's IN USE, guys!!
         }
      }

      MyFreeSelector(SELECTOROF(lpC));  /* don't forget to do this! */
   }




   // STEP 2:  Now that the SFT doesn't have any open files on this drive,
   //          check the various 'current directories' to see if they are
   //          assigned to this drive... (assume *NOT* this task!)


      /* here I check if drive is active using the task chain */

   if(!lpTaskFirst(&te))
   {
      PrintErrorMessage(667);    // unable to obtain task list

      return(TRUE);
   }

   do
   {
      if(te.hTask && te.hTask!=hCurrentTask)
      {
         lpTDB = GlobalLock(te.hTask);

         if((WORD)(lpTDB->bDrive & 0x7f)==(WORD)(wDrive - 1))
         {
            GlobalUnlock(te.hTask); // every lock needs an unlock!

            return(TRUE);
         }

         GlobalUnlock(te.hTask); // every lock needs an unlock!
      }

   } while(lpTaskNext(&te) && te.hTask);



   return(FALSE);                  // drive is *NOT* in use!! YES!!


}







BOOL FAR PASCAL DblspaceComputeRegions(HFILE hFile, LPMDRGN lpAreg,
                                       LPMDBPB lpMDBPB)
{
int     ireg;                       // Index to walk region table
int     cbPerSec;                   // Count of bytes per sector
char    csecPerClu;                 // Count of sectors per cluster
long    ccluTotal;                  // Current total clusters
long    ccluTotalMax;               // Maximum total clusters
long    csecTotal;                  // Current total sectors
long    csecTotalMax;               // Maximum total sectors
//long    seekpos;                    // File seek position
char sig_buf[cbDS_STAMP];
long    ibCVFStamp2, cbCVF;


   // READ 'MDBPB' structure into 'mdbpb'

   _llseek(hFile, 0, SEEK_SET);  // rewind to beginning of file
   if(_lread(hFile, (LPSTR)lpMDBPB, sizeof(*lpMDBPB))!=sizeof(*lpMDBPB))
   {
      return(TRUE);
   }


   // Get common values, to make code more readable

   cbPerSec   = lpMDBPB->cbPerSec;         // Count of bytes per sector
   csecPerClu = lpMDBPB->csecPerClu;       // Count of sectors per cluster


   // Get drive size reported to DOS when CVF is mounted

   if (lpMDBPB->csecTotalWORD != 0)        // Small drive
   {
      csecTotal = lpMDBPB->csecTotalWORD;
   }
   else                                // Large drive
   {
       csecTotal = lpMDBPB->csecTotalDWORD;
   }

   ccluTotal = csecTotal/csecPerClu;   // Total number of clusters



   // Check CVF signatures

   _llseek(hFile, (lpMDBPB->csecMDReserved + 1) * (long)cbPerSec, SEEK_SET);

   if(_lread(hFile, &sig_buf, sizeof(sig_buf))!=sizeof(sig_buf))
   {
      return(TRUE);
   }
   if(_fmemcmp(sig_buf, szDS_STAMP1, cbDS_STAMP) &&
      _fmemcmp(sig_buf, szDS_STAMP2, cbDS_STAMP)) return(TRUE);


   // The 2nd stamp is located at the start of the last complete sector
   // in the CVF.  If the CVF is exactly a sector multiple, then this
   // is indeed the last sector of the file.  However, sometimes CVFs are
   // not exactly a sector multiple in length, in which case it is the
   // next to last sector of the CVF which contains the 2nd stamp.

   cbCVF = _llseek(hFile, 0L, SEEK_END);       // TOTAL # OF BYTES IN FILE

// COMMENTED OUT 2ND 'STAMP' VERIFICATION DUE TO EXCESS TIME REQUIRED TO
// PERFORM THIS CHECK ON LARGE CVF'S AND SLOW CPU'S

//   ibCVFStamp2 = (cbCVF / cbPerSec - csecRETRACT_STAMP) * cbPerSec;
//   _llseek(hFile, ibCVFStamp2, SEEK_SET);
//
//   if(_lread(hFile, &sig_buf, sizeof(sig_buf))!=sizeof(sig_buf))
//   {
//      return(TRUE);
//   }
//   if(_fmemcmp(sig_buf, szDS_STAMP1, cbDS_STAMP) &&
//      _fmemcmp(sig_buf, szDS_STAMP2, cbDS_STAMP)) return(TRUE);


   // Get Maximum CVF size information

   csecTotalMax = (lpMDBPB->cmbCVFMax * 1024L * 1024L) / cbPerSec;
   ccluTotalMax = csecTotalMax / csecPerClu;

   // Compute MDBPB region

   ireg = iregMDBPB;
   lpAreg[ireg].ibStart  = 0L;           // Always first thing in the CVF
   lpAreg[ireg].cbTotal  = cbPerSec;     // Always consumes one sector
   lpAreg[ireg].cbActive = sizeof(MDBPB); // Only MDBPB structure is valid

   // Compute BitFAT region

   ireg++;                             // iregBITFAT
   lpAreg[ireg].ibStart  = lpAreg[ireg-1].ibStart + lpAreg[ireg-1].cbTotal;

   // The BitFAT cbTotal should also ==
   //   (cmbCVFMax * 1024 * 1024) / (8 * cbPerSec)
   //   (file capicity in sectors / 8 sector bits per byte)

   lpAreg[ireg].cbTotal  = lpMDBPB->cpageBitFAT * (long)cbPER_BITFAT_PAGE;
   // lpAreg[iregBIITFAT].cbActive is computed below, after we know how large
   // the sector heap is.

   // Compute RESERVED1 region

   ireg++;                             // iregRESERVED1
   lpAreg[ireg].ibStart  = lpAreg[iregBITFAT].ibStart + lpAreg[iregBITFAT].cbTotal;
   lpAreg[ireg].cbTotal  = csecRESERVED1 * (long)cbPerSec;
   lpAreg[ireg].cbActive = 0;            // none in use

   // Compute MDFAT region

   ireg++;                             // iregMDFAT

   // The MDFAT starts just after the BitFAT so
   //   secMDFATStart * cbPerSec + cbPerSec (for MDBPB) should ==
   //   lpAreg[iregBITFAT].ibStart + lpAreg[iregBITFAT].cbTotal

   lpAreg[ireg].ibStart  = lpMDBPB->secMDFATStart * (long)cbPerSec + (long)cbPerSec;

   // The MDFAT size depends on the maximum number of clusters that the CVF
   // could hold, but we will compute it instead by inference from the
   // other information we have, so we'll calculate the MDFAT size as
   // MDReserved - BitFAT size - MDBPB size - other reserved sizes

   lpAreg[ireg].cbTotal  = (lpMDBPB->csecMDReserved * (long)cbPerSec) -
                         lpAreg[iregMDBPB].cbTotal -     // MDBPB size
                         lpAreg[iregBITFAT].cbTotal -    // BitFAT size
                         lpAreg[iregRESERVED1].cbTotal - // RESERVED1 size
                         (csecRESERVED2 * (long)cbPerSec); // RESERVED2 size
   lpAreg[ireg].cbActive = ccluTotal * cbMDFATENTRY;

   // Compute RESERVED2 region

   ireg++;                             // iregRESERVED2
   lpAreg[ireg].ibStart  = lpAreg[iregMDFAT].ibStart + lpAreg[iregMDFAT].cbTotal;
   lpAreg[ireg].cbTotal  = csecRESERVED2 * (long)cbPerSec;
   lpAreg[ireg].cbActive = 0;            // none in use

   // Compute BOOT  region

   ireg++;                             // iregDOSBOOT
   lpAreg[ireg].ibStart  = lpMDBPB->csecMDReserved * (long)cbPerSec;
   lpAreg[ireg].cbTotal  = cbPerSec;
   lpAreg[ireg].cbActive = cbPerSec;

   // Compute RESERVED3 region

   ireg++;                             // iregRESERVED3
   lpAreg[ireg].ibStart  = lpAreg[iregDOSBOOT].ibStart
                         + lpAreg[iregDOSBOOT].cbTotal;
   lpAreg[ireg].cbTotal  = (lpMDBPB->csecReserved - 1) * (long)cbPerSec;
   lpAreg[ireg].cbActive = 0;

   // Compute DOSFAT region

   ireg++;                             // iregDOSFAT
   lpAreg[ireg].ibStart  = lpAreg[iregDOSBOOT].ibStart +
                         (lpMDBPB->csecReserved * (long)cbPerSec);
   lpAreg[ireg].cbTotal  = lpMDBPB->csecFAT * (long)cbPerSec;
   lpAreg[ireg].cbActive = lpMDBPB->f12BitFAT ?
                         ((ccluTotal * 3)/2) :         // 12-bit FAT
                         (ccluTotal * 2);              // 16-bit FAT

   // Compute ROOTDIR  region

   ireg++;                             // iregDOSROOTDIR
   lpAreg[ireg].ibStart  = (lpMDBPB->secRootDirStart + lpMDBPB->csecMDReserved)
                         * (long)cbPerSec;
   lpAreg[ireg].cbTotal  = lpMDBPB->cRootDirEntries * cbDIR_ENT;
   lpAreg[ireg].cbActive = lpAreg[ireg].cbTotal;

   // Compute RESERVED4 region

   ireg++;                             // iregRESERVED4
   lpAreg[ireg].ibStart  = lpAreg[iregDOSROOTDIR].ibStart
                         + lpAreg[iregDOSROOTDIR].cbTotal;
   lpAreg[ireg].cbTotal  = csecRESERVED4 * (long)cbPerSec;
   lpAreg[ireg].cbActive = 0;

   // Compute SECTORHEAP  region

   ireg++;                             // iregSECTORHEAP
   lpAreg[ireg].ibStart  = lpAreg[iregRESERVED4].ibStart
                         + lpAreg[iregRESERVED4].cbTotal;

   // Total and Active SECTORHEAP sizes are the same -- unlike other
   // regions, the SECTORHEAP is not preallocated for the max capacity.
   // The SECTORHEAP is followed by the 2nd MD STAMP, which occupies
   // the last <2 sectors of the CVF.  Since we already did the
   // RETRACT_STAMP computation above, all we have to do is subtract
   // the start of the sector heap from the start of the 2nd stamp.

   lpAreg[ireg].cbTotal  = ibCVFStamp2 - lpAreg[ireg].ibStart;
   lpAreg[ireg].cbActive = lpAreg[ireg].cbTotal;

   // Now we can compute the active region of the BitFAT.  There is
   // one bit in the BitFAT for every sector in the sector heap, and
   // we round up to the nearest byte.
   lpAreg[iregBITFAT].cbActive =
                           (lpAreg[iregSECTORHEAP].cbTotal/cbPerSec + 7) / 8;


   return(FALSE);            // IT WORKED!!
}


// THIS NEXT FUNCTION READS THE 'FAT' AND 'MDFAT' ENTRIES ASSOCIATED
// WITH 'dwClust', returning the FAT entry and placing the MDFAT info
// into the 'lpMDFAT' buffer.


#define FAT12ODD(X)  (((*((BYTE FAR *)(X))) >> 4) + \
                      ((*((BYTE FAR *)((X) + 1))) << 4))
#define FAT12EVEN(X) ((*((BYTE FAR *)(X))) + \
                      (((*((BYTE FAR *)((X) + 1))) & 0x0f) << 8))


WORD FAR PASCAL DblspaceGetFATEntry(HFILE hFile, LPMDRGN lpAreg,
                                    WORD wClust, LPMDFATENTRY lpMDFAT,
                                    LPMDBPB lpMDBPB, LPSTR FAR *lpFATBuf)
{
LPSTR lpB1, lpB2;
DWORD FAR *lpDW1, FAR *lpDW2;
DWORD dwDOSFAT, dwMDFAT;
WORD wRVal;
BOOL fOddEven;


   if(!*lpFATBuf)
   {
      *lpFATBuf = GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                 16384L * 2L + 2L * sizeof(DWORD));
      if(!*lpFATBuf) return(0);
   }

   lpDW1 = (DWORD FAR *)(*lpFATBuf);
   lpDW2 = (DWORD FAR *)(*lpFATBuf + sizeof(DWORD));

   lpB1 = *lpFATBuf + sizeof(DWORD) * 2;
   lpB2 = *lpFATBuf + sizeof(DWORD) * 2 + (unsigned short)16384;


   // CALCULATE the area from which to read the FAT entry and MDFAT entry

   if(lpMDBPB->f12BitFAT)      // are we using a '12-bit' FAT??
   {
      fOddEven = (wClust & 1)?1:0;   // ODD values require 2 successive bytes

      dwDOSFAT = (wClust * 3L) / 2L; // 3/2 bytes per entry
   }
   else
   {
      fOddEven = 0;             // always even!

      dwDOSFAT = wClust * 2L;   // 2 bytes per entry
   }

   dwDOSFAT += lpAreg[iregDOSFAT].ibStart;  // get actual start position

   dwMDFAT = lpAreg[iregMDFAT].ibStart + cbMDFATENTRY * wClust;


   // NEXT, see if DOSFAT ENTRY is within the range of the current buffer

   if(!*lpDW1 || *lpDW1> dwDOSFAT || (*lpDW1 + 16384) <= dwDOSFAT)
   {
      *lpDW1 = ((dwDOSFAT - lpAreg[iregDOSFAT].ibStart) & ~(16384L))
               + lpAreg[iregDOSFAT].ibStart;

      _llseek(hFile, *lpDW1, SEEK_SET);

      _lread(hFile, lpB1, 16384);
   }

   if(lpMDBPB->f12BitFAT)
   {
      if(fOddEven)
      {
         wRVal = FAT12ODD(lpB1 + (dwDOSFAT - *lpDW1));
      }
      else
      {
         wRVal = FAT12EVEN(lpB1 + (dwDOSFAT - *lpDW1));
      }
   }
   else
   {
      wRVal = *((WORD FAR *)(lpB1 + (dwDOSFAT - *lpDW1)));
   }

   if(!*lpDW2 || *lpDW2 > dwMDFAT || (*lpDW2 + 16384) <= dwMDFAT)
   {
      *lpDW2 = ((dwMDFAT - lpAreg[iregMDFAT].ibStart) & ~(16384L))
               + lpAreg[iregMDFAT].ibStart;

      _llseek(hFile, *lpDW2, SEEK_SET);

      _lread(hFile, lpB2, 16384);
   }

   _fmemcpy((LPSTR)lpMDFAT, lpB2 + (dwMDFAT - *lpDW2), sizeof(*lpMDFAT));


   return(wRVal);
}



// THIS NEXT FUNCTION DOES AN 'ABSOLUTE' READ OF A CLUSTER FROM THE
// CVF FOR A PARTICULAR CLUSTER.  STARTING CLUSTER IS ALWAYS '2'.
// THE RETURN VALUE IS THE TOTAL NUMBER OF BYTES ACTUALLY READ!

WORD FAR PASCAL DblspaceAbsoluteRead(HFILE hFile, LPMDRGN lpAreg,
                                     LPMDFATENTRY lpMDFAT, WORD wCluster,
                                     LPSTR FAR *lpFATBuf, LPSTR lpClusterBuf)
{
WORD wRval;
MDFATENTRY mdf;
DWORD FAR *lpDW2, dwMDFAT;
LPSTR lpB2;



   if(!lpMDFAT) lpMDFAT = &mdf;


   dwMDFAT = lpAreg[iregMDFAT].ibStart + wCluster * cbMDFATENTRY;

   if(lpFATBuf)
   {
      if(!*lpFATBuf)
      {
         *lpFATBuf = GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                 16384L * 2L + 2L * sizeof(DWORD));
         if(!*lpFATBuf) return(0);
      }

      lpDW2 = (DWORD FAR *)(*lpFATBuf + sizeof(DWORD));

      lpB2 = *lpFATBuf + sizeof(DWORD) * 2 + (unsigned short)16384;


      if(!*lpDW2 || *lpDW2 > dwMDFAT || (*lpDW2 + 16384) <= dwMDFAT)
      {
         *lpDW2 = ((dwMDFAT - lpAreg[iregMDFAT].ibStart) & ~(16384L))
                  + lpAreg[iregMDFAT].ibStart;

         _llseek(hFile, *lpDW2, SEEK_SET);

         _lread(hFile, lpB2, 16384);
      }

      _fmemcpy((LPSTR)lpMDFAT, lpB2 + (dwMDFAT - *lpDW2), sizeof(*lpMDFAT));

   }
   else
   {
      _llseek(hFile, dwMDFAT, SEEK_SET);

      _lread(hFile, lpMDFAT, sizeof(*lpMDFAT));
   }


   // now that I have the MDFAT entry, get the actual data and place it
   // into 'lpClusterBuf'.

   wRval = (lpMDFAT->csecCoded + 1) * 512;  // for now, bytes/sec is 512,
                                            // and HARD CODED!!

   _llseek(hFile, lpAreg[iregSECTORHEAP].ibStart + lpMDFAT->secStart * 512L,
           SEEK_SET);

   _lread(hFile, lpClusterBuf, wRval);  // READ THE BLOOMIN' DATA!! YES!!


   return(wRval);

}


#pragma auto_inline()



__inline void PASCAL ParseFileNameCorrectly(LPSTR lpStart, LPSTR lpEnd,
                                            LPSTR namebuf, LPSTR extbuf)
{
LPSTR lp3, lp4;


   _fmemset(namebuf, ' ', 8);
   _fmemset(extbuf, ' ', 3);

   for(lp3=lpStart, lp4=namebuf; lp3<lpEnd; lp3++, lp4++)
   {
      if(*lp3=='.' && ((lp3 + 1)>=lpEnd || lp3[1]!='.' || lp3!=lpStart))
      {                    // I found an extension, I found an extension...
         break;
      }

      *lp4 = toupper(*lp3);
   }

   while(lp3<lpEnd && *lp3!='.') lp3++;    // keep going until a '.'
   if(*lp3=='.')
   {
      for(lp3++, lp4 = extbuf; lp3<lpEnd; lp3++, lp4++)
      {
         *lp4 = toupper(*lp3);
      }
   }
}



//INT 21h function 4302h
//
//Get physical size.  The physical size is the amount of space the
//given file or directory occupies on the volume.  For compressed
//volumes, this is the compressed size.  For uncompressed volumes,
//this is the file size rounded up to the nearest cluster boundary.
//Note that this call works on directories as well as files; thus,
//you can tell how much space a directory itself consumes.
//
//ENTRY   (AX) = 4302h
//        (DS:DX) = path pointer
//
//EXIT    Carry clear
//            (DX:AX) = physical size of file or directory in bytes
//        Carry set
//            (AX) = error code


long FAR PASCAL DblspaceGetCompressedFileSize(LPSTR lpName)
{
long rval = -1; // an error return value, initially


   if(IsChicago) _asm
   {
      push ds
      lds dx, lpName

      mov ax, 0x7143    // see additional info, below
      mov bl, 2
      int 0x21          // GET PHYSICAL FILE SIZE

      pop ds

      jc bailout2
      mov WORD PTR rval, ax
      mov WORD PTR rval + 2, dx

bailout2:

   }
   else _asm
   {
      push ds
      lds dx, lpName

      mov ax, 0x4302
      int 0x21          // GET PHYSICAL FILE SIZE

      pop ds

      jc bailout
      mov WORD PTR rval, ax
      mov WORD PTR rval + 2, dx

bailout:

   }

   return(rval);
}



//long FAR PASCAL OldDblspaceGetCompressedFileSize(LPSTR lpName)
//{
//MDRGN areg[11];               // region info (see function above)
//MDBPB mdbpb;                  // 'bpb' for dblspace info
//HFILE hFile;
//char szName[18];              // size of CVF file name string is 18 bytes!
//BOOL fSwapped;
//BYTE fHost;
//long size=0L;
//int seq;
//LPSTR lpClustBuf, lpFatBuf=NULL, lp1, lp2;
//WORD wCluster, w;
//DWORD dw1;
//LPDOSDIRENTRY lpDI;
//char namebuf[8], extbuf[3];
//
//
//   // ON ENTRY IT IS ASSUMED THAT 'lpName' IS A FULLY QUALIFIED PATH!
//   // Therefore, find out the 'CVF' file name and open the thing.
//
//   if(!IsDoubleSpaceDrive((BYTE)(toupper(*lpName) - 'A'), &fSwapped,
//                          &fHost, &seq))
//   {
//      return(-1L);             // this signals an ERROR!
//   }
//
//   szName[0] = fHost + 'A';
//   wsprintf(szName + 1, ":\\DBLSPACE.%03d", seq);
//
//   hFile = _lopen(szName, OF_READ | OF_SHARE_DENY_NONE);
//
//   if(hFile==HFILE_ERROR) return(-1L);
//
//
//   // NEXT, get 'position' info for various regions within the file.
//
//   if(DblspaceComputeRegions(hFile, areg, &mdbpb))
//   {
//      _lclose(hFile);
//
//      return(-1L);
//   }
//
//
//   // NEXT, starting with the ROOT DIRECTORY read the file and get
//   // DIRECTORY information until I find the entry I'm looking for.
//
//
//   lpClustBuf = GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT, cbCLUSTERSIZE);
//                          // 1 CLUSTER IS ALWAYS 16 * 512 bytes IN LENGTH!!
//
//   if(!lpClustBuf)
//   {
//      _lclose(hFile);
//
//      return(-1L);
//   }
//
//
//   // FIND THE APPROPRIATE ENTRY IN THE ROOT DIRECTORY!!
//
//   lp1 = lpName + 3;               // for 'D:\PATH' - points to the letter 'P'
//   lp2 = _fstrchr(lpName, '\\');   // find the next '\'
//   if(!lp2) lp2 = lp1 + lstrlen(lp1);  // if none, point to end of string
//
//
//   ParseFileNameCorrectly(lp1, lp2, namebuf, extbuf);
//
//   wCluster = 0;
//
//   for(dw1=0; dw1<(DWORD)(areg[iregDOSROOTDIR].cbActive);
//       dw1 += cbCLUSTERSIZE)
//   {
//      _llseek(hFile, areg[iregDOSROOTDIR].ibStart + dw1, SEEK_SET);
//
//      _lread(hFile, lpClustBuf, cbCLUSTERSIZE);
//
//      for(w=0, lpDI=(LPDOSDIRENTRY)lpClustBuf;
//          w < (cbCLUSTERSIZE / sizeof(DOSDIRENTRY)); w++, lpDI++)
//      {
//         if(!_fmemcmp(lpDI->name, namebuf, sizeof(namebuf)) &&
//            !_fmemcmp(lpDI->ext, extbuf, sizeof(extbuf)))
//         {
//            // ENTRY FOUND IN ROOT DIRECTORY!  Get starting sector
//
//            wCluster = lpDI->cluster;
//
//            break;
//         }
//
//
//      }
//
//      if(w < (cbCLUSTERSIZE / sizeof(DOSDIRENTRY)))
//      {
//         // THIS MEANS WE FOUND THE ENTRY (ABOVE)
//
//         break;
//      }
//
//   }
//
//   // RIGHT ON!  OK, now that we have the starting cluster, we shall go
//   // through the file
//
//
//   // FINALLY, trace through the file's clusters, and get
//
//   GlobalFreePtr(lpClustBuf);
//   if(lpFatBuf) GlobalFreePtr(lpFatBuf);
//
//   _lclose(hFile);
//
//   return(size);                // for now, this is what is returned!!
//}




LPSTR FAR PASCAL DblspaceCompressionRatio(long lFileSize, long lCVFSize)
{   // returns pointer to a string representing the compression ratio
static char pRatio[16];
//WORD w;


   if(!lCVFSize)
   {
      return(" N/A ");
   }
   else if(lCVFSize == -1)          // an error value
   {
      return(" ERR ");
   }

   wsprintf(pRatio, "%4ld",
            (long)(10.0 * (double)lFileSize / (double)lCVFSize + .5));

   pRatio[5] = 0;
   pRatio[4] = pRatio[3];
   pRatio[3] = '.';
   if(pRatio[2]<=' ') pRatio[2]='0';

//   lstrltrim(pRatio);
//   lstrcat(pRatio, " to 1.");

   return(pRatio);

}
