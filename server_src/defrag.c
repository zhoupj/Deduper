#include "global.h"
/*
target: speeding up restoring speed
Idea: simulate the LRU cacheing container like the process of resotring
Principle:
1. Read less containers increasing cache hit and reducing the disk operation
2. Ensure that the decrease of the deduplication ratio is not more that 5% 
*/

htable * scan_htable=NULL;
double rewrite_size;

double max_factor; //
double cur_factor;

int factor_buckets[1000+1];
int max_b=0;


void  opt_init(){
	ReferItem r;
	scan_htable=htable_init((char*)&r.next-(char*)&r,4,32); // 32 ¸öcontainer
	max_factor=1.0/G_CAPPING_T; 
	cur_factor=max_factor;
	rewrite_size=0;
	memset(factor_buckets,0,sizeof(factor_buckets));
	printf("max_factor:%f , cur_factor:%f \n",max_factor,cur_factor);
}
void opt_destroy(){
	htable_destroy(scan_htable);
	
}
void opt_insert(int old_container_id, int chunk_size){
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
		rf->rewrite=0;
		htable_add(scan_htable,&rf->container_id,rf);
	}
	
}

int opt_decision(int old_container_id, int chunk_size){
	int rewrite=0;
	double thr;
	int value;
	
	if((scan_htable->num_items*1.0)<= MIN(1.0/cur_factor,1.0/max_factor)){
		rewrite=0;; // no rewrite
		int i=0;
		double size=0;
		while(i<=scan_htable->num_items && ( size/G_total_size<=0.05))
			size+=factor_buckets[i];
		
		cur_factor=size/G_total_size;
	}
		
	else{
		if(max_b<scan_htable->num_items)
			max_b=scan_htable->num_items;
		
		factor_buckets[scan_htable->num_items]+=chunk_size;
		
		//printf("%d \t",scan_htable->num_items);
		ReferItem * rf=htable_lookup(scan_htable,&old_container_id);
		if(rf){ // find the min container_id with the minimal counts
			
			int min_cid=0;
			int min_count=2147483647;
			int size=0;
			ReferItem * var;
			foreach_htable(var,scan_htable){
				if(min_count> var->count){
					min_count=var->chunk_count;
					min_cid=var->container_id;
					size=var->size;
				}
			}
			if(min_cid){
				htable_remove(cap_htable,&min_cid);
				rewrite_size+=size;
				int i=0;
				
				
			}
		}
	}
}


