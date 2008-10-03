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
** ItemList.cpp: Implementation of ItemList class
*/

#ifndef _ITEMLIST_CPP_
#define _ITEMLIST_CPP_

#define ARRAY_STEP 10

// template <class T> bool ItemList<T>::m_bDebug = false;

template <class T> ItemList<T>::ItemList(bool bDelete)
{
   STACKTRACE

   m_bDelete = bDelete;

   m_pItems = NULL;
   m_iNumItems = 0;
   m_iMaxItems = 0;
}

template <class T> ItemList<T>::~ItemList()
{
   STACKTRACE
   int iItemNum = 0;

   // printf("ItemList::~ItemList %p\n", this);

   if(m_bDelete == true)
   {
      STACKTRACEUPDATE
      for(iItemNum = 0; iItemNum < m_iNumItems; iItemNum++)
      {
         // printf("ItemList::~ItemList item %d %p\n", iItemNum, m_pItems[iItemNum]);
         delete m_pItems[iItemNum];
      }
   }

   STACKTRACEUPDATE

   delete[] m_pItems;
}

template <class T>bool ItemList<T>::Delete(long lID)
{
   STACKTRACE
   int iItemNum = 0;
   // int iDelete = 0;
   T pItem = NULL;

   // if(m_bDebug == true)
   {
      debug(DEBUGLEVEL_DEBUG, "ItemList::Delete entry %ld\n", lID);
   }

   pItem = Find(lID, &iItemNum);

   if(pItem == NULL)
   {
      // if(m_bDebug == true)
      {
         debug(DEBUGLEVEL_DEBUG, "ItemList::Delete exit false, %ld not found\n", lID);
      }
      return false;
   }

   /* for(iDelete = 0; iDelete < m_iNumItems; iDelete++)
   {
      debug("ItemList::Delete item %d, %ld\n", iDelete, ItemID(m_pItems[iDelete]));
   } */

   // debug("ItemList::Delete removing %d\n", iItemNum);

   if(m_bDelete == true)
   {
      delete pItem;
   }

   ARRAY_DEC(T, m_pItems, m_iNumItems, iItemNum)

   /* for(iDelete = 0; iDelete < m_iNumItems; iDelete++)
   {
      debug("ItemList::Delete item %d, %ld\n", iDelete, ItemID(m_pItems[iDelete]));
   } */

   // if(m_bDebug == true)
   {
      debug(DEBUGLEVEL_DEBUG, "ItemList::Delete exist true\n");
   }
   return true;
}

template <class T>T ItemList<T>::Find(long lID, int *iItemNum)
{
   STACKTRACE
   int iMin = 0, iMax = 0, iMid = 0;
   T pReturn = NULL;

   // debug("EDFItemList::find %ld %p, %d\n", lID, pMid, m_iNumItems);
   if(m_iNumItems == 0)
   {
      STACKTRACEUPDATE
      if(iItemNum != NULL)
      {
         *iItemNum = 0;
      }

      // debug("EDFItemList::find exit NULL, %d\n", iMid);
      return NULL;
   }

   iMax = m_iNumItems - 1;

   do
   {
      STACKTRACEUPDATE
      iMid = (iMin + iMax) / 2;
      // debug("EDFItemList::find mid %d (min %d / max %d), %p(%ld)", iMid, iMin, iMax, m_pItems[iMid], m_pItems[iMid]->GetID());
      if(ItemID(m_pItems[iMid]) == lID)
      {
         pReturn = m_pItems[iMid];
         // debug(", return");
      }
      else if(ItemID(m_pItems[iMid]) < lID)
      {
         iMin = iMid + 1;
         // debug(", min = %d", iMin);
      }
      else
      {
         iMax = iMid - 1;
         // debug(", max = %d", iMax);
      }
      // debug("\n");
   }
   while(pReturn == NULL && iMin <= iMax);

   STACKTRACEUPDATE

   if(iItemNum != NULL)
   {
      *iItemNum = iMid;
   }

   // debug("EDFItemList::find exit %p, %d\n", pReturn, iMid);
   return pReturn;
}

template <class T>T ItemList<T>::Item(int iItemNum)
{
   if(iItemNum < 0 || iItemNum >= m_iNumItems)
   {
      return NULL;
   }

   return m_pItems[iItemNum];
}

template <class T>int ItemList<T>::Count()
{
   return m_iNumItems;
}

/* template <class T>bool ItemList<T>::Debug(bool bDebug)
{
   bool bReturn = m_bDebug;

   m_bDebug = bDebug;
   // printf("ItemList::Debug %s\n", BoolStr(m_bDebug));

   return bReturn;
}

template <class T>bool ItemList<T>::Debug()
{
   return m_bDebug;
} */

template <class T> bool ItemList<T>::Add(T pItem, int *iItemNum)
{
   STACKTRACE
   int iAdd = -1;
   T pFind = NULL, *pItems = NULL;

   // if(m_bDebug == true)
   {
      debug(DEBUGLEVEL_DEBUG, "ItemList::Add entry %p, %d\n", pItem, m_iNumItems);
   }

   if(pItem == NULL)
   {
      if(iItemNum != NULL)
      {
         *iItemNum = -1;
      }

      // if(m_bDebug == true)
      {
         debug(DEBUGLEVEL_INFO, "ItemList::Add exit false, NULL item\n");
      }
      return false;
   }

   if(m_iNumItems == 0)
   {
      iAdd = 0;
   }
   else
   {
      pFind = Find(ItemID(pItem), &iAdd);
      if(pFind != NULL)
      {
         if(iItemNum != NULL)
         {
            *iItemNum = iAdd;
         }
         // if(m_bDebug == true)
         {
            debug(DEBUGLEVEL_DEBUG, "ItemList::Add exit false, %ld already added at %d\n", ItemID(pItem), iAdd);
         }
         return false;
      }
      else
      {
         if(ItemID(m_pItems[iAdd]) < ItemID(pItem))
         {
            // if(m_bDebug == true)
            {
               debug(DEBUGLEVEL_DEBUG, "ItemList::Add move up\n");
            }

            while(iAdd < m_iNumItems && ItemID(m_pItems[iAdd]) < ItemID(pItem))
            {
               iAdd++;
            }
         }
         else
         {
            // if(m_bDebug == true)
            {
               debug(DEBUGLEVEL_DEBUG, "ItemList::Add move down\n");
            }

            while(iAdd > 0 && ItemID(m_pItems[iAdd - 1]) > ItemID(pItem))
            {
               iAdd--;
            }
         }
      }
   }

   // if(m_bDebug == true)
   {
      debug(DEBUGLEVEL_DEBUG, "ItemList::Add point %d\n", iAdd);
   }

   if(iItemNum != NULL)
   {
      *iItemNum = iAdd;
   }

   if(iAdd == -1)
   {
      // if(m_bDebug == true)
      {
         debug("ItemList::Add exit false, no add point for %ld\n", ItemID(pItem));
      }
      return false;
   }

   // if(m_bDebug == true)
   {
      debug(DEBUGLEVEL_DEBUG, "ItemList::Add %p to %p (%d max %d)\n", pItem, m_pItems, m_iNumItems, m_iMaxItems);
   }

   ARRAY_INC(T, m_pItems, m_iNumItems, m_iMaxItems, ARRAY_STEP, pItem, iAdd, pItems)

   /* for(iAdd = 0; iAdd < m_iNumItems; iAdd++)
   {
      debug("ItemList::Add item %d, %ld\n", iAdd, ItemID(m_pItems[iAdd]));
   } */

   // if(m_bDebug == true)
   {
      debug(DEBUGLEVEL_DEBUG, "ItemList::Add exit true\n");
   }
   return true;
}

template <class T>bool ItemList<T>::ItemDelete(int iItemNum)
{
   STACKTRACE

   // printf("ItemList::ItemDelete %d\n", iItemNum);

   // if(m_bDebug == true)
   {
      debug(DEBUGLEVEL_DEBUG, "ItemList::ItemDelete entry %d\n", iItemNum);
   }

   // debug("ItemList::ItemDelete check 0\n");

   if(iItemNum < 0 || iItemNum >= m_iNumItems)
   {
      debug("ItemList::ItemDelete exit false, not found\n");
      return false;
   }

   // debug("ItemList::ItemDelete check 1\n");

   if(m_bDelete == true)
   {
      debug(DEBUGLEVEL_DEBUG, "ItemList::ItemDelete %d %p\n", iItemNum, m_pItems[iItemNum]);
      delete m_pItems[iItemNum];
   }

   // debug("ItemList::ItemDelete check 2\n");

   STACKTRACEUPDATE

   // debug("ItemList::ItemDelete check 3\n");

   debug(DEBUGLEVEL_DEBUG, "ItemList::ItemDelete array dec %d %p %d\n", iItemNum, m_pItems, iItemNum);

   ARRAY_DEC(T, m_pItems, m_iNumItems, iItemNum)

   // debug("ItemList::ItemDelete check 4\n");

   // if(m_bDebug == true)
   {
      debug(DEBUGLEVEL_DEBUG, "ItemList::ItemDelete exist true\n");
   }
   return true;
}

#endif
