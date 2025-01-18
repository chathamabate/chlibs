
#ifndef CHRPC_SERIAL_TYPE_H
#define CHRPC_SERIAL_TYPE_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

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

// I will manually define many of these constant numeric values because
// they may be serialized themselves. Sizing is kinda funky with C enum types.

// A collection of status codes used across this library.
typedef uint8_t chrpc_status_t;

#define CHRPC_SUCCESS 0
#define CHRPC_SYNTAX_ERROR 1
#define CHRPC_UNEXPECTED_END 2
#define CHRPC_BUFFER_TOO_SMALL 3
#define CHRPC_EMPTY_STRUCT_TYPE 4
#define CHRPC_STRUCT_TYPE_TOO_LARGE 5
#define CHRPC_MALFORMED_TYPE 6
#define CHRPC_SERVER_CREATION_ERROR 7
#define CHRPC_SERVER_FULL 8
#define CHRPC_UKNOWN_ENDPOINT 9
#define CHRPC_ARGUMENT_MISMATCH 10
#define CHRPC_CLIENT_CHANNEL_EMTPY 11
#define CHRPC_CLIENT_CHANNEL_ERROR 12 
#define CHRPC_BAD_REQUEST 13
#define CHRPC_SERVER_INTERNAL_ERROR 14
#define CHRPC_DISCONNECT 15

typedef uint8_t chrpc_type_id_t;

// Primitive Types.

#define CHRPC_BYTE_TID 0
#define CHRPC_INT16_TID 1
#define CHRPC_INT32_TID 2
#define CHRPC_INT64_TID 3
#define CHRPC_UINT16_TID 4
#define CHRPC_UINT32_TID 5
#define CHRPC_UINT64_TID 6
#define CHRPC_FLOAT32_TID 7
#define CHRPC_FLOAT64_TID 8
#define CHRPC_STRING_TID 9

// Composite Types.

#define CHRPC_STRUCT_TID 10
#define CHRPC_ARRAY_TID 11


// This relies on primite type IDs being defined consecutively.
static inline bool chrpc_type_id_is_primitive(chrpc_type_id_t tid) {
    return (/*CHRPC_BYTE_TID <= tid &&*/ tid <= CHRPC_STRING_TID);
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

#define CHRPC_MAX_STRUCT_FIELDS 64

struct _chrpc_struct_fields_types_t {
    uint8_t num_fields;
    chrpc_type_t **field_types;
};

static inline bool chrpc_type_is_primitive(const chrpc_type_t *ct) {
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
extern chrpc_type_t * const CHRPC_FLOAT32_T;
extern chrpc_type_t * const CHRPC_FLOAT64_T;
extern chrpc_type_t * const CHRPC_STRING_T; 

// Returns NULL if given type ID is not primitive.
chrpc_type_t *new_chrpc_primitive_type_from_id(chrpc_type_id_t tid);

// NOTE: Just like with the JSON library, these args for the composite types
// are owned by the returned type.
//
// Deleting the returned type will delete the given internal type.
chrpc_type_t *new_chrpc_array_type(chrpc_type_t *array_cell_type);

chrpc_type_t *new_chrpc_struct_type_from_internals(chrpc_type_t **field_types, uint8_t num_fields);

// This expects a NULL terminated sequence of chrpc_type_t *'s.
// dummy is not used, just for varargs.
//
// NOTE: Returns NULL if too many fields given (or not enough)
chrpc_type_t *_new_chrpc_struct_type(int dummy,...);

// The given varargs cannot be empty.
#define new_chrpc_struct_type(...) \
    _new_chrpc_struct_type(0, __VA_ARGS__, NULL)


void delete_chrpc_type(chrpc_type_t *ct);

// I'm deciding to use buffer directly here instead of the stream types 
// found in chutil. (Really just because the channel interface uses buffers)

chrpc_status_t chrpc_type_to_buffer(const chrpc_type_t *ct, uint8_t *buf, size_t buf_len, size_t *written);
chrpc_status_t chrpc_type_from_buffer(chrpc_type_t **ct, const uint8_t *buf, size_t buf_len, size_t *readden);

// Make a copy of a given type.
chrpc_type_t *new_chrpc_type_copy(const chrpc_type_t *ct);

bool chrpc_type_equals(const chrpc_type_t *ct1, const chrpc_type_t *ct2);

#endif
