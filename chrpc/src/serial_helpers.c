
#include "chrpc/serial_helpers.h"
#include "chrpc/serial_type.h"
#include "chutil/stream.h"
#include "chutil/string.h"


const char *chrpc_primitive_type_label(const chrpc_type_t *ct) {
    switch (ct->type_id) {
    case CHRPC_BYTE_TID:
        return "b8";
    case CHRPC_INT16_TID:
        return "i16";
    case CHRPC_INT32_TID:
        return "i32";
    case CHRPC_INT64_TID:
        return "i64";
    case CHRPC_UINT16_TID:
        return "u16";
    case CHRPC_UINT32_TID:
        return "u32";
    case CHRPC_UINT64_TID:
        return "u64";
    case CHRPC_FLOAT32_TID:
        return "f32";
    case CHRPC_FLOAT64_TID:
        return "f64";
    case CHRPC_STRING_TID:
        return "str";
    default:
        return NULL;
    }
}

#define CHRPC_SERIAL_TAB "  "
#define TAB_OUT(os, spaced, tabs) \
    if (spaced) { \
        for (size_t __ti = 0; __ti < (tabs); __ti++) { \
            OS_PUTS(os, CHRPC_SERIAL_TAB) ; \
        } \
    }

static stream_state_t _chrpc_type_to_stream(out_stream_t *os, const chrpc_type_t *ct, bool spaced, size_t tabs) {

    if (chrpc_type_is_primitive(ct)) {
        TAB_OUT(os, spaced, tabs);
        return os_puts(os, chrpc_primitive_type_label(ct));
    }

    if (ct->type_id == CHRPC_ARRAY_TID) {
        TRY_STREAM_CALL(_chrpc_type_to_stream(os, ct->array_cell_type, spaced, tabs));
        return os_puts(os, "[]");
    }

    // Otherwise a struct.

    TAB_OUT(os, spaced, tabs);
    OS_PUTC(os, '{');

    if (spaced) {
        OS_PUTC(os, '\n');
    }

    for (uint8_t fi = 0; fi < ct->struct_fields_types->num_fields; fi++) {
        const chrpc_type_t *ft = ct->struct_fields_types->field_types[fi];
        TRY_STREAM_CALL(_chrpc_type_to_stream(os, ft, spaced, tabs + 1));

        OS_PUTC(os, ';');

        if (spaced) {
            OS_PUTC(os, '\n');
        }
    }

    TAB_OUT(os, spaced, tabs);
    OS_PUTC(os, '}');

    return STREAM_SUCCESS;
}

stream_state_t chrpc_type_to_stream(out_stream_t *os, const chrpc_type_t *ct, bool spaced) {
    return _chrpc_type_to_stream(os, ct, spaced, 0);
}

void chrpc_type_print(const chrpc_type_t *ct) {
    string_t *s = new_string();
    out_stream_t *os = new_out_stream_to_string(s);

    stream_state_t ss = chrpc_type_to_stream(os, ct, true);

    if (ss == STREAM_SUCCESS) {
        printf("%s\n", s_get_cstr(s));
    }

    delete_out_stream(os);
    delete_string(s);
}


