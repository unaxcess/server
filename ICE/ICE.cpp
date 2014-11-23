/*
** ICE - Internet Communications Engine
** (c) 1998 Michael Wood (mike@compsoc.man.ac.uk)
**
** ICE.cpp: Main wrapper for server
*/

// Standard headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#ifdef UNIX
#include <dlfcn.h>
#include <unistd.h>
#endif
#include <errno.h>
#include <signal.h>
#include <ctype.h>

// Class headers
#include "common/EDFConn.h"

#include "ICE.h"

// File handle constants
#define BLOCK 500
#define MAXPOINT 60

// Difference reporting
// #define MAX_TICK 100
#define MAX_TICK 0
// #define MIN_DIFF 5000
#define MIN_DIFF 0
// #define INPUT_SIZE 2000

#ifndef SIGUNUSED
#define SIGUNUSED 31
#endif

// Library stuff
#ifdef UNIX
typedef void * HMODULE;
#endif
HMODULE m_pLibrary = NULL;
LIBSERVERFN m_pServerStartup = NULL, m_pServerUnload = NULL, m_pServerShutdown = NULL;
LIBSERVERTMFN m_pServerLoad = NULL, m_pServerWrite = NULL;
LIBSERVERBGFN m_pServerBackground = NULL;
LIBCONNFN m_pConnOpen = NULL, m_pConnClose = NULL, m_pConnLost = NULL, m_pConnTimeout = NULL;
LIBCONNBGFN m_pConnBackground = NULL;
LIBCONNREQUESTFN m_pConnRequest = NULL;
LIBINTERREADFN m_pInterRead = NULL;
LIBINTERWRITEFN m_pInterWrite = NULL;
int iPanic = 0;

char *m_szConfigFile = NULL, *m_szDataFile = NULL;

// void *LibFunction(char *szFunction, bool bError = true);
bool LibFunctions(EDF *pConfig);

// Variables defined in file scope so they can be access by the signal handler
int m_iNumClients = 0;
EDFConn **m_pClients = NULL, *m_pServer = NULL;
EDF *m_pData = NULL, *m_pIn = NULL;

void EDFBytes(char *szTitle, EDF *pEDF)
{
   bytes *pBytes = EDFParser::Write(pEDF, EDFElement::EL_ROOT);
   bytesprint(szTitle, pBytes, false);
}

// Write data to a file
bool WriteData(bool bBackup, char *szFilename, LIBSERVERTMFN pServerWrite)
{
   STACKTRACE
   int iDebug = 0;
   bool bReturn = false;
   long lDiff = 0;
   double dTick = 0;

   if(bBackup == true)
   {
      rename(m_szDataFile, "backup.edf");
   }

   // EDFPrint("WriteData", m_pData);

   // EDF::Debug(true);
   iDebug = debuglevel(DEBUGLEVEL_DEBUG);
   dTick = gettick();
   bReturn = EDFParser::ToFile(m_pData, szFilename);
   lDiff = tickdiff(dTick);
   // EDF::Debug(false);
   debuglevel(iDebug);

   // EDFBytes("Info: WriteData", m_pData);

   if(bReturn == false)
   {
      debug("WriteData failed to write data, %s\n", strerror(errno));
      return false;
   }
   else
   {
      debug("WriteData data written in %ld ms\n", lDiff);
   }

   if(pServerWrite != NULL)
   {
      m_pData->Root();
      (*pServerWrite)(m_pData, lDiff, 0);
   }

   return true;
}

void memstats(char *szTitle, char *szDiff = NULL, int iDiff = 0)
{
   int iMemUsage = 0;
#ifdef LEAKTRACEON
   long lNumAllocs = 0, lTotalAllocs = 0;
   leakGetAllocs(&lNumAllocs, &lTotalAllocs);
#endif

   iMemUsage = memusage();

   debug("Info(%ld): memusage(%s) %u", time(NULL), szTitle, iMemUsage);
#ifdef LEAKTRACEON
   debug(" (%ld allocs of %ld bytes)", lNumAllocs, lTotalAllocs);
#endif
   if(szDiff != NULL)
   {
      debug(", %s%sdiff %d", szDiff, strlen(szDiff) > 0 ? " " : "", iMemUsage - iDiff);
   }

   debug("\n");
}

// Perform emergency shutdown on program error
#ifdef UNIX
#ifdef SIGACTION
void Panic(int iSignal, siginfo_t *pInfo = NULL, void *pPanic = NULL)
{
   int iDebug = 0;

   debug("Panic entry %d, e=%d t=%d %p %p", iSignal, errno, tiume(NULL), pInfo, pPanic);
   if(pInfo != NULL)
   {
      debug(", %d %p", pInfo->si_code,pInfo->si_addr);
   }
   debug("\n");
#else
void Panic(int iSignal)
{
   int iDebug = 0;

   debug("Panic entry %d, e=%d t=%d\n", iSignal, errno, time(NULL));
#endif
#else
void Panic(int iSignal)
{
   int iDebug = 0;

   debug("Panic entry %d, e=%d t=%d e2=%d\n", iSignal, errno, time(NULL), GetLastError());
#endif

   if(iPanic != 0)
   {
      debug("Panic multi panic\n");
      exit(3);
   }

   iPanic++;

   STACKTRACEPRINT

   if(iSignal != SIGINT)
   {
      // EDF::Debug(true);
      iDebug = debuglevel(DEBUGLEVEL_DEBUG);
      EDFParser::ToFile(m_pData, "panic.edf");
      // EDF::Debug(false);
      debuglevel(iDebug);

      // EDFBytes("Info: Panic", m_pData);
   }

   // delete m_pServer;

   if(m_pIn != NULL)
   {
      EDFParser::debugPrint("Panic input", m_pIn);
   }

   debug("Panic exit\n");
   exit(2);
}

#ifdef UNIX
#define LIBFUNCTIONPROC(szFunction) dlsym(m_pLibrary, szFunction)
#else
#define LIBFUNCTIONPROC(szFunction) GetProcAddress(m_pLibrary, szFunction)
#endif

#ifdef UNIX
#define LIBFUNCTIONERR(szFunction) printf("Error: Unable to find %s function, %s\n", szFunction, dlerror())
#else
#define LIBFUNCTIONERR(szFunction) printf("Error: Unable to find %s function, %d\n", szFunction, GetLastError())
#endif \

#define LIBFUNCTION(pFunction, RETTYPE, szFunction, bError) \
pFunction = (RETTYPE)LIBFUNCTIONPROC(szFunction); \
debug("LibFunction %s, %p\n", szFunction, pFunction); \
if(pFunction == NULL && bError == true) \
{ \
   LIBFUNCTIONERR(szFunction); \
   return false; \
}

bool LibFunctions(EDF *pConfig)
{
   STACKTRACE
   char *szLibrary = NULL;

#ifdef UNIX
#ifndef LEAKTRACEON
   if(m_pLibrary != NULL)
   {
      debug("LibFunctions closing %p\n", m_pLibrary);
      dlclose(m_pLibrary);
   }
   pConfig->GetChild("library", &szLibrary);
#endif
   m_pLibrary = dlopen(szLibrary, RTLD_NOW);
#else
   if(m_pLibrary == NULL)
   {
      m_pLibrary = GetModuleHandle(NULL);
   }
#endif
   debug("LibFunctions library load %s, %p\n", szLibrary, m_pLibrary);

   if(m_pLibrary == NULL)
   {
#ifdef UNIX
      debug("Error: Failed to load function library %s, %s\n", szLibrary != NULL ? szLibrary : "[internal]", dlerror());
#else
      debug("Error: Failed to load function library %s, %d\n", szLibrary != NULL ? szLibrary : "[internal]", GetLastError());
#endif
      return false;
   }
   delete[] szLibrary;

   // Get server functions
   LIBFUNCTION(m_pServerStartup, LIBSERVERFN, "ServerStartup", true);
   LIBFUNCTION(m_pServerLoad, LIBSERVERTMFN, "ServerLoad", true);
   LIBFUNCTION(m_pServerUnload, LIBSERVERFN, "ServerUnload", true);
   LIBFUNCTION(m_pServerShutdown, LIBSERVERFN, "ServerShutdown", true);
   LIBFUNCTION(m_pServerWrite, LIBSERVERTMFN, "ServerWrite", true);
   LIBFUNCTION(m_pServerBackground, LIBSERVERBGFN, "ServerBackground", true);

   // Get connection functions
   LIBFUNCTION(m_pConnOpen, LIBCONNFN, "ConnectionOpen", true);
   LIBFUNCTION(m_pConnClose, LIBCONNFN, "ConnectionClose", true);
   LIBFUNCTION(m_pConnLost, LIBCONNFN, "ConnectionLost", true);
   LIBFUNCTION(m_pConnTimeout, LIBCONNFN, "ConnectionTimeout", false);
   LIBFUNCTION(m_pConnRequest, LIBCONNREQUESTFN, "ConnectionRequest", true);
   LIBFUNCTION(m_pConnBackground, LIBCONNBGFN, "ConnectionBackground", true);

   // Get interface functions
   LIBFUNCTION(m_pInterRead, LIBINTERREADFN, "InterfaceRead", false);
   LIBFUNCTION(m_pInterWrite, LIBINTERWRITEFN, "InterfaceWrite", false);

   return true;
}

// EDF Server command messages
bool EDFMessageProcess(EDF *&pConfig, EDFConn *pConn, EDF *pIn, EDF *pOut, char *szMessage)
{
   STACKTRACE
   int iTimeout = 0, iScope = 0, iDebug = 0;
   bool bReturn = true;
   bool bScope = true;
   double dTick = 0;
   EDF *pReload = NULL;

   // EDFPrint("EDFMessageProcess EDF control", pIn);

   if(stricmp(szMessage, "on") == 0 || stricmp(szMessage, "echo") == 0)
   {
      // Echo back to the server
      // printf("Info: echo check\n", time(NULL));
      pOut->Set("edf", szMessage);
   }
   else
   {
      pConfig->Root();
      if(pConfig->Child("edf") == true)
      {
         if(m_pConnRequest != NULL)
         {
            // Check that the request can be processed in the connection's current state
            pConfig->GetChild("scope", &iScope);

            m_pData->Root();
            bScope = (*m_pConnRequest)(pConn, m_pData, szMessage, pIn, iScope);
            debug("EDFMessageProcess request %s (%d)\n", BoolStr(bScope), iScope);
         }
      }

      if(bScope == true)
      {
         if(stricmp(szMessage, "reload") == 0)
         {
            // Reload config and library
            pOut->Set("edf", "reload");

            debug("EDFMessageProcess reloading config\n");

            iDebug = debuglevel(DEBUGLEVEL_DEBUG);

            pReload = EDFParser::FromFile(m_szConfigFile);
            if(pReload == NULL)
            {
               debug("Error: Failed to reload %s\n", m_szConfigFile);
               pOut->SetChild("fail", true);
            }
            else
            {
               // Replace the current config
               delete pConfig;
               pConfig = pReload;

               // Reset timeout
               pConfig->GetChild("timeout", &iTimeout);
               if(iTimeout > 0)
               {
                  m_pServer->Timeout(iTimeout);
               }

#ifdef UNIX
               // Stop Windows from reloading

               dTick = gettick();

               m_pData->Root();

               (*m_pServerUnload)(m_pData, ICELIB_RELOAD);

               WriteData(true, "reload.edf", NULL);

               if(LibFunctions(pConfig) == false)
               {
                  Panic(0);
               }

               m_pData->Root();

               (*m_pServerLoad)(m_pData, tickdiff(dTick), ICELIB_RELOAD);

               WriteData(false, m_szDataFile, NULL);
#endif
            }

            debuglevel(iDebug);
         }
      }
      else
      {
         pOut->Set("edf", "rq_invalid");
      }
   }

   // EDFPrint("EDF control message out", pOut);
   // printf("EDFMessageProcess exit %s\n", BoolStr(bReturn));
   return bReturn;
}

// Process a message of the form <request="...">
void RequestMessageProcess(EDF *&pConfig, EDFConn *pConn, EDF *pIn, EDF *pOut, char *szMessage)
{
   STACKTRACE
   int iScope = 0;
   bool bScope = true, bProcess = false;
   char *szFunction = NULL, *szReply = NULL;
   LIBMSGFN pMsgProcess = NULL;

   debug(DEBUGLEVEL_DEBUG, "RequestMessageProcess entry %s, %d %p\n", szMessage, pConn->State(), pConn->Data());
   EDFParser::debugPrint(DEBUGLEVEL_DEBUG, pIn);
   // EDFPrint(pIn, false, false);
   // EDFPrint("RequestMessageProcess", pIn);

   // Find library function
   pConfig->Root();
   if(pConfig->Child("request", szMessage) == true)
   {
      pConfig->GetChild("function", &szFunction);
      // printf("LibMsg function %s\n", szFunction);
      if(szFunction != NULL)
      {
#ifdef UNIX
         pMsgProcess = (LIBMSGFN)dlsym(m_pLibrary, szFunction);
#else
         pMsgProcess = (LIBMSGFN)GetProcAddress(m_pLibrary, szFunction);
#endif
         if(pMsgProcess == NULL)
         {
#ifdef UNIX
            debug("Error: RequestMessageProcess unable to sym %s, %s\n", szFunction, dlerror());
#else
            debug("Error: RequestMessageProcess unable to sym %s, %d\n", szFunction, GetLastError());
#endif
         }
         delete[] szFunction;
      }
   }

   STACKTRACEUPDATE

   if(pMsgProcess == NULL)
   {
      // Invalid message
      pOut->Set("reply", MSG_RQ_INVALID);
      pOut->AddChild("request", szMessage);
   }
   else
   {
      if(m_pConnRequest != NULL)
      {
         // Check that the request can be processed in the connection's current state
         pConfig->GetChild("scope", &iScope);

         m_pData->Root();
         bScope = (*m_pConnRequest)(pConn, m_pData, szMessage, pIn, iScope);
      }
      STACKTRACEUPDATE
      if(bScope == true)
      {
         // Process the request
         // m_pData->SetCurr((EDFElement *)m_pConn->ID());
         // EDFPrint("RequestMessageProcess ID'd EDF", m_pData, false);
         // printf("RequestProcessMessage %p\n", pMsgProcess);

         m_pData->Root();
         bProcess = (*pMsgProcess)(pConn, m_pData, pIn, pOut);
         STACKTRACEUPDATE

         pOut->Root();
         if(bProcess == true)
         {
            pOut->Set("reply", szMessage);
         }
         else
         {
            pOut->Get(NULL, &szReply);
            if(szReply == NULL)
            {
               debug("Info: RequestMessageProcess NULL reply\n");
               pOut->Set("reply", MSG_RQ_INVALID);
            }
            delete[] szReply;
         }

         // EDFPrint("RequestMessageProcess output", pOut);
      }
      else
      {
         // Invalid request
         pOut->Root();
         pOut->Set("reply", MSG_RQ_INVALID);
         pOut->AddChild("request", szMessage);
         pOut->AddChild("scope", iScope);
      }
   }

   /* if(pOut->Children() <= 10)
   {
   debugEDFPrint("RequestMessage exit", pOut);
   }
   else
   {
   debug("RequestMessage exit, %d children in output\n", pOut->Children());
   } */
}

int ConnCount()
{
   return m_iNumClients;
}

EDFConn *ConnList(int iConnNum)
{
   if(iConnNum < 0 || iConnNum >= m_iNumClients)
   {
      return NULL;
   }

   return m_pClients[iConnNum];
}

double ConnSent()
{
   int iConnNum = 0;
   double dReturn = 0;

   for(iConnNum = 0; iConnNum < m_iNumClients; iConnNum++)
   {
      dReturn += m_pClients[iConnNum]->Sent();
   }

   return dReturn;
}

double ConnReceived()
{
   int iConnNum = 0;
   double dReturn = 0;

   for(iConnNum = 0; iConnNum < m_iNumClients; iConnNum++)
   {
      dReturn += m_pClients[iConnNum]->Recieved();
   }

   return dReturn;
}

void debuglog(bool bOpen)
{
   time_t tTime = 0;
   char szDebug[100];
   struct tm *tmTime = NULL;

   if(debugfile() != NULL)
   {
      debugclose();

      debug("Info(%ld): Closed debug file\n", time(NULL));
   }

   if(bOpen == true)
   {
      tTime = time(NULL);
      tmTime = localtime(&tTime);

      strftime(szDebug, sizeof(szDebug), "ICE.%Y-%m-%d:%H:%M:%S.txt", tmTime);
      debug("Info(%ld): Opening debug file %s\n", time(NULL), szDebug);
      debugopen(szDebug);
   }
}

int main(int argc, char **argv)
{
   STACKTRACE
   int iPort = 0, iUpdate = 2, iArgNum = 1, iStartup = 0, iMemDiff = 0, iLoopDiff = 0, iNumInputs = 0, iClientNum = 0, iDebug = DEBUGLEVEL_INFO;
   int iConnBG = 0, iServerBG = 0, iStorage = 0;
   long lTimeout = 0, lTickDiff = 0, lInputSize = -1, lBufferSize = -1;
   time_t tServerLog = 0, tServerBackground = 0;
   bool bValid = false, bReturn = false, bNoDataValid = false, bProcess = true, bTimeout = true, bLogRotate = false; //, bLoop = true;
   double dTick = 0, dLoop = 0, dRead = 0;
   char *szCertificate = NULL, szInput[100];
   char *szType = NULL, *szMessage = NULL;
   EDF *pConfig = NULL, *pIn = NULL, *pOut = NULL;
   EDFConn *pAccept = NULL, **pTemp = NULL;
   unsigned int iMemUsage = 0, iLoopUsage = 0, iStartUsage = 0, iEntryUsage = 0;

#ifdef LEAKTRACEON
   leakSetStack1();
#endif

   iEntryUsage = memusage();

   debug("Info: main entry\n");
   // printf("Info(%ld): memusage(entry) %u\n", time(NULL), iEntryUsage);
   memstats("entry");

#ifdef UNIX
   // Divert signals to the Panic function
   for(int i = SIGHUP; i < SIGUNUSED; i++)
   {
      if(i != SIGKILL && i != SIGSTOP)
      {
         signal(i, Panic);
      }
   }
#else
   signal(SIGINT, Panic);
   signal(SIGILL, Panic);
   signal(SIGSEGV, Panic);
#endif

   srand(time(NULL));

   // Check for config file
   if(argc < 2 || (argc <= 2 && (strcmp(argv[1], "-version") == 0 || strcmp(argv[1], "--version") == 0 ||
      strcmp(argv[1], "-help") == 0 || strcmp(argv[1], "--help") == 0)))
   {
      debug("Usage: ICE [<config file> [options]]\n");
      debug("       ICE -config <config file> [options]\n");
      debug("           -port <number>\n");
      debug("           -recover\n");

      return 0;
   }
   else if(argc == 2)
   {
      m_szConfigFile = argv[1];
      iArgNum = 2;
   }
   else if(argc >= 3 && stricmp(argv[1], "-config") == 0)
   {
      m_szConfigFile = argv[2];
   }

   if(m_szConfigFile == NULL)
   {
      debug("Error: No config file specified\n");
      return 1;
   }

   // debuglevel(DEBUGLEVEL_DEBUG);

   // Load config file
   debug("Info: Using %s as config file\n", m_szConfigFile);
   pConfig = EDFParser::FromFile(m_szConfigFile);
   if(pConfig == NULL)
   {
      debug("Error: unable to read %s, %s\n", m_szConfigFile, strerror(errno));
      return 1;
   }

   pConfig->GetChild("debuglevel", &iDebug);
   debug("Info: Got debug level %d from the config file\n", iDebug);
   pConfig->GetChild("port", &iPort);
#ifdef CONNSECURE
   pConfig->GetChild("certificate", &szCertificate);
#endif
   pConfig->GetChild("timeout", &lTimeout);
   pConfig->GetChild("update", &iUpdate);
   bNoDataValid = pConfig->GetChildBool("nodatavalid");
   pConfig->GetChild("inputsize", &lInputSize);
   pConfig->GetChild("buffersize", &lBufferSize);

   while(iArgNum < argc)
   {
      debug("Info: Arg %d, %s\n", iArgNum, argv[iArgNum]);
      if(iArgNum < argc - 1 && stricmp(argv[iArgNum], "-port") == 0)
      {
         iPort = atoi(argv[++iArgNum]);
         // printf(". port %d", iPort);
      }
      else if(strcmp(argv[iArgNum], "-recover") == 0)
      {
         debug("Info: enabling recovery\n");
         iStartup |= ICELIB_RECOVER;
      }
      else if(strcmp(argv[iArgNum], "-logrotate") == 0)
      {
         debug("Info: enabling log rotation\n");
         bLogRotate = true;
      }
      else if(iArgNum < argc - 1 && stricmp(argv[iArgNum], "-config") == 0)
      {
         iArgNum++;
      }
      else if(iArgNum < argc - 1 && stricmp(argv[iArgNum], "-debuglevel") == 0)
      {
         iDebug = atoi(argv[++iArgNum]);
         debug("Info: Overriding debug level to %d from the command line\n", iDebug);
      }
      else
      {
         debug("Unrecognised option %s\n", argv[iArgNum]);
      }
      // printf("\n");
      iArgNum++;
   }

   debug("Info: Setting debug level to %d\n", iDebug);
   debuglevel(iDebug);

   // Load the data file
   pConfig->GetChild("data", &m_szDataFile);
   if(m_szDataFile == NULL)
   {
      debug("Error: datafile not specified\n");
      return 1;
   }

   if(bLogRotate == true)
   {
      debuglog(true);
      tServerLog = time(NULL);
   }

   debug("Info: Reading datafile %s with nodatavalid = %s\n", m_szDataFile, BoolStr(bNoDataValid));
   // EDF::Debug(true);
   iDebug = debuglevel(DEBUGLEVEL_DEBUG);
   m_pData = EDFParser::FromFile(m_szDataFile, NULL, 100000);
   // EDF::Debug(false);
   debuglevel(iDebug);
   if(m_pData == NULL)
   {
      if(bNoDataValid == false)
      {
         debug("Error: Failed to read EDF data\n");
         return 1;
      }
      else
      {
         debug("Warning: Failed to read EDF data, creating new object\n");
         m_pData = new EDF();
      }
   }
   debug("Info: EDF data storage space %ld bytes\n", m_pData->Storage(0));
   // EDFPrint(m_pData);

   // EDFBytes("Info: data file", m_pData);

   // Load library
   if(LibFunctions(pConfig) == false)
   {
      return 1;
   }

   m_pData->Root();
   bValid = (*m_pServerLoad)(m_pData, 0, 0);
   if(bValid == false)
   {
      debug("Error: Load failed\n");

      delete m_pData;
      delete pConfig;

      return 1;
   }

   m_pData->Root();
   bValid = (*m_pServerStartup)(m_pData, iStartup);
   if(bValid == false)
   {
      debug("Error: Startup failed\n");

      delete m_pData;
      delete pConfig;

      return 1;
   }

   // EDFPrint("Info: post startup", m_pData);

   iStartUsage = memusage();
   // printf("Info(%ld): memusage(start) %u (entry %u)\n", time(NULL), iStartUsage, iEntryUsage);
   memstats("start", "entry", iEntryUsage);

   // Create connection
   debug("Info: Starting server on port %d (timeout %gs)\n", iPort, lTimeout / 1000.0);
   m_pServer = new EDFConn();
   debug("Info: server %p\n", m_pServer);

   m_pServer->Timeout(lTimeout);
   if(m_pServer->Bind(iPort, szCertificate) == false)
   {
      debug("Error: Failed to start server, %s\n", m_pServer->Error());
#ifdef UNIX
      dlclose(m_pLibrary);
#else
      FreeLibrary(m_pLibrary);
#endif
      return 1;
   }

   // EDFPrint("Info: data", m_pData);

   tServerBackground = time(NULL);
   debug("Info: server background %ld ", tServerBackground);
   tServerBackground -= tServerBackground % 5;
   debug("-> %ld\n", tServerBackground);

   debug("Info: Ready for connections...\n");
   fflush(stdout);
   while(mask(iServerBG, ICELIB_QUIT) == false)
   {
      STACKTRACEUPDATE
      iNumInputs = 0;
      bTimeout = true;
      iLoopUsage = memusage();
      dLoop = gettick();

      m_pServer->AcceptReset();

      dTick = gettick();
      // printf("Info(%ld): checking %d clients\n", time(NULL), ConnCount());
      iClientNum = 0;
      while(iClientNum < m_iNumClients)
      {
         // printf("Info: checking client %d, c=%p d=%p\n", iClientNum, m_pClients[iClientNum], m_pClients[iClientNum]->Data());

         // printf("Info: check loop client %d of %d (%p), %d, %p\n", iClientNum, m_iNumClients, m_pClients[iClientNum], m_pClients[iClientNum]->State(), m_pClients[iClientNum]->Data());
         if(m_pClients[iClientNum]->State() == Conn::OPEN || m_pClients[iClientNum]->State() == Conn::CLOSING)
         {
            // printf("Info: accepting %p\n", m_pClients[iClientNum]);

            if(m_pConnTimeout != NULL)
            {
               if((*m_pConnTimeout)(m_pClients[iClientNum], m_pData) == false)
               {
                  bTimeout = false;
               }
            }

            m_pServer->AcceptCheck(m_pClients[iClientNum]);
            iClientNum++;
         }
         else
         {
            if(m_pClients[iClientNum]->State() == Conn::CLOSED)
            {
               m_pData->Root();
               bReturn = (*m_pConnClose)(m_pClients[iClientNum], m_pData);
            }
            else
            {
               m_pData->Root();
               bReturn = (*m_pConnLost)(m_pClients[iClientNum], m_pData);
            }

            debug("Info: deleting %p\n", m_pClients[iClientNum]);
            delete m_pClients[iClientNum];

            debug("Info: removing %p from array\n", m_pClients[iClientNum]);
            ARRAY_DELETE(EDFConn *, m_pClients, m_iNumClients, iClientNum, pTemp);
         }
      }
      // printf("Info: checks complete in %ld ms\n", tickdiff(dTick));
      fflush(stdout);

      // fflush(stdout);
      dTick = gettick();
      // printf("Info: %ld accept call (timeout %s)\n", time(NULL), BoolStr(bTimeout));
      pAccept = (EDFConn *)m_pServer->Accept(bTimeout);
      // printf("Info: %ld accept return %p, %ld ms\n", time(NULL), pAccept, tickdiff(dTick));
      fflush(stdout);

      if(pAccept != NULL)
      {
         dTick = gettick();
         debug("Info: accept %p\n", pAccept);

         ARRAY_INSERT(EDFConn *, m_pClients, m_iNumClients, pAccept, 0, pTemp);

         m_pData->Root();
         bReturn = (*m_pConnOpen)(pAccept, m_pData);
         if(bReturn == true)
         {
            debug("Info: connection opened\n");

            pAccept->Timeout(0);
            // ARRAY_INSERT(EDFConn *, m_pClients, m_iNumClients, pAccept, 0, pTemp);
         }
         else
         {
            debug("Info: connection open failed, disconnecting\n");
            pAccept->Disconnect();

            debug("Info: deleting %p\n", pAccept);
            delete pAccept;

            debug("Info: removing %p from array\n", pAccept);
            ARRAY_DELETE(EDFConn *, m_pClients, m_iNumClients, 0, pTemp);
         }
         // printf("Info: accept complete in %ld ms\n", tickdiff(dTick));
         fflush(stdout);
      }

      dTick = gettick();
      // printf("Info(%ld): reading %d clients\n", time(NULL), ConnCount());
      for(iClientNum = 0; iClientNum < m_iNumClients; iClientNum++)
      {
         // printf("Info: reading client %d, c=%p d=%p\n", iClientNum, m_pClients[iClientNum], m_pClients[iClientNum]->Data());

         szType = NULL;
         szMessage = NULL;
         // dTick = gettick();
         iMemUsage = memusage();

         dRead = gettick();
         pIn = m_pClients[iClientNum]->Read();
         // printf("Info: client %d read in %ld ms\n", iClientNum, tickdiff(dRead));
         if(pIn != NULL)
         {
            STACKTRACEUPDATE
            // printf("Info: check request client %d (%p), %d %p\n", iClientNum, m_pClients[iClientNum], m_pClients[iClientNum]->State(), m_pClients[iClientNum]->Data());

            iStorage = pIn->Storage(0);
            // if(iStorage > INPUT_SIZE)
            if(lInputSize != -1 && iStorage > lInputSize)
            {
               debug("Warning(%ld): input from %s(%s) is %ld bytes\n", time(NULL), m_pClients[iClientNum]->Hostname(), m_pClients[iClientNum]->AddressStr(), pIn->Storage(0));
            }

            // EDFPrint("pIn", pIn);
            iNumInputs++;

            pOut = new EDF();
            pIn->Root();
            pIn->Get(&szType, &szMessage);
            debug("Info(%ld): input message %s / %s, %d %p\n", time(NULL), szType, szMessage, m_pClients[iClientNum]->State(), m_pClients[iClientNum]->Data());
            m_pIn = pIn;

            if(szType != NULL && szMessage != NULL)
            {
               // Check the message type
               // dTick = gettick();

               if(stricmp(szType, "edf") == 0)
               {
                  bProcess = EDFMessageProcess(pConfig, m_pClients[iClientNum], pIn, pOut, szMessage);
               }
               else if(stricmp(szType, "request") == 0)
               {
                  RequestMessageProcess(pConfig, m_pClients[iClientNum], pIn, pOut, szMessage);
               }
               else
               {
                  // Invalid message
                  pOut->Set("reply", "msg_invalid");
                  pOut->AddChild("type", szType);
                  pOut->AddChild("request", szMessage);
               }

               // lTickDiff = tickdiff(dTick);
               /* if(lTickDiff > MAX_DIFF)
               {
                  debug("Info(%ld): input %s/%s processed in %ld ms\n", time(NULL), szType, szMessage, lTickDiff);
               } */
            }

            STACKTRACEUPDATE

            pOut->Root();
            /* if(pOut->Children(NULL, true) > 100)
            {
               char *szOut1 = NULL, *szOut2 = NULL;
               pOut->Get(&szOut1, &szOut2);
               debug("Info: pOut %s / %s is HUGE!!\n", szOut1, szOut2);
               delete[] szOut1;
               delete[] szOut2;
            }
            else
            {
               debugEDFPrint("Info: pOut", pOut);
            } */
            m_pClients[iClientNum]->Write(pOut);

            m_pIn = NULL;

            delete pOut;
            delete pIn;

            fflush(stdout);
         }
         else
         {
            if(lBufferSize != -1 && m_pClients[iClientNum]->Buffer() > lBufferSize)
            {
               debug("Warning(%ld): buffer from %s(%s) is %ld bytes\n", time(NULL), m_pClients[iClientNum]->Hostname(), m_pClients[iClientNum]->AddressStr(), m_pClients[iClientNum]->Buffer());
            }
         }

         // Perform background processing for this connection
         STACKTRACEUPDATE

         if(m_pClients[iClientNum]->State() == Conn::OPEN)
         {
            // dTick = gettick();
            m_pData->Root();
            iConnBG = (*m_pConnBackground)(m_pClients[iClientNum], m_pData);
            // printf("Info: ConnBackground return %d\n", iConnBG);
            m_pClients[iClientNum]->UpdateTime(time(NULL));

            if(mask(iConnBG, ICELIB_CLOSE) == true)
            {
               debug("Info: disconnecting client %d %p\n", iClientNum, m_pClients[iClientNum]);
               m_pClients[iClientNum]->Disconnect();
            }

            /* lTickDiff = tickdiff(dTick);
            if(lTickDiff > MAX_TICK)
            {
               debug("Info: connection background processed in %ld ms\n", lTickDiff);
            } */

            STACKTRACEUPDATE
         }

         iMemDiff = memusage() - iMemUsage;
         if(abs(iMemDiff) > MIN_DIFF || lTickDiff > MAX_TICK)
         {
            // printf("Info(%ld): memusage(%s/%s) %u / %d -> %u (%ld ms)\n", time(NULL), szType, szMessage, iMemUsage, iMemDiff, memusage(), tickdiff(dTick));
            strcpy(szInput, "");
            if(szType != NULL || szMessage != NULL)
            {
#ifdef UNIX
               snprintf(szInput, sizeof(szInput), "%s / %s, ", szType, szMessage);
#else
               sprintf(szInput, "%s / %s, ", szType, szMessage);
#endif
            }
#ifdef UNIX
            snprintf(szInput, sizeof(szInput), "%s%ld ms", szInput, lTickDiff);
#else
            sprintf(szInput, "%s%ld ms", szInput, lTickDiff);
#endif
            memstats(szInput, "", iMemUsage);
         }

         delete[] szType;
         delete[] szMessage;

         fflush(stdout);
      }
      // printf("Info: clients read in %ld ms\n", tickdiff(dTick));
      fflush(stdout);

      // Perform background processing for the whole server
      STACKTRACEUPDATE
      dTick = gettick();
      // printf("Info(%ld): checking server background\n", time(NULL));
      if(time(NULL) > tServerBackground + 5)
      {
         STACKTRACEUPDATE
         iMemUsage = memusage();
         // dTick = gettick();
         m_pData->Root();
         iServerBG = (*m_pServerBackground)(m_pData);
         // printf("Info: ServerBackground return %d\n", iServerBG);
         tServerBackground = time(NULL);

         if(mask(iServerBG, ICELIB_WRITE) == true)
         {
            WriteData(true, m_szDataFile, m_pServerWrite);
         }
         // lTickDiff = tickdiff(dTick);
         iMemDiff = memusage() - iMemUsage;
         if(abs(iMemDiff) > MIN_DIFF || lTickDiff > MAX_TICK)
         {
            // printf("Info(%ld): memusage(background) %u / %d -> %u (%ld ms)\n", time(NULL), iMemUsage, iMemDiff, memusage(), lTickDiff);
            memstats("background", "", iMemUsage);
         }

         if(bLogRotate == true && time(NULL) > tServerLog + 86400)
         {
            debuglog(true);
            tServerLog = time(NULL);
         }
      }
      // printf("Info: server background checked in %ld ms\n", tickdiff(dTick));
      fflush(stdout);

      iLoopDiff = memusage() - iLoopUsage;
      if(iLoopDiff != 0)
      {
         // printf("Info(%ld): memusage(loop) %u / %d -> %u (%d input%s in %ld ms)\n", time(NULL), iLoopUsage, iLoopDiff, memusage(), iNumInputs, iNumInputs != 1 ? "s" : "", lTickDiff);
         memstats("loop", "", iLoopUsage);
      }

      fflush(stdout);
   }

   debug("Info(%ld): Closing all connections\n", time(NULL));

   // Call close function on each connection
   for(iClientNum = 0; iClientNum < m_iNumClients; iClientNum++)
   {
      m_pData->Root();
      bReturn = (*m_pConnClose)(m_pClients[iClientNum], m_pData);
   }

   debug("Info(%ld): Calling server shutdown\n", time(NULL));

   m_pData->Root();
   (*m_pServerShutdown)(m_pData, 0);

   debug("Info(%ld): Calling server unload\n", time(NULL));

   m_pData->Root();
   (*m_pServerUnload)(m_pData, 0);

   debug("Info(%ld): Deleting clients\n", time(NULL));

   // Close each connection cleanly
   for(iClientNum = 0; iClientNum < m_iNumClients; iClientNum++)
   {
      m_pClients[iClientNum]->Disconnect();
      delete m_pClients[iClientNum];
   }
   delete[] m_pClients;

   // printf("Info(%ld): memusage(stop) %u (start %u)\n", time(NULL), memusage(), iStartUsage);
   memstats("stop", "start", iStartUsage);

   delete m_pServer;

   debug("Info(%ld): Unloading library\n", time(NULL));

#ifdef UNIX
   dlclose(m_pLibrary);
#else
   FreeLibrary(m_pLibrary);
#endif

   WriteData(false, m_szDataFile, NULL);

   delete m_pData;

   delete pConfig;

   // delete[] m_szConfigFile;
   delete[] szCertificate;
   delete[] m_szDataFile;

   // List unfreed memory
   // memstat();

   if(bLogRotate == true)
   {
      debuglog(false);
   }

   // printf("Info(%ld): memusage(exit) %u (start %u, entry %u)\n", time(NULL), memusage(), iStartUsage, iEntryUsage);
   memstats("exit", "start", iStartUsage);
   memstats("exit", "entry", iEntryUsage);

   debug("Info: main exit %d\n", mask(iServerBG, ICELIB_RESTART) == true ? 4 : 0);
   return (mask(iServerBG, ICELIB_RESTART) == true ? 4 : 0);
}
