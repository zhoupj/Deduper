#include "global.h"

htable * cbr_htable=NULL;
double left_number=0;
double total_numbel=0;
double rewirte_ratio=0.05;
double max_utility=0.3;
double current_uitilty=0.3;
int utility_buckets[1000+1];

void  Cbr_init(){
	left_number=0;
	total_numbel=0;
	rewirte_ratio=0.05;
	max_utility=0.3;
         current_uitilty=0.3;
	CBRItem r;
	cbr_htable=htable_init((char*)&r.next-(char*)&r,4,32); // 32 ¸öcontainer
	memset(utility_buckets,0,sizeof(utility_buckets));
	
}
void Cbr_destroy(){
	htable_destroy(cbr_htable);
}
void Cbr_scan(int old_container_id, int chunk_size){
	CBRItem * rf=htable_lookup(cbr_htable,&old_container_id);
	if(rf) {
		rf->size+=chunk_size;
		//rf->count++;
		}
	else{
		rf=(CBRItem *)malloc(sizeof(CBRItem));
		rf->size=chunk_size;
		rf->container_id=old_container_id;
		rf->rewrite_flag=1;
		//rf->count=0;
		htable_add(cbr_htable,&rf->container_id,rf);
	}
}
int cbr_decision(int old_container_id,int chunk_size){
	CBRItem * rf=htable_lookup(cbr_htable,&old_container_id);
	int rewrite=0;
	double thr=0;
	int value=0;
	int i;
	if(rf){
		total_numbel++;
		thr=rf->size*1.0/ DEFAULT_CONTAINER_SIZE;
		value=thr/0.001;
		if(value>1000)
			value=1000;
		utility_buckets[value]++;
		
		if(!rf->rewrite_flag)
			rewrite=0;
		
		else if(thr<=(MIN(max_utility,current_uitilty))){
			rewrite=1;
			//printf("%f (%f )  ",thr, current_uitilty);
		}
		else{
			rf->rewrite_flag=0;
			rewrite=0;
		}
		rf->size-=chunk_size;
		if(rf->size==0)
			htable_remove(cbr_htable,&old_container_id);
		
		left_number=0;
		i=0;
		current_uitilty=0.05;
		while(left_number/total_numbel< rewirte_ratio && i<=1000){// expected to be improved
			left_number+=utility_buckets[i++];
			current_uitilty+=0.001;
		}
		if(current_uitilty<0.05)
			current_uitilty=0.05;
		
	}
	else{
		err_msg1("error ==========");
		return 1;
	}
	return rewrite;
		
}



