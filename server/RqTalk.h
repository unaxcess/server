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
** RqTalk.h: Server side talk request functions
*/

#ifndef _RQTALK_H_
#define _RQTALK_H_

#include "EDF/EDF.h"

// Internal functions
bool TalkReadDB(EDF *pData);
bool TalkWriteDB(bool bFull);
bool TalkSetup(EDF *pData, int iOptions, bool bValidate);

#endif
