define gather =
$(call var_base,$1,,+=)
$(if $(filter $2,$($1)),,
$(call var_base,CLEAN,$2,,+=)
$(call var_base,$1,$2,+=)
$(call var_base,PARENT,$2,$(addprefix $(PREFIX)/,$(filter-out .,$(patsubst %/,%,$(dir $(patsubst $(PREFIX)/%,%,$2))))),:=)
$(if $(PARENT:$2),
$(call var_base,CLEAN,$(PARENT:$2),clean($2),+=)
$(call gather,GATHER_DIR,$(PARENT:$2))))
endef

define gather_file =
$(eval
$$1: | $$$$(PARENT$$$$(COL)$$$$@))
endef

define gather_mkdir =
$(eval
$$1: | $$$$(PARENT$$$$(COL)$$$$@)
    mkdir $$@)
endef

clean = $(1:%=clean$(LP)%$(RP))
clean2 = $$(1:$$(PERC)=clean$$(LP)$$(PERC)$$(RP))

define gather_rm_r =
$(eval
.PHONY: $(clean2)
$(clean2): | $$$$(CLEAN$$$$(COL)$$$$%)
    $$(if $$(wildcard $$%),rm -r $$%))
endef

define gather_rmdir =
$(eval
.PHONY: $(clean2)
$(clean2): | $$$$(CLEAN$$$$(COL)$$$$%)
    $$(if $$(wildcard $$%),$$(if $$(wildcard $$%/*),,rmdir $$%)))
endef

define gather_rm =
$(eval
.PHONY: $(clean2)
$(clean2): | $$$$(CLEAN$$$$(COL)$$$$%)
    $$(if $$(wildcard $$%),rm $$%))
endef
