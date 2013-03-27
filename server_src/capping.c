#include "global.h"


CapItem * CT=NULL;
htable * cap_htable=NULL;

void  capping_init(){
	
	CapItem r;
	cap_htable=htable_init((char*)&r.next-(char*)&r,4,32); // 32 ¸öcontainer

}
void capping_destroy(){
	htable_destroy(cap_htable);
	
}
void  capping_reset(){
	
	CapItem * rf;
	foreach_htable(rf,cap_htable){
		htable_remove(cap_htable,&rf->container_id);
	}
	if(cap_htable->num_items!=0)
		err_msg1("reset wrong");
	
}

void capping_insert(int old_container_id){
	CapItem * rf=htable_lookup(cap_htable,&old_container_id);
	if(rf) {
		//rf->data_size+=chunk_size;
		rf->chunk_count++;
		}
	else{
		rf=(CapItem *)malloc(sizeof(CapItem));
		rf->chunk_count=1;
		rf->container_id=old_container_id;
		//rf->rewrite=0;
		htable_add(cap_htable,&rf->container_id,rf);
	}
}
int capping_build(){
	if(cap_htable->num_items<=G_CAPPING_T){
		return 0;
	}

	int diff_num=cap_htable->num_items-G_CAPPING_T;
	printf("%s %d needing cappping total (%d)  %d \n",__FILE__,__LINE__,diff_num,cap_htable->num_items);
	int i=0;
	int min_ck=2147483647;
	int min_cid=0;
	CapItem *var=NULL;
	for(i=0;i<diff_num;i++){
		min_cid=0;
		int min_ck=2147483647;
		foreach_htable(var,cap_htable){
			if(min_ck> var->chunk_count){
				min_ck=var->chunk_count;
				min_cid=var->container_id;
			}
		}
		if(min_cid)
			htable_remove(cap_htable,&min_cid);
		else
			err_msg1("maybe wrong");
	}
	printf("===============%d \n",cap_htable->num_items);
}

int capping_rewrite(int old_container_id){
	CapItem * rf=htable_lookup(cap_htable,&old_container_id);
	if(rf)
		return 0;
	else
		return 1; // ÖØĞ´
		
}



