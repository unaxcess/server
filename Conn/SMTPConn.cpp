/*
** SMTPConn: Mail connection class based on Conn class
** (c) 2001 Michael Wood (mike@compsoc.man.ac.uk)
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided this copyright message remains
*/

#include "stdafx.h"

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
#include <regex.h>
#else
#include "regex/hsregex.h"
#endif

#include "SMTPConn.h"

#define SMTP_PORT 25

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

// Constructor
SMTPConn::SMTPConn() : Conn()
{
   STACKTRACE
   debug("SMTPConn::SMTPConn\n");

   // m_iConnType = NONE;
   m_iReadState = 0;

   m_szFrom = NULL;
   m_szTo = NULL;

   m_pFields = NULL;
   m_iNumFields = 0;

   m_szText = NULL;
}

SMTPConn::~SMTPConn()
{
   STACKTRACE
   int iFieldNum = 0;

   debug("SMTPConn::~SMTPConn\n");

   delete[] m_szFrom;
   delete[] m_szTo;

   for(iFieldNum = 0; iFieldNum < m_iNumFields; iFieldNum++)
   {
      delete[] m_pFields[iFieldNum]->m_szName;
      delete[] m_pFields[iFieldNum]->m_szValue;
   }

   delete[] m_pFields;

   delete[] m_szText;
}

bool SMTPConn::Connect(const char *szServer, int iPort, bool bSecure)
{
   debug("SMTPConn::Connect %s %d %s\n", szServer, iPort, BoolStr(bSecure));

   return Conn::Connect(szServer, iPort, bSecure);
}

char *SMTPConn::GetFrom()
{
   return strmk(m_szFrom);
}

bool SMTPConn::SetFrom(const char *szFrom)
{
   delete[] m_szFrom;

   m_szFrom = strmk(szFrom);

   return true;
}

char *SMTPConn::GetTo()
{
   return strmk(m_szTo);
}

bool SMTPConn::SetTo(const char *szTo)
{
   delete[] m_szTo;

   m_szTo = strmk(szTo);

   return true;
}

char *SMTPConn::GetField(const char *szName)
{
   STACKTRACE
   int iFieldNum = 0;

   debug(DEBUGLEVEL_DEBUG, "SMTPConn::GetField %s, %d\n", szName, m_iNumFields);

   while(iFieldNum < m_iNumFields && stricmp(m_pFields[iFieldNum]->m_szName, szName) != 0)
   {
      debug(DEBUGLEVEL_DEBUG, "SMTPConn::GetField not matched %d %s\n",  iFieldNum, m_pFields[iFieldNum]->m_szName);
      iFieldNum++;
   }

   if(iFieldNum < m_iNumFields)
   {
      debug(DEBUGLEVEL_DEBUG, "SMTPConn::GetField matched %d %s %s\n",  iFieldNum, m_pFields[iFieldNum]->m_szName, m_pFields[iFieldNum]->m_szValue);
      return strmk(m_pFields[iFieldNum]->m_szValue);
   }

   return NULL;
}

bool SMTPConn::GetField(int iFieldNum, char **szName, char **szValue)
{
   if(iFieldNum < 0 || iFieldNum >= m_iNumFields)
   {
      return false;
   }

   *szName = strmk(m_pFields[iFieldNum]->m_szName);
   *szValue = strmk(m_pFields[iFieldNum]->m_szValue);

   return true;
}

void SMTPConn::CheckField(const char *szName, const char *szValue)
{
   if(strcmp(szName, "To") == 0)
   {
      SetTo(szValue);
   }
   else if(strcmp(szName, "From") == 0)
   {
      SetFrom(szValue);
   }
}

bool SMTPConn::AddField(const char *szName, const char *szValue)
{
   STACKTRACE
   ConnField *pNew = NULL, **pTemp = NULL;

   debug(DEBUGLEVEL_DEBUG, "SMTPConn::AddField entry %s %s\n", szName, szValue);
   pNew = new ConnField;
   pNew->m_szName = strmk(szName);
   pNew->m_szValue = strmk(szValue);

   debug(DEBUGLEVEL_DEBUG, "SMTPConn::AddField array add\n");
   ARRAY_INSERT(ConnField *, m_pFields, m_iNumFields, pNew, m_iNumFields, pTemp)
     
   CheckField(szName, szValue);

   debug(DEBUGLEVEL_DEBUG, "SMTPConn::AddField exit true\n");
   return true;
}

bool SMTPConn::SetField(const char *szName, const char *szValue)
{
   STACKTRACE
   int iFieldNum = 0;

   debug(DEBUGLEVEL_DEBUG, "SMTPConn::SetField entry %s %s\n", szName, szValue);

   while(iFieldNum < m_iNumFields && stricmp(m_pFields[iFieldNum]->m_szName, szName) != 0)
   {
      iFieldNum++;
   }

   if(iFieldNum < m_iNumFields)
   {
      debug(DEBUGLEVEL_DEBUG, "SMTPConn::SetField set field %d\n", iFieldNum);
      delete[] m_pFields[iFieldNum]->m_szValue;
      m_pFields[iFieldNum]->m_szValue = strmk(szValue);
      
      CheckField(szName, szValue);
   }
   else
   {
      AddField(szName, szValue);
   }

   debug(DEBUGLEVEL_DEBUG, "SMTPConn::SetField exit true\n");
   return true;
}

int SMTPConn::CountFields()
{
   return m_iNumFields;
}

char *SMTPConn::GetText()
{
   return strmk(m_szText);
}

bool SMTPConn::SetText(const char *szText)
{
   STACKTRACE

   delete m_szText;
   m_szText = strmk(szText);

   return true;
}

bool SMTPConn::Read()
{
   STACKTRACE
   int iFieldNum = 0;
   long lReadLen = 0;
   bool bRead = false, bReturn = false;
   char szWrite[1000];
   char *szRead = NULL, *szCRLF = NULL;
   byte *pRead = NULL;

   debug(DEBUGLEVEL_DEBUG, "SMTPConn::Read enty, %d\n", m_iReadState);

   bRead = Conn::Read();
   debug(DEBUGLEVEL_DEBUG, "SMTPConn::Read base %s\n", BoolStr(bRead));

   lReadLen = Conn::ReadBuffer(&pRead);
   if(lReadLen > 0)
   {
      memprint(DEBUGLEVEL_DEBUG, debugfile(), "SMTPConn::Read buffer", pRead, lReadLen);

      szRead = (char *)pRead;

      szCRLF = strstr(szRead, "\r\n");

      if(szCRLF != NULL)
      {
         debug("SMTPConn::Read end of line\n");

         szCRLF += 2;

         if(m_iReadState == 0 && strncmp(szRead, "220 ", 4) == 0)
         {
            debug("SMTPConn::Read sending from\n");

            sprintf(szWrite, "MAIL FROM: %s\r\n", m_szFrom);
            Write(szWrite);

            Release(szCRLF - szRead);

            m_iReadState = 1;
         }
         else if(m_iReadState == 1 && strncmp(szRead, "250 ", 4) == 0)
         {
            debug("SMTPConn::Read sending to\n");

            sprintf(szWrite, "RCPT TO: %s\r\n", m_szTo);
            Write(szWrite);

            Release(szCRLF - szRead);

            m_iReadState = 2;
         }
         else if(m_iReadState == 2 && strncmp(szRead, "250 ", 4) == 0)
         {
            debug("SMTPConn::Read sending data\n");

            Write("DATA\r\n");

            Release(szCRLF - szRead);

            m_iReadState = 3;
         }
         else if(m_iReadState == 3 && strncmp(szRead, "354 ", 4) == 0)
         {
            debug("SMTPConn::Read sending fields and text\n");

            for(iFieldNum = 0; iFieldNum < m_iNumFields; iFieldNum++)
            {
               sprintf(szWrite, "%s: %s\r\n", m_pFields[iFieldNum]->m_szName, m_pFields[iFieldNum]->m_szValue);
               Write(szWrite);
            }

            Write(m_szText);
            Write("\r\n.\r\n");

            Release(szCRLF - szRead);

            m_iReadState = 4;
         }
         else if(m_iReadState == 4 && strncmp(szRead, "250 ", 4) == 0)
         {
            debug("SMTPConn::Rend finished sending\n");

            Write("QUIT\r\n");

            Release(szCRLF - szRead);

            // m_iReadState = 0;

            bReturn = true;
         }
      }
   }

   debug("SMTPConn::Read exit false\n");
   return bReturn;
}

SMTPConn *SMTPConn::Read(const char *szFilename)
{
   STACKTRACE
   size_t lReadLen = 0;
   char *szRead = NULL, *szStart = NULL, *szPos = NULL, *szName = NULL, *szValue = NULL;
   char *szTextPos = NULL;
   byte *pRead = NULL;
   bytes *pValue = NULL;
   SMTPConn *pReturn = NULL;

   debug("SMTPConn::Read entry %s\n", szFilename);

   pRead = FileRead(szFilename, &lReadLen);
   debug("SMTPConn::Read file content %p %ld\n", pRead, lReadLen);

   if(pRead == NULL)
   {
      debug("SMTPConn::Read exit NULL, read failed %s\n", strerror(errno));
      return NULL;
   }

   memprint(debugfile(), "SMTPConn::Read buffer", pRead, lReadLen);

   szRead = (char *)pRead;

   szTextPos = strstr(szRead, "\n\n");
   if(szTextPos == NULL)
   {
      debug("SMTPConn::Read exit NULL, no headers\n");
      return NULL;
   }
   szTextPos += 2;

   pReturn = new SMTPConn();

   if(strmatch(szRead, "^From .+@.+ .+\n", false, false, false) == false)
   {
      delete pReturn;

      debug("SMTPConn::Read exit NULL, bad first line\n");
      return NULL;
   }

   // Get from
   szStart = szRead + 5;
   szPos = strstr(szStart, " ");
   szValue = strmk(szStart, 0, szPos - szStart);
   debug("SMTPConn::Read from '%s'\n", szValue);
   pReturn->SetFrom(szValue);
   delete[] szValue;

   szStart = szPos + 1;
   szPos = strstr(szStart, "\n");
   // debug(DEBUGLEVEL_DEBUG, "SMTPConn::Read date string point %s\n", szStart);
   szValue = strmk(szStart, 0, szPos - szStart);
   debug("SMTPConn::Read date '%s'\n", szValue);
   delete[] szValue;

   szStart = szRead;
   while(szStart != NULL)
   {
      szStart = strstr(szPos, "\n");
      if(szStart != NULL)
      {
         if(strncmp(szStart, "\n\n", 2) == 0)
         {
            szStart = NULL;
         }
         else
         {
            szStart += 1;

            if(strmatch(szStart, "^[a-z-]+: .+\n", false, false, true) == true)
            {
               // New field
               if(szName != NULL && pValue != NULL)
               {
                  pReturn->AddField(szName, (char *)pValue->Data(false));

                  STR_NULL(szName);		  
                  delete pValue;
		  pValue = NULL;
               }

               szPos = strstr(szStart, ":");
               szName = strmk(szStart, 0, szPos - szStart);
               debug("SMTPConn::Read name '%s'\n", szName);

               // Move past field name
               szStart = szPos + 2;

               pValue = new bytes();
            }
            else
            {
               // More value for current field
               pValue->Append("\n");
            }

            szPos = strstr(szStart, "\n");
            szValue = strmk(szStart, 0, szPos - szStart);
            debug("SMTPConn::Read value '%s'\n", szValue);
            pValue->Append(szValue);
            delete[] szValue;
         }
      }
   }

   pReturn->SetText(szTextPos);

   debug("SMTPConn::Read exit %p\n", pReturn);
   return pReturn;
}

// Create: Override base class creator for Accept call
Conn *SMTPConn::Create(bool bSecure)
{
   SMTPConn *pConn = NULL;

   debug("SMTPConn::Create\n");

   pConn = new SMTPConn();
   return pConn;
}

/* bool SMTPConn::Write()
{
   STACKTRACE
   return false;
} */
