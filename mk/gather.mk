include mk/common.mk
include mk/env.mk

gather = $(call __gather,$2)\
$(call var_base,CLEAN_RANGE,$1,+=)\
$(call __clean,$1,$2)$2

__gather = $(if $(filter undefined,$(flavor PARENT:$1)),\
$(eval PARENT$$(COL)$$1 := $$(addprefix $$(PREFIX)/,$$(filter-out .,$$(patsubst %/,%,$$(dir $$(patsubst $(PREFIX)/%,%,$$1))))))\
$(call var_base,GATHER_DIR,$(filter-out $(if $(filter-out undefined,$(flavor GATHER_DIR)),$(GATHER_DIR)),$(PARENT:$1)),+=)\
$(if $(PARENT:$1),$(call __gather,$(PARENT:$1))))

__clean = $(if $(and $(PARENT:$2),$(or $(filter undefined,$(flavor CLEAN:$1:$(PARENT:$2))),$(filter-out $(CLEAN:$1:$(PARENT:$2)),$2))),\
$(call var_base,CLEAN,$1,$(PARENT:$2),$2,+=)\
$(call __clean,$1,$(PARENT:$2)))

$(call var_base,GATHER_FILE,$(call gather,1,$(PREFIX)/10/9/8/7/6/5/4/3/2/1),+=)
$(call var_base,GATHER_FILE,$(call gather,1,$(PREFIX)/10/9/80/7/6/5/4/3),+=)
$(call var_base,GATHER_FILE,$(call gather,1,$(PREFIX)/10/9/90),+=)

.PHONY: a(x)
a(x):
    @echo $% 

a(x): | b b b

a(x): | c c c 

b c:
    @echo $@


define gather_dir =
$(eval
$$1:
    mkdir $$@)
endef

define clean_dir_opaq =
$(eval
.PHONY: $$(1:$$(PERC)=clean$$(LP)$$(PERC)$$(RP))
$$(1:$$(PERC)=clean$$(LP)$$(PERC)$$(RP)):
    $$(if $$(shell rm -r $$% 2>&1 || echo .),@echo rm -r $$%))
endef

define clean_dir =
$(eval
.PHONY: $$(1:$$(PERC)=clean$$(LP)$$(PERC)$$(RP))
$$(1:$$(PERC)=clean$$(LP)$$(PERC)$$(RP)):
    $$(if $$(shell rmdir $$% 2>&1 || echo .),@echo rmdir $$%))
endef

define clean_file =
$(eval
.PHONY: $$(1:$$(PERC)=clean$$(LP)$$(PERC)$$(RP))
$$(1:$$(PERC)=clean$$(LP)$$(PERC)$$(RP)):
    $$(if $$(shell rm $$% 2>&1 || echo .),@echo rm $$%))
endef

define prereq =
$(eval
$$1: | $$$$(PARENT$$$$(COL)$$$$@)

$$(1:$$(PERC)=clean$$(LP)$$(PERC)$$(RP)): | $$$$(foreach i,$$$$(filter $$$$(CLEAN_RANGE),$$$$(CLEAN)),$$$$(if $$$$(filter-out undefined,$$$$(flavor CLEAN$$$$(COL)$$$$i$$$$(COL)$$$$%)),$$$$(patsubst $$$$(PERC),clean$$$$(LP)$$$$(PERC)$$$$(RP),$$$$(CLEAN$$$$(COL)$$$$i$$$$(COL)$$$$%))))

)
endef

$(warning  $(foreach i,$(filter $(CLEAN_RANGE),$(CLEAN)),$(if $(filter-out undefined,$(flavor CLEAN:$i:../10)),$(patsubst %,clean(%),$(CLEAN:$i:../10)))))

$(call gather_dir,$(GATHER_DIR))
$(call clean_dir,$(GATHER_DIR))
$(call clean_file,$(GATHER_FILE))

.SECONDEXPANSION:
$(call prereq,$(GATHER_FILE) $(GATHER_DIR))

