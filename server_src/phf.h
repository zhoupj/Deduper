#ifndef PHF_H_
#define PHF_H_

typedef struct FP{
	unsigned char hash[21];
	struct FP * next;
}BucketData;

typedef struct {
	int index;
	int size;
	BucketData *data;
}Bucket;

typedef  unsigned int hash_fun(char *str);

typedef struct {
	int64_t finger_num;
	int64_t bucket_num;
	int64_t vector_num;
	Bucket * B;
	uint16_t * D;  // intermidiate value
	uint8_t * T;// storing vector result
	hash_fun * g_hash;
	hash_fun * f1_hash;
	hash_fun * f2_hash;	
}PHF;

PHF * phf_init(int64_t max_finger_number);
void phf_destory(PHF * phf);
void phf_walk(PHF * phf);

int phf_input(PHF * phf, Fingerprint  fp);
void phf_build(PHF * phf);
void phf_mark(PHF *phf,Fingerprint fp);
int phf_search(PHF *phf,Fingerprint fp);

#endif
