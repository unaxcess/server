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
** UserItem.cpp: Implmentation of UserItem class
*/

#include <stdio.h>
#include <string.h>
#include <time.h>
/* #ifdef WIN32
#include <typeinfo.h>
#else
#include <typeinfo>
#endif */

#include "ua.h"

#include "Conn/Conn.h"

#include "UserItem.h"

// #define USERWRITE_HIDDEN 1

#define MAP_DETAIL(szDetail, pMember) \
if(stricmp(szName, szDetail) == 0) \
{ \
   if(bSet == true) \
   { \
      /* printf("UserItem::MapItem %s into %p\n", szDetail, *pItem); */ \
      pMember = *pItem; \
   } \
   else \
   { \
      /* printf("UserItem::MapItem %s out of %p\n", szDetail, pMember); */ \
      *pItem = pMember; \
   } \
\
   return true; \
}

long UserItem::m_lSystemTimeIdle = -1;
UserItem *UserItem::m_pInit = NULL;

// Constructor with no data
UserItem::UserItem(long lID) : EDFItem(lID)
{
   STACKTRACE

   init();
}

// Constructor with EDF data
UserItem::UserItem(EDF *pEDF) : EDFItem(pEDF, -1)
{
   STACKTRACE

   init();

   // Top level fields
   ExtractMember(pEDF, "usertype", &m_iType, 0);
   ExtractMember(pEDF, "name", &m_szName);
   ExtractMember(pEDF, "password", &m_szPassword);
   ExtractMember(pEDF, "accesslevel", &m_iAccessLevel, LEVEL_NONE);
   ExtractMember(pEDF, "accessname", &m_szAccessName);
   ExtractMember(pEDF, "gender", &m_iGender, 0);

   ExtractMember(pEDF, "description", &m_pDescription);
   // printf("UserItem::UserItem EDF description '%s'\n", m_szDescription);

   ExtractMember(pEDF, "created", &m_lCreated, -1);
   ExtractMember(pEDF, "ownerid", &m_lOwnerID, -1);

   // login section
   if(pEDF->Child("login") == true)
   {
      ExtractMember(pEDF, "client", &m_szClient);
      ExtractMember(pEDF, "hostname", &m_szHostname);
      /* if(pEDF->Child("hostname") == true)
      {
         ExtractEDF(pEDF, "hostname");
         pEDF->Parent();
      } */
      ExtractMember(pEDF, "address", &m_lAddress, 0);
      ExtractMember(pEDF, "location", &m_szLocation);
      ExtractMember(pEDF, "protocol", &m_szProtocol);
      m_bSecure = pEDF->GetChildBool("secure");
      ExtractMember(pEDF, "status", &m_iStatus, LOGIN_OFF);
      ExtractMember(pEDF, "statusmsg", &m_pStatusMsg);
      ExtractMember(pEDF, "timebusy", &m_lTimeBusy, -1);
      ExtractMember(pEDF, "timeidle", &m_lTimeIdle, -1);
      ExtractMember(pEDF, "timeoff", &m_lTimeOff, -1);
      ExtractMember(pEDF, "timeon", &m_lTimeOn, -1);

      pEDF->Parent();
   }

   // stats section
   if(pEDF->Child("stats") == true)
   {
      ExtractMember(pEDF, "numlogins", &m_pNumLogins.m_lValue, 0);
      ExtractMember(pEDF, "totallogins", &m_pTotalLogins.m_lValue, 0);
      ExtractMember(pEDF, "longestlogin", &m_pLongestLogin.m_lValue, 0);
      ExtractMember(pEDF, "nummsgs", &m_pNumMsgs.m_lValue, 0);
      ExtractMember(pEDF, "totalmsgs", &m_pTotalMsgs.m_lValue, 0);
      ExtractMember(pEDF, "lastmsg", &m_pLastMsg.m_lValue, -1);
      ExtractMember(pEDF, "numvotes", &m_pNumVotes.m_lValue, 0);

      pEDF->Parent();
   }

   if(pEDF->Child("periodstats") == true)
   {
      ExtractMember(pEDF, "numlogins", &m_pNumLogins.m_lPeriodValue, 0);
      ExtractMember(pEDF, "totallogins", &m_pTotalLogins.m_lPeriodValue, 0);
      ExtractMember(pEDF, "longestlogin", &m_pLongestLogin.m_lPeriodValue, 0);
      ExtractMember(pEDF, "nummsgs", &m_pNumMsgs.m_lPeriodValue, 0);
      ExtractMember(pEDF, "totalmsgs", &m_pTotalMsgs.m_lPeriodValue, 0);
      ExtractMember(pEDF, "lastmsg", &m_pLastMsg.m_lPeriodValue, -1);
      ExtractMember(pEDF, "numvotes", &m_pNumVotes.m_lPeriodValue, 0);

      pEDF->Parent();
   }

   EDFFields(pEDF);

   setup();
}

UserItem::UserItem(DBTable *pTable) : EDFItem(pTable, 32)
{
   STACKTRACE

   init();

   // printf("UserItem::UserItem entry table %p\n", pTable);

   GET_INT_TABLE(1, m_iType, 0)
   GET_STR_TABLE(2, m_szName, false)
   GET_STR_TABLE(3, m_szPassword, false)
   GET_INT_TABLE(4, m_iAccessLevel, LEVEL_NONE)
   GET_STR_TABLE(5, m_szAccessName, false)
   GET_INT_TABLE(6, m_iGender, 0)

   GET_BYTES_TABLE(7, m_pDescription, true)
   // printf("UserItem::UserItem table description '%s'\n", m_szDescription);

   GET_INT_TABLE(8, m_lCreated, -1)
   GET_INT_TABLE(9, m_lOwnerID, -1)

   STACKTRACEUPDATE

   GET_STR_TABLE(10, m_szClient, false)
   GET_STR_TABLE(11, m_szHostname, false)
   GET_INT_TABLE(12, m_lAddress, 0)
   GET_STR_TABLE(13, m_szLocation, false)
   GET_STR_TABLE(14, m_szProtocol, false)
   GET_INT_TABLE(15, m_bSecure, false)
   // m_iStatus = LOGIN_OFF;
   // m_lTimeBusy = -1;
   // m_lTimeIdle = -1;
   GET_INT_TABLE(16, m_lTimeOn, -1)
   GET_INT_TABLE(17, m_lTimeOff, -1)

   GET_INT_TABLE(18, m_pNumLogins.m_lValue, 0)
   GET_INT_TABLE(19, m_pTotalLogins.m_lValue, 0)
   GET_INT_TABLE(20, m_pLongestLogin.m_lValue, 0)
   GET_INT_TABLE(21, m_pNumMsgs.m_lValue, 0)
   GET_INT_TABLE(22, m_pTotalMsgs.m_lValue, 0)
   GET_INT_TABLE(23, m_pLastMsg.m_lValue, -1)
   GET_INT_TABLE(24, m_pNumVotes.m_lValue, 0)

   GET_INT_TABLE(25, m_pNumLogins.m_lPeriodValue, 0)
   GET_INT_TABLE(26, m_pTotalLogins.m_lPeriodValue, 0)
   GET_INT_TABLE(27, m_pLongestLogin.m_lPeriodValue, 0)
   GET_INT_TABLE(28, m_pNumMsgs.m_lPeriodValue, 0)
   GET_INT_TABLE(29, m_pTotalMsgs.m_lPeriodValue, 0)
   GET_INT_TABLE(30, m_pLastMsg.m_lPeriodValue, -1)
   GET_INT_TABLE(31, m_pNumVotes.m_lPeriodValue, 0)

   STACKTRACEUPDATE

   // printf("UserItem::UserItem EDF %p\n", m_pEDF);
   EDFParser::debugPrint("UserItem::UserItem EDF", m_pEDF);

   EDFFields(NULL);

   setup();
}

// Destructor
UserItem::~UserItem()
{
   // STACKTRACE

   // printf("UserItem::~UserItem\n");

   // Top level fields
   delete[] m_szName;
   delete[] m_szPassword;
   delete[] m_szAccessName;

   delete m_pDescription;

   // login section
   delete[] m_szClient;
   delete[] m_szHostname;
   delete[] m_szLocation;
   delete[] m_szProtocol;
   delete m_pStatusMsg;

   // details section
   DeleteDetail("email");
   DeleteDetail("homepage");
   DeleteDetail("realname");
   DeleteDetail("sms");
   DeleteDetail("refer");
   DeleteDetail("picture");
   DeleteDetail("pgpkey");
}

/* const char *UserItem::GetClass()
{
   return typeid(this).name();
} */

// Member access
int UserItem::GetUserType()
{
   return m_iType;
}

bool UserItem::SetUserType(int iUserType)
{
   if(mask(iUserType, USERTYPE_AGENT) == true)
   {
      m_iAccessLevel = LEVEL_NONE;
      STR_NULL(m_szAccessName);
   }

   SET_INT(m_iType, iUserType)
}

char *UserItem::GetName(bool bCopy, long lDate)
{
   bool bLoop = false, bDataCopy = false;
   long lNameDate = 0, lTemp = 0;
   char *szName = NULL, *szTemp = NULL;

   if(lDate == -1)
   {
      GET_STR(m_szName)
   }

   STACKTRACE

   // EDFPrint("UserItem::GetName old names", m_pEDF);

   bDataCopy = m_pEDF->DataCopy(false);
   szName = m_szName;

   // printf("UserItem::GetName old check %s (%ld)\n", szName, lDate);
   m_pEDF->Root();
   bLoop = m_pEDF->Child("oldname");
   while(bLoop == true)
   {
      szTemp = NULL;
      lTemp = 0;

      m_pEDF->Get(NULL, &lTemp);
      m_pEDF->GetChild("name", &szTemp);
      if(lTemp > 0 && szTemp != NULL)
      {
         // printf("UserItem::GetName check %s (%ld) nd=%ld", szTemp, lTemp, lNameDate);
         if(lTemp > lDate && (lNameDate == 0 || lTemp < lNameDate))
         {
            // printf(". setting");
            lNameDate = lTemp;
            szName = szTemp;
         }
         // printf("\n");
      }

      bLoop = m_pEDF->Next("oldname");
      if(bLoop == false)
      {
         m_pEDF->Parent();
      }
   }

   m_pEDF->DataCopy(bDataCopy);

   if(bCopy == true)
   {
      szName = strmk(szName);
   }
   return szName;
}

bool UserItem::SetName(char *szName)
{
   if(szName == NULL)
   {
      return false;
   }

   m_pEDF->Root();

   if(m_szName != NULL && strcmp(m_szName, szName) != 0)
   {
      m_pEDF->Add("oldname", (int)(time(NULL)));
      m_pEDF->AddChild("name", m_szName);
      m_pEDF->Parent();
   }

   SET_STR(m_szName, szName, UA_NAME_LEN)
}

char *UserItem::GetPassword(bool bCopy)
{
   GET_STR(m_szPassword)
}

bool UserItem::SetPassword(char *szPassword)
{
   SET_STR(m_szPassword, szPassword, UA_NAME_LEN)
}

int UserItem::GetAccessLevel()
{
   return m_iAccessLevel;
}

bool UserItem::SetAccessLevel(int iAccessLevel)
{
   SET_INT(m_iAccessLevel, iAccessLevel)
}

bool UserItem::SetAccessLevel(EDF *pEDF)
{
   SET_INT_EDF(m_iAccessLevel, "accesslevel")
}

char *UserItem::GetAccessName(bool bCopy)
{
   GET_STR(m_szAccessName)
}

bool UserItem::SetAccessName(char *szAccessName)
{
   SET_STR(m_szAccessName, szAccessName, UA_NAME_LEN)
}

bool UserItem::SetAccessName(EDF *pEDF)
{
   SET_STR_EDF(m_szAccessName, "accessname", false, UA_NAME_LEN)
}

int UserItem::GetGender()
{
   return m_iGender;
}

bool UserItem::SetGender(EDF *pEDF)
{
   SET_INT_EDF(m_iGender, "gender")
}

bytes *UserItem::GetDescription(bool bCopy)
{
   GET_BYTES(m_pDescription)
}

bool UserItem::SetDescription(EDF *pEDF)
{
   SET_BYTES_EDF(m_pDescription, "description", false, -1)
}

long UserItem::GetCreated()
{
   return m_lCreated;
}

bool UserItem::SetCreated(EDF *pEDF)
{
   SET_INT_EDF(m_lCreated, "created")
}

long UserItem::GetOwnerID()
{
   return m_lOwnerID;
}

bool UserItem::SetOwnerID(long lOwnerID)
{
   SET_INT(m_lOwnerID, lOwnerID)
}

EDF *UserItem::GetAllowDeny(bool bCreate)
{
   GET_EDF_SECTION("login", NULL)
}

/* bool UserItem::SetAddress(EDF *pEDF)
{
   SET_STR_EDF(m_szAddress, "address")
} */

bool UserItem::SetAddress(unsigned long lAddress)
{
   SET_INT(m_lAddress, lAddress)
}

bool UserItem::SetAddressProxy(unsigned long lAddressProxy)
{
   SET_EDF_INT(NULL, "addressproxy", lAddressProxy, true)
}

char *UserItem::GetClient(bool bCopy)
{
   GET_STR(m_szClient)
}

bool UserItem::SetClient(char *szClient)
{
   SET_STR(m_szClient, szClient, UA_NAME_LEN)
}

bool UserItem::SetHostname(char *szHostname)
{
   SET_STR(m_szHostname, szHostname, -1)
}

bool UserItem::SetHostname(EDF *pEDF)
{
   SET_STR_EDF(m_szHostname, "hostname", false, -1)
}

bool UserItem::SetHostnameProxy(char *szHostnameProxy)
{
   SET_EDF_STR(NULL, "hostnameproxy", szHostnameProxy, true, -1)
}

bool UserItem::SetLocation(char *szLocation)
{
   SET_STR(m_szLocation, szLocation, UA_SHORTMSG_LEN)
}

bool UserItem::SetLocation(EDF *pEDF)
{
   SET_STR_EDF(m_szLocation, "location", false, UA_SHORTMSG_LEN)
}

bool UserItem::SetProtocol(char *szProtocol)
{
   SET_STR(m_szProtocol, szProtocol, UA_NAME_LEN)
}

bool UserItem::SetSecure(bool bSecure)
{
   SET_INT(m_bSecure, bSecure)
}

int UserItem::GetStatus()
{
   return m_iStatus;
}

bool UserItem::SetStatus(int iStatus)
{
   SET_INT(m_iStatus, iStatus)
}

bytes *UserItem::GetStatusMsg(bool bCopy)
{
   GET_BYTES(m_pStatusMsg)
}

bool UserItem::SetStatusMsg(bytes *pStatusMsg)
{
   SET_BYTES(m_pStatusMsg, pStatusMsg, UA_SHORTMSG_LEN)
}

/* bool UserItem::SetStatusMsg(EDF *pEDF)
{
   SET_BYTES_EDF(m_pStatusMsg, "statusmsg", true, UA_SHORTMSG_LEN)
} */

long UserItem::GetTimeOff()
{
   return m_lTimeOff;
}

bool UserItem::SetTimeOff(long lTimeOff)
{
   SET_INT(m_lTimeOff, lTimeOff)
}

long UserItem::GetTimeOn()
{
   return m_lTimeOn;
}

bool UserItem::SetTimeOn(long lTimeOn)
{
   SET_INT(m_lTimeOn, lTimeOn)
}

bool UserItem::SetTimeBusy(long lTimeBusy)
{
   SET_INT(m_lTimeBusy, lTimeBusy)
}

bool UserItem::SetTimeIdle(long lTimeIdle)
{
   SET_INT(m_lTimeIdle, lTimeIdle)
}

EDF *UserItem::GetLogin(bool bCreate)
{
   GET_EDF_SECTION("login", NULL)
}

long UserItem::GetStat(char *szName, bool bPeriod)
{
   long lReturn = 0;
   stat_item *pItem = NULL;

   pItem = MapStat(szName);

   if(pItem == NULL)
   {
      return 0;
   }

   if(bPeriod == true)
   {
      lReturn = pItem->m_lPeriodValue;
   }
   else
   {
      lReturn = pItem->m_lValue;
   }

   return lReturn;
}

bool UserItem::SetStat(char *szName, long lValue)
{
   return SetStat(szName, lValue, false) && SetStat(szName, lValue, true);
}

bool UserItem::SetStat(char *szName, long lValue, bool bPeriod)
{
   stat_item *pItem = NULL;

   pItem = MapStat(szName);

   if(pItem == NULL)
   {
      return false;
   }

   if(bPeriod == true)
   {
      SET_INT(pItem->m_lPeriodValue, lValue)
   }
   else
   {
      SET_INT(pItem->m_lValue, lValue)
   }

   return true;
}

bool UserItem::SetStat(char *szName, EDF *pIn, bool bPeriod)
{
   bool bReturn = false;
   long lValue = 0;

   if(pIn->GetChild(szName, &lValue) == true)
   {
      bReturn = SetStat(szName, lValue, bPeriod);
   }

   return bReturn;
}

bool UserItem::IncStat(char *szName, long lValue)
{
   stat_item *pItem = NULL;

   pItem = MapStat(szName);

   if(pItem == NULL)
   {
      return false;
   }

   pItem->m_lValue += lValue;
   pItem->m_lPeriodValue += lValue;

   SetChanged(true);

   return true;
}

/* long UserItem::GetNumLogins()
{
   return m_lNumLogins;
}

bool UserItem::SetNumLogins(long lNumLogins)
{
   SET_INT(m_lNumLogins, lNumLogins)
}

bool UserItem::SetNumLogins(EDF *pEDF)
{
   SET_INT_EDF(m_lNumLogins, "numlogins")
}

long UserItem::GetTotalLogins()
{
   return m_lTotalLogins;
}

bool UserItem::SetTotalLogins(long lTotalLogins)
{
   SET_INT(m_lTotalLogins, lTotalLogins)
}

bool UserItem::SetTotalLogins(EDF *pEDF)
{
   SET_INT_EDF(m_lTotalLogins, "totallogins")
}

long UserItem::GetLongestLogin()
{
   return m_lLongestLogin;
}

bool UserItem::SetLongestLogin(long lLongestLogin)
{
   SET_INT(m_lLongestLogin, lLongestLogin)
}

bool UserItem::SetLongestLogin(EDF *pEDF)
{
   SET_INT_EDF(m_lLongestLogin, "longestlogin")
}

long UserItem::GetNumMsgs()
{
   return m_lNumMsgs;
}

bool UserItem::SetNumMsgs(long lNumMsgs)
{
   SET_INT(m_lNumMsgs, lNumMsgs)
}

bool UserItem::SetNumMsgs(EDF *pEDF)
{
   SET_INT_EDF(m_lNumMsgs, "nummsgs")
}

long UserItem::GetTotalMsgs()
{
   return m_lTotalMsgs;
}

bool UserItem::SetTotalMsgs(long lTotalMsgs)
{
   SET_INT(m_lTotalMsgs, lTotalMsgs)
}

bool UserItem::SetTotalMsgs(EDF *pEDF)
{
   SET_INT_EDF(m_lTotalMsgs, "totalmsgs")
}

long UserItem::GetLastMsg()
{
   return m_lLastMsg;
}

bool UserItem::SetLastMsg(long lLastMsg)
{
   SET_INT(m_lLastMsg, lLastMsg)
}

long UserItem::GetNumVotes()
{
   return m_lNumVotes;
}

bool UserItem::SetNumVotes(long lNumVotes)
{
   if(lNumVotes < 0)
   {
      return false;
   }

   SET_INT(m_lNumVotes, lNumVotes)
} */

bool UserItem::GetDetail(char *szName, char **szContentType, bytes **pData, int *iType)
{
   STACKTRACE
   detail_item *pItem = NULL;

   pItem = MapDetail(szName);

   if(pItem == NULL)
   {
      return false;
   }

   if(szContentType != NULL)
   {
      *szContentType = pItem->m_szContentType;
   }
   if(pData != NULL)
   {
      *pData = pItem->m_pData;
   }
   if(iType != NULL)
   {
      *iType = pItem->m_iType;
   }

   return true;
}

// MessageSpec ignore start
bool UserItem::SetDetail(char *szName, char *szContentType, bytes *pData, int iType)
{
   detail_item *pItem = NULL;

   debug("UserItem::SetDetail entry %s %s %s(%ld) %d\n", szName, szContentType, pData != NULL ? pData->Data(false) : NULL, pData != NULL ? pData->Length() : -1, iType);

   pItem = MapDetail(szName);
   if(pItem == NULL)
   {
      debug("UserItem::SetDetail exit false\n");
      return false;
   }

   if(pData != NULL)
   {
      delete[] pItem->m_szContentType;
      pItem->m_szContentType = strmk(szContentType);

      delete pItem->m_pData;
      pItem->m_pData = new bytes(pData);
   }
   pItem->m_iType = iType;

   debug("UserItem::SetDetail exit true\n");
   return true;
}
// MessageSpec ignore stop

bool UserItem::DeleteDetail(char *szName)
{
   detail_item *pItem = NULL;

   debug("UserItem::DeleteDetail entry %s\n", szName);

   pItem = MapDetail(szName);
   if(pItem == NULL)
   {
      debug("UserItem::DeleteDetail exit false\n");
      return false;
   }

   DeleteDetail(pItem);

   debug("UserItem::DeleteDetail exit true\n");
   return true;
}

EDF *UserItem::GetClientEDF(bool bCreate)
{
   GET_EDF_SECTION("client", m_szClient)
}

EDF *UserItem::GetAgent(bool bCreate)
{
   GET_EDF_SECTION("agent", NULL)
}

bool UserItem::SetAgentData(EDF *pEDF)
{
   SET_EDF("agent")

   m_pEDF->DeleteChild("data");
   if(pEDF != NULL)
   {
      m_pEDF->Add("data");
      m_pEDF->Copy(pEDF, false);
   }

   return true;
}

char *UserItem::GetAgentName(bool bCopy)
{
   GET_STR_EDF("agent", NULL)
}

bool UserItem::SetAgentName(char *szAgentName)
{
   SET_EDF_STR("agent", (char *)NULL, szAgentName, false, UA_NAME_LEN)
}

EDF *UserItem::GetPrivilege(bool bCreate)
{
   GET_EDF_SECTION("privilege", NULL)
}

int UserItem::GetMarking()
{
   return m_iMarking;
}

bool UserItem::SetMarking(int iMarking)
{
   SET_INT(m_iMarking, iMarking)
}

int UserItem::GetArchiving()
{
   return m_iArchiving;
}

bool UserItem::SetArchiving(int iArchiving)
{
   SET_INT(m_iArchiving, iArchiving)
}

EDF *UserItem::GetRules(bool bCreate)
{
   GET_EDF_SECTION("rules", NULL)
}

int UserItem::AddRule(EDF *pRule)
{
   int iReturn = 0;

   EDFParser::debugPrint("UserItem::AddRule entry", pRule, EDFElement::EL_CURR + EDFElement::PR_SPACE);

   if(m_pEDF->Child("rules") == false)
   {
      m_pEDF->Add("rules");
   }

   iReturn = EDFMax(m_pEDF, "rule", false) + 1;

   m_pEDF->Add("rule", iReturn);
   m_pEDF->Copy(pRule, false);
   m_pEDF->Parent();

   // EDFParser::debugPrint("UserItem::AddRule rules", m_pEDF, EDFElement::EL_CURR + EDFElement::PR_SPACE);

   m_pEDF->Parent();

   SetChanged(true);

   debug("UserItem::AddRule exit true\n");
   // EDFParser::debugPrint("UserItem::AddRule exit true", m_pEDF);
   return iReturn;
}

bool UserItem::DeleteRule(int iRuleID)
{
   debug("UserItem::DeleteRule entry %d\n", iRuleID);

   if(m_pEDF->Child("rules") == false)
   {
      debug("UserItem::DeleteRule exit false\n");
      return false;
   }

   if(EDFFind(m_pEDF, "rule", iRuleID, false) == false)
   {
      debug("UserItem::DeleteRule exit false\n");
      return false;
   }

   EDFParser::debugPrint("UserItem::DeleteRule deleting", m_pEDF, EDFElement::EL_CURR + EDFElement::PR_SPACE);
   m_pEDF->Delete();

   EDFParser::debugPrint("UserItem::DeleteRule rules", m_pEDF, EDFElement::EL_CURR + EDFElement::PR_SPACE);

   m_pEDF->Parent();

   SetChanged(true);

   debug("UserItem::DeleteRule exit true\n");
   return true;
}

EDF *UserItem::GetFriends(bool bCreate)
{
   GET_EDF_SECTION("friends", NULL)
}

bool UserItem::IsIdle(long lTimeIdle)
{
   if(m_lSystemTimeIdle != -1 && lTimeIdle != -1 && time(NULL) >= lTimeIdle + m_lSystemTimeIdle)
   {
      return true;
   }

   return false;
}

bool UserItem::WriteLogin(EDF *pEDF, int iLevel, int iID, int iStatus, bool bSecure, int iRequests, int iAnnounces, long lTimeOn, long lTimeOff, long lTimeBusy, long lTimeIdle, const char *szStatusName, bytes *pStatusMsg, const char *szHostname, unsigned long lAddress, const char *szProxy, unsigned long lProxy, const char *szLocation, const char *szClient, const char *szProtocol, long lAttachmentSize, bool bCurrent)
{
   STACKTRACE
   int iPos = EDFElement::ABSLAST;
   char *szTemp = NULL;

   // printf("UserItem::WriteLogin level %d\n", iLevel);

   if(bCurrent == true)
   {
      iPos = EDFElement::FIRST;
   }

   if(mask(iLevel, USERITEMWRITE_LOGINFLAT) == false)
   {
      // MessageSpec condition start
      if(iID != -1)
      {
         pEDF->Add("login", iID, iPos);
      }
      else
      {
         pEDF->Add("login", (char *)NULL, iPos);
      }
      // MessageSpec condition stop
   }
   else if(iID != -1)
   {
      pEDF->AddChild("connectionid", iID, iPos);
   }

   /* if(bCurrent == true)
   {
      pEDF->AddChild("current", true);
   } */

   // printf("UserItem::WriteLogin time off %ld\n", lTimeOff);
   if(mask(iStatus, LOGIN_ON) == true)
   {
      if(IsIdle(lTimeIdle) == true)
      {
         iStatus |= LOGIN_IDLE;
      }

      // printf("UserItem::WriteLogin status output %d\n", iStatus);
      pEDF->AddChild("status", iStatus);

      if(mask(iStatus, LOGIN_BUSY) == true || mask(iLevel, EDFITEMWRITE_HIDDEN) == true)
      {
         pEDF->AddChild("timebusy", lTimeBusy);
      }
      if(pStatusMsg != NULL && pStatusMsg->Length() > 0)
      {
         pEDF->AddChild(szStatusName, pStatusMsg);
      }
      if(mask(iStatus, LOGIN_IDLE) == true)
      {
         pEDF->AddChild("timeidle", lTimeIdle);
      }
   }
   if(lTimeOff != -1)
   {
      pEDF->AddChild("timeoff", lTimeOff);
   }

   if(mask(iLevel, USERITEMWRITE_LOGIN) == true)
   {
      if(lTimeOn != -1)
      {
         pEDF->AddChild("timeon", lTimeOn);
      }

      if(mask(iLevel, EDFITEMWRITE_ADMIN) == true && bSecure == true)
      {
         pEDF->AddChild("secure", true);
      }
      if(mask(iLevel, EDFITEMWRITE_ADMIN) == true)
      {
         if(iRequests > 0)
         {
            pEDF->AddChild("requests", iRequests);
         }
         if(iAnnounces > 0)
         {
            pEDF->AddChild("announces", iAnnounces);
         }
      }

      if(szLocation != NULL)
      {
         STACKTRACEUPDATE
         pEDF->AddChild("location", szLocation);
      }

      STACKTRACEUPDATE

      if(mask(iLevel, EDFITEMWRITE_ADMIN) == true)
      {
         // debug("UserItem::WriteLogin proxy %s / %ld\n", szProxy, lProxy);
         if(lProxy > 0)
         {
            if(szProxy != NULL && strlen(szProxy) > 0)
            {
               pEDF->AddChild("hostname", szProxy);
            }
            if(lProxy > 0)
            {
               szTemp = Conn::AddressToString(lProxy);
               // printf("UserItem::WriteLogin address %lu -> %s\n", lProxy, szTemp);
               pEDF->AddChild("address", szTemp);
               delete[] szTemp;
            }

            if(szHostname != NULL)
            {
               pEDF->AddChild("proxyhostname", szHostname);
            }
            if(lAddress > 0)
            {
               szTemp = Conn::AddressToString(lAddress);
               pEDF->AddChild("proxyaddress", szTemp);
               delete[] szTemp;
            }
         }
         else
         {
            if(szHostname != NULL && strlen(szHostname) > 0)
            {
               pEDF->AddChild("hostname", szHostname);
            }
            if(lAddress > 0)
            {
               szTemp = Conn::AddressToString(lAddress);
               pEDF->AddChild("address", szTemp);
               delete[] szTemp;
            }
         }

         if(szClient != NULL)
         {
            pEDF->AddChild("client", szClient);
         }
         if(szProtocol != NULL)
         {
            pEDF->AddChild("protocol", szProtocol);
         }

         if(lAttachmentSize != 0)
         {
            pEDF->AddChild("attachmentsize", lAttachmentSize);
         }
      }
   }

   // EDFPrint("UserItem::WriteLogin", pEDF);

   if(mask(iLevel, USERITEMWRITE_LOGINFLAT) == false)
   {
      pEDF->Parent();
   }

   return true;
}

bool UserItem::SetSystemTimeIdle(long lTimeIdle)
{
   m_lSystemTimeIdle = lTimeIdle;

   return true;
}

UserItem *UserItem::NextRow(DBTable *pTable)
{
   STACKTRACE

   if(m_pInit == NULL)
   {
      m_pInit = new UserItem((long)0);
      // debug("UserItem::NextRow UserItem class %s\n", m_pInit->GetClass());
   }

   m_pInit->InitFields(pTable);

   if(pTable->NextRow() == false)
   {
      return NULL;
   }

   return new UserItem(pTable);
}

bool UserItem::Write(EDF *pEDF, int iLevel, const char *szStatusMsg, const char *szClient)
{
   bool bReturn = false;

   m_szWriteStatusMsg = (char *)szStatusMsg;
   m_szWriteClient = (char *)szClient;

   bReturn = EDFItem::Write(pEDF, iLevel);

   m_szWriteStatusMsg = NULL;
   m_szWriteClient = NULL;

   return bReturn;
}

// IsEDFField: Field name checker
bool UserItem::IsEDFField(char *szName)
{
   // Fields that have member storage
   if(stricmp(szName, "accesslevel") == 0 ||
      stricmp(szName, "accessname") == 0 ||
      stricmp(szName, "created") == 0 ||
      stricmp(szName, "description") == 0 ||
      stricmp(szName, "gender") == 0 ||
      stricmp(szName, "login") == 0 ||
      stricmp(szName, "name") == 0 ||
      stricmp(szName, "ownerid") == 0 ||
      stricmp(szName, "password") == 0 ||
      stricmp(szName, "stats") == 0 ||
      stricmp(szName, "periodstats") == 0 ||
      stricmp(szName, "usertype") == 0)
   {
      return false;
   }

   // Fields that should not be stored
   if(stricmp(szName, "bufferstatus") == 0 ||
      stricmp(szName, "channels") == 0 ||
      stricmp(szName, "checknew") == 0 ||
      stricmp(szName, "creatorid") == 0 ||
      stricmp(szName, "currchannel") == 0 ||
      stricmp(szName, "currfoldername") == 0 ||
      stricmp(szName, "currfolder") == 0 ||
      stricmp(szName, "currfrom") == 0 ||
      stricmp(szName, "currmessage") == 0 ||
      stricmp(szName, "deleted") == 0 ||
      stricmp(szName, "edfrequest") == 0 ||
      stricmp(szName, "folderaddid") == 0 ||
      stricmp(szName, "folderaddstatus") == 0 ||
      stricmp(szName, "foldersubscribe") == 0 ||
      stricmp(szName, "folderunreadonly") == 0 ||
      stricmp(szName, "folderunsubonly") == 0 ||
      stricmp(szName, "input") == 0 ||
      stricmp(szName, "listdetails") == 0 ||
      stricmp(szName, "listtype") == 0 ||
      stricmp(szName, "lookupparent") == 0 ||
      stricmp(szName, "messagefolder") == 0 ||
      stricmp(szName, "messageisreply") == 0 ||
      stricmp(szName, "messagereply") == 0 ||
      stricmp(szName, "messageto") == 0 ||
      stricmp(szName, "messagestatus") == 0 ||
      stricmp(szName, "messagesubject") == 0 ||
      stricmp(szName, "messagetoid") == 0 ||
      stricmp(szName, "navmode") == 0 ||
      stricmp(szName, "nextunreadmsg") == 0 ||
      stricmp(szName, "oldstatus") == 0 ||
      stricmp(szName, "page") == 0 ||
      stricmp(szName, "pageid") == 0 ||
      stricmp(szName, "pagedivert") == 0 ||
      stricmp(szName, "pagelogin") == 0 ||
      stricmp(szName, "pagename") == 0 ||
      stricmp(szName, "pagetext") == 0 ||
      stricmp(szName, "pagestatus") == 0 ||
      stricmp(szName, "replyfolder") == 0 ||
      stricmp(szName, "talkstatus") == 0 ||
      stricmp(szName, "voted") == 0 ||
      stricmp(szName, "votetype") == 0 ||
      stricmp(szName, "whosort") == 0 ||
      stricmp(szName, "whotype") == 0 ||
      stricmp(szName, "") == 0)
   {
      return false;
   }

   if(stricmp(szName, "details") != 0 &&
      stricmp(szName, "agent") != 0 &&
      stricmp(szName, "client") != 0 &&
      stricmp(szName, "proxy") != 0 &&
      stricmp(szName, "privilege") != 0 &&
      stricmp(szName, "friends") != 0)
   {
      debug("UserItem::IsEDFField %s, true\n", szName);
   }
   return true;
}

// WriteFields: Output to EDF
bool UserItem::WriteFields(EDF *pEDF, int iLevel)
{
   STACKTRACE
   int iUserType = 0;
   // int iStatus = LOGIN_ON;
   // unsigned long lAddress = 0;
   unsigned long lProxy = 0;
   // char *szHostname = NULL, *szTemp = NULL;
   char *szProxy = NULL;

   if(iLevel == -1)
   {
      iLevel = EDFITEMWRITE_HIDDEN + EDFITEMWRITE_ADMIN + USERITEMWRITE_LOGIN + USERITEMWRITE_DETAILS + USERITEMWRITE_SELF;
   }

   // printf("UserItem::WriteFields %d, %ld\n", iLevel, GetID());

   /* if(mask(iLevel, USERWRITE_FLAT) == true)
   {
      pEDF->Parent();
      pEDF->DeleteChild("user");
      pEDF->AddChild("userid", GetID());
   } */

   STACKTRACEUPDATE

   // Top level fields
   iUserType = m_iType;
   if(GetDeleted() == true)
   {
      iUserType += USERTYPE_DELETED;
   }
   if(iUserType > 0)
   {
      pEDF->AddChild("usertype", iUserType);
   }
   pEDF->AddChild("name", m_szName);
   if(mask(iLevel, EDFITEMWRITE_HIDDEN) == true)
   {
      pEDF->AddChild("password", m_szPassword);
   }
   pEDF->AddChild("accesslevel", m_iAccessLevel);
   if(m_szAccessName != NULL && stricmp(m_szAccessName, "") != 0)
   {
      // printf("UserItem::WriteFields accessname '%s'\n", m_szAccessName);
      pEDF->AddChild("accessname", m_szAccessName);
   }
   if(m_iGender != 0)
   {
      pEDF->AddChild("gender", m_iGender);
   }

   // printf("UserItem::WriteFields description '%s'\n", m_szDescription);
   if(mask(iLevel, USERITEMWRITE_DETAILS) == true && m_pDescription != NULL)
   {
      pEDF->AddChild("description", m_pDescription);
   }

   if(mask(iLevel, EDFITEMWRITE_ADMIN) == true && mask(iLevel, USERITEMWRITE_DETAILS) == true)
   {
      STACKTRACEUPDATE

      if(m_lCreated != -1)
      {
         pEDF->AddChild("created", m_lCreated);
      }
      if(m_lOwnerID != -1)
      {
         pEDF->AddChild("ownerid", m_lOwnerID);
      }
   }

   // login section
   // printf("UserItem::WriteFields login check %d, %ld to %ld / %d\n", GetID(), m_lTimeOn, m_lTimeOff, m_iStatus);
   /* if(m_lTimeOn != -1 && !(mask(m_iStatus, LOGIN_SILENT) == true && mask(iLevel, USERWRITE_ADMIN) == false))
   {
      // printf("UserItem::WriteFields login section check %d, %s / %s\n", iLevel, BoolStr(mask(iLevel, USERWRITE_LOGIN)), BoolStr(mask(iLevel, USERWRITE_NOSTATUS)));
      if(mask(iLevel, USERWRITE_LOGIN) == false && mask(iLevel, USERWRITE_NOSTATUS) == false)
      {
         WriteLogin(pEDF, false, -1, iLevel, m_iStatus, false, 0, 0, -1, m_lTimeOff, m_lTimeBusy, -1, m_szWriteStatusMsg, m_pStatusMsg, NULL, 0, NULL, 0, NULL, NULL, NULL, 0);
      }
      else if(mask(iLevel, USERWRITE_LOGIN) == true)
      {
         m_pEDF->GetChild("hostnameproxy", &szProxy);
         m_pEDF->GetChild("addressproxy", &lProxy);
         WriteLogin(pEDF, true, -1, iLevel, m_iStatus, false, 0, 0, m_lTimeOn, m_lTimeOff, m_lTimeBusy, m_lTimeIdle, m_szWriteStatusMsg != NULL ? m_szWriteStatusMsg : "statusmsg", m_pStatusMsg, m_szHostname, m_lAddress, szProxy, lProxy, m_szLocation, m_szClient, m_szProtocol, 0);
         if(m_pEDF->GetCopy() == true)
         {
            delete[] szProxy;
         }
      }
   } */
   if(mask(iLevel, USERITEMWRITE_LOGIN) == true || mask(iLevel, USERITEMWRITE_LOGINFLAT) == true || mask(iLevel, EDFITEMWRITE_HIDDEN) == true)
   {
      WriteLogin(pEDF, iLevel, -1, m_iStatus, m_bSecure, 0, 0, m_lTimeOn, m_lTimeOff, m_lTimeBusy, m_lTimeIdle, m_szWriteStatusMsg, m_pStatusMsg, m_szHostname, m_lAddress, szProxy, lProxy, m_szLocation, m_szClient, m_szProtocol, 0, false);
   }

   // stats section
   if(mask(iLevel, USERITEMWRITE_DETAILS) == true)
   {
      STACKTRACEUPDATE

      WriteStats(pEDF, iLevel, "stats", m_pNumLogins.m_lValue, m_pTotalLogins.m_lValue, m_pLongestLogin.m_lValue, m_pNumMsgs.m_lValue, m_pTotalMsgs.m_lValue, m_pLastMsg.m_lValue, m_pNumVotes.m_lValue);
      WriteStats(pEDF, iLevel, "periodstats", m_pNumLogins.m_lPeriodValue, m_pTotalLogins.m_lPeriodValue, m_pLongestLogin.m_lPeriodValue, m_pNumMsgs.m_lPeriodValue, m_pTotalMsgs.m_lPeriodValue, m_pLastMsg.m_lPeriodValue, m_pNumVotes.m_lPeriodValue);

      // details section
      if(mask(iLevel, EDFITEMWRITE_ADMIN) == true)
      {
         // printf("UserItem::WriteFields writing details\n");

         WriteDetail(pEDF, iLevel, "email", &m_pEmail);
         WriteDetail(pEDF, iLevel, "homepage", &m_pHomepage);
         WriteDetail(pEDF, iLevel, "realname", &m_pRealName);
         WriteDetail(pEDF, iLevel, "sms", &m_pSMS);
         WriteDetail(pEDF, iLevel, "refer", &m_pRefer);
         if(mask(iLevel, EDFITEMWRITE_HIDDEN) == true)
         {
            WriteDetail(pEDF, iLevel, "picture", &m_pPicture);
         }
         WriteDetail(pEDF, iLevel, "pgpkey", &m_pPGPKey);

         // EDFPrint("UserItem::WriteFields details written", pEDF, false);
      }
   }

   /* if(mask(iLevel, USERWRITE_DETAILS) == true)
   {
      debug("UserItem::WriteFields marking %d\n", m_iMarking);
   } */
   if(mask(iLevel, EDFITEMWRITE_ADMIN) == true && mask(iLevel, USERITEMWRITE_DETAILS) == true)
   {
      if(pEDF->Child("folders") == false)
      {
         pEDF->Add("folders");
      }
      pEDF->SetChild("marking", m_iMarking);
      pEDF->SetChild("archiving", m_iArchiving);
      pEDF->Parent();
   }

   // printf("UserItem::WriteFields exit true\n");
   return true;
}

bool UserItem::WriteEDF(EDF *pEDF, int iLevel)
{
   if(iLevel == -1)
   {
      iLevel = EDFITEMWRITE_HIDDEN + EDFITEMWRITE_ADMIN + USERITEMWRITE_LOGIN + USERITEMWRITE_DETAILS + USERITEMWRITE_SELF;
   }

   if(mask(iLevel, USERITEMWRITE_SELF) == true)
   {
      // printf("UserItem::WriteEDF, %s", m_szClient);

      if(m_pEDF->Child("client", m_szWriteClient != NULL ? m_szWriteClient : m_szClient) == true)
      {
         // printf(" client");

         pEDF->Add("client", m_szWriteClient != NULL ? m_szWriteClient : m_szClient);
         pEDF->Copy(m_pEDF, false);
         pEDF->Parent();
      }
      m_pEDF->Parent();

      // printf("\n");
      // EDFPrint(m_pEDF);
   }
   if(mask(iLevel, EDFITEMWRITE_ADMIN) == true && mask(iLevel, USERITEMWRITE_DETAILS) == true)
   {
      if(m_pEDF->Child("agent") == true)
      {
         pEDF->Copy(m_pEDF);
         m_pEDF->Parent();
      }

      if(m_pEDF->Child("privilege") == true)
      {
         // EDFPrint("UserItem::WriteEDF privilege section", m_pEDF, EDFElement::EL_CURR | EDFElement::PR_SPACE);

         if(m_pEDF->Children() > 0)
         {
            pEDF->Copy(m_pEDF);
            // EDFPrint("UserItem::WriteEDF privilege section", pEDF, EDFElement::EL_CURR | EDFElement::PR_SPACE);
         }

         m_pEDF->Parent();
      }

      if(m_pEDF->Child("friends") == true)
      {
         // EDFPrint("UserItem::WriteEDF privilege section", m_pEDF, EDFElement::EL_CURR | EDFElement::PR_SPACE);

         if(m_pEDF->Children() > 0)
         {
            pEDF->Copy(m_pEDF);
            // EDFPrint("UserItem::WriteEDF privilege section", pEDF, EDFElement::EL_CURR | EDFElement::PR_SPACE);
         }

         m_pEDF->Parent();
      }

      if(m_pEDF->Child("rules") == true)
      {
         if(m_pEDF->Children() > 0)
         {
            if(pEDF->Child("folders") == false)
            {
               pEDF->Add("folders");
            }

            EDFParser::debugPrint("UserItem::WriteEDF rules section", m_pEDF, EDFElement::EL_CURR | EDFElement::PR_SPACE);
            pEDF->Copy(m_pEDF, false);

            pEDF->Parent();
         }

         m_pEDF->Parent();
      }
   }

   return true;
}

bool UserItem::WriteFields(DBTable *pTable, EDF *pEDF)
{
   STACKTRACE

   // printf("UserItem::WriteFields %p\n", m_pDescription);
   // bytesprint("UserItem::WriteFields description", m_pDescription);

   pTable->SetField(m_iType);
   pTable->SetField(m_szName);
   pTable->SetField(m_szPassword);
   pTable->SetField(m_iAccessLevel);
   pTable->SetField(m_szAccessName);
   pTable->SetField(m_iGender);

   pTable->SetField(m_pDescription);

   pTable->SetField(m_lCreated);
   pTable->SetField(m_lOwnerID);

   pTable->SetField(m_szClient);
   pTable->SetField(m_szHostname);
   pTable->SetField(m_lAddress);
   pTable->SetField(m_szLocation);
   pTable->SetField(m_szProtocol);
   pTable->SetField(m_bSecure);
   pTable->SetField(m_lTimeOn);
   pTable->SetField(m_lTimeOff);

   pTable->SetField(m_pNumLogins.m_lValue);
   pTable->SetField(m_pTotalLogins.m_lValue);
   pTable->SetField(m_pLongestLogin.m_lValue);
   pTable->SetField(m_pNumMsgs.m_lValue);
   pTable->SetField(m_pTotalMsgs.m_lValue);
   pTable->SetField(m_pLastMsg.m_lValue);
   pTable->SetField(m_pNumVotes.m_lValue);

   pTable->SetField(m_pNumLogins.m_lPeriodValue);
   pTable->SetField(m_pTotalLogins.m_lPeriodValue);
   pTable->SetField(m_pLongestLogin.m_lPeriodValue);
   pTable->SetField(m_pNumMsgs.m_lPeriodValue);
   pTable->SetField(m_pTotalMsgs.m_lPeriodValue);
   pTable->SetField(m_pLastMsg.m_lPeriodValue);
   pTable->SetField(m_pNumVotes.m_lPeriodValue);

   WriteDetail(pEDF, EDFITEMWRITE_ADMIN, "email", &m_pEmail);
   WriteDetail(pEDF, EDFITEMWRITE_ADMIN, "homepage", &m_pHomepage);
   WriteDetail(pEDF, EDFITEMWRITE_ADMIN, "realname", &m_pRealName);
   WriteDetail(pEDF, EDFITEMWRITE_ADMIN, "sms", &m_pSMS);
   WriteDetail(pEDF, EDFITEMWRITE_ADMIN, "refer", &m_pRefer);
   WriteDetail(pEDF, EDFITEMWRITE_ADMIN, "picture", &m_pPicture);
   WriteDetail(pEDF, EDFITEMWRITE_ADMIN, "pgpkey", &m_pPGPKey);

   if(pEDF->Child("folders") == false)
   {
      pEDF->Add("folders");
   }
   pEDF->SetChild("marking", m_iMarking);
   pEDF->SetChild("archiving", m_iArchiving);
   pEDF->Parent();

   return true;
}

bool UserItem::ReadFields(DBTable *pTable)
{
   STACKTRACE

   // usertype, name, password, accesslevel, accessname, gender
   pTable->BindColumnInt();
   pTable->BindColumnBytes();
   pTable->BindColumnBytes();
   pTable->BindColumnInt();
   pTable->BindColumnBytes();
   pTable->BindColumnInt();

   // description
   pTable->BindColumnBytes();

   // creatorid, ownerid
   pTable->BindColumnInt();
   pTable->BindColumnInt();

   // client, hostname, address, location, proxy, secure, timeon, timeoff
   pTable->BindColumnBytes();
   pTable->BindColumnBytes();
   pTable->BindColumnInt();
   pTable->BindColumnBytes();
   pTable->BindColumnBytes();
   pTable->BindColumnInt();
   pTable->BindColumnInt();
   pTable->BindColumnInt();

   // numlogins, totallogins, longestlogin, nummsgs, totalmsgs, lastmsg, numvotes
   pTable->BindColumnInt();
   pTable->BindColumnInt();
   pTable->BindColumnInt();
   pTable->BindColumnInt();
   pTable->BindColumnInt();
   pTable->BindColumnInt();
   pTable->BindColumnInt();

   // numlogins, totallogins, longestlogin, nummsgs, totalmsgs, lastmsg, numvotes
   pTable->BindColumnInt();
   pTable->BindColumnInt();
   pTable->BindColumnInt();
   pTable->BindColumnInt();
   pTable->BindColumnInt();
   pTable->BindColumnInt();
   pTable->BindColumnInt();

   return true;
}

char *UserItem::TableName()
{
   return UI_TABLENAME;
}

char *UserItem::KeyName()
{
   return "userid";
}

char *UserItem::WriteName()
{
   return "user";
}

void UserItem::init()
{
   // Top level fields
   m_iType = 0;
   m_szName = NULL;
   m_szPassword = NULL;
   m_iAccessLevel = LEVEL_NONE;
   m_szAccessName = NULL;
   m_iGender = 0;

   m_pDescription = NULL;

   m_lCreated = time(NULL);
   m_lOwnerID = -1;

   // login section
   m_szClient = NULL;
   m_szHostname = NULL;
   m_lAddress = 0;
   m_szLocation = NULL;
   m_szProtocol = NULL;
   m_iStatus = LOGIN_OFF;
   m_pStatusMsg = NULL;
   m_lTimeBusy = -1;
   m_lTimeIdle = -1;
   m_lTimeOff = -1;
   m_lTimeOn = -1;

   // stats section
   InitStat(&m_pNumLogins, 0);
   InitStat(&m_pTotalLogins, 0);
   InitStat(&m_pLongestLogin, 0);
   InitStat(&m_pNumMsgs, 0);
   InitStat(&m_pTotalMsgs, 0);
   InitStat(&m_pLastMsg, -1);
   InitStat(&m_pNumVotes, 0);

   // details section
   InitDetail(&m_pEmail);
   InitDetail(&m_pHomepage);
   InitDetail(&m_pRealName);
   InitDetail(&m_pSMS);
   InitDetail(&m_pRefer);
   InitDetail(&m_pPicture);
   InitDetail(&m_pPGPKey);

   m_iMarking = 0;
   m_iArchiving = 0;

   m_szWriteStatusMsg = NULL;
   m_szWriteClient = NULL;
}

void UserItem::setup()
{
   bool bLoop = false;
   int iPos = 0;
   char *szName = NULL;
   EDF *pRules = NULL;

   // debugEDFPrint("UserItem::setup", m_pEDF);

   // details section
   while(m_pEDF->Child("details") == true)
   {
      SetDetail(m_pEDF, "email");
      SetDetail(m_pEDF, "homepage");
      SetDetail(m_pEDF, "realname");
      SetDetail(m_pEDF, "sms");
      SetDetail(m_pEDF, "refer");
      SetDetail(m_pEDF, "picture");
      SetDetail(m_pEDF, "pgpkey");

      // printf("UserItem::setup details %p %p %p %p %p %p %p\n", m_pEmail, m_pHomepage, m_pRealName, m_pSMS, m_pRefer, m_pPicture, m_pPGPKey);

      m_pEDF->Delete();
   }

   // Old style data fixing

   if(m_pEDF->Child("folders") == true)
   {
      // EDFParser::debugPrint("UserItem::setup", m_pEDF, EDFElement::EL_CURR | EDFElement::PR_SPACE);

      ExtractMember(m_pEDF, "marking", &m_iMarking, 0);
      ExtractMember(m_pEDF, "archiving", &m_iArchiving, -1);
      if(m_iArchiving == -1)
      {
         m_iArchiving = ARCHIVE_PUBLIC;
         debug("UserItem::setup default archiving on %d to %d\n", GetID(), m_iArchiving);
      }

      m_pEDF->Delete();
      while(m_pEDF->DeleteChild("folders") == true)
      {
         SetChanged(true);
      }
   }

   bLoop = m_pEDF->Child("client");
   while(bLoop == true)
   {
      while(m_pEDF->DeleteChild("hostnameproxy") == true);

      bLoop = m_pEDF->Next("client");
      if(bLoop == false)
      {
         m_pEDF->Parent();
      }
   }

   bLoop = m_pEDF->Child("agent");
   while(bLoop == true)
   {
      while(m_pEDF->DeleteChild("hostnameproxy") == true);

      bLoop = m_pEDF->Next("agent");
      if(bLoop == false)
      {
         m_pEDF->Parent();
      }
   }

   if(m_pEDF->Child("proxy") == true)
   {
      m_pEDF->Set("privilege");
      m_pEDF->Parent();

      SetChanged(true);
   }

   // Old style data fixing
   if(m_pEDF->GetChildBool("deleted") == true)
   {
      m_pEDF->DeleteChild("deleted");
      SetDeleted(true);
   }

   if(mask(m_iType, USERTYPE_AGENT) == true && m_szAccessName != NULL && stricmp(m_szAccessName, "agent") == 0)
   {
      SetUserType(USERTYPE_AGENT);
   }

   bLoop = m_pEDF->Child();
   while(bLoop == true)
   {
      m_pEDF->Get(&szName);
      if(stricmp(szName, "rules") == 0)
      {
         EDFParser::debugPrint("UserItem::setup rules element", m_pEDF, EDFElement::PR_SPACE);
         if(m_pEDF->Children("rule") > 0)
         {
            if(pRules == NULL)
            {
               pRules = new EDF();
            }
            pRules->Copy(m_pEDF, false);
         }

         iPos = m_pEDF->Position();
         debug("UserItem::setup deleting %d\n", iPos);
         m_pEDF->Delete();
         if(m_pEDF->Child(NULL, iPos) == false)
         {
            bLoop = m_pEDF->Iterate();
         }

         SetChanged(true);
      }
      else
      {
         bLoop = m_pEDF->Iterate();
      }

      delete[] szName;
   }

   if(pRules != NULL)
   {
      EDFParser::debugPrint("UserItem::setup new rules", pRules);

      m_pEDF->Add("rules");
      m_pEDF->Copy(pRules, false);

      delete pRules;

      EDFParser::debugPrint("UserItem::setup EDF section", m_pEDF);
   }
}

void UserItem::InitDetail(detail_item *pItem)
{
   pItem->m_szContentType = NULL;
   pItem->m_pData = NULL;
   pItem->m_iType = 0;
}

UserItem::detail_item *UserItem::MapDetail(char *szName)
{
   detail_item *pReturn = NULL;

   if(stricmp(szName, USERITEMDETAIL_EMAIL) == 0)
   {
      pReturn = &m_pEmail;
   }
   else if(stricmp(szName, USERITEMDETAIL_HOMEPAGE) == 0)
   {
      pReturn = &m_pHomepage;
   }
   else if(stricmp(szName, USERITEMDETAIL_REALNAME) == 0)
   {
      pReturn = &m_pRealName;
   }
   else if(stricmp(szName, USERITEMDETAIL_SMS) == 0)
   {
      pReturn = &m_pSMS;
   }
   else if(stricmp(szName, USERITEMDETAIL_REFER) == 0)
   {
      pReturn = &m_pRefer;
   }
   else if(stricmp(szName, USERITEMDETAIL_PICTURE) == 0)
   {
      pReturn = &m_pPicture;
   }
   else if(stricmp(szName, USERITEMDETAIL_PGPKEY) == 0)
   {
      pReturn = &m_pPGPKey;
   }

   return pReturn;
}

bool UserItem::SetDetail(EDF *pEDF, char *szName)
{
   STACKTRACE
   int iType = 0;
   char *szContentType = NULL;
   bytes *pData = NULL;
   // detail_item *pItem = NULL;

   // EDFParser::debugPrint("UserItem::SetDetail entry", pEDF, EDFElement::EL_CURR);
   debug("UserItem::SetDetail entry %p %d\n", pEDF, szName);

   /* pItem = MapDetail(szName);
   if(pItem == NULL)
   {
      // EDFPrint("UserItem::SetDetail exit false", pEDF, EDFElement::EL_CURR);
      debug("UserItem::SetDetail exit false(1)\n");
      return false;
   } */

   // printf("UserItem::SetDetail %p %s\n", pEDF, szName);

   if(pEDF->Child(szName) == false)
   {
      // EDFPrint("UserItem::SetDetail exit false", pEDF, EDFElement::EL_CURR);
      debug("UserItem::SetDetail exit false(2)\n");
      return false;
   }

   EDFParser::debugPrint("UserItem::SetDetail input", pEDF, EDFElement::EL_CURR | EDFElement::PR_SPACE);

   if(pEDF->GetChild("content-type", &szContentType) == true)
   {
      pEDF->GetChild("data", &pData);
   }
   else
   {
      pEDF->Get(NULL, &pData);
   }

   // if(pData != NULL)
   {
      pEDF->GetChild("type", &iType);
      if(pEDF->GetChildBool("valid") == true)
      {
         iType |= DETAIL_VALID;
      }
      if(pEDF->GetChildBool("public") == true)
      {
         iType |= DETAIL_PUBLIC;
      }

      SetDetail(szName, szContentType, pData, iType);
   }

   if(pEDF->DataCopy() == true)
   {
      delete[] szContentType;
      delete pData;
   }

   pEDF->Parent();

   // EDFPrint("UserItem::SetDetail exit true", pEDF, EDFElement::EL_CURR);
   debug("UserItem::SetDetail exit true\n");
   return true;
}

void UserItem::DeleteDetail(detail_item *pItem)
{
   STACKTRACE

   STR_NULL(pItem->m_szContentType);
   delete pItem->m_pData;
   pItem->m_pData = NULL;
}

bool UserItem::WriteDetail(EDF *pEDF, int iLevel, char *szName, detail_item *pItem)
{
   STACKTRACE

   // printf("UserItem::WriteDetail %p %s %p('%s' %d) %d\n", pEDF, szName, pItem, pItem != NULL ? pItem->m_szValue : NULL, pItem != NULL ? pItem->m_iType : -1, iLevel);
   // printf("UserItem::WriteDetail %s %p\n", szName, pItem);

   if((mask(iLevel, EDFITEMWRITE_ADMIN) == true || mask(pItem->m_iType, DETAIL_PUBLIC) == true) && pItem != NULL && pItem->m_pData != NULL)
   {
      if(pEDF->Child("details") == false)
      {
         pEDF->Add("details");
      }

      /* if(pItem->m_iType > 0)
      {
         pEDF->AddChild("type", pItem->m_iType);
      } */
      if(pItem->m_szContentType != NULL)
      {
         bytesprint(debugfile(), "UserItem::WriteDetail data", pItem->m_pData, true);

         pEDF->Add(szName);
         pEDF->AddChild("content-type", pItem->m_szContentType);
         pEDF->AddChild("data", pItem->m_pData);
         if(pItem->m_iType > 0)
         {
            pEDF->AddChild("type", pItem->m_iType);
         }
         pEDF->Parent();
      }
      else
      {
         pEDF->Add(szName, pItem->m_pData);
         if(pItem->m_iType > 0)
         {
            pEDF->AddChild("type", pItem->m_iType);
         }
         pEDF->Parent();
      }
      /* if(mask(pItem->m_iType, DETAIL_PUBLIC) == true)
      {
         pEDF->AddChild("public", true);
      }
      if(mask(pItem->m_iType, DETAIL_VALID) == true)
      {
         pEDF->AddChild("valid", true);
      } */
      // EDFPrint("UserItem::WriteDetail EDF", pEDF, false);

      pEDF->Parent();

      return true;
   }

   return false;
}

void UserItem::InitStat(stat_item *pItem, long lValue)
{
   pItem->m_lValue = lValue;
   pItem->m_lPeriodValue = lValue;
}

UserItem::stat_item *UserItem::MapStat(char *szName)
{
   stat_item *pReturn = NULL;

   if(stricmp(szName, USERITEMSTAT_NUMLOGINS) == 0)
   {
      pReturn = &m_pNumLogins;
   }
   else if(stricmp(szName, USERITEMSTAT_TOTALLOGINS) == 0)
   {
      pReturn = &m_pTotalLogins;
   }
   else if(stricmp(szName, USERITEMSTAT_LONGESTLOGIN) == 0)
   {
      pReturn = &m_pLongestLogin;
   }
   else if(stricmp(szName, USERITEMSTAT_NUMMSGS) == 0)
   {
      pReturn = &m_pNumMsgs;
   }
   else if(stricmp(szName, USERITEMSTAT_TOTALMSGS) == 0)
   {
      pReturn = &m_pTotalMsgs;
   }
   else if(stricmp(szName, USERITEMSTAT_LASTMSG) == 0)
   {
      pReturn = &m_pLastMsg;
   }
   else if(stricmp(szName, USERITEMSTAT_NUMVOTES) == 0)
   {
      pReturn = &m_pNumVotes;
   }

   return pReturn;
}

bool UserItem::WriteStats(EDF *pEDF, int iLevel, char *szName, long lNumLogins, long lTotalLogins, long lLongestLogin, long lNumMsgs, long lTotalMsgs, long lLastMsg, long lNumVotes)
{
   if(mask(iLevel, EDFITEMWRITE_ADMIN) == true && (lNumLogins > 0 || lNumMsgs > 0 || lNumVotes > 0))
   {
      pEDF->Add(szName);

      if(lNumLogins > 0)
      {
         pEDF->AddChild("numlogins", lNumLogins);
         pEDF->AddChild("totallogins", lTotalLogins);
      }
      if(lLongestLogin != -1)
      {
         pEDF->AddChild("longestlogin", lLongestLogin);
      }
      if(lNumMsgs > 0)
      {
         pEDF->AddChild("nummsgs", lNumMsgs);
         pEDF->AddChild("totalmsgs", lTotalMsgs);
      }
      if(lLastMsg != -1)
      {
         pEDF->AddChild("lastmsg", lLastMsg);
      }
      if(lNumVotes > 0)
      {
         pEDF->AddChild("numvotes", lNumVotes);
      }

      pEDF->Parent();
   }

   return true;
}
