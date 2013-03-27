
#include "../global.h"



int readn(int fd, char *vptr, int n)
{
   int    nleft;
    int   nread;
    char    *ptr;
    ptr = vptr;
    nleft = n;
    while (nleft > 0) 
	{
        if ( (nread = read(fd, ptr, nleft)) < 0) 
		{ //当read返回－1
            if (errno == EINTR)
                nread = 0; /*若出错是因为被信号中断，继续读*/
            else
                return(ERROR);   //若不是因为被信号中断而出错，则不再读readn返回－1
        } else if (nread == 0)  //当read返回0时，不再读下去，readn返回已经读的字节数
            break;  /* EOF */
        nleft -= nread;//当read返回值>0但不等于n时，继续读
        ptr += nread;
    }
    return(n - nleft);  /* return >= 0 */
}

int writen(int fd, const char *vptr, int n)
{
    int        nleft;
    int       nwritten;
    const char    *ptr;
    ptr = vptr;
    nleft = n;
    while (nleft > 0) 
	{                                        
        if ( (nwritten = write(fd, ptr, nleft)) <= 0) 
		{//当write出错返回－1或者返回0时，
            if (nwritten < 0 && errno == EINTR)
                nwritten = 0;  /*若出错返回－1是因为被信号中断，继续写 */
            else
                return(ERROR);//若不是因为被信号中断而出错返回－1或者 返回0时，则不再写writen返回－1
        }
        nleft -= nwritten; //当write返回值>0但不等于n时，继续写
        ptr += nwritten;
    }
    return(n);
}
 
 int bnet_send(int fd, char* msg, int len) {
	 int net_order_size = htonl(len);
	 int wn=0;
	 if (writen(fd, &net_order_size, sizeof(int)) != sizeof(int)) {
		  printf("%s,%d failed to send msg length!\n",__FILE__,__LINE__);
		 return ERROR;
	 }
	 wn=writen(fd,msg,len);
	 if(wn!=len)
		 printf("%s,%d writen is wrong(%d !=%d )\n",__FILE__,__LINE__,wn,len);
	//msg[len]=0;
	// printf("\033[40;32m send: %s (%d) \033[0m\n",msg,len);
	return wn;
 }
 
 /* you must ensure the msg has enough space to contain the data bytes */
 int bnet_recv(int fd,char *msg, int *len) {
	 int host_order_size;
	 int rn=0;
	 if (readn(fd, &host_order_size,sizeof(int)) != sizeof(int)) {
		 printf("%s,%d failed to recv msg length or shutdown by other side!\n",__FILE__,__LINE__);
		 return ERROR;
	 }
	 host_order_size=ntohl(host_order_size);
	 if (host_order_size <=0) {
		 *len =host_order_size;
		 //printf("recv an end signal!\n");
		 return host_order_size;
	 }
	 *len=host_order_size;
	 rn=readn(fd,msg,*len);
	if(rn==ERROR)
		return ERROR;
	 if(rn !=*len)
		 printf("%s,%d readn is wrong\n",__FILE__,__LINE__);
	 msg[rn]=0;
	//  printf("\033[40;32m recv %s (%d) \033[0m\n",msg,*len);
	 return rn;
 }


int bnet_signal(int fd, int sig) {
	int net_order_size = htonl(sig);
    if (writen(fd, &net_order_size, sizeof(int)) != sizeof(int)) {
        printf("failed to send signal!\n");
        return ERROR;
    }
    return 0;
}

void set_recvbuf_size(int fd, int size){
	
	if(setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size)) < 0)
		err_msg1("set recv buffer wrong");
}
void set_sendbuf_size(int fd, int size){
	
	if(setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size)) < 0)
		err_msg1("set send buffer wrong");
}
void get_socket_default_bufsize(int fd){
	
	  int snd_size = 0;	/* 发送缓冲区大小 */ 
	  int rcv_size = 0;	/* 接收缓冲区大小 */ 
	  socklen_t optlen;	/* 选项值长度 */ 
	  int err=0;
	  optlen = sizeof(snd_size); 
	 err = getsockopt(fd, SOL_SOCKET, SO_SNDBUF,&snd_size, &optlen); 
	 if(err<0){ 
		err_msg1("get system socket buffer wrong"); 
	 }
	 
	 optlen = sizeof(rcv_size); 
	 err = getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rcv_size, &optlen); 
	 if(err<0){ 
		err_msg1("get system socket buffer wrong"); 
	 } 
  	printf(" default socket send buffer: %d \n",snd_size); 
	printf(" default socket recv buffer: %d \n",rcv_size); 
}


