
// NOTE:  These defines must be done MANUALLY or it won't work!
#define _X86_
#define WIN32 1

#include "windows.h"
#include "windowsx.h"
#include "mmsystem.h"
#include "stdio.h"
#include "stdlib.h"
#include "stddef.h"
#include "math.h"

#include "scrnsave.h"
#include "star32.h"
#include "resource.h"


#pragma comment(lib,"scrnsave.lib")
#pragma comment(lib,"winmm.lib")

/* Externals defined in SCRNSAVE.LIB. Required for all screen savers. */

// 'hMainInstance' defined in scrnsave.lib now!
//HINSTANCE hMainInstance;                      // app instance handle
extern HWND hMainWindow;                      // main window handle

#define CD _cdecl

extern "C" {
TCHAR CD szName[TITLEBARNAMELEN]    ="";
TCHAR CD szIsPassword[22]           ="";
TCHAR CD szIniFile[MAXFILELEN]      ="";
TCHAR CD szScreenSaver[22]          ="";
TCHAR CD szPassword[16]             ="";
TCHAR CD szDifferentPW[BUFFLEN]     ="";
TCHAR CD szChangePW[30]             ="";
TCHAR CD szBadOldPW[BUFFLEN]        ="";
TCHAR CD szHelpFile[MAXFILELEN]     ="";
TCHAR CD szNoHelpMemory[BUFFLEN]    ="";
TCHAR CD szAppName[APPNAMEBUFFERLEN]="";
UINT  CD MyHelpMessage;

};

HOOKPROC _cdecl fpMessageFilter;




                        // CLASSES AND STRUCTURES

//typedef double fixpoint; // FOR TESTING!!
typedef long fixpoint; // FOR TESTING!!

#define fpsqrt(X) sqrt(X)

//#include "fixpoint.h"



typedef struct {
   fixpoint XDelta, YDelta;  // 'delta' from X,Y origin (i.e. along my course)
//   fixpoint ZDelta;          // distance in 'Z' axis from bow of ship
   double ZDelta;            // for testing!
   fixpoint MinZDelta;       // 'ZDelta' value below which star disappears
   fixpoint ZDelta2;         // distance in 'Z' axis for DISTANCE2 to star
   fixpoint ZDelta3;         // distance in 'Z' axis for DISTANCE3 to star
   fixpoint ZDelta4;         // distance in 'Z' axis for DISTANCE4 to star
   fixpoint ZDelta5;         // distance in 'Z' axis for DISTANCE5 to star
   fixpoint ZDelta6;         // distance in 'Z' axis for DISTANCE6 to star
   fixpoint ZDelta7;         // distance in 'Z' axis for DISTANCE7 to star
   fixpoint ZDelta8;         // distance in 'Z' axis for DISTANCE8 to star
   fixpoint ZDelta9;         // distance in 'Z' axis for DISTANCE9 to star
   fixpoint ZDelta10;        // distance in 'Z' axis for DISTANCE10 to star
   fixpoint ZDelta11;        // distance in 'Z' axis for DISTANCE11 to star
   fixpoint ZDelta12;        // distance in 'Z' axis for DISTANCE12 to star
   fixpoint ZDelta13;        // distance in 'Z' axis for DISTANCE13 to star
   fixpoint ZDelta14;        // distance in 'Z' axis for DISTANCE14 to star
   fixpoint ZDelta15;        // distance in 'Z' axis for DISTANCE15 to star
   int wX, wY;               // last calculated 'X' and 'Y' (screen coord)
   int wXZDelta;             // last "Z Delta" for X (anagraphic)
   } STAR;

typedef STAR FAR *LPSTAR;






                /* function prototypes (internal ones) */

void PASCAL paint_picture(HWND);
void WriteProfileInt(const char *, const char *, int);
void GetIniSettings(void);


extern "C" BOOL WINAPI ScreenSaverConfigureDialog(HWND hDlg, UINT msg,
                                                  WPARAM wParam, LPARAM lParam);
extern "C" BOOL WINAPI AboutDialog(HWND hDlg, UINT msg,
                                   WPARAM wParam, LPARAM lParam);

void CreateStar(LPSTAR);      // creates new star entry at address specified

void DrawStar(LPSTAR, HDC);   // draws star in specified device context
void UnDrawStar(LPSTAR, HDC); // un-draws star in specified device context

BOOL MoveStar(LPSTAR, HDC);   // un-draws star, updates star's position, and
                              // re-draws star in new position. returns TRUE
                              // if star should be deleted and another created


                      /** GLOBAL VARIABLES **/


//#define SCALE_FACTOR (256L)  /* this is ONE way to eliminate floats */
#define SCALE_FACTOR 1 /* for testing... */
//#define SCALE_FACTOR 4 /* for testing */

#define DEFAULT_STARS      500     /* default is 500 which is 'ok'! */
#define DEFAULT_WARP         3     /* warp factor 3 - user perception! */
#define DEFAULT_INTERVAL    50     /* 50 msec per timer interval (smooth!) */
#define DEFAULT_ENABLE_SOUND 0     /* by default, sound is DISabled */
#define DEFAULT_ANAGRAPHIC   0     /* by default, anagrahpic is NOT enabled */

#define DISTANCE1a 4096  /* distance at which star becomes visible - dk grey */
#define DISTANCE2a 2048  /* distance at which star becomes light grey */
#define DISTANCE3a 1024  /* distance at which star becomes white */
#define DISTANCE4a  512  /* distance at which star becomes 2 white dots */
#define DISTANCE5a  256  /* distance at which star becomes 4 white dots */
#define DISTANCE6a  128  /* distance at which star becomes 9 white dots */

#define DISTANCE1 ((long)DISTANCE1a * SCALE_FACTOR)
#define DISTANCE2 ((long)DISTANCE2a * SCALE_FACTOR)
#define DISTANCE3 ((long)DISTANCE3a * SCALE_FACTOR)
#define DISTANCE4 ((long)DISTANCE4a * SCALE_FACTOR)
#define DISTANCE5 ((long)DISTANCE5a * SCALE_FACTOR)
#define DISTANCE6 ((long)DISTANCE6a * SCALE_FACTOR)

WORD wNStars, wTimer, wWarpFactor, wInterval;
BOOL bEnableSound, bAnagraphic;
RECT rcScreen;

//fixpoint fpScreenWidth, fpSpeed;
double fpScreenWidth, fpSpeed;  // for testing!


LPSTAR lpStars = NULL;


// MIDI support

#define MCI_NOT_IN_USE (MCIDEVICEID)-1L 

MCIDEVICEID mciID = MCI_NOT_IN_USE;  // when open, allows sound playback
char szMIDI[MAX_PATH] = "";


// INI constants

const char szStars[]      = "Stars";
const char szInterval[]   = "Interval";
const char szWarpFactor[] = "WarpFactor";
const char szEnableSound[]= "EnableSound";
const char szAnagraphic[] = "Anagraphic";



//extern "C" LONG WINAPI _WinMain(void *dw1, void *dw2, char *dw3, DWORD dw4)
//{
//   return(WinMain(dw1, dw2, dw3, dw4));
//}


void GetIniEntries(void)
{
  // Load Common Strings from stringtable...

  LoadString(hMainInstance, idsAppName,      szName,         TITLEBARNAMELEN);
  LoadString(hMainInstance, idsIsPassword,   szIsPassword,   22);
  LoadString(hMainInstance, idsIniFile,      szIniFile,      MAXFILELEN);
  LoadString(hMainInstance, idsScreenSaver,  szScreenSaver,  22);
  LoadString(hMainInstance, idsPassword,     szPassword,     16);
  LoadString(hMainInstance, idsDifferentPW,  szDifferentPW,  BUFFLEN);
  LoadString(hMainInstance, idsChangePW,     szChangePW,     30);
  LoadString(hMainInstance, idsBadOldPW,     szBadOldPW,     255);
  LoadString(hMainInstance, idsHelpFile,     szHelpFile,     MAXFILELEN);
  LoadString(hMainInstance, idsNoHelpMemory, szNoHelpMemory, BUFFLEN);
}


int random(int r)                 /* Generate Random Number 0-r */
{
   int s;
   long t;

   s = rand();
   t = (long) s * r / 32768;
   return((int) t);
}



extern "C" LONG WINAPI ScreenSaverProc(HWND hWnd, unsigned msg, WPARAM wParam, LPARAM lParam)
{
WORD w1;
char tbuf[64];  // temporary!


   switch (msg)
   {
      case WM_CREATE:                             // STAR32-specific

         GetIniEntries();
         GetIniSettings();

         GetClientRect(hWnd, &rcScreen);

         lpStars = (STAR FAR *)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
                                              sizeof(*lpStars) * wNStars);

         if(!lpStars) return(-1);  // this is what you return to FAIL!


         // randomize the random number generator

         srand(HIWORD(GetTickCount()) ^ LOWORD(GetTickCount()));

         // CALCULATE DISTANCE TO 'DISPLAY PLANE' (as if it were a window)
         // Distance to display plane is assumed to be 1/5 of the horizontal
         // width from the viewer's nose so that an object goes 'off screen'
         // at roughly 60 degrees given the worst case (one of the corners).

//         fpScreenWidth = (rcScreen.right - rcScreen.left) / (4.0 * sqrt(2));
         fpScreenWidth = SCALE_FACTOR *
                         (rcScreen.right - rcScreen.left) / (4.0 * sqrt(2));

         // WARP SPEED is determined by the number of 'fpScreenWidth'
         // distances covered in 1 second.  Using 'wInterval' I can
         // determine how much relative distance this is per 'wInterval'

         fpSpeed = ((double)(wInterval / 1000.0) * wWarpFactor) * fpScreenWidth;


         // set timer for correct 'wInterval' value

         wTimer = SetTimer(hWnd, ID_TIMER, wInterval, NULL);


         // create all of the stars!

         for(w1=0; w1<wNStars; w1++)
         {
            CreateStar(lpStars + w1);
         }

         // if user has elected to allow sound, read the 'MIDI' resource,
         // write it to a temp file, then open the MCI sequencer
         // at this time.  On error, don't bother reporting it (just ignore)

         if(bEnableSound)  // sound is enabled!
         {
            // create a temporary file and write the RIFF resource into it

            HRSRC hrsrc = FindResource(hMainInstance, "SPACE_THRILLER",
                                       "MIDI");
            if(hrsrc)
            {
               DWORD dwResSize = SizeofResource(hMainInstance, hrsrc);

               HGLOBAL hres = LoadResource(hMainInstance, hrsrc);
               LPSTR lpRes = NULL;

               if(dwResSize && hres && (lpRes = (LPSTR)LockResource(hres)))
               {
                  static char szTemp[MAX_PATH];

                  GetTempPath(sizeof(szTemp), szTemp);

                  // get temporary file name
                  if(!GetTempFileName(szTemp, "STAR32.TMP", 0, szMIDI))
                  {
                     *szMIDI = 0;  // ensure it's zero length buffer now
                  }

                  HANDLE hFile = INVALID_HANDLE_VALUE;

                  if(*szMIDI) hFile = CreateFile(szMIDI,
                                                 GENERIC_READ | GENERIC_WRITE,
                                                 0,
                                                 NULL,
                                                 CREATE_ALWAYS,
                                                 FILE_ATTRIBUTE_TEMPORARY,
                                                 NULL);

                  if(hFile == INVALID_HANDLE_VALUE)
                  {
                     *szMIDI = 0;  // prevent purging file (in case of problem)
                  }
                  else
                  {
                     WriteFile(hFile, lpRes, dwResSize, &dwResSize, NULL);
                     CloseHandle(hFile);

                     MCI_OPEN_PARMS mop;
                     mop.dwCallback = NULL;
                     mop.wDeviceID = MCI_NOT_IN_USE; // this is returned afterwards
                     mop.lpstrDeviceType = "sequencer";
                     mop.lpstrElementName = szMIDI;
                     mop.lpstrAlias = NULL;

                     if(!mciSendCommand(0, MCI_OPEN,
                                        MCI_WAIT | MCI_OPEN_ELEMENT | MCI_OPEN_TYPE,
                                        (DWORD)(LPVOID)&mop))
                     {
                        mciID = mop.wDeviceID;  // MIDI DEVICE!

                        // IMMEDIATELY begin playing the appropriate file

                        MCI_PLAY_PARMS mpp;

                        mpp.dwCallback = (DWORD)hWnd;  // callback to ME!
                        mpp.dwFrom = 0;
                        mpp.dwTo = 0;

                        if(mciSendCommand(mciID, MCI_PLAY, MCI_NOTIFY,
                                          (DWORD)(LPVOID)&mpp))
                        {
                           // on error, just close the MIDI device now

                           MCI_GENERIC_PARMS mgp;
                           mgp.dwCallback = NULL;

                           mciSendCommand(mciID, MCI_CLOSE, MCI_WAIT,
                                          (DWORD)(LPVOID)&mgp);

                           mciID = MCI_NOT_IN_USE;
                        }
                     }
                  }
//                  UnlockResource(hres);  no longer needed; for reference


               }



//               if(hres) FreeResource(hres);  no longer needed; for reference
            }

         }


         break;


      case MM_MCINOTIFY:  // ah, HAH!  I'm to repeat my MIDI!!!

         if((WORD)wParam == MCI_NOTIFY_SUCCESSFUL)
         {
            // rewind first

            MCI_SEEK_PARMS msp;
            msp.dwCallback = NULL;

            if(mciSendCommand(mciID, MCI_SEEK, MCI_WAIT | MCI_SEEK_TO_START,
                              (DWORD)(LPVOID)&msp))
            {
               // TODO:  put some kind of error trap here
            }

            MCI_PLAY_PARMS mpp;

            mpp.dwCallback = (DWORD)hWnd;  // callback to ME!
            mpp.dwFrom = 0;
            mpp.dwTo = 0;

            mciSendCommand(mciID, MCI_PLAY, MCI_NOTIFY,
                          (DWORD)(LPVOID)&mpp);
         }
         else  // I got something else
         {
            // TODO:  put error trap here
         }

         return(0);  // always


      case WM_TIMER:

         paint_picture(hWnd);
         break;


      case WM_DESTROY:

         if(mciID != MCI_NOT_IN_USE)
         {
            // close midi device (should stop playing)

            MCI_GENERIC_PARMS mgp;

            mgp.dwCallback = NULL;
            mciSendCommand(mciID, MCI_STOP, MCI_WAIT,
                           (DWORD)(LPVOID)&mgp);

            mgp.dwCallback = NULL;
            mciSendCommand(mciID, MCI_CLOSE, MCI_WAIT,
                           (DWORD)(LPVOID)&mgp);

            mciID = MCI_NOT_IN_USE;  // just because

            Sleep(500);  // let the file system "catch up"
         }

         // if the temp file exists, delete it

         if(*szMIDI)
         {
            if(!DeleteFile(szMIDI))
            {
               Sleep(500);
               DeleteFile(szMIDI);
            }
         }

         *szMIDI = 0;


         if(lpStars)
         {
            GlobalFreePtr(lpStars);
            lpStars = NULL;
         }

         break;


      case WM_ERASEBKGND:

         FillRect((HDC)wParam, &rcScreen,
                  (HBRUSH)GetStockObject(BLACK_BRUSH));

         return 0L;


      default:
          break;
   }

   return DefScreenSaverProc(hWnd, msg, wParam, lParam);
}



extern "C" BOOL WINAPI RegisterDialogClasses(HINSTANCE hInst)
//extern "C" BOOL _cdecl RegisterDialogClasses( HINSTANCE hInst )
{
   return TRUE;
}



extern "C" BOOL WINAPI ScreenSaverConfigureDialog(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
   switch (msg)
   {
      case WM_INITDIALOG:                         // STAR32-specific
         GetIniEntries();
         GetIniSettings();

         SetDlgItemInt(hDlg, ID_STARS,    wNStars,    FALSE);
         SetDlgItemInt(hDlg, ID_WARP,     wWarpFactor,  FALSE);
         SetDlgItemInt(hDlg, ID_INTERVAL, wInterval,  FALSE);

         CheckDlgButton(hDlg, IDC_MUSIC, bEnableSound?1:0);
         CheckDlgButton(hDlg, IDC_ANAGRAPH, bAnagraphic?1:0);

         return TRUE;

      case WM_COMMAND:                            // STAR32-specific
         switch (wParam)
         {
            case IDOK:
               wNStars     = GetDlgItemInt(hDlg, ID_STARS,    NULL, FALSE);
               WriteProfileInt(szName, szStars, wNStars);

               wWarpFactor = GetDlgItemInt(hDlg, ID_WARP, NULL, FALSE);
               WriteProfileInt(szName, szWarpFactor, wWarpFactor);

               wInterval   = GetDlgItemInt(hDlg, ID_INTERVAL, NULL, FALSE);
               WriteProfileInt(szName, szInterval, wInterval);

               bEnableSound = IsDlgButtonChecked(hDlg, IDC_MUSIC)?1:0;
               WriteProfileInt(szName, szEnableSound, bEnableSound);

               bAnagraphic = IsDlgButtonChecked(hDlg, IDC_ANAGRAPH) ? 1 : 0;
               WriteProfileInt(szName, szAnagraphic, bAnagraphic);

               EndDialog(hDlg, TRUE);
               return TRUE;

            case IDCANCEL:
               EndDialog(hDlg, FALSE);
               return TRUE;

            case IDC_ABOUT:
               DialogBox(hMainInstance, MAKEINTRESOURCE(IDD_ABOUT_SFT),
                         hDlg, (DLGPROC)AboutDialog);
               return(TRUE);

            default:
               break;
         }

      default:
         break;
   }

   return FALSE;
}

extern "C" BOOL WINAPI AboutDialog(HWND hDlg, UINT msg,
                                   WPARAM wParam, LPARAM lParam)
{
static const char szAbout[]=
  "Stewart~Frazier Tools, Inc., is located in Del Mar, California.\r\n"
  "We specialize in consulting services for Business Planning and "
  "Materials Management, and supply business analysis software tools "
  "that help to generate and maintain accurate forecasts, plus "
  "perform critical analysis that can help to improve the entire "
  "business planning process.\r\n\n"
  "Our software products include the Demand Planning Tool, our "
  "business analysis and forecast system, and SFTShell(tm), an "
  "enhanced command line interpreter for Windows and Windows '95.\r\n\n"
  "Please visit our internet site for additional information about "
  "the business planning process, forecasting, and product information.";

   switch (msg)
   {
      case WM_INITDIALOG:
         SetDlgItemText(hDlg, IDC_ABOUT_TEXT, szAbout);
         return(TRUE);

      case WM_COMMAND:                            // STAR32-specific
         switch (wParam)
         {
            case IDOK:
            case IDCANCEL:
               EndDialog(hDlg, 0);
               return(TRUE);
         }

         break;
   }

   return(FALSE);

}


void GetIniSettings()
{

   if(!*szIniFile)
   {
      OutputDebugString("?STAR32.SCR: GetIniSettings() - Force load 'INI' file name\r\n");

      GetIniEntries();
   }


   wNStars      = GetPrivateProfileInt(szName, szStars, DEFAULT_STARS,
                                       szIniFile);
   wWarpFactor  = GetPrivateProfileInt(szName, szWarpFactor, DEFAULT_WARP,
                                       szIniFile);
   wInterval    = GetPrivateProfileInt(szName, szInterval, DEFAULT_INTERVAL,
                                       szIniFile);

   bEnableSound = GetPrivateProfileInt(szName, szEnableSound, DEFAULT_ENABLE_SOUND,
                                       szIniFile);
   bAnagraphic  = GetPrivateProfileInt(szName, szAnagraphic, DEFAULT_ANAGRAPHIC,
                                       szIniFile);

}


void WriteProfileInt(const char * szApp, const char * szKey, int i)
{
char achBuf[40];

   /* write out as unsigned because GetPrivateProfileInt() can't
    * cope with signed values!
    */
   wsprintf(achBuf, "%u", i);

   WritePrivateProfileString(szApp,szKey,achBuf,szIniFile);
}






void CreateStar(LPSTAR lpEntry)
{
double Distance, scale2;


   lpEntry->XDelta = (fixpoint)(SCALE_FACTOR * (random(2 * DISTANCE1) - DISTANCE1));

   lpEntry->YDelta = (fixpoint)(SCALE_FACTOR * (random(2 * DISTANCE1) - DISTANCE1));


      // At what angle will the star be 60 degrees off the bow?

//   lpEntry->MinZDelta = fpsqrt(lpEntry->XDelta * lpEntry->XDelta +
//                               lpEntry->YDelta * lpEntry->YDelta)
//                               * (fixpoint)(.5);  // i.e. cos(60)

   // TEMPORARY

   lpEntry->MinZDelta = 0;     // for now, I'll do it *THIS* way!

   // TEMPORARY


   Distance  = sqrt(((double)lpEntry->XDelta / SCALE_FACTOR *
                     (double)lpEntry->XDelta / SCALE_FACTOR) +
                    ((double)lpEntry->YDelta / SCALE_FACTOR *
                     (double)lpEntry->YDelta / SCALE_FACTOR));

   if((Distance * SCALE_FACTOR) < (double)DISTANCE1)
   {
//      lpEntry->ZDelta = fpsqrt((fixpoint)DISTANCE1 * (fixpoint)DISTANCE1 -
//                               Distance * Distance);
      lpEntry->ZDelta = sqrt(((double)DISTANCE1 / SCALE_FACTOR *
                              (double)DISTANCE1 / SCALE_FACTOR) -
                              Distance * Distance) * SCALE_FACTOR;
   }
   else
   {
      lpEntry->ZDelta = DISTANCE2;
   }


   // calculate the various 'ZDelta' values for DISTANCEx constants!

   if((Distance * SCALE_FACTOR) < (double)DISTANCE2)
   {
//      lpEntry->ZDelta2 = fpsqrt((fixpoint)DISTANCE2 * (fixpoint)DISTANCE2 -
//                                Distance * Distance);
      lpEntry->ZDelta2 = sqrt((double)DISTANCE2 / SCALE_FACTOR *
                              (double)DISTANCE2 / SCALE_FACTOR -
                              Distance * Distance) * SCALE_FACTOR;
   }
   else
   {
      lpEntry->ZDelta2 = -1;
   }


   if((Distance * SCALE_FACTOR) < (double)DISTANCE3)
   {
//      lpEntry->ZDelta3 = fpsqrt((fixpoint)DISTANCE3 * (fixpoint)DISTANCE3 -
//                                Distance * Distance);
      lpEntry->ZDelta3 = sqrt((double)DISTANCE3 / SCALE_FACTOR *
                              (double)DISTANCE3 / SCALE_FACTOR -
                              Distance * Distance) * SCALE_FACTOR;
   }
   else
   {
      lpEntry->ZDelta3 = -1;
   }


   if((Distance * SCALE_FACTOR) < (double)DISTANCE4)
   {
//      lpEntry->ZDelta4 = fpsqrt((fixpoint)DISTANCE4 * (fixpoint)DISTANCE4 -
//                                Distance * Distance);
      lpEntry->ZDelta4 = sqrt((double)DISTANCE4 / SCALE_FACTOR *
                              (double)DISTANCE4 / SCALE_FACTOR -
                              Distance * Distance) * SCALE_FACTOR;
   }
   else
   {
      lpEntry->ZDelta4 = -1;
   }


   if((Distance * SCALE_FACTOR) < (double)DISTANCE5)
   {
//      lpEntry->ZDelta5 = fpsqrt((fixpoint)DISTANCE5 * (fixpoint)DISTANCE5 -
//                                Distance * Distance);
      lpEntry->ZDelta5 = sqrt((double)DISTANCE5 / SCALE_FACTOR *
                              (double)DISTANCE5 / SCALE_FACTOR -
                              Distance * Distance) * SCALE_FACTOR;
   }
   else
   {
      lpEntry->ZDelta5 = -1;
   }


   if((Distance * SCALE_FACTOR) < (double)DISTANCE6)
   {
//      lpEntry->ZDelta6 = fpsqrt((fixpoint)DISTANCE6 * (fixpoint)DISTANCE6 -
//                                Distance * Distance);
      lpEntry->ZDelta6 = sqrt((double)DISTANCE6 / SCALE_FACTOR *
                              (double)DISTANCE6 / SCALE_FACTOR -
                              Distance * Distance) * SCALE_FACTOR;
   }
   else
   {
      lpEntry->ZDelta6 = -1;
   }


   DrawStar(lpEntry, NULL);  // assigns other fields but does not paint

}


void DrawStar(LPSTAR lpEntry, HDC hDC)
{
int wX, wY;
COLORREF clr;
fixpoint fp1;

  COLORREF clrR = RGB(255,0,0);  // red
  COLORREF clrL = RGB(0,255,255);  // cyan
  COLORREF clrLDim = RGB(192,0,0);  // red
  COLORREF clrRDim = RGB(0,192,192);  // cyan

   // CALCULATE CORRECT 'X' and 'Y' SCREEN COORDINATES based on
   // 'fpScreenWidth' (appx 1/5 of screen - see 'WM_CREATE' processing)


   if(lpEntry->ZDelta != 0)
   {
//      fp1 = (fpScreenWidth / lpEntry->ZDelta);

//      wX = (int)((fixpoint)((rcScreen.right - rcScreen.left) >> 1) +
//                  (lpEntry->XDelta * fp1));

//      wY = (int)((fixpoint)((rcScreen.bottom - rcScreen.top) >> 1) +
//                  (lpEntry->YDelta * fp1));


      wX = (int)((fixpoint)((rcScreen.right - rcScreen.left) >> 1) +
        (lpEntry->XDelta * fpScreenWidth / lpEntry->ZDelta / SCALE_FACTOR));

      wY = (int)((fixpoint)((rcScreen.bottom - rcScreen.top) >> 1) +
        (lpEntry->YDelta * fpScreenWidth / lpEntry->ZDelta / SCALE_FACTOR));

   }
   else
   {
      wX = (int)((rcScreen.right - rcScreen.left) >> 1);

      wY = (int)((rcScreen.bottom - rcScreen.top) >> 1);
   }

//   int wXZDelta = (int)((fixpoint)8 * fpScreenWidth
//                        / sqrt(lpEntry->XDelta * lpEntry->XDelta
//                               + lpEntry->YDelta * lpEntry->YDelta
//                               + lpEntry->ZDelta * lpEntry->ZDelta)
//                        + 32 * fpScreenWidth / (lpEntry->ZDelta * lpEntry->ZDelta));

   int wXZDelta = 0;
   if(bAnagraphic)
     wXZDelta = (int)( ((fixpoint)8 * fpScreenWidth)
                       / lpEntry->ZDelta );

   if(wX>rcScreen.right || wX<rcScreen.left) wX = -1;
   if(wY>rcScreen.bottom || wY<rcScreen.top) wY = -1;

   lpEntry->wX = wX;
   lpEntry->wY = wY;
   lpEntry->wXZDelta = wXZDelta;

   int wRLimit = rcScreen.right - wXZDelta;

   if(wX<0 || wY<0 || !hDC)
   {
      return;                            // do not paint if NULL hDC or
                                         // off of the screen!!
   }

   if(lpEntry->ZDelta > lpEntry->ZDelta2)
   {
//      clr = RGB(128,128,128);  // dark grey
      clr = RGB(192,192,192);  // dark grey

      if(bAnagraphic && wX > 1)
        SetPixel(hDC, wX-1, wY, clrLDim);

      if(bAnagraphic && wX < rcScreen.right - 1)
        SetPixel(hDC, wX+1, wY, clrRDim);

      if(!bAnagraphic)
        SetPixel(hDC, wX, wY, clr);
   }
   else if(lpEntry->ZDelta > lpEntry->ZDelta3)
   {
//      clr = RGB(192,192,192);  // light grey
      clr = RGB(208,208,208);  // light grey

      if(bAnagraphic && wX > 1)
        SetPixel(hDC, wX-1, wY, clrLDim);

      if(bAnagraphic && wX < rcScreen.right - 1)
        SetPixel(hDC, wX+1, wY, clrRDim);

      SetPixel(hDC, wX, wY, clr);
   }
   else if(lpEntry->ZDelta > lpEntry->ZDelta4)
   {
      clr = RGB(255,255,255);  // white

      if(bAnagraphic && wX > wXZDelta)
        SetPixel(hDC, wX-wXZDelta, wY, clrL);

      if(bAnagraphic && wX < wRLimit)
        SetPixel(hDC, wX+wXZDelta, wY, clrR);

      if(!bAnagraphic || wXZDelta < 1)
        SetPixel(hDC, wX, wY, clr);
   }
   else if(lpEntry->ZDelta > lpEntry->ZDelta5)
   {
      clr = RGB(255,255,255);  // white

      // For this one, set 3 consecutive horizontal pixels
      // and put red on the right, cyan on the left

      if(bAnagraphic && wX > wXZDelta)
      {
        SetPixel(hDC, wX-wXZDelta, wY, clrL);
        SetPixel(hDC, wX-wXZDelta, wY+1, clrL);
      }

      if(bAnagraphic && wX < wRLimit)
      {
        SetPixel(hDC, wX+wXZDelta, wY, clrR);
        SetPixel(hDC, wX+wXZDelta, wY+1, clrR);
      }

      if(!bAnagraphic || wXZDelta < 1)
      {
        SetPixel(hDC, wX, wY, clr);
        SetPixel(hDC, wX, wY+1, clr);
      }
   }
   else if(lpEntry->ZDelta > lpEntry->ZDelta6)
   {
      clr = RGB(255,255,255);  // white

      // For this one, set 2 consecutive horizontal pixels and
      // vertical pixels (total of 4) and put extra cyan/red
      // on the left and right

      if(bAnagraphic && wX > wXZDelta)
      {
        SetPixel(hDC, wX-wXZDelta, wY, clrL);
        SetPixel(hDC, wX+1-wXZDelta, wY, clrL);
        SetPixel(hDC, wX-wXZDelta, wY+1, clrL);
        SetPixel(hDC, wX+1-wXZDelta, wY+1, clrL);
      }
      if(bAnagraphic && wX < wRLimit)
      {
        SetPixel(hDC, wX+wXZDelta, wY, clrR);
        SetPixel(hDC, wX+1+wXZDelta, wY, clrR);
        SetPixel(hDC, wX+wXZDelta, wY+1, clrR);
        SetPixel(hDC, wX+1+wXZDelta, wY+1, clrR);
      }

      if(!bAnagraphic || wXZDelta < 2)
      {
        SetPixel(hDC, wX, wY, clr);
        SetPixel(hDC, wX, wY+1, clr);
      }

      if(!bAnagraphic || wXZDelta < 1)
      {
        SetPixel(hDC, wX+1, wY, clr);
        SetPixel(hDC, wX+1, wY+1, clr);
      }
   }
   else
   {
      clr = RGB(255,255,255);  // white

      // For this one, set 3 consecutive horizontal pixels and
      // vertical pixels (total of 9) plus extra pixels for left
      // and right

      if(bAnagraphic && wX > wXZDelta)
      {
        SetPixel(hDC, wX-wXZDelta, wY, clrL);
        SetPixel(hDC, wX+1-wXZDelta, wY, clrL);
        SetPixel(hDC, wX+2-wXZDelta, wY, clrL);
        SetPixel(hDC, wX-wXZDelta, wY+1, clrL);
        SetPixel(hDC, wX+1-wXZDelta, wY+1, clrL);
        SetPixel(hDC, wX+2-wXZDelta, wY+1, clrL);
        SetPixel(hDC, wX-wXZDelta, wY+2, clrL);
        SetPixel(hDC, wX+1-wXZDelta, wY+2, clrL);
        SetPixel(hDC, wX+2-wXZDelta, wY+2, clrL);
      }

      if(bAnagraphic && wX < wRLimit)
      {
        SetPixel(hDC, wX+wXZDelta, wY, clrR);
        SetPixel(hDC, wX+1+wXZDelta, wY, clrR);
        SetPixel(hDC, wX+2+wXZDelta, wY, clrR);
        SetPixel(hDC, wX+wXZDelta, wY+1, clrR);
        SetPixel(hDC, wX+1+wXZDelta, wY+1, clrR);
        SetPixel(hDC, wX+2+wXZDelta, wY+1, clrR);
        SetPixel(hDC, wX+wXZDelta, wY+2, clrR);
        SetPixel(hDC, wX+1+wXZDelta, wY+2, clrR);
        SetPixel(hDC, wX+2+wXZDelta, wY+2, clrR);
      }

      if(!bAnagraphic || wXZDelta < 3)
      {
        SetPixel(hDC, wX, wY, clr);
        SetPixel(hDC, wX, wY+1, clr);
        SetPixel(hDC, wX, wY+2, clr);
      }

      if(!bAnagraphic || wXZDelta < 2)
      {
        SetPixel(hDC, wX+1, wY, clr);
        SetPixel(hDC, wX+1, wY+1, clr);
        SetPixel(hDC, wX+1, wY+2, clr);
      }

      if(!bAnagraphic || wXZDelta < 1)
      {
        SetPixel(hDC, wX+2, wY, clr);
        SetPixel(hDC, wX+2, wY+1, clr);
        SetPixel(hDC, wX+2, wY+2, clr);
      }
   }
}

void UnDrawStar(LPSTAR lpEntry, HDC hDC)
{
int wX, wY;
COLORREF clr;


   wX = lpEntry->wX;
   wY = lpEntry->wY;

   if(wX<0 || wY<0 || !hDC)
   {
      return;                            // do not paint if NULL hDC or
                                         // off of the screen!!
   }


   clr = RGB(0,0,0);           // black (always!)

   int wXZDelta = lpEntry->wXZDelta;
   int wRLimit = rcScreen.right - wXZDelta;

   if(lpEntry->ZDelta > lpEntry->ZDelta2)
   {
      if(bAnagraphic && wX > 1)
        SetPixel(hDC, wX-1, wY, clr);

      if(bAnagraphic && wX < rcScreen.right - 1)
        SetPixel(hDC, wX+1, wY, clr);

      if(!bAnagraphic)
        SetPixel(hDC, wX, wY, clr);
   }
   else if(lpEntry->ZDelta > lpEntry->ZDelta3)
   {
      if(bAnagraphic && wX > 1)
        SetPixel(hDC, wX-1, wY, clr);

      if(bAnagraphic && wX < rcScreen.right - 1)
        SetPixel(hDC, wX+1, wY, clr);

      SetPixel(hDC, wX, wY, clr);
   }
   else if(lpEntry->ZDelta > lpEntry->ZDelta4)
   {
      if(bAnagraphic && wX > wXZDelta)
        SetPixel(hDC, wX-wXZDelta, wY, clr);

      if(bAnagraphic && wX < wRLimit)
        SetPixel(hDC, wX+wXZDelta, wY, clr);

      SetPixel(hDC, wX, wY, clr);
   }
   else if(lpEntry->ZDelta > lpEntry->ZDelta5)
   {
      // For this one, set 2 consecutive horizontal pixels

      if(bAnagraphic && wX > wXZDelta)
      {
        SetPixel(hDC, wX-wXZDelta, wY, clr);
        SetPixel(hDC, wX-wXZDelta, wY+1, clr);
      }

      if(bAnagraphic && wX < wRLimit)
      {
        SetPixel(hDC, wX+wXZDelta, wY, clr);
        SetPixel(hDC, wX+wXZDelta, wY+1, clr);
      }

      if(!bAnagraphic || wXZDelta <= 1)
      {
        SetPixel(hDC, wX, wY, clr);
        SetPixel(hDC, wX, wY+1, clr);
      }
   }
   else if(lpEntry->ZDelta > lpEntry->ZDelta6)
   {
      // For this one, set 2 consecutive horizontal pixels and
      // vertical pixels (total of 4) plus extra on left

      if(bAnagraphic && wX > wXZDelta)
      {
        SetPixel(hDC, wX-wXZDelta, wY, clr);
        SetPixel(hDC, wX+1-wXZDelta, wY, clr);
        SetPixel(hDC, wX-wXZDelta, wY+1, clr);
        SetPixel(hDC, wX+1-wXZDelta, wY+1, clr);
      }

      if(bAnagraphic && wX < wRLimit)
      {
        SetPixel(hDC, wX+wXZDelta, wY, clr);
        SetPixel(hDC, wX+1+wXZDelta, wY, clr);
        SetPixel(hDC, wX+wXZDelta, wY+1, clr);
        SetPixel(hDC, wX+1+wXZDelta, wY+1, clr);
      }

      if(!bAnagraphic || wXZDelta <= 2)
      {
        SetPixel(hDC, wX, wY, clr);
        SetPixel(hDC, wX+1, wY, clr);
        SetPixel(hDC, wX, wY+1, clr);
        SetPixel(hDC, wX+1, wY+1, clr);
      }
   }
   else
   {
      // For this one, set 3 consecutive horizontal pixels and
      // vertical pixels (total of 9)

      if(bAnagraphic && wX > wXZDelta)
      {
        SetPixel(hDC, wX-wXZDelta, wY, clr);
        SetPixel(hDC, wX+1-wXZDelta, wY, clr);
        SetPixel(hDC, wX+2-wXZDelta, wY, clr);
        SetPixel(hDC, wX-wXZDelta, wY+1, clr);
        SetPixel(hDC, wX+1-wXZDelta, wY+1, clr);
        SetPixel(hDC, wX+2-wXZDelta, wY+1, clr);
        SetPixel(hDC, wX-wXZDelta, wY+2, clr);
        SetPixel(hDC, wX+1-wXZDelta, wY+2, clr);
        SetPixel(hDC, wX+2-wXZDelta, wY+2, clr);
      }

      if(bAnagraphic && wX < wRLimit)
      {
        SetPixel(hDC, wX+wXZDelta, wY, clr);
        SetPixel(hDC, wX+1+wXZDelta, wY, clr);
        SetPixel(hDC, wX+2+wXZDelta, wY, clr);
        SetPixel(hDC, wX+wXZDelta, wY+1, clr);
        SetPixel(hDC, wX+1+wXZDelta, wY+1, clr);
        SetPixel(hDC, wX+2+wXZDelta, wY+1, clr);
        SetPixel(hDC, wX+wXZDelta, wY+2, clr);
        SetPixel(hDC, wX+1+wXZDelta, wY+2, clr);
        SetPixel(hDC, wX+2+wXZDelta, wY+2, clr);
      }

      if(!bAnagraphic || wXZDelta <= 3)
      {
        SetPixel(hDC, wX, wY, clr);
        SetPixel(hDC, wX+1, wY, clr);
        SetPixel(hDC, wX+2, wY, clr);
        SetPixel(hDC, wX, wY+1, clr);
        SetPixel(hDC, wX+1, wY+1, clr);
        SetPixel(hDC, wX+2, wY+1, clr);
        SetPixel(hDC, wX, wY+2, clr);
        SetPixel(hDC, wX+1, wY+2, clr);
        SetPixel(hDC, wX+2, wY+2, clr);
      }
   }
}


// 'MoveStar()' - un-draws star, updates star's position, and
//                re-draws star in new position. returns TRUE
//                if star should be deleted and another created

//BOOL MoveStar(LPSTAR lpEntry, HDC hDC)
//{
//
//
//
//}



void PASCAL paint_picture(HWND hWnd)
{
HDC hDC;
WORD w1, w2;
LPSTAR lpE;


   hDC = GetDC(hWnd);

   for(w1=0, lpE = lpStars; w1<wNStars;)
   {
      UnDrawStar(lpE, hDC);  // un-draw the star (if drawn)

      lpE->ZDelta = lpE->ZDelta - fpSpeed;  // subtract current 'speed' factor
//      lpE->ZDelta -= fpSpeed;  // subtract current 'speed' factor

      if(lpE->wX<0 || lpE->wY<0 ||
         lpE->ZDelta <= lpE->MinZDelta)  // should star 'die'?
      {
         for(w2=w1 + 1; w2<wNStars; w2++)
         {
            lpStars[w2 - 1] = lpStars[w2];
         }

         CreateStar(lpStars + wNStars - 1);
      }
      else       // star is valid - draw it and increment counters!
      {
         DrawStar(lpE, hDC);

         w1++;
         lpE++;
      }
   }


   ReleaseDC(hWnd, hDC);

}
