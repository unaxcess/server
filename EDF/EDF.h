/*
** EDF - Encapsulated Data Format
** (c) 1998 Michael Wood (mike@compsoc.man.ac.uk)
**
** EDF.h: Definition of EDF class
**
** Wraps over the less convenient EDFElement class and has a parser for creating
** an EDFElement structure
*/

#ifndef _EDF_H_
#define _EDF_H_

#include "EDFElement.h"
#include "EDFParser.h"

#define EDF_FIRST EDFElement::FIRST
#define EDF_LAST EDFElement::LAST
#define EDF_ABSFIRST EDFElement::ABSFIRST
#define EDF_ABSLAST EDFElement::ABSLAST

#define EDF_NULL EDFElement::NONE
#define EDF_BYTE EDFElement::BYTE
#define EDF_INT EDFElement::INT
#define EDF_FLOAT 0

#define EDFSET_MIN 1
#define EDFSET_MAX 2
#define EDFSET_REMOVEIFNULL 4
#define EDFSET_REMOVEIFMISSING 8

class EDF
{
public:
   /* EDF(const char *szData = NULL);
   EDF(const byte *pData, long lDataLen);
   EDF(const bytes *pData); */
   EDF(EDF *pEDF = NULL);
   EDF(EDFElement *pElement);
   ~EDF();

   bool Get(char **szName);
   bool Get(char **szName, char **szValue);
   bool Get(char **szName, bytes **pValue);
   bool Get(char **szName, int *iValue);
   bool Get(char **szName, long *lValue);
   bool Get(char **szName, double *dValue);
   int TypeGet(char **szName, char **szValue, long *lValue, double *dValue);
   int TypeGet(char **szName, bytes **pValue, long *lValue, double *dValue);

   bool Set(const char *szName, const char *szValue = NULL);
   bool Set(const char *szName, const bytes *pValue);
   bool Set(const char *szName, const int iValue);
   bool Set(const char *szName, const long lValue);
   bool Set(const char *szName, const unsigned long lValue);
   bool Set(const char *szName, const double dValue);

   bool Add(const char *szName, const char *szValue = NULL, int iPosition = EDFElement::ABSLAST);
   bool Add(const char *szName, const bytes *pValue, int iPosition = EDFElement::ABSLAST);
   bool Add(const char *szName, const int iValue, int iPosition = EDFElement::ABSLAST);
   bool Add(const char *szName, const long lValue, int iPosition = EDFElement::ABSLAST);
   bool Add(const char *szName, const double dValue, int iPosition = EDFElement::ABSLAST);

   bool Delete();

   bool GetChild(const char *szName, char **szValue, int iPosition = EDFElement::FIRST);
   bool GetChild(const char *szName, bytes **pValue, int iPosition = EDFElement::FIRST);
   bool GetChild(const char *szName, int *iValue, int iPosition = EDFElement::FIRST);
   bool GetChild(const char *szName, long *lValue, int iPosition = EDFElement::FIRST);
   bool GetChild(const char *szName, unsigned long *lValue, int iPosition = EDFElement::FIRST);
   bool GetChild(const char *szName, double *dValue, int iPosition = EDFElement::FIRST);
   bool GetChildBool(const char *szName, const bool bDefault = false, int iPosition = EDFElement::FIRST);
   int TypeGetChild(const char *szName, char **szValue, long *lValue, double *dValue, int iPosition = EDFElement::FIRST);
   int TypeGetChild(const char *szName, bytes **pValue, long *lValue, double *dValue, int iPosition = EDFElement::FIRST);

   bool SetChild(const char *szName, const char *szValue = NULL, int iPosition = EDFElement::FIRST);
   bool SetChild(const char *szName, const bytes *pValue, int iPosition = EDFElement::FIRST);
   bool SetChild(const char *szName, const int iValue, int iPosition = EDFElement::FIRST);
   bool SetChild(const char *szName, const bool bValue, int iPosition = EDFElement::FIRST);
   bool SetChild(const char *szName, const long lValue, int iPosition = EDFElement::FIRST);
   bool SetChild(const char *szName, const unsigned long lValue, int iPosition = EDFElement::FIRST);
   bool SetChild(const char *szName, const double dValue, int iPosition = EDFElement::FIRST);
   bool SetChild(EDF *pEDF);
   bool SetChild(EDF *pEDF, const char *szName, int iPosition = EDFElement::FIRST);

   bool AddChild(const char *szName, const char *szValue = NULL, int iPosition = EDFElement::ABSLAST);
   bool AddChild(const char *szName, const bytes *pValue, int iPosition = EDFElement::ABSLAST);
   bool AddChild(const char *szName, const int iValue, int iPosition = EDFElement::ABSLAST);
   bool AddChild(const char *szName, const bool bValue, int iPosition = EDFElement::ABSLAST);
   bool AddChild(const char *szName, const long lValue, int iPosition = EDFElement::ABSLAST);
   bool AddChild(const char *szName, const double dValue, int iPosition = EDFElement::ABSLAST);
   bool AddChild(EDF *pEDF);
   bool AddChild(EDF *pEDF, const char *szName, int iPosition = EDFElement::FIRST);
   bool AddChild(EDF *pEDF, const char *szName, const char *szNewName, int iPosition = EDFElement::FIRST);

   bool DeleteChild(const char *szName = NULL, const int iPosition = EDFElement::FIRST);

   int Children(const char *szName = NULL, const bool bRecurse = false);

   bool Root();

   bool Child(const char *szName = NULL, const int iPosition = EDFElement::FIRST);
   bool Child(const char *szName, const char *szValue, const int iPosition = EDFElement::FIRST);
   bool Child(const char *szName, bytes *pValue, const int iPosition = EDFElement::FIRST);
   bool IsChild(const char *szName, const char *szValue = NULL, const int iPosition = EDFElement::FIRST);
   bool First(const char *szName = NULL);
   bool Last(const char *szName = NULL);
   bool Next(const char *szName = NULL);
   bool Prev(const char *szName = NULL);
   bool Parent();

   bool Iterate(const char *szIter = NULL, const char *szStop = NULL, const bool bMatch = true, const bool bChild = true);

   bool SortReset(const char *szItems, const bool bRecurse = false);
   bool SortAddSection(const char *szName);
   bool SortAddKey(const char *szName, bool bAscending = true);
   bool SortParent();
   bool Sort();
   bool Sort(const char *szItems, const char *szKey = NULL, const bool bRecurse = false, const bool bAscending = true);

   bool Copy(EDF *pEDF, const bool bSrcCurr = true, const bool bDestSet = false, const bool bRecurse = true);

   /* long Read(const char *szData, int iProgress = -1, const int iOptions = 0);
   long Read(const bytes *pData, int iProgress = -1, const int iOptions = 0);
   long Read(const byte *pData, long lDataLen, int iProgress = -1, const int iOptions = 0);
   bytes *Write(const bool bRoot, const bool bCurr, const bool bPretty = true, const bool bCRLF = false);
   bytes *Write(int iOptions = 0); */

   bool DataCopy(const bool bDataCopy);
   bool DataCopy();

   bool TempMark();
   bool TempUnmark();

   EDFElement *GetCurr();
   bool SetCurr(EDFElement *pElement);

   bool SetRoot(EDFElement *pElement);

   bool MoveTo(EDFElement *pElement, const int iPosition = EDFElement::ABSLAST);
   bool MoveFrom(EDFElement *pElement, const int iPosition = EDFElement::ABSLAST);

   int Depth();
   int Position(const bool bName = false);

   long Storage(const int iOptions = -1);

   // static bool Debug(const bool bDebug);

   // bool SetParser(EDFParser *pParser);

protected:
   // EDFElement *child(const char *szName, const char *szValue, int iPosition);

private:
   EDFElement *m_pRoot;
   EDFElement *m_pCurr;
   EDFElement *m_pTempMark;

   bool m_bDataCopy;

   SortElement *m_pSortRoot;
   SortElement *m_pSortCurr;

   void init();

   bool sortAdd(int iType, const char *szName, bool bOption);
   bool sortDelete(SortElement *pElement);
};

#if 0
bool EDFToFile(EDF *pEDF, const char *szFilename, int iOptions = -1);
EDF *FileToEDF(const char *szFilename, size_t *lReadLen = NULL, const int iProgress = -1, const int iOptions = 0);

bool EDFPrint(EDF *pEDF, int iOptions = -1);
bool EDFPrint(EDF *pEDF, const bool bRoot, const bool bCurr);
bool EDFPrint(const char *szTitle, EDF *pEDF, int iOptions = -1);
// void EDFPrint(const char *szTitle, EDF *pEDF, const bool bRoot, const bool bCurr);
bool EDFPrint(FILE *fOutput, const char *szTitle, EDF *pEDF, int iOptions = -1);
bool EDFPrint(FILE *fOutput, const char *szTitle, EDF *pEDF, const bool bRoot, const bool bCurr);

bool debugEDFPrint(EDF *pEDF, int iOptions = -1);
bool debugEDFPrint(EDF *pEDF, const bool bRoot, const bool bCurr);
bool debugEDFPrint(int iLevel, EDF *pEDF, int iOptions = -1);
bool debugEDFPrint(const char *szTitle, EDF *pEDF, int iOptions = -1);
bool debugEDFPrint(int iLevel, const char *szTitle, EDF *pEDF, int iOptions = -1);
#endif

int EDFMax(EDF *pEDF, const char *szName, bool bRecurse = false);

int EDFSetStr(EDF *pDest, const char *szName, bytes *pBytes, int iMax, int iOptions = 0);
int EDFSetStr(EDF *pDest, EDF *pSrc, const char *szName, int iMax, int iOptions = 0, const char *szDestName = NULL);

bool EDFFind(EDF *pEDF, const char *szName, int iID, bool bRecurse);

int EDFDelete(EDF *pEDF, const char *szName, bool bRecurse = false);

#endif
