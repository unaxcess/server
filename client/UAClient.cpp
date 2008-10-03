#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#ifdef UNIX
#include <unistd.h>
#endif

#include "UAClient.h"

#include "CliUser.h"
#include "CliFolder.h"

#ifndef SIGHUP
#define SIGHUP 1
#endif
#ifndef SIGUNUSED
#define SIGUNUSED 31
#endif

int g_iUAClientPanic = 0;
UACLIENTPANIC g_pPanic = NULL;

// Perform emergency shutdown on program error
void UAClientPanic(int iSignal)
{
#ifdef UNIX
   printf("UAClientPanic entry %d, %d %d\r\n", iSignal, getpid(), errno);
#else
   printf("UAClientPanic entry %d, %d %d\r\n", iSignal, errno, GetLastError());
#endif

   if(g_iUAClientPanic != 0)
   {
      printf("UAClientPanic multi panic\r\n");
      signal(iSignal, SIG_DFL);
      exit(3);
   }

   g_iUAClientPanic++;

   if(g_pPanic != NULL)
   {
      (*g_pPanic)(iSignal);
   }

   STACKPRINT

   printf("UAClientPanic exit\r\n");

   exit(2);
}

UAClient::UAClient(bool bPanic) : EDFConn()
{
   debug(DEBUGLEVEL_INFO, "UAClient::UAClient entry %s\n", BoolStr(bPanic));

   if(bPanic == true)
   {
#ifdef UNIX
      // Divert signals to the Panic function
      for(int i = SIGHUP; i < SIGUNUSED; i++)
      {
         if(i != SIGKILL && i != SIGSTOP)
         {
            signal(i, UAClientPanic);
         }
      }
#else
      signal(SIGINT, UAClientPanic);
      signal(SIGILL, UAClientPanic);
      signal(SIGSEGV, UAClientPanic);
#endif
   }

   m_pData = new EDF();
   m_pUser = NULL;

   m_szFile = NULL;
   m_szProtocol = NULL;

   m_pOptions = NULL;
   m_iNumOptions = 0;

   m_bRequest = false;

   m_pUserList = NULL;
   m_pFolderList = NULL;

   // Set standard options
   addOption("server", OPT_STRING, true);
   addOption("port", OPT_INT), true;
   addOption("timeout", OPT_INT);
   addOption("username", OPT_STRING);
   addOption("password", OPT_STRING);
   // addOption("hostname", OPT_STRING);
   // addOption("address", OPT_STRING);
#ifdef CONNSECURE
   addOption("secure", OPT_BOOL);
   addOption("certificate", OPT_STRING);
#endif
   addOption("force", OPT_BOOL);
   addOption("busy", OPT_BOOL);
   addOption("silent", OPT_BOOL);
   addOption("shadow", OPT_BOOL);
   addOption("nocontact", OPT_BOOL);
   addOption("statusmsg", OPT_STRING);
   addOption("debuglevel", OPT_INT);
   addOption("debugfile", OPT_STRING);
   addOption("debugdir", OPT_STRING);
   addOption("loglevel", OPT_INT);

   debug(DEBUGLEVEL_INFO, "UAClient::UAClient exit\n");
}

UAClient::~UAClient()
{
   int iOptionNum = 0;

   debug(DEBUGLEVEL_INFO, "UAClient::~UAClient entry\n");

   shutdown();

   delete m_pData;
   delete m_pUser;

   delete[] m_szFile;

   for(iOptionNum = 0; iOptionNum < m_iNumOptions; iOptionNum++)
   {
      delete[] m_pOptions[iOptionNum]->m_szName;
      delete m_pOptions[iOptionNum];
   }
   delete[] m_pOptions;

   debug(DEBUGLEVEL_INFO, "UAClient::~UAClient exit\n");
}

bool UAClient::config(EDF *pConfig)
{
   return true;
}

bool UAClient::startup(int argc, char **argv)
{
   return true;
}

bool UAClient::shutdown()
{
   STACKTRACE
   int iTime = 0;
   bool bLoop = false;
   char *szType = NULL, *szMessage = NULL;
   EDF *pRead = NULL;

   debug(DEBUGLEVEL_INFO, "UAClient::shutdown entry\n");

   if(m_pUser != NULL)
   {
      debug(DEBUGLEVEL_INFO, "UAClient::shutdown logging out\n");
		UAClient::request(MSG_USER_LOGOUT);

      delete m_pUser;
      m_pUser = NULL;

      // Loop until disconnected properly
      iTime = time(NULL);
      Timeout(100);
      while(bLoop == true)
      {
         // if(Read(&pRead) == true)
         pRead = Read();
         if(pRead != NULL)
         {
            szType = NULL;
            szMessage = NULL;

            pRead->Root();
            pRead->Get(&szType, &szMessage);
            if(szType != NULL && szMessage != NULL)
            {
               if(stricmp(szType, "reply") == 0 && stricmp(szMessage, MSG_USER_LOGOUT) == 0)
               {
                  debug(DEBUGLEVEL_INFO, "UAClient::shutdown quit got %s reply\n", MSG_USER_LOGOUT);
                  bLoop = false;
               }
            }
            delete[] szType;
            delete[] szMessage;

            delete pRead;
         }

         if(State() != OPEN)
         {
            debug(DEBUGLEVEL_INFO, "UAClient::shutdown quit state not open\n");
            bLoop = false;
         }
         else if(time(NULL) > iTime + 3)
         {
            debug(DEBUGLEVEL_INFO, "UAClient::shutdown quit state timeout\n");
            bLoop = false;
         }
      }
   }

   Disconnect();

   debug(DEBUGLEVEL_INFO, "UAClient::shutdown exit true\n");
   return true;
}

bool UAClient::connected()
{
   return true;
}

bool UAClient::login(EDF *pRequest)
{
   return true;
}

bool UAClient::loggedIn()
{
   return true;
}

bool UAClient::notLoggedIn()
{
   debug(DEBUGLEVEL_INFO, "UAClient::notLoggedIn\n");
   return true;
}

bool UAClient::reply(const char *szReply, EDF *pReply)
{
   return false;
}

bool UAClient::announce(const char *szAnnounce, EDF *pAnnounce)
{
   return true;
}

UAClient::BackgroundReturn UAClient::background()
{
   return BG_NONE;
}

void UAClient::SetPanic(UACLIENTPANIC pPanic)
{
   g_pPanic = pPanic;
}

bool UAClient::requestMode()
{
   return m_bRequest;
}

bool UAClient::requestMode(bool bRequest)
{
   bool bReturn = m_bRequest;

   m_bRequest = bRequest;

   return bReturn;
}

int UAClient::UserGet(char *szName)
{
   STACKTRACE
   int iReturn = 0;
   char *szTemp = strmk(szName);

   // Setup user list and run a find

   userList();

   iReturn = ::UserGet(m_pUserList, szTemp, true);
   debug(DEBUGLEVEL_DEBUG, "UAClient::UserGet %s -> %d(%s)\n", szName, iReturn, szTemp);

   delete[] szTemp;

   return iReturn;
}

bool UAClient::UserGet(int iUserID, char **szName)
{
   STACKTRACE
   bool bReturn = false;

   // Setup user list and run a find

   userList();

   bReturn = ::UserGet(m_pUserList, iUserID, szName, true);
   debug(DEBUGLEVEL_DEBUG, "UAClient::UserGet %d -> %s %s\n", iUserID, BoolStr(bReturn), szName != NULL ? *szName : NULL);
   return bReturn;
}

int UAClient::FolderGet(char *szName)
{
   STACKTRACE
   int iReturn = 0;
   char *szTemp = strmk(szName);

   // Setup folder list and run a find

   folderList();

   iReturn = ::FolderGet(m_pFolderList, szTemp, true);
   debug(DEBUGLEVEL_DEBUG, "UAClient::FolderGet %s -> %d(%s)\n", szName, iReturn, szTemp);

   delete[] szTemp;

   return iReturn;
}

bool UAClient::FolderGet(int iFolderID, char **szName)
{
   STACKTRACE
   bool bReturn = false;

   // Setup folder list and run a find

   folderList();

   bReturn = ::FolderGet(m_pFolderList, iFolderID, szName, true);
   debug(DEBUGLEVEL_DEBUG, "UAClient::FolderGet %d -> %s %s\n", iFolderID, BoolStr(bReturn), szName != NULL ? *szName : NULL);
   return bReturn;
}

bool UAClient::IsFolderPrivate(int iFolderID)
{
   STACKTRACE
   bool bReturn = false;
   int iAccessMode = FOLDERMODE_NORMAL;
   
   // Setup folder list and run a find
   
   folderList();
   
   if(::FolderGet(m_pFolderList, iFolderID, NULL, false) == true)
   {
      m_pFolderList->GetChild("accessmode", &iAccessMode);
      bReturn = mask(iAccessMode, ACCMODE_PRIVATE);
   }
   
   return bReturn;
}


bool UAClient::FolderSubscribe(int iFolderID, int iUserID, int iSubType)
{
   STACKTRACE
   bool bReturn = false;
   EDF *pRequest = NULL, *pReply = NULL;

   debug("UAClient::FolderSubscribe %d %d %d\n", iFolderID, iUserID, iSubType);

   pRequest = new EDF();
   pRequest->AddChild("folderid", iFolderID);
   if(iUserID > 0)
   {
      pRequest->AddChild("userid", iUserID);
   }
   if(iSubType != SUBTYPE_SUB)
   {
      pRequest->AddChild("subtype", iSubType);
   }

   EDFParser::debugPrint("UAClient::FolderSubscribe request", pRequest);
   bReturn = UAClient::request(MSG_FOLDER_SUBSCRIBE, pRequest, &pReply);
   EDFParser::debugPrint("UAClient::FolderSubscribe reply", pReply);

   delete pRequest;

   delete pReply;

   return bReturn;
}

bool UAClient::request(const char *szRequest, EDF **pReply)
{
   return UAClient::request(szRequest, NULL, pReply);
}

bool UAClient::request(const char *szRequest, EDF *pRequest, EDF **pReply)
{
   STACKTRACE
   int iLogLevel = 0;
   bool bRequest = requestMode(true), bReturn = false;
   double dTick = 0;
   char *szReply = NULL;
   EDF *pTemp1 = NULL, *pTemp2 = NULL;

   debug(DEBUGLEVEL_DEBUG, "UAClient::request entry %s\n", szRequest);
   EDFParser::debugPrint(DEBUGLEVEL_DEBUG, pRequest);

   m_pData->GetChild("loglevel", &iLogLevel);

   // Create an empty request if there isn't one
   if(pRequest == NULL)
   {
      pTemp1 = new EDF();
   }
   else
   {
      pTemp1 = pRequest;
   }

   pTemp1->Root();
   pTemp1->Set("request", szRequest);
   if(iLogLevel >= 1)
   {
      EDFParser::debugPrint("UAClient::request EDF", pTemp1);
   }

   // Try sending request
   dTick = gettick();
   if(Write(pTemp1) == false)
   {
      // Write failed
      m_pData->Root();
      m_pData->SetChild("quit", true);

      requestMode(bRequest);

      debug(DEBUGLEVEL_DEBUG, "UAClient::request exit false, write failed\n");
      return false;
   }

   // Clean up temp request
   if(pTemp1 != pRequest)
   {
      delete pTemp1;
   }

   loop(&pTemp2);

   if(State() != OPEN)
   {
      requestMode(bRequest);

      return false;
   }

   pTemp2->Get(NULL, &szReply);
   if(stricmp(szRequest, szReply) == 0)
   {
      bReturn = true;
   }
   delete[] szReply;

   debug(DEBUGLEVEL_DEBUG, "UAClient::request exit true, %s %ld ms\n", szRequest, tickdiff(dTick));
   EDFParser::debugPrint(DEBUGLEVEL_DEBUG, pTemp2);

   if(iLogLevel >= 1)
   {
      EDFParser::debugPrint("UAClient::reply EDF", pTemp2);
   }

   if(pReply != NULL)
   {
      *pReply = pTemp2;
   }
   else
   {
      delete pTemp2;
   }

   requestMode(bRequest);

   return bReturn;
}

int UAClient::version(const char *szVersion)
{
   STACKTRACE
   return ProtocolCompare(m_szProtocol, szVersion);
}

bool UAClient::run(int argc, char **argv, EDF *pConfig, bool bLoop)
{
   STACKTRACE
   int iHelp = 0, iOptionNum = 0, iArgNum = 1, iStatus = 0, iTimeout = 1000, iUserID = -1, iPort = 0, iAttachmentSize = 0, iDebugLevel = 0;
   bool bReturn = true, bDefConfig = false, bError = false;
   char *szConfig = NULL, *szReply = NULL, *szType = NULL, *szDebugFile = NULL, *szDebugDir = NULL, *szServer = NULL, *szCertificate = NULL, *szUsername = NULL;
   char *szHostname = NULL, *szAddress = NULL;
   char szFilename[200];
   EDF *pRequest = NULL, *pReply = NULL, *pFile = NULL, *pSystemList = NULL;

   debug("UAClient::run %d %p %p %s\n", argc, argv, pConfig, BoolStr(bLoop));

   if(argc == 1 && CLIENT_BASENAME() != NULL)
   {
      // Get the default config file based on EXE name
      szConfig = new char[200];
#ifdef UNIX
      sprintf(szConfig, "%s/.%src", getenv("HOME"), CLIENT_BASENAME());
#else
      sprintf(szConfig, "%s.edf", CLIENT_BASENAME());
#endif
      bDefConfig = true;
   }
   else if(argc >= 2)
   {
      if(strcmp(argv[1], "--help") == 0)
      {
         iHelp = 1;
      }
      else if(strcmp(argv[1], "--version") == 0)
      {
         iHelp = 2;
      }
      else if(argv[1][0] != '-')
      {
         szConfig = argv[1];
         iArgNum++;
      }
   }
   else if(pConfig == NULL)
   {
      iHelp = 1;
   }

   if(iHelp == 1)
   {
      printf("Usage:\n");
      printf("  --help\n");
      printf("  --version\n");
      printf("\n");

      printf("  [<config>] [options]\n");
      printf("\n");

      for(iOptionNum = 0; iOptionNum < m_iNumOptions; iOptionNum++)
      {
         if(m_pOptions[iOptionNum]->m_iType == OPT_BOOL)
         {
            szType = "";
         }
         if(m_pOptions[iOptionNum]->m_iType == OPT_STRING)
         {
            szType = " <string>";
         }
         if(m_pOptions[iOptionNum]->m_iType == OPT_INT)
         {
            szType = " <number>";
         }

         printf("  -%s%s\n", m_pOptions[iOptionNum]->m_szName, szType);
      }

      debug("UAClient::run exit false\n");
      return false;
   }
   else if(iHelp == 2)
   {
      help();

      debug("UAClient::run exit false\n");
      return false;
   }

   debug("UAClient::run entry argc=%d argv=%p config=%s/%p loop=%s\n", argc, argv, szConfig, pConfig, BoolStr(bLoop));

   // printf("UAClient::run config file %s\n", szConfig);

   if(szConfig != NULL || pConfig != NULL)
   {
      debug("UAClient::run config file %s object %p\n", szConfig, pConfig);

      if(pConfig != NULL)
      {
         pFile = pConfig;
      }
      else
      {
         pFile = EDFParser::FromFile(szConfig);
      }

      debug("UAClient::run file %p\n", pFile);

      if(pFile != NULL)
      {
         // EDFPrint("Config file", pFile);

         config(pFile);

         delete m_pData;

         if(pFile != pConfig)
         {
            m_pData = pFile;
         }
         else
         {
            m_pData = new EDF(pFile);
         }
      }
      else if(bDefConfig = false)
      {
         debug("UAClient::run exit false, no config file %s (%s)\n", szConfig, strerror(errno));
         return false;
      }
   }

   debug("UAClient::run check args %d %d\n", iArgNum, argc);
   while(iArgNum < argc)
   {
      debug("UAClient::run arg %d %s", iArgNum, argv[iArgNum]);

      iOptionNum = 0;
      while(iOptionNum < m_iNumOptions)
      {
         if(argv[iArgNum][0] == '-' && stricmp(argv[iArgNum] + 1, m_pOptions[iOptionNum]->m_szName) == 0)
         {
            debug(", type %d", m_pOptions[iOptionNum]->m_iType);
            if(m_pOptions[iOptionNum]->m_iType == OPT_BOOL)
            {
               debug(", bool true");
               m_pData->SetChild(m_pOptions[iOptionNum]->m_szName, true);

               iOptionNum = m_iNumOptions;
            }
            else if(m_pOptions[iOptionNum]->m_iType == OPT_STRING || m_pOptions[iOptionNum]->m_iType == OPT_INT)
            {
               if(iArgNum < argc - 1)
               {
                  iArgNum++;

                  if(m_pOptions[iOptionNum]->m_iType == OPT_STRING)
                  {
                     debug(", str %s", argv[iArgNum]);
                     m_pData->SetChild(m_pOptions[iOptionNum]->m_szName, argv[iArgNum]);
                  }
                  else
                  {
                     debug(", int %d", atoi(argv[iArgNum]));
                     m_pData->SetChild(m_pOptions[iOptionNum]->m_szName, atoi(argv[iArgNum]));
                  }
               }
               else
               {
                  debug(", no value");
               }

               iOptionNum = m_iNumOptions;
            }
         }
         else
         {
            iOptionNum++;
            if(iOptionNum == m_iNumOptions)
            {
               debug(", unrecognised");
            }
         }
      }

      debug("\n");

      iArgNum++;
   }

   m_pData->Root();
   while(m_pData->DeleteChild("quit") == true);

   for(iOptionNum = 0; iOptionNum < m_iNumOptions; iOptionNum++)
   {
      if(m_pOptions[iOptionNum]->m_bMandatory == true && m_pData->IsChild(m_pOptions[iOptionNum]->m_szName) == false)
      {
         debug("UAClient::run missing option %s\n", m_pOptions[iOptionNum]->m_szName);
         bError = true;
      }
   }

   if(bError == true)
   {
      debug("UAClient::run exit false, missing option\n");
      return false;
   }

   debug("UAClient::run startup\n");
   if(startup(argc, argv) == false)
   {
      debug("UAClient::run exit false, startup failed\n");
      return false;
   }

   m_pData->Root();

   debug("UAClient::run debug setup\n");
   if(m_pData->GetChild("debuglevel", &iDebugLevel) == true)
   {
      debuglevel(iDebugLevel);
   }

   if(m_pData->GetChild("debugfile", &szDebugFile) == true)
   {
      m_pData->GetChild("debugdir", &szDebugDir);
      sprintf(szFilename, "%s/%s", szDebugDir != NULL ? szDebugDir : ".", szDebugFile);

      if(debugopen(szFilename, 077) == false)
      {
         debug(DEBUGLEVEL_INFO, "UAClient::run exit false, Unable to open debug file %s/%s (%s)\n", szDebugDir, szDebugFile, strerror(errno));

         delete[] szDebugFile;
         delete[] szDebugDir;

         return false;
      }

      delete[] szDebugFile;
      delete[] szDebugDir;
   }

   debugbuffer(false);

   m_pData->GetChild("server", &szServer);
   m_pData->GetChild("port", &iPort);
   m_pData->GetChild("certificate", &szCertificate);

   debug(DEBUGLEVEL_INFO, "UAClient::run connecting\n");
   if(Connect(szServer, iPort, m_pData->GetChildBool("secure"), szCertificate) == false)
   {
      debug(DEBUGLEVEL_INFO, "UAClient::run exit false, Unable to connect to server %s / port %d (%s)\n", szServer, iPort, Error());

      delete[] szServer;
      delete[] szCertificate;

      return false;
   }

   delete[] szServer;
   delete[] szCertificate;

   Timeout(100);

   while(State() == OPEN && Connected() == false)
   {
      Read();

      debug(DEBUGLEVEL_DEBUG, "UAClient::run state %d connected %s\n", State(), BoolStr(Connected()));
   }
   if(State() != OPEN)
   {
      debug(DEBUGLEVEL_INFO, "UAClient::run exit false, disconnected during EDF mode loop\n");
      return false;
   }
   debug(DEBUGLEVEL_INFO, "UAClient::run EDF mode on\n");

   m_pData->GetChild("timeout", &iTimeout);
   Timeout(iTimeout);

   UAClient::request(MSG_SYSTEM_LIST, &pSystemList);
   pSystemList->GetChild("protocol", &m_szProtocol);
   delete pSystemList;

   m_pData->Root();
   connected();

   m_pData->GetChild("username", &szUsername);

   // Auto-login if possible
   if(szUsername != NULL)
   {
      debug(DEBUGLEVEL_INFO, "UAClient::run attempt login as %s\n", szUsername);

      // debug("UAClient::run creating login request\n");
      pRequest = new EDF();
      if(login(pRequest) == true)
      {
         if(pRequest->IsChild("name") == false)
         {
            pRequest->SetChild("name", szUsername);
            // EDFPrint("UAClient::run post name add", pRequest);
         }

         if(pRequest->IsChild("password") == false)
         {
            pRequest->SetChild(m_pData, "password");
            // EDFPrint("UAClient::run post password add", pRequest);
         }

			GetHostInfo(&szHostname, &szAddress);

			if(szHostname != NULL)
			{
				pRequest->SetChild("hostname", szHostname);
			}
			if(szAddress != NULL)
			{
				pRequest->SetChild("address", szAddress);
			}

         pRequest->SetChild("client", CLIENT_NAME());
         if(CLIENT_BASENAME() != NULL)
         {
            pRequest->SetChild("clientbase", CLIENT_BASENAME());
         }
         pRequest->SetChild("protocol", PROTOCOL);

         // EDFPrint("UAClient::run post client / protocol add", pRequest);
         /* if(m_bClientDebug == true)
         {
            debugEDFPrint("UAClient::run login", pRequest);
         } */

         EDFParser::debugPrint("UAClient::run current data", m_pData);

         pRequest->GetChild("status", &iStatus);
         if(m_pData->GetChildBool("force") == true)
         {
            pRequest->AddChild("force", true);
         }
         if(m_pData->GetChildBool("busy") == true)
         {
            iStatus |= LOGIN_BUSY;
         }
         if(m_pData->GetChildBool("silent") == true)
         {
            iStatus |= LOGIN_SILENT;
         }
         if(m_pData->GetChildBool("shadow") == true)
         {
            iStatus |= LOGIN_SHADOW;
         }
         if(m_pData->GetChildBool("nocontact") == true)
         {
            iStatus |= LOGIN_NOCONTACT;
         }
         if(iStatus > 0)
         {
            pRequest->SetChild("status", iStatus);
         }

         login(pRequest);

         // EDFPrint("UAClient::run login", pRequest);
         // debug("UAClient::run logging in...\n");
         if(UAClient::request(MSG_USER_LOGIN, pRequest, &pReply) == true)
         {
            debug(DEBUGLEVEL_INFO, "UAClient::run logged in\n");

            if(pReply->GetChild("userid", &iUserID) == true)
            {
               pReply->DeleteChild("userid");
               pReply->Set("user", iUserID);
            }
            m_pUser = pReply;

            loggedIn();
         }
         else
         {
            EDFParser::debugPrint("UAClient::run login failed", pReply);

            if(pReply != NULL)
            {
               pReply->Get(NULL, &szReply);
               if(reply(szReply, pReply) == false)
               {
                  defaultreply(szReply, pReply);
               }
               delete[] szReply;

               delete pReply;
            }

            bReturn = false;
         }
      }
      // debug("UAClient::run deleting login request\n");
      delete pRequest;
      // debug("UAClient::run login request deleted\n");
   }
   else
   {
      notLoggedIn();
   }

   delete[] szUsername;

   if(Connected() == true && bLoop == true)
   {
      while(loop() == true);

      m_pData->Root();
      shutdown();
   }

   debug(DEBUGLEVEL_INFO, "UAClient::run exit %s\n", BoolStr(bReturn));
   return bReturn;
}

bool UAClient::loop()
{
   return loop(NULL);
}

void UAClient::help()
{
   printf("%s (protocol %s)\n", CLIENT_NAME(), PROTOCOL);
}

/* bool UAClient::debug(bool bDebug)
{
   return true;
} */

char *UAClient::CLIENT_BASENAME()
{
   return NULL;
}

void UAClient::addOption(char *szName, CommandLineOption iType, bool bMandatory)
{
   option *pOption = NULL, **pTemp = NULL;

   pOption = new option;
   pOption->m_szName = strmk(szName);
   pOption->m_iType = iType;
   pOption->m_bMandatory = bMandatory;

   ARRAY_INSERT(option *, m_pOptions, m_iNumOptions, pOption, m_iNumOptions, pTemp)
}

bool UAClient::loop(EDF **pReply)
{
   STACKTRACE
   BackgroundReturn iBackground = BG_NONE;
   int iLogLevel = 0;
   long lEntry = 0, lExit = 0, lPostRead = 0, lMinus = 0;
   bool bReturn = true, bQuit = false, bReply = false;
   double dTick = 0;
   char *szType = NULL, *szMessage = NULL;
   EDF *pRead = NULL;

   // debug("UAClient::loop entry\n");

   m_pData->GetChild("loglevel", &iLogLevel);

   lEntry = memusage();

   // Check for a message from the server
   dTick = gettick();

   do
   {
      // debug("UAClient::loop state %d == %d, reply %p != NULL, reply %s == false\n", State(), OPEN, pReply, BoolStr(bReply));

      pRead = Read();
      // debug("UAClient::loop read %s in %ld ms\n", BoolStr(bRead), tickdiff(dTick));
      // debug("UAClient::loop read return %p\n", pRead);

      lPostRead = memusage();
      // debug("UAClient::loop read difference %ld bytes\n", lDiff);

      if(pRead != NULL)
      {
         EDFParser::debugPrint(DEBUGLEVEL_DEBUG, "UAClient::loop read", pRead);

         pRead->Root();
         pRead->Get(&szType, &szMessage);
         lMinus = memusage() - lPostRead;
         // debug("UAClient::loop minus %ld %ld %ld\n", lEntry, lPostRead, lMinus);

         if(szType != NULL)
         {
            if(stricmp(szType, "reply") == 0)
            {
               // Process reply
               /* if(m_bClientDebug == true)
               {
                  debugEDFPrint("UAClient: Reply message", pRead, false);
               } */
               if(m_pUser != NULL)
               {
                  m_pUser->Root();
               }

               // debug("UAClient::loop reply '%s'\n", szMessage);

               /* if(stricmp(szMessage, MSG_USER_LOGIN) == 0)
               {
                  // debug("UAClient::loop logged in\n");

                  m_pUser = new EDF();
                  m_pUser->Copy(pRead, true, true);
                  if(m_pUser->GetChild("userid", &iUserID) == true)
                  {
                     m_pUser->DeleteChild("userid");
                     m_pUser->Set("user", iUserID);
                  }

                  loggedIn();
               }
               else */
               {
                  if(pReply != NULL)
                  {
                     *pReply = pRead;
                     bReply = true;
                  }
                  else if(reply(szMessage, pRead) == false)
                  {
                     defaultreply(szMessage, pRead);
                  }
               }
            }
            else if(stricmp(szType, "announce") == 0)
            {
               // Process asynchronous event
               /* if(m_bClientDebug == true)
               {
                  debugEDFPrint("UAClient: Announce message", pRead, false);
               } */

               // debug("UAClient::loop announce '%s'\n", szMessage);

               if(iLogLevel >= 2)
               {
                  EDFParser::debugPrint("UAClient::loop announce EDF", pRead);
               }

               m_pData->Root();
               announce(szMessage, pRead);
            }
            else
            {
               EDFParser::debugPrint("UAClient: Unknown message", pRead);
               m_pData->Root();
               m_pData->SetChild("quit", true);
            }
         }

         if(bReply == false)
         {
            delete pRead;
         }
      }
   }
   while(State() == OPEN && pReply != NULL && bReply == false);

   // debug("UAClient::loop state %d\n", State());
   if(State() != OPEN)
   {
      bQuit = true;
      bReturn = false;
   }
   else if(pReply == NULL)
   {
      m_pData->Root();
      // debug("UAClient::loop calling background\n");
      iBackground = background();
		if(iBackground == BG_INVALID)
		{
			debug("UAClient::loop invalid background return\n");
		}
      else if(mask(iBackground, BG_WRITE) == true)
      {
         // Write config file
         EDFParser::ToFile(m_pData, m_szFile);
      }

      m_pData->Root();
      bQuit = m_pData->GetChildBool("quit");
   }

   // debug("UAClient::loop quit check %d %s\n", iBackground, BoolStr(bQuit));
   if(State() == OPEN && (mask(iBackground, BG_QUIT) == true || bQuit == true))
   {
      shutdown();
      bReturn = false;
   }

   lExit = memusage() - lMinus;
   if(lEntry != lExit)
   {
      /* debug("UAClient::loop memusage");
      if(szType != NULL || szMessage != NULL)
      {
         debug("(%s/%s)", szType, szMessage);
      }
      debug(" %ld -> %ld (%ld)\n", lEntry, lExit, lExit - lEntry); */
   }

   delete[] szType;
   delete[] szMessage;

   // debug("UAClient::loop exit return %s\n", BoolStr(bReturn));

   fflush(stdout);

   return bReturn;
}

bool UAClient::defaultreply(const char *szReply, EDF *pReply)
{
   if(stricmp(szReply, MSG_USER_LOGIN_INVALID) == 0)
   {
      m_pData->Root();
      m_pData->SetChild("quit", true);
   }

   return true;
}

bool UAClient::userList()
{
    STACKTRACE

     if(m_pUserList == NULL)
     {
	if(request(MSG_USER_LIST, &m_pUserList) == false)
	{
	   return false;
	}

	m_pUserList->Sort("user", "name");
     }

   return true;
}


bool UAClient::folderList()
{
   STACKTRACE
   EDF *pRequest = NULL;

   if(m_pFolderList == NULL)
   {
      pRequest = new EDF();
      pRequest->AddChild("searchtype", 2);

      if(request(MSG_FOLDER_LIST, pRequest, &m_pFolderList) == false)
      {
	 delete pRequest;

	 return false;
      }

      delete pRequest;

      m_pFolderList->Sort("folder", "name");
   }
   
   m_pFolderList->Root();

   return true;
}
