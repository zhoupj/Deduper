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
	
	psJcr->read_time=0; // 查询指纹所用时间
	psJcr->chunk_time=0;  // 写数据时间
	psJcr->sha_time=0; // 写文件元数据的时间
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

