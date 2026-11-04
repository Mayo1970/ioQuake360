# Renderer API port

This file lists how Quake3e's `code/renderervk/` (commit `3173cef0`) was ported to ioq3's renderer API (commit `87a99d18`). The null renderer remains the default. `RENDERER=vk` statically links renderervk, while engine objects keep the same flags and shared build paths for both renderer choices.

The ioq3 `refimport_t` is extended only under `__XSDK__` with the approved Vulkan, gamma and JPEG hooks. The platform creates the 1280x720 `VK_KHR_display` surface through `vulkan4xenon`; gamma stays disabled. `RENDERER=vk` links `libvulkan4xenon.a`, while the default null renderer does not.

## `refimport_t`

| Difference | Resolution | Files |
|---|---|---|
| Quake3e gamma and Vulkan platform callbacks are absent from ioq3. | Append the Quake3e signatures under `__XSDK__`; assign the gamma and Vulkan stubs in `CL_InitRef`. | `code/renderercommon/tr_public.h`, `code/client/cl_main.c`, `code/sys/xenon_vk.c` |
| JPEG load and save callbacks are absent from ioq3. | Append the Quake3e signatures under `__XSDK__` and assign the client JPEG functions in `CL_InitRef`. | `code/renderercommon/tr_public.h`, `code/client/cl_main.c`, `code/client/cl_jpeg.c` |
| `Microseconds` is not imported. | Not needed: no compiled renderer code calls it. | - |
| `Hunk_GetTempMemory` is not imported. | Load the world file through ioq3's `FS_ReadFile`. | `code/renderervk/tr_bsp.c` |
| `FreeAll` is not imported. | `TR_FreeAll` calls `Z_FreeTags( TAG_RENDERER )`, as Quake3e's client does. ioq3's `ri.Malloc` uses the same tag. | `code/renderervk/tr_init.c`, `code/renderervk/tr_ioq3.c` |
| Cvar groups are not imported. | `TR_CvarSetGroup` keeps a list of the renderer's group cvars. `TR_CvarCheckGroup` reports a set `modified` flag, and `TR_CvarResetGroup` clears the flags. So `r_gamma`, `r_textureMode` and the bloom cvars apply at the end of the frame, as in Quake3e. | `code/renderervk/tr_cmds.c`, `code/renderervk/tr_init.c`, `code/renderervk/tr_ioq3.c` |
| `Cvar_VariableString` and `Cvar_VariableStringBuffer` are not imported. | Call the engine's functions directly. The link is static. | `code/renderervk/tr_init.c`, `code/renderervk/tr_shader.c` |
| `Cvar_CheckRange` takes string limits and a validator in Quake3e, but numeric limits and an integral flag in ioq3. | Parse optional string limits locally and call ioq3's numeric range function. | `code/renderervk/tr_init.c`, `code/renderervk/tr_ioq3.c` |
| `CL_IsMinimized`, `CL_SetScaling` and `Sys_SetClipboardBitmap` are absent. | Treat the fixed display as not minimized; scaling and clipboard bitmap requests are no-ops. | `code/renderervk/tr_cmds.c`, `code/renderervk/tr_init.c`, `code/renderervk/vk.c`, `code/renderervk/tr_ioq3.c` |
| `Com_RealTime` is not imported. | Call ioq3's qcommon function directly. | `code/renderervk/tr_init.c` |
| OpenGL `GLimp_Init`, `GLimp_Shutdown`, `GLimp_EndFrame` and `GL_GetProcAddress` are absent. | Not needed: they are only used in the `#ifndef USE_VULKAN` branches, which do not compile. | - |
| Quake3e allocator callbacks accept `size_t`; ioq3 callbacks accept `int`. | Keep ioq3's callbacks; the Xenon build uses a 32-bit ABI, so the renderer's existing allocation sizes have the same width. | `code/renderercommon/tr_public.h`, `code/renderervk/` |
| Quake3e `FS_ReadFile` returns `int`; ioq3 returns `long`. | Use ioq3's return value directly. `long` is 32-bit in this build. | `code/renderercommon/tr_public.h`, `code/renderervk/tr_bsp.c`, `code/renderervk/renderercommon/` |

## `refexport_t`

| Difference | Resolution | Files |
|---|---|---|
| ioq3 `Shutdown(qboolean destroyWindow)` differs from Quake3e's `refShutdownCode_t`. | Wrap the call: false keeps the context; true destroys the window. | `code/renderervk/tr_init.c`, `code/renderervk/tr_ioq3.h` |
| ioq3 `AddRefEntityToScene` has no `intShaderTime` argument. | Wrap the renderer entry point and always pass false. | `code/renderervk/tr_init.c` |
| ioq3 cinematic draw and upload callbacks take `const byte *`; Quake3e takes `byte *`. | Use ioq3-signature wrappers to call the renderer's read-only implementations. | `code/renderervk/tr_init.c` |
| `ThrottleBackend` is not an ioq3 export. | Keep it renderer-internal; ioq3 has no client call site. | `code/renderervk/tr_cmds.c`, `code/renderervk/tr_init.c` |
| `FinishBloom` is not an ioq3 export. | Not called. Quake3e's client calls it only before the console. `RB_StretchPic` already applies bloom before the first 2D draw. Only the debug cvars `r_showImages` and `r_showtris 1` change timing. | `code/renderervk/tr_cmds.c` |
| `SetColorMappings` is not an ioq3 export. | Keep it renderer-internal; renderer cvar and image paths call the mapping code directly. | `code/renderervk/tr_cmds.c`, `code/renderervk/tr_image.c`, `code/renderervk/tr_init.c` |
| `CanMinimize` is not an ioq3 export. | Keep the renderer check internal; ioq3's fixed-display compatibility helper always reports not minimized. | `code/renderervk/tr_cmds.c`, `code/renderervk/tr_init.c`, `code/renderervk/tr_ioq3.c` |
| `GetConfig` is not an ioq3 export. | Use the renderer's `glConfig`; ioq3 receives the config through `BeginRegistration`. | `code/renderervk/tr_init.c` |
| `VertexLighting` is not an ioq3 export. | Keep the renderer toggle internal; ioq3 has no corresponding client call. | `code/renderervk/tr_cmds.c`, `code/renderervk/tr_init.c` |
| `SyncRender` is not an ioq3 export. | Keep the sync routine internal; ioq3 has no corresponding client call. | `code/renderervk/tr_init.c` |
| `AddLinearLightToScene` is not an ioq3 export. | Keep the Quake3e entry point internal; ioq3 game modules do not issue that Quake3e syscall. | `code/renderervk/tr_scene.c`, `code/renderervk/tr_init.c` |
| Quake3e has extra export slots. | `GetRefAPI` fills every ioq3 `refexport_t` slot and leaves the Quake3e-only slots out. | `code/renderervk/tr_init.c` |

## `tr_types.h`

| Difference | Resolution | Files |
|---|---|---|
| Quake3e `refEntity_t` uses `shader.rgba`/`shader.u32` and a `floatint_t shaderTime`; ioq3 uses `shaderRGBA[4]` and `float shaderTime`. | Read ioq3's byte array and float. Packed color copies use `Com_Memcpy`; the export wrapper always sets `intShaderTime` false. | `code/renderervk/tr_shade.c`, `code/renderervk/tr_shade_calc.c`, `code/renderervk/tr_surface.c`, `code/renderervk/tr_backend.c`, `code/renderervk/tr_init.c` |
| Quake3e `polyVert_t.modulate` is a `color4ub_t`; ioq3 exposes `byte[4]`. | Copy the four bytes into the renderer's local color union. | `code/renderervk/tr_surface.c` |
| `color4ub_t` and `MAX_VIDEO_HANDLES` are Quake3e renderer types/constants, absent from ioq3. | Define these renderer-local in the compatibility header. | `code/renderervk/tr_ioq3.h` |
| ioq3 `qfiles.h` stores `drawVert_t.color` as `byte[4]`; Quake3e uses `color4ub_t`. | Index ioq3 vertex bytes directly and copy them into the renderer's color union where needed. | `code/renderervk/tr_curve.c`, `code/renderervk/tr_bsp.c`, `code/renderervk/tr_surface.c` |

## q_shared/qcommon helpers

| Difference | Resolution | Files |
|---|---|---|
| ioq3 lacks Quake3e renderer helpers and constants: `QALIGN`, `Vector4Set`, `VectorScale4`, `DotProduct4`, `SGN`, `MAX_UINT`, `CONTENTS_NODE` and `myftol`. | Define renderer-local equivalents without changing game-shared qcommon files. | `code/renderervk/tr_ioq3.h` |
| ioq3 lacks `log2pad`, `Q_stradd`, `Q_atof`, `Com_Split`, `Com_GenerateHashValue` and `crc32_buffer`. | Copy Quake3e's bodies unchanged (`Q_isfinite` becomes C99 `isfinite`). Macros rename them to `TR_*`, so they do not clash with engine symbols. | `code/renderervk/tr_ioq3.h`, `code/renderervk/tr_ioq3.c` |
| Quake3e expects `tokenType_t`, `com_tokentype` and parser functions using `const char **`; ioq3's parser uses `char **` and lacks `COM_ParseComplex`. | Copy Quake3e's parser unchanged (`COM_BeginParseSession`, `COM_GetCurrentParseLine`, `COM_Parse`, `COM_ParseExt`, `COM_ParseComplex`, `SkipBracedSection`, `SkipRestOfLine`), renamed to `TR_*`. The renderer keeps its own line count for parse messages. | `code/renderervk/tr_ioq3.h`, `code/renderervk/tr_ioq3.c` |
| Quake3e `cvarGroup_t`, `cvarValidator_t` and `refShutdownCode_t` are absent from ioq3. | Define them in the renderer header with Quake3e's values. | `code/renderervk/tr_ioq3.h` |
| ioq3's `tr_types.h` defines `REFENTITYNUM_BITS` 10 and its related macros; Quake3e's `tr_local.h` uses 12. | `tr_ioq3.h` undefines ioq3's values, so the renderer keeps Quake3e's 12 bits. The client never reads these macros. | `code/renderervk/tr_ioq3.h` |
| ioq3 lacks `CVAR_DEVELOPER` and `CVAR_ARCHIVE_ND`. | Map the developer-only flag to zero and map non-dirty archive to `CVAR_ARCHIVE` in the renderer compatibility header. | `code/renderervk/tr_ioq3.h`, `code/renderervk/tr_init.c` |

## Headers and include layout

| Difference | Resolution | Files |
|---|---|---|
| Quake3e's copied `tr_public.h` and `tr_types.h` shadow ioq3's API; its bundled `renderercommon/vulkan/` shadows the selected Vulkan headers. | Delete those copies. Renderer common sources include ioq3 headers; `vk.h` includes `<vulkan/vulkan.h>` from the submodule include path, added for all objects. | `code/renderervk/renderercommon/`, `code/renderervk/vk.h`, `code/renderercommon/tr_public.h`, `Makefile` |
| Quake3e renderer common sources use paths relative to its original tree; the PNG loader refers to a missing local `qcommon/puff.h`. | Point includes at ioq3's qcommon and renderercommon headers and build ioq3's `puff.c` for `RENDERER=vk`. | `code/renderervk/renderercommon/*.c`, `code/renderercommon/puff.c`, `Makefile` |
| ioq3's renderercommon image loaders and `tr_font.c` depend on the GL renderer. | The Vulkan source list builds Quake3e's renderercommon sources and leaves ioq3's GL sources out; FreeType remains disabled. | `Makefile` |

## vulkan4xenon

Each row cites the `vulkan4xenon/docs/SUPPORT.md` section. "No change" rows are limits the upstream code already respects.

| Missing in vulkan4xenon | Resolution | Files |
|---|---|---|
| The only display path is `VK_KHR_display`: one 1280x720 mode, full-screen, opaque, untransformed surfaces (Display and swapchain). | `VK_CreateSurface` takes the first display mode and creates plane 0 with identity transform and opaque alpha; `VK_GetInstanceProcAddr` returns the layer's `vkGetInstanceProcAddr`. | `code/sys/xenon_vk.c` |
| Swapchain images are colour attachments only (Display and swapchain). With `r_fbo 0` the renderer asks for transfer usage and stops. | `r_fbo` defaults to 1 and is forced to 1, so the final pass draws into the swapchain. | `code/renderervk/tr_init.c` |
| `sampledImageDepthSampleCounts` is 0 and images have one sample (Physical device). `MIN` of colour and depth counts gives 0 samples for the screen map. | `vkMaxSamples` is 1 and MSAA is off. | `code/renderervk/vk.c` |
| Storage-buffer descriptors are refused (Descriptor sets). The flare test creates them at init. | The storage pool entry, set layout, pipeline layout, buffer, descriptor and their destroy calls are compiled out. `fragmentStores` stays false, so upstream `R_ClearFlares` keeps flares off. | `code/renderervk/vk.c` |
| `R16G16B16A16_UNORM` (`r_hdr 1`) and `B4G4R4A4` (`r_hdr -1`) are not colour attachments (Formats). | `get_hdr_format` returns the base format. | `code/renderervk/vk.c` |
| Vulkan 1.0 only, no layers; instance extensions `VK_KHR_surface` and `VK_KHR_display` only (Version, Extensions). | No change: the release build asks 1.0 (1.1 needs `_DEBUG`), validation needs `_WIN32`, and the renderer enables only listed extensions. | - |
| Only six device features (Device). | No change: `wideLines`, `fragmentStoresAndAtomics` and `shaderInt64` are enabled only when reported. | - |
| One surface format, `B8G8R8A8_UNORM` / `SRGB_NONLINEAR` (Display and swapchain). | No change: `vk_select_surface_format` falls back to the base format for any `r_presentBits`. | - |
| FIFO only, 2 or 3 images, one acquired image, one swapchain, no `oldSwapchain` (Display and swapchain). | No change: the renderer picks FIFO and 3 images, presents before its next acquire, and destroys the swapchain before a restart with `oldSwapchain` `VK_NULL_HANDLE`. | - |
| Linear tiling has no `BLIT_DST` (Formats). | No change: `vk_blit_enabled` is false and `capture_format` falls back to `color_format`. | - |
| Xenia has no display controller, so `vkCreateSwapchainKHR` returns `VK_ERROR_INITIALIZATION_FAILED` (Display and swapchain). | Expected; no workaround. | - |

## Shader pack

`make shaders` builds `code/renderervk/shaders/xenon/vk_shader_bundle.c` and `.h`, which are committed. `tools/build_vk_shaders.py` takes every variant `compile.bat` makes and adds `Q360_XENOS`. glslang compiles each one, and xost turns it into a Xenos container through `vulkan4xenon/tools/build_shaders.py`. Vertex shaders also get an instanced variant. On `__XSDK__`, `vk.c` includes the bundle and passes its SPIR-V to `vkCreateShaderModule`. The layer then finds the container by hash, so the console compiles no shader. The script stops if xost refuses any variant. 113 shaders build, and none of them exceeds the 64-register share.

| Difference | Resolution | Files |
|---|---|---|
| vulkan4xenon and xost had no specialization constants, and renderervk relies on them. | `patches/x360-sdk` and `patches/vulkan4xenon` add them. Each scalar constant gets a float constant register, and the layer writes the value from `VkSpecializationInfo` when it binds the pipeline. xost lowers `OpSpecConstantOp` (logical ops, `Select`, int comparisons) and `OpSpecConstantComposite`. It also lowers int comparisons. A fragment input at a skipped location still takes its interpolator. The Makefile applies the patches with `patch(1)` before the SDK builds. | `patches/`, `Makefile` |
| xost takes a pixel shader's if/else only when each arm stores one local or the output, merges one if/else local in each shader, and takes no discard-only arm or partial store. | `Q360_XENOS` bodies: the texture modes work out their samples first, and each arm stores `base`. Alpha-test and discard modes become one summed discard. Fog `acff`, `abs_light`, bloom extract and gamma become weights. Upstream code stays under `#else`. | `shaders/gen_frag.tmpl`, `light_frag.tmpl`, `bloom.frag`, `gamma.frag` |
| An env vertex shader stores `frag_tex_coord0` in parts. | It stores the whole vector, the same value. | `shaders/gen_vert.tmpl` |
| The pixel parameters hold no depth, so `gl_FragCoord.z` is refused. | The `df` variant writes colour and no depth, and its pipeline masks colour. | `shaders/gen_frag.tmpl` |
| `gl_VertexIndex` is refused, and `gamma.vert` builds the post-process quad from it. | A 4-vertex quad buffer, bound before each post-process draw. The post-process pipelines read it at locations 0 and 2. | `shaders/gamma.vert`, `code/renderervk/vk.c`, `vk.h` |
| `dot.frag` (flare test) writes a storage buffer. | The pack leaves out the dot shaders, and `vk.c` does not create their modules. The dot pipeline was already never made (`fragmentStores` false). | `tools/build_vk_shaders.py`, `code/renderervk/vk.c` |
| `gamma.frag` dither reads `gl_FragCoord` as ints. | The Xenos body has no dither, so `r_dither` has no effect. | `shaders/gamma.frag` |
| glslang was not in the toolchain image. | Debian's `glslang-tools` is added in its own layer after the toolchain. | `docker/xsdk-toolchain/Dockerfile` |

## JPEG

| Difference | Resolution | Files |
|---|---|---|
| Quake3e's JPEG code targets a different client and bundled libjpeg; ioq3 already vendors jpeg-9f. | Adapt `cl_jpeg.c` to ioq3 filesystem and hunk calls, include jpeg-9f, and use `USE_INTERNAL_JPEG`. Build `cl_jpeg.c` and jpeg-9f in both renderer modes; suppress warnings only on jpeg-9f objects. | `code/client/cl_jpeg.c`, `code/thirdparty/jpeg-9f/`, `Makefile` |
| The Quake3e JPEG image loader expects `CL_LoadJPG` through `refimport_t`. | Append and assign all three Quake3e JPEG callbacks under `__XSDK__`. | `code/renderervk/renderercommon/tr_image_jpg.c`, `code/renderercommon/tr_public.h`, `code/client/cl_main.c` |

## Xbox 360 defaults

| Cvar | Default on `__XSDK__` | Reason | Files |
|---|---|---|---|
| Display size | 1280x720, full screen | `VKimp_Init` fixes 1280x720 at 16:9, full screen; `r_renderWidth` and `r_renderHeight` default to 1280x720 so `r_renderScale` keeps 720p. | `code/sys/xenon_vk.c`, `code/renderervk/tr_init.c` |
| `r_ext_multisample` | 0, forced | Images have one sample. Q360-38 revisits MSAA. | `code/renderervk/tr_init.c` |
| `r_ext_supersample` | 0, forced | Doubles the render targets. | `code/renderervk/tr_init.c` |
| `r_bloom` | 0 | Upstream default. The player can enable it; the bloom shaders are in the pack. | `code/renderervk/tr_init.c` |
| `r_hdr` | 0 | Already falls back to the base format (see the vulkan4xenon table). | `code/renderervk/vk.c` |
| `r_picmip` | 0 | Upstream Quake3e default: full-size textures fit the 256 MB heap. | `code/renderervk/tr_init.c` |
| `r_textureMode` and `r_ext_max_anisotropy` | GL_LINEAR_MIPMAP_NEAREST, 8 | Upstream defaults; vulkan4xenon supports anisotropy 1 to 16. | `code/renderervk/tr_init.c` |
| `r_gamma`, `r_overBrightBits`, `r_mapOverBrightBits` | 1, 1, 2 | Upstream: `deviceSupportsGamma` is false, so gamma and overbright apply in the final `r_fbo` pass. | `code/renderervk/tr_init.c`, `code/renderervk/tr_image.c` |
| `r_swapInterval` | 1, registered on `__XSDK__` | FIFO is the only present mode. | `code/renderervk/tr_init.c` |
| GPU memory | Vertex 4 MB, staging 2 MB, image chunks 32 MB | Vertex and staging buffers grow on demand; image chunks stay 32 MB. | `code/renderervk/vk.c` |

2D paths use the world's pipelines:

| Path | Route | Files |
|---|---|---|
| Console, fonts, 2D menus | `re.DrawStretchPic` -> `RE_StretchPic` -> `RB_StretchPic`, drawn through the tess buffer and the same pipelines as world shaders. | `code/renderervk/tr_cmds.c`, `code/renderervk/tr_backend.c` |
| Font data | `tr_font.c` `readInt` builds ints byte by byte and `readFloat` swaps on `Q3_BIG_ENDIAN`. | `code/renderervk/renderercommon/tr_font.c` |
| RoQ cinematics | `cl_cin.c` writes pixels with `LittleLong` (RGBA byte order on big endian); `RE_StretchRaw` -> `RE_UploadCinematic` uploads to the scratch image through `vk_upload_image_data`, then `RE_StretchPic` with `tr.cinematicShader`. | `code/client/cl_cin.c`, `code/renderervk/tr_backend.c` |
