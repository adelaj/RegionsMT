define gather =
$(call var_base,$1,,+=)
$(if $(filter-out $($1),$2),
$(call var_base,$1,$2,+=)
$(call var_base,PARENT,$2,$(addprefix $(PREFIX)/,$(filter-out .,$(patsubst %/,%,$(dir $(patsubst $(PREFIX)/%,%,$2))))),:=)
$(if $(and $(PARENT:$2),$(filter-out $(CLEAN:$2) $2)),
$(call var_base,CLEAN,$(PARENT:$2),$2,+=)
$(call gather,GATHER_DIR,$(PARENT:$2))))
endef

define clean =
$(if $(or $(filter undefined,$(flavor CLEAN:$1:$(PARENT:$2))),$(filter-out,$(CLEAN:$1:$(PARENT:$2)),$2)),
$(call var_base,CLEAN,$1,$(PARENT:$2),$2,+=)
$(call clean,$1,$(PARENT:$2)))
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

decorate = $(1:%=$2$(LP)%$(RP))
clean2 = $$(1:$$(PERC)=clean$$(LP)$$(PERC)$$(RP))

define gather_rm_r =
$(eval
.PHONY: $(clean2)
$(clean2): | $$$$(CLEAN$$$$(COL)$$$$%)
    $$(if $$(shell [ -d "$$%" ] && echo $$%),rm -r $$%))
endef

# 'wildcard' is not reliable and should not be used
define gather_rmdir =
$(eval
.PHONY: $(clean2)
$(clean2): | $$$$(CLEAN$$$$(COL)$$$$%)
    $$(if $$(shell [ -d "$$%" ] && [ -z "`ls -qAL $$%`" ] && echo $$%),rmdir $$%))
endef

define gather_rm =
$(eval
.PHONY: $(clean2)
$(clean2): | $$$$(CLEAN$$$$(COL)$$$$%)
    $$(if $$(shell [ -f "$$%" ] && echo $$%),rm $$%))
endef
