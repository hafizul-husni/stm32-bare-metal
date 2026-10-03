/*
 * GPIO driver for STM32F407, register-level, derived from RM0090 (section
 * on GPIO). Works on any port/pin, unlike uart.c's Day 2 GPIOA-only macros.
 */
#include <stddef.h>
#include "gpio.h"

/*
 * Register layout for one GPIO port (RM0090 Table "GPIO register map").
 * Every port (GPIOA..GPIOI) has this same layout, just at a different
 * base address, so one struct describes all of them -- a single pointer
 * cast reaches whichever port we're asked for, instead of needing a
 * separate #define per register per port like GPIOA_MODER/GPIOA_AFRL in
 * uart.c.
 *
 * This is the same technique CMSIS device headers use (a GPIO_TypeDef
 * struct + one #define per port base address). The difference from plain
 * #defines isn't the register addresses -- it's that the port becomes a
 * runtime value (a struct pointer) instead of being baked into each macro
 * name, so the same code works for GPIOA, GPIOD, GPIOG, etc.
 *
 * `volatile` is on each member (not just the pointer) so the compiler
 * never reorders, caches in a register, or elides a read/write to any of
 * these -- each one is a real bus access with a hardware side effect,
 * which the "as-if" rule doesn't know about.
 */
typedef struct {
    volatile uint32_t MODER;   /* 0x00: mode (input/output/AF/analog), 2 bits/pin   */
    volatile uint32_t OTYPER;  /* 0x04: output type (push-pull/open-drain), 1 bit/pin */
    volatile uint32_t OSPEEDR; /* 0x08: output speed, 2 bits/pin                   */
    volatile uint32_t PUPDR;   /* 0x0C: pull-up/pull-down, 2 bits/pin              */
    volatile uint32_t IDR;     /* 0x10: input data (read-only)                     */
    volatile uint32_t ODR;     /* 0x14: output data                                */
    volatile uint32_t BSRR;    /* 0x18: bit set/reset (write-only, atomic)         */
    volatile uint32_t LCKR;    /* 0x1C: configuration lock                         */
    volatile uint32_t AFR[2];  /* 0x20/0x24: alternate function low/high, 4 bits/pin */
} gpio_regs_t;

/* Catches any typo above (wrong member, missing one) at compile time instead
 * of silently reading/writing the wrong register at runtime. */
_Static_assert(sizeof(gpio_regs_t) == 0x28, "gpio_regs_t does not match RM0090's GPIO register map");

#define GPIOA_BASE 0x40020000UL
#define GPIO_PORT_STRIDE 0x400UL /* each port's register block is 0x400 bytes */

static gpio_regs_t *gpio(gpio_port_t port)
{
    return (gpio_regs_t *)(GPIOA_BASE + (uint32_t)port * GPIO_PORT_STRIDE);
}

/* RCC_AHB1ENR, base 0x40023800 + 0x30 offset. Bit n enables GPIO port n's
 * clock (GPIOAEN = bit 0 ... GPIOIEN = bit 8), which is exactly our
 * gpio_port_t enum value. */
#define RCC_AHB1ENR (*(volatile uint32_t *)(0x40023800UL + 0x30))

void gpio_init_output(gpio_port_t port, uint8_t pin)
{
    /* Enable this port's clock before touching any of its registers. */
    RCC_AHB1ENR |= (1u << (uint32_t)port);

    /* Errata "Delay after an RCC peripheral clock enabling" (same as
     * uart.c): read the enable register back to force the write to land
     * before we access the port's own registers below. */
    (void)RCC_AHB1ENR;

    gpio_regs_t *p = gpio(port);

    /* MODER: 2 bits per pin, 01 = general-purpose output. Clear the pair
     * of bits for this pin first, then set them to 01. */
    p->MODER &= ~(3u << (pin * 2));
    p->MODER |= (1u << (pin * 2));

    /* OTYPER: 1 bit per pin, 0 = push-pull (drives both high and low;
     * what an LED or any normal output needs, vs. open-drain which only
     * pulls low). Clearing is enough, but be explicit. */
    p->OTYPER &= ~(1u << pin);

    /* PUPDR: 2 bits per pin, 00 = no pull-up/pull-down -- a push-pull
     * output drives the line itself, it doesn't need one. */
    p->PUPDR &= ~(3u << (pin * 2));
}

void gpio_write(gpio_port_t port, uint8_t pin, bool value)
{
    gpio_regs_t *p = gpio(port);

    /*
     * BSRR (bit set/reset register) instead of a read-modify-write on
     * ODR. ODR |= (1 << pin) compiles to load-modify-store: three
     * separate bus steps with our locally-read ODR value held in a
     * register in between. If an interrupt (USART2 RX, SysTick, ...)
     * runs between that load and that store and changes a *different*
     * pin on the same port, its change gets silently overwritten when
     * we store our now-stale copy back -- a lost update.
     *
     * BSRR's low 16 bits each set one ODR bit to 1, its high 16 bits
     * (bit n+16) each reset one ODR bit to 0, and writing a 0 to any
     * BSRR bit does nothing. So BSRR = (1 << pin) touches only that one
     * pin, in a single atomic store, with no read at all -- there's no
     * stale copy of the other 15 pins to accidentally write back.
     */
    if (value) {
        p->BSRR = (1u << pin);
    } else {
        p->BSRR = (1u << (pin + 16));
    }
}

void gpio_toggle(gpio_port_t port, uint8_t pin)
{
    gpio_regs_t *p = gpio(port);

    /* No "toggle" bit exists in BSRR, so we still have to read ODR once
     * to learn the pin's current level -- but the actual write back out
     * goes through BSRR (via gpio_write), not a read-modify-write of
     * ODR, so it stays a single atomic store and can't stomp on any
     * other pin's bit the way `ODR ^= (1 << pin)` could. */
    bool current = (p->ODR & (1u << pin)) != 0;
    gpio_write(port, pin, !current);
}

bool gpio_read(gpio_port_t port, uint8_t pin)
{
    return (gpio(port)->IDR & (1u << pin)) != 0;
}
