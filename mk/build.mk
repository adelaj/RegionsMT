define build_cc =
$(eval
$(eval
GATHER_TARGET$$(COL)$1 :=
GATHER_OBJ$$(COL)$1 :=)
$(call gather,TARGET:$1,$($1))
$(call foreachl,3,proxycall,gather,OBJ:$1$(COMMA)$$3,$(addprefix $(DIR-$1$2)/,$(patsubst $(PREFIX)/src/%,$(dir $($1))/obj/%.o,$(call rwildcard,$(PREFIX)/src/,*.c))))

.SECONDEXPANSION:
$$(GATHER_OBJ$$(COL)$1): $(dir $($1))/obj/%.c.o: $(PREFIX)/src/%.c | $$$$(PARENT$$$$(COL)$$$$@)
    $(BUILD_CC$2) $(BUILD_CC_OPT$2) $(addprefix -I,$(BUILD_CC_INC$2)) -o $$@ -c $$^

.SECONDEXPANSION:
$$(GATHER_TARGET-$1$2): $(GATHER_OBJ-$1$2) | $$$$(PARENT-$$$$@)
    $(BUILD_LD$2) $(BUILD_LD_OPT$2) -o $$@ $$^ $(addprefix -L,$(BUILD_LD_INC$2)) $(addprefix -l,$(BUILD_LD_LIB$2))

GATHER_CLEAN_FILE += $(GATHER_CLEAN_OBJ:$1) $(GATHER_CLEAN_TARGET-$1$2))
endef

define build_cc =
$(eval DIR-$1/$2/$3/$4 := $(DIR-$1/$2/$3)/$4) \
$(eval $1/$2/$3/$4 := $$(DIR-$1/$2/$3/$4)/$1) \
$(call build_cc_impl,$1,/$2/$3/$4)
endef

define build_cc_cfg =
$(eval DIR-$1/$2/$3 := $(DIR/$2)/$1-$3) \
$(call gather,DIR,$(DIR-$1/$2/$3)) \
$(call foreachl,4,build_cc,$1,$2,$3,$4)
endef