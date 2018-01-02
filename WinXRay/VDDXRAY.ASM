.LIST
.LALL

;***************************************************************************
;**                                                                       **
;**    VDDXRAY.386 - Special 'VDD' for Win386 that performs deep system   **
;**    level 'magic stuff' within the guts of Win386 for WINXRAY.         **
;**                                                                       **
;***************************************************************************



	.386p

;******************************************************************************
;                             I N C L U D E S
;******************************************************************************

	INCLUDE VMM.Inc
	INCLUDE Debug.Inc
	INCLUDE V86MMGR.Inc


VDDXRAY_Device_ID  EQU 7fffH
VDDXRAY_Init_Order EQU 0F000000H


Declare_Virtual_Device VDDXRAY, 1, 0, VDDXRAY_Control, VDDXRAY_Device_ID, \
		       VDDXRay_Init_Order
;                       VDDXRAY_Init_Order, \
;                       VDDXRAY_PM_Callback, VDDXRAY_RM_Callback


;******************************************************************************
;                  I N I T I A L I Z A T I O N   D A T A
;******************************************************************************

VxD_IDATA_SEG



VxD_IDATA_ENDS


;******************************************************************************
;                          L O C A L   D A T A
;******************************************************************************

VxD_DATA_SEG

VX_Orig_PM_2F_Seg  dd 0
VX_Orig_PM_2F_Off  dd 0


VxD_DATA_ENDS



;******************************************************************************
;                  I N I T I A L I Z A T I O N   C O D E
;******************************************************************************

VxD_ICODE_SEG

BeginProc VDDXRAY_Sys_Critical_Init

; * THIS SECTION STOLEN FROM 'BIOSXLAT.ASM' TO DEMONSTRATE 'HOW' *

;        mov     esi, OFFSET32 BIOSXlat_Int1C
;        mov     edx, 1Ch
;        VMMcall Allocate_PM_Call_Back
;        jc      DEBFAR BXLSCI_NoPM
;        xchg    edx, eax
;        mov     ecx, edx
;        shr     ecx, 10h
;        movzx   edx, dx
;        mov     [BX_I1C_Our_Hook_Seg], cx
;        mov     [BX_I1C_Our_Hook_Off], dx
;        VMMcall Set_PM_Int_Vector
;
;
;        mov     esi, OFFSET32 BIOSXlat_V86_Int1C
;        VMMcall Hook_V86_Int_Chain


;** ALLOCATE PM AND V86 INTERRUPT HOOKS FOR INT 2FH **

	mov     eax, 2fh
	VMMcall Get_PM_Int_Vector
	mov     [VX_Orig_PM_2F_Seg], ecx
	mov     [VX_Orig_PM_2F_Off], edx
	mov     esi, OFFSET32 VDDXRAY_Int2f
	mov     edx, 2fh
	VMMcall Allocate_PM_Call_Back
	jc      BXLSCI_NoPM
	xchg    edx, eax
	mov     ecx, edx
	shr     ecx, 10h
	movzx   edx, dx
	VMMcall Set_PM_Int_Vector

	mov     esi, OFFSET32 VDDXRAY_V86_Int2f
	VMMcall Hook_V86_Int_Chain


;* FOR DEMO PURPOSES - THIS NEXT CODE SECTION MESSES WITH THE 'INT VECTOR' *
;* TABLE IN THE LOW DOS MEMORY - USEFUL FOR REFERENCE, LATER!              *

;
;   Set the Int 1Eh vector to point to the same address as the real mode
;   Int 1Eh -- This is the floppy data area.
;
;        Begin_Touch_1st_Meg
;        mov     eax, DWORD PTR ds:[1Eh*4]
;        End_Touch_1st_Meg
;        movzx   ecx, ax
;        shr     eax, 16
;        shl     eax, 4
;        add     eax, ecx
;
;        VMMcall _BuildDescriptorDWORDs, <eax, 100h, RW_Data_Type, D_GRAN_BYTE, 0>
;        VMMcall _Allocate_GDT_Selector, <edx, eax, 0>
;        or      edx,eax
;IFDEF DEBUG
;        jnz     short BXLSCID10
;        debug_out "Could not allocate GDT selector BIOSXlat_Sys_Critical_Init"
;BXLSCID10:
;ENDIF
;        jz      short BXLSCI_Fail
;
;        movzx   ecx, ax
;        xor     edx, edx
;        mov     eax, 1Eh
;        VMMcall Set_PM_Int_Vector
;

	clc
	ret

BXLSCI_NoPM:
IFDEF DEBUG
	debug_out "Could not allocate one of the PM call backs VDDXRAY_Sys_Critical_Init"
ENDIF
BXLSCI_Fail:
	VMMcall Fatal_Memory_Error

EndProc VDDXRAY_Sys_Critical_Init

VxD_ICODE_ENDS



VxD_LOCKED_CODE_SEG

;****************************************************************************
;
;   VDDXRAY_Control
;
;****************************************************************************

BeginProc VDDXRAY_Control

	Control_Dispatch Sys_Critical_Init, VDDXRAY_Sys_Critical_Init
	clc
	ret

EndProc VDDXRAY_Control
VxD_LOCKED_CODE_ENDS






VxD_CODE_SEG

BeginProc VDDXRAY_Process

	;** on entry, registers same as INT procs **
	;** on exit, carry set if I should chain  **
	;** to next interrupt, clear if I eat it. **


	push eax             ; save registers in case values are important
	push edx


	.IF [ebp].Client_Byte_Reg_Struc.Client_AL == 00H
		   ;**        subfunction 0H          **
		   ;** Identify the presence of the   **
		   ;** 'VDDXRAY' Int 2F services!     **


	   mov ax, 0ffH                ; AX == 00FFH on return for
				       ; compatibility with other int 2f's
	   mov [ebp].Client_Word_Reg_Struc.Client_AX, ax


	   clc                         ;** exit from int! Done! **


	.ELSEIF [ebp].Client_Byte_Reg_Struc.Client_AL == 01H
		   ;**        subfunction 01H         **
		   ;** get CB_High_Linear for THIS VM **
		   ;** into AX:DX, and place the 'VM  **
		   ;** HANDLE' for the VM in si:di    **


	   push ebx

	   mov edx, DWORD PTR [ebx + CB_VMID]

	   VMMCall Get_Cur_VM_Handle

	   shr edx, 10H       ; place high word of edx into dx (useful?)
	   mov [ebp].Client_Word_Reg_Struc.Client_BX, dx  
			      ; return this to caller in 'BX'

	   mov ax, WORD PTR [ebx + CB_High_Linear]
	   mov dx, WORD PTR [ebx + CB_High_Linear + 2]

	   mov [ebp].Client_Word_Reg_Struc.Client_AX, ax
	   mov [ebp].Client_Word_Reg_Struc.Client_DX, dx

	   mov eax, ebx
	   mov [ebp].Client_Word_Reg_Struc.Client_DI, ax ; LOW WORD

	   shr eax, 10H
	   mov [ebp].Client_Word_Reg_Struc.Client_SI, ax ; HIGH WORD

	   clc                         ;** exit from int! Done! **

	   pop ebx


	.ELSEIF [ebp].Client_Byte_Reg_Struc.Client_AL == 02H
		   ;**        subfunction 02H         **
		   ;** get CB_High_Linear for the VM  **
		   ;** specified in 'BX' and place it **
		   ;** into AX:DX, and place the 'VM  **
		   ;** HANDLE' for the VM in si:di    **


	   push ebx

	   VMMCall Get_Cur_VM_Handle

	   mov edx, DWORD PTR [ebx + CB_VMID]

	   .IF dx != [ebp].Client_Word_Reg_Struc.Client_BX ; NOT CURRENT VM!

	      mov eax, ebx

	      .REPEAT

		 VMMCall Get_Next_VM_Handle   ;** NOTE:  List is CIRCULAR **

		 mov edx, DWORD PTR [ebx + CB_VMID]

	      .UNTIL ebx == eax || \
		     dx == [ebp].Client_Word_Reg_Struc.Client_BX

	   .ENDIF


	   mov edx, DWORD PTR [ebx + CB_VMID]

	   .IF dx == [ebp].Client_Word_Reg_Struc.Client_BX   ; VM matches!!

	      ;** FOUND!  If we get here, the VM ID matches Client's 'bx' **

	      shr edx, 10H    ; place high word of edx into dx (useful?)
	      mov [ebp].Client_Word_Reg_Struc.Client_BX, dx  
			      ; return this to caller in 'BX'

	      mov ax, WORD PTR [ebx + CB_High_Linear]
	      mov dx, WORD PTR [ebx + CB_High_Linear + 2]

	      mov [ebp].Client_Word_Reg_Struc.Client_AX, ax
	      mov [ebp].Client_Word_Reg_Struc.Client_DX, dx

	      mov eax, ebx
	      mov [ebp].Client_Word_Reg_Struc.Client_DI, ax ; LOW WORD

	      shr eax, 10H
	      mov [ebp].Client_Word_Reg_Struc.Client_SI, ax ; HIGH WORD

	   .ELSE

	      mov ax, 0
	      mov [ebp].Client_Word_Reg_Struc.Client_AX, ax
	      mov [ebp].Client_Word_Reg_Struc.Client_BX, ax
	      mov [ebp].Client_Word_Reg_Struc.Client_DX, ax
	      mov [ebp].Client_Word_Reg_Struc.Client_DI, ax ; LOW WORD
	      mov [ebp].Client_Word_Reg_Struc.Client_SI, ax ; HIGH WORD


	   .ENDIF


	   pop ebx                     ; remove original 'ebx' from stack

	   clc                         ;** exit from int! Done! **


        .ELSEIF [ebp].Client_Byte_Reg_Struc.Client_AL == 0fcH
                   ;**        subfunction fcH         **
                   ;**  Fill registers with register  **
                   ;**  info, and return to caller.   **

           push edx
           push ecx
           push eax

           pop [ebp].Client_Word_Reg_Struc.Client_AX ; BX::AX == EAX
           pop [ebp].Client_Word_Reg_Struc.Client_BX
           pop [ebp].Client_Word_Reg_Struc.Client_CX ; DX::CX == ECX
           pop [ebp].Client_Word_Reg_Struc.Client_DX
           pop [ebp].Client_Word_Reg_Struc.Client_SI ; DI::SI == EDX
           pop [ebp].Client_Word_Reg_Struc.Client_DI

           clc


        .ELSEIF [ebp].Client_Byte_Reg_Struc.Client_AL == 0fdH
                   ;**        subfunction fdH         **
                   ;**  Fill registers with register  **
                   ;**  info, and return to caller.   **

           push edi
           push esi
           push ebx

           pop [ebp].Client_Word_Reg_Struc.Client_AX ; BX::AX == EBX
           pop [ebp].Client_Word_Reg_Struc.Client_BX
           pop [ebp].Client_Word_Reg_Struc.Client_CX ; DX::CX == ESI
           pop [ebp].Client_Word_Reg_Struc.Client_DX
           pop [ebp].Client_Word_Reg_Struc.Client_SI ; DI::SI == EDI
           pop [ebp].Client_Word_Reg_Struc.Client_DI

           clc


        .ELSEIF [ebp].Client_Byte_Reg_Struc.Client_AL == 0feH
                   ;**        subfunction feH         **
                   ;**  Fill registers with register  **
                   ;**  info, and return to caller.   **

           push ebp
           push esp

           pop [ebp].Client_Word_Reg_Struc.Client_AX ; BX::AX == EBP
           pop [ebp].Client_Word_Reg_Struc.Client_BX
           pop [ebp].Client_Word_Reg_Struc.Client_CX ; DX::CX == ESP
           pop [ebp].Client_Word_Reg_Struc.Client_DX

           mov [ebp].Client_Word_Reg_Struc.Client_SI, es
           mov [ebp].Client_Word_Reg_Struc.Client_DI, ss

           clc


        .ELSEIF [ebp].Client_Byte_Reg_Struc.Client_AL == 0ffH
                   ;**        subfunction ffH         **
                   ;**  Fill registers with internal  **
                   ;**  info, and return to caller.   **


           pushd OFFSET32 VDDXRAY_Process

           pop [ebp].Client_Word_Reg_Struc.Client_AX ; BX::AX is code offset
           pop [ebp].Client_Word_Reg_Struc.Client_BX

           mov [ebp].Client_Word_Reg_Struc.Client_CX, cs
           mov [ebp].Client_Word_Reg_Struc.Client_DX, ds

           pushd OFFSET32 VX_Orig_PM_2F_Seg

           pop [ebp].Client_Word_Reg_Struc.Client_SI ; DI::SI is data offset
           pop [ebp].Client_Word_Reg_Struc.Client_DI

           clc


        .ELSE

	   stc                         ;** causes interrupt to be reflected **
				       ;** down the chain, and in R-Mode.   **
	.ENDIF


	pop edx
	pop eax                        ; restore registers saved at beginning

	ret


EndProc VDDXRAY_Process

BeginProc VDDXRAY_Int2F

	cmp     [ebp].Client_Byte_Reg_Struc.Client_AH, 03H
				       ;** is this INT 2FH function 3? **
	jne     SHORT VX_I2F_Reflect

	call    VDDXRAY_Process

	jnc     SHORT VX_I2F_Exit



VX_I2F_Reflect:
	mov     eax, 2FH          ;** interrupt to reflect **
	mov     ecx, [VX_Orig_PM_2F_Seg]
	mov     edx, [VX_Orig_PM_2F_Off]
	mov     eax, ecx
	or      eax, edx
	jz      SHORT VX_PI2F_Reflect_To_V86
	VMMjmp  Simulate_Far_Jmp

VX_PI2F_Reflect_To_V86:
	VMMcall Simulate_Iret
	VMMcall Begin_Nest_V86_Exec
        mov     eax, 2Fh
	VMMcall Exec_Int
	VMMjmp  End_Nest_Exec

VX_I2F_Exit:
	mov     eax, 2FH          ;** interrupt to reflect **
	VMMcall Simulate_Iret
	ret

EndProc VDDXRAY_Int2F



;** REAL MODE INT 2FH HANDLER - REFLECT INTERRUPT TO 'PM' HANDLER!! **

BeginProc VDDXRAY_V86_Int2F

	cmp     [ebp].Client_Byte_Reg_Struc.Client_AH, 03H
				       ;** is this INT 2FH function 3? **
	jne     SHORT VX_V86_I2F_Exit

	call    VDDXRAY_Process

	jnc     SHORT VX_V86_I2F_ExitNC



    ;** for some reason, I'm checking if it's a PM app or not! **
    ;** I suppose if it is, I've already reflected the PM int! **
    ;** So, I'll just leave the code here, as long as it works **

	test    [ebx + CB_VM_Status], VMStat_PM_App
	jz      SHORT VX_V86_I2F_Exit


VX_V86_I2F_Reflect_To_PM:
	VMMcall Begin_Nest_Exec
	VMMcall Exec_Int
	VMMcall End_Nest_Exec

VX_V86_I2F_Exit:
	stc

VX_V86_I2F_ExitNC:
	ret

EndProc VDDXRAY_V86_Int2F


VxD_CODE_ENDS



END
