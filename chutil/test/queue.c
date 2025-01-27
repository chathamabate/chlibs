
#include "./queue.h"
#include "chutil/queue.h"

#include "unity/unity.h"
#include "unity/unity_internals.h"

static void test_queue_simple1(void) {
    queue_t *q = new_queue(3, sizeof(int));

    int s;
    int d;

    TEST_ASSERT_EQUAL_INT(1, q_poll(q, &d));

    s = 2;
    TEST_ASSERT_EQUAL_INT(0, q_push(q, &s));

    TEST_ASSERT_EQUAL_INT(0, q_poll(q, &d));
    TEST_ASSERT_EQUAL_INT(2, d);

    delete_queue(q);
}

static void test_queue_simple2(void) {
    typedef struct {
        int64_t x;
        int64_t y;
    } coord_t;

    size_t cap = 10;
    queue_t *q = new_queue(cap, sizeof(coord_t));

    TEST_ASSERT_EQUAL_size_t(cap, q_cap(q));
    TEST_ASSERT_EQUAL_size_t(0, q_len(q));

    for (size_t i = 0; i < cap; i++) {
        coord_t c = {
            .x = i,
            .y = i * 2
        };

        TEST_ASSERT_EQUAL_INT(0, q_push(q, &c));
        TEST_ASSERT_EQUAL_size_t(i + 1, q_len(q));
    }

    coord_t tc;
    TEST_ASSERT_EQUAL_INT(1, q_push(q, &tc));
    TEST_ASSERT_TRUE(q_full(q));

    for (size_t i = 0; i < cap; i++) {
        coord_t d;

        TEST_ASSERT_EQUAL_INT(0, q_poll(q, &d));
        TEST_ASSERT_EQUAL_INT64(i, d.x);
        TEST_ASSERT_EQUAL_INT64(i * 2, d.y);

        TEST_ASSERT_EQUAL_size_t(cap - i - 1, q_len(q));
    }

    delete_queue(q);
}

static void test_queue_simple3(void) {
    const size_t cap = 10;
    queue_t *q = new_queue(cap, sizeof(uint8_t));

    const uint8_t group_size = 3;

    for (size_t try = 0; try < 100; try++) {
        for (uint8_t i = 0; i < group_size; i++) {
            uint8_t s = i + 3;
            TEST_ASSERT_EQUAL_INT(0, q_push(q, &s));
        }

        TEST_ASSERT_EQUAL_size_t(group_size, q_len(q));

        for (uint8_t i = 0; i < group_size; i++) {
            uint8_t d;
            TEST_ASSERT_EQUAL_INT(0, q_poll(q, &d));
            TEST_ASSERT_EQUAL_UINT8(i + 3, d);
        }

        TEST_ASSERT_EQUAL_size_t(0, q_len(q));
    }

    delete_queue(q);
}

void queue_tests(void) {
    RUN_TEST(test_queue_simple1);
    RUN_TEST(test_queue_simple2);
    RUN_TEST(test_queue_simple3);
}
