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

	double read_time;        // ����ǰ, ��64KB Ϊ��λ����ʱ�� 
	double chunk_time;   // �ֿ�ʱ��
	double sha_time;       //��ָ��ʱ��
	double search_time; // ����ָ�ƣ�����ָ�Ʋ��غ�Ľṹ�ܹ�ʱ��
	double read2_time;  // ���غ��Էֿ��С����ȡ�ļ���ʱ��
	double send_time;   // ��������ʱ��
	double total_time;    // ������������ʱ��
	char backup_path[256];
	char restore_path[256];

} JCR;

JCR* jcr_new() ;
 void jcr_free(JCR* jcr);


#endif /* JCR_H_ */

