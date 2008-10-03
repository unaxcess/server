#include <stdio.h>
#include <stdlib.h>

#include "useful/useful.h"

int main(int argc, char **argv)
{
   int iDate = 0;
   double dTick = 0;
   DBTable *pTable = NULL;

   if(DBTable::Conntect("ua27dev", "ua-admin", "fuck77pig") == false)
   {
      printf("Unable to connect to database\n");
      return 1;
   }

   pTable = new DBTable();

   pTable->BindColumnInt();

   dTick = gettick();
   if(pTable->Execute("select message_date from folder_message_item order by message_date") == false)
   {
      printf("Cannot execute query\n");

      delete pTable;

      DBTable::Disconnect();

      return 1;
   }
   printf("Query ran in %ld ms\n", tickdiff(dTick));

   dTick = gettick();
   while(pTable->NextRow() == true)
   {
      pTable->GetField(0, &iDate);
   }
   printf("Results processed in %ld ms\n", tickdiff(dTick));

   delete pTable;

   DBTable::Disconnect();

   return 0;
}