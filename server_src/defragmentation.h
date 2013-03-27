#ifndef DEFRAGMENTATION_H_
#define DEFRAGMENTATION_H_

typedef struct{
	int container_id; // 0 meeans new items, or means old itmes
	int length;

}FingerStream;

typedef struct reference_item{
	int container_id;
	int size;
	int count;
	hlink next;
}ReferItem;



void  scan_init();
void scan_destroy();
void scan_reset();
int  scan_numer();
void * scan_search(int old_container_id);
void scan_insert(int old_container_id, int size);
char scan_decision(int old_container_id);

#endif