ARCH ?= $(shell arch)
CFG ?= Release
TOOLCHAIN ?= gcc
BUILD_PATH ?= ..

ARCH := $(call uniq,$(filter $(VALID_ARCH),$(ARCH)))
CFG := $(call uniq,$(filter $(VALID_CONFIG),$(CFG)))
TOOLCHAIN := $(call uniq,$(filter $(VALID_TOOLCHAIN),$(call firstsep,/,$(TOOLCHAIN))))
TOOLCHAIN_BASE := $(call firstsep,-,$(TOOLCHAIN))
TOOLCHAIN_VER := $(filter-out $(TOOLCHAIN_BASE),$(TOOLCHAIN))