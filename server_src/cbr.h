#ifndef CBR_H_
#define CBR_H_

typedef struct CBR_item{
	int container_id;
	int size;
	int rewrite_flag;
	//int count;
	hlink next;
}CBRItem;


int cbr_decision(int old_container_id,int chunk_size);
void Cbr_scan(int old_container_id, int chunk_size);
void Cbr_destroy();
void  Cbr_init();



#endif

