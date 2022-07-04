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

#ifndef MODEMINIT_H
#define MODEMINIT_H



// Open the serial port, returns file handle or -1 on error
// Device is the name of serial port.
// modemname is a name for the modem, used for alarm_handler and logging
int openmodem(char* device, char* modemname); 




// Setup serial port
// modem is the file handle as returned by open()
// modemname is a name for the modem, used for alarm_handler and logging
// rtscts 0/1 enables hardware handshake
// baudrate is the baudrate as integer number
void setmodemparams(int modem, char* modemname, int rtscts, int baudrate);




// Send init strings. 
// Returns 0 on success
//         1 modem is not ready
//         2 cannot enter pin
//         3 cannot enter init strings
//         4 modem is not registered
//         5 cannot enter pdu mode
//         6 cannot enter smsc number

// modem is the file handle as returned by open()
// modemname is a name for the modem, used for alarm_handler and logging
// send_delay can be used to inser a delay between sending each character
// error_sleeptime number of seconds to sleep after ERROR answer
// pin is the pin for the SIM card (can be empty)
// initstring1 and initstring2 are two initialisation commands (can be empty)
// mode can me old, new or ascii
// smsc sets the sms service center (can be empty)
int initmodem(int modem, char* modemname, int send_delay, int error_sleeptime, char* pin, char* initstring1, char* initstring2, char* mode, char* smsc); 




// Sends a command to the modem and waits max timout*0.1 seconds for an answer.
// The function returns the length of the answer.
// The function waits until a timeout occurs or the expected answer occurs. 

// modem is the serial port file handle
// modemname is a name for the modem, used for alarm_handler and logging
// send_delay can be used to inser a delay between sending each character
// command is the command to send (may be empty or NULL)
// answer is the received answer
// max is the maxmum allowed size of the answer
// timeout control the time how long to wait for the answer
// expect is an extended regular expression. If this matches the modem answer, 
// then the program stops waiting for the timeout (may be empty or NULL).

int put_command(int modem, char* modemname, int send_delay, char* command,char* answer,int max,int timeout,char* expect);

#endif
