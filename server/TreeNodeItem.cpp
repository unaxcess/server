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
** TreeNodeItem.cpp: Implmentation of TreeNodeItem class
*/

#ifndef _TREENODEITEM_CPP_
#define _TREENODEITEM_CPP_

template <class T> TreeNodeItem<T>::TreeNodeItem(long lID, T pParent) : EDFItem(lID)
{
   init(pParent);
}

template <class T> TreeNodeItem<T>::TreeNodeItem(EDF *pEDF, T pParent) : EDFItem(pEDF)
{
   init(pParent);
}

template <class T> TreeNodeItem<T>::TreeNodeItem(DBTable *pTable, T pParent, int iEDFField) : EDFItem(pTable, iEDFField)
{
   init(pParent);

   if(pParent == NULL)
   {
      pTable->GetField(1, &m_lParentID);
   }
}

template <class T>TreeNodeItem<T>::~TreeNodeItem()
{
   // printf("TreeNodeItem::~TreeNodeItem entry %p, %ld %p\n", this, GetID(), m_pChildren);

   // printf("TreeNodeItem::~TreeNodeItem %p(%ld), %p(%d)\n", this, GetID(), m_pChildren, m_pChildren != NULL ? m_pChildren->Count() : -1);

   delete m_pChildren;

   // printf("TreeNodeItem::~TreeNodeItem exit, %p\n", this);
}

template <class T> T TreeNodeItem<T>::GetParent()
{
   return m_pParent;
}

template <class T> bool TreeNodeItem<T>::SetParent(T pParent)
{
   m_pParent = pParent;
   if(m_pParent != NULL)
   {
      m_lParentID = m_pParent->GetID();
   }
   else
   {
      m_lParentID = -1;
   }

   SetChanged(true);

   return true;
}

template <class T> long TreeNodeItem<T>::GetParentID()
{
   if(m_pParent != NULL)
   {
      return m_pParent->GetID();
   }

   return m_lParentID;
}

template <class T> bool TreeNodeItem<T>::SetParentID(long lParentID)
{
   if(m_pParent != NULL)
   {
      printf("TreeNodeItem::SetParentID exit false, parent %p %ld\n", m_pParent, m_pParent->GetID());
      return false;
   }

   m_lParentID = lParentID;

   SetChanged(true);

   return true;
}

template <class T> bool TreeNodeItem<T>::Add(T pItem)
{
   STACKTRACE

   if(find(pItem->GetID(), true, NULL) != NULL)
   {
      return false;
   }

   if(m_pChildren == NULL)
   {
      m_pChildren = new EDFItemList(false);
   }

   return m_pChildren->Add(pItem);
}

template <class T> bool TreeNodeItem<T>::ItemDelete(int iItemNum)
{
   return m_pChildren->ItemDelete(iItemNum);
}

template <class T> bool TreeNodeItem<T>::Delete(long lID)
{
   int iChildNum = 0;
   T pItem = NULL;

   if(m_pChildren->Delete(lID) == true)
   {
      return true;
   }

   while(iChildNum < m_pChildren->Count())
   {
      pItem = (T)m_pChildren->Item(iChildNum);
      if(pItem->Delete(lID) == true)
      {
         return true;
      }
      else
      {
         iChildNum++;
      }
   }

   return false;
}

template <class T> int TreeNodeItem<T>::Count(bool bRecurse)
{
   // STACKTRACE
   int iReturn = 0, iChildNum = 0;
   T pChild = NULL;

   if(m_pChildren == NULL)
   {
      return 0;
   }

   iReturn = m_pChildren->Count();

   if(bRecurse == true)
   {
      for(iChildNum = 0; iChildNum < m_pChildren->Count(); iChildNum++)
      {
         pChild = (T)m_pChildren->Item(iChildNum);
         iReturn += pChild->Count(bRecurse);
      }
   }

   return iReturn;
}

/* template <class T> int TreeNodeItem<T>::Pos(long lID, bool bRecurse)
{
   int iReturn = 0;
   T pItem = NULL;

   // printf("TreeNodeItem::Pos entry %ld %s\n", lID, BoolStr(bRecurse));

   pItem = find(lID, bRecurse, &iReturn);

   // printf("TreeNodeItem::Pos item %p (%d)\n", pItem, iReturn);

   if(pItem != NULL)
   {
      return iReturn;
   }

   return 0;
} */

template <class T> T TreeNodeItem<T>::Find(long lID, bool bRecurse)
{
   return find(lID, bRecurse, NULL);
}

template <class T> T TreeNodeItem<T>::Child(int iChildNum)
{
   STACKTRACE
   if(m_pChildren == NULL)
   {
      return NULL;
   }

   return (T)m_pChildren->Item(iChildNum);
}

template <class T> bool TreeNodeItem<T>::Delete(char *szTable)
{
   return EDFItem::Delete(szTable);
}

template <class T> bool TreeNodeItem<T>::WriteFields(DBTable *pTable, EDF *pEDF)
{
   STACKTRACE

   pTable->SetField(GetParentID());

   return true;
}

template <class T> bool TreeNodeItem<T>::ReadFields(DBTable *pTable)
{
   STACKTRACE

   // parentid
   pTable->BindColumnInt();

   return true;
}

template <class T> T TreeNodeItem<T>::find(long lID, bool bRecurse, int *iPos)
{
   // STACKTRACE
   int iChildNum = 0;
   T pChild = NULL, pFind = NULL;

   if(iPos != NULL)
   {
      (*iPos)++;
      // printf("TreeNodeItem::find %ld %s %d, %ld\n", lID, BoolStr(bRecurse), *iPos, GetID());
   }

   // printf("TreeNodeItem::find entry %ld %s %p, %ld\n", lID, BoolStr(bRecurse), iPos, GetID());
   if(lID == GetID())
   {
      // printf("TreeNodeItem::find exit %p, this\n", this);
      return (T)this;
   }

   if(bRecurse == false || m_pChildren == NULL)
   {
      // printf("TreeNodeItem::find exit NULL, no recurse (children %p)\n", m_pChildren);
      return NULL;
   }

   while(iChildNum < m_pChildren->Count())
   {
      pChild = (T)m_pChildren->Item(iChildNum);
      pFind = pChild->find(lID, true, iPos);
      if(pFind != NULL)
      {
         // printf("TreeNodeItem::find exit %p, child %d\n", pFind, iChildNum);
         return pFind;
      }
      else
      {
         iChildNum++;
      }
   }

   // printf("TreeNodeItem::find exit NULL\n");
   return NULL;
}

template <class T> void TreeNodeItem<T>::init(T pParent)
{
   m_pParent = pParent;
   m_lParentID = -1;

   m_pChildren = NULL;
}

#endif
