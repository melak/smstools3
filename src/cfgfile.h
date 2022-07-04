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

#ifndef CFGFILE_H
#define CFGFILE_H

#include <stdio.h>

/*  Gets a single parameter from a list of parameters wich uses colons
to separate them. Returns 1 if successful. */

int getsubparam( char*  parameter, 
                 int n, 
                char*  subparam,  int size_subparam);


/* Converts a string to a boolean value. The string can be:
   1=  true, yes, on, 1
   0 = all other strings
   Only the first character is significant. */
		
int yesno( char*  value);


/* Searches for a section [name] in a config file and goes to the
   next line. Return 1 if successful. */
   
int gotosection(FILE* file,  char*  name);



/* Reads the next line from a config file beginning at the actual position.
   Returns 1 if successful. If the next section or eof is encountered it 
   returns 0. If the file contains syntax error it returns -1 and the wrong
   line in value.*/

int my_getline(FILE* file, 
            char*  name,  int size_name,
	    char*  value,  int size_value);
	    
#endif
