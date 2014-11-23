/*
** EDF - Encapsulated Data Format
** (c) 1998 Michael Wood (mike@compsoc.man.ac.uk)
**
** EDFElement.h: Definition of EDFElement class
**
** An EDFElement has a name and optional value (number or string) plus optional EDFElement children:
** <name1="..."/>
** <name2=n/>
** <name3>
**   <name4=n/>
**   <name5="..."/>
** </name3>
*/

#ifndef _EDFELEMENT_H_
#define _EDFELEMENT_H_

#include "useful/useful.h"

#define NEWCOPY(pDest, pSrc, lLen, tDest, tSrc) \
pDest = new tDest[lLen + 1];\
memcpy(pDest, pSrc, lLen);\
pDest[lLen] = '\0';

class EDFElementTypeException
{
public:
   EDFElementTypeException(const char *szMessage);
   ~EDFElementTypeException();

   char *getMessage();

private:
   char *m_szMessage;
};

#define SORT_ITEM 1
#define SORT_SECTION 2
#define SORT_KEY 4
#define SORT_RECURSE 8
#define SORT_ASCENDING 16
#define SORT_IGNORECASE 32

struct SortElement
{
   int m_iType;
   char *m_szName;

   struct SortElement *m_pParent;
   int m_iNumChildren;
   struct SortElement **m_pChildren;
};

class EDFElement
{
public:
   enum { FIRST = 0, LAST = -1, PREV = -2, NEXT = -3, ABSFIRST = -4, ABSLAST = -5 };
   enum { NONE = 0, BYTES = 1, INT = 2, FLOAT = 3 };
   enum { EL_ROOT = 1, EL_CURR = 2, PR_SPACE = 4, PR_CRLF = 8, PR_BIN = 32 };

   EDFElement();
   EDFElement(const char *szName);
   EDFElement(const char *szName, const char *szValue, int iPosition = EDFElement::ABSLAST);
   EDFElement(const char *szName, const bytes *pValue, int iPosition = EDFElement::ABSLAST);
   EDFElement(const char *szName, const long lValue, int iPosition = EDFElement::ABSLAST);
   EDFElement(const char *szName, const double dValue, int iPosition = EDFElement::ABSLAST);
   EDFElement(EDFElement *pParent, const char *szName, int iPosition = EDFElement::ABSLAST);
   EDFElement(EDFElement *pParent, const char *szName, const char *szValue, int iPosition = EDFElement::ABSLAST);
   EDFElement(EDFElement *pParent, const char *szName, const bytes *pValue, int iPosition = EDFElement::ABSLAST);
   EDFElement(EDFElement *pParent, const char *szName, const long lValue, int iPosition = EDFElement::ABSLAST);
   EDFElement(EDFElement *pParent, const char *szName, const double dValue, int iPosition = EDFElement::ABSLAST);
   ~EDFElement();

   char *getName(const bool bCopy);
   int getType();
   char *getValueStr(const bool bCopy, bool bLiterals = false, int iOptions = 0);
   bytes *getValueBytes(const bool bCopy, const bool bLiterals = false, int iOptions = 0);
   long getValueInt();
   double getValueFloat();

   bool set(const char *szName);
   bool setValue(const char *szValue, const bool bLiterals = false, int iOptions = 0);
   bool setValue(const byte *pValue, const long lValueLen, const bool bLiterals = false, int iOptions = 0);
   bool setValue(const bytes *pValue, const bool bLiterals = false, int iOptions = 0);
   bool setValue(const long lValue);
   bool setValue(const double dValue);

   bool add(EDFElement *pElement, int iPosition = ABSLAST);
   bool remove(const int iPosition);
   bool moveFrom(EDFElement *pElement, int iPosition = EDFElement::ABSLAST);
   bool moveTo(EDFElement *pElement, int iPosition = EDFElement::ABSLAST);

   int children(const char *szName = NULL, const bool bRecurse = false);

   EDFElement *child(const int iChildNum);
   EDFElement *child(const char *szName, const char *szValue = NULL, int iPosition = FIRST);
   EDFElement *child(const char *szName, bytes *pValue, int iPosition = FIRST);
   int child(EDFElement *pChild);
   bool first(const char *szName = NULL);
   bool last(const char *szName = NULL);
   bool next(const char *szName = NULL);
   bool prev(const char *szName = NULL);
   EDFElement *parent();

   bool sort(SortElement *pSort);

   bool copy(EDFElement *pElement, const bool bRecurse = true);

   long storage(const int iOptions);
   bytes *write(const int iOptions = EL_ROOT + EL_CURR);
   void print(const char *szTitle = NULL, int iOptions = -1);

   static bool validName(const char *szName);

protected:
   bool add(EDFElement *pElement, bool bName, int iPosition, const bool bRootCheck);

   bool find(EDFElement *pElement);
   bool remove(EDFElement *pElement);
   void parent(EDFElement *pElement);

   void debugprint(int iDepth);

private:
   char *m_szName;
	
   char m_vType;
   union
   {
      long m_lValue;
      double m_dValue;
      bytes *m_pValue;
   };

   bool m_bDelete;

   EDFElement *m_pParent;
   
   int m_iNumChildren;
   int m_iMaxChildren;
   EDFElement **m_pChildren;

   void init(EDFElement *pParent, const char *szName, int iPosition);

   int find(const char *szName, bytes *pValue, int iPosition);

   bytes *addLiterals(int iOptions);
   void removeLiterals(const byte *pValue, const long lValueLen, int iOptions);

   long storage(const int iOptions, int iDepth);
   long write(byte *pWrite, long lOffset, const int iOptions, int iDepth);

   void childList();
};

#endif
