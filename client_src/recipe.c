/*
 * recipe.c
 *
 * 
 */

#include "global.h"
FingerChunk * fingerchunk_new(){
	FingerChunk * fc=(FingerChunk*)malloc(sizeof(FingerChunk));
	memset(fc->fingerprint,0,sizeof(Fingerprint));
	fc->existed=0;
	fc->chunklen=0;
	fc->next=NULL;
	return fc;
}
void fingerchunk_free(FingerChunk * fc){
	free(fc);
}

Recipe* recipe_new() {
	Recipe *new_recipe = (Recipe*) malloc(sizeof(Recipe));
	new_recipe->chunknum = 0;
	new_recipe->fileindex = 0;
	new_recipe->first= NULL;
	new_recipe->last = NULL;
	new_recipe->flag=0;
	new_recipe->filename=(char *)calloc(1,FILE_NAME_LEN);
	//memset(new_recipe->filename, 0, FILE_NAME_LEN);
	new_recipe->next=NULL;
	return new_recipe;
}
Recipe* recipe_create(int flag,int fileindex){
	Recipe *new_recipe = (Recipe*) malloc(sizeof(Recipe));
	new_recipe->chunknum = 0;
	new_recipe->fileindex = fileindex;
	new_recipe->first= NULL;
	new_recipe->last = NULL;
	new_recipe->flag=flag;
	new_recipe->filename=NULL;
	new_recipe->next=NULL;
	return new_recipe;
}

void recipe_free(Recipe* trash) {
    FingerChunk *fchunk = trash->first;
    while(fchunk){
        trash->first = fchunk->next;
        free(fchunk);
        fchunk = trash->first;    
    } 
   if(trash->filename)
   	free(trash->filename);
   free(trash);
}

Recipe* recipe_append_fingerchunk(Recipe* semis, FingerChunk *fchunk) {
    fchunk->next = 0;
	if (!semis->first) {
		semis->first = fchunk;
	}else{
        semis->last->next = fchunk;
    }
       semis->last = fchunk;
	semis->chunknum++;
	return semis;
}

void recipe_check(Recipe *rp){
	FingerChunk *fc=rp->first;
	int totallen=0;
	char hash[41]={0};
	while(fc){
		digestToHash(fc->fingerprint,hash);
		printf("%s %d \n",hash,fc->chunklen);
		totallen+=fc->chunklen;
		fc=fc->next;
	}
	printf("totalen:%d \n  chunkcounts:%d  \n  avglen:%.2f\n",totallen,rp->chunknum,totallen*1.00/rp->chunknum);
}

