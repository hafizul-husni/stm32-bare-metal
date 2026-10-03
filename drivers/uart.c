/*
 * USART2 polling driver for STM32F407 (PA2 = TX, PA3 = RX, AF7).
 * 115200 baud, 8N1, using the default 16 MHz HSI clock on APB1.
 * Register-level only, derived from RM0090.
 */
#include <stdint.h>
#include "uart.h"
#include "ring_buffer.h"

/* ---- RCC (Reset and Clock Control), base 0x40023800 ---- */
#define RCC_BASE    0x40023800UL
#define RCC_AHB1ENR (*(volatile uint32_t *)(RCC_BASE + 0x30))
#define RCC_APB1ENR (*(volatile uint32_t *)(RCC_BASE + 0x40))

#define RCC_AHB1ENR_GPIOAEN  (1u << 0)
#define RCC_APB1ENR_USART2EN (1u << 17)

/* ---- GPIOA, base 0x40020000 ---- */
#define GPIOA_BASE  0x40020000UL
#define GPIOA_MODER (*(volatile uint32_t *)(GPIOA_BASE + 0x00))
#define GPIOA_AFRL  (*(volatile uint32_t *)(GPIOA_BASE + 0x20))

/* ---- USART2, base 0x40004400 ---- */
#define USART2_BASE 0x40004400UL
#define USART2_SR   (*(volatile uint32_t *)(USART2_BASE + 0x00))
#define USART2_DR   (*(volatile uint32_t *)(USART2_BASE + 0x04))
#define USART2_BRR  (*(volatile uint32_t *)(USART2_BASE + 0x08))
#define USART2_CR1  (*(volatile uint32_t *)(USART2_BASE + 0x0C))

#define USART2_SR_ORE  (1u << 3)
#define USART2_SR_RXNE (1u << 5)
#define USART2_SR_TXE  (1u << 7)
#define USART2_CR1_RE     (1u << 2)
#define USART2_CR1_TE     (1u << 3)
#define USART2_CR1_RXNEIE (1u << 5)
#define USART2_CR1_UE     (1u << 13)

/* ---- NVIC (Cortex-M4 core) ---- */
#define NVIC_ISER1 (*(volatile uint32_t *)(0xE000E104UL))
#define NVIC_USART2_BIT (1u << 6) /* USART2 = IRQ38; ISER1 covers IRQ32-63, bit 38-32=6 */

static ring_buffer_t uart_rx_buffer;

void uart_init(void)
{
    /* Enable peripheral clocks: GPIOA (for PA2/PA3) and USART2 */
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC_APB1ENR |= RCC_APB1ENR_USART2EN;

    /*
     * Errata "Delay after an RCC peripheral clock enabling": the enable
     * write takes 1-2 peripheral clock cycles to actually reach the
     * peripheral. Reading the enable register back forces that write to
     * complete before we touch GPIOA/USART2 registers below.
     */
    (void)RCC_AHB1ENR;
    (void)RCC_APB1ENR;

    /*
     * PA2/PA3 -> alternate function mode (MODER = 10), AF7 (USART2) in
     * AFRL. Each pin gets 2 bits in MODER and 4 bits in AFRL.
     */
    GPIOA_MODER &= ~((3u << (2 * 2)) | (3u << (3 * 2)));
    GPIOA_MODER |= ((2u << (2 * 2)) | (2u << (3 * 2)));

    GPIOA_AFRL &= ~((0xFu << (4 * 2)) | (0xFu << (4 * 3)));
    GPIOA_AFRL |= ((7u << (4 * 2)) | (7u << (4 * 3)));

    /*
     * Baud rate (RM0090): USARTDIV = fCK / (16 * baud), fCK = 16 MHz
     * (HSI, APB1 undivided at reset), baud = 115200:
     *   USARTDIV = 16,000,000 / (16 * 115200) = 8.68
     *   mantissa = 8, fraction = round(0.68 * 16) = 11 (0xB)
     *   BRR = (mantissa << 4) | fraction = (8 << 4) | 11 = 0x8B
     */
    USART2_BRR = (8u << 4) | 11u;

    ring_buffer_init(&uart_rx_buffer);

    /* Enable transmitter, receiver, and the RX-not-empty interrupt, then the USART itself */
    USART2_CR1 |= USART2_CR1_TE | USART2_CR1_RE | USART2_CR1_RXNEIE;
    USART2_CR1 |= USART2_CR1_UE;

    /* Let the NVIC forward USART2's interrupt request to USART2_IRQHandler */
    NVIC_ISER1 |= NVIC_USART2_BIT;
}

void uart_putc(char c)
{
    while (!(USART2_SR & USART2_SR_TXE)) {
    }
    USART2_DR = (uint32_t)(uint8_t)c;
}

void uart_puts(const char *s)
{
    while (*s) {
        uart_putc(*s++);
    }
}

bool uart_available(void)
{
    return !ring_buffer_is_empty(&uart_rx_buffer);
}

bool uart_getc(char *c)
{
    uint8_t byte;
    if (!ring_buffer_pop(&uart_rx_buffer, &byte)) {
        return false;
    }
    *c = (char)byte;
    return true;
}

void USART2_IRQHandler(void)
{
    uint32_t sr = USART2_SR;

    if (sr & (USART2_SR_RXNE | USART2_SR_ORE)) {
        /*
         * Reading DR after SR clears both RXNE and ORE, whichever is
         * set. We must always do this read when either flag is set --
         * on overrun the byte that triggered ORE is already gone (a
         * different byte was lost before we got here), but skipping
         * the read would leave ORE stuck high and the USART would keep
         * re-firing this interrupt forever.
         */
        uint8_t byte = (uint8_t)USART2_DR;

        if (!(sr & USART2_SR_ORE)) {
            (void)ring_buffer_push(&uart_rx_buffer, byte);
        }
        /* On ORE, the byte just read is stale/not ours to keep -- drop it. */
    }
}
