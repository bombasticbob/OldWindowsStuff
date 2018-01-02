###################################################################################
#                                                                                 #
# MAKE FILE for SFTSHFON.FON (c) 1991-1993 by R. E. Frazier - all rights reserved #
#                                                                                 #
###################################################################################




# BUILDING THE 'FON' FILE FOR RC COMPILER


sftshfon.obj:  sftshfon.asm
   ml -c sftshfon.asm

sftshfon.res:  sftshfon.rc sftshel5.fnt sftshel6.fnt sftshel7.fnt \
               sftshel8.fnt sftshl12.fnt sftshl18.fnt
   rc -r sftshfon.rc

sftshfon.fon:  sftshfon.obj sftshfon.res sftshfon.def
   link4 /NOD sftshfon.obj,sftshfon.fon,,,sftshfon.def;
   rc -k -30 sftshfon.res sftshfon.fon


