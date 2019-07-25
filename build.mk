define build_cc =
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

define build_cfg =
$(eval DIR-$1/$2/$3/$4 := $(DIR-$1/$2/$3)/$4) \
$(eval $1/$2/$3/$4 := $$(DIR-$1/$2/$3/$4)/$1) \
$(call build_cc,$1,/$2/$3/$4)
endef

define build =
$(eval DIR-$1/$2/$3 := $(DIR/$2)/$1-$3) \
$(call gather,DIR,$(DIR-$1/$2/$3)) \
$(call foreachl,4,build_cfg,$1,$2,$3,$(CFG))
endef