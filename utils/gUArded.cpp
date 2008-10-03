/*
** Provides a digest of UA messages by email
** Which folders are monitored and the frequency of messages
** is all configurable
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>

#include "Conn/SMTPConn.h"

#include "gUArded.h"

#define PARA "\n<p>&nbsp;</p>\n\n"

#define PARSE_RANGE 1
#define PARSE_ALL 2
#define PARSE_FREQ 3

char *g_pTypeName[] = { "hourcheck", "mincheck" } ;
int g_pTypeMax[] = { 24, 60 } ;

#define TYPE_HOUR 0
#define TYPE_MIN 1

#define DATE_NUM(x) (szDate[x] - '0')

int datetonum(const char *szDate)
{
   bool bLoop = true;
   int iReturn = -1, iPos = 0;
   char *szPos = (char *)szDate;
   
   while(bLoop == true && *szPos != '\0' && iPos < 8)
   {
      if(!isdigit(*szPos))
      {
	 bLoop = false;
      }
      iPos++;
   }
   
   if(bLoop == true)
   {
      iReturn = 10000000 * DATE_NUM(0) + 1000000 * DATE_NUM(1) + 100000 * DATE_NUM(2) + 10000 * DATE_NUM(3)
	+ 1000 * DATE_NUM(4) + 100 * DATE_NUM(5)
	+ 10 * DATE_NUM(6) + DATE_NUM(7);
      debug(DEBUGLEVEL_INFO, "datetonum %s -> %d\n", szDate, iReturn);
   }
   
   return iReturn;
}

int daytonum(const char *szDay)
{
   int iReturn = -1;

   if(strnicmp(szDay, "mon", 3) == 0)
   {
      iReturn = 1;
   }
   if(strnicmp(szDay, "tue", 3) == 0)
   {
      iReturn = 2;
   }
   if(strnicmp(szDay, "wed", 3) == 0)
   {
      iReturn = 3;
   }
   if(strnicmp(szDay, "thu", 3) == 0)
   {
      iReturn = 4;
   }
   if(strnicmp(szDay, "fri", 3) == 0)
   {
      iReturn = 5;
   }
   if(strnicmp(szDay, "sat", 3) == 0)
   {
      iReturn = 6;
   }
   if(strnicmp(szDay, "sun", 3) == 0)
   {
      iReturn = 7;
   }

   debug(DEBUGLEVEL_DEBUG, "daytonum %s -> %d\n", szDay, iReturn);
   return iReturn;
}

char *NumSuffix(int iValue)
{
   if(iValue != 11 && iValue % 10 == 1)
   {
      return "st";
   }
   else if(iValue != 12 && iValue % 10 == 2)
   {
      return "nd";
   }
   else if(iValue != 13 && iValue % 10 == 3)
   {
      return "rd";
   }

   return "th";
}

gUArded::gUArded()
{
   m_iMinute = -1;
   m_bBackground = true;
   m_fOutput = NULL;
   m_bSummary = false;
   m_iTotalMsgs = 0;
}

char *gUArded::CLIENT_NAME()
{
   return "gUArded v0.1";
}

bool gUArded::startup(int argc, char **argv)
{
   STACKTRACE
   bool bLoop = false;
   char *szDefMin = NULL, *szDefHour = NULL, *szDefDay = NULL;
   char *szDate = NULL, *szMin = NULL, *szHour = NULL, *szDay = NULL;

   debug("gUArded::startup entry %d %p\n", argc, argv);

   m_pData->GetChild("defmin", &szDefMin);
   m_pData->GetChild("defhour", &szDefHour);
   m_pData->GetChild("defday", &szDefDay);
   
   parseDay(szDefDay);
   parseTime(TYPE_HOUR, szDefHour);
   parseTime(TYPE_MIN, szDefMin);
   EDFParser::debugPrint("gUArded::startup default", m_pData, EDFElement::EL_CURR | EDFElement::PR_SPACE);

   bLoop = m_pData->Child("disabledate");
   while(bLoop == true)
   {
      m_pData->Get(NULL, &szDate);
      m_pData->TempMark();
      m_pData->Parent();
      parseDisableDate(szDate);
      m_pData->TempUnmark();
      delete[] szDate;
      
      bLoop = m_pData->Next("disabledate");
      if(bLoop == false)
      {
	 m_pData->Parent();
      }
   }
   EDFParser::debugPrint("gUArded::startup disable", m_pData, EDFElement::EL_CURR | EDFElement::PR_SPACE);

   bLoop = m_pData->Child("folder");
   while(bLoop == true)
   {
      szMin = NULL;
      szHour = NULL;
      szDay = NULL;

      m_pData->GetChild("min", &szMin);
      m_pData->GetChild("hour", &szHour);
      m_pData->GetChild("day", &szDay);

      if(szMin == NULL)
      {
         szMin = szDefMin;
      }
      if(szHour == NULL)
      {
         szHour = szDefHour;
      }
      if(szDay == NULL)
      {
         szDay = szDefDay;
      }

      parseDay(szDay);
      parseTime(TYPE_HOUR, szHour);
      parseTime(TYPE_MIN, szMin);
      EDFParser::debugPrint("gUArded::startup folder element", m_pData, EDFElement::EL_CURR | EDFElement::PR_SPACE);

      if(szMin != szDefMin)
      {
         delete[] szMin;
      }
      if(szHour != szDefHour)
      {
         delete[] szHour;
      }
      if(szDay != szDefDay)
      {
         delete[] szDay;
      }

      bLoop = m_pData->Next("folder");
      if(bLoop == false)
      {
         m_pData->Parent();
      }
   }

   delete[] szDefMin;
   delete[] szDefHour;
   delete[] szDefDay;

   debug("gUArded::startup exit true\n");
   return true;
}

bool gUArded::login(EDF *pRequest)
{
   m_pData->SetChild("shadow", true);

   return true;
}

UAClient::BackgroundReturn gUArded::background()
{
   STACKTRACE
   bool bLoggedIn = false, bLoop = false;
   int iUserID = 0, iUserEDF = 0, iStatus = 0;
   time_t lTime = 0;
   struct tm *tmTime = NULL;
   EDF *pRequest = NULL, *pReply = NULL;
   
   if(m_bBackground == true)
   {
      m_bBackground = false;

      lTime = time(NULL);
      tmTime = localtime(&lTime);

      if(tmTime->tm_min != m_iMinute)
      {
         debug("gUArded::background minute check %d\n", tmTime->tm_min);
	 m_iMinute = tmTime->tm_min;
	 
	 if(checkDisableDate(tmTime->tm_year+ 1900, tmTime->tm_mon + 1, tmTime->tm_mday) == false)
	 {  
	 if(m_pData->GetChildBool("disableiflogin") == true)
	 {
	    // debug("gUArded::background checking for login\n");
	    
	    m_pUser->Get(NULL, &iUserID);
	    
	    pRequest = new EDF();
	    pRequest->AddChild("searchtype", 1);
	    if(request(MSG_USER_LIST, pRequest, &pReply) == true)
	    {
	       // EDFParser::debugPrint("gUArded::background wholist", pReply);
	       
	       bLoop = pReply->Child("user");
	       while(bLoggedIn == false && bLoop == true)
	       {
		  pReply->Get(NULL, &iUserEDF);
		  if(iUserID == iUserEDF && pReply->Child("login") == true)
		  {
		     pReply->GetChild("status", &iStatus);
		     // debug("gUArded::background user match status %d\n", iStatus);
		     
		     if(mask(iStatus, LOGIN_SHADOW) == false)
		     {
			debug("gUArded::background primary login found\n");
			bLoggedIn = true;
		     }
		     
		     pReply->Parent();
		  }
		  bLoop = pReply->Next("user");
		  if(bLoop == false)
		  {
		     pReply->Parent();
		  }
	       }
	    }
	    
	    delete pReply;
	    delete pRequest;
	 }
		 
	 
	 if(bLoggedIn == false)
	 {
	    showMessages(tmTime->tm_wday, tmTime->tm_hour, tmTime->tm_min);
	 }
	 }
	 else
	 {
	    debug(DEBUGLEVEL_DEBUG, "gUArded::background current date is disabled\n");
	 }
      }

      m_bBackground = true;
   }

   return BG_NONE;
}

bool gUArded::showMessages(int iWDay, int iHour, int iMin)
{
   STACKTRACE
   bool bLoop = false;
   int iTimeout = 0, iFolderID = 0;
   size_t lFileLen = 0;
   char *szEmail = NULL;
   byte *pFile = NULL;
   SMTPConn *pEmail = NULL;

   debug("gUArded::showMessages entry %d %d %d\n", iWDay, iHour, iMin);

   iTimeout = Timeout();
   Timeout(5);

   m_bSummary = false;
   m_iTotalMsgs = 0;
   
   if(checkSchedule(iWDay, iHour, iMin) == true)
   {
      debug("gUArded::showMessages summary\n");
      
      showSummary();
   }

   bLoop = m_pData->Child("folder");
   while(bLoop == true)
   {
      // EDFParser::debugPrint("gUArded::showMessages folder element", m_pData, EDFElement::EL_CURR | EDFElement::PR_SPACE);
      m_pData->Get(NULL, &iFolderID);

      if(checkSchedule(iWDay, iHour, iMin) == true)
      {
         showMessages(iFolderID);
      }

      // EDFParser::debugPrint("gUArded::showMessages next point", m_pData, EDFElement::EL_CURR | EDFElement::PR_SPACE);
      bLoop = m_pData->Next("folder");
      if(bLoop == false)
      {
         m_pData->Parent();
      }
   }

   outputFooter();

   debug("gUArded::showMessages total messages %d\n", m_iTotalMsgs);

   if((m_bSummary == true || m_iTotalMsgs > 0) && m_pData->GetChildBool("sendmail", true) == true)
   {
      m_pData->GetChild("email", &szEmail);
      
      pEmail = new SMTPConn();

      pEmail->SetFrom("gUArded@ua2.org");

      pEmail->SetTo(szEmail);

      pEmail->AddField("To", szEmail);
      pEmail->AddField("Content-Type", "text/html");
      pEmail->AddField("Subject", "UA2 Digest Mail");

      pFile = FileRead(m_szOutputFile, &lFileLen);

      pEmail->SetText((char *)pFile);

      delete[] pFile;

      pEmail->Connect("localhost", 25);

      while(pEmail->State() == Conn::OPEN || pEmail->State() == Conn::CLOSING)
      {
         if(pEmail->Read() == true)
         {
            pEmail->Disconnect();
         }
      }

      delete pEmail;
      
      delete[] szEmail;
   }

   Timeout(iTimeout);

   debug("gUArded::showMessages exit true\n");
   return true;
}

bool gUArded::showMessages(int iFolderID)
{
   STACKTRACE
   int iUserID = 0, iNumMsgs = 0, iMessageID = 0, iReadMsgs = 0;
   bool bHR = false, bReturn = false, bLoop = false;
   EDF *pRequest = NULL, *pReply = NULL;

   debug("gUArded::showMessages entry %d, %d\n", iFolderID, m_iTotalMsgs);

   pRequest = new EDF();
   pRequest->AddChild("folderid", iFolderID);
   
   if(IsFolderPrivate(iFolderID) == true)
   {
      m_pUser->Get(NULL, &iUserID);
      pRequest->AddChild("toid", iUserID);
   }

   if(request(MSG_MESSAGE_LIST, pRequest, &pReply) == true)
   {
      pReply->GetChild("nummsgs", &iNumMsgs);

      bLoop = pReply->Child("message");
      while(bLoop == true)
      {
         pReply->Get(NULL, &iMessageID);
         if(pReply->GetChildBool("read") == false)
         {
            outputHeader();

            if(bHR == false && (m_bSummary == true || m_iTotalMsgs > 0))
            {
               fprintf(m_fOutput, "%s<hr>%s", PARA, PARA);
	       bHR = true;
            }
            if(iReadMsgs > 0) 
            {
               fprintf(m_fOutput, PARA);
            }

            if(showMessage(iMessageID, iNumMsgs) == true)
	    {
	       iReadMsgs++;
	    }
         }

         bLoop = pReply->Iterate("message");
      }

      delete pReply;

      delete pRequest;

      bReturn = true;
   }
   
   m_iTotalMsgs += iReadMsgs;

   debug("gUArded::showMessages exit %s, %d\n", BoolStr(bReturn), iReadMsgs, m_iTotalMsgs);
   return bReturn;
}

bool gUArded::showMessage(int iMessageID, int iNumMsgs)
{
   STACKTRACE
   int iMsgDate = 0, iMsgPos = 0;
   bool bReturn = false;
   char szMsgDate[100];
   char *szFolderName = NULL, *szToName = NULL, *szFromName = NULL, *szSubject = NULL, *szText = NULL, *szPos = NULL, *szPrev = NULL;
   EDF *pRequest = NULL, *pReply = NULL;
   struct tm *tmMsgDate = NULL;

   debug("gUArded::showMessage entry %d %d\n", iMessageID, iNumMsgs);

   outputHeader();

   pRequest = new EDF();
   pRequest->AddChild("messageid", iMessageID);

   if(request(MSG_MESSAGE_LIST, pRequest, &pReply) == true)
   {
      pReply->GetChild("foldername", &szFolderName);

      if(pReply->Child("message") == true)
      {
         debug("gUArded::showMessage message %d\n", iMessageID);
         EDFParser::debugPrint(DEBUGLEVEL_DEBUG, pReply, EDFElement::PR_SPACE);
         debug(DEBUGLEVEL_DEBUG, "\n");

         pReply->GetChild("date", &iMsgDate);
         pReply->GetChild("fromname", &szFromName);
         pReply->GetChild("toname", &szToName);
         pReply->GetChild("subject", &szSubject);
         pReply->GetChild("text", &szText);
         pReply->GetChild("msgpos", &iMsgPos);

         fprintf(m_fOutput, "<table class=\"msg\">\n");

         fprintf(m_fOutput, "<tr><td colspan=2 align=\"left\" class=\"msginfo\">Message <font class=\"msgvalue\">%d</font> (<font class=\"msgvalue\">%d%s</font> of <font class=\"msgvalue\">%d</font> in <font class=\"msgvalue\">%s</font>)</td></tr>\n", iMessageID, iMsgPos, NumSuffix(iMsgPos), iNumMsgs, szFolderName);

         tmMsgDate = localtime((time_t *)&iMsgDate);
         strftime(szMsgDate, sizeof(szMsgDate), "%A, %d %B %Y - %H:%M:%S", tmMsgDate);

         fprintf(m_fOutput, "<tr><td class=\"msgheadfield\">Date</td><td class=\"msgheadvalue\">%s</td></tr>\n", szMsgDate);

         fprintf(m_fOutput, "<tr><td class=\"msgheadfield\">From</td><td class=\"msgheadvalue\">%s</td>\n", szFromName);
         if(szToName != NULL)
         {
            fprintf(m_fOutput, "<tr><td class=\"msgheadfield\">To</td><td class=\"msgheadvalue\">%s</td></tr>\n", szToName);
         }

         if(szSubject != NULL)
         {
            fprintf(m_fOutput, "<tr><td class=\"msgheadfield\">Subject</td><td class=\"msgheadvalue\">%s</td></tr>\n", szSubject);
         }

         showMessageReply(pReply, "replyto", "In-Reply-To");

         if(szText != NULL)
         {
            // debug("gUArded::showMessage text:\n%s\n", szText);

            fprintf(m_fOutput, "<tr><td colspan=2 class=\"msgtext\">\n<p>");
            szPos = szText;
            while(szPos != NULL)
            {
               szPrev = szPos;
               szPos = strchr(szPos, '\n');
               if(szPos != NULL)
               {
                  *szPos = '\0';
                  fprintf(m_fOutput, szPrev);
                  *szPos = '\n';

                  if(*(szPos + 1) == '\n')
                  {
                     fprintf(m_fOutput, "</p>\n<p>");
                     szPos += 2;
                  }
                  else
                  {
                     fprintf(m_fOutput, "<br>\n");
                     szPos++;
                  }
               }
               else
               {
                  fprintf(m_fOutput, "%s</p>\n", szPrev);
               }
            }
            fprintf(m_fOutput, "</td><tr>\n");
         }

         showMessageReply(pReply, "replyby", "Replied-To-In");
	 
	 showMessageAnnotation(pReply);

         fprintf(m_fOutput, "</table>\n");

         delete[] szText;
         delete[] szSubject;

         delete[] szToName;
         delete[] szFromName;

         pReply->Parent();
      }

      delete[] szFolderName;

      bReturn = true;
   }
   else
   {
      EDFParser::debugPrint("gUArded::showMessage unable to get message", pReply);
   }

   delete pReply;

   delete pRequest;

   debug("gUArded::showMessage exit %s\n", BoolStr(bReturn));
   return bReturn;
}

bool gUArded::showMessageReply(EDF *pMessage, const char *szType, const char *szTypeDisplay)
{
   STACKTRACE
   int iMessageID = 0;
   bool bLoop = false, bFirst = true;
   char *szFromName = NULL, *szFolderName = NULL;

   bLoop = pMessage->Child(szType);
   while(bLoop == true)
   {
      pMessage->Get(NULL, &iMessageID);
      pMessage->GetChild("fromname", &szFromName);
      pMessage->GetChild("foldername", &szFolderName);

      if(bFirst == true)
      {
         fprintf(m_fOutput, "<tr><td class=\"msgheadfield\">%s</td><td class=\"msginfo\">", szTypeDisplay);

         bFirst = false;
      }
      else
      {
         fprintf(m_fOutput, ", ");
      }
      fprintf(m_fOutput, "<font class=\"msgvalue\">%d</font> by <font class=\"msgvalue\">%s</font>", iMessageID, szFromName);
      if(szFolderName != NULL)
      {
	 fprintf(m_fOutput, " (in <font class=\"msgvalue\">%s</font>)", szFolderName);
      }

      STR_NULL(szFolderName);
      delete[] szFromName;

      bLoop = pMessage->Next(szType);
      if(bLoop == false)
      {
         pMessage->Parent();

         fprintf(m_fOutput, "</td></tr>\n");
      }
   }

   return true;
}

bool gUArded::showMessageAnnotation(EDF *pMessage)
{
   STACKTRACE
   int iDate = 0, iSeconds = 0, iValue = 0;
   bool bLoop = false;
   char *szContentType = NULL, *szFromName = NULL, *szText = NULL, *szValueType = 0;

   bLoop = pMessage->Child("attachment");
   while(bLoop == true)
   {
      szContentType = NULL;
      pMessage->GetChild("content-type", &szContentType);
      
      if(szContentType != NULL)
      {
	 if(strcmp(szContentType, MSGATT_ANNOTATION) == 0)
	 {
	    szFromName = NULL;
	    szText = NULL;

	    pMessage->GetChild("date", &iDate);
	    pMessage->GetChild("fromname", &szFromName);
	    pMessage->GetChild("text", &szText);
	    
	    iSeconds = time(NULL) - iDate;
	    if(iSeconds >= 604800)
	    {
	       iValue = iSeconds / 604800;
	       szValueType = "week";
	    }
	    else if(iSeconds >= 86400)
	    {
	       iValue = iSeconds / 86400;
	       szValueType = "day";
	    }
	    else if(iSeconds >= 3600)
	    {
	       iValue = iSeconds / 3600;
	       szValueType = "hour";
	    }
	    else if(iSeconds >= 60)
	    {
	       iValue = iSeconds / 60;
	       szValueType = "minute";
	    }
	    else
	    {
	       iValue = iSeconds;
	       szValueType = "second";
	    }

	    fprintf(m_fOutput, "<tr><td class=\"msgheadfield\">Annotated-By</td><td class=\"msginfo\">");
	    fprintf(m_fOutput, "<font class=\"msgvalue\">%s</font> (<font class=\"msgvalue\">%d</font> %s%s after message posted)", szFromName, iValue, szValueType, iValue != 1 ? "s" : "");
	    if(szText != NULL)
	    {
	       fprintf(m_fOutput, "<br>%s\n", szText);
	    }
	    fprintf(m_fOutput, "</td></tr>\n");

	    delete[] szFromName;
	    delete[] szText;
	 }

	 delete[] szContentType;
      }
      
      bLoop = pMessage->Next("attachment");
      if(bLoop == false)
      {
	 pMessage->Parent();
      }
   }

   return true;
}

void gUArded::showSummary()
{
   STACKTRACE
   int iSubType = 0, iNumUnread = 0, iTotalUnread = 0;
   bool bLoop = false, bFirst = true;
   char *szName = NULL;
   EDF *pRequest = NULL, *pReply = NULL;
   
   pRequest = new EDF();
   pRequest->AddChild("searchtype", 1);
   
   if(request(MSG_FOLDER_LIST, pRequest, &pReply) == true)
   {
      pReply->Sort("folder", "name");
	 
      bLoop = pReply->Child("folder");
      while(bLoop == true)
      {
	 if(pReply->GetChild("subtype", &iSubType) == true && iSubType > 0 && pReply->GetChild("unread", &iNumUnread) == true && iNumUnread > 0)
	 {
	    outputHeader();
	       
	    pReply->GetChild("name", &szName);
	    debug("gUArded::showMessages %s %d\n", szName, iNumUnread);
	       
	    if(bFirst == true)
	    {
	       fprintf(m_fOutput, "<table class=\"msg\">\n");
		  
	       bFirst = false;
	    }
	    
	    fprintf(m_fOutput, "<tr><td class=\"msgheadfield\">%s</td><td class=\"msginfo\"><font class=\"msgvalue\">%d</font> unread message%s</td></tr>\n", szName, iNumUnread, iNumUnread != 1 ? "s" : "");
	    
	    iTotalUnread += iNumUnread;

	    m_bSummary = true;
	 }

	 bLoop = pReply->Iterate("folder");
      }
	 
      if(bFirst == false)
      {
	 fprintf(m_fOutput, "<tr><td class=\"msgheadfield\" colspan=2>&nbsp;</td></tr>\n");
	 fprintf(m_fOutput, "<tr><td class=\"msgheadfield\">Total</td><td class=\"msginfo\"><font class=\"msgvalue\">%d</font> unread message%s</td></tr>\n", iTotalUnread, iTotalUnread != 1 ? "s" : "");
	 
	 fprintf(m_fOutput, "</table>\n\n");
      }
	 
      delete pReply;
   }
   
   delete pRequest;
}

void gUArded::parseDisableDate(const char *szDate)
{
   STACKTRACE
   int iStatus = -1, iValue = 0, iStart = 0, iEnd = 0;
   char *szPos = NULL;

   debug(DEBUGLEVEL_INFO, "gUArded::parseDisableDate entry %s\n", szDate);

   szPos = (char *)szDate;
   while(*szPos != '\0')
   {
      debug(DEBUGLEVEL_INFO, "gUArded::parseDisableDate %d(%d %d): %c %s\n", szPos - szDate, iStatus, iValue, *szPos, szPos);
      if(*szPos == '-')
      {
         iStart = iValue;
         iValue = 0;

         iStatus = PARSE_RANGE;
      }
      else if(*szPos == ',')
      {
         parseDateVal(iStatus, iStart, iValue);

         iValue = 0;
         iStart = 0;
         iEnd = 0;

         iStatus = -1;
      }
      else
      {
         iValue = datetonum(szPos);
         if(iValue != -1)
         {
            szPos += 7;
         }

         if(iStatus == -1)
         {
            iStatus = 0;
         }
      }

      szPos++;
   }

   if(iStatus >= 0)
   {
      parseDateVal(iStatus, iStart, iValue);
   }

   debug(DEBUGLEVEL_INFO, "gUArded::parseDisableDate exit\n");
}

void gUArded::parseDateVal(int iStatus, int iStart, int iValue)
{
   STACKTRACE
   int iEnd = 0, iSet = 0;
   int iYear = 0, iMonth = 0, iDay = 0;

   debug(DEBUGLEVEL_INFO, "gUArded::parseDateVal %d %d %d\n", iStatus, iStart, iValue);

   if(iStatus == PARSE_RANGE)
   {
      iSet = iStart;
      iEnd = iValue;
      while(iSet <= iEnd)
      {
         debug(DEBUGLEVEL_INFO, "gUArded::parseDateVal set %d\n", iSet);
         m_pData->AddChild("uncheckdate", iSet);
	 iYear = iSet / 10000;
	 iMonth = (iSet % 10000) / 100;
	 iDay = iSet % 100;
	 debug(DEBUGLEVEL_INFO, "gUArded::parseDateVal %d %d %d\n", iYear, iMonth, iDay);
	 iDay++;
	 if((iMonth == 9 || iMonth == 4 || iMonth == 6 || iMonth == 11) && iDay > 30)
	 {
	    iDay = 1;
	    iMonth++;
	 }
	 else if((iYear % 4 != 0 && iMonth == 2 && iDay > 28) || (iYear % 4 == 0 && iMonth == 2 && iDay > 29))
	 {
	    iDay = 1;
	    iMonth++;
	 }
	 else if(iDay > 31)
	 {
	    iDay = 1;
	    iMonth++;
	 }
	 
	 if(iMonth > 12)
	 {
	    iMonth = 1;
	    iYear++;
	 }
	 
	 iSet = 10000 * iYear + 100 * iMonth + iDay;     
      }
   }
   else
   {
      debug(DEBUGLEVEL_INFO, "gUArded::parseDateVal set %d\n", iValue);
      m_pData->AddChild("uncheckdate", iValue);
   }
}

void gUArded::parseDay(const char *szDay)
{
   STACKTRACE
   int iStatus = -1, iValue = 0, iStart = 0, iEnd = 0;
   char *szPos = NULL;

   debug(DEBUGLEVEL_DEBUG, "gUArded::parseDay entry %s\n", szDay);

   szPos = (char *)szDay;
   while(*szPos != '\0')
   {
      debug(DEBUGLEVEL_DEBUG, "gUArded::parseDay %d(%d %d): %c %s\n", szPos - szDay, iStatus, iValue, *szPos, szPos);
      if(*szPos == '-')
      {
         iStart = iValue;
         iValue = 0;

         iStatus = PARSE_RANGE;
      }
      else if(*szPos == '*')
      {
         iStatus = PARSE_ALL;
      }
      else if(*szPos == ',')
      {
         parseDayVal(iStatus, iStart, iValue);

         iValue = 0;
         iStart = 0;
         iEnd = 0;

         iStatus = -1;
      }
      else
      {
         iValue = daytonum(szPos);
         if(iValue != -1)
         {
            szPos += 2;
         }

         if(iStatus == -1)
         {
            iStatus = 0;
         }
      }

      szPos++;
   }

   if(iStatus >= 0)
   {
      parseDayVal(iStatus, iStart, iValue);
   }

   debug(DEBUGLEVEL_DEBUG, "gUArded::parseDay exit\n");
}

void gUArded::parseDayVal(int iStatus, int iStart, int iValue)
{
   STACKTRACE
   int iEnd = 0, iSet = 0;

   debug(DEBUGLEVEL_DEBUG, "gUArded::parseDayVal %d %d %d\n", iStatus, iStart, iValue);

   if(iStatus == PARSE_RANGE)
   {
      iEnd = iValue;
      for(iSet = iStart; iSet <= iEnd; iSet++)
      {
         debug(DEBUGLEVEL_DEBUG, "gUArded::parseDayVal set %d\n", iSet);
         m_pData->AddChild("daycheck", iSet % 7);
      }
   }
   else if(iStatus == PARSE_ALL)
   {
      for(iSet = 1; iSet < 7; iSet++)
      {
         debug(DEBUGLEVEL_DEBUG, "gUArded::parseDayVal set %d\n", iSet);
         m_pData->AddChild("daycheck", iSet % 7);
      }
   }
   else
   {
      debug(DEBUGLEVEL_DEBUG, "gUArded::parseDayVal set %d\n", iValue);
      m_pData->AddChild("daycheck", iValue % 7);
   }
}

void gUArded::parseTime(int iType, const char *szTime)
{
   STACKTRACE
   int iStatus = -1, iValue = 0, iStart = 0, iEnd = 0;
   char *szPos = NULL;

   debug(DEBUGLEVEL_DEBUG, "gUArded::parseTime entry %d %s\n", iType, szTime);

   szPos = (char *)szTime;
   while(*szPos != '\0')
   {
      debug(DEBUGLEVEL_DEBUG, "gUArded::parseTime %d(%d %d): %c\n", szPos - szTime, iStatus, iValue, *szPos);
      if(*szPos == '-')
      {
         iStart = iValue;
         iValue = 0;

         iStatus = PARSE_RANGE;
      }
      else if(*szPos == '*')
      {
         iStatus = PARSE_ALL;
      }
      else if(*szPos == '/')
      {
         iStatus = PARSE_FREQ;
      }
      else if(*szPos == ',')
      {
         parseTimeVal(iType, iStatus, iStart, iValue);

         iValue = 0;
         iStart = 0;
         iEnd = 0;

         iStatus = -1;
      }
      else if(isdigit(*szPos))
      {
         iValue = 10 * iValue + ((*szPos) - '0');

         if(iStatus == -1)
         {
            iStatus = 0;
         }
      }

      szPos++;
   }

   if(iStatus >= 0)
   {
      parseTimeVal(iType, iStatus, iStart, iValue);
   }

   debug(DEBUGLEVEL_DEBUG, "gUArded::parseTime exit\n");
}

void gUArded::parseTimeVal(int iType, int iStatus, int iStart, int iValue)
{
   STACKTRACE
   int iEnd = 0, iSet = 0;

   debug(DEBUGLEVEL_DEBUG, "gUArded::parseTimeVal %d %d %d %d\n", iType, iStatus, iStart, iValue);

   if(iStatus == PARSE_RANGE)
   {
      iEnd = iValue;
      for(iSet = iStart; iSet <= iEnd; iSet++)
      {
         debug(DEBUGLEVEL_DEBUG, "gUArded::parseTimeVal set %d\n", iSet);
         m_pData->AddChild(g_pTypeName[iType], iSet);
      }
   }
   else if(iStatus == PARSE_ALL)
   {
      for(iSet = 0; iSet < g_pTypeMax[iType]; iSet++)
      {
         debug(DEBUGLEVEL_DEBUG, "gUArded::parseTimeVal set %d\n", iSet);
         m_pData->AddChild(g_pTypeName[iType], iSet);
      }
   }
   else if(iStatus == PARSE_FREQ)
   {
      for(iSet = 0; iSet < g_pTypeMax[iType]; iSet += g_pTypeMax[iType] / iValue)
      {
         debug(DEBUGLEVEL_DEBUG, "gUArded::parseTimeVal set %d\n", iSet);
         m_pData->AddChild(g_pTypeName[iType], iSet);
      }
   }
   else
   {
      debug(DEBUGLEVEL_DEBUG, "gUArded::parseTimeVal set %d\n", iValue);
      m_pData->AddChild(g_pTypeName[iType], iValue);
   }
}

bool gUArded::outputHeader()
{
   time_t lTime = 0;
   struct tm *tmTime = NULL;

   if(m_fOutput == NULL)
   {
      lTime = time(NULL);
      tmTime = localtime(&lTime);
      sprintf(m_szOutputFile, "gUArded-%02d%02d%02d.html", tmTime->tm_wday, tmTime->tm_hour, tmTime->tm_min);
      m_fOutput = fopen(m_szOutputFile, "w");
      if(m_fOutput == NULL)
      {
         debug("gUArded::outputHeader unable to open %s, %s\n", m_szOutputFile, strerror(errno));
         return false;
      }

      fprintf(m_fOutput, "<html>\n\n");

      fprintf(m_fOutput, "<head>\n");

      fprintf(m_fOutput, "<style type=\"text/css\">\n");
      fprintf(m_fOutput, "<!--\n");

      fprintf(m_fOutput, "  .msg { width: 75%%; border: #213139; border-style: solid; border-top-width: 1px; border-left-width: 1px; border-bottom-width: 1px; border-right-width: 1px }\n");
      fprintf(m_fOutput, "  .msginfo { background: #d6d6e7; color: #213139; border: #213139; border-style: solid; border-top-width: 0px; border-left-width: 0px; border-bottom-width: 0px; border-right-width: 0px; padding: 2px }\n");
      fprintf(m_fOutput, "  .msgheadfield { background: #d6d6e7; color: #213139; border: #213139; border-style: solid; border-top-width: 0px; border-left-width: 0px; border-bottom-width: 0px; border-right-width: 0px; padding: 2px; white-space: nowrap; vertical-align: top }\n");
      fprintf(m_fOutput, "  .msgheadvalue { width=100%%; background: #d6d6e7; color: #213139; font-weight: bold; border: #213139; border-style: solid; border-top-width: 0px; border-left-width: 0px; border-bottom-width: 0px; border-right-width: 0px; padding: 2px }\n");
      fprintf(m_fOutput, "  .msgvalue { font-weight: bold }\n");
      fprintf(m_fOutput, "  .msgtext { }\n");

      fprintf(m_fOutput, "-->\n");
      fprintf(m_fOutput, "</style>\n");

      fprintf(m_fOutput, "</head>\n\n");
      
      fprintf(m_fOutput, "<body>\n\n");
   }

   return true;
}

bool gUArded::outputFooter()
{
   if(m_fOutput == NULL)
   {
      return false;
   }

   fprintf(m_fOutput, "\n</body>\n\n</html>\n");

   fclose(m_fOutput);

   m_fOutput = NULL;

   return true;
}

bool gUArded::checkDisableDate(int iYear, int iMonth, int iDay)
{
   STACKTRACE
   int iDate = 10000 * iYear + 100 * iMonth + iDay;
   bool bReturn = false;
   
   bReturn = EDFFind(m_pData, "uncheckdate", iDate, false);
   
   debug(DEBUGLEVEL_INFO, "gUArded::checkDisableDate %d %d %d -> %d %s\n", iYear, iMonth, iDay, iDate, BoolStr(bReturn));
   return bReturn;
}

bool gUArded::checkSchedule(int iWDay, int iHour, int iMin)
{
   STACKTRACE
   bool bReturn = true;
   
   if(bReturn == true && EDFFind(m_pData, "daycheck", iWDay, false) == true)
   {
      m_pData->Parent();
   }
   else
   {
      bReturn = false;
   }
   if(bReturn == true && EDFFind(m_pData, "hourcheck", iHour, false) == true)
   {
      m_pData->Parent();
   }
   else
   {
      bReturn = false;
   }
   if(bReturn == true && EDFFind(m_pData, "mincheck", iMin, false) == true)
   {
      m_pData->Parent();
   }
   else
   {
      bReturn = false;
   }
   
   return bReturn;
}

int main(int argc, char **argv)
{
   gUArded *pClient = NULL;

   debugbuffer(true);

   pClient = new gUArded();

   pClient->run(argc, argv);

   delete pClient;

   return 0;
}
