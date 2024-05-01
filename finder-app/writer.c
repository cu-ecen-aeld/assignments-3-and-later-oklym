#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>


void printSysLog(int level, char* msg);

int main (int argc, char* argv[])
{
  char msg[256];

  if(argc != 3) {
    printSysLog(LOG_ERR, "Error: you should provide exact two mandatory parameters: writefile and writestr\n");
    return 1;
  }

  FILE *fptr;
  fptr = fopen(argv[1], "w");
  if(fptr == NULL) {
    sprintf(msg, "Error: failed to open %s\n", argv[1]); 
    printSysLog(LOG_ERR, msg);  
    return(1);
  }

  sprintf(msg, "Writing %s to %s\n", argv[2], argv[1]);
  printSysLog(LOG_DEBUG, msg);
  fprintf(fptr, "%s", argv[2]);
  fclose(fptr);

  return 0;
}


void printSysLog(int level, char* msg) {
  // printf("%s", msg);
  openlog("assignment-2-oklym", LOG_PID, LOG_USER);
  syslog(level, "%s", msg);
  closelog();
}
