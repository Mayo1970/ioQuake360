#!/usr/bin/env bash
# Runs make with the given arguments in the toolchain container, with the repo root at /app.

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)" || exit 1
# Git Bash: pwd -W gives the Windows path Docker Desktop mounts; MSYS_NO_PATHCONV stops /app being rewritten.
repo_root="$(cd -- "$script_dir/.." && { pwd -W 2>/dev/null || pwd -P; })" || exit 1
image="${Q360_IMAGE:-ioquake360/xsdk-toolchain}"

if ! command -v docker >/dev/null 2>&1; then
    printf 'Error: docker was not found. Once Docker is available, run: docker build -t ioquake360/xsdk-toolchain docker/xsdk-toolchain\n' >&2
    exit 1
fi

if ! MSYS_NO_PATHCONV=1 docker image inspect "$image" >/dev/null 2>&1; then
    printf 'Error: Docker image %s is missing or unavailable. Run: docker build -t ioquake360/xsdk-toolchain docker/xsdk-toolchain\n' "$image" >&2
    exit 1
fi

jobs='-j"$(nproc)"'
for arg in "$@"; do
    case "$arg" in
        -j*|--jobs*) jobs= ;;
    esac
done

# No -it: make reads no input, and mintty's pty is not a TTY to docker.exe.
MSYS_NO_PATHCONV=1 docker run --rm --volume "${repo_root}:/app" --workdir /app "$image" \
    bash -c "exec make $jobs \"\$@\"" make "$@"
