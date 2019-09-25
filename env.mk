include common.mk
VALID_ARCH := x86_64 i386 i686
VALID_CFG := Debug Release
VALID_TOOLCHAIN := gcc gcc-% clang clang-% icc

ARCH ?= $(shell arch)
CFG ?= Release
TOOLCHAIN ?= gcc
BUILD_PATH ?= ..

cont = $(if $(filter-out $(subst $2, ,$1),$1),,$1)
pred = $(strip $(foreach i,$2,$(if $(eval __tmp := $$(call $$1,$$i$(call argmsku,2,$(argcnt))))$(__tmp),$(__tmp))))

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

m := $(firstword $(filter CXX%,$(.VARIABLES)))
l := gcc-8
n := $(subst %,$(patsubst $(word 2,$(subst $(COL), ,$m)),%,$l),$($m))
t := $(foreach i,a b c %,a$i)

var6 = $(call foreachl,3 4 5 6,feval,$$3$$(COL)$$4$$(COL)$$5$$(COL)$$6 := $$2,$1,$2,$3,$4,$5)

var = $(eval $(eval __tmp := $(argcnt))$$(call foreachl,$(call rangeu,1,$(__tmp)),feval,$$$$2$(call argmskud,2,$(__tmp),$$$$(COL)$$) := $$$$($(call inc,$(__tmp))),$$1$(call argmsku,1,$(__tmp))))


m := $(call rangeu,3,10)

z := $(call var,CC CC1,%,x86_64 i386,Debug Release MinRel,gcc%)