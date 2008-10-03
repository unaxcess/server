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
** DBSub.cpp: Implementation of DBSub class
*/

#include <stdio.h>
#include <string.h>

#include "DBSub.h"

#include "ua.h"

// Constructor
DBSub::DBSub() : DBItemList<DBSubData *>(0, true, false)
{
   setup(NULL, NULL);
}

DBSub::DBSub(EDF *pEDF, char *szType) : DBItemList<DBSubData *>(0, true, false)
{
   setup(NULL, NULL, 0, 0, -1, pEDF, szType);
}

DBSub::DBSub(char *szTable, char *szID, long lUserID, int iLevel, int iInitExtra) : DBItemList<DBSubData *>(lUserID, true, true)
{
   setup(szTable, szID, lUserID, iLevel, iInitExtra);
}

DBSub::DBSub(EDF *pEDF, char *szType, char *szTable, char *szID, long lUserID, int iLevel, int iInitExtra) : DBItemList<DBSubData *>(lUserID, true, true)
{
   setup(szTable, szID, lUserID, iLevel, iInitExtra, pEDF, szType);
}

// Destructor
DBSub::~DBSub()
{
   STACKTRACE

   delete[] m_szTable;
   delete[] m_szID;
}

bool DBSub::Add(long lID, int iSubType, int iExtra, bool bTemp)
{
   STACKTRACE
   DBSubData *pData = new DBSubData;

   pData->m_iSubType = iSubType;
   pData->m_iExtra = iExtra;
   pData->m_bTemp = bTemp;

   return(DBItemList<DBSubData *>::Add(lID, pData) == ADD_YES);
}

bool DBSub::Update(long lID, int iSubType, int iExtra, bool bTemp)
{
   STACKTRACE
   DBSubData *pData = new DBSubData;

   pData->m_iSubType = iSubType;
   pData->m_iExtra = iExtra;
   pData->m_bTemp = bTemp;

   return(DBItemList<DBSubData *>::Add(lID, pData) != ADD_NO);
}

bool DBSub::UserAdd(char *szTable, char *szID, long lUserID, long lID, int iSubType, int iExtra)
{
   STACKTRACE
   return DBItemWrite(szTable, szID, 0, OP_ADD, lUserID, lID, iSubType, iExtra, false);
}

bool DBSub::UserDelete(char *szTable, char *szID, long lUserID, long lID)
{
   STACKTRACE
   debug(DEBUGLEVEL_DEBUG, "DBSub::UserDelete %s %s %ld %ld\n", szTable, szID, lUserID, lID);
   return DBItemWrite(szTable, szID, 0, OP_DELETE, lUserID, lID, -1, 0, false);
}

bool DBSub::UserUpdate(char *szTable, char *szID, long lUserID, long lID, int iSubType, int iExtra)
{
   STACKTRACE
   return DBItemWrite(szTable, szID, 0, OP_UPDATE, lUserID, lID, iSubType, iExtra, false);
}

DBSubData *DBSub::UserData(char *szTable, char *szID, long lUserID, long lID)
{
   STACKTRACE
   char szSQL[200];
   DBTable *pTable = NULL;
   DBSubData *pSubData = NULL;

   pTable = new DBTable();
   pTable->BindColumnBytes();
   pTable->BindColumnInt();

   sprintf(szSQL, "select subtype,extra from %s where userid = %ld and %s = %ld", szTable, lUserID, szID, lID);

   if(pTable->Execute(szSQL) == false)
   {
      delete pTable;

      return NULL;
   }

   if(pTable->NextRow() == false)
   {
      delete pTable;

      return NULL;
   }

   pSubData = new DBSubData;

   // pTable->GetField(0, (int *)&pSubData->m_iSubType);
   pSubData->m_iSubType = IntType(pTable, 0);
   pTable->GetField(1, &pSubData->m_iExtra);

   delete pTable;

   return pSubData;
}

long DBSub::ItemID(int iItemNum)
{
   STACKTRACE
   DBItem<DBSubData *> *pItem = Item(iItemNum);

   if(pItem == NULL)
   {
      return -1;
   }

   return pItem->m_lID;
}

int DBSub::ItemSubType(int iItemNum)
{
   STACKTRACE
   DBItem<DBSubData *> *pItem = Item(iItemNum);
   DBSubData *pData = NULL;

   if(pItem == NULL)
   {
      return -1;
   }

   pData = (DBSubData *)pItem->m_pData;
   return pData->m_iSubType;
}

int DBSub::ItemExtra(int iItemNum)
{
   STACKTRACE
   DBItem<DBSubData *> *pItem = Item(iItemNum);
   DBSubData *pData = NULL;

   if(pItem == NULL)
   {
      return -1;
   }

   pData = (DBSubData *)pItem->m_pData;
   return pData->m_iExtra;
}

DBSubData *DBSub::SubData(long lID)
{
   STACKTRACE
   DBItem<DBSubData *> *pItem = Find(lID);

   if(pItem == NULL)
   {
      return NULL;
   }

   if(Op(pItem) == OP_DELETE)
   {
      return NULL;
   }

   return pItem->m_pData;
}

// SubType: Subscription level for an item
int DBSub::SubType(long lID)
{
   DBSubData *pSubData = SubData(lID);

   // printf("DBSub::SubType %ld -> %p", lID, pSubData);
   if(pSubData == NULL)
   {
      // printf("\n");
      return 0;
   }

   // printf("(%d %d %d)\n", pSubData->m_iSubType, pSubData->m_iExtra, pSubData->m_bTemp);

   return pSubData->m_iSubType;
}

// CountType: Number of subscriptions of a particular type
int DBSub::CountType(int iSubType)
{
   STACKTRACE
   int iItemNum = 0, iReturn = 0;
   DBItem<DBSubData *> *pItem = NULL;
   DBSubData *pData = NULL;

   for(iItemNum = 0; iItemNum < Count(); iItemNum++)
   {
      pItem = Item(iItemNum);
      if(Op(pItem) != OP_DELETE)
      {
         pData = (DBSubData *)pItem->m_pData;
         if(pData->m_iSubType == iSubType)
         {
            iReturn++;
         }
      }
   }

   return iReturn;
}

// CountType: Subscription level for any user
int DBSub::SubType(char *szTable, char *szID, long lUserID, long lID)
{
   STACKTRACE
   int iSubType = 0;
   char szSQL[200];
   DBTable *pTable = NULL;

   pTable = new DBTable();
   pTable->BindColumnBytes();
   sprintf(szSQL, "select subtype from %s where userid = %ld and %s = %ld", szTable, lUserID, szID, lID);
   if(pTable->Execute(szSQL) == false)
   {
      delete pTable;

      debug("DBSub::SubType query failed\n");
      return -1;
   }

   if(pTable->NextRow() == false)
   {
      return 0;
   }

   // pTable->GetField(0, &iSubType);
   iSubType = IntType(pTable, 0);

   delete pTable;

   return iSubType;
}

// CountType: Number of subscriptions of a particular type for any user
int DBSub::CountType(char *szTable, long lUserID, int iSubType)
{
   STACKTRACE
   int iReturn = 0;//, iValue = 0;
   char szSQL[200];
   DBTable *pTable = NULL;

   pTable = new DBTable();
   pTable->BindColumnInt();
   sprintf(szSQL, "select count(subtype) from %s where userid = %ld", szTable, lUserID);
   if(pTable->Execute(szSQL) == false)
   {
      delete pTable;

      debug("DBSub::SubType query failed\n");
      return -1;
   }

   if(pTable->NextRow() == true)
   {
      pTable->GetField(0, &iReturn);
      /* if(iValue == iSubType)
      {
         iReturn++;
      } */
   }

   delete pTable;

   return iReturn;
}

// IntType: Change field character to sub type
int DBSub::IntType(DBTable *pTable, int iFieldNum)
{
   STACKTRACE
   char *szValue = NULL;

   if(pTable->GetField(iFieldNum, &szValue) == false)
   {
      return -1;
   }

   if(szValue == NULL || strlen(szValue) == 0)
   {
      return -1;
   }

   if(szValue[0] == 'E')
   {
      return SUBTYPE_EDITOR;
   }
   else if(szValue[0] == 'M')
   {
      return SUBTYPE_MEMBER;
   }
   else if(szValue[0] == 'S')
   {
      return SUBTYPE_SUB;
   }

   return -1;
}

// CharType: Change sub type to field character
char DBSub::CharType(int iSubType)
{
   STACKTRACE
   if(iSubType == SUBTYPE_EDITOR)
   {
      return 'E';
   }
   else if(iSubType == SUBTYPE_MEMBER)
   {
      return 'M';
   }
   else if(iSubType == SUBTYPE_SUB)
   {
      return 'S';
   }

   return '\0';
}

bool DBSub::Write(bool bLock)
{
   return DBItemList<DBSubData *>::Write(bLock);
}

bool DBSub::Write(EDF *pEDF, char *szType)
{
   STACKTRACE
   int iItemNum = 0;
   DBItem<DBSubData *> *pItem = NULL;
   DBSubData *pData = NULL;

   debug("DBSub::Write %p %s, %d\n", pEDF, szType, Count());

   for(iItemNum = 0; iItemNum < Count(); iItemNum++)
   {
      pItem = Item(iItemNum);
      debug("DBSub::Write item %d -> %p\n", iItemNum, pItem);
      pData = (DBSubData *)pItem->m_pData;
      debug("DBSub::Write data %p\n", pData);

      pEDF->Add(szType, pItem->m_lID);
      pEDF->AddChild("subtype", pData->m_iSubType);
      pEDF->AddChild("extra", pData->m_iExtra);
      pEDF->AddChild("temp", pData->m_bTemp);
      pEDF->Parent();
   }

   return true;
}

bool DBSub::ItemCompare(DBSubData *pData1, DBSubData *pData2)
{
   if(pData1->m_iSubType == pData2->m_iSubType && pData1->m_iExtra == pData2->m_iExtra)
   {
      return true;
   }

   return false;
}

bool DBSub::PreWrite(bool bLock)
{
   STACKTRACE

   return true;
}

bool DBSub::ItemWrite(DBItem<DBSubData *> *pItem)
{
   STACKTRACE
   DBSubData *pData = (DBSubData *)pItem->m_pData;

   return DBItemWrite(m_szTable, m_szID, m_iLevel, pItem->m_iOp, GetID(), pItem->m_lID, pData->m_iSubType, pData->m_iExtra, pData->m_bTemp);
}

bool DBSub::PostWrite(bool bLock)
{
   STACKTRACE
   if(bLock == true)
   {
      DBTable::Unlock();
   }

   return true;
}

void DBSub::setup(char *szTable, char *szID, long lUserID, int iLevel, int iInitExtra, EDF *pEDF, char *szType)
{
   STACKTRACE
   int iID = 0, iSubType = 0, iExtra = 0;
   bool bLoop = false, bTemp = false;
   char szSQL[200];
   DBTable *pTable = NULL;
   DBSubData *pData = NULL;

   debug("DBSub::setup entry %s %s %ld %d %d %p %s\n", szTable, szID, lUserID, iLevel, iInitExtra, pEDF, szType);

   m_szTable = strmk(szTable);
   m_szID = strmk(szID);
   m_iLevel = iLevel;

   if(pEDF != NULL && szType != NULL)
   {
      bLoop = pEDF->Child(szType);
      while(bLoop == true)
      {
         iSubType = 0;
         iExtra = iInitExtra;

         pEDF->Get(NULL, &iID);
         pEDF->GetChild("subtype", &iSubType);
         pEDF->GetChild("extra", &iExtra);
         bTemp = pEDF->GetChildBool("temp");

         pData = new DBSubData;
         pData->m_iSubType = iSubType;
         pData->m_iExtra = iExtra;
         pData->m_bTemp = bTemp;
         DBItemList<DBSubData *>::Add(iID, pData, OP_NONE);

         bLoop = pEDF->Next(szType);
         if(bLoop == false)
         {
            pEDF->Parent();
         }
      }
   }
   else if(m_szID != NULL && m_szTable != NULL)
   {
      pTable = new DBTable();
      pTable->BindColumnInt();
      pTable->BindColumnBytes();
      pTable->BindColumnInt();

      sprintf(szSQL, "select %s, subtype, extra from %s where userid = %ld", m_szID, m_szTable, lUserID);
      if(pTable->Execute(szSQL) == false)
      {
         debug("DBSub::setup query failed\n");
      }
      else
      {
         SetWrite(false);

         while(pTable->NextRow() == true)
         {
            if(pTable->GetField(0, &iID) == true)
            {
               iSubType = IntType(pTable, 1);
               pTable->GetField(2, &iExtra);
               if(iSubType >= m_iLevel)
               {
                  pData = new DBSubData;
                  pData->m_iSubType = iSubType;
                  if(iInitExtra != -1)
                  {
                     pData->m_iExtra = iInitExtra;
                  }
                  else
                  {
                     pData->m_iExtra = iExtra;
                  }
                  DBItemList<DBSubData *>::Add(iID, pData, OP_NONE);
               }
            }
         }

         SetWrite(true);
      }

      delete pTable;
   }

   SetChanged(false);

   // Debug(bDebug);

   debug("DBSub::setup exit, %d items\n", Count());
}

// DBItemWrite: Prepare SQL command
bool DBSub::DBItemWrite(char *szTable, char *szID, int iLevel, int iOp, long lUserID, long lID, int iSubType, int iExtra, bool bTemp)
{
   bool bExecute = false;
   char szSQL[200], cSubType = '\0';
   DBTable *pTable = NULL;

   debug(DEBUGLEVEL_DEBUG, "DBSub::DBItemWrite entry t=%s i=%s l=%d o=%d u=%ld i=%ld s=%d e=%d t=%s\n", szTable, szID, iLevel, iOp, lUserID, lID, iSubType, iExtra, BoolStr(bTemp));

   // if(iSubType < iLevel || bTemp == true)
   if(bTemp == true)
   {
      debug(DEBUGLEVEL_DEBUG, "DBSub::DBItemWrite exit true, no action\n");
      return true;
   }

   cSubType = CharType(iSubType);

   szSQL[0] = '\0';
   switch(Op(iOp))
   {
      case OP_ADD:
         sprintf(szSQL, "insert into %s values(%ld, %ld, '%c', %d)", szTable, lUserID, lID, cSubType, iExtra);
         break;

      case OP_DELETE:
         sprintf(szSQL, "delete from %s where userid = %ld", szTable, lUserID);
         if(lID > 0)
         {
            sprintf(szSQL, "%s and %s = %ld", szSQL, szID, lID);
         }
         break;

      case OP_UPDATE:
         sprintf(szSQL, "update %s set subtype = '%c', extra = %d where userid = %ld and %s = %ld", szTable, cSubType, iExtra, lUserID, szID, lID);
         break;

      default:
         debug("DBSub::DBItemWrite invalid op %d\n", iOp);
         break;
   }

   if(szSQL[0] == '\0')
   {
      debug(DEBUGLEVEL_DEBUG, "DBSub::DBItemWrite exit false, no SQL\n");
      return false;
   }

   // DBTable::Debug(true);
   pTable = new DBTable();
   bExecute = pTable->Execute(szSQL);
   /* if(bExecute == false)
   {
      debug("DBSub::ItemWrite db failed '%s'\n", szSQL);
   } */
   delete pTable;
   // DBTable::Debug(false);

   debug(DEBUGLEVEL_DEBUG, "DBSub::DBItemWrite exit %s\n", BoolStr(bExecute));
   return bExecute;
}
