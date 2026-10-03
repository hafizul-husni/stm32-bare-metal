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
#include "gpio.h"
#include "systick.h"

#define BOOT_MAGIC 0xC0FFEEu

/*
 * Day 4: the STM32F4 Discovery board's green user LED is wired to PD12
 * (also how Renode's stm32f4_discovery.repl models it: "UserLED: ... @
 * gpioPortD" with "12 -> UserLED@0").
 */
#define LED_PORT        GPIO_PORT_D
#define LED_PIN         12u
#define BLINK_PERIOD_MS 500u

volatile uint32_t boot_magic = BOOT_MAGIC;
volatile uint32_t loop_counter;
volatile uint32_t startup_ok;

/*
 * We're freestanding (-ffreestanding, no libc), so there's no printf.
 * This is a minimal decimal printer just for the blink demo's timing
 * output below -- not a general-purpose driver function.
 */
static void uart_put_uint32(uint32_t value)
{
    char digits[10]; /* UINT32_MAX = 4294967295 is 10 digits */
    int n = 0;

    if (value == 0) {
        uart_putc('0');
        return;
    }

    while (value > 0) {
        digits[n++] = (char)('0' + (value % 10));
        value /= 10;
    }

    while (n > 0) {
        uart_putc(digits[--n]);
    }
}

int main(void)
{
    if (boot_magic == BOOT_MAGIC && loop_counter == 0) {
        startup_ok = 1;
    }

    uart_init();
    systick_init();
    gpio_init_output(LED_PORT, LED_PIN);

    uart_puts("Hello from bare-metal STM32!\r\n");

    uint32_t last_toggle = millis();

    while (1) {
        loop_counter++;

        /*
         * Day 4: non-blocking 500 ms LED blink. This checks elapsed time
         * instead of calling a blocking delay(), so the UART RX echo
         * below still gets serviced every single loop iteration -- the
         * LED toggling never stalls the echo, and vice versa.
         */
        if (systick_since(last_toggle) >= BLINK_PERIOD_MS) {
            /* += BLINK_PERIOD_MS, not "= millis()": keeps the 500 ms
             * period exact over time instead of drifting by however
             * long this iteration's loop body took to run. */
            last_toggle += BLINK_PERIOD_MS;
            gpio_toggle(LED_PORT, LED_PIN);

            uart_puts("LED toggle, millis()=");
            uart_put_uint32(millis());
            uart_puts("\r\n");
        }

        /* Day 3: echo bytes received via the USART2 RX interrupt */
        char c;
        if (uart_getc(&c)) {
            uart_putc(c);
        }
    }
}
