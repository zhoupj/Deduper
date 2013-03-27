/*
 * job.c
 *
 *
 */

#include "../global.h"


int jobv_read_head(JOB_V* psVolume);
int jobv_write_head(JOB_V* psVolume);
JOB_V* jobv_init();


static char job_count_file[256];
static int g_job_count=0;
pthread_mutex_t g_mutex=PTHREAD_MUTEX_INITIALIZER;

int jobcount_init() {
	strcpy(job_count_file,BackupVolPath);
	strcat(job_count_file,"job_count");
	 int fd;
	  if ((fd = open(job_count_file, O_CREAT | O_RDWR, 0644)) <= 0) {
	        printf("Can not open job_count_file\n");
	        return FAILURE;
	 }
	  
    if (read(fd, &g_job_count, 4) == 4) {
      	printf("%s,%d total job counts:%d\n",__FILE__,__LINE__,g_job_count);
    } else {
        g_job_count = 0;
        lseek(fd, 0, SEEK_SET);
        write(fd, &g_job_count, 4);
    }
    close(fd);
    return SUCCESS;
}
int  jobcount_close(){
    int fd;
    if ((fd = open(job_count_file, O_RDWR)) <= 0) {
        printf("Can not write job_count_file\n");
        return FAILURE;
    }
    write(fd, &g_job_count, 4);
    close(fd);
    return SUCCESS;
}
int jobcount_max(){
	return g_job_count;
}


JOB_V* jobv_init(){
	JOB_V *jv=(JOB_V*)malloc(sizeof(JOB_V));
	jv->fd=-1;
	jv->isDel=0;
	jv->nFileNum=0;
	jv->nJobId=0;
	//jv->readoffset=0;
	//jv->writeoffset=0;
	memset(jv->szBackupPath,0, FILE_NAME_LEN);
	return jv;
}

JOB_V *jobv_new(char * pathname){  /* for backup */
	JOB_V *jv=jobv_init();
	char path[256]={0};
	char str[50]={0};
	strcpy(path,BackupVolPath);
	P(g_mutex);
	jv->nJobId=++g_job_count;
	V(g_mutex);
	sprintf(str,"job_%d",g_job_count);
	strcat(path,str);
	printf("new job_v:%s\n",path);
	if ((jv->fd = open(path, O_CREAT | O_TRUNC | O_RDWR, 0644)) <= 0) {
       		err_msg2("can not creat  job_%d",jv->nJobId);
       	         jobv_destroy(jv);
		//g_job_count--;
        		return NULL;
        }
	memcpy(jv->szBackupPath,pathname,strlen(pathname));
	jobv_write_head( jv);
	return jv;
}
JOB_V *jobv_open(int id){ /* for restore */
	JOB_V *jv=jobv_init();
	char path[256]={0};
	char str[50]={0};
	strcpy(path,BackupVolPath);
	sprintf(str,"job_%d",id);
	strcat(path,str);
	printf("read  job_v:%s\n",path);
	if ((jv->fd = open(path,  O_RDWR)) <= 0) {
		err_msg2("can not open job_%d ",id);
       	         jobv_destroy(jv);
        		return NULL;
        }
	jobv_read_head(jv);
	printf("%s,%d total file number:%d\n",__FILE__,__LINE__,jv->nFileNum);
	return jv;
}
int jobv_write_head(JOB_V* jv) {
    lseek(jv->fd, 0, SEEK_SET);
    write(jv->fd, &jv->nJobId, 4);
    write(jv->fd, &jv->isDel, 4);
    write(jv->fd, jv->szBackupPath, FILE_NAME_LEN);
    write(jv->fd, &jv->nFileNum, 4);
    return SUCCESS;
}

int jobv_read_head(JOB_V* psVolume) {
    lseek(psVolume->fd, 0, SEEK_SET);
    read(psVolume->fd, &psVolume->nJobId, 4);
    read(psVolume->fd, &psVolume->isDel, 4);
    read(psVolume->fd, psVolume->szBackupPath, FILE_NAME_LEN);
    read(psVolume->fd, &psVolume->nFileNum, 4);
    return SUCCESS;
}

void jobv_insert_recipe(JOB_V * jv,Recipe *rp){
	char *buf=NULL,*p;
	buf=(char *)calloc(1,4+4+FILE_NAME_LEN+rp->chunknum*32+10);
	p=buf;
	memcpy(p,&rp->fileindex,sizeof(int));p+=4;
	memcpy(p,&rp->chunknum,sizeof(int));p+=4;
	memcpy(p,rp->filename,FILE_NAME_LEN);p+=FILE_NAME_LEN;
	FingerChunk *fc;
	fc=rp->first;
	while(fc){
		memcpy(p,fc->fingerprint,sizeof(Fingerprint));p+=sizeof(Fingerprint);
		memcpy(p,&fc->offset,8);p+=8;
		memcpy(p,&fc->length,4);p+=4;
		fc=fc->next;
	}
	writen(jv->fd,buf,(p-buf));
	free(buf);
	jv->nFileNum++;
	
}
Recipe * jobv_search_next_recipe(JOB_V*jv){
	char *buf=NULL,*p=NULL;
	int i=0;
	Recipe *rp=NULL;
	int chunknum=0;
	rp=recipe_new();
	if(!rp)
		return NULL;
	if(read(jv->fd,&rp->fileindex,4)!=4)
		return NULL;
	read(jv->fd,&chunknum,4);
	buf=(char *)malloc(FILE_NAME_LEN+chunknum*32);
	p=buf;
	readn(jv->fd,buf,FILE_NAME_LEN+chunknum*32);
	memcpy(rp->filename,p,FILE_NAME_LEN);p+=FILE_NAME_LEN;
	for(i=0;i<chunknum;i++){
		FingerChunk *fc=(FingerChunk *)malloc(sizeof(FingerChunk));
		fc->existed=0; /* not used for restoring */
		memcpy(fc->fingerprint,p,sizeof(Fingerprint));p+=sizeof(Fingerprint);
		memcpy(&fc->offset,p,8);p+=8;
		memcpy(&fc->length,p,4);p+=4;
		recipe_append_fingerchunk(rp,fc);
		
	}
	if(rp->chunknum!=chunknum)
		err_msg1("chunk numbers are not matched \n");
	free(buf);
	return rp;
}

void jobv_destroy(JOB_V *jv)
{
	jobv_write_head(jv);
	if(jv->fd>0)
		close(jv->fd);
	jv->fd=-1;
	free(jv);
}

//#define JOB_TEST
#ifdef JOB_TEST

#define TOTAL 1000000
#define BUF_LEN 200
int main(){
	int i;
        char hash[20]={0};
        char buf[BUF_LEN]={0};
	Recipe *rp=NULL;
	FingerChunk *fc=NULL;
	int ID;
	
	///============================write
	
	printf("===============write==============\n");

	jobcount_init(); // global initialization
	JOB_V * job=jobv_new("/home/zpj/abc");
	ID=job->nJobId;
	if(!job)
		return;
	rp=recipe_new();
	memcpy(rp->filename,"root/zpj/bac/0",strlen("root/zpj/bac/0"));
	for(i=1;i<TOTAL;i++){
		memset(buf,0,BUF_LEN);
		sprintf(buf,"/root/zpj/dir%d/abc/job_%d",i,i);
		
		
		finger_chunk(buf,strlen(buf),hash);
		fc=fingerchunk_new(hash,0);
		recipe_append_fingerchunk(rp,fc);
		if(i%10000==0){
			rp->fileindex=i/100;
			memcpy(rp->filename,buf,strlen(buf));
			jobv_insert_recipe(job, rp);
			recipe_free(rp);
			rp=recipe_new();
			printf("new recipe\n");
			
		}	
	}
	if(rp && rp->chunknum>0){
		rp->fileindex=i/100+1;
		memcpy(rp->filename,buf,strlen(buf));
		jobv_insert_recipe(job, rp);
		recipe_free(rp);	
	}
	jobv_destroy(job);
	jobcount_close();

	
	///r============================================read
	printf("===============read===============\n");
	jobcount_init(); // global initialization
	job=jobv_open(ID);
	if(!job)
		return;
	while((rp=jobv_search_next_recipe(job))){
		printf("%d  %d  %s\n",rp->fileindex,rp->chunknum,rp->filename);
		recipe_free(rp);
     
	}
	jobv_destroy(job);
	jobcount_close();
}

#endif
