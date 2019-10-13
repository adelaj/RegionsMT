BUILD_CC_TOOLCHAIN := gcc gcc-% clang clang-% icc

define build_cc_base =
$(call var_base,BUILD_CC_QUERY,$1,$(call transform,$$1:$$(lastword $$(subst :, ,$$(filter $$2%,$$(TOOLCHAIN)))):$$3:$$4,$1),0)\
$(call var_base,BUILD_CC_TOOLCHAIN,$1,$$(word 2,$$(subst :, ,$$(BUILD_CC_QUERY:$$2))),0)\
$(if $(filter $(BUILD_CC_TOOLCHAIN),$(BUILD_CC_TOOLCHAIN:$1)),$(eval
$(call var_base,BUILD_CC_BASE,$1,$$(PREFIX)/$$(firstword $$(subst :, ,$$2))/,.)
$(call var_base,BUILD_CC,$1,$$(call fetch_var,CC:$$(BUILD_CC_TOOLCHAIN$$(COL)$$2)),.)
$(call var_base,BUILD_CC_CFLAGS,$1,$$(call fetch_var,CFLAGS:$$(BUILD_CC_QUERY$$(COL)$$2)),.)
$(call var_base,BUILD_CC_CINC,$1,$$(call fetch_var2,CINC$$(COL)$$(BUILD_CC_QUERY$$(COL)$$2),CINC$$(COL)$$2),.)
$(call gather,BUILD_CC_GATHER_TARGET:$1,$($1))
$(call foreachl,2,gather,BUILD_CC_GATHER_DEP:$1,$(patsubst $(BUILD_CC_BASE:$1)src/%,$(dir $($1))depend/%.mk,$(call fetch_var,CSRC:$(BUILD_CC_QUERY:$1))))
$(call foreachl,2,gather,BUILD_CC_GATHER_OBJ:$1,$(patsubst $(dir $($1))depend/%.mk,$(dir $($1))obj/%.o,$(BUILD_CC_GATHER_DEP:$1)))

.SECONDEXPANSION:
$$(BUILD_CC_GATHER_OBJ$$(COL)$$1): $$(dir $$($$1))obj/%.c.o: $$(BUILD_CC_BASE$$(COL)$$1)src/%.c $$(dir $$($$1))depend/%.c.mk | $$$$(PARENT$$$$(COL)$$$$@) $$(BUILD_CC_CINC:$$1)
    $$(strip $(BUILD_CC:$1) -MMD -MP -MF$$(word 2,$$^) $(BUILD_CC_CFLAGS:$1) $$(addprefix -I,$$(word 2,$$|)) -o $$@ -c $$<)

$$(BUILD_CC_GATHER_DEP$$(COL)$$1): | $$$$(PARENT$$$$(COL)$$$$@);

$$(BUILD_CC_GATHER_TARGET$$(COL)$$1):;

$$(BUILD_CC_CINC:$$1):;

$(call var_base,GATHER_INC,$(BUILD_CC_GATHER_DEP:$1),0)
$(call var_base,GATHER_FILE,$(BUILD_CC_GATHER_TARGET:$1) $(BUILD_CC_GATHER_OBJ:$1) $(BUILD_CC_GATHER_DEP:$1),0)))
endef

build_cc = $(call foreachl,1,build_cc_base,$1)
