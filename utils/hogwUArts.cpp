/*
** A very silly program which puts everyone in a Hogwarts house
*/

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "client/UAClient.h"

class hogwUArts : public UAClient
{
   char *CLIENT_NAME()
   {
      return"hogwUArts";
   }

   bool loggedIn()
   {
      int iUserID = 0;
      bool bLoop = false;
      char *szName = NULL, *szAccessName = NULL;
      EDF *pUserList = NULL, *pUserEdit = NULL, *pUserReply = NULL;
      
      if(request(MSG_USER_LIST, &pUserList) == true)
	{
	   pUserList->Sort("user", "name");
	   
	   bLoop = pUserList->Child("user");
	   while(bLoop == true)
	     {
		pUserList->Get(NULL, &iUserID);
		pUserList->GetChild("name", &szName);
		
		// printf("hogwUArts::loggedIn user ID %d\n", iUserID);
		
		switch(iUserID % 4)
		  {
		   case 0:
		     szAccessName = "Gryffindor";
		     break;
		     
		   case 1:
		     szAccessName = "Hufflepuff";
		     break;
		     
		   case 2:
		     szAccessName = "Ravenclaw";
		     break;
		     
		   case 3:
		     szAccessName = "Slytherin";
		     break;
		  }
		
		printf("hogwUArts::loggedIn putting %s(%d) in the %s house\n", szName, iUserID, szAccessName);
		
		// if(iUserID == 2)
		  {
		     pUserEdit = new EDF();
		     pUserEdit->AddChild("userid", iUserID);
		     // pUserEdit->AddChild("accessname", szAccessName);
		     pUserEdit->AddChild("accessname", (char *)NULL);;
		     if(request(MSG_USER_EDIT, pUserEdit, &pUserReply) == true)
		       {
			  printf("hogwUArts::loggedIn access name changed\n");
		       }
		     else
		       {
			  EDFParser::Print("hogwUArts::loggedIn user edit failed", pUserReply);
		       }
		
		     delete pUserEdit;
		     delete pUserReply;
		  }
		
		delete[] szName;
		
		bLoop = pUserList->Next("user");
		if(bLoop == false)
		  {
		     pUserList->Parent();
		  }
		
	     }
	   
	   delete pUserList;
	}
      
      
      m_pData->SetChild("quit", true);
      return true;
   }
};

int main(int argc, char** argv)
{
   hogwUArts *pClient = NULL;
   
   debuglevel(DEBUGLEVEL_INFO);

   pClient = new hogwUArts();

   pClient->run(argc, argv);

   delete pClient;
}
