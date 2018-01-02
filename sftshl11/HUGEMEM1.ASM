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


USE_32BIT_OFFSETS = 1    ; uses 32-bit offsets rather than tiling selectors


_hmemcmp  PROTO FAR C, :DWORD, :DWORD, :DWORD
_hmemcpy  PROTO FAR C, :DWORD, :DWORD, :DWORD
_hmemswap PROTO FAR C, :DWORD, :DWORD, :DWORD
_hmemset  PROTO FAR C, :DWORD, :WORD, :DWORD


_DATA SEGMENT BYTE PUBLIC 'DATA'
; extern unsigned char __near __cdecl _ctype[];

EXTERNDEF C _ctype: BYTE

_DATA ENDS


DGROUP GROUP _DATA


_UPPER    = 1      ;/* upper case letter */
_LOWER    = 2      ;/* lower case letter */
_DIGIT    = 4      ;/* digit[0-9] */
_SPACE    = 8      ;/* tab, carriage return, newline, */
                   ;/* vertical tab or form feed */
_PUNCT    = 10H    ;/* punctuation character */
_CONTROL  = 20H    ;/* control character */
_BLANK    = 40H    ;/* space char */
_HEX      = 80H    ;/* hexadecimal digit */



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

IF USE_32BIT_OFFSETS

   mov esi, 0
   mov edi, 0

   lds SI, lp1       ; CMPS subtracts DEST from SRC! (backwards)
   les DI, lp2

   mov ECX, DWORD PTR dwSize
   or ECX,ECX
   jz hmemcmp_386_done

   cld
   repz cmps BYTE PTR ds:[esi], BYTE PTR es:[edi]

ELSE

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

ENDIF

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


_hmemmove_entry_point:  ; this is used by '_hmemmove' (below)

   mov ax, _ok_to_use_386_instructions_  ; can I use '386' instructions?
   or ax,ax
   jne hmemcpy_386
   jmp hmemcpy_no386

.386p

ALIGN 4

hmemcpy_386:                   ; use this code if '386

IF USE_32BIT_OFFSETS

   mov esi, 0
   mov edi, 0

   les DI, lp1         ; destination is the 1st parm...
   lds SI, lp2         ; source is the 2nd parm...

   mov ECX, DWORD PTR dwSize
   or ECX,ECX
   jz hmemcpy_386_done 

   cld
   push ecx
   and ecx, 0fffffffcH          ; trim off lowest 2 bits - total # of DWORD's
   jz hmemcpy_386_byte

   shr ecx, 2                   ; change 'ecx' to DWORD count

   rep movs DWORD PTR es:[edi], ds:[esi]

hmemcpy_386_byte:               ; what's left?  0, 1, 2, or 3 bytes

   pop ecx

   .IF ecx & 2
      movs WORD PTR es:[edi], ds:[esi]
   .ENDIF

   .IF ecx & 1
      movs BYTE PTR es:[edi], ds:[esi]
   .ENDIF

ELSE

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

ENDIF

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
;*      int FAR _cdecl _hmemmove(void _huge *, void _huge *, DWORD)         *
;*                                                                         *
;***************************************************************************

.286p

ALIGN 4

_hmemmove PROC FAR C PUBLIC USES ds es si di, lp1:DWORD, lp2:DWORD, dwSize:DWORD


   ; assume that if lp1 and lp2 have different selectors that they
   ; compare according to the 'tiling' method...

   mov ax, WORD PTR lp2
   mov dx, WORD PTR lp2 + 2

   .IF dx > WORD PTR lp1 + 2 || \
       (dx == WORD PTR lp1 + 2 && ax >= WORD PTR lp1)

      ; 'lp2' is "greater than or equal to" 'lp1'

      jmp _hmemmove_entry_point   ; normal '_hmemcpy()' operation is ok!

   .ENDIF

   ; see if 'lp1' and 'lp2' *REALLY* overlap by adding 'dwSize' to
   ; the 'tiled selector' pointer 'lp2', and comparing to lp1 again.
   ; if 'lp2' is LESS THAN OR EQUAL TO 'lp1', they don't overlap

   add ax, WORD PTR dwSize
   .IF CARRY?
      add dx, _ahincr
   .ENDIF

   mov bx, WORD PTR dwSize + 2
   imul bx, _ahincr
   add dx, bx

   .IF dx < WORD PTR lp1 + 2 || \
       (dx == WORD PTR lp1 + 2 && ax <= WORD PTR lp1)

      ; 'lp2' is "less than or equal to" 'lp1'

      jmp _hmemmove_entry_point   ; they do NOT overlap!

   .ENDIF


   ;** forced to do a "MOVE" operation **


   mov ax, _ok_to_use_386_instructions_  ; can I use '386' instructions?
   or ax,ax
   jne hmemmove_386
   jmp hmemmove_no386

.386p

ALIGN 4

hmemmove_386:                   ; use this code if '386

IF USE_32BIT_OFFSETS

   mov esi, 0
   mov edi, 0

   les DI, lp1         ; destination is the 1st parm...
   lds SI, lp2         ; source is the 2nd parm...

   mov ECX, DWORD PTR dwSize
   or ECX,ECX
   jz hmemmove_386_done

   add EDI, dwSize     ; initially offset them by the length
   add ESI, dwSize

   std
   push ecx
   and ecx, 0fffffffcH          ; trim off lowest 2 bits - total # of DWORD's
   jz hmemmove_386_byte

   shr ecx, 2                   ; change 'ecx' to DWORD count

   sub edi, 4
   sub esi, 4                   ; need to "pre-decrement" these

   rep movs DWORD PTR es:[edi], ds:[esi]

   add edi, 4                   ; 'post adjust' them
   add esi, 4

hmemmove_386_byte:               ; what's left?  0, 1, 2, or 3 bytes

   pop ecx

   .IF ecx & 2

      sub edi, 2
      sub esi, 2

      mov ax, ds:[esi]
      mov es:[edi], ax

   .ENDIF

   .IF ecx & 1

      dec edi
      dec esi

      mov al, ds:[esi]
      mov es:[edi], al

   .ENDIF

ELSE

   mov bx, _ahincr

   mov DI, WORD PTR lp1   ; destination is the 1st parm...
   mov SI, WORD PTR lp2   ; source is the 2nd parm...

   mov ECX, DWORD PTR dwSize
   or ECX,ECX
   jz hmemmove_386_done




   add edi, dwSize           ; offset source to the end
   dec edi                   ; and pre-decrement first pointer

   mov eax, esi              ; calculate the adjusted selector....
   and esi, 0ffffH           ; trim upper WORD for 16-bit only offset

   shr eax, 16               ; high WORD of new offset
   imul ax, bx
   add ax, WORD PTR lp2 + 2  ; the current selector's value
   mov ds, ax                ; load 'tiled' selector now


   add esi, dwSize           ; do it again for the destination
   dec esi

   mov eax, edi
   and edi, 0ffffH

   shr eax, 16
   imul ax, bx
   add ax, WORD PTR lp1 + 2
   mov es, ax


   jmp hmemmove_386_loop0


   ALIGN 4
hmemmove_386_loop:

   sub di, 1
   jnc hmemmove_386_loop2

   mov ax, es
   sub ax, bx             ; sub _ahincr
   mov es, ax             ; new segment for destination

   ALIGN 4
hmemmove_386_loop2:

   sub si, 1
   jnc hmemmove_386_loop0

   mov ax, ds
   sub ax, bx             ; sub _ahincr
   mov ds, ax             ; new segment for destination

   ALIGN 4
hmemmove_386_loop0:

   mov al, BYTE PTR [SI]
   mov BYTE PTR ES:[DI], al

   loopd hmemmove_386_loop

ENDIF

hmemmove_386_done:

   jmp hmemmove_end        ; I am done here - go to end


.286p

ALIGN 4

hmemmove_no386:

   mov bx, _ahincr

   mov DI, WORD PTR lp1         ; destination is the 1st parm...
   mov SI, WORD PTR lp2         ; source is the 2nd parm...

   mov CX, WORD PTR dwSize
   mov DX, WORD PTR dwSize + 2
   push dx
   or dx, cx
   pop dx
   jz hmemmove_no386_done      ; ZERO LENGTH!


   ; point 'ds:si' to end of source, and 'es:di' to end of destination

   mov ax, WORD PTR dwSize + 2
   add si, WORD PTR dwSize     ; offset to the end
   adc ax, 0
   sub si, 1
   sbb ax, 0                   ; ax:dx is now the offset to the end
   imul ax, bx                 ; convert 'ax' to selector tiling offset
   add ax, WORD PTR lp2 + 2    ; the current selector's value
   mov ds, ax                  ; load 'tiled' selector now

   mov ax, WORD PTR dwSize + 2
   add di, WORD PTR dwSize     ; offset to the end
   adc ax, 0
   sub di, 1
   sbb ax, 0                   ; ax:dx is now the offset to the end
   imul ax, bx                 ; convert 'ax' to selector tiling offset
   add ax, WORD PTR lp1 + 2    ; the current selector's value
   mov es, ax                  ; load 'tiled' selector now

   jmp hmemmove_no386_loop0


   ALIGN 4
hmemmove_no386_loop:

   sub di, 1
   jnc hmemmove_no386_loop2 ; if result isn't zero,

   mov ax, es
   sub ax, bx             ; sub _ahincr
   mov es, ax             ; new segment for destination

   ALIGN 4
hmemmove_no386_loop2:

   sub si, 1
   jnc hmemmove_no386_loop0

   mov ax, ds
   sub ax, bx             ; sub _ahincr
   mov ds, ax             ; new segment for destination

   ALIGN 4
hmemmove_no386_loop0:

   mov al, BYTE PTR [SI]
   mov BYTE PTR ES:[DI], al

   loop hmemmove_no386_loop
   or dx,dx               ; is 'dx' also zero?
   jz hmemmove_no386_done

   dec dx                 ; decrement the 'high' word
   jmp hmemmove_no386_loop ; and continue looping


hmemmove_no386_done:



hmemmove_end:

   ret

_hmemmove ENDP



;***************************************************************************
;*                                                                         *
;*      int FAR _cdecl _hmemswap(void _huge *, void _huge *, DWORD)        *
;*                                                                         *
;***************************************************************************

.286p

ALIGN 4

_hmemswap PROC FAR C PUBLIC USES ds es si di, lp1:DWORD, lp2:DWORD, dwSize:DWORD


   mov ax, _ok_to_use_386_instructions_  ; can I use '386' instructions?
   or ax,ax
   jne hmemswap_386
   jmp hmemswap_no386

.386p

ALIGN 4

hmemswap_386:                   ; use this code if '386

IF USE_32BIT_OFFSETS

   mov esi, 0
   mov edi, 0

   lds SI, lp1
   les DI, lp2

   mov ECX, DWORD PTR dwSize
   or ECX,ECX
   jz hmemswap_386_done         ; ZERO LENGTH!

   push ecx
   and ecx, 0fffffffcH          ; trim off lowest 2 bits - total # of DWORD's
   jz hmemswap_386_loop2

   shr ecx, 2                   ; change 'ecx' to DWORD count

hmemswap_386_loop:

   mov eax, DWORD PTR ds:[esi]
   xchg eax, DWORD PTR es:[edi]
   mov DWORD PTR ds:[esi], eax

   add esi, 4
   add edi, 4

   loopd hmemswap_386_loop

hmemswap_386_loop2:

   pop ecx
   .IF ecx & 2                  ; is there a WORD left?

      mov ax, WORD PTR ds:[esi]
      xchg ax, WORD PTR es:[edi]
      mov WORD PTR ds:[esi], ax

      add esi, 2
      add edi, 2

   .ENDIF

   .IF ecx & 1                  ; is there a BYTE left?

      mov al, BYTE PTR ds:[esi]
      xchg al, BYTE PTR es:[edi]
      mov BYTE PTR ds:[esi], al

      inc esi                   ; by convention - why not!
      inc edi                   ; it's a way of ensuring COPYRIGHT protection

   .ENDIF

ELSE

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

ENDIF

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


   mov ax, _ok_to_use_386_instructions_  ; can I use '386' instructions?
   or ax,ax
   jne hmemset_386
   jmp hmemset_no386

.386p

ALIGN 4

hmemset_386:                   ; use this code if '386

IF USE_32BIT_OFFSETS

   mov edi, 0
   les DI, lp1

   mov ECX, DWORD PTR dwSize
   or ECX,ECX
   jz hmemset_386_done         ; ZERO LENGTH!

   mov al, BYTE PTR wValue
   mov ah, al
   pushw ax
   pushw ax
   pop eax                      ; this gives me a DWORD which duplicates the
                                ; value in 'al' 4 times in 'eax'
   cld
   push ecx
   and ecx, 0fffffffcH          ; trim off lowest 2 bits - total # of DWORD's
   jz hmemset_386_byte

   shr ecx, 2                   ; change 'ecx' to DWORD count

   rep stos DWORD PTR es:[edi]

hmemset_386_byte:               ; what's left?  0, 1, 2, or 3 bytes

   pop ecx

   .IF ecx & 2
      stos WORD PTR es:[edi]
   .ENDIF

   .IF ecx & 1
      stos BYTE PTR es:[edi]
   .ENDIF

ELSE

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

ENDIF

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





;****************************************************************************
;
;        'HUGE STRING' FUNCTIONS WHICH PERFORM LIKE '_fstr' FUNCTIONS
;
;****************************************************************************



HSTR_TEXT SEGMENT PARA PUBLIC USE16 'CODE'


hstrlen PROTO FAR C, lpSrc:DWORD
hstrcpy PROTO FAR C, lpDest:DWORD, lpSrc:DWORD



hstrcat PROC FAR C USES es si di, lpDest:DWORD, lpSrc:DWORD


   INVOKE hstrlen, lpDest

   add WORD PTR lpDest, ax
   adc dx, 0                      ; dx is # of '_ahincr' I need to add

   .IF dx != 0

      imul dx, _ahincr
      add WORD PTR lpDest + 2, dx

   .ENDIF

   INVOKE hstrcpy, lpDest, lpSrc

   RET


hstrcat ENDP



hstrcpy PROC FAR C USES es si di, lpDest:DWORD, lpSrc:DWORD


   .IF _ok_to_use_386_instructions_!=0  ; can I use '386' instructions?

.386p

      mov esi, 0
      mov edi, 0

      push fs
      lfs si, lpSrc
      les di, lpDest

      .REPEAT

         mov al, fs:[esi]        ; optimized for 486 - 1 cycle!
         mov es:[edi], al        ; LODSB/STOSB uses 10 cycles

         inc esi                 ; loop time 8 cycles for 386, 4 for 486
         inc edi                 ; (LODSB/STOSB is the same for 386)

      .UNTIL al == 0

      pop fs

.286p

   .ELSE

      mov dx, _ahincr

      lds si, lpSrc
      les di, lpDest

      ;** for now, pick a simple method.  Later I can speed it up if needed

      .REPEAT

         mov al, BYTE PTR [si]
         mov BYTE PTR es:[di], al

         .BREAK .IF al==0

         inc si
         .IF CARRY?
            add WORD PTR lpSrc+2, dx
            mov ds, WORD PTR lpSrc+2
         .ENDIF

         inc di
         .IF CARRY?
            add WORD PTR lpDest+2, dx
            mov es, WORD PTR lpDest+2
         .ENDIF

      .UNTIL 0

   .ENDIF

   mov ax, WORD PTR lpDest
   mov dx, WORD PTR lpDest+2

   RET

hstrcpy ENDP



hstrcmp PROC FAR C USES es si di, lpDest:DWORD, lpSrc:DWORD

   .IF _ok_to_use_386_instructions_!=0  ; can I use '386' instructions?

.386p

      mov esi, 0
      mov edi, 0

      push fs
      lfs si, lpSrc
      les di, lpDest

      .REPEAT

         mov al, fs:[esi]        ; compare dest with source
         cmp es:[edi], al

         .BREAK .IF !ZERO?

         inc esi
         inc edi

      .UNTIL al == 0             ; here it returns 0 if strings equal!

      pop fs

.286p

   .ELSE

      mov dx, _ahincr

      lds si, lpSrc
      les di, lpDest

      ;** for now, pick a simple method.  Later I can speed it up if needed

      .REPEAT

         mov al, BYTE PTR [si]
         cmp BYTE PTR es:[di], al

         .BREAK .IF !ZERO?

         or al,al
         .BREAK .IF ZERO?     ; here it returns 0 if strings equal!

         inc si
         .IF CARRY?
            add WORD PTR lpSrc+2, dx
            mov ds, WORD PTR lpSrc+2
         .ENDIF

         inc di
         .IF CARRY?
            add WORD PTR lpDest+2, dx
            mov es, WORD PTR lpDest+2
         .ENDIF

      .UNTIL 0

   .ENDIF

   mov ax, 0               ; 2(1) cycles

   .IF !ZERO?              ; 8:3(3:1) cycles (1 fail)   12 max if 'pass'
      .IF CARRY?           ; 8:3(3:1) cycles (1 pass or 1 each)
         mov ax, -1        ;          worst case 13(5)
      .ELSE
         mov ax, 1
      .ENDIF               ; total: 5-15(2-6)
   .ENDIF


   RET

hstrcmp ENDP



hstricmp PROC FAR C USES es si di, lpDest:DWORD, lpSrc:DWORD

   .IF _ok_to_use_386_instructions_!=0  ; can I use '386' instructions?

.386p

      mov esi, 0
      mov edi, 0

      push fs
      lfs si, lpSrc
      les di, lpDest

      .REPEAT

         mov al, fs:[esi]
         mov bl, al
         mov bh, 0

         .IF _ctype[bx + 1] & _LOWER

            sub al, 'a'-'A'      ; al contains SOURCE byte

         .ENDIF


         mov bl, BYTE PTR es:[edi]

         .IF _ctype[bx + 1] & _LOWER

            sub bl, 'a'-'A'      ; bl contains DEST byte

         .ENDIF


         cmp bl, al              ; compare dest with source

         .BREAK .IF !ZERO?

         inc esi
         inc edi

      .UNTIL al == 0             ; here it returns 0 if strings equal!

      pop fs

.286p

   .ELSE

      mov dx, _ahincr

      mov cx, ds                 ; assign DGROUP to 'cx' register

      lds si, lpSrc
      les di, lpDest

      ;** for now, pick a simple method.  Later I can speed it up if needed

      .REPEAT

         mov bl, BYTE PTR [si]
         mov al, bl
         mov bh, 0

         push ds
         mov ds, cx              ; RE-LOAD DGROUP IN 'DS'

         .IF _ctype[bx + 1] & _LOWER

            sub al, 'a'-'A'      ; al contains SOURCE byte

         .ENDIF


         mov bl, BYTE PTR es:[di]

         .IF _ctype[bx + 1] & _LOWER

            sub bl, 'a'-'A'      ; bl contains DEST byte

         .ENDIF


         pop ds                  ; RESTORE 'DS'


         cmp bl, al              ; compare dest with source

         .BREAK .IF !ZERO?

         or al,al
         .BREAK .IF ZERO?        ; here it returns 0 if strings equal!

         inc si
         .IF CARRY?
            add WORD PTR lpSrc+2, dx
            mov ds, WORD PTR lpSrc+2
         .ENDIF

         inc di
         .IF CARRY?
            add WORD PTR lpDest+2, dx
            mov es, WORD PTR lpDest+2
         .ENDIF

      .UNTIL 0

   .ENDIF

   mov ax, 0               ; 2(1) cycles

   .IF !ZERO?              ; 8:3(3:1) cycles (1 fail)   12 max if 'pass'
      .IF CARRY?           ; 8:3(3:1) cycles (1 pass or 1 each)
         mov ax, -1        ;          worst case 13(5)
      .ELSE
         mov ax, 1
      .ENDIF               ; total: 5-15(2-6)
   .ENDIF


   RET

hstricmp ENDP



hstrlen PROC FAR C USES es di, lpSrc:DWORD

   .IF _ok_to_use_386_instructions_!=0  ; can I use '386' instructions?

.386p

      mov edi, 0

      les di, lpSrc

      mov ecx, 0ffffffffH

      mov al, 0
      cld
      repnz scas BYTE PTR ES:[edi]  ; 'edi' index forces 'ecx' counter!

      ;OK!  ecx is DECREMENTED before the 'scas' is performed, so I must
      ;now INCREMENT IT once to adjust the value...

      inc ecx

      ;NOW, 'flip the bits' of ecx to get the length of the string

      xor ecx, 0ffffffffH

      mov eax, ecx         ; both EAX and dx:ax contain the length!
      shr ecx, 10H
      mov dx, cx

.286p

   .ELSE

      mov dx, _ahincr
      mov ax, 0
      mov dx, ax

      les di, lpSrc

      .REPEAT

         .BREAK .IF BYTE PTR es:[di]==0

         inc di
         .IF CARRY?
            add WORD PTR lpSrc+2, dx
            mov es, WORD PTR lpSrc+2
         .ENDIF

         inc ax
         adc dx, 0

      .UNTIL 0

   .ENDIF

   ; at this point, dx:ax contains the return value

   RET

hstrlen ENDP



hstrncpy PROC FAR C USES es si di,
         lpDest:DWORD, lpSrc:DWORD, dwLen:DWORD

   LOCAL dwCtr:DWORD


   .IF _ok_to_use_386_instructions_!=0  ; can I use '386' instructions?

.386p

      mov ecx, dwLen

      .IF ecx != 0

         mov esi, 0
         mov edi, 0

         push fs
         lfs si, lpSrc
         les di, lpDest

         .REPEAT

            mov al, fs:[esi]        ; optimized for 486 - 1 cycle!
            mov es:[edi], al        ; LODSB/STOSB uses 10 cycles

            inc esi                 ; loop time 8 cycles for 386, 4 for 486
            inc edi                 ; (LODSB/STOSB is the same for 386)

            dec ecx

         .UNTIL al == 0 || ecx == 0

         pop fs

      .ENDIF

.286p

   .ELSE

      .IF WORD PTR dwLen != 0 && WORD PTR dwLen + 2 != 0

         mov dx, _ahincr

         lds si, lpSrc
         les di, lpDest

         mov WORD PTR dwCtr, 0
         mov WORD PTR dwCtr+2, 0


         ;** for now, pick a simple method.  Later I can speed it up if needed

         .REPEAT

            mov al, BYTE PTR [si]
            mov BYTE PTR es:[di], al

            .BREAK .IF al==0

            inc si
            .IF CARRY?
               add WORD PTR lpSrc+2, dx
               mov ds, WORD PTR lpSrc+2
            .ENDIF

            inc di
            .IF CARRY?
               add WORD PTR lpDest+2, dx
               mov es, WORD PTR lpDest+2
            .ENDIF


            inc WORD PTR dwCtr
            adc WORD PTR dwCtr+2, 0

            mov ax, WORD PTR dwCtr+2
            cmp ax, WORD PTR dwLen+2

            .BREAK .IF !(ZERO? || CARRY?)

            mov ax, WORD PTR dwCtr

         .UNTIL ZERO? && (ax >= (WORD PTR dwLen))

      .ENDIF

   .ENDIF

   mov ax, WORD PTR lpDest
   mov dx, WORD PTR lpDest+2

   RET

hstrncpy ENDP


hstrncmp PROC FAR C USES es si di,
         lpDest:DWORD, lpSrc:DWORD, dwLen:DWORD

   LOCAL dwCtr:DWORD

   .IF _ok_to_use_386_instructions_!=0  ; can I use '386' instructions?

.386p

      mov ecx, dwLen

      .IF ecx == 0

         xor eax, eax

         RET

      .ELSE

         mov esi, 0
         mov edi, 0

         push fs
         lfs si, lpSrc
         les di, lpDest

         pushf                   ; pre-stuff flags on stack

         .REPEAT

            popf

            mov al, fs:[esi]        ; compare dest with source
            cmp es:[edi], al

            pushf

            .BREAK .IF !ZERO?

            inc esi
            inc edi
            dec ecx

         .UNTIL al == 0 || ecx == 0

         popf
         pop fs


      .ENDIF

.286p

   .ELSE

      mov dx, _ahincr

      .IF (WORD PTR dwLen == 0) && (WORD PTR dwLen+2 == 0)

         xor ax, ax

         RET

      .ENDIF

      lds si, lpSrc
      les di, lpDest

      ;** for now, pick a simple method.  Later I can speed it up if needed

      pushf                   ; pre-stuff flags on stack

      .REPEAT

         popf

         mov al, BYTE PTR [si]
         cmp BYTE PTR es:[di], al

         .BREAK .IF !ZERO?

         pushf

         or al,al
         .BREAK .IF ZERO?     ; here it returns 0 if strings equal!

         inc si
         .IF CARRY?
            add WORD PTR lpSrc+2, dx
            mov ds, WORD PTR lpSrc+2
         .ENDIF

         inc di
         .IF CARRY?
            add WORD PTR lpDest+2, dx
            mov es, WORD PTR lpDest+2
         .ENDIF

         inc WORD PTR dwCtr
         adc WORD PTR dwCtr+2, 0

         mov ax, WORD PTR dwCtr+2
         cmp ax, WORD PTR dwLen+2

         .BREAK .IF !(ZERO? || CARRY?)

         mov ax, WORD PTR dwCtr

      .UNTIL ZERO? && (ax >= (WORD PTR dwLen))

      popf                    ; get results of COMPARE (above)

   .ENDIF

   mov ax, 0               ; 2(1) cycles

   .IF !ZERO?              ; 8:3(3:1) cycles (1 fail)   12 max if 'pass'
      .IF CARRY?           ; 8:3(3:1) cycles (1 pass or 1 each)
         mov ax, -1        ;          worst case 13(5)
      .ELSE
         mov ax, 1
      .ENDIF               ; total: 5-15(2-6)
   .ENDIF


   RET

hstrncmp ENDP



hstrnicmp PROC FAR C USES es si di,
          lpDest:DWORD, lpSrc:DWORD, dwLen:DWORD

   LOCAL dwCtr:DWORD


   .IF _ok_to_use_386_instructions_!=0  ; can I use '386' instructions?

.386p

      mov ecx, dwLen

      .IF ecx == 0

         xor eax, eax

         RET

      .ELSE

         mov esi, 0
         mov edi, 0

         push fs
         lfs si, lpSrc
         les di, lpDest

         pushf                   ; pre-stuff flags on stack

         .REPEAT

            popf

            mov al, fs:[esi]
            mov bl, al
            mov bh, 0

            .IF _ctype[bx + 1] & _LOWER

               sub al, 'a'-'A'      ; al contains SOURCE byte

            .ENDIF


            mov bl, BYTE PTR es:[edi]

            .IF _ctype[bx + 1] & _LOWER

               sub bl, 'a'-'A'      ; bl contains DEST byte

            .ENDIF


            cmp bl, al              ; compare dest with source

            pushf

            .BREAK .IF !ZERO?

            inc esi
            inc edi
            dec ecx

         .UNTIL al == 0 || ecx == 0

         popf
         pop fs


      .ENDIF

.286p

   .ELSE

      mov dx, _ahincr

      .IF (WORD PTR dwLen == 0) && (WORD PTR dwLen+2 == 0)

         mov ax, 0

         RET

      .ENDIF

      pushf


      mov cx, ds                 ; assign DGROUP to 'cx' register

      lds si, lpSrc
      les di, lpDest

      ;** for now, pick a simple method.  Later I can speed it up if needed

      .REPEAT

         mov bl, BYTE PTR [si]
         mov al, bl
         mov bh, 0

         push ds
         mov ds, cx              ; RE-LOAD DGROUP IN 'DS'

         .IF _ctype[bx + 1] & _LOWER

            sub al, 'a'-'A'      ; al contains SOURCE byte

         .ENDIF


         mov bl, BYTE PTR es:[di]

         .IF _ctype[bx + 1] & _LOWER

            sub bl, 'a'-'A'      ; bl contains DEST byte

         .ENDIF


         pop ds                  ; RESTORE 'DS'

         popf

         cmp bl, al              ; compare dest with source

         .BREAK .IF !ZERO?

         pushf

         or al,al
         .BREAK .IF ZERO?        ; here it returns 0 if strings equal!

         inc si
         .IF CARRY?
            add WORD PTR lpSrc+2, dx
            mov ds, WORD PTR lpSrc+2
         .ENDIF

         inc di
         .IF CARRY?
            add WORD PTR lpDest+2, dx
            mov es, WORD PTR lpDest+2
         .ENDIF

         inc WORD PTR dwCtr
         adc WORD PTR dwCtr+2, 0

         mov ax, WORD PTR dwCtr+2
         cmp ax, WORD PTR dwLen+2

         .BREAK .IF !(ZERO? || CARRY?)

         mov ax, WORD PTR dwCtr

      .UNTIL ZERO? && (ax >= (WORD PTR dwLen))

      popf

   .ENDIF

   mov ax, 0               ; 2(1) cycles

   .IF !ZERO?              ; 8:3(3:1) cycles (1 fail)   12 max if 'pass'
      .IF CARRY?           ; 8:3(3:1) cycles (1 pass or 1 each)
         mov ax, -1        ;          worst case 13(5)
      .ELSE
         mov ax, 1
      .ENDIF               ; total: 5-15(2-6)
   .ENDIF


   RET

hstrnicmp ENDP





hstrchr PROC FAR C USES es si di, lpSrc:DWORD, wByte:WORD

   LOCAL lpRval:DWORD



   .IF _ok_to_use_386_instructions_!=0  ; can I use '386' instructions?

.386p

      mov esi, 0

      mov lpRval, esi

      les si, lpSrc
      mov al, BYTE PTR wByte



      mov bl, es:[esi]

      .WHILE al != bl && bl != 0

         inc esi

         mov bl, es:[esi]

      .ENDW

      .IF al == bl         ; this means it was FOUND!

         mov WORD PTR lpRval, si ; esi points to the string in the SOURCE
         shr esi, 10H
         imul esi, _ahincr       ; total # of selectors I 'transcended'

         add si, WORD PTR lpSrc + 2  ; original selector of source

         mov WORD PTR lpRval + 2, si ; save result here!

      .ENDIF

      mov eax, 0     ; ensure high word is ZERO!

.286p

   .ELSE

      ;* I don't care much about '286 CPU's, so I'll do this one *
      ;* the old-fashioned brute-force way to get it 'over with' *

      mov WORD PTR lpRval, 0
      mov WORD PTR lpRval + 2, 0


      les si, lpSrc
      mov al, BYTE PTR wByte

      mov bl, es:[si]

      .WHILE al != bl && bl != 0

         inc si
         .IF si == 0

            mov ax, es
            add ax, _ahincr
            mov es, ax

         .ENDIF

         mov bl, es:[si]

      .ENDW

      .IF al == bl         ; this means it was FOUND!

         mov WORD PTR lpRval, si     ; si points to string in the SOURCE
         mov WORD PTR lpRval + 2, es ; modified selector of source

      .ENDIF

   .ENDIF


   mov ax, WORD PTR lpRval
   mov dx, WORD PTR lpRval + 2

   RET

hstrchr ENDP



hstrichr PROC FAR C USES es si di, lpSrc:DWORD, wByte:WORD

   LOCAL lpRval:DWORD



   .IF _ok_to_use_386_instructions_!=0  ; can I use '386' instructions?

.386p

      mov esi, 0

      mov lpRval, esi

      les si, lpSrc

      mov al, BYTE PTR wByte
      mov bl, al
      mov bh, 0

      .IF _ctype[bx + 1] & _LOWER

         sub al, 'a'-'A'      ; al contains compare byte

      .ENDIF


      mov bl, es:[esi]

      .IF _ctype[bx + 1] & _LOWER

         sub bl, 'a'-'A'      ; bl contains SRC byte

      .ENDIF


      .WHILE al != bl && bl != 0

         inc esi

         mov bl, es:[esi]

         .IF _ctype[bx + 1] & _LOWER

            sub bl, 'a'-'A'      ; bl contains SRC byte

         .ENDIF

      .ENDW

      .IF al == bl         ; this means it was FOUND!

         mov WORD PTR lpRval, si ; esi points to the string in the SOURCE
         shr esi, 10H
         imul esi, _ahincr       ; total # of selectors I 'transcended'

         add si, WORD PTR lpSrc + 2  ; original selector of source

         mov WORD PTR lpRval + 2, si ; save result here!

      .ENDIF

      mov eax, 0     ; ensure high word is ZERO!

.286p

   .ELSE

      ;* I don't care much about '286 CPU's, so I'll do this one *
      ;* the old-fashioned brute-force way to get it 'over with' *

      mov WORD PTR lpRval, 0
      mov WORD PTR lpRval + 2, 0


      les si, lpSrc
      mov al, BYTE PTR wByte
      mov bl, al
      mov bh, 0

      .IF _ctype[bx + 1] & _LOWER

         sub al, 'a'-'A'      ; al contains compare byte

      .ENDIF


      mov bl, es:[si]

      .IF _ctype[bx + 1] & _LOWER

         sub bl, 'a'-'A'      ; bl contains SRC byte

      .ENDIF


      .WHILE al != bl && bl != 0

         inc si
         .IF si == 0

            mov ax, es
            add ax, _ahincr
            mov es, ax

         .ENDIF

         mov bl, es:[si]

         .IF _ctype[bx + 1] & _LOWER

            sub bl, 'a'-'A'      ; bl contains SRC byte

         .ENDIF

      .ENDW

      .IF al == bl         ; this means it was FOUND!

         mov WORD PTR lpRval, si     ; si points to string in the SOURCE
         mov WORD PTR lpRval + 2, es ; modified selector of source

      .ENDIF

   .ENDIF


   mov ax, WORD PTR lpRval
   mov dx, WORD PTR lpRval + 2

   RET

hstrichr ENDP



hstrrchr PROC FAR C USES es si di, lpSrc:DWORD, wByte:WORD

   LOCAL lpRval:DWORD, dwLen:DWORD


   INVOKE hstrlen, lpSrc

   mov WORD PTR dwLen, ax
   mov WORD PTR dwLen + 2, dx

   .IF ax==0 && dx==0

      RET               ; return NULL pointer (not found) if lpSrc is empty

   .ENDIF



   .IF _ok_to_use_386_instructions_!=0  ; can I use '386' instructions?

.386p

      mov esi, 0

      mov lpRval, esi

      les si, lpSrc
      mov al, BYTE PTR wByte

      add esi, dwLen
      dec esi
      mov ecx, 1

      mov bl, es:[esi]

      .WHILE al != bl && ecx < dwLen

         dec esi
         inc ecx

         mov bl, es:[esi]

      .ENDW

      .IF al == bl         ; this means it was FOUND!

         mov WORD PTR lpRval, si ; esi points to the string in the SOURCE
         shr esi, 10H
         imul esi, _ahincr       ; total # of selectors I 'transcended'

         add si, WORD PTR lpSrc + 2  ; original selector of source

         mov WORD PTR lpRval + 2, si ; save result here!

      .ENDIF

      mov eax, 0     ; ensure high word is ZERO!

.286p

   .ELSE

      ;* I don't care much about '286 CPU's, so I'll do this one *
      ;* the old-fashioned brute-force way to get it 'over with' *

      mov WORD PTR lpRval, 0
      mov WORD PTR lpRval + 2, 0


      ;* decrement the length FIRST *

      dec WORD PTR dwLen
      sbb WORD PTR dwLen + 2, 0


      les si, lpSrc
      mov al, BYTE PTR wByte

      add si, WORD PTR dwLen             ; calculate NEW pointer
      mov di, WORD PTR dwLen + 2
      adc di, 0

      .IF di != 0

         imul di, _ahincr
         mov bx, es
         add bx, di
         mov es, bx

      .ENDIF


      mov cx, WORD PTR dwLen
      mov dx, WORD PTR dwLen + 2

      mov bl, es:[si]

      .WHILE al != bl && (cx != 0 || dx != 0)

         .IF si == 0

            mov ax, es
            sub ax, _ahincr
            mov es, ax

         .ENDIF

         dec si


         .IF cx == 0

            dec dx

         .ENDIF

         dec cx


         mov bl, es:[si]

      .ENDW

      .IF al == bl         ; this means it was FOUND!

         mov WORD PTR lpRval, si     ; si points to string in the SOURCE
         mov WORD PTR lpRval + 2, es ; modified selector of source

      .ENDIF


   .ENDIF


   mov ax, WORD PTR lpRval
   mov dx, WORD PTR lpRval + 2

   RET

hstrrchr ENDP






hstrstr PROC FAR C USES es si di, lpSrc:DWORD, lpStr:DWORD

   LOCAL lpRval:DWORD



   .IF _ok_to_use_386_instructions_!=0  ; can I use '386' instructions?

.386p

      mov esi, 0
      mov edi, 0

      mov lpRval, edi

      push fs
      lfs si, lpSrc
      les di, lpStr

      .REPEAT

         push esi
         push edi

         mov al, fs:[esi]
         mov bl, es:[edi]

         .WHILE al != 0 && bl != 0 && al == bl

            inc esi
            inc edi

            mov al, fs:[esi]
            mov bl, es:[edi]

         .ENDW

         pop edi
         pop esi

         .IF bl == 0         ; this means it was FOUND!

            mov WORD PTR lpRval, si ; esi points to the string in the SOURCE
            shr esi, 10H
            imul esi, _ahincr       ; total # of selectors I 'transcended'

            add si, WORD PTR lpSrc + 2  ; original selector of source

            mov WORD PTR lpRval + 2, si ; save result here!

            .BREAK

         .ELSEIF al == 0

            .BREAK           ; this means it was NOT FOUND!

         .ENDIF

         inc esi

      .UNTIL 0

      pop fs

      mov eax, 0     ; ensure high word is ZERO!

.286p

   .ELSE

      ;* I don't care much about '286 CPU's, so I'll do this one *
      ;* the old-fashioned brute-force way to get it 'over with' *

      mov WORD PTR lpRval, 0
      mov WORD PTR lpRval + 2, 0


      lds si, lpSrc
      les di, lpStr

      .REPEAT

         push si
         push ds
         push di
         push es

         mov al, [si]
         mov bl, es:[di]

         .WHILE al != 0 && bl != 0 && al == bl

            inc si
            .IF si == 0

               mov ax, ds
               add ax, _ahincr
               mov ds, ax

            .ENDIF

            inc di
            .IF di == 0

               mov ax, es
               add ax, _ahincr
               mov es, ax

            .ENDIF

            mov al, [si]
            mov bl, es:[di]

         .ENDW

         pop es
         pop di
         pop ds
         pop si

         .IF bl == 0         ; this means it was FOUND!

            mov WORD PTR lpRval, si     ; si points to string in the SOURCE
            mov WORD PTR lpRval + 2, ds ; modified selector of source

            .BREAK

         .ELSEIF al == 0

            .BREAK           ; this means it was NOT FOUND!

         .ENDIF

         inc si
         .IF si == 0

            mov ax, ds
            add ax, _ahincr
            mov ds, ax

         .ENDIF

      .UNTIL 0

   .ENDIF


   mov ax, WORD PTR lpRval
   mov dx, WORD PTR lpRval + 2

   RET

hstrstr ENDP



hstristr PROC FAR C USES es si di, lpSrc:DWORD, lpStr:DWORD

   LOCAL lpRval:DWORD



   .IF _ok_to_use_386_instructions_!=0  ; can I use '386' instructions?

.386p

      mov esi, 0
      mov edi, 0

      mov lpRval, edi

      push fs
      lfs si, lpSrc
      les di, lpStr

      .REPEAT

         push esi
         push edi

         mov al, fs:[esi]
         mov bl, al
         mov bh, 0

         .IF _ctype[bx + 1] & _LOWER

            sub al, 'a'-'A'      ; al contains SOURCE byte

         .ENDIF


         mov bl, BYTE PTR es:[edi]

         .IF _ctype[bx + 1] & _LOWER

            sub bl, 'a'-'A'      ; bl contains DEST byte

         .ENDIF


         .WHILE al != 0 && bl != 0 && al == bl

            inc esi
            inc edi

            mov al, fs:[esi]
            mov bl, al
            mov bh, 0

            .IF _ctype[bx + 1] & _LOWER

               sub al, 'a'-'A'      ; al contains SOURCE byte

            .ENDIF


            mov bl, BYTE PTR es:[edi]

            .IF _ctype[bx + 1] & _LOWER

               sub bl, 'a'-'A'      ; bl contains DEST byte

            .ENDIF

         .ENDW

         pop edi
         pop esi

         .IF bl == 0         ; this means it was FOUND!

            mov WORD PTR lpRval, si ; esi points to the string in the SOURCE
            shr esi, 10H
            imul esi, _ahincr       ; total # of selectors I 'transcended'

            add si, WORD PTR lpSrc + 2  ; original selector of source

            mov WORD PTR lpRval + 2, si ; save result here!

            .BREAK

         .ELSEIF al == 0

            .BREAK           ; this means it was NOT FOUND!

         .ENDIF

         inc esi

      .UNTIL 0

      pop fs

      mov eax, 0     ; ensure high word is ZERO!

.286p

   .ELSE

      ;* I don't care much about '286 CPU's, so I'll do this one *
      ;* the old-fashioned brute-force way to get it 'over with' *

      mov WORD PTR lpRval, 0
      mov WORD PTR lpRval + 2, 0

      mov cx, ds                 ; save DGROUP here

      lds si, lpSrc
      les di, lpStr

      .REPEAT

         push si
         push ds
         push di
         push es

         mov bl, BYTE PTR [si]
         mov al, bl
         mov bh, 0

         push ds
         mov ds, cx              ; RE-LOAD DGROUP IN 'DS'

         .IF _ctype[bx + 1] & _LOWER

            sub al, 'a'-'A'      ; al contains SOURCE byte

         .ENDIF


         mov bl, BYTE PTR es:[di]

         .IF _ctype[bx + 1] & _LOWER

            sub bl, 'a'-'A'      ; bl contains DEST byte

         .ENDIF

         pop ds


         .WHILE al != 0 && bl != 0 && al == bl

            inc si
            .IF si == 0

               mov ax, ds
               add ax, _ahincr
               mov ds, ax

            .ENDIF

            inc di
            .IF di == 0

               mov ax, es
               add ax, _ahincr
               mov es, ax

            .ENDIF


            mov bl, BYTE PTR [si]
            mov al, bl
            mov bh, 0

            push ds
            mov ds, cx              ; RE-LOAD DGROUP IN 'DS'

            .IF _ctype[bx + 1] & _LOWER

               sub al, 'a'-'A'      ; al contains SOURCE byte

            .ENDIF


            mov bl, BYTE PTR es:[di]

            .IF _ctype[bx + 1] & _LOWER

               sub bl, 'a'-'A'      ; bl contains DEST byte

            .ENDIF

            pop ds

         .ENDW

         pop es
         pop di
         pop ds
         pop si

         .IF bl == 0         ; this means it was FOUND!

            mov WORD PTR lpRval, si     ; si points to string in the SOURCE
            mov WORD PTR lpRval + 2, ds ; modified selector of source

            .BREAK

         .ELSEIF al == 0

            .BREAK           ; this means it was NOT FOUND!

         .ENDIF

         inc si
         .IF si == 0

            mov ax, ds
            add ax, _ahincr
            mov ds, ax

         .ENDIF

      .UNTIL 0

   .ENDIF


   mov ax, WORD PTR lpRval
   mov dx, WORD PTR lpRval + 2

   RET

hstristr ENDP






;****************************************************************************
;
;   'HUGE STRING' FUNCTIONS THAT OBTAIN THE VALUE OF A STRING AS A NUMBER
;
;****************************************************************************



_hatol PROC FAR PASCAL USES es bx si di, lpSrc:DWORD

   mov di, _ahincr

   les si, lpSrc
   mov ax, 0
   mov bh, al
   mov dx, ax

   .REPEAT

      mov bl, BYTE PTR es:[si]

      .BREAK .IF bl==0 || bl>' '

      inc si
      .IF CARRY?
         add WORD PTR lpSrc+2, di
         mov es, WORD PTR lpSrc+2
      .ENDIF

   .UNTIL 0

   .IF bl!=0

      .REPEAT

         mov bl, BYTE PTR es:[si]

         .BREAK .IF bl==0

         inc si
         .IF CARRY?
            add WORD PTR lpSrc+2, di
            mov es, WORD PTR lpSrc+2
         .ENDIF


         .IF bl>='0' && bl<='9'

            sub bl, '0'

            ; do a 'multiply by 10' of dx:ax.  Use 'shift' instructions
            ; because I'd otherwise have to move registers and use 'mul'
            ; twice, which defeats the purpose of using 'mul' anyway.

            push bx

            mov bx, ax         ; 2(1) cycles each (386/486)
            mov cx, dx

            shl ax, 1          ; 3 cycles each (386/486)
            rcl dx, 1          ; multiply by 2
            shl ax, 1
            rcl dx, 1          ; multiply by 4

            add ax, bx         ; 2(1) cycles each (386/486)
            adc dx, cx         ; net result: multiply by 5

            shl ax, 1          ; 3 cycles each (386/486)
            rcl dx, 1          ; net result: multiply by 10

                               ; total = 32(30) cycles (386/486)
            pop bx

            ;** well, that was rather painful, but it worked.

            add al, bl         ; add this digit back in
            adc ah, 0
            adc dx, 0          ; this is probably faster than push/pop

         .ELSEIF bl=='+'

            .IF bh & 1          ; syntax error - stop here

               stc              ; carry set to indicate a problem...

               .BREAK

            .ENDIF

            and bh, NOT 2

         .ELSEIF bl=='-'

            .IF bh & 1          ; syntax error - stop here

               stc              ; carry set to indicate a problem...

               .BREAK

            .ENDIF

            or bh, 2            ; negative!!!

         .ELSE

            stc                 ; carry set to indicate a problem...

            .BREAK              ; syntax error - stop here!

         .ENDIF


         or bh, 1               ; set bit to flag 'not the first char'

      .UNTIL 0


      pushf                     ; save CARRY, actually

      .IF bh & 2                ; This is a NEGATIVE number!

         xor ax, 0ffffH
         xor dx, 0ffffH
         inc ax
         adc dx, 0              ; using this method is guaranteed to work
                                ; and only uses 8(4) cycles (386/486)
      .ENDIF

      popf                      ; restore CARRY, actually


   .ENDIF

   RET

_hatol ENDP



ldONEHALF REAL10 0.5
ldTWO     REAL10 2.0


MyExponent PROC NEAR

   LOCAL temp:REAL10, iExp:WORD, wTemp:WORD, fcw:WORD

   ; exponent 'X' value is in dx; st(0) contains 'Y' value to 'exponentiate'

   .IF dx==0
      RET                 ; do nothing if dx is zero
   .ENDIF


   fstcw fcw              ; save floating pt control word
   mov   ax, fcw
   or    ax, 0a00H        ; sets bits 10 and 11 ('RC' bits - truncate mode)
   mov   wTemp, ax
   fldcw wTemp            ; load control word with RC bits set to '11' so
                          ; that all float to integer operations TRUNCATE.

   mov   wTemp, dx
   fild  wTemp            ; put 'X' into st(0); st(1) contains 'Y' value

   fldl2t                 ; load LOG2(10) into st(0); st(1) contains 'X'
   fmul                   ; multiply 'X' by log2(10) to get 'base 2'
                          ; exponent (rather than base 10).
                          ; st(0) contains 'X'; st(1) contains 'Y' value

   fld   st               ; make copy of current st (gets popped again below)
   fstp  temp             ; store base 2 exponent (float value) into 'temp'


   fist  iExp             ; store as integer into iExp...
   fild  iExp             ; re-load integer version
   fsub                   ; float part minus int part -> st(0)
   fstp  temp             ; store 'remainder' into 'temp'
   fild  iExp             ; re-load integer part of base-2 exponent...

   fxch                   ; st(1) contains INT('X'); st(0) contains 'Y' value
   fscale                 ; st(0) contains Y * 2^INT('X')

   fxch                   ; switch st(0) and st(1) again
   fstp  st               ; discard st(0); new st(0) is Y * 2^INT('X')
                          ; no other elements on stack are 'in use'

   ; SCALE THE 'exponent remainder' to 0 <= 'X' < 1; then, calculate the
   ; value of 2^x and multiply by 'result' value (currently in 'st(0)')

   fld   temp             ; load base 2 exponent (fractional 'remainder')
                          ; st(0) contains exponent; st(1) contains 'result'

   ftst                   ; is the exponent value above or below zero?
   fstsw ax
   sahf

   .IF CARRY?             ; carry set == 'BELOW'

      fld   ldONEHALF     ; load value of 1/2 onto stack
      fmulp st(2),st      ; multiply by current 'result' value & pop '1/2'

      fld1                ; load value '1.0' onto stack
      fadd                ; add '1' to exponent (after dividing result by 2)

   .ENDIF


   fld   ldONEHALF        ; load value of 1/2 onto stack
   fcomp                  ; compare st(1) (exponent) with 1/2...


   .IF !CARRY? && !ZERO?  ; above 1/2!

      fld   ldTWO         ; load value '2' into st(0)
      fsqrt               ; take square root of 2 and leave in st(0)

      fxch  st(1)         ; exchange st(1) (exponent) with sqrt(2)

      f2xm1               ; change exponent to 2^x - 1
      fld1                ; load value '1' into st(0)
      fadd                ; add '1' to '2^x - 1' exponent value
      fmul                ; multiply st(1) * '2^x - 1' to get final result

   .ELSE

      f2xm1               ; change exponent to 2^x - 1
      fld1                ; load value '1' into st(0)
      fadd                ; add '1' to '2^x - 1' exponent value

   .ENDIF

   fmul                   ; multiply exponent (factor) by current 'result'

   fldcw fcw              ; restore previous fpp control word

   RET


MyExponent ENDP



dTEN   REAL8 10.0
dTENTH REAL8 0.1


_hatof PROC FAR PASCAL USES es bx si di,
             lpSrc:DWORD, sbpRval:NEAR PTR REAL8

   LOCAL rval:REAL8, dFactor:REAL8, bSignFlag:WORD, bDecimalFlag:WORD,
         wTemp:WORD


   fldz                     ; load zero onto fp stack (the result)
   fstp rval                ; store zero into 'rval'

   fld  dTENTH
   fstp dFactor             ; store 0.1 into 'dFactor' (for later)

   mov di, _ahincr

   les si, lpSrc
   mov bx, 0
   mov bSignFlag, bx        ; initially, sign is positive
   mov bDecimalFlag, bx     ; initially, decimal flag is OFF
   mov dx, bx               ; initial exponent (10^0)


   .REPEAT

      mov bl, BYTE PTR es:[si]

      .BREAK .IF bl==0 || bl>' '

      inc si
      .IF CARRY?
         add WORD PTR lpSrc+2, di
         mov es, WORD PTR lpSrc+2
      .ENDIF

   .UNTIL 0

   .IF bl!=0

      .REPEAT

         mov bl, BYTE PTR es:[si]

         .BREAK .IF bl==0

         inc si
         .IF CARRY?
            add WORD PTR lpSrc+2, di
            mov es, WORD PTR lpSrc+2
         .ENDIF


         .IF bl>='0' && bl<='9'

            sub bl, '0'

            .IF bDecimalFlag == 0       ; the INTEGER portion of the number

               fld  rval
               fmul dTEN          ; multiply st(0) by 10
               mov  wTemp, bx
               fild wTemp         ; load value of digit
               fadd               ; add value of digit!
               fstp rval          ; store result in 'rval'

            .ELSEIF bDecimalFlag == 1

               fld  dFactor       ; load 'factor' (decimal place multiplier)
               mov  wTemp, bx
               fild wTemp         ; load value of digit
               fmul st,st(1)      ; multiply by 'factor' (decimal place)

               fadd rval          ; add to 'rval'
               fstp rval          ; and store back into 'rval'

               fmul dTENTH        ; multiply 'factor' by 0.1
               fstp dFactor       ; and store back into 'factor'

            .ELSE

               imul dx, 10
               add dx, bx

            .ENDIF

         .ELSEIF bl=='+'

            .IF bDecimalFlag == 2

               .IF bSignFlag & 4   ; syntax error - stop here

                  stc              ; carry set to indicate a problem...

                  .BREAK

               .ENDIF

               and bSignFlag, NOT 8

            .ELSE

               .IF bSignFlag & 1   ; syntax error - stop here

                  stc              ; carry set to indicate a problem...

                  .BREAK

               .ENDIF

               and bSignFlag, NOT 2

            .ENDIF


         .ELSEIF bl=='-'

            .IF bDecimalFlag == 2

               .IF bSignFlag & 4   ; syntax error - stop here

                  stc              ; carry set to indicate a problem...

                  .BREAK

               .ENDIF

               or bSignFlag, 8

            .ELSE
               .IF bSignFlag & 1   ; syntax error - stop here

                  stc              ; carry set to indicate a problem...

                  .BREAK

               .ENDIF

               or bSignFlag, 2     ; negative!!!

            .ENDIF

         .ELSEIF bl=='.'        ; decimal point - meaning of loop changes

            .IF bDecimalFlag != 0

               stc              ; 2 decimal points would be a syntax error

               .BREAK

            .ENDIF

            mov bDecimalFlag, 1 ; place constant into decimal flag

         .ELSEIF bl=='e' || bl=='E' ; exponent!

            .IF bDecimalFlag == 2

               stc              ; 2 'E's would be a syntax error

               .BREAK

            .ENDIF

            mov bDecimalFlag, 2 ; flag 'exponent'

         .ELSE

            stc                 ; carry set to indicate a problem...

            .BREAK              ; syntax error - stop here!

         .ENDIF


         .IF bDecimalFlag==2 && bl != 'e' && bl != 'E'

            or bSignFlag, 4        ; set bit to flag 'not the first char'

         .ELSE

            or bSignFlag, 1        ; set bit to flag 'not the first char'

         .ENDIF

      .UNTIL 0

   .ELSE

      clc

   .ENDIF


   pushf                     ; save CARRY, actually

   .IF bDecimalFlag == 2     ; number has an exponent

      .IF bSignFlag & 8      ; exponent is negative

         neg dx              ; make dx a negative value

      .ENDIF

      fld rval
      call MyExponent        ; exponent in dx, value in st(0)
                             ; result returned in st(0)
      fstp rval

   .ENDIF

   .IF bSignFlag & 2         ; This is a NEGATIVE number!

      fld rval
      fchs                   ; change sign (negate)

      mov bx, sbpRval        ; stack-based offset of destination
      fstp REAL8 PTR ss:[bx] ; store result... done!

   .ELSE

      mov bx, sbpRval        ; copy return value to destination

      fld rval               ; this is the fastest method on 486
      fstp REAL8 PTR ss:[bx]

   .ENDIF

   popf                      ; restore CARRY, actually

   mov ax, sbpRval           ; this is expected by some languages (like VB)
   mov dx, ss

   RET

_hatof ENDP


ldTEN   REAL10 10.0
ldTENTH REAL10 0.1


_hatold PROC FAR PASCAL USES es bx si di,
             lpSrc:DWORD, sbpRval:NEAR PTR REAL10


   LOCAL rval:REAL10, dFactor:REAL10, bSignFlag:WORD, bDecimalFlag:WORD,
         wTemp:WORD


   fldz                     ; load zero onto fp stack (the result)
   fstp rval                ; store zero into 'rval'

   fld  ldTENTH
   fstp dFactor             ; store 0.1 into 'dFactor' (for later)

   mov di, _ahincr

   les si, lpSrc
   mov bx, 0
   mov bSignFlag, bx        ; initially, sign is positive
   mov bDecimalFlag, bx     ; initially, decimal flag is OFF
   mov dx, bx               ; initial exponent (10^0)

   .REPEAT

      mov bl, BYTE PTR es:[si]

      .BREAK .IF bl==0 || bl>' '

      inc si
      .IF CARRY?
         add WORD PTR lpSrc+2, di
         mov es, WORD PTR lpSrc+2
      .ENDIF

   .UNTIL 0

   .IF bl!=0

      .REPEAT

         mov bl, BYTE PTR es:[si]

         .BREAK .IF bl==0

         inc si
         .IF CARRY?
            add WORD PTR lpSrc+2, di
            mov es, WORD PTR lpSrc+2
         .ENDIF


         .IF bl>='0' && bl<='9'

            sub bl, '0'

            .IF bDecimalFlag == 0       ; the INTEGER portion of the number

               fld  rval
               fld  ldTEN
               fmul               ; multiply st(0) by 10
               mov  wTemp, bx
               fild wTemp         ; load value of digit
               fadd               ; add value of digit!
               fstp rval          ; store result in 'rval'

            .ELSEIF bDecimalFlag == 1

               fld  dFactor       ; load 'factor' (decimal place multiplier)
               mov  wTemp, bx
               fild wTemp         ; load value of digit
               fmul st,st(1)      ; multiply by 'factor' (decimal place)

               fld  rval
               faddp st(1),st     ; add to 'rval'
               fstp rval          ; and store back into 'rval'

               fld  ldTENTH
               fmul               ; multiply 'factor' by 0.1
               fstp dFactor       ; and store back into 'factor'

            .ELSE                 ; 'bDecimalFlag' is 2 (exponent)

               imul dx, 10
               add dx, bx

            .ENDIF

         .ELSEIF bl=='+'

            .IF bDecimalFlag == 2

               .IF bSignFlag & 4   ; syntax error - stop here

                  stc              ; carry set to indicate a problem...

                  .BREAK

               .ENDIF

               and bSignFlag, NOT 8

            .ELSE

               .IF bSignFlag & 1   ; syntax error - stop here

                  stc              ; carry set to indicate a problem...

                  .BREAK

               .ENDIF

               and bSignFlag, NOT 2

            .ENDIF


         .ELSEIF bl=='-'

            .IF bDecimalFlag == 2

               .IF bSignFlag & 4   ; syntax error - stop here

                  stc              ; carry set to indicate a problem...

                  .BREAK

               .ENDIF

               or bSignFlag, 8

            .ELSE
               .IF bSignFlag & 1   ; syntax error - stop here

                  stc              ; carry set to indicate a problem...

                  .BREAK

               .ENDIF

               or bSignFlag, 2     ; negative!!!

            .ENDIF

         .ELSEIF bl=='.'        ; decimal point - meaning of loop changes

            .IF bDecimalFlag != 0

               stc              ; 2 decimal points would be a syntax error

               .BREAK

            .ENDIF

            mov bDecimalFlag, 1 ; place constant into decimal flag

         .ELSEIF bl=='e' || bl=='E' ; exponent!

            .IF bDecimalFlag == 2

               stc              ; 2 'E's would be a syntax error

               .BREAK

            .ENDIF

            mov bDecimalFlag, 2 ; flag 'exponent'

         .ELSE

            stc                 ; carry set to indicate a problem...

            .BREAK              ; syntax error - stop here!

         .ENDIF


         .IF bDecimalFlag==2 && bl != 'e' && bl != 'E'

            or bSignFlag, 4        ; set bit to flag 'not the first char'

         .ELSE

            or bSignFlag, 1        ; set bit to flag 'not the first char'

         .ENDIF


      .UNTIL 0


   .ELSE

      clc

   .ENDIF


   pushf                     ; save CARRY, actually

   .IF bDecimalFlag == 2     ; number has an exponent

      .IF bSignFlag & 8      ; exponent is negative

         neg dx              ; make dx a negative value

      .ENDIF

      fld rval
      call MyExponent        ; exponent in dx, value in st(0)
                             ; result returned in st(0)
      fstp rval

   .ENDIF

   .IF bSignFlag & 2         ; This is a NEGATIVE number!

      fld rval
      fchs                   ; change sign (negate)

      mov bx, sbpRval        ; stack-based offset of destination
      fstp REAL10 PTR ss:[bx]; store result... done!

   .ELSE

      mov bx, sbpRval        ; copy return value to destination

      fld rval               ; this is the fastest method on 486
      fstp REAL10 PTR ss:[bx]

   .ENDIF

   popf                      ; restore CARRY, actually

   mov ax, sbpRval           ; this is expected by some languages (like VB)
   mov dx, ss

   RET

_hatold ENDP


HSTR_TEXT ENDS


END
