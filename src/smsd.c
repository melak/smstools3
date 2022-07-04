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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <signal.h>
#include <time.h>
#ifndef NOSTATS
#include <mm.h>
#endif
#include <fcntl.h>
#include <unistd.h>
#include "extras.h"
#include "locking.h"
#include "smsd_cfg.h"
#include "stats.h"
#include "version.h"
#include "blacklist.h"
#include "whitelist.h"
#include "logging.h"
#include "alarm.h"
#include "charset.h"
#include "cfgfile.h"
#include "pdu.h"
#include "modeminit.h"

int logfilehandle;  // handle of log file.
int thread_id; // -1 for main task, all other have numbers starting with 0.
int concatenated_id=0; // id number for concatenated messages.
int terminate=0;  // The current process terminates if this is 1

/* =======================================================================
   Runs checkhandler and returns return code
   ======================================================================= */
   
int run_checkhandler(char* filename)
{
  char cmdline[PATH_MAX+PATH_MAX+32];
  if (checkhandler[0])
  {
    sprintf(cmdline,"%s %s",checkhandler,filename);
    return my_system(cmdline);
  }
  else
  {
    return 0;
  }
}

/* =======================================================================
   Stops the program if the given file exists
   ======================================================================= */

/* filename1 is checked. The others arguments are used to compose an error message. */

void stop_if_file_exists(char* infotext1, char* filename1, char* infotext2, char* filename2)
{
  int datei;
  datei=open(filename1,O_RDONLY);
  if (datei>=0)
  {
    close(datei);
    writelogfile(LOG_CRIT,"smsd","Fatal error: %s %s %s %s. %s. Check file and dir permissions.",infotext1,filename1,infotext2,filename2,strerror(errno));
    alarm_handler(LOG_CRIT,"smsd","Fatal error: %s %s %s %s. %s. Check file and dir permissions.",infotext1,filename1,infotext2,filename2,strerror(errno));
    remove_pid("/var/run/smsd.pid");
    signal(SIGTERM,SIG_IGN);     
    kill(0,SIGTERM);
    exit(127);
  }
}

/* =======================================================================
   Get a field from a modem answer, remove quotes
   ======================================================================= */

void getfield(char* line, int field, char* result)
{
  char* start;
  char* end;
  int i;
  int length;
#ifdef DEBUGMSG
  printf("!! getfield(line=%s, field=%i, ...)\n",line,field);
#endif
  *result=0;
  start=strstr(line,":");
  if (start==0)
    return;
  for (i=1; i<field; i++)
  {
    start=strchr(start+1,',');
    if (start==0)
      return;      
  }
  start++;
  while (start[0]=='\"' || start[0]==' ')
    start++;
  if (start[0]==0)
    return;
  end=strstr(start,",");
  if (end==0)
    end=start+strlen(start)-1;
  while ((end[0]=='\"' || end[0]=='\"' || end[0]==',') && (end>=start))
    end--;
  length=end-start+1;
  strncpy(result,start,length);
  result[length]=0;
#ifdef DEBUGMSG
  printf("!! result=%s\n",result);
#endif    
}

/* =======================================================================
   Read the header of an SMS file
   ======================================================================= */

void readSMSheader(char* filename, /* Filename */
// output variables are:
                   char* to, /* destination number */
	           char* from, /* sender name or number */
	           int*  alphabet, /* -1=GSM 0=ISO 1=binary 2=UCS2 3=unknown */
                   int* with_udh,  /* UDH flag */
                   char* udh_data,  /* UDH data in hex dump format, e.g. "05 00 03 b2 02 01". Only used in alphabet<=0 */
	           char* queue, /* Name of Queue */
	           int*  flash, /* 1 if send as Flash SMS */
	           char* smsc, /* SMSC Number */
                   int*  report,  /* 1 if request status report */
		   int*  split)  /* 1 if request splitting */
{
  FILE* File;
  char line[256];
  char *ptr;
  to[0]=0;
  from[0]=0;
  *alphabet=0; 
  *with_udh=-1;
  udh_data[0]=0;
  queue[0]=0;
  *flash=0;
  smsc[0]=0;
  *report=-1; 
  *split=-1;
  
#ifdef DEBUGMSG
  printf("!! readSMSheader(filename=%s, ...)\n",filename);
#endif 
 
  File=fopen(filename,"r");  
  // read the header line by line 
  if (File)
  {
    // read until end of file or until an empty line was found
    while (fgets(line,sizeof(line)-1,File))
    {
      if ((line[0]==0) || (line[0]=='\n') || (line[0]=='\r'))
        break;
      if (strstr(line,"To:")==line)
      {
        // remove the To: and spaces
        memmove(line,line+3,strlen(line)-2);
        cutspaces(line);
        // correct phone number if it has wrong syntax
        if (strstr(line,"00")==line)
          strcpy(to,line+2);
        else if ((ptr=strchr(line,'+')) != NULL)
          strcpy(to,ptr+1);
        else
          strcpy(to,line);
        // Truncate after last digit
        if (*to=='s')
          to[strspn(to+1,"1234567890")+1]=0;
        else
          to[strspn(to,"1234567890")]=0;
      }
      else if (strstr(line,"From:")==line)
      {
        strcpy(from,line+5);
        cutspaces(from);
      }
      else if (strstr(line,"SMSC:")==line)
      {
        // remove the SMSC: and spaces
        memmove(line,line+5,strlen(line)-4);
        cutspaces(line);
        // correct phone number if it has wrong syntax
        if (strstr(line,"00")==line)
          strcpy(smsc,line+2);
        else if (strchr(line,'+')==line)
          strcpy(smsc,line+1);
        else
          strcpy(smsc,line);
      }
      else if (strstr(line,"Flash:")==line)
      {
        memmove(line,line+6,strlen(line)-5);
        cutspaces(line);
        *flash=yesno(line);
      }  
      else if (strstr(line,"Provider:")==line)
      {
        strcpy(queue,line+9);
        cutspaces(queue);
      }
      else if (strstr(line,"Queue:")==line)
      {
        strcpy(queue,line+6);
        cutspaces(queue);
      }
      else if (strstr(line,"Binary:")==line)
      {
        memmove(line,line+7,strlen(line)-6);
        cutspaces(line);
        *alphabet=yesno(line);
      }
      else if (strstr(line,"Report:")==line)
      {
        memmove(line,line+7,strlen(line)-6);
        cutspaces(line);
        *report=yesno(line);
      }
      else if (strstr(line,"Autosplit:")==line)
      {
	memmove(line,line+10,strlen(line)-9);
        cutspaces(line);
	*split=atoi(line);
      }
      else if (strstr(line,"Alphabet:")==line)
      {
        memmove(line,line+9,strlen(line)-8);
        cutspaces(line);
        if (strcasecmp(line,"GSM")==0)
          *alphabet=-1;
        else if (strncasecmp(line,"iso",3)==0)
          *alphabet=0;
        else if (strncasecmp(line,"lat",3)==0)
          *alphabet=0;
        else if (strncasecmp(line,"ans",3)==0)
          *alphabet=0;
        else if (strncasecmp(line,"bin",3)==0)
          *alphabet=1;
        else if (strncasecmp(line,"chi",3)==0)
          *alphabet=2;
        else if (strncasecmp(line,"ucs",3)==0)
          *alphabet=2;
        else if (strncasecmp(line,"uni",3)==0)
          *alphabet=2;
        else
          *alphabet=3;
      } 
      else if (strstr(line,"UDH-DATA:")==line)
      {
        strcpy(udh_data,line+9);
        cutspaces(udh_data);
      }
      else if (strstr(line,"UDH-DUMP:")==line) // same as UDH-DATA for backward compatibility
      {
        strcpy(udh_data,line+9);
        cutspaces(udh_data);
      }      
      else if (strstr(line,"UDH:")==line) 
      {
        memmove(line,line+4,strlen(line)-3);
        cutspaces(line);
        *with_udh=yesno(line);
      }      
      else // if the header is unknown, then simply ignore
      {
        ;;
      }
    }
    // End of header reached
    fclose(File);
#ifdef DEBUGMSG
  printf("!! to=%s\n",to);
  printf("!! from=%s\n",from);
  printf("!! alphabet=%i\n",*alphabet);
  printf("!! with_udh=%i\n",*with_udh);
  printf("!! udh_data=%s\n",udh_data);
  printf("!! queue=%s\n",queue);
  printf("!! flash=%i\n",*flash);
  printf("!! smsc=%s\n",smsc);
  printf("!! report=%i\n",*report);
  printf("!! split=%i\n",*split);
#endif 
  }
  else
  {
     writelogfile(LOG_ERR,"smsd","Cannot read sms file %s.",filename);
     alarm_handler(LOG_ERR,"smsd","Cannot read sms file %s.",filename);          
  }
}


/* =======================================================================
   Read the message text or binary data of an SMS file
   ======================================================================= */

void readSMStext(char* filename, /* Filename */
                 int do_convert, /* shall I convert from ISO to GSM? Do not try to convert binary data. */
// output variables are:
                 char* text,     /* message text */
                 int* textlen)   /* text length */
{
  int File;
  int readcount;
  char* p;
  char* position;
  char tmp[maxtext];
  int part1_size,part2_size;
  // Initialize result with empty string
  text[0]=0;
  *textlen=0;

#ifdef DEBUGMSG
  printf("readSMStext(filename=%s, do_convert=%i, ...)\n",filename,do_convert);
#endif
  
  File=open(filename,O_RDONLY);  
  // read the header line by line 
  if (File>=0)
  {
    position=0;
    readcount=read(File,tmp,sizeof(tmp)-1);
    // Search empty line
    while (readcount>0 && position==0)
    {
      tmp[readcount]=0;
      // Search double line feed in the block
      if ((p=strstr(tmp,"\n\n")))
        position=p+2;
      else if ((p=strstr(tmp,"\r\n\r\n")))
        position=p+4;
      // Search single line feed at begin of this block    
      else if (strstr(tmp,"\n\r\n")==tmp)
        position=tmp+3;
      else if (strstr(tmp,"\r\n")==tmp)
        position=tmp+2;
      else if (strstr(tmp,"\n")==tmp)
        position=tmp+1;
      if (position==0)
        readcount=read(File,tmp,sizeof(tmp)-1); 
    }
    // If found, then move to the beginning of tmp and read the rest of file
    if (position)  
    {
      part1_size=readcount-(position-tmp);
      memmove(tmp,position,part1_size); 
      part2_size=read(File,tmp+part1_size,sizeof(tmp)-part1_size); 
      // Convert character set or simply copy 
      if (do_convert==1)
        *textlen=iso2gsm(tmp,part1_size+part2_size,text,maxtext);
      else
      {
        memmove(text,tmp,part1_size+part2_size);
        *textlen=part1_size+part2_size;
      }
    }
    close(File);
  }
  else
  {
    writelogfile(LOG_ERR,"smsd","Cannot read sms file %s.",filename);
    alarm_handler(LOG_ERR,"smsd","Cannot read sms file %s.",filename);          
  }
#ifdef DEBUGMSG
  printf("!! textlen=%i\n",*textlen);
#endif
}

/* =======================================================================
   Mainspooler (sorts SMS into queues)
   ======================================================================= */

void mainspooler()
{
  char filename[PATH_MAX];
  char to[100];
  char from[100];
  char smsc[100];
  char queuename[100];
  char directory[PATH_MAX];
  char cmdline[PATH_MAX+PATH_MAX+32];
  int with_udh;
  char udh_data[500];
  int queue;
  int alphabet;
  int i;
  int flash;
  int success;
  int report;
  int split;
  writelogfile(LOG_INFO,"smsd","outgoing file checker has started.");
  while (terminate==0)
  {
    success=0;
    if (getfile(d_spool,filename))
    {
      readSMSheader(filename,to,from,&alphabet,&with_udh,udh_data,queuename,&flash,smsc,&report,&split);
      // Is the destination set?
      if (to[0]==0)
      {
        writelogfile(LOG_NOTICE,"smsd","No destination in file %s",filename);
        alarm_handler(LOG_NOTICE,"smsd","No destination in file %s",filename);
      }
      // Does the checkhandler accept the message?
      else if (run_checkhandler(filename))
      {   
        writelogfile(LOG_NOTICE,"smsd","SMS file %s rejected by checkhandler",filename);
        alarm_handler(LOG_NOTICE,"smsd","SMS file %s rejected by checkhandler",filename);    
      }
      // is To: in the blacklist?
      else if (inblacklist(to))
      {
        writelogfile(LOG_NOTICE,"smsd","Destination %s in file %s is blacklisted",to,filename);
        alarm_handler(LOG_NOTICE,"smsd","Destination %s in file %s is blacklisted",to,filename);
      }
      // is To: in the whitelist?
      else if (! inwhitelist(to))
      {
        writelogfile(LOG_NOTICE,"smsd","Destination %s in file %s is not whitelisted",to,filename);
        alarm_handler(LOG_NOTICE,"smsd","Destination %s in file %s is not whitelisted",to,filename);
      }
      // Is the alphabet setting valid?
      else if (alphabet>2)
      {
        writelogfile(LOG_NOTICE,"smsd","Invalid alphabet in file %s",filename);
        alarm_handler(LOG_NOTICE,"smsd","Invalid alphabet in file %s",filename);
      }
      // is there is a queue name, then set the queue by this name
      else if ((queuename[0]) && ((queue=getqueue(queuename,directory))==-1))
      {
        writelogfile(LOG_NOTICE,"smsd","Wrong provider queue %s in file %s",queuename,filename);
        alarm_handler(LOG_NOTICE,"smsd","Wrong provider queue %s in file %s",queuename,filename);
      }
      // if there is no queue name, set it by the destination phone number
      else if ((queuename[0]==0) && ((queue=getqueue(to,directory))==-1))
      {
        writelogfile(LOG_NOTICE,"smsd","Destination number %s in file %s does not match any provider",to,filename);
        alarm_handler(LOG_NOTICE,"smsd","Destination number %s in file %s does not match any provider",to,filename);
      }
      // everything is ok, move the file into the queue 
      else 
      {
        movefilewithdestlock(filename,directory);
        stop_if_file_exists("Cannot move",filename,"to",directory);
        writelogfile(LOG_INFO,"smsd","Moved file %s to %s",filename,directory);
        success=1;
      }
      if (! success)
      {
        rejected_counter++;
        if (eventhandler[0])
        {
          sprintf(cmdline,"%s %s %s",eventhandler,"FAILED",filename);
          my_system(cmdline);
        }
        if (d_failed[0])
        {
          movefilewithdestlock(filename,d_failed);
          stop_if_file_exists("Cannot move",filename,"to",d_failed);
        }
        else
        {
          unlink(filename);
          stop_if_file_exists("Cannot delete",filename,"","");
          writelogfile(LOG_INFO,"smsd","Deleted file %s",filename);
        }
      }
    }
    else
    {
      // Sleep a while and output status monitor
      for (i=0; i<delaytime; i++)
      {
        print_status();
	checkwritestats();
        if (terminate==1)
          return;
        sleep(1);
      }
    }
  }
}

/* =======================================================================
   Delete message on the SIM card
   ======================================================================= */

void deletesms(int device, int modem, int sim) /* deletes the selected sms from the sim card */
{
  char command[100];
  char answer[500];
  writelogfile(LOG_INFO,devices[device].name,"Deleting message %i",sim);
  sprintf(command,"AT+CMGD=%i\r",sim);
  put_command(modem, devices[device].name,devices[device].send_delay, command, answer, sizeof(answer), 50, "(OK)|(ERROR)");
}

/* =======================================================================
   Check size of SIM card
   ======================================================================= */

void check_memory(int device, int modem, int *used_memory,int *max_memory) // checks the size of the SIM memory
{
  char answer[500];
  char* start;
  char* end;
  char tmp[100];
  // Set default values in case that the modem does not support the +CPMS command
  *used_memory=1;
  *max_memory=10;

  writelogfile(LOG_INFO,devices[device].name,"Checking memory size");
  put_command(modem, devices[device].name,devices[device].send_delay, "AT+CPMS?\r", answer, sizeof(answer), 50, "(\\+CPMS:.*OK)|(ERROR)");
  if ((start=strstr(answer,"+CPMS:")))
  {
    end=strchr(start,'\r');
    if (end)
    {
      *end=0;
      getfield(start,2,tmp);
      if (tmp[0])
        *used_memory=atoi(tmp);
      getfield(start,3,tmp);
      if (tmp[0])
        *max_memory=atoi(tmp);    
      writelogfile(LOG_INFO,devices[device].name,"Used memory is %i of %i",*used_memory,*max_memory);
      return;
    }
  }
  writelogfile(LOG_INFO,devices[device].name,"Command failed, using defaults.");
}


/* =======================================================================
   Read a memory space from SIM card
   ======================================================================= */

int readsim(int device, int modem, int sim, char* line1, char* line2)  
/* reads a SMS from the given SIM-memory */
/* returns number of SIM memory if successful, otherwise 0 */
/* line1 contains the first line of the modem answer */
/* line2 contains the pdu string */
{                                
  char command[500];
  char answer[1024];
  char* begin1;
  char* begin2;
  char* end1;
  char* end2;
  line2[0]=0;
  line1[0]=0;
#ifdef DEBUGMSG
  printf("!! readsim(device=%i, modem=%i, sim=%i, ...)\n",device,modem,sim);
#endif
  writelogfile(LOG_INFO,devices[device].name,"Trying to get stored message %i",sim);
  sprintf(command,"AT+CMGR=%i\r",sim);
  put_command(modem, devices[device].name,devices[device].send_delay, command,answer,sizeof(answer),50,"(\\+CMGR:.*OK)|(ERROR)");
  if (strstr(answer,",,0\r")) // No SMS,  because Modem answered with +CMGR: 0,,0 
    return -1;
  if (strstr(answer,"ERROR")) // No SMS,  because Modem answered with ERROR 
    return -1;  
  begin1=strstr(answer,"+CMGR:");
  if (begin1==0)
    return -1;
  end1=strstr(begin1,"\r");
  if (end1==0)
    return -1;
  begin2=end1+1;
  end2=strstr(begin2+1,"\r");
  if (end2==0)
    return -1;
  strncpy(line1,begin1,end1-begin1);
  line1[end1-begin1]=0;
  strncpy(line2,begin2,end2-begin2);
  line2[end2-begin2]=0;
  cutspaces(line1);
  cut_ctrl(line1);
  cutspaces(line2);
  cut_ctrl(line2); 
  if (strlen(line2)==0)
    return -1;
#ifdef DEBUGMSG
  printf("!! line1=%s, line2=%s\n",line1,line2);
#endif
  return sim;
}



/* =======================================================================
   Write a received message into a file 
   ======================================================================= */
   
int received2file(char* line1, char* line2, char* mode, char* modemname, char* filename, int cs_convert) // returns 1 if this was a status report
{
  int userdatalength;
  char ascii[300]= {};
  char sendr[100]= {};
  int with_udh=0;
  char udh_data[450]= {};
  char smsc[31]= {};
  char name[64]= {};
  char date[9]= {};
  char Time[9]= {};
  char status[40]={};
  int alphabet=0;
  int is_statusreport=0;
  FILE* fd;
  
#ifdef DEBUGMSG
  printf("!! received2file(line1=%s, line2=%s, mode=%s, modemname=%s, filename=%s, cs_convert=%i)\n", line1, line2, mode, modemname, filename, cs_convert);
#endif

    getfield(line1,1,status);
    getfield(line1,2,name);
    // Check if field 2 was a number instead of a name
    if (atoi(name)>0)
    {
      name[0]=0;//Delete the name because it is missing
    }    
    userdatalength=splitpdu(line2, mode, &alphabet, sendr, date, Time, ascii, smsc, &with_udh, udh_data, &is_statusreport);
    if (alphabet==-1 && cs_convert==1)
      userdatalength=gsm2iso(ascii,userdatalength,ascii,sizeof(ascii));
  
#ifdef DEBUGMSG
  printf("!! userdatalength=%i\n",userdatalength);
  printf("!! name=%s\n",name);  
  printf("!! sendr=%s\n",sendr);  
  printf("!! date=%s\n",date);
  printf("!! Time=%s\n",Time); 
  if ((alphabet==-1 && cs_convert==1)||(alphabet==0))
  printf("!! ascii=%s\n",ascii); 
  printf("!! smsc=%s\n",smsc); 
  printf("!! with_udh=%i\n",with_udh);
  printf("!! udh_data=%s\n",udh_data);   
  printf("!! is_statusreport=%i\n",is_statusreport);   
#endif
  writelogfile(LOG_NOTICE, "smsd","SMS received, From: %s",sendr);

  //Replace the temp file by a new file with same name. This resolves wrong file permissions.
  unlink(filename);
  fd = fopen(filename, "w");
  if (fd)
  { 
  
    fprintf(fd, "From: %s\n",sendr);
    if (name[0])
      fprintf(fd,"Name: %s\n",name);
    if (smsc[0])
      fprintf(fd,"From_SMSC: %s\n",smsc);
    if (date[0] && Time[0])
      fprintf(fd, "Sent: %s %s\n",date,Time);
    // Add local timestamp
    {
      char timestamp[40];
      time_t now;
      time(&now);
      strftime(timestamp,sizeof(timestamp),"%y-%m-%d %H:%M:%S",localtime(&now));
      fprintf(fd,"Received: %s\n",timestamp);
    }
    fprintf(fd, "Subject: %s\n",modemname);
    if (alphabet==-1)
    {
      if (cs_convert)
        fprintf(fd,"Alphabet: ISO\n");
      else
        fprintf(fd,"Alphabet: GSM\n");
    }
    else if (alphabet==0)
      fprintf(fd,"Alphabet: ISO\n");
    else if (alphabet==1)
      fprintf(fd,"Alphabet: binary\n");
    else if (alphabet==2)
      fprintf(fd,"Alphabet: UCS2\n");
    else if (alphabet==3)
      fprintf(fd,"Alphabet: reserved\n");
    if (udh_data[0])
    {
      fprintf(fd,"UDH-DATA: %s\n",udh_data);
    }
    if (with_udh)
      fprintf(fd,"UDH: true\n");
    else
      fprintf(fd,"UDH: false\n");
    fprintf(fd,"\n");
    fwrite(ascii,1,userdatalength,fd);
    fclose(fd);
    return is_statusreport;
  }
  else
  {
    writelogfile(LOG_ERR,"smsd","Cannot create file %s!", filename);
    alarm_handler(LOG_ERR,"smsd","Cannot create file %s!", filename);
    return 0;
  }
}

/* =======================================================================
   Receive one SMS
   ======================================================================= */

int receivesms(int device, int modem, int* quick, int only1st)  
// receive one SMS or as many as the modem holds in memory
// if quick=1 then no initstring
// if only1st=1 then checks only 1st memory space
// Returns 1 if successful
// Return 0 if no SM available
// Returns -1 on error
{
  int max_memory,used_memory;
  int start_time=time(0);  
  int found;
  int foundsomething=0;
  int statusreport;
  int sim;
  int first_sim;
  char line1[1024];
  char line2[1024];
  char filename[PATH_MAX];
  char cmdline[PATH_MAX+PATH_MAX+32];
#ifdef DEBUGMSG
  printf("receivesms(device=%i, modem=%i, quick=%i, only1st=%i)\n",device,modem,*quick,only1st);
#endif

  statistics[device]->status='r';
  writelogfile(LOG_INFO,devices[device].name,"Checking device for incoming SMS");

  if (*quick==0)
  {
    // Initialize modem
    if (initmodem(modem,  devices[device].name, devices[device].send_delay, errorsleeptime, devices[device].pin, devices[device].initstring, devices[device].initstring2, devices[device].mode, devices[device].smsc)>0)
    {
      statistics[device]->usage_r+=time(0)-start_time;
      return -1;
    }
  }
  
  // Check how many memory spaces we really can read
  check_memory(device,modem,&used_memory,&max_memory);
  found=0;
  if (used_memory>0)
  {
    for (sim=devices[device].read_memory_start; sim<=devices[device].read_memory_start+max_memory-1; sim++)
    {
      found=readsim(device,modem,sim,line1,line2);
      if (found>=0)
      {
        //Prepare a temp file for received message
        sprintf(filename,"%s/%s.XXXXXXXX",d_incoming,devices[device].name);
        int tempfile=mkstemp(filename);
        if (tempfile==-1)
        {
           writelogfile(LOG_ERR,"smsd","Cannot create file %s!", filename);
           alarm_handler(LOG_ERR,"smsd","Cannot create file %s!", filename);
           return 0;
        }
        else 
          close(tempfile);

        foundsomething=1;
        *quick=1;
        
        statusreport=received2file(line1,line2, devices[device].mode, devices[device].name, filename, devices[device].cs_convert);
        statistics[device]->received_counter++;
        if (eventhandler[0] || devices[device].eventhandler[0])
        {
          if (devices[device].eventhandler[0] && statusreport==1)
            sprintf(cmdline,"%s %s %s",devices[device].eventhandler,"REPORT",filename);
          else if (eventhandler[0] && statusreport==1)
            sprintf(cmdline,"%s %s %s",eventhandler,"REPORT",filename);
          else if (devices[device].eventhandler[0] && statusreport==0)
            sprintf(cmdline,"%s %s %s",devices[device].eventhandler,"RECEIVED",filename);
          else if (eventhandler[0] && statusreport==0)
            sprintf(cmdline,"%s %s %s",eventhandler,"RECEIVED",filename);
          my_system(cmdline);
        }
        deletesms(device,modem,found);
        used_memory--;
        if (used_memory<1) 
          break; // Stop reading memory if we got everything
      }
      if (only1st)
        break;
    }
  }  
  statistics[device]->usage_r+=time(0)-start_time;
  if (foundsomething)
  {
    return 1;
  }
  else
  {
    writelogfile(LOG_INFO,devices[device].name,"No SMS received");
    return 0;
  }
}

/* ==========================================================================================
   Send a part of a message, this is physically one SM with max. 160 characters or 14 bytes
   ========================================================================================== */

int send_part(int device, int modem, char* from, char* to, char* text, int textlen, int alphabet, int with_udh, char* udh_data, int quick, int flash, char* messageid, char* smsc, int report)
// alphabet can be -1=GSM 0=ISO 1=binary 2=UCS2
// with_udh can be 0=off or 1=on or -1=auto (auto = 1 for binary messages and text message with udh_data)
// udh_data is the User Data Header, only used when alphabet= -1 or 0. With alphabet=1 or 2, the User Data Header should be included in the text argument.
// smsc is optional. Can be used to override config file setting.
// Output: messageid
{
  char pdu[1024];
  int retries;
  char command[128];
  char command2[1024];
  char answer[1024];
  char* posi1;
  char* posi2;
#ifdef DEBUGMSG
  printf("!! send_part(device=%i, modem=%i, from=%s, to=%s, text=..., textlen=%i, alphabet=%i, with_udh=%i, udh_data=%s, quick=%i, flash=%i, messageid=..., smsc=%s, report=%i)\n", device, modem, from, to, textlen, alphabet, with_udh, udh_data, quick, flash, smsc, report);
#endif
  
  time_t start_time=time(0);
  // Mark modem as sending
  statistics[device]->status='s';
  // Initialize messageid
  strcpy(messageid,"");
  writelogfile(LOG_INFO,devices[device].name,"Sending SMS from %s to %s",from,to);
  if ((report==1) && (devices[device].incoming==0))
  {
    writelogfile(LOG_NOTICE,devices[device].name,"Cannot receive status report because receiving is disabled");
  }

  if (quick==0)
  {
    // Initialize modem
    if (initmodem(modem, devices[device].name,devices[device].send_delay, errorsleeptime,devices[device].pin, devices[device].initstring, devices[device].initstring2, devices[device].mode, devices[device].smsc)>0)
    {
      statistics[device]->usage_s+=time(0)-start_time;
      return 0;
    }
  }
  
  // Use config file setting if report is unset in file header
  if (report==-1)
    report=devices[device].report;
    
  // Compose the modem command
  make_pdu(to,text,textlen,alphabet,flash,report,with_udh,udh_data,devices[device].mode,pdu);
  if (strcasecmp(devices[device].mode,"old")==0)
    sprintf(command,"AT+CMGS=%i\r",strlen(pdu)/2);
  else
    sprintf(command,"AT+CMGS=%i\r",strlen(pdu)/2-1);

  sprintf(command2,"%s\x1A",pdu);
  
  retries=0;
  while (1)
  {
    // Send modem command
    put_command(modem, devices[device].name,devices[device].send_delay, command, answer, sizeof(answer), 50, "(>)|(ERROR)");
    // Send message if command was successful
    if (! strstr(answer,"ERROR"))
      put_command(modem, devices[device].name,devices[device].send_delay, command2, answer ,sizeof(answer), 300, "(OK)|(ERROR)");
    // Check answer
    if (strstr(answer,"OK"))
    {
      writelogfile(LOG_NOTICE,devices[device].name,"SMS sent, To: %s",to);
      // If the modem answered with an ID number then copy it into the messageid variable.
      posi1=strstr(answer,"CMGS: ");
      if (posi1)
      {
        posi1+=6;
        posi2=strchr(posi1,'\r');
        if (! posi2) 
          posi2=strchr(posi1,'\n');
        if (posi2)
          posi2[0]=0;
        strcpy(messageid,posi1);
#ifdef DEBUGMSG
  printf("!! messageid=%s\n",messageid);
#endif
      }
      statistics[device]->usage_s+=time(0)-start_time;
      statistics[device]->succeeded_counter++;
      return 1;
    }  
    else 
    {
      writelogfile(LOG_ERR,devices[device].name,"The modem said ERROR or did not answer.");
      alarm_handler(LOG_ERR,devices[device].name,"The modem said ERROR or did not answer.");
      retries+=1;
      if (retries<=2)
      {
        writelogfile(LOG_NOTICE,devices[device].name,"Waiting %i sec. before retrying",errorsleeptime);
        sleep(errorsleeptime);
        // Initialize modem after error
        if (initmodem(modem, devices[device].name,devices[device].send_delay, errorsleeptime,devices[device].pin, devices[device].initstring, devices[device].initstring2, devices[device].mode, devices[device].smsc)>0)
        {
          // Cancel if initializing failed
          statistics[device]->usage_s+=time(0)-start_time;
          statistics[device]->failed_counter++;
          writelogfile(LOG_WARNING,devices[device].name,"Sending SMS to %s failed",to);
          alarm_handler(LOG_WARNING,devices[device].name,"Sending SMS to %s failed",to);
          return 0;
        }
      }
      else
      {
        // Cancel if too many retries
        statistics[device]->usage_s+=time(0)-start_time;
        statistics[device]->failed_counter++;
        writelogfile(LOG_WARNING,devices[device].name,"Sending SMS to %s failed",to);
        alarm_handler(LOG_WARNING,devices[device].name,"Sending SMS to %s failed",to);
        return 0;
      }
    }
  }  
}

/* =======================================================================
   Send a whole message, this can have many parts
   ======================================================================= */

int send1sms(int device, int modem, int* quick, int* errorcounter)    
// Search the queues for SMS to send and send one of them.
// Returns 0 if queues are empty
// Returns -1 if sending failed
// Returns 1 if successful
{
  char filename[PATH_MAX];
  char to[100];
  char from[100];
  char smsc[100];
  char provider[100];
  char text[maxtext];
  int with_udh=-1;
  char udh_data[500];
  int textlen;
  char part_text[maxsms_pdu+1];
  int part_text_size;
  char directory[PATH_MAX];
  char cmdline[PATH_MAX+PATH_MAX+32];
  int q,queue;
  int part;
  int parts;
  int maxpartlen;
  int eachpartlen;
  int alphabet;
  int success=0;
  int flash;
  int report;
  int split;
  int tocopy;
  int reserved;
  char messageid[10]={0};
  int found_a_file=0;

#ifdef DEBUGMSG
  printf("!! send1sms(device=%i, modem=%i, quick=%i, errorcounter=%i)\n", device, modem, *quick, *errorcounter);
#endif

  // Search for one single sms file  
  for (q=0; q<PROVIDER; q++)
  {
    if ((devices[device].queues[q][0]) &&
       ((queue=getqueue(devices[device].queues[q],directory))!=-1) &&
       (getfile(directory,filename)) &&
       (lockfile(filename)))
    {
      found_a_file=1;
      break;
    }
  }
 
  // If there is no file waiting to send, then do nothing
  if (found_a_file==0)
  { 
#ifdef DEBUGMSG
  printf("!! No file\n");
  printf("!! quick=%i errorcounter=%i\n",*quick,*errorcounter);
#endif
    return 0;
  }
  else
  {
    readSMSheader(filename,to,from,&alphabet,&with_udh,udh_data,provider,&flash,smsc,&report,&split);
    // Set a default for with_udh if it is not set in the message file.
    if (with_udh==-1)
    {
      if (alphabet==1 || udh_data[0])
        with_udh=1;
      else
        with_udh=0;
    }
    // If the header includes udh-data then enforce udh flag even if it is not 1.
    if (udh_data[0])
      with_udh=1;
    // if Split is unset, use the default value from config file
    if (split==-1) 
      split=autosplit;
    // disable splitting if udh flag or udh_data is 1
    if (with_udh && split)
    {
      split=0;
      writelogfile(LOG_INFO,"smsd","Cannot split this message because it has an UDH.");      
    }
#ifdef DEBUGMSG
  printf("!! to=%s, from=%s, alphabet=%i, with_udh=%i, udh_data=%s, provider=%s, flash=%i, smsc=%s, report=%i, split=%i\n",to,from,alphabet,with_udh,udh_data,provider,flash,smsc,report,split);
#endif
    // If this is a text message, then read also the text    
    if (alphabet<1)
    {
#ifdef DEBUGMSG
  printf("!! This is text message\n");
#endif
      maxpartlen=maxsms_pdu;
      readSMStext(filename,devices[device].cs_convert && (alphabet==0),text,&textlen);
      // Is the message empty?
      if (textlen==0)
      {
        writelogfile(LOG_NOTICE,"smsd","The file %s has no text",filename);
        alarm_handler(LOG_NOTICE,"smsd","The file %s has no text",filename);
        parts=0;
        success=-1;
      }
      // The message is not empty
      else
      {
        // In how many parts do we need to split the text?
        if (split>0)
        {
          // if it fits into 1 SM, then we need 1 part
          if (textlen<=maxpartlen)
          {
            parts=1;
            reserved=0;
            eachpartlen=maxpartlen;
          }
          else if (split==2) // number with text
	  {
            // reserve 4 chars for the numbers
            reserved=4;
            eachpartlen=maxpartlen-reserved;
            parts=(textlen+eachpartlen-1)/(eachpartlen);
	    // If we have more than 9 parts, we need to reserve 6 chars for the numbers
            // And recalculate the number of parts.
	    if (parts>9)
            {
              reserved=6;
              eachpartlen=maxpartlen-reserved;
              parts=(textlen+eachpartlen-1)/(eachpartlen);  
            }
	  }
          else if (split==3) // number with udh
          {
            // reserve 7 chars for the UDH
            reserved=7;
            eachpartlen=maxpartlen-reserved;
            parts=(textlen+eachpartlen-1)/(eachpartlen);
            concatenated_id++;
            if (concatenated_id>255)
              concatenated_id=0;
          }
          else // split==0 or 1, no part numbering
          {
            // no numbering, each part can have the full size
            eachpartlen=maxpartlen;
            reserved=0;
            parts=(textlen+eachpartlen-1)/eachpartlen;
          }
        }
        else
        {
          eachpartlen=maxpartlen;
          reserved=0;
          parts=1; 
        }
	if (parts>1)
	  writelogfile(LOG_INFO,"smsd","Splitting this message into %i parts of max %i characters.",parts,eachpartlen);
      }
    }
    else
    {
#ifdef DEBUGMSG
  printf("!! This is a binary or unicode message.\n");
#endif      
      // Read ucs2 or binary data. No splitting and no conversion.
      // In both cases, the result is stored in text and textlen.
      // How long is the maximum allowed message size
      if (alphabet==2)
        maxpartlen=maxsms_ucs2;
      else
        maxpartlen=maxsms_binary;
      readSMStext(filename,0,text,&textlen);
      eachpartlen=maxpartlen;
      reserved=0;
      parts=1;
      // Is the message empty?
      if (textlen==0)
      {
        writelogfile(LOG_NOTICE,"smsd","The file %s has no text or data.",filename);
        alarm_handler(LOG_NOTICE,"smsd","The file %s has no text or data.",filename);
        parts=0;
        success=-1;
      }
    }
    
    
    // Try to send each part  
    writelogfile(LOG_INFO,"smsd","I have to send %i short message for %s",parts,filename); 
#ifdef DEBUGMSG
  printf("!! textlen=%i\n",textlen);
#endif 
    for (part=0; part<parts; part++)
    {
      if (split==2 && parts>1) // If more than 1 part and text numbering
      {
        sprintf(part_text,"%i/%i ",part+1,parts);
	tocopy=textlen-(part*eachpartlen);
        if (tocopy>eachpartlen)
          tocopy=eachpartlen;
#ifdef DEBUGMSG
  printf("!! tocopy=%i, part=%i, eachpartlen=%i, reserved=%i\n",tocopy,part,eachpartlen,reserved);
#endif 
        memcpy(part_text+reserved,text+(eachpartlen*part),tocopy);
        part_text_size=tocopy+reserved;
      }
      else if (split==3 && parts>1)  // If more than 1 part and UDH numbering
      {
        // in this case the numbers are not part of the text, but UDH instead
	tocopy=textlen-(part*eachpartlen);
        if (tocopy>eachpartlen)
          tocopy=eachpartlen;
#ifdef DEBUGMSG
  printf("!! tocopy=%i, part=%i, eachpartlen=%i, reserved=%i\n",tocopy,part,eachpartlen,reserved);
#endif 
        memcpy(part_text,text+(eachpartlen*part),tocopy);
        part_text_size=tocopy; 
        sprintf(udh_data,"05 00 03 %02X %02X %02X",concatenated_id,parts,part+1); 
        with_udh=1;  
      }
      else  // No part numbers
      {
        tocopy=textlen-(part*eachpartlen);
        if (tocopy>eachpartlen)
          tocopy=eachpartlen;
#ifdef DEBUGMSG
  printf("!! tocopy=%i, part=%i, eachpartlen=%i\n",tocopy,part,eachpartlen);
#endif 
        memcpy(part_text,text+(eachpartlen*part),tocopy);
        part_text_size=tocopy;
      }

  
      // Some modems cannot send if the memory is full. 
      if ((receive_before_send) && (devices[device].incoming))
      { 
        receivesms(device,modem,quick,1);
      }

      // Try to send the sms      
      if (send_part(device, modem, from, to, part_text, part_text_size, alphabet, with_udh, udh_data, *quick, flash ,messageid, smsc, report))
      {
        // Sending was successful
        *quick=1;
        success=1;
      }
      else
      {
        // Sending failed
        *quick=0;
        success=-1;
        // Do not send the next part if sending failed
        break;
      }
      // If receiving has high priority, then receive one message between sending each part.
      if (part<parts-1)
        if (devices[device].incoming==2)
          receivesms(device,modem,quick,0);
    }
    
    // Mark modem status as idle while eventhandler is running
    statistics[device]->status='i';
    
    // Run eventhandler if configured
    if (eventhandler[0] || devices[device].eventhandler[0])
    {
      if (success<1)
        strcpy(text,"FAILED");
      else
        strcpy(text,"SENT");
      if (devices[device].eventhandler[0])
        sprintf(cmdline,"%s %s %s %s",devices[device].eventhandler,text,filename,messageid);
      else
        sprintf(cmdline,"%s %s %s %s",eventhandler,text,filename,messageid);
        my_system(cmdline);
    }
    
    // If sending failed
    if (success==-1)
    {
      // Move file into failed queue or delete
      if (d_failed[0])
      {
        movefilewithdestlock(filename,d_failed);
        stop_if_file_exists("Cannot move",filename,"to",d_failed);
	writelogfile(LOG_INFO,"smsd","Moved file %s to %s",filename,d_failed);
      }
      else
      {
        unlink(filename);
        stop_if_file_exists("Cannot delete",filename,"","");
	writelogfile(LOG_INFO,"smsd","Deleted file %s",filename);
      }
      un_lock(filename);
      // Check how often this modem failed and block it if it seems to be broken
      (*errorcounter)++;
      if (*errorcounter>=blockafter)
      {
        writelogfile(LOG_CRIT,devices[device].name,"Fatal error: sending failed 3 times. Blocking %i sec.",blocktime);
        alarm_handler(LOG_CRIT,devices[device].name,"Fatal error: sending failed 3 times. Blocking %i sec.",blocktime);
        statistics[device]->multiple_failed_counter++;
        statistics[device]->status='b';
        sleep(blocktime);
        *errorcounter=0;
      }
    }
    
    // Sending was successful
    else
    {
      // Move file into failed queue or delete
      if (d_sent[0])
      {
        movefilewithdestlock(filename,d_sent);
        stop_if_file_exists("Cannot move",filename,"to",d_sent);
	writelogfile(LOG_INFO,"smsd","Moved file %s to %s",filename,d_sent);
      }
      else
      {
        unlink(filename);
        stop_if_file_exists("Cannot delete",filename,"","");
	writelogfile(LOG_INFO,"smsd","Deleted file %s",filename);
      }
      un_lock(filename);
    }
    
#ifdef DEBUGMSG
  printf("quick=%i errorcounter=%i\n",*quick,*errorcounter);
  printf("returns %i\n",success);
#endif
    return success;
  }
}


/* =======================================================================
   Device-Spooler (one for each modem)
   ======================================================================= */


void devicespooler(int device)
{
  int workless;
  int quick=0;
  int errorcounter;
  int modem;
  int i;
  
  writelogfile(LOG_INFO,devices[device].name,"Modem handler %i has started.",device);
  errorcounter=0;  
  
  // Open serial port or return if not successful
#ifdef DEBUGMSG
  printf("!! Opening serial port %s\n",devices[device].device);
#endif 
  modem=openmodem(devices[device].device,devices[device].name);
  if (modem==-1)
    return;  
#ifdef DEBUGMSG
  printf("!! Setting modem parameters\n");
#endif   
  setmodemparams(modem, devices[device].name,devices[device].rtscts, devices[device].baudrate);
#ifdef DEBUGMSG
  printf("!! Entering endless send/receive loop\n");
#endif    
  while (terminate==0) /* endless loop */
  {
    workless=1;

    // Send SM
    while (send1sms(device, modem, &quick, &errorcounter)>0)
    {
      workless=0;
      if (devices[device].incoming==2) // repeat only if receiving has low priority
        break;
      if (terminate==1)
        return;
    }

    if (terminate==1)
      return;

    // Receive SM
    if (devices[device].incoming)
      if (receivesms(device, modem, &quick,0)>0) 
      {
        workless=0;
      }

    if (workless==1) // wait a little bit if there was no SM to send or receive to save CPU usage
    {
      // Disable quick mode if modem was workless
      quick=0;
      statistics[device]->status='i';
      for (i=0; i<delaytime; i++)
      {
        if (terminate==1)
          return;
        sleep(1);
      }
    }
  }
}

/* =======================================================================
   Termination handler
   ======================================================================= */

// Stores termination request when termination signal has been received 
   
void soft_termination_handler (int signum)
{
  if (thread_id==-1)
  {
    signal(SIGTERM,SIG_IGN);
    signal(SIGINT,SIG_IGN);
    writelogfile(LOG_CRIT,"smsd","Smsd main program received termination signal.");
    if (signum==SIGINT)
     printf("Received SIGINT, smsd will terminate now.\n");  
    kill(0,SIGTERM);
  }
  else if (thread_id>=0)
  {
    signal(SIGTERM,SIG_IGN);
    signal(SIGINT,SIG_IGN);
    // thread_id has always the same value like device when it is larger than -1
    writelogfile(LOG_CRIT,devices[thread_id].name,"Modem handler %i has received termination signal, will terminate after current task has been finished.",thread_id);
  }
  terminate=1;
}


/* =======================================================================
   Main
   ======================================================================= */

int main(int argc,char** argv)
{
  int i;
  setpgid(0,0);
  signal(SIGTERM,soft_termination_handler);
  signal(SIGINT,soft_termination_handler);
  signal(SIGHUP,SIG_IGN);
  parsearguments(argc,argv);
  initcfg();
  readcfg();
  logfilehandle=openlogfile(logfile,"smsd",LOG_DAEMON,loglevel);  
  initstats();
  loadstats();
  writelogfile(LOG_CRIT,"smsd","Smsd v%s started.",smsd_version);  
  // Start sub-processes for each modem
  for (i=0; i<DEVICES; i++)
  {
    if (devices[i].name[0])
      if (fork()==0)
      {
        thread_id=i;
        write_pid("/var/run/smsd.pid");
        devicespooler(i);
        statistics[i]->status='b';
        remove_pid("/var/run/smsd.pid");
        writelogfile(LOG_CRIT,devices[i].name,"Modem handler %i terminated.",i);
        exit(127);
      }
  } 
  // Start main program
  thread_id=-1;
  write_pid("/var/run/smsd.pid");
  mainspooler();
  writelogfile(LOG_CRIT,"smsd","Smsd main program is awaiting the termination of all modem handlers.");
  waitpid(0,0,0);
  savestats();
#ifndef NOSTATS
    MM_destroy();
#endif
  remove_pid("/var/run/smsd.pid");
  writelogfile(LOG_CRIT,"smsd","Smsd main program terminated.");
  return 0;
}
