/***************************************************************************/
/*                                                                         */
/*   SFTSH32X.C - Command line interpreter for Microsoft(r) Windows (tm)   */
/*                                                                         */
/*                      TRADE SECRET - CONFIDENTIAL                        */
/*                                                                         */
/*             The compiled libraries and API header files are             */
/*          (c) Copyright 1990-1995 by Stewart~Frazier Tools, Inc          */
/*                                                                         */
/*   This is a 32-bit "helper" application that uses the command line as   */
/*   an argument to 'CreateProcess()', returning the 16-bit HTASK as the   */
/*   program return code.  Caller must wait for process to terminate.      */
/*                                                                         */
/***************************************************************************/

#define WIN32 1

#include "windows.h"
#include "wownt32.h"


int PASCAL WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
STARTUPINFO StartupInfo;
PROCESS_INFORMATION ProcessInfo;
LPSTR lp1, lp0, lp2, lp3;
int iRval = -1;
int i1, iWnd;
HWND hCaller;
LPSTR lpEnv, lpEnv0;
char tbuf[MAX_PATH + 8];
HANDLE hCon;
BOOL bNotCon;


  // do NOT create a window or message loop.

  memset(&StartupInfo, 0, sizeof(StartupInfo));

  StartupInfo.cb = sizeof(StartupInfo);
  StartupInfo.wShowWindow = nCmdShow;

  StartupInfo.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
  StartupInfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
  StartupInfo.hStdError  = GetStdHandle(STD_ERROR_HANDLE);

  bNotCon = FALSE;  // this seems to work universally, FWIW

#if 0

  // see if the stdin, stdout, stderr handles refer to the console.
  // If they do, don't bother specifying 'STARTF_USESTDHANDLES' and
  // don't let process inherit handles, either.

  if(StartupInfo.hStdInput  != INVALID_HANDLE_VALUE &&
     StartupInfo.hStdOutput != INVALID_HANDLE_VALUE &&
     StartupInfo.hStdError  != INVALID_HANDLE_VALUE)
  {
//    MessageBox(NULL, "IS invalid handles (assume CON)", "* SFTSH32X.EXE *", MB_OK | MB_ICONINFORMATION);
  }
  else if(GetFileType(StartupInfo.hStdInput) == FILE_TYPE_UNKNOWN ||
          GetFileType(StartupInfo.hStdOutput) == FILE_TYPE_UNKNOWN ||
          GetFileType(StartupInfo.hStdError) == FILE_TYPE_UNKNOWN)
  {
//    MessageBox(NULL, "IS UNKNOWN device", "* SFTSH32X.EXE *", MB_OK | MB_ICONINFORMATION);
  }
  else if(GetFileType(StartupInfo.hStdInput) != FILE_TYPE_CHAR ||
          GetFileType(StartupInfo.hStdOutput) != FILE_TYPE_CHAR ||
          GetFileType(StartupInfo.hStdError) != FILE_TYPE_CHAR)
  {
//    MessageBox(NULL, "IS NOT char device", "* SFTSH32X.EXE *", MB_OK | MB_ICONINFORMATION);
    bNotCon = TRUE;
  }
  else
  {
    BY_HANDLE_FILE_INFORMATION bhfiCon, bhfi;

    GetFileInformationByHandle(StartupInfo.hStdError, &bhfiCon);

    GetFileInformationByHandle(StartupInfo.hStdInput, &bhfi);

    if(bhfi.nFileIndexHigh != bhfiCon.nFileIndexHigh ||
       bhfi.nFileIndexLow != bhfiCon.nFileIndexLow)
    {
      bNotCon = TRUE;
//      MessageBox(NULL, "'CON' doesn't match stdin", "* SFTSH32X.EXE *", MB_OK | MB_ICONINFORMATION);
    }
    else
    {
      GetFileInformationByHandle(StartupInfo.hStdOutput, &bhfi);

      if(bhfi.nFileIndexHigh != bhfiCon.nFileIndexHigh ||
         bhfi.nFileIndexLow != bhfiCon.nFileIndexLow)
      {
        bNotCon = TRUE;
//        MessageBox(NULL, "'CON' doesn't match stdout", "* SFTSH32X.EXE *", MB_OK | MB_ICONINFORMATION);
      }
    }
  }

#endif // 0

  if(bNotCon)
  {
//    MessageBox(NULL, "IS NOT 'CON'", "* SFTSH32X.EXE *", MB_OK | MB_ICONINFORMATION);
    StartupInfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES
                        | STARTF_FORCEONFEEDBACK;
  }
  else
  {
//    MessageBox(NULL, "IS 'CON'", "* SFTSH32X.EXE *", MB_OK | MB_ICONINFORMATION);
    StartupInfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_FORCEONFEEDBACK;
  }


  // I will need to send messages to the calling process' main window.
  // The 1st 6 characters of the command line will be "$xxxx ",
  // where 'xxxx' is the HEX value of the main window handle of the
  // calling process.

  iWnd = 0;
  lp1 = lpCmdLine;
  while(*lp1 && *lp1 <= ' ') lp1++;

  if(*lp1 != '$')  // begins with '$'?
  {
     MessageBox(NULL, "Command line Syntax Error",
                "* SFTSH32X.EXE *", MB_OK | MB_ICONINFORMATION);

     return(1);  // syntax error
  }

  for(i1=0, lp1++; i1 < 4 && *lp1; i1++, lp1++)
  {
     iWnd = iWnd * 16;

     if(*lp1 >= '0' && *lp1 <= '9')
     {
        iWnd += *lp1 - '0';
     }
     else if(*lp1 >= 'A' && *lp1 <= 'F')
     {
        iWnd += *lp1 + 10 - 'A';
     }
     else if(*lp1 >= 'a' && *lp1 <= 'f')
     {
        iWnd += *lp1 + 10 - 'a';
     }
     else
     {
        MessageBox(NULL, "Command line Syntax Error (2)",
                   "* SFTSH32X.EXE *", MB_OK | MB_ICONINFORMATION);

        return(1);  // command syntax error
     }
  }

  if(*lp1 && *lp1 <= ' ')
  {
     lp1++;  // skip the final "blank"
  }
  else
  {
     MessageBox(NULL, "Command line Syntax Error (3)",
                "* SFTSH32X.EXE *", MB_OK | MB_ICONINFORMATION);

     return(1);     // command syntax error
  }

  hCaller = (HWND)WOWHandle32((WORD)iWnd, WOW_TYPE_HWND);  // 32-bit HWND value


  // 'lp1' is the command.  Set the 'CMDLINE' environment variable to match
  // a few programs seem to rely on this, but I don't know why...

  SetEnvironmentVariable("CMDLINE", lp1);

  // next, look for a matching entry in the following location:
  // "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths"
  // The EXE file without the path may be found here, and its qualified path must
  // also match the {default} value for this key.
  // The 'Path' value (if present) is a string that's PREPENDED to the PATH=
  // environment variable.  Several programs rely on this.  Boo, hiss on Microsoft
  // for making it *SO* *NON-OBVIOUS*!!!!!

  {
    HKEY hKey;
    if(RegOpenKey(HKEY_LOCAL_MACHINE,
                  "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths",
                  &hKey) == ERROR_SUCCESS)
    {
      HKEY hKey2;

      // find EXE name and isolate it, so I can look up that key

      lp2 = lp1;
      if(*lp2 == '"')
      {
        lp2++;

        while(*lp2 && *lp2 != '"') lp2++;

        if(lp2 > (lp1 + 1)) memcpy(tbuf, lp1 + 1, lp2 - lp1 - 1);
        tbuf[lp2 - lp1 - 1] = 0;
      }
      else
      {
        while(*lp2 && *lp2 > ' ') lp2++;

        if(lp2 > lp1) memcpy(tbuf, lp1, lp2 - lp1);
        tbuf[lp2 - lp1] = 0;
      }

      if(lp2 = strrchr(tbuf, '\\'))
      {
        lp2++;
      }
      else
      {
        lp2 = tbuf;
      }

      strupr(lp2);

      if(RegOpenKey(hKey, lp2,  &hKey2) == ERROR_SUCCESS)
      {
        // found!  See if the path matches my path

        static char tbuf2[MAX_PATH * 4 + 8192];

        DWORD dwType;
        DWORD cbData = sizeof(tbuf2);

        if(RegQueryValueEx(hKey2, "", NULL, &dwType, (LPBYTE)tbuf2, &cbData)
           == ERROR_SUCCESS)
        {
          tbuf2[cbData] = 0;

          GetFullPathName(tbuf2, sizeof(tbuf2), tbuf2, NULL);
          GetShortPathName(tbuf2, tbuf2, sizeof(tbuf2));

          GetFullPathName(tbuf, sizeof(tbuf), tbuf, NULL);
          GetShortPathName(tbuf, tbuf, sizeof(tbuf));

          if(!stricmp(tbuf, tbuf2))  // that is, the paths match!
          {
            // get the 'Path' entry for this key

            cbData = sizeof(tbuf2);

            if(RegQueryValueEx(hKey2, "Path", NULL, &dwType, (LPBYTE)tbuf2, &cbData)
               == ERROR_SUCCESS)
            {
              tbuf2[cbData] = 0;
              cbData = lstrlen(tbuf2);  // just in case

              // 'Path' entry found - take value and prepend to 'PATH' env var

              if(tbuf2[cbData - 1] != ';') // ends in ';'?
              {
                tbuf2[cbData++] = ';';     // no - add one
                tbuf2[cbData] = 0;
              }

              GetEnvironmentVariable("PATH",tbuf2 + cbData,sizeof(tbuf2) - cbData);
              SetEnvironmentVariable("PATH",tbuf2);
            }
          }
        }

        RegCloseKey(hKey2);
      }

      RegCloseKey(hKey);
    }
  }



  //  MessageBox(NULL, lpCmdLine, "* SFTSH32X.EXE *", MB_OK | MB_ICONINFORMATION);

  // TODO:  check for 'PIF' file, handle with special 'PIF' info API's

  lpEnv0 = GetEnvironmentStrings();

  // search through the environment, looking for '=X:' entries (drives).
  // They must be at the beginning.  If I don't find any, add an entry for
  // the current drive/dir only.  If I do find one, don't change it.

  // scan env, get size.

  lp0 = lpEnv0;
  while(*lp0)
  {
    lp0 += lstrlen(lp0) + 1;
  }

  // add MAX_PATH + 16 to the length, and alloc some memory

  lpEnv = (HGLOBAL)GlobalAlloc(GPTR, (lp0 - lpEnv0) + MAX_PATH + 16);

  if(lpEnv)  // just in case
  {
    lp3 = lpEnv;

    *tbuf = 0;
    GetCurrentDirectory(sizeof(tbuf), tbuf);

    lp2 = lpEnv0;
    while(*tbuf && lp2 < lp0 && *lp2 == '=')
    {
      if(toupper(lp2[1]) == toupper(*tbuf))
      {
        *tbuf = 0;  // don't bother using it
        break;
      }
      else if(toupper(lp2[1]) > toupper(*tbuf))  // passed it!
      {
        memcpy(lp3, lpEnv0, lp2 - lpEnv0);

        lp3 += lp2 - lpEnv0;

        *(lp3++) = '=';
        *(lp3++) = *tbuf;
        *(lp3++) = ':';
        *(lp3++) = '=';
        lstrcpy(lp3, tbuf);

        lp3 += lstrlen(lp3) + 1;

        lp3[0] = 0;  // just in case
        lp3[1] = 0;

        lpEnv0 = lp2;
        *tbuf = 0;
        break;
      }

      lp2 += lstrlen(lp2) + 1;
    }

    // copy path, then remainder of environment.

    if(*tbuf)
    {
      *(lp3++) = '=';
      *(lp3++) = *tbuf;
      *(lp3++) = ':';
      *(lp3++) = '=';
      lstrcpy(lp3, tbuf);

      lp3 += lstrlen(lp3) + 1;
    }

    memcpy(lp3, lpEnv0, lp0 - lpEnv0);

    lp3 += lp0 - lpEnv0;
    *(lp3++) = 0;
    *(lp3++) = 0;
    *(lp3++) = 0;
    *(lp3++) = 0;  // mark the end!
  }

  *tbuf = 0;
  GetCurrentDirectory(sizeof(tbuf), tbuf);
  if(*tbuf) lp2 = tbuf;  // current directory (temporary - try it!)


//  MessageBox(NULL, lpEnv, "* SFTSH32X.EXE *", MB_OK | MB_ICONINFORMATION);

  if(CreateProcess(NULL, lp1, 0, 0, bNotCon,
                   NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE,
                   lpEnv, lp2, &StartupInfo, &ProcessInfo))
  {
     // return the 16-bit 'HTASK' from the THREAD ID

     iRval = (WORD)WOWHandle16((HANDLE)ProcessInfo.dwThreadId, WOW_TYPE_HTASK);

     SendMessage(hCaller, WM_USER, (WPARAM)12345, (LPARAM)ProcessInfo.dwProcessId);
  }
  else
  {
     iRval = GetLastError();

//     sprintf(tbuf, "Error %d returned", iRval);
//     MessageBox(NULL, tbuf, "* SFTSH32X.EXE *", MB_OK | MB_ICONINFORMATION);


     if(iRval == 7 || iRval == 9 || iRval == 14)
     {
        iRval = 0;  // out of memory or corrupted EXE
     }
     else if(iRval < 32)
     {
        // this is approximately correct - no change
     }
     else
     {
        iRval = 31; // unknown error code or general failure
     }
  }


  if(lpEnv) GlobalFree((HGLOBAL)lpEnv);

//  wsprintf(tbuf, "Return value = %d (%xH)", iRval, iRval);
//
//  MessageBox(NULL, tbuf, "* SFTSH32X.EXE *", MB_OK | MB_ICONINFORMATION);



  return(iRval);
}
