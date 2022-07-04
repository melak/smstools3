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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include "extras.h"
#include "cfgfile.h"
#include "smsd_cfg.h"
#include "stats.h"
#include "version.h"
#include "blacklist.h"
#include "whitelist.h"
#include "alarm.h"


void initcfg()
{
  int i;
  int j;
  autosplit=3;
  receive_before_send=0;
  delaytime=10;
  blocktime=60*60;
  errorsleeptime=10;
  blacklist[0]=0;
  whitelist[0]=0;
  eventhandler[0]=0;
  checkhandler[0]=0;
  alarmhandler[0]=0;
  logfile[0]=0;
  loglevel=-1;  // Will be changed after reading the cfg file if stil -1
  alarmlevel=LOG_WARNING;
  
  strcpy(d_spool,"/var/spool/sms/outgoing");
  strcpy(d_incoming,"/var/spool/sms/incoming");
  strcpy(d_checked,"/var/spool/sms/checked");
  strcpy(mypath,"/usr/local/bin");
  d_failed[0]=0;
  logfile[0]=0;
  d_sent[0]=0;
  d_stats[0]=0;
  stats_interval=0;
  stats_no_zeroes=0;
  for (i=0; i<PROVIDER; i++)
  {
    queues[i].name[0]=0;
    queues[i].directory[0]=0;
    for (j=0; j<NUMS; j++)
      queues[i].numbers[j][0]=0;
  }
  for (i=0; i<DEVICES; i++)
  {
    devices[i].name[0]=0;
    devices[i].device[0]=0;
    devices[i].incoming=0;
    devices[i].pin[0]=0;
    devices[i].smsc[0]=0;
    devices[i].baudrate=19200;
    devices[i].send_delay=1;
    devices[i].cs_convert=1;
    devices[i].initstring[0]=0;
    devices[i].initstring2[0]=0;
    devices[i].eventhandler[0]=0;
    devices[i].report=0;
    devices[i].rtscts=1;
    devices[i].read_memory_start=1;
    strcpy(devices[i].mode,"new");
    strcpy(devices[i].queues[0],d_checked);
    for (j=1; j<PROVIDER; j++)
      devices[i].queues[j][0]=0;
  }
}

void readcfg()
{
  FILE* File;
  char devices_list[256];
  char name[32];
  char value[PATH_MAX];
  char tmp[PATH_MAX];
  char device_name[32];
  int result;
  int i,j;
  File=fopen(configfile,"r");
  if (File)
  {
    /* read global parameter */
    result=my_getline(File,name,sizeof(name),value,sizeof(value));
    while (result==1)
    {
      if (strcasecmp(name,"devices")==0)
        strcpy(devices_list,value);
      else if (strcasecmp(name,"spool")==0)
        strcpy(d_spool,value);
      else if (strcasecmp(name,"outgoing")==0)
        strcpy(d_spool,value);
      else if (strcasecmp(name,"stats")==0)
        strcpy(d_stats,value);
      else if (strcasecmp(name,"failed")==0)
        strcpy(d_failed,value);
      else if (strcasecmp(name,"incoming")==0)
        strcpy(d_incoming,value);
      else if (strcasecmp(name,"checked")==0)
        strcpy(d_checked,value);
      else if (strcasecmp(name,"sent")==0)
        strcpy(d_sent,value);
      else if (strcasecmp(name,"mypath")==0)
        strcpy(mypath,value);
      else if (strcasecmp(name,"delaytime")==0)
        delaytime=atoi(value);
      else if (strcasecmp(name,"blocktime")==0)
        blocktime=atoi(value);
      else if (strcasecmp(name,"stats_interval")==0)
        stats_interval=atoi(value);
      else if (strcasecmp(name,"stats_no_zeroes")==0)
        stats_no_zeroes=yesno(value);
      else if (strcasecmp(name,"errorsleeptime")==0)
        errorsleeptime=atoi(value);
      else if (strcasecmp(name,"eventhandler")==0)
        strcpy(eventhandler,value);
      else if (strcasecmp(name,"checkhandler")==0)
        strcpy(checkhandler,value);	
      else if (strcasecmp(name,"alarmhandler")==0)
        strcpy(alarmhandler,value);
      else if (strcasecmp(name,"blacklist")==0)
        strcpy(blacklist,value);
      else if (strcasecmp(name,"whitelist")==0)
        strcpy(whitelist,value);
      else if (strcasecmp(name,"logfile")==0)
        strcpy(logfile,value);
      else if (strcasecmp(name,"loglevel")==0)
        loglevel=atoi(value);
      else if (strcasecmp(name,"alarmlevel")==0)
        alarmlevel=atoi(value);
      else if (strcasecmp(name,"autosplit")==0)
	autosplit=atoi(value);
      else if (strcasecmp(name,"receive_before_send")==0)
        receive_before_send=yesno(value);
      else
        fprintf(stderr,"Unknown variable in config file: %s\n",name);
      result=my_getline(File,name,sizeof(name),value,sizeof(value));
    }
    if (result==-1)
      fprintf(stderr,"Syntax Error in config file: %s\n",value);

    /* read queue-settings */
    if ((gotosection(File,"queue")) || (gotosection(File,"queues")))
    {
      i=0;
      result=my_getline(File,name,sizeof(name),value,sizeof(value));
      while ((result==1) && (i<PROVIDER))
      {
        strcpy(queues[i].name,name);
        strcpy(queues[i].directory,value);
        i++;
        result=my_getline(File,name,sizeof(name),value,sizeof(value));
      }
      if (result==-1)
        fprintf(stderr,"Syntax Error in config file: %s\n",value);
    }

    /* read provider-settings */
    if ((gotosection(File,"provider")) || (gotosection(File,"providers")))
    {
      result=my_getline(File,name,sizeof(name),value,sizeof(value));
      while (result==1)
      {
        i=getqueue(name,tmp);
        if (i>=0)
        {
          for (j=1; j<=NUMS; j++)
	  {
	    if (getsubparam(value,j,tmp,sizeof(tmp)))
	    {
#ifdef DEBUGMSG
  printf("!! queues[%i].numbers[%i]=%s\n",i,j-1,tmp);
#endif
	      strcpy(queues[i].numbers[j-1],tmp);
	    }
	    else
	      break;
	  }
        }
        else
          fprintf(stderr,"Error in config file: missing queue for %s.\n",name);
        result=my_getline(File,name,sizeof(name),value,sizeof(value));
      }
      if (result==-1)
        fprintf(stderr,"Syntax Error in config file: %s\n",value);
    }

    /* read device-settings */
    for (i=0; i<DEVICES; i++)
    {
      if (getsubparam(devices_list,i+1,device_name,sizeof(device_name)))
      {
        if (!gotosection(File,device_name))
	  fprintf(stderr,"Could not find device [%s] in config file.\n",device_name);
        else
        {
	  strcpy(devices[i].name,device_name);
          result=my_getline(File,name,sizeof(name),value,sizeof(value));
	  while (result==1)
	  {
	    if (strcasecmp(name,"device")==0)
	      strcpy(devices[i].device,value);
	    else if (strcasecmp(name,"queues")==0)
	    {
	      for (j=1; j<=NUMS; j++)
	        if (getsubparam(value,j,tmp,sizeof(tmp)))
	          strcpy(devices[i].queues[j-1],tmp);
  	        else
	          break;
	    }
	    else if (strcasecmp(name,"incoming")==0)
	    {
	      if (strcasecmp(value,"high") ==0)
                 devices[i].incoming=2;
	      else
	      {
	        devices[i].incoming=atoi(value);
	        if (devices[i].incoming==0)  // For backward compatibility to older version with boolean value
	          devices[i].incoming=yesno(value);
	      }
            }
	    else if (strcasecmp(name,"cs_convert")==0)
	      devices[i].cs_convert=yesno(value);
	    else if (strcasecmp(name,"pin")==0)
	      strcpy(devices[i].pin,value);
	    else if (strcasecmp(name,"mode")==0)
            {
              if (strcasecmp(value,"ascii")==0)
                fprintf(stderr,"Ascii mode is not supported anymore.\n",value);
              if ((strcasecmp(value,"old")!=0) && (strcasecmp(value,"new")!=0))
                fprintf(stderr,"Invalid mode=%s in config file.\n",value);
              else
                strcpy(devices[i].mode,value);
            }
	    else if (strcasecmp(name,"smsc")==0)
	      strcpy(devices[i].smsc,value);
	    else if (strcasecmp(name,"baudrate")==0)
	      devices[i].baudrate=atoi(value);
            else if (strcasecmp(name,"send_delay")==0)
              devices[i].send_delay=atoi(value);
            else if (strcasecmp(name,"memory_start")==0)
              devices[i].read_memory_start=atoi(value);
	    else if (strcasecmp(name,"init")==0)
            {
  	      strcpy(devices[i].initstring,value);
              strcat(devices[i].initstring,"\r");
            }
            else if (strcasecmp(name,"init1")==0)
            {
              strcpy(devices[i].initstring,value);
              strcat(devices[i].initstring,"\r");
            }
            else if (strcasecmp(name,"init2")==0)
            {
              strcpy(devices[i].initstring2,value);
              strcat(devices[i].initstring2,"\r");
            }
	    else if (strcasecmp(name,"eventhandler")==0)
    	      strcpy(devices[i].eventhandler,value);
	    else if (strcasecmp(name,"report")==0)
    	      devices[i].report=yesno(value);
	    else if (strcasecmp(name,"rtscts")==0)
    	      devices[i].rtscts=yesno(value);
	    else
	      fprintf(stderr,"Unknown variable in config file for modem %s: %s\n",device_name,name);
	    result=my_getline(File,name,sizeof(name),value,sizeof(value));
	  }
          if (result==-1)
            fprintf(stderr,"Syntax Error in config file: %s\n",value);
          if ((devices[i].report==1) && (devices[i].incoming==0))
            fprintf(stderr,"Warning: Cannot receive status reports because receiving is disabled on modem %s\n",device_name);
        }
      }
      else
        break;
    }
    fclose(File);
    set_alarmhandler(alarmhandler,alarmlevel);
    // if loglevel is unset, then set it depending on if we use syslog or a logfile
    if (loglevel==-1)
    {
      if (logfile[0]==0)
        loglevel=LOG_DEBUG;
      else
        loglevel=LOG_WARNING;
    }
  }
  else
    fprintf(stderr,"Cannot open config file for read.\n");
}


int getqueue(char* name, char* directory) // Name can also be a phone number
{
  int i;
  int j;
#ifdef DEBUGMSG
  printf("!! getqueue(name=%s,... )\n",name);
#endif

  // If no queues are defined, then directory is always d_checked
  if (queues[0].name[0]==0)
  {
    strcpy(directory,d_checked);
#ifdef DEBUGMSG
  printf("!! Returns -2, no queues, directory=%s\n",directory);
#endif
    return -2;
  } 
  if (is_number(name))
  {
#ifdef DEBUGMSG
  printf("!! Searching by number\n");
#endif
    i=0;
    while (queues[i].name[0] && (i<PROVIDER))
    {
      j=0;
      while (queues[i].numbers[j][0] && (j<NUMS))
      {
        if (!strncmp(queues[i].numbers[j],name,strlen(queues[i].numbers[j])))
	{
  	  strcpy(directory,queues[i].directory);
#ifdef DEBUGMSG
  printf("!! Returns %i, directory=%s\n",i,directory);
#endif
	  return i;
	}
	j++;
      }
      i++;
    }
  }
  else
  {
#ifdef DEBUGMSG
  printf("!! Searching by name\n");
#endif
    i=0;
    while (queues[i].name[0] && (i<PROVIDER))
    {
      if (!strcmp(name,queues[i].name))
      {
        strcpy(directory,queues[i].directory);
#ifdef DEBUGMSG
  printf("!! Returns %i, directory=%s\n",i,directory);
#endif
        return i;
      }
      i++;
    }
  }
  /* not found */
  directory[0]=0;
#ifdef DEBUGMSG
  printf("!! Returns -1, not found, directory=%s\n",directory);
#endif
  return -1;
}

int getdevice(char* name)
{
  int i=0;
  while (devices[i].name[0] && (i<DEVICES))
  {
    if (!strcmp(name,devices[i].name))
      return i;
    i++;
  }
  return -1;
}

void help()
{
  printf("smsd spools incoming and outgoing sms.\n\n");
  printf("Usage:\n");
  printf("         smsd [options]\n\n");
  printf("Options:\n");
  printf("         -cx set config file to x\n");
  printf("         -h  this help\n");
  printf("         -s  display status monitor\n");
  printf("         -V  print copyright and version\n\n");
  printf("All other options are set by the file /etc/smsd.conf.\n\n");
  printf("Output is written to stdout, errors are written to stderr.\n\n");
  exit(0);
}

void parsearguments(int argc,char** argv)
{
  int result;
  strcpy(configfile,"/etc/smsd.conf");
  printstatus=0;

  do
  {
    result=getopt(argc,argv,"sdhc:V");
    switch (result)
    {
      case 'h': help(); 
                break;
      case 'c': strcpy(configfile,optarg);
                break;
      case 's': printstatus=1;
                break;
      case 'V': printf("Version %s, Copyright (c) Stefan Frings, stefan@stefanfrings.de\n",smsd_version);
                exit(0);
    }
  }
  while (result>0);
}

