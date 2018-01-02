TITLE BOOT RECORD

;**************************************************************************
;** This is the 'BOOT RECORD' for a formatted floppy drive.  Modify the ***
;** boot record as required for the particular disk being formatted.    ***
;** To be stored as a resource for reference by WINCMD when needed.     ***
;**************************************************************************


_ABS_ZERO SEGMENT BYTE AT 0

ORG 7C00H

L7C00  dw ?       ; BOOT

ORG 7C0BH
L7C0B  dw ?       ; BYTES_SEC
L7C0D  db ?       ; CLUSTSIZE
L7C0E  dw ?       ; RESERVED
L7C10  db ?       ; NUMFATS
L7C11  dw ?       ; ROOTSIZE
L7C13  dw ?       ; TOTLSECTR
L7C15  db ?       ; MEDIACODE
L7C16  dw ?       ; SECPERFAT
L7C18  dw ?       ; SECPERTRK
L7C1A  dw ?       ; NUMHEADS
L7C1C  dw ?       ; NUMHIDSEC  (dd)
L7C1E  dw ?
L7C20  dw ?       ; DOS4STUFF  (dd)
L7C22  dw ?
L7C24  db ?       ; zero
L7C25  db ?       ; zero

ORG 7C38H
L7C38  dw ?

ORG 7C3EH
L7C3E  dw ?       ; START_CODE

ORG 7C49H
L7C49  dw ?
L7C4B  dw ?
L7C4D  dw ?
L7C4F  db ?
L7C50  dw ?
L7C52  dw ?


ORG 7D9EH       ; BOOTERROR
L7D9E  dw ?

ORG 7DE6H       ; IO_SYS
L7DE6  dw ?

_ABS_ZERO ENDS

_STARTUP_TEXT SEGMENT AT 70H

ORG 0
STARTUP LABEL FAR

_STARTUP_TEXT ENDS


_TEXT SEGMENT PARA PUBLIC 'CODE'

assume CS:_TEXT, DS:_ABS_ZERO, ES:NOTHING, SS:NOTHING


ORG 00H   ; it's actually at 7C00, but we want to start here!

BOOT:     JMP SHORT START_CODE
          NOP


OEMNAME   DB 'SFTSHELL'   ; 03H    ; exactly 8 characters

BYTES_SEC DW 0200h        ; 0BH    ; 512 bytes/sector
CLUSTSIZE DB 2            ; 0DH    ; 2 sectors per cluster
RESERVED  DW 1            ; 0EH    ; only boot sector is reserved
NUMFATS   DB 2            ; 10H    ; by convention, 2 FAT's

          ;** the following items must be updated **

ROOTSIZE  DW 112          ; 11H    ; for 360k/720k; 224 for 1.2/1.44
TOTLSECTR DW 720          ; 13H    ; 720=360k; 1440=720k;
                                   ; 2400=1.2M; 2880=1.44m
MEDIACODE DB 0fdh         ; 15H    ; 0fd=360k; 0f9=720k, 0f9=1.2m; 0f0=1.44m
SECPERFAT DW 2            ; 16H    ;   2=360k;   3=720k;   7=1.2m;   9=1.44m
SECPERTRK DW 9            ; 18H    ; for 360k/720k; 15 for 1.2m; 18 for 1.44m

          ;** the remaining items stay the same **

NUMHEADS  DW 2            ; 1AH    ; all diskettes have 2 heads
NUMHIDSEC DD 0            ; 1CH    ; no hidden sectors
DOS4STUFF DD 0            ; 20H    ; DOS 4+ total sectors (N/A)
          DB 0            ; 24H    ; Physical drive #
          DB 0            ; 25H    ; reserved
EXTBOOTSG DB 29H          ; 26H    ; signature (29H) for extended boot info
VOLSERIAL DD 0            ; 27H    ; 32-bit volume serial #
          DB 'NO NAME    '; 2BH    ; 11 byte volume label
          DB 'FAT12   '   ; 36H    ; 8 bytes (reserved?)


          ;** 3EH **

START_CODE:

          CLI                    ; prevent interrupts
          XOR     AX,AX          ; load AX with zero
          MOV     SS,AX          ; load SS with zero
          MOV     SP, OFFSET L7C00; new stack ptr at 0:7C00H
          PUSH    SS             ; transfer SS to ES
          POP     ES
          MOV     BX,0078H       ; int 1E vector address
          LDS     SI,SS:[BX]     ; place seg:offset into DS:SI
          PUSH    DS
          PUSH    SI
          PUSH    SS
          PUSH    BX             ; save DS,SI,SS,BX
          MOV     DI,OFFSET L7C3E
          MOV     CX,000BH       ; 11 bytes to transfer
          CLD                    ; going up
          REPZ    MOVSB          ; moves 11 bytes
          PUSH    ES             ; ES==SS at this time
          POP     DS             ; now DS==SS
          MOV     BYTE PTR [DI-02],0FH
          MOV     CX, L7C18
          MOV     [DI-07],CL
          MOV     [BX+02],AX
          MOV     WORD PTR [BX], OFFSET L7C3E
          STI
          INT     13H            ; AH=0 - reset disk
          JB      L01ED
          XOR     AX,AX
          CMP     L7C13,AX
          JZ      L0184
          MOV     CX,L7C13
          MOV     L7C20,CX

L0184:
          MOV     AL,L7C10
          MUL     WORD PTR L7C16
          ADD     AX,L7C1C
          ADC     DX,L7C1E
          ADD     AX,L7C0E
          ADC     DX,+00
          MOV     L7C50,AX
          MOV     L7C52,DX
          MOV     L7C49,AX
          MOV     L7C4B,DX
          MOV     AX,0020H
          MUL     WORD PTR L7C11
          MOV     BX,L7C0B
          ADD     AX,BX
          DEC     AX
          DIV     BX
          ADD     L7C49,AX
          ADC     WORD PTR L7C4B,+00
          MOV     BX,0500H
          MOV     DX,L7C52
          MOV     AX,L7C50
          CALL    L0260
          JB      L01ED
          MOV     AL,01
          CALL    L0281
          JB      L01ED
          MOV     DI,BX
          MOV     CX,000BH
          MOV     SI,OFFSET L7DE6  ; IO_SYS
          REPZ    CMPSB
          JNZ     L01ED
          LEA     DI,[BX+20H]
          MOV     CX,000BH
          REPZ    CMPSB
          JZ      L0205


          ;** ERROR BOOTING UP THE DISK **  L01ED

L01ED:
          MOV     SI,OFFSET L7D9E  ; BOOTERROR
          CALL    L0252
          XOR     AX,AX
          INT     16H
          POP     SI
          POP     DS
          POP     [SI]
          POP     [SI+02]
          INT     19H

L0200:
          POP     AX
          POP     AX
          POP     AX
          JMP     L01ED

L0205:
          MOV     AX,[BX+1AH]
          DEC     AX
          DEC     AX
          MOV     BL,L7C0D
          XOR     BH,BH
          MUL     BX
          ADD     AX,L7C49
          ADC     DX,L7C4B
          MOV     BX,0700H
          MOV     CX,0003H
L0220:
          PUSH    AX
          PUSH    DX
          PUSH    CX
          CALL    L0260
          JB      L0200
          MOV     AL,01
          CALL    L0281
          POP     CX
          POP     DX
          POP     AX
          JB      L01ED
          ADD     AX,0001H
          ADC     DX,+00
          ADD     BX,L7C0B
          LOOP    L0220
          MOV     CH,L7C15
          MOV     DL,L7C24
          MOV     BX,L7C49
          MOV     AX,L7C4B
          JMP     STARTUP

L0252:
          LODSB
          OR      AL,AL
          JZ      L0280
          MOV     AH,0EH
          MOV     BX,0007H
          INT     10H
          JMP     L0252

L0260:
          CMP     DX,L7C18
          JNB     L027F
          DIV     WORD PTR L7C18
          INC     DL
          MOV     L7C4F,DL
          XOR     DX,DX
          DIV     WORD PTR L7C1A
          MOV     L7C25,DL
          MOV     L7C4D,AX
          CLC
          RET

L027F:
          STC

L0280:
          RET

L0281:
          MOV     AH,02
          MOV     DX,L7C4D
          MOV     CL,06
          SHL     DH,CL
          OR      DH,L7C4F
          MOV     CX,DX
          XCHG    CH,CL
          MOV     DL,L7C24
          MOV     DH,L7C25
          INT     13H
          RET


BOOTERROR DB 0dh, 0ah
          DB 'Non-System disk or disk error'
          DB 0dh, 0ah
          DB 'Replace and press any key when ready'
          DB 0dh, 0ah, 00h

IO_SYS    DB 'IO      SYS'
MSDOS_SYS DB 'MSDOS   SYS'

          ORG BOOT + 0200H - 2  ; 2 bytes before end of boot sector

BOOT_SIG  DB 055h, 0aah


_TEXT ENDS






;1B29:013E FA            CLI                    ; prevent interrupts
;1B29:013F 33C0          XOR     AX,AX          ; load AX with zero
;1B29:0141 8ED0          MOV     SS,AX          ; load SS with zero
;1B29:0143 BC007C        MOV     SP,7C00        ; new stack ptr at 0:7C00H
;1B29:0146 16            PUSH    SS             ; transfer SS to ES
;1B29:0147 07            POP     ES
;1B29:0148 BB7800        MOV     BX,0078        ; int 1E vector address
;1B29:014B 36C537        LDS     SI,SS:[BX]     ; place seg:offset into DS:SI
;1B29:014E 1E            PUSH    DS
;1B29:014F 56            PUSH    SI
;1B29:0150 16            PUSH    SS
;1B29:0151 53            PUSH    BX             ; save DS,SI,SS,BX
;1B29:0152 BF3E7C        MOV     DI,7C3E
;1B29:0155 B90B00        MOV     CX,000B        ; 11 bytes to transfer
;1B29:0158 FC            CLD                    ; going up
;1B29:0159 F3A4          REPZ MOVSB             ; moves 11 bytes
;1B29:015B 06            PUSH    ES             ; ES==SS at this time
;1B29:015C 1F            POP     DS             ; now DS==SS
;1B29:015D C645FE0F      MOV     BYTE PTR [DI-02],0F
;1B29:0161 8B0E187C      MOV     CX,[7C18]
;1B29:0165 884DF9        MOV     [DI-07],CL
;1B29:0168 894702        MOV     [BX+02],AX
;1B29:016B C7073E7C      MOV     WORD PTR [BX],7C3E
;1B29:016F FB            STI
;1B29:0170 CD13          INT     13             ; AH=0 - reset disk
;1B29:0172 7279          JB      L01ED
;1B29:0174 33C0          XOR     AX,AX
;1B29:0176 3906137C      CMP     [7C13],AX
;1B29:017A 7408          JZ      L0184
;1B29:017C 8B0E137C      MOV     CX,[7C13]
;1B29:0180 890E207C      MOV     [7C20],CX
;
;L0184:
;1B29:0184 A0107C        MOV     AL,[7C10]
;1B29:0187 F726167C      MUL     WORD PTR [7C16]
;1B29:018B 03061C7C      ADD     AX,[7C1C]
;1B29:018F 13161E7C      ADC     DX,[7C1E]
;1B29:0193 03060E7C      ADD     AX,[7C0E]
;1B29:0197 83D200        ADC     DX,+00
;1B29:019A A3507C        MOV     [7C50],AX
;1B29:019D 8916527C      MOV     [7C52],DX
;1B29:01A1 A3497C        MOV     [7C49],AX
;1B29:01A4 89164B7C      MOV     [7C4B],DX
;1B29:01A8 B82000        MOV     AX,0020
;1B29:01AB F726117C      MUL     WORD PTR [7C11]
;1B29:01AF 8B1E0B7C      MOV     BX,[7C0B]
;1B29:01B3 03C3          ADD     AX,BX
;1B29:01B5 48            DEC     AX
;1B29:01B6 F7F3          DIV     BX
;1B29:01B8 0106497C      ADD     [7C49],AX
;1B29:01BC 83164B7C00    ADC     WORD PTR [7C4B],+00
;1B29:01C1 BB0005        MOV     BX,0500
;1B29:01C4 8B16527C      MOV     DX,[7C52]
;1B29:01C8 A1507C        MOV     AX,[7C50]
;1B29:01CB E89200        CALL    L0260
;1B29:01CE 721D          JB      L01ED
;1B29:01D0 B001          MOV     AL,01
;1B29:01D2 E8AC00        CALL    L0281
;1B29:01D5 7216          JB      L01ED
;1B29:01D7 8BFB          MOV     DI,BX
;1B29:01D9 B90B00        MOV     CX,000B
;1B29:01DC BEE67D        MOV     SI,7DE6
;1B29:01DF F3            REPZ
;1B29:01E0 A6            CMPSB
;1B29:01E1 750A          JNZ     L01ED
;1B29:01E3 8D7F20        LEA     DI,[BX+20]
;1B29:01E6 B90B00        MOV     CX,000B
;1B29:01E9 F3            REPZ
;1B29:01EA A6            CMPSB
;1B29:01EB 7418          JZ      L0205
;
;L01ED:
;1B29:01ED BE9E7D        MOV     SI,7D9E
;1B29:01F0 E85F00        CALL    L0252
;1B29:01F3 33C0          XOR     AX,AX
;1B29:01F5 CD16          INT     16
;1B29:01F7 5E            POP     SI
;1B29:01F8 1F            POP     DS
;1B29:01F9 8F04          POP     [SI]
;1B29:01FB 8F4402        POP     [SI+02]
;1B29:01FE CD19          INT     19
;
;L0200:
;1B29:0200 58            POP     AX
;1B29:0201 58            POP     AX
;1B29:0202 58            POP     AX
;1B29:0203 EBE8          JMP     L01ED
;
;L0205:
;1B29:0205 8B471A        MOV     AX,[BX+1A]
;1B29:0208 48            DEC     AX
;1B29:0209 48            DEC     AX
;1B29:020A 8A1E0D7C      MOV     BL,[7C0D]
;1B29:020E 32FF          XOR     BH,BH
;1B29:0210 F7E3          MUL     BX
;1B29:0212 0306497C      ADD     AX,[7C49]
;1B29:0216 13164B7C      ADC     DX,[7C4B]
;1B29:021A BB0007        MOV     BX,0700
;1B29:021D B90300        MOV     CX,0003
;1B29:0220 50            PUSH    AX
;1B29:0221 52            PUSH    DX
;1B29:0222 51            PUSH    CX
;1B29:0223 E83A00        CALL    L0260
;1B29:0226 72D8          JB      L0200
;1B29:0228 B001          MOV     AL,01
;1B29:022A E85400        CALL    L0281
;1B29:022D 59            POP     CX
;1B29:022E 5A            POP     DX
;1B29:022F 58            POP     AX
;1B29:0230 72BB          JB      01ED
;1B29:0232 050100        ADD     AX,0001
;1B29:0235 83D200        ADC     DX,+00
;1B29:0238 031E0B7C      ADD     BX,[7C0B]
;1B29:023C E2E2          LOOP    0220
;1B29:023E 8A2E157C      MOV     CH,[7C15]
;1B29:0242 8A16247C      MOV     DL,[7C24]
;1B29:0246 8B1E497C      MOV     BX,[7C49]
;1B29:024A A14B7C        MOV     AX,[7C4B]
;1B29:024D EA00007000    JMP     0070:0000
;
;L0252:
;1B29:0252 AC            LODSB
;1B29:0253 0AC0          OR      AL,AL
;1B29:0255 7429          JZ      L0280
;1B29:0257 B40E          MOV     AH,0E
;1B29:0259 BB0700        MOV     BX,0007
;1B29:025C CD10          INT     10
;1B29:025E EBF2          JMP     L0252
;
;L0260:
;1B29:0260 3B16187C      CMP     DX,[7C18]
;1B29:0264 7319          JNB     L027F
;1B29:0266 F736187C      DIV     WORD PTR [7C18]
;1B29:026A FEC2          INC     DL
;1B29:026C 88164F7C      MOV     [7C4F],DL
;1B29:0270 33D2          XOR     DX,DX
;1B29:0272 F7361A7C      DIV     WORD PTR [7C1A]
;1B29:0276 8816257C      MOV     [7C25],DL
;1B29:027A A34D7C        MOV     [7C4D],AX
;1B29:027D F8            CLC
;1B29:027E C3            RET
;
;L027F:
;1B29:027F F9            STC
;
;L0280:
;1B29:0280 C3            RET
;
;L0281:
;1B29:0281 B402          MOV     AH,02
;1B29:0283 8B164D7C      MOV     DX,[7C4D]
;1B29:0287 B106          MOV     CL,06
;1B29:0289 D2E6          SHL     DH,CL
;1B29:028B 0A364F7C      OR      DH,[7C4F]
;1B29:028F 8BCA          MOV     CX,DX
;1B29:0291 86E9          XCHG    CH,CL
;1B29:0293 8A16247C      MOV     DL,[7C24]
;1B29:0297 8A36257C      MOV     DH,[7C25]
;1B29:029B CD13          INT     13
;1B29:029D C3            RET

END BOOT
