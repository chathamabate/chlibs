
#ifndef CHRPC_SERIAL_HELPERS_H
#define CHRPC_SERIAL_HELPERS_H

#include "chrpc/serial_value.h"
#include "chutil/string.h"
#include "chutil/stream.h"

#include "chrpc/serial_type.h"

const char *chrpc_primitive_type_label(const chrpc_type_t *ct);

// The "to_stream" functions are really meant for debugging.
// They print a human readable version of the given argument.
//
// NOTE: The "human readable" syntax is arbitrary, values cannot be parsed
// from this syntax. It is just for conveinence.

stream_state_t chrpc_type_to_stream(out_stream_t *os, const chrpc_type_t *ct, bool spaced);
stream_state_t chrpc_inner_value_to_stream(out_stream_t *os, const chrpc_type_t *ct, const chrpc_inner_value_t *iv, bool spaced);
stream_state_t chrpc_value_to_stream(out_stream_t *os, const chrpc_value_t *v, bool spaced);

void chrpc_type_print(const chrpc_type_t *ct, bool spaced);
void chrpc_inner_value_print(const chrpc_type_t *ct, const chrpc_inner_value_t *iv, bool spaced);
void chrpc_value_print(const chrpc_value_t *v, bool spaced);

#endif
