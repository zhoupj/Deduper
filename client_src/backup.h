#ifndef BACKUP_H_
#define BACKUP_H_

/// Data:2012/12/29
/// Author: zhoupj

int backup_client(int socket, char *path);
int walk_dir(JCR *psJcr, char *path);
int send_file(JCR *psJcr, char *path);
void chunk_file(JCR *jcr,Recipe *rp);
void send_finger(JCR *jcr,Recipe *rp);
void recv_finger_rsp(JCR *jcr,Recipe *rp);
void send_data(JCR *jcr,Recipe *rp);
void send_file_data(JCR *jcr,char * filename); //not deduplication


#endif

