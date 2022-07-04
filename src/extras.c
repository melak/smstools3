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

#include "extras.h"
#include "locking.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/wait.h>

void cut_ctrl(char* message) /* removes all ctrl chars */
{
  char tmp[500];
  int posdest=0;
  int possource;
  int count;
  count=strlen(message);
  for (possource=0; possource<=count; possource++)
  {
    if ((message[possource]>=' ') || (message[possource]==0))
      tmp[posdest++]=message[possource];
  }
  strcpy(message,tmp);
}


int is_blank(char c)
{
  return (c==9) || (c==32);
}


int movefile( char*  filename,  char*  directory)
{
  char newname[PATH_MAX];
  char storage[1024];
  int source,dest;
  int readcount;
  char* cp;
  struct stat statbuf;
  if (stat(filename,&statbuf)<0)
    statbuf.st_mode=0640;
  statbuf.st_mode&=07777;
  cp=strrchr(filename,'/');
  if (cp)
    sprintf(newname,"%s%s",directory,cp);
  else
    sprintf(newname,"%s/%s",directory,filename);
  source=open(filename,O_RDONLY);
  if (source>=0)
  {
    dest=open(newname,O_WRONLY|O_CREAT|O_TRUNC,statbuf.st_mode);
    if (dest>=0)
    {
      while ((readcount=read(source,&storage,sizeof(storage)))>0)
        if (write(dest,&storage,readcount)<readcount)
	{
	  close(dest);
	  close(source);
	  return 0;
	}
      close(dest);
      close(source);
      unlink(filename);
      return 1;
    }
    else
    {
      close(source);
      return 0;
    }
  }
  else
    return 0;
}

int movefilewithdestlock( char*  filename,  char*  directory)
{
  char lockfilename[PATH_MAX];
  char* cp;
  //create lockfilename in destination
  cp=strrchr(filename,'/');
  if (cp)
    sprintf(lockfilename,"%s%s",directory,cp);
  else
    sprintf(lockfilename,"%s/%s",directory,filename);
  //create lock and move file
  if (!lockfile(lockfilename))
    return 0;
  if (!movefile(filename,directory))
    return 0;
  if (!un_lock(lockfilename))
    return 0;
  return 1;
}

void cutspaces(char*  text)
{
  int count;
  int Length;
  int i;
  int omitted;
  /* count ctrl chars and spaces at the beginning */
  count=0;
  while ((text[count]!=0) && ((is_blank(text[count])) || (iscntrl(text[count]))) )
    count++;
  /* remove ctrl chars at the beginning and \r within the text */
  omitted=0;
  Length=strlen(text);
  for (i=0; i<=(Length-count); i++)
    if (text[i+count]=='\r')
      omitted++;
    else
      text[i-omitted]=text[i+count];
  Length=strlen(text);
  while ((Length>0) && ((is_blank(text[Length-1])) || (iscntrl(text[Length-1]))))
  {
    text[Length-1]=0;
    Length--;
  }
}

void cut_emptylines(char*  text)
{
  char* posi;
  char* found;
  posi=text;
  while (posi[0] && (found=strchr(posi,'\n')))
  {
    if ((found[1]=='\n') || (found==text))
      memmove(found,found+1,strlen(found));
    else
      posi++;
  }
}

int is_number( char*  text)
{
  int i;
  int Length;
  Length=strlen(text);
  for (i=0; i<Length; i++)
    if (((text[i]>'9') || (text[i]<'0')) && (text[i]!='-'))
      return 0;
  return 1;
}

int getfile( char* dir, 
            char*  filename)
{
  DIR* dirdata;
  struct dirent* ent;
  struct stat statbuf;
  int found=0;
  if ((dirdata=opendir(dir)))
  {
    while ((ent=readdir(dirdata)) && !found)
    {
      sprintf(filename,"%s/%s",dir,ent->d_name);
      stat(filename,&statbuf);
      if (S_ISDIR(statbuf.st_mode)==0) /* Is this a directory? */
      {
        /* File found, check for lock file  */
	if (strstr(filename,".LOCK")==0) /* Is the file a lock file itself? */
	  if (!islocked(filename)) /* no, is there a lock file for this file? */
	  {
	    /* check if the file grows at the moment (another program writes to it) */
	    int groesse1;
	    int groesse2;
	    groesse1=statbuf.st_size;
	    sleep(1);
	    stat(filename,&statbuf);
	    groesse2=statbuf.st_size;
	    if (groesse1==groesse2)
  	      found=1;
	  }
      }	
    }
    closedir(dirdata);
  }
  if (!found)
    filename[0]=0;
  return found;
}

int my_system( char*  command)
{
  int pid,status;
#ifdef DEBUGMSG
  printf("!! my_system(%s)\n",command);
#endif
  if (command==0)
    return 1;
  pid=fork();
  if (pid==-1)
    return -1;
  if (pid==0)  // only executed in the child
  {
#ifdef DEBUGMSG
    printf("!! pid=%i, child running external command\n",pid);
#endif
    char* argv[4];
    argv[0]="sh";
    argv[1]="-c";
    argv[2]=(char*) command;
    argv[3]=0;
    execv("/bin/sh",argv);  // replace child with the external command
#ifdef DEBUGMSG
    printf("!! pid=%i, execv() failed, child exits now\n",pid);
#endif
    exit(1);                // exit with error when the execv call failed
  }
  errno=0;
#ifdef DEBUGMSG
    printf("!! father waiting for child %i\n",pid);
#endif
  do
  {
    if (waitpid(pid,&status,0)==-1)
    {
      if (errno!=EINTR)
        return -1;
    }
    else
      return WEXITSTATUS(status);
  }
  while (1);    
}

void write_pid( char* filename)
{
  char list[1024];
  char pid[20];
  ssize_t bytes=0;
  char* posi1;
  int pidfile;
  list[0]=0;
  pidfile=open(filename,O_RDONLY);
  if (pidfile>=0)
  {
    bytes=read(pidfile,&list,sizeof(list)-1);
    if (bytes>=0)
      list[bytes]=0;
    close(pidfile);
  }
  sprintf(pid," %i",getpid());
  posi1=strstr(list,pid);
  if (! posi1)
  {
    pidfile=open(filename,O_CREAT|O_WRONLY|O_TRUNC,0644);
    if (pidfile>=0)
    {
      strcat(list,pid);
      write(pidfile,&list,strlen(list));
      close(pidfile);
    }
  }
}

void remove_pid( char* filename)
{
  char list[1024];
  char tmp[1024];
  char pid[20];
  ssize_t bytes=0;
  char* posi1;
  char* posi2;
  int pidfile;
  pidfile=open(filename,O_RDONLY);
  list[0]=0;
  if (pidfile>=0)
  {
    bytes=read(pidfile,&list,sizeof(list)-1);
    if (bytes>=0)
      list[bytes]=0;
    close(pidfile);
  }
  if (list[0])
  {
    sprintf(pid," %i",getpid());
    posi1=strstr(list,pid);
    if (posi1)
    {
      posi2=strstr(posi1+1," ");
      if (posi2)
      {
        strcpy(tmp,posi2);
        strcpy(posi1,tmp);
      }
      else
        *posi1=0;
      if (list[0])
      {
        pidfile=open(filename,O_CREAT|O_WRONLY|O_TRUNC,0644);
        if (pidfile>=0)
        {
          write(pidfile,&list,strlen(list));
          close(pidfile);
        }
      }
      else
        unlink(filename);
    }
  }
  else
    unlink(filename);
}


