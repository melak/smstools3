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
#include <limits.h>
#include "alarm.h"
#include "smsd_cfg.h"
#include "stats.h"
#include <syslog.h>
#include "logging.h"
#include <errno.h>
#ifndef NOSTATS
#include <mm.h>
#endif

char newstatus[DEVICES+1]={0};
char oldstatus[DEVICES+1]={0};

void initstats()
{
  int i;
// if the mm Library is not available the statistic funktion does not work.
// Use unshared memory instead disabling all statistc related functions.
// This is much easier to program.
#ifndef NOSTATS
  MM_create(DEVICES*sizeof(_stats),tempnam(0,0));
#endif
  for (i=0; i<DEVICES; i++)
  {
#ifndef NOSTATS
    if ((statistics[i]=(_stats*)MM_malloc(sizeof(_stats))))
#else
    if ((statistics[i]=(_stats*)malloc(sizeof(_stats))))
#endif
    {
      statistics[i]->succeeded_counter=0;
      statistics[i]->failed_counter=0;
      statistics[i]->received_counter=0;
      statistics[i]->multiple_failed_counter=0;
      statistics[i]->status='-';
      statistics[i]->usage_s=0;
      statistics[i]->usage_r=0;
    }
    else
    {
//      syslog(LOG_ERR,"Cannot reserve memory for statistics.");
      writelogfile(LOG_ERR,"smsd","Cannot reserve memory for statistics.");
      alarm_handler(LOG_ERR,"smsd","Cannot reserve memory for statistics.");
      exit(2);
    }
  }
  rejected_counter=0;
  start_time=time(0);
  last_stats=time(0);
}

void resetstats()
{
  int i;
  for (i=0; i<DEVICES; i++)
  {
    statistics[i]->succeeded_counter=0;
    statistics[i]->failed_counter=0;
    statistics[i]->received_counter=0;
    statistics[i]->multiple_failed_counter=0;
    statistics[i]->status='-';
    statistics[i]->usage_s=0;
    statistics[i]->usage_r=0;
  }
  rejected_counter=0;
  start_time=time(0);
  last_stats=time(0);
}

void savestats()
{
  char filename[PATH_MAX];
  FILE* datei;
  int i;
  time_t now=time(0);
  if (d_stats[0] && stats_interval)
  {
    sprintf(filename,"%s/stats.tmp",d_stats);
    datei=fopen(filename,"w");
    if (datei)
    {
      fwrite(&now,sizeof(now),1,datei);
      fwrite(&start_time,sizeof(start_time),1,datei);
      for (i=0; i<DEVICES; i++)
        fwrite(statistics[i],sizeof(_stats),1,datei);
      fclose(datei);
    }
    else
    {
//      syslog(LOG_ERR,"Cannot write tmp file for statistics.");
	writelogfile(LOG_ERR,"smsd","Cannot write tmp file for statistics. %s %s",filename,strerror(errno));
        alarm_handler(LOG_ERR,"smsd","Cannot write tmp file for statistics. %s %s",filename,strerror(errno));
    }
  }
}

void loadstats()
{
  char filename[PATH_MAX];
  FILE* datei;
  int i;
  time_t saved_time;
  if (d_stats[0] && stats_interval)
  {
    sprintf(filename,"%s/stats.tmp",d_stats);
    datei=fopen(filename,"r");
    if (datei)
    {
      fread(&saved_time,sizeof(time_t),1,datei);
      fread(&start_time,sizeof(time_t),1,datei);
      start_time=time(0)-(saved_time-start_time);
      for (i=0; i<DEVICES; i++)
      {
        fread(statistics[i],sizeof(_stats),1,datei);
        statistics[i]->status='-';
      }
      fclose(datei);
    }
  }    
}

void print_status(char* buffer)
{
  int j;
  if (printstatus)
  {
    strcpy(oldstatus,newstatus);
    for (j=0; j<DEVICES; j++)
      newstatus[j]=statistics[j]->status;
    newstatus[DEVICES]=0;
    if (strcmp(oldstatus,newstatus))
    {
      printf("%s\n",newstatus);
    }
  }
}  
      
      
void checkwritestats()
{
  time_t now,next_time;
  char filename[PATH_MAX];
  char s[20];
  FILE* datei;
  int i;
  int sum_counter;
  if (d_stats[0] && stats_interval)
  {
    next_time=last_stats+stats_interval;
    next_time=stats_interval*(next_time/stats_interval);  // round value
    now=time(0);
    if (now>=next_time) // reached timepoint of next stats file?
    {


      // Check if there was activity if user does not want zero-files 
      if (stats_no_zeroes)
      {
        sum_counter=rejected_counter;
	for (i=0; i<DEVICES; i++)
	{
          if (devices[i].name[0])
          {
            sum_counter+=statistics[i]->succeeded_counter;
	    sum_counter+=statistics[i]->failed_counter;
	    sum_counter+=statistics[i]->received_counter;
	    sum_counter+=statistics[i]->multiple_failed_counter;
          }
        }
        if (sum_counter==0)
        {
          resetstats();
          last_stats=now;
          return;
        }
      }


      last_stats=time(0);
      strftime(s,sizeof(s),"%y%m%d.%H%M%S",localtime(&next_time));
      syslog(LOG_INFO,"Writing stats file %s",s);
      strcpy(filename,d_stats);
      strcat(filename,"/");
      strcat(filename,s);
      datei=fopen(filename,"w");
      if (datei)
      {
        fprintf(datei,"runtime,rejected\n");
	fprintf(datei,"%li,%i\n\n",now-start_time,rejected_counter);
	fprintf(datei,"name,succeeded,failed,received,multiple_failed,usage_s,usage_r\n");
	for (i=0; i<DEVICES; i++)
	{
	  if (devices[i].name[0])
  	    fprintf(datei,"%s,%i,%i,%i,%i,%i,%i\n",
	      devices[i].name,
	      statistics[i]->succeeded_counter,
	      statistics[i]->failed_counter,
	      statistics[i]->received_counter,
	      statistics[i]->multiple_failed_counter,
	      statistics[i]->usage_s,
	      statistics[i]->usage_r);
	}
        fclose(datei);
	resetstats();
	last_stats=now;
      }
      else
      {
//         syslog(LOG_ERR,"Cannot write statistic file.");
	 writelogfile(LOG_ERR,"smsd","Cannot write statistic file. %s %s",filename,strerror(errno));
	 alarm_handler(LOG_ERR,"smsd","Cannot write statistic file. %s %s",filename,strerror(errno));
      }
    }
  }
}

