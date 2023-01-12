#include "producer-consumer.h"

int pcq_create(pc_queue_t *queue, size_t capacity){
    pthread_mutex_init(&queue->pcq_current_size_lock, NULL);
    pthread_mutex_init(&queue->pcq_head_lock, NULL);
    pthread_mutex_init(&queue->pcq_tail_lock, NULL);
    pthread_mutex_init(&queue->pcq_pusher_condvar_lock, NULL);
    pthread_cond_init(&queue->pcq_pusher_condvar, NULL);
    pthread_mutex_init(&queue->pcq_popper_condvar_lock, NULL);
    pthread_cond_init(&queue->pcq_popper_condvar, NULL);
    queue->pcq_capacity = capacity;
    queue->pcq_head = queue->pcq_current_size = 0;
    queue->pcq_tail = capacity - 1;
    queue->pcq_buffer = malloc(queue->pcq_capacity * sizeof(void*));
    for(int i = 0; i < queue->pcq_capacity; i++) {
        queue->pcq_buffer[i] = (void*)malloc(3000); // nao compreendo a parte do malloc ao pcq_buffer
    }
}

int pcq_destroy(pc_queue_t *queue){
    for(int i = 0; i < queue->pcq_capacity; i++) {
        free(queue->pcq_buffer[i]);
    }
    free(queue->pcq_buffer);
}

int pcq_enqueue(pc_queue_t *queue, void *elem){
    if (queue->pcq_current_size == queue->pcq_capacity){
        sleep(5);
    }   
    queue->pcq_tail = (queue->pcq_tail + 1) % queue->pcq_capacity;
    queue->pcq_buffer[queue->pcq_tail] = elem;
    queue->pcq_current_size = queue->pcq_current_size + 1;
}

void *pcq_dequeue(pc_queue_t *queue){
    if (queue->pcq_current_size == 0){
        sleep(5);
    }
    void * elem = queue->pcq_buffer[queue->pcq_head];
    queue->pcq_head = (queue->pcq_head + 1) % queue->pcq_capacity;
    queue->pcq_current_size = queue->pcq_current_size - 1;
    return elem;
}