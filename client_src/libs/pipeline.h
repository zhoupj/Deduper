#ifndef PIPELINE_H_
#define PIPELINE_H_
/*
 * describe data structure for pipeline
 */

typedef struct {
    Queue *queue;
    int max_size;/* the max size of queue */
    pthread_mutex_t mutex;
    pthread_cond_t max_work;
    pthread_cond_t min_work;
}PipelineQueue;

PipelineQueue* pipeline_queue_init(int);
void pipeline_queue_destroy(PipelineQueue*);
void pipeline_queue_push(PipelineQueue*, void*);
void* pipeline_queue_pop(PipelineQueue*);

#endif
