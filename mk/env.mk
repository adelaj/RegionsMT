ARCH ?= $(shell uname -m)
CFG ?= Release
TOOLCHAIN ?= gcc
PREFIX ?= ..

override ARCH := $(call uniq,$(ARCH))
override CFG := $(call uniq,$(CFG))
override TOOLCHAIN := $(foreach i,$(TOOLCHAIN),$(call rremsuffix,:,$(call compress,,$(wordlist 1,3,$(subst :, : ,$i)))))
override TOOLCHAIN := $(foreach i,$(call uniq,$(call firstsep,:,$(TOOLCHAIN))),$(patsubst %:,%,$(lastword $(filter $i:%,$(addsuffix :,$(TOOLCHAIN))))))
override PREFIX := $(lastword $(PREFIX))

$(foreachl,5,var_base,TOOLCHAIN,$$(firstword $$(subst :, ,$$5)),$$(lastword $$(subst :, ,$$5)),.,$(TOOLCHAIN))

map_toolchain = $(strip $(foreach i,$(TOOLCHAIN),$(call __map_toolchain,$1,$(subst :, ,$i))))
__map_toolchain = $(if $(filter $1,$(firstword $2)),$(lastword $2))