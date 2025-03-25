#ifndef FIFO_QUEUE_H
#define FIFO_QUEUE_H

#include <stddef.h>
#include <stdint.h>

// FIFO Queue Structure
typedef struct {
    void *data;          // Pointer to the underlying data array
    size_t element_size; // Size of a single element
    size_t capacity;     // Maximum number of elements the queue can hold
    size_t front;        // Index of the front element
    size_t back;         // Index of the back element (one past the last element)
    size_t size;         // Current number of elements in the queue
} FifoQueue;

// Function declarations
FifoQueue* fifo_queue_create(size_t element_size, size_t initial_capacity);
void fifo_queue_destroy(FifoQueue *queue);
int fifo_queue_enqueue(FifoQueue *queue, const void *element);
int fifo_queue_dequeue(FifoQueue *queue, void *element_out);
int fifo_queue_is_empty(const FifoQueue *queue);
size_t fifo_queue_size(const FifoQueue *queue);

#endif // FIFO_QUEUE_H