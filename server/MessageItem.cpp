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
** MessageItem.cpp: Implmentation of MessageItem class
*/

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "ua.h"

#include "MessageItem.h"
#include "MessageTreeItem.h"

// #define MESSAGEWRITE_HIDDEN 1

// Constructor with no data
MessageItem::MessageItem(long lID, MessageItem *pParent, MessageTreeItem *pTree) : TreeNodeItem<MessageItem *>(lID, pParent)
{
   STACKTRACE

   init(pTree);
}

// Constructor with EDF data
MessageItem::MessageItem(EDF *pEDF, MessageItem *pParent, MessageTreeItem *pTree) : TreeNodeItem<MessageItem *>(pEDF, pParent)
{
   STACKTRACE
   // bool bCopy = false;

   // printf("MessageItem::MessageItem %p %p %p\n", pEDF, pParent, pTree);

   init(pTree);

   // EDFPrint("MessageItem::MessageItem", pEDF, EDFElement::EL_CURR | EDFElement::PR_SPACE);

   // Top level fields
   ExtractMember(pEDF, "msgtype", &m_iType, 0);
   ExtractMember(pEDF, "date", &m_lDate, -1);
   // printf("MessageItem::MessageItem date %ld\n", m_lDate);
   ExtractMember(pEDF, "fromid", &m_lFromID, -1);
   ExtractMember(pEDF, "toid", &m_lToID, -1);
   ExtractMember(pEDF, "toname", &m_pToName);
   ExtractMember(pEDF, "text", &m_pText);

   setup();
}

MessageItem::MessageItem(DBTable *pTable, MessageItem *pParent, MessageTreeItem *pTree, int iEDFField) : TreeNodeItem<MessageItem *>(pTable, pParent, iEDFField)
{
   STACKTRACE
   // bytes *pEDF = NULL;

   init(pTree);

   GET_INT_TABLE(2, m_lTreeID, -1)

   // GET_INT_TABLE(3, m_iType, 0)
   GET_INT_TABLE(3, m_lDate, -1)
   GET_INT_TABLE(4, m_lFromID, -1)
   GET_INT_TABLE(5, m_lToID, -1)
   GET_BYTES_TABLE(6, m_pText, true)

   // pTable->GetField(8, &pEDF);
   // bytesprint("MessageItem::MessageItem EDF field", pEDF);

   // EDFPrint("MessageItem::MessageItem EDF section", m_pEDF);

   setup();
}

// Destructor
MessageItem::~MessageItem()
{
   STACKTRACE
   // printf("MessageItem::~MessageItem %p(%ld)\n", this, GetID());

   delete m_pText;
}

MessageTreeItem *MessageItem::GetTree()
{
   return m_pTree;
}

bool MessageItem::SetTree(MessageTreeItem *pTree)
{
   m_pTree = pTree;
   if(m_pTree != NULL)
   {
      m_lTreeID = m_pTree->GetID();
   }
   else
   {
      m_lTreeID = -1;
   }

   SetChanged(true);

   return true;
}

long MessageItem::GetTreeID()
{
   if(m_pTree != NULL)
   {
      return m_pTree->GetID();
   }

   return m_lTreeID;
}

bool MessageItem::SetTreeID(long lTreeID)
{
   if(m_pTree != NULL)
   {
      debug("MessageItem::SetTreeID exit false, folder %p %ld\n", m_pTree, m_pTree->GetID());
      return false;
   }

   m_lTreeID = lTreeID;

   SetChanged(true);

   return true;
}

int MessageItem::GetType()
{
   int iReturn = m_iType;

   if(GetDeleted() == true)
   {
      iReturn |= MSGTYPE_DELETED;
   }

   return iReturn;
}

bool MessageItem::SetType(int iType)
{
   SET_INT(m_iType, iType)
}

long MessageItem::GetDate()
{
   return m_lDate;
}

long MessageItem::GetFromID()
{
   return m_lFromID;
}

bool MessageItem::SetFromID(long lFromID)
{
   SET_INT(m_lFromID, lFromID)
}

bytes *MessageItem::GetFromName(bool bCopy)
{
   GET_BYTES_EDF(NULL, "fromname")
}

bool MessageItem::SetFromName(bytes *pFromName)
{
   SET_EDF_BYTES(NULL, "fromname", pFromName, true, UA_SHORTMSG_LEN)
}

long MessageItem::GetToID()
{
   return m_lToID;
}

bool MessageItem::SetToID(long lToID)
{
   SET_INT(m_lToID, lToID)
}

bytes *MessageItem::GetToName(bool bCopy)
{
   GET_BYTES(m_pToName)
}

bool MessageItem::SetToName(bytes *pToName)
{
   SET_BYTES(m_pToName, pToName, UA_SHORTMSG_LEN)
}

bytes *MessageItem::GetText(bool bCopy)
{
   GET_BYTES(m_pText)
}

bool MessageItem::SetText(bytes *pText)
{
   SET_BYTES(m_pText, pText, -1)
}

bool MessageItem::AddText(bytes *pText)
{
   m_pEDF->Add("text", pText);
   m_pEDF->AddChild("date", (int)time(NULL));
   m_pEDF->Parent();

   SetChanged(true);

   return true;
}

char *MessageItem::KeyName()
{
   return "messageid";
}

char *MessageItem::WriteName()
{
   return "message";
}

bool MessageItem::AddDetail(char *szName, long lUserID, long lDate, bool bParent)
{
   m_pEDF->Add(szName, (int)(lDate != -1 ? lDate : time(NULL)));
   m_pEDF->AddChild("userid", lUserID);
   if(bParent == true)
   {
      m_pEDF->Parent();
   }

   SetChanged(true);

   return true;
}

bool MessageItem::AddDelete(long lUserID, long lDate)
{
   return AddDetail("delete", lUserID, lDate);
}

bool MessageItem::AddEdit(long lUserID, long lDate)
{
   return AddDetail("edit", lUserID, lDate);
}

bool MessageItem::AddAttachment(bool bParent, char *szURL, char *szContentType, char *szFilename, bytes *pData, EDF *pEDF, long lFromID, long lDate)
{
   debug("MessageItem::AddAttachment %s %s %s %s %p(%ld) %p %ld %ld\n", BoolStr(bParent), szURL, szContentType, szFilename, pData, pData != NULL ? pData->Length() : 0, pEDF, lFromID, lDate);

   m_pEDF->Add("attachment", EDFMax(m_pEDF, "attachment", false) + 1);

   if(szURL != NULL)
   {
      m_pEDF->AddChild("url", szURL);
   }
   else
   {
      m_pEDF->AddChild("content-type", szContentType);
      if(szFilename != NULL)
      {
         m_pEDF->AddChild("filename", szFilename);
      }

      if(pData != NULL)
      {
         m_pEDF->AddChild("data", pData);
      }
      else if(pEDF != NULL)
      {
         m_pEDF->Add("content");
         m_pEDF->Copy(pEDF, false);
      }
   }

   if(lFromID != -1)
   {
      m_pEDF->AddChild("fromid", lFromID);
      m_pEDF->AddChild("date", (int)(lDate != -1 ? lDate : time(NULL)));
   }

   if(bParent == true)
   {
      m_pEDF->Parent();
   }

   SetChanged(true);

   return true;
}

bool MessageItem::AddAttachment(char *szURL, long lFromID, long lDate)
{
   return AddAttachment(true, szURL, NULL, NULL, NULL, NULL, lFromID, lDate);
}

bool MessageItem::AddAttachment(char *szContentType, char *szFilename, bytes *pData, EDF *pEDF, long lFromID, long lDate)
{
   return AddAttachment(true, NULL, szContentType, szFilename, pData, pEDF, lFromID, lDate);
}

bool MessageItem::DeleteAttachment(int iAttachmentID, long lUserID, long lDate)
{
   if(EDFFind(m_pEDF, "attachment", iAttachmentID, false) == false)
   {
      return false;
   }

	if(m_pEDF->IsChild("delete") == true)
	{
		m_pEDF->Parent();

		return false;
	}

   // m_pEDF->Delete();

   AddDetail("delete", lUserID, lDate);

   m_pEDF->Parent();

   SetChanged(true);

   return true;
}

bool MessageItem::WriteAttachment(EDF *pEDF, int iAttachmentID, int iLevel)
{
   if(EDFFind(m_pEDF, "attachment", iAttachmentID, false) == false)
   {
      return false;
   }

   // pEDF->Copy(m_pEDF);

   /* if(m_pEDF->IsChild("delete") == true && mask(iLevel, EDFITEMWRITE_ADMIN) == false)
   {
      m_pEDF->Parent();

      return false;
   } */

   WriteEDFAttachment(pEDF, MESSAGEITEMWRITE_ATTDATA);

   EDFParser::debugPrint("MessageItem::WriteAttachment", pEDF, EDFElement::EL_CURR | EDFElement::PR_SPACE | EDFElement::PR_BIN);

   return true;
}

long MessageItem::AttachmentID(int iAttachmentNum)
{
   int iAttachmentID = -1;

   if(m_pEDF->Child("attachment", iAttachmentNum) == false)
   {
      return -1;
   }

   m_pEDF->Get(NULL, &iAttachmentID);

   m_pEDF->Parent();

   return iAttachmentID;
}

long MessageItem::AttachmentSize(int iAttachmentNum)
{
   int iAttachmentID = 0;
   long lReturn = 0;
   char *szContentType = NULL;
   bytes *pText = NULL, *pData = NULL;

   if(m_pEDF->Child("attachment", iAttachmentNum) == false)
   {
      return -1;
   }

   m_pEDF->Get(NULL, &iAttachmentID);
   m_pEDF->GetChild("content-type", &szContentType);

   // printf("MessageItem::AttachmentSize %d %s\n", iAttachmentID, szContentType);

   if(szContentType != NULL)
   {
      if(stricmp(szContentType, MSGATT_ANNOTATION) == 0 || stricmp(szContentType, MSGATT_LINK) == 0)
      {
         m_pEDF->GetChild("text", &pText);
         if(pText != NULL)
         {
            lReturn = pText->Length();
            delete pText;
         }
      }
      else
      {
         m_pEDF->GetChild("data", &pData);
         if(pData != NULL)
         {
            lReturn = pData->Length();
            delete pData;
         }
      }

      delete[] szContentType;
   }

   m_pEDF->Parent();

   return lReturn;
}

int MessageItem::AttachmentCount()
{
   /* EDFPrint("MessageItem::AttachmentCount whole", m_pEDF);
   debug("\n");
   debugEDFPrint("MessageItem::AttachmentCount current", m_pEDF, EDFElement::EL_CURR | EDFElement::PR_SPACE); */

   return m_pEDF->Children("attachment");
}

int MessageItem::MessagePos(long lID, bool *bFound)
{
   // STACKTRACE
   int iChildNum = 0, iReturn = 0;
   MessageItem *pChild = NULL;

   iReturn++;

   if(GetID() == lID)
   {
      (*bFound) = true;
   }
   else
   {
      while((*bFound) == false && iChildNum < Count(false))
      {
         pChild = Child(iChildNum);
         if(GetTreeID() == pChild->GetTreeID())
         {
            iReturn += pChild->MessagePos(lID, bFound);
         }
         if((*bFound) == false)
         {
            iChildNum++;
         }
      }
   }

   // printf("MessageItem::MessagePos %d\n", iReturn);
   return iReturn;
}

long MessageItem::GetMaxID()
{
   int iChildNum = 0;
   long lReturn = 0, lID = GetID();
   MessageItem *pChild = NULL;

   for(iChildNum = 0; iChildNum < Count(false); iChildNum++)
   {
      pChild = Child(iChildNum);

      lID = pChild->GetMaxID();
      if(lID > lReturn)
      {
         lReturn = lID;
      }
   }

   return lReturn;
}

// IsEDFField: Field name checker
bool MessageItem::IsEDFField(char *szName)
{
   STACKTRACE
   char szTreeID[32];

   // printf("MessageItem::IsEDFField %s\n", szName);

   sprintf(szTreeID, "%sid", TreeName());

   // Fields that have member storage
   if(stricmp(szName, "parentid") == 0 ||
      stricmp(szName, szTreeID) == 0 ||
      stricmp(szName, "msgtype") == 0 ||
      stricmp(szName, "date") == 0 ||
      stricmp(szName, "fromid") == 0 ||
      stricmp(szName, "toid") == 0 ||
      stricmp(szName, "text") == 0)
   {
      return false;
   }

   // Fields that should not be stored
   if(stricmp(szName, "message") == 0 ||
      stricmp(szName, "parentlink") == 0 ||
      stricmp(szName, "childlink") == 0)
   {
      return false;
   }

   // return TreeNodeItem<MessageItem *>::IsEDFField(szName, true);

   /* if(stricmp(szName, "vote") != 0)
   {
      debug("MessageItem::IsEDFField %s, true\n", szName);
   } */
   return true;
}

// WriteFields: Output to EDF
bool MessageItem::WriteFields(EDF *pEDF, int iLevel)
{
   STACKTRACE
   // int iVoteType = 0, iVoteID = 0, iUserID = 0, iNumVotes = 0, iTotalVotes = 0;
   // bool bLoop = false;
   char szTreeID[32];
   bytes *pFromName = NULL;

   sprintf(szTreeID, "%sid", TreeName());

   debug(DEBUGLEVEL_DEBUG, "MessageItem::WriteFields %p %d, %p %p\n", pEDF, iLevel, GetParent(), GetTree());

   if(iLevel == -1)
   {
      iLevel = EDFITEMWRITE_HIDDEN + EDFITEMWRITE_ADMIN + MESSAGEITEMWRITE_DETAILS + MESSAGEITEMWRITE_TEXT + MESSAGEITEMWRITE_ATTDATA;
   }

   // printf("MessageItem::WriteFields date %ld\n", m_lDate);

   if(mask(iLevel, EDFITEMWRITE_HIDDEN) == true)
   {
      pEDF->AddChild("parentid", GetParentID());
      pEDF->AddChild(szTreeID, GetTreeID());
   }

   if(GetType() > 0)
   {
      // printf("MessageItem::WriteFields type %d\n", m_iType);
      pEDF->AddChild("msgtype", GetType());
   }

   if(mask(iLevel, MESSAGEITEMWRITE_DETAILS) == true || mask(iLevel, MESSAGEITEMWRITE_CACHE) == true)
   {
      pEDF->AddChild("date", m_lDate);
   }

   if(mask(iLevel, MESSAGEITEMWRITE_DETAILS) == true)
   {
      // printf("MessageItem::WriteFields %p %d, %p %s(%ld)\n", pEDF, iLevel, m_pTree, m_pSubject, m_lSubjectLen);

      if(m_lFromID != -1)
      {
         pEDF->AddChild("fromid", m_lFromID);
      }
      if(m_pEDF->GetChild("fromname", &pFromName) == true)
      {
         pEDF->AddChild("fromname", pFromName);
         delete pFromName;
      }
      if(m_lToID != -1)
      {
         pEDF->AddChild("toid", m_lToID);
      }
      else if(m_pToName != NULL)
      {
         pEDF->AddChild("toname", m_pToName);
      }
   }

   if(mask(iLevel, MESSAGEITEMWRITE_TEXT) == true)
   {
      if(m_pText != NULL)
      {
         pEDF->AddChild("text", m_pText);
      }
   }

   debug(DEBUGLEVEL_DEBUG, "MessageItem::WriteFields exit true\n");
   return true;
}

bool MessageItem::WriteEDF(EDF *pEDF, int iLevel)
{
   bool bLoop = false;

   // EDFPrint("MessageItem::WriteEDF", m_pEDF);

   if(iLevel == -1)
   {
      iLevel = EDFITEMWRITE_HIDDEN + EDFITEMWRITE_ADMIN + MESSAGEITEMWRITE_DETAILS + MESSAGEITEMWRITE_TEXT + MESSAGEITEMWRITE_ATTDATA;
   }

   if(mask(iLevel, MESSAGEITEMWRITE_DETAILS) == true)
   {
      // bytesprint("MessageItem::WriteEDF toname", m_pToName);

      if(m_pToName != NULL)
      {
         pEDF->AddChild("toname", m_pToName);
      }

      bLoop = m_pEDF->Child("attachment");
      while(bLoop == true)
      {
         WriteEDFAttachment(pEDF, iLevel);

         bLoop = m_pEDF->Next("attachment");
         if(bLoop == false)
         {
            m_pEDF->Parent();
         }
      }
   }

   return true;
}

bool MessageItem::WriteFields(DBTable *pTable, EDF *pEDF)
{
   STACKTRACE

   TreeNodeItem<MessageItem *>::WriteFields(pTable, pEDF);

   pTable->SetField(GetTreeID());
   // pTable->SetField(m_iType);
   pTable->SetField(m_lDate);
   pTable->SetField(m_lFromID);
   pTable->SetField(m_lToID);
   pTable->SetField(m_pText);

   if(GetType() > 0)
   {
      debug("MessageItem::WriteFields type %d\n", GetType());
      pEDF->AddChild("msgtype", GetType());
   }

   // bytesprint("MessageItem::WriteFields toname", m_pToName);
   if(m_pToName != NULL)
   {
      pEDF->AddChild("toname", m_pToName);
   }

   return true;
}

bool MessageItem::ReadFields(DBTable *pTable)
{
   STACKTRACE

   TreeNodeItem<MessageItem *>::ReadFields(pTable);

   // treeid, type, date, fromid, toid, text
   pTable->BindColumnInt();
   // pTable->BindColumnInt();
   pTable->BindColumnInt();
   pTable->BindColumnInt();
   pTable->BindColumnInt();
   pTable->BindColumnBytes();
   // pTable->BindColumnBytes();

   return true;
}

void MessageItem::WriteDetail(EDF *pEDF, char *szName, int iLevel)
{
   int iDate = -1, iUserID = -1, iTreeID = -1;
   bool bLoop = false;

   bLoop = m_pEDF->Child(szName);
   while(bLoop == true)
   {
      iDate = -1;
      iUserID = -1;
      iTreeID = -1;

      m_pEDF->Get(NULL, &iDate);
      m_pEDF->GetChild("userid", &iUserID);
      m_pEDF->GetChild("folderid", &iTreeID);

      if(iDate != -1)
      {
         pEDF->Add(szName, iDate);
      }
      else
      {
         pEDF->Add(szName);
      }
      if(mask(iLevel, EDFITEMWRITE_ADMIN) == true && iUserID != -1)
      {
         pEDF->AddChild("userid", iUserID);
      }
      if(iTreeID != -1)
      {
         pEDF->AddChild("folderid", iTreeID);
      }
      pEDF->Parent();

      bLoop = m_pEDF->Next(szName);
      if(bLoop == false)
      {
         m_pEDF->Parent();
      }
   }
}

void MessageItem::init(MessageTreeItem *pTree)
{
   // Top level fields

   m_pTree = pTree;
   m_lTreeID = -1;

   m_iType = 0;
   m_lDate = time(NULL);
   m_lFromID = -1;
   m_lToID = -1;
   m_pToName = NULL;
   m_pText = NULL;
}

void MessageItem::setup()
{
   // int iDate = -1, iFromID = -1, iTreeID = -1, iUserID = -1;
   // bool bMove = false;
   char *szFromName = NULL;
   // bytes *pText = NULL;
   // EDFElement *pVotes = NULL, *pElement = NULL;

   // EDFPrint("MessageItem::setup", m_pEDF);

   // Old style data fixing
   // bCopy = m_pEDF->GetCopy(false);

   m_pEDF->GetChild("msgtype", &m_iType);
   if(mask(m_iType, MSGTYPE_DELETED) == true)
   {
      m_iType -= MSGTYPE_DELETED;
      SetDeleted(true);
   }
   // printf("MessageItem::setup msgtype %d\n", m_iType);

   m_pEDF->GetChild("toname", &m_pToName);

   if(m_pEDF->GetChild("fromname", &szFromName) == true)
   {
      debug("MessageItem::setup fromname %s\n", szFromName);
      delete[] szFromName;
   }

   // m_pEDF->GetCopy(bCopy);
}

void MessageItem::WriteEDFAttachment(EDF *pEDF, int iLevel)
{
   int iAttachmentID = 0;
   char *szContentType = NULL, *szFilename = NULL;
   bytes *pData = NULL;

   if(mask(iLevel, EDFITEMWRITE_ADMIN) == true || m_pEDF->IsChild("delete") == false)
   {
      m_pEDF->Get(NULL, &iAttachmentID);

      // pEDF->Copy(m_pEDF);
      pEDF->Add("attachment", iAttachmentID);
      if(pEDF->AddChild(m_pEDF, "url") == false)
      {
         m_pEDF->GetChild("content-type", &szContentType);
         pEDF->AddChild("content-type", szContentType);
         if(stricmp(szContentType, MSGATT_ANNOTATION) == 0)
         {
            pEDF->AddChild(m_pEDF, "text");
         }
         else if(stricmp(szContentType, MSGATT_LINK) == 0)
         {
            pEDF->AddChild(m_pEDF, "text");
            pEDF->AddChild(m_pEDF, "messageid");
         }
         else
         {
            m_pEDF->GetChild("filename", &szFilename);
            // if(mask(iLevel, MESSAGEWRITE_ATTDATA) == true)
            {
               m_pEDF->GetChild("data", &pData);
            }

            debug("MessageItem::WriteEDFAttachment %d (%s) %s", iAttachmentID, szContentType, szFilename);
            pEDF->AddChild("filename", szFilename);
            if(pData != NULL)
            {
               debug(", %p(%ld)\n", pData, pData->Length());
               if(mask(iLevel, MESSAGEITEMWRITE_ATTDATA) == true)
               {
                  pEDF->AddChild("data", pData);
               }
               else
               {
                  pEDF->AddChild("content-length", pData->Length());
               }
            }
            else if(m_pEDF->Child("content") == true)
            {
               debug("\n");
               EDFParser::debugPrint(m_pEDF, false, true);

               pEDF->Copy(m_pEDF);
               m_pEDF->Parent();
            }
            else
            {
               debug("\n");
            }

            delete pData;
            delete[] szFilename;
         }
         delete[] szContentType;
      }

      pEDF->AddChild(m_pEDF, "fromid");
      pEDF->AddChild(m_pEDF, "date");

      WriteDetail(pEDF, "delete", iLevel);

	   pEDF->Parent();
	}
}
