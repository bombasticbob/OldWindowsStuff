
goal: flip.scr

flip.obj:    flip.c
    cl -c -W3 -AS -G2sw -Zeilp -Od -DSTRICT flip.c

flip.res: flip.rc flip.ico flip.h
    rc -r flip.rc

flip.scr::    flip.obj flip.def
    link /NOD/NOE/MAP @<<
    flip.obj,
    flip.scr,
    flip.map,
    scrnsave libw slibcew mmsystem,
    flip.def
<<

flip.scr::    flip.obj flip.def flip.res
    rc flip.res flip.scr
