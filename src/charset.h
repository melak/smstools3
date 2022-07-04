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

#ifndef CHARSET_H
#define CHARSET_H

// Both functions return the size of the converted string
// max limits the number of characters to be written into
// destination
// size is the size of the source string
// max is the maximum size of the destination string
// The GSM character set contains 0x00 as a valid character

int gsm2iso(char* source, int size, char* destination, int max);

int iso2gsm(char* source, int size, char* destination, int max);

#endif
