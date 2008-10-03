#include <stdio.h>
#include <stdlib.h>

#include <map>
using namespace std;

#include "headers.h"

class User
{
public:
   int m_iID;
   char *m_szName;
   int m_iNumThreads;
   int m_iTotalSize;
   int m_iLargestID;
   float m_fAverage;

   User()
   {
      // printf("User::User\n");

      m_iID = 0;
      m_szName = NULL;
   }

   ~User()
   {
      /* if(m_iID > 0)
      {
         printf("User::~User, %d\n", m_iID);
      } */

      delete[] m_szName;
   }

   void operator=(const User &pUser)
   {
      m_iID = pUser.m_iID;
      m_szName = strmk(pUser.m_szName);
      m_iNumThreads = pUser.m_iNumThreads;
      m_iTotalSize = pUser.m_iTotalSize;
      m_iLargestID = pUser.m_iLargestID;
      m_fAverage = pUser.m_fAverage;
   }
};

class Message
{
public:
   int m_iID;
   int m_iThreadID;
   int m_iDate;
   int m_iFromID;
   char *m_szSubject;
   int m_iSize;
   int m_iSpeed;
   float m_fAverage;

   Message()
   {
      // printf("Message::Message\n");

      m_iID = 0;
      m_szSubject = NULL;
   }

   ~Message()
   {
      /* if(m_iID > 0)
      {
         printf("Message::~Message, %d\n", m_iID);
      } */

      delete[] m_szSubject;
   }

   void operator=(const Message &pMessage)
   {
      m_iID = pMessage.m_iID;
      m_iThreadID = pMessage.m_iThreadID;
      m_iDate = pMessage.m_iDate;
      m_iFromID = pMessage.m_iFromID;
      m_szSubject = strmk(pMessage.m_szSubject);
      m_iSize = pMessage.m_iSize;
      m_iSpeed = pMessage.m_iSpeed;
   }
};


#define SORT_LARGEST 1
#define SORT_AVERAGE 2
#define SORT_SPEED 3

int g_iSort = 0;
int g_iNumThreads = 0;

map<int, char *> g_pNames;
map<int, Message> g_pMessages;
map<int, User> g_pUsers;

int SortUsers(const void *p1, const void *p2)
{
   int iReturn = 0;
   User *pUser1 = (User *)p1, *pUser2 = (User *)p2;
   map<int, Message>::const_iterator pMessage1, pMessage2;

   switch(g_iSort)
   {

      case SORT_LARGEST:
         pMessage1 = g_pMessages.find(pUser1->m_iLargestID);
         pMessage2 = g_pMessages.find(pUser2->m_iLargestID);

         if(pMessage1 != g_pMessages.end() && pMessage2 != g_pMessages.end())
         {
            if(pMessage1->second.m_iSize < pMessage2->second.m_iSize)
            {
               iReturn = 1;
            }
            else if(pMessage1->second.m_iSize > pMessage2->second.m_iSize)
            {
               iReturn = -1;
            }
         }
         break;

      case SORT_AVERAGE:
         if(pUser1->m_fAverage < pUser2->m_fAverage)
         {
            iReturn = 1;
         }

         else if(pUser1->m_fAverage > pUser2->m_fAverage)
         {
            iReturn = -1;
         }
         break;
   }

   // printf("SortUsers %d %d -> %d\n", pUser1->m_iID, pUser2->m_iID, iReturn);

   return iReturn;
}

int SortThreads(const void *p1, const void *p2)
{
   int iReturn = 0;
   Message *pMessage1 = (Message *)p1, *pMessage2 = (Message *)p2;

   switch(g_iSort)
   {
      case SORT_LARGEST:
         if(pMessage1->m_iSize < pMessage2->m_iSize)
         {
            iReturn = 1;
         }
         else if(pMessage1->m_iSize > pMessage2->m_iSize)
         {
            iReturn = -1;
         }
         break;

      case SORT_SPEED:
         if(pMessage1->m_iSpeed < pMessage2->m_iSpeed)
         {
            iReturn = -1;
         }
         else if(pMessage1->m_iSpeed > pMessage2->m_iSpeed)
         {
            iReturn = 1;
         }
         break;
   }

   // printf("SortThreads %d(%d) %d(%d) -> %d\n", pMessage1->m_iSpeed, pMessage1->m_iID, pMessage2->m_iSpeed, pMessage2->m_iID, iReturn);

   return iReturn;
}

User *UserGet(int iUserID)
{
   map<int, User>::const_iterator pUser;
   User *pReturn = NULL;

   pUser = g_pUsers.find(iUserID);
   if(pUser == g_pUsers.end())
   {
      // Create
      g_pUsers[iUserID].m_iID = iUserID;
      pUser = g_pUsers.find(iUserID);
   }

   pReturn = (User *)&pUser->second;
   return pReturn;
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

bool UserGetData()
{
   int iUserID = 0;
   double dTick = gettick();
   char *szUsername = NULL;
   DBTable *pTable = NULL;

   printf("UserGetData entry\n");

   pTable = new DBTable();

   pTable->BindColumnInt();
   pTable->BindColumnBytes();

   if(pTable->Execute("select userid,name from user_item") == false)
   {
      printf("UserGetData exit false, query failed\n");
      return false;
   }

   while(pTable->NextRow() == true)
   {
      pTable->GetField(0, &iUserID);
      pTable->GetField(1, &szUsername);

      // printf("Names %d %s\n", iUserID, szUsername);
      g_pNames[iUserID] = strmk(szUsername);
   }

   delete pTable;

   printf("UserGetData exit true, %d (%ld ms)\n", g_pNames.size(), tickdiff(dTick));
   return true;
}

bool MessageGetData(int iMinID, int iSize)
{
   int iNumRows = 0, iMessageID = 0, iParentID = -1, iDate = 0, iFromID = -1;
   double dTick = gettick();
   char *szSubject = NULL;
   char szSQL[200];
   DBTable *pTable = NULL;
   Message *pMessage = NULL, *pThread = NULL;
   User *pUser = NULL;

   printf("MessageGetData entry %d\n", iSize);

   pTable = new DBTable();

   pTable->BindColumnInt();
   pTable->BindColumnInt();
   pTable->BindColumnInt();
   pTable->BindColumnInt();
   pTable->BindColumnBytes();

   sprintf(szSQL, "select messageid,parentid,message_date,fromid,subject from folder_message_item");
   if(iMinID != -1)
   {
      sprintf(szSQL, "%s where messageid >= %d", szSQL, iMinID);
   }
   strcat(szSQL, " order by messageid");

   if(pTable->Execute(szSQL) == false)
   {
      delete pTable;

      printf("MessageGetData exit false, query '%s' failed\n", szSQL);
      return false;
   }
   printf("MessageGetData query run (%ld ms)\n", tickdiff(dTick));

   while(pTable->NextRow() == true)
   {
      // szSubject = NULL;

      iNumRows++;

      pTable->GetField(0, &iMessageID);
      pTable->GetField(1, &iParentID);
      pTable->GetField(2, &iDate);
      pTable->GetField(3, &iFromID);
      pTable->GetField(4, &szSubject);

      if(iNumRows % 1000 == 0)
      {
         printf("MessageGetData row %d: m=%d p=%d d=%d f=%d s=%s\n", iNumRows, iMessageID, iParentID, iDate, iFromID, szSubject);
      }

      if(iFromID > 0)
      {
         pMessage = MessageGet(iMessageID);

         pMessage->m_iDate = iDate;
         pMessage->m_iFromID = iFromID;
         pMessage->m_iSpeed = 0;

         if(g_pUsers.find(iFromID) == g_pUsers.end())
         {
            pUser = UserGet(iFromID);

            pUser->m_szName = strmk(g_pNames.find(pUser->m_iID)->second);
            pUser->m_iNumThreads = 0;
            pUser->m_iTotalSize = 0;
            pUser->m_iLargestID = -1;
         }

         if(iParentID == -1)
         {
            // New thread
            pMessage->m_iThreadID = -1;
            pMessage->m_szSubject = strmk(szSubject);
            pMessage->m_iSize = 1;

            g_iNumThreads++;
         }
         else
         {
            pMessage->m_szSubject = NULL;
            pMessage->m_iSize = 0;

            // Find the parent
            if(g_pMessages.find(iParentID) != g_pMessages.end())
            {
               pThread = MessageGet(iParentID);
               if(pThread->m_iThreadID != -1)
               {
                  // Find the top of thread
                  pThread = MessageGet(pThread->m_iThreadID);
               }

               // Increase thread size
               pThread->m_iSize++;
               if(iSize > 0 && pThread->m_iSize == iSize)
               {
                  pThread->m_iSpeed = iDate - pThread->m_iDate;
                  // printf("MessageGetData thread %d reached size %d seconds\n", pThread->m_iID, pThread->m_iSpeed);
               }

               pMessage->m_iThreadID = pThread->m_iID;
            }
            else
            {
               printf("MessageGetData cannot find parent %d in message %d\n", iParentID, iMessageID);
            }
         }
      }
      else
      {
         printf("MessageGetData invalid from %d in message %d\n", iFromID, iMessageID);
      }

      // delete[] szSubject;
   }

   delete pTable;

   printf("MessageGetData exit true, r=%d t=%d (%ld ms)\n", iNumRows, g_pMessages.size(), tickdiff(dTick));
   return true;
}

void ThreadStatsShow(Message *pThreads, int iNumThreads, int iSort, int iSize)
{
   int iThreadNum = 0, iSpeed = 0;
   char *szUnits = NULL;
   User *pUser = NULL;

   printf("ThreadStatsShow entry %p %d %d\n", pThreads, iNumThreads, iSort);

   g_iSort = iSort;

   printf("ThreadStats sorting\n");
   qsort(pThreads, iNumThreads, sizeof(Message), SortThreads);

   if(iSort == SORT_LARGEST)
   {
      printf("ThreadStatsShow threads above %d messages:\n", iSize);
   }
   else if(iSort == SORT_SPEED)
   {
      printf("ThreadStatsShow fastest rise to %d messages:\n", iSize);
   }

   for(iThreadNum = 0; iThreadNum < iNumThreads; iThreadNum++)
   {
      pUser = UserGet(pThreads[iThreadNum].m_iFromID);
      printf("%s(%d, %s):", pThreads[iThreadNum].m_szSubject, pThreads[iThreadNum].m_iID, pUser->m_szName);
      if(iSort == SORT_SPEED)
      {
         iSpeed = pThreads[iThreadNum].m_iSpeed;
         if(iSpeed >= 3600)
         {
            iSpeed /= 3600;
            szUnits = "hour";
         }
         else if(iSpeed >= 60)
         {
            iSpeed /= 60;
            szUnits = "minute";
         }
         else
         {
            szUnits = "second";
         }
         printf("%d %s%s,", iSpeed, szUnits, iSpeed != 1 ? "s" : "");
      }
      printf(" %d msgs\n", pThreads[iThreadNum].m_iSize);
   }

   printf("ThreadStatsShow exit\n\n");
}

void ThreadStats(int iSize)
{
   int iNumThreads = 0;
   map<int, Message>::const_iterator pMsgIter;
   Message *pThreads = NULL, *pMessage = NULL;

   printf("ThreadStats entry, %d\n", g_iNumThreads);

   pThreads = new Message[g_iNumThreads];

   pMsgIter = g_pMessages.begin();
   while(pMsgIter != g_pMessages.end())
   {
      pMessage = (Message *)&pMsgIter->second;

      /* if(pMessage->m_iThreadID == -1)
      {
         printf("ThreadStats %d size %d\n", pMessage->m_iID, pMessage->m_iSize);
      } */

      if(pMessage->m_iThreadID == -1 && pMessage->m_iFromID > 0 && pMessage->m_iSpeed > 0)
      {
         // Top of thread
         // printf("ThreadStats thread %d %d\n", iNumThreads, pMessage->m_iID);

         pThreads[iNumThreads] = (*pMessage);

         iNumThreads++;
      }

      pMsgIter++;
   }

   ThreadStatsShow(pThreads, iNumThreads, SORT_LARGEST, iSize);
   ThreadStatsShow(pThreads, iNumThreads, SORT_SPEED, iSize);

   delete[] pThreads;

   printf("ThreadStats exit\n");
}

void UserStatsShow(User *pUsers, int iNumUsers, int iSort, int iSize)
{
   int iUserNum = 0;
   Message *pMessage = NULL;

   printf("UserStatsShow entry %p %d %d %d\n", pUsers, iNumUsers, iSort, iSize);

   g_iSort = iSort;

   printf("UserStatsShow sorting\n");
   qsort(pUsers, g_pUsers.size(), sizeof(User), SortUsers);

   if(iSort == SORT_LARGEST)
   {
      printf("UserStatsShow largest thread above %d messages:\n", iSize);
   }
   else if(iSort == SORT_AVERAGE)
   {
      printf("UserStatsShow average thread size above %d threads posted:\n", iSize);
   }

   for(iUserNum = 0; iUserNum < iNumUsers; iUserNum++)
   {
      if(pUsers[iUserNum].m_iNumThreads >= iSize)
      {
         pMessage = MessageGet(pUsers[iUserNum].m_iLargestID, false);
         printf("%s(%d): ", pUsers[iUserNum].m_szName, pUsers[iUserNum].m_iID);
         if(g_iSort == SORT_LARGEST && pMessage != NULL)
         {
            printf("%s(%d), %d msgs", pMessage->m_szSubject, pMessage->m_iID, pMessage->m_iSize);
         }
         else if(g_iSort == SORT_AVERAGE)
         {
            printf("%d msgs / %d threads / avg %0.2f", pUsers[iUserNum].m_iTotalSize, pUsers[iUserNum].m_iNumThreads, pUsers[iUserNum].m_fAverage);
         }
         printf("\n");
      }
   }

   printf("UserStatsShow exit\n\n");
}

void UserStats(int iSize)
{
   int iNumUsers = 0;
   double dTick = gettick();
   float f1 = 0, f2 = 0;
   map<int, Message>::const_iterator pMsgIter;
   map<int, User>::const_iterator pUserIter;
   Message *pMessage = NULL, *pLargest = NULL;
   User *pUser = NULL, *pUsers = NULL;

   printf("UserStats entry\n");

   pMsgIter = g_pMessages.begin();
   while(pMsgIter != g_pMessages.end())
   {
      pMessage = (Message *)&pMsgIter->second;

      if(pMessage->m_iThreadID == -1 && pMessage->m_iFromID > 0)
      {
         // Top of thread

         pUser = UserGet(pMessage->m_iFromID);

         pUser->m_iNumThreads++;
         pUser->m_iTotalSize += pMessage->m_iSize;

         if(pUser->m_iLargestID == -1)
         {
            pUser->m_iLargestID = pMessage->m_iID;
         }
         else
         {
            pLargest = MessageGet(pUser->m_iLargestID, false);
            if(pLargest != NULL && pMessage->m_iSize > pLargest->m_iSize)
            {
               pUser->m_iLargestID = pMessage->m_iID;
            }
         }
      }

      pMsgIter++;
   }
   printf("UserStats users size %d, %ld ms\n", g_pUsers.size(), tickdiff(dTick));

   if(g_pUsers.size() > 0)
   {
      pUsers = new User[g_pUsers.size()];

      pUserIter = g_pUsers.begin();
      while(pUserIter != g_pUsers.end())
      {
         pUser = (User *)&pUserIter->second;

         if(g_iSort != SORT_AVERAGE || pUser->m_iNumThreads >= 10)
         {
            pUsers[iNumUsers] = (*pUser);
            f1 = pUsers[iNumUsers].m_iNumThreads;
            f2 = pUsers[iNumUsers].m_iTotalSize;
            pUsers[iNumUsers].m_fAverage = f2 / f1;

            iNumUsers++;
         }

         pUserIter++;
      }

      UserStatsShow(pUsers, iNumUsers, SORT_LARGEST, iSize);
      UserStatsShow(pUsers, iNumUsers, SORT_AVERAGE, iSize);

      delete[] pUsers;
   }

   printf("UserStats exit\n");
}

int main(int argc, char **argv)
{
   int iMinID = 0, iSize = 0;

   // debuglevel(DEBUGLEVEL_DEBUG);

   if(argc != 6)
   {
      printf("Usage: Poster <database> <username> <password> <min> <size>\n");

      return 1;
   }

   if(DBTable::Connect(argv[1], argv[2], argv[3]) == false)
   {
      printf("Cannot connect to database\n");

      return 1;
   }

   if(UserGetData() == false)
   {
      printf("Cannot get user data\n");

      DBTable::Disconnect();

      return 1;
   }

   iMinID = atoi(argv[4]);
   iSize = atoi(argv[5]);

   if(MessageGetData(iMinID, iSize) == false)
   {
      printf("Cannot get message data\n");

      DBTable::Disconnect();

      return 1;
   }

   DBTable::Disconnect();

   printf("\n");

   ThreadStats(iSize);

   printf("\n");

   UserStats(iSize);

   return 0;
}
