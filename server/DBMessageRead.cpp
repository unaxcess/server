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
** DBMessageRead.cpp: Implementation of DBMessageRead class
*/

#include <stdio.h>

#include "db/DBTable.h"
#include "DBMessageRead.h"

// Constructor
DBMessageRead::DBMessageRead(long lUserID) : DBItemList<DBMessageReadData *>(lUserID, true, false)
{
	STACKTRACE
   int iMessageID = 0, iMarkType = 0;
   char *szMarkType = NULL;
   char szSQL[200];
   DBTable *pTable = NULL;
   DBMessageReadData *pData = NULL;

   pTable = new DBTable();
   pTable->BindColumnInt();
   pTable->BindColumnBytes();

   sprintf(szSQL, "select messageid,marktype from %s where userid = %ld", FM_READ_TABLE, lUserID);
   if(pTable->Execute(szSQL) == false)
   {
      debug("DBMessageRead::DBMessageRead query failed\n");
   }
   else
   {
      while(pTable->NextRow() == true)
      {
         if(pTable->GetField(0, &iMessageID) == true)
         {
            iMarkType = 0;
            // pTable->GetField(1, &iMarkType);
            pTable->GetField(1, &szMarkType);
            if(szMarkType != NULL)
            {
               iMarkType = szMarkType[0] - '0';
               if(iMarkType > 0)
               {
                  // printf("DBMessageRead::DBMessageRead marktype %s -> %d\n", szMarkType, iMarkType);
               }
            }

            pData = new DBMessageReadData;
            pData->m_iMarkType = iMarkType;
            DBItemList<DBMessageReadData *>::Add(iMessageID, pData, OP_NONE);
         }
      }
   }

   delete pTable;

   SetChanged(false);
}

// Destructor
DBMessageRead::~DBMessageRead()
{
	STACKTRACE
}

bool DBMessageRead::Add(long lMessageID, int iMarkType)
{
   int iAdd = 0;
   // int iDebug = debuglevel(DEBUGLEVEL_DEBUG);
   bool bReturn = false;
   DBMessageReadData *pData = NULL;

   if(iMarkType > 0)
   {
      debug(DEBUGLEVEL_DEBUG, "DBMessageRead::Add entry %ld %d\n", lMessageID, iMarkType);
   }

   pData = new DBMessageReadData;
   pData->m_iMarkType = iMarkType;

   iAdd = DBItemList<DBMessageReadData *>::Add(lMessageID, pData);

   if(iAdd == ADD_YES || iAdd == ADD_UNDELETE)
   {
      bReturn = true;
   }

   // if(Debug() == true)
   {
      debug(DEBUGLEVEL_DEBUG, "DBMessageRead::Add exit %s\n", BoolStr(bReturn));
   }
   // debuglevel(iDebug);
   return bReturn;
}

bool DBMessageRead::UserAdd(long lUserID, long lMessageID, int iMarkType)
{
   return ItemWrite(OP_ADD, lUserID, lMessageID, iMarkType);
}

bool DBMessageRead::UserDelete(long lUserID, long lMessageID)
{
   return ItemWrite(OP_DELETE, lUserID, lMessageID, 0);
}

DBMessageRead *DBMessageRead::UserCatchups(long lMessageID)
{
   STACKTRACE
   int iMarkType = -1;
   long lUserID = -1;
   char *szMarkType = NULL;
   char szSQL[200];
   DBTable *pTable = NULL;
   DBMessageRead *pCatchups = NULL;

   pTable = new DBTable();
   pTable->BindColumnInt();
   pTable->BindColumnBytes();

   sprintf(szSQL, "select userid,marktype from %s where messageid = %ld and marktype > 0", FM_READ_TABLE, lMessageID);

   if(pTable->Execute(szSQL) == false)
   {
      delete pTable;

      return pCatchups;
   }

   pCatchups = new DBMessageRead(lMessageID);

   while(pTable->NextRow() == true)
   {
	   pTable->GetField(0, &lUserID);
	   pTable->GetField(1, &szMarkType);
	   if(szMarkType != NULL)
	   {
		  iMarkType = szMarkType[0] - '0';
		  if(iMarkType > 0)
		  {
			 debug("DBMessageRead::UserCatchups marktype %s -> %d\n", szMarkType, iMarkType);
			 pCatchups->Add(lUserID, iMarkType);
		  }
	   }
   }

   delete pTable;

   return pCatchups;
}

int DBMessageRead::Get(long lMessageID)
{
	STACKTRACE
   DBItem<DBMessageReadData *> *pItem = NULL;
   
   pItem = Find(lMessageID);
   if(pItem == NULL)
   {
      return -1;

   }

   if(Op(pItem) == OP_DELETE)
   {
      // printf("DBMessageRead::Get -1, OP_DELETE\n");
      return -1;
   }

   return pItem->m_pData->m_iMarkType;
}

/* void DBMessageRead::ItemDelete(DBMessageReadData *pData)
{
} */

bool DBMessageRead::ItemCompare(DBMessageReadData *pData1, DBMessageReadData *pData2)
{
   if(pData1->m_iMarkType == pData2->m_iMarkType)
   {
      return true;
   }

   return false;
}

bool DBMessageRead::PreWrite(bool bLock)
{
	STACKTRACE
   if(bLock == true)
   {
      DBTable::Lock(FM_READ_TABLE);
   }

   return true;
}

bool DBMessageRead::ItemWrite(DBItem<DBMessageReadData *> *pItem)
{
   return ItemWrite(pItem->m_iOp, GetID(), pItem->m_lID, pItem->m_pData->m_iMarkType);
}

// ItemWrite: Prepare SQL command
bool DBMessageRead::ItemWrite(int iOp, long lUserID, long lMessageID, int iMarkType)
{
	STACKTRACE
   // int iDebug = debuglevel(DEBUGLEVEL_DEBUG);
   bool bExecute = false, bUser = false;
   char szSQL[200];
   DBTable *pTable = NULL;

   debug(DEBUGLEVEL_DEBUG, "DBMessageRead::ItemWrite %d %ld %ld %d\n", iOp, lUserID, lMessageID, iMarkType);

   szSQL[0] = '\0';
   switch(Op(iOp))
   {
      case OP_ADD:
         sprintf(szSQL, "insert into %s values(%ld, %ld, '%d')", FM_READ_TABLE, lUserID, lMessageID, iMarkType);
         break;

      case OP_DELETE:
         sprintf(szSQL, "delete from %s", FM_READ_TABLE);
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

      case OP_UPDATE:
         sprintf(szSQL, "update %s set marktype = '%d' where userid = %ld and messageid = %ld", FM_READ_TABLE, iMarkType, lUserID, lMessageID);
         break;

      default:
         debug("DBMessageRead::DBItemWrite invalid op %d\n", iOp);
         break;
   }

   if(szSQL[0] == '\0')
   {
      // debuglevel(iDebug);
      return false;
   }

   pTable = new DBTable();
   bExecute = pTable->Execute(szSQL);
   /* if(bExecute == false)
   {
#ifdef UNIX
      if(pTable->Error() == ER_DUP_ENTRY)
      {
         debug("DBMessageRead::ItemWrite ignoring dup error\n");
         bExecute = true;
      }
      else
#endif
      {
         debug("DBMessageRead::ItemWrite db failed '%s'\n", szSQL);
      }
   } */
   delete pTable;

   // debuglevel(iDebug);
   return bExecute;
}

bool DBMessageRead::PostWrite(bool bLock)
{
	STACKTRACE
   if(bLock == true)
   {
      DBTable::Unlock();
   }

   return true;
}
