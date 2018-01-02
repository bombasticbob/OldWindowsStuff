/***************************************************************************/
/*                                                                         */
/*  TOILET.C - a 'toilet' for dumping trash into.  Allows you to 'flush'   */
/*             it at any time (deleting the appropriate files).  As part   */
/*             of its operation it hides any files that are marked for     */
/*             deletion (except on floppies - there it will DELETE them    */
/*             with a dialog box to let you change your mind).  If you     */
/*             wish to 'undelete' any files, you may.  The toilet will     */
/*             flush automatically when Windows exits; a 'warning' dialog  */
/*             box will give the user a 'last minute' chance to save       */
/*             the files before Windows exits.                             */
/*                                                                         */
/***************************************************************************/

#define STRICT
#define WIN31

#include "windows.h"
#include "windowsx.h"
#include "mmsystem.h"
#include "shellapi.h"
#include "commdlg.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "dos.h"
#include "direct.h"
#include "toilet.h"


#define MAX_NAMES 256



		       /** ICON RESOURCE FORMAT **/

#pragma pack(1)


typedef struct IconDirectoryEntry {
    BYTE  bWidth;
    BYTE  bHeight;
    BYTE  bColorCount;
    BYTE  bReserved;
    WORD  wPlanes;
    WORD  wBitCount;
    DWORD dwBytesInRes;
    DWORD dwImageOffset;
} ICONDIRENTRY;


typedef struct ICONDIR {
    WORD          idReserved;
    WORD          idType;
    WORD          idCount;
    ICONDIRENTRY  idEntries[1];
} ICONHEADER;


typedef ICONHEADER FAR *LPICONHEADER;
typedef ICONDIRENTRY FAR *LPICONDIRENTRY;


// for now, the following are the ONLY acceptable icon formats!!


typedef struct tagICONIMAGE16_32x32 {         // 16-color icon image, 32x32
    BITMAPINFOHEADER icHeader;
    RGBQUAD          icColors[16];
    BYTE             icXOR[32*32*4/8];        // 32x32, 4-plane, 8 bits/byte
    BYTE             icAND[32*32*1/8];        // 32x32, 1-plane, 8 bits/byte
    } ICONIMAGE16_32x32;

typedef ICONIMAGE16_32x32 FAR *LPICONIMAGE16_32x32;

typedef struct tagICONIMAGE8_32x32 {          // 8-color icon image, 32x32
    BITMAPINFOHEADER icHeader;
    RGBQUAD          icColors[8];
    BYTE             icXOR[32*32*4/8];        // 32x32, 4-plane, 8 bits/byte
    BYTE             icAND[32*32*1/8];        // 32x32, 1-plane, 8 bits/byte
    } ICONIMAGE8_32x32;

typedef ICONIMAGE8_32x32 FAR *LPICONIMAGE8_32x32;

typedef struct tagICONIMAGE2_32x32 {          // 2-color icon image, 32x32
    BITMAPINFOHEADER icHeader;
    RGBQUAD          icColors[2];             // contains 2 colors!
    BYTE             icXOR[32*32*1/8];        // 32x32, 1-plane, 8 bits/byte
    BYTE             icAND[32*32*1/8];        // 32x32, 1-plane, 8 bits/byte
    } ICONIMAGE2_32x32;

typedef ICONIMAGE2_32x32 FAR *LPICONIMAGE2_32x32;





#pragma pack()

			 /* FUNCTION PROTOTYPES */



int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
		   LPSTR lpCmdLine, int nCmdShow);

BOOL FAR PASCAL InitApplication(HINSTANCE hInstance);
BOOL FAR PASCAL InitInstance(HINSTANCE hInstance, int nCmdShow);

BOOL CALLBACK __export About(HWND hDlg, UINT message, 
			     WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK __export MainWndProc(HWND hWnd, UINT message,
                                      WPARAM wParam, LPARAM lParam);
BOOL FAR PASCAL __export UndeleteDlgProc(HWND hDlg, UINT message,
					 WPARAM wParam, LPARAM lParam);

BOOL FAR PASCAL __export SetupDlgProc(HWND hDlg, UINT message,
                                      WPARAM wParam, LPARAM lParam);


void NEAR PASCAL MakeFlushingSound(void);
void NEAR PASCAL MakeDropSound(void);


LRESULT FAR PASCAL DoDropFiles(HWND hWnd, WPARAM wParam, LPARAM lParam);





BOOL FAR PASCAL My_hide(LPCSTR lpPath);

BOOL FAR PASCAL My_restore(LPCSTR lpPath);

BOOL FAR PASCAL My_purge(LPCSTR lpPath);

BOOL FAR PASCAL My_mkdir(LPCSTR lpPath);

BOOL FAR PASCAL PutDirTreeInListBox(LPSTR lpPath, HWND hWnd);

BOOL FAR PASCAL MyPurgeNode(LPSTR lpPath);

BOOL FAR PASCAL My_findfirst(LPSTR lpPath, UINT uiAttrib,
                             struct _find_t FAR *lpFF);

BOOL FAR PASCAL My_findnext(struct _find_t FAR *lpFF);



HICON FAR PASCAL MyCreateIcon(LPSTR pOpenIconName, LPSTR FAR *lplpOpenIcon);



extern void FAR PASCAL DOS3Call(void);


		       /* GLOBAL VARIABLE SECTION */


HINSTANCE hInst;
HWND hMainWnd;

HGLOBAL hresWave=NULL, hresWave2=NULL;
HRSRC hResInfo=NULL, hResInfo2=NULL;

BOOL Flushing = FALSE, Moving = FALSE;
WORD nNames=0, attr;
HLOCAL hNames[MAX_NAMES];
char pNameBuf[256];

char drive_flags[26] = {0};  // drive flags - am I using a particular drive?

BOOL KeepInFront=0;

char temp[128];


char pWindowClass[] = "Windows_3.1_Toilet_Class";
char pWindowCaption[] = "* TOILET *";





		/* 'WIN.INI' APPLICATION AND ITEM NAMES */

char pTOILET[]            = "TOILET";              // APPLICATION NAME

char pKEEPTOILETINFRONT[] = "KeepToiletInFront";
char _pFLUSHINGSOUND[]    = "FlushingSound";
char _pOPENICONNAME[]     = "OpenIconName";
char _pSHUTICONNAME[]     = "ShutIconName";
char _pDROPSOUND[]        = "DropSound";
char _pFULLICONNAME[]     = "FullIconName";
char _pHIDDENPATHNAME[]   = "HiddenPathName";
char _pICONCAPTION[]      = "IconCaption";

char pWAVE[]              = "WAVE";

char pTOILET_ERROR[]      = "** TOILET MALFUNCTION **";
char pDefHIDDENPATHNAME[] = "\\DELETED.SYS\\";
char pDefICONCAPTION[]    = "* TOILET *";


		     /** ICON AND WAVE RESOURCES **/


HICON hIcon1=NULL, hIcon1a=NULL, hIcon2=NULL;  // NULL means 'use pointers'

char pOpenIconName[128], pFullIconName[128], pShutIconName[128];
char pFlushingSound[128], pDropSound[128];
char pHiddenPathName[128];                     // always terminate with a '\'
char pIconCaption[64];

LPSTR lpOpenIcon=NULL, lpFullIcon=NULL, lpShutIcon=NULL; // POINTERS TO DATA!




int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
		   LPSTR lpCmdLine, int nCmdShow)
{
MSG msg;


   // first, change error mode to 1 so I can trap my own INT 24H stuff

   SetErrorMode(1);





   if (!hPrevInstance)
   {
       if (!InitApplication(hInstance))
       {
	   MessageBox(NULL, "?Unable to init instance!",
		       "** MEMLEFT **",
		       MB_OK | MB_ICONHAND | MB_SYSTEMMODAL);
	   return (FALSE);
       }
   }
   else
   {
       hMainWnd = FindWindow(pWindowClass, pWindowCaption);
       if(hMainWnd)
       {
	  SetFocus(hMainWnd);
	  return(0);
       }

   }

   if (!InitInstance(hInstance, nCmdShow))
   {
       MessageBox(NULL, "?Unable to init application!",
		   "** MEMLEFT **",
		   MB_OK | MB_ICONHAND | MB_SYSTEMMODAL);
       return (FALSE);
   }


   /* Acquire and dispatch messages until a WM_QUIT message is received. */

   while(GetMessage(&msg, NULL, NULL, NULL))
   {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
   }


   if(hIcon1)  DestroyIcon(hIcon1);
   if(hIcon1a) DestroyIcon(hIcon1a);
   if(hIcon2)  DestroyIcon(hIcon2);

   if(lpOpenIcon) GlobalFreePtr(lpOpenIcon);
   if(lpFullIcon) GlobalFreePtr(lpFullIcon);
   if(lpShutIcon) GlobalFreePtr(lpShutIcon);

   if(hresWave)  FreeResource(hresWave);  /* free old handle */
   if(hresWave2) FreeResource(hresWave2);

   return(msg.wParam);
}



BOOL FAR PASCAL InitApplication(HINSTANCE hInstance)
{
WNDCLASS  wc;


   wc.style         = 0;
   wc.lpfnWndProc   = MainWndProc;

   wc.cbClsExtra    = 0;
   wc.cbWndExtra    = 0;
   wc.hInstance     = hInstance;
   wc.hIcon         = NULL;

   wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
   wc.hbrBackground = GetStockObject(NULL_BRUSH);
   wc.lpszMenuName  = NULL;
   wc.lpszClassName = pWindowClass;

   return (RegisterClass(&wc));

}




BOOL FAR PASCAL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
int i, j;
char tbuf[66];


   hInst = hInstance;


   GetProfileString(pTOILET, _pOPENICONNAME, "", pOpenIconName,
		    sizeof(pOpenIconName));

   GetProfileString(pTOILET, _pFULLICONNAME, pOpenIconName, pFullIconName,
                    sizeof(pFullIconName));

   GetProfileString(pTOILET, _pSHUTICONNAME, "", pShutIconName,
		    sizeof(pShutIconName));

   GetProfileString(pTOILET, _pFLUSHINGSOUND, "", pFlushingSound,
		    sizeof(pFlushingSound));

   GetProfileString(pTOILET, _pDROPSOUND, "", pDropSound,
                    sizeof(pDropSound));

   GetProfileString(pTOILET, _pHIDDENPATHNAME, "", pHiddenPathName,
                    sizeof(pHiddenPathName));

   GetProfileString(pTOILET, _pICONCAPTION, "", pIconCaption,
                    sizeof(pIconCaption));


   if(!*pHiddenPathName)
   {
      strcpy(pHiddenPathName, pDefHIDDENPATHNAME);
   }

   if(!*pIconCaption)
   {
      strcpy(pIconCaption, pDefICONCAPTION);
   }

   if(!*pOpenIconName || !(hIcon1=MyCreateIcon(pOpenIconName,&lpOpenIcon)))
   {
      hIcon1 = LoadIcon(hInst, "TOILET1");
   }

   if(!*pFullIconName || !(hIcon1a=MyCreateIcon(pFullIconName,&lpFullIcon)))
   {
      hIcon1a = LoadIcon(hInst, "TOILET1A");
   }

   if(!*pShutIconName || !(hIcon2=MyCreateIcon(pShutIconName,&lpShutIcon)))
   {
      hIcon2 = LoadIcon(hInst, "TOILET2");
   }

   if(!*pFlushingSound)
   {
      if( hResInfo = FindResource(hInst, _pFLUSHINGSOUND, pWAVE) )
      {
	  hresWave = LoadResource(hInst, hResInfo);
      }
      else
      {
	  hresWave = NULL;
      }
   }

   if(!*pDropSound)
   {
      if( hResInfo2 = FindResource(hInst, _pDROPSOUND, pWAVE) )
      {
          hresWave2 = LoadResource(hInst, hResInfo2);
      }
      else
      {
          hresWave2 = NULL;
      }
   }


   hMainWnd = CreateWindow(pWindowClass, pWindowCaption,
			   WS_ICONIC | WS_OVERLAPPED | WS_SYSMENU,
			   CW_USEDEFAULT, CW_USEDEFAULT,
			   CW_USEDEFAULT, CW_USEDEFAULT,
			   NULL, NULL, hInstance, NULL);

   if(!hMainWnd)
   {

      if(hIcon1)  DestroyIcon(hIcon1);
      if(hIcon1a) DestroyIcon(hIcon1a);
      if(hIcon2)  DestroyIcon(hIcon2);

      if(lpOpenIcon) GlobalFreePtr(lpOpenIcon);
      if(lpFullIcon) GlobalFreePtr(lpFullIcon);
      if(lpShutIcon) GlobalFreePtr(lpShutIcon);

      if(hresWave)     FreeResource(hresWave);  /* free old handle */
      if(hresWave2)    FreeResource(hresWave2);

      hIcon1 = NULL;
      hIcon1a = NULL;
      hIcon2 = NULL;
      lpOpenIcon = NULL;
      lpFullIcon = NULL;
      lpShutIcon = NULL;

      hresWave = NULL;

      return (FALSE);
   }

   ShowWindow(hMainWnd, SW_SHOWMINNOACTIVE);

   // now, use a special form of 'PutDirTreeInListBox()' to get the total
   // number of files in the 'DELETED' pathname.

   nNames = 0;
   lstrcpy(tbuf + 2, pHiddenPathName);
   tbuf[1] = ':';

   for(i=0; i<26; i++)
   {
      j = GetDriveType(i);
      if(j==0 || j==1) continue;

      // here I can check some flags as to whether or not to include
      // removable drives in the list of DELETED FILES.
      // 'j==DRIVE_REMOVABLE' for such drives.



//      if(j==DRIVE_REMOVABLE) continue;  // on initial pass, ignore removables

      if(j==DRIVE_REMOVABLE)  // removable drive?? OK, make sure door shut!
      {                       // this method is clean, really clean!
         _asm
         {
            // PERFORM A 'DISK RESET' TO KICK SYSTEM IN THE TROUSERS

            mov ax, 0x0d00
            int 0x21              // performs a DISK RESET!

            mov ah, 0x36
            mov dl, BYTE PTR i
            inc dl
            int 0x21

            mov j, ax
         }

         if(j==0xffff) continue;  // NO DISK IN DRIVE!  Ignore it!

         j = DRIVE_REMOVABLE;     // restore original value for below...
      }

      tbuf[0] = (char)('A' + i);

      // only count errors for NON-REMOVABLE disk drives, eh?

      if(PutDirTreeInListBox(tbuf, NULL) && j!=DRIVE_REMOVABLE)
      {
         strcpy(temp, "?Error reading \"");
         lstrcat(temp, tbuf);
         lstrcat(temp, "\"");

         MessageBox(hMainWnd, temp, pTOILET_ERROR, MB_OK | MB_ICONHAND);
      }
   }


   InvalidateRect(hMainWnd, NULL, TRUE);

   UpdateWindow(hMainWnd);


   return(TRUE);

}


LRESULT CALLBACK __export MainWndProc(HWND hWnd, UINT message, WPARAM wParam,
                                      LPARAM lParam)
{
static HMENU hMenu;
LRESULT lRval;
FARPROC lpProc;
WORD i, j;
DWORD dwStartTime;
RECT r;
HDC hDC;
PAINTSTRUCT ps;
HCURSOR hCursor;



   switch(message)
   {

      case WM_CREATE:

	 hMenu = GetSystemMenu(hWnd, FALSE);

	 AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);

	 AppendMenu(hMenu, MF_STRING | MF_ENABLED | MF_UNCHECKED,
		    IDM_FLUSH, (LPSTR)"&Flush Toilet");

	 AppendMenu(hMenu, MF_STRING | MF_ENABLED | MF_UNCHECKED,
		    IDM_UNDELETE, (LPSTR)"&Undelete File");

	 AppendMenu(hMenu, MF_STRING | MF_ENABLED | MF_UNCHECKED,
		    IDM_ABOUT, (LPSTR)"&About 'Toilet'");

	 AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);

	 KeepInFront = GetProfileInt(pTOILET,pKEEPTOILETINFRONT,0);
	 if(KeepInFront)
	 {
	    AppendMenu(hMenu, MF_STRING | MF_ENABLED | MF_CHECKED,
		       IDM_INFRONT, (LPSTR)"&Keep Toilet in Front");
	    SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0,
			 SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
	 }
	 else
	 {
	    AppendMenu(hMenu, MF_STRING | MF_ENABLED | MF_UNCHECKED,
		       IDM_INFRONT, (LPSTR)"&Keep Toilet in Front");
	 }


	 AppendMenu(hMenu, MF_STRING | MF_ENABLED | MF_UNCHECKED,
		    IDM_SETUP, (LPSTR)"&Setup");


	 DragAcceptFiles(hWnd, TRUE);

         SetWindowText(hWnd, pIconCaption);

	 break;



      case WM_QUERYOPEN:
	 return(NULL);


      case WM_QUERYDRAGICON:

//         return(MAKELRESULT(LoadCursor(hInst, "TOILETCURSOR"), 0));

         if(hIcon1a && nNames)
            return(MAKELRESULT((WORD)hIcon1a,0));
         else
            return(MAKELRESULT((WORD)hIcon1,0));


      case WM_PAINT:

	 hDC = BeginPaint(hWnd, (LPPAINTSTRUCT)&ps);

	 DefWindowProc(hWnd, WM_ICONERASEBKGND, (WPARAM)hDC, NULL);

	 if(Flushing)
	 {
	    DrawIcon(hDC, 1, 1, hIcon2);

	    EndPaint(hWnd, (LPPAINTSTRUCT)&ps);
	 }
         else if(hIcon1a && nNames)
         {
            DrawIcon(hDC, 1, 1, hIcon1a);

	    EndPaint(hWnd, (LPPAINTSTRUCT)&ps);
	 }
         else
	 {
	    DrawIcon(hDC, 1, 1, hIcon1);

	    EndPaint(hWnd, (LPPAINTSTRUCT)&ps);
	 }

	 return(FALSE);


      case WM_DROPFILES:

         lRval = DoDropFiles(hWnd, wParam, lParam);
         MakeDropSound();

         GetClientRect(hWnd, (LPRECT)&r);
         InvalidateRect(hWnd, (LPRECT)&r, TRUE);
         UpdateWindow(hWnd);

         return(lRval);


      case WM_MENUSELECT:

//         if(((WORD)wParam & 0xfff0)==SC_RESTORE)
//         {
//            goto UnDeleteBox;
//         }
         break;


      case WM_DESTROY:
      case WM_ENDSESSION:

	 if(KeepInFront)
	 {
	    WriteProfileString(pTOILET,pKEEPTOILETINFRONT,"1");
	 }
	 else
	 {
	    WriteProfileString(pTOILET,pKEEPTOILETINFRONT,"0");
	 }

	 if(message==WM_ENDSESSION)
	 {
	    if(!wParam || !nNames) break;

	    i = MessageBox(hWnd, "Flush 'Toilet' before leaving Windows?",
			   "** TOILET - EXIT WINDOWS **",
			   MB_YESNO | MB_ICONHAND | MB_TASKMODAL);
	 }
	 else
	 {
	    if(nNames)
	    {
	       i = MessageBox(NULL, "Flush 'Toilet' before closing?",
			      "** TOILET - CLOSING **",
                              MB_YESNO | MB_ICONHAND | MB_TASKMODAL);
            }
	    else
	    {
	       DragAcceptFiles(hWnd, FALSE);

	       GetSystemMenu(hWnd, TRUE);
				    /* causes modified menu to go away */
	       PostQuitMessage(0);

	       break;
	    }
	 }

         if(i!=IDYES)  // If I get here, I am closing the app!!
         {
            DragAcceptFiles(hWnd, FALSE);

            GetSystemMenu(hWnd, TRUE);
				    /* causes modified menu to go away */
            PostQuitMessage(0);
            break;
         }


	       /* flows through to next section */

      case WM_SYSCOMMAND:

         if(message == WM_SYSCOMMAND &&
            ((WORD)wParam & 0xfff0) == SC_RESTORE)
         {
            goto UnDeleteBox;
         }
         else if(message==WM_SYSCOMMAND && wParam == IDM_ABOUT)
	 {
	    lpProc = MakeProcInstance((FARPROC)About, hInst);

	    DialogBox(hInst, "AboutBox", hWnd, (DLGPROC)lpProc);

	    FreeProcInstance(lpProc);
	    return(NULL);
	 }
	 else if(message!=WM_SYSCOMMAND || wParam == IDM_FLUSH)
         {
          static char tbuf[66];


	    hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

	    dwStartTime = GetCurrentTime();

	    if(message!=WM_DESTROY)
	    {
	       Flushing = TRUE;
	       GetClientRect(hWnd, (LPRECT)&r);
	       InvalidateRect(hWnd, (LPRECT)&r, TRUE);
	       UpdateWindow(hWnd);
	    }


            lstrcpy(tbuf + 2, pHiddenPathName);
            tbuf[1] = ':';

            for(i=0; i<26; i++)
            {
               j = GetDriveType(i);
               if(j==0 || j==1) continue;

               // here I can check some flags as to whether or not to include
               // removable drives in the list of DELETED FILES.
               // 'j==DRIVE_REMOVABLE' for such drives.



               if(j==DRIVE_REMOVABLE && !drive_flags[i])
               {
                  continue;
               }
               else if(j==DRIVE_REMOVABLE)
               {            // removable drive?? OK, make sure door is shut!
                            // this method is clean, really clean!
                  _asm
                  {
                     // PERFORM A 'DISK RESET' TO KICK SYSTEM IN THE TROUSERS

                     mov ax, 0x0d00
                     int 0x21              // performs a DISK RESET!

                     mov ah, 0x36
                     mov dl, BYTE PTR i
                     inc dl
                     int 0x21

                     mov j, ax
                  }

                  if(j==0xffff)
                  {
                     drive_flags[i] = 0;    // turn 'OFF' the flags!

                     continue;              // NO DISK IN DRIVE!  Ignore it!
                  }

                  j = DRIVE_REMOVABLE;     // restore original value for below...
               }

               tbuf[0] = (char)('A' + i);

               MyPurgeNode(tbuf);  // attempt to purge all files in the
                                   // 'pHiddenPathName' path on each
                                   // valid drive letter.
            }

            nNames = 0;

	    if(message!=WM_DESTROY)
	    {
	       MakeFlushingSound();

	       while((GetCurrentTime() - dwStartTime)<1000)
		  ;  /* just hang out for a bit... */

	       Flushing = FALSE;
	       GetClientRect(hWnd, (LPRECT)&r);
	       InvalidateRect(hWnd, (LPRECT)&r, TRUE);
	       UpdateWindow(hWnd);
	    }

	    SetCursor(hCursor);

	    if(message==WM_DESTROY)  /* window is being destroyed! */
	    {
	       DragAcceptFiles(hWnd, FALSE);

	       GetSystemMenu(hWnd, TRUE);
				    /* causes modified menu to go away */
	       PostQuitMessage(0);

	       break;
	    }

	    return(FALSE);
	 }
	 else if(wParam==IDM_UNDELETE)
         {
UnDeleteBox:

	    lpProc = MakeProcInstance((FARPROC)UndeleteDlgProc, hInst);

	    if(!lpProc)
	    {
	       MessageBox(hWnd, "?Unable to create 'undelete' dialog box",
                          pTOILET_ERROR, MB_OK | MB_ICONHAND);

	       return(NULL);
	    }


	    if(DialogBox(hInst, "UndeleteDialog", hWnd, (DLGPROC)lpProc))
	    {
	       FreeProcInstance(lpProc);

	       MessageBox(hWnd, "?Unable to create 'undelete' dialog box",
                          pTOILET_ERROR, MB_OK | MB_ICONHAND);

	       return(NULL);
	    }

            FreeProcInstance(lpProc);

            GetClientRect(hWnd, (LPRECT)&r);
            InvalidateRect(hWnd, (LPRECT)&r, TRUE);
            UpdateWindow(hWnd);

            return(NULL);

	 }
	 else if(wParam==IDM_INFRONT)
	 {
	    KeepInFront = !KeepInFront;

	    CheckMenuItem(hMenu, IDM_INFRONT,
			  MF_BYCOMMAND |
			  (KeepInFront?MF_CHECKED:MF_UNCHECKED));

	    if(KeepInFront)
	    {
	       SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0,
			    SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
	    }
	    else
	    {
	       SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0,
			    SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
	    }
         }
         else if(wParam==IDM_SETUP)
         {
            lpProc = MakeProcInstance((FARPROC)SetupDlgProc, hInst);

	    if(!lpProc)
	    {
               MessageBox(hWnd, "?Unable to create 'setup' dialog box",
                          pTOILET_ERROR, MB_OK | MB_ICONHAND);

	       return(NULL);
	    }


            if(DialogBox(hInst, "SetupDialog", hWnd, (DLGPROC)lpProc))
	    {
	       FreeProcInstance(lpProc);

               MessageBox(hWnd, "?Unable to create 'setup' dialog box",
                          pTOILET_ERROR, MB_OK | MB_ICONHAND);

	       return(NULL);
	    }

            FreeProcInstance(lpProc);

            GetClientRect(hWnd, (LPRECT)&r);
            InvalidateRect(hWnd, (LPRECT)&r, TRUE);
            UpdateWindow(hWnd);

            return(NULL);

         }

	 break;      /* continue on to 'DefWindowProc()' */


   }

   return(DefWindowProc(hWnd, message, wParam, lParam));
}




BOOL CALLBACK __export About(HWND hDlg, UINT message, WPARAM wParam, 
			     LPARAM lParam)
{

   switch (message)
   {
      case WM_INITDIALOG:

	 SetDlgItemText(hDlg, IDM_BRIEFHELP,
	   "To use this program, drag a file set from the File Manager"
           " onto the toilet icon.  The files will be moved to the 'hidden"
           " path' on the same drive (as assigned using the 'Setup' menu"
           " option). Flushing the toilet deletes all of the files in the"
           " 'hidden path'.  These files may be selectively restored (prior"
           " to flushing) by using 'Undelete' menu option. You can also"
           " customize 'TOILET' using 'Setup' (icons, caption, sounds).");

	 return (TRUE);

      case WM_COMMAND:

	 if (wParam == IDOK || wParam == IDCANCEL)
	 {
	     EndDialog(hDlg, TRUE);
	     return (TRUE);
	 }
	 break;
   }

   return (FALSE);
}





void NEAR PASCAL MakeFlushingSound()
{
LPSTR lpWave;

   if(hresWave)
   {
      if(!(lpWave = LockResource(hresWave)))
      {
	 FreeResource(hresWave);  /* free old handle */

	 if(!(hresWave = LoadResource(hInst, hResInfo))) return;

	 if(!(lpWave = LockResource(hresWave))) return;

      }

      sndPlaySound(lpWave, SND_ASYNC | SND_MEMORY | SND_NODEFAULT);

      UnlockResource(hresWave);
   }
   else if(*pFlushingSound)
   {
      sndPlaySound(pFlushingSound, SND_ASYNC | SND_NODEFAULT);
   }
}


void NEAR PASCAL MakeDropSound()
{
LPSTR lpWave;

   if(hresWave2)
   {
      if(!(lpWave = LockResource(hresWave2)))
      {
         FreeResource(hresWave2);  /* free old handle */

         if(!(hresWave2 = LoadResource(hInst, hResInfo2))) return;

         if(!(lpWave = LockResource(hresWave2))) return;

      }

      sndPlaySound(lpWave, SND_ASYNC | SND_MEMORY | SND_NODEFAULT);

      UnlockResource(hresWave2);
   }
   else if(*pDropSound)
   {
      sndPlaySound(pDropSound, SND_ASYNC | SND_NODEFAULT);
   }
}


LRESULT FAR PASCAL DoDropFiles(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
HANDLE hDrop;
WORD i, wNDrop;
HCURSOR hOldCursor;


   hDrop = (HANDLE)wParam;

   wNDrop = DragQueryFile(hDrop, -1, pNameBuf, sizeof(pNameBuf));

   hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

   for(i=0; i<wNDrop; i++)
   {
      DragQueryFile(hDrop, i, pNameBuf, sizeof(pNameBuf));
      My_hide(pNameBuf);
      nNames++;
   }

   SetCursor(hOldCursor);

   DragFinish(hDrop);

   return(TRUE);

}


void FAR PASCAL GetIconOrWaveFileName(HWND hDlg, WORD wCtlID)
{
OPENFILENAME ofn;
char tbuf[256];


   _fmemset(&ofn, 0, sizeof(ofn));

   GetDlgItemText(hDlg, wCtlID, tbuf, sizeof(tbuf));
   ofn.lStructSize  = sizeof(ofn);
   ofn.hwndOwner    = hDlg;

   if(wCtlID==IDM_WAVEFILE_NAME || wCtlID==IDM_DROPWAVE_NAME)
   {
      ofn.lpstrFilter  = "WAVE Files\0*.wav\0All Files\0*.*\0";
   }
   else
   {
      ofn.lpstrFilter  = "Icon Files\0*.ico\0All Files\0*.*\0";
   }

   ofn.nFilterIndex = 1;
   ofn.lpstrFile    = tbuf;
   ofn.nMaxFile     = sizeof(tbuf);
   ofn.lpstrTitle   = (LPSTR)((wCtlID==IDM_OPENICON_NAME)?
                              "* Get 'OPEN' icon file name *":
                              (wCtlID==IDM_FULLICON_NAME)?
                               "* Get 'FULL' icon file name *":
                              (wCtlID==IDM_SHUTICON_NAME)?
                               "* Get 'SHUT' icon file name *":
                              (wCtlID==IDM_WAVEFILE_NAME)?
                               "* Get 'FLUSH' wave file name *":
                              (wCtlID==IDM_DROPWAVE_NAME)?
                               "* Get 'DROP' wave file name *":
                               "* Get *UNKNOWN* File Name *");

   ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR |
               OFN_READONLY | OFN_NOTESTFILECREATE;


   if(GetOpenFileName(&ofn))
   {
      SetDlgItemText(hDlg, wCtlID, tbuf);

      if(wCtlID != IDM_WAVEFILE_NAME && wCtlID != IDM_DROPWAVE_NAME)
      {
         SendMessage(hDlg, WM_COMMAND, (WPARAM)wCtlID,
                  MAKELPARAM((WORD)GetDlgItem(hDlg, wCtlID), EN_KILLFOCUS));
      }
   }
}


BOOL FAR PASCAL __export SetupDlgProc(HWND hDlg, UINT message,
                                      WPARAM wParam, LPARAM lParam)
{
static LPSTR lpIc1, lpIc1a, lpIc2;
static HICON hIc1, hIc1a, hIc2;
static BOOL end_flag = FALSE;
DRAWITEMSTRUCT FAR *lpDI;
RECT r;
HWND hFocus;
HBRUSH hBrush, hOldBrush;
HPEN hPen, hPen2, hOldPen;
HBITMAP hBmp, hOldBmp;
COLORREF clrOldBk, clrOldFg;
HDC hDC;
OFSTRUCT ofstr;
int i, j;



   switch(message)
   {
      case WM_INITDIALOG:
         hIc1  = hIcon1;
         hIc1a = hIcon1a;
         hIc2  = hIcon2;

         lpIc1  = lpOpenIcon;
         lpIc1a = lpFullIcon;
         lpIc2  = lpShutIcon;

         SetDlgItemText(hDlg, IDM_OPENICON_NAME, pOpenIconName);
         SetDlgItemText(hDlg, IDM_FULLICON_NAME, pFullIconName);
         SetDlgItemText(hDlg, IDM_SHUTICON_NAME, pShutIconName);

         SetDlgItemText(hDlg, IDM_WAVEFILE_NAME, pFlushingSound);
         SetDlgItemText(hDlg, IDM_DROPWAVE_NAME, pDropSound);

         SetDlgItemText(hDlg, IDM_HIDDENPATH_NAME, pHiddenPathName);

         SetDlgItemText(hDlg, IDM_ICONCAPTION, pIconCaption);

         end_flag = FALSE;

         return(TRUE);


      case WM_COMMAND:

	 switch(wParam)
         {
            case IDOK:   //  this is what happens if the user hits ENTER

               hFocus = GetFocus();
               if(hFocus==GetDlgItem(hDlg, IDM_OPENICON_NAME))
               {
                  SendMessage(hDlg, WM_COMMAND, (WPARAM)IDM_OPENICON_NAME,
                              MAKELPARAM(hFocus, EN_KILLFOCUS));
                  break;
               }
               if(hFocus==GetDlgItem(hDlg, IDM_FULLICON_NAME))
               {
                  SendMessage(hDlg, WM_COMMAND, (WPARAM)IDM_FULLICON_NAME,
                              MAKELPARAM(hFocus, EN_KILLFOCUS));
                  break;
               }
               if(hFocus==GetDlgItem(hDlg, IDM_SHUTICON_NAME))
               {
                  SendMessage(hDlg, WM_COMMAND, (WPARAM)IDM_SHUTICON_NAME,
                              MAKELPARAM(hFocus, EN_KILLFOCUS));
                  break;
               }

               // for any others, just do default processing

               break;



            case IDNO:

               SetDlgItemText(hDlg, IDM_OPENICON_NAME, "");
               SetDlgItemText(hDlg, IDM_FULLICON_NAME, "");
               SetDlgItemText(hDlg, IDM_SHUTICON_NAME, "");
               SetDlgItemText(hDlg, IDM_WAVEFILE_NAME, "");
               SetDlgItemText(hDlg, IDM_DROPWAVE_NAME, "");
               SetDlgItemText(hDlg, IDM_WAVEFILE_NAME, "");

               SetDlgItemText(hDlg, IDM_HIDDENPATH_NAME, pDefHIDDENPATHNAME);
               SetDlgItemText(hDlg, IDM_ICONCAPTION, pDefICONCAPTION);


               SendMessage(hDlg, WM_COMMAND, (WPARAM)IDM_OPENICON_NAME,
                           MAKELPARAM(hFocus, EN_KILLFOCUS));

               SendMessage(hDlg, WM_COMMAND, (WPARAM)IDM_FULLICON_NAME,
                           MAKELPARAM(hFocus, EN_KILLFOCUS));

               SendMessage(hDlg, WM_COMMAND, (WPARAM)IDM_SHUTICON_NAME,
                           MAKELPARAM(hFocus, EN_KILLFOCUS));

               break;



            case IDCANCEL:     // user is bailing out without saving changes

               end_flag = TRUE;

               // make sure I free up resources used by this dialog!!

               if(lpIc1 && lpOpenIcon!=lpIc1)
               {
                  GlobalFreePtr(lpIc1);
                  lpIc1 = NULL;

                  if(hIc1 && hIc1!=hIcon1)
                  {
                     DestroyIcon(hIc1);
                  }
               }

               hIc1 = NULL;

               if(lpIc1a && lpFullIcon!=lpIc1a)
               {
                  GlobalFreePtr(lpIc1a);
                  lpIc1a = NULL;

                  if(hIc1a && hIc1a!=hIcon1)
                  {
                     DestroyIcon(hIc1a);
                  }
               }

               hIc1a = NULL;

               if(lpIc2 && lpShutIcon!=lpIc2)
               {
                  GlobalFreePtr(lpIc2);
                  lpIc2 = NULL;

                  if(hIc2 && hIc2!=hIcon1)
                  {
                     DestroyIcon(hIc2);
                  }
               }

               hIc2 = NULL;

               EndDialog(hDlg, 0);
               return(NULL);



            case IDM_OPENICON_NAME:

               if(end_flag) break;

               if(HIWORD(lParam)==EN_KILLFOCUS)
               {
                  GetDlgItemText(hDlg, IDM_OPENICON_NAME, temp, sizeof(temp));
                  while(*temp && *temp<=' ') strcpy(temp, temp+1);

                  if(*temp)
                  {
                     if(OpenFile(temp, &ofstr,
                                 OF_EXIST | OF_READ | OF_SHARE_DENY_NONE)
                        ==HFILE_ERROR)
                     {
                        return(FALSE);  // just ignore if it's not a file...
                     }

                     lstrcpy(temp, ofstr.szPathName);
                  }

                  if(lpIc1 && lpOpenIcon!=lpIc1)
                  {
                     GlobalFreePtr(lpIc1);
                     lpIc1 = NULL;

                     if(hIc1 && hIc1!=hIcon1)
                     {
                        DestroyIcon(hIc1);
                     }
                  }

                  hIc1 = NULL;

                  if(*temp)
                  {
                     hIc1 = MyCreateIcon(temp, &lpIc1);
                  }
                  else
                  {
                     hIc1 = LoadIcon(hInst, "TOILET1");
                  }

                  hFocus = GetDlgItem(hDlg, IDM_OPENICON);
                  InvalidateRect(hFocus, NULL, TRUE);
                  UpdateWindow(hFocus);

               }

               return(TRUE);


            case IDM_OPENICON_NAME2:

               GetIconOrWaveFileName(hDlg, IDM_OPENICON_NAME);
               return(TRUE);


            case IDM_FULLICON_NAME:

               if(end_flag) break;

               if(HIWORD(lParam)==EN_KILLFOCUS)
               {
                  GetDlgItemText(hDlg, IDM_FULLICON_NAME, temp, sizeof(temp));
                  while(*temp && *temp<=' ') strcpy(temp, temp+1);

                  if(*temp)
                  {
                     if(OpenFile(temp, &ofstr,
                                 OF_EXIST | OF_READ | OF_SHARE_DENY_NONE)
                        ==HFILE_ERROR)
                     {
                        return(FALSE);  // just ignore if it's not a file...
                     }

                     lstrcpy(temp, ofstr.szPathName);
                  }

                  if(lpIc1a && lpFullIcon!=lpIc1a)
                  {
                     GlobalFreePtr(lpIc1a);
                     lpIc1a = NULL;

                     if(hIc1a && hIc1a!=hIcon1)
                     {
                        DestroyIcon(hIc1a);
                     }
                  }

                  hIc1a = NULL;

                  if(*temp)
                  {
                     hIc1a = MyCreateIcon(temp, &lpIc1a);
                  }
                  else
                  {
                     hIc1a = LoadIcon(hInst, "TOILET1A");
                  }

                  hFocus = GetDlgItem(hDlg, IDM_FULLICON);
                  InvalidateRect(hFocus, NULL, TRUE);
                  UpdateWindow(hFocus);

               }

               return(TRUE);


            case IDM_FULLICON_NAME2:

               GetIconOrWaveFileName(hDlg, IDM_FULLICON_NAME);
               return(TRUE);


            case IDM_SHUTICON_NAME:

               if(end_flag) break;

               if(HIWORD(lParam)==EN_KILLFOCUS)
               {
                  GetDlgItemText(hDlg, IDM_SHUTICON_NAME, temp, sizeof(temp));
                  while(*temp && *temp<=' ') strcpy(temp, temp+1);

                  if(*temp)
                  {
                     if(OpenFile(temp, &ofstr,
                                 OF_EXIST | OF_READ | OF_SHARE_DENY_NONE)
                        ==HFILE_ERROR)
                     {
                        return(FALSE);  // just ignore if it's not a file...
                     }

                     lstrcpy(temp, ofstr.szPathName);
                  }

                  if(lpIc2 && lpShutIcon!=lpIc2)
                  {
                     GlobalFreePtr(lpIc2);
                     lpIc2 = NULL;

                     if(hIc2 && hIc2!=hIcon1)
                     {
                        DestroyIcon(hIc2);
                     }
                  }

                  hIc2 = NULL;

                  if(*temp)
                  {
                     hIc2 = MyCreateIcon(temp, &lpIc2);
                  }
                  else
                  {
                     hIc2 = LoadIcon(hInst, "TOILET2");
                  }

                  hFocus = GetDlgItem(hDlg, IDM_SHUTICON);
                  InvalidateRect(hFocus, NULL, TRUE);
                  UpdateWindow(hFocus);

               }

               return(TRUE);



            case IDM_SHUTICON_NAME2:

               GetIconOrWaveFileName(hDlg, IDM_SHUTICON_NAME);
               return(TRUE);


            case IDM_WAVEFILE_NAME2:

               GetIconOrWaveFileName(hDlg, IDM_WAVEFILE_NAME);
               return(TRUE);


            case IDM_DROPWAVE_NAME2:

               GetIconOrWaveFileName(hDlg, IDM_DROPWAVE_NAME);
               return(TRUE);


            case IDYES:

               end_flag = TRUE;

               // OPEN ICON

               GetDlgItemText(hDlg, IDM_OPENICON_NAME, temp, sizeof(temp));
               while(*temp && *temp<=' ') strcpy(temp, temp+1);
               if(*temp)
               {
                  if(OpenFile(temp, &ofstr,
                              OF_EXIST | OF_READ | OF_SHARE_DENY_NONE)
                     ==HFILE_ERROR)
                  {
                     memmove(temp + 1, temp, strlen(temp) + 1);
                     *temp = '\"';
                     strcat(temp, "\" does not exist");

                     MessageBox(hDlg,temp, pTOILET_ERROR, MB_OK | MB_ICONHAND);

                     end_flag = FALSE;
                     return(FALSE);
                  }

                  lstrcpy(temp, ofstr.szPathName);
               }
               strcpy(pOpenIconName, temp);
               if(lpIc1 && lpOpenIcon!=lpIc1)
               {
                  GlobalFreePtr(lpIc1);
                  lpIc1 = NULL;

                  if(hIc1 && hIc1!=hIcon1)
                  {
                     DestroyIcon(hIc1);
                  }
               }

               hIc1 = NULL;

               if(lpOpenIcon)
               {
                  GlobalFreePtr(lpOpenIcon);
                  lpOpenIcon = NULL;

                  if(hIcon1) DestroyIcon(hIcon1);
               }
               if(*temp)
               {
                  hIcon1 = MyCreateIcon(pOpenIconName, &lpOpenIcon);
               }
               else
               {
                  hIcon1 = LoadIcon(hInst, "TOILET1");
               }

               // FULL ICON

               GetDlgItemText(hDlg, IDM_FULLICON_NAME, temp, sizeof(temp));
               while(*temp && *temp<=' ') strcpy(temp, temp+1);
               if(*temp)
               {
                  if(OpenFile(temp, &ofstr,
                              OF_EXIST | OF_READ | OF_SHARE_DENY_NONE)
                     ==HFILE_ERROR)
                  {
                     memmove(temp + 1, temp, strlen(temp) + 1);
                     *temp = '\"';
                     strcat(temp, "\" does not exist");

                     MessageBox(hDlg,temp, pTOILET_ERROR, MB_OK | MB_ICONHAND);

                     end_flag = FALSE;
                     return(FALSE);
                  }

                  lstrcpy(temp, ofstr.szPathName);
               }
               strcpy(pFullIconName, temp);
               if(lpIc1a && lpFullIcon!=lpIc1a)
               {
                  GlobalFreePtr(lpIc1a);
                  lpIc1a = NULL;

                  if(hIc1a && hIc1a!=hIcon1a)
                  {
                     DestroyIcon(hIc1a);
                  }
               }

               hIc1a = NULL;

               if(lpFullIcon)
               {
                  GlobalFreePtr(lpFullIcon);
                  lpFullIcon = NULL;

                  if(hIcon1a) DestroyIcon(hIcon1a);
               }

               if(*temp)
               {
                  hIcon1a = MyCreateIcon(pFullIconName, &lpFullIcon);
               }
               else
               {
                  hIcon1a = LoadIcon(hInst, "TOILET1A");
               }

               // SHUT ICON

               GetDlgItemText(hDlg, IDM_SHUTICON_NAME, temp, sizeof(temp));
               while(*temp && *temp<=' ') strcpy(temp, temp+1);
               if(*temp)
               {
                  if(OpenFile(temp, &ofstr,
                              OF_EXIST | OF_READ | OF_SHARE_DENY_NONE)
                     ==HFILE_ERROR)
                  {
                     memmove(temp + 1, temp, strlen(temp) + 1);
                     *temp = '\"';
                     strcat(temp, "\" does not exist");

                     MessageBox(hDlg,temp, pTOILET_ERROR, MB_OK | MB_ICONHAND);

                     end_flag = FALSE;
                     return(FALSE);
                  }

                  lstrcpy(temp, ofstr.szPathName);
               }

               strcpy(pShutIconName, temp);

               if(lpIc2 && lpShutIcon!=lpIc2)
               {
                  GlobalFreePtr(lpIc2);
                  lpIc2 = NULL;

                  if(hIc2 && hIc2!=hIcon2)
                  {
                     DestroyIcon(hIc2);
                  }
               }

               hIc2 = NULL;

               if(lpShutIcon)
               {
                  GlobalFreePtr(lpShutIcon);
                  lpShutIcon = NULL;

                  if(hIcon2) DestroyIcon(hIcon2);
               }

               if(*temp)
               {
                  hIcon2 = MyCreateIcon(pShutIconName, &lpShutIcon);
               }
               else
               {
                  hIcon2 = LoadIcon(hInst, "TOILET2");
               }


               // FLUSHING SOUND

               GetDlgItemText(hDlg, IDM_WAVEFILE_NAME, temp, sizeof(temp));
               while(*temp && *temp<=' ') strcpy(temp, temp+1);
               if(*temp)
               {
                  if(OpenFile(temp, &ofstr,
                              OF_EXIST | OF_READ | OF_SHARE_DENY_NONE)
                     ==HFILE_ERROR)
                  {
                     memmove(temp + 1, temp, strlen(temp) + 1);
                     *temp = '\"';
                     strcat(temp, "\" does not exist");

                     MessageBox(hDlg,temp, pTOILET_ERROR, MB_OK | MB_ICONHAND);

                     end_flag = FALSE;
                     return(FALSE);
                  }

                  lstrcpy(temp, ofstr.szPathName);
               }

               strcpy(pFlushingSound, temp);

               if(hresWave) FreeResource(hresWave);  /* free old handle */
               if(*temp)
               {
                  hresWave = NULL;
               }
               else
               {
                  if(hResInfo = FindResource(hInst, _pFLUSHINGSOUND, pWAVE))
                  {
                     hresWave = LoadResource(hInst, hResInfo);
                  }
                  else
                  {
                     hresWave = NULL;
                  }
               }



               // DROP SOUND

               GetDlgItemText(hDlg, IDM_DROPWAVE_NAME, temp, sizeof(temp));
               while(*temp && *temp<=' ') strcpy(temp, temp+1);
               if(*temp)
               {
                  if(OpenFile(temp, &ofstr,
                              OF_EXIST | OF_READ | OF_SHARE_DENY_NONE)
                     ==HFILE_ERROR)
                  {
                     memmove(temp + 1, temp, strlen(temp) + 1);
                     *temp = '\"';
                     strcat(temp, "\" does not exist");

                     MessageBox(hDlg,temp, pTOILET_ERROR, MB_OK | MB_ICONHAND);

                     end_flag = FALSE;
                     return(FALSE);
                  }

                  lstrcpy(temp, ofstr.szPathName);
               }

               strcpy(pDropSound, temp);

               if(hresWave2) FreeResource(hresWave2);  /* free old handle */
               if(*temp)
               {
                  hresWave2 = NULL;
               }
               else
               {
                  if(hResInfo2 = FindResource(hInst, _pDROPSOUND, pWAVE))
                  {
                     hresWave2 = LoadResource(hInst, hResInfo2);
                  }
                  else
                  {
                     hresWave2 = NULL;
                  }
               }



               // HIDDEN PATH

               GetDlgItemText(hDlg, IDM_HIDDENPATH_NAME, temp, sizeof(temp));
               while(*temp && *temp<=' ') strcpy(temp, temp+1);
               if(*temp)
               {
                  i = strlen(temp);
                  if(temp[i - 1]!='\\')
                  {
                     temp[i++] = '\\';
                     temp[i] = 0;
                  }

                  strcpy(pHiddenPathName, temp);

               }
               else
               {
                  strcpy(pHiddenPathName, pDefHIDDENPATHNAME);
               }



               // ICON CAPTION

               GetDlgItemText(hDlg, IDM_ICONCAPTION, temp, sizeof(temp));
               while(*temp && *temp<=' ') strcpy(temp, temp+1);
               if(!*temp)
               {
                  strcpy(pIconCaption, pDefICONCAPTION);
               }
               else
               {
                  strcpy(pIconCaption, temp);
               }


               // SAVE STUFF IN PROFILE

               WriteProfileString(pTOILET, _pOPENICONNAME, pOpenIconName);
               WriteProfileString(pTOILET, _pFULLICONNAME, pFullIconName);
               WriteProfileString(pTOILET, _pSHUTICONNAME, pShutIconName);

               WriteProfileString(pTOILET, _pFLUSHINGSOUND, pFlushingSound);
               WriteProfileString(pTOILET, _pDROPSOUND, pDropSound);

               WriteProfileString(pTOILET, _pHIDDENPATHNAME, pHiddenPathName);
               WriteProfileString(pTOILET, _pICONCAPTION, pIconCaption);

               SetWindowText(hMainWnd, pIconCaption);

               EndDialog(hDlg, 0);

               return(TRUE);

         }

         break;

      case WM_DRAWITEM:

         (LPARAM)lpDI = lParam;

         _fmemcpy((LPSTR)&r, (LPSTR)&(lpDI->rcItem), sizeof(r));

         if(lpDI->CtlID == IDM_OPENICON_NAME2 ||
            lpDI->CtlID == IDM_FULLICON_NAME2 ||
            lpDI->CtlID == IDM_SHUTICON_NAME2 ||
            lpDI->CtlID == IDM_WAVEFILE_NAME2 ||
            lpDI->CtlID == IDM_DROPWAVE_NAME2)
         {
            // These buttons are 'drop down' buttons, like combo boxes!

            hBrush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
            FillRect(lpDI->hDC, &r, hBrush);  // erase background
            DeleteObject(hBrush);

            hPen = (HPEN)CreatePen(PS_SOLID, 1, RGB(0,0,0));

            hOldPen = (HPEN)SelectObject(lpDI->hDC, hPen);
            MoveTo(lpDI->hDC, r.left + 1, r.top);
            LineTo(lpDI->hDC, r.right - 2, r.top);
            LineTo(lpDI->hDC, r.right - 1, r.top + 1);
            LineTo(lpDI->hDC, r.right - 1, r.bottom - 2);
            LineTo(lpDI->hDC, r.right - 2, r.bottom - 1);
            LineTo(lpDI->hDC, r.left + 1, r.bottom - 1);
            LineTo(lpDI->hDC, r.left, r.bottom - 2);
            LineTo(lpDI->hDC, r.left, r.top + 1);
            LineTo(lpDI->hDC, r.left + 1, r.top);

            if(lpDI->itemState & ODS_FOCUS)
            {
               MoveTo(lpDI->hDC, r.left + 1, r.top + 1);
               LineTo(lpDI->hDC, r.right - 2, r.top + 1);
               LineTo(lpDI->hDC, r.right - 2, r.bottom - 2);
               LineTo(lpDI->hDC, r.left + 1, r.bottom - 2);
               LineTo(lpDI->hDC, r.left + 1, r.top + 1);
               i = 1;
            }
            else
            {
               i = 0;
            }

            SelectObject(lpDI->hDC, hOldPen);
            DeleteObject(hPen);

            hPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNHIGHLIGHT));
            hPen2 = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNSHADOW));

            for(j=i; j<i+2; j++)
            {
               if(lpDI->itemState & ODS_SELECTED)
               {
                  SelectObject(lpDI->hDC, hPen2);

                  MoveTo(lpDI->hDC, r.left + 1 + j, r.bottom - 2 - j);
                  LineTo(lpDI->hDC, r.left + 1 + j, r.top + 1 + j);
                  LineTo(lpDI->hDC, r.right - 2 - j, r.top + 1 + j);
               }
               else
               {
                  SelectObject(lpDI->hDC, hPen);

                  MoveTo(lpDI->hDC, r.left + 1 + j, r.bottom - 2 - j);
                  LineTo(lpDI->hDC, r.left + 1 + j, r.top + 1 + j);
                  LineTo(lpDI->hDC, r.right - 2 - j, r.top + 1 + j);

                  SelectObject(lpDI->hDC, hPen2);

                  MoveTo(lpDI->hDC, r.right - 2 - j, r.top + 1 + j);
                  LineTo(lpDI->hDC, r.right - 2 - j, r.bottom - 2 - j);
                  LineTo(lpDI->hDC, r.left + 1 + j, r.bottom - 2 - j);
               }

            }

            DeleteObject(hPen);
            DeleteObject(hPen2);
            SelectObject(lpDI->hDC, hOldPen);

            if(lpDI->itemState & ODS_SELECTED)
            {
               r.left += j + 1;
               r.top += j + 1;
            }
            else
            {
               r.left += j;
               r.top += j;
               r.right -= j;
               r.bottom -= j;
            }

            // 'r' is the rectangle into which I center the bitmap.

            hBmp = LoadBitmap(hInst, "DNARROW");

            // bitmap size is 8x8, always!  Center it.

            r.left += (r.right - r.left - 8) / 2;
            r.top  += (r.bottom - r.top - 8) / 2;

            hDC = CreateCompatibleDC(lpDI->hDC);
            hOldBmp = SelectObject(hDC, hBmp);

            SetTextColor(hDC, RGB(0,0,0));
            SetBkColor(hDC, GetSysColor(COLOR_BTNFACE));

            clrOldFg = SetTextColor(lpDI->hDC, RGB(0,0,0));
            clrOldBk = SetBkColor(lpDI->hDC, GetSysColor(COLOR_BTNFACE));

            BitBlt(lpDI->hDC, r.left, r.top, 8, 8, hDC, 0, 0, SRCCOPY);

            SetTextColor(lpDI-hDC, clrOldFg);
            SetBkColor(lpDI->hDC, clrOldBk);

            SelectObject(hDC, hOldBmp);
            DeleteDC(hDC);
            DeleteObject(hBmp);

            return(TRUE);
         }

         i = (r.right - r.left);
         if(i>32)
         {
            r.left += (i - 32) / 2;
            r.right = r.left + 32;
         }

         i = (r.bottom - r.top);
         if(i>32)
         {
            r.top += (i - 32) / 2;
            r.bottom = r.top + 32;
         }

         switch(lpDI->CtlID)
         {
            case IDM_OPENICON:
               DrawIcon(lpDI->hDC, r.left, r.top, hIc1);
               return(TRUE);

            case IDM_FULLICON:
               DrawIcon(lpDI->hDC, r.left, r.top, hIc1a);
               return(TRUE);

            case IDM_SHUTICON:
               DrawIcon(lpDI->hDC, r.left, r.top, hIc2);
               return(TRUE);
         }

         return(FALSE);

   }


   return(NULL);
}




BOOL FAR PASCAL __export UndeleteDlgProc(HWND hDlg, UINT message, 
					 WPARAM wParam, LPARAM lParam)
{
NPSTR pName;
WORD i, j, k;
HCURSOR hCursor;
char tbuf[66];


   switch(message)
   {
      case WM_INITDIALOG:

	 SendDlgItemMessage(hDlg, IDM_LISTBOX, LB_RESETCONTENT, 0, 0);

	 SendDlgItemMessage(hDlg, IDM_LISTBOX, LB_SETHORIZONTALEXTENT,
			    (WPARAM)640, 0);


         // for each logical (non-removable) drive, generate a recursed
         // list of all files contained within the appropriate 'deleted'
         // directory, as specified by 'pHiddenPathName'


         tbuf[0] = 0;
         tbuf[1] = ':';
         lstrcpy(tbuf + 2, pHiddenPathName);

         for(i=0; i<26; i++)
         {
            j = GetDriveType(i);
            if(j==0 || j==1) continue;

            // here I can check some flags as to whether or not to include
            // removable drives in the list of DELETED FILES.
            // 'j==DRIVE_REMOVABLE' for such drives.



//            if(j==DRIVE_REMOVABLE && !drive_flags[i]) continue;
            if(j==DRIVE_REMOVABLE)  // removable drive?? OK, make sure door shut!
            {                       // this method is clean, really clean!
               _asm
               {
                  // PERFORM A 'DISK RESET' TO KICK SYSTEM IN THE TROUSERS

                  mov ax, 0x0d00
                  int 0x21              // performs a DISK RESET!

                  mov ah, 0x36
                  mov dl, BYTE PTR i
                  inc dl
                  int 0x21

                  mov j, ax
               }

               if(j==0xffff) continue;  // NO DISK IN DRIVE!  Ignore it!

               j = DRIVE_REMOVABLE;     // restore original value for below...
            }


            tbuf[0] = (char)('A' + i);

            // only count errors for NON-REMOVABLE disk drives, eh?

            if(PutDirTreeInListBox(tbuf, GetDlgItem(hDlg, IDM_LISTBOX)) &&
               j!=DRIVE_REMOVABLE)
            {
               strcpy(temp, "?Error reading \"");
               lstrcat(temp, tbuf);
               lstrcat(temp, "\"");

               MessageBox(hDlg, temp, pTOILET_ERROR, MB_OK | MB_ICONHAND);
            }
         }

	 return(TRUE);



      case WM_COMMAND:

	 switch(wParam)
	 {
	    case IDCANCEL:

	       EndDialog(hDlg, 0);
	       return(NULL);



	    case IDM_RESTOREALL:

	       hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

               j = (WORD)SendDlgItemMessage(hDlg, IDM_LISTBOX, LB_GETCOUNT, 0, 0);
               if(j==LB_ERR || !j) break;

               for(i=0; i<j; i++)
	       {
                    /* 'un-delete' all of the files */

                  SendDlgItemMessage(hDlg, IDM_LISTBOX, LB_GETTEXT,
                                     (WPARAM)i, (LPARAM)((LPSTR)tbuf));

                  My_restore(tbuf);

               }

	       nNames = 0;

	       SetCursor(hCursor);

	       SendDlgItemMessage(hDlg, IDM_LISTBOX, LB_RESETCONTENT, 0, 0);

	       return(NULL);



            case IDM_RESTORE:
            case IDM_LISTBOX:
            case IDM_PURGE:


               if(wParam==IDM_LISTBOX && HIWORD(lParam)!=LBN_DBLCLK)
	       {
		  break;
	       }

	       j = (WORD)SendDlgItemMessage(hDlg, IDM_LISTBOX,
					    LB_GETCURSEL, 0, 0);

	       if(j==LB_ERR ||
		  SendDlgItemMessage(hDlg, IDM_LISTBOX, LB_GETTEXT,
				     (WPARAM)j, (LPARAM)((LPSTR)pNameBuf))
		  == LB_ERR )
	       {
		  MessageBeep(MB_ICONHAND);

		  return(NULL);
	       }


               if(wParam==IDM_PURGE)
               {
                  if(My_purge(pNameBuf))
                  {
                     MessageBox(hDlg,
                                "?Unable to purge item - internal problem",
                                pTOILET_ERROR, MB_OK | MB_ICONHAND);
                  }
                  else
                  {
                     SendDlgItemMessage(hDlg, IDM_LISTBOX, LB_DELETESTRING,
					(WPARAM)j, 0);
                  }
               }
               else
               {
                  if(My_restore(pNameBuf))
                  {
                     MessageBox(hDlg,
                                "?Unable to restore item - internal problem",
                                pTOILET_ERROR, MB_OK | MB_ICONHAND);
                  }
                  else
                  {
                     SendDlgItemMessage(hDlg, IDM_LISTBOX, LB_DELETESTRING,
					(WPARAM)j, 0);
                  }
               }

               // here is where I make sure that 'nNames' represents the
               // number of files *ACTUALLY* 'in the toilet' (as seen in
               // the dialog box) at this moment in time.

               j = (WORD)SendDlgItemMessage(hDlg, IDM_LISTBOX, LB_GETCOUNT, 0, 0);
               if(j!=LB_ERR)
               {
                  if((nNames && !j) || (!nNames && j))
                  {
                     InvalidateRect(hMainWnd, NULL, TRUE);
                     UpdateWindow(hMainWnd);
                  }

                  nNames = j;
               }

               return(NULL);


         }

         break;



   }

   return(NULL);

}





/***************************************************************************/
/*                                                                         */
/*     LOW-LEVEL FUNCTIONS WHICH PROCESS THE 'DELETED' FILES DIRECTLY      */
/*                                                                         */
/*                                                                         */
/***************************************************************************/


BOOL FAR PASCAL My_hide(LPCSTR lpPath)
{
static char buf[128], buf2[66];
static OFSTRUCT ofstr;
LPSTR lp1;
WORD w;


   if(OpenFile(lpPath, &ofstr, OF_EXIST | OF_READ | OF_SHARE_DENY_NONE)
      ==HFILE_ERROR)
   {
      lstrcpy(buf, "Unable to delete \"");
      lstrcat(buf, lpPath);
      lstrcat(buf, "\"!");

      MessageBox(hMainWnd, buf, pTOILET_ERROR, MB_OK | MB_ICONHAND);
      return(TRUE);
   }

   if(ofstr.szPathName[1]==':' &&
      ((ofstr.szPathName[0]>='A' && ofstr.szPathName[0]<='Z') ||
       (ofstr.szPathName[0]>='a' && ofstr.szPathName[0]<='z')))
   {
      w = ofstr.szPathName[0];

      if(w >= 'a') w -= ('a' - 'A');

      drive_flags[w - 'A'] |= 1;   // I deleted a file on this drive!!!
   }


   lstrcpy(buf, ofstr.szPathName);  // a somewhat 'fully qualified' path

   lp1 = _fstrchr(buf, '\\');      // find 'first' backslash

   if(lp1)    // we need to jam the 'hidden path' into the file's path
   {
      w = strlen(pHiddenPathName);
      if(w==0)
      {
         lstrcpy(pHiddenPathName, pDefHIDDENPATHNAME);
         w = strlen(pHiddenPathName);
      }

      if(w>1)  // that is, it's not a '\'
      {
            // make space for the 'hidden' path (without the trailing '\\')

         _fmemmove(lp1 + w - 1, lp1, lstrlen(lp1) + 1);

            // stuff the bloomin' thing in the hole I created

         _fmemcpy(lp1, pHiddenPathName, w - 1);
      }

      lp1 = _fstrrchr(buf, '\\');      // find 'last' backslash

      if(lp1)
      {
         *lp1 = 0;
      }
   }

   if(!lp1 || My_mkdir(buf))
   {
      lstrcpy(buf, "Unable to delete (mkdir) \"");
      lstrcat(buf, lpPath);
      lstrcat(buf, "\"!");

      MessageBox(hMainWnd, buf, pTOILET_ERROR, MB_OK | MB_ICONHAND);
      return(TRUE);
   }

   *lp1 = '\\';

   lstrcpy(buf2, lpPath);
   unlink(buf);                   // in case the 'deleted' file already exists...

   if(rename(buf2, buf))          // move file!!
   {
      lstrcpy(buf, "Unable to delete (move) \"");
      lstrcat(buf, lpPath);
      lstrcat(buf, "\"!");

      MessageBox(hMainWnd, buf, pTOILET_ERROR, MB_OK | MB_ICONHAND);
      return(TRUE);
   }

   return(FALSE);                   // DONE!!
}



BOOL FAR PASCAL My_restore(LPCSTR lpPath)
{
static char buf[128], buf2[66];
static OFSTRUCT ofstr;
LPSTR lp1;
WORD w;


   if(OpenFile(lpPath, &ofstr, OF_EXIST | OF_READ | OF_SHARE_DENY_NONE)
      !=HFILE_ERROR)
   {
      lstrcpy(buf, "The file \"");
      lstrcat(buf, lpPath);
      lstrcat(buf, "\" exists!  Overwrite with old?");

      if(MessageBox(hMainWnd, buf, pTOILET_ERROR,
                    MB_YESNO | MB_ICONHAND)==IDYES)
      {
         lstrcpy(buf, lpPath);
         unlink(buf);
      }
      else
      {
         return(TRUE);
      }
   }


   lstrcpy(buf, ofstr.szPathName);  // a somewhat 'fully qualified' path

   lp1 = _fstrchr(buf, '\\');      // find 'first' backslash

   if(lp1)    // we need to jam the 'hidden path' into the file's path
   {
      w = strlen(pHiddenPathName);
      if(w==0)
      {
         lstrcpy(pHiddenPathName, pDefHIDDENPATHNAME);
         w = strlen(pHiddenPathName);
      }

      if(w>1)  // that is, it's not a '\'
      {
            // make space for the 'hidden' path (without the trailing '\\')

         _fmemmove(lp1 + w - 1, lp1, lstrlen(lp1) + 1);

            // stuff the bloomin' thing in the hole I created

         _fmemcpy(lp1, pHiddenPathName, w - 1);
      }

   }

   lstrcpy(buf2, lpPath);

   if(!lp1 || rename(buf, buf2))            // move file!!
   {
      lstrcpy(buf, "Unable to restore (move) \"");
      lstrcat(buf, lpPath);
      lstrcat(buf, "\"!");

      MessageBox(hMainWnd, buf, pTOILET_ERROR, MB_OK | MB_ICONHAND);
      return(TRUE);
   }


   // now for some *REAL* fun!  attempt to remove all of the paths down
   // to the ROOT directory (if there are no files in each directory it
   // will work just fine - otherwise it fails, and I bail out).

   do
   {
      lp1 = _fstrrchr(buf, '\\');
      if(lp1 && lp1>buf && *(lp1 - 1)!=':' && *(lp1 - 1)!='\\')
      {
         *lp1 = 0;
         if(_rmdir(buf)) break;
      }
      else
      {
         break;
      }

   } while(lp1);


   return(FALSE);                   // DONE!!
}



BOOL FAR PASCAL My_purge(LPCSTR lpPath)
{
static char buf[128];
static OFSTRUCT ofstr;
LPSTR lp1;
WORD w;



   lstrcpy(buf, lpPath);

   lp1 = _fstrchr(buf, '\\');      // find 'first' backslash

   if(lp1)    // we need to jam the 'hidden path' into the file's path
   {
      w = strlen(pHiddenPathName);
      if(w==0)
      {
         lstrcpy(pHiddenPathName, pDefHIDDENPATHNAME);
         w = strlen(pHiddenPathName);
      }

      if(w>1)  // that is, it's not a '\'
      {
            // make space for the 'hidden' path (without the trailing '\\')

         _fmemmove(lp1 + w - 1, lp1, lstrlen(lp1) + 1);

            // stuff the bloomin' thing in the hole I created

         _fmemcpy(lp1, pHiddenPathName, w - 1);
      }

   }

   if(!lp1 || unlink(buf))            // purge (deleted) file
   {
      lstrcpy(buf, "Unable to purge \"");
      lstrcat(buf, lpPath);
      lstrcat(buf, "\"!");

      MessageBox(hMainWnd, buf, pTOILET_ERROR, MB_OK | MB_ICONHAND);
      return(TRUE);
   }


   // now for some *REAL* fun!  attempt to remove all of the paths down
   // to the 'ROOT' directory (if there are no files in each directory it
   // will work just fine - otherwise it fails, and I bail out).

   do
   {
      lp1 = _fstrrchr(buf, '\\');
      if(lp1 && lp1>buf && *(lp1 - 1)!=':' && *(lp1 - 1)!='\\')
      {
         *lp1 = 0;
         if(_rmdir(buf)) break;
      }
      else
      {
         break;
      }

   } while(lp1);


   return(FALSE);                   // DONE!!
}


BOOL FAR PASCAL My_mkdir(LPCSTR lpPath)
{
LPSTR lp1;
WORD w;
char buf[66];
static char buf2[66];
static OFSTRUCT ofstr;



   lstrcpy(buf, lpPath);

   w = lstrlen(buf);

   if(buf[w - 1]=='\\')
   {
      if(buf[w - 1]==':' && w>=1)
      {
         lstrcpy(buf + w, "NUL");

         if(OpenFile(buf, &ofstr, OF_EXIST | OF_READ | OF_SHARE_DENY_NONE)
            ==HFILE_ERROR)
         {
            return(TRUE);  // path does not exist!!
         }
         else
         {
            return(FALSE); // path exists!
         }
      }
      else if(w==1 && *buf=='\\')  // network 'root'
      {
         return(FALSE);            // path 'should' exist...
      }
      else
         return(TRUE);     // not legal - no device


      buf[--w] = 0;
   }

   lp1 = _fstrrchr(buf, '\\');  // find the last backslash in the path

   if(lp1)
   {
      *lp1 = 0;
      My_mkdir(buf);            // recurse to build path up to this point
      *lp1 = '\\';
   }

   lstrcpy(buf + w, "\\NUL");

   if(OpenFile(buf, &ofstr, OF_EXIST | OF_READ | OF_SHARE_DENY_NONE)
      ==HFILE_ERROR)
   {
      buf[w] = 0;

      lstrcpy(buf2, buf);

      if(_mkdir(buf2)) return(TRUE);


      // ensure that the 'root' (DELETED.SYS) path is hidden!

      if(buf2[1]==':')
      {
         lstrcpy(buf2 + 3, pHiddenPathName);

         _dos_setfileattr(buf2, _A_SUBDIR | _A_HIDDEN);
      }
   }

   return(FALSE);

}


// given a path 'lpPath' and a LISTBOX control 'hWnd' add the entire
// directory tree (file names only; no directories) to the listbox.
// Assume that the listbox is already empty, and sorted flags are
// already assigned as desired by the calling function.

// As an extra added bonus, if 'hWnd' is NULL I increment 'nNames' instead.


BOOL FAR PASCAL PutDirTreeInListBox(LPSTR lpPath, HWND hWnd)
{
WORD wStart, wEnd, w;
struct _find_t ff;
char tbuf[66];



   // first:  make copy of 'lpPath', and ensure it ends in '\\'

   if(!lpPath || !*lpPath) return(TRUE);  // this is a safety valve

   lstrcpy(tbuf, lpPath);

   if(tbuf[lstrlen(tbuf) - 1]!='\\') lstrcat(tbuf,"\\");


   // next part:  see where 'pHiddenPathName' starts and ends

   wEnd = strlen(pHiddenPathName);

   for(wStart=0; tbuf[wStart]; wStart++)
   {
      if(!_fstrnicmp(tbuf + wStart, pHiddenPathName, wEnd))
      {
         wEnd += wStart - 1;
         break;
      }
   }

   if(!tbuf[wStart])   // I *DID NOT* find it!
   {
      wStart = 0;
      wEnd = 0;
   }


   // 3rd:  main loop!  Find all non-subdirectory files and place the
   //       full path name (minus the section between 'wStart' and 'wEnd')
   //       into the list box.

   lstrcat(tbuf,"*.*");

   if(w = My_findfirst(tbuf, (~_A_VOLID) & 0xff, &ff))
   {
      // no files found - just return

      return(w!=0x12 && w!=2 && w!=3);  // 'no more files' is ok
                              // also 'file not found' and 'path not found'
   }

   do
   {
      if(ff.attrib & _A_SUBDIR)   // this thing is a sub-directory
      {
         if(!_fstrcmp(ff.name,".") || !_fstrcmp(ff.name,".."))
         {
            continue;
         }

         // let's recurse, dudes!  First, build a path, then append the
         // current file name (a directory, ok?).

         lstrcpy(tbuf, lpPath);

         if(tbuf[lstrlen(tbuf) - 1]!='\\') lstrcat(tbuf,"\\");

         lstrcat(tbuf, ff.name);

         if(PutDirTreeInListBox(tbuf, hWnd)) return(TRUE);
                            // return error if it fails, otherwise continue!!
      }
      else
      {
         // build the full path name for this file, and delete it

         lstrcpy(tbuf, lpPath);

         if(tbuf[lstrlen(tbuf) - 1]!='\\') lstrcat(tbuf,"\\");

         lstrcat(tbuf, ff.name);


         // next, use 'wStart' and 'wEnd' to trim out 'pHiddenPathName'

         if(wStart!=wEnd)
         {
            lstrcpy(tbuf + wStart, tbuf + wEnd);
         }

         // 'tbuf' contains the correct file name.  NOW, post the string to
         // the listbox control!

         if(hWnd)
            SendMessage(hWnd, LB_ADDSTRING, NULL, (LPARAM)((LPSTR)tbuf));
         else
            nNames++;  // increment 'nNames' when 'hWnd' is NULL!!!

         if(*tbuf>='A' && *tbuf<='Z' && tbuf[1]==':')
         {
            drive_flags[*tbuf - 'A'] |= 1;  // drive has DELETED files on it!
         }
      }

   } while(!My_findnext(&ff));


   return(FALSE);

}



// this next function is *VERY* dangerous!  it will purge all files from
// a node OUTWARD, whether or not they are readonly, where SS!=DS.
// all 'child' sub-directories are removed, but the directory specified
// in the path parameter 'lpPath' is NOT removed!

BOOL FAR PASCAL MyPurgeNode(LPSTR lpPath)
{
static WORD w;
static char sbuf[66];
struct _find_t ff;
char tbuf[66];


   if(!lpPath || !*lpPath) return(TRUE);  // this is a safety valve

   lstrcpy(tbuf, lpPath);

   if(tbuf[lstrlen(tbuf) - 1]!='\\') lstrcat(tbuf,"\\");

   lstrcat(tbuf,"*.*");

   if(w = My_findfirst(tbuf, (~_A_VOLID) & 0xff, &ff))
   {
      // no files found - just return

      return(w!=0x12 && w!=2 && w!=3);  // 'no more files' is ok
                              // also 'file not found' and 'path not found'
   }

   do
   {
      if(ff.attrib & _A_SUBDIR)   // this thing is a sub-directory
      {
         if(!_fstrcmp(ff.name,".") || !_fstrcmp(ff.name,".."))
         {
            continue;
         }

         // let's recurse, dudes!  First, build a path, then append the
         // current file name (a directory, ok?).

         lstrcpy(tbuf, lpPath);

         if(tbuf[lstrlen(tbuf) - 1]!='\\') lstrcat(tbuf,"\\");

         lstrcat(tbuf, ff.name);

         if(MyPurgeNode(tbuf)) return(TRUE);  // return error if it fails,
                                              // otherwise continue!!
         lstrcpy(sbuf, tbuf);

         _dos_getfileattr(sbuf, &w);

         w &= (~_A_RDONLY) & (~_A_HIDDEN) & (~_A_SYSTEM);

         _dos_setfileattr(sbuf, w);   // reset attribs without the 'READONLY'

         if(_rmdir(sbuf)) return(TRUE);       // if this fails, bail!
      }
      else
      {
         // build the full path name for this file, and delete it

         lstrcpy(sbuf, lpPath);

         if(sbuf[lstrlen(sbuf) - 1]!='\\') lstrcat(sbuf,"\\");

         lstrcat(sbuf, ff.name);

         _dos_getfileattr(sbuf, &w);

         w &= (~_A_RDONLY) & (~_A_HIDDEN) & (~_A_SYSTEM);

         _dos_setfileattr(sbuf, w);   // reset attribs without the 'READONLY'

         if(unlink(sbuf)) return(TRUE);       // if this fails, bail!
      }

   } while(!My_findnext(&ff));


   return(FALSE);

}



BOOL FAR PASCAL My_findfirst(LPSTR lpPath, UINT uiAttrib,
                             struct _find_t FAR *lpFF)
{
LPSTR lpOldDTA;
BOOL rval;


   // STEP 1:  record original DTA address

   _asm
   {
      mov ax, 0x2f00
      call DOS3Call

      mov WORD PTR lpOldDTA + 2, es
      mov WORD PTR lpOldDTA, bx
   }

   if(lpOldDTA!=(LPSTR)lpFF)    // are they equal?  do I need to reset DTA?
   {
      _asm                      // assign DTA to 'lpFF'
      {
         mov ax, 0x1a00
         push ds
         lds dx, lpFF
         call DOS3Call

         pop ds
      }
   }


   // perform a DOS FINDFIRST (int 21H function 4eH)

   _asm
   {
      mov ax, 0x4e00
      mov cx, uiAttrib
      push ds
      lds dx, lpPath

      call DOS3Call

      pop ds
      jc was_error
      mov ax, 0

was_error:

      mov rval, ax          // stores the error code (or 0 if successful)

   }


   // last step:  restore DTA (if necessary)

   if(lpOldDTA!=(LPSTR)lpFF)    // are they equal?  do I need to restore DTA?
   {
      _asm                      // assign DTA to 'lpOldDTA'
      {
         mov ax, 0x1a00
         push ds
         lds dx, lpOldDTA
         call DOS3Call

         pop ds
      }
   }

   return(rval);

}


BOOL FAR PASCAL My_findnext(struct _find_t FAR *lpFF)
{
LPSTR lpOldDTA;
BOOL rval;


   // STEP 1:  record original DTA address

   _asm
   {
      mov ax, 0x2f00
      call DOS3Call

      mov WORD PTR lpOldDTA + 2, es
      mov WORD PTR lpOldDTA, bx
   }

   if(lpOldDTA!=(LPSTR)lpFF)    // are they equal?  do I need to reset DTA?
   {
      _asm                      // assign DTA to 'lpFF'
      {
         mov ax, 0x1a00
         push ds
         lds dx, lpFF
         call DOS3Call

         pop ds
      }
   }


   // perform a DOS FINDNEXT (int 21H function 4fH)

   _asm
   {
      mov ax, 0x4f00

      call DOS3Call

      jc was_error
      mov ax, 0

was_error:

      mov rval, ax          // stores the error code (or 0 if successful)

   }


   // last step:  restore DTA (if necessary)

   if(lpOldDTA!=(LPSTR)lpFF)    // are they equal?  do I need to restore DTA?
   {
      _asm                      // assign DTA to 'lpOldDTA'
      {
         mov ax, 0x1a00
         push ds
         lds dx, lpOldDTA
         call DOS3Call

         pop ds
      }
   }

   return(rval);

}


/***************************************************************************/
/*                                                                         */
/*     ICON RESOURCES - namely '.ICO' files and creating an 'HICON'        */
/*                                                                         */
/*                                                                         */
/***************************************************************************/




HICON FAR PASCAL MyCreateIcon(LPSTR lpIconName, LPSTR FAR *lplpIcon)
{
WORD wCount;
HFILE hFile;
OFSTRUCT ofstr;



   while(*lpIconName && *lpIconName<=' ') lstrcpy(lpIconName, lpIconName+1);

   if(!*lpIconName) return(NULL);  // this is a 'just in case' check

   hFile = OpenFile(lpIconName, &ofstr, OF_EXIST | OF_SHARE_DENY_NONE);
   if(hFile==HFILE_ERROR)
   {
      hFile = OpenFile(lpIconName, &ofstr, OF_EXIST | OF_SHARE_COMPAT);
      if(hFile==HFILE_ERROR) 
      {
         MessageBox(NULL, lpIconName,
                    "** FILE OPEN ERROR **",
                    MB_OK | MB_ICONHAND | MB_SYSTEMMODAL);


	 return(NULL);
      }
   }


   wCount = (WORD)ExtractIcon(hInst, ofstr.szPathName, -1);

   if(!wCount)
   {
      return(NULL);
   }
   else if(wCount==1)
   {
      return(ExtractIcon(hInst, ofstr.szPathName, 0));
   }
   else
   {
      // HERE I CAN DETERMINE WHETHER OR NOT TO CHOOSE A PARTICULAR
      // ICON FROM THE FILE - IN THIS CASE, PICK THE '1st' ONE ONLY!

      return(ExtractIcon(hInst, ofstr.szPathName, 0));
   }

}



//
//HICON FAR PASCAL OldMyCreateIcon(LPSTR lpIconName, LPSTR FAR *lplpIcon)
//{
//HFILE hFile;
//HICON hRval;
//HDC hDC;
//HBITMAP hDIB;
//LPICONHEADER lpICO;
//OFSTRUCT ofstr;
//DWORD dwSize, dwPtr;
//int i1, i2, j;
//char c;
//WORD wItem, wPlanes, wXBytes;
//LPSTR lp1, lp2, lp3, lp4;
//BITMAP bmp;
//
//
//
//   while(*lpIconName && *lpIconName<=' ') lstrcpy(lpIconName, lpIconName+1);
//
//   if(!*lpIconName) return(NULL);  // this is a 'just in case' check
//
//   hFile = OpenFile(lpIconName, &ofstr, OF_READ | OF_SHARE_DENY_NONE);
//   if(hFile==HFILE_ERROR)
//   {
//
//
//      MessageBox(NULL, lpIconName,
//                       "** FILE OPEN ERROR **",
//                       MB_OK | MB_ICONHAND | MB_SYSTEMMODAL);
//
//      hFile = OpenFile(lpIconName, &ofstr, OF_READ | OF_SHARE_COMPAT);
//      if(hFile==HFILE_ERROR)
//      {
//         MessageBox(NULL, "?Unable to open file (2nd time)",
//                       "** ICON OPEN ERROR TRACKING **",
//                       MB_OK | MB_ICONHAND | MB_SYSTEMMODAL);
//
//         return(NULL);
//      }
//   }
//
//   dwSize = _llseek(hFile, 0, 2);  // positions file to end, gets size!
//
//   if(!dwSize || dwSize==HFILE_ERROR)
//   {
//      MessageBox(NULL, "?Unable to get file size (is it zero?)",
//                       "** ICON OPEN ERROR TRACKING **",
//                       MB_OK | MB_ICONHAND | MB_SYSTEMMODAL);
//
//      _lclose(hFile);
//      return(NULL);
//   }
//
//
//   _llseek(hFile, 0, 0);           // back to the beginning!
//
//
//   *lplpIcon = GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT, dwSize);
//   if(!*lplpIcon)
//   {
//      MessageBox(NULL, "?Unable to allocate global memory for icon",
//                       "** ICON OPEN ERROR TRACKING **",
//                       MB_OK | MB_ICONHAND | MB_SYSTEMMODAL);
//
//      _lclose(hFile);
//      return(NULL);
//   }
//
//   dwPtr = 0;
//
//   do
//   {
//      if((dwSize - dwPtr)>0x8000L)
//      {
//         if(_lread(hFile, (char _huge *)(*lplpIcon) + dwPtr, 0x8000)
//            !=0x8000L)
//         {
//            MessageBox(NULL, "?Reading file was not 0x8000L bytes!",
//                       "** ICON OPEN ERROR TRACKING **",
//                       MB_OK | MB_ICONHAND | MB_SYSTEMMODAL);
//
//            GlobalFreePtr(*lplpIcon);
//            *lplpIcon = NULL;
//            _lclose(hFile);
//            return(NULL);
//         }
//
//         dwPtr += 0x8000L;
//      }
//      else
//      {
//         if(_lread(hFile, (char _huge *)(*lplpIcon) + dwPtr,
//                   (WORD)(dwSize - dwPtr))!=(WORD)(dwSize - dwPtr))
//         {
//            MessageBox(NULL, "?Reading file was not correct # of bytes!",
//                       "** ICON OPEN ERROR TRACKING **",
//                       MB_OK | MB_ICONHAND | MB_SYSTEMMODAL);
//
//            GlobalFreePtr(*lplpIcon);
//            *lplpIcon = NULL;
//
//            _lclose(hFile);
//            return(NULL);
//         }
//
//         dwPtr = dwSize;
//      }
//
//   } while(dwPtr<dwSize);
//
//   _lclose(hFile);
//   hFile = HFILE_ERROR;
//
//
//   /* now that I've read in the file, get the icon info out of it!! */
//
//   lpICO = (LPICONHEADER)*lplpIcon;
//
//
////   if(lpICO->idReserved!=0 || lpICO->idType!=1 ||
////      lpICO->idCount==0)
////   {
////      GlobalFreePtr(*lplpIcon);
////      *lplpIcon = NULL;
////      return(NULL);
////   }
//
//
//   // for now, pick only the first one in the list... later I check the
//   // video type and pick the 'best fit', but for now this is good enough
//
//   wItem = 0;
//
//   if(lpICO->idEntries[wItem].bColorCount==2)       wPlanes=1;
//   else if(lpICO->idEntries[wItem].bColorCount==8)  wPlanes=3;
//   else if(lpICO->idEntries[wItem].bColorCount==16) wPlanes=4;
//   else                                                 wPlanes=1; // default
//
//   wXBytes = (lpICO->idEntries[wItem].bWidth *
//              lpICO->idEntries[wItem].bHeight * (wPlanes==1?1:4)) / 8;
//
//
//
//   lp1 = (*lplpIcon + lpICO->idEntries[wItem].dwImageOffset);
//
//   // for now, assume 32x32 icons!
//
//   if(wPlanes==4)
//   {
//      lp2 = ((LPICONIMAGE16_32x32)lp1)->icAND;
//      lp3 = ((LPICONIMAGE16_32x32)lp1)->icXOR;
//   }
//   else if(wPlanes==3)
//   {
//      lp2 = ((LPICONIMAGE8_32x32)lp1)->icAND;
//      lp3 = ((LPICONIMAGE8_32x32)lp1)->icXOR;
//   }
//   else // assume 1-plane
//   {
//      lp2 = ((LPICONIMAGE2_32x32)lp1)->icAND;
//      lp3 = ((LPICONIMAGE2_32x32)lp1)->icXOR;
//   }
//
//   // ok kiddies - now it gets worse!  obtain a device context for the
//   // current display by grabbing the DC for the Desktop Window!
//
//   hDC = GetDC(GetDesktopWindow());
//
//   // next, create a device DEPENDENT bitmap for this device context
//   // using one of our favorite functions, CreateDIBitmap()!
//
//
//   hDIB = NULL;
//
//   if(hDC)
//   {
//      ((BITMAPINFOHEADER FAR *)lp1)->biHeight /= 2;  // this is twice as large
//                                                     // as it should be!!
//
//      hDIB = CreateDIBitmap(hDC, (BITMAPINFOHEADER FAR *)lp1, CBM_INIT,
//                            lp3, lp1, DIB_RGB_COLORS);
//
//
//
//      // now, obtain the bits for this device-dependent bitmap in a
//      // global memory block.  Cowabunga!
//
//      if(hDIB)
//      {
//         GetObject(hDIB, sizeof(BITMAP), (void FAR *)&bmp);
//
//
//         lp4 = GlobalAllocPtr(GMEM_MOVEABLE | GMEM_ZEROINIT,
//                              (DWORD)bmp.bmHeight * bmp.bmWidthBytes
//                                                  * bmp.bmPlanes);
//      }
//   }
//   else
//   {
//      hDIB = NULL;
//   }
//
//
//   // finally, obtain the bits for the device-dependent bitmap and
//   // pass all of the info needed to 'CreateIcon()'
//
//   if(hDIB && lp4)
//   {
//      GetBitmapBits(hDIB, (DWORD)bmp.bmHeight * bmp.bmWidthBytes
//                                              * bmp.bmPlanes, lp4);
//
//
//      // now for some more fun - because the XOR mask is 'upside down' (DIB)
//      // we must invert it.  For this we need to know how many rows/cols
//      // to deal with.  That should be easy, using the 'bmp' fields.
//
//
//      for(i1=0, i2=bmp.bmHeight - 1; i2>i1; i1++, i2--)
//      {
//         for(j=0; j<(bmp.bmWidth/8); j++)
//         {
//            c = lp2[i1 * (bmp.bmWidth/8) + j];
//
//            lp2[i1 * (bmp.bmWidth/8) + j] = lp2[i2 * (bmp.bmWidth/8) + j];
//
//            lp2[i2 * (bmp.bmWidth/8) + j] = c;
//         }
//      }
//
//      hRval = CreateIcon(hInst,lpICO->idEntries[wItem].bWidth,
//                               lpICO->idEntries[wItem].bHeight,
//                         wPlanes,  // # of planes in color (XOR) mask
//                         1,        // 1 bit per pixel (per plane)
//                         (void FAR *)lp2, (void FAR *)lp4);
//
//
//      GlobalFreePtr(lp4);
//   }
//   else
//   {
//      hRval = NULL;
//   }
//
//   if(hDIB) DeleteObject(hDIB);                  // I am now done with it!
//   if(hDC)  ReleaseDC(GetDesktopWindow(), hDC);  // and this, too!
//
//
//
//   if(!hRval)
//   {
//      MessageBox(NULL, "?Error creating the 'ICON' handle!",
//                       "** ICON OPEN ERROR TRACKING **",
//                       MB_OK | MB_ICONHAND | MB_SYSTEMMODAL);
//      GlobalFreePtr(*lplpIcon);
//      *lplpIcon = NULL;
//      return(NULL);
//   }
//   else
//      return(hRval);
//
//}
//
