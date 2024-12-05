
#include "chsys/mem.h"

#include "chrpc/channel.h"
#include "chrpc/channel_helpers.h"
#include <pthread.h>
#include <unistd.h>

static void *channel_echo_thread_loop(void *_arg) {
    // TODO: REWRITE THIS FUNCTION...
    channel_echo_thread_t *arg = (channel_echo_thread_t *)_arg;

    channel_status_t status;

    size_t mms;
    status = chn_max_msg_size(arg->chn, &mms);
    if (status != CHN_SUCCESS) {
        pthread_exit((void *)status);
    }

    void *buf = safe_malloc(mms * sizeof(uint8_t));

    bool should_stop;
    size_t readden;

    while (true) {
        pthread_mutex_lock(&(arg->mut));
        should_stop = arg->should_stop;
        pthread_mutex_unlock(&(arg->mut));

        if (should_stop) {
            status = CHN_SUCCESS;
            break;
        }

        status = chn_receive(arg->chn, buf, mms, &readden);

        if (status == CHN_SUCCESS) {
            // echo time!
            status = chn_send(arg->chn, buf, readden);
            if (status != CHN_SUCCESS) {
                break;
            }
        } else if (status == CHN_NO_INCOMING_MSG) {
            // Time to sleep and refresh!
            usleep(50);
            status = chn_refresh(arg->chn);
            if (status != CHN_SUCCESS) {
                break;
            }
        } else {
            // Return unexpected receive error.
            break;
        }
    }

    safe_free(buf);
    pthread_exit((void *)status);

    // Should never make it here.
    return NULL;
}

channel_echo_thread_t *new_channel_echo_thread(channel_t *chn) {
    channel_echo_thread_t *et = 
        (channel_echo_thread_t *)safe_malloc(sizeof(channel_echo_thread_t));
    // Maybe the buffer can be in here also???
    // might be worthwhile just saying...

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
    return CHN_MEM_ERROR;
}
