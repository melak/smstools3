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

#ifndef LOCKING_H
#define LOCKING_H

/* Locks a file and returns 1 if successful */

int lockfile( char*  filename);


/* Checks, if a file is locked */

int islocked( char*  filename);


/* Unlocks a file */

int un_lock( char*  filename);

#endif
