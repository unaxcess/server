#ifndef _NNTPCONN_H_
#define _NNTPCONN_H_

#include "Conn.h"

class NNTPConn : public Conn
{
public:
   NNTPConn();
   ~NNTPConn();

   bool Connect(const char *szServer, int iPort);

   bool SetArticle(const char *szArticle);

   bool Read();
   
   static void Init();

private:
   int m_iReadState;

   char *m_szArticle;

   bytes *m_pArticle;
};

#endif
