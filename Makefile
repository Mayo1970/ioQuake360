# Build the XSDK title stub, serializing clean before combined build goals.

.DEFAULT_GOAL := all

include flavors.mk

TOOLCHAIN ?= powerpc-xsdk-elf-
CC := $(TOOLCHAIN)gcc
PYTHON ?= python3
XSDK ?= x360-sdk

SOURCES := code/sys/xenon_main.c
OBJECTS := $(patsubst %.c,$(BUILD)/obj/%.o,$(SOURCES))
DEPS := $(OBJECTS:.o=.d)

CFLAGS := -mcpu=cell -fno-pic -std=gnu17 -O2 -Wall -Wextra -I$(XSDK)/include -MMD -MP $(DEFINES_EXTRA) -DQ360_TITLE='"$(TITLE)"'
ifeq ($(DEBUG),1)
  CFLAGS += -DX360_DEBUG -g
endif

LDFLAGS := -static -nostdlib -T $(XSDK)/src/xsdk.ld
LIBS := -Wl,--start-group $(XSDK)/build/libxsdk.a -lstdc++ -lc -lm -lgcc -Wl,--end-group

CLEAN_BUILD_GOALS := $(filter-out clean,$(MAKECMDGOALS))
ifneq ($(filter clean,$(MAKECMDGOALS)),)
  ifneq ($(CLEAN_BUILD_GOALS),)
.PHONY: __clean_then_build $(CLEAN_BUILD_GOALS)
__clean_then_build:
	$(MAKE) clean
	$(MAKE) $(CLEAN_BUILD_GOALS)
$(CLEAN_BUILD_GOALS): __clean_then_build
clean:
	@:
  else
clean:
	rm -rf build build_ta build_oa
  endif
else
clean:
	rm -rf build build_ta build_oa

all: $(BUILD)/$(TARGET).xex

all-flavors:
	$(MAKE)
	$(MAKE) TA=1
	$(MAKE) OA=1

# Always asked, so the SDK's make rebuilds the library after a submodule bump; the ELF relinks only if it changed.
$(XSDK)/build/libxsdk.a: FORCE
	$(MAKE) -C $(XSDK) build/libxsdk.a

FORCE:

$(BUILD)/obj/%.o: %.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/$(TARGET).elf: $(OBJECTS) $(XSDK)/build/libxsdk.a $(XSDK)/src/xsdk.ld
	mkdir -p $(@D)
	$(CC) $(LDFLAGS) -Wl,-Map,$(BUILD)/$(TARGET).map -o $@ $(OBJECTS) $(LIBS)

$(BUILD)/$(TARGET).xex: $(BUILD)/$(TARGET).elf $(XSDK)/tools/elf2xex.py
	$(PYTHON) $(XSDK)/tools/elf2xex.py $(BUILD)/$(TARGET).elf $(BUILD)/$(TARGET).xex

-include $(DEPS)
endif

.PHONY: all ta oa all-flavors clean
