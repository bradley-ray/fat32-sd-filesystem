CFLAGS ?= -W -Wall -Wextra -Werror -Wundef -Wshadow -Wdouble-promotion \
					-Wformat-truncation -fno-common -Wconversion \
					-g3 -Os -ffunction-sections -fdata-sections \
					-mcpu=cortex-m0plus -mthumb $(EXTRA_CFLAGS)
LDFLAGS ?= -Tlink.ld -nostartfiles -nostdlib --specs nano.specs -lc -lgcc -Wl,--gc-sections -Wl,-Map=build/$@.map
SOURCES ?= startup.s src/*.c lib/*/*.c
INCLUDE ?= -Ihelpers -Ilib/hal -Ilib/fat32 -Ilib/sd -Iinc

firmware.elf: $(SOURCES)
	arm-none-eabi-gcc $(SOURCES) $(CFLAGS) $(INCLUDE) $(LDFLAGS) -o build/$@ 

firmware.bin: firmware.elf
	arm-none-eabi-objcopy -O binary $< $@

clean:
	rm -rf build/firmware.*
