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
** RqFolder.cpp: Implementation of server side folder request functions
*/

#include <stdio.h>
#include <errno.h>
#include <string.h>
#ifdef UNIX
#include <strings.h>
#endif
#include <sys/types.h>
#include <time.h>
#include <stdlib.h>
#include <ctype.h>

// Framework headers
#include "headers.h"
#include "ICE/ICE.h"

// Common headers
#include "Folder.h"
#include "User.h"

// Library headers
#include "server.h"

#include "RqTree.h"
#include "RqFolder.h"
#include "RqUser.h"
#include "RqSystem.h"

#define MATCH_OR 1
#define MATCH_AND 2
#define MATCH_NOT 4

#define FMLS_FOLDERINFO 256

#define FMIL_SINGLE 0 // Single message mode
#define FMIL_SEARCH 1 // Multi message search mode
#define FMIL_FOLDER 2 // Multi message tree mode

MessageTreeItem *g_pBulletinList = NULL;

bytes *MessageSQLItem(EDF *pEDF, int iIndent);

bool MessageVoteClose(EDF *pData, int iBase, FolderMessageItem *pItem, bool bAnnounce);

bool CrossFolder(int iFolderID, MessageTreeItem *pChild)
{
   MessageTreeItem *pParent = FolderGet(iFolderID);
   
   if(pParent == pChild)
   {
      return true;
   }
   
   debug("CrossFolder %p(%d %d) %p(%d)", pParent, pParent->GetID(), pParent->GetReplyID(), pChild, pChild->GetID());
   if(pParent->GetReplyID() != -1 && pParent->GetReplyID() == pChild->GetID())
   {
      debug(" -> true (reply ID)\n");
      return true;
   }

   debug(" -> false\n");
   return false;
}

bool FolderReadItemDB(EDF *pData, MessageTreeItem *pFolder, int iExpire)
{
   STACKTRACE
   int iNumMsgs = 0, iNumAdds = 0, iTempExpire = 0;
   bool bAdd = false;
   char szSQL[100];
   double dTick = gettick();
   bytes *pSubject = NULL;
   DBTable *pTable = NULL;
   MessageTreeItem *pTemp = NULL;
   FolderMessageItem *pFolderMessage = NULL;

   debug("FolderReadItemDB entry %p %ld\n", pFolder, iExpire);

   pTable = new DBTable();

   sprintf(szSQL, "select * from %s", FMI_TABLENAME);
   if(pFolder != NULL)
   {
      sprintf(szSQL, "%s where folderid = %ld", szSQL, pFolder->GetID());
   }
   else if(iExpire > 0)
   {
      sprintf(szSQL, "%s where message_date >= %ld", szSQL, time(NULL) - iExpire);
   }
   debug("FolderReadItemDB SQL: %s\n", szSQL);
   if(pTable->Execute(szSQL) == false)
   {
      delete pTable;

      debug("FolderReadItemDB exit false, SQL failed\n");
      return false;
   }

   debug("FolderReadItemDB checking message rows\n");
   // while(pTable->NextRow() == true)
   do
   {
      pFolderMessage = FolderMessageItem::NextRow(pTable, NULL, NULL);

      if(pFolderMessage != NULL)
      {
         bAdd = false;
         if(pFolder != NULL)
         {
            bAdd = true;
         }
         else
         {
            pTemp = FolderGet(pFolderMessage->GetTreeID());
            iTempExpire = FolderExpire(pData, pTemp, pFolderMessage);
            if(pFolderMessage->GetDate() >= time(NULL) - iTempExpire)
            {
               bAdd = true;
            }
         }

         pSubject = pFolderMessage->GetSubject();
         debug("FolderReadItemDB %s %ld. %ld(%p) %ld %ld", bAdd == true ? "adding" : "rejecting",
            pFolderMessage->GetID(), pFolderMessage->GetTreeID(), pFolder != NULL ? pFolder : pTemp,
            pFolderMessage->GetDate(), pFolderMessage->GetExpire());
         if(pSubject != NULL)
         {
            debug(" %s", pSubject->Data(false));
         }
         debug("\n");

         if(bAdd == true)
         {
            FolderMessageAdd(pFolderMessage);
            iNumAdds++;
         }
	 else
	 {
	    delete pFolderMessage;
	 }

         iNumMsgs++;
      }
   }
   while(pFolderMessage != NULL);

   debug("FolderReadItemDB exit, %d of %d messages in %ld ms\n", iNumAdds, iNumMsgs, tickdiff(dTick));
   return true;
}

// FolderReadDB: Read folders / messages from the database and populate the folder / message lists
bool FolderReadDB(EDF *pData)
{
   STACKTRACE
   int iNumFolders = 0, iFolderNum = 0, iMaxExpire = 0;
   // bool bDebug = false;
   double dTick = gettick();
   DBTable *pTable = NULL;
   MessageTreeItem *pFolder = NULL;

   debug("FolderReadDB entry\n");

   // pTable = FolderMessageItem::ReadFields();
   pTable = new DBTable();

   // Get rows
   if(pTable->Execute("select * from folder_item") == false)
   {
      debug("FolderReadDB exit false, folder query failed\n");
      delete pTable;

      return false;
   }

   debug("FolderReadDB checking folder rows\n");
   // while(pTable->NextRow() == true)
   do
   {
      pFolder = MessageTreeItem::NextRow("folder", pTable, NULL, false);

      if(pFolder != NULL)
      {
         iNumFolders++;

         // pFolder = new MessageTreeItem("folder", pTable, NULL, false);
         debug("FolderReadDB folder %ld, %s\n", pFolder->GetID(), pFolder->GetName());

         /* EDF *pEDF = new EDF();
         pFolder->Write(pEDF, -1);
         pEDF->Root();
         pEDF->Child();
         debugEDFPrint(pEDF);
         delete pEDF; */

         FolderAdd(pFolder);
      }
   }
   while(pFolder != NULL);

   delete pTable;

   // Get system-wide expiry first
   iMaxExpire = SystemExpire(pData);

   // Do the expiring folders first
   for(iFolderNum = 0; iFolderNum < FolderCount(); iFolderNum++)
   {
      pFolder = FolderList(iFolderNum);

      if(pFolder->GetExpire() > 0 && pFolder->GetExpire() > iMaxExpire)
      {
         iMaxExpire = pFolder->GetExpire();
      }
   }

   FolderReadItemDB(pData, NULL, iMaxExpire);

   // Now do the non expiring folders
   for(iFolderNum = 0; iFolderNum < FolderCount(); iFolderNum++)
   {
      pFolder = FolderList(iFolderNum, false);
      if(pFolder->GetExpire() == -1)
      {
         FolderReadItemDB(pData, pFolder, 0);
      }
   }

   debug("FolderReadDB exit true, %ld ms\n", tickdiff(dTick));
   return true;
}

// FolderWriteDB: Write modified folders / messages to the database
bool FolderWriteDB(bool bFull)
{
   STACKTRACE
   int iFolderNum = 0, iMessageNum = 0, iDebug = 0;
   // bool bDebug = false;
   double dTick = gettick(), dSingle = gettick();
   char szSQL[100];
   MessageTreeItem *pFolder = NULL;
   FolderMessageItem *pFolderMessage = NULL;
   DBTable *pTable = NULL;

   debug("FolderWriteDB entry\n");

   // bDebug = DBTable::Debug(true);
   iDebug = debuglevel(DEBUGLEVEL_DEBUG);

   if(bFull == true)
   {
      // Full write - delete current contents
      pTable = new DBTable();
      if(pTable->Execute("delete from folder_item") == false)
      {
         delete pTable;

         // DBTable::Debug(bDebug);
         debuglevel(iDebug);

         debug("FolderWriteDB exit false, delete query failed\n");
         return false;
      }
      delete pTable;
   }

   for(iFolderNum = 0; iFolderNum < FolderCount(); iFolderNum++)
   {
      dSingle = gettick();
      pFolder = FolderList(iFolderNum);
      if(bFull == true)
      {
         debug("FolderWriteDB inserting %ld\n", pFolder->GetID());
         pFolder->Insert();
         debug("FolderWriteDB inserted %ld ms\n", tickdiff(dSingle));
      }
      else if(pFolder->GetChanged() == true)
      {
         debug("FolderWriteDB updating %ld\n", pFolder->GetID());
         pFolder->Update();
         debug("FolderWriteDB inserted %ld ms\n", tickdiff(dSingle));
      }
   }

   if(bFull == true)
   {
      // Full write - delete current contents
      pTable = new DBTable();
      sprintf(szSQL, "delete from %s", FMI_TABLENAME);
      if(pTable->Execute(szSQL) == false)
      {
         delete pTable;

         // DBTable::Debug(bDebug);
         debuglevel(iDebug);

         debug("MessageWriteDB exit false, delete query failed\n");
         return false;
      }
      delete pTable;
   }

   for(iMessageNum = 0; iMessageNum < FolderMessageCount(); iMessageNum++)
   {
      dSingle = gettick();
      pFolderMessage = FolderMessageList(iMessageNum);
      if(bFull == true)
      {
         debug("MessageWriteDB inserting %ld\n", pFolderMessage->GetID());
         pFolderMessage->Insert();
         debug("MessageWriteDB inserted %ld ms\n", tickdiff(dSingle));
      }
      else if(pFolderMessage->GetChanged() == true)
      {
         debug("MessageWriteDB updating %ld\n", pFolderMessage->GetID());
         pFolderMessage->Update();
         debug("MessageWriteDB inserted %ld ms\n", tickdiff(dSingle));
      }
   }

   // DBTable::Debug(bDebug);
   debuglevel(iDebug);

   debug("FolderWriteDB exit true %ld ms\n", tickdiff(dTick));
   return true;
}

// FolderSetup: Setup folder / message hierarchy, total stats, perform validation
bool FolderSetup(EDF *pData, int iOptions, bool bValidate)
{
   STACKTRACE
   int iFolderNum = 0, iMessageNum = 0;
   MessageTreeItem *pFolder = NULL;//, *pReply = NULL;
   FolderMessageItem *pFolderMessage = NULL, *pParent = NULL;
   // UserItem *pProvider = NULL;

   debug("FolderSetup folders...\n");
   debug("FolderSetup count %d(list) / %d (tree)\n", FolderCount(), FolderCount(true));
   for(iFolderNum = 0; iFolderNum < FolderCount(); iFolderNum++)
   {
      pFolder = FolderList(iFolderNum);

      debug("FolderSetup folder %ld, %s %ld\n", pFolder->GetID(), pFolder->GetName(), pFolder->GetParentID());

      if(NameValid(pFolder->GetName()) == false)
      {
         // Disable invalid folders
         debug("FolderSetup warning: Invalid folder name, setting access mode to %d\n", 0);
         pFolder->SetAccessMode(0);
      }

      if(pFolder->GetParentID() != -1)
      {
         // Build folder hierarchy
         FolderSetParent(pFolder, pFolder->GetParentID());
      }

      /* if(pFolder->GetReplyID() != -1)
      {
         pReply = FolderGet(pFolder->GetReplyID());
         if(pReply != NULL)
         {
            pFolder->SetReply(pReply);
         }
      } */

      /* if(pFolder->GetProviderID() != -1)
      {
         pProvider = UserGet(pFolder->GetProviderID());
         if(pProvider != NULL)
         {
            pFolder->SetProvider(pProvider);
         }
      } */

      pFolder->SetChanged(false);
   }

   debug("FolderSetup messages...\n");
   for(iMessageNum = 0; iMessageNum < FolderMessageCount(); iMessageNum++)
   {
      pFolderMessage = FolderMessageList(iMessageNum);

      debug("FolderSetup message %ld, p=%ld f=%ld\n", pFolderMessage->GetID(), pFolderMessage->GetParentID(), pFolderMessage->GetTreeID());

      if(pFolderMessage->GetParentID() != -1)
      {
         // Build message hierarchy
         pParent = FolderMessageGet(pFolderMessage->GetParentID());
         if(pParent != NULL)
         {
            pFolderMessage->SetParent(pParent);
            pParent->Add(pFolderMessage);
         }
         else
         {
            debug("FolderSetup warning: Cannot find message parent %ld\n", pFolderMessage->GetParentID());
         }
      }
      else
      {
         pParent = NULL;
      }

      if(pFolderMessage->GetTreeID() == 0)
      {
         debug("FolderSetup adding %ld to bulletin list\n", pFolderMessage->GetID());

         pFolderMessage->SetTree(g_pBulletinList);
         g_pBulletinList->MessageAdd(pFolderMessage);
      }
      else if(pFolderMessage->GetTreeID() != -1)
      {
         pFolder = FolderGet(pFolderMessage->GetTreeID());
         if(pFolder != NULL)
         {
            // Assigned message to a folder
            pFolderMessage->SetTree(pFolder);

            if(pParent == NULL || pParent->GetTreeID() != pFolder->GetID())
            {
               debug("FolderSetup adding to folder %ld\n", pFolder->GetID());
               pFolder->MessageAdd(pFolderMessage);
            }
            /* else if(pParent != NULL && pParent->GetTree() != pFolder)
            {
               debug("FolderSetup adding to folder %ld (parent folder %ld)\n", pFolder->GetID(), pParent->GetTree()->GetID());
               pFolder->MessageAdd(pFolderMessage);
            } */

            debug("FolderSetup now %d top level messages\n", pFolder->MessageCount(false));
         }
         else
         {
            debug("FolderSetup warning: Cannot find message folder %ld\n", pFolderMessage->GetTreeID());
         }
      }

      pFolderMessage->SetChanged(false);
   }

   // Summary report
   debug("FolderSetup folders: total %d (tree total %d), max ID %ld\n", FolderCount(), FolderCount(), FolderMaxID());
   for(iFolderNum = 0; iFolderNum < FolderCount(); iFolderNum++)
   {
      pFolder = FolderList(iFolderNum);
      debug("  folder %ld(%s, parent %p), %ld messages of %ld total, %d children\n", pFolder->GetID(), pFolder->GetName(), pFolder->GetParent(), pFolder->GetNumMsgs(), pFolder->GetTotalMsgs(), pFolder->Count(false));
   }

   debug("  bulletins, %ld of %ld total\n", g_pBulletinList->GetNumMsgs(), g_pBulletinList->GetTotalMsgs());

   return true;
}

int FolderExpire(EDF *pData, MessageTreeItem *pFolder, FolderMessageItem *pMessage)
{
   STACKTRACE
   int iSysExpire = -1, iReturn = -1;

   iSysExpire = SystemExpire(pData);

   if(pMessage != NULL && pMessage->GetExpire() > 0)
   {
      iReturn = pMessage->GetExpire();
   }
   else if(pFolder != NULL)
   {
      if(pFolder->GetExpire() > 0)
      {
         iReturn = pFolder->GetExpire();
      }
      else if(pFolder->GetExpire() == 0)
      {
         iReturn = iSysExpire;
      }
   }

   return iReturn;
}

void FolderMaintenanceBG(EDFConn *pConn, EDF *pData)
{
   STACKTRACE
   int iMessageNum = 0;
   FolderMessageItem *pItem = NULL;

   for(iMessageNum = 0; iMessageNum < FolderMessageCount(); iMessageNum++)
   {
      pItem = FolderMessageList(iMessageNum);

      if(mask(pItem->GetType(), MSGTYPE_VOTE) == true && mask(pItem->GetVoteType(), VOTE_CLOSED) == false && pItem->GetVoteClose() != -1 && time(NULL) > pItem->GetVoteClose())
      {
         MessageVoteClose(pData, RFG_MESSAGE, pItem, true);
      }
   }
}

void FolderMaintenance(EDFConn *pConn, EDF *pData)
{
   STACKTRACE
   int iAge = 0, iMessageNum = 0, iListNum = 0, iExpire = -1, iDebug = 0;
   long lParentID = 0, lMessageID = 0;
   bool bDelete = false, bChanged = false;
   FolderMessageItem *pFolderMessage = NULL, *pParent = NULL;
   MessageTreeItem *pFolder = NULL;
   UserItem *pFrom = NULL;
   EDFConn *pListConn = NULL;
   ConnData *pListData = NULL;

   debug("FolderMaintenance entry %p %p\n", pConn, pData);

   FolderMaintenanceBG(pConn, pData);

   // Flatten the tree
   for(iMessageNum = 0; iMessageNum < FolderMessageCount(); iMessageNum++)
   {
      pFolderMessage = FolderMessageList(iMessageNum);

      bChanged = pFolderMessage->GetChanged();
      pParent = (FolderMessageItem *)pFolderMessage->GetParent();
      if(pParent != NULL)
      {
         pFolderMessage->SetParent(NULL);
         pFolderMessage->SetParentID(pParent->GetID());
      }
      else if(pFolderMessage->GetParentID() != -1)
      {
         debug("FolderMaintenance parent ID %ld\n", pFolderMessage->GetParentID());
      }

      while(pFolderMessage->Count(false) > 0)
      {
         pFolderMessage->ItemDelete(0);
      }

      if(bChanged == false)
      {
         pFolderMessage->SetChanged(false);
      }
   }

   debug("FolderMaintenance pre-check %d messages\n", FolderMessageCount());

   iMessageNum = 0;
   // for(iMessageNum = 0; iMessageNum < pChecks->Count(); iMessageNum++)
   while(iMessageNum < FolderMessageCount())
   {
      bDelete = false;
      pFolderMessage = FolderMessageList(iMessageNum);
      lMessageID = pFolderMessage->GetID();

      // printf("FolderMaintenance %d, %p (%ld)\n", iMessageNum, pFolderMessage, lMessageID);

      pFolder = pFolderMessage->GetTree();

      if(pFolder != NULL)
      {
         iAge = time(NULL) - pFolderMessage->GetDate();
         iExpire = FolderExpire(pData, pFolder);
         // printf("FolderMaintenance age %d, expire %ld\n", iAge, pFolder->GetExpire());
         // printf("FolderMaintenance %d %p (%s), %d > %d ?\n", pFolderMessage->GetType(), pFolder, pFolder->GetName(), iAge, pFolder->GetExpire());

         if((iExpire != -1 && iAge > iExpire) || (pFolderMessage->GetExpire() != -1 && time(NULL) > pFolderMessage->GetExpire()))
         {
            // if(mask(pFolderMessage->GetType(), MSGTYPE_VOTE) == false || iAge > 2 * iExpire)
            {
               debug("FolderMaintenance %ld (%d / %s) in %s, %g days old marked for deletion\n", pFolderMessage->GetID(), pFolderMessage->GetType(), pFolderMessage->GetSubject() != NULL ? pFolderMessage->GetSubject()->Data(false) : NULL, pFolder->GetName(), iAge / 86400.0);

               // Add to archive table, delete from current table
               // pFolderMessage->Insert(FM_ARCHIVE_TABLE);
               // pFolderMessage->Delete((char *)NULL);

               for(iListNum = 0; iListNum < ConnCount(); iListNum++)
               {
                  pListConn = ConnList(iListNum);
                  pListData = (ConnData *)pListConn->Data();
                  if(pListData->m_pReads != NULL)
                  {
                     pListData->m_pReads->Delete(pFolderMessage->GetID());
                  }
               }

               DBMessageRead::UserDelete(-1, pFolderMessage->GetID());

               pFrom = UserGet(pFolderMessage->GetFromID());
               if(pFrom != NULL)
               {
                  if((mask(pFolder->GetAccessMode(), ACCMODE_PRIVATE) == true && mask(pFrom->GetArchiving(), ARCHIVE_PRIVATE) == false) ||
                     (mask(pFolder->GetAccessMode(), ACCMODE_PRIVATE) == false && mask(pFrom->GetArchiving(), ARCHIVE_PUBLIC) == false))
                  {
                     debug("FolderMaintenance removing %ld(in %s from %s) from database\n", pFolderMessage->GetID(), pFolder->GetName(), pFrom->GetName());
                     pFolderMessage->Delete();
                  }
               }

               bDelete = true;
            }
            /* else
            {
               // if(mask(pFolderMessage->GetVoteType(), VOTE_CLOSED) == false)
               {
                  // printf("FolderMaintenance %ld (%s) in %s is a vote. Now closing\n", pFolderMessage->GetID(), pFolderMessage->GetSubject() != NULL ? pFolderMessage->GetSubject()->Data(false) : NULL, pFolder->GetName());
                  MessageVoteClose(pData, RFG_MESSAGE, pFolderMessage, true);
               }

               // iNumOthers++;
            } */
         }
         /* else
         {
            // printf("FolderMaintenance keeping %ld (%s) in %s\n", pFolderMessage->GetID(), pFolderMessage->GetSubject() != NULL ? pFolderMessage->GetSubject()->Data(false) : NULL, pFolder->GetName());

            // iNumKept++;
         } */
      }
      else
      {
         debug("FolderMaintenance message %ld has no folder\n", pFolderMessage->GetID());

         // iNumOthers++;
      }

      if(bDelete == true)
      {
         debug("FolderMaintenance deleting %p (%ld)\n", pFolderMessage, pFolderMessage->GetID());

         if(pFolder !=  NULL)
         {
            // printf("FolderMaintenance remove from folder %p (%ld %s)\n", pFolder, pFolder->GetID(), pFolder->GetName());

            pFolder->MessageDelete(pFolderMessage->GetID());
         }
         else
         {
            debug("FolderMaintenance no folder to remove from\n");
         }

         iDebug = debuglevel(DEBUGLEVEL_DEBUG);

         // printf("FolderMaintenance deleting %d %p\n", iMessageNum, MessageList(iMessageNum));
         if(FolderMessageDelete(iMessageNum) == false)
         {
            iMessageNum++;
         }

         debuglevel(iDebug);
      }
      else
      {
         iMessageNum++;
      }
   }

   debug("FolderMaintenance post-check %d messages\n", FolderMessageCount());

   for(iMessageNum = 0; iMessageNum < FolderMessageCount(); iMessageNum++)
   {
      pFolderMessage = FolderMessageList(iMessageNum);
      bChanged = pFolderMessage->GetChanged();
      debug("FolderMaintenance %ld changed %s\n", pFolderMessage->GetID(), BoolStr(pFolderMessage->GetChanged()));

      lParentID = pFolderMessage->GetParentID();
      if(lParentID != -1)
      {
         pParent = FolderMessageGet(lParentID);
         debug("FolderMaintenance %d, %p (%ld) -> %ld -> %p\n", iMessageNum, pFolderMessage, pFolderMessage->GetID(), lParentID, pParent);

         if(pParent != NULL)
         {
            pFolderMessage->SetParent(pParent);

            pParent->Add(pFolderMessage);
         }
         else if(lParentID != -1)
         {
            debug("FolderMaintenance cannot remap %p(%ld, %ld, '%s') to parent %ld\n",
               pFolderMessage, pFolderMessage->GetID(), pFolderMessage->GetFromID(),
               pFolderMessage->GetSubject() != NULL ? pFolderMessage->GetSubject()->Data(false) : NULL,
               lParentID);

            pFolder = pFolderMessage->GetTree();
            if(pFolder != NULL)
            {
               pFolder->MessageAdd(pFolderMessage);
            }
            else
            {
               debug("FolderMaintenance cannot put %p(%ld) in tree\n", pFolderMessage, pFolderMessage->GetID());
            }
         }
      }

      if(bChanged == true)
      {
         debug("FolderMaintenance updating message %ld\n", pFolderMessage->GetID());
         pFolderMessage->Update();
      }
      else
      {
         pFolderMessage->SetChanged(false);
      }
   }

   debug("FolderMaintenance exit\n");
}

bool BulletinLoad()
{
   STACKTRACE
   g_pBulletinList = new MessageTreeItem("bulletin", (long)0, NULL, false);
   // debug("BulletinLoad class %s\n", g_pBulletinList->GetClass());

   return true;
}

bool BulletinUnload()
{
   delete g_pBulletinList;

   return true;
}

FolderMessageItem *BulletinGet(int iBulletinID)
{
   FolderMessageItem *pReturn = NULL;

   pReturn = (FolderMessageItem *)g_pBulletinList->MessageFind(iBulletinID);
   debug("BulletinGet %d -> %p\n", iBulletinID, pReturn);

   return pReturn;
}

FolderMessageItem *BulletinList(int iBulletinNum)
{
   return (FolderMessageItem *)g_pBulletinList->MessageChild(iBulletinNum);
}

/* bool MessageAccessMove(int iAccessLevel, int iSubType, int iTreeMode)
{
   if(iAccessLevel >= LEVEL_WITNESS || iSubType == SUBTYPE_EDITOR)
   {
      return true;
   }

   if(mask(iTreeMode, FOLDERMODE_SUB_MOVE) == true || (iSubType == SUBTYPE_MEMBER && mask(iTreeMode, FOLDERMODE_MEM_MOVE) == true))
   {
      return true;
   }

   return false;
}


bool MessageAccessDelete(int iUserID, int iAccessLevel, int iSubType, int iTreeMode, int iFromID)
{
   if(MessageAccessMove(iAccessLevel, iSubType, 0) == true)
   {
      return true;
   }

   if(mask(iTreeMode, FOLDERMODE_SUB_ADEL) == true || (mask(iTreeMode, FOLDERMODE_SUB_SDEL) == true && iFromID == iUserID))
   {
      return true;
   }
   else if(iSubType == SUBTYPE_MEMBER && (mask(iTreeMode, FOLDERMODE_MEM_ADEL) == true || (mask(iTreeMode, FOLDERMODE_MEM_SDEL) == true && iFromID == iUserID)))
   {
      return true;
   }

   return false;
} */

bool MessageMatchFields(EDF *pIn, ConnData *pConnData)
{
   STACKTRACE
   bool bLoop = false, bReturn = false;
   char *szName = NULL;

   // EDFPrint("MessageMatchFields entry", pIn);

   bLoop = pIn->Child();
   while(bLoop == true && bReturn == false)
   {
      pIn->Get(&szName);

      if(stricmp(szName, "noparent") == 0)
      {
         bReturn = true;
      }
      else if(stricmp(szName, "startdate") == 0 || stricmp(szName, "enddate") == 0)
      {
         bReturn = true;
      }
      else if(stricmp(szName, "startday") == 0 || stricmp(szName, "endday") == 0 || stricmp(szName, "starttime") == 0 || stricmp(szName, "endtime") == 0)
      {
         bReturn = true;
      }
      else if(stricmp(szName, "fromid") == 0 || stricmp(szName, "toid") == 0 || stricmp(szName, "userid") == 0)
      {
         bReturn = true;
      }
      else if(stricmp(szName, "subject") == 0 || stricmp(szName, "text") == 0 || stricmp(szName, "keyword") == 0)
      {
         bReturn = true;
      }
      else if(ConnVersion(pConnData, "2.6") >= 0 && (stricmp(szName, "not") == 0 || stricmp(szName, "and") == 0 || stricmp(szName, "or") == 0 || stricmp(szName, "nand") == 0 || stricmp(szName, "nor") == 0))
      {
         bReturn = true;
      }
	  else if(ConnVersion(pConnData, "2.9") >= 0 && stricmp(szName, "saved") == 0)
	  {
		  bReturn = true;
	  }

      delete[] szName;

      if(bReturn == true)
      {
         pIn->Parent();
      }
      else
      {
         bLoop = pIn->Next();
         if(bLoop == false)
         {
            pIn->Parent();
         }
      }
   }

   // printf("MessageMatchFields exit %s\n", BoolStr(bReturn));
   if(bReturn == true)
   {
      // EDFPrint("MessageMatchFields", pIn);
   }
   return bReturn;
}

bool MessageMatchBytes(bytes *pBytes, bytes *pMatch, EDF *pIn, bool bFull)
{
   STACKTRACE
   int iDebug = 0;
   bool bRegEx = false, bReturn = false;

   bRegEx = pIn->GetChildBool("regex");

   if(pBytes != NULL && pMatch != NULL)
   {
      if(bRegEx == true)
      {
         // debug("MessageMatchBytes regex %s -vs- %s\n", pBytes->Data(false), pMatch->Data(false));

         // iDebug = debuglevel(DEBUGLEVEL_DEBUG);
         if(strmatch((char *)pBytes->Data(false), (char *)pMatch->Data(false), false, bFull, true) == true)
         {
            bReturn = true;
         }
         // debuglevel(iDebug);
      }
      else
      {
         if((bFull == false && stristr(pBytes, pMatch) == true) ||
            (bFull == true && stricmp((char *)pBytes->Data(false), (char *)pMatch->Data(false)) == 0))
         {
            bReturn = true;
         }
      }
   }

   return bReturn;
}

// MessageMatch: Check message for matching search criteria
bool MessageMatch(FolderMessageItem *pItem, EDF *pIn, int iOp, int iDepth = 0)
{
   STACKTRACE
   bool bLoop = false, bReturn = false, bField = false, bName = false;
   int iSeconds = 0, iNumNames = 0, iDebug = 0;
   long lValue = -1;
   time_t lDate = 0;
   char *szName = NULL;
   bytes *pValue = NULL;
   struct tm *pTime = NULL;

   // iDebug = debuglevel(DEBUGLEVEL_DEBUG);
   debug(DEBUGLEVEL_DEBUG, "MessageMatch entry %p(%ld %ld) %d, %d\n", pItem, pItem->GetID(), pItem->GetTreeID(), iOp, iDepth);
   EDFParser::debugPrint(DEBUGLEVEL_DEBUG, pIn, EDFElement::EL_CURR | EDFElement::PR_SPACE);

   bLoop = pIn->Child();
   while(bLoop == true)
   {
      bField = false;
      bName = true;
      pValue = NULL;
      lValue = -1;

      pIn->TypeGet(&szName, &pValue, &lValue, NULL);
      // debug("MessageMatch %s %p(%s) %ld\n", szName, pValue, pValue != NULL ? pValue->Data(false) : NULL, lValue);

      // if(ConnVersion(pConnData, "2.6") >= 0 && iDepth > 0 && stricmp(szName, "folderid") == 0)
      if(stricmp(szName, "folderid") == 0)
      {
         if(pItem->GetTreeID() == lValue)
         {
            bField = true;
         }
      }
      else if(stricmp(szName, "noparent") == 0)
      {
         if(pItem->GetParentID() == -1)
         {
            bField = true;
         }
      }
      else if(stricmp(szName, "startdate") == 0)
      {
         if(pItem->GetDate() >= lValue)
         {
            bField = true;
         }
      }
      else if(stricmp(szName, "enddate") == 0)
      {
         if(pItem->GetDate() <= lValue)
         {
            bField = true;
         }
      }
      else if(stricmp(szName, "startday") == 0 || stricmp(szName, "endday") == 0 || stricmp(szName, "starttime") == 0 || stricmp(szName, "endtime") == 0)
      {
         lDate = pItem->GetDate();
         pTime = localtime(&lDate);
         iSeconds = 3600 * pTime->tm_hour + 60 * pTime->tm_min + pTime->tm_sec;

         if(pTime->tm_wday == 0)
         {
            pTime->tm_wday = 7;
         }

         debug("MessageMatch day/time check m=%ld,%ld(%d %d) v=%ld\n", pItem->GetID(), pItem->GetDate(), pTime->tm_wday, iSeconds, lValue);

         if(stricmp(szName, "startday") == 0 && pTime->tm_wday >= lValue)
         {
            bField = true;
         }
         else if(stricmp(szName, "endday") == 0 && pTime->tm_wday <= lValue)
         {
            bField = true;
         }
         else if(stricmp(szName, "starttime") == 0 && iSeconds >= lValue)
         {
            bField = true;
         }
         else if(stricmp(szName, "endtime") == 0 && iSeconds <= lValue)
         {
            bField = true;
         }
      }
      else if(stricmp(szName, "fromid") == 0)
      {
         if(pItem->GetFromID() == lValue)
         {
            bField = true;
         }
      }
      else if(stricmp(szName, "toid") == 0)
      {
         if(pItem->GetToID() == lValue)
         {
            bField = true;
         }
      }
      else if(stricmp(szName, "subject") == 0 && pValue != NULL)
      {
         bField = MessageMatchBytes(pItem->GetSubject(), pValue, pIn, true);
      }
      else if(stricmp(szName, "text") == 0 && pValue != NULL)
      {
         bField = MessageMatchBytes(pItem->GetText(), pValue, pIn, false);
      }
      else if(stricmp(szName, "keyword") == 0 && pValue != NULL)
      {
         bField = MessageMatchBytes(pItem->GetSubject(), pValue, pIn, false) || MessageMatchBytes(pItem->GetText(), pValue, pIn, false);
      }
      else if(stricmp(szName, "not") == 0)
      {
         // As no op is specified use AND
         bField = MessageMatch(pItem, pIn, MATCH_NOT | MATCH_AND, iDepth + 1);
      }
      else if(stricmp(szName, "and") == 0)
      {
         bField = MessageMatch(pItem, pIn, MATCH_AND, iDepth + 1);
      }
      else if(stricmp(szName, "or") == 0)
      {
         bField = MessageMatch(pItem, pIn, MATCH_OR, iDepth + 1);
      }
      else if(stricmp(szName, "nand") == 0)
      {
         bField = MessageMatch(pItem, pIn, MATCH_AND | MATCH_NOT, iDepth + 1);
      }
      else if(stricmp(szName, "nor") == 0)
      {
         bField = MessageMatch(pItem, pIn, MATCH_OR | MATCH_NOT, iDepth + 1);
      }
      else
      {
         bName = false;
      }
      debug(DEBUGLEVEL_DEBUG, "MessageMatch %s %p(%s) %ld -> %s %s\n", szName, pValue, pValue != NULL ? pValue->Data(false) : NULL, lValue, BoolStr(bField), BoolStr(bName));

      // printf("MessageMatch fields=%s match=%s\n", BoolStr(bField), BoolStr(bMatch));

      delete pValue;
      delete[] szName;

      if(bName == true)
      {
         iNumNames++;

         // Processable field, did it match?
         if(bField == true && mask(iOp, MATCH_OR) == true)
         {
            // Matched one of the fields
            bReturn = true;
            bLoop = false;
         }
         else if(bField == false && mask(iOp, MATCH_AND) == true)
         {
            // Didn't match one of the fields
            bReturn = false;
            bLoop = false;
         }
         else if(bField == true && mask(iOp, MATCH_AND) == true && iNumNames == 1)
         {
            // Matched first AND field
            bReturn = true;
         }
      }

      if(bLoop == true)
      {
         bLoop = pIn->Next();
      }
      if(bLoop == false)
      {
         pIn->Parent();
      }
   }

   if(mask(iOp, MATCH_NOT) == true)
   {
      debug(DEBUGLEVEL_DEBUG, "MessageMatch not %s\n", BoolStr(bReturn));
      bReturn = !bReturn;
   }

   debug(DEBUGLEVEL_DEBUG, "MessageMatch exit %s\n", BoolStr(bReturn));
   // debuglevel(iDebug);
   return bReturn;
}

bytes *MessageSQLItemAndOr(EDF *pEDF, char *szOp, int iIndent)
{
   STACKTRACE
   bool bLoop = false;
   bytes *pReturn = NULL, *pTemp = NULL;

   debug("%-*sMessageSQLItemAndOr entry %s\n", iIndent, "", szOp);

   pReturn = new bytes();

   bLoop = pEDF->Child();
   while(bLoop == true)
   {
      pTemp = MessageSQLItem(pEDF, iIndent + 1);

      if(pTemp->Length() > 0)
      {
         if(pReturn->Length() > 0)
         {
            pReturn->Append(" ");
            pReturn->Append(szOp);
            pReturn->Append(" ");
         }
         else
         {
            pReturn->Append("(");
         }

         pReturn->Append(pTemp);
      }

      delete pTemp;

      bLoop = pEDF->Next();
      if(bLoop == false)
      {
         pEDF->Parent();
      }
   }

   if(pReturn->Length() > 0)
   {
      pReturn->Append(")");
   }

   debug("%-*sMessageSQLItemAndOr exit %s\n", iIndent, "", pReturn->Data(false));
   return pReturn;
}

bytes *MessageSQLItemNot(EDF *pEDF, int iIndent)
{
   bytes *pReturn = NULL, *pTemp = NULL;

   debug("%-*sMessageSQLItemNot entry\n", iIndent, "");

   pReturn = new bytes();

   if(pEDF->Child() == true)
   {
      pTemp = MessageSQLItem(pEDF, iIndent + 1);

      if(pTemp->Length() > 0)
      {
         pReturn->Append("not(");
         pReturn->Append(pTemp);
         pReturn->Append(")");
      }

      delete pTemp;

      pEDF->Parent();
   }

   debug("%-*sMessageSQLItemNot exit %s\n", iIndent, "", pReturn->Data(false));
   return pReturn;
}

bytes *MessageSQLItem(EDF *pEDF, int iIndent)
{
   STACKTRACE
   int iType = EDFElement::NONE;
   long lValue = -1;
   char szSQL[100];
   char *szName = NULL;
   bytes *pValue = NULL, *pReturn = NULL, *pTemp = NULL;
   EDF *pKeyword = NULL;

   strcpy(szSQL, "");

   iType = pEDF->TypeGet(&szName, &pValue, &lValue, NULL);
   debug("%-*sMessageSQLItem entry, %s %p(%ld,'%s') %ld\n", iIndent, "", szName, pValue, pValue != NULL ? pValue->Length() : -1, pValue != NULL ? (char *)pValue->Data(false) : "", lValue);

   if(stricmp(szName, "folderid") == 0 && lValue > 0)
   {
      sprintf(szSQL, "folderid = %ld", lValue);
   }
   else if(stricmp(szName, "startdate") == 0 || stricmp(szName, "enddate") == 0)
   {
      sprintf(szSQL, "message_date %s %ld", stricmp(szName, "startdate") == 0 ? "<=" : ">=", lValue);
   }
   else if((stricmp(szName, "fromid") == 0 || stricmp(szName, "toid") == 0) && lValue > 0)
   {
      sprintf(szSQL, "%s = %ld", szName, lValue);
   }
   else if((stricmp(szName, "subject") == 0 || stricmp(szName, "text") == 0) && pValue != NULL)
   {
      pReturn = new bytes();

      if(stricmp(szName, "text") == 0)
      {
         pReturn->Append("message_text");
      }
      else
      {
         pReturn->Append(szName);
      }

      if(pEDF->GetChildBool("full") == true)
      {
         pReturn->Append(" = '");
         pReturn->Append(pValue);
         pReturn->Append("'");
      }
      else
      {
         pReturn->Append(" like '%");
         pReturn->Append(pValue);
         pReturn->Append("%'");
      }
   }
   else if(stricmp(szName, "keyword") == 0 && pValue != NULL)
   {
      pKeyword = new EDF();

      pKeyword->Set("or");

      pKeyword->Add("subject", pValue);
      if(pEDF->GetChildBool("full") == true)
      {
         pKeyword->AddChild("full", true);
      }
      pKeyword->Parent();

      pKeyword->Add("text", pValue);
      if(pEDF->GetChildBool("full") == true)
      {
         pKeyword->AddChild("full", true);
      }
      pKeyword->Parent();

      pReturn = MessageSQLItem(pKeyword, iIndent + 1);
   }
   else if(stricmp(szName, "and") == 0 || stricmp(szName, "or") == 0)
   {
      pReturn = MessageSQLItemAndOr(pEDF, szName, iIndent);
   }
   else if(stricmp(szName, "not") == 0)
   {
      pReturn = MessageSQLItemNot(pEDF, iIndent);
   }
   else if(stricmp(szName, "nand") == 0 || stricmp(szName, "nor") == 0)
   {
      pReturn = new bytes();

      pTemp = MessageSQLItemAndOr(pEDF, szName + 1, iIndent);

      if(pTemp->Length() > 0)
      {
         pReturn->Append("not(");
         pReturn->Append(pTemp);
         pReturn->Append(")");
      }

      delete pTemp;
   }

   delete pValue;
   delete []szName;

   if(pReturn == NULL)
   {
      pReturn = new bytes(szSQL);
   }

   debug("%-*sMessageItemSQL exit %s\n", iIndent, "", pReturn->Data(false));
   return pReturn;
}

char *MessageSQL(EDF *pEDF)
{
   STACKTRACE
   char *szReturn = NULL;
   bytes *pSQL = NULL;

   if(pEDF->Child() == true)
   {
      pSQL = MessageSQLItem(pEDF, 0);
      pEDF->Parent();
   }

   if(pSQL != NULL && pSQL->Length() > 0)
   {
      szReturn = new char[pSQL->Length() + 500];

      sprintf(szReturn, "select messageid,parentid,folderid,message_date,fromid,toid,null,subject,null from %s", FMI_TABLENAME);
      if(pSQL->Length() > 0)
      {
         strcat(szReturn, " where ");
         strcat(szReturn, (char *)pSQL->Data(false));
      }
      strcat(szReturn, " order by messageid");

      delete pSQL;
   }

   return szReturn;
}

void FolderMessageItemAnnounce(EDF *pAnnounce, int iOp, FolderMessageItem *pItem, int iOldVoteType)
{
   STACKTRACE
   bytes *pSubject = NULL;

   if(mask(pItem->GetType(), MSGTYPE_VOTE) == true && pItem->GetVoteType() != iOldVoteType)
   {
      pAnnounce->AddChild("votetype", pItem->GetVoteType());
      pAnnounce->AddChild("oldvotetype", iOldVoteType);
   }

   if(iOp == RQ_ADD)
   {
      pSubject = pItem->GetSubject();
      if(pSubject != NULL)
      {
         pAnnounce->AddChild("subject", pSubject);
      }
   }
}

bool FolderMessageItemEdit(EDF *pData, int iOp, FolderMessageItem *pItem, EDF *pIn, EDF *pOut, UserItem *pCurr, ConnData *pConnData, char *szRequest, int iBase, bool bRecurse, int iSubType, int *iMarked, int *iOldVoteType)
{
   STACKTRACE
   int iMinValue = 0, iMaxValue = 0, iMsgType = 0, iVoteClose = 0, iVoteType = 0;
   bool bLoop = false, bVoteType = false;
   double dMinValue = 0, dMaxValue = 0;
   bytes *pSubject = NULL, *pVote = NULL;
   MessageTreeItem *pFolder = NULL;

   pFolder = pItem->GetTree();

   if(iOp == RQ_ADD || pCurr->GetAccessLevel() >= LEVEL_WITNESS)
   {
      if(pIn->GetChild("subject", &pSubject) == true)
      {
         pItem->SetSubject(pSubject);
         pOut->AddChild("subject", pSubject);

         delete pSubject;
      }
   }

   if(pCurr->GetAccessLevel() >= LEVEL_WITNESS && iOp == RQ_EDIT)
   {
      if(pIn->GetChild("msgtype", &iMsgType) == true && mask(iMsgType, MSGTYPE_DELETED) == false && pItem->GetDeleted() == true)
      {
         pItem->SetDeleted(false);
      }
   }

   if(pCurr->GetAccessLevel() >= LEVEL_EDITOR)
   {
      if(iOp == RQ_ADD)
      {
         pIn->GetChild("msgtype", &iMsgType);

         if(iMsgType == MSGTYPE_VOTE)
         {
            EDFParser::debugPrint("MessageItemEdit vote options", pIn, EDFElement::EL_CURR | EDFElement::PR_SPACE);

            pItem->SetType(pItem->GetType() | MSGTYPE_VOTE);
         }
      }

      if(mask(pItem->GetType(), MSGTYPE_VOTE) == true)
      {
         (*iOldVoteType) = pItem->GetVoteType();

         bVoteType = pIn->GetChild("votetype", &iVoteType);

         if(iOp == RQ_ADD)
         {
            if(mask(iVoteType, VOTE_MULTI) == true)
            {
               pItem->SetVoteType(VOTE_NAMED | VOTE_MULTI);
            }
            else if(mask(iVoteType, VOTE_NAMED) == true)
            {
               pItem->SetVoteType(VOTE_NAMED);
            }

            if(mask(iVoteType, VOTE_INTVALUES) == true)
            {
               pItem->SetVoteType(pItem->GetVoteType() | VOTE_INTVALUES);
               if(pIn->GetChild("minvalue", &iMinValue) == true)
               {
                  pItem->SetVoteMinValue(iMinValue);
               }
               if(pIn->GetChild("maxvalue", &iMaxValue) == true)
               {
                  pItem->SetVoteMaxValue(iMaxValue);
               }
            }
            else if(mask(iVoteType, VOTE_PERCENT) == true)
            {
               pItem->SetVoteType(pItem->GetVoteType() | VOTE_PERCENT);
            }
            if(mask(iVoteType, VOTE_FLOATVALUES) == true)
            {
               pItem->SetVoteType(pItem->GetVoteType() | VOTE_FLOATVALUES);
               if(pIn->GetChild("minvalue", &dMinValue) == true)
               {
                  pItem->SetVoteMinValue(dMinValue);
               }
               if(pIn->GetChild("maxvalue", &dMaxValue) == true)
               {
                  pItem->SetVoteMaxValue(dMaxValue);
               }
            }
            else if(mask(iVoteType, VOTE_FLOATPERCENT) == true)
            {
               pItem->SetVoteType(pItem->GetVoteType() | VOTE_FLOATPERCENT);
            }
            else if(mask(iVoteType, VOTE_STRVALUES) == true)
            {
               pItem->SetVoteType(pItem->GetVoteType() | VOTE_STRVALUES);
            }

            if(mask(iVoteType, VOTE_NAMED) == true && mask(iVoteType, VOTE_CHANGE) == true)
            {
               pItem->SetVoteType(pItem->GetVoteType() | VOTE_CHANGE);
            }

            if(pIn->GetChild("voteclose", &iVoteClose) == true)
            {
               pItem->SetVoteClose(iVoteClose);
            }
            else
            {
               pItem->SetVoteClose(time(NULL) + FolderExpire(pData, pFolder, pItem) / 2);
            }
         }

         if(iOp == RQ_ADD || (iOp == RQ_EDIT && bVoteType == true && (pCurr->GetAccessLevel() >= LEVEL_WITNESS || pItem->GetFromID() == pCurr->GetID())))
         {
            if(mask(iVoteType, VOTE_PUBLIC) == true && mask(pItem->GetVoteType(), VOTE_PUBLIC) == false)
            {
               pItem->SetVoteType(pItem->GetVoteType() | VOTE_PUBLIC);
            }
            else if(mask(iVoteType, VOTE_PUBLIC) == false && mask(pItem->GetVoteType(), VOTE_PUBLIC) == true)
            {
               pItem->SetVoteType(pItem->GetVoteType() - VOTE_PUBLIC);
            }

            if(mask(pItem->GetVoteType(), VOTE_CLOSED) == false)
            {
               if(mask(iVoteType, VOTE_PUBLIC_CLOSE) == true && mask(pItem->GetVoteType(), VOTE_PUBLIC_CLOSE) == false)
               {
                  pItem->SetVoteType(pItem->GetVoteType() | VOTE_PUBLIC_CLOSE);
               }
               else if(mask(iVoteType, VOTE_PUBLIC_CLOSE) == false && mask(pItem->GetVoteType(), VOTE_PUBLIC_CLOSE) == true)
               {
                  pItem->SetVoteType(pItem->GetVoteType() - VOTE_PUBLIC_CLOSE);
               }
            }

            if(iOp == RQ_EDIT && (pCurr->GetAccessLevel() >= LEVEL_WITNESS || pItem->GetFromID() == pCurr->GetID()))
            {
               if(mask(iVoteType, VOTE_CLOSED) == true && MessageVoteClose(pData, iBase, pItem, false) == true)
               {
                  (*iMarked) = MARKED_UNREAD;
               }
            }
         }

         if(iOp == RQ_ADD && IS_VALUE_VOTE(pItem->GetVoteType()) == false)
         {
            bLoop = pIn->Child("vote");
            while(bLoop == true)
            {
               pVote = NULL;

               if(pIn->Get(NULL, &pVote) == true && pVote != NULL)
               {
                  pItem->AddVote(pVote);

                  delete pVote;
               }

               bLoop = pIn->Next("vote");
               if(bLoop == false)
               {
                  pIn->Parent();
               }
            }
         }
      }
   }

   return true;
}

void FolderMessageListDetail(EDF *pEDF, char *szName, char *szDate, UserItem *pUser, DBSub *pSubs)
{
   STACKTRACE
   int iDate = -1, iID = -1; //, iSubType = 0;
   bool bLoop = false;
   char *szValue = NULL;
   MessageTreeItem *pFolder = NULL;
   EDFElement *pDelete = NULL, *pTemp = NULL;

   bLoop = pEDF->Child(szName);
   while(bLoop == true)
   {
      iDate = -1;

      if(szDate != NULL)
      {
         pEDF->GetChild(szDate, &iDate);
      }
      else
      {
         pEDF->Get(NULL, &iDate);
      }

      if(pEDF->GetChild("userid", &iID) == true && UserGet(iID, &szValue, false, iDate) != NULL)
      {
         pEDF->AddChild("username", szValue);
      }
      if(pEDF->GetChild("fromid", &iID) == true && UserGet(iID, &szValue, false, iDate) != NULL)
      {
         pEDF->AddChild("fromname", szValue);
      }
      if(pEDF->GetChild("folderid", &iID) == true)
      {
         pFolder = FolderGet(iID, &szValue, false);
         if(pFolder != NULL)
         {
            if(pUser == NULL || pSubs == NULL || MessageTreeAccess(pFolder, ACCMODE_SUB_READ, ACCMODE_MEM_READ, pUser->GetAccessLevel(), pSubs->SubType(iID)) > 0)
            {
               pEDF->AddChild("foldername", szValue);
            }
            else if(pUser != NULL && pSubs != NULL)
            {
               debug("FolderMessageListDetail %s %s denied access to %s\n", szName, pUser->GetName(), pFolder->GetName());
               pDelete = pEDF->GetCurr();
            }
         }
      }

      bLoop = pEDF->Next(szName);
      if(bLoop == false)
      {
         pEDF->Parent();
      }

      if(pDelete != NULL)
      {
         pTemp = pEDF->GetCurr();

         pEDF->SetCurr(pDelete);
         pEDF->Delete();

         pEDF->SetCurr(pTemp);

         pDelete = NULL;
      }
   }
}

FolderMessageItem *MessageListReply(FolderMessageItem *pFolderMessage, int *iFolderID, int iUserID, int iAccessLevel, DBSub *pFolders)
{
   STACKTRACE
   int iSubType = 0;

   // printf("MessageListReply %p %d %d %d %p\n", pFolderMessage, *iFolderID, iUserID, iAccessLevel, pFolders);

   if(pFolderMessage->GetDeleted() == true && iAccessLevel < LEVEL_WITNESS)
   {
      debug("MessageListReply message %ld deleted\n", pFolderMessage->GetID());
      pFolderMessage = NULL;
   }
   else
   {
      // printf("MessageListReply tree ID check %d %d\n", pFolderMessage->GetTreeID(), *iFolderID);
      if(pFolderMessage->GetTreeID() != (*iFolderID))
      {
         iSubType = pFolders->SubType(pFolderMessage->GetTreeID());
         // printf("MessageListReply tree mode check %p %d %d\n", pFolderMessage->GetTree(), pFolderMessage->GetTree()->GetAccessMode(), iSubType);
         if(MessageTreeAccess(pFolderMessage->GetTree(), ACCMODE_SUB_READ, ACCMODE_MEM_READ, iAccessLevel, iSubType) == 0)
         {
            debug("MessageListReply message %ld MessageTreeMode failed\n", pFolderMessage->GetID());
            pFolderMessage = NULL;
         }
         else
         {
            (*iFolderID) = pFolderMessage->GetTreeID();
         }
      }

      if(pFolderMessage != NULL && mask(pFolderMessage->GetTree()->GetAccessMode(), ACCMODE_PRIVATE) == true)
      {
         // printf("MessageListReply private check\n");
         if(MessageAccess(pFolderMessage, iUserID, iAccessLevel, iSubType) == 0)
         {
            debug("MessageListReply message %ld MessageAccessRead failed\n", pFolderMessage->GetID());
            pFolderMessage = NULL;
         }
      }
   }

   return pFolderMessage;
}

int MessageMark(FolderMessageItem *pMessage, bool bRead, UserItem *pUser, DBMessageRead *pReads, int iMarkType = 0)
{
   int iReturn = 0;

   if(bRead == true && ((pReads != NULL && pReads->Add(pMessage->GetID(), iMarkType) == true) ||
      (pReads == NULL && DBMessageRead::UserAdd(pUser->GetID(), pMessage->GetID(), iMarkType) == true)))
   {
      iReturn = MARKED_READ;
   }
   else if(bRead == false && ((pReads != NULL && pReads->Delete(pMessage->GetID()) == true) ||
      (pReads == NULL && DBMessageRead::UserDelete(pUser->GetID(), pMessage->GetID()) == true)))
   {
      iReturn = MARKED_UNREAD;
   }

   // debug("MessageMark %p(%d) %s %p %p %d -> %d\n", pMessage, pMessage->GetID(), BoolStr(bRead), pUser, pReads, iMarkType, iReturn);

   return iReturn;
}

int MessageMarking(UserItem *pUser, DBMessageRead *pReads, const char *szAnnounce, FolderMessageItem *pItem, int iOverride, int *pMarkType)
{
   STACKTRACE
   int iMarking = 0, iReturn = -1, iMarkType = 0;
   bool bPrivate = false, bLoop = false;
   EDF *pRules = NULL;
   MessageTreeItem *pFolder = NULL, *pParentFolder = NULL;
   FolderMessageItem *pParent = NULL;

   pFolder = pItem->GetTree();

   if(pFolder != NULL)
   {
      if(pUser != NULL && pItem != NULL)
      {
         // Get marktype for parent message
         if(pReads != NULL)
         {
            iMarkType = pReads->Get(pItem->GetParentID());
         }
         else
         {
            iMarkType = DBMessageRead::UserType(pUser->GetID(), pItem->GetParentID());
         }

         // printf("MessageMarking %ld(%s) / %ld / %ld / %d", pUser->GetID(), pUser->GetName(), pFolderMessage->GetID(), pFolderMessage->GetParentID(), iMarkType);

         if(mask(iMarkType, THREAD_MSGCHILD) == true || mask(iMarkType, THREAD_CHILD) == true)
         {
            // Stay caught up
            pParent = (FolderMessageItem *)pItem->GetParent();
            if(pParent != NULL)
            {
               // Parent must be same folder or...
               if(pParent->GetTreeID() == pItem->GetTreeID())
               {
                  iReturn = MessageMark(pItem, true, pUser, pReads, THREAD_MSGCHILD);
               }
               else
               {
                  // ...default reply folder
                  pParentFolder = pParent->GetTree();
                  if(pParentFolder->GetReplyID() == pItem->GetTreeID())
                  {
                     iReturn = MessageMark(pItem, true, pUser, pReads, THREAD_MSGCHILD);
                  }
                  else
                  {
                     iMarkType = 0;
                  }
               }
            }

            if(iReturn == 0)
            {
               // Thread should not be marked from this point on
               iMarkType = 0;
            }
         }

         if(iReturn == -1)
         {
            // Check rules
            pRules = pUser->GetRules();

            if(pRules != NULL)
            {
               bLoop = pRules->Child("rule");
               while(bLoop == true)
               {
                  // EDFParser::debugPrint("MessageMarking rule", pRules, EDFElement::EL_CURR + EDFElement::PR_SPACE);

                  if(MessageMatch(pItem, pRules, MATCH_AND) == true)
                  {
                     iMarkType = 0;
                     pRules->GetChild("marktype", &iMarkType);

                     iReturn = MessageMark(pItem, true, pUser, pReads, iMarkType);
                     debug("MessageMarking marked %d by rule (marktype %d)\n", iReturn, iMarkType);
                     EDFParser::debugPrint(pRules, false, true);

                     bLoop = false;
                     pRules->Parent();
                  }
                  else
                  {
                     bLoop = pRules->Next("rule");
                     if(bLoop == false)
                     {
                        pRules->Parent();
                     }
                  }
               }
            }
         }

         if(iReturn == -1)
         {
            // printf(" -> %d / %d(%p)\n", iReturn, iMarkType, pMarkType);
            iMarking = pUser->GetMarking();

            if(iOverride > 0 || iMarking > 0)
            {
               // Standard marking types
               bPrivate = mask(pFolder->GetAccessMode(), ACCMODE_PRIVATE);

               if(iOverride > 0 || pUser->GetID() == pItem->GetFromID() || stricmp(szAnnounce, MSG_MESSAGE_EDIT) == 0)
               {
                  // printf("MessageMarking %ld(%s) / %ld / %s", pUser->GetID(), pUser->GetName(), pFolderMessage->GetID(), BoolStr(bPrivate));

                  if(iOverride == MARKED_READ ||
                     (stricmp(szAnnounce, MSG_MESSAGE_ADD) == 0 &&
                     ((bPrivate == true && mask(iMarking, MARKING_ADD_PRIVATE) == true) || (bPrivate == false && mask(iMarking, MARKING_ADD_PUBLIC) == true))))
                  {
                     iReturn = MessageMark(pItem, true, pUser, pReads);
                  }
                  else if(iOverride == MARKED_UNREAD ||
                     (stricmp(szAnnounce, MSG_MESSAGE_EDIT) == 0 &&
                     ((bPrivate == true && mask(iMarking, MARKING_EDIT_PRIVATE) == true) || (bPrivate == false && mask(iMarking, MARKING_EDIT_PUBLIC) == true))))
                  {
                     iReturn = MessageMark(pItem, false, pUser, pReads);
                  }
                  // printf(" -> %d\n", iReturn);
               }
            }
         }
      }
      else
      {
         // printf("MessageMarking no user %p / message %p\n", pUser, pFolderMessage);
      }
   }
   else
   {
      // printf("MessageMarking no folder\n");
   }

   if(pMarkType != NULL)
   {
      *pMarkType = iMarkType;
   }

   debug(DEBUGLEVEL_DEBUG, "MessageMarking %d %d -> %d, %d\n", pUser->GetID(), pItem->GetID(), iReturn, iMarkType);

   return iReturn;
}

// MessageListRead: Add marktype / read field to output
bool MessageListRead(EDF *pEDF, int iMessageID, DBMessageRead *pReads)
{
   STACKTRACE
   int iMarkType = 0;

   iMarkType = pReads->Get(iMessageID);
   if(iMarkType >= 0)
   {
      pEDF->AddChild("read", true);
      if(iMarkType > 0)
      {
         pEDF->AddChild("marktype", iMarkType);
      }

      return true;
   }

   return false;
}

bool MessageListUsers(EDF *pEDF)
{
   STACKTRACE
   int iID = 0;
   bool bLoop = false;
   char *szName = NULL;

   bLoop = pEDF->Child("userid");
   while(bLoop == true)
   {
      pEDF->Get(NULL, &iID);

      if(UserGet(iID, &szName, false, -1) != NULL)
      {
         pEDF->AddChild("username", szName);
      }

      bLoop = pEDF->Next("userid");
      if(bLoop == false)
      {
         pEDF->Parent();
      }
   }

   return true;
}

void FolderMessageItemListReply(EDF *pOut, char *szType, int iMessageID, int iDate, int iFromID, bytes *pFrom, int iBase, MessageTreeItem *pFolder, int iRelatedID, DBMessageRead *pReads)
{
   STACKTRACE
   int iAttDate = -1, iAttPos = 0;
   bool bAtt = false;
   char *szName = NULL;

   debug(DEBUGLEVEL_DEBUG, "FolderMessageItemListReply %s m=%d d=%d f=%d/%s %d %p %d %p\n",
      szType, iMessageID, iDate, iFromID, pFrom != NULL ? (char *)pFrom->Data(false) : NULL, iBase, pFolder, iRelatedID, pReads);

   if(pOut->Children("attachment") == 0)
   {
      pOut->Add(szType, iMessageID);
   }
   else
   {
      // Insert reply at the right place
      // printf("MessageListSingle replyto date %ld\n", pChild->GetDate());

      bAtt = pOut->Child("attachment");
      while(bAtt == true)
      {
         iAttDate = -1;
         if(pOut->GetChild("date", &iAttDate) == true)
         {
            debug("MessageListSingle attachment date %d\n", iAttDate);
            if(iAttDate > iDate)
            {
               iAttPos = pOut->Position(true);

               pOut->Parent();


               // Insert a new element just before the attachment we were looking at
               pOut->Add("attachment", iMessageID, iAttPos);
               // Now it's in the right place rename it to what we want
               pOut->Set(szType, iMessageID);

               bAtt = false;
            }
         }

         if(bAtt == true)
         {
            bAtt = pOut->Next("attachment");
            if(bAtt == false)
            {
               pOut->Parent();

               pOut->Add(szType, iMessageID);
            }
         }
      }
   }

   STACKTRACEUPDATE

   // Check marking
   if(pReads != NULL)
   {
      MessageListRead(pOut, iMessageID, pReads);
   }
   if(UserGet(iFromID, &szName, false, iDate) != NULL)
   {
      pOut->AddChild("fromid", iFromID);
      if(pFrom != NULL)
      {
         pOut->AddChild("fromname", pFrom);
      }
      else
      {
         pOut->AddChild("fromname", szName);
      }
   }
   if(pFolder != NULL && pFolder->GetID() != iRelatedID)
   {
      pOut->AddChild(MessageTreeID(iBase), pFolder->GetID());
      pOut->AddChild(MessageTreeName(iBase), pFolder->GetName());
   }

   pOut->Parent();
}

/*
** FolderMessageItemList: Insert message item into EDF hierarchy
*/
bool FolderMessageItemList(EDF *pOut, int iLevel, FolderMessageItem *pItem, int iBase, MessageTreeItem *pFolder, int iTree, int iUserID, int iAccessLevel, int iSubType, bool bArchive, bool bMarkRead, bool bSaved, ConnData *pConnData, DBSub *pFolders, DBMessageRead *pReads, DBMessageSave *pSaves, EDF *pIn, int iDefOp, int iAttachmentID, int *iMinID, int *iMaxID, int *iNumMsgs)
{
   STACKTRACE
   int iChildNum = 0, iNumChildren = 0, iAllReplies = 0, iNumReplies = 0, iFolderID = -1, iHiddenID = 0, iHiddenReplies = 0;
   int iParentID = -1, iVoteID = 0;
   int iReplyByID = 0, iReplyByFolder = 0, iReplyByDate = 0, iReplyByFrom = 0;
   bool bLoop = false, bDeleted = false, bWrite = true, bWritten = false;
   char *szName = NULL;
   char szSQL[200];
   FolderMessageItem *pChild = NULL, *pParent = NULL;
   DBTable *pTable = NULL;
   UserItem *pUser = pConnData->m_pUser;

   debug(DEBUGLEVEL_DEBUG, "FolderMessageItemList l=%d i=%p(%d) b=%d f=%p(%s) t=%d u=%d a=%d s=%d m=%s a=%s d=%p f=%p r=%p s=%p i=%p a=%d m=%p m=%p\n",
      iLevel, pItem, pItem != NULL ? pItem->GetID() : -1, iBase, pFolder, pFolder != NULL ? pFolder->GetName(false) : NULL, iTree,
      iUserID, iAccessLevel, iSubType, BoolStr(bArchive), BoolStr(bMarkRead), pConnData, pFolders, pReads, pSaves, pIn, iAttachmentID, iMinID, iMaxID);

   STACKTRACEUPDATE

   if(pItem != NULL && pFolder != NULL && mask(pFolder->GetAccessMode(), ACCMODE_PRIVATE) == true)
   {
      if(MessageAccess(pItem, iUserID, iAccessLevel, iSubType) == 0)
      {
         // printf("MessageListSingle no access to private message %ld by user %d level %d sub %d\n", pItem->GetID(), iUserID, iAccessLevel, iSubType);
         // return false;
         bWrite = false;
      }
   }

   STACKTRACEUPDATE

   if(pItem != NULL)
   {
      // Message output
      /* if(iTree == 0)
      {
         debug("MessageListSingle %p\n", pItem);
      } */
      // printf(", %ld / %s\n", pItem->GetID(), pItem->GetSubject());

      debug(DEBUGLEVEL_DEBUG, "FolderMessageItemList write=%s, tree %p / %p\n", BoolStr(bWrite), pFolder, pItem->GetTree());

      if(bWrite == true && (pFolder == NULL || pItem->GetTree() == pFolder))
      {
         if(pItem->GetDeleted() == false || iAccessLevel >= LEVEL_WITNESS)
         {
            if(iTree == FMIL_FOLDER && iMinID != NULL && iMaxID != NULL)
            {
               if(iMinID != NULL && (*iMinID == -1 || pItem->GetID() < *iMinID))
               {
                  debug(DEBUGLEVEL_DEBUG, "FolderMessageItemList set min ID %d\n", *iMinID);
                  *iMinID = pItem->GetID();
               }
               if(iMaxID != NULL && (*iMaxID == -1 || pItem->GetID() > *iMaxID))
               {
                  debug(DEBUGLEVEL_DEBUG, "FolderMessageItemList set max ID %d\n", *iMinID);
                  *iMaxID = pItem->GetID();
               }
            }

            STACKTRACEUPDATE

            // Any folder or only messages in required folder
            if(iTree != FMIL_SEARCH || MessageMatch(pItem, pIn, iDefOp) == true || (bSaved == true && pSaves != NULL && pSaves->IsSaved(pItem->GetID()) == true))
            {
               if(iNumMsgs != NULL)
               {
                  debug(DEBUGLEVEL_DEBUG, "FolderMessageItemList inc num msgs\n");
                  (*iNumMsgs)++;
               }

               // Message matched, output to EDF
               pItem->Write(pOut, iLevel);
               bWritten = true;

               // EDFParser::Print("FolderMessageItemList write", pOut, EDFElement::EL_CURR | EDFElement::PR_SPACE);

               /* if(pOut->Child("votes") == true)
               {
                  debugEDFPrint("MessageListSingle vote section", pOut, EDFElement::EL_CURR | EDFElement::PR_SPACE);
                  pOut->Parent();
               } */

               if(iBase == RFG_BULLETIN)
               {
                  pOut->Set("bulletin", pItem->GetID());
               }
               else if(iBase == RFG_CONTENT)
               {
                  pOut->Set("object", pItem->GetID());

                  if(mask(iLevel, EDFITEMWRITE_ADMIN) == false)
                  {
                     pOut->DeleteChild("fromid");
                  }
               }

               if(pFolder != NULL && mask(iLevel, FMLS_FOLDERINFO) == true)
               {
                  pOut->AddChild("folderid", pFolder->GetID());
                  pOut->AddChild("foldername", pFolder->GetName());
               }
               else if(iTree == FMIL_SEARCH)
               {
                  if(FolderGet(pItem->GetTreeID(), &szName, false) != NULL)
                  {
                     pOut->AddChild("folderid", pItem->GetTreeID());
                     pOut->AddChild("foldername", szName);
                  }
               }

               if(mask(iLevel, MESSAGEITEMWRITE_ATTDATA) == true)
               {
                  debug("FolderMessageItemList no action for attdata\n");
               }
               else
               {
                  // Fill out extra fields message does not store
                  if(mask(iLevel, MESSAGEITEMWRITE_DETAILS) == true)
                  {
                     if(pOut->IsChild("fromid") == true && pOut->IsChild("fromname") == false)
                     {
                        if(UserGet(pItem->GetFromID(), &szName, false, pItem->GetDate()) != NULL)
                        {
                           pOut->AddChild("fromname", szName);
                        }
                        else
                        {
                           pOut->DeleteChild("fromid");
                        }
                     }
                  }
                  if(mask(iLevel, MESSAGEITEMWRITE_DETAILS) == true && pOut->IsChild("toname") == false)
                  {
                     if(UserGet(pItem->GetToID(), &szName, false, pItem->GetDate()) != NULL)
                     {
                        pOut->AddChild("toname", szName);
                     }
                     else
                     {
                        pOut->DeleteChild("toid");
                     }
                  }

                  if(pReads != NULL)
                  {
                     // Check marking
                     if(MessageListRead(pOut, pItem->GetID(), pReads) == false && iTree == FMIL_SINGLE && bMarkRead)
                     {
                        // debug("FolderMessageItemList marking read\n");
                        // bDebug = DBMessageRead::Debug(true);
                        pReads->Add(pItem->GetID(), 0);
                        // DBMessageRead::Debug(bDebug);
                     }
                  }

				  if(pSaves != NULL)
				  {
					  if(pSaves->IsSaved(pItem->GetID()) == true)
					  {
						  pOut->AddChild("saved", true);
					  }
				  }

                  // printf("MessageListSingle %d children in message\n", pItem->Count());
               }

               if(iTree == FMIL_SINGLE)
               {
                  // Single message only fields
                  if(mask(iLevel, MESSAGEITEMWRITE_ATTDATA) == false)
                  {
                     iParentID = pItem->GetParentID();
                     pParent = (FolderMessageItem *)pItem->GetParent();
                     iFolderID = pItem->GetTreeID();
                     /* if(pParent != NULL)
                     {
                        iFolderID = pParent->GetTreeID();
                     } */

                     // printf("MessageListSingle replyto compare %p %p\n", pParent, pItem->GetParent());

                     while(pParent != NULL)
                     {
                        // Fill out replyto (ie parents)
                        // printf("MessageListSingle replyto %p, %ld\n", pParent, pParent->GetID());

                        iParentID = pParent->GetParentID();
                        pParent = MessageListReply(pParent, &iFolderID, iUserID, iAccessLevel, pFolders);

                        if(pParent != NULL)
                        {
                           FolderMessageItemListReply(pOut, "replyto", pParent->GetID(), pParent->GetDate(), pParent->GetFromID(), pParent->GetFromName(),
                              iBase, pParent->GetTree(), pItem->GetTreeID(), pReads);

                           pParent = (FolderMessageItem *)pParent->GetParent();
                        }
                     }

                     if(iAccessLevel >= LEVEL_WITNESS && pParent == NULL && iParentID != -1)
                     {
                        debug("MessageListSingle missing parent %d\n", iParentID);
                        pOut->Add("replyto", iParentID);
                        pOut->AddChild("exist", false);
                        pOut->Parent();
                     }

                     // Fill out replyby (ie children)
                     iNumChildren = pItem->Count(false);
                     iAllReplies = pItem->Count(true);
                     iFolderID = pItem->GetTreeID();

                     for(iChildNum = 0; iChildNum < iNumChildren; iChildNum++)
                     {
                        pChild = (FolderMessageItem *)pItem->Child(iChildNum);
                        iHiddenID = pChild->GetID();
                        iNumReplies = pChild->Count(true);

                        pChild = MessageListReply(pChild, &iFolderID, iUserID, iAccessLevel, pFolders);

                        if(pChild != NULL)
                        {
                           FolderMessageItemListReply(pOut, "replyby", pChild->GetID(), pChild->GetDate(),
                              pChild->GetFromID(), pChild->GetFromName(), iBase, pChild->GetTree(), pItem->GetTreeID(),  pReads);
                        }
                        else
                        {
                           iFolderID = pItem->GetTreeID();

                           // iNumReplies = pChild->Count();

                           debug("MessageListSingle not adding reply %d or %d children\n", iHiddenID, iNumReplies);

                           iHiddenReplies += iNumReplies + 1;
                        }
                     }

                     if(bArchive == true)
                     {
                        sprintf(szSQL, "select messageid,folderid,message_date,fromid from %s where parentid = %ld", FMI_TABLENAME, pItem->GetID());

                        pTable = new DBTable();
                        pTable->BindColumnInt();
                        pTable->BindColumnInt();
                        pTable->BindColumnInt();
                        pTable->BindColumnInt();
                        if(pTable->Execute(szSQL) == true)
                        {
                           while(pTable->NextRow() == true)
                           {
                              pTable->GetField(0, &iReplyByID);
                              pTable->GetField(1, &iReplyByFolder);
                              pTable->GetField(2, &iReplyByDate);
                              pTable->GetField(3, &iReplyByFrom);

                              debug("FolderMessageItemList replyby %d %d %d %d\n", iReplyByID, iReplyByFolder, iReplyByDate, iReplyByFrom);

                              FolderMessageItemListReply(pOut, "replyby", iReplyByID, iReplyByDate, iReplyByFrom, NULL, iBase, FolderGet(iReplyByFolder), pItem->GetTreeID(), NULL);
                           }
                        }
                        delete pTable;
                     }

                     if(iHiddenReplies > 0)
                     {
                        debug("MessageListSingle %d counted children, %d hidden replies\n", iAllReplies, iHiddenReplies);
                     }
                     iNumReplies = iAllReplies - iHiddenReplies;

                     if(iNumReplies > 0)
                     {
                        pOut->AddChild("numreplies", iNumReplies);
                     }

                     if(pFolder != NULL && mask(pItem->GetType(), MSGTYPE_ARCHIVE) == false)
                     {
                        pOut->AddChild("msgpos", pFolder->MessagePos(pItem->GetID()));
                     }

                     if(pOut->Child("votes") == true)
                     {
                        // EDFPrint("MessageListSingle votes section", pOut, EDFElement::EL_CURR | EDFElement::PR_SPACE);

                        bLoop = pOut->Child("vote");
                        while(bLoop == true)
                        {
                           MessageListUsers(pOut);

                           bLoop = pOut->Next("vote");
                           if(bLoop == false)
                           {
                              pOut->Parent();
                           }
                        }

                        MessageListUsers(pOut);

                        if(pConnData != NULL)
                        {
                           iVoteID = pItem->GetVoteUser(pConnData->m_pUser->GetID());
                           if(iVoteID > 0)
                           {
                              if(EDFFind(pOut, "vote", iVoteID, false) == true)
                              {
                                 pOut->AddChild("voted", true);
                                 pOut->Parent();
                              }
                           }
                           else if(iVoteID == 0)
                           {
                              pOut->AddChild("voted", true);
                           }
                        }

                        pOut->Parent();
                     }

                     FolderMessageListDetail(pOut, "delete", NULL, pUser, pFolders);
                     FolderMessageListDetail(pOut, "edit", NULL, pUser, pFolders);
                     FolderMessageListDetail(pOut, "move", NULL, pUser, pFolders);
                  }
                  else
                  {
                     pItem->WriteAttachment(pOut, iAttachmentID, iLevel);
                  }

                  FolderMessageListDetail(pOut, "attachment", "date", NULL, NULL);
               }
               else if(iTree == FMIL_SEARCH)
               {
                  // Finish element now
                  pOut->Parent();
               }
            }
         }
         else
         {
            bDeleted = true;
         }

         STACKTRACEUPDATE
      }

      STACKTRACEUPDATE

      if(iTree != FMIL_SINGLE)
      {
         // Tree mode - iterate over children
         iNumChildren = pItem->Count(false);
         for(iChildNum = 0; iChildNum < iNumChildren; iChildNum++)
         {
            pChild = (FolderMessageItem *)pItem->Child(iChildNum);
            debug(DEBUGLEVEL_DEBUG, "FolderMessageItemList child %d %p\n", iChildNum, pChild);
            FolderMessageItemList(pOut, iLevel, pChild, iBase, pFolder, iTree, iUserID, iAccessLevel, iSubType, bArchive, bMarkRead, bSaved, pConnData, pFolders, pReads, pSaves, pIn, iDefOp, iAttachmentID, iMinID, iMaxID, iNumMsgs);
         }
      }

      if(bWritten == true && bDeleted == false && iTree != FMIL_SEARCH)
      {
         // Finish element now
         pOut->Parent();
      }
   }
   else if(pFolder != NULL)
   {
      // Tree mode - iterate over children
      iNumChildren = pFolder->MessageCount(false);
      for(iChildNum = 0; iChildNum < iNumChildren; iChildNum++)
      {
         pChild = (FolderMessageItem *)pFolder->MessageChild(iChildNum);
         debug(DEBUGLEVEL_DEBUG, "FolderMessageItemList child %d %p\n", iChildNum, pChild);
         FolderMessageItemList(pOut, iLevel, pChild, iBase, pFolder, iTree, iUserID, iAccessLevel, iSubType, bArchive, bMarkRead, bSaved, pConnData, pFolders, pReads, pSaves, pIn, iDefOp, iAttachmentID, iMinID, iMaxID, iNumMsgs);
      }
   }

   return true;
}

bool MessageListFolder(MessageTreeItem *pFolder, int iFolderID, bool bFolderCheck,
                       ConnData *pConnData, EDF *pIn, int iDefOp, EDF *pOut, int iBase, int iLevel, int iTree, bool bArchive, bool bMarkRead, bool bSaved,
                       int *iMinID, int *iMaxID, int *iNumMsgs, MessageTreeItem **pReturn)
{
   STACKTRACE
   bool bCheck = true, bReturn = false;;
   int iUserID = 0, iAccessLevel = 0, iSubType = 0;
   DBSub *pFolders = NULL;
   DBMessageRead *pReads = NULL;
   DBMessageSave *pSaves = NULL;
   UserItem *pCurr = NULL;

   if(pConnData != NULL)
   {
      pCurr = pConnData->m_pUser;
      if(pCurr != NULL)
      {
         iUserID = pCurr->GetID();
         iAccessLevel = pCurr->GetAccessLevel();
      }
      pFolders = pConnData->m_pFolders;
      pReads = pConnData->m_pReads;
	  pSaves = pConnData->m_pSaves;
   }

   if(bFolderCheck == true)
   {
      // Check access
      if(pFolder != NULL)
      {
         bCheck = MessageTreeAccess(RFG_FOLDER, MTA_MESSAGETREE_READ, pFolder, pCurr, pFolders, NULL, &iSubType);
      }
      else
      {
         bCheck = MessageTreeAccess(RFG_FOLDER, MTA_MESSAGETREE_READ, iFolderID, pCurr, pFolders, pOut, &pFolder, NULL, &iSubType);
      }
   }

   if(pReturn != NULL)
   {
      *pReturn = pFolder;
   }

   if(bCheck == false)
   {
      debug("MessageListFolder check %d(%p,%s) failed\n", iFolderID, pFolder, pFolder != NULL ? pFolder->GetName() : NULL);
      return false;
   }

   // debug("MessageListFolder %p(%d %d)\n", pCurr, iUserID, iAccessLevel);

   bReturn = FolderMessageItemList(pOut, iLevel, NULL, iBase, pFolder, iTree, iUserID, iAccessLevel, iSubType, bArchive, bMarkRead, bSaved, pConnData, pFolders, pReads, pSaves, pIn, iDefOp, -1, iMinID, iMaxID, iNumMsgs);

   return bReturn;
}

/*
** MessageThread: Perform request on messages in hierarchy according to search matches
** Possible iType values:
** 0 - This message only
** 1 - This message and children
** 2 - Only children
*/
int MessageThreadAction(char *szRequest, int iType, bool bCrossFolder, int iThread, EDF *pIn, bool bMatchFields,
                  FolderMessageItem *pFolderMessage, int iFolderID, MessageTreeItem *pFolder1, MessageTreeItem *pFolder2,
                  int iUserID, int iAccessLevel, ConnData *pConnData, DBSub *pFolders, DBMessageRead *pReads, DBMessageSave *pSaves, EDF *pOut)
{
   int iChildNum = 0, iNumChildren = 0, iReturn = 0;;//, iDebug = 0;
   bool bCheck = true, bChange = false;
   FolderMessageItem *pParent = NULL, *pChild = NULL;
   MessageTreeItem *pParentTree = NULL;

   // debug("MessageThread entry %s %d %s\n", szRequest, iType, BoolStr(bCrossFolder));
   debug("MessageThreadAction r=%s ty=%d cf=%s th=%d i=%p mf=%s fm=%p(%ld) fid=%d f1=%p(%ld) f2=%p(%ld) uid=%d al=%d cd=%p f=%p r=%p s=%p o=%p\n",
      szRequest, iType, BoolStr(bCrossFolder), iThread, pIn, BoolStr(bMatchFields),
      pFolderMessage, pFolderMessage != NULL ? pFolderMessage->GetID() : -1, iFolderID, pFolder1, pFolder1 != NULL ? pFolder1->GetID() : -1, pFolder2, pFolder2 != NULL ? pFolder2->GetID() : -1,
                  iUserID, iAccessLevel, pConnData, pFolders, pReads, pSaves, pOut);

   if(pFolderMessage != NULL && mask(iType, THREAD_CHILD) == false)
   {
      // debug("MessageThread folder %d %d\n", iFolderID, pFolderMessage->GetTreeID());

      // Check boolean is a per message setting
      if(iFolderID != pFolderMessage->GetTreeID())
      {
         if(stricmp(szRequest, MSG_MESSAGE_DELETE) == 0)
         {
            // Only admin and editors can delete messages
            if(iAccessLevel < LEVEL_WITNESS && pFolders->SubType(pFolderMessage->GetTreeID()) < SUBTYPE_EDITOR)
            {
               bCheck = false;
            }
         }
         else
         {
            // Private messages cannot be moved
            if(pFolderMessage->GetTree() != NULL && mask(pFolderMessage->GetTree()->GetAccessMode(), ACCMODE_PRIVATE) == true)
            {
               bCheck = false;
            }
         }
      }
      
      // debug("MessageThreadAction check point 1\n");

      if(bCheck == true && (bMatchFields == false || MessageMatch(pFolderMessage, pIn, MATCH_AND, NULL) == true))
      {
         // printf("MessageThread matched %ld\n", pFolderMessage->GetID());

         // bDebug = DBMessageRead::Debug(true);

         if(stricmp(szRequest, MSG_MESSAGE_DELETE) == 0 && pFolderMessage->GetDeleted() == false)
         {
            pFolderMessage->SetDeleted(true);
            pFolderMessage->AddDelete(iUserID);

            pOut->AddChild("messageid", pFolderMessage->GetID());
            iReturn++;
         }
         else if(stricmp(szRequest, MSG_MESSAGE_MOVE) == 0) // && (iAccessLevel >= LEVEL_WITNESS || mask(pFolderMessage->GetType(), MSGTYPE_VOTE) == false))
         {
            pParent = (FolderMessageItem *)pFolderMessage->GetParent();
            if(pParent == NULL)
            {
               // Top level message
               debug("MessageThreadAction remove from %ld(%s), add to %ld(%s)\n", pFolder1->GetID(), pFolder1->GetName(), pFolder2->GetID(), pFolder2->GetName());
               pFolder1->MessageDelete(pFolderMessage->GetID());

               pFolder2->MessageAdd(pFolderMessage);
            }
            else
            {
               // Not top level message - check for reattach
               if(pFolder2->MessageFind(pParent->GetID()) != NULL)
               {
                  // Parent message in this folder - put it back in the right place
                  debug("MessageThreadAction reattach to %ld\n", pParent->GetID());
                  pParent->Add(pFolderMessage);
               }
               else
               {
                  debug("MessageThreadAction add to %ld(%s)\n", pFolder2->GetID(), pFolder2->GetName());
                  pFolder2->MessageAdd(pFolderMessage);
               }
            }

            for(iChildNum = 0; iChildNum < pFolderMessage->Count(false); iChildNum++)
            {
               pChild = (FolderMessageItem *)pFolderMessage->Child(iChildNum);
               debug("MessageThreadAction child %ld, folder %ld(%s)\n", pChild->GetTreeID(), pFolder2->GetID(), pFolder2->GetName());
               if(pChild->GetTree() == pFolder2 && pFolder2->MessageFind(pChild->GetID(), false) != NULL)
               {
                  // Move child folder from folder back to the right place
                  debug("MessageThreadAction remove child %d (%ld) from folder level\n", iChildNum, pChild->GetID());
                  pFolder2->MessageDelete(pChild->GetID());
               }
            }

            if(pFolderMessage->GetTree() != pFolder2)
            {
               pFolderMessage->AddMove(iUserID);
               bChange = true;
            }
            pFolderMessage->SetTree(pFolder2);

            // iDebug = debuglevel(DEBUGLEVEL_DEBUG);
            pFolderMessage->Update();
            // debuglevel(iDebug);

            if(bChange == true)
            {
               pOut->AddChild("messageid", pFolderMessage->GetID());
               iReturn++;
            }
         }
         else if((stricmp(szRequest, MSG_MESSAGE_MARK_READ) == 0 && pReads->Add(pFolderMessage->GetID(), iThread) == true) ||
            (stricmp(szRequest, MSG_MESSAGE_MARK_UNREAD) == 0 && pReads->Delete(pFolderMessage->GetID()) == true))
         {
            pOut->Add("messageid", pFolderMessage->GetID());
            if(iFolderID != pFolderMessage->GetTreeID())
            {
               pOut->AddChild("folderid", pFolderMessage->GetTree()->GetID());
               pOut->AddChild("foldername", pFolderMessage->GetTree()->GetName());
            }
            pOut->Parent();

            iReturn++;
         }
         else if((stricmp(szRequest, MSG_MESSAGE_MARK_SAVE) == 0 && pSaves->Add(pFolderMessage->GetID()) == true) ||
            (stricmp(szRequest, MSG_MESSAGE_MARK_UNSAVE) == 0 && pSaves->Delete(pFolderMessage->GetID()) == true))
         {
            pOut->Add("messageid", pFolderMessage->GetID());
            if(iFolderID != pFolderMessage->GetTreeID())
            {
               pOut->AddChild("folderid", pFolderMessage->GetTree()->GetID());
               pOut->AddChild("foldername", pFolderMessage->GetTree()->GetName());
            }
            pOut->Parent();

            iReturn++;
         }
         else
         {
            debug("MessageThreadAction no action for %s on %d\n", szRequest, pFolderMessage->GetID());
         }

         // DBMessageRead::Debug(bDebug);
      }
   }
   
   // debug("MessageThreadAction check point 2 %p %p\n", pFolderMessage, pFolder1);

   if(mask(iType, THREAD_MSGCHILD) == true || mask(iType, THREAD_CHILD) == true)
   {
      // Children depend on input folder / message
      if(pFolderMessage != NULL)
      {
	 pParentTree = pFolderMessage->GetTree();
         iNumChildren = pFolderMessage->Count(false);
      }
      else
      {
	 pParentTree = pFolder1;
         iNumChildren = pFolder1->MessageCount(false);
      }
      // debug("MessageThreadAction %d children\n", iNumChildren);
      
      // Check children
      for(iChildNum = 0; iChildNum < iNumChildren; iChildNum++)
      {
         if(pFolderMessage != NULL)
         {
            pChild = (FolderMessageItem *)pFolderMessage->Child(iChildNum);
         }
         else
         {
            pChild = (FolderMessageItem *)pFolder1->MessageChild(iChildNum);
         }

         if(mask(iType, THREAD_CHILD) == true || bCrossFolder == true || CrossFolder(iFolderID, pChild->GetTree()) == true)
         {
            iReturn += MessageThreadAction(szRequest, THREAD_MSGCHILD, bCrossFolder, iThread, pIn, bMatchFields, pChild, iFolderID, pFolder1, pFolder2, iUserID, iAccessLevel, pConnData, pFolders, pReads, pSaves, pOut);
         }
         /* else
         {
            debug("MessageThreadAction cross folder failure %d %d\n", pChild->GetTreeID(), pFolderMessage->GetTreeID());
         } */
      }
   }

   // debug("MessageThread exit %d\n", iReturn);
   return iReturn;
}

int MessageArchive(EDF *pOut, int iLevel, int iBase, int iUserID, int iAccessLevel, ConnData *pConnData, DBSub *pFolders, EDF *pIn)
{
   STACKTRACE
   int iDebug = debuglevel(DEBUGLEVEL_DEBUG), iReturn = 0;
   double dEntry = gettick(), dTick = 0;
   char *szSQL = NULL;
   DBTable *pTable = NULL;
   FolderMessageItem *pFolderMessage = NULL;

   EDFParser::debugPrint("MessageArchive entry", pIn);

   szSQL = MessageSQL(pIn);
   if(szSQL != NULL)
   {
      pTable = new DBTable();

      dTick = gettick();
      debug("MessageArchive running query: %s\n", szSQL);
      if(pTable->Execute(szSQL) == true)
      {
         debug("MessageArchive query time %ld ms\n", tickdiff(dTick));

         do
         {
            pFolderMessage = FolderMessageItem::NextRow(pTable, NULL, NULL);

            if(pFolderMessage != NULL)
            {
               debug("MessageArchive folder message %p\n", pFolderMessage);
               FolderMessageItemList(pOut, iLevel, pFolderMessage, iBase, NULL, FMIL_SINGLE, iUserID, iAccessLevel, 0, false, false, false, pConnData, pFolders, NULL, NULL, NULL, 0, 0, NULL, NULL, NULL);

               delete pFolderMessage;

               iReturn++;
            }
         }
         while(pFolderMessage != NULL);
      }
   }
   delete[] szSQL;

   // EDFParser::debugPrint("MessageArchive output", pOut);
   debug("MessageArchive exit %d, %ld ms\n", iReturn, tickdiff(dEntry));
   debuglevel(iDebug);
   return iReturn;
}

bool MessageVoteClose(EDF *pData, int iBase, FolderMessageItem *pItem, bool bAnnounce)
{
   STACKTRACE
   int iListNum = 0;
   bool bReturn = false;
   EDF *pAnnounce = NULL;
   EDFConn *pListConn = NULL;
   ConnData *pListData = NULL;
   MessageTreeItem *pFolder = NULL;

   if(mask(pItem->GetVoteType(), VOTE_CLOSED) == false)
   {
      pFolder = pItem->GetTree();

      debug("MessageVoteClose %ld current vote type %d\n", pItem->GetID(), pItem->GetVoteType());
      pItem->SetVoteType(pItem->GetVoteType() | VOTE_CLOSED);
      if(mask(pItem->GetVoteType(), VOTE_PUBLIC_CLOSE) == true && mask(pItem->GetVoteType(), VOTE_PUBLIC) == false)
      {
         pItem->SetVoteType(pItem->GetVoteType() | VOTE_PUBLIC);
      }
      debug("MessageVoteClose new vote type %d\n", pItem->GetVoteType());

      for(iListNum = 0; iListNum < ConnCount(); iListNum++)
      {
         pListConn = ConnList(iListNum);
         pListData = (ConnData *)pListConn->Data();
         if(pListData->m_pUser != NULL && pListData->m_pReads != NULL)
         {
            MessageMarking(pListData->m_pUser, pListData->m_pReads, MSG_MESSAGE_EDIT, pItem, MARKED_UNREAD);
         }
      }

      DBMessageRead::UserDelete(-1, pItem->GetID());

      if(bAnnounce == true)
      {
         pAnnounce = new EDF();
         pAnnounce->AddChild(MessageID(iBase), pItem->GetID());
         if(pFolder != NULL)
         {
            pAnnounce->AddChild(MessageTreeID(iBase), pFolder->GetID());
            pAnnounce->AddChild(MessageTreeName(iBase), pFolder->GetName());
         }

         ServerAnnounce(pData, MSG_MESSAGE_EDIT, pAnnounce);
      }

      bReturn = true;
   }

   return bReturn;
}

// EDF message functions
extern "C"
{

/*
** MessageAdd: Create a new message / bulletin
**
** markread:   Specifies whether the message is to be marked read when added (default is false)
** votetype(admin):  Type of vote to use
**    1 - Simple yes / no
**    2 - Multiple option vote (set automatically if vote fields are used)
*/
ICELIBFN bool MessageAdd(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   STACKTRACE
   int iReplyID = -1, iFolderID = -1;
   int iBase = RequestGroup(pIn);
   long lMessageID = -1;
   FolderMessageItem *pItem = NULL, *pParent = NULL;
   ConnData *pConnData = CONNDATA;
   MessageTreeItem *pFolder = NULL;
   UserItem *pCurr = CONNUSER;
   DBSub *pFolders = CONNFOLDERS;

   // STACKTRACEUPDATE

   // EDFPrint("MessageAdd entry", pIn);

   if(iBase == RFG_MESSAGE)
   {
      if(pIn->GetChild("replyid", &iReplyID) == true)
      {
         // Replying to another message
         debug("MessageAdd finding reply ID %d\n", iReplyID);

         if(MessageTreeAccess(iBase, MTA_MESSAGE_READ, iReplyID, pCurr, pFolders, pOut, &pFolder, (MessageItem **)&pParent, NULL) == false)
         {
            // Cannot access message
            return false;
         }

         debug("MessageAdd parent folder %p, message %p\n", pFolder, pParent);
      }

      if(pIn->GetChild("folderid", &iFolderID) == true)
      {
         // Replying to a folder
         debug("MessageAdd folder from ID %d\n", iFolderID);

         pFolder = NULL;

         if(MessageTreeAccess(iBase, MTA_MESSAGE_WRITE, iFolderID, pCurr, pFolders, pOut, &pFolder, NULL, NULL) == false)
         {
            // Cannot access folder
            return false;
         }
      }
      else
      {
         if(pParent != NULL && ((FolderMessageItem *)pParent)->GetReplyID() != -1)
         {
            pFolder = FolderGet(((FolderMessageItem *)pParent)->GetReplyID());
            debug("MessageAdd message reply %p(%ld)\n", pFolder, pFolder->GetID());
         }
         else if(pFolder != NULL && pFolder->GetReplyID() != -1)
         {
            pFolder = FolderGet(pFolder->GetReplyID());
            debug("MessageAdd folder reply %p(%ld)\n", pFolder, pFolder->GetID());
         }

         if(pFolder != NULL)
         {
            // Same folder as reply message
            if(MessageTreeAccess(iBase, MTA_MESSAGE_WRITE, pFolder, pCurr, pFolders, pOut, NULL) == false)
            {
               // Cannot access folder
               return false;
            }
         }
         else
         {
            // Nowhere to put reply
            pOut->Set("reply", MSG_FOLDER_NOT_EXIST);
            return false;
         }
      }
   }
   else if(iBase == RFG_CONTENT)
   {
      if(pIn->GetChild("folderid", &iFolderID) == false)
      {
         pOut->Set("reply", MSG_FOLDER_NOT_EXIST);

         return false;
      }

      if(MessageTreeAccess(iBase, MTA_MESSAGE_WRITE, iFolderID, pCurr, pFolders, pOut, &pFolder, NULL, NULL) == false)
      {
         // Cannot access folder
         return false;
      }

      if(pIn->GetChild("replyid", &iReplyID) == true)
      {
         pParent = (FolderMessageItem *)pFolder->MessageFind(iReplyID);
      }
   }
   else
   {
      pFolder = g_pBulletinList;
   }

   // Create message
   if(iBase == RFG_CONTENT)
   {
      lMessageID = pFolder->GetMaxID();
   }
   else
   {
      lMessageID = FolderMaxID();
   }
   lMessageID++;

   debug("MessageAdd creating message %ld in %ld", lMessageID, pFolder->GetID());
   if(pParent != NULL)
   {
      debug(", parent of %ld in %ld", pParent->GetID(), pParent->GetTree()->GetID());
   }
   debug("\n");

   pItem = new FolderMessageItem(lMessageID, pParent, pFolder);
   if(iBase == RFG_MESSAGE)
   {
      FolderMessageAdd(pItem);
   }

   // debug("MessageAdd class %s\n", pMessage->GetClass());

   // Update parent message and folder hierarchies
   if(pParent != NULL)
   {
      pParent->Add(pItem);

      // pFolderMessage->SetTopID(pMessageParent->GetTopID());
   }
   /* else
   {
      pFolderMessage->SetTopID(pFolderMessage->GetID());
   } */

   if(pItem->GetParent() == NULL)
   {
      pFolder->MessageAdd(pItem);
   }
   else if(pParent != NULL && pParent->GetTree() != pFolder)
   {
      pFolder->MessageAdd(pItem);
   }

   pItem->SetFromID(pCurr->GetID());

   // Set initial fields
   MessageItemEdit(pData, RQ_ADD, pItem, pIn, pOut, pCurr, pConnData, NULL, iBase, false, pFolders->SubType(pFolder->GetID()));

   pCurr->IncStat(USERITEMSTAT_NUMMSGS, 1);
   if(pItem->GetText() != NULL)
   {
      pCurr->IncStat(USERITEMSTAT_TOTALMSGS, pItem->GetText()->Length());
   }
   pCurr->SetStat(USERITEMSTAT_LASTMSG, time(NULL));

   // EDFPrint("MessageAdd exit true", pOut);
   return true;
}

/*
** MessageThread: Generic thread operation function for message_delete, message_move, message_mark_read, message_mark_unread, message_mark_save, message_mark_unsave
**
** messageid:  ID of message (overrides folderid field, delete/move/mark_* optional)
** folderid:      ID of the folder to mark messages in (mark_*)
** [delete|move|mark]type:      Type of thread operation to perform
**    0(default) - Single message only
**    1 - Message and all child messages
**    2 - Only child messages
** moveid:   ID of folder to move to (move)
** startdate:     Timestamp of messages after
** enddate:       Timestamp of messages before
** fromid:        ID of user to mark from
** crossfolder:   Also move messages in other folders from messageid (default is false)
** markkeep:      Mark subsequent children as read (mark_read from marktype 1 and 2)
** saved:         Whether to save the message
*/
ICELIBFN bool MessageThread(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   STACKTRACE
   int iBase = RequestGroup(pIn), iAccessLevel = LEVEL_NONE, iMessageID = -1, iFolderID = -1, iType = 0, iMoveID = -1, iNumMsgs = 0, iActionID = -1, iThread = 0, iSubType = 0;
   // int iTreeAccess = 0;
   bool bMessageID = false, bCrossFolder = false, bMatchFields = false, bAnnounce = true, bTypeField = false;
   char *szRequest = NULL, *szTypeField = NULL;
   EDF *pAnnounce = NULL, *pAdmin = NULL;
   ConnData *pConnData = CONNDATA;
   UserItem *pCurr = CONNUSER;
   DBSub *pFolders = CONNFOLDERS;
   MessageTreeItem *pFolder1 = NULL, *pFolder2 = NULL, *pTemp = NULL;
   FolderMessageItem *pFolderMessage = NULL;
   DBMessageRead *pReads = CONNREADS;
   DBMessageSave *pSaves = CONNSAVES;

   pIn->Get(NULL, &szRequest);

   iAccessLevel = pCurr->GetAccessLevel();

   EDFParser::debugPrint("MessageThread entry", pIn);

   bMessageID = pIn->GetChild("messageid", &iMessageID);
   if(bMessageID == false || iMessageID == -1)
   {
      STACKTRACEUPDATE
      if(stricmp(szRequest, MSG_MESSAGE_MOVE) == 0 || stricmp(szRequest, MSG_MESSAGE_DELETE) == 0 || stricmp(szRequest, MSG_MESSAGE_MARK_SAVE) == 0 || stricmp(szRequest, MSG_MESSAGE_MARK_UNSAVE) == 0)
      {
         pOut->Set("reply", MSG_MESSAGE_NOT_EXIST);

         debug("MessageThread %s exit false, %s\n", szRequest, MSG_MESSAGE_NOT_EXIST);
         delete[] szRequest;
         return false;
      }
      else
      {
         if(pIn->GetChild("folderid", &iFolderID) == false)
         {
            pOut->Set("reply", MSG_FOLDER_NOT_EXIST);

            debug("MessageThread %s exit false, %s\n", szRequest, MSG_FOLDER_NOT_EXIST);
            delete[] szRequest;
            return false;
         }
      }

      if(MessageTreeAccess(iBase, MTA_MESSAGETREE_READ, iFolderID, pCurr, pFolders, pOut, &pFolder1, NULL, NULL) == false)
      {
         debug("MessageThread %s exit false, MTA_MESSAGETREE_READ %d\n", szRequest, iFolderID);
         delete[] szRequest;
         return false;
      }

      bMatchFields = MessageMatchFields(pIn, pConnData);

      iType = THREAD_CHILD;
   }

   if(stricmp(szRequest, MSG_MESSAGE_MOVE) == 0 && pIn->GetChild("moveid", &iMoveID) == false)
   {
      pOut->Set("reply", MSG_FOLDER_NOT_EXIST);

      debug("MessageThread %s exit false, %s\n", szRequest, MSG_FOLDER_NOT_EXIST);
      delete[] szRequest;
      return false;
   }

   if(iMessageID != -1 && MessageTreeAccess(iBase, MTA_MESSAGE_READ, iMessageID, pCurr, pFolders, pOut, &pTemp, (MessageItem **)&pFolderMessage, &iSubType) == false)
   {
      debug("MessageThread %s exit false, MTA_MESSAGE_READ\n", szRequest);
      delete[] szRequest;
      return false;
   }

   if(stricmp(szRequest, MSG_MESSAGE_DELETE) == 0 || stricmp(szRequest, MSG_MESSAGE_MOVE) == 0)
   {
      pFolder1 = pTemp;

      if(iAccessLevel >= LEVEL_WITNESS || iSubType == SUBTYPE_EDITOR)
      {
         bTypeField = true;
      }
      else if(stricmp(szRequest, MSG_MESSAGE_DELETE) == 0)
      {
         if((mask(pFolder1->GetAccessMode(), FOLDERMODE_SUB_ADEL) == true ||
            (mask(pFolder1->GetAccessMode(), FOLDERMODE_SUB_SDEL) == true && pFolderMessage->GetFromID() == pCurr->GetID())) ||
            (iSubType == SUBTYPE_MEMBER &&
            (mask(pFolder1->GetAccessMode(), FOLDERMODE_MEM_ADEL) == true ||
            (mask(pFolder1->GetAccessMode(), FOLDERMODE_MEM_SDEL) == true && pFolderMessage->GetFromID() == pCurr->GetID()))))
         {
         }
         else
         {
            pOut->Set("reply", MSG_ACCESS_INVALID);
            pOut->AddChild("messageid", iMessageID);
            debug("MessageThread %s exit false, %s\n", szRequest, MSG_ACCESS_INVALID);
            delete[] szRequest;
            return false;
         }
      }
      else if(stricmp(szRequest, MSG_MESSAGE_MOVE) == 0)
      {
         if(mask(pFolder1->GetAccessMode(), FOLDERMODE_SUB_MOVE) == true ||
            (iSubType == SUBTYPE_MEMBER && mask(pFolder1->GetAccessMode(), FOLDERMODE_MEM_MOVE) == true))
         {
         }
         else
         {
            pOut->Set("reply", MSG_ACCESS_INVALID);
            pOut->AddChild("messageid", iMessageID);
            debug("MessageThread %s exit false, %s\n", szRequest, MSG_ACCESS_INVALID);
            delete[] szRequest;
            return false;
         }
      }
   }

   STACKTRACEUPDATE

   if(stricmp(szRequest, MSG_MESSAGE_MOVE) == 0 && MessageTreeAccess(iBase, MTA_MESSAGE_WRITE, iMoveID, pCurr, pFolders, pOut, &pFolder2, NULL, NULL) == false)
   {
      debug("MessageThread %s exit false, MTA_MESSAGE_WRITE\n", szRequest);
      delete[] szRequest;
      return false;
   }

   if(pFolder1 != NULL)
   {
      pOut->AddChild("folderid", pFolder1->GetID());
      pOut->AddChild("foldername", pFolder1->GetName());
   }

   if(iType == 0)
   {
      if(bTypeField == true && stricmp(szRequest, MSG_MESSAGE_DELETE) == 0)
      {
         szTypeField = "deletetype";
      }
      else if(bTypeField == true && stricmp(szRequest, MSG_MESSAGE_MOVE) == 0)
      {
         szTypeField = "movetype";
      }
      else if(stricmp(szRequest, MSG_MESSAGE_MARK_READ) == 0 || stricmp(szRequest, MSG_MESSAGE_MARK_UNREAD) == 0)
      {
         szTypeField = "marktype";
      }
      debug("MessageThread type field %s\n", szTypeField);
      if(szTypeField != NULL)
      {
         pIn->GetChild(szTypeField, &iType);
         if(iType < 0 || iType > THREAD_CHILD)
         {
            iType = 0;
         }
         pOut->AddChild(szTypeField, iType);
      }

      if(stricmp(szRequest, MSG_MESSAGE_MARK_READ) == 0 && (mask(iType, THREAD_CHILD) == true || mask(iType, THREAD_MSGCHILD) == true))
      {
         if(pIn->GetChildBool("markkeep") == true)
         {
            pOut->AddChild("markkeep", true);
		 }

         iThread = iType;
      }
   }
   
   if(bMessageID == true)
   {
      bCrossFolder = pIn->GetChildBool("crossfolder");
   }

   if(stricmp(szRequest, MSG_MESSAGE_MOVE) == 0)
   {
      pOut->AddChild("moveid", pFolder2->GetID());
      pOut->AddChild("movename", pFolder2->GetName());
   }

   debug("MessageThread folder message check %s %d %p", szRequest, iType, pFolderMessage);
   if(pFolderMessage != NULL)
   {
      debug("(%ld %ld)", pFolderMessage->GetID(), pFolderMessage->GetTreeID());
   }
   debug(" %p %p\n", pFolder1, pFolder2);

   iNumMsgs = MessageThreadAction(szRequest, iType, bCrossFolder, iThread, pIn, bMatchFields, pFolderMessage, pFolderMessage != NULL ? pFolderMessage->GetTreeID() : iFolderID, pFolder1, pFolder2, pCurr->GetID(), iAccessLevel, pConnData, pFolders, pReads, pSaves, pOut);

   if(iMessageID != -1)
   {
      if(pOut->GetChild("messageid", &iActionID) == false || iActionID != iMessageID)
      {
         pOut->Add("messageid", iMessageID, EDFElement::FIRST);
         pOut->AddChild("action", false);
         pOut->Parent();
      }
   }

   if(iNumMsgs > 0)
   {
      if(stricmp(szRequest, MSG_MESSAGE_DELETE) == 0)
      {
         pOut->AddChild("numdeleted", iNumMsgs);
      }
      else if(stricmp(szRequest, MSG_MESSAGE_MOVE) == 0)
      {
         pOut->AddChild("nummoved", iNumMsgs);
      }
      else if(stricmp(szRequest, MSG_MESSAGE_MARK_READ) == 0 || stricmp(szRequest, MSG_MESSAGE_MARK_UNREAD) == 0)
      {
         pOut->AddChild("nummarked", iNumMsgs);
      }
   }
   else
   {
      debug("MessageThread no messages affected, disabling announcement\n");
      bAnnounce = false;
   }

   if(pFolder1 != NULL)
   {
      pOut->AddChild("nummsgs", pFolder1->GetNumMsgs());
   }

   if((stricmp(szRequest, MSG_MESSAGE_DELETE) == 0 || stricmp(szRequest, MSG_MESSAGE_MOVE) == 0) && bAnnounce == true)
   {
      pAnnounce = new EDF();
      pAnnounce->Copy(pOut, true, true);
      /* pAnnounce->AddChild(pOut, "numdeleted");
      pAnnounce->AddChild(pOut, "nummoved");
      pAnnounce->AddChild(pOut, "nummsgs"); */
      EDFParser::debugPrint("MessageThread announcement", pAnnounce);

      pAdmin = new EDF();
      pAdmin->Copy(pAnnounce, true, true);
      pAdmin->AddChild("byid", pCurr->GetID());
      pAdmin->AddChild("byname", pCurr->GetName());
      EDFParser::debugPrint("MessageThread admin announcement", pAdmin);

      ServerAnnounce(pData, szRequest, pAnnounce, pAdmin);
   }

   debug("MessageThread %s exit true\n", szRequest);
   EDFParser::debugPrint(pOut);
   delete[] szRequest;
   return true;
}

/*
** MessageEdit: Edit a message
*/
ICELIBFN bool MessageEdit(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   STACKTRACE
   int iBase = RequestGroup(pIn), iID = 0, iSubType = 0;
   UserItem *pCurr = CONNUSER;
   ConnData *pConnData = CONNDATA;
   FolderMessageItem *pFolderMessage = NULL;
   DBSub *pFolders = CONNFOLDERS;

   EDFParser::debugPrint("MessageEdit entry", pIn);

   if(pIn->GetChild(MessageID(iBase), &iID) == false)
   {
      pOut->Set("reply", MessageStr(iBase, "not_exist"));
      debug("MessageEdit exit false, %s\n", MessageTreeStr(iBase, "not_exist"));
      return false;
   }

   if(MessageTreeAccess(iBase, MTA_MESSAGE_EDIT, iID, pCurr, pFolders, pOut, NULL, (MessageItem **)&pFolderMessage, &iSubType) == false)
   {
      return false;
   }

   // Set fields
   MessageItemEdit(pData, RQ_EDIT, pFolderMessage, pIn, pOut, pCurr, pConnData, NULL, iBase, true, iSubType);

   EDFParser::debugPrint("MessageEdit exit true", pOut);
   return true;
}

/*
** MessageList: Generate a list of messages
**
** folderid: ID of the folder to list
** searchtype: Type of list to generate
**              0(default) - low details (ID, subject, read status)
**              1 - high details (complete except for message text)
**              3 - useful cache specific type (ID, date, read status)
** keyword:  Search subject and text fields for this keyword
** fromid:  Search for messages from this user
** toid:    Search for messages to this user
** startdate: Search for messages after this date
** enddate: Search for messages before this date
** messageid: Search for this message only (other search fields ignored)
** markread: Whether to mark messages as read (for messageid only, default is true)
** saved: Search for saved messages only
**
** At least one of folderid, messageid or search fields must be specified.
** The search fields can be combined
*/
ICELIBFN bool MessageList(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   STACKTRACE
   int iBase = RequestGroup(pIn), iFolderID = -1, iID = 0, iType = 0, iLevel = 0, iUserID = -1, iAccessLevel = LEVEL_NONE, iNumMsgs = 0;
   int iTree = 0, iSubType = SUBTYPE_SUB, iAttachmentID = -1, iFolderNum = 0, iMinID = -1, iMaxID = -1, iDebug = 0, iDefOp = MATCH_AND;
   bool bArchive = false, bNoPrivate = false, bMarkRead = true, bSaved = false; //, bLoop = true, bCheck = false;
   double dEntry = gettick();
   MessageTreeItem *pFolder = NULL;
   FolderMessageItem *pFolderMessage = NULL;
   ConnData *pConnData = CONNDATA;
   UserItem *pCurr = CONNUSER;
   DBMessageRead *pReads = CONNREADS;
   DBMessageSave *pSaves = CONNSAVES;
   DBSub *pFolders = CONNFOLDERS;

   if(pCurr != NULL)
   {
      // Get fields if they're present
      iUserID = pCurr->GetID();
      iAccessLevel = pCurr->GetAccessLevel();
   }

   // EDFPrint("MessageList entry", pIn);

   if(iAccessLevel >= LEVEL_WITNESS)
   {
      // Archive is admin only
      iLevel |= EDFITEMWRITE_ADMIN;
      bArchive = pIn->GetChildBool("archive", false);
   }

   if(pIn->GetChild(MessageID(iBase), &iID) == true)
   {
      // Specific message

      if(iBase == RFG_CONTENT)
      {
         // Content needs a folder and message ID
         if(pIn->GetChild("folderid", &iFolderID) == false)
         {
            pOut->Set("reply", MSG_FOLDER_NOT_EXIST);

            // EDFPrint("MessageList exit false", pOut);
            return false;
         }
      }

      if(MessageTreeAccess(iBase, MTA_MESSAGE_READ, iID, iFolderID, &bArchive, pCurr, pFolders, pOut, &pFolder, (MessageItem **)&pFolderMessage, &iSubType) == false)
      {
         // Cannot access message
         return false;
      }

      // Admin rights granted to editors and providers
      if(iSubType == SUBTYPE_EDITOR)
      {
         iLevel |= EDFITEMWRITE_ADMIN;
      }
      if(pFolder != NULL && pCurr != NULL && pFolder->GetProviderID() != -1 && pFolder->GetProviderID() == pCurr->GetID())
      {
         iLevel |= EDFITEMWRITE_ADMIN;
      }

      // More level checking (attachments are asked for separately)
      if(pIn->GetChild("attachmentid", &iAttachmentID) == true)
      {
         iLevel |= MESSAGEITEMWRITE_ATTDATA;
      }
      else
      {
         iLevel |= MESSAGEITEMWRITE_DETAILS + MESSAGEITEMWRITE_TEXT;
      }

      if(pCurr != NULL && pFolderMessage->GetFromID() == pCurr->GetID())
      {
    iLevel |= FOLDERMESSAGEITEMWRITE_SELF;
      }

      bMarkRead = pIn->GetChildBool("markread", true);
      FolderMessageItemList(pOut, iLevel, pFolderMessage, iBase, pFolder, FMIL_SINGLE, iUserID, iAccessLevel, iSubType, bArchive, bMarkRead, bSaved, pConnData, pFolders, pReads, pSaves, pIn, iDefOp, iAttachmentID, &iMinID, &iMaxID, &iNumMsgs);

      if(bArchive == true)
      {
         // This message will have been created on the fly
         delete pFolderMessage;
      }

      // EDFPrint("MessageList message", pOut);
   }
   else
   {
      // Get search fields
      if(MessageMatchFields(pIn, pConnData) == true)
      {
         debug("MessageList %ld(%s):\n", pCurr->GetID(), pCurr->GetName());
         EDFParser::debugPrint(pIn);

         iTree = FMIL_SEARCH;
      }
      else
      {
         iTree = FMIL_FOLDER;
      }

	  bSaved = pIn->IsChild("saved");

      if(iBase == RFG_MESSAGE || iBase == RFG_CONTENT)
      {
         // If there wasn't a specific ID, a folder is needed
         if(pIn->GetChild("folderid", &iFolderID) == false)
         {
            if(iTree != FMIL_SEARCH)
            {
               pOut->Set("reply", MSG_FOLDER_NOT_EXIST);

               // EDFPrint("MessageList exit false", pOut);
               return false;
            }

            iLevel |= FMLS_FOLDERINFO;
         }
      }

      // Get details level
      pIn->GetChild("searchtype", &iType);
      if(iType < 0 || iType > 3)
      {
         iType = 0;
      }

      if(iType == 0)
      {
         iLevel |= MESSAGEITEMWRITE_SUBJECT;
      }
      else if(iType == 1)
      {
         iLevel |= MESSAGEITEMWRITE_DETAILS;
      }
      else if(iType == 3)
      {
         iLevel = MESSAGEITEMWRITE_CACHE;
      }
      /* if(iAccessLevel >= LEVEL_WITNESS)
      {
         iLevel |= EDFITEMWRITE_ADMIN;
      } */

      if(iBase == RFG_MESSAGE && bArchive == true)
      {
         MessageArchive(pOut, iLevel, iBase, iUserID, iAccessLevel, pConnData, pFolders, pIn);
         EDFParser::debugPrint("MessageList archive", pOut);
      }
      else
      {
         if(iBase == RFG_BULLETIN)
         {
            // Bulletins
            MessageListFolder(g_pBulletinList, -1, false, pConnData, pIn, MATCH_AND, pOut, iBase, iLevel, iTree, bArchive, bMarkRead, bSaved, &iMinID, &iMaxID, NULL, NULL);
         }
         else if(iFolderID != -1)
         {
            if(ConnVersion(pConnData, "2.7") < 0)
            {
               pIn->DeleteChild("folderid");
               iDefOp = MATCH_OR;
            }

            // Messages in single folder
            // iDebug = debuglevel(DEBUGLEVEL_DEBUG);
            MessageListFolder(NULL, iFolderID, true, pConnData, pIn, iDefOp, pOut, iBase, iLevel, iTree, bArchive, bMarkRead, bSaved, &iMinID, &iMaxID, NULL, &pFolder);
            // debuglevel(iDebug);
         }
         else
         {
            bNoPrivate = pIn->GetChildBool("noprivate");

            // Messages in multiple folders
            for(iFolderNum = 0; iFolderNum < FolderCount(false); iFolderNum++)
            {
               pFolder = FolderList(iFolderNum, false);
               if(bNoPrivate == false || mask(pFolder->GetAccessMode(), ACCMODE_PRIVATE) == false)
               {
                  MessageListFolder(pFolder, -1, true, pConnData, pIn, MATCH_AND, pOut, iBase, iLevel, iTree, bArchive, bMarkRead, bSaved, &iMinID, &iMaxID, &iNumMsgs, NULL);
               }
               pFolder = NULL;
            }
         }
      }
   }

   // Output fields
   pOut->AddChild("searchtype", iType);
   if(pFolder != NULL)
   {
      // Single folder fields
      if(pFolder->GetID() > 0)
      {
         pOut->AddChild("folderid", pFolder->GetID());
         pOut->AddChild("foldername", pFolder->GetName());
      }

      pOut->AddChild("nummsgs", pFolder->GetNumMsgs());
      if(iAccessLevel >= LEVEL_WITNESS || iSubType == SUBTYPE_EDITOR)
      {
         pOut->AddChild("totalmsgs", pFolder->GetTotalMsgs());
      }

      if(iMinID != -1)
      {
         pOut->AddChild("minid", iMinID);
      }
      if(iMaxID != -1)
      {
         pOut->AddChild("maxid", iMaxID);
      }
   }
   else if(iNumMsgs > 0)
   {
      pOut->AddChild("nummsgs", iNumMsgs);
   }

   // EDFPrint("MessageList exit true", pOut);
   // printf("MessageList exit true\n");
   if((bArchive == false && tickdiff(dEntry) > 500) || iBase == RFG_BULLETIN)
   {
      // What was it doing?
      debug("MessageList %ld ms\n", tickdiff(dEntry));
      EDFParser::debugPrint(pIn);
   }
   /* if(bArchive == true)
   {
      EDFParser::debugPrint("MessageList archive", pOut);
   } */
   return true;
}

/*
** MessageVote: Vote on a message
**
** folderid(required): ID of folder message is in
** messageid(required): ID of message in folder
** voteid(required): ID of vote number
**
** Non-admin users cannot change their vote once they have voted
*/
ICELIBFN bool MessageVote(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   STACKTRACE
   int iBase = RequestGroup(pIn), iAccessLevel = LEVEL_NONE, iMessageID = -1, iVoteID = -1, iVoteType = 0;
   long lVoteValue = 0;
   double dVoteValue = 0;
   char *szVoteValue = NULL;
   FolderMessageItem *pFolderMessage = NULL;
   MessageTreeItem *pFolder = NULL;
   UserItem *pCurr = CONNUSER;
   DBSub *pFolders = CONNFOLDERS;

   iAccessLevel = pCurr->GetAccessLevel();

   EDFParser::debugPrint("MessageVote entry", pIn);

   if(pIn->GetChild(MessageID(iBase), &iMessageID) == false)
   {
      pOut->Set("reply", MessageTreeStr(iBase, "not_exist"));

      debug("MessageVote exit false, %s\n", MSG_MESSAGE_NOT_EXIST);
      return false;
   }

   if(MessageTreeAccess(iBase, MTA_MESSAGE_READ, iMessageID, pCurr, pFolders, pOut, &pFolder, (MessageItem **)&pFolderMessage, NULL) == false)
   {
      debug("MessageVote exit false, MTA_MESSAGE_READ\n");
      return false;
   }

   pOut->AddChild(MessageID(iBase), pFolderMessage->GetID());
   if(pFolderMessage->GetTree() != NULL)
   {
      pOut->AddChild("folderid", pFolderMessage->GetTree()->GetID());
      pOut->AddChild("foldername", pFolderMessage->GetTree()->GetName());
   }

   if(pIn->GetChild("voteid", &iVoteID) == false && pIn->TypeGetChild("votevalue", &szVoteValue, &lVoteValue, &dVoteValue) == -1)
   {
      pOut->Set("reply", MessageTreeStr(iBase, "access_invalid"));

      debug("MessageVote exit false, %s (no vote ID / value field)\n", MessageTreeStr(iBase, "access_invalid"));
      return false;
   }

   /* if(mask(pFolderMessage->GetVoteType(), VOTE_CHOICE) == true)
   {
      pIn->GetChild("votetype", &iVoteType);
   } */

   /* if(pFolder->GetExpire() > 0)
   {
      iExpire = pFolder->GetExpire();
   }
   else if(pFolder->GetExpire() == 0)
   {
      pData->Root();
      pData->Child("system");
      pData->GetChild("expire", &iExpire);
   }

   if(iExpire != -1 && time(NULL) > pFolderMessage->GetDate() + iExpire)
   {
      pOut->Set("reply", MessageTreeStr(iBase, "access_invalid"));

      debug("MessageVote exit false, %s (after expiry %d)\n", MessageTreeStr(iBase, "access_invalid"), iExpire);
      return false;
   } */

   debug("MessageVote %ld %d %s %d\n", pFolderMessage->GetID(), iVoteID, szVoteValue, lVoteValue, dVoteValue, iVoteType);
   if(pFolderMessage->SetVoteUser(pCurr->GetID(), iVoteID, szVoteValue, lVoteValue, dVoteValue, iVoteType) == false)
   {
      pOut->Set("reply", MessageTreeStr(iBase, "access_invalid"));

      debug("MessageVote exit false, %s\n", MessageTreeStr(iBase, "access_invalid"));
      delete[] szVoteValue;
      return false;
   }

   pCurr->IncStat(USERITEMSTAT_NUMVOTES, 1);

   delete[] szVoteValue;

   EDFParser::debugPrint("MessageVote exit true", pOut);
   return true;
}

}
