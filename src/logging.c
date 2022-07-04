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

#include "logging.h"
#include "extras.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>

int  Filehandle;
int  Level;

int openlogfile(char* filename,char* progname,int facility,int level)
{
  Level=level;
  if (filename==0 || filename[0]==0 || strcmp(filename,"syslog")==0 || strcmp(filename,"0")==0)
  {
    openlog(progname,LOG_CONS,facility);
    Filehandle=-1;
    return 0;
  }
  else if (is_number(filename))
  {
    int oldfilehandle;
    oldfilehandle=atoi(filename);
    Filehandle=dup(oldfilehandle);
    if (Filehandle<0)
    {
      fprintf(stderr,"cannot duplicate logfile handle\n");
      exit(1);
    }
    else
      return Filehandle;
  }
  else
  {
    Filehandle=open(filename,O_APPEND|O_WRONLY|O_CREAT,0666);
    if (Filehandle<0)
    {
      fprintf(stderr,"cannot open logfile\n");
      exit(1);
    }
    else
      return Filehandle;
  }
}

void closelogfile()
{
  if (Filehandle>=0)
    close(Filehandle);
}

void writelogfile(int severity,char* progname,char* format, ...)
{
  va_list argp;
  char text[2048];
  char text2[1024];
  char timestamp[40];
  time_t now;
  // make a string of the arguments
  va_start(argp,format);
  vsnprintf(text,sizeof(text),format,argp);
  va_end(argp);
  if (severity<=Level)
  {
    if (Filehandle<0)
      syslog(severity,"%s",text);
    else
    {
      time(&now);
      strftime(timestamp,sizeof(timestamp),"%Y-%m-%d %H:%M:%S",localtime(&now));
      snprintf(text2,sizeof(text2),"%s,%i, %s: %s\n",timestamp,severity,progname,text);
      write(Filehandle,text2,strlen(text2));
    }
  }
}

