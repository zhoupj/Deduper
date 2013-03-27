#include "global.h"

/* avoid the problem of multi definition */
char backup_cmd[] = "BACK fileset=%s";
char backup_msg[]="BMSG %s"; /* 报告备份成功与否*/

char restore_cmd[] = "REST jobid=%d";
char restore_msg[]="RMSG %s"; /* 报告恢复成功与否*/

char delete_cmd[] = "DELE jobid=%d";
char delete_msg[]="DMSG %s";

char BackupVolPath[]="/home/zpj/deduper/working/";

 int G_PIPELINE=0;
 //int  MAX_FINERS_NUM=256;
 int  MAX_FINERS_NUM=2*1024*1024;// 2M
 int G_NOT_DEDUP=0;
int G_VERBOSE=0;
 
 static char buf[1024];
 int err_msg (char *filename,int line,const char * fmt, ...)
 {
  va_list args;
  int n=0;
  printf("\033[40;31m %s,%d Error:",filename,line);
  va_start(args, fmt);
  n=vsprintf(buf, fmt, args);
  buf[n]='\0';
  printf(buf);
  va_end(args);
  printf("\033[0m \n");
  return n;
 }
 
 
