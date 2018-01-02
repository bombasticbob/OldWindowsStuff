 	TITLE	TEST IOCTL INT 21H, 440DH, FCNS 40H, 42H, 47H, 60H, and 67H
	page	60,132

;---------------------------------------------------------------------------
;FORMAT12.ASM  DATE: 01-29-91  BY: Pierre Desloover for MICROSOFT PSS
;PURPOSE:  Demonstrate successful MISC. MS-DOS 5.0 IOCTL Int 21H, 440DH
;          functions (CL=40H, 42H, 47H, 60H, 67H) in order to FORMAT ALL 
;          TRACKS of a 1.2Mb diskette.
;NOTES:	   SPECIAL VERSION FORMATS ALL TRACKS OF MEDIA
;Current test set up for 1.2Mb drive and disk in Drive A: or B:.  To
;accommodate other drive and/or media types, this test code needs to be
;modified slightly.  Also, Int 21h, 440dH, FCN 42H (FORMAT TRK & Verify)
;is hard coded to format the first TRACK: HEAD #0, CYLINDER #0.  
;
;1. MS-DOS version 4.x and greater.
;2. Command line input arg is operand logical drive LETTER and can be
;   Drive A: or B: ONLY in order to protect from accidently formatting
;   hard drives C:, D:, etc. Example: FORM A: <cr>. (Formats media in Drive A:)
;3. MASM 5.0+, LINK to program .EXE image.
;---------------------------------------------------------------------------

	DOSSEG
	.MODEL	SMALL

	.DATA

;operand logical drive# (0=default, 1=A:, 2=B:, 3=C:, etc.)
drive      db ?
cylinder_CNT  dw  0   ;1st DOS cyl.
head_CNT      dw  0   ;	"   "  head

;test program messages
formatting    db 13,10,'Formatting media',13,10,10,'$'
head_cylinder db 13,'Head    Cylinder   ','$'
done_form     db 13,10,10,'Format complete','$'

usage_msg  db 13,10,'Command line usage is:  FORM D:',13,10
           db       'where D: is Drive letter A: or B:',13,10,10,'$'
error_40h  db 13,10,'Error: IOCTL Int 21H, fcn 40H, AL=  ',13,10,10,'$'
error_60h  db 13,10,'Error: IOCTL Int 21H, fcn 60H, AL=  ',13,10,10,'$'
error_67h  db 13,10,'Error: IOCTL Int 21H, fcn 67H, AL=  ',13,10,10,'$'
error_47h  db 13,10,'Error: IOCTL Int 21H, fcn 47H, AL=  ',13,10,10,'$'
error_42h  db 13,10,'Error: IOCTL Int 21H, fcn 42H, AL=  ',13,10,10,'$'
not_supp   db 13,10,'Error: IOCTL Int 21H, fcn 42H, (format status) STAT=  ',13,10,10,'$'
no_errors  db 13,10,'OK: no IOCTL Int 21H errors encountered',13,10,10,'$'
wrong_drive db 13,10,'Command line error: specify Drive A: or B: only!',13,10,10,'$'

;structure for Int 21H, fcn 42h
FVBLOCK    db 0		;special functions (must be zero)
	   dw 0		;head	          (according to FORMAT, it's zero)
	   dw 0		;cylinder	  (1st logical cylinder = 0)

;Structure for Int 21H, fcn 67h
ACCESSBLOCK db 0	;special functions (must be zero)
            db ?        ;disk access flag (0=access blocked by driver)

saved_access db  ?  ;var to save current access flag, not part of structure

;structures for Int 21H, fcn's 40H and 60H
;DPB (Device Parameter Block) for 4.x and greater
PARMBLOCK  db 0		;special functions field
	   db ?		;device type; device sets	
	   dw ?		;device attributes; device sets
	   dw ?		;max number cylinders supported; device sets
	   db ?         ;media type field; must set to quad or double density
					 ; for drives that support multiple
                                         ; densities, example 1.2M drive.
_BPB       db 31 dup (?)  ;Bios Parameter Block structure (MS-DOS 4.0+ only,
			  ;version 3.3 is smaller)		           

;*** Do not declare any vars from HERE end _BPB to TrkLayOut structure below

;The following EQUATE controls sectors/track.  Change it for target media.
SECS_TRK  =  15	   ;media descriptor: F9H = DS, 15 sectors/track for 1.2Mb
;;;SECS_TRK  =  9  ;media descriptor: FDH = DS, 9 sectors/track for 360kb
MAXHEADS = 2   ;diskette
MAXCYLS  = 80  ;1.2 media, same as total #tracks for diskette because 2 heads only
BYTES_SECT = 512   ;bytes/sector

TRACKINF MACRO 
N = 1
    REPT SECS_TRK
      DW N
      DW BYTES_SECT
      N=N+1
    ENDM
ENDM

;Start of Track Layout structure (MUST directly follow _BPB structure)
TRKLAYOUT  dw  SECS_TRK	;sectors per track

;Invoke trackinfo macro to build rest of fields for track layout structure.
;(Format.exe allocates and initializes dynamically.)
	TRACKINF  <>

	.CODE

START:	jmp 	short START_TEST

START_TEST:
	mov	ax,@DATA
	mov	ds,ax		;Note: it is assumed that DS is not changed
				;for the remainder of this test code.

;PARSE COMMAND TAIL
;Extract command tail drive letter from PSP.
;If no command line arg, don't use default drive
	mov	al,es:[80h]
	or	al,al
	jz	usage_error	;don't allow default (0) drive, could be C:

parse_cmdtail:
;Perform simple parsing of command tail
	mov     al,es:[82h]
	cmp	al,'A'
	jb	usage_error
	cmp	al,'Z'
	jbe	have_upper
	cmp	al,'a'
	jb	usage_error
	cmp	al,'z'
	jbe	have_lower

usage_error:
	mov	dx,offset usage_msg
	jmp	term_test

have_lower:
;convert to upper
	and	al,0dfh

have_upper:
;make 1 based logical drive number
	sub	al,'@'

set_drvno:
	mov	[drive],al

;safety check in this test program so don't format drive C:, D:, etc.
	cmp	al,3
	jb	_OK_proceed  		;Drive A: or B: only
	mov	dx,offset wrong_drive
	jmp	term_test
;END PARSE COMMAND TAIL

_OK_proceed:
;Issue function 60H (Get Device Parameters) to use with function 40H
;(Set Device Parameters) which is necessary prior to function 42H.
	mov	bx,offset PARMBLOCK
	mov	byte ptr [bx],00000000B	;init to zero  ;this is what FORMAT does

	mov	bl,[drive] 		;logical drive into BL
	mov	ch,8			;disk category
	mov	cl,60h  		;GET Device parms
	mov	dx,offset PARMBLOCK	;DS:DX 
	mov	ax,440dh
	int	21h
	jnc	_issue_40h

;some error happened
	mov	dx,offset error_60h
;convert error# in AL to one's digit ascii for display
	add	al,48
	mov	bx,dx
	mov	byte ptr [bx][37],al	
	jmp	term_test

_issue_40h:
;We will assume that the Device parameter block returned via CL=60H
;contains valid info although some info may need to be altered to
;match the actual MEDIA in target drive before CL=40H.
	mov	bx,offset PARMBLOCK
	mov	byte ptr [bx],00000101B  ;set bit 2=1 and bit 0=1 ;FORMAT does this
					 
;init various structure members accordingly for device type
	mov	byte ptr [bx][1],1  ;set Device type, 1=1.2Mb
	;;;mov	byte ptr [bx][1],0  ;set Device type, 0=360Kb

	mov	byte ptr [bx][6],00000000B  ;set Media type, bit 0=0 (DD,1.2Mb)
	;;;mov	byte ptr [bx][6],00000001B  ;set Media type, bit 0=1 (DD,360kb)

	mov	bl,[drive] 		;logical drive into BL
	mov	ch,8			;disk category
	mov	cl,40h  		;SET Device parms
	mov	dx,offset PARMBLOCK	;DS:DX 
	mov	ax,440dh
	int	21h
	jnc	_issue_67h

;some error happened
	mov	dx,offset error_40h
;convert error# in AL to one's digit ascii for display
	add	al,48
	mov	bx,dx
	mov	byte ptr [bx][37],al	
	jmp	term_test

_issue_67h:
;Accordying to FORMAT.EXE, need to get, set Disk Access flag for HARD only.
;(documented in Advanced MS-DOS Programming)
;Get disk access flag before set disk access flag so can save and restore.
COMMENT  |  Non functional for floppy (removable media) test
	mov	bx,offset ACCESSBLOCK
	mov	byte ptr [bx],00000000B	;special, must be zero

	mov	bl,[drive]  		;logical drive number into BL
	mov	ch,8			;disk category
	mov	cl,67h  		;Get Access Flag
	mov	dx,offset ACCESSBLOCK   ;DS:DX 
	mov	ax,440dh
	int	21h
	jnc	_issue_47h

;error happened
	mov	dx,offset error_67h
;convert error# in AL to one's digit ascii for display
	add	al,48
	mov	bx,dx
	mov	byte ptr [bx][37],al	
	jmp	term_test

_issue_47h:
;first save returned disk access flag
	mov	bx,offset ACCESSBLOCK
	mov	al,byte ptr [bx][1]
	mov	[saved_access],al

;now issue set access flag
	mov	byte ptr [bx],00000000B	;special, must be zero
	mov	byte ptr [bx][1],1 ; set disk access flag non-zero

	mov	bl,[drive]  		;logical drive number into BL
	mov	ch,8			;disk category
	mov	cl,47h  		;Set Access Flag
	mov	dx,offset ACCESSBLOCK   ;DS:DX 
	mov	ax,440dh
	int	21h
	jnc	_issue_42h

;error happened
	mov	dx,offset error_47h
;convert error# in AL to one's digit ascii for display
	add	al,48
	mov	bx,dx
	mov	byte ptr [bx][37],al	
	jmp	short term_test
	| ;;;END COMMENT

_issue_42h:
;(MS-DOS 4.0 or greater) first issue with special functions=1 so can see
;if specified number of tracks and cylinders are supported by BIOS.
;(documented in Advanced MS-DOS Programming)
	mov	bx,offset FVBLOCK
	mov	byte ptr [bx],00000001B ;special, 1= format status call
	mov	bl,[drive] 		;logical drive into BL
	mov	ch,8			;disk category
	mov	cl,42h  		;Format and Verify Track
	mov	dx,offset FVBLOCK	;DS:DX 
	mov	ax,440dh
	int	21h

jnc	test_rs
jmp	_error_42h

test_rs:
;test returned status
	mov	bx,offset FVBLOCK
	cmp	byte ptr [bx],0  	;special functions?, 0= supported
	jz	format_track	

;Format status says not supported or other error (consult Advanced MS-DOS
;Programming for exact meaning of values).
	mov	dx,offset not_supp
;convert error status# in structure to one's digit ascii for display
	mov	al,byte ptr [bx] ;get error status#
	add	al,48
	mov	bx,dx
	mov	byte ptr [bx][55],al	
	jmp	term_test

format_track:
comment	|
;attempt to format one track
	mov	bx,offset FVBLOCK
	mov	byte ptr [bx],0      ;special, must be zero (FORMAT.exe)

	mov	word ptr [bx][1],0   ;first HEAD (it's 0 according to FORMAT)
	mov	word ptr [bx][3],0   ;first CYLINDER (it's 0 according to FORMAT)

	mov	bl,[drive] 		;logical drive into BL
	mov	ch,8			;disk category
	mov	cl,42h  		;Format and Verify Track
	mov	dx,offset FVBLOCK	;DS:DX 
	mov	ax,440dh
	int	21h
	jc	_error_42h

;attempt to format one track
	mov	bx,offset FVBLOCK
	mov	byte ptr [bx],0      ;special, must be zero (FORMAT.exe)

	mov	word ptr [bx][1],1   ;first HEAD (it's 0 according to FORMAT)
	mov	word ptr [bx][3],0   ;first CYLINDER (it's 0 according to FORMAT)

	mov	bl,[drive] 		;logical drive into BL
	mov	ch,8			;disk category
	mov	cl,42h  		;Format and Verify Track
	mov	dx,offset FVBLOCK	;DS:DX 
	mov	ax,440dh
	int	21h
	jc	_error_42h
	|

mov	dx,offset formatting
mov     ah,9
int	21h

mov	[cylinder_CNT],0	;init first cylinder, zero based

NEXT_CYL:

mov	[head_CNT],0		;re-init head cnt, make zero based

NEXT_HEAD:
;display head and cylinder numbers in decimal
mov	ax,[head_CNT]
mov	bp,offset head_cylinder
push	bp
add	bp,6
call	VAL_TO_DEC_ASCII
add 	bp,12
mov	ax,[cylinder_CNT]
call	VAL_TO_DEC_ASCII
pop	dx
mov	ah,9
int	21h

;attempt to format one track
	mov	bx,offset FVBLOCK
	mov	byte ptr [bx],00000000B  ;special, zero

	mov	ax,[head_cnt]
	mov	[bx][1],ax   ;first HEAD (it's 0 according to FORMAT)
	mov	ax,[cylinder_cnt]
	mov	[bx][3],ax   ;first CYLINDER (it's 0 according to FORMAT)

	mov	bl,[drive] 		;logical drive into BL
	mov	ch,8			;disk category
	mov	cl,42h  		;Format and Verify Track
	mov	dx,offset FVBLOCK	;DS:DX 
	mov	ax,440dh
	int	21h
	jc	_error_42h

inc	[head_cnt]
cmp	[head_cnt],MAXHEADS-1		;make zero based
jbe	NEXT_HEAD
inc	[cylinder_cnt]
cmp	[cylinder_cnt],MAXCYLS-1
jbe	NEXT_CYL

no_error:
mov	dx,offset done_form
mov	ah,9
int	21h

	mov	dx,offset no_errors
	jmp	short term_test

_error_42h:
;IMPORTANT TO GET EXTENDED ERROR HERE FOR EXACT EXPLANATION OF ERROR
;AND DO APPROPRIATE ACTIONS HERE. (For example: FORMAT.exe marks whole
;TRACK BAD in FAT if "sector not found" (or other HW errors) occurs here 
;and then proceeds on to attempt to format next TRACK.)
;In this test program, all we do is report the Extended Error Value,
;and abort attempt to format the track.  
	mov	ah,59h
	xor	bx,bx
	int	21h

;convert error# in AL to tens and one's digit ascii for display
	mov	cl,48
	mov	bl,10
	div	bl
	or	al,al
	jz	_ones
	add	al,48
	mov	cl,al	;save tens digit
_ones:
	mov	al,ah	;use remainder
	add	al,48
	mov	dx,offset error_42h
	mov	bx,dx
	mov	byte ptr [bx][36],cl	;tens digit	
	mov	byte ptr [bx][37],al	;ones   "
	
term_test:
;display message on exit, DX has ptr to text string
	mov	ah,9
	int	21h

COMMENT  |  Non functional for floppy (removable media) test
;Restore saved disk access flag via fcn 47h.
	mov	bx,offset ACCESSBLOCK
	mov	byte ptr [bx],00000000B	;special, must be zero
	mov	al,[saved_access]
	mov	byte ptr [bx][1],al	;restore to previous value

	mov	bl,[drive]  		;logical drive number into BL
	mov	ch,8			;disk category
	mov	cl,47h  		;Set Access Flag
	mov	dx,offset ACCESSBLOCK   ;DS:DX 
	mov	ax,440dh
	int	21h			;report error here?
	| ;;;END COMMENT

;terminate test program	
	mov	ax,4c00h
	int	21h

VAL_TO_DEC_ASCII  proc near
;On entry: AX contains word to convert to decimal ascii.
;	   BP has ptr to buffer that will get ascii string
;On exit:  DI has length of string
;Note:	   LEADING ZERO SUPPRESSION on normal entry point.
;          CALL label val_to_dec_ascii_x with reg CL<>0
;          for NO leading zero supression.

   	xor	cl,cl			;supress leading zeros

val_to_dec_ascii_x:			;No leading zero supression
	xor	di,di			;entry point.  CL must have
					;non-zero value on entry.
ck_10000:
	mov	bx,10000
	xor	dx,dx
	div	bx
   	or	al,al
	jz	ck_1000

add_10000:
	call	store_dec_char

CK_1000:mov	ax,dx
	xor	dx,dx
	mov	bx,1000
	div	bx
   	or	al,al
   	jnz	add_1000
   	or	cl,cl
   	jz	ck_100
add_1000:
        call	store_dec_char

ck_100:	mov	ax,dx
	mov	bl,100
	div	bl
   	or	al,al
   	jnz	add_100
   	or	cl,cl
  	jz	ck_10
add_100:call	store_dec_char
	
CK_10:	mov	al,ah			;use remainder
	xor	ah,ah
	mov	bl,10
	div	bl
   	or	al,al
   	jnz	add_10
   	or	cl,cl
   	jz	ones
add_10: call	store_dec_char

ones:	mov	al,ah
	call	store_dec_char
        ret
VAL_TO_DEC_ASCII  endp

STORE_DEC_CHAR	proc  near

   	mov	cl,0ffh			;no more leading zero supress.
	mov	bl,cl			
	add	al,'0'
	mov	byte ptr ds:[bp][di],al
 	inc	di
	ret
STORE_DEC_CHAR	endp

	END	START
