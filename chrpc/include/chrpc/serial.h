
#ifndef CHRPC_SERIAL_H
#define CHRPC_SERIAL_H

#include <stdlib.h>
#include <stdbool.h>

// Type Grammar:
// PRIM ::= BYTE | INT | UINT | STRING
// COMP ::= STRUCT <NUM_FIELDS> TYPE+ 
//          | ARRAY TYPE
// TYPE ::= COMP | PRIM
//
// The number of fields a struct type has will be encoded as a single
// unsigned byte. THUS, a struct can have a maximum of 255 fields.
//
// Values will parsed after parsing the prepended type signature.
// The only values which are serialized in a somewhat unique way are
// strings, which end with \0, and arrays which start with a 32-bit unsigned 
// length value.
//
// NOTE: ALL MULTI BYTE NUMBERIC VALUES ARE LITTLE ENDIAN!

// A collection of status codes used across this library.
typedef enum _chrpc_status_t {
    CHRPC_SUCCESS = 0,
    CHRPC_SYNTAX_ERROR,

    // Returned when the buffer ends before a parse is complete.
    CHRPC_UNEXPECTED_END,

    // Returned if the given buffer isn't large enough.
    CHRPC_BUFFER_TOO_SMALL,
} chrpc_status_t;

typedef enum _chrpc_type_id_t {
    CHRPC_BYTE_TID = 0,

    CHRPC_INT16_TID,
    CHRPC_INT32_TID,
    CHRPC_INT64_TID,

    CHRPC_UINT16_TID,
    CHRPC_UINT32_TID,
    CHRPC_UINT64_TID,

    CHRPC_STRING_TID, 

    CHRPC_STRUCT_TID,
    CHRPC_ARRAY_TID,
} chrpc_type_id_t;

// This relies on primite type IDs being defined consecutively.
static inline bool chrpc_type_id_is_primitive(chrpc_type_id_t tid) {
    return (CHRPC_BYTE_TID <= tid && tid <= CHRPC_STRING_TID);
}

// A Tree like structure for helping specify and work with serializable types.

typedef struct _chrpc_type_t chrpc_type_t;
typedef struct _chrpc_struct_fields_types_t chrpc_struct_fields_types_t;

struct _chrpc_type_t {
    chrpc_type_id_t type_id;

    // When representing a primitive type, the bytes used by the below
    // union are left unused.

    union {
        // Only used if type_id = CHRPC_ARRAY_T
        chrpc_type_t *array_cell_type;

        // Only used if type_id = CHRPC_STRUCT_T
        chrpc_struct_fields_types_t *struct_fields_types;
    };
};

struct _chrpc_struct_fields_types_t {
    size_t num_fields;
    chrpc_type_t **field_types;
};

static inline bool chrpc_type_is_primitive(chrpc_type_t *ct) {
    return chrpc_type_id_is_primitive(ct->type_id);
}

// Primite types will all be singletons stored in static memory.
// You CAN call the type destructor on these types without error,
// the destructor will simply do nothing.
extern chrpc_type_t * const CHRPC_BYTE_T;
extern chrpc_type_t * const CHRPC_INT16_T;
extern chrpc_type_t * const CHRPC_INT32_T;
extern chrpc_type_t * const CHRPC_INT64_T;
extern chrpc_type_t * const CHRPC_UINT16_T;
extern chrpc_type_t * const CHRPC_UINT32_T;
extern chrpc_type_t * const CHRPC_UINT64_T;
extern chrpc_type_t * const CHRPC_STRING_T; 


// NOTE: Just like with the JSON library, these args for the composite types
// are owned by the returned type.
//
// Deleting the returned type will delete the given internal type.
chrpc_type_t *new_chrpc_array_type(chrpc_type_t *array_cell_type);

// This expects a NULL terminated sequence of chrpc_type_t *'s.
// dummy is not used, just for varargs.
chrpc_type_t *_new_chrpc_struct_type(int dummy,...);

// The given varargs cannot be empty.
#define new_chrpc_struct_type(...) \
    _new_chrpc_struct_type(0, __VA_ARGS__, NULL)


void delete_chrpc_type(chrpc_type_t *ct);

chrpc_status_t chrpc_type_to_buffer(chrpc_type_t *ct, uint8_t *buf, size_t buf_len, size_t *written);
chrpc_status_t chrpc_type_from_buffer(uint8_t *buf, size_t buf_len, chrpc_type_t **ct, size_t *readden);

#endif
