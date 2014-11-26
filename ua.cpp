/*
** UNaXcess II Conferencing System
** (c) 1998 Michael Wood (mike@compsoc.man.ac.uk)
**
** Concepts based on Bradford UNaXcess (c) 1984-87 Brandon S Allbery
** Extensions (c) 1989, 1990 Andrew G Minter
** Manchester UNaXcess extensions by Rob Partington, Gryn Davies,
** Michael Wood, Andrew Armitage, Francis Cook, Brian Widdas
**
** The look and feel was reproduced. No code taken from the original
** UA was someone else's inspiration. Copyright and 'nuff respect due
**
** ua.cpp: Implmentation for common UA functions
*/

#include "stdafx.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "useful/useful.h"

#include "ua.h"

bool NameValid(const char *szName)
{
   int iCharPos = 1;

   // printf("NameValid %s\n", szName);

   if(szName == NULL)
   {
      // printf("NameValid exit false, NULL\n");
      return false;
   }

   if(strlen(szName) == 0 || strlen(szName) > UA_NAME_LEN)
   {
      // printf("NameValid exit false, too long\n");
      return false;
   }

   if(!isalpha(szName[0]))
   {
      // printf("NameValid exit false, non-alpha first char\n");
      return false;
   }

   while(szName[iCharPos] != '\0')
   {
      if(!isalnum(szName[iCharPos]) && strchr(".,' @-_!", szName[iCharPos]) == NULL)
      {
         // printf("NameValid exit false, invalid char %d\n", iCharPos);
         return false;
      }
      else
      {
         iCharPos++;
      }
   }

   if(szName[iCharPos - 1] == ' ')
   {
      // printf("NameValid exit false, space last char\n");
      return false;
   }

   // printf(". exit true\n");
   return true;
}

// AccessName: Convert numerical access level into character string
char *AccessName(int iLevel, int iType)
{
   if(iType != -1 && iType == USERTYPE_AGENT)
   {
      return "Agent";
   }

   switch(iLevel)
   {
      case LEVEL_NONE:
         return "None";

      case LEVEL_GUEST:
         return "Guest";

      case LEVEL_MESSAGES:
         return "Messages";

      case LEVEL_EDITOR:
         return "Editor";

      case LEVEL_WITNESS:
         return "Witness";

      case LEVEL_SYSOP:
         return "SysOp";
   }

   return "";
}

char *SubTypeStr(int iSubType)
{
	STACKTRACE
   if(iSubType == SUBTYPE_EDITOR)
   {
      return SUBNAME_EDITOR;
   }
   else if(iSubType == SUBTYPE_MEMBER)
   {
      return SUBNAME_MEMBER;
   }
   else if(iSubType == SUBTYPE_SUB)
   {
      return SUBNAME_SUB;
   }

   return "";
}

int SubTypeInt(const char *szSubType)
{
	STACKTRACE
   if(stricmp(szSubType, SUBNAME_EDITOR) == 0)
   {
      return SUBTYPE_EDITOR;
   }
   else if(stricmp(szSubType, SUBNAME_MEMBER) == 0)
   {
      return SUBTYPE_MEMBER;
   }
   else if(stricmp(szSubType, SUBNAME_SUB) == 0)
   {
      return SUBTYPE_SUB;
   }

   return -1;
}

/* int ProtocolVersion(const char *szVersion)
{
   return ProtocolCompare(PROTOCOL, szVersion);
} */

int ProtocolCompare(const char *szVersion1, const char *szVersion2)
{
   return stricmp(szVersion1, szVersion2);
}
