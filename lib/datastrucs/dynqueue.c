#include "dynqueue.h"
#include <stdlib.h>

// Create a new queue
Queue *queue_create() {
    Queue *queue = (Queue *)malloc(sizeof(Queue));
    if (!queue) return NULL;
    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;
    return queue;
}

// Destroy the queue and optionally free the data
void queue_destroy(Queue *queue, void (*free_data)(void *)) {
    if (!queue) return;

    QueueNode *current = queue->head;
    while (current) {
        QueueNode *next = current->next;
        if (free_data) {
            free_data(current->data);
        }
        free(current);
        current = next;
    }
    free(queue);
}

// Enqueue an element to the queue
QueueNode *queue_enqueue(Queue *queue, void *data) {
    if (!queue) return NULL;

    QueueNode *new_node = (QueueNode *)malloc(sizeof(QueueNode));
    if (!new_node) return NULL;

    new_node->data = data;
    new_node->prev = queue->tail;
    new_node->next = NULL;

    if (queue->tail) {
        queue->tail->next = new_node;
    } else {
        queue->head = new_node;
    }
    queue->tail = new_node;
    queue->size++;

    return new_node;
}

// Dequeue an element from the queue
void *queue_dequeue(Queue *queue) {
    if (!queue || !queue->head) return NULL;

    QueueNode *node = queue->head;
    void *data = node->data;

    queue->head = node->next;
    if (queue->head) {
        queue->head->prev = NULL;
    } else {
        queue->tail = NULL;
    }

    free(node);
    queue->size--;

    return data;
}

// Remove a specific node from the queue
bool queue_remove(Queue *queue, QueueNode *node) {
    if (!queue || !node) return false;

    if (node->prev) {
        node->prev->next = node->next;
    } else {
        queue->head = node->next;
    }

    if (node->next) {
        node->next->prev = node->prev;
    } else {
        queue->tail = node->prev;
    }

    free(node);
    queue->size--;

    return true;
}

// Get the size of the queue
size_t queue_size(const Queue *queue) {
    return queue ? queue->size : 0;
}