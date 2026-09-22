# stm32-bare-metal

Bare-metal firmware for the STM32F407 (Cortex-M4), written directly against the
reference manual (RM0090) with no HAL, no CMSIS device drivers, and no vendor
libraries. This is a learning project for firmware/embedded job interviews —
optimize for understanding, not speed of delivery.

## Rules

- **Register-level only.** No ST HAL, no CMSIS peripheral drivers, no vendor
  libraries. Peripherals are accessed via raw memory-mapped register writes
  (`#define`/struct + volatile pointers), derived from RM0090. CMSIS core
  headers for the Cortex-M4 core itself (NVIC/SysTick intrinsics) are fine if
  ever needed, but peripheral register definitions are hand-written.
- **Target/board:** STM32F407 (Cortex-M4), tested in Renode using the
  `stm32f4_discovery` board.
- **Build system:** `make`. Keep `make`, `make run`, and `make clean` working
  at all times.
- **Before writing code:** explain the plan first — what we're building and
  why, and which reference manual registers/bits are involved (name the
  register, e.g. `RCC_AHB1ENR`, `GPIOx_MODER`, and the relevant bit fields).
  Don't write code before this explanation.
- **After writing code:** explain every line of what was added or changed —
  this is for interview prep, so err on the side of over-explaining register
  writes and bit manipulation.
- **Commits:** small and single-purpose — one feature or driver per commit.
  Don't bundle unrelated changes.

## Project structure

```
src/main.c                      application
startup/startup_stm32f407.c     vector table + Reset_Handler (in C, no assembly)
linker/stm32f407.ld             memory map: 1 MB FLASH, 128 KB RAM
renode/stm32f4.resc             simulator setup
```

## Build and run

Requirements: `arm-none-eabi-gcc`, `make`, [Renode](https://renode.io)

```bash
make          # builds build/firmware.elf and build/firmware.bin
make run      # opens Renode with the firmware loaded
```
