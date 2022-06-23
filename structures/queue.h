#ifndef _QUEUE_H_
#define _QUEUE_H_
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>

struct queue {
    int front, rear;
    unsigned int capacity;
    void **array;
    sem_t mutex;
    sem_t slots;
    sem_t items;
};
//Initialize queue and queue array
struct queue *init_queue(unsigned int capacity);
void free_queue(struct queue * queue);
int push(struct queue * queue, void *item);
void *pop(struct queue *queue);
#endif