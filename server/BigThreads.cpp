#include <stdio.h>
#include <stdlib.h>

#include <map>
#include <list>
using namespace std;

#include "useful/useful.h"
#include "db/DBTable.h"

class Message
{
public:
   int m_iID;
   int m_iFromID;
   char *m_szSubject;
   int m_iCount;
   int m_iSize;

   Message()
   {
      // printf("Message::Message\n");

      m_iID = -1;
      m_iFromID = -1;
      m_szSubject = NULL;
      m_iCount = 0;
      m_iSize = 0;
   }

   ~Message()
   {
      // printf("Message::~Message, %d\n", m_iID);

      delete[] m_szSubject;
   }

   void operator=(const Message &pMessage)
   {
      delete[] m_szSubject;
      
      m_iID = pMessage.m_iID;
      m_iFromID = pMessage.m_iFromID;
      m_szSubject = strmk(pMessage.m_szSubject);
      m_iCount = pMessage.m_iCount;
      m_iSize = pMessage.m_iSize;
   }
};

#define SORT_COUNT 1
#define SORT_SIZE 2

int g_iSortType = 0;

map<int, Message> g_pMessages;

int SortThreads(const void *p1, const void *p2)
{
   int iReturn = 0;
   Message *pMessage1 = (Message *)p1, *pMessage2 = (Message *)p2;

   switch(g_iSortType)
   {
      case SORT_COUNT:
         if(pMessage1->m_iCount < pMessage2->m_iCount)
         {
            iReturn = 1;
         }
         else if(pMessage1->m_iCount > pMessage2->m_iCount)
         {
            iReturn = -1;
         }
         break;

      case SORT_SIZE:
         if(pMessage1->m_iSize < pMessage2->m_iSize)
         {
            iReturn = -1;
         }
         else if(pMessage1->m_iSize > pMessage2->m_iSize)
         {
            iReturn = 1;
         }
         break;
   }

   // printf("SortThreads %d(%d) %d(%d) -> %d\n", pMessage1->m_iSpeed, pMessage1->m_iID, pMessage2->m_iSpeed, pMessage2->m_iID, iReturn);

   return iReturn;
}

Message *MessageGet(int iMessageID, bool bCreate = true)
{
   map<int, Message>::const_iterator pMessage;
   Message *pReturn = NULL;

   pMessage = g_pMessages.find(iMessageID);
   if(pMessage == g_pMessages.end() && bCreate == true)
   {
      // Create
      g_pMessages[iMessageID].m_iID = iMessageID;
      pMessage = g_pMessages.find(iMessageID);
   }

   if(pMessage != g_pMessages.end())
   {
      pReturn = (Message *)&pMessage->second;
   }
   return pReturn;
}

bool BigThread(int iParentID, int iThreadID)
{
   int iNumMessages = 0, iMessageID = -1, iLength = 0;
   double dTick = gettick();
   char szSQL[200];
   DBTable *pMessages = NULL;
   Message *pMessage = NULL;
   list<Message *> pList;
   
   // printf("BigThread entry %d %d\n", iParentID, iThreadID);
   
   pMessages = new DBTable();
   pMessages->BindColumnInt();
   pMessages->BindColumnInt();
   
   sprintf(szSQL, "select messageid,length(message_text) from folder_message_item where parentid=%d", iParentID);
   // printf("BigThread running '%s'\n", szSQL);
   if(pMessages->Execute(szSQL) == false)
   {
      printf("BigThread exit false, query '%s' failed\n", szSQL);
      
      delete pMessages;
	
      return false;
   }
   // printf("BigThread query run (%ld ms)\n", tickdiff(dTick));
   
   while(pMessages->NextRow() == true)
   {
      iNumMessages++;
				
      pMessages->GetField(0, &iMessageID);
      pMessages->GetField(1, &iLength);
      // printf("BigThread %d %d\n", iMessageID, iLength);
      
      pMessage = new Message();
      pMessage->m_iID = iMessageID;
      pMessage->m_iSize = iLength;
      
      pList.push_back(pMessage);
      
      pMessage = MessageGet(iThreadID, false);
      // printf("BigThread %p\n", pMessage);
      pMessage->m_iCount++;
      pMessage->m_iSize += iLength;
   }
   
   delete pMessages;
   
   while(pList.empty() == false)
   {
      pMessage = pList.front();
      BigThread(pMessage->m_iID, iThreadID);
      
      pList.pop_front();
   }
   
   // printf("BigThread exit true, %d messages\n", iNumMessages);
   return true;
}

bool BigThreads(int iFolderID, int iLimit)
{
   STACKTRACE
   int iNumThreads = 0, iThreadNum = 0, iMessageID = -1, iFromID = -1;
   double dTick = gettick();
   char *szSubject = NULL;
   char szSQL[200];
   DBTable *pThreads = NULL;
   Message *pMessage = NULL, *pThreadList = NULL;
   map<int, Message>::const_iterator pThreadIter;
   
   printf("BigThreads entry %d\n", iFolderID);
   
   pThreads = new DBTable();
   pThreads->BindColumnInt();
   pThreads->BindColumnInt();
   pThreads->BindColumnBytes();

   sprintf(szSQL, "select messageid,fromid,subject from folder_message_item where parentid=-1");
   if(iFolderID != -1)
   {
      sprintf(szSQL, "%s and folderid=%d", szSQL, iFolderID);
   }
   else
   {
      strcat(szSQL, " and folderid <> 384");
   }
   strcat(szSQL, " order by messageid");

   if(pThreads->Execute(szSQL) == false)
   {
      delete pThreads;

      printf("BigThreads exit false, query '%s' failed\n", szSQL);
      return false;
   }
   printf("BigThreads query run (%ld ms)\n", tickdiff(dTick));

   while(pThreads->NextRow() == true)
   {
      if(iNumThreads > 0 && iNumThreads % 100 == 0)
      {
	 printf("BigThreads querying %dth thread\n", iNumThreads);
      }
	   
      iNumThreads++;
      
      pThreads->GetField(0, &iMessageID);
      pThreads->GetField(1, &iFromID);
      pThreads->GetField(2, &szSubject);
      
      // printf("BigThreads query message %d %s\n", iMessageID, szSubject);
      
      pMessage = MessageGet(iMessageID, true);
      pMessage->m_iFromID = iFromID;
      pMessage->m_szSubject = strmk(szSubject);
      
      szSubject = NULL;
   }

   delete pThreads;
   
   printf("BigThreads %d threads\n", iNumThreads);
   
   pThreadList = new Message[iNumThreads];

   pThreadIter = g_pMessages.begin();
   while(pThreadIter != g_pMessages.end())
   {
      if(iThreadNum > 0 && iThreadNum % 100 == 0)
      {
	 printf("BigThreads processing %dth of %d threads\n", iThreadNum, iNumThreads);
      }
      
      pMessage = (Message *)&pThreadIter->second;
      
      BigThread(pMessage->m_iID, pMessage->m_iID);

      pThreadList[iThreadNum] = *pMessage;
      
      pThreadIter++;
      iThreadNum++;
   }
   
   g_iSortType = SORT_COUNT;
   qsort(pThreadList, iNumThreads, sizeof(Message), SortThreads);
   
   if(iLimit == 0 || iLimit > iNumThreads)
   {
      iLimit = iNumThreads;
   }
   
   printf("BigThreads biggest %d threads:\n", iLimit);
   
   for(iThreadNum = 0; iThreadNum < iLimit; iThreadNum++)
   {
      pMessage = &pThreadList[iThreadNum];
	
      printf("BigThreads thread %d(from %d, %s), %d messages (total size %d bytes)\n", pMessage->m_iID, pMessage->m_iFromID, pMessage->m_szSubject, pMessage->m_iCount, pMessage->m_iSize);
   }
   
   printf("BigThreads exit true, %d threads\n", iNumThreads);
   return true;
}

int main(int argc, char **argv)
{
   int iFolderID = 0, iLimit = 0;

   // debuglevel(DEBUGLEVEL_DEBUG);

   if(argc != 6)
   {
      printf("Usage: BigThreads <database> <username> <password> <folderid> <limit>\n");

      return 1;
   }

   if(DBTable::Connect(argv[1], argv[2], argv[3]) == false)
   {
      printf("Cannot connect to database\n");

      return 1;
   }

   iFolderID = atoi(argv[4]);
   iLimit = atoi(argv[5]);

   if(BigThreads(iFolderID, iLimit) == false)
   {
      printf("Cannot get threads data\n");

      DBTable::Disconnect();

      return 1;
   }

   DBTable::Disconnect();

   printf("\n");

   // ThreadStats();

   return 0;
}
