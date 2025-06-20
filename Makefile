# ===== Makefile for ARM Cortex-M0 C Project with build/ output =====

# Project name
TARGET := firmware

# Build and source folders
BUILD_DIR := build
SRC_DIRS := app driver misc ui helper external external/printf .

# Tools
AS := arm-none-eabi-gcc
CC := arm-none-eabi-gcc
LD := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
SIZE := arm-none-eabi-size

# OS detection for RM and path fix
ifeq ($(OS),Windows_NT)
	RM := del /Q
	FixPath = $(subst /,\\,$1)
	WHERE := where
	NULL_OUTPUT := nul
else
	RM := rm -f
	FixPath = $1
	WHERE := which
	NULL_OUTPUT := /dev/null
endif

# === Sources ===
STARTUP := external/libcpu/start.S
C_SRCS := $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)/*.c))
ASM_SRCS := $(STARTUP)

C_OBJS := $(patsubst %.c, $(BUILD_DIR)/%.o, $(C_SRCS))
ASM_OBJS := $(patsubst %.S, $(BUILD_DIR)/%.o, $(ASM_SRCS))

OBJS := $(C_OBJS) $(ASM_OBJS)
DEPS := $(C_OBJS:.o=.d)

# Pre-create build directories
DIRS := $(sort $(dir $(OBJS)))
$(shell mkdir -p $(DIRS))

# Python detection
ifneq (, $(shell $(WHERE) python))
	MY_PYTHON := python
else ifneq (, $(shell $(WHERE) python3))
	MY_PYTHON := python3
endif
ifdef MY_PYTHON
	HAS_CRCMOD := $(shell $(MY_PYTHON) -c "import crcmod" 2>&1)
endif

# Version string from Git or fallback
AUTHOR_STRING ?= JasonLee
ifneq (, $(shell $(WHERE) git))
	VERSION_STRING ?= $(shell git describe --tags --exact-match 2>$(NULL_OUTPUT))
	ifeq (, $(VERSION_STRING))
		VERSION_STRING := $(shell git rev-parse --short HEAD)
	endif
endif
ifeq (, $(VERSION_STRING))
	VERSION_STRING := NOGIT
endif

# Flags
ASFLAGS := -c -mcpu=cortex-m0
CFLAGS := -Os -Wall -Werror -mcpu=cortex-m0 -fno-builtin -fshort-enums \
	-fno-delete-null-pointer-checks -std=c2x -MMD -Wextra \
	-DPRINTF_INCLUDE_CONFIG_H -DAUTHOR_STRING=\"$(AUTHOR_STRING)\" \
	-DVERSION_STRING=\"$(VERSION_STRING)\"
LDFLAGS := -z noexecstack -mcpu=cortex-m0 -nostartfiles -Wl,-T,external/libcpu/firmware.ld \
	-Wl,--gc-sections --specs=nano.specs
INC := -I. -Iexternal/libcpu/ -Iexternal/libcpu/ARMCM0/Include/

# ===== Rules =====
all: $(TARGET)
	$(OBJCOPY) -O binary $< $<.bin
ifeq (, $(MY_PYTHON))
	$(info !!!!!!!! PYTHON NOT FOUND, *.PACKED.BIN WON'T BE BUILT)
else ifneq (, $(HAS_CRCMOD))
	$(info !!!!!!!! CRCMOD NOT INSTALLED, *.PACKED.BIN WON'T BE BUILT)
	$(info !!!!!!!! run: pip install crcmod)
else
	-$(MY_PYTHON) tools/fw-pack.py $<.bin $(AUTHOR_STRING) $(VERSION_STRING) $<.packed.bin
endif
	$(SIZE) $<

$(TARGET): $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

# === Compile rules ===
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

$(BUILD_DIR)/%.o: %.S
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -c $< -o $@

# Only include dependencies from C files
-include $(DEPS)

clean:
	$(RM) $(call FixPath, $(TARGET) $(TARGET).bin $(TARGET).packed.bin)
	$(RM) -r $(call FixPath, $(BUILD_DIR))

.PHONY: all clean
