#ifndef JCR_H_
#define JCR_H_

typedef struct jcr {
	int32_t id;
	int data_socket;
	int finger_socket;
	int32_t file_count;

	int64_t old_size;
	int64_t dedup_size;

	int32_t chunk_count;
	int32_t dedup_chunk_count;

	double read_time;        // 查重前, 以64KB 为单位读的时间 
	double chunk_time;   // 分块时间
	double sha_time;       //求指纹时间
	double search_time; // 发送指纹，接收指纹查重后的结构总共时间
	double read2_time;  // 查重后，以分块大小，读取文件的时间
	double send_time;   // 发送数据时间
	double total_time;    // 整个备份所用时间
	char backup_path[256];
	char restore_path[256];

} JCR;

JCR* jcr_new() ;
 void jcr_free(JCR* jcr);


#endif /* JCR_H_ */

