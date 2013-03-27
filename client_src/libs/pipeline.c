#include "../global.h"

PipelineQueue* pipeline_queue_init(int size){
    PipelineQueue *p_queue = (PipelineQueue*)malloc(sizeof(PipelineQueue));
    p_queue->queue = queue_new();
    p_queue->max_size = size;

    if(pthread_mutex_init(&p_queue->mutex, 0)
            || pthread_cond_init(&p_queue->max_work, 0) || pthread_cond_init(&p_queue->min_work, 0)){
        puts("Failed to init mutex or work in PipelineQueue!");
        return NULL;
    }
    return p_queue;
}

void pipeline_queue_destroy(PipelineQueue* p_queue){
    queue_free(p_queue->queue);
    pthread_mutex_destroy(&p_queue->mutex);
    pthread_cond_destroy(&p_queue->max_work);
    pthread_cond_destroy(&p_queue->min_work);
    free(p_queue);
}

void pipeline_queue_push(PipelineQueue* p_queue, void* item){
    if(pthread_mutex_lock(&p_queue->mutex) != 0){
        puts("failed to lock!");
        return;
    }

    while(p_queue->max_size>0&&queue_size(p_queue->queue) >= p_queue->max_size){
        pthread_cond_wait(&p_queue->max_work, &p_queue->mutex);
    }

    queue_push(p_queue->queue, item);

    pthread_cond_broadcast(&p_queue->min_work);

    if(pthread_mutex_unlock(&p_queue->mutex)){
        puts("failed to lock!");
        return;
    }
}

void* pipeline_queue_pop(PipelineQueue* p_queue){
    if(pthread_mutex_lock(&p_queue->mutex) != 0){
        puts("failed to lock!");
        return NULL;
    }

    while(queue_size(p_queue->queue) <= 0){
        pthread_cond_wait(&p_queue->min_work, &p_queue->mutex);
    }

    void * item = queue_pop(p_queue->queue);
    pthread_cond_broadcast(&p_queue->max_work);

    pthread_mutex_unlock(&p_queue->mutex);
    return item;
}
