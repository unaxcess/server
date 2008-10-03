#include "stdafx.h"

#ifdef STACKTRACEON

#include <stdio.h>
#include <string.h>

#include "useful.h"

#ifdef UNIX
#define CRLF "\r\n"
#else
#define CRLF "\n"
#endif

#define MAX_STACK 50
int StackTrace::m_iStackNum = 0, g_iMaxStack = 0;
struct StackTrace::StackTraceNode StackTrace::m_pNodes[MAX_STACK];
struct StackTrace::StackTraceNode StackTrace::m_pLast;

StackTrace::StackTrace(const char *szFile, const char *szFunction, int iLine, bool bLogging)
{
   if(m_iStackNum >= MAX_STACK)
   {
      if(m_iStackNum > g_iMaxStack)
      {
         printf("StackTrace::StackTrace (%s %s %d) cannot increase stack from %d\n", szFile, szFunction, iLine, m_iStackNum);
         g_iMaxStack = m_iStackNum;
      }
   }
   else
   {
      // printf("StackTrace::StackTrace entry %s %s %d\n", szFile, szFunction, iLine);
      if(bLogging == true)
      {
         debug("%-*s%s entry (mem: %ld bytes)\n", m_iStackNum, "", szFunction, memusage());
      }

      strcpy(m_pNodes[m_iStackNum].m_szFile, szFile);
      if(szFunction != NULL)
      {
         strcpy(m_pNodes[m_iStackNum].m_szFunction, szFunction);
      }
      else
      {
         strcpy(m_pNodes[m_iStackNum].m_szFunction, "");
      }
      m_pNodes[m_iStackNum].m_iLine = iLine;
      m_pNodes[m_iStackNum].m_bLogging = bLogging;
      m_pNodes[m_iStackNum].m_dTick = gettick();

      strcpy(m_pLast.m_szFile, m_pNodes[m_iStackNum].m_szFile);
      strcpy(m_pLast.m_szFunction, m_pNodes[m_iStackNum].m_szFunction);
      m_pLast.m_iLine = m_pNodes[m_iStackNum].m_iLine;
   }

   m_iStackNum++;
}

StackTrace::~StackTrace()
{
   m_iStackNum--;

   if(m_iStackNum < MAX_STACK)
   {
      // strcpy(m_szCurrent, m_pNodes[m_iStackNum].m_szFunction);

      if(m_pNodes[m_iStackNum].m_bLogging == true)
      {
         debug("%-*s%s exit (mem: %ld bytes, %ld ms)\n", m_iStackNum, "", m_pNodes[m_iStackNum].m_szFunction, memusage(), tickdiff(m_pNodes[m_iStackNum].m_dTick));
      }
   }
}

void StackTrace::update(int iLine)
{
   if(m_iStackNum < MAX_STACK)
   {
      m_pNodes[m_iStackNum - 1].m_iLine = iLine;
   }
}

int StackTrace::depth()
{
   return m_iStackNum;
}

char *StackTrace::file(int iStackNum)
{
   if(iStackNum == -1)
   {
      return m_pLast.m_szFile;
   }

   return m_pNodes[iStackNum].m_szFile;
}

char *StackTrace::function(int iStackNum)
{
   if(iStackNum == -1)
   {
      return m_pLast.m_szFunction;
   }

   return m_pNodes[iStackNum].m_szFunction;
}

int StackTrace::line(int iStackNum)
{
   if(iStackNum == -1)
   {
      return m_pLast.m_iLine;
   }

   return m_pNodes[iStackNum].m_iLine;
}

void StackTracePrint()
{
   int iStackNum, iNumStacks;

   iNumStacks = StackTrace::depth();
   printf("StackTracePrint entry, %d%s", iNumStacks, CRLF);

   printf(" Last trace. Line %d", StackTrace::line(-1));
   fflush(stdout);
   printf(" of %s", StackTrace::file(-1));
   fflush(stdout);
   if(strcmp(StackTrace::function(-1), "") != 0)
   {
      printf(" (%s)", StackTrace::function(-1));
   }
   printf(CRLF);

   for(iStackNum = iNumStacks - 1; iStackNum >= 0; iStackNum--)
   {
      if(iStackNum >= MAX_STACK)
      {
         printf(" No stack for %d%s", iStackNum, CRLF);
      }
      else
      {
         printf(" Stack %d. Line %d", iStackNum, StackTrace::line(iStackNum));
         fflush(stdout);
         printf(" of %s", StackTrace::file(iStackNum));
         fflush(stdout);
         if(strcmp(StackTrace::function(iStackNum), "") != 0)
         {
            printf(" (%s)", StackTrace::function(iStackNum));
         }
         printf(CRLF);
      }
   }

   printf("StackTracePrint exit%s", CRLF);
}

#endif
