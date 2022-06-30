#include "queue.h"

struct queue *init_queue(unsigned int capacity)
{
    struct queue *queue;
    if((queue = malloc(sizeof(struct queue))) == NULL)
        return NULL;

    if((queue->array = malloc(capacity * sizeof(void *))) == NULL)
    {
        free(queue);
        return NULL;
    }
    queue->front = queue->rear = 0;
    queue->capacity = capacity;
    sem_init(&queue->mutex, 0, 1);
    sem_init(&queue->slots, 0 , capacity);
    sem_init(&queue->items, 0, 0);

    return queue;
}

void free_queue(struct queue * queue)
{
    if(!queue)
        return;
    free(queue->array);
    free(queue);
}


int push(struct queue *queue, void *item)
{
    if(!queue)
        return 0;
    sem_wait(&queue->slots);
    sem_wait(&queue->mutex);
    queue->array[(++queue->rear) % queue->capacity] = item;
    sem_post(&queue->mutex);
    sem_post(&queue->items);
    return 1;
}
void *pop(struct queue *queue)
{
    void *item;
    if(!queue)
        return NULL;
    //Used to wait for item
    sem_wait(&queue->items);
    sem_wait(&queue->mutex);
    item = queue->array[(++queue->front) % queue->capacity];
    sem_post(&queue->mutex);
    sem_post(&queue->slots);
    return item;
}