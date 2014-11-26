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
** DBMessageSave.cpp: Implementation saved message class
*/

#include <stdio.h>

#include "db/DBTable.h"
#include "DBMessageSave.h"

// Constructor
DBMessageSave::DBMessageSave(long lUserID) : DBItemList<DBMessageSaveData *>(lUserID, true, false)
{
	STACKTRACE
   int iMessageID = 0;
   char szSQL[200];
   DBTable *pTable = NULL;
   DBMessageSaveData *pData = NULL;

   pTable = new DBTable();
   pTable->BindColumnInt();

   sprintf(szSQL, "select messageid from %s where userid = %ld", FM_SAVE_TABLE, lUserID);
   if(pTable->Execute(szSQL) == false)
   {
      debug("DBMessageSave::DBMessageSave query failed\n");
   }
   else
   {
      while(pTable->NextRow() == true)
      {
         if(pTable->GetField(0, &iMessageID) == true)
         {
            pData = new DBMessageSaveData;
            DBItemList<DBMessageSaveData *>::Add(iMessageID, pData, OP_NONE);
         }
      }
   }

   delete pTable;

   SetChanged(false);
}

// Destructor
DBMessageSave::~DBMessageSave()
{
	STACKTRACE
}

bool DBMessageSave::Add(long lMessageID)
{
   int iAdd = 0;
   // int iDebug = debuglevel(DEBUGLEVEL_DEBUG);
   bool bReturn = false;
   DBMessageSaveData *pData = NULL;

   debug(DEBUGLEVEL_DEBUG, "DBMessageSave::Add entry %ld\n", lMessageID);

   pData = new DBMessageSaveData;

   iAdd = DBItemList<DBMessageSaveData *>::Add(lMessageID, pData);

   if(iAdd == ADD_YES || iAdd == ADD_UNDELETE)
   {
      bReturn = true;
   }

   // if(Debug() == true)
   {
      debug(DEBUGLEVEL_DEBUG, "DBMessageSave::Add exit %s\n", BoolStr(bReturn));
   }
   // debuglevel(iDebug);
   return bReturn;
}

bool DBMessageSave::UserAdd(long lUserID, long lMessageID)
{
   return ItemWrite(OP_ADD, lUserID, lMessageID);
}

bool DBMessageSave::UserDelete(long lUserID, long lMessageID)
{
   return ItemWrite(OP_DELETE, lUserID, lMessageID);
}

bool DBMessageSave::IsSaved(long lMessageID)
{
   STACKTRACE
   DBItem<DBMessageSaveData *> *pItem = NULL;
   
   pItem = Find(lMessageID);
   if(pItem == NULL)
   {
      return false;
   }

   if(Op(pItem) == OP_DELETE)
   {
      // printf("DBMessageSave::Get -1, OP_DELETE\n");
      return false;
   }

   return true;
}

/* void DBMessageSave::ItemDelete(DBMessageSaveData *pData)
{
} */

bool DBMessageSave::ItemCompare(DBMessageSaveData *pData1, DBMessageSaveData *pData2)
{
   return true;
}

bool DBMessageSave::PreWrite(bool bLock)
{
	STACKTRACE
   if(bLock == true)
   {
      DBTable::Lock(FM_SAVE_TABLE);
   }

   return true;
}

bool DBMessageSave::ItemWrite(DBItem<DBMessageSaveData *> *pItem)
{
   return ItemWrite(pItem->m_iOp, GetID(), pItem->m_lID);
}

// ItemWrite: Prepare SQL command
bool DBMessageSave::ItemWrite(int iOp, long lUserID, long lMessageID)
{
	STACKTRACE
   // int iDebug = debuglevel(DEBUGLEVEL_DEBUG);
   bool bExecute = false, bUser = false;
   char szSQL[200];
   DBTable *pTable = NULL;

   debug(DEBUGLEVEL_DEBUG, "DBMessageSave::ItemWrite %d %ld %ld\n", iOp, lUserID, lMessageID);

   szSQL[0] = '\0';
   switch(Op(iOp))
   {
      case OP_ADD:
         sprintf(szSQL, "insert into %s values(%ld, %ld)", FM_SAVE_TABLE, lUserID, lMessageID);
         break;

      case OP_DELETE:
         sprintf(szSQL, "delete from %s", FM_SAVE_TABLE);
         if(lUserID > 0)
         {
            sprintf(szSQL, "%s where userid = %ld", szSQL, lUserID);

            bUser = true;
         }
         if(lMessageID > 0)
         {
            sprintf(szSQL, "%s %s messageid = %ld", szSQL, bUser == true ? "and" : "where", lMessageID);
         }
         break;

      default:
         debug("DBMessageSave::DBItemWrite invalid op %d\n", iOp);
         break;
   }

   if(szSQL[0] == '\0')
   {
      // debuglevel(iDebug);
      return false;
   }

   pTable = new DBTable();
   bExecute = pTable->Execute(szSQL);
   delete pTable;

   // debuglevel(iDebug);
   return bExecute;
}

bool DBMessageSave::PostWrite(bool bLock)
{
	STACKTRACE
   if(bLock == true)
   {
      DBTable::Unlock();
   }

   return true;
}
