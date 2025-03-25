#ifndef DYNAMIC_QUEUE_H
#define DYNAMIC_QUEUE_H

#include <stddef.h>
#include <stdbool.h>

// Define a node in the queue
typedef struct QueueNode {
    void *data;
    struct QueueNode *prev;
    struct QueueNode *next;
} QueueNode;

// Define the queue structure
typedef struct {
    QueueNode *head;
    QueueNode *tail;
    size_t size;
} Queue;

// Function declarations
Queue *queue_create();
void queue_destroy(Queue *queue, void (*free_data)(void *));
QueueNode *queue_enqueue(Queue *queue, void *data);
void *queue_dequeue(Queue *queue);
bool queue_remove(Queue *queue, QueueNode *node);
size_t queue_size(const Queue *queue);

#endif // DYNAMIC_QUEUE_H
