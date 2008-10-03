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
** RqSystem.cpp: Implementation of server side system request functions
*/

// Standard headers
#include <stdio.h>
#include <time.h>
#include <string.h>
#ifdef UNIX
#include <sys/times.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#endif

// Framework headers
#include "headers.h"
#include "ICE/ICE.h"

// Common headers
#include "Folder.h"
#include "Talk.h"

// Library headers
#include "server.h"

#include "RqFolder.h"
#include "RqUser.h"
#include "RqSystem.h"

// Stat lookups
#define STAT_NUMLOGINS 1
#define STAT_TOTALLOGINS 2
#define STAT_AVERAGELOGIN 3
#define STAT_LONGESTLOGIN 4
#define STAT_CONNPER 5
#define STAT_NUMMSGS 6
#define STAT_TOTALMSGS 7
#define STAT_AVERAGEMSG 8

struct system_stat
{
   double m_dValue;
   int m_iUserID;
};

#define MAX_STAT 9

long m_lContactID = 0;

// System statistics
void StatSet(system_stat *pStats, int iStatNum, int iUserID, double dValue)
{
   if(dValue > pStats[iStatNum].m_dValue)
   {
      pStats[iStatNum].m_dValue = dValue;
      pStats[iStatNum].m_iUserID = iUserID;
   }
}

void StatsSet(system_stat *pStats, UserItem *pUser, bool bPeriod, int iStart, int iTimeOn)
{
   int iLongestLogin = 0, iTotalLogins = 0;
   double dConnPer = 0;

   if(iTimeOn > pUser->GetStat(USERITEMSTAT_LONGESTLOGIN, bPeriod))
   {
      iLongestLogin = iTimeOn;
   }
   else
   {
      iLongestLogin = pUser->GetStat(USERITEMSTAT_LONGESTLOGIN, bPeriod);
   }
   iTotalLogins = pUser->GetStat(USERITEMSTAT_TOTALLOGINS, bPeriod) + iTimeOn;

   StatSet(pStats, STAT_NUMLOGINS, pUser->GetID(), pUser->GetStat(USERITEMSTAT_NUMLOGINS, bPeriod));
   StatSet(pStats, STAT_TOTALLOGINS, pUser->GetID(), pUser->GetStat(USERITEMSTAT_TOTALLOGINS, bPeriod));
   if(iStart >= 0)
   {
      dConnPer = (100.0 * iTotalLogins) / (1.0 * (time(NULL) - iStart));
      // debug("StatSet highest conn per %d %s, %d, %d / %d -> %f\n", pUser->GetID(), pUser->GetName(), iStart, iTotalLogins, time(NULL) - iStart, dConnPer);
      StatSet(pStats, STAT_AVERAGELOGIN, pUser->GetID(), dConnPer);
   }
   else
   {
      // debug("StatsSet no start for %d %s\n", pUser->GetID(), pUser->GetName());
   }
   StatSet(pStats, STAT_LONGESTLOGIN, pUser->GetID(), iLongestLogin);

   StatSet(pStats, STAT_NUMMSGS, pUser->GetID(), pUser->GetStat(USERITEMSTAT_NUMMSGS, bPeriod));
   StatSet(pStats, STAT_TOTALMSGS, pUser->GetID(), pUser->GetStat(USERITEMSTAT_TOTALMSGS, bPeriod));
   if(pUser->GetStat(USERITEMSTAT_NUMMSGS, bPeriod) > 0)
   {
      StatSet(pStats, STAT_AVERAGEMSG, pUser->GetID(), (100 * pUser->GetStat(USERITEMSTAT_TOTALMSGS, bPeriod)) / pUser->GetStat(USERITEMSTAT_NUMMSGS, bPeriod));
   }
}

void SystemListStat(EDF *pOut, system_stat *pStats, int iStatNum, char *szStatName, bool bUser)
{
   char *szUserName = NULL;

   if(bUser == true)
   {
      if(UserGet(pStats[iStatNum].m_iUserID, &szUserName) != NULL)
      {
         pOut->Add(szStatName, pStats[iStatNum].m_dValue);
         pOut->AddChild("userid", pStats[iStatNum].m_iUserID);
         pOut->AddChild("username", szUserName);
         pOut->Parent();
      }
   }
   else
   {
      pOut->AddChild(szStatName, pStats[iStatNum].m_dValue);
   }
}

bool SystemListStats(EDF *pOut, char *szName, system_stat *pStats, bool bUser)
{
   pOut->Add(szName);

   SystemListStat(pOut, pStats, STAT_NUMLOGINS, USERITEMSTAT_NUMLOGINS, bUser);
   SystemListStat(pOut, pStats, STAT_TOTALLOGINS, USERITEMSTAT_TOTALLOGINS, bUser);
   SystemListStat(pOut, pStats, STAT_AVERAGELOGIN, "averagelogin", bUser);
   if(bUser == true)
   {
      SystemListStat(pOut, pStats, STAT_LONGESTLOGIN, USERITEMSTAT_LONGESTLOGIN, bUser);
   }
   if(bUser == true)
   {
      SystemListStat(pOut, pStats, STAT_CONNPER, "connper", bUser);
   }

   SystemListStat(pOut, pStats, STAT_NUMMSGS, USERITEMSTAT_NUMMSGS, bUser);
   SystemListStat(pOut, pStats, STAT_TOTALMSGS, USERITEMSTAT_TOTALMSGS, bUser);
   SystemListStat(pOut, pStats, STAT_AVERAGEMSG, "averagemsg", bUser);

   pOut->Parent();

   return true;
}

void LocationSame(EDF *pData, int iPos)
{
   int iID = 0, iDepth = 0, iLoopID = 0, iLoopPos = 0, iUse = -1, iLoopUse = -1, iTotal = 0, iLoopTotal = 0;
   bool bLoop = false;
   char *szName = NULL, *szLoopName = NULL, *szType = NULL, *szValue = NULL;
   EDFElement *pInput = NULL, *pLoop = NULL, *pFrom = NULL, *pTo = NULL, *pCurr = NULL;

   pData->Get(NULL, &iID);
   pData->GetChild("name", &szName);
   pData->GetChild("lastuse", &iUse);
   pData->GetChild("totaluses", &iTotal);
   iDepth = pData->Depth();

   /* if(szName == NULL)
   {
      debugEDFPrint("LocationSame bad current", pData, EDFElement::EL_CURR | EDFElement::PR_SPACE);
   } */

   pInput = pData->GetCurr();

   pData->Root();
   pData->Child("locations");
   bLoop = pData->Child("location");
   while(bLoop == true)
   {
      iLoopPos++;
      szLoopName = NULL;
      iLoopUse = -1;

      pData->Get(NULL, &iLoopID);
      pData->GetChild("name", &szLoopName);

      // printf("LocationSame %s / %d\n", szLoopName, iLoopID);

      if(iLoopID != iID && pData->GetChildBool("delete") == false)
      {
         // printf("LocationName %s / %s\n", szName, szLoopName);
         /* if(szName == NULL || szLoopName == NULL)
         {
            debug("LocationSame bad compare %s / %s\n", szName, szLoopName);
            debugEDFPrint(pData, EDFElement::EL_CURR | EDFElement::PR_SPACE);
         } */

         if(strcmp(szName, szLoopName) == 0)
         {
            debug("LocationSame %d (%s at %d / %d) same as %d (%s at %d / %d)\n", iID, szName, iPos, iDepth, iLoopID, szLoopName, iLoopPos, pData->Depth());

            pData->GetChild("lastuse", &iLoopUse);
            pData->GetChild("totaluses", &iLoopTotal);

            debug("LocationSame usage %d / %d -vs- %d / %d\n", iUse, iTotal, iLoopUse, iLoopTotal);

            pLoop = pData->GetCurr();

            if(iUse != -1 && (iLoopUse == -1 || iUse > iLoopUse))
            {
               debug("  copy loop fields to input (%d -vs- %d)\n", iUse, iLoopUse);

               pFrom = pLoop;
               pTo = pInput;
            }
            else
            {
               debug("  copy input fields to loop (%d -vs- %d)\n", iUse, iLoopUse);

               pFrom = pInput;
               pTo = pLoop;
            }

            pData->SetCurr(pFrom);
            pData->SetChild("delete", true);

            // pSet = new EDF();

            while(pData->Child("hostname") == true || pData->Child("address") == true)
            {
               // pSet->Copy(pData);
               // pData->Delete();

               szValue = NULL;

               pData->Get(&szType, &szValue);

               pData->TempMark();

               pData->SetCurr(pTo);
               if(pData->Child(szType, szValue) == false)
               {
                  pData->AddChild(szType, szValue);
               }

               pData->TempUnmark();

               pData->Delete();

               delete[] szType;
               delete[] szValue;
            }

            // EDFPrint("LocationSame set", pSet);

            pCurr = pData->GetCurr();
            while(pData->Child("location") == true)
            {
               pTo->moveFrom(pData->GetCurr());
               pData->SetCurr(pCurr);
            }

            // printf("LocationSame location_delete request\n");

            /* if(pFrom == pInput)
            {
               debug("LocationSame delete outside\n");
               bReturn = true;
            }
            else
            {
               debug("LocationSame delete inside\n");
               pData->Delete();
               // pInput = pData->GetCurr();
            } */

            // EDFPrint("LocationSame post-delete from", pData, EDFElement::EL_CURR | EDFElement::PR_SPACE);

            pData->SetCurr(pTo);

            // EDFPrint("LocationSame pre-set to", pData, EDFElement::EL_CURR | EDFElement::PR_SPACE);

            // EDFCopyUnique(pData, pSet, "hostname");
            // EDFCopyUnique(pData, pSet, "address");

            /* while(pSet->Child("location") == true)
            {
               // EDFPrint("LocationSame copying set child", pSet, EDFElement::EL_CURR | EDFElement::PR_SPACE);

               pData->Copy(pSet);
               pSet->Delete();
            } */

            // delete pSet;

            if(iTotal + iLoopTotal > 0)
            {
               // printf("LocationSame uses %d + %d\n", iTotal, iLoopTotal);
               pData->SetChild("totaluses", iTotal + iLoopTotal);
            }

            // EDFPrint("LocationSame combined", pData, EDFElement::EL_CURR | EDFElement::PR_SPACE);

            // printf("LocationSame set loop %p\n", pLoop);

            /* if(pFrom != pLoop)
            {
               pData->SetCurr(pLoop);
            } */

            bLoop = false;

            // printf("LocationSame check 1\n");
         }
      }

      delete[] szLoopName;

      if(bLoop == true)
      {
         bLoop = pData->Iterate("location", "locations");
      }

      // printf("LocationSame check 2\n");
   }

   delete[] szName;

   pData->SetCurr(pInput);

   // printf("LocationSame check 3\n");

   /* if(bReturn == true)
   {
      debug("LocationSame true\n");
   } */
   // return bReturn;
}

bool LocationBetter(EDF *pData, int iID, char *szType, char *szValue)
{
   STACKTRACE
   int iLoopID = -1;
   unsigned long lMin = 0, lMax = 0, lValue = 0;
   bool bLoop = false, bMatch = false, bHostname = false;//, bPrint = false;
   char *szLoopValue = NULL;
   EDFElement *pTo = NULL, *pFrom = NULL;

   if(stricmp(szType, "address") == 0)
   {
      if(Conn::StringToAddress(szValue, &lValue, false) == false)
      {
         return false;
      }
   }

   pFrom = pData->GetCurr();

   debug("LocationBetter entry %s %s(%lu)\n", szType, szValue, lValue);

   // Try to match hostname / address with a location
   pData->Root();
   pData->Child("locations");
   bLoop = pData->Child("location");
   while(bLoop == true)
   {
      bMatch = false;

      pData->Get(NULL, &iLoopID);

      /* if(bPrint == true)
      {
         debug("LocationBetter %s %d\n", pData->GetCurr()->getName(false), iLoopID);
      } */

      if(iLoopID != iID && pData->GetChildBool("delete") == false)
      {
         // printf("LocationBetter %s %d\n", pData->GetCurr()->getName(false), iLoopID);

         bHostname = pData->Child(szType);
         while(bHostname == true && bLoop == true)
         {
            szLoopValue = NULL;

            pData->Get(NULL, &szLoopValue);
            // printf("LocationBetter check with location %d, pattern %s\n", iLoopID, szLoopValue);
            if(stricmp(szType, "hostname") == 0)
            {
               if(strmatch(szValue, szLoopValue, true, true, false) == true)
               {
                  debug("LocationBetter matched %s with hostname %s\n", szValue, szLoopValue);

                  pData->Parent();
                  pTo = pData->GetCurr();

                  bMatch = true;
               }
            }
            else if(stricmp(szType, "address") == 0)
            {
               if(Conn::CIDRToRange(szLoopValue, &lMin, &lMax) == true && lMin <= lValue && lValue <= lMax)
               {
                  debug("LocationBetter matched %s with address %s\n", szValue, szLoopValue);

                  pData->Parent();
                  pTo = pData->GetCurr();

                  bMatch = true;
               }
            }

            delete[] szLoopValue;

            if(bMatch == true)
            {
               bLoop = pData->Child("location");
               // printf("LocationBetter child location %s\n", BoolStr(bLoop));
               if(bLoop == true)
               {
                  // EDFPrint(pData, EDFElement::EL_CURR | EDFElement::PR_SPACE);
                  /* bPrint = true;
                  debug("LocationBetter print on\n"); */

                  bHostname = false;
               }
            }
            else
            {
               bHostname = pData->Next(szType);
               if(bHostname == false)
               {
                  pData->Parent();
               }
            }
         }
      }

      if(bMatch == false)
      {
         bLoop = pData->Next("location");
         /* if(bPrint == true)
         {
            // printf("LocationBetter next location %s\n", BoolStr(bLoop));
            if(bLoop == true)
            {
               debugEDFPrint(pData, EDFElement::EL_CURR | EDFElement::PR_SPACE);
            }
         } */
      }
   }

   pData->SetCurr(pFrom);

   if(pTo != NULL)
   {
      /* pParent = pTo;
      debug("LocationBetter match");
      while(pParent != NULL)
      {
         debug(" %p", pParent);
         pParent = pParent->parent();
      }
      pParent = pFrom;
      debug(", input");
      while(pParent != NULL)
      {
         debug(" %p", pParent);
         pParent = pParent->parent();
      }
      debug("\n"); */

      if(pTo != pFrom->parent()->parent())
      {
         /* pData->Parent();
         pData->Parent();

         // pData->SetCurr(pTo);

         // pTo->print("LocationBetter move to");

         pTo->moveFrom(pFrom->parent()); */

         // (pFrom->parent())->print("LocationBetter copy element");
         // pTo->print("LocationBetter best match");

         pTo->copy(pFrom->parent());
         bMatch = true;
      }
      else
      {
         debug("LocationBetter already at best match\n");
         bMatch = false;
      }
   }

   // bMatch = false;

   // printf("LocationHostame exit %s\n", BoolStr(bMatch));
   return bMatch;
}

int LocationCheck(EDF *pData, int iPos, bool bThis)
{
   int iID = -1;
   bool bLoop = false;
   char *szName = NULL, *szType = NULL, *szValue = NULL;

   // printf("LocationCheck entry %d, %d\n", iPos, pData->Depth());
   // EDFPrint(pData, EDFElement::EL_CURR | EDFElement::PR_SPACE);

   bLoop = pData->Child("location");
   while(bLoop == true)
   {
      iPos = LocationCheck(pData, iPos, true);

      bLoop = pData->Next("location");
      if(bLoop == false)
      {
         pData->Parent();
      }
   }

   // printf("LocationCheck this %s\n", BoolStr(bThis));

   if(bThis == true && pData->GetChildBool("delete") == false)
   {
      iPos++;

      iID = -1;
      szName = NULL;

      pData->Get(NULL, &iID);
      pData->GetChild("name", &szName);

      debug("LocationCheck pos %d, %d (%s)\n", iPos, iID, szName);
      // EDFPrint(pData, EDFElement::EL_CURR | EDFElement::PR_SPACE);

      LocationSame(pData, iPos);

      bLoop = pData->Child();
      while(bLoop == true)
      {
         szValue = NULL;

         pData->Get(&szType, &szValue);

         // printf("LocationCheck element %s / %s\n", szType, szValue);

         if(stricmp(szType, "hostname") == 0 || stricmp(szType, "address") == 0)
         {
            if(LocationBetter(pData, iID, szType, szValue) == true)
            {
               pData->Parent();
               pData->SetChild("delete", true);

               bLoop = false;
            }
         }

         if(bLoop == true)
         {
            bLoop = pData->Next();
            if(bLoop == false)
            {
               pData->Parent();
            }
         }
      }
   }

   // printf("LocationCheck exit %d\n", iPos);
   return iPos;
}

bool LocationFix(EDF *pData)
{
   int iTemp = 0;
   bool bLoop = false;
   char *szName = NULL;

   bLoop = pData->Child("location");
   while(bLoop == true)
   {
      iTemp = pData->Position(true);
      if(LocationFix(pData) == true)
      {
         bLoop = pData->Child("location", iTemp);
      }
      else
      {
         bLoop = pData->Next("location");
         if(bLoop == false)
         {
            pData->Parent();
         }
      }
   }

   if(pData->GetChildBool("delete") == true)
   {
      EDFParser::debugPrint("LocationDelete delete element", pData, EDFElement::EL_CURR | EDFElement::PR_SPACE);

      pData->Delete();
      return true;
   }

   if(pData->GetChild("name", &szName) == true)
   {
      pData->DeleteChild("name");
      pData->AddChild("name", szName, EDFElement::ABSFIRST);
      delete[] szName;
   }

   return false;
}

// TaskNext: Calculate time to next run of task
long TaskNext(int iHour, int iMinute, int iDay, int iRepeat)
{
   int iLoop = 0;
   bool bLoop = true;
   time_t lNext = time(NULL);
   struct tm *tmNext = localtime(&lNext);

   debug("TaskNext entry %d %d %d %d, %ld (%d %d %d)\n", iHour, iMinute, iDay, iRepeat, lNext, tmNext->tm_hour, tmNext->tm_min, tmNext->tm_sec);

   lNext -= tmNext->tm_sec;
   lNext -= 60 * (tmNext->tm_min - iMinute);
   lNext -= 3600 * (tmNext->tm_hour - iHour);

   iLoop = tmNext->tm_wday;

   debug("TaskNext pre-loop %ld %d\n", lNext, iLoop);

   while(bLoop == true)
   {
      debug("TaskNext loop %ld %d, %ld\n", lNext, iLoop, time(NULL));

      if(lNext >= time(NULL))
      {
         if(iRepeat == TASK_WEEKLY && iLoop == iDay)
         {
            bLoop = false;
         }
         else if(iRepeat == TASK_WEEKDAY && iLoop >= 0 && iLoop <= 4)
         {
            bLoop = false;
         }
         else if(iRepeat == TASK_WEEKEND && (iLoop == 5 || iLoop == 6))
         {
            bLoop = false;
         }
         else if(iRepeat == TASK_DAILY)
         {
            bLoop = false;
         }
      }

      if(bLoop == true)
      {
         // Add another day
         lNext += 86400;
         iLoop++;
         if(iLoop == 7)
         {
            iLoop = 0;
         }
      }
   }

   debug("TaskNext exit %ld\n", lNext);
   return lNext;
}

bool AgentLogout(EDF *pData, UserItem *pUser)
{
   STACKTRACE
   int iServiceID = -1, iProviderID = -1, iListNum = 0;
   bool bLoop = false;
   char *szServiceName = NULL;
   EDF *pAnnounce = NULL;
   EDFConn *pListConn = NULL;
   ConnData *pListData = NULL;
   DBSubData *pServiceData = NULL;

   debug("AgentLogout entry %p %ld\n", pData, pUser->GetID());

   pData->Root();
   pData->Child("services");

   bLoop = pData->Child("service");
   while(bLoop == true)
   {
      iProviderID = -1;

      pData->Get(NULL, &iServiceID);
      pData->GetChild("providerid", &iProviderID);
      pData->GetChild("name", &szServiceName);

      pAnnounce = new EDF();
      pAnnounce->AddChild("serviceid", iServiceID);
      pAnnounce->AddChild("servicename", szServiceName);
      pAnnounce->AddChild("serviceaction", ACTION_LOGOUT);

      if(iProviderID == pUser->GetID())
      {
         debug("AgentLogout deactivate service %d\n", iServiceID);

         for(iListNum = 0; iListNum < ConnCount(); iListNum++)
         {
            pListConn = ConnList(iListNum);
            pListData = (ConnData *)pListConn->Data();
            if(pListData != NULL && pListData->m_pServices != NULL)
            {
               pServiceData = pListData->m_pServices->SubData(iServiceID);
               if(pServiceData != NULL && pServiceData->m_bTemp == true)
               {
                  // logout active services
                  debug("  user %ld\n", pListData->m_pUser->GetID());

                  ServerAnnounce(pData, MSG_USER_PAGE, pAnnounce, NULL, pListConn);
               }
            }
         }
      }

      delete pAnnounce;

      delete[] szServiceName;

      bLoop = pData->Next("service");
   }

   debug("AgentLogout exit true\n");
   return true;
}

int SystemExpire(EDF *pData)
{
   int iReturn = -1;

   pData->Root();
   pData->Child("system");
   pData->GetChild("expire", &iReturn);

   return iReturn;
}

void ContactID(long lContactID)
{
   m_lContactID = lContactID;
}

long ContactID()
{
   return m_lContactID;
}

char *SystemContactStr(int iBase, char *szStr)
{
   char *szReturn = NULL;

   switch(iBase)
   {
      case RFG_USER:
         if(stricmp(szStr, "invalid") == 0)
         {
            szReturn = MSG_USER_CONTACT_INVALID;
         }
         break;

      case RFG_SYSTEM:
         if(stricmp(szStr, "invalid") == 0)
         {
            szReturn = MSG_SYSTEM_CONTACT_INVALID;
         }
         break;
   }

   debug("SystemContactStr %d %s -> %s\n", iBase, szStr, szReturn);
   return szReturn;
}

bool SystemContactFind(EDFConn *pConn, EDF *pData, EDF *pIn, char *szRequest, int iBase, bool bDivert, int iToID, char *szTo, long lContactID, bool bOverride, int iFromID, int iUserType, int iAccessLevel, EDF *pOut, EDFConn **pReturn, char **szUser)
{
   STACKTRACE
   int iStatus = LOGIN_OFF, iFolderID = -1, iReplyID = -1;
   bool bLoop = false, bReturn = false;
   UserItem *pContact = NULL;
   EDFConn *pContactConn = NULL;
   ConnData *pConnData = CONNDATA, *pContactData = NULL;
   EDF *pTempIn = NULL;
   MessageItem *pReply = NULL;
   MessageTreeItem *pFolder = NULL;

   pContact = UserGet(iToID, szUser);
   if(pContact == NULL)
   {
      // No such user
      pOut->Set("reply", MSG_USER_NOT_EXIST);
      pOut->AddChild("userid", iToID);

      debug("SystemContactFind exit false, %s %d\n", MSG_USER_NOT_EXIST, iToID);
      return false;
   }

   STACKTRACEUPDATE

   // Do a primary connection check first
   pContactConn = ConnectionFind(CF_USERPRI, iToID);
   if(pContactConn == NULL)
   {
      // Do a user ID check now
      pContactConn = ConnectionFind(CF_USERID, iToID);
   }
   if(pContactConn != NULL)
   {
      pContactData = (ConnData *)pContactConn->Data();
      iStatus = pContactData->m_iStatus;
      // if(iAccessLevel < LEVEL_WITNESS && (pContact->GetOwnerID() == -1 || pContact->GetOwnerID() != iFromID) && mask(iStatus, LOGIN_SILENT) == true)
      if(ConnectionVisible(pContactData, iFromID, iAccessLevel) == false)
      {
         debug("SystemContactFind silent login flagged as off\n");
         iStatus = LOGIN_OFF;
      }
   }

   if(bDivert == false) // && mask(iStatus, LOGIN_NOCONTACT) == false)
   {
      if(mask(iStatus, LOGIN_ON) == false)
      {
         // Not logged in
         pOut->Set("reply", MSG_USER_NOT_ON);
         pOut->AddChild("userid", iToID);
         pOut->AddChild("username", *szUser);
         if(lContactID != -1)
         {
            pOut->AddChild("contactid", lContactID);
         }

         debug("SystemContactFind exit false, %s\n", MSG_USER_NOT_ON);
         return false;
      }

      if(mask(pContactData->m_iStatus, LOGIN_SHADOW) == true)
      {
         pOut->Set("reply", MSG_USER_CONTACT_NOCONTACT);
         pOut->AddChild("userid", iToID);
         pOut->AddChild("username", *szUser);
         if(lContactID != -1)
         {
            pOut->AddChild("contactid", lContactID);
         }

         debug("SystemContactFind exit false, %s\n", MSG_USER_CONTACT_NOCONTACT);
         return false;
      }

      if(mask(iStatus, LOGIN_BUSY) == true &&
         (iAccessLevel < LEVEL_WITNESS ||
         (iAccessLevel >= LEVEL_WITNESS && bOverride == false)))
      {
         // User is busy and override is off
         pOut->Set("reply", MSG_USER_BUSY);
         pOut->AddChild("userid", iToID);
         pOut->AddChild("username", *szUser);
         pOut->AddChild("status", pContactData->m_iStatus);
         if(pContactData->m_pStatusMsg != NULL)
         {
            pOut->AddChild(ConnVersion(pConnData, "2.5") >= 0 ? "statusmsg" : "busymsg", pContactData->m_pStatusMsg);
         }
         if(mask(pContactData->m_iStatus, LOGIN_BUSY) == true)
         {
            pOut->AddChild("timebusy", pContactData->m_lTimeBusy);
         }
         if(lContactID != -1)
         {
            pOut->AddChild("contactid", lContactID);
         }

         debug("SystemContactFind exit false, %s\n", MSG_USER_BUSY);
         return false;
      }
   }
   else if(bDivert == true && (mask(iUserType, USERTYPE_AGENT) == true || iAccessLevel >= LEVEL_MESSAGES))
   {
      pData->Root();
      pData->Child("system");
      pData->GetChild("contactfolder", &iFolderID);

      debug("SystemContactFind contact folder %d\n", iFolderID);

      pFolder = FolderGet(iFolderID);
      if(pFolder == NULL)
      {
         // Invalid request
         pOut->Set("reply", SystemContactStr(iBase, "invalid"));
         // pOut->AddChild("contacttype", iContactType);

         debug("SystemContactFind exit false, %s\n", MSG_RQ_INVALID);
         return false;
      }

      pIn->GetChild("replyid", &iReplyID);
      debug("SystemContactFind input reply ID %d\n", iReplyID);

      pTempIn = new EDF();
      pTempIn->Set("request", MSG_MESSAGE_ADD);
      pReply = FolderMessageGet(iReplyID);
      if(pReply != NULL && pReply->GetTreeID() == iFolderID)
      {
         pTempIn->AddChild("replyid", iReplyID);
      }
      else
      {
         pTempIn->AddChild("folderid", iFolderID);
      }
      pTempIn->AddChild("toid", iToID);
      pTempIn->AddChild(pIn, "fromname");
      pTempIn->AddChild(pIn, "subject");
      pTempIn->AddChild(pIn, "text");
      pTempIn->AddChild(pIn, "markread");
      bLoop = pIn->Child("attachment");
      while(bLoop == true)
      {
         pTempIn->Copy(pIn);

         bLoop = pIn->Next("attachment");
         if(bLoop == false)
         {
            pIn->Parent();
         }
      }

      STACKTRACEUPDATE

      // EDFPrint("SystemContactFind post", pTempIn);
      // pData->SetCurr(CONNID);
      bReturn = MessageAdd(pConn, pData, pTempIn, pOut);

      delete pTempIn;

      if(bReturn == true)
      {
         pOut->Set("reply", szRequest);
      }

      STACKTRACEUPDATE

      // printf("SystemContactFind exit %s\n", BoolStr(bReturn));
      // return bReturn;

      debug("SystemContactFind exit false, CONTACT_POST %s\n", BoolStr(bReturn));
      return false;
   }
   else
   {
      // Invalid request
      pOut->Set("reply", SystemContactStr(iBase, "invalid"));
      // pOut->AddChild("contacttype", iContactType);

      debug("SystemContactFind exit false, %s\n", MSG_RQ_INVALID);
      return false;
   }

   *pReturn = pContactConn;

   return true;
}

extern "C"
{

/*
** SystemEdit: Edit properties of the system
*/
ICELIBFN bool SystemEdit(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   STACKTRACE
   int iValue = 0;

   pData->Root();
   pData->Child("system");

   pData->SetChild(pIn, "banner");
   if(pIn->GetChild("uptime", &iValue) == true)
   {
      if(iValue > time(NULL))
      {
         iValue = time(NULL);
      }
      pData->SetChild("uptime", iValue);
   }

   if(pIn->GetChild("inittime", &iValue) == true)
   {
      if(iValue > time(NULL))
      {
         iValue = time(NULL);
      }
      pData->SetChild("inittime", iValue);
   }

   pData->SetChild(pIn, "newuser");

   if(pIn->GetChild("idletime", &iValue) == true)
   {
      if(iValue < 0)
      {
         iValue = 0;
      }

      if(iValue == 0)
      {
         pData->DeleteChild("idletime");
         UserItem::SetSystemTimeIdle(-1);
      }
      else
      {
         pData->SetChild("idletime", iValue);
         UserItem::SetSystemTimeIdle(iValue);
      }
   }

   /* if(pIn->GetChild("gonetime", &iValue) == true)
   {
      if(iValue < 0)
      {
         iValue = 0;
      }

      if(iValue == 0)
      {
         pData->DeleteChild("gonetime");
      }
      else
      {
         pData->SetChild("gonetime", iValue);
      }
   } */

   pData->SetChild(pIn, "contactfolder");
   pData->SetChild(pIn, "expire");

   pData->SetChild(pIn, "attachmentsize");

#ifdef UNIX
   pData->SetChild(pIn, "mimetypes");
#endif

   if(pIn->Child("connection") == true)
   {
      // Connections
      // EDFPrint("SystemEdit login", pIn, false);

      if(pData->Child("connection") == false)
      {
         pData->Add("connection");
      }

      EDFParser::debugPrint("SystemEdit pre-section connection", pData, EDFElement::EL_CURR | EDFElement::PR_SPACE);

      EDFSection(pData, pIn, "allow", RQ_ADD, "hostname", EDFElement::BYTES | EDFSECTION_NOTNULL | EDFSECTION_NOTEMPTY | EDFSECTION_MULTI, "address", EDFElement::BYTES | EDFSECTION_NOTNULL | EDFSECTION_NOTEMPTY | EDFSECTION_MULTI, NULL);
      EDFSection(pData, pIn, "allow", RQ_DELETE, "hostname", EDFElement::BYTES, "address", EDFElement::BYTES, NULL);
      EDFSection(pData, pIn, "deny", RQ_ADD, "hostname", EDFElement::BYTES | EDFSECTION_NOTNULL | EDFSECTION_NOTEMPTY | EDFSECTION_MULTI, "address", EDFElement::BYTES | EDFSECTION_NOTNULL | EDFSECTION_NOTEMPTY | EDFSECTION_MULTI, NULL);
      EDFSection(pData, pIn, "deny", RQ_DELETE, "hostname", EDFElement::BYTES, "address", EDFElement::BYTES, NULL);
      EDFSection(pData, pIn, "trust", RQ_ADD, "hostname", EDFElement::BYTES | EDFSECTION_NOTNULL | EDFSECTION_NOTEMPTY | EDFSECTION_MULTI, "address", EDFElement::BYTES | EDFSECTION_NOTNULL | EDFSECTION_NOTEMPTY | EDFSECTION_MULTI, NULL);
      EDFSection(pData, pIn, "trust", RQ_DELETE, "hostname", EDFElement::BYTES, "address", EDFElement::BYTES, NULL);

      EDFParser::debugPrint("SystemEdit post-section connection", pData, EDFElement::EL_CURR | EDFElement::PR_SPACE);

      pData->Parent();

      pIn->Parent();
   }

   if(pIn->Child("defaultsubs") == true)
   {
      // DefaultSubsEdit(pData, pIn, "add");
      EDFSection(pData, pIn, RQ_ADD, "folderid", EDFElement::INT | EDFSECTION_GTZERO);
      // DefaultSubsEdit(pData, pIn, "delete");
      EDFSection(pData, pIn, RQ_DELETE, "folderid", EDFElement::INT);

      pIn->Parent();
   }

   return true;
}

/*
** SystemList: Generate a list of system attributes
*/
ICELIBFN bool SystemList(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   STACKTRACE
   int iCurrID = 0, iAccessLevel = LEVEL_NONE, iStatNum = 0, iInitTime = 0, iNumChildren = 0, iChildNum = 0, iValue = 0, iFolderID = 0;
   int iUserNum = 0, iStart = 0, iTimeOn = 0, iTotalCreate = 0; //, iTotalStart = 0;
   bool bLoop = false;
   char *szName = NULL;
   system_stat pTotals[MAX_STAT], pPeriod[MAX_STAT], pRecords[MAX_STAT];
   UserItem *pCurr = CONNUSER, *pUser = NULL;
   EDFConn *pUserConn = NULL;
   ConnData *pUserData = NULL;

   // printf("SystemList entry\n");

   for(iStatNum = 0; iStatNum < MAX_STAT; iStatNum++)
   {
      pTotals[iStatNum].m_dValue = 0;
      pTotals[iStatNum].m_iUserID = 0;

      pPeriod[iStatNum].m_dValue = 0;
      pPeriod[iStatNum].m_iUserID = 0;

      pRecords[iStatNum].m_dValue = 0;
      pRecords[iStatNum].m_iUserID = 0;
   }

   // pData->GetChild("accesslevel", &iAccessLevel);
   if(pCurr != NULL)
   {
      iCurrID = pCurr->GetID();
      iAccessLevel = pCurr->GetAccessLevel();
   }

   pData->Root();
   pData->Child("system");

   pData->GetChild("inittime", &iInitTime);
   debug("SystemList inittime %d\n", iInitTime);

   pOut->AddChild(pData, "name");
   pOut->AddChild(pData, "version"); // MessageSpec STR
   pOut->AddChild(pData, "protocol"); // MessageSpec STR
   pOut->AddChild(pData, "buildnum"); // MessageSpec INT
   pOut->AddChild(pData, "builddate"); // MessageSpec STR
   pOut->AddChild(pData, "buildtime");
   pOut->AddChild(pData, "banner"); // MessageSpec STR
   pOut->AddChild(pData, "newuser"); // MessageSpec STR
   pOut->AddChild("systemtime", (int)(time(NULL))); // MessageSpec INT
   pOut->AddChild(pData, "uptime"); // MessageSpec INT
   pOut->AddChild(pData, "inittime"); // MessageSpec INT

   pOut->AddChild(pData, "expire"); // MessageSpec INT

   if(iAccessLevel >= LEVEL_WITNESS)
   {
      pOut->AddChild(pData, "contactfolder"); // MessageSpec INT

      pOut->AddChild(pData, "attachmentsize"); // MessageSpec INT

#ifdef UNIX
      pOut->AddChild(pData, "mimetypes");
#endif

      pOut->AddChild("numtasks", pData->Children("task")); // MessageSpec INT

      pOut->AddChild(pData, "maxconns"); // MessageSpec INT

      pOut->AddChild(pData, "downtime"); // MessageSpec INT
      // pOut->Copy(pData, "reloadtime");
      iNumChildren = pData->Children("reloadtime"); // MessageSpec INT
      for(iChildNum = 0; iChildNum < iNumChildren; iChildNum++)
      {
         pData->GetChild("reloadtime", &iValue, iChildNum);
         pOut->AddChild("reloadtime", iValue);
      }
      pOut->AddChild(pData, "idletime"); // MessageSpec INT
      // pOut->AddChild(pData, "gonetime");
      pOut->AddChild(pData, "mainttime"); // MessageSpec INT
      pOut->AddChild(pData, "writetime"); // MessageSpec INT

#ifdef UNIX
      // CPU usage
      // char szFloat[64];
      struct tms tmsBuffer;
      times(&tmsBuffer);
      /* sprintf(szFloat, "%0.2f", (1.0 * tmsBuffer.tms_utime) / sysconf(_SC_CLK_TCK));
      pOut->AddChild("usercputime", szFloat);
      sprintf(szFloat, "%0.2f", (1.0 * tmsBuffer.tms_stime) / sysconf(_SC_CLK_TCK));
      pOut->AddChild("syscputime", szFloat); */
      pOut->AddChild("usercputime", (1.0 * tmsBuffer.tms_utime) / sysconf(_SC_CLK_TCK)); // MessageSpec INT
      pOut->AddChild("syscputime", (1.0 * tmsBuffer.tms_stime) / sysconf(_SC_CLK_TCK)); // MessageSpec INT

      // Resource usage
      struct rusage rBuffer;
      getrusage(RUSAGE_SELF, &rBuffer);

      pOut->Add("cpuusage");
      pOut->AddChild("maxrss", rBuffer.ru_maxrss); // MessageSpec INT
      pOut->AddChild("ixrss", rBuffer.ru_ixrss); // MessageSpec INT
      pOut->AddChild("idrss", rBuffer.ru_idrss); // MessageSpec INT
      pOut->AddChild("isrss", rBuffer.ru_isrss); // MessageSpec INT

      pOut->AddChild("minflt", rBuffer.ru_minflt); // MessageSpec INT
      pOut->AddChild("majflt", rBuffer.ru_minflt); // MessageSpec INT
      pOut->AddChild("nswap", rBuffer.ru_minflt); // MessageSpec INT
      pOut->Parent();
#endif

      pOut->AddChild("memusage", (double)memusage()); // MessageSpec INT

      // pOut->AddChild("memlocs", memlocs());
      // pOut->AddChild("memtotal", memtotal());

      pOut->AddChild("connections", ConnCount()); // MessageSpec INT
      // printf("SystemList bytes sent %d, recv %d\n", pConn->Sent(), pConn->Received());
      pOut->AddChild("sent", ConnSent()); // MessageSpec INT
      pOut->AddChild("received", ConnReceived()); // MessageSpec INT

      pOut->AddChild(pData, "requests"); // MessageSpec INT
      pOut->AddChild(pData, "announces"); // MessageSpec INT

      if(pData->Child("connection") == true)
      {
         pOut->Copy(pData);

         pData->Parent();
      }
   }

   if(pData->Child("defaultsubs") == true)
   {
      EDFParser::debugPrint("SystemList default subs", pData, false);

      pOut->Add("defaultsubs");
      bLoop = pData->Child("folderid");
      while(bLoop == true)
      {
         pData->Get(NULL, &iFolderID);
         if(FolderGet(iFolderID, &szName) != NULL)
         {
            pOut->Add("folder", iFolderID);
            pOut->AddChild("name", szName);
            pOut->Parent();
         }

         bLoop = pData->Next("folderid");
         if(bLoop == false)
         {
            pData->Parent();
         }
      }
      pOut->Parent();

      pData->Parent();
   }

   pData->Root();
   pData->Child("users");
   // pOut->AddChild(pData, "numusers");
   pOut->AddChild("numusers", UserCount()); // MessageSpec INT
   pOut->AddChild("maxuserid", UserMaxID()); // MessageSpec INT
   if(iAccessLevel >= LEVEL_WITNESS)
   {
      if(pData->Child("maxlogins") == true)
      {
         pData->Get(NULL, &iValue);

         pOut->Add("maxlogins", iValue); // MessageSpec INT
         pOut->AddChild(pData, "date"); // MessageSpec INT
         pOut->Parent();

         pData->Parent();
      }
   }

   /* bLoop = pData->Child("user");
   while(bLoop == true) */
   for(iUserNum = 0; iUserNum < UserCount(); iUserNum++)
   {
      pUser = UserList(iUserNum);

      iStart = iInitTime;
      iTimeOn = 0;

      pUserConn = ConnectionFind(CF_USERPRI, pUser->GetID());
      if(pUserConn != NULL)
      {
         pUserData = (ConnData *)pUserConn->Data();

         if(mask(pUserData->m_iStatus, LOGIN_ON) == true)
         {
            iTimeOn = time(NULL) - pUserData->m_lTimeOn;

            pTotals[STAT_NUMLOGINS].m_dValue++;
            pTotals[STAT_TOTALLOGINS].m_dValue++;
         }
      }

      pTotals[STAT_NUMLOGINS].m_dValue += pUser->GetStat(USERITEMSTAT_NUMLOGINS, false);
      pTotals[STAT_TOTALLOGINS].m_dValue += pUser->GetStat(USERITEMSTAT_TOTALLOGINS, false);

      pTotals[STAT_NUMMSGS].m_dValue += pUser->GetStat(USERITEMSTAT_NUMMSGS, false);
      pTotals[STAT_TOTALMSGS].m_dValue += pUser->GetStat(USERITEMSTAT_TOTALMSGS, false);

      if(pUser->GetCreated() > iStart)
      {
         iStart = pUser->GetCreated();
      }

      if(iStart >= 0)
      {
         iTotalCreate += time(NULL) - iStart;
      }

      StatsSet(pRecords, pUser, false, iStart, iTimeOn);
      StatsSet(pPeriod, pUser, true, iStart, iTimeOn);
   }

   if(pTotals[STAT_NUMLOGINS].m_dValue > 0)
   {
      pTotals[STAT_AVERAGELOGIN].m_dValue = pTotals[STAT_TOTALLOGINS].m_dValue / pTotals[STAT_NUMLOGINS].m_dValue;
   }
   if(iTotalCreate > 0)
   {
      pTotals[STAT_CONNPER].m_dValue = (100 * pTotals[STAT_TOTALLOGINS].m_dValue) / iTotalCreate;
   }
   if(pTotals[STAT_NUMMSGS].m_dValue > 0)
   {
      pTotals[STAT_AVERAGEMSG].m_dValue = pTotals[STAT_TOTALMSGS].m_dValue / pTotals[STAT_NUMMSGS].m_dValue;
   }

   SystemListStats(pOut, "totalstats", pTotals, false);

   if(iAccessLevel >= LEVEL_WITNESS)
   {
      SystemListStats(pOut, "recordstats", pRecords, true);
      SystemListStats(pOut, "periodstats", pPeriod, true);
   }

   // pData->Root();
   // pData->Child("folders");
   // pOut->AddChild(pData, "numfolders");
   // pOut->AddChild(pData, "maxid", "maxfoldermsgid");
   pOut->AddChild("numfolders", FolderCount()); // MessageSpec INT
   pOut->AddChild("maxfoldermsgid", FolderMaxID()); // MessageSpec INT

   /* pData->Root();
   pData->Child("channels"); */
   pOut->AddChild("numchannels", ChannelCount(false)); // MessageSpec INT
   pOut->AddChild("maxtalkmsgid", TalkMaxID()); // MessageSpec INT

   // EDFPrint("SystemList exit true", pOut);

   // printf("SystemList exit true\n");
   return true;
}

/*
** SystemWrite: Trigger a write of the EDF data
*/
ICELIBFN bool SystemWrite(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   STACKTRACE

   pData->Root();
   pData->Child("system");
   pData->AddChild("write", true);

   return true;
}

/*
** SystemShutdown: Shutdown the server (logout everyone out)
*/
ICELIBFN bool SystemShutdown(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   STACKTRACE
   int iInterval = 0;
   EDF *pAnnounce = NULL;
   UserItem *pCurr = CONNUSER;

   pIn->GetChild("interval", &iInterval);
   if(iInterval < 0)
   {
      iInterval = 0;
   }

   pData->Root();
   pData->Child("system");
   pData->SetChild("quit", (int)(time(NULL) + iInterval));

   pAnnounce = new EDF();
   pAnnounce->AddChild("fromid", pCurr->GetID());
   pAnnounce->AddChild("fromname", pCurr->GetName());
   pAnnounce->AddChild("interval", iInterval);
   pAnnounce->AddChild(pIn, "text");

   ServerAnnounce(pData, MSG_SYSTEM_SHUTDOWN, NULL, pAnnounce);
   delete pAnnounce;

   return true;
}

/*
** SystemMessage: Send a system message
**
** text:    Text of message
*/
ICELIBFN bool SystemMessage(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   STACKTRACE
   int iFromID = 0;
   char *szFrom = NULL;
   EDF *pAnnounce = NULL;
   UserItem *pCurr = CONNUSER;

   /* pData->Get(NULL, &iFromID);
   pData->GetChild("name", &szFrom); */
   iFromID = pCurr->GetID();
   szFrom = pCurr->GetName();

   pAnnounce = new EDF();
   pAnnounce->AddChild("fromid", iFromID);
   pAnnounce->AddChild("fromname", szFrom);
   pAnnounce->AddChild(pIn, "text");
   ServerAnnounce(pData, MSG_SYSTEM_MESSAGE, pAnnounce);

   delete pAnnounce;

   // delete[] szFrom;

   return true;
}

/*
** SystemContact: Send a message to a user using a variety of methods
**
** toid:       ID of user to send the message to
** contactid:  ID of conversation thread
** divert:     Divert message to folder (supercedes CONTACT_POST type)
** override:   Ignore LOGIN_BUSY flag and send message (admin only)
** fromname:   From user in preference to real one (agents only)
** serviceid:  ID of service to use for contact
** toname:     String identifier of use (service mode only)
** content-type:  MIME type of data (system_contact only)
** data:       Type dependent section (system_contact only)
** username:   Authentication for service (service mode only)
** password:   Authentication for service (service mode only)
**
** This function is called for user_contact and system_contact
** serviceid can be used in user_contact (any user) and must be used in
** system_contact (only the provider agent listed can make this request)
** toid can be used multiple times in system_contact but is ignored in
** user_contact if the requesting user is not the service provider
*/
ICELIBFN bool SystemContact(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   STACKTRACE
   int iFromID = -1, iAccessLevel = LEVEL_NONE, iUserType = USERTYPE_NONE, iToID = -1, iBase = 0, iServiceID = -1, iProviderID = -1, iCurrID = -1;
   int iServiceType = 0, iContactType = 0, iExtra = 0, iSubType = 0;
   long lContactID = -1;
   bool bDivert = false, bOverride = false, bPrivilege = false, bMultiple = false, bLoop = false;
   char *szFrom = NULL, *szRequest = NULL, *szTo = NULL, *szValue = NULL, *szProvider = NULL, *szService = NULL, *szAnnounce = NULL, *szUser = NULL, *szServiceAction = 0;
   EDF *pAnnounce = NULL;
   UserItem *pCurr = CONNUSER, *pContact = NULL;
   EDFConn *pProviderConn = NULL, *pContactConn = NULL;
   ConnData *pConnData = CONNDATA, *pContactData = NULL;
   DBSubData *pSubData = NULL;

   if(mask(pConnData->m_iStatus, LOGIN_SHADOW) == true)
   {
      pOut->Set("reply", MSG_RQ_INVALID);

      return false;
   }

   iFromID = pCurr->GetID();
   szFrom = pCurr->GetName(true);
   iAccessLevel = pCurr->GetAccessLevel();
   iUserType = pCurr->GetUserType();

   pIn->Get(NULL, &szRequest);
   iBase = RequestGroup(pIn);
   szAnnounce = szRequest;

   pIn->GetChild("toid", &iToID);
   pIn->GetChild("toname", &szTo);
   pIn->GetChild("contactid", &lContactID);
   pIn->GetChild("serviceid", &iServiceID);

   if(ConnVersion(pConnData, "2.6") >= 0)
   {
      bDivert = pIn->GetChildBool("divert");
   }
   else
   {
      pIn->GetChild("contacttype", &iContactType);
      if(iContactType == CONTACT_POST)
      {
         bDivert = true;
      }
   }

   if(iBase == RFG_USER)
   {
      szAnnounce = MSG_USER_PAGE;
   }

   bOverride = pIn->GetChildBool("override");
   debug("SystemContact entry %s(%d, %s) %d %s %s %ld %d\n", szRequest, iBase, szAnnounce, iToID, szTo, BoolStr(bDivert), lContactID, iServiceID);
   if(iBase == RFG_SYSTEM)
   {
      EDFParser::debugPrint(pIn);
   }

   if(pIn->GetChild("fromname", &szValue) == true)
   {
      bPrivilege = PrivilegeValid(pCurr, szRequest);
      debug("SystemContact fromname %s, privilege %s\n", szValue, BoolStr(bPrivilege));
      if(bPrivilege == true)
      {
         delete[] szFrom;
         szFrom = szValue;
      }
      else
      {
         delete[] szValue;
      }
   }

   if(iBase == RFG_SYSTEM || iServiceID != -1)
   {
      EDFParser::debugPrint("SystemContact system / service", pIn);

      pData->Root();
      pData->Child("services");

      if(EDFFind(pData, "service", iServiceID, false) == false)
      {
         pOut->Set("reply", SystemContactStr(iBase, "invalid"));
         pOut->SetChild("serviceid", iServiceID);

         debug("SystemContact exit false, %s (cannot find service %d)\n", SystemContactStr(iBase, "invalid"), iServiceID);
         return false;
      }

      pData->GetChild("name", &szService);
      pData->GetChild("servicetype", &iServiceType);

      pData->GetChild("providerid", &iProviderID);
      if(iBase == RFG_USER)
      {
         if(iProviderID == iFromID)
         {
            // This is the provider sending a request back to the server
            EDFParser::debugPrint("SystemContact provider content", pIn);

            delete[] szFrom;
            if(pIn->GetChild("fromname", &szFrom) == false)
            {
               szFrom = NULL;
            }

            if(SystemContactFind(pConn, pData, pIn, szRequest, iBase, false, iToID, szTo, lContactID, bOverride, iFromID, iUserType, iAccessLevel, pOut, &pContactConn, &szUser) == false)
            {
               debug("SystemContact exit false\n");
               return true;
            }
         }
         else
         {
            STACKTRACEUPDATE

            if(mask(iServiceType, SERVICE_CONTACT) == true)
            {
               pSubData = pConnData->m_pServices->SubData(iServiceID);

               if(pSubData != NULL)
               {
                  iSubType = pSubData->m_iSubType;
                  iExtra = pSubData->m_iExtra;
               }
            }

            debug("SystemContact servicetype %d extra %d\n", iServiceType, iExtra);
            if(mask(iServiceType, SERVICE_CONTACT) == false || iExtra == 0)
            {
               pOut->Set("reply", SystemContactStr(iBase, "invalid"));
               pOut->SetChild("serviceid", iServiceID);
               pOut->SetChild("servicename", szService);

               delete[] szService;

               debug("SystemContact exit false, %s (no contact mask or zero extra)\n", SystemContactStr(iBase, "invalid"));
               return false;
            }

            pProviderConn = ConnectionFind(CF_USERPRI, iProviderID);
            if(pProviderConn == NULL)
            {
               pOut->Set("reply", SystemContactStr(iBase, "invalid"));
               pOut->SetChild("serviceid", iServiceID);
               pOut->SetChild("servicename", szService);
               if(iAccessLevel >= LEVEL_WITNESS)
               {
                  pOut->AddChild("providerid", iProviderID);
                  if(UserGet(iProviderID, &szProvider) != NULL)
                  {
                     pOut->AddChild("providername", szProvider);
                  }
               }

               delete[] szService;

               debug("SystemContact exit false, %s (no provider)\n", SystemContactStr(iBase, "invalid"));
               return false;
            }

            pContactConn = pProviderConn;
         }
      }
      else if(iBase == RFG_SYSTEM)
      {
         if(mask(pCurr->GetUserType(), USERTYPE_AGENT) == false || iCurrID != iProviderID)
         {
            pOut->Set("reply", MSG_ACCESS_INVALID);

            delete[] szService;

            debug("SystemContact exit false, %s (no agent mask or curr %d not provider %d\n", MSG_ACCESS_INVALID, iCurrID, iProviderID);
            return false;
         }

         /* if(pIn->IsChild("content-type") == false)
         {
            pOut->Set("reply", SystemContactStr(iBase, "invalid"));

            delete[] szService;

            debug("SystemContact exit false, %s\n", SystemContactStr(iBase, "invalid"));
            return false;
         } */

         bMultiple = true;
      }

      pData->GetChild("name", &szService);

      pAnnounce = new EDF();
      /* if(iBase == RFG_USER)
      {
         iToID = iProviderID;
      } */
      pAnnounce->AddChild("serviceid", iServiceID);
      pAnnounce->AddChild("servicename", szService);
      if(pIn->Child("serviceaction") == true)
      {
         pAnnounce->Copy(pIn, true);

         pIn->Get(NULL, &szServiceAction);

         pIn->Parent();
      }
      /* else if(mask(iServiceType, SERVICE_LOGIN) == true)
      {
         pAnnounce->AddChild(pIn, "username");
         pAnnounce->AddChild(pIn, "password");
      } */

      // pAnnounce->AddChild(pIn, "content-type");
      if(pIn->Child("data") == true)
      {
         pAnnounce->Copy(pIn);
         pIn->Parent();
      }

      delete[] szService;
   }
   else
   {
      if(SystemContactFind(pConn, pData, pIn, szRequest, iBase, bDivert, iToID, szTo, lContactID, bOverride, iFromID, iUserType, iAccessLevel, pOut, &pContactConn, &szUser) == false)
      {
         debug("SystemContact exit false\n");
         return false;
      }
   }

   // Create the announcement
   if(pAnnounce == NULL)
   {
      pAnnounce = new EDF();
   }
   pAnnounce->AddChild("date", (int)(time(NULL)));
   if(iServiceID == -1 || iFromID != iProviderID)
   {
      pAnnounce->AddChild("fromid", iFromID);
   }
   if(szFrom != NULL)
   {
      pAnnounce->AddChild("fromname", szFrom);
   }
   if(bMultiple == false)
   {
      pAnnounce->AddChild("toid", iToID);
      if(szTo != NULL)
      {
         pAnnounce->AddChild("toname", szTo);
      }
      else if(szUser != NULL)
      {
         pAnnounce->AddChild("toname", szUser);
      }

      if(lContactID == -1)
      {
         m_lContactID++;
         lContactID = m_lContactID;
      }
      pAnnounce->AddChild("contactid", lContactID);
   }
   pAnnounce->AddChild(pIn, "text");

   if(bMultiple == true)
   {
      bLoop = pIn->Child("toid");
   }
   else
   {
      bLoop = true;
   }

   if(iServiceID != -1)
   {
      EDFParser::debugPrint("SystemContact service announcement", pAnnounce);
   }

   while(bLoop == true)
   {
      if(bMultiple == true)
      {
         pIn->Get(NULL, &iToID);
         pContactConn = ConnectionFind(CF_USERPRI, iToID);
      }

      if(pContactConn != NULL)
      {
         pContactData = (ConnData *)pContactConn->Data();
         pContact = pContactData->m_pUser;

         while(pAnnounce->DeleteChild("attachment") == true);
         AttachmentSection(pData, iBase, RQ_ADD, RQ_ADD, NULL, pAnnounce, pContactData, bMultiple, pIn, pOut, pContact->GetID(), pContact->GetAccessLevel(), 0);

         if(iProviderID == iFromID && szServiceAction != NULL)
         {
            if(stricmp(szServiceAction, ACTION_LOGIN) == 0)
            {
               pContactData->m_pServices->Update(iServiceID, iSubType > 0 ? iSubType : SUBTYPE_SUB, 1, true);
            }
            else if(stricmp(szServiceAction, ACTION_LOGOUT) == 0)
            {
               pContactData->m_pServices->Delete(iServiceID);
            }
         }

         ServerAnnounce(pData, szAnnounce, pAnnounce, NULL, pContactConn);

         if(iProviderID == -1 || pContact->GetAccessLevel() >= LEVEL_WITNESS)
         {
            pOut->AddChild("toid", iToID);
            pOut->AddChild("toname", pContact->GetName());
         }
      }

      if(bMultiple == true)
      {
         bLoop = pIn->Next("toid");
         if(bLoop == false)
         {
            pIn->Parent();
         }
      }
      else
      {
         bLoop = false;
      }
   }

   delete pAnnounce;

   delete[] szServiceAction;

   delete[] szRequest;
   delete[] szFrom;
   delete[] szTo;

   return true;
}

/*
** SystemMaintenance: Begin maintenance cycle
*/
ICELIBFN bool SystemMaintenance(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   STACKTRACE
   // bool bLoop = false, bDoMaint = false;
   // int iFolderNum = 0; //, iID = 0, iMaxID = 0;
   long lTick = 0;
   double dTick = 0;
   EDF *pAnnounce = NULL, *pAdmin = NULL;
   UserItem *pCurr = NULL;

   if(pConn != NULL)
   {
      pCurr = CONNUSER;
   }

   pData->Root();
   pData->Child("system");

   /* while(pData->DeleteChild("maintfolder") == true);

   for(iFolderNum = 0; iFolderNum < FolderCount(); iFolderNum++)
   {
      pData->AddChild("maintfolder", FolderList(iFolderNum)->GetID());
   } */

   pData->SetChild("mainttime", (int)(time(NULL)));

   // EDFPrint("SystemMaintenance", pData, false);

   pAnnounce = new EDF();
   pAnnounce->AddChild("mainttype", 0);
   if(pCurr != NULL)
   {
      pAnnounce->AddChild("byid", pCurr->GetID());
      pAnnounce->AddChild("byname", pCurr->GetName());
   }
   ServerAnnounce(pData, MSG_SYSTEM_MAINTENANCE, NULL, pAnnounce);
   /* if(bDoMaint == false)
   {
      pAnnounce->SetChild("mainttype", 1);
      ServerAnnounce(pConn, pData, MSG_SYSTEM_MAINTENANCE, NULL, NULL, pAnnounce);

      pData->Root();
      pData->Child("system");
      pData->SetChild("write", true);
   } */
   delete pAnnounce;

   dTick  = gettick();
   FolderMaintenance(pConn, pData);
   UserMaintenance(pConn, pData);
   lTick = tickdiff(dTick);

   pAnnounce = new EDF();
   pAnnounce->AddChild("mainttype", 1);
   pAdmin = new EDF(pAnnounce);
   pAdmin->AddChild("mainttime", lTick);
   ServerAnnounce(pData, MSG_SYSTEM_MAINTENANCE, pAnnounce, pAdmin);
   delete pAdmin;
   delete pAnnounce;

   // delete pMaint;

   // DBExecute("delete from message_info");
   // DBExecute("delete from message_text");

   return true;
}

/*
** ConnectionDrop: Remove a connection from the list
*/
ICELIBFN bool ConnectionDrop(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   STACKTRACE
   int iConnectionID = -1;
   UserItem *pCurr = CONNUSER;

   if(pIn->GetChild("connectionid", &iConnectionID) == false)
   {
      pOut->Set("reply", MSG_CONNECTION_NOT_EXIST);
      return false;
   }

   if(ConnectionShut(pData, CF_CONNID, iConnectionID, pCurr, pIn) == false)
   {
      pOut->Set("reply", MSG_CONNECTION_NOT_EXIST);
      pOut->AddChild("connectionid", iConnectionID);
      return false;
   }

   pOut->AddChild("connectionid", iConnectionID);

   return true;
}

/*
** LocationAdd: Add an entry to the locations list
*/
ICELIBFN bool LocationAdd(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   STACKTRACE
   int iMaxID = 0, iNumLocations = 0, iPosition = 0, iParentID = 0;//, iLocationID = 0;
   bool bFound = false;//, bLoop = true
   char *szName = NULL;//, *szType = NULL, *szValue = NULL;

   EDFParser::debugPrint("LocationAdd entry", pIn);

   if(pIn->GetChild("name", &szName) == false || szName == NULL)
   {
      // pOut->Set("reply", MSG_LOCATION_INVALID);

      debug("LocationAdd exit false\n");
      return false;
   }

   if(pIn->Children("hostname") + pIn->Children("address") == 0)
   {
      pOut->Set("reply", MSG_LOCATION_INVALID);
      pOut->AddChild("name", szName);

      debug("LocationAdd exit false\n");
      return false;
   }

   pData->Root();
   // pData->Child("system");
   pData->Child("locations");

   // pData->GetChild("numlocations", &iNumLocations);
   iNumLocations = pData->Children("location", true);
   // pData->GetChild("maxid", &iMaxID);
   iMaxID = EDFMax(pData, "location", true);

   if(pIn->GetChild("parentid", &iParentID) == true)
   {
      /* bLoop = pData->Child("location");
      while(bLoop == true && bFound == false)
      {
         pData->Get(NULL, &iLocationID);
         if(iLocationID == iParentID)
         {
            iNumLocations = pData->Children("location");
            bFound = true;
         }
         else
         {
            bLoop = pData->Iterate("location", "locations");
         }
      } */
      bFound = EDFFind(pData, "location", iParentID, true);

      if(bFound == false)
      {
         pOut->Set("reply", MSG_LOCATION_NOT_EXIST);
         pOut->AddChild("locationid", iParentID);

         delete[] szName;

         debug("LocationAdd exit false\n");
         return false;
      }
   }

   // printf("LocationAdd init position %d\n", iPosition);
   pIn->GetChild("position", &iPosition);
   if(iPosition < 0 || iPosition >= iNumLocations)
   {
      iPosition = EDFElement::LAST;
   }
   debug("LocationAdd new position %d\n", iPosition);

   pData->Add("location", ++iMaxID, iPosition);
   pData->AddChild("name", szName);

   /* bLoop = pIn->Child();
   while(bLoop == true)
   {
      szValue = NULL;

      pIn->Get(&szType, &szValue);
      if((stricmp(szType, "hostname") == 0 || stricmp(szType, "address") == 0) && szValue != NULL)
      {
         pData->AddChild(szType, szValue);
      }

      delete[] szType;
      delete[] szValue;

      bLoop = pIn->Next();
      if(bLoop == false)
      {
         pIn->Parent();
      }
   }
   debugEDFPrint("LocationAdd new location", pData, false); */

   EDFParser::debugPrint("LocationAdd pre-section location", pData, EDFElement::EL_CURR | EDFElement::PR_SPACE);

   EDFSection(pData, pIn, RQ_ADD, false, "hostname", EDFElement::BYTES | EDFSECTION_NOTNULL | EDFSECTION_NOTEMPTY, "address", EDFElement::BYTES | EDFSECTION_NOTNULL | EDFSECTION_NOTEMPTY, NULL);

   EDFParser::debugPrint("LocationAdd post-section location", pData, EDFElement::EL_CURR | EDFElement::PR_SPACE);

   // pData->Root();
   // pData->Child("system");
   // pData->Child("locations");
   // pData->SetChild("numlocations", iNumLocations + 1);
   // pData->SetChild("maxid", iMaxID);

   ConnectionLocations(pData);

   pOut->AddChild("locationid", iMaxID);
   pOut->AddChild("name", szName);

   delete[] szName;

   debug("LocationAdd exit true\n");
   return true;
}

/*
** LocationDelete: Delete an entry from the locations list
*/
ICELIBFN bool LocationDelete(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   STACKTRACE
   int iLocationID = 0;//, iLocationEDF = 0, iNumLocations = 0;
   // bool bLoop = true, bSuccess = false;

   if(pIn->GetChild("locationid", &iLocationID) == false)
   {
      pOut->Set("reply", MSG_LOCATION_NOT_EXIST);

      return false;
   }

   pData->Root();
   pData->Child("locations");

   if(EDFFind(pData, "location", iLocationID, true) == false)
   {
      pOut->Set("reply", MSG_LOCATION_NOT_EXIST);
      pOut->AddChild("locationid", iLocationID);

      return false;
   }

   EDFParser::debugPrint("LocationDelete delete point", pData, EDFElement::EL_CURR | EDFElement::PR_SPACE);
   pData->Delete();

   // if(bSuccess == false)
   {
      ConnectionLocations(pData);

      /* pOut->Set("reply", MSG_LOCATION_NOT_EXIST);
      pOut->AddChild("locationid", iLocationID);

      return false; */
   }

   pOut->AddChild("locationid", iLocationID);

   return true;
}

/*
** LocationEdit: Edit an entry in the locations list
*/
ICELIBFN bool LocationEdit(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   STACKTRACE
   int iLocationID = 0, /* iLocationEDF = 0, */ iParentID = -1, iPosition = EDFElement::ABSLAST;
   // bool bLoop = true, bFound = false;
   char *szLocationName = NULL;
   EDFElement *pElement = NULL;

   if(pIn->GetChild("locationid", &iLocationID) == false)
   {
      pOut->Set("reply", MSG_LOCATION_NOT_EXIST);

      return false;
   }

   pData->Root();
   pData->Child("locations");

   if(EDFFind(pData, "location", iLocationID, true) == false)
   {
      pOut->Set("reply", MSG_LOCATION_NOT_EXIST);
      pOut->AddChild("locationid", iLocationID);

      return false;
   }

   EDFSetStr(pData, pIn, "name", UA_SHORTMSG_LEN);
   pData->GetChild("name", &szLocationName);

   if(pIn->GetChild("parentid", &iParentID) == true)
   {
      pElement = pData->GetCurr();

      pData->Root();
      pData->Child("locations");

      if(iParentID != -1)
      {
         if(EDFFind(pData, "location", iParentID, true) == true)
         {
            pData->MoveFrom(pElement);
         }
      }
      else
      {
         pData->MoveFrom(pElement);
      }

      pData->SetCurr(pElement);
   }

   if(pIn->GetChild("position", &iPosition) == true)
   {
      pData->MoveFrom(pElement, iPosition);
   }

   EDFParser::debugPrint("LocationEdit pre-section location", pData, EDFElement::EL_CURR | EDFElement::PR_SPACE);

   EDFSection(pData, pIn, RQ_ADD, "hostname", EDFElement::BYTES | EDFSECTION_NOTNULL | EDFSECTION_NOTEMPTY, "address", EDFElement::BYTES | EDFSECTION_NOTNULL | EDFSECTION_NOTEMPTY, NULL);
   EDFSection(pData, pIn, RQ_DELETE, "hostname", EDFElement::BYTES, "address", EDFElement::BYTES, NULL);

   EDFParser::debugPrint("LocationEdit post-section location", pData, EDFElement::EL_CURR | EDFElement::PR_SPACE);

   ConnectionLocations(pData);

   pOut->AddChild("locationid", iLocationID);
   pOut->AddChild("locationname", szLocationName);

   delete[] szLocationName;

   return true;
}

/*
** LocationList: Generate a list of locations
*/
ICELIBFN bool LocationList(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   STACKTRACE
   int /* iNumLocations = 0, */ iLocationID = 0;
   bool bLoop = true, bNext = false;//, bGetCopy = pData->GetCopy(false);
   // char *szType = NULL, *szValue = NULL;

   pData->Root();
   // pData->Child("system");
   pData->Child("locations");
   // EDFPrint("LocationList entry", pData, false);

   // pData->GetChild("numlocations", &iNumLocations);
   pOut->AddChild("numlocations", pData->Children("location", true));

   if(pIn->GetChild("locationid", &iLocationID) == true)
   {
      if(EDFFind(pData, "location", iLocationID, true) == false)
      {
         pOut->Set("reply", MSG_LOCATION_NOT_EXIST);
         pOut->AddChild("locationid", iLocationID);

         return false;
      }

      pOut->Add("location", iLocationID);
      pOut->Copy(pData, false, false);
      while(pOut->DeleteChild("location"));
   }
   else
   {
      bLoop = pData->Child("location");
      while(bLoop == true)
      {
         pData->Get(NULL, &iLocationID);

         // EDFPrint("LocationList single entry", pData, EDFElement::EL_CURR | EDFElement::PR_SPACE);

         pOut->Add("location", iLocationID);
         pOut->Copy(pData, false, false);
         while(pOut->DeleteChild("location"));

         if(pData->Child("location") == false)
         {
            pOut->Parent();

            bNext = pData->Next("location");
            while(bNext == false)
            {
               bNext = pData->Parent();
               if(bNext == true)
               {
                  pOut->Parent();
               }

               if(bNext == false || stricmp(pData->GetCurr()->getName(false), "locations") == 0)
               {
                  bNext = true;
                  bLoop = false;
               }
               else
               {
                  bNext = pData->Next("location");
               }
            }
         }
      }
   }

   // pData->GetCopy(bGetCopy);

   // pOut->AddChild("numlocations", iNumLocations);

   // EDFPrint("LocationList exit", pOut);
   return true;
}

/*
** LocationSort: Consolidate the locations list (same names, better matches etc)
*/
ICELIBFN bool LocationSort(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   STACKTRACE
   int iCheck = 0;

   pData->Root();
   pData->Child("locations");

   debug("LocationSort %d locations\n", pData->Children("location", true));
   iCheck = LocationCheck(pData, 0, false);
   debug("LocationSort %d checks\n\n", iCheck);

   pData->Root();
   pData->Child("locations");
   EDFParser::debugPrint("LocationSort pre-fix", pData);

   LocationFix(pData);

   pData->Root();
   pData->Child("locations");
   EDFParser::debugPrint("LocationSort post-fix", pData);

   ConnectionLocations(pData);

   return true;
}

/*
** LocationLookup: Match a hostname / address to a location
*/
ICELIBFN bool LocationLookup(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   STACKTRACE
   unsigned long lAddress = 0;
   char *szHostname = NULL, *szAddress = NULL, *szLocation = NULL;

   EDFParser::debugPrint("LocationLookup entry", pIn);

   pIn->GetChild("hostname", &szHostname);
   pIn->GetChild("address", &szAddress);

   if(szHostname != NULL)
   {
      pOut->AddChild("hostame", szHostname);

      szLocation = ConnectionLocation(pData, szHostname, 0);
   }
   else if(szAddress != NULL)
   {
      pOut->AddChild("address", szAddress);

      if(Conn::StringToAddress(szAddress, &lAddress, false) == true)
      {
         szLocation = ConnectionLocation(pData, NULL, lAddress);
      }
   }

   if(szLocation != NULL)
   {
      debug("LocationLookup matched %s\n", szLocation);
      pOut->AddChild("location", szLocation);
   }
   else
   {
      debug("LocationLookup no match\n");
   }

   delete[] szHostname;
   delete[] szAddress;
   delete[] szLocation;

   debug("LocationLookup exit true\n");
   return true;
}

/*
** HelpAdd: Add a help topic
*/
ICELIBFN bool HelpAdd(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   STACKTRACE
   int iMaxID = 0;

   pData->Root();
   pData->Child("system");

   iMaxID = EDFMax(pData, "help");

   pData->Add("help", ++iMaxID);
   pData->AddChild(pIn, "subject");
   pData->AddChild(pIn, "text");
   pData->AddChild("date", (int)(time(NULL)));
   pData->Parent();

   pOut->AddChild("helpid", iMaxID);

   debug("HelpAdd exit true, %d topics\n", pData->Children("help"));
   return true;
}

/*
** HelpDelete: Delete a help topic
*/
ICELIBFN bool HelpDelete(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   STACKTRACE
   int iHelpID = 0;//, iHelpEDF = 0;
   // bool bLoop = false, bFound = false;

   pIn->GetChild("helpid", &iHelpID);

   pData->Root();
   pData->Child("system");

   if(EDFFind(pData, "help", iHelpID, false) == false)
   {
      pOut->Set("reply", MSG_HELP_NOT_EXIST);
      pOut->AddChild("helpid", iHelpID);

      return false;
   }

   pData->Delete();

   pOut->AddChild("helpid", iHelpID);

   /* if(bFound == false)
   {
      pOut->Set("reply", MSG_HELP_NOT_EXIST);
   } */

   return true;
}

/*
** HelpEdit: Edit a help topic
*/
ICELIBFN bool HelpEdit(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   return true;
}

/*
** HelpList: Generate a list of help topics
**
** helpid:    ID of the required topic
*/
ICELIBFN bool HelpList(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   STACKTRACE
   int iSearchType = 1, iHelpID = 0, iHelpEDF = 0, iAccessLevel = LEVEL_NONE, iUserID = 0;
   bool bLoop = false;
   char *szUsername = NULL;

   if(pIn->GetChild("helpid", &iHelpID) == true)
   {
      iSearchType = 2;
   }

   pData->GetChild("accesslevel", &iAccessLevel);

   pData->Root();
   pData->Child("system");

   debug("HelpList entry %d, %d %d topics\n", iHelpID, iSearchType, pData->Children("help"));
   // EDFPrint(pData, false);

   bLoop = pData->Child("help");
   while(bLoop == true)
   {
      pData->Get(NULL, &iHelpEDF);

      if(iSearchType == 1 || (iSearchType == 2 && iHelpID == iHelpEDF))
      {
         pOut->Add("help", iHelpEDF);
         if(iAccessLevel >= LEVEL_WITNESS)
         {
            pOut->AddChild(pData, "date");
            pData->GetChild("fromid", &iUserID);
            // szUsername = NULL;
            // if(UserGet(pData, iUserID, &szUsername, true) == true)
            if(UserGet(iUserID, &szUsername) != NULL)
            {
               pOut->AddChild("fromid", iUserID);
               pOut->AddChild("fromname", szUsername);
            }
            // delete[] szUsername;
         }
         pOut->AddChild(pData, "subject");
         if(iSearchType == 2)
         {
            pOut->AddChild(pData, "text");
         }
         pOut->Parent();

         if(iSearchType == 2)
         {
            bLoop = false;
         }
      }

      if(bLoop == true)
      {
         bLoop = pData->Next("help");
      }
   }

   pOut->AddChild("searchtype", iSearchType);

   debug("HelpList exit\n");
   return true;
}

/*
** TaskAdd: Add a task topic
*/
ICELIBFN bool TaskAdd(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   STACKTRACE
   int iMinute = 0, iHour = 0, iDay = 0, iRepeat = 0, iMaxID = 0;
   long lTime = 0;
   UserItem *pCurr = CONNUSER;

   if(pIn->IsChild("request") == false)
   {
      pOut->Set("reply", MSG_TASK_NOT_EXIST);
      debug("TaskAdd exit false, %s\n", MSG_TASK_NOT_EXIST);
      return false;
   }

   if(pIn->GetChild("time", &lTime) == false)
   {
      pIn->GetChild("repeat", &iRepeat);
      if(iRepeat < TASK_DAILY || iRepeat > TASK_WEEKLY)
      {
         iRepeat = TASK_DAILY;
      }

      pIn->GetChild("minute", &iMinute);
      pIn->GetChild("hour", &iHour);
      if(iRepeat == TASK_WEEKLY && pIn->GetChild("day", &iDay) == true)
      {
         if(iDay < 0 || iDay >= 7)
         {
            iDay = 0;
         }
      }
   }

   pData->Root();
   pData->Child("system");
   iMaxID = EDFMax(pData, "task");
   pData->Add("task", ++iMaxID);

   pData->AddChild("date", (int)(time(NULL)));
   pData->AddChild("fromid", pCurr->GetID());

   if(lTime > 0)
   {
      pData->AddChild("nexttime", lTime);

      pOut->AddChild("nexttime", lTime);
   }
   else
   {
      lTime = TaskNext(iHour, iMinute, iDay, iRepeat);

      pData->AddChild("minute", iMinute);
      pData->AddChild("hour", iHour);
      if(iRepeat == TASK_WEEKLY)
      {
         pData->AddChild("day", iDay);
      }
      pData->AddChild("nexttime", lTime);
      if(iRepeat > 0)
      {
         pData->AddChild("repeat", iRepeat);
      }

      pOut->AddChild("minute", iMinute);
      pOut->AddChild("hour", iHour);
      pOut->AddChild("nexttime", lTime);
      if(iRepeat == TASK_WEEKLY)
      {
         pOut->AddChild("day", iDay);
      }
      if(iRepeat > 0)
      {
         pOut->AddChild("repeat", iRepeat);
      }
   }

   pIn->Child("request");
   pData->Copy(pIn);
   pIn->Parent();

   pOut->AddChild("taskid", iMaxID);

   return true;
}

/*
** TaskDelete: Delete a task topic
*/
ICELIBFN bool TaskDelete(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   STACKTRACE
   int iTaskID = 0;

   pIn->GetChild("taskid", &iTaskID);

   pData->Root();
   pData->Child("system");

   if(EDFFind(pData, "task", iTaskID, false) == false)
   {
      pOut->Set("reply", MSG_TASK_NOT_EXIST);
      pOut->AddChild("taskid", iTaskID);

      return false;
   }

   pData->Delete();

   pOut->AddChild("taskid", iTaskID);

   return true;
}

/*
** TaskEdit: Edit a task topic
*/
ICELIBFN bool TaskEdit(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   int iTaskID = 0;

   if(pIn->GetChild("taskid", &iTaskID) == false)
   {
      pOut->Set("reply", MSG_TASK_NOT_EXIST);
      return false;
   }

   pData->Root();
   pData->Child("system");

   if(EDFFind(pData, "task", iTaskID, false) == false)
   {
      pData->Set("reply", MSG_TASK_NOT_EXIST);
      pData->AddChild("taskid", iTaskID);
      return false;
   }

   pOut->AddChild("taskid", iTaskID);

   return true;
}

/*
** TaskList: Generate a list of task topics
**
** taskid:    ID of the required topic
*/
ICELIBFN bool TaskList(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   STACKTRACE
   int iSearchType = 1, iTaskID = 0, iTaskEDF = 0, iAccessLevel = LEVEL_NONE, iUserID = 0;
   bool bLoop = false;
   char *szUsername = NULL;

   if(pIn->GetChild("taskid", &iTaskID) == true)
   {
      iSearchType = 2;
   }

   pData->GetChild("accesslevel", &iAccessLevel);

   pData->Root();
   pData->Child("system");

   debug("TaskList entry %d, %d %d tasks\n", iTaskID, iSearchType, pData->Children("task"));
   // EDFPrint(pData, false);

   bLoop = pData->Child("task");
   while(bLoop == true)
   {
      pData->Get(NULL, &iTaskEDF);

      if(iSearchType == 1 || (iSearchType == 2 && iTaskID == iTaskEDF))
      {
         pOut->Add("task", iTaskEDF);
         // if(iAccessLevel >= LEVEL_WITNESS)
         {
            pOut->AddChild(pData, "date");
            pData->GetChild("fromid", &iUserID);
            // szUsername = NULL;
            // if(UserGet(pData, iUserID, &szUsername, true) == true)
            if(UserGet(iUserID, &szUsername) != NULL)
            {
               pOut->AddChild("fromid", iUserID);
               pOut->AddChild("fromname", szUsername);
            }
            // delete[] szUsername;

            pOut->AddChild(pData, "nexttime");
            pOut->AddChild(pData, "minute");
            pOut->AddChild(pData, "hour");
            pOut->AddChild(pData, "day");
            pOut->AddChild(pData, "repeat");
            pOut->AddChild(pData, "done");

            if(iSearchType == 2)
            {
               if(pData->Child("request") == true)
               {
                  pOut->Copy(pData);
                  pData->Parent();
               }
            }
            else
            {
               pOut->AddChild(pData, "request");
            }
         }
         pOut->Parent();

         if(iSearchType == 2)
         {
            bLoop = false;
         }
      }

      if(bLoop == true)
      {
         bLoop = pData->Next("task");
      }
   }

   pOut->AddChild("searchtype", iSearchType);

   debug("TaskList exit\n");
   return true;
}

/*
** ServiceAdd: Add an entry to the Services list
*/
ICELIBFN bool ServiceAdd(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   STACKTRACE
   int iServiceID = 0, iValue = 0, iServiceType = 0;
   char *szName = NULL;

   EDFParser::debugPrint("ServiceAdd entry", pIn);

   if(pIn->GetChild("name", &szName) == false || szName == NULL)
   {
      pOut->Set("reply", MSG_SERVICE_INVALID);

      debug("ServiceAdd exit false\n");
      return false;
   }

   pData->Root();
   // pData->Child("system");
   pData->Child("services");

   iServiceID = EDFMax(pData, "service", false);
   /* if(iServiceID <= CONTACT_POST)
   {
      iServiceID = CONTACT_POST + 1;
   } */

   pData->Add("service", ++iServiceID);
   pData->AddChild("name", szName);
   pData->AddChild(pIn, "content-type");
   pData->SetChild(pIn, "providerid");
   pIn->GetChild("servicetype", &iValue);
   if(mask(iValue, SERVICE_CONTACT) == true)
   {
      iServiceType |= SERVICE_CONTACT;
   }
   if(mask(iValue, SERVICE_LOGIN) == true)
   {
      iServiceType |= SERVICE_LOGIN;
   }
   if(mask(iValue, SERVICE_TONAME) == true)
   {
      iServiceType |= SERVICE_TONAME;
   }
   if(iServiceType > 0)
   {
      pData->SetChild("servicetype", iServiceType);
   }

   pOut->AddChild("serviceid", iServiceID);
   pOut->AddChild(pData, "name", "servicename");

   pData->Parent();

   delete[] szName;

   debug("ServiceAdd exit true\n");
   return true;
}

/*
** ServiceDelete: Delete an entry from the Services list
*/
ICELIBFN bool ServiceDelete(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   STACKTRACE
   int iServiceID = 1;

   if(pIn->GetChild("serviceid", &iServiceID) == false)
   {
      pOut->Set("reply", MSG_SERVICE_NOT_EXIST);
      return false;
   }

   pData->Root();
   pData->Child("services");

   if(EDFFind(pData, "service", iServiceID, false) == false)
   {
      pOut->Set("reply", MSG_SERVICE_NOT_EXIST);
      pOut->AddChild("serviceid", iServiceID);
      return false;
   }

   pOut->AddChild("serviceid", iServiceID);
   pOut->AddChild(pData, "name", "servicename");

   pData->Delete();

   return true;
}

/*
** ServiceEdit: Edit an entry in the Services list
*/
ICELIBFN bool ServiceEdit(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   STACKTRACE
   int iServiceID = -1, iValue = 0, iServiceType = 0;

   if(pIn->GetChild("serviceid", &iServiceID) == false)
   {
      pOut->Set("reply", MSG_SERVICE_NOT_EXIST);
      return false;
   }

   pData->Root();
   pData->Child("services");

   if(EDFFind(pData, "service", iServiceID, false) == false)
   {
      pOut->Set("reply", MSG_SERVICE_NOT_EXIST);
      pOut->AddChild("serviceid", iServiceID);
      return false;
   }

   EDFParser::debugPrint("SystemEdit service", pData, EDFElement::EL_CURR | EDFElement::PR_SPACE);

   pData->SetChild(pIn, "name");
   pData->SetChild(pIn, "content-type");
   pData->SetChild(pIn, "providerid");
   pIn->GetChild("servicetype", &iValue);
   pData->GetChild("servicetype", &iServiceType);
   if(mask(iValue, SERVICE_CONTACT) == true && mask(iServiceType, SERVICE_CONTACT) == false)
   {
      iServiceType += SERVICE_CONTACT;
   }
   else if(mask(iValue, SERVICE_CONTACT) == false && mask(iServiceType, SERVICE_CONTACT) == true)
   {
      iServiceType -= SERVICE_CONTACT;
   }
   if(mask(iValue, SERVICE_LOGIN) == true && mask(iServiceType, SERVICE_LOGIN) == false)
   {
      iServiceType += SERVICE_LOGIN;
   }
   else if(mask(iValue, SERVICE_LOGIN) == false && mask(iServiceType, SERVICE_LOGIN) == true)
   {
      iServiceType -= SERVICE_LOGIN;
   }
   if(iServiceType > 0)
   {
      pData->SetChild("servicetype", iServiceType);
   }

   pOut->AddChild("serviceid", iServiceID);
   pOut->AddChild(pData, "name", "servicename");

   pData->Parent();

   return true;
}

/*
** ServiceList: Generate a list of Services
*/
ICELIBFN bool ServiceList(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   STACKTRACE
   int iProviderID = -1, iValue = 0, iAccessLevel = LEVEL_NONE, iCurrID = -1;
   bool bLoop = false;
   char *szProvider = NULL;
   UserItem *pCurr = CONNUSER;
   ConnData *pConnData = CONNDATA;
   DBSubData *pSubData = NULL;

   if(pCurr != NULL)
   {
      iCurrID = pCurr->GetID();
      iAccessLevel = pCurr->GetAccessLevel();
   }

   pData->Root();
   pData->Child("services");

   EDFParser::debugPrint("ServiceList data", pData, EDFElement::EL_CURR | EDFElement::PR_SPACE);

   bLoop = pData->Child("service");
   while(bLoop == true)
   {
      iProviderID = -1;

      pData->Get(NULL, &iValue);
      pData->GetChild("providerid", &iProviderID);

      if(iProviderID != -1 || iAccessLevel >= LEVEL_WITNESS || iProviderID == iCurrID)
      {
         pOut->Add("service", iValue);
         pOut->AddChild(pData, "name");
         pOut->AddChild(pData, "content-type");
         pOut->AddChild(pData, "servicetype");
         if(iAccessLevel >= LEVEL_WITNESS || iProviderID == iCurrID)
         {
            if(UserGet(iProviderID, &szProvider) != NULL)
            {
               pOut->AddChild("providerid", iProviderID);
               pOut->AddChild("providername", szProvider);
            }
         }

         if(pConnData->m_pServices != NULL)
         {
            debug("SystemList service subtype %d -> %d\n", iValue, pConnData->m_pServices->SubType(iValue));
            pSubData = pConnData->m_pServices->SubData(iValue);
            if(pSubData != NULL)
            {
               pOut->AddChild("subtype", pSubData->m_iSubType);
               if(pSubData->m_iExtra > 0)
               {
                  pOut->AddChild("active", true);
               }
            }
         }
         pOut->Parent();
      }

      bLoop = pData->Next("service");
      if(bLoop == false)
      {
         pData->Parent();
      }
   }

   return true;
}

/*
** ServiceSubscribe: Subscribe to a folder / channel
**
** serviceid: ID of service
** userid(admin):  ID of the user to subscribe
** subtype: Type of subscription
**
** subtype defaults to SUBTYPE_SUB if the subscribe request is used and 0 if unsubscribe is used
*/
ICELIBFN bool ServiceSubscribe(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   STACKTRACE
   int iAccessLevel = LEVEL_NONE, iServiceID = -1, iUserID = -1, iCurrType = 0, iSubType = 0, iDebug = 0, iProviderID = -1, iServiceType = 0;
   bool bSuccess = false, bActive = false;
   char *szRequest = NULL, *szServiceName = NULL, *szUsername = NULL, *szPassword = NULL;
   EDF *pAnnounce = NULL;
   UserItem *pCurr = CONNUSER, *pItem = NULL;
   DBSub *pSubs = CONNSERVICES;
   EDFConn *pItemConn = NULL, *pProviderConn = NULL;
   ConnData *pConnData = CONNDATA, *pItemData = NULL;
   DBSubData *pSubData = NULL;

   iAccessLevel = pCurr->GetAccessLevel();

   pIn->Get(NULL, &szRequest);
   debug("ServiceSubscribe entry %s\n", szRequest);
   EDFParser::debugPrint(pIn, EDFElement::EL_CURR | EDFElement::PR_SPACE);

   // Get the ID
   if(pIn->GetChild("serviceid", &iServiceID) == false)
   {
      // No ID supplied
      pOut->Set("reply", MSG_SERVICE_NOT_EXIST);

      delete[] szRequest;

      debug("ServiceSubscribe exit false, %s\n", MSG_SERVICE_NOT_EXIST);
      return false;
   }

   pData->Root();
   pData->Child("services");
   if(EDFFind(pData, "service", iServiceID, false) == false)
   {
      pOut->Set("reply", MSG_SERVICE_NOT_EXIST);
      pOut->AddChild("serviceid", iServiceID);

      delete[] szRequest;

      debug("ServiceSubscribe exit false, %s %d\n", MSG_SERVICE_NOT_EXIST, iServiceID);
      return false;
   }

   pData->GetChild("name", &szServiceName);
   pData->GetChild("servicetype", &iServiceType);
   debug("ServiceSubscribe %s %d\n", szServiceName, iServiceType);

   if(stricmp(szRequest, MSG_SERVICE_SUBSCRIBE) == 0)
   {
      bActive = pIn->GetChildBool("active");
   }

   if(mask(iServiceType, SERVICE_CONTACT) == true && bActive == true)
   {
      debug("ServiceSubscribe contact active\n");

      pData->GetChild("providerid", &iProviderID);
      debug("ServiceSubscribe provider %d\n", iProviderID);
      if(iProviderID != -1)
      {
         pProviderConn = ConnectionFind(CF_USERID, iProviderID);
         if(pProviderConn == NULL)
         {
            pOut->Set("reply", MSG_SERVICE_ACCESS_INVALID);
            pOut->AddChild("serviceid", iServiceID);

            delete[] szServiceName;

            delete[] szRequest;

            debug("ServiceSubscribe exit false, %s (provider does not exist / not logged in)\n", MSG_SERVICE_ACCESS_INVALID);
            return false;
         }
      }
   }

   if(iAccessLevel >= LEVEL_WITNESS || pSubs->SubType(iServiceID) == SUBTYPE_EDITOR)
   {
      // Admin / restricted folder editors allowed to specify a user
      pIn->GetChild("userid", &iUserID);
   }

   if(iUserID != -1)
   {
      // Find specied user
      pItem = UserGet(iUserID);
      if(pItem == NULL)
      {
         // User does not exist
         pOut->Set("reply", MSG_USER_NOT_EXIST);
         pOut->SetChild("userid", iUserID);

         delete[] szServiceName;
         delete[] szRequest;

         debug("ServiceSubscribe exit false, %s %d\n", MSG_USER_NOT_EXIST, iUserID);
         return false;
      }

      // Check access again
      if(pItem->GetAccessLevel() >= iAccessLevel)
      {
         // Trying to change a user higher than the current one
         pOut->Set("reply", MSG_SERVICE_ACCESS_INVALID);

         delete[] szServiceName;
         delete[] szRequest;

         debug("ServiceSubscribe exit false, %s\n", MSG_ACCESS_INVALID);
         return false;
      }

      // Find this user's connection data
      pItemConn = ConnectionFind(CF_USERID, iUserID);

      if(pItemConn != NULL)
      {
         pItemData = (ConnData *)pItemConn->Data();
         pSubs = pItemData->m_pServices;

         pSubData = pSubs->SubData(iServiceID);
      }
      else
      {
         pSubs = NULL;

         pSubData = DBSub::UserData(SERVICE_SUB, "serviceid", iUserID, iServiceID);
      }
   }
   else
   {
      // Subscription for current user
      pItem = pCurr;
      pItemData = pConnData;

      pSubData = pSubs->SubData(iServiceID);
      debug("ServiceSubscribe current user %p, %d -> %p\n", pSubs, iServiceID, pSubData);
   }

   if(pSubData != NULL)
   {
      iCurrType = pSubData->m_iSubType;
   }

   debug("ServiceSubscribe %s %d, %s %ld %s %p", szRequest, iServiceID, pItem == pCurr ? "self" : "user", pItem->GetID(), pItem->GetName(), pSubData);
   if(pSubData != NULL)
   {
      debug("(%d %d %s)", pSubData->m_iSubType, pSubData->m_iExtra, BoolStr(pSubData->m_bTemp));
   }
   debug("\n");

   if(stricmp(szRequest, MSG_SERVICE_SUBSCRIBE) == 0)
   {
      // Subscribe
      if(pIn->GetChild("subtype", &iSubType) == true)
      {
         if(iAccessLevel >= LEVEL_WITNESS)
         {
            if(iSubType < SUBTYPE_SUB || iSubType > SUBTYPE_EDITOR)
            {
               iSubType = SUBTYPE_SUB;
            }
         }
         else if(iSubType != SUBTYPE_SUB)
         {
            iSubType = SUBTYPE_SUB;
         }

         if(pSubData != NULL)
         {
            if(pSubData->m_iSubType > iSubType)
            {
               iSubType = pSubData->m_iSubType;
            }
         }
      }
      else
      {
         if(pSubData != NULL)
         {
            iSubType = pSubData->m_iSubType;
         }
         else
         {
            iSubType = SUBTYPE_SUB;
         }
      }

      // bActive = pIn->GetChildBool("active");

      // bDebug = DBTable::Debug(true);
      iDebug = debuglevel(DEBUGLEVEL_DEBUG);
      if(pSubs != NULL)
      {
         /* if(iBase == RFG_FOLDER)
         {
            debug("ServiceSubscribe current eds %d\n", pItemData->m_pFolders->CountType(SUBTYPE_EDITOR) + pItemData->m_pChannels->CountType(SUBTYPE_EDITOR));
         } */

         // Update connection (assign auto active if login not required)
         bSuccess = pSubs->Update(iServiceID, iSubType, mask(iServiceType, SERVICE_LOGIN) == false ? SUBTYPE_SUB : 0, bActive);
      }
      else
      {
         if(iCurrType == 0)
         {
            // Add to database
            bSuccess = DBSub::UserAdd(SERVICE_SUB, "serviceid", pItem->GetID(), iServiceID, iSubType, 0);
         }
         else
         {
            // Update database
            bSuccess = DBSub::UserUpdate(SERVICE_SUB, "serviceid", pItem->GetID(), iServiceID, iSubType, 0);
         }
      }
      // DBTable::Debug(bDebug);
      debuglevel(iDebug);
   }
   else if(iCurrType > 0)
   {
      // Unsubcribe
      if(pSubs != NULL)
      {
         // Remove from connection

         // printf("ServiceSubscribe current eds %d\n", pSubs->CountType(SUBTYPE_EDITOR));

         if(pSubData != NULL)
         {
            bActive = pSubData->m_bTemp;
         }

         pSubs->Delete(iServiceID);
      }
      else
      {
         // Remove from database
         DBSub::UserDelete(SERVICE_SUB, "serviceid", pItem->GetID(), iServiceID);
      }

      bSuccess = true;
   }

   if(pProviderConn != NULL)
   {
      pAnnounce = new EDF();
      pAnnounce->AddChild("serviceid", iServiceID);
      pAnnounce->AddChild(pData, "name", "servicename");

      if(stricmp(szRequest, MSG_SERVICE_SUBSCRIBE) == 0)
      {
         pAnnounce->AddChild("userid", pItem->GetID());
         pAnnounce->AddChild("username", pItem->GetName());

         pAnnounce->AddChild(pIn, "name");
         pAnnounce->AddChild(pIn, "password");

         pAnnounce->AddChild(pIn, "status");
      }

      if(iSubType > 0)
      {
         // Current subtype
         pAnnounce->AddChild("subtype", iSubType);
      }
      else if(iSubType == 0 && iCurrType > 0)
      {
         // Previous subtype
         pAnnounce->AddChild("subtype", iCurrType);
      }

      ServerAnnounce(pData, szRequest, pAnnounce, NULL, pProviderConn);
   }

   pOut->AddChild("serviceid", iServiceID);
   pOut->AddChild("servicename", szServiceName);

   if(pCurr != pItem)
   {
      // Add user info if not the current user
      pOut->AddChild("userid", pItem->GetID());
      pOut->AddChild("username", pItem->GetName());
   }

   if(iSubType > 0)
   {
      // Current subtype
      pOut->AddChild("subtype", iSubType);
   }
   else if(iSubType == 0 && iCurrType > 0)
   {
      // Previous subtype
      pOut->AddChild("subtype", iCurrType);
   }

   delete[] szServiceName;
   delete[] szRequest;

   EDFParser::debugPrint("ServiceSubscribe exit true", pOut);
   return true;
}

}
