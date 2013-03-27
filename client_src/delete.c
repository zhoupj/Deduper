#include "global.h"


int delete_client(int sock, char* arg) {
	
	char buf[256]={0};
	int len=0;
	int jobid;
	if (sscanf(arg, "%d", &jobid) != 1) {
		err_msg1("cmd wrong");
		return 1;
	}
	sprintf(buf, delete_cmd, jobid);
	printf("%s \n",buf);
	bnet_send(sock, buf, strlen(buf));		
	
	if(bnet_recv(sock,buf,&len)>0){
		printf("%s \n",buf);
	}
	else
		err_msg1("delete job  fail");
	
		
	return 0;
}


