ARCH ?= $(shell uname -m)
CFG ?= Release
TOOLCHAIN ?= gcc
PREFIX ?= ..

toolchain_add = $(call __toolchain_add,$1,$(subst :, :,$2))
__toolchain_add = $(call __toolchain_def,$1,$(firstword $2),$(call nofirstword,$2))
__toolchain_def = $(call var_base,$1,$2,$(if $3,$(patsubst :%,%,$(call compress,,$3)),$2),:=)$2

maybe = $(if $1,$1,$2)

override ARCH := $(call uniq,$(ARCH))
override CFG := $(call uniq,$(CFG))
override TOOLCHAIN := $(call foreachl,2,toolchain_add,TOOLCHAIN,$(TOOLCHAIN))
override PREFIX := $(lastword $(PREFIX))
override CLEAN := $(call maybe,$(filter $(call range,1,3),$(CLEAN)),1)