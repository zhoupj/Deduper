#include "global.h"

#define PIPELINE_FILE_NEW -7
#define PIPELINE_FILE_OVER -9
#define PIPELINE_FILE_NODATA -10
#define PIPELINE_OVER -8
int old_container_num=0;
int new_container_num=1;

PipelineQueue * finger_queue=NULL;
PipelineQueue * chunk_queue=NULL;
MemIndex * mem_index=NULL;
JCR * jcr=NULL;



void *pipeline_backup(int fd_finger,int fd_data,char *msg){
	char *buf;
	char *buffer;
	int buflen=0;
	char fileset[256]={0};
	int i=0;
	char *p;
	Recipe *rp=NULL;
	FingerChunk * fc=NULL;
	FingerChunk * first_fc=NULL;
	JOB_V *jobv=NULL;
	int finger_sockid=fd_finger;
	int index=0;
	JCR *jcr=NULL;
	pthread_t recv_data,write_data;
	TIMER_DECLARE(start,end);
	TIMER_DECLARE(s_start,s_end);
	//TIMER_DECLARE(w_start,w_end);
	//TIMER_DECLARE(Sstart,Send);
	//TIMER_DECLARE(Rstart,Rend);
	//TIMER_DECLARE(Lstart,Lend);

	ChunkAddr *cka=NULL;
	
	buf=(char *)malloc(MAX_FINGERS_NUM/(2*1024)*(24+256+4+4)+1);
	buffer=(char *)malloc(MAX_FINGERS_NUM/(2*1024)+1);
	container_vol_init();
	jobcount_init();
	jcr=jcr_new();
	jcr->fingerSocket=fd_finger;
	jcr->dataSocket=fd_data;
	mem_index=index_init();
	chunk_queue=pipeline_queue_init(-1);
	old_container_num=0;
	new_container_num=1;
	pthread_create(&recv_data, NULL, pipeline_recv_data, jcr);
         pthread_create(&write_data, NULL, pipeline_write_data, jcr);

	TIMER_START(start);
	if(sscanf(msg,backup_cmd,fileset)!=1){ // backup cmd
		err_msg1("backup_cmd is wrong ");
	}
	printf("\033[40;32m recv filepath: %s  \033[0m\n",msg);
	jobv=jobv_new(fileset);
	
	int namelen=0;
	int chunklen=0;
	int j=0;
	
	//=======================
	//if(G_DEFRAGMENT)
	//scan_init();
	Queue * scan_queue=NULL;
	FingerStream * fs=NULL;
	int scan_len=0;
	int send_len=0;
	int send_index=0;
	scan_queue=queue_new();
	//=====================
	Cbr_init();
	//====================
	capping_init();

	//============
	
	TIMER_START(s_start);
	//format: | filename_len | filename | fingerprint_count |fingerprint0 size0 fingerprint1 size1...|
	while(bnet_recv(finger_sockid,buf,&buflen)>0){
		//if(G_DEFRAGMENT) 
		//scan_reset();
		p=buf;
		j=0;
		while((p-buf)<buflen){
			memcpy(&namelen,p,4); p+=4;
			//printf("%d \t",namelen);
			if(namelen>0){
				if(rp){
					jobv_insert_recipe(jobv, rp);
					jcr->nChunkCount+=rp->chunknum;
					recipe_free(rp);
				}
				rp=recipe_new();
				memcpy(rp->filename,p,namelen);p+=namelen;
				rp->fileindex=(++index);	
				jcr->nFileCount++;
				if(G_VERBOSE)
					printf("recv filename : %s (%d)\n",rp->filename,jcr->nFileCount);
			}
			memcpy(&chunklen,p,4);p+=4;
			//printf("%d \n",chunklen);
			i=0;
			while(i<chunklen){
				fc=fingerchunk_new(p,0); 
				p+=sizeof(Fingerprint);
				memcpy(&fc->length,p,sizeof(fc->length));p+=sizeof(fc->length);
				jcr->nSize+=fc->length;
				G_total_size+=fc->length;
				//printf("%d \n",fc->length);
				cka=(ChunkAddr*)index_lookup(mem_index,fc->fingerprint);
				 if(G_DEFRAGMENT==1){
					if (scan_len>=SCAN_REGION_SIZE){
						fs=queue_pop(scan_queue);
						if(fs->container_id>0)
							buffer[send_index++]=cbr_decision(fs->container_id,fs->length) ==1? '0' : '1';
						else if(fs->container_id==0)
							buffer[send_index++]='1'; //作业内自身重删
						else
							buffer[send_index++]='0'; // new items
						
						send_len+=fs->length;
						scan_len-=fs->length;
						if(send_len>=MAX_FINGERS_NUM){
						//	printf("send finger number:%d \n",send_index);
							bnet_send(jcr->dataSocket,buffer,send_index);	
							send_index=0;
							send_len=0;
						}
						free(fs);
					}
					fs=(FingerStream *)malloc(sizeof(FingerStream));
					scan_len+=fc->length;
					fs->length=fc->length;
					if(cka){  // old items
						fs->container_id=cka->container_id;
						if(fs->container_id>0)
							Cbr_scan(fs->container_id,fs->length);	
					}
					else{
						
						fs->container_id=-1; // new items
						
					}
					queue_push(scan_queue,fs);
					
				 }
				 else if(G_DEFRAGMENT==2){
				 	if (scan_len>=SCAN_REGION_SIZE){
						capping_build();
						while(scan_len){
							fs=queue_pop(scan_queue);
							if(fs->container_id>0)
								buffer[send_index++]=capping_rewrite(fs->container_id) ==1? '0' : '1';
							else if(fs->container_id==0)
								buffer[send_index++]='1'; //作业内自身重删
							else
								buffer[send_index++]='0'; // new items
							
							send_len+=fs->length;
							scan_len-=fs->length;
							if(send_len>=MAX_FINGERS_NUM){
							//	printf("send finger number:%d \n",send_index);
								bnet_send(jcr->dataSocket,buffer,send_index);	
								send_index=0;
								send_len=0;
							}
							free(fs);
						}
						capping_reset();
					}
					fs=(FingerStream *)malloc(sizeof(FingerStream));
					scan_len+=fc->length;
					fs->length=fc->length;
					if(cka){  // old items
						fs->container_id=cka->container_id;
						if(fs->container_id>0)
							capping_insert(fs->container_id);	
					}
					else{
						
						fs->container_id=-1; // new items
						
					}
					queue_push(scan_queue,fs);
				 }
            
				else{
				
					if(cka){
						//printf("%d ",cka->container_id);
						buffer[j++]='1';//不需要重发
					}
					else
					{
						//printf("0 ");
						buffer[j++]='0'; // 指纹对应的块需要重发
					}
				}
				recipe_append_fingerchunk(rp,fc);
				i++;
			}
			
			
		}
		//old_container_num+=scan_numer();
		if(j>0){
			buffer[j]=0;
		//	printf("\033[40;32m send  (%d) \033[0m\n",j);
			//printf("not defragementation\n");
			bnet_send(jcr->dataSocket,buffer,j);		
		}
		
		
	}
	if(G_DEFRAGMENT==1) {
		while(queue_size(scan_queue)){
			fs=queue_pop(scan_queue);
			if(fs->container_id>0)
				buffer[send_index++]=cbr_decision(fs->container_id,fs->length) ==1? '0' : '1';
			else if(fs->container_id==0)
				buffer[send_index++]='1'; //作业内自身重删
			else
				buffer[send_index++]='0'; // new items
						
			send_len+=fs->length;		
			if(send_len>=MAX_FINGERS_NUM){
				//printf("send finger number:%d \n",send_index);
				bnet_send(jcr->dataSocket,buffer,send_index);	
				send_index=0;
				send_len=0;
			}
			free(fs);
		}
		if(send_index){
			printf("send finger number:%d \n",send_index);
			bnet_send(jcr->dataSocket,buffer,send_index);	
		}
	}
	else if(G_DEFRAGMENT==2){
		capping_build();
		while(queue_size(scan_queue)){
			fs=queue_pop(scan_queue);
			if(fs->container_id)
				buffer[send_index++]=capping_rewrite(fs->container_id) ==1? '0' : '1';
			else if(fs->container_id==0)
				buffer[send_index++]='1'; //作业内自身重删
			else
				buffer[send_index++]='0'; // new items
						
			send_len+=fs->length;		
			if(send_len>=MAX_FINGERS_NUM){
				//printf("send finger number:%d \n",send_index);
				bnet_send(jcr->dataSocket,buffer,send_index);	
				send_index=0;
				send_len=0;
			}
			free(fs);
		}
		if(send_index){
			printf("send finger number:%d \n",send_index);
			bnet_send(jcr->dataSocket,buffer,send_index);	
		}
	}
	queue_free(scan_queue);
	//scan_destroy();
	//==============
	Cbr_destroy();
	//==========
	capping_destroy();
	//==============
	TIMER_END(s_end);
	TIMER_DIFF(jcr->searchTime,s_start,s_end);
	jobv_insert_recipe(jobv, rp);
	jcr->nChunkCount+=rp->chunknum;
	recipe_free(rp);
	
	if(buflen==STREAM_END)
		printf("%s %d recv all fingerprints right \n",__FILE__,__LINE__);
	else
		err_msg1("recv fingerprints wrong");

	pthread_join(recv_data, NULL);
	pthread_join(write_data, NULL);

	bnet_send(jcr->dataSocket,"OK",2);  // 发送备份成功信息
	
         TIMER_END(end);
	TIMER_DIFF(jcr->totalTime,start,end);
	
	pipeline_queue_destroy(chunk_queue);
	jobv_destroy(jobv);
	container_vol_destroy();
	jobcount_close();
	index_destroy(mem_index);

	printf("============back over===============\n");
	printf("total time:%.4f  total data size: %ld    throughput:       %.4f MB/s\n",jcr->totalTime,jcr->nDedupSize,jcr->nDedupSize*1.0/jcr->totalTime/1036288.0);
	printf("writerecipe time:  %-20.4f \n ",jcr->writeRecipeTime);
	printf("search time:  %-20.4f \n ",jcr->searchTime);
	printf("recv time:   %-20.4f        throughput:        %.4f MB/s\n",jcr->recvTime,jcr->nDedupSize*1.0/jcr->recvTime/1036288.0);
	printf("writedata  time:  %-20.4f    throughput:        %.4f MB/s\n",jcr->writeDataTime,jcr->nDedupSize*1.0/jcr->writeDataTime/1036288.0);
	printf("old chunk count:  %d,   deduped chunk count:%d\n",jcr->nChunkCount,jcr->nDedupChunkCount);
	printf("file total  %d \n",jcr->nFileCount);
	printf(" container number/MB: %10.2f  [%d %ld   ]\n",(new_container_num+old_container_num)*1.0/(jcr->nSize*1.0/(1024*1024)),(new_container_num+old_container_num),jcr->nSize);
	return NULL;
}

void * pipeline_recv_data(void * arg){

	char *buf=(char *)calloc(1,SOCKET_BUF_SIZE+21);
	int len=0;
	int64_t file_len=0;;
	char *p=NULL;
	Chunk *chunk=NULL;
	int fd;
	JCR *jcr=(JCR *)arg;
	TIMER_DECLARE(start,end);
	printf("%s ,%d pipeline_recv_data start \n",__FILE__,__LINE__);
	TIMER_START(start);
	while(bnet_recv(jcr->dataSocket,buf,&len)!=ERROR){
		
		if(len==STREAM_END){
			printf("%s %d backup is over\n",__FILE__,__LINE__);
			break;
		}
		else if(len==DATA_END){
			//printf("\033[40;32m recv: one file data over (%ld) \033[0m\n",file_len);
			file_len=0;
		}
		else{
			chunk=chunk_new(buf,buf+sizeof(Fingerprint),len-sizeof(Fingerprint));
			jcr->nDedupChunkCount++;
			jcr->nDedupSize+=chunk->length;
			file_len+=chunk->length;
			pipeline_queue_push(chunk_queue,chunk);
			//printf("\033[40;32m recv data len: %d  \033[0m\n",chunk->length);
		}
		
	}
	TIMER_END(end);
	TIMER_DIFF(jcr->recvTime,start,end);
	
	chunk=(Chunk *)malloc(sizeof(Chunk));
	chunk->data=NULL;
	chunk->length=STREAM_END;
	pipeline_queue_push(chunk_queue,chunk);
	printf("%s ,%d pipeline_recv_data over \n",__FILE__,__LINE__);
	return NULL;
	
}
void * pipeline_write_data(void *arg){
	JCR *jcr=(JCR *)arg;
	Chunk *chunk=NULL;
	Container *container=NULL;
	container=container_new();
	TIMER_DECLARE(start,end);
	printf("%s ,%d pipeline_write_data start \n",__FILE__,__LINE__);
	while(1){
		chunk=(Chunk*)pipeline_queue_pop(chunk_queue);
		TIMER_START(start);
		if(chunk->length<0){
			chunk_free(chunk);
			break;
		}
		while(write_chunk(container, chunk)==false){
			new_container_num++;
			write_container(container);
			index_insert(mem_index,container);
			container_destroy(container);
			container=container_new();
		}
		chunk_free(chunk);
		TIMER_END(end);
		TIMER_DIFF(jcr->writeDataTime,start,end);
	}
	TIMER_START(start);
	if(container->data_size>0){
		write_container(container);
		index_insert(mem_index,container);
	}
	container_destroy(container);
	TIMER_END(end);
	TIMER_DIFF(jcr->writeDataTime,start,end);
	printf("%s ,%d pipeline_write_data over \n",__FILE__,__LINE__);
	return NULL;
}
