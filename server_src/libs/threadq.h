#ifndef __THREAD_H
#define __THREAD_H 1

/* thread pool */
/*
 * Structure to keep track of work queue request
 */
typedef struct workq_ele_tag {
   struct workq_ele_tag *next;
   void                 *data;
} workq_ele_t;

/*
 * Structure describing a work queue
 */
typedef struct workq_tag {
	pthread_mutex_t   mutex;           /* queue access control */
	pthread_cond_t    work;            /* wait for work */
	pthread_attr_t    attr;            /* create detached threads */
	workq_ele_t       *first, *last;   /* work queue */
	int               valid;           /* queue initialized �����Ƿ��ʼ��*/
	int               quit;            /* workq should quit �����Ƿ��˳���1�����˳���0�������*/
	int               max_workers;     /* max threads ����߳���*/
	int               num_workers;     /* current threads ��ǰ���е��߳��� */
	int               idle_workers;    /* idle threads ���õ��߳� */
	void             *(*engine)(void *arg); /* user engine �û������� */
} workq_t;

/*��ʼ���̳߳ؽṹ�����ݳ�Ա*/
int workq_init(workq_t *wq, int threads, void *(*engine)(void *arg));

/*���̳߳��е�������������һ�����У�����ͼִ����*/
int workq_add(workq_t *wq, void *element, workq_ele_t **work_item, int priority);

/* ������������Ƴ�һ�����У�������ִ����*/
int workq_remove(workq_t *wq, workq_ele_t *work_item);

/*�����̳߳ؽṹ*/
int workq_destroy(workq_t *wq);

/*��������*/
void *workq_server(void *arg);


#endif /* __THREAD_H */
