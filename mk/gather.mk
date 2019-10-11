define gather =
$(call var_base,GATHER_$1,,0)
$(if $(filter $2,$(GATHER_$1)),,
$(call var_base,CLEAN,$2,,0)
$(call var_base,GATHER_$1,$2,0)
$(call var_base,GATHER_CLEAN_$1,clean($2),0)
$(call var_base,PARENT,$2,$(addprefix $(PREFIX)/,$(filter-out .,$(patsubst %/,%,$(dir $(patsubst $(PREFIX)/%,%,$2))))),0)
$(if $(PARENT:$2),
$(call var_base,CLEAN,$(PARENT:$2),clean($2),0)
$(call gather,DIR,$(PARENT:$2))))
endef

define gather_dir =
$(call var_base,GATHER_DIR,,0)\
$(eval
.SECONDEXPANSION:
$$(GATHER_DIR): | $$$$(PARENT$$$$(COL)$$$$@); mkdir $$@)
endef

define gather_clean_dist_dir =
$(call var_base,GATHER_CLEAN_DIST_DIR,,0)\
$(eval
.PHONY: $$(GATHER_CLEAN_DIST_DIR)
.SECONDEXPANSION:
$$(GATHER_CLEAN_DIST_DIR): clean(%): | $$$$(CLEAN$$$$(COL)%)
    $$(if $$(wildcard $$*),rm -r $$*))
endef

define gather_clean_dir =
$(call var_base,GATHER_CLEAN_DIR,,0)\
$(eval
.PHONY: $$(GATHER_CLEAN_DIR)
.SECONDEXPANSION:
$$(GATHER_CLEAN_DIR): clean(%): | $$$$(CLEAN$$$$(COL)%)
    $$(if $$(wildcard $$*),$$(if $$(wildcard $$*/*),,rmdir $$*)))
endef

define gather_clean_file =
$(call var_base,GATHER_CLEAN_FILE,,0)\
$(eval
.PHONY: $$(GATHER_CLEAN_FILE)
.SECONDEXPANSION:
$$(GATHER_CLEAN_FILE): clean(%): | $$$$(CLEAN$$$$(COL)%)
    $$(if $$(wildcard $$*),rm $$*))
endef
