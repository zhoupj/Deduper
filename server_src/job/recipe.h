/*
 * recipe.h
 *
 */

#ifndef RECIPE_H_
#define RECIPE_H_

#define FILE_NAME_LEN 256


struct finger_chunk_tag{
    Fingerprint fingerprint;
    int64_t offset;
    int  length;
    char existed; /* existed */
    struct finger_chunk_tag *next;
};

typedef struct finger_chunk_tag FingerChunk;

struct recipe_tag {
	int fileindex;
	int chunknum;
	char filename[256];
    	FingerChunk *first;
    	FingerChunk *last;
};

typedef struct recipe_tag Recipe;

FingerChunk * fingerchunk_new(Fingerprint hash,char existed);
void fingerchunk_free(FingerChunk * fc);
Recipe* recipe_new();
void recipe_free(Recipe* trash);
Recipe* recipe_append_fingerchunk(Recipe* semis, FingerChunk *fchunk);

#endif /* RECIPE_H_ */

