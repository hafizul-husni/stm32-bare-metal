/*
 * Day 1: prove the startup code works.
 *
 * boot_magic   lives in .data -> checks that Reset_Handler copied .data from FLASH to RAM
 * loop_counter lives in .bss  -> checks that Reset_Handler zeroed .bss
 *                                and that main() is running (it keeps increasing)
 * startup_ok   is set to 1 only if both checks pass
 */
#include <stdint.h>
#include "uart.h"

#define BOOT_MAGIC 0xC0FFEEu

volatile uint32_t boot_magic = BOOT_MAGIC;
volatile uint32_t loop_counter;
volatile uint32_t startup_ok;

int main(void)
{
    if (boot_magic == BOOT_MAGIC && loop_counter == 0) {
        startup_ok = 1;
    }

    uart_init();
    uart_puts("Hello from bare-metal STM32!\r\n");

    while (1) {
        loop_counter++;
    }
}
