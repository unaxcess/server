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
** User.h: Definition of server side user functions
*/

#ifndef _USER_H_
#define _USER_H_

#include "UserItem.h"

bool UserLoad();
bool UserUnload();

bool UserAdd(UserItem *pItem);
bool UserDelete(UserItem *pItem);
int UserCount();
long UserMaxID();

UserItem *UserGet(int iUserID, char **szName = NULL, bool bCopy = false, long lDate = -1);
UserItem *UserGet(char *szName);
UserItem *UserList(int iListNum);

#endif
