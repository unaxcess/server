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
** server.cpp: Implementation of main server side functions
*/

// Standard headers
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#include <errno.h>
#ifdef UNIX
#include <sys/time.h>
#include <unistd.h>
#include <regex.h>
#include <stdarg.h>
#include <sys/timeb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#else
#include "regex.h"
#endif

#include "headers.h"
#include "ICE/ICE.h"

// Function headers
#include "server.h"

#include "Folder.h"
#include "Talk.h"

#include "RqTree.h"
#include "RqFolder.h"
#include "RqTalk.h"
#include "RqSystem.h"
#include "RqUser.h"

#include "build.h"

#define MAXPOINT 60
#define NOSCOPE 0
#define USEROFF 1
#define USERON 2
#define USERNORMAL 3
#define USERADMIN 4

int m_iWriteTime = 0, m_iTaskTime = 0;

#ifdef WIN32
char *crypt(char *szSource, char *szKey)
{
   return strmk(szSource);
}
#endif

bool stristr(bytes *pValue1, bytes *pValue2)
{
   int iValuePos = 0, iValueLen = 0;
   bool bReturn = false;
   char *szValue1 = NULL, *szValue2 = NULL;

   if(pValue1 == NULL || pValue2 == NULL)
   {
      return false;
   }

   szValue1 = (char *)pValue1->Data(true);
   szValue2 = (char *)pValue2->Data(true);

   if(szValue1 == NULL || szValue2 == NULL)
   {
      // printf("stristr false, %p %p\n", szValue1, szValue2);

      delete[] szValue1;
      delete[] szValue2;

      return false;
   }

   iValueLen = strlen(szValue1);
   for(iValuePos = 0; iValuePos < iValueLen; iValuePos++)
   {
      szValue1[iValuePos] = tolower(szValue1[iValuePos]);
   }

   iValueLen = strlen(szValue2);
   for(iValuePos = 0; iValuePos < iValueLen; iValuePos++)
   {
      szValue2[iValuePos] = tolower(szValue2[iValuePos]);
   }

   if(strstr(szValue1, szValue2) != NULL)
   {
      bReturn = true;
   }

   // printf("stristr %s, '%s' '%s'\n", BoolStr(bReturn), szValue1, szValue2);

   delete[] szValue1;
   delete[] szValue2;

   return bReturn;
}

// strmatch: Matches a string with a given pattern
bool strmatch(char *szString, char *szPattern, bool bLazy, bool bFull, bool bIgnoreCase)
{
   int iCompiled = 0, iMatchPos = 0, iPatternPos = 0, iOptions = REG_NOSUB;
   bool bMatch = false;
   char *szMatch = NULL;

   debug(DEBUGLEVEL_DEBUG, "strmatch '%s' '%s' %s %s %s", szString, szPattern, BoolStr(bLazy), BoolStr(bFull), BoolStr(bIgnoreCase));

   if(szString == NULL || szPattern == NULL)
   {
      debug(DEBUGLEVEL_DEBUG, ", false (NULL string)\n");
      return false;
   }

   // Convert the pattern string from lazy format to the regex format
   if(bLazy == true || bFull == true)
   {
      szMatch = new char[3 * strlen(szPattern) + 3];
      iMatchPos = 0;
      szMatch[iMatchPos++] = '^';
      for(iPatternPos = 0; iPatternPos < strlen(szPattern); iPatternPos++)
      {
         if(bLazy == true && szPattern[iPatternPos] == '?')
         {
               szMatch[iMatchPos++] = '.';
         }
         else if(bLazy == true && szPattern[iPatternPos] == '.')
         {
               szMatch[iMatchPos++] = '\\';
               szMatch[iMatchPos++] = '.';
         }
         else if(bLazy == true && szPattern[iPatternPos] == '*')
         {
               szMatch[iMatchPos++] = '.';
               szMatch[iMatchPos++] = '*';
         }
         else
         {
               szMatch[iMatchPos++] = szPattern[iPatternPos];
         }
      }
      szMatch[iMatchPos++] = '$';
      szMatch[iMatchPos++] = '\0';

      debug(DEBUGLEVEL_DEBUG, " -> %s", szMatch);
   }
   else
   {
      szMatch = szPattern;
   }

   if(bLazy == false)
   {
      iOptions += REG_EXTENDED;
   }

   // Regex match the string and pattern
   regex_t *pCompiled = (regex_t *)malloc(sizeof(regex_t));
   if(bIgnoreCase == true)
   {
      iOptions |= REG_ICASE;
   }
   iCompiled = regcomp(pCompiled, szMatch, iOptions);
   debug(DEBUGLEVEL_DEBUG, ", regcomp %d", iCompiled);
   if(iCompiled != 0)
   {
      debug(DEBUGLEVEL_DEBUG, ", false (bad pattern %d)\n", iCompiled);
      return false;
   }
   if(regexec(pCompiled, szString, 1, NULL, 0) == 0)
   {
      bMatch = true;
   }
   regfree(pCompiled);
   free(pCompiled);

   if(szMatch != szPattern)
   {
      delete[] szMatch;
   }

   debug(DEBUGLEVEL_DEBUG, ", %s\n", BoolStr(bMatch));
   return bMatch;
}

bool EDFSectionFull(EDF *pEDF, EDF *pIn, bool bSection, char *szSection, char *szSectionValue, int iOp, char *szName, va_list vList)
{
   STACKTRACE
   int iArgValue = 0, iArgType = -1;
   bool bChild = false, bArg = true, bLoop = false, bSet = false;
   char *szOp = NULL, *szArgName = NULL, *szEDFName = NULL;
   bytes *pValue = NULL;
   EDFElement *pElement = NULL;
   EDFSECTIONBYTESFN pBytesFunc = NULL;
   EDFSECTIONINTFN pIntFunc = NULL;

   debug("EDFSectionFull entry %p %p %s %s %s %d %s ...\n", pEDF, pIn, BoolStr(bSection), szSection, szSectionValue, iOp, szName);
   EDFParser::debugPrint(pEDF, false, true);

   if(bSection == true)
   {
      if(iOp == RQ_ADD)
      {
         szOp = "add";
      }
      else if(iOp == RQ_DELETE)
      {
         szOp = "delete";
      }
      else if(iOp == RQ_EDIT)
      {
         szOp = "edit";
      }
      else
      {
         debug("EDFSectionFull exit false, invalid op\n");
         EDFParser::debugPrint(pEDF, false, true);
         return false;
      }

      STACKTRACEUPDATE

      if(szSection != NULL)
      {
         STACKTRACEUPDATE

         if(pIn->Child(szSection, szOp) == false)
         {
            debug("EDFSectionFull exit false, no section %s / %s\n", szSection, szOp);
            EDFParser::debugPrint(pEDF, false, true);
            return false;
         }

         debug("EDFSectionFull moving to %s / %s section\n", szSection, szOp);

         if(pIn->Children() == 0)
         {
            if(iOp == RQ_ADD)
            {
               if(pEDF->Child(szSection) == false)
               {
                  pEDF->Add(szSection);
               }
               pEDF->Parent();

               debug("EDFSectionFull exit true, add section %s\n", szSection);
               EDFParser::debugPrint(pEDF, false, true);
               return true;
            }
            else if(iOp == RQ_DELETE)
            {
               pEDF->DeleteChild(szSection);
               debug("EDFSectionFull exit true, delete section %s\n", szSection);
               EDFParser::debugPrint(pEDF, false, true);
               return true;
            }

            debug("EDFSectionFull exit false, no change\n");
            EDFParser::debugPrint(pEDF, false, true);
            return false;
         }

         if(szSectionValue != NULL)
         {
            bChild = pEDF->Child(szSection, szSectionValue);
         }
         else
         {
            bChild = pEDF->Child(szSection);
         }

         if(bChild == false)
         {
            debug("EDFSectionFull adding section\n");
            pEDF->Add(szSection, szSectionValue);
         }
      }
      else
      {
         STACKTRACEUPDATE

         if(pIn->Child(szOp) == false)
         {
            debug("EDFSectionFull exit false, no section %s\n", szOp);
            EDFParser::debugPrint(pEDF, false, true);
            return false;
         }
      }
   }

   STACKTRACEUPDATE

   // va_start(vList, szName);
   // szArgName = va_arg(vList, char *);
   szArgName = szName;
   while(bArg == true)
   {
      STACKTRACEUPDATE

      if(szArgName != NULL)
      {
         iArgValue = va_arg(vList, int);
         iArgType = iArgValue;
         if(mask(iArgValue, EDFSECTION_FUNCTION) == true)
         {
            iArgType -= EDFSECTION_FUNCTION;
         }
         if(mask(iArgValue, 32) == true)
         {
            iArgType -= 32;
         }
         if(mask(iArgValue, 64) == true)
         {
            iArgType -= 64;
         }
         if(mask(iArgValue, EDFSECTION_MULTI) == true)
         {
            iArgType -= EDFSECTION_MULTI;
         }

         szEDFName = szArgName;
      }
      debug("EDFSectionFull arg %s / %d(%d)\n", szArgName, iArgType, iArgValue);
      EDFParser::debugPrint(pIn, EDFElement::EL_CURR | EDFElement::PR_SPACE);

      bLoop = pIn->Child(szArgName);
      while(bLoop == true)
      {
         STACKTRACEUPDATE

         bSet = false;

         pElement = pIn->GetCurr();
         if(szArgName == NULL)
         {
            szEDFName = pElement->getName(false);
         }
         debug("EDFSectionFull %s / %d\n", szEDFName, pElement->getType());

         if(pElement->getType() == EDFElement::NONE && (iArgType == -1 || iArgType == EDFElement::NONE))
         {
            if(iOp == RQ_ADD || iOp == RQ_EDIT)
            {
               debug("EDFSectionFull setting %s\n", szEDFName);
               pEDF->SetChild(szEDFName);
            }
            else
            {
               pEDF->DeleteChild(szEDFName);
            }
         }
         else if(pElement->getType() == EDFElement::BYTES && (iArgType == -1 || iArgType == EDFElement::BYTES))
         {
            pValue = pElement->getValueBytes(false);
            if(iOp == RQ_ADD || iOp == RQ_EDIT)
            {
               debug("EDFSectionFull bytes %s / %p(%ld)\n", szEDFName, pValue, pValue != NULL ? pValue->Length() : -1);
               if(mask(iArgValue, EDFSECTION_FUNCTION) == true)
               {
                  pBytesFunc = va_arg(vList, EDFSECTIONBYTESFN);
                  bSet = (*pBytesFunc)(pValue);
               }
               else if(mask(iArgValue, EDFSECTION_NOTNULL) == false ||
                  (pValue != NULL && (mask(iArgValue, EDFSECTION_NOTEMPTY) == false || pValue->Length() > 0)))
               {
                  bSet = true;
               }

               if(bSet == true)
               {
                  if(iOp == RQ_ADD && mask(iArgValue, EDFSECTION_MULTI) == true)
                  {
                     debug("EDFSectionFull adding\n");
                     pEDF->AddChild(szEDFName, pValue);
                  }
                  else
                  {
                     debug("EDFSectionFull setting\n");
                     pEDF->SetChild(szEDFName, pValue);
                  }
               }
            }
            else
            {
               if(pEDF->Child(szEDFName, pValue) == true)
               {
                  pEDF->Delete();
               }
            }
         }
         else if(pElement->getType() == EDFElement::INT && (iArgType == -1 || iArgType == EDFElement::INT))
         {
            if(iOp == RQ_ADD || iOp == RQ_EDIT)
            {
               debug("EDFSectionFull int %s / %ld\n", szEDFName, pElement->getValueInt());
               if(mask(iArgValue, EDFSECTION_FUNCTION) == true)
               {
                  pIntFunc = va_arg(vList, EDFSECTIONINTFN);
#ifdef UNIX
                  bSet = (*pIntFunc)(pElement->getValueInt());
#else
                  bSet = pIntFunc(pElement->getValueInt());
#endif
               }
               else if((mask(iArgValue, EDFSECTION_GEZERO) == false && mask(iArgValue, EDFSECTION_GTZERO) == false) ||
                  (mask(iArgValue, EDFSECTION_GEZERO) == true && pElement->getValueInt() >= 0) ||
                  (mask(iArgValue, EDFSECTION_GTZERO) == true && pElement->getValueInt() > 0))
               {
                  bSet = true;
               }

               if(bSet == true)
               {
                  debug("EDFSectionFull setting\n");
                  pEDF->SetChild(szEDFName, pElement->getValueInt());
               }
            }
         }
         else if(pElement->getType() == EDFElement::FLOAT && (iArgType == -1 || iArgType == EDFElement::FLOAT))
         {
            if(iOp == RQ_ADD || iOp == RQ_EDIT)
            {
               debug("EDFSectionFull float %s / %g\n", szEDFName, pElement->getValueFloat());
               if((mask(iArgValue, EDFSECTION_GEZERO) == false && mask(iArgValue, EDFSECTION_GTZERO) == false) ||
                  (mask(iArgValue, EDFSECTION_GEZERO) == true && pElement->getValueFloat() >= 0) ||
                  (mask(iArgValue, EDFSECTION_GTZERO) == true && pElement->getValueFloat() > 0))
               {
                  debug("EDFSectionFull setting\n");
                  pEDF->SetChild(szEDFName, pElement->getValueFloat());
               }
            }
         }

         if(bLoop == true)
         {
            bLoop = pIn->Next(szArgName);
            if(bLoop == false)
            {
               pIn->Parent();
            }
         }
      }

      if(szArgName != NULL)
      {
         szArgName = va_arg(vList, char *);
      }
      if(szArgName == NULL)
      {
         bArg = false;
      }

      // pIn->Parent();
   }

   va_end(vList);

   if(bSection == true)
   {
      if(iOp == RQ_DELETE && pEDF->Children() == 0)
      {
         pEDF->Delete();
      }
      else
      {
         pEDF->Parent();
      }
   }

   debug("EDFSectionFull exit true\n");
   EDFParser::debugPrint(pEDF, false, true);
   return true;
}

bool EDFSection(EDF *pEDF, EDF *pIn, int iOp, char *szName, ...)
{
   va_list vList;

   va_start(vList, szName);

   return EDFSectionFull(pEDF, pIn, true, (char *)NULL, (char *)NULL, iOp, szName, vList);
}

bool EDFSection(EDF *pEDF, EDF *pIn, char *szSection, int iOp, char *szName, ...)
{
   va_list vList;

   va_start(vList, szName);

   return EDFSectionFull(pEDF, pIn, true, szSection, (char *)NULL, iOp, szName, vList);
}

bool EDFSection(EDF *pEDF, EDF *pIn, char *szSection, char *szSectionValue, int iOp, char *szName, ...)
{
   va_list vList;

   va_start(vList, szName);

   return EDFSectionFull(pEDF, pIn, true, szSection, szSectionValue, iOp, szName, vList);
}

bool EDFSection(EDF *pEDF, EDF *pIn, int iOp, bool bSection, char *szName, ...)
{
   va_list vList;

   va_start(vList, szName);

   return EDFSectionFull(pEDF, pIn, bSection, (char *)NULL, (char *)NULL, iOp, szName, vList);
}

// Add EDF object to the announcement list
int ConnectionAnnounce(ConnData *pConnData, EDF *pEDF)
{
   STACKTRACE
   ConnAnnounce *pNew = NULL;

   if(pConnData == NULL)
   {
      debug("ConnectionAnnounce no conn data\n");
      return 0;
   }

   pNew = new ConnAnnounce;
   pNew->m_pAnnounce = new EDF();
   pNew->m_pAnnounce->Copy(pEDF, true, true);
   pNew->m_pNext = NULL;

   if(pConnData->m_pAnnounceFirst == NULL)
   {
      pConnData->m_pAnnounceFirst = pNew;
      // printf("ConnectionAnnounce announce d=%p / f=%p\n", pConnData, pConnData->m_pAnnounceFirst);
      pConnData->m_pAnnounceLast = pNew;
   }
   else
   {
      pConnData->m_pAnnounceLast->m_pNext = pNew;
      pConnData->m_pAnnounceLast = pNew;
   }

   pConnData->m_iAnnounces++;

   return 1;
}

// Add an announcement to users (depending on the parameters)
int ServerAnnounce(EDF *pData, const char *szAnnounce, EDF *pAnnounce, EDF *pAdmin, EDFConn *pConn)
{
   STACKTRACE
   int iBase = 0, iID = -1, iMoveID = -1, iConnNum = 0, iNumAnnounces = 0, iTotalAnnounces = 0, iSubType = 0, iSubType2 = 0;
   int iMessageID = -1, iMarked = 0, iMarkType = 0, iDebug = 0;
   bool bAnnounce = false, bIsMarked = false;
   EDF *pEDF = NULL;
   EDFConn *pListConn = NULL;
   ConnData *pListData = NULL;
   UserItem *pList = NULL;
   MessageTreeItem *pTree = NULL, *pFolder2 = NULL;
   MessageItem *pMessage = NULL;
   DBSubData *pSubData = NULL;

   debug("ServerAnnounce entry %s %p %p %p\n", szAnnounce, pAnnounce, pAdmin, pConn);

   if(pAnnounce != NULL)
   {
      pAnnounce->Root();
      pAnnounce->Set("announce", szAnnounce);
      pAnnounce->AddChild("announcetime", (int)(time(NULL)));

      // EDFParser::debugPrint("ServerAnnounce user", pAnnounce);

      pEDF = pAnnounce;
   }
   if(pAdmin != NULL)
   {
      pAdmin->Root();
      pAdmin->Set("announce", szAnnounce);
      pAdmin->AddChild("announcetime", (int)(time(NULL)));

      // EDFParser::debugPrint("ServerAnnounce admin", pAdmin);

      if(pEDF == NULL)
      {
         pEDF = pAdmin;
      }
   }

   if(pConn != NULL)
   {
      // EDFPrint("ServerAnnounce connection supplied", pAnnounce);

      iNumAnnounces += ConnectionAnnounce((ConnData *)pConn->Data(), pAnnounce);
   }
   else
   {
      iBase = RequestGroup(pEDF);

      if(iBase == RFG_FOLDER)
      {
         if(pEDF->GetChild("folderid", &iID) == true)
         {
            pTree = FolderGet(iID);
         }

         debug("ServerAnnounce %s, %d(%p %s)\n", szAnnounce, iID, pTree, pTree->GetName());
      }
      else if(iBase == RFG_MESSAGE)
      {
         pEDF->GetChild("messageid", &iMessageID);
         pMessage = FolderMessageGet(iMessageID);
         if(pEDF->GetChild("folderid", &iID) == true)
         {
            pTree = FolderGet(iID);
         }

         if(stricmp(szAnnounce, MSG_MESSAGE_MOVE) == 0 && pEDF->GetChild("moveid", &iMoveID) == true)
         {
            pFolder2 = FolderGet(iMoveID);
         }

         bIsMarked = pEDF->IsChild("marked");

         debug("ServerAnnounce %s, %d(%p) from %ld to %ld in %ld(%s)", szAnnounce, iMessageID, pMessage, pMessage->GetFromID(), pMessage->GetToID(), pTree->GetID(), pTree->GetName());
         if(pFolder2 != NULL)
         {
            debug(", move %ld(%s)", pFolder2->GetID(), pFolder2->GetName());
         }
         debug(" marked %s\n", BoolStr(bIsMarked));
      }
      else if(iBase == RFG_CHANNEL)
      {
         if(pEDF->GetChild("channelid", &iID) == true)
         {
            pTree = ChannelGet(iID);

            if(pEDF->GetChild("messageid", &iMessageID) == true)
            {
               pMessage = pTree->MessageFind(iMessageID);
            }
         }

         debug("ServerAnnounce %s,", szAnnounce);
         if(pMessage != NULL)
         {
            debug(" %d(%p) from %ld to %ld in", iMessageID, pMessage, pMessage->GetFromID(), pMessage->GetToID());
         }
         debug(" %d(%p %s):\n", iID, pTree, pTree->GetName());
         EDFParser::debugPrint(pEDF);
      }

      for(iConnNum = 0; iConnNum < ConnCount(); iConnNum++)
      {
         pListConn = ConnList(iConnNum);
         if(pListConn->State() == Conn::OPEN)
         {
            pListData = (ConnData *)pListConn->Data();
            // printf("ServerAnnounce connection %ld data %p\n", pListConn->ID(), pListData);
            if(pListData != NULL)
            {
               pList = pListData->m_pUser;
               // printf("ServerAnnounce connection %ld user %p\n", pListConn->ID(), pList);
               if(pList != NULL)
               {
                  // Logged in user
                  bAnnounce = false;
                  iMarked = 0;
                  iMarkType = 0;

                  if(iBase == RFG_FOLDER || iBase == RFG_MESSAGE)
                  {
                     iSubType = pListData->m_pFolders->SubType(pTree->GetID());
                     // printf("ServerAnnounce user %ld sub type %d\n", pList->GetID(), iSubType);
                     if(iBase == RFG_MESSAGE)
                     {
                        if(pFolder2 != NULL)
                        {
                           iSubType2 = pListData->m_pFolders->SubType(pFolder2->GetID());
                        }

                        if((iSubType > 0 || iSubType2 > 0) &&
                           MessageTreeAccess(pTree, ACCMODE_SUB_READ, ACCMODE_MEM_READ, pList->GetAccessLevel(), iSubType) > 0 &&
                           MessageAccess(pMessage, pList->GetID(), pList->GetAccessLevel(), iSubType, true, stricmp(szAnnounce, MSG_MESSAGE_DELETE) != 0) > 0 &&
                           (pFolder2 == NULL || MessageTreeAccess(pFolder2, ACCMODE_SUB_READ, ACCMODE_MEM_READ, pList->GetAccessLevel(), iSubType2) > 0))
                        {
                           // iMarked = 0;
                           bAnnounce = true;
                           if((stricmp(szAnnounce, MSG_MESSAGE_ADD) == 0 || stricmp(szAnnounce, MSG_MESSAGE_EDIT) == 0) && bIsMarked == false)
                           {
                              // iDebug = debuglevel(DEBUGLEVEL_DEBUG);
                              iMarked = MessageMarking(pList, pListData->m_pReads, NULL, szAnnounce, (FolderMessageItem *)pMessage, 0, &iMarkType);
                              // debuglevel(iDebug);
                           }

                           debug("ServerAnnounce announcing to %d (marked=%d marktype=%d)\n", pList->GetID(), iMarked, iMarkType);
                        }
                     }
                     else if((stricmp(szAnnounce, MSG_FOLDER_ADD) == 0 || stricmp(szAnnounce, MSG_FOLDER_DELETE) == 0 || stricmp(szAnnounce, MSG_FOLDER_EDIT) == 0) &&
                        MessageTreeAccess(pTree, ACCMODE_SUB_READ, ACCMODE_MEM_READ, pList->GetAccessLevel(), iSubType) > 0)
                     {
                        bAnnounce = true;
                     }
                  }
                  else if(iBase == RFG_CHANNEL)
                  {
                     pSubData = pListData->m_pChannels->SubData(pTree->GetID());
                     debug("ServerAnnounce channel check %p\n", pSubData);
                     if(pSubData != NULL)
                     {
                        debug("ServerAnnounce channel check %d %d, %d\n", pSubData->m_iSubType, pSubData->m_iExtra, pList->GetAccessLevel());
                        if(pSubData->m_iExtra == 1 && MessageTreeAccess(pTree, ACCMODE_SUB_READ, ACCMODE_MEM_READ, pList->GetAccessLevel(), pSubData->m_iSubType) > 0)
                        {
                           bAnnounce = true;
                        }
                     }
                  }
                  else
                  {
                     bAnnounce = true;
                  }

                  if(bAnnounce == true)
                  {
                     if(pAdmin != NULL && pList->GetAccessLevel() >= LEVEL_WITNESS)
                     {
                        if((stricmp(szAnnounce, MSG_MESSAGE_ADD) == 0 || stricmp(szAnnounce, MSG_MESSAGE_EDIT) == 0) && bIsMarked == false)
                        {
                           if(iMarked > 0)
                           {
                              pAdmin->SetChild("marked", iMarked);
                           }
                           if(iMarkType > 0)
                           {
                              pAdmin->SetChild("marktype", iMarkType);
                           }

                           EDFParser::debugPrint("ServerAnnounce message_add / message_edit admin marking", pAdmin);
                        }
                        iNumAnnounces += ConnectionAnnounce(pListData, pAdmin);
                        if((stricmp(szAnnounce, MSG_MESSAGE_ADD) == 0 || stricmp(szAnnounce, MSG_MESSAGE_EDIT) == 0) && bIsMarked == false)
                        {
                           pAdmin->DeleteChild("marked");
                           pAdmin->DeleteChild("marktype");
                        }
                     }
                     else if(pAnnounce != NULL)
                     {
                        if((stricmp(szAnnounce, MSG_MESSAGE_ADD) == 0 || stricmp(szAnnounce, MSG_MESSAGE_EDIT) == 0) && bIsMarked == false)
                        {
                           if(iMarked > 0)
                           {
                              pAnnounce->SetChild("marked", iMarked);
                           }
                           if(iMarkType > 0)
                           {
                               pAnnounce->SetChild("marktype", iMarkType);
                           }

                           EDFParser::debugPrint("ServerAnnounce message_add / message_edit announce marking", pAnnounce);
                        }
                        iNumAnnounces += ConnectionAnnounce(pListData, pAnnounce);
                        if((stricmp(szAnnounce, MSG_MESSAGE_ADD) == 0 || stricmp(szAnnounce, MSG_MESSAGE_EDIT) == 0) && bIsMarked == false)
                        {
                           pAnnounce->DeleteChild("marked");
                           pAnnounce->DeleteChild("marktype");
                        }
                     }
                  }
               }

               // printf("ServerAnnounce announce c=%p d=%p f=%p\n", pListConn, pListData, pListData->m_pAnnounceFirst);
            }
            else
            {
               debug("ServerAnnounce connection %ld NULL data\n", pListConn->ID());
            }
         }
         else
         {
            debug("ServerAnnounce connection %ld not open\n", pListConn->ID());
         }
      }
   }

   // Announce messages to multiple users only count as a single message
   if(iNumAnnounces > 0)
   {
      pData->Root();
      pData->Child("system");
      pData->GetChild("announces", &iTotalAnnounces);
      pData->SetChild("announces", iTotalAnnounces + 1);
   }

   debug("ServerAnnounce return %d\n", iNumAnnounces);
   return iNumAnnounces;
}

bool ConnectionAnnounce(EDFConn *pConn, ConnData *pConnData, EDF *pData)
{
   STACKTRACE
   bool bReturn = false;
   ConnAnnounce *pNext = NULL;

   if(pConnData == NULL)
   {
      debug("ConnectionAnnounce ERROR: no local data for %s\n", pConn->Hostname());
      return false;
   }

   if(pConnData->m_pAnnounceFirst == NULL)
   {
      // printf("ConnectionAnnounce no announce first\n");
      return false;
   }

   // EDFPrint("ConnectionAnnounce", pLocalData->m_pAnnounceFirst->m_pAnnounce);
   if(pConn->State() == Conn::OPEN)
   {
      // EDFPrint("ConnectionAnnounce writing", pConnData->m_pAnnounceFirst->m_pAnnounce);
      pConn->Write(pConnData->m_pAnnounceFirst->m_pAnnounce);
      bReturn = true;
   }

   // printf("ConnectionAnnounce delete\n");
   delete pConnData->m_pAnnounceFirst->m_pAnnounce;
   pNext = pConnData->m_pAnnounceFirst->m_pNext;
   if(pNext == NULL)
   {
      pConnData->m_pAnnounceLast = NULL;
   }
   delete pConnData->m_pAnnounceFirst;

   pConnData->m_pAnnounceFirst = pNext;
   // printf("ConnectionAnnounce announce first %p\n", pConnData->m_pAnnounceFirst);

   // printf("ConnectionAnnounce exit %s\n", BoolStr(bReturn));
   return bReturn;
}

// Check all child nodes for matching hostname and address fields
bool ConnectionMatch(EDF *pData, char *szHostname, unsigned long lAddress)
{
   STACKTRACE
   unsigned long lMin = 0, lMax = 0;
   bool bLoop = false, bMatch = false;
   char *szType = NULL, *szValue = NULL, *szName = NULL;

   debug(DEBUGLEVEL_DEBUG, "ConnectionMatch entry %s %ul\n", szHostname, lAddress);
   // EDFPrint(pData, false);

   pData->GetChild("name", &szName);
   debug(DEBUGLEVEL_DEBUG, "ConnectionMatch location %s\n", szName);
   delete[] szName;

   bLoop = pData->Child();
   while(bLoop == true)
   {
      szValue = NULL;
      pData->Get(&szType, &szValue);

      debug(DEBUGLEVEL_DEBUG, "ConnectionMatch check %s %s\n", szType, szValue);
      if(stricmp(szType, "hostname") == 0 && szValue != NULL && szHostname != NULL)
      {
         bMatch = strmatch(szHostname, szValue, true, true, false);
      }
      else if(stricmp(szType, "address") == 0 && szValue != NULL && lAddress != 0)
      {
         if(Conn::CIDRToRange(szValue, &lMin, &lMax) == true)
         {
            debug(DEBUGLEVEL_DEBUG, "ConnectionMatch %s -> %lu / %lu\n", szValue, lMin, lMax);

            if(lMin <= lAddress && lAddress <= lMax)
            {
               bMatch = true;
            }

            /* lMin = htonl(lMin);
            lMax = htonl(lMax);
            debug("ConnectionMatch address %s, %ul -> %ul\n", szValue, lMin, lMax);
            if(lMin <= lAddress && lAddress <= lMax)
            {
               bMatch = true;
            } */
         }
      }

      if(bMatch == true)
      {
         bLoop = false;
         pData->Parent();
      }
      else
      {
         bLoop = pData->Next();
         if(bLoop == false)
         {
            pData->Parent();
         }
      }

      delete[] szType;
      delete[] szValue;
   }

   debug(DEBUGLEVEL_DEBUG, "ConnectionMatch exit %s\n", BoolStr(bMatch));
   // EDFPrint(pData, false);
   return bMatch;
}

char *ConnectionLocation(EDF *pData, char *szHostname, unsigned long lAddress)
{
   STACKTRACE
   int iTotalUses = 0;
   bool bLoop = false;
   char *szLocation = NULL;
   EDFElement *pBookmark = NULL;

   debug("ConnectionLocation entry %s %lu\n", szHostname, lAddress);

   // Try to match hostname / address with a location
   pData->Root();
   pData->Child("locations");
   bLoop = pData->Child("location");
   while(bLoop == true)
   {
      // EDFPrint("ConnectionLocation match check", pData);
      if(ConnectionMatch(pData, szHostname, lAddress) == true)
      {
         // EDFPrint("ConnectionLocation matched", pData, false);

         pBookmark = pData->GetCurr();
         bLoop = pData->Child("location");
      }
      else
      {
         bLoop = pData->Next("location");
      }
   }
   if(pBookmark != NULL)
   {
      pData->SetCurr(pBookmark);
      pData->GetChild("name", &szLocation);
      pData->SetChild("lastuse", (int)(time(NULL)));
      pData->GetChild("totaluses", &iTotalUses);
      pData->SetChild("totaluses", ++iTotalUses);

      debug("ConnectionLocation location %s\n", szLocation);
   }
   /* else
   {
      // Unable to find location - set it to the default
      szLocation = strmk("Unknown");
   } */

   debug("ConnectionLocation exit %s\n", szLocation);
   return szLocation;
}

char *ConnectionLocation(EDFConn *pConn, EDF *pData)
{
   ConnData *pConnData = CONNDATA;

   debug("ConnectionLocation %p %p, %p\n", pConn, pData, pConnData);

   if(pConnData != NULL && (pConnData->m_szProxy != NULL || pConnData->m_lProxy > 0))
   {
      return ConnectionLocation(pData, pConnData->m_szProxy, pConnData->m_lProxy);
   }

   return ConnectionLocation(pData, pConn->Hostname(), pConn->Address());
}

void ConnectionLocations(EDF *pData)
{
   int iConnNum = 0;
   EDFConn *pListConn = NULL;
   ConnData *pListData = NULL;

   for(iConnNum = 0; iConnNum < ConnCount(); iConnNum++)
   {
      pListConn = ConnList(iConnNum);
      pListData = (ConnData *)pListConn->Data();

      delete[] pListData->m_szLocation;
      pListData->m_szLocation = ConnectionLocation(pListConn, pData);
   }
}

bool ConnectionVisible(ConnData *pConnData, int iUserID, int iAccessLevel)
{
   // printf("ConnectionVisible %p(%p %d) %d %d\n", pConnData, pConnData->m_pUser, pConnData->m_iStatus, iUserID, iAccessLevel);

   if(iAccessLevel >= LEVEL_WITNESS || mask(pConnData->m_iStatus, LOGIN_SILENT) == false)
   {
      return true;
   }

   if(pConnData->m_pUser != NULL && (pConnData->m_pUser->GetID() == iUserID || (iUserID != -1 && pConnData->m_pUser->GetOwnerID() == iUserID)))
   {
      return true;
   }

   debug("ConnectionVisible false, %p(%p %d) %d %d\n", pConnData, pConnData->m_pUser, pConnData->m_iStatus, iUserID, iAccessLevel);
   return false;
}

ConnData *ConnectionSetup(EDFConn *pConn, EDF *pData, int iAttempts)
{
   ConnData *pConnData = NULL;

   debug("ConnectionSetup entry %p %p %d, %s %s\n", pConn, pData, iAttempts, pConn->Hostname(), pConn->AddressStr());

   pConnData = new ConnData;

   pConnData->m_bClose = false;
   pConnData->m_lTimeOn = time(NULL);
   pConnData->m_lTimeIdle = pConnData->m_lTimeOn;
   pConnData->m_szLocation = ConnectionLocation(pConn, pData);
   pConnData->m_iStatus = LOGIN_OFF;
   pConnData->m_iRequests = 0;
   pConnData->m_iAnnounces = 0;

   debug("ConnectionSetup location %s\n", pConnData->m_szLocation);

   pConnData->m_iLoginAttempts = iAttempts;

   pConnData->m_pUser = NULL;

   pConnData->m_pAnnounceFirst = NULL;
   // printf("ConnectionSetup announce first %p\n", pConnData->m_pAnnounceFirst);
   pConnData->m_pAnnounceLast = NULL;

   pConnData->m_lTimeBusy = -1;
   pConnData->m_szClient = NULL;
   pConnData->m_szProtocol = NULL;
   pConnData->m_szProxy = NULL;
   pConnData->m_lProxy = 0;
   pConnData->m_pStatusMsg = NULL;
   pConnData->m_lAttachmentSize = 0;

   pConnData->m_pFolders = NULL;
   pConnData->m_pReads = NULL;
   pConnData->m_pSaves = NULL;

   pConnData->m_pChannels = NULL;
   pConnData->m_pServices = NULL;

   debug("ConnectionSetup exit %p\n", pConnData);
   return pConnData;
}

// ConnectionInfo: Put connection / connection data into an EDF object
void ConnectionInfo(EDF *pEDF, EDFConn *pInfoConn, int iLevel, bool bCurrent)
{
   STACKTRACE
   int iChannelNum = 0, iTalking = 0;
   ConnData *pInfoData = NULL;
   UserItem *pInfo = NULL;

   // printf("ConnectionInfo entry %p %p %d\n", pEDF, pInfoConn, iLevel);

   pInfoData = (ConnData *)pInfoConn->Data();
   pInfo = pInfoData->m_pUser;

   /* if(pInfoData->m_pChannels != NULL)
   {
      while(iTalking == 0 && iChannelNum < pInfoData->m_pChannels->Count())
      {
         if(pInfoData->m_pChannels->ItemExtra(iChannelNum) == 1)
         {
            iTalking = LOGIN_TALKING;
         }
         else
         {
            iChannelNum++;
         }
      }
   } */

   // printf("ConnectionInfo talking=%d data=%p user=%p\n", iTalking, pInfoData, pInfo);

   UserItem::WriteLogin(pEDF, iLevel, mask(pInfoData->m_iStatus, LOGIN_SHADOW) == true || pInfoData->m_pUser == NULL ? pInfoConn->ID() : -1, pInfoData->m_iStatus | iTalking, pInfoConn->GetSecure(), pInfoData->m_iRequests, pInfoData->m_iAnnounces,
      pInfoData->m_lTimeOn, pInfo != NULL ? pInfo->GetTimeOff() : -1, pInfoData->m_lTimeBusy, pInfoData->m_lTimeIdle, ConnVersion(pInfoData, "2.5") >= 0 ? "statusmsg" : "busymsg", pInfoData->m_pStatusMsg,
      pInfoConn->Hostname(), pInfoConn->Address(), pInfoData->m_szProxy, pInfoData->m_lProxy,
      pInfoData->m_szLocation, pInfoData->m_szClient, pInfoData->m_szProtocol, pInfoData->m_lAttachmentSize, bCurrent);

   // EDFPrint("ConnectionInfo exit", pEDF, EDFElement::EL_CURR | EDFElement::PR_SPACE);
}

/*
** ConnectionFind: Find a connection from the given type and ID
** iType:   1 - Connection ID
**          2 - User ID
*/
EDFConn *ConnectionFind(int iType, int iID)
{
   STACKTRACE
   bool bFound = false;
   int iConnNum = 0;
   EDFConn *pListConn = NULL;
   ConnData *pListData = NULL;
   UserItem *pList = NULL;

   // printf("ConnectionFind entry %d %d\n", iType, iID);

   while(bFound == false && iConnNum < ConnCount())
   {
      pListConn = ConnList(iConnNum);
      pListData = (ConnData *)pListConn->Data();
      pList = pListData->m_pUser;

      // printf("ConnectionFind %p(%ld) %p\n", pListConn, pListConn->ID(), pListData);

      if(iType == CF_CONNID && iID == pListConn->ID())
      {
         bFound = true;
      }
      else if((iType == CF_USERID || iType == CF_USERPRI) && pList != NULL && pList->GetID() == iID && (iType == CF_USERID || mask(pListData->m_iStatus, LOGIN_SHADOW) == false))
      {
         bFound = true;
      }
      else
      {
         iConnNum++;
      }
   }

   // printf("ConnectionFind found %s\n", BoolStr(bFound));

   if(bFound == false)
   {
      // printf("ConnectionFind exit NULL, %d %d\n", iType, iID);
      return NULL;
   }

   return pListConn;
}

int ConnectionShadows(int iID, int iUserID, int iAccessLevel)
{
   int iListNum = 0, iReturn = 0;
   EDFConn *pListConn = NULL;
   ConnData *pListData = NULL;
   UserItem *pList = NULL;

   for(iListNum = 0; iListNum < ConnCount(); iListNum++)
   {
      pListConn = ConnList(iListNum);
      pListData = (ConnData *)pListConn->Data();
      pList = pListData->m_pUser;
      if(pList != NULL && pList->GetID() == iID && mask(pListData->m_iStatus, LOGIN_SHADOW) == true && ConnectionVisible(pListData, iUserID, iAccessLevel) == true)
      {
         iReturn++;
      }
   }

   if(iReturn > 0)
   {
      debug("ConnectionShadows %d -> %d\n", iID, iReturn);
   }
   return iReturn;
}

void UserLogoutAnnounce(EDF *pEDF, int iID, char *szName, int iByID, bool bLost, EDF *pIn)
{
   pEDF->AddChild("userid", iID);
   pEDF->AddChild("username", szName);
   if(iByID != -1)
   {
      debug("UserLogoutAnnounce shut forced\n");
      pEDF->AddChild("force", true);
   }
   else if(bLost == true)
   {
      pEDF->AddChild("lost", true);
   }

   if(pIn != NULL && iByID == -1)
   {
      EDFSetStr(pEDF, pIn, "text", UA_SHORTMSG_LEN);
   }
}

int UserLogoutChannels(EDF *pData, UserItem *pItem, DBSub *pChannels)
{
   int iChannelNum = 0, iChannelID = -1, iCurrType = 0, iSubType = 0;
   char *szAnnounce = NULL;
   MessageTreeItem *pChannel = NULL;

   debug("UserLogoutChannels %p(%ld %s) %p(%d)\n", pItem, pItem->GetID(), pItem->GetName(), pChannels, pChannels->Count());

   for(iChannelNum = 0; iChannelNum < pChannels->Count(); iChannelNum++)
   {
      iChannelID = pChannels->ItemID(iChannelNum);
      iCurrType = pChannels->ItemSubType(iChannelNum);
      if(iCurrType >= SUBTYPE_MEMBER)
      {
         szAnnounce = MSG_CHANNEL_SUBSCRIBE;
         iSubType = iCurrType;
      }
      else
      {
         szAnnounce = MSG_CHANNEL_UNSUBSCRIBE;
         iSubType = 0;
      }

      pChannel = ChannelGet(iChannelID);

      MessageTreeSubscribeAnnounce(pData, szAnnounce, RFG_CHANNEL, pChannel, pItem, iSubType, 0, iCurrType, pItem);
   }

   return 0;
}

/*
** ConnectionShut: Close a connection
** iType:   0 - Current connection
**          1 - Connection ID
**          2 - User ID
*/
bool ConnectionShut(EDFConn *pConn, EDF *pData, int iType, int iID, UserItem *pCurr, EDF *pIn, bool bLost)
{
   STACKTRACE
   int iByID = -1, iConnNum = 0;
   bool bFound = false, bConnDataDelete = true;
   char *szByName = NULL, *szName = NULL;
   bytes *pText = NULL;
   EDF *pAnnounce = NULL, *pAdmin = NULL;
   EDFConn *pShut = NULL, *pListConn = NULL;
   ConnData *pShutData = NULL, *pListData = NULL;
   UserItem *pItem = NULL;
   ConnAnnounce *pNext = NULL;

   debug("ConnectionShut %p %d %d %p %s\n", pConn, iType, iID, pCurr, BoolStr(bLost));

   if(iType == 0)
   {
      pShut = pConn;
   }
   else
   {
      pShut = ConnectionFind(iType, iID);
   }

   debug("ConnectionShut conn %p\n", pShut);

   if(pShut != NULL)
   {
      if(pCurr != NULL)
      {
         iByID = pCurr->GetID();
         szByName = pCurr->GetName();
      }

      pShutData = (ConnData *)pShut->Data();
      pItem = pShutData->m_pUser;

      debug("ConnectionShut data %p\n", pShutData);

      if(pItem != NULL)
      {
         debug("ConnectionShut user %p\n", pItem);

         iID = pItem->GetID();
         szName = pItem->GetName();

         debug("ConnectionShut user %s %d\n", szName, pShutData->m_iStatus);

         if(mask(pShutData->m_iStatus, LOGIN_SILENT) == false) // && mask(pShutData->m_iStatus, LOGIN_SHADOW) == false)
         {
            pAnnounce = new EDF();
            UserLogoutAnnounce(pAnnounce, iID, szName, iByID, bLost, pIn);
         }

         STACKTRACEUPDATE

         UserLogoutChannels(pData, pShutData->m_pUser, pShutData->m_pChannels);

         STACKTRACEUPDATE

         pAdmin = new EDF();
         if(pAnnounce != NULL)
         {
            pAdmin->Copy(pAnnounce, true, true);
         }
         else
         {
            UserLogoutAnnounce(pAdmin, iID, szName, iByID, bLost, pIn);
         }
         if(iByID != -1)
         {
            pAdmin->AddChild("byid", iByID);
            pAdmin->AddChild("byname", szByName);
            if(pIn != NULL)
            {
               EDFSetStr(pAdmin, pIn, "text", UA_SHORTMSG_LEN);
            }
         }

         // Get the truncated logout message
         pAdmin->GetChild("text", &pText);

         STACKTRACEUPDATE

         if(mask(pShutData->m_iStatus, LOGIN_SHADOW) == false)
         {
            UserLogout(pData, pItem, pShutData, pText);
         }

         delete pText;

         if(pAnnounce != NULL)
         {
            ConnectionInfo(pAnnounce, pShut, USERITEMWRITE_LOGIN + USERITEMWRITE_LOGINFLAT, false);
         }
         if(pAdmin != NULL)
         {
            ConnectionInfo(pAdmin, pShut, EDFITEMWRITE_ADMIN + USERITEMWRITE_LOGIN + USERITEMWRITE_LOGINFLAT, false);
         }
         ServerAnnounce(pData, MSG_USER_LOGOUT, pAnnounce, pAdmin);

         delete pAnnounce;
         delete pAdmin;

         debug("ConnectionShut delete user announce data\n");
         while(pShutData->m_pAnnounceFirst != NULL)
         {
            pNext = pShutData->m_pAnnounceFirst->m_pNext;

            delete pShutData->m_pAnnounceFirst->m_pAnnounce;
            delete pShutData->m_pAnnounceFirst;

            pShutData->m_pAnnounceFirst = pNext;
            // printf("ConnectionShut announce first %p\n", pShutData->m_pAnnounceFirst);
         }
         pShutData->m_pAnnounceLast = NULL;

         /* if(mask(pItem->GetUserType(), USERTYPE_TEMP) == true)
         {
            debug("ConnectionShut deleting temp user\n");
            UserDelete(pData, pCurr, pItem);
         } */

         // DBTable::Debug(true);

         // Find other connections by this user
         debug("ConnectionShut find other connections\n");
         while(iConnNum < ConnCount() && bConnDataDelete == true)
         {
            pListConn = ConnList(iConnNum);
            pListData = (ConnData *)pListConn->Data();

            debug("ConnectionShut connection %d %p(%ld) %p\n", iConnNum, pListConn, pListConn->ID(), pListData);

            if(pListData != NULL && pListData->m_pUser != NULL && pListData->m_pUser->GetID() == pItem->GetID() && pListConn->ID() != pShut->ID())
            {
               debug("ConnectionShut found other connection\n");
               bConnDataDelete = false;
            }
            else
            {
               iConnNum++;
            }
         }

         if(mask(pItem->GetUserType(), USERTYPE_TEMP) == false)
         {
            debug("ConnectionShut storing folder sub list\n");
            pShutData->m_pFolders->Write();
         }
         else
         {
            pShutData->m_pFolders->SetChanged(false);
         }
         if(bConnDataDelete == true)
         {
            debug("ConnectionShut deleting folder subs\n");
            delete pShutData->m_pFolders;
         }
         pShutData->m_pFolders = NULL;

         if(mask(pItem->GetUserType(), USERTYPE_TEMP) == false)
         {
            debug("ConnectionShut storing message read list\n");
            pShutData->m_pReads->Write();
            debug("ConnectionShut storing message save list\n");
            pShutData->m_pSaves->Write();
         }
         else
         {
            pShutData->m_pReads->SetChanged(false);
            pShutData->m_pSaves->SetChanged(false);
         }
         if(bConnDataDelete == true)
         {
            debug("ConnectionShut deleting message read list\n");
            delete pShutData->m_pReads;
            debug("ConnectionShut deleting message save list\n");
            delete pShutData->m_pSaves;
         }
         pShutData->m_pReads = NULL;
         pShutData->m_pSaves = NULL;

         if(mask(pItem->GetUserType(), USERTYPE_TEMP) == false)
         {
            debug("ConnectionShut storing channel sub list\n");
            pShutData->m_pChannels->Write();
         }
         else
         {
            pShutData->m_pChannels->SetChanged(false);
         }

         debug("ConnectionShut deleting channel subs\n");
         delete pShutData->m_pChannels;
         pShutData->m_pChannels = NULL;

         if(mask(pItem->GetUserType(), USERTYPE_TEMP) == false)
         {
            debug("ConnectionShut storing channel sub list\n");
            pShutData->m_pServices->Write();
         }
         else
         {
            pShutData->m_pServices->SetChanged(false);
         }

         debug("ConnectionShut deleting services\n");
         delete pShutData->m_pServices;
         pShutData->m_pServices = NULL;

         if(mask(pItem->GetUserType(), USERTYPE_TEMP) == false)
         {
            debug("ConnectionShut storing user\n");
            pShutData->m_pUser->Update();
            pShutData->m_pUser->SetChanged(false);
         }
         else
         {
            debug("ConnectionShut deleting temp user\n");
            UserDelete(pData, pCurr, pItem);
         }

         // DBTable::Debug(false);

         pShutData->m_pUser = NULL;

         delete[] pShutData->m_szClient;
         delete[] pShutData->m_szProtocol;
         delete[] pShutData->m_szProxy;
         delete pShutData->m_pStatusMsg;

         if(mask(pItem->GetUserType(), USERTYPE_AGENT) == true)
         {
            AgentLogout(pData, pItem);
         }
      }
      else // if(pShutConnection != NULL)
      {
         debug("ConnectionShut connection\n");

         // pShutConnection->Get(NULL, &iID);

         pAdmin = new EDF();

         if(iByID != -1)
         {
            debug("ConnectionShut shut forced\n");
            pAdmin->AddChild("force", true);
            pAdmin->AddChild("byid", iByID);
            pAdmin->AddChild("byname", szByName);
            if(pIn != NULL)
            {
               // pAdmin->AddChild(pIn, "text");
               EDFSetStr(pAdmin, pIn, "text", UA_SHORTMSG_LEN);
            }
            /* if(pConnData->m_szCloseText != NULL)
            {
               EDFSetStr(pAdmin, pConnData->m_szCloseText, "text", UA_SHORTMSG_LEN);
            } */
         }
         if(bLost == true)
         {
            pAdmin->AddChild("lost", true);
         }

         ConnectionInfo(pAdmin, pShut, EDFITEMWRITE_ADMIN + USERITEMWRITE_LOGIN + USERITEMWRITE_LOGINFLAT, false);
         ServerAnnounce(pData, MSG_CONNECTION_CLOSE, NULL, pAdmin);

         delete pAdmin;

         debug("ConnectionShut deleting other fields\n");

         // printf("ConnectionShut delete connection EDF %p\n", pShutData->m_pConnection);
         // delete pShutData->m_pConnection;
      }

      debug("ConnectionShut close true\n");
      pShutData->m_bClose = true;

      bFound = true;
   }

   debug("ConnectionShut exit %s\n", BoolStr(bFound));
   return bFound;
}

bool ConnectionShut(EDF *pData, int iType, int iID, UserItem *pCurr, EDF *pIn)
{
   return ConnectionShut(NULL, pData, iType, iID, pCurr, pIn, false);
}

bool ConnectionShut(EDFConn *pConn, EDF *pData, UserItem *pCurr, bool bLost, EDF *pIn)
{
   return ConnectionShut(pConn, pData, 0, -1, pCurr, pIn, bLost);
}

bool ConnectionTrust(EDFConn *pConn, EDF *pData)
{
   STACKTRACE
   bool bTrust = false;

   debug("ConectionTrust entry %s %lu\n", pConn->Hostname(), pConn->Address());

   pData->TempMark();

   pData->Root();
   pData->Child("system");
   if(pData->Child("connection") == true)
   {
      // EDFPrint("ConnectionTrust login section", pData, false);

      if(pData->Child("trust") == true)
      {
         // EDFPrint("ConnectionTrust trust section", pData, false);

         bTrust = ConnectionMatch(pData, pConn->Hostname(), pConn->Address());
      }
   }

   pData->TempUnmark();

   debug("ConnectionTrust exit %s\n", BoolStr(bTrust));
   return bTrust;
}

// Ensure a connection is allowed from the current host / location
bool ConnectionValid(EDFConn *pConn, EDF *pData)
{
   STACKTRACE
   int iNumDenies = 0;
   bool bReturn = true;
   // char *szHostname = NULL;

   if(pData == NULL)
   {
      // No login data
      return true;
   }

   // szHostname = pConn->Hostname();

   debug("ConnectionValid entry %s %lu\n", pConn->Hostname(), pConn->Address());
   // EDFPrint(pData, false);

   // Check for a deny section (default is allow all)
   if(pData->Child("deny") == false)
   {
      // delete[] szHostname;

      debug("ConnectionValid exit true, no deny\n");
      return true;
   }

   debug("ConnectionValid deny exists\n");
   iNumDenies = pData->Children("hostname") + pData->Children("address");

   if(iNumDenies == 0)
   {
      debug("ConnectionValid deny from all\n");

      // Deny from all, check for allows
      pData->Parent();

      if(pData->Child("allow") == false)
      {
         // No allow from anywhere

         // delete[] szHostname;

         debug("ConnectionValid exit false, no allow\n");
         return false;
      }

      debug("ConnectionValid allow exists\n");
   }

   bReturn = ConnectionMatch(pData, pConn->Hostname(), pConn->Address());

   // Return to entry node
   pData->Parent();

   if(iNumDenies > 0)
   {
      // This is a deny check so true means a deny matched
      bReturn = !bReturn;
   }

   // delete[] szHostname;

   debug("ConnectionValid exit %s\n", BoolStr(bReturn));
   return bReturn;
}

bool PrivilegeValid(UserItem *pCurr, char *szMessage, int iTargetID, char **szTargetName)
{
   STACKTRACE
   int iSourceID = 0;//, iUserEDF = 0;
   // bool bLoop = false;
   bool bMatch = false;
   EDF *pPrivilege = NULL;
   UserItem *pTarget = NULL;

   // pData->Get(NULL, &iSourceID);
   iSourceID = pCurr->GetID();
   debug("PrivilegeValid entry %d %s %d\n", iSourceID, szMessage, iTargetID);

   // if(pData->Child("proxy") == false)
   pPrivilege = pCurr->GetPrivilege();
   if(pPrivilege == NULL)
   {
      debug("PrivilegeValid exit false, no source section\n");
      return false;
   }

   EDFParser::debugPrint("PrivilegeValid source user privilege", pPrivilege, EDFElement::EL_CURR | EDFElement::PR_SPACE);

   if(pPrivilege->IsChild("grant", szMessage) == false)
   {
      debug("PrivilegeValid exit false, no grant\n");
      return false;
   }

   if(iTargetID != -1)
   {
      // pTempMark = pData->GetCurr();

      pTarget = UserGet(iTargetID, szTargetName, true);
      if(pTarget == NULL)
      {
         // pData->SetCurr(pTempMark);

         debug("PrivilegeValid exit false, no target user\n");
         return false;
      }

      pPrivilege = pTarget->GetPrivilege();
      if(pPrivilege == NULL)
      {
         // pData->SetCurr(pTempMark);

         debug("PrivilegeValid exit false, no target section\n");
         return false;
      }

      if(pPrivilege->Child("allow", szMessage) == false)
      {
         debug("PrivilegeValid exit false, no allow\n");
         return false;
      }

      if(pPrivilege->Children("userid") == 0)
      {
         // pData->SetCurr(pTempMark);

         debug("PrivilegeValid exit true, allows message for all users\n");
         return true;
      }

      bMatch = EDFFind(pPrivilege, "userid", iSourceID, false);

      // pData->SetCurr(pTempMark);
   }
   else
   {
      debug("PrivilegeValid no target\n");

      bMatch = true;
   }

   debug("PrivilegeValid return %s\n", BoolStr(bMatch));
   return bMatch;
}

char *FilenameToMIME(EDF *pData, char *szFilename)
{
   char *szExt = NULL, *szReturn = NULL;
#ifdef UNIX
   int iLinePos = 0, iStartPos = 0;
   char szLine[1000];
   char *szMIMETypes = NULL, *szContentType = NULL, *szMatch = NULL;
   FILE *fMIMETypes = NULL;
#else
   int iKeyPos = 0, iValuePos = 0;
   char szKey[1000], szName[1000];
   BYTE pValue[1000];
   DWORD dKeyLen = 1000, dNameLen = 0, dValueLen = 0;
   LONG dContentLen = 1000;
   HKEY hFileKey = NULL;
#endif

   if(szFilename == NULL)
   {
      return NULL;
   }

   debug("FilenameToMIME entry %s\n", szFilename);

   szExt = szFilename + strlen(szFilename);
   while(szExt > szFilename && (*szExt) != '.')
   {
      szExt--;
   }

   if((*szExt) != '.')
   {
      debug("FilenameToMIME exit NULL, no extension\n");
      return NULL;
   }

   debug("FilenameToMIME extension '%s'\n", szExt);

   if(stricmp(szExt, ".edf") == 0)
   {
      debug("FilenameToMIME exit %s, EDF extension\n", MSGATT_EDF);
      return strmk(MSGATT_EDF);
   }

#ifdef UNIX

   pData->Root();
   pData->Child("system");

   if(pData->GetChild("mimetypes", &szMIMETypes) == false || szMIMETypes == NULL)
   {
      debug("FilenameToMIME exit NULL, no mimetypes field\n");
      return NULL;
   }

   fMIMETypes = fopen(szMIMETypes, "r");
   if(fMIMETypes == NULL)
   {
      debug("FilenameToMIME exit NULL, cannot open %s (%s)\n", szMIMETypes, strerror(errno));
      return NULL;
   }

   szExt++;

   debug("FilenameToMIME extension strip '%s'\n", szExt);

   while(szReturn == NULL && fgets(szLine, sizeof(szLine), fMIMETypes) != NULL)
   {
      // printf("FilenameToMIME line %s", szLine);

      while(strlen(szLine) > 1 && (szLine[strlen(szLine) - 1] == '\r' || szLine[strlen(szLine) - 1] == '\n'))
      {
         szLine[strlen(szLine) - 1] = '\0';
      }

      if(strlen(szLine) > 1 && szLine[0] != '#')
      {
         iLinePos = 0;
         szContentType = NULL;

         while(iLinePos < strlen(szLine) && szLine[iLinePos] != '\t' && szLine[iLinePos] != ' ')
         {
            iLinePos++;
         }

         if(iLinePos < strlen(szLine))
         {
            szContentType = strmk(szLine, 0, iLinePos);
            debug("FilenameToMIME content-type '%s'\n", szContentType);

            while(szReturn == NULL && iLinePos <= strlen(szLine))
            {
               while(iLinePos < strlen(szLine) && (szLine[iLinePos] == '\t' || szLine[iLinePos] == ' '))
               {
                  iLinePos++;
               }

               if(iLinePos < strlen(szLine))
               {
                  iStartPos = iLinePos;
                  while(iLinePos < strlen(szLine) && szLine[iLinePos] != '\t' && szLine[iLinePos] != ' ')
                  {
                     iLinePos++;
                  }

                  if(iLinePos <= strlen(szLine))
                  {
                     szMatch = strmk(szLine, iStartPos, iLinePos);

                     debug("FilenameToMIME match '%s'\n", szMatch);
                     if(stricmp(szExt, szMatch) == 0)
                     {
                        szReturn = strmk(szContentType);
                     }
                     else
                     {
                        iLinePos++;
                     }

                     delete[] szMatch;
                  }
               }
            }

            delete[] szContentType;
         }
      }
   }

   fclose(fMIMETypes);

#else

   dKeyLen = sizeof(szKey);
   while(szReturn == NULL && RegEnumKey(HKEY_CLASSES_ROOT, iKeyPos, szKey, dKeyLen) == ERROR_SUCCESS)
   {
      // printf("FilenameToMIME key %s, %d\n", szName, dKeyLen);

      if(strlen(szKey) >= 2)
      {
         if(szKey[0] == '.' && stricmp(szExt, szKey) == 0)
         {
            if(RegOpenKey(HKEY_CLASSES_ROOT, szKey, &hFileKey) == ERROR_SUCCESS)
            {
               // printf("FilenameToMIME extension %s\n", szKey);

               iValuePos = 0;

               dNameLen = sizeof(szName);
               dValueLen = sizeof(pValue);
               while(szReturn == NULL && RegEnumValue(hFileKey, iValuePos, szName, &dNameLen, NULL, NULL, pValue, &dValueLen) == ERROR_SUCCESS)
               {
                  if(stricmp(szName, "Content Type") == 0)
                  {
                     szReturn = strmk((char *)pValue);
                  }
                  else
                  {
                     iValuePos++;
                     dNameLen = sizeof(szName);
                     dValueLen = sizeof(pValue);
                  }
               }

               RegCloseKey(hFileKey);
            }
            else
            {
               debug("MIMEValid failed to open %s\n", szKey);
            }
         }
      }

      if(szReturn == NULL)
      {
         iKeyPos++;
         dKeyLen = sizeof(szKey);
      }
   }

#endif

   debug("FilenameToMIME exit %s\n", szReturn);
   return szReturn;
}

int ConnVersion(ConnData *pConnData, const char *szProtocol)
{
   STACKTRACE
   int iReturn = 0;
   UserItem *pCurr = NULL;

   // printf("ConnVersion %p %s\n", pConnData, szProtocol);

   if(pConnData == NULL)
   {
      // printf("ConnVersion -1, NULL connection data\n");
      return -1;
   }

   pCurr = pConnData->m_pUser;

   if(pConnData->m_szProtocol == NULL || strlen(pConnData->m_szProtocol) == 0)
   {
      // printf("ConnVersion -1, %s has '%s' protocol\n", pCurr != NULL ? pCurr->GetName() : NULL, pConnData->m_szProtocol);
      return -1;
   }

   iReturn = ProtocolCompare(pConnData->m_szProtocol, szProtocol);
   // printf("ConnVersion %s -vs- %s -> %d\n", pConnData->m_szProtocol, szProtocol, iReturn);
   return iReturn;
}

int RequestGroup(EDF *pIn)
{
   int iReturn = 0;
   char *szRequest = NULL;

   pIn->Get(NULL, &szRequest);

   if(strnicmp(szRequest, "folder_", 7) == 0)
   {
      iReturn = RFG_FOLDER;
   }
   else if(strnicmp(szRequest, "message_", 8) == 0)
   {
      iReturn = RFG_MESSAGE;
   }
   else if(strnicmp(szRequest, "content_", 8) == 0)
   {
      iReturn = RFG_CONTENT;
   }
   else if(strnicmp(szRequest, "bulletin_", 9) == 0)
   {
      iReturn = RFG_BULLETIN;
   }
   else if(strnicmp(szRequest, "channel_", 8) == 0)
   {
      iReturn = RFG_CHANNEL;
   }
   else if(strnicmp(szRequest, "system_", 7) == 0)
   {
      iReturn = RFG_SYSTEM;
   }
   else if(strnicmp(szRequest, "user_", 5) == 0)
   {
      iReturn = RFG_USER;
   }
   else if(strnicmp(szRequest, "service_", 5) == 0)
   {
      iReturn = RFG_SERVICE;
   }
   else
   {
      debug("RequestGroup invalid request %s\n", szRequest);
   }

   delete[] szRequest;

   return iReturn;
}

bool ConnectionClose(EDFConn *pConn, EDF *pData, bool bLost)
{
   STACKTRACE
   ConnData *pConnData = CONNDATA;

   debug("ConnectionClose entry %p %p %s\n", pConn, pData, BoolStr(bLost));

   if(pConnData != NULL)
   {
      if(pConnData->m_bClose == false)
      {
         debug("ConnectionClose shut required\n");

         ConnectionShut(pConn, pData, NULL, bLost);
      }

      delete[] pConnData->m_szLocation;

      delete pConnData;
      pConn->Data(NULL);
   }

   debug("ConnectionClose exit true\n");
   return true;
}

bool AttachmentSection(EDF *pData, int iBase, int iOp, int iAttOp, MessageItem *pMessage, EDF *pEDF, ConnData *pConnData, bool bMultiple, EDF *pIn, EDF *pOut, int iUserID, int iAccessLevel, int iSubType)
{
   STACKTRACE
   int iAttID = -1, iMessageID = -1, iAttachmentSize = 0;
   bool bLoop = false, bReturn = false;
   char *szValue = NULL, *szURL = NULL, *szContentType = NULL, *szFilename = NULL;
   bytes *pAttData = NULL, *pText = NULL;
   FolderMessageItem *pFolderMessage = NULL;

   if(iBase == RFG_MESSAGE)
   {
      pFolderMessage = (FolderMessageItem *)pMessage;
   }

   debug("AttachmentSection %p %d %d %d %p %p %p %s %p %p %d %d %d\n", pData, iBase, iOp, iAttOp, pMessage, pEDF, pConnData, BoolStr(bMultiple), pIn, pOut, iUserID, iAccessLevel, iSubType);

   if(pConnData != NULL)
   {
      iAttachmentSize = pConnData->m_lAttachmentSize;
   }
   else
   {
      pData->Root();
      pData->Child("system");
      pData->GetChild("attachmentsize", &iAttachmentSize);
   }

   if(iAttachmentSize == -1 || iAttachmentSize > 0)
   {
      bLoop = pIn->Child("attachment");
      while(bLoop == true)
      {
         szValue = NULL;
         pIn->Get(NULL, &szValue);

         debug("AttachmentSection compare %s\n", szValue);
         if(iOp == RQ_ADD || (iAttOp == RQ_ADD && szValue != NULL && stricmp(szValue, "add") == 0))
         {
            szURL = NULL;
            szContentType = NULL;
            szFilename = NULL;
            pAttData = NULL;

            if(pIn->GetChild("url", &szURL) == true)
            {
               debug("AttachmentSection URL %s\n", szURL);

               if(pMessage != NULL)
               {
                  pMessage->AddAttachment(szURL, iOp == RQ_EDIT ? iUserID : -1);
               }
               bReturn = true;
            }
            else
            {
               pIn->GetChild("content-type", &szContentType);
               pIn->GetChild("filename", &szFilename);
               pIn->GetChild("data", &pAttData);

               debug("AttachmentSection content %s, filename %s, data %p(%ld)\n", szContentType, szFilename, pAttData, pAttData != NULL ? pAttData->Length() : 0);

               if(szContentType == NULL)
               {
                  szContentType = FilenameToMIME(pData, szFilename);
               }

               if(szContentType != NULL)
               {
                  if(iBase == RFG_MESSAGE && stricmp(szContentType, MSGATT_ANNOTATION) == 0)
                  {
                     if(iOp == RQ_EDIT && (iAccessLevel >= LEVEL_WITNESS || iSubType == SUBTYPE_EDITOR || pFolderMessage->IsAnnotation(iUserID) == false))
                     {
                        // Annotations on edit only for from/to(single), admin and editor

                        pText = NULL;

                        if(pIn->GetChild("text", &pText) == true && pText != NULL && pText->Length() > 0)
                        {
                           pFolderMessage->AddAnnotation(pText, iUserID);
                           bReturn = true;
                        }

                        delete pText;
                     }
                  }
                  else if(iBase == RFG_MESSAGE && stricmp(szContentType, MSGATT_LINK) == 0)
                  {
                     if(iAccessLevel >= LEVEL_WITNESS || iSubType == SUBTYPE_EDITOR)
                     {
                        // Links for admin and editor
                        pText = NULL;

                        if(pIn->GetChild("text", &pText) == true && pText != NULL && pText->Length() > 0 && pIn->GetChild("messageid", &iMessageID) == true && iMessageID > 0)
                        {
                           pFolderMessage->AddLink(pText, iMessageID, iOp == RQ_EDIT ? iUserID : -1);
                           bReturn = true;
                        }

                        delete pText;
                     }
                  }
                  else
                  {
                     if(pAttData != NULL)
                     {
                        bytesprint("AttachmentSection data", pAttData, true);

                        if(pAttData != NULL && (iAccessLevel >= LEVEL_WITNESS || pMessage == NULL || pMessage->GetFromID() == iUserID))
                        {
                           // Attachments for admin or message from this user

                           if(iAttachmentSize == -1 || (iAttachmentSize > 0 && iAttachmentSize >= pAttData->Length()))
                           {
                              debug("AttachmentSection adding %ld byte attachment", pAttData->Length());

                              if(pMessage != NULL)
                              {
                                 debug(" from %d\n", iUserID);
                                 pMessage->AddAttachment(szContentType, szFilename, pAttData, NULL, iOp == RQ_EDIT ? iUserID : -1);
                              }
                              else if(pEDF != NULL)
                              {
                                 debug(" to EDF\n");
                                 pEDF->Add("attachment", EDFMax(pEDF, "attachment", false) + 1);

                                 pEDF->AddChild("content-type", szContentType);
                                 if(szFilename != NULL)
                                 {
                                    pEDF->AddChild("filename", szFilename);
                                 }
                                 pEDF->AddChild("data", pAttData);

                                 pEDF->Parent();
                              }

                              bReturn = true;
                           }
                           else
                           {
                              debug("AttachmentSection not adding %ld byte attachment\n", pAttData->Length());

                              if(szFilename != NULL)
                              {
                                 pOut->Add("attachment");
                                 pOut->AddChild("filename", szFilename);
                                 pOut->AddChild("failed", MSGATT_TOOBIG);
                                 EDFParser::debugPrint("MessageAttachment attachment failed", pOut, EDFElement::EL_CURR | EDFElement::PR_SPACE);
                                 pOut->Parent();
                              }
                           }
                        }
                     }
                     else
                     {
                        debug("AttachmentSection no data\n");
                     }
                  }
               }
               else
               {
                  debug("MessageAttachment cannot match attachment file %s\n", szFilename);

                  if(szFilename != NULL)
                  {
                     pOut->Add("attachment");
                     pOut->AddChild("filename", szFilename);
                     pOut->AddChild("failed", MSGATT_MIMETYPE);
                     EDFParser::debugPrint("MessageAttachment attachment failed", pOut, EDFElement::EL_CURR | EDFElement::PR_SPACE);
                     pOut->Parent();
                  }
               }
            }

            delete[] szContentType;
            delete[] szFilename;
            delete pAttData;
         }
         else if(iAttOp == RQ_DELETE && (iAccessLevel >= LEVEL_WITNESS || iSubType == SUBTYPE_EDITOR))
         {
            pIn->GetChild("attachmentid", &iAttID);
            if(pMessage != NULL)
            {
               pMessage->DeleteAttachment(iAttID, iUserID);
            }
         }

         bLoop = pIn->Next("attachment");
         if(bLoop == false)
         {
            pIn->Parent();
         }

         delete[] szValue;
      }
   }
   else if(pConnData != NULL && bMultiple == false && pIn->Children("attachment") > 0)
   {
      debug("AttachmentSection not adding %d attachments (size %d)\n", pIn->Children("attachment"), iAttachmentSize);

      pOut->Add("attachment");
      pOut->AddChild("failed", MSGATT_FAILED);
      EDFParser::debugPrint("MessageAttachment attachment failed", pOut, EDFElement::EL_CURR | EDFElement::PR_SPACE);
      pOut->Parent();
   }

   return bReturn;
}

extern "C"
{

// ServerLoad: Config file / library have been loaded
ICELIBFN bool ServerLoad(EDF *pData, long lMilliseconds, int iOptions)
{
   STACKTRACE
   int iIdleTime = -1, iConnNum = 0, iID = 0;
   int iUserID = 0, iParentID = 0, iFolderID = 0;
   int iAttempts = 0, iTemp = 0, iChannelID = 0;
   bool bConnect = false, bLoop = false;
   double dTick = gettick(), dAttach = 0, dReset = 0;
   char *szDatabase = NULL, *szDBUser1 = NULL, *szDBPassword1 = NULL, *szDBUser2 = NULL, *szDBPassword2 = NULL;
   EDF *pAnnounce = NULL;
   EDFElement *pTemp = NULL;
   EDFConn *pListConn = NULL;
   UserItem *pUser = NULL;
   MessageTreeItem *pFolder = NULL, *pChannel = NULL;
   FolderMessageItem *pFolderMessage = NULL;
   ChannelMessageItem *pChannelMessage = NULL;
   ConnData *pListData = NULL;
   ConnAnnounce *pConnAnnounce = NULL;

   debug("ServerLoad entry %ld %d, %s %d %s %s\n", lMilliseconds, iOptions, VERSION, BUILDNUM, BUILDDATE, BUILDTIME);

   m_iWriteTime = time(NULL);

   pData->Root();
   pData->Child("system");
   pData->SetChild("name", SERVER_NAME);
   pData->SetChild("version", VERSION);
   pData->SetChild("protocol", PROTOCOL);
   pData->SetChild("buildnum", BUILDNUM);
   pData->SetChild("builddate", BUILDDATE);
   pData->SetChild("buildtime", BUILDTIME);
   pData->AddChild("reloadtime", (int)(time(NULL)));

   pData->GetChild("loginattempts", &iAttempts);

   pData->GetChild("idletime", &iIdleTime);
   UserItem::SetSystemTimeIdle(iIdleTime);

   pData->GetChild("database", &szDatabase);
   pData->GetChild("dbuser", &szDBUser1);
   pData->GetChild("dbpassword", &szDBPassword1);
   if(szDatabase == NULL)
   {
      debug("ServerLoad no database connection\n");
      return false;
   }
   if(szDBUser1 != NULL && szDBUser1[0] == '$')
   {
      szDBUser2 = getenv(szDBUser1 + 1);
   }
   else
   {
      szDBUser2 = szDBUser1;
   }
   if(szDBPassword1 != NULL && szDBPassword1[0] == '$')
   {
      szDBPassword2 = getenv(szDBPassword1 + 1);
   }
   else
   {
      szDBPassword2 = szDBPassword1;
   }
   // DBTable::Debug(true);
   bConnect = DBTable::Connect(szDatabase, szDBUser2, szDBPassword2);
   // DBTable::Debug(false);
   delete[] szDatabase;
   delete[] szDBUser1;
   delete[] szDBPassword1;
   if(bConnect == false)
   {
      debug("ServerLoad exit false, database connection failed\n");
      return false;
   }

   BulletinLoad();
   FolderLoad();
   TalkLoad();
   UserLoad();

   if(mask(iOptions, ICELIB_RELOAD) == true)
   {
      debug("ServerLoad checking temp section\n");

      pData->Root();

      pData->GetChild("contactid", &iID);
      ContactID(iID);

      if(pData->Child("temp") == true)
      {
         pTemp = pData->GetCurr();
         // EDFPrint("ServerLoad temp data", pData, EDFElement::EL_CURR | EDFElement::PR_SPACE);

         debug("ServerLoad reading folders\n");
         bLoop = pData->Child("folder");
         while(bLoop == true)
         {
            iParentID = -1;

            pData->Get(NULL, &iID);
            pData->GetChild("parentid", &iParentID);
            debug("ServerLoad reading folder %d, parent %d\n", iID, iParentID);

            pFolder = new MessageTreeItem("folder", pData, NULL, false);
            // debug("ServerLoad MessageTreeItem class %s\n", pFolder->GetClass());
            pFolder->SetParentID(iParentID);
            FolderAdd(pFolder);

            bLoop = pData->Next("folder");
            if(bLoop == false)
            {
               pData->Parent();
            }
         }

         debug("ServerLoad reading folder messages\n");
         bLoop = pData->Child("message");
         while(bLoop == true)
         {
            iParentID = -1;
            iFolderID = -1;

            pData->Get(NULL, &iID);
            pData->GetChild("parentid", &iParentID);
            pData->GetChild("folderid", &iFolderID);
            debug("ServerLoad reading message %d, parent %d, folder %d\n", iID, iParentID, iFolderID);

            pFolderMessage = new FolderMessageItem(pData, NULL, NULL);
            // debug("ServerLoad FolderMessageItem class %s\n", pFolderMessage->GetClass());
            pFolderMessage->SetParentID(iParentID);
            pFolderMessage->SetTreeID(iFolderID);
            FolderMessageAdd(pFolderMessage);

            bLoop = pData->Next("message");
            if(bLoop == false)
            {
               pData->Parent();
            }
         }

         FolderSetup(pData, 0, false);


         debug("ServerLoad reading channels\n");
         bLoop = pData->Child("channel");
         while(bLoop == true)
         {
            iParentID = -1;

            pData->Get(NULL, &iID);
            pData->GetChild("parentid", &iParentID);
            debug("ServerLoad reading channel %d, parent %d\n", iID, iParentID);

            pChannel = new MessageTreeItem("channel", pData, NULL, false);
            // debug("ServerLoad MessageTreeItem class %s\n", pChannel->GetClass());
            pChannel->SetParentID(iParentID);
            ChannelAdd(pChannel);

            bLoop = pData->Child("message");
            while(bLoop == true)
            {
               // iParentID = -1;
               iChannelID = -1;

               pData->Get(NULL, &iID);
               // pData->GetChild("parentid", &iParentID);
               pData->GetChild("channelid", &iChannelID);
               debug("ServerLoad reading message %d, parent %d, channel %d\n", iID, iParentID, iChannelID);

               pChannelMessage = new ChannelMessageItem(pData, NULL);
               // debug("ServerLoad ChannelMessageItem class %s\n", pChannelMessage->GetClass());
               // pChannelMessage->SetParentID(iParentID);
               // pChannelMessage->SetFolderID(iFolderID);
               // ChannelMessageAdd(pChannelMessage);
               pChannel = ChannelGet(iFolderID);
               if(pChannel != NULL)
               {
                  pChannel->MessageAdd(pChannelMessage);
                  pChannelMessage->SetTree(pChannel);
               }

               bLoop = pData->Next("message");
               if(bLoop == false)
               {
                  pData->Parent();
               }
            }

            bLoop = pData->Next("channel");
            if(bLoop == false)
            {
               pData->Parent();
            }
         }

         TalkSetup(pData, 0, false);

         pData->SetCurr(pTemp);
         debug("ServerLoad reading users\n");
         bLoop = pData->Child("user");
         while(bLoop == true)
         {
            pData->Get(NULL, &iID);
            debug("ServerLoad reading user %d\n", iID);

            pUser = new UserItem(pData);
            // debug("ServerLoad UserItem class %s\n", pUser->GetClass());
            UserAdd(pUser);

            bLoop = pData->Next("user");
            if(bLoop == false)
            {
               pData->Parent();
            }
         }

         UserSetup(pData, 0, false, 0);

         pData->SetCurr(pTemp);
         debug("ServerLoad re-attaching connections\n");
         // EDFPrint(pData, EDFElement::EL_CURR | EDFElement::PR_SPACE);
         dAttach = gettick();

         for(iConnNum = 0; iConnNum < ConnCount(); iConnNum++)
         {
            pListConn = ConnList(iConnNum);

            dReset = gettick();

            if(EDFFind(pData, "connection", pListConn->ID(), false) == true)
            // bLoop = pData->Child("connection");
            // while(bLoop == true)
            {
               // pData->Get(NULL, &iID);
               // if(iID == pListConn->ID())
               {
                  debug("ServerLoad reading connection %d\n", iID);
                  EDFParser::debugPrint(pData, EDFElement::EL_CURR | EDFElement::PR_SPACE);

                  iUserID = -1;
                  iTemp = iAttempts;

                  pData->GetChild("loginattempts", &iTemp);

                  pTemp = pData->GetCurr();
                  pListData = ConnectionSetup(pListConn, pData, iTemp);
                  pData->SetCurr(pTemp);

                  bLoop = pData->Child("announce");
                  while(bLoop == true)
                  {
                     pConnAnnounce = new ConnAnnounce;
                     pConnAnnounce->m_pAnnounce = new EDF();
                     pConnAnnounce->m_pAnnounce->Copy(pData, true, true);
                     if(pListData->m_pAnnounceFirst == NULL)
                     {
                        pListData->m_pAnnounceFirst = pConnAnnounce;
                        // printf("ServerLoad announce first %p\n", pListData->m_pAnnounceFirst);
                     }

                     bLoop = pData->Next("announce");
                     if(bLoop == false)
                     {
                        pData->Parent();
                     }
                  }

                  // EDFPrint("ServerLoad conn data", pData, EDFElement::EL_CURR | EDFElement::PR_SPACE);


                  pListData->m_pAnnounceLast = pConnAnnounce;

                  pListData->m_bClose = pData->GetChildBool("close");
                  pData->GetChild("timeon", &pListData->m_lTimeOn);
                  pData->GetChild("timeidle", &pListData->m_lTimeIdle);
                  pData->GetChild("location", &pListData->m_szLocation);
                  pData->GetChild("status", &pListData->m_iStatus);

                  pData->GetChild("userid", &iUserID);
                  if(iUserID > 0)
                  {
		     int iTempDebug = debuglevel(DEBUGLEVEL_INFO);
		     
                     pListData->m_pUser = UserGet(iUserID);
                     debug("ServerLoad user %d -> %p\n", iUserID, pListData->m_pUser);

                     pData->GetChild("timebusy", &pListData->m_lTimeBusy);
                     pData->GetChild("client", &pListData->m_szClient);
                     pData->GetChild("protocol", &pListData->m_szProtocol);
                     pData->GetChild("proxyhostname", &pListData->m_szProxy);
                     pData->GetChild("proxyaddress", &pListData->m_lProxy);
                     pData->GetChild("statusmsg", &pListData->m_pStatusMsg);
                     pData->GetChild("attachmentsize", &pListData->m_lAttachmentSize);

                     debug("ServerLoad reading folder subs\n");
                     pListData->m_pFolders = new DBSub(MessageTreeSubTable(RFG_FOLDER), MessageTreeID(RFG_FOLDER), iUserID);

                     debug("ServerLoad reading message read list\n");
                     pListData->m_pReads = new DBMessageRead(iUserID);

                     debug("ServerLoad reading message save list\n");
                     pListData->m_pSaves = new DBMessageSave(iUserID);

                     debug("ServerLoad reading channel subs\n");
                     pListData->m_pChannels = new DBSub(pData, "channel", MessageTreeSubTable(RFG_CHANNEL), MessageTreeID(RFG_CHANNEL), iUserID);

                     debug("ServerLoad reading service subs\n");
                     pListData->m_pServices = new DBSub(pData, "service");
		     
		     debuglevel(iTempDebug);
                  }

                  pListConn->Data(pListData);

                  debug("ServerLoad re-attached in %ld ms\n", tickdiff(dReset));
                  /* pConnection = new EDF();
                  ConnectionInfo(pListConn, pListData, pConnection, USERWRITE_ADMIN, false, true);
                  debugEDFPrint(pConnection, EDFElement::EL_ROOT | EDFElement::EL_CURR | EDFElement::PR_SPACE);
                  delete pConnection; */

                  // bLoop = false;
                  // pData->Parent();

                  pData->Delete();
               }
               /* else
               {
                  bLoop = pData->Next("connection");
                  if(bLoop == false)
                  {
                     pData->Parent();
                  }
               } */
            }
         }

         debug("ServerLoad connections re-attached in %ld ms\n", tickdiff(dAttach));

         debug("ServerLoad deleting temp section\n");

         pData->Delete();
      }

      pAnnounce = new EDF();
      pAnnounce->AddChild("reloadtime", lMilliseconds + tickdiff(dTick));
      ServerAnnounce(pData, MSG_SYSTEM_RELOAD, NULL, pAnnounce);
      delete pAnnounce;
   }

   debug("ServerLoad exit true\n");
   return true;
}

// Server functions
ICELIBFN bool ServerStartup(EDF *pData, int iOptions)
{
   STACKTRACE
   // int iID = 0;
   int iUptime = 0, iNumDeletes = 0, iWriteTime = -1;
   int iNumLocations = 0, iMaxLocation = 0;//, iAllLogins = 0;
   char szSQL[100];
   // bool bLoop = false;
   double dEntry = gettick();
   DBTable *pFields = NULL;
   MessageTreeItem *pChannel = NULL;

   debug("ServerStartup entry %d\n", iOptions);
   // EDFPrint("ServerStartup check point 1", pData);

   // DBTable::Debug(true);

   // memtrack(true);

   // Validate system section
   debug("ServerStartup validating system section...\n");
   // EDFPrint(pData, EDFElement::EL_CURR | EDFElement::PR_SPACE);
   if(pData->Child("system") == true)
   {
      // Setup system values
      pData->GetChild("uptime", &iUptime);
      pData->SetChild("uptime", (int)(time(NULL)));
      if(iUptime > 0)
      {
         debug("ServerStartup server down for %ld seconds\n", time(NULL) - iUptime);
         pData->SetChild("downtime", iUptime);
      }
      pData->GetChild("writetime", &iWriteTime);
      if(iWriteTime != -1)
      {
         debug("ServerStartup file written %ld seconds ago\n", time(NULL) - iWriteTime);
      }
      while(pData->DeleteChild("quit") == true)
      {
         iNumDeletes++;
      }
      while(pData->DeleteChild("reloadtime") == true)
      {
         iNumDeletes++;
      }
      while(pData->DeleteChild("requests") == true)
      {
         iNumDeletes++;
      }
      while(pData->DeleteChild("announces") == true)
      {
         iNumDeletes++;
      }
      while(pData->DeleteChild("writetime") == true)
      {
         iNumDeletes++;
      }
      while(pData->DeleteChild("mainttime") == true)
      {
         iNumDeletes++;
      }
      while(pData->DeleteChild("maintfolder") == true)
      {
         iNumDeletes++;
      }
      while(pData->DeleteChild("maintuser") == true)
      {
         iNumDeletes++;
      }
      while(pData->DeleteChild("keepmessage") == true)
      {
         iNumDeletes++;
      }

      debug("ServerStartup deleted %d elements\n", iNumDeletes);
   }
   else
   {
      debug("ServerStartup initial system section\n");

      pData->Add("system");
      pData->AddChild("uptime", (int)(time(NULL)));
      pData->AddChild("idletime", 300);
      pData->AddChild("database", "ua");

      // EDFPrint("ServerStartup initial system section", pData, false);
   }

   STACKTRACEUPDATE

   debug("ServerStartup validating database tables...\n");

   debug("ServerStartup check folder subs\n");
   pFields = new DBTable();
   pFields->BindColumnInt();
   pFields->BindColumnInt();
   sprintf(szSQL, "select max(userid), max(folderid) from %s", MessageTreeSubTable(RFG_FOLDER));
   if(pFields->Execute(szSQL) == false)
   {
      delete pFields;

      debug("ServerStartup folder_sub initial query failed\n");
      return false;
   }
   delete pFields;

   STACKTRACEUPDATE

   pFields = new DBTable();
   pFields->BindColumnInt();
   pFields->BindColumnInt();
   debug("ServerStartup check %s\n", FM_READ_TABLE);
   sprintf(szSQL, "select max(userid), max(messageid) from %s", FM_READ_TABLE);
   if(pFields->Execute(szSQL) == false)
   {
      delete pFields;

      debug("ServerStartup message_read initial query failed\n");
      return false;
   }
   delete pFields;

   STACKTRACEUPDATE

   debug("ServerStartup check channel subs\n");
   pFields = new DBTable();
   pFields->BindColumnInt();
   pFields->BindColumnInt();
   sprintf(szSQL, "select max(userid), max(channelid) from %s", MessageTreeSubTable(RFG_CHANNEL));
   if(pFields->Execute(szSQL) == false)
   {
      delete pFields;

      debug("ServerStartup channel_sub initial query failed\n");
      return false;
   }
   delete pFields;

   STACKTRACEUPDATE

   // EDFPrint("ServerStartup check point 3", pData);

   // Validate locations section
   debug("ServerStartup validating locations section...\n");
   pData->Root();
   if(pData->Child("locations") == false)
   {
      debug("ServerStartup adding locations section\n");
      pData->Add("locations");
      // pData->SetChild("numlocations", 0);
   }
   pData->Parent();

   /* pData->GetChild("maxid", &iMaxLocation);
   bLoop = pData->Child("location");
   while(bLoop == true)
   {
      iID = 0;
      iNumLocations++;
      pData->Get(NULL, &iID);
      if(iID > iMaxLocation)
      {
         iMaxLocation = iID;
      }

      bLoop = pData->Iterate("location", "locations");
   }
   pData->SetChild("numlocations", iNumLocations);
   pData->SetChild("maxid", iMaxLocation); */
   iNumLocations = pData->Children("location", true);
   iMaxLocation = EDFMax(pData, "location", true);
   debug("ServerStartup locations: %d total, max ID %d\n", iNumLocations, iMaxLocation);

   STACKTRACEUPDATE

   // Validate services section
   debug("ServerStartup validating services section...\n");
   pData->Root();
   if(pData->Child("services") == false)
   {
      debug("ServerStartup adding services section\n");
      pData->Add("services");
      // pData->SetChild("numservices", 0);
   }
   pData->Parent();

   STACKTRACEUPDATE

   // EDFPrint("ServerStartup check point 4", pData);

   // FolderStartup();
   // UserStartup();

   // EDFPrint("ServerStartup check point 5", pData);

   // Validate folders section
   debug("ServerStartup validating folders section...\n");
   pData->Root();
   if(pData->Child("folders") == false)
   {
      pData->Add("folders");
   }

   // DBTable::Lock("folder_item");
   FolderReadDB(pData);
   // DBTable::Unlock();

   FolderSetup(pData, iOptions, true);

   STACKTRACEUPDATE

   // Validate channels section
   debug("ServerStartup validating channels section...\n");
   pData->Root();
   if(pData->Child("channels") == false)
   {
      pData->Add("channels");
   }

   // DBTable::Lock("channel_item");
   TalkReadDB(pData);
   // DBTable::Unlock();

   if(ChannelCount() == 0)
   {
      // No channels - create a default one
      debug("ServerStartup creating default channel\n");

      pChannel = new MessageTreeItem("channel", 1, NULL, true);
      // debug("ServerStartup MessageTreeItem class %s\n", pChannel->GetClass());
      pChannel->SetName("Talk");
      pChannel->SetAccessMode(CHANNELMODE_NORMAL);
      ChannelAdd(pChannel);

      pChannel = new MessageTreeItem("channel", 2, NULL, true);
      // debug("ServerStartup MessageTreeItem class %s\n", pChannel->GetClass());
      pChannel->SetName("Whisper");
      pChannel->SetAccessMode(CHANNELMODE_RESTRICTED);
      ChannelAdd(pChannel);

      pChannel = new MessageTreeItem("channel", 3, NULL, true);
      // debug("ServerStartup MessageTreeItem class %s\n", pChannel->GetClass());
      pChannel->SetName("Debate");
      pChannel->SetAccessMode(CHANNELMODE_DEBATE);
      ChannelAdd(pChannel);
   }

   TalkSetup(pData, iOptions, true);

   STACKTRACEUPDATE

   // Validate users section
   debug("ServerStartup validating users section\n");

   pData->Root();
   if(pData->Child("users") == false)
   {
      pData->Add("users");
   }

   // DBTable::Lock("user_item");
   UserReadDB(pData);
   // DBTable::Unlock();

   if(UserCount() == 0)
   {
      // No users - create a default one
      debug("ServerStartup creating default sysop\n");

      UserAdd("SysOp", "sysop", LEVEL_SYSOP);
   }

   // EDFPrint("ServerStartup check point 7", pData);

   UserSetup(pData, iOptions, true, iWriteTime);

   // pData->Root();
   // pData->Child("users");
   // pData->SetChild("numusers", UserCount());
   // pData->SetChild("maxid", iMaxUser);
   // pData->SetChild("totallogins", iAllLogins);
   // printf("UserStartup users: total %d, max ID %ld, logins %d\n", UserCount(), UserMaxID(), iAllLogins);

   STACKTRACEUPDATE

   // EDFPrint("ServerStartup check point 8", pData);

   debug("ServerStartup exit true, %ld ms\n", tickdiff(dEntry));
   return true;
}

// Server has been shutdown by the main programm
ICELIBFN bool ServerShutdown(EDF *pData)
{
   STACKTRACE

   debug("ServerShutdown entry\n");

   FolderWriteDB(false);

   TalkWriteDB(false);

   UserWriteDB(false);

   pData->Root();
   pData->Child("system");
   // pData->DeleteChild("quit");
   pData->SetChild("downtime", (int)(time(NULL)));

   // DBDisconnect();

   // memstat();

   debug("ServerShutdown exit\n");
   return false;
}

// Server library unloaded
ICELIBFN bool ServerUnload(EDF *pData, int iOptions)
{
   STACKTRACE
   bool bDelete = false;
   int iConnNum = 0, iFolderNum = 0, iMessageNum = 0, iChannelNum = 0, iUserNum = 0, iOtherNum = 0;
   ConnData *pListData = NULL, *pOtherData = NULL;
   ConnAnnounce *pConnAnnounce = NULL, *pTemp = NULL;
   EDFConn *pListConn = NULL, *pOtherConn = NULL;
   UserItem *pUser = NULL;
   MessageTreeItem *pFolder = NULL, *pChannel = NULL;
   FolderMessageItem *pFolderMessage = NULL;
   ChannelMessageItem *pChannelMessage = NULL;

   debug("ServerUnload entry %d\n", iOptions);

   // bDebug = DBTable::Debug(true);

   if(mask(iOptions, ICELIB_RELOAD) == true)
   {
      pData->Root();
      while(pData->DeleteChild("temp") == true);
      pData->Add("temp");

      pData->SetChild("contactid", ContactID());

      debug("ServerUnload storing temp conns\n");

      for(iConnNum = 0; iConnNum < ConnCount(); iConnNum++)
      {
         pListConn = ConnList(iConnNum);
         debug("ServerUnload conn %d, %p %ld\n", iConnNum, pListConn, pListConn->ID());

         pListData = (ConnData *)pListConn->Data();
         debug("ServerUnload writing conn %p %p\n", pListData, pListData->m_pUser);

         pData->Add("connection", pListConn->ID());

         pConnAnnounce = pListData->m_pAnnounceFirst;
         while(pConnAnnounce != NULL)
         {
            pData->Copy(pConnAnnounce->m_pAnnounce);
            delete pConnAnnounce->m_pAnnounce;

            pTemp = pConnAnnounce;
            pConnAnnounce = pConnAnnounce->m_pNext;
            delete pTemp;
         }

         pListData->m_pAnnounceFirst = NULL;
         // printf("ServerUnload announce first %p\n", pListData->m_pAnnounceFirst);
         pListData->m_pAnnounceLast = NULL;

         pData->AddChild("close", pListData->m_bClose);
         pData->AddChild("timeon", pListData->m_lTimeOn);
         if(pListData->m_lTimeIdle != -1)
         {
            pData->AddChild("timeidle", pListData->m_lTimeIdle);
         }
         if(pListData->m_iStatus > 0)
         {
            pData->AddChild("status", pListData->m_iStatus);
         }

         if(pListData->m_pUser != NULL)
         {
            pData->AddChild("userid", pListData->m_pUser->GetID());
            if(mask(pListData->m_iStatus, LOGIN_BUSY) == true)
            {
               pData->AddChild("timebusy", pListData->m_lTimeBusy);
            }
            if(pListData->m_szLocation != NULL)
            {
               pData->AddChild("location", pListData->m_szLocation);
            }
            if(pListData->m_szClient != NULL)
            {
               pData->AddChild("client", pListData->m_szClient);
            }
            if(pListData->m_szProtocol != NULL)
            {
               pData->AddChild("protocol", pListData->m_szProtocol);
            }
            if(pListData->m_szProxy != NULL)
            {
               pData->AddChild("proxyhostname", pListData->m_szProxy);
            }
            if(pListData->m_lProxy > 0)
            {
               pData->AddChild("proxyaddress", (double)pListData->m_lProxy);
            }
            if(pListData->m_pStatusMsg != NULL)
            {
               pData->AddChild("statusmsg", pListData->m_pStatusMsg);
            }
            pData->AddChild("attachmentsize", pListData->m_lAttachmentSize);

            // Find other connections by this user
            debug("ServerUnload find other connections\n");
            bDelete = true;
            iOtherNum = 0;
            while(iOtherNum < ConnCount() && bDelete == true)
            {
               pOtherConn = ConnList(iOtherNum);
               pOtherData = (ConnData *)pOtherConn->Data();

               debug("ServerUnload conn list %d %p %p\n", iOtherNum, pOtherConn, pOtherData);

               if(pOtherData != NULL && pOtherData->m_pUser != NULL && pOtherData->m_pUser->GetID() == pListData->m_pUser->GetID() && pOtherConn->ID() != pListConn->ID())
               {
                  debug("ServerUnload found other connection\n");
                  bDelete = false;
               }
               else
               {
                  iOtherNum++;
               }
            }

            if(bDelete == true)
            {
               debug("ServerUnload writing and deleting conn data for %s\n", pListData->m_pUser->GetName());

               debug("ServerUnload writing folder subs %p\n", pListData->m_pFolders);
               pListData->m_pFolders->Write();
               debug("ServerUnload deleting folder subs\n");
               delete pListData->m_pFolders;
               pListData->m_pFolders = NULL;

               debug("ServerUnload writing message read list %p\n", pListData->m_pReads);
               pListData->m_pReads->Write(true);
               debug("ServerUnload deleting message read list\n");
               delete pListData->m_pReads;
               pListData->m_pReads = NULL;

               debug("ServerUnload writing message save list %p\n", pListData->m_pSaves);
               pListData->m_pSaves->Write(true);
               debug("ServerUnload deleting message save list\n");
               delete pListData->m_pSaves;
               pListData->m_pSaves = NULL;
            }

            // printf("ServerUnload writing channels %p\n", pListData->m_pChannels);
            // pListData->m_pChannels->Write(true);
            debug("ServerUnload storing channels\n");
            pListData->m_pChannels->Write(pData, "channel");
            debug("ServerUnload deleting channels\n");
            delete pListData->m_pChannels;
            pListData->m_pChannels = NULL;

            debug("ServerUnload storing services\n");
            pListData->m_pServices->Write(pData, "service");
            debug("ServerUnload deleting services\n");
            delete pListData->m_pServices;
            pListData->m_pServices = NULL;

            debug("ServiceUnload login data removed\n");
         }
         else
         {
            pData->AddChild("loginattempts", pListData->m_iLoginAttempts);
         }

         delete pListData;
         pListConn->Data(NULL);

         pData->Parent();
      }

      debug("ServerUnload storing temp folders\n");
      for(iFolderNum = 0; iFolderNum < FolderCount(); iFolderNum++)
      {
         pFolder = FolderList(iFolderNum);

         debug("ServerUnload writing folder %d, %p %ld %s\n", iFolderNum, pFolder, pFolder->GetID(), pFolder->GetName());

         pFolder->Write(pData, -1);

         pData->Parent();
      }

      debug("ServerUnload storing temp folder messages\n");
      for(iMessageNum = 0; iMessageNum < FolderMessageCount(); iMessageNum++)
      {
         pFolderMessage = FolderMessageList(iMessageNum);

         debug("ServerUnload writing message %d, %p %ld\n", iMessageNum, pFolderMessage, pFolderMessage->GetID());

         pFolderMessage->Write(pData, -1);

         if(pFolderMessage->GetType() > 0)
         {
            EDFParser::debugPrint("ServerUnload message", pData, EDFElement::EL_CURR | EDFElement::PR_SPACE);

         }

         pData->Parent();
      }

      debug("ServerUnload storing temp channels\n");
      for(iChannelNum = 0; iChannelNum < ChannelCount(); iChannelNum++)
      {
         pChannel = ChannelList(iChannelNum);

         debug("ServerUnload writing channel %d, %p %ld %s\n", iChannelNum, pChannel, pChannel->GetID(), pChannel->GetName());

         pChannel->Write(pData, -1);

         for(iMessageNum = 0; iMessageNum < pChannel->MessageCount(); iMessageNum++)
         {
            pChannelMessage = (ChannelMessageItem *)pChannel->MessageChild(iMessageNum);

            debug("ServerUnload writing message %d, %p %ld\n", iMessageNum, pChannelMessage, pChannelMessage->GetID());

            pChannelMessage->Write(pData, -1);

            if(pChannelMessage->GetType() > 0)
            {
               EDFParser::debugPrint("ServerUnload message", pData, EDFElement::EL_CURR | EDFElement::PR_SPACE);
            }

            pData->Set("message", pChannelMessage->GetID());
            pData->Parent();
         }

         pData->Parent();
      }

      debug("ServerUnload storing temp users\n");
      for(iUserNum = 0; iUserNum < UserCount(); iUserNum++)
      {
         pUser = UserList(iUserNum);

         debug("ServerUnload writing user %d, %p %ld %s\n", iUserNum, pUser, pUser->GetID(), pUser->GetName());

         pUser->Write(pData, -1, NULL, NULL);

         pData->Parent();
      }

      // EDFPrint("ServerUnload temp data", pData, EDFElement::EL_CURR | EDFElement::PR_SPACE);

      pData->Parent();
   }

   debug("ServerUnload unloading bulletins\n");
   BulletinUnload();

   debug("ServerUnload unloading folders\n");
   FolderUnload();

   debug("ServerUnload unloading talk\n");
   TalkUnload();

   debug("ServerUnload unloading users\n");
   UserUnload();

   debug("ServerUnload disconnecting from database\n");
   DBTable::Disconnect();

   // DBTable::Debug(bDebug);

   debug("ServerUnload exit true\n");
   return true;
}

// Called after data is written to file
ICELIBFN bool ServerWrite(EDF *pData, long lMilliseconds)
{
   debug("ServerWrite in %ld ms\n", lMilliseconds);

   m_iWriteTime = time(NULL);

   EDF *pAnnounce = new EDF();
   pAnnounce->AddChild("writetime", lMilliseconds);
   ServerAnnounce(pData, MSG_SYSTEM_WRITE, NULL, pAnnounce);
   delete pAnnounce;

   return true;
}

// Background processing for server-wide functions
ICELIBFN int ServerBackground(EDF *pData)
{
   STACKTRACE
   int iReturn = 0, iTime = time(NULL), iConnNum = 0, iQuit = 0, iNextTime = 0, iRepeat = 0, iMinute = 0, iHour = 0, iDay = 0, iValue = 0;
   bool bLoop = false;
   long lWriteTime = 0, lSingleTime = 0;
   double dTick = 0;
   char *szRequest = NULL;
   EDF *pIn = NULL, *pOut = NULL;
   EDFElement *pElement = NULL;
   EDFConn *pListConn = NULL;
   ConnData *pListData = NULL;

   // printf("ServerBackground entry\n");

   pData->Root();
   pData->Child("system");
   // EDFPrint("ServerBackground system section", pData, false, false);
   // iUserMaints = pData->Children("maintuser");

   // printf("ServerBackground write time %d -> %d = %d\n", m_iWriteTime, iTime, iTime - m_iWriteTime);
   if(pData->Child("quit") == true)
   {
      pData->Get(NULL, &iQuit);
      debug("ServerBackground quit check %ld > %d?\n", time(NULL), iQuit);
      if(time(NULL) > iQuit)
      {
         iReturn += ICELIB_QUIT;
      }
      pData->Parent();
   }

   if(iTime - m_iWriteTime > 600 || pData->GetChildBool("write") == true)
   {
      debug("ServerBackground write time %d\n", iTime);
      fflush(stdout);

      m_iWriteTime = iTime;
      // pData->SetChild("writetime", iTime);
      // m_dTick = gettick();

      iReturn |= ICELIB_WRITE;
      pData->DeleteChild("write");

      /* pData->GetChild("restarttime", &iRestartTime);
      if(iRestartTime > iTime)
      {
         debug("ServerBackground requesting restart\n");
         iReturn += SERVERBG_RESTART;
      } */

      // DBTable::Debug(true);

      FolderWriteDB(false);

      TalkWriteDB(false);

      UserWriteDB(false);

      // DBTable::Debug(false);

      // Just in case
      DBTable::Unlock();
      DBTable::Lock(FM_READ_TABLE);
      DBTable::Lock(FM_SAVE_TABLE);
      DBTable::Lock(MessageTreeSubTable(RFG_FOLDER));
      DBTable::Lock(MessageTreeSubTable(RFG_CHANNEL));

      dTick = gettick();
      for(iConnNum = 0; iConnNum < ConnCount(); iConnNum++)
      {
         lSingleTime = 0;

         pListConn = ConnList(iConnNum);
         pListData = (ConnData *)pListConn->Data();

         if(pListData->m_pFolders != NULL)
         {
            lSingleTime += pListData->m_pFolders->Write(false);
         }

         if(pListData->m_pReads != NULL)
         {
            lSingleTime += pListData->m_pReads->Write(false);
         }

         if(pListData->m_pSaves != NULL)
         {
            lSingleTime += pListData->m_pSaves->Write(false);
         }

         if(pListData->m_pChannels != NULL)
         {
            lSingleTime += pListData->m_pChannels->Write(false);
         }

         if(pListData->m_pServices != NULL)
         {
            lSingleTime += pListData->m_pServices->Write(false);
         }

         lWriteTime += lSingleTime;
      }

      DBTable::Unlock();

      debug("ServerBackground %s / %s / %s / %s write %ld ms (DBFolder %ld ms)\n", FM_READ_TABLE, MessageTreeSubTable(RFG_FOLDER), MessageTreeSubTable(RFG_CHANNEL), SERVICE_SUB, tickdiff(dTick), lWriteTime);
   }

   if(time(NULL) > m_iTaskTime + 30)
   {
      // printf("ServerBackground task check %ld\n", time(NULL));

      pData->Root();
      pData->Child("system");
      bLoop = pData->Child("task");
      while(bLoop == true)
      {
         if(pData->GetChildBool("done") == false)
         {
            pData->GetChild("nexttime", &iNextTime);

            if(time(NULL) > iNextTime)
            {
               pData->GetChild("repeat", &iRepeat);

               if(pData->Child("request") == true)
               {
                  szRequest = NULL;
                  pData->Get(NULL, &szRequest);

                  debug("ServerBackground %d %s\n", iNextTime, szRequest);

                  if(szRequest != NULL)
                  {
                     pIn = new EDF();
                     pIn->Copy(pData, true, true);
                     pIn->Set("request", szRequest);
                     EDFParser::debugPrint("ServerBackground input", pIn);

                     pOut = new EDF();

                     pElement = pData->GetCurr();

                     if(stricmp(szRequest, MSG_SYSTEM_EDIT) == 0)
                     {
                        SystemEdit(NULL, pData, pIn, pOut);
                     }
                     else if(stricmp(szRequest, MSG_SYSTEM_MAINTENANCE) == 0)
                     {
                        SystemMaintenance(NULL, pData, pIn, pOut);
                     }
                     else
                     {
                        debug("ServerBackground cannot process %s request\n", szRequest);
                     }

                     EDFParser::debugPrint("ServerBackground output", pOut);

                     pData->SetCurr(pElement);

                     delete[] szRequest;
                  }

                  delete pIn;
                  delete pOut;

                  pData->Parent();
               }

               if(iRepeat == 0)
               {
                  pData->SetChild("done", true);
               }
               else
               {
                  if(iRepeat < TASK_DAILY || iRepeat > TASK_WEEKLY)
                  {
                     debug("ServerBackground task repeat reset from %d\n", iRepeat);
                     iRepeat = TASK_DAILY;
                  }

                  pData->GetChild("minute", &iMinute);
                  pData->GetChild("hour", &iHour);
                  debug("ServerBackground reset repeat %d %d %d\n", iHour, iMinute, iRepeat);
                  if(iRepeat == TASK_WEEKLY && pData->GetChild("day", &iDay) == true)
                  {
                     if(iDay < 0 || iDay >= 7)
                     {
                        iDay = 0;
                     }
                  }

                  iValue = TaskNext(iHour, iMinute, iDay, iRepeat);
                  debug("ServerBackground next time %d -> %d\n", iNextTime, iValue);
                  pData->SetChild("nexttime", iValue);
               }
            }
         }

         bLoop = pData->Next("task");
         if(bLoop == false)
         {
            pData->Parent();
         }
      }

      m_iTaskTime = time(NULL);
   }

   // printf("ServerBackground exit %d\n", iReturn);
   return iReturn;
}


// Connection functions
ICELIBFN bool ConnectionOpen(EDFConn *pConn, EDF *pData)
{
   STACKTRACE
   int iAttempts = 3, iMaxConns = 0;
   ConnData *pConnData = NULL;
   EDF *pAdmin = NULL;
   EDFElement *pTemp = NULL;

   debug("ConnectionOpen entry %p %s %lu\n", pConn, pConn->Hostname(), pConn->Address());

   pData->Root();
   pData->Child("system");
   pData->GetChild("loginattempts", &iAttempts);
   pData->GetChild("maxconns", &iMaxConns);

   pTemp = pData->GetCurr();
   pConnData = ConnectionSetup(pConn, pData, iAttempts);
   pData->SetCurr(pTemp);

   pConn->Data((void *)pConnData);

   // Check the connection isn't from a banned site
   if(pData->Child("connection") == true)
   {
      // EDFPrint("ConnectionOpen valid check", pData, false);
      if(ConnectionValid(pConn, pData) == false)
      {
         pAdmin = new EDF();
         ConnectionInfo(pAdmin, pConn, EDFITEMWRITE_ADMIN + USERITEMWRITE_LOGIN + USERITEMWRITE_LOGINFLAT, false);
         ServerAnnounce(pData, MSG_CONNECTION_DENIED, NULL, pAdmin);
         delete pAdmin;

         debug("ConnectionOpen exit false, hostname / address blocked\n");
         return false;
      }
      pData->Parent();
   }

   pAdmin = new EDF();
   ConnectionInfo(pAdmin, pConn, EDFITEMWRITE_ADMIN + USERITEMWRITE_LOGIN + USERITEMWRITE_LOGINFLAT, false);
   ServerAnnounce(pData, MSG_CONNECTION_OPEN, NULL, pAdmin);
   delete pAdmin;

   if(ConnCount() > iMaxConns)
   {
      pData->Root();
      pData->Child("system");
      pData->SetChild("maxconns", ConnCount());
   }

   debug("ConnectionOpen exit true\n");
   return true;
}

// Connection is being closed
ICELIBFN bool ConnectionClose(EDFConn *pConn, EDF *pData)
{
   return ConnectionClose(pConn, pData, false);
}

// Connection was lost
ICELIBFN bool ConnectionLost(EDFConn *pConn, EDF *pData)
{
   return ConnectionClose(pConn, pData, true);
}

// Connection was lost
ICELIBFN bool ConnectionTimeout(EDFConn *pConn, EDF *pData)
{
   STACKTRACE
   ConnData *pConnData = CONNDATA;

   // printf("ConnectionTimeout entry %p %p\n", pConn, pData);

   if(pConnData == NULL)
   {
      debug("ConnetionTimeout no conn data\n");
      return true;
   }

   // printf("ConnectionTimeout announce c=%p d=%p f=%p\n", pConn, pConnData, pConnData->m_pAnnounceFirst);

   if(pConnData->m_pAnnounceFirst != NULL)
   {
      // printf("ConnectionTimeout false, announcements %p\n", pConnData->m_pAnnounceFirst);
      return false;
   }

   // printf("ConnectionTimeout exit true\n");
   return true;
}

// Verfiy request is valid for access level / login status
ICELIBFN bool ConnectionRequest(EDFConn *pConn, EDF *pData, char *szRequest, EDF *pIn, int iScope)
{
   STACKTRACE
   int iUserID = -1, iUserType = 0, iAccessLevel = LEVEL_NONE, iRequests = 0; //, iStatus = LOGIN_OFF;
   bool bSuccess = false;
   ConnData *pConnData = CONNDATA;
   // EDF *pConnection = CONNCONNECTION;
   UserItem *pCurr = CONNUSER;

   // printf("ConnRequest %s %d, %d %p\n", szRequest, iScope, pConn->State(), pConn->Data());

   if(pCurr != NULL)
   {

      iUserID = pCurr->GetID();
      iAccessLevel = pCurr->GetAccessLevel();
      iUserType = pCurr->GetUserType();
      // iStatus = pCurr->GetStatus();
      // pCurr->SetTimeIdle(time(NULL));
   }
   /* else if(pConnection != NULL)
   {
      pConnection->SetChild("timeidle", time(NULL));
   } */
   if(pIn->GetChildBool("idlereset", true) == true)
   {
      // printf("ConnectionRequest resetting idle time to %d\n", time(NULL));

      pConnData->m_lTimeIdle = time(NULL);
      if(pCurr != NULL && mask(pConnData->m_iStatus, LOGIN_SHADOW) == false)
      {
         pCurr->SetTimeIdle(time(NULL));
      }
   }
   else
   {
      debug("ConnectionRequest not resetting idle time\n");
   }

   // printf("ConnectionRequest entry %d, %d %d\n", iScope, iAccessLevel, iStatus);

   // Scope is the setting used in the config file
   if(iScope == NOSCOPE ||
      (iScope == USEROFF && mask(pConnData->m_iStatus, LOGIN_ON) == false) ||
      (iScope == USERON && mask(pConnData->m_iStatus, LOGIN_ON) == true) ||
      (iScope == USERNORMAL && (iAccessLevel >= LEVEL_MESSAGES || iUserType == USERTYPE_AGENT)) ||
      (iScope == USERADMIN && iAccessLevel >= LEVEL_WITNESS)
      )
   {
      bSuccess = true;

     pConnData->m_iRequests++;

      pData->Root();
      pData->Child("system");
      pData->GetChild("requests", &iRequests);
      pData->SetChild("requests", iRequests + 1);
   }
   else
   {
      debug("ConnectionRequest %s failed (c=%p u=%d s=%d, a=%d, t=%d)\n", szRequest, pCurr, iUserID, pConnData->m_iStatus, iAccessLevel, iUserType);
   }

   // printf("ConnectionRequest exit %s\n", BoolStr(bSuccess));
   return bSuccess;
}

// Background processing related to this connection only
ICELIBFN int ConnectionBackground(EDFConn *pConn, EDF *pData)
{
   STACKTRACE
   int iNumAnnounces = 0;
   ConnData *pConnData = CONNDATA;

   if(pConnData != NULL)
   {
      // printf("ConnectionBackground %p %d %p\n", pConn, pConn->State(), pConnData);

      if(pConn->State() == Conn::OPEN)
      {
         // pConnData = CONNDATA;
         while(pConnData != NULL && pConnData->m_pAnnounceFirst != NULL && iNumAnnounces < 10)
         {
            // printf("ConnectionBackground announce first %p\n", pConnData->m_pAnnounceFirst);
            ConnectionAnnounce(pConn, pConnData, pData);
            iNumAnnounces++;
         }

		 if(pConnData->m_pUser == NULL && UserItem::IsIdle(pConnData->m_lTimeOn) && pConnData->m_bClose == false)
		 {
			 ConnectionShut(pConn, pData, 0, 0, NULL, NULL, false);
		 }
      }

      // printf("ConnectionBackground close %s\n", BoolStr(pConnData->m_bClose));
      if(pConnData->m_bClose == true)
      {
         debug("ConnectionBackground close\n");
         return ICELIB_CLOSE;
      }
   }
   else
   {
      debug("ConnectionBackground no connection data\n");
   }

   return 0;
}

}
