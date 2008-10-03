#ifndef _UASERVICE_H_
#define _UASERVICE_H_

#include "client/UAClient.h"

class UAService : public UAClient
{
public:
   UAService();
   virtual ~UAService();

protected:
   struct ServiceLookup
   {
      int m_iServiceID;
      int m_iContactID;

      int m_iLocalID;
      char *m_szLocalName;

      int m_iRemoteID;
      char *m_szRemoteName;

      void *m_pData;
   };

   virtual bool shutdown();

   virtual bool loggedIn();

   virtual bool announce(const char *szAnnounce, EDF *pAnnounce);

   BackgroundReturn background();

   virtual bool ServiceBackground();
   virtual bool ServiceBackground(ServiceLookup *pLookup);

   virtual bool ServiceConnect(ServiceLookup *pLookup);
   virtual bool ServiceLogin(ServiceLookup *pLookup, char *szPassword, int iStatus);
   virtual bool ServiceLogout(ServiceLookup *pLookup);
   virtual bool ServiceDisconnect(ServiceLookup *pLookup);

   virtual bool ServiceSend(ServiceLookup *pLookup, char *szTo, char *szText);
   virtual bool ServiceStatus(ServiceLookup *pLookup, char *szMessage, bool bBusy);

   virtual bool ServiceAction(ServiceLookup *pLookup, char *szAction, EDF *pEDF);

   bool page(int iContactID, char *szFromName, int iToID, char *szText, EDF *pEDF);
   bool service(int iToID, char *szText, const char *szAction);
   bool service(ServiceLookup *pLookup, char *szFromName, char *szText, const char *szAction, EDF *pAction, EDF *pEDF);
   bool serviceAll(int iServiceID, char *szFromName, char *szText, const char *szAction, EDF *pAction, EDF *pEDF);
   virtual bool command(int iFromID, char *szText);

   int RemoteToLocal(int iRemoteID);

   ServiceLookup *LookupGet(int iUserID, int *iLookupNum = NULL);
   ServiceLookup *LookupList(int iLookupNum);
   int LookupCount();

private:
   int m_iNumLookups;
   struct ServiceLookup **m_pLookups;

   EDF *m_pServices;

   bool UASubscribe(EDF *pEDF);
   bool UAUnsubscribe(EDF *pEDF);
};

#endif
