#include "global.h" 


#define SERVER_PORT 8888
#define SERVER_OTH_PORT 8889
/* example:
   ./client -q
   ./client -b /root/zpj/abc
   ./client -r 2
   ./client -d 2
*/
enum OPER_TYPE {
	type_backup=1,
	type_restore,
	type_delete,
	type_quit,
	type_help,
	type_test
};
char SERVER_IP[30]; 

#define chunk_size 64*1024
void test_data(int socket,char *filename){
	TIMER_DECLARE(start,end);
	TIMER_DECLARE(Rstart,Rend);
	TIMER_DECLARE(Sstart,Send);
	char  buf[chunk_size+1];
	int readlen=0;
	double total_time=0;
	double send_time=0;
	double read_time=0;
	double total_len=0;
	int  fd=-1;
	if ((fd=open(filename, O_RDONLY)) < 0) {
		 printf("%s,%d open file error!\n",__FILE__,__LINE__);
		return;
	 }
	TIMER_START(start);
	while(1){
		TIMER_START(Rstart);
		if((readlen=readn(fd,buf,chunk_size))<=0)
			break;
		TIMER_END(Rend);
		TIMER_DIFF(read_time,Rstart,Rend);
		TIMER_START(Sstart);
		bnet_send(socket,buf,readlen);
		TIMER_END(Send);
		TIMER_DIFF(send_time,Sstart,Send);
		total_len+=readlen;
	}
	TIMER_END(end);
	TIMER_DIFF(total_time,start,end);
	close(fd);
	close(socket);
	printf("read time=%.4f  %.4fMB/s\n",read_time,total_len/read_time/1036288);
	printf("send time=%.4f  %.4fMB/s\n",send_time,total_len/send_time/1036288);
	printf("total time=%.4f  %.4fMB/s\n", total_time,total_len/total_time/1036288);
}

static void usage()
{
        fprintf(stderr, "Usage: ./deduper_client [OPTION...] [FILE]...\n");
        fprintf(stderr, "Examples:\n");
        fprintf(stderr, "  ./deduper_client -b /root/zpj     # backup file or dirctory \n");
        fprintf(stderr, "  ./deduper_client -p -b /root/zpj  # backup file or dirctory with pipeline method \n");
        fprintf(stderr, "  ./deduper_client -r 1,/root/zpj/  # restore job 1, restore to directory /root/zpj/ \n");
        fprintf(stderr, "  ./deduper_client -t filename      # test the network bandwidth  \n");
        fprintf(stderr, "  -H host                           # connect the destination host (ip address)  \n");
        fprintf(stderr, "  -h, --help    give this help list\n\n");
        fprintf(stderr, "Report bugs to <zhoupj1987@163.com>.\n\n");
}

int main(int argc, char **argv) {

     int fd ,ip,fd_oth;
     struct sockaddr_in s_addr,s_addr_oth;
      int opt;
    enum OPER_TYPE type;
    char path[300];
    memset(path,0,300);
	

  if(argc<2)
  	return -1;
  memset(SERVER_IP,0,30);
  strncpy(SERVER_IP,"192.168.1.64",12);
  
  while((opt=getopt(argc, argv, "qb:r:d:ps:t:hH:ov"))!=-1){
        switch(opt){
	   case 'p':
	  	G_PIPELINE=1;
		break;
	   case 's':
	   	MAX_FINERS_NUM=atoi(optarg)*1024*1024;
		break;
            case 'q':
		type=type_quit;
		break;
            case 'b':
	  	type=type_backup;
		strncpy(path,optarg,strlen(optarg));
		break;
            case 'r':
               	type=type_restore;
		strncpy(path,optarg,strlen(optarg));
		  break;
            case 'd':
              	type=type_delete;
		strncpy(path,optarg,strlen(optarg));
		break;
	  case 't':
	  	 type=type_test;
		 strncpy(path,optarg,strlen(optarg));
		 break;
	   case 'h':
	   	 type=type_help;
		 break;
	  case 'v':
	  	G_VERBOSE=1;
	    break;
      case 'H':
	  	memset(SERVER_IP,0,30);
	  	 strncpy(SERVER_IP,optarg,strlen(optarg));
	  	 break;
	case 'o':
		G_NOT_DEDUP=1;
		break;
	   default:
	   	printf("Your command is wrong \n");
		type=type_help;
		break;
        }
    }
     printf("max_fingers_num:%d\n",MAX_FINERS_NUM);
     printf("server_ip=%s\n",SERVER_IP);

   if(type==type_help){
   	usage();
	return;
   }

  fd= socket(AF_INET, SOCK_STREAM, 0);
   inet_pton(AF_INET,SERVER_IP,&ip);
  s_addr.sin_family = AF_INET;
  s_addr.sin_addr.s_addr = ip;
  s_addr.sin_port = htons(SERVER_PORT);
  
  if (connect(fd, (struct sockaddr*) &s_addr, sizeof(struct sockaddr))	!= 0) {
	 err_msg1("connection rejected!");
	  return 0;
  }
   if(G_PIPELINE){
  
	  fd_oth= socket(AF_INET, SOCK_STREAM, 0);
		  inet_pton(AF_INET,SERVER_IP,&ip);
	  s_addr_oth.sin_family = AF_INET;
		  s_addr_oth.sin_addr.s_addr = ip;
		  s_addr_oth.sin_port = htons(SERVER_OTH_PORT);
  
	  if (connect(fd_oth, (struct sockaddr*) &s_addr_oth, sizeof(struct sockaddr))	!= 0) {
			 err_msg1("connection rejected!");
				 return 0;
	  }
   }
  if(type==type_test){
  	test_data(fd,path);
	return;
  }
 
    switch(type){
	    case type_quit:
                	     bnet_signal(fd,SYSTEM_QUIT);
                	     break;
            case type_backup:
	   	     if(G_PIPELINE)
			 backup_pipeline(fd, fd_oth,path);
	     		 else
                			 backup_client(fd, path);
               		 break;
            case  type_restore:
                   restore_client(fd, path);
                   break;
            case type_delete:
                  delete_client(fd, path);
                break;
				
	   default:
	   	printf("Your command is wrong \n");
		break;
    	}

  
  
    close(fd);
    if(G_PIPELINE)
	close(fd_oth);
    return 0;
}
