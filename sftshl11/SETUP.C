/***************************************************************************/
/*                                                                         */
/*  SETUP.C - runs "SFTSHELL SFTSETUP.BAT"                                 */
/*                                                                         */
/***************************************************************************/


#include "windows.h"
#include "stdlib.h"
#include "string.h"
#include "memory.h"


#ifndef EXPORT
#define EXPORT __export
#endif /* EXPORT */



BOOL EXPORT WINAPI TempDialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);


int PASCAL WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nCMDShow)
{
char tbuf[256];
LPSTR lp1;
HWND hDlg;



   hDlg = CreateDialog(hInst, "SETUPDLG", NULL, TempDialogProc);
   if(hDlg)
   {
      ShowWindow(hDlg, SW_SHOWNORMAL);

      SetWindowPos(hDlg, HWND_TOPMOST, 0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE);
   }

   GetModuleFileName((HMODULE)hInst, tbuf, sizeof(tbuf));

   if(!*tbuf)
   {
      DestroyWindow(hDlg);

      MessageBox(NULL, "?Internal error - cannot get module file name",
                 "* SFTSHELL SETUP 'KICKOFF' PROGRAM *",
                 MB_OK | MB_ICONHAND | MB_SYSTEMMODAL);

      return(1);
   }


   lp1 = _fstrrchr(tbuf, '\\');
   if(!lp1 || !*lp1)
   {
      DestroyWindow(hDlg);

      MessageBox(NULL, "?Internal error - cannot get module path",
                 "* SFTSHELL SETUP 'KICKOFF' PROGRAM *",
                 MB_OK | MB_ICONHAND | MB_SYSTEMMODAL);

      return(1);
   }

   lstrcpy(lp1 + 1, "SFTSHELL.EXE SFTSETUP.BAT");

   if((UINT)WinExec(tbuf, SW_SHOWNORMAL) <= 32)
   {
      DestroyWindow(hDlg);

      MessageBox(NULL, "?Internal error - unable to execute SFTShell from source diskette",
                 "* SFTSHELL SETUP 'KICKOFF' PROGRAM *",
                 MB_OK | MB_ICONHAND | MB_SYSTEMMODAL);

      return(1);
   }


   DestroyWindow(hDlg);

   return(0);  // let-er rip!

}



BOOL EXPORT WINAPI TempDialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

   switch(message)
   {
      case WM_INITDIALOG:

         return(TRUE);



   }

   return(FALSE);

}
