#ifndef _STACKTRACE_H_
#define _STACKTRACE_H_

#ifdef STACKTRACEON

class StackTrace
{
public:
   StackTrace(const char *szFile, const char *szFunction, int iLine, bool bLogging);
   ~StackTrace();

   void update(int iLine);

   static int depth();
   static char *file(int iStackNum);
   static char *function(int iStackNum);
   static int line(int iStackNum);

private:
   struct StackTraceNode
   {
      char m_szFile[100];
      char m_szFunction[50];
      int m_iLine;
      bool m_bLogging;
      double m_dTick;
   };
   static StackTraceNode m_pNodes[];
   static int m_iStackNum;

   static StackTraceNode m_pLast;
};

#ifdef STACKTRACENOFUNCTION
#define STACKTRACE StackTrace trace(__FILE__, NULL, __LINE__, false);
#define STACKTRACELOG StackTrace trace(__FILE__, NULL, __LINE__, true);
#else
#define STACKTRACE StackTrace trace(__FILE__, __FUNCTION__, __LINE__, false);
#define STACKTRACELOG StackTrace trace(__FILE__, __FUNCTION__, __LINE__, true);
#endif

#define STACKTRACEUPDATE trace.update(__LINE__);

void StackTracePrint();
#define STACKTRACEPRINT StackTracePrint();
#define STACKPRINT StackTracePrint(); // Backwards compatibility

#else

#define STACKTRACE
#define STACKTRACELOG
#define STACKTRACEUPDATE
#define STACKTRACEPRINT
#define STACKPRINT // Backwards compatibility

#endif

#endif
