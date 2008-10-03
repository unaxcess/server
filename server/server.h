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
** server.h: Server functions
*/

#ifndef _SERVER_H_
#define _SERVER_H_

#include "User.h"
#include "MessageItem.h"

#include "DBSub.h"
#include "DBMessageRead.h"

#define SERVER_NAME "UNaXcess"
#define VERSION PROTOCOL

#define CONNDATA ((ConnData *)pConn->Data())
#define CONNUSER (CONNDATA != NULL ? CONNDATA->m_pUser : NULL)
#define CONNREADS (CONNDATA != NULL ? CONNDATA->m_pReads : NULL)
#define CONNFOLDERS (CONNDATA != NULL ? CONNDATA->m_pFolders : NULL)
#define CONNCHANNELS (CONNDATA != NULL ? CONNDATA->m_pChannels : NULL)
#define CONNSERVICES (CONNDATA != NULL ? CONNDATA->m_pServices : NULL)

// Request function group / ops
#define RFG_FOLDER 1
#define RFG_MESSAGE 2
#define RFG_CONTENT 3
#define RFG_BULLETIN 4
#define RFG_CHANNEL 5
#define RFG_SYSTEM 6
#define RFG_USER 7
#define RFG_SERVICE 8

#define RQ_ADD 1
#define RQ_DELETE 2
#define RQ_EDIT 3

#define CF_CONNID 1
#define CF_USERID 2
#define CF_USERPRI 3

// Per connection data structure
struct ConnAnnounce
{
   EDF *m_pAnnounce;
   struct ConnAnnounce *m_pNext;
};

struct ConnData
{
   bool m_bClose;
   long m_lTimeOn;
   long m_lTimeIdle;
   char *m_szLocation;
   int m_iStatus;
   int m_iRequests;
   int m_iAnnounces;

   int m_iLoginAttempts;

   UserItem *m_pUser;

   ConnAnnounce *m_pAnnounceFirst;
   ConnAnnounce *m_pAnnounceLast;

   long m_lTimeBusy;
   char *m_szClient;
   char *m_szProtocol;
   char *m_szProxy;
   unsigned long m_lProxy;
   bytes *m_pStatusMsg;
   long m_lAttachmentSize;

   DBSub *m_pFolders;
   DBMessageRead *m_pReads;

   DBSub *m_pChannels;
   DBSub *m_pServices;
};

bool stristr(bytes *pValue1, bytes *pValue2);
bool strmatch(char *szString, char *szPattern, bool bLazy, bool bFull, bool bIgnoreCase);

#define EDFSECTION_FUNCTION 16
#define EDFSECTION_NOTNULL 32
#define EDFSECTION_NOTEMPTY 64
#define EDFSECTION_GEZERO 32
#define EDFSECTION_GTZERO 64
#define EDFSECTION_MULTI 128

typedef bool (*EDFSECTIONBYTESFN)(bytes *);
typedef bool (*EDFSECTIONINTFN)(int);

bool EDFSection(EDF *pEDF, EDF *pIn, int iOp, char *szName, ...);
bool EDFSection(EDF *pEDF, EDF *pIn, char *szSection, int iOp, char *szName, ...);
bool EDFSection(EDF *pEDF, EDF *pIn, char *szSection, char *szSectionValue, int iOp, char *szName, ...);
bool EDFSection(EDF *pEDF, EDF *pIn, int iOp, bool bSection, char *szName, ...);

int ServerAnnounce(EDF *pData, const char *szAnnounce, EDF *pAnnounce, EDF *pAdmin = NULL, EDFConn *pConn = NULL);

bool PrivilegeValid(UserItem *pCurr, char *szMessage, int iTargetID = -1, char **szTargetName = NULL);

char *FilenameToMIME(EDF *pData, char *szFilename);

int ConnVersion(ConnData *pConnData, const char *szProtocol);

int RequestGroup(EDF *pIn);

bool ConnectionShut(EDF *pData, int iType, int iID, UserItem *pCurr, EDF *pIn);
bool ConnectionShut(EDFConn *pConn, EDF *pData, UserItem *pCurr = NULL, bool bLost = false, EDF *pIn = NULL);

EDFConn *ConnectionFind(int iType, int iID);
int ConnectionShadows(int iID, int iUserID, int iAccessLevel);
void ConnectionInfo(EDF *pEDF, EDFConn *pConn, int iLevel, bool bCurrent);
bool ConnectionTrust(EDFConn *pConn, EDF *pData);
bool ConnectionValid(EDFConn *pConn, EDF *pData);
char *ConnectionLocation(EDF *pData, char *szHostname, unsigned long lAddress);
void ConnectionLocations(EDF *pData);
bool ConnectionVisible(ConnData *pConnData, int iUserID, int iAccessLevel);

bool AttachmentSection(EDF *pData, int iBase, int iOp, int iAttOp, MessageItem *pMessage, EDF *pEDF, ConnData *pConnData, bool bMultiple, EDF *pIn, EDF *pOut, int iUserID, int iAccessLevel, int iSubType);

#endif
