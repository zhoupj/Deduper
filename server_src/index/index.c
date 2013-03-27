
#include "../global.h"

/* read from file */
void index_open(MemIndex* memdex){ 
	char path[256]={0};
	int fd;
	unsigned int count=0;
	int i;
	ContainerAddr * cddr=NULL;
	strcpy(path,BackupVolPath);
	strcat(path,"global_index");
	fd=open(path, O_RDWR | O_CREAT , S_IREAD|S_IWRITE);
	if(fd<0){
		printf("%s,%d file open wrong \n",__FILE__,__LINE__);
		return;
	}
	lseek(fd, 0, SEEK_SET);
	if(read(fd,&count,4)==4){
		for(i=0;i<count;i++){ /* you can use a big enough  buffer*/
			cddr=(ContainerAddr*)malloc(sizeof(ContainerAddr));
			read(fd,cddr->hash,sizeof(Fingerprint));
			read(fd,&cddr->container_id,4);
			htable_insert(memdex->index_table,cddr->hash,cddr);
		}
		printf("%s,%d,total index items:%u\n",__FILE__,__LINE__,memdex->index_table->num_items);
			
	}
	close(fd);
}

/* write into file */
void index_close(MemIndex* memdex){
	char path[256]={0};
	int fd;
	strcpy(path,BackupVolPath);
	strcat(path,"global_index");
	fd=open(path, O_RDWR);
	if(fd<0){
		printf("%s,%d file open wrong \n",__FILE__,__LINE__);
		return;
	}
	 lseek(fd, 0, SEEK_SET);
	ContainerAddr * cddr=NULL;
	write(fd,&(memdex->index_table->num_items),sizeof(int));
	 foreach_htable(cddr,memdex->index_table){
	 	write(fd,cddr->hash,sizeof(Fingerprint));
		write(fd,&cddr->container_id,sizeof(int));
	 }
	 close(fd);
}
MemIndex* index_init(){
	MemIndex * index=(MemIndex *)malloc(sizeof(MemIndex));
	ContainerAddr caddr;
	ChunkAddr cdr;
	index->index_table=htable_init((char*)&(caddr.next)-(char*)&caddr,sizeof(Fingerprint),HASH_TBALE_SIZE); // all times
	index->index_new=htable_init((char*)&(cdr.next)-(char*)&cdr,sizeof(Fingerprint),10*1024);
	index->index_cache=cache_init(30*1024*1024); // 30MB cache
	//index->container_cache=htable_init((char*)&(ct.next)-(char*)&ct,sizeof(int),1024);
	
	 pthread_mutex_init(&index->mutex, 0);
	 index_open(index); // like DDFS, these items actually are stored in disk, but we stimulate it in memory 
	return index;
}

/* insert the items of container ct  into MemDex */
void index_insert(MemIndex* memdex,Container * ct){
	P(memdex->mutex);
	ChunkAddr*cddr;
	ContainerAddr *ctner;
	 foreach_htable(cddr, ct->meta_table){
	 	ctner=htable_lookup(memdex->index_table,cddr->hash);
		if(ctner){
			ctner->container_id=ct->container_id; // 改为最新的container id
			//printf("global index update \n");
		}
		else{
			ctner=(ContainerAddr *)malloc(sizeof(ContainerAddr));
			ctner->container_id=ct->container_id;
			memcpy(ctner->hash,cddr->hash,sizeof(Fingerprint));
			htable_add(memdex->index_table,ctner->hash,ctner);
		}
		htable_remove(memdex->index_new,cddr->hash);
		/* ==bloom filter=== */
	 }	
	V(memdex->mutex);
}
void * index_lookup(MemIndex* memdex,unsigned char * key){ /* for backup */
	P(memdex->mutex);
	ContainerAddr * caddr=NULL;
	ChunkAddr *cka=NULL;
	Container *ca=NULL;
	// First Step: search it from cache in memory firstly 
	cka=(ChunkAddr *)cache_search(memdex->index_cache,  key);
	if(cka){
		V(memdex->mutex);
		return cka;
	}

	// Secondly, search it  in Bloom Filter which is not completed now

	/*
	 *
	 *
	*/

	
	// Thirdly, search it from global indexs in disk (Actually, they are stored in memory, but we assum they are stored in disk)
	caddr=htable_lookup(memdex->index_table, key);//全局重删

	// The Fourth, search it from the previous indexs generate by this stream or file
	if(caddr==NULL) // new item
	{
		cka=htable_lookup(memdex->index_new,key);  // 作业内自身重删
		if(!cka){
			cka=chunkaddr_new(key,0,0);
			cka->container_id=0;
			htable_add(memdex->index_new,cka->hash,cka);
			V(memdex->mutex);
			return NULL;
		}
		V(memdex->mutex);
		return cka;
	}
	else{  // found in disk
		
		ca=read_container(caddr->container_id);
		if(ca==NULL){
			err_msg1("container read error");
			V(memdex->mutex);
			return NULL;
		}
		cache_insert(memdex->index_cache,ca); // insert all items of the found container into cache
		//cka=(ChunkAddr *)htable_lookup(ca->meta_table,key);
		container_destroy(ca);
		cka=(ChunkAddr *)cache_search(memdex->index_cache,  key);	
		V(memdex->mutex);
		return cka;
	}
}


void index_destroy(MemIndex* memdex){
	index_close( memdex);
	htable_destroy(memdex->index_table);
	cache_destroy(memdex->index_cache);
	htable_destroy(memdex->index_new);
	free(memdex);
}

//============================================for restore
//==================================================
MemIndex* index_init_r(){
	MemIndex * index=(MemIndex *)malloc(sizeof(MemIndex));
	ContainerAddr caddr;
	index->index_table=htable_init((char*)&(caddr.next)-(char*)&caddr,sizeof(Fingerprint),HASH_TBALE_SIZE); // all times
	//index->index_new=htable_init((char*)&(caddr.next)-(char*)&caddr,sizeof(Fingerprint),10*1024);
	index->index_cache=cache_init_r( G_CONTAINER_NUM); 
	//index->container_cache=htable_init((char*)&(ct.next)-(char*)&ct,sizeof(int),1024);
	 pthread_mutex_init(&index->mutex, 0);
	 index_open(index); // like DDFS, these items actually are stored in disk, but we stimulate it in memory 
	return index;
}

// restore

Chunk* index_search_r(MemIndex* memdex,unsigned char * key){ /* for restore */

	P(memdex->mutex);
	ContainerAddr * caddr=NULL;
	ChunkAddrRes *cka=NULL;
	Chunk *ck=NULL;
	Container *ca;
	//1. search in cache
	cka=(ChunkAddrRes *)cache_search_r(memdex->index_cache,  key);
	if(cka){
		//
		 if(cka->length<0||cka->offset > cka->container->data_size || cka->offset < 0){
		       err_msg3("Invalid chunk length or offset, length=%d, offset=%d",cka->length, cka->offset);
		        return NULL;
		    }
		    Chunk *chunk =chunk_new( cka->hash,cka->container->data_buffer+cka->offset,cka->length);
		    if(*(cka->container->data_buffer+cka->offset+chunk->length) != '\t'){
		      	err_msg1("container has been corrupted.");
		    }
		    check_chunk(chunk);
		//
		V(memdex->mutex);
		return chunk;
	}
	// 2. search in disk
	caddr=htable_lookup(memdex->index_table, key);
	if(caddr==NULL){
		err_msg1("sorry , not found , maybe wrong");
	}
	else
	{  // found in disk
		ca=read_container(caddr->container_id);
		G_R_READNUM++;
		ck=read_chunk(ca,key);
		cache_insert_r(memdex->index_cache,ca); // insert all items of the found container into cache
	}
	V(memdex->mutex);
	return ck;
}

void index_destroy_r(MemIndex* memdex){
	index_close( memdex);
	htable_destroy(memdex->index_table);
	 cache_statistics(memdex->index_cache);
	while(memdex->index_cache->plist->size>0){
		Container * ca=(Container *)dlist_delete_tail(memdex->index_cache->plist);
		if(ca)
			container_destroy(ca);
	}
	//cache_destroy(memdex->index_cache);
	//htable_destroy(memdex->index_new);
	free(memdex);
}



///Function: test
///Date:2012-12-27
///Author:zhoupj


//#define INDEX_TEST
#ifdef INDEX_TEST
MemIndex * g_index=NULL;
#define TOTAL 1000000
#define BUF_LEN 200
int main(){
	int i;
        char hash[20]={0};
        char buf[BUF_LEN]={0};
	Chunk *chunk=NULL;
	int ctaID=0;
	///============================write
	printf("fingerprint size=%d\n",sizeof(Fingerprint));
	printf("===============write==============\n");
	container_vol_init(); // public volume initialization
	Container *cta=container_init();
	cta->container_id=container_id_new();// new id
	g_index=index_init();
	for(i=0;i<TOTAL;i++){
		memset(buf,0,BUF_LEN);
		sprintf(buf,"%dACE==EDRY3DKLFJSL#O*NCVL4C%d,%d123ddfgdg",i,i,i);
		finger_chunk(buf,strlen(buf),hash);
		if(index_lookup(g_index,hash)){
			//printf("existed \n");
			continue;
		}
		chunk=chunk_new(hash,buf,strlen(buf));
		while(write_chunk(cta,chunk)==false){//container is full
			write_container(cta);
			index_insert(g_index,cta);
			printf("%d\n",i);
			container_destroy(cta);
			cta=container_init();  // new container 
			cta->container_id=container_id_new();// new id
		}
		chunk_free(chunk);
		
	}
	if(cta->data_size>0){
		write_container(cta);
		index_insert(g_index,cta);
	        printf("container id %d (datasize:%d,chunkcount:%d) \n",cta->container_id,cta->data_size,cta->chunk_num);
	}
	ctaID=cta->container_id; // max id;
	
	container_destroy(cta);
	container_vol_destroy();
	cache_statistics(g_index->index_cache);
	index_destroy(g_index);

	
	///r============================================read
	printf("===============read===============\n");
	container_vol_init(); 
	g_index=index_init();
	int j=0;
	int k=0;
	srand(time(NULL));
	cta=NULL;
	chunk=NULL;
	while(1){
		k++;
		if(k>10)
			break;
		i=TOTAL-rand()%(TOTAL);
		memset(buf,0,BUF_LEN);
		sprintf(buf,"%dACE==EDRY3DKLFJSL#O*NCVL4C%d,%d123ddfgdg",i,i,i);
		finger_chunk(buf,strlen(buf),hash);
		if(cta==NULL)
			cta=index_search(g_index,hash);
		if(cta==NULL)
			printf("%d Not found %d\n",k,i);
		else{
			
			while((chunk=read_chunk(cta,hash))==NULL){
				printf("not in this container %d\n",cta->container_id);
				container_destroy(cta);
				cta=index_search(g_index,hash);
				if(cta==NULL)
					break;
			}

			if(chunk!=NULL){
					printf("%d found %d ++ containerid:%d %d %s \n",k,i,cta->container_id,chunk->length,chunk->data);
			}
			else
				printf("%d  chunk==NULL fail to find %d \n", k,i);
	
		}
		       
	}
	
	if(chunk)
		free(chunk);
	if(cta)
		container_destroy(cta);
	container_vol_destroy();
	cache_statistics(g_index->index_cache);
	index_destroy(g_index);
}
#endif
