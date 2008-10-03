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
** Folder.cpp: Implementation of server side folder functions
*/

#include <stdio.h>
#include <string.h>

#include "EDFItemList.h"

#include "Tree.h"
#include "Folder.h"

#include "../ua.h"

EDFItemList *g_pFolderTree = NULL, *g_pFolderList = NULL;
EDFItemList *g_pFolderMessageList = NULL;
long g_lFolderMaxID = 0;

bool FolderLoad()
{
   FolderUnload();

	g_pFolderTree = new EDFItemList(false);
	g_pFolderList = new EDFItemList(true);

	g_pFolderMessageList = new EDFItemList(true);

   return true;
}

bool FolderUnload()
{
   delete g_pFolderTree;
   delete g_pFolderList;

   delete g_pFolderMessageList;

   return true;
}

bool FolderAdd(MessageTreeItem *pItem)
{
   return TreeAdd(g_pFolderTree, g_pFolderList, pItem, &g_lFolderMaxID);
}

bool FolderDelete(MessageTreeItem *pItem)
{
   return TreeDelete(g_pFolderTree, g_pFolderList, pItem);
}

int FolderCount(bool bTree)
{
   if(bTree == true)
   {
      return g_pFolderTree->Count();
   }

   return g_pFolderList->Count();
}

long FolderMaxID()
{
   return g_lFolderMaxID;
}

bool FolderMessageAdd(FolderMessageItem *pItem)
{
   if(pItem->GetID() > g_lFolderMaxID)
   {
      g_lFolderMaxID = pItem->GetID();
   }

   return g_pFolderMessageList->Add(pItem);
}

bool FolderMessageDelete(FolderMessageItem *pItem)
{
   return g_pFolderMessageList->Delete(pItem->GetID());
}

bool FolderMessageDelete(int iMessageNum)
{
   STACKTRACE
   return g_pFolderMessageList->ItemDelete(iMessageNum);
}

int FolderMessageCount()
{
   return g_pFolderMessageList->Count();
}

MessageTreeItem *FolderGet(int iFolderID, char **szName, bool bCopy)
{
   return TreeGet(g_pFolderList, iFolderID, szName, bCopy);
}

MessageTreeItem *FolderGet(char *szName)
{
   return TreeGet(g_pFolderList, szName);
}

MessageTreeItem *FolderList(int iFolderNum, bool bTree)
{
   if(bTree == true)
   {
      return (MessageTreeItem *)g_pFolderTree->Item(iFolderNum);
   }

   return (MessageTreeItem *)g_pFolderList->Item(iFolderNum);
}

bool FolderSetParent(MessageTreeItem *pItem, int iParentID)
{
   return TreeSetParent(g_pFolderList, g_pFolderTree, pItem, iParentID);
}

FolderMessageItem *FolderMessageGet(int iMessageID)
{
   return (FolderMessageItem *)g_pFolderMessageList->Find(iMessageID);
}

FolderMessageItem *FolderMessageList(int iMessageNum)
{
   return (FolderMessageItem *)g_pFolderMessageList->Item(iMessageNum);
}
