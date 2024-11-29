
#ifndef TEST_CHRPC_CHANNEL_H
#define TEST_CHRPC_CHANNEL_H

#include <stdint.h>
#include "chrpc/channel.h"

// This function will run a series of tests on the given channel.
//
// NOTE: It expects that the given channel will echo back what is sent.
void test_chn_echo(channel_t *chn, uint32_t wait_amt);

// This function also expects the channel to echo.
// It does not test a diverse set of message lengths,
// rather it will test the mms repeatedly.
void test_chn_stressful_echo(channel_t *chn, uint32_t wait_amt);

#endif
