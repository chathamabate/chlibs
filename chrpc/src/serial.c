
#include "chrpc/serial.h"
#include "chsys/mem.h"
#include "chutil/list.h"
#include <stdarg.h>
#include <string.h>

static chrpc_type_t _CHRPC_BYTE_T = {
    .type_id = CHRPC_BYTE_TID
};
static chrpc_type_t _CHRPC_INT16_T = {
    .type_id = CHRPC_INT16_TID
};
static chrpc_type_t _CHRPC_INT32_T = {
    .type_id = CHRPC_INT32_TID
};
static chrpc_type_t _CHRPC_INT64_T = {
    .type_id = CHRPC_INT64_TID
};
static chrpc_type_t _CHRPC_UINT16_T = {
    .type_id = CHRPC_UINT16_TID
};
static chrpc_type_t _CHRPC_UINT32_T = {
    .type_id = CHRPC_UINT32_TID
};
static chrpc_type_t _CHRPC_UINT64_T = {
    .type_id = CHRPC_UINT64_TID
};
static chrpc_type_t _CHRPC_STRING_T = {
    .type_id = CHRPC_STRING_TID
}; 

chrpc_type_t * const CHRPC_BYTE_T = &_CHRPC_BYTE_T;
chrpc_type_t * const CHRPC_INT16_T = &_CHRPC_INT16_T;
chrpc_type_t * const CHRPC_INT32_T = &_CHRPC_INT32_T;
chrpc_type_t * const CHRPC_INT64_T = &_CHRPC_INT64_T;
chrpc_type_t * const CHRPC_UINT16_T = &_CHRPC_UINT16_T;
chrpc_type_t * const CHRPC_UINT32_T = &_CHRPC_UINT32_T;
chrpc_type_t * const CHRPC_UINT64_T = &_CHRPC_UINT64_T;
chrpc_type_t * const CHRPC_STRING_T = &_CHRPC_STRING_T; 

chrpc_type_t *new_chrpc_primitive_type_from_id(chrpc_type_id_t tid) {
    switch (tid) {
    case CHRPC_BYTE_TID:
        return CHRPC_BYTE_T;
    case CHRPC_INT16_TID:
        return CHRPC_INT16_T;
    case CHRPC_INT32_TID:
        return CHRPC_INT32_T;
    case CHRPC_INT64_TID:
        return CHRPC_INT64_T;
    case CHRPC_UINT16_TID:
        return CHRPC_UINT16_T;
    case CHRPC_UINT32_TID:
        return CHRPC_UINT32_T;
    case CHRPC_UINT64_TID:
        return CHRPC_UINT64_T;
    case CHRPC_STRING_TID:
        return CHRPC_STRING_T;
    default:
        return NULL;
    }
}

chrpc_type_t *new_chrpc_array_type(chrpc_type_t *array_cell_type) {
    chrpc_type_t *ct = (chrpc_type_t *)safe_malloc(sizeof(chrpc_type_t));
    ct->type_id = CHRPC_ARRAY_TID;
    ct->array_cell_type = array_cell_type;

    return ct;
}

// Again, the given array will be placed into the created struct type. (i.e. ownership transfer)
static chrpc_type_t *new_chrpc_struct_type_from_internals(uint8_t num_fields, chrpc_type_t **field_types) {
    chrpc_struct_fields_types_t *sfts = (chrpc_struct_fields_types_t *)
        safe_malloc(sizeof(chrpc_struct_fields_types_t));

    sfts->num_fields = num_fields;
    sfts->field_types = field_types;

    chrpc_type_t *ct = (chrpc_type_t *)safe_malloc(sizeof(chrpc_type_t));
    ct->type_id = CHRPC_STRUCT_TID;
    ct->struct_fields_types = sfts;

    return ct;
}

chrpc_type_t *_new_chrpc_struct_type(int dummy,...) {
    va_list args;
    va_start(args, dummy);

    // Kinda awkward here, we will populate this array list,
    // then afterwards copy it into a normal array.

    chrpc_type_t *iter;
    list_t *l = new_list(ARRAY_LIST_IMPL, sizeof(chrpc_type_t *));

    while ((iter = va_arg(args, chrpc_type_t *))) {
        l_push(l, &iter);
    }

    va_end(args);

    size_t num_fields = l_len(l);
    chrpc_type_t **sfts = (chrpc_type_t **)
        safe_malloc(sizeof(chrpc_type_t *) * num_fields);

    for (size_t i = 0; i < num_fields; i++) {
        l_get_copy(l, i, &(sfts[i]));
    }

    delete_list(l);

    return new_chrpc_struct_type_from_internals((uint8_t)num_fields, sfts);
}

void delete_chrpc_type(chrpc_type_t *ct) {
    if (chrpc_type_is_primitive(ct)) {
        return; // DO NOTHING FOR SINGLETONS.
    }

    if (ct->type_id == CHRPC_ARRAY_TID) {
        delete_chrpc_type(ct->array_cell_type);
    } else if (ct->type_id == CHRPC_STRUCT_TID) {
        for (size_t i = 0; i < ct->struct_fields_types->num_fields; i++) {
            delete_chrpc_type(ct->struct_fields_types->field_types[i]);
        }

        safe_free(ct->struct_fields_types->field_types);
        safe_free(ct->struct_fields_types);
    }

    safe_free(ct);
}

chrpc_status_t chrpc_type_to_buffer(const chrpc_type_t *ct, uint8_t *buf, size_t buf_len, size_t *written) {
    if (buf_len < 1) {
        return CHRPC_BUFFER_TOO_SMALL; 
    } 

    if (chrpc_type_is_primitive(ct)) {
        buf[0] = ct->type_id;
        *written = 1;
        return CHRPC_SUCCESS;
    }

    // Now time for recursive types.
    // We will assume the given chrpc_type_t is always correctly formed.

    if (ct->type_id == CHRPC_ARRAY_TID) {
        buf[0] = CHRPC_ARRAY_TID;

        size_t array_cell_type_written = 0;
        chrpc_status_t rec_status = 
            chrpc_type_to_buffer(ct->array_cell_type, buf + 1, buf_len - 1, &array_cell_type_written); 
        
        *written = array_cell_type_written + 1;

        return rec_status;
    }

    // Struct type will be most complex.
    // Needs at least 2 bytes to be written.
    // Really, it needs more bytes, but this will be handled by recursive calls.
    
    if (buf_len < 2) {
        return CHRPC_BUFFER_TOO_SMALL;
    }

    uint8_t num_fields = ct->struct_fields_types->num_fields;
    chrpc_type_t **fields = ct->struct_fields_types->field_types;

    buf[0] = CHRPC_STRUCT_TID;
    buf[1] = num_fields;

    size_t pos = 2;
    for (size_t i = 0; i < num_fields; i++) {
        size_t field_type_written = 0;
        chrpc_status_t rec_status = 
            chrpc_type_to_buffer(fields[i], buf + pos, buf_len - pos, &field_type_written);

        if (rec_status != CHRPC_SUCCESS) {
            return rec_status;
        }

        pos += field_type_written;
    }

    *written = pos;

    return CHRPC_SUCCESS;
}

chrpc_status_t chrpc_type_from_buffer(uint8_t *buf, size_t buf_len, chrpc_type_t **ct, size_t *readden) {
    if (buf_len < 1) {
        return CHRPC_UNEXPECTED_END;
    }
    
    chrpc_type_id_t tid = (chrpc_type_id_t)buf[0];

    chrpc_type_t *prim_type = new_chrpc_primitive_type_from_id(tid);
    if (prim_type) {
        *readden = 1;
        *ct = prim_type;
        return CHRPC_SUCCESS;
    }

    // Non primitive scenario.

    if (tid == CHRPC_ARRAY_TID) {
        chrpc_type_t *array_field_type;
        size_t array_field_type_readden = 0; 
        
        chrpc_status_t rec_status = 
            chrpc_type_from_buffer(buf + 1, buf_len - 1, &array_field_type, &array_field_type_readden);

        if (rec_status != CHRPC_SUCCESS) {
            return rec_status;
        }

        *ct = new_chrpc_array_type(array_field_type);
        *readden = 1 + array_field_type_readden;
        
        return CHRPC_SUCCESS;
    }

    if (tid == CHRPC_STRUCT_TID) {
        if (buf_len < 2) {
            return CHRPC_UNEXPECTED_END;
        }

        uint8_t num_fields = buf[1];
        
        if (num_fields < 1) {
            return CHRPC_EMPTY_STRUCT_TYPE;
        }

        size_t pos = 2;
        chrpc_status_t rec_status = CHRPC_SUCCESS;

        chrpc_type_t **fields = 
            (chrpc_type_t **)safe_malloc(sizeof(chrpc_type_t) * num_fields);

        for (size_t i = 0; i < num_fields; i++) {
            size_t field_readden; 
            rec_status = chrpc_type_from_buffer(buf + pos, buf_len - pos, &(fields[i]), &field_readden);

            // Error case, remember to clean up all previously made types.
            if (rec_status != CHRPC_SUCCESS) {
                for (size_t j = 0; j < i; j++) {
                    delete_chrpc_type(fields[j]);
                }
                safe_free(fields);
                return rec_status;
            }

            pos += field_readden;
        }

        *ct = new_chrpc_struct_type_from_internals(num_fields, fields);
        *readden = pos;

        return CHRPC_SUCCESS;
    }

    // Bad type ID found.
    return CHRPC_SYNTAX_ERROR;
}

bool chrpc_type_equals(const chrpc_type_t *ct1, const chrpc_type_t *ct2) {
    if (ct1->type_id != ct2->type_id) {
        return false;
    }

    chrpc_type_id_t tid = ct1->type_id;

    if (chrpc_type_id_is_primitive(tid)) {
        return true;
    }
    
    if (tid == CHRPC_ARRAY_TID) {
        return chrpc_type_equals(ct1->array_cell_type, ct2->array_cell_type);
    }

    // Otherwise we are using a struct type.
    
    if (ct1->struct_fields_types->num_fields != ct2->struct_fields_types->num_fields) {
        return false;
    }

    size_t num_fields = ct1->struct_fields_types->num_fields;

    for (size_t i = 0; i < num_fields; i++) {
        if (!chrpc_type_equals(ct1->struct_fields_types->field_types[i], 
                    ct2->struct_fields_types->field_types[i])) {
            return false; 
        }
    }

    return true;
}

// --------------------------- VALUE WORK ---------------------------

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
    array_list_t *al = new_array_list(sizeof(char *));

    return NULL;

}


/*

chrpc_value_t *new_chrpc_array_value(chrpc_value_t **array_entries, uint32_t array_len) {
    if (array_len == 0) {
        return NULL;
    }

    // All cells must have the same type!
    chrpc_type_t *cell_type = array_entries[0]->type;

    for (uint32_t i = 1; i < array_len; i++) {
        if (!chrpc_type_equals(cell_type, array_entries[i]->type)) {
            return NULL;
        }
    }

    // We will only hold onto the first type, the rest can be deleted.
    for (uint32_t i = 1; i < array_len; i++) {
        delete_chrpc_type(array_entries[i]->type);
    }



    // We need to edit this if primite and non-string tbh.

    // Is our type primitive and non-string?
    // IDK my head kinda hurts today tbh...

    // Now, we will transfer all inner values into a new ivs array which
    // will be used to create our new chrpc_value_t.
    // 
    // Afterwards we will need to delete the left over malloc'd memory 
    // from the array_entries.

    chrpc_inner_value_t **array_entry_ivs = 
        (chrpc_inner_value_t **)safe_malloc(sizeof(chrpc_inner_value_t *) * array_len);

    for (uint32_t i = 0; i < array_len; i++) {
        array_entry_ivs[i] = array_entries[i]->value;
    }

    // NOTE: we will be saving the first cell type, so don't delete it!


    // This is slightly dangerous, but now we WILL delete all of the left over structs.
    for (uint32_t i = 0; i < array_len; i++) {
        safe_free(array_entries[i]);
    }
    safe_free(array_entries);

    // Finally create and return our new chrpc_value_t.

    chrpc_inner_value_t *iv = (chrpc_inner_value_t *)safe_malloc(sizeof(chrpc_inner_value_t));
    iv->array_entries = array_entry_ivs;
    iv->array_len = array_len;

    return new_chrpc_value_from_pair(new_chrpc_array_type(cell_type), iv);
}

chrpc_value_t *_new_chrpc_array_value_va(int dummy,...) {
    va_list args;
    va_start(args, dummy);

    array_list_t *al = new_array_list(sizeof(chrpc_value_t *));
    chrpc_value_t *iter;
    while ((iter = va_arg(args, chrpc_value_t *))) {
        al_push(al, &iter);
    }

    size_t len = al_len(al);;
    chrpc_value_t **array_entries = 
        (chrpc_value_t **)delete_and_move_array_list(al);

    return new_chrpc_array_value(array_entries, (uint32_t)len);
}

chrpc_value_t *new_chrpc_struct_value(chrpc_value_t **struct_fields, uint32_t num_fields) {
    if (num_fields == 0) {
        return NULL;
    }

    chrpc_type_t **field_types = 
        (chrpc_type_t **)safe_malloc(sizeof(chrpc_type_t *) * num_fields);
    chrpc_inner_value_t **ivs = 
        (chrpc_inner_value_t **)safe_malloc(sizeof(chrpc_inner_value_t *) * num_fields);

    for (uint32_t i = 0; i < num_fields; i++) {
        field_types[i] = struct_fields[i]->type;
        ivs[i] = struct_fields[i]->value;
    }

    chrpc_type_t *struct_type = new_chrpc_struct_type_from_internals(num_fields, field_types);
    chrpc_inner_value_t *struct_iv = (chrpc_inner_value_t *)safe_malloc(sizeof(chrpc_inner_value_t));
    struct_iv->struct_entries = ivs;

    chrpc_value_t *struct_value = new_chrpc_value_from_pair(struct_type, struct_iv);

    // Finally, delete left over structs and array.
    for (uint32_t i = 0; i < num_fields; i++) {
        safe_free(struct_fields[i]);
    }
    safe_free(struct_fields);

    return struct_value;
}

chrpc_value_t *_new_chrpc_struct_value_va(int dummy,...) {
    return NULL;
}

*/

