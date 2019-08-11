include common.mk
VALID_ARCH := x86_64 i386 i686
VALID_CFG := Debug Release
VALID_TOOLCHAIN := gcc gcc-% clang clang-% icc

ARCH ?= $(shell arch)
CFG ?= Release
TOOLCHAIN ?= gcc
BUILD_PATH ?= ..

cont = $(if $(filter-out $(subst $2, ,$1),$1),,$1)
pred = $(strip $(foreach i,$2,$(if $(eval __tmp := $$(call $$1,$$i$(call argmsk,2,$(argcnt))))$(__tmp),$(__tmp))))

W := $(call cont,a/b,/) $(call cont,ab/,/) $(call cont,/ab,/)
V := $(call pred,cont,a/b a/ /b x y a/b/c a b,a)
U := $(filter-out ,a/b a/ /b a/b/c)

define toolchain_arrange =
$(eval
$(if $(filter $(VALID_TOOLCHAIN),$1),
DIR/$(call lastsep,:,$1) := 1
))
endef

$(call foreachl,2,feval,$$2 := $$(call uniq,$$(filter $$(VALID_$$2),$$($$2))),ARCH CFG TOOLCHAIN)

TOOLCHAIN_NAME := $(call lastsep,:,$(TOOLCHAIN))

TOOLCHAIN_BASE := $(call firstsep,-,$(TOOLCHAIN))
TOOLCHAIN_VER := $(filter-out $(TOOLCHAIN_BASE),$(TOOLCHAIN))

$(call foreachl,2,feval,DIR/$$2 := $(BUILD_PATH)/$$2,$(TOOLCHAIN))


