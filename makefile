.DEFAULT_GOAL = all

VALID_ARCH := x86_64 i386 i686
VALID_CONFIG := Debug Release
VALID_TOOLCHAIN := gcc gcc-% clang clang-% icc

include common.mk
include env.mk
include var.mk
include gather.mk

TARGET := RegionsMT
OBJ_DIR := obj
SRC_DIR := src

GATHER_DIR :=

define build =
$(eval 
$(eval 
TARGET$1 := $(DIR$1)/$(TARGET)
GATHER_TARGET$1 :=
GATHER_OBJ$1 :=)
$(call gather,TARGET$1,$(TARGET$1))
$(foreach o,$(addprefix $(DIR$1)/,$(patsubst $(SRC_DIR)/%,$(OBJ_DIR)/%.o,$(call rwildcard,$(SRC_DIR)/,*.c))),\
$(call gather,OBJ$1,$o))

.SECONDEXPANSION:
$$(GATHER_OBJ$1): $(DIR$1)/$(OBJ_DIR)/%.c.o: $(SRC_DIR)/%.c | $$$$(PARENT-$$$$@)
    $$(strip $(BUILD_CC$1) $(BUILD_CC_OPT$1) $(addprefix -I,$(BUILD_CC_INC$1)) -o $$@ -c $$^)

.SECONDEXPANSION:
$$(GATHER_TARGET$1): $$(GATHER_OBJ$1) | $$$$(PARENT-$$$$@)
    $$(strip $(BUILD_LD$1) $(BUILD_LD_OPT$1) -o $$@ $$^ $(addprefix -L,$(BUILD_LD_INC$1)) $(addprefix -l,$(BUILD_LD_LIB$1)))

GATHER_CLEAN_FILE += $(GATHER_CLEAN_OBJ$1) $(GATHER_CLEAN_TARGET$1))
endef

$(foreach t,$(TOOLCHAIN),\
$(foreach a,$(ARCH),\
$(foreach c,$(CFG),\
$(call build,/$t/$a/$c))))

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

