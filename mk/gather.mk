define gather =
$(eval
$(if $(filter $2,$(GATHER_$1)),,
CLEAN$$(COL)$2 :=
GATHER_$1 += $2
GATHER_CLEAN_$1 += clean($2)
$(eval 
PARENT$$(COL)$2 := $(addprefix $(PREFIX)/,$(filter-out .,$(patsubst %/,%,$(dir $(patsubst $(PREFIX)/%,%,$2))))))
$(if $(PARENT:$2),
CLEAN$$(COL)$(PARENT:$2) += clean($2)
$(call gather,DIR,$(PARENT:$2)))))
endef

define gather_dir =
$(eval
.SECONDEXPANSION:
$$(GATHER_DIR): | $$$$(PARENT$$$$(COL)$$$$@); mkdir $$@)
endef

define gather_clean_dist_dir =
$(eval
.PHONY: $$(GATHER_CLEAN_DIST_DIR)
.SECONDEXPANSION:
$$(GATHER_CLEAN_DIST_DIR): clean(%): | $$$$(CLEAN$$$$(COL)%)
    $$(if $$(wildcard $$*),rm -r $$*))
endef

define gather_clean_dir
$(eval
.PHONY: $$(GATHER_CLEAN_DIR)
.SECONDEXPANSION:
$$(GATHER_CLEAN_DIR): clean(%): | $$$$(CLEAN$$$$(COL)%)
    $$(if $$(wildcard $$*),$$(if $$(wildcard $$*/*),,rmdir $$*)))
endef

define gather_clean_file =
$(eval
.PHONY: $$(GATHER_CLEAN_FILE)
.SECONDEXPANSION:
$$(GATHER_CLEAN_FILE): clean(%): | $$$$(CLEAN$$$$(COL)%)
    $$(if $$(wildcard $$*),rm $$*))
endef

GATHER_DIR :=
GATHER_FILE :=
GATHER_CLEAN_DIST_DIR :=
GATHER_CLEAN_DIR :=
GATHER_CLEAN_FILE :=
