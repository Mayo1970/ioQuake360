# Shared flavor selection for q3, Team Arena, and Open Arena.

ifneq ($(filter ta,$(MAKECMDGOALS)),)
  TA := 1
endif
ifneq ($(filter oa,$(MAKECMDGOALS)),)
  OA := 1
endif

ifeq ($(TA),1)
  TITLE         := Team Arena
  TARGET        := ioquake360_ta
  BUILD         := build_ta
  DEFINES_EXTRA := -DSTANDALONETA -DMISSIONPACK
else ifeq ($(OA),1)
  TITLE         := Open Arena
  TARGET        := ioquake360_oa
  BUILD         := build_oa
  DEFINES_EXTRA := -DSTANDALONEOA
else
  TITLE         := ioQuake3
  TARGET        := ioquake360
  BUILD         := build
  DEFINES_EXTRA :=
endif

.DEFAULT_GOAL := all

.PHONY: ta oa
ta: all
oa: all
