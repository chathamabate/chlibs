
#ifndef CHSYS_WRAPPERS_H
#define CHSYS_WRAPPERS_H

// Here are a few wrappers for common system functions.
// They exit the system and print a message if things go wrong.

// They aren't used everywhere. In honesty, I introduced these pretty late.
// However, they for functions where you don't want to write a bunch
// of error checking code.

#include <pthread.h>

void safe_pthread_create(pthread_t *t, const pthread_attr_t *attr, 
        void *(*start_routine)(void *), void *arg);

void safe_pthread_join(pthread_t t, void **ret_val);

void safe_pthread_mutex_init(pthread_mutex_t *m, const pthread_mutexattr_t *attr);
void safe_pthread_mutex_destroy(pthread_mutex_t *m);

void safe_pthread_mutex_lock(pthread_mutex_t *m);
void safe_pthread_mutex_unlock(pthread_mutex_t *m);

#endif
