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
** RqUser.h: Server side user request functions
*/

#ifndef _RQUSER_H_
#define _RQUSER_H_

#include "UserItem.h"

// Internal functions
bool UserReadDB(EDF *pData);
bool UserWriteDB(bool bFull);
bool UserSetup(EDF *pData, int iOptions, bool bValidate, int iWriteTime);

void UserMaintenance(EDFConn *pConn, EDF *pData);

UserItem *UserAdd(char *szName, char *szPassword, int iAccessLevel, int iUserType = USERTYPE_NONE);
bool UserDelete(EDF *pData, UserItem *pCurr, UserItem *pDelete);
void UserLogout(EDF *pData, UserItem *pUser, ConnData *pConnData, bytes *pText);
bool UserEmailValid(char *szEmail);

bool UserAccess(UserItem *pCurr, EDF *pIn, char *szField, bool bField, UserItem **pItem, EDF *pOut);

#endif
