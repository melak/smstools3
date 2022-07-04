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

#include "cfgfile.h"
#include "extras.h"
#include <sys/types.h>
#include <limits.h>
#include <string.h>

void cutcomment(char*  text)
{
  char* cp;
  int laenge;
  cp=strchr(text,'#');
  if (cp!=0)
    *cp=0;
  laenge=strlen(text);
  while ((laenge>0)&&(text[laenge-1]<=' '))
  {
    text[laenge-1]=0;
    laenge--;
  }
}

int yesno( char*  value)
{
  if ((value[0]=='1') ||
      (value[0]=='y') ||
      (value[0]=='Y') ||
      (value[0]=='t') ||
      (value[0]=='T') ||
      (value[1]=='n') && (
        (value[0]=='o') ||
        (value[0]=='O')
      ))
    return 1;
  else
    return 0;
}

int getsubparam( char*  parameter, 
                 int n, 
                char*  subparam,  int size_subparam)
{
  int j;
  char* cp;
  char* cp2;
  int size;
  cp=(char*)parameter;
  subparam[0]=0;
  for (j=1; j<n; j++)
  {
    if (!strchr(cp,','))
      return 0;
    cp=strchr(cp,',')+1;
  }
  cp2=strchr(cp,',');
  if (cp2)
    size=cp2-cp;
  else
    size=strlen(cp);
  strncpy(subparam,cp,size);
  subparam[size]=0;
  cutspaces(subparam);
  return 1;
}

int splitline( char*  source, 
              char*  name,  int size_name,
	      char*  value,  int size_value)
{
  char* equalchar;
  int n;
  equalchar=strchr(source,'=');
  value[0]=0;
  name[0]=0;
  if (equalchar)
  {
    strncpy(value,equalchar+1,size_value-1);
    value[size_value-1]=0;
    cutspaces(value);
    n=equalchar-source;
    if (n>0)
    {
      if (n>(size_name-1))
        n=(size_name-1);
      strncpy(name,source,n);
      name[n]=0;
      cutspaces(name);
      return 1;
    }
  }
  return 0;
}

int gotosection(FILE* file,  char*  name)
{
  char line[PATH_MAX+32];
  int Length;
  char* posi;
  fseek(file,0,SEEK_SET);
  while (fgets(line,sizeof(line),file))
  {
    cutcomment(line);
    Length=strlen(line);
    if (Length>2)
    {
      posi=strchr(line,']');
      if ((line[0]=='[') && posi)
        *posi=0;
        if (strcmp(line+1,name)==0)
	  return 1;
    }
  }
  return 0;
}

int my_getline(FILE* file,
            char*  name,  int size_name,
	    char*  value,  int size_value)
{
  char line[PATH_MAX+32];
  int Length;
  while (fgets(line,sizeof(line),file))
  {
    cutcomment(line);
    Length=strlen(line);
    if (Length>2)
    {
      if (line[0]=='[')
        return 0;
      else
        if (splitline(line,name,size_name,value,size_value)==0)
	{
	  strncpy(value,line,size_value-1);
	  value[size_value-1]=0;
	  return -1;
	}
	else
	  return 1;
    }
  }
  return 0;
}
