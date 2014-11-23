/*
** DBTable library
** (c) 1999 Michael Wood (mike@compsoc.man.ac.uk)
**
** db-common.h: Implementation dependent functions
**
*/

#ifndef _DBCOMMON_H_
#define _DBCOMMON_H_

bool DBConnect(char *szDatabase, char *szUsername, char *szPassword, char *szHostname);
bool DBDisconnect();

bool DBExecute(byte *pSQL, long pSQLLen, DBRESULTS *pReturn = NULL, long *lPrepareTime = NULL, long *lExecuteTime = NULL);
DBRESULTS DBResultsFree(DBRESULTS pResults);

bool DBLock(char *szTables);
bool DBUnlock();

bool DBNextRow(DBRESULTS pResults, DBField **pFields, int iNumFields);

long DBLiteralise(byte *pDest, long lDestPos, bytes *pSource);

int DBError();

#endif
