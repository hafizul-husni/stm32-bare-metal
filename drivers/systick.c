/*
 * SysTick driver: a free-running 1 ms tick counter.
 *
 * SysTick is part of the Cortex-M4 core itself (ARMv7-M architecture),
 * not an STM32 peripheral, so its registers come from the ARMv7-M
 * reference manual rather than RM0090 -- but they're still plain
 * memory-mapped registers, accessed the same register-level way as
 * everything else in this project.
 */
#include <stdint.h>
#include "systick.h"

/* ---- SysTick, base 0xE000E010 (fixed on every Cortex-M core) ---- */
#define SYST_CSR (*(volatile uint32_t *)(0xE000E010UL)) /* control and status */
#define SYST_RVR (*(volatile uint32_t *)(0xE000E014UL)) /* reload value       */
#define SYST_CVR (*(volatile uint32_t *)(0xE000E018UL)) /* current value      */

#define SYST_CSR_ENABLE    (1u << 0) /* 1 = counter enabled                    */
#define SYST_CSR_TICKINT   (1u << 1) /* 1 = request SysTick_Handler on wrap    */
#define SYST_CSR_CLKSOURCE (1u << 2) /* 1 = processor clock, 0 = that clock/8  */

static volatile uint32_t ms_ticks;

void systick_init(void)
{
    /*
     * Reload value for a 1 ms period, running from the 16 MHz HSI (the
     * clock the chip boots with -- we haven't touched any PLL or
     * prescaler, so the core clock is still 16 MHz) with CLKSOURCE = 1
     * (count processor clock cycles directly, not /8):
     *
     *   clocks per ms = 16,000,000 Hz / 1000 = 16,000
     *
     * SysTick counts *down* from RVR to 0, and the hardware reloads it
     * and fires on the 0 -> RVR wrap. So a period of N clocks needs
     * RVR = N - 1 (it counts N values: RVR, RVR-1, ..., 1, 0 -- that's
     * N clock edges before the wrap, matching a period of N clocks).
     */
    SYST_RVR = 16000u - 1u;

    /* Any write to CVR resets it to 0 and clears COUNTFLAG. Do this
     * before enabling so the first tick is a full 1 ms period, not
     * whatever was left over in CVR from before. */
    SYST_CVR = 0;

    SYST_CSR = SYST_CSR_ENABLE | SYST_CSR_TICKINT | SYST_CSR_CLKSOURCE;
}

/* Overrides the weak alias in startup_stm32f407.c. Runs once per ms. */
void SysTick_Handler(void)
{
    ms_ticks++;
}

uint32_t millis(void)
{
    /* A 32-bit aligned load is a single atomic bus transaction on
     * Cortex-M4 -- it can't observe a torn/half-updated value even if
     * SysTick_Handler fires between two instructions of the caller, so
     * no critical section (disabling interrupts) is needed here. */
    return ms_ticks;
}

uint32_t systick_elapsed(uint32_t now, uint32_t start)
{
    /*
     * Plain unsigned subtraction, which C defines to wrap modulo 2^32.
     * That's exactly what makes this safe across a millis() wraparound:
     * e.g. start = 0xFFFFFF00, now = 0x00000064 (ms_ticks wrapped past
     * zero in between) gives now - start = 0x164 = 356 in mod-2^32
     * arithmetic, which is the correct elapsed time -- even though
     * `now < start` as plain numbers would suggest otherwise.
     *
     * The bug this avoids: comparing `millis() > deadline` directly
     * works right up until the wrap, then is wrong for the next 2^32 ms.
     * Always going through a subtraction like this one sidesteps that.
     */
    return now - start;
}

uint32_t systick_since(uint32_t start)
{
    return systick_elapsed(millis(), start);
}
