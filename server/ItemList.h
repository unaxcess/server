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
** ItemList.h: Definition of ItemList class
**
** Stores items in a sorted list (to allow binary searches)
*/

#ifndef _ITEMLIST_H_
#define _ITEMLIST_H_

template <class T> class ItemList
{
public:
   ItemList(bool bDelete);
   virtual ~ItemList();

   bool Delete(long lID);

   int Count();

protected:
   bool m_bDelete;

   bool Add(T pItem, int *iItemNum = NULL);

   T Find(long lID, int *iItemNum = NULL);
   T Item(int iItemNum);

   bool ItemDelete(int iItemNum);

   virtual long ItemID(T pItem) = 0;

private:
   T *m_pItems;
   int m_iNumItems;
   int m_iMaxItems;
};

#include "ItemList.cpp"

#endif
