;*************************************************
;** GROUPS.ASM - GROUP DEFINITIONS FOR SFTSHELL **
;*************************************************


CodeSeg  MACRO X
            X SEGMENT WORD PUBLIC 'CODE'
            X ENDS
         ENDM

CodeSegs MACRO Z, A:VARARG
            FOR X:REQ, <A>

               CodeSeg <X>

               Z GROUP X

            ENDM
         ENDM



;CodeSegs MAIN_GRP0, CMDRem_TEXT, PROCESS_COMMAND_TEXT, CMDCalc_TEXT
;CodeSegs MAIN_GRP0, WINCMD_TEXT, WINCMD_C_TEXT, WINCMD_X_TEXT

CodeSegs CODE_GROUP, CMDMkdir_TEXT,    CMDExpand_TEXT,   CMDChdir_TEXT
CodeSegs CODE_GROUP, CMDUndelete_TEXT, CMDMax_TEXT,      CMDCls_TEXT,     CMDFc_TEXT
CodeSegs CODE_GROUP, CMDMin_TEXT,      CMDRmdir_TEXT,    CMDCommand_TEXT, CMDVer_TEXT
CodeSegs CODE_GROUP, CMDFind_TEXT,     CMDMode_TEXT,     CMDComp_TEXT,    CMDMore_TEXT
CodeSegs CODE_GROUP, CMDPath_TEXT,     CMDTruename_TEXT, CMDShare_TEXT
CodeSegs CODE_GROUP, CMDDebug_TEXT,    CMDPrompt_TEXT,   CMDReplace_TEXT
CodeSegs CODE_GROUP, CMDSort_TEXT,     CMD_Bad_TEXT,     CMDEdit_TEXT,    CMDRexx_TEXT



END
