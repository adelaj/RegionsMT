ARCH ?= $(shell uname -m)
CFG ?= Release
TOOLCHAIN ?= gcc
PREFIX ?= ..

override ARCH := $(call uniq,$(ARCH))
override CFG := $(call uniq,$(CFG))
override TOOLCHAIN := $(foreach i,$(TOOLCHAIN),$(call rremsuffix,:,$(call compress,,$(wordlist 1,3,$(subst :, : ,$i)))))
override TOOLCHAIN := $(foreach i,$(call uniq,$(call firstsep,:,$(TOOLCHAIN))),$(patsubst %:,%,$(lastword $(filter $i:%,$(addsuffix :,$(TOOLCHAIN))))))
override PREFIX := $(lastword $(PREFIX))