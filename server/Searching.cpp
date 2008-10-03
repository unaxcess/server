#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>

#include "headers.h"

#define FM_ARCHIVE_TABLE "folder_message_archive"

char *MessageSQL(EDF *pEDF);

EDF *QueryToEDF(char *szQuery)
{
   int iValue = 0;
   bool bLiteral = false, bNot = false;
   char cQuote = '\0';
   char *szPos = szQuery, *szStart = NULL, *szToken = NULL, *szChr = NULL, *szCommand = NULL, *szValue = NULL, *szName = NULL;
   EDF *pEDF = NULL, *pReturn = NULL;
   EDFElement *pElement = NULL, *pDelete = NULL;

   printf("QueryToEDF entry '%s'\n", szQuery);

   pEDF = new EDF();
   pEDF->Set("and");

   while(*szPos != '\0' && (*szPos == ' ' || *szPos == '\t' || *szPos == '\n'))
   {
      szPos++;
   }

   while(*szPos != '\0')
   {
      printf("QueryToEDF loop %d %c\n", szPos - szQuery, *szPos);

      if(*szPos == '(')
      {
         printf("QueryToEDF open bracket\n");

         if(bNot == true)
         {
            pEDF->Add("not");
            bNot = false;
         }

         pEDF->Add("and");

         szPos++;
      }
      else if(*szPos == ')')
      {
         EDFParser::Print("QueryToEDF close bracket", pEDF, EDFElement::EL_CURR | EDFElement::PR_SPACE);

         if(pEDF->Children() == 0)
         {
            pEDF->Delete();
         }
         else if(pEDF->Children() == 1)
         {
            EDFParser::Print("QueryToEDF pre delete", pEDF, EDFElement::EL_CURR | EDFElement::PR_SPACE);

            pEDF->Child();
            pElement = pEDF->GetCurr();
            pEDF->Parent();
            pDelete = pEDF->GetCurr();
            pEDF->Parent();
            pEDF->MoveFrom(pElement);
            pEDF->SetCurr(pDelete);
            pEDF->Delete();

            EDFParser::Print("QueryToEDF post delete", pEDF, EDFElement::EL_CURR | EDFElement::PR_SPACE);
         }
         else
         {
            pEDF->Parent();
         }

         pEDF->Get(&szName);
         if(strcmp(szName, "not") == 0)
         {
            pEDF->Parent();
         }
         delete[] szName;

         szPos++;
      }
      else
      {
         if(*szPos == '"' || *szPos == '\'')
         {
            cQuote = *szPos;
            szPos++;
         }
         else
         {
            cQuote = '\0';
         }

         szStart = szPos;
         if(cQuote != '\0')
         {
            bLiteral = false;

            // while(*szPos != '\0' && !(bLiteral == false && *szPos == cQuote));
            while(*szPos != '\0' && *szPos != cQuote)
            {
               /* if(*szPos == '\\')
               {
                  bLiteral = true;
               }
               else
               {
                  bLiteral = false;
               } */

               szPos++;
            }
         }
         else
         {
            while(*szPos != '\0' && isspace(*szPos) == 0 && *szPos != '(' && *szPos != ')')
            {
               szPos++;
            }
         }

         szToken = strmk(szQuery, szStart - szQuery, szPos - szQuery);
         // printf("%-*sQueryToEDF token '%s'\n", iIndent, "", szToken);

         if(cQuote != '\0')
         {
            szPos++;
         }

         szChr = strchr(szToken, ':');
         if(szChr != NULL)
         {
            szCommand = strmk(szToken, 0, szChr - szToken);
            szValue = strmk(szChr + 1);
            iValue = atoi(szValue);
            printf("QueryToEDF command %s: '%s' / %d\n", szCommand, szValue, iValue);

            if(bNot == true)
            {
               pEDF->Add("not");
            }

            if(stricmp(szCommand, "folder") == 0 || stricmp(szCommand, "folderid") == 0)
            {
               pEDF->AddChild("folderid", iValue);
            }
            else if(stricmp(szCommand, "from") == 0 || stricmp(szCommand, "fromid") == 0)
            {
               pEDF->AddChild("fromid", iValue);
            }
            else if(stricmp(szCommand, "to") == 0 || stricmp(szCommand, "toid") == 0)
            {
               pEDF->AddChild("toid", iValue);
            }
            else if(stricmp(szCommand, "user") == 0 || stricmp(szCommand, "userid") == 0)
            {
               pEDF->Add("or");
               pEDF->AddChild("fromid", iValue);
               pEDF->AddChild("toid", iValue);
               pEDF->Parent();
            }

            if(bNot == true)
            {
               pEDF->Parent();

               bNot = false;
            }

            delete[] szCommand;
            delete[] szValue;
         }
         else
         {
            if(stricmp(szToken, "and") == 0 || stricmp(szToken, "or") == 0)
            {
               printf("QueryToEDF logical %s\n", szToken);

               pEDF->Set(szToken);
            }
            else if(stricmp(szToken, "not") == 0)
            {
               bNot = true;
            }
            else
            {
               printf("QueryToEDF keyword '%s'\n", szToken);
               pEDF->AddChild("keyword", szToken);
            }
         }

         delete[] szToken;
      }

      if(isspace(*szPos) != 0)
      {
         do
         {
            szPos++;
         }
         while(*szPos != '\0' && isspace(*szPos) != 0);
      }
   }

   pReturn = new EDF();

   pReturn->Copy(pEDF);

   delete pEDF;

   EDFParser::Print("QueryToEDF exit", pReturn);
   return pReturn;
}

int main(int argc, char **argv)
{
   int iDebugLevel = 0, iNumMsgs = 0;
   long lMessageID = -1, lParentID = -1, lFolderID = -1, lDate = 0, lFromID = -1, lToID = -1;
   char *szSQL = NULL;
   bytes *pText = NULL, *pSubject = NULL, *pEDF = NULL;
   EDF *pSearch = NULL;
   DBTable *pTable = NULL;
   double dTick = 0;

   if(argc != 3 || !(strcmp(argv[1], "-edf") == 0 || strcmp(argv[1], "-query") == 0))
   {
      printf("Usage: Searching -edf <file>\n");
      printf("Usage: Searching -query <string>\n");

      return 1;
   }

   if(strcmp(argv[1], "-edf") == 0)
   {
      pSearch = EDFParser::FromFile(argv[2]);
      if(pSearch == NULL)
      {
         printf("Could not parse %s, %s\n", argv[2], strerror(errno));
         return 1;
      }

      EDFParser::Print("Search", pSearch);
   }
   else
   {
      pSearch = QueryToEDF(argv[2]);

      printf("Query %s\n", argv[2]);
      EDFParser::Print("Search", pSearch);

      // return 0;
   }
   printf("\n");

   dTick = gettick();
   szSQL = MessageSQL(pSearch);
   printf("SQL(%ld ms): %s\n", tickdiff(dTick), szSQL);
   if(szSQL == NULL)
   {
      printf("Nothing to search for\n");

      return 1;
   }

   if(DBTable::Connect("ua27") == false)
   {
      printf("Cannot connect to database, %s\n", strerror(errno));

      delete pSearch;

      return 1;
   }

   pTable = new DBTable();

   printf("Binding columns\n");

   // messageid
   pTable->BindColumnInt();

   // parentid
   pTable->BindColumnInt();

   // treeid, date, fromid, toid, text
   pTable->BindColumnInt();
   pTable->BindColumnInt();
   pTable->BindColumnInt();
   pTable->BindColumnInt();
   pTable->BindColumnBytes();

   // subject
   pTable->BindColumnBytes();

   // edf
   pTable->BindColumnBytes();

   dTick = gettick();
   printf("Running query\n");
   if(pTable->Execute(szSQL) == true)
   {
      printf("Execute(%ld ms)\n", tickdiff(dTick));

      // iDebugLevel = debuglevel(DEBUGLEVEL_DEBUG);

      dTick = gettick();

      while(pTable->NextRow() == true)
      {
         iNumMsgs++;

         lMessageID = -1;

         lParentID = -1;

         lFolderID = -1;
         lDate = 0;
         lFromID = -1;
         lToID = -1;
         pText = NULL;

         pSubject = NULL;

         pEDF = NULL;

         // messageid
         pTable->GetField(0, &lMessageID);

         // parentid
         pTable->GetField(1, &lParentID);

         // treeid, date, fromid, toid, text
         pTable->GetField(2, &lFolderID);
         pTable->GetField(3, &lDate);
         pTable->GetField(4, &lFromID);
         pTable->GetField(5, &lToID);
         pTable->GetField(6, &pText);

         // subject
         pTable->GetField(7, &pSubject);

         // edf
         pTable->GetField(7, &pEDF);

         printf("m=%ld p=%ld f=%ld d=%ld %ld %ld", lMessageID, lParentID, lFolderID, lDate, lFromID, lToID);
         if(pSubject != NULL)
         {
            printf(", %s", (char *)pSubject->Data(false));
         }
         if(pText != NULL)
         {
            printf(":\n%s", (char *)pText->Data(false));
         }
         printf("\n");

         // delete pEDF;
         // delete pSubject;
         // delete pText;
      }

      // debuglevel(iDebugLevel);

      printf("Found %d messages(%ld ms)\n", iNumMsgs, tickdiff(dTick));
   }
   else
   {
      printf("Query failed, %s\n", strerror(errno));
   }

   delete pTable;

   DBTable::Disconnect();

   delete[] szSQL;

   delete pSearch;

   return 0;
}
