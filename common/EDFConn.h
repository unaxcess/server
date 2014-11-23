/*
** EDFConn: EDF connection class based on Conn class
** (c) 2001 Michael Wood (mike@compsoc.man.ac.uk)
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided this copyright message remains
*/

#ifndef _EDFCONN_H_
#define _EDFCONN_H_

#include "Conn/Conn.h"
#include "EDF/EDF.h"

class EDFConn : public Conn
{
public:
   EDFConn();
   virtual ~EDFConn();

   virtual bool Connect(const char *szServer, int iPort, bool bSecure = false, const char *szCertFile = NULL);

   bool Connected();

   // Read / write methods
   virtual EDF *Read();
   bool Write(EDF *pData, bool bRoot = true, bool bCurr = true);

   long Buffer();

protected:
   virtual Conn *Create(bool bSecure = false);

private:
   bool m_bEDF;
   int m_iEncoding;
};

#endif
