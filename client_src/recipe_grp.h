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
	char *filename;
    	FingerChunk *first;
    	FingerChunk *last;
	struct recipe_tag *next;
};

typedef struct recipe_tag Recipe;

struct recipe_grp_tag{
	int finger_count;
	int max_count;
	Recipe * first;
	Recipe *last;
};
typedef struct recipe_grp_tag RecipeGrp;


#define FLAG_BEGAIN 1
#define FLAG_END 2

FingerChunk * fingerchunk_new();
void fingerchunk_free(FingerChunk * fc);
Recipe* recipe_new();
void recipe_free(Recipe* trash);
Recipe* recipe_create(int flag,int fileindex);

Recipe* recipe_append_fingerchunk(Recipe* semis, FingerChunk *fchunk);
void recipe_check(Recipe *rp);
RecipeGrp * recipe_grp_new(int max_count);
RecipeGrp *recipegrp_apppend_recipe(RecipeGrp *rgp,Recipe *rp);


#endif /* RECIPE_H_ */

