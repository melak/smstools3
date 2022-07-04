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

#ifndef SMSD_CFG_H
#define SMSD_CFG_H

#define PROVIDER 16 
#define DEVICES 32
#define NUMS 16

// Maximum size of a message text
#define maxtext 2048

// Maxmum size of a single sms, can be 160/140 or less
#define maxsms_pdu 160
#define maxsms_ucs2 140
#define maxsms_binary 140

// Block modem after n errors
#define blockafter 3

#include <limits.h>

typedef struct
{
  char name[32]; 		// Name of the queue
  char numbers[NUMS][16];	// Phone numbers assigned to this queue
  char directory[PATH_MAX];		// Queue directory
} _queue;

typedef struct
{
  char name[32];		// Name of the modem
  char device[PATH_MAX];        // Serial port name
  char queues[PROVIDER][32];	// Assigned queues
  int incoming; 		// Try to receive SMS. 0=No, 1=Low priority, 2=High priority
  int report; 			// Ask for delivery report 0 or 1 (experimental state)
  char pin[16];			// SIM PIN
  char mode[10]; 		// Command set version old or new
  char smsc[16];		// Number of SMSC
  int baudrate;			// Baudrate
  int send_delay;		// Makes sending characters slower (milliseconds)
  int cs_convert; 		// Convert character set  0 or 1 (iso-9660)
  char initstring[100];		// first Init String
  char initstring2[100];        // second Init String
  char eventhandler[PATH_MAX];	// event handler program or script
  int rtscts;			// hardware handshake RTS/CTS, 0 or 1
  int read_memory_start;	// first memory space for sms
} _device;

int debug;			// 1 if debug mode (do not really send and do not delete received SM
char configfile[PATH_MAX];	// Path to config file
char mypath[PATH_MAX];		// Path to binaries
char d_spool[PATH_MAX];		// Spool directory
char d_failed[PATH_MAX];	// Failed spool directory
char d_incoming[PATH_MAX];	// Incoming spool directory
char d_sent[PATH_MAX];		// Sent spool directory
char d_checked[PATH_MAX];	// Spool directory for checked messages (only used when no provider queues used)
char eventhandler[PATH_MAX];	// Global event handler program or script
char alarmhandler[PATH_MAX];	// Global alarm handler program or script
char checkhandler[PATH_MAX];    // Handler that checks if the sms file is valid.
int alarmlevel;			// Alarm Level (9=highest). Verbosity of alarm handler.
char logfile[PATH_MAX];		// Name or Handle of Log File
int  loglevel;			// Log Level (9=highest). Verbosity of log file.
_queue queues[PROVIDER];	// Queues
_device devices[DEVICES];	// Modem devices
int delaytime;			// sleep-time after workless
int blocktime;			// sleep-time after multiple errors
int errorsleeptime;		// sleep-time after each error
int autosplit;			// Splitting of large text messages 0=no, 1=yes 2=number with text, 3=number with UDH
int receive_before_send;	// if 1 smsd tries to receive one message before sending

/* initialize all variable with default values */

void initcfg();


/* read the config file */

void readcfg();


/* Retuns the array-index and the directory of a queue or -1 if
   not found. Name is the name of the queue or a phone number. */

int getqueue(char* name, char* directory);


/* Returns the array-index of a device or -1 if not found */

int getdevice(char* name);


/* Show help */

void help();

/* parse arguments */

void parsearguments(int argc,char** argv);

#endif
