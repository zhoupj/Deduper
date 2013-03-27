#include "global.h"

#define PIPELINE_FILE_NEW -7
#define PIPELINE_FILE_OVER -9
#define PIPELINE_FILE_NODATA -10
#define PIPELINE_OVER -8


PipelineQueue* chunk_queue;  /* chunk queue */
PipelineQueue* sha_queue;       /* hash queue */
PipelineQueue * filter_queue; 
PipelineQueue *read_queue;

//PipelineQueue * data_queue;
int small_chunk=0;

int backup_pipeline(int finger_socket,int data_socket, char *path) {
	char buf[256]={0};
	int len=0;
	struct stat state;
	 pthread_t  chunk_id, sha_id, filter_id,send_data_id;
	 chunk_t *chunk;
	JCR *jcr = jcr_new();
	jcr->data_socket=data_socket;
	jcr->finger_socket=finger_socket;
	strcpy(jcr->backup_path, path);

	if (access(jcr->backup_path, F_OK) != 0) {
		puts("This path does not exist or can not be read!");
		return FAILURE;
	}

	if (stat(jcr->backup_path, &state) != 0) {
		puts("backup path does not exist!");
		return FAILURE;
	}
	
       chunk_alg_init();
        read_queue=pipeline_queue_init(100);
        chunk_queue = pipeline_queue_init((MAX_FINERS_NUM/8/1024)*2);
       sha_queue = pipeline_queue_init(-1);
       filter_queue= pipeline_queue_init(-1);
	   
       TIMER_DECLARE(start,end);
       TIMER_START(start);
	
        pthread_create(&chunk_id, NULL, chunk_module, jcr);
        pthread_create(&sha_id, NULL, sha_module, jcr);
        pthread_create(&filter_id, NULL, filter_module, jcr);
        pthread_create(&send_data_id,NULL,data_module,jcr);
  
	//get_socket_default_bufsize(jcr->finger_socket);
	//set_sendbuf_size(jcr->finger_socket,0);
	//set_recvbuf_size(jcr->finger_socket,0);
	
	if (S_ISREG(state.st_mode)) { //single file
	
		char *p = jcr->backup_path + strlen(jcr->backup_path) - 1;
		while (*p != '/')
			--p;
		*(p + 1) = 0;
		
		sprintf(buf, backup_cmd, jcr->backup_path); //send pathname
		printf("backup_path=%s\n",buf);
		bnet_send(jcr->finger_socket, buf, strlen(buf));
		pipeline_send_file(jcr, path);
		
	} else {
		int len=strlen(jcr->backup_path);
		if(jcr->backup_path[len-1]!='/')
			jcr->backup_path[len]='/';
		sprintf(buf, backup_cmd, jcr->backup_path);
		printf("backup_path=%s\n",jcr->backup_path);
		bnet_send(jcr->finger_socket ,buf, strlen(buf));
		pipeline_walk_dir(jcr, jcr->backup_path);
	}

	 chunk=(chunk_t *)malloc(sizeof(chunk_t));
	 chunk->datalen=PIPELINE_OVER;
	 pipeline_queue_push(read_queue,chunk);

	pthread_join(chunk_id, NULL);
    	pthread_join(sha_id, NULL);
    	pthread_join(filter_id, NULL);
    	pthread_join(send_data_id, NULL);

	if(bnet_recv(jcr->data_socket,buf,&len)>0){
		if(memcmp(buf,"OK",2)==0)
			printf("=congratulations ====backup success=========\n");
		else
		err_msg1("backup fail");
	}
	else
		err_msg1("backup fail");

	TIMER_END(end);
	TIMER_DIFF(jcr->total_time,start,end);

	 pipeline_queue_destroy(read_queue);
	 pipeline_queue_destroy(chunk_queue);
	 pipeline_queue_destroy(sha_queue);
	 pipeline_queue_destroy(filter_queue);
	 
	
    	printf("read_time:  %-10.3fs  %-10.3fMB/s\n", jcr->read_time,jcr->old_size*1.0/jcr->read_time/1036288); //1024*1024
	printf("chuk_time:  %-10.3fs  %-10.3fMB/s\n", jcr->chunk_time,jcr->old_size*1.0/jcr->chunk_time/1036288);
	printf("sha1_time:  %-10.3fs  %-10.3fMB/s\n", jcr->sha_time,jcr->old_size*1.0/jcr->sha_time/1036288);
	printf("srch_time:  %-10.3fs  \n", jcr->search_time);
	printf("send_time:  %-10.3fs  %-10.3fMB/s\n\n", jcr->send_time,jcr->dedup_size*1.0 /jcr->send_time/1036288);
	printf("totl_time:  %-10.3fs  %-10.3fMB/s\n", jcr->total_time,jcr->old_size*1.0/jcr->total_time/1036288);

	printf("deduped_size/old_size    :   %20ld/%20ld   %20.4f% \n",jcr->dedup_size,jcr->old_size,(1-jcr->dedup_size*1.0/jcr->old_size)*100);
	printf("deduped_chunks/old_chunks:   %20d/%20d   %20.4f% \n",jcr->dedup_chunk_count,jcr->chunk_count,jcr->dedup_chunk_count*1.00/jcr->chunk_count*100);
	printf("average chunk size       :   %20.2f KB\n",jcr->old_size*1.0/jcr->chunk_count/1024.0);
	printf("total file               :   %20d \n",jcr->file_count);
    	//printf("small chunk(<2K>)        :   %20d \n",small_chunk);	
	jcr_free( jcr);
	return 0;
}

void pipeline_send_file(JCR *jcr,char *path){

	chunk_t *chunk;
	 int subFile;
	 int32_t srclen=0;
	TIMER_DECLARE(start,end);
	TIMER_START(start);
	
	 chunk=(chunk_t *)malloc(sizeof(chunk_t));
	 chunk->datalen=PIPELINE_FILE_NEW;
	 strncpy(chunk->data,path,strlen(path)+1);
	 pipeline_queue_push(read_queue,chunk);
	 
	  if ((subFile=open(path, O_RDONLY)) < 0) {
	         err_msg2("open file %s error!",path);
		  return;
	  }
	chunk=(chunk_t *)malloc(sizeof(chunk_t));
	while((chunk->datalen=readn(subFile,chunk->data,MAX_CHUNK_SIZE))>0){
		 pipeline_queue_push(read_queue,chunk);
		 chunk=(chunk_t *)malloc(sizeof(chunk_t));
	}
	free(chunk);
	close(subFile);
	// chunk_module(jcr,path); // 求指纹
	  TIMER_END(end);
	  TIMER_DIFF(jcr->read_time,start,end);
}


void* chunk_module(void * arg){  
	 int subFile;
          int32_t srclen=0, left_bytes = 0;
          int32_t size=0,len=0; 
          int32_t n = MAX_CHUNK_SIZE;
	chunk_t *chunk;
	chunk_t *ck;
	unsigned char * p;
         unsigned char * src = (unsigned char *)malloc(MAX_CHUNK_SIZE*2);	
	JCR *jcr=(JCR *)arg;

	TIMER_DECLARE(start,end);
	

        // chunk_alg_init();
	 if(src == NULL) {
           	err_msg1("Memory allocation failed.");
		  return;
     	}
         printf("%s %d chunk_module start\n",__FILE__,__LINE__);
	 while (1) 
	 {
	 	ck=(chunk_t *)pipeline_queue_pop(read_queue);
		TIMER_START(start);
		if(ck->datalen==PIPELINE_OVER){
			break;
		}
		else if(ck->datalen==PIPELINE_FILE_NEW){
		        /******more(last file )******/
		     if(srclen>0)
		    {
				jcr->chunk_count++;
				chunk=(chunk_t *)malloc(sizeof(chunk_t));
				if(!chunk)
					err_msg1("malloc wrong");
				memcpy(chunk->data,src,srclen);
				chunk->datalen=srclen;
				pipeline_queue_push(chunk_queue,chunk);
			 }
			 srclen=0;
			 left_bytes = 0;
			pipeline_queue_push(chunk_queue,ck);
		}
		else{
			memcpy(src+left_bytes,ck->data,ck->datalen);
			jcr->old_size+=ck->datalen;
			srclen+=ck->datalen;
			free(ck);
			left_bytes=0;
		
			 if(srclen<=MIN_CHUNK_SIZE){
			            small_chunk++;
			            continue;  // 避免没必要的求指纹
			   }
			 	//break;  
			
			p = src;
			len=0;
			while (len < srclen) 
			{
	          		n = srclen -len;
				size=chunk_data(p, n);/*根据数据分块*/
				if(n==size && n < MAX_CHUNK_SIZE)
				{ 	
					/*将未分完的数据拷贝到left中*/
	          			memmove(src, src+len, n );
	          			left_bytes = n;
	                			break;
				}  
				chunk=(chunk_t *)malloc(sizeof(chunk_t));
				memcpy(chunk->data,p,size);
				chunk->datalen=size;
				pipeline_queue_push(chunk_queue,chunk);
		
				jcr->chunk_count++;
				p = p + size;
				len+=size;
			}
			srclen=left_bytes;
		}
		TIMER_END(end);
		TIMER_DIFF(jcr->chunk_time,start,end);
    	 }
	
	 /******more******/
	 len=0;
	 if(srclen>0)
	 	len=srclen;
	 else  if(left_bytes>0)
	 	len=left_bytes;
	 if(len>0){
		jcr->chunk_count++;
	 	p= src;
		chunk=(chunk_t *)malloc(sizeof(chunk_t));
		memcpy(chunk->data,p,len);
		chunk->datalen=len;
		pipeline_queue_push(chunk_queue,chunk);
	 }
	pipeline_queue_push(chunk_queue,ck);
          free(src);
          printf("%s %d chunk_module over\n",__FILE__,__LINE__);
}

void * sha_module(void *arg){
	JCR *jcr=(JCR *)arg;
	chunk_t *chunk=NULL;
	
	Recipe *first=NULL,*last=NULL;
	//int finger_count=0;
	int finger_len=0;
	Recipe *rp=NULL,*newrp=NULL;
	FingerChunk * fc=NULL;
	int quit=0;
	int total_chunk_count=0;//small files
	
	TIMER_DECLARE(start,end);
	printf("%s %d sha_module start\n",__FILE__,__LINE__);
	while(!quit){
		chunk=(chunk_t *)pipeline_queue_pop(chunk_queue);
		TIMER_START(start);
		switch(chunk->datalen){
			case PIPELINE_FILE_NEW:
				rp=recipe_new(); //包含文件名

				if(first==NULL)  // rp 链表
					first=rp;
				else
					last->next=rp;
				last=rp;
				
				rp->fileindex=(++jcr->file_count);
				rp->flag=PIPELINE_FILE_NEW;
				memcpy(rp->filename,chunk->data,strlen(chunk->data)+1);
				free(chunk);
				break;
		
			case PIPELINE_OVER:

				pipeline_queue_push(sha_queue,first);

				rp=recipe_new();
				rp->flag=PIPELINE_OVER;
				pipeline_queue_push(sha_queue,rp);
				free(chunk);
				quit=1;
				break;
				
			default:
				if(finger_len>=MAX_FINERS_NUM){
					newrp=recipe_create(0,rp->fileindex); 
					pipeline_queue_push(sha_queue,first);
					first=newrp;
					last=newrp;
					//finger_count=0;
					finger_len=0;
					rp=newrp;
				}
				fc=fingerchunk_new();
				chunk_finger(chunk->data,chunk->datalen,fc->fingerprint);/*对分得的块求取指纹*/
				fc->chunklen=chunk->datalen;
				recipe_append_fingerchunk(rp,fc);
				//finger_count++; 
				finger_len+=chunk->datalen;
				free(chunk);
		}
		TIMER_END(end);
		TIMER_DIFF(jcr->sha_time,start,end);
	}
	printf("%s %d sha_module over\n",__FILE__,__LINE__);
}
void * filter_module(void *arg){
	JCR *jcr=(JCR *)arg;
	Recipe *rp=NULL;
	Recipe *prp=NULL;
	int namelen=0;
	int finger_count=0;
	FingerChunk *fc=NULL;
	char * buf;
	int sendlen=0;
	int recvlen=0;
	int i=0;
	int flag=0;
	int quit=0;
	double recv_time=0;
	double send_time=0;
	buf=(char *)malloc((MAX_FINERS_NUM/2/1024)*(24+256+4+4)+1);
	TIMER_DECLARE(start,end);
	TIMER_DECLARE(r_start,r_end);
	TIMER_DECLARE(s_start,s_end);
	printf("%s %d filter_module start\n",__FILE__,__LINE__);
	while(!quit){
		 rp=(Recipe *)pipeline_queue_pop(sha_queue);
		TIMER_START(start);

		if(rp->flag== PIPELINE_OVER){
			bnet_signal(jcr->finger_socket,STREAM_END);
			quit=1;
		}
		else {  // [namelen name chunk_number chunk_finger,chunk_size,chunk_finger,chunk_size ]
			sendlen=0;
			finger_count=0;
			prp=rp;
			while(prp){
				if(prp->flag==PIPELINE_FILE_NEW){
					namelen=strlen(prp->filename); // 如果两个主机字节序不一样，此处需要更改
					memcpy(buf+sendlen,&namelen,4);sendlen+=4;
					memcpy(buf+sendlen,prp->filename,namelen);sendlen+=namelen;
				}
				else{
					namelen=0;
					memcpy(buf+sendlen,&namelen,4);sendlen+=4;
				}
				memcpy(buf+sendlen,&prp->chunknum,4);sendlen+=4;
				fc=prp->first;
				while(fc){
					memcpy(buf+sendlen,fc->fingerprint,sizeof(Fingerprint));sendlen+=sizeof(Fingerprint);
					memcpy(buf+sendlen,&fc->chunklen,sizeof(fc->chunklen));sendlen+=sizeof(fc->chunklen);
					//printf("%d  \n",fc->chunklen);
					fc=fc->next;
					finger_count++;
				}
				prp=prp->next;
			}
			if(sendlen>0){
				        TIMER_START(s_start);
					bnet_send(jcr->finger_socket,buf,sendlen);
					TIMER_END(s_end);
					TIMER_DIFF(send_time,s_start,s_end);
					//printf("finger count:%d\n",finger_count);
					/*i=0;
					recvlen=0;
					TIMER_START(r_start);
					if(bnet_recv(jcr->finger_socket,buf,&recvlen)>0){
						TIMER_END(r_end);
						TIMER_DIFF(recv_time,r_start,r_end);

						if(finger_count!=recvlen)
							err_msg1("recv finger response wrong");
						//printf("%s\n",buf);
						prp=rp;
						while(prp && i<recvlen){
							fc=prp->first;
							while(i<recvlen && fc){
								fc->existed=buf[i];
								i++;
								fc=fc->next;
							}
							prp=prp->next;
						}			
					}
					*/
			}
		}
		
		TIMER_END(end);
		TIMER_DIFF(jcr->search_time,start,end);
		
		pipeline_queue_push(filter_queue,rp);
		
		
	}
	printf("%s %d filter_module over\n",__FILE__,__LINE__);
	printf("=========send finger time:%.4f \n",send_time);
	//printf("=========recv finger time:%.4f \n",recv_time);
}
void * data_module(void *arg){
	JCR *jcr=(JCR *)arg;
	Recipe *rp=NULL;
	Recipe  *prp=NULL;
	FingerChunk *fc=NULL;
	static int64_t read_len=0;
	//char stream[30]={0};
	char buf[SOCKET_BUF_SIZE+21]={0};
	int fd=-1;
	int quit=0;
	int recvlen=0;
	char *buff=(char *)malloc((MAX_FINERS_NUM/2/1024)+1);
	int len=0;
	int i=0;
	TIMER_DECLARE(start,end);
	printf("%s %d data_module start\n",__FILE__,__LINE__);
	//set_sendbuf_size(jcr->data_socket,32*1024);
	get_socket_default_bufsize(jcr->data_socket);
	while(!quit){
		rp=(Recipe*)pipeline_queue_pop(filter_queue);
		
		TIMER_START(start);
		if(rp->flag==PIPELINE_OVER){
			bnet_signal(jcr->data_socket,DATA_END);
			recipe_free(rp);
			bnet_signal(jcr->data_socket,STREAM_END);  
			quit=1;
		}
		else{
			i=0;
			if(bnet_recv(jcr->data_socket,buff,&recvlen)>0){// recv fingerprint answers
					//printf("%d\n",recvlen);
						prp=rp;
						while(prp && i<recvlen){
							fc=prp->first;
							while(i<recvlen && fc){
								fc->existed=buff[i];
								i++;
								fc=fc->next;
							}
							prp=prp->next;
						}			
			}
			prp=rp;
			while(prp){
				//printf("data:%s\n",prp->filename);
				if(prp->flag==PIPELINE_FILE_NEW){
					if(fd>0){
						close(fd);
						bnet_signal(jcr->data_socket,DATA_END);
					}
					if(G_VERBOSE)
					    printf("send file :%s \n",prp->filename);
					if ((fd=open(prp->filename, O_RDONLY)) < 0) {
		 				printf("%s,%d open file error!\n",__FILE__,__LINE__);
					 }
					lseek(fd,0,SEEK_SET);
					read_len=0;
				}
				
				for(fc=prp->first;fc;fc=fc->next){
						if(fc->existed=='0'){
							lseek(fd,read_len,SEEK_SET);
							memcpy(buf,fc->fingerprint,sizeof(Fingerprint));
							if(readn(fd,buf+sizeof(Fingerprint),fc->chunklen)!=fc->chunklen)
								printf("%s %d read data chunklen is wrong \n",__FILE__,__LINE__);
							 bnet_send(jcr->data_socket,buf,fc->chunklen+sizeof(Fingerprint));
							jcr->dedup_chunk_count++;
							jcr->dedup_size+=fc->chunklen;
						}
						read_len+=fc->chunklen;
				}
				rp=prp->next;
				recipe_free(prp);
				prp=rp;
				
			}
		}
		
		TIMER_END(end);
		TIMER_DIFF(jcr->send_time,start,end);
	}
	printf("%s %d data_module over \n",__FILE__,__LINE__);
	if(fd>0)
		close(fd);

}
int  pipeline_walk_dir (JCR *psJcr, char *path) {
	struct stat state;
	if (stat(path, &state) != 0) {
		puts("file does not exist! ignored!");
		return 0;
	}
	if (S_ISDIR(state.st_mode)) {
		DIR *dir = opendir(path);
		struct dirent *entry;
		char newpath[512];
		memset(newpath,0,512);
		if (strcmp(path + strlen(path) - 1, "/")) {
			strcat(path, "/");
		}

		while ((entry = readdir(dir)) != 0) {
			/*ignore . and ..*/
			if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))//(entry->d_name[0]=='.')
				continue;
			strcpy(newpath, path);
			strcat(newpath, entry->d_name);
			if (pipeline_walk_dir(psJcr, newpath) != 0) {
				return -1;
			}
		}
		//printf("*** out %s direcotry ***\n", path);
		closedir(dir);
	} 
	else if (S_ISREG(state.st_mode)) {
		pipeline_send_file(psJcr, path);

	} else {
		puts("illegal file type! ignored!");
		return 0;
	}
	return 0;
}


