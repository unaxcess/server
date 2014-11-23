/*
** EDFItem library
** (c) 2000 Michael Wood (mike@compsoc.man.ac.uk)
**
** EDFItemList.cpp: Implementation of EDFItemList class
*/

#include <stdio.h>
#include <string.h>
#include "EDFItemList.h"

EDFItemList::EDFItemList(bool bDelete) : ItemList<EDFItem *>(bDelete)
{
   STACKTRACE
}

EDFItemList::~EDFItemList()
{
   STACKTRACE
   // printf("EDFItemList::~EDFItemList %p\n", this);
}

bool EDFItemList::Add(EDFItem *pItem)
{
   return ItemList<EDFItem *>::Add(pItem);
}

EDFItem *EDFItemList::Find(long lID)
{
   return ItemList<EDFItem *>::Find(lID);
}

EDFItem *EDFItemList::Item(int iItemNum)
{
   return ItemList<EDFItem *>::Item(iItemNum);
}

bool EDFItemList::ItemDelete(int iItemNum)
{
   // printf("EDFItemList::ItemDelete %d\n", iItemNum);
   return ItemList<EDFItem *>::ItemDelete(iItemNum);
}

long EDFItemList::ItemID(EDFItem *pItem)
{
   return pItem->GetID();
}
