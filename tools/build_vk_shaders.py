#!/usr/bin/env python3
"""Builds renderervk's shader pack for vulkan4xenon: each variant compile.bat makes, through glslang and xost
(vulkan4xenon/tools/build_shaders.py), into the C source and header vk.c includes on __XSDK__. Run from the repo root."""

import argparse
import re
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "vulkan4xenon" / "tools"))
import build_shaders  # noqa: E402

SHADERS = Path("code/renderervk/shaders")
WORK = Path("build/vk_shaders")
# compile.bat's template lines: glslang's stage, template and defines, then bin2hex's array name.
TEMPLATE_LINE = re.compile(r'^"%cl%" -S (vert|frag) -V -o "%tmpf%" (\S+)((?: -D\w+)*)\s*\n"%bh%" "%tmpf%" %outf% (\w+)',
                           re.MULTILINE)


def variants():
    """(name, stage, source, defines) for each shader shader_data.c holds, under its array's name."""
    # The flare test's dot shaders need fragment stores, which vulkan4xenon lacks; vk.c skips them on __XSDK__.
    found = [(f"{path.stem}_{path.suffix[1:]}_spv", path.suffix[1:], path, [])
             for suffix in ("*.vert", "*.frag") for path in sorted(SHADERS.glob(suffix)) if path.stem != "dot"]
    for stage, template, defines, name in TEMPLATE_LINE.findall((SHADERS / "compile.bat").read_text()):
        found.append((name, stage, SHADERS / template, [d[2:] for d in defines.split()]))
    return found


def wrapped(name, stage, source, defines):
    """The source with its defines after #version, in a file glslang takes the stage from."""
    version, body = source.read_text().split("\n", 1)
    path = WORK / f"{name}.{stage}"
    lines = [f"#define {d}\n" for d in ["Q360_XENOS"] + defines]
    path.write_text(version + "\n" + "".join(lines) + body, newline="\n")
    return path


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--xost", type=Path, required=True, help="xost's folder (x360-sdk/xenos-toolchain)")
    parser.add_argument("--glslang", type=Path, default=Path("glslangValidator"))
    parser.add_argument("--output", type=Path, required=True, help="the C source and header to write, less suffix")
    parser.add_argument("--optimize", action="store_true", help="have xost rewrite each program's instructions first")
    args = parser.parse_args()
    WORK.mkdir(parents=True, exist_ok=True)
    shaders, refused = [], []
    with tempfile.TemporaryDirectory(dir="build") as directory:
        for name, stage, source, defines in variants():
            spirv = build_shaders.compile_spirv(args.glslang, wrapped(name, stage, source, defines), directory)
            # The instanced variant of a vertex shader, as build_shaders.py builds it, for pipelines with one.
            for instanced in (False, True) if stage == "vert" else (False,):
                try:
                    container, literals, resources, clip_planes, specializations = build_shaders.lower(
                        args.xost.resolve(), spirv, name, args.optimize, instanced)
                except SystemExit as error:
                    refused.append(str(error))
                    break
                shaders.append({"name": name + ("_instanced" if instanced else ""), "spirv": spirv,
                                "hash": build_shaders.fnv1a_64(spirv), "container": container, "literals": literals,
                                "resources": resources, "clip_planes": clip_planes,
                                "specializations": specializations, "instanced": instanced})
                if any(resource[0] == "instance" for resource in resources):
                    break
    if refused:
        sys.exit("xost refused these, so none is written:\n" + "\n".join(refused))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    build_shaders.write_outputs(args.output, shaders, [source for _, _, source, _ in variants()],
                                build_shaders.DEFAULT_BUNDLE)
    print(f"{args.output}: {len(shaders)} shaders")


if __name__ == "__main__":
    main()
