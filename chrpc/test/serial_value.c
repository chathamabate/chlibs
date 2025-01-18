
#include "serial_value.h"
#include "chrpc/serial_type.h"
#include "chrpc/serial_value.h"

#include "chsys/mem.h"
#include "unity/unity.h"
#include "unity/unity_internals.h"

#define TEST_CHRPC_TEST_BUFFER_LEN 512

static void test_chrpc_value_simple_constructors(void) {
    chrpc_value_t *cv;

    cv = new_chrpc_b8_value(5);
    TEST_ASSERT_EQUAL_UINT8(5, cv->value->b8);
    delete_chrpc_value(cv);

    cv = new_chrpc_i16_value(-23);
    TEST_ASSERT_EQUAL_INT16(-23, cv->value->i16);
    delete_chrpc_value(cv);

    cv = new_chrpc_i32_value(-3444);
    TEST_ASSERT_EQUAL_INT32(-3444, cv->value->i32);
    delete_chrpc_value(cv);

    cv = new_chrpc_i64_value(-556655);
    TEST_ASSERT_EQUAL_INT64(-556655, cv->value->i64);
    delete_chrpc_value(cv);

    cv = new_chrpc_u16_value(23);
    TEST_ASSERT_EQUAL_UINT16(23, cv->value->u16);
    delete_chrpc_value(cv);

    cv = new_chrpc_u32_value(3444);
    TEST_ASSERT_EQUAL_UINT32(3444, cv->value->u32);
    delete_chrpc_value(cv);

    cv = new_chrpc_u64_value(556655);
    TEST_ASSERT_EQUAL_UINT64(556655, cv->value->u64);
    delete_chrpc_value(cv);

    cv = new_chrpc_f32_value(1.34);
    TEST_ASSERT_EQUAL_FLOAT(1.34, cv->value->f32);
    delete_chrpc_value(cv);

    cv = new_chrpc_f64_value(-231.34);
    TEST_ASSERT_EQUAL_DOUBLE(-231.34, cv->value->f64);
    delete_chrpc_value(cv);
}

static void test_chrpc_value_simple_array_constructors(void) {
    chrpc_value_t *cv;

    cv = new_chrpc_b8_array_value_va(1, 2, 3);
    uint8_t b8_arr[] = {1, 2, 3};
    TEST_ASSERT_EQUAL_UINT8_ARRAY(b8_arr, cv->value->b8_arr, 3);
    delete_chrpc_value(cv);

    cv = new_chrpc_i32_array_value_va(-1, 212, -3443);
    int32_t i32_arr[] = {-1, 212, -3443};
    TEST_ASSERT_EQUAL_INT32_ARRAY(i32_arr, cv->value->i32_arr, 3);
    delete_chrpc_value(cv);

    cv = new_chrpc_f32_array_value_va(1.0f, 3.3f, 4343.9f);
    float f32_arr[] = {1.0f, 3.3f, 4343.9f};
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(f32_arr, cv->value->f32_arr, 3);
    delete_chrpc_value(cv);

    cv = new_chrpc_f64_array_value_va(1.23, 3.1111, 4343.34343, 112323.1);
    double f64_arr[] = {1.23, 3.1111, 4343.34343, 112323.1};
    TEST_ASSERT_EQUAL_FLOAT_ARRAY(f64_arr, cv->value->f64_arr, 4);
    delete_chrpc_value(cv);

    cv = new_chrpc_u64_empty_array_value();
    TEST_ASSERT_EQUAL_UINT32(0, cv->value->array_len);
    delete_chrpc_value(cv);
}

static void test_chrpc_value_str_constructors(void) {
    chrpc_value_t *cv;

    const char *temp_str = "Hello";
    cv = new_chrpc_str_value(temp_str);
    TEST_ASSERT_EQUAL_STRING(temp_str, cv->value->str);

    // Confirm a copy is held, not the OG String.
    TEST_ASSERT(temp_str != cv->value->str);

    delete_chrpc_value(cv);

    cv = new_chrpc_str_array_value_va("Hello", "Aye-Yo", "Uh-Oh");
    const char *strs[] = {
        "Hello", "Aye-Yo", "Uh-Oh"
    };
    TEST_ASSERT_EQUAL_STRING_ARRAY(strs, cv->value->str_arr, 3);
    delete_chrpc_value(cv);
}

static void test_chrpc_struct_value(void) {
    chrpc_value_t *cv;

    cv = new_chrpc_struct_value_va(
        new_chrpc_str_value("Billy"),
        new_chrpc_str_value("Smith"),
        new_chrpc_b8_value(15)
    );

    TEST_ASSERT_EQUAL_STRING("Billy", cv->value->struct_entries[0]->str);
    TEST_ASSERT_EQUAL_STRING("Smith", cv->value->struct_entries[1]->str);
    TEST_ASSERT_EQUAL_UINT8(15, cv->value->struct_entries[2]->b8);

    chrpc_type_t *exp_type = new_chrpc_struct_type(
        CHRPC_STRING_T,
        CHRPC_STRING_T,
        CHRPC_BYTE_T
    );

    TEST_ASSERT_TRUE(chrpc_type_equals(exp_type, cv->type));
    delete_chrpc_type(exp_type);

    delete_chrpc_value(cv);

    // 70 values should be too much here.
    cv = new_chrpc_struct_value_va(
        new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1),
        new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1),
        new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1),
        new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1),
        new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1),
        new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1),
        new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1),
        new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1),
        new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1),
        new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1), new_chrpc_b8_value(1)
    );
    TEST_ASSERT_NULL(cv);
}

static void test_chrpc_composite_array(void) {
    chrpc_value_t *cv; 

    cv = new_chrpc_composite_nempty_array_value_va(
        new_chrpc_struct_value_va(
            new_chrpc_str_value("Bobby"),
            new_chrpc_i16_value(45)
        ),
        new_chrpc_struct_value_va(
            new_chrpc_str_value("Mark"),
            new_chrpc_i16_value(32)
        ),
        new_chrpc_struct_value_va(
            new_chrpc_str_value("David"),
            new_chrpc_i16_value(33)
        )
    );

    TEST_ASSERT_EQUAL_UINT32(3, cv->value->array_len);
    TEST_ASSERT_EQUAL_STRING("Mark", cv->value->array_entries[1]->struct_entries[0]->str);
    TEST_ASSERT_EQUAL_INT16(33, cv->value->array_entries[2]->struct_entries[1]->i16);

    delete_chrpc_value(cv);

    cv = new_chrpc_composite_empty_array_value(
        new_chrpc_struct_type(CHRPC_STRING_T)
    );
    TEST_ASSERT_NOT_NULL(cv);
    delete_chrpc_value(cv);

    cv = new_chrpc_composite_nempty_array_value_va(
        new_chrpc_b8_value(1)
    );
    TEST_ASSERT_NULL(cv);

    cv = new_chrpc_composite_nempty_array_value_va(
        new_chrpc_struct_value_va(
            new_chrpc_b8_value(1)
        ),
        new_chrpc_struct_value_va(
            new_chrpc_i32_value(22)
        )
    );
    TEST_ASSERT_NULL(cv);

    cv = new_chrpc_composite_nempty_array_value_va(
        new_chrpc_f32_value(1.0f),
        new_chrpc_f32_value(1.0f),
        new_chrpc_b8_value(1)
    );
    TEST_ASSERT_NULL(cv);

    cv = new_chrpc_composite_empty_array_value(CHRPC_STRING_T);
    TEST_ASSERT_NULL(cv);
}

static void test_chrpc_big_composite_value(void) {
    chrpc_value_t *cv;

    cv = new_chrpc_composite_nempty_array_value_va(
        new_chrpc_struct_value_va(
            // First/Last Name
            new_chrpc_str_value("Bobby"),
            new_chrpc_str_value("Smith"),

            // Places visited
            new_chrpc_composite_nempty_array_value_va(
                new_chrpc_struct_value_va(
                    new_chrpc_str_value("Milan"),
                    new_chrpc_str_value("Italy")
                ),
                new_chrpc_struct_value_va(
                    new_chrpc_str_value("Paris"),
                    new_chrpc_str_value("France")
                )
            )
        ),
        new_chrpc_struct_value_va(
            // First/Last Name
            new_chrpc_str_value("Mark"),
            new_chrpc_str_value("Berger"),

            // Places visited
            new_chrpc_composite_nempty_array_value_va(
                new_chrpc_struct_value_va(
                    new_chrpc_str_value("Seattle"),
                    new_chrpc_str_value("United States")
                )
            )
        )
    );

    TEST_ASSERT_NOT_NULL(cv);
    TEST_ASSERT_EQUAL_STRING("Milan", 
        cv->value->array_entries[0]->struct_entries[2]->array_entries[0]->struct_entries[0]->str
    );
    TEST_ASSERT_EQUAL_STRING("Berger", 
        cv->value->array_entries[1]->struct_entries[1]->str
    );
    delete_chrpc_value(cv);
}

static void test_assert_eq_chrpc_value(chrpc_value_t *v0, chrpc_value_t *v1) {
    TEST_ASSERT_TRUE(chrpc_value_equals(v0, v1));

    delete_chrpc_value(v0);
    delete_chrpc_value(v1);
}

static void test_assert_neq_chrpc_value(chrpc_value_t *v0, chrpc_value_t *v1) {
    TEST_ASSERT_FALSE(chrpc_value_equals(v0, v1));

    delete_chrpc_value(v0);
    delete_chrpc_value(v1);
}

static void test_chrpc_value_equals(void) {
    test_assert_eq_chrpc_value(
        new_chrpc_b8_value(1),
        new_chrpc_b8_value(1)
    );

    test_assert_neq_chrpc_value(
        new_chrpc_b8_value(2),
        new_chrpc_b8_value(1)
    );

    test_assert_neq_chrpc_value(
        new_chrpc_b8_value(2),
        new_chrpc_i16_value(2)
    );

    test_assert_eq_chrpc_value(
        new_chrpc_str_value("Hello"),
        new_chrpc_str_value("Hello")
    );

    test_assert_neq_chrpc_value(
        new_chrpc_str_value("Hello"),
        new_chrpc_str_value("ello")
    );

    test_assert_eq_chrpc_value(
        new_chrpc_struct_value_va(
            new_chrpc_str_value("Bobby"),
            new_chrpc_i64_value(-123)
        ),
        new_chrpc_struct_value_va(
            new_chrpc_str_value("Bobby"),
            new_chrpc_i64_value(-123)
        )
    );

    test_assert_eq_chrpc_value(
        new_chrpc_f32_value(1.3f),
        new_chrpc_f32_value(1.3f)
    );

    test_assert_eq_chrpc_value(
        new_chrpc_f64_array_value_va(1.0, 3.2, 33.2),
        new_chrpc_f64_array_value_va(1.0, 3.2, 33.2)
    );

    test_assert_neq_chrpc_value(
        new_chrpc_f64_array_value_va(10, 3.2, 33.2),
        new_chrpc_f64_array_value_va(1.0, 3.2, 33.2)
    );

    test_assert_neq_chrpc_value(
        new_chrpc_struct_value_va(
            new_chrpc_str_value("Bobby")
        ),
        new_chrpc_struct_value_va(
            new_chrpc_str_value("Bobby"),
            new_chrpc_i64_value(-123)
        )
    );

    test_assert_neq_chrpc_value(
        new_chrpc_struct_value_va(
            new_chrpc_str_value("Bobby"),
            new_chrpc_i64_value(123)
        ),
        new_chrpc_struct_value_va(
            new_chrpc_str_value("Bobby"),
            new_chrpc_i64_value(-123)
        )
    );

    // Finally arrays.
    test_assert_eq_chrpc_value(
        new_chrpc_i16_array_value_va(1, 2, 3),
        new_chrpc_i16_array_value_va(1, 2, 3)
    );

    test_assert_neq_chrpc_value(
        new_chrpc_i16_array_value_va(1, 3),
        new_chrpc_i16_array_value_va(1, 2, 3)
    );

    test_assert_neq_chrpc_value(
        new_chrpc_i16_array_value_va(1, 4, 3),
        new_chrpc_i16_array_value_va(1, 2, 3)
    );

    test_assert_eq_chrpc_value(
        new_chrpc_composite_nempty_array_value_va(
            new_chrpc_b8_array_value_va(1, 2, 3),
            new_chrpc_b8_array_value_va(4, 5, 6)
        ),
        new_chrpc_composite_nempty_array_value_va(
            new_chrpc_b8_array_value_va(1, 2, 3),
            new_chrpc_b8_array_value_va(4, 5, 6)
        )
    );

    test_assert_neq_chrpc_value(
        new_chrpc_composite_nempty_array_value_va(
            new_chrpc_b8_array_value_va(1, 4, 3),
            new_chrpc_b8_array_value_va(4, 5, 6)
        ),
        new_chrpc_composite_nempty_array_value_va(
            new_chrpc_b8_array_value_va(1, 2, 3),
            new_chrpc_b8_array_value_va(4, 5, 6)
        )
    );
}

static void test_chrpc_inner_value_to_and_from_buffer(void) {
    typedef struct {
        chrpc_value_t *val;

        uint8_t buf[TEST_CHRPC_TEST_BUFFER_LEN];
        size_t buf_len;
    } test_case_t;

    test_case_t CASES[] = {
        {
            .val = new_chrpc_b8_value(1),
            .buf = { 1 },
            .buf_len = 1 
        },
        {
            .val = new_chrpc_u32_value(256),
            .buf = { 0x0, 0x1, 0x0, 0x0 },
            .buf_len = 4 
        },
        {
            .val = new_chrpc_i64_value(-2),
            .buf = { 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
            .buf_len = 8 
        },
        {
            .val = new_chrpc_f32_value(23.2f),
            .buf = { 0x9A, 0x99, 0xB9, 0x41 },
            .buf_len = 4 
        },
        {
            .val = new_chrpc_i16_array_value_va(1, 3, -1),
            .buf = { 3, 0x0, 0x0, 0x0, 1, 0x0, 3, 0x0, 0xFF, 0xFF },
            .buf_len = 10 
        },
        {
            .val = new_chrpc_f64_array_value_va(12.2, 1.0),
            .buf = { 
                2, 0, 0, 0,
                
                0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x28, 0x40,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x3F
            },
            .buf_len = 20
        },
        {
            .val = new_chrpc_str_value("Hello"),
            .buf = { 6, 0, 0, 0, 'H', 'e', 'l', 'l', 'o', 0 },
            .buf_len = 10 
        },
        {
            .val = new_chrpc_str_array_value_va(
                "Hey", "Bye", "Yo"
            ),
            .buf = { 
                3, 0, 0, 0, 
                4, 0, 0, 0, 'H', 'e', 'y',  0,
                4, 0, 0, 0, 'B', 'y', 'e',  0,
                3, 0, 0, 0, 'Y', 'o',  0,
            },
            .buf_len = 27
        },
        {
            .val = new_chrpc_struct_value_va(
                new_chrpc_str_value("Di"),
                new_chrpc_u16_value(100)
            ),
            .buf = { 3, 0, 0, 0, 'D', 'i', 0, 100, 0x0 },
            .buf_len = 9 
        },
        {
            .val = new_chrpc_i16_empty_array_value(),
            .buf = { 0, 0, 0, 0 },
            .buf_len = 4 
        },
        {
            .val = new_chrpc_composite_empty_array_value(
                new_chrpc_struct_type(CHRPC_BYTE_T)
            ),
            .buf = { 0, 0, 0, 0 },
            .buf_len = 4 
        },
        {
            .val = new_chrpc_composite_nempty_array_value_va(
                new_chrpc_struct_value_va(
                    new_chrpc_b8_value(1),
                    new_chrpc_b8_value(2)
                ),
                new_chrpc_struct_value_va(
                    new_chrpc_b8_value(3),
                    new_chrpc_b8_value(4)
                ) 
            ),
            .buf = { 
                2, 0, 0, 0,
                1, 2, 3, 4
            },
            .buf_len = 8 
        },
        {
            .val = new_chrpc_composite_nempty_array_value_va(
                new_chrpc_struct_value_va(
                    new_chrpc_str_value("Bobby"),
                    new_chrpc_str_value("Flay"),
                    new_chrpc_b8_array_value_va(1, 2, 3)
                ),
                new_chrpc_struct_value_va(
                    new_chrpc_str_value("Mark"),
                    new_chrpc_str_value("David"),
                    new_chrpc_b8_array_value_va(1, 1, 1)
                ),
                new_chrpc_struct_value_va(
                    new_chrpc_str_value("Alyssa"),
                    new_chrpc_str_value("Smith"),
                    new_chrpc_b8_array_value_va(4, 5)
                ),
                new_chrpc_struct_value_va(
                    new_chrpc_str_value("Larry"),
                    new_chrpc_str_value("Alm"),
                    new_chrpc_b8_array_value_va(0)
                )
            ),
            .buf = {
                4, 0, 0, 0, // 4

                6, 0, 0, 0, 'B', 'o', 'b', 'b', 'y', 0, // 10
                5, 0, 0, 0, 'F', 'l', 'a', 'y', 0, // 9
                3, 0, 0, 0, 1, 2, 3, // 7

                5, 0, 0, 0, 'M', 'a', 'r', 'k', 0, // 9
                6, 0, 0, 0, 'D', 'a', 'v', 'i', 'd', 0, // 10
                3, 0, 0, 0, 1, 1, 1, // 7

                7, 0, 0, 0, 'A', 'l', 'y', 's', 's', 'a', 0, // 11
                6, 0, 0, 0, 'S', 'm', 'i', 't', 'h', 0, // 10
                2, 0, 0, 0, 4, 5, // 6

                6, 0, 0, 0, 'L', 'a', 'r', 'r', 'y', 0, // 10
                4, 0, 0, 0, 'A', 'l', 'm', 0, // 8
                1, 0, 0, 0, 0 // 5
                
                // total = 106
            },
            .buf_len = 106
        }
    };
    size_t num_cases = sizeof(CASES) / sizeof(test_case_t);

    for (size_t i = 0; i < num_cases; i++) {
        chrpc_status_t status;

        test_case_t c = CASES[i];

        // First in the to direction.

        size_t written;
        uint8_t act_buf[TEST_CHRPC_TEST_BUFFER_LEN];

        status = chrpc_inner_value_to_buffer(c.val->type, c.val->value, 
                    act_buf, TEST_CHRPC_TEST_BUFFER_LEN, &written);

        TEST_ASSERT_TRUE(status == CHRPC_SUCCESS);
        TEST_ASSERT_EQUAL_size_t(c.buf_len, written);
        TEST_ASSERT_EQUAL_UINT8_ARRAY(c.buf, act_buf, c.buf_len);

        // Next in the from direction.
        
        size_t readden;
        chrpc_inner_value_t *iv;

        status = chrpc_inner_value_from_buffer(c.val->type, &iv, c.buf, c.buf_len, &readden);

        TEST_ASSERT_TRUE(status == CHRPC_SUCCESS);
        TEST_ASSERT_EQUAL_size_t(c.buf_len, readden);

        chrpc_value_t v = {
            .type = c.val->type, // Remeber this is owned by c.val
                                 // Only allowing it here to use the equals function below.
            .value = iv
        };

        TEST_ASSERT_TRUE(chrpc_value_equals(c.val, &v));
        delete_chrpc_inner_value(c.val->type, iv);

        // Cleanup at the end.
        delete_chrpc_value(c.val);
    }
}

static void test_chrpc_inner_value_to_buffer_failures(void) {
    // When writing to the buffer, the only possible failure is that the buffer
    // is too short. 
    typedef struct {
        chrpc_value_t *v; 

        size_t short_buf_len;
    } test_case_t;

    test_case_t CASES[] = {
        {
            .v = new_chrpc_b8_value(48),
            .short_buf_len = 0
        },
        {
            .v = new_chrpc_i16_value(48),
            .short_buf_len = 1 
        },
        {
            .v = new_chrpc_b8_array_value_va(0),
            .short_buf_len = 2
        },
        {
            .v = new_chrpc_f32_array_value_va(1.0f),
            .short_buf_len = 7
        },
        {
            .v = new_chrpc_b8_array_value_va(0),
            .short_buf_len = 4
        },
        {
            .v = new_chrpc_struct_value_va(
                new_chrpc_i32_value(3),
                new_chrpc_i32_value(4)
            ),
            .short_buf_len = 7
        },
        {
            .v = new_chrpc_struct_value_va(
                new_chrpc_i32_value(3),
                new_chrpc_str_value("Hello") // 4 + 6 = 10
            ),
            .short_buf_len = 10
        },
        {
            .v = new_chrpc_composite_nempty_array_value_va(
                new_chrpc_b8_array_value_va(1, 2, 3), // 7
                new_chrpc_b8_array_value_va(0),       // 5
                new_chrpc_b8_empty_array_value()      // 4
            ),
            .short_buf_len = 15
        }
    };

    size_t num_cases = sizeof(CASES) / sizeof(test_case_t);

    for (size_t i = 0; i < num_cases; i++) {
        test_case_t c = CASES[i];

        size_t written;
        uint8_t *tmp_buf = (uint8_t *)safe_malloc(c.short_buf_len);

        chrpc_status_t bad_status =
            chrpc_inner_value_to_buffer(c.v->type, c.v->value, tmp_buf, c.short_buf_len, &written);

        TEST_ASSERT(bad_status == CHRPC_BUFFER_TOO_SMALL);

        safe_free(tmp_buf);
        delete_chrpc_value(c.v);
    }
}

static void test_chrpc_inner_value_from_buffer_failures(void) {
    // All these cases will be buffers which are cut short.
    // All cases should return unexpected end.
    typedef struct {
        chrpc_type_t *exp_type;
        uint8_t buf[TEST_CHRPC_TEST_BUFFER_LEN];
        size_t buf_len;
    } test_case_t;

    test_case_t CASES[] = {
        {
            .exp_type = CHRPC_INT16_T,
            .buf = {
                3
            },
            .buf_len = 1
        },
        {
            .exp_type = CHRPC_INT32_T,
            .buf = {
                3, 3, 1
            },
            .buf_len = 3
        },
        {
            .exp_type = CHRPC_FLOAT64_T,
            .buf = {
                0, 0, 0, 0, 0, 0, 0
            },
            .buf_len = 7
        },
        {
            .exp_type = CHRPC_STRING_T,
            .buf = {
                1, 0, 0
            },
            .buf_len = 3
        },
        {
            .exp_type = CHRPC_STRING_T,
            .buf = {
                1, 0, 0, 0
            },
            .buf_len = 4
        },
        {
            .exp_type = CHRPC_STRING_T,
            .buf = {
                3, 0, 0, 0, 'a', 'a'
            },
            .buf_len = 6
        },
        {
            .exp_type = new_chrpc_array_type(CHRPC_INT16_T),
            .buf = {
                2, 0, 0, 0, 1, 0
            },
            .buf_len = 6
        },
        {
            .exp_type = new_chrpc_array_type(new_chrpc_array_type(CHRPC_BYTE_T)),
            .buf = {
                3, 0, 0, 0,

                2, 0, 0, 0, 1, 2,
                3, 0, 0, 0, 1, 2, 3,
                1, 0

            },
            .buf_len = 19
        },
        {
            .exp_type = new_chrpc_struct_type(CHRPC_BYTE_T, CHRPC_INT16_T),
            .buf = {
                3, 0
            },
            .buf_len = 2
        },
        {
            .exp_type = new_chrpc_array_type(new_chrpc_struct_type(CHRPC_STRING_T, CHRPC_BYTE_T)),
            .buf = {
                2, 0, 0, 0,

                2, 0, 0, 0, 'b', 0, 
                1,

                4, 0, 0
            },
            .buf_len = 14
        }
    };

    size_t num_cases = sizeof(CASES) / sizeof(test_case_t);

    for (size_t i = 0; i < num_cases; i++) {
        test_case_t c = CASES[i];

        size_t readden;
        chrpc_inner_value_t *iv;
        chrpc_status_t status = 
            chrpc_inner_value_from_buffer(c.exp_type, &iv, c.buf, c.buf_len, &readden);

        TEST_ASSERT_TRUE(status == CHRPC_UNEXPECTED_END);
        delete_chrpc_type(c.exp_type);
    }
}

// value to/from buffer, is really just a wrapper around functions which have already been tested.

static void test_chrpc_value_to_buffer(void) {
    chrpc_status_t status;

    uint8_t buf[TEST_CHRPC_TEST_BUFFER_LEN];

    chrpc_value_t *v = new_chrpc_struct_value_va(
        new_chrpc_str_value("Dale"),
        new_chrpc_u16_value(23)
    );

    size_t written;
    status = chrpc_value_to_buffer(v, buf, TEST_CHRPC_TEST_BUFFER_LEN, &written);

    uint8_t exp_buf[] = {
        CHRPC_STRUCT_TID, 2, CHRPC_STRING_TID, CHRPC_UINT16_TID,

        5, 0, 0, 0, 'D', 'a', 'l', 'e', 0,
        23, 0
    };
    size_t exp_len = sizeof(exp_buf) / sizeof(uint8_t);

    TEST_ASSERT_TRUE(status == CHRPC_SUCCESS);
    TEST_ASSERT_EQUAL_size_t(exp_len, written);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(exp_buf, buf, exp_len);

    delete_chrpc_value(v);
}

static void test_chrpc_value_from_buffer(void) {
    chrpc_status_t status; 

    uint8_t valid_buf[] = {
        CHRPC_ARRAY_TID, CHRPC_INT16_TID,

        2, 0, 0, 0, 
        1, 0, 2, 0
    };
    size_t valid_buf_len = sizeof(valid_buf) / sizeof(uint8_t);

    chrpc_value_t *v;
    size_t readden;

    status = chrpc_value_from_buffer(&v, valid_buf, valid_buf_len, &readden);
    TEST_ASSERT_TRUE(status == CHRPC_SUCCESS);
    TEST_ASSERT_EQUAL_size_t(valid_buf_len, readden);

    chrpc_value_t *exp_val = new_chrpc_i16_array_value_va(1, 2);
    TEST_ASSERT_TRUE(chrpc_value_equals(exp_val, v));
    delete_chrpc_value(exp_val);

    delete_chrpc_value(v);

    uint8_t invalid_buf0[] = {
        CHRPC_ARRAY_TID, 123 
    };
    size_t invalid_buf0_len = sizeof(invalid_buf0) / sizeof(uint8_t);
    status = chrpc_value_from_buffer(&v, invalid_buf0, invalid_buf0_len, &readden);
    TEST_ASSERT_TRUE(status != CHRPC_SUCCESS);

    uint8_t invalid_buf1[] = {
        CHRPC_STRUCT_TID, 2, CHRPC_INT16_TID, CHRPC_INT32_TID,

        1, 0, 
        1, 0, 0
    };
    size_t invalid_buf1_len = sizeof(invalid_buf1) / sizeof(uint8_t);
    status = chrpc_value_from_buffer(&v, invalid_buf1, invalid_buf1_len, &readden);
    TEST_ASSERT_TRUE(status != CHRPC_SUCCESS);
}

static void test_chrpc_value_to_buffer_with_length(void) {
    chrpc_status_t status;
    size_t written;

    uint8_t buf[TEST_CHRPC_TEST_BUFFER_LEN]; 

    chrpc_value_t *val = NULL;
    status = chrpc_value_to_buffer_with_length(val, buf, TEST_CHRPC_TEST_BUFFER_LEN, &written); 
    TEST_ASSERT_TRUE(status == CHRPC_SUCCESS);
    TEST_ASSERT_EQUAL_size_t(sizeof(uint32_t), written);
    TEST_ASSERT_EQUAL_UINT32(0, *(uint32_t *)buf);

    status = chrpc_value_to_buffer_with_length(val, buf, sizeof(uint32_t) - 1, &written);
    TEST_ASSERT_TRUE(status != CHRPC_SUCCESS);

    val = new_chrpc_str_value("Hello"); // (Type)(1) + (Value)(4 + 5 + 1)
    status = chrpc_value_to_buffer_with_length(val, buf, TEST_CHRPC_TEST_BUFFER_LEN, &written);
    TEST_ASSERT_TRUE(status == CHRPC_SUCCESS);
    TEST_ASSERT_EQUAL_size_t(sizeof(uint32_t) + 1 + sizeof(uint32_t) + 5 + 1, written);
    TEST_ASSERT_EQUAL_size_t(11, *(uint32_t *)buf);
    TEST_ASSERT_EQUAL_STRING("Hello", (char *)(buf + sizeof(uint32_t) + 1 + sizeof(uint32_t)));

    status = chrpc_value_to_buffer_with_length(val, buf, 8, &written);
    TEST_ASSERT_TRUE(status != CHRPC_SUCCESS);

    status = chrpc_value_to_buffer_with_length(val, buf, sizeof(uint32_t) - 1, &written);
    TEST_ASSERT_TRUE(status != CHRPC_SUCCESS);

    delete_chrpc_value(val);
}

void chrpc_serial_value_tests(void) {
    RUN_TEST(test_chrpc_value_simple_constructors);
    RUN_TEST(test_chrpc_value_simple_array_constructors);
    RUN_TEST(test_chrpc_value_str_constructors);
    RUN_TEST(test_chrpc_struct_value);
    RUN_TEST(test_chrpc_composite_array);
    RUN_TEST(test_chrpc_big_composite_value);
    RUN_TEST(test_chrpc_value_equals);
    RUN_TEST(test_chrpc_inner_value_to_and_from_buffer);
    RUN_TEST(test_chrpc_inner_value_to_buffer_failures);
    RUN_TEST(test_chrpc_inner_value_from_buffer_failures);
    RUN_TEST(test_chrpc_value_to_buffer);
    RUN_TEST(test_chrpc_value_from_buffer);
    RUN_TEST(test_chrpc_value_to_buffer_with_length);
}
