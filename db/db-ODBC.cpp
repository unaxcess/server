/*
** DBTable library
** (c) 1999 Michael Wood (mike@compsoc.man.ac.uk)
**
** db-ODBC.cpp: Implementation of ODBC specific functions for use with DBTable class
*/

#include <stdio.h>

#include "useful/useful.h"

#include "DBTable.h"
#include "db-common.h"

SQLHENV g_hEnv = NULL;
SQLHDBC g_hDBC = NULL;

// bool DBTable::m_bDebug = false;

#define INIT_VALUE 1000

// DBCheck: Return codes from SQL function
int DBCheck(char *szTitle, SQLRETURN iRetCode, SQLSMALLINT iType, SQLHANDLE pHandle, byte *pSQL = NULL, int iSQLLen = -1)
{
   SQLCHAR szState[6], szMessage[SQL_MAX_MESSAGE_LENGTH];
   SQLINTEGER iError = 0;
   SQLSMALLINT iDiagNum = 1, iLength = 0;
   SQLRETURN iReturn = SQL_SUCCESS;

   // printf("DBCheck %s, r=%d t=%d h=%p\n", szTitle, iRetCode, iType, pHandle);

   if(iRetCode == SQL_SUCCESS || iRetCode == SQL_NO_DATA)
   {
      return SQL_SUCCESS;
   }

   debug("%s (%s)\n", szTitle, iRetCode == SQL_SUCCESS_WITH_INFO ? "information" : "error");
   if(pSQL != NULL)
   {
      memprint(pSQL, iSQLLen);
   }
   while(iReturn != SQL_NO_DATA)
   {
      iError = 0;
      iLength = 0;
      szState[0] = '\0';
      szMessage[0] = '\0';

      iReturn = SQLGetDiagRec(iType, pHandle, iDiagNum, szState, &iError, szMessage, sizeof(szMessage), &iLength);
      debug("%s(%d): %s\n", szState, iError, szMessage);
      iDiagNum++;
   }

   if(iRetCode == SQL_SUCCESS_WITH_INFO)
   {
      return SQL_SUCCESS;
   }

   return iRetCode;
}

// DBConnect: Establish connection to a database
bool DBConnect(char *szDatabase, char *szUsername, char *szPassword, char *szHostname)
{
   SQLRETURN retcode = SQL_SUCCESS;
   SQLSMALLINT iOutLen;
   SQLCHAR szInConn[1024], szOutConn[1024];
   bool bReturn = false;

   retcode = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &g_hEnv);
   if(DBCheck("DBConnect environment allocation", retcode, SQL_HANDLE_ENV, g_hEnv) == SQL_SUCCESS)
   {
      retcode = SQLSetEnvAttr(g_hEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0); 
      if(DBCheck("DBConnect environment setting", retcode, SQL_HANDLE_ENV, g_hEnv) == SQL_SUCCESS)
      {
         retcode = SQLAllocHandle(SQL_HANDLE_DBC, g_hEnv, &g_hDBC); 
         if(DBCheck("DBConnect connection allocation", retcode, SQL_HANDLE_ENV, g_hEnv) == SQL_SUCCESS)
         {
            sprintf((char *)szInConn, "DSN=%s", szDatabase);
            if(szUsername != NULL)
            {
               sprintf((char *)szInConn, "%s;UID=%s", szInConn, szUsername);
            }
            if(szPassword != NULL)
            {
               sprintf((char *)szInConn, "%s;PWD=%s", szInConn, szPassword);
            }
            // if(DBTable::Debug() == true)
            {
               debug(DEBUGLEVEL_DEBUG, "DBConnect connection string %s\n", szInConn);
            }
            retcode = SQLDriverConnect(g_hDBC, NULL, szInConn, strlen((char *)szInConn), szOutConn, sizeof(szOutConn), &iOutLen, SQL_DRIVER_NOPROMPT);
            if(DBCheck("DBConnect driver connect", retcode, SQL_HANDLE_DBC, g_hDBC) == SQL_SUCCESS)
            {
               bReturn = true;
            }
         }
      }
   }

   return bReturn;
}

// DBDisconnect: Close connection to database
bool DBDisconnect()
{
   if(g_hDBC != NULL)
   {
      SQLDisconnect(g_hDBC);
   }

   if(g_hDBC != NULL)
   {
      SQLFreeHandle(SQL_HANDLE_DBC, g_hDBC);
      g_hDBC = NULL;
   }

   if(g_hEnv != NULL)
   {
      SQLFreeHandle(SQL_HANDLE_ENV, g_hEnv);
      g_hEnv = NULL;
   }

   return true;
}

bool DBLock(char *szTables)
{
   return true;
}

bool DBUnlock()
{
   return true;
}

// DBExecute: Run an SQL command
bool DBExecute(byte *pSQL, long lLength, DBRESULTS *pReturn, long *lPrepareTime, long *lExecuteTime)
{
   double dTick = 0;
   SQLRETURN retcode = SQL_SUCCESS;
   // HSTMT pStmt = NULL;
   DBRESULTS pResults = NULL;

   retcode = SQLAllocHandle(SQL_HANDLE_STMT, g_hDBC, &pResults);
   if(DBCheck("DBExecute statement allocation", retcode, SQL_HANDLE_DBC, g_hDBC) != SQL_SUCCESS)
   {
      return false;
   }

   /* if(DBTable::Debug() == true)
   {
      // printf("DBExecute '%s'\n", pSQL);
      memprint("DBExecute SQL string", pSQL, lLength);
   } */

   dTick = gettick();
   retcode = SQLPrepare(pResults, (SQLCHAR *)pSQL, lLength);
   if(lPrepareTime != NULL)
   {
      *lPrepareTime = tickdiff(dTick);
   }
   if(DBCheck("DBExecute SQL prepare", retcode, SQL_HANDLE_STMT, pResults) != SQL_SUCCESS)
   {
      return false;
   }

   dTick = gettick();

   retcode = SQLExecute(pResults);
   if(lExecuteTime != NULL)
   {
      *lExecuteTime = tickdiff(dTick);
   }
   if(DBCheck("DBExecute SQL execute", retcode, SQL_HANDLE_STMT, pResults, pSQL, lLength) != SQL_SUCCESS)
   {
      return false;
   }

   /* retcode = SQLExecDirect(pResults, pSQL, lLength);
   if(DBCheck("DBExecute SQL execute", retcode, SQL_HANDLE_STMT, pResults) != SQL_SUCCESS)
   {
      return false;
   } */

   if(pReturn != NULL)
   {
      *pReturn = pResults;
   }
   else
   {
      DBResultsFree(pResults);
   }

   return true;
}

DBRESULTS DBResultsFree(DBRESULTS pResults)
{
   if(pResults != NULL)
   {
      SQLFreeHandle(SQL_HANDLE_STMT, pResults);
      pResults = NULL;
   }

   return pResults;
}

// DBNextRow: Fill result set with next row of data
bool DBNextRow(DBRESULTS pResults, DBField **pFields, int iNumFields)
{
   STACKTRACE
   int iFieldNum = 0;
   SQLINTEGER iColumnLen = 0, iLength = 0, iReturn = 0, iResize = 0;
   SQLRETURN retcode = SQL_SUCCESS;
   byte *pValue = NULL, *pTemp = NULL;

   // printf("DBNextRow entry, %p\n", pResults);

   retcode = SQLFetch(pResults);
   if(retcode == SQL_NO_DATA)
   {
      // printf("DBNextRow exit false, SQL_NO_DATA\n");
      return false;
   }
   else if(DBCheck("DBNextRow", retcode, SQL_HANDLE_STMT, pResults) != SQL_SUCCESS)
   {
      // printf("DBNextRow exit false, SQLFetch error\n");
      return false;
   }

   /* printf("DBNextRow entry %d,", iNumColumns);
   for(iColumnNum = 0; iColumnNum < iNumColumns; iColumnNum++)
   {
      switch(pColumns[iColumnNum]->m_iColumnType)
      {
         case DBTable::INT:
            debug(" [num]");
            break;

         case DBTable::BYTE:
            debug(" [dat]");
            break;
      }
   }
   debug("\n"); */

   STACKTRACEUPDATE

   // printf("DBNextRow fields %p\n", pFields);

   iFieldNum = 0;
   while(retcode == SQL_SUCCESS && iFieldNum < iNumFields)
   {
      /* if(DBTable::Debug() == true)
      {
         debug("DBNextRow field %d, %p\n", iFieldNum, pFields[iFieldNum]);
         debug("DBNextRow f=%d c=%d t=%d", iFieldNum, pFields[iFieldNum]->m_iColumnNum - 1, pFields[iFieldNum]->m_iType);
         if(pFields[iFieldNum]->m_iType == DBTable::BYTE)
         {
            debug(" l=%d", pFields[iFieldNum]->m_lValueLen);
         }
         debug("\n");
      } */

      STACKTRACEUPDATE

      switch(pFields[iFieldNum]->m_iType)
      {
         case DBTable::INT:
            retcode = SQLGetData(pResults, pFields[iFieldNum]->m_iColumnNum, SQL_C_LONG, &pFields[iFieldNum]->m_dValue, sizeof(pFields[iFieldNum]->m_dValue), NULL);
            // printf("DBNextRow v=%ld\n", pFields[iFieldNum]->m_lValue);
            break;

         case DBTable::BYTES:
            if(pFields[iFieldNum]->m_pValue != NULL)
            {
               delete pFields[iFieldNum]->m_pValue;
            }

            iLength = INIT_VALUE;
            pValue = new byte[iLength + 1];
            retcode = SQLGetData(pResults, pFields[iFieldNum]->m_iColumnNum, SQL_C_CHAR, pValue, iLength, &iReturn);
            if(retcode == SQL_SUCCESS)
            {
               if(iReturn == -1)
               {
                  // pFields[iFieldNum]->m_pValue[0] = '\0';
                  // iLength = 0;
                  pFields[iFieldNum]->m_pValue = NULL;
               }
               else
               {
                  pFields[iFieldNum]->m_pValue = new bytes(pValue, iReturn);
               }

               // printf("DBNextRow v=%s (%d of %d bytes)\n", pFields[iFieldNum]->m_pValue, iLength, iReturn);
            }
            else if(retcode == SQL_SUCCESS_WITH_INFO && iReturn > 0)
            {
               debug("DBNextRow get %d returned %d, %d\n", iLength, retcode, iReturn);
               // memprint("DBNextRow byte field", pFields[iFieldNum]->m_pValue, iLength);

               iResize = iLength;
               iLength = iReturn;

               pTemp = new byte[iReturn + 2];
               memcpy(pTemp, pValue, iResize);
               delete[] pValue;
               pValue = pTemp;

               retcode = SQLGetData(pResults, pFields[iFieldNum]->m_iColumnNum, SQL_C_CHAR, (byte *)&pValue[iResize - 1], iLength, &iReturn);
               pFields[iFieldNum]->m_pValue = new bytes(pValue, iLength);
               // pFields[iFieldNum]->m_lValueLen = iLength;

               debug("DBNextRow get %d returned %d, %d\n", iLength, retcode, iReturn);
               // memprint("DBNextRow byte field", pFields[iFieldNum]->m_pValue, pFields[iFieldNum]->m_lValueLen);

               // printf("DBNextRow v=%s (%d of %d bytes)\n", pFields[iFieldNum]->m_pValue, iLength, iReturn);
            }
            delete[] pValue;

            pFields[iFieldNum]->m_bCopy = true;
            // memprint("DBNextRow byte field", pFields[iFieldNum]->m_pValue, pFields[iFieldNum]->m_lValueLen);
            break;
      }

      retcode = DBCheck("DBNextRow get data", retcode, SQL_HANDLE_STMT, pResults);
      // printf("DBNextRow retcode %d(%d, info %d)\n", retcode, SQL_SUCCESS, SQL_SUCCESS_WITH_INFO);

      /* if(retcode == SQL_SUCCESS)
      {
         debug("DBNextRow Field %d, ", iFieldNum);
         switch(pFields[iFieldNum]->m_iType)
         {
            case DBTable::INT:
               debug("%ld", pFields[iFieldNum]->m_lValue);
               break;

            case DBTable::BYTE:
               if(pFields[iFieldNum]->m_lLength != -1)
               {
                  debug("(%d bytes) '", pFields[iFieldNum]->m_lLength);
                  memprint(pFields[iFieldNum]->m_pValue, pFields[iFieldNum]->m_lLength);
                  debug("'");
               }
               else
               {
                  debug("NULL");
                  pFields[iFieldNum]->m_pValue = NULL;
               }
               break;
         }
         debug("\n");
      } */

      iFieldNum++;
   }

   STACKTRACEUPDATE

   // printf("DBNextRow final retcode %d\n", retcode);

   if(retcode != SQL_SUCCESS)
   {
      // printf("DBNextRow exit false, retcode != SQL_SUCCESS\n");
      return false;
   }

   // printf("DBNextRow exit true\n");
   return true;
}

// DBLiteralise: Escape problem characters
long DBLiteralise(byte *pDest, long lDestPos, bytes *pSource)
{
   long lValueLen = 0, lValuePos = 0, lDestLen = 0;
   byte *pValue = NULL;

   pValue = pSource->Data(false);
   lValueLen = pSource->Length();

   for(lValuePos = 0; lValuePos < lValueLen; lValuePos++)
   {
      if(pValue[lValuePos] == '\'')
      {
         pDest[lDestPos++] = '\'';
      }
      /* else if(pSource[lSourcePos] == '\0')
      {
         pDest[lDestPos++] = '\\';
      } */
      pDest[lDestPos++] = pValue[lValuePos];
   }

   return lDestPos;
}

int DBError()
{
   return 0;
}