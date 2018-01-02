** PLEASE READ THIS FIRST, OK? **

The program 'WINXRAY.EXE' and its associated help file
'WINXRAY.HLP' are *INCOMPLETE*!  Please do not distribute
these files (yet) until I have finished with them.  There
are mistakes in the HELP FILE and certain features in the
program are incomplete or "not yet reliable".

Some new features have been added which are CHICAGO-specific
(like the VxD list) or use VxD calls that are specific to
Chicago. This includes a bug fix for problems that began with
Chicago beta release #1 (M6), due to the use of undocumented
features that changed from M5 to M6.  This should no longer be 
an issue, since Chicago now provides similar functionality 
through an extension of the VxD services over that of Win 3.x .

This is *STILL* a 16-bit application, and it will not work 
if run under Windows NT.  It has been reported to me that
NT will not crash if you run this program, but I don't suggest
you test it for yourself.

The 'PAGE MAP' function is still slightly inaccurate.  Those
pages which are 'not present' may either be marked 'RESERVED'
or 'PAGING FILE'.  This is misleading, due to lack of proper
documentation on the use of the page flags.  Once I have 
determined how they are applied, I will fix this problem.
Also, pages labelled 'Global' and mapped to a particular VM
are not accurate under Chicago, especially because of Win32.



WINXRAY is designed to be an advertising platform for
Stewart~Frazier Tools, Inc.  and as such will be a FREE
software package.  The purpose of this program is to provide
a useful tool for developers and "power users" with a major
plug for our company.  It is, to some extent, a 'concerto'
program in which we 'strut our stuff' a bit, and do some
relatively impossible things, and include a help file to
explain the technical end of things.
