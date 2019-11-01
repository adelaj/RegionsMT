make_dir = $(eval $1:; mkdir $$@)

define rm-r =
$(eval
.PHONY: $1
$1:
    rm -r $$%)
endef

define rm-rf =
$(eval
.PHONY: $1
$1:
    rm -rf $$%)
endef

define rmdir =
$(eval
.PHONY: $1
$1:
    rmdir $$%)
endef

define rm =
$(eval
.PHONY: $1
$1:
    rm $$%)
endef

parent = $(addprefix $(PREFIX)/,$(filter-out .,$(patsubst %/,%,$(dir $(patsubst $(PREFIX)/%,%,$1)))))

gather = $(if $(call var_base_decl,$1,$$(filter-out $$(GATHER),$$1),GATHER),\
$(info Gathered for BUILD: $1)$(if $2,$(call $2,$1))$(call gather_dir,$1,$(call parent,$1)))

gather_dir = $(if $2,\
$(eval $1: | $2)$(call gather,$2,make_dir))

clean = $(if $(call var_base_decl,$1,$$(filter-out $$(CLEAN),$$1),CLEAN),\
$(info Gathered for CLEAN: $1)\
$(call var_base,$2,,CLEAN:$1)\
$(call clean_dir,$1,$(call parent,$1)))

clean_dir = $(if $2,\
$(call var_base,$1,$1,CHILD:$2)\
$(call clean,$2,rmdir))

do_clean =\
$(foreach i,$(call coalesce,CLEAN,),clean($i)\
$(if $(wildcard $i),\
$(if $(call coalesce,CHILD:$i,),\
$(eval clean($$i): | $$(patsubst %,clean(%),$$(CHILD:$$i)))\
$(if $(filter-out $(CHILD:$i),$(wildcard $i/*)),$(eval clean($$i):;),$(if $(CLEAN:$i),$(call $(CLEAN:$i),clean($i)))),\
$(if $(CLEAN:$i),$(call $(CLEAN:$i),clean($i)))),$(eval clean($$i):;)))

