
#include "chrpc/serial.h"
#include "chsys/mem.h"
#include "chutil/list.h"
#include <stdarg.h>

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
static chrpc_type_t *new_chrpc_struct_type_from_internals(size_t num_fields, chrpc_type_t **field_types) {
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

    size_t num_fields = l_len(l);
    chrpc_type_t **sfts = (chrpc_type_t **)
        safe_malloc(sizeof(chrpc_type_t *) * num_fields);

    for (size_t i = 0; i < num_fields; i++) {
        l_get_copy(l, i, &(sfts[i]));
    }

    delete_list(l);

    return new_chrpc_struct_type_from_internals(num_fields, sfts);
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

    size_t num_fields = ct->struct_fields_types->num_fields;
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

