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

NATIVE_MODULE_OBJECTS :=
NATIVE_MODULE_DEPS :=

# Each flavor links qagame, cgame and ui natively.
CFLAGS += -DQ360_NATIVE_MODULES

ifeq ($(OA),1)
QAGAME_SOURCES := \
	code/oa/code/game/g_main.c \
	code/oa/code/game/ai_chat.c \
	code/oa/code/game/ai_cmd.c \
	code/oa/code/game/ai_dmnet.c \
	code/oa/code/game/ai_dmq3.c \
	code/oa/code/game/ai_main.c \
	code/oa/code/game/ai_team.c \
	code/oa/code/game/ai_vcmd.c \
	code/oa/code/game/bg_misc.c \
	code/oa/code/game/bg_lib.c \
	code/oa/code/game/bg_pmove.c \
	code/oa/code/game/bg_slidemove.c \
	code/oa/code/game/g_active.c \
	code/oa/code/game/g_arenas.c \
	code/oa/code/game/g_admin.c \
	code/oa/code/game/g_bot.c \
	code/oa/code/game/g_client.c \
	code/oa/code/game/g_cmds.c \
	code/oa/code/game/g_cmds_ext.c \
	code/oa/code/game/g_combat.c \
	code/oa/code/game/g_items.c \
	code/oa/code/game/bg_alloc.c \
	code/oa/code/game/g_fileops.c \
	code/oa/code/game/g_killspree.c \
	code/oa/code/game/g_misc.c \
	code/oa/code/game/g_missile.c \
	code/oa/code/game/g_mover.c \
	code/oa/code/game/g_playerstore.c \
	code/oa/code/game/g_session.c \
	code/oa/code/game/g_spawn.c \
	code/oa/code/game/g_svcmds.c \
	code/oa/code/game/g_svcmds_ext.c \
	code/oa/code/game/g_target.c \
	code/oa/code/game/g_team.c \
	code/oa/code/game/g_trigger.c \
	code/oa/code/game/g_unlagged.c \
	code/oa/code/game/g_utils.c \
	code/oa/code/game/g_vote.c \
	code/oa/code/game/g_weapon.c \
	code/oa/code/qcommon/q_math.c \
	code/oa/code/qcommon/q_shared.c \
	code/oa/code/game/g_syscalls.c

CGAME_SOURCES := \
	code/oa/code/cgame/cg_main.c \
	code/oa/code/game/bg_misc.c \
	code/oa/code/game/bg_lib.c \
	code/oa/code/game/bg_pmove.c \
	code/oa/code/game/bg_slidemove.c \
	code/oa/code/cgame/cg_challenges.c \
	code/oa/code/cgame/cg_consolecmds.c \
	code/oa/code/cgame/cg_draw.c \
	code/oa/code/cgame/cg_drawtools.c \
	code/oa/code/cgame/cg_effects.c \
	code/oa/code/cgame/cg_ents.c \
	code/oa/code/cgame/cg_event.c \
	code/oa/code/cgame/cg_info.c \
	code/oa/code/cgame/cg_localents.c \
	code/oa/code/cgame/cg_marks.c \
	code/oa/code/cgame/cg_players.c \
	code/oa/code/cgame/cg_playerstate.c \
	code/oa/code/cgame/cg_predict.c \
	code/oa/code/cgame/cg_scoreboard.c \
	code/oa/code/cgame/cg_servercmds.c \
	code/oa/code/cgame/cg_snapshot.c \
	code/oa/code/cgame/cg_unlagged.c \
	code/oa/code/cgame/cg_view.c \
	code/oa/code/cgame/cg_weapons.c \
	code/oa/code/qcommon/q_math.c \
	code/oa/code/qcommon/q_shared.c \
	code/oa/code/cgame/cg_syscalls.c

UI_SOURCES := \
	code/oa/code/q3_ui/ui_main.c \
	code/oa/code/game/bg_misc.c \
	code/oa/code/game/bg_lib.c \
	code/oa/code/q3_ui/ui_addbots.c \
	code/oa/code/q3_ui/ui_atoms.c \
	code/oa/code/q3_ui/ui_cdkey.c \
	code/oa/code/q3_ui/ui_challenges.c \
	code/oa/code/q3_ui/ui_cinematics.c \
	code/oa/code/q3_ui/ui_confirm.c \
	code/oa/code/q3_ui/ui_connect.c \
	code/oa/code/q3_ui/ui_controls2.c \
	code/oa/code/q3_ui/ui_credits.c \
	code/oa/code/q3_ui/ui_demo2.c \
	code/oa/code/q3_ui/ui_display.c \
	code/oa/code/q3_ui/ui_firstconnect.c \
	code/oa/code/q3_ui/ui_gameinfo.c \
	code/oa/code/q3_ui/ui_ingame.c \
	code/oa/code/q3_ui/ui_loadconfig.c \
	code/oa/code/q3_ui/ui_menu.c \
	code/oa/code/q3_ui/ui_mfield.c \
	code/oa/code/q3_ui/ui_mods.c \
	code/oa/code/q3_ui/ui_network.c \
	code/oa/code/q3_ui/ui_options.c \
	code/oa/code/q3_ui/ui_password.c \
	code/oa/code/q3_ui/ui_playermodel.c \
	code/oa/code/q3_ui/ui_players.c \
	code/oa/code/q3_ui/ui_playersettings.c \
	code/oa/code/q3_ui/ui_preferences.c \
	code/oa/code/q3_ui/ui_qmenu.c \
	code/oa/code/q3_ui/ui_removebots.c \
	code/oa/code/q3_ui/ui_saveconfig.c \
	code/oa/code/q3_ui/ui_serverinfo.c \
	code/oa/code/q3_ui/ui_servers2.c \
	code/oa/code/q3_ui/ui_setup.c \
	code/oa/code/q3_ui/ui_sound.c \
	code/oa/code/q3_ui/ui_sparena.c \
	code/oa/code/q3_ui/ui_specifyserver.c \
	code/oa/code/q3_ui/ui_splevel.c \
	code/oa/code/q3_ui/ui_sppostgame.c \
	code/oa/code/q3_ui/ui_spskill.c \
	code/oa/code/q3_ui/ui_startserver.c \
	code/oa/code/q3_ui/ui_team.c \
	code/oa/code/q3_ui/ui_teamorders.c \
	code/oa/code/q3_ui/ui_video.c \
	code/oa/code/q3_ui/ui_votemenu.c \
	code/oa/code/q3_ui/ui_votemenu_fraglimit.c \
	code/oa/code/q3_ui/ui_votemenu_timelimit.c \
	code/oa/code/q3_ui/ui_votemenu_gametype.c \
	code/oa/code/q3_ui/ui_votemenu_kick.c \
	code/oa/code/q3_ui/ui_votemenu_map.c \
	code/oa/code/q3_ui/ui_votemenu_custom.c \
	code/oa/code/qcommon/q_math.c \
	code/oa/code/qcommon/q_shared.c \
	code/oa/code/ui/ui_syscalls.c
else

QAGAME_SOURCES := \
	code/game/g_main.c \
	code/game/ai_chat.c \
	code/game/ai_cmd.c \
	code/game/ai_dmnet.c \
	code/game/ai_dmq3.c \
	code/game/ai_main.c \
	code/game/ai_team.c \
	code/game/ai_vcmd.c \
	code/game/bg_misc.c \
	code/game/bg_pmove.c \
	code/game/bg_slidemove.c \
	code/game/bg_lib.c \
	code/game/g_active.c \
	code/game/g_arenas.c \
	code/game/g_bot.c \
	code/game/g_client.c \
	code/game/g_cmds.c \
	code/game/g_combat.c \
	code/game/g_items.c \
	code/game/g_mem.c \
	code/game/g_misc.c \
	code/game/g_missile.c \
	code/game/g_mover.c \
	code/game/g_session.c \
	code/game/g_spawn.c \
	code/game/g_svcmds.c \
	code/game/g_target.c \
	code/game/g_team.c \
	code/game/g_trigger.c \
	code/game/g_utils.c \
	code/game/g_weapon.c \
	code/qcommon/q_math.c \
	code/qcommon/q_shared.c \
	code/game/g_syscalls.c

CGAME_SOURCES := \
	code/cgame/cg_main.c \
	code/game/bg_misc.c \
	code/game/bg_pmove.c \
	code/game/bg_slidemove.c \
	code/game/bg_lib.c \
	code/cgame/cg_consolecmds.c \
	code/cgame/cg_draw.c \
	code/cgame/cg_drawtools.c \
	code/cgame/cg_effects.c \
	code/cgame/cg_ents.c \
	code/cgame/cg_event.c \
	code/cgame/cg_info.c \
	code/cgame/cg_localents.c \
	code/cgame/cg_marks.c \
	code/cgame/cg_particles.c \
	code/cgame/cg_players.c \
	code/cgame/cg_playerstate.c \
	code/cgame/cg_predict.c \
	code/cgame/cg_scoreboard.c \
	code/cgame/cg_servercmds.c \
	code/cgame/cg_snapshot.c \
	code/cgame/cg_view.c \
	code/cgame/cg_weapons.c \
	code/qcommon/q_math.c \
	code/qcommon/q_shared.c \
	code/cgame/cg_syscalls.c

ifeq ($(TA),1)
CGAME_SOURCES += \
	code/cgame/cg_newdraw.c \
	code/ui/ui_shared.c

UI_SOURCES := \
	code/ui/ui_main.c \
	code/game/bg_misc.c \
	code/game/bg_lib.c \
	code/ui/ui_atoms.c \
	code/ui/ui_gameinfo.c \
	code/ui/ui_players.c \
	code/ui/ui_shared.c \
	code/qcommon/q_math.c \
	code/qcommon/q_shared.c \
	code/ui/ui_syscalls.c
else
UI_SOURCES := \
	code/q3_ui/ui_main.c \
	code/game/bg_misc.c \
	code/game/bg_lib.c \
	code/q3_ui/ui_addbots.c \
	code/q3_ui/ui_atoms.c \
	code/q3_ui/ui_cdkey.c \
	code/q3_ui/ui_cinematics.c \
	code/q3_ui/ui_confirm.c \
	code/q3_ui/ui_connect.c \
	code/q3_ui/ui_controls2.c \
	code/q3_ui/ui_credits.c \
	code/q3_ui/ui_demo2.c \
	code/q3_ui/ui_display.c \
	code/q3_ui/ui_gameinfo.c \
	code/q3_ui/ui_ingame.c \
	code/q3_ui/ui_loadconfig.c \
	code/q3_ui/ui_menu.c \
	code/q3_ui/ui_mfield.c \
	code/q3_ui/ui_mods.c \
	code/q3_ui/ui_network.c \
	code/q3_ui/ui_options.c \
	code/q3_ui/ui_playermodel.c \
	code/q3_ui/ui_players.c \
	code/q3_ui/ui_playersettings.c \
	code/q3_ui/ui_preferences.c \
	code/q3_ui/ui_qmenu.c \
	code/q3_ui/ui_removebots.c \
	code/q3_ui/ui_saveconfig.c \
	code/q3_ui/ui_serverinfo.c \
	code/q3_ui/ui_servers2.c \
	code/q3_ui/ui_setup.c \
	code/q3_ui/ui_sound.c \
	code/q3_ui/ui_sparena.c \
	code/q3_ui/ui_specifyserver.c \
	code/q3_ui/ui_splevel.c \
	code/q3_ui/ui_sppostgame.c \
	code/q3_ui/ui_spskill.c \
	code/q3_ui/ui_startserver.c \
	code/q3_ui/ui_team.c \
	code/q3_ui/ui_teamorders.c \
	code/q3_ui/ui_video.c \
	code/qcommon/q_math.c \
	code/qcommon/q_shared.c \
	code/ui/ui_syscalls.c
endif
endif

QAGAME_OBJECTS := $(patsubst %.c,$(BUILD)/qagame/%.o,$(QAGAME_SOURCES))
CGAME_OBJECTS := $(patsubst %.c,$(BUILD)/cgame/%.o,$(CGAME_SOURCES))
UI_OBJECTS := $(patsubst %.c,$(BUILD)/ui/%.o,$(UI_SOURCES))
# -G 0 and -fno-common keep all module data in .data/.bss, inside the brackets of code/sys/xenon_module.ld.
QAGAME_CFLAGS := $(CFLAGS) -DQAGAME -fno-common -G 0
CGAME_CFLAGS := $(CFLAGS) -DCGAME -fno-common -G 0
UI_CFLAGS := $(CFLAGS) -DUI -fno-common -G 0
NATIVE_MODULE_OBJECTS := $(BUILD)/qagame_native.o $(BUILD)/cgame_native.o $(BUILD)/ui_native.o
NATIVE_MODULE_DEPS := $(QAGAME_OBJECTS:.o=.d) $(CGAME_OBJECTS:.o=.d) $(UI_OBJECTS:.o=.d)
DEPS += $(NATIVE_MODULE_DEPS)
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

$(BUILD)/qagame/%.o: %.c
	mkdir -p $(@D)
	$(CC) $(QAGAME_CFLAGS) -c $< -o $@

$(BUILD)/cgame/%.o: %.c
	mkdir -p $(@D)
	$(CC) $(CGAME_CFLAGS) -c $< -o $@

$(BUILD)/ui/%.o: %.c
	mkdir -p $(@D)
	$(CC) $(UI_CFLAGS) -c $< -o $@

# Each module links on its own, then hides every global but its entry points, so its q_shared/bg_* copies stay private.
$(BUILD)/qagame_native.o: $(QAGAME_OBJECTS) code/sys/xenon_module.ld
	mkdir -p $(@D)
	$(TOOLCHAIN)ld -r -T code/sys/xenon_module.ld $(QAGAME_OBJECTS) -o $(BUILD)/qagame_partial.o
	$(TOOLCHAIN)objcopy --redefine-sym dllEntry=qagame_dllEntry --redefine-sym vmMain=qagame_vmMain \
		--redefine-sym __module_data_start=__qagame_data_start --redefine-sym __module_data_end=__qagame_data_end \
		--redefine-sym __module_bss_start=__qagame_bss_start --redefine-sym __module_bss_end=__qagame_bss_end \
		$(BUILD)/qagame_partial.o $(BUILD)/qagame_renamed.o
	$(TOOLCHAIN)objcopy -G qagame_dllEntry -G qagame_vmMain \
		-G __qagame_data_start -G __qagame_data_end -G __qagame_bss_start -G __qagame_bss_end \
		$(BUILD)/qagame_renamed.o $@

$(BUILD)/cgame_native.o: $(CGAME_OBJECTS) code/sys/xenon_module.ld
	mkdir -p $(@D)
	$(TOOLCHAIN)ld -r -T code/sys/xenon_module.ld $(CGAME_OBJECTS) -o $(BUILD)/cgame_partial.o
	$(TOOLCHAIN)objcopy --redefine-sym dllEntry=cgame_dllEntry --redefine-sym vmMain=cgame_vmMain \
		--redefine-sym __module_data_start=__cgame_data_start --redefine-sym __module_data_end=__cgame_data_end \
		--redefine-sym __module_bss_start=__cgame_bss_start --redefine-sym __module_bss_end=__cgame_bss_end \
		$(BUILD)/cgame_partial.o $(BUILD)/cgame_renamed.o
	$(TOOLCHAIN)objcopy -G cgame_dllEntry -G cgame_vmMain \
		-G __cgame_data_start -G __cgame_data_end -G __cgame_bss_start -G __cgame_bss_end \
		$(BUILD)/cgame_renamed.o $@

$(BUILD)/ui_native.o: $(UI_OBJECTS) code/sys/xenon_module.ld
	mkdir -p $(@D)
	$(TOOLCHAIN)ld -r -T code/sys/xenon_module.ld $(UI_OBJECTS) -o $(BUILD)/ui_partial.o
	$(TOOLCHAIN)objcopy --redefine-sym dllEntry=ui_dllEntry --redefine-sym vmMain=ui_vmMain \
		--redefine-sym __module_data_start=__ui_data_start --redefine-sym __module_data_end=__ui_data_end \
		--redefine-sym __module_bss_start=__ui_bss_start --redefine-sym __module_bss_end=__ui_bss_end \
		$(BUILD)/ui_partial.o $(BUILD)/ui_renamed.o
	$(TOOLCHAIN)objcopy -G ui_dllEntry -G ui_vmMain \
		-G __ui_data_start -G __ui_data_end -G __ui_bss_start -G __ui_bss_end \
		$(BUILD)/ui_renamed.o $@

$(BUILD)/$(TARGET).elf: $(OBJECTS) $(NATIVE_MODULE_OBJECTS) $(XSDK)/build/libxsdk.a $(XSDK)/src/xsdk.ld
	mkdir -p $(@D)
	$(CC) $(LDFLAGS) -Wl,-Map,$(BUILD)/$(TARGET).map -o $@ $(OBJECTS) $(NATIVE_MODULE_OBJECTS) $(LIBS)

$(BUILD)/$(TARGET).xex: $(BUILD)/$(TARGET).elf $(XSDK)/tools/elf2xex.py
	$(PYTHON) $(XSDK)/tools/elf2xex.py $(BUILD)/$(TARGET).elf $(BUILD)/$(TARGET).xex

-include $(DEPS)
endif

.PHONY: all ta oa all-flavors clean FORCE
