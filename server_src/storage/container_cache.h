#ifndef _CONTAINER_CACHE_H_
#define _CONTAINER_CACHE_H_

typedef struct lru_cache{
	int item_count; 
	int hit;
	int miss;
	htable *htable;
	DList *plist; // record  lru-based container id by using double linked lists
}Cache;

Cache* cache_init(int item_size);
void cache_destroy(Cache * cache);
int cache_insert(Cache *cache,Container * ca);
void cache_remove(Cache * cache,Container * ca);
void* cache_search(Cache*cache,unsigned char *key);
void cache_statistics(Cache * cache);

///=============
Cache * cache_init_r(int container_num);
int cache_insert_r(Cache *cache,Container * ca);
void * cache_search_r(Cache*cache,unsigned char *key);


#endif

