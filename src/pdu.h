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

#ifndef PDU_H
#define PDU_H

//Alphabet values: -1=GSM 0=ISO 1=binary 2=UCS2


// Make the PDU string from a mesage text and destination phone number.
// The destination variable pdu has to be big enough. 
// alphabet indicates the character set of the message.
// flash_sms enables the flash flag.
// mode select the pdu version (old or new).
// if udh is true, then udh_data contains the optional user data header in hex dump, example: "05 00 03 AF 02 01"

void make_pdu(char* number, char* message, int messagelen, int alphabet, int flash_sms, int report, int udh, char* udh_data, char* mode, char* pdu);



// Splits a PDU string into the parts 
// Input: 
// pdu is the pdu string
// mode can be old or new and selects the pdu version
// Output:
// alphabet indicates the character set of the message.
// sendr Sender
// date and time Date/Time-stamp
// message is the message text or binary message
// smsc that sent this message
// with_udh returns the udh flag of the message
// is_statusreport is 1 if this was a status report
// udh return the udh as hex dump
// Returns the length of the message 

int splitpdu(char* pdu, char* mode, int* alphabet, char* sendr, char* date, char* time, char* message, char* smsc, int* with_udh, char* udh_data, int* is_statusreport);


#endif
