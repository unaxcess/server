#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "useful/useful.h"

int main(int argc, char **argv)
{
   int iArgNum = 2, iProcess = 0, iMove = 0;
   bool bComment = false;
   char *szVB = NULL, *szName = NULL, *szValue = NULL, *szChr = NULL;
   FILE *fInput = NULL, *fVB = NULL;
   char szLine[1000], szComment[5000];

   if(argc < 2)
   {
      printf("Usage: SpecConv <input> [-vb <output>]\n");
      return 1;
   }

   while(iArgNum < argc)
   {
      if(iArgNum < argc - 1 && strcmp(argv[iArgNum], "-vb") == 0)
      {
         szVB = argv[++iArgNum];
      }
      else
      {
         printf("Unrecognised option %s\n", argv[iArgNum]);
      }

      iArgNum++;
   }

   fInput = fopen(argv[1], "r");
   if(fInput == NULL)
   {
      printf("Unable to open %s, %s\n", argv[1], strerror(errno));
      return 1;
   }

   if(szVB != NULL)
   {
      fVB = fopen(szVB, "w");
      if(fVB == NULL)
      {
         fclose(fInput);

         printf("Unable to open %s\n, %s\n", szVB, strerror(errno));
         return 1;
      }

      fprintf(fVB, "Public Class ua\n");
   }

   szComment[0] = '\0';

   while(iProcess < 4 && fgets(szLine, sizeof(szLine), fInput) != NULL)
   {
      while(strlen(szLine) >= 1 && (szLine[strlen(szLine) - 1] == '\r' || szLine[strlen(szLine) - 1] == '\n'))
      {
         szLine[strlen(szLine) - 1] = '\0';
      }

      // printf("Process %d: %s\n", iProcess, szLine);

      if(iProcess == 0 && strcmp(szLine, "/*") == 0)
      {
         iProcess = 1;
      }
      else if(iProcess == 1)
      {
         if(strcmp(szLine, "*/") == 0)
         {
            iProcess = 2;
         }
         else
         {
            printf("Comment %s\n", szLine);

            if(strncmp(szLine, "**", 2) == 0)
            {
               iMove = 3;
            }
            else
            {
               iMove = 0;
            }

            if(strlen(szComment) > 0)
            {
               strcat(szComment, "\n");
            }
            if(strlen(szLine) >= iMove && strncmp(szLine + iMove, "ua.h", 4) != 0)
            {
               strcat(szComment, szLine + iMove);
            }
         }
      }
      else if(iProcess == 3)
      {
         if(strcmp(szLine, "// MessageSpec stop") == 0)
         {
            printf("SpecConv OFF\n");

            iProcess = 4;
         }
         else
         {
            if(bComment == true || strncmp(szLine, "// ", 3) == 0)
            {
               if(bComment == true && strcmp(szLine, "*/") == 0)
               {
                  printf("Spec comment OFF\n");
                  bComment = false;
               }
               else
               {
                  printf("Spec comment %s\n", szLine);

                  if(strncmp(szLine, "**", 2) == 0 || strncmp(szLine, "//", 2) == 0)
                  {
                     iMove = 3;
                  }
                  else
                  {
                     iMove = 0;
                  }

                  if(fVB != NULL)
                  {
                     fprintf(fVB, "\t'%s\n", szLine + iMove);
                  }
               }
            }
            else if(strncmp(szLine, "/*", 2) == 0)
            {
               printf("Spec comment ON\n");
               bComment = true;
            }
            else if(strncmp(szLine, "#define ", 8) == 0)
            {
               szChr = strchr(szLine + 8, ' ');

               if(szChr != NULL)
               {
                  printf("Spec define %s\n", szLine + 8);

                  if(fVB != NULL)
                  {
                     szName = strmk(szLine, 8, szChr - szLine);
                     fprintf(fVB, "\tPublic Const %s = ", szName);

                     szValue = strmk(szChr + 1);
                     szChr = strstr(szValue, "//");
                     if(szChr != NULL)
                     {
                        szValue[szChr - szValue - 1] = '\0';
                        if(strlen(szValue) >= 1 && szValue[strlen(szValue) - 1] == ' ')
                        {
                           szValue[strlen(szValue) - 1] = '\0';
                        }
                        fprintf(fVB, "%s '%s", szValue, szChr + 3);
                     }
                     else
                     {
                        fprintf(fVB, szValue);
                     }

                     fprintf(fVB, "\n");

                     delete[] szName;
                     delete[] szValue;
                  }
               }
               else
               {
                  printf("Spec define error %s\n", szLine + 8);
               }
            }
            else if(strcmp(szLine, "") == 0)
            {
               printf("Spec blank\n");

               if(fVB != NULL)
               {
                  fprintf(fVB, "\n");
               }
            }
            else
            {
               printf("Spec other %s\n", szLine);
            }
         }
      }
      else
      {
         // printf("Other line: %s\n", szLine);
      }

      if(strcmp(szLine, "// MessageSpec start") == 0)
      {
         printf("SpecConv ON\n");

         while(strlen(szComment) >= 1 && (szComment[strlen(szComment) - 1] == '\r' || szComment[strlen(szComment) - 1] == '\n'))
         {
            szComment[strlen(szComment) - 1] = '\0';
         }
         printf("%s\n", szComment);

         iProcess = 3;
      }
   }

   fclose(fInput);

   if(fVB != NULL)
   {
      fprintf(fVB, "End Class\n");

      fclose(fVB);
   }

   return 0;
}