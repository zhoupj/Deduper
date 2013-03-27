#include "global.h"
void local_simulate_restore(int jobid){
		JCR *jcr=NULL;
		Recipe *rp=NULL;
		FingerChunk *fc=NULL;
		Chunk *chunk=NULL;
		TIMER_DECLARE(start,end);
		double restore_time=0;
		//char stream[30]={0};
		G_R_READNUM=0;
		
		container_vol_init();
		jcr=jcr_new();
	
		jcr->memIndex=index_init_r();
		jcr->nJobId=jobid;
		
	
		jcr->jobv=jobv_open(jcr->nJobId);

		if(jcr->jobv==NULL){
			goto FAIL;
		}

		printf("%s,%d restore jobid:%d \n", __FILE__,__LINE__,jcr->nJobId);
		while((rp=jobv_search_next_recipe(jcr->jobv))){
			
			if(G_VERBOSE)
			  	printf("send file %s \n",rp->filename);
			for(fc=rp->first;fc;fc=fc->next){
				TIMER_START(start);
				chunk=index_search_r(jcr->memIndex,fc->fingerprint);	
				jcr->nSize+=chunk->length;
				TIMER_END(end);
				TIMER_DIFF(restore_time,start,end);
				
				if(chunk){
					chunk_free(chunk);
				}
				else
					err_msg1("not found");
			}
			recipe_free(rp);
		}
		
		printf("read chunk speed : %20.4f MB/s \n",jcr->nSize*1.0/restore_time/(1024*1024));
		double speed=G_R_READNUM*1.0 /(jcr->nSize/(1024*1024));
	       
		char str[30]={0};
		snprintf(str,30,"%f\t",speed);
		if(G_FILE_FD){
			write(G_FILE_FD,str,strlen(str));
		}
		
		printf("read containere number:%d , %20.4f /MB \n",G_R_READNUM, speed);
	
	jobv_destroy(jcr->jobv);
		FAIL:
		index_destroy_r(jcr->memIndex);
		jcr_free(jcr);
		container_vol_destroy();

}
void restore(int fd,char *msg){
		JCR *jcr=NULL;
		Recipe *rp=NULL;
		FingerChunk *fc=NULL;
		Chunk *chunk=NULL;
		TIMER_DECLARE(start,end);
		double restore_time=0;
		//char stream[30]={0};
		G_R_READNUM=0;
		
		container_vol_init();
		jcr=jcr_new();
	
		jcr->dataSocket=fd;
		jcr->memIndex=index_init_r();
		
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
			//memset(stream,0,30);
			
			//snprintf(stream,30,"%d %d",rp->fileindex,FILE_NAME);
			//bnet_send(fd,stream,strlen(stream));// stream type
			if(G_VERBOSE)
			  	printf("send file %s \n",rp->filename);
			bnet_send(fd,rp->filename,strlen(rp->filename));
			//bnet_signal(fd,NAME_END);

			//send data
			//snprintf(stream,30,"%d %d",rp->fileindex,FILE_DATA);
			//bnet_send(fd,stream,strlen(stream));// stream type
			
			for(fc=rp->first;fc;fc=fc->next){
				TIMER_START(start);
				chunk=index_search_r(jcr->memIndex,fc->fingerprint);	
				jcr->nSize+=chunk->length;
				TIMER_END(end);
				TIMER_DIFF(restore_time,start,end);
				
				if(chunk){
					//模拟恢复不需要整正的恢复即可
					bnet_send(fd,chunk->data,chunk->length);
					chunk_free(chunk);
				}
				else
					err_msg1("not found");
			}
			
			bnet_signal(fd,DATA_END);
			recipe_free(rp);
			
		}
	
		
		printf("read chunk speed : %20.4f MB/s \n",jcr->nSize*1.0/restore_time/(1024*1024));
		double speed=G_R_READNUM*1.0 /(jcr->nSize/(1024*1024));
		printf("read containere number:%d , %20.4f /MB \n",G_R_READNUM, speed);
		
	FAIL:
		bnet_signal(fd,STREAM_END); /* over */
		jobv_destroy(jcr->jobv);
		index_destroy_r(jcr->memIndex);
		jcr_free(jcr);
		container_vol_destroy();
}


///test
/// Date:2012-12-31
/// Author:zhoupj
//#define RESTORE_TEST
#ifdef RESTORE_TEST
int main(){
	JOB_V *job;
	Recipe *rp;
	FingerChunk *fc;
	Chunk *chunk;
	JCR *jcr;
	container_vol_init();
	jcr=jcr_new();
	jcr->memIndex=index_init();
	job=jobv_open(1);
	if(!job)
		return;
	printf("@job information@ %d,%d,%s\n",job->nJobId, job->nFileNum,job->szBackupPath);
	while((rp=jobv_search_next_recipe(job))){
			printf("%10d % 10d %s \n",rp->fileindex,rp->chunknum,rp->filename);
			for(fc=rp->first;fc;fc=fc->next){
				if(jcr->container==NULL)
					jcr->container=index_search(jcr->memIndex,fc->fingerprint);
				while((chunk=read_chunk(jcr->container,fc->fingerprint))==NULL)
					jcr->container=index_search(jcr->memIndex,fc->fingerprint);
				if(chunk){

					printf("chunk length:%d\n",chunk->length);
					chunk_free(chunk);
				}
			}
			recipe_free(rp);
			
	}
	jobv_destroy(job);
	index_destroy(jcr->memIndex);
	jcr_free(jcr);
	container_vol_destroy();
}
#endif
