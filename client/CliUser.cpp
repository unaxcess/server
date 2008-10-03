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
** CliUser.cpp: Implementation of common user functions
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "EDF/EDF.h"

#include "ua.h"
#include "CliUser.h"

// UserGet: Find a given user
// iUserID -         ID of the user to search for
// iSearchType -     Flag for which search (0 = use szUserName, 1 = use iUserID)
// bReset -          Flag for temporary position setting
bool UserGet(EDF *pData, int *iUserID, char **szUserName, int iSearchType, bool bReset, long lDate)
{
	STACKTRACE
   int iUserEDF = -1;
   long lDateEDF = 0;
   char *szUserEDF = NULL;
   bool bLoop = true, bFound = false, bGetCopy = false;

   if((iSearchType == 0 && (szUserName == NULL || *szUserName == NULL)) ||
      (iSearchType == 1 && (iUserID == NULL || *iUserID <= 0)))
   {
      return false;
   }
   
   bGetCopy = pData->DataCopy(false);
   if(bReset == true)
   {
		// Mark position for reset
       pData->TempMark();
   }

   /* if(lDate != -1)
   {
      debug("UserGet userid %d (date %ld)\n", *iUserID, lDate);
   } */

   // Move to first user
   pData->Root();
   pData->Child("users");
   bLoop = pData->Child("user");
   while(bFound == false && bLoop == true)
   {
      if(lDate != -1 || pData->GetChildBool("deleted") == false)
      {
         if(iSearchType == 0)
         {
            // Search by name
            pData->GetChild("name", &szUserEDF);
            if(stricmp(*szUserName, szUserEDF) == 0)
            {
               // Set user ID return
               pData->Get(NULL, iUserID);
               strcpy(*szUserName, szUserEDF);
               bFound = true;
            }
         }
         else
         {
            // Search by ID
            pData->Get(NULL, &iUserEDF);
            if(*iUserID == iUserEDF)
            {
               if(szUserName != NULL)
               {
                  // Set user name return
                  /* if(lDate != -1)
                  {
                     EDFPrint("UserGet historical pre check", pData, false);
                  } */

                  pData->Child("name", EDFElement::LAST);
                  pData->Get(NULL, &szUserEDF);
                  if(lDate != -1)
                  {
                     // Check historical names
                     // printf("UserGet current name %s\n", szUserEDF);

                     bLoop = pData->Prev("name");
                     while(bLoop == true)
                     {
                        lDateEDF = time(NULL);
                        pData->GetChild("date", &lDateEDF);
                        debug("UserGet historical check %ld", lDateEDF);
                        if(lDateEDF >= lDate)
                        {
                           pData->Get(NULL, &szUserEDF);
                           debug(". match %s", szUserEDF);

                           bLoop = pData->Prev("name");
                        }
                        else
                        {
                           bLoop = false;
                        }
                        debug("\n");
                     }

                     // printf("UserGet historical name %s\n", szUserEDF);

                     // EDFPrint("UserGet historial name post check", pData, false);
                  }
                  pData->Parent();

                  /* if(lDate != -1)
                  {
                     EDFPrint("UserGet historial name post check", pData, false);
                  } */

                  *szUserName = strmk(szUserEDF);
               }
               bFound = true;
            }
         }
      }
      if(bFound == false)
      {
         // Move to next child
         bLoop = pData->Next("user");
      }
   }

   if(bReset == true)
   {
		// Reset to marked position
      pData->TempUnmark();
   }

   pData->DataCopy(bGetCopy);

   return bFound;
}

// UserGet: Get user from name (convinence function)
int UserGet(EDF *pData, char *&szUserName, bool bReset)
{
	STACKTRACE
   int iUserID = -1;

   UserGet(pData, &iUserID, &szUserName, 0, bReset, -1);

   return iUserID;
}

// UserGet: Get user from ID (convinence function)
bool UserGet(EDF *pData, int iUserID, char **szUserName, bool bReset, long lDate)
{
	STACKTRACE
   bool bReturn = false;

   bReturn = UserGet(pData, &iUserID, szUserName, 1, bReset, lDate);

   return bReturn;
}
