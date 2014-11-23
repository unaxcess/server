/*
** EDFItem library
** (c) 2000 Michael Wood (mike@compsoc.man.ac.uk)
**
** EDFItem.h: Definition of EDFItem class
**
** A hybrid storage class which can provide both fast access member functions
** and EDF storage. Each item is created with an ID which cannot be changed.
** The objects can be read from / written to using EDF or DBTable rows
*/

#ifndef _EDFITEM_H_
#define _EDFITEM_H_

#include "EDF/EDF.h"
// Database access for server builds only
#include "db/DBTable.h"

// Member get / set macros - convenience functions so change tracking is used without having to think about it

// Get string (no length returned)
#define GET_STR(szSource) \
return (bCopy == true ? strmk(szSource) : szSource);

// Get bytes (length returned)
#define GET_BYTES(pSource) \
return (bCopy == true ? new bytes(pSource) : (bytes *)pSource);

// Get string from EDF
#define GET_STR_EDF(szSection, szName) \
EDFElement *pElement = NULL; \
m_pEDF->Root(); \
if((char *)szSection != NULL && m_pEDF->Child(szSection) == false) \
{ \
   return NULL; \
} \
if((char *)szName != NULL && m_pEDF->Child((char *)szName) == false) \
{ \
   return NULL; \
} \
pElement = m_pEDF->GetCurr(); \
if(pElement->getType() != EDFElement::BYTES) \
{ \
   return NULL; \
} \
return pElement->getValueStr(bCopy);

// Get bytes from EDF
#define GET_BYTES_EDF(szSection, szName) \
EDFElement *pElement = NULL; \
m_pEDF->Root(); \
if((char *)szSection != NULL && m_pEDF->Child(szSection) == false) \
{ \
   return NULL; \
} \
if((char *)szName != NULL && m_pEDF->Child((char *)szName) == false) \
{ \
   return NULL; \
} \
pElement = m_pEDF->GetCurr(); \
if(pElement->getType() != EDFElement::BYTES) \
{ \
   return NULL; \
} \
return pElement->getValueBytes(bCopy);

// Get string from table
#define GET_STR_TABLE(iFieldNum, szValue, bNullIfEmpty) \
if(pTable->GetField(iFieldNum, &szValue) == true) \
{ \
   if(szValue != NULL && bNullIfEmpty == true && szValue != NULL && strlen(szValue) == 0) \
   { \
      szValue = NULL; \
   } \
   else \
   { \
      szValue = strmk(szValue); \
   } \
} \
else \
{ \
   szValue = NULL; \
}

// Get bytes from table (length returned)
#define GET_BYTES_TABLE(iFieldNum, pValue, bNullIfEmpty) \
if(pTable->GetField(iFieldNum, &pValue) == true) \
{ \
   if(pValue != NULL && bNullIfEmpty == true && pValue != NULL && pValue->Length() == 0) \
   { \
      pValue = NULL; \
   } \
   else \
   { \
      pValue = new bytes(pValue); \
   } \
} \
else \
{ \
   pValue = NULL; \
}

// Set string
#define SET_STR(szDest, szValue, iMaxLen) \
char *szTemp = NULL; \
delete[] szDest; \
if(iMaxLen != -1 && szValue != NULL && strlen(szValue) > iMaxLen) \
{ \
   szTemp = strmk(szValue); \
   szTemp[iMaxLen] = '\0'; \
} \
else \
{ \
   szTemp = szValue; \
} \
szDest = strmk(szTemp); \
if(szTemp != szValue) \
{ \
   delete[] szTemp; \
} \
SetChanged(true); \
return true;

// Set bytes (length specified)
#define SET_BYTES(pDest, pValue, iMaxLen) \
bytes *pTemp = NULL; \
delete pDest; \
if(iMaxLen != -1 && pValue != NULL && pValue->Length() > iMaxLen) \
{ \
   pTemp = new bytes(pValue, iMaxLen); \
} \
else \
{ \
   pTemp = pValue; \
} \
pDest = new bytes(pTemp); \
if(pTemp != pValue) \
{ \
   delete pTemp; \
} \
SetChanged(true); \
return true;

// Set string from EDF
#define SET_STR_EDF(szDest, szName, bNullDefault, iMaxLen) \
char *szValue = NULL, *szTemp = NULL; \
if(pEDF->GetChild(szName, &szValue) == false) \
{ \
   if(bNullDefault == false) \
   { \
      return false; \
   } \
   delete[] szDest; \
   szDest = NULL; \
   SetChanged(true); \
   return true; \
} \
if(pEDF->DataCopy() == false) \
{ \
   szValue = strmk(szValue); \
} \
if(iMaxLen != -1 && szValue != NULL && strlen(szValue) > iMaxLen) \
{ \
   szTemp = strmk(szValue); \
   szTemp[iMaxLen] = '\0'; \
} \
else \
{ \
   szTemp = szValue; \
} \
delete[] szDest; \
szDest = szTemp; \
if(szTemp != szValue) \
{ \
   delete[] szTemp; \
} \
SetChanged(true); \
return true;

// Set bytes from EDF
#define SET_BYTES_EDF(pDest, szName, bNullDefault, iMaxLen) \
bytes *pValue = NULL, *pTemp = NULL; \
if(pEDF->GetChild(szName, &pValue) == false) \
{ \
   if(bNullDefault == false) \
   { \
      return false; \
   } \
   delete pDest; \
   pDest = NULL; \
   SetChanged(true); \
   return true; \
} \
if(pEDF->DataCopy() == false) \
{ \
   pValue = new bytes(pValue); \
} \
if(iMaxLen != -1 && pValue != NULL && pValue->Length() > iMaxLen) \
{ \
   pTemp = new bytes(pValue, iMaxLen); \
} \
else \
{ \
   pTemp = pValue; \
} \
delete pDest; \
pDest = pTemp; \
if(pTemp != pValue) \
{ \
   delete pTemp; \
} \
SetChanged(true); \
return true;

// Get integer from table
#define GET_INT_TABLE(iFieldNum, iValue, iDefault) \
if(pTable->GetField(iFieldNum, &iValue) == false) \
{ \
   iValue = iDefault; \
}

// Set integer
#define SET_INT(iDest, iSource) \
iDest = iSource; \
SetChanged(true); \
return true;

// Set integer from EDF
#define SET_INT_EDF(iDest, szName) \
int iValue = 0; \
if(pEDF->GetChild(szName, &iValue) == false) \
{ \
   return false; \
} \
iDest = iValue; \
SetChanged(true); \
return true;

// Get EDF
#define GET_EDF \
m_pEDF->Root(); \
return m_pEDF;

// Get EDF section
#define GET_EDF_SECTION(szSection, szValue) \
m_pEDF->Root(); \
if(m_pEDF->Child(szSection, (char *)szValue) == false) \
{ \
   if(bCreate == false) \
   { \
      return NULL; \
   } \
   m_pEDF->Add(szSection, (char *)szValue); \
} \
return m_pEDF;

// Set EDF section
#define SET_EDF(szSection) \
m_pEDF->Root(); \
if(m_pEDF->Child(szSection) == false) \
{ \
   m_pEDF->Add(szSection); \
} \
SetChanged(true);

// Set EDF from string
#define SET_EDF_STR(szSection, szName, szValue, bDelNullValue, iMaxLen) \
char *szTemp = NULL; \
m_pEDF->Root(); \
if((char *)szSection != NULL && m_pEDF->Child((char *)szSection) == false) \
{ \
   if(bDelNullValue == true) \
   { \
      m_pEDF->DeleteChild((char *)szSection); \
      SetChanged(true); \
      return true; \
   } \
   else if((char *)szSection != NULL) \
   { \
      m_pEDF->Add((char *)szSection); \
   } \
} \
if(iMaxLen != -1 && szValue != NULL && strlen(szValue) > iMaxLen) \
{ \
   szTemp = strmk(szValue); \
   szTemp[iMaxLen] = '\0'; \
} \
else \
{ \
   szTemp = szValue; \
} \
if(szName != NULL) \
{ \
   m_pEDF->SetChild((char *)szName, szTemp); \
} \
else \
{ \
   m_pEDF->Set(NULL, szTemp);\
} \
if(szTemp != szValue) \
{ \
   delete[] szTemp; \
} \
SetChanged(true); \
return true;

// Set EDF from bytes
#define SET_EDF_BYTES(szSection, szName, pValue, bDelNullValue, iMaxLen) \
bytes *pTemp = NULL; \
m_pEDF->Root(); \
if((char *)szSection != NULL && m_pEDF->Child((char *)szSection) == false) \
{ \
   if(bDelNullValue == true) \
   { \
      m_pEDF->DeleteChild((char *)szSection); \
      SetChanged(true); \
      return true; \
   } \
   else if((char *)szSection != NULL) \
   { \
      m_pEDF->Add((char *)szSection); \
   } \
} \
if(iMaxLen != -1 && pValue != NULL && pValue->Length() > iMaxLen) \
{ \
   pTemp = new bytes(pValue, iMaxLen); \
} \
else \
{ \
   pTemp = pValue; \
} \
if(szName != NULL) \
{ \
   m_pEDF->SetChild((char *)szName, pTemp); \
} \
else \
{ \
   m_pEDF->Set(NULL, pTemp);\
} \
if(pTemp != pValue) \
{ \
   delete pTemp; \
} \
SetChanged(true); \
return true;

// Set EDF from integer
#define SET_EDF_INT(szSection, szName, lValue, bDelNullValue) \
m_pEDF->Root(); \
if((char *)szSection != NULL && m_pEDF->Child((char *)szSection) == false) \
{ \
   if(bDelNullValue == true) \
   { \
      m_pEDF->DeleteChild((char *)szSection); \
      SetChanged(true); \
      return true; \
   } \
   else if((char *)szSection != NULL) \
   { \
      m_pEDF->Add((char *)szSection); \
   } \
} \
if(szName != NULL) \
{ \
   m_pEDF->SetChild((char *)szName, lValue); \
} \
else \
{ \
   m_pEDF->Set(NULL, lValue);\
} \
SetChanged(true); \
return true;

#define EDFITEMWRITE_HIDDEN 1
#define EDFITEMWRITE_ADMIN 2
#define EDFITEMWRITE_FLAT 4

class EDFItem
{
public:
   EDFItem(long lID);
   EDFItem(EDF *pEDF, long lID = -1);
   EDFItem(DBTable *pTable, int iEDFField = -1);
   virtual ~EDFItem();

   // virtual const char *GetClass();

   // Access methods for base class members
   long GetID();
   EDF *GetEDF();

   bool GetDeleted();
   bool SetDeleted(bool bDeleted);

   // EDF output method
   bool Write(EDF *pEDF, int iLevel);

   // Database methods
   bool Insert(char *szTable = NULL);
   bool Delete(char *szTable = NULL);
   bool Update(char *szTable = NULL);

   bool GetChanged();
   bool SetChanged(bool bChanged);

protected:
   EDF *m_pEDF;

   // EDF extraction methods
   bool ExtractMember(EDF *pEDF, char *szName, char **szMember, bool bNullIfEmpty = false);
   bool ExtractMember(EDF *pEDF, char *szName, bytes **pMember);
   bool ExtractMember(EDF *pEDF, char *szName, int *iMember, int iDefault);
   bool ExtractMember(EDF *pEDF, char *szName, long *lMember, long lDefault);
   bool ExtractMember(EDF *pEDF, char *szName, unsigned long *lMember, unsigned long lDefault);
   bool ExtractEDF(EDF *pEDF, char *szName);

   bool EDFFields(EDF *pFields);

   // Methods to be define by subclasses
   virtual bool IsEDFField(char *szName) = 0;
   virtual bool WriteFields(EDF *pEDF, int iLevel) = 0;
   virtual bool WriteEDF(EDF *pEDF, int iLevel) = 0;
   virtual bool WriteFields(DBTable *pTable, EDF *pEDF) = 0;
   virtual bool ReadFields(DBTable *pTable) = 0;

   bool InitFields(DBTable *pTable);

   virtual char *TableName() = 0;
   virtual char *KeyName() = 0;
   virtual char *WriteName() = 0;

private:
   long m_lID;
   bool m_bDeleted;

   bool m_bChanged;

   void init(long lID);
};

#endif
