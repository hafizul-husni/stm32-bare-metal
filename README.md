# STM32 Bare-Metal Drivers

Register-level drivers for the STM32F407 (Cortex-M4), written from the reference manual with no HAL or vendor libraries, and tested in the Renode simulator.

## Progress
- [x] Day 1: Startup code, vector table, linker script, Makefile
- [ ] Day 2: UART driver
- [ ] Day 3: Interrupt-driven RX + ring buffer (unit tested)
- [ ] Day 4: GPIO + SysTick drivers
- [ ] Day 5: Automated Renode tests
- [ ] Day 6: GitHub Actions CI

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

In the Renode monitor, type `start`, wait a second, then `pause` and check:
```
sysbus ReadDoubleWord 0x20000000   # boot_magic   -> 0x00C0FFEE (.data copied)
sysbus ReadDoubleWord 0x20000004   # startup_ok   -> 0x00000001 (checks passed)
sysbus ReadDoubleWord 0x20000008   # loop_counter -> keeps increasing (main is running)
```

## Design decisions
- **Startup code in C instead of assembly:** easier to read and review; the Cortex-M core loads the stack pointer from the vector table itself, so no assembly is required.
- **Weak aliased handlers:** any interrupt handler can be overridden just by defining a function with the same name.
- **`-ffunction-sections` + `--gc-sections`:** unused code is removed, keeping the binary small (under 200 bytes today).
