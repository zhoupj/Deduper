/*
 * 
 */

#ifndef JOBMANAGE_H_
#define JOBMANAGE_H_


typedef struct job_des { 
	int32_t nJobId;
	int isDel;
	char szBackupPath[ FILE_NAME_LEN];
	int32_t nFileNum;
	int fd;
	//int64_t writeoffset;
	//int64_t readoffset;
} JOB_V;

#define JOB_V_HEAD_SIZE sizeof(JOB_V);

int jobcount_init() ;
int  jobcount_close();
int jobcount_max();


JOB_V *jobv_new(char * path);

JOB_V *jobv_open(int id);

void jobv_insert_recipe(JOB_V * jv,Recipe *rp);
Recipe * jobv_search_next_recipe(JOB_V*jv);

void jobv_destroy(JOB_V *jv);


#endif /* JOBMANAGE_H_ */

