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
** DBItemList.cpp: Implementation of DBItemList class
*/

#ifndef _DBITEMLIST_CPP_
#define _DBITEMLIST_CPP_

#include <stdio.h>
#include <string.h>

// #include "DBItemList.h"

#define ARRAY_STEP 10

template <class T> DBItemList<T>::DBItemList(long lID, bool bDelete, bool bWrite) : ItemList<DBItem<T> *>(true)
{
   STACKTRACE

   m_lID = lID;
   m_bWrite = bWrite;
   m_bChanged = false;
   m_bDataDelete = bDelete;
}

template <class T> DBItemList<T>::~DBItemList()
{
   STACKTRACE
   DBItem<T> *pItem = NULL;

   int iItemNum = 0;

   if(m_bChanged == true)
   {
      Write();
   }

   if(m_bDataDelete == true)
   {
      for(iItemNum = 0; iItemNum < this->Count(); iItemNum++)
      {
         pItem = this->Item(iItemNum);
         // printf("DBItemList::~DBItemList data %d %p\n", iItemNum, pItem->m_pData);
         delete pItem->m_pData;
      }
   }
}

template <class T> int DBItemList<T>::Add(long lID, T pItem, int iOp)
{
   STACKTRACE
   int iItemNum = 0;
   bool bAdd = false;
   DBItem<T> *pAdd = NULL, *pChange = NULL;

   // if(Debug() == true)
   {
      debug(DEBUGLEVEL_DEBUG, "DBItemList::Add entry %ld %p %d\n", lID, pItem, iOp);
   }

   pAdd = new DBItem<T>;
   pAdd->m_lID = lID;
   pAdd->m_pData = pItem;
   pAdd->m_iOp = iOp;

   bAdd = ItemList<DBItem<T> *>::Add(pAdd, &iItemNum);

   if(bAdd == false && iItemNum == -1)
   {
      // Failed to add item
      if(m_bDataDelete == true)
      {
         delete pAdd->m_pData;
      }
      delete pAdd;

      // if(Debug() == true)
      {
         debug(DEBUGLEVEL_DEBUG, "DBItemList::Add exit ADD_NO, failed\n");
      }
      return ADD_NO;
   }
   else if(bAdd == true)
   {
      // Item added

      if(m_bWrite == true && pAdd->m_iOp > 0 && pAdd->m_iOp != OP_NONE && ItemWrite(pAdd) == true)
      {
         // if(Debug() == true)
         {
            debug(DEBUGLEVEL_DEBUG, "DBItemList::Add add written, OP_NONE\n");
         }
         pAdd->m_iOp = OP_NONE;
      }
      else if(pAdd->m_iOp != OP_NONE)
      {
         // if(Debug() == true)
         {
            debug(DEBUGLEVEL_DEBUG, "DBItemList::Add add changed\n");
         }

         m_bChanged = true;
      }

      // if(Debug() == true)
      {
         debug(DEBUGLEVEL_DEBUG, "DBItemList::Add exit ADD_YES, add\n");
      }
      return ADD_YES;
   }

   // Item not added (already present)
   delete pAdd;

   // if(Debug() == true)
   {
      debug(DEBUGLEVEL_DEBUG, "DBItemList::Add not added (item %d)\n", iItemNum);
   }

   pChange = this->Item(iItemNum);
   if(Op(pChange) == OP_DELETE)
   {
      Undelete(pChange);

      if(ItemCompare(pChange->m_pData, pItem) == true)
      {
         // if(Debug() == true)
         {
            debug(DEBUGLEVEL_DEBUG, "DBItemList::Add undelete same data\n");
         }

         if(m_bDataDelete == true)
         {
            delete pItem;
         }

         // pChange->m_iOp = OP_NONE;
      }
      else
      {
         // if(Debug() == true)
         {
            debug(DEBUGLEVEL_DEBUG, "DBItemList::Add undelete different data\n");
         }
         if(m_bDataDelete == true)
         {
            delete pChange->m_pData;
         }
         pChange->m_pData = pItem;
         // pChange->m_iOp = OP_UPDATE;

         m_bChanged = true;
      }

      // if(Debug() == true)
      {
         debug(DEBUGLEVEL_DEBUG, "DBItemList::Add exit ADD_UNDELETE\n");
      }

      return ADD_UNDELETE;
   }
   else if(ItemCompare(pChange->m_pData, pItem) == false)
   {
      // if(Debug() == true)
      {
         // debug(DEBUGLEVEL_DEBUG, "DBItemList::Add change OP_UPDATE\n");
      }
      if(m_bDataDelete == true)
      {
         delete pChange->m_pData;
      }
      pChange->m_pData = pItem;

      if(m_bWrite == true && pChange->m_iOp > 0 && pChange->m_iOp != OP_NONE && ItemWrite(pChange) == true)
      {
         // if(Debug() == true)
         {
            debug(DEBUGLEVEL_DEBUG, "DBItemList::Add change written, OP_NONE\n");
         }
         pChange->m_iOp = OP_NONE;
      }
      else
      {
         if(Op(pChange) == OP_NONE)
         {
            debug(DEBUGLEVEL_DEBUG, "DBItemList::Add change change OP_UPDATE\n");
            pChange->m_iOp = OP_UPDATE;
         }

         m_bChanged = true;
      }

      // if(Debug() == true)
      {
         debug(DEBUGLEVEL_DEBUG, "DBItemList::Add exit ADD_CHANGE\n");
      }
      return ADD_CHANGE;
   }

   // if(Debug() == true)
   {
      debug(DEBUGLEVEL_DEBUG, "DBItemList::Add exit ADD_NO\n");
   }
   return ADD_NO;
}

template <class T> long DBItemList<T>::GetID()
{
   return m_lID;
}

template <class T> bool DBItemList<T>::Delete(long lID)
{
   STACKTRACE
   int iItemNum = 0;
   DBItem<T> *pItem = NULL;

   pItem = this->Find(lID, &iItemNum);

   // if(Debug() == true)
   {
      debug(DEBUGLEVEL_DEBUG, "DBItemList::Delete entry %ld, %p\n", lID, pItem);
   }

   if(pItem == NULL)
   {
      // if(Debug() == true)
      {
         debug(DEBUGLEVEL_DEBUG, "DBItemList::Delete exit false, not found\n");
      }
      return false;
   }

   Delete(pItem);

   if(m_bWrite == true && pItem->m_iOp > 0 && pItem->m_iOp != OP_NONE && ItemWrite(pItem) == true)
   {
      // if(Debug() == true)
      {
         debug(DEBUGLEVEL_DEBUG, "DBItemList::Delete written\n");
      }
      if(m_bDataDelete == true)
      {
         delete pItem->m_pData;
         pItem->m_pData = NULL;
      }
      this->ItemDelete(iItemNum);
   }
   else
   {
      m_bChanged = true;
   }

   // if(Debug() == true)
   {
      debug(DEBUGLEVEL_DEBUG, "DBItemList::Delete exit true\n");
   }
   return true;
}

template <class T> bool DBItemList<T>::Write(bool bLock)
{
   STACKTRACE
   int iItemNum = 0;
   double dTick = gettick();
   DBItem<T> *pItem = NULL;

   debug("DBItemList::Write %s, %s\n", BoolStr(bLock), BoolStr(m_bChanged));

   if(m_bChanged == true)
   {
      // printf("DBItemList::Write has changed\n");

      STACKTRACEUPDATE

      PreWrite(bLock);

      STACKTRACEUPDATE

      debug(DEBUGLEVEL_DEBUG, "DBItemList::Write checking item ops\n");

      while(iItemNum < this->Count())
      {
         pItem = this->Item(iItemNum);
         if(pItem->m_iOp > 0 && pItem->m_iOp != OP_NONE)
         {
            STACKTRACEUPDATE
            if(ItemWrite(pItem) == true)
            {
               STACKTRACEUPDATE
               if(Op(pItem) == OP_DELETE)
               {
                  this->ItemDelete(iItemNum);
               }
               else
               {
                  pItem->m_iOp = OP_NONE;
                  iItemNum++;
               }
            }
            else
            {
               iItemNum++;
            }
         }
         else
         {
            iItemNum++;
         }
      }

      STACKTRACEUPDATE
      PostWrite(bLock);

      m_bChanged = false;

      STACKTRACEUPDATE
      // if(tickdiff(dTick) > 50)
      {
         debug("DBItemList::Write %ld ms\n", tickdiff(dTick));
      }
   }

   return true;
}

/* template <class T> void DBItemList<T>::ItemDelete(DBItem<T> *pItem)
{
   ItemDelete(pItem->m_pData);

   delete pItem;
} */

template <class T> long DBItemList<T>::ItemID(DBItem<T> *pItem)
{
   return pItem->m_lID;
}

template <class T> bool DBItemList<T>::GetChanged()
{
   return m_bChanged;
}

template <class T> bool DBItemList<T>::SetChanged(bool bChanged)
{
   m_bChanged = bChanged;

   if(m_bChanged == true && m_bWrite == true)
   {
      Write();
   }

   return true;
}

template <class T> bool DBItemList<T>::GetWrite()
{
   return m_bWrite;
}

template <class T> bool DBItemList<T>::SetWrite(bool bWrite)
{
   m_bWrite = bWrite;

   return true;
}

template <class T>int DBItemList<T>::Op(int iOp)
{
   if(mask(iOp, OP_DELETE) == true)
   {
      return OP_DELETE;
   }

   return iOp;
}

template <class T>int DBItemList<T>::Op(DBItem<T> *pItem)
{
   return Op(pItem->m_iOp);
}

template <class T>void DBItemList<T>::Delete(DBItem<T> *pItem)
{
   if(mask(pItem->m_iOp, OP_DELETE) == false)
   {
      pItem->m_iOp += OP_DELETE;
   }
}

template <class T>void DBItemList<T>::Undelete(DBItem<T> *pItem)
{
   if(mask(pItem->m_iOp, OP_DELETE) == true)
   {
      pItem->m_iOp -= OP_DELETE;
   }
}

#endif
