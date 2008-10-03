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
** UserItem.h: Definition of UserItem class (derived from EDFItem)
*/

#ifndef _USERITEM_H_
#define _USERITEM_H_

#include "EDFItem.h"

#define UI_TABLENAME "user_item"

// Write method level masks
#define USERITEMWRITE_LOGIN 8
#define USERITEMWRITE_DETAILS 16
#define USERITEMWRITE_SELF 32
// #define USERITEMWRITE_FLAT 32
#define USERITEMWRITE_LOGINFLAT 64

#define USERITEMSTAT_NUMLOGINS "numlogins"
#define USERITEMSTAT_TOTALLOGINS "totallogins"
#define USERITEMSTAT_LONGESTLOGIN "longestlogin"
#define USERITEMSTAT_NUMMSGS "nummsgs"
#define USERITEMSTAT_TOTALMSGS "totalmsgs"
#define USERITEMSTAT_LASTMSG "lastmsg"
#define USERITEMSTAT_NUMVOTES "numvotes"

#define USERITEMDETAIL_EMAIL "email"
#define USERITEMDETAIL_HOMEPAGE "homepage"
#define USERITEMDETAIL_REALNAME "realname"
#define USERITEMDETAIL_SMS "sms"
#define USERITEMDETAIL_REFER "refer"
#define USERITEMDETAIL_PICTURE "picture"
#define USERITEMDETAIL_PGPKEY "pgpkey"

class UserItem : public EDFItem
{
public:
   UserItem(long lID);
   UserItem(EDF *pEDF);
   UserItem(DBTable *pTable);
   ~UserItem();

   // const char *GetClass();

   int GetUserType();
   bool SetUserType(int iUserType);

   char *GetName(bool bCopy = false, long lDate = -1);
   bool SetName(char *szName);
   char *GetPassword(bool bCopy = false);
   bool SetPassword(char *szPassword);
   int GetAccessLevel();
   bool SetAccessLevel(int iAccessLevel);
   bool SetAccessLevel(EDF *pEDF);
   char *GetAccessName(bool bCopy = false);
   bool SetAccessName(char *szAccessName);
   bool SetAccessName(EDF *pEDF);
   int GetGender();
   bool SetGender(EDF *pIn);

   bytes *GetDescription(bool bCopy = false);
   bool SetDescription(bytes *pDescription);
   bool SetDescription(EDF *pEDF);

   long GetCreated();
   bool SetCreated(long lCreated);
   bool SetCreated(EDF *pEDF);
   long GetOwnerID();
   bool SetOwnerID(long lOwnerID);

   // bool SetAddress(EDF *pEDF);
   bool SetAddress(unsigned long lAddress);
   bool SetAddressProxy(unsigned long lAddressProxy);
   EDF *GetAllowDeny(bool bCreate = false);
   char *GetClient(bool bCopy = false);
   bool SetClient(char *szClient);
   bool SetHostname(char *szHostname);
   bool SetHostname(EDF *pEDF);
   bool SetHostnameProxy(char *szHostnameProxy);
   bool SetLocation(char *szLocation);
   bool SetLocation(EDF *pEDF);
   bool SetProtocol(char *szProtocol);
   bool SetSecure(bool bSecure);
   int GetStatus();
   bool SetStatus(int iStatus);
   bytes *GetStatusMsg(bool bCopy = false);
   bool SetStatusMsg(bytes *pStatusMsg);
   // bool SetStatusMsg(EDF *pEDF);
   bool SetTimeBusy(long lTimeBusy);
   bool SetTimeIdle(long lTimeIdle);
   long GetTimeOn();
   bool SetTimeOn(long lTimeOn);
   long GetTimeOff();
   bool SetTimeOff(long lTimeOff);
   EDF *GetLogin(bool bCreate = false);

   long GetStat(char *szName, bool bPeriod);// = false);
   bool SetStat(char *szName, long lValue);// = false);
   bool SetStat(char *szName, EDF *pEDF);// = false);
   bool SetStat(char *szName, long lValue, bool bPeriod);// = false);
   bool SetStat(char *szName, EDF *pEDF, bool bPeriod);// = false);
   bool IncStat(char *szName, long lValue);

   bool GetDetail(char *szName, char **szContentType, bytes **pData, int *iType = NULL);
   bool SetDetail(char *szName, char *szContentType, bytes *pData, int iType);
   bool DeleteDetail(char *szName);

   EDF *GetClientEDF(bool bCreate = false);

   EDF *GetAgent(bool bCreate = false);
   bool SetAgentData(EDF *pEDF);
   char *GetAgentName(bool bCopy = false);
   bool SetAgentName(char *szAgentName);

   EDF *GetPrivilege(bool bCreate = false);

   int GetMarking();
   bool SetMarking(int iMarking);
	int GetArchiving();
	bool SetArchiving(int iArchiving);

   EDF *GetRules(bool bCreate = false);
   int AddRule(EDF *pRule);
   bool DeleteRule(int iRuleID);

   EDF *GetFriends(bool bCreate = false);

   static bool IsIdle(long lTimeIdle);
   static bool WriteLogin(EDF *pEDF, int iLevel, int iID, int iStatus, bool bSecure, int iRequests, int iAnnounces, long lTimeOn, long lTimeOff, long lTimeBusy, long lTimeIdle, const char *szStatusMsg, bytes *pStatusMsg, const char *szHostname, unsigned long lAddress, const char *szProxy, unsigned long lProxy, const char *szLocation, const char *szClient, const char *szProtocol, long lAttachmentSize, bool bCurrent);

   static bool SetSystemTimeIdle(long lTimeIdle);

   static UserItem *NextRow(DBTable *pTable);

   // EDF output method
   bool Write(EDF *pEDF, int iLevel, const char *szStatusMsg, const char *szClient);

protected:
   bool IsEDFField(char *szName);
   bool WriteFields(EDF *pEDF, int iLevel);
   bool WriteEDF(EDF *pEDF, int iLevel);
   bool WriteFields(DBTable *pTable, EDF *pEDF);
   bool ReadFields(DBTable *pTable);

   char *TableName();
   char *KeyName();
   char *WriteName();

private:
   struct detail_item
   {
      char *m_szContentType;
      bytes *m_pData;
      int m_iType;
   };

   struct stat_item
   {
      long m_lValue;
      long m_lPeriodValue;
   };

   // Top level fields
   int m_iType;
   char *m_szName;
   char *m_szPassword;
   int m_iAccessLevel;
   char *m_szAccessName;
   int m_iGender;

   bytes *m_pDescription;

   long m_lCreated;
   long m_lCreatorID;
   long m_lOwnerID;

   // login section
   // char *m_szBusyMsg;
   char *m_szClient;
   char *m_szHostname;
   unsigned long m_lAddress;
   char *m_szLocation;
   char *m_szProtocol;
   bool m_bSecure;
   int m_iStatus;
   bytes *m_pStatusMsg;
   long m_lTimeBusy;
   long m_lTimeIdle;
   long m_lTimeOff;
   long m_lTimeOn;

   // stats section
   stat_item m_pNumLogins;
   stat_item m_pTotalLogins;
   stat_item m_pLongestLogin;
   stat_item m_pNumMsgs;
   stat_item m_pTotalMsgs;
   stat_item m_pLastMsg;
   stat_item m_pNumVotes;

   // details section
   detail_item m_pEmail;
   detail_item m_pHomepage;
   detail_item m_pRealName;
   detail_item m_pSMS;
   detail_item m_pRefer;
   detail_item m_pPicture;
   detail_item m_pPGPKey;

   int m_iMarking;
	int m_iArchiving;

   char *m_szWriteStatusMsg;
   char *m_szWriteClient;

   static long m_lSystemTimeIdle;
   static UserItem *m_pInit;

   void init();
   void setup();

   void InitDetail(detail_item *pItem);
   detail_item *MapDetail(char *szName);
   bool SetDetail(EDF *pEDF, char *szName);
   void DeleteDetail(detail_item *pItem);
   bool WriteDetail(EDF *pEDF, int iLevel, char *szName, detail_item *pDetail);
   /* bool MapDetail(char *szName, detail_item **pDetail, bool bSet);
   bool ExtractDetail(EDF *pEDF, char *szName, detail_item **pDetail);
   bool DeleteDetail(detail_item *pDetail); */

   void InitStat(stat_item *pItem, long lValue);
   stat_item *MapStat(char *szName);
   bool WriteStats(EDF *pEDF, int iLevel, char *szName, long lNumLogins, long lTotalLogins, long lLongestLogin, long lNumMsgs, long lTotalMsgs, long lLastMsg, long lNumVotes);
};

#endif
