/*
 * Host-native unit tests for the SysTick elapsed-time helpers (Unity
 * framework). No MCU or Renode needed -- run with `make test`.
 *
 * These tests link drivers/systick.c directly and call its pure
 * software functions (systick_elapsed, systick_since, millis,
 * SysTick_Handler). They never call systick_init(), so the SysTick
 * hardware registers at 0xE000E0xx are never touched -- safe to run on
 * the host.
 */
#include "unity.h"
#include "systick.h"

void setUp(void)
{
}

void tearDown(void)
{
}

void test_elapsed_zero_when_now_equals_start(void)
{
    TEST_ASSERT_EQUAL_UINT32(0, systick_elapsed(1000, 1000));
}

void test_elapsed_normal_forward_difference(void)
{
    TEST_ASSERT_EQUAL_UINT32(500, systick_elapsed(1500, 1000));
}

void test_elapsed_wraparound_across_overflow(void)
{
    /* ms_ticks wrapped from near UINT32_MAX back around past 0 between
     * start and now. Correct elapsed time is 356 ms, even though
     * `now < start` as plain numbers. */
    uint32_t start = 0xFFFFFF00u;
    uint32_t now   = 0x00000064u;
    TEST_ASSERT_EQUAL_UINT32(356, systick_elapsed(now, start));
}

void test_elapsed_start_at_max_uint32(void)
{
    /* One tick past the maximum value wraps to 0. */
    TEST_ASSERT_EQUAL_UINT32(1, systick_elapsed(0, 0xFFFFFFFFu));
}

void test_since_reflects_ticks_advanced(void)
{
    /* Exercise millis()/SysTick_Handler/systick_since() together,
     * relative to a start point taken right before -- this doesn't
     * assume anything about ms_ticks' absolute value, so it's unaffected
     * by whatever other tests in this binary already advanced it to. */
    uint32_t start = millis();

    for (int i = 0; i < 5; i++) {
        SysTick_Handler();
    }

    TEST_ASSERT_EQUAL_UINT32(5, systick_since(start));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_elapsed_zero_when_now_equals_start);
    RUN_TEST(test_elapsed_normal_forward_difference);
    RUN_TEST(test_elapsed_wraparound_across_overflow);
    RUN_TEST(test_elapsed_start_at_max_uint32);
    RUN_TEST(test_since_reflects_ticks_advanced);
    return UNITY_END();
}
