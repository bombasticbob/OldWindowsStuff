# make file for 'sftsh32x.exe' (32-bit "helper" for SFTShell)

all: sftsh32x.exe


!IFDEF DEBUG

sftsh32x.exe: sftsh32x.c
  cl -Od -Zi7 sftsh32x.c /link /DEBUGTYPE:BOTH /PDB:NONE wow32.lib kernel32.lib user32.lib shell32.lib advapi32.lib

!ELSE #DEBUG

sftsh32x.exe: sftsh32x.c
  cl sftsh32x.c /link /PDB:NONE wow32.lib kernel32.lib user32.lib shell32.lib advapi32.lib

!ENDIF #DEBUG
