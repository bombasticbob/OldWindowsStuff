#############################################################################
#                          MAKE FILE FOR STAR32                             #
#############################################################################

#DEBUG=TRUE     #allows debugging, removes optimization

obj_list    = star32.obj
map_file    = star32
exe_file    = star32
res_file    = star32
headerfiles = star32.h resource.h fixpoint.h

exe_depend  = 
def_file    = star32.DEF
lib_file    = scrnsave.lib libc.LIB KERNEL32.LIB USER32.LIB GDI32.LIB ADVAPI32.LIB WINMM.LIB

!IFDEF DEBUG
optimize = -Ot
linkparm = /OPT:REF /DEBUG /SUBSYSTEM:WINDOWS /MACHINE:i386
#linkparm = /OPT:REF /DEBUG /MACHINE:i386
warnings = -W3
!ELSE
optimize = -Ogitw
linkparm = /OPT:REF /SUBSYSTEM:WINDOWS /MACHINE:i386
#linkparm = /OPT:REF /MACHINE:i386
warnings = -W1
!ENDIF

PATH=$(PATH_SEG32);$(PATH)
INCLUDE=C:\SDK\INCLUDE;$(INCLUDE32)
LIB=$(LIB32)

all: $(exe_file).scr


#############################################################################
#                          *** RESOURCE FILE ***                            #
#############################################################################

star32.res: star32.rc star32.rc2 resource.h
    rc -R star32.rc


#############################################################################
#                          *** OBJECT FILES ***                             #
#############################################################################


star32.obj: star32.cpp $(headerfiles)
    cl -c -G3 $(optimize) $(warnings) -Zeilp star32.cpp


#############################################################################
#                     *** GENERATING THE .SCR FILE ***                      #
#############################################################################


$(exe_file).scr: $(obj_list) $(def_file) $(exe_depend) star32.res star32.mak
    link $(linkparm) @<<star32.LNK
        $(obj_list)
        $(res_file).res
        $(lib_file)
        /OUT:$(exe_file).scr
        /MAP:$(map_file).map
        /DEF:$(def_file)
<<



############################ END OF MAKE FILE ###############################
