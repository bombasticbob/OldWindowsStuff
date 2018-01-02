/***************************************************************************/
/*                                                                         */
/*   WINCMD_F.H - Command line interpreter for Microsoft(r) Windows (tm)   */
/*                                                                         */
/*                      TRADE SECRET - CONFIDENTIAL                        */
/*                                                                         */
/*             The compiled libraries and API header files are             */
/*          (c) Copyright 1990-1995 by Stewart~Frazier Tools, Inc          */
/*                                                                         */
/*    This file contains function prototypes for GLOBAL functions and      */
/*    non-critical external variable definitions.  Normally a source       */
/*    file that includes this header file does *NOT* need to have it as    */
/*    a build dependency; however a major 'revamping' may require a        */
/*    'build all' to be performed in order to prevent problems.            */
/*                                                                         */
/***************************************************************************/



#include "sql.h"      // ODBC
#include "sqlext.h"   // ODBC


#pragma warning(error: 4013) /* undefined function - assuming external */
#pragma warning(error: 4020) /* too many actual parameters */


typedef unsigned long HWDB;   /* required for certain 'WDBUTIL' functions */



                        /*************************/
                        /*** UTILITY FUNCTIONS ***/
                        /*************************/


                           /* 'Generic' stuff */

BOOL FAR PASCAL DOS3Call(void);              /* assembly-only API function */

WORD FAR PASCAL MyErrorMessageBox(HWND hWnd, LPSTR lpText, LPSTR lpTitle);

//BOOL FAR PASCAL LoopDispatch(void);

void FAR PASCAL UpdateStatusBar(void);

void FAR PASCAL BatchIteration(void);/* performs 1 iteration for the batch */
                   /* processing loop.  A function should call this *ONLY* */
                   /* if 'MthreadGetCurrentThread()==hBatchThread' and batch file */
                   /* execution must return to the current proc when done. */

void FAR PASCAL RemoveCodeBlockEntry(CODE_BLOCK FAR *lpCB);
  // call this to remove ANY 'CODE_BLOCK' entry from 'lpBatchInfo' in order
  // to ensure that all resources are properly freed.


             /* Functions to process ENVIRONMENT STRINGS */

BOOL FAR PASCAL ItemHasHotLink(LPCSTR szItem);  // TRUE if env string HOT LINKED

LPSTR FAR PASCAL GetEnvString(LPCSTR lpName);

BOOL FAR PASCAL SetEnvString(LPCSTR lpName, LPCSTR lpValue);

BOOL FAR PASCAL DelEnvString(LPCSTR lpName);

LPSTR FAR PASCAL GetMyEnvironment(void);

BOOL FAR PASCAL FreeMyEnvironment(void);

LPSTR FAR PASCAL EvalString(LPSTR lpStr); /* caller must 'GlobalFreePtr()' */

double FAR PASCAL CalcEvalNumber(LPSTR lpS);       // part of WCALC.C
LPSTR FAR PASCAL StringFromFloat(double d);        // part of WCALC.C
LPSTR FAR PASCAL StringFromInt(long l, int radix); // part of WCALC.C



                  /* Functions to PRINT TO THE SCREEN */

void FAR PASCAL ReDirectOutput(PARSE_INFO FAR *lpParseInfo, BOOL redir_flag);

BOOL FAR PASCAL PrintAChar(int chr);

BOOL FAR PASCAL PrintBuffer(LPCSTR lpBuf, WORD wCount);

void FAR PASCAL PrintErrorMessage(WORD wMessageID);

BOOL FAR PASCAL FarCheckScreenScroll(BOOL PaintFlag);

BOOL FAR PASCAL PrintString(LPCSTR lpString);
BOOL FAR PASCAL PrintStringX(LPCSTR lpString);
   // Note:  'PrintStringX()' like 'PrintString()' except that it pauses
   //        after each 25 lines.




void FAR PASCAL ReDisplayUserInput(LPCSTR lpcBuffer);   // cursor at end
void FAR PASCAL ReDisplayUserInput0(LPCSTR lpcBuffer);  // cursor left as-is

BOOL FAR PASCAL DisplayPrompt(void);

BOOL FAR PASCAL MoveCursor(int x, int y);

BOOL FAR PASCAL ClearScreen(void);

BOOL NEAR _fastcall ProcessAttribCode(int code);


HCURSOR FAR PASCAL MySetCursor(HCURSOR hCursor);

void FAR PASCAL UpdateTextMetrics(HWND hWnd);
  // call THIS whenever I need to ensure 'tmMain' has been assigned properly



             /* Functions which Process Command Line Stuff */

WORD FAR PASCAL GetUserInput(LPSTR lpBuf);  /* special - gets string */
WORD FAR PASCAL GetUserInput2(LPSTR lpBuf);  // like above, but allows 'default'

BOOL FAR PASCAL ProcessCommand(LPSTR lpCmd);

LPPARSE_INFO FAR PASCAL CMDLineParse(LPSTR lpCmdLine);
LPPARSE_INFO FAR PASCAL CMDRunProgramCommandLineParse(LPSTR lpCmdLine);

BOOL FAR PASCAL QualifyPath(LPSTR dest, LPSTR src, BOOL bWildFlag);

int FAR PASCAL GetPathNameInfo(LPSTR lpPath);

volatile LPSTR FAR PASCAL GetExtension(const LPSTR lpPath);

BOOL FAR PASCAL NewExtension(volatile LPSTR lpPath, const LPSTR lpNewExt);


BOOL FAR PASCAL MyGetDiskFreeSpace(WORD wDrive, struct _diskfree_t FAR *lpDFT);


BOOL FAR PASCAL GetDriveLabel(LPSTR lpDest, LPSTR lpSrc);
             /* also includes 'SERIAL NUMBER ####-####' for Dos >=4.0 */

BOOL FAR PASCAL GetDriveLabelOnly(LPSTR lpDest, LPSTR lpSrc);
             // like above, but doesn't print any warning messages when
             // there's no label (generates special labels instead)

LPTREE FAR PASCAL GetDirTree(LPSTR lpPath);  /* returns globally allocated */
                                             /* 'TREE' structure for path */

BOOL FAR PASCAL GetDirTreeLevel(LPTREE FAR *lplpTree, NPSTR pPath,
                                WORD FAR *lpwTreeSize, WORD FAR *lpwNext,
                                DWORD FAR *lpdwTickCount);

WORD FAR PASCAL GetDirList(LPSTR lpFileSpec, int attrib,
                           LPDIR_INFO FAR *lplpDirInfo, LPSTR lpPathBuffer);

WORD FAR PASCAL GetDirList0(LPSTR lpFileSpec, int attrib,
                            LPDIR_INFO FAR *lplpDirInfo, LPSTR lpPathBuffer,
                            BOOL wQualifyFlag, LPSTR FAR *lpLFNInfo);

void FAR PASCAL ShortToLongName(LPSTR lpName, LPSTR lpPath);
   // use this function to translate a 'short' name to a long name, and
   // specify the actual path in 'lpPath' if not included in 'lpName' or
   // the path in 'lpName' is not correct.  'lpPath' is NULL to keep any
   // path intact in 'lpName', or a blank string to strip path from 'lpName'.


BOOL FAR PASCAL UpdateFileName(LPSTR lpDest, LPSTR lpPattern,LPSTR lpName);
      /* function to create drive:\path\name from 'pattern' and 'filename' */

BOOL FAR PASCAL UpdateFileName2(LPSTR lpDest, LPSTR lpPattern,LPSTR lpName,
                                BOOL bDirCheckFlag);
      // like above, but it lets you specify whether to add "*.*" to dir
      // names in 'lpPattern' that don't already have wildcards in them.


BOOL FAR PASCAL SubmitFilesToCopy(LPSTR lpSrc, LPSTR lpDest, BOOL verify,
                                  BOOL def_binary, BOOL fromfile_binary,
                                  BOOL tofile_binary);

BOOL FAR PASCAL SubmitDirInfoToCopy(LPDIR_INFO lpDISrc, WORD wNFiles,
                                    LPSTR lpPath, LPSTR lpDest, BOOL verify,
                                    BOOL modify);

void FAR PASCAL ReportCopyResults(void);



                    /** Low Level Functions, etc. **/

char FAR *_lgetdcwd(int drive, char FAR *buffer, int maxlen);

UINT FAR PASCAL MyRead(HFILE hFile, void HUGE *hpBuf, UINT cbBuf);

UINT FAR PASCAL MyWrite(HFILE hFile, const void HUGE *hpBuf, UINT cbBuf);

UINT FAR PASCAL MyWrite2(HFILE hFile, void HUGE *hpBuf, UINT cbBuf,
                         UINT wDestSectorSize);
   // use 'MyWrite2()' when files are open in 'no buffer' mode


HFILE FAR PASCAL MyDupFileHandle(HFILE hFile);     // create dup handle

BOOL FAR PASCAL MyAssignFileHandle(HFILE hFile, HFILE hDupFile);
                         // force 'hFile' to refer to same file as 'hDupFile'


void FAR PASCAL GetExtErrMessage(LPSTR lpMsgBuf);

void SaveDosErrorCodes(void);

void RestoreDosErrorCodes(void);

NPSTR FAR PASCAL LocalAllocPtr(WORD wFlags, WORD wNbytes);

BOOL FAR PASCAL LocalFreePtr(NPSTR npArg);

LPSTR FAR _cdecl _fstrichr(LPSTR lpSrc, int c);

LPSTR FAR _cdecl _fstrtrim(LPSTR lpSrc);

#ifdef WIN32

#define _hmemcpy(X,Y,Z) memcpy(X,Y,Z)
#define _hmemcmp(X,Y,Z) memcmp(X,Y,Z)
#define _hmemset(X,Y,Z) memset(X,Y,Z)
#define _hmemmove(X,Y,Z) memmove(X,Y,Z)

static __inline void _hmemswap(void *lp1, void *lp2, DWORD maxcount)
{
char *p1=(char *)lp1, *p2=(char *)lp2;
DWORD dw1;

  for(dw1=0; dw1 < maxcount; dw1++, p1++, p2++)
  {
    char c = *p1;
    *p1 = *p2;
    *p2 = c;
  }
}

#else // WIN32

void FAR _cdecl _hmemcpy(void HUGE *hpNewData, const void HUGE *hpOldData,
                         DWORD maxcount);

void FAR _cdecl _hmemmove(void HUGE *hpNewData, const void HUGE *hpOldData,
                          DWORD maxcount);

void FAR _cdecl _hmemset(void HUGE *hpData, WORD value, DWORD maxcount);

void FAR _cdecl _hmemswap(void HUGE *hpData1, void HUGE *hpData2,
                          DWORD maxcount);

int FAR _cdecl _hmemcmp(const void HUGE *hpData1, const void HUGE *hpData2,
                        DWORD maxcount);
#endif // WIN32

void FAR _cdecl _lqsort(char HUGE *base, DWORD num, DWORD width,
                       int (FAR _cdecl *compare)(LPSTR p1, LPSTR p2));

static int FAR PASCAL _lqsort_(LONG left, LONG right, SBPQSORTVARS bpqsv);

void NEAR _fastcall _lqsort2_(LONG left, LONG right, LONG FAR *lpI,
                              LONG FAR *lpJ, SBPQSORTVARS bpqsv);

void NEAR _fastcall _lqsort_middle_(LONG left, LONG right,
                                    SBPQSORTVARS bpqsv);

static int FAR _cdecl _lqsort_defcmp(LPSTR p1, LPSTR p2, SBPQSORTVARS bpqsv);


WORD FAR PASCAL GetPhysDriveFromLogical(WORD wDrive);


LPVOID FAR PASCAL GetDosCurDirEntry(WORD wDrive);
 /** This function gets the 'CURRENT DIRECTORY' structure entry for **/
 /** the logical drive specified by 'wDrive' and returns a pointer  **/
 /** to it.                                                         **/
 /**                                                                **/
 /** IMPORTANT:  THE CALLER MUST FREE THE POINTER'S SELECTOR WHEN   **/
 /**             ITS USE IS COMPLETE BY CALLING 'MyFreeSelector()'! **/

WORD FAR PASCAL GetDosMaxDrives(void);  // returns LOL's max # of drives


WORD FAR PASCAL MyAllocSelector(void);

BOOL FAR PASCAL MyChangeSelector(WORD wSelector, DWORD dwAddr, WORD wLimit);

void FAR PASCAL MyFreeSelector(WORD wSelector);

DWORD FAR PASCAL MyGetSelectorBase(WORD wSelector);
DWORD FAR PASCAL MyGetSelectorLimit(WORD wSelector);

BOOL FAR PASCAL GetDPB(WORD wDrive, LPDPB lpDPB);

BOOL FAR PASCAL MyGetOpenFileName(LPCSTR lpTitle, LPSTR lpBuf, BOOL bExec);


                     /*** EQUATION EVALUATION ***/

long FAR PASCAL MyValue(LPSTR lpText);         /* value of a string (term) */

LPSTR FAR PASCAL Calculate(LPSTR line);
                                   // returns a string representation of the
                                   // result obtained from equation in 'line'
                                   // caller *must* free return string!
BOOL FAR PASCAL CalcAssignVariable(LPSTR varexp, LPSTR lpValue);
                             // assigns value to variable, including arrays!



void FAR PASCAL GetSystemDateTime(LPSFTDATE lpDate, LPSFTTIME lpTime);
BOOL FAR PASCAL atodate(LPSTR string, LPSFTDATE d);
BOOL FAR PASCAL atotime(LPSTR string, LPSFTTIME t);
int  FAR PASCAL datecmp(LPSFTDATE d1, LPSFTDATE d2);
int  FAR PASCAL timecmp(LPSFTTIME t1, LPSFTTIME t2);
long FAR PASCAL days(LPSFTDATE d);   /* convert date to # days since 1/1/1900 */
void FAR PASCAL Date(long n, LPSFTDATE d);     /* convert 'n' days to a date! */
BOOL FAR PASCAL GetDayOfWeekDate(LPSTR lpDOW, LPSFTDATE lpDate);
             /* converts 'day of week' (Monday, etc.) to corresponding date */
             /* assumes the 'next' day of week that matches including today */


void FAR PASCAL WinIniChange(); // call this when 'WM_WININICHANGE' processed
LPSTR FAR PASCAL DaysStr(long dwDays);
LPSTR FAR PASCAL DateStr(LPSFTDATE lpDate);
LPSTR FAR PASCAL TimeStr(LPSFTTIME lpTime);


                      /*** 'TOOLHELP' CLONES ***/

BOOL FAR PASCAL MyMemManInfo(LPMEMMANINFO lpInfo);

BOOL  FAR PASCAL MyTaskFirst(LPTASKENTRY lpTask);
BOOL  FAR PASCAL MyTaskNext(LPTASKENTRY lpTask);
HTASK FAR PASCAL MyTaskFindHandle(LPTASKENTRY lpTask, HTASK hTask);



                     /*** 'TOOLHELP' HELPERS ***/

BOOL FAR PASCAL MyIsTask(HTASK hTask);

BOOL FAR PASCAL IsWINOLDAP(HTASK hTask);
HTASK FAR PASCAL GetTaskFromInst(HINSTANCE hInst);


                     /*** 'FORMAT' UTILITIES ***/

#ifndef WIN32
BOOL FAR PASCAL RealModeInt(WORD wIntNum, LPREALMODECALL lpIR);
#endif // WIN32

BOOL FAR PASCAL FormatDrive(UINT wDrive, UINT wTracks, UINT wHeads,
                            UINT wSectors);

BOOL FAR PASCAL QuickFormatDrive(UINT wDrive);

BOOL FAR PASCAL GetDriveParms(UINT wDrive, UINT FAR *lpwDriveType,
                              UINT FAR *lpwMaxCyl, UINT FAR *lpwMaxSector,
                              UINT FAR *lpwHeads, UINT FAR *lpwNumDrives);

void FAR PASCAL SubmitFormatRequest(UINT wDrive, UINT wTracks, UINT wHeads,
                                    UINT wSectors, UINT wSwitchFlag,
                                    LPSTR lpLabel);

#ifdef WIN32

#ifndef VWIN32_DIOC_GETVERSION

#define VWIN32_DIOC_GETVERSION DIOC_GETVERSION
#define  VWIN32_DIOC_DOS_IOCTL   1
#define  VWIN32_DIOC_DOS_INT25   2
#define  VWIN32_DIOC_DOS_INT26   3
#define  VWIN32_DIOC_DOS_INT13   4
#define  VWIN32_DIOC_CLOSEHANDLE DIOC_CLOSEHANDLE

typedef struct DIOCRegs {
   DWORD reg_EBX;
   DWORD reg_EDX;
   DWORD reg_ECX;
   DWORD reg_EAX;
   DWORD reg_EDI;
   DWORD reg_ESI;
   DWORD reg_Flags;
} DIOC_REGISTERS;

#endif // VWIN32_DIOC_GETVERSION

static __inline BOOL WINAPI ExecuteDOSInterrupt(HANDLE hVWIN32, DIOC_REGISTERS *pRegs)
{
DWORD cb;

   return(DeviceIoControl(hVWIN32, VWIN32_DIOC_DOS_IOCTL,
                          pRegs, sizeof(*pRegs), pRegs, sizeof(*pRegs),
                          &cb, NULL));
}

BOOL WINAPI PerformGenericIOCTL(HANDLE hVWIN32, UINT wDrive,
                                UINT wFunction, LPVOID lpParmBlock);
void FlushDiskSystem(HANDLE hVWIN32, UINT wDrive);
BOOL PASCAL LockDrive(HANDLE hVWIN32, UINT wDrive);
void PASCAL UnlockDrive(HANDLE hVWIN32, UINT wDrive);

static __inline HANDLE OpenVWIN32(void)
{
   return(CreateFile("\\\\.\\vwin32", 0, 0, NULL,
                     OPEN_EXISTING, FILE_FLAG_DELETE_ON_CLOSE, 0));
}


#endif // WIN32

                          /*** DDE FUNCTIONS ***/

HDDEDATA EXPENTRY MyDdeCallBack(WORD wType, WORD wFmt, HCONV hConv,
                                HSZ hsz1, HSZ hsz2, HDDEDATA hData,
                                DWORD dwData1, DWORD dwData2);


                /*** SUPPORT FUNCTIONS FOR COMMANDS ***/

BOOL FAR PASCAL CMDRunProgram(LPSTR lpCmd, LPSTR lpArgs, int nCmdShow);

int FAR PASCAL FindProgram(LPSTR lpName, LPOFSTRUCT lpOfstr);

int NEAR FASTCALL FindProgramExt(NPSTR path, NPSTR drive, NPSTR dir,
                                 NPSTR name, NPSTR ext, LPOFSTRUCT lpOfstr);

BOOL FAR PASCAL IsProgramFile(LPSTR lpName, LPOFSTRUCT lpOfstr,
                              int FAR *lpType);
   /* return value non-zero if error or file not found.  'lpType' points */
   /* to 'return'ed type:   0 = non-EXE format;  >0 = WINDOWS EXE;       */
   /*                      <0 = NON-WINDOWS EXE;                         */
   /*                   (-1 is DOS or OTHER EXE; -2 is OS2 EXE )         */
   /*                   (-3 is 32-bit OS/2; -4 is 32 bit ???)            */


BOOL FAR PASCAL IsCONDevice(HFILE hFile);

WORD FAR PASCAL RunProgramUsingCreateProcess(LPSTR lpProgName,
                                             LPDOSRUNPARMBLOCK lpParmBlock,
                                             LPSTR lpStdIn, LPSTR lpStdOut);

DWORD FAR PASCAL GetThreadIDFromHTask(HTASK hTask);      // 32-bit "helpers"
DWORD FAR PASCAL GetProcessIDFromHTask(HTASK hTask);
DWORD FAR PASCAL DoOpenProcess(DWORD dwProcessID);
DWORD FAR PASCAL DoGetExitCodeProcess(DWORD dwhProcess);
DWORD FAR PASCAL DoCloseHandle(DWORD dwHandle);


WORD FAR PASCAL DosRunProgram(LPSTR lpProgName, LPDOSRUNPARMBLOCK lpParmBlock);


WORD FAR PASCAL ExecuteBatchFile(LPSTR lpProgName, LPDOSRUNPARMBLOCK lpParmBlock);
  // PASS 'NULL' for 'lpProgName' and 'lpParmBlock' points to the memory
  // batch program to run memory batch; else, same parms as 'DosRunProgram()'


LONG FAR PASCAL CalcUserFunctionThread(HANDLE hCaller, HANDLE hParms, UINT wFlags);
  // this thread proc is used by 'wcalc.c' to invoke a 'CALL' to a
  // user-defined function.  It's located in 'wmthread.c' so that it can
  // make use of static variables that aren't globally linked.


void FAR PASCAL ConvertAsterisks(LPSTR lpBuffer);

void FAR PASCAL DosQualifyPath(LPSTR lpDest, LPCSTR lpSrc);
void FAR PASCAL DosQualifyPath0(LPSTR lpDest, LPCSTR lpSrc);


BOOL FAR PASCAL FileExists(LPCSTR lpcFileName);

BOOL FAR PASCAL IsValidPortName(LPCSTR szPortName);
HDC FAR PASCAL CreatePrinterDCFromPort(LPCSTR szPortName);


// Special 'LFN' FindFirst/FindNext and other functions

WORD FAR PASCAL MyFindFirst(LPCSTR lpName, WORD attrib,
                            struct _find_t FAR *lpFI);
WORD FAR PASCAL MyFindNext(struct _find_t FAR *lpFI);
void FAR PASCAL MyFindClose(struct _find_t FAR *lpFI);

LPWIN32_FIND_DATA FAR PASCAL GetWin32FindData(struct _find_t FAR *lpFI);
       // This function gets the LFN information obtained by calling one
       // of the 'MyFindxxx' functions (above).



int FAR PASCAL MyChDir(LPSTR lpDir);    // does a special 'chdir()' function



BOOL WINAPI MyCreateFile(LPCSTR szPath, UINT uiModeFlags, UINT uiAttrib,
                         UINT uiActionFlags, HFILE FAR *lphFile);

HFILE WINAPI MyOpenFile(LPCSTR szPath, LPOFSTRUCT lpOfstr, UINT uiFlags);

BOOL WINAPI GetShortName(LPCSTR szPath, LPSTR lpDest);

BOOL WINAPI GetLongName(LPCSTR szPath, LPSTR lpDest);

BOOL WINAPI GetFullName(LPCSTR szPath, LPSTR lpDest);


int FAR PASCAL MyRename(LPSTR lpOldName, LPSTR lpNewName);
int FAR PASCAL MyUnlink(LPSTR lpPath);

unsigned FAR PASCAL MyGetFileAttr(LPSTR lpPath, unsigned FAR *lpwAttr);
unsigned FAR PASCAL MySetFileAttr(LPSTR lpPath, unsigned wAttr);

int FAR PASCAL MyGetFTime(HFILE hFile, unsigned FAR *lpDate, unsigned FAR *lpTime);
int FAR PASCAL MySetFTime(HFILE hFile, unsigned date, unsigned time);

int FAR PASCAL My_Chdir(LPSTR lpPath);
int FAR PASCAL My_Rmdir(LPSTR lpPath);
int FAR PASCAL My_Mkdir(LPSTR lpPath);

LPSTR FAR PASCAL MyGetCurDir(WORD wDrive, LPSTR lpBuffer);



// BATCH FILE PROCESSING

LPSTR FAR PASCAL GetAnotherBatchLine(LPBATCH_INFO lpBatchInfo);

void FAR PASCAL AddBatchLabel(LPBATCH_INFO lpBatchInfo, LPSTR lpLabel);

BOOL FAR PASCAL GotoBatchLabel(LPBATCH_INFO lpBatchInfo, LPSTR lpLabel);

BOOL FAR PASCAL SetBatchFilePosition(LPBATCH_INFO lpBatchInfo, long lFilePos);

void FAR PASCAL TerminateCurrentBatchFile();
     /* this function 'terminates' the current batch file, restoring */
     /* control to the 'calling' batch file (as req'd).  Use this to */
     /* ensure that *ALL* memory blocks are properly freed!          */


BOOL FAR PASCAL CMDRunErrorMessage(WORD wRval);

BOOL FAR PASCAL CMDErrorCheck(BOOL rval);

LPSTR FAR PASCAL GetTaskAndModuleFromPSP(WORD wMachID, WORD wPSPSeg,
                                         HTASK FAR *lphTask);

BOOL FAR PASCAL CMDOdbcDisconnectAll(void);  // call this before exiting!


// these next 3 functions use the conversation ID to return important info

LPCSTR FAR PASCAL CMDOdbcGetConnectString(LPCSTR lpcConvID);
HDBC FAR PASCAL CMDOdbcGetDbc(LPCSTR lpcConvID);
HSTMT FAR PASCAL CMDOdbcGetStmt(LPCSTR lpcConvID);


// these next functions return a 'GlobalAlloc'ed string containing info
// based on the ODBC conversation ID (as a string), when applicable

LPSTR FAR PASCAL CMDOdbcGetData(LPCSTR lpcConvID, WORD wColumn);
LPSTR FAR PASCAL CMDOdbcGetColumnIndex(LPCSTR lpcConvID, LPCSTR lpName);
LPSTR FAR PASCAL CMDOdbcGetColumnInfo(LPCSTR lpcConvID, WORD wColumnIndex);
LPSTR FAR PASCAL CMDOdbcGetCursorName(LPCSTR lpcConvID);
LPSTR FAR PASCAL CMDOdbcGetInfo(LPCSTR lpcConvID, WORD wIndex);
LPSTR FAR PASCAL CMDOdbcGetTables(LPCSTR lpcConvID);
LPSTR FAR PASCAL CMDOdbcGetDrivers(void);




            /* FUNCTION PROTOTYPES FOR 'DBLSPACE' FUNCTIONS */

BOOL FAR PASCAL IsDoubleSpaceDrive(BYTE drive, BOOL FAR *pfSwapped, BYTE FAR *pdrHost,
                                   int FAR *pseq);

BOOL FAR PASCAL DblspaceCommand(LPSTR lpCommand);

long FAR PASCAL DblspaceGetCompressedFileSize(LPSTR lpName);

LPSTR FAR PASCAL DblspaceCompressionRatio(long lFileSize, long lCVFSize);
   // returns pointer to a string representing the compression ratio


BOOL FAR PASCAL IsDriveInUse(WORD wDrive);  // 1-based drive # ('A'==1)
                                            // returns TRUE if 'IN USE'


            /*** ENTRY POINT FOR 'WREXX.DLL' WinREXX API ***/

BOOL FAR PASCAL RexxRun(LPSTR lpArgs, BOOL IsCommand);
 /* IsCommand==TRUE when 'lpArgs' contains a command; FALSE if it's a file */

void FAR PASCAL RexxUnattach();  // this function 'un-attaches' my task
                                 // from the WREXX.DLL API.  Call regardless.

                 /**----------------------------------**/
                 /** FUNCTIONS WHICH PERFORM COMMANDS **/
                 /**----------------------------------**/

BOOL FAR PASCAL CMDNoRun   (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDBad     (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDNotYet  (LPSTR lpCmd, LPSTR lpArgs);

BOOL FAR PASCAL CMDAttrib  (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDBreak   (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDCalc    (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDCall    (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDChdir   (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDCloseTask(LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDCls     (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDCommand (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDComp    (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDContinue(LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDCopy    (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDDate    (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDDblspace(LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDDde     (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDDebug   (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDDefine  (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDDel     (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDDeltree (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDDir     (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDEcho    (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDEdit    (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDElse    (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDEnd     (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDEndif   (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDExit    (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDExpand  (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDFc      (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDFind    (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDFor     (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDFormat  (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDGoto    (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDHelp    (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDIf      (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDInput   (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDJoin    (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDKilltask(LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDLabel   (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDLet     (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDList    (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDListOpen(LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDLoadhigh(LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDLoop    (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDMax     (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDMkdir   (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDMem     (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDMin     (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDMode    (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDMore    (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDMove    (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDNet     (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDNext    (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDNuketask(LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDOdbc    (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDPath    (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDPause   (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDPlaySound(LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDPrint   (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDPrompt  (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDRem     (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDRemove  (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDRename  (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDRepeat  (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDReplace (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDReturn  (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDRexx    (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDRmdir   (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDSet     (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDShare   (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDShift   (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDSort    (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDSql     (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDStart   (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDSubst   (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDSys     (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDTasklist(LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDTime    (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDTree    (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDTruename(LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDType    (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDUndelete(LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDUntil   (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDVer     (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDVerify  (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDVol     (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDWait    (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDWend    (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDWhile   (LPSTR lpCmd, LPSTR lpArgs);
BOOL FAR PASCAL CMDXcopy   (LPSTR lpCmd, LPSTR lpArgs);




                       /**************************/
                       /*** EXTERNAL VARIABLES ***/
                       /**************************/

 // this next 'define' is specific for COMMAND strings & function addresses


#ifdef WIN32
#define PCMDB
#else  // WIN32
#pragma code_seg("PROCESS_COMMAND_TEXT","CODE")
#pragma code_seg()
#define PCMDB __based(__segname("PROCESS_COMMAND_TEXT"))
#endif // WIN32

#ifndef STATIC_VARS

extern HINSTANCE hInst;                /* handle for this instance */
extern HWND      hMainWnd;                    /* handle for main window */
extern HWND      hFrameWnd;            // handle for FRAME around 'hMainWnd'
extern HACCEL    hAccel;
extern BOOL      IsShell;       /* TRUE if app is the SHELL; FALSE otherwise */

extern BOOL      IsChicago;     /* TRUE if WIN 3.80 or greater; FALSE else   */
extern BOOL      IsNT;          /* TRUE if 32-bit WOW functions available */

extern char FAR *lpScreen;

extern char (__based(lpScreen) *screen)[SCREEN_COLS]; // array containing screen buffer
extern char (__based(lpScreen) *attrib)[SCREEN_COLS]; // array containing attrib bytes

extern int iScreenLines;        // # of lines in screen buffer
extern int LineScrollCount;     // # of lines to scroll when painting screen
                                // and direction to scroll them (+up,-down)

extern LOCALHANDLE lhCmd[COMMAND_HIST_SIZE]; /* record of last 32 commands */

extern volatile int maxlines, curline, curcol;
                                        /* max line and current line/column */
                                        /* on display (applies to KB input) */
extern volatile int cur_attr;     /* current attribute value - WHITE on BLACK */
extern volatile int start_line, start_col;   /* starting line/column for input */
extern volatile int end_line, end_col;   /* ending line/col for input (so far) */

extern volatile BOOL ctrl_break;       /* the 'ctrl-break is active' flag! */
extern volatile BOOL stall_output;  /* the 'stall output' flag! (ctrl s,q) */

extern struct DOSERROR last_error;         /* ext error information for last error */

extern volatile LPSTR lpMyEnv;      /* pointer to my (current) env. */
extern volatile WORD  orig_env;
extern volatile WORD env_size;

extern volatile LPSTR lpCmdHist;        /* points to command history buffer */
extern volatile LPSTR lpCmdHistIndex;   /* 'DOSKEY' pointer for arrows keys */
extern volatile LPSTR lpLastCmd;              /* points to previous command */
extern volatile LPSTR lpCurCmd;         /* points to current command!! This */
                                           /* is NULL if no command waiting */

extern volatile LPCOPY_INFO lpCopyInfo;/* points to 'head' struc */
extern volatile BOOL copying;           /* 'TRUE' if copying in background */

extern volatile BOOL formatting;     /* 'TRUE' if formatting in background */

extern volatile BOOL pausing_flag;    /* 'TRUE' if PAUSE command in effect */

extern volatile BOOL waiting_flag;     /* 'TRUE' if WAIT command in effect */
                                     /* this affects the main message loop! */
extern volatile BOOL waiting_quiet;   // a 'quiet' flag for waiting
                                      // if TRUE, no message when done


extern volatile SFTDATE WaitForDay;     /* flags which cause a wait until a */
extern volatile SFTTIME WaitForTime;    /* particular date/time.            */
extern volatile int  WaitForTimeFlag;


extern volatile BOOL copy_complete_flag;/* 'TRUE' when last copy completes */

extern volatile BOOL GettingUserInput;   /* 'TRUE' if getting 'user input' */

extern volatile BOOL BatchMode;                      /* batch mode switches */
extern volatile BOOL BatchEcho;
extern volatile BOOL IsCall;   // TRUE when 'CALL' is used...
                               // remains TRUE if waiting for program to end
extern volatile BOOL IsStart;  // TRUE if 'START' invoked the program - this
                               // causes all BATCH and REXX files to run in
                               // a separate instance of SFTSHELL.
extern volatile BOOL IsFor;    /* similar to 'IsCall' but for 'FOR' command */
extern volatile BOOL IsRexx;   /* about the same thing as 'IsFor' */

extern volatile DWORD dwLastProcessID;  // assigned by SFTSH32X (when used)

extern volatile WORD PipingFlag;           // increment for each 'pipe' level

extern volatile LPBATCH_INFO lpBatchInfo;         /* batch file information */
extern volatile LPSMARTFILEPARM lpSmartFileParm;  /* file information */

extern volatile DWORD TotalCopiesSubmitted;     /* # of copies added to QUEUE that */
                                                /* have not yet been reported. */

extern volatile BOOL BlockCopyResults;      /* TRUE to prevent printing the */
                                          /* # of files added to copy queue */

extern volatile int WindowState;     /* 0 for normal, 1 for max, -1 for min */

extern volatile BOOL InvalidateFlag;  /* TRUE when entire client is invalid */

extern volatile TEXTMETRIC tmMain;    /* updated 1st time window paints! */
extern volatile BOOL TMFlag;          /* FALSE until tmMain is updated.  */

extern volatile HANDLE hCopyThread;   /* handles for various threads */
extern volatile HANDLE hFormatThread;
extern volatile HANDLE hBatchThread;
extern volatile HANDLE hWaitThread;
extern volatile HANDLE hCommandThread;
extern volatile HANDLE hFlashThread;
extern volatile HANDLE hMemoryBatchFileThread;

extern WORD wIGROUP;                            /* 'IGROUP' segment for KERNEL */
extern WIN_VER uVersion;               /* the current Windows and DOS version!! */

extern char work_buf[STD_BUFFER_SIZE * 4], UserInputBuffer[STD_BUFFER_SIZE];

extern char TypeAhead[STD_BUFFER_SIZE];
extern WORD TypeAheadHead, TypeAheadTail;

extern struct find_t ff;

extern BOOL insert_mode;
extern BOOL own_caret;
extern int  caret_hidden;
extern HBITMAP hCaret1, hCaret2;

extern char cmd_buf[STD_BUFFER_SIZE];
extern int cmd_buf_len;
extern BOOL ctrl_p;

extern HFILE redirect_output, redirect_input;
extern HDC   redirect_output_DC;

#define HFILE_USE_DC ((HFILE)0x8000) /* assign THIS to 'redirect_output' for DC */



extern DWORD idDDEInst; /* the ID for the DDEML instance */
extern LPSTR lpDDEInfo; /* the 'info' block for DDE conversations */

extern LPSTR lpHotLinkItems; // this contains a list of 'hot-linked' items
                             // which allows excellent optimization!

extern char pq[];
extern char *pOpenMode[8];
extern char *pShareMode[8];


//extern char _near *cmd_array[];
extern const char PCMDB * const PCMDB cmd_array[];

extern WORD wNcmd_array;

//extern CMD_FN *fn_array[];
extern CMD_FN * const PCMDB fn_array[];

extern char *pDdeOption[];
extern WORD wNDdeOption;

extern char *pOdbcOption[];
extern WORD wNOdbcOption;

extern char pDDESERVERLIST[];      /* env variable for 'SERVER' list */
extern char pDDESERVERTOPICLIST[]; /* env variable for 'SERVER|TOPIC' list */
extern char pDEFSERVERNAME[];      /* the 'default' DDE SERVER name */
extern char *pPROGRAMNAME;         /* the name of the program (SFTSHELL) */
extern char pDDERESULT[];          /* env variable for 'DDE' results */

extern char pCOMMA[];              /* points to a comma followed by a NULL */
extern char pOK[];                 /* 'OK' - result in 'DDE_RESULT' */
extern char pERROR[];              /* 'ERROR' - result in 'DDE_RESULT' */

extern char pSYSTEM[];             /* the 'SYSTEM' topic */

extern char pTASK_ID[];            /* 'TASK_ID' variable name */


extern const char pCRLF[];
extern const char pCRLFLF[];
extern const char pQCRLF[];
extern const char pQCRLFLF[];
extern const char pBACKSLASH[];

extern const char pNOTENOUGHMEMORY[];/* the 'Not enough memory' error message */
                                     /* this message MUST be static!!         */

extern const char pMEMORYLOW[];      /* 'memory low' warning */

extern const char pNOMATCHINGFILES[];

extern const char pCOMSPEC[];
extern const char pCOMMANDCOM[];
extern const char pIOSYS[];
extern const char pMSDOSSYS[];

extern const char pHELPFILE[];

extern const char pPIF[];
extern const char pREX[];
extern const char pBAT[];
extern const char pCOM[];
extern const char pEXE[];


extern const char pWILDCARD[];
extern const char pVOLUMEINDRIVE[];
extern const char pHASNOLABEL[];

extern const char pON[];
extern const char pOFF[];
extern const char pILLEGALARGUMENT[];
extern const char pILLEGALSWITCH[];

extern const char pEXTRANEOUSARGSIGNORED[];

extern const char pSYNTAXERROR[];

extern const char pERRORREADINGDRIVE[];

extern const char pINVALIDPATH[];
extern const char pINVALIDDRIVE[];

extern const char pNoSysMessage[];

extern const char pCTRLBREAKMESSAGE[];

extern const char pCTRLC[];

extern const char pWINCMDERROR[];









                /*** EXTERNALLY DEFINED PROC ADDRESSES ***/

extern HMODULE hToolHelp;
extern HMODULE hCommDlg;
extern HMODULE hMMSystem;
extern HMODULE hShell;
extern HMODULE hKernel;
extern HMODULE hUser;
extern HMODULE hDdeml;
extern HMODULE hODBC;
extern HMODULE hRexxAPI;  /* module handle for WREXX.DLL (REXX API) */
extern HMODULE hWDBUTIL;  /* module handle for WDBUTIL.DLL */


#ifndef WIN32

extern BOOL (FAR PASCAL *lpGetOpenFileName)(OPENFILENAME FAR *lpofn);
extern BOOL (FAR PASCAL *lpGetSaveFileName)(OPENFILENAME FAR *lpofn);
extern BOOL (FAR PASCAL *lpChooseFont)(CHOOSEFONT FAR *lpcf);
extern DWORD (FAR PASCAL *lpCommDlgExtendedError)(void);

extern BOOL (FAR PASCAL *lpGlobalFirst)(LPGLOBALENTRY lpGlobal, WORD wFlags);
extern BOOL (FAR PASCAL *lpGlobalNext)(LPGLOBALENTRY lpGlobal, WORD wFlags);
extern BOOL (FAR PASCAL *lpMemManInfo)(LPMEMMANINFO lpInfo);
extern BOOL (FAR PASCAL *lpModuleFirst)(LPMODULEENTRY lpModule);
extern BOOL (FAR PASCAL *lpModuleNext)(LPMODULEENTRY lpModule);
extern HTASK (FAR PASCAL *lpTaskFindHandle)(LPTASKENTRY lpTask, HTASK hTask);
extern HMODULE (FAR PASCAL *lpModuleFindHandle)(LPMODULEENTRY lpModule,
                                                HMODULE hModule);
extern BOOL (FAR PASCAL *lpNotifyRegister)(HTASK, LPFNNOTIFYCALLBACK, WORD);
extern BOOL (FAR PASCAL *lpNotifyUnRegister)(HTASK);

extern BOOL (FAR PASCAL *lpIsTask)(HTASK hTask);
extern BOOL (FAR PASCAL *lpTaskFirst)(LPTASKENTRY lpTask);
extern BOOL (FAR PASCAL *lpTaskNext)(LPTASKENTRY lpTask);

extern DWORD (FAR PASCAL *lpGlobalMasterHandle)(void);
extern DWORD (FAR PASCAL *lpGlobalHandleNoRIP)(WORD wSeg);

extern BOOL (WINAPI *lpsndPlaySound)(LPCSTR lpszSoundName, WORD wFlags);




// WOW Generic Thunk procs (WOWNT16.H)

extern DWORD hKernel32;   // 32-bit module (Generic Thunk)
extern DWORD hWOW32;      // 32-bit module (Generic Thunk)
extern DWORD hMPR;        // 32-bit module (Generic Thunk)
extern DWORD hSFTSH32T;   // 32-bit module (Generic Thunk)


extern DWORD (FAR PASCAL *lpGetVDMPointer32W)(LPVOID vp, UINT fMode);
                // NOTE:  'fMode' is 1 for SELECTOR, 0 for real mode segment

extern DWORD (FAR PASCAL *lpLoadLibraryEx32W)(LPCSTR lpszLibFile,
                                              DWORD hFile, DWORD dwFlags);
                // NOTE:  'hFile' must be NULL (reserved parameter)
                //        'dwFlags' should contain zero (no special handling)

extern DWORD (FAR PASCAL *lpGetProcAddress32W)(DWORD hModule,
                                               LPCSTR lpszProc);

extern DWORD (FAR PASCAL *lpFreeLibrary32W)(DWORD hLibModule);


#define CPEX_DEST_STDCALL   0x00000000L  /* 'or' with 'dwCount' parm */
#define CPEX_DEST_CDECL     0x80000000L  /* 'or' with 'dwCount' parm */

extern DWORD (FAR __cdecl *lpCallProcEx32W)(DWORD dwCount, DWORD lpCvt,
                                            DWORD lpProc, ... );


// 16-bit WNET stuff (from USER.EXE)

extern UINT (FAR PASCAL *lpWNetAddConnection)(LPSTR, LPSTR, LPSTR);
extern UINT (FAR PASCAL *lpWNetCancelConnection)(LPSTR, BOOL);
extern UINT (FAR PASCAL *lpWNetGetConnection)(LPSTR, LPSTR, UINT FAR *);

// flags for 32-bit WNET functions

#define RESOURCE_CONNECTED      0x00000001
#define RESOURCE_GLOBALNET      0x00000002
#define RESOURCE_REMEMBERED     0x00000003
#define RESOURCE_RECENT         0x00000004
#define RESOURCE_CONTEXT        0x00000005

#define RESOURCETYPE_ANY        0x00000000
#define RESOURCETYPE_DISK       0x00000001
#define RESOURCETYPE_PRINT      0x00000002
#define RESOURCETYPE_RESERVED   0x00000008
#define RESOURCETYPE_UNKNOWN    0xFFFFFFFF

#define RESOURCEUSAGE_CONNECTABLE   0x00000001
#define RESOURCEUSAGE_CONTAINER     0x00000002
#define RESOURCEUSAGE_NOLOCALDEVICE 0x00000004
#define RESOURCEUSAGE_SIBLING       0x00000008
#define RESOURCEUSAGE_ALL           (RESOURCEUSAGE_CONNECTABLE | RESOURCEUSAGE_CONTAINER)
#define RESOURCEUSAGE_RESERVED      0x80000000

#define RESOURCEDISPLAYTYPE_GENERIC        0x00000000
#define RESOURCEDISPLAYTYPE_DOMAIN         0x00000001
#define RESOURCEDISPLAYTYPE_SERVER         0x00000002
#define RESOURCEDISPLAYTYPE_SHARE          0x00000003
#define RESOURCEDISPLAYTYPE_FILE           0x00000004
#define RESOURCEDISPLAYTYPE_GROUP          0x00000005
#define RESOURCEDISPLAYTYPE_NETWORK        0x00000006
#define RESOURCEDISPLAYTYPE_ROOT           0x00000007
#define RESOURCEDISPLAYTYPE_SHAREADMIN     0x00000008
#define RESOURCEDISPLAYTYPE_DIRECTORY      0x00000009
#define RESOURCEDISPLAYTYPE_TREE           0x0000000A


typedef struct  _NETRESOURCEA {
    DWORD    dwScope;
    DWORD    dwType;
    DWORD    dwDisplayType;
    DWORD    dwUsage;
    LPSTR    lpLocalName;
    LPSTR    lpRemoteName;
    LPSTR    lpComment ;
    LPSTR    lpProvider;
} NETRESOURCE, FAR *LPNETRESOURCE;




// REGISTRY

extern LONG (FAR PASCAL *lpRegCloseKey)(HKEY);
extern LONG (FAR PASCAL *lpRegCreateKey)(HKEY, LPCSTR, HKEY FAR *);
extern LONG (FAR PASCAL *lpRegDeleteKey)(HKEY, LPCSTR);
extern LONG (FAR PASCAL *lpRegEnumKey)(HKEY, DWORD, LPSTR, DWORD);
extern LONG (FAR PASCAL *lpRegOpenKey)(HKEY, LPCSTR, HKEY FAR *);
extern LONG (FAR PASCAL *lpRegQueryValue)(HKEY, LPSTR, LPSTR, LONG FAR *);
extern LONG (FAR PASCAL *lpRegSetValue)(HKEY, LPCSTR, DWORD, LPCSTR, DWORD);

extern void (FAR PASCAL *lpDragAcceptFiles)(HWND, BOOL);
extern void (FAR PASCAL *lpDragFinish)(HANDLE);
extern WORD (FAR PASCAL *lpDragQueryFile)(HANDLE, WORD, LPSTR, WORD);
extern HINSTANCE (FAR PASCAL *lpShellExecute)(HWND, LPCSTR, LPCSTR, LPCSTR,
                                              LPCSTR, int);


                          /** DDEML STUFF **/

extern BOOL      (WINAPI *lpDdeAbandonTransaction)(DWORD, HCONV, DWORD);
extern BYTE      (WINAPI *lpDdeAccessData)(HDDEDATA, DWORD FAR *);
extern HDDEDATA  (WINAPI *lpDdeAddData)(HDDEDATA, void FAR *, DWORD, DWORD);
extern HDDEDATA  (WINAPI *lpDdeClientTransaction)(void FAR *, DWORD, HCONV,
                                         HSZ, UINT, UINT, DWORD, DWORD FAR *);
extern int       (WINAPI *lpDdeCmpStringHandles)(HSZ, HSZ);
extern HCONV     (WINAPI *lpDdeConnect)(DWORD, HSZ, HSZ, CONVCONTEXT FAR *);
extern HCONVLIST (WINAPI *lpDdeConnectList)(DWORD, HSZ, HSZ, HCONVLIST,
                                         CONVCONTEXT);
extern HDDEDATA  (WINAPI *lpDdeCreateDataHandle)(DWORD, void FAR *, DWORD,
                                              DWORD, HSZ, UINT, UINT);
extern HSZ       (WINAPI *lpDdeCreateStringHandle)(DWORD, LPCSTR, int);
extern BOOL      (WINAPI *lpDdeDisconnect)(HCONV);
extern BOOL      (WINAPI *lpDdeDisconnectList)(HCONVLIST);
extern BOOL      (WINAPI *lpDdeEnableCallback)(DWORD, HCONV, UINT);
extern BOOL      (WINAPI *lpDdeFreeDataHandle)(HDDEDATA);
extern BOOL      (WINAPI *lpDdeFreeStringHandle)(DWORD, HSZ);
extern DWORD     (WINAPI *lpDdeGetData)(HDDEDATA, void FAR *, DWORD, DWORD);
extern UINT      (WINAPI *lpDdeGetLastError)(DWORD);
extern UINT      (WINAPI *lpDdeInitialize)(DWORD FAR *, PFNCALLBACK, DWORD,
                                        DWORD);
extern BOOL      (WINAPI *lpDdeKeepStringHandle)(DWORD, HSZ);
extern HDDEDATA  (WINAPI *lpDdeNameService)(DWORD, HSZ, HSZ, UINT);
extern BOOL      (WINAPI *lpDdePostAdvise)(DWORD, HSZ, HSZ);
extern UINT      (WINAPI *lpDdeQueryConvInfo)(HCONV, DWORD, CONVINFO FAR *);
extern HCONV     (WINAPI *lpDdeQueryNextServer)(HCONVLIST, HCONV);
extern DWORD     (WINAPI *lpDdeQueryString)(DWORD, HSZ, LPSTR, DWORD, int);
extern HCONV     (WINAPI *lpDdeReconnect)(HCONV);
extern BOOL      (WINAPI *lpDdeSetUserHandle)(HCONV, DWORD, DWORD);
extern BOOL      (WINAPI *lpDdeUnaccessData)(HDDEDATA);
extern BOOL      (WINAPI *lpDdeUninitialize)(DWORD);

extern HDDEDATA  (CALLBACK *lpDdeCallBack)(UINT,UINT,HCONV,HSZ,HSZ,
                                           HDDEDATA,DWORD,DWORD);


#endif // WIN32


                           /** ODBC STUFF **/

// CORE

extern RETCODE (SQL_API *lpSQLAllocConnect)(HENV,HDBC FAR *);
extern RETCODE (SQL_API *lpSQLAllocEnv)(HENV FAR *);
extern RETCODE (SQL_API *lpSQLAllocStmt)(HDBC,HSTMT FAR *);
extern RETCODE (SQL_API *lpSQLBindCol)(HSTMT,UWORD,SWORD,PTR,SDWORD,SDWORD FAR *);
extern RETCODE (SQL_API *lpSQLCancel)(HSTMT);
extern RETCODE (SQL_API *lpSQLColAttributes)(HSTMT,UWORD,UWORD,PTR,SWORD,
                                             SWORD FAR *,SDWORD FAR *);
extern RETCODE (SQL_API *lpSQLConnect)(HDBC,UCHAR FAR *,SWORD,UCHAR FAR *,SWORD,
                                UCHAR FAR *,SWORD);
extern RETCODE (SQL_API *lpSQLDescribeCol)(HSTMT,UWORD,UCHAR FAR *,SWORD,
                                           SWORD FAR *,SWORD FAR *,UDWORD FAR *,
                                           SWORD FAR *,SWORD FAR *);
extern RETCODE (SQL_API *lpSQLDisconnect)(HDBC);
extern RETCODE (SQL_API *lpSQLError)(HENV,HDBC,HSTMT,UCHAR FAR *,SDWORD FAR *,
                                     UCHAR FAR *,SWORD,SWORD FAR *);
extern RETCODE (SQL_API *lpSQLExecDirect)(HSTMT,UCHAR FAR *,SDWORD);
extern RETCODE (SQL_API *lpSQLExecute)(HSTMT);
extern RETCODE (SQL_API *lpSQLFetch)(HSTMT);
extern RETCODE (SQL_API *lpSQLFreeConnect)(HDBC);
extern RETCODE (SQL_API *lpSQLFreeEnv)(HENV);
extern RETCODE (SQL_API *lpSQLFreeStmt)(HSTMT,UWORD);
extern RETCODE (SQL_API *lpSQLGetCursorName)(HSTMT,UCHAR FAR *,SWORD,SWORD FAR *);
extern RETCODE (SQL_API *lpSQLNumResultCols)(HSTMT,SWORD FAR *);
extern RETCODE (SQL_API *lpSQLPrepare)(HSTMT,UCHAR FAR *,SDWORD);
extern RETCODE (SQL_API *lpSQLRowCount)(HSTMT,SDWORD FAR *);
extern RETCODE (SQL_API *lpSQLSetCursorName)(HSTMT,UCHAR FAR *,SWORD);
extern RETCODE (SQL_API *lpSQLTransact)(HENV,HDBC,UWORD);


// EXTENSIONS

extern RETCODE (SQL_API *lpSQLColumns)(HSTMT,UCHAR FAR *,SWORD,UCHAR FAR *,SWORD,
                                       UCHAR FAR *,SWORD,UCHAR FAR *,SWORD);
extern RETCODE (SQL_API *lpSQLDriverConnect)(HDBC,HWND,UCHAR FAR *,SWORD,
                                             UCHAR FAR *,SWORD,SWORD FAR *,UWORD);
extern RETCODE (SQL_API *lpSQLGetConnectOption)(HDBC,UWORD,PTR);
extern RETCODE (SQL_API *lpSQLGetData)(HSTMT,UWORD,SWORD,PTR,SDWORD,SDWORD FAR *);
extern RETCODE (SQL_API *lpSQLGetFunctions)(HDBC,UWORD,UWORD FAR *);
extern RETCODE (SQL_API *lpSQLGetInfo)(HDBC,UWORD,PTR,SWORD,SWORD FAR *);
extern RETCODE (SQL_API *lpSQLGetStmtOption)(HSTMT,UWORD,PTR);
extern RETCODE (SQL_API *lpSQLGetTypeInfo)(HSTMT,SWORD);
extern RETCODE (SQL_API *lpSQLParamData)(HSTMT,PTR FAR *);
extern RETCODE (SQL_API *lpSQLPutData)(HSTMT,PTR,SDWORD);
extern RETCODE (SQL_API *lpSQLSetConnectOption)(HDBC,UWORD,UDWORD);
extern RETCODE (SQL_API *lpSQLSetStmtOption)(HSTMT,UWORD,UDWORD);
extern RETCODE (SQL_API *lpSQLSpecialColumns)(HSTMT,UWORD,UCHAR FAR *,SWORD,
                                              UCHAR FAR *,SWORD,UCHAR FAR *,
                                              SWORD,UWORD,UWORD);
extern RETCODE (SQL_API *lpSQLStatistics)(HSTMT,UCHAR FAR *,SWORD,UCHAR FAR *,
                                          SWORD,UCHAR FAR *,SWORD,UWORD,UWORD);
extern RETCODE (SQL_API *lpSQLTables)(HSTMT,UCHAR FAR *,SWORD,UCHAR FAR *,SWORD,
                                      UCHAR FAR *,SWORD,UCHAR FAR *,SWORD);
extern RETCODE (SQL_API *lpSQLBrowseConnect)(HDBC,UCHAR FAR *,SWORD,UCHAR FAR *,
                                             SWORD,SWORD FAR *);
extern RETCODE (SQL_API *lpSQLColumnPrivileges)(HSTMT,UCHAR FAR *,SWORD,
                                                UCHAR FAR *,SWORD,UCHAR FAR *,
                                                SWORD,UCHAR FAR *,SWORD);
extern RETCODE (SQL_API *lpSQLDataSources)(HENV,UWORD,UCHAR FAR *,SWORD,
                                           SWORD FAR *,UCHAR FAR *,SWORD,
                                           SWORD FAR *);
extern RETCODE (SQL_API *lpSQLDescribeParam)(HSTMT,UWORD,SWORD FAR *,UDWORD FAR *,
                                             SWORD FAR *,SWORD FAR *);
extern RETCODE (SQL_API *lpSQLExtendedFetch)(HSTMT,UWORD,SDWORD,UDWORD FAR *,
                                             UWORD FAR *);
extern RETCODE (SQL_API *lpSQLForeignKeys)(HSTMT,UCHAR FAR *,SWORD,UCHAR FAR *,
                                           SWORD,UCHAR FAR *,SWORD,
                                           UCHAR  FAR *,SWORD,UCHAR FAR *,
                                           SWORD,UCHAR FAR *,SWORD);
extern RETCODE (SQL_API *lpSQLMoreResults)(HSTMT);
extern RETCODE (SQL_API *lpSQLNativeSql)(HDBC,UCHAR FAR *,SDWORD,UCHAR FAR *,
                                         SDWORD,SDWORD FAR *);
extern RETCODE (SQL_API *lpSQLNumParams)(HSTMT,SWORD FAR *);
extern RETCODE (SQL_API *lpSQLParamOptions)(HSTMT,UDWORD,UDWORD FAR *);
extern RETCODE (SQL_API *lpSQLPrimaryKeys)(HSTMT,UCHAR FAR *,SWORD,UCHAR FAR *,
                                           SWORD,UCHAR FAR *,SWORD);
extern RETCODE (SQL_API *lpSQLProcedureColumns)(HSTMT,UCHAR FAR *,SWORD,
                                                UCHAR FAR *,SWORD,UCHAR FAR *,
                                                SWORD,UCHAR FAR *,SWORD);
extern RETCODE (SQL_API *lpSQLProcedures)(HSTMT,UCHAR FAR *,SWORD,UCHAR FAR *,
                                          SWORD,UCHAR FAR *,SWORD);
extern RETCODE (SQL_API *lpSQLSetPos)(HSTMT,UWORD,UWORD,UWORD);
extern RETCODE (SQL_API *lpSQLTablePrivileges)(HSTMT,UCHAR FAR *,SWORD,UCHAR FAR *,
                                               SWORD,UCHAR FAR *,SWORD);


/* SDK 2.0 Additions */


extern RETCODE (SQL_API *lpSQLDrivers)(HENV,UWORD,UCHAR FAR *,SWORD,SWORD FAR *,
                                       UCHAR FAR *,SWORD,SWORD FAR *);

extern RETCODE (SQL_API *lpSQLBindParameter)(HSTMT,UWORD,SWORD,SWORD,SWORD,
                                             UDWORD,SWORD,PTR,SDWORD,SDWORD FAR *);

// obsolete stuff

/* Use SQLBindParameter for ODBC 2.0 */
extern RETCODE (SQL_API *lpSQLSetParam)(HSTMT,UWORD,SWORD,SWORD,UDWORD,SWORD,
                                        PTR,SDWORD FAR *);

/* Use SQLSetStmtOptions for ODBC 2.0 */
extern RETCODE (SQL_API *lpSQLSetScrollOptions)(HSTMT,UWORD,SDWORD,UWORD);


// IMPORT ORDINALS FOR ODBC

#define ORD_SQLALLOCCONNECT        MAKEINTRESOURCE(1)
#define ORD_SQLALLOCENV            MAKEINTRESOURCE(2)
#define ORD_SQLALLOCSTMT           MAKEINTRESOURCE(3)
#define ORD_SQLBINDCOL             MAKEINTRESOURCE(4)
#define ORD_SQLBINDKEY             MAKEINTRESOURCE(72)
#define ORD_SQLBROWSECONNECT       MAKEINTRESOURCE(55)
#define ORD_SQLCANCEL              MAKEINTRESOURCE(5)
#define ORD_SQLCOLATTRIBUTES       MAKEINTRESOURCE(6)
#define ORD_SQLCOLUMNPRIVILEGES    MAKEINTRESOURCE(56)
#define ORD_SQLCOLUMNS             MAKEINTRESOURCE(40)
#define ORD_SQLCONNECT             MAKEINTRESOURCE(7)
#define ORD_SQLDATASOURCES         MAKEINTRESOURCE(57)
#define ORD_SQLDESCRIBECOL         MAKEINTRESOURCE(8)
#define ORD_SQLDESCRIBEPARAM       MAKEINTRESOURCE(58)
#define ORD_SQLDISCONNECT          MAKEINTRESOURCE(9)
#define ORD_SQLDRIVERCONNECT       MAKEINTRESOURCE(41)
#define ORD_SQLDRIVERS             MAKEINTRESOURCE(71)
#define ORD_SQLERROR               MAKEINTRESOURCE(10)
#define ORD_SQLEXECDIRECT          MAKEINTRESOURCE(11)
#define ORD_SQLEXECUTE             MAKEINTRESOURCE(12)
#define ORD_SQLEXTENDEDFETCH       MAKEINTRESOURCE(59)
#define ORD_SQLFETCH               MAKEINTRESOURCE(13)
#define ORD_SQLFOREIGNKEYS         MAKEINTRESOURCE(60)
#define ORD_SQLFREECONNECT         MAKEINTRESOURCE(14)
#define ORD_SQLFREEENV             MAKEINTRESOURCE(15)
#define ORD_SQLFREESTMT            MAKEINTRESOURCE(16)
#define ORD_SQLGETCONNECTOPTION    MAKEINTRESOURCE(42)
#define ORD_SQLGETCURSORNAME       MAKEINTRESOURCE(17)
#define ORD_SQLGETDATA             MAKEINTRESOURCE(43)
#define ORD_SQLGETFUNCTIONS        MAKEINTRESOURCE(44)
#define ORD_SQLGETINFO             MAKEINTRESOURCE(45)
#define ORD_SQLGETSTMTOPTION       MAKEINTRESOURCE(46)
#define ORD_SQLGETTYPEINFO         MAKEINTRESOURCE(47)
#define ORD_SQLMORERESULTS         MAKEINTRESOURCE(61)
#define ORD_SQLNATIVESQL           MAKEINTRESOURCE(62)
#define ORD_SQLNUMPARAMS           MAKEINTRESOURCE(63)
#define ORD_SQLNUMRESULTCOLS       MAKEINTRESOURCE(18)
#define ORD_SQLOPENTABLE           MAKEINTRESOURCE(73)
#define ORD_SQLPARAMDATA           MAKEINTRESOURCE(48)
#define ORD_SQLPARAMOPTIONS        MAKEINTRESOURCE(64)
#define ORD_SQLPREPARE             MAKEINTRESOURCE(19)
#define ORD_SQLPRIMARYKEYS         MAKEINTRESOURCE(65)
#define ORD_SQLPROCEDURECOLUMNS    MAKEINTRESOURCE(66)
#define ORD_SQLPROCEDURES          MAKEINTRESOURCE(67)
#define ORD_SQLPUTDATA             MAKEINTRESOURCE(49)
#define ORD_SQLROWCOUNT            MAKEINTRESOURCE(20)
#define ORD_SQLSETCONNECTOPTION    MAKEINTRESOURCE(50)
#define ORD_SQLSETCURSORNAME       MAKEINTRESOURCE(21)
#define ORD_SQLSETPARAM            MAKEINTRESOURCE(22)
#define ORD_SQLSETPOS              MAKEINTRESOURCE(68)
#define ORD_SQLSETSCROLLOPTIONS    MAKEINTRESOURCE(69)
#define ORD_SQLSETSTMTOPTION       MAKEINTRESOURCE(51)
#define ORD_SQLSPECIALCOLUMNS      MAKEINTRESOURCE(52)
#define ORD_SQLSTATISTICS          MAKEINTRESOURCE(53)
#define ORD_SQLTABLEPRIVILEGES     MAKEINTRESOURCE(70)
#define ORD_SQLTABLES              MAKEINTRESOURCE(54)
#define ORD_SQLTRANSACT            MAKEINTRESOURCE(23)







                       // WDBUTIL PROC ADDRESSES

extern BOOL (FAR PASCAL *wdbSQLProgram)(LPCSTR lpcString);
extern int (FAR PASCAL *wdbQueryErrorMessage)(LPSTR lpBuf, WORD cbBufLength);
extern HWND (FAR PASCAL *wdbCreateBrowseWindow)(HWDB hDb, HWND hwndParent, WORD wFlags);
extern HWDB (FAR PASCAL *wdbOpenDatabase)(LPCSTR lpszName, DWORD dwMode);
extern BOOL (FAR PASCAL *wdbCloseDatabase)(HWDB hDb);



extern char pVersionString[];


                      /** TRULY EXTERNAL VARIABLES **/

extern WORD PASCAL __ahincr;  /* use OFFSET of this to increment HUGE sel */

extern SBPCHAR FAR PASCAL _HELP_START_[];
extern SBPCHAR FAR PASCAL _HELP_START2_[];
extern char FAR PASCAL _HELP_COMMANDS_[];
extern char FAR PASCAL _HELP_KEYS_[];
extern char FAR PASCAL _HELP_VARIABLES_[];
extern char FAR PASCAL _HELP_ANSI_[];


extern WORD wFormatPercentComplete;     // current 'percent complete' for
                                        // current disk format operation.

extern WORD wCopyPercentComplete;       // like the above, but used for
                                        // tracking copy operations.




/***************************************************************************/
/*                                                                         */
/*          SPECIAL 'inline' FUNCTIONS (most stolen from DBUTIL1.H)        */
/*                (only included if static vars are defined)               */
/*                                                                         */
/***************************************************************************/


__inline short PASCAL iswap(short i)
{
   return((int)(((WORD)i >> 8) + ((WORD)(i & 0xff) << 8)));
}




#ifdef WIN32

#define _ihmemcpy(X,Y,Z) memcpy(X,Y,Z)
#define _ihmemcmp(X,Y,Z) memcmp(X,Y,Z)
#define _ihmemset(X,Y,Z) memset(X,Y,Z)


// 16-bit procs no longer supported in 32-bit world

#define MoveTo(X,Y,Z) MoveToEx(X,Y,Z,NULL)

static __inline DWORD WINAPI GetTextExtent(HDC hDC, LPCSTR szStr, DWORD cbLen)
{
SIZE sz;

  GetTextExtentPoint32(hDC, szStr, cbLen, &sz);
  return(MAKELONG((WORD)sz.cx,(WORD)sz.cy));
}


#else /* WIN32 */


             // SET UP 'INLINE' MEMORY COPY/COMPARE/SET

#define _fmemcpy(X,Y,Z) _ihmemcpy(X,Y,Z)
#define _fmemcmp(X,Y,Z) _ihmemcmp(X,Y,Z)
#define _fmemset(X,Y,Z) _ihmemset(X,Y,Z)


#pragma optimize("gl",off)    // disables global optimizations


static __inline char HUGE * PASCAL _ihmemcpy(void HUGE *hp1,
                                               const void HUGE *hp2,
                                               DWORD dwSize)
{
extern BOOL _ok_to_use_386_instructions_;


   if(!dwSize) return(hp1);

   /* 'DS' must be DGROUP for this to work! */

   if(_ok_to_use_386_instructions_)
   {
      _asm
      {
         push ds
         push es
         _emit(0x66) _asm push si
         _emit(0x66) _asm push di

         _emit(0x66) _asm xor si, si   /* mov esi, 0 */
         _emit(0x66) _asm xor di, di   /* mov edi, 0 */

         les DI, hp1         /* destination is the 1st parm...  */
         lds SI, hp2         /* source is the 2nd parm...       */

         _emit(0x66) _asm mov CX, WORD PTR dwSize  /* mov ecx, dwSize */
         _emit(0x66) _asm or CX,CX

         jz hmemcpy_386_done

         cld
         _emit(0x66) _asm push cx /* push ecx */

         _emit(0x66) _asm _emit(0x83) _asm _emit(0xe1) _asm _emit(0xfc)
         /* and ecx, 0xfffffffc ;trim off lowest 2 bits - total # of DWORD's */

         jz hmemcpy_386_word

         _emit(0x66) _asm shr cx, 2  /* shr ecx, 2   change 'ecx' to DWORD count */

         _emit(0xf3) _asm _emit(0x67) _asm _emit(0x66) _asm _emit(0xa5)
                   /*  rep movs DWORD PTR es:[edi], ds:[esi] */


hmemcpy_386_word:               ; what's left?  0, 1, 2, or 3 bytes

         _emit(0x66) _asm pop cx  /* pop ecx */

         test cx, 2
         jz hmemcpy_386_byte

         _emit(0x67) _asm _emit(0xa5) /* movs WORD PTR es:[edi], ds:[esi] */


hmemcpy_386_byte:

         test cx, 1
         jz hmemcpy_386_done

         _emit(0x67) _asm _emit(0xa4) /* movs BYTE PTR es:[edi], ds:[esi] */


hmemcpy_386_done:

         _emit(0x66) _asm pop di
         _emit(0x66) _asm pop si

         pop es
         pop ds
      }

      return(hp1);
   }
   else
   {
      _hmemcpy(hp1, hp2, dwSize);
      return(hp1);
   }


}


static __inline int PASCAL _ihmemcmp(const void HUGE *hp1,
                                     const void HUGE *hp2, DWORD dwSize)
{
register int rval;
extern BOOL _ok_to_use_386_instructions_;


   if(!dwSize) return(0);

   /* 'DS' must be DGROUP for this to work! */

   if(_ok_to_use_386_instructions_)
   {
      _asm
      {
         push ds
         push es
         _emit(0x66) _asm push si
         _emit(0x66) _asm push di

         _emit(0x66) _asm xor si, si   /* mov esi, 0 */
         _emit(0x66) _asm xor di, di   /* mov edi, 0 */

         lds SI, hp1         /* source is the 1st parm...  */
         les DI, hp2         /* target is the 2nd parm...  */

         _emit(0x66) _asm mov CX, WORD PTR dwSize  /* mov ecx, dwSize */
         _emit(0x66) _asm or CX,CX

         jz hmemcmp_386_done


         _emit(0xf3) _asm _emit(0x67) _asm _emit(0xa6)
                               /*  repz cmps BYTE PTR ds:[esi], es:[edi] */

hmemcmp_386_done:

         _emit(0x66) _asm pop di
         _emit(0x66) _asm pop si

         pop es
         pop ds

         mov rval, 0
         jz hmemcmp_386_equal
         jb hmemcmp_386_below

         mov rval, 1              /* ABOVE! */
         jmp hmemcmp_386_equal

hmemcmp_386_below:

         mov rval, 0xffff

hmemcmp_386_equal:

      }

      return(rval);
   }
   else
   {
      return(_hmemcmp(hp1, hp2, dwSize));
   }


}


static __inline LPVOID _cdecl _ihmemset(LPVOID x, int y, DWORD dwSize)
{
extern BOOL _ok_to_use_386_instructions_;


   if(!dwSize) return(x);

   if(_ok_to_use_386_instructions_)
   {
      _asm
      {
         _emit(0x66) _asm push cx
         push di
         push es

         _emit(0x66) _asm xor di, di   /* mov edi, 0 */

         les di, x
         mov al, BYTE PTR y
         mov ah, al
         push ax
         _emit(0x66) _asm shl ax, 16        /* shl eax, 16 */
         pop ax

         cld
         _emit(0x66) _asm mov CX, WORD PTR dwSize  /* mov ecx, dwSize */
         _emit(0x66) _asm or CX,CX

         jz hmemset_386_done

         cld
         _emit(0x66) _asm push cx /* push ecx */

         _emit(0x66) _asm _emit(0x83) _asm _emit(0xe1) _asm _emit(0xfc)
         /* and ecx, 0xfffffffc ;trim off lowest 2 bits - total # of DWORD's */

         jz hmemset_386_word

         _emit(0x66) _asm shr cx, 2  /* shr ecx, 2   change 'ecx' to DWORD count */

         _emit(0xf3) _asm _emit(0x67) _asm _emit(0x66) _asm stosw
                                     /*  rep stos DWORD PTR es:[edi] */


hmemset_386_word:               ; what's left?  0, 1, 2, or 3 bytes

         _emit(0x66) _asm pop cx  /* pop ecx */

         test cx, 2
         jz hmemset_386_byte

         _emit(0x67) _asm stosw      /* stos WORD PTR es:[edi] */


hmemset_386_byte:

         test cx, 1
         jz hmemset_386_done

         _emit(0x67) _asm stosb      /* rep stos BYTE PTR es:[edi] */


hmemset_386_done:


         pop es
         pop di
         _emit(0x66) _asm pop cx
      }
   }
   else
   {
      _hmemset((HPSTR)x, (WORD)y, dwSize);
      return(x);
   }


   return(x);

}


#pragma optimize("",on)        // restores 'default' optimization


#endif /* WIN32 */



#endif /* STATIC_VARS */
