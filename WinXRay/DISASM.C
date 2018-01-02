/***************************************************************************/
/*                                                                         */
/*         DISASSEMBLER - 'Disassemble()' and supporting functions         */
/*                                                                         */
/*  Call 'Disassemble()' with FAR pointer to instruction to disassemble    */
/*  returns pointer to NEXT instruction and text of 'disassembled' code.   */
/*  Caller must select default CODE/DATA size (16-bit or 32-bit).          */
/*                                                                         */
/***************************************************************************/


LPCSTR FAR PASCAL Disassemble(const char __huge *lpCode, LPSTR lpBuf,
                              BOOL bUse32, DWORD FAR *lpdwOff32);
// NOTE:  'lpdwOff32' enables disassembly of FLAT model 32-bit code.  If
//        not NULL, this value is used as the 'EIP'; otherwise, the OFFSET
//        of 'lpCode' is used as EIP.


#define CODEBASESTR static const char __based(__segname("_CODE"))

//   'I' = IMM DATA (SHORT FORM), 'd' = DIR (TO MEM), 'w' = WORD, 's' = SIGN EXTEND
//   IF 'I' SET, 'd' MUST ALWAYS BE 0 (DEST ALWAYS al/ax/eax)

CODEBASESTR pAAA[]    = "AAA      ";
CODEBASESTR pAAD[]    = "AAD      ";
CODEBASESTR pAAM[]    = "AAM      ";
CODEBASESTR pAAS[]    = "AAS      ";
CODEBASESTR pADC[]    = "ADC      ";
CODEBASESTR pADD[]    = "ADD      ";
CODEBASESTR pAND[]    = "AND      ";
CODEBASESTR pARPL[]   = "ARPL     ";
CODEBASESTR pBOUND[]  = "BOUND    ";
CODEBASESTR pBSF[]    = "BSF      ";
CODEBASESTR pBSR[]    = "BSR      ";
CODEBASESTR pBT[]     = "BT       ";
CODEBASESTR pBTC[]    = "BTC      ";
CODEBASESTR pBTR[]    = "BTR      ";
CODEBASESTR pBTS[]    = "BTS      ";
CODEBASESTR pCALLN[]  = "CALL NEAR";
CODEBASESTR pCALLF[]  = "CALL FAR ";
CODEBASESTR pCBW[]    = "CBW      ";
CODEBASESTR pCWDE[]   = "CWDE     ";
CODEBASESTR pCLC[]    = "CLC      ";
CODEBASESTR pCLD[]    = "CLD      ";
CODEBASESTR pCLI[]    = "CLI      ";
CODEBASESTR pCLTS[]   = "CLTS     ";
CODEBASESTR pCMC[]    = "CMC      ";
CODEBASESTR pCMP[]    = "CMP      ";
CODEBASESTR pCMPS[]   = "CMPS     ";
CODEBASESTR pCWD[]    = "CWD      ";
CODEBASESTR pCDQ[]    = "CDQ      ";
CODEBASESTR pDAA[]    = "DAA      ";
CODEBASESTR pDAS[]    = "DAS      ";
CODEBASESTR pDEC[]    = "DEC      ";
CODEBASESTR pDIV[]    = "DIV      ";
CODEBASESTR pENTER[]  = "ENTER    ";
CODEBASESTR pHLT[]    = "HLT      ";
CODEBASESTR pIDIV[]   = "IDIV     ";
CODEBASESTR pIMUL[]   = "IMUL     ";
CODEBASESTR pIN[]     = "IN       ";
CODEBASESTR pINC[]    = "INC      ";
CODEBASESTR pINS[]    = "INS      ";
CODEBASESTR pINT3[]   = "INT 3    ";
CODEBASESTR pINTO[]   = "INTO     ";
CODEBASESTR pINT[]    = "INT      ";
CODEBASESTR pIRET[]   = "IRET     ";
CODEBASESTR pJO[]     = "JO       ";
CODEBASESTR pJNO[]    = "JNO      ";
CODEBASESTR pJB[]     = "JB       ";
CODEBASESTR pJAE[]    = "JAE      ";
CODEBASESTR pJZ[]     = "JZ       ";
CODEBASESTR pJNZ[]    = "JNZ      ";
CODEBASESTR pJBE[]    = "JBE      ";
CODEBASESTR pJA[]     = "JA       ";
CODEBASESTR pJS[]     = "JS       ";
CODEBASESTR pJNS[]    = "JNS      ";
CODEBASESTR pJP[]     = "JP       ";
CODEBASESTR pJNP[]    = "JNP      ";
CODEBASESTR pJL[]     = "JL       ";
CODEBASESTR pJGE[]    = "JGE      ";
CODEBASESTR pJLE[]    = "JLE      ";
CODEBASESTR pJG[]     = "JG       ";
CODEBASESTR pJCXZ[]   = "JCXZ     ";
CODEBASESTR pJECXZ[]  = "JECXZ    ";
CODEBASESTR pJMPS[]   = "JMP SHORT";
CODEBASESTR pJMPN[]   = "JMP NEAR ";
CODEBASESTR pJMPF[]   = "JMP FAR  ";
CODEBASESTR pLAHF[]   = "LAHF     ";
CODEBASESTR pLAR[]    = "LAR      ";
CODEBASESTR pLEA[]    = "LEA      ";
CODEBASESTR pLEAVE[]  = "LEAVE    ";
CODEBASESTR pLGDT[]   = "LGDT     ";
CODEBASESTR pLIDT[]   = "LIDT     ";
CODEBASESTR pLDS[]    = "LDS      ";
CODEBASESTR pLES[]    = "LES      ";
CODEBASESTR pLFS[]    = "LFS      ";
CODEBASESTR pLGS[]    = "LGS      ";
CODEBASESTR pLSS[]    = "LSS      ";
CODEBASESTR pLLDT[]   = "LLDT     ";
CODEBASESTR pLMSW[]   = "LMSW     ";
CODEBASESTR pLODS[]   = "LODS     ";
CODEBASESTR pLOOP[]   = "LOOP     ";
CODEBASESTR pLOOPZ[]  = "LOOPZ    ";
CODEBASESTR pLOOPNZ[] = "LOOPNZ   ";
CODEBASESTR pLSL[]    = "LSL      ";
CODEBASESTR pLTR[]    = "LTR      ";
CODEBASESTR pMOV[]    = "MOV      ";




CODEBASESTR pMOVS[]   = "MOVS     ";
CODEBASESTR pMOVSX[]  = "MOVSX    ";
CODEBASESTR pMOVZX[]  = "MOVZX    ";
CODEBASESTR pMUL[]    = "MUL      ";
CODEBASESTR pNEG[]    = "NEG      ";
CODEBASESTR pNOP[]    = "NOP      ";
CODEBASESTR pNOT[]    = "NOT      ";
CODEBASESTR pOR[]     = "OR       ";
CODEBASESTR pOUT[]    = "OUT      ";
CODEBASESTR pOUTS[]   = "OUTS     ";
CODEBASESTR pPOP[]    = "POP      ";
CODEBASESTR pPOPA[]   = "POPA     ";
CODEBASESTR pPOPAD[]  = "POPAD    ";
CODEBASESTR pPOPF[]   = "POPF     ";
CODEBASESTR pPOPFD[]  = "POPFD    ";
CODEBASESTR pPUSH[]   = "PUSH     ";
CODEBASESTR pPUSHA[]  = "PUSHA    ";
CODEBASESTR pPUSHAD[] = "PUSHAD   ";
CODEBASESTR pPUSHF[]  = "PUSHF    ";
CODEBASESTR pPUSHFD[] = "PUSHFD   ";
CODEBASESTR pRCL[]    = "RCL      ";
CODEBASESTR pRCR[]    = "RCR      ";
CODEBASESTR pRETN[]   = "RETN     ";
CODEBASESTR pRETF[]   = "RETF     ";
CODEBASESTR pROL[]    = "ROL      ";
CODEBASESTR pROR[]    = "ROR      ";
CODEBASESTR pSAHF[]   = "SAHF     ";
CODEBASESTR pSAL[]    = "SAL      ";
CODEBASESTR pSAR[]    = "SAR      ";
CODEBASESTR pSBB[]    = "SBB      ";
CODEBASESTR pSCAS[]   = "SCAS     ";
CODEBASESTR pSETO[]   = "SETO     ";
CODEBASESTR pSETNO[]  = "SETNO    ";
CODEBASESTR pSETB[]   = "SETB     ";
CODEBASESTR pSETAE[]  = "SETAE    ";
CODEBASESTR pSETZ[]   = "SETZ     ";
CODEBASESTR pSETNZ[]  = "SETNZ    ";
CODEBASESTR pSETBE[]  = "SETBE    ";
CODEBASESTR pSETA[]   = "SETA     ";
CODEBASESTR pSETS[]   = "SETS     ";
CODEBASESTR pSETNS[]  = "SETNS    ";
CODEBASESTR pSETP[]   = "SETP     ";
CODEBASESTR pSETNP[]  = "SETNP    ";
CODEBASESTR pSETL[]   = "SETL     ";
CODEBASESTR pSETGE[]  = "SETGE    ";
CODEBASESTR pSETLE[]  = "SETLE    ";
CODEBASESTR pSETG[]   = "SETG     ";
CODEBASESTR pSGDT[]   = "SGDT     ";
CODEBASESTR pSHL[]    = "SHL      ";
CODEBASESTR pSHR[]    = "SHR      ";
CODEBASESTR pSHLD[]   = "SHLD     ";
CODEBASESTR pSHRD[]   = "SHRD     ";
CODEBASESTR pSIDT[]   = "SIDT     ";
CODEBASESTR pSLDT[]   = "SLDT     ";
CODEBASESTR pSMSW[]   = "SMSW     ";
CODEBASESTR pSTC[]    = "STC      ";
CODEBASESTR pSTD[]    = "STD      ";
CODEBASESTR pSTI[]    = "STI      ";
CODEBASESTR pSTOS[]   = "STOS     ";
CODEBASESTR pSTR[]    = "STR      ";
CODEBASESTR pSUB[]    = "SUB      ";
CODEBASESTR pTEST[]   = "TEST     ";
CODEBASESTR pVERR[]   = "VERR     ";
CODEBASESTR pVERW[]   = "VERW     ";
CODEBASESTR pWAIT[]   = "WAIT     ";
CODEBASESTR pXCHG[]   = "XCHG     ";
CODEBASESTR pXLATB[]  = "XLATB    ";
CODEBASESTR pXOR[]    = "XOR      ";


CODEBASESTR pILLEGAL[]= "*********";


// PREFIXES:  ADDRESS SIZE = 01100111     DATA SIZE = 01100110

CODEBASESTR pSEGCS[]  = "CS:";       // 00101110
CODEBASESTR pSEGDS[]  = "DS:";       // 00111110
CODEBASESTR pSEGES[]  = "ES:";       // 00100110
CODEBASESTR pSEGFS[]  = "FS:";       // 01100100
CODEBASESTR pSEGGS[]  = "GS:";       // 01100101
CODEBASESTR pSEGSS[]  = "SS:";       // 00110110
CODEBASESTR pNOSEG[]  = "   ";

CODEBASESTR pREP[]    = "REP   ";    // 11110011  [movs,lods,outs,stos,ins]
CODEBASESTR pREPZ[]   = "REPZ  ";    // 11110011  [cmps, scas]
CODEBASESTR pREPNZ[]  = "REPNZ ";    // 11110010  [cmps, scas]
CODEBASESTR pNOREP[]  = "      ";

CODEBASESTR pLOCK[]   = "LOCK  ";    // 11110000
CODEBASESTR pNOLOCK[] = "      ";


CODEBASESTR pAL[]     = "AL ";
CODEBASESTR pAH[]     = "AH ";
CODEBASESTR pAX[]     = "AX ";
CODEBASESTR pEAX[]    = "EAX";
CODEBASESTR pBL[]     = "BL ";
CODEBASESTR pBH[]     = "BH ";
CODEBASESTR pBX[]     = "BX ";
CODEBASESTR pEBX[]    = "EBX";
CODEBASESTR pCL[]     = "CL ";
CODEBASESTR pCH[]     = "CH ";
CODEBASESTR pCX[]     = "CX ";
CODEBASESTR pECX[]    = "ECX";
CODEBASESTR pDL[]     = "DL ";
CODEBASESTR pDH[]     = "DH ";
CODEBASESTR pDX[]     = "DX ";
CODEBASESTR pEDX[]    = "EDX";
CODEBASESTR pSI[]     = "SI ";
CODEBASESTR pESI[]    = "ESI";
CODEBASESTR pDI[]     = "DI ";
CODEBASESTR pEDI[]    = "EDI";
CODEBASESTR pBP[]     = "BP ";
CODEBASESTR pEBP[]    = "EBP";
CODEBASESTR pSP[]     = "SP ";
CODEBASESTR pESP[]    = "ESP";


CODEBASESTR pCS[]     = "CS ";
CODEBASESTR pDS[]     = "DS ";
CODEBASESTR pES[]     = "ES ";
CODEBASESTR pFS[]     = "FS ";
CODEBASESTR pGS[]     = "GS ";
CODEBASESTR pSS[]     = "SS ";


CODEBASESTR pCR0[]    = "CR0";
CODEBASESTR pCR2[]    = "CR2";
CODEBASESTR pCR3[]    = "CR3";

CODEBASESTR pDR0[]    = "DR0";
CODEBASESTR pDR1[]    = "DR1";
CODEBASESTR pDR2[]    = "DR2";
CODEBASESTR pDR3[]    = "DR3";
CODEBASESTR pDR6[]    = "DR6";
CODEBASESTR pDR7[]    = "DR7";

CODEBASESTR pTR4[]    = "TR4";
CODEBASESTR pTR5[]    = "TR5";
CODEBASESTR pTR6[]    = "TR6";
CODEBASESTR pTR7[]    = "TR7";

CODEBASESTR pERR[]    = "ERR";

CODEBASESTR * const pBYTEREG[]  = {pAL,pCL,pDL,pBL,pAH,pCH,pDH,pBH};
CODEBASESTR * const pWORDREG[]  = {pAX,pCX,pDX,pBX,pSP,pBP,pSI,pDI};
CODEBASESTR * const pDWORDREG[] = {pEAX,pECX,pEDX,pEBX,pESP,pEBP,pESI,pEDI};
CODEBASESTR * const pSEGREG[]   = {pES,pCS,pSS,pDS,pFS,pGS,pERR,pERR};

CODEBASESTR pRM000[]  = "[BX+SI]";
CODEBASESTR pRM001[]  = "[BX+DI]";
CODEBASESTR pRM010[]  = "[BP+SI]";
CODEBASESTR pRM011[]  = "[BP+DI]";
CODEBASESTR pRM100[]  = "[SI]";
CODEBASESTR pRM101[]  = "[DI]";
CODEBASESTR pRM110[]  = "[BP]";
CODEBASESTR pRM111[]  = "[BX]";

CODEBASESTR * const pRM[]   = {pRM000,pRM001,pRM010,pRM011,
                               pRM100,pRM101,pRM110,pRM111};

// NOTE:  32-bit R/M values use special 'ss index base' encoding for
//        modes 0, 1, 2  (mode 3 is still the same as for 16-bit)




// The following function will disassemble a single op code

LPCSTR FAR PASCAL Disassemble(const char __huge *lpCode, LPSTR lpBuf,
                              BOOL bUse32, DWORD FAR *lpdwOff32)
{
DWORD dwOff32;
WORD wSeg, wLock, wRep, wAddr, wData, wType;  // prefix values
BOOL bEnhanced;
BYTE b, b2;
LPSTR lpInst;


   if(lpdwOff32)
   {
      dwOff32 = *lpdwOff32;
   }
   else
   {
      dwOff32 = (DWORD)OFFSETOF(lpCode);
   }

   wSeg = wLock = wRep = 0;
   wAddr = wData = bUse32?!0:0;   // assign default 'USE32/USE16' value

   lpInst = NULL;

   wType = 0;
   bEnhanced = 0;


   // first step:  read all of the 'prefix' bytes until I run out...

   while(1)
   {
      if(*lpCode == 0x67)       wAddr = !wAddr;
      else if(*lpCode == 0x66)  wData = !wData;
      else if(*lpCode == 0x2e)  wSeg = 1;
      else if(*lpCode == 0x3e)  wSeg = 2;
      else if(*lpCode == 0x26)  wSeg = 3;
      else if(*lpCode == 0x64)  wSeg = 4;
      else if(*lpCode == 0x65)  wSeg = 5;
      else if(*lpCode == 0x36)  wSeg = 6;
      else if(*lpCode == 0xf3)  wRep = 1;
      else if(*lpCode == 0xf2)  wRep = 2;
      else break;

      lpCode++;      // point to next item...
      dwOff32++;     // same here (as required)
   }

   // NEXT, see if this is an ENHANCED instruction...

   if(*lpCode == 0xf)
   {
      bEnhanced = TRUE;
      lpCode++;
      dwOff32++;     // same here (as required)

      // Is this a Type '2' instruction??

      b = *lpCode;

      if(b == 0x6)
      {
         lpInst = pCLTS;

         lpCode ++;
         dwOff32++;     // same here (as required)
         wType = 2;
      }

      // Is it a Type '103' instruction?  ('NEAR' CONDITIONAL BRANCH!)

#define ASSIGN_TYPE103(X) {lpInst = X; wType = 103; lpCode++; dwOff32++;}

      else if(b == 0x80)       ASSIGN_TYPE103(pJO);
      else if(b == 0x81)       ASSIGN_TYPE103(pJNO);
      else if(b == 0x82)       ASSIGN_TYPE103(pJB);
      else if(b == 0x83)       ASSIGN_TYPE103(pJAE);
      else if(b == 0x84)       ASSIGN_TYPE103(pJZ);
      else if(b == 0x85)       ASSIGN_TYPE103(pJNZ);
      else if(b == 0x86)       ASSIGN_TYPE103(pJBE);
      else if(b == 0x87)       ASSIGN_TYPE103(pJA);
      else if(b == 0x88)       ASSIGN_TYPE103(pJS);
      else if(b == 0x89)       ASSIGN_TYPE103(pJNS);
      else if(b == 0x8a)       ASSIGN_TYPE103(pJP);
      else if(b == 0x8b)       ASSIGN_TYPE103(pJNP);
      else if(b == 0x8c)       ASSIGN_TYPE103(pJL);
      else if(b == 0x8d)       ASSIGN_TYPE103(pJGE);
      else if(b == 0x8e)       ASSIGN_TYPE103(pJLE);
      else if(b == 0x8f)       ASSIGN_TYPE103(pJG);

#undef ASSIGN_TYPE103

      // Is it a Type '4' instruction?  (ENHANCED 'single arg' functions)

      else if((b & 0xc7) == 0x81)     // POP SREG
      {
         lpInst = pPOP;
         wType = 204;

        // Do not increment EIP!
      }
      else if((b & 0xc7) == 0x80)     // PUSH SREG
      {
         lpInst = pPOP;
         wType = 204;

        // Do not increment EIP!
      }
      else if(b == 0)          // PMODE instructions
      {
         lpCode++;
         dwOff32++;

         b2 = (*lpCode) & 0x38;  // get ONLY the 'REG' bits of MOD:REG:R/M

         if(b2 == 0)
         {
            lpInst = pSLDT;
            wMode = 4;
         }
         else if(b2 == 1)
         {
            lpInst = pSTR;
            wMode = 4;
         }
         else if(b2 == 2)
         {
            lpInst = pLLDT;
            wMode = 4;
         }
         else if(b2 == 3)
         {
            lpInst = pLTR;
            wMode = 4;
         }
         else if(b2 == 4)
         {
            lpInst = pVERR;
            wMode = 4;
         }
         else if(b2 == 5)
         {
            lpInst = pVERW;
            wMode = 4;
         }
      }
      else if(b == 1)          // PMODE instructions
      {
         lpCode++;
         dwOff32++;

         b2 = (*lpCode) & 0x38;  // get ONLY the 'REG' bits of MOD:REG:R/M

         if(b2 == 0)
         {
            lpInst = pSGDT;
            wMode = 94;
         }
         else if(b2 == 1)
         {
            lpInst = pSIDT;
            wMode = 94;
         }
         else if(b2 == 2)
         {
            lpInst = pLGDT;
            wMode = 94;
         }
         else if(b2 == 3)
         {
            lpInst = pLIDT;
            wMode = 94;
         }
         else if(b2 == 4)
         {
            lpInst = pSMSW;
            wMode = 4;
         }
         else if(b2 == 6)
         {
            lpInst = pLMSW;
            wMode = 4;
         }
      }
      else if((b & 0xf0) == 0x90)  // 'SETxx' instructions
      {
         lpCode++;
         dwOff32++;

         b2 = (*lpCode) & 0x38;  // get ONLY the 'REG' bits of MOD:REG:R/M

         if(b2 == 0)             // this is required for 'SETxx'
         {
            wType = 14;          // these are considered 'BYTE' - all of them!

            if(b == 0x90)        lpInst = pSETO;
            else if(b == 0x91)   lpInst = pSETNO;
            else if(b == 0x92)   lpInst = pSETB
            else if(b == 0x93)   lpInst = pSETAE
            else if(b == 0x94)   lpInst = pSETZ
            else if(b == 0x95)   lpInst = pSETNZ
            else if(b == 0x96)   lpInst = pSETBE
            else if(b == 0x97)   lpInst = pSETA
            else if(b == 0x98)   lpInst = pSETS
            else if(b == 0x99)   lpInst = pSETNS
            else if(b == 0x9a)   lpInst = pSETP
            else if(b == 0x9b)   lpInst = pSETNP
            else if(b == 0x9c)   lpInst = pSETL
            else if(b == 0x9d)   lpInst = pSETGE
            else if(b == 0x9e)   lpInst = pSETLE
            else if(b == 0x9f)   lpInst = pSETG
         }
      }

      // Is this a TYPE '5' instruction??

      else if(b == 0xbc)
      {
         lpInst = pBSF;

         lpCode ++;
         dwOff32++;
         wType = 25;    // essentially, 'REG' is the 'DESTINATION' as far
                        // as coding is concerned...
      }
      else if(b == 0xbd)
      {
         lpInst = pBSR;

         lpCode ++;
         dwOff32++;
         wType = 25;    // essentially, 'REG' is the 'DESTINATION' as far
                        // as coding is concerned...
      }
      else if(b == 0xba)  // 'BT', 'BTC', 'BTR', or 'BTS' {IMM8}
      {
         lpCode++;        // pre-increment only - don't POST increment!
         dwOff32++;

         b2 = (*lpCode) & 0x38;  // get ONLY the 'REG' bits of MOD:REG:R/M

         if(b2 == 0x20)             // this is required for 'SETxx'
         {
            lpInst = pBT;
            wType = 115;
         }
         else if(b2 == 0x38)
         {
            lpInst = pBTC;
            wType = 115;
         }
         else if(b2 == 0x30)
         {
            lpInst = pBTR;
            wType = 115;
         }
         else if(b2 == 0x28)
         {
            lpInst = pBTS;
            wType = 115;
         }
      }
      else if(b == 0xa3)
      {
         lpInst = pBT;

         lpCode ++;
         dwOff32++;
         wType = 25;    // essentially, 'REG' is the 'DESTINATION' as far
                        // as coding is concerned...
      }
      else if(b == 0xbb)
      {
         lpInst = pBTC;

         lpCode ++;
         dwOff32++;
         wType = 25;    // essentially, 'REG' is the 'DESTINATION' as far
                        // as coding is concerned...
      }
      else if(b == 0xb3)
      {
         lpInst = pBTR;

         lpCode ++;
         dwOff32++;
         wType = 25;    // essentially, 'REG' is the 'DESTINATION' as far
                        // as coding is concerned...
      }
      else if(b == 0xab)
      {
         lpInst = pBTS;

         lpCode ++;
         dwOff32++;
         wType = 25;    // essentially, 'REG' is the 'DESTINATION' as far
                        // as coding is concerned...
      }
      else if(b == 0x2)
      {
         lpInst = pLAR;

         lpCode ++;
         dwOff32++;
         wType = 25;
      }
      else if(b == 0x3)
      {
         lpInst = pLSL;

         lpCode ++;
         dwOff32++;
         wType = 25;
      }
      else if(b == 0xb8)
      {
         lpInst = pLFS;

         lpCode ++;
         dwOff32++;
         wType = 25;
      }
      else if(b == 0xb9)
      {
         lpInst = pLGS;

         lpCode ++;
         dwOff32++;
         wType = 25;
      }
      else if(b == 0xb2)
      {
         lpInst = pLSS;

         lpCode ++;
         dwOff32++;
         wType = 25;
      }
      else if((b & 0xfd) == 0x20)  // mov cr0/2/3, xxx or mov xxx,cr0/2/3
      {
         lpInst = pMOV;

         lpCode ++;
         dwOff32++;

         if(b & 2)    wType = 305;   // 'RRR' is SOURCE  (i.e. backwards)
         else         wType = 325;   // 'RRR' is DEST
      }
      else if((b & 0xfd) == 0x21)  // mov dr#, xxx or mov xxx,dr#
      {
         lpInst = pMOV;

         lpCode ++;
         dwOff32++;

         if(b & 2)    wType = 405;   // 'RRR' is SOURCE  (i.e. backwards)
         else         wType = 425;   // 'RRR' is DEST
      }
      else if((b & 0xfd) == 0x24)  // mov tr#, xxx or mov xxx,tr#
      {
         lpInst = pMOV;

         lpCode ++;
         dwOff32++;

         if(b & 2)    wType = 505;   // 'RRR' is SOURCE  (i.e. backwards)
         else         wType = 525;   // 'RRR' is DEST
      }
      else if(b == 0xaf)
      {
         lpInst = pIMUL;

         lpCode ++;
         dwOff32++;
         wType = 25;    // 'REG' is always the 'DESTINATION'
      }
      else if((b & 0xfe)==0xf6)
      {
         lpInst = pIMUL;

         lpCode ++;
         dwOff32++;

         if((*lpCode & 0x38)==0x28) // always must be this
         {
            if(b & 1)  wType = 605;   // WORD
            else       wType = 615;   // BYTE
         }
      }
      else if((b & 0xfd)==0x69)     // '3' arg version of IMUL!!
      {
         lpInst = pIMUL;

         lpCode ++;
         dwOff32++;

         if(b & 2)      // sign extend - 8 bit immediate
         {
            wType = 715;
         }
         else
         {
            wType = 705;
         }
      }

      // Is this a TYPE '6' instruction??

      else if((b & 0xfe)==0xbe)
      {
         lpInst = pMOVSX;

         lpCode ++;
         dwOff32++;

         if(b & 1)   wType = 26;   // WORD ('REG' is always the DESTINATION!)
         else        wType = 36;   // BYTE
      }
      else if((b & 0xfe)==0xb6)
      {
         lpInst = pMOVZX;

         lpCode ++;
         dwOff32++;

         if(b & 1)   wType = 26;   // WORD ('REG' is always the DESTINATION!)
         else        wType = 36;   // BYTE
      }

      // Is this a TYPE '8' instruction?? (no type 7 or 9)

      else if(b == 0xa4)       // SHLD {IMM8}
      {
         lpInst = pSHLD;

         lpCode ++;
         dwOff32++;
         wType = 308;          // note that 'REG' is always the SOURCE!
      }
      else if(b == 0xa5)       // SHLD {CL}
      {
         lpInst = pSHLD;

         lpCode ++;
         dwOff32++;
         wType = 408;          // note that 'REG' is always the SOURCE!
      }
      else if(b == 0xaa)       // SHRD {IMM8}
      {
         lpInst = pSHRD;

         lpCode ++;
         dwOff32++;
         wType = 308;          // note that 'REG' is always the SOURCE!
      }
      else if(b == 0xab)       // SHRD {CL}
      {
         lpInst = pSHRD;

         lpCode ++;
         dwOff32++;
         wType = 408;          // note that 'REG' is always the SOURCE!
      }
   }
   else
   {
      bEnhanced = FALSE;

      // Is this a Type '1' instruction??

      b = *lpCode;

#define ASSIGN_TYPE1(X) {lpInst = X; wType = 1; lpCode++; dwOff32++;}

      if(b==0x37)                ASSIGN_TYPE1(pAAA);
      else if(b==0x3f)           ASSIGN_TYPE1(pAAS);
      else if(b==0x98 && !wData) ASSIGN_TYPE1(pCBW);
      else if(b==0x98 && wData)  ASSIGN_TYPE1(pCWDE);
      else if(b==0xf8)           ASSIGN_TYPE1(pCLC);
      else if(b==0xfc)           ASSIGN_TYPE1(pCLD);
      else if(b==0xfa)           ASSIGN_TYPE1(pCLI);
      else if(b==0xf9)           ASSIGN_TYPE1(pCMC);
      else if(b==0x99 && !wData) ASSIGN_TYPE1(pCWD);
      else if(b==0x99 && wData)  ASSIGN_TYPE1(pCDQ);
      else if(b==0x27)           ASSIGN_TYPE1(pDAA);
      else if(b==0x2f)           ASSIGN_TYPE1(pDAS);
      else if(b==0xc8)           ASSIGN_TYPE1(pENTER);
      else if(b==0xf4)           ASSIGN_TYPE1(pHLT);
      else if(b==0xcc)           ASSIGN_TYPE1(pINT3);
      else if(b==0xce)           ASSIGN_TYPE1(pINTO);
      else if(b==0xcf)           ASSIGN_TYPE1(pIRET);
      else if(b==0x9f)           ASSIGN_TYPE1(pLAHF);
      else if(b==0xc9)           ASSIGN_TYPE1(pLEAVE);
      else if(b==0x90)           ASSIGN_TYPE1(pNOP);
      else if(b==0x9e)           ASSIGN_TYPE1(pSAHF);
      else if(b==0xf9)           ASSIGN_TYPE1(pSTC);
      else if(b==0xfd)           ASSIGN_TYPE1(pSTD);
      else if(b==0xfb)           ASSIGN_TYPE1(pSTI);
      else if(b==0x9b)           ASSIGN_TYPE1(pWAIT);
      else if(b==0xd7)
      {
         lpInst = pXLATB;

         wType = 101;          // this indicates 'special treatment'
         lpCode++;
         dwOff32++;
      }
      else if(b==0x61 && !wData) ASSIGN_TYPE1(pPOPA);
      else if(b==0x61 && wData)  ASSIGN_TYPE1(pPOPAD);
      else if(b==0x9d && !wData) ASSIGN_TYPE1(pPOPF);
      else if(b==0x9d && wData)  ASSIGN_TYPE1(pPOPFD);
      else if(b==0x60 && !wData) ASSIGN_TYPE1(pPUSHA);
      else if(b==0x60 && wData)  ASSIGN_TYPE1(pPUSHAD);
      else if(b==0x9c && !wData) ASSIGN_TYPE1(pPUSHF);
      else if(b==0x9c && wData)  ASSIGN_TYPE1(pPUSHFD);

#undef ASSIGN_TYPE1

      // Is this a Type '2' instruction??

      else if(b==0xd3 && lpCode[1]==0xa)
      {
         lpInst = pAAD;

         lpCode += 2;
         dwOff32 += 2;
         wType = 2;
      }
      else if(b==0xd2 && lpCode[1]==0xa)
      {
         lpInst = pAAM;

         lpCode += 2;
         dwOff32 += 2;
         wType = 2;
      }

     // Is this a Type '3' instruction (SHORT conditional branch/loop)

#define ASSIGN_TYPE3(X) {lpInst = X; wType = 3; lpCode++; dwOff32++;}

      else if(b==0x70)           ASSIGN_TYPE3(pJO)
      else if(b==0x71)           ASSIGN_TYPE3(pJNO)
      else if(b==0x72)           ASSIGN_TYPE3(pJB)
      else if(b==0x73)           ASSIGN_TYPE3(pJAE)
      else if(b==0x74)           ASSIGN_TYPE3(pJZ)
      else if(b==0x75)           ASSIGN_TYPE3(pJNZ)
      else if(b==0x76)           ASSIGN_TYPE3(pJBE)
      else if(b==0x77)           ASSIGN_TYPE3(pJA)
      else if(b==0x78)           ASSIGN_TYPE3(pJS)
      else if(b==0x79)           ASSIGN_TYPE3(pJNS)
      else if(b==0x7a)           ASSIGN_TYPE3(pJP)
      else if(b==0x7b)           ASSIGN_TYPE3(pJNP)
      else if(b==0x7c)           ASSIGN_TYPE3(pJL)
      else if(b==0x7d)           ASSIGN_TYPE3(pJGE)
      else if(b==0x7e)           ASSIGN_TYPE3(pJLE)
      else if(b==0x7f)           ASSIGN_TYPE3(pJG)
      else if(b==0xe3 && !wData) ASSIGN_TYPE3(pJCXZ)
      else if(b==0xe3 && wData)  ASSIGN_TYPE3(pJECXZ)
      else if(b==0xe2)           ASSIGN_TYPE3(pLOOP)
      else if(b==0xe1)           ASSIGN_TYPE3(pLOOPZ)
      else if(b==0xe0)           ASSIGN_TYPE3(pLOOPNZ)

#undef ASSIGN_TYPE3

     // Is this a Type '4' instruction (single argument)

      else if(b==0xfe)
      {
         lpCode++;
         dwOff32++;

         b2 = (*lpCode) & 0x38;  // get ONLY the 'REG' bits of MOD:REG:R/M

         if(b2 == 0x08)          // DEC
         {
            lpInst = pDEC;
            wMode = 14;
         }
         else if(b2 == 0)        // INC
         {
            lpInst = pINC;
            wMode = 14;
         }
      }
      else if(b==0xff)
      {
         // IMPORTANT!  In this case, other instructions may be present
         // which are NOT 'type 4's so I must account for them as well...

         lpCode++;
         dwOff32++;

         b2 = (*lpCode) & 0x38;  // get ONLY the 'REG' bits of MOD:REG:R/M

         // MOD:XXX:R/M version of 'TYPE 9' instructions, as well as
         // the Type '4' instructions using the '0ffH' prefix.

         if(b2==0x10)      // NEAR CALL []
         {
            lpInst = pCALLN;
            wMode = 209;
         }
         else if(b2==0x18) // FAR CALL  []
         {
            lpInst = pCALLF;
            wMode = 249;
         }
         else if(b2==0x20) // NEAR JUMP []
         {
            lpInst = pJMPN;
            wMode = 209;
         }
         else if(b2==0x28) // FAR JUMP  []
         {
            lpInst = pJMPF;
            wMode = 249;
         }

         // The remainders are TYPE '4's!

         else if(b2 == 0x08)     // DEC
         {
            lpInst = pDEC;
            wMode = 4;
         }
         else if(b2 == 0)        // INC
         {
            lpInst = pINC;
            wMode = 4;
         }
      }
      else if((b & 0xf8) == 0x48)  // DEC
      {
         lpInst = pDEC;
         wMode = 104;

         // NOTE:  Do not increment EIP pointer!
      }
      else if((b & 0xf8) == 0x40)  // INC
      {
         lpInst = pDEC;
         wMode = 104;

         // NOTE:  Do not increment EIP pointer!
      }
      else if((b & 0xf8) == 0x58)  // POP REG
      {
         lpInst = pPOP;
         wMode = 104;

         // NOTE:  Do not increment EIP pointer!
      }
      else if((b & 0xf8) == 0x50)  // PUSH REG
      {
         lpInst = pPUSH;
         wMode = 104;

         // NOTE:  Do not increment EIP pointer!
      }
      else if((b & c7)==0x7)       // POP SREG
      {
         lpInst = pPOP;
         wMode = 204;

         // NOTE:  Do not increment EIP pointer!
      }
      else if((b & c7)==0x6)       // PUSH SREG
      {
         lpInst = pPUSH;
         wMode = 204;

         // NOTE:  Do not increment EIP pointer!
      }
      else if(b == 0x68)           // PUSH {IMM16}
      {
         lpInst = pPUSH;
         wMode = 304;

         lpCode++;
         dwOff32++;
      }
      else if(b == 0x6a)           // PUSH {IMM8} (sign extend)
      {
         lpInst = pPUSH;
         wMode = 314;

         lpCode++;
         dwOff32++;
      }
      else if(b == 0xf6)
      {
         lpCode++;
         dwOff32++;

         b2 = (*lpCode) & 0x38;  // get ONLY the 'REG' bits of MOD:REG:R/M

         if(b2 == 0x10)  // NOT
         {
            lpInst = pNOT;
            wMode = 14;
         }
         else if(b2 == 0x18)
         {
            lpInst = pNEG;
            wMode = 14;
         }
         else if(b2 == 0x20)
         {
            lpInst = pMUL;
            wMode = 14;
         }
         else if(b2 == 0x28)
         {
            lpInst = pIMUL;
            wMode = 615;        // note that this is a TYPE 5!
         }
         else if(b2 == 0x30)
         {
            lpInst = pDIV;
            wMode = 14;
         }
         else if(b2 == 0x38)
         {
            lpInst = pIDIV;
            wMode = 14;
         }
      }
      else if(b == 0xf7)
      {
         lpCode++;
         dwOff32++;

         b2 = (*lpCode) & 0x38;  // get ONLY the 'REG' bits of MOD:REG:R/M

         if(b2 == 0x10)  // NOT
         {
            lpInst = pNOT;
            wMode = 4;
         }
         else if(b2 == 0x18)
         {
            lpInst = pNEG;
            wMode = 4;
         }
         else if(b2 == 0x20)
         {
            lpInst = pMUL;
            wMode = 4;
         }
         else if(b2 == 0x28)
         {
            lpInst = pIMUL;
            wMode = 605;        // note that this is a TYPE 5!
         }
         else if(b2 == 0x30)
         {
            lpInst = pDIV;
            wMode = 4;
         }
         else if(b2 == 0x38)
         {
            lpInst = pIDIV;
            wMode = 4;
         }
      }

      // Is this a TYPE '5' instruction??

      else if(b == 0x63)
      {
         lpInst = pARPL;

         lpCode ++;
         dwOff32++;
         wType = 5;          // note that 'REG' is always the SOURCE!
      }
      else if(b == 0x62)
      {
         lpInst = pBOUND;

         lpCode ++;
         dwOff32++;
         wType = 25;         // note that 'REG' is always the DESTINATION!
      }
      else if(b == 0x8b)
      {
         lpInst = pLEA;

         lpCode ++;
         dwOff32++;
         wType = 25;         // note that 'REG' is always the DESTINATION!
      }
      else if(b == 0xc5)
      {
         lpInst = pLDS;

         lpCode ++;
         dwOff32++;
         wType = 25;         // note that 'REG' is always the DESTINATION!
      }
      else if(b == 0xc4)
      {
         lpInst = pLES;

         lpCode ++;
         dwOff32++;
         wType = 25;         // note that 'REG' is always the DESTINATION!
      }
      else if((b & 0xfd) == 0x8c)
      {
         lpInst = pLES;

         lpCode ++;
         dwOff32++;
         if(b & 2)  wType = 225;         // 'SREG' is destination
         else       wType = 205;         // 'SREG' is source
      }

      // Is this a TYPE '6' instruction??

      if((b & 0xfd)==0x81)         // type 6 MOD:XXX:R/M {IMM[8]}
      {
         lpCode++;
         dwOff32++;

         b2 = (*lpCode) & 0x38;    // get ONLY the 'REG' bits of MOD:REG:R/M

         if(b2 == 0)
         {
            lpInst = pADD;
            wType = (b & 2)?416:406;
         }
         else if(b2 == 1)
         {
            lpInst = pOR;
            wType = (b & 2)?416:406;
         }
         else if(b2 == 2)
         {
            lpInst = pADC;
            wType = (b & 2)?416:406;
         }
         else if(b2 == 3)
         {
            lpInst = pSBB;
            wType = (b & 2)?416:406;
         }
         else if(b2 == 4)
         {
            lpInst = pAND;
            wType = (b & 2)?416:406;
         }
         else if(b2 == 5)
         {
            lpInst = pSUB;
            wType = (b & 2)?416:406;
         }
         else if(b2 == 6)
         {
            lpInst = pXOR;
            wType = (b & 2)?416:406;
         }
         else if(b2 == 7)
         {
            lpInst = pCMP;
            wType = (b & 2)?416:406;
         }
      }
      else if((b & 0xfe) == 0x14)
      {
         lpInst = pADC;

         lpCode ++;
         dwOff32++;
         wType = (b & 1)?506:516;  // WORD/BYTE {IMM} data
      }
      else if((b & 0xfe) == 0x04)
      {
         lpInst = pADD;

         lpCode ++;
         dwOff32++;
         wType = (b & 1)?506:516;  // WORD/BYTE {IMM} data
      }
      else if((b & 0xfe) == 0x22)
      {
         lpInst = pAND;

         lpCode ++;
         dwOff32++;
         wType = (b & 1)?506:516;  // WORD/BYTE {IMM} data
      }
      else if((b & 0xfe) == 0x3c)
      {
         lpInst = pCMP;

         lpCode ++;
         dwOff32++;
         wType = (b & 1)?506:516;  // WORD/BYTE {IMM} data
      }
      else if((b & 0xfe) == 0x0c)
      {
         lpInst = pOR;

         lpCode ++;
         dwOff32++;
         wType = (b & 1)?506:516;  // WORD/BYTE {IMM} data
      }
      else if((b & 0xfe) == 0x1c)
      {
         lpInst = pSBB;

         lpCode ++;
         dwOff32++;
         wType = (b & 1)?506:516;  // WORD/BYTE {IMM} data
      }
      else if((b & 0xfe) == 0x2c)
      {
         lpInst = pSUB;

         lpCode ++;
         dwOff32++;
         wType = (b & 1)?506:516;  // WORD/BYTE {IMM} data
      }
      else if((b & 0xfe) == 0xa8)
      {
         lpInst = pTEST;

         lpCode ++;
         dwOff32++;
         wType = (b & 1)?506:516;  // WORD/BYTE {IMM} data
      }
      else if((b & 0xfe) == 0x34)
      {
         lpInst = pXOR;

         lpCode ++;
         dwOff32++;
         wType = (b & 1)?506:516;  // WORD/BYTE {IMM} data
      }
      else if((b & 0xfc)==0x10)    // type 6 MOD:REG:R/M
      {
         lpInst = pADC;

         lpCode ++;
         dwOff32++;

         if(b & 2) wType = (b & 1)?26:36;  // WORD/BYTE {REG is DESTINATION}
         else      wType = (b & 1)?06:16;  // WORD/BYTE {REG is SOURCE}
      }
      else if((b & 0xfc)==0x00)    // type 6 MOD:REG:R/M
      {
         lpInst = pADD;

         lpCode ++;
         dwOff32++;

         if(b & 2) wType = (b & 1)?26:36;  // WORD/BYTE {REG is DESTINATION}
         else      wType = (b & 1)?06:16;  // WORD/BYTE {REG is SOURCE}
      }
      else if((b & 0xfc)==0x20)    // type 6 MOD:REG:R/M
      {
         lpInst = pAND;

         lpCode ++;
         dwOff32++;

         if(b & 2) wType = (b & 1)?26:36;  // WORD/BYTE {REG is DESTINATION}
         else      wType = (b & 1)?06:16;  // WORD/BYTE {REG is SOURCE}
      }
      else if((b & 0xfc)==0x38)    // type 6 MOD:REG:R/M
      {
         lpInst = pCMP;

         lpCode ++;
         dwOff32++;

         if(b & 2) wType = (b & 1)?26:36;  // WORD/BYTE {REG is DESTINATION}
         else      wType = (b & 1)?06:16;  // WORD/BYTE {REG is SOURCE}
      }
      else if((b & 0xfc)==0x88)    // type 6 MOD:REG:R/M
      {
         lpInst = pMOV;

         lpCode ++;
         dwOff32++;

         if(b & 2) wType = (b & 1)?26:36;  // WORD/BYTE {REG is DESTINATION}
         else      wType = (b & 1)?06:16;  // WORD/BYTE {REG is SOURCE}
      }
      else if((b & 0xfc)==0x08)    // type 6 MOD:REG:R/M
      {
         lpInst = pOR;

         lpCode ++;
         dwOff32++;

         if(b & 2) wType = (b & 1)?26:36;  // WORD/BYTE {REG is DESTINATION}
         else      wType = (b & 1)?06:16;  // WORD/BYTE {REG is SOURCE}
      }
      else if((b & 0xfc)==0x18)    // type 6 MOD:REG:R/M
      {
         lpInst = pSBB;

         lpCode ++;
         dwOff32++;

         if(b & 2) wType = (b & 1)?26:36;  // WORD/BYTE {REG is DESTINATION}
         else      wType = (b & 1)?06:16;  // WORD/BYTE {REG is SOURCE}
      }
      else if((b & 0xfc)==0x28)    // type 6 MOD:REG:R/M
      {
         lpInst = pSUB;

         lpCode ++;
         dwOff32++;

         if(b & 2) wType = (b & 1)?26:36;  // WORD/BYTE {REG is DESTINATION}
         else      wType = (b & 1)?06:16;  // WORD/BYTE {REG is SOURCE}
      }
      else if((b & 0xfc)==0x30)    // type 6 MOD:REG:R/M
      {
         lpInst = pXOR;

         lpCode ++;
         dwOff32++;

         if(b & 2) wType = (b & 1)?26:36;  // WORD/BYTE {REG is DESTINATION}
         else      wType = (b & 1)?06:16;  // WORD/BYTE {REG is SOURCE}
      }
      else if((b & 0xfe)==0x84)    // type 6 MOD:REG:R/M (no direction bit!)
      {
         lpInst = pTEST;

         lpCode ++;
         dwOff32++;

         wType = (b & 1)?06:16;  // WORD/BYTE {REG is *ALWAYS* SOURCE}
      }
      else if((b & 0xfe)==0xf6)    // TEST MOD:000:R/M {IMM}
      {
         lpCode++;
         dwOff32++;

         b2 = (*lpCode) & 0x38;    // get ONLY the 'REG' bits of MOD:REG:R/M

         if(b2 == 0)
         {
            lpInst = pTEST;
            wType = (b & 2)?416:406;
         }
      }
      else if((b & 0xfe)==0xc6)    // MOV MOD:000:R/M {IMM}
      {
         lpCode++;
         dwOff32++;

         b2 = (*lpCode) & 0x38;    // get ONLY the 'REG' bits of MOD:REG:R/M

         if(b2 == 0)
         {
            lpInst = pMOV;
            wType = (b & 2)?416:406;
         }
      }
      else if((b & 0xf8)==0x90)      // XCHG xxxxx:REG  (one arg is AX/EAX)
      {
         lpInst = pXCHG;
         wType = 0x306;              // NOTE that 'lpCode' and 'dwOff32' are
                                     // NOT incremented here!
      }
      else if((b & 0xf0)==0xb0)      // MOV xxxxw:REG {IMM}
      {
         lpInst = pMOV;
         wType = (b & 0x80)?246:256; // NOTE that 'lpCode' and 'dwOff32' are
                                     // NOT incremented here!
      }
      else if((b & 0xfc)==0xa0)      // MOV AL/AX/EAX, {DISP}
      {                              // MOV {DISP}, AL/AX/EAX
         lpInst = pXOR;

         lpCode ++;
         dwOff32++;

         // NOTE:  The 'd' flag is REVERSED in its behavior for
         //        this instruction!  Maybe not to the CHIP designers,
         //        but semantically it's BACKWARDS!!

         if(b & 2) wType = (b & 1)?106:116;  // WORD/BYTE {REG is SOURCE}
         else      wType = (b & 1)?126:136;  // WORD/BYTE {REG is DESTINATION}
      }


      // Is this a TYPE '7' instruction??

      else if((b & 0xf6)==0xe4)   // IN al/ax/eax,{IMM8} or IN al/ax/eax,DX
      {
         lpInst = pIN;

         lpCode ++;
         dwOff32++;

         if(b & 0x80)
            wType = (b & 1)?207:217; // {DX} is port
         else
            wType = (b & 1)?107:117; // {IMM8} is port
      }
      else if((b & 0xf6)==0xe6)   // OUT {IMM8},al/ax/eax or OUT DX,al/ax/eax
      {
         lpInst = pIN;

         lpCode ++;
         dwOff32++;

         if(b & 0x80)
            wType = (b & 1)?207:217; // {DX} is port
         else
            wType = (b & 1)?107:117; // {IMM8} is port
      }
      else if((b & 0xfe)==0xa6)
      {
         lpInst = pCMPS;

         lpCode ++;
         dwOff32++;

         wType = (b & 1)?27:37;
      }
      else if((b & 0xfe)==0x6c)
      {
         lpInst = pINS;

         lpCode ++;
         dwOff32++;

         wType = (b & 1)?07:17;
      }
      else if((b & 0xfe)==0xac)
      {
         lpInst = pLODS;

         lpCode ++;
         dwOff32++;

         wType = (b & 1)?07:17;
      }
      else if((b & 0xfe)==0xa4)
      {
         lpInst = pMOVS;

         lpCode ++;
         dwOff32++;

         wType = (b & 1)?07:17;
      }
      else if((b & 0xfe)==0x6e)
      {
         lpInst = pOUTS;

         lpCode ++;
         dwOff32++;

         wType = (b & 1)?07:17;
      }
      else if((b & 0xfe)==0xae)
      {
         lpInst = pSCAS;

         lpCode ++;
         dwOff32++;

         wType = (b & 1)?27:37;
      }
      else if((b & 0xfe)==0xaa)
      {
         lpInst = pSTOS;

         lpCode ++;
         dwOff32++;

         wType = (b & 1)?07:17;
      }




      // Is this a TYPE '8' instruction??

      else if((b & 0xfe)==0xd0)    // TYPE 8 MOD:xxx:R/M {1} (implied arg)
      {
         lpCode++;
         dwOff32++;

         b2 = (*lpCode) & 0x38;    // get ONLY the 'REG' bits of MOD:REG:R/M

         if(b2 == 0)
         {
            lpInst = pROL;
            wType = (b & 1)?8:18;
         }
         else if(b2 == 1)
         {
            lpInst = pROR;
            wType = (b & 1)?8:18;
         }
         else if(b2 == 2)
         {
            lpInst = pRCL;
            wType = (b & 1)?8:18;
         }
         else if(b2 == 3)
         {
            lpInst = pRCR;
            wType = (b & 1)?8:18;
         }
         else if(b2 == 4)
         {
            lpInst = pSHL;
            wType = (b & 1)?8:18;
         }
         else if(b2 == 5)
         {
            lpInst = pSHR;
            wType = (b & 1)?8:18;
         }
         else if(b2 == 7)
         {
            lpInst = pSAR;
            wType = (b & 1)?8:18;
         }
      }
      else if((b & 0xfe)==0xd2)    // TYPE 8 MOD:xxx:R/M {CL} (implied arg)
      {
         lpCode++;
         dwOff32++;

         b2 = (*lpCode) & 0x38;    // get ONLY the 'REG' bits of MOD:REG:R/M

         if(b2 == 0)
         {
            lpInst = pROL;
            wType = (b & 1)?108:118;
         }
         else if(b2 == 1)
         {
            lpInst = pROR;
            wType = (b & 1)?108:118;
         }
         else if(b2 == 2)
         {
            lpInst = pRCL;
            wType = (b & 1)?108:118;
         }
         else if(b2 == 3)
         {
            lpInst = pRCR;
            wType = (b & 1)?108:118;
         }
         else if(b2 == 4)
         {
            lpInst = pSHL;
            wType = (b & 1)?108:118;
         }
         else if(b2 == 5)
         {
            lpInst = pSHR;
            wType = (b & 1)?108:118;
         }
         else if(b2 == 7)
         {
            lpInst = pSAR;
            wType = (b & 1)?108:118;
         }
      }
      else if((b & 0xfe)==0xc0)    // TYPE 8 MOD:xxx:R/M {IMM8}
      {
         lpCode++;
         dwOff32++;

         b2 = (*lpCode) & 0x38;    // get ONLY the 'REG' bits of MOD:REG:R/M

         if(b2 == 0)
         {
            lpInst = pROL;
            wType = (b & 1)?208:218;
         }
         else if(b2 == 1)
         {
            lpInst = pROR;
            wType = (b & 1)?208:218;
         }
         else if(b2 == 2)
         {
            lpInst = pRCL;
            wType = (b & 1)?208:218;
         }
         else if(b2 == 3)
         {
            lpInst = pRCR;
            wType = (b & 1)?208:218;
         }
         else if(b2 == 4)
         {
            lpInst = pSHL;
            wType = (b & 1)?208:218;
         }
         else if(b2 == 5)
         {
            lpInst = pSHR;
            wType = (b & 1)?208:218;
         }
         else if(b2 == 7)
         {
            lpInst = pSAR;
            wType = (b & 1)?208:218;
         }
      }


      // Is this a TYPE '9' instruction??

      else if(b == 0xc3)     // NEAR return
      {
         lpInst = pRETN;

         lpCode ++;
         dwOff32++;

         wType = 1;          // no argument - code it as a TYPE 1
      }
      else if(b == 0xc2)
      {
         lpInst = pRETN;

         lpCode ++;
         dwOff32++;

         wType = 9;          // 1 argument {IMM8}
      }
      else if(b == 0xcb)     // FAR return
      {
         lpInst = pRETF;

         lpCode ++;
         dwOff32++;

         wType = 1;          // no argument - code it as a TYPE 1
      }
      else if(b == 0xca)
      {
         lpInst = pRETF;

         lpCode ++;
         dwOff32++;

         wType = 9;          // 1 argument {IMM8}
      }
      else if(b == 0x62)     // INT {imm8}
      {
         lpInst = pINT;

         lpCode ++;
         dwOff32++;

         wType = 9;          // 1 argument {IMM8}
      }
      else if(b == 0xe8)     // CALL NEAR {DISP}
      {
         lpInst = pCALLN;

         lpCode ++;
         dwOff32++;

         wType = 109;
      }
      else if(b == 0x9a)     // CALL FAR {SEG:OFF}
      {
         lpInst = pCALLF;

         lpCode ++;
         dwOff32++;

         wType = 149;
      }
      else if(b == 0xeb)     // JMP SHORT {DISP8}
      {
         lpInst = pJMPS;

         lpCode ++;
         dwOff32++;

         wType = 119;
      }
      else if(b == 0xe9)     // JMP NEAR {DISP}
      {
         lpInst = pJMPN;

         lpCode ++;
         dwOff32++;

         wType = 109;
      }
      else if(b == 0xea)     // JMP FAR {SEG:OFF}
      {
         lpInst = pJMPF;

         lpCode ++;
         dwOff32++;

         wType = 149;
      }
   }

   if(wType == 0)        // this is an ILLEGAL instruction!
   {
      lpInst = "******";
   }


   // WHEW!!!  Now that I've done THAT part, the remaining section requires
   // that I properly decode the arguments.  Using the 'wType' value, I
   // can now generate the CORRECT arguments for the appropriate instructions






}



// TYPE '1' INSTRUCTIONS   (single byte, no arguments, single mode)
// TYPE '101' INSTRUCTIONS (single 'forced' argument)

// AAA       // 00110111
// AAS       // 00111111
// CBW       // 10011000  SAME CODE AS CWDE
// CWDE      // 10011000  SAME CODE AS CBW
// CLC       // 11111000
// CLD       // 11111100
// CLI       // 11111010
// CMC       // 11110101
// CWD       // 10011001  SAME CODE AS CDQ
// CDQ       // 10011001  SAME CODE AS CWD
// DAA       // 00100111
// DAS       // 00101111
// ENTER     // 11001000
// HLT       // 11110100
// INT3      // 11001100
// INTO      // 11001110
// IRET      // 11001111
// LAHF      // 10011111
// LEAVE     // 11001001
// NOP       // 10010000
// POPA      // 01100001 [SAME AS POPAD]
// POPAD     // 01100001 [SAME AS POPA]
// POPF      // 10011101 [SAME AS POPFD]
// POPFD     // 10011101 [SAME AS POPF]
// PUSHA     // 01100000 [SAME AS PUSHAD]
// PUSHAD    // 01100000 [SAME AS PUSHA]
// PUSHF     // 10011100 [SAME AS PUSHFD]
// PUSHFD    // 10011100 [SAME AS PUSHF]
// SAHF      // 10011110
// STC       // 11111001
// STD       // 11111101
// STI       // 11111011
// WAIT      // 10011011
// XLATB     // 11010111   use '[SEG:][BX]' or '[SEG:][EBX]' as argument





// TYPE '2' (multiple byte, no argument)

// AAD       // 11010101 00001010
// AAM       // 11010100 00001010
// CLTS      // 00001111 00000110





// TYPE '3'   CONDITIONAL BRANCH / LOOP (1 relative distance argument)
// TYPE '103' CONDITIONAL BRANCH (1 NEAR relative distance argument)

// JO        // 01110000 {DISP8} | 00001111 10000000 {DISP}
// JNO       // 01110001 {DISP8} | 00001111 10000001 {DISP}
// JB        // 01110010 {DISP8} | 00001111 10000010 {DISP}
// JAE       // 01110011 {DISP8} | 00001111 10000011 {DISP}
// JZ        // 01110100 {DISP8} | 00001111 10000100 {DISP}
// JNZ       // 01110101 {DISP8} | 00001111 10000101 {DISP}
// JBE       // 01110110 {DISP8} | 00001111 10000110 {DISP}
// JA        // 01110111 {DISP8} | 00001111 10000111 {DISP}
// JS        // 01111000 {DISP8} | 00001111 10001000 {DISP}
// JNS       // 01111001 {DISP8} | 00001111 10001001 {DISP}
// JP        // 01111010 {DISP8} | 00001111 10001010 {DISP}
// JNP       // 01111011 {DISP8} | 00001111 10001011 {DISP}
// JL        // 01111100 {DISP8} | 00001111 10001100 {DISP}
// JGE       // 01111101 {DISP8} | 00001111 10001101 {DISP}
// JLE       // 01111110 {DISP8} | 00001111 10001110 {DISP}
// JG        // 01111111 {DISP8} | 00001111 10001111 {DISP}
// JCXZ      // 11100011 {DISP8} [SAME AS JECXZ]
// JECXZ     // 11100011 {DISP8} [SAME AS JCXZ]
// LOOP      // 11100010 {DISP8}
// LOOPZ     // 11100001 {DISP8}
// LOOPNZ    // 11100000 {DISP8}




// TYPE '004' to '314' - single argument instructions (general)
//      4:  MOD:xxx:R/M (WORD)
//     14:  MOD:xxx:R/M (BYTE)
//     94:  MOD:xxx:R/M (FWORD)
//    104:  xxxxx:REG   (do not increment EIP on 1st pass)
//    204:  xx:SREG:xxx  (do not increment EIP on 1st pass)
//    304:  {imm16} ('sign extend' bit clear)
//    314:  {imm8}  ('sign extend' bit set)

// DEC       // 1111111w MOD:001:R/M | 01001:REG (word/dword ONLY)
// DIV       // 1111011w MOD:110:R/M
// IDIV      // 1111011w MOD:111:R/M
// INC       // 1111111w MOD:000:R/M | 01000:REG
// LGDT      // 00001111 00000001 MOD:010:R/M
// LIDT      // 00001111 00000001 MOD:011:R/M
// LLDT      // 00001111 00000000 MOD:001:R/M
// LMSW      // 00001111 00000001 MOD:110:R/M
// LTR       // 00001111 00000000 MOD:001:R/M
// MUL       // 1111011w MOD:110:R/M
// NEG       // 1111011w MOD:011:R/M
// NOT       // 1111011w MOD:010:R/M
// POP       // 10001111 MOD:000:R/M | 01011:REG | 00001111 10:SREG:001 | 00:SREG:111
// PUSH      // 11111111 MOD:110:R/M | 01010:REG | 00001111 10:SREG:000 | 00:SREG:110 | 011010s0 {IMM}
// SETO      // 00001111 10010000 MOD:000:R/M
// SETNO     // 00001111 10010001 MOD:000:R/M
// SETB      // 00001111 10010010 MOD:000:R/M
// SETAE     // 00001111 10010011 MOD:000:R/M
// SETZ      // 00001111 10010100 MOD:000:R/M
// SETNZ     // 00001111 10010101 MOD:000:R/M
// SETBE     // 00001111 10010110 MOD:000:R/M
// SETA      // 00001111 10010111 MOD:000:R/M
// SETS      // 00001111 10011000 MOD:000:R/M
// SETNS     // 00001111 10011001 MOD:000:R/M
// SETP      // 00001111 10011010 MOD:000:R/M
// SETNP     // 00001111 10011011 MOD:000:R/M
// SETL      // 00001111 10011100 MOD:000:R/M
// SETGE     // 00001111 10011101 MOD:000:R/M
// SETLE     // 00001111 10011110 MOD:000:R/M
// SETG      // 00001111 10011111 MOD:000:R/M
// SGDT      // 00001111 00000001 MOD:000:R/M
// SIDT      // 00001111 00000001 MOD:001:R/M
// SLDT      // 00001111 00000000 MOD:001:R/M
// SMSW      // 00001111 00000001 MOD:100:R/M
// STR       // 00001111 00000000 MOD:001:R/M
// VERR      // 00001111 00000000 MOD:100:R/M
// VERW      // 00001111 00000000 MOD:101:R/M





// TYPE '5' - 2 argument instructions in which 1 is always a register
//      5: MOD:REG:R/M {REG is source; d==0}        (WORD)
//     15: MOD:REG:R/M {REG is source; d==0}        (BYTE)
//     25: MOD:REG:R/M {REG is destination; d==1}   (WORD)
//     35: MOD:REG:R/M {REG is destination; d==1}   (BYTE)
//    105: MOD:xxx:R/M {IMM16}                      ('s' bit clear)
//    115: MOD:xxx:R/M {IMM8}                       ('s' bit set or N/A)
//    205: MOD:SREG:R/M  {sreg is SOURCE}
//    225: MOD:SREG:R/M  {sreg is DEST}
//    305: 11:RRR:REG   {RRR is SOURCE}   CR0/2/3
//    325: 11:RRR:REG   {RRR is DEST}        "
//    405: 11:RRR:REG   {RRR is SOURCE}   DR0/1/2/3/6/7
//    425: 11:RRR:REG   {RRR is DEST}        "
//    505: 11:RRR:REG   {RRR is SOURCE}   TR6/7
//    525: 11:RRR:REG   {RRR is DEST}        "
//    605: MOD:xxx:R/M  {AX/EAX is DEST}
//    615: MOD:xxx:R/M  {AL is DEST}
//    705: MOD:REG:R/M + {IMM}            (3 arg IMUL) (WORD)
//    715: MOD:REG:R/M + {IMM8}           (3 arg IMUL) (BYTE)


// ARPL      // 01100011 MOD:REG:R/M
// BOUND     // 01100010 MOD:REG:R/M
// BSF       // 00001111 10111100 MOD:REG:R/M
// BSR       // 00001111 10111101 MOD:REG:R/M
// BT        // 00001111 [10111010 MOD:100:R/M {IMM8} | 10100011 MOD:REG:R/M]
// BTC       // 00001111 [10111010 MOD:111:R/M {IMM8} | 10111011 MOD:REG:R/M]
// BTR       // 00001111 [10111010 MOD:110:R/M {IMM8} | 10110011 MOD:REG:R/M]
// BTS       // 00001111 [10111010 MOD:101:R/M {IMM8} | 10101011 MOD:REG:R/M]
// LAR       // 00001111 00000010 MOD:REG:R/M
// LEA       // 10001101 MOD:REG:R/M
// LDS       // 11000101 MOD:REG:R/M
// LES       // 11000100 MOD:REG:R/M
// LFS       // 00001111 10110100 MOD:REG:R/M
// LGS       // 00001111 10110101 MOD:REG:R/M
// LSS       // 00001111 10110010 MOD:REG:R/M
// LSL       // 00001111 00000011 MOD:REG:R/M
// (MOV)     // 100011d0 MOD:SREG:R/M            d==1, 'SREG' is destination
// (MOV)     // 00001111 001000[!d]0 11:RRR:REG  d==1, 'RRR' is source
// (MOV)     // 00001111 001000[!d]1 11:RRR:REG
// (MOV)     // 00001111 001001[!d]0 11:RRR:REG
// IMUL      // 00001111 10101111 MOD:REG:R/M | 1111011w MOD:101:R/M | 011010s1 MOD:REG:R/M {IMM}




// TYPE '6' - 2 argument instructions (general)
//      6: MOD:REG:R/M {REG is source; d==0}        (WORD)
//     16: MOD:REG:R/M {REG is source; d==0}        (BYTE)
//     26: MOD:REG:R/M {REG is destination; d==1}   (WORD)
//     36: MOD:REG:R/M {REG is destination; d==1}   (BYTE)
//    106: {DISP}      {AX/EAX is source; !d == 1}  (WORD)
//    116: {DISP}      {AL is source;     !d == 1}  (BYTE)
//    126: {DISP}      {AX/EAX is dest;   !d == 0}  (WORD)
//    136: {DISP}      {AL is dest;       !d == 0}  (BYTE)
//    206: MOD:000:R/M {IMM}                        (WORD)
//    216: MOD:000:R/M {IMM}                        (BYTE)
//    246: XXXX:1:REG  {IMM}                        (WORD)
//    256: XXXX:0:REG  {IMM}                        (BYTE)
//    306: XXXXX:REG   {AX/EAX, REG}                (WORD)
//    406: MOD:XXX:R/M {IMM} {'s' == 0)
//    416: MOD:XXX:R/M {IMM8} {'s' == 1)

// ADC       // 00010Idw MOD:REG:R/M | 100000s1 MOD:010:R/M
// ADD       // 00000Idw MOD:REG:R/M | 100000s1 MOD:000:R/M
// AND       // 00100Idw MOD:REG:R/M | 100000s1 MOD:100:R/M
// CMP       // 00111Idw MOD:REG:R/M | 100000s1 MOD:111:R/M
// MOV       // 100010dw MOD:REG:R/M | 101000[!d]w {DISP} | 1100011w MOD:000:R/M {IMM} | 1011w:REG {IMM}
// MOVSX     // 00001111 1011111w MOD:REG:R/M
// MOVZX     // 00001111 1011011w MOD:REG:R/M
// OR        // 00001Idw MOD:REG:R/M | 100000s1 MOD:001:R/M
// SBB       // 00011Idw MOD:REG:R/M | 100000s1 MOD:011:R/M
// SUB       // 00101Idw MOD:REG:R/M | 100000s1 MOD:101:R/M
// TEST      // 1000010w MOD:REG:R/M | 1111011w MOD:000:R/M {IMM} | 1010100w {IMM}
// XCHG      // 1000011w MOD:REG:R/M | 10010:REG
// XOR       // 00110Idw MOD:REG:R/M | 100000s1 MOD:110:R/M





// TYPE '7' - STRING instructions
//      7: STRING {not cmps,scas}  (WORD)
//     17: STRING {not cmps,scas}  (BYTE)
//     27: STRING {cmps or scas}   (WORD)
//     37: STRING {cmps or scas}   (BYTE)
//    107: IN/OUT AX,{IMM8}        (WORD)
//    117: IN/OUT AL,{IMM8}        (BYTE)
//    207: IN/OUT AX,DX            (WORD)
//    217: IN/OUT AL,DX            (BYTE)

// CMPS      // 1010011w
// INS       // 1010011w
// LODS      // 1010110w
// MOVS      // 1010010w
// OUTS      // 0110111w
// SCAS      // 1010111w
// STOS      // 1010101w
//
// IN        // 1110010w {IMM8} | 1110110w {DX}
// OUT       // 1110010w {IMM8} | 1110110w {DX}





// TYPE '8' - SHIFT and ROTATE instructions
//      8: MOD:xxx:R/M {1 is implied}       (WORD)
//     18: MOD:xxx:R/M {1 is implied}       (BYTE)
//    108: MOD:xxx:R/M {CL is implied}      (WORD)
//    118: MOD:xxx:R/M {CL is implied}      (BYTE)
//    208: MOD:xxx:R/M {IMM8}               (WORD)
//    218: MOD:xxx:R/M {IMM8}               (BYTE)
//    308: MOD:REG:R/M {IMM8}               (SHLD, SHRD)
//    408: MOD:REG:R/M {CL is implied}      (SHLD, SHRD)

// RCL       // 1101000w MOD:010:R/M {1} | 1101001w MOD:010:R/M {CL} | 1100000w MOD:010:R/M {IMM}
// RCR       // 1101000w MOD:011:R/M {1} | 1101001w MOD:011:R/M {CL} | 1100000w MOD:011:R/M {IMM}
// ROL       // 1101000w MOD:000:R/M {1} | 1101001w MOD:000:R/M {CL} | 1100000w MOD:000:R/M {IMM}
// ROR       // 1101000w MOD:001:R/M {1} | 1101001w MOD:001:R/M {CL} | 1100000w MOD:001:R/M {IMM}
// SAR       // 1101000w MOD:111:R/M {1} | 1101001w MOD:111:R/M {CL} | 1100000w MOD:111:R/M {IMM}
// SHL       // 1101000w MOD:100:R/M {1} | 1101001w MOD:100:R/M {CL} | 1100000w MOD:100:R/M {IMM}
// SHR       // 1101000w MOD:101:R/M {1} | 1101001w MOD:101:R/M {CL} | 1100000w MOD:101:R/M {IMM}
// SHLD      // 00001111 10100100 MOD:REG:R/M {IMM} | 00001111 10100101 MOD:REG:R/M {CL}
// SHRD      // 00001111 10101100 MOD:REG:R/M {IMM} | 00001111 10101101 MOD:REG:R/M {CL}





// TYPE '9' to '409' - CALL, JUMP, INT, RET [x]
//      9: {IMM8}
//    109: {DISP}      {NEAR}
//    119: {DISP8}     {SHORT}
//    149: {SEG:OFF}   {FAR}
//    209: MOD:xxx:R/M {NEAR - contains OFFSET}
//    249: MOD:xxx:R/M {FAR - contains SEG:OFF}

// CALL NEAR // 11101000 {DISP}    | 11111111 MOD:010:R/M
// CALL FAR  // 10011010 {SEG:OFF} | 11111111 MOD:011:R/M
// JMP SHORT // 11101011 {DISP8}
// JMP NEAR  // 11101001 {DISP}    | 11111111 MOD:100:R/M
// JMP FAR   // 11101010 {SEG:OFF} | 11111111 MOD:101:R/M
// INT       // 01100010 {IMM8}
// RETN      // 11000011 | 11000010 {IMM8}
// RETF      // 11001011 | 11001010 {IMM8}
