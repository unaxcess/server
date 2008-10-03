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
** MessageTree.cpp: Implementation of server side folder / channel functions
*/

#include <stdio.h>
#include <string.h>

#include "EDFItemList.h"

#include "Tree.h"

#include "../ua.h"

bool TreeAdd(EDFItemList *pTree, EDFItemList *pList, MessageTreeItem *pItem, long *lMaxID)
{
   MessageTreeItem *pParent = NULL;

   if(pList->Add(pItem) == false)
   {
      return false;
   }

   if(pItem->GetID() > *lMaxID)
   {
      *lMaxID = pItem->GetID();
   }

   pParent = pItem->GetParent();

   debug("TreeAdd adding %ld(%s) to %p", pItem->GetID(), pItem->GetName(), pParent);
   if(pParent != NULL)
   {
      debug(", %ld(%s)", pParent->GetID(), pParent->GetName());
   }
   debug("\n");

   if(pParent == NULL)
   {
      pTree->Add(pItem);
   }
   else
   {
      pParent->Add(pItem);
   }

   return true;
}

bool TreeDelete(EDFItemList *pTree, EDFItemList *pList, MessageTreeItem *pItem)
{
   pTree->Delete(pItem->GetID());

   return pList->Delete(pItem->GetID());
}

/* bool TreeItemAdd(EDFItemList *pList, MessageTreeItem *pItem, long *lMaxID)
{
   if(pList->Add(pItem) == false)
   {
      return false;
   }

   if(pItem->GetID() > *lMaxID)
   {
      *lMaxID = pItem->GetID();
   }

   return true;
}

bool TreeItemDelete(EDFItemList *pList, MessageTreeItem *pItem)
{
   return pList->Delete(pItem->GetID());
}

bool TreeItemDelete(EDFItemList *pList, int iItemNum)
{
   return pList->ItemDelete(iItemNum);
} */

MessageTreeItem *TreeGet(EDFItemList *pList, int iTreeID, char **szName, bool bCopy)
{
   MessageTreeItem *pReturn = NULL;

   pReturn = (MessageTreeItem *)pList->Find(iTreeID);
   // printf("TreeGet %ld, %p (%s)\n", iTreeID, pReturn, pReturn != NULL ? pReturn->GetName(false) : "");
   if(pReturn != NULL && szName != NULL)
   {
      (*szName) = pReturn->GetName(bCopy);
   }

   return pReturn;
}

MessageTreeItem *TreeGet(EDFItemList *pList, char *szName)
{
   int iTreeNum = 0;
   MessageTreeItem *pItem = NULL;

   if(szName == NULL)
   {
      return NULL;
   }

   // printf("TreeGet entry %s, %d\n", szName, pList->Count());

   while(iTreeNum < pList->Count())
   {
      pItem = (MessageTreeItem *)pList->Item(iTreeNum);
      // printf("TreeGet check %s\n", pItem->GetName());

      if(stricmp(szName, pItem->GetName()) == 0)
      {
         // printf("TreeGet exit %p\n", pItem);
         return pItem;
      }
      else
      {
         iTreeNum++;
      }
   }

   // printf("TreeGet exit NULL\n");
   return NULL;
}

MessageTreeItem *TreeList(EDFItemList *pList, int iItemNum)
{
   return (MessageTreeItem *)pList->Item(iItemNum);
}

// TreeSetParent: Change parent
bool TreeSetParent(EDFItemList *pList, EDFItemList *pTree, MessageTreeItem *pItem, int iParentID)
{
   MessageTreeItem *pParent = pItem->GetParent(), *pNew = NULL;

   debug("TreeSetParent %p %p %p %d, %ld %p\n", pList, pTree, pItem, iParentID, pItem->GetParentID(), pItem->GetParent());

   if(iParentID == -1)
   {
      if(pParent != NULL)
      {
         pParent->Delete(pItem->GetID());
         pItem->SetParent(NULL);

         // Add to top level of tree
         pTree->Add(pItem);

         return true;
      }
   }
   else
   {
      pNew = TreeGet(pList, iParentID);
      if(pNew != NULL && pNew != pParent)
      {
         if(pParent != NULL)
         {
            // Remove from current parent
            debug("TreeSetParent remove %ld(%s) from parent %ld(%s)\n", pItem->GetID(), pItem->GetName(), pParent->GetID(), pParent->GetName());
            pParent->Delete(pItem->GetID());
         }
         else
         {
            // Remove from top level of tree
            debug("TreeSetParent remove %ld(%s) from top\n", pItem->GetID(), pItem->GetName());
            pTree->Delete(pItem->GetID());
         }

         debug("TreeSetParent add to %ld(%s)\n", pNew->GetID(), pNew->GetName());
         pItem->SetParent(pNew);
         pNew->Add(pItem);

         return true;
      }
   }

   return false;
}
