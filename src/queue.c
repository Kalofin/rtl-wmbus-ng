
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

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

void queue_init(Queue *q) {
    q->head = q->tail = NULL;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->cond, NULL);
}

void queue_destroy(Queue *q) {
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->cond);
}

void enqueue(Queue *q, void *payload) {
    // allocate a new node
    Node *node = malloc(sizeof(Node));
    if (!node) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    // Add the pointer to the payload
    node->payload = payload;
    // make this the last node in the queue
    node->next = NULL;

    // lock to ensure we are the only one to modify the queue
    pthread_mutex_lock(&q->mutex);
    // if there are elements in the queue
    if (q->tail) {
        // add the node to the end
        q->tail->next = node;
    } else {
        // no elements in the queue, make this node the first one
        q->head = node;
    }
    // make this node the last one in the queue
    q->tail = node;
    // release locked
    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->mutex);
}

void *dequeue(Queue *q) {
    // lock to ensure we are the only one to modify the queue
    pthread_mutex_lock(&q->mutex);
    // While there are no elements in the queue ...
    while (q->head == NULL) {
        // temporarily release the lock, wait for the signal in q->cond, and reaquire the lock
        pthread_cond_wait(&q->cond, &q->mutex);
    }
    // get the first node in the queue
    Node *node = q->head;
    // advance the queue heade
    q->head = node->next;
    //If the que got empty, updated the queue tail pointer
    if (q->head == NULL) {
        q->tail = NULL;
    }
    // release the lock
    pthread_mutex_unlock(&q->mutex);

    // get the pointer of the payload in this node
    void *payload = node->payload;
    // dispose the node
    free(node);
    // and return the pointer to the payload
    return payload;
}
