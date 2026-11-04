# Build the ioquake3 engine for the XSDK title.

.DEFAULT_GOAL := all

include flavors.mk

TOOLCHAIN ?= powerpc-xsdk-elf-
CC := $(TOOLCHAIN)gcc
PYTHON ?= python3
XSDK ?= x360-sdk
ARGS ?=

ZLIB_DIR := code/thirdparty/zlib-1.3.1
ZLIB_SOURCES := $(wildcard $(ZLIB_DIR)/*.c)
ENGINE_SOURCES := \
	code/qcommon/cm_load.c \
	code/qcommon/cm_patch.c \
	code/qcommon/cm_polylib.c \
	code/qcommon/cm_test.c \
	code/qcommon/cm_trace.c \
	code/qcommon/cmd.c \
	code/qcommon/common.c \
	code/qcommon/cvar.c \
	code/qcommon/files.c \
	code/qcommon/md4.c \
	code/qcommon/md5.c \
	code/qcommon/msg.c \
	code/qcommon/net_chan.c \
	code/qcommon/net_ip.c \
	code/qcommon/huffman.c \
	code/qcommon/q_math.c \
	code/qcommon/q_shared.c \
	code/qcommon/unzip.c \
	code/qcommon/ioapi.c \
	code/qcommon/vm.c \
	code/qcommon/vm_interpreted.c \
	code/server/sv_bot.c \
	code/server/sv_client.c \
	code/server/sv_ccmds.c \
	code/server/sv_game.c \
	code/server/sv_init.c \
	code/server/sv_main.c \
	code/server/sv_net_chan.c \
	code/server/sv_snapshot.c \
	code/server/sv_world.c \
	$(wildcard code/botlib/*.c) \
	code/client/cl_cgame.c \
	code/client/cl_cin.c \
	code/client/cl_console.c \
	code/client/cl_input.c \
	code/client/cl_keys.c \
	code/client/cl_main.c \
	code/client/cl_net_chan.c \
	code/client/cl_parse.c \
	code/client/cl_scrn.c \
	code/client/cl_ui.c \
	code/client/cl_avi.c \
	code/client/snd_adpcm.c \
	code/client/snd_dma.c \
	code/client/snd_mem.c \
	code/client/snd_mix.c \
	code/client/snd_wavelet.c \
	code/client/snd_main.c \
	code/client/snd_codec.c \
	code/client/snd_codec_wav.c \
	code/client/snd_openal.c \
	code/null/null_input.c \
	code/sys/sys_xenon.c \
	code/sys/xenon_main.c \
	code/sys/xenon_null_renderer.c \
	code/sys/xenon_snddma.c

SOURCES := $(ENGINE_SOURCES) $(ZLIB_SOURCES)
OBJECTS := $(patsubst %.c,$(BUILD)/obj/%.o,$(SOURCES))
DEPS := $(OBJECTS:.o=.d)
ARGS_STAMP := $(BUILD)/args.stamp

DEFINES := -DBOTLIB -DNO_VM_COMPILED -DUSE_INTERNAL_ZLIB -DNO_GZIP
CFLAGS := -mcpu=cell -fno-pic -fno-strict-aliasing -std=gnu17 -O2 -Wall -Wextra \
	-I$(XSDK)/include -I$(ZLIB_DIR) -MMD -MP $(DEFINES) $(DEFINES_EXTRA) \
	-DQ360_TITLE='"$(TITLE)"' -DQ360_ARGS='"$(ARGS)"'
ifeq ($(DEBUG),1)
  CFLAGS += -DX360_DEBUG -g
else
  CFLAGS += -DNDEBUG
endif

LDFLAGS := -static -nostdlib -T $(XSDK)/src/xsdk.ld
LIBS := -Wl,--start-group $(XSDK)/build/libxsdk.a -lstdc++ -lc -lm -lgcc -Wl,--end-group

$(patsubst %.c,$(BUILD)/obj/%.o,$(ZLIB_SOURCES)): CFLAGS += -w

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

$(ARGS_STAMP): FORCE
	mkdir -p $(@D)
	@if [ ! -f "$@" ] || [ "$$(cat "$@")" != '$(ARGS)' ]; then printf '%s' '$(ARGS)' > "$@"; fi

$(BUILD)/obj/code/sys/xenon_main.o: $(ARGS_STAMP)

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

.PHONY: all ta oa all-flavors clean FORCE
