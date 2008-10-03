#ifndef _UACLIENT_H_
#define _UACLIENT_H_

#include "common/EDFConn.h"

#include "ua.h"

typedef void (*UACLIENTPANIC)(int);

class UAClient : public EDFConn
{
public:
   enum BackgroundReturn { BG_INVALID = -1, BG_NONE = 0, BG_QUIT = 1, BG_WRITE = 2 };

   // Constructor and destructor
   UAClient(bool bPanic = true);
   virtual ~UAClient();

   virtual bool request(const char *szRequest, EDF **pReply = NULL);
   virtual bool request(const char *szRequest, EDF *pRequest, EDF **pReply = NULL);

   int version(const char *szVersion);

   // Main setup and loop function
   bool run(int argc, char **argv, EDF *pConfig = NULL, bool bLoop = true);
   bool loop();

   virtual void help();

   virtual char *CLIENT_NAME() = 0;
   virtual char *CLIENT_BASENAME();

protected:
   enum CommandLineOption { OPT_INT = 1, OPT_STRING = 2, OPT_BOOL = 3 };

   EDF *m_pData;
   EDF *m_pUser;

   void addOption(char *szName, CommandLineOption iType, bool bMandatory = false);
   bool setOption(char *szName, int iValue);

   bool setInterval(int iInterval);

   // Functions to be derived
   virtual bool config(EDF *pConfig);

   virtual bool startup(int argc, char **argv);
   virtual bool shutdown();
   virtual bool connected();

   virtual bool login(EDF *pRequest);
   virtual bool loggedIn();
   virtual bool notLoggedIn();

   virtual bool reply(const char *szReply, EDF *pReply);
   virtual bool announce(const char *szAnnounce, EDF *pAnnounce);

   virtual BackgroundReturn background();

   static void SetPanic(UACLIENTPANIC pPanic);

   bool requestMode();
   bool requestMode(bool bRequest);

   // Lookups
   int UserGet(char *szName);
   bool UserGet(int iUserID, char **szName);
   int FolderGet(char *szName);
   bool FolderGet(int iFolderID, char **szName);
   bool IsFolderPrivate(int iFolderID);

   // Requests
   bool FolderSubscribe(int iFolderID, int iUserID = -1, int iSubType = SUBTYPE_SUB);

private:
   struct option
   {
      char *m_szName;
      CommandLineOption m_iType;
      bool m_bMandatory;
   };

   char *m_szFile;
   char *m_szProtocol;

   option **m_pOptions;
   int m_iNumOptions;

   bool m_bRequest;

   EDF *m_pUserList;
   EDF *m_pFolderList;

   // bool loop();
   bool loop(EDF **pReply);
   bool defaultreply(const char *szReply, EDF *pReply);

   bool userList();
   bool folderList();
};

#endif
