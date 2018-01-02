# Microsoft Visual C++ Generated NMAKE File, Format Version 2.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

!IF "$(CFG)" == ""
CFG=Win32 Debug
!MESSAGE No configuration specified.  Defaulting to Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "Win32 Release" && "$(CFG)" != "Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "sftshl32.mak" CFG="Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

################################################################################
# Begin Project
# PROP Target_Last_Scanned "Win32 Debug"
MTL=MkTypLib.exe
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WinRel"
# PROP BASE Intermediate_Dir "WinRel"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "WinRel"
# PROP Intermediate_Dir "WinRel"
OUTDIR=.\WinRel
INTDIR=.\WinRel

ALL : $(OUTDIR)/sftshl32.exe $(OUTDIR)/sftshl32.bsc

$(OUTDIR) : 
    if not exist $(OUTDIR)/nul mkdir $(OUTDIR)

# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE CPP /nologo /W3 /GX /YX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /FR /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /c
# SUBTRACT CPP /YX /Fr
CPP_PROJ=/nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /Fo$(INTDIR)/\
 /c 
CPP_OBJS=.\WinRel/
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
RSC_PROJ=/l 0x409 /fo$(INTDIR)/"WINCMD.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_SBRS= \
	
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o$(OUTDIR)/"sftshl32.bsc" 

$(OUTDIR)/sftshl32.bsc : $(OUTDIR)  $(BSC32_SBRS)
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /NOLOGO /SUBSYSTEM:windows /MACHINE:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /NOLOGO /SUBSYSTEM:windows /MACHINE:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /NOLOGO /SUBSYSTEM:windows /INCREMENTAL:no\
 /PDB:$(OUTDIR)/"sftshl32.pdb" /MACHINE:I386 /DEF:".\WINCMD.DEF"\
 /OUT:$(OUTDIR)/"sftshl32.exe" 
DEF_FILE=.\WINCMD.DEF
LINK32_OBJS= \
	$(INTDIR)/WINCMD.res \
	$(INTDIR)/WINCMD_X.OBJ \
	$(INTDIR)/WINCMD_B.OBJ \
	$(INTDIR)/WINCMD_C.OBJ \
	$(INTDIR)/WINCMD_U.OBJ \
	$(INTDIR)/WINCMD_D.OBJ \
	$(INTDIR)/WINCMD.OBJ \
	$(INTDIR)/WINCMD_V.OBJ \
	$(INTDIR)/WCALC.OBJ \
	$(INTDIR)/mth32.obj \
	$(INTDIR)/winfmt32.obj

$(OUTDIR)/sftshl32.exe : $(OUTDIR)  $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WinDebug"
# PROP BASE Intermediate_Dir "WinDebug"
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "WinDebug"
# PROP Intermediate_Dir "WinDebug"
OUTDIR=.\WinDebug
INTDIR=.\WinDebug

ALL : $(OUTDIR)/sftshl32.exe $(OUTDIR)/sftshl32.bsc

$(OUTDIR) : 
    if not exist $(OUTDIR)/nul mkdir $(OUTDIR)

# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE CPP /nologo /W3 /GX /Zi /YX /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /FR /c
# ADD CPP /nologo /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /c
# SUBTRACT CPP /YX /Fr
CPP_PROJ=/nologo /W3 /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /Fo$(INTDIR)/ /Fd$(OUTDIR)/"sftshl32.pdb" /c 
CPP_OBJS=.\WinDebug/
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
RSC_PROJ=/l 0x409 /fo$(INTDIR)/"WINCMD.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_SBRS= \
	
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o$(OUTDIR)/"sftshl32.bsc" 

$(OUTDIR)/sftshl32.bsc : $(OUTDIR)  $(BSC32_SBRS)
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /NOLOGO /SUBSYSTEM:windows /DEBUG /MACHINE:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /NOLOGO /SUBSYSTEM:windows /DEBUG /MACHINE:I386
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib\
 odbccp32.lib /NOLOGO /SUBSYSTEM:windows /INCREMENTAL:yes\
 /PDB:$(OUTDIR)/"sftshl32.pdb" /DEBUG /MACHINE:I386 /DEF:".\WINCMD.DEF"\
 /OUT:$(OUTDIR)/"sftshl32.exe" 
DEF_FILE=.\WINCMD.DEF
LINK32_OBJS= \
	$(INTDIR)/WINCMD.res \
	$(INTDIR)/WINCMD_X.OBJ \
	$(INTDIR)/WINCMD_B.OBJ \
	$(INTDIR)/WINCMD_C.OBJ \
	$(INTDIR)/WINCMD_U.OBJ \
	$(INTDIR)/WINCMD_D.OBJ \
	$(INTDIR)/WINCMD.OBJ \
	$(INTDIR)/WINCMD_V.OBJ \
	$(INTDIR)/WCALC.OBJ \
	$(INTDIR)/mth32.obj \
	$(INTDIR)/winfmt32.obj

$(OUTDIR)/sftshl32.exe : $(OUTDIR)  $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

################################################################################
# Begin Group "Source Files"

################################################################################
# Begin Source File

SOURCE=.\WINCMD.RC
DEP_WINCM=\
	.\WINCMD.ICO\
	.\CARET1.BMP\
	.\CARET2.BMP\
	.\toolbar.bmp\
	.\SPIN_UP.BMP\
	.\SPIN_DWN.BMP\
	.\drivecd.bmp\
	.\drivefl3.bmp\
	.\drivefl5.bmp\
	.\drivehd.bmp\
	.\drivenet.bmp\
	.\driveram.bmp\
	.\drivesub.bmp\
	.\drivenon.bmp\
	.\FLDROPEN.BMP\
	.\FLDRGRAY.BMP\
	.\FLDRSHUT.BMP\
	.\WINCMD.H\
	.\WINCMD.VER

$(INTDIR)/WINCMD.res :  $(SOURCE)  $(DEP_WINCM) $(INTDIR)
   $(RSC) $(RSC_PROJ)  $(SOURCE) 

# End Source File
################################################################################
# Begin Source File

SOURCE=.\WINCMD_X.C
DEP_WINCMD=\
	.\MYWIN.H\
	.\MTH.H\
	.\WINCMD.H\
	.\WINCMD_F.H\
	.\WREXX.H\
	.\ON_WM.H\
	D:\SDK\h\TLHELP32.H\
	.\shlthunk.h\
	\WINDEV\INCLUDE\TOOLHELP.H\
	\msvc20\INCLUDE\SYS\TYPES.H\
	\msvc20\INCLUDE\SYS\STAT.H

$(INTDIR)/WINCMD_X.OBJ :  $(SOURCE)  $(DEP_WINCMD) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\WINCMD_B.C
DEP_WINCMD_=\
	.\MYWIN.H\
	.\MTH.H\
	.\WINCMD.H\
	.\WINCMD_F.H\
	.\ON_WM.H\
	D:\SDK\h\TLHELP32.H\
	.\shlthunk.h\
	\WINDEV\INCLUDE\TOOLHELP.H\
	\msvc20\INCLUDE\SYS\TYPES.H\
	\msvc20\INCLUDE\SYS\STAT.H

$(INTDIR)/WINCMD_B.OBJ :  $(SOURCE)  $(DEP_WINCMD_) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\WINCMD_C.C
DEP_WINCMD_C=\
	.\MYWIN.H\
	D:\SDK\h\DBT.H\
	.\MTH.H\
	.\WINCMD.H\
	.\WINCMD_F.H\
	.\ON_WM.H\
	D:\SDK\h\TLHELP32.H\
	.\shlthunk.h\
	\WINDEV\INCLUDE\TOOLHELP.H\
	\msvc20\INCLUDE\SYS\TYPES.H\
	\msvc20\INCLUDE\SYS\STAT.H

$(INTDIR)/WINCMD_C.OBJ :  $(SOURCE)  $(DEP_WINCMD_C) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\WINCMD_U.C
DEP_WINCMD_U=\
	.\MYWIN.H\
	.\MTH.H\
	.\WINCMD.H\
	.\WINCMD_F.H\
	.\ON_WM.H\
	D:\SDK\h\TLHELP32.H\
	.\shlthunk.h\
	\WINDEV\INCLUDE\TOOLHELP.H\
	\msvc20\INCLUDE\SYS\TYPES.H\
	\msvc20\INCLUDE\SYS\STAT.H

$(INTDIR)/WINCMD_U.OBJ :  $(SOURCE)  $(DEP_WINCMD_U) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\WINCMD_D.C
DEP_WINCMD_D=\
	.\MYWIN.H\
	.\MTH.H\
	.\WINCMD.H\
	.\WINCMD_F.H\
	.\ON_WM.H\
	D:\SDK\h\TLHELP32.H\
	.\shlthunk.h\
	\WINDEV\INCLUDE\TOOLHELP.H\
	\msvc20\INCLUDE\SYS\TYPES.H\
	\msvc20\INCLUDE\SYS\STAT.H

$(INTDIR)/WINCMD_D.OBJ :  $(SOURCE)  $(DEP_WINCMD_D) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\WINCMD.C
DEP_WINCMD_C6=\
	.\MYWIN.H\
	.\MTH.H\
	.\WINCMD.H\
	.\WINCMD_F.H\
	.\ON_WM.H\
	D:\SDK\h\TLHELP32.H\
	.\shlthunk.h\
	\WINDEV\INCLUDE\TOOLHELP.H\
	\msvc20\INCLUDE\SYS\TYPES.H\
	\msvc20\INCLUDE\SYS\STAT.H

$(INTDIR)/WINCMD.OBJ :  $(SOURCE)  $(DEP_WINCMD_C6) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\WINCMD_V.C
DEP_WINCMD_V=\
	.\MYWIN.H\
	.\WINCMD.H\
	.\WINCMD_F.H\
	.\WINCMD.VER\
	.\ON_WM.H\
	D:\SDK\h\TLHELP32.H\
	.\shlthunk.h\
	\WINDEV\INCLUDE\TOOLHELP.H\
	\msvc20\INCLUDE\SYS\TYPES.H\
	\msvc20\INCLUDE\SYS\STAT.H

$(INTDIR)/WINCMD_V.OBJ :  $(SOURCE)  $(DEP_WINCMD_V) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\WINCMD.DEF
# End Source File
################################################################################
# Begin Source File

SOURCE=.\WCALC.C
DEP_WCALC=\
	.\MYWIN.H\
	.\MTH.H\
	.\WINCMD.H\
	.\WINCMD_F.H\
	.\WCALC_F.H\
	.\ON_WM.H\
	D:\SDK\h\TLHELP32.H\
	.\shlthunk.h\
	\WINDEV\INCLUDE\TOOLHELP.H\
	\msvc20\INCLUDE\SYS\TYPES.H\
	\msvc20\INCLUDE\SYS\STAT.H

$(INTDIR)/WCALC.OBJ :  $(SOURCE)  $(DEP_WCALC) $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\mth32.c

$(INTDIR)/mth32.obj :  $(SOURCE)  $(INTDIR)

# End Source File
################################################################################
# Begin Source File

SOURCE=.\winfmt32.c

$(INTDIR)/winfmt32.obj :  $(SOURCE)  $(INTDIR)

# End Source File
# End Group
# End Project
################################################################################
