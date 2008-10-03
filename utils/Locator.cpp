/*
** Removes old location entries
*/

#include <stdio.h>

#include "client/UAClient.h"

class Locator : public UAClient
{
   char *CLIENT_NAME()
   {
      return"Locator";
   }
   
   int checkLocations(EDF *pLocations)
   {
      STACKTRACE
      int iNumLocations = 0, iLocationID = 0;
      bool bLoop = false, bDelete = false;
      long lLastUse = 0, lAgo = 0;
      time_t tNow = time(NULL);
      char *szUnits = NULL;

      bLoop = pLocations->Child("location");
      while(bLoop)
      {
		  // Check the locations inside this one first (i.e. more specific ones)
         iNumLocations += checkLocations(pLocations);

         lLastUse = -1;
         bDelete = false;

         iNumLocations++; 
         pLocations->Get(NULL, &iLocationID);
         pLocations->GetChild("lastuse", &lLastUse);
         if(lLastUse != -1)
         {
            lAgo = tNow - lLastUse;
            if(lAgo > 86400)
            {
               lAgo /= 86400;
               szUnits = "days";

               if(lAgo > 90)
               {
                  bDelete = true;
               }
            }
            else
            {
               lAgo /= 3600;
               szUnits = "hours";
            }
            debug("Location %d last used %ld %s ago\n", iLocationID, lAgo, szUnits);
         }
         else
         {
            debug("Location %d has never been used\n");
            bDelete = true;
         }

         if(bDelete == true)
         {
			 // Don't delete locations with more specific ones inside even if this one has not been used
            if(pLocations->Children("location") == 0)
            {
               debug("Will delete location %d\n", iLocationID);

               EDF *pRequest = new EDF(), *pReply = NULL;
               pRequest->AddChild("locationid", iLocationID);
               if(request(MSG_LOCATION_DELETE, pRequest, &pReply) == true)
               {
                  EDFParser::Print("Location delete reply", pReply);
                  delete pReply;
               }

               delete pRequest;
            }
            else
            {
               debug("Cannot delete location %d because it contains children\n", iLocationID);
            }
         }

         bLoop = pLocations->Next("location");
         if(bLoop == false)
         {
            pLocations->Parent();
         }
      }

      return iNumLocations;
   }

   bool loggedIn()
   {
      STACKTRACE
      int iNumLocations = 0;
      EDF *pReply = NULL;
      
      if(request(MSG_LOCATION_LIST, NULL, &pReply) == true)
      {
         EDFParser::Print("Locations", pReply);

         iNumLocations = checkLocations(pReply);
         debug("Checked %d locations\n", iNumLocations);

         delete pReply;
      }

      m_pData->SetChild("quit", true);
      return true;
   }
};

int main(int argc, char** argv)
{
   Locator *pClient = NULL;

   pClient = new Locator();

   pClient->run(argc, argv);

   delete pClient;

   return 0;
}

