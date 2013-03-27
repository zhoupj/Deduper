#ifndef _CONTAINER_H_
#define _CONTAINER_H_

typedef unsigned char Fingerprint[20];

struct chunk_tag{
    int32_t length;
    Fingerprint hash; 
    unsigned char *data;
};

struct chunk_address{
    int32_t offset;
    int32_t length;
    int32_t container_id; //2013-1-23
    Fingerprint hash;
    hlink next;
};





struct container_tag{
    //descriptor
    int32_t container_id;
    int32_t data_size;
    int32_t chunk_num;
	
    htable *meta_table;  /* 数据块的元数据信息*/
    unsigned char *data_buffer;
    hlink next;	
};

/* volume */
struct container_volume{
    int32_t container_num;
   Queue * freequeue; // 空闲container 的队列
    int file_descriptor;
    int free_fd;
    pthread_mutex_t mutex;
    char volume_path[256];
    char free_vol_path[256];
};

typedef struct chunk_tag Chunk;
typedef struct chunk_address ChunkAddr;
typedef struct container_tag Container;                


typedef struct chunk_address_restore{ // for restore 2013-2-21
	 int32_t offset;
   	 int32_t length;
   	 Container *container; 
   	 Fingerprint hash;
   	 hlink next;
}ChunkAddrRes;

Chunk * chunk_new(Fingerprint hash,unsigned char *data,int datalen);
void chunk_free(Chunk *ck);
ChunkAddr * chunkaddr_new(Fingerprint hash,int offset,int length);
void chunkaddr_free(ChunkAddr *ca);

bool container_vol_init();
int container_vol_max_num();

void container_vol_update();
void container_vol_destroy();

int container_id_new();
Container *container_new();
void container_free(int id);
double container_usage(Container *ca);
Container * container_open_last();



Container *container_init();
void container_destroy(Container *container);
int32_t write_container(Container *container);
Container *read_container(int32_t id);
Chunk* read_chunk(Container *container, Fingerprint *hash);
bool check_chunk(Chunk *psChunk);

bool write_chunk (Container *container, Chunk *chunk);
void test_read_container();


#endif

