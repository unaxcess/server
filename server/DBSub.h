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
** DBSub.h: Definition of DBSub class
*/

#ifndef _DBSUB_H_
#define _DBSUB_H_

#include "DBItemList.h"
#include "db/DBTable.h"

#include "EDF/EDF.h"

// Data to be stored by DBItemList
struct DBSubData
{
   char m_iSubType;
   int m_iExtra;
   bool m_bTemp;
};

class DBSub : public DBItemList<DBSubData *>
{
public:
   DBSub();
   DBSub(EDF *pEDF, char *szType);
   DBSub(char *szTable, char *szID, long lUserID, int iLevel = 0, int iInitExtra = -1);
   DBSub(EDF *pEDF, char *szType, char *szTable, char *szID, long lUserID, int iLevel = 0, int iInitExtra = -1);
   ~DBSub();

   bool Add(long lID, int iSubType, int iExtra, bool bTemp);
   bool Update(long lID, int iSubType, int iExtra, bool bTemp);

   static bool UserAdd(char *szTable, char *szID, long lUserID, long lID, int iSubType, int iExtra);
   static bool UserDelete(char *szTable, char *szID, long lUserID, long lID);
   static bool UserUpdate(char *szTable, char *szID, long lUserID, long lID, int iSubType, int iExtra);
   static DBSubData *UserData(char *szTable, char *szID, long lUserID, long lID);

   long ItemID(int iItemNum);
   int ItemSubType(int iItemNum);
   int ItemExtra(int iExtra);

   DBSubData *SubData(long lID);
   int SubType(long lID);
   int CountType(int iSubType);

   static int SubType(char *szTable, char *szID, long lUserID, long lID);
   static int CountType(char *szTable, long lUserID, int iSubType);

   static int IntType(DBTable *pTable, int iFieldNum);
   static char CharType(int iSubType);

   bool Write(bool bLock = true);
   bool Write(EDF *pEDF, char *szType);

protected:
   bool ItemCompare(DBSubData *pData1, DBSubData *pData2);

   bool PreWrite(bool bLock);
   bool ItemWrite(DBItem<DBSubData *> *pItem);
   bool PostWrite(bool bLock);

private:
   char *m_szTable;
   char *m_szID;
   int m_iLevel;

   void setup(char *szTable, char *szID, long lUserID = 0, int iLevel = 0, int iInitExtra = -1, EDF *pEDF = NULL, char *szType = NULL);

   static bool DBItemWrite(char *szTable, char *szID, int iLevel, int iOp, long lUserID, long lID, int iSubType, int iExtra, bool bTemp);
};

#endif
