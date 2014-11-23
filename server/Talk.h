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
** Talk.h: Declaration of server side talk functions
*/

#ifndef _TALK_H_
#define _TALK_H_

#include "MessageTreeItem.h"

#include "ChannelMessageItem.h"

bool TalkLoad();
bool TalkUnload();

bool ChannelAdd(MessageTreeItem *pChannel);
bool ChannelDelete(MessageTreeItem *pChannel);
int ChannelCount(bool bTree = false);
long TalkMaxID();

MessageTreeItem *ChannelGet(int iChannelID, char **szName = NULL, bool bCopy = false);
MessageTreeItem *ChannelGet(char *szName);
MessageTreeItem *ChannelList(int iChannelNum, bool bTree = false);
bool ChannelSetParent(MessageTreeItem *pChannel, int iParentID);

bool TalkMessageAdd(ChannelMessageItem *pMessage);

#endif
