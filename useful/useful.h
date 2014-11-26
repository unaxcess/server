/*
** Useful functions
** (c) 1998 Michael Wood (mike@compsoc.man.ac.uk)
*/

#ifndef _USEFUL_H_
#define _USEFUL_H_

typedef unsigned char byte;
#ifdef FREEBSD
#define bool int
#endif

#include "StackTrace.h"
#include "LeakTrace.h"

#ifdef UNIX
#include <strings.h>
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif

class bytes
{
public:
   bytes();
   bytes(const char *szValue, int iValueLen = -1);
   bytes(const byte *pValue, int iValueLen);
   bytes(const bytes *pValue, int iValueLen = -1);
   ~bytes();

   byte *Data(bool bCopy);
   byte Char(int iPos);
   long Length();

   long Append(char *szValue);
   long Append(byte *pValue, int iValueLen);
   long Append(bytes *pValue, int iValueLen = -1);
   long Append(int iValue);

   bytes *SubString(int iLength);

   int Compare(const char *szValue);
   int Compare(const char *szValue, int iLength);
   int Compare(bytes *pValue, bool bIgnoreCase);

private:
   byte *m_pData;
   long m_lLength;
};

// String functions
char *strmk(const char *szString, int iStart, int iEnd);
char *strmk(const char *szString);
char *strmk(char cChar);
char *strmk(int iNumber);
char *strtrim(const char *szString, int iStart, int iEnd);
char *strtrim(const char *szString);
char *stristr(const char *szString1, const char *szString2);

char *streol(const char *szString);

void strpre(char *szString1, const char *szString2);

// Byte functions
byte *memmk(const byte *pData, long lDataLen);
int memprint(const byte *pData, long lDataLen, bool bRaw = false);
int memprint(const char *szTitle, const byte *pData, long lDataLen, bool bRaw = false);
int memprint(FILE *fOutput, const char *szTitle, const byte *pData, long lDataLen, bool bRaw = false);
int memprint(int iLevel, const char *szTitle, const byte *pData, long lDataLen, bool bRaw = false);
int memprint(int iLevel, FILE *fOutput, const char *szTitle, const byte *pData, long lDataLen, bool bRaw = false);

int bytesprint(const char *szTitle, const bytes *pBytes, bool bRaw = false);
int bytesprint(FILE *fOutput, const char *szTitle, const bytes *pBytes, bool bRaw = false);

// Miscellaneous function
#define BoolStr(x) (x == true ? "true" : "false")
#define mask(x, y) ((x & y) == y ? true : false)
#ifndef PALMOS
byte *FileRead(const char *szFilename, size_t *pFileLength = NULL);
long FileWrite(const char *szFilename, const byte *pWrite, long lWriteLen = -1, int iMask = -1);
#endif

double gettick();
long tickdiff(double dStart);

unsigned long memusage();

#ifdef WIN32
void MsgError(const char *szTitle);
#endif

int debug(const char *szFormat, ...);
int debug(int iLevel, const char *szFormat, ...);

bool debugbuffer(bool bBuffer);

bool debugopen(const char *szFilename, int iMask = -1);
bool debugclose(bool bDelete = false);

bool debugfile(FILE *fFile);
FILE *debugfile();

int debuglevel(int iLevel);
int debuglevel();

// Debug levels
#define DEBUGLEVEL_DEBUG 9
#define DEBUGLEVEL_INFO  7
#define DEBUGLEVEL_WARN  5
#define DEBUGLEVEL_ERR   3
#define DEBUGLEVEL_CRIT  1

#define DEBUGLEVEL_MAX DEBUGLEVEL_DEBUG

// Some handy macros

#define STR_NULL(x) \
delete[] x; \
x = NULL;

// Array element macros
#define ARRAY_INSERT(sType, pArray, iNumItems, pItem, iItemPos, pTemp) \
pTemp = new sType[iNumItems + 1]; \
if(iItemPos > 0) \
{ \
   memcpy(pTemp, pArray, iItemPos * sizeof(sType)); \
} \
if(iItemPos < iNumItems) \
{ \
   memcpy(&pTemp[iItemPos + 1], &pArray[iItemPos], (iNumItems - iItemPos) * sizeof(sType)); \
} \
pTemp[iItemPos] = pItem; \
delete[] pArray; \
pArray = pTemp; \
iNumItems++;

#define ARRAY_INC(sType, pArray, iNumItems, iMaxItems, iIncItems, pItem, iItemPos, pTemp) \
if(iNumItems == iMaxItems) \
{ \
   iMaxItems += iIncItems; \
   pTemp = new sType[iMaxItems]; \
} \
else \
{ \
   pTemp = pArray; \
} \
if(pTemp != pArray && iItemPos > 0) \
{ \
   memcpy(pTemp, pArray, iItemPos * sizeof(sType)); \
} \
if(iItemPos < iNumItems) \
{ \
   memmove(&pTemp[iItemPos + 1], &pArray[iItemPos], (iNumItems - iItemPos) * sizeof(sType)); \
} \
pTemp[iItemPos] = pItem; \
if(pArray != pTemp) \
{ \
   delete[] pArray; \
   pArray = pTemp; \
} \
iNumItems++;

#define ARRAY_DELETE(sType, pArray, iNumItems, iItemPos, pTemp) \
iNumItems--; \
pTemp = new sType[iNumItems]; \
if(iItemPos > 0) \
{ \
   memcpy(pTemp, pArray, iItemPos * sizeof(sType)); \
} \
if(iItemPos < iNumItems) \
{ \
   memcpy(&pTemp[iItemPos], &pArray[iItemPos + 1], (iNumItems - iItemPos) * sizeof(sType)); \
} \
delete[] pArray; \
pArray = pTemp; \

#define ARRAY_DEC(sType, pArray, iNumItems, iItemPos) \
iNumItems--; \
if(iItemPos < iNumItems) \
{ \
   memmove(&pArray[iItemPos], &pArray[iItemPos + 1], (iNumItems - iItemPos) * sizeof(sType)); \
}

#define ARRAY_DEC_SIZE(sType, pArray, iNumItems, iMaxItems, iIncItems, iItemPos, pTemp) \
if(iNumItems - 1 <= iMaxItems - iIncItems) \
{ \
   iMaxItems -= iIncItems; \
   pTemp = new sType[iMaxItems]; \
} \
else \
{ \
   pTemp = pArray; \
} \
if(pTemp != pArray && iItemPos > 0) \
{ \
   memcpy(pTemp, pArray, iItemPos * sizeof(sType)); \
} \
iNumItems--; \
if(iItemPos < iNumItems - 1) \
{ \
   memmove(&pTemp[iItemPos], &pArray[iItemPos + 1], (iNumItems - iItemPos) * sizeof(sType)); \
} \
if(pTemp != pArray) \
{ \
   delete[] pArray; \
   pArray = pTemp; \
}

#endif
