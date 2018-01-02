# MAKE FILE FOR SHLTHUNK.DLL
# 16-bit definitions are in effect!!!

all: SHLTHUNK.DLL winrel\shlth32.obj windebug\shlth32.obj


INCLUDE=$(INCLUDE16)
LIB=$(LIB16)
PATH=$(PATH_SEG16);$(PATH_SEG32);$(PATH_SEG)


shlthunk.asm: shlthunk.thk
  thunk shlthunk.thk


shlthdll.obj: shlthdll.c shlthunk.h
  cl -c -ALw -GD -FPi -Zeip2 -Foshlthdll.obj shlthdll.c


shlth16.obj: shlthunk.asm
  ml /DIS_16 /c /W3 /Zi /FPi /Foshlth16.obj shlthunk.asm


winrel\shlth32.obj: shlthunk.asm
  ml /DIS_32 /c /coff /W3 /FPi /Fowinrel\shlth32.obj shlthunk.asm


windebug\shlth32.obj: shlthunk.asm
  ml /DIS_32 /c /coff /W3 /FPi /Zi /Fowindebug\shlth32.obj shlthunk.asm


shlthunk.res: shlthunk.rc
  rc -r shlthunk.rc shlthunk.res


shlthunk.dll: shlth16.obj shlthdll.obj shlthunk.def shlthunk.mak
  link /NOE /NOPACKC shlthdll+shlth16.obj,shlthunk,shlthunk,ldllcew toolhelp shell commdlg,shlthunk.def;
  rc -40 -t shlthunk.res shlthunk.dll

