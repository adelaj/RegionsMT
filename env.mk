ifndef ARCH
ARCH = $(shell arch)
endif

ifndef CFG
CFG = Release
endif

ifndef TOOLCHAIN
TOOLCHAIN = gcc
endif

ifndef BUILD_PATH
BUILD_PATH = ..
endif

ARCH := $(call uniq,$(filter $(VALID_ARCH),$(ARCH)))
CFG := $(call uniq,$(filter $(VALID_CONFIG),$(CFG)))
TOOLCHAIN := $(call uniq,$(filter $(VALID_TOOLCHAIN),$(call firstsep,/,$(TOOLCHAIN))))
TOOLCHAIN_BASE := $(call firstsep,-,$(TOOLCHAIN))
TOOLCHAIN_VER := $(filter-out $(TOOLCHAIN_BASE),$(TOOLCHAIN))