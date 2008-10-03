/*
** EDF - Encapsulated Data Format
** (c) 1998 Michael Wood (mike@compsoc.man.ac.uk)
**
** EDF.cpp: Implementation of EDF class
*/

#include "stdafx.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>

#include "EDF.h"

// bool m_bDebug = false;

#if 0
// Constructor
EDF::EDF(const char *szData)
{
   STACKTRACE
   init();

   if(szData != NULL)
   {
      Read(szData);
   }

   // printf(" EDF::EDF root element %p\n", m_pRoot);
}

EDF::EDF(const byte *pData, const long lDataLen)
{
   STACKTRACE
   init();

   if(pData != NULL)
   {
      Read(pData, lDataLen);
   }

   // printf(" EDF::EDF root element %p\n", m_pRoot);
}
#endif

EDF::EDF(EDF *pEDF)
{
   STACKTRACE
   init();

   if(pEDF != NULL)
   {
      Copy(pEDF, true, true);
   }
}

EDF::EDF(EDFElement *pElement)
{
   STACKTRACE
   init();

   if(pElement != NULL)
   {
      delete m_pRoot;

      m_pRoot = pElement;
      while(m_pRoot->parent() != NULL)
      {
         m_pRoot = m_pRoot->parent();
      }

      m_pCurr = m_pRoot;
   }
}

// Destructor
EDF::~EDF()
{
   STACKTRACE
   double dTick = gettick();

   // printf(" EDF::~EDF deleting root %p\n", m_pRoot);

   delete m_pRoot;
   if(tickdiff(dTick) > 250)
   {
      debug("EDF::~EDF delete time %ld ms\n", tickdiff(dTick));
   }

   sortDelete(m_pSortRoot);
}


// Current name and value retrieval methods (methods protect against EDFElementExceptions by checking type first)
bool EDF::Get(char **szName)
{
   STACKTRACE
   if(szName != NULL)
   {
      *szName = m_pCurr->getName(m_bDataCopy);
   }

   return true;
}

bool EDF::Get(char **szName, char **szValue)
{
   STACKTRACE
   if(szName != NULL)
   {
      *szName = m_pCurr->getName(m_bDataCopy);
   }

   STACKTRACEUPDATE

   if(m_pCurr->getType() != EDFElement::BYTES && m_pCurr->getType() != EDFElement::NONE)
   {
      return false;
   }

   STACKTRACEUPDATE

   if(szValue != NULL)
   {
      *szValue = m_pCurr->getValueStr(m_bDataCopy);
   }

   STACKTRACEUPDATE

   return true;
}

bool EDF::Get(char **szName, bytes **pValue)
{
   STACKTRACE
   // long lReturn = 0;

   if(szName != NULL)
   {
      *szName = m_pCurr->getName(m_bDataCopy);
   }

   if(m_pCurr->getType() != EDFElement::BYTES && m_pCurr->getType() != EDFElement::NONE)
   {
      return false;
   }

   if(pValue != NULL)
   {
      // lReturn = m_pCurr->getValueByte(pValue, m_bGetCopy);
      *pValue = m_pCurr->getValueBytes(m_bDataCopy);
   }
   /* if(lValueLen != NULL)
   {
      *lValueLen = lReturn;
   } */

   return true;
}

bool EDF::Get(char **szName, int *iValue)
{
   long lValue = 0;

   if(Get(szName, &lValue) == false)
   {
      return false;
   }

   *iValue = lValue;
   return true;
}

bool EDF::Get(char **szName, long *lValue)
{
   STACKTRACE
   if(szName != NULL)
   {
      *szName = m_pCurr->getName(m_bDataCopy);
   }

   if(m_pCurr->getType() != EDFElement::INT && m_pCurr->getType() != EDFElement::FLOAT && m_pCurr->getType() != EDFElement::NONE)
   {
      // printf("EDF::Get invalid type for integer get\n");
      return false;
   }

   // printf("EDF::Get %s %d\n", m_pCurr->getName(false), m_pCurr->getValueInt());

   if((m_pCurr->getType() == EDFElement::INT || m_pCurr->getType() == EDFElement::FLOAT) && lValue != NULL)
   {
      (*lValue) = m_pCurr->getValueInt();
   }

   return true;
}

bool EDF::Get(char **szName, double *dValue)
{
   STACKTRACE
   if(szName != NULL)
   {
      *szName = m_pCurr->getName(m_bDataCopy);
   }

   if(m_pCurr->getType() != EDFElement::INT && m_pCurr->getType() != EDFElement::FLOAT && m_pCurr->getType() != EDFElement::NONE)
   {
      // printf("EDF::Get invalid type for integer get\n");
      return false;
   }

   // printf("EDF::Get %s %d\n", m_pCurr->getName(false), m_pCurr->getValueInt());

   if((m_pCurr->getType() == EDFElement::INT || m_pCurr->getType() == EDFElement::FLOAT) && dValue != NULL)
   {
      (*dValue) = m_pCurr->getValueFloat();
   }

   return true;
}

// Current name and value type independent retrieval methods
int EDF::TypeGet(char **szName, char **szValue, long *lValue, double *dValue)
{
   STACKTRACE
   int iReturn = 0;
   bytes *pValue = NULL;

   iReturn = TypeGet(szName, szValue != NULL ? &pValue : NULL, lValue, dValue);

   if(pValue != NULL)
   {
      if(m_bDataCopy == true)
      {
         *szValue = strmk((char *)pValue->Data(false), 0, pValue->Length());
      }
      else
      {
         *szValue = (char *)pValue->Data(false);
      }
   }

   if(m_bDataCopy == true)
   {
      delete pValue;
   }

   return iReturn;
}

int EDF::TypeGet(char **szName, bytes **pValue, long *lValue, double *dValue)
{
   STACKTRACE
   int iType = m_pCurr->getType();
   // long lTemp = 0;

   if(szName != NULL)
   {
      *szName = m_pCurr->getName(m_bDataCopy);
   }

   if(iType == EDFElement::BYTES && pValue != NULL)
   {
      /* lTemp = m_pCurr->getValueByte(pValue, m_bGetCopy);
      if(lValueLen != NULL)
      {
         *lValueLen = lTemp;
      } */
      (*pValue) = m_pCurr->getValueBytes(m_bDataCopy);
   }
   else if(iType == EDFElement::INT && lValue != NULL)
   {
      *lValue = m_pCurr->getValueInt();
   }
   else if(iType == EDFElement::FLOAT && dValue != NULL)
   {
      *dValue = m_pCurr->getValueFloat();
   }

   return iType;
}


bool EDF::Set(const char *szName, const char *szValue)
{
   STACKTRACE
   m_pCurr->set(szName);
   m_pCurr->setValue(szValue);

   return true;
}

bool EDF::Set(const char *szName, bytes *pValue)
{
   STACKTRACE
   m_pCurr->set(szName);
   m_pCurr->setValue(pValue);

   return true;
}

bool EDF::Set(const char *szName, const int iValue)
{
   STACKTRACE
   m_pCurr->set(szName);
   m_pCurr->setValue((long)iValue);

   return true;
}

bool EDF::Set(const char *szName, const long lValue)
{
   STACKTRACE
   m_pCurr->set(szName);
   m_pCurr->setValue(lValue);

   return true;
}

bool EDF::Set(const char *szName, const unsigned long lValue)
{
   STACKTRACE
   m_pCurr->set(szName);
   m_pCurr->setValue((double)lValue);

   return true;
}

bool EDF::Set(const char *szName, const double dValue)
{
   STACKTRACE
   m_pCurr->set(szName);
   m_pCurr->setValue(dValue);

   return true;
}


// Add methods are convenience functions which map to private method
bool EDF::Add(const char *szName, const char *szValue, const int iPosition)
{
   STACKTRACE

   // return add(szName, EDFElement::BYTES, (byte *)szValue, strlen(szValue), 0, iPosition, true);
   m_pCurr = new EDFElement(m_pCurr, szName, szValue, iPosition);

   return true;
}

bool EDF::Add(const char *szName, bytes *pValue, const int iPosition)
{
   STACKTRACE

   // return add(szName, EDFElement::BYTES, pValue, lValueLen, 0, iPosition, true);
   m_pCurr = new EDFElement(m_pCurr, szName, pValue, iPosition);

   return true;
}

bool EDF::Add(const char *szName, const int iValue, const int iPosition)
{
   STACKTRACE

   // return add(szName, EDFElement::INT, NULL, 0, iValue, iPosition, true);
   m_pCurr = new EDFElement(m_pCurr, szName, (long)iValue, iPosition);

   return true;
}

bool EDF::Add(const char *szName, const long lValue, const int iPosition)
{
   STACKTRACE

   // return add(szName, EDFElement::INT, NULL, 0, lValue, iPosition, true);
   m_pCurr = new EDFElement(m_pCurr, szName, lValue, iPosition);

   return true;
}

bool EDF::Add(const char *szName, const double dValue, const int iPosition)
{
   STACKTRACE

   // return add(szName, EDFElement::INT, NULL, 0, dValue, iPosition, true);
   m_pCurr = new EDFElement(m_pCurr, szName, dValue, iPosition);

   return true;
}

// Delete method removeds the current element
bool EDF::Delete()
{
   STACKTRACE
   EDFElement *pParent = NULL;

   if(m_pCurr == m_pRoot)
   {
      // Cannot delete the root element
      return false;
   }

   // printf("EDF::Delete finding parent\n");
   pParent = m_pCurr->parent();

   // printf("EDF::Delete deleting current\n");
   delete m_pCurr;

   // printf("EDF::Delete resetting current\n");
   m_pCurr = pParent;

   // printf("EDF::Delete exit true\n");
   return true;
}


bool EDF::GetChild(const char *szName, char **szValue, int iPosition)
{
   STACKTRACE
   EDFElement *pChild = NULL;

   pChild = m_pCurr->child(szName, (bytes *)NULL, iPosition);
   if(pChild == NULL)
   {
      return false;
   }

   if(pChild->getType() == EDFElement::BYTES && szValue != NULL)
   {
      *szValue = pChild->getValueStr(m_bDataCopy);
   }
   else if(pChild->getType() == EDFElement::NONE && szValue != NULL)
   {
      *szValue = NULL;
   }

   return true;
}

bool EDF::GetChild(const char *szName, bytes **pValue, int iPosition)
{
   STACKTRACE
   // long lTemp = 0;
   EDFElement *pChild = NULL;

   pChild = m_pCurr->child(szName, (bytes *)NULL, iPosition);
   if(pChild == NULL)
   {
      return false;
   }

   if(pChild->getType() == EDFElement::BYTES && pValue != NULL)
   {
      /* lTemp = pChild->getValueByte(pValue, m_bGetCopy);
      if(lValueLen != NULL)
      {
         *lValueLen = lTemp;
      } */
      (*pValue) = pChild->getValueBytes(m_bDataCopy);
   }

   return true;
}

bool EDF::GetChild(const char *szName, int *iValue, int iPosition)
{
   STACKTRACE
   long lValue = 0;
   if(GetChild(szName, &lValue, iPosition) == false)
   {
      return false;
   }

   *iValue = lValue;

   return true;
}

bool EDF::GetChild(const char *szName, long *lValue, int iPosition)
{
   STACKTRACE
   EDFElement *pChild = NULL;

   pChild = m_pCurr->child(szName, (bytes *)NULL, iPosition);
   if(pChild == NULL)
   {
      return false;
   }

   if((pChild->getType() == EDFElement::INT || pChild->getType() == EDFElement::FLOAT) && lValue != NULL)
   {
      *lValue = pChild->getValueInt();
   }

   return true;
}

bool EDF::GetChild(const char *szName, unsigned long *lValue, int iPosition)
{
   STACKTRACE
   EDFElement *pChild = NULL;

   pChild = m_pCurr->child(szName, (bytes *)NULL, iPosition);
   if(pChild == NULL)
   {
      return false;
   }

   if((pChild->getType() == EDFElement::INT || pChild->getType() == EDFElement::FLOAT) && lValue != NULL)
   {
      *lValue = (unsigned long)pChild->getValueFloat();
   }

   return true;
}

bool EDF::GetChild(const char *szName, double *dValue, int iPosition)
{
   STACKTRACE
   EDFElement *pChild = NULL;

   pChild = m_pCurr->child(szName, (bytes *)NULL, iPosition);
   if(pChild == NULL)
   {
      return false;
   }

   if((pChild->getType() == EDFElement::INT || pChild->getType() == EDFElement::FLOAT) && dValue != NULL)
   {
      *dValue = pChild->getValueFloat();
   }

   return true;
}

bool EDF::GetChildBool(const char *szName, const bool bDefault, int iPosition)
{
   STACKTRACE
   EDFElement *pChild = NULL;

   pChild = m_pCurr->child(szName, (bytes *)NULL, iPosition);
   if(pChild == NULL)
   {
      return bDefault;
   }

   if(pChild->getType() == EDFElement::BYTES)
   {
      if(stricmp(pChild->getValueStr(false), "y") == 0 || stricmp(pChild->getValueStr(false), "yes") == 0 || stricmp(pChild->getValueStr(false), "true") == 0 || stricmp(pChild->getValueStr(false), "1") == 0)
      {
         return true;
      }
      else
      {
         return false;
      }
   }
   else if(pChild->getType() == EDFElement::INT || pChild->getType() == EDFElement::FLOAT)
   {
      if(pChild->getValueInt() == 1)
      {
         return true;
      }
      else
      {
         return false;
      }
   }

   return bDefault;
}

int EDF::TypeGetChild(const char *szName, char **szValue, long *lValue, double *dValue, int iPosition)
{
   STACKTRACE
   int iType = 0;
   // long lTemp = 0;
   EDFElement *pChild = NULL;

   pChild = m_pCurr->child(szName, (bytes *)NULL, iPosition);
   if(pChild == NULL)
   {
      return -1;
   }

   iType = pChild->getType();
   if(iType == EDFElement::BYTES && szValue != NULL)
   {
      /* lTemp = pChild->getValueByte(pValue, m_bGetCopy);
      if(lValueLen != NULL)
      {
         *lValueLen = lTemp;
      } */
      (*szValue) = pChild->getValueStr(m_bDataCopy);
   }
   else if(iType == EDFElement::INT && lValue != NULL)
   {
      *lValue = pChild->getValueInt();
   }
   else if(iType == EDFElement::FLOAT && dValue != NULL)
   {
      *dValue = pChild->getValueFloat();
   }

   return iType;
}

int EDF::TypeGetChild(const char *szName, bytes **pValue, long *lValue, double *dValue, int iPosition)
{
   STACKTRACE
   int iType = 0;
   // long lTemp = 0;
   EDFElement *pChild = NULL;

   pChild = m_pCurr->child(szName, (bytes *)NULL, iPosition);
   if(pChild == NULL)
   {
      return -1;
   }

   iType = pChild->getType();
   if(iType == EDFElement::BYTES && pValue != NULL)
   {
      /* lTemp = pChild->getValueByte(pValue, m_bGetCopy);
      if(lValueLen != NULL)
      {
         *lValueLen = lTemp;
      } */
      (*pValue) = pChild->getValueBytes(m_bDataCopy);
   }
   else if(iType == EDFElement::INT && lValue != NULL)
   {
      *lValue = pChild->getValueInt();
   }
   else if(iType == EDFElement::FLOAT && dValue != NULL)
   {
      *dValue = pChild->getValueFloat();
   }

   return iType;
}


bool EDF::SetChild(const char *szName, const char *szValue, int iPosition)
{
   STACKTRACE
   EDFElement *pChild = NULL;

   // printf("EDF::SetChild '%s' '%s' %d\n", szName, szValue, iPosition);

   pChild = m_pCurr->child(szName, (bytes *)NULL, iPosition);
   if(pChild == NULL)
   {
      // add(szName, EDFElement::BYTES, (byte *)szValue, strlen(szValue), 0, EDFElement::ABSLAST, false);
      pChild = new EDFElement(m_pCurr, szName, szValue, EDFElement::FIRST);
      return true;
   }

   pChild->setValue(szValue);

   return true;
}

bool EDF::SetChild(const char *szName, bytes *pValue, int iPosition)
{
   STACKTRACE
   EDFElement *pChild = NULL;

   // printf("EDF::SetChild '%s' %p %ld %d\n", szName, pValue, lValueLen, iPosition);

   pChild = m_pCurr->child(szName, (bytes *)NULL, iPosition);
   if(pChild == NULL)
   {
      // add(szName, EDFElement::BYTES, pValue, lValueLen, 0, EDFElement::ABSLAST, false);
      pChild = new EDFElement(m_pCurr, szName, pValue, EDFElement::FIRST);
      return true;
   }

   pChild->setValue(pValue);

   return true;
}

bool EDF::SetChild(const char *szName, const int iValue, int iPosition)
{
   STACKTRACE
   EDFElement *pChild = NULL;

   // printf("EDF::SetChild '%s' %d %d\n", szName, iValue, iPosition);

   pChild = m_pCurr->child(szName, (bytes *)NULL, iPosition);
   if(pChild == NULL)
   {
      // add(szName, EDFElement::INT, NULL, 0, iValue, EDFElement::ABSLAST, false);
      pChild = new EDFElement(m_pCurr, szName, (long)iValue, EDFElement::FIRST);
      return true;
   }

   pChild->setValue((long)iValue);

   return true;
}

bool EDF::SetChild(const char *szName, const bool bValue, int iPosition)
{
   STACKTRACE
   EDFElement *pChild = NULL;

   // printf("EDF::SetChild '%s' %s %d\n", szName, BoolStr(bValue), iPosition);

   pChild = m_pCurr->child(szName, (bytes *)NULL, iPosition);
   if(pChild == NULL)
   {
      // add(szName, EDFElement::INT, NULL, 0, iValue, EDFElement::ABSLAST, false);
      pChild = new EDFElement(m_pCurr, szName, (long)bValue, EDFElement::FIRST);
      return true;
   }

   pChild->setValue((long)bValue);

   return true;
}

bool EDF::SetChild(const char *szName, const long lValue, int iPosition)
{
   STACKTRACE
   EDFElement *pChild = NULL;

   // printf("EDF::SetChild '%s' %ld %d\n", szName, lValue, iPosition);

   pChild = m_pCurr->child(szName, (bytes *)NULL, iPosition);
   if(pChild == NULL)
   {
      // add(szName, EDFElement::INT, NULL, 0, lValue, EDFElement::ABSLAST, false);
      pChild = new EDFElement(m_pCurr, szName, lValue, EDFElement::FIRST);
      return true;
   }

   pChild->setValue(lValue);

   return true;
}

bool EDF::SetChild(const char *szName, const unsigned long lValue, int iPosition)
{
   STACKTRACE
   EDFElement *pChild = NULL;

   // printf("EDF::SetChild '%s' %ld %d\n", szName, lValue, iPosition);

   pChild = m_pCurr->child(szName, (bytes *)NULL, iPosition);
   if(pChild == NULL)
   {
      // add(szName, EDFElement::INT, NULL, 0, (double)lValue, EDFElement::ABSLAST, false);
      pChild = new EDFElement(m_pCurr, szName, (double)lValue, EDFElement::FIRST);
      return true;
   }

   pChild->setValue((double)lValue);

   return true;
}

bool EDF::SetChild(const char *szName, const double dValue, int iPosition)
{
   STACKTRACE
   EDFElement *pChild = NULL;

   // printf("EDF::SetChild '%s' %ld %d\n", szName, lValue, iPosition);

   pChild = m_pCurr->child(szName, (bytes *)NULL, iPosition);
   if(pChild == NULL)
   {
      // add(szName, EDFElement::INT, NULL, 0, dValue, EDFElement::ABSLAST, false);
      pChild = new EDFElement(m_pCurr, szName, dValue, EDFElement::FIRST);
      return true;
   }

   pChild->setValue(dValue);

   return true;
}

bool EDF::SetChild(EDF *pEDF)
{
   STACKTRACE
   // long lValueLen = 0;
   // bytes *pValue = NULL;
   EDFElement *pElement = NULL, *pChild = NULL;

   // printf("EDF::SetChild entry %p", pEDF);
   pElement = pEDF->GetCurr();
   // printf(", %p", pElement);
   // printf("(%s)", pElement->getName(false));

   pChild = m_pCurr->child(pElement->getName(false));
   // printf(", %p\n", pChild);
   if(pChild == NULL)
   {
      if(pElement->getType() == EDFElement::BYTES)
      {
         // lValueLen = pElement->getValueByte(&pValue, false);
         // AddChild(pElement->getName(false), pValue, lValueLen);
         AddChild(pElement->getName(false), pElement->getValueBytes(false));
      }
      else if(pElement->getType() == EDFElement::INT)
      {
         AddChild(pElement->getName(false), pElement->getValueInt());
      }
      else if(pElement->getType() == EDFElement::FLOAT)
      {
         AddChild(pElement->getName(false), pElement->getValueFloat());
      }
      else
      {
         AddChild(pElement->getName(false));
      }
      return true;
   }

   if(pElement->getType() == EDFElement::BYTES)
   {
      // lValueLen = pElement->getValueByte(&pValue, false);
      // pChild->setValue(pValue, lValueLen);
      pChild->setValue(pElement->getValueBytes(false));
   }
   else if(pElement->getType() == EDFElement::INT)
   {
      pChild->setValue(pElement->getValueInt());
   }
   else if(pElement->getType() == EDFElement::FLOAT)
   {
      pChild->setValue(pElement->getValueFloat());
   }
   else
   {
      pChild->setValue((char *)NULL);
   }

   return true;
}

bool EDF::SetChild(EDF *pEDF, const char *szName, int iPosition)
{
   STACKTRACE
   bool bDataCopy = pEDF->DataCopy(false);
   int iType = 0;
   // long lValueLen = 0;
   long lValue = 0;
   double dValue = 0;
   bytes *pValue = NULL;

   // printf("EDF::SetChild %p '%s' %d\n", pEDF, szName, iPosition);

   iType = pEDF->TypeGetChild(szName, &pValue, &lValue, &dValue, iPosition);
   if(iType == -1)
   {
      pEDF->DataCopy(bDataCopy);

      return false;
   }

   if(iType == EDFElement::BYTES)
   {
      SetChild(szName, pValue);
      // delete[] pValue;
   }
   else if(iType == EDFElement::INT)
   {
      SetChild(szName, lValue);
   }
   else if(iType == EDFElement::FLOAT)
   {
      SetChild(szName, dValue);
   }

   pEDF->DataCopy(bDataCopy);

   return true;
}


bool EDF::AddChild(const char *szName, const char *szValue, int iPosition)
{
   STACKTRACE
   EDFElement *pChild = NULL;

   // return add(szName, EDFElement::BYTES, (byte *)szValue, strlen(szValue), 0, iPosition, false);
   pChild = new EDFElement(m_pCurr, szName, szValue, iPosition);

   return true;
}

bool EDF::AddChild(const char *szName, bytes *pValue, int iPosition)
{
   STACKTRACE
   EDFElement *pChild = NULL;

   // return add(szName, EDFElement::BYTES, (byte *)szValue, strlen(szValue), 0, iPosition, false);
   pChild = new EDFElement(m_pCurr, szName, pValue, iPosition);

   return true;
}

bool EDF::AddChild(const char *szName, const int iValue, int iPosition)
{
   STACKTRACE
   EDFElement *pChild = NULL;

   // return add(szName, EDFElement::BYTES, (byte *)szValue, strlen(szValue), 0, iPosition, false);
   pChild = new EDFElement(m_pCurr, szName, (long)iValue, iPosition);

   return true;
}

bool EDF::AddChild(const char *szName, const bool bValue, int iPosition)
{
   STACKTRACE
   EDFElement *pChild = NULL;

   // return add(szName, EDFElement::BYTES, (byte *)szValue, strlen(szValue), 0, iPosition, false);
   pChild = new EDFElement(m_pCurr, szName, (long)bValue, iPosition);

   return true;
}

bool EDF::AddChild(const char *szName, const long lValue, int iPosition)
{
   STACKTRACE
   EDFElement *pChild = NULL;

   // return add(szName, EDFElement::BYTES, (byte *)szValue, strlen(szValue), 0, iPosition, false);
   pChild = new EDFElement(m_pCurr, szName, lValue, iPosition);

   return true;
}

bool EDF::AddChild(const char *szName, const double dValue, int iPosition)
{
   STACKTRACE
   EDFElement *pChild = NULL;

   // return add(szName, EDFElement::BYTES, (byte *)szValue, strlen(szValue), 0, iPosition, false);
   pChild = new EDFElement(m_pCurr, szName, dValue, iPosition);

   return true;
}

bool EDF::AddChild(EDF *pEDF)
{
   STACKTRACE
   // long lValueLen = 0;
   // bytes *pValue = NULL;
   EDFElement *pElement = pEDF->GetCurr(), *pNew = NULL;

   // m_pCurr->print("EDF::AddChild entry");
   // pElement->print("EDF::AddChild source");

   if(pElement->getType() == EDFElement::BYTES)
   {
      // lValueLen = pElement->getValueByte(&pValue, false);
      // pNew = new EDFElement(m_pCurr, pElement->getName(false), pValue, lValueLen);
      pNew = new EDFElement(m_pCurr, pElement->getName(false), pElement->getValueBytes(false));
   }
   else if(pElement->getType() == EDFElement::INT)
   {
      pNew = new EDFElement(m_pCurr, pElement->getName(false), pElement->getValueInt());
   }
   else if(pElement->getType() == EDFElement::FLOAT)
   {
      pNew = new EDFElement(m_pCurr, pElement->getName(false), pElement->getValueFloat());
   }
   else
   {
      pNew = new EDFElement(m_pCurr, pElement->getName(false));
   }

   // m_pCurr->print("EDF::AddChild exit");
   return true;
}

bool EDF::AddChild(EDF *pEDF, const char *szName, int iPosition)
{
   return AddChild(pEDF, szName, szName, iPosition);
}

bool EDF::AddChild(EDF *pEDF, const char *szName, const char *szNewName, int iPosition)
{
   STACKTRACE
   bool bDataCopy = pEDF->DataCopy(false);
   int iType = 0;
   // long lValueLen = 0;
   long lValue = 0;
   double dValue = 0;
   bytes *pValue = NULL;
   EDFElement *pChild = NULL;

   iType = pEDF->TypeGetChild(szName, &pValue, &lValue, &dValue, iPosition);
   if(iType == -1)
   {
      pEDF->DataCopy(bDataCopy);

      return false;
   }

   if(iType == EDFElement::BYTES)
   {
      // add(szName, EDFElement::BYTES, pValue, lValueLen, 0, EDFElement::ABSLAST, false);
      pChild = new EDFElement(m_pCurr, szNewName, pValue);
      // delete[] pValue;
   }
   else if(iType == EDFElement::INT)
   {
      // add(szName, EDFElement::INT, NULL, 0, lValue, EDFElement::ABSLAST, false);
      pChild = new EDFElement(m_pCurr, szNewName, lValue);
   }
   else if(iType == EDFElement::FLOAT)
   {
      // add(szName, EDFElement::FLOAT, NULL, 0, lValue, EDFElement::ABSLAST, false);
      pChild = new EDFElement(m_pCurr, szNewName, dValue);
   }

   pEDF->DataCopy(bDataCopy);

   return true;
}


bool EDF::DeleteChild(const char *szName, const int iPosition)
{
   STACKTRACE
   EDFElement *pChild = NULL;

   // m_pCurr->print("EDF::DeleteChild");

   pChild = m_pCurr->child(szName, (bytes *)NULL, iPosition);
   if(pChild == NULL)
   {
      return false;
   }

   delete pChild;

   // m_pCurr->print("EDF::DeleteChild");

   return true;
}


int EDF::Children(const char *szName, const bool bRecurse)
{
   STACKTRACE
   return m_pCurr->children(szName, bRecurse);
}


bool EDF::Root()
{
   STACKTRACE
   m_pCurr = m_pRoot;

   return true;
}


bool EDF::Child(const char *szName, const int iPosition)
{
   STACKTRACE
   EDFElement *pChild = NULL;

   pChild = m_pCurr->child(szName, (bytes *)NULL, iPosition);
   if(pChild == NULL)
   {
      return false;
   }

   m_pCurr = pChild;

   return true;
}

bool EDF::Child(const char *szName, const char *szValue, const int iPosition)
{
   STACKTRACE
   EDFElement *pChild = NULL;

   pChild = m_pCurr->child(szName, szValue, iPosition);
   if(pChild == NULL)
   {
      return false;
   }

   m_pCurr = pChild;

   return true;
}

bool EDF::Child(const char *szName, bytes *pValue, const int iPosition)
{
   STACKTRACE
   EDFElement *pChild = NULL;

   pChild = m_pCurr->child(szName, pValue, iPosition);
   if(pChild == NULL)
   {
      return false;
   }

   m_pCurr = pChild;

   return true;
}

bool EDF::IsChild(const char *szName, const char *szValue, const int iPosition)
{
   STACKTRACE
   EDFElement *pChild = NULL;

   // printf("EDF::IsChild '%s' '%s' %d\n", szName, szValue, iPosition);

   pChild = m_pCurr->child(szName, szValue, iPosition);
   if(pChild == NULL)
   {
      return false;
   }

   return true;
}

bool EDF::First(const char *szName)
{
   STACKTRACE
   EDFElement *pChild = NULL;

   pChild = m_pCurr->child(szName, (bytes *)NULL, EDFElement::FIRST);
   if(pChild == NULL)
   {
      return false;
   }

   m_pCurr = pChild;

   return true;
}

bool EDF::Last(const char *szName)
{
   STACKTRACE
   EDFElement *pChild = NULL;

   pChild = m_pCurr->child(szName, (bytes *)NULL, EDFElement::LAST);
   if(pChild == NULL)
   {
      return false;
   }

   m_pCurr = pChild;

   return true;
}

bool EDF::Next(const char *szName)
{
   STACKTRACE
   int iChildNum = 0;
   EDFElement *pParent = NULL;

   pParent = m_pCurr->parent();
   if(pParent == NULL)
   {
      return false;
   }

   iChildNum = pParent->child(m_pCurr);

   do
   {
      iChildNum++;
   }
   // while(szName != NULL && iChildNum < pParent->children() && stricmp(pParent->child(iChildNum)->getName(false), m_pCurr->getName(false)) != 0);
   while(szName != NULL && iChildNum < pParent->children() && stricmp(pParent->child(iChildNum)->getName(false), szName) != 0);

   if(iChildNum == pParent->children())
   {
      return false;
   }

   m_pCurr = pParent->child(iChildNum);

   return true;
}

bool EDF::Prev(const char *szName)
{
   STACKTRACE
   int iChildNum = 0;
   EDFElement *pParent = NULL;

   pParent = m_pCurr->parent();
   if(pParent == NULL)
   {
      return false;
   }

   iChildNum = pParent->child(m_pCurr);

   do
   {
      iChildNum--;
   }
   // while(szName != NULL && iChildNum >= 0 && stricmp(pParent->child(iChildNum)->getName(false), m_pCurr->getName(false)) != 0);
   while(szName != NULL && iChildNum >= 0 && stricmp(pParent->child(iChildNum)->getName(false), szName) != 0);

   if(iChildNum < 0)
   {
      return false;
   }

   m_pCurr = pParent->child(iChildNum);

   return true;
}

bool EDF::Parent()
{
   STACKTRACE

   if(m_pCurr->parent() == NULL)
   {
      return false;
   }

   m_pCurr = m_pCurr->parent();

   return true;
}


bool EDF::Iterate(const char *szIter, const char *szStop, const bool bMatch, const bool bChild)
{
   STACKTRACE
   bool bIter = true, bNext = false;
   int iStopLen = 0;

   if(szStop != NULL)
   {
      iStopLen = strlen(szStop);
   }

   // printf("EDF::Iterate entry %s %d\n", m_pCurr->getName(false), m_pCurr->getType() == EDFElement::INT ? m_pCurr->getValueInt() : -1);

   // printf("EDF::Iterate entry %s, %s %s %s\n", szIter, szStop, BoolStr(bFullMatch), BoolStr(bChild));
   if(bChild == false || Child(szIter) == false)
   {
      // No child iterates, move to next iterate
      bNext = Next(szIter);
      // printf("EDF::Iterate move to next %s\n", BoolStr(bNext));

      while(bNext == false)
      {
         // printf("EDF::Iterate move %s %d\n", m_pCurr->getName(false), m_pCurr->getType() == EDFElement::INT ? m_pCurr->getValueInt() : -1);

         // No next iterate, move to one after parent
         if(szStop != NULL &&
            (bMatch == true && stricmp(m_pCurr->getName(false), szStop) == 0) ||
            (bMatch == false && strnicmp(m_pCurr->getName(false), szStop, iStopLen) == 0))
         {
            bNext = true;
            bIter = false;
         }
         else
         {
            bNext = Parent();

            // bNext is false if we're at the root (this will trap NULL values of szStop)
            if(bNext == false || (szStop != NULL &&
               (bMatch == true && stricmp(m_pCurr->getName(false), szStop) == 0) ||
               (bMatch == false && strnicmp(m_pCurr->getName(false), szStop, iStopLen) == 0)))
            {
               // Reached stop element
               bNext = true;
               bIter = false;

               // printf("EDF::Iterate stop iterating\n");
            }
            else
            {
               // Move to next iterate
               bNext = Next(szIter);
               // printf("EDF::Iterate move to next %s\n", BoolStr(bNext));
            }
         }
      }
   }
   /* else
   {
      debug("EDF::Iterate move to child\n");
   } */

   // printf("EDF::Iterate exit %s %d\n", m_pCurr->getName(false), m_pCurr->getType() == EDFElement::INT ? m_pCurr->getValueInt() : -1);

   return bIter;
}


bool EDF::SortReset(const char *szItems, const bool bRecurse)
{
   sortDelete(m_pSortRoot);

   m_pSortRoot = NULL;
   m_pSortCurr = NULL;

   return sortAdd(SORT_ITEM, szItems, bRecurse);
}

bool EDF::SortAddSection(const char *szName)
{
   return sortAdd(SORT_SECTION, szName, false);
}

bool EDF::SortAddKey(const char *szName, bool bAscending)
{
   return sortAdd(SORT_KEY, szName, bAscending);
}

bool EDF::SortParent()
{
   if(m_pSortCurr == m_pSortRoot)
   {
      return false;
   }

   m_pSortCurr = m_pSortCurr->m_pParent;

   return true;
}

bool EDF::Sort()
{
   // debugEDFPrint("EDF::Sort", this, EDFElement::EL_CURR | EDFElement::PR_SPACE);

   return m_pCurr->sort(m_pSortRoot);
}

bool EDF::Sort(const char *szItems, const char *szKey, const bool bRecurse, const bool bAscending)
{
   STACKTRACE

   SortReset(szItems, bRecurse);
   if(SortAddKey(szKey, bAscending) == false)
   {
      return false;
   }

   return Sort();
}


bool EDF::Copy(EDF *pEDF, const bool bSrcCurr, const bool bDestSet, const bool bRecurse)
{
   STACKTRACE
   int iChildNum = 0;
   // long lValueLen = 0;
   // bytes *pValue = NULL;
   EDFElement *pElement = pEDF->GetCurr();

   if(bSrcCurr == true)
   {
      if(bDestSet == true)
      {
         m_pCurr->set(pElement->getName(false));
         if(pElement->getType() == EDFElement::BYTES)
         {
            // lValueLen = pElement->getValueByte(&pValue, false);
            // m_pCurr->setValue(pValue, lValueLen);
            m_pCurr->setValue(pElement->getValueBytes(false));
         }
         else if(pElement->getType() == EDFElement::INT)
         {
            m_pCurr->setValue(pElement->getValueInt());
         }
         else if(pElement->getType() == EDFElement::FLOAT)
         {
            m_pCurr->setValue(pElement->getValueFloat());
         }
         else
         {
            m_pCurr->setValue((char *)NULL);
         }

         for(iChildNum = 0; iChildNum < pElement->children(); iChildNum++)
         {
            m_pCurr->copy(pElement->child(iChildNum), bRecurse);
         }
      }
      else
      {
         m_pCurr->copy(pElement, bRecurse);
      }
   }
   else
   {
      for(iChildNum = 0; iChildNum < pElement->children(); iChildNum++)
      {
         m_pCurr->copy(pElement->child(iChildNum), bRecurse);
      }
   }

   return true;
}


bool EDF::DataCopy(bool bDataCopy)
{
   STACKTRACE

   m_bDataCopy = bDataCopy;

   return true;
}

bool EDF::DataCopy()
{
   STACKTRACE

   return m_bDataCopy;
}


bool EDF::TempMark()
{
   STACKTRACE

   m_pTempMark = m_pCurr;

   return true;
}

bool EDF::TempUnmark()
{
   STACKTRACE

   if(m_pTempMark == NULL)
   {
      return false;
   }

   m_pCurr = m_pTempMark;

   return true;
}

EDFElement *EDF::GetCurr()
{
   STACKTRACE
   return m_pCurr;
}

bool EDF::SetCurr(EDFElement *pElement)
{
   STACKTRACE
   EDFElement *pCurr = NULL;

   // printf("EDF::SetCurr %p\n", pElement);

   if(pElement == NULL)
   {
      return false;
   }

   pCurr = pElement;
   while(pCurr->parent() != NULL)
   {
      pCurr = pCurr->parent();
   }

   // printf("EDF::SetCurr root %p %p\n", pCurr, m_pCurr);
   if(pCurr != m_pRoot)
   {
      return false;
   }

   m_pCurr = pElement;

   return true;
}

bool EDF::SetRoot(EDFElement *pElement)
{
   STACKTRACE

   debug(DEBUGLEVEL_DEBUG, "EDF::SetRoot %p\n", this);

   debug(DEBUGLEVEL_DEBUG, "EDF::SetRoot %p, %p\n", pElement, m_pRoot);

   if(pElement == NULL)
   {
      return false;
   }

   delete m_pRoot;

   m_pRoot = pElement;
   while(m_pRoot->parent() != NULL)
   {
      m_pRoot = m_pRoot->parent();
   }

   m_pCurr = m_pRoot;

   debug(DEBUGLEVEL_DEBUG, "EDF::SetRoot %p(%p) %p\n", m_pRoot, m_pRoot->parent(), m_pCurr);

   return true;
}

bool EDF::MoveTo(EDFElement *pElement, const int iPosition)
{
   STACKTRACE

   m_pCurr->moveTo(pElement, iPosition);

   return true;
}

bool EDF::MoveFrom(EDFElement *pElement, const int iPosition)
{
   STACKTRACE

   m_pCurr->moveFrom(pElement, iPosition);

   return true;
}


int EDF::Depth()
{
   STACKTRACE
   int iDepth = 0;
   EDFElement *pCurr = m_pCurr;

   if(m_pCurr == NULL)
   {
      return -1;
   }

   while(pCurr->parent() != NULL)
   {
      pCurr = pCurr->parent();
      iDepth++;
   }

   return iDepth;
}

int EDF::Position(const bool bName)
{
   STACKTRACE
   int iPosition = 0, iReturn = 0;
   EDFElement *pCurr = m_pCurr;

   if(m_pCurr == NULL)
   {
      return -1;
   }

   pCurr = pCurr->parent();
   if(pCurr == NULL)
   {
      return 0;
   }

   while(pCurr->child(iPosition) != m_pCurr)
   {
      if(bName == false || stricmp(pCurr->child(iPosition)->getName(false), m_pCurr->getName(false)) == 0)
      {
         iReturn++;
      }
      iPosition++;
   }

   return iReturn;
}

long EDF::Storage(const int iOptions)
{
   STACKTRACE
   return m_pRoot->storage(iOptions);
}

/* bool EDF::Debug(const bool bDebug)
{
   STACKTRACE
   bool bReturn = m_bDebug;

   m_bDebug = bDebug;

   return bReturn;
} */


void EDF::init()
{
   STACKTRACE

   m_pRoot = new EDFElement();
   m_pCurr = m_pRoot;
   m_pTempMark = NULL;

   m_bDataCopy = true;

   m_pSortRoot = NULL;
   m_pSortCurr = NULL;
}

bool EDF::sortAdd(int iType, const char *szName, bool bOption)
{
   SortElement *pElement = NULL, **pTemp = NULL;

   // debug("EDF::sortAdd %d %s %s, %s\n", iType, szName, BoolStr(bOption), m_pSortCurr != NULL ? m_pSortCurr->m_szName : "no parent");

   /* if(szName == NULL)
   {
      return false;
   } */

   pElement = new SortElement;
   pElement->m_iType = iType;
   pElement->m_szName = strmk(szName);
   if(pElement->m_iType == SORT_ITEM && bOption == true)
   {
      pElement->m_iType |= SORT_RECURSE;
   }
   else if(pElement->m_iType == SORT_KEY && bOption == true)
   {
      pElement->m_iType |= SORT_ASCENDING;
   }
   pElement->m_pChildren = NULL;
   pElement->m_iNumChildren = 0;

   pElement->m_pParent = m_pSortCurr;
   if(m_pSortRoot == NULL)
   {
      m_pSortRoot = pElement;
   }
   else
   {
      ARRAY_INSERT(SortElement *, m_pSortCurr->m_pChildren, m_pSortCurr->m_iNumChildren, pElement, m_pSortCurr->m_iNumChildren, pTemp);
   }
   m_pSortCurr = pElement;

   return true;
}

bool EDF::sortDelete(SortElement *pElement)
{
   int iChildNum = 0;

   if(pElement == NULL)
   {
      return false;
   }

   for(iChildNum = 0; iChildNum < pElement->m_iNumChildren; iChildNum++)
   {
      sortDelete(pElement->m_pChildren[iChildNum]);
   }
   delete[] pElement->m_pChildren;

   delete[] pElement->m_szName;
   delete pElement;

   return true;
}

/* #define childcomp(x, y, z) \
(stricmp(x->getName(false), y) == 0 && (z == NULL || (z != NULL && x->getType() == EDFElement::BYTES && stricmp(x->getValueStr(false), z) == 0)))

EDFElement *EDF::child(const char *szName, const char *szValue, int iPosition)
{
   STACKTRACE
   int iChildNum = 0, iNumMatches = 0;

   debug("EDF::child '%s' '%s' %d", szName, szValue, iPosition);

   if(m_pCurr->children() == 0)
   {
      debug(", NULL\n");
      return NULL;
   }

   if(szName == NULL)
   {
      if(iPosition == EDFElement::ABSFIRST || iPosition == EDFElement::FIRST)
      {
         debug(", %p (%s)\n", m_pCurr->child(0), m_pCurr->child(0)->getName(false));
         return m_pCurr->child(0);
      }
      else if(iPosition == EDFElement::ABSLAST || iPosition == EDFElement::LAST)
      {
         debug(", %p (%s)\n", m_pCurr->child(m_pCurr->children() - 1), m_pCurr->child(m_pCurr->children() - 1)->getName(false));
         return m_pCurr->child(m_pCurr->children() - 1);
      }
      else if(iPosition >= 0 && iPosition < m_pCurr->children())
      {
         debug(", %p (%s)\n", m_pCurr->child(iPosition), m_pCurr->child(iPosition)->getName(false));
         return m_pCurr->child(iPosition);
      }
   }
   else
   {
      if(iPosition == EDFElement::ABSFIRST || iPosition == EDFElement::FIRST)
      {
         while(iChildNum < m_pCurr->children() && childcomp(m_pCurr->child(iChildNum), szName, szValue) == false)
         {
            iChildNum++;
         }
      }
      else if(iPosition == EDFElement::ABSLAST || iPosition == EDFElement::LAST)
      {
         iChildNum = m_pCurr->children() - 1;
         while(iChildNum >= 0 && childcomp(m_pCurr->child(iChildNum), szName, szValue) == false)
         {
            iChildNum--;
         }
      }
      else if(iPosition >= 0 && iPosition < m_pCurr->children())
      {
         while(iChildNum < m_pCurr->children() && iNumMatches <= iPosition)
         {
            if(childcomp(m_pCurr->child(iChildNum), szName, szValue) == true)
            {
               iNumMatches++;
               if(iNumMatches == iPosition)
               {
                  debug(", %p (%s)\n", m_pCurr->child(iChildNum), m_pCurr->child(iChildNum)->getName(false));
                  return m_pCurr->child(iChildNum);
               }
            }
            iChildNum++;
         }
      }
   }

   if(iChildNum < 0 || iChildNum >= m_pCurr->children())
   {
      debug(", NULL\n");
      return NULL;
   }

   debug(", %p (%s)\n", m_pCurr->child(iChildNum), m_pCurr->child(iChildNum)->getName(false));
   return m_pCurr->child(iChildNum);
} */

/* bool EDF::add(const char *szName, const int vType, const byte *pValue, const long lValueLen, const long lValue, const int iPosition, const bool bMoveTo)
{
   STACKTRACE
   EDFElement *pNew = NULL;

   debug("EDF::add '%s' %d %p %ld %ld %d %s, %p\n", szName, vType, pValue, lValueLen, lValue, iPosition, BoolStr(bMoveTo), m_pCurr);

   if(vType == EDFElement::BYTES)
   {
      pNew = new EDFElement(m_pCurr, szName, pValue, lValueLen, iPosition);
   }
   else if(vType == EDFElement::INT)
   {
      pNew = new EDFElement(m_pCurr, szName, lValue, iPosition);
   }
   else
   {
      pNew = new EDFElement(m_pCurr, szName, iPosition);
   }

   if(bMoveTo == true)
   {
      m_pCurr = pNew;
   }

   return true;
} */

int EDFMax(EDF *pEDF, const char *szName, bool bRecurse)
{
   int iID = 0, iMaxID = 0, iValue = 0;
   bool bLoop = false;

   // printf("EDFMax entry %p %s %s, %d children\n", pEDF, szName, BoolStr(bRecurse), pEDF->Children(szName));

   bLoop = pEDF->Child(szName);
   while(bLoop == true)
   {
      pEDF->Get(NULL, &iID);
      if(iID > iMaxID)
      {
         iMaxID = iID;
      }

      if(bRecurse == true)
      {
         iValue = EDFMax(pEDF, szName, true);
         if(iValue > iMaxID)
         {
            iMaxID = iValue;
            // debug("EDFMax %s %d\n", szName, iMaxID);
         }
      }

      bLoop = pEDF->Next(szName);
      if(bLoop == false)
      {
         pEDF->Parent();
      }
   }

   // printf("EDFMax exit %d\n", iMaxID);
   return iMaxID;
}

int EDFSetStr(EDF *pDest, const char *szName, bytes *pBytes, int iMax, int iOptions)
{
   int iLen = 0;
   bytes *pTemp = NULL;

   if(pBytes != NULL)
   {
      iLen = pBytes->Length();
      if(iLen > iMax)
      {
         debug("EDFSetStr cut string %ld to %d\n", pBytes->Length(), iMax);
         // szTemp = strmk(szValue, 0, iMax);
         pTemp = new bytes(pBytes, iMax);
      }
      else
      {
         // szTemp = strmk(szValue);
         pTemp = new bytes(pBytes);
      }

      pDest->SetChild(szName, pTemp);

      delete pTemp;
   }
   else if(mask(iOptions, EDFSET_REMOVEIFNULL) == true)
   {
      // printf("EDFSetStr NULL remove\n");
      pDest->DeleteChild(szName);
   }

   return iLen;
}

int EDFSetStr(EDF *pDest, EDF *pSrc, const char *szName, int iMax, int iOptions, const char *szDestName)
{
   STACKTRACE
   int iLen = -1;
   bytes *pValue = NULL;

   // printf("EDFSetStr %s %d %d\n", szName, iMax, iOptions);
   // EDFPrint(pSrc, false);

   if(pSrc->Child(szName) == true)
   {
      pSrc->Get(NULL, &pValue);
      iLen = EDFSetStr(pDest, szDestName != NULL ? szDestName : szName, pValue, iMax, iOptions);
      pSrc->Parent();
      delete pValue;
   }
   else if(mask(iOptions, EDFSET_REMOVEIFMISSING) == true)
   {
      // printf("EDFSetStr missing remove\n");
      pDest->DeleteChild(szName);
   }

   return iLen;
}

bool EDFFind(EDF *pEDF, const char *szName, int iID, bool bRecurse)
{
   bool bLoop = true;
   EDFElement *pElement = NULL;

   // debug("EDFFind entry %p %s %d %s\n", pEDF, szName, iID, BoolStr(bRecurse));

   bLoop = pEDF->Child(szName);
   while(bLoop == true)
   {
      pElement = pEDF->GetCurr();
      if(pElement->getType() == EDFElement::INT || pElement->getType() == EDFElement::FLOAT)
      {
         // debug("EDFFind value %ld\n", pElement->getValueInt());
         if(pElement->getValueInt() == iID)
         {
            // debug("EDFFind exit true, %p\n", pElement);
            return true;
         }
      }

      if(bRecurse == true)
      {
         if(EDFFind(pEDF, szName, iID, true) == true)
         {
            // debug("EDFFind exit true\n");
            return true;
         }
      }

      bLoop = pEDF->Next(szName);
      if(bLoop == false)
      {
         pEDF->Parent();
      }
   }

   // debug("EDFFind exit false\n");
   return false;
}

int EDFDelete(EDF *pEDF, char *szName, bool bRecurse)
{
   int iReturn = 0;
   bool bLoop = false;

   if(bRecurse == true)
   {
      bLoop = pEDF->Child();
      while(bLoop == true)
      {
         iReturn += EDFDelete(pEDF, szName, true);

         bLoop = pEDF->Next();
         if(bLoop == false)
         {
            pEDF->Parent();
         }
      }
   }

   while(pEDF->DeleteChild(szName) == true)
   {
      iReturn++;
   }

   return iReturn;
}
