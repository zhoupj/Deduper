#include "global.h"


htable * scan_htable=NULL;
int fragmentation_threshold=0;

int64_t rewrite_size=0;
double current_threshold=0;

void  scan_init(){
	ReferItem r;
	scan_htable=htable_init((char*)&r.next-(char*)&r,4,32); // 32 ¸öcontainer
	 fragmentation_threshold=DEFAULT_CONTAINER_SIZE*G_THRESHOLD;
}
void scan_destroy(){
	htable_destroy(scan_htable);
	
}
void scan_reset(){
	
	ReferItem * rf;
	foreach_htable(rf,scan_htable){
		htable_remove(scan_htable,&rf->container_id);
	}
	if(scan_htable->num_items!=0)
		err_msg1("scan_reset wrong");
	
}
int scan_numer(){
	return scan_htable->num_items;
}
void scan_insert(int old_container_id, int chunk_size){
	ReferItem * rf=htable_lookup(scan_htable,&old_container_id);
	if(rf) {
		rf->size+=chunk_size;
		rf->count++;
		}
	else{
		rf=(ReferItem *)malloc(sizeof(ReferItem));
		rf->size=chunk_size;
		rf->container_id=old_container_id;
		rf->count=1;
		htable_add(scan_htable,&rf->container_id,rf);
	}
	
}
void * scan_search(int old_container_id){
	ReferItem * rf=htable_lookup(scan_htable,&old_container_id);
	return rf;
}
char scan_decision(int old_container_id){
	ReferItem * rf=htable_lookup(scan_htable,&old_container_id);
	//printf("%d \t",scan_htable->num_items);
	int exist=0;
	if(rf){
		if(rf->size<= DEFAULT_CONTAINER_SIZE*G_THRESHOLD ){
			//printf("%d --%d \n",rf->container_id,rf->size);
			if(current_threshold<=0.05){
				G_THRESHOLD+=0.01;
				exist=0;
				rewrite_size+=rf->size;
			}
			else{
				G_THRESHOLD-=0.01;
				exist=1;
			}
			// fragmetaion
			printf("%f   %f \n", G_THRESHOLD,current_threshold);
		}
		else{
			exist=1;
		}
		current_threshold=rewrite_size*1.0/G_total_size;
		rf->count--;
		if(rf->count==0)
			htable_remove(scan_htable,&old_container_id);
	}
	else{
		printf("error ==========\n");
		return '0';
	}
	if(exist)
		return '1';
	else
		return '0';
}


