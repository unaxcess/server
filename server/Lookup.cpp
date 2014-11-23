#include <stdio.h>

#include "headers.h"
#include "server.h"

bool LocationFix(EDF *pData);

int main(int argc, char **argv)
{
   bool bFix = false;
   unsigned long lAddress = 0;
   char *szHostname = NULL, *szLocation = NULL;
   EDF *pData = NULL;

   if(argc != 3)
   {
      printf("Usage: Lookup <data> <lookup>\n");
      return 1;
   }

   debuglevel(DEBUGLEVEL_DEBUG);

   pData = EDFParser::FromFile(argv[1]);

   if(pData == NULL)
   {
      printf("Failed to read %s\n", argv[1]);

      return 1;
   }

   if(bFix == true)
   {
      pData->Child("locations");
      LocationFix(pData);
      EDFParser::ToFile(pData, argv[1]);
      pData->Root();
   }

   if(Conn::StringToAddress(argv[2], &lAddress, false) == false)
   {
      szHostname = argv[2];
   }

   szLocation = ConnectionLocation(pData, szHostname, lAddress);
   printf("%s -> %s\n", argv[2], szLocation);

   delete pData;

   return 0;
}
