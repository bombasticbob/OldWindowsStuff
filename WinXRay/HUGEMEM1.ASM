PAGE 58,132
;***************************************************************************
;*                                                                         *
;*            Windows Database Utility Library - WDBUTIL.DLL               *
;*                                                                         *
;*         (c) 1990,1991 by R. E. Frazier - All rights reserved            *
;*                                                                         *
;*                                                                         *
;***************************************************************************
;***************************************************************************
;*                                                                         *
;*       Windows Multi-Thread Manager Utility Library - WMTHREAD.DLL       *
;*                                                                         *
;*         (c) 1990,1991 by R. E. Frazier - All rights reserved            *
;*                                                                         *
;*                                                                         *
;***************************************************************************

.286p

.XLIST
.XCREF
INCLUDE WINDOWS.INC
.LIST
.CREF



_hmemcmp  PROTO FAR C, :DWORD, :DWORD, :DWORD
_hmemcpy  PROTO FAR C, :DWORD, :DWORD, :DWORD
_hmemswap PROTO FAR C, :DWORD, :DWORD, :DWORD
_hmemset  PROTO FAR C, :DWORD, :WORD, :DWORD

_DATA SEGMENT BYTE PUBLIC 'DATA'
_DATA ENDS

DGROUP GROUP _DATA

EXTERNDEF C _ok_to_use_386_instructions_:WORD, C _ahincr:ABS


HMEM_TEXT SEGMENT PARA USE16 PUBLIC 'CODE'

ASSUME CS:HMEM_TEXT, DS:DGROUP


;***************************************************************************
;*                                                                         *
;*      int FAR _cdecl _hmemcmp(void _huge *, void _huge *, DWORD)         *
;*                                                                         *
;***************************************************************************

ALIGN 4

.286p

_hmemcmp PROC FAR C PUBLIC USES ds es si di, lp1:DWORD, lp2:DWORD, dwSize:DWORD


   mov ax, _ok_to_use_386_instructions_  ; can I use '386' instructions?
   or ax,ax
   jne hmemcmp_386
   jmp hmemcmp_no386


.386p

ALIGN 4

hmemcmp_386:                   ; use this code if '386

   mov bx, _ahincr

   lds SI, lp1
   les DI, lp2

   mov ECX, DWORD PTR dwSize
   or ECX,ECX
   jz hmemcmp_386_done 
   jmp hmemcmp_386_loop0


   ALIGN 4
hmemcmp_386_loop:         ; SECTION TO INCREMENT SI,DI

   inc DI
   jnz hmemcmp_386_loop2

   mov ax, es
   add ax, bx             ; add _ahincr
   mov es, ax             ; new segment for destination

   ALIGN 4
hmemcmp_386_loop2:

   inc SI
   jnz hmemcmp_386_loop0

   mov ax, ds
   add ax, bx             ; add _ahincr
   mov ds, ax             ; new segment for destination

   ALIGN 4
hmemcmp_386_loop0:        ; ACTUAL LOOP BEGINS HERE

   mov al, BYTE PTR ES:[DI]
   cmp BYTE PTR [SI], al

   jne hmemcmp_386_done

   loopd hmemcmp_386_loop

   xor ax,ax              ; sets the 'z' flag - they are equal!

hmemcmp_386_done:


   jmp hmemcmp_end        ; I am done here - go to end


.286p

ALIGN 4

hmemcmp_no386:

   mov bx, _ahincr

   lds SI, lp1
   les DI, lp2

   mov CX, WORD PTR dwSize
   mov DX, WORD PTR dwSize + 2
   push dx
   or dx, cx
   pop dx
   jz hmemcmp_no386_done      ; zero length
   jmp hmemcmp_no386_loop0


   ALIGN 4
hmemcmp_no386_loop:


   inc DI
   jnz hmemcmp_no386_loop2

   mov ax, es
   add ax, bx             ; add _ahincr
   mov es, ax             ; new segment for destination

   ALIGN 4
hmemcmp_no386_loop2:

   inc SI
   jnz hmemcmp_no386_loop0

   mov ax, ds
   add ax, bx             ; add _ahincr
   mov ds, ax             ; new segment for destination

   ALIGN 4
hmemcmp_no386_loop0:

   mov al, BYTE PTR ES:[DI]
   cmp BYTE PTR [SI], al

   jne hmemcmp_no386_done

   loop hmemcmp_no386_loop
   or dx,dx               ; is 'dx' also zero?
   jz hmemcmp_no386_loop4

   dec dx                 ; decrement the 'high' word
   jmp hmemcmp_no386_loop ; and continue looping


   ALIGN 4
hmemcmp_no386_loop4:

   xor ax,ax              ; sets the 'z' flag - they are equal!

hmemcmp_no386_done:



hmemcmp_end:

   mov ax, 0              ; pre-load result for 'equal'

   jz hmemcmp_equal       ; if equal, end now.
   jb hmemcmp_below       ; if lp1 < lp2, load a '0ffffH'

   mov ax,1               ; otherwise, load a 1
   jmp hmemcmp_equal

hmemcmp_below:

   mov ax, 0ffffH

hmemcmp_equal:

   ret

_hmemcmp ENDP


;***************************************************************************
;*                                                                         *
;*      int FAR _cdecl _hmemcpy(void _huge *, void _huge *, DWORD)         *
;*                                                                         *
;***************************************************************************

.286p

ALIGN 4

_hmemcpy PROC FAR C PUBLIC USES ds es si di, lp1:DWORD, lp2:DWORD, dwSize:DWORD


   mov ax, _ok_to_use_386_instructions_  ; can I use '386' instructions?
   or ax,ax
   jne hmemcpy_386
   jmp hmemcpy_no386

.386p

ALIGN 4

hmemcpy_386:                   ; use this code if '386

   mov bx, _ahincr

   les DI, lp1         ; destination is the 1st parm...
   lds SI, lp2         ; source is the 2nd parm...

   mov ECX, DWORD PTR dwSize
   or ECX,ECX
   jz hmemcpy_386_done 
   jmp hmemcpy_386_loop0


   ALIGN 4
hmemcpy_386_loop:

   inc DI
   jnz hmemcpy_386_loop2

   mov ax, es
   add ax, bx             ; add _ahincr
   mov es, ax             ; new segment for destination

   ALIGN 4
hmemcpy_386_loop2:

   inc SI
   jnz hmemcpy_386_loop0

   mov ax, ds
   add ax, bx             ; add _ahincr
   mov ds, ax             ; new segment for destination

   ALIGN 4
hmemcpy_386_loop0:

   mov al, BYTE PTR [SI]
   mov BYTE PTR ES:[DI], al

   loopd hmemcpy_386_loop

hmemcpy_386_done:


   jmp hmemcpy_end        ; I am done here - go to end


.286p

ALIGN 4

hmemcpy_no386:

   mov bx, _ahincr

   les DI, lp1         ; destination is the 1st parm...
   lds SI, lp2         ; source is the 2nd parm...

   mov CX, WORD PTR dwSize
   mov DX, WORD PTR dwSize + 2
   push dx
   or dx, cx
   pop dx
   jz hmemcpy_no386_done      ; ZERO LENGTH!
   jmp hmemcpy_no386_loop0


   ALIGN 4
hmemcpy_no386_loop:

   inc DI
   jnz hmemcpy_no386_loop2 ; if result isn't zero,

   mov ax, es
   add ax, bx             ; add _ahincr
   mov es, ax             ; new segment for destination

   ALIGN 4
hmemcpy_no386_loop2:

   inc SI
   jnz hmemcpy_no386_loop0

   mov ax, ds
   add ax, bx             ; add _ahincr
   mov ds, ax             ; new segment for destination

   ALIGN 4
hmemcpy_no386_loop0:

   mov al, BYTE PTR [SI]
   mov BYTE PTR ES:[DI], al

   loop hmemcpy_no386_loop
   or dx,dx               ; is 'dx' also zero?
   jz hmemcpy_no386_done

   dec dx                 ; decrement the 'high' word
   jmp hmemcpy_no386_loop ; and continue looping


hmemcpy_no386_done:



hmemcpy_end:

   ret

_hmemcpy ENDP



;***************************************************************************
;*                                                                         *
;*      int FAR _cdecl _hmemswap(void _huge *, void _huge *, DWORD)        *
;*                                                                         *
;***************************************************************************

.286p

ALIGN 4

_hmemswap PROC FAR C PUBLIC USES ds es si di, lp1:DWORD, lp2:DWORD, dwSize:DWORD


   mov ax, DGROUP:_ok_to_use_386_instructions_  ; can I use '386' instructions?
   or ax,ax
   jne hmemswap_386
   jmp hmemswap_no386

.386p

ALIGN 4

hmemswap_386:                   ; use this code if '386

   mov bx, _ahincr

   lds SI, lp1
   les DI, lp2

   mov ECX, DWORD PTR dwSize
   or ECX,ECX
   jz hmemswap_386_done         ; ZERO LENGTH!
   jmp hmemswap_386_loop0


   ALIGN 4
hmemswap_386_loop:

   inc DI
   jnz hmemswap_386_loop2

   mov ax, es
   add ax, bx             ; add _ahincr
   mov es, ax             ; new segment for destination

   ALIGN 4
hmemswap_386_loop2:

   inc SI
   jnz hmemswap_386_loop0

   mov ax, ds
   add ax, bx             ; add _ahincr
   mov ds, ax             ; new segment for destination

   ALIGN 4
hmemswap_386_loop0:

   mov al,  BYTE PTR [SI]
   xchg al, BYTE PTR es:[DI]
   mov BYTE PTR [SI], al

   loopd hmemswap_386_loop

hmemswap_386_done:


   jmp hmemswap_end        ; I am done here - go to end


.286p

ALIGN 4

hmemswap_no386:

   mov bx, _ahincr

   lds SI, lp1
   les DI, lp2

   mov CX, WORD PTR dwSize
   mov DX, WORD PTR dwSize + 2
   push dx
   or dx, cx
   pop dx
   jz hmemswap_no386_done      ; ZERO LENGTH!
   jmp hmemswap_no386_loop0


   ALIGN 4
hmemswap_no386_loop:

   inc DI
   jnz hmemswap_no386_loop2 ; if result isn't zero,

   mov ax, es
   add ax, bx             ; add _ahincr
   mov es, ax             ; new segment for destination

   ALIGN 4
hmemswap_no386_loop2:

   inc SI
   jnz hmemswap_no386_loop0

   mov ax, ds
   add ax, bx             ; add _ahincr
   mov ds, ax             ; new segment for destination

   ALIGN 4
hmemswap_no386_loop0:

   mov al,  BYTE PTR [SI]
   xchg al, BYTE PTR es:[DI]
   mov BYTE PTR [SI], al

   loop hmemswap_no386_loop
   or dx,dx               ; is 'dx' also zero?
   jz hmemswap_no386_done

   dec dx                 ; decrement the 'high' word
   jmp hmemswap_no386_loop ; and continue looping


hmemswap_no386_done:



hmemswap_end:

   ret

_hmemswap ENDP



;***************************************************************************
;*                                                                         *
;*          int FAR _cdecl _hmemset(void _huge *, WORD, DWORD)             *
;*                                                                         *
;***************************************************************************


.286p

ALIGN 4

_hmemset PROC FAR C PUBLIC USES ds es si di, lp1:DWORD, wValue:WORD, dwSize:DWORD


   mov ax, DGROUP:_ok_to_use_386_instructions_  ; can I use '386' instructions?
   or ax,ax
   jne hmemset_386
   jmp hmemset_no386

.386p

ALIGN 4

hmemset_386:                   ; use this code if '386

   mov bx, _ahincr

   les DI, lp1

   mov ECX, DWORD PTR dwSize
   or ECX,ECX
   jz hmemset_386_done         ; ZERO LENGTH!

   mov al, BYTE PTR wValue

   jmp hmemset_386_loop0


   ALIGN 4
hmemset_386_loop:

   inc DI
   jnz hmemset_386_loop0

   mov ax, es
   add ax, bx             ; add _ahincr
   mov es, ax             ; new segment for destination
   mov al, BYTE PTR wValue

   ALIGN 4
hmemset_386_loop0:

   mov BYTE PTR ES:[DI], al

   loopd hmemset_386_loop

hmemset_386_done:


   jmp hmemset_end        ; I am done here - go to end


.286p

ALIGN 4

hmemset_no386:

   mov bx, _ahincr

   les DI, lp1

   mov CX, WORD PTR dwSize
   mov DX, WORD PTR dwSize + 2
   push dx
   or dx, cx
   pop dx
   jz hmemset_no386_done      ; ZERO LENGTH!

   mov al, BYTE PTR wValue

   jmp hmemset_no386_loop0


   ALIGN 4
hmemset_no386_loop:

   inc DI
   jnz hmemset_no386_loop0 ; if result isn't zero,

   mov ax, es
   add ax, bx             ; add _ahincr
   mov es, ax             ; new segment for destination
   mov al, BYTE PTR wValue

   ALIGN 4
hmemset_no386_loop0:

   mov BYTE PTR ES:[DI], al

   loop hmemset_no386_loop
   or dx,dx               ; is 'dx' also zero?
   jz hmemset_no386_done

   dec dx                 ; decrement the 'high' word
   jmp hmemset_no386_loop ; and continue looping


hmemset_no386_done:



hmemset_end:

   ret

_hmemset ENDP


HMEM_TEXT ENDS


END
