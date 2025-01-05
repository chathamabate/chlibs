

#include "chsys/mem.h"
#include "chsys/log.h"
#include "chsys/sys.h"
#include <stdlib.h>


void *safe_malloc_p(bool acquire_lock, size_t s) {
    if (s == 0) {
        return NULL;
    }

    void *mem = malloc(s);
    if (!mem) {
        log_fatal_p(acquire_lock, "Failed to malloc");
    }
    sys_inc_malloc_count_p(acquire_lock);

    return mem;
}

void *safe_realloc_p(bool acquire_lock, void *mem, size_t s) {
    if (s == 0) {
        safe_free(mem);
        return NULL;
    }

    if (!mem) {
        return safe_malloc_p(acquire_lock, s);
    }

    void *new_mem = realloc(mem, s);
    if (!new_mem) {
        log_fatal_p(acquire_lock, "Failed to realloc");
    }

    return new_mem;
}

void safe_free_p(bool acquire_lock, void *mem) {
    if (!mem) {
        return;
    }

    sys_dec_malloc_count_p(acquire_lock);
    free(mem);
}
