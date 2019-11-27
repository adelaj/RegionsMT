mkdir = $(eval $1:; mkdir $$@)

define noop =
$(eval
.PHONY: $1
$1:;)
endef

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

gather_base = $(if $(call var_base_decl,$1,$$(filter-out $$($$3),$$1),GATHER),\
$(if $2,$(call $2,$1))$(call gather_dir,$1,$(call parent,$1)))

gather_dir = $(if $2,\
$(eval $1: | $2)$(call gather_base,$2,mkdir))

clean = $(call vect,1,$1,$2,$3,clean_base)

clean_base = $(if $(call var_base_decl,$1,$$(filter-out $${$$3},$$1),CLEAN:$3),\
$(call var_base,$2,$2,LEAF:$3:$1)\
$(call clean_dir,$1,$(call parent,$1),$3))

clean_dir = $(if $2,\
$(call var_base,$1,$1,CHILD:$3:$2)\
$(call clean_base,$2,,$3))

do_clean_base =\
$(if $(call var_base_decl,$1,$$(filter-out $$($$3),$$1),DO_CLEAN),\
$(eval clean: | clean($$1))\
$(if $(wildcard $1),\
$(if $(call var_base_decl,$(foreach i,$(CLEAN),$(call coalesce,CHILD:$i:$1,)),,CHILD:%:$1),\
$(eval clean($$1): | $$(patsubst %,clean(%),$$(CHILD:%:$$1)))\
$(call vect,1,$(CHILD:%:$1),do_clean_base)\
$(if $(or $(filter $(call coalesce,DIRTY,),$(CHILD:%:$1)),$(filter-out $(CHILD:%:$1),$(wildcard $1/*))),\
$(call var_base,$1,$1,DIRTY)$(call noop,clean($1)),\
$(call rmdir,clean($1))),\
$(call $(lastword noop $(foreach i,$(CLEAN),$(call coalesce,LEAF:$i:$1,))),clean($1))),\
$(call noop,clean($1))))

do_clean = $(call vect,1,$(foreach i,$(CLEAN),$(call coalesce,CLEAN:$i,)),do_clean_base)
