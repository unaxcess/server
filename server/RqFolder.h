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
** RqFolder.h: Server side folder request functions
*/

#ifndef _RQFOLDER_H_
#define _RQFOLDER_H_

#include "MessageItem.h"
#include "DBMessageRead.h"
#include "DBMessageSave.h"

// Internal functions
bool FolderReadDB(EDF *pData);
bool FolderWriteDB(bool bFull);
bool FolderSetup(EDF *pData, int iOptions, bool bValidate);

int FolderExpire(EDF *pData, MessageTreeItem *pFolder, FolderMessageItem *pMessage = NULL);

void FolderMaintenance(EDFConn *pConn, EDF *pData);

bool BulletinLoad();
bool BulletinUnload();
FolderMessageItem *BulletinGet(int iBulletinID);
FolderMessageItem *BulletinList(int iListNum);

int MessageMarking(UserItem *pUser, DBMessageRead *pReads, DBMessageRead *pCatchups, const char *szAnnounce, FolderMessageItem *pItem, int iOverride = 0, int *pMarkType = NULL);

void FolderMessageItemAnnounce(EDF *pAnnounce, int iOp, FolderMessageItem *pItem, int iOldVoteType);

bool FolderMessageItemEdit(EDF *pData, int iOp, FolderMessageItem *pItem, EDF *pIn, EDF *pOut, UserItem *pCurr, ConnData *pConnData, char *szRequest, int iBase, bool bRecurse, int iSubType, int *iMarked, int *iOldVoteType);

// Library functions
extern "C"
{
ICELIBFN bool MessageAdd(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut);
}

#endif
