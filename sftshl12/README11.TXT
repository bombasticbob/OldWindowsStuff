          ** DO NOT RE-DISTRIBUTE SFTShell OR THIS DOCUMENT! **

This is a preliminary evaluation copy of SFTShell release 1.10.  At this time
it is not yet feature complete.  Several key capabilities, including ODBC
support, have not yet been implemented.  The user interface appearance,
specifically the operation of the tool bar and status bar, is complete.  Also,
several new features have been added to the application.  Feel free to make
any comments to me on CIS at '70172,177', or '70172.177@compuserve.com'.


KNOWN PROBLEMS:
---------------

1) 'Remove Item' buttons on QUEUE dialog are non-functional.
2) Parts of the 'ODBC' command set does not function, specifically database
   update commands and transaction-oriented commands.
3) The 'Teeny' and 'Tiny' fonts do not contain the correct characters below
   ASCII 32.  This will be rectified at a future date.
4) The 'Associate' dialog does not display any document types under Win 3.x.
5) A number of features have not yet been included in the HELP file, though
   the sample programs have been improved to use them.  These include the
   IF/ELSE/ENDIF, FOR/NEXT, REPEAT/UNTIL, and WHILE/WEND commands.
6) The ODBC 'DSN connect' dialog does not always respond properly to arrow
   keys.  This may not be due to SFTShell, but I plan on taking a closer look
   at this problem in the future.
7) Screen repaints may be painfully slow on 16Mhz and 20Mhz 386 CPU's. The
   screen scroller will be better optimized prior to product release.


NEW FEATURES NOT PRESENT IN PREVIOUS RELEASES:
----------------------------------------------

Tool Bar user interface added.
Main Menu restructured for better Tool Bar support.
'Instant Batch' tool bar button and menu selection (under 'Edit') allow you
to create quick batch files and execute them from memory.
'Queue' dialog allows user to suspend and resume background operations, as well
as remove items from the queues.
'Folder' dialog provides a graphical user interface for changing the current
drive and directory.
Additional font sizes have been added, using an internal font resource.
Custom font selections are now supported.
The 'Associate' dialog allows you to associate a particular file extension with
a known application type.
The 'Paste' menu allows you to execute the clipboard contents as though it
were a batch file.
The 'Erase Command History' menu provides a means of clearing the command
history buffer.  This is useful if you wish to duplicate a series of commands
using the clipboard to store them.
The language has been enhanced to include FOR/NEXT and IF/ELSE/ENDIF, as well
as REPEAT/UNTIL and WHILE/WEND.  The 'EXIT LOOP' and 'CONTINUE' commands
let you exit a loop, or jump to the end and continue iterating.
The 'ODBC' command allows you to embed SQL into a BATCH LANGUAGE application,
thus giving you the benefit of a large DBMS server application from an ODBC
client.  SFTShell uses ODBC 2.0 and has been tested with several different
drivers for read access.  Write access via cursors is not supported in this
release, although SQL commands that perform updates may be executed.  See the
ODBCDEMO.BAT sample application for an example of how the ODBC commands work.


(c) 1995 by Stewart~Frazier Tools, Inc. - all rights reserved
