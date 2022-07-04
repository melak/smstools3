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
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <syslog.h>
#include <time.h>
#include "alarm.h"
#include "extras.h"

char* _alarmhandler={0};
int _alarmlevel=LOG_WARNING;

void set_alarmhandler(char* handler,int level)
{
  _alarmhandler=handler;
  _alarmlevel=level;
}

void alarm_handler(int severity,char* devicename,char* format, ...)
{
  va_list argp;
  char text[1024];
  char cmdline[PATH_MAX+1024];
  char timestamp[40];
  time_t now;
  if (_alarmhandler[0])
  {
    va_start(argp,format);
    vsnprintf(text,sizeof(text),format,argp);
    va_end(argp);
    if (severity<=_alarmlevel)
    {
      time(&now);
      strftime(timestamp,sizeof(timestamp),"%Y-%m-%d %H:%M:%S",localtime(&now));
      snprintf(cmdline,sizeof(cmdline),"%s ALARM %s %i %s \"%s\"",_alarmhandler,timestamp,severity,devicename,text);      
      my_system(cmdline);
    }  
  }
}

