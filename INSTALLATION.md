# Installation

This port produces **three separate Xbox 360 executables** from one source
tree. Each is a standalone `.xex` with its own title name and title ID —
install only the ones you want. None of them include the copyrighted game
data; you provide your own `.pk3` files, bought legally, and FTP them to the
console.

You need an **RGH or JTAG** Xbox 360 with a homebrew dashboard such as
**Aurora** and its FTP server enabled.

## What each build is

| Build | XEX | What it is | Notable features |
|---|---|---|---|
| **ioQuake3** | `ioquake360.xex` | Quake III Arena | Full gameplay, bots, online/LAN multiplayer, mods via `fs_game` |
| **Team Arena** | `ioquake360_ta.xex` | Q3A + the *Team Arena* mission pack | Adds TA's extra weapons, game modes and UI on top of ioQuake3 |
| **Open Arena** | `ioquake360_oa.xex` | Free/open Q3A-compatible game | Same engine features, uses free OA game data — no retail purchase needed |

## Where to get the required files

Buy the base game(s) legally, then copy the `.pk3` files from your
install/disc into the paths below.

- **Quake III Arena** (needed for ioQuake3 and Team Arena): [Here](https://www.gog.com/en/game/quake_iii_arena)
- **Open Arena** [Here](https://openarena.ws/)

---

## Common layout

All three `.xex` files go into one folder, with the game data next to them.
The engine reads its data from the folder the `.xex` runs from, so the three
builds share one copy of the paks.

```
Hdd1:\Games\ioQuake360\<xex files>
Hdd1:\Games\ioQuake360\<game dir(s) below>
```

Create `Hdd1:\Games\ioQuake360\` via FTP before the first boot. Any folder on
the HDD or a USB drive works, as long as the game data sits next to the
`.xex`.

---

## ioQuake3 (Q3A)

XEX: `ioquake360.xex`

Copy your Quake III Arena `baseq3` paks:

```
Hdd1:\Games\ioQuake360\baseq3\pak0.pk3 … pak8.pk3
```

## Team Arena

XEX: `ioquake360_ta.xex`

Needs **both** Q3A's `baseq3` and Team Arena's `missionpack`:

```
Hdd1:\Games\ioQuake360\baseq3\pak0.pk3 … pak8.pk3
Hdd1:\Games\ioQuake360\missionpack\pak0.pk3 … pak3.pk3
```

## Open Arena

XEX: `ioquake360_oa.xex`

Copy Open Arena's paks — no `baseq3` needed:

```
Hdd1:\Games\ioQuake360\baseoa\pak0.pk3 …
```

---

## Installing a release

Releases will include a prebuilt `.xex` for each build — no need to compile
anything yourself.

1. Download the release zip and extract it.
2. FTP the `ioQuake360` folder to `Hdd1:\Games\` on the console.
3. FTP the matching game data from the sections above into it.
4. Launch the `.xex` you want from Aurora's file manager, or add
   `Hdd1:\Games\` as a scan path and launch it from the game list.

## Troubleshooting

If a build misbehaves after switching between builds (wrong game name,
missing intro video, etc.), delete that build's `*config.cfg` in its game
dir (`baseq3\`, `missionpack\` or `baseoa\`) — a stale config can override
the correct defaults.

Debug builds (`DEBUG=1`) write `ioquake360.log` / `ioquake360_ta.log` /
`ioquake360_oa.log` next to the `.xex`. Send it along with any bug report.
Release builds write no log.
