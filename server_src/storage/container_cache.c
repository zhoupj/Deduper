#include "../global.h"

Cache * cache_init(int cache_size){
	Cache *cache=NULL;
	    cache=(Cache *)malloc(sizeof(Cache));
	if(!cache)
		err_msg1("malloc worng");
	cache->hit=0;
	cache->item_count=cache_size/60; // item number
	cache->miss=0;
	cache->plist=dlist_init();
	ChunkAddr ca;
	cache->htable=NULL;
	cache->htable=htable_init((char*)&ca.next-(char*)&ca,20,cache->item_count);
	return cache;
}
void cache_destroy(Cache * cache){
	 cache_statistics(cache);
	 dlist_destory(cache->plist);
	 if(cache->htable)
	 	htable_destroy(cache->htable);
	 free(cache);
}


/* read container meta into cache (htable_base) according to container id */
int cache_insert(Cache *cache,Container * ca){
	Container * evitor_ca=NULL;
	ChunkAddr *cadr=NULL;
	ChunkAddr *newcadr=NULL;
	int *evitor_ca_id=0;
	if(cache->htable->num_items>=cache->item_count){ //LRU 
		dlist_traver(cache->plist);
		evitor_ca_id=(int*)dlist_delete_tail(cache->plist);
		if(*evitor_ca_id>0){
			evitor_ca=read_container(*evitor_ca_id);
			if(evitor_ca==NULL)
				err_msg1("wrong read container in cache");
			cache_remove(cache,evitor_ca);
			container_destroy(evitor_ca);
			
		}
		free(evitor_ca_id);
	}
	foreach_htable(cadr,ca->meta_table){
		newcadr=chunkaddr_new(cadr->hash,cadr->offset,cadr->length);
		newcadr->container_id=ca->container_id;
		htable_insert(cache->htable,newcadr->hash,newcadr);
	}
	int *id=(int *)malloc(sizeof(int));
	*id=ca->container_id;
	dlist_insert(cache->plist,id); //LRU List
	 return SUCCESS;
}
void cache_remove(Cache * cache,Container * ca){

	//printf("evict container %d \n",ca->container_id);
	ChunkAddr *cadr=NULL;
	foreach_htable(cadr,ca->meta_table){
		htable_remove(cache->htable,cadr->hash);
	}
}
void * cache_search(Cache*cache,unsigned char *key){
	void * res=htable_lookup(cache->htable, key);
	if(res){
		cache->hit++;
		dlist_move_value(cache->plist,&(((ChunkAddr*)res)->container_id), sizeof(int)); //LRU
	}
	else
		cache->miss++;
	return res;
}
void cache_statistics(Cache * cache){

	printf("\n============cache statistics=============\n");
	printf("container count included:%d \n",cache->plist->size);
	printf("current items in cache:%d \n",cache->htable->num_items);
	printf("max items in cache:   %d \n",cache->item_count);
	printf("search cache counts:%d\n",(cache->hit+cache->miss));
	printf("hits: %d \n",cache->hit);
	printf("miss: %d \n",cache->miss);
	printf("============cache statistics=============\n\n");
}


// for restore 
//date: 2013-2-21
// author: zhou,p.j.
Cache * cache_init_r(int container_num){
	Cache *cache=NULL;
	    cache=(Cache *)malloc(sizeof(Cache));
	if(!cache)
		err_msg1("malloc worng");
	cache->hit=0;
	cache->item_count=container_num; // item number
	cache->miss=0;
	cache->plist=dlist_init();
	ChunkAddrRes ca;
	cache->htable=NULL;
	cache->htable=htable_init((char*)&ca.next-(char*)&ca,20,cache->item_count*512);
	return cache;
}
/* read container meta into cache (htable_base) according to container id */
int cache_insert_r(Cache *cache,Container * ca){
	Container * evitor_ca=NULL;
	ChunkAddr *cadr=NULL;
	ChunkAddrRes *newcadr=NULL;
	
	if(cache->plist->size>=cache->item_count){ //LRU 
		//dlist_traver(cache->plist);================check

		/*PNode p=cache->plist->head;
		while(p){
			printf("%d\t  ",((Container*)p->data)->container_id);
			p=p->next;
		}
		printf("\n");
		*/
		//===================
		evitor_ca=(Container*)dlist_delete_tail(cache->plist);
		if(evitor_ca){
			cache_remove(cache,evitor_ca);
			container_destroy(evitor_ca);
			
		}
	}
	foreach_htable(cadr,ca->meta_table){
		newcadr=(ChunkAddrRes*)malloc(sizeof(ChunkAddrRes));
		newcadr->length=cadr->length;
		newcadr->offset=cadr->offset;
		newcadr->container=ca;
		memcpy(newcadr->hash,cadr->hash,sizeof(Fingerprint));
		htable_insert(cache->htable,newcadr->hash,newcadr);
	}
	dlist_insert(cache->plist,ca); //LRU List
	 return SUCCESS;
}
void * cache_search_r(Cache*cache,unsigned char *key){
	ChunkAddrRes* res=(ChunkAddrRes*)htable_lookup(cache->htable, key);
	if(res){
		cache->hit++;
		dlist_move_ptr(cache->plist,res->container); //LRU
	}
	else
		cache->miss++;
	return res;
}


