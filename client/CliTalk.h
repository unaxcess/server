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
** CliTalk.h: Client side talk functions
*/

#ifndef _CLITALK_H_
#define _CLITALK_H_

int ChannelGet(EDF *pData, char *&szChannelName, bool bReset = false);
bool ChannelGet(EDF *pData, int iChannelID, char **szChannelName = NULL, bool bReset = false);

#endif
