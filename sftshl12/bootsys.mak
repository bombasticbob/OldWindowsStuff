##################################################################################
#                                                                                #
# MAKE FILE for BOOTSYS.BIN (c) 1991-1993 by R. E. Frazier - all rights reserved #
#                                                                                #
##################################################################################



bootsys.obj: bootsys.asm
    ml /c /I. /Zi /Zm /Cu /Fobootsys.obj /Flbootsys.lst /Ta bootsys.asm 

bootsys.bin: bootsys.obj
    link /T bootsys.obj,bootsys.bin,;


