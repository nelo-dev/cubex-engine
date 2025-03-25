#ifndef TS_FIFO_QUEUE_H
#define TS_FIFO_QUEUE_H

#include <stddef.h>
#include <pthread.h>

#define TS_FIFO_QUEUE_SUCCESS 0
#define TS_FIFO_QUEUE_ERROR -1

typedef struct {
    void *data;            // Pointer to the queue data
    size_t item_size;      // Size of each item
    size_t capacity;       // Current capacity of the array
    size_t count;          // Current number of elements in the queue
    size_t front;          // Index of the front element
    size_t back;           // Index of the back element
    pthread_mutex_t lock;  // Mutex for thread safety
    pthread_cond_t not_empty; // Condition variable for dequeue
    pthread_cond_t not_full;  // Condition variable for enqueue
} TS_FIFO_Queue;

// Initializes the queue with a specified item size and initial capacity
int ts_fifo_queue_init(TS_FIFO_Queue *queue, size_t item_size, size_t initial_capacity);

// Frees any resources used by the queue
void ts_fifo_queue_destroy(TS_FIFO_Queue *queue);

// Adds an item to the back of the queue (thread-safe)
int ts_fifo_queue_enqueue(TS_FIFO_Queue *queue, const void *item);

// Removes an item from the front of the queue (thread-safe)
int ts_fifo_queue_dequeue(TS_FIFO_Queue *queue, void *item);

size_t ts_fifo_queue_count(TS_FIFO_Queue *queue);

int ts_fifo_queue_try_dequeue(TS_FIFO_Queue *queue, void *item);

#endif // TS_FIFO_QUEUE_H

