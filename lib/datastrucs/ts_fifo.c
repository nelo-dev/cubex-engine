#include "ts_fifo.h"
#include <stdlib.h>
#include <string.h>

int ts_fifo_queue_init(TS_FIFO_Queue *queue, size_t item_size, size_t initial_capacity) {
    if (!queue || item_size == 0 || initial_capacity == 0) return TS_FIFO_QUEUE_ERROR;

    queue->data = malloc(item_size * initial_capacity);
    if (!queue->data) return TS_FIFO_QUEUE_ERROR;

    queue->item_size = item_size;
    queue->capacity = initial_capacity;
    queue->count = 0;
    queue->front = 0;
    queue->back = 0;

    pthread_mutex_init(&queue->lock, NULL);
    pthread_cond_init(&queue->not_empty, NULL);
    pthread_cond_init(&queue->not_full, NULL);

    return TS_FIFO_QUEUE_SUCCESS;
}

void ts_fifo_queue_destroy(TS_FIFO_Queue *queue) {
    if (!queue) return;

    free(queue->data);
    pthread_mutex_destroy(&queue->lock);
    pthread_cond_destroy(&queue->not_empty);
    pthread_cond_destroy(&queue->not_full);
}

static int ts_fifo_queue_resize(TS_FIFO_Queue *queue) {
    size_t new_capacity = queue->capacity * 2;
    void *new_data = malloc(queue->item_size * new_capacity);
    if (!new_data) return TS_FIFO_QUEUE_ERROR;

    // Copy data in the correct order
    if (queue->front < queue->back) {
        memcpy(new_data, (char*)queue->data + queue->front * queue->item_size, queue->count * queue->item_size);
    } else {
        size_t front_part_size = (queue->capacity - queue->front) * queue->item_size;
        memcpy(new_data, (char*)queue->data + queue->front * queue->item_size, front_part_size);
        memcpy((char*)new_data + front_part_size, queue->data, queue->back * queue->item_size);
    }

    free(queue->data);
    queue->data = new_data;
    queue->capacity = new_capacity;
    queue->front = 0;
    queue->back = queue->count;

    return TS_FIFO_QUEUE_SUCCESS;
}

int ts_fifo_queue_enqueue(TS_FIFO_Queue *queue, const void *item) {
    pthread_mutex_lock(&queue->lock);

    while (queue->count == queue->capacity) {
        if (ts_fifo_queue_resize(queue) == TS_FIFO_QUEUE_ERROR) {
            pthread_mutex_unlock(&queue->lock);
            return TS_FIFO_QUEUE_ERROR;
        }
    }

    memcpy((char*)queue->data + queue->back * queue->item_size, item, queue->item_size);
    queue->back = (queue->back + 1) % queue->capacity;
    queue->count++;

    pthread_cond_signal(&queue->not_empty);
    pthread_mutex_unlock(&queue->lock);

    return TS_FIFO_QUEUE_SUCCESS;
}

#include "stdio.h"

int ts_fifo_queue_dequeue(TS_FIFO_Queue *queue, void *item) {
    pthread_mutex_lock(&queue->lock);

    while (queue->count == 0) {
        pthread_cond_wait(&queue->not_empty, &queue->lock);
    }

    memcpy(item, (char*)queue->data + queue->front * queue->item_size, queue->item_size);
    queue->front = (queue->front + 1) % queue->capacity;
    queue->count--;

    pthread_cond_signal(&queue->not_full);
    pthread_mutex_unlock(&queue->lock);

    return TS_FIFO_QUEUE_SUCCESS;
}

int ts_fifo_queue_try_dequeue(TS_FIFO_Queue *queue, void *item) {
    if (!queue || !item) return TS_FIFO_QUEUE_ERROR;

    pthread_mutex_lock(&queue->lock);

    if (queue->count == 0) {
        // Queue is empty, cannot dequeue
        pthread_mutex_unlock(&queue->lock);
        return TS_FIFO_QUEUE_ERROR;
    }

    // Copy the item from the front of the queue
    memcpy(item, (char*)queue->data + queue->front * queue->item_size, queue->item_size);
    // Update the front index and decrease the count
    queue->front = (queue->front + 1) % queue->capacity;
    queue->count--;

    // Signal that the queue is not full (in case producers are waiting)
    pthread_cond_signal(&queue->not_full);
    pthread_mutex_unlock(&queue->lock);

    return TS_FIFO_QUEUE_SUCCESS;
}

size_t ts_fifo_queue_count(TS_FIFO_Queue *queue) {
    if (!queue) return 0;

    pthread_mutex_lock(&queue->lock);
    size_t count = queue->count;
    pthread_mutex_unlock(&queue->lock);

    return count;
}