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
** Talk.cpp: Implementation of server side talk functions
*/

#include <stdio.h>
#include <string.h>

#include "EDFItemList.h"

#include "Tree.h"
#include "Talk.h"

#include "../ua.h"

EDFItemList *g_pChannelTree = NULL, *g_pChannelList = NULL;
EDFItemList *g_pTalkMessageList = NULL;
long g_lTalkMaxID = 0;

bool TalkLoad()
{
   TalkUnload();

	g_pChannelTree = new EDFItemList(false);
	g_pChannelList = new EDFItemList(true);

	g_pTalkMessageList = new EDFItemList(true);

   return true;
}

bool TalkUnload()
{
   delete g_pChannelTree;
   delete g_pChannelList;

   delete g_pTalkMessageList;

   return true;
}

bool ChannelAdd(MessageTreeItem *pChannel)
{
   return TreeAdd(g_pChannelTree, g_pChannelList, pChannel, &g_lTalkMaxID);
}

bool ChannelDelete(MessageTreeItem *pChannel)
{
   return TreeDelete(g_pChannelTree, g_pChannelList, pChannel);
}

int ChannelCount(bool bTree)
{
   if(bTree == true)
   {
      return g_pChannelTree->Count();
   }

   return g_pChannelList->Count();
}

long TalkMaxID()
{
   return g_lTalkMaxID;
}

MessageTreeItem *ChannelGet(int iChannelID, char **szName, bool bCopy)
{
   return TreeGet(g_pChannelList, iChannelID, szName, bCopy);
}

MessageTreeItem *ChannelGet(char *szName)
{
   return TreeGet(g_pChannelList, szName);
}

MessageTreeItem *ChannelList(int iChannelNum, bool bTree)
{
   if(bTree == true)
   {
      return (MessageTreeItem *)g_pChannelTree->Item(iChannelNum);
   }

   return (MessageTreeItem *)g_pChannelList->Item(iChannelNum);
}

bool ChannelSetParent(MessageTreeItem *pChannel, int iParentID)
{
   return TreeSetParent(g_pChannelList, g_pChannelTree, pChannel, iParentID);
}

ChannelMessageItem *TalkMessageGet(int iMessageID)
{
   return (ChannelMessageItem *)g_pTalkMessageList->Find(iMessageID);
}

ChannelMessageItem *TalkMessageList(int iMessageNum)
{
   return (ChannelMessageItem *)g_pTalkMessageList->Item(iMessageNum);
}

bool TalkMessageAdd(ChannelMessageItem *pMessage)
{
   if(pMessage->GetID() > g_lTalkMaxID)
   {
      g_lTalkMaxID = pMessage->GetID();
   }

   return true;
}
