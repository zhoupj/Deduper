#ifndef INDEX_H_
#define INDEX_H_

// 根据container_index 找到对应container
struct container_index{
	Fingerprint hash; 
	int32_t container_id;
	hlink next;
};
typedef struct container_index ContainerAddr;

struct mem_index{
	htable *index_table;            /* include all ContainerAddr items in disk*/
	htable *index_new;             /* new item (only include fingerprints */
	Cache *index_cache;         /* part of ChunkAddr items in memory */
	htable * container_cache; /* include container for restore*/
	pthread_mutex_t mutex;
};
typedef struct mem_index MemIndex;

MemIndex* index_init();
void index_insert(MemIndex* memdex,Container * ct);
void * index_lookup(MemIndex* memdex,unsigned char * key);
Container* index_search(MemIndex* memdex,unsigned char * key);
void index_destroy(MemIndex* memdex);


MemIndex* index_init_r();
Chunk* index_search_r(MemIndex* memdex,unsigned char * key);
void index_destroy_r(MemIndex* memdex);



#endif
