#include "global.h"

int backup_client(int socket, char *path) {
	char buf[256]={0};
	int len=0;
	struct stat state;
	JCR *jcr = jcr_new();
	jcr->data_socket=socket;
	strcpy(jcr->backup_path, path);

	if (access(jcr->backup_path, F_OK) != 0) {
		puts("This path does not exist or can not be read!");
		return FAILURE;
	}

	if (stat(jcr->backup_path, &state) != 0) {
		puts("backup path does not exist!");
		return FAILURE;
	}

	

	TIMER_DECLARE(start,end);
	TIMER_START(start);
	
	if (S_ISREG(state.st_mode)) { //single file
	
		char *p = jcr->backup_path + strlen(jcr->backup_path) - 1;
		while (*p != '/')
			--p;
		*(p + 1) = 0;
		
		sprintf(buf, backup_cmd, jcr->backup_path); //send pathname
		printf("%s\n",buf);
		bnet_send(socket, buf, strlen(buf));
		send_file(jcr, path);
		
	} else {
		int len=strlen(jcr->backup_path);
		if(jcr->backup_path[len-1]!='/')
			jcr->backup_path[len]='/';
		sprintf(buf, backup_cmd, jcr->backup_path);
		printf("%s\n",jcr->backup_path);
		bnet_send(socket, buf, strlen(buf));
		walk_dir(jcr, jcr->backup_path);
	}

	bnet_signal(socket, STREAM_END) ;
	if(bnet_recv(socket,buf,&len)>0){
		if(memcmp(buf,"OK",2)==0)
			printf("=congratulations ====backup success=========\n");
		else
		err_msg1("backup fail");
	}
	else
		err_msg1("backup fail");
		
	TIMER_END(end);
	TIMER_DIFF(jcr->total_time,start,end);
	
    	printf("read_time:  %.3fs    %.2fMB/s\n", jcr->read_time,jcr->old_size*1.0/jcr->read_time/1036288); //1024*1024
	printf("chuk_time:  %.3fs    %.2fMB/s\n", jcr->chunk_time,jcr->old_size*1.0/jcr->chunk_time/1036288);
	printf("sha1_time:  %.3fs   %.2fMB/s\n", jcr->sha_time,jcr->old_size*1.0/jcr->sha_time/1036288);
	printf("srch_time:  %.3fs \n", jcr->search_time);
	printf("rad2_time:  %.3fs    %.2fMB/s\n", jcr->read2_time,jcr->dedup_size*1.0/jcr->read2_time/1036288); //1024*1024
	printf("send_time: %.3fs    %.2fMB/s\n", jcr->send_time,jcr->dedup_size*1.0/jcr->send_time/1036288);
	printf("total_time:  %.3fs    %.2fMB/s\n", jcr->total_time,jcr->old_size*1.0/jcr->total_time/1036288);
	

	printf("deduped_size/old_size  :            %ld/%ld   %.4f \n",jcr->dedup_size,jcr->old_size,jcr->dedup_size*1.0/jcr->old_size);
	printf("deduped_chunks/ old_chunks:   %d/%d    %.4f \n",jcr->dedup_chunk_count,jcr->chunk_count,jcr->dedup_chunk_count*1.00/jcr->chunk_count);
	printf("average chunk size: %.4f KB\n",jcr->old_size*1.0/jcr->chunk_count/1024.0);
	
	jcr_free( jcr);
	return 0;
}

int  walk_dir (JCR *psJcr, char *path) {
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
			if (walk_dir(psJcr, newpath) != 0) {
				return -1;
			}
		}
		//printf("*** out %s direcotry ***\n", path);
		closedir(dir);
	} 
	else if (S_ISREG(state.st_mode)) {
		send_file(psJcr, path);

	} else {
		puts("illegal file type! ignored!");
		return 0;
	}
	return 0;
}

int send_file(JCR *jcr, char *path) {

	if(G_NOT_DEDUP){
		send_file_data(jcr,path);
		return SUCCESS;
	}
	
	Recipe *rp=NULL;
	jcr->file_count++;
	TIMER_DECLARE(start,end);
	rp=recipe_new();
	rp->fileindex=jcr->file_count;
	memcpy(rp->filename,path,strlen(path));
	if(G_VERBOSE)
		printf("send file %s (%d)\n",rp->filename,jcr->file_count);
	chunk_file(jcr,rp); // 求指纹
	//recipe_check(rp);
	TIMER_START(start);
	send_finger( jcr,rp);//发送指纹
	recv_finger_rsp(jcr, rp);
	TIMER_END(end);
	TIMER_DIFF(jcr->search_time,start,end);
	send_data(jcr,rp); //发送数据
	return SUCCESS;

}
 void chunk_file(JCR *jcr,Recipe *rp){  
	TIMER_DECLARE(c_start,c_end); //分块时间
	TIMER_DECLARE(s_start,s_end);  // 求指纹时间
	TIMER_DECLARE(r_start,r_end);   // 读文件时间
	 int subFile;
          int32_t srclen=0, left_bytes = 0;
          int32_t size=0,len=0; 
          int32_t n = MAX_CHUNK_SIZE;

	 unsigned char * p;
	FingerChunk *fc;
         unsigned char * src = (unsigned char *)malloc(MAX_CHUNK_SIZE*2);	

         chunk_alg_init();
	 if(src == NULL) {
           	printf("%s,%d Memory allocation failed.\n",__FILE__,__LINE__);
		  return;
     	}

	 if ((subFile=open(rp->filename, O_RDONLY)) < 0) {
	           printf("%s,%d open file error!\n",__FILE__,__LINE__);
		  free((char*)src);
		  return;
	  }

	 while (1) 
	 {
	 	 TIMER_START(r_start);
	 	 if((srclen =  readn(subFile,src+left_bytes, MAX_CHUNK_SIZE)) <= 0)
		 	break;
	 	TIMER_END(r_end);
		TIMER_DIFF(jcr->read_time,r_start,r_end);
		jcr->old_size+=srclen;

		/*left 上次未分完的内容*/
		if (left_bytes > 0){ 
			srclen+= left_bytes;
			left_bytes = 0;
		} 

		 if(srclen<MIN_CHUNK_SIZE)
		 	break;  // 避免没必要的求指纹
		
		p = src;
		len=0;
		while (len < srclen) 
		{
          		n = srclen -len;
			TIMER_START(c_start);
			size=chunk_data(p, n);/*根据数据分块*/
			TIMER_END(c_end);
			TIMER_DIFF(jcr->chunk_time,c_start,c_end);
			if(n==size && n < MAX_CHUNK_SIZE)
			{ 	
				/*将未分完的数据拷贝到left中*/
          			memmove(src, src+len, n );
          			left_bytes = n;
                			break;
			}  
      			
			fc=fingerchunk_new();
			TIMER_START(s_start);
			chunk_finger(p,size,fc->fingerprint);/*对分得的块求取指纹*/
			TIMER_END(s_end);
			TIMER_DIFF(jcr->sha_time,s_start,s_end);
			fc->chunklen=size;
			recipe_append_fingerchunk(rp,fc);
			
			jcr->chunk_count++;
			p = p + size;
			len+=size;
		}
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
	 	fc=fingerchunk_new();
		TIMER_START(s_start);
		chunk_finger(p,len,fc->fingerprint);/*对分得的块求取指纹*/
		TIMER_END(s_end);
		TIMER_DIFF(jcr->sha_time,s_start,s_end);
		fc->chunklen=len;
		recipe_append_fingerchunk(rp,fc);
	 }	
	 close(subFile);
   	 free(src);
}

void send_finger(JCR *jcr,Recipe *rp){
	char * buf=NULL,*p=NULL;
	FingerChunk * fc;
	char stream[30]={0};
	buf=(char *)malloc(SOCKET_BUF_SIZE);
	snprintf(stream,30,"%d %d",rp->fileindex,FILE_NAME); // send file name
	bnet_send(jcr->data_socket,stream,strlen(stream));

	bnet_send(jcr->data_socket,rp->filename,strlen(rp->filename));
	bnet_signal(jcr->data_socket,NAME_END);
	
	snprintf(stream,30,"%d %d",rp->fileindex,FILE_FINGERPRINT);// send fine fingerprint
	bnet_send(jcr->data_socket,stream,strlen(stream));

	p=buf;
	 fc=rp->first;
	while(fc){
		memcpy(p,fc->fingerprint,sizeof(Fingerprint));
		p+=sizeof(Fingerprint);
		if((p-buf)>SOCKET_BUF_SIZE-sizeof(Fingerprint)){
			bnet_send(jcr->data_socket,buf,p-buf);
			p=buf;
		}
		fc=fc->next;
	}
	if(p-buf>0)
		bnet_send(jcr->data_socket,buf,p-buf);
	bnet_signal(jcr->data_socket,FINGER_END);
	free(buf);
}
void recv_finger_rsp(JCR *jcr,Recipe *rp){
	char buf[SOCKET_BUF_SIZE]={0};
	int len=0;
	int i=0;
	FingerChunk * fc=rp->first;
	while(bnet_recv(jcr->data_socket,buf,&len)>0){
		while(i<len && fc){
			fc->existed=buf[i];
			i++;
			fc=fc->next;
		}
		i=0;
	}
}


void send_data(JCR *jcr,Recipe *rp){
	char buf[SOCKET_BUF_SIZE+21]={0};
	FingerChunk *fc=NULL;
	char stream[30]={0};
	int64_t  len=0;
	int fd=-1;
	TIMER_DECLARE(s_start,s_end);
	TIMER_DECLARE(r_start,r_end);
	
	if ((fd=open(rp->filename, O_RDONLY)) < 0) {
		 printf("%s,%d open file error!\n",__FILE__,__LINE__);
		goto FAIL;
	 }
	lseek(fd,0,SEEK_SET);
	
	snprintf(stream,30,"%d %d",rp->fileindex,FILE_DATA); // send file data
	bnet_send(jcr->data_socket,stream,strlen(stream));
	for(fc=rp->first;fc;fc=fc->next){
		if(fc->existed=='0'){
			lseek(fd,len,SEEK_SET);
			memcpy(buf,fc->fingerprint,sizeof(Fingerprint));
			TIMER_START(r_start);
			if(readn(fd,buf+sizeof(Fingerprint),fc->chunklen)!=fc->chunklen)
				printf("%s %d read data chunklen is wrong \n",__FILE__,__LINE__);
			TIMER_END(r_end);
			TIMER_DIFF(jcr->read2_time,r_start,r_end);
			
			TIMER_START(s_start);
			 bnet_send(jcr->data_socket,buf,fc->chunklen+sizeof(Fingerprint));
			TIMER_END(s_end);
			TIMER_DIFF(jcr->send_time,s_start,s_end);
			jcr->dedup_chunk_count++;
			jcr->dedup_size+=fc->chunklen;
		}
		len+=fc->chunklen;
	}
	close(fd);	
FAIL:
	bnet_signal(jcr->data_socket,DATA_END);
}



/// 选择 -o  选项，选择非重删时候，才会执行 此函数
void send_file_data(JCR *jcr,char * filename){
	char buf[SOCKET_BUF_SIZE+21]={0};
	FingerChunk *fc=NULL;
	int64_t  totallen=0;
	int readlen=0;
	int fd=-1;
	TIMER_DECLARE(s_start,s_end);
	TIMER_DECLARE(r_start,r_end);

	bnet_send(jcr->data_socket,filename,strlen(filename)); // send file name
	if ((fd=open(filename, O_RDONLY)) < 0) {
		 printf("%s,%d open file error!\n",__FILE__,__LINE__);
		goto FAIL;
	 }
	lseek(fd,0,SEEK_SET);

	//send data
	while((readlen=readn(fd,buf+20,SOCKET_BUF_SIZE))>0){
		chunk_finger(buf+20,readlen,buf);
		bnet_send(jcr->data_socket,buf,readlen+20);
		jcr->chunk_count++;
		jcr->old_size+=readlen;
	}	
	close(fd);	
	jcr->file_count++;

FAIL:
	bnet_signal(jcr->data_socket,DATA_END);
	if(G_VERBOSE)
		printf("send file %s   number:%d \n",filename,jcr->file_count);
}

///===============================================================
//// 测试并发读小文件，结果发现，即使多线程读小文件，速度
///也不会提升多少，流水线中读文件成为了瓶颈，这将奈何?
/*
workq_t file_wq;
pthread_mutex_t mutext=PTHREAD_MUTEX_INITIALIZER;
int64_t file_total=0;
//// test
//// zhoupj
/// 2012/12/30
int  read_dir(void * arg){
	char * path=(char *)arg;
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
			//ignore . and .
			if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))//(entry->d_name[0]=='.')
				continue;
			strcpy(newpath, path);
			strcat(newpath, entry->d_name);
			read_dir( newpath) ;
		}
		//printf("*** out %s direcotry ***\n", path);
		closedir(dir);
	} 
	else if (S_ISREG(state.st_mode)) {
		char * filename=(char *)malloc(256);
		memcpy(filename,path,strlen(path)+1);
		workq_add(&file_wq,filename,NULL,0);
	} else {
		puts("illegal file type! ignored!");
		return 0;
	}
	return 0;
	
	
}
void * cur_read_file(void * arg){
	int subFile=-1;
	int srclen=0;
	char src[MAX_CHUNK_SIZE+1];
	printf("%s \n",(char *)arg);
	if ((subFile=open((char*)arg, O_RDONLY)) < 0) {
		err_msg1("open file error!");
		  return;
	 }
	 while((srclen =  readn(subFile,src, MAX_CHUNK_SIZE)) >0){
	 	P(mutext);
		file_total+=srclen;
		printf("%d  %ld \n",srclen,file_total);
		V(mutext);
	 }
	 close(subFile);
	 free(arg);
	 return NULL;
}
*/

//int main(int argc, char **argv){
/*	JCR * jcr=jcr_new();
	Recipe * rp=recipe_new();
	if(argc<2){
		printf("wrong, please input like  ./client filename\n");
		return -1;
	}
	memcpy(rp->filename,argv[1],strlen(argv[1]));
	
	chunk_file(jcr, rp);
	recipe_check(rp);
	printf("file len:%d\n",jcr->old_size);
	printf("read_time:  %.3fs    %.2fMB/s\n", jcr->read_time,jcr->old_size*1.0/jcr->read_time/1000000);
	printf("chuk_time:  %.3fs    %.2fMB/s\n", jcr->chunk_time,jcr->old_size*1.0/jcr->chunk_time/1000000);
	printf("sha1_time:  %.3fs   %.2fMB/s\n", jcr->sha_time,jcr->old_size*1.0/jcr->sha_time/1000000);
	recipe_free(rp);
	jcr_free(jcr);
*/	
/*	if(argc<2){
			printf("wrong, please input like  ./client dir \n");
			return -1;
	}

	double read_time=0;
	TIMER_DECLARE(start,end);
	workq_init(&file_wq,4,cur_read_file);
	TIMER_START(start);
	
	 read_dir(argv[1]);
	 workq_destroy(&file_wq);
	 
	TIMER_END(end);
	TIMER_DIFF(read_time,start,end);
	printf("read_time %.3fs  %ldB  %.3fMB/s \n",read_time,file_total,file_total*1.0/read_time/1036288);
	
}
*/


