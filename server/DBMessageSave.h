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
** DBMessageSave.h: Definition of saved message class
*/

#ifndef _DBMESSAGESAVE_H_
#define _DBMESSAGESAVE_H_

#include "DBItemList.h"

#define FM_SAVE_TABLE "folder_message_save"

struct DBMessageSaveData
{
};

class DBMessageSave : public DBItemList<DBMessageSaveData *>
{
public:
   DBMessageSave(long lUserID);
   ~DBMessageSave();

   bool Add(long lMessageID);

   static bool UserAdd(long lUserID, long lMessageID);
   static bool UserDelete(long lUserID, long lMessageID);

   bool IsSaved(long lMessageID);

protected:
   bool ItemCompare(DBMessageSaveData *pData1, DBMessageSaveData *pData2);

   bool PreWrite(bool bLock);
   bool ItemWrite(DBItem<DBMessageSaveData *> *pItem);
   static bool ItemWrite(int iOp, long lUserID, long lMessageID);
   bool PostWrite(bool bLock);
};

#endif
