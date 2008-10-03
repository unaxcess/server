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
** User.cpp: Implementation of server side user functions
*/

#include <stdio.h>
#include <string.h>

#include "EDFItemList.h"

#include "User.h"

EDFItemList *g_pUsers = NULL;
long g_lMaxUserID = 0;

bool UserLoad()
{
   STACKTRACE

   UserUnload();

   g_pUsers = new EDFItemList(true);

   return true;
}

bool UserUnload()
{
   STACKTRACE

   delete g_pUsers;
   g_pUsers = NULL;

   return true;
}

bool UserAdd(UserItem *pItem)
{
   STACKTRACE
   bool bReturn = false;

   // printf("UserAdd %p to %p\n", pItem, g_pItems);

   if(pItem->GetID() > g_lMaxUserID)
   {
      // printf("UserAdd new max ID %ld\n", pItem->GetID());
      g_lMaxUserID = pItem->GetID();
   }

   bReturn = g_pUsers->Add(pItem);
   // printf("UserAdd exit %s\n", BoolStr(bReturn));
   return bReturn;
}

bool UserDelete(UserItem *pItem)
{
   STACKTRACE

   // printf("UserDelete %ld\n", pItem->GetID());
   // return g_pItems->Remove(pItem->GetID());
   return g_pUsers->Delete(pItem->GetID());
}

int UserCount()
{
   STACKTRACE

   /* if(g_pUsers == NULL)
   {
      return 0;
   } */

   return g_pUsers->Count();
}

long UserMaxID()
{
   return g_lMaxUserID;
}

UserItem *UserGet(int iUserID, char **szName, bool bCopy, long lDate)
{
   STACKTRACE
   UserItem *pReturn = NULL;

   /* if(g_pUsers == NULL)
   {
      return 0;
   } */

   pReturn = (UserItem *)g_pUsers->Find(iUserID);
   if(pReturn != NULL && szName != NULL)
   {
      (*szName) = pReturn->GetName(bCopy, lDate);
   }

   return pReturn;
}

UserItem *UserGet(char *szName)
{
   STACKTRACE
   int iUserNum = 0;
   UserItem *pReturn = NULL;

   // if(szName == NULL || g_pUsers == NULL)
   if(szName == NULL)
   {
      return NULL;
   }

   // printf("UserGet entry %s, %d\n", szName, g_pUsers->Count());

   while(iUserNum < g_pUsers->Count())
   {
      pReturn = (UserItem *)g_pUsers->Item(iUserNum);
      // printf("UserGet check %s\n", pUser->GetName());

      if(stricmp(szName, pReturn->GetName()) == 0)
      {
         // printf("UserGet exit %p\n", pReturn);
         return pReturn;
      }
      else
      {
         iUserNum++;
      }
   }

   // printf("UserGet exit NULL\n");
   return NULL;
}

UserItem *UserList(int iListNum)
{
   STACKTRACE
   /* if(g_pUsers == NULL)
   {
      return NULL;
   } */

   return (UserItem *)g_pUsers->Item(iListNum);
}
