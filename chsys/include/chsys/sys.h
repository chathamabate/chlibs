
#ifndef CHSYS_SYS_H
#define CHSYS_SYS_H

#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>

// NOTE: Be aware by defualt the system ignores:
//
// SIGINT and SIGPIPE.
//
// If a SIGINT is received, a worker thread will handle it and exit.
// Nothing happens when a SIGPIPE is received.

// An error during setup will result in exit.
// A message will be printed, but not in a pretty way!
void sys_init(void);

bool sys_sig_exit_requested(void);

// If set to true, the process as a whole will exit when certain signals are received.
// If given, the sig exit routine will be called.
//
// If set to false, sig_exit_requested will return true... the signal handling thread will exit.
void sys_set_sig_exit(bool e);

// Remeber, when a signal is received that triggers a safe exit, it is not handled
// by a signal handler, but rather an independent thread.
//
// If you'd like the thread to run some action before exiting, use this function
// to specify the action.
//
// NOTE: Very important, to make life easier, the routine is NOT
// executed while holding the system lock.
void sys_sig_exit_routine(void (*routine)(void *), void *arg);

// The system state contains a lock.
// You can aquire/release said lock using these calls!
// If given acquire lock is false, these two below functions are no-ops.
void sys_lock_p(bool acquire_lock);
static inline void sys_lock(void) {
    sys_lock_p(true);
}

void sys_unlock_p(bool acquire_lock);
static inline void sys_unlock(void) {
    sys_unlock_p(true);
}

void sys_set_quiet_p(bool acquire_lock, bool q);
static inline void sys_set_quiet(bool q) {
    sys_set_quiet_p(true, q);
}

bool sys_is_quiet_p(bool acquire_lock);
static inline bool sys_is_quiet(void) {
    return sys_is_quiet_p(true);
}

void sys_inc_malloc_count_p(bool acquire_lock);
static inline void sys_inc_malloc_count(void) {
    sys_inc_malloc_count_p(true);
}

void sys_dec_malloc_count_p(bool acquire_lock);
static inline void sys_dec_malloc_count(void) {
    sys_dec_malloc_count_p(true);
}

size_t sys_get_malloc_count_p(bool acquire_lock);
static inline size_t sys_get_malloc_count(void) {
    return sys_get_malloc_count_p(true);
}

void sys_reset_malloc_count_p(bool acquire_lock);
static inline void sys_reset_malloc_count(void) {
    sys_reset_malloc_count_p(true);
}

pid_t safe_fork(void);
pid_t safe_waitpid(pid_t pid, int *wstatus, int options);

void safe_exit_p(bool acquire_lock, int status);
static inline void safe_exit(int status) {
    safe_exit_p(true, status);
}

#endif
