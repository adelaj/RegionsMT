BUILD_CC_TOOLCHAIN := gcc gcc-% clang clang-% icc

define build_cc_base =
$(call var_base,BUILD_CC_QUERY,$1,$(call transform,$$1:$$(lastword $$(subst :, ,$$(filter $$2%,$$(TOOLCHAIN)))):$$3:$$4,$1),0)\
$(call var_base,BUILD_CC_TOOLCHAIN,$1,$$(word 2,$$(subst :, ,$$(BUILD_CC_QUERY:$$2))),0)\
$(if $(filter $(BUILD_CC_TOOLCHAIN),$(BUILD_CC_TOOLCHAIN:$1)),$(eval
$(call var_base,BUILD_CC_BASE,$1,$$(PREFIX)/$$(firstword $$(subst :, ,$$2))/,0)

$(call gather,BUILD_CC_TARGET:$1,$($1))
$(call foreachl,2,gather,BUILD_CC_DEP:$1,$(patsubst $(BUILD_CC_BASE:$1)src/%,$(dir $($1))obj/%.d,$(call fetch_var,CSRC:$(BUILD_CC_QUERY:$1))))
$(call foreachl,2,gather,BUILD_CC_OBJ:$1,$(patsubst %.d,%.o,$(GATHER_BUILD_CC_DEP:$1)))

.SECONDEXPANSION:
$$(GATHER_BUILD_CC_OBJ$$(COL)$$1): $$(dir $$($$1))obj/%.c.o: $$(BUILD_CC_BASE$$(COL)$$1)src/%.c $$(dir $$($$1))obj/%.c.d | $$$$(PARENT$$$$(COL)$$$$@) $$(call fetch_var2,CINC:$$(BUILD_CC_QUERY$$(COL)$$1),CINC:$$1)
    $$(strip $$(call fetch_var,CC:$(BUILD_CC_TOOLCHAIN:$1)) -MMD -MP -MF$$(word 2,$$^) $(call fetch_var,CFLAGS:$(BUILD_CC_QUERY:$1)) $$(addprefix -I,$$(word 2,$$|)) -o $$@ -c $$<)

$$(GATHER_BUILD_CC_DEP$$(COL)$$1):;

$(call var_base,GATHER_INC,$(GATHER_BUILD_CC_DEP:$1),0)
$(call var_base,GATHER_FILE,$(GATHER_BUILD_CC_OBJ:$1),0)
$(call var_base,GATHER_CLEAN_FILE,$(GATHER_CLEAN_BUILD_CC_TARGET:$1) $(GATHER_CLEAN_BUILD_CC_OBJ:$1) $(GATHER_CLEAN_BUILD_CC_DEP:$1),0)))
endef

build_cc = $(call foreachl,1,build_cc_base,$1)
