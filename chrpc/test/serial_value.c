
#include "serial_value.h"
#include "chrpc/serial_type.h"
#include "chrpc/serial_value.h"

#include "unity/unity.h"
#include "unity/unity_internals.h"

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

void chrpc_serial_value_tests(void) {
    RUN_TEST(test_chrpc_value_simple_constructors);
    RUN_TEST(test_chrpc_value_simple_array_constructors);
    RUN_TEST(test_chrpc_value_str_constructors);
    RUN_TEST(test_chrpc_struct_value);
    RUN_TEST(test_chrpc_composite_array);
    RUN_TEST(test_chrpc_big_composite_value);
    RUN_TEST(test_chrpc_value_equals);
}
