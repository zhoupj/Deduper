#ifndef CAPPING_H_
#define CAPPING_H_



typedef struct Cap_item{
	int container_id; 
	//int data_size; //total data size
	int chunk_count; // chunk count
	//int access_count; // ทรฮสดฮสý
	//int rewrite;
	hlink next;
}CapItem;


void  capping_init();
void capping_destroy();
void  capping_reset();
void capping_insert(int old_container_id);
int capping_build();
int capping_rewrite(int old_container_id);

#endif

