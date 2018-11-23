CC = gcc
CC_OPT = -std=c11 -mavx -flto -Wall

LD = $(CC)
LD_OPT = -flto
LD_LIB = m pthread gsl gslcblas

ifeq ($(ARCH),i386)
    CC_OPT += m32
else ifneq ($(ARCH),x86_64)
    ARCH = x86_64
endif

GSL_DIR = ../gsl
GSL_DIR_Release = $(GSL_DIR)-$(ARCH)-Release
GSL_DIR_Debug = $(GSL_DIR)-$(ARCH)-Debug

CC_OPT_Release = $(CC_OPT) -O3
CC_OPT_Debug = $(CC_OPT) -D_DEBUG -mavx -O0 -ggdb
CC_INC_Release = $(GSL_DIR_Release)
CC_INC_Debug = $(GSL_DIR_Debug)

LD_OPT_Release = $(LD_OPT)
LD_OPT_Debug = $(LD_OPT)
LD_INC_Release = $(GSL_DIR_Release)
LD_INC_Debug = $(GSL_DIR_Debug)
LD_LIB_Release = $(LD_LIB)
LD_LIB_Debug = $(LD_LIB)

TARGET_Release = RegionsMT-$(ARCH)-Release
TARGET_Debug = RegionsMT-$(ARCH)-Debug

OBJ_DIR_Release = ./obj-$(ARCH)-Release
OBJ_DIR_Debug = ./obj-$(ARCH)-Debug
SRC_DIR = ./src

CONFIG = Release Debug

rwildcard = $(wildcard $(addsuffix $2,$1)) $(foreach d,$(wildcard $(addsuffix *,$1)),$(call rwildcard,$d/,$2))
struct = $(sort $(patsubst $(SRC_DIR)/%,$($(addprefix OBJ_DIR_,$1))/%,$(dir $(call rwildcard,$(SRC_DIR)/,))))
obj = $(sort $(patsubst $(addprefix $(SRC_DIR)/%,$2),$(addprefix $($(addprefix OBJ_DIR_,$1))/%,$(addsuffix .o,$2)),$(call rwildcard,$(SRC_DIR)/,$(addprefix *,$2))))

.PHONY: all release debug
all: release debug;
release: Release;
debug: Debug;

.PHONY: clean
clean: clean-Release clean-Debug;

define build =
.PHONY: $1
$1: | $(call struct,$1) $($(addprefix TARGET_,$1));
$(call struct,$1):; mkdir -p $$@
$($(addprefix TARGET_,$1)): $(call obj,$1,.c); $(LD) $($(addprefix LD_OPT_, $1)) -o $$@ $$^ $(addprefix -L,$($(addprefix LD_INC_,$1))) $(addprefix -l,$($(addprefix LD_LIB_,$1)))
$(call obj,$1,.c): $($(addprefix OBJ_DIR_,$1))/%.c.o: $(SRC_DIR)/%.c; $(CC) $($(addprefix CC_OPT_,$1)) $(addprefix -I,$($(addprefix CC_INC_,$1))) -o $$@ -c $$^
endef

define clean =
.PHONY: $(addprefix clean-,$1) $(addprefix clean-obj-,$1) $(addprefix clean-target-,$1)
$(addprefix clean-,$1): $(addprefix clean-obj-,$1) $(addprefix clean-target-,$1);
$(addprefix clean-obj-,$1):; rm -rf $($(addprefix OBJ_DIR_,$1))
$(addprefix clean-target-,$1):; rm -f $($(addprefix TARGET_,$1))
endef

$(foreach cfg, $(CONFIG), $(eval $(call build, $(cfg))))
$(foreach cfg, $(CONFIG), $(eval $(call clean, $(cfg))))

.PHONY: print-%
print-% :; @echo $* = $($*)
