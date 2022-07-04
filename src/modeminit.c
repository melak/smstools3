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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <regex.h>
#include "logging.h"
#include "alarm.h"

#ifdef SOLARIS
#include <sys/filio.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/ioctl.h>
#include <errno.h>
#include "extras.h"
#include "modeminit.h"
#include "smsd_cfg.h"

// Define a dummy if the OS does not support hardware handshake
#ifndef CRTSCTS
#define CRTSCTS 0
#endif

// The following functions are for internal use of put_command and therefore not in the modeminit.h.


int write_to_modem(int modem,char* modemname,int send_delay,char* command,int timeout)
{  
  int status=0;
  int timeoutcounter=0;
  int x=0;
  struct termios tio;
  tcgetattr(modem,&tio);
  
  if (command && command[0])
  {
    if (tio.c_cflag & CRTSCTS)
    {
      ioctl(modem,TIOCMGET,&status);
      while (!(status & TIOCM_CTS))
      {
        usleep(100000);
        timeoutcounter++;
        ioctl(modem,TIOCMGET,&status);
        if (timeoutcounter>timeout)
        {
          writelogfile(LOG_ERR,modemname,"Modem is not clear to send");
          alarm_handler(LOG_ERR,modemname,"Modem is not clear to send");
          return 0;
        }
      }
    }
    writelogfile(LOG_DEBUG,modemname,"-> %s",command);
    for(x=0;x<strlen(command);x++) 
    {
      if (write(modem,command+x,1)<1)
      {
        writelogfile(LOG_ERR,modemname,"Could not send character %c, cause: %s",command[x],strerror(errno));
        alarm_handler(LOG_ERR,modemname,"Could not send character %c, cause: %s",command[x],strerror(errno));
        return 0;
      }
      if (send_delay)
        usleep(send_delay*1000);
      tcdrain(modem);
    }  
  }
  return 1;
}

// Read max characters from modem. The function returns when it received at 
// least 1 character and then the modem is quiet for timeout*0.1s.
// The answer might contain already a string. In this case, the answer 
// gets appended to this string.
int read_from_modem(int modem, char* modemname, int send_delay, char* answer,int max,int timeout)
{
  int count=0;
  int got=0;
  int timeoutcounter=0;
  int success=0;
  int toread=0;
  
  // Cygwin does not support TIOC functions, so we cannot use it.
  // ioctl(modem,FIONREAD,&available);	// how many bytes are available to read?

  do 
  {
    // How many bytes do I want to read maximum? Not more than buffer size -1 for termination character.
    count=strlen(answer);
    toread=max-count-1;
    if (toread<=0)
      break;
    // read data
    got=read(modem,answer+count,toread);
    // if nothing received ...
    if (got<=0)
    {
      // wait a litte bit and then repeat this loop
      got=0;
      usleep(100000);
      timeoutcounter++;
    }
    else  
    {
      // restart timout counter
      timeoutcounter=0;
      // append a string termination character
      answer[count+got]=0;
      success=1;      
    }
  }
  while (timeoutcounter < timeout);
  return success;
}


// Exported functions:


int put_command(int modem, char* modemname, int send_delay,char* command,char* answer,int max,int timeout,char* expect)
{
  char loganswer[2048];
  int timeoutcounter=0;
  regex_t re;

  // compile regular expressions
  if (expect && expect[0] && (regcomp(&re,expect, REG_EXTENDED|REG_NOSUB) != 0)) 
  {	  
    fprintf(stderr,"Programming error: Modem answer %s is not a valid regepr\n",expect);
    exit(1);
  }  

  // clean input buffer
  // It seems that this command does not do anything because actually it 
  // does not clear the input buffer. However I do not remove it until I 
  // know why it does not work.
  tcflush(modem,TCIFLUSH);
  
  // send command
  if (write_to_modem(modem,modemname,send_delay,command,30)==0)
  {
    sleep(errorsleeptime);
    return 0;
  }
  writelogfile(LOG_DEBUG,modemname,"Command is sent, waiting for the answer");

  // wait for the modem-answer 
  answer[0]=0;
  timeoutcounter=0;
  do
  {
    read_from_modem(modem,modemname,send_delay,answer,max,2);  // One read attempt is 200ms

    // check if it's the expected answer
    if (expect && expect[0] && (regexec(&re, answer, (size_t) 0, NULL, 0)==0))
        break;
    timeoutcounter+=2;
  }
  // repeat until timout
  while (timeoutcounter<timeout);

  strncpy(loganswer,answer,sizeof(loganswer)-1);
  loganswer[sizeof(loganswer)-1]=0;
  cutspaces(loganswer);
  cut_emptylines(loganswer);
  writelogfile(LOG_DEBUG,modemname,"<- %s",loganswer);

  // Free memory used by regexp
  regfree(&re);

  return strlen(answer);
}

void setmodemparams(int modem, char* modemname, int rtscts,int baudrate) /* setup serial port */
{
  struct termios newtio;
  bzero(&newtio, sizeof(newtio));
  newtio.c_cflag = CS8 | CLOCAL | CREAD | O_NDELAY | O_NONBLOCK;
  if (rtscts)
    newtio.c_cflag |= CRTSCTS;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;
  newtio.c_lflag = 0;
  newtio.c_cc[VTIME]    = 0;
  newtio.c_cc[VMIN]     = 0;
  switch (baudrate)
  {
    case 300:    baudrate=B300; break;
    case 1200:   baudrate=B1200; break;
    case 2400:   baudrate=B2400; break;
    case 9600:   baudrate=B9600; break;
    case 19200:  baudrate=B19200; break;
    case 38400:  baudrate=B38400; break;
#ifdef B57600
    case 57600:  baudrate=B57600; break;
#endif
#ifdef B115200
    case 115200: baudrate=B115200; break;
#endif
#ifdef B230400
    case 230400: baudrate=B230400; break;
#endif
    default: writelogfile(LOG_ERR,modemname,"Baudrate not supported, using 9600"); baudrate=B9600;    
  }
  cfsetispeed(&newtio,baudrate);
  cfsetospeed(&newtio,baudrate);
  tcsetattr(modem,TCSANOW,&newtio);
}

int initmodem(int modem, char* modemname, int send_delay, int error_sleeptime, char* pin, char* initstring1, char* initstring2, char* mode, char* smsc) 
{
  char command[100];
  char answer[500];
  int retries=0;
  int success=0;

  writelogfile(LOG_INFO,modemname,"Checking if modem is ready");
  retries=0;
  do
  {
    retries++;
    put_command(modem,modemname,send_delay,"AT\r",answer,sizeof(answer),50,"(OK)|(ERROR)");
    if ( strstr(answer,"OK")==NULL && strstr(answer,"ERROR")==NULL)
    {
      // if Modem does not answer, try to send a PDU termination character
      put_command(modem,modemname,send_delay,"\x1A\r",answer,sizeof(answer),50,"(OK)|(ERROR)");
    }
  }
  while ((retries<=10) && (! strstr(answer,"OK")));
  if (! strstr(answer,"OK"))
  {
    writelogfile(LOG_ERR,modemname,"Modem is not ready to answer commands");
    alarm_handler(LOG_ERR,modemname,"Modem is not ready to answer commands");
    return 1;
  }

  if (pin[0])
  {
    writelogfile(LOG_INFO,modemname,"Checking if modem needs PIN");
    put_command(modem,modemname,send_delay,"AT+CPIN?\r",answer,sizeof(answer),50,"(\\+CPIN:.*OK)|(ERROR)");
    if (strstr(answer,"+CPIN: SIM PIN"))
    {
      writelogfile(LOG_NOTICE,modemname,"Modem needs PIN, entering PIN...");
      sprintf(command,"AT+CPIN=\"%s\"\r",pin);
      put_command(modem,modemname,send_delay,command,answer,sizeof(answer),300,"(OK)|(ERROR)");
      put_command(modem,modemname,send_delay,"AT+CPIN?\r",answer,sizeof(answer),50,"(\\+CPIN:.*OK)|(ERROR)");
      if (strstr(answer,"+CPIN: SIM PIN"))
      {
        writelogfile(LOG_ERR,modemname,"Modem did not accept this PIN");
        alarm_handler(LOG_ERR,modemname,"Modem did not accept this PIN");
        return 2;
      }
      else if (strstr(answer,"+CPIN: READY"))
        writelogfile(LOG_INFO,modemname,"PIN Ready");
    }
    if (strstr(answer,"+CPIN: SIM PUK"))
    {
      writelogfile(LOG_CRIT,modemname,"Your PIN is locked. Unlock it manually");
      alarm_handler(LOG_CRIT,modemname,"Your PIN is locked. Unlock it manually");
      return 2;
    }
  }

  if (initstring1[0])
  {
    writelogfile(LOG_INFO,modemname,"Initializing modem");
    put_command(modem,modemname,send_delay,initstring1,answer,sizeof(answer),100,"(OK)|(ERROR)");
    if (strstr(answer,"OK")==0)
    {
      writelogfile(LOG_ERR,modemname,"Modem did not accept the init string");
      alarm_handler(LOG_ERR,modemname,"Modem did not accept the init string");
      return 3;
    }
  }

  if (initstring2[0])
  {
    put_command(modem,modemname,send_delay,initstring2,answer,sizeof(answer),100,"(OK)|(ERROR)");
    if (strstr(answer,"OK")==0)
    {
      writelogfile(LOG_ERR,modemname,"Modem did not accept the second init string");
      alarm_handler(LOG_ERR,modemname,"Modem did not accept the second init string");
      return 3;
    }
  }

    writelogfile(LOG_INFO,modemname,"Checking if Modem is registered to the network");
    success=0;
    retries=0;
    do
    {
      retries++;
      put_command(modem,modemname,send_delay,"AT+CREG?\r",answer,sizeof(answer),100,"(\\+CREG:.*OK)|(ERROR)");
      if (strstr(answer,"1"))
      {
        writelogfile(LOG_INFO,modemname,"Modem is registered to the network");
        success=1;
      }
      else if (strstr(answer,"5"))
      {
      	// added by Thomas Stoeckel
      	writelogfile(LOG_INFO,modemname,"Modem is registered to a roaming partner network");
	success=1;
      }
      else if (strstr(answer,"ERROR"))
      {
        writelogfile(LOG_INFO,modemname,"Ignoring that modem does not support +CREG command.");
	success=1;
      }
      else if (strstr(answer,"+CREG:"))
      {
        writelogfile(LOG_NOTICE,modemname,"Modem is not registered, waiting %i sec. before retrying",error_sleeptime);
        sleep(error_sleeptime);
	success=0;
      }
      else
      {
        writelogfile(LOG_ERR,modemname,"Error: Unexpected answer from Modem after +CREG?, waiting %i sec. before retrying",error_sleeptime);
	alarm_handler(LOG_ERR,modemname,"Error: Unexpected answer from Modem after +CREG?, waiting %i sec. before retrying",error_sleeptime);
	sleep(error_sleeptime);
	success=0;
      }
    }
    while ((success==0)&&(retries<10));


  if (success==0)
  {
    writelogfile(LOG_ERR,modemname,"Error: Modem is not registered to the network");
    alarm_handler(LOG_ERR,modemname,"Error: Modem is not registered to the network");
    return 4;
  }


  if (mode[0])
  {
    if ((strcmp(mode,"ascii")==0))
    {
      writelogfile(LOG_INFO,modemname,"Selecting ASCII mode");
      strcpy(command,"AT+CMGF=1\r");
    }
    else
    {
      writelogfile(LOG_INFO,modemname,"Selecting PDU mode");
      strcpy(command,"AT+CMGF=0\r");
    }
    put_command(modem,modemname,send_delay,command,answer,sizeof(answer),50,"(OK)|(ERROR)");
    if (strstr(answer,"ERROR"))
    {
      writelogfile(LOG_ERR,modemname,"Error: Modem did not accept mode selection");
      alarm_handler(LOG_ERR,modemname,"Error: Modem did not accept mode selection");
      return 5;
    }
  }

  if (smsc[0])
  {
    writelogfile(LOG_INFO,modemname,"Changing SMSC");
    sprintf(command,"AT+CSCA=\"+%s\"\r",smsc);
    put_command(modem,modemname,send_delay,command,answer,sizeof(answer),50,"(OK)|(ERROR)");
    if (strstr(answer,"ERROR"))
    {
      writelogfile(LOG_ERR,modemname,"Error: Modem did not accept SMSC");
      alarm_handler(LOG_ERR,modemname,"Error: Modem did not accept SMSC");
      return 6;
    }
  }
  return 0;
}

int openmodem(char* device,char* modemname)
{
  int modem;
  modem = open(device, O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (modem <0)
  {
    writelogfile(LOG_ERR,modemname,"Cannot open serial port, cause: %s",strerror(errno));
    alarm_handler(LOG_ERR,modemname,"Cannot open serial port, error: %s",strerror(errno));
    return -1;
  }
  return modem;
}

