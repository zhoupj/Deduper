/*

 */

#include "global.h"

#include <stdlib.h>

JCR* jcr_new() {
	JCR *psJcr = (JCR*) malloc(sizeof(JCR));
	psJcr->id=0;
	psJcr->data_socket=-1;
	psJcr->finger_socket=-1;
	psJcr->file_count=0;
	psJcr->old_size=0;
	psJcr->dedup_size=0;
	
	psJcr->chunk_count=0;
	psJcr->dedup_chunk_count=0;
	
	psJcr->read_time=0; // ��ѯָ������ʱ��
	psJcr->chunk_time=0;  // д����ʱ��
	psJcr->sha_time=0; // д�ļ�Ԫ���ݵ�ʱ��
	psJcr->send_time=0;
	psJcr->search_time=0;
	psJcr->read2_time=0;
	psJcr->total_time=0;

	memset(psJcr->backup_path,0,256);
	memset(psJcr->restore_path,0,256);
	return psJcr;
}

void jcr_free(JCR* jcr) {
	free(jcr);
}

