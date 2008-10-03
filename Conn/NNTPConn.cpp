#ifdef PTHREADON
#include <pthread.h>
#endif
#include <stdio.h>
#include <string.h>

#include "NNTPConn.h"

#ifdef PTHREADON
pthread_mutex_t g_pMutex;
#endif

NNTPConn::NNTPConn() : Conn()
{
   m_iReadState = 0;

   m_szArticle = NULL;

   m_pArticle = NULL;
}

NNTPConn::~NNTPConn()
{
   delete[] m_szArticle;

   delete m_pArticle;
}

bool NNTPConn::Connect(const char *szServer, int iPort)
{
   debug("NNTPConn::Connect %s %d\n", szServer, iPort);

   return Conn::Connect(szServer, iPort);
}

bool NNTPConn::SetArticle(const char *szArticle)
{
   delete m_pArticle;
   m_pArticle = NULL;

   m_szArticle = strmk(szArticle);

   return true;
}

bool NNTPConn::Read()
{
   STACKTRACE
   long lReadLen = 0;
   bool bRead = false, bReturn = false;
   char szWrite[1000];
   char *szRead = NULL, *szCRLF = NULL, *szEOA = NULL;
   byte *pRead = NULL;

   debug(DEBUGLEVEL_DEBUG, "NNTPConn::Read enty, %d\n", m_iReadState);

   bRead = Conn::Read();
   debug(DEBUGLEVEL_DEBUG, "NNTPConn::Read base %s\n", BoolStr(bRead));

   lReadLen = Conn::ReadBuffer(&pRead);
   if(lReadLen > 0)
   {
      memprint(DEBUGLEVEL_DEBUG, debugfile(), "NNTPConn::Read buffer", pRead, lReadLen);

      szRead = (char *)pRead;

      szCRLF = strstr(szRead, "\r\n");
      if(szCRLF != NULL)
      {
         debug(DEBUGLEVEL_DEBUG, "NNTPConn::Read end of line\n");

         szCRLF += 2;

         if(m_iReadState == 0 && strncmp(szRead, "200 ", 4) == 0)
         {
            debug("NNTPConn::Read going into read mode\n");

            Write("mode reader\r\n");

	         Release(szCRLF - szRead);

            m_iReadState = 1;
         }
         else if(m_iReadState == 1 && strncmp(szRead, "200 ", 4) == 0)
         {
            debug("NNTPConn::Read going into article mode\n");

	         Release(szCRLF - szRead);

            m_iReadState = 2;
         }
         else if(m_iReadState == 3 && strncmp(szRead, "220 ", 4) == 0)
         {
            debug("NNTPConn::Read going into read article mode\n");

            m_pArticle = new bytes();

	         Release(szCRLF - szRead);

	         szRead = szCRLF;

            m_iReadState = 4;
         }

         if(m_iReadState == 4)
         {
            debug(DEBUGLEVEL_DEBUG, "NNTPConn::Read reading article\n");

#ifdef PTHREADON
	    pthread_mutex_lock(&g_pMutex);
#endif
            szEOA = strstr(szRead, "\r\n.\r\n");
#ifdef PTHREADON
	    pthread_mutex_unlock(&g_pMutex);
#endif

            if(szEOA != NULL)
            {
               debug("NNTPConn::Read found end of article\n");

               m_pArticle->Append(szRead, szEOA - szRead);

               bytesprint(DEBUGLEVEL_DEBUG, debugfile(), "NNTPConn::Read arcticle", m_pArticle);

	            Release(strlen(szRead));

               m_iReadState = 2;

               bReturn = true;
            }
            else
            {
               debug(DEBUGLEVEL_DEBUG, "NNTPConn::Read appending article\n");

               m_pArticle->Append(szRead);

               Release(strlen(szRead));
            }
         }
      }
   }

   if(m_iReadState == 2 && m_szArticle != NULL)
   {
      debug("NNTPConn::Read write article number\n");

      sprintf(szWrite, "article <%s>\r\n", m_szArticle);
      Write(szWrite);

      STR_NULL(m_szArticle);

      m_iReadState = 3;
   }

   debug(DEBUGLEVEL_DEBUG, "NNTPConn::Read exit %s\n", BoolStr(bReturn));
   return bReturn;
}

void NNTPConn::Init()
{
#ifdef PTHREADON
   pthread_mutex_init(&g_pMutex, NULL);
#endif
}
