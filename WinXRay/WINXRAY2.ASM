;****************************************************************************
;**                                                                        **
;**     WINXRAY - (c) 1992,1993 by R. E. Frazier - all rights reserved     **
;**                                                                        **
;**      Assembly language support and 32-bit code are defined here!       **
;**                                                                        **
;****************************************************************************

MASM6 EQU 1

.386p
.LALL

ASSUME CS:NOTHING, DS:NOTHING, ES:NOTHING, SS:NOTHING


No_VMSeg EQU 1          ; this was added by me to exclude VM segment def's
NO_SEGMENTS EQU 1         ; CHICAGO header files do the same thing, this way!


INCLUDE VMM.INC
INCLUDE SHELL.INC
INCLUDE VXDLDR.INC
INCLUDE V86MMGR.INC

.NOLIST
INCLUDE WINDOWS.INC
.LIST

ASSUME CS:NOTHING, DS:NOTHING, ES:NOTHING, SS:NOTHING


;                      ******************************
;                      ** STRUCTURES AND CONSTANTS **
;                      ******************************


CALL_GATE STRUCT 1

   OffsetLow  WORD ?     ; LOW WORD offset  of called proc's address
   Selector   WORD ?     ; SELECTOR of called proc - may be a higher DPL
   ParmCount  BYTE ?
   Flags      BYTE ?     ; Must be CALL_GATE_Flags16 or CALL_GATE_Flags32
                         ; OR'd with the appropriate CALL_GATE_DPL# value

   OffsetHigh WORD ?     ; HIGH WORD offset of called proc's address

CALL_GATE ENDS

CALL_GATE_Flags16 = 10000100b
CALL_GATE_Flags32 = 10001100b
CALL_GATE_DPL0    = 00000000b
CALL_GATE_DPL1    = 00100000b
CALL_GATE_DPL2    = 01000000b
CALL_GATE_DPL3    = 01100000b



DESCRIPTOR STRUCT 1
   LowLimit   WORD ?     ; LOW WORD of segment limit
   LowBase    WORD ?     ; LOW WORD of segment base address
   MidBase    BYTE ?     ; 'Middle Byte' of segment base address
   LowFlags   BYTE ?     ; Low byte of FLAGS (DPL, type)
   HighFlags  BYTE ?     ; High byte of FLAGS, plus upper 4 bits of seg limit
   HighBase   BYTE ?     ; High byte of segment base address
DESCRIPTOR ENDS

;* the following flags are used in the LAST word of MyChangeSelector2 *
;* and must be bit-wise 'OR'ed with one another to get proper results *

MCS2_CODE      = 0000000000011000b
MCS2_DATA      = 0000000000010000b

MCS2_ACCESS    = 0000000000000001b
MCS2_EXECREAD  = 0000000000000010b    ; APPLIES TO CODE SEGMENT
MCS2_CONFORM   = 0000000000000100b    ; APPLIES TO CODE SEGMENT
MCS2_WRITEDATA = 0000000000000010b    ; APPLIES TO DATA SEGMENT
MCS2_XPANDDOWN = 0000000000000100b    ; APPLIES TO DATA SEGMENT

MCS2_DPL0      = 0000000000000000b    ; for reference only - not required
MCS2_DPL1      = 0000000000100000b
MCS2_DPL2      = 0000000001000000b
MCS2_DPL3      = 0000000001100000b
MCS2_DPLMASK   = 0000000001100000b

MCS2_PRESENT   = 0000000010000000b    ; must be set, or a 'not present'
                                      ; exception will occur when accessed


;** 32-bit segment bits - only assign these for 32-bit segments **

MCS2_32BIT     = 0100000000000000b    ; 32-bit when set; 16-bit when clear
                                      ; REQUIRED if any of the next 3 are set
MCS2_AVAIL     = 0001000000000000b    ; 'AVAILABLE' bit - usable by app?
MCS2_PAGEGRAN  = 1000000000000000b    ; PAGE granularity if set






;                ****************************************
;                ** PROTOTYPES AND SEGMENT DEFINITIONS **
;                ****************************************



;** Windows PROC and EXTERNAL DATA definitions **

GetWinFlags      PROTO FAR16 PASCAL

GlobalPageLock   PROTO FAR16 PASCAL, :WORD
GlobalPageUnlock PROTO FAR16 PASCAL, :WORD

GlobalHandle     PROTO FAR16 PASCAL, :WORD

GlobalFix        PROTO FAR16 PASCAL, :WORD
GlobalUnfix      PROTO FAR16 PASCAL, :WORD



EXTERNDEF PASCAL __WinFlags:WORD

WF_80x87 = 400H     ; correct definition for THIS flag in '__WinFlags'




; NOTE:  These 4 procs are exported by 3.0 kernel also!

GetSelectorLimit PROTO FAR16 PASCAL, :WORD
SetSelectorLimit PROTO FAR16 PASCAL, :WORD, :DWORD
GetSelectorBase  PROTO FAR16 PASCAL, :WORD
SetSelectorBase  PROTO FAR16 PASCAL, :WORD, :DWORD



;** INTERNAL PROC DEFINITIONS **

MyAllocSelector    PROTO FAR16 PASCAL
MyFreeSelector     PROTO FAR16 PASCAL, :WORD
MyChangeSelector   PROTO FAR16 PASCAL, :WORD, :DWORD, :WORD
MyChangeSelector2  PROTO FAR16 PASCAL, :WORD, :DWORD, :DWORD, :WORD





; BOOL FAR PASCAL MyChangeSelector2(WORD wSel, DWORD dwOffs, DWORD dwLim,
;                                   WORD wFlags);

;** PUBLIC DEFINITIONS - LABELS AND PROCS **


PUBLIC PASCAL wCode32Alias, PASCAL wData32Alias
PUBLIC PASCAL wCallGateSeg, PASCAL wCode32Seg ; , PASCAL wData32Seg



;** PROTOTYPES (INTERNAL STUFF) **

RingZeroProc        PROTO FAR32 PASCAL, :DWORD
GetSystemVM32       PROTO FAR32 PASCAL
GetNextVM32         PROTO FAR32 PASCAL
MyCopyPageTable32   PROTO FAR32 PASCAL
GetVMInfo32         PROTO FAR32 PASCAL
GetEMSXMSLimits32   PROTO FAR32 PASCAL
GetPgStatus32       PROTO FAR32 PASCAL
GetTimeSlicePri32   PROTO FAR32 PASCAL
GetDemandPageInfo32 PROTO FAR32 PASCAL
GetVMPgCount32      PROTO FAR32 PASCAL
GetSysPageCount32   PROTO FAR32 PASCAL
GetPageTable32      PROTO FAR32 PASCAL
GetPageDirectory32  PROTO FAR32 PASCAL
;Enable87_32         PROTO FAR32 PASCAL
;Disable87_32        PROTO FAR32 PASCAL
GetVMExecTime32     PROTO FAR32 PASCAL
GetVXDList32        PROTO FAR32 PASCAL
GetVMMDDB32         PROTO FAR32 PASCAL



MyGetSystemVM       PROTO FAR16 PASCAL
MyGetNextVM         PROTO FAR16 PASCAL, :DWORD
MyCopyPageTable     PROTO FAR16 PASCAL, :DWORD, :DWORD, :FAR16 PTR BYTE
MyGetVMInfo         PROTO FAR16 PASCAL, :DWORD, :FAR16 PTR DWORD
MyGetDemandPageInfo PROTO FAR16 PASCAL, :FAR16 PTR DemandInfoStruc


MyCreateCallGate   PROTO FAR16 PASCAL, :WORD, :DWORD, :WORD, :WORD
MyAssignDescriptor PROTO FAR16 PASCAL, :WORD, :FAR16 PTR DESCRIPTOR

MyGetVXDList       PROTO FAR16 PASCAL, :FAR16 PTR DWORD

;
; **  SEGMENT DEFINITIONS (with all of their appropriate attributes)  **
;


MYDATA16 SEGMENT WORD PUBLIC USE16 'DATA'
MYDATA16 ENDS

_HELPERCODE SEGMENT WORD PUBLIC USE16 'CODE'
_HELPERCODE ENDS

CODE32_TEXT SEGMENT DWORD PUBLIC USE32 'PCODE'
CODE32_TEXT ENDS

;DATA32_DATA SEGMENT DWORD PUBLIC USE32 'PDATA'
;DATA32_DATA ENDS


DGROUP GROUP MYDATA16

HUGEMEM_TEXT  SEGMENT BYTE PUBLIC USE16 'CODE'
HUGEMEM_TEXT  ENDS
HUGEMEM1_TEXT SEGMENT BYTE PUBLIC USE16 'CODE'
HUGEMEM1_TEXT ENDS
HUGEMEM2_TEXT SEGMENT BYTE PUBLIC USE16 'CODE'
HUGEMEM2_TEXT ENDS
HUGEMEM3_TEXT SEGMENT BYTE PUBLIC USE16 'CODE'
HUGEMEM3_TEXT ENDS
HUGEMEM4_TEXT SEGMENT BYTE PUBLIC USE16 'CODE'
HUGEMEM4_TEXT ENDS
HUGEMEM5_TEXT SEGMENT BYTE PUBLIC USE16 'CODE'
HUGEMEM5_TEXT ENDS
LQSORT_TEXT   SEGMENT BYTE PUBLIC USE16 'CODE'
LQSORT_TEXT   ENDS


HGROUP GROUP HUGEMEM_TEXT, HUGEMEM1_TEXT, HUGEMEM2_TEXT, HUGEMEM3_TEXT
HGROUP GROUP HUGEMEM4_TEXT, HUGEMEM5_TEXT, LQSORT_TEXT





;                  **************************************
;                  ** DATA SEGMENTS - STATIC VARIABLES **
;                  **************************************


;DATA32_DATA SEGMENT
;
;StackBottom32 LABEL DWORD
;           DD 400H DUP (0)
;StackTop32 LABEL DWORD
;
;
;DATA32_DATA ENDS
;


MYDATA16 SEGMENT

wCode32Alias    WORD 0   ;** selector for CODE32_TEXT alias (FLAT) **
wData32Alias    WORD 0   ;** selector for DATA32_DATA alias (FLAT) **


lpRingZeroProc  LABEL FWORD
                DWORD 0
wCallGateSeg    WORD 0    ;** selector for Call Gate **

ReturnVMHandle  DWORD 0
ReturnDDBAddr   DWORD 0

dwPageNum       DWORD 0   ; start page # for MyCopyPageBuf and
                          ; 'page directory' entry for MyGetPageTable
dwNPages        DWORD 0   ; # of pages to copy for MyCopyPageBuf
dwPageBuf       DWORD 0   ; 32-bit OFFSET for page buffer in MyCopyPageBuf
                          ; and MyGetPageTable, MyGetPageDirectory.

dwEMSMin        DWORD ?   ; return value storage for MyGetVMInfo
dwEMSMax        DWORD ?
dwXMSMin        DWORD ?
dwXMSMax        DWORD ?
dwHMAEnabled    DWORD ?


pDI             DemandInfoStruc <0>
                                               ; used by 'GetDemandPageInfo'


;lpCallGate1     LABEL DWORD
;                WORD 0
;wCallGateSeg1   WORD 0   ;** selector for Call Gate #1      **
;
;lpCallGate2     LABEL DWORD
;                WORD 0
;wCallGateSeg2   WORD 0   ;** selector for Call Gate #2      **
;

wCode32Seg       WORD 0   ; use as flag; if NOT ZERO it's PAGE LOCKED!

;wCode32Seg      WORD CODE32_TEXT  ;** segment for 32-bit CODE **
;wData32Seg      WORD DATA32_DATA  ;** segment for 32-bit DATA **

;
;lpGetSystemVM32 LABEL FWORD
;                DWORD LROFFSET CODE32_TEXT:GetSystemVM32  ;** OFFSET **
;                WORD  0
;lpGetNextVM32   LABEL FWORD
;                DWORD LROFFSET CODE32_TEXT:GetNextVM32    ;** OFFSET **
;                WORD  0



MYDATA16 ENDS




;** The following macros support inter-segment calls between **
;** USE16 and USE32 segments.  For convenience, really...    **


RET32 MACRO arg1
   IF @WordSize == 2
      DB 66H              ; performs USE32 override on return
   ENDIF
   RET arg1
   ENDM

RET16 MACRO arg1
   IF @WordSize == 4      ; performs USE16 override on return
      DB 66H
   ENDIF
   RET arg1
   ENDM

CALL32 MACRO arg1
   IF @WordSize == 2
      DB 66H
   ENDIF

   CALL arg1
   ENDM

CALL16 MACRO arg1
   IF @WordSize == 4
      DB 66H
   ENDIF

   CALL arg1
   ENDM




;                             **************
;                             ** THE CODE **
;                             **************



_HELPERCODE SEGMENT

ASSUME DS:DGROUP, SS:NOTHING

;** Procs in the HELPER segment must use WORDSIZE OVERRIDE 'DB 66H' **
;** on CALL and RETF to and from USE32 segments to prevent crashes! **
;** Therefore, I must assume which word size the caller has before  **
;** I perform any returns.                                          **



MYDATA16 SEGMENT          ; STATIC DATA FOR NEXT FEW PROCS

CallGateTemp    CALL_GATE <0,0,0,0,0>

DescriptorTemp  DESCRIPTOR <0,0,0,0,0,0>


GDTTemp         LABEL FWORD
   GDTLimit     WORD 0
   GDTAddress   DWORD 0

MYDATA16 ENDS


MyGetDescriptor PROC FAR16 PASCAL PUBLIC USES bx cx si di,\
                wSelector:WORD, lpDestDescriptor:FAR PTR DESCRIPTOR

   ASSUME DS:DGROUP

   LOCAL wTempSel:WORD, lpDescriptor:FAR PTR DESCRIPTOR, dwAddr:DWORD


   ;** first, get the GDT information! **

   sgdt FWORD PTR GDTTemp

   INVOKE MyAllocSelector    ; get a selector!
   mov wTempSel, ax

   .IF ax!=0

      ;** assign GDT address to this selector, and 'lpDescriptor' pointer **

      mov WORD PTR lpDescriptor + 2, ax

      INVOKE MyChangeSelector, wTempSel, GDTAddress, 0ffffH

      .IF ax==0

         ;** now that I have the GDT info here, let's see if the **
         ;** desired selector is a GDT entry, or an LDT entry!   **

         clc

         .IF wSelector & 4H          ; this selector is an LDT selector?

            sldt ax                  ; get the LDT selector

            and ax, 0fff8H           ; mask out any unwanted bits
            mov WORD PTR lpDescriptor, ax

            push es
            les bx, lpDescriptor     ; load pointer to descriptor

            ASSUME BX:PTR DESCRIPTOR

            mov ax, es:[bx].LowBase  ; 'low' base address
            mov dl, es:[bx].MidBase  ; 'mid' base address
            mov dh, es:[bx].HighBase ; 'high' base address

            mov WORD PTR dwAddr, ax  ; store the FULL address in 'dwAddr'
            mov WORD PTR dwAddr+2, dx

            pop es

            ASSUME BX:NOTHING

            ;** re-assign 'wTempSel' to point to LDT's address **

            INVOKE MyChangeSelector, wTempSel, dwAddr, 0ffffH

            .IF ax==0
               clc
            .ELSE
               stc
            .ENDIF

         .ENDIF

         .IF !CARRY?

            mov ax, wSelector
            and ax, 0fff8H           ; mask out any unwanted bits
            mov WORD PTR lpDescriptor, ax


            push ds
            push es

            lds si, lpDescriptor
            les di, lpDestDescriptor
            cld
            mov cx, SIZE DESCRIPTOR
            rep movsb     ; copies 'lpSrcDescriptor' onto 'lpDescriptor'

            pop es
            pop ds

            xor ax, ax    ; this flags 'good result'

         .ELSE

            mov ax, 1     ; ** this is an error code **

         .ENDIF

      .ENDIF

      push ax

      INVOKE MyFreeSelector, wTempSel

      pop ax

   .ELSE

      inc ax

   .ENDIF

   RET

MyGetDescriptor ENDP



MyAssignDescriptor PROC FAR16 PASCAL PUBLIC USES bx cx si di,\
                   wSelector:WORD, lpSrcDescriptor:FAR PTR DESCRIPTOR

   ASSUME DS:DGROUP

   LOCAL wTempSel:WORD, lpDescriptor:FAR PTR DESCRIPTOR, dwAddr:DWORD


   ;** first, get the GDT information! **

   sgdt FWORD PTR GDTTemp

   INVOKE MyAllocSelector    ; get a selector!
   mov wTempSel, ax

   .IF ax!=0

      ;** assign GDT address to this selector, and 'lpDescriptor' pointer **

      mov WORD PTR lpDescriptor + 2, ax

      INVOKE MyChangeSelector, wTempSel, GDTAddress, 0ffffH

      .IF ax==0

         ;** now that I have the GDT info here, let's see if the **
         ;** desired selector is a GDT entry, or an LDT entry!   **

         clc

         .IF wSelector & 4H          ; this selector is an LDT selector?

            sldt ax                  ; get the LDT selector

            and ax, 0fff8H           ; mask out any unwanted bits
            mov WORD PTR lpDescriptor, ax

            push es
            les bx, lpDescriptor     ; load pointer to descriptor

            ASSUME BX:PTR DESCRIPTOR

            mov ax, es:[bx].LowBase  ; 'low' base address
            mov dl, es:[bx].MidBase  ; 'mid' base address
            mov dh, es:[bx].HighBase ; 'high' base address

            mov WORD PTR dwAddr, ax  ; store the FULL address in 'dwAddr'
            mov WORD PTR dwAddr+2, dx

            pop es

            ASSUME BX:NOTHING

            ;** re-assign 'wTempSel' to point to LDT's address **

            INVOKE MyChangeSelector, wTempSel, dwAddr, 0ffffH

            .IF ax==0
               clc
            .ELSE
               stc
            .ENDIF

         .ENDIF

         .IF !CARRY?

            mov ax, wSelector
            and ax, 0fff8H           ; mask out any unwanted bits
            mov WORD PTR lpDescriptor, ax


            push ds
            push es

            lds si, lpSrcDescriptor
            les di, lpDescriptor
            cld
            mov cx, SIZE DESCRIPTOR
            rep movsb     ; copies 'lpSrcDescriptor' onto 'lpDescriptor'

            pop es
            pop ds

            mov ax, ss    ; 'jog' the segment registers to refresh from RAM
            mov ss, ax
            mov ax, fs
            mov fs, ax
            mov ax, gs
            mov gs, ax

            xor ax, ax    ; this flags 'good result'

         .ELSE

            mov ax, 1     ; ** this is an error code **

         .ENDIF

      .ENDIF

      push ax

      INVOKE MyFreeSelector, wTempSel

      pop ax

   .ELSE

      inc ax

   .ENDIF

   RET

MyAssignDescriptor ENDP



MyGetAccessRights PROC FAR16 PASCAL PUBLIC, wSelector:WORD

   LOCAL desc:DESCRIPTOR


   INVOKE MyGetDescriptor, wSelector, ADDR desc

   .IF ax==0

      mov al, desc.LowFlags
      mov ah, desc.HighFlags
      and ah, 0f0H            ; only upper 4 bits are FLAGS

      clc

   .ELSE

      stc
      mov ax, 0
      mov dx, 0

   .ENDIF

   RET

MyGetAccessRights ENDP



MyGetSelectorBase PROC FAR16 PASCAL PUBLIC, wSelector:WORD

   LOCAL desc:DESCRIPTOR


   INVOKE MyGetDescriptor, wSelector, ADDR desc

   .IF ax==0

      mov ax, desc.LowBase
      mov dl, desc.MidBase
      mov dh, desc.HighBase

      clc

   .ELSE

      stc
      mov ax, 0

   .ENDIF

   RET

MyGetSelectorBase ENDP



MyGetSelectorLimit PROC FAR16 PASCAL PUBLIC, wSelector:WORD

   LOCAL desc:DESCRIPTOR


   INVOKE MyGetDescriptor, wSelector, ADDR desc

   .IF ax==0

      mov ax, desc.LowLimit
      mov dh, desc.HighFlags
      mov dl, dh
      and dl, 0fH                  ; lower 4 bits are part of limit

      .IF dh & 80H                 ; that is, 'G' bit set (page granularity)

         mov dh, dl
         mov dl, ah
         mov ah, al                ; quickly rotate 8 bits to the left
         mov al, 0ffH              ; pre-load with 1's

         shl ax,1
         rcl dx,1
         shl ax,1
         rcl dx,1
         shl ax,1
         rcl dx,1
         shl ax,1
         rcl dx,1                  ; 4 rotates of DWORD value

         mov al, 0ffH              ; ensure low 12 bits are 'fffH'

      .ELSE

         mov dh, 0

      .ENDIF

      clc

   .ELSE

      stc
      mov ax, 0

   .ENDIF

   RET

MyGetSelectorLimit ENDP



MyCreateCallGate  PROC FAR16 PASCAL PUBLIC USES di bx es,\
                  wSelector:WORD, dwOffs:DWORD, wGateFlags:WORD, \
                  wParmCount:WORD

   ASSUME DS:DGROUP

   LOCAL wCallGate:WORD   ; the selector value (call gate) returned by
                          ; this proc.  Caller must free it by calling
                          ; 'MyFreeSelector()'.  Caller may call a function
                          ; using this call gate at any time so long as
                          ; the selector 'wSelector' is valid.


   .IF wParmCount>1fH     ; PARM COUNT in excess of 31 is ILLEGAL!
      mov ax, 0
      RET
   .ENDIF

   INVOKE MyAllocSelector
   mov wCallGate, ax

   .IF ax!=0

      ;** load information into 'CallGateTemp' structure... **

      mov ax, wSelector
      mov CallGateTemp.Selector, ax

      mov ax, WORD PTR dwOffs
      mov CallGateTemp.OffsetLow, ax

      mov ax, WORD PTR dwOffs + 2
      mov CallGateTemp.OffsetHigh, ax

      mov al, BYTE PTR wParmCount
      mov CallGateTemp.ParmCount, al

      mov al, BYTE PTR wGateFlags
      mov CallGateTemp.Flags, al

      INVOKE MyAssignDescriptor, wCallGate, ADDR DS:CallGateTemp

      .IF ax!=0

         INVOKE MyFreeSelector, wCallGate    ; free allocated selector first
         xor ax, ax
         mov wCallGate, ax                   ; store a zero here for 'Rval'

      .ENDIF

   .ENDIF

   mov ax, wCallGate      ; return value!

   RET

MyCreateCallGate  ENDP



MyChangeSelector2 PROC FAR16 PASCAL PUBLIC USES bx cx, \
                  wSelector:WORD, dwAddr:DWORD, dwLimit:DWORD, wFlags:WORD

   ASSUME DS:DGROUP

   mov bx, wFlags
   and bx, 8000H            ; see if PAGE GRANULARITY is being used...

   .IF wSelector==0 || dwLimit==0 || wFlags==0 \
       || (dwLimit > 0fffffH && bx==0)

      mov ax, 1

   .ELSE

      ; Determine if PAGE or BYTE granularity, and calculate Segment Limit

      .IF bx==0           ; this means BYTE GRANULARITY

         mov ax, WORD PTR dwLimit
         mov DescriptorTemp.LowLimit, ax

         mov ax, WORD PTR dwLimit + 2
         and al, 0fH      ; maximum value for upper 4 bits...
         mov DescriptorTemp.HighFlags, al

      .ELSE

         mov eax, dwLimit
         shr eax, 12      ; shift 12 bits to the right to convert to pages

         mov DescriptorTemp.LowLimit, ax

         shr eax, 16      ; put the 'upper 4 bits' into 'al's lower 4 bits
         and al, 0fH      ; do this 'just in case'
         mov DescriptorTemp.HighFlags, al
                          ; upper 4 bits of 'al' are always zero at this pt.

      .ENDIF


      ;** Next step:  The base address **

      mov ax, WORD PTR dwAddr
      mov DescriptorTemp.LowBase, ax

      mov ax, WORD PTR dwAddr + 2
      mov DescriptorTemp.MidBase, al
      mov DescriptorTemp.HighBase, ah


      ;** Finally, the FLAGS word - don't verify, just do it! **

      mov ax, wFlags
      and ax, 0f0ffH      ; this masks out the 'upper limit' bits...

      mov DescriptorTemp.LowFlags, al
      or  DescriptorTemp.HighFlags, ah


      ;** At this point the descriptor has been built.  NOW, copy **
      ;** this descriptor on top of the descriptor for 'wSelector'**

      INVOKE MyAssignDescriptor, wSelector, ADDR DS:DescriptorTemp

      ; result comes back in 'ax' and is passed to caller from here!

   .ENDIF

   RET

MyChangeSelector2 ENDP




GetCode32Alias PROC NEAR PASCAL PRIVATE USES SI DI ES

   LOCAL dwLimit:DWORD, dwBase:DWORD, IDT:FWORD, wMySel:WORD

   ASSUME DS:DGROUP


   ; FIRST, need to PageLock and FIX the selector!

   .IF wCode32Seg==0

      mov ax, CODE32_TEXT                  ; segment for RingZeroProc!
      mov wCode32Seg, ax                   ; store in 'wCode32Seg'

      INVOKE GlobalHandle, ax              ; get HGLOBAL from SELECTOR
      push ax

      INVOKE GlobalFix, ax                 ; FIX code seg in linear memory!

      pop ax

      INVOKE GlobalPageLock, ax            ; page lock this segment also!

   .ENDIF


   ; NEXT, OBTAIN SELECTOR LIMIT AND BASE FOR 'CODE32_TEXT'

   mov si, CODE32_TEXT

   INVOKE GetSelectorLimit, si

   mov WORD PTR dwLimit, ax
   mov WORD PTR dwLimit + 2, dx

   INVOKE GetSelectorBase, si

   mov WORD PTR dwBase, ax
   mov WORD PTR dwBase + 2, dx


;   ;FOR NOW, DO THIS!  Later, I might create my own 'flat segment'
;
;   mov wCode32Alias, 28H
;

   sidt FWORD PTR IDT      ; get the IDT address!  Yay!

   ;* Then, allocate a selector and assign the IDT offset to it! *

   INVOKE MyAllocSelector
   mov wMySel, ax

   INVOKE MyChangeSelector, wMySel, DWORD PTR (IDT + 2), 0ffffH


   ;* Next, find the first IDT entry with a 32-bit RING 0 FLAT CS *

   mov es, wMySel
   mov si, 0
   mov cx, 0ffH

   .REPEAT

      mov di, es:[si + 2]    ; the selector portion of the interrupt gate!

      INVOKE MyGetSelectorBase, di

      .IF !CARRY? && ax==0 && dx==0                ; correct base...

         INVOKE MyGetSelectorLimit, di

         .IF !CARRY? && ax==0ffffH && dx==0ffffH   ; correct limit...

            INVOKE MyGetAccessRights, di

            and al, 78H         ; mask out 'C', 'P', 'R', & 'A' flags

            .BREAK .IF al==18H         ; it's a RING 0 CODE SELECTOR!!

         .ENDIF

      .ENDIF

      add si, SIZE DESCRIPTOR   ; add size of descriptor
      mov di, 0                 ; flags 'bad' result (as needed)

   .UNTILCXZ

   mov wCode32Alias, di

   push ds
   pop es                  ; don't leave invalid selector here!

   INVOKE MyFreeSelector, wMySel ; free temp selector I allocated earlier

   .IF wCode32Alias==0     ; I still have a non-valid selector here

      mov ax, 1
      RET

   .ENDIF




   ;** CREATE 2 32-bit CALL GATES USING 'wCode32Alias' AS THE SELECTOR! **

   mov eax, OFFSET CODE32_TEXT:RingZeroProc ; have to do it *THIS* way...
   add eax, dwBase         ; create ABSOLUTE ADDRESS for this proc!!

   INVOKE MyCreateCallGate, wCode32Alias, eax,\
                            (CALL_GATE_Flags32 + CALL_GATE_DPL3), 1

   ;** NOTE:  Call Gate passes 1 DWORD (4 bytes) of parms to proc, and **
   ;**        automatically removes 1 DWORD from both stacks on 'RET'  **
   ;**        (parm count of proc must match DWORD count in call gate) **


   .IF ax==0      ; this only happens on error!!
      xor ax, ax
      mov wCode32Alias, ax

      inc ax
      RET
   .ENDIF

   mov wCallGateSeg, ax                 ; CALL GATE for 'RingZeroProc'
   mov WORD PTR lpRingZeroProc + 4, ax  ; assign same value to FWORD PTR

   mov ax, 0

   RET


GetCode32Alias ENDP


GetData32Alias PROC NEAR PASCAL PRIVATE USES SI ES

   LOCAL dwLimit:DWORD, dwBase:DWORD, wMySel:WORD

   ASSUME DS:DGROUP

;   ;FOR NOW, DO THIS!
;
;   mov wData32Alias, 30H
;   mov ax, 0
;
   ;* Using the ACTIVE TSS obtain the RING 0 'SS' *
   ;* which is the correct data segment!!         *


   str si            ; note:  this segment may *NOT* have accessible DPL

   INVOKE MyGetSelectorBase, si

   mov WORD PTR dwBase, ax
   mov WORD PTR dwBase + 2, dx


   ;** allocate a selector with a valid DPL and assign the TSS's base **
   ;** address to it so that I can access its contents.               **

   INVOKE MyAllocSelector
   mov wMySel, ax

   INVOKE MyChangeSelector, wMySel, dwBase, 0ffffH

   mov es, wMySel
   mov si, 8               ; offset 8 into 32-bit TSS is the correct SS/DS

   mov ax, es:[si]

   mov wData32Alias, ax

   push ds
   pop es                  ; don't leave invalid selector here!

   INVOKE MyFreeSelector, wMySel ; free temp selector I allocated earlier

   INVOKE GetSelectorBase, wData32Alias
   or ax, dx

   ;** If the selector base is not zero, the return value  **
   ;** is not zero; otherwise, it's zero (indicating 'ok') **

   RET

GetData32Alias ENDP


_HELPERCODE ENDS





;****************************************************************************
;*                                                                          *
;*          PROCS WHICH PERFORM THE 'VMM' API CALLS & 'HELPER' CODE         *
;*                                                                          *
;****************************************************************************


CallRingZeroProc MACRO F

IFB <F>
ERROR Parameter must not be blank, 'CallRingZeroProc'
ENDIF


   .IF wCode32Alias==0
      INVOKE GetCode32Alias
      .IF ax!=0
         mov ax, 0
         mov dx, 0
         RET
      .ENDIF
   .ENDIF

   .IF wData32Alias==0
      INVOKE GetData32Alias
      .IF ax!=0
         mov ax, 0
         mov dx, 0
         RET
      .ENDIF
   .ENDIF

   ;** get ready to call 32-bit code...     **
   ;** ON ENTRY:  FS == DGROUP, GS == STACK **
   ;**            ESI == ESP                **
   ;** ALSO:  AX == 'NEW' DS (ring 0 FLAT)  **

   push ds
   push es

   push ebp                       ; save ebp, esi, edi
   push esi
   push edi

;;   pushad                         ; push all (32-bit) registers

   mov esi, esp
   mov ax, ss
   mov gs, ax                     ; use 'GS' for OLD STACK SEGMENT

   mov ax, ds
   mov fs, ax                     ; use 'FS' for DGROUP


   mov ax, wData32Alias

   PUSHCONTEXT ASSUMES

   ASSUME DS:NOTHING, ES:NOTHING, FS:DGROUP, GS:NOTHING


   pushd F                        ; function code (as needed) - one DWORD
   call DS:[lpRingZeroProc]

;;   popad                          ; pop all (32-bit) registers

   pop edi
   pop esi
   pop ebp                        ; restore ebp, esi, edi

   pop es
   pop ds

   POPCONTEXT ASSUMES

   ENDM



_HELPERCODE SEGMENT

ASSUME DS:DGROUP, ES:NOTHING, FS:NOTHING, GS:NOTHING, SS:NOTHING


MyGetSystemVM PROC FAR16 PASCAL PUBLIC


   CallRingZeroProc 0     ; function 0 - Get System VM

   mov ax, WORD PTR ReturnVMHandle
   mov dx, WORD PTR ReturnVMHandle + 2


   ;** at this point DX:AX contains the VM HANDLE.  Next, clear **
   ;** the carry bit to 'definitely' flag that it worked ok,    **
   ;** unless DX:AX contains zero (in which case we set carry). **

   .IF AX==0 && DX==0
      stc
   .ELSE
      clc
   .ENDIF

   RET


MyGetSystemVM ENDP


MyGetNextVM PROC FAR16 PASCAL PUBLIC, dwPreviousVM:DWORD


   .IF dwPreviousVM==0           ; if 'PREVIOUS VM' is ZERO return
      INVOKE MyGetSystemVM       ; the SYSTEM VM (assume we're starting)
      RET
   .ENDIF


   mov ebx, dwPreviousVM
   mov ReturnVMHandle, ebx        ; store here - 'GetNextVM32' uses it

   CallRingZeroProc 1             ; function 1 - Get NEXT VM

   mov ax, WORD PTR ReturnVMHandle
   mov dx, WORD PTR ReturnVMHandle + 2


   ;** at this point DX:AX contains the VM HANDLE.  Next, clear **
   ;** the carry bit to 'definitely' flag that it worked ok,    **
   ;** unless DX:AX contains zero (in which case we set carry). **

   .IF AX==0 && DX==0
      stc
   .ELSE
      clc
   .ENDIF

   RET

MyGetNextVM ENDP


MyCopyPageTable PROC FAR16 PASCAL PUBLIC,
                dwStartPage:DWORD, dwPageCount:DWORD, lpPageBuf:FAR16 PTR BYTE


   INVOKE GetSelectorBase, WORD PTR lpPageBuf + 2

   shl edx, 10H
   mov dx, ax             ; 'edx' contains SELECTOR BASE for buffer

   movzx eax, WORD PTR lpPageBuf

   add eax, edx           ; ABSOLUTE offset for buffer!

   mov dwPageBuf, eax     ; store offset here...

   mov eax, dwStartPage
   mov dwPageNum, eax

   mov eax, dwPageCount
   mov dwNPages, eax      ; parameters copied.  Now, on with the show!

   CallRingZeroProc 2     ; function 2 - COPY PAGE TABLE
                          ; value in EAX remains 'as is' on exit

   RET


MyCopyPageTable ENDP


MyGetPgStatus PROC FAR16 PASCAL PUBLIC, dwVMHandle:DWORD, dwPage:DWORD

   mov ebx, dwVMHandle
   mov ReturnVMHandle, ebx

   mov eax, dwPage
   mov dwPageNum, eax

   CallRingZeroProc 6     ; function 6 - Get Page Status for page, VM

   mov edx, eax
   shr edx, 10H           ; returns flags in dx:ax and eax

   RET

MyGetPgStatus ENDP



MyGetVMInfo PROC FAR16 PASCAL PUBLIC USES ES SI,
            dwVMHandle:DWORD, lpdwRval:FAR16 PTR DWORD

LOCAL hWinOldAp:WORD

   mov ebx, dwVMHandle
   mov ReturnVMHandle, ebx        ; store here - 'GetVMInfo32' uses it

   CallRingZeroProc 3     ; function 3 - Get VM Info (3.1 and later)
                          ; value in EAX remains 'as is' on exit

   les si, lpdwRval

   mov es:[si], eax       ; 'low' flags
   mov es:[si + 4], ecx   ; 'high' flags (reserved - none yet defined)

   mov hWinOldAp, dx      ; Chicago returns WINOLDAP hWnd in dx


   ; NEXT, obtain additional info about EMS, XMS

   CallRingZeroProc 4     ; function 4 - Get EMS & XMS Limits


   les si, lpdwRval

   mov eax, dwEMSMin
   mov es:[si + 8], eax

   mov eax, dwEMSMax
   mov es:[si + 0cH], eax

   mov eax, dwXMSMin
   mov es:[si + 10H], eax

   mov eax, dwXMSMax
   mov es:[si + 14H], eax

   mov eax, dwHMAEnabled
   mov es:[si + 18H], eax


   ;** Get 'time slice' information **

   CallRingZeroProc 5     ; function 5 - Get VM's 'time slice' info

   les si, lpdwRval

   mov es:[si + 1cH], eax ; Exclusive/Background FLAGS (see Get_Time_Slice_Priority)

   mov es:[si + 20H], ecx ; HIGH WORD == FG pri; LOW WORD == BG pri

   mov es:[si + 24H], edx ; CPU '%' usage (calculated by VMM)


   CallRingZeroProc 8     ; function 8 - Get VM's page count info

   mov es:[si + 28H], eax ; total # of pages (mapped + non-mapped)
   mov es:[si + 2cH], edx ; total # of un-mapped pages


   CallRingZeroProc 14    ; function 14 - Get VM's total CPU usage info

   mov es:[si + 30H], eax ; total VM execution time

   mov ax, hWinOldAp      ; return the window handle


   RET


MyGetVMInfo ENDP


MyGetVMPgCount PROC FAR16 PASCAL PUBLIC USES ES SI,
               dwVMHandle:DWORD, lpdwRval:FAR16 PTR DWORD


   mov ebx, dwVMHandle
   mov ReturnVMHandle, ebx        ; store here - ring 0 procs use it

   CallRingZeroProc 8     ; get the VM's page count info

   les si, lpdwRval

   mov es:[si], eax       ; total # of pages (mapped + non-mapped)
   mov es:[si + 4], edx   ; total # of un-mapped pages

   RET

MyGetVMPgCount ENDP


MyGetSysPageCount PROC FAR16 PASCAL PUBLIC USES ES SI


   CallRingZeroProc 9     ; get the System page count info

   RET

MyGetSysPageCount ENDP


MyGetDemandPageInfo PROC FAR16 PASCAL PUBLIC USES es si di,
                    lpDI:FAR16 PTR DemandInfoStruc


;;   CallRingZeroProc 7     ; function 7 - 'GetDemandPageInfo'

   cld

   mov cx, SIZE DemandInfoStruc  ; copy LOCAL structure to DEST struct!
   mov si, OFFSET DGROUP:pDI
   les di, lpDI
   rep movsb

   RET


MyGetDemandPageInfo ENDP



MyGetPageDirectory PROC FAR16 PASCAL PUBLIC, lpPageBuf:FAR16 PTR DWORD


   INVOKE GetSelectorBase, WORD PTR lpPageBuf + 2

   shl edx, 10H
   mov dx, ax             ; 'edx' contains SELECTOR BASE for buffer

   movzx eax, WORD PTR lpPageBuf

   add eax, edx           ; ABSOLUTE offset for buffer!

   mov dwPageBuf, eax     ; store offset here...

   CallRingZeroProc 10    ; function 10 - 'GetPageDirectory'
                          ; IMPORTANT!  eax contains its linear address
   mov edx, eax
   shr edx, 10H

   RET


MyGetPageDirectory ENDP



MyGetPageTable PROC FAR16 PASCAL PUBLIC,
               dwPageDirEntry:DWORD, lpPageBuf:FAR16 PTR DWORD


   INVOKE GetSelectorBase, WORD PTR lpPageBuf + 2

   shl edx, 10H
   mov dx, ax             ; 'edx' contains SELECTOR BASE for buffer

   movzx eax, WORD PTR lpPageBuf

   add eax, edx           ; ABSOLUTE offset for buffer!

   mov dwPageBuf, eax     ; store offset here...

   mov eax, dwPageDirEntry
   mov dwPageNum, eax     ; store 'Page Directory Entry' value here!

   CallRingZeroProc 11    ; function 11 - 'GetPageTable'
                          ; IMPORTANT!  eax contains its linear address
   mov edx, eax
   shr edx, 10H

   RET


MyGetPageTable ENDP


MyGetVXDList PROC FAR16 PASCAL PUBLIC, lpVXDBuf:FAR16 PTR DWORD

   ; THIS PROC RETURNS A LIST OF VxD CONTROL BLOCK ADDRESSES
   ; THE FIRST [0] ENTRY IS THE FIRST 'DeviceInfo' STRUCTURE
   ; THE LAST ENTRY IS A <NULL>


   INVOKE GetSelectorBase, WORD PTR lpVXDBuf + 2

   shl edx, 10H
   mov dx, ax             ; 'edx' contains SELECTOR BASE for buffer

   movzx eax, WORD PTR lpVXDBuf

   add eax, edx           ; ABSOLUTE offset for buffer!

   mov dwPageBuf, eax     ; store offset here...


   CallRingZeroProc 15    ; function 15 - 'GetVXDList'


   RET

MyGetVXDList ENDP


MyGetVMMDDB PROC FAR16 PASCAL PUBLIC

   ; THIS PROC RETURNS THE DDB FOR 'VMM' IN DX:AX


   CallRingZeroProc 16    ; function 16 - 'GetVMMDDB'

   mov eax, ReturnDDBAddr ; return value guaranteed correct HERE

   mov edx, eax
   shr edx, 16            ; result in eax and dx:ax


   RET

MyGetVMMDDB ENDP



;MyEnable87 PROC FAR16 PASCAL PUBLIC USES es bx, bEnable:WORD
;
;
;   .IF bEnable != 0
;
;      CallRingZeroProc 12       ; enable math co-processor
;
;      mov ax, SEG __WinFlags
;      mov bx, OFFSET __WinFlags
;      mov es, ax
;
;      or WORD PTR es:[bx], WF_80x87      ; set the MATH COPROCESSOR bit in WINFLAGS
;
;   .ELSE
;
;      CallRingZeroProc 13       ; disable math co-processor
;
;      mov ax, SEG __WinFlags
;      mov bx, OFFSET __WinFlags
;      mov es, ax
;
;      and WORD PTR es:[bx], NOT WF_80x87 ; clear the MATH COPROCESSOR bit in WINFLAGS
;
;   .ENDIF
;
;
;
;MyEnable87 ENDP




_HELPERCODE ENDS






CODE32_TEXT SEGMENT

   ASSUME DS:NOTHING, ES:NOTHING, FS:DGROUP, GS:NOTHING, SS:NOTHING

RingZeroProc PROC FAR32 PASCAL USES DS ES ESI EDI, dwParm:DWORD

   mov ds, ax                    ; on entry, ax == NEW DS for 32 bit!
   mov es, ax                    ; load ES with same segment (why not!)

   mov eax, dwParm               ; let's see which function I do...


;   push eax                                  ; just in case, save regs...
;   push fs
;   push gs

   VMMcall Begin_Nest_Exec                   ; prevents 'CRASH' problems...

;   pop gs                                    ; restore regs
;   pop fs
;   pop eax


   .IF eax==0

      INVOKE GetSystemVM32

   .ELSEIF eax==1

      INVOKE GetNextVM32

   .ELSEIF eax==2

      INVOKE MyCopyPageTable32

   .ELSEIF eax==3

      INVOKE GetVMInfo32

   .ELSEIF eax==4

      INVOKE GetEMSXMSLimits32

   .ELSEIF eax==5

      INVOKE GetTimeSlicePri32

   .ELSEIF eax==6

      INVOKE GetPgStatus32

   .ELSEIF eax==7

      INVOKE GetDemandPageInfo32

   .ELSEIF eax==8

      INVOKE GetVMPgCount32

   .ELSEIF eax==9

      INVOKE GetSysPageCount32

   .ELSEIF eax==10

      INVOKE GetPageDirectory32

   .ELSEIF eax==11

      INVOKE GetPageTable32

   .ELSEIF eax==12               ; this entry reserved (for later)

;      INVOKE Enable87_32
      stc

   .ELSEIF eax==13               ; this entry reserved (for later)

;      INVOKE Disable87_32
      stc

   .ELSEIF eax==14

       INVOKE GetVMExecTime32

   .ELSEIF eax==15

       INVOKE GetVXDList32

   .ELSEIF eax==16

       INVOKE GetVMMDDB32

   .ELSE

      stc                        ; carry set (error?)

   .ENDIF


;   push eax                      ; temporarily save regs
;   push edx

   VMMcall End_Nest_Exec         ; this 'un-does' the 'Begin_Nest_Exec'

;   pop edx                       ; restore regs
;   pop eax


   RET                           ; back to where I came from!!
                                 ; (note - 'ret' must clean up stack!)

RingZeroProc ENDP


GetSystemVM32 PROC FAR32 PASCAL

   mov ebx, 0

   VMMcall Get_Sys_VM_Handle

   mov FS:ReturnVMHandle, ebx

   clc

   RET

GetSystemVM32 ENDP



GetNextVM32   PROC FAR32 PASCAL

   mov ebx, FS:ReturnVMHandle    ; previous handle in STATIC data!

   VMMcall Get_Next_VM_Handle    ;** NOTE:  List is CIRCULAR **

   mov FS:ReturnVMHandle, ebx

   clc

   RET

GetNextVM32   ENDP



MyCopyPageTable32 PROC FAR32 PASCAL

   VMMcall Get_Sys_VM_Handle     ; load 'ebx' with SYS VM handle

   VMMcall _CopyPageTable, <FS:dwPageNum, FS:dwNPages, FS:dwPageBuf, 0>

   clc

   RET

MyCopyPageTable32 ENDP


GetVMInfo32 PROC FAR32 PASCAL

   VxDcall SHELL_Get_Version

   .IF ax < 30aH

      mov eax, 0
      mov ecx, 0

      stc

   .ELSE

      mov ebx, FS:ReturnVMHandle    ; parameter VM handle in STATIC data!

      VxDcall SHELL_GetVMInfo

      clc

   .ENDIF


   RET


GetVMInfo32 ENDP


GetEMSXMSLimits32 PROC FAR32 PASCAL USES esi edi


   mov ebx, FS:ReturnVMHandle    ; parameter VM handle in STATIC data!

   VxDcall V86MMGR_Get_EMS_XMS_Limits

   mov FS:dwEMSMin, eax           ; EMS low limit
   mov FS:dwEMSMax, edx          ; EMS high limit
   mov FS:dwXMSMin, esi           ; XMS low limit
   mov FS:dwXMSMax, edi          ; XMS high limit
   mov FS:dwHMAEnabled, ecx       ; HMA Enabled/Disabled (1 == Enabled)

   RET

GetEMSXMSLimits32 ENDP


GetTimeSlicePri32  PROC FAR32 PASCAL USES esi edi

   mov ebx, FS:ReturnVMHandle    ; parameter VM handle in STATIC data!

   VMMcall Get_Time_Slice_Priority

   shl ecx, 10H                  ; FG priority is 'cx'; place in high word
   mov cx, dx                    ; place BG priority into 'cx'
                                 ; ECX == FG:BG priority (hi word, lo word)
   mov edx, esi                  ; CPU '%' usage

   RET


GetTimeSlicePri32  ENDP


GetPgStatus32 PROC FAR32 PASCAL

   VxDcall V86MMGR_Get_Version

   .IF ax < 30aH

      mov eax, 0

      stc

   .ELSE

      mov ebx, FS:ReturnVMHandle    ; parameter VM handle in STATIC data!
                                    ; if ZERO, gets 'global' information

      mov ecx, FS:dwPageNum         ; the page # for which to get info
                                    ; NOTE:  DOCS limit to pages 0-10FH

      VxDcall V86MMGR_GetPgStatus


   .ENDIF


   RET


GetPgStatus32 ENDP


GetDemandPageInfo32 PROC FAR32 PASCAL USES ebx esi edi ebp

   ;** Step 1:  Get current VM handle! **



   VMMcall Get_Cur_VM_Handle                 ; place VM handle in ebx
   mov ebp, [ebx.CB_Client_Pointer]          ; client pointer in EBP first

   mov esi, 0
   mov si, fs

   VMMcall _SelectorMapFlat, <ebx, esi, 0>   ; return correct FLAT DS offset
                                             ; for the DGROUP segment.

   add eax, OFFSET DGROUP:pDI                ; TRUE linear address! Yay!
   mov edi, eax                              ; attempt to prevent 'bug'


   mov eax, DWORD PTR FS:pDI
   .IF eax == DWORD PTR [edi]                ; alias verification, eh?

      xor eax, -1
      mov DWORD PTR FS:pDI, eax


      .IF eax == DWORD PTR [edi]


         VMMcall _GetDemandPageInfo, <edi, 0>      ; Do it!  Yes!  Result saved!


      .ENDIF

   .ENDIF



   RET

GetDemandPageInfo32 ENDP


GetVMPgCount32   PROC FAR32 PASCAL

   mov ebx, FS:ReturnVMHandle    ; desired VM handle in STATIC data!

   VMMcall _GetVMPgCount, <ebx, 0>
                                 ; edx:eax contains return value

   RET

GetVMPgCount32   ENDP


GetSysPageCount32   PROC FAR32 PASCAL

;   LOCAL dwOldReEntrantFlag:DWORD, dwRval:DWORD


;   VMMcall Begin_Reentrant_Execution         ; prevents 'CRASH' problems...
;   mov dwOldReEntrantFlag, ecx               ; result from previous call

   VMMcall _GetSysPageCount, <0>

;   mov dwRval, eax                           ; store it temporarily
;   mov ecx, dwOldReEntrantFlag
;   VMMcall End_Reentrant_Execution           ; prevents 'CRASH' problems...
;   mov eax, dwRval                           ; in case above messes up regs

   RET

GetSysPageCount32   ENDP


GetPageDirectory32 PROC FAR32 PASCAL USES ES ESI EDI

  ;** This proc is SPECIAL - it uses the page registers/tables directly! **


  mov eax, cr3              ; get page table PHYSICAL address!!
  VMMcall _MapPhysToLinear, <eax, 1000H, 0>
                            ; map this to a LINEAR address...
  push eax                  ; save it in 'eax' for return value
  mov esi, eax              ; source offset from DS
  mov edi, FS:dwPageBuf

  cld
  mov ecx, 400H             ; 1024 DWORD entries in directory!!

  push ds
  pop es                    ; ensure ES == DS

  rep movs DWORD PTR es:[edi], DWORD PTR ds:[esi]   ; copy table!  How nice!

  pop eax                   ; return the linear address in 'eax'

  RET

GetPageDirectory32 ENDP


GetPageTable32 PROC FAR32 PASCAL USES ES ESI EDI

  ;** This proc is SPECIAL - it uses the page registers/tables directly! **

  mov eax, FS:dwPageNum     ; 'PAGE DIRECTORY' entry value!  Better be right!
  and eax, 0fffff000H       ; trim off low 12 bits for PAGE TABLE PHYSICAL
                            ; memory address.
  push gs
  push fs                   ; am I losing my segment registers??

  VMMcall _MapPhysToLinear, <eax, 1000H, 0>
                            ; map this to a LINEAR address...
  pop fs
  pop gs
  push eax                  ; save linear address in 'eax' for return value

  mov esi, eax              ; source offset from DS
  mov edi, FS:dwPageBuf

  cld
  mov ecx, 400H             ; 1024 DWORD entries in table!!

  push ds
  pop es                    ; ensure ES == DS

  rep movs DWORD PTR es:[edi], DWORD PTR ds:[esi]   ; copy table!  How nice!

  pop eax                   ; return the linear address in 'eax'

  RET

GetPageTable32 ENDP



;Enable87_32 PROC FAR32 PASCAL USES ES ESI EDI
;
;
;   VMMcall Get_Cur_VM_Handle   ; places current VM handle into EBX
;
;   mov al, 1                   ; EM bit reset, MP bit set
;   VxDcall VMCPD_Set_Virt_State
;
;   RET
;
;Enable87_32 ENDP
;
;
;
;Disable87_32 PROC FAR32 PASCAL USES ES ESI EDI
;
;
;   VMMcall Get_Cur_VM_Handle   ; places current VM handle into EBX
;
;   mov al, 2                   ; EM bit set, MP bit reset
;   VxDcall VMCPD_Set_Virt_State
;
;   RET
;
;Disable87_32 ENDP


GetVMExecTime32 PROC FAR32 PASCAL


   mov ebx, FS:ReturnVMHandle    ; parameter VM handle in STATIC data!
                                 ; (I'm not sure if it works properly)

   VMMcall Get_Last_Updated_VM_Exec_Time
                                 ; return value remains in 'eax'

   RET

GetVMExecTime32 ENDP



OldGetVXDList32 PROC FAR32 PASCAL

   ; ON ENTRY dwPageBuf -> DWORD array

   push esi
   push edi

   VxDcall VXDLDR_GetDeviceList

   mov esi, eax          ; pointer to DeviceInfo structures
   mov edi, FS:dwPageBuf


   ASSUME esi:PTR DeviceInfo
   ASSUME edi:PTR DWORD

   mov [edi], esi        ; initial entry is ptr to 1st DeviceInfo struct
   add edi, 4

   ; GO THROUGH A LOOP to get the list of VxD's


   .WHILE esi != 0

      mov eax, [esi].DI_DDB
      mov [edi], eax

      mov esi, [esi].DI_Next
      add edi, 4

   .ENDW

   mov [edi], 0

   ASSUME esi:NOTHING
   ASSUME edi:NOTHING

   pop edi
   pop esi

   RET

OldGetVXDList32 ENDP


GetVXDList32 PROC FAR32 PASCAL

   ; ON ENTRY dwPageBuf -> DWORD array

   push esi
   push edi
   push ecx

   VxDcall VXDLDR_GetDeviceList

   mov edi, FS:dwPageBuf

   mov [edi], eax        ; initial entry is ptr to 1st DeviceInfo struct
   add edi, 4

   ; get the DDB for 'VMM' which should be the first one

   mov eax, VMM_DEVICE_ID  ; device ID for VMM
   mov edx, 0              ; not used - store 0 in it
   VMMcall Get_DDB
   mov esi, ecx

   ASSUME esi:PTR VxD_Desc_Block
   ASSUME edi:PTR DWORD

   ; GO THROUGH A LOOP to get the list of VxD's


   .WHILE esi != 0

      mov [edi], esi

      mov esi, [esi].DDB_Next
      add edi, 4

   .ENDW

   mov [edi], 0

   ASSUME esi:NOTHING
   ASSUME edi:NOTHING

   pop ecx
   pop edi
   pop esi

   RET

GetVXDList32 ENDP


GetVMMDDB32 PROC FAR32 PASCAL

   push ecx

   ; get the DDB for 'VMM' which should be the first one

   mov eax, VMM_DEVICE_ID  ; device ID for VMM
   mov edx, 0              ; not used - store 0 in it
   VMMcall Get_DDB

   mov FS:ReturnDDBAddr, ecx
   mov eax, ecx
   pop ecx


   ; return value is in EAX for VMM's DDB

   RET

GetVMMDDB32 ENDP



CODE32_TEXT ENDS








END
