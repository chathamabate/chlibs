

#include "serial.h"

#include "chrpc/serial.h"
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
    struct {
        chrpc_type_t *ct1;
        chrpc_type_t *ct2;
        bool should_equate;
    } cases[] = {
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

static void test_chrpc_type_to_buffer_successes(void) {
    struct {
        uint8_t exp[TEST_CHRPC_TEST_BUFFER_LEN];
        size_t exp_len;
        chrpc_type_t *ct;
    } cases[] = {
        {
            .exp = {
                CHRPC_BYTE_TID
            },
            .exp_len = 1,
            .ct = CHRPC_BYTE_T
        },
        {
            .exp = {
                CHRPC_ARRAY_TID,
                CHRPC_INT16_TID
            },
            .exp_len = 2,
            .ct = new_chrpc_array_type(CHRPC_INT16_T)
        },
        {
            .exp = {
                CHRPC_STRUCT_TID,
                2,
                CHRPC_BYTE_TID,
                CHRPC_STRING_TID
            },
            .exp_len = 4,
            .ct = new_chrpc_struct_type(
                CHRPC_BYTE_T,
                CHRPC_STRING_T
            )
        },
        {
            .exp = {
                CHRPC_STRUCT_TID,
                2,
                CHRPC_STRING_TID,
                CHRPC_STRUCT_TID,
                2,
                CHRPC_BYTE_TID,
                CHRPC_ARRAY_TID,
                CHRPC_STRING_TID
            },
            .exp_len = 8,
            .ct = new_chrpc_struct_type(
                CHRPC_STRING_T, 
                new_chrpc_struct_type(
                    CHRPC_BYTE_T,
                    new_chrpc_array_type(CHRPC_STRING_T)
                )
            )
        },
        {
            .exp = {
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
            .exp_len = 9,
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
        }
    };

    const size_t num_cases = sizeof(cases) / sizeof(cases[0]);

    uint8_t act[TEST_CHRPC_TEST_BUFFER_LEN];
    for (size_t i = 0; i < num_cases; i++) {
        size_t written = 0;
        TEST_ASSERT_EQUAL_INT(CHRPC_SUCCESS, 
                chrpc_type_to_buffer(cases[i].ct, act, TEST_CHRPC_TEST_BUFFER_LEN, &written));
        TEST_ASSERT_EQUAL_size_t(cases[i].exp_len, written);
        TEST_ASSERT_EQUAL_UINT8_ARRAY(cases[i].exp, act, written);
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

// NOTE: consider just redoing these tests tbh....
// Shouldn't need a to and a from tbh...

static void test_chrpc_type_from_buffer_successes(void)  {
    /*
    struct {
        uint8_t act_buf[TEST_CHRPC_TEST_BUFFER_LEN];
        size_t act_len;
        chrpc_type_t *exp_type;
    } cases[] = {

    };
    size_t num_cases = sizeof(cases) / sizeof(cases[0]);

    for (size_t i = 0; i < num_cases; i++) {
        size_t readden;
        chrpc_type_t *act_type;
        TEST_ASSERT_EQUAL_INT(CHRPC_SUCCESS, 
                chrpc_type_from_buffer(cases[i].act_buf, cases[i].act_len, 
                    &act_type, &readden));
        TEST_ASSERT_EQUAL_size_t(cases[i].act_len, readden);
        TEST_ASSERT_TRUE(chrpc_type_equals(cases[i].exp_type, act_type));
        
        delete_chrpc_type(act_type);
    }

    // Cleanup.
    for (size_t i = 0; i < num_cases; i++) {
        delete_chrpc_type(cases[i].exp_type);
    }
    */
}

static void test_chrpc_type_from_buffer_failures(void)  {

}

void chrpc_serial_tests(void) {
    RUN_TEST(test_chrpc_type_new_and_delete);
    RUN_TEST(test_chrpc_type_equals);
    RUN_TEST(test_chrpc_type_to_buffer_successes);
    RUN_TEST(test_chrpc_type_to_buffer_failures);
    RUN_TEST(test_chrpc_type_from_buffer_successes);
    RUN_TEST(test_chrpc_type_from_buffer_failures);
}
