#ifndef BACKUP_PIPELINE_H_
#define BACKUP_PIPELINE_H_

typedef struct chunk{
	uint8_t data[64*1024+1];
	int datalen;
}chunk_t;

int backup_pipeline(int data_socket, int finger_socket,char *path);
int  pipeline_walk_dir (JCR *psJcr, char *path);
void pipeline_send_file(JCR *jcr,char *path);
void * data_module(void *arg);
void * filter_module(void *arg);
void * sha_module(void *arg);
void* chunk_module(void * arg);





#endif
