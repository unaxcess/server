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
** DBItemList.h: Definition of DBItemList class
**
** Builds on the ItemList class to add database writeback and change flags
*/

#ifndef _DBITEMLIST_H_
#define _DBITEMLIST_H_

#include "useful/useful.h"

#include "ItemList.h"

template <class T> class DBItem
{
public:
   long m_lID;
   T m_pData;
   char m_iOp;
};

template <class T> class DBItemList : public ItemList <DBItem <T> *>
{
public:
   DBItemList(long lID, bool bDelete, bool bWrite);
   virtual ~DBItemList();

   long GetID();

   bool Delete(long lID);

   bool Write(bool bLock = true);

   bool GetChanged();
   bool SetChanged(bool bChanged);

protected:
   enum { ADD_YES = 1, ADD_NO = 0, ADD_CHANGE = 2, ADD_UNDELETE } ;
   enum { OP_NONE = 1, OP_ADD = 2, OP_UPDATE = 3, OP_DELETE = 4 } ;

   int Add(long lID, T pItem, int iOp = OP_ADD);

   long ItemID(DBItem<T> *pItem);

   virtual bool ItemCompare(T pItem1, T pItem2) = 0;

   virtual bool PreWrite(bool bLock) = 0;
   virtual bool ItemWrite(DBItem<T> *pItem) = 0;
   virtual bool PostWrite(bool bLock) = 0;

   bool GetWrite();
   bool SetWrite(bool bWrite);

   static int Op(int iOp);
   static int Op(DBItem<T> *pItem);
   static void Delete(DBItem<T> *pItem);
   static void Undelete(DBItem<T> *pItem);

private:
   long m_lID;
   bool m_bWrite;
   bool m_bChanged;
   bool m_bDataDelete;
};

#include "DBItemList.cpp"

#endif
