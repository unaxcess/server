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
** RqTree.cpp: Implementation of server side message tree request functions
*/

#include <stdio.h>
#ifdef WIN32
#include <time.h>
#endif

// Framework headers
#include "headers.h"
#include "ICE/ICE.h"

// Common headers
#include "Folder.h"
#include "Talk.h"

// Library headers
#include "server.h"

#include "RqTree.h"
#include "RqFolder.h"
#include "RqTalk.h"

char *MessageType(int iBase)
{
   switch(iBase)
   {
      case RFG_FOLDER:
         return "folder";

      case RFG_CHANNEL:
         return "channel";
   }

   debug("MessageType NULL\n");
   return NULL;
}

char *MessageTreeSubTable(int iBase)
{
   switch(iBase)
   {
      case RFG_FOLDER:
         return "folder_sub";

      case RFG_CHANNEL:
         return "channel_sub";
   }

   debug("MessageTreeSubTable NULL\n");
   return NULL;
}

int MessageTreeCount(int iBase, bool bTree)
{
   int iReturn = 0;

   switch(iBase)
   {
      case RFG_FOLDER:
         iReturn = FolderCount(bTree);
         break;

      case RFG_CHANNEL:
         iReturn = ChannelCount(bTree);
         break;

      default:
         debug("MessageTreeCount invalid base %d\n", iBase);
         break;
   }

   // printf("MessageTreeCount %d %s -> %d\n", iBase, BoolStr(bTree), iReturn);
   return iReturn;
}

MessageTreeItem *MessageTreeGet(int iBase, int iID, char **szName, bool bCopy)
{
   MessageTreeItem *pReturn = NULL;

   switch(iBase)
   {
      case RFG_FOLDER:
      case RFG_MESSAGE:
      case RFG_CONTENT:
         pReturn = FolderGet(iID, szName, bCopy);
         break;

      case RFG_CHANNEL:
         pReturn = ChannelGet(iID, szName, bCopy);
         break;

      default:
         debug("MessageTreeGet invalid base %d\n", iBase);
         break;
   }

   return pReturn;
}

MessageTreeItem *MessageTreeList(int iBase, int iListNum, bool bTree)
{
   MessageTreeItem *pReturn = NULL;

   switch(iBase)
   {
      case RFG_FOLDER:
         pReturn = FolderList(iListNum, bTree);
         break;

      case RFG_CHANNEL:
         pReturn = ChannelList(iListNum, bTree);
         break;

      default:
         debug("MessageTreeList invalid base %d\n", iBase);
         break;
   }

   // printf("MessageTreeCount %d %s -> %p\n", iBase, BoolStr(bTree), pReturn);
   return pReturn;
}

bool MessageTreeSetParent(int iBase, MessageTreeItem *pItem, int iParentID)
{
   bool bReturn = false;

   switch(iBase)
   {
      case RFG_FOLDER:
         bReturn = FolderSetParent(pItem, iParentID);
         break;

      case RFG_CHANNEL:
         bReturn = ChannelSetParent(pItem, iParentID);
         break;

      default:
         debug("MessageTreeSetParent invalid base %d\n", iBase);
         break;
   }

   return bReturn;
}

// MessageTreeAccess: Access checker
int MessageTreeAccess(MessageTreeItem *pItem, int iSubMode, int iMemMode, int iAccessLevel, int iSubType, bool bSubCheck)
{
   if(iSubType == 0 && bSubCheck == true)
   {
      return 0;
   }

   if(iAccessLevel >= LEVEL_WITNESS)
   {
      return SUBTYPE_EDITOR;
   }

   if(pItem->GetDeleted() == true)
   {
      return 0;
   }

   if(iAccessLevel >= LEVEL_WITNESS || iSubType == SUBTYPE_EDITOR)
   {
      return SUBTYPE_EDITOR;
   }

   if(mask(pItem->GetAccessMode(), iMemMode) == true &&
      (iSubType >= SUBTYPE_MEMBER ||(pItem->GetAccessLevel() != -1 && iAccessLevel >= pItem->GetAccessLevel())))
   {
      return SUBTYPE_MEMBER;
   }

   if(mask(pItem->GetAccessMode(), iSubMode) == true)
   {
      return SUBTYPE_SUB;
   }

   return 0;
}

// MessageAccess: Access checker
int MessageAccess(MessageItem *pMessage, int iUserID, int iAccessLevel, int iSubType, bool bPrivateCheck, bool bDeleteCheck)
{
   if(bPrivateCheck == false && (iAccessLevel >= LEVEL_WITNESS || iSubType == SUBTYPE_EDITOR))
   {
      return SUBTYPE_EDITOR;
   }

   if(bDeleteCheck == true && iAccessLevel < LEVEL_WITNESS && pMessage->GetDeleted() == true)
   {
      return 0;
   }

   if(mask(pMessage->GetTree()->GetAccessMode(), ACCMODE_PRIVATE) == false ||
      (iUserID != -1 && (pMessage->GetFromID() == iUserID || pMessage->GetToID() == iUserID)))
   {
      // printf("MessageAccessRead %p(%s %d) %d %d(%ld %ld) %d %d true\n", pMessage, pMessage->GetTree()->GetName(), pMessage->GetTree()->GetAccessMode(), iOp, iUserID, pMessage->GetFromID(), pMessage->GetToID(), iAccessLevel, iSubType);
      return SUBTYPE_SUB;
   }

   return 0;
}

char *MessageTreeStr(int iBase, char *szStr)
{
   char *szReturn = NULL;

   switch(iBase)
   {
      case RFG_FOLDER:
         if(stricmp(szStr, "not_exist") == 0)
         {
            szReturn = MSG_FOLDER_NOT_EXIST;
         }
         else if(stricmp(szStr, "access_invalid") == 0)
         {
            szReturn = MSG_FOLDER_ACCESS_INVALID;
         }
         break;

      case RFG_MESSAGE:
         if(stricmp(szStr, "not_exist") == 0)
         {
            szReturn = MSG_MESSAGE_NOT_EXIST;
         }
         else if(stricmp(szStr, "access_invalid") == 0)
         {
            szReturn = MSG_MESSAGE_ACCESS_INVALID;
         }
         break;

      case RFG_CONTENT:
         if(stricmp(szStr, "not_exist") == 0)
         {
            szReturn = MSG_CONTENT_NOT_EXIST;
         }
         else if(stricmp(szStr, "access_invalid") == 0)
         {
            szReturn = MSG_CONTENT_ACCESS_INVALID;
         }
         break;

      case RFG_BULLETIN:
         if(stricmp(szStr, "not_exist") == 0)
         {
            szReturn = MSG_BULLETIN_NOT_EXIST;
         }
         else if(stricmp(szStr, "access_invalid") == 0)
         {
            szReturn = MSG_BULLETIN_ACCESS_INVALID;
         }
         break;

      case RFG_CHANNEL:
         if(stricmp(szStr, "not_exist") == 0)
         {
            szReturn = MSG_CHANNEL_NOT_EXIST;
         }
         else if(stricmp(szStr, "access_invalid") == 0)
         {
            szReturn = MSG_CHANNEL_ACCESS_INVALID;
         }
         break;
   }

   if(szReturn == NULL)
   {
      debug("MessageTreeStr %d %s -> %s\n", iBase, szStr, szReturn);
   }
   return szReturn;
}

char *MessageTreeStr(int iBase, int iOp)
{
   char *szReturn = NULL;

   switch(iBase)
   {
      case RFG_FOLDER:
         if(iOp == RQ_ADD)
         {
            szReturn = MSG_FOLDER_ADD;
         }
         else if(iOp == RQ_DELETE)
         {
            szReturn = MSG_FOLDER_DELETE;
         }
         else if(iOp == RQ_EDIT)
         {
            szReturn = MSG_FOLDER_EDIT;
         }
         break;

      case RFG_MESSAGE:
         if(iOp == RQ_ADD)
         {
            szReturn = MSG_MESSAGE_ADD;
         }
         else if(iOp == RQ_DELETE)
         {
            szReturn = MSG_MESSAGE_DELETE;
         }
         else if(iOp == RQ_EDIT)
         {
            szReturn = MSG_MESSAGE_EDIT;
         }
         break;

      case RFG_CONTENT:
         if(iOp == RQ_ADD)
         {
            szReturn = MSG_CONTENT_ADD;
         }
         else if(iOp == RQ_DELETE)
         {
            szReturn = MSG_CONTENT_DELETE;
         }
         else if(iOp == RQ_EDIT)
         {
            szReturn = MSG_CONTENT_EDIT;
         }
         break;

      case RFG_BULLETIN:
         if(iOp == RQ_ADD)
         {
            szReturn = MSG_BULLETIN_ADD;
         }
         else if(iOp == RQ_DELETE)
         {
            szReturn = MSG_BULLETIN_DELETE;
         }
         else if(iOp == RQ_EDIT)
         {
            szReturn = MSG_BULLETIN_EDIT;
         }
         break;

      case RFG_CHANNEL:
         if(iOp == RQ_ADD)
         {
            szReturn = MSG_CHANNEL_ADD;
         }
         else if(iOp == RQ_DELETE)
         {
            szReturn = MSG_CHANNEL_DELETE;
         }
         else if(iOp == RQ_EDIT)
         {
            szReturn = MSG_CHANNEL_EDIT;
         }
         break;
   }

   if(szReturn == NULL)
   {
      debug("MessageTreeStr %d %d -> %s\n", iBase, iOp, szReturn);
   }
   return szReturn;
}

char *MessageTreeID(int iBase)
{
   char *szReturn = NULL;

   switch(iBase)
   {
      case RFG_FOLDER:
      case RFG_MESSAGE:
      case RFG_CONTENT:
         szReturn = "folderid";
         break;

      case RFG_CHANNEL:
         szReturn = "channelid";
         break;

      default:
         debug("MessageTreeID invalid base %d\n", iBase);
         break;
   }

   return szReturn;
}

char *MessageTreeName(int iBase)
{
   char *szReturn = NULL;

   switch(iBase)
   {
      case RFG_FOLDER:
      case RFG_MESSAGE:
      case RFG_CONTENT:
         szReturn = "foldername";
         break;

      case RFG_CHANNEL:
         szReturn = "channelname";
         break;

      default:
         debug("MessageTreeName invalid base %d\n", iBase);
         break;
   }

   return szReturn;
}

char *MessageTreeNum(int iBase)
{
   char *szReturn = NULL;

   switch(iBase)
   {
      case RFG_FOLDER:
         szReturn = "numfolders";
         break;

      case RFG_BULLETIN:
         szReturn = "nummsgs";
         break;

      case RFG_CHANNEL:
         szReturn = "numchannels";
         break;

      default:
         debug("MessageTreeNum invalid base %d\n", iBase);
         break;
   }

   return szReturn;
}

char *MessageStr(int iBase, char *szStr)
{
   char *szReturn = NULL;

   switch(iBase)
   {
      case RFG_MESSAGE:
      case RFG_CHANNEL:
         if(stricmp(szStr, "not_exist") == 0)
         {
            szReturn = MSG_MESSAGE_NOT_EXIST;
         }
         else if(stricmp(szStr, "access_invalid") == 0)
         {
            szReturn = MSG_MESSAGE_ACCESS_INVALID;
         }
         break;

      case RFG_CONTENT:
         if(stricmp(szStr, "not_exist") == 0)
         {
            szReturn = MSG_CONTENT_NOT_EXIST;
         }
         else if(stricmp(szStr, "access_invalid") == 0)
         {
            szReturn = MSG_CONTENT_ACCESS_INVALID;
         }
         break;

      case RFG_BULLETIN:
         if(stricmp(szStr, "not_exist") == 0)
         {
            szReturn = MSG_BULLETIN_NOT_EXIST;
         }
         else if(stricmp(szStr, "access_invalid") == 0)
         {
            szReturn = MSG_BULLETIN_ACCESS_INVALID;
         }
         break;

      default:
         debug("MessageStr invalid base %d\n", iBase);
         break;
   }

   return szReturn;
}

char *MessageID(int iBase)
{
   char *szReturn = NULL;

   switch(iBase)
   {
      case RFG_MESSAGE:
      case RFG_CHANNEL:
         szReturn = "messageid";
         break;

      case RFG_CONTENT:
         szReturn = "objectid";
         break;

      case RFG_BULLETIN:
         szReturn = "bulletinid";
         break;

      default:
         debug("MessageID invalid base %d\n", iBase);
         break;
   }

   return szReturn;
}

/*
** MessageTreeAccessFull: Check user has access to requested message tree / message for required operation
** Returns message tree / message and fills output EDF with relevant fields on failure
*/
bool MessageTreeAccessFull(int iBase, int iOp, int iID, int iTreeID, bool *pArchive, UserItem *pUser, DBSub *pSubs, EDF *pOut, MessageTreeItem **pMessageTreeOut, MessageItem **pMessageOut, int *pSubType)
{
   STACKTRACE
   int iUserID = -1, iAccessLevel = LEVEL_NONE, iSubType = 0;
   char szSQL[200];
   char *szUserName = NULL;
   MessageTreeItem *pMessageTree = NULL, *pMessageTreeParent = NULL;
   MessageItem *pMessage = NULL;
   DBTable *pTable = NULL;
   FolderMessageItem *pFolderMessage = NULL;

   // printf("MessageTreeAccessFull entry %d %d %d %s %p %p\n", iBase, iOp, iID, szArchive, pUser, pSubs);

   if(iOp == MTA_MESSAGE_READ || iOp == MTA_MESSAGE_EDIT)
   {
      STACKTRACEUPDATE
      if(iBase == RFG_MESSAGE || iBase == RFG_BULLETIN)
      {
         // Get message from ID
         if(iBase == RFG_BULLETIN)
         {
            pMessage = BulletinGet(iID);
            debug("MessageTreeAccessFull bulletin %d -> %p\n", iID, pMessage);
         }
         else
         {
            pMessage = MessageGet(iBase, iID);
         }
         if(pMessageOut != NULL)
         {
            *pMessageOut = pMessage;
         }

         if(pMessage != NULL)
         {
            if(pArchive != NULL)
            {
               *pArchive = false;
            }
         }
         else
         {
            if(pArchive != NULL && *pArchive == true)
            {
               debug("MessageTreeAccessFull get message %d from archive\n", iID);

               sprintf(szSQL, "select * from %s where messageid = %d", FMI_TABLENAME, iID);

               // pTable = FolderMessageItem::ReadFields();
               pTable = new DBTable();
               if(pTable->Execute(szSQL) == true)
               {
                  pMessage = FolderMessageItem::NextRow(pTable, NULL, NULL);
                  if(pMessage != NULL)
                  // if(pTable->NextRow() == true)
                  {
                     // pMessage = new FolderMessageItem(pTable, NULL, NULL);

                     pMessageTree = MessageTreeGet(iBase, pMessage->GetTreeID());
                     if(pMessageTree != NULL)
                     {
                        pMessage->SetTree(pMessageTree);
                     }

                     if(pMessageOut != NULL)
                     {
                        *pMessageOut = pMessage;
                     }
                     if(pMessageTreeOut != NULL)
                     {
                        *pMessageTreeOut = pMessageTree;
                     }

                     pMessage->SetType(pMessage->GetType() | MSGTYPE_ARCHIVE);

                     // printf("MessageTreeAccessFull exit true, archive %p (%p) %d\n", pMessage, pMessageTree, iID);
                     return true;
                  }
                  else
                  {
                     // printf("MessageTreeAccessFull archive SQL no rows\n");
                  }
               }
               else
               {
                  debug("MessageTreeAccessFull archive SQL failed\n");
               }
               delete pTable;
            }

            if(pMessage == NULL)
            {
               if(pOut != NULL)
               {
                  pOut->Set("reply", MessageStr(iBase, "not_exist"));
                  pOut->AddChild(MessageID(iBase), iID);
               }

               debug("MessageTreeAccessFull exit false(1), %s %d\n", MessageTreeStr(iBase, "not_exist"), iID);
               return false;
            }
         }
      }
      else
      {
         // Get message from parameter
         pMessage = *pMessageOut;
      }

      pMessageTree = pMessage->GetTree();
      if(pMessageTreeOut != NULL)
      {
         *pMessageTreeOut = pMessageTree;
      }
   }
   else
   {
      STACKTRACEUPDATE
      if(pMessageTreeOut != NULL && *pMessageTreeOut != NULL)
      {
         pMessageTree = *pMessageTreeOut;
      }
      else
      {
         iTreeID = iID;

         // Get message from ID
         if(iTreeID > 0)
         {
            pMessageTree = MessageTreeGet(iBase, iTreeID);
            if(pMessageTreeOut != NULL)
            {
               *pMessageTreeOut = pMessageTree;
            }
         }

         if(pMessageTree == NULL)
         {
            if(pOut != NULL)
            {
               pOut->Set("reply", MessageTreeStr(iBase, "not_exist"));
               pOut->AddChild(MessageTreeID(iBase), iTreeID);
            }

            debug("MessageTreeAccessFull exit false(2), %s %d\n", MessageTreeStr(iBase, "not_exist"), iTreeID);
            return false;
         }
      }
   }

   STACKTRACEUPDATE

   if(pUser != NULL)
   {
      iUserID = pUser->GetID();
      szUserName = pUser->GetName();
      iAccessLevel = pUser->GetAccessLevel();
   }

   if(pSubs != NULL)
   {
      iSubType = pSubs->SubType(pMessageTree->GetID());
      if(pSubType != NULL)
      {
         *pSubType = iSubType;
      }
   }

   // printf("MessageTreeAccessFull user %s %d %d, message tree %s %d %d\n", szUserName, iAccessLevel, iSubType, pMessageTree->GetName(), pMessageTree->GetAccessMode(), pMessageTree->GetAccessLevel());

   if(pMessageTree != NULL &&
      (iBase == RFG_CONTENT && pMessageTree->GetProviderID() == -1 && mask(pMessageTree->GetAccessMode(), FOLDERMODE_CONTENT) == false) ||
      (iBase != RFG_CONTENT && (pMessageTree->GetProviderID() != -1 || mask(pMessageTree->GetAccessMode(), FOLDERMODE_CONTENT) == true)))
   {
      if(pOut != NULL)
      {
         pOut->Set("reply", MessageTreeStr(iBase, "access_invalid"));
         pOut->AddChild(MessageTreeID(iBase), pMessageTree->GetID());
         pOut->AddChild(MessageTreeName(iBase), pMessageTree->GetName());
      }

      printf("MessageTreeAccessFull exit false(3), %s (%ld content request %d)\n", MessageTreeStr(iBase, "access_invalid"), pMessageTree->GetID(), iBase);
      return false;
   }

   if(iAccessLevel >= LEVEL_WITNESS)
   {
      // printf("MessageTreeAccessFull exit true, LEVEL_WITNESS\n");
      return true;
   }

   if(iBase == RFG_MESSAGE || iBase == RFG_CONTENT)
   {
      pFolderMessage = (FolderMessageItem *)pMessage;
   }

   if(iBase == RFG_MESSAGE)
   {
      // Check deleted
      if(pMessage != NULL && pFolderMessage->GetDeleted() == true)
      {
         if(pOut != NULL)
         {
            pOut->Set("reply", MessageStr(iBase, "not_exist"));
            pOut->AddChild(MessageID(iBase), pMessage->GetID());
         }

         debug("MessageTreeAccessFull exit false(4), %s (%ld no access)\n", MessageTreeStr(iBase, "not_exist"), pMessage->GetID());
         return false;
      }
   }

   if(iSubType == SUBTYPE_EDITOR && iOp != MTA_MESSAGE_EDIT)
   {
      // printf("MessageTreeAccessFull exit true, SUBTYPE_EDITOR\n");
      return true;
   }

   if(pMessageTree != NULL)
   {
      if(pMessageTree->GetDeleted() == true)
      {
         // Message tree cannot be read (deleted)
         if(pOut != NULL)
         {
            pOut->Set("reply", MessageStr(iBase, "not_exist"));
            pOut->AddChild(MessageID(iBase), iTreeID);
         }

         debug("MessageTreeAccessFull exit false(5), %s (%d deleted)\n", MessageTreeStr(iBase, "not_exist"), iTreeID);
         return false;
      }

      pMessageTreeParent = pMessageTree;
      while(pMessageTreeParent != NULL)
      {
         if(MessageTreeAccess(pMessageTreeParent, ACCMODE_SUB_READ, ACCMODE_MEM_READ, iAccessLevel, iSubType) == 0)
         {
            // Message tree cannot be read (hidden from user)
            if(pOut != NULL)
            {
               pOut->Set("reply", MessageTreeStr(iBase, "not_exist"));
               pOut->AddChild(MessageTreeID(iBase), iTreeID);
            }

            debug("MessageTreeAccessFull exit false(6), %s (%d no access)\n", MessageTreeStr(iBase, "not_exist"), iTreeID);
            return false;
         }
         else
         {
            pMessageTreeParent = pMessageTreeParent->GetParent();
         }
      }
   }

   if(iOp == MTA_MESSAGETREE_READ)
   {
      // printf("MessageTreeAccessFull exit true, MTA_MESSAGETREE_READ\n");
      return true;
   }

   if(iOp == MTA_MESSAGETREE_EDIT || (iOp == MTA_MESSAGE_WRITE && MessageTreeAccess(pMessageTree, ACCMODE_SUB_WRITE, ACCMODE_MEM_WRITE, iAccessLevel, iSubType) == 0))
   {
      // Message tree cannot be edited / written
      if(pOut != NULL)
      {
         pOut->Set("reply", MessageTreeStr(iBase, "access_invalid"));
         pOut->AddChild(MessageTreeID(iBase), pMessageTree->GetID());
         pOut->AddChild(MessageTreeName(iBase), pMessageTree->GetName());
      }

      printf("MessageTreeAccessFull exit false(7), %s (%ld no access)\n", MessageTreeStr(iBase, "access_invalid"), pMessageTree->GetID());
      return false;
   }

   if(iOp == MTA_MESSAGE_WRITE)
   {
      if(iBase == RFG_CONTENT && iUserID != pMessageTree->GetProviderID())
      {
         if(pOut != NULL)
         {
            pOut->Set("reply", MessageTreeStr(iBase, "access_invalid"));
            pOut->AddChild(MessageTreeID(iBase), pMessageTree->GetID());
            pOut->AddChild(MessageTreeName(iBase), pMessageTree->GetName());
         }

         printf("MessageTreeAccessFull exit false(8), %s (%ld no access)\n", MessageTreeStr(iBase, "access_invalid"), pMessageTree->GetID());
         return false;
      }

      // printf("MessageTreeAccessFull exit true, MTA_MESSAGETREE_WRITE\n");
      return true;
   }

   // printf("MessageTreeAccessFull checking access read %p %d %d %d %d\n", pMessage, iOp, iUserID, iAccessLevel, iSubType);
   if(MessageAccess(pMessage, iUserID, iAccessLevel, iSubType) == 0)
   {
      if(pOut != NULL)
      {
         pOut->Set("reply", MessageStr(iBase, "not_exist"));
         pOut->AddChild(MessageID(iBase), pMessage->GetID());
      }

      printf("MessageTreeAccessFull exit false(9), %s (%ld no access)\n", MessageTreeStr(iBase, "not_exist"), pMessage->GetID());
      return false;
   }

   if(iOp == MTA_MESSAGE_READ)
   {
      // printf("MessageTreeAccessFull exit true, MTA_MESSAGE_READ\n");
      return true;
   }

   if(iOp == MTA_MESSAGE_EDIT && (iAccessLevel >= LEVEL_WITNESS || iSubType == SUBTYPE_EDITOR || iUserID == pMessage->GetFromID() || iUserID == pMessage->GetToID()))
   {
      // printf("MessageTreeAccessFull exit true, MTA_MESSAGE_EDIT\n");
      return true;
   }

   // Some other access - default to no access
   if(pMessage != NULL)
   {
      if(pOut != NULL)
      {
         pOut->Set("reply", MessageStr(iBase, "access_invalid"));
         pOut->AddChild(MessageID(iBase), pMessage->GetID());
         if(pMessageTree != NULL)
         {
            pOut->AddChild(MessageTreeID(iBase), pMessageTree->GetID());
            pOut->AddChild(MessageTreeName(iBase), pMessageTree->GetName());
         }
      }

      printf("MessageTreeAccessFull exit false(10), %s (%ld no access)\n", MessageStr(iBase, "access_invalid"), pMessage->GetID());
      return false;
   }
   else
   {
      if(pOut != NULL)
      {
         pOut->Set("reply", MessageTreeStr(iBase, "access_invalid"));
         pOut->AddChild(MessageTreeID(iBase), pMessageTree->GetID());
         pOut->AddChild(MessageTreeName(iBase), pMessageTree->GetName());
      }

      printf("MessageTreeAccessFull exit false(11), %s (%ld %s no access)\n", MessageTreeStr(iBase, "access_invalid"), pMessageTree->GetID(), pMessageTree->GetName());
      return false;
   }

   // printf("MessageTreeAccessFull exit true\n");
   return true;
}

bool MessageTreeAccess(int iBase, int iOp, int iID, UserItem *pUser, DBSub *pSubs, EDF *pOut, MessageTreeItem **pMessageTreeOut, MessageItem **pMessageOut, int *iSubType)
{
   return MessageTreeAccessFull(iBase, iOp, iID, -1, NULL, pUser, pSubs, pOut, pMessageTreeOut, pMessageOut, iSubType);
}

bool MessageTreeAccess(int iBase, int iOp, int iID, int iTreeID, UserItem *pUser, DBSub *pSubs, EDF *pOut, MessageTreeItem **pMessageTreeOut, MessageItem **pMessageOut, int *iSubType)
{
   return MessageTreeAccessFull(iBase, iOp, iID, iTreeID, NULL, pUser, pSubs, pOut, pMessageTreeOut, pMessageOut, iSubType);
}

bool MessageTreeAccess(int iBase, int iOp, int iID, int iTreeID, bool *pArchive, UserItem *pUser, DBSub *pSubs, EDF *pOut, MessageTreeItem **pMessageTreeOut, MessageItem **pMessageOut, int *iSubType)
{
   return MessageTreeAccessFull(iBase, iOp, iID, iTreeID, pArchive, pUser, pSubs, pOut, pMessageTreeOut, pMessageOut, iSubType);
}

bool MessageTreeAccess(int iBase, int iOp, MessageTreeItem *pMessageTree, UserItem *pUser, DBSub *pSubs, EDF *pOut, int *iSubType)
{
   return MessageTreeAccessFull(iBase, iOp, -1, -1, NULL, pUser, pSubs, pOut, &pMessageTree, NULL, iSubType);
}

#define ACC_CHECK(x) \
debug("MessageTreeEdit mask %d", x); \
if(mask(iAccessMode, x) == true) \
{ \
   debug(", set"); \
   iChangeMode += x; \
} \
debug("\n");

// MessageTreeEdit: Set fields for message tree add / edit request
bool MessageTreeItemEdit(EDF *pData, int iOp, MessageTreeItem *pItem, EDF *pIn, EDF *pOut, UserItem *pCurr, int iBase)
{
   STACKTRACE
   bool bParent = false;
   int iParentID = -1, iReplyID = -1, iAccessMode = 0, iAccessLevel = LEVEL_NONE, iChangeMode = 0, iExpire = -1, iProviderID = -1;
   // long lTextLen = 0;
   char *szName = NULL;
   bytes *pText = NULL;
   EDF *pAnnounce = NULL, *pAdmin = NULL;
   MessageTreeItem *pParent = NULL, *pReply = NULL;
   UserItem *pProvider = NULL;

   if(pCurr->GetAccessLevel() >= LEVEL_WITNESS)
   {
      // Admin only settings
      if(iOp != RQ_ADD && pIn->GetChild("name", &szName) == true)
      {
         if(NameValid(szName) == true)
         {
            // Only set a name if valid
            pItem->SetName(szName);
         }
         else
         {
            debug("MessageTreeEdit invalid name change '%s'\n", szName);
         }
         delete[] szName;
      }


      if(pIn->GetChild("parentid", &iParentID) == true)
      {
         // Reset parent
         if(MessageTreeSetParent(iBase, pItem, iParentID) == true)
         {
            bParent = true;
         }
      }

      if(pIn->GetChild("replyid", &iReplyID) == true)
      {
         // Reset reply ID
         pReply = MessageTreeGet(iBase, iReplyID);
         debug("MessageTreeEdit reply ID %d -> %p\n", iReplyID, pReply);

         if(pReply != NULL || iReplyID == -1)
         {
            pItem->SetReplyID(iReplyID);
         }
      }

      if(pIn->GetChild("providerid", &iProviderID) == true)
      {
         pProvider = UserGet(iProviderID);

         if(pProvider != NULL || iProviderID == -1)
         {
            pItem->SetProviderID(iProviderID);
         }
      }

      if(pIn->GetChild("accessmode", &iAccessMode) == true)
      {
         ACC_CHECK(ACCMODE_SUB_READ);
         ACC_CHECK(ACCMODE_SUB_WRITE);
         ACC_CHECK(ACCMODE_MEM_READ);
         ACC_CHECK(ACCMODE_MEM_WRITE);
         ACC_CHECK(ACCMODE_PRIVATE);
         ACC_CHECK(FOLDERMODE_SUB_SDEL);
         ACC_CHECK(FOLDERMODE_SUB_ADEL);
         ACC_CHECK(FOLDERMODE_SUB_MOVE);
         ACC_CHECK(FOLDERMODE_MEM_SDEL);
         ACC_CHECK(FOLDERMODE_MEM_ADEL);
         ACC_CHECK(FOLDERMODE_MEM_MOVE);

         debug("MessageTreeEdit access mode %d, %d / %d", pItem->GetAccessMode(), iAccessMode, iChangeMode);
         if(iChangeMode != pItem->GetAccessMode())
         {
            // printf("MessageTreeEdit access mode change %d -> %d\n", pItem->GetAccessMode(), iChangeMode);
            pItem->SetAccessMode(iChangeMode);
         }
         debug("\n");
      }

      if(pIn->GetChild("accesslevel", &iAccessLevel) == true && (iAccessLevel == -1 || iAccessLevel == LEVEL_MESSAGES || iAccessLevel == LEVEL_EDITOR || iAccessLevel == LEVEL_WITNESS))
      {
         pItem->SetAccessLevel(iAccessLevel);
      }

      if(pIn->GetChild("expire", &iExpire) == true && (iExpire >= 0 || iExpire == -1))
      {
         pItem->SetExpire(iExpire);
      }
   }

   if(pIn->Child("info") == true)
   {
      // Change info file
      if(pIn->GetChild("text", &pText) == true)
      {
         pItem->SetInfo(pCurr->GetID(), pText);
         delete pText;
      }

      pIn->Parent();
   }

   // Always announce a message tree being added, otherwise only if something has happened
   if(iOp == RQ_ADD || pItem->GetChanged() == true)
   {
      if(bParent == true)
      {
         pParent = pItem->GetParent();
      }

      if(iOp == RQ_ADD)
      {
         if(iBase == RFG_FOLDER)
         {
            pItem->Insert();
         }
      }
      else
      {
         if(iBase == RFG_FOLDER)
         {
            pItem->Update();
         }
      }

      pAnnounce = new EDF();
      pAnnounce->AddChild(MessageTreeID(iBase), pItem->GetID());
      pAnnounce->AddChild(MessageTreeName(iBase), pItem->GetName());
      if(pParent != NULL)
      {
         pAnnounce->AddChild("parentid", pParent->GetID());
         pAnnounce->AddChild("parentname", pParent->GetName());
      }

      pAdmin = new EDF();
      pAdmin->Copy(pAnnounce, true, true);
      // Admin get who edited the message tree
      pAdmin->AddChild("byid", pCurr->GetID());
      pAdmin->AddChild("byname", pCurr->GetName());

      ServerAnnounce(pData, MessageTreeStr(iBase, iOp), pAnnounce, pAdmin);

      delete pAnnounce;
      delete pAdmin;
   }

   if(pOut != NULL)
   {
      pOut->AddChild(MessageTreeID(iBase), pItem->GetID());
      pOut->AddChild(MessageTreeName(iBase), pItem->GetName());
   }

   return true;
}

int MessageUnread(MessageTreeItem *pItem, FolderMessageItem *pMessage, int iUserID, int iAccessLevel, int iSubType, DBMessageRead *pReads)
{
   int iReturn = 0, iChildNum = 0;

   if(pMessage->GetTree() == pItem && MessageAccess(pMessage, iUserID, iAccessLevel, iSubType, true) > 0 && pReads->Get(pMessage->GetID()) == -1)
   {
      iReturn++;
   }

   for(iChildNum = 0; iChildNum < pMessage->Count(false); iChildNum++)
   {
      iReturn += MessageUnread(pItem, (FolderMessageItem *)pMessage->Child(iChildNum), iUserID, iAccessLevel, iSubType, pReads);
   }

   return iReturn;
}

int PrivateMessageStats(MessageTreeItem *pItem, MessageItem *pMessage, int iUserID)
{
   int iChildNum = 0, iNumChildren = 0, iNumMsgs = 0;
   MessageItem *pChild = NULL;

   if(pItem != NULL)
   {
      iNumChildren = pItem->Count(false);
   }
   else if(pMessage != NULL)
   {
      iNumChildren = pMessage->Count(false);
   }

   while(iChildNum < iNumChildren)
   {
      if(pItem != NULL)
      {
         pChild = pItem->MessageChild(iChildNum);
      }
      else
      {
         pChild = pMessage->Child(iChildNum);
      }

      if(pChild->GetFromID() == iUserID || pChild->GetToID() == iUserID)
      {
         iNumMsgs++;
      }

      iNumMsgs += PrivateMessageStats(NULL, pChild, iUserID);

      iChildNum++;
   }

   return iNumMsgs;
}

void MessageTreeItemParent(EDF *pOut, MessageTreeItem *pItem, int iBase)
{
   STACKTRACE
   MessageTreeItem *pParent = NULL;

   pParent = pItem->GetParent();
   if(pParent != NULL)
   {
      pOut->Add("pathid", pParent->GetID());
      pOut->AddChild(MessageTreeName(iBase), pParent->GetName());

      MessageTreeItemParent(pOut, pParent, iBase);

      pOut->Parent();
   }
}

/*
** MessageTreeItemList: Insert message tree item into EDF hierarchy
** iTree: output type
      0 - Single tree mode
      1 - Multi tree
      2 - Multi tree search mode (flat structure)
*/
int MessageTreeItemList(EDF *pOut, int iLevel, MessageTreeItem *pItem, int iBase, EDF *pData, int iTree, int iStartDate, int iUserID, int iAccessLevel, ConnData *pConnData, DBSub *pSubs, DBMessageRead *pReads, EDF *pSubInfo)
{
   STACKTRACE
   int iChildNum = 0, iUnread = 0, iReturn = 1, iSubTree = 0, iSubUser = 0, iSubType = 0, iUserSub = 0;//, iServiceID = -1;
   bool bLoop = false, bActive = false;
   long lDiff = 0;
   double dTick = 0;
   char *szName = NULL;
   MessageTreeItem *pChild = NULL, *pReply = NULL;//, *pParent = NULL;
   DBSubData *pSubData = NULL;
   UserItem *pUser = NULL, *pProvider = NULL;

   if(pSubs != NULL)
   {
      iUserSub = pSubs->SubType(pItem->GetID());
   }

   if(MessageTreeAccess(pItem, ACCMODE_SUB_READ, ACCMODE_MEM_READ, iAccessLevel, iUserSub) == 0)
   {
      // No access to this message tree
      debug("MessageTreeItemList %d cannot access %ld(%s %s %d %d)\n", iUserID, pItem->GetID(), pItem->GetName(), BoolStr(pItem->GetDeleted()), pItem->GetAccessMode(), pItem->GetAccessLevel());
      return 0;
   }

   STACKTRACEUPDATE

   /* if(bTree == false)
   {
      debug("MessageTreeItemList %p %s %d %d %d\n", pItem, pItem->GetName(), pItem->GetAccessLevel(), pItem->GetAccessMode(), pItem->GetExpire());
   } */

   /* if(iStartDate != -1)
   {
      debug("MessageTreeItemList %d %d, %p %s %ld\n", iTree, iStartDate, pItem, pItem->GetName(), pItem->GetCreated());
   } */

   /* if(iStartDate != -1)
   {
      debug("MessageTreeItemList tree %d date %d created %ld\n", iTree, iStartDate, pItem->GetCreated());
   } */

   if(iTree == 0 || iTree == 1 || (iTree == 2 && (iStartDate == -1 && iUserSub > 0) || (iStartDate != -1 && pItem->GetCreated() >= iStartDate)))
   {
      // Match on criteria
      pItem->Write(pOut, iLevel);

      // Fill out extra fields the message tree does not have

      if(pOut->Child("replyid") == true)
      {
         pReply = FolderGet(pItem->GetReplyID());

         if(pReply != NULL)
         {
            pOut->AddChild(MessageTreeName(iBase), pReply->GetName());
         }

         pOut->Parent();
      }

      if(pOut->Child("providerid") == true)
      {
         pProvider = UserGet(pItem->GetProviderID());

         if(pProvider != NULL)
         {
            pOut->AddChild("username", pProvider->GetName());
         }

         pOut->Parent();
      }

      if(pOut->Child("lastedit") == true)
      {
         pOut->Parent();
      }

      if(pOut->Child("info") == true)
      {
         if(UserGet(pItem->GetInfoID(), &szName, false) != NULL)
         {
            // Name of info user
            pOut->AddChild("fromname", szName);
         }
         else
         {
            pOut->DeleteChild("fromid");
         }
         pOut->Parent();
      }

      STACKTRACEUPDATE

      if(mask(pItem->GetAccessMode(), ACCMODE_PRIVATE) == true && mask(iLevel, MESSAGETREEITEMWRITE_DETAILS) == true && (iAccessLevel < LEVEL_WITNESS && iUserSub < SUBTYPE_EDITOR))
      {
         pOut->SetChild("nummsgs", PrivateMessageStats(pItem, NULL, iUserID));
      }

      if(pSubs != NULL)
      {
         // Add subscription info from self
         pSubData = pSubs->SubData(pItem->GetID());
         if(pSubData != NULL && pSubData->m_iSubType > 0)
         {
            pOut->AddChild("subtype", pSubData->m_iSubType);
            if(ConnVersion(pConnData, "2.5") >= 0)
            {
               if(pSubData->m_iExtra > 0)
               {
                  pOut->AddChild(iBase == RFG_FOLDER ? "priority" : "active", pSubData->m_iExtra);
               }
               if(pSubData->m_bTemp == true)
               {
                  pOut->AddChild("temp", true);
               }
            }
            else
            {
               pOut->AddChild("subscribed", true);
            }
         }
      }

      STACKTRACEUPDATE

      if(iBase == RFG_FOLDER && pReads != NULL)
      {
         // Calculate unread message count
         dTick = gettick();
         for(iChildNum = 0; iChildNum < pItem->MessageCount(false); iChildNum++)
         {
            iUnread += MessageUnread(pItem, (FolderMessageItem *)pItem->MessageChild(iChildNum), iUserID, iAccessLevel, iUserSub, pReads);
         }
         lDiff = tickdiff(dTick);
         if(lDiff > 100)
         {
            debug("MessageTreeItemList unread counted in %ld ms\n", lDiff);
         }

         if(iUnread > 0)
         {
            pOut->AddChild("unread", iUnread);
         }
      }

      STACKTRACEUPDATE

      if(iTree == 0)
      {
         if(pSubInfo != NULL || iBase == RFG_CHANNEL)
         {
            STACKTRACEUPDATE

            // Add subscription info
            debug("MessageTreeItemList subs list %ld, n=%s m=%d l=%d\n", pItem->GetID(), pItem->GetName(), pItem->GetAccessMode(), pItem->GetAccessLevel());
            if(iBase == RFG_CHANNEL)
            {
               EDFParser::debugPrint(pSubInfo);
            }

            bLoop = pSubInfo->Child();
            while(bLoop == true)
            {
               // iSubUser = 0;
               pSubInfo->Get(NULL, &iSubUser);
               // iSubType = SubTypeInt(pSubInfo->GetCurr()->getName(false));
               pSubInfo->GetChild("subtype", &iSubType);
               // pSubInfo->GetChild("userid", &iSubUser);
               bActive = pSubInfo->GetChildBool("active");

               pUser = UserGet(iSubUser, NULL, false, -1);
               if(pUser != NULL)
               {
                  debug("MessageTreeItemList subs user %d %d, n=%s l=%d t=%ld", iSubType, iSubUser, pUser->GetName(), pUser->GetAccessLevel(), pUser->GetTimeOn());

                  /* if((pUser->GetAccessLevel() >= LEVEL_WITNESS || (mask(pItem->GetAccessMode(), ACCMODE_SUB_READ) == true && time(NULL) - pUser->GetTimeOn() <= 30 * 86400)) ||
                     (mask(pItem->GetAccessMode(), ACCMODE_MEM_READ) == true && (iSubType >= SUBTYPE_MEMBER || (pItem->GetAccessLevel() != -1 && pUser->GetAccessLevel() >= pItem->GetAccessLevel())))) */
						if(mask(iLevel, EDFITEMWRITE_ADMIN) == true ||
							(iSubType == SUBTYPE_EDITOR && iUserSub >= SUBTYPE_SUB) ||
							(iSubType == SUBTYPE_MEMBER && iUserSub >= SUBTYPE_MEMBER))
                  {
                     debug(". adding\n");

                     pOut->Add(SubTypeStr(iSubType), iSubUser);
                     pOut->AddChild("name", pUser->GetName());
                     /* if(mask(pUser->GetStatus(), LOGIN_ON) == true)
                     {
                        pOut->AddChild("login", true);
                     } */
                     if(ConnectionFind(CF_USERID, pUser->GetID()) != NULL)
                     {
                        pOut->AddChild("login", true);
                     }
                     if(bActive == true)
                     {
                        pOut->AddChild("active", true);
                     }
                     // EDFPrint(pOut, EDFElement::EL_CURR | EDFElement::PR_SPACE);
                     pOut->Parent();
                  }
                  else
                  {
                     debug("\n");
                  }
               }

               bLoop = pSubInfo->Next();
               if(bLoop == false)
               {
                  pSubInfo->Parent();
               }
            }
         }

         // Single mode - add path to top of hierarchy
         MessageTreeItemParent(pOut, pItem, iBase);

         pOut->Parent();
      }
      else if(pSubInfo != NULL)
      {
         STACKTRACEUPDATE

         // Add edit info for everyone
         bLoop = pSubInfo->Child(SubTypeStr(SUBTYPE_EDITOR));
         while(bLoop == true)
         {
            iSubTree = 0;
            pSubInfo->Get(NULL, &iSubTree);

            // printf("MessageTreeItemList editors f=%d\n", iSubTree);

            if(iSubTree > pItem->GetID())
            {
               bLoop = false;
               pSubInfo->Parent();
            }
            else if(iSubTree == pItem->GetID())
            {
               iSubUser = 0;
               pSubInfo->GetChild("userid", &iSubUser);
               if(UserGet(iSubUser, &szName, false, -1) != NULL)
               {
                  // printf("MessageTreeItemList editors f=%d u=%d n=%s\n", iSubTree, iSubUser, szName);

                  pOut->Add(SubTypeStr(SUBTYPE_EDITOR), iSubUser);
                  pOut->AddChild("name", szName);
                  pOut->Parent();
               }
            }

            if(bLoop == true)
            {
               bLoop = pSubInfo->Next(SubTypeStr(SUBTYPE_EDITOR));
               if(bLoop == false)
               {
                  pSubInfo->Parent();
               }
            }
         }
      }
   }

   STACKTRACEUPDATE

   if(iTree != 0)
   {
      if(iTree == 2)
      {
         pOut->Parent();
      }

      // Tree mode - iterate through children
      for(iChildNum = 0; iChildNum < pItem->Count(false); iChildNum++)
      {
         pChild = pItem->Child(iChildNum);
         iReturn += MessageTreeItemList(pOut, iLevel, pChild, iBase, pData, iTree, iStartDate, iUserID, iAccessLevel, pConnData, pSubs, pReads, pSubInfo);
      }
   }

   STACKTRACEUPDATE

   if(iTree == 1)
   {
      pOut->Parent();
   }

   return iReturn;
}

MessageItem *MessageGet(int iBase, int iID)
{
   MessageItem *pReturn = NULL;

   switch(iBase)
   {
      case RFG_MESSAGE:
         pReturn = FolderMessageGet(iID);
         break;

      case RFG_BULLETIN:
         pReturn = BulletinGet(iID);
         break;

      default:
         debug("MessageGet invalid base %d\n", iBase);
         break;
   }

   return pReturn;
}

// MessageItemEdit: Set fields for message add / edit request
bool MessageItemEdit(EDF *pData, int iOp, MessageItem *pItem, EDF *pIn, EDF *pOut, UserItem *pCurr, ConnData *pConnData, char *szRequest, int iBase, bool bRecurse, int iSubType)
{
   STACKTRACE
   int iToID = -1, iUserNum = 0, iAttachmentNum = 0, iAttachmentID = -1, iMarked = 0;
   int iOldVoteType = 0;
   bool bPrivilege = false;
   bytes *pFromName = NULL, *pToName = NULL, *pText = NULL, *pTemp = NULL;
   EDF *pAnnounce = NULL, *pAdmin = NULL;
   EDFConn *pListConn = NULL;
   MessageTreeItem *pTree = NULL;
   UserItem *pFrom = NULL, *pTo = NULL, *pList = NULL;
   // FolderMessageItem *pFolderMessage = NULL;
   ChannelMessageItem *pChannelMessage = NULL;

   if(szRequest == NULL)
   {
      szRequest = MessageTreeStr(iBase, iOp);
   }

   debug("MessageItemEdit %p %d %d %p %p(%ld) %p %p\n", pData, iBase, iOp, pCurr, pItem, pItem->GetID(), pIn, pOut);

   if(iBase == RFG_MESSAGE)
   {
      // pFolderMessage = (FolderMessageItem *)pMessage;
   }
   else if(iBase == RFG_CHANNEL)
   {
      pChannelMessage = (ChannelMessageItem *)pItem;
   }

   if(iBase != RFG_BULLETIN && pItem != NULL)
   {
      pTree = pItem->GetTree();
      if(pTree != NULL)
      {
         debug("MessageItemEdit tree %p(%ld)\n", pTree, pTree->GetID());
      }
   }

   if(iOp == RQ_ADD || pCurr->GetAccessLevel() >= LEVEL_WITNESS)
   {
      if(iOp == RQ_ADD && iBase == RFG_MESSAGE)
      {
         // Add only fields

         pItem->SetFromID(pCurr->GetID());

         if(pIn->GetChild("fromname", &pFromName) == true)
         {
            bytesprint("MessageItemEdit input fromname", pFromName);

            bPrivilege = PrivilegeValid(pCurr, MSG_MESSAGE_ADD);
            debug("MessageItemEdit %s privilege %s\n", MSG_MESSAGE_ADD, BoolStr(bPrivilege));
            if(bPrivilege == true)
            {
               pItem->SetFromName(pFromName);
            }
            else
            {
               delete pFromName;
               pFromName = NULL;
            }
         }

         pIn->GetChild("toid", &iToID);
         if(UserGet(iToID) != NULL)
         {
            pItem->SetToID(iToID);
         }
         if(pIn->GetChild("toname", &pToName) == true)
         {
            if(pToName->Length() > 0)
            {
               pItem->SetToName(pToName);
               pOut->AddChild("toname", pToName);
            }

            delete pToName;
         }
      }

      if(pIn->GetChild("text", &pText) == true)
      {
         if(iOp == RQ_EDIT)
         {
            pTemp = pItem->GetText();
            pItem->AddText(pTemp);
         }

         pItem->SetText(pText);
         pOut->AddChild("text", pText);

         delete pText;
      }
   }

   if(iBase == RFG_MESSAGE)
   {
		FolderMessageItemEdit(pData, iOp, (FolderMessageItem *)pItem, pIn, pOut, pCurr, pConnData, szRequest, iBase, bRecurse, iSubType, &iMarked, &iOldVoteType);
   }

   // Attachments
   if(iOp == RQ_ADD)
   {
      AttachmentSection(pData, iBase, RQ_ADD, RQ_ADD, pItem, NULL, NULL, false, pIn, pOut, pCurr->GetID(), pCurr->GetAccessLevel(), iSubType);
   }
   else
   {
      AttachmentSection(pData, iBase, RQ_EDIT, RQ_ADD, pItem, NULL, NULL, false, pIn, pOut, pCurr->GetID(), pCurr->GetAccessLevel(), iSubType);
      AttachmentSection(pData, iBase, RQ_EDIT, RQ_DELETE, pItem, NULL, NULL, false, pIn, pOut, pCurr->GetID(), pCurr->GetAccessLevel(), iSubType);
   }

   if(iOp == RQ_ADD || pItem->GetChanged() == true)
   {
      // Always announce a message_add, only announce edit if something has changed

      pAnnounce = new EDF();
      pAnnounce->AddChild(MessageID(iBase), pItem->GetID());
      if(pTree != NULL)
      {
         pAnnounce->AddChild(MessageTreeID(iBase), pTree->GetID());
         pAnnounce->AddChild(MessageTreeName(iBase), pTree->GetName());
      }

      if(iMarked > 0)
      {
         pAnnounce->AddChild("marked", iMarked);
      }

		if(pItem->GetType() > 0)
		{
			pAnnounce->AddChild("msgtype", pItem->GetType());
		}

      if(iBase == RFG_MESSAGE)
		{
			FolderMessageItemAnnounce(pAnnounce, iOp, (FolderMessageItem *)pItem, iOldVoteType);
      }

      if(iOp == RQ_ADD)
      {
         pFrom = UserGet(pItem->GetFromID());
         if(iBase != RFG_CHANNEL || pIn->GetChildBool("nameless") == false)
         {
            pAnnounce->AddChild("fromid", pFrom->GetID());
            if(pItem->GetFromName() != NULL)
            {
               pAnnounce->AddChild("fromname", pItem->GetFromName());
            }
            else
            {
               pAnnounce->AddChild("fromname", pFrom->GetName());
            }
         }

         if(iBase == RFG_MESSAGE)
         {
            pTo = UserGet(pItem->GetToID());
            if(pTo != NULL)
            {
               pAnnounce->AddChild("toid", pTo->GetID());
            }

            pToName = pItem->GetToName();
            if(pToName != NULL)
            {
               pAnnounce->AddChild("toname", pToName);
            }
            else if(pTo != NULL)
            {
               pAnnounce->AddChild("toname", pTo->GetName());
            }

            if(pItem->GetParentID() != -1)
            {
               pAnnounce->AddChild("replyto", pItem->GetParentID());
            }
			}
         else if(iBase == RFG_CHANNEL)
         {
            pText = pChannelMessage->GetText();
            if(pText != NULL)
            {
               pAnnounce->AddChild("text", pText);
            }

            for(iAttachmentNum = 0; iAttachmentNum < pChannelMessage->AttachmentCount(); iAttachmentNum++)
            {
               iAttachmentID = pChannelMessage->AttachmentID(iAttachmentNum);
               pChannelMessage->WriteAttachment(pAnnounce, iAttachmentID, pCurr->GetAccessLevel() >= LEVEL_WITNESS ? EDFITEMWRITE_ADMIN : 0);
            }
         }

         if(iBase != RFG_CHANNEL)
         {
            // bDebug = DBTable::Debug(true);
            // iDebug = debuglevel(DEBUGLEVEL_DEBUG);
            pItem->Insert();
            // DBTable::Debug(bDebug);
            // debuglevel(iDebug);
         }
      }
      else
      {
         pAdmin = new EDF();
         pAdmin->Copy(pAnnounce, true, true);
         // Admin see who edited the message
         pAdmin->AddChild("byid", pCurr->GetID());
         pAdmin->AddChild("byname", pCurr->GetName());

         debug("MessageItemEdit adding edit mark\n");

         if(iBase == RFG_MESSAGE)
         {
            pItem->AddEdit(pCurr->GetID());
         }

         if(iBase != RFG_CHANNEL)
         {
            // bDebug = DBTable::Debug(true);
            // iDebug = debuglevel(DEBUGLEVEL_DEBUG);
            pItem->Update();
            // DBTable::Debug(bDebug);
            // debuglevel(iDebug);
         }
      }

      ServerAnnounce(pData, szRequest, pAnnounce, pAdmin);

      if(iBase == RFG_MESSAGE)
      {
         for(iUserNum = 0; iUserNum < UserCount(); iUserNum++)
         {
            pList = UserList(iUserNum);
            pListConn = ConnectionFind(CF_USERID, pList->GetID());

            // if(mask(pItem->GetStatus(), LOGIN_ON) == false)
            if(pListConn == NULL)
            {
               MessageMarking(pList, NULL, MessageTreeStr(iBase, iOp), (FolderMessageItem *)pItem);
            }
         }
      }

      delete pAnnounce;
      delete pAdmin;
   }

   pOut->AddChild(MessageID(iBase), pItem->GetID());
   if(pTree != NULL)
   {
      pOut->AddChild(MessageTreeID(iBase), pTree->GetID());
      pOut->AddChild(MessageTreeName(iBase), pTree->GetName());
   }

   delete pFromName;

   return true;
}

int MessageTreeSubscribeAnnounce(EDF *pData, char *szRequest, int iBase, MessageTreeItem *pTree, UserItem *pSubscribe, int iSubType, int iExtra, int iCurrType, UserItem *pCurr)
{
   STACKTRACE
   int iReturn = 0;
   EDF *pAnnounce = NULL, *pAdmin = NULL;

   // Send an announcement
   pAnnounce = new EDF();
   pAnnounce->AddChild(MessageTreeID(iBase), pTree->GetID());
   pAnnounce->AddChild(MessageTreeName(iBase), pTree->GetName());
   pAnnounce->AddChild("userid", pSubscribe->GetID());
   pAnnounce->AddChild("username", pSubscribe->GetName());
   // printf("MessageTreeSubscribe subtype check %d %d\n", iSubType, iCurrSub);
   if(iSubType > 0)
   {
      pAnnounce->AddChild("subtype", iSubType);
      if(iExtra > 0)
      {
         pAnnounce->AddChild(iBase == RFG_FOLDER ? "priority" : "active", iExtra);
      }
   }
   else if(iSubType == 0 && iCurrType > 0)
   {
      pAnnounce->AddChild("subtype", iCurrType);
   }

   if(iBase == RFG_FOLDER)
   {
      pAdmin = pAnnounce;
      pAnnounce = NULL;
   }
   else
   {
      pAdmin = new EDF();
      pAdmin->Copy(pAnnounce, true, true);
   }

   if(pCurr != pSubscribe)
   {
      pAdmin->AddChild("byid", pCurr->GetID());
      pAdmin->AddChild("byname", pCurr->GetName());
   }

   iReturn = ServerAnnounce(pData, szRequest, pAnnounce, pAdmin);

   if(pAnnounce != pAdmin)
   {
      delete pAnnounce;
   }
   delete pAdmin;

   return iReturn;
}

// EDF message functions
extern "C"
{

/*
** MessageTreeAdd: Create a new folder / channel
**
** name(required)
** parentid:         ID of the parent (created at the top level if this is not specified)
** accessmode:       Access restrictions (ACCMODE_NORMAL is the default setting)
** accesslevel:      Lowest level required to access (no default setting)
*/
ICELIBFN bool MessageTreeAdd(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   STACKTRACE
   int iBase = RequestGroup(pIn), iParentID = -1;
   long lMaxID = 0;
   char *szName = NULL;
   MessageTreeItem *pTree = NULL, *pParent = NULL;
   UserItem *pCurr = CONNUSER;

   EDFParser::debugPrint("MessageTreeAdd entry", pIn);

   if(pIn->GetChild("name", &szName) == false)
   {
      // No name field
      pOut->Set("reply", MessageTreeStr(iBase, "not_exist"));

      debug("MessageTreeAdd exit false, %s (no name)\n", MSG_FOLDER_INVALID);
      return false;
   }

   if(NameValid(szName) == false)
   {
      // Invalid name
      pOut->Set("reply", MessageTreeStr(iBase, "invalid"));
      pOut->AddChild("name", szName);

      delete[] szName;

      debug("MessageTreeAdd exit false, %s (invalid name)\n", MessageTreeStr(iBase, "invalid"));
      return false;
   }

   if(pIn->GetChild("parentid", &iParentID) == true)
   {
      // Find parent
      pParent = MessageTreeGet(iBase, iParentID);
      if(pParent == NULL)
      {
         pOut->Set("reply", MessageTreeStr(iBase, "not_exist"));
         pOut->AddChild(MessageTreeID(iBase), iParentID);

         delete[] szName;

         debug("MessageTreeAdd exit false, %s (parent not exist)\n", MessageTreeStr(iBase, "not_exist"));
         return false;
      }
   }

   // Create the folder / channel
   if(iBase == RFG_FOLDER)
   {
      lMaxID = FolderMaxID();
   }
   else if(iBase == RFG_CHANNEL)
   {
      lMaxID = TalkMaxID();
   }
   lMaxID++;
   pTree = new MessageTreeItem(MessageType(iBase), lMaxID, pParent, false);
   // debug("MessageTreeAdd MessageTreeItem class %s\n", pTree->GetClass());
   if(iBase == RFG_FOLDER)
   {
      FolderAdd(pTree);
   }
   else if(iBase == RFG_CHANNEL)
   {
      ChannelAdd(pTree);
   }
   pTree->SetName(szName);
   if(pIn->IsChild("accessmode") == false)
   {
      if(iBase == RFG_FOLDER)
      {
         pTree->SetAccessMode(FOLDERMODE_NORMAL);
      }
      else
      {
         pTree->SetAccessMode(CHANNELMODE_NORMAL);
      }
   }

   delete[] szName;

   // Set initial fields
   MessageTreeItemEdit(pData, RQ_ADD, pTree, pIn, pOut, pCurr, iBase);

   EDFParser::debugPrint("MessageTreeAdd exit true", pOut);
   return true;
}

/*
** MessageTreeDelete: Delete a folder / channel
**
** folderid(required)
*/
ICELIBFN bool MessageTreeDelete(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   STACKTRACE
   int iBase = RequestGroup(pIn), iID = 0;
   EDF *pAnnounce = NULL, *pAdmin = NULL;
   UserItem *pCurr = CONNUSER;
   MessageTreeItem *pTree = NULL;

   EDFParser::debugPrint("MessageTreeDelete entry", pIn);

   if(pIn->GetChild(MessageTreeID(iBase), &iID) == false)
   {
      // No id field
      pOut->Set("reply", MessageTreeStr(iBase, "not_exist"));
      debug("MessageTreeDelete exit false, %s\n", MessageTreeStr(iBase, "not_exist"));
      return false;
   }

   if(MessageTreeAccess(iBase, MTA_MESSAGETREE_EDIT, iID, pCurr, NULL, pOut, &pTree, NULL, NULL) == false)
   {
      // Cannot edit message tree
      return false;
   }

   // Set fields
   if(pTree->GetDeleted() == false)
   {
      pTree->SetDeleted(true);

      pAnnounce = new EDF();
      pAnnounce->AddChild(MessageTreeID(iBase), pTree->GetID());
      pAnnounce->AddChild(MessageTreeName(iBase), pTree->GetName());

      pAdmin = new EDF();
      pAdmin->Copy(pAnnounce, true, true);
      // Admin get who edited the message tree
      pAdmin->AddChild("byid", pCurr->GetID());
      pAdmin->AddChild("byname", pCurr->GetName());

      ServerAnnounce(pData, MessageTreeStr(iBase, RQ_DELETE), pAnnounce, pAdmin);

      delete pAnnounce;
      delete pAdmin;
   }

   EDFParser::debugPrint("MessageTreeEdit exit true", pOut);
   return true;
}

/*
** MessageTreeEdit: Edit a folder / channel
**
** folderid(required)
*/
ICELIBFN bool MessageTreeEdit(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   STACKTRACE
   int iBase = RequestGroup(pIn), iID = 0;
   UserItem *pCurr = CONNUSER;
   MessageTreeItem *pTree = NULL;
   DBSub *pSubs = NULL;

   EDFParser::debugPrint("MessageTreeEdit entry", pIn);

   if(pIn->GetChild(MessageTreeID(iBase), &iID) == false)
   {
      // No id field
      pOut->Set("reply", MessageTreeStr(iBase, "not_exist"));
      debug("MessageTreeEdit exit false, %s\n", MessageTreeStr(iBase, "not_exist"));
      return false;
   }

   if(iBase == RFG_FOLDER)
   {
      pSubs = CONNFOLDERS;
   }
   else if(iBase == RFG_CHANNEL)
   {
      pSubs = CONNCHANNELS;
   }

   if(MessageTreeAccess(iBase, MTA_MESSAGETREE_EDIT, iID, pCurr, pSubs, pOut, &pTree, NULL, NULL) == false)
   {
      // Cannot edit message tree
      return false;
   }

   // Set fields
   MessageTreeItemEdit(pData, RQ_EDIT, pTree, pIn, pOut, pCurr, iBase);

   EDFParser::debugPrint("MessageTreeEdit exit true", pOut);
   return true;
}

/*
** MessageTreeList: Generate a list of folders / channels
**
** [folder|channel]id:   ID of the folder to list (other fields ignored if present)
** searchtype: Type of list to generate
**             0(default) - Subs only, low detail (ID, name, editor, unread, replyid)
**             1 - Subs only, high detail (above + messages, size, creation and expiry)
**             2 - All items, low detail
**             3 - All items, high detail
** startdate: Items created after this date
**
** Specifying a ID will give complete details for that item only (search type 4)
*/
ICELIBFN bool MessageTreeList(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   STACKTRACE
   int iBase = RequestGroup(pIn), iUserID = -1, iAccessLevel = LEVEL_NONE, iTreeID = 0, iTreeNum = 0, iType = 0, iLevel = 0, iStartDate = -1;
   int iField1 = 0, iField2 = 0, iField3 = 0, iListNum = 0, iChannelNum = 0;
   char szSQL[100];
   EDF *pSubInfo = NULL;
   ConnData *pConnData = CONNDATA, *pItemData = NULL;
   UserItem *pCurr = CONNUSER, *pItemUser = NULL;
   MessageTreeItem *pTree = NULL;
   DBSub *pSubs = NULL, *pItemSubs = NULL;
   DBMessageRead *pReads = CONNREADS;
   DBTable *pTable = NULL;
   EDFConn *pItem = NULL;

   pCurr = CONNUSER;
   if(pCurr != NULL)
   {
      iUserID = pCurr->GetID();
      iAccessLevel = pCurr->GetAccessLevel();
      if(iBase == RFG_FOLDER)
      {
         pSubs = CONNFOLDERS;
      }
      else if(iBase == RFG_CHANNEL)
      {
         pSubs = CONNCHANNELS;
      }
   }

   // EDFPrint("MessageTreeList entry", pIn);

   if(pIn->GetChild(MessageTreeID(iBase), &iTreeID) == true)
   {
      // Specific message tree
      if(MessageTreeAccess(iBase, MTA_MESSAGETREE_READ, iTreeID, pCurr, pSubs, pOut, &pTree, NULL, NULL) == false)
      {
         // Cannot access
         return false;
      }

      if(iAccessLevel >= LEVEL_WITNESS || pSubs->SubType(iTreeID) == SUBTYPE_EDITOR)
      {
         // Grant access level fields output to admin and editors
         iLevel |= EDFITEMWRITE_ADMIN;
      }

      pSubInfo = new EDF();

      if(iBase == RFG_CHANNEL)
      {
         for(iListNum = 0; iListNum < ConnCount(); iListNum++)
         {
            pItem = ConnList(iListNum);
            pItemData = (ConnData *)pItem->Data();
            pItemUser = pItemData->m_pUser;
            pItemSubs = pItemData->m_pChannels;
            if(pItemSubs != NULL)
            {
               for(iChannelNum = 0; iChannelNum < pItemSubs->Count(); iChannelNum++)
               {
                  iField1 = pItemSubs->ItemID(iChannelNum);
                  iField2 = pItemSubs->ItemSubType(iChannelNum);
                  iField3 = pItemSubs->ItemExtra(iChannelNum);

                  if(iField1 == iTreeID)
                  {
                     debug("MessageTreeList adding user %d sub %d active %d\n", iField1, iField2, iField3);

                     pSubInfo->Add("user", pItemUser->GetID());
                     pSubInfo->AddChild("subtype", iField2);
                     if(iField3 > 0)
                     {
                        pSubInfo->AddChild("active", iField3);
                     }
                     pSubInfo->Parent();
                  }
               }
            }
         }
      }

      // if(iBase == RFG_CHANNEL || (iBase == RFG_FOLDER && (iAccessLevel >= LEVEL_WITNESS || pSubs->SubType(iTreeID) >= SUBTYPE_MEMBER)))
      {
         // Get full subscription info for admin and editors of restricted items
         pTable = new DBTable();
         pTable->BindColumnBytes();
         pTable->BindColumnInt();
         sprintf(szSQL, "select subtype,userid from %s where %s = %ld", MessageTreeSubTable(iBase), MessageTreeID(iBase), pTree->GetID());
         if(iBase == RFG_CHANNEL)
         {
            sprintf(szSQL, "%s and subtype >= %d", szSQL, SUBTYPE_MEMBER);
         }
			else if(mask(iLevel, EDFITEMWRITE_ADMIN) == false)
			{
				sprintf(szSQL, "%s and subtype = %d", szSQL, SUBTYPE_EDITOR);
			   debug("MessageTreeList non-admin query %s\n", szSQL);
			}
         if(pTable->Execute(szSQL) == true)
         {
            while(pTable->NextRow() == true)
            {
               iField2 = 0;

               iField1 = DBSub::IntType(pTable, 0);
               pTable->GetField(1, &iField2);

               if(SubTypeStr(iField1) != NULL)
               {
                  // printf("MessageTreeList subs s=%d u=%d\n", iField1, iField2);

                  if(EDFFind(pSubInfo, "user", iField2, false) == false)
                  {
                     pSubInfo->Add("user", iField2);
                     pSubInfo->AddChild("subtype", iField1);
                  }

                  pSubInfo->Parent();
               }
            }
         }
         else
         {
            debug("MessageTreeList subs query '%s' failed\n", szSQL);
         }
         delete pTable;
      }

      if(mask(iLevel, EDFITEMWRITE_ADMIN) == false)
      {
         EDFParser::debugPrint("MessageTreeList non admin subs", pSubInfo);
      }

      MessageTreeItemList(pOut, iLevel + MESSAGETREEITEMWRITE_DETAILS + MESSAGETREEITEMWRITE_INFO, pTree, iBase, pData, 0, -1, iUserID, iAccessLevel, pConnData, pSubs, pReads, pSubInfo);

      delete pSubInfo;
   }
   else
   {
      // Get editors info for all users
      pSubInfo = new EDF();

      pTable = new DBTable();
      pTable->BindColumnInt();
      pTable->BindColumnInt();
      sprintf(szSQL, "select %s,userid from %s where subtype='E' order by %s", MessageTreeID(iBase), MessageTreeSubTable(iBase), MessageTreeID(iBase));
      if(pTable->Execute(szSQL) == true)
      {
         while(pTable->NextRow() == true)
         {
            iField1 = 0;
            iField2 = 0;

            pTable->GetField(0, &iField1);
            pTable->GetField(1, &iField2);

            // printf("MessageTreeList editor f=%d u=%d\n", iField1, iField2);

            pSubInfo->Add(SubTypeStr(SUBTYPE_EDITOR), iField1);
            pSubInfo->AddChild("userid", iField2);
            pSubInfo->Parent();
         }

         // EDFPrint("MessageTreeList editor subs", pSubs);
      }
      else
      {
         debug("MessageTreeList subs query '%s' failed\n", szSQL);
      }
      delete pTable;

      pIn->GetChild("searchtype", &iType);
      if(iType < 0 || iType > 3)
      {
         iType = 0;
      }
      if(iType == 1 || iType == 3)
      {
         iLevel |= MESSAGETREEITEMWRITE_DETAILS;
         if(iAccessLevel >= LEVEL_WITNESS)
         {
            iLevel |= EDFITEMWRITE_ADMIN;
         }
      }
      pIn->GetChild("startdate", &iStartDate);
      /* if(iStartDate != -1)
      {
         debugEDFPrint("MessageTreeList startdate", pIn);
      } */

      // Iterate over top level trees
      for(iTreeNum = 0; iTreeNum < MessageTreeCount(iBase, true); iTreeNum++)
      {
         pTree = MessageTreeList(iBase, iTreeNum, true);
         // printf("MessageTreeList tree item %d -> %p(%s)\n", iTreeNum, pTree, pTree->GetName());
         MessageTreeItemList(pOut, iLevel, pTree, iBase, pData, iType == 0 || iType == 1 || iStartDate != -1 ? 2 : 1, iStartDate, iUserID, iAccessLevel, pConnData, pSubs, pReads, pSubInfo);
      }

      delete pSubInfo;
   }

   pOut->Root();
   pOut->AddChild(MessageTreeNum(iBase), MessageTreeCount(iBase));

   if(iBase == RFG_CHANNEL)// || iStartDate != -1)
   {
      EDFParser::debugPrint("MessageTreeList exit true", pOut);
   }
   return true;
}

/*
** MessageTreeSubscribe: Subscribe to a folder / channel
**
** [folder|channel]id(required)
** userid(admin):  ID of the user to subscribe
** subtype: Type of subscription
**
** subtype defaults to SUBTYPE_SUB if the subscribe request is used and 0 if unsubscribe is used
*/
ICELIBFN bool MessageTreeSubscribe(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   STACKTRACE
   int iBase = RequestGroup(pIn), iSubType = 0, iAccessLevel = LEVEL_NONE, iUserID = -1, iID = 0, iExtra = 0, iNumEds = 0, iDebug = 0, iCurrType = 0;
   bool bSuccess = false, bTemp = false;
   char *szRequest = NULL;
   EDFConn *pItemConn = NULL;
   ConnData *pConnData = CONNDATA, *pItemData = NULL;
   UserItem *pCurr = CONNUSER, *pItem = NULL;
   MessageTreeItem *pTree = NULL;
   DBSub *pSubs = NULL;
   DBSubData *pSubData = NULL;

   iAccessLevel = pCurr->GetAccessLevel();

   if(iBase == RFG_FOLDER)
   {
      pSubs = CONNFOLDERS;
   }
   else if(iBase == RFG_CHANNEL)
   {
      pSubs = CONNCHANNELS;
   }

   pIn->Get(NULL, &szRequest);
   debug("MessageTreeSubscribe entry %s\n", szRequest);
   EDFParser::debugPrint(pIn, EDFElement::EL_CURR | EDFElement::PR_SPACE);

   // Get the ID
   if(pIn->GetChild(MessageTreeID(iBase), &iID) == false)
   {
      // No ID supplied
      pOut->Set("reply", MessageTreeStr(iBase, "not_exist"));

      delete[] szRequest;

      return false;
   }

   if(MessageTreeAccess(iBase, MTA_MESSAGETREE_READ, iID, pCurr, pSubs, pOut, &pTree, NULL, NULL) == false)
   {
      // Cannot access

      delete[] szRequest;

      return false;
   }

   if(iAccessLevel >= LEVEL_WITNESS || (mask(pTree->GetAccessMode(), ACCMODE_MEM_READ) == true && pSubs->SubType(iID) == SUBTYPE_EDITOR))
   {
      // Admin / restricted folder editors allowed to specify a user
      pIn->GetChild("userid", &iUserID);
   }

   if(iUserID != -1)
   {
      // Find specied user
      pItem = UserGet(iUserID);
      if(pItem == NULL)
      {
         // User does not exist
         pOut->Set("reply", MSG_USER_NOT_EXIST);
         pOut->SetChild("userid", iUserID);

         delete[] szRequest;

         debug("MessageTreeSubscribe exit false, %s %d\n", MSG_USER_NOT_EXIST, iUserID);
         return false;
      }

      // Check access again
      if(pItem->GetAccessLevel() >= iAccessLevel)
      {
         // Trying to change a user higher than the current one
         pOut->Set("reply", MessageTreeStr(iBase, "access_invalid"));

         delete[] szRequest;

         debug("MessageTreeSubscribe exit false, %s\n", MSG_ACCESS_INVALID);
         return false;
      }

      // Find this user's connection data
      pItemConn = ConnectionFind(CF_USERID, iUserID);

      if(pItemConn != NULL)
      {
         pItemData = (ConnData *)pItemConn->Data();
         if(iBase == RFG_FOLDER)
         {
            pSubs = pItemData->m_pFolders;
         }
         else if(iBase == RFG_CHANNEL)
         {
            pSubs = pItemData->m_pChannels;
         }

         pSubData = pSubs->SubData(iID);
      }
      else
      {
         pSubs = NULL;

	 iDebug = debuglevel(DEBUGLEVEL_DEBUG);
         pSubData = DBSub::UserData(MessageTreeSubTable(iBase), MessageTreeID(iBase), iUserID, iID);
	 debuglevel(iDebug);
      }
   }
   else
   {
      // Subscription for current user
      pItem = pCurr;
      pItemData = pConnData;

      pSubData = pSubs->SubData(iID);
      debug("MessageTreeSubscribe current user %p, %d -> %p\n", pSubs, iID, pSubData);
   }

   if(pSubData != NULL)
   {
      iCurrType = pSubData->m_iSubType;
      debug("MessageTreeSubscribe curr type %d\n", iCurrType);
   }

   debug("MessageTreeSubscribe %s %d, %s %ld %s %p", szRequest, iID, pItem == pCurr ? "self" : "user", pItem->GetID(), pItem->GetName(), pSubData);
   if(pSubData != NULL)
   {
      debug("(%d %d)", pSubData->m_iSubType, pSubData->m_iExtra);
   }
   debug("\n");

   if(stricmp(szRequest, MSG_FOLDER_SUBSCRIBE) == 0 || stricmp(szRequest, MSG_CHANNEL_SUBSCRIBE) == 0)
   {
      // Subscribe
      if(pIn->GetChild("subtype", &iSubType) == true)
      {
         if(iAccessLevel >= LEVEL_WITNESS)
         {
            if(iSubType < SUBTYPE_SUB || iSubType > SUBTYPE_EDITOR)
            {
               iSubType = SUBTYPE_SUB;
            }
         }
         else if(mask(pTree->GetAccessMode(), ACCMODE_SUB_READ) == false && mask(pTree->GetAccessMode(), ACCMODE_MEM_READ) == true)
         {
            iSubType = SUBTYPE_MEMBER;
         }
         else if(iSubType != SUBTYPE_SUB)
         {
            iSubType = SUBTYPE_SUB;
         }

         if(pSubData != NULL)
         {
            if(pSubData->m_iSubType > iSubType)
            {
               iSubType = pSubData->m_iSubType;
            }
         }
      }
      else
      {
         if(pSubData != NULL)
         {
            iSubType = pSubData->m_iSubType;
         }
         else
         {
            iSubType = SUBTYPE_SUB;
         }
      }

      if(stricmp(szRequest, MSG_FOLDER_SUBSCRIBE) == 0)
      {
         if(pIn->GetChild("priority", &iExtra) == false && pSubData != NULL)
         {
            iExtra = pSubData->m_iExtra;
         }
         bTemp = pIn->GetChildBool("temp");
      }
      else if(stricmp(szRequest, MSG_CHANNEL_SUBSCRIBE) == 0)
      {
         if(pIn->GetChildBool("active") == true)
         {
            iExtra = 1;
         }
      }

      // bDebug = DBTable::Debug(true);
      iDebug = debuglevel(DEBUGLEVEL_DEBUG);
      if(pSubs != NULL)
      {
         /* if(iBase == RFG_FOLDER)
         {
            debug("MessageTreeSubscribe current eds %d\n", pItemData->m_pFolders->CountType(SUBTYPE_EDITOR) + pItemData->m_pChannels->CountType(SUBTYPE_EDITOR));
         } */

         // Update connection
         bSuccess = pSubs->Update(iID, iSubType, iExtra, bTemp);
      }
      else
      {
         if(iCurrType == 0)
         {
            // Add to database
            bSuccess = DBSub::UserAdd(MessageTreeSubTable(iBase), MessageTreeID(iBase), pItem->GetID(), iID, iSubType, iExtra);
         }
         else
         {
            // Update database
            bSuccess = DBSub::UserUpdate(MessageTreeSubTable(iBase), MessageTreeID(iBase), pItem->GetID(), iID, iSubType, iExtra);
         }
      }
      // DBTable::Debug(bDebug);
      debuglevel(iDebug);

      if(iBase == RFG_FOLDER && mask(pItem->GetUserType(), USERTYPE_AGENT) == false && iSubType == SUBTYPE_EDITOR)
      {
         debug("MessageTreeSubscribe editor access check %d < %d\n", pItem->GetAccessLevel(), LEVEL_EDITOR);
         if(pItem->GetAccessLevel() < LEVEL_EDITOR)
         {
            // Promote user
            pItem->SetAccessLevel(LEVEL_EDITOR);
         }
      }
   }
   else if(iCurrType > 0)
   {
      // Unsubcribe
      debug("MessageTreeSubscribe unsubscroibe\n");
      iDebug = debuglevel(DEBUGLEVEL_DEBUG);

      if(pSubs != NULL)
      {
         // Remove from connection
	 debug("MessageTreeSuscribe remove from connection\n");

         // printf("MessageTreeSubscribe current eds %d\n", pSubs->CountType(SUBTYPE_EDITOR));

         pSubs->Delete(iID);

         if(mask(pItem->GetUserType(), USERTYPE_AGENT) == false && pItem->GetAccessLevel() == LEVEL_EDITOR)
         {
            iNumEds = pItemData->m_pFolders->CountType(SUBTYPE_EDITOR) + pItemData->m_pChannels->CountType(SUBTYPE_EDITOR);
         }
      }
      else
      {
         // Remove from database
	 debug("MessageTreeSubscribe remove from database\n");

         DBSub::UserDelete(MessageTreeSubTable(iBase), MessageTreeID(iBase), pItem->GetID(), iID);

         if(iBase == RFG_FOLDER && mask(pItem->GetUserType(), USERTYPE_AGENT) == false && pItem->GetAccessLevel() == LEVEL_EDITOR)
         {
            iNumEds = DBSub::CountType(MessageTreeSubTable(RFG_FOLDER), pItem->GetID(), SUBTYPE_EDITOR) +
                      DBSub::CountType(MessageTreeSubTable(RFG_CHANNEL), pItem->GetID(), SUBTYPE_EDITOR);
         }
      }

      if(mask(pItem->GetUserType(), USERTYPE_AGENT) == false && pItem->GetAccessLevel() == LEVEL_EDITOR)
      {
         debug("MessageTreeSubscribe editor access check %d <= %d && %d == 0\n", pItem->GetAccessLevel(), LEVEL_EDITOR, iNumEds);

         if(iNumEds == 0)
         {
            // Demote user
            pItem->SetAccessLevel(LEVEL_MESSAGES);
         }
      }

      bSuccess = true;

      debuglevel(iDebug);
   }

   if(bSuccess == true)
   {
      MessageTreeSubscribeAnnounce(pData, szRequest, iBase, pTree, pItem, iSubType, iExtra, iCurrType, pCurr);
   }

   if(pCurr != pItem)
   {
      // Add user info if not the current user
      pOut->AddChild("userid", pItem->GetID());
      pOut->AddChild("username", pItem->GetName());
   }
   pOut->AddChild(MessageTreeID(iBase), pTree->GetID());
   pOut->AddChild(MessageTreeName(iBase), pTree->GetName());

   if(iSubType > 0)
   {
      // Current subtype
      pOut->AddChild("subtype", iSubType);
      if(iExtra > 0)
      {
         pOut->AddChild(iBase == RFG_FOLDER ? "priority" : "active", iExtra);
      }
   }
   else if(iSubType == 0 && iCurrType > 0)
   {
      // Previous subtype
      pOut->AddChild("subtype", iCurrType);
   }

   delete[] szRequest;

   EDFParser::debugPrint("MessageTreeSubscribe exit true", pOut);
   return true;
}

}
