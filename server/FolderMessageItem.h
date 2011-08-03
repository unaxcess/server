/*

** EDFItem library

** (c) 2000 Michael Wood (mike@compsoc.man.ac.uk)

**

** FolderMessageItem.h: Definition of FolderMessageItem class

**

** Definition of FolderMessageItem class (derived from MessageItem)

*/



#ifndef _FOLDERMESSAGEITEM_H_

#define _FOLDERMESSAGEITEM_H_



#include "MessageItem.h"



#define FOLDERMESSAGEITEMWRITE_SELF 256



#define FMI_TABLENAME "folder_message_item"



class FolderMessageItem : public MessageItem

{

public:

   FolderMessageItem(long lID, FolderMessageItem *pParent, MessageTreeItem *pTree);

   FolderMessageItem(EDF *pEDF, FolderMessageItem *pParent, MessageTreeItem *pTree);

   FolderMessageItem(DBTable *pTable, FolderMessageItem *pParent, MessageTreeItem *pTree);

   ~FolderMessageItem();



   // const char *GetClass();



   // long GetTopID();

   // bool SetTopID(long lTopID);



   bytes *GetSubject(bool bCopy = false);

   bool SetSubject(bytes *pSubject);



   int GetVoteType();

   // bool SetVoteType(int iVoteType, int iMinValue = -1, int iMaxValue = -1);

   bool SetVoteType(int iVoteType);

   bool SetVoteMinValue(int iValue);

   bool SetVoteMaxValue(int iValue);

   bool SetVoteMinValue(double dValue);

   bool SetVoteMaxValue(double Value);

   bool AddVote(bytes *pText);

   int GetVoteUser(long lUserID);

   bool SetVoteUser(long lUserID, int iVoteID, char *szVoteValue, int iVoteValue, double dVoteValue, int iVoteType);

   long GetVoteClose();

   bool SetVoteClose(long lTime);



   bool AddMove(long lUserID, long lDate = -1);



   bool AddAnnotation(bytes *pText, long lFromID, long lDate = -1);

   bool IsAnnotation(int iFromID);



   bool AddLink(bytes *pText, long lMessageID, long lFromID = -1, long lDate = -1);



   // MessageTreeItem *GetReply();

   // bool SetReply(MessageTreeItem *pReply);

   long GetReplyID();

   bool SetReplyID(long lReplyID);



   long GetExpire();

   bool SetExpire(long lTime);

	long GetThreadID();

	bool SetThreadID(long lThreadID);

   static FolderMessageItem *NextRow(DBTable *pTable, FolderMessageItem *pParent, MessageTreeItem *pTree);



protected:

   bool IsEDFField(char *szName);

   bool WriteFields(EDF *pEDF, int iLevel);

   bool WriteEDF(EDF *pEDF, int iLevel);

   bool WriteFields(DBTable *pTable, EDF *pEDF);

   bool ReadFields(DBTable *pTable);



   char *TableName();

   char *TreeName();



private:

   void init(MessageTreeItem *pTree);

   void setup();



   bool SetVoteValue(const char *szField, int iVoteValue, int iValue, double dValue);



   // long m_lTopID;

   bytes *m_pSubject;



   // MessageTreeItem *m_pReply;

   long m_lReplyID;



   long m_lExpire;



   static FolderMessageItem *m_pInit;

};



#endif

