
#include "chrpc/serial_value.h"

#include "chrpc/serial_type.h"
#include "chsys/mem.h"
#include "chutil/list.h"
#include <stdarg.h>
#include <stdint.h>
#include <string.h>

void delete_chrpc_inner_value(const chrpc_type_t *t, chrpc_inner_value_t *iv) {
    if (t->type_id == CHRPC_STRUCT_TID) {
        chrpc_struct_fields_types_t *struct_fields = t->struct_fields_types;

        for (uint8_t i = 0; i < struct_fields->num_fields; i++) {
            chrpc_type_t *field_type = struct_fields->field_types[i]; 
            delete_chrpc_inner_value(field_type, iv->struct_entries[i]);
        }
    } else if (t->type_id == CHRPC_ARRAY_TID) {
        chrpc_type_t *cell_type = t->array_cell_type;

        switch (cell_type->type_id) {
            case CHRPC_BYTE_TID:
                if (iv->b8_arr) {
                    safe_free(iv->b8_arr);
                }
                break;

            case CHRPC_UINT16_TID:
                if (iv->u16_arr) {
                    safe_free(iv->u16_arr);
                }
                break;

            case CHRPC_UINT32_TID:
                if (iv->u32_arr) {
                    safe_free(iv->u32_arr);
                }
                break;

            case CHRPC_UINT64_TID:
                if (iv->u64_arr) {
                    safe_free(iv->u64_arr);
                }
                break;

            case CHRPC_INT16_TID:
                if (iv->i16_arr) {
                    safe_free(iv->i16_arr);
                }
                break;

            case CHRPC_INT32_TID:
                if (iv->i32_arr) {
                    safe_free(iv->i32_arr);
                }
                break;

            case CHRPC_INT64_TID:
                if (iv->i64_arr) {
                    safe_free(iv->i64_arr);
                }
                break;

            case CHRPC_STRING_TID:
                for (uint32_t i = 0; i < iv->array_len; i++) {
                    safe_free(iv->str_arr[i]);
                }

                if (iv->str_arr) {
                    safe_free(iv->str_arr);
                }
                break;

            // Composite Types! (Array Recursive Case)
            case CHRPC_ARRAY_TID:
            case CHRPC_STRUCT_TID:
                for (uint32_t i = 0; i < iv->array_len; i++) {
                    delete_chrpc_inner_value(cell_type, iv->array_entries[i]);
                }
                break;
        }
    } else if (t->type_id == CHRPC_STRING_TID) {
        safe_free(iv->str);
    }

    safe_free(iv);
}

static chrpc_value_t *new_chrpc_value_from_pair(chrpc_type_t *ct, chrpc_inner_value_t *iv) {
    chrpc_value_t *cv = (chrpc_value_t *)safe_malloc(sizeof(chrpc_value_t));

    cv->type = ct;
    cv->value = iv;

    return cv;
}

#define VA_POPULATE(num_eles, arr, t) \
    do { \
        va_list __args; \
        va_start(__args, num_eles); \
        for (uint32_t i = 0; i < num_eles; i++) { \
            (arr)[i] = (t)va_arg(__args, int64_t); \
        } \
        va_end(__args); \
    } while (0)

// BYTE8

chrpc_value_t *new_chrpc_b8_value(uint8_t b8_val) {
    chrpc_inner_value_t *iv = (chrpc_inner_value_t *)safe_malloc(sizeof(chrpc_inner_value_t));
    iv->b8 = b8_val;

    return new_chrpc_value_from_pair(CHRPC_BYTE_T, iv);
}

chrpc_value_t *new_chrpc_b8_array_value(uint8_t *b8_array_val, uint32_t array_len) {
    chrpc_inner_value_t *iv = (chrpc_inner_value_t *)safe_malloc(sizeof(chrpc_inner_value_t));
    iv->b8_arr = b8_array_val;
    iv->array_len = array_len;

    return new_chrpc_value_from_pair(new_chrpc_array_type(CHRPC_BYTE_T), iv);
}

chrpc_value_t *_new_chrpc_b8_array_value_va(uint32_t num_eles,...) {
    uint8_t *b8_array_val = (uint8_t *)safe_malloc(sizeof(uint8_t) * num_eles);

    VA_POPULATE(num_eles, b8_array_val, uint8_t);

    return new_chrpc_b8_array_value(b8_array_val, num_eles);
}

// INT16

chrpc_value_t *new_chrpc_i16_value(int16_t i16_val) {
    chrpc_inner_value_t *iv = (chrpc_inner_value_t *)safe_malloc(sizeof(chrpc_inner_value_t));
    iv->i16 = i16_val;

    return new_chrpc_value_from_pair(CHRPC_INT16_T, iv);
}

chrpc_value_t *new_chrpc_i16_array_value(int16_t *i16_array_val, uint32_t array_len) {
    chrpc_inner_value_t *iv = (chrpc_inner_value_t *)safe_malloc(sizeof(chrpc_inner_value_t));
    iv->i16_arr = i16_array_val;
    iv->array_len = array_len;

    return new_chrpc_value_from_pair(new_chrpc_array_type(CHRPC_INT16_T), iv);
}

chrpc_value_t *_new_chrpc_i16_array_value_va(uint32_t num_eles,...) {
    int16_t *i16_array_val = (int16_t *)safe_malloc(sizeof(int16_t) * num_eles);

    VA_POPULATE(num_eles, i16_array_val, int16_t);

    return new_chrpc_i16_array_value(i16_array_val, num_eles);
}

// INT32

chrpc_value_t *new_chrpc_i32_value(int32_t i32_val) {
    chrpc_inner_value_t *iv = (chrpc_inner_value_t *)safe_malloc(sizeof(chrpc_inner_value_t));
    iv->i32 = i32_val;

    return new_chrpc_value_from_pair(CHRPC_INT32_T, iv);
}

chrpc_value_t *new_chrpc_i32_array_value(int32_t *i32_array_val, uint32_t array_len) {
    chrpc_inner_value_t *iv = (chrpc_inner_value_t *)safe_malloc(sizeof(chrpc_inner_value_t));
    iv->i32_arr = i32_array_val;
    iv->array_len = array_len;

    return new_chrpc_value_from_pair(new_chrpc_array_type(CHRPC_INT32_T), iv);
}

chrpc_value_t *_new_chrpc_i32_array_value_va(uint32_t num_eles,...) {
    int32_t *i32_array_val = (int32_t *)safe_malloc(sizeof(int32_t) * num_eles);

    VA_POPULATE(num_eles, i32_array_val, int32_t);

    return new_chrpc_i32_array_value(i32_array_val, num_eles);
}

// INT64

chrpc_value_t *new_chrpc_i64_value(int64_t i64_val) {
    chrpc_inner_value_t *iv = (chrpc_inner_value_t *)safe_malloc(sizeof(chrpc_inner_value_t));
    iv->i64 = i64_val;

    return new_chrpc_value_from_pair(CHRPC_INT64_T, iv);
}

chrpc_value_t *new_chrpc_i64_array_value(int64_t *i64_array_val, uint32_t array_len) {
    chrpc_inner_value_t *iv = (chrpc_inner_value_t *)safe_malloc(sizeof(chrpc_inner_value_t));
    iv->i64_arr = i64_array_val;
    iv->array_len = array_len;

    return new_chrpc_value_from_pair(new_chrpc_array_type(CHRPC_INT64_T), iv);
}

chrpc_value_t *_new_chrpc_i64_array_value_va(uint32_t num_eles,...) {
    int64_t *i64_array_val = (int64_t *)safe_malloc(sizeof(int64_t) * num_eles);

    VA_POPULATE(num_eles, i64_array_val, int64_t);

    return new_chrpc_i64_array_value(i64_array_val, num_eles);
}

// UINT16

chrpc_value_t *new_chrpc_u16_value(uint16_t u16_val) {
    chrpc_inner_value_t *iv = (chrpc_inner_value_t *)safe_malloc(sizeof(chrpc_inner_value_t));
    iv->u16 = u16_val;

    return new_chrpc_value_from_pair(CHRPC_UINT16_T, iv);
}

chrpc_value_t *new_chrpc_u16_array_value(uint16_t *u16_array_val, uint32_t array_len) {
    chrpc_inner_value_t *iv = (chrpc_inner_value_t *)safe_malloc(sizeof(chrpc_inner_value_t));
    iv->u16_arr = u16_array_val;
    iv->array_len = array_len;

    return new_chrpc_value_from_pair(new_chrpc_array_type(CHRPC_UINT16_T), iv);
}

chrpc_value_t *_new_chrpc_u16_array_value_va(uint32_t num_eles,...) {
    uint16_t *u16_array_val = (uint16_t *)safe_malloc(sizeof(uint16_t) * num_eles);

    VA_POPULATE(num_eles, u16_array_val, uint16_t);

    return new_chrpc_u16_array_value(u16_array_val, num_eles);
}

// UINT32

chrpc_value_t *new_chrpc_u32_value(uint32_t u32_val) {
    chrpc_inner_value_t *iv = (chrpc_inner_value_t *)safe_malloc(sizeof(chrpc_inner_value_t));
    iv->u32 = u32_val;

    return new_chrpc_value_from_pair(CHRPC_UINT32_T, iv);
}

chrpc_value_t *new_chrpc_u32_array_value(uint32_t *u32_array_val, uint32_t array_len) {
    chrpc_inner_value_t *iv = (chrpc_inner_value_t *)safe_malloc(sizeof(chrpc_inner_value_t));
    iv->u32_arr = u32_array_val;
    iv->array_len = array_len;

    return new_chrpc_value_from_pair(new_chrpc_array_type(CHRPC_UINT32_T), iv);
}

chrpc_value_t *_new_chrpc_u32_array_value_va(uint32_t num_eles,...) {
    uint32_t *u32_array_val = (uint32_t *)safe_malloc(sizeof(uint32_t) * num_eles);

    VA_POPULATE(num_eles, u32_array_val, uint32_t);

    return new_chrpc_u32_array_value(u32_array_val, num_eles);
}

// UINT64

chrpc_value_t *new_chrpc_u64_value(uint64_t u64_val) {
    chrpc_inner_value_t *iv = (chrpc_inner_value_t *)safe_malloc(sizeof(chrpc_inner_value_t));
    iv->u64 = u64_val;

    return new_chrpc_value_from_pair(CHRPC_UINT64_T, iv);
}

chrpc_value_t *new_chrpc_u64_array_value(uint64_t *u64_array_val, uint32_t array_len) {
    chrpc_inner_value_t *iv = (chrpc_inner_value_t *)safe_malloc(sizeof(chrpc_inner_value_t));
    iv->u64_arr = u64_array_val;
    iv->array_len = array_len;

    return new_chrpc_value_from_pair(new_chrpc_array_type(CHRPC_UINT64_T), iv);
}

chrpc_value_t *_new_chrpc_u64_array_value_va(uint32_t num_eles,...) {
    uint64_t *u64_array_val = (uint64_t *)safe_malloc(sizeof(uint64_t) * num_eles);

    VA_POPULATE(num_eles, u64_array_val, uint64_t);

    return new_chrpc_u64_array_value(u64_array_val, num_eles);
}

// STRING 

chrpc_value_t *new_chrpc_str_value(const char *str_val) {
    char *new_str = (char *)safe_malloc((strlen(str_val) + 1) * sizeof(char));
    strcpy(new_str, str_val);

    chrpc_inner_value_t *iv = (chrpc_inner_value_t *)safe_malloc(sizeof(chrpc_inner_value_t));
    iv->str = new_str;

    return new_chrpc_value_from_pair(CHRPC_STRING_T, iv);
}

chrpc_value_t *new_chrpc_str_array_value(char **str_array_val, uint32_t array_len) {
    chrpc_inner_value_t *iv = (chrpc_inner_value_t *)safe_malloc(sizeof(chrpc_inner_value_t));
    iv->str_arr = str_array_val;
    iv->array_len = array_len;

    return new_chrpc_value_from_pair(new_chrpc_array_type(CHRPC_STRING_T), iv);
}

chrpc_value_t *_new_chrpc_str_array_value_va(int dummy,...) {
    list_t *l = new_list(ARRAY_LIST_IMPL, sizeof(char *));

    va_list args;
    va_start(args, dummy);

    const char *iter;
    while ((iter = va_arg(args, const char *))) {
        size_t s_len = strlen(iter);
        char *dyna_buf = (char *)safe_malloc((s_len + 1) * sizeof(char));
        strcpy(dyna_buf, iter);

        l_push(l, dyna_buf);
    }

    va_end(args);

    uint32_t len = (uint32_t)l_len(l);
    char **arr = (char **)delete_and_move_list(l);
    
    return new_chrpc_str_array_value(arr, len);
}

chrpc_value_t *new_chrpc_struct_value(chrpc_value_t **struct_fields, uint8_t num_fields) {
    if (num_fields < 1 || CHRPC_MAX_STRUCT_FIELDS < num_fields) {
        return NULL;
    }

    chrpc_type_t **field_types = 
        (chrpc_type_t **)safe_malloc(sizeof(chrpc_type_t *) * num_fields);
    chrpc_inner_value_t **ivs = 
        (chrpc_inner_value_t **)safe_malloc(sizeof(chrpc_inner_value_t *) * num_fields);

    for (uint8_t i = 0; i < num_fields; i++) {
        field_types[i] = struct_fields[i]->type;
        ivs[i] = struct_fields[i]->value;
    }

    chrpc_type_t *struct_type = new_chrpc_struct_type_from_internals(field_types, num_fields);
    chrpc_inner_value_t *struct_iv = (chrpc_inner_value_t *)safe_malloc(sizeof(chrpc_inner_value_t));
    struct_iv->struct_entries = ivs;

    chrpc_value_t *struct_value = new_chrpc_value_from_pair(struct_type, struct_iv);

    // Finally, delete left over structs and array.
    for (uint8_t i = 0; i < num_fields; i++) {
        safe_free(struct_fields[i]);
    }
    safe_free(struct_fields);

    return struct_value;
}

chrpc_value_t *_new_chrpc_struct_value_va(int dummy,...) {
    list_t *buffer = new_list(ARRAY_LIST_IMPL, sizeof(chrpc_value_t *));

    va_list args;
    va_start(args, dummy);
    chrpc_value_t *iter;
    while ((iter = va_arg(args, chrpc_value_t *))) {
        l_push(buffer, iter);
    }
    va_end(args);

    size_t len = l_len(buffer);
    if (len > CHRPC_MAX_STRUCT_FIELDS) {
        // Doing this check here too because length will be cast down to a single
        // byte when calling the above function.
        delete_list(buffer);
        return NULL;
    }

    chrpc_value_t **arr = (chrpc_value_t **)delete_and_move_list(buffer);
    return new_chrpc_struct_value(arr, len);
}


