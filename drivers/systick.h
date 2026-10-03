#ifndef SYSTICK_H
#define SYSTICK_H

#include <stdint.h>

/* Starts the 1 ms SysTick interrupt. Call once at boot. */
void systick_init(void);

/* Milliseconds since systick_init() was called. Wraps every 2^32 ms
 * (~49.7 days); use systick_elapsed()/systick_since() to compare two
 * readings safely across that wrap. */
uint32_t millis(void);

/* now - start, as unsigned 32-bit wraparound arithmetic. Correct even if
 * millis() has wrapped between the two readings -- see the comment in
 * systick.c for why. */
uint32_t systick_elapsed(uint32_t now, uint32_t start);

/* Convenience: systick_elapsed(millis(), start). */
uint32_t systick_since(uint32_t start);

/* The SysTick interrupt handler itself -- normally only ever invoked by
 * hardware via the vector table, never called directly by application
 * code. Declared here so the host-side unit tests can call it to
 * simulate ticks passing without needing real SysTick hardware. */
void SysTick_Handler(void);

#endif
