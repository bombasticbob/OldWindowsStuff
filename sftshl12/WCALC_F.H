/***************************************************************************/
/*                                                                         */
/*   WCALC_F.H - Command line interpreter for Microsoft(r) Windows (tm)    */
/*                                                                         */
/*                      TRADE SECRET - CONFIDENTIAL                        */
/*                                                                         */
/*             The compiled libraries and API header files are             */
/*          (c) Copyright 1990-1995 by Stewart~Frazier Tools, Inc          */
/*                                                                         */
/*        This contains the function name definitions for the 'CALC'       */
/*            function and 'related' functions used elsewhere              */
/*                                                                         */
/***************************************************************************/



#pragma code_seg("CALC_FUNCTION_TEXT","CODE")
#pragma code_seg()

#define CALC_FUNCTION_BASED __based(_segname("CALC_FUNCTION_TEXT"))


 // THE FOLLOWING ITEMS ARE 'CODE-BASED' ARRAYS FOR 'CalcEvaluateFunction()'



static const char CALC_FUNCTION_BASED szLEN[]                = "LEN";
static const char CALC_FUNCTION_BASED szSTR[]                = "STR";
static const char CALC_FUNCTION_BASED szVAL[]                = "VAL";
static const char CALC_FUNCTION_BASED szLEFT[]               = "LEFT";
static const char CALC_FUNCTION_BASED szRIGHT[]              = "RIGHT";
static const char CALC_FUNCTION_BASED szMID[]                = "MID";
static const char CALC_FUNCTION_BASED szUPPER[]              = "UPPER";
static const char CALC_FUNCTION_BASED szLOWER[]              = "LOWER";
static const char CALC_FUNCTION_BASED szISYES[]              = "ISYES";
static const char CALC_FUNCTION_BASED szISNO[]               = "ISNO";
static const char CALC_FUNCTION_BASED szISTASK[]             = "ISTASK";
static const char CALC_FUNCTION_BASED szISWINDOW[]           = "ISWINDOW";
static const char CALC_FUNCTION_BASED szISINSTANCE[]         = "ISINSTANCE";
static const char CALC_FUNCTION_BASED szISMODULE[]           = "ISMODULE";
static const char CALC_FUNCTION_BASED szGETWINDOW[]          = "GETWINDOW";
static const char CALC_FUNCTION_BASED szGETTASK[]            = "GETTASK";
static const char CALC_FUNCTION_BASED szGETINSTANCE[]        = "GETINSTANCE";
static const char CALC_FUNCTION_BASED szGETMODULE[]          = "GETMODULE";
static const char CALC_FUNCTION_BASED szSIN[]                = "SIN";
static const char CALC_FUNCTION_BASED szCOS[]                = "COS";
static const char CALC_FUNCTION_BASED szTAN[]                = "TAN";
static const char CALC_FUNCTION_BASED szASIN[]               = "ASIN";
static const char CALC_FUNCTION_BASED szACOS[]               = "ACOS";
static const char CALC_FUNCTION_BASED szATAN[]               = "ATAN";
static const char CALC_FUNCTION_BASED szLOG[]                = "LOG";
static const char CALC_FUNCTION_BASED szLN[]                 = "LN";
static const char CALC_FUNCTION_BASED szEXP[]                = "EXP";
static const char CALC_FUNCTION_BASED szEXP10[]              = "EXP10";
static const char CALC_FUNCTION_BASED szINT[]                = "INT";
static const char CALC_FUNCTION_BASED szTRUNC[]              = "TRUNC";
static const char CALC_FUNCTION_BASED szROUND[]              = "ROUND";
static const char CALC_FUNCTION_BASED szPAUSE[]              = "PAUSE";
static const char CALC_FUNCTION_BASED szINPUT[]              = "INPUT";
static const char CALC_FUNCTION_BASED szPARSE[]              = "PARSE";
static const char CALC_FUNCTION_BASED szCENTER[]             = "CENTER";
static const char CALC_FUNCTION_BASED szLJ[]                 = "LJ";
static const char CALC_FUNCTION_BASED szRJ[]                 = "RJ";
static const char CALC_FUNCTION_BASED szLTRIM[]              = "LTRIM";
static const char CALC_FUNCTION_BASED szRTRIM[]              = "RTRIM";
static const char CALC_FUNCTION_BASED szCHAR[]               = "CHAR";
static const char CALC_FUNCTION_BASED szASC[]                = "ASC";
static const char CALC_FUNCTION_BASED szNOW[]                = "NOW";
static const char CALC_FUNCTION_BASED szDATE[]               = "DATE";
static const char CALC_FUNCTION_BASED szDATEVAL[]            = "DATEVAL";
static const char CALC_FUNCTION_BASED szTIME[]               = "TIME";
static const char CALC_FUNCTION_BASED szTIMEVAL[]            = "TIMEVAL";
static const char CALC_FUNCTION_BASED szDOW[]                = "DOW";
static const char CALC_FUNCTION_BASED szSECONDS[]            = "SECONDS";
static const char CALC_FUNCTION_BASED szTASKLIST[]           = "TASKLIST";
static const char CALC_FUNCTION_BASED szMODULELIST[]         = "MODULELIST";
static const char CALC_FUNCTION_BASED szDIRLIST[]            = "DIRLIST";
static const char CALC_FUNCTION_BASED szFILEINFO[]           = "FILEINFO";
static const char CALC_FUNCTION_BASED szMODULEINFO[]         = "MODULEINFO";
static const char CALC_FUNCTION_BASED szTASKINFO[]           = "TASKINFO";
static const char CALC_FUNCTION_BASED szINSTR[]              = "INSTR";
static const char CALC_FUNCTION_BASED szTRUENAME[]           = "TRUENAME";
static const char CALC_FUNCTION_BASED szCHDIR[]              = "CHDIR";
static const char CALC_FUNCTION_BASED szCHDRIVE[]            = "CHDRIVE";
static const char CALC_FUNCTION_BASED szGETDIR[]             = "GETDIR";
static const char CALC_FUNCTION_BASED szGETDRIVE[]           = "GETDRIVE";
static const char CALC_FUNCTION_BASED szGETCURSOR[]          = "GETCURSOR";
static const char CALC_FUNCTION_BASED szSETCURSOR[]          = "SETCURSOR";
static const char CALC_FUNCTION_BASED szGETBKCOLOR[]         = "GETBKCOLOR";
static const char CALC_FUNCTION_BASED szSETBKCOLOR[]         = "SETBKCOLOR";
static const char CALC_FUNCTION_BASED szGETTEXTCOLOR[]       = "GETTEXTCOLOR";
static const char CALC_FUNCTION_BASED szSETTEXTCOLOR[]       = "SETTEXTCOLOR";
static const char CALC_FUNCTION_BASED szPRINTSTRING[]        = "PRINTSTRING";
static const char CALC_FUNCTION_BASED szGETCLIPBOARDDATA[]   = "GETCLIPBOARDDATA";
static const char CALC_FUNCTION_BASED szSETCLIPBOARDDATA[]   = "SETCLIPBOARDDATA";
static const char CALC_FUNCTION_BASED szSHOWWINDOW[]         = "SHOWWINDOW";
static const char CALC_FUNCTION_BASED szSENDKEYS[]           = "SENDKEYS";
static const char CALC_FUNCTION_BASED szSENDVIRTUALKEY[]     = "SENDVIRTUALKEY";
static const char CALC_FUNCTION_BASED szGETCHILDWINDOW[]     = "GETCHILDWINDOW";
static const char CALC_FUNCTION_BASED szFILEOPEN[]           = "FILEOPEN";
static const char CALC_FUNCTION_BASED szREADLN[]             = "READLN";
static const char CALC_FUNCTION_BASED szWRITELN[]            = "WRITELN";
static const char CALC_FUNCTION_BASED szFILECLOSE[]          = "FILECLOSE";
static const char CALC_FUNCTION_BASED szISEOF[]              = "ISEOF";
static const char CALC_FUNCTION_BASED szGETPROFILESTRING[]   = "GETPROFILESTRING";
static const char CALC_FUNCTION_BASED szWRITEPROFILESTRING[] = "WRITEPROFILESTRING";
static const char CALC_FUNCTION_BASED szNROWS[]              = "NROWS";
static const char CALC_FUNCTION_BASED szNCOLS[]              = "NCOLS";
static const char CALC_FUNCTION_BASED szFILESMATCH[]         = "FILESMATCH";
static const char CALC_FUNCTION_BASED szSTRING[]             = "STRING";
static const char CALC_FUNCTION_BASED szFINDWINDOW[]         = "FINDWINDOW";
static const char CALC_FUNCTION_BASED szFINDCHILDWINDOW[]    = "FINDCHILDWINDOW";

static const char CALC_FUNCTION_BASED szGETSCREENFONT[]      = "GETSCREENFONT";
static const char CALC_FUNCTION_BASED szSETSCREENFONT[]      = "SETSCREENFONT";

static const char CALC_FUNCTION_BASED szODBCCONNECTSTRING[]  = "ODBCCONNECTSTRING";
static const char CALC_FUNCTION_BASED szODBCGETINFO[]        = "ODBCGETINFO";
static const char CALC_FUNCTION_BASED szODBCGETTABLES[]      = "ODBCGETTABLES";
static const char CALC_FUNCTION_BASED szODBCGETDRIVERS[]     = "ODBCGETDRIVERS";

static const char CALC_FUNCTION_BASED szODBCGETDATA[]        = "ODBCGETDATA";
static const char CALC_FUNCTION_BASED szODBCGETCOLUMNINFO[]  = "ODBCGETCOLUMNINFO";
static const char CALC_FUNCTION_BASED szODBCGETCOLUMNINDEX[] = "ODBCGETCOLUMNINDEX";
static const char CALC_FUNCTION_BASED szODBCGETCURSORNAME[]  = "ODBCGETCURSORNAME";

static const char CALC_FUNCTION_BASED szMONTH[]              = "MONTH";
static const char CALC_FUNCTION_BASED szDAY[]                = "DAY";
static const char CALC_FUNCTION_BASED szYEAR[]               = "YEAR";
static const char CALC_FUNCTION_BASED szHOUR[]               = "HOUR";
static const char CALC_FUNCTION_BASED szMINUTE[]             = "MINUTE";
static const char CALC_FUNCTION_BASED szSECOND[]             = "SECOND";

static const char CALC_FUNCTION_BASED szREGDELETEKEY[]        = "REGDELETEKEY";
static const char CALC_FUNCTION_BASED szREGDELETEVALUE[]      = "REGDELETEVALUE";
static const char CALC_FUNCTION_BASED szREGENUMKEYS[]         = "REGENUMKEYS";
static const char CALC_FUNCTION_BASED szREGENUMVALUES[]       = "REGENUMVALUES";
static const char CALC_FUNCTION_BASED szREGQUERYVALUE[]       = "REGQUERYVALUE";
static const char CALC_FUNCTION_BASED szREGSETVALUE[]         = "REGSETVALUE";

static const char CALC_FUNCTION_BASED szNETGETUNC[]           = "NETGETUNC";
static const char CALC_FUNCTION_BASED szNETUSE[]              = "NETUSE";
static const char CALC_FUNCTION_BASED szNETVIEW[]             = "NETVIEW";


//static const char CALC_FUNCTION_BASED szSQLDATETIME[]        = "SQLDATETIME";
//static const char CALC_FUNCTION_BASED szSQLDATETIMEVAL[]     = "SQLDATETIMEVAL";

//static const char CALC_FUNCTION_BASED szODBCGETDRIVERSTATUS[]= "ODBCGETDRIVERSTATUS";
//static const char CALC_FUNCTION_BASED szODBCGETROWCOUNT[]    = "ODBCGETROWCOUNT";
//static const char CALC_FUNCTION_BASED szODBCGETCURRENTROW[]  = "ODBCGETCURRENTROW";
//static const char CALC_FUNCTION_BASED szODBCSETCURRENTROW[]  = "ODBCSETCURRENTROW";




enum {     // first one must have '=0' on it...
   idACOS=0,
   idASC,
   idASIN,
   idATAN,
   idCENTER,
   idCHAR,
   idCHDIR,
   idCHDRIVE,
   idCOS,
   idDATE,
   idDATEVAL,
   idDAY,
   idDIRLIST,
   idDOW,
   idEXP,
   idEXP10,
   idFILECLOSE,
   idFILEINFO,
   idFILEOPEN,
   idFILESMATCH,
   idFINDCHILDWINDOW,
   idFINDWINDOW,
   idGETBKCOLOR,
   idGETCHILDWINDOW,
   idGETCLIPBOARDDATA,
   idGETCURSOR,
   idGETDIR,
   idGETDRIVE,
   idGETINSTANCE,
   idGETMODULE,
   idGETPROFILESTRING,
   idGETSCREENFONT,
   idGETTASK,
   idGETTEXTCOLOR,
   idGETWINDOW,
   idHOUR,
   idINPUT,
   idINSTR,
   idINT,
   idISEOF,
   idISINSTANCE,
   idISMODULE,
   idISNO,
   idISTASK,
   idISWINDOW,
   idISYES,
   idLEFT,
   idLEN,
   idLJ,
   idLN,
   idLOG,
   idLOWER,
   idLTRIM,
   idMID,
   idMINUTE,
   idMODULEINFO,
   idMODULELIST,
   idMONTH,
   idNCOLS,
   idNETGETUNC,
   idNETUSE,
   idNETVIEW,
   idNOW,
   idNROWS,
   idODBCCONNECTSTRING,
   idODBCGETCOLUMNINDEX,
   idODBCGETCOLUMNINFO,
   idODBCGETCURSORNAME,
   idODBCGETDATA,
   idODBCGETDRIVERS,
   idODBCGETINFO,
   idODBCGETTABLES,
   idPARSE,
   idPAUSE,
   idPRINTSTRING,
   idREADLN,
   idREGDELETEKEY,
   idREGDELETEVALUE,
   idREGENUMKEYS,
   idREGENUMVALUES,
   idREGQUERYVALUE,
   idREGSETVALUE,
   idRIGHT,
   idRJ,
   idROUND,
   idRTRIM,
   idSECOND,
   idSECONDS,
   idSENDKEYS,
   idSENDVIRTUALKEY,
   idSETBKCOLOR,
   idSETCLIPBOARDDATA,
   idSETCURSOR,
   idSETSCREENFONT,
   idSETTEXTCOLOR,
   idSHOWWINDOW,
   idSIN,
//   idSQLDATETIME,
//   idSQLDATETIMEVAL,
   idSTR,
   idSTRING,
   idTAN,
   idTASKINFO,
   idTASKLIST,
   idTIME,
   idTIMEVAL,
   idTRUENAME,
   idTRUNC,
   idUPPER,
   idVAL,
   idWRITELN,
   idWRITEPROFILESTRING,
   idYEAR

};


typedef struct {

   const char CALC_FUNCTION_BASED *pName;
   BYTE ParmsMin, ParmsMax;

  } CALC_FUNCTION_LIST;


#define CALC_FUNCTION_ENTRY(X,Y,Z) {(const char CALC_FUNCTION_BASED *)X,Y,Z}

static const CALC_FUNCTION_LIST CALC_FUNCTION_BASED pFunctionList[]={
   CALC_FUNCTION_ENTRY(szACOS,               1, 1),   //  ACOS
   CALC_FUNCTION_ENTRY(szASC,                1, 1),   //  ASC
   CALC_FUNCTION_ENTRY(szASIN,               1, 1),   //  ASIN
   CALC_FUNCTION_ENTRY(szATAN,               1, 1),   //  ATAN
   CALC_FUNCTION_ENTRY(szCENTER,             1, 1),   //  CENTER
   CALC_FUNCTION_ENTRY(szCHAR,               1, 1),   //  CHAR
   CALC_FUNCTION_ENTRY(szCHDIR,              1, 1),   //  CHDIR
   CALC_FUNCTION_ENTRY(szCHDRIVE,            1, 1),   //  CHDRIVE
   CALC_FUNCTION_ENTRY(szCOS,                1, 1),   //  COS
   CALC_FUNCTION_ENTRY(szDATE,               0, 1),   //  DATE
   CALC_FUNCTION_ENTRY(szDATEVAL,            0, 1),   //  DATEVAL
   CALC_FUNCTION_ENTRY(szDAY,                0, 1),   //  DAY
   CALC_FUNCTION_ENTRY(szDIRLIST,            1, 2),   //  DIRLIST
   CALC_FUNCTION_ENTRY(szDOW,                0, 1),   //  DOW
   CALC_FUNCTION_ENTRY(szEXP,                1, 1),   //  EXP
   CALC_FUNCTION_ENTRY(szEXP10,              1, 1),   //  EXP10
   CALC_FUNCTION_ENTRY(szFILECLOSE,          1, 1),   //  FILECLOSE
   CALC_FUNCTION_ENTRY(szFILEINFO,           1, 1),   //  FILEINFO
   CALC_FUNCTION_ENTRY(szFILEOPEN,           2, 2),   //  FILEOPEN
   CALC_FUNCTION_ENTRY(szFILESMATCH,         2, 2),   //  FILESMATCH
   CALC_FUNCTION_ENTRY(szFINDCHILDWINDOW,    1, 4),   //  FINDCHILDWINDOW
   CALC_FUNCTION_ENTRY(szFINDWINDOW,         0, 3),   //  FINDWINDOW
   CALC_FUNCTION_ENTRY(szGETBKCOLOR,         0, 0),   //  GETBKCOLOR
   CALC_FUNCTION_ENTRY(szGETCHILDWINDOW,     2, 2),   //  GETCHILDWINDOW
   CALC_FUNCTION_ENTRY(szGETCLIPBOARDDATA,   0, 0),   //  GETCLIPBOARDDATA
   CALC_FUNCTION_ENTRY(szGETCURSOR,          0, 0),   //  GETCURSOR
   CALC_FUNCTION_ENTRY(szGETDIR,             0, 1),   //  GETDIR
   CALC_FUNCTION_ENTRY(szGETDRIVE,           0, 0),   //  GETDRIVE
   CALC_FUNCTION_ENTRY(szGETINSTANCE,        0, 2),   //  GETINSTANCE
   CALC_FUNCTION_ENTRY(szGETMODULE,          0, 2),   //  GETMODULE
   CALC_FUNCTION_ENTRY(szGETPROFILESTRING,   2, 3),   //  GETPROFILESTRING
   CALC_FUNCTION_ENTRY(szGETSCREENFONT,      0, 0),   //  GETSCREENFONT
   CALC_FUNCTION_ENTRY(szGETTASK,            0, 2),   //  GETTASK
   CALC_FUNCTION_ENTRY(szGETTEXTCOLOR,       0, 0),   //  GETTEXTCOLOR
   CALC_FUNCTION_ENTRY(szGETWINDOW,          0, 2),   //  GETWINDOW
   CALC_FUNCTION_ENTRY(szHOUR,               0, 1),   //  HOUR
   CALC_FUNCTION_ENTRY(szINPUT,              0, 2),   //  INPUT
   CALC_FUNCTION_ENTRY(szINSTR,              2, 3),   //  INSTR
   CALC_FUNCTION_ENTRY(szINT,                1, 1),   //  INT
   CALC_FUNCTION_ENTRY(szISEOF,              1, 1),   //  ISEOF
   CALC_FUNCTION_ENTRY(szISINSTANCE,         1, 1),   //  ISINSTANCE
   CALC_FUNCTION_ENTRY(szISMODULE,           1, 1),   //  ISMODULE
   CALC_FUNCTION_ENTRY(szISNO,               1, 1),   //  ISNO
   CALC_FUNCTION_ENTRY(szISTASK,             1, 1),   //  ISTASK
   CALC_FUNCTION_ENTRY(szISWINDOW,           1, 1),   //  ISWINDOW
   CALC_FUNCTION_ENTRY(szISYES,              1, 1),   //  ISYES
   CALC_FUNCTION_ENTRY(szLEFT,               2, 2),   //  LEFT
   CALC_FUNCTION_ENTRY(szLEN,                1, 1),   //  LEN
   CALC_FUNCTION_ENTRY(szLJ,                 1, 1),   //  LJ
   CALC_FUNCTION_ENTRY(szLN,                 1, 1),   //  LN
   CALC_FUNCTION_ENTRY(szLOG,                1, 1),   //  LOG
   CALC_FUNCTION_ENTRY(szLOWER,              1, 1),   //  LOWER
   CALC_FUNCTION_ENTRY(szLTRIM,              1, 1),   //  LTRIM
   CALC_FUNCTION_ENTRY(szMID,                2, 3),   //  MID
   CALC_FUNCTION_ENTRY(szMINUTE,             0, 1),   //  MINUTE
   CALC_FUNCTION_ENTRY(szMODULEINFO,         1, 1),   //  MODULEINFO
   CALC_FUNCTION_ENTRY(szMODULELIST,         0, 1),   //  MODULELIST
   CALC_FUNCTION_ENTRY(szMONTH,              0, 1),   //  MONTH
   CALC_FUNCTION_ENTRY(szNCOLS,              1, 1),   //  NCOLS
   CALC_FUNCTION_ENTRY(szNETGETUNC,          1, 1),   //  NETGETUNC
   CALC_FUNCTION_ENTRY(szNETUSE,             0, 2),   //  NETUSE
   CALC_FUNCTION_ENTRY(szNETVIEW,            0, 1),   //  NETVIEW
   CALC_FUNCTION_ENTRY(szNOW,                0, 0),   //  NOW
   CALC_FUNCTION_ENTRY(szNROWS,              1, 1),   //  NROWS
   CALC_FUNCTION_ENTRY(szODBCCONNECTSTRING,  1, 1),   //  ODBCCONNECTSTRING
   CALC_FUNCTION_ENTRY(szODBCGETCOLUMNINDEX, 2, 2),   //  ODBCGETCOLUMNINDEX
   CALC_FUNCTION_ENTRY(szODBCGETCOLUMNINFO,  1, 2),   //  ODBCGETCOLUMNINFO
   CALC_FUNCTION_ENTRY(szODBCGETCURSORNAME,  1, 1),   //  ODBCGETCURSORNAME
   CALC_FUNCTION_ENTRY(szODBCGETDATA,        1, 2),   //  ODBCGETDATA
   CALC_FUNCTION_ENTRY(szODBCGETDRIVERS,     0, 0),   //  ODBCGETDRIVERS
   CALC_FUNCTION_ENTRY(szODBCGETINFO,        2, 2),   //  ODBCGETINFO
   CALC_FUNCTION_ENTRY(szODBCGETTABLES,      1, 1),   //  ODBCGETTABLES
   CALC_FUNCTION_ENTRY(szPARSE,              1, 1),   //  PARSE
   CALC_FUNCTION_ENTRY(szPAUSE,              1, 1),   //  PAUSE
   CALC_FUNCTION_ENTRY(szPRINTSTRING,        1, 1),   //  PRINTSTRING
   CALC_FUNCTION_ENTRY(szREADLN,             1, 1),   //  READLN
   CALC_FUNCTION_ENTRY(szREGDELETEKEY,       1, 1),   //  REGDELETEKEY
   CALC_FUNCTION_ENTRY(szREGDELETEVALUE,     2, 2),   //  REGDELETEVALUE
   CALC_FUNCTION_ENTRY(szREGENUMKEYS,        1, 1),   //  REGENUMKEYS
   CALC_FUNCTION_ENTRY(szREGENUMVALUES,      1, 1),   //  REGENUMVALUES
   CALC_FUNCTION_ENTRY(szREGQUERYVALUE,      2, 2),   //  REGQUERYVALUE
   CALC_FUNCTION_ENTRY(szREGSETVALUE,        3, 4),   //  REGSETVALUE
   CALC_FUNCTION_ENTRY(szRIGHT,              2, 2),   //  RIGHT
   CALC_FUNCTION_ENTRY(szRJ,                 1, 1),   //  RJ
   CALC_FUNCTION_ENTRY(szROUND,              1, 1),   //  ROUND
   CALC_FUNCTION_ENTRY(szRTRIM,              1, 1),   //  RTRIM
   CALC_FUNCTION_ENTRY(szSECOND,             0, 1),   //  SECOND
   CALC_FUNCTION_ENTRY(szSECONDS,            0, 1),   //  SECONDS
   CALC_FUNCTION_ENTRY(szSENDKEYS,           2, 2),   //  SENDKEYS
   CALC_FUNCTION_ENTRY(szSENDVIRTUALKEY,     2, 3),   //  SENDVIRTUALKEY
   CALC_FUNCTION_ENTRY(szSETBKCOLOR,         1, 1),   //  SETBKCOLOR
   CALC_FUNCTION_ENTRY(szSETCLIPBOARDDATA,   0, 1),   //  SETCLIPBOARDDATA
   CALC_FUNCTION_ENTRY(szSETCURSOR,          1, 2),   //  SETCURSOR
   CALC_FUNCTION_ENTRY(szSETSCREENFONT,      0, 1),   //  SETSCREENFONT
   CALC_FUNCTION_ENTRY(szSETTEXTCOLOR,       1, 1),   //  SETTEXTCOLOR
   CALC_FUNCTION_ENTRY(szSHOWWINDOW,         1, 2),   //  SHOWWINDOW
   CALC_FUNCTION_ENTRY(szSIN,                1, 1),   //  SIN
//   CALC_FUNCTION_ENTRY(szSQLDATETIME,        1, 1),   //  SQLDATETIME
//   CALC_FUNCTION_ENTRY(szSQLDATETIMEVAL,     1, 1),   //  SQLDATETIMEVAL
   CALC_FUNCTION_ENTRY(szSTR,                1, 2),   //  STR
   CALC_FUNCTION_ENTRY(szSTRING,             2, 2),   //  STRING
   CALC_FUNCTION_ENTRY(szTAN,                1, 1),   //  TAN
   CALC_FUNCTION_ENTRY(szTASKINFO,           1, 1),   //  TASKINFO
   CALC_FUNCTION_ENTRY(szTASKLIST,           0, 1),   //  TASKLIST
   CALC_FUNCTION_ENTRY(szTIME,               0, 1),   //  TIME
   CALC_FUNCTION_ENTRY(szTIMEVAL,            0, 1),   //  TIMEVAL
   CALC_FUNCTION_ENTRY(szTRUENAME,           1, 1),   //  TRUENAME
   CALC_FUNCTION_ENTRY(szTRUNC,              1, 1),   //  TRUNC
   CALC_FUNCTION_ENTRY(szUPPER,              1, 1),   //  UPPER
   CALC_FUNCTION_ENTRY(szVAL,                1, 1),   //  VAL
   CALC_FUNCTION_ENTRY(szWRITELN,            2, 2),   //  WRITELN
   CALC_FUNCTION_ENTRY(szWRITEPROFILESTRING, 3, 4),   //  WRITEPROFILESTRING
   CALC_FUNCTION_ENTRY(szYEAR,               0, 1)    //  YEAR
   };


static const char CALC_FUNCTION_BASED szSunday[]    = "Sunday";
static const char CALC_FUNCTION_BASED szMonday[]    = "Monday";
static const char CALC_FUNCTION_BASED szTuesday[]   = "Tuesday";
static const char CALC_FUNCTION_BASED szWednesday[] = "Wednesday";
static const char CALC_FUNCTION_BASED szThursday[]  = "Thursday";
static const char CALC_FUNCTION_BASED szFriday[]    = "Friday";
static const char CALC_FUNCTION_BASED szSaturday[]  = "Saturday";


static const char CALC_FUNCTION_BASED * const CALC_FUNCTION_BASED pDOW[]={
    (const char CALC_FUNCTION_BASED *)szSunday,
    (const char CALC_FUNCTION_BASED *)szMonday,
    (const char CALC_FUNCTION_BASED *)szTuesday,
    (const char CALC_FUNCTION_BASED *)szWednesday,
    (const char CALC_FUNCTION_BASED *)szThursday,
    (const char CALC_FUNCTION_BASED *)szFriday,
    (const char CALC_FUNCTION_BASED *)szSaturday
  };


static const UINT CODE_BASED uiFontMenuID[]={
   IDM_FONT_NORMAL, IDM_FONT7x5, IDM_FONT8x6, IDM_FONT9x7,
   IDM_FONT12x8, IDM_FONT16x12, IDM_FONT24x18, IDM_FONT32x24,
   IDM_FONT48x36, IDM_FONT_MENU };

static const UINT CODE_BASED uiFontValue[]={
   0, 7, 8, 9, 12, 16, 24, 32, 48, (UINT)-1 };
