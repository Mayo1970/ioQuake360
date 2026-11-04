# ioQuake360

A port of [ioQuake3](https://github.com/ioquake/ioq3) to the Xbox 360, built on the
open [xsdk](https://github.com/openpak/x360-sdk) homebrew SDK and
[vulkan4xenon](https://github.com/xenosworks/vulkan4xenon), with
[Quake3e](https://github.com/ec-/Quake3e)'s Vulkan renderer.

Three builds are produced from the same source tree. Each is a `.xex` for an
RGH/JTAG console, with native `cgame`, `qagame` and `ui` (no QVM):

| Variant | XEX | Game dir |
|---|---|---|
| ioQuake3   | `ioquake360.xex`    | `baseq3` |
| Team Arena | `ioquake360_ta.xex` | `baseq3` + `missionpack` |
| Open Arena | `ioquake360_oa.xex` | `baseoa` |

## Status

**Pre-alpha. Not playable yet.** The repository holds the plan, the docs and
the SDK submodules. The engine port lands task by task. The first hardware test
is when Quake III Arena boots into gameplay.

Planned for the first release, to match the PS3 port:

- Boots, loads maps, gameplay with bots, online multiplayer
- Native game modules for Q3A, Team Arena and Open Arena
- Networking: LAN discovery, internet server browser, master server, hosting
- Xbox 360 controller dual-stick input + **rumble**
- **Mod support** via `+set fs_game <mod>` (also how TA loads `missionpack`)
- OGG Vorbis music with per-map playlists
- 1280x720 output through the console's scaler

## Prerequisites (Windows)

### 1. Docker Desktop

All builds run in a Docker image with the SDK's cross toolchain
(GCC 15.3, binutils 2.47, newlib 4.6, target `powerpc-xsdk-elf`).
Install [Docker Desktop](https://www.docker.com/products/docker-desktop/) with
the WSL 2 backend.

### 2. Submodules

```
git clone --recursive https://github.com/Mayo1970/ioQuake360.git
```

Or, in an existing clone:

```
git submodule update --init
```

### 3. Game data

You need to bring your own `.pk3` files — none are included in this repo.
See [INSTALLATION.md](INSTALLATION.md).

---

## Building

> The build system lands in the first sprint. The commands below are its
> target interface.

### Build the toolchain image (once)

```
docker build -t ioquake360/xsdk-toolchain docker/xsdk-toolchain
```

`build.sh` downloads the GCC, binutils and newlib sources and checks their
SHA-256 hashes. The build takes a while the first time.

### Build all variants at once

PowerShell:

```
tools\docker-make.ps1 clean all-flavors
```

Bash:

```bash
./tools/docker-make.sh clean all-flavors
```

### Build individual variants

```bash
./tools/docker-make.sh clean all       # ioQuake3 (Q3A)
./tools/docker-make.sh clean ta        # Team Arena
./tools/docker-make.sh clean oa        # Open Arena
```

Other targets (all variants):

```bash
make          # .xex for the selected variant
make dist     # dist/ioQuake360/ with every .xex, docs and notices
make shaders  # regenerate the Xenos shader bundle
make clean    # wipe all build dirs (build/, build_ta/, build_oa/)
```

### Build flags

| Flag | Effect |
|---|---|
| `DEBUG=1` | Adds `-DX360_DEBUG -g`; writes `log[_ta\|_oa].txt` next to the `.xex`. Release builds emit no log file. |

### Outputs

| Variant | Path |
|---|---|
| ioQuake3   | `build/ioquake360.xex` |
| Team Arena | `build_ta/ioquake360_ta.xex` |
| Open Arena | `build_oa/ioquake360_oa.xex` |

Always run `make clean` before rebuilding after a flag change.

### Compiler flags (under the hood)

- `TA=1` → `-DSTANDALONETA -DMISSIONPACK`. TA keeps `BASEGAME = "baseq3"` and
  forces `+set fs_game missionpack` on boot.
- `OA=1` → `-DSTANDALONEOA`. OA keeps `gamename = "Quake3Arena"`, like the
  PS3 port, for master server compatibility.
- Neither → Q3A (default).

---

## Xbox 360 directory layout

All three `.xex` files live in one folder with the game data. The engine's
base path is the folder the `.xex` runs from (`game:\`), so the three builds
share the paks and keep their own config files.

```
Hdd1:\Games\ioQuake360\ioquake360.xex
Hdd1:\Games\ioQuake360\ioquake360_ta.xex
Hdd1:\Games\ioQuake360\ioquake360_oa.xex
Hdd1:\Games\ioQuake360\baseq3\
Hdd1:\Games\ioQuake360\missionpack\
Hdd1:\Games\ioQuake360\baseoa\
```

### Installing custom mods

The engine respects `fs_game`. Drop mods under their own dir alongside
`baseq3\`:

```
Hdd1:\Games\ioQuake360\excessive\pak0.pk3
```

Then use the console (`\fs_game excessive`, then `\vid_restart`).

---

## Installing on Xbox 360

See **[INSTALLATION.md](INSTALLATION.md)** for the full step-by-step,
including where to get each build's required game files and exactly where to
place them.

---

## Controls (Xbox 360 controller)

Planned layout: the PS3 port's dual-stick layout on the 360 pad. Buttons are
rebindable from the in-game menu.

### In-game

| Input | Action |
|---|---|
| Left stick | Move (forward/back + strafe) |
| Right stick | Look (yaw + pitch) |
| **RT** | Fire |
| **LT** | Zoom |
| **A** | Jump |
| **B** | Crouch |
| **X** | Previous weapon |
| **Y** | Next weapon |
| **LB** | Strafe left |
| **RB** | Strafe right |
| **LS** (click) | Run / walk toggle |
| **RS** (click) | Scoreboard |
| **Back** | Scoreboard |
| **Start** | Menu (Escape) |
| **Back + A** | Open chat |
| **Back + Y** | Toggle console |
| **Back + Start** | Quit to dashboard |
| **LS + RS** | Toggle rumble on/off |

### Rumble

Triggered on own pain, own weapon fire and hit feedback.

| Cvar | Default | Notes |
|---|---|---|
| `x360_rumbleEnable` | `1` | 0 = off |
| `x360_rumbleScale`  | `1.0` | clamps to `[0, 1]` |

### Menus

| Input | Action |
|---|---|
| Left / Right stick | Move cursor |
| D-pad | Arrow keys |
| **A** | Confirm (Enter) |
| **B** | Back (Escape) |
| **Start** | Escape |

---

## Custom music

OGG Vorbis background music, with the PS3 port's per-map playlists.

**Search order:**

1. `playlist_<mapname>.cfg` — map-specific playlist
2. `autoexec_<mapname>.cfg` — map-specific autoexec
3. `playlist.cfg` — fallback generic playlist

**Format** (one path per line; `#` and `//` are comment lines):

```
random                 # optional: pick one song at random
music/track1.ogg       # bare path
music track2.ogg       # or "music <path>" syntax
```

---

## Important notes

- The Xbox 360 CPU is big-endian. ioQuake3 is endian-clean; new code must swap
  little-endian file and network data explicitly.
- The GPU has no runtime shader compiler in the driver. Shaders are compiled
  offline into a Xenos bundle (`make shaders`).
- All `.sh` scripts must have LF line endings.
- Kinect is not supported: the SDK has no camera or Kinect support.

## Credits

- **[ioQuake3](https://github.com/ioquake/ioq3)** — the upstream engine this port is based on.
- **[Quake3e](https://github.com/ec-/Quake3e)** — the Vulkan renderer.
- **[xsdk](https://github.com/openpak/x360-sdk)** — the Xbox 360 homebrew SDK, GPU layer and toolchain.
- **[vulkan4xenon](https://github.com/xenosworks/vulkan4xenon)** — the Vulkan 1.0 layer on Xenos.
- **[OpenArena](https://github.com/OpenArena/gamecode)** — the Open Arena game code.
- **[Xenia](https://github.com/xenia-project/xenia)** and **[libxenon](https://github.com/Free60Project/libxenon)** — the public reverse-engineering the SDK builds on.

---

## AI disclosure

Parts of this port were developed with the assistance of **Claude** (Anthropic) and **Codex** (OpenAI). AI was used for code generation, debugging, porting guidance, and documentation. All AI-generated code is reviewed and tested on hardware before release.

---

## License

ioQuake3 is GPLv2. This port layer is also GPLv2. See `LICENSE`.
Third-party components and their licences are listed in
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
