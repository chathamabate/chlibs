
#ifndef CHUTIL_QUEUE_H
#define CHUTIL_QUEUE_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

// This will be a constant sized queue implementation.
// Cyclic.

typedef struct _queue_t {
    size_t cap;
    size_t len;
    size_t cell_size;

    // Index of next element to return from this queue.
    // (Only valid if len > 0).
    size_t i;

    // size = cell_size * cap.
    void *table;
} queue_t;

queue_t *new_queue(size_t cap, size_t cs);
void delete_queue(queue_t *q);

static inline size_t q_cap(queue_t *q) {
    return q->cap;
}

static inline size_t q_len(queue_t *q) {
    return q->len;
}

static inline bool q_full(queue_t *q) {
    return q->cap == q->len;
}

static inline void *q_peek(queue_t *q) {
    if (q->len == 0) {
        return NULL;
    }

    return (void *)(((uint8_t *)(q->table)) + (q->cell_size * q->i));
}

//  These return 0 on success. 1 if full (for push) or empty (for pop)
int q_push(queue_t *q, const void *src);

// dest CAN be NULL here.
int q_poll(queue_t *q, void *dest);

#endif
