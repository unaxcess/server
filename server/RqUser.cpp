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
** RqUser.cpp: Implementation of server side user request functions
*/

// Standard headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifdef UNIX
#if !defined(FreeBSD) && !defined(OPENBSD)
#include <crypt.h>
#endif
#include <unistd.h>
#include <strings.h>
#include <sys/times.h>
#endif
#include <sys/types.h>
#include <time.h>
#include <ctype.h>

// Framework headers
#include "headers.h"
#include "ICE/ICE.h"

// Common headers
#include "Folder.h"

// Server headers
#include "server.h"

#include "RqUser.h"
#include "RqFolder.h"
#include "RqTree.h"
#include "RqSystem.h"

#define ULS_FULL 128
#define ULS_PICTURE 256

#define UL_LOGIN 1
#define UL_IDLE 2
#define UL_MED 3
#define UL_HIGH 4
#define UL_CURR 5
#define UL_IS_CONN(x) (x == UL_LOGIN || x == UL_IDLE)

// Local functions
bool UserItemEdit(EDF *pData, int iOp, UserItem *pItem, EDF *pIn, EDF *pOut, UserItem *pCurr, ConnData *pCurrData);

// UserReadDB: Read users from the database and populate user list
bool UserReadDB(EDF *pData)
{
   int iNumUsers = 0;
   double dTick = gettick();
   char szSQL[100];
   // EDF *pEDF = NULL;
   UserItem *pItem = NULL;
   DBTable *pTable = NULL;

   debug("UserReadDB entry\n");

   pTable = new DBTable();

   // Get rows
   sprintf(szSQL, "select * from %s", UI_TABLENAME);
   if(pTable->Execute(szSQL) == false)
   {
      debug("UserReadDB exit false, query failed\n");
      delete pTable;

      return false;
   }

   debug("UserReadDB checking rows\n");
   // while(pTable->NextRow() == true)
   do
   {
      pItem = UserItem::NextRow(pTable);

      if(pItem != NULL)
      {
         iNumUsers++;

         // pItem = new UserItem(pTable);
         debug("UserReadDB %d, %ld %s %d\n", iNumUsers, pItem->GetID(), pItem->GetName(), pItem->GetAccessLevel());

         /* EDF *pEDF = new EDF();
         pItem->Write(pEDF, -1);
         pEDF->Root();
         pEDF->Child();
         debugEDFPrint(pEDF);
         delete pEDF; */

         if(pItem->GetUserType() != USERTYPE_TEMP)
         {
            UserAdd(pItem);
         }
      }
   }
   while(pItem != NULL);

   delete pTable;

   debug("UserReadDB exit true, %d users in %ld ms\n", iNumUsers, tickdiff(dTick));
   return true;
}

// UserWriteDB: Write modified users to the database
bool UserWriteDB(bool bFull)
{
   int iUserNum = 0;
   double dTick = gettick(), dSingle = gettick();
   char szSQL[100];
   UserItem *pItem = NULL;
   DBTable *pTable = NULL;
   // ConnData *pItemData = NULL;

   debug("UserWriteDB entry\n");

   if(bFull == true)
   {
      // Full write - delete current contents
      pTable = new DBTable();
      sprintf(szSQL, "delete from %s", UI_TABLENAME);
      if(pTable->Execute(szSQL) == false)
      {
         delete pTable;
         debug("UserWriteDB exit false, delete query failed\n");
         return false;
      }
      delete pTable;
   }

   for(iUserNum = 0; iUserNum < UserCount(); iUserNum++)
   {
      dSingle = gettick();

      pItem = UserList(iUserNum);



      if(bFull == true)
      {
         debug("UserWriteDB inserting %ld\n", pItem->GetID());
         pItem->Insert();
         debug("UserWriteDB inserted %ld ms\n", tickdiff(dSingle));
      }
      else if(pItem->GetChanged() == true)
      {
         debug("UserWriteDB updating %ld\n", pItem->GetID());
         pItem->Update();
         debug("UserWriteDB inserted %ld ms\n", tickdiff(dSingle));
      }
   }

   debug("UserWriteDB exit true %ld ms\n", tickdiff(dTick));
   return true;
}

// UserSetup: Set up stats, perform validation
bool UserSetup(EDF *pData, int iOptions, bool bValidate, int iWriteTime)
{
   STACKTRACE
   int iListNum = 0, iTimeOn = 0, iDemoteLevel = 0;
   bytes *pEmail = NULL, *pRealName = NULL;
   UserItem *pItem = NULL, *pOwner = NULL;

   for(iListNum = 0; iListNum < UserCount(); iListNum++)
   {
      iTimeOn = 0;
      pOwner = NULL;

      pItem = UserList(iListNum);
      iDemoteLevel = pItem->GetAccessLevel();

      debug("UserSetup user %ld, %s %d %ld\n", pItem->GetID(), pItem->GetName(), pItem->GetAccessLevel(), pItem->GetOwnerID());

      if(bValidate == true)
      {
         STACKTRACEUPDATE

         if(NameValid(pItem->GetName()) == false)
         {
            // Disable invalid users
            debug("UserSetup warning: Invalid user name, setting access level to %d\n", LEVEL_NONE);
            pItem->SetAccessLevel(LEVEL_NONE);
         }

         if(mask(pItem->GetStatus(), LOGIN_ON) == true && mask(iOptions, ICELIB_RECOVER) == true && iWriteTime != -1)
         {
            // Recovering from a crash - add extra login time
            iTimeOn = pItem->GetTimeOn();
            if(iTimeOn > 0)
            {
               iTimeOn = iWriteTime - iTimeOn;
            }
            pItem->SetTimeOff(iWriteTime);
            debug("UserSetup extra time on %d\n", iTimeOn);
         }
         pItem->SetStatus(LOGIN_OFF);

         STACKTRACEUPDATE

         // Check details
         if(pItem->GetOwnerID() != -1)
         {
            pOwner = UserGet(pItem->GetOwnerID());
            debug("UserSetup owner %ld -> %p\n", pItem->GetOwnerID(), pOwner);
         }

         pItem->GetDetail("realname", NULL, &pRealName);
         if(pRealName == NULL && pOwner != NULL)
         {
            pOwner->GetDetail("realname", NULL, &pRealName);
         }

         // printf("UserSetup realname %p\n", pRealName);
         if(pRealName == NULL || pRealName->Data(false) == NULL || strcmp((char *)pRealName->Data(false), "") == 0)
         {
            debug("UserSetup warning: Invalid real name '%s'\n", pRealName != NULL ? pRealName->Data(false) : NULL);
            iDemoteLevel = LEVEL_GUEST;
         }

         STACKTRACEUPDATE

         pItem->GetDetail("email", NULL, &pEmail);
         if(pEmail == NULL && pOwner != NULL)
         {
            pOwner->GetDetail("email", NULL, &pEmail);
         }

         if(pEmail == NULL || UserEmailValid((char *)pEmail->Data(false)) == false)
         {
            debug("UserSetup warning: Invalid email address '%s'\n", pEmail != NULL ? pEmail->Data(false) : NULL);
            iDemoteLevel = LEVEL_GUEST;
         }

         STACKTRACEUPDATE

         if(pItem->GetAccessLevel() != iDemoteLevel)
         {
            // Change access level
            if(pItem->GetAccessLevel() >= LEVEL_WITNESS)
            {
               debug("UserSetup not demoting admin user\n");
            }
            else
            {
               debug("UserSetup demoting from to %d\n", iDemoteLevel);
               pItem->SetAccessLevel(iDemoteLevel);
            }
         }
      }

      STACKTRACEUPDATE

      // Calculate total stats
      if(iTimeOn > 0)
      {
         debug("UserSetup total login time t=%d p=%d i=%d\n", pItem->GetStat(USERITEMSTAT_TOTALLOGINS, true), pItem->GetStat(USERITEMSTAT_TOTALLOGINS, false), iTimeOn);
         pItem->IncStat(USERITEMSTAT_TOTALLOGINS, iTimeOn);
         if(iTimeOn > pItem->GetStat(USERITEMSTAT_LONGESTLOGIN, false))
         {
            pItem->SetStat(USERITEMSTAT_LONGESTLOGIN, iTimeOn);
         }
      }

      STACKTRACEUPDATE

      pItem->SetChanged(false);
   }

   // Summary report
   debug("UserStartup total %d, max ID %ld\n", UserCount(), UserMaxID());

   return true;
}

void UserMaintenance(EDFConn *pConn, EDF *pData)
{
}

UserItem *UserAdd(char *szName, char *szPassword, int iAccessLevel, int iUserType)
{
   STACKTRACE
   int iMaxID = 0;
   // char *szClient = NULL;
   char szTemp[100];
   UserItem *pItem = NULL;

   debug("UserAdd %s %s %d %d\n", szName, szPassword, iAccessLevel, iUserType);

   iMaxID = UserMaxID();
   iMaxID++;

   if(iUserType == USERTYPE_TEMP)
   {
      pItem = new UserItem(iMaxID);
      // debug("UserAdd UserItem class %s\n", pItem->GetClass());

      sprintf(szTemp, "guest%d", iMaxID);
      pItem->SetName(szTemp);
      pItem->SetUserType(USERTYPE_TEMP);
   }
   else
   {
      pItem = new UserItem(iMaxID);
      // debug("UserAdd UserItem class %s\n", pItem->GetClass());

      pItem->SetName(szName);
      pItem->SetAccessLevel(iAccessLevel);
   }

   UserAdd(pItem);

   // delete[] szName;

   return pItem;
}

bool UserDelete(EDF *pData, UserItem *pCurr, UserItem *pDelete)
{
   int iDebug = 0;
   // char szSQL[256];
   EDF *pAnnounce = NULL, *pAdmin = NULL;
   // DBTable *pTable = NULL;

   pAnnounce = new EDF();
   pAnnounce->AddChild("userid", pDelete->GetID());
   pAnnounce->AddChild("username", pDelete->GetName());

   // Allow admin users to see who deleted the user
   if(pCurr != NULL && pCurr != pDelete)
   {
      pAdmin = new EDF();
      /* pAdmin->AddChild("userid", pDelete->GetID());
      pAdmin->AddChild("username", pDelete->GetName()); */
      pAdmin->Copy(pAnnounce, true, true);
      pAdmin->AddChild("byid", pCurr->GetID());
      pAdmin->AddChild("byname", pCurr->GetName());
   }

   EDFParser::debugPrint("UserDelete user announcement", pAnnounce);
   EDFParser::debugPrint("UserDelete admin announcement", pAdmin);
   ServerAnnounce(pData, MSG_USER_DELETE, pAnnounce, pAdmin);

   delete pAnnounce;
   delete pAdmin;

   iDebug = debuglevel(DEBUGLEVEL_DEBUG);
   /* pTable = new DBTable();
   sprintf(szSQL, "delete from %s where userid = %ld", FM_READ_TABLE, pDelete->GetID());
   pTable->Execute(szSQL);
   sprintf(szSQL, "delete from %s where userid = %ld", MessageTreeSubTable(RFG_FOLDER), pDelete->GetID());
   pTable->Execute(szSQL);
   sprintf(szSQL, "delete from %s where userid = %ld", MessageTreeSubTable(RFG_CHANNEL), pDelete->GetID());
   pTable->Execute(szSQL);
   delete pTable; */
   DBMessageRead::UserDelete(pDelete->GetID(), -1);
   DBSub::UserDelete(MessageTreeSubTable(RFG_FOLDER), MessageTreeID(RFG_FOLDER), pDelete->GetID(), -1);
   DBSub::UserDelete(MessageTreeSubTable(RFG_CHANNEL), MessageTreeID(RFG_CHANNEL), pDelete->GetID(), -1);
   debuglevel(iDebug);

   if(pDelete->GetUserType() == USERTYPE_TEMP)
   {
      UserDelete(pDelete);
   }
   else
   {
      pDelete->SetDeleted(true);
   }

   // Decrease the number of users
   // pData->Root();
   // pData->Child("users");
   // pData->GetChild("numusers", &iNumUsers);
   // iNumUsers = UserCount();
   // pData->SetChild("numusers", iNumUsers - 1);

   return true;
}

bool UserItemEditDetail(EDF *pData, int iOp, UserItem *pItem, EDF *pIn, EDF *pOut, UserItem *pCurr, char *szName, bool bIsValid, bool bIsPublic, bool bIsAttachment)
{
   int iType = 0, iSetType = 0;
   char *szContentType = 0;
   bytes *pBytes = NULL;

   // EDFPrint("UserItemEditDetail input", pIn, false);

   if(pIn->Child(szName) == false)
   {
      return false;
   }

   pIn->GetChild("type", &iType);
   if(bIsAttachment == true && pIn->GetChild("content-type", &szContentType) == true)
   {
      pIn->GetChild("data", &pBytes);
   }
   else
   {
      pIn->Get(NULL, &pBytes);
   }

   if(szContentType != NULL && (strnicmp(szContentType, "image/", 6) != 0 || pBytes == NULL))
   {
      delete pBytes;
      delete[] szContentType;

      return false;
   }

   if(bIsValid == true)
   {
      iSetType |= DETAIL_VALID;
   }
   if(bIsPublic == true || mask(iType, DETAIL_PUBLIC) == true)
   {
      iSetType |= DETAIL_PUBLIC;
   }

   debug("UserItemEditDetail user %ld setting %s to %s/%p as %d\n", pItem->GetID(), szName, szContentType, pBytes, iSetType);
   pItem->SetDetail(szName, szContentType, pBytes, iSetType);

   pIn->Parent();

   delete pBytes;

   return true;
}

bool UserItemEditDetails(EDF *pData, int iOp, UserItem *pItem, EDF *pIn, EDF *pOut, UserItem *pCurr, bool bAccess)
{
   bool bLoop = false;
   char *szType = NULL, *szName = NULL;
   // EDF *pClient = pItem->GetDetailsEDF();

   // EDFPrint("UserItemEditDetails input", pIn, false);

   if(pIn->Child("details") == true)
   {
      pIn->Get(NULL, &szType);

      // printf("UserItemEditDetails entry %s\n", BoolStr(bAccess));
      // EDFPrint(pIn, false);

      /* if(pClient->Child("details") == false)
      {
      pData->Add("details");
      } */

      // EDFPrint("UserItemEditDetails pre data section", pItem->GetEDF());

      if(szType != NULL && stricmp(szType, "delete") == 0)
      {
         // MessageSpec ignore start
         /* UserItemEditDetail(pItem, pIn, 1, "email");
         UserItemEditDetail(pItem, pIn, 1, "sms"); */
         bLoop = pIn->Child();
         while(bLoop == true)
         {
            pIn->Get(&szName);
            pItem->DeleteDetail(szName);
            delete[] szName;

            bLoop = pIn->Next();
         }
         // MessageSpec ignore stop
      }
      else
      {
         UserItemEditDetail(pData, iOp, pItem, pIn, pOut, pCurr, "realname", bAccess, false, false);
         UserItemEditDetail(pData, iOp, pItem, pIn, pOut, pCurr, "email", bAccess, false, false);
         UserItemEditDetail(pData, iOp, pItem, pIn, pOut, pCurr, "sms", bAccess, false, false);
         UserItemEditDetail(pData, iOp, pItem, pIn, pOut, pCurr, "homepage", false, false, false);
         if(bAccess == true)
         {
            UserItemEditDetail(pData, iOp, pItem, pIn, pOut, pCurr, "refer", false, false, false);
            UserItemEditDetail(pData, iOp, pItem, pIn, pOut, pCurr, "picture", false, true, true);
            UserItemEditDetail(pData, iOp, pItem, pIn, pOut, pCurr, "pgpkey", false, true, false);
         }
      }

      // EDFPrint("UserItemEditDetails post data section", pItem->GetEDF());

      // pClient->Parent();
      pIn->Parent();

      delete[] szType;
   }

   return true;
}

bool UserItemEditStat(EDF *pData, int iOp, UserItem *pItem, EDF *pIn, EDF *pOut, UserItem *pCurr, bool bPeriod)
{
   if(pIn->Child(bPeriod == true ? "periodstat" : "stat") == true)
   {
      // Logins, number of messages etc
      pItem->SetStat(USERITEMSTAT_NUMLOGINS, pIn, bPeriod);
      pItem->SetStat(USERITEMSTAT_TOTALLOGINS, pIn, bPeriod);
      pItem->SetStat(USERITEMSTAT_LONGESTLOGIN, pIn, bPeriod);
      pItem->SetStat(USERITEMSTAT_NUMMSGS, pIn, bPeriod);
      pItem->SetStat(USERITEMSTAT_TOTALMSGS, pIn, bPeriod);
      pItem->SetStat(USERITEMSTAT_LASTMSG, pIn, bPeriod);

      if(pItem->GetStat(USERITEMSTAT_LONGESTLOGIN, bPeriod) > pItem->GetStat(USERITEMSTAT_TOTALLOGINS, bPeriod))
      {
         // Longest single login cannot be greater than the total login time
         pItem->SetStat(USERITEMSTAT_TOTALLOGINS, pItem->GetStat(USERITEMSTAT_LONGESTLOGIN, bPeriod), bPeriod);
      }

      pIn->Parent();
   }

   return true;
}

// UserItemEdit: Set fields for user add / edit
bool UserItemEdit(EDF *pData, int iOp, UserItem *pItem, EDF *pIn, EDF *pOut, UserItem *pCurr, ConnData *pCurrData)
{
   STACKTRACE
   int iUserType = USERTYPE_NONE, iAccessLevel = LEVEL_NONE, iOwnerID = -1, iValue = 0, iConnectionID = -1, iOldStatus = 0, iUptime = time(NULL), iMarking = 0, iStatus = LOGIN_OFF, iDebug = 0, iRuleID = 0, iArchiving = 0;
   bool bStatus = false, bLoop = false;
   char *szName = NULL, *szAgent = NULL, *szOldName = NULL;
   bytes *pStatusMsg = NULL;
#ifdef UNIX
   char *szPassword = NULL, *szCrypt = NULL;
   char szSalt[3];
#endif
   EDF *pAnnounce = NULL, *pAdmin = NULL, *pLogin = NULL, *pClient = NULL, *pPrivilege = NULL, *pFriends = NULL;
   UserItem *pChange = NULL;
   EDFConn *pItemConn = NULL;
   ConnData *pItemData = NULL;

   szOldName = pItem->GetName();

   pData->Root();
   pData->Child("system");
   pData->GetChild("uptime", &iUptime);

   // Admin only changes
   if(pCurr == NULL || pCurr->GetAccessLevel() >= LEVEL_WITNESS)
   {
      // Only admin users can change names
      if(iOp != RQ_ADD && pIn->GetChild("name", &szName) == true)
      {
         // Check validity
         if(NameValid(szName) == true)
         {
            pChange = UserGet(szName);
            // Must not match another user (or if it does must be the same user)
            if(pChange == NULL || pChange == pItem)
            {
               pItem->SetName(szName);
            }
            else
            {
               debug("UserItemEdit name '%s' in use\n", szName);
            }
         }
         else
         {
            debug("UserItemEdit invalid name change '%s'\n", szName);
         }
         delete[] szName;

         // pIn->Parent();
      }

      // Nobody can change their own access level
      if(iOp != RQ_ADD && pCurr != pItem && pIn->GetChild("accesslevel", &iAccessLevel) == true)
      {
         // pItem->SetAccessLevel(pIn);
         if(iAccessLevel < pCurr->GetAccessLevel())
         {
            pItem->SetAccessLevel(iAccessLevel);
         }
      }

      pItem->SetAccessName(pIn);
      if(pIn->GetChild("usertype", &iUserType) == true)
      {
         if(iUserType == USERTYPE_NONE || iUserType == USERTYPE_AGENT)
         {
            debug("UserItemEdit user type %d\n", iUserType);
            pItem->SetUserType(iUserType);
         }
      }

      // Check the owner
      if(pIn->GetChild("ownerid", &iOwnerID) == true)
      {
         debug("UserItemEdit ownerid %d", iOwnerID);
         if(iOwnerID == 0)
         {
            debug(". delete");
            pItem->SetOwnerID(-1);
         }
         else if(UserGet(iOwnerID) != NULL)
         {
            debug(". set");
            pItem->SetOwnerID(iOwnerID);
         }
         debug("\n");
      }

      pItem->SetCreated(pIn);

      UserItemEditStat(pData, iOp, pItem, pIn, pOut, pCurr, false);
      UserItemEditStat(pData, iOp, pItem, pIn, pOut, pCurr, true);
   }

#ifdef UNIX
   // User level changes
   if(pIn->GetChild("password", &szPassword) == true)
   {
      // Randomise a salt
      szSalt[0] = 65 + rand() % 26;
      szSalt[1] = 65 + rand() % 26;
      szSalt[2] = '\0';
      szCrypt = crypt(szPassword, szSalt);

      debug("UserItemEdit crypted password %s\n", szCrypt);
      // pData->SetChild("password", szCrypt);
      pItem->SetPassword(szCrypt);

      delete[] szPassword;
   }
#endif

   pItem->SetGender(pIn);

   pItem->SetDescription(pIn);

   UserItemEditDetails(pData, iOp, pItem, pIn, pOut, pCurr, pCurr != NULL && pCurr->GetAccessLevel() >= LEVEL_WITNESS ? true : false);

   // Login details - look for current connection
   pItemConn = ConnectionFind(CF_USERPRI, pItem->GetID());
   if(pItemConn != NULL)
   {
      pItemData = (ConnData *)pItemConn->Data();
   }

   if(iOp == RQ_EDIT && pIn->Child("login") == true)
   {
      pIn->Get(NULL, &iConnectionID);
      if(iConnectionID != -1)
      {
         // Affect a specific connection
         pItemConn = ConnectionFind(CF_CONNID, iConnectionID);
         if(pItemConn != NULL)
         {
            pItemData = (ConnData *)pItemConn->Data();
         }
      }

      if(pItemData != NULL)
      {
         // Connection found
         if(mask(pItemData->m_iStatus, LOGIN_ON) == true)
         {
            // Only allow status changes if the user is already logged in

            iOldStatus = pItemData->m_iStatus;
            iStatus = pItemData->m_iStatus;
            pIn->GetChild("status", &iStatus);

            if(mask(pItemData->m_iStatus, LOGIN_SHADOW) == false && mask(pItemData->m_iStatus, LOGIN_BUSY) == true && mask(iStatus, LOGIN_BUSY) == false)
            {
               // Busy flag off
               debug("UserItemEdit busy off\n");

               pItemData->m_iStatus -= LOGIN_BUSY;
               pItemData->m_lTimeBusy = -1;

               pItem->SetStatus(pItemData->m_iStatus);

               bStatus = true;
            }
            else if(mask(pItemData->m_iStatus, LOGIN_SHADOW) == false && mask(pItemData->m_iStatus, LOGIN_BUSY) == false && mask(iStatus, LOGIN_BUSY) == true)
            {
               // Busy flag on
               debug("UserItemEdit busy on\n");

               pItemData->m_iStatus += LOGIN_BUSY;
               pItemData->m_lTimeBusy = time(NULL);

               pItem->SetStatus(pItemData->m_iStatus);
               pItem->SetTimeBusy(pItemData->m_lTimeBusy);

               bStatus = true;
            }

            if((ConnVersion(pCurrData, "2.5") >= 0 && pIn->GetChild("statusmsg", &pStatusMsg) == true) ||
               (ConnVersion(pCurrData, "2.5") < 0 && bStatus == true && mask(pItemData->m_iStatus, LOGIN_BUSY) == true && pIn->GetChild("busymsg", &pStatusMsg) == true))
            {
               // Change status message
               delete pItemData->m_pStatusMsg;

               if(pStatusMsg != NULL)
               {
                  pItemData->m_pStatusMsg = pStatusMsg->SubString(UA_SHORTMSG_LEN);

                  delete pStatusMsg;
               }
               else
               {
                  pItemData->m_pStatusMsg = NULL;
               }

               pItem->SetStatusMsg(pItemData->m_pStatusMsg);

               bStatus = true;
            }

            if(bStatus == true)
            {
               // Only announce if status changed
               pAnnounce = new EDF();
               pAnnounce->AddChild("userid", pItem->GetID());
               pAnnounce->AddChild("username", pItem->GetName());
               pAnnounce->AddChild("gender", pItem->GetGender());
               if(pItemData->m_iStatus != iOldStatus)
               {
                  pAnnounce->AddChild("status", pItemData->m_iStatus);
               }
               if(pItemData->m_pStatusMsg != NULL)
               {
                  pAnnounce->AddChild("statusmsg", pItemData->m_pStatusMsg);
               }
               if(mask(pItemData->m_iStatus, LOGIN_BUSY) == true)
               {
                  pAnnounce->AddChild("timebusy", pItemData->m_lTimeBusy);
               }
               ServerAnnounce(pData, MSG_USER_STATUS, pAnnounce);
               delete pAnnounce;
            }
         }

         STACKTRACEUPDATE

         // Admin only changes
         if(pCurr != NULL && pCurr->GetAccessLevel() >= LEVEL_WITNESS)
         {
            pIn->GetChild("location", &pItemData->m_szLocation);

            if(pIn->GetChild("timeon", &iValue) == true)
            {
               debug("UserItemEdit timeon %d (checks at %d -> %ld)\n", iValue, iUptime, time(NULL));

               // Login time cannot be after system time
               if(iValue > time(NULL))
               {
                  iValue = time(NULL);
               }

               // Login time cannot be before uptime
               if(iValue < 0)
               {
                  iValue = iUptime;
               }
               pItemData->m_lTimeOn = iValue;
            }
         }

         if(mask(pItemData->m_iStatus, LOGIN_SHADOW) == false)
         {
            // This is the "real" login, update stored user details
            pItem->SetStatus(pItemData->m_iStatus);
            pItem->SetStatusMsg(pItemData->m_pStatusMsg);
            pItem->SetLocation(pItemData->m_szLocation);
         }
      }

      if(pCurr != NULL && pCurr->GetAccessLevel() >= LEVEL_WITNESS)
      {
         // Per user connection access
         if(pIn->Children("allow") + pIn->Children("deny") > 0)
         {
            pLogin = pItem->GetLogin(true);

            EDFParser::debugPrint("UserItemEdit pre-section login", pLogin);
            EDFSection(pLogin, pIn, "allow", RQ_ADD, "hostname", EDFElement::BYTES | EDFSECTION_NOTNULL | EDFSECTION_NOTEMPTY, "address", EDFElement::BYTES | EDFSECTION_NOTNULL | EDFSECTION_NOTEMPTY, NULL);
            EDFSection(pLogin, pIn, "allow", RQ_DELETE, "hostname", EDFElement::BYTES, "address", EDFElement::BYTES, NULL);
            EDFSection(pLogin, pIn, "deny", RQ_ADD, "hostname", EDFElement::BYTES | EDFSECTION_NOTNULL | EDFSECTION_NOTEMPTY, "address", EDFElement::BYTES | EDFSECTION_NOTNULL | EDFSECTION_NOTEMPTY, NULL);
            EDFSection(pLogin, pIn, "deny", RQ_DELETE, "hostname", EDFElement::BYTES, "address", EDFElement::BYTES, NULL);
            EDFParser::debugPrint("UserItemEdit post-section logion", pLogin);
         }
      }

      pIn->Parent();
   }

   STACKTRACEUPDATE

   if(pItem->GetUserType() == USERTYPE_AGENT)
   {
      // Only used if the user is an agent anyway
      if(pIn->Child("agent") == true)
      {
         EDFParser::debugPrint("UserItemEdit input agent", pIn, false);
         EDFParser::debugPrint("UserItemEdit agent data pre-edit", pItem->GetAgent(), false);

         // Agent client name string change by admin only
         if(pCurr != NULL && pCurr->GetAccessLevel() >= LEVEL_WITNESS)
         {
            szAgent = NULL;
            if(pIn->GetChild("name", &szAgent) == true && szAgent != NULL)
            {
               pItem->SetAgentName(szAgent);
               delete[] szAgent;
            }
         }

         // Set / delete agent data
         if(pIn->Child("data", "add") == true || pIn->Child("data", "edit") == true)
         {
            pItem->SetAgentData(pIn);
            pIn->Parent();
         }
         else if(pIn->IsChild("data", "delete") == true)
         {
            pItem->SetAgentData(NULL);
         }

         EDFParser::debugPrint("UserItemEdit agent data post-edit", pItem->GetAgent(), false);

         pIn->Parent();
      }
   }
   else if(pItemData != NULL && pIn->IsChild("client") == true)
   {
      // Check for client settings
      pClient = pItem->GetEDF();

      EDFParser::debugPrint("UserItemEdit input", pIn);
      EDFParser::debugPrint("UserItemEdit pre-section client", pClient, EDFElement::EL_CURR | EDFElement::PR_SPACE);

      EDFSection(pClient, pIn, "client", pItemData->m_szClient, RQ_ADD, NULL);
      EDFSection(pClient, pIn, "client", pItemData->m_szClient, RQ_DELETE, NULL);
      EDFSection(pClient, pIn, "client", pItemData->m_szClient, RQ_EDIT, NULL);

      EDFParser::debugPrint("UserItemEdit post-section client", pClient, EDFElement::EL_CURR | EDFElement::PR_SPACE);
      EDFParser::debugPrint("UserItemEdit whole EDF", pItem->GetEDF());
   }

   if(pIn->Child("privilege") == true)
   {
      /* Privilege section
         grants to allow agents to perform requests for other people
         allows for people to let agents perform requests for them) */

      pPrivilege = pItem->GetPrivilege(true);
      EDFParser::debugPrint("UserItemEdit privilege data pre-edit", pPrivilege, EDFElement::EL_CURR | EDFElement::PR_SPACE);

      if(pCurr != NULL && pCurr->GetAccessLevel() >= LEVEL_WITNESS)
      {
         // Only admin can grant
         EDFSection(pPrivilege, pIn, RQ_ADD, "grant", EDFElement::BYTES | EDFSECTION_NOTNULL | EDFSECTION_NOTEMPTY | EDFSECTION_MULTI, NULL);
         EDFSection(pPrivilege, pIn, RQ_DELETE, "grant", EDFElement::BYTES, NULL);
      }

      if(pCurr != NULL && (pCurr->GetAccessLevel() >= LEVEL_WITNESS || pCurr->GetOwnerID() == pItem->GetID()))
      {
         // Admin or owned user
         EDFSection(pPrivilege, pIn, RQ_ADD, "allow", EDFElement::BYTES | EDFSECTION_NOTNULL | EDFSECTION_NOTEMPTY | EDFSECTION_MULTI, NULL);
         EDFSection(pPrivilege, pIn, RQ_DELETE, "allow", EDFElement::BYTES, NULL);
      }

      EDFParser::debugPrint("UserItemEdit privilege data post-edit", pPrivilege, EDFElement::EL_CURR | EDFElement::PR_SPACE);

      pIn->Parent();
   }

   if(pIn->Child("folders") == true)
   {
      if(pIn->GetChild("marking", &iMarking) == true)
      {
         pItem->SetMarking(iMarking);
      }
      
      if(pIn->GetChild("archiving", &iArchiving) == true)
      {
	 pItem->SetArchiving(iArchiving);
      }

      if(pIn->Child("rules", "add") == true)
      {
         bLoop = pIn->Child("rule");
         while(bLoop == true)
         {
            pItem->AddRule(pIn);

            bLoop = pIn->Next("rule");
            if(bLoop == false)
            {
               pIn->Parent();
            }
         }

         pIn->Parent();
      }

      if(pIn->Child("rules", "delete") == true)
      {
         bLoop = pIn->Child("ruleid");
         while(bLoop == true)
         {
            pIn->Get(NULL, &iRuleID);
            pItem->DeleteRule(iRuleID);

            bLoop = pIn->Next("ruleid");
            if(bLoop == false)
            {
               pIn->Parent();
            }
         }
      }

      pIn->Parent();
   }

   if(pIn->Child("friends") == true)
   {
      pFriends = pItem->GetFriends(true);
      EDFParser::debugPrint("UserItemEdit friends data pre-edit", pFriends, EDFElement::EL_CURR | EDFElement::PR_SPACE);

      EDFSection(pFriends, pIn, RQ_ADD, "userid", EDFElement::INT | EDFSECTION_MULTI, NULL);

      EDFSection(pFriends, pIn, RQ_DELETE, "userid", EDFElement::INT | EDFSECTION_MULTI, NULL);

      EDFParser::debugPrint("UserItemEdit friends data post-edit", pFriends, EDFElement::EL_CURR | EDFElement::PR_SPACE);

      pIn->Parent();
   }

   if(iOp == RQ_ADD || strcmp(szOldName, pItem->GetName()) != 0)
   {
      if(iOp == RQ_ADD)
      {
         // bDebug = DBTable::Debug(true);
         iDebug = debuglevel(DEBUGLEVEL_DEBUG);
         pItem->Insert();
         // DBTable::Debug(bDebug);
         debuglevel(iDebug);
      }
      else
      {

         // bDebug = DBTable::Debug(true);
         iDebug = debuglevel(DEBUGLEVEL_DEBUG);
         pItem->Update();
         // DBTable::Debug(bDebug);
         debuglevel(iDebug);
      }

      pAnnounce = new EDF();
      pAnnounce->AddChild("userid", pItem->GetID());
      pAnnounce->AddChild("username", pItem->GetName());
      pAnnounce->AddChild("accesslevel", pItem->GetAccessLevel());
      pAnnounce->AddChild("usertype", pItem->GetUserType());

      if(pCurr != NULL)
      {
         pAdmin = new EDF();
         pAdmin->Copy(pAnnounce, true, true);
         pAdmin->AddChild("byid", pCurr->GetID());
         pAdmin->AddChild("byname", pCurr->GetName());
      }

      ServerAnnounce(pData, iOp == RQ_ADD ? MSG_USER_ADD : MSG_USER_EDIT, pAnnounce, pAdmin);

      delete pAnnounce;
      delete pAdmin;
   }

   pOut->AddChild("userid", pItem->GetID());
   pOut->AddChild("username", pItem->GetName());

   return true;
}

bool UserEmailValid(char *szEmail)
{
   int iAt = 0;
   char *szAt = NULL;

   // printf("UserEmailValid %s\n", szEmail);

   if(szEmail == NULL || strlen(szEmail) < 3)
   {
      // printf("UserEmailInvalid exit false, NULL\n");
      return false;
   }

   szAt = strchr(szEmail, '@');

   if(szAt == NULL)
   {
      // printf("UserEmailInvalid exit false, no @ sign\n");
      return false;
   }

   iAt = szAt - szEmail;

   if(iAt == 0 || iAt == strlen(szEmail) - 1)
   {
      return false;
   }

   return true;
}

bool OwnerSetDetail(EDF *pOut, UserItem *pItem, char *szName)
{
   int iType = 0;
   char *szContentType = NULL;
   bytes *pData = NULL;

   if(pOut->IsChild(szName) == false)
   {
      pItem->GetDetail(szName, &szContentType, &pData, &iType);
      debug("OwnerSetDetail %s %s %d\n", szName, pData != NULL ? pData->Data(false) : NULL, iType);

      iType |= DETAIL_OWNER;

      if(szContentType != NULL)
      {
         pOut->Add(szName);
         pOut->AddChild("type", iType);
         pOut->AddChild("content-type", szContentType);
         pOut->AddChild("data", pData);
         pOut->Parent();
      }
      else if(pData != NULL)
      {
         pOut->Add(szName, pData);
         pOut->AddChild("type", iType);
         pOut->Parent();
      }
   }

   return false;
}

bool UserItemListSubs(EDF *pOut, UserItem *pItem, int iLevel, int iBase, char *szSection, DBSub *pSubs, int iUserLevel, DBSub *pUserSubs, bool bSubCheck)
{
   STACKTRACE
   int iSubNum = 0, iID = 0, iSubType = 0, iExtra = 0, iUserSub = 0;
   bool bSection = false;
   char *szName = NULL;
   // char szSQL[200];
   // DBTable *pTable = NULL;
   MessageTreeItem *pTree = NULL;

   if(pOut->Child(szSection) == true)
   {
      bSection = true;
   }

   if(pSubs != NULL)
   {
      STACKTRACEUPDATE

      for(iSubNum = 0; iSubNum < pSubs->Count(); iSubNum++)
      {
         iID = pSubs->ItemID(iSubNum);
         iSubType = pSubs->ItemSubType(iSubNum);
         iExtra = pSubs->ItemExtra(iSubNum);

         pTree = MessageTreeGet(iBase, iID, &szName);
         if(pTree != NULL)
         {
            if(pUserSubs != NULL)
            {
               iUserSub = pUserSubs->SubType(pTree->GetID());
            }
            iUserSub = MessageTreeAccess(pTree, ACCMODE_SUB_READ, ACCMODE_MEM_READ, iUserLevel, iUserSub, bSubCheck);

            if(mask(iLevel, EDFITEMWRITE_ADMIN) == true ||
               (iSubType == SUBTYPE_EDITOR && iUserSub >= SUBTYPE_SUB) ||
               (iSubType == SUBTYPE_MEMBER && iUserSub >= SUBTYPE_MEMBER))
            {
               if(bSection == false)
               {
                  pOut->Add(szSection);
                  bSection = true;
               }
               pOut->Add(SubTypeStr(iSubType), iID);
               pOut->AddChild("name", szName);
               if(iExtra > 0)
               {
                  if(iBase == RFG_FOLDER)
                  {
                     pOut->AddChild("priority", iExtra);
                  }
                  else
                  {
                     pOut->AddChild("active", true);
                  }
               }
               pOut->Parent();
            }
         }
      }
   }

   // EDFPrint("UserItemListSubs EDF", pOut, false);
   if(bSection == true)
   {
      pOut->Parent();
   }

   return bSection;
}

bool UserItemListPicture(EDF *pOut, UserItem *pItem, int iLevel, ConnData *pConnData)
{
   STACKTRACE
   int iType = 0;
   bool bReturn = false;
   char *szContentType = 0;
   bytes *pContent = NULL;

   /* if(pEDF->Child("details") == true)
   {
      if(pEDF->Child("picture") == true)
      {
         bReturn = true;

         pEDF->Parent();
      }

      pEDF->Parent();
   } */

   // if(bReturn == false)
   {
      if(pItem->GetDetail("picture", &szContentType, &pContent, &iType) == true)
      {
         debug(DEBUGLEVEL_DEBUG, "UserListPicture content '%s' %p(%ld) %d\n", szContentType, pContent, pContent != NULL ? pContent->Length() : -1, iType);

         if(mask(iLevel, EDFITEMWRITE_ADMIN) == true || mask(iType, DETAIL_PUBLIC) == true)
         {
            if(szContentType == NULL)
            {
               if(pOut->Child("details") == false)
               {
                  pOut->Add("details");
               }

               pOut->AddChild("picture", pContent);

               pOut->Parent();
            }
            else if(pConnData->m_lAttachmentSize == -1 || pConnData->m_lAttachmentSize >= pContent->Length())
            {
               // bytesprint("UserListPicture content", pContent, true);

               if(pOut->Child("details") == false)
               {
                  pOut->Add("details");
               }

               pOut->Add("picture");
               pOut->AddChild("type", iType);
               pOut->AddChild("content-type", szContentType);
               pOut->AddChild("data", pContent);

               pOut->Parent();

               pOut->Parent();
            }
         }
      }
      else
      {
         // printf("UserListPicture user %ld no picture data\n", pItem->GetID());
      }
   }

   return bReturn;
}

// UserItemList: Copy user details into output EDF. Detail types are:
//    0 - ID, name, access level
//    1 - Above + login details
//    2 - Above + description, stats and details
//    3 - Above + folder / channel details
bool UserItemList(EDF *pOut, UserItem *pItem, int iLevel, int iLogins, EDFConn *pItemConn, int iNumShadows, DBSub *pFolders, DBSub *pChannels, DBSub *pServices, EDFConn *pConn, int iSystemIdle)
{
   STACKTRACE
   int iListNum = 0, iTimeOn = -1, iTimeOff = -1, iFriendID = -1, iUserLevel = LEVEL_NONE;
   bool bLoop = false;
#ifdef UNIX
   char szSQL[200];
#endif
   char *szOwnerName = NULL, *szClient = NULL, *szFriendName = NULL;
   DBTable *pTable = NULL;
   UserItem *pOwner = NULL;
   ConnData *pConnData = CONNDATA, *pItemData = NULL;

   // printf("UserItemList %p %p %d %d %p %d %p %p %p %p %d\n", pOut, pItem, iLevel, iLogins, pItemConn, iNumShadows, pFolders, pChannels, pServices, pConn, iSystemIdle);

   /* if(bAdd == false)
   {
      iLevel |= USERWRITE_NOADD;
   } */

   if(pItemConn != NULL)
   {
      pItemData = (ConnData *)pItemConn->Data();
   }

   if(mask(iLevel, USERITEMWRITE_SELF) == true && pConnData->m_szClient != NULL)
   {
      szClient = pConnData->m_szClient;
   }

   STACKTRACEUPDATE
   pItem->Write(pOut, iLevel, ConnVersion(pConnData, "2.5") >= 0 ? "statusmsg" : "busymsg", szClient);
   STACKTRACEUPDATE

   // EDFPrint("UserItemList post item write", pOut, EDFElement::EL_CURR | EDFElement::PR_SPACE);

   if(pOut->IsChild("ownerid") == true)
   {
      pOwner = UserGet(pItem->GetOwnerID(), &szOwnerName, false, -1);
      if(pOwner != NULL)
      {
         pOut->AddChild("ownername", szOwnerName);
      }
   }

   STACKTRACEUPDATE

   if(pOut->Child("friends") == true)
   {
      bLoop = pOut->Child("userid");
      while(bLoop == true)
      {
         pOut->Get(NULL, &iFriendID);
         if(UserGet(iFriendID, &szFriendName, false) != NULL)
         {
            pOut->AddChild("name", szFriendName);
         }

         bLoop = pOut->Next("userid");
         if(bLoop == false)
         {
            pOut->Parent();
         }
      }

      pOut->Parent();
   }

   STACKTRACEUPDATE

   if(iLogins > 0)
   {
      if(pItemData != NULL)
      {
         ConnectionInfo(pOut, pItemConn, iLevel + iLogins, pItemConn == pConn);
      }
      else
      {
         for(iListNum = 0; iListNum < ConnCount(); iListNum++)
         {
            STACKTRACEUPDATE

            pItemConn = ConnList(iListNum);
            pItemData = (ConnData *)pItemConn->Data();

            if(pItemData->m_pUser != NULL && pItemData->m_pUser->GetID() == pItem->GetID() && ConnectionVisible(pItemData, pConnData != NULL ? pConnData->m_pUser->GetID() : -1, pConnData != NULL ? pConnData->m_pUser->GetAccessLevel() : LEVEL_NONE) == true)
            {
               ConnectionInfo(pOut, pItemConn, iLevel + iLogins, pItemConn == pConn);
            }
         }
      }
   }

   STACKTRACEUPDATE

   if(iNumShadows > 0)
   {
      pOut->AddChild("numshadows", iNumShadows);
   }

   // EDFPrint("UserItemList added", pOut, false);

   STACKTRACEUPDATE

   if(mask(iLevel, USERITEMWRITE_DETAILS) == true)
   {
      if(pOwner != NULL && mask(iLevel, EDFITEMWRITE_ADMIN) == true)
      {
         if(pOut->Child("details") == false)
         {
            pOut->Add("details");
         }
         OwnerSetDetail(pOut, pOwner, "realname");
         OwnerSetDetail(pOut, pOwner, "email");
         // OwnerSetDetail(pOut, pOwner, "homepage");
         pOut->Parent();
      }

      if(mask(iLevel, ULS_FULL) == true)
      {
         if(pConnData != NULL && pConnData->m_pUser != NULL)
         {
            iUserLevel = pConnData->m_pUser->GetAccessLevel();
         }

         UserItemListSubs(pOut, pItem, iLevel, RFG_FOLDER, "folders", pFolders, iUserLevel, pConnData != NULL ? pConnData->m_pFolders : NULL, false);
         UserItemListSubs(pOut, pItem, iLevel, RFG_CHANNEL, "channels", pChannels, iUserLevel, pConnData != NULL ? pConnData->m_pChannels : NULL, true);
         if(mask(iLevel, EDFITEMWRITE_ADMIN) == true)
         {
            UserItemListSubs(pOut, pItem, iLevel, RFG_SERVICE, "services", pServices, iUserLevel, NULL, false);

#ifdef UNIX
            pTable = new DBTable();

            pTable->BindColumnInt();
            pTable->BindColumnInt();

            sprintf(szSQL, "select timeon, timeoff from user_login where userid = %ld order by timeon desc limit 5", pItem->GetID());
            if(pTable->Execute(szSQL) == true)
            {
               while(pTable->NextRow() == true)
               {
                  pTable->GetField(0, &iTimeOn);
                  pTable->GetField(1, &iTimeOff);

                  // printf("UserItemList old login %d %d\n", iTimeOn, iTimeOff);

                  pOut->Add("oldlogin");
                  pOut->AddChild("timeon", iTimeOn);
                  pOut->AddChild("timeoff", iTimeOff);
                  pOut->Parent();
               }
            }

            delete pTable;
#endif
         }
      }
   }

   STACKTRACEUPDATE

   if((mask(iLevel, ULS_FULL) == true || mask(iLevel, ULS_PICTURE) == true) && pConnData != NULL)
   {
      UserItemListPicture(pOut, pItem, iLevel, pConnData);
   }

   STACKTRACEUPDATE

   // EDFPrint("UserItemList exit", pOut, EDFElement::EL_CURR | EDFElement::PR_SPACE);

   pOut->Parent();

   return true;
}

// UserLoginFail: Decrease the attempt count on this connection
void UserLoginFail(EDFConn *pConn, EDF *pData, EDF *pOut)
{
   STACKTRACE
   ConnData *pConnData = CONNDATA;

   pConnData->m_iLoginAttempts--;
   pOut->AddChild("attempts", pConnData->m_iLoginAttempts);
   if(pConnData->m_iLoginAttempts <= 0)
   {
      ConnectionShut(pConn, pData);
   }
}

void UserLogout(EDF *pData, UserItem *pItem, ConnData *pConnData, bytes *pText)
{
   STACKTRACE
   int iUserID = 0, iTimeOn = 0, iTimeOff = time(NULL);
   DBTable *pTable = NULL;

   iUserID = pItem->GetID();
   // iTimeOn = pItem->GetTimeOn();
   iTimeOn = pConnData->m_lTimeOn;

   debug("UserLogout entry %p %p %p %s, %d (%d)\n", pData, pItem, pConnData, pText != NULL ? pText->Data(false) : NULL, iTimeOn, iTimeOff - iTimeOn);

   debug("UserLogout new\n");
   pTable = new DBTable();

   debug("UserLogout set fields\n");
   pTable->SetField(iUserID);
   pTable->SetField(iTimeOn);
   pTable->SetField(iTimeOff);
   pTable->SetField(pText);

   debug("UserLogout table insert\n");
   pTable->Insert("user_login");

   debug("UserLogout delete\n");
   delete pTable;

   STACKTRACEUPDATE

   // Turn login flag off
   if(mask(pConnData->m_iStatus, LOGIN_SHADOW) == false && mask(pConnData->m_iStatus, LOGIN_SILENT) == false)
   {
      debug("UserLogout time off %d\n", iTimeOff);

      pItem->SetStatus(LOGIN_OFF);
      pItem->SetTimeOff(iTimeOff);
   }

   iTimeOn = iTimeOff - iTimeOn;

   debug("UserLogout total logins %ld / %ld + %d\n", pItem->GetStat(USERITEMSTAT_TOTALLOGINS, false), pItem->GetStat(USERITEMSTAT_TOTALLOGINS, true), iTimeOn);
   pItem->IncStat(USERITEMSTAT_TOTALLOGINS, iTimeOn);
   if(iTimeOn > pItem->GetStat(USERITEMSTAT_LONGESTLOGIN, true))
   {
      pItem->SetStat(USERITEMSTAT_LONGESTLOGIN, iTimeOn, true);
   }
   if(iTimeOn > pItem->GetStat(USERITEMSTAT_LONGESTLOGIN, false))
   {
      pItem->SetStat(USERITEMSTAT_LONGESTLOGIN, iTimeOn, false);
   }

   debug("UserLogout exit\n");
}

// UserAccess: Generic access check for user-related  requests
bool UserAccess(UserItem *pCurr, EDF *pIn, char *szField, bool bField, UserItem **pItem, EDF *pOut)
{
   int iUserID = -1;
   UserItem *pCheck = NULL, *pOwner = NULL;

   if(szField == NULL)
   {
      // No field specified, default to current user
      *pItem = pCurr;

      debug("UserAccess true, NULL field name OK\n");
      return true;
   }

   if(pIn->GetChild(szField, &iUserID) == true)
   {
      *pItem = UserGet(iUserID);
      if(*pItem == NULL)
      {
         // Cannot find request uesr
         pOut->Set("reply", MSG_USER_NOT_EXIST);
         pOut->AddChild("userid", iUserID);

         debug("UserAccess false, %s %d\n", MSG_USER_NOT_EXIST, iUserID);
         return false;
      }
   }
   else
   {
      if(bField == true)
      {
         // No field value
         *pItem = NULL;

         pOut->Set("reply", MSG_USER_NOT_EXIST);

         debug("UserAccess false, %s\n", MSG_USER_NOT_EXIST);
         return false;
      }
      else
      {
         // No field value required, default to current user
         *pItem = pCurr;

         debug("UserAccess true, no field value OK\n");
         return true;
      }
   }

   if(pCurr == *pItem)
   {
      // User is the current one
      debug("UserAccess true, current user\n");
      return true;
   }

   // Additional checking

   pCheck = *pItem;

   if(pCurr->GetAccessLevel() >= LEVEL_WITNESS)
   {
      if(pCheck->GetAccessLevel() >= LEVEL_WITNESS && pCheck->GetAccessLevel() >= pCurr->GetAccessLevel())
      {
         // Must be above request user's level for admin users
         pOut->Set("reply", MSG_ACCESS_INVALID);

         debug("UserAccess false, witness check failed (check %d > curr %d)\n", pCheck->GetAccessLevel(), pCurr->GetAccessLevel());
         return false;
      }
      else
      {
         debug("UserAccess true, witness check OK\n");
         return true;
      }
   }

   if(pCheck->GetOwnerID() > 0)
   {
      pOwner = UserGet(pCheck->GetOwnerID());
      if(pOwner == pCurr)
      {
         // Current user owns request user

         debug("UserAccess false, owner check Ok %ld / %ld -vs- %p\n", pCurr->GetID(), pCheck->GetOwnerID(), pOwner);
         return true;
      }
      else
      {
         // Owner not found or owner not current user
         pOut->Set("reply", MSG_ACCESS_INVALID);

         debug("UserAccess false, owner check failed %ld / %ld -vs- %p\n", pCurr->GetID(), pCheck->GetOwnerID(), pOwner);
         return false;
      }
   }

   pOut->Set("reply", MSG_ACCESS_INVALID);

   debug("UserAccess false\n");
   return false;
}

// EDF message functions

extern "C"
{

/*
** UserAdd: Add a user
**
** name(required):   Name of user
*/
ICELIBFN bool UserAdd(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   STACKTRACE
   int iAccessLevel = LEVEL_GUEST, iMaxID = 0;
   char *szName = NULL;//, *szClient = NULL;
   UserItem *pCurr = CONNUSER, *pItem = NULL;

   if(pIn->GetChild("name", &szName) == false)
   {
      // No name field
      pOut->Set("reply", MSG_USER_INVALID);

      debug("UserItemAdd exit false, %s\n", MSG_USER_INVALID);
      return false;
   }

   if(NameValid(szName) == false)
   {
      // Invalid name
      pOut->Set("reply", MSG_USER_INVALID);
      pOut->AddChild("name", szName);

      delete[] szName;

      debug("UserItemAdd exit false, %s\n", MSG_USER_INVALID);
      return false;
   }

   pItem = UserGet(szName);
   if(pItem != NULL)
   {
      // User already exists
      pOut->Set("reply", MSG_USER_EXIST);
      pOut->AddChild("userid", pItem->GetID());
      pOut->AddChild("username", pItem->GetName());

      delete[] szName;

      debug("UserItemAdd exit false, %s\n", MSG_USER_EXIST);
      return false;
   }

   if(pCurr != NULL)
   {
      if(pCurr->GetAccessLevel() >= LEVEL_WITNESS)
      {
         pIn->GetChild("accesslevel", &iAccessLevel);
         if(iAccessLevel > pCurr->GetAccessLevel())
         {
            pIn->SetChild("accesslevel", pCurr->GetAccessLevel() - 1);
         }
      }
   }

   /* if(pIn->Child("client") == true)
   {
      pIn->Get(NULL, &szClient);

      if(szClient != NULL)
      {
         pItem->SetClient(szClient);
         pIn->Set(NULL, "add");

         delete[] szClient;
      }
      else
      {
         pIn->Delete();
      }
   } */

   iMaxID = UserMaxID();
   iMaxID++;

   pItem = new UserItem(iMaxID);
   // debug("UserAdd UserItem class %s\n", pItem->GetClass());
   UserAdd(pItem);

   pItem->SetName(szName);
   pItem->SetAccessLevel(iAccessLevel);

   UserItemEdit(pData, RQ_ADD, pItem, pIn, pOut, pCurr, NULL);

   // delete[] szName;

   return true;
}



/*
** UserDelete: Delete a user
*/
ICELIBFN bool UserDelete(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   STACKTRACE
   int iAccessLevel = LEVEL_NONE, iByID = 0, iUserID = 0, iListNum = 0;
   char *szName = NULL, *szBy = NULL;
   UserItem *pCurr = CONNUSER, *pDelete = NULL, *pItem = NULL;
   EDFConn *pItemConn = NULL;
   ConnData *pItemData = NULL;

   debug("UserDelete entry\n");
   iByID = pCurr->GetID();
   szBy = pCurr->GetName();
   iAccessLevel = pCurr->GetAccessLevel();

   if(UserAccess(pCurr, pIn, "userid", true, &pDelete, pOut) == false)
   {
      return false;
   }

   if(pDelete->GetDeleted() == false)
   {
      for(iListNum = 0; iListNum < ConnCount(); iListNum++)
      {
         pItemConn = ConnList(iListNum);
         pItemData = (ConnData *)pItemConn->Data();
         pItem = pItemData->m_pUser;
         if(pItem == pDelete)
         {
            ConnectionShut(pItemConn, pData, pCurr, false);
         }
      }

      UserDelete(pData, pCurr, pDelete);
   }

   // Set the reply
   pOut->AddChild("userid", iUserID);
   pOut->AddChild("username", szName);

   // delete[] szBy;
   // delete[] szName;

   debug("UserDelete exit true\n");
   return true;
}

/*
** UserEdit: Edit user details
**
** userid(admin): ID of user to edit (always ID of current user for non-admin)
*/
ICELIBFN bool UserEdit(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   STACKTRACE
   UserItem *pCurr = CONNUSER, *pItem = NULL;
   ConnData *pConnData = CONNDATA;

   if(UserAccess(pCurr, pIn, "userid", false, &pItem, pOut) == false)
   {
      return false;
   }

   EDFParser::debugPrint("UserEdit", pIn);

   UserItemEdit(pData, RQ_EDIT, pItem, pIn, pOut, pCurr, pConnData);

   debug("UserEdit exit true\n");
   return true;
}

/*
** UserList: Generate a list of users
**
** searchtype: Type of list to generate
**    0(default) - Specified users, low details (ID, name, access level, user type)
**    1 - All logged in users, medium details (above + login details)
**    2 - Specified users, medium details
**    3 - Specified users, high details
**    4 - Current user
**    5 - Idle users
**
** Specifying a user ID will give complete details for that user only (equivalent searchtype 4)
** Idle users was added in v2.7-beta2 so a translation is done
*/
ICELIBFN bool UserList(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   STACKTRACE
   int iType = 0, iAccessLevel = LEVEL_NONE, iCurrUser = -1, iUserID = -1, iLevel = 0, iNumShadows = 0;
   int iListLowest = LEVEL_NONE, iListHighest = LEVEL_SYSOP, iNumUsers = 0, iFound = 0, iLoopLevel = LEVEL_NONE, iSystemIdle = 0;
   int iListNum = 0, iUserType = -1, iLogins = 0, iLevelSingle = 0, iLoginsSingle = 0;
   bool bAdd = false, bLoop = true, bAdmin = false;
   char *szListName = NULL, *szLoopName = NULL;
   UserItem *pCurr = CONNUSER, *pItem = NULL;
   DBSub *pItemFolders = NULL, *pItemChannels = NULL, *pItemServices = NULL;
   ConnData *pConnData = CONNDATA, *pItemData = NULL;
   EDFConn *pItemConn = NULL;

   if(pCurr != NULL)
   {
      // pData->Get(NULL, &iCurrUser);
      iCurrUser = pCurr->GetID();
      // pData->GetChild("accesslevel", &iAccessLevel);
      iAccessLevel = pCurr->GetAccessLevel();
      if(iAccessLevel >= LEVEL_WITNESS)
      {
         bAdmin = true;
      }
      else
      {
         bAdmin = PrivilegeValid(pCurr, MSG_USER_LIST);
      }
   }

   // printf("UserList, %d %d\n", iCurrUser, iAccessLevel);
   // EDFPrint("UserList entry", pIn);

   // Get system values
   pData->Root();
   pData->Child("system");
   pData->GetChild("idletime", &iSystemIdle);
   pData->Parent();
   // pData->Child("users");
   // pData->GetChild("numusers", &iNumUsers);
   iNumUsers = UserCount();

   if(pIn->GetChild("userid", &iUserID) == true)
   {
      // Details for a specific user
      debug("UserList entry, user ID %d\n", iUserID);

      // if(UserGet(pData, iUserID) == false)
      pItem = UserGet(iUserID);
      if(pItem == NULL || (pItem->GetDeleted() == true && bAdmin == false))
      {
         pOut->Set("reply", MSG_USER_NOT_EXIST);
         pOut->AddChild("userid", iUserID);

         return false;
      }

      // pConn->TempMark();

      pItemConn = ConnectionFind(CF_USERID, pItem->GetID());
      if(pItemConn != NULL)
      {
         pItemData = (ConnData *)pItemConn->Data();
         pItemFolders = pItemData->m_pFolders;
         pItemChannels = pItemData->m_pChannels;
         pItemServices = pItemData->m_pServices;

         iLogins |= USERITEMWRITE_LOGIN;
      }
      else
      {
         pItemFolders = new DBSub(MessageTreeSubTable(RFG_FOLDER), MessageTreeID(RFG_FOLDER), pItem->GetID());
         pItemChannels = new DBSub(MessageTreeSubTable(RFG_CHANNEL), MessageTreeID(RFG_CHANNEL), pItem->GetID(), SUBTYPE_MEMBER, 0);
         pItemServices = new DBSub(SERVICE_SUB, "serviceid", pItem->GetID(), 0, 0);

         iLevel |= USERITEMWRITE_LOGIN;
      }

      if(bAdmin == true || pItem->GetID() == iCurrUser || (pItem->GetOwnerID() != -1 && iCurrUser == pItem->GetOwnerID()))
      {
         iLevel |= EDFITEMWRITE_ADMIN;
      }

      // printf("UserList login section check %d, %s / %s\n", iLevel, BoolStr(mask(iLevel, USERWRITE_LOGIN)), BoolStr(mask(iLevel, USERWRITE_NOSTATUS)));

      UserItemList(pOut, pItem, iLevel + USERITEMWRITE_DETAILS + ULS_FULL, iLogins, NULL, 0, pItemFolders, pItemChannels, pItemServices, pConn, iSystemIdle);

      if(pItemFolders != NULL && pItemData == NULL)
      {
         delete pItemFolders;
      }
      if(pItemChannels != NULL && pItemData == NULL)
      {
         delete pItemChannels;
      }

      iFound = 1;
      // iType = 4;
   }
   else
   {
      // Map between search type and detail level
      pIn->GetChild("searchtype", &iType);
      if(iType < 0 || iType > 5)
      {
         iType = 0;
      }
      if(iType == 5)
      {
         iType = 2;
      }
      else if(iType >= 2)
      {
         iType++;
      }

      if(iType == UL_CURR)
      {
         // Details for this user

         UserItemList(pOut, pCurr, USERITEMWRITE_SELF + EDFITEMWRITE_ADMIN + USERITEMWRITE_DETAILS + ULS_FULL, USERITEMWRITE_LOGIN, NULL, 0, pConnData->m_pFolders, pConnData->m_pChannels, pConnData->m_pServices, pConn, iSystemIdle);

         iFound = 1;
      }
      else
      {
         if(iType >= UL_HIGH)
         {
            iLevel |= USERITEMWRITE_DETAILS;
         }
         if(bAdmin == true)
         {
            iLevel |= EDFITEMWRITE_ADMIN;
         }

         debug("UserList entry, type %d", iType);
         if(!UL_IS_CONN(iType))
         {
            // Check further search parameters
            if(pIn->GetChild("name", &szListName) == true)
            {
               pOut->AddChild("name", szListName);
               debug(", name %s", szListName);
            }
            if(pIn->GetChild("lowest", &iListLowest) == true)
            {
               pOut->AddChild("lowest", iListLowest);
               debug(", lowest %d", iListLowest);
            }
            if(pIn->GetChild("highest", &iListHighest) == true)
            {
               pOut->AddChild("highest", iListHighest);
               debug(", highest %d", iListHighest);
            }
            pIn->GetChild("usertype", &iUserType);
         }
         else
         {
            iLevel |= ULS_PICTURE;
         }
         debug(", level %d\n", iLevel);

         if(UL_IS_CONN(iType))
         {
            if(ConnCount() == 0)
            {
               bLoop = false;
            }
         }

         while(bLoop == true)
         {
            if(UL_IS_CONN(iType))
            {
               pItemConn = ConnList(iListNum);
               pItemData = (ConnData *)pItemConn->Data();
               pItem = pItemData->m_pUser;
               iNumShadows = 0;
            }
            else
            {
               pItem = UserList(iListNum);

               pItemConn = ConnectionFind(CF_USERPRI, pItem->GetID());
               iNumShadows = ConnectionShadows(pItem->GetID(), iCurrUser, iAccessLevel);
            }

            if(pItem != NULL)
            {
               bAdd = false;
               if(bAdmin == true || pItem->GetDeleted() == false)
               {
                  if(UL_IS_CONN(iType))
                  {
                     if(ConnectionVisible(pItemData, iCurrUser, iAccessLevel) == true &&
                        (iType != UL_IDLE || UserItem::IsIdle(pItemData->m_lTimeIdle) == false))
                     {
                        bAdd = true;
                     }
                  }
                  else
                  {
                     szLoopName = pItem->GetName();
                     iLoopLevel = pItem->GetAccessLevel();
                     /* if(iUserType != -1)
                     {
                        debug("UserList check %ld %s %d\n", pItem->GetID(), szLoopName, pItem->GetUserType());
                     } */
                     if((szListName == NULL || strmatch(szLoopName, szListName, true, true, true) == true) &&
                        (iLoopLevel >= iListLowest && iLoopLevel <= iListHighest) &&
                        (iUserType == -1 || mask(pItem->GetUserType(), iUserType) == true))
                     {
                        bAdd = true;
                     }
                  }

                  if(bAdd == true)
                  {
                     // Add the current user to the output EDF
                     iFound++;

                     iLevelSingle = iLevel;
                     iLoginsSingle = iLogins;
                     if(pItemConn != NULL)
                     {
                        if(iType == UL_IDLE)
                        {
                        }
                        else if(iType >= UL_LOGIN)
                        {
                           iLoginsSingle |= USERITEMWRITE_LOGIN;
                        }
                        else
                        {
                           iLoginsSingle |= USERITEMWRITE_LOGINFLAT;
                        }
                     }
                     else
                     {
                        if(iType >= UL_MED)
                        {
                           iLevelSingle |= USERITEMWRITE_LOGIN;
                        }
                        else
                        {
                           iLevelSingle |= USERITEMWRITE_LOGINFLAT;
                        }
                     }
                     if(pCurr != NULL && (pCurr == pItem || pCurr->GetID() == pItem->GetOwnerID()))
                     {
                        iLevelSingle |= EDFITEMWRITE_ADMIN;
                     }

                     STACKTRACEUPDATE

                     UserItemList(pOut, pItem, iLevelSingle, iLoginsSingle, pItemConn, iNumShadows, NULL, NULL, NULL, pConn, iSystemIdle);

                     STACKTRACEUPDATE
                  }
               }
            }
            else if(iType == UL_LOGIN && iAccessLevel >= LEVEL_WITNESS)
            {
               STACKTRACEUPDATE

               pOut->Add("connection", pItemConn->ID());
               ConnectionInfo(pOut, pItemConn, EDFITEMWRITE_ADMIN + USERITEMWRITE_LOGIN, false);
               pOut->Parent();

               iFound++;

               STACKTRACEUPDATE
            }

            STACKTRACEUPDATE

            // bLoop = pData->Next("user");
            iListNum++;
            if(UL_IS_CONN(iType) && iListNum >= ConnCount())
            {
               bLoop = false;
            }
            else if(!UL_IS_CONN(iType) && iListNum >= UserCount())
            {
               bLoop = false;
            }
         }

         STACKTRACEUPDATE

         delete[] szListName;
      }
   }

   STACKTRACEUPDATE

   if(iType > 0)
   {
      if(iType == UL_IDLE)
      {
         iType = 5;
      }
      else if(iType >= UL_MED)
      {
         iType--;
      }
      pOut->AddChild("searchtype", iType);
   }
   pOut->AddChild("numusers", iNumUsers);
   pOut->AddChild("found", iFound);
   pOut->AddChild("systemtime", (int)(time(NULL))); // MessageSpec INT
   pOut->AddChild("idletime", iSystemIdle);

   // printf("UserList exit true, %d %d\n", iNumUsers, iFound);
   // EDFPrint(NULL, pOut);
   return true;
}

/*
** UserLogin: Login as a user
*/
ICELIBFN bool UserLogin(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   STACKTRACE
   int iStatus = 0, iAttachmentSize = 0, iDebug = 0;
   int iMaxLogins = 0, iCurrLogins = 0, iUserType = USERTYPE_NONE, iPosition = 0, iListNum = 0;
   bool bForce = false, bLoop = true, bClientUpgrade = true, bDelete = false, bValid = true;
   unsigned long lHostname = 0, lAddress = 0;
   char *szRequest = NULL, *szName = NULL, *szPassword = NULL, *szDataPW = NULL;
   char *szClientBase = NULL, *szAgent = NULL, *szHostname = NULL, *szAddress = NULL;
   bytes *pStatusMsg = NULL;
   EDF *pAnnounce = NULL, *pAdmin = NULL, *pClient = NULL, *pGet = NULL;
   EDFElement *pElement = NULL;
   EDFConn *pItemConn = NULL;
   ConnData *pConnData = CONNDATA, *pItemData = NULL;
   UserItem *pCurr = CONNUSER;

   pIn->Get(NULL, &szRequest);

   if(stricmp(szRequest, MSG_USER_LOGIN) == 0)
   {
      pIn->GetChild("status", &iStatus);
      pIn->GetChild("usertype", &iUserType);
   }
   if(mask(iUserType, USERTYPE_TEMP) == false)
   {
      pIn->GetChild("name", &szName);
      bForce = pIn->GetChildBool("force");
   }

   debug("UserLogin entry %s %s %d %d, %p\n", szRequest, szName, iStatus, iUserType, pConnData);
   if(stricmp(szRequest, MSG_USER_LOGIN_QUERY) == 0)
   {
      EDFParser::debugPrint(pIn);
   }

   if(stricmp(szRequest, MSG_USER_LOGIN_QUERY) == 0 && szName != NULL && PrivilegeValid(pCurr, MSG_USER_LOGIN_QUERY) == false)
   {
      pOut->Set("reply", MSG_ACCESS_INVALID);

      delete[] szRequest;

      debug("UserLogin exit false, %s\n", MSG_ACCESS_INVALID);
      return false;
   }

   // Check this host can be trusted to supply hostname / address
   if(ConnectionTrust(pConn, pData) == true)
   {
      debug("UserLogin trusted hostname %s\n", pConn->Hostname());
      pIn->GetChild("hostname", &szHostname);
      pIn->GetChild("address", &szAddress);
   }

   if(mask(iUserType, USERTYPE_TEMP) == true)
   {
      // Create a temporary user
      pCurr = UserAdd(NULL, NULL, LEVEL_NONE, USERTYPE_TEMP);

      szName = pCurr->GetName();
      // iAccessLevel = pCurr->GetAccessLevel();
   }
   else
   {
      // Find the user

      pCurr = UserGet(szName);

      if(pCurr == NULL || pCurr->GetDeleted() == true)
      {
         // Trying to login as a deleted user
         pOut->Set("reply", MSG_USER_LOGIN_INVALID);
         pOut->AddChild("name", szName);

         if(stricmp(szRequest, MSG_USER_LOGIN) == 0)
         {
            UserLoginFail(pConn, pData, pOut);
         }

         delete[] szName;
         delete[] szRequest;

         debug("UserLogin exit false, %s (no such user)\n", MSG_USER_LOGIN_INVALID);
         return false;
      }

      // Get the properly cased name
      delete[] szName;
      szName = pCurr->GetName();

      // iAccessLevel = pCurr->GetAccessLevel();
      // iUserType = pCurr->GetUserType();

      STACKTRACEUPDATE

#ifdef UNIX
      // Check the password
      char szSalt[3], szInPW[256];

      strcpy(szInPW, "");

      pIn->GetChild("password", &szPassword);
      szDataPW = pCurr->GetPassword();
      if(szDataPW != NULL && strlen(szDataPW) >= 2 && szPassword != NULL)
      {
         // if(pIn->GetChildBool("crypt") == false)
         {
            szSalt[0] = szDataPW[0];
            szSalt[1] = szDataPW[1];
            szSalt[2] = '\0';

            strcpy(szInPW, crypt(szPassword, szSalt));
         }
         /* else
         {
            strcpy(szInPW, szPassword);
         } */
      }
      else
      {
	 // Not a good password, deny access
	    
	 pOut->Set("reply", MSG_USER_LOGIN_INVALID);
	 pOut->AddChild("name", szName);
	   
	 delete[] szRequest;
	 
	 debug("UserLogin exit false, %s (bad password stored)\n", MSG_USER_LOGIN_INVALID);
	 return false;
      }

      // printf("UserLogin passwords %s %s\n", szPassword, szDataPW);

      delete[] szPassword;

      if(szDataPW != NULL && strlen(szDataPW) >= 2 && strcmp(szInPW, szDataPW) != 0)
      {
         // Passwords don't match
         pOut->Set("reply", MSG_USER_LOGIN_INVALID);
         pOut->AddChild("name", szName);

         if(stricmp(szRequest, MSG_USER_LOGIN) == 0)
         {
            if(pCurr->GetAccessLevel() >= LEVEL_WITNESS)
            {
               pAdmin = new EDF();
               pAdmin->AddChild("userid", pCurr->GetID());
               pAdmin->AddChild("username", szName);
               ConnectionInfo(pAdmin, pConn, EDFITEMWRITE_ADMIN + USERITEMWRITE_LOGIN + USERITEMWRITE_LOGINFLAT, false);
               ServerAnnounce(pData, MSG_USER_LOGIN_INVALID, NULL, pAdmin);
               delete pAdmin;
            }

            UserLoginFail(pConn, pData, pOut);
         }

         delete[] szRequest;

         debug("UserLogin exit false, %s (invalid password)\n", MSG_USER_LOGIN_INVALID);
         return false;
      }

      // delete[] szDataPW;
#endif

      STACKTRACEUPDATE

      // Check this user is not banned
      if(ConnectionValid(pConn, pCurr->GetAllowDeny()) == false)
      {
         // Deny the login but don't say why
         pOut->Set("reply", MSG_USER_LOGIN_INVALID);
         pOut->AddChild("name", szName);

         if(stricmp(szRequest, MSG_USER_LOGIN) == 0)
         {
            // Tell admin a banner user login attempt was made
            pAdmin = new EDF();
            pAdmin->AddChild("userid", pCurr->GetID());
            pAdmin->AddChild("username", szName);
            ConnectionInfo(pAdmin, pConn, EDFITEMWRITE_ADMIN + USERITEMWRITE_LOGIN + USERITEMWRITE_LOGINFLAT, false);
            ServerAnnounce(pData, MSG_USER_LOGIN_DENIED, NULL, pAdmin);
            delete pAdmin;

            // Mark this connection for closing
            ConnectionShut(pConn, pData);
         }

         delete[] szRequest;

         debug("UserLogin exit false, %s (denied / not allowed)\n", MSG_USER_LOGIN_INVALID);
         return false;
      }

      if(stricmp(szRequest, MSG_USER_LOGIN_QUERY) == 0)
      {
         delete[] szRequest;

         pOut->AddChild("name", szName);

         debug("UserLogin exit true, %s success\n", MSG_USER_LOGIN_QUERY);
         return true;
      }

      delete[] szRequest;

      // Check the access type
      debug("UserLogin user type %d\n", pCurr->GetUserType());
      if(pCurr->GetUserType() == USERTYPE_AGENT)
      {
         // Agent login attempt
         STACKTRACEUPDATE

         pIn->GetChild("client", &pConnData->m_szClient);
         debug("UserLogin client name '%s'\n", pConnData->m_szClient);
         if(pConnData->m_szClient == NULL)
         {
            debug("UserLogin no client name\n");
            bValid = false;
         }
         else if(pCurr->GetAgent() == NULL)
         {
            debug("UserLogin no agent section\n");
            bValid = false;
         }
         else
         {
            szAgent = pCurr->GetAgentName();
            debug("UserLogin name compare %s -vs- %s\n", pConnData->m_szClient, szAgent);
            if(strmatch(pConnData->m_szClient, szAgent, true, true, false) == false)
            {
               debug("UserLogin agent names match failed\n");
               bValid = false;
            }
         }

         debug("UserLogin post agent check valid = %s\n", BoolStr(bValid));

         if(bValid == false)
         {
            // Agent could not login
            pOut->Set("reply", MSG_USER_LOGIN_INVALID);
            pOut->AddChild("name", szName);

            // Tell admin a failed agent login attempt was made
            pAdmin = new EDF();
            pAdmin->AddChild("userid", pCurr->GetID());
            pAdmin->AddChild("username", szName);
            ConnectionInfo(pAdmin, pConn, EDFITEMWRITE_ADMIN + USERITEMWRITE_LOGIN + USERITEMWRITE_LOGINFLAT, false);
            // pAdmin->AddChild(pIn, "client");
            ServerAnnounce(pData, MSG_USER_LOGIN_DENIED, NULL, pAdmin);
            delete pAdmin;

            // Mark this connection for closing
            ConnectionShut(pConn, pData);

            debug("UserLogin exit false, %s (invalid agent)\n", MSG_USER_LOGIN_INVALID);
            return false;
         }
      }
      else
      {
         // Normal user
         STACKTRACEUPDATE

         // Check the access level
         if(pCurr->GetAccessLevel() == LEVEL_NONE)
         {
            // No access
            debug("UserLogin invalid access level\n");

            pOut->Set("reply", MSG_USER_LOGIN_INVALID);
            pOut->AddChild("name", szName);

            // Tell admin a no access level user login attempt was made
            pAdmin = new EDF();
            pAdmin->AddChild("userid", pCurr->GetID());
            pAdmin->AddChild("username", szName);
            ConnectionInfo(pAdmin, pConn, EDFITEMWRITE_ADMIN + USERITEMWRITE_LOGIN + USERITEMWRITE_LOGINFLAT, false);
            // pAdmin->AddChild(pIn, "client");
            ServerAnnounce(pData, MSG_USER_LOGIN_DENIED, NULL, pAdmin);
            delete pAdmin;

            ConnectionShut(pConn, pData);

            debug("UserLogin exit false, %s (no access)\n", MSG_USER_LOGIN_INVALID);
            return false;
         }
      }

      STACKTRACEUPDATE

      if(mask(iStatus, LOGIN_NOCONTACT) == true)
      {
         debug("UserLogin mapping nocontact to shadow\n");
         iStatus |= LOGIN_SHADOW;
      }

      if(mask(iStatus, LOGIN_SHADOW) == false)
      {
         // Standard connection status attempt (check for current connection by this user)
         pItemConn = ConnectionFind(CF_USERPRI, pCurr->GetID());
         if(pItemConn != NULL)
         {
            // Is this a forced login?
            STACKTRACEUPDATE

            if(bForce == false)
            {
               // User already logged in
               pOut->Set("reply", MSG_USER_LOGIN_ALREADY_ON);
               pOut->AddChild("name", szName);

               debug("UserLogin exit false, %s\n", MSG_USER_LOGIN_ALREADY_ON);
               return false;
            }
            else
            {
               // Force the other connection off
               debug("UserLogin force off connection\n");

               ConnectionShut(pItemConn, pData, pCurr);
            }
         }
      }
   }

   STACKTRACEUPDATE

   // Reset time
   pConnData->m_lTimeOn = time(NULL);
   pConnData->m_lTimeIdle = pConnData->m_lTimeOn;

   // Setup status flag for this connection
   pConnData->m_iStatus = LOGIN_ON;
   if(mask(iStatus, LOGIN_SHADOW) == true)
   {
      pConnData->m_iStatus += LOGIN_SHADOW;
   }
   if(mask(iStatus, LOGIN_SHADOW) == false && mask(iStatus, LOGIN_BUSY) == true)
   {
      debug("UserLogin flagged as busy\n");
      pConnData->m_iStatus += LOGIN_BUSY;

      pConnData->m_lTimeBusy = time(NULL);
   }
   else
   {
      pConnData->m_lTimeBusy = -1;
   }
   if(mask(iStatus, LOGIN_SILENT) == true && (mask(iStatus, LOGIN_SHADOW) == true || pCurr->GetUserType() == USERTYPE_AGENT || pCurr->GetAccessLevel() >= LEVEL_WITNESS))
   {
      // printf("UserLogin agent / admin login flagged as silent\n");
      pConnData->m_iStatus += LOGIN_SILENT;
   }
   /* if(mask(iStatus, LOGIN_NOCONTACT) == true)
   {
      debug("UserLogin flagged as no contact\n");
      pConnData->m_iStatus += LOGIN_NOCONTACT;
   } */

   // pConnData->m_lUserID = pCurr->GetID();
   pConnData->m_pUser = pCurr;

   pIn->GetChild("client", &pConnData->m_szClient);
   pIn->GetChild("clientbase", &szClientBase);
   bClientUpgrade = pIn->GetChildBool("clientupgrade", true);
   pIn->GetChild("protocol", &pConnData->m_szProtocol);

   debug("UserLogin settings %s/%s(%s) %s, %s(%s %s), %s\n", pConn->Hostname(), pConn->AddressStr(), pConnData->m_szLocation, BoolStr(pConn->GetSecure()), pConnData->m_szClient, szClientBase, BoolStr(bClientUpgrade), pConnData->m_szProtocol);

   // Proxy settings
   if(szHostname != NULL && Conn::StringToAddress(szHostname, &lHostname) == true)
   {
      debug("UserLogin proxy hostname %s -> %lu\n", szHostname, lHostname);

      pConnData->m_szProxy = szHostname;
      pConnData->m_lProxy = lHostname;
   }
   else if(szAddress != NULL && Conn::StringToAddress(szAddress, &lAddress, false) == true)
   {
      debug("UserLogin proxy address %s -> %lu\n", szAddress, lAddress);

      pConnData->m_lProxy = lAddress;
   }
   if(pConnData->m_lProxy > 0)
   {
      delete[] pConnData->m_szLocation;
      pConnData->m_szLocation = ConnectionLocation(pData, pConnData->m_szProxy, pConnData->m_lProxy);

      debug("UserLogin proxy location %s / %lu -> %s\n", pConnData->m_szProxy, pConnData->m_lProxy, pConnData->m_szLocation);
   }

   STACKTRACEUPDATE

   if(szClientBase != NULL)
   {
      pClient = new EDF();
      pGet = pCurr->GetEDF();

      bLoop = pGet->Child("client");
      while(bLoop == true)
      {
         bDelete = false;

         pElement = pGet->GetCurr();
         if(pElement->getType() == EDFElement::BYTES)
         {
            if(strnicmp(pElement->getValueStr(false), szClientBase, strlen(szClientBase)) == 0)
            {
               // Match on this client section
               bLoop = pGet->Child();
               while(bLoop == true)
               {
                  // Migrate settings from this section
                  pClient->SetChild(pGet);

                  bLoop = pGet->Next();
                  if(bLoop == false)
                  {
                     pGet->Parent();
                  }
               }

               // Remove this section
               bDelete = true;
            }
         }

         if(bDelete == true && bClientUpgrade == true)
         {
            iPosition = pGet->Position(true);
            pGet->Delete();
            bLoop = pGet->Child("client", iPosition);
         }
         else
         {
            bLoop = pGet->Next("client");
            if(bLoop == false)
            {
               pGet->Parent();
            }
         }
      }

      if(pClient->Children() > 0)
      {
         // EDFParser::Print("UserLogin client settings", pClient);

         if(pGet->Child("client", pConnData->m_szClient) == false)
         {
            pGet->Add("client", pConnData->m_szClient);
         }
         else
         {
            while(pGet->DeleteChild() == true);
         }
         pGet->Copy(pClient, false, false);

         /* if(bClientUpgrade == true)
         {
            debugEDFPrint("UserLogin client post-update data section", pGet);
         } */
      }

      debug("UserLogin updated client settings to latest client\n");
      // EDFPrint(pGet);

      delete pClient;

      debug("UserLogin deleting client base string\n");

      delete[] szClientBase;
   }

   if(pIn->GetChild(ConnVersion(pConnData, "2.5") >= 0 ? "statusmsg" : "busymsg", &pStatusMsg) == true)
   {
      delete pConnData->m_pStatusMsg;

      if(pStatusMsg != NULL)
      {
         pConnData->m_pStatusMsg = pStatusMsg->SubString(UA_SHORTMSG_LEN);
         delete pStatusMsg;
      }
      else
      {
         pConnData->m_pStatusMsg = NULL;
      }
   }

   if(pIn->GetChild("attachmentsize", &iAttachmentSize) == true && (iAttachmentSize > 0 || iAttachmentSize == -1))
   {
      debug("UserLogin setting attachmentsize to %d\n", iAttachmentSize);
      pConnData->m_lAttachmentSize = iAttachmentSize;
   }

   // printf("UserLogin deleting client string\n");
   // delete[] szClient;

   STACKTRACEUPDATE

   if(mask(pConnData->m_iStatus, LOGIN_SHADOW) == false && mask(pConnData->m_iStatus, LOGIN_SILENT) == false)
   {
      // Set user stored details for normal connection
      pCurr->SetAddress(pConn->Address());
      pCurr->SetAddressProxy(pConnData->m_lProxy);
      pCurr->SetClient(pConnData->m_szClient);
      debug("UserLogin setting hostname '%s'\n", pConn->Hostname());
      pCurr->SetHostname(pConn->Hostname());
      debug("UserLogin setting proxy hostname '%s'\n", pConnData->m_szProxy);
      pCurr->SetHostnameProxy(pConnData->m_szProxy);
      pCurr->SetLocation(pConnData->m_szLocation);
      pCurr->SetProtocol(pConnData->m_szProtocol);
      pCurr->SetSecure(pConn->GetSecure());
      pCurr->SetStatus(pConnData->m_iStatus);
      pCurr->SetStatusMsg(pConnData->m_pStatusMsg);
      pCurr->SetTimeBusy(pConnData->m_lTimeBusy);
      pCurr->SetTimeOn(pConnData->m_lTimeOn);
   }

   // Update the stats
   pCurr->IncStat(USERITEMSTAT_NUMLOGINS, 1);

   debug("UserLogin conn data %p %p\n", pConnData->m_pFolders, pConnData->m_pReads);

   for(iListNum = 0; iListNum < ConnCount(); iListNum++)
   {
      pItemConn = ConnList(iListNum);
      pItemData = (ConnData *)pItemConn->Data();

      if(pItemData->m_pUser != NULL)
      {
         iCurrLogins++;
         if(pItemData->m_pUser != NULL && pItemData->m_pUser->GetID() == pCurr->GetID() && pItemConn->ID() != pConn->ID())
         {
            debug("UserLogin referencing conn data from connection %ld\n", pItemConn->ID());

            pConnData->m_pFolders = pItemData->m_pFolders;
            pConnData->m_pReads = pItemData->m_pReads;
			pConnData->m_pSaves = pItemData->m_pSaves;
         }
      }
   }

   if(pConnData->m_pFolders == NULL)
   {
      debug("UserLogin creating new folder sub data\n");
      pConnData->m_pFolders = new DBSub(MessageTreeSubTable(RFG_FOLDER), MessageTreeID(RFG_FOLDER), pCurr->GetID());

      debug("UserLogin creating new message read list\n");
	  pConnData->m_pReads = new DBMessageRead(pCurr->GetID());

      debug("UserLogin creating new message save list\n");
	  pConnData->m_pSaves = new DBMessageSave(pCurr->GetID());
   }
   
   debug("UserLogin %d folders, %d reads, %d saves\n", pConnData->m_pFolders->Count(), pConnData->m_pReads->Count(), pConnData->m_pSaves->Count());

   debug("UserLogin creating new channel data\n");
   // iDebug = debuglevel(DEBUGLEVEL_DEBUG);
   pConnData->m_pChannels = new DBSub(MessageTreeSubTable(RFG_CHANNEL), MessageTreeID(RFG_CHANNEL), pCurr->GetID(), SUBTYPE_MEMBER, 0);
   // debuglevel(iDebug);

   pConnData->m_pServices = new DBSub(SERVICE_SUB, "serviceid", pCurr->GetID(), 0, 0);

   /* if(pCurr->GetAccessLevel() == LEVEL_EDITOR && pConnData->m_pFolders->CountType(SUBTYPE_EDITOR) == 0)
   {
      debug("UserLogin demoting %s to level %d\n", pCurr->GetName(), LEVEL_MESSAGES);
      pCurr->SetAccessLevel(LEVEL_MESSAGES);
   } */

   debug("UserLogin conn data %d folders, %d reads, %d saves, %d channels, %d services\n", pConnData->m_pFolders->Count(), pConnData->m_pReads->Count(), pConnData->m_pSaves->Count(), pConnData->m_pChannels->Count(), pConnData->m_pServices->Count());

   UserItemList(pOut, pCurr, EDFITEMWRITE_ADMIN + EDFITEMWRITE_FLAT + USERITEMWRITE_DETAILS + USERITEMWRITE_SELF + ULS_FULL, USERITEMWRITE_LOGIN, NULL, 0, pConnData->m_pFolders, pConnData->m_pChannels, pConnData->m_pServices, pConn, 0);
   // EDFPrint("UserLogin list single", pOut);

   STACKTRACEUPDATE

   if(mask(iStatus, LOGIN_SILENT) == false)
   {
      pAnnounce = new EDF();
      pAnnounce->AddChild("userid", pCurr->GetID());
      pAnnounce->AddChild("username", szName);
      // pAnnounce->AddChild("timeon", pConnData->m_lTimeOn);
      ConnectionInfo(pAnnounce, pConn, USERITEMWRITE_LOGIN + USERITEMWRITE_LOGINFLAT, false);
   }

   pAdmin = new EDF();
   pAdmin->AddChild("userid", pCurr->GetID());
   pAdmin->AddChild("username", szName);
   // pAdmin->AddChild("timeon", pConnData->m_lTimeOn);
   ConnectionInfo(pAdmin, pConn, EDFITEMWRITE_ADMIN + USERITEMWRITE_LOGIN + USERITEMWRITE_LOGINFLAT, false);
   // pAdmin->AddChild(pIn, "client");
   // pAdmin->AddChild(pIn, "protocol");

   ServerAnnounce(pData, MSG_USER_LOGIN, pAnnounce, pAdmin);
   delete pAnnounce;
   delete pAdmin;

   STACKTRACEUPDATE

   pData->Root();
   pData->Child("users");
   pData->GetChild("maxlogins", &iMaxLogins);

   debug("UserLogin logins max %d, curr %d\n", iMaxLogins, iCurrLogins);
   if(iCurrLogins > iMaxLogins)
   {
      if(pData->Child("maxlogins") == false)
      {
         pData->Add("maxlogins");
      }
      pData->Set(NULL, iCurrLogins);
      pData->SetChild("date", (int)(time(NULL)));
      pData->Parent();
   }

   debug("UserLogin exit true\n");
   // debugEDFPrint(pOut);
   return true;
}

/*
** UserLogout: Log the specified user out
**
** userid(admin):     ID of the user to logout
** text:    Logout message
**
** If userid not specified log the current user out
*/
ICELIBFN bool UserLogout(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   STACKTRACE
   bool bReturn = false;
   char *szText = NULL;
   UserItem *pCurr = CONNUSER, *pItem = NULL;

   // iByID = pCurr->GetID();
   // szBy = pCurr->GetName();
   // iAccessLevel = pCurr->GetAccessLevel();

   pIn->GetChild("text", &szText);

   if(pIn->IsChild("userid") == true)
   {
      if(UserAccess(pCurr, pIn, "userid", false, &pItem, pOut) == false)
      {
         return false;
      }

      // pIn->GetChild("userid", &iUserID);
      // iUserID = pItem->GetID();
      // bForce = pIn->GetChildBool("force");

      debug("UserLogout entry %ld by %ld\n", pItem->GetID(), pCurr->GetID());

      // iStatus = pItem->GetStatus();

      /* if(mask(pItem->GetStatus(), LOGIN_ON) == false)
      {
      // User isn't logged on anyway
      pOut->Set("reply", MSG_USER_NOT_ON);
      pOut->AddChild("userid", pItem->GetID());
      pOut->AddChild("username", pItem->GetName());
      // delete[] szName;
      // delete[] szBy;

      debug("UserLogout exit false, %s\n", MSG_USER_NOT_ON);
      return false;
      } */

      bReturn = ConnectionShut(pData, CF_USERPRI, pItem->GetID(), pCurr, pIn);
   }
   else
   {
      debug("UserLogout entry self\n");

      pItem = pCurr;

      bReturn = ConnectionShut(pConn, pData, NULL, false, pIn);
   }

   // pOut->AddChild("name", szName);

   if(bReturn == false)
   {
      pOut->Set("reply", MSG_USER_NOT_ON);
      pOut->AddChild("userid", pItem->GetID());
      pOut->AddChild("username", pItem->GetName());
      return false;
   }

   pOut->AddChild("userid", pItem->GetID());
   pOut->AddChild("username", pItem->GetName());

   // delete[] szName;
   // delete[] szBy;

   debug("UserLogout exit %s\n", BoolStr(bReturn));
   return bReturn;
}

/*
** UserLogoutList: Generate a list of logout messages
*/
ICELIBFN bool UserLogoutList(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   STACKTRACE
   /* int iUserID = 0;
   bool bLoop = false;
   char *szUserName = NULL;

   pData->Root();
   pData->Child("users");
   pData->Child("logins");
   bLoop = pData->Child("user");
   while(bLoop == true)
   {
   pData->Get(NULL, &iUserID);
   if(UserGet(pData, iUserID, &szUserName, true) == true)
   {
   pOut->Add("user", iUserID);
   pOut->AddChild("name", szUserName);
   pOut->AddChild(pData, "text");
   pOut->Parent();

   delete[] szUserName;
   }

   bLoop = pData->Next("user");
   } */

   // EDFPrint("UserLogoutList output", pOut);

   return true;
}

/*
** UserLogoutClear: Clears down user logout messages
*/
ICELIBFN bool UserLogoutClear(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   STACKTRACE
   /* int iNumLogouts = 0;

   pData->Root();
   pData->Child("users");
   pData->Child("logins");

   if(pIn->GetChild("numlogouts", &iNumLogouts) == false || iNumLogouts < 0 || iNumLogouts > pData->Children("user"))
   {
   iNumLogouts = pData->Children("user");
   }

   debug("UserLogoutClear removing %d logouts\n", iNumLogouts);
   while(iNumLogouts > 0)
   {
   pData->DeleteChild("user");
   iNumLogouts--;
   } */

   return true;
}

/*
** UserSysop: Transfer sysop level status from one user to another
**
**
** Only the sysop can do this
*/

ICELIBFN bool UserSysop(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   STACKTRACE
   int iAccessLevel = LEVEL_NONE, iUserID = 0, iSelfID = 0;
   UserItem *pCurr = CONNUSER, *pSysop = NULL;

   /* pData->Get(NULL, &iSelfID);
   pData->GetChild("accesslevel", &iAccessLevel); */
   iSelfID = pCurr->GetID();
   iAccessLevel = pCurr->GetAccessLevel();

   debug("UserSysop entry %d %d\n", iSelfID, iAccessLevel);
   EDFParser::debugPrint(pIn);
   if(iAccessLevel < LEVEL_SYSOP)
   {
      pOut->Set("reply", MSG_ACCESS_INVALID);
      return false;
   }

   /* if(pIn->GetChild("userid", &iUserID) == false)
   {
      pOut->Set("reply", MSG_USER_NOT_EXIST);
      return false;
   }

   // if(UserGet(pData, iUserID) == false)
   pSysop = UserGet(iUserID);
   if(pSysop == NULL)
   {
      // User specified to transfer to does not exist
      pOut->Set("reply", MSG_USER_NOT_EXIST);
      pOut->AddChild("userid", iUserID);
      return false;
   } */

   if(UserAccess(pCurr, pIn, "userid", true, &pSysop, pOut) == false)
   {
      return false;
   }

   // Check the user to transfer is not this one, which would leave no SysOp
   if(iUserID != iSelfID)
   {
      // pData->SetChild("accesslevel", LEVEL_SYSOP);
      pSysop->SetAccessLevel(LEVEL_SYSOP);

      // pData->SetCurr(CONNID);
      // pData->SetChild("accesslevel", LEVEL_WITNESS);
      pCurr->SetAccessLevel(LEVEL_WITNESS);
   }

   return true;
}

}
