/***************************************************************************/
/*                                                                         */
/* SHLTHUNK.H - 32-bit and 16-bit THUNK definitions                        */
/*                                                                         */
/*                      TRADE SECRET - CONFIDENTIAL                        */
/*                                                                         */
/*             The compiled libraries and API header files are             */
/*          (c) Copyright 1990-1995 by Stewart~Frazier Tools, Inc          */
/*                                                                         */
/***************************************************************************/


#ifdef WIN32

#define TOOLHELP_MAX_PATH        (255)
#define TOOLHELP_MAX_MODULE_NAME (8 + 1)

// TAKEN FROM 16-bit HEADER FILES

#ifndef _FIND_T_DEFINED
#pragma pack(2)

struct _find_t {
    char reserved[21];
    char attrib;
    unsigned wr_time;
    unsigned wr_date;
    long size;
    char name[13];
    };

#pragma pack()
#define _FIND_T_DEFINED
#endif 



// TAKEN FROM 16-bit TOOLHELP.H

typedef struct tagGLOBALENTRY
{
    DWORD dwSize;
    DWORD dwAddress;
    DWORD dwBlockSize;
    HGLOBAL hBlock;
    WORD wcLock;
    WORD wcPageLock;
    WORD wFlags;
    BOOL wHeapPresent;
    HGLOBAL hOwner;
    WORD wType;
    WORD wData;
    DWORD dwNext;
    DWORD dwNextAlt;
} GLOBALENTRY;

/* GlobalFirst()/GlobalNext() flags */
#define GLOBAL_ALL      0
#define GLOBAL_LRU      1
#define GLOBAL_FREE     2

/* GLOBALENTRY.wType entries */
#define GT_UNKNOWN      0
#define GT_DGROUP       1
#define GT_DATA         2
#define GT_CODE         3
#define GT_TASK         4
#define GT_RESOURCE     5
#define GT_MODULE       6
#define GT_FREE         7
#define GT_INTERNAL     8
#define GT_SENTINEL     9
#define GT_BURGERMASTER 10

/* If GLOBALENTRY.wType==GT_RESOURCE, the following is GLOBALENTRY.wData: */
#define GD_USERDEFINED      0
#define GD_CURSORCOMPONENT  1
#define GD_BITMAP           2
#define GD_ICONCOMPONENT    3
#define GD_MENU             4
#define GD_DIALOG           5
#define GD_STRING           6
#define GD_FONTDIR          7
#define GD_FONT             8
#define GD_ACCELERATORS     9
#define GD_RCDATA           10
#define GD_ERRTABLE         11
#define GD_CURSOR           12
#define GD_ICON             14
#define GD_NAMETABLE        15
#define GD_MAX_RESOURCE     15

/* GLOBALENTRY.wFlags */
#define GF_PDB_OWNER        0x0100      /* Low byte is KERNEL flags */


typedef struct tagMEMMANINFO
{
    DWORD dwSize;
    DWORD dwLargestFreeBlock;
    DWORD dwMaxPagesAvailable;
    DWORD dwMaxPagesLockable;
    DWORD dwTotalLinearSpace;
    DWORD dwTotalUnlockedPages;
    DWORD dwFreePages;
    DWORD dwTotalPages;
    DWORD dwFreeLinearSpace;
    DWORD dwSwapFilePages;
    WORD wPageSize;
} MEMMANINFO;

typedef struct tagMODULEENTRY
{
    DWORD dwSize;
    char szModule[TOOLHELP_MAX_MODULE_NAME + 1];
    HMODULE hModule;
    WORD wcUsage;
    char szExePath[TOOLHELP_MAX_PATH + 1];
    WORD wNext;
} MODULEENTRY;

typedef struct tagTASKENTRY
{
    DWORD dwSize;
    HTASK hTask;
    HTASK hTaskParent;
    HINSTANCE hInst;
    HMODULE hModule;
    WORD wSS;
    WORD wSP;
    WORD wStackTop;
    WORD wStackMinimum;
    WORD wStackBottom;
    WORD wcEvents;
    HGLOBAL hQueue;
    char szModule[TOOLHELP_MAX_MODULE_NAME + 1];
    WORD wPSPOffset;
    HANDLE hNext;
} TASKENTRY;

int FAR PASCAL MyGetModuleUsage16(HMODULE hModule);

#define GetModuleUsage(X) MyGetModuleUsage16(X)

#define GetCurrentTask() ((HTASK)GetCurrentProcessId())

#endif // WIN32



// 16-bit functions


// TOOLHELP thunks

BOOL FAR PASCAL lpGlobalFirst(LPGLOBALENTRY lpGlobal, UINT wFlags);
BOOL FAR PASCAL lpGlobalNext(LPGLOBALENTRY lpGlobal, UINT wFlags);
BOOL FAR PASCAL lpMemManInfo(LPMEMMANINFO lpInfo);
BOOL FAR PASCAL lpModuleFirst(LPMODULEENTRY lpModule);
BOOL FAR PASCAL lpModuleNext(LPMODULEENTRY lpModule);
HTASK FAR PASCAL lpTaskFindHandle(LPTASKENTRY lpTask, HTASK hTask);
HMODULE FAR PASCAL lpModuleFindHandle(LPMODULEENTRY lpModule, HMODULE hModule);

BOOL FAR PASCAL lpIsTask(HTASK hTask);
BOOL FAR PASCAL lpTaskFirst(LPTASKENTRY lpTask);
BOOL FAR PASCAL lpTaskNext(LPTASKENTRY lpTask);



// internal procs

UINT FAR PASCAL GetDosMaxDrives(void);
LPTDB GetTDBFromPSP(UINT wPSPSeg);
LPSTR FAR PASCAL GetTaskAndModuleFromPSP(UINT wMachID, UINT wPSPSeg,
                                         HTASK FAR *lphTask);
LPWOATABLE FAR PASCAL GetWOATABLE();

