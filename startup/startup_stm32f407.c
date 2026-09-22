/*
 * Minimal startup code for STM32F407 (Cortex-M4), written in C.
 *
 * On reset the Cortex-M core:
 *   1. loads the initial stack pointer from vector_table[0]
 *   2. jumps to the address in vector_table[1] (Reset_Handler)
 *
 * Reset_Handler then prepares RAM for C code and calls main().
 */
#include <stdint.h>

/* Symbols defined in the linker script */
extern uint32_t _estack;   /* top of RAM (initial stack pointer)      */
extern uint32_t _sidata;   /* start of .data initial values in FLASH  */
extern uint32_t _sdata;    /* start of .data in RAM                   */
extern uint32_t _edata;    /* end of .data in RAM                     */
extern uint32_t _sbss;     /* start of .bss in RAM                    */
extern uint32_t _ebss;     /* end of .bss in RAM                      */

int main(void);

void Reset_Handler(void);
void Default_Handler(void);

/* Weak aliases: any handler can be overridden later by defining a function with the same name */
void NMI_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void)     __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void)    __attribute__((weak, alias("Default_Handler")));

/* Core exception vectors. Peripheral IRQs (e.g. USART2) will be added on Day 3. */
__attribute__((section(".isr_vector"), used))
void (* const vector_table[])(void) = {
    (void (*)(void))(&_estack),  /* 0: initial stack pointer */
    Reset_Handler,               /* 1: reset                 */
    NMI_Handler,                 /* 2                        */
    HardFault_Handler,           /* 3                        */
    MemManage_Handler,           /* 4                        */
    BusFault_Handler,            /* 5                        */
    UsageFault_Handler,          /* 6                        */
    0, 0, 0, 0,                  /* 7-10: reserved           */
    SVC_Handler,                 /* 11                       */
    DebugMon_Handler,            /* 12                       */
    0,                           /* 13: reserved             */
    PendSV_Handler,              /* 14                       */
    SysTick_Handler,             /* 15                       */
};

void Reset_Handler(void)
{
    /* Copy initialised global variables (.data) from FLASH to RAM */
    uint32_t *src = &_sidata;
    for (uint32_t *dst = &_sdata; dst < &_edata; ) {
        *dst++ = *src++;
    }

    /* Zero uninitialised global variables (.bss) - C requires this */
    for (uint32_t *dst = &_sbss; dst < &_ebss; ) {
        *dst++ = 0;
    }

    main();

    /* main() should never return; trap here if it does */
    while (1) { }
}

void Default_Handler(void)
{
    /* Unexpected interrupt or fault: stop here so a debugger can inspect it */
    while (1) { }
}
