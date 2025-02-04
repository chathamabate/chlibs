
#include "chsys/sys.h"

#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>

#include <stdint.h>

#include "chsys/log.h"

// mean to only be used during setup.
#define ERROR_OUT(...) \
    do {\
        printf(__VA_ARGS__);\
        exit(1);\
    } while (0)

// Core linked list for holding children.
typedef struct _child_node_t {
    pid_t pid;
    struct _child_node_t *next;
} child_node_t;

typedef struct _sys_state_t {
    bool quiet;

    void *signal_exit_routine_arg;
    void (*signal_exit_routine)(void *);

    // NOTE: this counter will keep track of user mallocs only.
    // All mallocs within this file are not counted!
    size_t malloc_count;
    child_node_t *child_list;
} sys_state_t;

static pthread_mutex_t sys_mut;
static sys_state_t *ss = NULL;

static void *sig_thread(void *arg) {
    (void)arg;

    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);

    int sig; // Not really needed, unsure if you can pass NULL
             // into sigwait tho.
    int s;

    // Wait for just one SIGINT.
    s = sigwait(&set, &sig);     
    if (s) {
        log_fatal_p(true, "Error from sigwait. (%d)", s);
    }

    log_info("SIGINT received");

    sys_lock();
    void *exit_rout_arg = ss->signal_exit_routine_arg;
    void (*exit_rout)(void *) = ss->signal_exit_routine;
    sys_unlock();

    if (exit_rout) {
        log_info("Performing SIGINT exit routine");
        exit_rout(exit_rout_arg);
    }

    safe_exit(0);

    // Will never make it here.
    return NULL;
}

static void spawn_sig_thread_p(bool acquire_lock) {
    pthread_t st;
    int s = pthread_create(&st, NULL, sig_thread, NULL);
    if (s) {
        log_fatal_p(acquire_lock, "Failed to create signal handling thread. (%d)", s);
    }
}

void sys_init(void) {
    int s;

    if (ss) {
        // We can call log here since our system is already setup!
        log_fatal_p(true, "System attempting to be init'd more than once");
    }

    s = pthread_mutex_init(&(sys_mut), NULL);
    if (s) {
        ERROR_OUT("Could not init system state mutex\n");
    }

    ss = malloc(sizeof(sys_state_t));
    if (!ss) {
        ERROR_OUT("Could not malloc system state\n");
    }

    ss->quiet = 0;
    ss->signal_exit_routine_arg = NULL;
    ss->signal_exit_routine = NULL;
    ss->malloc_count = 0;
    ss->child_list = NULL;

    // We've initialized our system state!
    // Now we can call log!

    // All threads will block SIGINT and SIGPIPE
    // The signal thread will be the only one which actually
    // services the pending signal.
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGPIPE);
    s = pthread_sigmask(SIG_BLOCK, &set, NULL);
    if (s) {
        log_fatal_p(true, "Failed to block SIGINT and SIGPIPE. (%d)", s);
    }

    // Spawn our signal handling thread. 
    // (THIS SHOULD ALWAYS BE THE LAST ACTION OF THIS FUNCITON)
    spawn_sig_thread_p(true);
}

void sys_sig_exit_routine(void (*routine)(void *), void *arg) {
    sys_lock();
    ss->signal_exit_routine = routine;
    ss->signal_exit_routine_arg = arg;
    sys_unlock();
}

void sys_lock_p(bool acquire_lock) {
    if (acquire_lock) {
        pthread_mutex_lock(&(sys_mut));
    }
}

void sys_unlock_p(bool acquire_lock) {
    if (acquire_lock) {
        pthread_mutex_unlock(&(sys_mut));
    }
}

void sys_set_quiet_p(bool acquire_lock, bool q) {
    sys_lock_p(acquire_lock);
    ss->quiet = q;
    sys_unlock_p(acquire_lock);
}

bool sys_is_quiet_p(bool acquire_lock) {
    bool res;

    sys_lock_p(acquire_lock);
    res = ss->quiet;
    sys_unlock_p(acquire_lock);

    return res;
}

void sys_inc_malloc_count_p(bool acquire_lock) {
    sys_lock_p(acquire_lock);
    if (ss->malloc_count == SIZE_MAX) {
        log_fatal_p(false, "Malloc overflow");
    }
    ss->malloc_count++;
    sys_unlock_p(acquire_lock);
}

void sys_dec_malloc_count_p(bool acquire_lock) {
    sys_lock_p(acquire_lock);
    if (ss->malloc_count == 0) {
        log_fatal_p(false, "Malloc underflow");
    }
    ss->malloc_count--;
    sys_unlock_p(acquire_lock);
}

size_t sys_get_malloc_count_p(bool acquire_lock) {
    size_t mc;

    sys_lock_p(acquire_lock);
    mc = ss->malloc_count;
    sys_unlock_p(acquire_lock);

    return mc;
}

void sys_reset_malloc_count_p(bool acquire_lock) {
    sys_lock_p(acquire_lock);
    ss->malloc_count = 0;
    sys_unlock_p(acquire_lock);
}

// This should be called after a fork within the child process.
// We must have the lock when we call this function.
// We will release the lock before returning.
//
// NOTE: I have learned that only the calling thread is duplicated over
// to the child process. So, we are responsiblle for recreating the 
// SIGINT handling thread.
static int prepare_from_child(void) {
    child_node_t *iter = ss->child_list;
    child_node_t *temp;
    while (iter) {
        temp = iter->next;
        free(iter);
        iter = temp;
    }

    ss->child_list = NULL;

    sys_unlock_p(true);

    // Spawn our signal thread.
    spawn_sig_thread_p(true);

    return 0;
}

pid_t safe_fork(void) {
    // Acquire the system lock before forking!
    sys_lock_p(true);

    pid_t pid = fork();
    
    if (pid < 0) {
        log_fatal_p(false, "Failed to fork");
    }

    if (pid == 0) {
        return prepare_from_child();
    }

    // parent process.
    child_node_t *node = malloc(sizeof(child_node_t));

    if (!node) {
        // Since our new child was never put in the list...
        // gotta kill/wait on it here manually.
        kill(pid, SIGINT);
        waitpid(pid, NULL, 0);

        log_fatal_p(false, "Unable to malloc child node");
    }

    node->pid = pid;
    node->next = ss->child_list;
    ss->child_list = node;

    sys_unlock_p(true);
    
    return pid;
}

pid_t safe_waitpid(pid_t pid, int *wstatus, int options) {
    pid_t p = waitpid(pid, wstatus, options);

    if (p < 0) {
        log_fatal_p(true, "Failed to waitpid");
    }

    // WNOHANG case.
    if (p == 0) {
        return 0;
    }

    // Process was reaped! let's remove it from our child list.

    sys_lock_p(true);


    child_node_t *prev = NULL;
    child_node_t *iter = ss->child_list;

    while (iter && iter->pid != p) {
        prev = iter;
        iter = prev->next;
    }

    if (!iter) {
        log_fatal_p(false, "Reaped child not found in child list");
    }

    if (prev) {
        prev->next = iter->next;
    } else {
        ss->child_list = iter->next;
    }

    free(iter);

    sys_unlock_p(true);
    return p;
}

void safe_exit_p(bool acquire_lock, int status) {
    sys_lock_p(acquire_lock);

    // NOTE: Log fatal calls this function, so we cannot call log fatal within exit.

    // Check malloc count.
    if (ss->malloc_count > 0) {
        log_warn_p(false, "Process exiting with memory leak. (%zu)", ss->malloc_count);
    }

    // kill all children. 
    child_node_t *temp;
    child_node_t *iter = ss->child_list;

    while (iter) {
        temp = iter->next;
        
        log_info_p(false, "Killing and reaping process with pid=%d", iter->pid);
        kill(iter->pid, SIGINT);
        waitpid(iter->pid, NULL, 0);

        // Finally free our iter.
        free(iter);

        iter = temp;
    }

    free(ss); 
    
    // NOTE: We exit while holding our lock!
    exit(status);
}
