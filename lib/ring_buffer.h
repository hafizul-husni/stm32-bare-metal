#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdint.h>
#include <stdbool.h>

#define RING_BUFFER_SIZE 64

_Static_assert((RING_BUFFER_SIZE & (RING_BUFFER_SIZE - 1)) == 0,
               "RING_BUFFER_SIZE must be a power of two");

/*
 * Lock-free single-producer/single-consumer byte ring buffer.
 *
 * Intended use: one interrupt handler (producer) calls push(), one
 * foreground context (consumer) calls pop(). head is written only by
 * the producer, tail only by the consumer -- each side only ever reads
 * the other's index, so no critical section (disabling interrupts) is
 * needed to use this safely from an ISR and main() at the same time.
 */
typedef struct {
    volatile uint8_t  data[RING_BUFFER_SIZE];
    volatile uint16_t head;           /* next slot to write (producer-owned) */
    volatile uint16_t tail;           /* next slot to read  (consumer-owned) */
    volatile uint16_t dropped_count;  /* bytes lost to push() while full     */
} ring_buffer_t;

void     ring_buffer_init(ring_buffer_t *rb);
bool     ring_buffer_push(ring_buffer_t *rb, uint8_t byte);
bool     ring_buffer_pop(ring_buffer_t *rb, uint8_t *byte);
bool     ring_buffer_is_empty(const ring_buffer_t *rb);
bool     ring_buffer_is_full(const ring_buffer_t *rb);
uint16_t ring_buffer_dropped_count(const ring_buffer_t *rb);

#endif
