/*
 * global.h
 *
 *  Created on: Dec 5, 2012
 *      Author: Zhou Pengju
 */

#ifndef GLOBAL_H_
#define GLOBAL_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <dirent.h>
#include <syslog.h>
#include <signal.h>
#include <alloca.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <math.h>
#include <linux/sem.h>
#include <netinet/tcp.h> 
#include <openssl/sha.h>


#define bool int
#define false 0
#define true 1
#define FALSE 0
#define TRUE 1
#define SUCCESS 1
#define FAILURE 0
#define ERROR -100

#define FILE_NAME 1
#define FILE_FINGERPRINT 2
#define FILE_DATA 3


#define SOCKET_BUF_SIZE  64*1024  /*socket */


extern char backup_cmd[] ;
extern char backup_msg[];

extern char restore_cmd[];
extern char restore_msg[];

extern char delete_cmd[] ;
extern char delete_msg[];



#define NAME_END -2

#define FINGER_END -3
#define FINGER_RESP_END -4

#define DATA_END -5
#define STREAM_END -6


#define SYSTEM_QUIT -6

extern char BackupVolPath[];
extern int G_PIPELINE;
extern int  MAX_FINERS_NUM;
extern  int G_NOT_DEDUP;
extern int G_VERBOSE;


/* compute the total time and average time*/
#define TIMER_DECLARE(start,end) struct timeval start,end
#define TIMER_START(start) gettimeofday(&start, NULL)
#define TIMER_END(end) gettimeofday(&end, NULL)
#define TIMER_DIFF(diff,start,end) (diff)+=(end.tv_usec-start.tv_usec)*1.0/1000000+(end.tv_sec-start.tv_sec)
#define TIMER_AVG(avg,total,count) avg=(total)*1.0/count

#define P(mutex) pthread_mutex_lock(&mutex)
#define V(mutex) pthread_mutex_unlock(&mutex)


#include "libs/bnet.h"
#include "libs/htable.h"
#include "libs/threadq.h"
#include "libs/queue.h"
#include "libs/rabin.h"
#include "libs/libz.h"
#include "libs/pipeline.h"

#include "jcr.h"
#include "recipe.h"
#include "backup.h"
#include "backup_pipeline.h"

#define err_msg1(s)   err_msg(__FILE__,__LINE__,s)
#define err_msg2(s1,s2)  err_msg(__FILE__,__LINE__,s1,s2)
#define err_msg3(s1,s2,s3)  err_msg(__FILE__,__LINE__,s1,s2,s3)
#define err_msg4(s1,s2,s3,s4)  err_msg(__FILE__,__LINE__,s1,s2,s3,s4)

int err_msg (char *filename,int line,const char * fmt, ...);


#endif 
/* GLOBAL_H_ */
