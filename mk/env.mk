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


$(call var,A%,C$(COMMA)$$(COMMA),$$$$2)

$(warning $(call fetch_var,AB:C$(COMMA)$$(COMMA)))