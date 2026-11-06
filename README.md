# ioQuake360

A port of [ioQuake3](https://github.com/ioquake/ioq3) to the Xbox 360, built on the
open [xsdk](https://github.com/xenosworks/xsdk) homebrew SDK. The default build
uses the null renderer; `RENDERER=vk` statically links
[Quake3e](https://github.com/ec-/Quake3e)'s Vulkan renderer on
[vulkan4xenon](https://github.com/xenosworks/vulkan4xenon), the Vulkan layer on Xenos.

Three builds are produced from the same source tree. Each is a `.xex` for an
RGH/JTAG console, with native `cgame` and `qagame`. Open Arena also has a
native `ui`; Quake III Arena and Team Arena run `vm/ui.qvm` from their paks.

| Variant | XEX | Title ID | Game dir | Native modules |
|---|---|---|---|---|
| Quake III Arena | `ioquake360.xex`    | `51330360` | `baseq3` | `qagame`, `cgame` |
| Team Arena      | `ioquake360_ta.xex` | `51330361` | `baseq3` + `missionpack` | `qagame`, `cgame` |
| Open Arena      | `ioquake360_oa.xex` | `51330362` | `baseoa` | `qagame`, `cgame`, `ui` |

## Status

**Complete.** The port runs on RGH/JTAG hardware with the Vulkan renderer,
and matches the PS3 port's feature set.

- Boots from Aurora with its own title name, title ID and icon per variant
- Quake3e's Vulkan renderer on vulkan4xenon, 1280x720 through the console's
  scaler, offline Xenos shader pack, every pipeline built at map load
- Native game modules for Q3A, Team Arena and Open Arena
- Gameplay with bots, every weapon precached at map load
- Networking: LAN discovery, internet server browser, master server, hosting,
  pure servers, HTTP map downloads ([curl-xenon](https://github.com/xenosworks/curl-xenon))
- Xbox 360 controller dual-stick input, **rumble**, aim assist, quick 180° turn
- The system keyboard for console, chat and menu fields
- Signed-in gamertag as the default player name
- XAudio sound, OGG Vorbis music with per-map playlists
- **Mod support** via `+set fs_game <mod>` (also how TA loads `missionpack`)
- The Guide, Back + Start to quit to the dashboard

Known limits:

- **1080i output is not supported.** The SDK's scaler refuses interlaced
  modes. Set the console to 720p or 1080p.
- No Kinect, no mouse, no USB keyboard or chatpad, no MSAA.

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
./tools/docker-make.sh clean all       # Quake III Arena (Q3A)
./tools/docker-make.sh clean ta        # Team Arena
./tools/docker-make.sh clean oa        # Open Arena
```

Other targets (all variants):

```bash
make          # .xex for the selected variant
make dist     # dist/ioQuake360/ with every .xex (RENDERER=vk), docs and notices
make shaders  # regenerate the Xenos shader bundle
make clean    # wipe all build dirs (build/, build_ta/, build_oa/)
```

### Build flags

| Flag | Effect |
|---|---|
| `DEBUG=1` | Adds `-DX360_DEBUG -g`; writes `ioquake360[_ta\|_oa].log` next to the `.xex`. Release builds write no log. |
| `RENDERER=vk` | Links Quake3e's Vulkan renderer. The default is `RENDERER=null`. |

### Outputs

| Variant | Path | Title ID |
|---|---|---|
| Quake III Arena | `build/ioquake360.xex` | `51330360` |
| Team Arena      | `build_ta/ioquake360_ta.xex` | `51330361` |
| Open Arena      | `build_oa/ioquake360_oa.xex` | `51330362` |

Run `make clean` after changing compiler flags. Switching `RENDERER` relinks
with the selected renderer while reusing the shared engine objects.

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

Layout: the PS3 port's dual-stick layout on the 360 pad. Buttons are
rebindable from the in-game menu. Bind screens show A, B, X, Y, LB, RB, LT, RT,
LS, RS and BACK; configs store them as `PAD_A` ... `PAD_BACK`.

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
| **RS** (click) | Quick 180° turn |
| **Back** | Scoreboard |
| **Start** | Menu (Escape) |
| **Back + A** | Open chat |
| **Back + Y** | Toggle console |
| **A** (console or chat open) | Open the system keyboard |
| **Back + Start** | Quit to dashboard |
| **LS + RS** | Toggle rumble on/off |
| **LB + RB** | Toggle aim assist on/off (default on) |

### Rumble

Triggered on own pain, own weapon fire and hit feedback.

| Cvar | Default | Notes |
|---|---|---|
| `x360_rumbleEnable` | `1` | 0 = off |
| `x360_rumbleScale`  | `1.0` | clamps to `[0, 1]` |

### Aim assist

Pulls the view toward the nearest visible enemy inside a small cone, weaker while the right stick is pushed. Servers can refuse it with `sv_allowAssist 0`.

| Cvar | Default | Notes |
|---|---|---|
| `cl_aimAssist` | `1` | 0 = off, 1 = on, 2 = only while zoomed |
| `cl_aimAssistAngle` | `5` | cone half-angle in degrees, max 45 |
| `cl_aimAssistStrength` | `0.80` | clamps to `[0, 1]` |

### Quick turn

**RS** turns the view 180°. Disabled on pure servers (`sv_pure 1`).

| Cvar | Default | Notes |
|---|---|---|
| `cl_turn180time` | `150` | turn time in ms, 0 = instant |

### Menus

| Input | Action |
|---|---|
| Left / Right stick | Move cursor |
| D-pad | Arrow keys |
| **A** | Confirm (Enter) |
| **A** on a text field | Open the system keyboard (TA: one A enters the field and opens it) |
| **B** | Back (Escape) |
| **Start** | Escape |

---

## Custom music

OGG Vorbis background music, with the PS3 port's per-map playlists.

The per-map playlists work in Quake III Arena and Team Arena. Open Arena plays the map's own track.

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
- **[xsdk](https://github.com/xenosworks/xsdk)** — the Xbox 360 homebrew SDK, GPU layer and toolchain.
- **[vulkan4xenon](https://github.com/xenosworks/vulkan4xenon)** — the Vulkan 1.0 layer on Xenos.
- **[OpenArena](https://github.com/OpenArena/gamecode)** — the Open Arena game code.
- **[Xenia](https://github.com/xenia-project/xenia)** and **[libxenon](https://github.com/Free60Project/libxenon)** — the public reverse-engineering the SDK builds on.

---

## AI disclosure

This port is an experiment. The goal was to test how well **Claude Haiku**
(Anthropic), a small and fast model, can write the code for a full console
port in a homebrew environment: a big-endian PowerPC target, an open SDK with no
Microsoft tooling, and a Vulkan layer over a GPU with no runtime shader
compiler.

The repo went from its first commit to a working port in three days
(2026-11-04 to 2026-11-06), in about 100 commits.

### Who did what

- **Planning and supervision: a higher capacity model.** The supervisor wrote the
  project plan. It cut the port into 10 phases (P0–P9) with milestone gates
  (M0–M8), and into 50 tasks (`Q360-1` … `Q360-50`). Each task has story
  points, a priority and acceptance criteria. The supervisor wrote a brief
  for each task, reviewed and built the result, merged it, and kept the plan
  up to date: results, commit hashes, decisions and new cards found during
  testing.
- **Coding: Claude Haiku.** A Haiku coding agent did the work for each task
  on a task branch, from the supervisor's brief.
- **Codex** (OpenAI) did a small part of the coding: the first draft of the
  shader specialization-constant patch (Q360-16), the diagnosis rounds after
  the first hardware test (Q360-29), and the patch rebase for the SDK move
  (Q360-45). Its sandbox could not run Docker or git, so the supervisor
  built, fixed and committed its output.
- **Human.** Before milestone M6 the human wrote no code and ran no tests. The
  only input was a choice when the supervisor asked one, for example which
  route to take for text input. Hands-on human work started at M6 (Hardware
  Gate 1): running each build on an RGH console, reporting what the screen
  and speakers did, collecting logs and captures, picking fixes, and the
  final polish.

### Kanban workflow

The work ran on a Kanban board (`plan/board.html`, a single HTML file) next
to the written plan (`plan/PROJECT_PLAN.md`). Both hold the same cards and
stay local (gitignored).

- **Columns:** Backlog → To Do → In Progress → In Review → Done, plus
  **Blocked**. A card carries its ID, phase, sprint, points, priority,
  acceptance criteria and an activity log.
- **WIP limit of one.** One card per session. When a card reaches Done, the
  supervisor reports and stops. A new session pulls the next card. This keeps
  each session's context small, which matters for a small model.
- **Gates.** Cards flow inside a phased roadmap. A phase's milestone must
  pass before the next phase's cards move to To Do. No hardware test was
  allowed before M6 (Q3A boots into gameplay); until then a card was verified
  by a clean build and a smoke run in Xenia.
- **Blocked cards** hold the reason and what would unblock them, for example
  1080i output (no source for the scaler values) and Kinect (no NUI support
  in the SDK).
- **Cross-cutting cards** run across all phases: review of every delegated
  task, licence notices, docs in sync, and submodule pinning.
- **New cards from testing.** Hardware runs added cards as they found
  problems, for example the SDK move (Q360-45), aim assist (Q360-48) and the
  first-use hitches (Q360-50).

### The task loop

1. The supervisor writes a brief: scope, files, acceptance criteria and the
   project rules (vendor before editing, `__XSDK__` guards, comment rules,
   keep upstream comments).
2. The coding agent works on `task/Q360-<n>-<slug>` and commits as
   `q360-dev`. The git history shows these commits.
3. The supervisor checks the agent for drift from the brief, reviews the
   diff, builds all three variants with both renderers in the Docker
   toolchain, and corrects the code to the project rules.
4. The supervisor runs the Xenia smoke where it applies, merges the branch
   into `main` (`Merge Q360-<n>: ...`), moves the card to Done and logs the
   result.

### How the work was verified

1. **Build gate:** Docker toolchain, three variants, null and Vulkan renderer.
2. **Emulator gate:** Xenia Canary, headless. Maps load and bots play to the
   time limit in all three variants. With the Vulkan renderer, every
   pipeline is created (Xenia has no display controller, so the swapchain
   step fails there by design).
3. **Hardware gate:** from M6 on, the builds ran on a real console. Each run
   went back to the supervisor as a log and captures. Example: one run showed
   a cyan frame. The supervisor traced it to the layer writing specialization
   constants to the `x` lane only, while the shader compiler reads a scalar
   register as a splat. The fix filled all four lanes, and the next run
   confirmed it.

[AGENTS.md](AGENTS.md) holds the rules the agents followed. All code was
reviewed and tested on hardware before release.

---

## License

ioQuake3 is GPLv2. This port layer is also GPLv2. See `LICENSE`.
Third-party components and their licences are listed in
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
