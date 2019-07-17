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

define foreach3 =
$(foreach a,$2,\
$(foreach b,$3,\
$(foreach c,$4,\
$(eval $$(call $$1,$$a,$$b,$$c$5)))))
endef

define foreach4 =
$(foreach a,$2,\
$(foreach b,$3,\
$(foreach c,$4,\
$(foreach d,$5,\
$(call $1,$a,$b,$c,$d)))))
endef

define var3_autocomplete =
$(eval 
$(call rremsuffix,/*,$1/$2/$3/$4) +=)
endef

define var3_set_o =
$(foreach t,$2 *,\
$(foreach a,$3 *,\
$(foreach c,$4 *,\
$(eval
BUILD_$1/$2/$3/$4 += $($(call rremsuffix,/*,$1/$t/$a/$c))
BUILD += $1/$2/$3/$4
))))
endef

define var3_append =
BUILD_$4/$5/$6/$7 += $($(call rremsuffix,/*,$4/$1/$2/$3))
BUILD += $4/$5/$6/$7
endef

define var3_set =
$(call foreach3 var3_append,$2 *,$3 *,$4 *,$(COMMA)$$$$6$(COMMA)$$$$7$(COMMA)$$$$8$(COMMA)$$$$9,$1,$2,$3,$4)
endef


define var3_copy =
$(eval
BUILD_$1/$2/$3/$4 += $(BUILD_$1/$(firstword $(subst -, ,$2))/$3/$4))
endef

define var3_last =
$(eval
BUILD_$1/$2/$3/$4 := $(lastword BUILD_$1/$2/$3/$4))
endef

EXEC := CC CXX LD AR
INC := CC_INC LD_INC
VAR := $(EXEC) $(INC) CC_OPT CC_INC LD_OPT LD_LIB 

$(call foreach4,var3_autocomplete,$(VAR),$(TOOLCHAIN_BASE) *,$(ARCH) *,$(CFG) *)
$(call foreach4,var3_set,$(VAR),$(TOOLCHAIN_BASE),$(ARCH),$(CFG))
$(call foreach4,var3_copy,$(VAR),$(TOOLCHAIN_VER),$(ARCH),$(CFG))
$(call foreach4,var3_last,$(EXEC),$(TOOLCHAIN),$(ARCH),$(CFG))


