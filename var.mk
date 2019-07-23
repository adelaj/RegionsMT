VALID_ARCH := x86_64 i386 i686
VALID_CONFIG := Debug Release
VALID_TOOLCHAIN := gcc gcc-% clang clang-% icc

include common.mk
include env.mk

CC/gcc := gcc
CC/clang := clang

CXX/gcc := g++
CXX/clang := clang++

LD/gcc := $(CC/gcc)
LD/clang := $(CC/clang)

AR/gcc := gcc-ar
AR/clang := llvm-ar

CC_OPT := -std=c11 -Wall -mavx
CC_OPT/*/i386 := -m32
CC_OPT/*/i686 := $(CC_OPT/*/i386)
CC_OPT/*/x86_64 := -m64
CC_OPT/*/*/Release := -O3 -flto
CC_OPT/*/*/Debug := -D_DEBUG -g
CC_OPT/gcc/*/Release := -fuse-linker-plugin
CC_OPT/gcc/*/Debug := -Og
CC_OPT/clang/*/Debug := -O0

CC_INC := gsl

LD_OPT/*/i386 := -m32
LD_OPT/*/i686 := $(LD_OPT/*/i386)
LD_OPT/*/x86_64 := -m64
LD_OPT/*/*/Release := -mavx -O3 -flto
LD_OPT/gcc/*/Release := -fuse-linker-plugin
LD_OPT/gcc/*/Debug := -Og
LD_OPT/clang/*/Release := -fuse-ld=gold
LD_OPT/clang/*/Debug := -O0

LD_INC := gsl
LD_LIB := m pthread gsl gslcblas

define var3_set = 
$(eval $1 :=) \
$(call foreachl,6 7 8,feval,$1 += $$($$(call rremsuffix,/*,$2/$$6/$$7/$$8)),$2,$3,$4,$5,$3 *,$4 *,$5 *) \
$(eval $1 := $$(strip $$($1)))
endef

EXEC := CC CXX LD AR
INC := CC_INC LD_INC
VAR := $(EXEC) $(INC) CC_OPT LD_OPT LD_LIB

$(call foreachl,2 3 4 5,feval,$$(call rremsuffix,/*,$$2/$$3/$$4/$$5) ?=,$(VAR),$(TOOLCHAIN_BASE) *,$(ARCH) *,$(CFG) *)
$(call foreachl,2 3 4 5,var3_set,BUILD_$$2/$$3/$$4/$$5,$(VAR),$(TOOLCHAIN_BASE),$(ARCH),$(CFG))
$(call foreachl,2 3 4 5,feval,BUILD_$$2/$$3/$$4/$$5 := $$(BUILD_$$2/$$(firstword $$(subst -, ,$$3))/$$4/$$5),$(VAR),$(TOOLCHAIN_VER),$(ARCH),$(CFG))
$(call foreachl,2 3 4 5,feval,BUILD_$$2/$$3/$$4/$$5 := $$(firstword $$(BUILD_$$2/$$3/$$4/$$5)),$(EXEC),$(TOOLCHAIN),$(ARCH),$(CFG))
$(call foreachl,2 3 4 5,feval,BUILD_$$2/$$3/$$4/$$5 := $$(addsuffix -$$4/$$5,$$(addprefix $(BUILD_PATH)/$$3/,$$(BUILD_$$2/$$3/$$4/$$5))),$(INC),$(TOOLCHAIN),$(ARCH),$(CFG))
$(call foreachl,2 3 4 5,feval,BUILD_$$2/$$3/$$4/$$5 := $$(addsuffix $$(patsubst $$(firstword $$(subst -, ,$$3))%,%,$$3),$$(BUILD_$$2/$$3/$$4/$$5)),$(EXEC),$(TOOLCHAIN_VER),$(ARCH),$(CFG))
