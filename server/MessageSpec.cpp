#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#ifdef UNIX
#include "regex.h"
#else
#include <regex/hsregex.h>
#endif

#include "EDF/EDF.h"

#include "ua.h"

#define OP_ADD 1
#define OP_EDIT 2
#define OP_LIST 4

#define FIELD_ADMIN 1
#define FIELD_OWNER 2
#define FIELD_IGNORE 4
#define FIELD_NOADD 8

#define FIELD_STR 1
#define FIELD_BYTES 2
#define FIELD_INT 4
#define FIELD_BOOL 8
#define FIELD_EDF 16

#define USEROFF 1
#define USERON 2
#define USERNORMAL 3
#define USERADMIN 4

int iCondition = 0;
bool bIgnore = false;

typedef bool (*SERVERLINE)(EDF *, char *, char *, int , int , char *);

bool ServerSection(EDF *pData, FILE *pFile, int iIndent, int iOp, int iFieldType, char *szClass, char *szICEFN, SERVERLINE pLine);

void Panic(int iSignal)
{
   STACKPRINT
   exit(2);
}

bool strstart(char *szString, char *szMatch)
{
   if(strlen(szString) < strlen(szMatch))
   {
      return false;
   }

   return strncmp(szString, szMatch, strlen(szMatch)) == 0;
}

// strmatch: Matches a string with a given pattern
bool strmatch(char *szString, char *szPattern)
{
   STACKTRACE
   int iCompiled = 0, iMatchPos = 0, iPatternPos = 0, iOptions = REG_NOSUB;
   bool bLazy = false, bFull = false, bIgnoreCase = false, bMatch = false;
   char *szMatch = NULL;

   // printf("strmatch '%s' '%s' %s", szString, szPattern, BoolStr(bIgnoreCase));

   if(szString == NULL || szPattern == NULL)
   {
      // printf(", false\n");
      return false;
   }

   // Convert the pattern string from lazy format to the regex format
   if(bLazy == true || bFull == true)
   {
        szMatch = new char[3 * strlen(szPattern) + 3];
        iMatchPos = 0;
         szMatch[iMatchPos++] = '^';
         for(iPatternPos = 0; iPatternPos < strlen(szPattern); iPatternPos++)
         {
            if(bLazy == true && szPattern[iPatternPos] == '?')
            {
                  szMatch[iMatchPos++] = '.';
            }
            else if(bLazy == true && szPattern[iPatternPos] == '.')
            {
                  szMatch[iMatchPos++] = '\\';
                  szMatch[iMatchPos++] = '.';
            }
            else if(bLazy == true && szPattern[iPatternPos] == '*')
            {
                  szMatch[iMatchPos++] = '.';
                  szMatch[iMatchPos++] = '*';
            }
            else
            {
                  szMatch[iMatchPos++] = szPattern[iPatternPos];
            }
         }
         szMatch[iMatchPos++] = '$';
         szMatch[iMatchPos++] = '\0';

      // printf(" -> %s", szMatch);
   }
   else
   {
      szMatch = szPattern;
   }

   // Regex match the string and pattern
   regex_t *pCompiled = (regex_t *)malloc(sizeof(regex_t));
   if(bIgnoreCase == true)
   {
      iOptions |= REG_ICASE;
   }
   iCompiled = regcomp(pCompiled, szMatch, iOptions);
   if(regexec(pCompiled, szString, 1, NULL, 0) == 0)
   {
      bMatch = true;
   }
   regfree(pCompiled);
   free(pCompiled);

   if(szMatch != szPattern)
   {
      delete[] szMatch;
   }

   // printf(", %s\n", BoolStr(bMatch));
   return bMatch;
}

FILE *FileOpen(char *szFilename, char *szMode = "r")
{
   STACKTRACE
   FILE *pReturn = NULL;

   pReturn = fopen(szFilename, szMode);
   if(pReturn == NULL)
   {
      printf("FileOpen unable to open %s, %s\n", szFilename, strerror(errno));
      return NULL;
   }

   return pReturn;
}

bool FileRead(FILE *pFile, char *szLine, int iLineLen, char **szStart)
{
   STACKTRACE
   int iPos = 0;

   // printf("FileRead %p %p %d %p\n", pFile, szLine, iLineLen, szStart);

   if(feof(pFile) != 0)
   {
      printf("FileRead false, feof failed (%s)\n", strerror(errno));
      STACKPRINT
      return false;
   }

   if(fgets(szLine, iLineLen, pFile) == NULL)
   {
      printf("FileRead false, fgets failed (%s)\n", strerror(errno));
      return false;
   }

   iPos = strlen(szLine);
   while(iPos > 0 && (szLine[iPos - 1] == '\r' || szLine[iPos - 1] == '\n'))
   {
      szLine[--iPos] = '\0';
   }

   (*szStart) = szLine;
   while((**szStart) != '\0' && ((**szStart) == ' ' || (**szStart) == '\t'))
   {
      (*szStart)++;
   }

   return true;
}

bool SourceRead(FILE *pFile, char *szLine, int iLineLen, char **szStart, bool bComment)
{
   STACKTRACE
   bool bLoop = true, bReturn = true;

   // printf("SourceRead entry %p %p %d %p\n", pFile, szLine, iLineLen, szStart);

   while(bLoop == true)
   {
      bLoop = FileRead(pFile, szLine, iLineLen, szStart);

      if(bLoop == true)
      {
         if(strlen(szLine) > 0)
         {
            if(strstart((*szStart), "/*") == true)
            {
               if(bComment == true)
               {
                  printf("SourceRead comment start '%s'\n", szLine);
               }
               while(bLoop == true)
               {
                  bLoop = FileRead(pFile, szLine, iLineLen, szStart);

                  if(bLoop == true)
                  {
                     if(strstart((*szStart) + strlen((*szStart)) - 2, "*/") == true)
                     {
                        if(bComment == true)
                        {
                           printf("SourceRead comment end '%s'\n", szLine);
                        }

                        FileRead(pFile, szLine, iLineLen, szStart);

                        bLoop = false;
                     }
                     else
                     {
                        if(bComment == true)
                        {
                           printf("SourceRead comment line '%s'\n", szLine);
                        }
                     }
                  }
                  else
                  {
                     printf("SourceRead FileRead failed #1\n");
                     bReturn = false;
                  }
               }
            }
            else
            {
               bLoop = false;
            }
         }
      }
      else
      {
         printf("SourceRead FileRead failed #2\n");
         bReturn = false;
      }
   }

   // printf("SourceRead exit true\n");
   return bReturn;
}

bool SourceCheck(char *szLine, char *szStart, bool bComment)
{
   STACKTRACE
   char *szMessageSpec = NULL;

   if(strlen(szLine) == 0)
   {
      // printf("SourceCheck exit false, strlen\n");
      return false;
   }

   if(strstart(szStart, "//") == true)
   {
      if(strstart(szStart, "// MessageSpec") == true)
      {
         szMessageSpec = strstr(szStart, "MessageSpec ") + 12;

         if(strcmp(szMessageSpec, "condition start") == 0)
         {
            printf("SourceCheck message spec condition start\n");
            iCondition = 1;
         }
         else if(strcmp(szMessageSpec, "condition stop") == 0)
         {
            printf("SourceCheck message spec condition end\n");
            iCondition = 0;
         }
         else if(strcmp(szMessageSpec, "ignore start") == 0)
         {
            printf("SourceCheck message spec ignore start\n");
            bIgnore = true;
         }
         else if(strcmp(szMessageSpec, "ignore stop") == 0)
         {
            printf("SourceCheck message spec ignore end\n");
            bIgnore = false;
         }
         else
         {
            printf("SourceCheck MessageSpec line '%s' -> '%s'\n", szLine, szMessageSpec);
            return true;
         }

         return false;
      }
      else
      {
         if(bComment == true)
         {
            // printf("SourceCheck comment '%s'\n", szLine);
         }
         return false;
      }
   }

   return true;
}

bool DefineHeader(EDF *pData, char *szBase)
{
   STACKTRACE
   bool bLoop = true, bMethod = false;
   char szFilename[100], szLine[1000];
   char *szStart = NULL, *szStr = NULL, *szDefine = NULL, *szValue = NULL, *szChange = NULL, *szVersion = NULL, *szUse = NULL;
   FILE *pFile = NULL;

   printf("DefineHeader entry %p %s\n", pData, szBase);

   sprintf(szFilename, "%s/ua.h", szBase);
   pFile = FileOpen(szFilename);
   if(pFile == NULL)
   {
      printf("DefineHeader exit false, FileOpen failed\n");
      return false;
   }

   while(bLoop == true && SourceRead(pFile, szLine, sizeof(szLine), &szStart, false) == true)
   {
      if(SourceCheck(szLine, szStart, false) == true)
      {
         if(strcmp(szLine, "// MessageSpec start") == 0)
         {
            bMethod = true;
         }
         else if(strcmp(szLine, "// MessageSpec stop") == 0)
         {
            bMethod = false;
         }
         else if(bMethod == true && strstart(szLine, "#define ") == true)
         {
            printf("DefineHeader line '%s'\n", szLine);

            szStr = strchr(szLine, ' ');
            szDefine = strmk(szStr, 1, strchr(szStr + 1, ' ') - szStr);
            if(strchr(szLine, '"') != NULL)
            {
               szStr = strchr(szLine, '"');
               szValue = strmk(szStr, 1, strchr(szStr + 1, '"') - szStr);
            }
            else
            {
               szStr = strchr(szStr + 1, ' ');
               szValue = strmk(szStr + 1);
            }
            /* if(strstr(szStr, "//") != NULL)
            {
               szValue = strmk(szStr, 1, strstr(szStr, "//") - szStr - 1);
            }
            else
            {
               szValue = strmk(szStr + 1);
            } */

            printf("DefineHeader define '%s' -> '%s' '%s' '%s'\n", szLine, szDefine, szValue, szStr);

            if(pData->Child("request", szValue) == true)
            {
               pData->AddChild("define", szDefine);
            }
            else
            {
               printf("DefineHeader no request '%s'\n", szLine);
               pData->Add("lookup", szDefine);
               pData->AddChild("value", szValue);
            }

            szStr = strstr(szStr, "//");
            if(szStr != NULL)
            {
               szStr = strchr(szStr, ' ') + 1;
               if(strstart(szStr, "Added in ") == true || strstart(szStr, "Deprecated in ") == true)
               {
                  if(strstart(szStr, "Added in ") == true)
                  {
                     szChange = "add";
                  }
                  else
                  {
                     szChange = "dep";
                  }

                  if(strstr(szStr, ", use") != NULL)
                  {
                     szVersion = strmk(szStr, strstr(szStr, " in ") - szStr + 4, strstr(szStr, ", use") - szStr);
                     szUse = strstr(szStr, ", use ") + 6;
                  }
                  else
                  {
                     szVersion = strmk(strstr(szStr, " in ") + 4);
                     szUse = NULL;
                  }

                  printf("DefineHeader protocol %s '%s' '%s'\n", szChange, szVersion, szUse);

                  pData->AddChild(szChange, szVersion);
                  if(szUse != NULL)
                  {
                     pData->AddChild("use", szUse);
                  }

                  // delete[] szVersion;
               }
               else
               {
                  printf("DefineHeader protocol comment '%s'\n", szStr);
               }
            }

            pData->Parent();

            // delete[] szValue;
            // delete[] szDefine;
         }
      }
   }

   fclose(pFile);

   printf("DefineHeader exit true\n");
   return true;
}

char *DefineLookup(EDF *pData, char *szDefine)
{
   STACKTRACE
   bool bLoop = false;
   char *szName = NULL, *szValue = NULL, *szReturn = NULL;
   EDFElement *pCurr = NULL;

   printf("DefineLookup entry %s\n", szDefine);

   pCurr = pData->GetCurr();

   pData->Root();
   bLoop = pData->Child();
   while(bLoop == true)
   {
      pData->Get(&szName);
      if(strcmp(szName, "request") == 0)
      {
         // EDFPrint(pData, false, false);

         szValue = NULL;
         pData->GetChild("define", &szValue);
         if(szValue != NULL && strcmp(szValue, szDefine) == 0)
         {
            pData->Get(NULL, &szReturn);
            bLoop = false;
         }
         // delete[] szValue;
      }
      else if(strcmp(szName, "lookup") == 0)
      {
         // EDFPrint(pData, false, false);

         szValue = NULL;
         pData->Get(NULL, &szValue);
         if(szValue != NULL && strcmp(szValue, szDefine) == 0)
         {
            pData->GetChild("value", &szReturn);
            bLoop = false;
         }
         // delete[] szValue;
      }
      // delete[] szName;

      if(bLoop == true)
      {
         bLoop = pData->Next();
         if(bLoop == false)
         {
            pData->Parent();
         }
      }
   }

   pData->SetCurr(pCurr);

   printf("DefineLookup exit %s\n", szReturn);
   return szReturn;
}

bool CommonHeader(EDF *pData, char *szBase, char *szSource, char *szClass)
{
   STACKTRACE
   bool bLoop = true, bPublic = false;
   char szFilename[100], szLine[1000];
   char *szStart = NULL, *szMethod = NULL;
   FILE *pFile = NULL;

   printf("CommonHeader entry %p %s %s %s\n", pData, szBase, szSource, szClass);

   sprintf(szFilename, "%s/server/%s", szBase, szSource);
   pFile = FileOpen(szFilename);
   if(pFile == NULL)
   {
      printf("CommonHeader exit false, FileOpen failed\n");
      return false;
   }

   pData->Add("class", szClass);

   pData->Add("method", "GetID");
   pData->AddChild("datatype", FIELD_INT);
   pData->Parent();

   while(bLoop == true && SourceRead(pFile, szLine, sizeof(szLine), &szStart, false) == true)
   {
      if(SourceCheck(szLine, szStart, false) == true)
      {
         if(bPublic == false && strcmp(szLine, "public:") == 0)
         {
            bPublic = true;
         }
         else if(bPublic == true)
         {
            if(strcmp(szLine, "protected:") == 0 || strcmp(szLine, "private:") == 0)
            {
               bLoop = false;
            }
            else
            {
               if(*szStart != '\0')
               {
                  if(strstr(szStart, "Get") != NULL)
                  {
                     szMethod = strmk(szStart, strstr(szStart, "Get") - szStart, strchr(szStart, '(') - szStart);

                     printf("CommonHeader getter '%s' -> '%s'\n", szStart, szMethod);

                     pData->AddChild("method", szMethod);

                     // delete[] szMethod;
                  }
                  else if(strstart(szStart, "bool Set") == true)
                  {
                     szMethod = strmk(szStart, 5, strchr(szStart, '(') - szStart);

                     printf("CommonHeader setter '%s' -> '%s'\n", szStart, szMethod);

                     pData->AddChild("method", szMethod);

                     // delete[] szMethod;
                  }
                  else
                  {
                     // printf("CommonHeader public line '%s'\n", szLine);
                  }
               }
            }
         }
      }
   }

   pData->Parent();

   fclose(pFile);

   printf("CommonHeader exit true\n");
   return true;
}

void CommonGet(EDF *pData, char *szMacro, char *szMethod)
{
   STACKTRACE
   int iDataType = 0;
   char *szPos = NULL, *szSection = NULL, *szField = NULL;

   printf("CommonGet '%s' %s\n", szMacro, szMethod);

   if(strstart(szMacro, "return m_sz") == true || strstart(szMacro, "return sz") == true || strstart(szMacro, "GET_STR") == true)
   {
      iDataType = FIELD_STR;
   }
   else if(strstart(szMacro, "GET_BYTES") == true)
   {
      iDataType = FIELD_BYTES;
   }
   else if(strstart(szMacro, "return i") == true || strstart(szMacro, "return l") == true || strstart(szMacro, "return m_i") == true || strstart(szMacro, "return m_l") == true)
   {
      iDataType = FIELD_INT;
   }
   else if(strstart(szMacro, "GET_EDF") == true)
   {
      iDataType = FIELD_EDF;
   }
   else
   {
      printf("CommonGet no data type for macro '%s'\n", szMacro);
   }

   if(iDataType > 0)
   {
      pData->SetChild("datatype", iDataType);

      szPos = strchr(szMacro, '(') + 1;

      if(strstart(szMacro, "GET_EDF") == true)
      {
         if(*szPos == '"')
         {
            szSection = strmk(szPos, 1, strchr(szPos + 1, '"') - szPos);
            printf("CommonGet EDF section '%s'\n", szSection);
            pData->SetChild("section", szSection);
         }
      }

      if(szSection != NULL)
      {
         szField = strmk(szMethod + 3 + strlen(szSection));
      }
      else
      {
         szField = strmk(szMethod + 3);
      }
      szPos = szField;
      while(*szPos != '\0')
      {
         *szPos = tolower(*szPos);
         szPos++;
      }
      pData->SetChild("field", szField);
      // delete[] szField;

      // delete[] szSection;
   }
}

void CommonSet(EDF *pData, char *szMacro, char *szMethod)
{
   STACKTRACE
   int iDataType = 0, iLength = 0;
   char *szPos = NULL, *szLength = NULL, *szSection = NULL, *szField = NULL;

   printf("CommonSet '%s' %s\n", szMacro, szMethod);

   if(strstart(szMacro, "SET_STR") == true || strstart(szMacro, "SET_EDF_STR") == true)
   {
      iDataType = FIELD_STR;
   }
   else if(strstart(szMacro, "SET_BYTES") == true || strstart(szMacro, "SET_EDF_BYTES") == true)
   {
      iDataType = FIELD_BYTES;
   }
   else if(strstart(szMacro, "SET_INT") == true || strstart(szMacro, "SET_EDF_INT") == true)
   {
      iDataType = FIELD_INT;
   }
   else if(strstart(szMacro, "SET_EDF(") == true)
   {
      iDataType = FIELD_EDF;
   }
   else
   {
      printf("CommonSet no data type for macro '%s'\n", szMacro);
   }

   if(iDataType > 0)
   {
      pData->SetChild("datatype", iDataType);

      szPos = strchr(szMacro, '(') + 1;

      if(strstart(szMacro, "SET_EDF") == true)
      {
         if(*szPos == '"')
         {
            szSection = strmk(szPos, 1, strchr(szPos + 1, '"') - szPos);
            printf("CommonSet EDF section '%s'\n", szSection);
            pData->SetChild("section", szSection);
         }
      }

      if(iDataType == FIELD_STR || iDataType == FIELD_BYTES)
      {
         szPos = strchr(szMacro, ',');
         if(strstart(szMacro, "SET_STR_EDF") == true || strstart(szMacro, "SET_BYTES_EDF") == true)
         {
            szPos = strchr(szPos + 1, ',');
         }
         else if(strstart(szMacro, "SET_EDF_STR") == true || strstart(szMacro, "SET_EDF_BYTES") == true)
         {
            // printf("CommonSet pos 1 '%s'\n", szPos);
            szPos = strchr(szPos + 1, ',');
            // printf("CommonSet pos 2 '%s'\n", szPos);
            szPos = strchr(szPos + 1, ',');
            // printf("CommonSet pos 3 '%s'\n", szPos);
         }
         szPos = strchr(szPos + 1, ',');
         szLength = strmk(szPos, 2, strchr(szPos, ')') - szPos);
         // printf("CommonSet string length '%s'\n", szLength);
         if(strcmp(szLength, "UA_NAME_LEN") == 0)
         {
            iLength = UA_NAME_LEN;
         }
         else if(strcmp(szLength, "UA_SHORTMSG_LEN") == 0)
         {
            iLength = UA_SHORTMSG_LEN;
         }
         else if(strcmp(szLength, "-1") == 0)
         {
            iLength = -1;
         }
         else if(szLength[0] >= '0' && szLength[0] <= '9')
         {
            iLength = atoi(szLength);
         }
         else
         {
            printf("CommonSet unknown length '%s'\n", szLength);
         }
         if(iLength > 0)
         {
            pData->SetChild("length", iLength);
         }

         // delete[] szLength;
      }

      if(szSection != NULL)
      {
         szField = strmk(szMethod + 3 + strlen(szSection));
      }
      else
      {
         szField = strmk(szMethod + 3);
      }
      szPos = szField;
      while(*szPos != '\0')
      {
         *szPos = tolower(*szPos);
         szPos++;
      }
      pData->SetChild("field", szField);
      // delete[] szField;

      // delete[] szSection;
   }
}

bool CommonWriteField(EDF *pData, char *szType, char *szStart, bool bParent)
{
   STACKTRACE
   int iDataType = 0;
   char *szStr = NULL, *szField = NULL;

   printf("CommonWriteField '%s' %s\n", szStart, BoolStr(bParent));

   szStr = strchr(szStart, '"');
   if(szStr != NULL)
   {
      szField = strmk(szStr, 1, strchr(szStr + 1, '"') - szStr);
      printf("CommonWriteField field '%s'\n", szField);
   }
   else
   {
      szField = strmk("");
      printf("CommonWriteField no field string '%s'\n", szStart);
   }

   szStr = strstr(szStart, ", ");
   if(szStr != NULL)
   {
      szStr += 2;
      printf("CommonWriteField variable '%s'\n", szStr);
      if(strstart(szStr, "m_sz") == true || strstart(szStr, "sz") == true)
      {
         iDataType = FIELD_STR;
      }
      else if(strstart(szStr, "m_p") == true || strstart(szStr, "p") == true)
      {
         iDataType = FIELD_BYTES;
      }
      else if(strstart(szStr, "m_i") == true || strstart(szStr, "i") == true || strstart(szStr, "m_l") == true || strstart(szStr, "l") == true )
      {
         iDataType = FIELD_INT;
      }
      else if(strstart(szStr, "m_b") == true || strstart(szStr, "b") == true || strstart(szStr, "true") == true || strstart(szStr, "false") == true)
      {
         iDataType = FIELD_BOOL;
      }
      else
      {
         printf("CommonWriteField no write type '%s'\n", szStr);
      }
   }
   else
   {
      printf("CommonWriteField no write type '%s'\n", szStart);
   }

   if(bParent == false)
   {
      if(iCondition == 2)
      {
         printf("CommonWriteField condition parent\n");
         pData->Parent();
      }
      else if(iCondition == 1)
      {
         iCondition = 2;
      }
   }

   if(pData->Child(szType, szField) == true)
   {
      printf("CommonWriteField already have '%s' section\n", szField);
   }
   else
   {
      pData->Add(szType, szField);
   }

   if(iDataType > 0)
   {
      pData->SetChild("datatype", iDataType);
   }

   if(bParent == true)
   {
      pData->Parent();
   }

   // delete[] szField;

   return true;
}

bool CommonWrite(EDF *pData, char *szLine, char *szStart)
{
   STACKTRACE
   char szCall[100];
   char *szFunction = NULL;

   strcpy(szCall, "^ *Write.*(pEDF, iLevel.*);$");

   if(strmatch(szLine, szCall) == true)
   {
      printf("CommonWrite call match '%s'\n", szLine);

      szFunction = strmk(szStart, 0, strchr(szStart, '(') - szStart);
      if(pData->Child("write", szFunction) == false)
      {
         pData->Add("write", szFunction);
      }
      else
      {
         pData->SetChild("multiple", true);
      }
      pData->Parent();
      // delete[] szFunction;
   }
   else
   {
      if(strstart(szStart, "pEDF->") == true)
      {
         if(strstart(szStart, "pEDF->AddChild(") == true)
         {
            CommonWriteField(pData, "out", szStart, true);
         }
         else if(strstart(szStart, "pEDF->Add(") == true)
         {
            CommonWriteField(pData, "outsection", szStart, false);
         }
         else if(strcmp(szStart, "pEDF->Parent();") == 0)
         {
            printf("CommonWrite EDF parent\n");
            pData->Parent();
         }
         else
         {
            printf("CommonWrite EDF line '%s'\n", szLine);
         }
      }
      else
      {
         printf("CommonWrite line '%s'\n", szLine);
      }
   }

   return true;
}

bool CommonSource(EDF *pData, char *szBase, char *szSource, char *szClass, char *szWriteSection)
{
   STACKTRACE
   int iMethod = 0;
   char szFilename[100], szLine[1000], szSet[100], szGet[100], szWriteMatch[100];
   char *szStart = NULL, *szMethod = NULL;
   FILE *pFile = NULL;

   printf("CommonSource entry %p %s %s %s\n", pData, szBase, szSource, szClass);

   sprintf(szFilename, "%s/server/%s", szBase, szSource);
   pFile = FileOpen(szFilename);
   if(pFile == NULL)
   {
      printf("CommonSource exit false, FileOpen failed\n");
      return false;
   }

   sprintf(szGet, "%s::Get", szClass);
   sprintf(szSet, "bool %s::Set", szClass);
   sprintf(szWriteMatch, "^bool %s::Write.*(EDF \\*pEDF, int iLevel.*)$", szClass);

   pData->Child("class", szClass);

   while(SourceRead(pFile, szLine, sizeof(szLine), &szStart, false) == true)
   {
      if(SourceCheck(szLine, szStart, false) == true)
      {
         if(iMethod > 0 && strcmp(szLine, "}") == 0)
         {
            printf("CommonSource method end\n");

            pData->Parent();

            // delete[] szMethod;
            iMethod = 0;
         }
         else if(iMethod == 1)
         {
            if(strstart(szStart, "return ") == true || strstart(szStart, "GET_") == true)
            {
               CommonGet(pData, szStart, szMethod);
            }
         }
         else if(iMethod == 2)
         {
            if(strstart(szStart, "SET_") == true)
            {
               CommonSet(pData, szStart, szMethod);
            }
         }
         else if(iMethod == 3)
         {
            CommonWrite(pData, szLine, szStart);
         }
         else
         {
            if(strstr(szLine, szGet) != NULL)
            {
               szMethod = strmk(strstr(szLine, szGet), strlen(szGet) - 3, strchr(szLine, '(') - strstr(szLine, szGet));

               printf("CommonSource get method '%s' -> '%s'\n", szLine, szMethod);

               if(pData->Child("method", szMethod) == false)
               {
                  printf("CommonSource no section for get method '%s'\n", szMethod);
                  pData->Add("function", szMethod);
               }

               if(bIgnore == true)
               {
                  printf("CommonSource ignore method\n");
                  pData->AddChild("ignore", true);
               }

               iMethod = 1;
            }
            else if(strstart(szLine, szSet) == true)
            {
               szMethod = strmk(szLine, strlen(szSet) - 3, strchr(szLine, '(') - szLine);

               printf("CommonSource set method '%s' -> '%s'\n", szLine, szMethod);

               if(pData->Child("method", szMethod) == false)
               {
                  printf("CommonSource no section for set method '%s'\n", szMethod);
                  pData->Add("function", szMethod);
               }

               if(bIgnore == true)
               {
                  printf("CommonSource ignore method\n");
                  pData->AddChild("ignore", true);
               }

               iMethod = 2;
            }
            else if(strmatch(szLine, szWriteMatch) == true)
            {
               szMethod = strmk(szLine, strlen(szClass) + 7, strchr(szLine, '(') - szLine);

               printf("CommonSource method '%s' -> '%s'\n", szLine, szMethod);

               if(strcmp(szMethod, "Write") == 0)
               {
                  printf("CommonSource Write override\n");
               }
               else
               {
                  pData->Add("write", szMethod);

                  if(strcmp(szMethod, "WriteFields") == 0)
                  {
                     pData->SetChild("writefields", szWriteSection);
                  }

                  iMethod = 3;
               }
            }
            else
            {
               // printf("CommonSource outside line '%s'\n", szLine);
            }
         }
      }
   }

   pData->Parent();

   fclose(pFile);

   printf("CommonSource exit true\n");
   return true;
}

bool ServerField(EDF *pData, char *szName, char *szField, int iDataType, int iFieldType, int iLength, bool bParent)
{
   STACKTRACE
   bool bReturn = false;

   printf("ServerField %p %s '%s' %d %d %d %s\n", pData, szName, szField, iDataType, iFieldType, iLength, BoolStr(bParent));

   /* if(mask(iFieldType, FIELD_IGNORE) == true)
   {
      printf("ServerField ignoring %s '%s'\n", szName, szField);
   }
   else */
   {
      if(mask(iFieldType, FIELD_NOADD) == false)
      {
         if(pData->Child(szName, szField) == true)
         {
            printf("ServerField already %s field '%s'\n", szName, szField);
         }
         else
         {
            if(iDataType == FIELD_EDF)
            {
               pData->SetChild("datatype", FIELD_EDF);
               bParent = false;
               bReturn = true;
            }
            else
            {
               if(mask(iFieldType, FIELD_NOADD) == false)
               {
                  pData->Add(szName, szField);
               }
               else
               {
                  bParent = false;
               }
            }
         }
      }
      else
      {
         bParent = false;
      }

      if(bReturn == false)
      {
         if(iDataType > 0)
         {
            pData->SetChild("datatype", iDataType);
         }
         if(iLength > 0)
         {
            pData->SetChild("length", iLength);
         }

         if(bIgnore == true)
         {
            printf("ServerField ignore on\n");
            iFieldType |= FIELD_IGNORE;
         }
         if(iFieldType > 0)
         {
            pData->SetChild("fieldtype", iFieldType);
         }
      }

      if(bParent == true)
      {
         pData->Parent();
      }
   }

   return true;
}

bool ServerField(EDF *pData, char *szClass, char *szMethod, char *szName, int iFieldType = -1)
{
   STACKTRACE
   int iDataType = 0, iLength = -1;
   bool bReturn = false;
   char *szField = NULL;
   EDFElement *pCurr = NULL;

   printf("ServerField %p %s %s %s %d\n", pData, szClass, szMethod, szName, iFieldType);

   pCurr = pData->GetCurr();

   pData->Root();
   pData->Child("class", szClass);
   if(pData->Child("method", szMethod) == true)
   {
      pData->GetChild("datatype", &iDataType);
      if(iFieldType <= 0)
      {
         pData->GetChild("fieldtype", &iFieldType);
         if(pData->GetChildBool("ignore") == true)
         {
            iFieldType |= FIELD_IGNORE;
         }
      }
      pData->GetChild("length", &iLength);
      pData->GetChild("field", &szField);
      pData->Parent();
   }
   else
   {
      printf("ServerField no method %s in class %s\n", szMethod, szClass);
   }

   pData->SetCurr(pCurr);

   if(mask(iFieldType, FIELD_IGNORE) == false)
   {
      bReturn = ServerField(pData, szName, szField, iDataType, iFieldType, iLength, true);
   }
   else
   {
      printf("ServerField ignoring field\n");
   }

   // delete[] szField;

   return bReturn;
}

bool ServerInField(EDF *pData, char *szStr, char *szName, int iFieldType, bool bParent)
{
   int iDataType = 0;
   char *szField = NULL, *szQuote = NULL, *szVar = NULL;

   // printf("ServerInField %s %d %s\n", szName, iField, BoolStr(bParent));

   if(strchr(szStr, '"') != NULL)
   {
      if(strstr(szStr, "? \"") != NULL)
      {
         szStr = strstr(szStr, "? \"") + 1;
      }
      szQuote = strchr(szStr + 1, '"');
      szVar = strchr(szQuote, '&');

      szField = strmk(szStr, szQuote - szStr + 1, strchr(szQuote + 1, '"') - szStr);
      // printf("ServerInField child line '%s' -> '%s'\n", szStr, szField);
   }
   else
   {
      // printf("ServerInField no field string '%s'\n", szStr);
      szField = strmk("");
      szVar = strchr(szStr, '&');
   }

   if(szVar != NULL)
   {
      if(strstart(szVar, "&sz") == true)
      {
         iDataType = FIELD_STR;
      }
      else if(strstart(szVar, "&p") == true)
      {
         iDataType = FIELD_BYTES;
      }
      else if(strstart(szVar, "&i") == true || strstart(szVar, "&l") == true)
      {
         iDataType = FIELD_INT;
      }
      else
      {
         printf("ServerInField no data type for var '%s'\n", szVar);
      }
   }
   else
   {
      if(strstart(szStr, "pIn->GetChildBool(\"") == true)
      {
         iDataType = FIELD_BOOL;
      }
      else
      {
         printf("ServerInField no data type for NULL var '%s'\n", szStr);
      }
   }

   ServerField(pData, szName, szField, iDataType, iFieldType, -1, bParent);

   // delete[] szField;

   return true;
}

bool ServerIn(EDF *pData, char *szLine, int iFieldType)
{
   char *szStr = NULL;

   szStr = strstr(szLine, "pIn->");
   if(szStr == NULL)
   {
      return false;
   }

   /* if(bIgnore == true)
   {
      printf("ServerIn ignoring input line '%s'\n", szStr);
   }
   else */
   {
      if(strstart(szStr, "pIn->Child(") == true)
      {
         ServerInField(pData, szStr, "insection", iFieldType, false);
      }
      else if(strcmp(szStr, "pIn->Parent();") == 0)
      {
         printf("ServerIn parent line\n");
         pData->Parent();
      }
      else if(strstart(szStr, "pIn->Get(") == true || strstart(szStr, "pIn->GetChild(") == true || strstart(szStr, "pIn->GetChildBool(") == true)
      {
         if(strstart(szStr, "pIn->Get(") == true)
         {
            printf("ServerIn no add field\n");
            iFieldType |= FIELD_NOADD;
         }
         ServerInField(pData, szStr, "in", iFieldType, true);
      }
      else
      {
         printf("ServerIn input line '%s'\n", szStr);
      }
   }

   return true;
}

bool UserLine(EDF *pData, char *szLine, char *szStart, int iOp, int iFieldType, char *szClass)
{
   STACKTRACE
   char *szStr = NULL, *szField = NULL, *szLookup = NULL;
   EDFElement *pCurr = NULL;

   // printf("UserLine %p '%s' '%s' %d %d '%s'\n", pData, szLine, szStart, iOp, iFieldType, szClass);

   if(strstr(szLine, "UserAccess(pCurr, pIn, ") != NULL)
   {
      szStr = strchr(szLine, ',');
      szStr = strchr(szStr + 1, ',');
      szStr = strchr(szStr + 1, '"');
      szField = strmk(szStr, 1, strchr(szStr + 1, '"') - szStr);

      printf("UserLine access check '%s' -> '%s'\n", szLine, szField);

      ServerField(pData, "in", szField, FIELD_INT, iFieldType | FIELD_ADMIN | FIELD_OWNER, -1, true);

      pCurr = pData->GetCurr();

      szLookup = DefineLookup(pData, "MSG_USER_NOT_EXIST");
      if(pData->Child("reply", "MSG_USER_NOT_EXIST") == false)
      {
         pData->Add("reply", "MSG_USER_NOT_EXIST");
      }
      pData->Root();
      if(pData->Child("reply", szLookup) == false)
      {
         pData->Add("reply", szLookup);
      }
      if(pData->Child("out", "userid") == false)
      {
         pData->Add("out", "userid");
      }
      // delete[] szLookup;

      pData->SetCurr(pCurr);

      szLookup = DefineLookup(pData, "MSG_ACCESS_INVALID");
      if(pData->Child("reply", "MSG_ACCESS_INVALID") == false)
      {
         pData->Add("reply", "MSG_ACCESS_INVALID");
      }
      pData->Root();
      if(pData->Child("reply", szLookup) == false)
      {
         pData->Add("reply", szLookup);
      }
      // delete[] szLookup;

      pData->SetCurr(pCurr);

      // delete[] szField;
   }
   else if(strstr(szLine, "ConnectionShut(pConn, pData, ") != NULL)
   {
      printf("UserLine connection shut call\n");
      if(pData->Child("call", "ConnectionShut") == false)
      {
         pData->Add("call", "ConnectionShut");
      }
      pData->Parent();
   }

   return true;
}

bool ServerOutField(EDF *pData, const char *szType, char *szStart, bool bParent, char *szClass)
{
   STACKTRACE
   int iDataType = 0;
   char *szStr = NULL, *szField = NULL, *szMethod = NULL;
   EDFElement *pCurr = NULL;

   printf("ServerOutField '%s' %s\n", szStart, BoolStr(bParent));

   szStr = strchr(szStart, '"');
   if(szStr != NULL)
   {
      szField = strmk(szStr, 1, strchr(szStr + 1, '"') - szStr);
      printf("ServerOutField field '%s'\n", szField);
   }
   else
   {
      szField = strmk("");
      printf("ServerOutField no field string '%s'\n", szStart);
   }

   if(strstr(szStr, "// MessageSpec") != NULL)
   {
      szStr = strstr(szStr, "// MessageSpec") + 15;
      printf("ServerOutField comment type %s\n", szStr);

      if(strstart(szStr, "STR") == true)
      {
         iDataType = FIELD_STR;
      }
      else if(strstart(szStr, "INT") == true)
      {
         iDataType = FIELD_INT;
      }
      else
      {
         printf("ServerOutField no data type for message spec '%s'\n", szStr);
      }
   }
   else
   {
      if(strstr(szStr, ", ") != NULL)
      {
         szStr = strstr(szStr, ", ") + 2;
         if(strstart(szStr, "pItem->Get") == true)
         {
            szMethod = strmk(szStr, 7, strchr(szStr, '(') - szStr);
            printf("ServerOutField item get method '%s'\n", szMethod);

            pCurr = pData->GetCurr();
            pData->Root();
            pData->Child("class", szClass);
            if(pData->Child("method", szMethod) == true)
            {
               pData->GetChild("datatype", &iDataType);
            }
            pData->SetCurr(pCurr);
         }
         else if(strstart(szStr, "sz") == true || strstr(szStr, "name\"") != NULL)
         {
            iDataType = FIELD_STR;
         }
         else if(strstart(szStr, "p") == true)
         {
            iDataType = FIELD_BYTES;
         }
         else if(strstart(szStr, "i") == true || strstart(szStr, "l") == true)
         {
            iDataType = FIELD_INT;
         }
         else if(strstart(szStr, "true") == true || strstart(szStr, "false") == true || strstart(szStr, "b") == true)
         {
            iDataType = FIELD_BOOL;
         }
         else
         {
            printf("ServerOutField no data type for get '%s'\n", szStr);
         }
      }
   }

   if(bParent == false)
   {
      if(iCondition == 2)
      {
         printf("ServerOutField condition parent\n");
         pData->Parent();
      }
      else if(iCondition == 1)
      {
         iCondition = 2;
      }
   }

   if(pData->Child(szType, szField) == true)
   {
      printf("ServerOutField already have '%s' section\n", szField);
   }
   else
   {
      pData->Add(szType, szField);
   }

   if(iDataType > 0)
   {
      pData->SetChild("datatype", iDataType);
   }

   if(bParent == true)
   {
      pData->Parent();
   }

   // delete[] szField;

   return true;
}

bool ServerOut(FILE *pFile, EDF *pData, char *szStart, char *szClass)
{
   STACKTRACE

   if(strstart(szStart, "pOut->AddChild(") == true)
   {
      ServerOutField(pData, "out", szStart, true, szClass);
   }
   else if(strstart(szStart, "pOut->Add(") == true)
   {
      ServerOutField(pData, strchr(szStart, ',') == NULL ? "outsection" : "out", szStart, false, szClass);
   }
   else if(strcmp(szStart, "pOut->Parent();") == 0)
   {
      printf("ServerOut output parent\n");
      pData->Parent();
   }

   return true;
}

bool ServerReply(FILE *pFile, EDF *pData, char *szStart, char *szClass)
{
   STACKTRACE
   bool bLoop = true;
   char szLine[1000];
   char *szReply = NULL, *szLookup = NULL;
   EDFElement *pCurr = NULL;

   szReply = strmk(szStart, 19, strchr(szStart, ')') - szStart);
   szLookup = DefineLookup(pData, szReply);
   printf("ServerReply '%s' -> '%s', '%s'\n", szStart, szReply, szLookup);

   if(pData->Child("reply", szReply) == false)
   {
      pData->Add("reply", szReply);
   }
   pData->Parent();

   pCurr = pData->GetCurr();

   pData->Root();
   if(pData->Child("reply", szLookup) == false)
   {
      pData->Add("reply", szLookup);
   }
   while(bLoop == true && SourceRead(pFile, szLine, sizeof(szLine), &szStart, true) == true)
   {
      if(SourceCheck(szLine, szStart, true) == true)
      {
         if(strcmp(szStart, "return false;") == 0 || strcmp(szStart, "return bReturn;") == 0)
         {
            printf("ServerReply end '%s'\n", szLine);
            bLoop = false;
         }
         else if(strstart(szStart, "pOut->") == true)
         {
            ServerOut(pFile, pData, szStart, szClass);
         }
         else
         {
            printf("ServerReply line '%s'\n", szLine);
         }
      }
   }
   pData->Parent();

   // delete[] szReply;

   pData->SetCurr(pCurr);

   return true;
}

bool ServerLine(EDF *pData, FILE *pFile, char *szLine, char *szStart, int iOp, int iFieldType, char *szClass, char *szICEFN, SERVERLINE pLine)
{
   STACKTRACE
   int iCheckType = 0;
   bool bProcess = false, bCheck = false;
   char szCall[100], szWrite[100], szCallFN[100];
   char *szStr = NULL, *szFunction = NULL, *szMethod = NULL;

   // printf("ServerLine '%s'\n", szLine);
   if(szClass == NULL)
   {
      printf("ServerLine %p %p '%s' '%s' %d %d '%s' '%s' %p\n", pData, pFile, szLine, szStart, iOp, iFieldType, szClass, szICEFN, pLine);
   }

   strcpy(szCall, "");
   strcpy(szWrite, "");
   strcpy(szCallFN, "");

   if(iOp == OP_ADD || iOp == OP_EDIT)
   {
      sprintf(szCall, "^ *%sEdit.*(pData, .*, p.*, pIn, pOut, pCurr.*);$", szClass);
   }
   else if(iOp == OP_LIST)
   {
      sprintf(szCall, "^ *%sList(pOut.*);$", szClass);
      strcpy(szWrite, "^ *pItem->Write(pOut, iLevel.*);");
   }
   else
   {
      sprintf(szCall, "^ *%sList(pOut.*);$", szClass);
      sprintf(szCallFN, "^ *.* = %s.*(.*);$", szICEFN);
   }

   if(strmatch(szLine, szCall) == true || strmatch(szLine, szCallFN) == true)
   {
      printf("ServerLine call match '%s'\n", szLine);

      szFunction = strmk(szStart, 0, strchr(szStart, '(') - szStart);
      if(pData->Child("call", szFunction) == false)
      {
         pData->Add("call", szFunction);
      }
      else
      {
         pData->SetChild("multiple", true);
      }
      if(strstr(szStart, "EDFITEMWRITE_FLAT") != NULL)
      {
         pData->SetChild("flat", true);
      }
      pData->Parent();
      // delete[] szFunction;
   }
   if(strmatch(szLine, szWrite) == true)
   {
      printf("ServerLine write match '%s'\n", szLine);

      if(pData->Child("write", szClass) == false)
      {
         pData->Add("write", szClass);
      }
      else
      {
         pData->SetChild("multiple", true);
      }
      // pData->Parent();
   }
   else if(strstart(szStart, "pOut->Set(\"reply") == true)
   {
      ServerReply(pFile, pData, szStart, szClass);
   }
   else if(strstart(szStart, "pOut->") == true)
   {
      ServerOut(pFile, pData, szStart, szClass);
   }
   else
   {
      szStr = strstr(szLine, "pItem->Set");
      if(szStr != NULL)
      {
         szMethod = strmk(szStr, 7, strchr(szStr, '(') - szStr);

         printf("ServerLine set line '%s' -> '%s', %d\n", szStr, szMethod, iFieldType);

         ServerField(pData, szClass, szMethod, "in", iFieldType);

         // delete[] szMethod;

         bProcess = true;
      }

      if(bProcess == false)
      {
         bProcess = ServerIn(pData, szLine, iFieldType);
      }

      if(bProcess == false && pLine != NULL)
      {
         bProcess = (*pLine)(pData, szLine, szStart, iOp, iFieldType, szClass);
      }

      if(bProcess == false)
      {
         szStr = strstr(szLine, "pCurr->GetAccessLevel() >= LEVEL_WITNESS");
         if(szStr != NULL)
         {
            printf("ServerLine access check line '%s'\n", szLine);
            bCheck = true;
            iCheckType |= FIELD_ADMIN;
         }

         if(bCheck == false)
         {
            szStr = strstr(szLine, "pCurr->GetOwnerID() == pItem->GetID()");
            if(szStr != NULL)
            {
               printf("ServerLine owner check line '%s'\n", szLine);
               bCheck = true;
               iCheckType |= FIELD_ADMIN;
            }
         }

         if(bCheck == false)
         {
            if(strstr(szLine, "if(iOp ==") != NULL || strstr(szLine, "if(iOp !=") != NULL)
            {
               printf("ServerLine op check line '%s'\n", szLine);
               bCheck = true;
            }
         }

         if(bCheck == true)
         {
            ServerSection(pData, pFile, szStart - szLine, iOp, iCheckType, szClass, szICEFN, pLine);
         }
         else
         {
            // printf("ServerLine edit line '%s'\n", szLine);
         }
      }
   }

   return true;
}

bool ServerSection(EDF *pData, FILE *pFile, int iIndent, int iOp, int iFieldType, char *szClass, char *szICEFN, SERVERLINE pLine)
{
   STACKTRACE
   bool bLoop = true;
   char szLine[1000];
   char *szStart = NULL;

   printf("ServerSection entry %p %p %d %d %d %s\n", pData, pFile, iIndent, iOp, iFieldType, szClass);

   while(bLoop == true && SourceRead(pFile, szLine, sizeof(szLine), &szStart, true) == true)
   {
      if(SourceCheck(szLine, szStart, true) == true)
      {
         if(strcmp(szStart, "}") == 0)
         {
            // printf("ServerSection close brace indent %d\n", iIndent);
            if(szStart - szLine == iIndent)
            {
               bLoop = false;
            }
         }
         else
         {
            ServerLine(pData, pFile, szLine, szStart, iOp, iFieldType, szClass, szICEFN, pLine);
         }
      }
   }

   printf("ServerSection exit true, %d\n", iIndent);
   return true;
}

bool ServerSource(EDF *pData, char *szBase)
{
   STACKTRACE
   bool bLoop = true, bMethod = false;
   char szFilename[100], szLine[1000];
   char *szStart = NULL;
   FILE *pFile = NULL;

   printf("ServerSource entry %p %s\n", pData, szBase);

   pData->Root();

   sprintf(szFilename, "%s/server/server.cpp", szBase);
   pFile = FileOpen(szFilename);
   if(pFile == NULL)
   {
      printf("ServerSource exit false, FileOpen failed\n");
      return false;
   }

   while(bLoop == true && SourceRead(pFile, szLine, sizeof(szLine), &szStart, bMethod) == true)
   {
      if(SourceCheck(szLine, szStart, bMethod) == true)
      {
         if(bMethod == true)
         {
            if(strcmp(szLine, "}") == 0)
            {
               printf("ServerSource move to parent\n");
               pData->Parent();

               bMethod = false;
            }
            else
            {
               ServerLine(pData, pFile, szLine, szStart, 0, 0, NULL, NULL, NULL);
            }
         }
         else
         {
            if(strcmp(szLine, "bool ConnectionShut(EDFConn *pConn, EDF *pData, int iType, int iID, UserItem *pCurr, EDF *pIn, bool bLost)") == 0)
            {
               printf("ServerSource connection shut function\n");

               pData->Add("function", "ConnectionShut");

               bMethod = true;
            }
         }
      }
   }

   fclose(pFile);

   printf("ServerSource exit true\n");
   return true;
}

bool ServerSource(EDF *pData, char *szBase, char *szSource, char *szClass, char *szGroup, SERVERLINE pLine)
{
   STACKTRACE
   int iOp = 0;
   bool bLoop = true, bMethod = false;
   char szFilename[100], szLine[1000];
   char szEditMatch[100], szListMatch[100], szRequestMatch[100];
   char *szStr = NULL, *szStart = NULL, *szFunction = NULL, *szICEFN = NULL, *szValue = NULL;
   FILE *pFile = NULL;
   EDFElement *pCurr = NULL;

   printf("ServerSource entry %p %s %s %s %s\n", pData, szBase, szSource, szClass, szGroup);

   if(szClass != NULL)
   {
      pData->Root();
      if(pData->IsChild("class", szClass) == false)
      {
         printf("ServerSource exit false, no class section\n");
         return false;
      }
   }

   sprintf(szFilename, "%s/server/%s", szBase, szSource);
   pFile = FileOpen(szFilename);
   if(pFile == NULL)
   {
      printf("ServerSource exit false, FileOpen failed\n");
      return false;
   }

   if(szClass != NULL)
   {
      sprintf(szEditMatch, "^bool %sEdit.*(EDF \\*pData, int iOp, %s \\*pItem, EDF \\*pIn, EDF \\*pOut.*)$", szClass, szClass);
      printf("ServerSource edit match '%s'\n", szEditMatch);

      sprintf(szListMatch, "^bool %sList.*(EDF \\*pOut, %s \\*pItem, int iLevel.*)$", szClass, szClass);
      printf("ServerSource list match '%s'\n", szListMatch);
   }
   else
   {
      strcpy(szEditMatch, "");
      strcpy(szListMatch, "");
   }

   sprintf(szRequestMatch, "^ICELIBFN bool %s.*(EDFConn \\*pConn, EDF \\*pData, EDF \\*pIn, EDF \\*pOut)$", szGroup != NULL ? szGroup : ".*");
   printf("ServerSource request match '%s'\n", szRequestMatch);

   while(SourceRead(pFile, szLine, sizeof(szLine), &szStart, bMethod) == true)
   {
      if(SourceCheck(szLine, szStart, bMethod) == true)
      {
         if(bMethod == true)
         {
            if(strcmp(szLine, "}") == 0)
            {
               printf("ServerSource move to parent\n");
               pData->Parent();

               iOp = 0;
               bMethod = false;

               // delete[] szICEFN;
               szICEFN = NULL;
            }
            else
            {
               ServerLine(pData, pFile, szLine, szStart, iOp, 0, szClass, szICEFN, pLine);
            }
         }
         else
         {
            if(strmatch(szLine, szEditMatch) == true)
            {
               iOp = OP_EDIT;
            }
            else if(strmatch(szLine, szListMatch) == true)
            {
               iOp = OP_LIST;
            }

            if(iOp > 0)
            {
               printf("ServerSource op match '%s'\n", szLine);

               bMethod = true;

               szStr = strchr(szLine, ' ');
               szICEFN = strmk(szStr, 1, strchr(szStr, '(') - szStr);
               printf("ServerSource op function '%s'\n", szICEFN);

               pData->Root();
               pData->Add("function", szICEFN);

               // delete[] szICEFN;
            }
            else if(strmatch(szLine, szRequestMatch) == true)
            {
               printf("ServerSource op request '%s'\n", szLine);

               bMethod = true;

               if(szGroup != NULL)
               {
                  szStr = strstr(szLine, szGroup);
               }
               else
               {
                  szStr = szLine + 14;
               }
               szICEFN = strmk(szStr, 0, strchr(szStr, '(') - szStr);

               if(strcmp(szICEFN + strlen(szICEFN) - 3, "Add") == 0)
               {
                  iOp = OP_ADD;
               }
               else if(strcmp(szICEFN + strlen(szICEFN) - 4, "Edit") == 0)
               {
                  iOp = OP_EDIT;
               }
               else if(strcmp(szICEFN + strlen(szICEFN) - 4, "List") == 0)
               {
                  iOp = OP_LIST;
               }

               printf("ServerSource op function '%s' -> %d\n", szICEFN, iOp);

               pData->Root();
               pData->Add("function", szICEFN);

               // delete[] szFunction;
            }
            else if(strstart(szLine, "bool ") == true)
            {
               szFunction = strmk(szLine, 5, strchr(szLine, '(') - szLine);

               printf("ServerSource function line '%s' -> %s\n", szLine, szFunction);

               pCurr = pData->GetCurr();

               bLoop = pData->Child("request");
               while(bLoop == true)
               {
                  szValue = NULL;

                  pData->GetChild("function", &szValue);
                  if(szValue != NULL)
                  {
                     if(strstart(szFunction, szValue) == true)
                     {
                        printf("ServerSource request helper '%s'\n", szFunction);
                        bLoop = false;
                     }
                     else
                     {
                        bLoop = pData->Next("request");
                        if(bLoop == false)
                        {
                           pData->Parent();
                        }
                     }
                  }

                  // delete[] szValue;
               }

               // delete[] szFunction;

               pData->SetCurr(pCurr);
            }
         }
      }
   }

   fclose(pFile);

   printf("ServerSource exit true\n");
   return true;
}

char *HTMLConv(char *szText, bool bBR)
{
   int iTextPos = 0, iReturnPos = 0;
   char *szReturn = new char[(4 * strlen(szText))];

   for(iTextPos = 0; iTextPos < strlen(szText); iTextPos++)
   {
      if(szText[iTextPos] == '\r')
      {
      }
      else if(szText[iTextPos] == '<')
      {
         szReturn[iReturnPos++] = '&';
         szReturn[iReturnPos++] = 'l';
         szReturn[iReturnPos++] = 't';
         szReturn[iReturnPos++] = ';';
      }
      else if(szText[iTextPos] == '>')
      {
         szReturn[iReturnPos++] = '&';
         szReturn[iReturnPos++] = 'g';
         szReturn[iReturnPos++] = 't';
         szReturn[iReturnPos++] = ';';
      }
      else
      {
         if(szText[iTextPos] == '\n' && bBR == true)
         {
            szReturn[iReturnPos++] = '<';
            szReturn[iReturnPos++] = 'b';
            szReturn[iReturnPos++] = 'r';
            szReturn[iReturnPos++] = '>';
         }

         szReturn[iReturnPos++] = szText[iTextPos];
      }
   }
   szReturn[iReturnPos] = '\0';

   return szReturn;
}

bool HTMLPrint(FILE *pFile, EDF *pEDF, int iOptions)
{
   bytes *pWrite = NULL;
   char *szHTML = NULL;

   pWrite = EDFParser::Write(pEDF, iOptions);
   szHTML = HTMLConv((char *)pWrite->Data(false), false);
   fprintf(pFile, "\n<pre>\n");
   fwrite(szHTML, 1, strlen(szHTML), pFile);
   fprintf(pFile, "</pre>\n");
   // delete[] szHTML;
   delete pWrite;

   return true;
}

bool HTMLField(EDF *pFields, EDF *pData, char *szName, char *szValue, bool bMultiple, bool bParent)
{
   STACKTRACE
   int iDataType = 0, iFieldType = 0, iLength = -1;

   pData->GetChild("datatype", &iDataType);
   pData->GetChild("fieldtype", &iFieldType);
   pData->GetChild("length", &iLength);

   printf("HTMLField field '%s' %d %d %d %s %s\n", szValue, iDataType, iFieldType, iLength, BoolStr(bMultiple), BoolStr(bParent));

   if(pFields->Child(szName, szValue) == false)
   {
      pFields->Add(szName, szValue);
   }

   if(iDataType > 0)
   {
      pFields->SetChild("datatype", iDataType);
   }
   if(iFieldType > 0)
   {
      pFields->SetChild("fieldtype", iFieldType);
   }
   if(iLength > 0)
   {
      pFields->SetChild("length", iLength);
   }
   if(bMultiple == true)
   {
      pFields->SetChild("multiple", true);
   }

   if(bParent == true)
   {
      pFields->Parent();
   }

   return true;
}

bool HTMLFields(EDF *pData, EDF *pFields, char *szType, bool bMultiple, bool bFlat, char *szClass)
{
   STACKTRACE
   int iDataType = 0, iFieldType = 0;
   bool bLoop = true, bMultipleTemp = false;
   char *szName = NULL, *szValue = NULL, *szWriteFields = NULL;
   EDFElement *pCurr = NULL;

   printf("HTMLFields entry %s\n", BoolStr(bMultiple));
   // EDFPrint(pData, false, true);

   bLoop = pData->Child();
   while(bLoop == true)
   {
      szValue = NULL;
      iDataType = 0;
      iFieldType = 0;

      pData->Get(&szName, &szValue);

      if(strcmp(szName, szType) == 0)
      {
         // EDFPrint(pData, EDFElement::PR_SPACE);

         HTMLField(pFields, pData, szType, szValue, false, true);
      }
      else if((strcmp(szType, "in") == 0 && strcmp(szName, "insection") == 0) || (strcmp(szType, "out") == 0 && strcmp(szName, "outsection") == 0))
      {
         iFieldType = 0;
         printf("HTMLFields section '%s'\n", szValue);

         if(pFields->Child(szName, szValue) == false)
         {
            pFields->GetChild("fieldtype", &iFieldType);
            if(mask(iFieldType, FIELD_IGNORE) == false)
            {
               HTMLField(pFields, pData, szName, szValue, bMultiple, false);
            }
            else
            {
               printf("HTMLFields ignore field\n");
               if(bMultiple == true)
               {
                  pFields->SetChild("multiple", true);
               }
            }

            HTMLFields(pData, pFields, szType, false, false, szClass);
         }
         else
         {
            pFields->SetChild("multiple", true);
         }

         if(mask(iFieldType, FIELD_IGNORE) == false)
         {
            pFields->Parent();
         }
      }
      else if(strcmp(szName, "call") == 0)
      {
         bMultipleTemp = pData->GetChildBool("multiple");
         bFlat = pData->GetChildBool("flat");
         printf("HTMLFields call '%s' %s\n", szValue, BoolStr(bMultipleTemp));

         pCurr = pData->GetCurr();

         pData->Root();
         if(pData->Child("function", szValue) == true)
         {
            HTMLFields(pData, pFields, szType, bMultipleTemp, bFlat, szClass);
         }
         else
         {
            printf("HTMLFields cannot find function %s\n", szValue);
         }

         pData->SetCurr(pCurr);
      }
      else if(strcmp(szName, "write") == 0)
      {
         printf("HTMLFields write '%s'\n", szValue);

         pCurr = pData->GetCurr();

         pData->Root();
         if(pData->Child("class", szClass != NULL ? szClass : szValue) == true)
         {
            if(szClass != NULL)
            {
               if(pData->Child("write", szValue) == true)
               {
                  HTMLFields(pData, pFields, szType, bMultiple, false, szClass);
                  pData->Parent();
               }
               else
               {
                  printf("HTMLFields cannot find method %s\n", szValue);
               }
            }
            else
            {
               if(pData->Child("write", "WriteFields") == true)
               {
                  if(bFlat == false)
                  {
                     pData->GetChild("writefields", &szWriteFields);
                     printf("HTMLFields write fields %s\n", szWriteFields);
                  }
                  if(szWriteFields != NULL)
                  {
                     if(pFields->Child("outsection", szWriteFields) == false)
                     {
                        pFields->Add("outsection", szWriteFields);
                     }
                  }

                  HTMLFields(pData, pFields, szType, bMultiple, false, szValue);
                  pData->Parent();

                  if(pData->Child("write", "WriteEDF") == true)
                  {
                     HTMLFields(pData, pFields, szType, bMultiple, false, szValue);
                     pData->Parent();
                  }
                  else
                  {
                     printf("HTMLFields cannot find method WriteEDF\n");
                  }
               }
               else
               {
                  printf("HTMLFields cannot find method WriteFields\n");
               }
            }
         }
         else
         {
            printf("HTMLFields cannot find class %s\n", szValue);
         }

         pData->SetCurr(pCurr);

         HTMLFields(pData, pFields, szType, bMultiple, false, szValue);
      }

      // delete[] szValue;
      // delete[] szName;

      bLoop = pData->Next();
      if(bLoop == false)
      {
         pData->Parent();
      }
   }

   if(szWriteFields != NULL)
   {
      pFields->Parent();
   }

   printf("HTMLFields exit true\n");
   return true;
}

void HTMLElementClose(FILE *pFile, int iIndent, const char *szName)
{
   STACKTRACE

   fprintf(pFile, "%-*s<span style=\"color: #0000ff\">&lt;</span>/%s<span style=\"color: #ff0000\">&gt;</span>\n", iIndent, "", szName != NULL ? szName : "");
}

void HTMLElementOpen(FILE *pFile, int iIndent, const char *szName, char *szValue, bool bSingleton)
{
   STACKTRACE

   fprintf(pFile, "%-*s<span style=\"color: #0000ff\">&lt;</span>%s", iIndent, "", szName != NULL ? szName : "");
   if(szValue != NULL)
   {
      fprintf(pFile, "=<span style=\"color: #007f7f\">%s</span>", szValue);
   }

   if(bSingleton == true)
   {
      fprintf(pFile, "/");
   }

   fprintf(pFile, "<span style=\"color: #0000ff\">&gt;</span>\n");
}

bool HTMLStructure(FILE *pFile, char *szMessage, EDF *pFields, char *szType, int iIndent, bool bMultiple)
{
   STACKTRACE
   int iDataType = 0;
   bool bValue = false, bLoop = true, bField = false;
   char *szName = NULL, *szValue = NULL, *szDataType = NULL;

   printf("HTMLStructure entry %s %d %s\n", szType, iIndent, BoolStr(bMultiple));
   // EDFPrint(pFields, false, false);

   if(iIndent == 0)
   {
      fprintf(pFile, "<pre>\n");
      // fprintf(pFile, "&lt;%s=\"%s\"&gt;\n", strcmp(szType, "in") == 0 ? "request" : "reply", szMessage);
      szValue = new char[strlen(szMessage) + 3];
      sprintf(szValue, "\"%s\"", szMessage);
      HTMLElementOpen(pFile, 0, strcmp(szType, "in") == 0 ? "request" : "reply", szValue, false);
      // delete[] szValue;
   }

   bLoop = pFields->Child();
   while(bLoop == true)
   {
      szValue = NULL;

      pFields->Get(&szName, &szValue);
      if(strcmp(szName, szType) == 0)
      {
         iDataType = 0;

         pFields->GetChild("datatype", &iDataType);
         if(iDataType == FIELD_STR || iDataType == FIELD_BYTES)
         {
            szDataType = "\"...\"";
         }
         else if(iDataType == FIELD_INT)
         {
            szDataType = "n";
         }
         else if(iDataType == FIELD_BOOL)
         {
            szDataType = "[0|1]";
         }
         else
         {
            printf("HTMLStructure no data type for field '%s'\n", szValue);
            szDataType = "";
         }
         // fprintf(pFile, "%-*s&lt;%s=%s/&gt;\n", iIndent + 2, "", szValue, szDataType);
         HTMLElementOpen(pFile, iIndent + 2, szValue, szDataType, true);

         bField = true;
      }
      else if((strcmp(szType, "in") == 0 && strcmp(szName, "insection") == 0) || (strcmp(szType, "out") == 0 && strcmp(szName, "outsection") == 0))
      {
         pFields->GetChild("datatype", &iDataType);
         bValue = pFields->GetChildBool("multiple");

         // fprintf(pFile, "%-*s&lt;%s&gt;\n", iIndent + 2, "", strlen(szValue) > 0 ? szValue : "...");
         HTMLElementOpen(pFile, iIndent + 2, strlen(szValue) > 0 ? szValue : "...", NULL, false);
         HTMLStructure(pFile, NULL, pFields, szType, iIndent + 2, bValue);
         // fprintf(pFile, "%-*s&lt;/%s&gt;\n", iIndent + 2, "", strlen(szValue) > 0 ? szValue : "...");
         HTMLElementClose(pFile, iIndent + 2, strlen(szValue) > 0 ? szValue : "...");
         if(bValue == true)
         {
            fprintf(pFile, "%-*s... repeat ...\n", iIndent + 2, "");
         }

         bField = true;
      }
      // delete[] szName;
      // delete[] szValue;

      bLoop = pFields->Next();
      if(bLoop == false)
      {
         pFields->Parent();
      }
   }

   if(iIndent == 0)
   {
      // fprintf(pFile, "&lt;/%s&gt;\n", strcmp(szType, "intput") == 0 ? "request" : "reply");
      HTMLElementClose(pFile, 0, strcmp(szType, "in") == 0 ? "request" : "reply");
      fprintf(pFile, "</pre>\n");
   }
   else
   {
      if(bField == false)
      {
         fprintf(pFile, "%-*s... fields ...\n", iIndent + 2, "");
      }
   }

   printf("HTMLStructure exit true\n");
   return true;
}

bool HTMLOutputGroup(EDF *pData, char *szGroup)
{
   STACKTRACE
   int iNumReplies = 0, iScope = 0;
   bool bLoop = false;
   char szFilename[100];
   char *szName = NULL, *szRequest = NULL, *szFunction = NULL, *szReply = NULL, *szComma = NULL, *szLookup = NULL, *szScope = NULL;
   char *szAdd = NULL, *szDep = NULL, *szUse = NULL;
   FILE *pFile = NULL;
   EDF *pFields = NULL;
   EDFElement *pRequest = NULL;

   sprintf(szFilename, "%s.html", szGroup);
   pFile = FileOpen(szFilename, "w");
   if(pFile == NULL)
   {
      printf("HTMLOutputGroup exit false, FileOpen failed\n");
      return false;
   }

   // fprintf(pFile, "<html>\n");
   // fprintf(pFile, "<title>Message Spec</title>\n");

   // fprintf(pFile, "<body>\n");

   // fprintf(pFile, "<h1>%c%s Request Group</h1>\n", toupper(szGroup[0]), szGroup + 1);

   fprintf(pFile, "<table>\n");

   bLoop = pData->Child();
   while(bLoop == true)
   {
      pData->Get(&szName, &szRequest);

      if(strstart(szRequest, szGroup) == true)
      {
         iScope = 0;

         printf("HTMLOutputGroup request %s\n", szRequest);
         EDFParser::Print(pData, EDFElement::EL_CURR | EDFElement::PR_SPACE);

         pData->GetChild("scope", &iScope);
         if(iScope == USEROFF)
         {
            szScope = "logged out";
         }
         else if(iScope == USERON)
         {
            szScope = "logged in";
         }
         else if(iScope == USERNORMAL)
         {
            szScope = "messages level";
         }
         else if(iScope == USERADMIN)
         {
            szScope = "admin only";
         }
         else
         {
            szScope = NULL;
         }

         fprintf(pFile, "<tr><td colspan=2>\n");
         fprintf(pFile, "<h2>%s%s%s%s</h2>\n", szRequest, szScope != NULL ? " (" : "", szScope != NULL ? szScope : "", szScope != NULL ? ")" : "");

         // HTMLPrint(pFile, pData, EDFElement::EL_CURR | EDFElement::PR_SPACE);

         if(pData->GetChild("dep", &szDep) == true)
         {
            fprintf(pFile, "Request deprecated in <b>%s</b>", szDep);
            if(pData->GetChild("use", &szUse) == true)
            {
               szLookup = DefineLookup(pData, szUse);
               fprintf(pFile, ", use <a href=\"\">%s</a>", szLookup);
               // delete[] szLookup;
               // delete[] szUse;
            }
            fprintf(pFile, "\n");
            // delete[] szDep;

            fprintf(pFile, "</td></tr>\n");
         }
         else
         {
            fprintf(pFile, "</td></tr>\n");

            if(pData->GetChild("add", &szAdd) == true)
            {
               fprintf(pFile, "<tr><td colspan=2>Added in <b>%s</b></td></tr>\n", szAdd);
               // delete[] szAdd;
            }

            if(strcmp(szName, "request") == 0)
            {
               pData->GetChild("function", &szFunction);

               pRequest = pData->GetCurr();

               pData->Root();
               if(pData->Child("function", szFunction) == true)
               {
                  EDFParser::Print("HTMLOutput function", pData, EDFElement::EL_CURR | EDFElement::PR_SPACE);
                  // HTMLPrint(pFile, pData, EDFElement::EL_CURR | EDFElement::PR_SPACE);

                  pFields = new EDF();

                  HTMLFields(pData, pFields, "in", false, false, NULL);
                  HTMLFields(pData, pFields, "out", false, false, NULL);

                  EDFParser::Print("HTMLOutput fields", pFields, EDFElement::PR_SPACE);

                  fprintf(pFile, "<tr>\n");
                  fprintf(pFile, "<td>\n");
                  HTMLStructure(pFile, szRequest, pFields, "in", 0, false);
                  fprintf(pFile, "</td>\n");
                  fprintf(pFile, "<td valign=\"top\">\n");
                  fprintf(pFile, "<table class=\"info\" border=1>\n");
                  fprintf(pFile, "<tr>\n");
                  fprintf(pFile, "<th>Field</th>\n");
                  fprintf(pFile, "<th>Type</th>\n");
                  fprintf(pFile, "<th>Required</th>\n");
                  fprintf(pFile, "<th>Admin</th>\n");
                  fprintf(pFile, "<th>Description</th>\n");
                  fprintf(pFile, "</tr>\n");
                  fprintf(pFile, "</table>\n");
                  fprintf(pFile, "</td>\n");
                  fprintf(pFile, "</tr>\n");

                  fprintf(pFile, "<tr>\n");
                  fprintf(pFile, "<td>\n");
                  HTMLStructure(pFile, szRequest, pFields, "out", 0, false);
                  fprintf(pFile, "</td>\n");
                  fprintf(pFile, "<td valign=\"top\">\n");
                  fprintf(pFile, "<table class=\"info\" border=1>\n");
                  fprintf(pFile, "<tr>\n");
                  fprintf(pFile, "<th>Field</th>\n");
                  fprintf(pFile, "<th>Type</th>\n");
                  fprintf(pFile, "<th>Admin</th>\n");
                  fprintf(pFile, "<th>Description</th>\n");
                  fprintf(pFile, "</tr>\n");
                  fprintf(pFile, "</table>\n");
                  fprintf(pFile, "</td>\n");
                  fprintf(pFile, "</tr>\n");

                  delete pFields;

                  iNumReplies = pData->Children("reply");
                  if(iNumReplies > 0)
                  {
                     fprintf(pFile, "<tr><td colspan=2>\n");
                     fprintf(pFile, "Other repl%s: ", iNumReplies != 1 ? "ies" : "y");

                     szComma = "";
                     bLoop = pData->Child("reply");
                     while(bLoop == true)
                     {
                        pData->Get(NULL, &szReply);
                        szLookup = DefineLookup(pData, szReply);

                        fprintf(pFile, "%s<a href=\"\">%s</a>", szComma, szLookup);

                        szComma = ", ";

                        // delete[] szLookup;
                        // delete[] szReply;

                        bLoop = pData->Next("reply");
                        if(bLoop == false)
                        {
                           pData->Parent();
                        }
                     }

                     fprintf(pFile, "\n");
                     fprintf(pFile, "</td></tr>\n");
                  }
               }
               else
               {
                  printf("HTMLOutputGroup no function section for %s (function %s)\n", szRequest, szFunction);
               }

               pData->SetCurr(pRequest);
            }
         }

         fprintf(pFile, "\n\n");

         // delete[] szFunction;
      }

      // delete[] szRequest;
      // delete[] szName;

      bLoop = pData->Next("request");
      if(bLoop == false)
      {
         pData->Parent();
      }
   }

   fprintf(pFile, "</table>\n");

   fprintf(pFile, "<p>Back to <a href=\"index.jsp\">message spec</a></p>\n");

   // fprintf(pFile, "</body>\n");
   // fprintf(pFile, "</html>\n");

   fclose(pFile);

   return true;
}

int main(int argc, char **argv)
{
   char szConfig[100];
   EDF *pData = NULL;

#ifdef UNIX
   for(int i = SIGHUP; i < SIGUNUSED; i++)
   {
      if(i != SIGKILL && i != SIGSTOP)
      {
         signal(i, Panic);
      }
   }
#else
   signal(SIGINT, Panic);
   signal(SIGILL, Panic);
   signal(SIGSEGV, Panic);
#endif

   if(argc != 2)
   {
      printf("Usage: MessageSpec <base>\n");
      return 1;
   }

   sprintf(szConfig, "%s/bin/ua.edf", argv[1]);

   pData = EDFParser::FromFile(szConfig);
   if(pData == NULL)
   {
      printf("Unable to open %s, %s\n", szConfig, strerror(errno));
      return 1;
   }

   DefineHeader(pData, argv[1]);

   ServerSource(pData, argv[1]);

   CommonHeader(pData, argv[1], "UserItem.h", "UserItem");
   CommonSource(pData, argv[1], "UserItem.cpp", "UserItem", "user");
   ServerSource(pData, argv[1], "RqUser.cpp", "UserItem", "User", UserLine);

   ServerSource(pData, argv[1], "RqSystem.cpp", NULL, NULL, NULL);

   pData->Root();

   EDFParser::ToFile(pData, "MessageSpec.edf");

   HTMLOutputGroup(pData, "bulletin");
   HTMLOutputGroup(pData, "channel");
   HTMLOutputGroup(pData, "connection");
   HTMLOutputGroup(pData, "content");
   HTMLOutputGroup(pData, "folder");
   HTMLOutputGroup(pData, "help");
   HTMLOutputGroup(pData, "message");
   HTMLOutputGroup(pData, "location");
   HTMLOutputGroup(pData, "service");
   HTMLOutputGroup(pData, "system");
   HTMLOutputGroup(pData, "task");
   HTMLOutputGroup(pData, "user");

   // delete pData;

   return 0;
}
