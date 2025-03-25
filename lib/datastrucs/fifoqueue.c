#include "fifoqueue.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

// Forward declarations
int fifo_queue_is_empty(const FifoQueue *queue);

// Internal helper to resize the queue when needed
static int fifo_queue_resize(FifoQueue *queue) {
    // Prevent size overflow
    if (queue->capacity > SIZE_MAX / 2) {
        fprintf(stderr, "Error: Cannot resize FIFO queue beyond size %zu.\n", queue->capacity);
        return -1;
    }

    size_t new_capacity = queue->capacity * 2;
    void *new_data = malloc(new_capacity * queue->element_size);
    if (!new_data) {
        fprintf(stderr, "Error: Failed to allocate memory for resizing FIFO queue.\n");
        return -1; // Allocation failed
    }

    // Copy elements to the new buffer
    if (queue->front < queue->back) {
        // Data is contiguous
        memcpy(new_data, (char*)queue->data + queue->front * queue->element_size, queue->size * queue->element_size);
    } else {
        // Data wraps around, copy in two parts
        size_t front_part_size = (queue->capacity - queue->front) * queue->element_size;
        memcpy(new_data, (char*)queue->data + queue->front * queue->element_size, front_part_size);
        memcpy((char*)new_data + front_part_size, queue->data, queue->back * queue->element_size);
    }

    free(queue->data);
    queue->data = new_data;
    queue->capacity = new_capacity;
    queue->front = 0;
    queue->back = queue->size;

    return 0;
}

// Create a new FIFO queue
FifoQueue* fifo_queue_create(size_t element_size, size_t initial_capacity) {
    if (element_size == 0) {
        fprintf(stderr, "Error: Element size must be greater than 0.\n");
        return NULL;
    }

    FifoQueue *queue = (FifoQueue*)malloc(sizeof(FifoQueue));
    if (!queue) {
        fprintf(stderr, "Error: Failed to allocate memory for FIFO queue.\n");
        return NULL;
    }

    queue->element_size = element_size;
    queue->capacity = initial_capacity > 0 ? initial_capacity : 1024;
    queue->size = 0;
    queue->front = 0;
    queue->back = 0;

    queue->data = malloc(queue->capacity * element_size);
    if (!queue->data) {
        fprintf(stderr, "Error: Failed to allocate memory for FIFO queue data.\n");
        free(queue);
        return NULL;
    }

    return queue;
}

// Destroy the queue
void fifo_queue_destroy(FifoQueue *queue) {
    if (queue) {
        free(queue->data);
        free(queue);
    }
}

// Check if the queue is empty
int fifo_queue_is_empty(const FifoQueue *queue) {
    return queue->size == 0;
}

// Enqueue an element into the queue
int fifo_queue_enqueue(FifoQueue *queue, const void *element) {
    if (!queue || !element) {
        fprintf(stderr, "Error: Invalid arguments to fifo_queue_enqueue.\n");
        return -1;
    }

    if (queue->size == queue->capacity) {
        // Resize the queue when full
        if (fifo_queue_resize(queue) != 0) {
            return -1; // Resize failed
        }
    }

    memcpy((char*)queue->data + (queue->back * queue->element_size), element, queue->element_size);
    queue->back = (queue->back + 1) % queue->capacity;
    queue->size++;

    return 0;
}

// Dequeue an element from the queue
int fifo_queue_dequeue(FifoQueue *queue, void *element_out) {
    if (!queue) {
        fprintf(stderr, "Error: Invalid argument to fifo_queue_dequeue.\n");
        return -1;
    }

    if (fifo_queue_is_empty(queue)) {
        return -1; // Queue is empty
    }

    if (element_out) {
        memcpy(element_out, (char*)queue->data + (queue->front * queue->element_size), queue->element_size);
    }

    queue->front = (queue->front + 1) % queue->capacity;
    queue->size--;

    return 0;
}

// Get the current size of the queue
size_t fifo_queue_size(const FifoQueue *queue) {
    if (!queue) {
        fprintf(stderr, "Error: Invalid argument to fifo_queue_size.\n");
        return 0;
    }
    return queue->size;
}
