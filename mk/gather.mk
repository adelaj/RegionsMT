make_dir = $(eval $1:; mkdir $$@)

define rm-r =
$(eval
.PHONY: $1
$1:; rm -r $$%)
endef

define rm-rf =
$(eval
.PHONY: $1
$1:; rm -rf $$%)
endef

define rmdir =
$(eval
.PHONY: $1
$1:; rmdir $$%)
endef

define rm =
$(eval
.PHONY: $1
$1:; rm $$%)
endef

parent = $(addprefix $(PREFIX)/,$(filter-out .,$(patsubst %/,%,$(dir $(patsubst $(PREFIX)/%,%,$1)))))

gather = $(call vect,1,$1,$2,gather_base)

gather_base = $(if $(call var_base_decl,$1,$$(filter-out $$(GATHER),$$1),GATHER),\
$(info Gathered for BUILD: $1)$(if $2,$(call $2,$1))$(call gather_dir,$1,$(call parent,$1)))

gather_dir = $(if $2,\
$(eval $1: | $2)$(call gather_base,$2,make_dir))

clean = $(call vect,1,$1,$2,clean_base)

clean_base = $(if $(call var_base_decl,$1,$$(filter-out $$(CLEAN),$$1),CLEAN),\
$(info Gathered for CLEAN: $1)\
$(call var_base,$2,,CLEAN:$1)\
$(call clean_dir,$1,$(call parent,$1)))

clean_dir = $(if $2,\
$(call var_base,$1,$1,CHILD:$2)\
$(call clean_base,$2,rmdir))

do_clean_base =\
$(if $(call var_base_decl,$1,$$(filter-out $$(CLEAN_SETUP),$$1),CLEAN_SETUP),\
$(eval clean: | clean($$1))\
$(if $(wildcard $1),\
$(if $(call coalesce,CHILD:$1,),\
$(eval clean($$1): | $$(patsubst %,clean(%),$$(CHILD:$$1)))\
$(call vect,1,$(CHILD:$1),clean_setup)\
$(if $(or $(filter $(call coalesce,DIRTY,),$(CHILD:$1)),$(filter-out $(CHILD:$1),$(wildcard $1/*))),$(call var_base,$1,$1,DIRTY)$(eval clean($$1):;),$(call $(CLEAN:$1),clean($1))),\
$(call $(CLEAN:$1),clean($1))),\
$(eval clean($$1):;)))

do_clean = $(call vect,1,$(call coalesce,CLEAN,),do_clean_base)
