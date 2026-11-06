# AGENTS.md

Instructions for AI coding agents that work in this repository.
Read this file before you change anything. The README covers the user side.

## Project

ioQuake360 is a port of ioQuake3 to the Xbox 360 (RGH/JTAG consoles).
It builds three `.xex` files from one tree: Quake III Arena, Team Arena and Open Arena.

- CPU: Xenon, 3 cores / 6 hardware threads, PowerPC, **big-endian**.
- GPU: Xenos. No runtime shader compiler. Shaders are compiled offline.
- SDK: [xsdk](https://github.com/xenosworks/xsdk) (submodule `x360-sdk`). No Microsoft tooling, no leaked XDK material. Never add any.
- Renderer: Quake3e's `renderervk` on [vulkan4xenon](https://github.com/xenosworks/vulkan4xenon).
- HTTP downloads: `curl-xenon` on `mbedtls-xenon` (submodules).

## Layout

| Path | Content |
|---|---|
| `code/` | Vendored ioq3 `87a99d18` with the port changes |
| `code/renderervk/` | Vendored Quake3e `3173cef0` Vulkan renderer |
| `code/oa/` | Vendored Open Arena 0.8.8 gamecode |
| `code/sys/sys_xenon.c`, `code/sys/xenon_*.c` | Platform layer: system, input, sound, Vulkan surface, entry point |
| `code/sys/xenon_module.ld` | Linker script for the native game modules |
| `code/renderervk/shaders/xenon/` | Generated Xenos shader bundle (`make shaders`) |
| `patches/x360-sdk/`, `patches/vulkan4xenon/` | Changes to the submodules, applied by the Makefile |
| `docker/xsdk-toolchain/` | Toolchain image (GCC 15.3, binutils 2.47, newlib 4.6, `powerpc-xsdk-elf`) |
| `tools/` | `docker-make.ps1`, `docker-make.sh`, `build_vk_shaders.py` |
| `docs/RENDERER.md` | Every Quake3e / ioq3 / Xenos difference and its resolution |
| `flavors.mk` | Per-flavor defines, names and title IDs |

`plan/` and `results/` are local only (gitignored). Do not commit them.

## Build

All builds run in the Docker toolchain image. Do not build on the host.

```
docker build -t ioquake360/xsdk-toolchain docker/xsdk-toolchain
./tools/docker-make.sh clean all-flavors          # Q3A, TA, OA, null renderer
./tools/docker-make.sh RENDERER=vk all-flavors    # Vulkan renderer
./tools/docker-make.sh dist                       # release layout, RENDERER=vk
./tools/docker-make.sh DEBUG=1 all                # debug build, writes a log
./tools/docker-make.sh ARGS="+map q3dm1" all      # bake a test command line
```

- Single flavors: `all` (Q3A), `ta`, `oa`.
- Run `make clean` after you change compiler flags or switch `DEBUG`.
- Release builds write **no log**. Only `DEBUG=1` writes `ioquake360[_ta|_oa].log`. The switch is `xsdk_log_wanted` (patch `0003-log-switch.patch`).
- A change is done only when **all three flavors** build with **both** `RENDERER=null` and `RENDERER=vk`, with no new warning.

## Rules

1. **Vendor before editing.** Edit only the copies under `code/`. Never edit a reference tree outside this repo (ioq3, Quake3e, the PS3 port).
2. **Guard port code.** Put Xbox 360 changes in shared files under `#ifdef __XSDK__`. Keep the upstream path under `#else` where it differs.
3. **Do not edit submodules in place.** Write a numbered patch under `patches/<submodule>/`. The Makefile applies it. Submodules use `ignore = dirty`.
4. **Submodule bumps** are their own commits, with the reason in the message.
5. **Endianness.** Swap little-endian file and network data explicitly in new code.
6. **Shaders.** Change GLSL only inside `Q360_XENOS` blocks, then run `make shaders` and commit the regenerated bundle.
7. **Licences.** Each vendored library adds its source, commit and licence to `THIRD_PARTY_NOTICES.md` in the same merge.
8. **Docs.** Keep `README.md`, `INSTALLATION.md` and `docs/RENDERER.md` in step with the code.
9. **Line endings.** `.sh` files must use LF.

## Comments

- Comment only non-obvious logic. Do not restate the code.
- 1-2 lines per comment, block comments included. Keep the "why".
- These rules apply to our code only. Keep upstream comments (ioq3, Quake3e, OA) as they are.

## Git

- Work on a branch `task/Q360-<n>-<slug>`. Prefix commit subjects with the task ID.
- Agent commits use the author `q360-dev <q360-dev@users.noreply.github.com>`.
- Do not merge to `main`. The reviewer builds, checks and merges.
- No AI co-author trailers and no AI attribution in commit messages.

## Testing

1. **Build gate:** the six builds above.
2. **Emulator gate:** [Xenia Canary](https://github.com/xenia-canary/xenia-canary). Use a `DEBUG=1` build and put `dbgprint.txt` next to the `.xex`. Headless (null renderer): maps load and bots play to the time limit. With `RENDERER=vk`: every pipeline is created. Xenia has no display controller, so swapchain creation fails there. That is expected.
3. **Hardware gate:** only the maintainer runs builds on a console. Do not claim a hardware result you did not receive. Stop and ask for a hardware run when a change needs one (sound, pad, display, network, timing).

## Known traps

- **Varargs ABI.** A by-value struct before `...` lets `va_start` spill `r10` over the caller's `0x50(r1)`. Pass structs such as `netadr_t` by pointer to varargs functions (see `NET_OutOfBandPrintTo` in `code/qcommon/qcommon.h`, and `x360-sdk/docs/HARDWARE.md`).
- **XAM socket option `0x5802`** breaks every send to a non-LAN address. Use `0x5801` only. When packets get no reply, capture what arrives before you blame XAM.
- **Physical address 0** cannot be reserved. Boot and exit noise is fixed by a black surface shown before the LUT fill (`0004-black-display-surface.patch`).
- **Synchronous logging** stalls the game thread. That is why release builds keep no log.
- **1080i** is not supported: xsdk's scaler refuses interlaced output, and no source for the scaler values exists.
- **No mouse, no Kinect.** The retail kernel gives titles no mouse API. The SDK has no camera or NUI support.
- **Native modules.** `qagame` and `cgame` are native in all flavors; `ui` is native on OA only (`Q360_NATIVE_UI`). Q3A and TA run `vm/ui.qvm` from their paks. Native modules must still mark their QVM's pak, or pure servers drop the client.
