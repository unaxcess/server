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
** CliFolder.h: Client side folder functions
*/

#ifndef _CLIFOLDER_H_
#define _CLIFOLDER_H_

// Folder functions
int FolderGet(EDF *pData, char *&szFolderName, bool bReset = false);
bool FolderGet(EDF *pData, int iFolderID, char **szFolderName = NULL, bool bReset = false);

// Message functions
bool MessageInFolder(EDF *pData, int iMsgID);

#endif
