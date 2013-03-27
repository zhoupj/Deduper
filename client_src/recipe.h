/*
 * recipe.h
 *
 */

#ifndef RECIPE_H_
#define RECIPE_H_

#define FILE_NAME_LEN 256
typedef unsigned char Fingerprint[20];


struct finger_chunk_tag{
    Fingerprint fingerprint;
    char existed; /* existed */
    int chunklen;
    struct finger_chunk_tag *next;
};

typedef struct finger_chunk_tag FingerChunk;

struct recipe_tag {
	int flag;
	int fileindex;
	int chunknum;
	char* filename;
    	FingerChunk *first;
    	FingerChunk *last;
	//small files
	struct recipe_tag *next;
};

typedef struct recipe_tag Recipe;

#define FLAG_BEGAIN 1
#define FLAG_END 2

FingerChunk * fingerchunk_new();

void fingerchunk_free(FingerChunk * fc);
Recipe* recipe_new();
Recipe* recipe_create(int flag,int fileindex);

void recipe_free(Recipe* trash);
Recipe* recipe_append_fingerchunk(Recipe* semis, FingerChunk *fchunk);
void recipe_check(Recipe *rp);

#endif /* RECIPE_H_ */

