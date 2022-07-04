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

#include <syslog.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include "whitelist.h"
#include "extras.h"
#include "logging.h"
#include "alarm.h"

int inwhitelist(char* msisdn)
{
  FILE* file;
  char line[256];
  char* posi;
  if (whitelist[0]) // is a whitelist file specified?
  {
    file=fopen(whitelist,"r");
    if (file)
    {
      while (fgets(line,sizeof(line),file))
      {
        posi=strchr(line,'#');     // remove comment
        if (posi)
          *posi=0;
        cutspaces(line);
	if (strlen(line)>0)
	{
          if (strncmp(msisdn,line,strlen(line))==0)
	  {
	    fclose(file);
            return 1;
	  }  
	  else if (msisdn[0]=='s' && strncmp(msisdn+1,line,strlen(line))==0)
	  {
	    fclose(file);
	    return 1;
	  }
	}  
      }  
      fclose(file);
      return 0;
    }  
    else
    {
      writelogfile(LOG_CRIT,"Cannot read whitelist file %s.",whitelist);
      alarm_handler(LOG_CRIT,"SMSD","Cannot read whitelist file %s.",whitelist);
      kill(0,SIGTERM);
    }
  }      
  return 1;
}
