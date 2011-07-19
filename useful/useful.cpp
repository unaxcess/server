/*
** Useful convenience functions
** (c) 1998 Michael Wood (mike@compsoc.man.ac.uk)
*/

#include "stdafx.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef UNIX
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#endif
#ifdef WIN32
#include <windows.h>
#ifndef PSAPIOFF
#include <psapi.h>
#endif
#endif

#include "useful.h"

bool m_bDebugBuffer = false;
bytes *m_pDebugBuffer = NULL;
char *m_szDebugFile = NULL;
FILE *g_fDebug = NULL;
bool g_bDebugOpened = false;
int g_iDebug = DEBUGLEVEL_INFO;

bytes::bytes()
{
   m_pData = NULL;
   m_lLength = 0;
}

bytes::bytes(const char *szValue, int iValueLen)
{
   STACKTRACE

   if(szValue != NULL)
   {
      if(iValueLen == -1)
      {
         m_lLength = strlen(szValue);
      }
      else
      {
         m_lLength = iValueLen;
      }

      m_pData = (byte *)strmk(szValue, 0, m_lLength);
   }
   else
   {
      m_pData = NULL;
      m_lLength = 0;
   }
}

bytes::bytes(const byte *pValue, int iValueLen)
{
   STACKTRACE

   if(pValue != NULL)
   {
      m_lLength = iValueLen;
      m_pData = memmk(pValue, iValueLen);
   }
   else
   {
      m_pData = NULL;
      m_lLength = 0;
   }
}

bytes::bytes(const bytes *pValue, int iValueLen)
{
   STACKTRACE

   if(pValue != NULL)
   {
      if(iValueLen == -1)
      {
         m_lLength = ((bytes *)pValue)->Length();
      }
      else
      {
         m_lLength = iValueLen;
      }

      m_pData = memmk(((bytes *)pValue)->Data(false), m_lLength);
   }
   else
   {
      m_pData = NULL;
      m_lLength = 0;
   }
}

bytes::~bytes()
{
   delete[] m_pData;
}

byte *bytes::Data(const bool bCopy)
{
   STACKTRACE
   byte *pReturn = m_pData;

   if(bCopy == true)
   {
      pReturn = memmk(m_pData, m_lLength);
   }

   return pReturn;
}

byte bytes::Char(int iPos)
{
   if(iPos < 0 || iPos >= m_lLength)
   {
      return '\0';
   }

   return m_pData[iPos];
}

long bytes::Length()
{
   STACKTRACE

   // printf("bytes::Length %p %ld\n", this, m_lLength);

   return m_lLength;
}

long bytes::Append(char *szValue)
{
   return Append((byte *)szValue, strlen(szValue));
}

long bytes::Append(byte *pValue, int iValueLen)
{
   byte *pData = NULL;

   if(pValue == NULL || iValueLen == 0)
   {
      return m_lLength;
   }

   // memprint("bytes::Append entry", pValue, iValueLen);

   if(m_pData != NULL)
   {
      // memprint("bytes::Append buffer", m_pData, m_lLength);

      pData = new byte[m_lLength + iValueLen + 1];
      memcpy(pData, m_pData, m_lLength);
      memcpy(pData + m_lLength, pValue, iValueLen);
      pData[m_lLength + iValueLen] = '\0';
      delete[] m_pData;
      m_pData = pData;

      m_lLength += iValueLen;
   }
   else
   {
      m_pData = memmk(pValue, iValueLen);

      m_lLength = iValueLen;
   }

   // memprint("bytes::Append exit", m_pData, m_lLength);
   return m_lLength;
}

long bytes::Append(bytes *pValue, int iValueLen)
{
   if(pValue == NULL)
   {
      return m_lLength;
   }

   return Append(pValue->Data(false), iValueLen != -1 ? iValueLen : pValue->Length());
}

long bytes::Append(int iValue)
{
   char szValue[32];

   sprintf(szValue, "%d", iValue);
   return Append((byte *)szValue, strlen(szValue));
}

bytes *bytes::SubString(int iLength)
{
   if(iLength > m_lLength)
   {
      return new bytes(this);
   }

   return new bytes(this, iLength > 0 ? iLength : 0);
}

int bytes::Compare(const char *szValue)
{
   return Compare(szValue, m_lLength);
}

int bytes::Compare(const char *szValue, int iLength)
{
   if(iLength == m_lLength)
   {
      return strcmp((char *)m_pData, szValue);
   }

   return strncmp((char *)m_pData, szValue, iLength);
}

int bytes::Compare(bytes *pValue, bool bIgnoreCase)
{
   int iPos = 0, iLength = pValue->Length(), iReturn = 0;
   byte bByte1 = 0, bByte2 = 0;
   byte *pData = pValue->Data(false);

   if(iLength > m_lLength)
   {
      iLength = m_lLength;
   }

   // memprint("bytes::Compare member   ", m_pData, m_lLength);
   // memprint("bytes::Compare parameter", pData, iLength);

   while(iReturn == 0 && iPos < iLength)
   {
      if(bIgnoreCase == true)
      {
         bByte1 = tolower(m_pData[iPos]);
         bByte2 = tolower(pData[iPos]);
      }
      else
      {
         bByte1 = m_pData[iPos];
         bByte2 = pData[iPos];
      }

      if(bByte1 < bByte2)
      {
         iReturn = -1;
      }
      else if(bByte1 > bByte2)
      {
         iReturn = 1;
      }
      else
      {
         iPos++;
      }
   }

   if(iReturn == 0 && iLength < pValue->Length())
   {
      // More bytes in compare value
      return -1;
   }
   else if(iReturn == 0 && iLength < m_lLength)
   {
      // More bytes in this value
      return 1;
   }

   return iReturn;

   /* if(iPos == iLength)
   {
      // printf("bytes::Compare 0\n");
      return 0;
   } */

   // printf("bytes::Compare %d\n", m_pData[iPos] - pData[iPos]);
   // return m_pData[iPos] - pData[iPos];

   // byte *pData = pValue->Data(false);
   // return strcmp((char *)m_pData, (char *)pData);
}

// strmk: Return a new string created from szString between iStart and iEnd
char *strmk(const char *szString, int iStart, int iEnd)
{
   char *szReturn = NULL;

   if(szString == NULL)
   {
      return NULL;
   }

   szReturn = new char[iEnd - iStart + 2];
   memcpy(szReturn, (char *)&szString[iStart], iEnd - iStart);
   szReturn[iEnd - iStart] = '\0';

   // printf("strmk return %ld (%d - %d), '%s'\n", szReturn, iStart, iEnd, szReturn);
   return szReturn;
}

// strmk: Return a new string created from szString
char *strmk(const char *szString)
{
   int iStrLen = 0;
   char *szReturn = NULL;

   if(szString == NULL)
   {
      return NULL;
   }

   iStrLen = strlen(szString);
   szReturn = new char[iStrLen + 1];
   // strcpy(szReturn, szString);
   memcpy(szReturn, szString, iStrLen);
   szReturn[iStrLen] = '\0';

   // printf("strmk return %ld, '%s'\n", szReturn, szReturn);
   return szReturn;
}

// strmk: Return a new string created from cChar
char *strmk(char cChar)
{
   char *szReturn = new char[2];

   szReturn[0] = cChar;
   szReturn[1] = '\0';

   return szReturn;
}

// strmk: Return a string given an integer value
char *strmk(int iValue)
{
   char *szReturn = new char[35];

   sprintf(szReturn, "%d", iValue);

   // printf("strmk return %ld, %s\n", szReturn, szReturn);
   return szReturn;
}

// strtrim: Remove white space from the beginning and end of a string
char *strtrim(const char *szString, int iStart, int iEnd)
{
   char *szReturn = NULL;

   while(iStart < iEnd && (szString[iStart] == ' ' || szString[iStart] == '\t'))
   {
      iStart++;
   }
   while(iEnd > iStart && (szString[iEnd - 1] == ' ' || szString[iEnd - 1] == '\t' || szString[iEnd - 1] == '\r' || szString[iEnd - 1] == '\n'))
   {
      iEnd--;
   }

   szReturn = strmk(szString, iStart, iEnd);
   // printf("strtrim '%s' -> '%s'\n", szString, szReturn);
   return szReturn;
}

char *strtrim(const char *szString)
{
   return strtrim(szString, 0, strlen(szString));
}

char *stristr(const char *szString1, const char *szString2)
{
	char *szTemp1 = strmk(szString1), *szTemp2 = strmk(szString2), *szPos = NULL, *szReturn = NULL;

	szPos = szTemp1;
	while(*szPos != '\0')
	{
		*szPos = tolower(*szPos);
		szPos++;
	}

	szPos = szTemp2;
	while(*szPos != '\0')
	{
		*szPos = tolower(*szPos);
		szPos++;
	}

	szReturn = strstr(szTemp1, szTemp2);

	delete[] szTemp1;
	delete[] szTemp2;

	return szReturn;
}

char *streol(const char *szString)
{
   const char *szCR = NULL, *szLF = NULL;

   if(szString == NULL)
   {
      return NULL;
   }

   szCR = strchr(szString, (int)'\r');
   szLF = strchr(szString, (int)'\n');

   if(szCR != NULL)
   {
      if(szCR[1] == '\n')
      {
         szCR++;
      }

      return (char *)szCR;
   }

   return (char *)szLF;
}

void strpre(char *szString1, const char *szString2)
{
   char *szWrite = new char[strlen(szString1) + strlen(szString2) + 2];

   sprintf(szWrite, "%s%s", szString2, szString1);

   strcpy(szString1, szWrite);

   delete[] szWrite;
}

// Byte functions
byte *memmk(const byte *pData, long lDataLen)
{
   byte *pReturn = NULL;

   if(pData == NULL)
   {
      return NULL;
   }

   pReturn = new byte[lDataLen + 2];
   memcpy(pReturn, pData, lDataLen);
   pReturn[lDataLen] = '\0';

   return pReturn;
}

int memprint(const byte *pData, long lDataLen, bool bRaw)
{
   return memprint(-1, NULL, NULL, pData, lDataLen, bRaw);
}

int memprint(const char *szTitle, const byte *pData, long lDataLen, bool bRaw)
{
   return memprint(-1, NULL, szTitle, pData, lDataLen, bRaw);
}

int memprint(FILE *fOutput, const char *szTitle, const byte *pData, long lDataLen, bool bRaw)
{
   return memprint(-1, fOutput, szTitle, pData, lDataLen, bRaw);
}

int memprint(int iLevel, const char *szTitle, const byte *pData, long lDataLen, bool bRaw)
{
   return memprint(iLevel, NULL, szTitle, pData, lDataLen, bRaw);
}

int memprint(int iLevel, FILE *fOutput, const char *szTitle, const byte *pData, long lDataLen, bool bRaw)
{
   // int iColumn = 0;
   long lDataPos = 0;

   // printf("memprint %d %p %p %p %ld %s\n", iLevel, fOutput, szTitle, pData, lDataLen, BoolStr(bRaw));

   if(iLevel > g_iDebug)
   {
      return -1;
   }

   if(fOutput == NULL)
   {
      fOutput = stdout;
   }

   if(szTitle != NULL)
   {
      fprintf(fOutput, "%s, ", szTitle);
      // fprintf(fOutput, "%ld bytes:%s", lDataLen, (bReturn == true && lDataLen > iReturnSize) ? "\n" : " ");
      fprintf(fOutput, "%ld bytes: ", lDataLen);
   }

   if(pData != NULL)
   {
      for(lDataPos = 0; lDataPos < lDataLen; lDataPos++)// && !(iStopChar != -1 && pData[lDataPos] == iStopChar); lDataPos++)
      {
         /* if(iscntrl(pData[lDataPos]) && bRaw == true)
         {
            fprintf(fOutput, "[%d]", pData[lDataPos]);
         }
         else
         {
            fprintf(fOutput, "%c", pData[lDataPos]);
         } */
         if(bRaw == false && isprint(pData[lDataPos]))
         {
            fprintf(fOutput, "%c", pData[lDataPos]);
         }
         else
         {
            fprintf(fOutput, "[%d]", pData[lDataPos]);
         }
      }
   }
   else
   {
      fprintf(fOutput, "NULL");
   }

   /* if(szTitle != NULL || bReturn == true)
   {
      fprintf(fOutput, "\n");
   } */

   if(szTitle != NULL)
   {
      fprintf(fOutput, "\n");
   }

   fflush(fOutput);

   return 0;
}

int bytesprint(const char *szTitle, const bytes *pBytes, bool bRaw)
{
   return bytesprint(stdout, szTitle, pBytes, bRaw);
}

int bytesprint(FILE *fOutput, const char *szTitle, const bytes *pBytes, bool bRaw)
{
   return memprint(fOutput, szTitle, pBytes != NULL ? ((bytes *)pBytes)->Data(false) : NULL, pBytes != NULL ? ((bytes *)pBytes)->Length() : 0, bRaw);
}

#define SKIP_WS \
while(isspace(*pPos)) \
{ \
   pPos++; \
}

#define SKIP_TOKEN \
while(isspace(*pPos)) \
{ \
   pPos++; \
} \
while(*pPos != '\0' && !isspace(*pPos)) \
{ \
   pPos++; \
}

unsigned long memusage()
{
   // STACKTRACE
#ifdef UNIX
   int fStat = -1;
   char szStat[100], szLine[10000], *pPos = NULL;
   int iLineLen = 0, iKVSize = 0;
   unsigned long lVSize = 0; //, iRSS = 0, iRLim = 0;

   sprintf(szStat, "/proc/%d/stat", getpid());
   fStat = open(szStat, O_RDONLY);
   /* if(fStat == NULL)
   {
      printf("Unable to open %s, %s\n", szStat, strerror(errno));
      return 0;
   } */

   // fgets(szLine, sizeof(szLine), fStat);
   iLineLen = read(fStat, szLine, sizeof(szLine) - 1);
   // memprint("memusage read line", (byte *)szLine, iLineLen);
   close(fStat);

   if(iLineLen == -1)
     {
   return 0;
     }

   // STACKTRACEUPDATE

   szLine[iLineLen] = '\0';

   // printf("memusage: %s", szLine);

   /* sscanf(szLine, "%d %s %c %d %d %d %d %d %u %u %u %u %u %d %d %d %d %d %d %u %u %d %u %u %u",
      // pid -> tgpid
      &iTemp, szTemp, &cTemp, &iTemp, &iTemp, &iTemp, &iTemp, &iTemp,
      // flags -> vsize
      &iTemp, &iTemp, &iTemp, &iTemp, &iTemp, &iTemp, &iTemp, &iTemp, &iTemp, &iTemp, &iTemp, &iTemp, &iTemp, &iTemp,
      &iVSize, &iRSS, &iRLim); */

   // STACKTRACEUPDATE

   pPos = szLine;
   pPos = strchr(pPos, '(') + 1;
   pPos = strchr(pPos, ')') + 1;

   SKIP_WS
   pPos++;

   SKIP_TOKEN  // ppid
   SKIP_TOKEN  // pgrp
   SKIP_TOKEN  // session
   SKIP_TOKEN  // tty
   SKIP_TOKEN  // tty pgrp
   SKIP_TOKEN  // flags
   SKIP_TOKEN  // min flt
   SKIP_TOKEN  // cmin flt
   SKIP_TOKEN  // maj flt
   SKIP_TOKEN  // cmaj flt

   // STACKTRACEUPDATE

   SKIP_TOKEN  // utime
   SKIP_TOKEN  // stime

   SKIP_TOKEN  // cutime
   SKIP_TOKEN  // cstime

   SKIP_TOKEN  // priority
   // printf("memusage priority %s\n", pPos);
   SKIP_TOKEN  // nice

   SKIP_TOKEN  // timeout
   SKIP_TOKEN  // it_real_val
   // SKIP_TOKEN  // start_time

   SKIP_TOKEN  // vsize
   lVSize = atol(pPos);
   iKVSize = (lVSize + 512) >> 10;
   // printf("memsuage vsize %u %u\n", iVSize, iKVSize);

   return lVSize;
#else
#ifndef PSAPIOFF
   PROCESS_MEMORY_COUNTERS mCounters;
   HANDLE pProcess = NULL;

   pProcess = GetCurrentProcess();
   mCounters.WorkingSetSize = 0;
   GetProcessMemoryInfo(pProcess, &mCounters, sizeof(mCounters));
   // printf("memusage %d, %ld %ld %ld\n", bReturn, mCounters.WorkingSetSize, mCounters.QuotaPagedPoolUsage, mCounters.PagefileUsage);

   return mCounters.WorkingSetSize;
#else
   return 0;
#endif
#endif
}

// Read a file into a character array
byte *FileRead(const char *szFilename, size_t *lFileLength)
{
   FILE *fData = NULL;
   byte *pData = NULL;
   long lRead = 0, lReadStart = 0, lReadBlock = 0;
   struct stat sBuff;

   // Check for a valid name
   if(szFilename == NULL)
   {
      return NULL;
   }

   // Attempt to open the file
   fData = fopen(szFilename, "rb");
   if(fData == NULL)
   {
      return NULL;
   }

   // Get file size
   fstat(fileno(fData), &sBuff);
   // printf("FileRead %ld bytes\n", sBuff.st_size);
   pData = new byte[sBuff.st_size + 1];

   // Read the data
   lReadBlock = sBuff.st_size;
// #ifdef UNIX
   while(lReadBlock > 0 && lRead != -1)
/* #else
   while(lReadBlock > 0 && lRead != -1 && !feof(fData))
#endif */
   {
      lRead = fread((char *)&pData[lReadStart], 1, lReadBlock, fData);
      // printf("FileRead read %ld of %ld bytes to %ld\n", lRead, lReadBlock, lReadStart);
      if(lRead > 0)
      {
         lReadStart += lRead;
         lReadBlock -= lRead;
      }
   }
   pData[lReadStart] = '\0';

   fclose(fData);

   if(lFileLength != NULL)
   {
      *lFileLength = lReadStart;
   }
   // printf("FileRead %p, %ld\n", pData, sBuff.st_size);
   return pData;
}

long FileWrite(const char *szFilename, const byte *pWrite, long lWriteLen, int iMask)
{
   int iOldMask = 0;
   long lWritten = 0, lWriteStart = 0, lWriteBlock = 0;
   FILE *fWrite = NULL;

   // printf("FileWrite entry %s %p %ld\n", szFilename, pWrite, lWriteLen);

   if(pWrite == NULL)
   {
      // printf("FileWrite exit 0\n");
      return 0;
   }

   if(lWriteLen == -1)
   {
      lWriteLen = strlen((char *)pWrite);
      // printf("FileWrite data len %ld\n", lWriteLen);
   }

#ifdef UNIX
   if(iMask != -1)
   {
      iOldMask = umask(iMask);
   }
#endif

   fWrite = fopen(szFilename, "wb");
   if(fWrite == NULL)
   {
#ifdef UNIX
      if(iMask != -1)
      {
         umask(iOldMask);
      }
#endif

      // printf("FileWrite exit -1\n");
      return -1;
   }

   lWriteBlock = lWriteLen;
   while(lWriteBlock > 0)
   {
      lWritten = fwrite((char *)&pWrite[lWriteStart], 1, lWriteBlock, fWrite);
      // printf("FileWrite wrote %ld bytes from %ld\n", lWritten, lWriteStart);
      if(lWritten > 0)
      {
         lWriteStart += lWritten;
         lWriteBlock -= lWritten;
      }
   }

   fclose(fWrite);

#ifdef UNIX
   if(iMask != -1)
   {
      umask(iOldMask);
   }
#endif

   // printf("FileWrite exit %ld\n", lWriteStart);
   return lWriteStart;
}

double gettick()
{
   double dTicker = 0.0;
#ifdef UNIX
   struct timeval bTime;
   gettimeofday(&bTime, NULL);
   dTicker = bTime.tv_sec + (double)bTime.tv_usec / 1000000;
#endif
#ifdef WIN32
   dTicker = (double)GetTickCount() / 1000;
#endif

   return dTicker;
}

long tickdiff(double dStart)
{
   double dCurr = gettick();
   long lDiff = (long)(1000 * (dCurr - dStart));

   return lDiff;
}

#ifdef WIN32
void MsgError(const char *szTitle)
{
   LPVOID lpMsgBuf;

   FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
      NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL);

   MessageBox(NULL, (char *)lpMsgBuf, szTitle, MB_OK | MB_ICONEXCLAMATION);
   LocalFree(lpMsgBuf);
}
#endif

int debug(int iLevel, const char *szFormat, va_list vList)
{
   int iPrint = 0;
   char szBuffer[2000];

   if(iLevel > g_iDebug)
   {
      // Don't output this level
      return 0;
   }

   if(m_bDebugBuffer == true)
   {
      vsprintf(szBuffer, szFormat, vList);
      m_pDebugBuffer->Append(szBuffer);
   }
   else
   {
      if(g_fDebug != NULL)
      {
         iPrint = vfprintf(g_fDebug, szFormat, vList);
         fflush(g_fDebug);
      }
      else
      {
         iPrint = vprintf(szFormat, vList);
         fflush(stdout);
      }
   }
   va_end(vList);

   return iPrint;
}

int debug(int iLevel, const char *szFormat, ...)
{
   va_list vList;
   va_start(vList, szFormat);

   return debug(iLevel, szFormat, vList);
}

int debug(const char *szFormat, ...)
{
   va_list vList;
   va_start(vList, szFormat);

   return debug(0, szFormat, vList);
}

bool debugbuffer(bool bBuffer)
{
   bool bReturn = m_bDebugBuffer;

   if(m_bDebugBuffer != bBuffer)
   {
      m_bDebugBuffer = bBuffer;

      if(m_bDebugBuffer == true)
      {
         m_pDebugBuffer = new bytes();
      }
      else
      {
         if(m_pDebugBuffer != NULL)
         {
            debug((char *)m_pDebugBuffer->Data(false));
            delete m_pDebugBuffer;
            m_pDebugBuffer = NULL;
         }
      }
   }

   return bReturn;
}

bool debugopen(const char *szFilename, int iMask)
{
   int iOldMask = 0;

   debugclose();

   if(szFilename == NULL)
   {
      return true;
   }

   // printf("debugopen %s %d\n", szFilename, iMask);

#ifdef UNIX
   if(iMask != -1)
   {
      iOldMask = umask(iMask);
   }
#endif

   g_fDebug = fopen(szFilename, "w");
   if(g_fDebug == NULL)
   {
      debug("debugopen failed to open %s, %s\n", szFilename, strerror(errno));
      return false;
   }

   m_szDebugFile = strmk(szFilename);
   g_bDebugOpened = true;

#ifdef UNIX
   if(iMask != -1)
   {
      umask(iOldMask);
   }
#endif

   return true;
}

bool debugclose(bool bDelete)
{
   if(g_fDebug != NULL && g_bDebugOpened == true)
   {
      fclose(g_fDebug);
      g_fDebug = NULL;
   }

   if(bDelete == true && m_szDebugFile != NULL)
   {
      remove(m_szDebugFile);
   }

   delete[] m_szDebugFile;
   m_szDebugFile = NULL;

   return true;
}

bool debugfile(FILE *fFile)
{
   debugclose(false);

   g_fDebug = fFile;
   g_bDebugOpened = false;

   return true;
}

FILE *debugfile()
{
   return g_fDebug;
}

int debuglevel(int iLevel)
{
   int iReturn = g_iDebug;

   if(iLevel < 0 || iLevel > DEBUGLEVEL_MAX)
   {
      return iReturn;
   }

   // fprintf(debugfile() != NULL ? debugfile() : stdout, "debuglevel %d\n", iLevel);

   g_iDebug = iLevel;

   return iReturn;
}

int debuglevel()
{
   return g_iDebug;
}
