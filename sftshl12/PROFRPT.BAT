@ECHO OFF
C:\PROF\BIN\PREP /IT SFTSHELL.PBT /IO SFTSHELL.PBO /OT SFTSHELL.PBT
C:\PROF\BIN\PLIST /ST SFTSHELL.PBT >SFTSHELL.OUT
DEL SFTSHELL.PBT >NUL
DEL SFTSHELL.PBI >NUL
DEL SFTSHELL.PBO >NUL
