include common.mk

ARCH ?= $(shell arch)
CFG ?= Release
TOOLCHAIN ?= gcc
PREFIX ?= ..

override ARCH := $(call uniq, $(ARCH))
override CFG := $(call uniq,$(CFG))
override TOOLCHAIN := $(foreach i,$(TOOLCHAIN),$(call rremsuffix,:,$(call compress,,$(wordlist 1,3,$(subst :, : ,$i)))))
override TOOLCHAIN := $(foreach i,$(call uniq,$(call firstsep,:,$(TOOLCHAIN))),$(patsubst %:,%,$(lastword $(filter $i:%,$(addsuffix :,$(TOOLCHAIN))))))
override PREFIX := $(lastword $(PREFIX))

$(call var,CFLAGS,gcc% clang%,%,%,-std=c11 -Wall -mavx)
$(call var,CFLAGS LDFLAGS,gcc% clang%,i386 i686,%,-m32)
$(call var,CFLAGS LDFLAGS,gcc% clang%,x86_64,%,-m64)
$(call var,CFLAGS LDFLAGS,gcc% clang%,%,Release,-O3 -flto)
$(call var,CFLAGS,gcc% clang%,%,Debug,-D_DEBUG -g)
$(call var,CFLAGS LDFLAGS,gcc%,%,Release,-fuse-linker-plugin)
$(call var,CFLAGS LDFLAGS,gcc%,%,Debug,-Og)
$(call var,CFLAGS LDFLAGS,clang%,%,Debug,-O0)
$(call var,LDFLAGS,gcc% clang%,%,%,-mavx)
$(call var,LDFLAGS,clang%,%,Release,-fuse-ld=gold)

k := $(call find_var,LDFLAGS clang-8 i686 Release,$(.VARIABLES))

build_var_z = $(eval __tmp := $(call argmskd,1,$(argcnt), ))$(__tmp)
build_var = $(eval $(eval __tmp := $(argcnt)) __tmp := $$(call foreachl,$(call range,1,$(__tmp)),build_var_z,$(call argmsk,1,$(__tmp))))$(__tmp)

TARGET := RegionsMT

$(call foreachl,1 2 3 4,var_base,$(TARGET),$(call firstsep,:,$(TOOLCHAIN)),$(ARCH),$(CFG),$(PREFIX)/$$2/$$3/$$4/$$1)

$(warning $(LDFLAGS:clang%:%:Release))
$(warning $k)