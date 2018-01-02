;************************************************************************
;**
;**           Copyright (c) 1993,1994 by Robert E. Frazier
;**                   and Stewart~Frazier Tools, Inc.
;**             all rights reserved.  used by permission.
;**
;************************************************************************
;**
;** IMPORTANT:  'MyAllocSelector' invokes DPMI to allocate
;**             a selector for the LDT.  'MyFreeSelector' 
;**             invokes DPMI to free the same selector.
;**             'MyChangeSelector' uses DPMI to assign the
;**             offset and limit (16-bit) to a selector.
;**             The '2'-suffixed versions bypass DPMI's checks
;**             on selector contents to update them directly.
;**             You should be able to duplicate the same
;**             functionality for the DPMI calls.  If not, 
;**             let me know and I'll send the 'C' code.
;**
;**  Please do not reprint or distribute this code at this time.  If
;**  you plan on using it or portions of it in any articles please let
;**  me know first.  I don't think it would be a problem, but I would
;**  at least want to make sure that the copyright notice appears in
;**  some sort of note like 'Portions of this code are copyright ...'
;**
;************************************************************************



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


;** sample 32-bit ring 0 proc **

RingZeroProc       PROTO FAR32 PASCAL, dwParm:DWORD
GetSystemVM32      PROTO FAR32 PASCAL




; BOOL FAR PASCAL MyChangeSelector2(WORD wSel, DWORD dwOffs, DWORD dwLim,
;                                   WORD wFlags);

;** PUBLIC DEFINITIONS - LABELS AND PROCS **


PUBLIC PASCAL wCode32Alias, PASCAL wData32Alias
PUBLIC PASCAL wCallGateSeg, PASCAL wCode32Seg ; , PASCAL wData32Seg


MyCreateCallGate   PROTO FAR16 PASCAL, :WORD, :DWORD, :WORD, :WORD
MyAssignDescriptor PROTO FAR16 PASCAL, :WORD, :FAR16 PTR DESCRIPTOR



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

;                  **************************************
;                  ** DATA SEGMENTS - STATIC VARIABLES **
;                  **************************************




MYDATA16 SEGMENT

wCode32Alias    WORD 0   ;** selector for CODE32_TEXT alias (FLAT) **
wData32Alias    WORD 0   ;** selector for DATA32_DATA alias (FLAT) **


lpRingZeroProc  LABEL FWORD
                DWORD 0
wCallGateSeg    WORD 0    ;** selector for Call Gate **

ReturnVMHandle  DWORD 0


wCode32Seg       WORD 0   ; use as flag; if NOT ZERO it's PAGE LOCKED!



MYDATA16 ENDS






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



MyGetDescriptor PROC FAR16 PASCAL USES bx cx si di,\
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



MyAssignDescriptor PROC FAR16 PASCAL USES bx cx si di,\
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



MyGetAccessRights PROC FAR16 PASCAL, wSelector:WORD

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



MyGetSelectorBase PROC FAR16 PASCAL, wSelector:WORD

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



MyGetSelectorLimit PROC FAR16 PASCAL, wSelector:WORD

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



MyCreateCallGate  PROC FAR16 PASCAL USES di bx es,\
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



MyChangeSelector2 PROC FAR16 PASCAL USES bx cx, \
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

   mov eax, OFFSET RingZeroProc
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


;** The following is a short example of code that uses **
;** a call gate to accomplish something.               **


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



_HELPERCODE ENDS





CODE32_TEXT SEGMENT

   ASSUME DS:NOTHING, ES:NOTHING, FS:DGROUP, GS:NOTHING, SS:NOTHING

RingZeroProc PROC FAR32 PASCAL USES DS ES ESI EDI, dwParm:DWORD

   mov ds, ax                    ; on entry, ax == NEW DS for 32 bit!
   mov es, ax                    ; load ES with same segment (why not!)

   mov eax, dwParm               ; let's see which function I do...

   VMMcall Begin_Nest_Exec                   ; prevents 'CRASH' problems...

   .IF eax==0

      INVOKE GetSystemVM32

   .ELSE

      stc                        ; carry set (error?)

   .ENDIF

   VMMcall End_Nest_Exec         ; this 'un-does' the 'Begin_Nest_Exec'

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

CODE32_TEXT ENDS








END
