/*
** EDFItem library
** (c) 2000 Michael Wood (mike@compsoc.man.ac.uk)
**
** MessageItem.h: Definition of MessageItem class
**
** Definition of MessageItem class (derived from TreeNodeItem)
*/

#ifndef _MESSAGEITEM_H_
#define _MESSAGEITEM_H_

#include "TreeNodeItem.h"

#define MESSAGEITEMWRITE_DETAILS 8
#define MESSAGEITEMWRITE_TEXT 16
#define MESSAGEITEMWRITE_CACHE 32
#define MESSAGEITEMWRITE_SUBJECT 64
#define MESSAGEITEMWRITE_ATTDATA 128

class MessageTreeItem;

class MessageItem : public TreeNodeItem<MessageItem *>
{
public:
	MessageItem(long lID, MessageItem *pParent, MessageTreeItem *pTree);
	MessageItem(EDF *pEDF, MessageItem *pParent, MessageTreeItem *pTree);
	MessageItem(DBTable *pTable, MessageItem *pParent, MessageTreeItem *pTree, int iEDFField);
	~MessageItem();

   MessageTreeItem *GetTree();
   bool SetTree(MessageTreeItem *pTree);
   long GetTreeID();
   bool SetTreeID(long lTreeID);

   int GetType();
   bool SetType(int iType);
   long GetDate();
   long GetFromID();
   bool SetFromID(long lFromID);
   bytes *GetFromName(bool bCopy = false);
   bool SetFromName(bytes *pFromName);
   long GetToID();
   bool SetToID(long lToID);
   bytes *GetToName(bool bCopy = false);
   bool SetToName(bytes *pToName);

   bytes *GetText(bool bCopy = false);
   bool SetText(bytes *pText);
   bool AddText(bytes *pText);

   bool AddDelete(long lUserID, long lDate = -1);
   bool AddEdit(long lUserID, long lDate = -1);

	bool AddAttachment(char *szURL, long lFromID = -1, long lDate = -1);
   bool AddAttachment(char *szContentType, char *szFilename, bytes *pData, EDF *pEDF, long lFromID = -1, long lDate = -1);
   bool DeleteAttachment(int iAttachmentID, long lUserID, long lDate = -1);
   bool WriteAttachment(EDF *pEDF, int iAttachmentID, int iLevel);
   long AttachmentID(int iAttachmentNum);
   long AttachmentSize(int iAttachmentNum);
   int AttachmentCount();
   
   int MessagePos(long lID, bool *bFound);

   long GetMaxID();

protected:
   virtual bool IsEDFField(char *szName);
   virtual bool WriteFields(EDF *pEDF, int iLevel);
   virtual bool WriteEDF(EDF *pEDF, int iLevel);
   virtual bool WriteFields(DBTable *pTable, EDF *pEDF);
   virtual bool ReadFields(DBTable *pTable);

   void WriteDetail(EDF *pEDF, char *szName, int iLevel);

   virtual char *TableName() = 0;
   virtual char *KeyName();
   virtual char *WriteName();
   virtual char *TreeName() = 0;

   bool AddDetail(char *szName, long lUserID, long lDate = -1, bool bParent = true);
   bool AddAttachment(bool bParent, char *szURL, char *szContentType, char *szFilename, bytes *pData, EDF *pEDF, long lFromID = -1, long lDate = -1);

private:
	void init(MessageTreeItem *pTree);
	void setup();

   MessageTreeItem *m_pTree;
   long m_lTreeID;

   int m_iType;
	long m_lDate;
	long m_lFromID;
	long m_lToID;
   bytes *m_pToName;
	bytes *m_pText;

   void WriteEDFAttachment(EDF *pEDF, int iLevel);
};

#endif
