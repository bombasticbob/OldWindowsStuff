/***************************************************************************/
/*                                                                         */
/*   WINFRMAT.C - Command line interpreter for Microsoft(r) Windows (tm)   */
/*                                                                         */
/*                      TRADE SECRET - CONFIDENTIAL                        */
/*                                                                         */
/*             The compiled libraries and API header files are             */
/*          (c) Copyright 1990-1995 by Stewart~Frazier Tools, Inc          */
/*                                                                         */
/*   This contains the 'FORMAT' processing functions, as well as some      */
/*   rather generic DPMI real-mode call/interrupt stuff.                   */
/*                                                                         */
/***************************************************************************/


#define THE_WORKS
#include "mywin.h"

#include "mth.h"
#include "wincmd.h"
#include "wincmd_f.h"


#define INLINE    /* __inline */



                       /** STRUCTURES & DEFINES **/

#define BYTES_PER_SECTOR    512
#define BYTES_PER_SECTOR_ID 2

#define CARRY_SET 1   /* mask for CARRY bit in 'flags' (above structure) */

#pragma pack(1)

typedef struct {                    /* Describes format of a sector */
   BYTE Track;         /* which cylinder */
   BYTE Side;          /* head/cyl info */
   BYTE Sector;        /* the logical sector # */
   BYTE Length;        /* the length 'id' of the sector in bytes */
                       /* 0==128, 1==256, 2==512, 3==1024 */
   } FORMAT;
typedef FORMAT FAR *LPFORMAT;

typedef struct {
   BYTE jump[3];            /* jump code */
   BYTE oemname[8];         /* OEM description */
   WORD BytesPerSector;     /* always 512 */
   BYTE ClusterSize;        /* always 2 for floppies */
   WORD ReservedSectors;    /* always 1 (boot sector only) */
   BYTE NumFats;            /* always 2 */
   WORD RootDirSize;        /* depends on disk */
   WORD TotalSectors;       /* total # of sectors */
   BYTE MediaByte;          /* depends on disk - defines MEDIA type */
   WORD SectorsPerFat;      /* # of sectors in a FAT */
   WORD SectorsPerTrack;    /* # of sectors in a track */
   WORD NumHeads;           /* # of heads */
   DWORD NumHiddenSectors;  /* # of 'hidden' sectors (always 0) */
   DWORD Dos4Sectors;       /* total # of sectors, DOS 4 style */
   BYTE PhysicalDriveNum;   /* Physical Drive # */
   BYTE reserved_usage;     /* reserved for later... */
   BYTE ExtendedBootSig;    /* 29H for EXTENDED BOOT INFO */
   DWORD SerialNumber;      /* disk serial number - random? */
   BYTE VolumeName[11];     /* 11 byte volume name, or "NO NAME    " */
   BYTE FatStyle[8];        /* may be OEM dependent; usually "FAT 12  " */
    /** OFFSET 0x3e **/
   BYTE BootCode[0x200 - 0x3e];
   } BOOT_SECTOR;

typedef BOOT_SECTOR FAR *LPBOOT_SECTOR;



typedef struct _tagBPB_ {
   WORD wBytesPerSector;    // always 512
   BYTE ClusterSize;        // always 2 for floppies
   WORD ReservedSectors;    // always 1 (boot sector only)
   BYTE NumFats;            // always 2
   WORD RootDirSize;        // depends on disk
   WORD TotalSectors;       // total # of sectors
   BYTE MediaByte;          // depends on disk - defines MEDIA type
   WORD SectorsPerFat;      // # of sectors in a FAT
   WORD SectorsPerTrack;    // # of sectors in a track
   WORD NumHeads;           // # of heads
   DWORD NumHiddenSectors;  // # of 'hidden' sectors (always 0)
   DWORD Dos4Sectors;       // total # of sectors, DOS 4 style
   BYTE PhysicalDriveNum;   // Physical Drive #
   BYTE reserved_usage;     // reserved for later...
   BYTE ExtendedBootSig;    // 29H for EXTENDED BOOT INFO
   WORD wReserved;          // 1st half of SerialNumber
   BYTE bReserved;          // next byte from SerialNumber (offset 1E)
  } BPB, FAR *LPBPB;

  // 'BPB' structure MUST be 31 bytes long, exactly


#pragma pack()


                      /*** FUNCTION PROTOTYPES ***/




BOOL FAR PASCAL SetMediaTypeForFormat(WORD wDrive, WORD wHeads,
                                      WORD wTracks, WORD wSectors,
                                      WORD wMediaByte, WORD wFatSize,
                                      WORD wRootSize, WORD wDriveType,
                                      DWORD dwDosBlock, WORD wDosBlockSize,
                                      DWORD FAR *lpdwMDT);

BOOL FAR PASCAL DOSSetMediaTypeForFormat(WORD wDrive, WORD wHeads,
                                         WORD wTracks, WORD wSectors,
                                         WORD wMediaByte, WORD wFatSize,
                                         WORD wRootSize, WORD wDriveType,
                                         DWORD dwDosBlock, WORD wDosBlockSize,
                                         DWORD FAR *lpdwMDT);

BOOL FAR PASCAL BIOSSetMediaTypeForFormat(WORD wDrive, WORD wHeads,
                                          WORD wTracks, WORD wSectors,
                                          WORD wMediaByte, WORD wFatSize,
                                          WORD wRootSize, WORD wDriveType,
                                          DWORD dwDosBlock, WORD wDosBlockSize,
                                          DWORD FAR *lpdwMDT);

BOOL FAR PASCAL FormatHeadTrackSector(WORD wDrive, WORD wHead, WORD wTrack,
                                      WORD wTotalTracks, WORD wSectors,
                                      LPFORMAT lpF, DWORD dwDosBlock,
                                      LPSTR lpSrcMDT, LPSTR lpSysMDT,
                                      WORD wSP);

BOOL FAR PASCAL WriteHeadTrackSector(WORD wDrive, WORD wHead,
                                     WORD wTrack, WORD wSector,
                                     WORD wDosSeg, WORD wSP);

BOOL FAR PASCAL WriteAbsoluteSector(WORD wDrive, WORD wAbsSector,
                                    WORD wHeads, WORD wSectors,
                                    WORD wDosSeg, WORD wSP);

BOOL FAR PASCAL ReadHeadTrackSector(WORD wDrive, WORD wHead,
                                    WORD wTrack, WORD wSector,
                                    WORD wDosSeg, WORD wSP);


                      /*** GLOBAL VARIABLES ***/


WORD wFormatCurDrive=0xffff, wFormatCurHead=0, wFormatCurTrack=0;



                         /*** "THE CODE" ***/


#pragma code_seg ("FORMAT_TEXT","CODE")


__inline BOOL FAR PASCAL PerformGenericIOCTL_RM(WORD wDrive, WORD wFunction,
                                                LPVOID lpParmBlock,
                                                DWORD cbBlockSize,
                                                DWORD dwDosBlock, WORD wSP)
{
REALMODECALL rmc;
BOOL rval;


   if(SELECTOROF(lpParmBlock)==LOWORD(dwDosBlock))
   {
      rmc.EDX = OFFSETOF(lpParmBlock);
   }
   else if(lpParmBlock && cbBlockSize)
   {
      rmc.EDX = 0;

      _fmemcpy(MAKELP(LOWORD(dwDosBlock),0), lpParmBlock, (WORD)cbBlockSize);
   }

   rmc.EAX = 0x440d;
   rmc.EBX = (wDrive & 0xff);
   rmc.ECX = 0x0800 | (wFunction & 0xff);

   rmc.DS = rmc.ES = rmc.SS = HIWORD(dwDosBlock);

   rmc.SP = wSP;


   rmc.ESI = rmc.DS;
   rmc.EDI = rmc.EDX;

   if(RealModeInt(0x21, &rmc))
   {
      return(0xffff);
   }


   if(SELECTOROF(lpParmBlock)!=LOWORD(dwDosBlock) &&
      lpParmBlock && cbBlockSize)
   {
      _fmemcpy(lpParmBlock, MAKELP(LOWORD(dwDosBlock),0), (WORD)cbBlockSize);
   }


   if(rmc.flags & CARRY_SET)
   {
      return(((WORD)rmc.EAX)?(WORD)rmc.EAX:0x8000);
   }
   else
   {
      return(FALSE);
   }
}


__inline BOOL FAR PASCAL PerformGenericIOCTL(WORD wDrive, WORD wFunction,
                                             LPVOID lpParmBlock)
{
BOOL rval = FALSE;


   _asm
   {
      push ds
      push bx
      push cx
      push si
      push di

      clc
      mov ax, 0x440d
      mov bl, BYTE PTR wDrive
      mov ch, 0x08
      mov cl, BYTE PTR wFunction
      lds dx, DWORD PTR lpParmBlock

      mov si, ds    // OS/2 'penalty box' seems to want this...
      mov di, dx

      call DOS3Call

      pop di
      pop si
      pop cx
      pop bx
      pop ds

      jnc not_error

      mov ax, 0x5900
      call DOS3Call      // get extended error code

      or ax, 0x8000    // ensure highest bit is set...
      mov rval, ax

not_error:

   }

   if(rval) return(rval & 0x7fff?rval & 0x7fff:0x8000);
   else     return(FALSE);
}







BOOL FAR PASCAL GetDriveParms(WORD wDrive, WORD FAR *lpwDriveType,
                              WORD FAR *lpwMaxCyl, WORD FAR *lpwMaxSector,
                              WORD FAR *lpwHeads, WORD FAR *lpwNumDrives)
{
REALMODECALL rmc;
DWORD dwDosBlock;


   dwDosBlock = GlobalDosAlloc(512);
   if(!dwDosBlock) return(TRUE);  /* error - no memory */

   _fmemset((LPSTR)&rmc, 0, sizeof(rmc));
   rmc.SS = HIWORD(dwDosBlock);
   rmc.SP = 510;    /* top of block - 2 */

   rmc.EAX = 0x800;
   rmc.EDX = wDrive;

   if(RealModeInt(0x13, &rmc) || (rmc.flags & CARRY_SET))
   {
      GlobalDosFree(LOWORD(dwDosBlock));
      return(TRUE);
   }

   if(lpwDriveType) *lpwDriveType = (WORD)(rmc.EBX & 0xff);
   if(lpwMaxCyl)    *lpwMaxCyl    = (WORD)((((rmc.ECX) >> 8) & 0xff) +
                                           ((((WORD)rmc.ECX) & 0xc0)<<2));
   if(lpwMaxSector) *lpwMaxSector = (WORD)(rmc.ECX & 0x3f);
   if(lpwHeads)     *lpwHeads     = (((WORD)rmc.EDX) >> 8) & 0xff;
   if(lpwNumDrives) *lpwNumDrives = ((WORD)rmc.EDX) & 0xff;



   GlobalDosFree(LOWORD(dwDosBlock));

   return(FALSE);

}


#pragma pack(1)

typedef struct {
  BYTE Operation;
  BYTE NumLocks;
  } PARAMBLOCK, FAR *LPPARAMBLOCK;

#pragma pack()

static WORD DriveAccessFlag[26];


#define IOCTL_LOCK_EXCLUSIVE 0  /* a 'level 0' lock */


BOOL PASCAL LockDrive(WORD wDrive, DWORD dwDosBlock, WORD wSP)
{
REALMODECALL rmc;
BOOL rval = FALSE;
char tbuf[128];


   wDrive++;    // 0 was 'A', 1 was 'B' - now 1 is 'A', 2 is 'B', etc.

   if(HIWORD(GetVersion()) >= 0x700)  // MS-DOS 7.0 or later
   {
      _fmemset((LPSTR)&rmc, 0, sizeof(rmc));
      rmc.SS = HIWORD(dwDosBlock);
      rmc.SP = wSP;                              /* top of block - 2 */
      rmc.EAX = 0x440d;

      rmc.EBX = (wDrive & 0xff) + (IOCTL_LOCK_EXCLUSIVE * 0x100);
      rmc.ECX = 0x84a;  // ch is '8' for DISK DRIVE; '4a' is function #

      rmc.EDX = 0;      // access flags (not used for level 0)

      rmc.DS = rmc.ES = rmc.SS;

      if(RealModeInt(0x21, &rmc))
      {
         rval = 0xffff;
      }
      else if(rmc.flags & CARRY_SET)
      {
         rval = (WORD)rmc.EAX;
      }
   }


   return(rval);
}

void PASCAL UnlockDrive(WORD wDrive, DWORD dwDosBlock, WORD wSP)
{
REALMODECALL rmc;



   wDrive++;    // 0 is 'A', 1 is 'B'

   if(HIWORD(GetVersion()) >= 0x700)  // MS-DOS 7.0 or later
   {
      _fmemset((LPSTR)&rmc, 0, sizeof(rmc));
      rmc.SS = HIWORD(dwDosBlock);
      rmc.SP = wSP;                              /* top of block - 2 */
      rmc.EAX = 0x440d;

      rmc.EBX = wDrive & 0xff;
      rmc.ECX = 0x86a;  // ch is '8' for DISK DRIVE; '6a' is function #

      rmc.DS = rmc.ES = rmc.SS;

      RealModeInt(0x21, &rmc);

   }

}


BOOL PASCAL LockDrive2(WORD wDrive, DWORD dwDosBlock, WORD wSP)
{
REALMODECALL rmc;
BOOL rval;


   rval = LockDrive(wDrive, dwDosBlock, wSP);
   if(rval) return(rval);


   wDrive++;    // 0 was 'A', 1 was 'B' - now 1 is 'A', 2 is 'B', etc.

   if(HIWORD(GetVersion()) >= 0x700)  // MS-DOS 7.0 or later
   {
      _fmemset((LPSTR)&rmc, 0, sizeof(rmc));
      rmc.SS = HIWORD(dwDosBlock);
      rmc.SP = wSP;                              /* top of block - 2 */
      rmc.EAX = 0x440d;

      rmc.EBX = (wDrive & 0xff) + (IOCTL_LOCK_EXCLUSIVE * 0x100);
      rmc.ECX = 0x84a;  // ch is '8' for DISK DRIVE; '4a' is function #

      rmc.EDX = 4;      // access flags (special for 2nd level 0 lock)

      rmc.DS = rmc.ES = rmc.SS;

      if(RealModeInt(0x21, &rmc))
      {
         rval = 0xffff;
      }
      else if(rmc.flags & CARRY_SET)
      {
         rval = (WORD)rmc.EAX;
      }
   }


   if(rval) UnlockDrive(wDrive, dwDosBlock, wSP);

   return(rval);
}

void PASCAL UnlockDrive2(WORD wDrive, DWORD dwDosBlock, WORD wSP)
{
   UnlockDrive(wDrive, dwDosBlock, wSP);
   UnlockDrive(wDrive, dwDosBlock, wSP);
}



BOOL FAR PASCAL FormatDrive(WORD wDrive, WORD wTracks, WORD wHeads,
                            WORD wSectors)
{
HGLOBAL hBoot;
LPBOOT_SECTOR lpBoot;
LPSTR lp1;
HRSRC hRes;
DWORD dwDosBlock, dwTemp;
BOOL AT;
REALMODECALL rmc;
WORD wTrack, wHead, i, wSP, wMDTSel, w;
LPFORMAT lpF;
WORD wDriveType, wMaxCyl, wMaxSector, wMaxHeads, wNumDrives,
     wRootSize, wFatSize, wMediaByte, format_pri;
BYTE SrcMDT[11];  /* contains Media Descriptor Table returned by int 13:18 */

extern BOOL FormatCancelFlag;  // defined in 'wincmd.c'



   /** Before I do anything, I *MUST* reset DOS's DISK SYSTEM! **/

   if(IsChicago) _asm
   {
      mov ax, 0x710d
      mov cx, 1           // flush flag:  '1' means flush ALL including cache
      mov dx, wDrive
      inc dx              // assume diskettes only...

      int 0x21            // flushes all drive buffers NOW!
   }
   else _asm
   {
      mov ax, 0xd00
      int 0x21
   }



   if(wDrive>=0x80) return(TRUE); /* ABSOLUTELY WILL NOT FORMAT Hard Disks */

   /** STEP 1:  Get drive parms for 'wDrive' **/

   if(GetDriveParms(wDrive, &wDriveType, &wMaxCyl, &wMaxSector,
                    &wMaxHeads, &wNumDrives))
   {
      return(TRUE);  /* error trying to get Drive Parms */
   }


   if(!wTracks)   wTracks  = wMaxCyl + 1;    /* 0 based */
   if(!wSectors)  wSectors = wMaxSector;     /* 1 based */
   if(!wHeads)    wHeads   = wMaxHeads + 1;  /* 0 based */

           /* see if desrmced track/head/sector is invalid */

   if((wMaxCyl + 1)<wTracks || wMaxSector<wSectors || (wMaxHeads + 1)<wHeads)
   {
      return(TRUE);  /** cannot format that configuration, eh? **/
   }


           /* create a DOS memory block for stack & Xfer area */

   wSP = wSectors * sizeof(FORMAT);
   if(wSP < BYTES_PER_SECTOR) wSP = BYTES_PER_SECTOR;

   wSP += 1022;        /* SP for DOS interrupt call == DOS BLOCK SIZE - 2 */

   if(!(dwDosBlock = GlobalDosAlloc(wSP + 2)))
   {
      return(TRUE);
   }


   // CHICAGO 'fix' - LOCK DRIVE BEFORE CONTINUING!

   if(LockDrive2(wDrive, dwDosBlock, wSP))
   {
      MessageBox(NULL, "Unable to LOCK the diskette for FORMAT!",
                 "* FORMAT ERROR *",
                 MB_OK | MB_ICONHAND);

      GlobalDosFree(LOWORD(dwDosBlock));
      return(TRUE);
   }


   lpF = (LPFORMAT)MAKELP(LOWORD(dwDosBlock), 0);    /* SEL:OFF */

                      /** RESET DISK SYSTEM FIRST! **/

   _fmemset((LPSTR)&rmc, 0, sizeof(rmc));
   rmc.SS = HIWORD(dwDosBlock);
   rmc.SP = wSP;    /* top of block - 2 */
   rmc.EAX = 0;     /* reset disk system */
   rmc.EDX = wDrive & 0xff;  /* drive ID */
   if(RealModeInt(0x13, &rmc))
   {
      UnlockDrive2(wDrive, dwDosBlock, wSP);
      GlobalDosFree(LOWORD(dwDosBlock));
      return(TRUE);
   }

   /** Now for the fun part:  set up to format 360k, 1.2M, 720k, or 1.44M **/

   _fmemset((LPSTR)&rmc, 0, sizeof(rmc));
   rmc.SS = HIWORD(dwDosBlock);
   rmc.SP = wSP;    /* top of block - 2 */

   switch(wDriveType)
   {
      case 1:         /* 360k drive - only support this format! */
         rmc.EAX = 0x1701;  /* format 360k disk in 360k drive */
         rmc.EDX = wDrive;  /* drive # */

         wRootSize  = 112;
         wFatSize   = 2;
         wMediaByte = 0xfd;

//         i = RealModeInt(0x13, &rmc);
         break;

      case 3:         /* 720k drive - only support this format! */
         rmc.EAX = 0x1704;  /* format 720k disk in 720k drive */
         rmc.EDX = wDrive;  /* drive # */
//         i = RealModeInt(0x13, &rmc);

         wRootSize  = 112;
         wFatSize   = 3;
         wMediaByte = 0xf9;

         break;

      case 2:         /* 1.2Mb drive */
         if(wTracks<=40 && wSectors<=9)  /* format 360k */
         {
            rmc.EAX = 0x1702;  /* format 360k disk in 1.2Mb drive */
            wRootSize  = 112;
            wFatSize   = 2;
            wMediaByte = 0xfd;
         }
         else
         {
            rmc.EAX = 0x1703;  /* format 1.2Mb disk in 1.2Mb drive */
            wRootSize  = 224;
            wFatSize   = 7;
            wMediaByte = 0xf9;
         }

         rmc.EDX = wDrive;  /* drive # */


//         i = RealModeInt(0x13, &rmc);
         break;

      case 4:         /* 1.44Mb drive */
         if(wSectors<=9)  /* format 720k */
         {
            rmc.EAX = 0x1705;  /* format 720k disk in 1.44Mb drive */
            wRootSize  = 112;
            wFatSize   = 3;
            wMediaByte = 0xf9;
         }
         else
         {
            rmc.EAX = 0x1706;  /* format 1.44Mb disk in 1.44Mb drive */
            wRootSize  = 224;
            wFatSize   = 9;
            wMediaByte = 0xf0;
         }

         rmc.EDX = wDrive;  /* drive # */


//         i = RealModeInt(0x13, &rmc);
         break;


      case 5:         /* 2.88Mb drive? */

         if(wSectors<=9 && wTracks<=80)  /* format 720k */
         {
            rmc.EAX = 0x1707;  /* format 720k disk in 2.88Mb drive? */
            wRootSize  = 112;
            wFatSize   = 3;
            wMediaByte = 0xf9;
         }
         else if(wSectors<=18 && wTracks<=80)
         {
            rmc.EAX = 0x1708;  /* format 1.44Mb disk in 2.88Mb drive? */
            wRootSize  = 224;
            wFatSize   = 9;
            wMediaByte = 0xf0;
         }
         else
         {
            rmc.EAX = 0x1709;  /* format 2.88Mb disk in 2.88Mb drive? */
            wRootSize  = 448;
            wFatSize   = 18;
            wMediaByte = 0xf0;
         }

         rmc.EDX = wDrive;  /* drive # */


//         i = RealModeInt(0x13, &rmc);
         break;

   }



                    /** SET MEDIA TYPE FOR FORMAT **/

   dwTemp = 0;  // linear address for 'MDT'

   if(SetMediaTypeForFormat(wDrive, wHeads, wTracks, wSectors,
                            wMediaByte, wFatSize, wRootSize,
                            wDriveType, dwDosBlock, wSP, &dwTemp))
   {
      UnlockDrive2(wDrive, dwDosBlock, wSP);
//      UnlockDrive(wDrive, dwDosBlock, wSP);
      GlobalDosFree(LOWORD(dwDosBlock));
      return(TRUE);
   }



   /* at this point ES:DI points to the BIOS's MDT for the format parms */
   /* get a copy of it and place the copy into SrcMDT[] (local var).    */

   if(!dwTemp)
   {
      wMDTSel = 0;
   }
   else
   {
      wMDTSel = MyAllocSelector();
      if(!wMDTSel)
      {
         UnlockDrive2(wDrive, dwDosBlock, wSP);
//         UnlockDrive(wDrive, dwDosBlock, wSP);
         GlobalDosFree(LOWORD(dwDosBlock));

         MessageBox(NULL, "Unable to format diskette!",
                    "* FORMAT ERROR *",
                    MB_OK | MB_SYSTEMMODAL | MB_ICONHAND);

         return(TRUE);
      }

      MyChangeSelector(wMDTSel, dwTemp, 0xffff);

      _fmemcpy(SrcMDT, MAKELP(wMDTSel, 0), 11);  /* copy it! */

      /* now, point the selector to the memory pointed to by int 1e vector */
      /* we basically have to assume that it won't change during format!   */


      MyChangeSelector(wMDTSel, 0x1e * sizeof(DWORD), 0xffff);

      dwTemp = *((DWORD FAR *)MAKELP(wMDTSel,0));  /* the VECTOR! */

      MyChangeSelector(wMDTSel, HIWORD(dwTemp) * 16L + LOWORD(dwTemp), 0xffff);

      /* 'wMDTSel' now points to the Media Descriptor Table (int 1e vector) */
   }

   wFormatCurDrive=wDrive;

   for(wTrack=0; wTrack<wTracks; wTrack++)
   {
      wFormatCurTrack = wTrack;

      for(wHead=0; wHead<wHeads; wHead++)
      {
         if(FormatCancelFlag)    // HAS THE FORMAT BEEN CANCELLED?????
         {
            UnlockDrive2(wDrive, dwDosBlock, wSP);
//            UnlockDrive(wDrive, dwDosBlock, wSP);

            if(wMDTSel) MyFreeSelector(wMDTSel);
            GlobalDosFree(LOWORD(dwDosBlock));
            return(TRUE);
         }

         wFormatCurHead = wHead;

         /** HERE IS WHERE I YIELD via LoopDispatch() or something... **/

         wFormatPercentComplete = (WORD)((100L * wHeads * wTrack + wHead)
                                          / (wHeads * wTracks));
         UpdateStatusBar();

         if(LoopDispatch()) break; /* if it returns TRUE... */

         w = FormatHeadTrackSector(wDrive, wHead, wTrack, wTracks, wSectors,
                                   lpF, dwDosBlock,
                                   SrcMDT, MAKELP(wMDTSel, 0), wSP);

         if(!FormatCancelFlag && w && w!=3 && w!=6 && w!=0x80)
         {
            if(SetMediaTypeForFormat(wDrive, wHeads, wTracks, wSectors,
                                     wMediaByte, wFatSize, wRootSize,
                                     wDriveType, dwDosBlock, wSP, &dwTemp))
            {
               UnlockDrive2(wDrive, dwDosBlock, wSP);
//               UnlockDrive(wDrive, dwDosBlock, wSP);

               if(wMDTSel) MyFreeSelector(wMDTSel);
               GlobalDosFree(LOWORD(dwDosBlock));
               return(TRUE);
            }

            w = FormatHeadTrackSector(wDrive, wHead, wTrack, wTracks, wSectors,
                                      lpF, dwDosBlock,
                                      SrcMDT, MAKELP(wMDTSel, 0), wSP);
         }

         if(!FormatCancelFlag && w && w!=3 && w!=6 && w!=0x80)
         {
            if(SetMediaTypeForFormat(wDrive, wHeads, wTracks, wSectors,
                                     wMediaByte, wFatSize, wRootSize,
                                     wDriveType, dwDosBlock, wSP, &dwTemp))
            {
               UnlockDrive2(wDrive, dwDosBlock, wSP);
//               UnlockDrive(wDrive, dwDosBlock, wSP);

               if(wMDTSel) MyFreeSelector(wMDTSel);
               GlobalDosFree(LOWORD(dwDosBlock));
               return(TRUE);
            }

            w = FormatHeadTrackSector(wDrive, wHead, wTrack, wTracks, wSectors,
                                      lpF, dwDosBlock,
                                      SrcMDT, MAKELP(wMDTSel, 0), wSP);
         }

         if(w)
         {
            UnlockDrive2(wDrive, dwDosBlock, wSP);
//            UnlockDrive(wDrive, dwDosBlock, wSP);

            if(wMDTSel) MyFreeSelector(wMDTSel);
            GlobalDosFree(LOWORD(dwDosBlock));

            if(w==3)
            {
               MessageBox(NULL, "?Diskette is write protected",
                          "* FORMAT ERROR *",
                          MB_OK | MB_SYSTEMMODAL | MB_ICONHAND);
            }
            else if(w==6)
            {
               MessageBox(NULL, "?Diskette removed from drive/door is open",
                          "* FORMAT ERROR *",
                          MB_OK | MB_SYSTEMMODAL | MB_ICONHAND);
            }
            else if(w==0x80)  /* disk timed out - possibly door is open */
            {
               MessageBox(NULL, "?Time-out error (drive door may be open)",
                          "* FORMAT ERROR *",
                          MB_OK | MB_SYSTEMMODAL | MB_ICONHAND);
            }
            else
            {
               MessageBox(NULL, "Unable to format diskette!",
                          "* FORMAT ERROR *",
                          MB_OK | MB_SYSTEMMODAL | MB_ICONHAND);
            }

            wFormatCurDrive = 0xffff;
            wFormatCurHead = 0;
            wFormatCurTrack = 0;
            wFormatPercentComplete= 0;

            return(w);

         }

         if(LoopDispatch()) break; /* if it returns TRUE... */


         if(!FormatCancelFlag && wHead == 0 && wTrack == 0)
         {
            //** Completed formatting Track #0, Head #0!!
            //** Now, write the BOOT SECTOR!  Must first obtain the
            //** resource, then copy to DOS mem, before writing sector

            hRes = FindResource(GetCurrentInstance(), "BOOT_SECTOR", "BOOTSECTOR");
            if(hRes)
            {
               hBoot = LoadResource(GetCurrentInstance(), hRes);

               if(hBoot)
               {
                  lpBoot = (LPBOOT_SECTOR) LockResource(hBoot);

                  _fmemcpy((LPSTR)lpF, (LPSTR)lpBoot, BYTES_PER_SECTOR);

                  UnlockResource(hBoot);
                  FreeResource(hBoot);
               }
               else
               {
                  UnlockDrive2(wDrive, dwDosBlock, wSP);
//                  UnlockDrive(wDrive, dwDosBlock, wSP);

                  FreeResource(hBoot);
                  if(wMDTSel) MyFreeSelector(wMDTSel);  /* I'm done with it now! */
                  GlobalDosFree(LOWORD(dwDosBlock));

                  MessageBox(NULL, "Unable to get resource for BOOT RECORD",
                             "* FORMAT ERROR *",
                             MB_OK | MB_SYSTEMMODAL | MB_ICONHAND);
                  return(TRUE);
               }

            }
            else
            {
               UnlockDrive2(wDrive, dwDosBlock, wSP);
//               UnlockDrive(wDrive, dwDosBlock, wSP);

               if(wMDTSel) MyFreeSelector(wMDTSel);  /* I'm done with it now! */
               GlobalDosFree(LOWORD(dwDosBlock));

               MessageBox(NULL, "Unable to get resource for BOOT RECORD",
                          "* FORMAT ERROR *",
                          MB_OK | MB_SYSTEMMODAL | MB_ICONHAND);

               return(TRUE);
            }


            LoopDispatch();


            lpBoot = (LPBOOT_SECTOR)lpF;  /* points to beginning of DOS memory */

            lpBoot->RootDirSize     = wRootSize;
            lpBoot->TotalSectors    = wHeads * wTracks * wSectors;
            lpBoot->MediaByte       = (BYTE)wMediaByte;
            lpBoot->SectorsPerFat   = wFatSize;
            lpBoot->SectorsPerTrack = wSectors;
            lpBoot->NumHeads        = wHeads;
            lpBoot->Dos4Sectors     = 0;       /* ignore this one */
            lpBoot->SerialNumber    = GetCurrentTime();  /* the serial # */


            /** now, write the boot sector **/

            if(WriteAbsoluteSector(wDrive, 1, wHeads, wSectors,
                                   HIWORD(dwDosBlock), wSP))
            {
               UnlockDrive2(wDrive, dwDosBlock, wSP);
//               UnlockDrive(wDrive, dwDosBlock, wSP);

               if(wMDTSel) MyFreeSelector(wMDTSel);  /* I'm done with it now! */
               GlobalDosFree(LOWORD(dwDosBlock));

               MessageBox(NULL, "Unable to write BOOT RECORD",
                          "* FORMAT ERROR *",
                          MB_OK | MB_SYSTEMMODAL | MB_ICONHAND);

               return(TRUE);                    /* format error - can't write... */
            }
         }
      }
   }

   if(wMDTSel) MyFreeSelector(wMDTSel);  /* I'm done with it now! */

   wFormatPercentComplete = 100;

   UpdateStatusBar();
   LoopDispatch();


//   /** Completed formatting drive.  Now, write the BOOT SECTOR **/
//   /** Must first obtain the resource, then copy to DOS mem.   **/
//
//   hRes = FindResource(GetCurrentInstance(), "BOOT_SECTOR", "BOOTSECTOR");
//   if(hRes)
//   {
//      hBoot = LoadResource(GetCurrentInstance(), hRes);
//
//      if(hBoot)
//      {
//         lpBoot = (LPBOOT_SECTOR) LockResource(hBoot);
//
//         _fmemcpy((LPSTR)lpF, (LPSTR)lpBoot, BYTES_PER_SECTOR);
//
//         UnlockResource(hBoot);
//         FreeResource(hBoot);
//      }
//      else
//      {
//         UnlockDrive2(wDrive, dwDosBlock, wSP);
//         FreeResource(hBoot);
//         GlobalDosFree(LOWORD(dwDosBlock));
//
//         MessageBox(NULL, "Unable to get resource for BOOT RECORD",
//                    "* FORMAT ERROR *",
//                    MB_OK | MB_SYSTEMMODAL | MB_ICONHAND);
//         return(TRUE);
//      }
//
//   }
//   else
//   {
//      UnlockDrive2(wDrive, dwDosBlock, wSP);
//      GlobalDosFree(LOWORD(dwDosBlock));
//
//      MessageBox(NULL, "Unable to get resource for BOOT RECORD",
//                 "* FORMAT ERROR *",
//                 MB_OK | MB_SYSTEMMODAL | MB_ICONHAND);
//
//      return(TRUE);
//   }
//
//
//   LoopDispatch();
//
//
//   lpBoot = (LPBOOT_SECTOR)lpF;  /* points to beginning of DOS memory */
//
//   lpBoot->RootDirSize     = wRootSize;
//   lpBoot->TotalSectors    = wHeads * wTracks * wSectors;
//   lpBoot->MediaByte       = (BYTE)wMediaByte;
//   lpBoot->SectorsPerFat   = wFatSize;
//   lpBoot->SectorsPerTrack = wSectors;
//   lpBoot->NumHeads        = wHeads;
//   lpBoot->Dos4Sectors     = 0;       /* ignore this one */
//   lpBoot->SerialNumber    = GetCurrentTime();  /* the serial # */
//
//
//   /** now, write the boot sector **/
//
//   if(WriteAbsoluteSector(wDrive, 1, wHeads, wSectors,
//                          HIWORD(dwDosBlock), wSP))
//   {
//      UnlockDrive2(wDrive, dwDosBlock, wSP);
//      GlobalDosFree(LOWORD(dwDosBlock));
//      return(TRUE);                    /* format error - can't write... */
//   }
//
   /* OK - next is the FAT - 2 copies, in which 1st sector has BOOT info */
   /* and the rest are full of 0's.                                      */

   lp1 = (LPSTR)lpF;

   _fmemset(lp1, 0, BYTES_PER_SECTOR);

   lp1[0] = (BYTE)wMediaByte;
   lp1[1] = 0xffU;
   lp1[2] = 0xffU;

   /* write the first FAT entry */

   if(WriteAbsoluteSector(wDrive, 2, wHeads, wSectors,
                          HIWORD(dwDosBlock), wSP))
   {
      UnlockDrive2(wDrive, dwDosBlock, wSP);
      GlobalDosFree(LOWORD(dwDosBlock));
      return(TRUE);                    /* format error - can't write... */
   }

   LoopDispatch();

   /* write the other '1st' FAT entry */

   if(WriteAbsoluteSector(wDrive, 2 + wFatSize, wHeads, wSectors,
                          HIWORD(dwDosBlock), wSP))
   {
      UnlockDrive2(wDrive, dwDosBlock, wSP);
      GlobalDosFree(LOWORD(dwDosBlock));
      return(TRUE);                    /* format error - can't write... */
   }


   /** next, write all of the remaining FAT tables **/

   *lp1 = 0;
   lp1[1] = 0;
   lp1[2] = 0;  /* remaining FAT records are all 0's */

   for(i=1; i<wFatSize; i++)
   {
      LoopDispatch();

      if(WriteAbsoluteSector(wDrive, 2 + i, wHeads, wSectors,
                             HIWORD(dwDosBlock), wSP) ||
         WriteAbsoluteSector(wDrive, 2 + i + wFatSize, wHeads, wSectors,
                             HIWORD(dwDosBlock), wSP))
      {
         UnlockDrive2(wDrive, dwDosBlock, wSP);
         GlobalDosFree(LOWORD(dwDosBlock));
         return(TRUE);                    /* format error - can't write... */
      }
   }

   /** NOW for some REAL fun!  The root dir *may* have enough entries to **/
   /** sneak onto the other side.  If so, I gotta know!                  **/

   for(i=2 + 2*wFatSize;
       i < (2 + 2*wFatSize + (wRootSize * 32)/BYTES_PER_SECTOR); i++)
   {
      LoopDispatch();

      if(WriteAbsoluteSector(wDrive, i, wHeads, wSectors,
                             HIWORD(dwDosBlock), wSP))
      {
         UnlockDrive2(wDrive, dwDosBlock, wSP);
         GlobalDosFree(LOWORD(dwDosBlock));
         return(TRUE);                    /* format error - can't write... */
      }
   }


   // flush disk system now in an attempt to ensure that
   // buffers for this diskette are clear before unlocking

   if(IsChicago) _asm
   {
      mov ax, 0x710d
      mov cx, 1           // flush flag:  '1' means flush ALL including cache
      mov dx, wDrive
      inc dx              // assume diskettes only...

      int 0x21            // flushes all drive buffers NOW!
   }
   else _asm
   {
      mov ax, 0xd00
      int 0x21
   }


   /* if it got here, I am done and it worked.  Only thing left to do is */
   /* write a LABEL (can be done using DOS) and transfer system on '/S'  */

   UnlockDrive2(wDrive, dwDosBlock, wSP);

   GlobalDosFree(LOWORD(dwDosBlock));


   /** Now that I'm done, make sure disk system is flushed, one more time **/

   if(IsChicago) _asm
   {
      mov ax, 0x710d
      mov cx, 1           // flush flag:  '1' means flush ALL including cache
      mov dx, wDrive
      inc dx              // assume diskettes only...

      int 0x21            // flushes all drive buffers NOW!
   }
   else _asm
   {
      mov ax, 0xd00
      int 0x21
   }


   return(FALSE);
}




BOOL FAR PASCAL SetMediaTypeForFormat(WORD wDrive, WORD wHeads,
                                      WORD wTracks, WORD wSectors,
                                      WORD wMediaByte, WORD wFatSize,
                                      WORD wRootSize, WORD wDriveType,
                                      DWORD dwDosBlock, WORD wDosBlockSize,
                                      DWORD FAR *lpdwMDT)
{
HCURSOR hCursor;
BOOL rval;


   hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

   if(IsChicago)  // set DOS parms also!!
   {
      rval = DOSSetMediaTypeForFormat(wDrive, wHeads, wTracks, wSectors,
                                      wMediaByte, wFatSize, wRootSize,
                                      wDriveType, dwDosBlock, wDosBlockSize,
                                      lpdwMDT);
   }


   rval = BIOSSetMediaTypeForFormat(wDrive, wHeads, wTracks, wSectors,
                                    wMediaByte, wFatSize, wRootSize,
                                    wDriveType, dwDosBlock, wDosBlockSize,
                                    lpdwMDT);

   if(hCursor) SetCursor(hCursor);

   return(rval);
}


BOOL FAR PASCAL BIOSSetMediaTypeForFormat(WORD wDrive, WORD wHeads,
                                          WORD wTracks, WORD wSectors,
                                          WORD wMediaByte, WORD wFatSize,
                                          WORD wRootSize, WORD wDriveType,
                                          DWORD dwDosBlock, WORD wDosBlockSize,
                                          DWORD FAR *lpdwMDT)
{
REALMODECALL rmc;
BOOL rval;


   _fmemset((LPSTR)&rmc, 0, sizeof(rmc));
   rmc.SS = HIWORD(dwDosBlock);
   rmc.SP = wDosBlockSize;                             /* top of block - 2 */

   rmc.EAX = 0x0;            /* RESET FLOPPY DISK SYSTEM */
   rmc.EDX = wDrive & 0xff;  /* drive # only */

   rmc.DS  = rmc.SS;
   rmc.ES  = rmc.SS;

   if(RealModeInt(0x13, &rmc) || (rmc.flags & CARRY_SET))
   {
#ifdef DEBUG
      PrintString("?Fatal error attempting to reset drive (1)\r\n\n");
#endif // DEBUG
      return(1);
   }

   _fmemset((LPSTR)&rmc, 0, sizeof(rmc));
   rmc.SS = HIWORD(dwDosBlock);
   rmc.SP = wDosBlockSize;                             /* top of block - 2 */
   rmc.EAX = 0x1800;                          /* set media type for FORMAT */
                        /* # of TRACKS is ZERO-BASED - indicate MAX value! */
   rmc.ECX = (((wTracks - 1)<< 8) & 0xff00) + (((wTracks - 1)>>2) & 0xc0)
             + (wSectors & 0xbf);

   rmc.EDX = wDrive & 0xff;             /* drive ID */

   rmc.DS = rmc.ES = rmc.SS;


   rval = RealModeInt(0x13, &rmc) || (rmc.flags & CARRY_SET);

   // return the real mode address of the 'MDT' (as appropriate)

   *lpdwMDT = (rmc.ES * 16L) + (rmc.EDI & 0xffff);

   return(rval);
}



BOOL FAR PASCAL DOSSetMediaTypeForFormat(WORD wDrive, WORD wHeads,
                                         WORD wTracks, WORD wSectors,
                                         WORD wMediaByte, WORD wFatSize,
                                         WORD wRootSize, WORD wDriveType,
                                         DWORD dwDosBlock, WORD wDosBlockSize,
                                         DWORD FAR *lpdwMDT)
{
BOOL rval;
WORD i;
BOOL AssignNewBPB = FALSE;

#pragma pack(1)
struct _tag_IOCTL_40_ {
   BYTE Flags;
   BYTE DevType;
   WORD wDevAttrib;
   WORD wNumCylinders;
   BYTE wMediaType;
   BPB  bpb;
   WORD wNumSectorsPerTrack;
   WORD pwTrackLayout[0];      // points to track layout information
 } FAR *lpIOCtlInfo40;
#pragma pack()

//struct _tag_2144_41_ {
//   BYTE Flags;
//   WORD wHead;
//   WORD wCylinder;
//   WORD wFirstSector;
//   WORD wNumSectors;
//   DWORD lpDTA;
// } IOCtlInfo41;



   *lpdwMDT = 0;  // this flags 'do not use MDT'

   wDrive++;  // need to add 1 to the drive letter (assume 'A' or 'B' only)

   lpIOCtlInfo40 = (struct _tag_IOCTL_40_ FAR *)MAKELP(LOWORD(dwDosBlock),0);

   _fmemset((LPSTR)lpIOCtlInfo40, 0, sizeof(*lpIOCtlInfo40));

   // Initially, get the drive media information via IOCTL sub-function 60H

   rval = PerformGenericIOCTL(wDrive, 0x60, lpIOCtlInfo40);
   if(rval)   // it failed - attempt to assign it "manually"
   {
      // ensure device media type set according to desired format

      switch(wDriveType)
      {
         case 1:         /* 360k drive - only support this format! */
            lpIOCtlInfo40->DevType = 0;
            break;

         case 3:         /* 720k drive - only support this format! */
            lpIOCtlInfo40->DevType = 2;
            break;

         case 2:         /* 1.2Mb drive */
            lpIOCtlInfo40->DevType = 1;
            break;

         case 4:         /* 1.44Mb drive */
            lpIOCtlInfo40->DevType = 7;
            break;

         case 5:         /* 2.88Mb drive? */
         default:
            lpIOCtlInfo40->DevType = 8;
            break;

      }

      lpIOCtlInfo40->wNumCylinders = wTracks;
      lpIOCtlInfo40->wMediaType    = 0;              // initially

      lpIOCtlInfo40->bpb.ClusterSize     = 2;        // always 2 for floppies
      lpIOCtlInfo40->bpb.ReservedSectors = 1;        // always 1 (boot sector only)
      lpIOCtlInfo40->bpb.NumFats         = 2;        // always 2

      lpIOCtlInfo40->bpb.NumHeads        = wHeads;   // # of heads
      lpIOCtlInfo40->bpb.NumHiddenSectors= 0;        // # of 'hidden' sectors (always 0)
      lpIOCtlInfo40->bpb.Dos4Sectors     = 0;        // total # of sectors, DOS 4 style

      lpIOCtlInfo40->bpb.PhysicalDriveNum = wDrive - 1;// Physical Drive #

      AssignNewBPB = TRUE;
   }


   // assign 'Media Type' byte for 1.2Mb drives

   if(lpIOCtlInfo40->DevType == 1  // 1.2Mb drive
      && wSectors <= 9)            // 360kb diskette format
   {
      lpIOCtlInfo40->wMediaType = 1; // 360k diskette in 1.2Mb drive
   }
   else
   {
      lpIOCtlInfo40->wMediaType = 0; // all other media types
   }


   // assign updated values for 'BPB' within function parm block,
   // if necessary, and flag whether they've been modified.

   if(lpIOCtlInfo40->bpb.wBytesPerSector != BYTES_PER_SECTOR)
   {
      lpIOCtlInfo40->bpb.wBytesPerSector = BYTES_PER_SECTOR;
      AssignNewBPB = TRUE;
   }

   if(lpIOCtlInfo40->bpb.RootDirSize != wRootSize)
   {
      lpIOCtlInfo40->bpb.RootDirSize = wRootSize;      // depends on disk
      AssignNewBPB = TRUE;
   }

   if(lpIOCtlInfo40->bpb.TotalSectors != wHeads * wTracks * wSectors)
   {
      lpIOCtlInfo40->bpb.TotalSectors = wHeads * wTracks * wSectors;
      AssignNewBPB = TRUE;
   }

   if(lpIOCtlInfo40->bpb.MediaByte != (BYTE)wMediaByte)
   {
      lpIOCtlInfo40->bpb.MediaByte = (BYTE)wMediaByte; // defines MEDIA type
      AssignNewBPB = TRUE;
   }

   if(lpIOCtlInfo40->bpb.SectorsPerFat != wFatSize)
   {
      lpIOCtlInfo40->bpb.SectorsPerFat = wFatSize;     // # of sectors in a FAT
      AssignNewBPB = TRUE;
   }

   if(lpIOCtlInfo40->bpb.SectorsPerTrack != wSectors)
   {
      lpIOCtlInfo40->bpb.SectorsPerTrack = wSectors;   // # of sectors in a track
      AssignNewBPB = TRUE;
   }

   if(lpIOCtlInfo40->bpb.reserved_usage != 0)
   {
      lpIOCtlInfo40->bpb.reserved_usage = 0;           // reserved for later...
      AssignNewBPB = TRUE;
   }

   if(lpIOCtlInfo40->bpb.ExtendedBootSig != 0)
   {
      lpIOCtlInfo40->bpb.ExtendedBootSig = 0;          // 29H for EXTENDED BOOT INFO
      AssignNewBPB = TRUE;
   }

   if(lpIOCtlInfo40->bpb.wReserved != 0)
   {
      lpIOCtlInfo40->bpb.wReserved = 0;                // 1st half of SerialNumber
      AssignNewBPB = TRUE;
   }



   // setup the 'track layout' section

   lpIOCtlInfo40->wNumSectorsPerTrack = wSectors;

   for(i=0; i<wSectors; i++)
   {
      lpIOCtlInfo40->pwTrackLayout[i * 2] = i + 1;
      lpIOCtlInfo40->pwTrackLayout[i * 2 + 1] = BYTES_PER_SECTOR;
   }

   lpIOCtlInfo40->pwTrackLayout[2 * wSectors] = 0;
   lpIOCtlInfo40->pwTrackLayout[2 * wSectors + 1] = 0;


   if(AssignNewBPB)
   {
      // set NEW drive parameters

      lpIOCtlInfo40->Flags = 0 + // bit 0 set to use old BPB (not new one)
                             0 + // bit 1 set to only use track layout fields
                             4;  // bit 2 set if all sectors in track same size

      rval = PerformGenericIOCTL(wDrive, 0x40, lpIOCtlInfo40);
      if(rval)
      {
         LoopDispatch();
         return(rval);
      }


      // NOW, do it again, but this time I want to use the 'old BPB' to see
      // if I correct a rather nasty bug this way...


      // get drive parameters (one more time)

      rval = PerformGenericIOCTL(wDrive, 0x60, lpIOCtlInfo40);
      if(rval)
      {
         LoopDispatch();
         return(rval);
      }


      // setup the 'track layout' section (again)

      lpIOCtlInfo40->wNumSectorsPerTrack = wSectors;

      for(i=0; i<wSectors; i++)
      {
         lpIOCtlInfo40->pwTrackLayout[i * 2] = i + 1;
         lpIOCtlInfo40->pwTrackLayout[i * 2 + 1] = BYTES_PER_SECTOR;
      }

      lpIOCtlInfo40->pwTrackLayout[2 * wSectors] = 0;
      lpIOCtlInfo40->pwTrackLayout[2 * wSectors + 1] = 0;
   }


   lpIOCtlInfo40->Flags = 1 + // bit 0 set to use old BPB (not new one)
                          0 + // bit 1 set to only use track layout fields
                          4;  // bit 2 set if all sectors in track same size

   rval = PerformGenericIOCTL(wDrive, 0x40, lpIOCtlInfo40);
   return(rval);


}









 /* wDrive: 'A'==0, 'B'==1, etc.                                           */
 /* lpF:  points to 'FORMAT' structure whose 'DOS' segment is in 'wDosSeg' */
 /*       and whose offset into this 'DOS' segment == DOS offset.          */
 /* wSP:  the stack pointer value on entry to the interrupt                */
 /* RETURNS:  1 if general failure, 2 if too many retries, 3 if write prot,*/
 /*           6 or 0x80 if disk drive door is open                         */

BOOL FAR PASCAL FormatHeadTrackSector(WORD wDrive, WORD wHead, WORD wTrack,
                                      WORD wTotalTracks, WORD wSectors,
                                      LPFORMAT lpF, DWORD dwDosBlock,
                                      LPSTR lpSrcMDT, LPSTR lpSysMDT,
                                      WORD wSP)
{
WORD i, iter;
REALMODECALL rmc;
WORD wDosSeg = HIWORD(dwDosBlock);


   for(i=0; i<wSectors; i++)
   {
      lpF[i].Track  = (BYTE)wTrack;
      lpF[i].Side   = (BYTE)wHead;
      lpF[i].Sector = i + 1;
      lpF[i].Length = BYTES_PER_SECTOR_ID; /* 0==128,1==256,2==512,3==1024 */
   }

   lpF[wSectors].Track  = 0;
   lpF[wSectors].Side   = 0;
   lpF[wSectors].Sector = 0;
   lpF[wSectors].Length = 0;

   for(iter=0; !iter || (iter<4 && !(wDrive & 0x80)); iter++)
   {
      /* before I attempt to format drive, copy the source 'MDT' */
      /* to the destination 'MDT'.                               */

      _fmemcpy(lpSysMDT, lpSrcMDT, 11); /* copy 'current format' data */

      lpSysMDT[4] = (BYTE)wSectors;  /* ensure it's set, no matter what! */

      _fmemset((LPSTR)&rmc, 0, sizeof(rmc));

      rmc.SS  = wDosSeg;  /* DOS segment */
      rmc.SP  = wSP;                 /* top of block - 2 */
      rmc.DS  = rmc.SS;

      rmc.EAX = 0x500 + (wSectors & 0xff);  /* format!! */

      rmc.ES  = rmc.SS;               /* same segment as STACK */
      rmc.EBX = OFFSETOF(lpF);        /* offset to FORMAT array */

      rmc.ECX = ((wTrack << 8) & 0xff00) + ((wTrack>>2) & 0xc0) + 1;
      rmc.EDX = ((wHead << 8) & 0xff00) + (wDrive & 0xff);

      if(RealModeInt(0x13, &rmc))
      {
         return(1);
      }

      if(!(rmc.flags & CARRY_SET))
      {
         return(FALSE);       /* so far so good!! */
      }


      i = (WORD)((rmc.EAX >> 8) & 0xff);  // get 'AL' on return for error type

      if(i==3)
      {
#ifdef DEBUG
         PrintString("?Disk is WRITE PROTECTED\r\n\n");
#endif // DEBUG
         if(iter>=3 || (wDrive & 0x80)) return(i);
      }
      else if(i==6)
      {
#ifdef DEBUG
         PrintString("?Disk removed/door open\r\n\n");
#endif // DEBUG
         if(iter>=3 || (wDrive & 0x80)) return(i);
      }
      else if(i==0x80)  /* disk timed out - possibly door is open */
      {                 /* retry max # of iterations and then bail out */
#ifdef DEBUG
         PrintString("?Diskette drive time-out\r\n\n");
#endif // DEBUG
         if(iter>=3 || (wDrive & 0x80)) return(i);
      }

      /* otherwise, retry up to 4 times in case motor isn't up to speed */

      LoopDispatch();  /* allow other things to run during re-try */

      _fmemset((LPSTR)&rmc, 0, sizeof(rmc));
      rmc.SS = wDosSeg;
      rmc.SP = wSP;             /* should be top of block - 2 */

      rmc.EAX = 0x0;            /* RESET FLOPPY DISK SYSTEM */
      rmc.EDX = wDrive & 0xff;  /* drive # only */

      rmc.DS  = rmc.SS;
      rmc.ES  = rmc.SS;

      if(RealModeInt(0x13, &rmc) || (rmc.flags & CARRY_SET))
      {
#ifdef DEBUG
         PrintString("?Fatal error attempting to reset drive (2)\r\n\n");
#endif // DEBUG
         return(1);
      }

      LoopDispatch();  /* allow other things to run during re-try */

                     /** 'RE'SET MEDIA TYPE FOR FORMAT **/

      _fmemset((LPSTR)&rmc, 0, sizeof(rmc));
      rmc.SS = wDosSeg;
      rmc.SP = wSP;                             /* top of block - 2 */
      rmc.EAX = 0x1800;                       /* set media type for FORMAT */

                        /* # of TRACKS is ZERO-BASED - indicate MAX value! */
      rmc.ECX = (((wTotalTracks - 1)<< 8) & 0xff00) + (((wTotalTracks - 1)>>2) & 0xc0)
                + (wSectors & 0xbf);

      rmc.EDX = wDrive & 0xff;             /* drive ID */

      rmc.DS = rmc.ES = rmc.SS;

      if(RealModeInt(0x13, &rmc) || (rmc.flags & CARRY_SET))
      {
#ifdef DEBUG
         PrintString("?Fatal error trying to reset drive parameters\r\n\n");
#endif // DEBUG

         return(1);
      }

   }

   if(iter>=4)
   {
#ifdef DEBUG
      PrintString("?Fatal error occurred while attempting to "
                  "format drive\r\n\n");
#endif // DEBUG
      if(i==0x80) return(0x80);     /* in case it was TIMEOUT error */
      else        return(2);
   }


   LoopDispatch();  /* call to 'LoopDispatch()' between format & verify */


          /* auto-retry 4 times for FLOPPY drives only */

   for(iter=0; !iter || (iter<4 && !(wDrive & 0x80)); iter++)
   {
      _fmemset((LPSTR)&rmc, 0, sizeof(rmc));

      rmc.SS  = wDosSeg;  /* DOS segment */
      rmc.SP  = wSP;                 /* top of block - 2 */

      rmc.DS  = rmc.SS;
      rmc.ES  = rmc.SS;               /* same segment as STACK */
      rmc.EBX = 0;                   /* offset to FORMAT array */
      rmc.EAX = 0x400 + (wSectors & 0xff);  /* verify format!! */
      rmc.ECX = ((wTrack << 8) & 0xff00) + ((wTrack>>2) & 0xc0) + 1;
      rmc.EDX = ((wHead << 8) & 0xff00) + (wDrive & 0xff);

      if(RealModeInt(0x13, &rmc))
      {
#ifdef DEBUG
         PrintString("?Unable to verify format on disk(ette)\r\n\n");
#endif // DEBUG
         return(1);
      }

      if(!(rmc.flags & CARRY_SET)) return(FALSE); /* it worked!! */

      i = (WORD)((rmc.EAX >> 8) & 0xff);  // get 'AL' on return for error type

      if(i==3)
      {
#ifdef DEBUG
         PrintString("?Disk is WRITE PROTECTED\r\n\n");
#endif // DEBUG
         if(iter>=3 || (wDrive & 0x80)) return(i);
      }
      else if(i==6)
      {
#ifdef DEBUG
         PrintString("?Disk removed/door open\r\n\n");
#endif // DEBUG
         if(iter>=3 || (wDrive & 0x80)) return(i);
      }

      /* otherwise, retry up to 4 times in case motor isn't up to speed */

      LoopDispatch();  /* allow other things to run during re-try */

      _fmemset((LPSTR)&rmc, 0, sizeof(rmc));
      rmc.SS = wDosSeg;
      rmc.SP = wSP;             /* should be top of block - 2 */

      rmc.EAX = 0x0;            /* RESET FLOPPY DISK SYSTEM */
      rmc.EDX = wDrive & 0xff;  /* drive # only */

      rmc.DS  = rmc.SS;
      rmc.ES  = rmc.SS;

      if(RealModeInt(0x13, &rmc))
      {
#ifdef DEBUG
         PrintString("?Fatal error attempting to reset drive\r\n\n");
#endif // DEBUG
         return(1);
      }

      LoopDispatch();  /* allow other things to run during re-try */

                     /** 'RE'SET MEDIA TYPE FOR FORMAT **/

      _fmemset((LPSTR)&rmc, 0, sizeof(rmc));
      rmc.SS = wDosSeg;
      rmc.SP = wSP;                             /* top of block - 2 */
      rmc.EAX = 0x1800;                       /* set media type for FORMAT */

                        /* # of TRACKS is ZERO-BASED - indicate MAX value! */
      rmc.ECX = (((wTotalTracks - 1)<< 8) & 0xff00) + (((wTotalTracks - 1)>>2) & 0xc0)
                + (wSectors & 0xbf);

      rmc.EDX = wDrive & 0xff;             /* drive ID */

      rmc.DS = rmc.ES = rmc.SS;

      if(RealModeInt(0x13, &rmc))
      {
         return(1);
      }

   }

#ifdef DEBUG
   PrintString("?Fatal error occurred while attempting to format drive\r\n\n");
#endif // DEBUG

   return(2);
}



 /* wDrive: 'A'==0, 'B'==1, etc.                                           */
 /* wSP:  the stack pointer value on entry to the interrupt                */
 /* wDosSeg: the DOS segment containing the data at offset ZERO            */
 /*          (and the stack area following the data)                       */
 /* 1st 'absolute' sector on the drive is 1!                               */
 /* RETURNS:  1 if general failure, 2 if too many retries, 3 if write prot,*/
 /*           6 or 0x80 if disk drive door is open                         */

BOOL FAR PASCAL WriteAbsoluteSector(WORD wDrive, WORD wAbsSector,
                                    WORD wHeads, WORD wSectors,
                                    WORD wDosSeg, WORD wSP)
{
WORD wHead, wTrack, wSector;

   wTrack  = (wAbsSector - 1) / (wHeads * wSectors);
   wSector = ((wAbsSector - 1) % wSectors) + 1;
   wHead   = ((wAbsSector - 1) % (wHeads * wSectors)) / wSectors;

   return(WriteHeadTrackSector(wDrive, wHead, wTrack, wSector, wDosSeg, wSP));
}



 /* wDrive: 'A'==0, 'B'==1, etc.                                           */
 /* wSP:  the stack pointer value on entry to the interrupt                */
 /* wDosSeg: the DOS segment containing the data at offset ZERO            */
 /*          (and the stack area following the data)                       */
 /* RETURNS:  1 if general failure, 2 if too many retries, 3 if write prot,*/
 /*           6 or 0x80 if disk drive door is open                         */

BOOL FAR PASCAL WriteHeadTrackSector(WORD wDrive, WORD wHead,
                                     WORD wTrack, WORD wSector,
                                     WORD wDosSeg, WORD wSP)
{
WORD i, iter;
REALMODECALL rmc;



          /* auto-retry 4 times for FLOPPY drives only */

   for(iter=0; !iter || (iter<4 && !(wDrive & 0x80)); iter++)
   {
      _fmemset((LPSTR)&rmc, 0, sizeof(rmc));
      rmc.SS = wDosSeg;
      rmc.SP = wSP;             /* should be top of block - 2 */

      rmc.EAX = 0x301;          /* write 1 sector */

      rmc.ECX = ((wTrack << 8) & 0xff00) + ((wTrack>>2) & 0xc0)
                + (wSector & 0xbf);
      rmc.EDX = ((wHead << 8) & 0xff00) + (wDrive & 0xff);

      rmc.DS  = rmc.SS;
      rmc.ES  = rmc.SS;
      rmc.EBX = 0;              /* address of BOOT SECTOR information */


      if(RealModeInt(0x13, &rmc))
      {
#ifdef DEBUG
         PrintString("?Fatal error attempting to write to drive\r\n\n");
#endif // DEBUG
         return(1);
      }

      if(!(rmc.flags & CARRY_SET))
      {
         return(FALSE);                        /* result is GOOD! */
      }

      i = (WORD)((rmc.EAX >> 8) & 0xff);  // get 'AL' on return for error type

      if(i==3)
      {
#ifdef DEBUG
         PrintString("?Disk is WRITE PROTECTED\r\n\n");
#endif // DEBUG
         if(iter>=3 || (wDrive & 0x80)) return(i);
      }
      else if(i==6)
      {
#ifdef DEBUG
         PrintString("?Disk removed/door open while writing to BIOS drive #");
         PrintAChar(wDrive + '0');
         PrintString("\r\n\n");
#endif // DEBUG
         if(iter>=3 || (wDrive & 0x80)) return(i);
      }
      else if(i==0x80)  /* disk timed out - possibly door is open */
      {
#ifdef DEBUG
         PrintString("?Diskette drive time-out\r\n\n");
#endif // DEBUG
         if(iter>=3 || (wDrive & 0x80)) return(i);
      }

      /* otherwise, retry up to 4 times in case motor isn't up to speed */

      LoopDispatch();  /* allow other things to run during re-try */

      _fmemset((LPSTR)&rmc, 0, sizeof(rmc));
      rmc.SS = wDosSeg;
      rmc.SP = wSP;             /* should be top of block - 2 */

      rmc.EAX = 0x0;            /* RESET FLOPPY DISK SYSTEM */
      rmc.EDX = wDrive & 0xff;  /* drive # only */

      rmc.DS  = rmc.SS;
      rmc.ES  = rmc.SS;

      if(RealModeInt(0x13, &rmc))
      {
#ifdef DEBUG
         PrintString("?Fatal error attempting to reset drive\r\n\n");
#endif // DEBUG
         return(1);
      }

      LoopDispatch();  /* allow other things to run during re-try */

   }

   if(i==0x80) return(0x80);
   else        return(2);

}



BOOL FAR PASCAL ReadHeadTrackSector(WORD wDrive, WORD wHead,
                                    WORD wTrack, WORD wSector,
                                    WORD wDosSeg, WORD wSP)
{
WORD i, iter;
REALMODECALL rmc;



          /* auto-retry 4 times for FLOPPY drives only */

   for(iter=0; !iter || (iter<4 && !(wDrive & 0x80)); iter++)
   {
      _fmemset((LPSTR)&rmc, 0, sizeof(rmc));
      rmc.SS = wDosSeg;
      rmc.SP = wSP;             /* should be top of block - 2 */

      rmc.EAX = 0x201;          /* read 1 sector */

      rmc.ECX = ((wTrack << 8) & 0xff00) + ((wTrack>>2) & 0xc0)
                + (wSector & 0xbf);
      rmc.EDX = ((wHead << 8) & 0xff00) + (wDrive & 0xff);

      rmc.DS  = rmc.SS;
      rmc.ES  = rmc.SS;
      rmc.EBX = 0;              /* address of BOOT SECTOR information */


      if(RealModeInt(0x13, &rmc))
      {
         return(1);
      }

      if(!(rmc.flags & CARRY_SET))
      {
         return(FALSE);                        /* result is GOOD! */
      }

      i = (WORD)((rmc.EAX >> 8) & 0xff);  // get 'AL' on return for error type

      if(i==3)
      {
         if(iter>=3 || (wDrive & 0x80)) return(i);
      }
      else if(i==6)
      {
         if(iter>=3 || (wDrive & 0x80)) return(i);
      }
      else if(i==0x80)  /* disk timed out - possibly door is open */
      {
#ifdef DEBUG
         PrintString("?Diskette drive time-out\r\n\n");
#endif // DEBUG
         if(iter>=3 || (wDrive & 0x80)) return(0x80);
      }

      /* otherwise, retry up to 4 times in case motor isn't up to speed */

      LoopDispatch();  /* allow other things to run during re-try */

      _fmemset((LPSTR)&rmc, 0, sizeof(rmc));
      rmc.SS = wDosSeg;
      rmc.SP = wSP;             /* should be top of block - 2 */

      rmc.EAX = 0x0;            /* RESET FLOPPY DISK SYSTEM */
      rmc.EDX = wDrive & 0xff;  /* drive # only */

      rmc.DS  = rmc.SS;
      rmc.ES  = rmc.SS;

      if(RealModeInt(0x13, &rmc))
      {
         return(1);
      }

      LoopDispatch();  /* allow other things to run during re-try */

   }


   if(i==0x80) return(0x80);
   else        return(2);

}





   /** this function gets the format info from the boot sector **/
   /** and generates a new FAT and root directory.             **/


BOOL FAR PASCAL QuickFormatDrive(WORD wDrive)
{
DWORD dwDosBlock;
LPSTR lp1;
WORD i, wMediaByte, wTracks, wHeads, wSectors, wSP, wFatSize, wRootSize;
LPBOOT_SECTOR lpBoot;


   /** Before I do anything, I *MUST* reset DOS's DISK SYSTEM! **/

   _asm
   {
      mov ax, 0xd00
      int 0x21
   }


   if(wDrive & 0x80) return(TRUE); /* cannot format a HARD DRIVE! */

   wSP = 1022;  /* size of 1 sector plus 510 bytes */
   dwDosBlock = GlobalDosAlloc(wSP + 2);

   if(!dwDosBlock) return(TRUE);  /* whoops! */

            /* it is assumed that wDrive is a VALID drive # */



   /** STEP 1:  read the BOOT SECTOR from the appropriate drive **/

   lpBoot = (LPBOOT_SECTOR)MAKELP(LOWORD(dwDosBlock), 0);


   if(i = ReadHeadTrackSector(wDrive, 0, 0, 1, HIWORD(dwDosBlock), wSP))
   {
      GlobalDosFree(LOWORD(dwDosBlock));
      return(i);
   }


   wRootSize  = lpBoot->RootDirSize;
//   wMediaByte = lpBoot->MediaByte;
   wFatSize   = lpBoot->SectorsPerFat;
   wSectors   = lpBoot->SectorsPerTrack;
   wHeads     = lpBoot->NumHeads;

   if(wHeads && wSectors)
   {
      wTracks    = lpBoot->TotalSectors / wHeads / wSectors;
   }
   else   // error in BOOT SECTOR - cannot 'quick' format
   {
      GlobalDosFree(LOWORD(dwDosBlock));
      return(TRUE);                    /* format error - can't write... */
   }


   // NOTE:  the media byte may be incorrect.  Calculate the CORRECT
   //        media byte based on tracks & sectors...

   if(wTracks <=40)        // assume 360k if 40 tracks
   {
      wMediaByte = 0xfd;   // 360k
   }
   else if(wSectors <=9)
   {
      wMediaByte = 0xf9;   // 720k
   }
   else if(wSectors <=15)
   {
      wMediaByte = 0xf9;   // 1.2 Mb
   }
   else
   {
      wMediaByte = 0xf0;   // 1.44 and above
   }


   if(lpBoot->MediaByte != wMediaByte)
   {
      MessageBox(hMainWnd,
                 "?Warning - drive type does not match BOOT sector media byte",
                 "** FORMAT WARNING **", MB_OK | MB_ICONHAND);

      lpBoot->MediaByte = (BYTE)wMediaByte;

      if(i = WriteAbsoluteSector(wDrive, 1, wHeads, wSectors,
                                 HIWORD(dwDosBlock), wSP))
      {
         GlobalDosFree(LOWORD(dwDosBlock));
         return(i);
      }
   }

   /* OK - next is the FAT - 2 copies, in which 1st sector has BOOT info */
   /* and the rest are full of 0's.                                      */


   _fmemset((LPSTR)lpBoot, 0, BYTES_PER_SECTOR);

   lp1 = (LPSTR)lpBoot;  // make sure I do it in this order to prevent alias
                         // problems...

   lp1[0] = (BYTE)wMediaByte;
   lp1[1] = 0xffU;
   lp1[2] = 0xffU;

   /* write the first FAT entry */

   if(WriteAbsoluteSector(wDrive, 2, wHeads, wSectors,
                          HIWORD(dwDosBlock), wSP))
   {
      GlobalDosFree(LOWORD(dwDosBlock));
      return(TRUE);                    /* format error - can't write... */
   }

   LoopDispatch();

   /* write the other '1st' FAT entry */

   if(WriteAbsoluteSector(wDrive, 2 + wFatSize, wHeads, wSectors,
                          HIWORD(dwDosBlock), wSP))
   {
      GlobalDosFree(LOWORD(dwDosBlock));
      return(TRUE);                    /* format error - can't write... */
   }


   /** next, write all of the remaining FAT tables **/

   *lp1 = 0;
   lp1[1] = 0;
   lp1[2] = 0;  /* remaining FAT records are all 0's */

   for(i=1; i<wFatSize; i++)
   {
      LoopDispatch();

      if(WriteAbsoluteSector(wDrive, 2 + i, wHeads, wSectors,
                             HIWORD(dwDosBlock), wSP) ||
         WriteAbsoluteSector(wDrive, 2 + i + wFatSize, wHeads, wSectors,
                             HIWORD(dwDosBlock), wSP))
      {
         GlobalDosFree(LOWORD(dwDosBlock));
         return(TRUE);                    /* format error - can't write... */
      }
   }

   /** NOW for some REAL fun!  The root dir *may* have enough entries to **/
   /** sneak onto the other side.  If so, I gotta know!                  **/

   for(i=2 + 2*wFatSize;
       i < (2 + 2*wFatSize + (wRootSize * 32)/BYTES_PER_SECTOR); i++)
   {
      LoopDispatch();

      if(WriteAbsoluteSector(wDrive, i, wHeads, wSectors,
                             HIWORD(dwDosBlock), wSP))
      {
         GlobalDosFree(LOWORD(dwDosBlock));
         return(TRUE);                    /* format error - can't write... */
      }
   }

   /* if it got here, I am done and it worked.  Only thing left to do is */
   /* write a LABEL (can be done using DOS) and transfer system on '/S'  */

   GlobalDosFree(LOWORD(dwDosBlock));

   /** Now that I'm done, make sure disk system is flushed **/

   _asm
   {
      mov ax, 0xd00
      int 0x21
   }


   return(FALSE);

}
