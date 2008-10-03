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
** Tree.h: Declaration of server side tree functions
*/

#ifndef _TREE_H_
#define _TREE_H_

#include "EDFItem.h"
#include "EDFItemList.h"
#include "MessageTreeItem.h"

enum TreeAccessOp { OpTreeEdit, OpTreeVisible, OpTreeRead, OpTreeSubscribe, OpMessageAdd, OpMessageDelete, OpMessageEdit, OpMessageRead, OpMessageMove };

bool TreeAdd(EDFItemList *pTree, EDFItemList *pList, MessageTreeItem *pItem, long *lMaxID);
bool TreeDelete(EDFItemList *pTree, EDFItemList *pList, MessageTreeItem *pItem);

MessageTreeItem *TreeGet(EDFItemList *pList, int iTreeID, char **szName = NULL, bool bCopy = false);
MessageTreeItem *TreeGet(EDFItemList *pList, char *szName);
MessageTreeItem *TreeList(EDFItemList *pList, int iItemNum);
bool TreeSetParent(EDFItemList *pList, EDFItemList *pTree, MessageTreeItem *pItem, int iParentID);

#endif
