###################################################################################
#                                                                                 #
# MAKE FILE for SFTSHELL.EXE (c) 1991-1993 by R. E. Frazier - all rights reserved #
#                                                                                 #
###################################################################################


#DEBUG=1

!IFDEF DEBUG

link_opt=/COD
force_link=NUL

mth_lib=mth_d.lib
mth_obj=mth_d.obj
mth_opt=-Od -DDEBUG


!IFDEF DEMO

cl_defines=/DDEMO
demo_force_build=NUL

!ELSE

!IFDEF NODEMO
demo_force_build=NUL
!ELSE
demo_force_build=
!ENDIF

!ENDIF


!ELSE

link_opt=

mth_lib=mth.lib
mth_obj=mth.obj
mth_opt=-Ocilntw

!IFDEF NODEBUG
force_link=NUL
!ELSE
force_link=
!ENDIF
!ENDIF


!IFDEF RXSHELL
cl_defines=/DRXSHELL
!ELSE

cl_defines=

#DEMO VERSION!!!

!IFDEF DEMO

cl_defines=/DDEMO
demo_force_build=NUL

!ELSE

!IFDEF NODEMO
demo_force_build=NUL
!ELSE
demo_force_build=
!ENDIF

!ENDIF


!ENDIF





#obj_depend=wincmd.obj wincmdh.obj wincmd_b.obj wincmd_c.obj wincmd_d.obj wincmd_l.obj wincmd_u.obj wincmd_v.obj wincmd_x.obj wcalc.obj winfrmat.obj hugemem1.obj
obj_depend=wincmd.obj wincmd_t.obj wincmdh.obj wincmd_b.obj wincmd_c.obj wincmd_d.obj wincmd_l.obj wincmd_u.obj wincmd_e.obj wincmd_s.obj wincmd_x.obj wcalc.obj winfrmat.obj hugemem1.obj
other_exe_depend=$(mth_lib) sftshell.def sftshell.mak

obj_link=wincmd+wincmd_t+wincmdh+wincmd_b+wincmd_c+wincmd_d+wincmd_l+wincmd_u+wincmd_e+wincmd_s+wincmd_v+wincmd_x+wcalc+winfrmat+hugemem1

res_depend = sftshell.rc sftshell.ver wincmd.h \
             res\bootsys.bin     \
             res\TOOLBAR.BMP     \
             res\DriveCD.BMP     \
             res\DriveFL3.BMP    \
             res\DriveFL5.BMP    \
             res\DriveHD.BMP     \
             res\DriveNET.BMP    \
             res\DriveRAM.BMP    \
             res\DriveSUB.BMP    \
             res\DriveNON.BMP    \
             res\FLDROPEN.BMP    \
             res\FLDRGRAY.BMP    \
             res\FLDRSHUT.BMP    \
             res\caret1.bmp      \
             res\caret2.bmp      \
             sftshfon.fon


lib_files=libw.lib mlibcewm.lib win87em.lib $(mth_lib) oldnames.lib

!IFDEF STACK_CHECK
code_opts=-G2ew 
!ELSE
code_opts=-G2sw
!ENDIF

cl_options=-c -AMw -FPi -Lw -W3 -Zeilp2 $(cl_defines) $(code_opts)

optimize=-Ocilntw
optimize2=-Octw
optimize_min=-Ocit
#optimize_max=-Ob2cegilrtw
optimize_max=-Ob1ceilrtwn

!IFDEF USE_PCH
pch_options=-Yd -Yc
pch_use=-YuMYWIN.H
pch_dep=MYWIN.PCH
!ELSE
pch_options=
pch_use=
pch_dep=
!ENDIF


all: SFTSHELL.exe



!IFDEF USE_PCH
mywin.pch: mywin.cpp mywin.h
    cl $(cl_options) $(optimize) $(pch_options) mywin.cpp

!ELSE
mywin.pch: NUL
   DEL mywin.pch >NUL

!ENDIF


$(mth_obj): mth.cpp mth.h
    cl -Fo$(mth_obj) $(cl_options) $(mth_opt) mth.cpp

$(mth_lib): $(mth_obj)
    del $(mth_lib)
    lib $(mth_lib) +$(mth_obj),;


winfrmat.obj: winfrmat.cpp $(pch_dep) wincmd.h mth.h
    cl -Fowinfrmat.obj $(cl_options) $(optimize) $(pch_use) winfrmat.cpp

wincmdh.obj: wincmdh.asm
    ml /c /I. /Zi /Zm /Cu /Fowincmdh.obj /Ta wincmdh.asm 

wincmd.obj: wincmd.cpp $(pch_dep) wincmd.h $(demo_force_build) mth.h
    cl -Fowincmd.obj $(cl_options) $(optimize) $(pch_use) wincmd.cpp

wincmd_t.obj: wincmd_t.cpp $(pch_dep) wincmd.h mth.h
    cl -Fowincmd_t.obj $(cl_options) $(optimize) $(pch_use) wincmd_t.cpp

wincmd_b.obj: wincmd_b.cpp $(pch_dep) wincmd.h mth.h
    cl -Fowincmd_b.obj $(cl_options) $(optimize) $(pch_use) wincmd_b.cpp

wincmd_c.obj: wincmd_c.cpp $(pch_dep) wincmd.h mth.h
!IFDEF QUICK
    cl -Fowincmd_c.obj -f $(cl_options) $(optimize_min) $(pch_use) wincmd_c.cpp
!ELSE
    cl -Fowincmd_c.obj $(cl_options) $(optimize) $(pch_use) wincmd_c.cpp
!ENDIF

wincmd_d.obj: wincmd_d.cpp $(pch_dep) wincmd.h mth.h
    cl -Fowincmd_d.obj $(cl_options) $(optimize2) $(pch_use) wincmd_d.cpp

wincmd_l.obj: wincmd_l.cpp $(pch_dep) wincmd.h mth.h
    cl -Fowincmd_l.obj $(cl_options) $(optimize) $(pch_use) wincmd_l.cpp

wincmd_u.obj: wincmd_u.cpp $(pch_dep) wincmd.h mth.h
    cl -Fowincmd_u.obj $(cl_options) $(optimize) $(pch_use) wincmd_u.cpp

wincmd_e.obj: wincmd_e.cpp $(pch_dep) wincmd.h mth.h
    cl -Fowincmd_e.obj $(cl_options) $(optimize) $(pch_use) wincmd_e.cpp

wincmd_s.obj: wincmd_s.cpp $(pch_dep) wincmd.h mth.h
    cl -Fowincmd_s.obj $(cl_options) $(optimize) $(pch_use) wincmd_s.cpp

wincmd_x.obj: wincmd_x.cpp $(pch_dep) wincmd.h mth.h
    cl -Fowincmd_x.obj $(cl_options) $(optimize) $(pch_use) wincmd_x.cpp

wcalc.obj: wcalc.cpp wcalc_f.h $(pch_dep) wincmd.h mth.h
    cl -Fowcalc.obj $(cl_options) $(optimize_max) $(pch_use) wcalc.cpp

hugemem1.obj: hugemem1.asm
    ml /c /I. /Zi /Zm /Cu /Fohugemem1.obj /Flhugemem1.lst /Ta hugemem1.asm
#    masm /ZI /MU hugemem1.asm,hugemem1,hugemem1;





!IFDEF DEBUG

wincmd_v.obj: wincmd_v.cpp $(pch_dep) wincmd.h mth.h sftshell.ver $(demo_force_build)
    cl -Fowincmd_v.obj $(cl_options) $(optimize) $(pch_use) wincmd_v.cpp


sftshell.res: $(res_depend)
    rc -r sftshell.rc sftshell.res


!ELSE

wincmd_v.obj: wincmd_v.cpp $(pch_dep) wincmd.h mth.h sftshell.ver $(obj_depend) $(other_exe_depend) $(force_link)
    autover sftshell.ver
    cl -Fowincmd_v.obj $(cl_options) $(optimize) $(pch_use) wincmd_v.cpp


sftshell.res: $(res_depend) wincmd_v.obj
    rc -r sftshell.rc sftshell.res


!ENDIF




SFTSHELL.exe:: wincmd_v.obj $(obj_depend) $(other_exe_depend) $(force_link)
    link /NOD:mlibce /NOE /MAP /LI $(link_opt) /NOPACKC /SEG:512 /ON:N @<<SFTSHELL.LNK
    $(obj_link),
    SFTSHELL,SFTSHELL,
    $(lib_files),
    sftshell.def;
<<KEEP


# do this whenever the one above is done (and afterwards), or 'wincmd.res'
# changes.  It *must* have the same dependencies plus 'wincmd.res'!!

SFTSHELL.exe:: sftshell.res wincmd_v.obj $(obj_depend) $(other_exe_depend) $(force_link)
    rc -t -30 sftshell.res SFTSHELL.EXE




