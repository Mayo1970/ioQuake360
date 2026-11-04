FROM debian:trixie

RUN apt-get update && apt-get install -y --no-install-recommends \
	build-essential curl ca-certificates xz-utils bzip2 patch file texinfo bison flex \
	libgmp-dev libmpfr-dev libmpc-dev libisl-dev zlib1g-dev python3 \
	&& rm -rf /var/lib/apt/lists/*

# Default HOME paths, so the SDK Makefile finds the toolchain at $(HOME)/xsdk-gcc unchanged.
ENV PREFIX=/root/xsdk-gcc WORK=/root/xsdk-gcc-work
COPY build.sh *.patch /opt/xsdk-toolchain/
COPY src/ /root/xsdk-gcc-work/src/
RUN bash /opt/xsdk-toolchain/build.sh || (tail -n 60 "$WORK"/logs/*.log; exit 1) \
	&& rm -rf "$WORK"

ENV PATH=/root/xsdk-gcc/bin:$PATH
WORKDIR /app
