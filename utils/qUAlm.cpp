/*
** UA to Email service
*/

#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "Conn/SMTPConn.h"

#include "qUAlm.h"

char *qUAlm::CLIENT_NAME()
{
   return "qUAlm v0.2";
}

qUAlm::qUAlm() : UAService()
{
   addOption("maildir", OPT_STRING);

   m_pDir = new EDF();

   m_pEmails = NULL;
   m_iNumEmails = 0;
}

qUAlm::~qUAlm()
{
   delete m_pDir;
}

bool qUAlm::loggedIn()
{
   if(UAService::loggedIn() == false)
   {
      return false;
   }

   Timeout(100);

   return true;
}

bool qUAlm::ServiceBackground()
{
   STACKTRACE
   int iToID = 0, iNumFields = 0, iFieldNum = 0, iEmailNum = 0;
   bool bReturn = true, bLoop = false;
   char *szMailDir = NULL, *szFilename = NULL;
   char szFullPath[200];
   char *szFrom = NULL, *szTo = NULL, *szName = NULL, *szValue = NULL, *szText = NULL, *szTrim = NULL;
   DIR *pDir = NULL;
   struct dirent *pEntry = NULL;
   SMTPConn *pEmail = NULL, **pTemp = NULL;
   ServiceLookup *pLookup = NULL;

   if(m_pUser == NULL)
   {
      return false;
   }

   m_pData->GetChild("maildir", &szMailDir);

   // Open the directory for reading
   pDir = opendir(szMailDir);

   if(pDir != NULL)
   {
      // Check the files
      while((pEntry = readdir(pDir)) != NULL)
      {
         if(strcmp(pEntry->d_name, ".") != 0 && strcmp(pEntry->d_name, "..") != 0)
         {
            // printf("qUAlm::ServiceBackground dir entry=%s added=%s reg=%s\n", pEntry->d_name, BoolStr(m_pDir->IsChild("file", pEntry->d_name)), BoolStr(pEntry->d_type == DT_REG));

            if(m_pDir->IsChild("file", pEntry->d_name) == false) // && pEntry->d_type == DT_REG)
            {
               // New file
               printf("qUAlm::ServiceBackground adding file %s\n", pEntry->d_name);

               m_pDir->AddChild("file", pEntry->d_name);
            }
         }
      }

      closedir(pDir);

      // Check files
      bLoop = m_pDir->Child("file");
      while(bLoop == true)
      {
         m_pDir->Get(NULL, &szFilename);

         if(m_pDir->GetChildBool("deleted") == true)
         {
         }
         else if(m_pDir->GetChildBool("sent") == true || m_pDir->GetChildBool("error") == true)
         {
            // Remove sent / error file

            printf("qUAlm::ServiceBackground removing %s (sent %s error %s)\n", szFilename, BoolStr(m_pDir->GetChildBool("sent")), BoolStr(m_pDir->GetChildBool("error")));

            m_pDir->SetChild("deleted", true);
         }
         else
         {
            sprintf(szFullPath, "%s/%s", szMailDir, szFilename);

            printf("qUAlm::ServiceBackground opening %s\n", szFullPath);
            pEmail = SMTPConn::Read(szFullPath);
            if(pEmail != NULL)
            {
               printf("qUAlm::ServiceBackground read email\n");

               szFrom = pEmail->GetFrom();
               printf("qUAlm::ServiceBackground email from %s\n", szFrom);

               iNumFields = pEmail->CountFields();
               for(iFieldNum = 0; iFieldNum < iNumFields; iFieldNum++)
               {
                  pEmail->GetField(iFieldNum, &szName, &szValue);
                  printf("qUAlm::ServiceBackground email field %s:%s%s\n", szName, strstr(szValue, "\n") != NULL ? "\n" : " ", szValue);

                  delete[] szName;
                  delete[] szValue;
               }

               szText = pEmail->GetText();
               szTrim = strtrim(szText);
               if(szTrim != NULL)
               {
                  printf("qUAlm::ServiceBackground email text:\n%s\n", szTrim);

                  szTo = pEmail->GetTo();
                  printf("qUAlm::ServiceBackground to: %s\n", szTo);
                  if(strstr(szTo, "<") != NULL)
                  {
                     szValue = strstr(szTo, "<") + 1;
                     printf("qUAlm::ServiceBackground to value moved: %s\n", szValue);
                  }
                  else
                  {
                     szValue = szTo;
                  }

                  iToID = atoi(szValue);
                  if(iToID > 0)
                  {
                     pLookup = LookupGet(iToID);
                     if(pLookup != NULL)
                     {
                        if(page(0, szFrom, iToID, szTrim, NULL) == true)
                        {
                           // m_pData->SetChild("quit", true);

                           printf("qUAlm::ServiceBackground page sent\n");
                           m_pDir->SetChild("sent", true);
                        }
                        else
                        {
                           printf("qUAlm::ServiceBackground page failed\n");
			   m_pDir->SetChild("error", true);
                        }
                     }
                     else
                     {
                        printf("qUAlm::ServiceBackground no lookup for user %d\n", iToID);
			m_pDir->SetChild("error", true);
                     }
                  }
                  else
                  {
                     printf("qUAlm::ServiceBackground unable to match %s to ID\n", szTo);
		     m_pDir->SetChild("error", true);
                  }
                  delete[] szTo;

                  delete[] szTrim;
               }

               delete[] szFrom;

               // m_pData->SetChild("quit", true);
            }
            else
            {
               printf("qUAlm::ServiceBackground unable to read %s\n", szFullPath);
               m_pDir->SetChild("error", true);
            }
         }

         bLoop = m_pDir->Next("file");
         if(bLoop == false)
         {
            m_pDir->Parent();
         }

         delete[] szFilename;
      }
   }
   else
   {
      printf("qUAlm::ServiceBackground unable to open directory %s, %s\n", szMailDir, strerror(errno));

      m_pData->SetChild("quit", true);

      bReturn = false;
   }

   delete[] szMailDir;

   while(iEmailNum < m_iNumEmails)
   {
      if(m_pEmails[iEmailNum]->State() == Conn::OPEN || m_pEmails[iEmailNum]->State() == Conn::CLOSING)
      {
         printf("qUAlm::ServiceBackground read email %d\n", iEmailNum);
         if(m_pEmails[iEmailNum]->Read() == true)
         {
            printf("qUAlm::ServiceBackground disconect email %d\n", iEmailNum);

            m_pEmails[iEmailNum]->Disconnect();
         }

         iEmailNum++;
      }
      else
      {
         printf("qUAlm::ServiceBackground remove email %d\n", iEmailNum);
         ARRAY_DELETE(SMTPConn *, m_pEmails, m_iNumEmails, iEmailNum, pTemp);
      }
   }

   return bReturn;
}

bool qUAlm::ServiceSend(ServiceLookup *pLookup, char *szTo, char *szText)
{
   STACKTRACE
   char szWrite[100];
   SMTPConn *pEmail = NULL, **pTemp = NULL;

   printf("qUAlm::ServiceSend entry %p '%s' '%s'\n", pLookup, szTo, szText);

   if(strcmp(szTo, "Techno") == 0)
   {
      pEmail = new SMTPConn();

      sprintf(szWrite, "%d@ua2.org", pLookup->m_iLocalID);
      pEmail->SetFrom(szWrite);

      // pEmail->SetTo("mikew@frottage.org");
      pEmail->SetTo("mike@localhost");

      sprintf(szWrite, "%s <%d@ua2.org>", pLookup->m_szLocalName, pLookup->m_iLocalID);
      pEmail->SetField("From", szWrite);

      pEmail->SetText(szText);

      pEmail->Connect("localhost");

      printf("qUAlm::ServiceSend adding email to list\n");
      ARRAY_INSERT(SMTPConn *, m_pEmails, m_iNumEmails, pEmail, m_iNumEmails, pTemp);
   }

   printf("qUAlm::ServiceSend exit true\n");
   return true;
}

bool qUAlm::command(int iFromID, char *szText)
{
   printf("qUAlm::command %d %s\n", iFromID, szText);
   
   if(strcmp(szText, "/x") == 0)
   {
      if(m_pUser->Child("agent", "qUAlm*") == false)
      {
	 m_pUser->Add("agent", "qUAlm*");
      }
      if(m_pUser->Child("data") == false)
      {
	 m_pUser->Add("data");
      }
      
      EDFParser::Print("qUAlm::command data", m_pUser);

      /* while(m_pUser->Children() > 0)
      {
	 m_pUser->DeleteChild();
      } */
   }
   
   return true;
}

int main(int argc, char **argv)
{
   UAClient *m_pClient = NULL;

   debuglevel(DEBUGLEVEL_INFO);

   debugbuffer(true);

   m_pClient = new qUAlm();

   m_pClient->run(argc, argv);

   delete m_pClient;

   return 0;
}
