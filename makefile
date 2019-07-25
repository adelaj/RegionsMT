.DEFAULT_GOAL = all

VALID_ARCH := x86_64 i386 i686
VALID_CONFIG := Debug Release
VALID_TOOLCHAIN := gcc gcc-% clang clang-% icc

TARGET := RegionsMT
OBJ_DIR := obj
SRC_DIR := src

include common.mk
include env.mk
include var.mk
include gather.mk
include build.mk

$(call foreachl,1 2 3,build,$(TARGET),$(TOOLCHAIN),$(ARCH))

.PHONY: all
all: $(call foreachl,2 3 4 5,id,$$($$2/$$3/$$4/$$5),$(TARGET),$(TOOLCHAIN),$(ARCH),$(CFG));

$(call gather_dir)
$(call gather_clean_dir)
$(call gather_clean_file)

.PHONY: clean
clean: | $(call foreachl,2,id,clean-$$(DIR/$$2),$(TOOLCHAIN)) $(call foreachl,2 3 4,id,clean-$$(DIR-$$2/$$3/$$4),$(TARGET),$(TOOLCHAIN),$(ARCH));