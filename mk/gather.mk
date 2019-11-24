make_dir = $(eval $1:; mkdir $$@)

define noop =
$(eval
.PHONY: $1
$1:;)
endef

define rm-r =
$(eval
.PHONY: $1
$1:; rm -r $$(%%))
endef

define rm-rf =
$(eval
.PHONY: $1
$1:; rm -rf $$(%%))
endef

define rmdir =
$(eval
.PHONY: $1
$1:; rmdir $$(%%))
endef

define rm =
$(eval
.PHONY: $1
$1:; rm $$(%%))
endef

parent = $(addprefix $(PREFIX)/,$(filter-out .,$(patsubst %/,%,$(dir $(patsubst $(PREFIX)/%,%,$1)))))

gather = $(call vect,1,$1,$2,gather_base)

gather_base = $(if $(call var_base_decl,$1,$$(filter-out $$(GATHER),$$1),GATHER),\
$(info Gathered for BUILD: $1)$(if $2,$(call $2,$1))$(call gather_dir,$1,$(call parent,$1)))

gather_dir = $(if $2,\
$(eval $1: | $2)$(call gather_base,$2,make_dir))

clean = $(call vect,1,$1,$2,$3,clean_base)

clean_base = $(if $(call var_base_decl,$1,$$(filter-out $${CLEAN:$3},$$1),CLEAN:$3),\
$(info Gathered for CLEAN: $1)\
$(call var_base,$2,,CLEAN:$3:$1)\
$(call clean_dir,$1,$(call parent,$1),$3))

clean_dir = $(if $2,\
$(call var_base,$1,$1,CHILD:$3:$2)\
$(call clean_base,$2,rmdir,$3))

do_clean_base =\
$(if $(call var_base_decl,$1,$$(filter-out $${CLEANED:$$2},$$1),CLEANED:$2),\
$(eval $$(2): | $$(2)($$1))\
$(if $(wildcard $1),\
$(if $(call coalesce,CHILD:$2:$1,),\
$(eval $$(2)($$1): | $$(patsubst %,$(2)(%),$${CHILD:$$2:$$1}))\
$(call vect,1,${CHILD:$2:$1},clean_setup)\
$(if $(or $(filter $(call coalesce,DIRTY:$2,),${CHILD:$2:$1}),$(filter-out ${CHILD:$2:$1},$(wildcard $1/*))),$(call var_base,$1,$1,DIRTY:$2)$(eval $$(2)($$1):;),$(call ${CLEAN:$2:$1},$(2)($1))),\
$(call ${CLEAN:$2:$1},$(2)($1))),\
$(eval $$(2)($$1):;)))

do_clean = $(call vect,2 1,$(call coalesce,CLEAN:$1,),$1,do_clean_base)
