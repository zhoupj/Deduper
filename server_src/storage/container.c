#include "../global.h"

extern  char BackupVolPath[];

static const int32_t cvol_magic_key = 0x7856ffee;
 const int32_t container_valid = 0xf5f5e4e4;

struct container_volume * g_cvol;

Chunk * chunk_new(Fingerprint hash,unsigned char *data,int datalen){
	Chunk* ck=(Chunk *)malloc(sizeof(Chunk));
	memcpy(ck->hash,hash,sizeof(Fingerprint));
	ck->length=datalen;
	ck->data=(unsigned char*)malloc(datalen+1);
	memcpy(ck->data,data,datalen);
	ck->data[datalen]=0;

	return ck;
}
void chunk_free(Chunk *ck){
	if(ck->data)
		free(ck->data);
	free(ck);
}
ChunkAddr * chunkaddr_new(Fingerprint hash,int offset,int length){
	ChunkAddr *ca=(ChunkAddr*)malloc(sizeof(ChunkAddr));
	ca->length=length;
	ca->offset=offset;
	memcpy(ca->hash,hash,sizeof(Fingerprint));
	return ca;
}	


void chunkaddr_free(ChunkAddr *ca){
	free(ca);
}

Container *container_init(){
    Container *container = (Container*)malloc(sizeof(Container));
    container->container_id = -1;
    container->data_size = 0;
    container->chunk_num = 0;
  
    container->data_buffer = (unsigned char*)malloc(DEFAULT_CONTAINER_SIZE);
    ChunkAddr tmp;
    container->meta_table = htable_init((char*)&tmp.next - (char*)&tmp,sizeof(Fingerprint),512);
    return container;
}

void container_destroy(Container *container){
    if(!container)
        return;
     htable_destroy(container->meta_table);
    if(container->data_buffer)
        free(container->data_buffer);
    free(container);
}

bool container_vol_init(){ // invalid container_num free_numer
    g_cvol = (struct container_volume*)malloc(sizeof(struct container_volume));
    g_cvol->container_num = 0;
    g_cvol->file_descriptor = -1;
   g_cvol->free_fd=-1;
    g_cvol->freequeue=queue_new();
   int num=0;
   int * free_num;

    strcpy(g_cvol->volume_path, BackupVolPath);
    strcat(g_cvol->volume_path, "container_volume");
     strcpy(g_cvol->free_vol_path, BackupVolPath);
    strcat(g_cvol->free_vol_path, "free_container_record");
    printf("%s, %d, volume name=%s\n",__FILE__,__LINE__,g_cvol->volume_path);
    if(access(g_cvol->free_vol_path, 0) != 0){ // not existed
	   ;
		if((g_cvol->free_fd = open(g_cvol->free_vol_path, O_RDWR | O_CREAT, S_IREAD|S_IWRITE))<0){
		        printf("%s, %d, Can not create free_container !\n",__FILE__,__LINE__);
		        return false;
   		 }
		write(g_cvol->free_fd, &num,4);
		printf("%s, %d,  create free_container_record success !\n",__FILE__,__LINE__);
		
  }
   else{
   	if((g_cvol->free_fd = open(g_cvol->free_vol_path, O_RDWR))<0){
	            printf("%s, %d, Can not open free_container volume!\n",__FILE__,__LINE__);
	            return false;
	     }
   	
	  read(g_cvol->free_fd, &num, 4);
	   printf("%s,%d   free container number:%d \n",__FILE__,__LINE__,num);
	   int n=num;
	   while(n){
	   	free_num=(int*)malloc(4);
		 read(g_cvol->free_fd,free_num, 4);
		 queue_push(g_cvol->freequeue,free_num);
		 n--;
	   }
	   if(num!=g_cvol->freequeue->elem_num)
	   	err_msg1("free container num wrong");
	
   }

    if(access(g_cvol->volume_path, 0) != 0){ // not existed
	        printf("%s, %d: container volume does not exist!\n",__FILE__,__LINE__);
		if((g_cvol->file_descriptor = open(g_cvol->volume_path, O_RDWR | O_CREAT, S_IREAD|S_IWRITE))<0){
		        printf("%s, %d, Can not create container volume!\n",__FILE__,__LINE__);
		        return false;
   		 }
		write(g_cvol->file_descriptor, &cvol_magic_key, 4);
        		write(g_cvol->file_descriptor, &g_cvol->container_num, 4);
		printf("%s, %d,  create container volume success !\n",__FILE__,__LINE__);
		
  }
   else{
   	if((g_cvol->file_descriptor = open(g_cvol->volume_path, O_RDWR))<0){
	            printf("%s, %d, Can not open container volume!\n",__FILE__,__LINE__);
	            return false;
	     }
   	    int32_t key;
	    if(read(g_cvol->file_descriptor, &key, 4)!=4 || key!=cvol_magic_key){
	        printf("%s, %d, An invalid container volume!\n",__FILE__,__LINE__);  
	    }
	    read(g_cvol->file_descriptor, &g_cvol->container_num, 4);
	    printf("%s,%d container number:%d \n",__FILE__,__LINE__,g_cvol->container_num);

   }
   
    pthread_mutex_init(&g_cvol->mutex, 0);
    return true;

}
int container_vol_max_num(){
	return g_cvol->container_num;
}
void container_vol_update(){
   P(g_cvol->mutex);
   int * num;
    lseek(g_cvol->file_descriptor, 4, SEEK_SET);
    write(g_cvol->file_descriptor, &g_cvol->container_num, 4);

  lseek(g_cvol->free_fd, 0, SEEK_SET);
   write(g_cvol->free_fd, &g_cvol->freequeue->elem_num, 4);
    while(g_cvol->freequeue->elem_num>0){
	num=(int *)queue_pop(g_cvol->freequeue);
	//printf(" freed container id:%d \n",*num);
	 write(g_cvol->free_fd, num, 4);
	free(num);
    }
	
    V(g_cvol->mutex);
}

void container_vol_destroy(){
   container_vol_update();
   queue_free(g_cvol->freequeue);
    pthread_mutex_destroy(&g_cvol->mutex);
    close(g_cvol->file_descriptor);
   close(g_cvol->free_fd);
    free(g_cvol);
}
void container_free(int id){ // if a container if free ,free it.
	int * num;
	int len;
	int valid=0;
	
         P(g_cvol->mutex);
	  len= 8+(id-1)*DEFAULT_CONTAINER_SIZE;
	   lseek(g_cvol->file_descriptor, len,SEEK_SET);
	 if(write(g_cvol->file_descriptor, &valid, 4)!=4){ // or ,you can set 0 bits for DEFAULT_CONTAINER_SIZ
     	 	 err_msg1("failed to free container");
      		 V(g_cvol->mutex);
       	 	return ;
   	 }
	printf("free container id %d\n",id);
		
 	 num=(int*)malloc(4);
	 *num=id;
	 queue_push(g_cvol->freequeue,num);
       V(g_cvol->mutex);
}
int container_id_new(){
	int id;
	int *num;

	P(g_cvol->mutex);
	 if(g_cvol->freequeue->elem_num>0){
		num=(int*)queue_pop(g_cvol->freequeue);
		id=*num;
		free(num);
   	 }
	else
		 id= ++g_cvol->container_num;
	 V(g_cvol->mutex);
	// printf("%s,%d new container id:%d\n",__FILE__,__LINE__,id);
	 return  id; //return locality *wrong
}
Container * container_open_last(){ // 打开上次作业最后一个contaienr
	Container *ca=NULL;
	ca=read_container(g_cvol->container_num);
	if(ca && container_usage(ca)<0.80)
		return ca;
	else
		return container_new();
}
Container *container_new(){
 
	int id;
	int *num;
	P(g_cvol->mutex);
	 if(g_cvol->freequeue->elem_num>0){
		num=queue_pop(g_cvol->freequeue);
		id=*num;
		free(num);
   	 }
	else
		 id= ++g_cvol->container_num;
	 V(g_cvol->mutex);
	 
      Container *container = container_init();
       container->container_id = id;
      return container;
}
double container_usage(Container *ca){
	double effect_ratio=0;
	effect_ratio=(ca->chunk_num*16+28+ca->data_size)*1.0/DEFAULT_CONTAINER_SIZE;
	printf("container %5d  usage: %3f\n",ca->container_id,effect_ratio);
	return (ca->chunk_num*16+28+ca->data_size)*1.0/DEFAULT_CONTAINER_SIZE;
}

/*
 * Append a new container to volume,
 * return container id.
 */
int32_t write_container(Container *container){
    if(container->chunk_num == 0){
        printf("%s, %d: append a null container\n", __FILE__,__LINE__);
        return -1;
    }
  
    char *buffer = (char*)malloc(DEFAULT_CONTAINER_SIZE+1);
    char *p=buffer;
    ChunkAddr *addr=NULL;
     int n=0;
     int diff=0;
    off_t len=0;
   memcpy(p,&container_valid,4);p+=4;
   memcpy(p,&container->container_id,4);p+=4;
   memcpy(p,&container->data_size,4);p+=4;
   memcpy(p,&container->chunk_num,4);p+=4;
   
   // printf("write new container id=%d\n",container->container_id);

  foreach_htable(addr,container->meta_table){
	memcpy(p,&addr->offset,4);p+=4;
	memcpy(p,&addr->length,4);p+=4;
	memcpy(p,addr->hash,sizeof(Fingerprint));p+=sizeof(Fingerprint);
       	n++;
    }
   if(n!=container->chunk_num || *(int *)(buffer+4)!=container->container_id)
   	err_msg1("write container is wrong");

    memcpy(p,container->data_buffer,container->data_size);p+=container->data_size;
   diff=(p-buffer);
    if(diff<DEFAULT_CONTAINER_SIZE){
        memset(p, 0xff, DEFAULT_CONTAINER_SIZE-diff);
    }
   len= 8+(container->container_id-1)*DEFAULT_CONTAINER_SIZE;
 //  printf("write offset %ld \n", len);
   P(g_cvol->mutex);
  
   lseek(g_cvol->file_descriptor, len,SEEK_SET);
   if(writen(g_cvol->file_descriptor, buffer, DEFAULT_CONTAINER_SIZE)!=DEFAULT_CONTAINER_SIZE){
       err_msg1("failed to append container");
       V(g_cvol->mutex);
        free(buffer);
        return -1;
    }
    V(g_cvol->mutex);
	
    free(buffer);
    return container->container_id;
}

/*
 * Read a container from container volume.
 * return NULL if failure.
 */
Container *read_container(int32_t id){
    char *buffer = (char*)malloc(DEFAULT_CONTAINER_SIZE+1);
   char *p=buffer;
   off_t len=0;
  if(id>g_cvol->container_num || id==0)
  	return NULL;

   len= 8+(id-1)*DEFAULT_CONTAINER_SIZE;
  // printf("read offset %ld \n", len);
    P(g_cvol->mutex); 

    lseek(g_cvol->file_descriptor,len, SEEK_SET);
    if(readn(g_cvol->file_descriptor, buffer, DEFAULT_CONTAINER_SIZE)!=DEFAULT_CONTAINER_SIZE){
        free(buffer);
       V(g_cvol->mutex);
	err_msg1("readn container wrong");
        return NULL;
    }
    V(g_cvol->mutex);

    int32_t valid = 0;
    memcpy(&valid,p,4);p+=4;

    if(valid!=container_valid){
        err_msg2("read an invalid container %d ",id);
        free(buffer);
        return NULL;
    }
    Container *container = (Container*)malloc(sizeof(Container));
    memcpy(&container->container_id,p,4);p+=4;
	
    if(container->container_id!=id){
        err_msg3("Read a wrong container with id=%d != %d",container->container_id,id);
        free(buffer);
        free(container);
       return NULL;
    }
   memcpy(&container->data_size,p,4);p+=4;
   memcpy(&container->chunk_num,p,4);p+=4;
   
    ChunkAddr tmp;
    container->meta_table = htable_init((char*)&tmp.next - (char*)&tmp,sizeof(Fingerprint),container->chunk_num);
	
    int i;
    for(i=0; i<container->chunk_num; ++i){
        ChunkAddr *addr = (ChunkAddr*)malloc(sizeof(ChunkAddr));
	memcpy(&addr->offset,p,4);p+=4;
	memcpy(&addr->length,p,4);p+=4;
	memcpy(addr->hash,p,sizeof(Fingerprint));p+=sizeof(Fingerprint);
	addr->container_id=id;
        htable_insert(container->meta_table,(unsigned char*)addr->hash, addr);
  
    }
    container->data_buffer = (unsigned char *)malloc(container->data_size);
    memcpy(container->data_buffer,p,container->data_size);
    
    free(buffer);
    return container;
}



 bool check_chunk(Chunk *psChunk){
    uint8_t tmphash[20];
    SHA_CTX ctx;
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, psChunk->data, psChunk->length);
    SHA1_Final(tmphash,&ctx);
    if(memcmp(tmphash, psChunk->hash, 20)!=0){
        err_msg1("Chunk has been corrupted!");
        return false;
    }
    return true;
}

Chunk* read_chunk(Container *container, Fingerprint *hash){
    ChunkAddr *addr = (ChunkAddr*)htable_lookup(container->meta_table,(unsigned char *)hash);

    if(!addr){
       // err_msg1(" such chunk does not exist in this container");
        return NULL;
    }
    if(addr->length<0||addr->offset > container->data_size || addr->offset < 0){
       err_msg3("Invalid chunk length or offset, length=%d, offset=%d",addr->length, addr->offset);
        return NULL;
    }
    //printf("%s, %d:length=%d, offset=%d\n",__FILE__,__LINE__,addr->length, addr->offset);
    Chunk *chunk =chunk_new( addr->hash,container->data_buffer+addr->offset,addr->length);
    if(*(container->data_buffer+addr->offset+chunk->length) != '\t'){
      	err_msg1("container has been corrupted.");
    }
    check_chunk(chunk);
    return chunk;
}


bool write_chunk (Container *container, Chunk *chunk){
    if((16+htable_get_size(container->meta_table)*28+container->data_size)>(DEFAULT_CONTAINER_SIZE-28-chunk->length-1)){
      //  printf("%s, %d: Container %d  is  is full (size:%d)\n",__FILE__,__LINE__,container->container_id,16+htable_get_size(container->meta_table)*28+container->data_size);
        return false;
    }
    check_chunk(chunk); // check whether the data is correct because of network tramssimition 
    ChunkAddr *new_addr = chunkaddr_new(chunk->hash,container->data_size,chunk->length);
    if(htable_insert(container->meta_table,new_addr->hash, new_addr)==1){
	    memcpy(container->data_buffer+container->data_size, chunk->data, chunk->length);
	    container->data_size += chunk->length;
	    memset(container->data_buffer+container->data_size, '\t', 1);
	    container->data_size++;
	    container->chunk_num++;
    }
   else
   	chunkaddr_free(new_addr);
   
    return true;
}
void test_read_container(){
	container_vol_init(); // public volume initialization
	Container *ca=NULL;
	int i;
	for(i=0;i<g_cvol->container_num;i++){
		ca=read_container(i+1);
		if(ca)
		{
			printf("%d ",i+1);
			container_destroy(ca);
		}
	}
	container_vol_destroy();
}


///=======================
/// Function:the following is used for test
/// Date:2012-12-26
/// Author:zhoupj
//#define CONTAINER_TEST
#ifdef CONTAINER_TEST

#define BUF_LEN 1000

int main(){
	int i;
        char hash[20]={0};
        char buf[BUF_LEN]={0};
	Chunk *chunk;
	int ctaID=0;
	
	printf("fingerprint size=%d\n",sizeof(Fingerprint));
	printf("===============write==============\n");
	container_vol_init(); // public volume initialization
	Container *cta=container_init();
	cta->container_id=container_id_new();// new id

	for(i=0;i<1000000;i++){
		memset(buf,0,BUF_LEN);
		sprintf(buf,"%dACE==EDRY3DKLFJSL#O*NCVL4C=== X#$",i);
		finger_chunk(buf,strlen(buf),hash);
		chunk=chunk_new(hash,buf,strlen(buf));
		//printf("%20s,%d,%s\n",chunk->hash,chunk->length,chunk->data);
		
		while(write_chunk(cta,chunk)==false){//container is full
			write_container(cta);
			printf("%d\n",i);
			container_destroy(cta);
			cta=container_init();  // new container 
			cta->container_id=container_id_new();// new id
		}
		chunk_free(chunk);
		
	}
	if(cta->data_size>0){
		write_container(cta);
	        printf("container id %d (datasize:%d,chunkcount:%d) \n",cta->container_id,cta->data_size,cta->chunk_num);
	}
	ctaID=cta->container_id;
	
	container_destroy(cta);
	container_vol_destroy();
	///read
	printf("===============read===============\n");
	container_vol_init(); 
	int j=0;
	int k=0;
	srand(time(NULL));
	while(1){
		k++;
		if(k>10)
			break;
		i=1000000-rand()%1000001;
		memset(buf,0,BUF_LEN);
		sprintf(buf,"%dACE==EDRY3DKLFJSL#O*NCVL4C=== X#$",i);
		finger_chunk(buf,strlen(buf),hash);
		for(j=1;j<=ctaID;j++){
			cta=read_container(j);
			if(cta!=NULL)
			{
				chunk=read_chunk(cta,hash);
				if(chunk!=NULL){
					printf("%d found %d ++ containerid:%d %d %s \n",k,i,j,chunk->length,chunk->data);
					printf("\n");
					break;
				}
				else
					printf("chunk==NULL\n");

				container_destroy(cta);
				cta=NULL;
			}
			else
				printf("cta==NULL\n");
		       
		}
		if(cta)
			container_destroy(cta);
		free(chunk);
	}
	container_vol_destroy();
}

#endif
