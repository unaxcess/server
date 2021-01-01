/*
** Conn: Generic client / server socket connector class with SSL support
** (c) 2001 Michael Wood (mike@compsoc.man.ac.uk)
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided this copyright message remains
*/

#include "stdafx.h"

// Standard headers
#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <ctype.h>
#include <math.h>
#ifdef UNIX
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#include "Conn.h"

#ifdef CONNSECURE
// SSL headers
#include <openssl/err.h>
#include <openssl/rand.h>
#endif

// Secure state
#define SECURE_OFF 0
#define SECURE_FLAG 1
// #define SECURE_SET 2
#define SECURE_INIT 3
#define SECURE_ON 4

#ifdef UNIX
#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
#define closesocket close
#define ioctlsocket ioctl
typedef hostent HOSTENT;
/* #ifdef SOLARIS
#define INADDR_NONE -1
#endif */
#else
#ifndef EWOULDBLOCK
#define EWOULDBLOCK WSAEWOULDBLOCK
#endif
#endif

// Select side
int Conn::m_iWidth = 0;
#ifdef WIN32
// Win32 initialiser
bool Conn::m_bWSAStartup = false;
#endif
#ifdef CONNSECURE
// Class-width context
SSL_CTX *Conn::m_pContext = NULL;
#endif
// Unique connection identifier
long Conn::m_lMaxID = 0;

#ifdef STACKTRACENOFUNCTION
// Fudge for compilers without FUNCTION macro
#define __FUNCTION__ "Conn::?"
#endif

// Error macros
#define DEBUG_ERROR(x) \
debug("%s %s\n", __FUNCTION__, x); \
SetError(x);

#define DEBUG_ERRORSOCK(x) \
SetError(); \
if(m_szError == NULL) \
{ \
   debug("%s %s (unknown error %d)\n", __FUNCTION__, x, GetError()); \
} \
else \
{ \
   debug("%s %s (%s)\n", __FUNCTION__, x, Error()); \
}

#ifdef CONNSECURE
#define DEBUG_ERRORSSL(x, y) \
SetErrorSSL(y); \
if(m_szError == NULL) \
{ \
   debug("%s %s (unknown SSL error %d, %s)\n", __FUNCTION__, x, SSL_get_error(m_pSSL, y), ERR_error_string(SSL_get_error(m_pSSL, y), NULL)); \
} \
else \
{ \
   debug("%s %s (%s)\n", __FUNCTION__, x, Error()); \
}
#endif

// Max size of a single read
#define BUFFER_SIZE 16384

#ifdef CONNSECURE

// ConnVerify: Check the context name for validity
int ConnVerify(int iOK, X509_STORE_CTX *pContext)
{
   STACKTRACE
   char szBuffer[256];
   X509 *pCert = NULL;
   int iError = 0, iDepth = 0;

   debug("ConnVerify entry\n");

   debug("ConnVerify depth = %d\n", iDepth);

   pCert = X509_STORE_CTX_get_current_cert(pContext);
   iError = X509_STORE_CTX_get_error(pContext);
   iDepth = X509_STORE_CTX_get_error_depth(pContext);

   X509_NAME_oneline(X509_get_subject_name(pCert), szBuffer, sizeof(szBuffer));
   debug("%s\n", szBuffer);

   if(iOK == 0)
   {
      debug("ConnVerify error %d, %s\n", iError, X509_verify_cert_error_string(iError));
      if(iDepth <= 0)
      {
         iOK = 1;
      }
      else
      {
         iOK = 0;
      }
   }

   switch(X509_STORE_CTX_get_error(pContext))
   {
      case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
         X509_NAME_oneline(X509_get_issuer_name(pCert), szBuffer, sizeof(szBuffer));
         debug("ConnVerify cannot get issuer = %s\n", szBuffer);
         break;

      case X509_V_ERR_CERT_NOT_YET_VALID:
      case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
         debug("ConnVerify not before\n");
         break;

      case X509_V_ERR_CERT_HAS_EXPIRED:
      case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
         debug("ConnVerify not after\n");
         break;
   }

   debug("ConnVerify exit %d\n", iOK);
   return iOK;
}

// ConnCertifyData: Raw data output
void ConnCertifyData(const char *szTitle, unsigned char *szData, int iDataLen)
{
   int iDataPos = 0;
   char cChar1 = '\0', cChar2 = '\0';

   debug("ConnCertifyData %s(%d): ", szTitle, iDataLen);
   for(iDataPos = 0; iDataPos < iDataLen; iDataPos++)
   {
      cChar1 = szData[iDataPos] % 16;
      if(cChar1 >= 10)
      {
         cChar1 = 'A' + cChar1 - 10;
      }
      else
      {
         cChar1 = '0' + cChar1;
      }
      cChar2 = (szData[iDataPos] >> 4) % 16;
      if(cChar2 >= 10)
      {
         cChar2 = 'A' + cChar2 - 10;
      }
      else
      {
         cChar2 = '0' + cChar2;
      }

      // debug("[%d, %d %d]", szData[iDataPos], cChar1, cChar2);
      debug("%c%c", cChar1, cChar2);
   }
   debug("\n");
}

// ConnCertify: Display certification keys, names and stuff
void ConnCertify(SSL *pSSL)
{
   int iStackNum = 0, iChain = 0, iCharPos = 0;
   char *pShare = NULL;
   char szBuffer[BUFFER_SIZE];
   STACK_OF(X509) *pStack = NULL;
   STACK_OF(X509_NAME) *pStackName = NULL;
   const SSL_CIPHER *pCipher = NULL;
   SSL_SESSION *pSession = NULL;
   X509 *pPeer = NULL;
   X509_NAME *pName = NULL;
   EVP_PKEY *pKey = NULL;

   debug("ConnCertify entry\n");

   pStack = SSL_get_peer_cert_chain(pSSL);
   if(pStack != NULL)
   {
      iChain = 1;

      debug("ConnCertify certificate chain:\n");
      for(iStackNum = 0; iStackNum < sk_X509_num(pStack); iStackNum++)
      {
         X509_NAME_oneline(X509_get_subject_name(sk_X509_value(pStack,iStackNum)), szBuffer, BUFFER_SIZE);
         debug("  s = %s\n", szBuffer);

         X509_NAME_oneline(X509_get_issuer_name(sk_X509_value(pStack,iStackNum)), szBuffer, BUFFER_SIZE);
         debug("  i = %s\n", szBuffer);

         // debug("  v = %s\n", sk_X509_value(pStack, iStackNum));
      }

      pPeer = SSL_get_peer_certificate(pSSL);
      if(pPeer != NULL)
      {
         debug("ConnCertify peer certificate:\n");

         // debug("   peer %s\n", pPeer);
/* doesn't build on 1.1
         debug("  c = ");
         //for(iCharPos = 0; iCharPos < pPeer->cert_info->key->public_key->length; iCharPos++)
         for(iCharPos = 0; iCharPos < EVP_PKEY_size(X509_get_pubkey(pPeer)); iCharPos++)
         {
            debug("[%d]", pPeer->cert_info->key->public_key->data[iCharPos]);
         }
         debug("\n");
*/

         X509_NAME_oneline(X509_get_subject_name(pPeer), szBuffer, BUFFER_SIZE);
         debug("  s = %s\n", szBuffer);

         X509_NAME_oneline(X509_get_issuer_name(pPeer), szBuffer, BUFFER_SIZE);
         debug("  i = %s\n", szBuffer);

         pKey = X509_get_pubkey(pPeer);
         debug("ConnCertify peer public key %d bits\n", EVP_PKEY_bits(pKey));
         EVP_PKEY_free(pKey);

         X509_free(pPeer);
      }
      else
      {
         debug("ConnCertify no peer certificate available\n");
      }

      pStackName = SSL_get_client_CA_list(pSSL);
      if(pStackName != NULL && sk_X509_NAME_num(pStackName) > 0)
      {
         debug("ConnCertify client certificate CA names:\n");
         for(iStackNum = 0; iStackNum < sk_X509_NAME_num(pStackName); iStackNum++)
         {
            pName = sk_X509_NAME_value(pStackName,iStackNum);
            X509_NAME_oneline(pName,szBuffer,BUFFER_SIZE);
            debug("  %s\n", szBuffer);
         }
      }
      else
      {
         debug("ClienCertify no client certificate CA names sent\n");
      }

      pShare = SSL_get_shared_ciphers(pSSL, szBuffer, BUFFER_SIZE);
      if(pShare != NULL)
      {
         debug("ConnCertify common cyphers:\n%s\n", pShare);
      }

      debug("ConnCertify handshake (%ld / %ld bytes)\n", BIO_number_read(SSL_get_rbio(pSSL)), BIO_number_written(SSL_get_wbio(pSSL)));
   }
   else
   {
      debug("ConnCertify no certificate chain\n");
   }
   // debug("ConnCertify hit %s\n", pSSL->hit ? "Reused" : "New");

   pCipher = SSL_get_current_cipher(pSSL);
   // if(pCipher != NULL)
   {
      debug("ConnCertify cypher %s, name = %s, version = %s\n", SSL_session_reused(pSSL) ? "Reused" : "New", SSL_CIPHER_get_name(pCipher), SSL_CIPHER_get_version(pCipher));
   }

   pSession = SSL_get_session(pSSL);
/*   
   if(pSession != NULL)
   {
      const unsigned char *certifydata;
      unsigned int certifydatalen;

      certifydata = SSL_SESSION_get_id(pSession, &certifydatalen);
      ConnCertifyData("Session ID", certifydata, certifydatalen);

      certifydata = SSL_SESSION_get0_id_context(pSession, &certifydatalen);
      ConnCertifyData("SID context", certifydata, certifydatalen);

//      certifydata = SSL_SESSION_get_master_key(pSession, &certifydatalen)
//      ConnCertifyData("Master key", pSession->master_key, pSession->master_key_length);
   }
*/
   debug("ConnCertify exit\n");
}
#endif

// Constructor
Conn::Conn()
{
   STACKTRACE

   m_iType = 0;

   m_lMaxID++;
   m_lID = m_lMaxID;
   m_lAddress = 0;
   m_szAddress = NULL;
   m_szHostname = NULL;
   m_iPort = 0;
   m_iSocket = -1;

   m_lTimeout = 1000;
   m_bTimeout = true;

   m_pWrite = NULL;
   m_lWriteLen = 0;
   m_lWritePos = 0;

   m_pData = NULL;

   FD_ZERO(&m_fReads);
   FD_ZERO(&m_fWrites);

#ifdef CONNSECURE
   m_pSSL = NULL;
   m_iSecure = SECURE_OFF;
   m_lSecureWrite = 0;
   m_bRead = true;
   m_bWrite = true;
#endif

   m_szError = NULL;

   m_lConnectTime = 0;
   m_lUpdateTime = 0;
   m_dSent = 0;
   m_dRecieved = 0;

   m_iState = CLOSED;

   m_pRead = NULL;
   m_lReadLen = 0;
}

// Destructor
Conn::~Conn()
{
   STACKTRACE
   debug("Conn::~Conn entry\n");

   Close(CLOSED);

   delete[] m_szAddress;
   m_szAddress = NULL;

   delete[] m_szHostname;
   m_szHostname = NULL;

   delete[] m_pRead;
   m_pRead = NULL;
   m_lReadLen = 0;

   delete[] m_szError;

   debug("Conn::~Conn exit\n");
}

// Connect: Connect to a remote server / port
bool Conn::Connect(const char *szServer, int iPort, bool bSecure, const char *szCertFile)
{
   STACKTRACE
   double dTick = 0;
   unsigned long lAddress = 0;
   struct sockaddr_in *pAddr = NULL;

   // Flag as client connection
   m_iType = CLIENT;

   pAddr = Init(0, szCertFile);
   if(pAddr == NULL)
   {
      DEBUG_ERROR("Init failed")
      return false;
   }

   // debug("Conn::Connect mem0: %ld bytes\n", memusage());

   // Set connection server / port
   if(StringToAddress(szServer, &lAddress) == false)
   {
      return false;
   }
   // debug("Conn::Connect name %s -> IP %lu\n", szServer, lAddress);
   m_lAddress = lAddress;
   m_szAddress = AddressToString(lAddress);
   m_szHostname = strmk(szServer);
   m_iPort = iPort;

   pAddr->sin_addr.s_addr = htonl(m_lAddress);
   pAddr->sin_port = htons((short)m_iPort);

   dTick = gettick();

   // debug("Conn::Connect mem1: %ld bytes\n", memusage());

   // Connect to the server
   if(connect(m_iSocket, (struct sockaddr *)pAddr, sizeof(struct sockaddr_in)) == SOCKET_ERROR)
   {
      DEBUG_ERRORSOCK("connect failed")

      delete pAddr;

      return false;
   }

   // debug("Conn::Connect mem2: %ld bytes\n", memusage());

   debug("Conn::Connect established (%ld ms)\n", tickdiff(dTick));

   m_iState = OPEN;

   // Go secure now?
   if(bSecure == true && SetSecure() == false)
   {
      debug("Conn::Connect SetSecure failed\n");
      Close(CLOSED);

      delete pAddr;

      return false;
   }

   // debug("Conn::Connect mem3: %ld bytes\n", memusage());

   delete pAddr;

   m_lConnectTime = time(NULL);
   m_lUpdateTime = m_lConnectTime;

   return true;
}

// Bind: Bind to a local port
bool Conn::Bind(int iPort, const char *szCertFile)
{
   STACKTRACE
   // int iSockOpt = 1;
   SOCKADDR_IN *pAddr = NULL;

   // Flag as bound connection
   m_iType = BIND;

   pAddr = Init(iPort, szCertFile);
   if(pAddr == NULL)
   {
      debug("Conn::Bind Init failed\n");

      return false;
   }

   // Start listening on the port
   listen(m_iSocket, SOMAXCONN);

   m_iState = OPEN;

   delete pAddr;

   return true;
}

// Accept: Accept a new connection from a local port
Conn *Conn::Accept(bool bTimeout)
{
   STACKTRACE
   int iReady = 0;
   Conn *pAccept = NULL;

   if(m_iType != BIND)
   {
      // Only bound connections can accept
      debug("Conn::Accept incorrect socket type\n");
      return NULL;
   }

   if(bTimeout == false)
   {
      UseTimeout(false);
   }

   // debug("Conn::Accept calling Select\n");
   iReady = Select(true, true, false);

   if(iReady < 0)
   {
      DEBUG_ERROR("Select failed");

      return NULL;
   }
   else if(iReady == 0)
   {
      // Nothing to do
      return NULL;
   }

   if(FD_ISSET(m_iSocket, &m_fReads))
   {
      // Accept a new connection
      pAccept = Create();

      if(pAccept->SetSocket(m_iSocket) == false)
      {
         debug("Conn::Accept failed\n");

         delete pAccept;

         return NULL;
      }
   }

   return pAccept;
}

// Disconnect: Close connection
bool Conn::Disconnect()
{
   STACKTRACE

   if(m_iState != OPEN)
   {
      debug("Conn::Disconnect socket not open\n");

      return false;
   }

   debug("Conn::Disconnect write len %d\n", m_lWriteLen);

   if(m_lWriteLen > 0)
   {
      // Still data to be written
      m_iState = CLOSING;
   }
   else
   {
      m_iState = CLOSED;
   }

   Close(m_iState);

   debug("Conn::Disconnect %d\n", m_iState);

   return true;
}

// AcceptReset: Additional socket checking for Accept method
bool Conn::AcceptReset()
{
   if(m_iType != BIND)
   {
      // Only bind connections can accept
      debug("Conn::AcceptReset not a bind type\n");
      return false;
   }

   FD_ZERO(&m_fReads);
   FD_ZERO(&m_fWrites);

   m_bTimeout = true;
   m_iWidth = 0;

   return true;
}

// AcceptCheck: Additional socket checking for Accept method
bool Conn::AcceptCheck(Conn *pSocket)
{
   if(m_iType != BIND)
   {
      // Only bind connections can accept
      debug("Conn::AcceptCheck not a bind type\n");
      return false;
   }

   if(pSocket->UseTimeout() == false)// || pSocket->WriteBuffer() == true || pSocket->State() == CLOSING)
   {
      // debug("Conn::AcceptCheck no timeout\n");
      m_bTimeout = false;
   }

   FD_SET(pSocket->Socket(), &m_fReads);

#ifdef UNIX
   if(pSocket->Socket() > m_iWidth)
   {
      m_iWidth = pSocket->Socket();
   }
#endif

   return true;
}

long Conn::ID()
{
   return m_lID;
}

bool Conn::Connected()
{
   return m_iState == OPEN;
}

Conn::ConnState Conn::State()
{
   STACKTRACE

   if(m_iState != OPEN)
   {
      debug(DEBUGLEVEL_INFO, "Conn::State %d\n", m_iState);
   }

   return m_iState;
}

long Conn::Timeout()
{
   return m_lTimeout;
}

long Conn::Timeout(long lTimeout)
{
   long lReturn = m_lTimeout;

   debug("Conn::Timeout %ld (%ld)\n", lTimeout, m_lTimeout);
   m_lTimeout = lTimeout;

   return m_lTimeout;
}

// SetSecure: Enable secure connection
bool Conn::SetSecure()
{
   STACKTRACE

   debug("Conn::SetSecure entry\n");

   if(m_iState != OPEN)
   {
      DEBUG_ERROR("socket not open")

      debug("Conn::SetSecure exit false\n");
      return false;
   }

   if(m_iType != CLIENT && m_iType != SERVER)
   {
      // Other types (currently only bind) cannot be secured
      DEBUG_ERROR("socket not client or server")

      debug("Conn::SetSecure exit false\n");
      return false;
   }

#ifdef CONNSECURE
   if(m_iSecure > 0)
   {
      debug("Conn::SetSecure exit true, already in secure mode\n");
      return true;
   }

   if(SecureInit() == false)
   {
      DEBUG_ERROR("Cannot enable secure mode")

      debug("Conn::SetSecure exit false");
      return false;
   }

   debug("Conn::SetSecure exit true\n");
   return true;
#else
   debug("Conn::SetSecure exit false, no secure support\n");
   return false;
#endif
}

bool Conn::GetSecure()
{
#ifdef CONNSECURE
   return m_iSecure == SECURE_ON;
#else
   return false;
#endif
}

bool Conn::SetCompress(int iType)
{
   STACKTRACE

   debug("Conn::SetCompres entry %d\n", iType);

   if(m_iState != OPEN)
   {
      DEBUG_ERROR("socket not open")

      debug("Conn::SetCompres exit false\n");
      return false;
   }

   if(m_iType != CLIENT && m_iType != SERVER)
   {
      // Other types (currently only bind) cannot be secured
      DEBUG_ERROR("socket not client or server")

      debug("Conn::SetCompres exit false\n");
      return false;
   }

#ifdef CONNCOMPRESS
   if(m_iCompress > 0)
   {
      if(m_iCompress != iType)
      {
         DEBUG_ERROR("Compress modes do not match")

         debug("Conn::SetCompress exit false\n");
         return false;
      }

      debug("Conn::SetCompress exit true, already in compress mode\n");
      return true;
   }

   debug("Conn::SetCompress exit true\n");
   return true;
#else
   debug("Conn::SetCompress exit false, no compress support\n");
   return false;
#endif
}

int Conn::GetCompress()
{
#ifdef CONNCOMPRESS
   return m_iCompress;
#else
   return 0;
#endif
}

char *Conn::Error()
{
   STACKTRACE
   return m_szError;
}

char *Conn::Hostname()
{
   return m_szHostname;
}

unsigned long Conn::Address()
{
   return m_lAddress;
}

char *Conn::AddressStr()
{
   return m_szAddress;
}

int Conn::Port()
{
   return m_iPort;
}

unsigned long Conn::ConnectTime()
{
   return m_lConnectTime;
}

unsigned long Conn::UpdateTime()
{
   return m_lUpdateTime;
}

bool Conn::UpdateTime(unsigned long lTime)
{
   m_lUpdateTime = lTime;
   return true;
}

double Conn::Sent()
{
   return m_dSent;
}

double Conn::Recieved()
{
   return m_dRecieved;
}

void *Conn::Data()
{
   return m_pData;
}

bool Conn::Data(void *pData)
{
   m_pData = pData;
   return true;
}

void Conn::GetHostInfo(char **szHostname, char **szAddress)
{
   char szString[100];
   struct in_addr pAddr;
   HOSTENT *pHost = NULL;

   if(gethostname(szString, sizeof(szString)) != SOCKET_ERROR)
   {
      debug("Conn::GetHostInfo %s\n", szString);

      pHost = gethostbyname(szString);
      if(pHost != NULL)
      {
         (*szAddress) = new char[100];

         memcpy(&pAddr, pHost->h_addr_list[0], sizeof(struct in_addr));
         strcpy(*szAddress, inet_ntoa(pAddr));

         debug("Conn::GetHostInfo address %s\n", (*szAddress));
      }
      else
      {
         debug("Conn::GetHostInfo no host entry\n");
      }
   }
   else
   {
      debug("Conn::GetHostInfo no hostname\n");
   }
}

// StringToAddress: Translate a string to an address via optional hostname lookup
bool Conn::StringToAddress(const char *szAddress, unsigned long *lAddress, bool bLookup)
{
   STACKTRACE
   struct hostent *pHost = NULL;

   if(szAddress == NULL || stricmp(szAddress, "") == 0)
   {
      return false;
   }

   // Check server string against a.b.c.d format
   debug("Conn::StringToAddress entry %s %p %s\n", szAddress, lAddress, BoolStr(bLookup));
   *lAddress = inet_addr(szAddress);
   if((*lAddress) == INADDR_NONE)
   {
      if(bLookup == false)
      {
         return false;
      }

      // No match, use lookup to get server IP address
      if((pHost = gethostbyname(szAddress)) == NULL)
      {
         debug("Conn::StringToAddress gethostbyname failed\n");

         return false;
      }
      else
      {
         debug("Conn::StringToAddress from hostent data\n");
         memcpy(lAddress, pHost->h_addr, sizeof(lAddress));
      }
   }
   else
   {
      debug("Conn::StringToAddress from inet_addr\n");
   }
   (*lAddress) = ntohl(*lAddress);

   debug("Conn::StringToAddress exit true, %lu\n", *lAddress);
   return true;
}

// CIDRToRange: Translate a IP range in CIDR format into min / max range
bool Conn::CIDRToRange(const char *szAddress, unsigned long *pMin, unsigned long *pMax)
{
   STACKTRACE
   int iAddrPos = 0, iSetNum = 0, iRange = 0;
   unsigned long iSet[4], lMin = 0, lMax = 0;

   iSet[0] = 0;
   iSet[1] = 0;
   iSet[2] = 0;
   iSet[3] = 0;
   iAddrPos = 0;
   iSetNum = 0;
   iRange = 0;

   // Split the address field into 4 numbers and optional range (for CIDR values)
   while(szAddress[iAddrPos] != '\0')
   {
      if(szAddress[iAddrPos] == '.')
      {
         if(iSetNum < 3)
         {
            // Start IP value
            iSetNum++;
         }
         else
         {
            // Not a valid CIDR string
            iSetNum = 5;
         }
      }
      else if(szAddress[iAddrPos] == '/')
      {
         // Start range value
         iSetNum = 4;
      }
      else if(iSetNum < 4 && isdigit(szAddress[iAddrPos]))
      {
         // IP value
         iSet[iSetNum] = 10 * iSet[iSetNum] + (szAddress[iAddrPos] - '0');
      }
      else if(iSetNum == 4 && isdigit(szAddress[iAddrPos]))
      {
         // Range value
         iRange = 10 * iRange + (szAddress[iAddrPos] - '0');
      }
      else
      {
         // Invalid character
         debug("ConnectionMatch bad char '%c' (%d of %s)\n", szAddress[iAddrPos], iAddrPos, szAddress);

         return false;
      }

      iAddrPos++;
   }

   // Bit shift to set minimum
   lMin = iSet[0] << 24;
   lMin += iSet[1] << 16;
   lMin += iSet[2] << 8;
   lMin += iSet[3];
   // lMin = htonl(lMin);
   if(iRange > 0)
   {
      // Maximum is 2 ^ range above
      lMax = lMin + (long)ldexp((double)2, 32 - iRange);
   }
   else
   {
      lMax = lMin;
   }

   *pMin = lMin;
   *pMax = lMax;

   // printf("ConnectionMatch address %s, %d.%d.%d.%d/%d, %lu -> %lu\n", szValue, iSet[0], iSet[1], iSet[2], iSet[3], iRange, lMin, lMax);

   return true;
}

// AddressToString: Convert IP number into octets
char *Conn::AddressToString(unsigned long lAddress)
{
   int iIPs[4];
   char *szAddress = NULL;

   // debug("Conn::AddressToString entry %lu\n", lAddress);

   szAddress = new char[20];

   iIPs[0] = (lAddress >> 24) & 255;
   iIPs[1] = (lAddress >> 16) & 255;
   iIPs[2] = (lAddress >> 8) & 255;
   iIPs[3] = lAddress & 255;

   sprintf(szAddress, "%d.%d.%d.%d", iIPs[0], iIPs[1], iIPs[2], iIPs[3]);

   // debug("Conn::AddressToString exit %s\n", szAddress);
   return szAddress;
}

// ValidHostname: Check string represents a valid hostname
bool Conn::ValidHostname(const char *szHostname)
{
   int iPos = 1;

   if(szHostname == NULL || strlen(szHostname) == 0)
   {
      return false;
   }

   if(!isupper(szHostname[0]) && !islower(szHostname[0]) && !isdigit(szHostname[0]))
   {
      return false;
   }

   while(szHostname[iPos] != '\0')
   {
      if(!isupper(szHostname[iPos]) && !islower(szHostname[iPos]) && !isdigit(szHostname[iPos]) &&
         szHostname[iPos] != '.' && szHostname[iPos] != '-')
      {
         return false;
      }

      iPos++;
   }

   return true;
}

// Accept a new socket from a bound socket
bool Conn::SetSocket(SOCKET iSocket)
{
   STACKTRACE
#ifndef CYGWIN
   char szHostname[NI_MAXHOST];
#endif
   int iHostname = 0;
#ifdef UNIX
   socklen_t iClientLen = 0;
#else
   int iClientLen = 0;
#endif
   double dTick = 0;
   struct sockaddr_in pSockClient;

   // Accept a new connection
   iClientLen = sizeof(pSockClient);

   dTick = gettick();
   m_iSocket = accept(iSocket, (struct sockaddr *)&pSockClient, &iClientLen);
   debug("Conn::SetSocket accept call %ld ms\n", tickdiff(dTick));

   if(m_iSocket == SOCKET_ERROR)
   {
      DEBUG_ERRORSOCK("accept failed")
      return false;
   }

   if(SetSock() == false)
   {
      debug("Conn::SetSocket SetSock failed\n");
      return false;
   }

   m_iType = SERVER;

   m_iState = OPEN;

   debug("Conn::SetSocket client len %d\n", iClientLen);

   // Get the host information
   m_lAddress = ntohl(pSockClient.sin_addr.s_addr);
   m_szAddress = AddressToString(m_lAddress);

#ifndef CYGWIN
   // Get hostname / address from the accept
   szHostname[0] = '\0';
   dTick = gettick();
   if(getnameinfo((sockaddr *)&pSockClient, iClientLen, szHostname, sizeof(szHostname), NULL, 0, NI_NAMEREQD) == 0)
   {
      debug("Conn::SetSocket getnameinfo %d, %s %d\n", iHostname, szHostname, GetError());
      if(strlen(szHostname) > 0)
      {
         m_szHostname = strmk(szHostname);
      }
   }
   else
   {
      DEBUG_ERRORSOCK("getnameinfo failed")
   }
   debug("Conn::SetSocket getnameinfo call %ld ms\n", tickdiff(dTick));
#endif

   debug("Conn::SetSocket host info %lu %s / %s\n", m_lAddress, m_szAddress, m_szHostname);

   m_lConnectTime = time(NULL);
   m_lUpdateTime = m_lConnectTime;

   return true;
}

// Close a socket
void Conn::Close(ConnState iState)
{
   STACKTRACE

   debug("Conn::Close %d, %d\n", iState, m_iState);

   if(iState != CLOSING)
   {
      // debug("Conn::Close socket %d\n", m_iSocket);

      if(m_iSocket != -1)
      {
         // Close the socket right now

         debug("Conn::Close closing socket\n");
#ifdef CONNSECURE
         if(m_pSSL != NULL)
         {
            debug("Conn::Close shutting down SSL\n");
            // SSL_set_shutdown(m_pSSL, SSL_SENT_SHUTDOWN | SSL_RECEIVED_SHUTDOWN);
            SSL_shutdown(m_pSSL);

            SSL_free(m_pSSL);

            m_pSSL = NULL;
         }
#endif

         debug("Conn::Close tidy up members\n");

         // debug("Conn::Close closing socket\n");
         closesocket(m_iSocket);
         m_iSocket = -1;
         m_iState = iState;

         // debug("Conn::Close delete read buffer\n");
         // delete[] m_pRead;
         // m_pRead = NULL;
         // m_lReadLen = 0;

         // debug("Conn::Close delete write buffer\n");
         delete[] m_pWrite;
         m_pWrite = NULL;
         m_lWriteLen = 0;

         m_iSocket = -1;
      }
   }
   else
   {
      // Flag as closing (subsequent reads will fail)
      m_iState = CLOSING;
   }

   debug("Conn::Close exit\n");
}

SOCKET Conn::Socket()
{
   return m_iSocket;
}

bool Conn::UseTimeout(bool bTimeout)
{
   m_bTimeout = bTimeout;
   // debug("Conn::UseTimeout %s\n", BoolStr(m_bTimeout));

   return true;;
}

bool Conn::UseTimeout()
{
   int iWriteLen = 0;//, iPending = 0;

   if(m_iType == BIND)
   {
      return m_bTimeout;
   }

#ifdef CONNSECURE
   if(m_iSecure == SECURE_FLAG)
   {
      iWriteLen = m_lSecureWrite - m_lWritePos;
      // debug("Conn::UseTimeout secure write %ld/%ld\n", m_lWritePos, m_lSecureWrite);
      /* if(iWriteLen > 0)
      {
         memprint(debugfile(), "Conn::UseTimeout write", m_pWrite + m_lWritePos, iWriteLen);
      } */
   }
   else if(m_iSecure == 0 || m_iSecure == SECURE_ON)
#endif
   {
      iWriteLen = m_lWriteLen - m_lWritePos;
      // debug("Conn::UseTimeout write len %ld/%ld\n", m_lWritePos, m_lWriteLen);
      /* if(iWriteLen > 0)
      {
         memprint(debugfile(), "Conn::UseTimeout write", m_pWrite + m_lWritePos, iWriteLen);
      }*/
   }

#ifdef CONNSECURE
   if(m_iSecure > 0)
   {
      if(m_bWrite == true)
      {
         // debug("Conn::UseTimeout false, want %s\n", BoolStr(m_bWrite));
         return false;
      }
   }
#endif

   if(iWriteLen > 0 || m_iState == CLOSING)
   {
#ifdef CONNSECURE
      /* if(m_iSecure > 0)
      {
         debug(", false\n");
      } */
#endif
      // debug("Conn::UseTimeout false, write len / closing\n");
      return false;
   }

#ifdef CONNSECURE
   /* if(m_iSecure > 0)
   {
      debug(", %s\n", BoolStr(m_bTimeout));
   } */
#endif
   return m_bTimeout;
}

int Conn::Type()
{
   return m_iType;
}

#define SELECT_RETURN \
if(iReady < 0) \
{ \
   DEBUG_ERROR("Select failed") \
   Close(LOST); \
   /*debug("Conn::Read exit false\n");*/ \
   return false; \
}

#define CLOSE_RETURN(x) \
debug("Conn::Read close %d\n", x); \
Close(x); \
/*debug("Conn::Read exit false, %d\n", x);*/ \
return false;

// Read data from a socket
bool Conn::Read()
{
   STACKTRACE
   int iReady = 0, iRead = 0, iWritten = 0, iLoopNum = 0, iAccept = 0;
   bool bLoop = false, bTimeout = true;
   long lWriteLen = 0;
   byte *pRead = NULL;
   double dEntry = 0, dLoop = 0;

   if(m_iType != CLIENT && m_iType != SERVER)
   {
      debug("Conn::Read wrong type\n");
      return false;
   }

   if(m_iState != OPEN && m_iState != CLOSING)
   {
      // Socket cannot be read
      debug("Conn::Read socket not open or closing, %d\n", m_iState);
      return false;
   }

   // debug("Conn::Read entry\n");

   dEntry = gettick();

   bTimeout = m_bTimeout;

   do
   {
      bLoop = false;
      iLoopNum++;

      /* if(iLoopNum > 1)
      {
         debug("Conn::Read loop %d, %ld / %ld -> %ld", iLoopNum, m_lReadLen, m_lWritePos, m_lWriteLen);
         if(dLoop > 0)
         {
            debug(" (%ld ms)", tickdiff(dLoop));
         }
         debug("\n");
      } */

      dLoop = gettick();

      // Reset the selectors
      FD_ZERO(&m_fReads);
      FD_ZERO(&m_fWrites);

#ifdef CONNSECURE
      if(m_iSecure >= SECURE_INIT)
      {
         // Secure is on
         if(m_iType == SERVER)
         {
            if(SSL_is_init_finished(m_pSSL) == 0)
            {
               // Still setting up
               iAccept = SSL_accept(m_pSSL);
               if(iAccept <= 0)
               {
                  DEBUG_ERRORSSL("SSL_accept failed", iAccept)
               }
               else
               {
                  // Full secure mode
                  ConnCertify(m_pSSL);

                  m_iSecure = SECURE_ON;
                  debug("Conn::Read secure ON\n");
               }
            }
         }
         else
         {
            // if(m_iSecure < SECURE_ON)
            {
               // debug("Conn::Read SSL_in_init %d, SSL_total_renegotations %d\n", SSL_in_init(m_pSSL), SSL_total_renegotiations(m_pSSL));
               if(SSL_in_init(m_pSSL) != 0 && SSL_total_renegotiations(m_pSSL) == 0)
               {
                  // Setup finished

                  // ConnCertify(m_pSSL);
                  // m_iSecure = SECURE_ON;
                  // debug("Conn::Read secure ON\n");

                  debug("Conn::Read init done\n");
               }
               else
               {
                  if(m_iSecure < SECURE_ON)
                  {
                     // Full secure mode
                     ConnCertify(m_pSSL);

                     m_iSecure = SECURE_ON;
                     debug("Conn::Read secure ON\n");
                  }
               }
            }
         }

         if(SSL_pending(m_pSSL) == 0)
         {
            // Check for socket activity
            iReady = Select(bTimeout, true, m_bWrite);

            SELECT_RETURN
         }

         // debug("Conn::Read pending %d, read %s -> %d, write %s -> %d\n", SSL_pending(m_pSSL), BoolStr(m_bRead), FD_ISSET(m_iSocket, &m_fReads), BoolStr(m_bWrite), FD_ISSET(m_iSocket, &m_fWrites));

         if(SSL_pending(m_pSSL) == 0 && FD_ISSET(m_iSocket, &m_fWrites))
         {
            // Write required
            lWriteLen = m_lWriteLen - m_lWritePos;

            iWritten = SSL_write(m_pSSL, (char *)m_pWrite + m_lWritePos, lWriteLen);
            /* debug("Conn::Read SSL_write %ld / %ld -> %d: ", lWriteLen, m_lWriteLen, iWritten);
            memprint(debugfile(), NULL, m_pWrite + m_lWritePos, lWriteLen, false);
            debug("\n"); */

            if(iWritten <= 0)
            {
               DEBUG_ERRORSSL("SSL_write failed", iWritten)
            }

            switch(SSL_get_error(m_pSSL, iWritten))
            {
               case SSL_ERROR_NONE:
                  if(iWritten <= 0)
                  {
                     CLOSE_RETURN(CLOSED)
                  }
                  else
                  {
                     // Write successful
                     m_lWritePos += iWritten;
                     m_dSent += iWritten;

                     if(m_lWritePos == m_lWriteLen)
                     {
                        // No data left to write
                        delete[] m_pWrite;
                        m_pWrite = NULL;

                        m_lWriteLen = 0;
                        m_lWritePos = 0;

                        if(m_iState == CLOSING)
                        {
                           // Fully closed
                           debug("Conn::Read closing -> closed\n");
                           m_iState = CLOSED;
                        }

                        m_bWrite = false;
                     }
                     else
                     {
                        debug("Conn::Read %ld bytes left to write\n", m_lWriteLen);

                        m_bWrite = true;

                        bLoop = true;
                     }
                  }
                  break;

               case SSL_ERROR_WANT_WRITE:
                  m_bWrite = true;
                  break;

               case SSL_ERROR_WANT_READ:
                  m_bRead = true;
                  m_bWrite = false;
                  break;

               case SSL_ERROR_ZERO_RETURN:
                  if(lWriteLen != 0)
                  {
                     CLOSE_RETURN(LOST)
                  }
                  else
                  {
                     m_bWrite = false;
                  }
                  break;

               case SSL_ERROR_SYSCALL:
                  if(iWritten != 0 || lWriteLen != 0)
                  {
                     CLOSE_RETURN(LOST)
                  }
                  else
                  {
                     m_bWrite = false;
                  }
                  break;

               case SSL_ERROR_SSL:
                  CLOSE_RETURN(LOST)
                  break;
            }
         }

         if(SSL_pending(m_pSSL) > 0 || FD_ISSET(m_iSocket, &m_fReads))
         {
            // Read required

            // dTick = gettick();
            pRead = new byte[m_lReadLen + BUFFER_SIZE + 1];
            memcpy(pRead, m_pRead, m_lReadLen);
            delete[] m_pRead;
            m_pRead = pRead;
            // printf("Conn::Read buffer resize in %ld ms\n", tickdiff(dTick));

            iRead = SSL_read(m_pSSL, (char *)m_pRead + m_lReadLen, BUFFER_SIZE);
            /* debug("Conn::Read SSL_read %d", iRead);
            if(iRead > 0)
            {
               debug(". ");
               memprint(debugfile(), NULL, m_pRead + m_lReadLen, iRead, false);
            }
            debug("\n"); */

            if(iRead <= 0)
            {
               DEBUG_ERRORSSL("SSL_read failed", iRead)

               switch(SSL_get_error(m_pSSL, iRead))
               {
                  case SSL_ERROR_NONE:
                     if(iRead <= 0)
                     {
                        CLOSE_RETURN(CLOSED)
                     }

                     // Read successful

                     m_lReadLen += iRead;
                     m_pRead[m_lReadLen] = '\0';

                     m_dRecieved += iRead;

                     m_bRead = false;
                     debug("Conn::Read want read false\n");
                     break;

                  case SSL_ERROR_ZERO_RETURN:
                     CLOSE_RETURN(CLOSED)
                     break;

                  case SSL_ERROR_WANT_WRITE:
                     m_bWrite = true;
                     break;

                  case SSL_ERROR_WANT_READ:
                     m_bRead = true;
                     m_bWrite = true;
                     break;

                  case SSL_ERROR_SYSCALL:
                  case SSL_ERROR_SSL:
                     CLOSE_RETURN(LOST)
                     break;
               }
            }
            else
            {
               m_lReadLen += iRead;
               m_pRead[m_lReadLen] = '\0';

               m_dRecieved += iRead;

               bLoop = true;
            }
         }
      }
      else
#endif
      {
         // Check for socket activity
         iReady = Select(bTimeout, true, false);
         SELECT_RETURN

         if(m_iState == OPEN && FD_ISSET(m_iSocket, &m_fReads))
         {
            // Read required

            // if(lBufferSize > 0)
            {
               // dTick = gettick();
               pRead = new byte[m_lReadLen + BUFFER_SIZE + 1];
               memcpy(pRead, m_pRead, m_lReadLen);
               delete[] m_pRead;
               m_pRead = pRead;
               // printf("Conn::Read buffer resize in %ld ms\n", tickdiff(dTick));
            }

            iRead = recv(m_iSocket, (char *)m_pRead + m_lReadLen, BUFFER_SIZE, 0);
            if(iRead <= 0)
            {
               debug("Conn::Read recv %d / %d\n", iRead, GetError());
               DEBUG_ERRORSOCK("recv failed")

               // Nothing read and no error - close at client end
               if(iRead == 0 && GetError() == 0)
               {
                  // DEBUG_ERRORSOCK("Connection reset by peer")
                  CLOSE_RETURN(LOST)
               }

               // Interrupt and blocking errors are OK
               if(GetError() != EINTR && (GetError() != EWOULDBLOCK || (iRead == 0 && GetError() == EWOULDBLOCK)))
               {
                  // DEBUG_ERRORSOCK("recv failed")
                  CLOSE_RETURN(LOST)
               }
            }
            else
            {
               // Read successful
               // debug("Conn::Read recv %d\n", iRead);
               memprint(DEBUGLEVEL_DEBUG, debugfile(), "Conn::Read read", m_pRead + m_lReadLen, iRead);

               m_lReadLen += iRead;
               m_pRead[m_lReadLen] = '\0';

               m_dRecieved += iRead;

               bLoop = true;
            }
         }

#ifdef CONNSECURE
         if(m_lSecureWrite > 0)
         {
            // Only write up to the flag
            lWriteLen = m_lSecureWrite - m_lWritePos;
         }
         else
#endif
         {
            lWriteLen = m_lWriteLen - m_lWritePos;
         }

         if(lWriteLen > 0)
         {
            // Write required
            memprint(DEBUGLEVEL_DEBUG, debugfile(), "Conn::Read write", m_pWrite + m_lWritePos, lWriteLen);

            iWritten = send(m_iSocket, (char *)m_pWrite + m_lWritePos, lWriteLen, 0);
            if(iWritten <= 0)
            {
               DEBUG_ERRORSOCK("send failed")

               if(GetError() != EINTR && GetError() != EWOULDBLOCK)
               {
                  CLOSE_RETURN(LOST)
               }
            }
            else
            {
               m_lWritePos += iWritten;
               m_dSent += iWritten;

#ifdef CONNSECURE
               if(m_iSecure == SECURE_FLAG)
               {
                  if(m_lWritePos == m_lSecureWrite)
                  {
                     // All data to flag written
                     debug("Conn::Read written to flag\n");

                     m_iSecure = 0;

                     if(m_iType == SERVER)
                     {
                        // Begin secure setup
                        SecureInit();
                     }
                  }
                  else
                  {
                     debug("Conn::Read %ld bytes left to flag\n", m_lWriteLen);

                     bLoop = true;
                  }
               }
               else
#endif
               {
                  if(m_lWritePos == m_lWriteLen)
                  {
                     delete[] m_pWrite;
                     m_pWrite = NULL;

                     m_lWriteLen = 0;
                     m_lWritePos = 0;

                     if(m_iState == CLOSING)
                     {
                        debug("Conn::Read closing -> closed\n");
                        m_iState = CLOSED;
                     }
                  }
                  else
                  {
                     debug("Conn::Read %ld bytes left to write\n", m_lWriteLen);

                     bLoop = true;
                  }
               }
            }
         }
      }

      if(m_iType == SERVER)
      {
         bTimeout = false;
      }
   }
   while(bLoop == true);

   /* if(iLoopNum > 1)
   {
      debug("Conn::Read loop %ld ms\n", tickdiff(dEntry));
   } */

   // debug("Conn::Read exit true, %ld ms %ld / %ld bytes\n", tickdiff(dEntry), m_lReadLen, m_lWriteLen);
   return true;
}

// ReadBuffer: Return length / contents of the read buffer
long Conn::ReadBuffer(byte **pData)
{
   // debug("Conn::ReadBuffer entry %p, %ld\n", pData, m_lReadLen);

   // memprint(debugfile(), "Conn::ReadBuffer", m_pRead, m_lReadLen);

   if(m_lReadLen == 0)
   {
      return 0;
   }

   if(pData != NULL)
   {
      *pData = m_pRead;
   }

   return m_lReadLen;
}

// Release: Remove data from the read buffer
long Conn::Release(long lLength)
{
   byte *pRead = NULL;

   if(lLength > m_lReadLen || lLength == -1)
   {
      lLength = m_lReadLen;
   }

   if(lLength == m_lReadLen)
   {
      // Remove the whole buffer

      delete[] m_pRead;
      m_pRead = NULL;

      m_lReadLen = 0;
   }
   else
   {
      // Remove first 'n' bytes

      pRead = new byte[m_lReadLen - lLength];
      memcpy(pRead, m_pRead + lLength, m_lReadLen - lLength);

      delete[] m_pRead;
      m_pRead = pRead;

      m_lReadLen -= lLength;
   }

   return m_lReadLen;
}

// Write: Write data (add to the write buffer)
bool Conn::Write(const char *szData)
{
   STACKTRACE
   return Write((const byte *)szData, strlen(szData));
}

// Write: Write data (add to the write buffer)
bool Conn::Write(const byte *pData, long lDataLen)
{
   STACKTRACE
   long lWriteLen = 0;
   byte *pWrite = NULL;

   /* debug("Conn::Write %p %ld, %d\n", pData, lDataLen, m_iState);
   memprint(debugfile(), NULL, pData, lDataLen, true);
   if(pData != NULL)
   {
      debug("\n");
   } */

   if(m_iState != OPEN)
   {
      return false;
   }

   if(pData == NULL || lDataLen == 0)
   {
      return true;
   }

   // memprint("Conn::Write data", pData, lDataLen, false);

   // Append the data onto the write buffer
   lWriteLen = m_lWriteLen - m_lWritePos;
   pWrite = new byte[lWriteLen + lDataLen];
   if(m_pWrite != NULL)
   {
      memcpy(pWrite, m_pWrite + m_lWritePos, lWriteLen);
   }
   memcpy(pWrite + lWriteLen, pData, lDataLen);
   delete[] m_pWrite;
   m_pWrite = pWrite;

   m_lWriteLen = lWriteLen + lDataLen;
   m_lWritePos = 0;

#ifdef CONNSECURE
   m_bWrite = true;
#endif

   // memprint(debugfile(), "Conn::Write buffer", m_pWrite, m_lWriteLen);

   return true;
}

// WriteBuffer: Return length of the write buffer
long Conn::WriteBuffer()
{
   return m_lWriteLen;
}

// FlagSecure: Mark all write buffer data after this point for sending only after secure setup completes
bool Conn::FlagSecure()
{
#ifdef CONNSECURE
   if(m_lWriteLen > 0)
   {
      m_lSecureWrite = m_lWriteLen;
      debug("Conn::FlagSecure write pos %ld\n", m_lSecureWrite);
   }

   if(m_iSecure == 0)
   {
      m_iSecure = SECURE_FLAG;
      debug("Conn::Read secure FLAG\n");
   }

   return true;
#else
   return false;
#endif
}

bool Conn::FlagCompress()
{
#ifdef CONNCOMPRESS
   if(m_lWriteLen > 0)
   {
      m_lCompressWrite = m_lWriteLen;
      debug("Conn::FlagCompress write pos %ld\n", m_lCompressWrite);
   }

   return true;
#else
   return false;
#endif
}

// Create: Make a new instance
Conn *Conn::Create(bool bSecure)
{
   Conn *pConn = NULL;

   debug("Conn::Create\n");

   pConn = new Conn();
   return pConn;
}

int Conn::GetError()
{
#ifdef WIN32
   return WSAGetLastError();
#else
   return errno;
#endif
}

// Setup a new socket
SOCKADDR_IN *Conn::Init(int iPort, const char *szCertFile)
{
   STACKTRACE
   int iSockOpt = 1;
#ifdef CONNSECURE
   int iVerify = 0, iReturn = 0;
   FILE *fCertFile = NULL;
#endif
   struct sockaddr_in *pAddr = NULL;
#ifdef WIN32
   DWORD dError = 0;
   WORD wVersion = 0;
   WSADATA wsaData;
#else
   struct rlimit rlim;
#endif

   debug("Conn::Init %d\n", iPort);

   if(m_iState != CLOSED)
   {
      debug("Conn::Init socketed not closed\n");
      return NULL;
   }

#ifdef UNIX
   // Ignore pipe errors
   signal(SIGPIPE, SIG_IGN);
#endif

#ifdef CONNSECURE
   if(m_pContext == NULL)
   {
      // Initialise SSL stuff

      debug("Conn::Init secure\n");

      SSL_load_error_strings();

      /* SSLeay_add_all_algorithms();
      SSLeay_add_all_ciphers();
      SSLeay_add_all_digests(); */

      SSLeay_add_ssl_algorithms();

      debug("Conn::Init create secure context\n");

      // Create a context
      m_pContext = SSL_CTX_new(SSLv23_method());
      if(m_pContext == NULL)
      {
         debug("Conn::Init SSL_CTX_new failed\n");
         return NULL;
      }

      if(szCertFile != NULL)
      {
         // Check the certificate file

         debug("Conn::Init certificate file %s\n", szCertFile);

         fCertFile = fopen(szCertFile, "r");
         if(fCertFile == NULL)
         {
            SetError();
            return NULL;
         }

         fclose(fCertFile);

         iReturn = SSL_CTX_use_certificate_file(m_pContext, szCertFile, X509_FILETYPE_PEM);
         if(iReturn <= 0)
         {
            // debug("Conn::Init cannot set certificate from %s\n", szCertFile);
            DEBUG_ERRORSSL("Cannot use certificate", iReturn)
            return NULL;
         }

         iReturn = SSL_CTX_use_RSAPrivateKey_file(m_pContext, szCertFile, X509_FILETYPE_PEM);
         if(iReturn <= 0)
         {
            // debug("Conn::Init cannot get private key from %s\n", szCertFile);
            DEBUG_ERRORSSL("Cannot get private key", iReturn)
            return NULL;
         }

         if(SSL_CTX_check_private_key(m_pContext) == 0)
         {
            // debug("Conn::Init private key does not match certificate key\n");
            DEBUG_ERRORSSL("Private key does not match certificate key", 0)
            return NULL;
         }

         // PEM_set_getkey_callback
      }
      else
      {
         debug("Conn::Init no certificate file used\n");

         if(m_iType == BIND)
         {
            SetError("No certificate file used");
            return NULL;
         }
      }

      if(m_iType == SERVER)
      {
         iVerify = SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE;
      }
      else if(m_iType == CLIENT)
      {
         iVerify = SSL_VERIFY_PEER;
      }
      SSL_CTX_set_verify(m_pContext, iVerify, ConnVerify);
   }

   debug("Conn::Init secure init complete\n");
#endif

#ifdef WIN32
   // WinSock specific setup
   if(m_bWSAStartup == false)
   {
      // Check WinSock v1.1 is available

      debug("Conn::Init WinSock startup\n");

      wVersion = MAKEWORD(1, 1);
      if(WSAStartup(wVersion, &wsaData) != 0)
      {
         DEBUG_ERROR("Unable to initialise WinSock")
         return NULL;
      }
      if(LOBYTE(wsaData.wVersion) != 1 || HIBYTE(wsaData.wVersion) != 1)
      {
         debug("Conn::Init BYTE check failed\n");
         SetError("WinSock is not v1.1");
         WSACleanup();

         return NULL;
      }
   }

   m_bWSAStartup = true;
#endif

#ifdef UNIX
   // Max out the available sockets
   getrlimit(RLIMIT_NOFILE, &rlim);
   rlim.rlim_cur = rlim.rlim_max;
   debug("Conn::Init setting file limit to %d\n", rlim.rlim_max);
   setrlimit(RLIMIT_NOFILE, &rlim);
#endif

   // Create a socket
   debug("Conn::Init socket creation\n");
   m_iSocket = socket(AF_INET, SOCK_STREAM, 0);
   if(m_iSocket == INVALID_SOCKET)
   {
      DEBUG_ERRORSOCK("socket failed")
      return NULL;
   }
#ifdef UNIX
   if(m_iSocket > m_iWidth)
   {
      m_iWidth = m_iSocket;
   }
#endif

   if(m_iType == BIND)
   {
      debug("Conn::Init socket binding\n");
      if(setsockopt(m_iSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&iSockOpt, sizeof(iSockOpt)) == SOCKET_ERROR)
      {
         DEBUG_ERRORSOCK("setsockopt failed")
         return NULL;
      }
   }

   /* if(SetSock() == false)
   {
      DEBUG_ERRORSOCK("SetSock failed");
      return NULL;
   } */

   m_bTimeout = true;

   // Bind to any local port
   pAddr = new SOCKADDR_IN;
   memset((char *)pAddr, '\0', sizeof(struct sockaddr_in));
   pAddr->sin_family = AF_INET;
   pAddr->sin_addr.s_addr = htonl(INADDR_ANY);
   pAddr->sin_port = htons(iPort);
   debug("Conn::Init binding to port %d\n", iPort);
   if(bind(m_iSocket, (struct sockaddr *)pAddr, sizeof(struct sockaddr_in)) == SOCKET_ERROR)
   {
      DEBUG_ERRORSOCK("bind failed")

      delete pAddr;

      return NULL;
   }

   m_iPort = iPort;

   return pAddr;
}

// Set socket options
bool Conn::SetSock()
{
// #ifdef WIN32
   unsigned long lReturn = 1;
// #endif

   // Setup non-blocking IO
/* #ifdef UNIX
   if(fcntl(m_iSocket, F_SETFL, O_NDELAY) == SOCKET_ERROR)
   {
      DEBUG_ERRORSOCK("fcntl failed")
      return false;
   }
#else */
   if(ioctlsocket(m_iSocket, FIONBIO, &lReturn) == SOCKET_ERROR)
   {
      DEBUG_ERRORSOCK("ioctlsocket failed")
      return false;
   }
// #endif

   return true;
}

// Select: Check for socket activity
int Conn::Select(bool bTimeout, bool bRead, bool bWrite)
{
   int iReady = 0;
   double dTick = 0;
   struct timeval *tvTimeout = NULL;
   // fd_set fErrors;

   // debug("Conn::Select timeout %s, write %ld\n", BoolStr(m_bTimeout), m_lWriteLen);

   if(bTimeout == false || UseTimeout() == false || m_lTimeout == 0)
   {
      // Come back straight away
      tvTimeout = new struct timeval;
      tvTimeout->tv_sec = 0;
      tvTimeout->tv_usec = 0;
      // debug("Conn::Select zero timeout (timeout %s, use %s, timeout %ld)\n", BoolStr(bTimeout), BoolStr(UseTimeout()), m_lTimeout);
   }
   else if(m_lTimeout != -1)
   {
      // Come back after a timeout
      tvTimeout = new struct timeval;
      tvTimeout->tv_sec = m_lTimeout / 1000;
      tvTimeout->tv_usec = 1000 * (m_lTimeout % 1000);
      // printf("Conn::Select timeout %d / %d\n", tvTimeout->tv_sec, tvTimeout->tv_usec);
   }
   /* else
   {
      debug("Conn::Select NULL timeout\n");
   } */

   // FD_ZERO(&fWrites);
   // FD_ZERO(&fErrors);

   // Add the member socket for read / write
   if(bRead == true)
   {
      FD_SET(m_iSocket, &m_fReads);
   }
   if(bWrite == true)
   {
      FD_SET(m_iSocket, &m_fWrites);
   }

   // FD_SET(m_iSocket, &fWrites);
   // FD_SET(m_iSocket, &fErrors);
#ifdef UNIX
   if(m_iSocket > m_iWidth)
   {
      // Number of sockets to check
      m_iWidth = m_iSocket;
   }
#endif

   /* if(m_iType == CLIENT)
   {
      debug("Conn::Select calling select timeout %ld/%ld\n", tvTimeout->tv_sec, tvTimeout->tv_usec);
   } */

   dTick = gettick();
   // iReady = select(m_iWidth + 1, &m_fReads, &fWrites, &fErrors, tvTimeout);
   iReady = select(m_iWidth + 1, &m_fReads, &m_fWrites, NULL, tvTimeout);
   /* if(m_iType == BIND) // && tickdiff(dTick) > 250)
   {
      debug("Conn::Select %d (width %d) ready in %ld ms (timeout %ld / %ld) at %ld\n", iReady, m_iWidth, tickdiff(dTick), tvTimeout->tv_sec, tvTimeout->tv_usec, time(NULL));
   } */

   /* if(FD_ISSET(m_iSocket, &fWrites))
   {
      debug("Conn::Select write set\n");
      iReady--;
   }
   if(FD_ISSET(m_iSocket, &fErrors))
   {
      debug("Conn::Select error set\n");
      iReady--;
   } */

   delete tvTimeout;

   return iReady;
}

void Conn::SetError(const char *szError)
{
   STACKTRACE
#ifdef WIN32
   char szCode[50];
#endif

   debug(DEBUGLEVEL_DEBUG, "Conn::SetError %s, %d\n", szError, GetError());

   if(szError == NULL)
   {
#ifdef WIN32
      // WinSock errors
      switch(GetError())
      {
         case 0:
            szError = NULL;
            break;

         case WSAENETDOWN:
            szError = "Network is down";
            break;

         case WSAHOST_NOT_FOUND:
            szError = "Host does not exist";
            break;

         case WSATRY_AGAIN:
            szError = "Host not found";
            break;

         case WSAEAFNOSUPPORT:
            szError = "TCP/IP support not present";
            break;

         case WSAECONNREFUSED:
            szError = "Connection refused";
            break;

         case WSAENETUNREACH:
            szError = "Network is unreachable";
            break;

         case WSAEHOSTUNREACH:
            szError = "No route to host";
            break;

         case WSAEADDRINUSE:
            szError = "Address already in use";
            break;

         case WSAECONNRESET:
            szError = "Connection reset by peer";
            break;

         case WSAECONNABORTED:
            szError = "Connection aborted";
            break;

         case WSAETIMEDOUT:
            szError = "Connection timed out";
            break;

         default:
            sprintf(szCode, "Unknown error %d", GetError());
            szError = szCode;
            break;
      }
#else
      // Unix errors
      szError = strerror(GetError());
#endif
   }

   m_szError = strmk(szError);

   debug(DEBUGLEVEL_DEBUG, "Conn::SetError string '%s'\n", m_szError);
}

#ifdef CONNSECURE
// SecureInit: Start secure setup
bool Conn::SecureInit()
{
   STACKTRACE

   // Create a new SSL connection
   m_pSSL = SSL_new(m_pContext);
   if(m_pSSL == NULL)
   {
      debug("Conn::SecureInit SSL_new failed\n");
      return false;
   }

   SSL_set_fd(m_pSSL, m_iSocket);

   // SSL_set_read_ahead(m_pSSL, 1);

   m_iSecure = SECURE_INIT;

   // State setting depends on the connection type
   if(m_iType == CLIENT)
   {
      SSL_set_connect_state(m_pSSL);
   }
   else
   {
      // SSL_clear(m_pSSL);
      SSL_set_accept_state(m_pSSL);
   }
   debug("Conn::SecureInit secure INIT\n");

   return true;
}

void Conn::SetErrorSSL(int iError, const char *szError)
{
   STACKTRACE
   char szCode[50];

   debug("Conn::SetErrorSSL %d %s, %d %s %s\n", iError, szError, GetError(), Hostname(), AddressStr());

   m_szError = NULL;

   if(szError == NULL)
   {
      switch(SSL_get_error(m_pSSL, iError))
      {
         case SSL_ERROR_NONE:
            szError = NULL;
            break;

         case SSL_ERROR_ZERO_RETURN:
            szError = "TLS/SSL connection has been closed";
            break;

         case SSL_ERROR_WANT_READ:
            szError = "The operation did not complete - want read";
            break;

         case SSL_ERROR_WANT_WRITE:
            szError = "The operation did not complete - want write";
            break;

         case SSL_ERROR_WANT_X509_LOOKUP:
            szError = "The operation did not complete - want X509 lookup";
            break;

         case SSL_ERROR_SYSCALL:
            if(ERR_get_error() == 0 && iError == 0)
            {
               szError = "Connection closed";
            }
            else
            {
               SetError();
            }
            break;

         case SSL_ERROR_SSL:
            szError = "SSL library failure";
            break;

         default:
            sprintf(szCode, "Unknown SSL error %d", iError);
            szError = szCode;
            break;
      }
   }

   if(m_szError == NULL)
   {
      m_szError = strmk(szError);
   }

   debug(DEBUGLEVEL_INFO, "Conn::SetErrorSSL string '%s'\n", m_szError);
}
#endif
