
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

        safe_free(ct->struct_fields_types);
    }

    safe_free(ct);
}

chrpc_status_t chrpc_type_to_buffer(chrpc_type_t *ct, uint8_t *buf, size_t buf_len, size_t *written) {
    if (chrpc_type_is_primitive(ct)) {
        if (buf_len == 0) {
            return CHRPC_BUFFER_TOO_SMALL; 
        } 

        buf[0] =ct->type_id;
    }

    return CHRPC_SUCCESS;
}

chrpc_status_t chrpc_type_from_buffer(uint8_t *buf, size_t buf_len, chrpc_type_t **ct, size_t *readden) {
    return CHRPC_SUCCESS;
}

