#include "global.h"
//2013.1.15
// not deduplication 
// zhou peng ju
static bool check_data(char hash[20],char* buf,int buflen){
    uint8_t tmphash[20];
    SHA_CTX ctx;
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, buf, buflen);
    SHA1_Final(tmphash,&ctx);
    if(memcmp(tmphash, hash, 20)!=0){
        err_msg1("data has been corrupted!");
        return false;
    }
    return true;
}



void backup_formal(int fd,char *msg){
	JCR *jcr=NULL;
	char fileset[256]={0};
	char *buf=(char *)calloc(1,SOCKET_BUF_SIZE+21);
	int len;

	int index=1;
	char vol_name[FILE_NAME_LEN];
	int vol_fd;
	Recipe *rp=NULL;
	FingerChunk *fc=NULL;
	char *p=NULL;
	int64_t rwlen=0;
	
	jobcount_init();
	jcr=jcr_new();
	jcr->dataSocket=fd;

	memset(vol_name,0,FILE_NAME_LEN);
	strcpy(vol_name,BackupVolPath);
	strcat(vol_name,"data_vol");
	vol_fd=open(vol_name,O_RDWR| O_CREAT,00644);
	if(vol_fd<0){
		err_msg1("can't open file");
		goto FAIL;
	}
	printf("%s %d vol_name:%s\n",__FILE__,__LINE__,vol_name);
        rwlen=lseek(vol_fd,0,SEEK_END);
	
	TIMER_DECLARE(gstart,gend);
	TIMER_DECLARE(wstart,wend);
	
	TIMER_START(gstart);
	if(sscanf(msg,backup_cmd,fileset)!=1){ // backup cmd
		goto FAIL;
	}
	jcr->jobv=jobv_new(fileset);
	jcr->nJobId=jcr->jobv->nJobId;
	
	printf("===========backup start==============\n");
	printf("%s,%d pathname:%s \n", __FILE__,__LINE__,fileset);

	
	while(bnet_recv(jcr->dataSocket,buf,&len)!=ERROR){ //文件名
		if(len==STREAM_END){
			printf("%s %d backup is over\n",__FILE__,__LINE__);
			break;
		}
		
		//printf("\033[40;32m recv file: %s (%d) \033[0m\n",buf,len);
		rp=recipe_new();
		memcpy(rp->filename,buf,len);
		rp->fileindex=index++;	
		
		while(bnet_recv(jcr->dataSocket,buf,&len)>0){ /*format: fingerprintf data data dta..*/
					//printf("\033[40;32m recv: file data (%d) \033[0m\n",len);	
			fc=fingerchunk_new(buf,0);
			fc->offset=rwlen;
			fc->length=len-sizeof(Fingerprint);

			check_data(fc->fingerprint,buf+sizeof(Fingerprint),fc->length);
					
			TIMER_START(wstart);
			 if(writen(vol_fd,buf+sizeof(Fingerprint),fc->length)!=fc->length)
					err_msg1("wrintn wrong");		 
			TIMER_END(wend);
			TIMER_DIFF(jcr->writeDataTime,wstart,wend);
			
			rwlen+=fc->length;		
			jcr->nChunkCount++;
			jcr->nSize+=fc->length;
			recipe_append_fingerchunk(rp,fc);
		}
				
		jcr->nFileCount++;
		if(G_VERBOSE)
			printf("receive file %s OK, total: %d\n",rp->filename,jcr->nFileCount);
		jobv_insert_recipe(jcr->jobv, rp);
		rp=NULL;	
	}
FAIL:	
	bnet_send(fd,"OK",2);  // 发送备份成功信息
	
	TIMER_END(gend);
	TIMER_DIFF(jcr->recvTime,gstart,gend);
	
	
	printf("============back over===============\n");
	printf("total time:%.4f   %.4f MB/s\n",jcr->recvTime,jcr->nSize*1.0/jcr->recvTime/1036288.0);
	printf("write time:%.4f  %.4f MB/s\n",jcr->writeDataTime,jcr->nSize*1.0/jcr->writeDataTime/1036288.0);
	printf("chunk count:%d\n",jcr->nChunkCount);
	printf("file count:%d\n",jcr->nFileCount);
	
	if(rp){
		recipe_free(rp);
	}
	
	jobv_destroy(jcr->jobv);
	jcr_free(jcr);
	jobcount_close();
	
	close(vol_fd);
}


void restore_formal(int fd,char *msg){
		JCR *jcr=NULL;
		Recipe *rp=NULL;
		FingerChunk *fc=NULL;
		char *buf=(char *)calloc(1,SOCKET_BUF_SIZE+21);
		char vol_name[FILE_NAME_LEN];
		int vol_fd;
		//char stream[30]={0};
		
		memset(vol_name,0,FILE_NAME_LEN);
		strcpy(vol_name,BackupVolPath);
		strcat(vol_name,"data_vol");
		vol_fd=open(vol_name,O_RDWR| O_CREAT,00644);

		jcr=jcr_new();
		jcr->dataSocket=fd;

		if(vol_fd<0){
			err_msg1("can't open file");
			goto FAIL;

		}
		
		if(sscanf(msg,restore_cmd,&jcr->nJobId)!=1){ // backup cmd
			goto FAIL;
		}
		jcr->jobv=jobv_open(jcr->nJobId);

		if(jcr->jobv==NULL){
			goto FAIL;
		}

		printf("%s,%d restore jobid:%d \n", __FILE__,__LINE__,jcr->nJobId);

		// send pathname
		bnet_send(fd,jcr->jobv->szBackupPath,strlen(jcr->jobv->szBackupPath));
		
		
		while((rp=jobv_search_next_recipe(jcr->jobv))){

			//send file name
		
			if(G_VERBOSE)
			  	printf("send file %s \n",rp->filename);
			bnet_send(fd,rp->filename,strlen(rp->filename));
			
			//send data
			
			for(fc=rp->first;fc;fc=fc->next){
				lseek(vol_fd,fc->offset,SEEK_SET);
				if(readn(vol_fd,buf,fc->length)!=fc->length)
					err_msg1("readn wrong");
				check_data(fc->fingerprint,buf,fc->length);
				bnet_send(fd,buf,fc->length);
			}
			bnet_signal(fd,DATA_END);

			recipe_free(rp);
		}
	FAIL:
		bnet_signal(fd,STREAM_END); /* over */
		jobv_destroy(jcr->jobv);
		jcr_free(jcr);
		
}
