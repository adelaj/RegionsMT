.DEFAULT_GOAL = all

VALID_ARCH := x86_64 i386 i686
VALID_CONFIG := Debug Release
VALID_TOOLCHAIN := gcc gcc-% clang clang-% icc

TARGET := RegionsMT
OBJ_DIR := obj
SRC_DIR := src

include common.mk
include env.mk
include var.mk
include gather.mk

define build =
$(eval
$(eval
GATHER_TARGET-$1$2 :=
GATHER_OBJ-$1$2 :=)
$(call gather,TARGET-$1$2,$($1$2))
$(call foreachl,3,proxycall,gather,OBJ-$1$2$(COMMA)$$3,$(addprefix $(DIR-$1$2)/,$(patsubst $(SRC_DIR)/%,$(OBJ_DIR)/%.o,$(call rwildcard,$(SRC_DIR)/,*.c))))

.SECONDEXPANSION:
$$(GATHER_OBJ-$1$2): $(DIR-$1$2)/$(OBJ_DIR)/%.c.o: $(SRC_DIR)/%.c | $$$$(PARENT-$$$$@)
    $(BUILD_CC$2) $(BUILD_CC_OPT$2) $(addprefix -I,$(BUILD_CC_INC$2)) -o $$@ -c $$^

.SECONDEXPANSION:
$$(GATHER_TARGET-$1$2): $(GATHER_OBJ-$1$2) | $$$$(PARENT-$$$$@)
    $(BUILD_LD$2) $(BUILD_LD_OPT$2) -o $$@ $$^ $(addprefix -L,$(BUILD_LD_INC$2)) $(addprefix -l,$(BUILD_LD_LIB$2))

GATHER_CLEAN_FILE += $(GATHER_CLEAN_OBJ-$1$2) $(GATHER_CLEAN_TARGET-$1$2))
endef

$(call foreachl,2 3 4,feval,DIR-$$2/$$3/$$4 := $$(DIR/$$3)/$$2-$$3,$(TARGET),$(TOOLCHAIN),$(ARCH))
$(call foreachl,3 4 5,proxycall,gather,DIR$(COMMA)$$(DIR-$$3/$$4/$$5),$(TARGET),$(TOOLCHAIN),$(ARCH))
$(call foreachl,2 3 4 5,feval,DIR-$$2/$$3/$$4/$$5 := $$(DIR-$$2/$$3/$$4)/$$5,$(TARGET),$(TOOLCHAIN),$(ARCH),$(CFG))
$(call foreachl,2 3 4 5,feval,$$2/$$3/$$4/$$5 := $$(DIR-$$2/$$3/$$4/$$5)/$$2,$(TARGET),$(TOOLCHAIN),$(ARCH),$(CFG))
$(call foreachl,3 4 5 6,proxycall,build,$$3$(COMMA)/$$4/$$5/$$6,$(TARGET),$(TOOLCHAIN),$(ARCH),$(CFG))

.PHONY: all
all: $(call foreachl,2 3 4 5,id,$$($$2/$$3/$$4/$$5),$(TARGET),$(TOOLCHAIN),$(ARCH),$(CFG));

$(call gather_dir)
$(call gather_clean_dir)
$(call gather_clean_file)

.PHONY: clean
clean: | $(call foreachl,2,id,clean-$$(DIR/$$2),$(TOOLCHAIN)) $(call foreachl,2 3 4,id,clean-$$(DIR-$$2/$$3/$$4),$(TARGET),$(TOOLCHAIN),$(ARCH));