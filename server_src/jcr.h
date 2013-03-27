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

	int64_t nRecvSize; /* 接收数据的总大小*/
	
	int32_t nChunkCount;
	int32_t nDedupChunkCount;
	
		
	JOB_V *jobv;
	MemIndex *memIndex;
	Container* container;
	
	double searchTime; // 查询指纹所用时间
	double writeDataTime;  // 写数据时间
	double writeRecipeTime; // 写文件元数据的时间
	double recvTime;
	double totalTime;


} JCR;

JCR* jcr_new() ;
void jcr_free(JCR* jcr) ;


#endif /* JCR_H_ */

