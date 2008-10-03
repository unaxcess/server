/*
** EDFConn: EDF connection class based on Conn class
** (c) 2001 Michael Wood (mike@compsoc.man.ac.uk)
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided this copyright message remains
*/

#include "stdafx.h"

// Standard headers
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "EDFConn.h"

// Constructor
EDFConn::EDFConn() : Conn()
{
   STACKTRACE

   m_bEDF = false;
   m_iEncoding = 0;
}

EDFConn::~EDFConn()
{
   STACKTRACE
   debug("EDFConn::~EDFConn\n");
}

bool EDFConn::Connect(const char *szServer, int iPort, bool bSecure, const char *szCertFile)
{
   STACKTRACE
   EDF *pEDF = NULL;

   debug("Conn::Connect\n");

   // Establish connection
   if(Conn::Connect(szServer, iPort, false, szCertFile) == false)
   {
      return false;
   }

   // Setup EDF mode
   pEDF = new EDF();
   pEDF->Set("edf", "on");
   if(bSecure == true)
   {
      pEDF->SetChild("secure", true);
   }

   EDFParser::debugPrint("EDFConn::Connect", pEDF);
   if(Write(pEDF) == false)
   {
      delete pEDF;

      return false;
   }

   if(Conn::Write("\r\n") == false)
   {
      return false;
   }

   if(bSecure == true)
   {
      FlagSecure();
   }

   delete pEDF;

   UseTimeout(false);
   
   return true;
}

bool EDFConn::Connected()
{
   return m_bEDF;
}

// Read EDF from a socket
EDF *EDFConn::Read()
{
   STACKTRACE
   int iEncoding = 0;
   long lReadLen = ReadBuffer(), lParse = 0;
   bool bReturn = true, bFlagSecure = false;
   char *szType = NULL, *szMessage = NULL; //, *szEncoding = NULL;
   byte *pBuffer = NULL;
   EDF *pRead = NULL, *pWrite = NULL;
   double dTick = 0;

   dTick = gettick();
   if(Conn::Read() == false)
   {
      // debug("EDFConn::Read exit NULL\n");
      return NULL;
   }
   if(tickdiff(dTick) > 250)
   {
      // debug("EDFConn::Read base %ld ms\n", tickdiff(dTick));
   }
   
   // debug("EDFConn::Read buffer compare %ld -vs- %ld / %s\n", lReadLen, m_lReadLen, BoolStr(UseTimeout()));
   /* if(m_lReadLen > 0)
   {
      memprint(debugfile(), NULL, m_pRead, m_lReadLen);
      debug("\n");
   } */

   if(ReadBuffer() == 0 || (UseTimeout() == true && ReadBuffer() == lReadLen))
   {
      // No data, no new data read
      UseTimeout(true);
      
      // debug("EDFConn::Read not attempting a parse\n");
      // debug("EDFConn::Read exit false, no parse\n");
      return NULL;
   }

   ReadBuffer(&pBuffer);

   if(m_bEDF == false && ReadBuffer() >= 5)
   {
      // memprint(debugfile(), "EDFConn::Read pre-EDF XML check", m_pRead, m_lReadLen);
      if(strnicmp((char *)pBuffer, "<?xml", 5) == 0 ||
         (ReadBuffer() >= 6 && strnicmp((char *)pBuffer, "<? xml", 6) == 0))
      {
         // m_iEncoding |= EDFElement::EN_XML + EDFElement::EN_XML_EDFROOT;
         m_bEDF = true;
      }
   }

   // Try parsing the buffer
   pRead = new EDF();
   dTick = gettick();
   /* if(m_lReadLen > 50000)
   {
      debug("EDFConn::Read parsing %p, %ld", pRead, m_lReadLen);
   } */
   lParse = EDFParser::Read(pRead, pBuffer, ReadBuffer(), -1, m_iEncoding);
   /* if(m_lReadLen > 50000)
   {
      debug(" -> %ld\n", lParse);
   } */
   if(lParse < 0 || ReadBuffer() > 50000 || tickdiff(dTick) > 250)
   {
      debug("EDFConn::Read parse %ld (%ld bytes in %ld ms)\n", lParse, ReadBuffer(), tickdiff(dTick));
   }
   if(lParse <= 0)
   {
      UseTimeout(true);

      delete pRead;
      
      // debug("EDFConn::Read exit false, failed parse\n");
      return NULL;
   }

   // memprint(debugfile(), "EDFConn::Read pre-release", m_pRead, m_lReadLen);
   Release(lParse);
   // memprint(debugfile(), "EDFConn::Read post-release", m_pRead, m_lReadLen);
   if(ReadBuffer() > 0)
   {
      UseTimeout(false);
   }

   /* if(mask(m_iEncoding, EDFElement::EN_XML) == true)
   {
      EDFParser::debugPrint("EDFConn::Read XML parsed", pRead);
   } */

   pRead->Get(&szType, &szMessage);

   if(szType != NULL && szMessage != NULL)
   {
      if(stricmp(szType, "edf") == 0 && stricmp(szMessage, "on") == 0 && m_bEDF == false)
      {
         // EDF mode

         // if(m_bEDF == false)
         {
            EDFParser::debugPrint("EDFConn::Read EDF mode", pRead);

            /* if(pRead->GetChild("encoding", &szEncoding) == true && szEncoding != NULL)
            {
               if(stricmp(szEncoding, "xml") == 0)
               {
                  // XML encoding supported
                  debug("EDFConn::Read XML encoding on\n");
                  iEncoding = EDFElement::EN_XML;
               }

               delete[] szEncoding;
            } */

            if(Type() == CLIENT)
            {
               if(pRead->GetChildBool("secure") == true)
               {
                  if(SetSecure() == false)
                  {
                     debug("EDFConn::Read secure failed\n");
                  }
               }
            }
            else
            {
               pWrite = new EDF();
               pWrite->Set("edf", "on");
               if(pRead->GetChildBool("secure") == true)
               {
                  // Flag as secure

                  // if(SetSecure() == true)
                  {
                     pWrite->SetChild("secure", true);
                     bFlagSecure = true;
                  }
                  /* else
                  {
                     debug("EDFConn::Read secure failed\n");
                  } */
               }
               /* if(iEncoding == EDFElement::EN_XML)
               {
                  pWrite->SetChild("encoding", "xml");
               } */
               Write(pWrite);
               delete pWrite;

               if(bFlagSecure == true)
               {
                  // Go secure after the EDF mode message has been sent
                  FlagSecure();
               }
            }

            m_bEDF = true;
            m_iEncoding |= iEncoding;
         }
         /* else
         {
            debugEDFPrint("EDFConn::Read EDF mode already on", pRead);
         } */

         bReturn = false;
      }

      delete[] szType;
      delete[] szMessage;
   }

   /* if(bReturn == true)
   {
      *pData = pRead;
      // debugEDFPrint("EDFConn::Read message", pRead);
   } */

   if(bReturn == false)
   {
      delete pRead;
      return NULL;
   }

   return pRead;
}

// Write EDF to a socket
bool EDFConn::Write(EDF *pData, bool bRoot, bool bCurr)
{
   STACKTRACE
   int iOptions = 0;
   bool bWrite = false;
   // long lWriteLen = 0;
   bytes *pWrite = NULL;

   if(bRoot == true)
   {
      iOptions |= EDFElement::EL_ROOT;
   }
   if(bCurr == true)
   {
      iOptions |= EDFElement::EL_CURR;
   }
   iOptions |= m_iEncoding;

   // debug("EDFConn::Write entry %p\n", pData);

   pWrite = EDFParser::Write(pData, iOptions);
   /* if(mask(m_iEncoding, EDFElement::EN_XML) == true)
   {
      memprint(debugfile(), "EDFConn::Write XML data", pWrite->Data(false), pWrite->Length());
   } */
   bWrite = Conn::Write(pWrite->Data(false), pWrite->Length());
   delete pWrite;

   // debug("EDFConn::Write exit %s\n", BoolStr(bWrite));
   return bWrite;
}

long EDFConn::Buffer()
{
   return ReadBuffer();
}

// Create: Override base class creator for Accept call
Conn *EDFConn::Create(bool bSecure)
{
   EDFConn *pConn = NULL;

   debug("EDFConn::Create\n");

   pConn = new EDFConn();
   return pConn;
}
