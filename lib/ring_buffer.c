/*
 * Lock-free single-producer/single-consumer ring buffer.
 * See ring_buffer.h for the ownership rules that make it safe without
 * disabling interrupts. Hardware-independent: built for the target
 * (arm-none-eabi-gcc) and unit tested on the host (`make test`).
 */
#include "ring_buffer.h"

void ring_buffer_init(ring_buffer_t *rb)
{
    rb->head = 0;
    rb->tail = 0;
    rb->dropped_count = 0;
    for (uint16_t i = 0; i < RING_BUFFER_SIZE; i++) {
        rb->data[i] = 0;
    }
}

bool ring_buffer_is_empty(const ring_buffer_t *rb)
{
    return rb->head == rb->tail;
}

bool ring_buffer_is_full(const ring_buffer_t *rb)
{
    return (uint16_t)(rb->head - rb->tail) == RING_BUFFER_SIZE;
}

uint16_t ring_buffer_dropped_count(const ring_buffer_t *rb)
{
    return rb->dropped_count;
}

bool ring_buffer_push(ring_buffer_t *rb, uint8_t byte)
{
    if (ring_buffer_is_full(rb)) {
        rb->dropped_count++;
        return false;
    }

    /*
     * Write the byte, THEN publish it by advancing head. If data[] were
     * not volatile, the compiler would be free to reorder these two
     * stores: head and data are different objects with no data
     * dependency between them in this function, so nothing the compiler
     * can see requires the data write to happen first. A consumer
     * running in a different context (main(), which this ISR can
     * interrupt at any point) could then observe the new head value and
     * read data[] before the byte is actually there. Marking data[]
     * volatile makes every element access an observable side effect,
     * and the C standard guarantees volatile accesses happen in program
     * order relative to each other -- so this store is guaranteed to
     * complete before head moves.
     */
    rb->data[rb->head & (RING_BUFFER_SIZE - 1)] = byte;
    rb->head = (uint16_t)(rb->head + 1);
    return true;
}

bool ring_buffer_pop(ring_buffer_t *rb, uint8_t *byte)
{
    if (ring_buffer_is_empty(rb)) {
        return false;
    }

    *byte = rb->data[rb->tail & (RING_BUFFER_SIZE - 1)];
    rb->tail = (uint16_t)(rb->tail + 1);
    return true;
}
