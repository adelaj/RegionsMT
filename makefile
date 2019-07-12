.DEFAULT_GOAL = all
.RECIPEPREFIX +=

ifndef ARCH
ARCH = $(shell arch)
endif

ifndef CFG
CFG = Release
endif

ifndef BUILD_PATH
BUILD_PATH = ..
endif

ifndef TOOLCHAIN
TOOLCHAIN = gcc
endif

CC =
CC/gcc = gcc
CC/clang = clang
CC_OPT = -std=c11 -Wall -mavx
CC_OPT/*/i386 = -m32
CC_OPT/*/x86_64 = -m64
CC_OPT/*/*/Release = -O3 -flto
CC_OPT/*/*/Debug = -D_DEBUG -g
CC_OPT/gcc/*/Release = -fuse-linker-plugin
CC_OPT/gcc/*/Debug = -Og
CC_OPT/clang/*/Debug = -O0
CC_INC = gsl

LD =
LD/gcc = $(CC/gcc)
LD/clang = $(CC/clang)
LD_OPT/*/i386 = -m32
LD_OPT/*/x86_64 = -m64
LD_OPT/*/*/Release = -mavx -O3 -flto
LD_OPT/gcc/*/Release = -fuse-linker-plugin
LD_OPT/gcc/*/Debug = -Og
LD_OPT/clang/*/Release = -fuse-ld=gold
LD_OPT/clang/*/Debug = -O0
LD_LIB = m pthread gsl gslcblas
LD_INC = gsl

TARGET = RegionsMT
OBJ_DIR = obj
SRC_DIR = src

GATHER_DIR =

rwildcard = $(sort $(wildcard $(addprefix $1,$2))) $(foreach d,$(sort $(wildcard $1*)),$(call rwildcard,$d/,$2))

define gather =
$(if $(filter $2,$(GATHER_$1)),,
$(eval
CLEAN-$2 =
GATHER_$1 += $2
GATHER_CLEAN_$1 += clean-$2
$(eval 
PARENT-$2 = $(addprefix $(BUILD_PATH)/,$(filter-out .,$(patsubst %/,%,$(dir $(patsubst $(BUILD_PATH)/%,%,$2))))))
$(if $(PARENT-$2),
CLEAN-$(PARENT-$2) += clean-$2
$(call gather,DIR,$(PARENT-$2)))))
endef

define toolset_var =
$(eval
$(eval 
$1/$3 +=)
BUILD_$1/$2 = $(strip $($1/$3))$4)
endef

define build_var =
$(eval
$(eval
$1 +=
$1/$5 +=
$1/*/$3 +=
$1/$5/$3 +=
$1/*/*/$4 +=
$1/*/$3/$4 +=
$1/$5/*/$4 +=
$1/$5/$3/$4 +=
$1/$2 +=
$1/$2/$3 +=
$1/$2/*/$4 +=
$1/$2/$3/$4 +=)
BUILD_$1/$2/$3/$4 = $(addsuffix $7,$(addprefix $6,\
$($1) $($1/$2) $($1/*/$3) $($1/$2/$3) $($1/*/*/$4) $($1/$2/*/$4) $($1/*/$3/$4) $($1/$2/$3/$4)\
$(if $(filter-out $5,$2),$($1/$5) $($1/$5/$3) $($1/$5/*/$4) $($1/$5/$3/$4)))))
endef

define build =
$(eval 
$(eval 
TARGET$1 = $(DIR$1)/$(TARGET)
GATHER_TARGET$1 =
GATHER_OBJ$1 =)
$(call gather,TARGET$1,$(TARGET$1))
$(foreach o,$(addprefix $(DIR$1)/,$(patsubst $(SRC_DIR)/%,$(OBJ_DIR)/%.o,$(call rwildcard,$(SRC_DIR)/,*.c))),\
$(call gather,OBJ$1,$o))

.SECONDEXPANSION:
$$(GATHER_OBJ$1): $(DIR$1)/$(OBJ_DIR)/%.c.o: $(SRC_DIR)/%.c | $$$$(PARENT-$$$$@)
    $$(strip $(BUILD_CC$2) $(BUILD_CC_OPT$1) $(addprefix -I,$(BUILD_CC_INC$1)) -o $$@ -c $$^)

.SECONDEXPANSION:
$$(GATHER_TARGET$1): $$(GATHER_OBJ$1) | $$$$(PARENT-$$$$@)
    $$(strip $(BUILD_LD$2) $(BUILD_LD_OPT$1) -o $$@ $$^ $(addprefix -L,$(BUILD_LD_INC$1)) $(addprefix -l,$(BUILD_LD_LIB$1)))

GATHER_CLEAN_FILE += $(GATHER_CLEAN_OBJ$1) $(GATHER_CLEAN_TARGET$1))
endef

$(foreach t,$(TOOLCHAIN),\
$(eval p = $(firstword $(subst -, ,$t)))\
$(foreach v,CC LD,$(call toolset_var,$v,$t,$p,$(patsubst $p%,%,$t)))\
$(eval DIR/$t = $(BUILD_PATH)/$t)\
$(foreach a,$(ARCH),\
$(eval DIR/$t/$a = $(DIR/$t)/$(TARGET)-$a)\
$(foreach c,$(CFG),\
$(eval DIR/$t/$a/$c = $(DIR/$t/$a)/$c)\
$(foreach v,CC_OPT LD_OPT LD_LIB,\
$(call build_var,$v,$t,$a,$c,$p,,))\
$(foreach v,CC_INC LD_INC,\
$(call build_var,$v,$t,$a,$c,$p,$(DIR/$t)/,-$a/$c))\
$(call build,/$t/$a/$c,/$t))))

.PHONY: all
all: $(foreach t,$(TOOLCHAIN),$(foreach a,$(ARCH),$(foreach c,$(CFG),$(TARGET/$t/$a/$c))))

.SECONDEXPANSION:
$(GATHER_DIR): | $$(PARENT-$$@); mkdir $@

.PHONY: $(GATHER_CLEAN_DIR)
.SECONDEXPANSION:
$(GATHER_CLEAN_DIR): clean-%: | $$(CLEAN-%)
    $(if $(wildcard $*),$(if $(wildcard $*/*),,rmdir $*))

.SECONDEXPANSION:
$(GATHER_CLEAN_FILE): clean-%: | $$(CLEAN-%)
    $(if $(wildcard $*),rm $*)

.PHONY: clean
clean: | $(foreach t,$(TOOLCHAIN),clean-$(DIR/$t));

.PHONY: print-%
print-%:; @echo $* = $($*)
