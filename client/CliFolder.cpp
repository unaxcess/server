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
** CliFolder.cpp: Implementation of client side folder functions
*/

#include <stdio.h>
#include <string.h>

#include "EDF/EDF.h"

#include "ua.h"
#include "CliFolder.h"

// Internal folder get method
bool FolderGet(EDF *pData, int *iFolderID, char **szFolderName, int iSearchType, bool bReset)
{
   STACKTRACE
   int iFolderEDF = -1;
   char *szFolderEDF = NULL;
   bool bLoop = true, bFound = false, bGetCopy = false;

   if((iSearchType == 0 && (szFolderName == NULL || *szFolderName == NULL)) ||
      (iSearchType == 1 && (iFolderID == NULL || *iFolderID <= 0)))
   {
      return false;
   }

   bGetCopy = pData->DataCopy(false);
   if(bReset == true)
   {
      pData->TempMark();
   }
	
   // Position to first folder
   pData->Root();
   // pData->Child("folders");
   bLoop = pData->Child("folder");
   while(bFound == false && bLoop == true)
   {
      if(iSearchType == 0)
      {
         // Search by name
         pData->GetChild("name", &szFolderEDF);
	 debug(DEBUGLEVEL_DEBUG, "FolderGet check %s\n", szFolderEDF);
         if(stricmp(*szFolderName, szFolderEDF) == 0)
         {
            // Set folder ID return (make a copy of the name)
            pData->Get(NULL, iFolderID);
            strcpy(*szFolderName, szFolderEDF);
            bFound = true;
         }
      }
      else
      {
         // Search by ID
         pData->Get(NULL, &iFolderEDF);
	 debug(DEBUGLEVEL_DEBUG, "FolderGet check %d\n", iFolderEDF);
         if(*iFolderID == iFolderEDF)
         {
            if(szFolderName != NULL)
            {
               // Set folder name return (use a copy)
               pData->GetChild("name", &szFolderEDF);
               *szFolderName = strmk(szFolderEDF);
            }
            bFound = true;
         }
      }
      if(bFound == false)
      {
         bLoop = pData->Iterate("folder");//, "folders");
      }
   }
   
   if(bReset == true)
   {
      pData->TempUnmark();
   }

   pData->DataCopy(bGetCopy);

   return bFound;
}

// Get folder from name
int FolderGet(EDF *pData, char *&szFolderName, bool bReset)
{
	STACKTRACE
   int iFolderID = -1;

   // debug("FolderGet entry %s\n", szFolderName);

   FolderGet(pData, &iFolderID, &szFolderName, 0, bReset);

   // debug("FolderGet exit %d\n", iFolderID);
   return iFolderID;
}

// Get folder from ID
bool FolderGet(EDF *pData, int iFolderID, char **szFolderName, bool bReset)
{
	STACKTRACE
   bool bReturn = false;

   // debug("FolderGet entry %d\n", iFolderID);

   bReturn = FolderGet(pData, &iFolderID, szFolderName, 1, bReset);

   // debug("FolderGet exit %s\n", BoolStr(bReturn));
   return bReturn;
}

bool MessageInFolder(EDF *pData, int iMsgID)
{
	STACKTRACE
   bool bLoop = true, bFound = false;
   int iMsgEDF = 0;

   // debug("MessageInFolder entry %d\n", iMsgID);

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
         bLoop = pData->Iterate("message", "folder", false);
      }
   }

   // debug("MessageInFolder exit %s, %d\n", BoolStr(bFound), iMsgEDF);
   
   return bFound;
}
