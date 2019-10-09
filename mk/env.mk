include mk/common.mk
include mk/gather.mk
#include mk/var.mk

ARCH ?= $(shell uname -m)
CFG ?= Release
TOOLCHAIN ?= gcc
PREFIX ?= ..

override ARCH := $(call uniq, $(ARCH))
override CFG := $(call uniq,$(CFG))
override TOOLCHAIN := $(foreach i,$(TOOLCHAIN),$(call rremsuffix,:,$(call compress,,$(wordlist 1,3,$(subst :, : ,$i)))))
override TOOLCHAIN := $(foreach i,$(call uniq,$(call firstsep,:,$(TOOLCHAIN))),$(patsubst %:,%,$(lastword $(filter $i:%,$(addsuffix :,$(TOOLCHAIN))))))
override PREFIX := $(lastword $(PREFIX))

TARGET := RegionsMT
ALL := $(call var_vect,$(TARGET),$(call firstsep,:,$(TOOLCHAIN)),$(ARCH),$(CFG),$(PREFIX)/$$2/$$1/$$3/$$4)

transform = $(eval __tmp := $$(call __transform,$(subst :,$(COMMA),$(call escape_comma,$2)),$$1))$(__tmp)
__transform = $(eval __tmp := $($(argcnt)))$(__tmp)

M := $(call transform,$$1:$$(lastword $$(subst :, ,$$(filter $$2%,$$(TOOLCHAIN)))):$$3:$$4,$(ALL))

$(warning $(ALL))
$(warning $M)

$(call var,A%,C$(COMMA)$$(COMMA),$$$$(call uniq,$$$$2 $$$$2 $$$$2 ))

$(warning $(call fetch_var,AB:C$(COMMA)$$(COMMA)))