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
** RqSystem.h: Server side system request functions
*/

#ifndef _RQSYSTEM_H_
#define _RQSYSTEM_H_

#define SERVICE_SUB "service_sub"

long ContactID();
void ContactID(long lContactID);

// Internal functions
long TaskNext(int iHour, int iMinute, int iDay, int iRepeat);

bool AgentLogout(EDF *pData, UserItem *pUser);

int SystemExpire(EDF *pData);

// Library functions
extern "C"
{
ICELIBFN bool SystemEdit(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut);
ICELIBFN bool SystemMaintenance(EDFConn *pConn, EDF *pData, EDF *pIn, EDF *pOut);
}

#endif
