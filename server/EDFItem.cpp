/*
** EDFItem library
** (c) 2000 Michael Wood (mike@compsoc.man.ac.uk)
**
** EDFItem.cpp: Implementation of EDFItem class
*/

#include <stdio.h>
#include <string.h>
/* #ifdef WIN32
#include <typeinfo.h>
#else
#include <typeinfo>
#endif */

#include "EDFItem.h"

EDFItem::EDFItem(long lID)
{
   init(lID);
}

EDFItem::EDFItem(EDF *pEDF, long lID)
{
   STACKTRACE

   if(lID == -1)
   {
      pEDF->Get(NULL, &lID);
   }

   init(lID);
}

EDFItem::EDFItem(DBTable *pTable, int iEDFField)
{
   STACKTRACE
   int iRead = 0;
   long lID = 0;//, lEDFLen = 0;
   bytes *pBytes = NULL;

   // printf("EDFItem::EDFItem entry table %p\n", pTable);

   pTable->GetField(0, &lID);

   init(lID);

   if(iEDFField != -1)
   {
      STACKTRACEUPDATE

      pTable->GetField(iEDFField, &pBytes);
      // printf("EDFItem::EDFIten read %p(%ld bytes) from column %d\n", pEDF, lEDFLen, iEDFField);
      // memprint("EDFItem::EDFItem EDF field", pEDF, lEDFLen);

      iRead = EDFParser::Read(m_pEDF, pBytes);
      if(iRead < 0)
      {
         debug("EDFItem::EDFItem EDF column parse %d\n", iRead);
         bytesprint(NULL, pBytes, false);
      }
      // printf("EDFItem::EDFItem read %d\n", iRead);
   }
   // EDFPrint("EDFItem::EDFItem EDF", m_pEDF);

   // printf("EDFItem::EDFItem exit\n");
}

EDFItem::~EDFItem()
{
   // STACKTRACE

   // printf("EDFItem::~EDFItem %p, %ld %p\n", this, m_lID, m_pEDF);

   // printf("EDFItem::~EDFItem %ld\n", m_lID);

   // EDFPrint(m_pEDF);
   delete m_pEDF;
}

/* const char *EDFItem::GetClass()
{
   return typeid(this).name();
} */

long EDFItem::GetID()
{
   return m_lID;
}

EDF *EDFItem::GetEDF()
{
   m_pEDF->Root();
   return m_pEDF;
}

bool EDFItem::GetDeleted()
{
   return m_bDeleted;
}

bool EDFItem::SetDeleted(bool bDeleted)
{
   debug("EDFItem::SetDeleted %s, %d\n", BoolStr(bDeleted), m_lID);
   SET_INT(m_bDeleted, bDeleted)
}

bool EDFItem::Write(EDF *pEDF, int iLevel)
{
   STACKTRACE
   char szWriteID[100];

   // printf("EDFItem::Write %p %d, %s\n", pEDF, iLevel, WriteName());

   if(WriteName() == NULL)
   {
      debug("EDFItem::Write ** null write name **\n");
      return false;
   }

   if(iLevel > 0 && mask(iLevel, EDFITEMWRITE_FLAT) == true)
   {
      sprintf(szWriteID, "%sid", WriteName());
      pEDF->AddChild(szWriteID, m_lID);
   }
   else
   {
      pEDF->Add(WriteName(), m_lID);
   }

   m_pEDF->Root();
   WriteFields(pEDF, iLevel);

   m_pEDF->Root();
   if(iLevel == -1 && m_pEDF->Children() > 0)
   {
      pEDF->Copy(m_pEDF, false);
   }
   else if(iLevel != -1)
   {
      WriteEDF(pEDF, iLevel);
   }

   return true;
}

bool EDFItem::Insert(char *szTable)
{
   STACKTRACE
   // long lWriteLen = 0;
   bytes *pBytes = NULL;
   DBTable *pTable = NULL;
   EDF *pEDF = NULL;

   // printf("EDFItem::Insert\n");

   if(szTable == NULL)
   {
      szTable = TableName();
   }

   if(szTable == NULL)
   {
      debug("EDFItem::Insert ** null table name **\n");
      return false;
   }

   pEDF = new EDF();

   pTable = new DBTable();

   pTable->SetField(m_lID);

   STACKTRACEUPDATE

   // printf("EDFItem::Insert writing fields %p %p\n", pTable, pEDF);

   WriteFields(pTable, pEDF);

   STACKTRACEUPDATE

   m_pEDF->Root();
   pEDF->Copy(m_pEDF, false);
   if(pEDF->Children() > 0)
   {
      STACKTRACEUPDATE

      pBytes = EDFParser::Write(pEDF, true, true, false);
      // bytesprint("EDFItem::Insert EDF field", pBytes, false);
      pTable->SetField(pBytes, false);
   }
   else
   {
      STACKTRACEUPDATE

      pTable->SetField((char *)NULL, false);
   }

   STACKTRACEUPDATE

   pTable->Insert(szTable);

   STACKTRACEUPDATE

   delete pBytes;

   delete pTable;

   delete pEDF;

   SetChanged(false);

   return true;
}

bool EDFItem::Delete(char *szTable)
{
   STACKTRACE
   char szSQL[200];
   DBTable *pTable = NULL;

   if(szTable == NULL)
   {
      szTable = TableName();
   }

   if(szTable == NULL || KeyName() == NULL)
   {
      debug("EDFItem::Delete ** null table / key name %s, %s **\n", szTable, KeyName());
      return false;
   }

   pTable = new DBTable();
   sprintf(szSQL, "delete from %s where %s = %ld", szTable, KeyName(), m_lID);
   if(pTable->Execute(szSQL) == false)
   {
      delete pTable;

      return false;
   }

   delete pTable;

   return true;
}

bool EDFItem::Update(char *szTable)
{
   if(Delete(szTable) == false)
   {
      debug("EDFItem::Update delete on %s failed\n", szTable);
   }

   return Insert(szTable);
}

bool EDFItem::GetChanged()
{
   return m_bChanged;
}

bool EDFItem::SetChanged(bool bChanged)
{
   // debug("EDFItem::SetChanged %s\n", BoolStr(bChanged));
   m_bChanged = bChanged;
   return true;
}

bool EDFItem::ExtractMember(EDF *pEDF, char *szName, char **szMember, bool bNullIfEmpty)
{
   STACKTRACE
   int iType = 0;
   bool bReturn = false;

   iType = pEDF->TypeGetChild(szName, szMember, NULL, NULL);
   if(iType == EDFElement::BYTES)
   {
      if(bNullIfEmpty == true && (*szMember) != NULL && strlen(*szMember) == 0)
      {
         delete[] *szMember;
         *szMember = NULL;
      }

      bReturn = true;
   }
   else
   {
      if(szMember != NULL)
      {
         *szMember = NULL;
      }
   }

   return bReturn;
}

bool EDFItem::ExtractMember(EDF *pEDF, char *szName, bytes **pMember)
{
   STACKTRACE
   int iType = 0;
   bool bReturn = false;

   iType = pEDF->TypeGetChild(szName, pMember, NULL, NULL);
   if(iType == EDFElement::BYTES)
   {
      bReturn = true;
   }
   else
   {
      if(pMember != NULL)
      {
         *pMember = NULL;
      }
      /* if(lMemberLen != NULL)
      {
         *lMemberLen = 0;
      } */
   }

   return bReturn;
}

bool EDFItem::ExtractMember(EDF *pEDF, char *szName, int *iMember, int iDefault)
{
   STACKTRACE
   int iType = 0;
   bool bReturn = false;
   long lValue = 0;
   double dValue = 0;

   iType = pEDF->TypeGetChild(szName, (bytes **)NULL, &lValue, &dValue);
   if(iType == EDFElement::INT)
   {
      if(iMember != NULL)
      {
         *iMember = lValue;
      }

      bReturn = true;
   }
   else if(iType == EDFElement::FLOAT)
   {
      if(iMember != NULL)
      {
         *iMember = (int)dValue;
      }

      bReturn = true;
   }
   else
   {
      if(iMember != NULL)
      {
         *iMember = iDefault;
      }
   }

   return bReturn;
}

bool EDFItem::ExtractMember(EDF *pEDF, char *szName, long *lMember, long lDefault)
{
   STACKTRACE
   int iType = 0;
   bool bReturn = false;
   double dValue = 0;

   iType = pEDF->TypeGetChild(szName, (bytes **)NULL, lMember, &dValue);
   if(iType == EDFElement::INT)
   {
      bReturn = true;
   }
   else if(iType == EDFElement::FLOAT)
   {
      if(lMember != NULL)
      {
         *lMember = (long)dValue;
      }
      bReturn = true;
   }
   else
   {
      if(lMember != NULL)
      {
         *lMember = lDefault;
      }
   }

   return bReturn;
}

bool EDFItem::ExtractMember(EDF *pEDF, char *szName, unsigned long *lMember, unsigned long lDefault)
{
   STACKTRACE
   int iType = 0;
   bool bReturn = false;
   double dValue = 0;

   iType = pEDF->TypeGetChild(szName, (bytes **)NULL, NULL, &dValue);
   if(iType == EDFElement::INT)
   {
      if(lMember != NULL)
      {
         *lMember = (unsigned long)dValue;
      }
      bReturn = true;
   }
   else if(iType == EDFElement::FLOAT)
   {
      if(lMember != NULL)
      {
         *lMember = (unsigned long)dValue;
      }
      bReturn = true;
   }
   else
   {
      if(lMember != NULL)
      {
         *lMember = lDefault;
      }
   }

   return bReturn;
}

bool EDFItem::ExtractEDF(EDF *pEDF, char *szName)
{
   if(pEDF->Child(szName) == false)
   {
      return false;
   }

   m_pEDF->AddChild(pEDF);

   pEDF->Parent();

   return true;
}

bool EDFItem::EDFFields(EDF *pFields)
{
   STACKTRACE
   int iPos = 0;
   bool bLoop = false, bNext = false, bField = false;
   EDF *pEDF = NULL;

   // printf("EDFItem::EDFFields %p\n", pFields);
   // EDFPrint(pFields, false, false);

   if(pFields == NULL)
   {
      pEDF = m_pEDF;
   }
   else
   {
      pEDF = pFields;
   }

   bLoop = pEDF->Child();
   while(bLoop == true)
   {
      bNext = true;
      bField = IsEDFField(pEDF->GetCurr()->getName(false));

      if(bField == true && pFields != NULL)
      {
         m_pEDF->Copy(pFields);
      }
      else if(bField == false && pFields == NULL)
      {
         iPos = m_pEDF->Position();
         m_pEDF->Delete();
         bLoop = m_pEDF->Child(NULL, iPos);

         bNext = false;
      }

      if(bNext == true)
      {
         bLoop = pEDF->Next();
         if(bLoop == false)
         {
            pEDF->Parent();
         }
      }
   }

   /* if(m_pEDF != NULL)
   {
      debugEDFPrint("EDFItem::EDFItem EDF fields", m_pEDF);
   } */

   return true;
}

/* bool EDFItem::IsEDFField(char *szName)
{
   // printf("EDFItem::EDFField %s, true\n", szName);
   return true;
}

bool EDFItem::WriteFields(EDF *pEDF, int iLevel)
{
   // printf("EDFItem::WriteFields %p, true\n", pEDF);
   return true;
}

bool EDFItem::WriteEDF(EDF *pEDF, int iLevel)
{
   debugEDFPrint("EDFItem::WriteEDF", m_pEDF);

   pEDF->Copy(m_pEDF);

   return true;
} */

bool EDFItem::InitFields(DBTable *pTable)
{
   STACKTRACE

   if(pTable->CountFields() > 0)
   {
      return false;
   }

   // ID
   pTable->BindColumnInt();

   ReadFields(pTable);

   // EDF
   pTable->BindColumnBytes();

   return true;
}

void EDFItem::init(long lID)
{
   STACKTRACE

   m_lID = lID;
   m_bDeleted = false;

   /* m_pFields = NULL;
   m_iNumFields = 0; */

   debug("EDFItem::init not changed\n");
   m_bChanged = false;

   m_pEDF = new EDF();

   // printf("EDFItem::init %ld\n", m_lID);
}
