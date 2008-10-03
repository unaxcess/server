/*
** EDFItem library
** (c) 2000 Michael Wood (mike@compsoc.man.ac.uk)
**
** ChannelMessageItem.h: Definition of ChannelMessageItem class
**
** Definition of ChannelMessageItem class (derived from MessageItem)
*/

#ifndef _CHANNELMESSAGEITEM_H_
#define _CHANNELMESSAGEITEM_H_

#include "MessageItem.h"

#define CMI_TABLENAME "channel_message_item"

class ChannelMessageItem : public MessageItem
{
public:
	ChannelMessageItem(long lID, MessageTreeItem *pTree);
	ChannelMessageItem(EDF *pEDF, MessageTreeItem *pTree);
	ChannelMessageItem(DBTable *pTable, MessageTreeItem *pTree);
	~ChannelMessageItem();

   // const char *GetClass();

protected:
   bool WriteFields(DBTable *pTable, EDF *pEDF);

   char *TableName();
   char *TreeName();
};

#endif
