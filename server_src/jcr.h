#ifndef JCR_H_
#define JCR_H_

typedef struct jcr {
	int32_t nJobId;
	int dataSocket;
	int fingerSocket;
	char szBackupPath[256];
	char szRestorePath[256];
	int32_t nFileCount;

	int64_t nSize;
	int64_t nDedupSize;

	int64_t nRecvSize; /* �������ݵ��ܴ�С*/
	
	int32_t nChunkCount;
	int32_t nDedupChunkCount;
	
		
	JOB_V *jobv;
	MemIndex *memIndex;
	Container* container;
	
	double searchTime; // ��ѯָ������ʱ��
	double writeDataTime;  // д����ʱ��
	double writeRecipeTime; // д�ļ�Ԫ���ݵ�ʱ��
	double recvTime;
	double totalTime;


} JCR;

JCR* jcr_new() ;
void jcr_free(JCR* jcr) ;


#endif /* JCR_H_ */

