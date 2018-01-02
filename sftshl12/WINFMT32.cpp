/***************************************************************************/
/*                                                                         */
/*            WINFMT32.C - Win '95 program to format diskettes             */
/* Copyright (c) 1995 by Stewart~Frazier Tools, Inc.- all rights reserved  */
/*                                                                         */
/*              WIN32 equivalent of 'winfrmat.c' for SFTSHELL              */
/*                  NOTE:  Win '95 *ONLY* - not under NT!                  */
/*                                                                         */
/***************************************************************************/


#define THE_WORKS
#include "mywin.h"

#include "mth.h"
#include "wincmd.h"
#include "wincmd_f.h"



#ifndef VWIN32_DIOC_GETVERSION

#define VWIN32_DIOC_GETVERSION DIOC_GETVERSION
#define	VWIN32_DIOC_DOS_IOCTL	1
#define	VWIN32_DIOC_DOS_INT25	2
#define	VWIN32_DIOC_DOS_INT26	3
#define	VWIN32_DIOC_DOS_INT13	4
#define	VWIN32_DIOC_CLOSEHANDLE DIOC_CLOSEHANDLE

typedef struct DIOCRegs {
	DWORD	reg_EBX;
	DWORD	reg_EDX;
	DWORD	reg_ECX;
	DWORD	reg_EAX;
	DWORD	reg_EDI;
	DWORD	reg_ESI;
	DWORD	reg_Flags;		
} DIOC_REGISTERS;

#endif // VWIN32_DIOC_GETVERSION



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


BOOL WINAPI FormatDrive(UINT wDrive, UINT wTracks, UINT wHeads, UINT wSectors);

BOOL WINAPI FormatDrive0(HANDLE hVWIN32, UINT wDrive, UINT wTracks,
                         UINT wHeads, UINT wSectors);


BOOL WINAPI SetMediaTypeForFormat(HANDLE hVWIN32, UINT wDrive, UINT wHeads,
                                  UINT wTracks, UINT wSectors,
                                  UINT wMediaByte, UINT wFatSize,
                                  UINT wRootSize, UINT wDriveType,
                                  LPSTR FAR *lpdwMDT);

BOOL WINAPI BIOSSetMediaTypeForFormat(HANDLE hVWIN32, UINT wDrive, UINT wHeads,
                                      UINT wTracks, UINT wSectors,
                                      UINT wMediaByte, UINT wFatSize,
                                      UINT wRootSize, UINT wDriveType,
                                      LPSTR FAR *lpdwMDT);

BOOL WINAPI DOSSetMediaTypeForFormat(HANDLE hVWIN32, UINT wDrive, UINT wHeads,
                                     UINT wTracks, UINT wSectors,
                                     UINT wMediaByte, UINT wFatSize,
                                     UINT wRootSize, UINT wDriveType,
                                     LPSTR FAR *lpdwMDT);

BOOL WINAPI FormatHeadTrackSector(HANDLE hVWIN32, UINT wDrive, UINT wHead,
                                  UINT wTrack, UINT wTotalTracks,
                                  UINT wSectors, LPFORMAT lpF,
                                  LPSTR lpSrcMDT, LPSTR lpSysMDT);


BOOL WINAPI WriteHeadTrackSector(HANDLE hVWIN32, UINT wDrive, UINT wHead,
                                 UINT wTrack, UINT wSector, LPCSTR lpBuf);

BOOL WINAPI WriteAbsoluteSector(HANDLE hVWIN32, UINT wDrive, UINT wAbsSector,
                                UINT wHeads, UINT wSectors, LPCSTR lpBuf);

BOOL WINAPI ReadHeadTrackSector(HANDLE hVWIN32, UINT wDrive, UINT wHead,
                                UINT wTrack, UINT wSector, LPSTR lpBuf);






                      /*** GLOBAL VARIABLES ***/





UINT wFormatCurDrive=0xffffffff, wFormatCurHead=0, wFormatCurTrack=0;
WORD wFormatPercentComplete=0;


                         /*** "THE CODE" ***/

// 'wDrive' is zero for 'A', 1 for 'B', and so on

BOOL WINAPI FormatDrive(UINT wDrive, UINT wTracks, UINT wHeads, UINT wSectors)
{
HANDLE hVWIN32;
BOOL rval;
UINT uiDriveType;
char szDriveBuf[4];


   lstrcpy(szDriveBuf, "A:\\");
   szDriveBuf[0] = 'A' + wDrive;

   uiDriveType = GetDriveType(szDriveBuf);

   if(uiDriveType != 0 && uiDriveType != 1 &&
      uiDriveType != DRIVE_REMOVABLE &&
      uiDriveType != DRIVE_RAMDISK)
   {
      return(TRUE); /* ABSOLUTELY WILL NOT FORMAT Hard Disks */
   }


   hVWIN32 = CreateFile("\\\\.\\vwin32", 0, 0, NULL,
                        OPEN_EXISTING, // CREATE_NEW, //OPEN_ALWAYS
                        FILE_FLAG_DELETE_ON_CLOSE,
                        0);


   if(hVWIN32 == INVALID_HANDLE_VALUE) return(TRUE); // ERROR!

   rval = FormatDrive0(hVWIN32, wDrive, wTracks, wHeads, wSectors);


   CloseHandle(hVWIN32);

   return(rval);
}




BOOL WINAPI PerformGenericIOCTL(HANDLE hVWIN32, UINT wDrive,
                                UINT wFunction, LPVOID lpParmBlock)
{
BOOL rval = FALSE;
DIOC_REGISTERS regs;
DWORD cb;


   regs.reg_Flags = 0;  // this also clears the carry bit, by the way
   regs.reg_EAX = 0x440d;  // IOCTL
   regs.reg_EBX = wDrive & 0xff;
   regs.reg_ECX = (wFunction & 0xff) | 0x0800;
   regs.reg_EDX = (DWORD)lpParmBlock;

   rval = !DeviceIoControl(hVWIN32, VWIN32_DIOC_DOS_IOCTL,
                           &regs, sizeof(regs), &regs, sizeof(regs),
                           &cb, NULL)
          || (regs.reg_Flags & CARRY_SET);

   return(rval);
}


void FlushDiskSystem(HANDLE hVWIN32, UINT wDrive)
{
DIOC_REGISTERS regs;
DWORD cb;

   regs.reg_EAX = 0x710d;
   regs.reg_ECX = 1;      // flush flag:  '1' means flush ALL including cache
   regs.reg_EDX = (wDrive + 1) & 0xff;

   DeviceIoControl(hVWIN32, VWIN32_DIOC_DOS_IOCTL,
                   &regs, sizeof(regs), &regs, sizeof(regs), &cb, NULL);
}


BOOL WINAPI _GetDriveParms(HANDLE hVWIN32, UINT wDrive, UINT FAR *lpwDriveType,
                           UINT FAR *lpwMaxCyl, UINT FAR *lpwMaxSector,
                           UINT FAR *lpwHeads, UINT FAR *lpwNumDrives);

BOOL WINAPI GetDriveParms(UINT wDrive, UINT FAR *lpwDriveType,
                          UINT FAR *lpwMaxCyl, UINT FAR *lpwMaxSector,
                          UINT FAR *lpwHeads, UINT FAR *lpwNumDrives)
{
BOOL rval;
HANDLE hVWIN32;

   hVWIN32 = CreateFile("\\\\.\\vwin32", 0, 0, NULL,
                        OPEN_EXISTING, // CREATE_NEW, //OPEN_ALWAYS
                        FILE_FLAG_DELETE_ON_CLOSE,
                        0);

   if(hVWIN32 == INVALID_HANDLE_VALUE) return(TRUE); // ERROR!

   rval = _GetDriveParms(hVWIN32, wDrive, lpwDriveType,
                         lpwMaxCyl, lpwMaxSector, lpwHeads, lpwNumDrives);
}

BOOL WINAPI _GetDriveParms(HANDLE hVWIN32, UINT wDrive, UINT FAR *lpwDriveType,
                           UINT FAR *lpwMaxCyl, UINT FAR *lpwMaxSector,
                           UINT FAR *lpwHeads, UINT FAR *lpwNumDrives)
{
DIOC_REGISTERS regs;
DWORD cb;


   regs.reg_EAX = 0x800;
   regs.reg_EDX = wDrive;

   if(!DeviceIoControl(hVWIN32, VWIN32_DIOC_DOS_INT13,
                       &regs, sizeof(regs), &regs, sizeof(regs),
                       &cb, NULL)
      || (regs.reg_Flags & CARRY_SET))
   {
      return(TRUE);
   }

   if(lpwDriveType) *lpwDriveType = (WORD)(regs.reg_EBX & 0xff);
   if(lpwMaxCyl)    *lpwMaxCyl    = (WORD)((((regs.reg_ECX) >> 8) & 0xff) +
                                           ((((WORD)regs.reg_ECX) & 0xc0)<<2));
   if(lpwMaxSector) *lpwMaxSector = (WORD)(regs.reg_ECX & 0x3f);
   if(lpwHeads)     *lpwHeads     = (((WORD)regs.reg_EDX) >> 8) & 0xff;
   if(lpwNumDrives) *lpwNumDrives = ((WORD)regs.reg_EDX) & 0xff;



   return(FALSE);

}


#pragma pack(1)

typedef struct {
  BYTE Operation;
  BYTE NumLocks;
  } PARAMBLOCK, FAR *LPPARAMBLOCK;

#pragma pack()

static UINT DriveAccessFlag[26];



#define IOCTL_LOCK_EXCLUSIVE 0  /* a 'level 0' lock */


BOOL PASCAL LockDrive(HANDLE hVWIN32, UINT wDrive)
{
DIOC_REGISTERS regs;
BOOL rval = FALSE;
DWORD cb;


   wDrive++;    // 0 was 'A', 1 was 'B' - now 1 is 'A', 2 is 'B', etc.


   regs.reg_EAX = 0x440d;

   regs.reg_EBX = (wDrive & 0xff) + (IOCTL_LOCK_EXCLUSIVE * 0x100);
   regs.reg_ECX = 0x84a;  // ch is '8' for DISK DRIVE; '4a' is function #

   regs.reg_EDX = 0;      // access flags (not used for level 0)

   if(!DeviceIoControl(hVWIN32, VWIN32_DIOC_DOS_IOCTL,
                       &regs, sizeof(regs), &regs, sizeof(regs),
                       &cb, NULL))
   {
      rval = 0xffffffff;
   }
   else if(regs.reg_Flags & CARRY_SET)
   {
      rval = (UINT)regs.reg_EAX;
   }

   return(rval);
}

void PASCAL UnlockDrive(HANDLE hVWIN32, UINT wDrive)
{
DIOC_REGISTERS regs;
DWORD cb;


   wDrive++;    // 0 is 'A', 1 is 'B'

   regs.reg_EAX = 0x440d;

   regs.reg_EBX = wDrive & 0xff;
   regs.reg_ECX = 0x86a;  // ch is '8' for DISK DRIVE; '6a' is function #

   DeviceIoControl(hVWIN32, VWIN32_DIOC_DOS_IOCTL,
                   &regs, sizeof(regs), &regs, sizeof(regs),
                   &cb, NULL);
}


BOOL PASCAL LockDrive2(HANDLE hVWIN32, UINT wDrive)
{
DIOC_REGISTERS regs;
BOOL rval;
DWORD cb;


   rval = LockDrive(hVWIN32, wDrive);
   if(rval) return(rval);


   wDrive++;    // 0 was 'A', 1 was 'B' - now 1 is 'A', 2 is 'B', etc.

   regs.reg_EAX = 0x440d;

   regs.reg_EBX = (wDrive & 0xff) + (IOCTL_LOCK_EXCLUSIVE * 0x100);
   regs.reg_ECX = 0x84a;  // ch is '8' for DISK DRIVE; '4a' is function #

   regs.reg_EDX = 4;      // access flags (special for 2nd level 0 lock)

   if(!DeviceIoControl(hVWIN32, VWIN32_DIOC_DOS_IOCTL,
                       &regs, sizeof(regs), &regs, sizeof(regs),
                       &cb, NULL))
   {
      rval = 0xffffffff;
   }
   else if(regs.reg_Flags & CARRY_SET)
   {
      rval = (UINT)regs.reg_EAX;
   }


   if(rval) UnlockDrive(hVWIN32, wDrive);

   return(rval);
}

void PASCAL UnlockDrive2(HANDLE hVWIN32, UINT wDrive)
{
   UnlockDrive(hVWIN32, wDrive);
   UnlockDrive(hVWIN32, wDrive);
}




BOOL WINAPI FormatDrive0(HANDLE hVWIN32, UINT wDrive, UINT wTracks,
                         UINT wHeads, UINT wSectors)
{
HGLOBAL hBoot;
LPBOOT_SECTOR lpBoot;
LPSTR lp1, lpTemp;
HRSRC hRes;
DWORD cb;
DIOC_REGISTERS regs;
UINT wTrack, wHead, i, w;
LPFORMAT lpF;
UINT wDriveType, wMaxCyl, wMaxSector, wMaxHeads, wNumDrives,
     wRootSize, wFatSize, wMediaByte;
BYTE SrcMDT[11];  /* contains Media Descriptor Table returned by int 13:18 */




   /** Before I do anything, I *MUST* reset DOS's DISK SYSTEM! **/

   FlushDiskSystem(hVWIN32, wDrive);



   /** STEP 1:  Get drive parms for 'wDrive' **/

   if(_GetDriveParms(hVWIN32, wDrive, &wDriveType, &wMaxCyl, &wMaxSector,
                    &wMaxHeads, &wNumDrives))
   {
      CloseHandle(hVWIN32);
      return(TRUE);  /* error trying to get Drive Parms */
   }


   if(!wTracks)   wTracks  = wMaxCyl + 1;    /* 0 based */
   if(!wSectors)  wSectors = wMaxSector;     /* 1 based */
   if(!wHeads)    wHeads   = wMaxHeads + 1;  /* 0 based */

   printf("Formatting Drive %c: %d Tracks, %d Sectors, %d Heads \n\n",
          (int)('A' + wDrive), wTracks, wSectors, wHeads);



           /* see if desrmced track/head/sector is invalid */

   if((wMaxCyl + 1)<wTracks || wMaxSector<wSectors || (wMaxHeads + 1)<wHeads)
   {
      return(TRUE);  /** cannot format that configuration, eh? **/
   }


           /* create a DOS memory block for stack & Xfer area */

   lpF = (LPFORMAT)GlobalAlloc(GPTR, max(wSectors * sizeof(FORMAT),
                                         BYTES_PER_SECTOR) + 128);



   // CHICAGO 'fix' - LOCK DRIVE BEFORE CONTINUING!

   if(LockDrive2(hVWIN32, wDrive))
   {
      printf("\nUnable to LOCK the diskette for FORMAT!\n");

      GlobalFree((HGLOBAL)lpF);
      return(TRUE);
   }


                  /** RESET DISK SYSTEM FIRST via BIOS! **/

   regs.reg_EAX = 0;     /* reset disk system */
   regs.reg_EDX = wDrive & 0xff;  /* drive ID */

   if(!DeviceIoControl(hVWIN32, VWIN32_DIOC_DOS_INT13,
                       &regs, sizeof(regs), &regs, sizeof(regs),
                       &cb, NULL))
   {
      UnlockDrive2(hVWIN32, wDrive);

      GlobalFree((HGLOBAL)lpF);
      return(TRUE);
   }

   /** Now for the fun part:  set up to format 360k, 1.2M, 720k, or 1.44M **/


   switch(wDriveType)
   {
      case 1:         /* 360k drive - only support this format! */
         wRootSize  = 112;
         wFatSize   = 2;
         wMediaByte = 0xfd;
         break;

      case 3:         /* 720k drive - only support this format! */
         wRootSize  = 112;
         wFatSize   = 3;
         wMediaByte = 0xf9;

         break;

      case 2:         /* 1.2Mb drive */
         if(wTracks<=40 && wSectors<=9)  /* format 360k */
         {
            wRootSize  = 112;
            wFatSize   = 2;
            wMediaByte = 0xfd;
         }
         else
         {
            wRootSize  = 224;
            wFatSize   = 7;
            wMediaByte = 0xf9;
         }

         break;

      case 4:         /* 1.44Mb drive */
         if(wSectors<=9)  /* format 720k */
         {
            wRootSize  = 112;
            wFatSize   = 3;
            wMediaByte = 0xf9;
         }
         else
         {
            wRootSize  = 224;
            wFatSize   = 9;
            wMediaByte = 0xf0;
         }

         break;


      case 5:         /* 2.88Mb drive? */

         if(wSectors<=9 && wTracks<=80)  /* format 720k */
         {
            wRootSize  = 112;
            wFatSize   = 3;
            wMediaByte = 0xf9;
         }
         else if(wSectors<=18 && wTracks<=80)
         {
            wRootSize  = 224;
            wFatSize   = 9;
            wMediaByte = 0xf0;
         }
         else
         {
            wRootSize  = 448;
            wFatSize   = 18;
            wMediaByte = 0xf0;
         }

         break;

   }


                    /** SET MEDIA TYPE FOR FORMAT **/

   lpTemp = NULL;  // linear address for 'MDT'

   if(SetMediaTypeForFormat(hVWIN32, wDrive, wHeads, wTracks, wSectors,
                            wMediaByte, wFatSize, wRootSize,
                            wDriveType, &lpTemp))
   {
      UnlockDrive2(hVWIN32, wDrive);
      return(TRUE);
   }




   /* at this point 'lpTemp' points to the BIOS's MDT for the format parms */
   /* get a copy of it and place the copy into SrcMDT[] (local var).       */

   if(lpTemp)
   {
      memcpy(SrcMDT, lpTemp, 11);  /* copy it! */
   }

   printf("\nBegin format...\n\n");

   wFormatCurDrive=wDrive;

   for(wTrack=0; wTrack<wTracks; wTrack++)
   {
      wFormatCurTrack = wTrack;

      for(wHead=0; wHead<wHeads; wHead++)
      {
         wFormatCurHead = wHead;


         wFormatPercentComplete = (UINT)((100L * wHeads * wTrack + wHead)
                                          / (wHeads * wTracks));

         printf("\r%2d%% Complete     \r", wFormatPercentComplete);


         w = FormatHeadTrackSector(hVWIN32, wDrive, wHead, wTrack, wTracks,
                                   wSectors, lpF, SrcMDT, lpTemp);

         if(w && w!=3 && w!=6 && w!=0x80)
         {
            w = FormatHeadTrackSector(hVWIN32, wDrive, wHead, wTrack, wTracks,
                                      wSectors, lpF, SrcMDT, lpTemp);
         }

         if(w && w!=3 && w!=6 && w!=0x80)
         {
            w = FormatHeadTrackSector(hVWIN32, wDrive, wHead, wTrack, wTracks,
                                      wSectors, lpF, SrcMDT, lpTemp);
         }

         if(w)
         {
            UnlockDrive2(hVWIN32, wDrive);

            if(w==3)
            {
               printf("\n?Diskette is write protected\n");
            }
            else if(w==6)
            {
               printf("\n?Diskette removed from drive/door is open\n");
            }
            else if(w==0x80)  /* disk timed out - possibly door is open */
            {
               printf("\n?Time-out error (drive door may be open)\n");
            }
            else
            {
               printf("\nUnable to format diskette!\n");
            }

            GlobalFree((HGLOBAL)lpF);

            return(w);

         }

         if(wHead == 0 && wTrack == 0)
         {
            // BUG FIX/WORKAROUND:  write boot sector NOW

            hRes = FindResource(hInst, "BOOT_SECTOR", "BOOTSECTOR");
            if(hRes)
            {
               hBoot = LoadResource(hInst, hRes);

               if(hBoot)
               {
                  lpBoot = (LPBOOT_SECTOR) LockResource(hBoot);

                  memcpy((LPSTR)lpF, (LPSTR)lpBoot, BYTES_PER_SECTOR);

                  UnlockResource(hBoot);
                  FreeResource(hBoot);
               }
               else
               {
                  UnlockDrive2(hVWIN32, wDrive);
                  FreeResource(hBoot);

                  printf("\nUnable to get resource for BOOT RECORD\n");
                  return(TRUE);
               }

            }
            else
            {
               UnlockDrive2(hVWIN32, wDrive);

               printf("\nUnable to get resource for BOOT RECORD\n");

               return(TRUE);
            }


            lpBoot = (LPBOOT_SECTOR)lpF;

            lpBoot->RootDirSize     = wRootSize;
            lpBoot->TotalSectors    = wHeads * wTracks * wSectors;
            lpBoot->MediaByte       = (BYTE)wMediaByte;
            lpBoot->SectorsPerFat   = wFatSize;
            lpBoot->SectorsPerTrack = wSectors;
            lpBoot->NumHeads        = wHeads;
            lpBoot->Dos4Sectors     = 0;       /* ignore this one */
            lpBoot->SerialNumber    = GetCurrentTime();  /* the serial # */


            /** now, write the boot sector **/

            if(WriteAbsoluteSector(hVWIN32, wDrive, 1, wHeads, wSectors, (LPSTR)lpBoot))
            {
               UnlockDrive2(hVWIN32, wDrive);

               GlobalFree((HGLOBAL)lpF);
               return(TRUE);                    /* format error - can't write... */
            }
         }
      }
   }

   printf("\r%2d%% Complete     \r", 100);



   /* OK - next is the FAT - 2 copies, in which 1st sector has BOOT info */
   /* and the rest are full of 0's.                                      */

   printf("\n\nWring FAT and root directory\n");

   lp1 = (LPSTR)lpF;

   memset(lp1, 0, BYTES_PER_SECTOR);

   lp1[0] = (BYTE)wMediaByte;
   lp1[1] = 0xffU;
   lp1[2] = 0xffU;

   /* write the first FAT entry */

   if(WriteAbsoluteSector(hVWIN32, wDrive, 2, wHeads, wSectors, lp1))
   {
      UnlockDrive2(hVWIN32, wDrive);

      GlobalFree((HGLOBAL)lpF);
      return(TRUE);                    /* format error - can't write... */
   }



   /* write the other '1st' FAT entry */

   if(WriteAbsoluteSector(hVWIN32, wDrive, (UINT)(2 + wFatSize), wHeads, wSectors, lp1))
   {
      UnlockDrive2(hVWIN32, wDrive);

      GlobalFree((HGLOBAL)lpF);
      return(TRUE);                    /* format error - can't write... */
   }


   /** next, write all of the remaining FAT tables **/

   *lp1 = 0;
   lp1[1] = 0;
   lp1[2] = 0;  /* remaining FAT records are all 0's */

   for(i=1; i<wFatSize; i++)
   {
      LoopDispatch();

      if(WriteAbsoluteSector(hVWIN32, wDrive, (UINT)(2 + i), wHeads, wSectors, lp1) ||
         WriteAbsoluteSector(hVWIN32, wDrive, (UINT)(2 + i + wFatSize), wHeads, wSectors, lp1))
      {
         UnlockDrive2(hVWIN32, wDrive);

         GlobalFree((HGLOBAL)lpF);
         return(TRUE);                    /* format error - can't write... */
      }
   }

   /** NOW for some REAL fun!  The root dir *may* have enough entries to **/
   /** sneak onto the other side.  If so, I gotta know!                  **/

   for(i=2 + 2*wFatSize;
       i < (2 + 2*wFatSize + (wRootSize * 32)/BYTES_PER_SECTOR); i++)
   {
      LoopDispatch();

      if(WriteAbsoluteSector(hVWIN32, wDrive, i, wHeads, wSectors, lp1))
      {
         UnlockDrive2(hVWIN32, wDrive);

         GlobalFree((HGLOBAL)lpF);
         return(TRUE);                    /* format error - can't write... */
      }
   }


   // flush disk system now in an attempt to ensure that
   // buffers for this diskette are clear before unlocking

   FlushDiskSystem(hVWIN32, wDrive);


   /* if it got here, I am done and it worked.  Only thing left to do is */
   /* write a LABEL (can be done using DOS) and transfer system on '/S'  */

   UnlockDrive2(hVWIN32, wDrive);

   GlobalFree((HGLOBAL)lpF);
   lpF = NULL;


   /** Now that I'm done, make sure disk system is flushed, one more time **/

   FlushDiskSystem(hVWIN32, wDrive);


   return(FALSE);
}



BOOL WINAPI SetMediaTypeForFormat(HANDLE hVWIN32, UINT wDrive, UINT wHeads,
                                  UINT wTracks, UINT wSectors,
                                  UINT wMediaByte, UINT wFatSize,
                                  UINT wRootSize, UINT wDriveType,
                                  LPSTR FAR *lpdwMDT)
{
BOOL rval;


   // BUG FIX/WORKAROUND:  Use MS-DOS to set media type for format first,
   //                      and afterwards use BIOS.

   rval = DOSSetMediaTypeForFormat(hVWIN32, wDrive, wHeads, wTracks,
                                   wSectors, wMediaByte, wFatSize,
                                   wRootSize, wDriveType, lpdwMDT);



   rval = BIOSSetMediaTypeForFormat(hVWIN32, wDrive, wHeads, wTracks,
                                    wSectors, wMediaByte, wFatSize,
                                    wRootSize, wDriveType, lpdwMDT);

   // TEMPORARY
   if(rval) printf("\n?SetMediaTypeForFormat failed!\n");

   return(rval);
}


BOOL WINAPI BIOSSetMediaTypeForFormat(HANDLE hVWIN32, UINT wDrive, UINT wHeads,
                                      UINT wTracks, UINT wSectors,
                                      UINT wMediaByte, UINT wFatSize,
                                      UINT wRootSize, UINT wDriveType,
                                      LPSTR FAR *lpdwMDT)
{
DIOC_REGISTERS regs;
BOOL rval;
DWORD cb;



   // BUG FIX/WORKAROUND: perform BIOS DRIVE RESET first...

   regs.reg_EAX = 0x0;            /* RESET FLOPPY DISK SYSTEM */
   regs.reg_EDX = wDrive & 0xff;  /* drive # only */

   if(!DeviceIoControl(hVWIN32, VWIN32_DIOC_DOS_INT13,
                       &regs, sizeof(regs), &regs, sizeof(regs),
                       &cb, NULL))
   {
      return(1);
   }


   // BIOS:  set media type for format


   regs.reg_EAX = 0x1800;                          /* set media type for FORMAT */
                        /* # of TRACKS is ZERO-BASED - indicate MAX value! */
   regs.reg_ECX = (((wTracks - 1)<< 8) & 0xff00) + (((wTracks - 1)>>2) & 0xc0)
             + (wSectors & 0xbf);

   regs.reg_EDX = wDrive & 0xff;             /* drive ID */

   rval = !DeviceIoControl(hVWIN32, VWIN32_DIOC_DOS_INT13,
                           &regs, sizeof(regs), &regs, sizeof(regs),
                           &cb, NULL)
          || (regs.reg_Flags & CARRY_SET);

   // return the address of the 'MDT' (as appropriate)

   *lpdwMDT = (LPSTR)regs.reg_EDI;     // this is a wild stab in the dark...

   return(rval);
}


BOOL WINAPI DOSSetMediaTypeForFormat(HANDLE hVWIN32, UINT wDrive, UINT wHeads,
                                     UINT wTracks, UINT wSectors,
                                     UINT wMediaByte, UINT wFatSize,
                                     UINT wRootSize, UINT wDriveType,
                                     LPSTR FAR *lpdwMDT)
{
BOOL rval;
UINT i;
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
 } *lpIOCtlInfo40;
#pragma pack()




   *lpdwMDT = 0;  // this flags 'do not use MDT'

   wDrive++;  // need to add 1 to the drive letter (assume 'A' or 'B' only)

   lpIOCtlInfo40 = (struct _tag_IOCTL_40_ *)
          GlobalAlloc(GPTR, sizeof(*lpIOCtlInfo40) + sizeof(WORD) * wTracks);

   memset((LPSTR)lpIOCtlInfo40, 0, sizeof(*lpIOCtlInfo40));



   // Initially, get the drive media information via IOCTL sub-function 60H

   rval = PerformGenericIOCTL(hVWIN32, wDrive, 0x60, lpIOCtlInfo40);

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
      && wSectors <= 9)         // 360kb diskette format
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

      rval = PerformGenericIOCTL(hVWIN32, wDrive, 0x40, lpIOCtlInfo40);
      if(rval)
      {
         LoopDispatch();

         GlobalFree((HGLOBAL)lpIOCtlInfo40);
         return(rval);
      }


      // NOW, do it again, but this time I want to use the 'old BPB' to see
      // if I correct a rather nasty bug this way...


      // get drive parameters (one more time)

      rval = PerformGenericIOCTL(hVWIN32, wDrive, 0x60, lpIOCtlInfo40);
      if(rval)
      {
         LoopDispatch();

         GlobalFree((HGLOBAL)lpIOCtlInfo40);
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

   rval = PerformGenericIOCTL(hVWIN32, wDrive, 0x40, lpIOCtlInfo40);

   GlobalFree((HGLOBAL)lpIOCtlInfo40);

   return(rval);

}









 /* wDrive: 'A'==0, 'B'==1, etc.                                           */
 /* lpF:  points to 'FORMAT' structure whose 'DOS' segment is in 'wDosSeg' */
 /*       and whose offset into this 'DOS' segment == DOS offset.          */
 /* wSP:  the stack pointer value on entry to the interrupt                */
 /* RETURNS:  1 if general failure, 2 if too many retries, 3 if write prot,*/
 /*           6 or 0x80 if disk drive door is open                         */

BOOL WINAPI FormatHeadTrackSector(HANDLE hVWIN32, UINT wDrive, UINT wHead,
                                  UINT wTrack, UINT wTotalTracks,
                                  UINT wSectors, LPFORMAT lpF,
                                  LPSTR lpSrcMDT, LPSTR lpSysMDT)
{
UINT i, iter;
DIOC_REGISTERS regs;
DWORD cb;


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
      regs.reg_EAX = 0x500 + (wSectors & 0xff);  /* format!! */

      regs.reg_EBX = (DWORD)lpF;             /* offset to FORMAT array */

      regs.reg_ECX = ((wTrack << 8) & 0xff00) + ((wTrack>>2) & 0xc0) + 1;
      regs.reg_EDX = ((wHead << 8) & 0xff00) + (wDrive & 0xff);

      /* before I attempt to format drive, copy the source 'MDT' */
      /* to the destination 'MDT'.                               */

      memcpy(lpSysMDT, lpSrcMDT, 11); /* copy 'current format' data */

      lpSysMDT[4] = (BYTE)wSectors;  /* ensure it's set, no matter what! */


//      if(!WriteProcessMemory(GetCurrentProcess(), lpSysMDT, lpSrcMDT,
//                             11, &cb) || cb != 11)
//      {
//        printf("?Unable to 'force' format parameters\n");
//      }

      if(!DeviceIoControl(hVWIN32, VWIN32_DIOC_DOS_INT13,
                          &regs, sizeof(regs), &regs, sizeof(regs),
                          &cb, NULL))
      {
         return(TRUE);  // error...
      }


      if(!(regs.reg_Flags & CARRY_SET))
      {
         return(FALSE);       /* so far so good!! */
      }


      i = (UINT)((regs.reg_EAX >> 8) & 0xff);  // get 'AL' on return for error type

      if(i==3)
      {
//         printf("?Disk is WRITE PROTECTED\n\n");
         if(iter>=3 || (wDrive & 0x80)) return(i);
      }
      else if(i==6)
      {
//         printf("?Disk removed/door open\n\n");
         if(iter>=3 || (wDrive & 0x80)) return(i);
      }
      else if(i==0x80)  /* disk timed out - possibly door is open */
      {                 /* retry max # of iterations and then bail out */
//         printf("?Diskette drive time-out\n\n");
         if(iter>=3 || (wDrive & 0x80)) return(i);
      }

      /* otherwise, retry up to 4 times in case motor isn't up to speed */

      LoopDispatch();  /* allow other things to run during re-try */

      regs.reg_EAX = 0x0;            /* RESET FLOPPY DISK SYSTEM */
      regs.reg_EDX = wDrive & 0xff;  /* drive # only */

      if(!DeviceIoControl(hVWIN32, VWIN32_DIOC_DOS_INT13,
                          &regs, sizeof(regs), &regs, sizeof(regs),
                          &cb, NULL))
      {
//         printf("?Fatal error attempting to reset drive\n\n");
         return(1);
      }

      LoopDispatch();  /* allow other things to run during re-try */


                     /** 'RE'SET MEDIA TYPE FOR FORMAT **/

      regs.reg_EAX = 0x1800;                       /* set media type for FORMAT */

                        /* # of TRACKS is ZERO-BASED - indicate MAX value! */
      regs.reg_ECX = (((wTotalTracks - 1)<< 8) & 0xff00) + (((wTotalTracks - 1)>>2) & 0xc0)
                     + (wSectors & 0xbf);

      regs.reg_EDX = wDrive & 0xff;             /* drive ID */

      if(!DeviceIoControl(hVWIN32, VWIN32_DIOC_DOS_INT13,
                          &regs, sizeof(regs), &regs, sizeof(regs),
                          &cb, NULL))
      {
         return(1);
      }

   }

   if(iter>=4)
   {
//      printf("?Fatal error occurred while attempting to "
//                  "format drive\n\n");
      if(i==0x80) return(0x80);     /* in case it was TIMEOUT error */
      else        return(2);
   }


   LoopDispatch();  /* call to 'LoopDispatch()' between format & verify */


          /* auto-retry 4 times for FLOPPY drives only */

   for(iter=0; !iter || (iter<4 && !(wDrive & 0x80)); iter++)
   {
      regs.reg_EBX = 0;                   /* offset to FORMAT array */
      regs.reg_EAX = 0x400 + (wSectors & 0xff);  /* verify format!! */
      regs.reg_ECX = ((wTrack << 8) & 0xff00) + ((wTrack>>2) & 0xc0) + 1;
      regs.reg_EDX = ((wHead << 8) & 0xff00) + (wDrive & 0xff);

      if(!DeviceIoControl(hVWIN32, VWIN32_DIOC_DOS_INT13,
                          &regs, sizeof(regs), &regs, sizeof(regs),
                          &cb, NULL))
      {
//         printf("?Unable to verify format on disk(ette)\n\n");
         return(1);
      }

      if(!(regs.reg_Flags & CARRY_SET)) return(FALSE); /* it worked!! */

      i = (UINT)((regs.reg_EAX >> 8) & 0xff);  // get 'AL' on return for error type

      if(i==3)
      {
//         printf("?Disk is WRITE PROTECTED\n\n");
         if(iter>=3 || (wDrive & 0x80)) return(i);
      }
      else if(i==6)
      {
//         printf("?Disk removed/door open\n\n");
         if(iter>=3 || (wDrive & 0x80)) return(i);
      }

      /* otherwise, retry up to 4 times in case motor isn't up to speed */

      LoopDispatch();  /* allow other things to run during re-try */

      regs.reg_EAX = 0x0;            /* RESET FLOPPY DISK SYSTEM */
      regs.reg_EDX = wDrive & 0xff;  /* drive # only */

      if(!DeviceIoControl(hVWIN32, VWIN32_DIOC_DOS_INT13,
                          &regs, sizeof(regs), &regs, sizeof(regs),
                          &cb, NULL))
      {
//         printf("?Fatal error attempting to reset drive\n\n");
         return(1);
      }

      LoopDispatch();  /* allow other things to run during re-try */


                     /** 'RE'SET MEDIA TYPE FOR FORMAT **/

      regs.reg_EAX = 0x1800;                       /* set media type for FORMAT */

                        /* # of TRACKS is ZERO-BASED - indicate MAX value! */
      regs.reg_ECX = (((wTotalTracks - 1)<< 8) & 0xff00) + (((wTotalTracks - 1)>>2) & 0xc0)
                     + (wSectors & 0xbf);

      regs.reg_EDX = wDrive & 0xff;             /* drive ID */

      if(!DeviceIoControl(hVWIN32, VWIN32_DIOC_DOS_INT13,
                          &regs, sizeof(regs), &regs, sizeof(regs),
                          &cb, NULL))
      {
         return(1);
      }

   }

//   printf("?Fatal error occurred while attempting to format drive\n\n");

   return(2);
}



 /* wDrive: 'A'==0, 'B'==1, etc.                                           */
 /* wSP:  the stack pointer value on entry to the interrupt                */
 /* wDosSeg: the DOS segment containing the data at offset ZERO            */
 /*          (and the stack area following the data)                       */
 /* 1st 'absolute' sector on the drive is 1!                               */
 /* RETURNS:  1 if general failure, 2 if too many retries, 3 if write prot,*/
 /*           6 or 0x80 if disk drive door is open                         */

BOOL WINAPI WriteAbsoluteSector(HANDLE hVWIN32, UINT wDrive, UINT wAbsSector,
                                UINT wHeads, UINT wSectors, LPCSTR lpBuf)
{
UINT wHead, wTrack, wSector;

   wTrack  = (wAbsSector - 1) / (wHeads * wSectors);
   wSector = ((wAbsSector - 1) % wSectors) + 1;
   wHead   = ((wAbsSector - 1) % (wHeads * wSectors)) / wSectors;

   return(WriteHeadTrackSector(hVWIN32, wDrive, wHead, wTrack, wSector, lpBuf));
}



 /* wDrive: 'A'==0, 'B'==1, etc.                                           */
 /* wSP:  the stack pointer value on entry to the interrupt                */
 /* wDosSeg: the DOS segment containing the data at offset ZERO            */
 /*          (and the stack area following the data)                       */
 /* RETURNS:  1 if general failure, 2 if too many retries, 3 if write prot,*/
 /*           6 or 0x80 if disk drive door is open                         */

BOOL WINAPI WriteHeadTrackSector(HANDLE hVWIN32, UINT wDrive, UINT wHead,
                                 UINT wTrack, UINT wSector, LPCSTR lpBuf)
{
UINT i, iter;
DIOC_REGISTERS regs;
DWORD cb;



          /* auto-retry 4 times for FLOPPY drives only */

   for(iter=0; !iter || (iter<4 && !(wDrive & 0x80)); iter++)
   {
      regs.reg_EAX = 0x301;          /* write 1 sector */

      regs.reg_ECX = ((wTrack << 8) & 0xff00) + ((wTrack>>2) & 0xc0)
                + (wSector & 0xbf);
      regs.reg_EDX = ((wHead << 8) & 0xff00) + (wDrive & 0xff);

      regs.reg_EBX = (DWORD)lpBuf;


      if(!DeviceIoControl(hVWIN32, VWIN32_DIOC_DOS_INT13,
                          &regs, sizeof(regs), &regs, sizeof(regs),
                          &cb, NULL))
      {
//         printf("?Fatal error attempting to write to drive\n\n");
         return(1);
      }

      if(!(regs.reg_Flags & CARRY_SET))
      {
         return(FALSE);                        /* result is GOOD! */
      }

      i = (UINT)((regs.reg_EAX >> 8) & 0xff);  // get 'AL' on return for error type

      if(i==3)
      {
//         printf("?Disk is WRITE PROTECTED\n\n");
         if(iter>=3 || (wDrive & 0x80)) return(i);
      }
      else if(i==6)
      {
//         printf("?Disk removed/door open while writing to BIOS drive #");
//         PrintAChar(wDrive + '0');
//         printf("\n\n");
         if(iter>=3 || (wDrive & 0x80)) return(i);
      }
      else if(i==0x80)  /* disk timed out - possibly door is open */
      {
//         printf("?Diskette drive time-out\n\n");
         if(iter>=3 || (wDrive & 0x80)) return(i);
      }

      /* otherwise, retry up to 4 times in case motor isn't up to speed */

      LoopDispatch();  /* allow other things to run during re-try */

      regs.reg_EAX = 0x0;            /* RESET FLOPPY DISK SYSTEM */
      regs.reg_EDX = wDrive & 0xff;  /* drive # only */

      if(!DeviceIoControl(hVWIN32, VWIN32_DIOC_DOS_INT13,
                          &regs, sizeof(regs), &regs, sizeof(regs),
                          &cb, NULL))
      {
//         printf("?Fatal error attempting to reset drive\n\n");
         return(1);
      }

      LoopDispatch();  /* allow other things to run during re-try */

   }

   if(i==0x80) return(0x80);
   else        return(2);

}



BOOL WINAPI ReadHeadTrackSector(HANDLE hVWIN32, UINT wDrive, UINT wHead,
                                UINT wTrack, UINT wSector, LPSTR lpBuf)
{
UINT i, iter;
DIOC_REGISTERS regs;
DWORD cb;



          /* auto-retry 4 times for FLOPPY drives only */

   for(iter=0; !iter || (iter<4 && !(wDrive & 0x80)); iter++)
   {
      regs.reg_EAX = 0x201;          /* read 1 sector */

      regs.reg_ECX = ((wTrack << 8) & 0xff00) + ((wTrack>>2) & 0xc0)
                + (wSector & 0xbf);
      regs.reg_EDX = ((wHead << 8) & 0xff00) + (wDrive & 0xff);

      regs.reg_EBX = (DWORD)lpBuf;  /* address of BOOT SECTOR information */


      if(!DeviceIoControl(hVWIN32, VWIN32_DIOC_DOS_INT13,
                          &regs, sizeof(regs), &regs, sizeof(regs),
                          &cb, NULL))
      {
         return(1);
      }

      if(!(regs.reg_Flags & CARRY_SET))
      {
         return(FALSE);                        /* result is GOOD! */
      }

      i = (UINT)((regs.reg_EAX >> 8) & 0xff);  // get 'AL' on return for error type

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
//         printf("?Diskette drive time-out\n\n");
         if(iter>=3 || (wDrive & 0x80)) return(0x80);
      }

      /* otherwise, retry up to 4 times in case motor isn't up to speed */

      LoopDispatch();  /* allow other things to run during re-try */

      regs.reg_EAX = 0x0;            /* RESET FLOPPY DISK SYSTEM */
      regs.reg_EDX = wDrive & 0xff;  /* drive # only */

      if(!DeviceIoControl(hVWIN32, VWIN32_DIOC_DOS_INT13,
                          &regs, sizeof(regs), &regs, sizeof(regs),
                          &cb, NULL))
      {
         return(1);
      }

      LoopDispatch();  /* allow other things to run during re-try */

   }


   if(i==0x80) return(0x80);
   else        return(2);

}
