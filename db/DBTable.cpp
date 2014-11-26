/*
** DBTable library
** (c) 1999 Michael Wood (mike@compsoc.man.ac.uk)
**
** db.cpp: Implementation of DBTable class
*/

#include <stdio.h>
#include <string.h>

#include "useful/useful.h"

#include "DBTable.h"
#include "db-common.h"

// bool DBTable::m_bDebug = false;

DBTable::DBTable()
{
   m_pResults = NULL;
   m_pFields = NULL;
   m_iNumFields = 0;
}

DBTable::~DBTable()
{
   int iFieldNum = 0;

	// printf("DBTable::~DBTable entry\n");

   DBResultsFree(m_pResults);

   for(iFieldNum = 0; iFieldNum < m_iNumFields; iFieldNum++)
   {
      // printf("DBTable::~DBTable %d %d\n", iFieldNum, m_pFields[iFieldNum]->m_iType);
      /* if(m_pFields[iFieldNum]->m_iType == DBTable::BYTE)
      {
         debug("DBTable::~DBTable byte copy %s\n", BoolStr(m_pFields[iFieldNum]->m_bCopy));
      } */
      if(m_pFields[iFieldNum]->m_iType == BYTES && m_pFields[iFieldNum]->m_bCopy == true)
      {
         // printf("DBTable::~DBTable deleting %p\n", m_pFields[iFieldNum]->m_pValue);
         delete m_pFields[iFieldNum]->m_pValue;
      }
      delete m_pFields[iFieldNum];
   }

   delete[] m_pFields;

	// printf("DBTable::~DBTable exit\n");
}

bool DBTable::Connect(char *szDatabase, char *szUsername, char *szPassword, char *szHostname)
{
   return DBConnect(szDatabase, szUsername, szPassword, szHostname);
}

bool DBTable::Disconnect()
{
   return DBDisconnect();
}

bool DBTable::BindColumnBytes(long lValueLen)
{
   return add(NULL, m_iNumFields + 1, BYTES, NULL, lValueLen, 0, 0, false, false);
}

bool DBTable::BindColumnBytes(int iColumnNum, long lValueLen)
{
   return add(NULL, iColumnNum, BYTES, NULL, lValueLen, 0, 0, false, false);
}

bool DBTable::BindColumnInt()
{
   return add(NULL, m_iNumFields + 1, INT, NULL, 0, 0, 0, false, false);
}

bool DBTable::BindColumnInt(int iColumnNum)
{
   return add(NULL, iColumnNum, INT, NULL, 0, 0, 0, false, false);
}

bool DBTable::SetField(const char *szValue, bool bCopy)
{
   return add(NULL, m_iNumFields + 1, STR, (void *)szValue, 0, 0, 0, true, bCopy);
}

bool DBTable::SetField(bytes *pValue, bool bCopy)
{
   return add(NULL, m_iNumFields + 1, BYTES, (void *)pValue, 0, 0, 0, true, bCopy);
}

/* bool DBTable::SetField(int iColumnNum, char *szValue, bool bCopy)
{
   return add(iColumnNum, STR, (void *)szValue, 0, 0, 0, true, bCopy);
} */

/* bool DBTable::SetField(int iColumnNum, bytes *pValue, bool bCopy)
{
   return add(iColumnNum, BYTES, pValue, 0, 0, 0, true, bCopy);
} */

bool DBTable::SetField(long lValue)
{
   return add(NULL, m_iNumFields + 1, INT, NULL, 0, lValue, 0, true, false);
}

/* bool DBTable::SetField(int iColumnNum, long lValue)
{
   return add(iColumnNum, INT, NULL, 0, lValue, 0, true, false);
} */

int DBTable::CountFields()
{
   return m_iNumFields;
}

bool DBTable::Execute(char *szSQL, long *lPrepareTime, long *lExecuteTime)
{
   bool bReturn = false;
   // bytes *pSQL = NULL;

   // pSQL = new bytes(szSQL);

   bReturn = Execute((byte *)szSQL, strlen(szSQL), lPrepareTime, lExecuteTime);

   // delete pSQL;

   return bReturn;
}

bool DBTable::Execute(byte *pSQL, long lSQLLen, long *lPrepareTime, long *lExecuteTime)
{
   m_pResults = DBResultsFree(m_pResults);

   debug(DEBUGLEVEL_DEBUG, "DBTable::Execute %s\n", (char *)pSQL);

   return DBExecute(pSQL, lSQLLen, &m_pResults, lPrepareTime, lExecuteTime);
}

bool DBTable::NextRow()
{
   return DBNextRow(m_pResults, m_pFields, m_iNumFields);
}

int DBTable::GetFieldType(int iFieldNum)
{
   if(iFieldNum < 0 || iFieldNum >= m_iNumFields)
   {
      return -1;
   }

   return m_pFields[iFieldNum]->m_iType;
}

bool DBTable::GetField(int iFieldNum, char **szValue)
{
   int iReturn = 0;
   bytes *pValue = NULL;

   iReturn = get(iFieldNum, BYTES, &pValue, NULL, NULL);

   if(pValue != NULL && szValue != NULL)
   {
      *szValue = (char *)pValue->Data(false);
   }

   return iReturn == BYTES;
}

bool DBTable::GetField(int iFieldNum, bytes **pValue)
{
   return get(iFieldNum, BYTES, pValue, NULL, NULL) == BYTES;
}

bool DBTable::GetField(int iFieldNum, int *iValue)
{
   int iType = 0;
   long lGet = 0;
   double dGet = 0;

   iType = get(iFieldNum, -1, NULL, &lGet, &dGet);
   if(iType == INT || iType == FLOAT)
   {
      if(iType == INT && iValue != NULL)
      {
         *iValue = (int)lGet;
      }
      else if(iType == FLOAT && iValue != NULL)
      {
         *iValue = (int)dGet;
      }

      return true;
   }

   return false;
}

bool DBTable::GetField(int iFieldNum, long *lValue)
{
   int iType = 0;
   long lGet = 0;
   double dGet = 0;

   iType = get(iFieldNum, -1, NULL, &lGet, &dGet);
   if(iType == INT || iType == FLOAT)
   {
      if(iType == INT && lValue != NULL)
      {
         *lValue = lGet;
      }
      else if(iType == FLOAT && lValue != NULL)
      {
         *lValue = (long)dGet;
      }

      return true;
   }

   return false;
}

bool DBTable::GetField(int iFieldNum, unsigned long *lValue)
{
   int iType = 0;
   long lGet = 0;
   double dGet = 0;

   iType = get(iFieldNum, -1, NULL, &lGet, &dGet);
   if(iType == INT || iType == FLOAT)
   {
      if(iType == INT && lValue != NULL)
      {
         *lValue = (unsigned long)lGet;
      }
      else if(iType == FLOAT && lValue != NULL)
      {
         *lValue = (unsigned long)dGet;
      }

      return true;
   }

   return false;
}

bool DBTable::GetField(int iFieldNum, bool *bValue)
{
   // long lValueLen = 0;
   long lGet = 0;
   double dGet = 0;
   int iType = 0;
   bytes *pValue = NULL;

   iType = get(iFieldNum, -1, &pValue, &lGet, &dGet);
   if(iType == INT || iType == FLOAT)
   {
      if(bValue != NULL)
      {
         if(dGet == 1 || lGet == 1)
         {
            *bValue = true;
         }
         else
         {
            *bValue = false;
         }
      }

      return true;
   }

   return false;
}

bool DBTable::Insert(char *szTable)
{
   STACKTRACE
   int iFieldNum = 0;
   bool bReturn = false;
   long lSQLLen = 0, lSQLPos = 0;
   byte *pSQL = NULL;
   char szNumber[20];
   double dTick = gettick();

   /* if(m_bDebug == true)
   {
      debug("DBTable::Insert entry %s, %p\n", szTable, pFields);
   } */

   if(szTable == NULL || m_iNumFields == 0)
   {
      debug("DBTable::Insert NULL table / no fields\n");
      return false;
   }

   // printf("DBTable::Insert %s, %d\n", szTable, m_iNumFields);

   lSQLLen = 40 + strlen(szTable);
   for(iFieldNum = 0; iFieldNum < m_iNumFields; iFieldNum++)
   {
      // printf("DBTable::Insert check length field %d, %p %d", iFieldNum, m_pFields[iFieldNum], m_pFields[iFieldNum]->m_iType);
      lSQLLen += 3;
      if(m_pFields[iFieldNum]->m_iType == BYTES && m_pFields[iFieldNum]->m_pValue != NULL)
      {
         // printf(", %p\n", m_pFields[iFieldNum]->m_pValue);
         lSQLLen += 2 * m_pFields[iFieldNum]->m_pValue->Length();
      }
      else if(m_pFields[iFieldNum]->m_iType == INT || m_pFields[iFieldNum]->m_iType == FLOAT)
      {
         // printf("\n");
         lSQLLen += sizeof(szNumber);
      }
   }

   STACKTRACEUPDATE

   // printf("DBTable::Insert SQL len %ld\n", lSQLLen);
   pSQL = new byte[lSQLLen];
   // printf("DBTable::Insert SQL string %p\n", pSQL);

   sprintf((char *)pSQL, "insert into %s values(", szTable);
   lSQLPos = strlen((char *)pSQL);
   STACKTRACEUPDATE
   for(iFieldNum = 0; iFieldNum < m_iNumFields; iFieldNum++)
   {
      // printf("DBTable::Insert field %d\n", iFieldNum);
      if(iFieldNum > 0)
      {
         pSQL[lSQLPos++] = ',';
      }
      if(m_pFields[iFieldNum]->m_iType == BYTES)
      {
         // printf("data %s", m_pFields[iFieldNum]->m_pValue);
         if(m_pFields[iFieldNum]->m_pValue != NULL)
         {
            // bytesprint("DBTable::Insert byte value", m_pFields[iFieldNum]->m_pValue);
         }

         pSQL[lSQLPos++] = '\'';
         lSQLPos = Literalise(pSQL, lSQLPos, m_pFields[iFieldNum]->m_pValue);
         pSQL[lSQLPos++] = '\'';
      }
      else if(m_pFields[iFieldNum]->m_iType == INT)
      {
         // printf("number %ld", m_pFields[iFieldNum]->m_lValue);
         sprintf(szNumber, "%ld", m_pFields[iFieldNum]->m_lValue);
         memcpy(pSQL + lSQLPos, szNumber, strlen(szNumber));
         lSQLPos += strlen(szNumber);
      }
      else if(m_pFields[iFieldNum]->m_iType == FLOAT)
      {
         // printf("number %ld", m_pFields[iFieldNum]->m_lValue);
         sprintf(szNumber, "%g", m_pFields[iFieldNum]->m_dValue);
         memcpy(pSQL + lSQLPos, szNumber, strlen(szNumber));
         lSQLPos += strlen(szNumber);
      }
      // printf("\n");
   }
   pSQL[lSQLPos++] = ')';
   pSQL[lSQLPos] = '\0';

   // printf("DBTable::Insert used %ld\n", lSQLPos);

   // if(m_bDebug == true)
   {
      // printf("DBTable::Insert %ld of %ld bytes used: '%s'\n", lSQLPos, lSQLLen, pSQL);
   }

   /* g_hStmt = DBDirectPrepare(pSQL, lSQLPos);
   if(g_hStmt != NULL)
   {
      bReturn = DBDirectExecute(g_hStmt);
   } */
   // memprint("DBTable::Insert", pSQL, lSQLPos);
   bReturn = Execute(pSQL, lSQLPos);
   // bReturn = true;

   delete[] pSQL;

   // if(m_bDebug == true)
   {
      debug(DEBUGLEVEL_DEBUG, "DBTable::Insert execute %s, %ld ms\n", BoolStr(bReturn), tickdiff(dTick));
   }
   return bReturn;
}

bool DBTable::Lock(char *szTable)
{
   return DBLock(szTable);
}

bool DBTable::Unlock()
{
   return DBUnlock();
}

long DBTable::Literalise(byte *pDest, long lDestPos, bytes *pSource)
{
   // printf("DBTable::QuoteData %p %d %p %d\n", pDest, lDestPos, pSource, lSourceLen);
   return DBLiteralise(pDest, lDestPos, pSource);
}

int DBTable::Error()
{
   return DBError();
}

bool DBTable::add(char *szColumnName, int iColumnNum, int iType, void *pValue, long lValueLen, long lValue, double dValue, bool bSet, bool bCopy)
{
   STACKTRACE
   DBField **pFields = NULL, *pField = NULL;

   // printf("DBTable::add n=%d t=%d v=%p/%ld/%ld/%g s=%s c=%s\n", iColumnNum, iType, pValue, lValueLen, lValue, dValue, BoolStr(bSet), BoolStr(bCopy));
   pField = new DBField;
   
   pField->m_szColumnName = strmk(szColumnName);
   pField->m_iColumnNum = iColumnNum;
   pField->m_iType = iType;
   if(iType == STR)
   {
      // printf("DBTable::add string field\n");
      pField->m_iType = BYTES;

      if(bSet == true)
      {
         // if(bCopy == true)
         {
            // pField->m_pValue = memmk(pValue, lValueLen);
            pField->m_pValue = new bytes((char *)pValue);
            // printf("DBTable::add copy %p\n", pField->m_pValue);
         }
         /* else
         {
            pField->m_pValue = pValue;
         } */
         bCopy = true;
      }
      else
      {
         // pField->m_lValueLen = lValueLen;
         pField->m_pValue = NULL;
      }
      pField->m_bCopy = bCopy;

      // bytesprint("DBTable::add field", pField->m_pValue);

      STACKTRACEUPDATE
   }
   else if(iType == BYTES)
   {
      // bytesprint("DBTable::add bytes field", (bytes *)pValue);
      if(bSet == true)
      {
         if(bCopy == true)
         {
            // pField->m_pValue = memmk(pValue, lValueLen);
            pField->m_pValue = new bytes((bytes *)pValue);
            // printf("DBTable::add copy %p\n", pField->m_pValue);
         }
         else
         {
            pField->m_pValue = (bytes *)pValue;
            // printf("DBTable::add no copy %p\n", pField->m_pValue);
         }
      }
      else
      {
         // pField->m_lValueLen = lValueLen;
         pField->m_pValue = NULL;
      }
      pField->m_bCopy = bCopy;

      STACKTRACEUPDATE
   }
   else if(iType == INT)
   {
      // printf("DBTable::add int field\n");
      if(bSet == true)
      {
         pField->m_lValue = lValue;
      }
      else
      {
         pField->m_lValue = 0;
      }
   }
   else if(iType == FLOAT)
   {
      // printf("DBTable::add int field\n");
      if(bSet == true)
      {
         pField->m_dValue = dValue;
      }
      else
      {
         pField->m_dValue = 0;
      }
   }

   // printf("DBTable::add array insert %p (last of %d into %p)\n", pField, m_iNumFields, m_pFields);
   ARRAY_INSERT(DBField *, m_pFields, m_iNumFields, pField, m_iNumFields, pFields)
   // printf("DBTable::add new array %p\n", m_pFields);
   
   // printf("DBTable::add exit true\n");
   return true;
}

int DBTable::get(int iFieldNum, int iType, bytes **pValue, long *lValue, double *dValue)
{
   if(iFieldNum < 0 || iFieldNum >= m_iNumFields)
   {
      return -1;
   }

   if(iType != -1 && iType != m_pFields[iFieldNum]->m_iType)
   {
      return -1;
   }

   if(pValue != NULL && m_pFields[iFieldNum]->m_iType == BYTES)
   {
      *pValue = m_pFields[iFieldNum]->m_pValue;
   }
   /* if(lValueLen != NULL && m_pFields[iFieldNum]->m_iType == BYTE)
   {
      *lValueLen = m_pFields[iFieldNum]->m_lValueLen;
   } */
   if(dValue != NULL && m_pFields[iFieldNum]->m_iType == INT)
   {
      *lValue = m_pFields[iFieldNum]->m_lValue;
   }
   if(dValue != NULL && m_pFields[iFieldNum]->m_iType == FLOAT)
   {
      *dValue = m_pFields[iFieldNum]->m_dValue;
   }

   return m_pFields[iFieldNum]->m_iType;
}

/* bool DBTable::Debug(bool bDebug)
{
   bool bReturn = m_bDebug;

   m_bDebug = bDebug;

   return bReturn;
}

bool DBTable::Debug()
{
   // printf("DBTable::Debug %s\n", BoolStr(m_bDebug));
   return m_bDebug;
} */
