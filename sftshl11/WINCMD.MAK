#################################################################################
#                                                                               #
# MAKE FILE for WINCMD.EXE (c) 1991-1993 by R. E. Frazier - all rights reserved #
#                                                                               #
#################################################################################


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
other_exe_depend=$(mth_lib) wincmd.def wincmd.mak

obj_link=wincmd+wincmd_t+wincmdh+wincmd_b+wincmd_c+wincmd_d+wincmd_l+wincmd_u+wincmd_e+wincmd_s+wincmd_v+wincmd_x+wcalc+winfrmat+hugemem1

res_depend = wincmd.rc wincmd.ver wincmd.h wincmd.ver bootsys.bin \
             TOOLBAR.BMP DriveCD.BMP DriveFL3.BMP DriveFL5.BMP    \
             DriveHD.BMP DriveNET.BMP DriveRAM.BMP DriveSUB.BMP   \
             DriveNON.BMP FLDROPEN.BMP FLDRGRAY.BMP FLDRSHUT.BMP  \
             caret1.bmp caret2.bmp sftshfon.fon


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


all: SFTSHELL.exe SFTSHELL.HLP SETUP.EXE



bootsys.obj: bootsys.asm
    ml /c /I. /Zi /Zm /Cu /Fobootsys.obj /Flbootsys.lst /Ta bootsys.asm 

bootsys.bin: bootsys.obj
    link /T bootsys.obj,bootsys.bin,;


!IFDEF USE_PCH
mywin.pch: mywin.c mywin.h
    cl $(cl_options) $(optimize) $(pch_options) mywin.c

!ELSE
mywin.pch: NUL
   DEL mywin.pch >NUL

!ENDIF


$(mth_obj): mth.c mth.h
    cl -Fo$(mth_obj) $(cl_options) $(mth_opt) mth.c

$(mth_lib): $(mth_obj)
    del $(mth_lib)
    lib $(mth_lib) +$(mth_obj),;


winfrmat.obj: winfrmat.c $(pch_dep) wincmd.h
    cl -Fowinfrmat.obj $(cl_options) $(optimize) $(pch_use) winfrmat.c

wincmdh.obj: wincmdh.asm
    ml /c /I. /Zi /Zm /Cu /Fowincmdh.obj /Ta wincmdh.asm 

wincmd.obj: wincmd.c $(pch_dep) wincmd.h $(demo_force_build)
    cl -Fowincmd.obj $(cl_options) $(optimize) $(pch_use) wincmd.c

wincmd_t.obj: wincmd_t.c $(pch_dep) wincmd.h
    cl -Fowincmd_t.obj $(cl_options) $(optimize) $(pch_use) wincmd_t.c

wincmd_b.obj: wincmd_b.c $(pch_dep) wincmd.h
    cl -Fowincmd_b.obj $(cl_options) $(optimize) $(pch_use) wincmd_b.c

wincmd_c.obj: wincmd_c.c $(pch_dep) wincmd.h
!IFDEF QUICK
    cl -Fowincmd_c.obj -f $(cl_options) $(optimize_min) $(pch_use) wincmd_c.c
!ELSE
    cl -Fowincmd_c.obj $(cl_options) $(optimize) $(pch_use) wincmd_c.c
!ENDIF

wincmd_d.obj: wincmd_d.c $(pch_dep) wincmd.h
    cl -Fowincmd_d.obj $(cl_options) $(optimize2) $(pch_use) wincmd_d.c

wincmd_l.obj: wincmd_l.c $(pch_dep) wincmd.h
    cl -Fowincmd_l.obj $(cl_options) $(optimize) $(pch_use) wincmd_l.c

wincmd_u.obj: wincmd_u.c $(pch_dep) wincmd.h
    cl -Fowincmd_u.obj $(cl_options) $(optimize) $(pch_use) wincmd_u.c

wincmd_e.obj: wincmd_e.c $(pch_dep) wincmd.h
    cl -Fowincmd_e.obj $(cl_options) $(optimize) $(pch_use) wincmd_e.c

wincmd_s.obj: wincmd_s.c $(pch_dep) wincmd.h
    cl -Fowincmd_s.obj $(cl_options) $(optimize) $(pch_use) wincmd_s.c

wincmd_x.obj: wincmd_x.c $(pch_dep) wincmd.h
    cl -Fowincmd_x.obj $(cl_options) $(optimize) $(pch_use) wincmd_x.c

wcalc.obj: wcalc.c wcalc_f.h $(pch_dep) wincmd.h
    cl -Fowcalc.obj $(cl_options) $(optimize_max) $(pch_use) wcalc.c

hugemem1.obj: hugemem1.asm
    ml /c /I. /Zi /Zm /Cu /Fohugemem1.obj /Flhugemem1.lst /Ta hugemem1.asm
#    masm /ZI /MU hugemem1.asm,hugemem1,hugemem1;



# BUILDING THE 'FON' FILE FOR RC COMPILER


sftshfon.obj:  sftshfon.asm
   ml -c sftshfon.asm

sftshfon.res:  sftshfon.rc sftshel5.fnt sftshel6.fnt sftshel7.fnt \
               sftshel8.fnt sftshl12.fnt sftshl18.fnt
   rc -r sftshfon.rc

sftshfon.fon:  sftshfon.obj sftshfon.res sftshfon.def
   link4 /NOD sftshfon.obj,sftshfon.fon,,,sftshfon.def;
   rc -k -30 sftshfon.res sftshfon.fon








!IFDEF DEBUG

wincmd_v.obj: wincmd_v.c $(pch_dep) wincmd.h wincmd.ver $(demo_force_build)
    cl -Fowincmd_v.obj $(cl_options) $(optimize) $(pch_use) wincmd_v.c


wincmd.res: $(res_depend)
    rc -r wincmd.rc


!ELSE

wincmd_v.obj: wincmd_v.c $(pch_dep) wincmd.h wincmd.ver $(obj_depend) $(other_exe_depend) $(force_link)
    autover wincmd.ver
    cl -Fowincmd_v.obj $(cl_options) $(optimize) $(pch_use) wincmd_v.c


wincmd.res: $(res_depend) wincmd_v.obj
    rc -r wincmd.rc


!ENDIF




SFTSHELL.exe:: wincmd_v.obj $(obj_depend) $(other_exe_depend) $(force_link)
    link /NOD:mlibce /NOE /MAP /LI $(link_opt) /NOPACKC /SEG:512 /ON:N @<<SFTSHELL.LNK
    $(obj_link),
    SFTSHELL,SFTSHELL,
    $(lib_files),
    wincmd.def;
<<KEEP


# do this whenever the one above is done (and afterwards), or 'wincmd.res'
# changes.  It *must* have the same dependencies plus 'wincmd.res'!!

SFTSHELL.exe:: wincmd.res wincmd_v.obj $(obj_depend) $(other_exe_depend) $(force_link)
    rc -t -30 wincmd.res SFTSHELL.EXE





# HELP FILE

SFTSHELL.HLP: SFTSHEL1.RTF SFTSHEL2.RTF SFTSHEL3.RTF SFTSHEL4.RTF SFTSHELL.HPJ
   D:\SDK\BIN\HC31 SFTSHELL.HPJ




# 'SETUP' program


SETUP.OBJ: SETUP.C
    cl -Fosetup.obj $(cl_options) $(optimize) $(pch_use) setup.c


!IFDEF DEBUG

SETUP.RES: SETUP.RC SETUP.VER
    rc -r -FoSETUP.RES SETUP.RC

!ELSE

SETUP.RES: SETUP.RC SETUP.VER SETUP.OBJ SETUP.DEF
    autover SETUP.VER
    rc -r -FoSETUP.RES SETUP.RC

!ENDIF


SETUP.EXE: SETUP.OBJ SETUP.RES SETUP.DEF
    link /NOD:mlibce /NOE /MAP /LI $(link_opt) /NOPACKC /SEG:512 /ON:N @<<SFTSHELL.LNK
    SETUP,
    SETUP,
    SETUP,
    $(lib_files),
    setup.def;
<<
    rc -t -30 setup.res setup.exe
