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
	int               valid;           /* queue initialized 队列是否初始化*/
	int               quit;            /* workq should quit 队列是否退出，1代表退出，0代表存在*/
	int               max_workers;     /* max threads 最大线程数*/
	int               num_workers;     /* current threads 当前运行的线程数 */
	int               idle_workers;    /* idle threads 闲置的线程 */
	void             *(*engine)(void *arg); /* user engine 用户处理函数 */
} workq_t;

/*初始化线程池结构的数据成员*/
int workq_init(workq_t *wq, int threads, void *(*engine)(void *arg));

/*向线程池中的任务队列中添加一个队列，并试图执行它*/
int workq_add(workq_t *wq, void *element, workq_ele_t **work_item, int priority);

/* 从任务队列中移除一个队列，并马上执行它*/
int workq_remove(workq_t *wq, workq_ele_t *work_item);

/*销毁线程池结构*/
int workq_destroy(workq_t *wq);

/*任务处理函数*/
void *workq_server(void *arg);


#endif /* __THREAD_H */
