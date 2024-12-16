

#include "serial_type.h"

#include "chrpc/serial_type.h"
#include "unity/unity.h"
#include "chsys/mem.h"

#define TEST_CHRPC_TEST_BUFFER_LEN 256

static void test_chrpc_type_new_and_delete(void) {
    chrpc_type_t *ct;

    ct = new_chrpc_primitive_type_from_id(CHRPC_INT32_TID);
    delete_chrpc_type(ct);

    ct = new_chrpc_array_type(
            new_chrpc_primitive_type_from_id(CHRPC_INT32_TID));
    delete_chrpc_type(ct);

    ct = new_chrpc_array_type(
             new_chrpc_array_type(
                 CHRPC_BYTE_T));
    delete_chrpc_type(ct);

    ct = new_chrpc_struct_type(
        CHRPC_INT16_T
    );
    delete_chrpc_type(ct);

    ct = new_chrpc_struct_type(
        CHRPC_INT16_T,
        CHRPC_STRING_T,
        CHRPC_BYTE_T
    );
    delete_chrpc_type(ct);

    ct = new_chrpc_struct_type(
        CHRPC_STRING_T,
        new_chrpc_array_type(
            new_chrpc_struct_type(
                CHRPC_STRING_T,
                CHRPC_INT64_T
            )
        )
    );
    delete_chrpc_type(ct);
}

static void test_chrpc_type_equals(void) {
    typedef struct {
        chrpc_type_t *ct1;
        chrpc_type_t *ct2;
        bool should_equate;
    } test_case_t;

    test_case_t cases[] = {
        {
            .ct1 = CHRPC_STRING_T,
            .ct2 = CHRPC_STRING_T,
            .should_equate = true
        },
        {
            .ct1 = CHRPC_UINT16_T,
            .ct2 = CHRPC_STRING_T,
            .should_equate = false 
        },
        {
            .ct1 = new_chrpc_array_type(CHRPC_STRING_T),
            .ct2 = CHRPC_STRING_T,
            .should_equate = false 
        },
        {
            .ct1 = new_chrpc_array_type(CHRPC_STRING_T),
            .ct2 = new_chrpc_array_type(CHRPC_STRING_T),
            .should_equate = true
        },
        {
            .ct1 = new_chrpc_array_type(CHRPC_STRING_T),
            .ct2 = new_chrpc_array_type(CHRPC_UINT16_T),
            .should_equate = false
        },
        {
            .ct1 = new_chrpc_array_type(CHRPC_STRING_T),
            .ct2 = new_chrpc_array_type(CHRPC_UINT16_T),
            .should_equate = false
        },
        {
            .ct1 = new_chrpc_struct_type(CHRPC_UINT16_T),
            .ct2 = new_chrpc_struct_type(CHRPC_UINT16_T),
            .should_equate = true
        },
        {
            .ct1 = new_chrpc_struct_type(CHRPC_UINT16_T, CHRPC_UINT64_T),
            .ct2 = new_chrpc_struct_type(CHRPC_UINT16_T),
            .should_equate = false
        },
        {
            .ct1 = new_chrpc_struct_type(CHRPC_UINT16_T, CHRPC_UINT64_T),
            .ct2 = new_chrpc_struct_type(CHRPC_UINT16_T, CHRPC_UINT32_T),
            .should_equate = false
        },
        {
            .ct1 = new_chrpc_struct_type(CHRPC_UINT16_T, CHRPC_UINT64_T),
            .ct2 = new_chrpc_struct_type(CHRPC_UINT16_T, CHRPC_UINT64_T),
            .should_equate = true
        }
    };
    size_t num_cases = sizeof(cases) / sizeof(cases[0]);

    for (size_t i = 0; i < num_cases; i++) {
        TEST_ASSERT_TRUE(cases[i].should_equate ==
                chrpc_type_equals(cases[i].ct1, cases[i].ct2));
    }

    // Cleanup.
    for (size_t i = 0; i < num_cases; i++) {
        delete_chrpc_type(cases[i].ct1);
        delete_chrpc_type(cases[i].ct2);
    }
}

static void test_chrpc_type_to_and_from_buffer(void) {
    typedef struct {
        uint8_t serial[TEST_CHRPC_TEST_BUFFER_LEN];
        size_t serial_len;

        chrpc_type_t *ct;
    } test_case_t;

    test_case_t cases[] = {
        {
            .serial = {
                CHRPC_BYTE_TID
            },
            .serial_len = 1,
            .ct = CHRPC_BYTE_T
        },
        {
            .serial = {
                CHRPC_ARRAY_TID,
                CHRPC_INT16_TID
            },
            .serial_len = 2,
            .ct = new_chrpc_array_type(CHRPC_INT16_T)
        },
        {
            .serial = {
                CHRPC_STRUCT_TID,
                2,
                CHRPC_BYTE_TID,
                CHRPC_STRING_TID
            },
            .serial_len = 4,
            .ct = new_chrpc_struct_type(
                CHRPC_BYTE_T,
                CHRPC_STRING_T
            )
        },
        {
            .serial = {
                CHRPC_STRUCT_TID,
                2,
                CHRPC_STRING_TID,
                CHRPC_STRUCT_TID,
                2,
                CHRPC_BYTE_TID,
                CHRPC_ARRAY_TID,
                CHRPC_STRING_TID
            },
            .serial_len = 8,
            .ct = new_chrpc_struct_type(
                CHRPC_STRING_T, 
                new_chrpc_struct_type(
                    CHRPC_BYTE_T,
                    new_chrpc_array_type(CHRPC_STRING_T)
                )
            )
        },
        {
            .serial = {
                CHRPC_STRUCT_TID,
                3,
                CHRPC_UINT32_TID,
                CHRPC_ARRAY_TID,
                CHRPC_STRUCT_TID,
                2,
                CHRPC_STRING_TID,
                CHRPC_STRING_TID,
                CHRPC_UINT32_TID
            },
            .serial_len = 9,
            .ct = new_chrpc_struct_type(
                CHRPC_UINT32_T,
                new_chrpc_array_type(
                    new_chrpc_struct_type(
                        CHRPC_STRING_T,
                        CHRPC_STRING_T
                    )
                ),
                CHRPC_UINT32_T
            )
        },
    };

    const size_t num_cases = sizeof(cases) / sizeof(cases[0]);

    uint8_t temp_buf[TEST_CHRPC_TEST_BUFFER_LEN];
    for (size_t i = 0; i < num_cases; i++) {
        test_case_t c = cases[i];

        // First test to_buffer.
        size_t written = 0;
        TEST_ASSERT_EQUAL_INT(CHRPC_SUCCESS, 
                chrpc_type_to_buffer(c.ct, temp_buf, TEST_CHRPC_TEST_BUFFER_LEN, &written));
        TEST_ASSERT_EQUAL_size_t(c.serial_len, written);
        TEST_ASSERT_EQUAL_UINT8_ARRAY(c.serial, temp_buf, written);

        // Now test from buffer.
        chrpc_type_t *ct;
        size_t readden;
        TEST_ASSERT_EQUAL_INT(CHRPC_SUCCESS, 
                chrpc_type_from_buffer(c.serial, c.serial_len, &ct, &readden));
        TEST_ASSERT_EQUAL_size_t(c.serial_len, readden);
        TEST_ASSERT_TRUE(chrpc_type_equals(c.ct, ct));
        
        delete_chrpc_type(ct);
    }

    // Clean up all created test types.
    for (size_t i = 0; i < num_cases; i++) {
        delete_chrpc_type(cases[i].ct);
    }
}

static void test_chrpc_type_to_buffer_failures(void) {
    // Only errors are when the buffer isn't large enough

    chrpc_type_t *ct;
    uint8_t dummy_buf[TEST_CHRPC_TEST_BUFFER_LEN];
    size_t written;

    TEST_ASSERT_EQUAL_INT(CHRPC_BUFFER_TOO_SMALL, 
            chrpc_type_to_buffer(CHRPC_BYTE_T, dummy_buf, 0, &written));

    ct = new_chrpc_array_type(CHRPC_UINT16_T);
    TEST_ASSERT_EQUAL_INT(CHRPC_BUFFER_TOO_SMALL, 
            chrpc_type_to_buffer(ct, dummy_buf, 1, &written));
    delete_chrpc_type(ct);

    ct = new_chrpc_struct_type(
        new_chrpc_struct_type(
            CHRPC_INT64_T,
            CHRPC_STRING_T
        ),
        CHRPC_INT16_T
    );
    TEST_ASSERT_EQUAL_INT(CHRPC_BUFFER_TOO_SMALL, 
            chrpc_type_to_buffer(ct, dummy_buf, 6, &written));
    delete_chrpc_type(ct);
}


static void test_chrpc_type_from_buffer_failures(void)  {
    typedef struct {
        uint8_t buf[TEST_CHRPC_TEST_BUFFER_LEN];
        size_t buf_len;
        chrpc_status_t exp_error;
    } test_case_t;

    test_case_t cases[] = {
        {
            .buf = {
                0
            },
            .buf_len = 0,
            .exp_error = CHRPC_UNEXPECTED_END
        },
        {
            .buf = {
                255 // This test depends on 255 being an unused ID value.
            },
            .buf_len = 1,
            .exp_error = CHRPC_SYNTAX_ERROR
        },
        {
            .buf = {
                CHRPC_ARRAY_TID
            },
            .buf_len = 1,
            .exp_error = CHRPC_UNEXPECTED_END
        },
        {
            .buf = {
                CHRPC_STRUCT_TID,
                0
            },
            .buf_len = 2,
            .exp_error = CHRPC_EMPTY_STRUCT_TYPE
        },
        {
            .buf = {
                CHRPC_STRUCT_TID,
                2,
                CHRPC_BYTE_TID
            },
            .buf_len = 3,
            .exp_error = CHRPC_UNEXPECTED_END
        },
        {
            .buf = {
                CHRPC_STRUCT_TID,
                2,
                CHRPC_STRUCT_TID,
                3,
                CHRPC_BYTE_TID,
                CHRPC_ARRAY_TID,
                CHRPC_STRING_TID
            },
            .buf_len = 7,
            .exp_error = CHRPC_UNEXPECTED_END
        }
    };

    size_t num_cases = sizeof(cases) / sizeof(cases[0]);

    for (size_t i = 0; i < num_cases; i++) {
        test_case_t c = cases[i];

        // dummies.
        chrpc_type_t *ct;
        size_t readden;

        TEST_ASSERT_EQUAL_INT(c.exp_error, 
                chrpc_type_from_buffer(c.buf, c.buf_len, &ct, &readden));
    }
}

void chrpc_serial_type_tests(void) {
    RUN_TEST(test_chrpc_type_new_and_delete);
    RUN_TEST(test_chrpc_type_equals);
    RUN_TEST(test_chrpc_type_to_and_from_buffer);
    RUN_TEST(test_chrpc_type_to_buffer_failures);
    RUN_TEST(test_chrpc_type_from_buffer_failures);
}
