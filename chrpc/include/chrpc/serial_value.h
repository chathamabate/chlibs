
#ifndef CHRPC_SERIAL_VALUE_H
#define CHRPC_SERIAL_VALUE_H

#include "chrpc/serial_type.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "chutil/misc.h"

typedef struct _chrpc_inner_value_t {
    union {
        // The primitive types will be stored densely in arrays.
        // Hence why they have their own array fields.
        //
        // An array's size in memory is always greater than or equal to the
        // length of the array.
        // If an array's length is 0, the array can be NULL.
        //
        // The struct entries array will never be empty when used since a 
        // struct with no fields is always invalid.
        //
        // EVERYTHING WILL BE ON THE HEAP!

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

        float f32;
        float *f32_arr;

        double f64;
        double *f64_arr;

        // UB if a given string's length is larger than UINT32_MAX
        char *str;
        char **str_arr;

        struct _chrpc_inner_value_t **array_entries;
        struct _chrpc_inner_value_t **struct_entries;
    };

    // This field is only used for array types.
    uint32_t array_len;
} chrpc_inner_value_t;

// This will delete the given inner value.
// The given type t will be used to perform the correct deletion routine on iv,
// but t itself will be left untouched.
void delete_chrpc_inner_value(const chrpc_type_t *t, chrpc_inner_value_t *iv);

// A serialized value is really just pair.
// The "value" field can only be interpreted using the type.
//
// chrpc_value_t will "own" both the type and value fields.

typedef struct _chrpc_value_t {
    chrpc_type_t *type;
    chrpc_inner_value_t *value;
} chrpc_value_t;

void delete_chrpc_value(chrpc_value_t *v);

// NOTE: In the below calls, array_len is ALWAYS the number of cells in the array,
// NOT the number of bytes.
//
// NOTE: VERY IMPORTANT: For the below primitive value calls.  ..._array_value(*arr, arr_len)
// the given array must be stored in dynamic memory and will be OWNED by the resulting value.

chrpc_value_t *new_chrpc_b8_value(uint8_t b8_val);
chrpc_value_t *new_chrpc_b8_array_value(uint8_t *b8_array_val, uint32_t array_len);
static inline chrpc_value_t *new_chrpc_b8_empty_array_value(void) {
    return new_chrpc_b8_array_value(NULL, 0);
}
chrpc_value_t *_new_chrpc_b8_array_value_va(uint32_t num_eles,...);
#define new_chrpc_b8_array_value_va(...) _new_chrpc_b8_array_value_va(NUM_ARGS(uint8_t, __VA_ARGS__), __VA_ARGS__)

chrpc_value_t *new_chrpc_i16_value(int16_t i16_val);
chrpc_value_t *new_chrpc_i16_array_value(int16_t *i16_array_val, uint32_t array_len);
static inline chrpc_value_t *new_chrpc_i16_empty_array_value(void) {
    return new_chrpc_i16_array_value(NULL, 0);
}
chrpc_value_t *_new_chrpc_i16_array_value_va(uint32_t num_eles,...);
#define new_chrpc_i16_array_value_va(...) _new_chrpc_i16_array_value_va(NUM_ARGS(int16_t, __VA_ARGS__), __VA_ARGS__)

chrpc_value_t *new_chrpc_i32_value(int32_t i32_val);
chrpc_value_t *new_chrpc_i32_array_value(int32_t *i32_array_val, uint32_t array_len);
static inline chrpc_value_t *new_chrpc_i32_empty_array_value(void) {
    return new_chrpc_i32_array_value(NULL, 0);
}
chrpc_value_t *_new_chrpc_i32_array_value_va(uint32_t num_eles,...);
#define new_chrpc_i32_array_value_va(...) _new_chrpc_i32_array_value_va(NUM_ARGS(int32_t, __VA_ARGS__), __VA_ARGS__)

chrpc_value_t *new_chrpc_i64_value(int64_t i64_val);
chrpc_value_t *new_chrpc_i64_array_value(int64_t *i64_array_val, uint32_t array_len);
static inline chrpc_value_t *new_chrpc_i64_empty_array_value(void) {
    return new_chrpc_i64_array_value(NULL, 0);
}
chrpc_value_t *_new_chrpc_i64_array_value_va(uint32_t num_eles,...);
#define new_chrpc_i64_array_value_va(...) _new_chrpc_i64_array_value_va(NUM_ARGS(int64_t, __VA_ARGS__), __VA_ARGS__)

chrpc_value_t *new_chrpc_u16_value(uint16_t u16_val);
chrpc_value_t *new_chrpc_u16_array_value(uint16_t *u16_array_val, uint32_t array_len);
static inline chrpc_value_t *new_chrpc_u16_empty_array_value(void) {
    return new_chrpc_u16_array_value(NULL, 0);
}
chrpc_value_t *_new_chrpc_u16_array_value_va(uint32_t num_eles,...);
#define new_chrpc_u16_array_value_va(...) _new_chrpc_u16_array_value_va(NUM_ARGS(uint16_t, __VA_ARGS__), __VA_ARGS__)

chrpc_value_t *new_chrpc_u32_value(uint32_t u32_val);
chrpc_value_t *new_chrpc_u32_array_value(uint32_t *u32_array_val, uint32_t array_len);
static inline chrpc_value_t *new_chrpc_u32_empty_array_value(void) {
    return new_chrpc_u32_array_value(NULL, 0);
}
chrpc_value_t *_new_chrpc_u32_array_value_va(uint32_t num_eles,...);
#define new_chrpc_u32_array_value_va(...) _new_chrpc_u32_array_value_va(NUM_ARGS(uint32_t, __VA_ARGS__), __VA_ARGS__)

chrpc_value_t *new_chrpc_u64_value(uint64_t u64_val);
chrpc_value_t *new_chrpc_u64_array_value(uint64_t *u64_array_val, uint32_t array_len);
static inline chrpc_value_t *new_chrpc_u64_empty_array_value(void) {
    return new_chrpc_u64_array_value(NULL, 0);
}
chrpc_value_t *_new_chrpc_u64_array_value_va(uint32_t num_eles,...);
#define new_chrpc_u64_array_value_va(...) _new_chrpc_u64_array_value_va(NUM_ARGS(uint64_t, __VA_ARGS__), __VA_ARGS__)

chrpc_value_t *new_chrpc_f32_value(float u32_val);
chrpc_value_t *new_chrpc_f32_array_value(float *u32_array_val, uint32_t array_len);
static inline chrpc_value_t *new_chrpc_f32_empty_array_value(void) {
    return new_chrpc_f32_array_value(NULL, 0);
}
chrpc_value_t *_new_chrpc_f32_array_value_va(uint32_t num_eles,...);
#define new_chrpc_f32_array_value_va(...) _new_chrpc_f32_array_value_va(NUM_ARGS(float, __VA_ARGS__), __VA_ARGS__)

chrpc_value_t *new_chrpc_f64_value(double u64_val);
chrpc_value_t *new_chrpc_f64_array_value(double *u64_array_val, uint32_t array_len);
static inline chrpc_value_t *new_chrpc_f64_empty_array_value(void) {
    return new_chrpc_f64_array_value(NULL, 0);
}
chrpc_value_t *_new_chrpc_f64_array_value_va(uint32_t num_eles,...);
#define new_chrpc_f64_array_value_va(...) _new_chrpc_f64_array_value_va(NUM_ARGS(double, __VA_ARGS__), __VA_ARGS__)

chrpc_value_t *new_chrpc_str_value(const char *str_val);

// Like the above primitive types, this given array will be owned by the created value.
// NOTE: SO: the given array must live in dynamic memory, AND each pointed to string must 
// live in dynamic memory. (THEY ALL WILL BE DELETED AT DESTRUCTION TIME)
chrpc_value_t *new_chrpc_str_array_value(char **str_array_val, uint32_t array_len);
static inline chrpc_value_t *new_chrpc_str_empty_array_value(void) {
    return new_chrpc_str_array_value(NULL, 0);
}

// For convenience, the varargs flavor of the string array constructor will accept temporary/stack allocated
// strings. Every string given as a vararg will be copied into a dynamic buffer which will then be pointed to by
// a dynamic array. The final array will be given to the above function.
//
// (YOU CAN EVEN GIVE LITERALS)
chrpc_value_t *_new_chrpc_str_array_value_va(int dummy,...);
#define new_chrpc_str_array_value_va(...) _new_chrpc_str_array_value_va(0, __VA_ARGS__, NULL)

// NOTE: For composite array types I created and empty and non-empty constructor.
// Unlike the above constructors, it's impossible to know the type of the resulting array
// if no entries were given!

// NOTE: for composite_nempty_array_value and struct_value, NULL is returned if the given entries array
// is malformed. (For example, if the entries given for the array are of mixed types)
// Remember that on success, the given array becomes property of the created value.
//
// When NULL is returned, the given array persists in memory. It is your job to clean up
// or reuse the array that was rejected.
//
// NOTE: This is not the case for the va flavors of these calls. As it would be impossible to have access
// to the array which was created, when NULL is returned, all created values are deleted.
//
// This is kinda confusing, I'd recommend just looking at the code for these functions.

// The given type will be owned by the created value.
chrpc_value_t *new_chrpc_composite_empty_array_value(chrpc_type_t *t);
chrpc_value_t *new_chrpc_composite_nempty_array_value(chrpc_value_t **entries, uint32_t num_entries);
chrpc_value_t *_new_chrpc_composite_nempty_array_value_va(int dummy,...);
#define new_chrpc_composite_nempty_array_value_va(...) \
    _new_chrpc_composite_nempty_array_value_va(0, __VA_ARGS__, NULL)

// Expects a non-zero number of pointers to other chrpc values.
// Same as above, struct_fields array MUST be a malloc'd array, and will be
// unusable after this call.
chrpc_value_t *new_chrpc_struct_value(chrpc_value_t **struct_fields, uint8_t num_fields);
chrpc_value_t *_new_chrpc_struct_value_va(int dummy,...);
#define new_chrpc_struct_value_va(...) \
    _new_chrpc_struct_value_va(0, __VA_ARGS__, NULL)

bool chrpc_value_equals(const chrpc_value_t *cv0, const chrpc_value_t *cv1);

// Now for actual serialization and deserialization.

// NOTE: We will assume this system is Little Endian.

// Serialization Grammar
//
// Serial(Numeric Value V) = V
// Serial(String Value V) = strlen(V), V[0], V[1], ..., V[strlen(V) - 1], '\0' (NULL Term is included)
// Serial(Struct Value V) = Serial(V.0), Serial(V.1), ..., Serial(V.(num fields - 1))
// Serial(Array Value V)  = V.len, Serial(V[0]), Serial(V[1]), ..., Serial(V[V.len - 1)

chrpc_status_t chrpc_inner_value_to_buffer(const chrpc_type_t *ct, const chrpc_inner_value_t *iv, uint8_t *buf, size_t buf_len, size_t *written);
chrpc_status_t chrpc_inner_value_from_buffer(const chrpc_type_t *ct, chrpc_inner_value_t **iv, const uint8_t *buf, size_t buf_len, size_t *readden);

#endif
