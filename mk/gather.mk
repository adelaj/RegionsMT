define make_dir =
$(eval 
$1:
    mkdir $$@)
endef

define remove_dir_opaq =
$(eval
.PHONY: $1
$1:
    $$(if $$(shell [ -d "$$%" ] && echo .),rm -r $$%))
endef

# Warning! Obvious usage of the function '$(wildcard $$%/*)' gives undesired result: removed files may be still listed!
define remove_dir =
$(eval
.PHONY: $1
$1:
    $$(if $$(shell [ -d "$$%" ] && [ -z "$$$$(ls -A $$%)" ] && echo .),rmdir $$%))
endef

define remove_file =
$(eval
.PHONY: $1
$1:
    $$(if $$(shell [ -f "$$%" ] && echo .),rm $$%))
endef

parent = $(addprefix $(PREFIX)/,$(filter-out .,$(patsubst %/,%,$(dir $(patsubst $(PREFIX)/%,%,$1)))))

gather = $(if $(call var_base_decl,$1,$$(filter-out $$(GATHER),$$1),GATHER),$(info GATHERED FOR BUILD: $1)$(if $2,$(call $2,$1))$(call gather_dir,$1,$(call parent,$1)))
gather_dir = $(if $2,$(eval $1: | $2)$(call gather,$2,make_dir))

clean = $(if $(call var_base_decl,$1,$$(filter-out $$(CLEAN),$$1),CLEAN),$(info GATHERED FOR CLEAN: $1)$(eval clean: | clean($1))$(if $2,$(call $2,clean($1)))$(call clean_dir,$1,$(call parent,$1)))
clean_dir = $(if $2,$(eval clean($2): | clean($1))$(call clean,$2,remove_dir))


