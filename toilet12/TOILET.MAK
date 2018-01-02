#          MEMLEFT.MAK

!IFDEF DEBUG
cv_info=/COD /MAP /LI
assert_link=NUL
!ELSE
cv_info=/MAP /LI
!IFDEF NODEBUG
assert_link=NUL
!ELSE
assert_link=
!ENDIF
!ENDIF



optimize = -Oct
linkparm = /NOD /NOE 
warnings = -W3

all: toilet.exe

toilet.res: toilet.rc toilet.h toilet1.ico toilet2.ico toilet.cur toilet.wav
    rc -r toilet.rc

toilet.obj: toilet.c toilet.h
    cl -c -AMw -G2sw $(optimize) $(warnings) -Zeilp toilet.c

toilet.exe:: toilet.obj toilet.def toilet.mak $(assert_link)
    link $(linkparm) $(cv_info) toilet,toilet,toilet,Mlibcew oldnames libw shell mmsystem commdlg, toilet.def;

toilet.exe:: toilet.res toilet.obj toilet.def toilet.mak $(assert_link)
    rc -31 -t toilet.res
