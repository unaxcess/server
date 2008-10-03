/*
** UA to Email service
*/

#ifndef _QUALM_H_
#define _QUALM_H_

#include "Conn/SMTPConn.h"

#include "client/UAService.h"

class qUAlm : public UAService
{
public:
   qUAlm();
   ~qUAlm();

   char *CLIENT_NAME();

   bool loggedIn();

protected:
   bool ServiceBackground();

   bool ServiceSend(ServiceLookup *pLookup, char *szTo, char *szText);

   bool command(int iFromID, char *szText);

private:
   EDF *m_pDir;

   SMTPConn **m_pEmails;
   int m_iNumEmails;
};

#endif
