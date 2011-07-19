#ifndef _EDFPARSER_H_
#define _EDFPARSER_H_

class EDF;

class EDFParser
{
public:
   static long Read(EDF *pEDF, const char *szData, int iProgress = -1, const int iOptions = 0);
   static long Read(EDF *pEDF, const bytes *pData, int iProgress = -1, const int iOptions = 0);
   static long Read(EDF *pEDF, const byte *pData, long lDataLen, int iProgress = -1, const int iOptions = 0);
   static bytes *Write(EDF *pEDF, const bool bRoot, const bool bCurr, const bool bPretty = true, const bool bCRLF = false);
   static bytes *Write(EDF *pEDF, int iOptions = 0);

   static EDF *FromFile(const char *szFilename, size_t *lReadLen = NULL, const int iProgress = -1, const int iOptions = 0);
   static bool ToFile(EDF *pEDF, const char *szFilename, int iOptions = -1);

   static bool Print(EDF *pEDF, int iOptions = -1);
   static bool Print(EDF *pEDF, const bool bRoot, const bool bCurr);
   static bool Print(const char *szTitle, EDF *pEDF, int iOptions = -1);
   // void EDFPrint(const char *szTitle, EDF *pEDF, const bool bRoot, const bool bCurr);
   static bool Print(FILE *fOutput, const char *szTitle, EDF *pEDF, int iOptions = -1);
   static bool Print(FILE *fOutput, const char *szTitle, EDF *pEDF, const bool bRoot, const bool bCurr);

   static bool debugPrint(EDF *pEDF, int iOptions = -1);
   static bool debugPrint(EDF *pEDF, const bool bRoot, const bool bCurr);
   static bool debugPrint(int iLevel, EDF *pEDF, int iOptions = -1);
   static bool debugPrint(const char *szTitle, EDF *pEDF, int iOptions = -1);
   static bool debugPrint(int iLevel, const char *szTitle, EDF *pEDF, int iOptions = -1);
};

#endif
