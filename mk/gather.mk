gather = $(call __gather,$2)$(call __clean,$1,$2)$2

__gather = $(if $(filter undefined,$(flavor PARENT:$1)),\
$(eval PARENT$$(COL)$1 := $(addprefix $(PREFIX)/,$(filter-out .,$(patsubst %/,%,$(dir $(patsubst $(PREFIX)/%,%,$1))))))\
$(call gather,$1,$(PARENT:$1)))

__clean = $(if $(or $(filter undefined,$(flavor CLEAN:$1:$(PARENT:$2))),$(filter-out,$(CLEAN:$1:$(PARENT:$2)),$2)),\
$(call var_base,CLEAN,$1,$(PARENT:$2),$2,+=)\
$(call clean,$1,$(PARENT:$2)))


define clean_dir_opaq =
$(eval
.PHONY: $$(1:$$(PERC)=clean$$(LP)$$(PERC)$$(RP))
$$(1:$$(PERC)=clean$$(LP)$$(PERC)$$(RP)):
    $$(if $$(shell [ -d "$$%" ] && echo $$%),rm -r $$%))
endef

# 'wildcard' is not reliable and should not be used inside 'if' clause
define clean_dir =
$(eval
.PHONY: $$(1:$$(PERC)=clean$$(LP)$$(PERC)$$(RP))
$$(1:$$(PERC)=clean$$(LP)$$(PERC)$$(RP)):
    $$(if $$(shell [ -d "$$%" ] && [ -z "`ls -A $$%`" ] && echo $$%),rmdir $$%))
endef

define clean_file =
$(eval
.PHONY: $$(1:$$(PERC)=clean$$(LP)$$(PERC)$$(RP))
$$(1:$$(PERC)=clean$$(LP)$$(PERC)$$(RP)):
    $$(if $$(shell [ -f "$$%" ] && echo $$%),rm $$%))
endef

define gather_dir =
$(eval
$$1: | $$$$(PARENT$$$$(COL)$$$$@)
    mkdir $$@)
endef

define clean_prereq =
$(eval
$$(1:$$(PERC)=clean$$(LP)$$(PERC)$$(RP)): | $$$$(for i,$$$$(CLEAN),$$$$(if $$$$(filter-out undefined,$$$$(flavor CLEAN$$$$(COL)$$$$i$$$$(COL)$$$$%)),$$$$(CLEAN$$$$(COL)$$$$i$$$$(COL)$$$$%:$$$$(PERC)=clean$$$$(LP)$$$$(PERC)$$$$(RP)))))
endef
