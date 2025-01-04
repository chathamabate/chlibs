
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
}

static void test_chrpc_value_simple_array_constructors(void) {
    chrpc_value_t *cv;

    cv = new_chrpc_b8_array_value_va(1, 2, 3);
    uint8_t b8_arr[] = {1, 2, 3};
    TEST_ASSERT_EQUAL_UINT8_ARRAY(b8_arr, cv->value->b8_arr, 3);
    delete_chrpc_value(cv);

    cv = new_chrpc_i32_array_value_va(-1, 212, -3443);
    int32_t i32_arr[] = {-1, 212, -3443};
    TEST_ASSERT_EQUAL_UINT8_ARRAY(i32_arr, cv->value->i32_arr, 3);
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

static void test_chrpc_inner_value_to_buffer(void) {
    typedef struct {
        chrpc_value_t *val;

        uint8_t exp_buf[TEST_CHRPC_TEST_BUFFER_LEN];
        size_t exp_len;
    } test_case_t;

    test_case_t CASES[] = {
        {
            .val = new_chrpc_b8_value(1),
            .exp_buf = { 1 },
            .exp_len = 1 
        },
        {
            .val = new_chrpc_u32_value(256),
            .exp_buf = { 0x0, 0x1, 0x0, 0x0 },
            .exp_len = 4 
        },
        {
            .val = new_chrpc_i64_value(-2),
            .exp_buf = { 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
            .exp_len = 8 
        },
        {
            .val = new_chrpc_i16_array_value_va(1, 3, -1),
            .exp_buf = { 3, 0x0, 0x0, 0x0, 1, 0x0, 3, 0x0, 0xFF, 0xFF },
            .exp_len = 10 
        },
        {
            .val = new_chrpc_str_value("Hello"),
            .exp_buf = { 6, 0, 0, 0, 'H', 'e', 'l', 'l', 'o', 0 },
            .exp_len = 10 
        },
        {
            .val = new_chrpc_str_array_value_va(
                "Hey", "Bye", "Yo"
            ),
            .exp_buf = { 
                3, 0, 0, 0, 
                4, 0, 0, 0, 'H', 'e', 'y',  0,
                4, 0, 0, 0, 'B', 'y', 'e',  0,
                3, 0, 0, 0, 'Y', 'o',  0,
            },
            .exp_len = 27
        },
        {
            .val = new_chrpc_struct_value_va(
                new_chrpc_str_value("Di"),
                new_chrpc_u16_value(100)
            ),
            .exp_buf = { 3, 0, 0, 0, 'D', 'i', 0, 100, 0x0 },
            .exp_len = 9 
        },
        {
            .val = new_chrpc_i16_empty_array_value(),
            .exp_buf = { 0, 0, 0, 0 },
            .exp_len = 4 
        },
        {
            .val = new_chrpc_composite_empty_array_value(
                new_chrpc_struct_type(CHRPC_BYTE_T)
            ),
            .exp_buf = { 0, 0, 0, 0 },
            .exp_len = 4 
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
            .exp_buf = { 
                2, 0, 0, 0,
                1, 2, 3, 4
            },
            .exp_len = 8 
        },

    };
    size_t num_cases = sizeof(CASES) / sizeof(test_case_t);

    for (size_t i = 0; i < num_cases; i++) {
        test_case_t c = CASES[i];

        size_t written;
        uint8_t act_buf[TEST_CHRPC_TEST_BUFFER_LEN];

        chrpc_status_t status = 
            chrpc_inner_value_to_buffer(c.val->type, c.val->value, 
                    act_buf, TEST_CHRPC_TEST_BUFFER_LEN, &written);

        TEST_ASSERT_TRUE(status == CHRPC_SUCCESS);
        TEST_ASSERT_EQUAL_size_t(c.exp_len, written);
        TEST_ASSERT_EQUAL_UINT8_ARRAY(c.exp_buf, act_buf, c.exp_len);

        // Cleanup at the end.
        delete_chrpc_value(c.val);
    }
}

static void test_chrpc_big_inner_value_to_buffer(void) {
    chrpc_value_t *v = new_chrpc_composite_nempty_array_value_va(
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
    );

    const uint8_t exp_buf[] = {
        4, 0, 0, 0,

        6, 0, 0, 0, 'B', 'o', 'b', 'b', 'y', 0,
        5, 0, 0, 0, 'F', 'l', 'a', 'y', 0,
        3, 0, 0, 0, 1, 2, 3,

        5, 0, 0, 0, 'M', 'a', 'r', 'k', 0,
        6, 0, 0, 0, 'D', 'a', 'v', 'i', 'd', 0,
        3, 0, 0, 0, 1, 1, 1,

        7, 0, 0, 0, 'A', 'l', 'y', 's', 's', 'a', 0,
        6, 0, 0, 0, 'S', 'm', 'i', 't', 'h', 0,
        2, 0, 0, 0, 4, 5,

        6, 0, 0, 0, 'L', 'a', 'r', 'r', 'y', 0,
        4, 0, 0, 0, 'A', 'l', 'm', 0,
        1, 0, 0, 0, 0
    };
    size_t exp_len = sizeof(exp_buf) / sizeof(uint8_t);

    uint8_t act_buf[TEST_CHRPC_TEST_BUFFER_LEN];
    size_t written;

    chrpc_status_t status = 
        chrpc_inner_value_to_buffer(v->type, v->value, act_buf, TEST_CHRPC_TEST_BUFFER_LEN, &written);

    TEST_ASSERT_TRUE(status == CHRPC_SUCCESS);
    TEST_ASSERT_EQUAL_size_t(exp_len, written);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(exp_buf, act_buf, exp_len);

    delete_chrpc_value(v);
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

static void test_chrpc_inner_value_from_buffer(void) {
    typedef struct {
        uint8_t act_buf[TEST_CHRPC_TEST_BUFFER_LEN];
        size_t act_buf_len;

        chrpc_value_t *exp_value;
    } test_case_t;

    test_case_t CASES[] = {
        {
            .act_buf = {
                1
            },
            .act_buf_len = 1,
            .exp_value = new_chrpc_b8_value(1)
        }
    };
    size_t num_cases = sizeof(CASES) / sizeof(test_case_t);

    for (size_t i = 0; i < num_cases; i++) {
        test_case_t c = CASES[i];

        chrpc_type_t *ct = new_chrpc_type_copy(c.exp_value->type);

        size_t readden;
        chrpc_inner_value_t *iv;
        
        chrpc_status_t status = 
            chrpc_inner_value_from_buffer(ct, &iv, c.act_buf, c.act_buf_len, &readden);

        TEST_ASSERT_TRUE(status == CHRPC_SUCCESS);
        TEST_ASSERT_EQUAL_size_t(c.act_buf_len, readden);

        // We construct our own value pair here just so we can call equals below.
        chrpc_value_t v = {
            .type = ct,
            .value = iv
        };

        TEST_ASSERT_TRUE(chrpc_value_equals(c.exp_value, &v));

        // Here we delete the type and value one at a time.
        delete_chrpc_inner_value(v.type, v.value);
        delete_chrpc_type(v.type);

        delete_chrpc_value(c.exp_value);
    }

    
}

void chrpc_serial_value_tests(void) {
    RUN_TEST(test_chrpc_value_simple_constructors);
    RUN_TEST(test_chrpc_value_simple_array_constructors);
    RUN_TEST(test_chrpc_value_str_constructors);
    RUN_TEST(test_chrpc_struct_value);
    RUN_TEST(test_chrpc_composite_array);
    RUN_TEST(test_chrpc_big_composite_value);
    RUN_TEST(test_chrpc_value_equals);
    RUN_TEST(test_chrpc_inner_value_to_buffer);
    RUN_TEST(test_chrpc_big_inner_value_to_buffer);
    RUN_TEST(test_chrpc_inner_value_to_buffer_failures);
    RUN_TEST(test_chrpc_inner_value_from_buffer);
}
