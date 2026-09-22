TARGET  := firmware
BUILD   := build

CC      := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
SIZE    := arm-none-eabi-size

CPU     := -mcpu=cortex-m4 -mthumb
CFLAGS  := $(CPU) -std=c11 -Wall -Wextra -Werror -Og -g3 \
           -ffreestanding -ffunction-sections -fdata-sections
LDFLAGS := $(CPU) -T linker/stm32f407.ld -nostdlib \
           -Wl,--gc-sections -Wl,-Map=$(BUILD)/$(TARGET).map

SRCS := src/main.c startup/startup_stm32f407.c
OBJS := $(SRCS:%.c=$(BUILD)/%.o)

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
	renode renode/stm32f4.resc

clean:
	rm -rf $(BUILD)

.PHONY: all run clean
