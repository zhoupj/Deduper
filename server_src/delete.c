#include "global.h"

void sanitization(){ //static erasing (assuming no job is running)

		Recipe *rp=NULL;
		FingerChunk *fc=NULL;
		
		Container *ca=NULL;
		ChunkAddr *cdr=NULL;
		Chunk *chunk=NULL;
		TIMER_DECLARE(start,end);
		TIMER_DECLARE(b_start,b_end);
		TIMER_DECLARE(m_start,m_end);
		TIMER_DECLARE(s_start,s_end);
		TIMER_DECLARE(c_start,c_end);
		double g_time=0;
		double b_time=0;
		double m_time=0;
		double s_time=0;
		double c_time=0;

		int container_min=2147483647;
		int container_max=0;

		//char stream[30]={0};
		int delete_num=0;
		int64_t delete_size=0;
		//
		int i;
		ContainerAddr *caddr;
		MemIndex * Gindex=NULL;
		JOB_V *jobv;
		container_vol_init();
		jobcount_init();
		PHF *phf;
		

	      TIMER_START(start);

		// 1. read disk index
		Gindex=index_init();
		if(Gindex->index_table->num_items<=0)
			goto FAIL;
		
		phf=phf_init(Gindex->index_table->num_items);
		
		// 2. build phf
		TIMER_START(b_start);
		foreach_htable(caddr,Gindex->index_table){
			phf_input(phf,caddr->hash);
		}
		phf_walk(phf);
		phf_build(phf);
		
		TIMER_END(b_end);
		TIMER_DIFF(b_time,b_start,b_end);

		TIMER_START(m_start);
		
		// 3. mark all live files
		for(i=1;i<=jobcount_max();i++){
			jobv=jobv_open(i);
			if(jobv){
				if(!jobv->isDel){
					while((rp=jobv_search_next_recipe(jobv))){
						if(G_VERBOSE)
				  			printf("track file %s \n",rp->filename);
						for(fc=rp->first;fc;fc=fc->next){
							phf_mark(phf,fc->fingerprint);
						}
						recipe_free(rp);
					}
				}
				else if(jobv->isDel==1){
					while((rp=jobv_search_next_recipe(jobv))){
						if(G_VERBOSE)
				  			printf("track deleted file %s \n",rp->filename);
						for(fc=rp->first;fc;fc=fc->next){
							caddr=(ContainerAddr *)htable_lookup(Gindex->index_table,fc->fingerprint);
							if(!caddr){
								err_msg1("maybe wrong");
								continue;
							}
							if(container_min>caddr->container_id)
								container_min=caddr->container_id;
							if(container_max<caddr->container_id)
								container_max=caddr->container_id;
						}
						recipe_free(rp);
					}
					jobv->isDel=2;// 此时，要真正删除作业
				}
				jobv_destroy(jobv);
			}
			
		}
		TIMER_END(m_end);
		TIMER_DIFF(m_time,m_start,m_end);
		
		int dead=0;
		int max_container_num=container_vol_max_num();
		// 4 , track all containers
		if(container_min<container_max)
			printf("****** scan container id from %d to %d \n",container_min,container_max);
		else
			printf("******no job is  deleted \n");
		for(i=container_min;i<=container_max;i++){
			TIMER_START(s_start);
			ca=read_container(i);
			if(!ca)
				continue;
			dead=0;
			foreach_htable(cdr,ca->meta_table){ // track
				if(!phf_search(phf,cdr->hash)){
					cdr->container_id=-1;
					dead++;
					delete_num++;
					delete_size+=cdr->length;
				}
			}
			TIMER_END(s_end);
			TIMER_DIFF(s_time,s_start,s_end);
			TIMER_START(c_start);
			//copy
			if(dead==ca->chunk_num){ // fully dead
				container_free(ca->container_id);
				foreach_htable(cdr,ca->meta_table){
					htable_remove(Gindex->index_table,cdr->hash);
				}
			}
			else if(dead>0){
				//printf("dead\n");
				Container * newca=container_init();
				foreach_htable(cdr,ca->meta_table){
					if(cdr->container_id>0){ // live for copy into new container
						  chunk =chunk_new( cdr->hash,ca->data_buffer+cdr->offset,cdr->length);
						     if(*(ca->data_buffer+cdr->offset+chunk->length) != '\t')
				      			err_msg1("container has been corrupted.");
						 if( write_chunk(newca,chunk)==false)
						 	err_msg1(" write chunk into new container  wrong");
					}
					else{// move it from G_index;
						htable_remove(Gindex->index_table,cdr->hash);
					}
				}
 				newca->container_id=ca->container_id;
				write_container(newca);
				container_usage(newca);//show result
				container_destroy(newca);
			}
			else {
				//container_usage(ca);//
			}
			TIMER_END(c_end);
			TIMER_DIFF(c_time,c_start,c_end);
			TIMER_START(s_start);
			 container_destroy(ca);
			TIMER_END(s_end);
			TIMER_DIFF(s_time,s_start,s_end);
		}
		phf_destory( phf);
	FAIL:	
		index_destroy(Gindex);
		TIMER_END(end);
		TIMER_DIFF(g_time,start,end);
		jobcount_close();
		container_vol_destroy();
		printf("delete nume:%d \n",delete_num);
		printf("delete size:%d \n",delete_size);
		printf("%10.3f \n%10.3f \n%10.3f \n%10.3f \n%10.3f \n",g_time,b_time,m_time,s_time,c_time);

}
void delete_job(int fd,char *msg){
			int deleted=0;
			JCR *jcr=NULL;
			jcr=jcr_new();
			char buf[50]={0};
			strcpy(buf,"OK");
			/*char *message;
			char *p;
			message=msg+strlen(delete_cmd)-2;
			p=strtok(message,",");
			while(p){


			}*/
			
			if(sscanf(msg,delete_cmd,&jcr->nJobId)!=1){ // backup cmd
				err_msg1("delete cmd wrong");
				return;
			}
			printf("delete job %d \n",jcr->nJobId);
			jcr->jobv=jobv_open(jcr->nJobId);
	
			if(jcr->jobv){
				if(jcr->jobv->isDel)
					strcpy(buf,"This job has been deleted before");
				else
					jcr->jobv->isDel=1;
			}
			jobv_destroy(jcr->jobv);
			bnet_send(fd,buf,strlen(buf));
			jcr_free(jcr);
			
}
void local_delete_job(int jobid){
			int deleted=0;
			JCR *jcr=NULL;
			
			char buf[50]={0};
			strcpy(buf,"sorry wrong");
			
			jcr=jcr_new();
			if(jobid<=0){ // backup cmd
				err_msg1("delete cmd wrong");
				return;
			}
			printf("delete job %d \n",jobid);
			jcr->jobv=jobv_open(jobid);
	
			if(jcr->jobv){
				if(jcr->jobv->isDel)
					strcpy(buf,"This job has been deleted before");
				else {
					jcr->jobv->isDel=1;
					strcpy(buf,"OK deleted it");
				}
			}
			
			jobv_destroy(jcr->jobv);
			jcr_free(jcr);
			printf("========%s \n",buf);
			
}

