/*
** SMTPConn: Mail connection class based on Conn class
** (c) 2001 Michael Wood (mike@compsoc.man.ac.uk)
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided this copyright message remains
*/

#ifndef _SMTPCONN_H_
#define _SMTPCONN_H_

#include "Conn.h"

#define SMTPCONN_PORT 25

class SMTPConn : public Conn
{
public:
   // enum ConnType { NONE = 0, READ = 1, WRITE = 2 } ;

   SMTPConn();
   ~SMTPConn();

   bool Connect(const char *szServer, int iPort = SMTPCONN_PORT, bool bSecure = false);

   char *GetFrom();
   bool SetFrom(const char *szFrom);

   char *GetTo();
   bool SetTo(const char *szTo);

   char *GetField(const char *szName);
   bool GetField(int iFieldNum, char **szName, char **szValue);
   bool AddField(const char *szName, const char *szValue);
   bool SetField(const char *szName, const char *szValue);
   int CountFields();

   char *GetText();
   bool SetText(const char *szText);

   bool Read();

   static SMTPConn *Read(const char *szFilename);

protected:
   Conn *Create(bool bSecure = false);

private:
   struct ConnField
   {
      char *m_szName;
      char *m_szValue;
   };

   // ConnType m_iConnType;
   int m_iReadState;

   char *m_szFrom;
   char *m_szTo;

   ConnField **m_pFields;
   int m_iNumFields;

   char *m_szText;

   // bool Write();
   void CheckField(const char *szName, const char *szValue);
};

#endif
