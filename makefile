.DEFAULT_GOAL = all
.RECIPEPREFIX +=

ifndef ARCH
ARCH = $(shell arch)
endif

ifndef CFG
CFG = Release
endif

ifndef LIBRARY_PATH
LIBRARY_PATH = ..
endif

ifndef INSTALL_PATH
INSTALL_PATH = ..
endif

CC = gcc
CC_OPT = -std=c11 -flto -fuse-linker-plugin -Wall -mavx
CC_OPT-i386 = -m32
CC_OPT-x86_64 = -m64
CC_OPT--Release = -O3
CC_OPT--Debug = -D_DEBUG -Og -ggdb
CC_INC = gsl

LD = $(CC)
LD_OPT = -flto -fuse-linker-plugin -mavx
LD_OPT-i386 = -m32
LD_OPT-x86_64 = -m64
LD_OPT--Release = -O3
LD_OPT--Debug = -Og
LD_LIB = m pthread gsl gslcblas
LD_INC = gsl

TARGET = RegionsMT
OBJ_DIR = obj
SRC_DIR = src

GATHER_DIR =

rwildcard = $(sort $(wildcard $(addprefix $1,$2))) $(foreach d,$(sort $(wildcard $1*)),$(call rwildcard,$d/,$2))

define gather =
$(if $(findstring $2,$(GATHER_$1)),,$(eval
CLEAN-$2 =
GATHER_$1 += $2
GATHER_CLEAN_$1 += clean-$2
$(eval PARENT-$2 = $(addprefix $(INSTALL_PATH)/,$(filter-out .,$(patsubst %/,%,$(dir $(patsubst $(INSTALL_PATH)/%,%,$2))))))
$(if $(PARENT-$2),
CLEAN-$(PARENT-$2) += clean-$2
$(call gather,DIR,$(PARENT-$2)))))
endef

define build_var =
$(eval 
$(eval
$1 +=
$1-$2 +=
$1--$3 +=
$1-$2-$3 +=)
BUILD_$1-$2-$3 = $(strip $(addsuffix $5,$(addprefix $4,$($1) $($1-$2) $($1--$3) $($1-$2-$3)))))
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
    $(CC) $(BUILD_CC_OPT$1) $(addprefix -I,$(BUILD_CC_INC$1)) -o $$@ -c $$^

.SECONDEXPANSION:
$$(GATHER_TARGET$1): $$(GATHER_OBJ$1) | $$$$(PARENT-$$$$@)
    $(LD) $(BUILD_LD_OPT$1) -o $$@ $$^ $(addprefix -L,$(BUILD_LD_INC$1)) $(addprefix -l,$(BUILD_LD_LIB$1))

.SECONDEXPANSION:
$$(GATHER_CLEAN_OBJ$1) $$(GATHER_CLEAN_TARGET$1): clean-%: | $$$$(CLEAN-%)
    $$(if $$(wildcard $$*),rm $$*))
endef

$(foreach a,$(ARCH),\
$(eval DIR-$a = $(INSTALL_PATH)/$(TARGET)-$a)\
$(foreach c,$(CFG),\
$(eval DIR-$a-$c = $(DIR-$a)/$c)\
$(foreach v,CC_OPT LD_OPT LD_LIB,\
$(call build_var,$v,$a,$c,,))\
$(foreach v,CC_INC LD_INC,\
$(call build_var,$v,$a,$c,$(LIBRARY_PATH)/,-$a/$c))\
$(call build,-$a-$c)))

DIR = $(INSTALL_PATH)/$(TARGET)-x86_64/Release/1/2
$(call gather,DIR,$(DIR))
$(call gather,DIR,$(INSTALL_PATH)/$(TARGET)-x86_64/Release/1/2/123)
$(call gather,DIR,$(INSTALL_PATH)/$(TARGET)-x86_64/Release/1/5)

.PHONY: all
all: $(foreach a,$(ARCH),$(foreach c,$(CFG),$(TARGET-$a-$c)))

.SECONDEXPANSION:
$(GATHER_DIR): | $$(PARENT-$$@); mkdir $@

.PHONY: $(GATHER_CLEAN_DIR)
.SECONDEXPANSION:
$(GATHER_CLEAN_DIR): clean-%: | $$(CLEAN-%); 
    $(if $(wildcard $*),$(if $(wildcard $*/*),,rmdir $*))

.PHONY: clean
clean: | $(foreach f,$(addprefix DIR-,$(ARCH)),clean-$($f));

.PHONY: print-%
print-%:; @echo $* = $($*)
