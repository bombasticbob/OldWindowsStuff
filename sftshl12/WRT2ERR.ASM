
                        ;**    __wrt2err       **

                        ;** Windows entry code **

_DATA SEGMENT WORD PUBLIC 'DATA'

EXTERNDEF __nfile:WORD, __qwinused:WORD, __wwflags:WORD

_DATA ENDS


DGROUP GROUP _DATA


_TEXT SEGMENT WORD PUBLIC 'CODE'
_TEXT ENDS




_wwrite PROTO NEAR C

_GetDGROUP PROTO FAR C



_TEXT SEGMENT

ASSUME DS:DGROUP

_wrt2err PROC FAR C PUBLIC              ;** ensure it has NO frame! **

   MOV     AX,DS
   NOP

   INC     BP
   PUSH    BP
   MOV     BP,SP
   PUSH    DS
   MOV     DS,AX


   PUSH    DS
   CALL    _GetDGROUP
   MOV     DS,AX
   MOV     AX,[__nfile]
   CMP     WORD PTR [__qwinused],+00
   POP     DS
   JZ      L144

   ;** This is the 'QWIN' code here!

   MOV     CL,[BX]
   SUB     CH,CH
   INC     BX
   MOV     DX,BX
   MOV     BX,AX
   CMP     BX,+02
   JZ      L130
   CALL    __wwrite
   JMP     L144


L130:

   MOV     AH,40
   CS:
   TEST    WORD PTR [__wflags],0001
   JZ      L142

   CALL    DOS3CALL
   JMP     L144


L142:

   INT     21

L144:


   ;** WINDOWS EXIT CODE

   SUB     BP,+02
   MOV     SP,BP
   POP     DS
   POP     BP
   DEC     BP
   RETF

_wrt2err ENDP

_TEXT ENDS
