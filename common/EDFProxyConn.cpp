/*
** ProxyConn: EDF over HTML connection class based on EDFConn class
** (c) 2001 Michael Wood (mike@compsoc.man.ac.uk)
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided this copyright message remains
*/

#include "stdafx.h"

// Standard headers
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include "EDFProxyConn.h"

#define DOC_INIT 0
#define DOC_HTML 1
#define DOC_BODY 2

// Constructor
ProxyConn::ProxyConn(bool bSecure) : EDFConn()
{
   debug("ProxyConn::ProxyConn %s\n", BoolStr(bSecure));

   m_szServer = NULL;
   m_bSecure = bSecure;
   m_szCertFile = NULL;

   m_szURL = NULL;

   m_iDoc = 0;

   m_szContentType = NULL;
   m_szCookie = NULL;
}

ProxyConn::~ProxyConn()
{
   STACKTRACE
   debug("ProxyConn::~ProxyConn\n");

   DocEnd();

   delete[] m_szServer;
   delete[] m_szCertFile;

   delete[] m_szURL;

   delete[] m_szCookie;
}

bool ProxyConn::Connect(const char *szServer, int iPort, bool bSecure, const char *szCertFile)
{
   debug("ProxyConn::Connect\n");
   return Connect(szServer, iPort, NULL, bSecure, szCertFile);
}

bool ProxyConn::Connect(const char *szServer, int iPort, const char *szURL, bool bSecure, const char *szCertFile)
{
   debug("ProxyConn::Connect %s %d %s %s %s\n", szServer, iPort, szURL, BoolStr(bSecure), szCertFile);
   m_szServer = strmk(szServer);
   m_iPort = iPort;
   m_bSecure = bSecure;
   m_szCertFile = (char *)szCertFile;

   m_szURL = strmk(szURL);

   return true;
}

bool ProxyConn::Connected()
{
   debug("ProxyConn::Connected\n");
   return Conn::Connected();
}

bool ProxyConn::Disconnect()
{
   DocEnd();

   return Conn::Disconnect();
}

// Read: Read EDF from a socket
EDF *ProxyConn::Read()
{
   STACKTRACE
   int iContentHeader = 0, iContentLength = 0;
   int iStrPos = 0, iStrLen = 0, iEDFPos = 0, iHi = 0, iLo = 0, iDebug = 0;
   bool bReturn = false;//, bDebug = false;
   long lCurrLen = 0, lReadLen = 0, lContentPos = 0;
   char cChar = '\0';
   char *szContent = NULL, *szContentLength = NULL, *szCookie = NULL, *szEDF = NULL, *szStr = NULL;
   byte *pRead = NULL;
   EDF *pReturn = NULL;

   lCurrLen = ReadBuffer();

   debug("ProxyConn::Read entry\n");
   bReturn = Conn::Read();

   lReadLen = ReadBuffer(&pRead);

   if(lReadLen > lCurrLen)
   {
      debug("ProxyConn::Read buffer:\n");
      memprint(debugfile(), NULL, pRead, lReadLen);
      debug("\n");

      szContent = strstr((char *)pRead, "\r\n\r\n");
      if(szContent != NULL)
      {
         szContentLength = strstr((char *)pRead, "Content-Length: ");
         if(szContentLength == NULL)
         {
            szContentLength = strstr((char *)pRead, "Content-length: ");
         }

         szCookie = strstr((char *)pRead, "Cookie: ");
         if(szCookie != NULL)
         {
            szCookie += 8;
            szStr = strstr(szCookie, "\r\n");
            if(szStr != NULL)
            {
               m_szCookie = strmk(szCookie, 0, szStr - szCookie);

               debug("ProxyConn::Read cookie %s (%p -> %p = %d)\n", m_szCookie, szCookie, szStr, szStr - szCookie);
            }
         }

         if(szContentLength != NULL)
         {
            iContentHeader = atoi(szContentLength + 16);
            debug("ProxyConn::Read content length(header): %d\n", iContentHeader);

            szContent += 4;
            lContentPos = szContent - (char *)pRead;
            iContentLength = lReadLen - lContentPos;
            debug("ProxyConn::Read content length: %d (%d -> %d)\n", iContentLength, lContentPos, lReadLen);

            if(iContentLength >= iContentHeader)
            {
               szStr = strstr((char *)szContent, "edf=");
               if(szStr != NULL)
               {
                  szStr += 4;
                  debug("ProxyConn::Read EDF string '%s'\n", szStr);

                  iStrLen = strlen(szStr);
                  szEDF = new char[iStrLen + 1];

                  for(iStrPos = 0; iStrPos < iStrLen; iStrPos++)
                  {
                     if(szStr[iStrPos] == '%')
                     {
                        iStrPos++;

                        if(isdigit(szStr[iStrPos]))
                        {
                           iHi = szStr[iStrPos] - '0';
                        }
                        else if(isupper(szStr[iStrPos]))
                        {
                           iHi = 10 + (szStr[iStrPos] - 'A');
                        }
                        else if(islower(szStr[iStrPos]))
                        {
                           iHi = 10 + (szStr[iStrPos] - 'a');
                        }

                        iStrPos++;

                        if(isdigit(szStr[iStrPos]))
                        {
                           iLo = szStr[iStrPos] - '0';
                        }
                        else if(isupper(szStr[iStrPos]))
                        {
                           iLo = 10 + (szStr[iStrPos] - 'A');
                        }
                        else if(islower(szStr[iStrPos]))
                        {
                           iLo = 10 + (szStr[iStrPos] - 'a');
                        }

                        cChar = 16 * iHi + iLo;
                        szEDF[iEDFPos++] = cChar;
                     }
                     else
                     {
                        szEDF[iEDFPos++] = szStr[iStrPos];
                     }
                  }
                  szEDF[iEDFPos] = '\0';

                  // bDebug = EDF::Debug(true);
                  iDebug = debuglevel(DEBUGLEVEL_DEBUG);

                  debug("ProxyConn::Read EDF parse '%s'\n", szEDF);
                  pReturn = new EDF(szEDF);

                  // EDF::Debug(bDebug);
                  debuglevel(iDebug);

                  delete[] szEDF;

                  // debugEDFPrint("ProxyConn::Read", pReturn);
               }
            }
         }
         else
         {
            pReturn = new EDF();
         }
      }
   }

   return pReturn;
}

// Write: Write EDF to a socket
bool ProxyConn::Write(const char *szFilename, EDF *pData, bool bRoot, bool bCurr)
{
   bool bReturn = false;
   bytes *pWrite = NULL;

   debug("ProxyConn::Write entry %p %s %s\n", pData, BoolStr(bRoot), BoolStr(bCurr));

   DocStart(DOC_INIT, szFilename);

   pWrite = pData->Write(bRoot, bCurr);

   bReturn = Conn::Write(pWrite->Data(false), pWrite->Length());

   delete pWrite;

   debug("ProxyConn::Write exit %s\n", BoolStr(bReturn));
   return bReturn;
}

bool ProxyConn::WriteBody(const char *szText, bool bTranslate)
{
   // int iTextPos = 0, iTextLen = 0, iHTMLPos = 0;
   bool bReturn = false;
   char *szHTML = NULL;

   debug("ProxyConn::WriteBody '%s' %s\n", szText, BoolStr(bTranslate));

   SetContentType("text/html");

   DocStart(DOC_BODY, NULL);

   /* if(bTranslate == true)
   {
      szHTML = new char[4 * strlen(szText) + 1];

      iTextLen = strlen(szText);
      for(iTextPos = 0; iTextPos < iTextLen; iTextPos++)
      {
         if(szText[iTextPos] == '<')
         {
            strcpy(szHTML + iHTMLPos, "&lt;");
            iHTMLPos += 4;
         }
         else if(szText[iTextPos] == '>')
         {
            strcpy(szHTML + iHTMLPos, "&gt;");
            iHTMLPos += 4;
         }
         else
         {
            szHTML[iHTMLPos++] = szText[iTextPos];
         }
      }
      szHTML[iHTMLPos] = '\0';

      debug("Conn::WriteBody HTML %s\n", szHTML);
   }
   else */
   {
      szHTML = (char *)szText;
   }

   bReturn = Conn::Write(szHTML);

   if(szHTML != szText)
   {
      delete[] szHTML;
   }

   return bReturn;
}

bool ProxyConn::SetContentType(const char *szContentType)
{
   m_szContentType = strmk(szContentType);

   return true;
}

bool ProxyConn::SetCookie(const char *szCookie)
{
   delete[] m_szCookie;

   m_szCookie = strmk(szCookie);

   return true;
}

char *ProxyConn::GetCookie()
{
   return m_szCookie;
}

// Create: Override base class creator for Accept call
Conn *ProxyConn::Create(bool bSecure)
{
   ProxyConn *pConn = NULL;

   debug("ProxyConn::Create\n");

   pConn = new ProxyConn(m_bSecure);
   return pConn;
}

bool ProxyConn::SetSocket(SOCKET iSocket)
{
   bool bReturn = false;

   bReturn = Conn::SetSocket(iSocket);

   if(m_bSecure == true)
   {
      bReturn = SetSecure();
   }

   return bReturn;
}

bool ProxyConn::DocStart(int iDoc, const char *szFilename)
{
   STACKTRACE
   bool bReturn = false;
   char szWrite[100];
   time_t tTime = 0;
   struct tm *tmTime = NULL;

   debug("ProxyConn::DocStart %d %s, %d\n", iDoc, szFilename, m_iDoc);

   if(iDoc >= DOC_INIT && m_iDoc == DOC_INIT)
   {
      bReturn = Conn::Write("HTTP/1.0 200 OK\r\n");

      sprintf(szWrite, "Content-Type: %s\r\n", m_szContentType != NULL ? m_szContentType : "text/x-edf");
      bReturn = Conn::Write(szWrite);

      if(m_szCookie != NULL)
      {
         bReturn = Conn::Write("Set-Cookie: ");
         bReturn = Conn::Write(m_szCookie);
         bReturn = Conn::Write("\r\n");
      }

      if(szFilename != NULL)
      {
         sprintf(szWrite, "Content-Disposition: attachment; filename=%s\r\n", szFilename);
         Conn::Write(szWrite);
      }

      tTime = time(NULL);
      tmTime = localtime(&tTime);
      strftime(szWrite, sizeof(szWrite), "Date: %a, %d %b %Y %T %Z\r\n", tmTime);
      bReturn = Conn::Write(szWrite);

      bReturn = Conn::Write("\r\n");
      m_iDoc++;
   }

   if(iDoc >= DOC_HTML && m_iDoc == DOC_HTML)
   {
      debug("ProxyConn::DocStart HMTL\n");

      bReturn = Conn::Write("<html>\r\n");
      m_iDoc++;
   }

   if(iDoc >= DOC_BODY && m_iDoc == DOC_BODY)
   {
      debug("ProxyConn::DocStart body\n");

      bReturn = Conn::Write("<body>\r\n");
      m_iDoc++;
   }

   return bReturn;
}

bool ProxyConn::DocEnd()
{
   bool bReturn = false;

   debug("ProxyConn::DocEnd %d\n", m_iDoc);

   if(m_iDoc > DOC_BODY)
   {
      debug("ProxyConn::DocEnd body\n");

      bReturn = Conn::Write("</body>\r\n");
   }

   if(m_iDoc > DOC_HTML)
   {
      debug("ProxyConn::DocEnd HTML\n");

      bReturn = Conn::Write("</hmtl>\r\n");
   }

   m_iDoc = 0;

   return bReturn;
}
