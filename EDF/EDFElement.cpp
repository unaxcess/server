/*
** EDF - Encapsulated Data Format
** (c) 1998 Michael Wood (mike@compsoc.man.ac.uk)
**
** EDFElement.cpp: Implementation of EDFElement class
*/

#include "stdafx.h"

#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "EDFElement.h"

#define CHILD_INC 10

EDFElementTypeException::EDFElementTypeException(const char *szMessage)
{
   m_szMessage = strmk(szMessage);
}

EDFElementTypeException::~EDFElementTypeException()
{
   delete[] m_szMessage;
}

char *EDFElementTypeException::getMessage()
{
   return m_szMessage;
}


EDFElement::EDFElement()
{
   init(NULL, NULL, ABSLAST);
}

EDFElement::EDFElement(const char *szName)
{
   init(NULL, szName, ABSLAST);
}

EDFElement::EDFElement(const char *szName, const char *szValue, int iPosition)
{
   init(NULL, szName, iPosition);
   setValue(szValue);
}

EDFElement::EDFElement(const char *szName, bytes *pValue, int iPosition)
{
   init(NULL, szName, iPosition);
   setValue(pValue);
}

EDFElement::EDFElement(const char *szName, const long lValue, int iPosition)
{
   init(NULL, szName, iPosition);
   setValue(lValue);
}

EDFElement::EDFElement(const char *szName, const double dValue, int iPosition)
{
   init(NULL, szName, iPosition);
   setValue(dValue);
}

EDFElement::EDFElement(EDFElement *pParent, const char *szName, int iPosition)
{
   init(pParent, szName, iPosition);
}

EDFElement::EDFElement(EDFElement *pParent, const char *szName, const char *szValue, int iPosition)
{
   init(pParent, szName, iPosition);
   setValue(szValue);
}

EDFElement::EDFElement(EDFElement *pParent, const char *szName, bytes *pValue, int iPosition)
{
   init(pParent, szName, iPosition);
   setValue(pValue);
}

EDFElement::EDFElement(EDFElement *pParent, const char *szName, const long lValue, int iPosition)
{
   init(pParent, szName, iPosition);
   setValue(lValue);
}

EDFElement::EDFElement(EDFElement *pParent, const char *szName, const double dValue, int iPosition)
{
   init(pParent, szName, iPosition);
   setValue(dValue);
}

EDFElement::~EDFElement()
{
   // STACKTRACE
   int iChildNum = 0;

   // printf("EDFElement::~EDFElement, %p\n", this);

   // printf("EDFElement::~EDFElement %p '%s', %p(%d), %p\n", this, m_szName, m_pChildren, m_iNumChildren, m_pParent);

   m_bDelete = true;

   // STACKTRACEUPDATE

   // printf("  children\n");
   for(iChildNum = 0; iChildNum < m_iNumChildren; iChildNum++)
   {
      delete m_pChildren[iChildNum];
   }

   // STACKTRACEUPDATE
   // printf("  child array\n");
   delete[] m_pChildren;

   // STACKTRACEUPDATE

   // printf("EDFElement::~EDFElement %p(%s) %d %p(%s)\n", m_szName, m_szName, m_vType, m_vType == BYTES ? m_pValue : NULL, m_vType == BYTES ? m_pValue : NULL);

   // printf("  name\n");
   delete[] m_szName;
   // STACKTRACEUPDATE
   if(m_vType == BYTES)
   {
      // printf("  byte value\n");
      // STACKTRACEUPDATE
      delete m_pValue;
   }

   // STACKTRACEUPDATE

   // printf("  parent check\n", m_pParent);
   if(m_pParent != NULL)
   {
      // printf("  parent remove %p\n", this);
      m_pParent->remove(this);
   }

   // printf("  exit\n");
}


char *EDFElement::getName(bool bCopy)
{
   if(bCopy == true)
   {
      return strmk(m_szName);
   }

   return m_szName;
}

int EDFElement::getType()
{
   return m_vType;
}

char *EDFElement::getValueStr(bool bCopy, bool bLiterals, int iOptions)
{
   // STACKTRACE
   char *szReturn = NULL;
   bytes *pValue = NULL;

   if(m_vType == NONE)
   {
      return NULL;
   }
   else if(m_vType != BYTES)
   {
      debug("Element %s is not of type BYTES\n", m_szName);
      throw new EDFElementTypeException("Element is not of type BYTES");
   }

   // STACKTRACEUPDATE

   if(bCopy == false)
   {
      // STACKTRACEUPDATE
      // szReturn = (char *)m_pValue;
      szReturn = (char *)m_pValue->Data(false);
   }
   else
   {
      // STACKTRACEUPDATE
      if(bLiterals == false)
      {
         // printf("EDFElement::getValueStr copy %p(%ld) to %p\n", m_pValue, m_lValueLen, szReturn);
         // NEWCOPY(szReturn, m_pValue, m_lValueLen, char, byte);
         szReturn = (char *)m_pValue->Data(true);
      }
      else
      {
         // getValueLiterals((byte **)&szReturn);
         pValue = addLiterals(iOptions);
         szReturn = (char *)pValue->Data(true);
         delete pValue;
      }
   }
   // szReturn = (char *)m_pValue->Data(bCopy, bLiterals);

   // STACKTRACEUPDATE

   return szReturn;
}

bytes *EDFElement::getValueBytes(const bool bCopy, const bool bLiterals, int iOptions)
{
   // long lReturn = m_lValueLen;
   bytes *pReturn = NULL;

   if(m_vType == NONE)
   {
      /* if(pValue != NULL)
      {
         *pValue = NULL;
      } */
      return NULL;
   }
   else if(m_vType != BYTES)
   {
      debug("Element %s is not of type BYTES\n", m_szName);
      throw new EDFElementTypeException("Element is not of type BYTES");
   }

   // if(pValue != NULL)
   {
      if(bCopy == false)
      {
         // (*pValue) = m_pValue;
         pReturn = m_pValue;
      }
      else
      {
         if(bLiterals == false)
         {
            // NEWCOPY((*pValue), m_pValue, m_lValueLen, byte, byte);
            pReturn = new bytes(m_pValue);
         }
         else
         {
            // lReturn = getValueLiterals(pValue);
            pReturn = addLiterals(iOptions);
         }
         // pReturn = new bytes(m_pValue, bLiterals);
      }
   }

   return pReturn;
}

long EDFElement::getValueInt()
{
   if(m_vType != INT && m_vType != FLOAT)
   {
      debug("Element %s is not of type INT or FLOAT\n", m_szName);
      throw new EDFElementTypeException("Element is not of type INT or FLOAT");
   }

   if(m_vType == FLOAT)
   {
      return (long)m_dValue;
   }

   return m_lValue;
}

double EDFElement::getValueFloat()
{
   if(m_vType != INT && m_vType != FLOAT)
   {
      debug("Element %s is not of type INT or FLOAT\n", m_szName);
      throw new EDFElementTypeException("Element is not of type INT or FLOAT");
   }

   if(m_vType == INT)
   {
      return (double)m_lValue;
   }

   return m_dValue;
}

bool EDFElement::set(const char *szName)
{
   if(validName(szName) == false)
   {
      return false;
   }

   delete[] m_szName;
   m_szName = strmk(szName);

   return true;
}

bool EDFElement::setValue(const char *szValue, bool bLiterals, int iOptions)
{
   // printf("EDFElement::setValue %s(%d) %s\n", szValue, strlen(szValue), BoolStr(bLiterals));

   if(m_vType == BYTES)
   {
      delete m_pValue;
   }

   if(szValue != NULL)
   {
      m_vType = BYTES;
      if(bLiterals == false)
      {
         // m_lValueLen = strlen(szValue);
         // NEWCOPY(m_pValue, szValue, m_lValueLen, byte, char);
         m_pValue = new bytes(szValue);
      }
      else
      {
         // setValueLiterals((byte *)szValue, strlen(szValue));
         removeLiterals((byte *)szValue, strlen(szValue), iOptions);
      }
      // m_pValue = new bytes(szValue, bLiterals);
   }
   else
   {
      m_vType = NONE;
   }

   return true;
}

bool EDFElement::setValue(const byte *pValue, const long lValueLen, bool bLiterals, int iOptions)
{
   // printf("EDFElement::setValue %s(%d) %s\n", szValue, strlen(szValue), BoolStr(bLiterals));

   if(m_vType == BYTES)
   {
      delete m_pValue;
   }

   if(pValue != NULL)
   {
      m_vType = BYTES;
      if(bLiterals == false)
      {
         // m_lValueLen = strlen(szValue);
         // NEWCOPY(m_pValue, szValue, m_lValueLen, byte, char);
         m_pValue = new bytes(pValue, lValueLen);
      }
      else
      {
         // setValueLiterals((byte *)szValue, strlen(szValue));
         removeLiterals(pValue, lValueLen, iOptions);
      }
      // m_pValue = new bytes(pValue, lValueLen, bLiterals);
   }
   else
   {
      m_vType = NONE;
   }

   return true;
}

bool EDFElement::setValue(bytes *pValue, const bool bLiterals, int iOptions)
{
   bytes *pTemp = (bytes *)pValue;

   if(m_vType == BYTES)
   {
      delete m_pValue;
   }

   if(pTemp != NULL)
   {
      m_vType = BYTES;
      if(bLiterals == false)
      {
         // m_lValueLen = lValueLen;
         // NEWCOPY(m_pValue, pValue, m_lValueLen, byte, byte);
         m_pValue = new bytes(pTemp);
      }
      else
      {
         // setValueLiterals(pValue, lValueLen);
         removeLiterals(pTemp->Data(false), pTemp->Length(), iOptions);
      }
      // m_pValue = new bytes(pValue, bLiterals);
   }
   else
   {
      m_vType = NONE;
   }

   return true;
}

bool EDFElement::setValue(const long lValue)
{
   if(m_vType == BYTES)
   {
      delete m_pValue;
   }

   m_vType = INT;
   m_lValue = lValue;

   return true;
}

bool EDFElement::setValue(const double dValue)
{
   if(m_vType == BYTES)
   {
      delete m_pValue;
   }

   m_vType = FLOAT;
   m_dValue = dValue;

   return true;
}


bool EDFElement::add(EDFElement *pElement, int iPosition)
{
   return add(pElement, true, iPosition, true);
}

bool EDFElement::remove(const int iPosition)
{
   // STACKTRACE
   // printf("EDFElement::remove %d, %s %d (%s)\n", iPosition, m_szName, m_iNumChildren, BoolStr(m_bDelete));
   // EDFElement **pChildren = NULL;

   if(iPosition < 0 || iPosition >= m_iNumChildren)
   {
      return false;
   }

   m_pChildren[iPosition]->parent(NULL);

   /* printf("EDFElement::remove %d children,", m_iNumChildren);
   for(iChildNum = 0; iChildNum < m_iNumChildren; iChildNum++)
   {
      debug(" %p", m_pChildren[iChildNum]);
   }
   debug("\n"); */
   // printf("EDFElement:remove element %d of %d from %p\n", iPosition, m_iNumChildren, m_pChildren);
   ARRAY_DEC(EDFElement *, m_pChildren, m_iNumChildren, iPosition)
   /* if(iPosition < m_iNumChildren - 1)
   {
      memmove((void *)&m_pChildren[iPosition], (void *)&m_pChildren[iPosition + 1], (m_iNumChildren - iPosition - 1) * sizeof(EDFElement *));
   }
   m_iNumChildren--; */
   /* printf("EDFElement::remove %d children,", m_iNumChildren);
   for(iChildNum = 0; iChildNum < m_iNumChildren; iChildNum++)
   {
      debug(" %p", m_pChildren[iChildNum]);
   }
   debug("\n"); */

   return true;
}

bool EDFElement::moveFrom(EDFElement *pElement, int iPosition)
{
   EDFElement *pParent = NULL;

   /* if(iPosition == ABSLAST || iPosition == LAST)
   {
      // iPosition = m_iNumChildren;
   }
   else if(iPosition == ABSFIRST || iPosition == FIRST)
   {
      // iPosition = 0;
   } */

   if(pElement == NULL ||
      (iPosition < 0 && iPosition > m_iNumChildren && iPosition != ABSLAST && iPosition != LAST && iPosition != ABSFIRST && iPosition != FIRST))
   {
      debug("EDFElement::move cannot do move (%p is NULL or %d is invalid)\n", pElement, iPosition);
      return false;
   }

   debug("EDFElement::move %p %d (of %d), %p\n", pElement, iPosition, m_iNumChildren, this);
   // childList();

   pParent = pElement->parent();
   if(pParent != NULL)
   {
      debug("EDFElement::move remove from parent\n");
      pParent->remove(pElement);
   }

   // childList();
   add(pElement, false, iPosition, true);
   // childList();

   return true;
}

bool EDFElement::moveTo(EDFElement *pElement, int iPosition)
{
   return pElement->moveFrom(this, iPosition);
}

int EDFElement::children(const char *szName, const bool bRecurse)
{
   int iNumChildren = 0, iChildNum = 0;

   if(szName == NULL)
   {
      iNumChildren = m_iNumChildren;
   }

   for(iChildNum = 0; iChildNum < m_iNumChildren; iChildNum++)
   {
      if(szName != NULL && stricmp(m_pChildren[iChildNum]->getName(false), szName) == 0)
      {
         iNumChildren++;
      }
      if(bRecurse == true)
      {
         iNumChildren += m_pChildren[iChildNum]->children(szName, true);
      }
   }

   return iNumChildren;
}


EDFElement *EDFElement::child(const int iChildNum)
{
   if(iChildNum < 0 || iChildNum >= m_iNumChildren)
   {
      return NULL;
   }

   return m_pChildren[iChildNum];
}

EDFElement *EDFElement::child(const char *szName, const char *szValue, int iPosition)
{
   bytes *pValue = NULL;
   EDFElement *pReturn = NULL;

   if(szValue != NULL)
   {
      pValue = new bytes(szValue);
   }

   pReturn = child(szName, pValue, iPosition);

   delete pValue;

   return pReturn;
}

EDFElement *EDFElement::child(const char *szName, bytes *pValue, int iPosition)
{
   int iChildNum = 0;

   // printf("EDFElement::child '%s' '%s' %d, %d\n", szName, pValue != NULL ? pValue->Data(false) : NULL, iPosition, m_iNumChildren);

   if(m_iNumChildren == 0)
   {
      return NULL;
   }

   iChildNum = find(szName, pValue, iPosition);
   // printf("EDFElement::child find %d\n", iChildNum);
   if(iChildNum == -1)
   {
      // printf("EDFElement::child exit NULL\n");
      return NULL;
   }

   // printf("EDFElement::child exit %p\n", m_pChildren[iChildNum]);
   return m_pChildren[iChildNum];
}

int EDFElement::child(EDFElement *pChild)
{
   int iChildNum = 0;

   while(iChildNum < m_iNumChildren && m_pChildren[iChildNum] != pChild)
   {
      iChildNum++;
   }

   if(iChildNum == m_iNumChildren)
   {
      return -1;
   }

   return iChildNum;
}

EDFElement *EDFElement::parent()
{
   return m_pParent;
}

SortElement *g_pSort = NULL;

#define SORT_CHILD(x, y, z) \
pElement = x; \
x = x->child(y); \
if(x == NULL) \
{ \
   /*debug("sortlist exit %d, no child '%s' of %p\n", z, y, pElement);*/ \
   return z; \
}

#define IS_NUM(x) \
(x == EDFElement::INT || x == EDFElement::FLOAT ? true : false)

int sortlist(const void *p1, const void *p2)
{
   // STACKTRACE
   int iChildNum = -1, iComp = 0, iReturn = 0;
   bool bNext = true;
   EDFElement *pElement1 = (EDFElement *)*(void **)p1, *pElement2 = (EDFElement *)*(void **)p2, *pElement = NULL;
   // EDFElement *pChild1 = pElement1, *pChild2 = pElement2;
   SortElement *pSort = g_pSort , *pParent = NULL;

   // debug("sortlist entry\n");

   while(pSort != NULL)
   {
      /* debug("sortlist sort %d %s, %p %s %d, %p %s %d\n", pSort->m_iType, pSort->m_szName,
         pElement1, pElement1->getName(false), pElement1->getType(),
         pElement2, pElement2->getName(false), pElement2->getType()); */

      if(mask(pSort->m_iType, SORT_ITEM) == false && pSort->m_szName != NULL)
      {
         SORT_CHILD(pElement1, pSort->m_szName, 1);
         SORT_CHILD(pElement2, pSort->m_szName, -1);
      }

      if(mask(pSort->m_iType, SORT_KEY) == true)
      {
         iReturn = 0;
         if(IS_NUM(pElement1->getType()) != IS_NUM(pElement2->getType()))
         {
            debug("sortlist different value types %d %s %g / %d %s %g\n",
               pElement1->getType(), pElement1->getType() == EDFElement::BYTES ? pElement1->getValueBytes(false)->Data(false) : NULL,  IS_NUM(pElement1->getType()) ? pElement1->getValueFloat() : 0,
               pElement2->getType(), pElement2->getType() == EDFElement::BYTES ? pElement2->getValueBytes(false)->Data(false) : NULL,  IS_NUM(pElement2->getType()) ? pElement2->getValueFloat() : 0);

            if(pElement1->getType() == EDFElement::BYTES)
            {
               iReturn = -1;
            }
            else
            {
               iReturn = 1;
            }
         }
         else if(pElement1->getType() == EDFElement::BYTES)
         {
            // debug("sortlist values %s / %s\n", pElement1->getValueStr(false), pElement2->getValueStr(false));

            if(pElement1->getValueBytes(false) == NULL && pElement2->getValueBytes(false) == NULL)
            {
               // iReturn = 0;
            }
            else if(pElement1->getValueBytes(false) == NULL)
            {
               // debug("sortlist no value for element 1\n");
               iReturn = 1;
            }
            else if(pElement2->getValueBytes(false) == NULL)
            {
               // debug("sortlist no value for element 2\n");
               iReturn = -1;
            }
            else
            {
               // iComp = stricmp(pElement1->getValueStr(false), pElement2->getValueStr(false));
               iComp = pElement1->getValueBytes(false)->Compare(pElement2->getValueBytes(false), true);
               if(iComp < 0)
               {
                  iReturn = -1;
                  // debug("sortlist element 1 < element 2\n");
               }
               else if(iComp > 0)
               {
                  iReturn = 1;
                  // debug("sortlist element 1 > element 2\n", iReturn);
               }
               else
               {
                  // debug("sortlist element 1 == element 2\n");
               }
            }

            // debug("sortlist return %d\n", iReturn);
         }
         else
         {
            // debug("sortlist values %g / %g\n", pElement1->getValueFloat(), pElement2->getValueFloat());

            if(pElement1->getValueFloat() < pElement2->getValueFloat())
            {
               iReturn = -1;
               // debug("sortlist element 1 < element 2\n");
            }
            else if(pElement1->getValueFloat() > pElement2->getValueFloat())
            {
               iReturn = 1;
               // debug("sortlist element 1 > element 2\n");
            }
         }

         if(iReturn != 0)
         {
            if(mask(pSort->m_iType, SORT_ASCENDING) == false)
            {
               iReturn = -iReturn;
            }

            // debug("sortlist exit %d\n", iReturn);
            return iReturn;
         }
      }

      if(pSort->m_iNumChildren == 0)
      {
         bNext = false;

         do
         {
            // debug("sortlist move to parent\n");
            pParent = pSort->m_pParent;
            if(pParent == NULL)
            {
               // debug("sortlist exit 0, NULL parent\n");
               return 0;
            }

            pElement1 = pElement1->parent();
            pElement2 = pElement2->parent();

            iChildNum = 0;
            while(pParent->m_pChildren[iChildNum] != pSort)
            {
               iChildNum++;
            }

            iChildNum++;
            if(iChildNum < pParent->m_iNumChildren)
            {
               // debug("sortlist move to child %d\n", iChildNum);
               pSort = pParent->m_pChildren[iChildNum];
               bNext = true;
            }
            else
            {
               // debug("sortlist reset parent\n");
               pSort = pParent;
            }
         }
         while(bNext == false);
      }
      else
      {
         // debug("sortlist move to child 0\n");
         pSort = pSort->m_pChildren[0];
         iChildNum = 0;
      }
   }

   // debug("sortlist exit 0, end of sort\n");
   return 0;
}

bool EDFElement::sort(SortElement *pSort)
{
   // STACKTRACE
   int iChildNum = 0;

   // debug("EDFElement::sort %p, %p\n", pSort, this);
   // print("EDFElement::sort");

   if(pSort == NULL)
   {
      return false;
   }

   g_pSort = pSort;
   qsort(m_pChildren, m_iNumChildren, sizeof(EDFElement *), sortlist);

   if(mask(g_pSort->m_iType, SORT_RECURSE) == true && (g_pSort->m_szName == NULL || children(g_pSort->m_szName) > 0))
   {
      for(iChildNum = 0; iChildNum < m_iNumChildren; iChildNum++)
      {
         m_pChildren[iChildNum]->sort(g_pSort);
      }
   }

   return true;
}

bool EDFElement::copy(EDFElement *pElement, const bool bRecurse)
{
   int iChildNum = 0;
   // long lValueLen = 0;
   // byte *pValue = NULL;
   EDFElement *pNew = NULL, *pParent = this;

   while(pParent != NULL && pParent != pElement)
   {
      pParent = pParent->parent();
   }

   if(pParent == pElement)
   {
      return false;
   }

   if(pElement->getType() == BYTES)
   {
      // lValueLen = pElement->getValueByte(&pValue, false);
      // pNew = new EDFElement(this, pElement->getName(false), pValue, lValueLen);
      pNew = new EDFElement(this, pElement->getName(false), pElement->getValueBytes(false));
   }
   else if(pElement->getType() == INT)
   {
      pNew = new EDFElement(this, pElement->getName(false), pElement->getValueInt());
   }
   else if(pElement->getType() == FLOAT)
   {
      pNew = new EDFElement(this, pElement->getName(false), pElement->getValueFloat());
   }
   else
   {
      pNew = new EDFElement(this, pElement->getName(false));
   }
   if(bRecurse == true)
   {
      for(iChildNum = 0; iChildNum < pElement->children(); iChildNum++)
      {
         pNew->copy(pElement->child(iChildNum));
      }
   }

   return true;
}

long EDFElement::storage(const int iOptions)
{
   return storage(iOptions, 0);
}

bytes *EDFElement::write(const int iOptions)
{
   // STACKTRACE
   long lStorageLen = 0, lWriteLen = 0;
   byte *pWrite = NULL;
   bytes *pReturn = NULL;

   /* if(pWrite == NULL)
   {
      return 0;
   } */

   // printf("EDFElement::write %p, %p '%s' %d\n", this, m_pParent, m_szName, m_iNumChildren);

   /* if(m_pParent == NULL && strcmp(m_szName, "")== 0 && m_iNumChildren == 0)
   {
      NEWCOPY((*pWrite), "<></>", 5, byte, char);
      return 5;
   } */

   /* if(bCurr == false)
   {
      debug("EDFElement::write no curr\n");
   } */

   // debugprint(0);

   lStorageLen += storage(iOptions, 0);
   pWrite = new byte[lStorageLen + 1];
   lWriteLen = write(pWrite, lWriteLen, iOptions, 0);
   pWrite[lWriteLen] = '\0';
   while(lWriteLen > 0 && (pWrite[lWriteLen - 1] == '\r' || pWrite[lWriteLen - 1] == '\n'))
   {
      pWrite[--lWriteLen] = '\0';
   }
   pWrite[lWriteLen] = '\0';

   /* if(lWriteLen > 200000)
   {
      debug("EDFElement::write written to %ld of %ld bytes\n", lWriteLen, lStorageLen);
   } */

   // printf("EDFElement::write %p %ld\n", pWrite, lWriteLen);
   pReturn = new bytes(pWrite, lWriteLen);

   delete[] pWrite;

   return pReturn;
}

void EDFElement::print(const char *szTitle, int iOptions)
{
   // long lWriteLen = 0;
   // byte *pWrite = NULL;
   bytes *pWrite = NULL;

   if(iOptions == -1)
   {
      iOptions = PR_SPACE;
   }

   // lWriteLen = write(&pWrite, PR_SPACE);
   pWrite = write(iOptions | EL_CURR);
   if(szTitle != NULL)
   {
      debug("%s:\n", szTitle);
   }
   bytesprint(NULL, pWrite);
   debug("\n");

   delete pWrite;
}


bool EDFElement::validName(const char *szName)
{
   if(szName == NULL)
   {
      return false;
   }

   return true;
}


bool EDFElement::add(EDFElement *pElement, bool bName, int iPosition, const bool bRootCheck)
{
   int iChildNum = 0;
   EDFElement *pRoot1 = this, *pRoot2 = pElement;
   EDFElement **pChildren = NULL;

   if(pElement == NULL)
   {
      debug("EDFElement::add failed, NULL\n");
      return false;
   }

   // printf("EDFElement::add %p %d %s, %p\n", pElement, iPosition, BoolStr(bRootCheck), this);

   if(iPosition == ABSLAST)
   {
      iChildNum = m_iNumChildren;
   }
   else if(iPosition == ABSFIRST)
   {
      iChildNum = 0;
   }
   else if(bName == true)
   {
      iChildNum = find(pElement->getName(false), NULL, iPosition);
      if(iChildNum == -1 && (iPosition == FIRST || iPosition == LAST))
      {
         // printf("EDFElement::add fail reset %d\n", m_iNumChildren);
         iChildNum = m_iNumChildren;
      }
   }
   else if(iChildNum >= 0 && iChildNum <= m_iNumChildren)
   {
      iChildNum = iPosition;
   }

   if(iChildNum == -1)
   {
      debug("EDFElement::add '%s' %d(%d) failed\n", pElement->getName(false), iPosition, m_iNumChildren);
      STACKPRINT
      return false;
   }

   if(iPosition == LAST && iChildNum < m_iNumChildren)
   {
      iChildNum++;
      // printf("EDFElement::add LAST reset %d\n", iChildNum);
   }
   else if(iPosition == FIRST && iChildNum < m_iNumChildren && iChildNum > 0)
   {
      iChildNum--;
      // printf("EDFElement::add FIRST reset %d\n", iChildNum);
   }

   // printf("EDFElement::add %p %d\n", pElement, iPosition);

   if(bRootCheck == true)
   {
      while(pRoot1->m_pParent != NULL)
      {
         pRoot1 = pRoot1->m_pParent;
      }
      // printf("EDFElement::add root(1) of %p: %p\n", this, pRoot1);

      while(pRoot2->m_pParent != NULL)
      {
         pRoot2 = pRoot2->m_pParent;
      }
      // printf("EDFElement::add root(2) of %p: %p\n", pElement, pRoot2);

      if(pRoot1->find(pRoot2) == true || pRoot2->find(pRoot1) == true)
      {
         debug("EDFElement::add roots already linked\n");
         return false;
      }

      if(pRoot1 == pRoot2)
      {
         debug("EDFElement::add roots the same\n");
         return false;
      }
   }
   /* else
   {
      debug("EDFElement::add internal %p\n", pElement);
   } */

   // iPosition = m_iNumChildren;
   // printf("EDFElement::add ");

   // printf("EDFElement::add %p %d\n", pElement, iChildNum);
   ARRAY_INC(EDFElement *, m_pChildren, m_iNumChildren, m_iMaxChildren, CHILD_INC, pElement, iChildNum, pChildren)

   pElement->parent(this);

   // childList();
   // print("EDFElement::add");

   return true;
}

bool EDFElement::find(EDFElement *pElement)
{
   bool bFound = false;
   int iChildNum = 0;

   // printf("EDFElement::find %p %p\n", this, pElement);

   if(this == pElement)
   {
      return true;
   }

   while(bFound == false && iChildNum < m_iNumChildren)
   {
      bFound = m_pChildren[iChildNum]->find(pElement);
      iChildNum++;
   }

   return bFound;
}

bool EDFElement::remove(EDFElement *pElement)
{
   // STACKTRACE
   int iChildNum = 0;

   // printf("EDFElement::remove %p\n", pElement);

   if(m_bDelete == true)
   {
      // printf("EDFElement::remove disabled during destructor\n");
      return false;
   }

   // childList();

   while(iChildNum < m_iNumChildren)
   {
      // printf("EDFElement::remove check %d, %p\n", iChildNum, m_pChildren[iChildNum]);
      if(m_pChildren[iChildNum] == pElement)
      {
         m_pChildren[iChildNum]->parent(NULL);

         if(iChildNum < m_iNumChildren - 1)
         {
            memmove((void *)&m_pChildren[iChildNum], (void *)&m_pChildren[iChildNum + 1], (m_iNumChildren - iChildNum - 1) * sizeof(EDFElement *));
         }
         m_iNumChildren--;
         iChildNum = m_iNumChildren;
      }
      else
      {
         iChildNum++;
      }
   }

   // childList();

   return false;
}

void EDFElement::parent(EDFElement *pElement)
{
   m_pParent = pElement;
}

void EDFElement::debugprint(int iDepth)
{
   int iPos = 0, iValueLen = 0, iChildNum = 0;
   byte *pValue = NULL;

   for(iPos = 0; iPos < iDepth; iPos++)
   {
      debug(" ");
   }

   debug("%s/%d", m_szName, m_vType);
   if(m_vType == BYTES)
   {
      debug("(%p/", m_pValue);
      if(m_pValue != NULL)
      {
         pValue = m_pValue->Data(false);
         iValueLen = m_pValue->Length();
         if(iValueLen > 20)
         {
            iValueLen = 20;
         }
         debug("'");
         // printf(" %s", m_pValue->Data(false));
         iPos = 0;
         while(iPos < iValueLen)
         {
            if(isprint(pValue[iPos]))
            {
               debug("%c", pValue[iPos]);
            }
            else
            {
               debug("[%d]", pValue[iPos]);
            }
            iPos++;
         }
         debug("'");
      }
      debug(")");
   }
   else if(m_vType == INT)
   {
      debug("(%ld)", m_lValue);
   }
   else if(m_vType == FLOAT)
   {
      debug("(%g)", m_dValue);
   }
   debug("\n");

   for(iChildNum = 0; iChildNum < m_iNumChildren; iChildNum++)
   {
      m_pChildren[iChildNum]->debugprint(iDepth + 1);
   }
}

void EDFElement::init(EDFElement *pParent, const char *szName, int iPosition)
{
   // printf("EDFElement::init %p %s %d\n", pParent, szName, iPosition);

   // printf("  EDFElement::init %p %s\n", this, szName);

   m_bDelete = false;

   if(validName(szName) == false)
   {
      m_szName = strmk("");
   }
   else
   {
      m_szName = strmk(szName);
   }

   m_vType = NONE;

   m_pParent = pParent;
   if(m_pParent != NULL)
   {
      // printf("EDFElement::init add %p to %p\n", this, m_pParent);
      m_pParent->add(this, true, iPosition, false);
   }

   m_iNumChildren = 0;
   m_iMaxChildren = 0;
   m_pChildren = NULL;
}

#define findcomp(x, y, z) \
(stricmp(x->getName(false), y) == 0 && (z == NULL || (z != NULL && x->getType() == EDFElement::BYTES && z->Compare(x->getValueBytes(false), false) == 0)))

int EDFElement::find(const char *szName, bytes *pValue, int iPosition)
{
   int iChildNum = 0, iNumMatches = 0;

   // childList();
   // printf("EDFElement::find '%s' '%s' %d, %d", szName, szValue, iPosition, m_iNumChildren);

   if(iPosition == ABSLAST || (iPosition == LAST && szName == NULL))
   {
      // printf(", %d\n", m_iNumChildren);
      return m_iNumChildren - 1;
   }
   else if(iPosition == LAST)
   {
      iChildNum = m_iNumChildren - 1;
      while(iChildNum >= 0 && findcomp(m_pChildren[iChildNum], szName, pValue) == false)
      {
         iChildNum--;
      }
   }
   else if(iPosition == ABSFIRST || (iPosition == FIRST && szName == NULL))
   {
      // printf(", 0\n");
      return 0;
   }
   else if(iPosition == FIRST)
   {
      while(iChildNum < m_iNumChildren && findcomp(m_pChildren[iChildNum], szName, pValue) == false)
      {
         iChildNum++;
      }
   }
   else if(iPosition >= 0 && iPosition < m_iNumChildren && szName == NULL)
   {
   }
   else if(iPosition >= 0 && iPosition < m_iNumChildren)
   {
      while(iChildNum < m_iNumChildren && iNumMatches <= iPosition)
      {
         if(findcomp(m_pChildren[iChildNum], szName, pValue) == true)
         {
            if(iNumMatches == iPosition)
            {
               // printf(", %d\n", iChildNum);
               return iChildNum;
            }
            iNumMatches++;
         }
         iChildNum++;
      }
   }

   if(iChildNum < 0 || iChildNum >= m_iNumChildren)
   {
      // printf(", -1\n");
      return -1;
   }

   // printf(", %d\n", iChildNum);
   return iChildNum;
}

bytes *EDFElement::addLiterals(int iOptions)
{
   // STACKTRACE
   int iSourcePos = 0, iSourceLen = 0, iDestExtra = 0;
   byte *pSource = NULL, *pDest = NULL;
   bytes *pReturn = NULL;

   if(m_pValue == NULL || m_pValue->Data(false) == NULL)
   {
      return NULL;
   }

   pSource = m_pValue->Data(false);
   iSourceLen = m_pValue->Length();

   for(iSourcePos = 0; iSourcePos < iSourceLen; iSourcePos++)
   {
      if(pSource[iSourcePos] == '\\' || pSource[iSourcePos] == '"')
      {
         iDestExtra++;
      }
   }

   if(iDestExtra > 0)
   {
      // bytesprint("EDFElement::addLiterals member", m_pValue);

      pDest = new byte[iSourceLen + iDestExtra];

      iDestExtra = 0;
      for(iSourcePos = 0; iSourcePos < iSourceLen; iSourcePos++)
      {
         if(pSource[iSourcePos] == '\\' || pSource[iSourcePos] == '"')
         {
            pDest[iSourcePos + iDestExtra] = '\\';
            iDestExtra++;
         }
         pDest[iSourcePos + iDestExtra] = pSource[iSourcePos];
      }

      pReturn = new bytes(pDest, iSourceLen + iDestExtra);
      // bytesprint("EDFElement::addLiterals return", pReturn);

      delete[] pDest;
   }
   else
   {
      pReturn = new bytes(m_pValue->Data(false), m_pValue->Length());
      // bytesprint("EDFElement::addLiterals return", pReturn);
   }

   return pReturn;
}

void EDFElement::removeLiterals(const byte *pValue, long lValueLen, int iOptions)
{
   // STACKTRACE
   int iValuePos = 0, iDestExtra = 0;
   byte *pDest = NULL;

   for(iValuePos = 0; iValuePos < lValueLen - 1; iValuePos++)
   {
      if(pValue[iValuePos] == '\\' || pValue[iValuePos] == '"')
      {
         iDestExtra++;
         iValuePos++;
      }
   }

   if(iDestExtra > 0)
   {
      // memprint("EDFElement::removeLiterals value", pValue, lValueLen);

      pDest = new byte[lValueLen - iDestExtra];
      // printf("EDFElement::removeLiterals byte array %p(%ld), %d extras\n", pDest, lValueLen - iDestExtra, iDestExtra);

      iDestExtra = 0;
      for(iValuePos = 0; iValuePos < lValueLen; iValuePos++)
      {
         if(iValuePos < lValueLen - 1 && (pValue[iValuePos] == '\\' || pValue[iValuePos] == '"'))
         {
            // pDest[iSourcePos + iDestExtra] = '\\';
            iDestExtra++;
            iValuePos++;
         }
         pDest[iValuePos - iDestExtra] = pValue[iValuePos];
      }

      m_pValue = new bytes(pDest, lValueLen - iDestExtra);
      // bytesprint("EDFElement::removeLiterals member", m_pValue);

      delete[] pDest;
   }
   else
   {
      m_pValue = new bytes(pValue, lValueLen);
      // bytesprint("EDFElement::removeLiterals member", m_pValue);
   }
}

/* void EDFElement::setName(const char *szName)
{
   delete[] m_szName;

   m_szName = (char *)szName;
} */

long EDFElement::storage(const int iOptions, int iDepth)
{
   int iChildNum = 0, iNameLen = strlen(m_szName);
   long lReturn = 0;

   // printf("EDFElement::storage entry %p %d %d, %s %d\n", this, iPretty, iDepth, m_szName, m_vType);

   if(mask(iOptions, PR_SPACE) == true)
   {
      lReturn += iDepth; // indent
   }
   lReturn += 13; // <,=,",",/,>\r,\n -- \r,\n,<,/,>
   lReturn += iNameLen; // name
   if(m_vType == BYTES && m_pValue != NULL)
   {
      if(mask(iOptions, PR_BIN) == true)
      {
         lReturn += 5 * m_pValue->Length(); // value in [nnn] format
      }
      else
      {
         lReturn += 2 * m_pValue->Length(); // escaped value
      }
      // printf("EDFElement::storage value %ld\n", m_lValueLen);
   }
   else if(m_vType == INT || m_vType == FLOAT)
   {
      if(m_vType == INT)
      {
         lReturn += 10; // value
      }
      else
      {
         lReturn += 20; // value
      }
   }
   if(m_iNumChildren > 0)
   {
      if(mask(iOptions, PR_SPACE) == true)
      {
         iDepth += 2;
      }
      for(iChildNum = 0; iChildNum < m_iNumChildren; iChildNum++)
      {
         lReturn += m_pChildren[iChildNum]->storage(iOptions, iDepth);
      }
      if(mask(iOptions, PR_SPACE) == true)
      {
         iDepth -= 2;

         lReturn += iDepth;
         lReturn += iNameLen; // name
      }
   }

   // printf("EDFElement::storage exit %ld, %p\n", lReturn, this);
   return lReturn;
}

long EDFElement::write(byte *pWrite, long lOffset, const int iOptions, int iDepth)
{
   int iChildNum = 0, iNameLen = strlen(m_szName);
   int iSourcePos = 0, iSourceLen = 0;
   // long lValueLen = 0;
   char szNumber[20];
   byte *pSource = NULL;
   bytes *pValue = NULL;

   /* printf("EDFElement::write %ld, '%s' %d", lOffset, m_szName, m_vType);
   if(m_vType == BYTES)
   {
      debug(" %p", m_pValue);
      if(m_pValue != NULL)
      {
         debug("/%ld", m_pValue->Length());
      }
   }
   debug("\n"); */

   if(mask(iOptions, EL_CURR) == true)
   {
      if(mask(iOptions, PR_SPACE) == true)
      {
         memset(pWrite + lOffset, ' ', iDepth);
         lOffset += iDepth;
      }
      pWrite[lOffset++] = '<';
      memcpy(pWrite + lOffset, m_szName, iNameLen);
      lOffset += iNameLen;
      if(m_vType == BYTES && m_pValue != NULL)
      {
         // bytesprint("EDFElement::write byte value", m_pValue);

         // lValueLen = getValueByte(&pValue, true, true);
         pValue = getValueBytes(true, true, iOptions);
         /* if(lValueLen != m_lValueLen && m_lValueLen < 70)
         {
            debug("EDFElement::write literalised %ld -vs- %ld bytes\n", lValueLen, m_lValueLen);
            memprint("EDFElement::write unliteralised value", m_pValue, m_lValueLen);
            memprint("EDFElement::write literalised value", pValue, lValueLen);
         } */

         if(pValue != NULL)
         {
            pWrite[lOffset++] = '=';
            pWrite[lOffset++] = '"';

            if(mask(iOptions, PR_BIN) == true)
            {
               pSource = pValue->Data(false);
               iSourceLen = pValue->Length();
               for(iSourcePos = 0; iSourcePos < iSourceLen; iSourcePos++)
               {
                  if(!iscntrl(pSource[iSourcePos]))
                  {
                     pWrite[lOffset++] = pSource[iSourcePos];
                  }
                  else
                  {
                     pWrite[lOffset++] = '[';
                     lOffset += sprintf((char *)(pWrite + lOffset), "%d", pSource[iSourcePos]);
                     pWrite[lOffset++] = ']';
                  }
               }
            }
            else
            {
               memcpy(pWrite + lOffset, pValue->Data(false), pValue->Length());
               lOffset += pValue->Length();
            }
            delete pValue;

            pWrite[lOffset++] = '"';
         }
      }
      else if(m_vType == INT || m_vType == FLOAT)
      {
         // printf("EDFElement::write int value %ld\n", m_lValue);

         pWrite[lOffset++] = '=';
         if(m_vType == INT)
         {
            sprintf(szNumber, "%ld", m_lValue);
         }
         else
         {
            sprintf(szNumber, "%g", m_dValue);
         }
         memcpy(pWrite + lOffset, szNumber, strlen(szNumber));
         lOffset += strlen(szNumber);
      }
   }
   /* else
   {
      debug("EDFElement::write no curr %s(%d)\n", m_szName, m_vType);
   } */

   if(m_iNumChildren > 0)
   {
      if(mask(iOptions, EL_CURR) == true)
      {
         pWrite[lOffset++] = '>';
         if(mask(iOptions, PR_SPACE) == true)
         {
            if(mask(iOptions, PR_CRLF) == true)
            {
               pWrite[lOffset++] = '\r';
            }
            pWrite[lOffset++] = '\n';

            iDepth += 2;
         }
      }
      for(iChildNum = 0; iChildNum < m_iNumChildren; iChildNum++)
      {
         lOffset = m_pChildren[iChildNum]->write(pWrite, lOffset, EL_CURR | iOptions, iDepth);
         if(mask(iOptions, PR_SPACE) == true)
         {
            if(mask(iOptions, PR_CRLF) == true)
            {
               pWrite[lOffset++] = '\r';
            }
            pWrite[lOffset++] = '\n';
         }
      }
      if(mask(iOptions, PR_SPACE) == true)
      {
         iDepth -= 2;

         if(mask(iOptions, EL_CURR) == true)
         {
            memset(pWrite + lOffset, ' ', iDepth);
            lOffset += iDepth;
         }
      }
      if(mask(iOptions, EL_CURR) == true)
      {
         pWrite[lOffset++] = '<';
         pWrite[lOffset++] = '/';
         if(mask(iOptions, PR_SPACE) == true)
         {
            memcpy(pWrite + lOffset, m_szName, iNameLen);
            lOffset += iNameLen;
         }
         pWrite[lOffset++] = '>';
      }
   }
   else if(mask(iOptions, EL_CURR) == true)
   {
      if(m_szName[0] == '\0')
      {
         pWrite[lOffset++] = '>';
         pWrite[lOffset++] = '<';
      }
      pWrite[lOffset++] = '/';
      pWrite[lOffset++] = '>';
   }

   return lOffset;
}

void EDFElement::childList()
{
   int iChildNum = 0;

   debug("EDFElement::childList %p", m_pChildren);
   if(m_iNumChildren > 0)
   {
      debug(", %d children:\n", m_iNumChildren);
      for(iChildNum = 0; iChildNum < m_iNumChildren; iChildNum++)
      {
         debug("  %p", m_pChildren[iChildNum]);
         debug(" (%s", m_pChildren[iChildNum]->getName(false));
         if(m_pChildren[iChildNum]->getType() == EDFElement::BYTES)
         {
            debug("=\"%s\"", m_pChildren[iChildNum]->getValueStr(false));
         }
         else if(m_pChildren[iChildNum]->getType() == EDFElement::INT)
         {
            debug("=\"%ld\"", m_pChildren[iChildNum]->getValueInt());
         }
         else if(m_pChildren[iChildNum]->getType() == EDFElement::FLOAT)
         {
            debug("=\"%g\"", m_pChildren[iChildNum]->getValueFloat());
         }
         if(m_pChildren[iChildNum]->children() > 0)
         {
            debug("*");
         }
         debug("\n");
      }
   }
   // printf("\n");
}
