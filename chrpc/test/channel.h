
#ifndef TEST_CHRPC_CHANNEL_H
#define TEST_CHRPC_CHANNEL_H

#include "chrpc/channel.h"

// This function will run a series of tests on the given channel.
//
// NOTE: It expects that the given channel will echo back what is sent.
void test_chn_echo(channel_t *chn);

#endif
