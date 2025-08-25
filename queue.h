#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <pthread.h>

typedef struct Node {
    void *payload;
    struct Node *next;
} Node;

typedef struct {
    Node *head;
    Node *tail;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} Queue;


void queue_init(Queue *q);
void queue_destroy(Queue *q);
void enqueue(Queue *q, void *payload);
void *dequeue(Queue *q);

#endif
