# Third-party notices

ioQuake360 is under the GNU General Public License, version 2 (LICENSE). It is built from the projects below. Each
keeps its own licence. This file ships beside every `.xex` in a release (`make dist`), which meets the binary notice
terms of the MIT and BSD licences below.

## Summary

| Component | Where | Licence | Linked into the `.xex` |
|---|---|---|---|
| ioquake3 (id Software, ioquake3 contributors) | `code/` (vendored) | GPL-2.0 | Yes |
| zlib 1.3.1 (Jean-loup Gailly and Mark Adler) | `code/thirdparty/zlib-1.3.1/` (vendored from ioq3 87a99d18) | zlib License | Yes |
| Quake3e Vulkan renderer | `code/renderervk/` (vendored) | GPL-2.0 | Only with `RENDERER=vk` |
| libjpeg 9f (Independent JPEG Group) | `code/thirdparty/jpeg-9f/` (vendored from ioq3 87a99d18) | IJG licence | Yes |
| OpenArena game code, `oa-0.8.8` | `code/oa/` (vendored) | GPL-2.0 | Yes (Open Arena build) |
| xsdk | `x360-sdk/` (submodule) | MIT, with BSD-3 parts | Yes (`libxsdk.a`) |
| vulkan4xenon | `vulkan4xenon/` (submodule) | MIT | No (Q360-14 uses headers only) |
| Khronos Vulkan headers | `vulkan4xenon/third_party/vulkan-headers/` | Apache-2.0 or MIT | Headers only |
| GCC runtime (libgcc, libstdc++) | toolchain | GPL-3.0 with the GCC Runtime Library Exception | Yes |
| newlib | toolchain | `COPYING.NEWLIB` (permissive) | Yes |
| GCC, binutils and the SDK's patches to them | `docker/xsdk-toolchain/` | GPL-3.0-or-later | No (build tools) |

## ioquake3, Quake3e and OpenArena

Quake III Arena's source code was released by id Software under the GPL, version 2. ioquake3, Quake3e and the
OpenArena game code carry that licence; the full text is in `LICENSE`. Their source files keep their own copyright
headers. The Quake III Arena, Team Arena and OpenArena **game data** is not covered and is not included.

## Vendored sources

- ioquake3: https://github.com/ioquake/ioq3, commit 87a99d18206e878a20289f05ac3c16005dcae5c3. Copied unchanged to
  `code/`: botlib, cgame, client, game, null, q3_ui, qcommon, renderercommon, server, sys, ui; `ui/menudef.h` copied
  unchanged to `ui/menudef.h` (the game code includes it as `../../ui/menudef.h`). GPL-2.0.
- Quake3e: https://github.com/ec-/Quake3e, commit 3173cef096dd0cb2fea26d91bce71049061f5a13. `code/renderervk` copied
  to `code/renderervk/` and `code/renderercommon` to `code/renderervk/renderercommon/`, then adapted to ioq3's renderer
  API (`docs/RENDERER.md`). The copied `renderercommon/tr_public.h`, `tr_types.h` and `vulkan/` headers were deleted, so
  the renderer uses ioq3's public API and the Vulkan headers from `vulkan4xenon`. `code/client/cl_jpeg.c` copied to
  `code/client/cl_jpeg.c`, then adapted to ioq3's client and jpeg-9f. GPL-2.0.
- libjpeg 9f: copied unchanged from ioq3 commit 87a99d18206e878a20289f05ac3c16005dcae5c3 at
  `code/thirdparty/jpeg-9f/`.
- OpenArena gamecode: https://github.com/OpenArena/gamecode, branch oa-0.8.8, copied unchanged from the Ioquake3-PS3
  port's `code/oa/` to `code/oa/`. `code/q3_ui` and `code/ui` (absent from the PS3 copy) copied unchanged from
  branch oa-0.8.8, commit 1006052, to `code/oa/code/q3_ui/` and `code/oa/code/ui/`. GPL-2.0.

## zlib

zlib 1.3.1 is copied unchanged from ioq3 commit 87a99d18206e878a20289f05ac3c16005dcae5c3 at
`code/thirdparty/zlib-1.3.1/`. The licence text is from `zlib.h`:

```
Copyright (C) 1995-2024 Jean-loup Gailly and Mark Adler

This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.

Jean-loup Gailly        Mark Adler
jloup@gzip.org          madler@alumni.caltech.edu
```

The Vulkan headers used by the renderer are from `vulkan4xenon/third_party/vulkan-headers/` and are available under
Apache-2.0 or MIT, as stated in `vulkan4xenon/third_party/vulkan-headers/LICENSE.md`.

## libjpeg 9f

The Independent JPEG Group's jpeg-9f sources are copied unchanged from ioq3 commit 87a99d18206e878a20289f05ac3c16005dcae5c3
at `code/thirdparty/jpeg-9f/`. Copyright (C) 1991-2024, Thomas G. Lane, Guido Vollbeding. IJG permits use, copying, modification and distribution without a fee. Source distributions must include
the original README copyright and no-warranty notice unchanged, and identify changes in accompanying documentation.
Binary distributions must include the acknowledgment: "this software is based in part on the work of the Independent JPEG
Group." Do not use an IJG author's or company's name to endorse derived products without prior written permission; refer
to the software as IJG software. It is provided without warranty or liability, and these terms also apply to derived
software. See the [IJG licence terms](https://jpegclub.org/reference/libjpeg-license/).

## xsdk

```
MIT License

Copyright (c) 2026 Mena

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

### Notices xsdk asks every title to ship

`libxsdk.a` includes code from libxenon and from Xenia, both under the 3-clause BSD licence, which asks that a program
distributed as a binary carry the notice in its documentation. A game or homebrew title built with xsdk meets that by
shipping this section with it: in its documentation, or in a notices file beside the `.xex`.

#### libxenon

In `src/gpu_draw.c`, `src/gpu_resolve.c`, `src/gpu_setup.c`, `src/shader.c` and `src/texture.c`, from libxenon's
`drivers/xenos/xe.c`.

```
Copyright (c) 2009, free60.org
USB Stack Copyright (c) 2000,2001,2002,2003,2005, Broadcom Corporation

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY free60.org ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL free60.org BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE
```

#### Xenia

In `src/texture_layout.c` and `src/tiled.c`, from Xenia's `src/xenia/gpu/texture_util.cc`, `texture_util.h` and
`texture_address.h`.
xost's shader encoder (`src/xsc_*.c`) also follows the instruction layouts in Xenia's microcode definitions.

```
Copyright (c) 2015, Ben Vanik.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the project nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL BEN VANIK BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```

## vulkan4xenon

```
MIT License

Copyright (c) 2026 Mena

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

vulkan4xenon includes the Khronos Group's Vulkan headers, under Apache-2.0 or MIT
(`vulkan4xenon/third_party/vulkan-headers/LICENSE.md`).

## The toolchain

`docker/xsdk-toolchain/` (from xsdk's `tools/toolchain/`) builds GCC, binutils and newlib, and its patches change those projects, so the patches are under
their licences: GCC's and binutils' GPL version 3 or later, and newlib's own licences (its `COPYING.NEWLIB`). Building
a title with the toolchain doesn't put the title under the GPL: GCC's runtime library exception covers the libgcc and
libstdc++ code linked into it. newlib's C library, also linked into every title, is under the permissive licences
listed in its `COPYING.NEWLIB`.
