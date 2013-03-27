/*

 */

#include "global.h"

#include <stdlib.h>

JCR* jcr_new() {
	JCR *psJcr = (JCR*) malloc(sizeof(JCR));
	psJcr->nJobId=0;
	psJcr->dataSocket=-1;
	psJcr->fingerSocket=-1;
	memset(psJcr->szBackupPath,0,256);
	memset( psJcr->szRestorePath,0,256);
	psJcr->nFileCount=0;
	psJcr->nSize=0;
	psJcr->nDedupSize=0;
	psJcr->nRecvSize=0;
	
	psJcr->nChunkCount=0;
	psJcr->nDedupChunkCount=0;
	
		
	psJcr->jobv=NULL;
	psJcr->memIndex=NULL;
	psJcr->container=NULL;
	
	psJcr->searchTime=0; // ��ѯָ������ʱ��
	psJcr->writeDataTime=0;  // д����ʱ��
	psJcr->writeRecipeTime=0; // д�ļ�Ԫ���ݵ�ʱ��
	psJcr->recvTime=0;
	psJcr->totalTime=0;
	return psJcr;
}

void jcr_free(JCR* jcr) {
	free(jcr);
}

