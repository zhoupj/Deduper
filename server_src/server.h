#ifndef SERVER_H_
#define SERVER_H_
void *wait_conncet(void * arg);
void* test_data(void *arg);
void usage();

void make_dir();
void *handle_client_request(void * data);
int  bnet_thread_server(workq_t *client_wq,int max_client_count,int port,void *handle_client_request(void * data)) ;

#endif
