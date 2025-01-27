
#include "chsys/wrappers.h"
#include "chsys/log.h"

#include <pthread.h>

void safe_pthread_create(pthread_t *t, const pthread_attr_t *attr, 
        void *(*start_routine)(void *), void *arg) {
    if (pthread_create(t, attr, start_routine, arg)) {
        log_fatal("Failed to spawn thread");        
    }
}

void safe_pthread_join(pthread_t t, void **ret_val) {
    if (pthread_join(t, ret_val)) {
        log_fatal("Failed to join thread");
    }
}

void safe_pthread_mutex_init(pthread_mutex_t *m, const pthread_mutexattr_t *attr) {
    if (pthread_mutex_init(m, attr)) {
        log_fatal("Failed to init mutex");
    }
}

void safe_pthread_mutex_destroy(pthread_mutex_t *m) {
    if (pthread_mutex_destroy(m)) {
        log_fatal("Failed to destory mutex");
    }
}

void safe_pthread_mutex_lock(pthread_mutex_t *m) {
    if (pthread_mutex_lock(m)) {
        log_fatal("Failed to lock on mutex");
    }
}

void safe_pthread_mutex_unlock(pthread_mutex_t *m) {
    if (pthread_mutex_unlock(m)) {
        log_fatal("Failed to unlock on mutex");
    }
}
