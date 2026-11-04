#!/usr/bin/env bash

script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)" || exit 1
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

has_jobs=0
for arg in "$@"; do
    case "$arg" in
        -j*) has_jobs=1 ;;
    esac
done

tty_args=()
if [ -t 0 ]; then
    tty_args=(-it)
fi

if [ "$has_jobs" -eq 1 ]; then
    MSYS_NO_PATHCONV=1 docker run --rm "${tty_args[@]}" --volume "${repo_root}:/app" --workdir /app "$image" bash -c 'exec make "$@"' make "$@"
else
    MSYS_NO_PATHCONV=1 docker run --rm "${tty_args[@]}" --volume "${repo_root}:/app" --workdir /app "$image" bash -c 'exec make -j"$(nproc)" "$@"' make "$@"
fi
exit $?
