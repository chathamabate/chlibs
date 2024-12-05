
#include "chsys/mem.h"

#include "chrpc/channel.h"
#include "chrpc/channel_helpers.h"

#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>

static channel_status_t _channel_echo_thread_loop(channel_echo_thread_t *et, void *buf, size_t buf_len) {
    size_t readden;
    channel_status_t status;

    const struct timespec slp_amt = {
        .tv_sec = 0,
        .tv_nsec = 10000000 // Should be 10 ms
    };

    // Somewhat unused.
    struct timespec rem;

    while (!chn_et_should_stop(et)) {
        TRY_CHANNEL_CALL(chn_refresh(et->chn));
        
        status = chn_incoming_len(et->chn, &readden);
        if (status == CHN_NO_INCOMING_MSG) {
            nanosleep(&slp_amt, &rem);
            continue;
        }

        // Some unexpected error.
        if (status != CHN_SUCCESS) {
            return status;
        }

        // Should never happen, but still just to be safe.
        if (readden > buf_len) {
            return CHN_BUFFER_TOO_SMALL;
        }

        TRY_CHANNEL_CALL(chn_receive(et->chn, buf, buf_len, &readden));

        // Finally, echo back.
        TRY_CHANNEL_CALL(chn_send(et->chn, buf, readden));
    }

    return CHN_SUCCESS;
}

static void *channel_echo_thread_loop(void *arg) {
    channel_echo_thread_t *et = (channel_echo_thread_t *)arg;
    channel_status_t status;

    size_t mms;
    status = chn_max_msg_size(et->chn, &mms);
    if (status != CHN_SUCCESS) {
        return (void *)status;
    }

    void *buf = safe_malloc(mms * sizeof(uint8_t));
    status = _channel_echo_thread_loop(et, buf, mms);
    safe_free(buf);

    return (void *)status;
}

channel_echo_thread_t *new_channel_echo_thread(channel_t *chn) {
    channel_echo_thread_t *et = 
        (channel_echo_thread_t *)safe_malloc(sizeof(channel_echo_thread_t));

    pthread_mutex_init(&(et->mut), NULL);
    et->should_stop = false;
    et->chn = chn;

    int create_error = pthread_create(&(et->thread), NULL, 
            channel_echo_thread_loop, (void *)et);

    if (create_error != 0) {
        pthread_mutex_destroy(&(et->mut));
        safe_free(et);
        return NULL;
    }

    return et;
}

channel_status_t delete_channel_echo_thread(channel_echo_thread_t *chn_et) {
    chn_et_stop(chn_et); 

    channel_status_t exit_status;
    int thread_error = pthread_join(chn_et->thread, (void **)&exit_status);

    // Now, regardless of the join status, let's just free all memory used.
    pthread_mutex_destroy(&(chn_et->mut));
    safe_free(chn_et);

    if (thread_error != 0) {
        return CHN_UNKNOWN_ERROR;
    }

    return exit_status;
}
