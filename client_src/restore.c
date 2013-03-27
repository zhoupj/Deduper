
#include "global.h"


int recv_files(JCR *psJcr);

int restore_client(int sock, char* arg) {
	
	char buf[256]={0};
	int length;
	JCR *jcr = jcr_new();
	jcr->data_socket= sock;
	

	/* jobid and restore path*/
	
	if (sscanf(arg, "%d,%s", &jcr->id, jcr->restore_path) == 2) {

	} else if (sscanf(arg, "%d", &jcr->id) == 1) {
		jcr->restore_path[0] = 0;
	} else {
		puts("illigal parameters!");
		goto FAIL;
	}
	
	TIMER_DECLARE(start,end);
	TIMER_START(start);
	sprintf(buf, restore_cmd, jcr->id);
	bnet_send(jcr->data_socket, buf, strlen(buf) );
	// receive pathname
	if (bnet_recv(jcr->data_socket, jcr->backup_path, &length) <= 0) {
		printf("%s, %d: failed to recv backup path!\n", __FILE__, __LINE__);
		goto FAIL;
	}
	if (length < 0 || length>256) {
		printf("no such job!\n");
		goto FAIL;
	}

	length=strlen(jcr->restore_path);
	if (length>0){
		if(jcr->restore_path[length-1]!='/')
			strcat(jcr->restore_path, "/");
	} else
		strcpy(jcr->restore_path, jcr->backup_path); /* restore to the old path */

	printf("restore jobid=%d,path=%s\n",jcr->id,jcr->restore_path);

	if (recv_files(jcr) != 0) {
		goto FAIL;
	}

	TIMER_END(end);
	TIMER_DIFF(jcr->total_time,start,end);
	printf("number_of_files: %d \n", jcr->file_count);
	printf("chunks_of_files: %d \n", jcr->chunk_count);
	printf("restore time:   %.3fs  %ld %.3f MB/s\n", jcr->total_time,jcr->old_size,jcr->old_size*1.0/jcr->total_time/1000000);
	
	jcr_free( jcr);
	return 0;
	FAIL:
		jcr_free( jcr);
		return -1;
		
}

int recv_files(JCR *psJcr) {
	char buf[SOCKET_BUF_SIZE+10]={0};
	char filename[400];;
	int buflen=0;
	char *p, *q;
	int fd=-1;
	int len = strlen(psJcr->restore_path);
	q = psJcr->restore_path+ 1;/* ignore the first char*/
	
	int blen=strlen(psJcr->backup_path);
	/*
	 * recursively make directory
	 */
	while ((p = strchr(q, '/'))) {
		if (*p == *(p - 1)) {
			q++;
			continue;
		}
		*p = 0;
		if (access(psJcr->restore_path, 0) != 0) {
			mkdir(psJcr->restore_path, S_IRWXU | S_IRWXG | S_IRWXO);
		}
		*p = '/';
		q = p + 1;
	}

	while (bnet_recv(psJcr->data_socket, buf, &buflen) !=ERROR) { // file name
		if(buflen==STREAM_END){
			printf("%s,%d restore is over\n",__FILE__,__LINE__);
			break;
		}
		psJcr->file_count++;
		/*if (sscanf(buf, "%s", filename) != 1) {
			printf("%s, %d: failed to resolve filename!\n", __FILE__, __LINE__);
			return -1;
		}*/
		strcpy(filename, psJcr->restore_path);
		strcat(filename, buf+blen);
		
		q = filename + len;

		while ((p = strchr(q, '/'))) {
			if (*p == *(p - 1)) {
				q++;
				continue;
			}
			*p = 0;
			if (access(filename, 0) != 0) {
				mkdir(filename, S_IRWXU | S_IRWXG | S_IRWXO);
			}
			*p = '/';
			q = p + 1;
		}
		if(G_VERBOSE)
			printf("restore file %s \n",filename);
		
		fd = open(filename, O_CREAT | O_TRUNC | O_WRONLY,00644);

		while (bnet_recv(psJcr->data_socket, buf, &buflen) >0) { //recv data

			if(writen(fd,buf,buflen)!=buflen)
				printf("%s, %d: failed to write filedata!\n", __FILE__, __LINE__);
			
           		psJcr->chunk_count++;
			psJcr->old_size+=buflen;

		}
		close(fd);
	}
	return 0;
}

