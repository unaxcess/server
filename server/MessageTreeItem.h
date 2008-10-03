/*
** EDFItem library
** (c) 2000 Michael Wood (mike@compsoc.man.ac.uk)
**
** MessageTreeItem.h: Definition of MessageTreeItem class
**
** Definition of MessageTreeItem class (derived from TreeNodeItem)
*/

#ifndef _MESSAGETREEITEM_H_
#define _MESSAGETREEITEM_H_

#include "TreeNodeItem.h"
#include "MessageItem.h"
// #include "UserItem.h"

#define MESSAGETREEITEMWRITE_DETAILS 8
#define MESSAGETREEITEMWRITE_INFO 16

class MessageTreeItem : public TreeNodeItem<MessageTreeItem *>
{
public:
	MessageTreeItem(char *szType, long lID, MessageTreeItem *pParent, bool bMessageDelete);
	MessageTreeItem(char *szType, EDF *pEDF, MessageTreeItem *pParent, bool bMessageDelete);
	MessageTreeItem(char *szType, DBTable *pTable, MessageTreeItem *pParent, bool bMessageDelete);
	~MessageTreeItem();

   // const char *GetClass();

   // char *GetMsgType(bool bCopy = false);

	char *GetName(bool bCopy = false);
   bool SetName(char *szName);

   int GetAccessMode();
   bool SetAccessMode(int iAccessMode);
   int GetAccessLevel();
   bool SetAccessLevel(int iAccessLevel);
   long GetCreated();
   long GetExpire();
   bool SetExpire(long lExpire);
   long GetNumMsgs();
   long GetTotalMsgs();

   // MessageTreeItem *GetReply();
   // bool SetReply(MessageTreeItem *pReply);
   long GetReplyID();
   bool SetReplyID(long lReplyID);

   // UserItem *GetProvider();
   // bool SetProvider(UserItem *pProvider);
   long GetProviderID();
   bool SetProviderID(long lProviderID);

   long GetInfoID();
   bool SetInfo(long lInfoID, bytes *pInfo);

	bool MessageAdd(MessageItem *pMessage);
   bool MessageDelete(long lMessageID);
	int MessageCount(bool bRecurse = true);
   int MessagePos(long lMessageID);
   MessageItem *MessageFind(long lID, bool bRecurse = true);
	MessageItem *MessageChild(int iChildNum);

   long GetMaxID();

	static MessageTreeItem *NextRow(char *szType, DBTable *pTable, MessageTreeItem *pParent, bool bMessageDelete);

protected:
   bool IsEDFField(char *szName);
   bool WriteFields(EDF *pEDF, int iLevel);
   bool WriteEDF(EDF *pEDF, int iLevel);
   bool WriteFields(DBTable *pTable, EDF *pEDF);
   bool ReadFields(DBTable *pTable);

   char *TableName();
   char *KeyName();
   char *WriteName();

   // void WritePath(EDF *pEDF);

private:
	void init(char *szType, bool bMessageDelete);
	void setup();

   char *m_szType;
   char *m_szTypeTable;
   char *m_szTypeKey;
   int m_iType;
   bool m_bMessageDelete;

	char *m_szName;
	int m_iAccessMode;
	int m_iAccessLevel;
	long m_lCreated;
	long m_lExpire;
	long m_lLastMsg;
	long m_lLastEdit;
   long m_lLastEditID;
	long m_lCreatorID;
   // MessageTreeItem *m_pReply;
	long m_lReplyID;
	long m_iMaxReplies;
   // UserItem *m_pProvider;
   long m_lProviderID;

	long m_lInfoDate;
	long m_lInfoID;
	bytes *m_pInfo;

	EDFItemList *m_pMessages;
   static MessageTreeItem *m_pInit;

   bool MessageStats(long *lNumMsgs, long *lTotalMsgs);
   void MessageStat(MessageItem *pMessage, long *lNumMsgs, long *lTotalMsgs);
};

#endif
