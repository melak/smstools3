/*
SMS Server Tools
Copyright (C) Stefan Frings

This program is free software unless you got it under another license directly
from the author. You can redistribute it and/or modify it under the terms of 
the GNU General Public License as published by the Free Software Foundation.
Either version 2 of the License, or (at your option) any later version.

http://stefanfrings.de/
mailto: stefan@stefanfrings.de
*/

#ifndef LOGGING_H
#define LOGGING_H

int openlogfile(char* filename,char* progname,int facility,int level);

// if filename if 0, "" or "syslog": opens syslog. Level is ignored.
// else: opens a log file. Facility is not used. Level specifies the verbosity (9=highest).
// If the filename is a number it is interpreted as the file handle and 
// duplicated. The file must be already open. 
// Returns the file handle to the log file.


void closelogfile();

// closes a log file.


void writelogfile(int severity,char* progname,char* format, ...);

// write to a log file. 2nd and following arguments are like in printf.

#endif
