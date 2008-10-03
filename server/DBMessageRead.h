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
** DBMessageRead.h: Definition of DBMessageRead class
*/

#ifndef _DBMESSAGEREAD_H_
#define _DBMESSAGEREAD_H_

#include "DBItemList.h"

#define FM_READ_TABLE "folder_message_read"

#define MARKTYPE_MSGCHILD 1
#define MARKTYPE_CHILD 2

struct DBMessageReadData
{
   char m_iMarkType;
};

class DBMessageRead : public DBItemList<DBMessageReadData *>
{
public:
   DBMessageRead(long lUserID);
   ~DBMessageRead();

   bool Add(long lMessageID, int iMarkType);

   static bool UserAdd(long lUserID, long lMessageID, int iMarkType);
   static bool UserDelete(long lUserID, long lMessageID);
   static int UserType(long lUserID, long lMessageID);

   int Get(long lMessageID);

protected:
   bool ItemCompare(DBMessageReadData *pData1, DBMessageReadData *pData2);

   bool PreWrite(bool bLock);
   bool ItemWrite(DBItem<DBMessageReadData *> *pItem);
   static bool ItemWrite(int iOp, long lUserID, long lMessageID, int iMarkType);
   bool PostWrite(bool bLock);
};

#endif
