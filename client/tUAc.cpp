#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "UAClient.h"

class tUAc : public UAClient
{
   char *CLIENT_NAME()
   {
      return"tUAc";
   }

   bool loggedIn()
   {
      m_pData->SetChild("quit", true);
      return true;
   }
};

int main(int argc, char** argv)
{
   tUAc *pClient = NULL;

   pClient = new tUAc();

   pClient->run(argc, argv);

   delete pClient;
}
