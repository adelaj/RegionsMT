define gather =
$(eval
$(if $(filter $2,$(GATHER_$1)),,
CLEAN-$2 :=
GATHER_$1 += $2
GATHER_CLEAN_$1 += clean-$2
$(eval 
PARENT-$2 := $(addprefix $(BUILD_PATH)/,$(filter-out .,$(patsubst %/,%,$(dir $(patsubst $(BUILD_PATH)/%,%,$2))))))
$(if $(PARENT-$2),
CLEAN-$(PARENT-$2) += clean-$2
$(call gather,DIR,$(PARENT-$2)))))
endef

define gather_dir =
$(eval
.SECONDEXPANSION:
$$(GATHER_DIR): | $$$$(PARENT-$$$$@); mkdir $$@)
endef

define gather_clean_dir
$(eval
.PHONY: $$(GATHER_CLEAN_DIR)
.SECONDEXPANSION:
$$(GATHER_CLEAN_DIR): clean-%: | $$$$(CLEAN-%)
    $$(if $$(wildcard $$*),$$(if $$(wildcard $$*/*),,rmdir $$*)))
endef

define gather_clean_file =
$(eval
.PHONY: $$(GATHER_CLEAN_FILE)
.SECONDEXPANSION:
$$(GATHER_CLEAN_FILE): clean-%: | $$$$(CLEAN-%)
    $$(if $$(wildcard $$*),rm $$*))
endef

GATHER_DIR :=
GATHER_CLEAN_DIR :=
GATHER_CLEAN_FILE :=

$(call foreachl,2,feval,DIR/$$2 := $(BUILD_PATH)/$$2,$(TOOLCHAIN))
$(call foreachl,3,proxycall,gather,DIR$(COMMA)$$(DIR/$$3),$(TOOLCHAIN))