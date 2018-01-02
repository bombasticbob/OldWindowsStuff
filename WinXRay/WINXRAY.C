/***************************************************************************/
/*                                                                         */
/*     WINXRAY.C - (c) 1992-94 by R. E. Frazier - all rights reserved      */
/*                                                                         */
/* Application to generate system information lists into a listbox, with   */
/* optional copy of information to the clipboard in TEXT, CSV, BIF format. */
/*                                                                         */
/***************************************************************************/
/*                                                                         */
/*               Portions of the code were taken from WINCMD               */
/*         Copyright 1992,93 by R. E. Frazier - all rights reserved        */
/*                                                                         */
/***************************************************************************/
/*                                                                         */
/*  Portions of this code also taken from WDBUTIL and WMTHREAD 'DLL's      */
/* Copyright 1990-93 by Stewart~Frazier Tools, Inc. - all rights reserved  */
/*                                                                         */
/***************************************************************************/

#define STRICT
#define WIN31
#include "windows.h"
#include "windowsx.h"
#include "toolhelp.h"
#include "stdio.h"
#include "stdlib.h"
#include "memory.h"
#include "string.h"
#include "hugemem.h"
#include "winxray.h"





		   /**********************************/
		   /** INTERNAL FUNCTION PROTOTYPES **/
		   /**********************************/


int PASCAL WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,
		   int nCmdShow);

BOOL InitApplication(HINSTANCE hInstance);

BOOL InitInstance(HANDLE hInstance, int nCmdShow);

long CALLBACK MainWndProc(HWND hWnd, UINT message, WORD wParam, LONG lParam);

BOOL CALLBACK __export About(HWND hDlg, UINT message, WORD wParam, LONG lParam);

BOOL CALLBACK __export InputBoxProc(HWND hDlg, UINT message, WORD wParam, LONG lParam);


			/**********************/
			/** GLOBAL VARIABLES **/
			/**********************/



char pClassName[]="WinXRayWindowClass";
char pMenuName[]="MainMenu";
char pHelpFileName[]="WinXRay.HLP";



char pq[]="???";
char *pOpenMode[8]={"Rd","Wrt","R/W",pq,"RWN",pq,pq,pq};
char *pShareMode[8]={"Comp","Excl","NoWr","NoRd","Shr",pq,pq,pq};


char buffer[256];

HINSTANCE hInst;
HWND hMainWnd;
WORD wIGROUP;


extern WORD PASCAL wCode32Alias, PASCAL wData32Alias;
extern WORD PASCAL wCallGateSeg;
extern WORD PASCAL wCode32Seg; // , PASCAL wData32Seg;


		/*** EXTERNALLY DEFINED PROC ADDRESSES ***/

HMODULE hToolHelp=(HMODULE)NULL;
HMODULE hKernel  =(HMODULE)NULL;

BOOL (FAR PASCAL *lpGlobalFirst)(LPGLOBALENTRY lpGlobal, WORD wFlags)=NULL;
BOOL (FAR PASCAL *lpGlobalNext)(LPGLOBALENTRY lpGlobal, WORD wFlags) =NULL;
BOOL (FAR PASCAL *lpMemManInfo)(LPMEMMANINFO lpInfo)                 =NULL;
BOOL (FAR PASCAL *lpModuleFirst)(LPMODULEENTRY lpModule)             =NULL;
BOOL (FAR PASCAL *lpModuleNext)(LPMODULEENTRY lpModule)              =NULL;
HTASK (FAR PASCAL *lpTaskFindHandle)(LPTASKENTRY lpTask, HTASK hTask)=NULL;
HMODULE (FAR PASCAL *lpModuleFindHandle)(LPMODULEENTRY lpModule,
					 HMODULE hModule)            =NULL;

BOOL (FAR PASCAL *lpIsTask)(HTASK hTask)                             =NULL;
BOOL (FAR PASCAL *lpTaskFirst)(LPTASKENTRY lpTask)                   =NULL;
BOOL (FAR PASCAL *lpTaskNext)(LPTASKENTRY lpTask)                    =NULL;

DWORD (FAR PASCAL *lpGlobalMasterHandle)(void)                       =NULL;
DWORD (FAR PASCAL *lpGlobalHandleNoRIP)(WORD wSeg)                   =NULL;



			  /*******************/
			  /** THE FUNCTIONS **/
			  /*******************/

int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrev, LPSTR lpCmdLine,
		   int nCmdShow)
{
MSG msg;


   if(!hPrev)
   {
      if(!InitApplication(hInstance)) return(FALSE);
   }

   if(!InitInstance(hInstance, nCmdShow)) return(FALSE);


   while(GetMessage(&msg,NULL,NULL,NULL))
   {
      if(!IsDialogMessage(hMainWnd, &msg))
      {
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      }
   }

   if(wCode32Seg)
   {
      GlobalPageUnlock((HGLOBAL)LOWORD(GlobalHandle((HGLOBAL)wCode32Seg)));
      GlobalUnfix((HGLOBAL)LOWORD(GlobalHandle((HGLOBAL)wCode32Seg)));
   }

   if(wCode32Alias) MyFreeSelector(wCode32Alias);
   if(wData32Alias) MyFreeSelector(wData32Alias);
   if(wCallGateSeg) MyFreeSelector(wCallGateSeg);

   wCode32Seg   = 0;        // in case of re-entry or restart, zero these
   wCode32Alias = 0;        // selectors - forces it to re-allocate them!
   wData32Alias = 0;
   wCallGateSeg = 0;

   return(msg.wParam);
}


BOOL InitApplication(HINSTANCE hInstance)
{
WNDCLASS  wc;

    wc.style = NULL;
    wc.lpfnWndProc = (WNDPROC)MainWndProc;

    wc.cbClsExtra = 0;
    wc.cbWndExtra = sizeof(WORD);
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, "WinXRayIcon");
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = GetStockObject(WHITE_BRUSH); 
    wc.lpszMenuName =  pMenuName;
    wc.lpszClassName = pClassName;

    return (RegisterClass(&wc));

}


BOOL InitInstance(HANDLE hInstance, int nCmdShow)
{
OFSTRUCT ofstr;
union {
    DWORD dw;
    struct {
      char win_major, win_minor;
      char dos_minor, dos_major;
    } ver;
  } uVersion;


    hInst = hInstance;

    hKernel = GetModuleHandle("KERNEL");      /* it's ALWAYS loaded!! */
    uVersion.dw = GetVersion();         /* the current windows/dos version */

    if(uVersion.ver.win_major<3 ||
       (uVersion.ver.win_major==3 && uVersion.ver.win_minor<0x0a))
    {
		 /** Earlier than 3.1 **/
       lpIsTask = MyIsTask;
    }
    else
    {
       (FARPROC)lpIsTask = GetProcAddress(hKernel, "IsTask");
       if(!lpIsTask) lpIsTask = MyIsTask;
    }


	 /** to prevent **BUGS** in 3.0, open 'TOOLHELP.DLL' and **/
	 /** check if exists #BEFORE# attempting 'LoadLibrary()' **/

    if(OpenFile("TOOLHELP.DLL", (LPOFSTRUCT)&ofstr, OF_EXIST)==(HFILE)-1)
    {
       hToolHelp = (HMODULE)NULL;
    }
    else
    {
       hToolHelp = LoadLibrary((LPSTR)ofstr.szPathName);
    }


    if((WORD)hToolHelp>=32)          /* this means it's ok! */
    {
       (FARPROC)lpGlobalFirst      = GetProcAddress(hToolHelp, "GlobalFirst");
       (FARPROC)lpGlobalNext       = GetProcAddress(hToolHelp, "GlobalNext");

       (FARPROC)lpMemManInfo       = GetProcAddress(hToolHelp, "MemManInfo");

       (FARPROC)lpModuleFirst      = GetProcAddress(hToolHelp, "ModuleFirst");
       (FARPROC)lpModuleNext       = GetProcAddress(hToolHelp, "ModuleNext");
       (FARPROC)lpModuleFindHandle = GetProcAddress(hToolHelp, "ModuleFindHandle");

       (FARPROC)lpTaskFirst        = GetProcAddress(hToolHelp, "TaskFirst");
       (FARPROC)lpTaskNext         = GetProcAddress(hToolHelp, "TaskNext");
       (FARPROC)lpTaskFindHandle   = GetProcAddress(hToolHelp, "TaskFindHandle");


       if(!lpGlobalFirst      || !lpGlobalNext || !lpMemManInfo     ||
	  !lpModuleFirst      || !lpModuleNext || !lpTaskFindHandle ||
	  !lpModuleFindHandle || !lpTaskFirst  || !lpTaskNext)
       {
	  FreeLibrary(hToolHelp);

	  hToolHelp = (HMODULE)NULL;  /* follows through to next section */
       }
    }

	      /* this section loads my 'toolhelp' procs */

    if((WORD)hToolHelp<32) /* an error loading procs, or library */
    {
       hToolHelp = (HMODULE)0;

       lpGlobalFirst      = NULL;     /* until I get my version written */
       lpGlobalNext       = NULL;

       lpMemManInfo       = MyMemManInfo;

       lpModuleFirst      = NULL;
       lpModuleNext       = NULL;
       lpModuleFindHandle = NULL;

       lpTaskFirst        = MyTaskFirst;
       lpTaskNext         = MyTaskNext;
       lpTaskFindHandle   = MyTaskFindHandle;


       (FARPROC)lpGlobalMasterHandle =
			   GetProcAddress(hKernel, "GlobalMasterHandle");
       (FARPROC)lpGlobalHandleNoRIP  =
			   GetProcAddress(hKernel, "GlobalHandleNoRIP");

       wIGROUP = (_segment)lpGlobalMasterHandle;   /* assume this is seg 1 */

    }


    hMainWnd = CreateWindow(pClassName,"* WinXRay *",
			    WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
			    CW_USEDEFAULT,CW_USEDEFAULT,
			    CW_USEDEFAULT,CW_USEDEFAULT,NULL,NULL,
			    hInstance,NULL);

    if (!hMainWnd)
    {
	return (FALSE);
    }


    ShowWindow(hMainWnd, nCmdShow);
    UpdateWindow(hMainWnd);
    return (TRUE);

}


void FAR PASCAL MySetCursor(WORD wFlag)
{
static HCURSOR hDefCursor=0, hWait=0;
static WORD wSeq=0;
static DWORD dwStartTime;
static const char NEAR * const pCursorName[7]={"WAIT0","WAIT0","WAIT1","WAIT1",
                                               "WAIT2","WAIT2","WAIT3"};


   // 'MySetCursor(0)' to assign DEFAULT cursor
   // 'MySetCursor(1)' to assign HOURGLASS cursor
   // 'MySetCursor(2)' to 'ROTATE' the hourglass!!

   if(wFlag != 1 && wFlag != 2)  // all others, set default
   {
      if(hDefCursor) SetCursor(hDefCursor);
      else           SetCursor(LoadCursor(NULL, IDC_ARROW));

      hDefCursor = 0;

      if(hWait) DestroyCursor(hWait);

      hWait = 0;
      wSeq = 0;
      dwStartTime = 0;
   }
   else if(wFlag == 1)
   {
      if(hWait) DestroyCursor(hWait);
      hWait = LoadCursor(hInst, pCursorName[3]);
      wSeq = 3;

      if(!hDefCursor) hDefCursor = SetCursor(hWait);
      else            SetCursor(hWait);

      dwStartTime = GetTickCount();
   }
   else if(wFlag == 2)
   {
      if(!hDefCursor)            // default cursor is currently displayed...
      {
         if(hWait) DestroyCursor(hWait);
         hWait = LoadCursor(hInst, pCursorName[0]);
         wSeq = 0;

         hDefCursor = SetCursor(hWait);        // set up for hourglass

         dwStartTime = GetTickCount();
      }
      else if((GetTickCount() - dwStartTime) >= 500)  // more than .5 sec?
      {
         dwStartTime = GetTickCount();

         if(wSeq >= 6) wSeq = 0;  // 0, 1, 2, 3, 4, 5, 6, 0...
         else          wSeq ++;

         if(hWait) DestroyCursor(hWait);

         hWait = LoadCursor(hInst, pCursorName[wSeq]);

         SetCursor(hWait);
      }
   }

}


long CALLBACK MainWndProc(HWND hWnd, UINT message, WORD wParam, LONG lParam)
{
FARPROC lpProcAbout;
long rval;
HWND hChild;
RECT r;
HFONT hFont, hOldFont;
HDC hDC;
static TEXTMETRIC tm;


    switch (message) 
    {
        case WM_CREATE:
           MySetCursor(1);

	   GetClientRect(hWnd, (LPRECT)&r);
	   hChild = CreateWindow("LISTBOX","",
				 WS_CHILD | LBS_NOINTEGRALHEIGHT 
				  | WS_HSCROLL | WS_VSCROLL
				  | LBS_STANDARD,
				 r.left, r.top, r.right, r.bottom,
				 hWnd, (HMENU)ID_LISTBOX, hInst, NULL);
	   if(!hChild)
	   {
	      return(-1L);
           }


           hFont = (HFONT)GetStockObject(OEM_FIXED_FONT);
           SendMessage(hChild, WM_SETFONT, (WPARAM)hFont, NULL);

           hDC = GetDC(hWnd);
           hOldFont = SelectObject(hDC, hFont);
           GetTextMetrics(hDC, (LPTEXTMETRIC)&tm);

           SendMessage(hChild, LB_SETHORIZONTALEXTENT,
                       (WPARAM)(tm.tmAveCharWidth * 80), NULL);

           SelectObject(hDC, hOldFont);
           ReleaseDC(hWnd, hDC);
           ShowWindow(hChild, SW_SHOWNORMAL);
           SetWindowWord(hWnd, 0, (WORD)hChild);
           PostMessage(hWnd, WM_COMMAND, IDM_TASKS, NULL);


           if(!(GetWinFlags() & WF_ENHANCED))  // IT ISN'T ENCHANTED MODE!
           {
              EnableMenuItem(GetMenu(hWnd), IDM_VM,
                             MF_GRAYED | MF_BYCOMMAND);

              EnableMenuItem(GetMenu(hWnd), IDM_PAGEMAP,
                             MF_GRAYED | MF_BYCOMMAND);
           }

           MySetCursor(0);

           return(0L);


        // DIALOG BOX 'PARENT' WINDOW MESSAGES

        case WM_NEXTDLGCTL:      // process this 'dialog' message

           if(lParam) SetFocus((HWND)wParam);
           else       SetFocus((HWND)GetWindowWord(hWnd, 0));
           return(TRUE);

        case DM_GETDEFID:

           return(MAKELONG(GetWindowWord(hWnd, 0),DC_HASDEFID));

        case DM_SETDEFID:
           return(TRUE);  // actually is not used...



        case WM_SETFOCUS:        // when I get the focus, set focus to hChild

           rval = DefWindowProc(hWnd, message, wParam, lParam);
           SetFocus((HWND)GetWindowWord(hWnd, 0));

           return(rval);



	case WM_COMMAND:           /* message: command from application menu */
	    if(wParam==IDM_EXIT)
	    {
	       PostQuitMessage(0);
	       break;
	    }
	    else if(wParam == IDM_ABOUT)
	    {
	       lpProcAbout = MakeProcInstance((FARPROC)About, hInst);

	       DialogBox(hInst,"AboutBox", hWnd, (DLGPROC)lpProcAbout);

	       FreeProcInstance(lpProcAbout);
	       return (NULL);
	    }
	    else if(wParam>=IDM_HELP_MINIMUM && wParam<=IDM_HELP_MAXIMUM)
	    {
	       switch(wParam)
	       {
		   case IDM_HELP_FILES:
		      WinHelp(hWnd, pHelpFileName, HELP_KEY,
			      (DWORD)((LPSTR)"FILES"));
		      break;

		   case IDM_HELP_MEMORY:
		      WinHelp(hWnd, pHelpFileName, HELP_KEY,
			      (DWORD)((LPSTR)"MEMORY"));
		      break;

		   case IDM_HELP_TASKS:
		      WinHelp(hWnd, pHelpFileName, HELP_KEY,
			      (DWORD)((LPSTR)"TASKS"));
		      break;

		   case IDM_HELP_IDT:
		      WinHelp(hWnd, pHelpFileName, HELP_KEY,
			      (DWORD)((LPSTR)"IDT"));
		      break;

		   case IDM_HELP_GDT:
		      WinHelp(hWnd, pHelpFileName, HELP_KEY,
			      (DWORD)((LPSTR)"GDT"));
		      break;

		   case IDM_HELP_LDT:
		      WinHelp(hWnd, pHelpFileName, HELP_KEY,
			      (DWORD)((LPSTR)"LDT"));
		      break;

                   case IDM_HELP_VM:
                      WinHelp(hWnd, pHelpFileName, HELP_KEY,
                              (DWORD)((LPSTR)"VM"));
		      break;

                   case IDM_HELP_PAGEMAP:
		      WinHelp(hWnd, pHelpFileName, HELP_KEY,
			      (DWORD)((LPSTR)"PAGEMAP"));
		      break;

		   default:
		      WinHelp(hWnd, pHelpFileName, HELP_CONTENTS, 0);
		      break;
	       }
	    }
	    else if(wParam>=IDM_MINIMUM && wParam<=IDM_MAXIMUM)
	    {
	       hChild = (HWND)GetWindowWord(hWnd, 0);
	       if(hChild)
               {
//                HCURSOR hOldCursor;
//
//
//                  hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
                  MySetCursor(1);

		  SendMessage(hChild, LB_RESETCONTENT, NULL, NULL);

                  // Initially, prevent window changes from being re-drawn
                  SendMessage(hChild, WM_SETREDRAW, 0, 0);

                  if(wParam==IDM_FILES)
		  {
		     CreateFileList(hChild);
		     SetWindowText(hWnd, "* WinXRay - List of Open Files *");
		  }
		  else if(wParam==IDM_TASKS)
		  {
		     CreateTaskList(hChild);
		     SetWindowText(hWnd, "* WinXRay - List of Running Tasks *");
		  }
		  else if(wParam==IDM_MEMORY)
		  {
		     CreateMemList(hChild);
		     SetWindowText(hWnd, "* WinXRay - List of Memory Usage *");
		  }
		  else if(wParam==IDM_IDT)
		  {
		     CreateVectorList(hChild);
		     SetWindowText(hWnd, "* WinXRay - List of IDT & P-Mode/R-Mode Int Vectors *");
		  }
		  else if(wParam==IDM_GDT)
		  {
		     CreateGDTList(hChild);
		     SetWindowText(hWnd, "* WinXRay - List of Global Descriptor Table *");
		  }
		  else if(wParam==IDM_LDT)
		  {
		     CreateLDTList(hChild);
		     SetWindowText(hWnd, "* WinXRay - List of Local Descriptor Table *");
		  }
                  else if(wParam==IDM_VM)
		  {
                     CreateVMList(hChild);
                     SetWindowText(hWnd, "* WinXRay - List of Virtual Machines *");
		  }
                  else if(wParam==IDM_VXD)
		  {
                     CreateVXDList(hChild);
                     SetWindowText(hWnd, "* WinXRay - List of Virtual Device Drivers *");
		  }
                  else if(wParam==IDM_PAGEMAP)
		  {
		     CreateMemoryPageMap(hChild);
		     SetWindowText(hWnd, "* WinXRay - List of Memory Page Map *");
		  }
		  else if(wParam==IDM_TEST)
		  {
		     TestFunction(hChild);
		     SetWindowText(hWnd, "* WinXRay - 'Test' Function *");
                  }
//                  else if(wParam==IDM_x87)
//                  {
//                     if(GetMenuState(GetMenu(hWnd), IDM_x87, MF_BYCOMMAND)
//                        & MF_CHECKED)
//                     {
//                        CheckMenuItem(GetMenu(hWnd), IDM_x87,
//                                      MF_BYCOMMAND | MF_UNCHECKED);
//
//                        // disable the co-processor!
//
//
//                     }
//                     else
//                     {
//                        CheckMenuItem(GetMenu(hWnd), IDM_x87,
//                                      MF_BYCOMMAND | MF_CHECKED);
//
//
//                        // enable the co-processor!
//
//
//                     }
//                  }


                  // Finally, allow the window changes to be re-drawn
                  SendMessage(hChild, WM_SETREDRAW, (WPARAM)1, 0);

                  // And, PAINT the suckah!
                  InvalidateRect(hChild, NULL, TRUE); // invalidate window
                  UpdateWindow(hChild);

//                  SetCursor(hOldCursor);     // restore the original cursor
                  MySetCursor(0);
               }
	       break;
	    }
	    else                            /* Lets Windows process it       */
	       break;

	case WM_SIZE:
	    hChild = (HWND)GetWindowWord(hWnd, 0);
	    GetClientRect(hWnd, (LPRECT)&r);
	    MoveWindow(hChild, r.left, r.top, r.right, r.bottom, TRUE);
	    break;

	case WM_DESTROY:                  /* message: window being destroyed */
	    PostQuitMessage(0);
	    return (NULL);

    }
    return (DefWindowProc(hWnd, message, wParam, lParam));

}


BOOL CALLBACK __export About(HWND hDlg, UINT message, WORD wParam, LONG lParam)
{

    switch (message)
    {
	case WM_INITDIALOG:
	    return (TRUE);

	case WM_COMMAND:
	    if (wParam == IDOK
		|| wParam == IDCANCEL) {
		EndDialog(hDlg, TRUE);
		return (TRUE);
	    }
	    break;
    }
    return (FALSE);
}


typedef struct {
   LPSTR lpTitle;
   LPSTR lpCaption;
   LPSTR lpBuffer;
   WORD  cbBufSize;
   } INPUT_BOX_PARMS;

typedef INPUT_BOX_PARMS FAR *LPINPUT_BOX_PARMS;


BOOL CALLBACK __export InputBoxProc(HWND hDlg, UINT message, WORD wParam, LONG lParam)
{
LPINPUT_BOX_PARMS lp1;
static const char __based(__segname("_CODE")) szProp1[]="POINTER1";
static const char __based(__segname("_CODE")) szProp2[]="POINTER2";


   switch (message)
   {
      case WM_INITDIALOG:

         SetProp(hDlg, szProp1, (HANDLE)OFFSETOF(lParam));
         SetProp(hDlg, szProp2, (HANDLE)SELECTOROF(lParam));

         lp1 = (LPINPUT_BOX_PARMS)lParam;

         if(lp1->lpTitle)     SetWindowText(hDlg, lp1->lpTitle);
         if(lp1->lpCaption)   SetDlgItemText(hDlg, ID_INPUT_LABEL, lp1->lpCaption);
         if(*(lp1->lpBuffer)) SetDlgItemText(hDlg, ID_INPUT_TEXT, lp1->lpBuffer);

         return (TRUE);


      case WM_COMMAND:

         lp1 = (LPINPUT_BOX_PARMS)MAKELP((WORD)GetProp(hDlg, szProp2),
                                         (WORD)GetProp(hDlg, szProp1));

         if(wParam == IDOK)
         {
            GetDlgItemText(hDlg, ID_INPUT_TEXT,
                           lp1->lpBuffer, lp1->cbBufSize);

            EndDialog(hDlg, 0);
            return (TRUE);
         }
         else if(wParam == IDCANCEL)
         {
            *(lp1->lpBuffer) = 0;

            EndDialog(hDlg, TRUE);
            return (TRUE);
         }

         break;


      case WM_CLOSE:
      case WM_DESTROY:

         RemoveProp(hDlg, szProp1);
         RemoveProp(hDlg, szProp2);

         break;

   }

   return (FALSE);
}



BOOL FAR PASCAL GetUserInput(LPSTR lpTitle, LPSTR lpCaption,
                             LPSTR lpInputBuf, WORD cbBufSize)
{
DLGPROC lpProc;
BOOL rval;
INPUT_BOX_PARMS ibp;



   ibp.lpTitle   = lpTitle;
   ibp.lpCaption = lpCaption;
   ibp.lpBuffer  = lpInputBuf;
   ibp.cbBufSize = cbBufSize;

   lpProc = (DLGPROC)MakeProcInstance((FARPROC)InputBoxProc, hInst);

   rval = (BOOL)DialogBoxParam(hInst, "USER_INPUT", hMainWnd, lpProc,
                               (LPARAM)(LPSTR)&ibp);

   FreeProcInstance((FARPROC)lpProc);

   return(rval);

}






  /*======================================================================*/
  /*                                                                      */
  /* TOOLHELP CLONES - Functions that mimic equivalent ToolHelp functions */
  /*                   for Windows 3.0 when TOOLHELP is not available!    */
  /*                                                                      */
  /*======================================================================*/


BOOL FAR PASCAL MyMemManInfo(LPMEMMANINFO lpInfo)
{
#define mmi (__mmi._mmi_)
struct {
  MEMMANINFO _mmi_;
  char junk[32];  /* 32 bytes of junk, just to make sure I don't overflow */
  } __mmi;


   if(lpInfo==(LPMEMMANINFO)NULL || lpInfo->dwSize!=sizeof(MEMMANINFO))
      return(FALSE);

   mmi.dwSize = sizeof(MEMMANINFO);

   _asm
   {
      push es
      push di

      lea di, WORD PTR mmi.dwLargestFreeBlock
				       /* the first element after 'dwSize' */
      push ss
      pop es

      mov ax, 0x0500    /* get memory manager info - documented in DDK */

      int 0x31                 /* Windows internal interrupt!! */

      pop di                       /* restore stack stuff */
      pop es
   }

   _fmemcpy((LPSTR)lpInfo, (LPSTR)&mmi, sizeof(MEMMANINFO));

   lpInfo->wPageSize = 0x1000;            /* that's the way it's done, ok? */

   return(TRUE);          /* IT WORKED!! */

}



BOOL FAR PASCAL MyIsTask(HTASK hTask)
{
TASKENTRY te;

   _fmemset((LPSTR)&te, 0, sizeof(TASKENTRY));

   te.dwSize = sizeof(TASKENTRY);


   if(!lpTaskFirst((LPTASKENTRY)&te))
   {
     return(FALSE);  /* didn't find it, so return FALSE (not a task) */
   }

   if(te.hTask == hTask) return(TRUE);  /* found - I'm done! */

   while(lpTaskNext((LPTASKENTRY)&te))
   {
      if(te.hTask == hTask) return(TRUE);  /* found - I'm done! */
   }

   return(FALSE);      /* not found, and I've searched the *WORLD* for it! */
}

BOOL FAR PASCAL MyTaskFirst(LPTASKENTRY lpTask)
{
HTASK FAR *lpFirst;

   lpFirst = ((HTASK FAR *)MAKELP(wIGROUP,0x1e));   /* first task in list! */
		    /* the address was hacked out of "other guy's" code... */
			/* and what a pain in the neck that was, too!! */

   return((WORD)MyTaskFindHandle(lpTask, *lpFirst)!=0);  /* return info!! */
		/* why bother checking for a UAE - if it works, it works! */
		/* and, if it don't --- ka-boom!  The thing should work.  */
}


BOOL FAR PASCAL MyTaskNext(LPTASKENTRY lpTask)
{

   if(lpTask->hNext)
      return((WORD)MyTaskFindHandle(lpTask, lpTask->hNext)!=0);
   else
      return(FALSE);
}


HTASK FAR PASCAL MyTaskFindHandle(LPTASKENTRY lpTask, HTASK hTask)
{
LPTDB lpTDB;
DWORD dwTDBSeg;

   if(!(dwTDBSeg = lpGlobalHandleNoRIP((WORD)hTask)))
   {
      return(FALSE);  /* bad handle - bail out with error code */
   }

   lpTDB = (LPTDB)MAKELP(HIWORD(dwTDBSeg), 0);


   /* now the above action gets me the address without locking it!!  */
   /* this will only work in protected mode, but I'm not running any */
   /* real mode code with this program anyway, so who cares??        */
   /* by doing this I hope to avoid some problems I've noted with    */
   /* the accessing of task database segments by other tasks...      */


   /* first step - 'TDB or not TDB - that is the question' (boo, hiss) */

   if(lpTDB->bSig[0]=='T' && lpTDB->bSig[1]=='D' && /* yep, it's a TDB */
      lpTDB->hSelf==hTask )                       /* just to make sure */
   {
      lpTask->hTask       = hTask;
      lpTask->hTaskParent = NULL;  /* I don't support this one.  So what. */
      lpTask->hInst       = lpTDB->hInst;
      lpTask->hModule     = lpTDB->hModule;
      lpTask->wSS         = lpTDB->wSS;
      lpTask->wSP         = lpTDB->wSP;
      lpTask->wStackTop   = NULL;  /* if I really want this I can get it.. */
      lpTask->wStackMinimum = NULL; /* but it's too much hassle for this */
      lpTask->wStackBottom= NULL;  /* program, so I won't bother... */
      lpTask->wcEvents    = lpTDB->nEvents;

      lpTask->hNext       = lpTDB->hNext;

      _fmemset(lpTask->szModule, 0, sizeof(lpTask->szModule));

      _fmemcpy(lpTask->szModule, lpTDB->sModule, sizeof(lpTDB->sModule));

      lpTask->wPSPOffset  = 0;  /* I don't know WHAT the blazes THIS is! */

      return(hTask);                 /* it worked! it worked! it worked! */
   }


   return(FALSE);                    /* for now, this is how I handle it */

}

	/** Uses documented info from DDK (above) to determine **/
	/** if a valid task handle belongs to a WINOLDAP task! **/

BOOL FAR PASCAL IsWINOLDAP(HTASK hTask)
{
TASKENTRY te;
LPTDB lpTDB;
DWORD dwTDBSeg;


   _fmemset((LPSTR)&te, 0, sizeof(te));

   te.dwSize = sizeof(te);

   if(lpIsTask(hTask) && lpTaskFindHandle((LPTASKENTRY)&te, hTask))
   {
      dwTDBSeg = GlobalHandle((WORD)hTask);
      lpTDB = (LPTDB)MAKELP(HIWORD(dwTDBSeg), 0);

      if(lpTDB->wFlags & 1)  /* ah, hah!! */
      {
	 return(TRUE);
      }
   }

       /** Either not a task, or task isn't a WINOLDAP task! **/

   return(FALSE);

}



HTASK FAR PASCAL GetTaskFromInst(HINSTANCE hInst)
{
TASKENTRY te;

   _fmemset((LPSTR)&te, 0, sizeof(te));

   te.dwSize = sizeof(te); /* initializes structure */


   if(!lpTaskFirst((LPTASKENTRY)&te))
   {
      return(NULL);
   }

   do
   {
      if(te.hInst==hInst)
      {
	 return(te.hTask);
      }
   }
   while(lpTaskNext((LPTASKENTRY)&te));

   return(NULL);  /* nope!  didn't find the thing!! */

}


/***************************************************************************/
/*         DPMI 'HELPER' FUNCTIONS THAT I NEED FOR SELECTORS, ETC.         */
/***************************************************************************/

      /** This function allocates an LDT selector using DPMI **/

WORD FAR PASCAL MyAllocSelector(void)
{
WORD wMySel;

		/** Allocate an LDT selector via DPMI **/

   _asm
   {
      mov ax, 0
      mov cx, 1
      int 0x31     ; allocate 1 selector from LDT
      jnc sel_no_error
      mov ax, 0

sel_no_error:
      mov WORD PTR wMySel, ax

   }


   return(wMySel);  /* returns 0 on error, or selector if OK */

}


      /** This function alters a selector's ADDRESS and LIMIT **/

BOOL FAR PASCAL MyChangeSelector(WORD wSelector, DWORD dwAddr, WORD wLimit)
{
register WORD wRval;

   if(!wSelector) return(TRUE);

   _asm
   {
	 mov ax, 0x0007               ; DPMI 'set selector base' function
	 mov bx, WORD PTR wSelector
	 mov cx, WORD PTR dwAddr + 2
	 mov dx, WORD PTR dwAddr      ; address of selector in linear memory

	 int 0x31
	 jc  was_error

	 mov ax, 0x0008               ; DPMI 'set selector limit' function
	 mov bx, WORD PTR wSelector
	 mov cx, 0
	 mov dx, wLimit               ; segment limit

	 int 0x31
	 jnc no_error

was_error:
	 mov ax, 1
	 jmp i_am_done

no_error:
	 mov ax, 0;

i_am_done:

	 mov wRval, ax           /* return 0 if OK, 1 if ERROR */
   }

   return(wRval);
}



 /** This function frees a selector allocated using 'MyAllocSelector()' **/

void FAR PASCAL MyFreeSelector(WORD wSelector)
{
   if(!wSelector) return;

   if(wSelector == 0x28 || wSelector == 0x30)  // GLOBAL 32-bit SELECTORS!
   {
      _asm stc

      return;
   }

   _asm
   {
      mov ax, 1
      mov bx, WORD PTR wSelector
      int 0x31                ; DPMI call to Free LDT Selector
   }

   return;
}


BOOL FAR PASCAL RealModeInt(WORD wIntNum, LPREALMODECALL lpIR)
{
register BOOL rval;


   _asm
   {
      push es
      push ds

      mov ax, 0x300
      mov bl, BYTE PTR wIntNum
      mov bh, 0               /* do not reset INT ctrlr & A20 line */
      mov cx, 0               /* do not copy parms onto real mode stack */
      les di, lpIR            /* the 'INTREGS' structure  */
      int 0x31
      xor ax, ax
      adc ax, 0               /* AX=1 if carry set, 0 if clear */

      mov rval, ax

      pop ds
      pop es
   }

   return(rval);              /* value in AX returns 0 on GOOD, 1 on ERROR */



}



// REAL MODE CALL - PROC ADDRESS IS IN lpIR->CS:lpIR->IP
// Proc must return with 'RETF' instruction, stack in same condition.

BOOL FAR PASCAL RealModeCall(LPREALMODECALL lpIR)
{
register BOOL rval;


   _asm
   {
      push es
      push ds

      mov ax, 0x301
      mov bl, 0               /* not used - assign value of 0 */
      mov bh, 0               /* do not reset INT ctrlr & A20 line */
      mov cx, 0               /* do not copy parms onto real mode stack */
      les di, lpIR            /* the 'INTREGS' structure  */
      int 0x31
      xor ax, ax
      adc ax, 0               /* AX=1 if carry set, 0 if clear */

      mov rval, ax

      pop ds
      pop es
   }

   return(rval);              /* value in AX returns 0 on GOOD, 1 on ERROR */



}






/***************************************************************************/
/*                    OTHER 'SYSTEM' STUFF THAT I NEED                     */
/***************************************************************************/

#pragma pack(1)

typedef struct {
   unsigned char type;  /* the letter 'M', or 'Z' */
   unsigned wOwnerPSP;  /* the DOS PSP of the owner */
   unsigned size;
   unsigned char unused[3];
   unsigned char dos4[8]; /* the application name, if dos 4+ */
   } MCB;
typedef MCB FAR *LPMCB;



HTASK FAR PASCAL GetInstanceTask(HINSTANCE hInst)
{
TASKENTRY te;

   _fmemset((LPSTR)&te, 0, sizeof(te));

   te.dwSize = sizeof(te); /* initializes structure */


   if(!lpTaskFirst((LPTASKENTRY)&te))
   {
      return(NULL);
   }

   do
   {
      if(te.hInst==hInst)
      {
	 return(te.hTask);
      }

   } while(lpTaskNext((LPTASKENTRY)&te));

   return(NULL);  /* nope!  didn't find the thing!! */

}


HINSTANCE FAR PASCAL GetTaskInstance(HTASK hTask)
{
TASKENTRY te;

   _fmemset((LPSTR)&te, 0, sizeof(te));

   te.dwSize = sizeof(te); /* initializes structure */

   if(lpTaskFindHandle(&te, hTask))
   {
      return(te.hInst);
   }
   else
   {
      return(NULL);
   }
}


LPTDB GetTDBFromPSP(WORD wPSPSeg)
{
TASKENTRY te;
LPTDB lpTDB;


   _hmemset(&te, 0, sizeof(te));

   te.dwSize = sizeof(te);

   if(!lpTaskFirst(&te))
   {
      return(NULL);
   }

   do
   {
      lpTDB = (LPTDB)MAKELP(HIWORD(GlobalHandle((WORD)te.hTask)),0);

      if(MyGetSelectorBase(lpTDB->wPDB) == (wPSPSeg * 0x10L))
      {
         return(lpTDB);
      }

   } while(lpTaskNext(&te));


   return(NULL);

}




LPSTR FAR PASCAL GetTaskAndModuleFromPSP(WORD wMachID, WORD wPSPSeg,
					 HTASK FAR *lphTask)
{
WORD wMySel, wMCB, wENV, wPSP, w, w2;
DWORD dwAddr, dwVer, dwWhen, dw1;
LPSTR lpRval, lpLOL, lp1, lp2, lp3, lpOldEnvData;
LPTDB lpTDB;
LPMCB lpMCB;
REALMODECALL rmc;
LPWOATABLE lpWT;
LPWOATABLE4 lpWT4;
LPWOATABLE30 lpWT30;
LPCONTROL_BLOCK lpCB;
static char buf[32], pFmt[]="%-8.8s";
static WORD CurrentVM = 0, CurrentVMFlag=0;


   if(!CurrentVMFlag)
   {
      _asm mov ax, 0x1683    /* Windows(tm) INT 2FH interface to get VM ID */
      _asm int 0x2f
      _asm mov CurrentVM, bx

      CurrentVMFlag = TRUE;
   }

   if(lphTask) *lphTask = 0;

   wMySel = MyAllocSelector();

   if(!wMySel) return("* DPMI ERROR *");



   if(wMachID==CurrentVM)            // It's the WINDOWS VM!  Windows App.
   {
      /** Assign address to it via DPMI, only subtract 256 bytes to get the **/
      /** TASK ID information - YES!  Then, read the task structure and get **/
      /** the other pertinent info out of it.                               **/


      dwAddr = wPSPSeg * 0x10L - 0x100; // the address of the TASK DESCRIPTOR
					// in LINEAR memory - normally <640k


      // First step:  To retain COMPATIBILITY walk the task list and find
      // out which one of the 'TDB's point to this PSP segment!  It's a 2
      // step process, but I can handle it...

      if(lpTDB = GetTDBFromPSP(wPSPSeg))
      {
         // for now, nothing else is done here.
      }
      else if(MyChangeSelector(wMySel, dwAddr, 0xffff))
      {
	 lpTDB = NULL;
      }
      else
      {
	 lpTDB = (LPTDB)MAKELP(wMySel, 0);
      }


      /* now, obtain the task and MODULE NAME from the task descriptor */

      if(lpTDB)
      {
	 if(lpTDB->bSig[0]!='T' || lpTDB->bSig[1]!='D')
	 {
	    lpMCB = (LPMCB)MAKELP(wMySel, 256 - sizeof(MCB));

	    if(lpMCB->type=='M' || lpMCB->type=='Z')
	    {
	       if(lpMCB->wOwnerPSP==wPSPSeg)  /* I  *FOUND*  It! */
	       {
		  lpRval = "* WINDOWS *";
	       }
	       else if(HIWORD(GetVersion())>=0x400)
	       {
		  wsprintf(buf, pFmt, (LPSTR)lpMCB->dos4);
		  lpRval = buf;
	       }
	       else
	       {
		  lpRval = "* UNKNOWN *";
	       }

	    }
	    else       // not owned by SYSTEM, so...
	    {
	     GLOBALENTRY ge;
	     TASKENTRY te;
	     MODULEENTRY me;


	       // Walk through ALL of memory, trying to find the section that
	       // contains *THIS* PSP.  When I find it, determine which
	       // APPLICATION / MODULE (hopefully the *FIRST* one) owns this
	       // block, and return the appropriate task handle/module name

	       lpRval = "* NOT VALID *"; // initial assigned value (if not found)


               _fmemset((LPSTR)&ge, 0, sizeof(ge));
	       _fmemset((LPSTR)&te, 0, sizeof(te));
	       _fmemset((LPSTR)&me, 0, sizeof(me));

               ge.dwSize = sizeof(ge);
	       te.dwSize = sizeof(te);
	       me.dwSize = sizeof(me);

	       if(lpGlobalFirst(&ge, GLOBAL_ALL))
	       {
		  do
		  {  /* bug in TOOLHELP - free blocks have NULL hBlock */
		     /*                   and NULL hOwner...           */

		     if(ge.wFlags==GT_FREE || (!ge.hBlock && !ge.hOwner))
		     {
			continue;
		     }


		     // NOTE:  'dwAddr + 0x100' is address of PSP...

		     if(ge.dwAddress <= (dwAddr + 0x100) &&
			(ge.dwAddress + ge.dwSize) >= (dwAddr + 0x200))
		     {
			// at this point 'ge' describes the first memory block
			// I've found that fully contains this PSP.


			// NOW, let's get info on the task & module that owns it...

			if(lpTaskFindHandle(&te, (HTASK)ge.hOwner))
			{
			   *lphTask = te.hTask;

                           if(lpModuleFindHandle &&
                              lpModuleFindHandle(&me, te.hModule))
                           {
                              wsprintf(buf, pFmt, (LPSTR)me.szModule);
                           }
                           else
                           {
                              wsprintf(buf, pFmt, (LPSTR)te.szModule);
                           }

                           lpRval = (LPSTR)buf;
                        }
			else if(lpModuleFindHandle &&
				lpModuleFindHandle(&me, (HMODULE)ge.hOwner))
			{
			   *lphTask = NULL;  // there is *NO* task handle!

			   wsprintf(buf, pFmt, (LPSTR)me.szModule);

			   lpRval = (LPSTR)buf;
			}


			break;       // see to it that I break out of the loop!
		     }

		  } while(lpGlobalNext(&ge, GLOBAL_ALL));
	       }
	    }
	 }
	 else
	 {
	    if(lphTask) *lphTask = lpTDB->hSelf;

	    wsprintf(buf, pFmt, (LPSTR)lpTDB->sModule);

	    lpRval = (LPSTR)buf;
	 }
      }
      else
      {
	 lpRval = "* DPMI ERR#2 *";
      }

   }
   else if(!wMachID)                       // MS-DOS 'SYSTEM' VM
   {
      lpRval = "* MS-DOS *";               // Assign as such!
   }
   else                                    // NON-WINDOWS APPLICATION
   {
      lpRval = "*NON-WINDOWS*";  // THE 'OLD METHOD' - INITIAL VALUE HERE!

      dwVer = GetVersion();


      if((GetWinFlags() & WF_ENHANCED) &&   // only valid in ENCHANTED mode
	  HIWORD(dwVer)>=0x400)      // DOS 4 or greater - MCB has app name!!
      {
         w = LOWORD(GetVersion());  // what Windows version?
         _asm mov ax, w
         _asm xchg ah,al
         _asm mov w, ax


         if(w < 0x350)      // don't do this for earlier than Windows 3.8
         {
            lpWT = GetWOATABLE();
         }

         if(w >= 0x350 || lpWT)
	 {
	    // OK - now that I have the WOATABLE I need to find the entry
	    // which corresponds with the correct VM ID.  This is a somewhat
	    // difficult process, but I think I can get it


            if(w>=0x350)   // checking for Windows 3.8 or later...
            {
             DWORD dwInfo[16];  // 16 DWORD's (I hope I don't need more!)
             HWND hwndWinOldAp;
             DWORD dwCBSys;

               dwCBSys = MyGetSystemVM();

               dw1 = dwCBSys;
               hwndWinOldAp = NULL;
               dwAddr = 0;

               lpCB = (LPCONTROL_BLOCK)MAKELP(wMySel, 0);

               do
               {
                  MyChangeSelector(wMySel, dw1, 0xffff);

                  if(LOWORD(lpCB->CB_VMID)==wMachID)
                  {
                     dwAddr = lpCB->CB_High_Linear + (wPSPSeg - 1) * 0x10L;
						     // actual address to view!!
   
                     hwndWinOldAp = MyGetVMInfo(dw1, dwInfo);
                     if(lphTask) *lphTask = GetWindowTask(hwndWinOldAp);

                     break;
                  }

                  dw1 = MyGetNextVM(dw1);

               } while(dw1 != dwCBSys);



#if 0 /* OLD METHOD HERE */

               lpWT4 = (WOATABLE4 FAR *)lpWT;

               w2 = GlobalSizePtr(lpWT4);  // how big is it, anyway?

               w2 = (w2 - sizeof(*lpWT4) + sizeof(lpWT4->EntryTable[0]))
                     / sizeof(lpWT4->EntryTable[0]);

	       if(w2 > 64) w2 = 64; // I assume maximum of 64 DOS APP's!
				    // Based on experimentation with Win 3.1
				    // WINOLDAP data segment size & rough calcs

	       lpCB = (LPCONTROL_BLOCK)MAKELP(wMySel, 0);

	       for(dwAddr=0, w=0; w<w2; w++)
	       {
                  if(lpWT4->EntryTable[w].hWnd       &&
                     lpWT4->EntryTable[w].hInst      &&
                     lpWT4->EntryTable[w].dwVMHandle &&
                     lpWT4->EntryTable[w].wFlags )
		  {
                     MyChangeSelector(wMySel, lpWT4->EntryTable[w].dwVMHandle,
				      0xffff);
   
		     if(LOWORD(lpCB->CB_VMID)==wMachID)
                     {
			dwAddr = lpCB->CB_High_Linear + (wPSPSeg - 1) * 0x10L;
						     // actual address to view!!
   
                        if(lphTask) *lphTask =
                                 GetInstanceTask(lpWT4->EntryTable[w].hInst);
   
			break;
		     }
		  }
               }
#endif /* 0  -  OLD METHOD */

            }
            else if(w>=0x30a)   // checking for Windows 3.1 or later...
	    {
	       w2 = GlobalSizePtr(lpWT);  // how big is it, anyway?

	       w2 = (w2 - sizeof(*lpWT) + sizeof(lpWT->EntryTable[0]))
		     / sizeof(lpWT->EntryTable[0]);

	       if(w2 > 64) w2 = 64; // I assume maximum of 64 DOS APP's!
				    // Based on experimentation with Win 3.1
				    // WINOLDAP data segment size & rough calcs

	       lpCB = (LPCONTROL_BLOCK)MAKELP(wMySel, 0);

	       for(dwAddr=0, w=0; w<w2; w++)
	       {
		  if(lpWT->EntryTable[w].hWnd       &&
		     lpWT->EntryTable[w].hInst      &&
		     lpWT->EntryTable[w].dwVMHandle &&
		     lpWT->EntryTable[w].hTask )
		  {
		     MyChangeSelector(wMySel, lpWT->EntryTable[w].dwVMHandle,
				      0xffff);
   
		     if(LOWORD(lpCB->CB_VMID)==wMachID)
		     {
			dwAddr = lpCB->CB_High_Linear + (wPSPSeg - 1) * 0x10L;
						     // actual address to view!!
   
			if(lphTask) *lphTask = lpWT->EntryTable[w].hTask;
   
			break;
		     }
		  }
	       }
	    }
	    else
	    {
	       lpWT30 = (LPWOATABLE30)lpWT;

	       w2 = GlobalSizePtr(lpWT30);  // how big is it, anyway?

	       w2 = (w2 - sizeof(*lpWT30) + sizeof(lpWT30->EntryTable[0]))
		     / sizeof(lpWT30->EntryTable[0]);

	       if(w2 > 64) w2 = 64; // I assume maximum of 64 DOS APP's!
				    // Based on experimentation with Win 3.1
				    // WINOLDAP data segment size & rough calcs

	       lpCB = (LPCONTROL_BLOCK)MAKELP(wMySel, 0);

	       for(dwAddr=0, w=0; w<w2; w++)
	       {
		  if(lpWT30->EntryTable[w].hWnd       &&
		     lpWT30->EntryTable[w].hInst      &&
		     lpWT30->EntryTable[w].dwVMHandle &&
		     lpWT30->EntryTable[w].hTask )
		  {
		     MyChangeSelector(wMySel, lpWT30->EntryTable[w].dwVMHandle,
				      0xffff);
   
		     if(LOWORD(lpCB->CB_VMID)==wMachID)
		     {
			dwAddr = lpCB->CB_High_Linear + (wPSPSeg - 1) * 0x10L;
						     // actual address to view!!
   
			if(lphTask) *lphTask = lpWT30->EntryTable[w].hTask;
   
			break;
		     }
		  }
	       }
	    }



            // At this point, dwAddr points to the PSP MCB within the VM's
            // address space so that I can attempt to find the app name

	    if(dwAddr && !MyChangeSelector(wMySel, dwAddr, 0xffff))
	    {
	       lpMCB = (LPMCB)MAKELP(wMySel, 0); // address of the MCB!!

	       if(lpMCB->wOwnerPSP != wPSPSeg)
	       {
		  wsprintf(buf, "* NOT PSP *");
	       }
	       else
	       {
		  _fmemset(buf, 0, sizeof(buf));
		  _fmemcpy(buf, lpMCB->dos4, sizeof(lpMCB->dos4));
	       }

	       lpRval = buf;                     // that's all, folks!
	    }

	 }

      }
   }


   MyFreeSelector(wMySel);

   return(lpRval);
}





LPSTR FAR PASCAL OldGetTaskAndModuleFromPSP(WORD wMachID, WORD wPSPSeg,
                                            HTASK FAR *lphTask)
{
WORD wMySel, wMCB, wENV, wPSP, w, w2;
DWORD dwAddr, dwVer, dwWhen;
LPSTR lpRval, lpLOL, lp1, lp2, lp3, lpOldEnvData;
LPTDB lpTDB;
LPMCB lpMCB;
REALMODECALL rmc;
LPWOATABLE lpWT;
LPWOATABLE30 lpWT30;
LPCONTROL_BLOCK lpCB;
static char buf[32], pFmt[]="%-8.8s";
static WORD CurrentVM = 0, CurrentVMFlag=0;


   if(!CurrentVMFlag)
   {
      _asm mov ax, 0x1683    /* Windows(tm) INT 2FH interface to get VM ID */
      _asm int 0x2f
      _asm mov CurrentVM, bx

      CurrentVMFlag = TRUE;
   }

   if(lphTask) *lphTask = 0;

   wMySel = MyAllocSelector();

   if(!wMySel) return("* DPMI ERROR *");



   if(wMachID==CurrentVM)            // It's the WINDOWS VM!  Windows App.
   {

      /** Assign address to it via DPMI, only subtract 256 bytes to get the **/
      /** TASK ID information - YES!  Then, read the task structure and get **/
      /** the other pertinent info out of it.                               **/


      dwAddr = wPSPSeg * 0x10L - 0x100; // the address of the TASK DESCRIPTOR
					// in LINEAR memory - normally <640k

      if(MyChangeSelector(wMySel, dwAddr, 0xffff))
      {
	 lpTDB = NULL;
      }
      else
      {
	 lpTDB = (LPTDB)MAKELP(wMySel, 0);
      }


      /* now, obtain the task and MODULE NAME from the task descriptor */

      if(lpTDB)
      {
	 if(lpTDB->bSig[0]!='T' || lpTDB->bSig[1]!='D')
	 {
	    lpMCB = (LPMCB)MAKELP(wMySel, 256 - sizeof(MCB));

	    if(lpMCB->type=='M' || lpMCB->type=='Z')
	    {
	       if(lpMCB->wOwnerPSP==wPSPSeg)  /* I  *FOUND*  It! */
	       {
		  lpRval = "* WINDOWS *";
	       }
	       else if(HIWORD(GetVersion())>=0x400)
	       {
		  wsprintf(buf, pFmt, (LPSTR)lpMCB->dos4);
		  lpRval = buf;
	       }
	       else
	       {
		  lpRval = "* UNKNOWN *";
	       }

	    }
//            else
//            {
//               lpRval = "** ??? **";
//            }
//
	    else       // not owned by SYSTEM, so...
	    {
	     GLOBALENTRY ge;
	     TASKENTRY te;
	     MODULEENTRY me;


	       // Walk through ALL of memory, trying to find the section that
	       // contains *THIS* PSP.  When I find it, determine which
	       // APPLICATION / MODULE (hopefully the *FIRST* one) owns this
	       // block, and return the appropriate task handle/module name

	       lpRval = "* NOT VALID *"; // initial assigned value (if not found)

               _fmemset((LPSTR)&ge, 0, sizeof(ge));
	       _fmemset((LPSTR)&te, 0, sizeof(te));
	       _fmemset((LPSTR)&me, 0, sizeof(me));

               ge.dwSize = sizeof(ge);
	       te.dwSize = sizeof(te);
	       me.dwSize = sizeof(me);


	       if(lpGlobalFirst(&ge, GLOBAL_ALL))
	       {
		  do
		  {  /* bug in TOOLHELP - free blocks have NULL hBlock */
		     /*                   and NULL hOwner...           */

		     if(ge.wFlags==GT_FREE || (!ge.hBlock && !ge.hOwner))
		     {
			continue;
		     }


		     // NOTE:  'dwAddr + 0x100' is address of PSP...

		     if(ge.dwAddress <= (dwAddr + 0x100) &&
			(ge.dwAddress + ge.dwSize) >= (dwAddr + 0x200))
		     {
			// at this point 'ge' describes the first memory block
			// I've found that fully contains this PSP.


			// NOW, let's get info on the task & module that owns it...

			if(lpTaskFindHandle(&te, (HTASK)ge.hOwner))
			{
			   *lphTask = te.hTask;

			   wsprintf(buf, pFmt, (LPSTR)te.szModule);

			   lpRval = (LPSTR)buf;
			}
			else if(lpModuleFindHandle &&
				lpModuleFindHandle(&me, (HMODULE)ge.hOwner))
			{
			   *lphTask = NULL;  // there is *NO* task handle!

			   wsprintf(buf, pFmt, (LPSTR)me.szModule);

			   lpRval = (LPSTR)buf;
			}


			break;       // see to it that I break out of the loop!
		     }

		  } while(lpGlobalNext(&ge, GLOBAL_ALL));
	       }
	    }
	 }
	 else
	 {
	    if(lphTask) *lphTask = lpTDB->hSelf;

	    wsprintf(buf, pFmt, (LPSTR)lpTDB->sModule);

	    lpRval = (LPSTR)buf;
	 }
      }
      else
      {
	 lpRval = "* DPMI ERR#2 *";
      }

   }
   else if(!wMachID)                       // MS-DOS 'SYSTEM' VM
   {
      lpRval = "* MS-DOS *";               // Assign as such!
   }
   else                                    // NON-WINDOWS APPLICATION
   {
      lpRval = "*NON-WINDOWS*";  // THE 'OLD METHOD' - INITIAL VALUE HERE!

      dwVer = GetVersion();


      if((GetWinFlags() & WF_ENHANCED) &&   // only valid in ENCHANTED mode
	  HIWORD(dwVer)>=0x400)      // DOS 4 or greater - MCB has app name!!
      {
//
//         COMMENT OUT THE OLD METHOD THAT USED 'VDDXRAY' - I'm done now!
//
//         _asm mov ax, 0x300
//         _asm int 0x2f            // check for 'VDDXRAY'
//         _asm mov w, ax
//
//         if(w==0xff)              // verifies 'VDDXRAY' is present
//         {
//            _asm mov ax, 0x302
//            _asm mov bx, wMachID
//            _asm int 0x2f         // look up address info!!
//
//            _asm mov WORD PTR dwAddr, ax
//            _asm mov WORD PTR dwAddr+2, dx
//
//            dwAddr += (wPSPSeg - 1) * 0x10L;


	 lpWT = GetWOATABLE();

	 if(lpWT)
	 {
	    // OK - now that I have the WOATABLE I need to find the entry
	    // which corresponds with the correct VM ID.  This is a somewhat
	    // difficult process, but I think I can get it

	    w = LOWORD(GetVersion());  // what Windows version?
	    _asm mov ax, w
	    _asm xchg ah,al
	    _asm mov w, ax

	    if(w>=0x30a)   // checking for Windows 3.1 or later...
	    {
	       w2 = GlobalSizePtr(lpWT);  // how big is it, anyway?

	       w2 = (w2 - sizeof(*lpWT) + sizeof(lpWT->EntryTable[0]))
		     / sizeof(lpWT->EntryTable[0]);

	       if(w2 > 64) w2 = 64; // I assume maximum of 64 DOS APP's!
				    // Based on experimentation with Win 3.1
				    // WINOLDAP data segment size & rough calcs

	       lpCB = (LPCONTROL_BLOCK)MAKELP(wMySel, 0);

	       for(dwAddr=0, w=0; w<w2; w++)
	       {
		  if(lpWT->EntryTable[w].hWnd       &&
		     lpWT->EntryTable[w].hInst      &&
		     lpWT->EntryTable[w].dwVMHandle &&
		     lpWT->EntryTable[w].hTask )
		  {
		     MyChangeSelector(wMySel, lpWT->EntryTable[w].dwVMHandle,
				      0xffff);
   
		     if(LOWORD(lpCB->CB_VMID)==wMachID)
		     {
			dwAddr = lpCB->CB_High_Linear + (wPSPSeg - 1) * 0x10L;
						     // actual address to view!!
   
			if(lphTask) *lphTask = lpWT->EntryTable[w].hTask;
   
			break;
		     }
		  }
	       }
	    }
	    else
	    {
	       lpWT30 = (LPWOATABLE30)lpWT;

	       w2 = GlobalSizePtr(lpWT30);  // how big is it, anyway?

	       w2 = (w2 - sizeof(*lpWT30) + sizeof(lpWT30->EntryTable[0]))
		     / sizeof(lpWT30->EntryTable[0]);

	       if(w2 > 64) w2 = 64; // I assume maximum of 64 DOS APP's!
				    // Based on experimentation with Win 3.1
				    // WINOLDAP data segment size & rough calcs

	       lpCB = (LPCONTROL_BLOCK)MAKELP(wMySel, 0);

	       for(dwAddr=0, w=0; w<w2; w++)
	       {
		  if(lpWT30->EntryTable[w].hWnd       &&
		     lpWT30->EntryTable[w].hInst      &&
		     lpWT30->EntryTable[w].dwVMHandle &&
		     lpWT30->EntryTable[w].hTask )
		  {
		     MyChangeSelector(wMySel, lpWT30->EntryTable[w].dwVMHandle,
				      0xffff);
   
		     if(LOWORD(lpCB->CB_VMID)==wMachID)
		     {
			dwAddr = lpCB->CB_High_Linear + (wPSPSeg - 1) * 0x10L;
						     // actual address to view!!
   
			if(lphTask) *lphTask = lpWT30->EntryTable[w].hTask;
   
			break;
		     }
		  }
	       }
	    }


	    if(dwAddr && !MyChangeSelector(wMySel, dwAddr, 0xffff))
	    {
	       lpMCB = (LPMCB)MAKELP(wMySel, 0); // address of the MCB!!

	       if(lpMCB->wOwnerPSP != wPSPSeg)
	       {
		  wsprintf(buf, "* NOT PSP *");
	       }
	       else
	       {
		  _fmemset(buf, 0, sizeof(buf));
		  _fmemcpy(buf, lpMCB->dos4, sizeof(lpMCB->dos4));
	       }

	       lpRval = buf;                     // that's all, folks!
	    }

	 }

      }
   }


   MyFreeSelector(wMySel);

   return(lpRval);
}



LPWOATABLE FAR PASCAL GetWOATABLE()
{
HMODULE hModule;
LPMDB lpMDB;
LPNEW_SEG1 lpS;
LPWOATABLE lpRval;
WORD w, w2;


   hModule = GetModuleHandle("WINOLDAP");

   if(!hModule) return(NULL);    // WINOLDAP IS NOT LOADED - NO SUCH LUCK!


   lpMDB = (LPMDB)MAKELP(HIWORD(GlobalHandle((WORD)hModule)),0);

   lpS = (LPNEW_SEG1)((LPSTR)(lpMDB) + lpMDB->ne_segtab);

   w2 = (lpMDB->ne_restab - lpMDB->ne_segtab) / sizeof(*lpS);


   for(w=1; w<=w2; w++)
   {
      if(w==lpMDB->ne_autodata)  continue;  // DON'T USE THIS ONE!!

      if((lpS[w - 1].newseg.ns_flags & NSDATA) == NSDATA)
      {
	 lpRval = (LPWOATABLE)
		  MAKELP(HIWORD(GlobalHandle(lpS[w - 1].ns_handle)),0);

	 return(lpRval);
      }
   }

   return(NULL);       // NOT FOUND!!

}


DWORD FAR PASCAL GetVMInfoFromTask(HTASK hTask, WORD FAR *lpwVMID,
                                   LPSTR lpAppName)
{
WORD w, w2, wMySel, wPSP;
LPWOATABLE lpWT;
LPWOATABLE4 lpWT4;
LPWOATABLE30 lpWT30;
LPCONTROL_BLOCK lpCB;
DWORD dwVMHandle, dwBase, dwAddr, dwCBSys, dw1;
LPSTR lp1;



   if(lpwVMID)   *lpwVMID = 0;
   if(lpAppName) *lpAppName = 0;

   if(!(GetWinFlags() & WF_ENHANCED))
   {
      lstrcpy(lpAppName, "{NON-WINDOWS}");

      return(0);  // won't work - no way!!
   }

   wMySel = MyAllocSelector();

   if(!wMySel)
   {
      return(0);
   }

   dwCBSys = MyGetSystemVM();



   w = LOWORD(GetVersion());  // what Windows version?
   _asm mov ax, w
   _asm xchg ah,al
   _asm mov w, ax


   dwVMHandle = 0;   // a flag


   if(w >= 0x350)      // checking for Windows 3.8 or later...
   {
    DWORD dwInfo[16];  // 16 DWORD's (I hope I don't need more!)
    HWND hwndWinOldAp;

      dw1 = dwCBSys;
      dwVMHandle = 0;

      do
      {
         hwndWinOldAp = MyGetVMInfo(dw1, dwInfo);

         if(IsWindow(hwndWinOldAp) &&
            GetWindowTask(hwndWinOldAp) == hTask)
         {
            dwVMHandle = dw1;
            break;
         }

         dw1 = MyGetNextVM(dw1);

      } while(dw1 != dwCBSys);

      if(!dwVMHandle)
      {
         lstrcpy(lpAppName, "#NON-WINDOWS#");
         return(TRUE);
      }


#if 0 /* OLD METHOD HERE */
    HINSTANCE hTaskInst = GetTaskInstance(hTask);


      if(!hTaskInst)
      {
         lstrcpy(lpAppName, "!NON-WINDOWS!");
         return(TRUE);
      }

      lpWT4 = (LPWOATABLE4)GetWOATABLE();
      if(!lpWT4)
      {
         lstrcpy(lpAppName, "|NON-WINDOWS|");
         return(TRUE);
      }


      w2 = GlobalSizePtr(lpWT4);  // how big is it, anyway?

      w2 = (w2 - sizeof(*lpWT4) + sizeof(lpWT4->EntryTable[0]))
           / sizeof(lpWT4->EntryTable[0]);

      if(w2 > 64)   w2 = 64;     // I assume maximum of 64 DOS APP's!
                                 // Based on experimentation with Win 3.1
                                 // WINOLDAP data segment size & rough calcs


      for(w=0; w<w2; w++)
      {
         if(lpWT4->EntryTable[w].hWnd       &&
            lpWT4->EntryTable[w].dwVMHandle &&
            ((WORD)lpWT4->EntryTable[w].hInst & 0xfff8) ==
            ((WORD)hTaskInst & 0xfff8) )
         {
            dwVMHandle = lpWT4->EntryTable[w].dwVMHandle;
            break;
         }
      }
#endif /* 0 */
   }
   else if(w>=0x30a)   // checking for Windows 3.1 or later...
   {
      lpWT = GetWOATABLE();
      if(!lpWT) return(TRUE);


      w2 = GlobalSizePtr(lpWT);  // how big is it, anyway?

      w2 = (w2 - sizeof(*lpWT) + sizeof(lpWT->EntryTable[0]))
           / sizeof(lpWT->EntryTable[0]);

      if(w2 > 64)   w2 = 64;     // I assume maximum of 64 DOS APP's!
                                 // Based on experimentation with Win 3.1
                                 // WINOLDAP data segment size & rough calcs


      for(w=0; w<w2; w++)
      {
         if(lpWT->EntryTable[w].hWnd       &&
            lpWT->EntryTable[w].hInst      &&
            lpWT->EntryTable[w].dwVMHandle &&
            lpWT->EntryTable[w].hTask == hTask)
         {
            dwVMHandle = lpWT->EntryTable[w].dwVMHandle;
            break;
         }
      }
   }
   else
   {
      lpWT30 = (LPWOATABLE30)GetWOATABLE();
      if(!lpWT30) return(TRUE);


      w2 = GlobalSizePtr(lpWT30);  // how big is it, anyway?

      w2 = (w2 - sizeof(*lpWT30) + sizeof(lpWT30->EntryTable[0]))
            / sizeof(lpWT30->EntryTable[0]);

      if(w2 > 64)   w2 = 64;     // I assume maximum of 64 DOS APP's!
                                 // Based on experimentation with Win 3.1
                                 // WINOLDAP data segment size & rough calcs


      for(w=0; w<w2; w++)
      {
         if(lpWT30->EntryTable[w].hWnd       &&
            lpWT30->EntryTable[w].hInst      &&
            lpWT30->EntryTable[w].dwVMHandle &&
            lpWT30->EntryTable[w].hTask == hTask)
         {
            dwVMHandle = lpWT30->EntryTable[w].dwVMHandle;
            break;
         }
      }
   }


   if(!dwVMHandle)  // this means I did *NOT* find it!!
   {
      MyFreeSelector(wMySel);

      lstrcpy(lpAppName, "?NON-WINDOWS?");

      return(0);            // NOT FOUND (error)
   }


   lpCB = (LPCONTROL_BLOCK)MAKELP(wMySel, 0);
   MyChangeSelector(wMySel, dwVMHandle, 0xffff);

   if(lpwVMID) *lpwVMID = (WORD)lpCB->CB_VMID;

   MyFreeSelector(wMySel);

   GetAppNameFromVMHandle(dwVMHandle, lpAppName);

   return(dwVMHandle);
}



void FAR PASCAL GetAppNameFromVMHandle(DWORD dwVMHandle, LPSTR lpAppName)
{
DWORD dwBase, dwAddr;
WORD w1, wPSP, wMySel;
REALMODECALL rmc;
LPSTR lp1;
LPMCB lpMCB;
LPCONTROL_BLOCK lpCB;



   if(dwVMHandle == MyGetSystemVM())
   {
      lstrcpy(lpAppName, "*WINDOWS*");  // indicate WINDOWS is the VM name!
      return;
   }

   wMySel = MyAllocSelector();

   if(!wMySel)
   {
      lstrcpy(lpAppName, "*DPMIERR2*");
      return;
   }


   lpCB = MAKELP(wMySel, 0);    // pointer to control block

   MyChangeSelector(wMySel, dwVMHandle, 0xffff);



   // Now for the fun part!  I get to find the NAME!

   dwBase = lpCB->CB_High_Linear;  // say 'byby' to 'lpCB' since I re-use sel!


   // With the base address, I must now blast into DOS and get the
   // correct PSP for the running task.  To do this, I get the 'DOS swap'
   // area address + 10H -> CURRENT PSP!

   // Let's see just how well this works..

   w1 = HIWORD(GetVersion());

   if(w1<0x400)                     // release 3.0 doesn't have what we want
   {
      if(lpAppName) lstrcpy(lpAppName, "[NON-WINDOWS]");
   }
   else if(lpAppName)    // only if it's non-null
   {
      if(w1 == 0x400)             // DOS 4.0?? (Verified no workee in 6.0!)
      {
         rmc.EAX = 0x5d0b;
      }
      else                       // EVERY OTHER version of DOS
      {
         rmc.EAX = 0x5d06;
      }

      rmc.SP = 0;
      rmc.SS = 0;    // a stack is created automatically

      rmc.flags = 0;

      RealModeInt(0x21, &rmc);  // get (via DPMI) the swappable DOS areas

      if(!(rmc.flags & 1))         // carry clear!!
      {

         dwAddr = (((DWORD)rmc.DS) << 4) + LOWORD(rmc.ESI);

         // OK!  Assume that all tasks use the SAME 'relative' address, and
         // add this to the current VM's 'BASE' address.

         MyChangeSelector(wMySel, dwAddr + dwBase, 0xffff);

         lp1 = MAKELP(wMySel, 0x10); // pointer has offset to 'current PSP'

         wPSP = *((WORD FAR *)lp1);  // get it!!  YES!!!


         // Now that I have the PSP, calculate the address of its MCB to verify
         // that it *IS* a PSP, and then get the PROGRAM NAME that owns it!

         MyChangeSelector(wMySel, dwBase + ((DWORD)wPSP << 4) - 0x10, 0xffff);

         lpMCB = (LPMCB)MAKELP(wMySel, 0);

         if(lpMCB->wOwnerPSP != wPSP)
         {
            lstrcpy(lpAppName, "* INVALID MCB *");
         }
         else
         {
            _fmemset(lpAppName, 0, sizeof(lpMCB->dos4) + 1);
            _fmemcpy(lpAppName, lpMCB->dos4, sizeof(lpMCB->dos4));
         }
      }
      else
      {
         lstrcpy(lpAppName, "(NON-WINDOWS)");
      }
   }


   MyFreeSelector(wMySel);

}


#pragma pack()
