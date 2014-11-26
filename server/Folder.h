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
** Folder.h: Declaration of server side folder functions
*/

#ifndef _FOLDER_H_
#define _FOLDER_H_

#include "MessageTreeItem.h"

#include "FolderMessageItem.h"

bool FolderLoad();
bool FolderUnload();

bool FolderAdd(MessageTreeItem *pItem);
bool FolderDelete(MessageTreeItem *pItem);
int FolderCount(bool bTree = false);
long FolderMaxID();

MessageTreeItem *FolderGet(int iFolderID, char **szName = NULL, bool bCopy = false);
MessageTreeItem *FolderGet(char *szName);
MessageTreeItem *FolderList(int iFolderNum, bool bTree = false);
bool FolderSetParent(MessageTreeItem *pItem, int iParentID);

bool FolderMessageAdd(FolderMessageItem *pItem);
bool FolderMessageDelete(FolderMessageItem *pItem);
bool FolderMessageDelete(int iMessageNum);
int FolderMessageCount();

FolderMessageItem *FolderMessageGet(int iMessageID);
FolderMessageItem *FolderMessageList(int iMessageNum);

#endif
