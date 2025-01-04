
#include "chrpc/serial_value.h"

#include "chrpc/serial_type.h"
#include "chsys/mem.h"
#include "chutil/list.h"
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

// A "Blockable" Type is a type who's value will always exist in its serialized form.
// For example an INT16 is blockable since to serial an integer, its value is just copied directly to the buffer.
// Since Numeric arrays and strings are also stored densely, these are also blockable.
//
// NOTE: The point of blockable types is to make serilization code less repetitive. All blockable values require
// just a single memcpy to serialize/deserialize.
static bool chrpc_type_is_blockable(const chrpc_type_t *ct) {
    return chrpc_type_id_is_primitive(ct->type_id) || 
        (ct->type_id == CHRPC_ARRAY_TID && 
         chrpc_type_id_is_primitive(ct->array_cell_type->type_id) && ct->array_cell_type->type_id != CHRPC_STRING_TID);
}

// This will return 0 if the give type is not blockable.
static size_t chrpc_blockable_type_cell_size(const chrpc_type_t *ct) {
    const chrpc_type_t *arr_cell_type;

    switch (ct->type_id) {

    // Primitive types.
    case CHRPC_BYTE_TID:
        return sizeof(uint8_t);
    case CHRPC_INT16_TID:
        return sizeof(int16_t);
    case CHRPC_INT32_TID:
        return sizeof(int32_t);
    case CHRPC_INT64_TID:
        return sizeof(int64_t);
    case CHRPC_UINT16_TID:
        return sizeof(uint16_t);
    case CHRPC_UINT32_TID:
        return sizeof(uint32_t);
    case CHRPC_UINT64_TID: 
        return sizeof(uint64_t);

    case CHRPC_STRING_TID:
        return sizeof(char);

    case CHRPC_ARRAY_TID:
        arr_cell_type = ct->array_cell_type;
        if (chrpc_type_is_primitive(arr_cell_type) && arr_cell_type->type_id != CHRPC_STRING_TID) {
            return chrpc_blockable_type_cell_size(arr_cell_type);
        }
        // Arrays of strings and composites are not blockable!
        return 0;
        
    case CHRPC_STRUCT_TID:
        return 0;

    default:
        return 0;
    }
}

// The below two functions are for getting pointers to the actual blocks themselves.
// This is extremely helpful for serialization and somewhat helpful for deserialization.

typedef struct _chrpc_block_value_t {
    void *block;
    size_t cell_size;
    uint32_t num_cells;
} chrpc_block_value_t;

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
        return (chrpc_block_value_t){.block = iv->str, .cell_size = sizeof(char), .num_cells = strlen(iv->str) + 1};

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
    if (chrpc_type_is_blockable(t)) {
        // Remember, numeric arrays and strings both live in dynamic memory.
        // We are responsible for freeing these blocks.
        bool dynamic = t->type_id == CHRPC_ARRAY_TID || t->type_id == CHRPC_STRING_TID;
        if (dynamic) {
            chrpc_block_value_t block = chrpc_inner_value_to_block_value(t, iv);

            if (block.block) {
                safe_free(block.block);
            }
        }

        // Otherwise, the block just lives inside the iv itself, i.e. a numeric value.
        // There is no dynamic buffer to free in this case.

        safe_free(iv);

        return;
    }

    // We must now either be working with a struct, an array of composites, or an array of strings.

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

    // If we make it here, we must be working with an array of strings, or an array of composites.
    
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

// Serialization Functions!

chrpc_status_t chrpc_block_value_to_buffer(chrpc_block_value_t block, bool len_prefix, uint8_t *buf, size_t buf_len, size_t *written) {
    size_t total_written = 0;

    if (len_prefix) {
        if (buf_len < sizeof(uint32_t)) {
            return CHRPC_BUFFER_TOO_SMALL;
        }

        *(uint32_t *)buf = block.num_cells;

        total_written += sizeof(uint32_t);
    }

    size_t arr_size = block.num_cells * block.cell_size;
    if (buf_len < total_written + arr_size) {
        return CHRPC_BUFFER_TOO_SMALL;
    }

    memcpy(buf + total_written, block.block, arr_size);
    total_written += arr_size; 

    *written = total_written;
    return CHRPC_SUCCESS;
}

chrpc_status_t chrpc_inner_value_to_buffer(const chrpc_type_t *ct, const chrpc_inner_value_t *iv, uint8_t *buf, size_t buf_len, size_t *written) {
    size_t total_written;
    chrpc_block_value_t block;
    size_t w;

    // Blockable values already kinda exist in their serialized form.
    // (Kinda the point of blockable values)
    // All we need to know, is whether we should add the length prefix or not.

    if (chrpc_type_is_blockable(ct)) {
        block = chrpc_inner_value_to_block_value(ct, (chrpc_inner_value_t *)iv);
        bool len_prefix = ct->type_id == CHRPC_STRING_TID || ct->type_id == CHRPC_ARRAY_TID;
        return chrpc_block_value_to_buffer(block, len_prefix, buf, buf_len, written);
    }

    // Finally, we have a non-blockable type. Either an array of strings, a struct, or an array of 
    // composites.

    if (ct->type_id == CHRPC_STRUCT_TID) {
        total_written = 0;

        for (uint8_t i = 0; i < ct->struct_fields_types->num_fields; i++) {
            const chrpc_type_t *ft = ct->struct_fields_types->field_types[i];
            const chrpc_inner_value_t *fv = iv->struct_entries[i]; 

            chrpc_status_t field_status = chrpc_inner_value_to_buffer(ft, fv, buf + total_written, buf_len - total_written, &w);

            if (field_status != CHRPC_SUCCESS) {
                return field_status;
            }

            total_written += w;
        }

        *written = total_written;
        return CHRPC_SUCCESS;
    }

    // Ok, now we must be working with an array of strings or an array of composites.

    if (buf_len < sizeof(uint32_t)) {
        return CHRPC_BUFFER_TOO_SMALL;
    }

    *(uint32_t *)buf = iv->array_len;

    total_written = sizeof(uint32_t);

    chrpc_status_t cell_status;

    // Array of stirngs.
    if (ct->array_cell_type->type_id == CHRPC_STRING_TID) {
        // Ok this is a little hacky, but each individual string in this array is 
        // kinda a block. So we can populate the block value ourselves, then pass
        // it to block_to_buffer.
        for (uint32_t i = 0; i < iv->array_len; i++) {
            block.block = iv->str_arr[i];
            block.num_cells = (uint32_t)(strlen(iv->str_arr[i]) + 1);
            block.cell_size = 1;

            cell_status = chrpc_block_value_to_buffer(block, true, buf + total_written, buf_len - total_written, &w);
            if (cell_status != CHRPC_SUCCESS) {
                return cell_status;
            }
            total_written += w;
        }

        *written = total_written;
        return CHRPC_SUCCESS;
    }

    // Array of composites.

    for (uint32_t i = 0; i < iv->array_len; i++) {
        cell_status = chrpc_inner_value_to_buffer(ct->array_cell_type, iv->array_entries[i], 
                buf + total_written, buf_len - total_written, &w);
        if (cell_status != CHRPC_SUCCESS) {
            return cell_status;
        }
        total_written += w;
    }

    *written = total_written;
    return CHRPC_SUCCESS;
}

// Assumes ct is primitive, but not a string.
static chrpc_status_t chrpc_numeric_inner_value_from_buffer(const chrpc_type_t *ct, chrpc_inner_value_t **iv, const uint8_t *buf, size_t buf_len, size_t *readden) {
    chrpc_inner_value_t *ivp = (chrpc_inner_value_t *)safe_malloc(sizeof(chrpc_inner_value_t));
    chrpc_block_value_t block = chrpc_inner_value_to_block_value(ct, ivp);

    if (buf_len < block.cell_size) {
        safe_free(ivp);
        return CHRPC_UNEXPECTED_END;
    }

    memcpy(block.block, buf, block.cell_size);

    *iv = ivp;
    *readden = block.cell_size;

    return CHRPC_SUCCESS;
}

// A singular string or numeric array. (Get just the array itself)
static chrpc_status_t chrpc_block_array_from_buffer(const chrpc_type_t *ct, void **dest_arr, uint32_t *num_eles, const uint8_t *buf, size_t buf_len, size_t *readden) {
    if (buf_len < sizeof(uint32_t)) {
        return CHRPC_UNEXPECTED_END;
    }

    uint32_t num_cells = *(uint32_t *)buf;

    size_t cell_size = chrpc_blockable_type_cell_size(ct);

    size_t rem_len = buf_len - sizeof(uint32_t);
    const uint8_t *rem_buf = buf + sizeof(uint32_t);

    size_t arr_bytes = cell_size * num_cells;

    if (rem_len < arr_bytes) {
        return CHRPC_UNEXPECTED_END;
    }

    void *arr = safe_malloc(arr_bytes);
    memcpy(arr, rem_buf, arr_bytes);

    // We have successfully read from the buffer.
    // We can write readden here.
    *readden = sizeof(uint32_t) + arr_bytes;
    *dest_arr = arr;

    if (num_eles) {
        *num_eles = num_cells;
    }

    return CHRPC_SUCCESS;
}

// This function really just wraps the above function.
static chrpc_status_t chrpc_block_array_inner_value_from_buffer(const chrpc_type_t *ct, chrpc_inner_value_t **iv, const uint8_t *buf, size_t buf_len, size_t *readden) {
    void *arr;

    uint32_t num_eles;
    chrpc_status_t arr_status = chrpc_block_array_from_buffer(ct, &arr, &num_eles, buf, buf_len, readden);

    if (arr_status != CHRPC_SUCCESS) {
        return arr_status;
    }

    chrpc_inner_value_t *ivp = (chrpc_inner_value_t *)safe_malloc(sizeof(chrpc_inner_value_t));

    if (ct->type_id == CHRPC_STRING_TID) {
        ivp->str = (char *)arr;
        *iv = ivp;

        // NOTE: Length is not used for strings (Might want to change this someday)

        return CHRPC_SUCCESS;
    }

    // If we make it here, we must be working with a numeric array.

    switch (ct->array_cell_type->type_id) {
    case CHRPC_BYTE_TID:
        ivp->b8_arr = (uint8_t *)arr;
        break;
    case CHRPC_INT16_TID:
        ivp->i16_arr = (int16_t *)arr;
        break;
    case CHRPC_INT32_TID:
        ivp->i32_arr = (int32_t *)arr;
        break;
    case CHRPC_INT64_TID:
        ivp->i64_arr = (int64_t *)arr;
        break;
    case CHRPC_UINT16_TID:
        ivp->u16_arr = (uint16_t *)arr;
        break;
    case CHRPC_UINT32_TID:
        ivp->u32_arr = (uint32_t *)arr;
        break;
    case CHRPC_UINT64_TID:
        ivp->u64_arr = (uint64_t *)arr;
        break;
    default:
        break; // SHOULD NEVER MAKE IT HERE.
    }

    ivp->array_len = num_eles;
    *iv = ivp;

    return CHRPC_SUCCESS;
}

static chrpc_status_t chrpc_struct_inner_value_from_buffer(const chrpc_type_t *ct, chrpc_inner_value_t **iv, const uint8_t *buf, size_t buf_len, size_t *readden) {
    size_t bytes_read = 0;

    const uint32_t num_fields = ct->struct_fields_types->num_fields;
    chrpc_inner_value_t **fields = (chrpc_inner_value_t **)
        safe_malloc(sizeof(chrpc_inner_value_t *) * num_fields);

    uint32_t fi = 0;
    chrpc_status_t field_status;

    for (fi = 0; fi < ct->struct_fields_types->num_fields; fi++) {
        size_t field_read;
        field_status = chrpc_inner_value_from_buffer(ct->struct_fields_types->field_types[fi], &(fields[fi]), 
                buf + bytes_read, buf_len - bytes_read, &field_read);

        if (field_status != CHRPC_SUCCESS) {
            break;
        }

        bytes_read += field_read;
    }

    if (field_status != CHRPC_SUCCESS) {
        // Delete all correctly constructed inner values.
        for (size_t i = 0; i < fi; i++) {
            delete_chrpc_inner_value(ct->struct_fields_types->field_types[i], fields[i]); 
        }
        safe_free(fields);
        return field_status;
    }

    chrpc_inner_value_t *struct_iv = (chrpc_inner_value_t *)
        safe_malloc(sizeof(chrpc_inner_value_t));

    struct_iv->struct_entries = fields;

    *iv = struct_iv;
    *readden = bytes_read;

    return CHRPC_SUCCESS;
}

// Specifically for an array of strings.
static chrpc_status_t chrpc_str_array_inner_value_from_buffer(const chrpc_type_t *ct, chrpc_inner_value_t **iv, const uint8_t *buf, size_t buf_len, size_t *readden) {
    size_t bytes_read = 0;

    if (buf_len < sizeof(uint32_t)) {
        return CHRPC_UNEXPECTED_END;
    }

    uint32_t num_eles = *(uint32_t *)buf;
    bytes_read += sizeof(uint32_t);

    char **str_arr = (char **)safe_malloc(sizeof(char *) * num_eles);
    
    // NOTE: We must give this an initial value because an array may have 0 elements.
    chrpc_status_t str_status = CHRPC_SUCCESS;

    uint32_t si;
    for (si = 0; si < num_eles; si++) {
        size_t tmp_read;

        char *str;

        // Array Cell type should always be a string here! (Don't need the length here)
        str_status = chrpc_block_array_from_buffer(ct->array_cell_type, (void **)&str, NULL, buf + bytes_read, buf_len - bytes_read, &tmp_read); 
        if (str_status != CHRPC_SUCCESS) {
            break;
        }
        
        bytes_read += tmp_read;
        str_arr[si] = str;
    }

    if (str_status != CHRPC_SUCCESS) {
        for (uint32_t i = 0; i < si; i++) {
            safe_free(str_arr[i]);
        }
        safe_free(str_arr);
        return str_status;
    }

    // Otherwise success!
    chrpc_inner_value_t *ivp = (chrpc_inner_value_t *)
        safe_malloc(sizeof(chrpc_inner_value_t));

    ivp->array_len = num_eles;
    ivp->str_arr = str_arr;

    *readden = bytes_read;
    *iv = ivp;

    return CHRPC_SUCCESS;
}

static chrpc_status_t chrpc_cmp_array_inner_value_from_buffer(const chrpc_type_t *ct, chrpc_inner_value_t **iv, const uint8_t *buf, size_t buf_len, size_t *readden) {
    size_t bytes_read = 0;

    if (buf_len < sizeof(uint32_t)) {
        return CHRPC_UNEXPECTED_END;
    }

    uint32_t num_eles = *(uint32_t *)buf;
    bytes_read += sizeof(uint32_t);

    chrpc_inner_value_t **arr = (chrpc_inner_value_t **)
        safe_malloc(sizeof(chrpc_inner_value_t *) * num_eles);

    uint32_t ci;
    chrpc_status_t status;

    for (ci = 0; ci < num_eles; ci++) {
        size_t tmp_read;
        chrpc_inner_value_t *val;

        status = chrpc_inner_value_from_buffer(ct->array_cell_type, &val, buf + bytes_read, buf_len - bytes_read, &tmp_read);
        if (status != CHRPC_SUCCESS) {
            break;
        }

        bytes_read += tmp_read;
        arr[ci] = val;
    }

    if (status != CHRPC_SUCCESS) {
        for (uint32_t i = 0; i < ci; i++) {
            delete_chrpc_inner_value(ct->array_cell_type, arr[i]);
        }
        safe_free(arr);
        return status;
    }

    chrpc_inner_value_t *ivp = 
        (chrpc_inner_value_t *)safe_malloc(sizeof(chrpc_inner_value_t));

    ivp->array_len = num_eles;
    ivp->array_entries = arr;

    *iv = ivp;
    *readden = bytes_read;

    return CHRPC_SUCCESS;
}

chrpc_status_t chrpc_inner_value_from_buffer(const chrpc_type_t *ct, chrpc_inner_value_t **iv, const uint8_t *buf, size_t buf_len, size_t *readden) {
    if (chrpc_type_is_blockable(ct)) {
        // Serialized dense array case. (Numeric arrays or strings)
        if (ct->type_id == CHRPC_STRING_TID || ct->type_id == CHRPC_ARRAY_TID) {
            return chrpc_block_array_inner_value_from_buffer(ct, iv, buf, buf_len, readden);
        }

        // Numeric case.
        return chrpc_numeric_inner_value_from_buffer(ct, iv, buf, buf_len, readden);
    }

    // It's not blockable:
    // Either a struct, an array of strings, or an array of composites.

    if (ct->type_id == CHRPC_STRUCT_TID) {
        return chrpc_struct_inner_value_from_buffer(ct, iv, buf, buf_len, readden);
    }


    // Array of strings case.
    if (ct->array_cell_type->type_id == CHRPC_STRING_TID) {
        return chrpc_str_array_inner_value_from_buffer(ct, iv, buf, buf_len, readden);
    }

    // Otherwise, composite array.
    return chrpc_cmp_array_inner_value_from_buffer(ct, iv, buf, buf_len, readden);
}

