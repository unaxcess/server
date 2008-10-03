/*
** ProxyConn: EDF over HTML connection class based on EDFConn class
** (c) 2001 Michael Wood (mike@compsoc.man.ac.uk)
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided this copyright message remains
*/

#ifndef _ProxyConn_H_
#define _ProxyConn_H_

#include "common/EDFConn.h"

class ProxyConn : public EDFConn
{
public:
   ProxyConn(bool bSecure = false);
   ~ProxyConn();

   bool Connect(const char *szServer, int iPort, bool bSecure = false, const char *szCertFile = NULL);
   bool Connect(const char *szServer, int iPort, const char *szURL, bool bSecure = false, const char *szCertFile = NULL);

   bool Connected();

   bool Disconnect();

   // Read / write methods
   EDF *Read();
   bool Write(const char *szFilename, EDF *pData, bool bRoot = true, bool bCurr = true);

   bool WriteBody(const char *szText, bool bTranslate = false);

   bool SetContentType(const char *szContentType);
   bool SetCookie(const char *szCookie);
   char *GetCookie();

protected:
   Conn *Create(bool bSecure = false);

   bool SetSocket(SOCKET iSocket);

private:
   char *m_szServer;
   int m_iPort;
   bool m_bSecure;
   char *m_szCertFile;

   char *m_szURL;

   int m_iDoc;

   char *m_szContentType;
   char *m_szCookie;

   bool DocStart(int iDoc, const char *szFilename);
   bool DocEnd();
};

#endif
