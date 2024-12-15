
#ifndef CHRPC_SERIAL_H
#define CHRPC_SERIAL_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "chutil/misc.h"

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
#define CHRPC_STRUCT_TYPE_TOO_LARGE 4

typedef uint8_t chrpc_type_id_t;

// Primitive Types.

#define CHRPC_BYTE_TID 0
#define CHRPC_INT16_TID 1
#define CHRPC_INT32_TID 2
#define CHRPC_INT64_TID 3
#define CHRPC_UINT16_TID 4
#define CHRPC_UINT32_TID 5
#define CHRPC_UINT64_TID 6
#define CHRPC_STRING_TID 7

// Composite Types.

#define CHRPC_STRUCT_TID 8
#define CHRPC_ARRAY_TID 9


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

#define CHRPC_MAX_STRUCT_FIELDS UINT8_MAX 

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
extern chrpc_type_t * const CHRPC_STRING_T; 

// Returns NULL if given type ID is not primitive.
chrpc_type_t *new_chrpc_primitive_type_from_id(chrpc_type_id_t tid);


// NOTE: Just like with the JSON library, these args for the composite types
// are owned by the returned type.
//
// Deleting the returned type will delete the given internal type.
chrpc_type_t *new_chrpc_array_type(chrpc_type_t *array_cell_type);

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
chrpc_status_t chrpc_type_from_buffer(uint8_t *buf, size_t buf_len, chrpc_type_t **ct, size_t *readden);

bool chrpc_type_equals(const chrpc_type_t *ct1, const chrpc_type_t *ct2);

typedef struct _chrpc_inner_value_t {
    union {
        // The primitive types will be stored densely in arrays.
        // Hence why they have their own array fields.
        //
        // If array_len is 0, the entries array should always be NULL!
        // Regardless of type of array it is.

        uint8_t b8;
        uint8_t *b8_arr;

        int16_t i16;
        int16_t *i16_arr;

        int32_t i32;
        int32_t *i32_arr;

        int64_t i64;
        int64_t *i64_arr;

        uint16_t u16;
        uint16_t *u16_arr;

        uint32_t u32;
        uint32_t *u32_arr;

        uint64_t u64;
        uint64_t *u64_arr;

        char *str;
        char **str_arr;

        struct _chrpc_inner_value_t **array_entries;
        struct _chrpc_inner_value_t **struct_entries;
    };

    // This field is only used for array types.
    uint32_t array_len;
} chrpc_inner_value_t;

// A serialized value is really just pair.
// The "value" field can only be interpreted using the type.
//
// chrpc_value_t will "own" both the type and value fields.

typedef struct _chrpc_value_t {
    chrpc_type_t *type;
    chrpc_inner_value_t *value;
} chrpc_value_t;

// NOTE: In the below calls, array_len is ALWAYS the number of cells in the array,
// NOT the number of bytes.
//
// NOTE: VERY IMPORTANT: For the below primitive value calls.  ..._array_value(*arr, arr_len)
// the given array must be stored in dynamic memory and will be OWNED by the resulting value.

chrpc_value_t *new_chrpc_b8_value(uint8_t b8_val);
chrpc_value_t *new_chrpc_b8_array_value(uint8_t *b8_array_val, uint32_t array_len);
chrpc_value_t *_new_chrpc_b8_array_value_va(uint32_t num_eles,...);
#define new_chrpc_b8_array_value_va(...) _new_chrpc_b8_array_value_va(NUM_ARGS(uint8_t, __VA_ARGS__), __VA_ARGS__)

chrpc_value_t *new_chrpc_i16_value(int16_t i16_val);
chrpc_value_t *new_chrpc_i16_array_value(int16_t *i16_array_val, uint32_t array_len);
chrpc_value_t *_new_chrpc_i16_array_value_va(uint32_t num_eles,...);
#define new_chrpc_i16_array_value_va(...) _new_chrpc_i16_array_value_va(NUM_ARGS(int16_t, __VA_ARGS__), __VA_ARGS__)

chrpc_value_t *new_chrpc_i32_value(int32_t i32_val);
chrpc_value_t *new_chrpc_i32_array_value(int32_t *i32_array_val, uint32_t array_len);
chrpc_value_t *_new_chrpc_i32_array_value_va(uint32_t num_eles,...);
#define new_chrpc_i32_array_value_va(...) _new_chrpc_i32_array_value_va(NUM_ARGS(int32_t, __VA_ARGS__), __VA_ARGS__)

chrpc_value_t *new_chrpc_i64_value(int64_t i64_val);
chrpc_value_t *new_chrpc_i64_array_value(int64_t *i64_array_val, uint32_t array_len);
chrpc_value_t *_new_chrpc_i64_array_value_va(uint32_t num_eles,...);
#define new_chrpc_i64_array_value_va(...) _new_chrpc_i64_array_value_va(NUM_ARGS(int64_t, __VA_ARGS__), __VA_ARGS__)

chrpc_value_t *new_chrpc_u16_value(uint16_t u16_val);
chrpc_value_t *new_chrpc_u16_array_value(uint16_t *u16_array_val, uint32_t array_len);
chrpc_value_t *_new_chrpc_u16_array_value_va(uint32_t num_eles,...);
#define new_chrpc_u16_array_value_va(...) _new_chrpc_u16_array_value_va(NUM_ARGS(uint16_t, __VA_ARGS__), __VA_ARGS__)

chrpc_value_t *new_chrpc_u32_value(uint32_t u32_val);
chrpc_value_t *new_chrpc_u32_array_value(uint32_t *u32_array_val, uint32_t array_len);
chrpc_value_t *_new_chrpc_u32_array_value_va(uint32_t num_eles,...);
#define new_chrpc_u32_array_value_va(...) _new_chrpc_u32_array_value_va(NUM_ARGS(uint32_t, __VA_ARGS__), __VA_ARGS__)

chrpc_value_t *new_chrpc_u64_value(uint64_t u64_val);
chrpc_value_t *new_chrpc_u64_array_value(uint64_t *u64_array_val, uint32_t array_len);
chrpc_value_t *_new_chrpc_u64_array_value_va(uint32_t num_eles,...);
#define new_chrpc_u64_array_value_va(...) _new_chrpc_u64_array_value_va(NUM_ARGS(uint64_t, __VA_ARGS__), __VA_ARGS__)

chrpc_value_t *new_chrpc_str_value(const char *str_val);

// Like the above primitive types, this given array will be owned by the created value.
// NOTE: SO: the given array must live in dynamic memory, AND each pointed to string must 
// live in dynamic memory. (THEY ALL WILL BE DELETED AT DESTRUCTION TIME)
chrpc_value_t *new_chrpc_str_array_value(char **str_array_val, uint32_t array_len);

// For convenience, the varargs flavor of the string array constructor will accept temporary/stack allocated
// strings. Every string given as a vararg will be copied into a dynamic buffer which will then be pointed to by
// a dynamic array. The final array will be given to the above function.
//
// (YOU CAN EVEN GIVE LITERALS)
chrpc_value_t *_new_chrpc_str_array_value_va(int dummy,...);
#define new_chrpc_str_array_value_va(...) _new_chrpc_str_array_value_va(0, __VA_ARGS__, NULL)

// Expects a non-zero number of pointers to other chrpc values.
// returns NULL if given length is 0, OR cell types don't match.
//
// NOTE: VERY IMPORTANT: all resrouces used by array_entries will either be moved
// into the new chrpc_value_t, or freed. This call REQUIRES that array_entries
// is a DYNAMICALLY ALLOCATED array on the heap. It will be freed within this call!
//
// Also if NULL, is returned because you screwed up the typing, the array_entries array
// will be left in its given condition.
/*
chrpc_value_t *new_chrpc_array_value(chrpc_value_t **array_entries, uint32_t array_len);
chrpc_value_t *_new_chrpc_array_value_va(int dummy,...);
#define new_chrpc_array_value_va(...) \
    _new_chrpc_array_value_va(0, __VA_ARGS__, NULL)
*/

// Expects a non-zero number of pointers to other chrpc values.
// Same as above, struct_fields array MUST be a malloc'd array, and will be
// unusable after this call.
/*
chrpc_value_t *new_chrpc_struct_value(chrpc_value_t **struct_fields, uint32_t num_fields);
chrpc_value_t *_new_chrpc_struct_value_va(int dummy,...);
#define new_chrpc_struct_value_va(...) \
    _new_chrpc_struct_value_va(0, __VA_ARGS__, NULL)
*/

#endif
