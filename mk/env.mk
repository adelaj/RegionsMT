SHELL = bash

ARCH ?= $(shell uname -m)
CFG ?= Release
TOOLCHAIN ?= gcc
PREFIX ?= ..
CLEAN_GROUP ?= 1

CC_TOOLCHAIN := gcc gcc-% clang clang-% icc
MSVC_TOOLCHAIN := msvc
CC_ARCH := i386 i686 x86_64
MSVC_ARCH := Win32 x64 

map_add = $(call __map_add,$1,$(subst :, :,$2))
__map_add = $(call __map_def,$1,$(firstword $2),$(call nofirstword,$2))
__map_def = $(call var_base,$(if $3,$(patsubst :%,%,$(call compress,,$3)),$2),,$1:$2)$2

override ARCH := $(call uniq,$(ARCH))
override CFG := $(call uniq,$(CFG))
override TOOLCHAIN := $(call vect,2,TOOLCHAIN,$(TOOLCHAIN),map_add)
override PREFIX := $(lastword $(PREFIX))
