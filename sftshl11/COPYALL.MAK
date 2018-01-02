DEST=B:^\

all: $(DEST)sftshell.exe $(DEST)sftshell.wri $(DEST)sftshell.hlp \
     $(DEST)sftsh32x.exe $(DEST)alarm.bat $(DEST)calculat.bat \
     $(DEST)controlr.bat $(DEST)dircomp.bat $(DEST)mydir.bat \
     $(DEST)odbcdemo.bat $(DEST)wininfo.bat \
     $(DEST)setup.exe $(DEST)sftsetup.bat

$(DEST)sftshell.exe: sftshell.exe
  copy $? $@

$(DEST)sftshell.wri: sftshell.wri
  copy $? $@

$(DEST)sftshell.hlp: sftshell.hlp
  copy $? $@

$(DEST)sftsh32x.exe: sftsh32x.exe
  copy $? $@

$(DEST)alarm.bat: alarm.bat
  copy $? $@

$(DEST)calculat.bat: calculat.bat
  copy $? $@

$(DEST)controlr.bat: controlr.bat
  copy $? $@

$(DEST)dircomp.bat: dircomp.bat
  copy $? $@

$(DEST)mydir.bat: mydir.bat
  copy $? $@

$(DEST)odbcdemo.bat: odbcdemo.bat
  copy $? $@

$(DEST)wininfo.bat: wininfo.bat
  copy $? $@

$(DEST)setup.exe: setup.exe
  copy $? $@

$(DEST)sftsetup.bat: sftsetup.bat
  copy $? $@

