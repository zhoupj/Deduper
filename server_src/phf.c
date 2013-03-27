/* author:zhou peng ju
Date: 2013-3-14
*/


#include "global.h"
int compare_num=65536;
int base_num=256;

void set_bit(uint8_t * array,uint32_t bit_number){
	array[bit_number/8] |= (1<<(bit_number%8));
}
int judge_bit(uint8_t * array,uint32_t bit_number){
	if( array[bit_number/8] & (1<<(bit_number%8)))
		return 1;
	return 0;
}
void clear_bit(uint8_t * array,uint32_t bit_number){
	array[bit_number/8] &= (~(1<<(bit_number%8)));
}

//quik sort descending)

int quick_adjust(Bucket * A, int64_t  first,int64_t last){
	int64_t i=first,j=last;
	Bucket x;
	memcpy(&x,&A[i],sizeof(x));
	while(i<j){
		while(i<j &&  (A[j].size<=x.size))
			j--;
		if(i<j)
			memcpy(&A[i++],&A[j],sizeof(x));
		
		while(i<j && (A[i].size>=x.size))
			i++;
		if(i<j)
			memcpy(&A[j--],&A[i],sizeof(x));
			
	}
	memcpy(&A[i],&x,sizeof(x));
	return i;
}
void quick_sort(Bucket *A, int64_t first, int64_t last){
	int64_t mid;
	if(first<last){
		mid=quick_adjust(A,first,last);
		quick_sort(A,first,mid-1);
		quick_sort(A,mid+1,last);
	}
	
}
unsigned int hash_g(char *str){
	return *(uint32_t*)str;
}
unsigned int hash_f1(char *str){
	return *(uint32_t*)(str+8);
}
unsigned int hash_f2(char *str){
	return *(uint32_t*)(str+16);
}


PHF * phf_init(int64_t max_finger_number){

	PHF *phf=(PHF*)malloc(sizeof(PHF));
	phf->finger_num=max_finger_number;
	phf->bucket_num=phf->finger_num/7+1;
	phf->vector_num=1.43*phf->finger_num+1; // number of bits 
	
	phf->B=(Bucket *)malloc(phf->bucket_num*sizeof(Bucket));
	memset(phf->B,0,phf->bucket_num*sizeof(Bucket));
	int i;
	for(i=0;i<phf->bucket_num;i++){		
		phf->B[i].index=i;
		phf->B[i].size=0;
		phf->B[i].data=NULL;
	}

	phf->D=(uint16_t*)malloc(phf->bucket_num*2);
	memset(phf->D,0,phf->bucket_num);
	int64_t vector_size=(phf->vector_num>>3)+1;
	phf->T=(uint8_t *)malloc( vector_size);
	memset(phf->T,0,vector_size);

	phf->g_hash=hash_g;
	phf->f1_hash=hash_f1;
	phf->f2_hash=hash_f2;
	printf("finger number: %ld \n bucket_num:%ld\n vector size: %ld\n",phf->finger_num,phf->bucket_num,vector_size);
	return phf;
}
void phf_destory(PHF * phf){
	//free(phf->B);
	free(phf->D);
	free(phf->T);
	free(phf);
	
}
void phf_walk(PHF * phf){
	int i;
	uint32_t count=0;
	printf("phf walk========================\n");
   	for(i=0;i<phf->bucket_num;i++){
		//printf("%d ",phf->B[i].size);
		if(phf->B[i].size)
			count++;
	}
		
	printf("\n =====%u / %ld =====n",count,phf->bucket_num);
}


int phf_input(PHF * phf, Fingerprint  fp){  // spilt S into r buckets
	BucketData * bdata=(BucketData *)malloc(sizeof(BucketData));
	bdata->next=NULL;
	memcpy(bdata->hash,fp,20);
	bdata->hash[20]=0;

	int64_t r;
	r=phf->g_hash(bdata->hash)% (phf->bucket_num);
	bdata->next=phf->B[r].data; // insert in the head
	phf->B[r].data=bdata;
	phf->B[r].size++;

}

void phf_build(PHF * phf){
	uint32_t  r=0;
	int32_t f_index=0;// 0,1,2,3,.....
	uint32_t * K=NULL;
	int ki=0;
	uint32_t f_p=0;
	int i=0;
	BucketData * bd=NULL;
	BucketData * next=NULL;
	int max_match_num=0;
	printf("sort:%ld \n",(phf->bucket_num-1));
	// sort  *****descending
	quick_sort(phf->B,0,(phf->bucket_num-1));
	if(phf->B[0].size<=0){
		err_msg1("no itmes");
		return;
	}
	K=(uint32_t *)malloc(phf->B[0].size*4);
	
	for(r=0;r<phf->bucket_num;r++){
		ki=0;
		f_index=0;
		if(phf->B[r].size>0){
			bd=phf->B[r].data;
			while(bd && f_index<compare_num){
				f_p=(phf->f1_hash(bd->hash)+ phf->f2_hash(bd->hash)*(f_index/base_num)+f_index %base_num) % phf->vector_num;
				if(judge_bit(phf->T,f_p)==0){
					bd=bd->next;
					K[ki++]=f_p;
				}
				else{
					ki=0;
					bd=phf->B[r].data;
					f_index++;
				}
			}
			if(f_index==compare_num|| phf->B[r].size !=ki)
				err_msg1("may be wrong");
			if(max_match_num<f_index)
				max_match_num=f_index;
		  
			phf->D[phf->B[r].index]=f_index;
			for(i=0;i<ki;i++){
				set_bit(phf->T,K[i]);
			}
			bd=phf->B[r].data;
			while(bd){
				next=bd->next;
				free(bd);
				bd=next;
			}
			
		}
		
	}
	printf("max number of itmes in buckets is:%d\n",max_match_num);
	int64_t  vector_size=(phf->vector_num>>3)+1;
	memset(phf->T,0,vector_size);
	free(K);
	free(phf->B);
}
void phf_mark(PHF *phf,Fingerprint fp){
	uint32_t g_index;
	uint32_t vec_index;
	uint8_t str[21];
	memcpy(str,fp,20);
	str[20]=0;
	g_index=phf->g_hash(str)%(phf->bucket_num);
	vec_index=(phf->f1_hash(str)+ phf->f2_hash(str)*(phf->D[g_index] /base_num)+phf->D[g_index] %base_num) % phf->vector_num;
	set_bit(phf->T,vec_index);
}
int phf_search(PHF *phf,Fingerprint fp){
	
	uint32_t g_index;
	uint32_t vec_index;
	uint8_t str[21];
	memcpy(str,fp,20);
	str[20]=0;
	g_index=phf->g_hash(str)%(phf->bucket_num);
	vec_index=(phf->f1_hash(str)+ phf->f2_hash(str)*(phf->D[g_index] /base_num)+phf->D[g_index] %base_num) % phf->vector_num;
	if(judge_bit(phf->T,vec_index))
		return 1;
	return 0;
	
}


