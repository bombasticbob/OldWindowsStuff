
#include <windows.h> 
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#include "flip.h"


// the following 'externals' aren't really needed.

//extern "C" char szIsPassword[22];
//extern "C" char szPassword[16];
//extern "C" char szDifferentPW[BUFFLEN];
//extern "C" char szChangePW[30];
//extern "C" char szBadOldPW[BUFFLEN];
//
//extern "C" HOOKPROC fpMessageFilter;



            /* function prototypes (internal ones) */

void PASCAL paint_picture(HWND);
void WriteProfileInt(LPSTR, LPSTR, int);
void GetIniSettings(void);


BOOL FAR PASCAL ScreenSaverConfigureDialog(HWND hDlg, UINT msg,
                                           WPARAM wParam, LPARAM lParam);



                /* Globals specific to FLIP */

#define DEFAULT_SPEED  25
#define DEFAULT_NLINES  4
#define BAR_HEIGHT     16         /* height of 'blanking' bar */

WORD wElapse, wTimer, wNLines;
RECT rcScreen;
WORD wCurX;      // current 'X' scroll offset!!

HBITMAP hBitmap; // bitmap storing 'hidden' bits

char szSpeedName[] = "Speed";
char szNLines[]    = "Lines";
char szIniName[]   = "CONTROL.INI";



void GetIniEntries(void)
{
  //Load Common Strings from stringtable...
//  LoadString(hMainInstance, idsIsPassword, szIsPassword, 22);
  LoadString(hMainInstance, idsIniFile, szIniFile, MAXFILELEN);
  LoadString(hMainInstance, idsScreenSaver, szScreenSaver, 22);
//  LoadString(hMainInstance, idsPassword, szPassword, 16);
//  LoadString(hMainInstance, idsDifferentPW, szDifferentPW, BUFFLEN);
//  LoadString(hMainInstance, idsChangePW, szChangePW, 30);
//  LoadString(hMainInstance, idsBadOldPW, szBadOldPW, 255);
  LoadString(hMainInstance, idsHelpFile, szHelpFile, MAXFILELEN);
  LoadString(hMainInstance, idsNoHelpMemory, szNoHelpMemory, BUFFLEN);
}


LONG FAR PASCAL ScreenSaverProc(HWND hWnd, unsigned msg, WPARAM wParam, LPARAM lParam)
{
DWORD dwTick;


   switch (msg)
   {
      case WM_CREATE:                             // FLIP-specific

         GetIniEntries();
         GetIniSettings();
         wTimer = SetTimer(hWnd, ID_TIMER, 1/*wElapse*/, NULL);
         hBitmap = NULL;

         break;

      case WM_TIMER:

         dwTick = GetTickCount();
         paint_picture(hWnd);

         dwTick = GetTickCount() - dwTick;

         if(dwTick < (DWORD)wElapse)
           Sleep((DWORD)wElapse - dwTick);

         break;

      case WM_DESTROY:

         if(hBitmap)
         {
            DeleteObject(hBitmap);
            hBitmap = NULL;
         }

         break;

      case WM_ERASEBKGND:

         // EAT CALL FOR 'ERASEBKGND' - I'm leaving it alone!

         return 0L;

      default:
          break;
   }

   return DefScreenSaverProc(hWnd, msg, wParam, lParam);
}

/* RegisterDialogClasses -- Entry point for registering window
 * classes required by configuration dialog box.
 *
 * Params:  hWnd -- Handle to window
 *
 * Return:  None
 */
BOOL WINAPI RegisterDialogClasses(HANDLE hInst)
{
    return TRUE;
}


/* ScreenSaverConfigureDialog -- Dialog box function for configuration
 * dialog.
 *
 * Params:  hWnd -- Handle to window
 *
 * Return:  None
 */
BOOL FAR PASCAL ScreenSaverConfigureDialog(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
   switch (msg)
   {
      case WM_INITDIALOG:                         // FLIP-specific
         GetIniSettings();
         SetDlgItemInt(hDlg, ID_SPEED,    wElapse,    FALSE);
         SetDlgItemInt(hDlg, ID_PRIORITY, wNLines,  FALSE);
         return TRUE;

      case WM_COMMAND:                            // FLIP-specific
         switch ((WORD)wParam)
         {
            case IDOK:
               wElapse    = GetDlgItemInt(hDlg, ID_SPEED,    NULL, FALSE);
               WriteProfileInt(szAppName, szSpeedName, wElapse);

               wNLines  = GetDlgItemInt(hDlg, ID_PRIORITY, NULL, FALSE);
               if(wNLines == 0  ||  wNLines >= (BAR_HEIGHT * 4))
               {
                  wNLines = DEFAULT_NLINES;
               }

               WriteProfileInt(szAppName, szNLines, wNLines);

               EndDialog(hDlg, TRUE);
               return TRUE;

            case IDCANCEL:
               EndDialog(hDlg, FALSE);
               return TRUE;

            default:
               break;
         }

      default:
         break;
   }

   return FALSE;
}



/* GetIniSettings -- Get initial bounce settings from WIN.INI
 *
 * Params:  hWnd -- Handle to window
 *
 * Return:  None
 */
void GetIniSettings()
{
   wElapse = GetPrivateProfileInt(szAppName, szSpeedName, DEFAULT_SPEED,
                                  szIniName);
   wNLines = GetPrivateProfileInt(szAppName, szNLines, DEFAULT_NLINES,
                                  szIniName);

   if(wNLines == 0  ||  wNLines >= (BAR_HEIGHT * 4))
   {
      wNLines = DEFAULT_NLINES;
   }

}


/* WriteProfileInt - Write an unsigned integer value to CONTROL.INI.
 *
 * Params:  name - hMainWindow - Handle to main application window.
 *
 * Return:  None
 */

void WriteProfileInt(LPSTR szApp, LPSTR szKey, int i)
{
char achBuf[40];

   /* write out as unsigned because GetPrivateProfileInt() can't
    * cope with signed values!
    */
   wsprintf(achBuf, "%u", i);

   WritePrivateProfileString(szApp,szKey,achBuf,szIniName);
}






void PASCAL paint_picture(HWND hWnd)
{
HDC hDC, hCompDC;
HBITMAP hOldBMP = NULL;
HPEN hBlk, hWht, hOldPen;
BOOL PaintNewDoohickey = FALSE;
WORD wRemaining;
int i;


   hDC = GetDC(hWnd);
   hCompDC = CreateCompatibleDC(hDC);

   hBlk = (HPEN)GetStockObject(BLACK_PEN);
   hWht = (HPEN)GetStockObject(WHITE_PEN);


   if(!hBitmap)  // signals that I'm just starting out...
   {
      GetWindowRect(hWnd, &rcScreen);

      wCurX = BAR_HEIGHT;     // always start with current 'X' as BAR HEIGHT

      hBitmap = CreateCompatibleBitmap(hDC, rcScreen.right - rcScreen.left,
                                       BAR_HEIGHT + wNLines * 2);

      hOldBMP = (HBITMAP)SelectObject(hCompDC, hBitmap);

      // make a copy of the upper 'BAR_HEIGHT + wNLines' lines of the screen

      BitBlt(hCompDC, 0, 0,
             rcScreen.right - rcScreen.left, BAR_HEIGHT + wNLines,
             hDC, rcScreen.left, rcScreen.top, SRCCOPY);


      // scroll screen up 'BAR_HEIGHT' lines

      ScrollWindowEx(hWnd, 0, - BAR_HEIGHT, NULL, NULL, NULL, NULL, 0);

      SelectObject(hCompDC, hOldBMP);

      PaintNewDoohickey = TRUE;
   }
   else
   {
      // see if I'm scrolling past the top of the screen (again)

      if((wCurX + wNLines) >= (rcScreen.bottom - rcScreen.top))
      {
         hOldBMP = (HBITMAP)SelectObject(hCompDC, hBitmap);

         wRemaining = (rcScreen.bottom - rcScreen.top) - wCurX;

         // make a copy of the 'remaining' rows of the screen into bitmap

         BitBlt(hCompDC, 0, BAR_HEIGHT,
                rcScreen.right - rcScreen.left, wRemaining,
                hDC, rcScreen.left, rcScreen.top, SRCCOPY);


         // scroll screen up 'BAR_HEIGHT + "remaining"' lines

         ScrollWindowEx(hWnd, 0, - (int)(BAR_HEIGHT + wRemaining),
                        NULL, NULL, NULL, NULL, 0);


         // NEXT, copy the current bitmap contents to the screen bottom.
         // Current height of bitmap is:  BAR_HEIGHT + wRemaining

         BitBlt(hDC, rcScreen.left, rcScreen.bottom - BAR_HEIGHT - wRemaining,
                rcScreen.right - rcScreen.left, BAR_HEIGHT + wRemaining,
                hCompDC, 0, 0, SRCCOPY);


         // AT THIS POINT THE SCREEN IS THE WAY IT WAS WHEN WE STARTED!!
         // Now, all I have to do is duplicate the method used at the
         // beginning of this process!

         SelectObject(hCompDC, hOldBMP);

         DeleteObject(hBitmap);       // forces process to start over!
         hBitmap = NULL;

         ReleaseDC(hWnd, hDC);
         DeleteDC(hCompDC);

         return;
      }
      else  // NO PROBLEM!  Just scroll the thing...
      {
         // FIRST step:  make a copy of the top lines from screen to
         //              my temp bitmap

         hOldBMP = (HBITMAP)SelectObject(hCompDC, hBitmap);

         BitBlt(hCompDC, 0, BAR_HEIGHT,
                rcScreen.right - rcScreen.left, wNLines,
                hDC, rcScreen.left, rcScreen.top, SRCCOPY);


         // NEXT step:  scroll screen by 'wNLines'

         ScrollWindowEx(hWnd, 0, - (int)wNLines, NULL, NULL, NULL, NULL, 0);



         // NEXT, scroll top 'wNLines' of bitmap to screen bottom

         BitBlt(hDC, rcScreen.left, rcScreen.bottom - wNLines,
                rcScreen.right - rcScreen.left, wNLines,
                hCompDC, 0, 0, SRCCOPY);


         // FINALLY, scroll my bitmap up by 'wNLines' thus matching the
         // 'blank' areas with the contents of my bitmap!

         BitBlt(hCompDC, 0, 0, rcScreen.right - rcScreen.left, BAR_HEIGHT,
                hCompDC, 0, wNLines, SRCCOPY);


         SelectObject(hCompDC, hOldBMP);

#ifdef USE_CURX
         wCurX += wNLines;
#else
         wCurX = BAR_HEIGHT + wNLines;  // just for fun!
#endif
      }
   }



   if(PaintNewDoohickey)
   {
      // paint the 'doohickey' at the bottom of the screen

      hOldPen = (HPEN)SelectObject(hDC, hBlk); // 4 black lines

#define MoveTo(X,Y,Z) MoveToEx(X,Y,Z,NULL)

      MoveTo(hDC, rcScreen.left, rcScreen.bottom - BAR_HEIGHT);
      LineTo(hDC, rcScreen.right, rcScreen.bottom - BAR_HEIGHT);

      MoveTo(hDC, rcScreen.left, rcScreen.bottom - BAR_HEIGHT + 1);
      LineTo(hDC, rcScreen.right, rcScreen.bottom - BAR_HEIGHT + 1);

      MoveTo(hDC, rcScreen.left, rcScreen.bottom - BAR_HEIGHT + 2);
      LineTo(hDC, rcScreen.right, rcScreen.bottom - BAR_HEIGHT + 2);

      MoveTo(hDC, rcScreen.left, rcScreen.bottom - BAR_HEIGHT + 3);
      LineTo(hDC, rcScreen.right, rcScreen.bottom - BAR_HEIGHT + 3);

      SelectObject(hDC, hWht);           // 2 white lines

      MoveTo(hDC, rcScreen.left, rcScreen.bottom - BAR_HEIGHT + 4);
      LineTo(hDC, rcScreen.right, rcScreen.bottom - BAR_HEIGHT + 4);

      MoveTo(hDC, rcScreen.left, rcScreen.bottom - BAR_HEIGHT + 5);
      LineTo(hDC, rcScreen.right, rcScreen.bottom - BAR_HEIGHT + 5);

      SelectObject(hDC, hBlk);           // 2 black lines

      MoveTo(hDC, rcScreen.left, rcScreen.bottom - BAR_HEIGHT + 6);
      LineTo(hDC, rcScreen.right, rcScreen.bottom - BAR_HEIGHT + 6);

      MoveTo(hDC, rcScreen.left, rcScreen.bottom - BAR_HEIGHT + 7);
      LineTo(hDC, rcScreen.right, rcScreen.bottom - BAR_HEIGHT + 7);

      SelectObject(hDC, hWht);           // with white 'nub' in middle!

      MoveTo(hDC, (rcScreen.left + rcScreen.right) / 2 - 16,
                  rcScreen.bottom - BAR_HEIGHT + 6);
      LineTo(hDC, (rcScreen.left + rcScreen.right) / 2 + 16,
                  rcScreen.bottom - BAR_HEIGHT + 6);

      MoveTo(hDC, (rcScreen.left + rcScreen.right) / 2 - 16,
                  rcScreen.bottom - BAR_HEIGHT + 7);
      LineTo(hDC, (rcScreen.left + rcScreen.right) / 2 + 16,
                  rcScreen.bottom - BAR_HEIGHT + 7);


      SelectObject(hDC, hBlk);           // remaining lines are black!

      for(i=8; i<BAR_HEIGHT; i++)
      {
         MoveTo(hDC, rcScreen.left, rcScreen.bottom - BAR_HEIGHT + i);
         LineTo(hDC, rcScreen.right, rcScreen.bottom - BAR_HEIGHT + i);

         MoveTo(hDC, rcScreen.left, rcScreen.bottom - BAR_HEIGHT + i);
         LineTo(hDC, rcScreen.right, rcScreen.bottom - BAR_HEIGHT + i);
      }

      SelectObject(hDC, hOldPen);
   }


   DeleteDC(hCompDC);

   ReleaseDC(hWnd, hDC);

}
