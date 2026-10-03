#ifndef UART_H
#define UART_H

#include <stdbool.h>

void uart_init(void);
void uart_putc(char c);
void uart_puts(const char *s);

/* Interrupt-driven RX, backed by a ring buffer filled by USART2_IRQHandler. */
bool uart_available(void);
bool uart_getc(char *c);

#endif
