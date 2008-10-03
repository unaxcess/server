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
** CliUser.h: Client side user functions
*/

#ifndef _CLIUSER_H_
#define _CLIUSER_H_

int UserGet(EDF *pData, char *&szUserName, bool bReset = false);
bool UserGet(EDF *pData, int iUserID, char **szUserName = NULL, bool bReset = false, long lDate = -1);

#endif
