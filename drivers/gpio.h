#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>
#include <stdbool.h>

/*
 * GPIO ports on the STM32F407, in the order RM0090 lists them (and in the
 * order their RCC_AHB1ENR enable bits appear: GPIOAEN is bit 0, GPIOBEN is
 * bit 1, ... GPIOIEN is bit 8). Using the enum value directly as that bit
 * index, and as the x in "the x-th 0x400 block after GPIOA", is what lets
 * one gpio_init_output()/gpio_write()/gpio_toggle() implementation work for
 * every port instead of needing a copy per port like uart.c's GPIOA-only
 * macros did.
 */
typedef enum {
    GPIO_PORT_A = 0,
    GPIO_PORT_B = 1,
    GPIO_PORT_C = 2,
    GPIO_PORT_D = 3,
    GPIO_PORT_E = 4,
    GPIO_PORT_F = 5,
    GPIO_PORT_G = 6,
    GPIO_PORT_H = 7,
    GPIO_PORT_I = 8,
} gpio_port_t;

void gpio_init_output(gpio_port_t port, uint8_t pin);
void gpio_write(gpio_port_t port, uint8_t pin, bool value);
void gpio_toggle(gpio_port_t port, uint8_t pin);
bool gpio_read(gpio_port_t port, uint8_t pin);

#endif
