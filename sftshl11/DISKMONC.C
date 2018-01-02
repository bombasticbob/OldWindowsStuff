/**********************************************************************/
/*                           D I S K M O N C                          */
/*--------------------------------------------------------------------*/
/*    Task           : DISKMON is a short disk monitor program,       */
/*                     using BIOS interrupt 13(h) functions           */
/*--------------------------------------------------------------------*/
/*    Author         : MICHAEL TISCHER                                */
/*    Developed on   : 08/15/1987                                     */
/*    last update    : 06/08/1989                                     */
/*--------------------------------------------------------------------*/
/*    (MICROSOFT C)                                                   */
/*    Creation       : CL /AS DISKMONC.C                              */
/*    Call           : DISKMONC                                       */
/*--------------------------------------------------------------------*/
/*    (BORLAND TURBO C)                                               */
/*    Creation       : Make sure Case-sensitive link is OFF before    */
/*                     compiling & linking                            */
/*                     Select Compile/Make or RUN (no project file)   */
/**********************************************************************/

/*== Add include files ===============================================*/

#include <dos.h>
#include <stdio.h>
#include <ctype.h>

/*== Typedefs ========================================================*/

typedef unsigned char byte;                          /* Create a byte */

/*== Constants =======================================================*/

#define FALSE 0                 /* Constants to make reading the     */
#define TRUE 1                   /* source code easier                */

#define NUL     0                                   /* null character */
#define BEL     7                              /* bell character code */
#define BS      8                         /* backspace character code */
#define TAB     9                               /* tab character code */
#define LF      10                         /* linefeed character code */
#define CR      13                                 /* Return key code */
#define EF      26                                /* End of file code */
#define ESC     27                                     /* Escape code */

/*== Macros ==========================================================*/

#ifndef MK_FP                               /* MK_FP still undefined? */
#define MK_FP(s,o) ((void far *) (((unsigned long)(s) << 16) | (o)))
#define peekb(a,b) (*((byte far *) MK_FP((a),(b))))
#endif

/*-- The following macros state the offset and segment addresses of --*/
/*-- any pointer -----------------------------------------------------*/

#define GETSEG(p) ((unsigned)(((unsigned long) ((void far *) p)) >> 16))
#define GETOFS(p) ((unsigned) ((void far *) p))

/* -- Function declarations ------------------------------------------*/

byte DRead( byte, byte, byte, byte, byte, byte far * );
byte DWrite( byte, byte, byte, byte, byte, byte far * );

/*== Structures ======================================================*/

struct FormatDes {                    /* Describes format of a sector */
                  byte Track,
                       Side,
                       Sector,               /* logical sector number */
                       Length;
                 };

/**********************************************************************/
/* RESETDISK: Reset  all drives connected to system                   */
/* Input     : none                                                   */
/* Output    : error status                                           */
/**********************************************************************/

byte ResetDisk()

{
 union REGS Register;         /* Register variable for interrupt call */

 Register.h.ah = 0;                  /* Function number for reset = 0 */
 Register.h.dl = 0;                              /* Reset disk drives */
 int86(0x13, &Register, &Register);       /* Call BIOS disk interrupt */
/* printf("Result: %d\n", Register.h.ah); */
 return(Register.h.ah);                        /* Return error status */
}

/**********************************************************************/
/* WDS: Display status of the last disk operation                     */
/* Input     : see below                                              */
/* Output    : TRUE if no error, otherwise FALSE                      */
/**********************************************************************/

byte WDS(Status)
byte Status;                                           /* Disk status */

{
 if (Status)                                       /* Error occurred? */
  {                                                            /* YES */
   printf("ERROR: ");
   switch (Status)                               /* Display error msg */
    {
     case 0x01 : printf("Function number not permitted\n");
                 break;
     case 0x02 : printf("Address marking not found\n");
                 break;
     case 0x03 : printf("Disk is write-protected\n");
                 break;
     case 0x04 : printf("Sector not found\n");
                 break;
     case 0x06 : printf("Disk changed\n");
                 break;
     case 0x08 : printf("DMA overflow\n");
                 break;
     case 0x09 : printf("Data transfer past segment limit\n");
                 break;
     case 0x10 : printf("Read error\n");
                 break;
     case 0x20 : printf("Disk controller error\n");
                 break;
     case 0x40 : printf("Track not found\n");
                 break;
     case 0x80 : printf("Time Out error\n");
                 break;
     case 0xff : printf("Illegal parameter\n");
                 break;
     default   : printf("Error %d unknown\n", Status);
    }
   ResetDisk();                             /* Execute reset on error */
  }
 return(!Status);
}

/**********************************************************************/
/* DREAD: Read specified sector from disk                             */
/* Input     : see below                                              */
/* Output    : error status                                           */
/**********************************************************************/

byte DRead(Drive, Side, Track, Sector, Number,  Buffer)
byte Drive,                                           /* Drive number */
     Side,                     /* Disk side or read-write head number */
     Track,                                           /* Track number */
     Sector,                               /* First sector to be read */
     Number,                       /* Number of sectors to be written */
     far * Buffer; 	              /* FAR pointer to a byte vector */

{
 union REGS Register;         /* Register variable for interrupt call */
 struct SREGS SRegs;                /* Variables for segment register */

 Register.h.ah = 2;                     /* Function no. for read is 2 */
 Register.h.al = Number;                     /* Number in AL register */
 Register.h.dh = Side;                         /* Side in DH register */
 Register.h.dl = Drive;                         /* Drive number in DL */
 Register.h.ch = Track;                       /* Track in CH register */
 Register.h.cl = Sector;                     /* Sector in CL register */
 Register.x.bx = GETOFS ( Buffer );       /* Offset address of buffer */
 SRegs.es = GETSEG( Buffer );            /* Segment address of buffer */
 int86x(0x13, &Register, &Register, &SRegs);
 return(Register.h.ah);                        /* Return error status */
}

/**********************************************************************/
/* DWRITE: Write to the specified number of sectors                   */
/* Input     : see below                                              */
/* Output    : error status                                           */
/**********************************************************************/

byte DWrite(Drive, Side, Track, Sector, Number,  Buffer)
byte Drive,                         /* Number of drive to be accessed */
     Side,                  /* Disk side or number of read-write head */
     Track,                                           /* Track number */
     Sector,                            /* First sector to be written */
     Number,                                     /* Number of sectors */
     far * Buffer;                    /* FAR pointer to a byte vector */

{
 union REGS Register;         /* Register variable for interrupt call */
 struct SREGS SRegs;                    /* Segment register variables */

 Register.h.ah = 3;                    /* Function no. for write is 3 */
 Register.h.al = Number;                     /* Number in AL register */
 Register.h.dh = Side;                         /* Side in DH register */
 Register.h.dl = Drive;                         /* Drive number in DL */
 Register.h.ch = Track;                       /* Track in CH register */
 Register.h.cl = Sector;                     /* Sector in CL register */
 Register.x.bx = GETOFS( Buffer );        /* Offset address of buffer */
 SRegs.es = GETSEG( Buffer );            /* Segment address of buffer */
 int86x(0x13, &Register, &Register, &SRegs);   /* BIOS disk int. call */
 return(Register.h.ah);                        /* Return error status */
}

/**********************************************************************/
/* FORMAT: format a track                                             */
/* Input     : see below                                              */
/* Output    : error status                                           */
/* Info      : BYTES parameter gives the number of bytes in the for-  */
/*             matted sector. The following codes are allowed:        */
/*                             0 = 128 bytes, 1 =  256 bytes          */
/*                             2 = 512 bytes, 3 = 1024 bytes          */
/**********************************************************************/

byte Format(Drive, Side, Track, Number, Bytes)
byte Drive,
     Side,                                        /* Side/head number */
     Track,                                  /* Track to be formatted */
     Number,                       /* Number of sectors in this track */
     Bytes;                             /* Number of bytes per sector */

{
 union REGS Register;         /* Register variable for interrupt call */
 struct SREGS SRegs;                    /* Segment register variables */
 struct FormatDes Formate[15];               /* Maximum of 15 sectors */
 byte i;                                              /* Loop counter */

 if (Number <= 15)                                 /* Is number o.k.? */
  {
   for (i = 0; i < Number; i++)              /* Set sector descriptor */
    {
     Formate[i].Track = Track;                        /* Track number */
     Formate[i].Side = Side;                             /* Disk side */
     Formate[i].Sector = i+1;               /* Sector increments by 1 */
     Formate[i].Length = Bytes;          /* Number of bytes in sector */
    }
   Register.h.ah = 5;               /* Function number for formatting */
   Register.h.al = Number;                            /* Number in AL */
   Register.h.dh = Side;                         /* Side number in DH */
   Register.h.dl = Drive;                              /* Drive in DL */
   Register.h.ch = Track;                       /* Track number in CH */
   Register.x.bx = GETOFS ( Formate );       /* Offset addr. of table */
   SRegs.es=GETSEG( Formate );           /* Segment address of buffer */
   int86x(0x13, &Register, &Register, &SRegs); /* Call BIOS disk intr.*/
   return(Register.h.ah);                      /* Return error status */
  }
 else return(0xFF);                             /* Illegal parameters */
}

/**********************************************************************/
/* CONSTANTS : Change drive number, disk side and disk type           */
/*             (PC/XT or AT)                                          */
/* Input     : see below                                              */
/* Output    : none                                                   */
/**********************************************************************/

void Constants(Drive, Side, FTyp, AT)
byte *Drive,                             /* Pointer to drive variable */
     *Side,                               /* Pointer to side variable */
     FTyp,                                         /* Disk drive type */
     AT;                                 /* TRUE if computer is an AT */

{
 printf("Drive number (0-3): ");
 scanf("%d", &Drive);                            /* Read drive number */
 printf("Disk side (0 or 1): ");
 scanf("%d", &Side);                              /* Read head number */
 if (AT)                                          /* Used only by ATs */
  {
   printf("Format parameter:\n");
   printf("  1 = 320/360K diskette in 320/360K drive\n");
   printf("  2 = 320/360K diskette in 1.2MB drive\n");
   printf("  3 = 1.2MB diskette in 1.2MB drive - please enter choice: ");
   scanf("%d", &FTyp);
  }
}

/**********************************************************************/
/* HELP: Display help screen                                          */
/* Input     : none                                                   */
/* Output    : none                                                   */
/**********************************************************************/

void Help()

{
 printf("\nDISKMON (c) 1987 by Michael Tischer\n\n");
 printf("C O M M A N D   O V E R V I E W\n");
 printf("-------------------------------\n");
 printf("[E/e] = End\n");
 printf("[G/g] = Get (read)\n");
 printf("[S/s] = Fill a sector\n");
 printf("[R/r] = Reset\n");
 printf("[F/f] = Format\n");
 printf("[C/c] = Constants\n");
 printf("[?]   = Help\n\n");
}

/**********************************************************************/
/* GET  : Read a disk sector and display it on the screen             */
/* Input     : none                                                   */
/* Output    : none                                                   */
/**********************************************************************/

void ReadSector(Drive, Side)
byte Drive;      /* Drive number */
byte Side;                                        /* Disk side number */

{
 byte Buffer[512];                       /* Contents of filled sector */
 int  i,                                              /* Loop counter */
      Track,                     /* Track in which filled sector lies */
      Sector;                        /* Number of sector to be filled */

 printf("Track : ");
 scanf("%d", &Track);              /* Read track number from keyboard */
 printf("Sector: ");
 scanf("%d", &Sector);                          /* Read sector number */
 if (WDS(DRead(Drive, Side, Track, Sector, 1, Buffer)))
  {
   printf("----------------------------------------");
   printf("----------------------------------------");
   for (i = 0; i < 512; i++)     /* Display characters read from disk */
    switch (Buffer[i])                       /* ASCII code conversion */
     {
      case NUL : printf("<NUL>");
                 break;
      case BEL : printf("<BEL>");
                 break;
      case BS  : printf("<BS>");
                 break;
      case TAB : printf("<TAB>");
                 break;
      case LF  : printf("<LF>");
                 break;
      case CR  : printf("<CR>");
                 break;
      case ESC : printf("<ESC>");
                 break;
      case EF : printf("<EOF>");
                 break;
      default  : printf("%c", Buffer[i]);
     }
   printf("\n----------------------------------------");
   printf("----------------------------------------\n");
  }
}

/**********************************************************************/
/* FORMAT:    Format a specified number of sectors in a track with    */
/*            512 bytes                                               */
/* Input     : none                                                   */
/* Output    : none                                                   */
/**********************************************************************/

void FormatIt(Drive, Side, AT, FTyp)
byte Drive,       /* Drive number */
     Side,                                        /* Disk side number */
     AT,                                 /* TRUE if computer is an AT */
     FTyp;                                         /* Disk drive type */

{
 int  Track,                                 /* Track to be formatted */
      Number;                    /* Number of sectors to be formatted */

 printf("Track          : ");
 scanf("%d", &Track);              /* Read track number from keyboard */
 printf("No. of sectors : ");
 scanf("%d", &Number);                      /* Read number of sectors */
 if (AT)                                        /* Is computer an AT? */
  {
   union REGS Register;       /* Register variable for interrupt call */

   Register.h.ah = 0x17;            /* Function no. for set DASD-Type */
   Register.h.al = FTyp;
   Register.h.dl = Drive;
   int86(0x13, &Register, &Register);     /* Call BIOS disk interrupt */
  }
 WDS(Format(Drive, Side, Track, Number, 2, AT, FTyp));
}

/**********************************************************************/
/* FILL   : Fill a sector with a character                            */
/* Input     : see below                                              */
/* Output    : none                                                   */
/**********************************************************************/

void FillIt(Drive, Side)
byte Drive;                                           /* Drive number */
byte Side;                                        /* Disk side number */

{
 byte Buffer[512];                 /* Contents of sector to be filled */
 int  i,                                              /* Loop counter */
      Track,                        /* Track in which the sector lies */
      Sector;                        /* Number of sector to be filled */
 char Character;                                    /* Fill character */


 printf("Track       : ");
 scanf("%d", &Track);              /* Read track number from keyboard */
 printf("Sector      : ");
 scanf("%d", &Sector);            /* Read sector number from keyboard */
 printf("Fill char.  : ");
 scanf("\r%c", &Character);      /* Read fill character from keyboard */
 for (i = 0; i < 512; Buffer[i++] = Character)
  ;
 WDS(DWrite(Drive, Side, Track, Sector, 1, (byte far *) Buffer));
}

/**********************************************************************/
/**                           MAIN PROGRAM                           **/
/**********************************************************************/

void main()

{
 int  Drive,        					     /* Drive */
      Side,                                              /* Disk side */
      FTyp;                             /* Disk and disk drive format */
 byte AT;                              /* Computer type (AT or PC/XT) */
 char Entry;                                     /* Accept user input */

 Drive = Side = 0;                      /* Default of drive 0, side 0 */
 FTyp = 3;                    /* 1.2-MB diskette in 1.2-MB disk drive */

 /*-- Read PC type from location in ROM-BIOS -------------------------*/

 AT = (((byte) peekb(0xF000, 0xFFFE)) == 0xFC) ? TRUE : FALSE;
 printf("\n\nDISKMON (c) 1987 By Michael Tischer\n\n");
 WDS(ResetDisk());                      /* Execute reset first */
 do
  {
   printf("? = Help> ");                            /* Display prompt */
   scanf("\r %1c", &Entry);                         /* Get user input */
   switch(Entry = toupper(Entry))                  /* Execute command */
    {
     case '?' : Help();                        /* Display help screen */
                break;
     case 'R' : WDS(ResetDisk());                    /* Execute reset */
                break;
     case 'S' : FillIt(Drive, Side);                 /* Fill a sector */
                break;
     case 'F' : FormatIt(Drive, Side, AT, FTyp);
                break;
     case 'G' : ReadSector(Drive, Side);              /* Read sectors */
                break;
     case 'C' : Constants(&Drive, &Side, &FTyp, AT);
                break;
     default  : if (Entry != 'E') printf("Unknown command\n");
    }
  }
 while (Entry != 'E');                     /* "E" or "e" ends program */
}
