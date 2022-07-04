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

#include "locking.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>

int lockfile( char*  filename)
{
  char lockfilename[PATH_MAX];
  int lockfile;
  struct stat statbuf;
  char pid[20];
  strcpy(lockfilename,filename);
  strcat(lockfilename,".LOCK");
  if (stat(lockfilename,&statbuf))
  {
    lockfile=open(lockfilename,O_CREAT|O_EXCL|O_WRONLY,0644);
    if (lockfile>=0)
    {
      sprintf(pid,"%i\n",getpid());
      write(lockfile,&pid,strlen(pid));
      close(lockfile);
      return 1;
    }
    else
      return 0;
  }
  else
    return 0;
}

int islocked( char*  filename)
{
  char lockfilename[PATH_MAX];
  struct stat statbuf;
  strcpy(lockfilename,filename);
  strcat(lockfilename,".LOCK");
  if (stat(lockfilename,&statbuf))
    return 0;
  else
    return 1;
}

int un_lock( char*  filename)
{
  char lockfilename[PATH_MAX];
  strcpy(lockfilename,filename);
  strcat(lockfilename,".LOCK");
  if (unlink(lockfilename))
    return 0;
  else
    return 1;
}
