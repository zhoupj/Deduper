/*
 * recipe.c
 *
 * 
 */

#include "../global.h"
FingerChunk * fingerchunk_new(Fingerprint hash,char existed){
	FingerChunk * fc=(FingerChunk*)malloc(sizeof(FingerChunk));
	memcpy(fc->fingerprint,hash,sizeof(Fingerprint));
	fc->length=0;
	fc->offset=0;
	fc->existed=existed;
	fc->next=NULL;
	return fc;
}

void fingerchunk_free(FingerChunk * fc){
	free(fc);
}

Recipe* recipe_new() {
	Recipe *rp = (Recipe*) malloc(sizeof(Recipe));
	rp->chunknum = 0;
	rp->fileindex = 0;
	rp->first= NULL;
	rp->last = NULL;
	memset(rp->filename, 0, FILE_NAME_LEN);
	return rp;
}

void recipe_free(Recipe* trash) {
    FingerChunk *fchunk = trash->first;
    while(fchunk){
        trash->first = fchunk->next;
        free(fchunk);
        fchunk = trash->first;    
    }
	free(trash);
}

Recipe* recipe_append_fingerchunk(Recipe* semis, FingerChunk *fchunk) {
    fchunk->next = NULL;
	if (semis->first==NULL) {
		semis->first = fchunk;
	}else{
		if(semis->last==NULL)
			printf("NULLNULLNULLLUNULLLL\n");
       		 semis->last->next = fchunk;
        }
          semis->last = fchunk;
	semis->chunknum++;
	return semis;
}
