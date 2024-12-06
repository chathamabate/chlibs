
#ifndef CHRPC_SERIAL_H
#define CHRPC_SERIAL_H

#include <stdlib.h>

// Tags will be encoded as single bytes.
typedef enum _chrpc_serial_tag_t {
    // The _T suffix signifies a tag used with type annotation.
    
    // Primitive Types.
    CHRPC_BYTE_T_TAG = 0,

    // Numeric Types are expected to be serialized in little endian.
    CHRPC_INT16_T_TAG,
    CHRPC_INT32_T_TAG,
    CHRPC_INT64_T_TAG,

    CHRPC_UINT16_T_TAG,
    CHRPC_UINT32_T_TAG,
    CHRPC_UINT64_T_TAG,

    // Null terminated array of characters.
    CHRPC_STRING_T_TAG, 

    // Composite Types.
    CHRPC_STRUCT_T_START_TAG,
    CHRPC_STRUCT_T_END_TAG,

    CHRPC_ARRAY_T_TAG,
} chrpc_serial_tag_t;

// Type Grammar:
// PRIM ::= BYTE | INT | UINT | STRING
// COMP ::= STRUCT_START TYPE+ STRUCT_END 
//          | ARRAY TYPE
// TYPE ::= COMP | PRIM

// Values will parsed after parsing the prepended type signature.
// The only values which are serialized in a somewhat unique way are
// strings, which end with \0, and arrays which start with a 32-bit unsigned 
// length value.
//
// NOTE: ALL MULTI BYTE NUMBERIC VALUES ARE LITTLE ENDIAN!


// These IDs are used internally to this library and are NEVER sent over
// serial.
typedef enum _chrpc_type_id_t {
    CHRPC_BYTE_T = 0,

    CHRPC_INT16_T,
    CHRPC_INT32_T,
    CHRPC_INT64_T,

    CHRPC_UINT16_T,
    CHRPC_UINT32_T,
    CHRPC_UINT64_T,

    CHRPC_STRING_T, 

    CHRPC_STRUCT_T,
    CHRPC_ARRAY_T,
} chrpc_type_id_t;

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




#endif
