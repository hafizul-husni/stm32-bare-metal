TARGET  := firmware
BUILD   := build

CC      := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
SIZE    := arm-none-eabi-size

CPU     := -mcpu=cortex-m4 -mthumb
CFLAGS  := $(CPU) -std=c11 -Wall -Wextra -Werror -Og -g3 \
           -ffreestanding -ffunction-sections -fdata-sections -Idrivers -Ilib
LDFLAGS := $(CPU) -T linker/stm32f407.ld -nostdlib \
           -Wl,--gc-sections -Wl,-Map=$(BUILD)/$(TARGET).map

SRCS := src/main.c startup/startup_stm32f407.c drivers/uart.c drivers/gpio.c drivers/systick.c lib/ring_buffer.c
OBJS := $(SRCS:%.c=$(BUILD)/%.o)

# Host-native unit tests for hardware-independent code (lib/ring_buffer.c,
# the pure math in drivers/systick.c), using the vendored Unity test
# framework. Runs on the host gcc, not arm-none-eabi-gcc. Each test file
# has its own Unity main(), so each gets its own binary.
HOST_CC   := gcc
UNITY_DIR := tests/unity

RING_TEST_SRCS := tests/test_ring_buffer.c lib/ring_buffer.c
RING_TEST_BIN  := $(BUILD)/test_ring_buffer

SYSTICK_TEST_SRCS := tests/test_systick.c drivers/systick.c
SYSTICK_TEST_BIN  := $(BUILD)/test_systick

all: $(BUILD)/$(TARGET).elf $(BUILD)/$(TARGET).bin

$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/$(TARGET).elf: $(OBJS) linker/stm32f407.ld
	$(CC) $(OBJS) $(LDFLAGS) -o $@
	$(SIZE) $@

$(BUILD)/$(TARGET).bin: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

run: all
	renode --console renode/stm32f4.resc

# Unity itself is vendored third-party code we don't control, so it's built
# with -Wall only; our own test/lib code is held to the same -Werror
# standard as the firmware.
$(BUILD)/unity.o: $(UNITY_DIR)/unity.c
	@mkdir -p $(BUILD)
	$(HOST_CC) -std=c11 -Wall -I$(UNITY_DIR) -c $< -o $@

$(RING_TEST_BIN): $(RING_TEST_SRCS) $(BUILD)/unity.o
	@mkdir -p $(BUILD)
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Ilib -I$(UNITY_DIR) \
	           $(RING_TEST_SRCS) $(BUILD)/unity.o -o $@

$(SYSTICK_TEST_BIN): $(SYSTICK_TEST_SRCS) $(BUILD)/unity.o
	@mkdir -p $(BUILD)
	$(HOST_CC) -std=c11 -Wall -Wextra -Werror -Idrivers -I$(UNITY_DIR) \
	           $(SYSTICK_TEST_SRCS) $(BUILD)/unity.o -o $@

test: $(RING_TEST_BIN) $(SYSTICK_TEST_BIN)
	$(RING_TEST_BIN)
	$(SYSTICK_TEST_BIN)

clean:
	rm -rf $(BUILD)

.PHONY: all run clean test
