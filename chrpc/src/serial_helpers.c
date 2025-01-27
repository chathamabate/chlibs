
#include "chrpc/serial_helpers.h"
#include "chrpc/serial_type.h"
#include "chrpc/serial_value.h"
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

static stream_state_t _chrpc_inner_value_to_stream(out_stream_t *os, const chrpc_type_t *ct, const chrpc_inner_value_t *iv, 
        bool spaced, size_t tabs);

// Here ct is the cell type of an array pointed to by iv.
static stream_state_t chrpc_inner_array_value_to_stream(out_stream_t *os, const chrpc_type_t *ct, const chrpc_inner_value_t *iv, 
        bool spaced, size_t tabs) {

    TAB_OUT(os, spaced, tabs);

    if (iv->array_len == 0) {
        OS_PUTS(os, "[]");
        return STREAM_SUCCESS;
    }

    // Otherwise, there are elements.

    OS_PUTC(os, '[');

    if (spaced) {
        OS_PUTC(os, '\n');
    }

    for (uint32_t i = 0; i < iv->array_len; i++) {
        bool buf_used = true;
        char buf[40];

        // This is kinda annoying IMO.
        switch (ct->array_cell_type->type_id) {
        case CHRPC_BYTE_TID:
            sprintf(buf, "0x%02X", iv->b8_arr[i]);
            break;
        case CHRPC_INT16_TID:
            sprintf(buf, "%d", iv->i16_arr[i]);
            break;
        case CHRPC_INT32_TID:
            sprintf(buf, "%d", iv->i32_arr[i]);
            break;
        case CHRPC_INT64_TID:
            sprintf(buf, "%lld", iv->i64_arr[i]);
            break;
        case CHRPC_UINT16_TID:
            sprintf(buf, "%u", iv->u16_arr[i]);
            break;
        case CHRPC_UINT32_TID:
            sprintf(buf, "%u", iv->u32_arr[i]);
            break;
        case CHRPC_UINT64_TID:
            sprintf(buf, "%llu", iv->u64_arr[i]);
            break;
        case CHRPC_FLOAT32_TID:
            sprintf(buf, "%f", iv->f32_arr[i]);
            break;
        case CHRPC_FLOAT64_TID:
            sprintf(buf, "%lf", iv->f64_arr[i]);
            break;

        case CHRPC_STRING_TID:
            buf_used = false;
            TAB_OUT(os, spaced, tabs + 1);
            OS_PUTC(os, '"');
            OS_PUTS(os, iv->str_arr[i]);
            OS_PUTC(os, '"');
            break;

        case CHRPC_STRUCT_TID:
        case CHRPC_ARRAY_TID:
            buf_used = false;
            TRY_STREAM_CALL(_chrpc_inner_value_to_stream(os, ct->array_cell_type, iv->array_entries[i], 
                        spaced, tabs + 1));
            break;

        default: // Should never make it here.
            return STREAM_ERROR;
        };

        if (buf_used) {
            TAB_OUT(os, spaced, tabs + 1);
            OS_PUTS(os, buf);
        }

        if (i < iv->array_len - 1) {
            OS_PUTC(os, ',');
        }

        if (spaced) {
            OS_PUTC(os, '\n');
        }
    }

    TAB_OUT(os, spaced, tabs);
    OS_PUTC(os, ']');

    return CHRPC_SUCCESS;
}

static stream_state_t _chrpc_inner_value_to_stream(out_stream_t *os, const chrpc_type_t *ct, const chrpc_inner_value_t *iv, 
        bool spaced, size_t tabs) {

    bool buf_used = true;
    char buf[40];

    switch (ct->type_id) {
    case CHRPC_BYTE_TID:
        sprintf(buf, "0x%02Xu", iv->b8);
        break;
    case CHRPC_INT16_TID:
        sprintf(buf, "%d", iv->i16);
        break;
    case CHRPC_INT32_TID:
        sprintf(buf, "%d", iv->i32);
        break;
    case CHRPC_INT64_TID:
        sprintf(buf, "%lld", iv->i64);
        break;
    case CHRPC_UINT16_TID:
        sprintf(buf, "%u", iv->u16);
        break;
    case CHRPC_UINT32_TID:
        sprintf(buf, "%u", iv->u32);
        break;
    case CHRPC_UINT64_TID:
        sprintf(buf, "%llu", iv->u64);
        break;
    case CHRPC_FLOAT32_TID:
        sprintf(buf, "%f", iv->f32);
        break;
    case CHRPC_FLOAT64_TID:
        sprintf(buf, "%lf", iv->f64);
        break;

    case CHRPC_STRING_TID:
        buf_used = false;
        TAB_OUT(os, spaced, tabs);
        OS_PUTC(os, '"');
        OS_PUTS(os, iv->str);
        OS_PUTC(os, '"');
        break;

    case CHRPC_STRUCT_TID:
        buf_used = false;

        TAB_OUT(os, spaced, tabs);
        OS_PUTC(os, '{');
        if (spaced) {
            OS_PUTC(os, '\n');
        }

        for (uint8_t fi = 0; fi < ct->struct_fields_types->num_fields; fi++) {
            TRY_STREAM_CALL(_chrpc_inner_value_to_stream(os, ct->struct_fields_types->field_types[fi], 
                        iv->struct_entries[fi], spaced, tabs + 1));

            OS_PUTC(os, ';');
            if (spaced) {
                OS_PUTC(os, '\n');
            }
        }

        TAB_OUT(os, spaced, tabs);
        OS_PUTC(os, '}');
        break;


    case CHRPC_ARRAY_TID:
        buf_used = false;
        TRY_STREAM_CALL(chrpc_inner_array_value_to_stream(os, ct, iv, spaced, tabs));
        break;

    default: // Should never make it here.
        return STREAM_ERROR;
    }

    if (buf_used) {
        TAB_OUT(os, spaced, tabs);
        OS_PUTS(os, buf);
    }

    return CHRPC_SUCCESS;
}

stream_state_t chrpc_inner_value_to_stream(out_stream_t *os, const chrpc_type_t *ct, const chrpc_inner_value_t *iv, bool spaced) {
    return _chrpc_inner_value_to_stream(os, ct, iv, spaced, 0);
}

stream_state_t chrpc_value_to_stream(out_stream_t *os, const chrpc_value_t *v, bool spaced) {
    TRY_STREAM_CALL(chrpc_type_to_stream(os, v->type, spaced));    
    
    if (spaced) {
        OS_PUTC(os, '\n');
    }

    OS_PUTC(os, '$');

    if (spaced) {
        OS_PUTC(os, '\n');
    }

    TRY_STREAM_CALL(chrpc_inner_value_to_stream(os, v->type, v->value, spaced));

    return STREAM_SUCCESS;
}

// Hacky Pseudo C Lamda here...

#define CHRPC_TRY_PRINT(func, ...) \
    do {\
        string_t *__s = new_string();\
        out_stream_t *__os = new_out_stream_to_string(__s);\
        stream_state_t __ss = func(__os, __VA_ARGS__);\
        if (__ss == STREAM_SUCCESS) {\
            printf("%s\n", s_get_cstr(__s));\
        }\
        delete_out_stream(__os);\
        delete_string(__s);\
    } while (0)

void chrpc_type_print(const chrpc_type_t *ct, bool spaced) {
    CHRPC_TRY_PRINT(chrpc_type_to_stream, ct, spaced);
}
void chrpc_inner_value_print(const chrpc_type_t *ct, const chrpc_inner_value_t *iv, bool spaced) {
    CHRPC_TRY_PRINT(chrpc_inner_value_to_stream, ct, iv, spaced);
}
void chrpc_value_print(const chrpc_value_t *v, bool spaced) {
    CHRPC_TRY_PRINT(chrpc_value_to_stream, v, spaced);
}
