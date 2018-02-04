STAR32.SCR - a 32-bit Screen Saver for Windows

(Released as-is to public domain, February 2018)

NOTE:  in order to build this, you will need SCRNSAVE.H and SCRNSAVE.LIB
which were included in the Win32 SDK at the time Win '95 was released.
I do not recall if there is any restriction in re-distributing these files,
so they were not included with the source for the screen saver.

(You should still be able to find them in the latest Win32 SDK from
msdn.microsoft.com)

STAR32 was adapted from an existing public domain screen saver, 'STAR2',
also written by Bob Frazier, co-founder of SFT, Inc. STAR32 is 32-bit
version specifically written for Windows '9x/ME and NT/2k/XP5.  In
addition, STAR32 contains a MIDI file that can play in the background
whenever the screen saver appears, and a new feature that greatly
increases the depth of field when used in conjunction with 3D glasses.
You can use the screen saver configuration to turn on or off both of
these features.  By default, they are both off.

To install 'STAR32.SCR', copy the file 'STAR32.SCR' to your
"\WINDOWS\SYSTEM" directory on the drive containing Windows '9x/ME, or
the "\WINNT\SYSTEM32" directory on the drive containing Windows NT/2k/XP.
Then, open the 'Screen Saver' properties dialog tab by either
right-clicking the mouse when the cursor is on the desktop background
and selecting the 'screen saver' tab, or else by selecting the 'Display'
control panel applet from the system control panel, and selecting
the 'screen saver' tab.  The 'star32' screen saver should appear in
the list of available screen savers.  Activitating the selection will
start a 'mini-copy' of the screen saver on the dialog box, including
the background sound (if activated).  You may then select this screen
saver by pressing the 'ok' button.

To change the settings, press the 'settings' button on the 'screen saver'
dialog tab.  The Star32 screen saver configuration dialog box will then
appear.  Change the 'Number of stars' value to adjust how many stars are
shown.  Increasing the number gives you a more dense display, but can also
slow down screen painting.  Change the 'Warp Factor' to increase or reduce
the speed at which the stars come toward you. Change the 'update interval'
to determine how often the screen is repainted (in milliseconds).  Check or
uncheck the 'Play Background Music' checkbox to enable or disable the
background MIDI file.

The newest addition is the "Anagraphic" feature which requires 3D glasses.
The 3D glasses that work best are "red/cyan" types, which are usually
supplied for watching color 3D movies, with red on the left, cyan on the
right. A red/blue combination or red/green combination will probably work,
but the colors won't be right (it would shift to purple/yellow instead of
remaining white).  In 'Anagraphic' mode, the depth of field is greatly
increased, and the stars appear to shoot out of the screen (works best
when more than 1000 stars are being displayed and 'warp' is below 5).


NOTE:  your computer must be capable of playing MIDI files in order to hear
the background music.  Normally, a Sound Blaster or compatible sound card
is adequate for this purpose.  If you have 'wave tables' on your sound card,
the quality will be significantly better than standard OPL2/OPL3 synthesis.
"Windows sound system" and 'FM' cards should also work fine using Microsoft
"Software" MIDI Synthesis found in more recent versions of windows.



Please visit our INTERNET site at:

  http://www.mrp3.com/

The background sound can also be found in MIDI format at:

  http://www.mrp3.com/bobf/music.html
  (The file is titled 'Space Thriller')

