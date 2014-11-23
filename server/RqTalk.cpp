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
** RqTalk.cpp: Implementation of server side talk request functions
*/

#include <stdio.h>

// Framework headers
#include "headers.h"
#include "ICE/ICE.h"

// Common headers
#include "MessageTreeItem.h"
#include "Talk.h"

// Library headers
#include "server.h"

#include "RqTree.h"
#include "RqTalk.h"

bool TalkReadDB(EDF *pData)
{
   return true;
}

bool TalkWriteDB(bool bFull)
{
   return true;
}

bool TalkSetup(EDF *pData, int iOptions, bool bValidate)
{
   return true;
}

extern "C"
{

ICELIBFN bool ChannelSend(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut)
{
   STACKTRACE
   int iChannelID = -1, iBase = RequestGroup(pIn);
   long lMessageID = -1;
   ConnData *pConnData = CONNDATA;
   ChannelMessageItem *pChannelMessage = NULL;
   MessageTreeItem *pChannel = NULL;
   UserItem *pCurr = CONNUSER;
   DBSub *pChannels = CONNCHANNELS;

   EDFParser::debugPrint("ChannelSend entry", pIn);

   if(pIn->GetChild("channelid", &iChannelID) == false)
   {
      pOut->Set("reply", MSG_CHANNEL_NOT_EXIST);
      return false;
   }

   debug("ChannelSend channel from ID %d\n", iChannelID);

   if(MessageTreeAccess(iBase, MTA_MESSAGE_WRITE, iChannelID, pCurr, pChannels, pOut, &pChannel, NULL, NULL) == false)
   {
      // Cannot access channel
      return false;
   }

   // Create message
   lMessageID = TalkMaxID();
   lMessageID++;

   debug("ChannelSend creating message %ld in %ld\n", lMessageID, pChannel->GetID());

   pChannelMessage = new ChannelMessageItem(lMessageID, pChannel);
   // debug("ChannelSend class %s\n", pChannelMessage->GetClass());

   TalkMessageAdd(pChannelMessage);

   if(mask(pChannel->GetAccessMode(), CHANNELMODE_LOGGED) == false)
   {
      pChannel->MessageAdd(pChannelMessage);
   }

   pChannelMessage->SetFromID(pCurr->GetID());

   // Set initial fields
   MessageItemEdit(pData, RQ_ADD, pChannelMessage, pIn, pOut, pCurr, pConnData, MSG_CHANNEL_SEND, iBase, false, pChannels->SubType(pChannel->GetID()));

   EDFParser::debugPrint("ChannelSend exit true", pOut);
   return true;
}

}
