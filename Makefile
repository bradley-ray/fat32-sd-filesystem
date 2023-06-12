CFLAGS ?= -W -Wall -Wextra -Werror -Wundef -Wshadow -Wdouble-promotion \
					-Wformat-truncation -fno-common -Wconversion \
					-g3 -Os -ffunction-sections -fdata-sections \
					-mcpu=cortex-m0plus -mthumb -I. $(EXTRA_CFLAGS)
# LDFLAGS ?= -Tlink.ld -nostartfiles -nostdlib --specs nano.specs -u_printf_float -lc -lgcc -Wl,--gc-sections -Wl,-Map=$@.map
LDFLAGS ?= -Tlink.ld -nostartfiles -nostdlib --specs nano.specs -lc -lgcc -Wl,--gc-sections -Wl,-Map=$@.map
SOURCES ?= main.c hal/hal.c fat32/fat32.c sd/sd.c helpers/helpers.c stm32c0_irq.c startup.s syscalls.c
# SOURCES ?= main.s startup.s
# INCLUDE ?= -I helpers/ -I fat32/ -I hal/ -I sd/

firmware.elf: $(SOURCES)
	arm-none-eabi-gcc $(SOURCES) $(CFLAGS) $(LDFLAGS) -o build/$@ 

firmware.bin: firmware.elf
	arm-none-eabi-objcopy -O binary $< $@

clean:
	rm -rf build/firmware.*
