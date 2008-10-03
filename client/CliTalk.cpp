/*
** UNaXcess Conferencing System
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
** CliTalk.cpp: Implementation of client side channel functions
*/

#include <stdio.h>
#include <string.h>

#include "EDF/EDF.h"

#include "ua.h"
#include "CliTalk.h"

// Internal channel get method
bool ChannelGet(EDF *pData, int *iChannelID, char **szChannelName, int iSearchType, bool bReset)
{
	STACKTRACE
   int iChannelEDF = -1;
   char *szChannelEDF = NULL;
   bool bLoop = true, bFound = false, bGetCopy = false;

	if((iSearchType == 0 && (szChannelName == NULL || *szChannelName == NULL)) ||
	   (iSearchType == 1 && (iChannelID == NULL || *iChannelID <= 0)))
	{
		return false;
	}

   bGetCopy = pData->DataCopy(false);
   if(bReset == true)
	{
       pData->TempMark();
	}
	
   // Position to first channel
   pData->Root();
   pData->Child("channels");
   bLoop = pData->Child("channel");
   while(bFound == false && bLoop == true)
   {
      if(iSearchType == 0)
      {
         // Search by name
         pData->GetChild("name", &szChannelEDF);
         if(stricmp(*szChannelName, szChannelEDF) == 0)
         {
            // Set channel ID return (make a copy of the name)
            pData->Get(NULL, iChannelID);
            strcpy(*szChannelName, szChannelEDF);
            bFound = true;
         }
      }
      else
      {
         // Search by ID
         pData->Get(NULL, &iChannelEDF);
         if(*iChannelID == iChannelEDF)
         {
            if(szChannelName != NULL)
            {
               // Set channel name return (use a copy)
               pData->GetChild("name", &szChannelEDF);
               *szChannelName = strmk(szChannelEDF);
            }
            bFound = true;
         }
      }
      if(bFound == false)
      {
         bLoop = pData->Iterate("channel", "channels");
      }
   }
   
   if(bReset == true)
   {
      pData->TempUnmark();
   }

   pData->DataCopy(bGetCopy);

   return bFound;
}

// Get channel from name
int ChannelGet(EDF *pData, char *&szChannelName, bool bReset)
{
	STACKTRACE
   int iChannelID = -1;

   // debug("ChannelGet entry %s\n", szChannelName);

   ChannelGet(pData, &iChannelID, &szChannelName, 0, bReset);

   // debug("ChannelGet exit %d\n", iChannelID);
   return iChannelID;
}

// Get channel from ID
bool ChannelGet(EDF *pData, int iChannelID, char **szChannelName, bool bReset)
{
	STACKTRACE
   bool bReturn = false;

   // debug("ChannelGet entry %d\n", iChannelID);

   bReturn = ChannelGet(pData, &iChannelID, szChannelName, 1, bReset);

   // debug("ChannelGet exit %s\n", BoolStr(bReturn));
   return bReturn;
}

bool MessageInChannel(EDF *pData, int iMsgID)
{
	STACKTRACE
   bool bLoop = true, bFound = false;
   int iMsgEDF = 0;

   // debug("MessageInChannel entry %d\n", iMsgID);

   if(iMsgID == EDFElement::FIRST)
   {
      bFound = pData->Child("message");
   }
   else if(iMsgID == EDFElement::LAST)
   {
      bFound = pData->GetChild("message", &iMsgID);
      while(pData->Child("message", EDFElement::LAST) == true)
      {
         pData->Last("message");
      }
      // debug("Last child %s\n", BoolStr(bFound));
   }
   else
   {
      bLoop = pData->Child("message");
   }

   while(bFound == false && bLoop == true)
   {
      pData->Get(NULL, &iMsgEDF);
      if(iMsgID == iMsgEDF)
      {
         bFound = true;
      }
      else
      {
         bLoop = pData->Iterate("message", "channel", false);
      }
   }

   // debug("MessageInChannel exit %s, %d\n", BoolStr(bFound), iMsgEDF);
   
   return bFound;
}
