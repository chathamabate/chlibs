
#include "chutil/queue.h"
#include "chsys/mem.h"
#include <string.h>
#include <stdint.h>

queue_t *new_queue(size_t cap, size_t cs) {
    if (cap == 0 || cs == 0) {
        return NULL;
    }

    queue_t *q = (queue_t *)safe_malloc(sizeof(queue_t));

    q->cap = cap;
    q->cell_size = cs;
    q->i = 0;
    q->len = 0;

    q->table = safe_malloc(q->cell_size * q->cap);

    return q;
}

void delete_queue(queue_t *q) {
    safe_free(q->table);
    safe_free(q);
}

int q_push(queue_t *q, const void *src) {
    if (q_full(q) || !src) {
        return 1;
    }

    size_t push_ind = (q->i + q->len) % q->cap;
    
    void *dest_cell = (void *)((uint8_t *)(q->table) + (push_ind * q->cell_size));
    memcpy(dest_cell, src, q->cell_size);
    q->len++;

    return 0;
}

int q_pop(queue_t *q, void *dest) {
    if (q->len == 0) {
        return 1;
    }

    if (dest) {
        size_t pop_ind = q->i;
        void *src_cell = (void *)((uint8_t *)(q->table) + (pop_ind * q->cell_size));
        memcpy(dest, src_cell, q->cell_size);
    }

    q->i++;
    if (q->i == q->cap) {
        q->i = 0;
    }

    q->len--;
    
    return 0;
}
