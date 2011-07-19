#include <stdio.h>
#include <ctype.h>

#include "headers.h"
#include "server/server.h"

int main(int argc, char **argv) {
  if(argc != 3) {
    printf("Usage: Location <host/IP> <data file>\n");
    return 1;
  }
  
  debuglevel(DEBUGLEVEL_DEBUG);

  EDF *pData = EDFParser::FromFile(argv[2]);
  printf("EDF %p\n", pData);

  char *szHostname = NULL;
  unsigned long lAddr = 0;

  printf("Checking %s as hostname or IP\n", argv[1]);
  if(isdigit(argv[1][0])) {
    Conn::CIDRToRange(argv[1], &lAddr, &lAddr);
  } else {
    szHostname = argv[1];
  }

  printf("Checking %s and %ul lookups\n", szHostname, lAddr);
  char *szLocation = ConnectionLocation(pData, szHostname, lAddr);
  printf("Location of %s is %s\n", argv[1], szLocation);

  return 0;
}

