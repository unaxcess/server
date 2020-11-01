#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "useful/useful.h"

#include "DBTable.h"
#include "db-common.h"

MYSQL *g_hSQL = NULL;

int g_iNumLocks = 0;
char g_szLocks[200];

#define DB_SUCCESS true

#define IS_STR(x) (x == FIELD_TYPE_STRING || x == FIELD_TYPE_VAR_STRING || x == FIELD_TYPE_BLOB)

bool DBCheck(char *szTitle, bool bCondition)
{
   bool bReturn = true;

   if(mysql_errno(g_hSQL) != 0 || bCondition == false)
   {
      debug("%s (error)", szTitle);
      if(mysql_errno(g_hSQL) != 0)
      {
         debug(": %s", mysql_error(g_hSQL));
      }
      debug("\n");

      bReturn = false;
   }
   else
   {
      debug(DEBUGLEVEL_DEBUG, "%s (no error)\n", szTitle);
   }

   return bReturn;
}

bool DBConnect(char *szDatabase, char *szUsername, char *szPassword, char *szHostname)
{
   bool bReturn = false;
   MYSQL *retcode = NULL;

   debug("DBConnect entry %s %s %s %s\n", szDatabase, szUsername, szPassword, szHostname);
   g_hSQL = mysql_init(NULL);
   if(DBCheck("DBConect init", g_hSQL != NULL) == true)
   {
      retcode = mysql_real_connect(g_hSQL, szHostname, szUsername, szPassword, szDatabase, 0, NULL, 0);
      if(DBCheck("DBConnect connection", retcode != NULL) == true)
      {
         bReturn = true;
      }
   }

   return bReturn;
}

bool DBDisconnect()
{
   /* if(g_hResult != NULL)
   {
      mysql_free_result(g_hResult);
      g_hResult = NULL;
   } */

   if(g_hSQL != NULL)
   {
      mysql_close(g_hSQL);
      g_hSQL = NULL;
   }

   return true;
}

bool DBExecute(byte *pSQL, long lLength, DBRESULTS *pReturn, long *lPrepareTime, long *lExecuteTime)
{
   int retcode = 0;
   double dTick = gettick();
   DBRESULTS pResults = NULL;

   debug(DEBUGLEVEL_DEBUG, "DBExecute %p %ld\n", pSQL, lLength);
   retcode = mysql_real_query(g_hSQL, (char *)pSQL, lLength);
   if(DBCheck("DBExecute run", retcode == 0) == false)
   {
      return false;
   }

   if(mysql_field_count(g_hSQL) > 0)
   {
      pResults = mysql_store_result(g_hSQL);
      if(DBCheck("DBExecute store", pResults != NULL) == false)
      {
         return false;
      }
   }

   if(lExecuteTime != NULL)
   {
      *lExecuteTime = tickdiff(dTick);
   }

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
   // printf("DBResultsFree %p\n", pResults);

   if(pResults != NULL)
   {
      mysql_free_result(pResults);
      pResults = NULL;
   }

   return pResults;
}

bool DBNextRow(DBRESULTS pResults, DBField **pFields, int iNumFields)
{
   STACKTRACE
   int iFieldNum = 0, iNumLengths = 0;
   MYSQL_ROW pRow = NULL;
   MYSQL_FIELD *pField = NULL;
   unsigned long *pLengths = NULL;

   debug(DEBUGLEVEL_DEBUG, "DBNextRow entry %p\n", pFields);

   if(pResults == NULL)
   {
      return false;
   }

   pRow = mysql_fetch_row(pResults);

   if(pRow == NULL)
   {
      // printf("DBNextRow exit false\n");
      return false;
   }

   iNumLengths = mysql_num_fields(pResults);
   debug(DEBUGLEVEL_DEBUG, "DBNextRow %d fields\n", iNumLengths);
   // pLengths = new unsigned long[iNumLengths];
   pLengths = mysql_fetch_lengths(pResults);

   if(pFields != NULL)
   {
      for(iFieldNum = 0; iFieldNum < iNumFields; iFieldNum++)
      {
         // printf("DBNextRow %d %d\n", iFieldNum, pFields[iFieldNum]->m_iType);
         if(pFields[iFieldNum]->m_iType == DBTable::BYTES)
         {
            // printf("DBNextRow deleting %p\n", pFields[iFieldNum]->m_pValue);
            delete pFields[iFieldNum]->m_pValue;
            pFields[iFieldNum]->m_pValue = NULL;
         }
      }

      iFieldNum = 0;
      // printf("DBNextRow value extract %d\n", pFields->m_iNumFields);
      while(iFieldNum != -1 && iFieldNum < iNumFields)
      {
         /* if(DBTable::Debug() == true)
         {
            printf("DBNextRow fetch field %d", pFields[iFieldNum]->m_iColumnNum - 1);
         } */
         pField = mysql_fetch_field_direct(pResults, pFields[iFieldNum]->m_iColumnNum - 1);
         /* if(DBTable::Debug() == true)
         {
            printf(" -> %p\n", pField);
         } */

         if(DBCheck("DBNextRow fetch", pField != NULL) == true)
         {
            debug(DEBUGLEVEL_DEBUG, "DBNextRow o=%d c=%d t=%d l=%ld\n", iFieldNum, pFields[iFieldNum]->m_iColumnNum - 1, pField->type, pLengths[pFields[iFieldNum]->m_iColumnNum - 1]);
            /* if(pFields[iFieldNum]->m_iType == DBTable::BYTES && pRow[pFields[iFieldNum]->m_iColumnNum - 1] != NULL)
            {
               printf("  ");
               memprint((byte *)pRow[pFields[iFieldNum]->m_iColumnNum - 1], pLengths[pFields[iFieldNum]->m_iColumnNum - 1], false);
               printf("\n");
            } */

            switch(pFields[iFieldNum]->m_iType)
            {
               case DBTable::INT:
                  /* if(DBTable::Debug() == true)
                  {
                     printf("DBNextRow number check %s (%s)\n", pRow[pFields[iFieldNum]->m_iColumnNum - 1], BoolStr(IS_NUM(pField->type)));
                  } */

                  // printf("DBNextRow int value %s\n", pRow[pFields[iFieldNum]->m_iColumnNum - 1]);
                  if(IS_NUM(pField->type) && pRow[pFields[iFieldNum]->m_iColumnNum - 1] != NULL)
                  {
                     pFields[iFieldNum]->m_lValue = atol(pRow[pFields[iFieldNum]->m_iColumnNum - 1]);
                  }
                  else
                  {
                     debug(DEBUGLEVEL_DEBUG, "DBNextRow column %d not DBTable::INT\n", pFields[iFieldNum]->m_iColumnNum - 1);
                     iFieldNum = -1;
                  }
                  break;

               case DBTable::FLOAT:
                  /* if(DBTable::Debug() == true)
                  {
                     printf("DBNextRow number check %s (%s)\n", pRow[pFields[iFieldNum]->m_iColumnNum - 1], BoolStr(IS_NUM(pField->type)));
                  } */

                  debug("DBNextRow float value %s\n", pRow[pFields[iFieldNum]->m_iColumnNum - 1]);
                  if(IS_NUM(pField->type) && pRow[pFields[iFieldNum]->m_iColumnNum - 1] != NULL)
                  {
                     pFields[iFieldNum]->m_dValue = atof(pRow[pFields[iFieldNum]->m_iColumnNum - 1]);
                  }
                  else
                  {
                     debug(DEBUGLEVEL_DEBUG, "DBNextRow column %d not DBTable::INT\n", pFields[iFieldNum]->m_iColumnNum - 1);
                     iFieldNum = -1;
                  }
                  break;

               case DBTable::BYTES:
                  /* if(DBTable::Debug() == true)
                  {
                     printf("DBNextRow string check %s, %d\n", pRow[pFields[iFieldNum]->m_iColumnNum - 1], pField->type);
                  } */

                  if(IS_STR(pField->type))
                  {
                     pFields[iFieldNum]->m_pValue = new bytes(pRow[pFields[iFieldNum]->m_iColumnNum - 1], pLengths[pFields[iFieldNum]->m_iColumnNum - 1]);
                     // memprint(DEBUGLEVEL_DEBUG, "DBNextRow src", (byte *)pRow[pFields[iFieldNum]->m_iColumnNum - 1], pLengths[pFields[iFieldNum]->m_iColumnNum - 1], false);
                     // bytesprint(DEBUGLEVEL_DEBUG, "DBNextRow dst", pFields[iFieldNum]->m_pValue, false);
                     // printf("DBNextRow allocated %ld bytes at %p\n", pLengths[pFields[iFieldNum]->m_iColumnNum - 1], pFields[iFieldNum]->m_pValue);
                     // pFields[iFieldNum]->m_lValueLen = pLengths[pFields[iFieldNum]->m_iColumnNum - 1];
                     pFields[iFieldNum]->m_bCopy = true;
                  }
                  else if(pField->type == FIELD_TYPE_NULL)
                  {
                     pFields[iFieldNum]->m_pValue = NULL;
                     // pFields[iFieldNum]->m_lValueLen = 0;
                  }
                  else
                  {
                     debug(DEBUGLEVEL_DEBUG, "DBNextRow column %d not DBTable::BYTES\n", pFields[iFieldNum]->m_iColumnNum - 1);
                     iFieldNum = -1;
                  }
                  break;

               default:
                  debug(DEBUGLEVEL_DEBUG, "DBNextRow invalid data type %d\n", pFields[iFieldNum]->m_iType);
                  iFieldNum = -1;
                  break;
            }
         }

         /* if(DBCheck("DBNextRow get data", retcode, SQL_HANDLE_STMT, hStmt) == SQL_SUCCESS)
         {
            printf("DBNextRow field %d, ", iFieldNum);
            switch(pFields[iFieldNum]->m_iFieldType)
            {
               case :
                  break;
            }
            printf("\n");
         } */

         if(iFieldNum != -1)
         {
            iFieldNum++;
         }
      }
   }

   // delete[] pLengths;

   if(iFieldNum == -1)
   {
      debug(DEBUGLEVEL_DEBUG, "DBNextRow exit false\n");
      return false;
   }

   debug(DEBUGLEVEL_DEBUG, "DBNextRow exit true\n");
   return true;
}

bool DBLock(char *szTable)
{
   char szSQL[200];

   if(g_iNumLocks > 0)
   {
      strcat(g_szLocks, ", ");
   }
   else
   {
      g_szLocks[0] = '\0';
   }
   g_iNumLocks++;
   strcat(g_szLocks, szTable);
   strcat(g_szLocks, " write");

   sprintf(szSQL, "lock tables %s", g_szLocks);
   debug(DEBUGLEVEL_DEBUG, "DBLock execute %s\n", szSQL);
   return DBExecute((byte *)szSQL, strlen(szSQL), NULL, NULL);
}

bool DBUnlock()
{
   g_iNumLocks = 0;

   return DBExecute((byte *)"unlock tables", 13, NULL, NULL);
}

long DBLiteralise(byte *pDest, long lDestPos, bytes *pSource)
{
   return lDestPos + mysql_real_escape_string(g_hSQL, (char *)&pDest[lDestPos], (char *)pSource->Data(false), pSource->Length());
}

int DBError()
{
   return mysql_errno(g_hSQL);
}
