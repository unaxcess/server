/*
** EDFItem library
** (c) 2000 Michael Wood (mike@compsoc.man.ac.uk)
**
** EDFItemList.h: Definition of EDFItemList class
**
** Wrapper around ItemList to provide EDFItem * returns
*/

#ifndef _EDFITEMLIST_H_
#define _EDFITEMLIST_H_

#include "EDFItem.h"

#include "ItemList.h"

class EDFItemList : public ItemList<class EDFItem *>
{
public:
   EDFItemList(bool bDelete);
   virtual ~EDFItemList();

   bool Add(EDFItem *pItem);
   EDFItem *Find(long lID);
   EDFItem *Item(int iItemNum);

   bool ItemDelete(int iItemNum);

protected:
   long ItemID(EDFItem *pItem);
};

#endif
