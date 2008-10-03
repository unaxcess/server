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
** RqTree.h: Server side message tree request functions
*/

#ifndef _RQTREE_H_
#define _RQTREE_H_

#include "EDF/EDF.h"

#include "MessageTreeItem.h"
#include "FolderMessageItem.h"
#include "UserItem.h"

#include "DBSub.h"

// MessageTreeAccess op values
#define MTA_MESSAGETREE_READ 1
#define MTA_MESSAGETREE_EDIT 3
#define MTA_MESSAGE_READ 5
#define MTA_MESSAGE_WRITE 7
#define MTA_MESSAGE_EDIT 6

// Internal functions
char *MessageTreeSubTable(int iBase);

int MessageTreeCount(int iBase, bool bTree = false);
MessageTreeItem *MessageTreeGet(int iBase, int iID, char **szName = NULL, bool bCopy = false);
MessageTreeItem *MessageTreeList(int iBase, int iListNum, bool bTree = false);

int MessageTreeAccess(MessageTreeItem *pItem, int iSubMode, int iMemMode, int iAccessLevel, int iSubType, bool bSubCheck = false);
int MessageAccess(MessageItem *pMessage, int iUserID, int iAccessLevel, int iSubType, bool bPrivateCheck = false, bool bDeletedCheck = true);

char *MessageTreeStr(int iBase, char *szStr);
char *MessageTreeStr(int iBase, int iOp);
char *MessageTreeID(int iBase);
char *MessageTreeName(int iBase);
char *MessageTreeNum(int iBase);

char *MessageStr(int iBase, char *szStr);
char *MessageID(int iBase);

bool MessageTreeAccess(int iBase, int iOp, int iID, UserItem *pUser, DBSub *pSubs, EDF *pOut, MessageTreeItem **pMesageTreeOut, MessageItem **pMessageOut, int *iSubType);
bool MessageTreeAccess(int iBase, int iOp, int iID, int iTreeID, UserItem *pUser, DBSub *pSubs, EDF *pOut, MessageTreeItem **pMesageTreeOut, MessageItem **pMessageOut, int *iSubType);
bool MessageTreeAccess(int iBase, int iOp, int iID, int iTreeID, bool *bArchive, UserItem *pUser, DBSub *pSubs, EDF *pOut, MessageTreeItem **pMesageTreeOut, MessageItem **pMessageOut, int *iSubType);
bool MessageTreeAccess(int iBase, int iOp, MessageTreeItem *pMessageTree, UserItem *pUser, DBSub *pSubs, EDF *pOut, int *iSubType);

MessageItem *MessageGet(int iBase, int iID);

bool MessageItemEdit(EDF *pData, int iOp, MessageItem *pItem, EDF *pIn, EDF *pOut, UserItem *pCurr, ConnData *pConnData, char *szRequest, int iBase, bool bRecurse, int iSubType);

int MessageTreeSubscribeAnnounce(EDF *pData, char *szRequest, int iBase, MessageTreeItem *pTree, UserItem *pSubscribe, int iSubType, int iExtra, int iCurrType, UserItem *pCurr);

#endif
