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

#ifndef EXTRAS_H
#define EXTRAS_H

#include <stdio.h>

/* removes all ctrl chars */
void cut_ctrl(char* message);

/* Is a character a space or tab? */
int is_blank(char c);

/* Moves a file into another directory. Returns 1 if success. */
int movefile(char* filename, char* directory);


/* Moves a file into another directory. Destination file is protected with
   a lock file during the operation. Returns 1 if success. */
int movefilewithdestlock(char* filename, char* directory);


/* removes ctrl chars at the beginning and the end of the text and removes */
/* \r in the text. */
void cutspaces(char* text);


/* removes all empty lines */
void cut_emptylines(char* text);


/* Checks if the text contains only numbers. */

int is_number(char* text);


/* Gets the first file that is not locked in the directory. Returns 0 if 
   there is no file. Filename is the filename including the path. 
   Additionally it cheks if the file grows at the moment to prevent
   that two programs acces the file at the same time. */
   int getfile( char*  dir, char*  filename);


/* Replacement for system() wich can be breaked. See man page of system() */
int my_system(char* command);

/* Create and remove a PID file */
void write_pid(char* filename);
void remove_pid(char* filename);


 

#endif
