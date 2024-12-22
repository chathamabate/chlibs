
#include "chrpc/serial_value.h"

#include "chrpc/serial_type.h"
#include "chsys/mem.h"
#include "chutil/list.h"
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

// NOTE: Block values only exists for the following types.
// numerics, numeric arrays, and strings.
// Arrays of strings, Arrays of composites, and Structs CANNOT be converted to a block value.
typedef struct _chrpc_block_value_t {
    void *block;
    size_t cell_size;
    uint32_t num_cells;
} chrpc_block_value_t;

static bool chrpc_type_is_blockable(const chrpc_type_t *ct) {
    return chrpc_type_id_is_primitive(ct->type_id) || 
        (ct->type_id == CHRPC_ARRAY_TID && 
         chrpc_type_id_is_primitive(ct->array_cell_type->type_id) && ct->array_cell_type->type_id != CHRPC_STRING_TID);
}

static chrpc_block_value_t chrpc_inner_array_value_to_block_value(const chrpc_type_t *ct, chrpc_inner_value_t *iv) {
    switch (ct->type_id) {
    case CHRPC_BYTE_TID:
        return (chrpc_block_value_t){.block = (iv->b8_arr), .cell_size = sizeof(uint8_t), .num_cells = iv->array_len};

    case CHRPC_INT16_TID:
        return (chrpc_block_value_t){.block = (iv->i16_arr), .cell_size = sizeof(uint16_t), .num_cells = iv->array_len};
    case CHRPC_INT32_TID:
        return (chrpc_block_value_t){.block = (iv->i32_arr), .cell_size = sizeof(uint32_t), .num_cells = iv->array_len};
    case CHRPC_INT64_TID:
        return (chrpc_block_value_t){.block = (iv->i64_arr), .cell_size = sizeof(uint64_t), .num_cells = iv->array_len};

    case CHRPC_UINT16_TID:
        return (chrpc_block_value_t){.block = (iv->u16_arr), .cell_size = sizeof(uint16_t), .num_cells = iv->array_len};
    case CHRPC_UINT32_TID:
        return (chrpc_block_value_t){.block = (iv->u32_arr), .cell_size = sizeof(uint32_t), .num_cells = iv->array_len};
    case CHRPC_UINT64_TID:
        return (chrpc_block_value_t){.block = (iv->u64_arr), .cell_size = sizeof(uint64_t), .num_cells = iv->array_len};

    // Arrays of these types cannot be interpreted as blocks.
    case CHRPC_STRING_TID:
    case CHRPC_STRUCT_TID:
    case CHRPC_ARRAY_TID:
        return (chrpc_block_value_t){.block = NULL};

    // Shouldn't make it here.
    default:
        return (chrpc_block_value_t){.block = NULL};
    }

}

static chrpc_block_value_t chrpc_inner_value_to_block_value(const chrpc_type_t *ct, chrpc_inner_value_t *iv) {
    switch (ct->type_id) {
    case CHRPC_BYTE_TID:
        return (chrpc_block_value_t){.block = &(iv->b8), .cell_size = sizeof(uint8_t), .num_cells = 1};

    case CHRPC_INT16_TID:
        return (chrpc_block_value_t){.block = &(iv->i16), .cell_size = sizeof(uint16_t), .num_cells = 1};
    case CHRPC_INT32_TID:
        return (chrpc_block_value_t){.block = &(iv->i32), .cell_size = sizeof(uint32_t), .num_cells = 1};
    case CHRPC_INT64_TID:
        return (chrpc_block_value_t){.block = &(iv->i64), .cell_size = sizeof(uint64_t), .num_cells = 1};

    case CHRPC_UINT16_TID:
        return (chrpc_block_value_t){.block = &(iv->u16), .cell_size = sizeof(uint16_t), .num_cells = 1};
    case CHRPC_UINT32_TID:
        return (chrpc_block_value_t){.block = &(iv->u32), .cell_size = sizeof(uint32_t), .num_cells = 1};
    case CHRPC_UINT64_TID:
        return (chrpc_block_value_t){.block = &(iv->u64), .cell_size = sizeof(uint64_t), .num_cells = 1};

    case CHRPC_STRING_TID:
        return (chrpc_block_value_t){.block = iv->str, .cell_size = sizeof(char), .num_cells = strlen(iv->str)};

    case CHRPC_STRUCT_TID:
        return (chrpc_block_value_t){.block = NULL};

    case CHRPC_ARRAY_TID:
        return chrpc_inner_array_value_to_block_value(ct->array_cell_type, iv);

    // Shouldn't make it here.
    default:
        return (chrpc_block_value_t){.block = NULL};
    }
}

void delete_chrpc_inner_value(const chrpc_type_t *t, chrpc_inner_value_t *iv) {
    if (t->type_id == CHRPC_STRING_TID) {
        safe_free(iv->str);
        safe_free(iv);

        return;
    }

    if (chrpc_type_is_primitive(t)) {
        // Non string primitive. (Do nothing)
        safe_free(iv);

        return;
    }

    if (t->type_id == CHRPC_STRUCT_TID) {
        chrpc_struct_fields_types_t *struct_fields = t->struct_fields_types;

        for (uint8_t i = 0; i < struct_fields->num_fields; i++) {
            chrpc_type_t *field_type = struct_fields->field_types[i]; 
            delete_chrpc_inner_value(field_type, iv->struct_entries[i]);
        }

        // Remember, we can't have an empty struct!
        safe_free(iv->struct_entries);
        safe_free(iv);

        return;
    }

    // If we make it here, we must be working with an array type!
    
    const chrpc_type_t *cell_type = t->array_cell_type;

    // Remember, string array is kinda unique.
    if (cell_type->type_id == CHRPC_STRING_TID) {
        for (uint32_t i = 0; i < iv->array_len; i++) {
            safe_free(iv->str_arr[i]);
        }
        if (iv->str_arr) {
            safe_free(iv->str_arr);
        }
        safe_free(iv);

        return;
    }
    
    // Array of non-string primitives can be turned into a block which
    // is gauranteed to point to dynamic memory.
    if (chrpc_type_is_primitive(cell_type)) {
        chrpc_block_value_t block = chrpc_inner_value_to_block_value(t, iv);
        if (block.block) {
            safe_free(block.block);
        }
        safe_free(iv);

        return;
    }

    // Finally, Array of composites!
    for (uint32_t i = 0; i < iv->array_len; i++) {
        delete_chrpc_inner_value(cell_type, iv->array_entries[i]);
    }

    if (iv->array_entries) {
        safe_free(iv->array_entries);
    }

    safe_free(iv);
}

static chrpc_value_t *new_chrpc_value_from_pair(chrpc_type_t *ct, chrpc_inner_value_t *iv) {
    chrpc_value_t *cv = (chrpc_value_t *)safe_malloc(sizeof(chrpc_value_t));

    cv->type = ct;
    cv->value = iv;

    return cv;
}

void delete_chrpc_value(chrpc_value_t *v) {
    delete_chrpc_inner_value(v->type, v->value);
    delete_chrpc_type(v->type);
    safe_free(v);
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

        l_push(l, &dyna_buf);
    }

    va_end(args);

    uint32_t len = (uint32_t)l_len(l);
    char **arr = (char **)delete_and_move_list(l);
    
    return new_chrpc_str_array_value(arr, len);
}

chrpc_value_t *new_chrpc_composite_empty_array_value(chrpc_type_t *t) {
    // This call only accepts composite types!
    if (t->type_id != CHRPC_ARRAY_TID && t->type_id != CHRPC_STRUCT_TID) {
        return NULL;
    }

    chrpc_inner_value_t *iv = (chrpc_inner_value_t *)safe_malloc(sizeof(chrpc_inner_value_t));

    iv->array_entries = NULL;
    iv->array_len = 0;

    return new_chrpc_value_from_pair(new_chrpc_array_type(t), iv);
}

chrpc_value_t *new_chrpc_composite_nempty_array_value(chrpc_value_t **entries, uint32_t num_entries) {
    if (num_entries == 0) {
        return NULL;
    }

    // Let's first get our array cell type. (Must be composite)
    // We'll save this type for later.
    chrpc_type_t *cell_type = entries[0]->type;
    if (cell_type->type_id != CHRPC_ARRAY_TID && cell_type->type_id != CHRPC_STRUCT_TID) {
        return NULL;
    }

    // All given values must have same type!
    for (uint32_t i = 1; i < num_entries; i++) {
        if (!chrpc_type_equals(cell_type, entries[i]->type)) {
            return NULL;
        }
    }

    // Delete all extra types.
    for (uint32_t i = 1; i < num_entries; i++) {
        delete_chrpc_type(entries[i]->type);
    }

    chrpc_inner_value_t **iv_arr = (chrpc_inner_value_t **)safe_malloc(sizeof(chrpc_inner_value_t *) * num_entries);
    for (uint32_t i = 0; i < num_entries; i++) {
        iv_arr[i] = entries[i]->value;
    }

    // Delete remaining value pair structs.
    for (uint32_t i = 0; i < num_entries; i++) {
        safe_free(entries[i]);
    }
    safe_free(entries);

    // Finally, put it all together!
    
    chrpc_inner_value_t *iv = (chrpc_inner_value_t *)safe_malloc(sizeof(chrpc_inner_value_t));
    iv->array_entries = iv_arr;
    iv->array_len = num_entries;

    return new_chrpc_value_from_pair(new_chrpc_array_type(cell_type), iv);
}

chrpc_value_t *_new_chrpc_composite_nempty_array_value_va(int dummy,...) {
    list_t *buffer = new_list(ARRAY_LIST_IMPL, sizeof(chrpc_value_t *));

    va_list args;
    va_start(args, dummy);
    chrpc_value_t *iter;
    while ((iter = va_arg(args, chrpc_value_t *))) {
        l_push(buffer, &iter);
    }
    va_end(args);

    uint32_t len = (uint32_t)l_len(buffer);

    if (len == 0) {
        delete_list(buffer);
        return NULL;
    }

    chrpc_value_t **arr = delete_and_move_list(buffer);
    chrpc_value_t *ret_val = new_chrpc_composite_nempty_array_value(arr, len);

    // On error case, cleanup these values which would otherwise become unreachable.
    if (!ret_val) {
        for (size_t i = 0; i < len; i++) {
            delete_chrpc_value(arr[i]);
        }
        safe_free(arr);
    }

    return ret_val;
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
        l_push(buffer, &iter);
    }
    va_end(args);

    size_t len = l_len(buffer);
    chrpc_value_t **arr = (chrpc_value_t **)delete_and_move_list(buffer);

    // Doing this check here too because length will be cast down to a single
    // byte when calling the above function.
    if (len < 1 || CHRPC_MAX_STRUCT_FIELDS < len) {
        for (size_t i = 0; i < len; i++) {
            delete_chrpc_value(arr[i]);
        }
        safe_free(arr);
        return NULL;
    }

    return new_chrpc_struct_value(arr, len);
}


// This function assumes iv0 and iv1 have the same type ct, and that neither inner value is NULL.
// (That is they belong to a valid chrpc_value_t object)
static bool chrpc_inner_value_equals(const chrpc_type_t *ct, const chrpc_inner_value_t *iv0, const chrpc_inner_value_t *iv1) {
    if (chrpc_type_is_blockable(ct)) {
        chrpc_block_value_t block0 = chrpc_inner_value_to_block_value(ct, (chrpc_inner_value_t *)iv0);
        chrpc_block_value_t block1 = chrpc_inner_value_to_block_value(ct, (chrpc_inner_value_t *)iv1);
        
        if (block0.num_cells != block1.num_cells) {
            return false;
        }

        // Same types mean same cell sizes!
        return memcmp(block0.block, block1.block, block0.cell_size * block0.num_cells) == 0;
    }

    // If not blockable we must have a Struct, an Array of Composites, or an Array of Strings.

    if (ct->type_id == CHRPC_STRUCT_TID) {
        for (uint8_t i = 0; i < ct->struct_fields_types->num_fields; i++) {
            const chrpc_type_t *ft = ct->struct_fields_types->field_types[i];
            const chrpc_inner_value_t *f0 = iv0->struct_entries[i];
            const chrpc_inner_value_t *f1 = iv1->struct_entries[i];

            if (!chrpc_inner_value_equals(ft, f0, f1)) {
                return false;
            }
        }

        return true;
    }

    // If we make it here, we must be working with an array of composites or strings.

    const chrpc_type_t *cell_type = ct->array_cell_type;

    if (iv0->array_len != iv1->array_len) {
        return false;
    }

    const uint32_t arr_len = iv0->array_len;

    if (cell_type->type_id == CHRPC_STRING_TID) {
        for (uint32_t i = 0; i < arr_len; i++) {
            if (strcmp(iv0->str_arr[i], iv1->str_arr[i])) {
                return false;
            }
        }

        return true;
    }

    // Finally composites.

    for (uint32_t i = 0; i < arr_len; i++) {
        if (!chrpc_inner_value_equals(cell_type, iv0->array_entries[i], iv1->array_entries[i])) {
            return false;
        }
    }

    return true;
}


bool chrpc_value_equals(const chrpc_value_t *cv0, const chrpc_value_t *cv1) {
    if (cv0 == cv1) {
        return true;
    }

    if (!cv0 || !cv1) {
        return false;
    }

    if (!chrpc_type_equals(cv0->type, cv1->type)) {
        return false;
    }

    return chrpc_inner_value_equals(cv0->type, cv0->value, cv1->value);
}

static chrpc_status_t chrpc_block_to_buffer(const void *arr, uint32_t arr_len, size_t cell_size, uint8_t *buf, size_t buf_len, size_t *written) {
    size_t array_size = arr_len * cell_size;
    size_t total_written = sizeof(uint32_t) + array_size;

    if (buf_len < total_written) {
        return CHRPC_BUFFER_TOO_SMALL;
    }

    *(uint32_t *)buf = arr_len;
    void *array_section = (void *)(((uint32_t *)buf) + 1);
    memcpy(array_section, arr, array_size);
    *written = total_written;

    return CHRPC_SUCCESS;
}

// Just like with equals, this is the case where iv has type ARRAY(ct)
static chrpc_status_t chrpc_inner_array_value_to_buffer(const chrpc_type_t *ct, const chrpc_inner_value_t *iv, uint8_t *buf, size_t buf_len, size_t *written) {
    size_t total_written;

    switch (ct->type_id) {
        case CHRPC_BYTE_TID:
            return chrpc_block_to_buffer(iv->b8_arr, iv->array_len, sizeof(uint8_t), buf, buf_len, written);
        case CHRPC_UINT16_TID:
            return chrpc_block_to_buffer(iv->u16_arr, iv->array_len, sizeof(uint16_t), buf, buf_len, written);
        case CHRPC_UINT32_TID:
            return chrpc_block_to_buffer(iv->u32_arr, iv->array_len, sizeof(uint32_t), buf, buf_len, written);
        case CHRPC_UINT64_TID:
            return chrpc_block_to_buffer(iv->u64_arr, iv->array_len, sizeof(uint64_t), buf, buf_len, written);
        case CHRPC_INT16_TID:
            return chrpc_block_to_buffer(iv->i16_arr, iv->array_len, sizeof(int16_t), buf, buf_len, written);
        case CHRPC_INT32_TID:
            return chrpc_block_to_buffer(iv->i32_arr, iv->array_len, sizeof(int32_t), buf, buf_len, written);
        case CHRPC_INT64_TID:
            return chrpc_block_to_buffer(iv->i64_arr, iv->array_len, sizeof(int64_t), buf, buf_len, written);

        // Array of strings is kinda unique because strings are seen as primitive yet don't have a fixed size.
        case CHRPC_STRING_TID:
            if (buf_len < sizeof(uint32_t)) {
                return CHRPC_BUFFER_TOO_SMALL;
            }

            *(uint32_t *)buf = iv->array_len;
            total_written = sizeof(uint32_t);

            for (size_t i = 0; i < iv->array_len; i++) {
                const char *cell_str = iv->str_arr[i];
                size_t cell_str_len = strlen(cell_str);

                size_t w;
                chrpc_status_t cs = chrpc_block_to_buffer(cell_str, cell_str_len, sizeof(char), buf + total_written, buf_len - total_written, &w);
                if (cs != CHRPC_SUCCESS) {
                    return cs;
                }

                total_written += w;
            }

            *written = total_written;
            return CHRPC_SUCCESS;
            

        // Composite types are stored in array_entries.
        case CHRPC_ARRAY_TID:
        case CHRPC_STRUCT_TID:
            if (buf_len < sizeof(uint32_t)) {
                return CHRPC_BUFFER_TOO_SMALL;
            }

            *(uint32_t *)buf = iv->array_len;
            total_written = sizeof(uint32_t);

            for (size_t i = 0; i < iv->array_len; i++) {
                size_t w;
                chrpc_status_t cs = chrpc_inner_value_to_buffer(ct, iv->array_entries[i], buf + total_written, buf_len - total_written, &w);
                if (cs != CHRPC_SUCCESS) {
                    return cs;
                }

                total_written += w;
            }

            *written = total_written;
            return CHRPC_SUCCESS;

        default:
            return CHRPC_MALFORMED_TYPE;
    }
}

#define IV_NUMERIC_TO_BUFFER(type, val, b, bl, w) \
    do { \
        size_t __s = sizeof(type); \
        if (bl < __s) { \
            return CHRPC_BUFFER_TOO_SMALL; \
        } \
        *(type *)(b) = val; \
        *(w) = __s; \
        return CHRPC_SUCCESS; \
    } while (0)

chrpc_status_t chrpc_inner_value_to_buffer(const chrpc_type_t *ct, const chrpc_inner_value_t *iv, uint8_t *buf, size_t buf_len, size_t *written) {
    size_t s_len;
    size_t total_written;

    switch (ct->type_id) {
        case CHRPC_BYTE_TID:
            IV_NUMERIC_TO_BUFFER(uint8_t, iv->b8, buf, buf_len, written);

        case CHRPC_UINT16_TID:
            IV_NUMERIC_TO_BUFFER(uint16_t, iv->u16, buf, buf_len, written);

        case CHRPC_UINT32_TID:
            IV_NUMERIC_TO_BUFFER(uint32_t, iv->u32, buf, buf_len, written);

        case CHRPC_UINT64_TID:
            IV_NUMERIC_TO_BUFFER(uint64_t, iv->u64, buf, buf_len, written);

        case CHRPC_INT16_TID:
            IV_NUMERIC_TO_BUFFER(int16_t, iv->i16, buf, buf_len, written);

        case CHRPC_INT32_TID:
            IV_NUMERIC_TO_BUFFER(int32_t, iv->i32, buf, buf_len, written);

        case CHRPC_INT64_TID:
            IV_NUMERIC_TO_BUFFER(int64_t, iv->i64, buf, buf_len, written);

        case CHRPC_STRING_TID:
            return chrpc_block_to_buffer(iv->str, strlen(iv->str), sizeof(char), buf, buf_len, written);

        case CHRPC_ARRAY_TID:
            return chrpc_inner_array_value_to_buffer(ct->array_cell_type, iv, buf, buf_len, written);

        case CHRPC_STRUCT_TID:
            total_written = 0;
            for (size_t i = 0; i < ct->struct_fields_types->num_fields; i++) {
                const chrpc_type_t *field_type = ct->struct_fields_types->field_types[i];
                const chrpc_inner_value_t *field_iv = iv->struct_entries[i];

                size_t w;
                chrpc_status_t field_status = chrpc_inner_value_to_buffer(field_type, field_iv, 
                        buf + total_written, buf_len - total_written, &w);

                if (field_status != CHRPC_SUCCESS) {
                    return field_status;
                }

                total_written += w;
            }

            *written = total_written;
            return CHRPC_SUCCESS;

        default:
            return CHRPC_MALFORMED_TYPE;
    }
}
