	page	,132

;Thunk Compiler Version 1.8  Dec 14 1994 14:53:05
;File Compiled Tue May 02 14:15:40 1995

;Command Line: D:\SDK\BINW16\thunk.exe shlthunk.thk 

	TITLE	$shlthunk.asm

	.386
	OPTION READONLY
	OPTION OLDSTRUCTS

IFNDEF IS_16
IFNDEF IS_32
%out command line error: specify one of -DIS_16, -DIS_32
.err
ENDIF  ;IS_32
ENDIF  ;IS_16


IFDEF IS_32
IFDEF IS_16
%out command line error: you can't specify both -DIS_16 and -DIS_32
.err
ENDIF ;IS_16
;************************* START OF 32-BIT CODE *************************


	.model FLAT,STDCALL


;-- Import common flat thunk routines (in k32)

externDef MapHInstLS	:near32
externDef MapHInstLS_PN	:near32
externDef MapHInstSL	:near32
externDef MapHInstSL_PN	:near32
externDef FT_Prolog	:near32
externDef FT_Thunk	:near32
externDef QT_Thunk	:near32
externDef FT_Exit0	:near32
externDef FT_Exit4	:near32
externDef FT_Exit8	:near32
externDef FT_Exit12	:near32
externDef FT_Exit16	:near32
externDef FT_Exit20	:near32
externDef FT_Exit24	:near32
externDef FT_Exit28	:near32
externDef FT_Exit32	:near32
externDef FT_Exit36	:near32
externDef FT_Exit40	:near32
externDef FT_Exit44	:near32
externDef FT_Exit48	:near32
externDef FT_Exit52	:near32
externDef FT_Exit56	:near32
externDef SMapLS	:near32
externDef SUnMapLS	:near32
externDef SMapLS_IP_EBP_8	:near32
externDef SUnMapLS_IP_EBP_8	:near32
externDef SMapLS_IP_EBP_12	:near32
externDef SUnMapLS_IP_EBP_12	:near32
externDef SMapLS_IP_EBP_16	:near32
externDef SUnMapLS_IP_EBP_16	:near32
externDef SMapLS_IP_EBP_20	:near32
externDef SUnMapLS_IP_EBP_20	:near32
externDef SMapLS_IP_EBP_24	:near32
externDef SUnMapLS_IP_EBP_24	:near32
externDef SMapLS_IP_EBP_28	:near32
externDef SUnMapLS_IP_EBP_28	:near32
externDef SMapLS_IP_EBP_32	:near32
externDef SUnMapLS_IP_EBP_32	:near32
externDef SMapLS_IP_EBP_36	:near32
externDef SUnMapLS_IP_EBP_36	:near32
externDef SMapLS_IP_EBP_40	:near32
externDef SUnMapLS_IP_EBP_40	:near32

MapSL	PROTO NEAR STDCALL p32:DWORD



	.code 

;************************* COMMON PER-MODULE ROUTINES *************************

	.data

public shlthunk_ThunkData32	;This symbol must be exported.
shlthunk_ThunkData32 label dword
	dd	3130534ch	;Protocol 'LS01'
	dd	019df3h	;Checksum
	dd	0	;Jump table address.
	dd	3130424ch	;'LB01'
	dd	0	;Flags
	dd	0	;Reserved (MUST BE 0)
	dd	0	;Reserved (MUST BE 0)
	dd	offset QT_Thunk_shlthunk - offset shlthunk_ThunkData32
	dd	offset FT_Prolog_shlthunk - offset shlthunk_ThunkData32



	.code 


externDef ThunkConnect32@24:near32

public shlthunk_ThunkConnect32@16
shlthunk_ThunkConnect32@16:
	pop	edx
	push	offset shlthunk_ThkData16
	push	offset shlthunk_ThunkData32
	push	edx
	jmp	ThunkConnect32@24
shlthunk_ThkData16 label byte
	db	"shlthunk_ThunkData16",0


		


pfnQT_Thunk_shlthunk	dd offset QT_Thunk_shlthunk
pfnFT_Prolog_shlthunk	dd offset FT_Prolog_shlthunk
	.data
QT_Thunk_shlthunk label byte
	db	32 dup(0cch)	;Patch space.

FT_Prolog_shlthunk label byte
	db	32 dup(0cch)	;Patch space.


	.code 





;************************ START OF THUNK BODIES************************




;
public MyGetModuleUsage@4
MyGetModuleUsage@4:
	mov	cl,13
; MyGetModuleUsage(16) = MyGetModuleUsage(32) {}
;
; dword ptr [ebp+8]:  hModule
;
public IIMyGetModuleUsage@4
IIMyGetModuleUsage@4:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	word ptr [ebp+8]	;hModule: dword->word
	call	dword ptr [pfnQT_Thunk_shlthunk]
	cwde
	leave
	retn	4





;
public lpGlobalFirst@8
lpGlobalFirst@8:
	mov	cx, (2 SHL 10) + (0 SHL 8) + 12
; lpGlobalFirst(16) = lpGlobalFirst(32) {}
;
; dword ptr [ebp+8]:  lpGlobal
; dword ptr [ebp+12]:  wFlags
;
public IIlpGlobalFirst@8
IIlpGlobalFirst@8:
	call	dword ptr [pfnFT_Prolog_shlthunk]
	sub	esp,36
	mov	esi,[ebp+8]
	or	esi,esi
	jnz	L0
	push	esi
	jmp	L1
L0:
	lea	edi,[ebp-100]
	push	edi	;lpGlobal: lpstruct32->lpstruct16
	or	dword ptr [ebp-20],02h	;Set flag to fixup ESP-rel argument.
L1:
	push	word ptr [ebp+12]	;wFlags: dword->word
	call	FT_Thunk
	movzx	ebx,ax
	mov	edi,[ebp+8]
	or	edi,edi
	jz	L2
	lea	esi,[ebp-100]	;lpGlobal  Struct16->Struct32
	movsd
	movsd
	movsd
	xor	eax,eax
	lodsw
	stosd
	movsd
	movsw
	inc	edi
	inc	edi
	xor	eax,eax
	lodsw
	stosd
	lodsw
	stosd
	movsd
	movsd
	movsd
L2:
	jmp	FT_Exit8





;
public lpGlobalNext@8
lpGlobalNext@8:
	mov	cx, (2 SHL 10) + (0 SHL 8) + 11
; lpGlobalNext(16) = lpGlobalNext(32) {}
;
; dword ptr [ebp+8]:  lpGlobal
; dword ptr [ebp+12]:  wFlags
;
public IIlpGlobalNext@8
IIlpGlobalNext@8:
	call	dword ptr [pfnFT_Prolog_shlthunk]
	sub	esp,36
	mov	esi,[ebp+8]
	or	esi,esi
	jnz	L3
	push	esi
	jmp	L4
L3:
	lea	edi,[ebp-100]
	push	edi	;lpGlobal: lpstruct32->lpstruct16
	or	dword ptr [ebp-20],02h	;Set flag to fixup ESP-rel argument.
	movsd
	movsd
	movsd
	lodsd
	stosw
	movsd
	movsw
	inc	esi
	inc	esi
	lodsd
	stosw
	lodsd
	stosw
	movsd
	movsd
	movsd
L4:
	push	word ptr [ebp+12]	;wFlags: dword->word
	call	FT_Thunk
	movzx	ebx,ax
	mov	edi,[ebp+8]
	or	edi,edi
	jz	L5
	lea	esi,[ebp-100]	;lpGlobal  Struct16->Struct32
	movsd
	movsd
	movsd
	xor	eax,eax
	lodsw
	stosd
	movsd
	movsw
	inc	edi
	inc	edi
	xor	eax,eax
	lodsw
	stosd
	lodsw
	stosd
	movsd
	movsd
	movsd
L5:
	jmp	FT_Exit8





;
public lpMemManInfo@4
lpMemManInfo@4:
	mov	cl,10
; lpMemManInfo(16) = lpMemManInfo(32) {}
;
; dword ptr [ebp+8]:  lpInfo
;
public IIlpMemManInfo@4
IIlpMemManInfo@4:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	call	SMapLS_IP_EBP_8
	push	eax
	call	dword ptr [pfnQT_Thunk_shlthunk]
	movzx	eax,ax
	call	SUnMapLS_IP_EBP_8
	leave
	retn	4





;
public lpModuleFirst@4
lpModuleFirst@4:
	mov	cx, (1 SHL 10) + (0 SHL 8) + 9
; lpModuleFirst(16) = lpModuleFirst(32) {}
;
; dword ptr [ebp+8]:  lpModule
;
public IIlpModuleFirst@4
IIlpModuleFirst@4:
	call	dword ptr [pfnFT_Prolog_shlthunk]
	sub	esp,276
	mov	esi,[ebp+8]
	or	esi,esi
	jnz	L6
	push	esi
	jmp	L7
L6:
	lea	edi,[ebp-340]
	push	edi	;lpModule: lpstruct32->lpstruct16
	or	dword ptr [ebp-20],01h	;Set flag to fixup ESP-rel argument.
L7:
	call	FT_Thunk
	movzx	ebx,ax
	mov	edi,[ebp+8]
	or	edi,edi
	jz	L8
	lea	esi,[ebp-340]	;lpModule  Struct16->Struct32
	movsd
	movsd
	movsd
	movsw
	inc	edi
	inc	edi
	xor	eax,eax
	lodsw
	stosd
	mov	ecx,65
	rep	movsd
L8:
	jmp	FT_Exit4





;
public lpModuleNext@4
lpModuleNext@4:
	mov	cx, (1 SHL 10) + (0 SHL 8) + 8
; lpModuleNext(16) = lpModuleNext(32) {}
;
; dword ptr [ebp+8]:  lpModule
;
public IIlpModuleNext@4
IIlpModuleNext@4:
	call	dword ptr [pfnFT_Prolog_shlthunk]
	sub	esp,276
	mov	esi,[ebp+8]
	or	esi,esi
	jnz	L9
	push	esi
	jmp	L10
L9:
	lea	edi,[ebp-340]
	push	edi	;lpModule: lpstruct32->lpstruct16
	or	dword ptr [ebp-20],01h	;Set flag to fixup ESP-rel argument.
	movsd
	movsd
	movsd
	movsw
	inc	esi
	inc	esi
	lodsd
	stosw
	mov	ecx,65
	rep	movsd
L10:
	call	FT_Thunk
	movzx	ebx,ax
	mov	edi,[ebp+8]
	or	edi,edi
	jz	L11
	lea	esi,[ebp-340]	;lpModule  Struct16->Struct32
	movsd
	movsd
	movsd
	movsw
	inc	edi
	inc	edi
	xor	eax,eax
	lodsw
	stosd
	mov	ecx,65
	rep	movsd
L11:
	jmp	FT_Exit4





;
public lpTaskFindHandle@8
lpTaskFindHandle@8:
	mov	cx, (2 SHL 10) + (0 SHL 8) + 7
; lpTaskFindHandle(16) = lpTaskFindHandle(32) {}
;
; dword ptr [ebp+8]:  lpTask
; dword ptr [ebp+12]:  hTask
;
public IIlpTaskFindHandle@8
IIlpTaskFindHandle@8:
	call	dword ptr [pfnFT_Prolog_shlthunk]
	sub	esp,40
	mov	esi,[ebp+8]
	or	esi,esi
	jnz	L12
	push	esi
	jmp	L13
L12:
	lea	edi,[ebp-104]
	push	edi	;lpTask: lpstruct32->lpstruct16
	or	dword ptr [ebp-20],02h	;Set flag to fixup ESP-rel argument.
L13:
	push	word ptr [ebp+12]	;hTask: dword->word
	call	FT_Thunk
	movzx	ebx,ax
	mov	edi,[ebp+8]
	or	edi,edi
	jz	L14
	lea	esi,[ebp-104]	;lpTask  Struct16->Struct32
	movsd
	xor	eax,eax
	mov	ecx,4
@@:
	lodsw
	stosd
	loop	@B
	movsd
	movsd
	movsd
	xor	eax,eax
	lodsw
	stosd
	movsd
	movsd
	movsd
	xor	eax,eax
	lodsw
	stosd
L14:
	jmp	FT_Exit8





;
public lpModuleFindHandle@8
lpModuleFindHandle@8:
	mov	cx, (2 SHL 10) + (0 SHL 8) + 6
; lpModuleFindHandle(16) = lpModuleFindHandle(32) {}
;
; dword ptr [ebp+8]:  lpModule
; dword ptr [ebp+12]:  hModule
;
public IIlpModuleFindHandle@8
IIlpModuleFindHandle@8:
	call	dword ptr [pfnFT_Prolog_shlthunk]
	sub	esp,276
	mov	esi,[ebp+8]
	or	esi,esi
	jnz	L15
	push	esi
	jmp	L16
L15:
	lea	edi,[ebp-340]
	push	edi	;lpModule: lpstruct32->lpstruct16
	or	dword ptr [ebp-20],02h	;Set flag to fixup ESP-rel argument.
L16:
	push	word ptr [ebp+12]	;hModule: dword->word
	call	FT_Thunk
	movzx	ebx,ax
	mov	edi,[ebp+8]
	or	edi,edi
	jz	L17
	lea	esi,[ebp-340]	;lpModule  Struct16->Struct32
	movsd
	movsd
	movsd
	movsw
	inc	edi
	inc	edi
	xor	eax,eax
	lodsw
	stosd
	mov	ecx,65
	rep	movsd
L17:
	jmp	FT_Exit8





;
public lpIsTask@4
lpIsTask@4:
	mov	cl,5
; lpIsTask(16) = lpIsTask(32) {}
;
; dword ptr [ebp+8]:  hTask
;
public IIlpIsTask@4
IIlpIsTask@4:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	word ptr [ebp+8]	;hTask: dword->word
	call	dword ptr [pfnQT_Thunk_shlthunk]
	movzx	eax,ax
	leave
	retn	4





;
public lpTaskFirst@4
lpTaskFirst@4:
	mov	cx, (1 SHL 10) + (0 SHL 8) + 4
; lpTaskFirst(16) = lpTaskFirst(32) {}
;
; dword ptr [ebp+8]:  lpTask
;
public IIlpTaskFirst@4
IIlpTaskFirst@4:
	call	dword ptr [pfnFT_Prolog_shlthunk]
	sub	esp,40
	mov	esi,[ebp+8]
	or	esi,esi
	jnz	L18
	push	esi
	jmp	L19
L18:
	lea	edi,[ebp-104]
	push	edi	;lpTask: lpstruct32->lpstruct16
	or	dword ptr [ebp-20],01h	;Set flag to fixup ESP-rel argument.
L19:
	call	FT_Thunk
	movzx	ebx,ax
	mov	edi,[ebp+8]
	or	edi,edi
	jz	L20
	lea	esi,[ebp-104]	;lpTask  Struct16->Struct32
	movsd
	xor	eax,eax
	mov	ecx,4
@@:
	lodsw
	stosd
	loop	@B
	movsd
	movsd
	movsd
	xor	eax,eax
	lodsw
	stosd
	movsd
	movsd
	movsd
	xor	eax,eax
	lodsw
	stosd
L20:
	jmp	FT_Exit4





;
public lpTaskNext@4
lpTaskNext@4:
	mov	cx, (1 SHL 10) + (0 SHL 8) + 3
; lpTaskNext(16) = lpTaskNext(32) {}
;
; dword ptr [ebp+8]:  lpTask
;
public IIlpTaskNext@4
IIlpTaskNext@4:
	call	dword ptr [pfnFT_Prolog_shlthunk]
	sub	esp,40
	mov	esi,[ebp+8]
	or	esi,esi
	jnz	L21
	push	esi
	jmp	L22
L21:
	lea	edi,[ebp-104]
	push	edi	;lpTask: lpstruct32->lpstruct16
	or	dword ptr [ebp-20],01h	;Set flag to fixup ESP-rel argument.
	movsd
	mov	ecx,4
@@:
	lodsd
	stosw
	loop	@B
	movsd
	movsd
	movsd
	lodsd
	stosw
	movsd
	movsd
	movsd
	lodsd
	stosw
L22:
	call	FT_Thunk
	movzx	ebx,ax
	mov	edi,[ebp+8]
	or	edi,edi
	jz	L23
	lea	esi,[ebp-104]	;lpTask  Struct16->Struct32
	movsd
	xor	eax,eax
	mov	ecx,4
@@:
	lodsw
	stosd
	loop	@B
	movsd
	movsd
	movsd
	xor	eax,eax
	lodsw
	stosd
	movsd
	movsd
	movsd
	xor	eax,eax
	lodsw
	stosd
L23:
	jmp	FT_Exit4





;
public GetDosMaxDrives@0
GetDosMaxDrives@0:
	mov	cl,2
; GetDosMaxDrives(16) = GetDosMaxDrives(32) {}
;
;
public IIGetDosMaxDrives@0
IIGetDosMaxDrives@0:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	call	dword ptr [pfnQT_Thunk_shlthunk]
	movzx	eax,ax
	leave
	retn





;
public GetTDBFromPSP@4
GetTDBFromPSP@4:
	mov	cl,1
; GetTDBFromPSP(16) = GetTDBFromPSP(32) {}
;
; dword ptr [ebp+8]:  wPSPSeg
;
public IIGetTDBFromPSP@4
IIGetTDBFromPSP@4:
	push	ebp
	mov	ebp,esp
	push	ecx
	sub	esp,60
	push	word ptr [ebp+8]	;wPSPSeg: dword->word
	call	dword ptr [pfnQT_Thunk_shlthunk]
	push	dx
	push	ax
	call	MapSL
	leave
	retn	4





;
public GetTaskAndModuleFromPSP@12
GetTaskAndModuleFromPSP@12:
	mov	cx, (3 SHL 10) + (0 SHL 8) + 0
; GetTaskAndModuleFromPSP(16) = GetTaskAndModuleFromPSP(32) {}
;
; dword ptr [ebp+8]:  wMachID
; dword ptr [ebp+12]:  wPSPSeg
; dword ptr [ebp+16]:  lphTask
;
public IIGetTaskAndModuleFromPSP@12
IIGetTaskAndModuleFromPSP@12:
	call	dword ptr [pfnFT_Prolog_shlthunk]
	xor	eax,eax
	push	eax
	mov	edx, dword ptr [ebp+16]
	or	edx,edx
	jz	@F
	or	dword ptr [edx], 0
@@:
	push	word ptr [ebp+8]	;wMachID: dword->word
	push	word ptr [ebp+12]	;wPSPSeg: dword->word
	mov	eax, dword ptr [ebp+16]
	call	SMapLS
	mov	[ebp-68],edx
	push	eax
	call	FT_Thunk
	push	dx
	push	ax
	call	MapSL
	mov	ebx,eax
	mov	edx, dword ptr [ebp+16]
	or	edx,edx
	jz	L24
	mov	word ptr [edx+2], 0
L24:
	mov	ecx, dword ptr [ebp-68]
	call	SUnMapLS
	jmp	FT_Exit12




ELSE
;************************* START OF 16-BIT CODE *************************




	OPTION SEGMENT:USE16
	.model LARGE,PASCAL


	.code	



externDef GetTaskAndModuleFromPSP:far16
externDef GetTDBFromPSP:far16
externDef GetDosMaxDrives:far16
externDef lpTaskNext:far16
externDef lpTaskFirst:far16
externDef lpIsTask:far16
externDef lpModuleFindHandle:far16
externDef lpTaskFindHandle:far16
externDef lpModuleNext:far16
externDef lpModuleFirst:far16
externDef lpMemManInfo:far16
externDef lpGlobalNext:far16
externDef lpGlobalFirst:far16
externDef MyGetModuleUsage:far16


FT_shlthunkTargetTable label word
	dw	offset GetTaskAndModuleFromPSP
	dw	   seg GetTaskAndModuleFromPSP
	dw	offset GetTDBFromPSP
	dw	   seg GetTDBFromPSP
	dw	offset GetDosMaxDrives
	dw	   seg GetDosMaxDrives
	dw	offset lpTaskNext
	dw	   seg lpTaskNext
	dw	offset lpTaskFirst
	dw	   seg lpTaskFirst
	dw	offset lpIsTask
	dw	   seg lpIsTask
	dw	offset lpModuleFindHandle
	dw	   seg lpModuleFindHandle
	dw	offset lpTaskFindHandle
	dw	   seg lpTaskFindHandle
	dw	offset lpModuleNext
	dw	   seg lpModuleNext
	dw	offset lpModuleFirst
	dw	   seg lpModuleFirst
	dw	offset lpMemManInfo
	dw	   seg lpMemManInfo
	dw	offset lpGlobalNext
	dw	   seg lpGlobalNext
	dw	offset lpGlobalFirst
	dw	   seg lpGlobalFirst
	dw	offset MyGetModuleUsage
	dw	   seg MyGetModuleUsage




	.data

public shlthunk_ThunkData16	;This symbol must be exported.
shlthunk_ThunkData16	dd	3130534ch	;Protocol 'LS01'
	dd	019df3h	;Checksum
	dw	offset FT_shlthunkTargetTable
	dw	seg    FT_shlthunkTargetTable
	dd	0	;First-time flag.



	.code 


externDef ThunkConnect16:far16

public shlthunk_ThunkConnect16
shlthunk_ThunkConnect16:
	pop	ax
	pop	dx
	push	seg    shlthunk_ThunkData16
	push	offset shlthunk_ThunkData16
	push	seg    shlthunk_ThkData32
	push	offset shlthunk_ThkData32
	push	cs
	push	dx
	push	ax
	jmp	ThunkConnect16
shlthunk_ThkData32 label byte
	db	"shlthunk_ThunkData32",0





ENDIF
END
