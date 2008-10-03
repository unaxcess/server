#include <stdio.h>
#include <string.h>

#include "UAService.h"

UAService::UAService() : UAClient()
{
   m_iNumLookups = 0;
   m_pLookups = NULL;

   m_pServices = NULL;
}

UAService::~UAService()
{
   shutdown();
}

bool UAService::shutdown()
{
   STACKTRACE
   int iLookupNum = 0;

   debug("UAService::shutdown entry\n");

   if(m_pServices != NULL)
   {
      delete m_pServices;
      m_pServices = NULL;

      for(iLookupNum = 0; iLookupNum < m_iNumLookups; iLookupNum++)
      {
         ServiceLogout(m_pLookups[iLookupNum]);

         delete m_pLookups[iLookupNum];
      }

      delete[] m_pLookups;
      m_iNumLookups = 0;
   }

   debug("UAService::shutdown exit true\n");
   return true;
}

bool UAService::loggedIn()
{
   STACKTRACE
   bool bLoop = false;
   int iFolderID = 0, iAccessMode = 0, iSubType = 0;
   EDF *pRequest = NULL, *pReply = NULL, *pTemp = NULL;

   debug("UAService::loggedIn entry\n");
   EDFParser::debugPrint(m_pUser);

   request(MSG_SERVICE_LIST, &m_pServices);

   pRequest = new EDF();
   pRequest->AddChild("searchtype", 2);

   request(MSG_FOLDER_LIST, pRequest, &pReply);

   bLoop = pReply->Child("folder");
   while(bLoop == true)
   {
      iAccessMode = 0;
      iSubType = 0;

      pReply->Get(NULL, &iFolderID);
      pReply->GetChild("subtype", &iSubType);
      pReply->GetChild("accessmode", &iAccessMode);

      if(mask(iAccessMode, ACCMODE_PRIVATE) == true)
      {
         if(iSubType == 0)
         {
            debug("UAService::loggedIn subscribing to folder %d\n", iFolderID);

            pTemp = new EDF();
            pTemp->AddChild("folderid", iFolderID);
            request(MSG_FOLDER_SUBSCRIBE, pTemp);
            delete pTemp;
         }
      }
      else
      {
         if(iSubType > 0)
         {
            debug("UAService::loggedIn unsubscribing from folder %d\n", iFolderID);

            pTemp = new EDF();
            pTemp->AddChild("folderid", iFolderID);
            request(MSG_FOLDER_UNSUBSCRIBE, pTemp);
            delete pTemp;
         }
      }

      bLoop = pReply->Iterate("folder");
   }

   delete pReply;

   delete pRequest;

   debug("UAService::loggedIn exit true\n");
   return true;
}

bool UAService::announce(const char *szAnnounce, EDF *pAnnounce)
{
   STACKTRACE
   int iLookupNum = 0, iUserID = 0, iStatus = LOGIN_OFF, iOwnerID = -1, iContactID = -1, iServiceID = -1;
   bool bBusy = false;
   char szWrite[2000];
   char *szFromName = NULL, *szToName = NULL, *szStatusMsg = NULL, *szUser = NULL, *szAction = NULL;
   bytes *pText = NULL;
   ServiceLookup *pLookup = NULL;

   EDFParser::debugPrint(DEBUGLEVEL_DEBUG, "UAService::announce", pAnnounce);

   if(stricmp(szAnnounce, MSG_USER_PAGE) == 0)
   {
      EDFParser::debugPrint(DEBUGLEVEL_DEBUG, "UAService::announce user page", pAnnounce);

      m_pUser->Root();
      m_pUser->GetChild("ownerid", &iOwnerID);

      pAnnounce->GetChild("fromid", &iUserID);
      pAnnounce->GetChild("fromname", &szFromName);
      pAnnounce->GetChild("text", &pText);
      pAnnounce->GetChild("contactid", &iContactID);
      pAnnounce->GetChild("serviceid", &iServiceID);

      pLookup = LookupGet(iUserID);

      if(iServiceID != -1 && pLookup != NULL)
      {
         pLookup->m_iContactID = iContactID;

         pAnnounce->GetChild("toname", &szToName);

         if(szToName != NULL)
         {
            ServiceSend(pLookup, szToName, (char *)pText->Data(false));
         }
         else
         {
            pAnnounce->GetChild("serviceaction", &szAction);

            if(szAction != NULL)
            {
               ServiceAction(pLookup, szAction, pAnnounce);
            }
            else
            {
               service(pLookup, NULL, NULL, ACTION_CONTACT_INVALID, NULL, NULL);
            }

            delete[] szAction;
         }
      }
      else
      {
         if(pText == NULL || pText->Length() == 0 || pText->Compare("/?") == 0)
         {
	    debug("UAService::announce help");
         }
         else if(iUserID == iOwnerID && pText->Compare("/c") == 0)
         {
            debug("UAService::announce connection query\n");

            if(m_iNumLookups > 0)
            {
               strcpy(szWrite, "Connections:");

               for(iLookupNum = 0; iLookupNum < m_iNumLookups; iLookupNum++)
               {
                  sprintf(szWrite, "%s\r\n  %d(%s) <--> %d(%s)", szWrite, m_pLookups[iLookupNum]->m_iLocalID, m_pLookups[iLookupNum]->m_szLocalName, m_pLookups[iLookupNum]->m_iRemoteID, m_pLookups[iLookupNum]->m_szRemoteName);
               }
            }
            else
            {
               strcpy(szWrite, "No active connections");
            }

            page(iContactID, NULL, iUserID, szWrite, NULL);
         }
         else if(iUserID == iOwnerID && pText->Compare("/s") == 0)
         {
            debug("UAService::announce shutdown request\n");

            shutdown();

            m_pData->SetChild("quit", true);
         }
	 else if(iUserID == iOwnerID)
	 {
	    command(iUserID, (char *)pText->Data(false));
	 }
      }

      delete[] szFromName;
      delete pText;
   }
   else if(stricmp(szAnnounce, MSG_USER_LOGOUT) == 0)
   {
      EDFParser::debugPrint("UAService::announce user logout", pAnnounce);

      UAUnsubscribe(pAnnounce);
   }
   else if(stricmp(szAnnounce, MSG_USER_STATUS) == 0)
   {
      pAnnounce->GetChild("userid", &iUserID);
      pAnnounce->GetChild("username", &szUser);
      pAnnounce->GetChild("status", &iStatus);

      // debug("UAService::announce user status %s\n", szUser);
      // EDFPrint(pAnnounce);

      pLookup = LookupGet(iUserID);

      if(pLookup != NULL)
      {
         pAnnounce->GetChild("busymsg", &szStatusMsg);
         pAnnounce->GetChild("statusmsg", &szStatusMsg);

         if(mask(iStatus, LOGIN_BUSY) == true)
         {
            bBusy = true;
         }

         if(szStatusMsg != NULL)
         {
         }

         if(szStatusMsg != NULL)
         {
            ServiceStatus(pLookup, szStatusMsg, bBusy);
         }
      }

      delete[] szStatusMsg;
   }
   else if(stricmp(szAnnounce, MSG_SERVICE_SUBSCRIBE) == 0)
   {
      EDFParser::debugPrint("UAService::announce service subscribe", pAnnounce);

      UASubscribe(pAnnounce);
   }
   else if(stricmp(szAnnounce, MSG_SERVICE_UNSUBSCRIBE) == 0)
   {
      EDFParser::debugPrint("UAService::announce service unsubscribe", pAnnounce);

      UAUnsubscribe(pAnnounce);
   }

   debug("UAService::announce exit true\n");
   return true;
}

UAClient::BackgroundReturn UAService::background()
{
   int iLookupNum = 0;

   ServiceBackground();

   while(iLookupNum < m_iNumLookups)
   {
      ServiceBackground(m_pLookups[iLookupNum]);

      iLookupNum++;
   }

   return BG_NONE;
}

bool UAService::UASubscribe(EDF *pEDF)
{
   STACKTRACE
   int iServiceID = -1, iUserID = -1, iServiceType = 0;
   char *szUserName = NULL, *szServiceUser = NULL, *szServicePassword = NULL;
   ServiceLookup *pLookup = NULL, **pTemp = NULL;

   pEDF->GetChild("serviceid", &iServiceID);

   pEDF->GetChild("userid", &iUserID);
   pEDF->GetChild("username", &szUserName);

   debug("UAService::UASubscribe entry %d %d %s\n", iServiceID, iUserID, szUserName);

   if(EDFFind(m_pServices, "service", iServiceID, false) == true)
   {
      m_pServices->GetChild("servicetype", &iServiceType);
      m_pServices->Parent();
   }

   if(mask(iServiceType, SERVICE_LOGIN) == true)
   {
      if(pEDF->GetChild("name", &szServiceUser) == false)
      {
         // page(-1, NULL, iUserID, ACTION_LOGIN_INVALID, NULL, NULL);
         service(iUserID, NULL, ACTION_LOGIN_INVALID);

         debug("UAService::UASubscribe exit false, no name field\n");
         return false;
      }

      if(pEDF->GetChild("password", &szServicePassword) == false)
      {
         // page(-1, NULL, iUserID, ACTION_LOGIN_INVALID, NULL, NULL);
         service(iUserID, NULL, ACTION_LOGIN_INVALID);

         delete[] szServiceUser;

         debug("UAService::UASubscribe exit false, no password field\n");
         return false;
      }
   }

   debug("UAService::UASubscribe creating lookup\n");

   pLookup = new ServiceLookup;

   pLookup->m_iServiceID = iServiceID;
   pLookup->m_iContactID = -1;

   pLookup->m_iLocalID = iUserID;
   pLookup->m_szLocalName = strmk(szUserName);

   pLookup->m_iRemoteID = 0;
   pLookup->m_szRemoteName = strmk(szServiceUser);

   pLookup->m_pData = NULL;

   debug("UAService::UASubscribe connecting to service\n");
   ServiceConnect(pLookup);

   if(mask(iServiceType, SERVICE_LOGIN) == true)
   {
      if(ServiceLogin(pLookup, szServicePassword, 0) == false)
      {
         delete[] szServiceUser;
         delete[] szServicePassword;

         delete pLookup;

         return false;
      }

      debug("UAService::UASubscribe insert lookup\n");
      ARRAY_INSERT(ServiceLookup *, m_pLookups, m_iNumLookups, pLookup, m_iNumLookups, pTemp);
   }
   else
   {
      debug("UAService::UASubscribe insert lookup\n");
      ARRAY_INSERT(ServiceLookup *, m_pLookups, m_iNumLookups, pLookup, m_iNumLookups, pTemp);

      service(pLookup, NULL, NULL, ACTION_LOGIN, NULL, NULL);
   }

   delete[] szServiceUser;
   delete[] szServicePassword;

   debug("UAService::UASubscribe exit true\n");
   return true;
}

bool UAService::UAUnsubscribe(EDF *pEDF)
{
   STACKTRACE
   int iUserID = 0, iLookupNum = 0, iServiceType = 0;
   char *szUser = NULL;
   ServiceLookup *pLookup = NULL, **pTemp = NULL;

   pEDF->GetChild("userid", &iUserID);
   pEDF->GetChild("username", &szUser);

   pLookup = LookupGet(iUserID, &iLookupNum);

   debug("UAService::UAUnsubscribe entry %d %s %p %d\n", iUserID, szUser, pLookup, iLookupNum);

   if(pLookup == NULL)
   {
      debug("UAService::UAUnsubscribe exit false, no lookup\n");
      return false;
   }

   if(EDFFind(m_pServices, "service", pLookup->m_iServiceID, false) == true)
   {
      m_pServices->GetChild("servicetype", &iServiceType);
      m_pServices->Parent();
   }

   if(mask(iServiceType, SERVICE_LOGIN) == true)
   {
      ServiceLogout(pLookup);
   }

   ServiceDisconnect(pLookup);

   delete[] m_pLookups[iLookupNum]->m_szLocalName;
   delete[] m_pLookups[iLookupNum]->m_szRemoteName;

   ARRAY_DELETE(ServiceLookup *, m_pLookups, m_iNumLookups, iLookupNum, pTemp);

   debug("UAService::UAUnsubscribe exit true\n");
   return true;
}

bool UAService::ServiceBackground()
{
   STACKTRACE
   return false;
}

bool UAService::ServiceBackground(ServiceLookup *pLookup)
{
   STACKTRACE
   return false;
}

bool UAService::ServiceConnect(ServiceLookup *pLookup)
{
   STACKTRACE
   return false;
}

bool UAService::ServiceLogin(ServiceLookup *pLookup, char *szPassword, int iStatus)
{
   STACKTRACE
   return false;
}

bool UAService::ServiceLogout(ServiceLookup *pLookup)
{
   STACKTRACE
   return false;
}

bool UAService::ServiceDisconnect(ServiceLookup *pLookup)
{
   STACKTRACE
   return false;
}

bool UAService::ServiceSend(ServiceLookup *pLookup, char *szTo, char *szText)
{
   STACKTRACE
   return false;
}

bool UAService::ServiceStatus(ServiceLookup *pLookup, char *szMessage, bool bBusy)
{
   STACKTRACE
   return false;
}

bool UAService::ServiceAction(ServiceLookup *pLookup, char *szAction, EDF *pEDF)
{
   STACKTRACE
   return false;
}

bool UAService::page(int iContactID, char *szFromName, int iToID, char *szText, EDF *pEDF)
{
   bool bReturn = false;
   EDF *pRequest = NULL, *pReply = NULL;

   pRequest = new EDF();

   if(iContactID > 0)
   {
      pRequest->AddChild("contactid", iContactID);
   }

   if(szFromName != NULL)
   {
      pRequest->AddChild("fromname", szFromName);
   }

   pRequest->AddChild("toid", iToID);

   if(szText != NULL)
   {
      pRequest->AddChild("text", szText);
   }

   if(pEDF != NULL)
   {
      pRequest->Copy(pEDF, false);
   }

   EDFParser::debugPrint("UAService::page request", pRequest);

   bReturn = request(MSG_USER_CONTACT, pRequest, &pReply);

   delete pReply;

   return bReturn;
}

bool UAService::service(int iToID, char *szText, const char *szAction)
{
   STACKTRACE
   bool bReturn = false;
   EDF *pEDF = NULL;

   pEDF = new EDF();

   pEDF->AddChild("serviceaction", szAction);

   bReturn = page(0, NULL, iToID, szText, pEDF);

   delete pEDF;

   return bReturn;
}

bool UAService::service(ServiceLookup *pLookup, char *szFromName, char *szText, const char *szAction, EDF *pAction, EDF *pEDF)
{
   STACKTRACE
   bool bReturn = false;
   EDF *pRequest = NULL;

   debug("UAService::service %p %s %s %p %p\n", pLookup, szAction, szText, pAction, pEDF);

   pRequest = new EDF();

   if(pLookup->m_iServiceID > 0)
   {
      pRequest->AddChild("serviceid", pLookup->m_iServiceID);
   }

   if(szAction != NULL)
   {
      pRequest->Add("serviceaction", szAction);

      if(pAction != NULL)
      {
         pRequest->Copy(pAction, false);
      }

      pRequest->Parent();
   }

   bReturn = page(pLookup->m_iContactID, szFromName, pLookup->m_iLocalID, szText, pRequest);

   delete pRequest;

   return bReturn;
}

bool UAService::serviceAll(int iServiceID, char *szFromName, char *szText, const char *szAction, EDF *pAction, EDF *pEDF)
{
   STACKTRACE
   int iLookupNum = 0;
   ServiceLookup *pLookup = NULL;

   debug("UAService::serviceAll %d %s %s %s %p %p\n", iServiceID, szFromName, szText, szAction, pAction, pEDF);

   for(iLookupNum = 0; iLookupNum < m_iNumLookups; iLookupNum++)
   {
      pLookup = m_pLookups[iLookupNum];
      if(pLookup->m_iServiceID == iServiceID)
      {
         service(pLookup, szFromName, szText, szAction, pAction, pEDF);
      }
   }

   return true;
}

bool UAService::command(int iFromID, char *szText)
{
   debug("UAService::command %d %s\n", iFromID, szText);
   
   return true;
}

int UAService::RemoteToLocal(int iRemoteID)
{
   int iLookupNum = 0, iReturn = -1;

   while(iReturn == -1 && iLookupNum < m_iNumLookups)
   {
      if(m_pLookups[iLookupNum]->m_iRemoteID == iRemoteID)
      {
         iReturn = m_pLookups[iLookupNum]->m_iLocalID;
      }
      else
      {
         iLookupNum++;
      }
   }

   return iReturn;
}

UAService::ServiceLookup *UAService::LookupGet(int iLocalID, int *iLookupNum)
{
   int iLookupPos = 0;
   ServiceLookup *pReturn = NULL;

   debug("UAService::LookupGet entry %d, %d\n", iLocalID, m_iNumLookups);

   while(pReturn == NULL && iLookupPos < m_iNumLookups)
   {
      debug("UAService::LookupGet %d %d\n", iLocalID, m_pLookups[iLookupPos]->m_iLocalID);
      if(iLocalID == m_pLookups[iLookupPos]->m_iLocalID)
      {
         pReturn = m_pLookups[iLookupPos];
         debug("UAService::LookupGet matched %d(%s) / %d(%s)\n", pReturn->m_iLocalID, pReturn->m_szLocalName, pReturn->m_iRemoteID, pReturn->m_szRemoteName);

         if(iLookupNum != NULL)
         {
            *iLookupNum = iLookupPos;
         }
      }
      else
      {
         iLookupPos++;
      }
   }

   debug("UAService::LookupGet exit %p\n", pReturn);
   return pReturn;
}

UAService::ServiceLookup *UAService::LookupList(int iLookupNum)
{
   if(iLookupNum < 0 || iLookupNum >= m_iNumLookups)
   {
      return NULL;
   }

   return m_pLookups[iLookupNum];
}

int UAService::LookupCount()
{
   return m_iNumLookups;
}
