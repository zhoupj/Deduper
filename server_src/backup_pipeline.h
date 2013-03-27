#ifndef BACKUP_PIPELINE_H
#define BACKUP_PIPELINE_H_

void *pipeline_backup(int fd_finger,int fd_data,char *msg);
void * pipeline_recv_data(void * arg);
void * pipeline_write_data(void *arg);

#endif

