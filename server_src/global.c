#include "global.h"

/* avoid the problem of multi definition */
char backup_cmd[] = "BACK fileset=%s";
char backup_msg[]="BMSG %s"; /* 报告备份成功与否*/

char restore_cmd[] = "REST jobid=%d";
char restore_msg[]="RMSG %s"; /* 报告恢复成功与否*/

char delete_cmd[] = "DELE jobid=%d";
char delete_msg[]="DMSG %s";

char BackupVolPath[100]="/home/zpj/deduper/working/";
int G_PIPELINE=0;
//int MAX_FINGERS_NUM =512;
int MAX_FINGERS_NUM =2*1024*1024;
int SCAN_REGION_SIZE=20*1024*1024;

int G_VERBOSE=0;
int G_CONTAINER_NUM=10; // for restore
int64_t DEFAULT_CONTAINER_SIZE =4*1024*1024;
double G_THRESHOLD=0.05;
int G_DEFRAGMENT=0;
double G_total_size=0;
int G_R_READNUM=0;
int G_CAPPING_T=5;
int G_FILE_FD=0;

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


