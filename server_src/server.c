#include "global.h"

#define SERVER_PORT 8888
#define SERVER_OTH_PORT 8889
#define BUF_DEFAULT_SIZE 64*1024
workq_t g_client_wq;
int g_client_count=20;

int  data_fd=-1;
#define chunk_size 64*1024
int G_TEST_DATA=0;
int G_NOT_DEDUP=0; // not deduplication (orignal)
int G_Sanitization=0;

int main(int argc, char **argv){
	int opt;
	int len;
	pthread_t data_socket;
	int id=0;
	int Restore_jobid=0;
	char out_file[50]={0};
         while((opt=getopt(argc, argv, "ps:S:tvhoc:C:f:T:P:RDd:a:r:O:"))!=-1){
	        switch(opt){
		   case 'p':  // 流水线
		  	G_PIPELINE=1;
			break;
		 case 'P':
			len=strlen(optarg);
		 	memcpy(BackupVolPath,optarg,len+1);
			if(BackupVolPath[len-1]!='/'){
				BackupVolPath[len]='/';
				BackupVolPath[len+1]=0;
			}
			break;
		case 'O':
		 	memcpy(out_file,optarg,len+1);
			printf("%s \n",out_file);
			if((G_FILE_FD= open(out_file, O_RDWR | O_CREAT |O_APPEND, S_IREAD|S_IWRITE))<0){
			        	printf("%s, %d, Can not create %s !\n",__FILE__,__LINE__,out_file);
			        return false;
			}
		       break;	
		case 'a': 
	   		G_CAPPING_T=atoi(optarg);
			break;
		 case 's': 
	   		MAX_FINGERS_NUM=atoi(optarg)*1024*1024;
			break;
		case 'S': 
	   		SCAN_REGION_SIZE=atoi(optarg)*1024*1024;
			break;
		case 'c': 
			 G_CONTAINER_NUM=atoi(optarg);
			break;
		case 'f':
			G_DEFRAGMENT=atoi(optarg);
			break;
		case 'r':
			Restore_jobid=atoi(optarg);
			break;
		case 'R':
			test_read_container();
			break;
		case 'C':  // MB
			 DEFAULT_CONTAINER_SIZE=(int64_t)atoi(optarg)*1024*1024;
			break;	
		case 'D':
			G_Sanitization=1;
			break;
		case 'd':
			id=atoi(optarg);
			break;
		case 't':
			G_TEST_DATA=1;
			break;
		case 'T':
			G_THRESHOLD=(double)atoi(optarg)/100.0;
			printf("Threshold=%f \n",G_THRESHOLD);
			break;
		case 'v':
			G_VERBOSE=1;
			break;
		case 'o':
			G_NOT_DEDUP=1;
			break;
		case 'h':
			usage();
			return 0;
		   default:
		   	printf("Your command is wrong \n");
			usage();
			return 0;
	        }
         }
	printf("==========parameters========== :\n");
	printf("Operation Path=%s\n",BackupVolPath);
	printf("deduption ?%s \n",G_NOT_DEDUP==1? "no":"yes");
         printf("pipeline is true ? %s \n", G_PIPELINE==1? "yes":"no");
	printf("transmittiong size:%d \n", MAX_FINGERS_NUM);
	printf("scan size:%d \n",SCAN_REGION_SIZE);
	printf("caching container num when restore: %d\n", G_CONTAINER_NUM);
	printf("container size:%d \n",DEFAULT_CONTAINER_SIZE);
	printf("defrag method: id %d \n",G_DEFRAGMENT);
	printf("capping numbers %d per %d MB \n",G_CAPPING_T,SCAN_REGION_SIZE);
		 
		 
	if(G_Sanitization) {// 删除整理
		sanitization();
		return 0;
	
	}
	if(Restore_jobid){
		local_simulate_restore(Restore_jobid);
		return 0;
	}
	if(id){
		local_delete_job(id);
		return 0;
	}
	if(G_TEST_DATA){
		bnet_thread_server(&g_client_wq,g_client_count,SERVER_PORT,test_data);
		return 0;
	}
	make_dir();
	if(G_PIPELINE)
		pthread_create(&data_socket, NULL, wait_conncet, NULL);
	bnet_thread_server(&g_client_wq,g_client_count,SERVER_PORT,handle_client_request);

	if(G_PIPELINE)
		pthread_join(data_socket,NULL);
	return 0;
}

void usage(){
	printf("\n===========usage============\n");
	printf("-p             # pipeline \n");
	printf("-s number      # recv fingerprints number per transmittion \n");
	printf("-v             #display filenames sent or recieved \n");
	printf("-t             # test network bandwidth \n");
	printf("-h             # give this help list \n");
}

void* test_data(void *arg){
	TIMER_DECLARE(start,end);
	char  buf[chunk_size+1];
	int len=0;
	double total_time=0;
	double total_len=0;
	int  fd=-1;
	int socket=*(int *)arg;
	if((fd = open("./test_bandwidth", O_CREAT | O_TRUNC | O_WRONLY,00644))<0){
		 printf("%s,%d create  file error!\n",__FILE__,__LINE__);
		return NULL;
	 }
	TIMER_START(start);
	while(bnet_recv(socket,buf, &len)>0){
		//writen(fd,buf,len);
		total_len+=len;
	}
	TIMER_END(end);
	TIMER_DIFF(total_time,start,end);
	close(fd);
	close(socket);
	printf("total time=%.4f  %.4fMB/s\n",total_time,total_len/total_time/1036288);
	return NULL;
}

void *handle_client_request(void * data){
	int fd=*(int *)data; /*socket fd */
	char msg[300];
	int len=0;
	int result=0;
	//get_socket_default_bufsize(fd);
	
	while(1){
		memset(msg,0,300);
		//if(G_PIPELINE){
		//	set_sendbuf_size(fd,0);
		//	set_recvbuf_size(fd,0);
		//}
		if(bnet_recv(fd,msg,&len)==ERROR ){
			printf("%s,%d network error\n",__FILE__,__LINE__);
			break;
		}
		if(len<=0)
			break;
		if(strncmp(msg,"BACK",4)==0){
			if(G_NOT_DEDUP){
				backup_formal(fd,msg);
			}
			else if(G_PIPELINE){
				while(data_fd<0)
					sleep(1);
				pipeline_backup( fd,data_fd,msg);
			}
			else
				backup_dedup(fd,msg);
		}	
		else if(strncmp(msg,"REST",4)==0){
			if(G_NOT_DEDUP){
				restore_formal( fd, msg);
			}
			else
				restore( fd, msg);
			break;
		}	
		else if(strncmp(msg,"DELE",4)==0){
			delete_job(fd,msg);
		}
		else {
			printf("%s,%d invalid command \n",__FILE__,__LINE__);
		}	
	}
	printf("%s %d client quit\n",__FILE__,__LINE__);
	close(fd);
	close(data_fd);
	data_fd=-1;
	
}


void *wait_conncet(void * arg){
	pthread_detach(pthread_self());
	struct sockaddr_in s_addr;
	s_addr.sin_family = AF_INET;
	s_addr.sin_addr.s_addr = INADDR_ANY;
	s_addr.sin_port = htons(SERVER_OTH_PORT );

	int newsockfd=-1, fd=-1;
	socklen_t clilen;
	struct sockaddr cli_addr;/* 客户端address */
	int reuse = 1;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	//setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));
	if (bind(fd, (struct sockaddr*) &s_addr, sizeof(struct sockaddr))!= 0) {
			printf("Can not bind address!\n");
			return -1;
	}
	if (listen(fd, 5) != 0) {
		printf("Can not listen the port!\n");
		return -1;
	}
    	
	for (;;) {
		unsigned int maxfd = 0;
		fd_set sockset;
		FD_ZERO(&sockset);

		FD_SET((unsigned) fd, &sockset);
		maxfd = maxfd > (unsigned) fd ? maxfd : fd;
		errno = 0;
		if (select(maxfd + 1, &sockset, NULL, NULL, NULL) < 0) {
			if (errno == EINTR)
				continue;
			close(fd); //error
			break;
		}
		if (FD_ISSET(fd, &sockset)) {
			/* Got a connection, now accept it. */
			do {
				clilen = sizeof(cli_addr);
				newsockfd = accept(fd, &cli_addr, &clilen);
			} while (newsockfd < 0 && errno == EINTR);
			if (newsockfd < 0) {
				continue;
			}
			data_fd= newsockfd;
			printf("%s %d connectd fd:%d\n",__FILE__,__LINE__,data_fd);	
		}
	}
	//err_msg1("wrong socket  connect");
	return NULL;
}
void make_dir(){
	/*
	 * recursively make directory
	 */
	 char *p=NULL;
	 char *q=BackupVolPath+1;
	while ((p = strchr(q, '/'))) {
		if (*p == *(p - 1)) {
			q++;
			continue;
		}
		*p = 0;
		if (access(BackupVolPath, 0) != 0) {
			mkdir(BackupVolPath, S_IRWXU | S_IRWXG | S_IRWXO);
		}
		*p = '/';
		q = p + 1;
	}
}


int  bnet_thread_server(workq_t *client_wq,int max_client_count,int port,void *handle_client_request(void * data)) {

	
	struct sockaddr_in s_addr;
	s_addr.sin_family = AF_INET;
	s_addr.sin_addr.s_addr = INADDR_ANY;
	s_addr.sin_port = htons(port);

	int newsockfd, fd;
	socklen_t clilen;
	struct sockaddr cli_addr;/* 客户端address */
	int reuse = 1;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));// 重新试用地址
	
	if (bind(fd, (struct sockaddr*) &s_addr, sizeof(struct sockaddr))!= 0) {
			printf("Can not bind address!\n");
			return -1;
	}
	if (listen(fd, 20) != 0) {
		printf("Can not listen the port!\n");
		return -1;
	}
    	workq_init(client_wq,max_client_count,handle_client_request);
	for (;;) {
		unsigned int maxfd = 0;
		fd_set sockset;
		FD_ZERO(&sockset);

		FD_SET((unsigned) fd, &sockset);
		maxfd = maxfd > (unsigned) fd ? maxfd : fd;
		errno = 0;
		if (select(maxfd + 1, &sockset, NULL, NULL, NULL) < 0) {
			if (errno == EINTR)
				continue;
			close(fd); //error
			break;
		}
		if (FD_ISSET(fd, &sockset)) {
			/* Got a connection, now accept it. */
			do {
				clilen = sizeof(cli_addr);
				newsockfd = accept(fd, &cli_addr, &clilen);
			} while (newsockfd < 0 && errno == EINTR);
			if (newsockfd < 0) {
				continue;
			}
			printf("%s %d connectd fd:%d\n",__FILE__,__LINE__,newsockfd);
			workq_add(client_wq,(void *)&newsockfd,NULL,0);	
		}

	}
	workq_destroy(client_wq);
	/* 销毁客户端作业工作队列 */
}

