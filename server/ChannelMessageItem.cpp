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
** ChannelMessageItem.cpp: Implmentation of ChannelMessageItem class
*/

#include <stdio.h>
#include <string.h>
#include <time.h>
/* #ifdef WIN32
#include <typeinfo.h>
#else
#include <typeinfo>
#endif */

#include "ua.h"

#include "ChannelMessageItem.h"
#include "MessageTreeItem.h"

// #define MESSAGEWRITE_HIDDEN 1

// Constructor with no data
ChannelMessageItem::ChannelMessageItem(long lID, MessageTreeItem *pTree) : MessageItem(lID, NULL, pTree)
{
}

// Constructor with EDF data
ChannelMessageItem::ChannelMessageItem(EDF *pEDF, MessageTreeItem *pTree) : MessageItem(pEDF, NULL, pTree)
{
}

ChannelMessageItem::ChannelMessageItem(DBTable *pTable, MessageTreeItem *pTree) : MessageItem(pTable, NULL, pTree, 8)
{
}

// Destructor
ChannelMessageItem::~ChannelMessageItem()
{
}

/* const char *ChannelMessageItem::GetClass()
{
   return typeid(this).name();
} */

bool ChannelMessageItem::WriteFields(DBTable *pTable, EDF *pEDF)
{
   STACKTRACE

   MessageItem::WriteFields(pTable, pEDF);

   return true;
}

char *ChannelMessageItem::TableName()
{
   return CMI_TABLENAME;
}

char *ChannelMessageItem::TreeName()
{
   return "channel";
}
