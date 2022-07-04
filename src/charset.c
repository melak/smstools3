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

#include "charset.h"
#include "logging.h"
#include <syslog.h>

// iso = ISO8859-15 (you might change the table to any other 8-bit character set)
// sms = sms character set used by mobile phones

//                  iso   sms
char charset[] = { '@' , 0x00,
		   0xA3, 0x01,
		   '$' , 0x02,
		   0xA5, 0x03,
		   0xE8, 0x04,
		   0xE9, 0x05,
		   0xF9, 0x06,
		   0xEC, 0x07,
		   0xF2, 0x08,
		   0xC7, 0x09,
		   0x0A, 0x0A,
		   0xD8, 0x0B,
		   0xF8, 0x0C,
		   0x0D, 0x0D,
		   0xC5, 0x0E,
		   0xE5, 0x0F,

// ISO8859-7, Capital greek characters
//		   0xC4, 0x10,
//		   0x5F, 0x11,
//		   0xD6, 0x12,
//		   0xC3, 0x13,
//		   0xCB, 0x14,
//		   0xD9, 0x15,
//		   0xD0, 0x16,
//		   0xD8, 0x17,
//		   0xD3, 0x18,
//		   0xC8, 0x19,
//		   0xCE, 0x1A,

// ISO8859-1, ISO8859-15
		   0x81, 0x10,
		   0x5F, 0x11,
		   0x82, 0x12,
		   0x83, 0x13,
		   0x84, 0x14,
		   0x85, 0x15,
		   0x86, 0x16,
		   0x87, 0x17,
		   0x88, 0x18,
		   0x89, 0x19,
		   0x8A, 0x1A,

		   0x1B, 0x1B,
		   0xC6, 0x1C,
		   0xE6, 0x1D,
		   0xDF, 0x1E,
		   0xC9, 0x1F,
		   ' ' , 0x20,
		   '!' , 0x21,
		   0x22, 0x22,
		   '#' , 0x23,
		   '%' , 0x25,
		   '&' , 0x26,
		   0x27, 0x27,
		   '(' , 0x28,
		   ')' , 0x29,
		   '*' , 0x2A,
		   '+' , 0x2B,
		   ',' , 0x2C,
		   '-' , 0x2D,
		   '.' , 0x2E,
		   '/' , 0x2F,
		   '0' , 0x30,
		   '1' , 0x31,
		   '2' , 0x32,
		   '3' , 0x33,
		   '4' , 0x34,
		   '5' , 0x35,
		   '6' , 0x36,
		   '7' , 0x37,
		   '8' , 0x38,
		   '9' , 0x39,
		   ':' , 0x3A,
		   ';' , 0x3B,
		   '<' , 0x3C,
		   '=' , 0x3D,
		   '>' , 0x3E,
		   '?' , 0x3F,
		   0xA1, 0x40,
		   'A' , 0x41,
		   'B' , 0x42,
		   'C' , 0x43,
		   'D' , 0x44,
		   'E' , 0x45,
		   'F' , 0x46,
		   'G' , 0x47,
		   'H' , 0x48,
		   'I' , 0x49,
		   'J' , 0x4A,
		   'K' , 0x4B,
		   'L' , 0x4C,
		   'M' , 0x4D,
		   'N' , 0x4E,
		   'O' , 0x4F,
		   'P' , 0x50,
		   'Q' , 0x51,
		   'R' , 0x52,
		   'S' , 0x53,
		   'T' , 0x54,
		   'U' , 0x55,
		   'V' , 0x56,
		   'W' , 0x57,
		   'X' , 0x58,
		   'Y' , 0x59,
		   'Z' , 0x5A,
		   0xC4, 0x5B,
		   0xD6, 0x5C,
		   0xD1, 0x5D,
		   0xDC, 0x5E,
		   0xA7, 0x5F,
		   0xBF, 0x60,
		   'a' , 0x61,
		   'b' , 0x62,
		   'c' , 0x63,
		   'd' , 0x64,
		   'e' , 0x65,
		   'f' , 0x66,
		   'g' , 0x67,
		   'h' , 0x68,
		   'i' , 0x69,
		   'j' , 0x6A,
		   'k' , 0x6B,
		   'l' , 0x6C,
		   'm' , 0x6D,
		   'n' , 0x6E,
		   'o' , 0x6F,
		   'p' , 0x70,
		   'q' , 0x71,
		   'r' , 0x72,
		   's' , 0x73,
		   't' , 0x74,
		   'u' , 0x75,
		   'v' , 0x76,
		   'w' , 0x77,
		   'x' , 0x78,
		   'y' , 0x79,
		   'z' , 0x7A,
		   0xE4, 0x7B,
		   0xF6, 0x7C,
		   0xF1, 0x7D,
		   0xFC, 0x7E,
		   0xE0, 0x7F,
		   0x60, 0x27,
                   0xE1, 0x61,  // replacement for accented a
                   0xED, 0x69,  // replacement for accented i
                   0xF3, 0x6F,  // replacement for accented o
                   0xFA, 0x75,  // replacement for accented u
		   0   , 0     // End marker
		 };

// Extended characters. In GSM they are preceeded by 0x1B.

char ext_charset[] = { 0x0C, 0x0A,
                       '^' , 0x14,
		       '{' , 0x28,
		       '}' , 0x29,
		       '\\', 0x2F,
		       '[' , 0x3C,
		       '~' , 0x3D,
		       ']' , 0x3E,
		       0x7C, 0x40,
		       0xA4, 0x65,
		       0   , 0     // End marker
	             };


int iso2gsm(char* source, int size, char* destination, int max)
{
  int table_row;
  int source_count=0;
  int dest_count=0;
  int found=0;
  destination[dest_count]=0;
  if (source==0 || size==0)
    return 0;
  // Convert each character untl end of string
  while (source_count<size && dest_count<max)
  {
    // search in normal translation table
    found=0;
    table_row=0;
    while (charset[table_row*2])
    {
      if (charset[table_row*2]==source[source_count])
      {
	destination[dest_count++]=charset[table_row*2+1];
 	found=1;
	break;
      }
      table_row++;
    }
    // if not found in normal table, then search in the extended table
    if (found==0)
    {
      table_row=0;
      while (ext_charset[table_row*2])
      {
        if (ext_charset[table_row*2]==source[source_count])
        {
          destination[dest_count++]=0x1B;
          if (dest_count<max)
	    destination[dest_count++]=ext_charset[table_row*2+1];
	  found=1;
	  break;
	}
	table_row++;
      }
    }
    // if also not found in the extended table, then log a note
    if (found==0)
    {
      writelogfile(LOG_INFO,"smsd","Cannot convert ISO character %c 0x%2X to GSM, you might need to update the translation tables.",source[source_count], source[source_count]);
    }
    source_count++;
  }
  // Terminate destination string with 0, however 0x00 are also allowed within the string.
  destination[dest_count]=0;
  return dest_count;
}

int gsm2iso(char* source, int size, char* destination, int max)
{
  int table_row;
  int source_count=0;
  int dest_count=0;
  int found=0;
  if (source==0 || size==0)
  {
    destination[0]=0;
    return 0;
  }
  // Convert each character untl end of string
  while (source_count<size && dest_count<max)
  {
    if (source[source_count]!=0x1B)
    {  
      // search in normal translation table
      found=0;
      table_row=0;
      while (charset[table_row*2])
      {
        if (charset[table_row*2+1]==source[source_count])
        {
	  destination[dest_count++]=charset[table_row*2];
  	  found=1;
	  break;
	}
	table_row++;
      }
      // if not found in the normal table, then log a note
      if (found==0)
      {
        writelogfile(LOG_INFO,"smsd","Cannot convert GSM character 0x%2X to ISO, you might need to update the 1st translation table.",source[source_count]);
      }
    }
    else if (++source_count<size)
    {
      // search in extended translation table
      found=0;
      table_row=0;
      while (ext_charset[table_row*2])
      {
        if (ext_charset[table_row*2+1]==source[source_count])
        {
	  destination[dest_count++]=ext_charset[table_row*2];
  	  found=1;
	  break;
	}
	table_row++;
      }
      // if not found in the normal table, then log a note
      if (found==0)
      {
        writelogfile(LOG_INFO,"smsd","Cannot convert extended GSM character 0x1B 0x%2X, you might need to update the 2nd translation table.",source[source_count]);
      }          
    }
    source_count++;
  }
  // Terminate destination string with 0, however 0x00 are also allowed within the string.
  destination[dest_count]=0;
  return dest_count;
}
