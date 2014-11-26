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
** MessageTreeItem.cpp: Implmentation of MessageTreeItem class
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

#include "MessageTreeItem.h"

// #define MESSAGETREEWRITE_HIDDEN 1

MessageTreeItem *MessageTreeItem::m_pInit = NULL;

// Constructor with no data
MessageTreeItem::MessageTreeItem(char *szType, long lID, MessageTreeItem *pParent, bool bMessageDelete) : TreeNodeItem<MessageTreeItem *>(lID, pParent)
{
   STACKTRACE

   init(szType, bMessageDelete);
}

// Constructor with EDF data
MessageTreeItem::MessageTreeItem(char *szType, EDF *pEDF, MessageTreeItem *pParent, bool bMessageDelete) : TreeNodeItem<MessageTreeItem *>(pEDF, pParent)
{
   STACKTRACE

   init(szType, bMessageDelete);

   // Top level fields
   ExtractMember(pEDF, "name", &m_szName);
   ExtractMember(pEDF, "accessmode", &m_iAccessMode, 0);
   ExtractMember(pEDF, "accesslevel", &m_iAccessLevel, -1);
   ExtractMember(pEDF, "created", &m_lCreated, -1);
   ExtractMember(pEDF, "expire", &m_lExpire, -1);
   ExtractMember(pEDF, "lastmsg", &m_lLastMsg, -1);
   // ExtractMember(pEDF, "lastedit", &m_lLastEdit, -1);
   if(pEDF->Child("lastedit") == true)
   {
      pEDF->Get(NULL, &m_lLastEdit);
      if(m_lLastEdit != -1)
      {
         ExtractMember(pEDF, "userid", &m_lLastEditID, -1);
      }
      pEDF->Parent();
   }
   ExtractMember(pEDF, "creatorid", &m_lCreatorID, -1);
   ExtractMember(pEDF, "replyid", &m_lReplyID, -1);
   ExtractMember(pEDF, "providerid", &m_lProviderID, -1);
   ExtractMember(pEDF, "maxreplies", &m_iMaxReplies, -1);

   EDFFields(pEDF);

   setup();
}

MessageTreeItem::MessageTreeItem(char *szType, DBTable *pTable, MessageTreeItem *pParent, bool bMessageDelete) : TreeNodeItem<MessageTreeItem *>(pTable, pParent, 16)
{
   STACKTRACE

   init(szType, bMessageDelete);

   GET_STR_TABLE(2, m_szName, false)
   GET_INT_TABLE(3, m_iAccessMode, 0)
   GET_INT_TABLE(4, m_iAccessLevel, -1)
   GET_INT_TABLE(5, m_lCreated, -1)
   GET_INT_TABLE(6, m_lExpire, -1)
   GET_INT_TABLE(7, m_lLastMsg, -1)
   GET_INT_TABLE(8, m_lLastEdit, -1)
   GET_INT_TABLE(9, m_lLastEditID, -1)
   GET_INT_TABLE(10, m_lCreatorID, -1)
   GET_INT_TABLE(11, m_lReplyID, -1)
   GET_INT_TABLE(12, m_iMaxReplies, -1)

   GET_INT_TABLE(13, m_lInfoDate, -1)
   GET_INT_TABLE(14, m_lInfoID, -1)
   GET_BYTES_TABLE(15, m_pInfo, true)

   EDFFields(NULL);

   setup();
}

// Destructor
MessageTreeItem::~MessageTreeItem()
{
   STACKTRACE
   // printf("MessageTreeItem::~MessageTreeItem %p, %ld %s\n", this, GetID(), m_szName);

   delete[] m_szType;
   delete[] m_szTypeTable;
   delete[] m_szTypeKey;

   delete[] m_szName;

   delete m_pInfo;

   delete m_pMessages;
}

/* const char *MessageTreeItem::GetClass()
{
   return typeid(this).name();
} */

bool MessageTreeItem::SetName(char *szName)
{
   SET_STR(m_szName, szName, UA_NAME_LEN)
}

char *MessageTreeItem::GetName(bool bCopy)
{
   GET_STR(m_szName)
}

int MessageTreeItem::GetAccessMode()
{
   return m_iAccessMode;
}

bool MessageTreeItem::SetAccessMode(int iAccessMode)
{
   SET_INT(m_iAccessMode, iAccessMode)
}

int MessageTreeItem::GetAccessLevel()
{
   return m_iAccessLevel;
}

bool MessageTreeItem::SetAccessLevel(int iAccessLevel)
{
   SET_INT(m_iAccessLevel, iAccessLevel)
}

long MessageTreeItem::GetCreated()
{
   return m_lCreated;
}

long MessageTreeItem::GetExpire()
{
   return m_lExpire;
}

bool MessageTreeItem::SetExpire(long lExpire)
{
   // printf("MessageTreeItem::SetExpire %ld\n", lExpire);
   SET_INT(m_lExpire, lExpire)
}

long MessageTreeItem::GetNumMsgs()
{
   long lNumMsgs = 0;

   MessageStats(&lNumMsgs, NULL);

   return lNumMsgs;
}

long MessageTreeItem::GetTotalMsgs()
{
   long lTotalMsgs = 0;

   MessageStats(NULL, &lTotalMsgs);

   return lTotalMsgs;
}

/* MessageTreeItem *MessageTreeItem::GetReply()
{
   return m_pReply;
} */

/* bool MessageTreeItem::SetReply(MessageTreeItem *pReply)
{
   m_pReply = pReply;
   if(m_pReply != NULL)
   {
      m_lReplyID = m_pReply->GetID();
   }
   else
   {
      m_lReplyID = -1;
   }

   m_bChanged = true;

   return true;
} */

long MessageTreeItem::GetReplyID()
{
   /* if(m_pReply != NULL)
   {
      return m_pReply->GetID();
   } */

   return m_lReplyID;
}

bool MessageTreeItem::SetReplyID(long lReplyID)
{
   /* if(m_pReply != NULL)
   {
      debug("MessageTreeItem::SetReplyID exit false, reply %p %ld\n", m_pReply, m_pReply->GetID());
      return false;
   } */

   m_lReplyID = lReplyID;

   SetChanged(true);

   return true;
}

/* UserItem *MessageTreeItem::GetProvider()
{
   return m_pProvider;
} */

/* bool MessageTreeItem::SetProvider(UserItem *pProvider)
{
   m_pProvider = pProvider;
   if(m_pProvider != NULL)
   {
      m_lProviderID = m_pProvider->GetID();
   }
   else
   {
      m_lProviderID = -1;
   }

   m_bChanged = true;

   return true;
} */

long MessageTreeItem::GetProviderID()
{
   /* if(m_pProvider != NULL)
   {
      return m_pProvider->GetID();
   } */

   return m_lProviderID;
}

bool MessageTreeItem::SetProviderID(long lProviderID)
{
   /* if(m_pProvider != NULL)
   {
      debug("MessageTreeItem::SetProviderID exit false, Provider %p %ld\n", m_pProvider, m_pProvider->GetID());
      return false;
   } */

   m_lProviderID = lProviderID;

   SetChanged(true);

   return true;
}

long MessageTreeItem::GetInfoID()
{
   return m_lInfoID;
}

bool MessageTreeItem::SetInfo(long lInfoID, bytes *pInfo)
{
   if(pInfo != NULL)
   {
      m_lInfoDate = time(NULL);
      m_lInfoID = lInfoID;

      SET_BYTES(m_pInfo, pInfo, -1)
   }
   else
   {
      m_lInfoDate = -1;
      m_lInfoID = -1;

      delete m_pInfo;
      m_pInfo = NULL;

      SetChanged(true);
   }

   return true;
}

bool MessageTreeItem::MessageAdd(MessageItem *pMessage)
{
   STACKTRACE
   // int iChildNum = 0;
   // MessageItem *pChild = NULL, *pFind = NULL;

   if(m_pMessages == NULL)
   {
      m_pMessages = new EDFItemList(m_bMessageDelete);
   }
   else
   {
      if(MessageFind(pMessage->GetID()) != NULL)
      {
         return false;
      }
   }

   if(pMessage->GetDate() > m_lLastMsg)
   {
      m_lLastMsg = pMessage->GetDate();
   }

   return m_pMessages->Add(pMessage);
}

bool MessageTreeItem::MessageDelete(long lMessageID)
{
   STACKTRACE
   if(m_pMessages == NULL)
   {
      return false;
   }

   return m_pMessages->Delete(lMessageID);
}

int MessageTreeItem::MessageCount(bool bRecurse)
{
   STACKTRACE
   int iReturn = 0, iChildNum = 0;
   MessageItem *pChild = NULL;

   // printf("MessageTreeItem::MessageCount %s, %p %s %p\n", BoolStr(bRecurse), this, m_szName, m_pMessages);

   if(m_pMessages == NULL)
   {
      return 0;
   }

   iReturn = m_pMessages->Count();

   if(bRecurse == true)
   {
      pChild = (MessageItem *)m_pMessages->Item(iChildNum);
      iReturn += pChild->Count(bRecurse);
   }

   return iReturn;
}

int MessageTreeItem::MessagePos(long lMessageID)
{
   STACKTRACE
   int iReturn = 0, iChildNum = 0;
   bool bFound = false;
   MessageItem *pChild = NULL;

   if(m_pMessages == NULL)
   {
      return 0;
   }

   while(bFound == false && iChildNum < m_pMessages->Count())
   {
      pChild = (MessageItem *)m_pMessages->Item(iChildNum);
      iReturn += pChild->MessagePos(lMessageID, &bFound);
      if(bFound == false)
      {
         iChildNum++;
      }
   }

   if(bFound == false)
   {
      return 0;
   }

   return iReturn;
}

MessageItem *MessageTreeItem::MessageFind(long lID, bool bRecurse)
{
   // STACKTRACE
   int iChildNum = 0;
   MessageItem *pChild = NULL, *pFind = NULL;

   if(m_pMessages == NULL)
   {
      return NULL;
   }

   // printf("TreeNodeItem::Find entry %ld %s, %ld\n", lID, BoolStr(bRecurse), GetID());
   /* if(lID == GetID())
   {
      // printf("TreeNodeItem::Find exit %p, this\n", this);
      return (T)this;
   } */

   /* if(bRecurse == false || m_pChildren == NULL)
   {
      // printf("TreeNodeItem::Find exit NULL, no recurse (children %p)\n", m_pChildren);
      return NULL;
   } */

   while(iChildNum < m_pMessages->Count())
   {
      pChild = (MessageItem *)m_pMessages->Item(iChildNum);
      pFind = pChild->Find(lID, true);
      if(pFind != NULL)
      {
         // printf("TreeNodeItem::Find exit %p, child %d\n", pFind, iChildNum);
         return pFind;
      }
      else
      {
         iChildNum++;
      }
   }

   // printf("TreeNodeItem::Find exit NULL\n");
   return NULL;
}

MessageItem *MessageTreeItem::MessageChild(int iChildNum)
{
   STACKTRACE

   if(m_pMessages == NULL)
   {
      return NULL;
   }

   return (MessageItem *)m_pMessages->Item(iChildNum);
}

long MessageTreeItem::GetMaxID()
{
   STACKTRACE
   int iChildNum = 0;
   long lReturn = 0, lID = 0;
   MessageItem *pChild = NULL;

   if(m_pMessages == NULL)
   {
      return 0;
   }

   for(iChildNum = 0; iChildNum < m_pMessages->Count(); iChildNum++)
   {
      pChild = (MessageItem *)m_pMessages->Item(iChildNum);

      lID = pChild->GetMaxID();
      if(lID > lReturn)
      {
         lReturn = lID;
      }
   }

   return lReturn;
}

MessageTreeItem *MessageTreeItem::NextRow(char *szType, DBTable *pTable, MessageTreeItem *pParent, bool bMessageDelete)
{
   STACKTRACE

   if(m_pInit == NULL)
   {
      m_pInit = new MessageTreeItem(NULL, (long)0, NULL, false);
      // debug("MessageTreeItem::NextRow class %s\n", m_pInit->GetClass());
   }

   m_pInit->InitFields(pTable);

   if(pTable->NextRow() == false)
   {
      return NULL;
   }

   return new MessageTreeItem(szType, pTable, pParent, bMessageDelete);
}

// IsEDFField: Field name checker
bool MessageTreeItem::IsEDFField(char *szName)
{
   STACKTRACE
   // Fields that have member storage
   if(stricmp(szName, "parentid") == 0 ||
      stricmp(szName, "name") == 0 ||
      stricmp(szName, "accessmode") == 0 ||
      stricmp(szName, "accesslevel") == 0 ||
      stricmp(szName, "created") == 0 ||
      stricmp(szName, "expire") == 0 ||
      stricmp(szName, "nummsgs") == 0 ||
      stricmp(szName, "totalmsgs") == 0 ||
      stricmp(szName, "lastmsg") == 0 ||
      stricmp(szName, "lastedit") == 0 ||
      stricmp(szName, "creatorid") == 0 ||
      stricmp(szName, "replyid") == 0)
   {
      return false;
   }

   // Fields that should not be stored
   if(stricmp(szName, "message") == 0 ||
      stricmp(szName, m_szType) == 0 ||
      stricmp(szName, "maxreplies") == 0 ||
      stricmp(szName, "childlink") == 0 ||
      stricmp(szName, "movelink") == 0)
   {
      return false;
   }

   // return TreeNodeItem<MessageItem *>::IsEDFField(szName, true);

   if(stricmp(szName, "info") != 0 && stricmp(szName, "providerid") != 0)
   {
      debug("MessageTreeItem::IsEDFField %s, true\n", szName);
   }
   return true;
}

// WriteFields: Output to EDF
bool MessageTreeItem::WriteFields(EDF *pEDF, int iLevel)
{
   STACKTRACE
   int iAccessMode = 0;
   long lNumMsgs = 0, lTotalMsgs = 0;

   // printf("MessageTreeItem::WriteFields %p %d, %s\n", pEDF, iLevel, m_szName);

   if(iLevel == -1)
   {
      iLevel = EDFITEMWRITE_HIDDEN + EDFITEMWRITE_ADMIN + MESSAGETREEITEMWRITE_DETAILS + MESSAGETREEITEMWRITE_INFO;
   }

   if(GetDeleted() == true)
   {
     pEDF->AddChild("deleted", true);
   }
   if(mask(iLevel, EDFITEMWRITE_HIDDEN) == true)
   {
      pEDF->AddChild("parentid", GetParentID());
   }

   pEDF->AddChild("name", m_szName);
   iAccessMode = m_iAccessMode;
   if(m_lProviderID != -1)
   {
      iAccessMode |= FOLDERMODE_CONTENT;
   }
   pEDF->AddChild("accessmode", iAccessMode);
   if(m_iAccessLevel != -1)
   {
      pEDF->AddChild("accesslevel", m_iAccessLevel);
   }

   if(m_lReplyID != -1)
   {
      pEDF->AddChild("replyid", m_lReplyID);
   }

   if(mask(iLevel, MESSAGETREEITEMWRITE_DETAILS) == true)
   {
      MessageStats(&lNumMsgs, &lTotalMsgs);
      pEDF->AddChild("nummsgs", lNumMsgs);
      if(m_lLastMsg != -1)
      {
         pEDF->AddChild("lastmsg", m_lLastMsg);
      }
      if(mask(iLevel, EDFITEMWRITE_ADMIN) == true)
      {
         pEDF->AddChild("totalmsgs", lTotalMsgs);
         if(m_lCreated != -1)
         {
            pEDF->AddChild("created", m_lCreated);
         }
         if(m_lExpire != -1)
         {
            // printf("MessageTreeItem::WriteFields expire %ld\n", m_lExpire);
            pEDF->AddChild("expire", m_lExpire);
         }
         if(m_lLastEdit != -1)
         {
            pEDF->Add("lastedit", m_lLastEdit);
            pEDF->AddChild("userid", m_lLastEditID);
            pEDF->Parent();
         }
         if(m_lCreatorID != -1)
         {
            pEDF->AddChild("creatorid", m_lCreatorID);
         }
         if(m_lProviderID != -1)
         {
            pEDF->AddChild("providerid", m_lProviderID);
         }
      }
      if(m_iMaxReplies != -1)
      {
         pEDF->AddChild("maxreplies", m_iMaxReplies);
      }

      if(mask(iLevel, MESSAGETREEITEMWRITE_INFO) == true && m_pInfo != NULL)
      {
         pEDF->Add("info");

         pEDF->AddChild("date", m_lInfoDate);
         pEDF->AddChild("fromid", m_lInfoID);
         pEDF->AddChild("text", m_pInfo);

         pEDF->Parent();
      }
   }

   // printf("MessageTreeItem::WriteFields exit true\n");
   return true;
}

bool MessageTreeItem::WriteEDF(EDF *pEDF, int iLevel)
{
   return true;
}

bool MessageTreeItem::WriteFields(DBTable *pTable, EDF *pEDF)
{
   STACKTRACE

   TreeNodeItem<MessageTreeItem *>::WriteFields(pTable, pEDF);

   // printf("MessageTreeItem::WriteFields %p\n", m_pInfo);
   // bytesprint("MessageTreeItem::WriteFields info", m_pInfo);

   pTable->SetField(m_szName);
   pTable->SetField(m_iAccessMode);
   pTable->SetField(m_iAccessLevel);
   pTable->SetField(m_lCreated);
   pTable->SetField(m_lExpire);
   pTable->SetField(m_lLastMsg);
   pTable->SetField(m_lLastEdit);
   pTable->SetField(m_lLastEditID);
   pTable->SetField(m_lCreatorID);
   pTable->SetField(m_lReplyID);
   pTable->SetField(m_iMaxReplies);

   pTable->SetField(m_lInfoDate);
   pTable->SetField(m_lInfoID);
   pTable->SetField(m_pInfo);

   if(m_lProviderID != -1)
   {
      pEDF->AddChild("providerid", m_lProviderID);
   }

   return true;
}

bool MessageTreeItem::ReadFields(DBTable *pTable)
{
   STACKTRACE

   TreeNodeItem<MessageTreeItem *>::ReadFields(pTable);

   // name, accessmode, accesslevel, created, expire, lastmsg, lastedit, lasteditid, creatorid, replyid, maxreplies
   pTable->BindColumnBytes();
   pTable->BindColumnInt();
   pTable->BindColumnInt();
   pTable->BindColumnInt();
   pTable->BindColumnInt();
   pTable->BindColumnInt();
   pTable->BindColumnInt();
   pTable->BindColumnInt();
   pTable->BindColumnInt();
   pTable->BindColumnInt();
   pTable->BindColumnInt();

   // infodate, infoid, infotext
   pTable->BindColumnInt();
   pTable->BindColumnInt();
   pTable->BindColumnBytes();

   return true;
}

char *MessageTreeItem::TableName()
{
   return m_szTypeTable;
}

char *MessageTreeItem::KeyName()
{
   return m_szTypeKey;
}

char *MessageTreeItem::WriteName()
{
   return m_szType;
}

void MessageTreeItem::init(char *szType, bool bMessageDelete)
{
   STACKTRACE
   char szTemp[100];

   debug("MessageTreeItem::init %s %s\n", szType, BoolStr(bMessageDelete));

   m_szType = strmk(szType);
   sprintf(szTemp, "%s_item", m_szType);
   m_szTypeTable = strmk(szTemp);
   sprintf(szTemp, "%sid", m_szType);
   m_szTypeKey = strmk(szTemp);
   m_bMessageDelete = bMessageDelete;

   // Top level fields
   m_szName = NULL;

   m_iAccessMode = 0;
   m_iAccessLevel = -1;
   m_lCreated = time(NULL);
   m_lExpire = -1;
   m_lLastMsg = -1;
   m_lLastEdit = -1;
   m_lLastEditID = -1;
   m_lCreatorID = -1;
   // m_pReply = NULL;
   m_lReplyID = -1;
   // m_pProvider = NULL;
   m_lProviderID = -1;
   m_iMaxReplies = -1;

   m_lInfoDate = -1;
   m_lInfoID = -1;
   m_pInfo = NULL;

   m_pMessages = NULL;
}

void MessageTreeItem::setup()
{
   STACKTRACE

   EDFParser::debugPrint("MessageTreeItem::setup", m_pEDF);

   if(m_pEDF->GetChildBool("deleted") == true)
   {
      SetDeleted(true);
      m_pEDF->DeleteChild("deleted");
   }

   if(m_pEDF->Child("info") == true)
   {
      ExtractMember(m_pEDF, "date", &m_lInfoDate, -1);
      ExtractMember(m_pEDF, "fromid", &m_lInfoID, -1);
      ExtractMember(m_pEDF, "text", &m_pInfo);

      m_pEDF->Delete();
   }

   if(m_pEDF->Child("providerid") == true)
   {
      m_pEDF->Get(NULL, &m_lProviderID);

      m_pEDF->Delete();
   }

   // Fix old data
   if(m_lExpire < 0)
   {
      m_lExpire = -1;
      SetChanged(true);
   }
}

bool MessageTreeItem::MessageStats(long *lNumMsgs, long *lTotalMsgs)
{
   STACKTRACE
   int iChildNum = 0;
   // double dTick = gettick();

   if(lNumMsgs != NULL)
   {
      *lNumMsgs = 0;
   }
   if(lTotalMsgs != NULL)
   {
      *lTotalMsgs = 0;
   }

   if(m_pMessages == NULL)
   {
      return false;
   }

   for(iChildNum = 0; iChildNum < m_pMessages->Count(); iChildNum++)
   {
      MessageStat((MessageItem *)m_pMessages->Item(iChildNum), lNumMsgs, lTotalMsgs);
   }

   // printf("MessageTreeItem::MessageStats %d -> %ld, %ld ms\n", iType, lReturn, tickdiff(dTick));
   return true;
}

void MessageTreeItem::MessageStat(MessageItem *pMessage, long *lNumMsgs, long *lTotalMsgs)
{
   // STACKTRACE
   int iChildNum = 0, iAttNum = 0, iNumAtts = 0;
   // long lReturn = 0;

   /* printf("MessageTreeItem::MessageStat %p(%ld)", pMessage, pMessage->GetID());
   if(pMessage->GetText() != NULL)
   {
      debug(", %p(%ld)", pMessage->GetText(), pMessage->GetText()->Length());
   }
   debug("\n"); */

   if(pMessage->GetTree() == this)
   {
      if(lNumMsgs != NULL)
      {
         (*lNumMsgs)++;
      }

      if(lTotalMsgs != NULL)
      {
         if(pMessage->GetText() != NULL)
         {
            (*lTotalMsgs) += pMessage->GetText()->Length();
         }

         iNumAtts = pMessage->AttachmentCount();
         for(iAttNum = 0; iAttNum < iNumAtts; iAttNum++)
         {
            (*lTotalMsgs) += pMessage->AttachmentSize(iAttNum);
         }
      }
   }

   for(iChildNum = 0; iChildNum < pMessage->Count(false); iChildNum++)
   {
      MessageStat(pMessage->Child(iChildNum), lNumMsgs, lTotalMsgs);
   }

   // printf("MessageTreeItem::MessageStat exit %ld\n", lReturn);
}
