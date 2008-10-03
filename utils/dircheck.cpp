#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char **argv)
{
   bool bLoop = true;
   DIR *pDir = NULL;
   struct dirent *pEntry = NULL;

   if(argc != 2)
   {
      printf("Usage: dircheck <directory>\n");
      return 1;
   }

   while(bLoop == true)
   {
      pDir = opendir(argv[1]);

      if(pDir != NULL)
      {
         while((pEntry = readdir(pDir)) != NULL)
         {
            if(strcmp(pEntry->d_name, ".") != 0 && strcmp(pEntry->d_name, "..") != 0)
            {
               printf("%s\n", pEntry->d_name);
            }
         }

         closedir(pDir);

         sleep(5);
      }
      else
      {
         bLoop = false;
      }
   }

   return 0;
}
