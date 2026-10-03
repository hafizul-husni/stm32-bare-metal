/*
 * Host-native unit tests for the ring buffer (Unity framework).
 * No MCU or Renode needed -- run with `make test`.
 */
#include "unity.h"
#include "ring_buffer.h"

static ring_buffer_t rb;

void setUp(void)
{
    ring_buffer_init(&rb);
}

void tearDown(void)
{
}

void test_init_is_empty_not_full(void)
{
    TEST_ASSERT_TRUE(ring_buffer_is_empty(&rb));
    TEST_ASSERT_FALSE(ring_buffer_is_full(&rb));
    TEST_ASSERT_EQUAL_UINT16(0, ring_buffer_dropped_count(&rb));
}

void test_push_then_pop_single_byte(void)
{
    TEST_ASSERT_TRUE(ring_buffer_push(&rb, 0x42));
    TEST_ASSERT_FALSE(ring_buffer_is_empty(&rb));

    uint8_t byte = 0;
    TEST_ASSERT_TRUE(ring_buffer_pop(&rb, &byte));
    TEST_ASSERT_EQUAL_UINT8(0x42, byte);
    TEST_ASSERT_TRUE(ring_buffer_is_empty(&rb));
}

void test_pop_on_empty_fails(void)
{
    uint8_t byte;
    TEST_ASSERT_FALSE(ring_buffer_pop(&rb, &byte));
}

void test_fifo_order_preserved(void)
{
    for (uint8_t i = 0; i < 10; i++) {
        TEST_ASSERT_TRUE(ring_buffer_push(&rb, i));
    }
    for (uint8_t i = 0; i < 10; i++) {
        uint8_t byte;
        TEST_ASSERT_TRUE(ring_buffer_pop(&rb, &byte));
        TEST_ASSERT_EQUAL_UINT8(i, byte);
    }
}

void test_full_buffer_rejects_push_and_counts_drop(void)
{
    for (int i = 0; i < RING_BUFFER_SIZE; i++) {
        TEST_ASSERT_TRUE(ring_buffer_push(&rb, (uint8_t)i));
    }
    TEST_ASSERT_TRUE(ring_buffer_is_full(&rb));
    TEST_ASSERT_EQUAL_UINT16(0, ring_buffer_dropped_count(&rb));

    TEST_ASSERT_FALSE(ring_buffer_push(&rb, 0xFF));
    TEST_ASSERT_EQUAL_UINT16(1, ring_buffer_dropped_count(&rb));

    TEST_ASSERT_FALSE(ring_buffer_push(&rb, 0xFE));
    TEST_ASSERT_EQUAL_UINT16(2, ring_buffer_dropped_count(&rb));
}

void test_wraparound_across_multiple_cycles(void)
{
    for (int round = 0; round < 3; round++) {
        for (int i = 0; i < RING_BUFFER_SIZE; i++) {
            TEST_ASSERT_TRUE(ring_buffer_push(&rb, (uint8_t)(i + round)));
        }
        for (int i = 0; i < RING_BUFFER_SIZE; i++) {
            uint8_t byte;
            TEST_ASSERT_TRUE(ring_buffer_pop(&rb, &byte));
            TEST_ASSERT_EQUAL_UINT8((uint8_t)(i + round), byte);
        }
    }
    TEST_ASSERT_EQUAL_UINT16(0, ring_buffer_dropped_count(&rb));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_is_empty_not_full);
    RUN_TEST(test_push_then_pop_single_byte);
    RUN_TEST(test_pop_on_empty_fails);
    RUN_TEST(test_fifo_order_preserved);
    RUN_TEST(test_full_buffer_rejects_push_and_counts_drop);
    RUN_TEST(test_wraparound_across_multiple_cycles);
    return UNITY_END();
}
