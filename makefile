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

define build =
$(eval
$(eval
TARGET$1 := $(DIR$1)/$(TARGET)
GATHER_TARGET$1 :=
GATHER_OBJ$1 :=)
$(call gather,TARGET$1,$(TARGET$1))
$(foreach o,$(addprefix $(DIR$1)/,$(patsubst $(SRC_DIR)/%,$(OBJ_DIR)/%.o,$(call rwildcard,$(SRC_DIR)/,*.c))),$(call gather,OBJ$1,$o))

.SECONDEXPANSION:
$$(GATHER_OBJ$1): $(DIR$1)/$(OBJ_DIR)/%.c.o: $(SRC_DIR)/%.c | $$$$(PARENT-$$$$@)
    $(BUILD_CC$1) $(BUILD_CC_OPT$1) $(addprefix -I,$(BUILD_CC_INC$1)) -o $$@ -c $$^

.SECONDEXPANSION:
$$(GATHER_TARGET$1): $(GATHER_OBJ$1) | $$$$(PARENT-$$$$@)
    $(BUILD_LD$1) $(BUILD_LD_OPT$1) -o $$@ $$^ $(addprefix -L,$(BUILD_LD_INC$1)) $(addprefix -l,$(BUILD_LD_LIB$1))

GATHER_CLEAN_FILE += $(GATHER_CLEAN_OBJ$1) $(GATHER_CLEAN_TARGET$1))
endef

GATHER_CLEAN_FILE :=

$(call foreachl,2,feval,DIR/$$2 := $(BUILD_PATH)/$$2,$(TOOLCHAIN))
$(call foreachl,2 3 4,feval,DIR/$$2/$$3/$$4 := $$(DIR/$$2)/$(TARGET)-$$3/$$4,$(TOOLCHAIN),$(ARCH),$(CFG))
$(call foreachl,3 4 5,proxy,build,/$$3/$$4/$$5,$(TOOLCHAIN),$(ARCH),$(CFG))

.PHONY: all
all: $(call foreachl,2 3 4,id,$$(TARGET/$$2/$$3/$$4),$(TOOLCHAIN),$(ARCH),$(CFG));

$(call gather_dir)
$(call gather_clean_dir)
$(call gather_clean_file)

.PHONY: clean
clean: | $(call foreachl,2,id,clean-$$(DIR/$$2),$(TOOLCHAIN));

# to do: add treatment for bad config/arch/toolchain 