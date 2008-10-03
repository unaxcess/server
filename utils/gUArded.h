/*
** Provides a digest of UA messages by email
** Which folders are monitored and the frequency of messages
** is all configurable
*/

#ifndef _GUARDED_H_
#define _GUARDED_H_

#include "client/UAClient.h"

class gUArded : public UAClient
{
public:
   gUArded();

   char *CLIENT_NAME();

protected:
   bool startup(int argc, char **argv);

   bool login(EDF *pRequest);
   BackgroundReturn background();

private:
   int m_iMinute;
   bool m_bBackground;
   char m_szOutputFile[100];
   FILE *m_fOutput;
   bool m_bSummary;
   int m_iTotalMsgs;

   bool showMessages(int iWDay, int iHour, int iMin);
   bool showMessages(int iFolderID);
   bool showMessage(int iMessageID, int iNumMsgs);
   bool showMessageReply(EDF *pMessage, const char *szType, const char *szTypeDisplay);
   bool showMessageAnnotation(EDF *pMessage);
   
   void showSummary();
   
   void parseDisableDate(const char *szDate);
   void parseDateVal(int iStatis, int iStart, int iValue);

   void parseDay(const char *szDay);
   void parseDayVal(int iStatus, int iStart, int iValue);

   void parseTime(int iType, const char *szTime);
   void parseTimeVal(int iType, int iStatus, int iStart, int iValue);

   bool outputHeader();
   bool outputFooter();
   
   bool checkDisableDate(int iYeay, int iMonth, int iDay);
   bool checkSchedule(int iWDay, int iHour, int iMin);
};

#endif
