/*
** DBTable library
** (c) 1999 Michael Wood (mike@compsoc.man.ac.uk)
**
** DBTable.h: Definition of DBTable class
**
** The DBTable class acts as an interface to the underlying database which
** is implemented separately
*/

#ifndef _DBTABLE_H_
#define _DBTABLE_H_

#ifdef DBODBC
#include <windows.h>
#include <sql.h>
#include <sqlext.h>

#define DBRESULTS HSTMT
#endif

#ifdef DBMYSQL
#include <mysql.h>
#include <mysqld_error.h>

#define DBRESULTS MYSQL_RES *
#endif

#include "useful/useful.h"

struct DBField
{
   char *m_szColumnName;
   int m_iColumnNum;

   char m_iType;
   union
   {
      long m_lValue;
      double m_dValue;
      bytes *m_pValue;
      // long m_lValueLen;
   };
   bool m_bCopy;
};

class DBTable
{
public:
   enum { STR = 1, BYTES = 2, INT = 3, FLOAT = 4 };

   DBTable();
   ~DBTable();

   static bool Connect(char *szDatabase, char *szUsername = NULL, char *szPassword = NULL, char *szHostname = NULL);
   static bool Disconnect();

   // Data get methods
   bool BindColumnBytes(long lColumnWidth = -1);
   bool BindColumnBytes(int iColumnNum, long lValueLen);
   bool BindColumnInt();
   bool BindColumnInt(int iColumnNum);

   // Data set methods
   bool SetField(const char *szValue, bool bCopy = true);
   // bool SetField(const char *szColumn, const char *szValue, bool bCopy = true);
   bool SetField(bytes *pValue, bool bCopy = true);
   // bool SetField(const char *szColumn, bytes *pValue, bool bCopy = true);
   bool SetField(long lValue);
   // bool SetField(const char *szColumn, long lValue);

   int CountFields();

   // SQL methods
   /* bool Reset();
   bool Select(char *szSelect);
   bool Select(bytes *pSelect);
   bool WhereColumn(char *szColumn, long lValue);
   bool WhereColumn(char *szColumn, char *szValue);
   bool WhereColumn(char *szColumn, bytes *pValue);
   bool Where(char *szCondidition);
   bool Where(bytes *pCondition); */
   
   bool Execute(long *lPrepareTime = NULL, long *lExecuteTime = NULL);
   bool Execute(char *szSQL, long *lPrepareTime = NULL, long *lExecuteTime = NULL);
   bool Execute(byte *pSQL, long lSQLLen, long *lPrepareTime = NULL, long *lExecuteTime = NULL);
   bool SetResults(DBRESULTS *pResults);
   bool NextRow();

   int GetFieldType(int iFieldNum);
   bool GetField(int iFieldNum, char **szValue);
   bool GetField(int iFieldNum, bytes **pValue);
   bool GetField(int iFieldNum, int *iValue);
   bool GetField(int iFieldNum, long *lValue);
   bool GetField(int iFieldNum, unsigned long *lValue);
   bool GetField(int iFieldNum, bool *bValue);

   bool Insert(char *szTable);

   static bool Lock(char *szTable);
   static bool Unlock();

   // static bool Debug(bool bDebug);
   // static bool Debug();

   static long Literalise(byte *pDest, long lDestPos, bytes *pSource);

   int Error();

private:
   DBField **m_pFields;
   int m_iNumFields;

   DBRESULTS m_pResults;

   // static bool m_bDebug;

   bool add(char *szColumnName, int iColumnNum, int iType, void *pValue, long lValueLen, long lValue, double dValue, bool bSet, bool bCopy);
   int get(int iFieldNum, int iType, bytes **pValue, long *lValue, double *dValue);
};

#endif
